[← Table of Contents](../README.md) · [← Lesson 23](lesson-23-cpp-fir-ring-buffer.md)

---

# Lesson 24 — Multithreading and Lock-Free Structures: Passing Samples Between Threads Safely

*Stage 9/10, continued. Lesson 23's ring buffer is correct — for exactly one thread. The moment a second thread touches it concurrently, "correct in every test you ran" and "actually correct" become genuinely different claims, for reasons that have nothing to do with your algorithm.*

### 1. Intuition

Lesson 23's ring buffer works perfectly as long as only one thread ever touches it. Real systems split work across threads deliberately — a hardware-capture thread (often running in an interrupt or DMA-callback context) and a separate DSP-processing thread are a completely standard split. The moment two threads touch the *same* ring buffer concurrently — one writing, one reading — something genuinely new can go wrong, and it isn't a logic error in the usual sense. It's that the compiler and CPU are explicitly *permitted* to reorder and cache memory operations in ways that are invisible and harmless in a single thread, but that a second thread can observe directly, breaking assumptions Lesson 23 never had to state out loud because a single thread can never violate them on its own.

### 2. The engineering problem that forces this

You need a capture thread (producer) to safely hand samples to a processing thread (consumer) running concurrently, without a **mutex/lock** — which can force the producer, often running in a real-time-critical context, to *block* waiting for the consumer, a form of priority inversion directly opposed to Lesson 23's whole determinism argument — and without a **data race**, which in C++ is not "a bug that sometimes gives wrong output" but formally **undefined behavior**, meaning the compiler is permitted to assume it never happens and optimize on that assumption. The standard real-time answer is a **single-producer single-consumer (SPSC) lock-free ring buffer**, built on atomics instead of locks.

### 3. Mathematical foundation

Not math this time — the foundation is the C++ **memory model**. An **atomic operation** is indivisible: no other thread can observe it half-completed. **Memory ordering** governs whether operations can be *reordered* relative to each other from another thread's point of view — critically, atomicity alone (`memory_order_relaxed`) says nothing about ordering; the specific pairing this lesson needs is **release** (producer) and **acquire** (consumer).

### 4. Formal definition

A **data race**: two threads access the same memory location, at least one access is a write, with no synchronization between them — formally undefined behavior in C++, distinct from an informal "race condition." A **release store** followed by a matching **acquire load** (of the *same* atomic variable, observing the value the release wrote) establishes a **happens-before** relationship: the C++ standard *guarantees* that every memory write the producer performed before its release-store is visible to the consumer after its acquire-load — not "probably visible," a provable guarantee the standard makes to you as a contract.

### 5. Derivation — why the naive version breaks, and why release/acquire specifically fixes it

A naive shared ring buffer does, in the producer: `buf[idx] = sample; idx = (idx+1) % size;` — two separate, ordinary (non-atomic) writes. Two distinct problems follow. **Reordering**: the compiler is free to reorder these two writes if doing so doesn't change *this thread's own* observable behavior — but from the *consumer's* perspective, if it sees the updated `idx` before it sees the corresponding write to `buf[idx]` (whether from actual instruction reordering, compiler optimization, or CPU store-buffer/cache effects), it reads stale or garbage data at that slot. This is explicitly *permitted* behavior for unsynchronized memory access under the C++ standard — not a rare hardware quirk, but a documented gap in what the language promises you. **Non-atomicity**: without `std::atomic`, nothing guarantees `idx` itself can't be observed mid-update.

The fix is precise, not just "add some synchronization somewhere": make `write_idx` an `std::atomic<size_t>`, have the producer perform its store to `write_idx` with `memory_order_release` — **after** writing the actual sample — and have the consumer perform its load of `write_idx` with `memory_order_acquire` — **before** reading the sample. This specific pairing is what creates the happens-before edge from §4: if the consumer's acquire-load reads the exact value the producer's release-store wrote, the standard guarantees the sample write (which happened-before the release, in program order) is visible to the consumer (which reads it after the acquire). `memory_order_relaxed` gives you atomicity of the index update alone, with *no* such cross-thread visibility guarantee for anything else — which is exactly the gap that produces the intermittent corruption in §11.

### 6. Visualization

```
Producer thread                         Consumer thread
--------------                          ---------------
buf[idx] = sample;        ─┐
write_idx.store(           │  "happens-before" edge
  idx, memory_order_release)│  (guaranteed by the standard,
                            │   only because release pairs
                            └─►  with a matching acquire)
                                 read_idx = write_idx.load(memory_order_acquire);
                                 x = buf[read_idx];   ← guaranteed to see the real sample,
                                                          not stale/garbage data
```

### 7. Worked example

Without proper ordering: the producer writes `buf[5] = 3.14` then updates `idx` to 6; due to reordering or caching effects, the consumer's thread observes `idx == 6` *before* the write of `3.14` becomes visible to it, and reads whatever stale value happened to be sitting in `buf[5]` from a previous cycle. With correct release/acquire: the consumer observing `idx == 6` (via the acquire-load) is, by the standard's own guarantee, *also* guaranteed to observe `buf[5] == 3.14` — the ordering isn't probabilistic, it's a contract.

### 8. Code

```cpp
#include <atomic>
#include <vector>
#include <cstddef>

template <typename T>
class SpscRingBuffer {
public:
    explicit SpscRingBuffer(size_t capacity)
        : buf_(capacity), capacity_(capacity), write_idx_(0), read_idx_(0) {}

    // Called ONLY from the producer thread
    bool push(const T& value) {
        size_t w = write_idx_.load(std::memory_order_relaxed);
        size_t next = (w + 1) % capacity_;
        if (next == read_idx_.load(std::memory_order_acquire)) {
            return false;  // buffer full
        }
        buf_[w] = value;                                     // write the data FIRST
        write_idx_.store(next, std::memory_order_release);    // THEN publish it
        return true;
    }

    // Called ONLY from the consumer thread
    bool pop(T& out) {
        size_t r = read_idx_.load(std::memory_order_relaxed);
        if (r == write_idx_.load(std::memory_order_acquire)) {
            return false;  // buffer empty
        }
        out = buf_[r];                                        // read the data FIRST
        read_idx_.store((r + 1) % capacity_, std::memory_order_release);
        return true;
    }

private:
    std::vector<T> buf_;
    size_t capacity_;
    std::atomic<size_t> write_idx_;
    std::atomic<size_t> read_idx_;
};
```

### 9. Experiment

Build a test harness: one thread pushes a long, known sequence of sequential integers (0, 1, 2, 3, ...); another thread pops them and verifies the sequence arrives complete and in order, with no gaps, repeats, or corrupted values. Run it first against a **naive** shared ring buffer using plain (non-atomic) indices — it may well *appear* to pass repeatedly on your specific machine, compiler, and optimization level, which is itself the important, unsettling lesson: absence of an observed failure is not proof of correctness for a genuine data race. Then compile and run both versions under **ThreadSanitizer** (`-fsanitize=thread`) — the real, principled tool for this, rather than trusting casual observation.

### 10. Expected output

The naive version may or may not show visible corruption in an ordinary run — that's expected and is the point, not a contradiction. ThreadSanitizer should flag a genuine, unambiguous data race on the naive version regardless of whether corruption happened to appear that run. The `SpscRingBuffer` version should pass both the sequence-integrity check and run clean under ThreadSanitizer, consistently.

### 11. Debugging exercise

Symptom, no cause given: your SPSC ring buffer passes every single-threaded test and appears to work correctly in nearly all multithreaded runs — but very rarely, maybe once in 100,000 runs under heavy load, the consumer reads a value at some position that was never actually written there by the producer.

What specific detail in §8's implementation, if gotten subtly wrong, would produce a failure this rare and this hard to reproduce — rather than a failure that shows up every time?

**Root cause (revealed):** almost certainly **incorrect memory ordering** — using `memory_order_relaxed` for the `write_idx`/`read_idx` store or load instead of the required release/acquire pairing. Relaxed ordering still guarantees the index update itself is atomic, but provides *no* happens-before guarantee for the actual sample data — meaning the visibility bug from §5 becomes possible again, just rare, because most of the time, on most hardware, under most optimization levels, things happen to line up anyway. This rarity is precisely what makes concurrency bugs uniquely dangerous compared to every other bug category in this curriculum: a logic bug fails consistently and is easy to catch in testing; a missing happens-before edge can pass thousands of test runs and only fail in production, under load, on a different machine.

### 12. Common mistakes

- Using `volatile` instead of `std::atomic` for cross-thread communication — a very common and completely incorrect substitution. `volatile` in C++ only prevents the compiler from caching/eliding repeated accesses to that one variable (its real purpose is memory-mapped hardware registers); it provides **zero** cross-thread synchronization or ordering guarantees, and solves an essentially unrelated problem.
- Using `memory_order_relaxed` everywhere "because it's faster" without understanding that it removes exactly the ordering guarantee this entire design depends on — the precise gap exploited in §11.
- Assuming an SPSC design generalizes trivially to multiple producers or multiple consumers — SPSC's simplicity specifically relies on exactly one writer and one reader per index; safely supporting multiple producers or consumers requires substantially more complex designs (and often isn't fully lock-free in the strict sense), a genuine scope boundary worth knowing rather than assuming away.

### 13. Real RF/cellular application

This exact SPSC lock-free pattern is the standard mechanism connecting a hardware-driven capture/interrupt/DMA context to a DSP processing thread in real SDR and modem software, and underlies high-performance kernel-bypass frameworks like DPDK — the same DPDK flagged in this curriculum's career-map research on Nokia/ORAN L1/L2 postings. Verifying this kind of code with ThreadSanitizer rather than casual observation is a genuine, real differentiator between engineers who "usually get away with it" and those who build something provably correct for production modem/RAN software — this lesson is a direct building block for Project 9 (Real-Time SDR Processing Pipeline).

### 14. Interview question

*"Why is a data race called undefined behavior in C++, rather than just 'a bug that sometimes produces the wrong answer'?"* Answer from §4: a program containing a genuine data race has no defined semantics at all under the standard — the compiler is explicitly permitted to assume data races never occur and optimize accordingly, so the actual resulting behavior can be far stranger than "occasionally stale data." This is exactly why "it worked when I tested it" provides no real guarantee, and why correct atomics and memory ordering — verified with tools like ThreadSanitizer, not by observation — are the only real path to correctness.

### 15. Active recall

1. What specific guarantee does a release-store paired with a matching acquire-load provide that plain atomicity alone does not?
2. Why can a naive shared ring buffer appear to work correctly in testing while still containing a genuine, serious bug?
3. Why is `volatile` not a substitute for `std::atomic` in multithreaded code?

### 16. Mini assignment

Build the sequence-integrity test harness from §9 for both the naive and `SpscRingBuffer` implementations, run both under ThreadSanitizer, and record the actual diagnostic output for each — turning "atomics fix this" from an assertion in this lesson into something you've personally verified with a real tool.

### 17. Exit test

1. Write, precisely, which memory order the producer's index store and the consumer's index load must use, and why that specific pairing (not `relaxed`, not `seq_cst` only) is what's necessary here.
2. True or false, with a one-line reason: "If a data race hasn't caused an observed problem after many test runs, the code is very likely free of data races."
3. Your reasoning from §11 about why this class of bug is rare and hard to reproduce, if you haven't sent it.

---

**Next:** Lesson 25 turns to **fixed-point arithmetic**: what happens when your DSP code has to run on hardware with no floating-point unit at all, and why the scaling decisions you make there can silently reproduce bugs that look exactly like the ones in this lesson, for a completely different underlying reason.

---

[← Lesson 23](lesson-23-cpp-fir-ring-buffer.md) · [Table of Contents](../README.md) · [Lesson 25 →](lesson-25-fixed-point-arithmetic.md)
