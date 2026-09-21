# DBMS → Transactions: Schedules, Serializability & Recoverability

**GATE 2027 syllabus:** Databases — transactions and concurrency control.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** rᵢ(x) and wᵢ(x) are a read and a write of item x by transaction Tᵢ; cᵢ and aᵢ are commit and abort. Two operations conflict if they are on the same item, by different transactions, and at least one is a write. Two schedules are conflict equivalent if they order every pair of conflicting operations identically. A schedule is recoverable if a transaction commits only after every transaction whose writes it has read has committed. A schedule of a set of transactions is an interleaving that keeps each transaction's own operation order.

---

## Questions

### Q1 · MCQ · Fundamental
The ACID property that guarantees a transaction either executes completely or has no effect at all is

- (A) Atomicity
- (B) Consistency
- (C) Isolation
- (D) Durability

---

### Q2 · NAT · Fundamental
The number of distinct serial schedules of 4 transactions is ______.

---

### Q3 · MCQ · Standard
Schedule S: r₂(A) r₃(C) w₂(B) w₃(B) w₁(A) w₁(C). Which one of the following serial schedules is conflict equivalent to S?

- (A) T₁, T₂, T₃
- (B) T₂, T₃, T₁
- (C) T₃, T₂, T₁
- (D) T₂, T₁, T₃

---

### Q4 · NAT · Standard
Schedule S: r₂(B) r₄(B) w₁(A) w₃(B) w₃(A) r₁(C) w₄(C) r₂(D). The number of serial schedules that are conflict equivalent to S is ______.

---

### Q5 · MSQ · Standard
Which of the following schedules is/are recoverable?

- (A) r₁(x) w₁(x) r₂(x) w₂(x) c₂ c₁
- (B) w₁(x) r₂(x) c₁ c₂
- (C) w₁(x) r₂(x) w₂(y) c₂ a₁
- (D) w₁(x) c₁ r₂(x) c₂

---

### Q6 · NAT · Moderate
T₁ = r₁(x) w₁(y) and T₂ = r₂(y) w₂(x). Among all 6 interleavings of T₁ and T₂ (each transaction's own order kept), the number that are conflict serializable is ______.

---

### Q7 · MCQ · Moderate
Schedule S: r₁(x) w₂(x) w₁(x) w₃(x). Which one of the following is TRUE?

- (A) S is conflict serializable
- (B) S is view serializable but not conflict serializable
- (C) S is neither view serializable nor conflict serializable
- (D) S is conflict serializable but not view serializable

---

### Q8 · NAT · Hard
Schedule S: w₃(y) w₃(x) w₁(z) w₂(z) w₂(y) w₁(x), where T₃ = w₃(y) w₃(x), T₁ = w₁(z) w₁(x) and T₂ = w₂(z) w₂(y). The number of schedules of T₁, T₂, T₃ that are conflict equivalent to S (including S itself) is ______.

---

### Q9 · MSQ · Hard
Which of the following statements is/are TRUE?

- (A) Every conflict-serializable schedule is view serializable.
- (B) Every recoverable schedule is conflict serializable.
- (C) Every strict schedule is cascadeless (avoids cascading aborts).
- (D) Every serial schedule (in which each transaction commits at its end) is recoverable.

---

### Q10 · NAT · Killer
T₁ = r₁(x) w₁(x), T₂ = r₂(x) w₂(x) and T₃ = w₃(x). Among all 30 interleavings of T₁, T₂, T₃, the number that are **view serializable but not conflict serializable** is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | A | All-or-nothing |
| 2 | 24 | 4! |
| 3 | B | Edges T₂→T₁ (A), T₃→T₁ (C), T₂→T₃ (B): unique order T₂, T₃, T₁ |
| 4 | 3 | Edges T₁→T₃, T₂→T₃, T₄→T₃, T₁→T₄: orders 2143, 1243, 1423 followed by T₃ |
| 5 | B, D | (A) T₂ commits before T₁; (C) T₂ commits, then T₁ aborts |
| 6 | 2 | Only r₁ w₁ r₂ w₂ and r₂ w₂ r₁ w₁ are acyclic |
| 7 | B | Cycle T₁ ⇄ T₂, but view equivalent to T₁, T₂, T₃ (blind writes) |
| 8 | 18 | Conflict pairs: w₃(y)<w₂(y), w₃(x)<w₁(x), w₁(z)<w₂(z), plus program orders; count linear extensions |
| 9 | A, C, D | (B) is false |
| 10 | 2 | r₁ w₃ r₂ w₁ w₂ and r₂ w₃ r₁ w₂ w₁ (8 view serializable, 6 conflict serializable) |
