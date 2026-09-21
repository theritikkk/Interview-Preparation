# DBMS → Relational Model, Keys & Integrity Constraints

**GATE 2027 syllabus:** Databases — relational model, integrity constraints.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** Relations are sets of tuples (no duplicates). NULL is not a value of any domain unless the question says the attribute may be NULL. Referential actions follow standard SQL (RESTRICT is checked immediately and the whole statement fails if it is violated).

---

## Questions

### Q1 · MCQ · Fundamental
A tuple with NULL in a primary-key attribute is inserted into a relation. Which integrity constraint is violated?

- (A) Referential integrity
- (B) Entity integrity
- (C) Domain constraint
- (D) Semantic integrity

---

### Q2 · NAT · Fundamental
A relation R(A, B, C, D) has exactly one candidate key, {A}. The number of superkeys of R is ______.

---

### Q3 · NAT · Standard
A relation R(A, B, C, D) has exactly two candidate keys, {A, B} and {C}. The number of superkeys of R is ______.

---

### Q4 · MSQ · Standard
Emp(eid, name, did) has a foreign key did referencing Dept(did). Which of the following operations can violate referential integrity?

- (A) Inserting a tuple into Emp
- (B) Deleting a tuple from Emp
- (C) Deleting a tuple from Dept
- (D) Updating Dept.did

---

### Q5 · NAT · Standard
Four tables are related by foreign keys:

- Dept has 1 row.
- Emp has 5 rows, each referencing that Dept row (ON DELETE CASCADE).
- Project has 10 rows, 2 per Emp row, each referencing its Emp row (ON DELETE SET NULL).
- Task has 30 rows, 3 per Project row, each referencing its Project row (ON DELETE CASCADE).

After `DELETE FROM Dept` (removing that single row) succeeds, the total number of rows removed from the four tables (including the Dept row) is ______.

---

### Q6 · NAT · Moderate
The maximum possible number of candidate keys in a relation with 5 attributes is ______.

---

### Q7 · MSQ · Moderate
Which of the following statements is/are TRUE?

- (A) No attribute of a primary key may be NULL.
- (B) A foreign-key attribute may be NULL.
- (C) A foreign key must reference the primary key of the other table; it cannot reference any other candidate key.
- (D) A table may have more than one primary key.

---

### Q8 · NAT · Moderate
R(A, B) has A as its primary key with domain {1, 2, 3}. Attribute B has domain {x, y} and **may be NULL**. The number of distinct legal instances of R (including the empty instance) is ______.

---

### Q9 · NAT · Hard
Tables A, B, C, D are related by foreign keys:

- A has 1 row.
- B has 4 rows, each referencing that row of A (ON DELETE CASCADE).
- C has 6 rows, each referencing some row of B, and every row of B is referenced by at least one row of C (ON DELETE RESTRICT).
- D has 3 rows, each referencing some row of C (ON DELETE CASCADE).

The statement `DELETE FROM A` (removing the single row) is executed. The total number of rows removed from the four tables is ______.

---

### Q10 · NAT · Killer
R(A, B, C, D) currently contains the five tuples

| A | B | C | D |
|---|---|---|---|
| 1 | x | p | 10 |
| 2 | x | q | 10 |
| 3 | y | p | 20 |
| 4 | y | p | 30 |
| 5 | z | q | 20 |

The number of non-empty subsets X of {A, B, C, D} that are **not ruled out** as superkeys by this instance (no two tuples agree on all attributes of X) is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | B | Primary key must be non-NULL |
| 2 | 8 | Every subset containing A: 2³ |
| 3 | 10 | 8 (contain C) + 4 (contain AB) − 2 (contain both) |
| 4 | A, C, D | Deleting an Emp tuple never breaks the reference |
| 5 | 6 | Dept + 5 Emp rows; SET NULL keeps Project rows, so Task rows are not cascaded |
| 6 | 10 | Largest antichain of subsets of a 5-set: C(5, 2) |
| 7 | A, B | (C): a foreign key may reference any candidate key; (D): only one primary key |
| 8 | 64 | Each key value is absent, or present with B ∈ {x, y, NULL}: 4³ |
| 9 | 0 | Cascade reaches C rows referencing B rows; RESTRICT blocks it and the whole statement fails |
| 10 | 10 | A, AB, AC, AD, CD, ABC, ABD, ACD, BCD, ABCD |
