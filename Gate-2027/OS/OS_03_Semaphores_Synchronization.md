# OS → Semaphores & Synchronization

**GATE 2027 syllabus:** Operating System — concurrency and synchronization, inter-process communication.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** `wait(S)` blocks while S = 0, otherwise decrements S; `signal(S)` increments S. Each `wait`, `signal`, `print` and each single machine instruction (LOAD, STORE, …) is atomic. Processes run concurrently on a system where any interleaving of these atomic steps is possible.

---

## Questions

### Q1 · NAT · Fundamental
A counting semaphore S is initialised to 7. After some time, 12 `wait(S)` operations and 8 `signal(S)` operations have **completed** on S (a `wait` that is still blocked is not counted among the 12). The current value of S is ______.

---

### Q2 · MCQ · Fundamental
Two processes P0 and P1 (i = 0 or 1, j = 1 − i) use the following entry/exit code. `flag[0] = flag[1] = false` initially.

```c
flag[i] = true;
while (flag[j]) ;        /* busy wait */
/* critical section */
flag[i] = false;
```

Which one of the following is TRUE?

- (A) Mutual exclusion can be violated.
- (B) Mutual exclusion is guaranteed, but both processes may loop forever in the `while` statement.
- (C) Mutual exclusion and progress are guaranteed, but bounded waiting is not.
- (D) Mutual exclusion, progress and bounded waiting are all guaranteed.

---

### Q3 · MSQ · Standard
Readers–writers solution, with `m = 1`, `w = 1` (semaphores) and `rc = 0` (shared integer):

```c
Writer:  wait(w);  WRITE;  signal(w);

Reader:  wait(m);  rc = rc + 1;  if (rc == 1) wait(w);  signal(m);
         READ;
         wait(m);  rc = rc - 1;  if (rc == 0) signal(w);  signal(m);
```

Which of the following statements is/are TRUE?

- (A) Several readers can be in `READ` at the same time.
- (B) A writer and a reader are never in their critical sections at the same time.
- (C) A continuous stream of overlapping readers can starve a waiting writer.
- (D) If the `wait(m)` / `signal(m)` pairs around the updates of `rc` are removed, the algorithm remains correct.

---

### Q4 · MCQ · Standard
Two threads share counting semaphores SA and SB:

```c
T1:  while (1) { wait(SA); print("A"); signal(SB); }
T2:  while (1) { wait(SB); wait(SB); print("B"); signal(SA); signal(SA); }
```

For which initial values (SA, SB) does the program print exactly the sequence **AABAABAAB…** forever?

- (A) (1, 0)
- (B) (2, 0)
- (C) (2, 1)
- (D) (3, 0)

---

### Q5 · NAT · Standard
A shared variable X is initialised to 1. Three processes run concurrently, with **no** synchronisation. Each executes its statement as three separate atomic instructions `LOAD X → reg`, `operate on reg`, `STORE reg → X`:

| P1 | P2 | P3 |
|---|---|---|
| X = X + 2 | X = X × 3 | X = X − 1 |

The number of **distinct final values** that X can have is ______.

---

### Q6 · MSQ · Moderate
Bounded buffer of N slots with a single producer and a single consumer, but the consumer's two `wait` operations are in the wrong order.

```c
semaphore empty = N, full = 0, mutex = 1;

Producer:  while (1) { produce;  wait(empty); wait(mutex);  add item;     signal(mutex); signal(full);  }
Consumer:  while (1) {           wait(mutex); wait(full);   remove item;  signal(mutex); signal(empty); consume; }
```

Which of the following statements is/are TRUE?

- (A) The system can reach a state in which both processes are blocked forever.
- (B) Whenever such a deadlock occurs, the buffer is empty.
- (C) The deadlock cannot be avoided by increasing N.
- (D) The producer and the consumer can access the buffer at the same time.

---

### Q7 · NAT · Moderate
A counting semaphore S is initialised to 2 and X = 0. Five processes execute the following code concurrently, where `X = X + 1` is compiled into the three atomic instructions LOAD, ADD, STORE:

```c
wait(S);
X = X + 1;
signal(S);
```

The **minimum** possible final value of X is ______.

---

### Q8 · MSQ · Moderate
Semaphores s = 0 and t = 0 are shared by two threads that start together:

```c
T1:  print("A"); signal(s); print("B"); wait(t); print("C");
T2:  wait(s);    print("D"); signal(t); print("E");
```

Which of the following output strings can be produced?

- (A) ABDEC
- (B) ADCBE
- (C) ADEBC
- (D) ABCDE

---

### Q9 · MCQ · Hard
Each of n (n ≥ 3) processes executes the following. Initially `m = 1`, `b = 0`, `count = 0`.

```c
/* CODE SECTION P */
wait(m);  count = count + 1;  if (count == n - 1) signal(b);  signal(m);
wait(b);  signal(b);
/* CODE SECTION Q */
```

Which one of the following is TRUE?

- (A) No process enters Q before all n processes have finished P.
- (B) A process may enter Q while exactly one other process has not yet finished P, but never while two or more have not finished P.
- (C) A process may enter Q while two or more other processes have not yet finished P.
- (D) The n-th process to reach the `wait(b)` may block forever.

---

### Q10 · NAT · Killer
Semaphores `s = 0` and `t = 1` are shared by three processes that run concurrently:

```c
P1:  wait(t); print("A"); signal(s); print("B");
P2:  wait(s); print("C"); signal(t); print("D");
P3:  wait(t); print("E"); signal(s); print("F");
```

Every run terminates. The number of **distinct output strings** (each containing all six letters) that can be produced over all possible interleavings is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | 3 | 7 − 12 + 8 |
| 2 | B | If both set their flag together, both spin forever; no `turn` variable |
| 3 | A, B, C | (D) fails: race on `rc` |
| 4 | B | (1,0) deadlocks after one A; (2,1) and (3,0) can print B early or a third A first |
| 5 | 8 | Final values {0, 2, 3, 4, 5, 6, 8, 9} |
| 6 | A, B, C | Consumer holds `mutex` while blocked on `full`; buffer must be empty then |
| 7 | 1 | One process stalls after LOAD holding a permit; the other four pass through the second permit (X = 4); stalled process stores 1 |
| 8 | A, C | Valid strings: ABDCE, ABDEC, ADBCE, ADBEC, ADEBC |
| 9 | B | Barrier opens when the (n−1)-th process arrives |
| 10 | 30 | First of A/E goes first, second of A/E only after C; 15 orderings for each case |
