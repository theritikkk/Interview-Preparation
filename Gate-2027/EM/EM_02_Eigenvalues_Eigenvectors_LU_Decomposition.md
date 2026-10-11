# Engineering Mathematics → Eigenvalues, Eigenvectors & LU Decomposition

**GATE 2027 syllabus:** Engineering Mathematics — Linear Algebra: eigenvalues and eigenvectors, LU decomposition.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** For an n×n matrix A, the sum of the eigenvalues equals trace(A) and the product equals det(A). LU decomposition writes A = LU with L unit lower-triangular (1's on the diagonal) and U upper-triangular, using Gaussian elimination without row interchanges (assumed possible for all matrices below).

---

## Questions

### Q1 · MCQ · Fundamental
For an n×n matrix A, the sum of all n eigenvalues (with multiplicity) equals

- (A) det(A)
- (B) trace(A)
- (C) rank(A)
- (D) n²

---

### Q2 · NAT · Fundamental
The eigenvalues of
$$A = \begin{pmatrix} 4 & 1 \\ 2 & 3 \end{pmatrix}$$
are 2 and 5. The value of det(A) is ______.

---

### Q3 · MSQ · Standard
Which of the following statements is/are TRUE about the eigenvalues of a **real symmetric** matrix?

- (A) All eigenvalues are real.
- (B) Eigenvectors corresponding to distinct eigenvalues are orthogonal.
- (C) The matrix is always diagonalizable.
- (D) All eigenvalues are always positive.

---

### Q4 · NAT · Standard
The matrix
$$D = \begin{pmatrix} 3 & 2 \\ 1 & 4 \end{pmatrix}$$
has eigenvalues 2 and 5. The larger eigenvalue of D² (D multiplied by itself) is ______.

---

### Q5 · MCQ · Standard
If A is invertible with eigenvalue λ (λ ≠ 0), then A⁻¹ has a corresponding eigenvalue of

- (A) λ
- (B) −λ
- (C) 1/λ
- (D) λ²

---

### Q6 · NAT · Moderate
LU decomposition (no row interchanges) of
$$A = \begin{pmatrix} 4 & 3 \\ 6 & 3 \end{pmatrix}$$
gives A = LU with L unit lower-triangular. The value of U₂₂ (the bottom-right entry of U) is ______  (give as a decimal, e.g. −1.5).

---

### Q7 · MSQ · Moderate
Which of the following statements about LU decomposition is/are TRUE?

- (A) det(A) = det(L) × det(U) = product of the diagonal entries of U (since L is unit lower-triangular, det(L) = 1).
- (B) LU decomposition can always be performed without row interchanges, for every invertible matrix.
- (C) Once A = LU is known, solving Ax = b reduces to solving two triangular systems (Ly = b, then Ux = y), each in O(n²) time instead of O(n³) for a fresh full elimination.
- (D) LU decomposition of a given matrix, when it exists without row interchanges, is unique if L is required to have 1's on its diagonal.

---

### Q8 · NAT · Moderate
The matrix
$$E = \begin{pmatrix} 2 & 1 \\ 0 & 2 \end{pmatrix}$$
has a repeated eigenvalue 2 (with algebraic multiplicity 2). The geometric multiplicity of this eigenvalue (the dimension of its eigenspace, i.e., the number of linearly independent eigenvectors) is ______.

---

### Q9 · MSQ · Hard
Let A be a 3×3 matrix with eigenvalues 2, 3 and −1. Which of the following statements is/are TRUE?

- (A) det(A) = −6.
- (B) trace(A) = 4.
- (C) A is invertible.
- (D) A is diagonalizable (this follows necessarily just from having 3 distinct eigenvalues for a 3×3 matrix).

---

### Q10 · NAT · Killer
The matrix
$$A = \begin{pmatrix} 1 & 2 \\ 3 & 4 \end{pmatrix}$$
has characteristic polynomial λ² − 5λ − 2 (from trace = 5, det = −2). By the Cayley–Hamilton theorem, A² = 5A + 2I. Using this relation, if A³ can be written as pA + qI for scalars p and q, then p + q is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | B | Standard fact: sum of eigenvalues = trace |
| 2 | 10 | 2 × 5 |
| 3 | A, B, C | (D) is false — eigenvalues can be negative, zero or positive; only positive **definite** symmetric matrices have all-positive eigenvalues |
| 4 | 25 | Eigenvalues of D² are the squares of D's eigenvalues: 2² and 5²; the larger is 25 |
| 5 | C | If Av = λv then A⁻¹v = (1/λ)v |
| 6 | -1.5 | Eliminating: R2 → R2 − (6/4)R1 gives U = [[4,3],[0,3−4.5]] = [[4,3],[0,−1.5]] |
| 7 | A, C, D | (B) is false — some invertible matrices require row interchanges (partial pivoting) for LU to exist |
| 8 | 1 | Only one independent eigenvector, e.g. (1,0); E is defective, not diagonalizable |
| 9 | A, B, C, D | All four are standard, correct facts — n distinct eigenvalues for an n×n matrix always guarantees diagonalizability |
| 10 | 37 | A³ = A·A² = A(5A+2I) = 5A² + 2A = 5(5A+2I) + 2A = 27A + 10I, so p = 27, q = 10 |
