# Appendix C — BCC Tool Development

*Study notes on Appendix C of BPF Performance Tools (Brendan Gregg, 2019). Original notes, not book text — see `00-index.md` for how these notes are written.*

> Explicitly optional, hands-on material extending Chapter 4 — a "crash course by example" building four real BCC programs in increasing complexity: `hello_world.py` (minimal skeleton), `sleepsnoop.py` (per-event output via the perf buffer), `bitehist.py` (in-kernel histogram + struct-argument gotchas), and a full annotated walkthrough of the real `biolatency.py` shipped elsewhere in this book. If you're going to write your own BCC tool rather than just run existing ones, this is the appendix to actually work through line by line yourself, not just read.

## Five tips worth internalizing before writing any BCC C code

1. **BPF C is a restricted dialect**: no loops, no arbitrary kernel function calls — only the `bpf_*` helper functions (Chapter 2 §2.3.6) and a handful of compiler built-ins.
2. **All memory access goes through `bpf_probe_read()`**, and memory can only be read onto the small BPF stack or into BPF maps. **You usually don't have to call this yourself**: write ordinary-looking dereferences (`a->b->c->d`) and let BCC's rewriter (Chapter 2 §2.3.3) try to convert them automatically first — only fall back to explicit `bpf_probe_read()` calls if the rewriter can't handle a particular case.
3. **Three ways to get data from kernel to user space**, worth choosing deliberately rather than defaulting to the first one you learn: `BPF_PERF_OUTPUT()` (per-event detail via a custom struct — used by `sleepsnoop.py` below); `BPF_HISTOGRAM()` or other BPF maps (summary statistics, read periodically from user space — efficient, used by `bitehist.py` and `biolatency.py`); and `bpf_trace_printk()` (debug-only, writes to the **shared** `trace_pipe` buffer — can clash with other tools reading the same pipe, exactly as flagged back in Chapter 4 §4.12.1).
4. **Prefer static instrumentation (tracepoints, USDT) over dynamic (kprobes, uprobes)** wherever available — the single most repeated piece of advice in this entire book, restated here as tip #4 of 5 for tool authors specifically.
5. **Check for new BCC and bpftrace capabilities before building something from scratch** — the ecosystem keeps adding features that might make a planned workaround unnecessary.

## Tool 1: `hello_world.py` — the minimal skeleton

The entire kernel-side program is one function:
```c
int kprobe__do_nanosleep()
{
    bpf_trace_printk("Hello, World!\n");
    return 0;
}
```
**Two things worth understanding precisely, since they're easy to skim past**: the `kprobe__` prefix on a function name is a **shortcut** — BCC treats anything named `kprobe__X` as "attach this function as a kprobe on kernel function `X`" automatically, no explicit `attach_kprobe()` call needed (though the book notes most real tools use the explicit call instead, mostly because they predate this shortcut existing). And `b.trace_print()` — the one line of the Python side actually doing something — simply reads and prints the shared debug trace buffer, the exact same `trace_pipe` mechanism from Chapter 4's debugging section, here used as the tool's actual (debug-only-grade) output mechanism rather than a troubleshooting aid.

## Tool 2: `sleepsnoop.py` — the *recommended* per-event output pattern

Same target function, but now using the real, production-appropriate output mechanism instead of the shared debug pipe:
```c
struct data_t {
    u64 ts;
    u32 pid;
};
BPF_PERF_OUTPUT(events);

int kprobe__do_nanosleep(void *ctx) {
    struct data_t data = {};
    data.pid = bpf_get_current_pid_tgid();
    data.ts = bpf_ktime_get_ns() / 1000;
    events.perf_submit(ctx, &data, sizeof(data));
    return 0;
};
```
Worth understanding each moving part of this as a template you'll reuse for essentially every per-event BCC tool: a plain C `struct` defines exactly what one event record looks like; `BPF_PERF_OUTPUT(events)` declares a named perf ring buffer to carry that struct to user space; the struct **must be zero-initialized** (`= {}`) since the verifier rejects any read of uninitialized memory; and `events.perf_submit()` is the actual handoff — one call, at the end of the kernel-side function, submitting one filled-in struct as one event.

On the Python side, `b["events"].open_perf_buffer(print_event)` registers a callback, and a `while True: b.perf_buffer_poll()` loop (with a `KeyboardInterrupt` handler for clean Ctrl-C exit) drives it — worth remembering as the standard event-loop shape for *any* perf-buffer-based BCC tool, not just this one. **A genuinely practical performance note worth carrying forward**: for high-frequency events, some real tools add a small `sleep()` inside this polling loop specifically to let several events batch up before Python wakes to process them — trading a little latency for meaningfully less Python-side wake-up overhead. **And the section's own closing line is worth remembering as the direct bridge to the next tool**: if events are frequent enough that this overhead becomes a real concern, ask whether the question can instead be answered by **summarizing in kernel context** — which is exactly what `bitehist.py` demonstrates next.

## Tool 3: `bitehist.py` — in-kernel histograms, and a real struct-argument-matching gotcha

```c
BPF_HISTOGRAM(dist);

int kprobe__blk_account_io_completion(struct pt_regs *ctx,
    void *req, unsigned int bytes)
{
    dist.increment(bpf_log2l(bytes / 1024));
    return 0;
}
```
`BPF_HISTOGRAM(dist)` declares a map shaped specifically for power-of-two histogram storage; `bpf_log2l()` converts a raw value into the correct bucket index, and `.increment()` bumps that bucket — the exact in-kernel-summarization pattern Chapter 2 §2.3.1 explained in the abstract, now shown as three real lines of C. The Python side barely does anything at all beyond sleeping and eventually calling `b["dist"].print_log2_hist("kbytes")` — worth noticing as a direct, concrete illustration of *how little* user-space code a well-designed BPF tool actually needs, since virtually all the real work already happened in-kernel.

**The genuinely instructive part of this tool, worth understanding in full, is the `void *req` argument** — because a kprobe'd function's arguments are read positionally off CPU registers according to the calling convention (`%rdi`, `%rsi`, `%rdx`, ... on x86_64), you must declare **every** preceding argument in your function signature, even ones you never use, just to keep the register positions correctly aligned for the argument you actually want (`bytes`, here). The real kernel function is `blk_account_io_completion(struct request *req, unsigned int bytes)` — but `struct request` isn't a type BPF knows about by default, and declaring it without an `#include` would fail to compile. **Two real fixes, worth remembering as a pair of genuinely different tradeoffs**: `#include <linux/blkdev.h>` (bringing in the real struct definition, letting you actually dereference its members later if you want to), or simply declaring the unused argument as `void *req` instead (compiles fine, since `void*` is always known — but you lose the ability to read anything out of that struct, since you've deliberately discarded its real type information). The book's own alternate version of this same tool shows the **first** approach in action — `req->__data_len` read directly, after including the real header — demonstrating that the choice is a genuine, situational tradeoff (convenience now vs. flexibility later), not a right-vs-wrong pick.

**A related, important question the book raises and answers directly, worth remembering as real context for why BTF matters (Chapter 2 §2.3.9)**: *why do you have to redeclare a function's own signature at all — doesn't the kernel already know it?* Yes, **if** matching kernel debuginfo happens to be installed — which, the book notes, is rarely the case in practice, since debuginfo packages are large. This is presented here as a direct, concrete illustration of exactly the problem BTF is meant to eventually solve — letting tools discover a function's real argument types automatically, without needing either debuginfo or manual, error-prone re-declaration (get the signature wrong, and the tool will still **compile successfully**, silently reading garbage from the wrong register).

## Tool 4: `biolatency.py` — a real, complete, "gritty" production tool, annotated in full

The book walks its own actual `biolatency.py` source end to end, explicitly calling it "a little gritty, which is both good (realistic) and bad (intimidating)" — worth reading as the payoff of the first three, simpler examples, now applied to genuine production code rather than a teaching toy.

**Header comment conventions worth adopting for your own tools**: tool name plus a one-sentence description, a platform/technology caveat line, a usage synopsis, copyright, and a changelog — the same convention you've now seen implicitly followed by every tool shown throughout Chapters 6–17.

**Argument parsing** uses Python's `argparse`, deliberately styled to resemble familiar Unix utilities (`vmstat(8)`, `iostat(1)`) — worth remembering as a deliberate design choice in service of Chapter 4's own "make it recognizable and easy to learn" tool philosophy, not an incidental implementation detail.

**The "code that writes code" templating technique — worth understanding as the single most instructive idea in this whole appendix.** `biolatency.py` needs to behave differently depending on its `-D` (per-disk) and `-m` (milliseconds) flags, but the book states directly it wants to avoid *actual* code generation complexity wherever possible. The solution: write the BPF C program as a Python string containing **placeholder tokens** — `STORAGE`, `STORE`, `FACTOR` — then use plain Python `.replace()` calls, chosen based on the parsed command-line flags, to substitute in the real code before ever compiling it:
```python
if args.milliseconds:
    bpf_text = bpf_text.replace('FACTOR', 'delta /= 1000000;')
else:
    bpf_text = bpf_text.replace('FACTOR', 'delta /= 1000;')

if args.disks:
    bpf_text = bpf_text.replace('STORAGE', 'BPF_HISTOGRAM(dist, disk_key_t);')
    bpf_text = bpf_text.replace('STORE',
        'disk_key_t key = {.slot = bpf_log2l(delta)}; '
        'bpf_probe_read(&key.disk, sizeof(key.disk), '
        'req->rq_disk->disk_name); dist.increment(key);')
else:
    bpf_text = bpf_text.replace('STORAGE', 'BPF_HISTOGRAM(dist);')
    bpf_text = bpf_text.replace('STORE', 'dist.increment(bpf_log2l(delta));')
```
This is worth remembering as a genuinely reusable pattern for **any** BCC tool whose kernel-side logic needs to vary by command-line option: keep the C source as one readable template with a small number of named placeholders, and let ordinary Python string substitution assemble the final variant — simpler to read and debug than actually generating code structurally, at the cost of needing a `if debug: print(bpf_text)` escape hatch (which the tool includes directly) to see what you actually built when something goes wrong.

**The core timing logic is the same start/lookup/delete pattern you've now seen in bpftrace form throughout this whole book, here shown as the real, raw C it compiles down to**:
```c
BPF_HASH(start, struct request *);

int trace_req_start(struct pt_regs *ctx, struct request *req) {
    u64 ts = bpf_ktime_get_ns();
    start.update(&req, &ts);
    return 0;
}

int trace_req_completion(struct pt_regs *ctx, struct request *req) {
    u64 *tsp, delta;
    tsp = start.lookup(&req);
    if (tsp == 0) { return 0; }   // missed issue
    delta = bpf_ktime_get_ns() - *tsp;
    FACTOR
    STORE
    start.delete(&req);
    return 0;
}
```
This is Chapter 5 §5.7.12's `vfs_read()` timer, Chapter 9's `biolatency(8)` struct-request-as-key technique, and Chapter 2's map-based state-passing all landing in one place, in the actual raw C form bpftrace's higher-level syntax exists specifically to spare you from writing by hand. **The `if (tsp == 0) { return 0; }` check is worth noting as the direct C equivalent of bpftrace's `/@start[tid]/` filter idiom** — both exist to handle exactly the same "I might see a completion with no matching recorded start" case Chapter 5 §5.17.1 walked through as a live debugging example.

**The `-Q` (queued time) option's implementation is a small, elegant illustration of a bigger idea worth remembering on its own**: rather than branching *inside* the traced function, `-Q` simply changes **which kernel function gets the entry probe attached to it** —
```python
if args.queued:
    b.attach_kprobe(event="blk_account_io_start", fn_name="trace_req_start")
else:
    b.attach_kprobe(event="blk_start_request", fn_name="trace_req_start")
    b.attach_kprobe(event="blk_mq_start_request", fn_name="trace_req_start")
b.attach_kprobe(event="blk_account_io_completion", fn_name="trace_req_completion")
```
This only works cleanly because **all three candidate entry functions happen to share the same first argument type** (`struct request *`) — letting one single `trace_req_start()` BPF function serve as the entry handler regardless of which specific kernel function actually triggered it. If the candidate functions' signatures had differed, this reuse trick wouldn't have been available, and separate BPF-side functions would have been needed for each. **The book's own honest aside here is worth remembering too**: this particular tool predates BPF's tracepoint support and was written entirely with kprobes — the book notes directly it "should be rewritten to use tracepoints" for exactly the stability reasons emphasized throughout this whole book, a small, candid admission that even the book's own flagship examples sometimes reflect their own history rather than the current best practice.

## More info

The appendix points to three maintained, external BCC resources worth bookmarking if you continue writing your own tools: the **BCC Python Developer Tutorial** (15+ progressively-detailed lessons), the **BCC Reference Guide** (a searchable, complete API reference for both the BPF C side and the Python side, with short examples for every capability), and a **contribution checklist** for anyone wanting to submit a new tool to the BCC project itself — all maintained in the `iovisor/bcc` repository.

---

## Quick-reference glossary (this appendix's terms)

- **`kprobe__` shortcut** — naming a BCC C function `kprobe__<name>` automatically attaches it as a kprobe on kernel function `<name>`, no explicit `attach_kprobe()` call needed.
- **`BPF_PERF_OUTPUT()`** — declares a perf ring buffer for submitting per-event, struct-shaped records to user space.
- **`BPF_HASH()` / `BPF_HISTOGRAM()`** — BCC's map-declaration macros for key/value storage and power-of-two histograms respectively.
- **Placeholder-token templating** — writing BPF C source as a string with named tokens (`STORAGE`, `STORE`, `FACTOR`), substituted via ordinary string replacement based on command-line options, as an alternative to structural code generation.

## Check your understanding

1. In `bitehist.py`, why does the kprobe'd function's signature need to declare a `struct request *req` argument it never actually uses inside the function body?
2. What's the real, concrete difference in capability between declaring that same argument as `void *req` versus `#include`-ing the real header and declaring it as `struct request *req`?
3. Why does `biolatency.py`'s `-Q` option change *which kernel functions get probed* rather than adding an `if` branch inside one single, already-probed function?
4. What problem does BTF (Chapter 2) promise to eventually solve that `bitehist.py`'s manual struct/signature redeclaration is a direct, present-day workaround for?
5. Why does the book prefer `sleepsnoop.py`'s `BPF_PERF_OUTPUT()`-based approach over `hello_world.py`'s `bpf_trace_printk()` approach for any tool beyond the simplest possible example?

<details>
<summary>Answers</summary>

1. Because kprobe function arguments are read positionally off the CPU's calling-convention registers, in the same order the real, traced kernel function receives them — to correctly read a later argument (like `bytes`, the second argument), every argument that comes *before* it in the real function's signature must also be declared in the BPF function's signature, purely to keep the register positions aligned, even if that earlier argument is never actually used inside the function body.
2. Declaring it as `void *req` compiles without needing any extra header, but the real type information is discarded — you cannot dereference any of its members. Declaring it as `struct request *req` (after including the header that defines that struct) requires an extra `#include` and makes the program dependent on that struct's definition being available, but in exchange lets you actually read fields out of the struct, such as `req->__data_len`.
3. Because the difference between the queued-time and issue-time measurement is really about *when the clock starts*, not about any different logic once it starts — attaching the same entry-timing BPF function to a different kernel function (one that fires when the I/O is queued vs. one that fires when it's actually issued to the device) achieves this directly, and works cleanly specifically because all the candidate kernel functions happen to share the same first argument type, letting one BPF function serve all of them without modification.
4. BTF promises to let tools discover a kernel function's real argument types and struct layouts automatically, without requiring either kernel debuginfo (which is often not installed, since debuginfo packages are large) or manual, error-prone redeclaration of structs and function signatures by the tool author — removing the exact problem `bitehist.py`'s `void *`/header-include workaround exists to route around.
5. Because `bpf_trace_printk()` writes into a single, shared trace buffer (`trace_pipe`) also used by other tracers, meaning its output can clash or interleave unpredictably with other tools reading that same buffer at the same time — it's explicitly meant for quick debugging only. `BPF_PERF_OUTPUT()` instead gives each tool its own dedicated, structured event channel, avoiding that interference and providing proper typed event records rather than a shared stream of arbitrary printed text.

</details>
