[← Table of Contents](../README.md) · [← Lesson 24](lesson-24-multithreading-lockfree.md)

---

# Lesson 25 — Fixed-Point Arithmetic: Q-Format, Overflow, and the 6.02B+1.76 dB Rule

*Stage 9/10, continued. Where the "noise floor" you've been fighting since Lesson 7 gets a second source — one your own code introduces, deterministically, and can be derived with exactly the same rigor.*

### 1. Intuition

Floating point is convenient, but many real embedded DSP cores — older TI DSPs, many FPGA/ASIC PHY implementations, power-constrained mobile SoC baseband cores — either lack a hardware floating-point unit entirely or make floating-point operations far more expensive in cycles and power than integer arithmetic. **Fixed-point** represents real numbers using ordinary integers with an implied, fixed binary-point position: fast, cheap, power-efficient, deterministic hardware cost — at the price of the programmer now being personally responsible for tracking scale explicitly, something floating point silently handled for you.

### 2. The engineering problem that forces this

You need to represent signal values with adequate precision and dynamic range using pure integer arithmetic, choosing exactly where the "binary point" sits, such that a real computation — an FIR filter's multiply-accumulate, exactly Lesson 23's kernel — doesn't silently overflow or lose more precision than you intended. This is a design decision made once, deliberately, at implementation time; there's no automatic dynamic-range safety net the way floating point provides one.

### 3. Mathematical foundation

**Q(m.n) format**: $m$ integer bits (including sign) and $n$ fractional bits; the stored integer $I$ represents the real value $I/2^n$. **Q15** (very common: a 16-bit word, 1 implicit sign bit, 15 fractional bits) represents values in $[-1, 1-2^{-15}]$.

**Multiplication**: two Q15 values multiplied give a Q30 result — it must be right-shifted back down to Q15 before further Q15 arithmetic, and that shift is a deliberate, information-losing step. **Addition**: operands must be in the *same* Q format first — exactly as you can't add meters and feet without converting.

### 4. Formal definition

**Overflow**: a result exceeds the representable range for its format, wrapping (or saturating, if the hardware/library is configured that way) instead of growing, unlike floating point's huge dynamic range. **Quantization noise**: the fractional information a finite format cannot represent gets truncated or rounded away — a genuine, measurable noise floor your own arithmetic introduces, mechanistically different from Lesson 7's channel-injected AWGN but with an entirely analogous effect on signal quality.

### 5. Derivation — quantization noise power, and the famous 6.02B+1.76 dB formula, from first principles

Model rounding error for step size $\Delta = 2^{-n}$ as uniformly distributed on $[-\Delta/2, \Delta/2]$ (a standard, reasonable assumption for a signal that isn't pathologically correlated with the quantizer grid). Its variance:
$$\text{Var} = \int_{-\Delta/2}^{\Delta/2} x^2 \cdot \frac{1}{\Delta}\,dx = \frac{1}{\Delta}\left[\frac{x^3}{3}\right]_{-\Delta/2}^{\Delta/2} = \frac{\Delta^2}{12}$$
For a signed $B$-bit representation spanning $[-1,1)$, the step size is $\Delta = 2^{1-B}$, so quantization noise power is $\Delta^2/12 = 2^{2-2B}/12$. A full-scale sinusoid (peak amplitude 1) has average power $1/2$ (Lesson 7-style average-power-of-a-sinusoid fact). The signal-to-quantization-noise ratio:
$$\text{SQNR} = \frac{1/2}{2^{2-2B}/12} = 6\cdot 2^{2B-2} = 1.5\cdot 2^{2B}$$
Convert to dB:
$$\text{SQNR (dB)} = 10\log_{10}(1.5) + 20B\log_{10}(2) = 1.76 + 6.02B$$
— the famous **"6.02B + 1.76 dB" rule**, derived rather than quoted. It's an SNR formula in exactly Lesson 7's sense, except this noise source is your own number format, not the physical channel.

### 6. Visualization

```
Q15 bit layout (16-bit word):
[S][f14][f13]...[f1][f0]     value = integer / 2^15
 ↑ sign bit    ↑ 15 fractional bits

Quantization (stair-step) vs. true continuous value:
true:  ╱‾‾‾‾╲
       actual signal
quant: ▔▔╱▔▔╲▔▔     ← rounded to nearest representable step, error = difference
```

### 7. Worked example

Q15 stored in a 16-bit word ($B=16$): SQNR $= 6.02(16)+1.76 = 98.1$ dB — the well-known "16-bit gives about 98 dB" figure. An 8-bit format ($B=8$, exactly Lesson 15's RTL-SDR native `cu8` format): SQNR $=6.02(8)+1.76\approx49.9$ dB — a direct, concrete numerical link back to why cheap 8-bit SDR hardware has a visibly higher noise floor than 16-bit lab-grade equipment, for a reason you can now derive rather than just recall.

### 8. Code

```cpp
#include <cstdint>
#include <cstdio>

using q15_t = int16_t;

// Fixed-point multiply: Q15 * Q15 -> Q30 in a 32-bit accumulator, then shift back to Q15
inline q15_t q15_mul(q15_t a, q15_t b) {
    int32_t product = (int32_t)a * (int32_t)b;   // Q30 result
    return (q15_t)(product >> 15);                // back to Q15 -- the deliberate shift, Sec 3
}

// A small fixed-point FIR (reusing Lesson 23's ring-buffer structure conceptually)
q15_t fir_step_fixed(const q15_t* taps, const q15_t* history, size_t M) {
    int32_t acc = 0;                      // WIDE accumulator -- see Sec 11's overflow bug
    for (size_t k = 0; k < M; ++k) {
        acc += (int32_t)taps[k] * (int32_t)history[k];   // stays in Q30, summed
    }
    return (q15_t)(acc >> 15);            // one shift at the end, not per-term
}

int main() {
    q15_t taps[3]    = { 16384, 16384, 16384 };  // ~0.5 in Q15 each (16384/32768 = 0.5)
    q15_t history[3] = { 32767, 32767, 32767 };  // ~1.0 in Q15 (near full scale)
    q15_t y = fir_step_fixed(taps, history, 3);
    printf("fixed-point FIR output (Q15 raw): %d  (~%f)\n", y, y / 32768.0);
}
```

### 9. Experiment

Run §8's fixed-point FIR against the same taps and input (converted to floating point) through Lesson 23's `StreamingFIR`, and compare outputs sample-for-sample. Measure the actual RMS error between the two and convert to an empirical SNR; compare it against §5's theoretical $6.02(16)+1.76\approx98$ dB prediction for Q15. Then deliberately choose taps and input values that are all near full-scale and in-phase (as in §8's example) to push the accumulator toward its limits, and watch what happens as you shrink the accumulator's bit width in the code.

### 10. Expected output

With a properly wide (32-bit) accumulator, the fixed-point output should track the floating-point reference closely, with an empirical SNR reasonably close to the ~98 dB theoretical prediction (some gap is expected — the uniform-noise assumption in §5 is a model, not an exact guarantee for every specific input). With a deliberately narrowed accumulator, output should show sudden, large, qualitatively different errors, not just added noise.

### 11. Debugging exercise

Symptom, no cause given: your fixed-point FIR filter's output isn't just slightly noisier than the floating-point reference — it's wildly wrong, occasionally flipping sign for no apparent reason, specifically when several large-magnitude taps and samples happen to align.

This is a genuinely different failure signature from ordinary quantization noise (§5's smooth, small, predictable degradation). What class of problem produces a sudden, large, sign-flipping error instead?

**Root cause (revealed):** **accumulator overflow** — a fundamentally different failure mode from quantization noise, and worth clearly distinguishing from it. Unlike floating point's huge dynamic range, a fixed-point accumulator has a hard maximum representable value; if the sum of several large-magnitude tap×sample products exceeds it, two's-complement arithmetic **wraps around** rather than saturating, producing a value that can be wildly, discontinuously wrong — including an apparent sign flip, since wrapping past the maximum positive value lands you at the most negative representable value. The fix is architectural, decided in advance: use a wide enough accumulator (32-bit or wider for 16-bit×16-bit products summed over many taps) sized for the *worst-case* sum, not the typical one, or use explicit saturating arithmetic instead of silent wraparound.

### 12. Common mistakes

- Adding fixed-point values in different Q formats without aligning them first — the exact "meters and feet" error, silently producing a numerically valid but meaningless result.
- Forgetting the post-multiply shift (§3), leaving a result in the wrong Q format — an easy, silent scale-factor bug (off by a power of 2) that can look almost-plausible rather than obviously broken.
- Sizing accumulator headroom based only on a single worst-case *term*, rather than the worst-case *sum* across all terms — an FIR with many taps can overflow from accumulated summation even when no single tap×sample product alone would, a genuinely easy oversight when reasoning term-by-term instead of about the full sum.

### 13. Real RF/cellular application

Many current embedded and ASIC baseband DSP cores in power-constrained mobile chips use fixed-point arithmetic specifically for the power/area/cost reasons in §1 — floating-point hardware costs real silicon area and power budget on a mobile SoC. Getting Q-format scaling correct throughout an entire LTE/NR PHY chain implemented on such a core is a genuine, often underappreciated engineering task, and this curriculum's career-map research turned up "fixed-point scaling" as an explicitly named skill in real DSP/PHY engineer job postings — this lesson's debugging exercise (§11) is precisely the classic real bug category in that work.

### 14. Interview question

*"Why does a 16-bit fixed-point format give roughly 98 dB of dynamic range — and where does that number actually come from?"* Answer by deriving, not reciting: uniform quantization error variance $\Delta^2/12$ (§5), combined with full-scale sinusoidal signal power, produces the general $6.02B+1.76$ dB relationship — a specific number that falls out of a specific, reproducible derivation, not a value to memorize as a fact on its own.

### 15. Active recall

1. Derive, from memory, why quantization noise variance equals $\Delta^2/12$.
2. What's the essential difference between quantization-noise degradation and accumulator overflow, in terms of how each actually looks in output data?
3. Why must fixed-point addition operands share the same Q format, while multiplication doesn't have that restriction?

### 16. Mini assignment

Using §8's code, deliberately narrow the accumulator to `int16_t` (forcing genuine overflow with the given near-full-scale taps/history), observe and record the resulting wraparound artifact, then fix it by restoring a wide accumulator and confirm correct behavior returns.

### 17. Exit test

1. Write the SQNR formula for an 8-bit and a 16-bit fixed-point format, and state which real format from an earlier lesson uses 8-bit samples.
2. True or false, with a one-line reason: "Fixed-point overflow and quantization noise are two names for the same underlying phenomenon."
3. Your reasoning from §11 distinguishing overflow from ordinary quantization noise, if you haven't sent it.

---

**Next:** Lesson 26 moves to **SIMD intrinsics**: hand-vectorizing a DSP kernel with AVX2/NEON directly, turning Lesson 22's "the compiler might auto-vectorize this" into "I explicitly control exactly how many samples move per instruction."

---

[← Lesson 24](lesson-24-multithreading-lockfree.md) · [Table of Contents](../README.md) · [Lesson 26 →](lesson-26-simd-avx2.md)