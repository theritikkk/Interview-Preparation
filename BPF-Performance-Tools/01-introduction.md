# Chapter 1 — Introduction

*Study notes on Chapter 1 of BPF Performance Tools (Brendan Gregg, 2019). Original notes, not book text — see `00-index.md` for how these notes are written.*

## What this chapter is actually for

This chapter doesn't teach you BPF — it teaches you the **vocabulary and the shape of the landscape** so that Chapter 2 (the deep technical dive) has somewhere to land. If you only read one chapter of this book casually before diving into tools, make it this one; everything here gets a full, precise treatment later.

## 1.1 What are BPF and eBPF?

**BPF** stands for **B**erkeley **P**acket **F**ilter — a name that is now almost entirely misleading, which is worth internalizing immediately so later terminology doesn't confuse you. It started in 1992 as a narrow, packet-filtering technology. In 2013, Alexei Starovoitov proposed a near-total rewrite (with Daniel Borkmann joining the effort), which merged into the Linux kernel in 2014. That rewrite turned BPF into something categorically different: a **general-purpose, sandboxed execution engine inside the kernel** — not just a filter for network packets.

The mental model worth keeping: **BPF lets you run small, safety-checked programs in response to kernel and application events.** A helpful analogy from the book: it's similar to how JavaScript lets a web page run small programs in response to browser events like clicks. BPF does the same thing for the *operating system* — disk I/O, function calls, scheduling decisions — and, unlike a browser sandbox, it's compiled and run directly by the kernel itself.

Three things make this safe and fast enough to use in production:
- **A verifier** checks every BPF program before it's allowed to load, rejecting anything that could crash or corrupt the kernel (it does *not* stop you from writing something that runs but doesn't make logical sense).
- **An interpreter and a JIT (just-in-time) compiler** actually execute the verified instructions — the JIT turns BPF bytecode into native machine instructions, so a loaded BPF program runs about as fast as any other kernel code.
- **Helper functions and BPF "maps"** give the sandboxed program controlled ways to read kernel state and to store data, since it can't just call arbitrary kernel functions or use unrestricted memory.

BPF today has three broad uses: **networking**, **observability**, and **security**. This book is entirely about the second: **observability**, specifically via **tracing**.

One terminology note that will save you confusion for the rest of the book: the "extended" rewrite is often nicknamed **eBPF**, but the kernel and the development community both just call it **BPF** — there's only one execution engine in the kernel, and it runs both old-style ("classic") and new-style ("extended") programs. This book uses "BPF" throughout to mean the modern, extended version.

## 1.2 Tracing, snooping, sampling, profiling, and observability — precise definitions

These words get used almost interchangeably in casual conversation, but the book is precise about them, and the distinctions actually matter for choosing the right tool later:

| Term | Definition | Example |
|---|---|---|
| **Tracing** | Event-based recording — capturing *every* occurrence of an event, with metadata, as it happens | `strace(1)` recording every syscall |
| **Snooping** | Historically just another word for tracing/event-dumping (a naming quirk — Solaris's version of `tcpdump` was literally called `snoop(1M)`, and the author's own early tools inherited the naming habit) | `execsnoop`, `opensnoop`, `biosnoop` |
| **Sampling / Profiling** | Taking a coarse *subset* of measurements at intervals to build an approximate picture, rather than recording every event | `profile(8)` taking a stack sample every 10ms |
| **Observability** | The umbrella term for *any* technique that lets you understand a system by watching it — includes tracing, sampling, and fixed-counter tools, but explicitly **excludes benchmarking** (which perturbs the system by running a synthetic workload) | All the tools in this book |

Why this distinction matters practically: a **tracer** gives you raw, complete, potentially voluminous per-event data — you can post-process it into anything, but it costs more to collect. A **sampler** trades completeness for lower overhead — it can miss short-lived events entirely, but for understanding "where is time generally going," a statistically representative sample is often all you need. **BPF's real superpower is that it lets a tracer do its own on-the-fly summarization inside the kernel**, which is the theme the rest of the book builds on (see Chapter 2, §2.3.1, for the concrete before/after example).

## 1.3 BCC, bpftrace, and IO Visor

Writing raw BPF instructions by hand is described as "extremely tedious" — nobody actually does this for observability work. Two higher-level front ends exist, and understanding when to reach for each is a genuinely practical skill:

- **BCC (BPF Compiler Collection)** — the older, more capable, more *verbose* front end. You write the kernel-side logic in C, and the user-space control/output logic in Python (or Lua or C++). It's also the origin of the `libbcc`/`libbpf` libraries that underlie the whole ecosystem. BCC ships 70+ ready-to-run tools.
- **bpftrace** — a newer, much more concise, purpose-built high-level language, similar in spirit to `awk` for the kernel. Source for a bpftrace tool is short enough that this book prints it inline, right next to the tool's explanation.
- **IO Visor** — not a tool, but the umbrella Linux Foundation GitHub organization (`github.com/iovisor`) that hosts both BCC and bpftrace as separate open-source projects (they don't live in the kernel source tree itself).

The practical rule of thumb the book gives, which is worth memorizing: **bpftrace is ideal for powerful one-liners and short custom scripts; BCC is better suited to complex, option-rich tools and long-running daemons**, partly because Python gives it access to libraries like `argparse` for building a real command-line interface. A natural workflow: prototype an idea as a bpftrace one-liner, and only "graduate" it to a full BCC tool if you need persistent daemon behavior or a rich set of command-line flags.

(There's also a third, lighter-weight front end called `ply`, aimed at embedded Linux — mentioned for completeness but not covered further, since the book focuses on bpftrace.)

## 1.4 A first look at BCC: quick wins

Two tools are used to show that BPF tracing has immediate, practical value even before you understand how any of it works internally:

**`execsnoop`** prints one line for every new process created (it traces the `execve(2)` syscall). This matters because *short-lived processes are invisible to most monitoring tools* that only take periodic snapshots (like `ps` or `top`) — if a process starts and exits between two snapshots, you never see it. A real production anecdote from the book: `execsnoop` was used to discover that a supposedly "idle" benchmarking server was actually launching a batch of ~30 short-lived processes every second, from a misconfigured service that kept failing and retrying — invisible to snapshot-based tools, but immediately obvious once you could see every process launch as it happened. Once the runaway service was fixed, the benchmark numbers stabilized. This is a nice concrete illustration of a performance-analysis technique called **workload characterization** (formally introduced in Chapter 3): often the fix isn't "make the slow thing faster," it's "stop doing the unnecessary work in the first place."

**`biolatency`** summarizes disk I/O *latency as a histogram*, computed entirely inside the kernel while it runs, and only prints the finished summary when you stop it. Reading a latency histogram is itself a skill: look for **modes** (clusters — e.g. a fast mode near 0–1ms that's probably cache hits, and a slower mode that suggests queuing) and **outliers** (a handful of very slow requests sitting far past the rest of the distribution — often the actual thing worth chasing, since a database with a strict latency SLA is only as good as its worst outliers, not its average).

## 1.5 BPF tracing visibility: seeing the whole stack at once

The book's framing here is that BPF tracing gives you visibility "like X-ray vision" because, unlike most tools which are scoped to one layer of the software stack, BPF can instrument **all of them, live, in production, with no reboot and no special restart mode**:

| Layer | Traditional tool | Can BPF tracing see it? |
|---|---|---|
| Applications with a language runtime (Java, Node.js, Ruby, PHP) | Runtime-specific debuggers | Yes, with runtime cooperation |
| Applications using compiled code (C, C++, Go) | System debuggers | Yes |
| System libraries (`/lib/*`) | `ltrace(1)` | Yes |
| Syscall interface | `strace(1)`, `perf(1)` | Yes |
| Kernel (scheduler, file systems, TCP/IP, etc.) | Ftrace, `perf(1)` sampling | Yes, in more detail |
| Hardware (CPU internals, devices) | `perf`, `sar`, `/proc` counters | Yes, directly or indirectly |

The practical value of this table: it's a **blind-spot finder**. If you list the components of your own stack against this table, anywhere you don't currently have a tool is a place BPF tracing can likely fill in.

## 1.6 Dynamic instrumentation: kprobes and uprobes

**Dynamic instrumentation** (a.k.a. dynamic tracing) means inserting instrumentation into *already-running, unmodified* software, live — with **zero overhead when the instrumentation isn't active**, because the software itself isn't touched until you actually turn tracing on. This is what lets you instrument any of the tens of thousands of functions in a real running kernel or application on demand.

A short, important history: the technique dates to 1990s debugger breakpoint tooling. Linux's own kernel-function version, **kprobes**, was merged in 2004 but stayed obscure and hard to use. The real inflection point for the *industry* was **2005**, when Sun Microsystems shipped **DTrace** in Solaris 10 — safe, easy to use (its own "D" language), and, crucially, *on by default in a production operating system*. That combination proved dynamic tracing could be trusted in production, and normalized the whole category of tool. Linux got the user-space equivalent, **uprobes**, in 2012.

The two flavors you'll see constantly in probe names for the rest of the book:

| Probe | Meaning |
|---|---|
| `kprobe:vfs_read` | Instrument the *start* of the kernel's `vfs_read()` function |
| `kretprobe:vfs_read` | Instrument the *return(s)* of `vfs_read()` (a function can return from multiple places — all are covered) |
| `uprobe:/bin/bash:readline` | Instrument the start of `readline()` inside the `/bin/bash` binary |
| `uretprobe:/bin/bash:readline` | Instrument the return of that same function |

## 1.7 Static instrumentation: tracepoints and USDT

Dynamic instrumentation has a real downside: it targets *functions*, and functions can be renamed, removed, or **inlined away by the compiler** between software versions — this is called an **interface stability** problem. Upgrade the kernel or the app, and your carefully-built tool might silently stop working (or start throwing "function not found" errors).

**Static instrumentation** fixes this by having the *developers themselves* embed stable, deliberately-named instrumentation points directly into the source. The kernel-side version is called **tracepoints**; the user-space version is called **USDT** (User-level Statically Defined Tracing). The tradeoff runs the other way from dynamic instrumentation: static instrumentation points are stable, but they're a maintenance burden for developers, so there are far fewer of them than there are instrumentable functions.

The book's recommended strategy, worth adopting as a habit: **when developing your own tool, try static instrumentation (tracepoints/USDT) first, and only fall back to dynamic instrumentation (kprobes/uprobes) when nothing static is available.**

| Probe | Meaning |
|---|---|
| `tracepoint:syscalls:sys_enter_open` | Instrument the `open(2)` syscall via its tracepoint |
| `usdt:/usr/sbin/mysqld:mysql:query__start` | Instrument the `query__start` USDT probe inside `mysqld` |

## 1.8 A first look at bpftrace: tracing `open()`

This section is a genuinely useful worked example of *how tool development actually happens in practice* — including hitting a real gotcha and working around it live. Walking through it:

**Step 1 — the naive one-liner:**
```
bpftrace -e 'tracepoint:syscalls:sys_enter_open { printf("%s %s\n", comm, str(args->filename)); }'
```
This attaches to the `open(2)` tracepoint system-wide and prints the calling process's name (`comm`) and the filename it's opening, for *every* process on the system — you don't need to know the bpftrace language yet to read this output; you're meant to focus on what it reveals.

**Step 2 — noticing something's off:** the output arrives more slowly than expected. This is the actual investigative instinct worth learning: *don't assume your tool is working correctly just because it produces plausible-looking output.*

**Step 3 — listing available probes to find the gap:**
```
bpftrace -l 'tracepoint:syscalls:sys_enter_open*'
```
This reveals there are actually **three** related tracepoints: `sys_enter_open`, `sys_enter_openat`, and `sys_enter_open_by_handle_at`. The original one-liner only covered one of three variants.

**Step 4 — confirming which variant actually matters**, using a count-based one-liner instead of a per-event one:
```
bpftrace -e 'tracepoint:syscalls:sys_enter_open* { @[probe] = count(); }'
```
This produced counts like 5 calls to plain `open` versus 308 calls to `openat` — confirming that **modern software overwhelmingly uses the `openat(2)` syscall variant**, not the original `open(2)`. This is a genuinely useful, durable fact about modern Linux systems, and a nice demonstration of how a tracer can be used to *validate an assumption* rather than just observe an event.

This whole sequence is the reason bpftrace ships a ready-made `opensnoop.bt` script that already handles both variants and formats the output as clean columns (PID, COMM, FD, ERR, PATH) — you don't need to rediscover this gotcha yourself every time.

## 1.9 Back to BCC: tracing `open()`

BCC's version of `opensnoop` produces visually similar output to the bpftrace version, but demonstrates the practical difference in philosophy between the two front ends: **it accepts command-line options** (`-x` for failed opens only, `-p` to filter by PID, `-d` for a fixed duration, `-n` to filter by process name substring, and more). This is the concrete version of the "BCC tools are more versatile, bpftrace tools are simpler" guidance from §1.3 — a bpftrace tool *could* be edited to add one specific behavior, but a BCC tool is more likely to already expose that behavior as a flag, because Python's `argparse` makes rich CLIs cheap to build.

The natural workflow this section reinforces: reach for a bpftrace one-liner or short script first when exploring; if you find yourself wanting several different modes of the same basic idea, that's the signal a BCC-style tool (or converting your bpftrace prototype into one) is the better investment.

## 1.10 Summary

BPF tracing tools solve performance and troubleshooting problems, and come from two complementary front-end projects: **BCC** (rich, Python-driven, option-heavy) and **bpftrace** (concise, one-liner-friendly). Both rely on the same two underlying instrumentation strategies: **dynamic** (kprobes/uprobes — trace any function, but unstable across versions) and **static** (tracepoints/USDT — stable, but limited in number). Chapter 2 explains every one of these mechanisms in much greater depth; Chapter 3 turns to methodology — *how* to actually approach a performance problem before reaching for any specific tool.

---

## Quick-reference glossary (this chapter's terms)

- **Verifier** — the kernel component that statically checks a BPF program's safety before allowing it to load.
- **JIT (just-in-time) compiler** — turns BPF bytecode into native machine instructions for execution speed.
- **Tracer** — records complete, per-event data (as opposed to sampling a subset).
- **Sampler / profiler** — records a representative subset of events at intervals.
- **kprobes / kretprobes** — dynamic instrumentation of kernel function entry / return.
- **uprobes / uretprobes** — dynamic instrumentation of user-space function entry / return.
- **Tracepoints** — stable, developer-maintained static instrumentation points in the kernel.
- **USDT** — stable, developer-maintained static instrumentation points in user-space applications.
- **IO Visor** — the Linux Foundation project umbrella hosting BCC and bpftrace on GitHub.

## Check your understanding

1. Why does the book insist "BPF" and not "eBPF" is the correct term for what runs in the kernel today?
2. What's the practical difference between a *tracer* and a *sampler*, and why would you ever prefer the one that gives you less data?
3. Why did `execsnoop` succeed at finding a problem that snapshot-based tools like `top` could never have found, even run continuously?
4. Give one concrete reason a kprobe-based tool might silently break after a kernel upgrade, and explain why a tracepoint-based tool is less likely to.
5. In the `open()`/`openat()` investigation, what was the actual clue that something was being missed, and what command confirmed the fix?

<details>
<summary>Answers</summary>

1. Because there is only one execution engine in the kernel (the "extended" BPF from 2014 onward) — "eBPF" is just a popular nickname for it, not a separate technology, and kernel developers themselves use "BPF" for it in mailing-list discussion.
2. A tracer records every occurrence of an event with full metadata (complete but potentially high-volume/high-overhead); a sampler only takes periodic snapshots (lower overhead, but can miss short-lived events entirely). You'd prefer a sampler when you want a representative statistical picture (e.g., "where does CPU time generally go") rather than a complete record, since it's cheaper to run continuously.
3. Because the offending processes were extremely short-lived (launched and exited within the same second, repeatedly) — a periodic-snapshot tool like `top` can only ever see whatever's running at the instant it happens to sample, so processes that live and die between two samples are invisible to it no matter how often you run it. `execsnoop` instruments the `execve()` event itself, so it can't miss a process no matter how short-lived.
4. A kprobe targets a specific kernel function by name; if a kernel upgrade renames, removes, or inlines that function (a compiler optimization), the kprobe has nothing left to attach to and the tool breaks or silently produces no output. A tracepoint is a stable, deliberately-maintained instrumentation point that kernel developers try to keep working across versions — it's explicitly designed as an interface contract, not an implementation detail.
5. The clue was that the tool's output arrived more slowly than expected, given how frequently applications open files. Running `bpftrace -l` with a wildcard revealed there were multiple related tracepoints (`sys_enter_open` and `sys_enter_openat` among them), and a `count()`-based one-liner confirmed `openat` was the far more frequently used variant that the original one-liner had missed entirely.

</details>
