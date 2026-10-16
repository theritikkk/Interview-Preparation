# DBMS Notes 04 — Transactions, Concurrency Control & Recovery (High-Yield Deep Dive)

> **Why this file exists:** "Transactions" is the alternate half of the video's "Indexing (preferred) / Transactions" topic. GATE's questions here are **classification + counting**: is a schedule conflict/view serializable, how many serial schedules are conflict-equivalent (topological orders), is it recoverable/cascadeless/strict, does a lock trace satisfy 2PL, what does timestamp ordering do operation-by-operation, and what happens at crash-recovery with checkpoints. Concurrency/history counting was **verified by exhaustive enumeration**; timestamp simulations and graphs were executed.
>
> **Companion question banks:** `DB_10_Transactions_Schedules_Serializability.md`, `DB_11_Concurrency_Control.md`. PW source: `../Reference/PW_Subject_Handbooks/PW_DBMS.txt` (§5, pages 10.32–10.46).

---

## 0. The mental model: a serial schedule is always correct — and always slow

```
   One transaction at a time  →  perfectly consistent, terrible throughput
   Interleave them            →  throughput up, correctness at risk
   Concurrency control = allow only schedules whose net effect equals SOME serial order
                         + make failures survivable (recovery)
```

**ACID, and who enforces what (PW §5.4 — a classic MCQ):**

```
   A  Atomicity    ┐
                   ├── RECOVERY MANAGEMENT component   (undo/redo, logs, checkpoints)
   D  Durability   ┘
   I  Isolation    ────  CONCURRENCY-CONTROL component  (locking / timestamps)
   C  Consistency  ────  the USER / application logic   (valid DB states)
```

---

## 1. Transactions, schedules, counting (PW §5.1–5.10)

- Operations: `rᵢ(X)` (read X into Ti's local variable), `wᵢ(X)` (write Ti's value of X into the DB). A transaction may also `commit` or `abort`.
- **Serial schedule:** no interleaving; `n` transactions ⇒ `n!` serial schedules.
- **Concurrent schedules** of `T1 (n ops)` and `T2 (m ops)` (internal order preserved): choose positions for T1's ops among `n + m` slots ⇒

```
        #interleavings  =  ( n + m )! / ( n! · m! )        (PW §5.10)
```

- **Serializable schedule:** equivalent (in effect) to some serial schedule. Two notions:

```
   CONFLICT serializability  — same ORDER of every conflicting operation pair
   VIEW serializability      — same "what each read sees" + same final writer
```

---

## 2. Conflict serializability (PW §5.13)

**Conflict pair** = two operations from *different* transactions, on the *same* data item, with at least one **write**:

```
   rᵢ(X) … wⱼ(X)      wᵢ(X) … rⱼ(X)      wᵢ(X) … wⱼ(X)        ✗ rᵢ(X) … rⱼ(X)  (never conflicts)
```

**Precedence (serialization) graph:** vertex per transaction; edge `Ti → Tj` if some op of Ti conflicts with a later op of Tj **in the schedule order**.

```
   S is CONFLICT SERIALIZABLE  ⇔  graph is ACYCLIC
   the conflict-equivalent serial orders = the TOPOLOGICAL ORDERS of the graph
   ⇒  #conflict-equivalent serial schedules = #topological orders      (PW §5.13.3 note)
```

Two schedules are conflict-equivalent ⇔ same conflicts / interconvertible by swapping *adjacent non-conflicting* operations (PW §5.13.4). Same precedence graph ⇒ may or may not be conflict-equivalent; conflict-equivalent ⇒ same graph.

**Worked graph.** `S: r1(x) w2(x) r3(x) w2(y) r1(y) w3(y)`:

```
   x : r1 → w2  (T1→T2)      w2 → r3  (T2→T3)
   y : w2 → r1  (T2→T1)      w2 → w3  (T2→T3)
   cycle T1→T2→T1  ⇒  NOT conflict serializable
```

---

## 3. View serializability (PW §5.15)

S and S′ (same transactions) are **view-equivalent** iff, for every data item:

```
   1. INITIAL READ : the transaction reading the item's ORIGINAL value is the same
   2. READ-FROM    : every read gets its value from the same writer (i.e., reads the same write's result)
   3. FINAL WRITE  : the transaction that writes the LAST version is the same
```

`S` is view serializable if it is view-equivalent to some **serial** schedule. Facts:

```
   conflict serializable  ⇒  view serializable          (strictly stronger)
   view serializable but NOT conflict serializable exists — only via BLIND WRITES
       (a write whose value is not read before being overwritten within the schedule)
   All conflict-serializable schedules given 3 transactions: view-S ∩ ¬CS cases are rare ⇒ GATE loves them
```

**Canonical non-CS but view-S example:** `S: r1(x) w2(x) w1(x) w3(x)` — the `w2(x)` is a blind write; the final writer (T3) and reads can be matched by a serial order while the cycle from `w1/w2` ordering exists. (Contrast: if the cycle comes from reads-after-writes — real dependencies — view serializability also fails.)

---

## 4. Recoverability ladder (PW §5.16–5.19)

**Why it matters:** serializability says nothing about *failure*. A schedule can be perfectly serializable and still unrecoverable.

**Dirty read:** `Tj` reads a value written by `Ti` that has not yet committed/aborted.

```
   IRRECOVERABLE : some Tj reads Ti's uncommitted value AND commits BEFORE Ti commits/aborts
                   — if Ti aborts now, Tj's committed read can never be undone
   RECOVERABLE   : for every dirty-read pair, Tj commits AFTER Ti commits/aborts
                   (equivalently: no commit happens "before" its writer's outcome is known)
   CASCADELESS   : no dirty reads at all  (⇒ no cascading rollbacks)
   STRICT        : neither READS nor WRITES of an uncommitted item are allowed
```

**Strict ⊆ cascadeless ⊆ recoverable** (as sets of schedules). And:

```
   serializability ⊥ recoverability — independent properties:
      serializable but irrecoverable  : w1(x) r2(x) c2 c1        (graph is T1→T2, serializable!)
      recoverable but non-serializable: easy to build with cycles that never dirty-read
   strict/recoverable schedules need NOT be serializable      (PW Note §5.19)
```

**Cascading rollback:** `T1` aborts → every transaction that read T1's data must abort too, transitively. Cascadeless schedules prevent the cascade.

**Lost update:** `Tj` overwrites a value that `Ti` already updated (both based on the same earlier read) — one update vanishes. This can happen even in "innocent-looking" serializable-looking schedules: `r1(x) r2(x) w1(x) w2(x)`.

---

## 5. Locking (PW §5.21–5.26)

```
   SHARED  S(X) : read lock       EXCLUSIVE X(X) : read+write lock

   compatibility:   S–S granted   |  S–X, X–S, X–X denied
```

**Two-Phase Locking (basic 2PL):** a transaction has a **growing phase** (only lock requests) and a **shrinking phase** (only unlocks); the switch-over is the **lock point**.

```
   lock point order  =  the conflict-equivalent serial order of 2PL transactions
   2PL guarantees : conflict serializability
   2PL does NOT guarantee : freedom from deadlock/starvation, recoverability,
                            cascadelessness, or no-lost-updates
```

**The 2PL family (PW §5.24–5.26):**

| Protocol | Extra rule | Guarantees | Deadlock-free? |
|---|---|---|---|
| Basic 2PL | no lock *requests* after first unlock | serializability | No |
| Strict 2PL | hold all **exclusive** locks until commit/abort | serializability + strictly recoverable | No |
| Rigorous 2PL | hold **all** locks (S and X) until commit/abort | serializability + strict + order = commit order | No |
| Conservative 2PL | pre-acquire **all** locks before starting; on failure release all and retry | serializability + deadlock-free | Yes (starvation: No) |

*(PW's Summary table — memorize it: Basic/Strict/Rigorous/Conservative = Serializability ✔✔✔✔; Strict-recoverable ✘✔✔✘; Deadlock-free ✘✘✘✔; Starvation-free ✘✘✘✘.)*

**Deadlock detection:** wait-for graph (Ti → Tj if Ti waits for a lock held by Tj). Cycle ⇔ deadlock.

---

## 6. Timestamp ordering (PW §5.27)

Every transaction gets a unique `TS(T)` (start-time order). Every data item X keeps:

```
   RTS(X) = largest TS of a transaction that READ X successfully
   WTS(X) = largest TS of a transaction that WROTE X successfully
```

**Basic TSO rules (PW §5.27.1):**

```
   T wants r(X):  if WTS(X) > TS(T)  → ABORT T        (T is too old; a newer write exists)
                  else allow; RTS(X) = max(RTS(X), TS(T))

   T wants w(X):  if RTS(X) > TS(T)  → ABORT T        (a newer txn already read; T too old)
                  else if WTS(X) > TS(T) → ABORT T   (a newer version exists)
                  else allow; WTS(X) = TS(T)
```

**Thomas write rule (TWR):** replace the *write-write* abort with an **ignore**:

```
   T wants w(X):  if RTS(X) > TS(T) → ABORT T
                  else if WTS(X) > TS(T) → IGNORE the write (no abort; obsolete value)
                  else allow; WTS(X) = TS(T)
```

The ignored write would be overwritten by an even newer write anyway — so it is skipped. TWR accepts more schedules (view-serializable ones) than basic TSO.

**Strict TSO:** a transaction wanting to read/write an item that a *younger* transaction has written must wait until that writer commits/aborts → adds strict recoverability.

| Protocol | Serializability type | Deadlock | Starvation | Strict recoverable |
|---|---|---|---|---|
| Basic TSO | conflict | free | possible | not guaranteed |
| Thomas TSO | view | free | possible | not guaranteed |
| Strict TSO | conflict | free | possible | guaranteed |

**Why TSO is deadlock-free:** no waiting — operations either execute or abort (roll back), so no hold-and-wait cycle can form. Starvation is possible only if aborts always hit the same old transaction (avoid with restarts carrying a *new* timestamp).

---

## 7. Recovery: logs, undo/redo, checkpoints (PW §5.5)

```
   The log records: <Ti, X, old, new> for every write + start/commit/abort records
   UNDO : restore old values (for uncommitted transactions)
   REDO : reapply new values (for committed transactions — makes commits DURABLE)

   CHECKPOINT (periodic): force-out all dirty blocks  ⇒  cuts recovery work

   CRASH RECOVERY (checkpoint-based, the GATE model):
      REDO every transaction that COMMITTED after the last checkpoint
      UNDO every transaction that was ACTIVE (uncommitted) at the crash
      transactions committed before the checkpoint: nothing to do (already durable)
```

Failure sources (PW §5.6): power/software crash (OS or DBMS restart), concurrency controller kills, disk failure (RAID).

---

## 8. Trap sheet

| # | Trap | Fact |
|---|---|---|
| 1 | "Serializable ⇒ recoverable?" | No — independent (and vice versa) |
| 2 | Conflict vs view | CS ⇒ view-S; view-S but not CS only via blind writes |
| 3 | Counting conflict-equivalent schedules | = #topological orders of the precedence graph, not `k!` |
| 4 | r–r pairs | never conflicts |
| 5 | Topological order count | multiply/sum carefully; a linear chain has exactly 1 |
| 6 | Dirty read → recoverability | Recoverable needs *commit order*, cascadeless needs *no dirty read at all* |
| 7 | "w1(x) w2(x)" | not strict, but **cascadeless** (no reads!) and recoverable — READ THE DEFINITION |
| 8 | 2PL ⇒ no deadlock? | False — 2PL can deadlock and starve; only conservative 2PL is deadlock-free |
| 9 | Strict 2PL scope | holds **exclusive** locks only; rigorous holds ALL locks |
| 10 | TSO aborts vs ignores | basic TSO aborts on WTS > TS; **Thomas rule ignores** the stale write |
| 11 | RTS check on write | `RTS(X) > TS(T)` ⇒ abort is required even under Thomas rule |
| 12 | Checkpoint recovery | redo = committed-after-checkpoint; undo = active-at-crash; pre-checkpoint commits: untouched |

---

# Practice set — DBMS Notes 04

*Schedule classifications and counts were verified by enumerating conflict graphs and topological orders; the timestamp trace was simulated.*

### Q1 · MCQ · Fundamental

Which ACID property is primarily enforced by the **user (database designer / application logic)** rather than by a DBMS software component?

- (A) Atomicity
- (B) Durability
- (C) Consistency
- (D) Isolation

### Q2 · NAT · Fundamental

`T1` and `T2` each contain exactly 3 operations. Keeping each transaction's internal operation order, the number of distinct concurrent (interleaved) schedules of `T1` and `T2` is ______.

### Q3 · MCQ · Standard

`S: r1(x) w2(x) r3(x) w2(y) r1(y) w3(y)`. Which one of the following is TRUE?

- (A) `S` is conflict serializable, equivalent to serial order `T1, T2, T3`
- (B) The precedence graph contains both `T1 → T2` and `T2 → T1` (plus `T2 → T3`), so `S` is not conflict serializable
- (C) `S` is conflict serializable, equivalent to serial order `T2, T1, T3`
- (D) The precedence graph of `S` is acyclic

### Q4 · NAT · Standard

`S: w1(x) r2(x) w1(y) r3(y)`. The number of serial schedules that are conflict-equivalent to `S` is ______.

### Q5 · MSQ · Standard

Call a schedule **cascadeless** if no transaction reads a data item written by another transaction that has not yet committed. Which of the following schedules are cascadeless?

- (A) `w1(x) r2(x) c1 c2`
- (B) `w1(x) r2(x) c2 c1`
- (C) `w1(x) w2(x) c1 c2`
- (D) `w1(x) c1 r2(x) c2`

### Q6 · MCQ · Moderate

At some instant the lock table shows: `T1` holds an exclusive lock on `A` and waits for `C`; `T2` holds an exclusive lock on `B` and waits for `A`; `T3` holds an exclusive lock on `C` and waits for `B`. Which statement is TRUE?

- (A) There is no deadlock — the waits will clear as transactions finish
- (B) There is a deadlock involving `T1`, `T2` and `T3`
- (C) There is a deadlock involving only `T1` and `T2`
- (D) `T3` is not blocked; only its holder is waiting

### Q7 · NAT · Moderate

Timestamps: `TS(T1) = 10`, `TS(T2) = 20`, `TS(T3) = 15`. Initially `RTS(A) = WTS(A) = 0`. The operations arrive in this order: `w1(A), w2(A), w3(A), r2(A)`. The concurrency controller uses **basic timestamp ordering extended with the Thomas write rule**. The number of write operations that are **ignored without aborting any transaction** is ______.

### Q8 · MCQ · Standard

`S: r1(x) r2(x) w1(x) w2(x)` (each transaction reads, then writes, x). This schedule exhibits

- (A) a dirty read
- (B) the lost-update problem
- (C) an unrepeatable read
- (D) a phantom read

### Q9 · NAT · Moderate

Recovery timeline with one checkpoint:

```
   9:00  T1 starts          9:02  T1 commits
   9:05  CHECKPOINT
   9:06  T2 starts          9:08  T2 commits
   9:07  T3 starts (no commit record)
   9:09  T4 starts (no commit record)
   9:10  CRASH
```

Restart uses the standard checkpoint rule: redo transactions committed after the last checkpoint; undo transactions active at the crash. Let `R` = number of transactions redone, `U` = number undone. Then `R + U =` ______.

### Q10 · NAT · Killer

How many of the following schedules are **conflict serializable**?

```
   S1: r1(x) r2(x) w2(x) w1(x)
   S2: r1(x) w2(x) w1(x)
   S3: w1(x) r2(x) w2(x)
```

---

## Answer key (quick)

| Q | Ans | Q | Ans |
|---|---|---|---|
| 1 | **C** | 6 | **B** |
| 2 | **20** | 7 | **1** |
| 3 | **B** | 8 | **B** |
| 4 | **2** | 9 | **3** |
| 5 | **C, D** | 10 | **1** |

---

## Detailed solutions

**Q1.** **C (Consistency).** PW §5.4: Atomicity & Durability ⇒ recovery manager; Isolation ⇒ concurrency-control component; Consistency ⇒ the user (the DB must be consistent before/after, but *only the application knows the business rules*).

**Q2.** Choose positions for T1's 3 operations among 6 slots: `C(6,3) = 20`. (Formula `(3+3)!/(3!·3!) = 20`.)

**Q3.** Conflicts:
```
   x: r1 → w2 (T1→T2);  w2 → r3 (T2→T3)
   y: w2 → r1 (T2→T1);  w2 → w3 (T2→T3)
   edges: T1→T2, T2→T3, T2→T1  ⇒  cycle T1→T2→T1  ⇒  NOT conflict serializable
```
⇒ **B**. (A), (C), (D) all assert serializability/acyclicity — false.

**Q4.** Conflicts: `w1(x)` vs `r2(x)` → `T1→T2`; `w1(y)` vs `r3(y)` → `T1→T3`. No other conflict pairs (T2 and T3 never touch the same item). Graph: `T1 → T2`, `T1 → T3` — acyclic with topological orders `(1,2,3)` and `(1,3,2)` ⇒ **2** conflict-equivalent serial schedules.

**Q5.** Cascadeless ⇔ no dirty reads:
- (A) `r2(x)` reads `w1(x)` while T1 is uncommitted → dirty ⇒ ✘.
- (B) same dirty read (and commits out of order, worse) ⇒ ✘.
- (C) no reads at all ⇒ **vacuously cascadeless ✔** (it is *not* strict, but cascade-freedom is only about reads).
- (D) read happens after `c1` ⇒ ✔.
⇒ **C, D**.

**Q6.** Wait-for edges: `T1 → T2` (T1 waits for lock on A held by T2), `T3 → T1` (T3 waits for C held by T1), `T2 → T3` (T2 waits for B held by T3). Cycle `T1 → T2 → T3 → T1` ⇒ deadlock among all three ⇒ **B**.

**Q7.** Simulate with Thomas rule:
```
   w1(A) TS=10 : RTS=0 ≤ 10, WTS=0 ≤ 10  → execute, WTS(A)=10
   w2(A) TS=20 : RTS=0 ≤ 20, WTS=10 < 20 → execute, WTS(A)=20
   w3(A) TS=15 : RTS=0 ≤ 15, WTS=20 > 15 → IGNORE (Thomas rule; no abort)      ← 1 ignored
   r2(A) TS=20 : WTS=20 ≤ 20             → execute, RTS(A)=20
```
Ignored writes = **1**, aborts = 0. (Under *basic* TSO, `w3` would have been aborted instead — the distinction GATE tests.)

**Q8.** Both transactions read x's old value; `w1(x)` writes T1's update; then `w2(x)` overwrites it with a value derived from the *pre-T1* read ⇒ **lost update (B)**. No uncommitted data is read (both reads precede the writes), so it is not a dirty read; nothing is re-read (not C); no new rows (not D).

**Q9.** Committed after the checkpoint ⇒ redo `{T2}` → `R = 1`. Active at crash (no commit) ⇒ undo `{T3, T4}` → `U = 2`. T1 committed before the checkpoint — its updates were forced out at 9:05, nothing to do. `R + U = 3`.

**Q10.** Build each graph:
```
   S1: r1 r2 → no conflict (r-r);  r1–w2 → T1→T2;  r2–w1 → T2→T1;  w2–w1 → T2→T1
       cycle T1→T2→T1                                        ✘
   S2: r1–w2 → T1→T2;  w2–w1 → T2→T1     cycle               ✘
       (interesting: view serializable as (T1,T2) — but NOT conflict serializable)
   S3: w1–r2 → T1→T2;  w1–w2 → T1→T2     acyclic ⇒ CS as (T1,T2)  ✔
```
Count = **1**.

---

*Cross-practice: `DB_10` (serializability, view-S counting, recoverability MSQ), `DB_11` (2PL variants, timestamp ordering, isolation levels). This closes the DBMS block — next subject: Algorithms.*
