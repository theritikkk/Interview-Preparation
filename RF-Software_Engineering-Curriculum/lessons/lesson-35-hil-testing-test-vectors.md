[← Table of Contents](../README.md) · [← Lesson 34](lesson-34-debugging-methodology.md)

---

# Lesson 35 — Hardware-in-the-Loop Testing and Test Vectors: From One-Off Investigation to Automated Regression Testing

*Stage 12, continued. Lesson 34 built a methodology for investigating a bug once someone notices it. This lesson is about never needing a human to notice in the first place — and it turns out designing a good regression test needs exactly the same statistical reasoning as a BER curve, now for a third completely different purpose.*

### 1. Intuition

Lesson 34's methodology is for investigation — a human noticed a symptom and needs to find the cause efficiently. Real engineering teams need something that runs *automatically*, catching a regression the moment code changes, before any human observes anything wrong. This needs two things: **test vectors** — fixed, versioned, known-good input/output pairs for each stage — and **hardware-in-the-loop (HIL)** testing, since real hardware introduces impairments (Lesson 15's DC offset and IQ imbalance, Lesson 34's thermal drift) that no simulated channel, however sophisticated, fully replicates. Designing both correctly turns out to need real statistical care, not just "compare the output to what we got last time."

### 2. The engineering problem that forces this

A test suite must catch real regressions reliably, *without* flagging false failures from ordinary numerical variation between runs or platforms, and must meaningfully exercise real hardware-induced behavior rather than only clean, simulated signals that might keep passing even after real-world robustness has silently regressed. Both requirements point to the same underlying need: deliberately, quantitatively justified tolerances — not exact equality, and not an arbitrarily loose threshold either.

### 3. Mathematical foundation

For a stochastic metric like BER, comparing a new measurement $\hat p$ (over $N$ bits) against a baseline $p_0$ needs a **statistically grounded** pass/fail rule, not a raw point comparison — reusing Lesson 7 §12 and Lesson 27 §5(a)'s standard-error logic a third time, now for regression-test design specifically:
$$\text{SE} = \sqrt{\frac{\hat p(1-\hat p)}{N}}, \qquad z = \frac{\hat p - p_0}{\text{SE}}$$
Flag a regression only when $z$ exceeds a chosen significance threshold (e.g. $|z|>3$), not whenever $\hat p > p_0$ by any amount at all.

### 4. Formal definition

A **test vector** is a fixed, versioned `(input, expected_output, tolerance)` triple, checked into source control, validating one processing stage deterministically — or within a defined statistical tolerance, for stochastic outputs. **Hardware-in-the-loop (HIL)** testing routes a signal through real RF hardware (an SDR loopback, or genuine transmit-over-air-and-receive) rather than a purely mathematical channel model, specifically to catch hardware-induced behavior no simulation fully captures.

### 5. Derivation — two reasons exact equality is the wrong test design

**(a) The statistical case, worked with real numbers.** Baseline $p_0=0.01$ measured over $N_0=100{,}000$ bits (respecting Lesson 7 §12's "enough trials" requirement). A new run measures $\hat p=0.0105$ over $N=100{,}000$: $\text{SE}=\sqrt{0.01\times0.99/100{,}000}\approx0.000315$, so $z=(0.0105-0.01)/0.000315\approx1.6$ — well within normal sampling variation, **not** a real regression, and a test that fails on this is simply flaky. Contrast $\hat p=0.013$: $z=(0.013-0.01)/0.000315\approx9.5$ — overwhelmingly significant, a genuine regression that absolutely should fail the build. Two numbers that look similarly "worse than baseline" by eye are, statistically, in completely different categories — exactly the kind of distinction a naive fixed threshold cannot make.

**(b) The floating-point case.** Floating-point addition is **not strictly associative** — summing the same values in a different order (different compiler, different optimization level per Lesson 22, a different SIMD path per Lesson 26, even different thread-scheduling order per Lesson 24) can legitimately produce tiny, last-bit-level differences in an otherwise completely correct computation. A test asserting exact bit-for-bit equality will spuriously fail across valid build configurations with **zero actual bugs present** — flakiness indistinguishable, to someone who doesn't know this, from (a)'s statistical flakiness, but with a completely different cause and fix. The correct tolerance isn't zero (too strict, guaranteed eventual false failures) or arbitrarily loose (too weak, misses real regressions) — it should be grounded in something concrete, such as Lesson 25's known fixed-point quantization floor, or an empirically-characterized floating-point error bound for the specific computation.

### 6. Visualization

```
Test vector file (conceptual):
{
  "input": [...],
  "expected_output": [...],
  "tolerance": 1e-5,          <- justified (e.g. Lesson 25's quantization floor), not arbitrary
  "metric": "max_abs_error"   <- or "z_score" for stochastic outputs, per Sec 3
}

Hardware-in-the-loop test loop:
[TX: known test vector] → [real SDR, real cable/air, Lesson 15's real impairments]
                                            ↓
                              [RX: real hardware capture]
                                            ↓
                         [compare against expected, WITH justified tolerance]
```

### 7. Worked example

A test vector for Lesson 4's FIR filter: fixed input array, expected output computed once by a trusted reference implementation, tolerance set to roughly $10\times$ machine epsilon for the accumulation length involved — tight enough to catch a genuine algorithmic regression (wrong tap, wrong sign, off-by-one), loose enough to tolerate legitimate cross-platform floating-point variation. A test vector for Lesson 7's BPSK BER: baseline $p_0$ and $N_0$ stored explicitly, compared via §3's $z$-score rather than a raw threshold.

### 8. Code

```python
import math

def ber_regression_check(p0, n0, p_hat, n, z_threshold=3.0):
    se = math.sqrt(p_hat * (1 - p_hat) / n)
    z = (p_hat - p0) / se if se > 0 else 0.0
    return {"z": z, "regression": abs(z) > z_threshold and p_hat > p0}

test_cases = [
    (0.01, 100_000, 0.0105, 100_000),   # small sampling variation -- should PASS
    (0.01, 100_000, 0.0130, 100_000),   # genuine regression -- should FAIL
    (0.01, 100_000, 0.0098, 100_000),   # slightly better -- should PASS
]
for p0, n0, p_hat, n in test_cases:
    result = ber_regression_check(p0, n0, p_hat, n)
    print(f"p0={p0} p_hat={p_hat}: z={result['z']:.2f}  "
          f"{'REGRESSION' if result['regression'] else 'pass'}")

# Floating-point order-sensitivity, Sec 5(b) made concrete:
values = [1e10, 1.0, -1e10, 1.0]
sum_forward = 0.0
for v in values: sum_forward += v
sum_reverse = 0.0
for v in reversed(values): sum_reverse += v
print("forward sum:", sum_forward, " reverse sum:", sum_reverse,
      " exactly equal:", sum_forward == sum_reverse)
```

### 9. Experiment

Run §8's code. Confirm the three BER test cases classify exactly as commented — the borderline case passing, the genuine regression failing. Then inspect the forward-vs-reverse summation result: depending on your platform, these may print as **not exactly equal**, despite representing the same mathematical sum — direct, hands-on confirmation of §5(b)'s claim, on your own machine rather than as an assertion to trust.

### 10. Expected output

The three BER cases classify correctly per §3's statistical logic. The order-sensitivity experiment demonstrates that even simple floating-point summation isn't guaranteed bit-exact across summation order — motivating tolerance-based comparison for essentially any nontrivial floating-point test, not as a special case but as the normal, expected situation.

### 11. Debugging exercise

Symptom, no cause given: your CI test suite is "flaky" — the *same* test, on the *same* code, with no changes at all, sometimes passes and sometimes fails, apparently at random across runs.

Using §5's two distinct mechanisms: what are the two genuinely different underlying causes this symptom could have, and how would you tell which one you're facing?

**Root cause (revealed):** two real, distinct possibilities, distinguished by what kind of output the test checks. If the test compares a **stochastic** quantity (a BER-like measurement) using a naive fixed threshold instead of §3's statistical comparison, ordinary sampling variation will occasionally cross that threshold at random — §5(a)'s failure mode. If the test asserts **exact equality** on a floating-point computation touched by nondeterministic execution order (parallel/threaded code, Lesson 24, or different optimization/SIMD paths across builds, Lessons 22/26), harmless last-bit differences will occasionally trip it — §5(b)'s failure mode. Distinguish them by checking whether the failing assertion involves a quantity that's inherently random (BER, a Monte Carlo estimate) or one that's mathematically deterministic but computed via floating-point accumulation — the fix differs (statistical threshold vs. numerical tolerance) even though the *symptom* looks identical from the outside.

### 12. Common mistakes

- Asserting exact equality on any output involving floating-point accumulation across many terms — the direct cause of §5(b)/§11's flakiness, and a very common test-design anti-pattern.
- Setting a tolerance so loose it would never catch a genuine regression, just to silence flaky failures — tolerance must be justified by something concrete (a known quantization floor, an empirical error bound), not tuned reactively until tests stop complaining.
- Relying on simulation-only testing and skipping hardware-in-the-loop entirely — missing exactly the class of real, hardware-induced regressions (Lesson 15/34-style) that no channel simulation, however sophisticated, can fully replicate.

### 13. Real RF/cellular application

Real modem and RAN software teams maintain exactly this layered test infrastructure: deterministic unit tests with justified tolerances for individual DSP stages, statistically-sound BER/BLER regression tests at the link level, and genuine hardware-in-the-loop rigs for full RF conformance and interoperability validation before shipping — precisely the core business of companies like Keysight, Rohde & Schwarz, and Anritsu, from this curriculum's career map. This lesson is the direct blueprint for **Project 12 (Hardware-in-the-Loop Wireless Test System)**.

### 14. Interview question

*"Your team's BER regression test occasionally fails even though nothing in the code changed. How would you diagnose and fix it?"* Answer from §3/§5(a): almost certainly a naive fixed-threshold comparison that doesn't account for BER's inherent sampling variance. The fix is a statistically-grounded pass/fail criterion — a standard-error-based z-test — reusing exactly the same "how many trials for a trustworthy estimate" reasoning from Lesson 7 and Lesson 27, now applied to test design specifically.

### 15. Active recall

1. Why is a fixed-threshold BER regression test statistically unsound, and what should replace it?
2. Why can the identical mathematical computation legitimately produce different floating-point results across valid build configurations?
3. Why is hardware-in-the-loop testing necessary even with a very sophisticated channel simulation?

### 16. Mini assignment

Extend §8's `ber_regression_check` into a small test-vector framework: given a list of `(name, p0, n0, p_hat, n)` tuples, produce a pass/fail report for all of them, and add a second checker function for deterministic array outputs using `numpy.allclose` with an explicitly justified (not arbitrary) tolerance.

### 17. Exit test

1. Write the z-score formula for BER regression testing from memory.
2. True or false, with a one-line reason: "Two floating-point computations that are mathematically equal will always produce bit-identical results on any correct implementation."
3. Your diagnosis from §11 distinguishing the two flakiness causes, if you haven't sent it.

---

**Next:** Lesson 36 continues Stage 12 with **latency and throughput analysis** for real-time pipelines — putting precise numbers on the deadline-meeting claims Lessons 23–27's real-time C++ work has been making informally, and building the measurement discipline to prove them rather than assume them.

---

[← Lesson 34](lesson-34-debugging-methodology.md) · [Table of Contents](../README.md) · [Lesson 36 →](lesson-36-latency-throughput.md)
