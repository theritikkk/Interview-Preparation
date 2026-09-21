# DBMS → Relational Algebra

**GATE 2027 syllabus:** Databases — relational algebra.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** Relations are sets (no duplicate tuples). σ = selection, π = projection, ⋈ = natural join, ⟗ = full outer natural join, − = set difference. Division: for R(A, B) and S(B), R ÷ S = π_A(R) − π_A((π_A(R) × S) − R).

---

## Questions

### Q1 · NAT · Fundamental
R and S are union-compatible relations with 15 and 12 tuples. Let M and m be the maximum and minimum possible number of tuples in R ∪ S. Then M − m = ______.

---

### Q2 · MCQ · Fundamental
Which one of the following is a **fundamental** (primitive) operator of relational algebra, i.e. one that cannot be expressed using the others?

- (A) Intersection
- (B) Natural join
- (C) Set difference
- (D) Division

---

### Q3 · NAT · Standard
R(A, B) = {(1, 2), (2, 3), (3, 4)} and S(B, C) = {(2, 5), (3, 6), (3, 7), (5, 8)}. The number of tuples in the full outer natural join R ⟗ S is ______.

---

### Q4 · MSQ · Standard
Which of the following equalities hold for all relations R and S (union-compatible where needed) and all conditions c, c₁, c₂?

- (A) σ_c1(σ_c2(R)) = σ_c2(σ_c1(R))
- (B) π_A(R ∪ S) = π_A(R) ∪ π_A(S)
- (C) π_A(R − S) = π_A(R) − π_A(S)
- (D) σ_c(R ∩ S) = σ_c(R) ∩ σ_c(S)

---

### Q5 · NAT · Standard
R(A, B) = {(a1, b1), (a1, b2), (a1, b3), (a2, b1), (a2, b3), (a3, b1), (a3, b2), (a3, b3), (a4, b2)} and S(B) = {b1, b3}. The number of tuples in R ÷ S is ______.

---

### Q6 · MCQ · Moderate
For R(A, B) and S(B), the expression π_A(R) − π_A((π_A(R) × S) − R) computes

- (A) R ÷ S
- (B) the A-values of R that are associated with at least one B-value of S
- (C) the A-values of R that are associated with no B-value of S
- (D) π_A(R ⋈ S)

---

### Q7 · MSQ · Moderate
Which of the following queries can be expressed in relational algebra **without** the set-difference operator (using only σ, π, ×, ∪ and renaming)?

- (A) Names of employees who work in department 'CS'
- (B) Employees who work on no project
- (C) Employees who work in department 'CS' or in department 'EE'
- (D) Employees who work on every project

---

### Q8 · NAT · Moderate
R(A, B) has 6 tuples and A is a key of R. S(C, D) has 5 tuples and C is a key of S. The maximum possible number of tuples in σ_{A=C}(R × S) is ______.

---

### Q9 · NAT · Hard
R(A, B) = {(1,2), (1,3), (2,3), (3,1), (2,1)}, S(B, C) = {(2,3), (3,1), (3,4), (1,2), (1,3)} and T(C, A) = {(3,1), (1,3), (4,1), (2,2), (1,2)}. The number of tuples in the natural join R ⋈ S ⋈ T is ______.

---

### Q10 · NAT · Killer
R(A, B) = {(1, x), (1, y), (2, x), (3, z), (4, y), (4, z)}. Let S₁ = {x}, S₂ = {x, y} and S₃ = ∅ (each is a relation over B). Then |R ÷ S₁| + |R ÷ S₂| + |R ÷ S₃| = ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | 12 | M = 27 (disjoint), m = 15 (S ⊆ R) |
| 2 | C | Intersection = R − (R − S); join and division are derived |
| 3 | 5 | 3 matching tuples + (3, 4, NULL) + (NULL, 5, 8) |
| 4 | A, B, D | (C) fails, e.g. R = {(1,a)}, S = {(1,b)} |
| 5 | 3 | a1, a2, a3 are related to both b1 and b3; a4 is not |
| 6 | A | This is the division expression |
| 7 | A, C | Negative and "every" queries need difference |
| 8 | 5 | Each S tuple matches at most one R tuple, since A is a key |
| 9 | 4 | (1,2,3), (1,3,4), (2,1,2), (2,3,1) |
| 10 | 7 | 2 + 1 + 4; dividing by an empty S returns all of π_A(R) |
