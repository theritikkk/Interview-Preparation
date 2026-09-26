# C

Your deepest, most evidenced language — NetSight is ~9,700 lines of C. Interviewers will treat C questions as project-verification questions, not abstract trivia. Expect them to probe whether you actually understand what your own ring buffer and Treiber stack are doing at the memory level.

---

## Cluster 1: Memory Model — Stack, Heap, Pointers, Undefined Behavior

### What is it?
C gives you two allocation regions with opposite lifetime rules: the **stack** (automatic, LIFO, freed when a function returns) and the **heap** (manual, freed only when you call `free()`). A pointer is just an address; C does nothing to stop you from using it wrong.

### Why does it exist?
No garbage collector, no runtime safety net — C trades safety for control and predictability, which is exactly why it's still the language for packet capture at line rate (NetSight) where GC pauses would be unacceptable.

### How does it work?
```c
void bad_example(void) {
    int *ptr;
    { int local = 42; ptr = &local; }   // local's stack frame is gone
    printf("%d", *ptr);                  // undefined behavior: dangling pointer
}

int *good_example(void) {
    int *ptr = malloc(sizeof(int));      // heap: lives until free()
    if (!ptr) return NULL;               // ALWAYS check malloc
    *ptr = 42;
    return ptr;                          // caller now owns it, must free()
}
```
Undefined behavior (UB) means the compiler is allowed to assume it never happens — and will optimize around that assumption, which is why UB bugs can vanish in debug builds and reappear in release builds.

### Real-world use / Resume Connection
NetSight's ring buffer and memory pool exist specifically to avoid `malloc`/`free` churn in the packet-processing hot path — heap allocation under load is slow and fragments memory, which is incompatible with your stated <0.2% memory growth over a 60-minute endurance test.

### Interview Questions
- **L1 (30s):** "Stack is automatic and fast, freed on scope exit. Heap is manual, freed by `free()`, and used when data must outlive the function or is too large for the stack."
- **L2 (2 min):** add: what a dangling pointer is, what a use-after-free is, why `malloc` can return `NULL`, and that C gives you no automatic bounds checking — that's your job.
- **L3 (deep):** be ready to explain why your endurance test measuring "<0.2% memory growth" is evidence *against* a slow leak specifically — a leak this small over 60 minutes could still be a real (if tiny) leak; be precise about what the test does and doesn't prove.

### Follow-up Chain
> **Q: How do you know NetSight doesn't leak memory?**
> A: We ran Valgrind leak checks in the test suite and a 60-minute endurance test showing under 0.2% memory growth.
> **Follow-up: 0.2% growth isn't 0% — how do you know that's not a slow leak?**
> A: **Verified against actual test data (`VALIDATION.md`):** RSS went from 316.16 MB at T0 to 316.66 MB at T60 (+500kB, 0.16% variance) — but critically, growth **plateaued** between T30 and T60 (+60kB in the final 30 minutes, versus +440kB in the first 30). A plateau shape is real evidence against a continuous leak — a genuine slow leak would keep growing roughly linearly across the full hour, not flatten out. This is a fully verified, quotable answer, not a hedge.
> **Trap: "Valgrind confirms there's no leak."**
> Correct response: Valgrind confirms no leaks **in the code paths your test suite actually exercised**. It cannot prove the absence of leaks in untested paths. Say this precisely — overclaiming what a static/dynamic analysis tool proves is a classic "sounds confident, is wrong" answer.

### Common Mistakes
- Confusing "no crash" with "no undefined behavior" (UB can be silently wrong, not just crash)
- Forgetting to check `malloc` return value
- Not distinguishing a memory leak (never freed) from a dangling pointer (freed too early, still used)

### DO NOT SAY THIS
"C has garbage collection for local variables." — No; stack variables are reclaimed automatically because of scope, which is a compile-time mechanism, not GC. Don't blur the two.

### Comparison With Alternatives
C++ adds RAII/smart pointers to automate exactly this lifetime management (see `CPP.md`) — a natural "why not use C++ instead" follow-up for NetSight, worth having a real answer to (control over layout, no exception-handling overhead in the hot path, simpler ABI for a project built as "no locks, no blocking" from scratch).

### Coding/Debugging Exercise
Given a function that returns a pointer to a stack-local `struct`, find the bug and fix it two ways (heap allocation vs. pass-by-output-parameter).

### Quick Revision
- Stack: automatic, fast, scope-bound. Heap: manual, `malloc`/`free`, programmer-owned lifetime.
- UB ≠ crash. UB = compiler is allowed to assume it can't happen.
- Always check `malloc`'s return value.

---

## Cluster 2: Concurrency & Lock-Free Programming (C11 Atomics)

### What is it?
Lock-free programming uses atomic hardware instructions (compare-and-swap, fetch-and-add) instead of mutexes to coordinate threads, so no thread ever blocks waiting for another to release a lock.

### Why does it exist?
Mutexes serialize access — under contention, that becomes a bottleneck and a source of priority inversion / latency spikes. NetSight's stated goal ("process traffic without locks or blocking") is precisely to avoid this in a packet-processing path where a stalled thread means dropped packets.

### How does it work?
```c
#include <stdatomic.h>

typedef struct node { void *data; struct node *next; } node_t;
_Atomic(node_t*) top = NULL;

void push(node_t *n) {
    node_t *old_top;
    do {
        old_top = atomic_load(&top);
        n->next = old_top;
    } while (!atomic_compare_exchange_weak(&top, &old_top, n));
}
```
This is a **Treiber stack** — exactly the structure your resume names as your memory pool's implementation. The core idea: read the current state, prepare a change, then atomically swap only if nothing else changed it first; retry if it did.

### Real-world use / Resume Connection
🔴 This is the single most likely deep-dive area for NetSight. You claim: a C11-atomic ring buffer, a Treiber-stack memory pool, and a 1M-entry hash-based flow table, "to process traffic without locks or blocking."

### Interview Questions
- **L1:** "Lock-free code uses atomic CPU instructions like compare-and-swap instead of mutexes, so threads never block each other."
- **L2:** add memory ordering — `memory_order_seq_cst` vs. `memory_order_acquire/release` vs. `memory_order_relaxed`, and why weaker orderings are faster but easier to get wrong.
- **L3:** explain the **ABA problem** precisely: thread A reads value A from a location, gets preempted, another thread changes A→B→A, and when A resumes, its CAS succeeds even though the world changed underneath it. **Verified against your actual NetSight code:** `mempool.c`'s free-list push/pop is a plain `atomic_compare_exchange` on a raw pointer — no tagged pointers, no generation counter, no hazard pointers. The honest, correct answer if asked is that there is **no explicit ABA mitigation** in this implementation, with the defensible justification that it's a bounded, internal-only pool (blocks are recycled solely within this system, never exposed externally), which narrows the practical risk window without formally eliminating it. See `13_PROJECT_DEEP_DIVES/NETSIGHT.md` for the full framing — do not claim tagged pointers or hazard pointers in an interview, since the code doesn't have them.

### Follow-up Chain
> **Q: Why lock-free instead of a mutex here?**
> A: A mutex means one thread blocks while another holds the lock — under high packet rates, that's latency and dropped packets. Lock-free uses CAS retries instead of blocking.
> **Follow-up: Doesn't a CAS retry loop also waste CPU under contention — so what did you actually gain?**
> A: You gain *no thread ever holds a lock across a preemption*, avoiding priority inversion and unbounded blocking; a spin-retry is bounded and typically very short compared to OS-level lock/unlock overhead and context switches.
> **Trap: "Lock-free means wait-free — no thread ever waits."**
> Correct response: No — lock-free only guarantees *some* thread makes progress system-wide (no permanent deadlock), not that every individual thread makes progress every step. Wait-free is the strictly stronger, rarer guarantee. Don't conflate the two terms in an interview; this is a very common and easily-checked mistake.

### Common Mistakes
- Saying "atomic" when you mean "thread-safe" generally (atomics only guarantee atomicity of the single operation, not of a multi-step sequence)
- Confusing lock-free with wait-free (see trap above)
- Not knowing what memory ordering your own `atomic_compare_exchange_weak` calls use by default (`memory_order_seq_cst` if unspecified)

### Comparison With Alternatives
Mutex + condition variable: simpler to reason about, but blocking. Lock-free: harder to write correctly (ABA, memory ordering), but no blocking. A hybrid (spinlock for very short critical sections) is a middle ground worth mentioning if asked for alternatives.

### Coding/Debugging Exercise
Implement a lock-free single-producer single-consumer (SPSC) ring buffer using `_Atomic size_t` head/tail indices, then explain why SPSC is easier to get right than multi-producer multi-consumer (MPMC).

### Quick Revision
- CAS = compare-and-swap = the building block of lock-free structures.
- Lock-free ≠ wait-free.
- ABA problem: value changes and changes back — CAS can't detect this without extra help (tags/hazard pointers).
- Memory ordering controls what reorderings the compiler/CPU may do around an atomic op.

---

## Cluster 3: Debugging Toolchain — GDB, Valgrind, cppcheck

### What is it?
GDB (a debugger — inspect running/crashed state), Valgrind (dynamic analysis — catches memory errors and leaks at runtime, at a real perf cost), and cppcheck (static analysis — flags suspicious patterns without running the code).

### Why does it exist?
C gives you no runtime safety net, so tooling has to catch what the language won't.

### How does it work?
- `gdb ./prog core` → `bt` (backtrace) to find the crash site; `p variable` to inspect state.
- `valgrind --leak-check=full ./prog` → runs your program under instrumentation, reports leaks, use-after-free, uninitialized reads.
- `cppcheck --enable=all src/` → flags patterns like unchecked returns, resource leaks, before you ever run the code.

### Real-world use / Resume Connection
NetSight's validation methodology explicitly names all three — this is a very likely "walk me through your testing process" question.

### Interview Questions
- **L1:** name what each tool catches and when you'd reach for it.
- **L2:** explain the tradeoff — Valgrind's overhead (often 10-50x slowdown) means it's for correctness validation, not production, and how that shaped when/how you ran it (offline test suite vs. live capture).
- **L3:** be ready to walk through one specific bug class each tool would catch that the others wouldn't (cppcheck catches it before compile; Valgrind catches it only if the buggy code path actually executes; GDB only helps after a crash already happened).

### Common Mistakes
- Treating a clean Valgrind run as proof of no bugs (only proves no bugs *in code paths exercised by that run*)
- Not knowing cppcheck runs without executing the program (a very quick "do you actually know what static analysis is" check)

### Coding/Debugging Exercise
Given a Valgrind report showing "Invalid read of size 4," trace back to the likely C mistake (reading after `free`, or an off-by-one array access) and fix it.

### Quick Revision
- GDB: post-mortem / interactive debugging.
- Valgrind: runtime memory-error and leak detection, high overhead.
- cppcheck: static analysis, no execution needed, fast.
