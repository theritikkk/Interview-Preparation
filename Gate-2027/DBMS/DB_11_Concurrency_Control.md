# DBMS → Concurrency Control (2PL, Timestamp Ordering, Deadlocks, Isolation)

**GATE 2027 syllabus:** Databases — transactions and concurrency control.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:**

- **Basic timestamp ordering (TO):** each item Q has R-TS(Q) and W-TS(Q) (initially 0). Read by T: if TS(T) < W-TS(Q), abort T; otherwise execute and set R-TS(Q) = max(R-TS(Q), TS(T)). Write by T: if TS(T) < R-TS(Q), abort T; else if TS(T) < W-TS(Q), abort T; otherwise execute and set W-TS(Q) = TS(T).
- **Thomas write rule:** as above, except that a write with R-TS(Q) ≤ TS(T) < W-TS(Q) is ignored instead of aborting T.
- When a transaction aborts, all its remaining operations in the schedule are ignored, and timestamps it has already set are not undone. TS(T₁) < TS(T₂) < …
- **Wait-die / wound-wait** (locks are exclusive and held until the transaction ends; timestamp smaller = older): in wait-die an older requester waits and a younger requester aborts; in wound-wait an older requester aborts (wounds) the younger holder and a younger requester waits. A blocked transaction issues no further requests.

---

## Questions

### Q1 · MCQ · Fundamental
In two-phase locking, after a transaction has released its first lock it

- (A) may still acquire new locks, but only shared locks
- (B) cannot acquire any new lock
- (C) must abort
- (D) must release all remaining locks immediately

---

### Q2 · MCQ · Fundamental
Strict two-phase locking (a transaction holds all its exclusive locks until it commits or aborts) guarantees

- (A) freedom from deadlock
- (B) conflict-serializable and cascadeless schedules
- (C) view-serializable schedules that are not conflict serializable
- (D) freedom from starvation

---

### Q3 · NAT · Standard
Basic timestamp ordering with TS(T₁) = 1, TS(T₂) = 2, TS(T₃) = 3 is applied to the schedule

r₂(A) w₃(A) r₁(A) w₂(B) r₃(B) w₁(B) w₃(C) r₂(C).

The number of transactions that are aborted is ______.

---

### Q4 · MSQ · Standard
Which of the following statements is/are TRUE?

- (A) Timestamp ordering with the Thomas write rule can accept schedules that are view serializable but not conflict serializable.
- (B) Under the Thomas write rule, a write that is older than W-TS(Q) but not older than R-TS(Q) is ignored.
- (C) Timestamp-ordering protocols can cause deadlocks.
- (D) Basic timestamp ordering guarantees recoverable schedules without any additional measures.

---

### Q5 · NAT · Moderate
TS(T₁) = 5, TS(T₂) = 10, TS(T₃) = 15. The following exclusive-lock requests are issued in this order and no transaction commits during the sequence:

T₂ on B, T₃ on A, T₃ on C, T₁ on C, T₁ on B, T₁ on A.

(Number of aborts under wound-wait) − (number of aborts under wait-die) = ______.

---

### Q6 · NAT · Moderate
The lock table at some instant is

| Item | Held by | Waiting requests |
|---|---|---|
| A | X lock, T₁ | X by T₃, X by T₆ |
| B | X lock, T₂ | X by T₁ |
| C | X lock, T₃ | X by T₂ |
| D | S lock, T₄ | X by T₅ |
| E | X lock, T₅ | X by T₄ |

The minimum number of transactions that must be aborted to break **all** deadlocks is ______.

---

### Q7 · MSQ · Moderate
In multiple-granularity locking, which of the following pairs of lock modes held by two different transactions on the same node is/are **compatible**?

- (A) IS and IX
- (B) S and IX
- (C) SIX and IS
- (D) SIX and S

---

### Q8 · MSQ · Moderate
Under the SQL isolation level READ COMMITTED, which of the following anomalies can occur?

- (A) Dirty read
- (B) Non-repeatable read
- (C) Phantom read
- (D) Dirty write

---

### Q9 · NAT · Hard
TS(T₁) < TS(T₂) < TS(T₃) < TS(T₄). The schedule is

r₁(A) w₄(A) w₂(A) w₃(A) r₄(B) r₂(C) w₁(C).

Let X be the number of transactions aborted under basic TO but not under the Thomas write rule, and Y the number aborted under the Thomas write rule but not under basic TO. Then X + Y = ______.

---

### Q10 · NAT · Killer
T₁ = r₁(x) w₁(x), T₂ = w₂(x) and T₃ = w₃(x), with TS(T₁) < TS(T₂) < TS(T₃). Among the 12 interleavings of these transactions, the number that are accepted by timestamp ordering with the Thomas write rule with no abort **and** are not conflict serializable (conflict serializability judged on the schedule exactly as submitted) is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | B | Shrinking phase: no new locks |
| 2 | B | Strict 2PL does not prevent deadlock |
| 3 | 2 | r₁(A) is too late (W-TS(A) = 3) so T₁ aborts; r₂(C) is too late (W-TS(C) = 3) so T₂ aborts |
| 4 | A, B | (C) TO is deadlock-free but may restart transactions; (D) not guaranteed |
| 5 | 2 | Wound-wait: T₁ wounds T₃ (on C) and T₂ (on B) — 2 aborts; wait-die: T₁ waits on C and issues nothing further — 0 aborts |
| 6 | 2 | Cycles T₁→T₂→T₃→T₁ and T₄⇄T₅ are disjoint; T₆ only waits for T₁ |
| 7 | A, C | Compatibility matrix: IS–IX yes, S–IX no, SIX–IS yes, SIX–S no |
| 8 | B, C | READ COMMITTED prevents dirty reads and dirty writes |
| 9 | 3 | Basic aborts T₂, T₃ (obsolete writes to A); Thomas ignores both, but T₂'s later read r₂(C) makes w₁(C) too late, so T₁ aborts: 2 + 1 |
| 10 | 4 | Thomas accepts 6 interleavings; 2 of them are conflict serializable |
