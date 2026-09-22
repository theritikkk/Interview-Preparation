[← Table of Contents](../README.md) · [← Lesson 35](lesson-35-hil-testing-test-vectors.md)

---

# Lesson 36 — Latency and Throughput: Measuring Real-Time Correctness Instead of Assuming It

*Stage 12, continued. Lessons 23–27 repeatedly claimed real-time correctness. This lesson is where those claims finally have to become numbers — and it turns out the single most common way to get those numbers wrong is reporting the one statistic that looks reassuring while hiding exactly what determines whether you meet your deadline.*

### 1. Intuition

Lessons 23–27 built real-time-flavored C++ and said things like "this is real-time safe." A claim isn't a measurement. This lesson is about actually characterizing **latency** (how long does one unit of work take) and **throughput** (how much data can the pipeline sustain continuously) with real numbers — and specifically about why the natural first instinct, reporting *average* latency, is close to useless for a hard real-time system, even though it's exactly the metric general backend/software engineering intuition reaches for first.

### 2. The engineering problem that forces this

A real-time DSP pipeline processing a continuous stream has a hard per-block budget — miss it occasionally and samples drop or buffers overflow (Lesson 23's ring buffer, Lesson 24's SPSC queue), a real, often user-visible failure, unlike a typical backend service where an occasional slow request is tolerable. You need to characterize the **worst-case** (or a high percentile) latency under **realistic** load and OS scheduling noise, and measure throughput as **sustained** capacity — not a best-case single-call number that overstates what the system can actually maintain continuously.

### 3. Mathematical foundation

Latency measurements form a **distribution**, not a single number — typically heavily right-skewed, with a long tail from occasional OS scheduling jitter, cache misses, or page faults. The relevant statistics are **percentiles**: p50 (median), p99, p99.9, and the true observed maximum. **Deadline-miss rate** — the fraction of intervals exceeding budget — is structurally a **binomial proportion**, exactly like BER (Lesson 7), a profiler's self-time fraction (Lesson 27), or a regression test's pass rate (Lesson 35): the same estimation and statistical-comparison machinery applies without modification, a fourth appearance of the identical underlying idea.

### 4. Formal definition

**Latency**: time from a block's arrival to its corresponding output being ready. **Throughput**: sustained samples (or blocks) processed per second, measured over a long window — not $1/(\text{best-case single-call latency})$, which ignores tail events and per-call overhead entirely. **Deadline-miss rate**: fraction of processing intervals whose latency exceeds the allotted budget, estimated and compared using exactly Lesson 35 §3's statistical machinery, since it's the same kind of quantity.

### 5. Derivation — why the mean actively hides the thing that matters

**(a) A concrete numeric demonstration.** Suppose a processing function takes 100 µs on 999 out of every 1000 calls, and 5000 µs (one cache miss / OS preemption event) on the 1000th. Mean latency: $(999\times100 + 5000)/1000 \approx 104.9\,\mu s$ — comfortably under almost any reasonable deadline, looking completely fine. But $p_{99.9} = 5000\,\mu s$: if your actual deadline is anywhere under 5 ms, this system **will** miss it, at a rate the mean number gives you zero visibility into. This isn't a contrived edge case — it's the *normal* shape of real latency distributions, and it's precisely why reporting only the mean is not a simplification but an active source of false confidence.

**(b) Deadline-miss rate gets the exact treatment Lesson 35 already built.** If you measure a deadline-miss rate of, say, 0.001 over 1,000,000 trials and want to know whether a code change made it statistically significantly worse, you apply Lesson 35 §3's $z$-score comparison **unchanged** — deadline-miss rate is a binomial proportion in exactly the same sense BER is. This isn't a coincidence requiring a new derivation; it's the same statistical object appearing under a fourth name, and recognizing that is the actual skill, not memorizing a new formula for each context it shows up in.

### 6. Visualization

```
Latency distribution (typical shape — long right tail):

count
  │ ███
  │ ███
  │ ███▓
  │ ███▓▓
  │ ███▓▓▓▒▒░ ░  ░       ░                    ░  ← rare, real tail events
  └────┬───┬────┬──────────────────────────────┬──→ latency
      p50 mean  p99                            max/p99.9
       ↑    ↑
   mean sits deceptively close to p50 — nowhere near where deadline risk actually lives
```

### 7. Worked example

Exactly §5(a)'s numbers: 999 calls at 100 µs, 1 call at 5000 µs, mean ≈ 104.9 µs, $p_{99.9}=5000\,\mu s$ — a 48× gap between what the mean reports and what actually threatens your deadline.

### 8. Code

```cpp
#include <vector>
#include <chrono>
#include <algorithm>
#include <cstdio>

std::vector<double> measure_latencies(int n_trials /*, your processing function */) {
    std::vector<double> latencies_us;
    latencies_us.reserve(n_trials);
    for (int i = 0; i < n_trials; ++i) {
        auto t0 = std::chrono::high_resolution_clock::now();
        // process_one_block(...);   // e.g. Lesson 23's StreamingFIR::process_block
        auto t1 = std::chrono::high_resolution_clock::now();
        latencies_us.push_back(
            std::chrono::duration<double, std::micro>(t1 - t0).count());
    }
    return latencies_us;
}

void report_percentiles(std::vector<double> latencies_us) {
    std::sort(latencies_us.begin(), latencies_us.end());
    auto pct = [&](double p) { return latencies_us[(size_t)(p * latencies_us.size())]; };
    double mean = 0; for (double v : latencies_us) mean += v; mean /= latencies_us.size();
    printf("mean=%.2f us  p50=%.2f  p99=%.2f  p99.9=%.2f  max=%.2f\n",
           mean, pct(0.50), pct(0.99), pct(0.999), latencies_us.back());
}
```

### 9. Experiment

Run §8's harness for a large number of trials (e.g. 100,000) under two conditions: system mostly idle, and system under artificial background load (spin up a few CPU-bound competing threads or processes). Compare the resulting `mean`, `p50`, `p99`, `p99.9`, and `max` between the two runs.

### 10. Expected output

Median should stay relatively stable across both conditions. `p99.9` and `max` should degrade substantially under background load — direct, measured confirmation that a benchmark run on a quiet development machine can look fine on every statistic *except* the one that actually matters in real deployment, where competing load is the normal condition, not the exception.

### 11. Debugging exercise

Symptom, no cause given: your pipeline's benchmark reports excellent average latency, comfortably within budget, and every existing test passes — yet in real deployment, users occasionally experience audible glitches or dropped samples, at a low but consistent, nonzero rate.

Using only §5: what's almost certainly wrong with how the benchmark's pass/fail criterion was defined, and what's likely different about the deployment environment versus wherever the benchmark was run?

**Root cause (revealed):** the benchmark's pass/fail criterion is checking **mean or median** latency against budget, exactly §5(a)'s blind spot — real, rare tail events cross the deadline at a low rate the mean-based check will never flag, while the *median* stays comfortably fine. Very likely compounding this: the benchmark ran on a quiet development machine (§9's "idle" condition), while real deployment involves genuine background load — meaning both the pass/fail statistic *and* the test environment understated real risk simultaneously. The fix is precisely §9's method: benchmark under realistic competing load, and gate pass/fail on a high percentile (p99.9) or true observed maximum against the actual deadline, not the mean.

### 12. Common mistakes

- Reporting or optimizing against mean or median latency for a hard real-time system — the single biggest, most common conceptual error this lesson corrects.
- Benchmarking only on an idle development machine and extrapolating to production — §9/§11 directly demonstrate the tail's sensitivity to background load, which a quiet machine systematically hides.
- Estimating throughput as the reciprocal of one call's best-case latency — this ignores tail events and inter-call overhead entirely, routinely overstating true sustainable throughput by a meaningful margin.

### 13. Real RF/cellular application

Real-time modem/PHY software is evaluated exactly this way in practice: deadline-miss rate and tail-latency percentiles — not average latency — are the metrics that actually determine whether a design meets a hard cellular timing budget, whether that's LTE's HARQ round-trip timing or NR's much tighter, numerology-dependent slot budgets from Lesson 28. This curriculum's career-map research explicitly named "latency analysis" and "throughput analysis" as real, named skills in DSP/PHY job postings — this lesson is that skill, and it directly sets the actual acceptance criteria for Project 9 (Real-Time SDR Processing Pipeline).

### 14. Interview question

*"Why is average latency the wrong metric for a hard real-time system, and what should you report instead?"* Answer from §5(a)'s concrete numbers: average latency can look completely fine while a system still regularly misses its deadline, because rare, real tail events — exactly what determines deadline compliance — get washed out by averaging. Report and enforce a high percentile (p99.9) or true observed worst-case against the actual deadline instead.

### 15. Active recall

1. Why can mean latency look excellent even when a system genuinely misses its deadline at a real, nonzero rate?
2. Why is deadline-miss rate the same kind of statistical quantity as BER, and what does that let you reuse?
3. Why does benchmarking only on an idle machine understate real deployment risk specifically in the tail, not just uniformly?

### 16. Mini assignment

Run §8/§9's experiment on your own machine, report your own measured `p50`/`p99`/`p99.9`/`max` under both idle and loaded conditions, and identify the percentile at which the two conditions' numbers first diverge meaningfully.

### 17. Exit test

1. In one sentence, why does a heavily right-skewed latency distribution make the mean a misleading summary statistic?
2. True or false, with a one-line reason: "If a pipeline's average latency is well under its deadline budget, the pipeline is real-time safe."
3. Your diagnosis from §11 connecting the symptom to both the wrong statistic and the wrong test environment, if you haven't sent it.

---

**Next:** Lesson 37 continues Stage 12 with **memory-safety tooling** — AddressSanitizer and Valgrind — catching buffer overflows and use-after-free bugs with the same measure-don't-assume discipline this lesson just applied to timing, closing the loop back to Lesson 24's concurrency work with the formal verification tools that make "I tested it and it seemed fine" unnecessary.

---

[← Lesson 35](lesson-35-hil-testing-test-vectors.md) · [Table of Contents](../README.md) · [Lesson 37 →](lesson-37-address-sanitizer.md)