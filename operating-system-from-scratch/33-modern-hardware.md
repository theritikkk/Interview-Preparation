# 33 — Modern Hardware

## Layer map for this file

```
┌─────────────────────────────────────────────────┐
│ 1. BOOK: none — x86 used implicitly throughout  │
│    but never explained at the microarchitecture │
│    level this file operates at                  │
├─────────────────────────────────────────────────┤
│ 2. CORE CONCEPT: collecting four deferred       │
│    threads — pipelining, register renaming,     │
│    branch prediction, memory ordering           │
├─────────────────────────────────────────────────┤
│ 3. MODERN: this IS the modern layer — SIMD,     │
│    cache hierarchy, why Spectre works           │
└─────────────────────────────────────────────────┘
```

Book coverage: **None.** Four separate files deferred something here
specifically:

| File | What it deferred |
|---|---|
| `03` | Register renaming — "registers aren't really there" |
| `04` | SIMD/AVX and micro-op translation |
| `08` | Pipelining — why the far jump had to flush it |
| `23` | Memory ordering and reordering |

Everything you've learned since file `03` describes the CPU's
**architectural** behavior — what it promises to make *look like*
happened. This file is what actually happens underneath, which can be
wildly different, as long as the illusion holds perfectly from the
outside.

## Prerequisites

Files `01`–`32`, especially `03`, `05`, `08`, `23`, `24`, `31`.

---

## [CORE CONCEPT] Pipelining, in Full

![Five-stage pipeline diagram: five instructions overlapping across Fetch, Decode, Execute, Memory, and Write-back stages, a new instruction entering Fetch every cycle, with a note that a jump changes what the next instruction even is after fetch has already begun, forcing everything mid-pipe to be discarded](images/33a-pipeline.svg)

File `08` asserted that a jump forces a pipeline flush without fully
explaining why. Here's the complete picture: a CPU splits instruction
execution into stages (fetch, decode, execute, memory access, write
back) and runs **multiple instructions through different stages
simultaneously** — while instruction 3 executes, instruction 4 decodes,
and instruction 5 is already being fetched. This is *why* one
instruction finishing "instantly" is a fiction — throughput comes from
overlap, not from any single instruction being fast. A jump is exactly
the case where the pipeline's guess about "what's next" turns out
wrong: instructions already fetched and decoded after the jump have to
be thrown away entirely, which is a real, measurable cost called a
**pipeline flush** — precisely file `08`'s far-jump requirement, now
fully explained.

## [CORE CONCEPT] Out-of-Order Execution and Register Renaming

Modern CPUs go further than pipelining alone: they execute instructions
**out of program order** internally, running whichever ones have their
inputs ready first, then **retire** results in the original order so
the outside world never sees anything but the architecturally correct
sequence. This creates a problem file `03` flagged without solving:
if two unrelated instructions both happen to use `eax`, running them
out of order risks one clobbering the other's value before it's read.
**Register renaming** is the fix — the CPU maintains far more physical
registers than the ISA exposes, and transparently maps each *use* of
`eax` to a different physical register when there's no real dependency,
letting both run in parallel safely. File `03`'s claim that "`rax`
doesn't correspond to one fixed physical storage cell" is this
mechanism, named properly.

## [CORE CONCEPT] Branch Prediction and Speculative Execution

![Branch prediction: a conditional jump like cmp/je gets predicted before the comparison result is even known; if the guess was correct, speculative work simply becomes real; if wrong, results are squashed at a real performance cost; but cache state from the wrong-path work isn't perfectly erased, which is exactly the gap Spectre exploits](images/33b-branch-prediction.svg)

Pipelining creates an awkward problem for file `05`'s `cmp`/`je`: the
CPU is already fetching instructions *after* a conditional jump before
it knows which way that jump will actually go. **Branch prediction**
guesses, based on the branch's own history, and the CPU **speculatively
executes** down the guessed path. Guess right (the common case, often
90%+ of the time for predictable branches) and the speculative work
simply becomes real output, with no visible sign anything unusual
happened. Guess wrong and everything speculative gets discarded — a
**misprediction penalty**, often costing more cycles than the pipeline
flush above.

Here's the connection file `31` set up and this file resolves:
architecturally, a squashed wrong guess is *supposed* to leave no
trace. In practice, it still touched the **cache** (file `24`) along
the way, and that touch is measurable through timing — the exact
mechanism behind Meltdown and Spectre, now with its actual name and
shape: a real, useful performance optimization with an observable side
channel nobody fully closes without giving up the optimization itself.

## [CORE CONCEPT] Memory Ordering, Resolved

File `23` warned that CPUs and compilers can reorder memory operations
for speed, and that `volatile` doesn't prevent it. The mechanism is the
same one this file has been describing throughout: out-of-order
execution applies to memory reads and writes too, constrained only by
whatever ordering guarantees the ISA actually promises. x86's guarantees
are relatively strong (stores aren't reordered with other stores; loads
aren't reordered with other loads); ARM's are deliberately weaker, which
is exactly why file `24`'s cross-architecture warning about racy code
"working on x86, failing on ARM" is a real, specific consequence of this
file's mechanism — not a vague caveat.

---

## [RELATED KNOWLEDGE] Why Caches Work At All: Locality

File `24` covered cache *coherence* (keeping copies consistent) without
covering why caching helps in the first place: **temporal locality**
(recently accessed data is likely to be accessed again soon) and
**spatial locality** (data near recently accessed data is likely to be
accessed soon too) are empirical properties of almost all real programs.
A 64-byte cache line (file `24`'s false-sharing unit) exists specifically
to exploit spatial locality — fetching one byte's neighborhood along
with it, betting correctly, most of the time, that the neighbors will be
needed next.

---

## [MODERN SYSTEM] SIMD

File `04` deferred this: **SIMD** (Single Instruction, Multiple Data)
instructions — SSE, AVX, AVX-512 — operate on several data values in
one instruction (adding eight pairs of 32-bit numbers in a single
instruction, say), rather than one value at a time. Compilers can
auto-vectorize simple loops to use these automatically; workloads like
multimedia processing, scientific computing, and machine learning
inference lean on them heavily and deliberately.

---

## Practical Experiment

1. `lscpu` — find cache sizes (L1d, L1i, L2, L3) and note how they grow
   and slow down at each level, matching file `32`'s latency table.
2. Write a loop summing a large array two ways: sequentially (good
   spatial locality) versus with a large, fixed stride (poor locality);
   time both — the difference is locality, made concrete.
3. `perf stat -e branch-misses,branches ./prog` on a program with a
   predictable branch (a sorted array's comparisons) versus an
   unpredictable one (random data) — see the misprediction rate differ
   directly.
4. Compile a simple numeric loop with `-O3 -march=native` versus `-O0`
   and check the assembly (`objdump -d`) for AVX/SSE instructions
   appearing in the optimized version — real, automatic
   vectorization, visible in the output.

## Debugging

A performance regression that appears only on certain input patterns
(sorted vs. random, sequential vs. scattered access) is often exactly
this file's mechanisms showing through: branch misprediction rate
changing with data pattern, or cache locality degrading with access
pattern. `perf stat`'s hardware counters (branch misses, cache misses,
file `32`) distinguish these directly instead of guessing from
symptoms alone.

## Source Code

The locality benchmark and the branch-predictability benchmark above.

---

## Common Misconceptions

- **"Instructions execute one at a time, each fully finishing before
  the next starts."** The architectural illusion, not the reality —
  pipelining and out-of-order execution mean many instructions are
  in flight simultaneously.
- **"`volatile` prevents memory reordering."** It only stops compiler
  register-caching; hardware reordering (this file) is a separate
  concern entirely, requiring real memory barriers.
- **"Branch misprediction is rare enough not to matter."** For
  unpredictable branches (data-dependent, non-repeating patterns), it's
  a real, frequently significant cost.

## Interview Questions

**Basic:** What is pipelining, in one sentence?
**Intermediate:** Why does a mispredicted branch cost real time, even
though the CPU never executes anything architecturally wrong?
**Advanced:** How does register renaming let out-of-order execution
avoid false dependencies between unrelated uses of the same register name?
**Systems Design:** Why might the same racy C code work on x86 but fail
on ARM?
**Debugging:** A loop's performance degrades sharply on random input but
not sorted input — what hardware mechanism is the likely cause?
**Explain what happens when...:** a CPU speculatively executes down a
mispredicted branch and then discovers the misprediction — trace what's
discarded and what, subtly, isn't?

## Active Recall

1. Which four earlier files deferred content to this one, and what did
   each defer?
2. What problem does register renaming solve?
3. What's the connection between branch prediction and file `31`'s
   Meltdown/Spectre content?
4. What are temporal and spatial locality?

## Answers

1. File `03`: register renaming. File `04`: SIMD and micro-op
   translation. File `08`: pipelining. File `23`: memory ordering.
2. False dependencies between unrelated instructions that happen to
   name the same architectural register, which would otherwise block
   safe out-of-order execution.
3. Speculative execution down a mispredicted branch touches the cache
   even though its architectural results are discarded — that leftover
   cache state is measurable through timing, the exact side channel
   Meltdown and Spectre exploit.
4. Temporal locality: recently accessed data is likely accessed again
   soon. Spatial locality: data near recently accessed data is likely
   to be accessed soon too — the basis for cache-line prefetching.

---

## What You Should Now Understand

- Four deferred threads (files `03`, `04`, `08`, `23`), now fully
  resolved in one place.
- Pipelining, out-of-order execution, and register renaming as one
  connected story about maintaining an illusion of simple, sequential
  execution.
- Branch prediction's real mechanism, and exactly how it connects to
  file `31`'s security content.
- Why caches work at all — locality, not magic.

---

**Next:** file `34-modern-boot-and-efi.md`.
