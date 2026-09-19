[← Table of Contents](../README.md) · [← Lesson 21](lesson-21-rlc-pdcp-rrc.md)

---

# Lesson 22 — From Python to C++: Memory Layout, std::complex, and Why "Compiled" Doesn't Automatically Mean "Fast"

*Opens the C++ implementation track (Stage 9/10 territory). Every lesson since Lesson 1 used Python/NumPy for convenience. Real-time PHY software runs in C++ for reasons tied directly to deadlines — and the very first surprise most people hit when making that jump has nothing to do with algorithms.*

### 1. Intuition

Lesson 1 built your mental model of complex numbers and phasors; everything since has used Python because it's fast to iterate in. Real DSP software runs in C/C++ because real-time deadlines (the whole premise of Stage 10) demand it. The natural instinct is to translate a working NumPy function line-by-line into a C++ loop and expect a speedup "because compiled code is fast." That instinct is usually wrong on the first attempt — and the reason is almost never "C++ is slow." It's that Python/NumPy was quietly doing several things right that you now have to do deliberately yourself, and the biggest one is *how your data sits in memory*, something Python's abstraction hid from you completely.

### 2. The engineering problem that forces this

You need to store and process a stream of complex IQ samples in C++ such that memory access is cache-friendly (sequential, not scattered), the compiler can auto-vectorize your inner loops (or you can hand-vectorize them later, a future lesson), and you're not silently paying for allocations or copies inside a hot loop that NumPy's design never let you pay for in the first place. This is the first of many places in this curriculum where "I understand the algorithm" and "I can make this run in real time" genuinely diverge.

### 3. Mathematical foundation

No new math here — Lesson 1 already covers the complex-number operations themselves. This lesson's foundation is **data layout**: **Array-of-Structs (AoS)** — `std::complex<float> samples[N]`, interleaved as re,im,re,im,... — versus **Struct-of-Arrays (SoA)** — separate `float re[N]; float im[N];`, each fully contiguous.

### 4. Formal definition

C++11 onward **guarantees** `std::complex<T>` has the identical memory layout as `T[2]` — i.e., contiguous real then imaginary, no hidden padding — making AoS the natural default and a genuinely reliable, portable choice. A **cache line** (typically 64 bytes on modern x86/ARM) is the unit the CPU actually fetches from memory — never a single value in isolation — so *how your data is laid out*, not just what you compute, directly determines how many of those fetches your algorithm needs.

### 5. Derivation — why AoS can silently defeat vectorization

Take a simple, common operation: compute $|x[n]|^2 = \text{re}[n]^2+\text{im}[n]^2$ for every sample — exactly the magnitude-squared step Lesson 6's correlation-peak search needs. With **SoA**, a SIMD instruction loads 8 consecutive `re[]` values into one vector register and 8 consecutive `im[]` values into another, then squares-and-adds all 8 lanes in parallel with two or three vector instructions total. With **AoS**, the 8 "re" values you want are *not* contiguous with each other — they're strided, one `float` apart from the next relevant one, interleaved with the `im` values you don't want yet. The CPU either needs an extra deinterleave/gather step, or the compiler falls back to scalar (one-sample-at-a-time) code, losing the SIMD speedup entirely. Nothing about the *algorithm* changed between the two versions — only where the bytes live — and that alone can be the difference between vectorized and scalar code.

### 6. Visualization

```
AoS (interleaved, std::complex<float>[N]):
[re0][im0][re1][im1][re2][im2][re3][im3]...
 ↑ the 4 "re" values you want for a SIMD load are NOT adjacent

SoA (planar, separate arrays):
re: [re0][re1][re2][re3][re4][re5][re6][re7]...  ← one contiguous SIMD load
im: [im0][im1][im2][im3][im4][im5][im6][im7]...  ← another contiguous SIMD load
```

### 7. Worked example

Computing magnitude-squared for 8 samples: with SoA, a single 256-bit AVX register load pulls in all 8 `re` values in one instruction, another pulls in all 8 `im` values, and one fused multiply-add-style sequence produces all 8 results. With AoS, the compiler either has to shuffle the interleaved data apart first (extra instructions, real overhead) or gives up and processes 8 samples with 8 separate scalar operations — the same math, a very different number of actual CPU instructions.

### 8. Code

```cpp
#include <vector>
#include <complex>
#include <chrono>
#include <cstdio>

// AoS: the "obvious" direct translation from Python
float magsq_aos(const std::vector<std::complex<float>>& x) {
    float sum = 0.0f;
    for (size_t n = 0; n < x.size(); ++n) {
        sum += std::norm(x[n]);           // norm() = re*re + im*im, no sqrt -- see Sec 12
    }
    return sum;
}

// SoA: real and imaginary parts kept in separate contiguous arrays
float magsq_soa(const std::vector<float>& re, const std::vector<float>& im) {
    float sum = 0.0f;
    for (size_t n = 0; n < re.size(); ++n) {
        sum += re[n]*re[n] + im[n]*im[n];
    }
    return sum;
}

int main() {
    const size_t N = 10'000'000;
    std::vector<std::complex<float>> aos(N, {1.0f, 2.0f});
    std::vector<float> re(N, 1.0f), im(N, 2.0f);

    auto t0 = std::chrono::high_resolution_clock::now();
    float r1 = magsq_aos(aos);
    auto t1 = std::chrono::high_resolution_clock::now();
    float r2 = magsq_soa(re, im);
    auto t2 = std::chrono::high_resolution_clock::now();

    printf("AoS: %f  (%ld us)\n", r1,
           std::chrono::duration_cast<std::chrono::microseconds>(t1-t0).count());
    printf("SoA: %f  (%ld us)\n", r2,
           std::chrono::duration_cast<std::chrono::microseconds>(t2-t1).count());
}
```

### 9. Experiment

Compile and run §8's code **twice**: once with no optimization flag (`g++ file.cpp -o test`, which defaults to `-O0`), and once with `g++ -O3 file.cpp -o test`. Record all four timings (AoS/SoA × O0/O3). Separately, if your compiler supports it, try adding `-fopt-info-vec` (GCC) to see a direct report of which loops actually got vectorized — real, concrete evidence rather than a guess.

### 10. Expected output

At `-O0`: both versions will likely be slow and fairly close to each other — optimization is disabled, so neither gets vectorized regardless of layout. At `-O3`: SoA is often noticeably faster than AoS, sometimes dramatically so, though the *exact* margin depends on your specific compiler and CPU — measure on your own machine rather than trust a hardcoded expectation here.

### 11. Debugging exercise

Symptom, no cause given: you port a working NumPy DSP function to C++, expecting a large speedup since it's now a compiled language instead of an interpreted one. Your new C++ version is barely faster than the NumPy version — sometimes even slower for large arrays.

Before suspecting your algorithm or your data layout: what's the very first, cheapest thing to check about *how you built the program itself*?

**Root cause (revealed):** almost always a **missing optimization flag**. A fresh C++ project typically compiles at `-O0` (debug, unoptimized) by default unless you explicitly request otherwise — while NumPy's underlying C code was *always* compiled fully optimized, regardless of what Python script calls into it. Comparing your own `-O0` C++ build against NumPy is comparing unoptimized code to optimized code, which has nothing to do with "C++ vs. Python" as languages. This is the single most common, most surprising-to-beginners cause of exactly this symptom — check build flags before investigating anything more sophisticated.

### 12. Common mistakes

- Forgetting optimization flags — §11's mistake, common enough to check first, every time, before any deeper investigation.
- Using `std::abs()` (which computes $\sqrt{re^2+im^2}$) inside a hot loop when only *comparing* magnitudes is needed — `std::norm()` (which gives $|z|^2$, no square root) is sufficient and cheaper whenever you're comparing relative sizes, e.g. searching for the tallest peak in Lesson 6's correlation output; the ordering is identical, and skipping the square root entirely on every single sample compounds into a real, measurable saving.
- Restructuring an entire codebase to SoA before ever profiling — committing to this data-layout change without first confirming (via profiling, coming soon in this curriculum) that a specific loop is actually a bottleneck is itself a real engineering mistake: real complexity and maintenance cost, paid for a benefit that was never measured, let alone confirmed necessary.

### 13. Real RF/cellular application

This exact interleaved-versus-planar IQ consideration is a standard, explicit design decision in real SDR/DSP libraries — GNU Radio's VOLK library, for instance, ships separate kernel variants specifically for interleaved-complex and deinterleaved (planar) data for precisely this reason, and some hardware ADC/SDR interfaces natively output planar IQ for the same underlying cause. Every serious PHY/baseband C++ codebase — srsRAN included — makes a deliberate, early architectural choice about IQ sample memory layout, for exactly the reasons derived in §5.

### 14. Interview question

*"You're asked to optimize a hot C++ DSP loop that underperforms compared to a NumPy prototype of the same algorithm. What's the very first thing you check, before considering algorithmic or data-layout changes?"* Answer from §11: verify the build's optimization flags first — the cheapest, most common real cause — and only move on to profiling-guided investigation of memory layout, allocations, or genuine algorithmic inefficiency once that's been ruled out.

### 15. Active recall

1. What memory-layout guarantee does C++11 make about `std::complex<T>`, and why does that make AoS a safe default?
2. Why can identical math run at very different speeds depending purely on AoS vs. SoA layout?
3. Why should `std::norm()` often be preferred over `std::abs()` inside a hot comparison loop?

### 16. Mini assignment

Run §8's code across all four combinations (AoS/SoA × O0/O3) and tabulate the results. Then use `-fopt-info-vec` (or your compiler's equivalent) to confirm, directly rather than by inference, which loop(s) actually got vectorized at `-O3` — connecting the measured timing difference to concrete compiler behavior rather than a hypothesis.

### 17. Exit test

1. Name the two axes this lesson varied (2×2 in the mini assignment) and state which one is the "language/compiled vs. interpreted" myth-buster, and which is the genuine algorithmic-engineering lesson.
2. True or false, with a one-line reason: "`std::complex<float>` and a raw `float[2]` are guaranteed to have the same memory layout in C++11 and later."
3. Your reasoning from §11 about the very first thing to check, if you haven't already internalized it — this single habit will save you real debugging time throughout the rest of this curriculum's C++ work.

---

**Next:** Lesson 23 builds directly on this: implementing your first real **FIR filter in C++** (Lesson 4's math, now as deadline-driven code) — ring buffers, avoiding per-sample heap allocation, and the first genuine real-time-flavored C++ of this curriculum.

---

[← Lesson 21](lesson-21-rlc-pdcp-rrc.md) · [Table of Contents](../README.md) · [Lesson 23 →](lesson-23-cpp-fir-ring-buffer.md)