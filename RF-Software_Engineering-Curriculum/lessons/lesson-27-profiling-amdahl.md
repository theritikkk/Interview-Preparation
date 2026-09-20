[← Table of Contents](../README.md) · [← Lesson 26](lesson-26-simd-avx2.md)

---

# Lesson 27 — Profiling: Measuring Where Time Actually Goes, Instead of Guessing

*Stage 9/10, continued. Lessons 22 and 25 both flagged the same warning — don't optimize before profiling — and deferred the "how." This lesson pays that debt, and it turns out the right way to think about profiler output is the exact same statistical-estimation logic used for BER curves back in Lesson 7.*

### 1. Intuition

Human intuition about which part of a program is "slow" is famously, consistently wrong. Real programs follow something close to a 90/10 rule: the overwhelming majority of runtime concentrates in a small fraction of the code, and that fraction is very often *not* where a person would guess by reading the source and judging what "looks expensive." Profiling is the discipline of measuring instead of guessing — and the specific measurement technique most profilers use (statistical sampling) deserves to be understood, not just operated.

### 2. The engineering problem that forces this

You have a real-time DSP pipeline — FIR filtering (Lesson 23), an FFT stage, decimation, an equalizer, ring-buffer handoffs between threads (Lesson 24) — that isn't meeting its deadline. Which stage is actually responsible? Guessing wrong means spending real engineering effort (SIMD-vectorizing, Lesson 26; fixed-point converting, Lesson 25) on code that was never the bottleneck, while the actual problem goes untouched. You need a principled way to find out.

### 3. Mathematical foundation

A **sampling profiler** (e.g. `perf record`) periodically interrupts the running program — say, every 1 ms — and records the current call stack at that exact instant. This is a **statistical sample**, not a direct timer on any function; the fraction of samples landing in a given function estimates the fraction of total time spent there. A **flame graph** visualizes this: each function's box width is proportional to its fraction of samples (not raw seconds directly, but proportional to them under §5's estimation logic).

### 4. Formal definition

**Self time**: time spent in a function's own code, excluding time spent in functions it calls. **Total (inclusive) time**: self time plus everything called from within it. A function can have enormous total time while doing almost no real work itself — a pure dispatcher calling one expensive routine is the clean example, and mistaking total time for self time when picking an optimization target is a genuine, common misread of profiler output.

### 5. Derivation — two things worth proving, not just accepting

**(a) Why sampling gives an unbiased time estimate.** Let $P(f)$ be the true fraction of wall-clock time spent executing function $f$. Take $N$ independent samples of "what's executing right now"; the count $k$ landing in $f$ follows approximately $\text{Binomial}(N, P(f))$ for $N$ large relative to typical stack-residency duration. The sample estimate $\hat P(f) = k/N$ is **unbiased** ($E[k/N]=P(f)$), with standard error $\sqrt{P(f)(1-P(f))/N}$ — shrinking as $1/\sqrt N$. This is the *exact same* "need enough trials for a trustworthy estimate" logic as Lesson 7 §12's BER-estimation caveat, and Lesson 10's Rayleigh-fading histogram experiment — the same statistical-estimation reasoning, now applied to "where does my program spend time" instead of "how often does my receiver make an error." A profile built from too few samples is exactly as untrustworthy as a BER estimated from too few bits, for the identical mathematical reason.

**(b) Why optimizing the wrong target has a hard ceiling — Amdahl's Law.** Suppose fraction $X$ of total runtime $T$ is sped up by factor $S$; everything else is unchanged. New total time: $T(1-X) + TX/S$. Speedup:
$$\text{Speedup} = \frac{T}{T(1-X)+TX/S} = \frac{1}{(1-X)+X/S}$$
As $S\to\infty$ (an infinitely fast version of that one part), speedup approaches $\frac{1}{1-X}$ — a hard ceiling determined **entirely** by how large a fraction of the total the optimized part was to begin with, regardless of how much engineering effort goes into it. If profiling shows a function is 3% of total runtime, even a perfect, infinite speedup on it caps overall improvement at $1/0.97\approx1.03\times$ — a 3% ceiling, full stop. This is precisely why "profile first" isn't a platitude; it's a quantifiable, provable prioritization rule.

### 6. Visualization

```
Flame graph (width ∝ fraction of samples, i.e. estimated time share):

[main______________________________________________]
[dispatch(low self time)____________________________]
  [ FFT stage 45% ][ equalizer 30% ][FIR 15%][ring buf 3%][misc 7%]
       ↑ widest box = most self time = the actual target worth optimizing
```

### 7. Worked example

Profiling a Lesson 22–26 pipeline might show: FFT stage 45% self time, equalizer 30%, FIR filter 15%, ring-buffer push/pop 3%, everything else 7%. Even a heroic $10\times$ speedup on the ring buffer (3%) yields, by §5(b): $1/(0.97+0.03/10) = 1/0.973 \approx 1.028\times$ overall — under 3% improvement, for what might be substantial engineering effort. The FFT stage, at 45%, has a genuinely large ceiling: even a modest $2\times$ local speedup there gives $1/(0.55+0.45/2)=1/0.775\approx1.29\times$ overall — a far better return on the same engineering time.

### 8. Code

```python
# Not DSP code this time -- a small, reusable Amdahl's Law calculator,
# meant to be run with YOUR actual profiled percentages before writing
# a single line of optimization code.

def amdahl_speedup(fraction_optimized: float, local_speedup: float) -> float:
    return 1.0 / ((1 - fraction_optimized) + fraction_optimized / local_speedup)

# Example: is a 4x local speedup on a function that's 15% of runtime worth it?
X, S = 0.15, 4.0
print(f"Overall speedup ceiling: {amdahl_speedup(X, S):.3f}x")

# Compare several real candidate targets from a hypothetical profile:
for name, frac in [("ring_buffer", 0.03), ("fir_filter", 0.15),
                    ("equalizer", 0.30), ("fft_stage", 0.45)]:
    print(f"{name:12s} ({frac*100:.0f}% of runtime): "
          f"even S=infinity caps overall speedup at {amdahl_speedup(frac, 1e9):.3f}x")
```

### 9. Experiment

Profile your actual Lesson 23/26 FIR pipeline (with an FFT call included, if you've built one) using `perf record -g ./your_binary` followed by `perf report` (Linux; macOS users can use `instruments` or `sample`), sorted by **self** time. Identify the top three self-time functions and their percentages. Before optimizing anything, plug the top function's percentage into §8's calculator with a realistic assumed local speedup (e.g., what Lesson 26's SIMD conversion might plausibly achieve) and compute the ceiling. Only then attempt the actual optimization, and compare your *measured* overall speedup against the *predicted* ceiling from Amdahl's Law.

### 10. Expected output

Real, machine-specific numbers — this will differ across your hardware and build, which is itself the point; there's no universal "correct" profile to expect. What should hold regardless of machine: your measured overall speedup after optimizing should not exceed the Amdahl's-Law ceiling computed from the pre-optimization profile — if it does, something about the measurement methodology (not the theory) is off.

### 11. Debugging exercise

Symptom, no cause given: you spend a significant afternoon SIMD-vectorizing (Lesson 26) a function that your own profiling data showed was only about 4% of total runtime. Locally, the function itself really is much faster. But the overall program's measured speedup is barely noticeable — nowhere near as impressive as the local improvement suggested it should be.

Using only what this lesson just derived: what should you have predicted *before* starting, and does the disappointing result actually contradict anything?

**Root cause (revealed):** this isn't a bug or a failed optimization — it's **exactly what Amdahl's Law predicts**, and a disappointing surprise only because the prediction wasn't made first. At 4% of runtime, even a perfect infinite speedup caps overall improvement at $1/0.96\approx1.04\times$ — a 4% ceiling. A large local speedup on a small-fraction function was never going to produce a large global speedup; the afternoon's effort wasn't wasted on bad code, it was spent optimizing the wrong target, which §9's "compute the ceiling before optimizing" habit exists specifically to catch in advance.

### 12. Common mistakes

- Profiling a **debug** (`-O0`) build instead of a release (`-O2`/`-O3`) build — producing a completely different, misleading hot-path picture, since unoptimized code has vastly different relative costs than optimized code. This is Lesson 22's build-flags lesson, returning as a profiling-methodology error specifically.
- Confusing self time with total time (§4) when choosing an optimization target — chasing a high-total-time dispatcher function that does almost no work itself.
- Profiling on an unrepresentative workload — a tiny test signal can make one-time setup/initialization costs look proportionally significant, while a realistic, continuous, high-rate stream would show a completely different steady-state balance. Profile under conditions that resemble real deployment, not a convenience-sized test case.

### 13. Real RF/cellular application

Profiling-driven optimization — not unguided effort — is standard, expected practice on any serious real-time PHY/baseband/modem software team. Being able to show you diagnosed a *real* performance issue with a profiler, computed an Amdahl's-Law-informed priority before investing effort, and measured the actual result afterward is a genuinely strong, differentiating signal in an interview or portfolio project — and it's exactly what Project 11 (High-Performance DSP Engine) explicitly asks you to document: profile, optimize the right target, prove the speedup with numbers.

### 14. Interview question

*"You're told a real-time DSP pipeline is missing its deadline. What's your first step?"* Answer from this lesson, not instinct: profile first, ranked by self time, to find the actual hot path — before touching any code. Then use Amdahl's Law to prioritize which target offers the largest *possible* overall improvement, rather than guessing based on which part of the code looks complex or unfamiliar.

### 15. Active recall

1. Why is a sampling profiler's time estimate for a function mathematically the same kind of estimate as a simulated BER?
2. In one sentence, what does Amdahl's Law say about optimizing a function that's a small fraction of total runtime?
3. Why can a function have high total time but low self time, and why does that distinction matter for choosing what to optimize?

### 16. Mini assignment

Profile your own Lesson 23/26 code, identify the top three self-time functions with real percentages, and use §8's calculator to compute the Amdahl's-Law speedup ceiling for optimizing the #1 function by an assumed 4×. Decide, from that number alone, whether it's worth pursuing — before writing a single line of optimization code.

### 17. Exit test

1. Derive, in one line, why the sampling estimator $\hat P(f)=k/N$ is unbiased.
2. Write Amdahl's Law from memory and compute the speedup ceiling for optimizing a function that's 20% of runtime by a factor of 5.
3. Your reasoning from §11 about the 4%-function afternoon, if you haven't sent it.

---

**Next:** This closes the Lessons 22–27 performance-engineering arc entirely — layout, real-time buffering, concurrency, fixed-point, vectorization, and now measurement, in that order. Lesson 28 returns the curriculum to **Stage 8 — 5G NR**, picking up where Lesson 19–21's LTE deep-dive left off: numerologies, the flexible NR resource grid, and SSB — reasoned as the deltas from LTE, per the roadmap's own promise.

---

[← Lesson 26](lesson-26-simd-avx2.md) · [Table of Contents](../README.md) · [Lesson 28 →](lesson-28-nr-numerology-ssb.md)