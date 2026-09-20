[← Table of Contents](../README.md) · [← Lesson 22](lesson-22-cpp-memory-layout.md)

---

# Lesson 23 — A Real-Time FIR Filter in C++: Ring Buffers and Why the Hot Path Must Never Allocate

*Stage 9/10, continued. Lesson 4 built an FIR filter as one batch operation on a signal you already had in full. A real receiver never has "the whole signal" — it has an endless stream, and this lesson is about processing it forever, in fixed memory, with zero surprises.*

### 1. Intuition

Lesson 4's FIR filter was `np.convolve(entire_signal, h)` — a single, one-shot operation on data you already had completely in hand. A real receiver doesn't get that luxury: samples arrive continuously, forever, and the filter must produce output continuously, forever, without accumulating unbounded memory and without ever pausing unpredictably. The tool that makes this possible is the **ring buffer**: a fixed-size array that wraps around on itself, giving you exactly the sliding window an FIR filter needs — without the buffer ever growing, shrinking, or being reallocated, no matter how many samples flow through it.

### 2. The engineering problem that forces this

You need to maintain the $M$ most recent input samples using a **fixed, small** amount of memory as an unbounded stream arrives — "keep the whole history" is both wasteful and, eventually, impossible. And critically: the processing loop must never call `malloc`/`new` in its hot path. Dynamic allocation has genuinely **unpredictable worst-case latency** — even though it's usually fast, an allocator can occasionally need to ask the OS for more memory, walk a free list, or contend on a lock — and a single unlucky allocation taking far longer than usual can silently blow a hard real-time deadline (Stage 10's central theme), even if it happens only once in a million calls. "Usually fast" is a Lesson-22 (average-case) concern; "guaranteed within budget, every single time" is what real-time correctness actually demands, and those are genuinely different bars.

### 3. Mathematical foundation

Same FIR math as Lesson 4: $y[n] = \sum_{k=0}^{M-1} h[k]\,x[n-k]$. The new engineering question isn't the math — it's *how* to maintain the $M$ most recent samples $x[n-M+1],\dots,x[n]$ using $O(M)$ fixed memory as new samples continuously arrive, which is exactly what an $M$-slot ring buffer provides.

### 4. Formal definition

A **ring buffer** is a fixed-size array of length $M$ with a write index that advances modulo $M$; a new sample overwrites the oldest one still present, giving $O(1)$ insertion and $O(M)$ access to the current window, without ever shifting or copying existing data. An operation is **real-time safe** (deterministic latency) if its worst-case execution time is bounded and known in advance — array indexing and arithmetic qualify; heap allocation does not, because its worst case is unbounded in practice.

### 5. Why the naive approaches fail — reasoned, not just asserted

Two naive streaming implementations both fail, for different reasons. **Shift-the-whole-buffer**: on every new sample, shift all $M$ stored values over by one position, insert the new one at the end. This costs $O(M)$ *per sample* just for the shift, on top of the $O(M)$ filter computation itself — wasteful, though not incorrect. **Allocate-a-fresh-window-every-sample**: something like `std::vector<float> window = get_last_M_samples();` inside the per-sample loop allocates on the heap *every single call*. At a typical baseband sample rate of tens of millions of samples per second, that's tens of millions of heap allocations per second, each with the unpredictable worst-case latency from §2 — not a minor inefficiency, a fundamental incompatibility with a hard deadline. A ring buffer eliminates both problems at once: no shifting (overwrite one slot, advance an index — $O(1)$), and no allocation (the buffer is allocated exactly once, at startup, and reused forever).

### 6. Visualization

```
Ring buffer, M=4, after inserting samples 0,1,2,3,4,5,6 in order:

slot:    [0]  [1]  [2]  [3]
after 4: x0   x1   x2   x3      write_idx -> 0 (wraps)
after 5: x4   x1   x2   x3      write_idx -> 1  (x4 overwrote x0)
after 6: x4   x5   x2   x3      write_idx -> 2  (x5 overwrote x1)
                                  ↑ the 4 most recent samples (x3,x4,x5,x6... wait x6 not yet)
Reading "most recent M, in order" always means reading starting from write_idx, wrapping around.
```

### 7. Worked example

$M=4$. Insert samples 0–5: after sample 3, physical slots hold $[x_0,x_1,x_2,x_3]$, write index wraps to 0. Sample 4 overwrites slot 0 → $[x_4,x_1,x_2,x_3]$. Sample 5 overwrites slot 1 → $[x_4,x_5,x_2,x_3]$. The four most recent samples, in true chronological order, are $x_2,x_3,x_4,x_5$ — reading them correctly means starting from the current write index (slot 2) and wrapping around, **not** just reading the physical array left to right.

### 8. Code

```cpp
#include <vector>
#include <cstddef>

template <typename T>
class RingBuffer {
public:
    explicit RingBuffer(size_t size) : buf_(size, T{}), size_(size), write_idx_(0) {}

    // O(1), no allocation -- the entire point of this class
    void push(T sample) {
        buf_[write_idx_] = sample;
        write_idx_ = (write_idx_ + 1) % size_;
    }

    // sample_back(0) = most recent, sample_back(1) = one before that, ...
    T sample_back(size_t k) const {
        size_t idx = (write_idx_ + size_ - 1 - k) % size_;
        return buf_[idx];
    }

    size_t size() const { return size_; }

private:
    std::vector<T> buf_;   // allocated ONCE, at construction -- never resized after
    size_t size_;
    size_t write_idx_;
};

class StreamingFIR {
public:
    explicit StreamingFIR(std::vector<float> taps)
        : taps_(std::move(taps)), history_(taps_.size()) {}

    // Real-time safe: only indexing and arithmetic, zero allocation, every call
    float process_sample(float x) {
        history_.push(x);
        float y = 0.0f;
        for (size_t k = 0; k < taps_.size(); ++k) {
            // h[0] pairs with the NEWEST sample (x[n]), per Lesson 4's y[n] = sum h[k]x[n-k]
            y += taps_[k] * history_.sample_back(k);
        }
        return y;
    }

    void process_block(const float* in, float* out, size_t n) {
        for (size_t i = 0; i < n; ++i) out[i] = process_sample(in[i]);
    }

private:
    std::vector<float> taps_;
    RingBuffer<float> history_;
};
```

### 9. Experiment

Feed a long, arbitrary test signal through `StreamingFIR::process_block` and compare, sample-for-sample, against a direct batch convolution of the same signal with the same taps (Lesson 4's reference) — they should match to floating-point precision. Then verify the "zero allocation in the hot path" claim empirically rather than just trusting the code: override `operator new`/`operator delete` globally with a counter (or use a tool like Valgrind/ASan's allocation tracking) around the `process_block` call specifically, and confirm the count is exactly zero during processing — only the constructor's initial `std::vector` allocation should ever show up, once, before the loop starts.

### 10. Expected output

Streaming output matches the batch-convolution reference exactly (within floating-point rounding). The allocation counter shows zero calls during the processing loop itself — a claim turned into a measured fact, matching this curriculum's running emphasis on verifying rather than asserting.

### 11. Debugging exercise

Symptom, no cause given: your `StreamingFIR`'s output is close to, but consistently different from, the Lesson 4 batch-convolution reference — not garbage, but as if the filter taps were subtly reversed, or the output were shifted by a small number of samples.

Go back to Lesson 4 §3's exact formula: $y[n] = \sum_{k=0}^{M-1} h[k]\,x[n-k]$. Which tap index should pair with the *newest* sample, and which with the *oldest*? Does §8's code actually implement that pairing, or its reverse?

**Root cause (revealed):** a **ring-buffer read-direction bug** — pairing `taps_[k]` with the wrong end of the history window. Per Lesson 4's formula, $h[0]$ multiplies $x[n]$ (the newest sample), and $h[M-1]$ multiplies $x[n-M+1]$ (the oldest) — exactly what `sample_back(k)` in §8 is defined to return for index `k`. Get this backward (e.g., iterate oldest-to-newest against `taps_[0..M-1]`) and you've implemented a subtly different, incorrect operation that happens to look almost right for symmetric filters and clearly wrong for asymmetric ones. This is one of the most common real bugs when first writing a ring-buffer-based filter, precisely because both directions "compile and mostly run."

### 12. Common mistakes

- The read-direction bug from §11 — extremely common on a first implementation.
- Off-by-one errors in the wraparound modulo arithmetic — a classic, easy-to-get-subtly-wrong category of bug whenever a write index is exactly at the boundary; test explicitly at the wraparound point, not just with an arbitrary large sample count that happens to avoid it.
- Correctly pre-sizing a ring buffer's storage once in the constructor, but then accidentally reintroducing the exact problem this lesson solves by later using `push_back()` on the underlying `std::vector` somewhere else in the code instead of writing to a fixed index — a subtle way to silently undo everything this lesson built, while believing the allocation problem was already solved.

### 13. Real RF/cellular application

Every real-time SDR/modem receive chain is built around exactly this pattern: fixed-size ring buffers moving samples between hardware capture (interrupt/DMA-driven, Lesson 15/16's territory) and processing threads, and between successive stages of a DSP pipeline. This is conceptually the same buffering abstraction underlying GNU Radio's block scheduler (Lesson 16 §4) and real SDR driver APIs (UHD, SoapySDR) — ring buffers are the fundamental building block connecting producer and consumer running at potentially uneven rates, and this lesson is the direct foundation for Project 9 (Real-Time SDR Processing Pipeline).

### 14. Interview question

*"Why is a plain `std::vector` with `push_back()` generally unsuitable for a hard real-time hot path, even though it usually performs fine?"* Answer from §2: `push_back()` can trigger a reallocation (copying all existing contents into new, larger storage) whenever capacity is exceeded — a genuinely unpredictable, much-larger-than-typical worst-case latency. Even though this is rare (amortized $O(1)$ on average), a hard real-time system must guarantee *every single call* meets its deadline, not just most calls on average — the average-case-vs-worst-case-guarantee distinction is the entire answer.

### 15. Active recall

1. Why does a ring buffer give $O(1)$ insertion without ever shifting existing data?
2. Precisely why is heap allocation incompatible with a hard real-time deadline, even when it's usually fast?
3. In §8's code, which tap index pairs with the newest sample, and why does that follow directly from Lesson 4's formula?

### 16. Mini assignment

Implement the allocation counter from §9 (global `operator new`/`delete` override, or an equivalent tool-based check) and confirm empirically that processing 10 million samples through `StreamingFIR::process_block` triggers exactly the allocations from construction and none during processing — report the actual measured count, not an assumption.

### 17. Exit test

1. Explain, precisely, why `sample_back(0)` must return the *most recently pushed* sample for §8's code to be correct.
2. True or false, with a one-line reason: "Amortized O(1) push_back performance is sufficient for a hard real-time audio/SDR processing callback."
3. Your reasoning from §11 about tap-pairing direction, if you haven't sent it.

---

**Next:** Lesson 24 moves to **multithreading and lock-free structures**: what happens once your ring buffer needs to safely pass samples between a capture thread and a processing thread running concurrently — where "correct in a single-threaded test" and "correct under real concurrent access" turn out to be very different claims.

---

[← Lesson 22](lesson-22-cpp-memory-layout.md) · [Table of Contents](../README.md) · [Lesson 24 →](lesson-24-multithreading-lockfree.md)