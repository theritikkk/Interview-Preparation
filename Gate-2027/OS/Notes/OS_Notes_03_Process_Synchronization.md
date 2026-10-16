# OS Notes 03 — Process Synchronization (High-Yield Deep Dive)

> **Why this file exists:** "Process Synchronization" is on the high-yield list (video says it can be de-prioritized *if time is short* — but when you do study it, semaphore questions are almost mechanical once you know the technique). This file: race conditions → critical-section requirements → Peterson → hardware primitives → semaphores → producer–consumer → readers–writers → dining philosophers → the **adversary method** for any GATE interleaving question. Verified 10-question practice set at the end.
>
> **Companion question bank:** `OS_03_Semaphores_Synchronization.md`.

---

## 0. The problem: concurrent processes share state

```c
/* Both threads run: */
counter++;   // intended: counter = counter + 1
```

Compiled to machine code (each line ≈ one instruction, but the composite is **not atomic**):

```
LOAD  R, counter
ADD   R, R, #1
STORE counter, R
```

Interleaving with `counter = 5`:

| step | T1 | T2 | counter |
|---|---|---|---|
| 1 | LOAD R1 ← 5 | | 5 |
| 2 | | LOAD R2 ← 5 | 5 |
| 3 | ADD R1 = 6 | | 5 |
| 4 | | ADD R2 = 6 | 5 |
| 5 | STORE → 6 | | 6 |
| 6 | | STORE → 6 | 6 |

Two increments, net result 6 — one update **lost**. This is a **race condition**: the result depends on the interleaving.

**The fix pattern:** identify **critical sections** (code touching shared state) and ensure they never overlap in execution.

---

## 1. The critical-section problem — three precise requirements

A correct solution guarantees:

1. **Mutual exclusion (ME):** never two processes in their critical sections simultaneously.
2. **Progress:** if the CS is free and some process wants in, *some* wanting process gets in — and the decision cannot be postponed indefinitely by processes not wanting in. (ME alone is trivial: just `while(1);` — progress is what makes it fair.)
3. **Bounded waiting:** after a process requests entry, only a *bounded* number of other processes may enter before it. (Kills starvation.)

Also assume: no process halts inside its CS; no assumption about relative speeds (any interleaving is legal); failing to meet **bounded waiting** = starvation possible; failing **progress** = spinning/blocking even when CS is free.

---

## 2. Software solutions (build the intuition, meet the exam traps)

### 2.1 Naive flag (broken ME)

```c
flag[i] = true;
while (flag[j]);     // if other wants in, wait
CS;
flag[i] = false;
```

Both set their flags, both spin forever — **deadlock** (and if you drop the flag-check timing, both can enter). This exact snippet appears as a GATE MCQ (answer: *mutual exclusion is guaranteed but they may spin forever* — actually in the literal interleaving "flag[i]=true; flag[j]=true; check" **both loop forever; ME is never violated** — that's the exam's favorite nuance!).

> Strip it down: what the code guarantees is ME (each waits if the other's flag is set — nobody enters while the other is inside *unless* both set flags before either checks, in which case both block: liveness dies, safety survives). This distinction — *safety (ME) vs liveness (progress/termination)* — is the single most tested idea in this topic.

### 2.2 Strict alternation (broken progress)

```c
/* P0: */  while (turn != 0);   CS;  turn = 1;
/* P1: */  while (turn != 1);   CS;  turn = 0;
```

ME ✔, bounded waiting ✔ (they take turns), **progress ✘**: if P0 finishes and P1 doesn't want in, P0 can't re-enter even though the CS is free. A process is blocked by a *non-wanting* process.

### 2.3 Peterson's algorithm (the 2-process gold standard)

```c
flag[i] = true;          // I want in
turn = j;                // I defer to you
while (flag[j] && turn == j);   // wait while you want in AND it's your turn
   CS
flag[i] = false;
```

- **ME proof (by contradiction):** suppose both in CS. P0 entered ⇒ `flag[1]=false` at some point OR `turn=0`. P1 entered ⇒ (`flag[0]=false` OR `turn=1`). If both read `flag[j]` true, then `turn=0` AND `turn=1` — impossible. Else one read the other's flag false; but flags are only cleared after CS and set before → contradiction with both being in CS. ∎
- **Progress ✔** — if the other isn't interested (`flag[j]=false`), you sail through.
- **Bounded waiting ✔** — the `turn` variable guarantees the other gets in next.

> **GATE trap:** the order matters. If you write `turn = j;` **before** `flag[i] = true;`, mutual exclusion can break. Canonical buggy interleaving: P0 sets `turn=1`; P1 sets `turn=0`; P1 sets `flag[1]=true` and reads `flag[0]=false` → P1 enters; P0 sets `flag[0]=true`, reads `flag[1]=true` but checks `turn==1` while `turn` is now 0 → condition false → P0 enters too. **Both in CS.**

(Peterson extends poorly to n > 2 — Lamport's **bakery** algorithm is the general version; know its name, not the proof.)

---

## 3. Hardware primitives

**Test-and-set** — atomically returns the old value and sets to 1:

```c
lock()   : while (test_and_set(&L));   // spin
unlock() : L = 0;
```

**Swap** — same idea with two variables. Properties:

- Guarantees ME ✔ (atomic test-and-set).
- **Busy waiting** (spinlock) — wastes CPU; fine if CS is tiny, terrible under contention.
- **No bounded waiting** ✘ — the winner is whoever executes the atomic instruction next; a losing process can lose forever (in theory). Ticket locks (ticket + turn counters) fix bounded waiting.

Exam line to remember: *hardware primitives give you ME cheaply; fairness (bounded waiting) is your job.*

---

## 4. Semaphores — the workhorse

A semaphore `S` is an integer with **two atomic operations**:

```c
wait(S)  / P(S):   S = S - 1;  if (S < 0) block;      // decrement, maybe sleep
signal(S)/ V(S):   S = S + 1;  if (S <= 0) wake one blocked process;
```

**Reading the value:**

- `S.value ≥ 0` → S is the number of available permits.
- `S.value < 0` → `|S.value|` = number of processes **blocked** on S.

**Semaphore arithmetic for exams** (with the "completed operations" phrasing):

```
S_final = S_initial − (#completed waits) + (#completed signals)
```

wait on blocked process: it stays blocked; when a signal wakes it, it completes its wait (net effect is the process proceeds — the algebra still works if you only count *completed* waits, per the standard convention).

- **Binary semaphore** (values 0/1): a mutex.
- **Counting semaphore** (init k): k identical permits.
- `signal` may be called by anyone — a process that never waited, or even a different process (this is how *events* are signaled: one process signals "data ready", another was blocked in wait).
- A counting semaphore **can** be implemented with binary semaphores + an integer counter guarded by a mutex.

**Blocking > spinning** when the wait is long (no CPU burn). Both are legal solutions to ME.

---

## 5. The classics — every GATE synchronization question is a variant

### 5.1 Producer–Consumer (bounded buffer, capacity N)

```c
semaphore mutex = 1;    // guards buffer access itself
semaphore empty = N;    // count of free slots
semaphore full  = 0;    // count of filled slots

Producer:                          Consumer:
  while (1) {                        while (1) {
    produce(item);                     wait(full);      // ① may block
    wait(empty);  // ① may block      wait(mutex);     // ②
    wait(mutex);  // ②                remove(item);
    insert(item);                      signal(mutex);
    signal(mutex);                     signal(empty);   // ③
    signal(full); // ③                consume(item);
  }                                  }
```

**Why `wait(empty)` must come before `wait(mutex)`:** if you hold the mutex and then block on `empty` (buffer full) — the consumer can never run (it needs the mutex to free a slot) → **deadlock**. Deadlock-free ordering rule: *acquire the lock last, after you're sure you can proceed.*

**Invariants to recite:** `empty + full = N` (when no thread is mid-update); `empty ≥ 0`, `full ≥ 0`; mutex is 1 except while someone is inserting/removing.

### 5.2 Readers–Writers (first variant: readers priority)

```c
semaphore m = 1;   // guards rc
semaphore w = 1;   // the "resource" lock
int rc = 0;

Reader:                          Writer:
  wait(m);                         wait(w);
  rc++;                            WRITE;
  if (rc == 1) wait(w);  // first   signal(w);
  signal(m);                       // writer exclusive vs everyone
  READ;
  wait(m);
  rc--;
  if (rc == 0) signal(w);  // last
  signal(m);
```

- Many readers concurrently ✔; writer exclusive vs all ✔.
- **Writer starvation:** a steady stream of overlapping readers keeps `rc > 0` forever; the writer blocks. A second variant (writer priority: writers block new readers) flips the starvation to readers.

### 5.3 Dining Philosophers (5 philosophers, 5 forks, each needs 2)

**Naive:** each grabs left fork, then right → all five grab left → **deadlock**.

Fixes (each is a valid GATE answer):

1. **Asymmetry:** the last philosopher grabs the *right* fork first (breaks the cycle in the wait-for graph).
2. **Waiter/chairs:** a semaphore permits at most **4 seated** at once → pigeonhole guarantees someone gets both forks.
3. **All-or-nothing:** pick up both forks atomically (mutex around fork acquisition).

**Maximum number eating simultaneously** with 5 forks = **2** (2 disjoint pairs = 4 forks; the 5th fork is useless alone).

> Even with any standard fix, whether *starvation* can occur is a separate question — e.g., four philosophers could alternate who sits out and a fifth never eats (the waiter fix allows starvation in theory unless it has a fair queue). GATE tests "deadlock impossible" vs "starvation impossible" as *different* claims.

### 5.4 Barriers

N processes must all reach a point before any proceeds. With a counter + mutex + a semaphore: the first N−1 arrivals block on the barrier semaphore; the last arrival releases all N−1. (See practice Q7.)

---

## 6. The adversary method — how to *solve* any sync question

You are the scheduler; your job is to **prove the property FALSE** if possible, else argue why it can't break.

1. **List the atomic operations** (every `wait`, `signal`, print, single instruction — per the question's convention).
2. **Look for a "safety" violation:** is there an interleaving where two processes execute CS code simultaneously? Try to delay each process right after every step in turn. (This is how the §2.3 trap was found.)
3. **Look for "liveness" death:** is there an interleaving where a semaphore is held by someone who then blocks on another semaphore held by a spin-waiter? (Circular wait ⇒ deadlock. Producer–consumer's swapped waits is the canonical case.)
4. **For numeric questions:** use the semaphore algebra of §4 and the invariant `blocked = max(0, −S)`; count conservatively for "max/min" phrasing — the adversary chooses who runs next.
5. **For "can this value occur" questions:** compute all interleavings of load/store pairs (like §0) — usually 3 outcomes: serial order 1, serial order 2, and one "lost update" hybrid.

---

## 7. Practice set (verified by simulation where applicable)

### Q1 · NAT · Fundamental
A counting semaphore S is initialized to 4. At some later point, 9 `wait(S)` and 7 `signal(S)` operations have **completed** (a wait still blocked is not counted as completed). The current value of S is ______.

---

### Q2 · MCQ · Standard
Two processes use the following (buggy) variant of Peterson's algorithm, with `flag[0]=flag[1]=false`, `turn` arbitrary:

```c
/* process i, other = j */
turn = j;             /* NOTE: set before flag */
flag[i] = true;
while (flag[j] && turn == j) ;
    CS
flag[i] = false;
```

Which of the following is TRUE?

- (A) Mutual exclusion can be violated.
- (B) The algorithm is correct — it is just a reordering.
- (C) Mutual exclusion holds, but progress is lost.
- (D) Both processes can never pass the while loop.

---

### Q3 · NAT · Standard
Bounded-buffer (capacity 5) with `empty = 5`, `full = 0`, `mutex = 1`. After 3 successful insertions and 2 successful removals, the value of `empty` is ______.

---

### Q4 · MSQ · Standard
Which of the following is/are TRUE for the standard bounded-buffer solution?

- (A) Swapping `wait(mutex)` before `wait(empty)` in the producer can lead to deadlock.
- (B) A binary semaphore cannot be used to implement a counting semaphore.
- (C) `signal(S)` may legally be executed by a process that has never executed `wait(S)`.
- (D) With two producers sharing the buffer, the mutex semaphore is still required even though `empty`/`full` are correct.

---

### Q5 · NAT · Standard
Five philosophers sit around a table with five forks; each repeatedly takes **left fork then right fork** (naive solution). At most, how many philosophers can be eating at the same time? ______

---

### Q6 · MCQ · Standard
In the first readers–writers solution (readers priority), which of the following is TRUE?

- (A) A writer may starve because new readers keep arriving and entering.
- (B) Readers may starve because writers keep arriving.
- (C) Two writers can be inside their critical sections simultaneously.
- (D) A reader and a writer can be inside their critical sections simultaneously.

---

### Q7 · NAT · Moderate
A barrier for 4 processes is implemented with a counter (`count`, initialized 0), a mutex, and a semaphore `barrier` (initialized 0): each process does `wait(mutex); count++; signal(mutex); if (count < 4) wait(barrier); else release all;`. At the instant the 4th process is executing the release code (before any release), how many processes are blocked on `barrier`? ______

---

### Q8 · MSQ · Moderate
Process P1 executes `wait(A); wait(B);`; process P2 executes `wait(B); wait(A);` (both semaphores start at 1). Which of the following is/are TRUE?

- (A) An interleaving exists in which P1 holds A and waits for B while P2 holds B and waits for A — a deadlock.
- (B) If both processes instead acquire the locks in the same global order (A before B), this deadlock can no longer occur.
- (C) Deadlock is guaranteed to occur.
- (D) Busy-waiting instead of blocking on the semaphores would eliminate the deadlock.

---

### Q9 · NAT · Hard
`X = 1` initially. Two threads execute concurrently:

```c
T1:  X = X + 2;     /* load, add, store */
T2:  X = X * 2;     /* load, mul, store */
```

Every individual machine instruction is atomic, and load/store do not touch memory other than their operand. The **maximum** possible final value of `X` over all interleavings is ______.

---

### Q10 · MSQ · Killer
The dining philosophers use a semaphore `waiter` initialized to 4; each philosopher does `wait(waiter)`, then picks up both forks (left, then right), eats, puts forks down, `signal(waiter)`. Which of the following is/are TRUE?

- (A) Deadlock is impossible with `waiter = 4`.
- (B) Starvation of some philosopher is still conceivable.
- (C) Even with `waiter = 5`, deadlock remains impossible.
- (D) The asymmetric fix (one philosopher takes the right fork first) also removes the deadlock.

---

## Answer Key

| Q | Answer | Why (condensed) |
|---|---|---|
| 1 | 2 | 4 − 9 + 7 = 2 |
| 2 | A | Setting `turn` before `flag` lets both pass: see interleaving below |
| 3 | 4 | 5 − 3 + 2 |
| 4 | A, C, D | (B) false — binary semaphores + counter implement counting semaphores |
| 5 | 2 | Only 2 disjoint left/right pairs exist on 5 forks |
| 6 | A | Readers-priority: readers bypass a waiting writer |
| 7 | 3 | First 3 arrivals block; the 4th releases |
| 8 | A, B | Deadlock possible, not guaranteed; ordering fixes it |
| 9 | 6 | Serial order T1 then T2: (1+2)×2 = 6 |
| 10 | A, B, D | (C) false: `waiter = 5` is the naive scheme — deadlock possible |

### Full solutions

**Q1.** `S = 4 − 9 + 7 = 2`. Note: if the phrasing were "12 waits were issued and 9 completed" you'd mix in blocked count; here "completed" only. The invariant: a completed wait decrements, a signal increments, and the final value can't sit negative (that would mean uncompleted waits — excluded).

**Q2.** A — mutual exclusion can be violated. Interleaving: (1) P0: `turn = 1`; (2) P1: `turn = 0`; (3) P1: `flag[1] = true`; (4) P1 reads `flag[0] = false` → **P1 enters CS**; (5) P0: `flag[0] = true`; (6) P0 evaluates `flag[1] && turn == 1`: `flag[1]` is true but `turn` is 0 → condition **false** → **P0 enters CS**. Both inside. The correct order (`flag[i]=true; turn=j;`) is what makes the `turn` variable authoritative.

**Q3.** Every insert consumes an `empty` permit and emits a `full` permit; every removal the reverse. `empty = 5 − 3 + 2 = 4` (equivalently `full = 3 − 2 = 1`; `empty + full = 5` ✔).

**Q4.** A ✔ — hold-mutex-then-block-on-empty deadlocks against the consumer. B ✘ — counting = counter + binary mutex; standard construction. C ✔ — that's "signaling an event" (e.g., `full` is signaled by the producer, waited by the consumer). D ✔ — two producers inserting concurrently race on the buffer's `in` index; `empty`/`full` don't serialize *the insert itself*.

**Q5.** Two. Five forks support at most ⌊5/2⌋ = 2 simultaneous "fork pairs". (And naive grabbing lets all five *hold* one fork each — deadlock, not eating.)

**Q6.** A. The canonical readers-priority code has readers ignore a waiting writer, so an unbroken stream of readers starves the writer. (B) is the second variant's problem. (C) ✘ — `w` is exclusive for writers. (D) ✘ — first reader holds `w`, blocking every writer.

**Q7.** The first three arrivals each increment `count` (< 4) and block on `barrier` → **3 blocked**. The 4th sees `count == 4` and runs the release path.

**Q8.** A ✔ — classic circular wait: P1 holds A wants B; P2 holds B wants A. B ✔ — a global lock order makes circular wait impossible. C ✘ — the adversary *may* schedule it; not forced. D ✘ — busy-waiting vs blocking doesn't change who *holds* which lock; both can spin forever. (This Q8 + OS_04's Deadlock file is where hold-and-wait theory lives.)

**Q9.** Simulate all 6!-instruction interleavings mentally via the three way the "lost update" pattern can go: (i) T1 fully before T2: (1+2)×2 = **6**; (ii) T2 fully before T1: 1×2+2 = 4; (iii) interleaved loads: both read 1 → stores of 3 and 2 land in either order → 3 or 2. Possible finals: {2, 3, 4, 6}; **maximum = 6**.

**Q10.** A ✔ — 4 seat permits, 5 philosophers ⇒ at least one seated philosopher can't be blocked by two neighbors both missing chairs (pigeonhole: with 4 seated, at least one pair of neighbors is complete... precisely: 5 chairs-forks, 4 seats ⇒ someone seated has both neighbors absent ⇒ gets both forks ⇒ progress; no circular wait among the 4 seated). B ✔ — same concern as any of these schemes without a fair queue. C ✘ — `waiter = 5` permits all five to sit and grab left forks — the naive deadlock. D ✔ — the asymmetric rule breaks the cycle.

---

## 8. Next steps

1. Re-derive the Q2 counterexample and the producer–consumer deadlock from memory.
2. Work through `OS_03_Semaphores_Synchronization.md` (10 more questions, incl. value-tracking puzzles).
3. Continue to `OS_Notes_04_CPU_Scheduling.md`.
