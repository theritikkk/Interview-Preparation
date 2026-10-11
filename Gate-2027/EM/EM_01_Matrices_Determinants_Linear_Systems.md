# Engineering Mathematics → Matrices, Determinants & Systems of Linear Equations

**GATE 2027 syllabus:** Engineering Mathematics — Linear Algebra: matrices, determinants, system of linear equations.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** A system Ax = b (A is m×n) is consistent iff rank(A) = rank([A|b]) (the augmented matrix). It has a unique solution iff additionally rank(A) = n (number of unknowns); otherwise it has infinitely many solutions. det(A) ≠ 0 iff A (square) is invertible iff rank(A) = n.

---

## Questions

### Q1 · MCQ · Fundamental
For a square matrix A, det(A) ≠ 0 is equivalent to

- (A) A has at least one zero eigenvalue
- (B) A is singular
- (C) A is invertible
- (D) rank(A) < n

---

### Q2 · NAT · Fundamental
The determinant of the matrix
$$A = \begin{pmatrix} 4 & 3 \\ 6 & 3 \end{pmatrix}$$
is ______.

---

### Q3 · MSQ · Standard
Which of the following elementary row operations on a square matrix A change the value of det(A)?

- (A) Swapping two rows (the determinant's sign flips)
- (B) Multiplying one row by a nonzero scalar c (the determinant is multiplied by c)
- (C) Adding a multiple of one row to another row
- (D) None of these operations ever change det(A), since row operations preserve the matrix's rank

---

### Q4 · NAT · Standard
The rank of the matrix
$$B = \begin{pmatrix} 1 & 2 & 3 \\ 2 & 4 & 6 \\ 1 & 1 & 1 \end{pmatrix}$$
is ______.

---

### Q5 · MCQ · Standard
A system of 2 linear equations in 3 unknowns, Ax = b, has rank(A) = 2 and rank([A|b]) = 2. The system has

- (A) no solution
- (B) a unique solution
- (C) infinitely many solutions (a 1-parameter family)
- (D) exactly two solutions

---

### Q6 · NAT · Moderate
Solve the system
$$x + y + z = 6,\qquad 2y + 5z = -4,\qquad 2x + 5y - z = 27.$$
The value of x + 2y (using the unique solution) is ______.

---

### Q7 · MSQ · Moderate
Which of the following statements is/are TRUE for a system Ax = b where A is m×n?

- (A) If m < n (fewer equations than unknowns) and the system is consistent, it cannot have a unique solution.
- (B) If m > n (more equations than unknowns), the system is always inconsistent.
- (C) The system Ax = 0 (homogeneous) is always consistent, since x = 0 is always a solution.
- (D) The solution set of Ax = 0, if it contains a nonzero vector, is a subspace of dimension n − rank(A).

---

### Q8 · NAT · Moderate
For the system
$$x + y + z = 6,\qquad 2x + 3y + kz = 14,\qquad x + ky + 3z = 10,$$
the coefficient matrix is singular for exactly two values of k. The **larger** of these two values is ______.

---

### Q9 · MSQ · Hard
Let A be a 4×4 matrix with rank(A) = 2. Which of the following statements is/are TRUE?

- (A) The system Ax = 0 has a solution space of dimension 2.
- (B) det(A) = 0.
- (C) A has at least 2 linearly independent rows and at least 2 linearly independent columns, and these two counts (row rank and column rank) are always equal.
- (D) The system Ax = b, for an arbitrary chosen b, is guaranteed to be consistent.

---

### Q10 · NAT · Killer
For the system
$$x + y + z = 6,\qquad 2x + 3y + kz = 14,\qquad x + ky + 3z = 10,$$
(same system as Q8, singular exactly when k = 0 or k = 3): at k = 0 the system is **inconsistent**, and at k = 3 the system has **infinitely many** solutions. At k = 3, one particular solution is found by setting z = 0. The value of x·y at that particular solution (with z = 0) is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | C | Non-zero determinant is exactly the invertibility condition |
| 2 | −6 | 4×3 − 3×6 |
| 3 | A, B, C | (D) is false — row operations preserve rank but can change the determinant's value (except adding a multiple of one row to another, which preserves it) |
| 4 | 2 | Row 2 = 2×Row 1, so only 2 independent rows |
| 5 | C | Consistent (equal ranks) but rank(A) = 2 < 3 unknowns, so a 1-parameter family |
| 6 | 11 | Unique solution (x,y,z) = (5,3,−2); x + 2y = 5 + 6 |
| 7 | A, C, D | (B) is false — a consistent system with m > n and redundant equations is entirely possible |
| 8 | 3 | det = −k² + 3k = −k(k−3), zero at k = 0 and k = 3; the larger root is 3 |
| 9 | A, B, C | (D) is false — with rank(A) = 2 < 4, most choices of b make Ax = b inconsistent |
| 10 | 8 | At k=3, z=0: x+y=6 and 2x+3y=14 give x=4, y=2 (checks against the third equation too); x·y = 8 |
