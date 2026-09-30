# 32 — Performance and Observability

## Layer map for this file

```
┌─────────────────────────────────────────────────┐
│ 1. BOOK: none — but a sampling profiler is      │
│    file 14's interrupt and file 20's timer      │
│    tick, repurposed for a new question          │
├─────────────────────────────────────────────────┤
│ 2. CORE CONCEPT: where time actually goes, the  │
│    USE method, sampling vs. instrumentation     │
├─────────────────────────────────────────────────┤
│ 3. MODERN: perf, flame graphs, latency numbers  │
└─────────────────────────────────────────────────┘
```

Book coverage: **None.** The mechanism behind this file's central tool
is one you already own: a sampling profiler is a timer interrupt (file
`14`) firing at a fixed rate, doing something other than a scheduler
tick (file `20`) with it.

## Prerequisites

Files `01`–`31`, especially `14`, `20`, `26`, `27`.

---

## [CORE CONCEPT] Where Time Actually Goes

File `01`'s abstraction stack, asked a performance question: for any
one unit of work, time is spent **on CPU** (actually executing), or
**off CPU** — waiting for a lock (file `23`), waiting for I/O (files
`17`, `30`), or waiting to be scheduled (file `20`, sitting Ready while
something else runs). "Slow" always means time landed in one of these
buckets more than expected — the entire job of performance analysis is
figuring out *which one*.

## [CORE CONCEPT] The USE Method

A systematic starting question for any resource (CPU, memory, disk,
network) beats guessing: **Utilization** (how busy is it?),
**Saturation** (how much work is queued, waiting for it?), **Errors**
(is it failing, and retrying?). A CPU at 60% utilization with a long run
queue (file `20`) is saturated despite looking underused by
utilization alone — the two numbers answer different questions and
both matter.

## [CORE CONCEPT] Sampling, Not Instrumenting

![A sampling profiler: a fast timer interrupt fires, its handler captures the current call stack, execution resumes and this repeats thousands of times per second, and the samples are aggregated into a histogram rendered as a flame graph, where the widest box at any level shows where samples landed most](images/32-sampling-profiler.svg)

This is the file's central idea: a profiler like `perf` doesn't modify
your code or slow down every function call to measure it.
It sets up a timer interrupt at a high, fixed frequency — the same
mechanism as file `14`'s ISR and file `20`'s scheduler tick, aimed at a
different job — and each time it fires, its handler simply records
**which function, and its entire call chain, was executing at that
exact instant.** Do this a few thousand times a second, aggregate the
results, and the function that shows up in the most samples is
provably where the most time was actually spent — without ever slowing
the program down enough to change its own behavior.

A **flame graph** is the standard way to look at the result: each box
is a function, its width proportional to how often it appeared in a
sample, stacked vertically by call depth. The widest box at any level
is the hot path — not a guess, a measurement.

---

## [RELATED KNOWLEDGE] Latency Numbers Worth Having Memorized

Concrete numbers for the layers this course already built, roughly:

| Operation | Approximate latency |
|---|---|
| L1 cache reference (file `24`) | ~1 nanosecond |
| L2/L3 cache reference | ~5–40 nanoseconds |
| Main memory reference (file `22`) | ~100 nanoseconds |
| SSD random read (file `17`) | ~100 microseconds |
| Spinning-disk seek (file `17`) | ~10 milliseconds |
| Same-datacenter network round trip (file `30`) | ~0.5 milliseconds |
| Cross-continent network round trip | ~100+ milliseconds |

Each row spans roughly three orders of magnitude over the one above
it. This is *why* file `22`'s page cache, file `24`'s CPU caches, and
file `18`'s filesystem buffering all exist — every one of them is
purely an attempt to keep access patterns as high in this table as
possible.

---

## [MODERN SYSTEM] perf

`perf` is Linux's own sampling profiler, using hardware performance
counters (real, on-chip counters for cache misses, branch
mispredictions, and instructions retired — file `33` territory) as
well as software events. `perf top` gives a live, continuously updating
view of exactly this file's histogram; `perf record` / `perf report`
capture and later browse one; `perf stat` gives simple aggregate
counts without full sampling overhead.

## [MODERN SYSTEM] Beyond CPU: Off-CPU Analysis

A pure CPU profiler is blind to time spent **waiting** — a thread
blocked on I/O or a lock never appears in an on-CPU sample at all,
because it isn't running. **Off-CPU flame graphs** (built from
scheduler tracepoints, tracking when a thread leaves and re-enters
Running, file `19`'s state diagram) answer the other half of "where did
time go" — essential for anything I/O- or lock-bound rather than
CPU-bound. `bpftrace` and eBPF-based tools (file `35`) are the modern
way to build these without a custom kernel module.

---

## Practical Experiment

1. `perf stat ls` — aggregate counts (instructions, cache misses,
   branch mispredictions) for a trivial command.
2. Write a small CPU-bound program (a tight numeric loop) and a
   small I/O-bound one (repeatedly reading a file); `perf record -g
   ./prog` then `perf report` on each — see the difference in what
   actually shows up in the profile.
3. Generate a real flame graph (Brendan Gregg's `FlameGraph` scripts,
   or `perf script | stackcollapse-perf.pl | flamegraph.pl`) for the
   CPU-bound program above; open the resulting SVG and click into a
   few boxes.
4. Time a small file read from a warm page cache (file `22`) versus a
   freshly-dropped cache (`echo 3 | sudo tee
   /proc/sys/vm/drop_caches`, then read again) — put real numbers next
   to this file's latency table.

## Debugging

Given a "the service is slow" report with no other information, the
disciplined order is: **USE method first** (which resource is actually
saturated?), then **on-CPU profile** if it's CPU-bound, or **off-CPU
analysis** if it's mostly waiting, then only after that, read source
code. Reading code first, before measuring anything, is the single most
common way performance investigations waste time on the wrong function
entirely.

## Source Code

Your CPU-bound and I/O-bound test programs, plus the flame graph SVGs
generated from each.

---

## Common Misconceptions

- **"A profiler slows a program down too much to trust the results."**
  Sampling profilers cost very little precisely because they don't
  instrument every call — they just look occasionally, the same way an
  exit poll doesn't require asking every voter.
- **"High CPU usage always means the CPU is the bottleneck."** The USE
  method's point exactly — saturation (queueing) matters as much as
  utilization, and a process can be slow while CPU usage looks low, if
  it's mostly waiting.
- **"Reading the source is the fastest way to find a performance bug."**
  Usually slower than measuring first — intuition about "the slow part"
  is wrong often enough that profiling beats guessing.

## Interview Questions

**Basic:** What does a sampling profiler actually record, each time it fires?
**Intermediate:** Why doesn't a sampling profiler significantly slow
down the program it's measuring?
**Advanced:** Why can a CPU-only profiler completely miss the real cause
of a slow, I/O-bound request?
**Systems Design:** Explain the USE method and why utilization alone
can be a misleading number.
**Debugging:** You're told "the app is slow" with no other detail — what
do you measure first, and why?
**Explain what happens when...:** `perf record -g` profiles a running
program — trace it from the timer interrupt to the flame graph you
eventually view?

## Active Recall

1. What mechanism, from an earlier file, does a sampling profiler
   actually use?
2. What are the three questions in the USE method?
3. Why can utilization alone be misleading?
4. Roughly how many orders of magnitude separate L1 cache access from
   an SSD random read?

## Answers

1. A timer interrupt (file `14`), at high frequency — the same
   mechanism behind file `20`'s scheduler tick, repurposed.
2. Utilization, Saturation, Errors.
3. A resource can be moderately utilized but heavily saturated (a long
   queue of waiting work), which utilization alone doesn't reveal.
4. Roughly five orders of magnitude (nanoseconds to ~100 microseconds).

---

## What You Should Now Understand

- That a sampling profiler is file `14`'s interrupt and file `20`'s
  timer tick, aimed at a new question.
- The USE method as a systematic starting point instead of guessing.
- Flame graphs as a direct visualization of sampled call stacks, width
  meaning time.
- Concrete latency numbers explaining why every cache this course built
  (file `22`, `24`) exists at all.

---

**Next:** file `33-modern-hardware.md`.
