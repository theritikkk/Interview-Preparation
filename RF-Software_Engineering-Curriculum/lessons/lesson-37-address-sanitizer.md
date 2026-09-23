[← Table of Contents](../README.md) · [← Lesson 36](lesson-36-latency-throughput.md)

---

# Lesson 37 — Memory-Safety Tooling: AddressSanitizer and Why "It Didn't Crash" Proves Nothing

*Stage 12, continued. Lesson 24 showed a data race can pass every test and still be wrong. This lesson shows the exact same thing is true of memory bugs — and the fix is the same philosophy as ThreadSanitizer, applied to a different kind of access.*

### 1. Intuition

Lesson 24 established that "it worked in every test" is not evidence of concurrency correctness — a data race can hide for thousands of runs. The identical principle holds for memory safety: a buffer overflow that writes one element past an array's end, or a use-after-free that touches memory just after it's been freed, can run "correctly" indefinitely — the corrupted byte simply doesn't happen to matter yet — until a different allocator, a different build, or a different data pattern turns invisible corruption into a crash, or worse, silently wrong output. This lesson is about tools that catch these violations deterministically, at the exact instant they happen, rather than waiting for luck to run out.

### 2. The engineering problem that forces this

C++ gives you manual memory control — Lesson 22's entire premise — and with it, complete responsibility for never reading or writing outside an allocation's bounds, never touching freed memory, never leaking what should be released. None of these violations are guaranteed to crash immediately or visibly: this is undefined behavior in exactly Lesson 24's sense, now for memory instead of concurrent access, meaning the runtime is free to do anything, including "appear to work perfectly" for an arbitrarily long time. You need tooling that checks every memory access against ground truth at the moment it happens, not tooling that waits for a downstream symptom that may never surface in your particular test run.

### 3. Mathematical foundation

Not equations this time — the mechanism worth understanding precisely is **redzones and shadow memory**. AddressSanitizer (ASan) surrounds every heap, stack, and global allocation with a "redzone" of poisoned memory, and maintains a compact "shadow memory" table tracking which bytes are currently valid to access. Every load and store the compiler emits gets an inserted check against this shadow table.

### 4. Formal definition

**Buffer overflow**: a read or write outside an allocation's valid bounds. **Use-after-free**: accessing memory after it has been freed, before it's safe to consider reused. **Double-free**: freeing the same allocation twice, corrupting allocator metadata. **AddressSanitizer** (`-fsanitize=address`): compiler instrumentation catching all of these at the exact faulting access, with the allocation site and access site both reported as full stack traces. **Valgrind/memcheck**: a binary-instrumentation alternative needing no recompilation, at substantially higher runtime cost — useful when source isn't available or ASan isn't an option.

### 5. Derivation — why these tools catch what pure observation cannot

Consider a one-element overflow: writing to `array[N]` when valid indices are `0..N-1`. Without instrumentation, that write lands wherever the allocator happened to place adjacent memory — often allocator bookkeeping or unused padding that nothing reads again in *this particular run*. The program appears completely correct. Change the allocator, change an unrelated allocation size elsewhere that shifts heap layout, or just run long enough, and that same unchanged write can corrupt something that genuinely does get read later — producing a bug that looks "new" even though the faulty write has been there the entire time, unchanged.

ASan's redzone technique sidesteps this fundamental problem entirely: it does not rely on the corrupted byte ever mattering to the program's own logic to catch the bug. Because redzones are inserted immediately adjacent to *every* allocation by construction, `array[N]` necessarily lands inside one — and the shadow-memory check flags this specific access as invalid **at the exact moment it happens**, unconditionally, regardless of whether the corruption would ever have produced an observable symptom on its own. This is precisely why ASan catches bugs pure testing cannot: it checks the *access itself* against ground truth every single time, rather than waiting for a downstream consequence to surface — the identical underlying philosophy as Lesson 24's ThreadSanitizer, which checks an access pattern against the happens-before contract directly rather than waiting for an observable race condition to actually corrupt something visibly.

### 6. Visualization

```
Unprotected allocation:                    ASan-instrumented allocation:
[array: N elements][adjacent memory]       [redzone][array: N elements][redzone]
        write to array[N] here →                    write to array[N] here →
        lands in "adjacent memory,"                  lands in REDZONE,
        may go unnoticed for a long time              flagged IMMEDIATELY, every time
```

### 7. Worked example

A one-line off-by-one bug — `for (int i = 0; i <= N; ++i) arr[i] = 0;` (note `<=`, should be `<`) — writes exactly one element past `arr`'s allocation on the final iteration. Under ASan, this produces an immediate report naming the exact faulting line, the exact byte offset past the allocation, and the full stack trace of where `arr` was originally allocated — turning "somewhere, sometime, this array got corrupted" into "this exact line, right now."

### 8. Code

```cpp
#include <cstdlib>
#include <cstdio>

void overflow_bug() {
    int* arr = new int[10];
    for (int i = 0; i <= 10; ++i) {   // BUG: should be i < 10
        arr[i] = i;
    }
    printf("sum: %d\n", arr[0] + arr[9]);
    delete[] arr;
}

void use_after_free_bug() {
    int* p = new int(42);
    delete p;
    printf("value: %d\n", *p);        // BUG: p was already freed
}

int main() {
    overflow_bug();
    // use_after_free_bug();          // try one bug at a time for a clean report
    return 0;
}
```

```bash
# Compile with ASan and debug symbols (Sec 12's requirement for a useful report):
g++ -fsanitize=address -g -O1 memory_bugs.cpp -o memory_bugs
./memory_bugs
```

### 9. Experiment

Compile and run §8's code **without** any sanitizer flag, many times in a row — note whether it crashes, and how often, if at all. Then compile with `-fsanitize=address -g` and run once: you should get an immediate, precise, deterministic report at the exact faulting line, every single time, no exceptions. Repeat for `use_after_free_bug()` separately.

### 10. Expected output

Unprotected: the program very likely runs to completion silently on most or all attempts, possibly printing a plausible-looking (if technically undefined) result — direct, empirical confirmation of §5's "silent most of the time" claim, not just an assertion to trust. Under ASan: a detailed, immediate crash report naming the overflow's exact location and the original allocation's stack trace, on every single run without exception.

### 11. Debugging exercise

Symptom, no cause given: your program segfaults, but the crash occurs inside a completely unrelated function, far from anything you recently changed — and the stack trace at the crash point doesn't obviously implicate any specific bug in that function itself.

Using only §5's reasoning: why might "where the program crashed" and "where the actual bug is" be two completely different places for this class of problem, unlike most other bug categories?

**Root cause (revealed):** classic heap corruption. Some **earlier** buffer overflow or use-after-free silently corrupted allocator metadata or unrelated heap data — the crash you're observing is simply wherever that corruption first got read in a way that broke something, which can be arbitrarily far away in the code and arbitrarily much later in time from the actual faulty write. This is exactly why "the crash location" is frequently *not* "the bug location" for memory corruption specifically — and exactly the class of bug ASan is uniquely suited to catch at its true source, since it flags the original faulty access immediately rather than waiting for corrupted data to eventually be read somewhere unrelated.

### 12. Common mistakes

- Running ASan-instrumented builds in production — substantial memory and runtime overhead make this appropriate for testing/CI, not typically for a shipped real-time product.
- Concluding "my program doesn't crash" means "my program has no memory bugs" — precisely §5's derived point, and a common, genuinely dangerous inference.
- Forgetting debug symbols (`-g`) — without them, ASan's report gives raw addresses instead of real source lines and readable stack traces, dramatically reducing its practical usefulness.
- Reaching for ASan only *after* a mysterious crash already occurred in an unprotected build, rather than running the full test suite under it routinely in CI — it should be standard, automated practice (directly extending Lesson 35's automated-regression-testing theme), not an emergency-only tool.

### 13. Real RF/cellular application

Real-time PHY/embedded C++ codebases — exactly Lessons 22–27's domain — are full of manual buffer management, ring buffers, and raw pointers into DMA-mapped memory: genuinely high-risk code for precisely these bug classes, and exactly the kind of code where invisible corruption is easiest to write and hardest to catch by observation alone. Running the full test suite, including Lesson 35's hardware-in-the-loop tests, under ASan/Valgrind routinely in CI is standard, expected practice on any serious embedded/PHY software team.

### 14. Interview question

*"Why can a C++ program with a genuine buffer overflow run correctly for a long time before eventually crashing or producing wrong output?"* Answer from §5: the overflow's write lands in whatever memory happens to be adjacent at that moment — often unused padding never read again in that run — so the corruption has no observable consequence until allocator layout, data patterns, or timing eventually cause that specific corrupted byte to be read somewhere that matters. The bug was always present; only its visible symptom was delayed and layout-dependent.

### 15. Active recall

1. How does ASan catch a buffer overflow regardless of whether the corrupted memory would ever have been read by the program's own logic?
2. Why can a memory-corruption crash occur far from the actual bug, in both code location and time?
3. Why does "my program hasn't crashed" provide weak evidence of memory safety specifically, more so than for many other bug classes?

### 16. Mini assignment

Run §8's two bugs, unprotected, 20 times each, and record how often (if ever) each visibly misbehaves. Then run both under ASan and confirm 100% deterministic, immediate, precise detection on every single run — turning §5's claim into your own measured data, the same empirical discipline this curriculum has applied throughout.

### 17. Exit test

1. Explain, in your own words, what a redzone is and why placing one around every allocation is sufficient to catch an out-of-bounds access unconditionally.
2. True or false, with a one-line reason: "A program that passes all its tests without crashing is very likely free of memory-safety bugs."
3. Your reasoning from §11 about why crash location and bug location can diverge for memory corruption, if you haven't sent it.

---

**Next:** Lesson 38 continues Stage 12 with **real protocol-trace field-failure diagnosis** — taking the modem-diagnostic-tooling skill (QXDM/QCAT-style trace analysis) this curriculum's career map has flagged repeatedly since Part 1, and finally practicing it directly on a realistic multi-layer trace, tying PHY, MAC, RLC, and RRC events together the way Project 10's Cellular Protocol Analyzer actually requires.

---

[← Lesson 36](lesson-36-latency-throughput.md) · [Table of Contents](../README.md) · [Lesson 38 →](lesson-38-multilayer-trace-analysis.md)