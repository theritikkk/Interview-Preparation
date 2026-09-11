# OSTEP Notes 6 — Concurrency: Introduction, Thread API, and Locks

> Covers Ch26 (Concurrency: An Introduction), Ch27 (Interlude: Thread API), Ch28 (Locks). See `00-index.md` for the full series map.

---

## Part A: Concurrency — An Introduction

### The Thread Abstraction
A **thread** is much like a separate process, except it shares the **same address space** with other threads of the same process (so multiple threads can access the same data). Each thread has its **own** private set of registers, including its own **PC** and **stack** — because of the multiple stacks, an address space with threads looks different from the single-stack layout in earlier notes files: each thread's stack is carved out of a shared region, no longer able to grow arbitrarily toward the heap the way a single stack could.

**Why use threads instead of separate processes?**
- **Parallelism** — genuinely spread work across multiple CPUs.
- **Avoiding blocking due to slow I/O** — while one thread waits on I/O, another can run, keeping the CPU busy without needing separate *processes* (which would require more complex inter-process communication to share data).

### `pthread_create` / `pthread_join` — a Simple Example
```c
void *mythread(void *arg) {
    printf("%s\n", (char *) arg);
    return NULL;
}

int main(int argc, char *argv[]) {
    pthread_t p1, p2;
    Pthread_create(&p1, NULL, mythread, "A");
    Pthread_create(&p2, NULL, mythread, "B");
    Pthread_join(p1, NULL);
    Pthread_join(p2, NULL);
    return 0;
}
```
⭐ **The output order is nondeterministic** — exactly like `fork()`'s parent/child ordering (earlier notes file) — the scheduler decides which thread runs when, and this program provides no guarantee about which prints first. This is the reader's first taste of the general theme of this whole Part: **concurrent execution order is fundamentally unpredictable**, unless the programmer explicitly constrains it.

### Why It Gets Worse: Shared Data — the Canonical Race-Condition Example
```c
static volatile int counter = 0;

void *worker(void *arg) {
    int i;
    for (i = 0; i < 1e7; i++) { counter++; }
    return NULL;
}
// main: launch two worker threads, join both, expect counter == 20,000,000
```
Running this can produce the *correct* `20000000` — or, just as easily, something like `19345221` or `19221041` on different runs. ⭐ **Not only wrong, but a *different* wrong answer each time.**

**Why: the compiler generates a 3-instruction sequence for `counter++`:**
```asm
mov 0x8049a1c, %eax   ; load counter into eax
add $0x1, %eax         ; increment
mov %eax, 0x8049a1c   ; store back
```
⭐ **The full disaster trace** (counter starts at 50; Thread 1 gets interrupted mid-sequence):
```
                    OS                    Thread 1                Thread 2          PC  %eax  counter
                                          before critical section                   100   -     50
                                          mov ...,%eax                              105  50     50
                                          add $0x1,%eax                             108  51     50
                    interrupt (save T1)
                    restore T2                                                      100   0     50
                                                              mov ...,%eax           105  50     50
                                                              add $0x1,%eax          108  51     50
                                                              mov %eax,...           113  51     51
                    interrupt (save T2)
                    restore T1                                                       108  51     51
                                          mov %eax,...                              113  51     51
```
Both threads ran the increment sequence once each, yet **counter only went from 50 to 51**, instead of the expected 52 — Thread 1's stale `eax=51` (computed *before* the interrupt, from the old `counter=50`) overwrites Thread 2's already-correct update.

### Key Concurrency Vocabulary (all coined by Edsger Dijkstra)
| Term | Definition |
|---|---|
| **Critical section** | A piece of code that accesses a shared resource (variable/data structure) and must not be concurrently executed by more than one thread |
| **Race condition** | Arises when multiple threads enter a critical section at roughly the same time, both attempting to update shared state, producing a surprising/undesirable outcome |
| **Indeterminate program** | A program containing one or more race conditions — output varies run to run, depending on scheduling |
| **Mutual exclusion** | The property that guarantees only one thread can be actively executing within a critical section at a time |

### The Wish for Atomicity
⭐ **The ideal (impractical) fix:** a single hardware instruction that performed the whole `counter += 1` **atomically** — "as a unit," uninterruptible mid-execution. In principle this would solve the specific bug, but is **not a general solution** — you can't reasonably expect hardware to provide an atomic instruction for every conceivable critical section (imagine wanting an "atomically update this B-tree" instruction). ⭐ **The real approach:** ask the hardware for a small set of useful, general-purpose **synchronization primitives**, and build higher-level tools (**locks**, and later condition variables/semaphores) on top of them, with OS support.

### One More Problem: Waiting for Another
Beyond mutual exclusion, there's a second recurring interaction pattern: one thread needing to **wait** for another to complete some action (e.g., waiting for a disk I/O to finish, or for another thread to signal that some condition now holds) — this motivates **condition variables**, covered in the next notes file.

⭐ **"Why is this in an OS class?"** — the OS was itself the **first concurrent program** — early multiprogrammed operating systems had to grapple with exactly these hazards internally (e.g., two processes both in the middle of a `fork()` call, interleaved by an interrupt). The concepts of locks, critical sections, and race conditions were born out of the practical need to make OS kernels safe under concurrency, long before user-level multi-threaded programming became common.

---

## Part B: Interlude — The Thread API

### Thread Creation
```c
int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                    void *(*start_routine)(void *), void *arg);
```
- `thread` — pointer to a `pthread_t`, filled in with a handle to the new thread.
- `attr` — optional attributes (stack size, scheduling priority); usually `NULL` for defaults.
- `start_routine` — the function the new thread begins executing (a function pointer taking and returning `void *`).
- `arg` — the single argument passed to that function (often a pointer to a struct, to pass multiple logical values).

### Thread Completion
```c
int pthread_join(pthread_t thread, void **value_ptr);
```
- Blocks the caller until the specified `thread` completes.
- `value_ptr` receives whatever the joined thread `return`ed (or passed to `pthread_exit()`) — note this must be a pointer to **heap-allocated or otherwise durable** memory, **never** a pointer to the exiting thread's own now-destroyed stack (a classic, subtle bug).

### Locks (preview — Part C covers this in depth)
```c
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
Pthread_mutex_lock(&lock);
balance = balance + 1;
Pthread_mutex_unlock(&lock);
```

### Condition Variables (preview — next notes file)
```c
Pthread_mutex_lock(&lock);
while (ready == 0)
    Pthread_cond_wait(&cond, &lock);
Pthread_mutex_unlock(&lock);
```
⭐ **General advice from this interlude, worth internalizing:** *"keep your locking simple,"* avoid overly clever tricks, and always double-check return codes (real code should check them; the book's own examples often elide this purely for readability, wrapping calls in `Pthread_*` helper functions that assert success internally).

---

## Part C: Locks

> **The Crux: How to build an efficient lock?** What hardware support is needed? What OS support?

### The Basic Idea
```c
lock_t mutex;
lock(&mutex);
balance = balance + 1;
unlock(&mutex);
```
A lock is just a variable holding one of two states: **available/unlocked/free**, or **acquired/locked/held.**
- `lock()` — if free, acquire it and enter the critical section (calling thread becomes the **owner**); if already held, **block/wait** until it becomes available.
- `unlock()` — releases the lock; if other threads are waiting, one of them will (eventually) acquire it.

⭐ **Conceptual framing:** locks give the *programmer* a controlled degree of influence over otherwise-chaotic OS scheduling — by wrapping a critical section in a lock, the programmer guarantees only one thread is ever active there, "transforming the chaos of scheduling into controlled activity."

- **POSIX's name for a lock: mutex** ("mutual exclusion"). Passing a **specific** lock variable (rather than one giant global lock) allows **fine-grained locking** — different locks protecting different data, allowing more threads to make progress concurrently, versus a simpler but more restrictive **coarse-grained** single-lock-for-everything approach.

### Evaluating Locks — Three Criteria
| Criterion | Question |
|---|---|
| **Correctness** | Does it actually provide mutual exclusion? |
| **Fairness** | Does every contending thread get a fair shot at the lock? Can any thread **starve**, never acquiring it? |
| **Performance** | What's the time overhead — on one CPU, and across multiple CPUs? |

### Attempt 1: Controlling Interrupts
The **earliest** solution (single-processor only): disable interrupts for the duration of the critical section.
```c
void lock()   { DisableInterrupts(); }
void unlock() { EnableInterrupts();  }
```
⚠️ **Serious problems:** (1) requires giving **any** calling thread the privileged ability to disable interrupts — dangerous (a greedy or buggy thread could monopolize the CPU, or an infinite loop with interrupts disabled would hang the whole machine); (2) **doesn't work on multiprocessors** at all — disabling interrupts on one CPU does nothing to stop a thread running concurrently on a *different* CPU from also entering the critical section; (3) can **lose interrupts entirely** if disabled during a hardware event (e.g., a disk completion), a correctness hazard beyond just performance. ⭐ **Genuinely still useful, but only inside the OS kernel itself** — for protecting short, simple kernel-internal data structures, not as a general user-level locking mechanism.

### Attempt 2: A Software-Only Flag (broken)
```c
typedef struct __lock_t { int flag; } lock_t;
void lock(lock_t *lock)   { while (lock->flag == 1) ; /* spin */  lock->flag = 1; }
void unlock(lock_t *lock) { lock->flag = 0; }
```
⚠️ **Broken — not mutually exclusive.** The check (`while flag==1`) and the set (`flag=1`) are **two separate, non-atomic steps.** An interrupt between them lets two threads both see `flag==0`, both proceed to set it to 1, and both enter the critical section:
```
Thread 1: while(flag==1) [sees 0]
          --- interrupt ---
Thread 2: while(flag==1) [sees 0]; flag=1; --- interrupt ---
Thread 1 (resumed): flag=1 (too!)   <-- both threads now "hold" the lock
```
⭐ **Performance problem, independent of the correctness bug:** waiting threads **spin-wait** — endlessly re-checking `flag` — wasting CPU cycles, and (on a single CPU) actively preventing the lock-holder from ever running again to release it.

### Attempt 3 (Working): Test-and-Set — a Hardware Primitive
```c
int TestAndSet(int *old_ptr, int new) {
    int old = *old_ptr;   // fetch old value
    *old_ptr = new;       // store new value
    return old;           // return the OLD value
}                          // <-- this entire sequence is ATOMIC in hardware
```
```c
typedef struct __lock_t { int flag; } lock_t;
void init(lock_t *lock)   { lock->flag = 0; }
void lock(lock_t *lock)   { while (TestAndSet(&lock->flag, 1) == 1) ; /* spin-wait */ }
void unlock(lock_t *lock) { lock->flag = 0; }
```
⭐ **Why this works:** test **and** set happen as one indivisible hardware operation — no interrupt can land in between them. If `flag` was 0 (free), `TestAndSet` returns 0 (so the loop condition is false, lock acquired) *and simultaneously* sets `flag` to 1. If `flag` was already 1 (held), `TestAndSet` keeps returning 1 — the thread just spins until the holder eventually calls `unlock()`.
- Known by different names per architecture (SPARC's `ldstub`, x86's `xchg`) but the same concept — called a **spin lock**, the simplest lock to implement.
- ⭐ **Requires a preemptive scheduler to be useful at all on a single CPU** — without timer-interrupt-driven preemption, a spinning thread would never yield the CPU, and the lock-holder (stuck in the ready queue) would never get a chance to run and release it.
- ⭐ **Tip: think like a malicious scheduler.** The way to reason about whether a concurrency primitive is *actually* correct: imagine an adversarial scheduler deliberately interrupting threads at the worst possible moments — if *any* interleaving breaks correctness, the primitive is broken, no matter how improbable that interleaving might be in practice.

### Evaluating Spin Locks
- **Correctness:** ✅ yes — only one thread can hold the lock.
- **Fairness:** ❌ **no guarantee** — a spinning thread could in principle spin forever under contention (**starvation** is possible).
- **Performance:**
  - **Single CPU:** can be quite bad — if the lock-holder is preempted mid-critical-section, every *other* contending thread wastes its **entire time slice** just spinning and re-checking a value that can't change until the holder runs again.
  - **Multiple CPUs (≈ as many CPUs as contending threads):** works reasonably well — if the critical section is short, a thread spinning on CPU 2 waiting for a lock held on CPU 1 doesn't waste much — the holder keeps running (on its own CPU) and releases the lock quickly.

### Compare-And-Swap (CAS)
```c
int CompareAndSwap(int *ptr, int expected, int new) {
    int actual = *ptr;
    if (actual == expected) *ptr = new;   // only update if it still matches
    return actual;                          // always return what was actually there
}
```
Builds a lock almost identically to test-and-set:
```c
void lock(lock_t *lock) {
    while (CompareAndSwap(&lock->flag, 0, 1) == 1)
        ; // spin
}
```
⭐ **CAS is strictly more powerful than test-and-set** — because it only updates *if the value hasn't changed*, it's the foundational primitive behind **lock-free** and **wait-free** concurrent data structures (briefly touched on elsewhere in the book) — a caller can optimistically compute a new value, then use CAS to install it only if nothing else modified the location in the meantime, retrying otherwise.

### Ticket Locks — Adding Fairness via Fetch-And-Add
```c
int FetchAndAdd(int *ptr) { int old = *ptr; *ptr = old + 1; return old; }  // atomic

typedef struct __lock_t { int ticket; int turn; } lock_t;
void lock_init(lock_t *lock) { lock->ticket = 0; lock->turn = 0; }
void lock(lock_t *lock) {
    int myturn = FetchAndAdd(&lock->ticket);
    while (lock->turn != myturn) ; // spin
}
void unlock(lock_t *lock) { lock->turn = lock->turn + 1; }
```
⭐ **The key fairness guarantee this adds:** every thread gets a numbered "ticket" the instant it calls `lock()` — and since tickets are strictly increasing, **every thread is guaranteed to eventually be served**, in the exact order it arrived, unlike test-and-set/CAS spin locks, which offer **no such guarantee** (a thread could in theory be perpetually unlucky and never win the race to set the flag).

### Too Much Spinning: What Now?
⭐ **The remaining problem:** hardware-only solutions are simple and correct, but wasteful. If a lock-holder is preempted mid-critical-section, **every** other contending thread burns an entire time slice uselessly spinning — with `N` threads, that's `N−1` wasted time slices, purely to wait for one thread to be rescheduled and finish.

> **The Crux: how to avoid needless spinning?** Requires OS support, not just hardware.

**Attempt: just yield.**
```c
void lock()   { while (TestAndSet(&flag, 1) == 1) yield(); /* give up the CPU */ }
void unlock() { flag = 0; }
```
`yield()` moves the calling thread from **running → ready**, letting the scheduler pick someone else. Works reasonably well for a *small* number of threads — but with, say, 100 threads contending, 99 of them will each **run-then-immediately-yield** in turn before the lock-holder gets scheduled again — real (if reduced) **context-switch overhead**, and **still no protection against starvation** — a thread could in principle be perpetually skipped.

**The real fix: queues + sleeping (Solaris-style `park()`/`unpark()`).**
```c
typedef struct __lock_t { int flag; int guard; queue_t *q; } lock_t;

void lock_init(lock_t *m) { m->flag = 0; m->guard = 0; queue_init(m->q); }

void lock(lock_t *m) {
    while (TestAndSet(&m->guard, 1) == 1) ;     // spin briefly to acquire the GUARD
    if (m->flag == 0) {
        m->flag = 1;             // lock is acquired
        m->guard = 0;
    } else {
        queue_add(m->q, gettid());
        m->guard = 0;
        park();                  // sleep — don't waste CPU spinning on the real lock
    }
}

void unlock(lock_t *m) {
    while (TestAndSet(&m->guard, 1) == 1) ;
    if (queue_empty(m->q))
        m->flag = 0;                             // no one waiting, just release
    else
        unpark(queue_remove(m->q));               // hand the lock directly to the next waiter
    m->guard = 0;
}
```
⭐ **Key design points:**
- A tiny `guard` spin lock still protects the `flag`/`queue` bookkeeping itself — but this spin window is **extremely short** (a few instructions), not the (potentially arbitrarily long) user critical section — so the residual spin-waste is minimal.
- A thread unable to get the lock **adds itself to a wait queue and calls `park()`**, actually going to **sleep** (blocked, not spinning) — genuinely freeing the CPU for other useful work.
- On `unlock()`, if the queue isn't empty, the lock is handed **directly** to the next waiter via `unpark()` — note `flag` is deliberately **not** reset to 0 in this branch — ownership transfers straight from the releaser to the woken thread, without a window where some *other*, newly-arriving thread could sneak in and steal the lock out from under the queue's rightful next-in-line.
- ⚠️ A subtle ordering hazard the book flags directly: releasing the `guard` **must** happen *before* calling `park()`, not after — reversing the order can cause a **lost wakeup** (the thread parks itself permanently, having missed a concurrent `unpark()` that arrived in the gap). This exact kind of subtle timing bug is *why* real production lock implementations are notoriously easy to get wrong and are best left to well-tested libraries rather than hand-rolled from scratch.

### Different OS, Different Support (briefly)
- **Linux's `futex`** (fast userspace mutex) provides a similar in-kernel queue + explicit wake primitive, associated with a specific memory address, letting most of the fast, uncontended-case locking logic stay entirely in **user space** (cheap), only falling into the kernel when there's actual contention to arbitrate.
- **Two-phase locks** generalize the "spin a little, then sleep" idea explicitly: spin for a short bounded period first (betting the lock will be released soon, avoiding a costly context switch for a short critical section), and only **then** fall back to parking/sleeping if it's still not available — a pragmatic hybrid combining the low latency of spinning with the CPU-efficiency of sleeping under real contention.

---

## Summary Table

| Approach | Correctness | Fairness | Performance | Needs |
|---|---|---|---|---|
| **Disabling interrupts** | ✅ (uniprocessor only) | ❌ | Dangerous if misused; kernel-only in practice | Privileged instruction |
| **Naive flag (software only)** | ❌ broken | — | — | — |
| **Test-and-set spin lock** | ✅ | ❌ (can starve) | Bad on 1 CPU under preemption mid-CS; fine on multi-CPU w/ short CS | Hardware atomic TAS |
| **Compare-and-swap spin lock** | ✅ | ❌ | Same as TAS, but the primitive also enables lock-free structures | Hardware atomic CAS |
| **Ticket lock (fetch-and-add)** | ✅ | ✅ **guaranteed** | Still spins, same CPU cost concerns as TAS | Hardware atomic F&A |
| **Queue-based (park/unpark)** | ✅ | Depends on queue discipline (often FIFO) | Minimal spin-waste — real CPU freed via sleep | Hardware TAS *and* OS queue/park/unpark support |

## Interview/Exam-Ready Talking Points

- Walk through the **`counter++` race-condition trace** from memory, instruction by instruction — this is the single most common concurrency interview/exam example, and being able to narrate it precisely (including *why* the final value is wrong, not just *that* it is) is a strong signal.
- Precisely define **critical section, race condition, indeterminate program, and mutual exclusion** — these four Dijkstra terms come up constantly and are easy to conflate if not rehearsed.
- Explain **why disabling interrupts is an inadequate general-purpose locking mechanism** (multiprocessor failure, privilege danger, lost-interrupt risk) even though it's still legitimately used inside kernels.
- Derive **why the naive software-flag approach is broken**, and then explain precisely what **test-and-set's atomicity** buys you that the naive version lacks.
- Compare **test-and-set vs. compare-and-swap vs. ticket locks** on the fairness axis specifically — ticket locks are the only one of the three with a *guaranteed* progress property.
- Explain the **park/unpark queue-based lock**, including the subtle **lost-wakeup hazard** if `guard` is released after (rather than before) `park()` — this level of detail signals genuine depth beyond "locks use test-and-set."