# Appendixes D & E — C BPF and BPF Instructions

*Study notes on Appendixes D–E of BPF Performance Tools (Brendan Gregg, 2019). Original notes, not book text — see `00-index.md` for how these notes are written.*

> The final two appendixes, and the deepest layer of the whole book — this is where the "you will never need to write this by hand" material from Chapter 2 §2.3.3 finally gets shown in full, for readers who want to see exactly what bpftrace and BCC are actually generating underneath. Appendix D shows real, complete BPF programs written directly in C (bypassing BCC/bpftrace entirely); Appendix E shows the literal instruction encoding one level below even that. Read together, they close the loop all the way from Chapter 5's `vfs_read()` one-liner down to individual opcode bytes.

## Appendix D: C BPF

### Why write raw C instead of BCC or bpftrace? — a genuine decision framework, not just nostalgia

The book gives four real reasons people still write BPF tools directly in C, each stated **with its own counterpoint**, worth reading as a balanced decision framework rather than a case for abandoning bpftrace/BCC:

- **Lower startup overhead** — a standalone compiled C binary has none of bpftrace's (~40 ms) or BCC's (~160 ms) compile-on-every-run cost. *Counterpoint*: this only matters if you're starting the tool frequently; the real fix in that case is often compiling a BPF object file once and re-loading (or "pinning," Chapter 2 §2.3.8) it, which Cilium and Cloudflare both already do in production via object-file templating.
- **No bulky compiler dependencies** — BCC/bpftrace need LLVM and Clang, which can add 80+ MB to a system and bring real API-churn maintenance burden (the book notes bpftrace's own development crossed LLVM versions 5 through 8) — genuinely prohibitive on constrained/embedded systems. *Counterpoint*: several projects (SystemTap's BPF backend, `ply`) were already working on lighter-weight compilation paths at the time of writing.
- **Lower runtime overhead** — mostly **not** a real advantage, the book says directly: the same BPF bytecode runs in the kernel regardless of which front end generated it, and any tool relying on in-kernel summarization (maps, histograms) already does the vast majority of its work with zero front-end CPU cost while running. The one case where it *can* matter: a tool printing thousands of events per second, where the front end's own event-processing loop becomes visible in `top(1)` — there, a C rewrite (or just tuning BCC's ring-buffer polling) can help.
- **"BPF hacking"** — anything the verifier accepts is fair game in raw C, a strictly larger capability surface than BCC/bpftrace expose. *Counterpoint*: BCC already accepts arbitrary C, making it hard to imagine a case this is actually necessary in practice.
- **A fifth, more durable reason**: using **`perf(1)`** directly, which has its own BPF integration for `perf record`/`perf trace` and may already be the best tool for a specific job (e.g., efficiently recording large volumes of events to a binary file) — covered in this appendix's own closing section.

**The book's own overall verdict, worth remembering as the practical takeaway**: bpftrace and BCC are expected to be sufficient for the large majority of tracing work; C is really the domain of **networking** BPF (Cilium and similar projects) more than tracing specifically.

### Two ways to specify a BPF program, and which one actually gets used

**Raw BPF instructions** — an array of `struct bpf_insn` embedded directly in a C program, passed to the `bpf(2)` syscall (Appendix E is the reference for reading these). **A real C program, compiled to BPF** — via LLVM's own BPF compilation target, the same way it compiles to x86 — stored in an ELF file, with the BPF program type as a named section. **The second is strongly preferred** in practice; the first is shown here (in `hello_world.c`) purely as a teaching example, since it's otherwise identical to how BCC/bpftrace themselves work internally.

### Tool 1: `hello_world.c` — actual raw BPF instructions, worth reading once for real

The entire "Hello, World!\n" string is built up **manually, four bytes at a time**, directly on the BPF stack, using `BPF_MOV64_IMM()` to load each 4-byte chunk as a 32-bit integer constant and `BPF_STX_MEM()` to store it — worth noticing that this is exactly the kind of tedious, error-prone work that C strings and `printf()`-style formatting exist specifically to spare you from, made visible here because there's no compiler between you and the instructions. The final call to `bpf_trace_printk()` is itself expressed as a raw instruction: `BPF_RAW_INSN(BPF_JMP | BPF_CALL, 0, 0, 0, BPF_FUNC_trace_printk)`. Two library calls do the real plumbing: `bpf_load_program()` (from `libbpf`) loads the instruction array and returns a program file descriptor; `bpf_attach_kprobe()` (from `libbcc`) attaches it to `do_nanosleep()` and returns a probe file descriptor.

**A real, concrete operational lesson worth remembering, directly echoing tool-hygiene advice from Chapter 4**: the program explicitly closes both file descriptors and detaches the kprobe before exiting — because, on kernels before Linux 5.2's move to fully file-descriptor-based probes, **skipping this cleanup can leave a kprobe enabled with no consumer reading its output**, costing overhead indefinitely until manually found (`cat /sys/kernel/debug/tracing/kprobe_events`, `bpftool prog show`) and removed (`reset-trace(8)`) — the exact same leftover-probe problem Chapter 4 §4.12 already covered as a debugging scenario, here shown from the tool-author's side as something your own cleanup code needs to actively prevent.

### Tool 2: `bigreads` — the real, split kernel/user-file structure

`bigreads` (equivalent to `bpftrace -e 'kr:vfs_read /retval > 1024*1024/ { printf("READ: %d bytes\n", retval); }'`) introduces the pattern **every real C BPF tool in this appendix follows**: a `_kern.c` file (compiled separately, targeting BPF, producing a `.o` object file) and a `_user.c` file (an ordinary native binary that loads that object file into the kernel and manages it). The kernel-side function reads a kretprobe's return value via `PT_REGS_RC(ctx)` — worth remembering as the raw-C equivalent of bpftrace's `retval` builtin — and, like `hello_world.c`, uses `bpf_trace_printk()` purely to keep the *example* short, with the book explicitly noting this should be replaced by `bpf_perf_event_output()` (a `BPF_MAP_TYPE_PERF_EVENT_ARRAY` map, exactly Chapter 2 §2.3.6's mechanism) for any real, production-grade tool — the debug-only-buffer caveat from Chapters 4, 12, and Appendix C, restated here at the lowest possible level.

### Tool 3: `bitehist` — the moment the concurrency lesson from Chapter 2 becomes literal, visible code

`bitehist` (the raw-C version of Appendix C's `bitehist.py`) declares an ordinary `BPF_MAP_TYPE_HASH` and looks up/updates a bucket count directly:
```c
value = bpf_map_lookup_elem(&hist_map, &key);
if (value)
        __sync_fetch_and_add(value, 1);
else
        bpf_map_update_elem(&hist_map, &key, &init_val, BPF_ANY);
```
**This is worth pausing on, since it's the one place in this entire book where the concurrency-safety mechanics from Chapter 2 §2.3.7 stop being described in the abstract and become a real, visible line of code you'd have to get right yourself.** Because this is an ordinary (non-per-CPU) hash map, a plain, non-atomic increment here would be exactly the kind of lost-update race Chapter 2 demonstrated concretely with the `@hash++` vs. `count()` comparison — which is precisely why the real code uses **`__sync_fetch_and_add()`**, a GCC/Clang atomic built-in, to perform the increment safely against concurrent access from other CPUs. **When you write in bpftrace or BCC and just call `count()` or `.increment()`, this exact correctness concern is being handled for you, invisibly** — seeing the raw version makes clear that it's a real problem being actively solved, not something that simply doesn't apply at this level.

### `perf C` — the third way to run a BPF program, briefly

`perf(1)` itself can run BPF programs via two of its own subcommands: `perf record` (attaching custom filters and emitting extra records into `perf.data`) and `perf trace` (using a BPF program purely to "beautify" trace output — e.g., showing a real filename string instead of a raw pointer). The book shows the identical `bigreads` logic re-expressed for `perf record` specifically, using `perf_event_output()` to emit raw byte data that `perf script` later dumps as a hex blob (`00 00 20 00 ...` — literally the bytes of `0x200000`, 2 MB, in little-endian) — worth noting mainly as evidence that `perf(1)` is a **third, real, independent front end** for running BPF programs, alongside BCC and bpftrace, though the book is candid that its BPF capabilities and documentation were both immature and rapidly changing at the time of writing.

## Appendix E: BPF Instructions

**Stated purpose, worth taking at face value**: just enough to *read* the instruction listings in Appendix D's `hello_world.c` and similar tool source — **not** a guide to writing raw instructions yourself, which the appendix explicitly says isn't recommended.

### The helper macros, and what they expand to

Every macro seen in `hello_world.c` (`BPF_MOV64_IMM`, `BPF_STX_MEM`, `BPF_ALU64_IMM`, `BPF_JMP_IMM`, `BPF_RAW_INSN`, `BPF_EXIT_INSN`, and others) is a thin C preprocessor wrapper that expands to a real `struct bpf_insn` value — the appendix's own reference table lists ~20 of these, covering ALU operations (register and immediate, 32- and 64-bit), moves, memory loads/stores (including an atomic `BPF_STX_XADD`, the raw-instruction-level version of the same atomic-add concern §D's `bitehist` example handles at the map-helper level), conditional and unconditional jumps, map operations, and calls (both to BPF helper functions and to other BPF functions directly).

### The actual instruction encoding — `struct bpf_insn`, laid out precisely

| Field | Opcode | Dest register | Source register | Signed offset | Signed immediate constant |
|---|---|---|---|---|---|
| Width | 8-bit | 8-bit | 8-bit | 16-bit | 32-bit |

Every BPF instruction is this fixed, five-field, 64-bit-total layout — worth remembering as the concrete answer to "what does a compiled BPF program actually consist of, physically." Instruction classes and fields (`BPF_ALU64`, `BPF_MOV`, `BPF_K` for an immediate operand vs. `BPF_X` for a register operand, and so on) are bitwise-OR'd together to build a complete opcode byte — e.g., `BPF_MOV64_IMM()`'s opcode is `BPF_ALU64 | BPF_MOV | BPF_K`.

### The full loop, closed: from a bpftrace one-liner down to an actual verified byte

**This is worth treating as the single best "it's turtles all the way down, and here's the bottom" moment in the whole book.** Taking the very first instruction in `hello_world.c`, `BPF_MOV64_IMM(BPF_REG_1, 0xa21)`, the appendix decodes it by hand: the opcode expands to `BPF_ALU64 | BPF_MOV | BPF_K`, which numerically comes out to **`0xb7`**. The book then **verifies this against the running kernel directly**, using `bpftool` (Chapter 2 §2.3.4) to dump the actual, loaded, JIT'd program:
```
# bpftool prog dump xlated id 907 opcodes
   0: (b7) r1 = 2593
       b7 01 00 00 21 0a 00 00
```
The printed `b7` matches the hand-computed opcode exactly; `01` is the destination register (`BPF_REG_1`); the remaining bytes encode the constant `0xa21` (2593 decimal). **This is worth remembering as the concrete, satisfying end point of a chain that started all the way back in Chapter 5**: a bpftrace one-liner compiles through an AST, then LLVM IR (Chapter 5 §5.16), then down to exactly this kind of raw instruction stream, verified here byte-for-byte against what the kernel actually loaded and ran. Everything in between — BCC's rewriter, bpftrace's map functions, the verifier, the JIT — exists specifically so that no one writing an actual tool ever has to work at this level themselves, but knowing it's there, and that you *can* check it directly with `bpftool`, is worth having as a genuine capability for the rare moment you need to debug at the very lowest level.

## More Info

Both appendixes point to the same small set of primary, canonical references worth bookmarking if you go further with raw BPF: `Documentation/networking/filter.txt` in the Linux kernel source, the `include/uapi/linux/bpf.h` and `bpf_common.h` headers (the actual, authoritative, always-current instruction and helper definitions), and Cilium's own "BPF and XDP Reference Guide" — a well-maintained, actively-updated community resource that the book itself defers to for anything beyond its own selected, teaching-oriented subset.

---

## Quick-reference glossary (this appendix pair's terms)

- **`_kern.c` / `_user.c`** — the standard file-split convention for a raw C BPF tool: kernel-side BPF program, compiled separately to an object file, and a native user-space loader/manager program.
- **`__sync_fetch_and_add()`** — the atomic increment a raw C BPF program must call explicitly to safely update a shared (non-per-CPU) map value; handled invisibly by `count()`/`.increment()` in bpftrace/BCC.
- **`struct bpf_insn`** — the fixed 8/8/8/16/32-bit (opcode/dst/src/offset/immediate) layout of one real BPF instruction.
- **`bpftool prog dump xlated ... opcodes`** — the command used to verify a program's actual, loaded instruction bytes against what you'd expect from its source.

## Check your understanding

1. The book lists "lower runtime overhead" as a reason people consider writing BPF tools in raw C, but immediately gives a strong counterpoint. What is that counterpoint, and in which specific, narrower case does writing in C actually still help?
2. In `bitehist`'s raw C source, why is `__sync_fetch_and_add()` necessary here, when Chapter 6's `count()` in bpftrace never required you to think about atomicity at all?
3. What real, concrete operational problem does `hello_world.c`'s explicit cleanup code (closing file descriptors, detaching the kprobe) exist to prevent, and on which kernels does this matter most?
4. Using Appendix E's encoding table, explain in your own words what it means for `BPF_MOV64_IMM()`'s opcode to be "`BPF_ALU64 | BPF_MOV | BPF_K`," bitwise-OR'd together.
5. Why does the book verify its hand-decoded opcode (`0xb7`) against `bpftool`'s actual output, rather than just trusting the manual calculation?

<details>
<summary>Answers</summary>

1. The counterpoint is that the same compiled BPF bytecode runs in the kernel regardless of which front end (C, BCC, or bpftrace) generated it, and any tool that summarizes results in-kernel via maps already does the overwhelming majority of its work with essentially no front-end CPU cost while running — so rewriting such a tool in C accomplishes nothing. The narrower case where C genuinely can help is a tool that prints a very high volume of individual events, to the point where the front end's own event-processing loop becomes a visible CPU cost in a tool like `top(1)` — there, a leaner C-based consumer can reduce that specific overhead.
2. Because `bitehist`'s raw C version uses an ordinary (non-per-CPU) hash map, and directly performing a non-atomic read-modify-write increment on a value that multiple CPUs might update concurrently risks exactly the kind of lost-update race Chapter 2 demonstrated with `@hash++` — two CPUs could both read the same old count and both write back the same incremented value, silently losing one increment. `count()` in bpftrace avoids ever needing this because it's implemented internally using a per-CPU map, eliminating the possibility of concurrent access to the same underlying memory in the first place; a hand-written raw-C hash map has no such protection built in, so the atomic operation has to be added explicitly.
3. It prevents a kprobe from being left enabled in the kernel with no user-space consumer reading its output, after the tool that set it up has already exited — which, on kernels before Linux 5.2 (before probes became fully file-descriptor-based and self-cleaning on process exit), would otherwise continue costing tracing overhead indefinitely until someone manually discovered and removed the orphaned probe.
4. It means the final opcode byte is constructed by taking three separate values — the instruction class `BPF_ALU64` (marking this as a 64-bit arithmetic/logic operation), the specific operation `BPF_MOV` (a move), and the operand type `BPF_K` (indicating the source is an immediate constant rather than another register) — and combining them with a bitwise OR into one single byte, where each contributing value occupies its own reserved bits within that byte, rather than needing three separate fields.
5. Because manual, by-hand decoding of a bitwise-OR'd opcode is easy to get subtly wrong, and the whole point of the exercise is to confirm what the kernel is *actually* running, not just what you intended to write — `bpftool`'s output reflects the real, loaded, verified program directly from the kernel itself, making it a genuine, independent source of truth rather than a second copy of the same manual calculation.

</details>

---

*This closes the full study-notes series for BPF Performance Tools — see `00-index.md` for the complete roadmap of everything covered, chapter by chapter.*
