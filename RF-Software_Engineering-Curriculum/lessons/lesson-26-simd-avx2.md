[← Table of Contents](../README.md) · [← Lesson 25](lesson-25-fixed-point-arithmetic.md)

---

# Lesson 26 — SIMD Intrinsics: Hand-Vectorizing a DSP Kernel with AVX2

*Stage 9/10, continued. Lesson 22 hoped the compiler would auto-vectorize a well-laid-out loop. This lesson stops hoping and takes explicit, direct control — at the cost of portability, which becomes its own real engineering problem by the end.*

### 1. Intuition

Lesson 22 showed that laying data out as SoA lets the *compiler* auto-vectorize a loop — but auto-vectorization is a heuristic the compiler applies only when it can *prove* a transformation is safe, and real DSP kernels often have structure (aliasing the compiler can't rule out, unusual access patterns, or simply being "complicated enough") that defeats this even with perfect data layout. **SIMD intrinsics** bypass the compiler's judgment call entirely: you explicitly say "load 8 floats into this register, multiply all 8 against these other 8, right here" — trading portability and readability for total, direct control over exactly how many samples move per instruction.

### 2. The engineering problem that forces this

You have a correct, tested scalar kernel (Lesson 23's FIR inner loop is the natural target) that the compiler won't vectorize well enough on its own, and a real deadline demanding more throughput. You need to explicitly load multiple samples into a wide register, compute on all of them simultaneously, and handle the unavoidable reality that your data length is almost never an exact multiple of the vector width — requiring deliberate handling of a "remainder" at the end, not an edge case you can wish away.

### 3. Mathematical foundation

No new math — the same FIR dot product as Lesson 23. The foundation here is the **SIMD execution model**: one instruction operates on multiple data **lanes** simultaneously. AVX2 (x86) processes 8×32-bit floats (256 bits) per register per instruction; NEON (ARM) commonly processes 4×32-bit floats (128 bits) per register per instruction.

### 4. Formal definition

A **vector register** holds multiple independent **lanes** of data. **FMA** (fused multiply-add) computes $a\times b + c$ across all lanes in one instruction, with a single rounding step — both faster and more numerically accurate than a separate multiply followed by an add. **Vertical** operations (add, multiply, FMA) act lane-wise and are fast and native to the hardware; **horizontal** operations (summing all lanes of one register into a single scalar) require dedicated, comparatively more expensive reduction instructions, because SIMD hardware is built for lane-parallel work, not cross-lane communication.

### 5. Derivation — restructuring the FIR dot product for vector execution

The scalar inner loop is `acc += taps[k] * history[k]` for $k=0,\dots,M-1$ — a sequential dot product. The vectorized version processes 8 taps and 8 history values at once: load 8 contiguous taps into one register, 8 contiguous history samples (contiguous specifically *because* of Lesson 22's SoA layout choice) into another, and issue a single FMA instruction accumulating all 8 products into a vector accumulator — roughly an 8× reduction in instruction count for this part of the loop, compared to 8 separate scalar multiply-adds. But you're left with a **vector** accumulator holding 8 *partial* sums, one per lane, that must be horizontally reduced into the single scalar output you actually want. This reduction step is comparatively expensive *per element*, but it happens exactly **once per output sample**, regardless of how many taps $M$ has — its cost is amortized across every vertical FMA that preceded it, so it becomes negligible as $M$ grows, a genuine, quantifiable trade-off rather than something to take on faith. Finally: $M$ is almost never an exact multiple of 8, so the loop processes $\lfloor M/8\rfloor \times 8$ taps with SIMD and handles the remaining $M \bmod 8$ taps with an ordinary scalar loop — a standard, unavoidable pattern in essentially all real SIMD code.

### 6. Visualization

```
Vector accumulator, 8 lanes (AVX2, __m256):
[p0][p1][p2][p3][p4][p5][p6][p7]   ← 8 independent partial products, computed in parallel

Horizontal reduction (happens ONCE, not per-tap):
[p0][p1][p2][p3][p4][p5][p6][p7]
   \  /    \  /    \  /    \  /
   [+]      [+]      [+]      [+]
      \    /            \    /
       [+]                [+]
          \              /
              [ final sum ]
```

### 7. Worked example

For $M=19$ taps: process 2 full groups of 8 (16 taps) via 2 vector FMA instructions instead of 16 scalar multiply-adds, then handle the remaining 3 taps with an ordinary scalar loop — the "vector chunk plus scalar remainder" pattern, concretely.

### 8. Code

```cpp
#include <immintrin.h>
#include <cstddef>

float horizontal_sum_avx(__m256 v) {
    __m128 lo = _mm256_castps256_ps128(v);
    __m128 hi = _mm256_extractf128_ps(v, 1);
    lo = _mm_add_ps(lo, hi);
    __m128 shuf = _mm_movehdup_ps(lo);
    __m128 sums = _mm_add_ps(lo, shuf);
    shuf = _mm_movehl_ps(shuf, sums);
    sums = _mm_add_ss(sums, shuf);
    return _mm_cvtss_f32(sums);
}

float fir_dot_avx2(const float* taps, const float* history, size_t M) {
    __m256 acc = _mm256_setzero_ps();
    size_t k = 0;
    for (; k + 8 <= M; k += 8) {
        __m256 t = _mm256_loadu_ps(&taps[k]);
        __m256 h = _mm256_loadu_ps(&history[k]);
        acc = _mm256_fmadd_ps(t, h, acc);   // acc += t*h, one instruction, 8 lanes
    }
    float sum = horizontal_sum_avx(acc);
    for (; k < M; ++k) {                     // scalar remainder, Sec 5's unavoidable tail
        sum += taps[k] * history[k];
    }
    return sum;
}
```

### 9. Experiment

Compile with `-O3 -mavx2 -mfma` (both AVX2 and FMA must be explicitly enabled — they are **not** on by default even at `-O3`, exactly Lesson 22's "check your build flags" lesson returning in a new form) and benchmark `fir_dot_avx2` against Lesson 23's scalar `process_sample` loop for a large tap count across many iterations. Test explicitly with tap counts that are and aren't multiples of 8, to exercise the remainder path.

### 10. Expected output

A real, measurable speedup over the scalar version at sufficiently large $M$ — the exact factor depends on your specific CPU and compiler, so measure it rather than trust a hardcoded expectation, per Lesson 22's same epistemic-humility point. Results should match the scalar reference to floating-point precision for every tap count tested, remainder cases included.

### 11. Debugging exercise

Symptom, no cause given: your AVX2 code compiles cleanly and works perfectly on your development machine. After deploying the same built binary to a different machine, it crashes immediately with an illegal-instruction fault (SIGILL) the moment the vectorized function runs.

What's different between "your development machine" and "a different machine" that this symptom points directly at?

**Root cause (revealed):** the deployment machine's CPU **doesn't support AVX2**. A binary compiled with `-mavx2` contains literal AVX2 machine instructions; if it runs on a CPU that doesn't implement that instruction set (older hardware, or a different CPU generation/vendor), the processor cannot decode those instructions at all and faults immediately — this is a genuine, common deployment bug in real HPC/DSP software, not a coding error in the usual sense. The standard, production fix is **runtime CPU-feature detection** (e.g., `__builtin_cpu_supports("avx2")`) combined with dispatching to the appropriate code path — AVX2, a narrower SSE fallback, or plain scalar — chosen once at startup based on the actual hardware the binary is currently running on, never assumed from the build machine.

### 12. Common mistakes

- Forgetting `-mavx2 -mfma` at **compile** time — a distinct failure point from §11's **deployment**-time SIGILL; the compile-time version typically shows up as a build error about unavailable intrinsics, not a runtime crash. Keep the two failure points conceptually separate — they have different causes and different fixes.
- Using aligned loads (`_mm256_load_ps`) on a pointer that isn't actually aligned to the required 32-byte boundary — a real, sharp footgun that can crash or (on some platforms) silently misbehave; default to unaligned loads (`_mm256_loadu_ps`, as used throughout §8) unless you've deliberately allocated aligned buffers and verified it.
- Forgetting the scalar remainder loop entirely, silently dropping the last `M mod 8` taps — an easy, common bug on a first vectorized implementation that produces a subtly wrong (not crashed) result, exactly the kind of error that could be mistaken for ordinary floating-point rounding if you don't verify against a scalar reference at every tap count, remainder cases included.

### 13. Real RF/cellular application

This is precisely how libraries like GNU Radio's **VOLK** (flagged back in Lesson 22) achieve real-world DSP throughput: VOLK ships multiple implementations of the same kernel — generic/scalar, SSE, AVX, AVX2, NEON — and performs runtime dispatch based on detected CPU features, exactly §11's fix, as a matter of standard practice rather than an afterthought. Every serious PHY/baseband C++ codebase handling real cellular sample rates (LTE's 30.72 Msps and beyond) relies on hand-vectorized inner loops for FIR filtering, correlation, and FFT-adjacent operations — this lesson is the direct, concrete payoff of Lessons 22 through 26 taken together: turning a Python-prototyped algorithm into code that can actually keep pace with a real radio.

### 14. Interview question

*"You've hand-vectorized a DSP kernel with AVX2 and it works perfectly on your machine. What's a real risk when deploying this binary elsewhere, and how do you guard against it?"* Answer from §11: not every deployment CPU supports AVX2; a binary built assuming it will crash with an illegal-instruction fault on hardware lacking it. The standard guard is runtime CPU-feature detection with multi-path dispatch, exactly VOLK's real production approach — never assume the build machine's capabilities match every deployment target.

### 15. Active recall

1. Why are vertical SIMD operations fast while horizontal reduction is comparatively expensive — what does that asymmetry come from?
2. Why does the "vector chunk plus scalar remainder" pattern show up in essentially all real SIMD code, rather than being avoidable with careful sizing?
3. What real-world deployment failure does §11 describe, and what's the standard production fix?

### 16. Mini assignment

Implement and test `fir_dot_avx2` against Lesson 23's scalar reference across several tap counts, deliberately including non-multiples of 8 to exercise the remainder path. Then write (pseudocode is fine) a small dispatch function using `__builtin_cpu_supports("avx2")` that calls the AVX2 path when available and falls back to the scalar path otherwise.

### 17. Exit test

1. Explain why the horizontal-sum step in §8's code happens once per FIR output sample, not once per tap, and why that amortization matters.
2. True or false, with a one-line reason: "A binary compiled with -mavx2 will run correctly on any x86-64 CPU."
3. Your reasoning from §11 about the SIGILL deployment failure, if you haven't sent it.

---

**Next:** This closes out the core Lessons 22–26 performance arc — memory layout, real-time buffering, concurrency, fixed-point, and hand-vectorization. Both Lesson 22 and Lesson 25 flagged "don't optimize before profiling" as a mistake to avoid; Lesson 27 finally teaches **profiling properly** — how to measure where time actually goes, rather than guessing, closing that recurring thread before this curriculum returns to finishing 5G NR.

---

[← Lesson 25](lesson-25-fixed-point-arithmetic.md) · [Table of Contents](../README.md) · [Lesson 27 →](lesson-27-profiling-amdahl.md)