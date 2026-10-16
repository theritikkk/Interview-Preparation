# EM Notes 01 — Linear Algebra: Matrices, Systems & Eigenvalues (High-Yield Deep Dive)

> **Why this file exists:** Engineering Mathematics is the final video topic, and the two linear-algebra banks industrialise a compact set of computations — **determinants → rank → linear systems → eigenvalues → LU**. Every number below was checked by exact arithmetic (fraction row-reduction, adjugate computation) before it was written; the practice questions are fresh (not in the banks) and every practice answer was verified by computation.
>
> **Companion question banks:** `../EM_01_Matrices_Determinants_Linear_Systems.md`, `../EM_02_Eigenvalues_Eigenvectors_LU_Decomposition.md`. **PW source:** `../Reference/PW_Subject_Handbooks/PW_EngineeringMath.txt` §2.9–§2.18 (pages 1.21–1.28: determinants, adjoint/inverse, rank, systems, eigenvalues, Cayley–Hamilton, similar matrices, diagonalization). **LU decomposition** is in the GATE syllabus but not in the PW extract — §6 supplies it.

---

## 0. What GATE actually asks

```
(A) Determinants        |kA| = kⁿ|A| ; |adj A| = |A|^(n−1) ; |AB| = |A||B| ; row-op effects
(B) Rank                echelon reduction ; rank–nullity ; ρ(AB) ; ρ(A+B) ; ρ(Aᵀ) = ρ(A)
(C) Linear systems      consistency trichotomy ; free parameters = n − ρ(A) ; singular k-values
(D) Eigenvalues         trace/det shortcuts ; eigen of Aᵐ, adj A, aA + bI ; diagonalizability
(E) Cayley–Hamilton     reduce A³, A⁴ … to cA + dI and read off coefficients
(F) LU decomposition    multipliers, U diagonal, det from U, cost, when pivoting is needed
(G) Standing structure  independence, basis, similar matrices, symmetric matrices
```

---

## 1. Matrices and vectors — the standing facts (PW §2.1–§2.8, §2.11)

- **Transpose reverses products:** (AB)ᵀ = BᵀAᵀ; (A + B)ᵀ = Aᵀ + Bᵀ; (Aᵀ)ᵀ = A (PW §2.3.1).
- **Trace:** trace(A + B) = trace A + trace B; **trace(AB) = trace(BA)** (cyclic swap); trace(cA) = c·trace A; transpose leaves the trace unchanged (PW §2.6.1).
- **Inverse reverses products:** (AB)⁻¹ = B⁻¹A⁻¹, (ABC)⁻¹ = C⁻¹B⁻¹A⁻¹; (Aᵀ)⁻¹ = (A⁻¹)ᵀ (PW §2.11).
- **2×2 inverse — memorise:** for

$$A = \begin{pmatrix} a & b \\ c & d \end{pmatrix},\qquad A^{-1} = \frac{1}{ad - bc}\begin{pmatrix} d & -b \\ -c & a \end{pmatrix}\quad (ad - bc \neq 0)$$

swap the diagonal entries, negate the off-diagonal entries, divide by the determinant (PW §2.11).
- **Named types worth knowing (PW §2.7–§2.8):** symmetric (Aᵀ = A); skew-symmetric (Aᵀ = −A: diagonal all zeros, **odd order ⇒ det = 0**); orthogonal (AᵀA = I ⇒ |det A| = 1); idempotent (A² = A ⇒ det ∈ {0, 1}); involutory (A² = I ⇒ det = ±1); nilpotent (Aᵏ = 0 ⇒ det = 0).
- **Linear independence test:** v₁, …, vₙ ∈ ℝⁿ are linearly independent ⟺ the n×n matrix with these vectors as rows (or columns) has nonzero determinant (PW §2.14).
- **Similar matrices:** B = P⁻¹AP for invertible P ⇒ A and B have the **same eigenvalues** (hence same trace and determinant) — this is the engine behind diagonalization (PW §2.17).

**Worked micro-example.** A = [[4,3],[6,3]]: det = 4·3 − 3·6 = **−6** ⇒ invertible, and A⁻¹ = (1/−6)·[[3,−3],[−6,4]]. (This is the banks' EM_01 anchor matrix; it reappears as the LU example in §6.)

---

## 2. Determinants (PW §2.9–§2.10)

The determinant is the scalar that is linear in each row, vanishes on equal rows, and equals 1 at I. Everything else follows:

| Property | Statement | Named grab-bag (PW §2.9.1) |
|---|---|---|
| Scaling | **\|kA\| = kⁿ·\|A\|** for n×n A | skew-symmetric odd order: \|A\| = 0 |
| Product | **\|AB\| = \|A\|·\|B\|** | orthogonal: \|A\| = ±1 |
| Transpose | **\|Aᵀ\| = \|A\|** | idempotent: \|A\| ∈ {0, 1} |
| Inverse | \|A⁻¹\| = 1/\|A\| | involutory: \|A\| = ±1 |
| Adjugate | **\|adj A\| = \|A\|^{n−1}** | nilpotent: \|A\| = 0 |
| Row swap | changes the sign | equal/proportional rows: \|A\| = 0 |
| Row × c | multiplies \|A\| by c | zero row/column: \|A\| = 0 |
| Rowᵢ += c·rowⱼ | **no change** | triangular/diagonal: product of the diagonal |

Two more identities worth keeping at fingertips: |adj(adj A)| = |A|^{(n−1)²} (PW §2.9.1 (iii)), and the adjugate's defining identity **A·adj(A) = adj(A)·A = |A|·I** (PW §2.9).

**Cofactor → adjugate → inverse (PW §2.10):**
- Minor Mᵢⱼ = determinant after deleting row i and column j; cofactor Cᵢⱼ = (−1)^{i+j}·Mᵢⱼ.
- **adj(A) = transpose of the cofactor matrix** — the transpose is the step everyone forgets.
- **A⁻¹ = adj(A)/|A|**, which exists exactly when |A| ≠ 0.

**Worked micro-example.** A = [[1,2],[3,4]]: |A| = −2, adj(A) = [[4,−2],[−3,1]], and A·adj(A) = [[−2,0],[0,−2]] = −2·I = |A|·I ✓.

**Elementary row operations vs the determinant:**
1. Rᵢ ↔ Rⱼ ⟹ det flips sign.
2. Rᵢ → c·Rᵢ (c ≠ 0) ⟹ det is multiplied by c.
3. Rᵢ → Rᵢ + c·Rⱼ ⟹ det is **unchanged**.

This is exactly why row-reducing to echelon form with a sign/scaling ledger computes determinants safely.

---

## 3. Rank (PW §2.12)

**Definition.** ρ(A) = the size of the largest square sub-matrix with nonzero determinant = the number of pivots (nonzero rows) in any row-echelon form of A. Rank is the dimension of the row space = dimension of the column space.

- ρ(A_{m×n}) ≤ min(m, n); the rank of the null matrix is 0 (PW §2.12).
- **Rank–nullity:** ρ(A) + nullity(A) = n (**number of columns**) — the most used identity in system questions (PW §2.12, §2.13.1).
- ρ(Aᵀ) = ρ(A); elementary row (and column) operations preserve rank (PW §2.12.1).
- ρ(AB) ≤ min{ρ(A), ρ(B)}; ρ(A + B) ≤ ρ(A) + ρ(B) — both bounds can be strict, and rank can *jump up* under addition (PW §2.12.1).
- **Full row rank:** ρ(A_{m×n}) = m; **full column rank:** ρ = n. For an m×n matrix, Ax = b is consistent **for every b** ⟺ ρ(A) = m.

**Worked micro-example.** B = [[1,2,3],[2,4,6],[1,1,1]]: row 2 = 2·row 1, while rows 1 and 3 are not proportional ⇒ ρ(B) = **2** (bank anchor). And for the classic [[1,2,3],[4,5,6],[7,8,9]]: row2 − row1 = (3,3,3), row3 − row1 = (6,6,6) = 2·(row2 − row1) ⇒ ρ = **2** (fresh practice Q4).

---

## 4. Linear systems Ax = b (PW §2.13)

Let A be m×n and [A|b] the augmented matrix. Compare ranks:

| Condition | Conclusion |
|---|---|
| ρ(A) ≠ ρ([A\|b]) | **No solution** (inconsistent) |
| ρ(A) = ρ([A\|b]) = n | **Unique solution** |
| ρ(A) = ρ([A\|b]) = r < n | **Infinitely many solutions**, an (n − r)-parameter family |

- b must lie in the column space of A for consistency.
- **Homogeneous Ax = 0 is always consistent** (x = 0 works). It has nontrivial solutions ⟺ ρ(A) < n; the solution set is a subspace of dimension n − ρ(A) (PW §2.13.2).
- **Solution-set geometry:** every solution is (one particular solution) + (any null-space/parameter combination): x = x_p + t₁v₁ + ⋯ + t_kv_k with k = n − r.
- **Square systems:** unique solution ⟺ |A| ≠ 0. If |A| = 0, the system is either inconsistent or has infinitely many solutions — compare ranks to decide.

**Worked micro-examples (bank-anchored):**
- 2 equations, 3 unknowns, ρ(A) = ρ([A|b]) = 2 ⇒ 3 − 2 = 1-parameter family (EM_01 Q5).
- The k-system x + y + z = 6, 2x + 3y + kz = 14, x + ky + 3z = 10 has coefficient determinant −k² + 3k = −k(k − 3): singular at k = 0 and k = 3. At k = 0 it is **inconsistent**; at k = 3 the equations collapse to rank 2 and the solutions form the family (4, 2 − t, t) (so z = 0 gives (4, 2, 0), x·y = 8) — EM_01 Q8/Q10.
- A 4×4 matrix with ρ(A) = 2 has a null space of dimension 2, det = 0, and Ax = b is *not* guaranteed consistent for arbitrary b (EM_01 Q9).

**Fresh practice twin (Q6 below):** x + 2y − z = 3, 2x − y + 2z = 6, 3x + y + z = 9 — the third equation is the sum of the first two, so rank 2 with a 1-parameter family through the particular solution (3, 0, 0).

---

## 5. Eigenvalues and eigenvectors (PW §2.14–§2.15)

**Definitions.** λ is an eigenvalue of A if (A − λI)v = 0 for some v ≠ 0; eigenvalues are the roots of the characteristic equation **det(A − λI) = 0**. For 2×2 matrices use the shortcut every time:

$$\lambda^2 - (\operatorname{trace} A)\,\lambda + \det A = 0$$

**Properties — learn as one block (PW §2.14.2 + §2.15.1):**

| If A (n×n) has eigenvalues λ₁, …, λₙ | then |
|---|---|
| Σλᵢ | = **trace(A)** |
| Πλᵢ | = **det(A)** |
| Aᵐ | eigenvalues λᵢᵐ (same eigenvectors) |
| adj(A) | eigenvalues \|A\|/λᵢ (λᵢ ≠ 0) |
| Aᵀ | same eigenvalues (eigenvectors generally differ!) |
| aA + bI | eigenvalues aλᵢ + b |
| triangular/diagonal | eigenvalues = diagonal entries |
| real symmetric | all eigenvalues real; eigenvectors orthogonal |
| skew-symmetric | eigenvalues 0 or purely imaginary; odd order ⇒ 0 is an eigenvalue |
| orthogonal | all eigenvalues have modulus 1 |
| constant row sums = k | k is an eigenvalue (eigenvector = all-ones) |

**Immediate consequences.** A is singular ⟺ 0 is an eigenvalue. A is invertible ⟺ no eigenvalue is 0, and then A⁻¹ has eigenvalues 1/λᵢ. Similar matrices share eigenvalues (PW §2.17).

**Multiplicity and diagonalizability (the exam-critical part).** For each eigenvalue λ: algebraic multiplicity = its power as a root of the characteristic polynomial; **geometric multiplicity = dim ker(A − λI) = n − ρ(A − λI)**. Always 1 ≤ geo ≤ alg.

- A is diagonalizable ⟺ A has **n linearly independent eigenvectors** ⟺ Σ geo(λᵢ) = n (PW §2.18.1).
- **n distinct eigenvalues ⇒ diagonalizable** (sufficient, *not* necessary — PW §2.18.1 note).
- A repeated eigenvalue decides nothing by itself: E = [[2,1],[0,2]] has alg 2 but geo 1 (defective). (PW §2.15.1 (iv) states a looser rule; the safe statement is the one here.)
- **Real symmetric matrices are always diagonalizable** — even with repeated eigenvalues; so are all matrices with n distinct eigenvalues. This is the banks' favourite True/False trap ("repeated eigenvalues ⇒ not diagonalizable" is false).

**Cayley–Hamilton (PW §2.15.2).** Every square matrix satisfies its own characteristic equation. Templates (both verified by direct multiplication):
- A = [[1,2],[3,4]]: char λ² − 5λ − 2 ⇒ A² = 5A + 2I ⇒ **A³ = A·A² = 27A + 10I** (the bank's killer: p = 27, q = 10).
- A = [[2,1],[1,2]]: char λ² − 4λ + 3 ⇒ A² = 4A − 3I ⇒ **A³ = 13A − 12I**.

**Worked bank anchors (use as templates):**
- A = [[4,1],[2,3]]: trace 7, det 10 ⇒ λ = 2, 5 (EM_02 Q2).
- D = [[3,2],[1,4]]: λ = 2, 5 ⇒ D² has eigenvalues 4 and 25 (EM_02 Q4).
- Eigenvalues 2, 3, −1 ⇒ det = −6, trace = 4, invertible, diagonalizable (3 distinct) (EM_02 Q9).

---

## 6. LU decomposition (GATE syllabus; not in the PW extract — supplement)

**Goal.** Factor A = L·U with L **unit lower-triangular** (1's on the diagonal) and U upper-triangular, using only rowᵢ → rowᵢ − m·rowⱼ operations (no swaps). The multipliers m that do the elimination are *exactly* the sub-diagonal entries of L.

**Doolittle, mechanically:**
1. Use the (1,1) pivot to clear column 1 below it; the ratios used (aᵢ₁/a₁₁) fill L's first column.
2. Move to the next pivot in the updated matrix; clear below it; multipliers fill L's next column. Repeat.
3. The final echelon matrix is U, and **det(A) = product of U's diagonal** (det L = 1).

**Worked example 1 (bank matrix).** A = [[4,3],[6,3]]: multiplier 6/4 = 1.5; U = [[4,3],[0, 3 − 1.5·3]] = [[4,3],[0,−1.5]] ⇒ **U₂₂ = −1.5** (EM_02 Q6); det = 4·(−1.5) = −6 ✓ — matching §1's direct determinant.

**Worked example 2 (3×3):**

$$A = \begin{pmatrix} 2 & 1 & 1 \\ 4 & 3 & 3 \\ 8 & 7 & 9 \end{pmatrix}$$

Eliminate: multipliers 2 and 4 (column 1), then 3 (column 2), giving

$$L = \begin{pmatrix} 1 & 0 & 0 \\ 2 & 1 & 0 \\ 4 & 3 & 1 \end{pmatrix},\qquad U = \begin{pmatrix} 2 & 1 & 1 \\ 0 & 1 & 1 \\ 0 & 0 & 2 \end{pmatrix},\qquad L \cdot U = A\ \checkmark,\quad \det A = 2 \cdot 1 \cdot 2 = 4$$

**Fresh 2×2.** A = [[1,2],[3,8]]: the only multiplier is 3, U = [[1,2],[0,2]] ⇒ **L₂₁ = 3** (practice Q9), det = 1·2 = 2 ✓.

**Why it matters (the standard GATE statements).**
- Solving Ax = b becomes Ly = b (forward substitution), then Ux = y (back substitution) — **O(n²) each** after an O(2n³/3) factorization; each *new* right-hand side costs only O(n²) (EM_02 Q7 (C)).
- det(A) = det(L)·det(U) = product of U's diagonal (EM_02 Q7 (A)).
- When it exists without swaps, the factorization is **unique** for unit-L (EM_02 Q7 (D)).
- **Pivoting caveat:** a no-swap LU does not always exist (a zero — or dangerous near-zero — pivot blocks it); with partial pivoting one computes **PA = LU**, P a permutation matrix. "Every invertible matrix has a no-swap LU" is **false** (EM_02 Q7 (B)).

---

## 7. Trap list (read before the exam)

1. **det(kA) = kⁿ det A** — the exponent n is for the whole matrix; multiplying a single row by k multiplies det by k.
2. **|adj A| = |A|^{n−1}** (n = size): a 3×3 with det 2 has det(adj A) = 2² = 4, not 2. And det(adj(adj A)) = |A|^{(n−1)²}.
3. adj(A) is the **transpose** of the cofactor matrix; A⁻¹ = adj(A)/|A| requires |A| ≠ 0.
4. ρ(A + B) ≤ ρ(A) + ρ(B) can jump to full rank from small ranks; ρ(AB) only drops (≤ min), it never exceeds either factor's rank.
5. Rank = number of **pivots** in echelon form = largest nonzero minor — never "number of nonzero entries".
6. Consistency needs **rank equality with the augmented matrix**; comparing ρ(A) with n alone is only half the story (it decides unique vs infinite *after* consistency).
7. Free parameters = **n − ρ(A)** (n = number of unknowns) — not m − ρ, and not n − rank([A|b]).
8. 0 is an eigenvalue ⟺ det = 0 ⟺ singular; eigenvectors are **nonzero** kernel vectors.
9. Repeated eigenvalue ≠ defective (identity and symmetric matrices are fine); distinct eigenvalues ⇒ diagonalizable, but the converse fails.
10. Aᵀ has the same eigenvalues but generally **different eigenvectors** (PW §2.15.1 (i)).
11. LU: L carries the multipliers with the diagonal forced to 1; U's diagonal is generally ≠ 1. No-swap LU can fail — exactly the bank's True/False test.

---

## 8. Practice questions (fresh — not in the banks)

### Q1 · MCQ · Fundamental

A is a 3×3 matrix with det(A) = 5. The value of det(2A) is

- (A) 10
- (B) 15
- (C) 40
- (D) 80

### Q2 · NAT · Fundamental

The determinant of the matrix

$$A = \begin{pmatrix} 2 & 7 & 1 \\ 0 & 3 & 9 \\ 0 & 0 & -5 \end{pmatrix}$$

is ______.

### Q3 · MSQ · Standard

Which of the following statements hold for all n×n matrices A and B (with the inverse existing wherever it appears)?

- (A) det(AB) = det(A)·det(B)
- (B) det(Aᵀ) = det(A)
- (C) det(A + B) = det(A) + det(B)
- (D) det(A⁻¹) = 1/det(A)

### Q4 · NAT · Standard

The rank of the matrix

$$B = \begin{pmatrix} 1 & 2 & 3 \\ 4 & 5 & 6 \\ 7 & 8 & 9 \end{pmatrix}$$

is ______.

### Q5 · MCQ · Standard

A consistent system Ax = b has 5 unknowns, and rank(A) = rank([A|b]) = 3. The solution set of the system is

- (A) a unique solution
- (B) a 1-parameter family of solutions
- (C) a 2-parameter family of solutions
- (D) a 3-parameter family of solutions

### Q6 · NAT · Moderate

Consider the system

$$x + 2y - z = 3,\qquad 2x - y + 2z = 6,\qquad 3x + y + z = 9.$$

(The third equation is the sum of the first two, so the system is consistent with infinitely many solutions.) The system has **exactly one** solution with z = 0. For that solution, the value of x + y + z is ______.

### Q7 · MSQ · Moderate

A is a 3×3 matrix whose eigenvalues, counted with multiplicity, are 1, 1, 4. Which of the following statements is/are necessarily TRUE?

- (A) det(A) = 4
- (B) trace(A) = 6
- (C) A is diagonalizable
- (D) A − I is singular

### Q8 · NAT · Moderate

For the matrix

$$A = \begin{pmatrix} 2 & 1 \\ 1 & 2 \end{pmatrix}$$

the geometric multiplicity of its eigenvalue 1 (the dimension of the eigenspace {v : Av = v}) is ______.

### Q9 · MCQ · Moderate

In the LU decomposition A = LU of

$$A = \begin{pmatrix} 1 & 2 \\ 3 & 8 \end{pmatrix}$$

with L unit lower-triangular and **no row interchanges**, the entry L₂₁ (the only multiplier used) is

- (A) 1
- (B) 2
- (C) 3
- (D) 8

### Q10 · NAT · Killer

A is a 3×3 matrix with det(A) = 2. The value of det(adj(A)) is ______.

---

## 9. Answer key & full solutions

**Answer key:** 1-C · 2-−30 · 3-A,B,D · 4-2 · 5-C · 6-3 · 7-A,B,D · 8-1 · 9-C · 10-4

**Q1 — C.** det(kA) = kⁿ·det(A) with k = 2, n = 3: 2³·5 = **40**. (A) 10 forgets the exponent; (D) 80 uses 2⁴.

**Q2 — −30.** Upper-triangular determinant = product of the diagonal: 2·3·(−5) = **−30**.

**Q3 — A, B, D.** (A) product rule ✓. (B) transposition invariance ✓. (D) det(A⁻¹)·det(A) = det(I) = 1 ✓. (C) ✘ is the classic falsehood — with A = I₂ and B = diag(1, 0): det(A + B) = 2 but det(A) + det(B) = 1 (verified counterexample).

**Q4 — 2.** row2 − row1 = (3, 3, 3) and row3 − row1 = (6, 6, 6) are proportional, so det = 0; rows 1 and 2 alone are independent ⇒ ρ(B) = **2**.

**Q5 — C.** Consistent with n = 5 unknowns and common rank r = 3 ⇒ solution family has n − r = 5 − 3 = **2 free parameters**.

**Q6 — 3.** Row3 = row1 + row2 (coefficients and right sides: 9 = 3 + 6), so rank(A) = rank([A|b]) = 2 < 3. Reducing the homogeneous part: x = −(3/5)z, y = (4/5)z, so all solutions are (x, y, z) = (3, 0, 0) + t(−3, 4, 5). z = 0 forces t = 0, giving the solution (3, 0, 0): x + y + z = **3**. (Check: 3 = 3 ✓, 6 = 6 ✓, 9 = 9 ✓.)

**Q7 — A, B, D.** (A) product of eigenvalues = 1·1·4 = 4 ✓. (B) sum = 1 + 1 + 4 = 6 ✓. (D) λ = 1 is an eigenvalue ⇒ det(A − I) = 0, i.e. A − I is singular ✓. (C) ✘ is *not* forced: [[1,1,0],[0,1,0],[0,0,4]] has exactly the eigenvalues 1, 1, 4, but rank(A − I) = 2 gives geometric multiplicity 1 < 2 for λ = 1 — defective, hence not diagonalizable (verified).

**Q8 — 1.** A − I = [[1,1],[1,1]] has rank 1, so its kernel has dimension n − ρ = 2 − 1 = **1** (the eigenspace is span{(1, −1)}). Sanity: eigenvalues of A are 1 and 3 (trace 4, det 3 ✓).

**Q9 — C.** Eliminate: row2 → row2 − 3·row1 gives U = [[1,2],[0, 8 − 6]] = [[1,2],[0,2]]; the multiplier is **3** and L = [[1,0],[3,1]]. Check L·U = [[1,2],[3,8]] ✓ and det = 1·2 = 2 = det(A) ✓.

**Q10 — 4.** det(adj A) = det(A)^{n−1} = 2^{3−1} = **4**. (Verified on a concrete det-2 matrix and on 19 random invertible 3×3 matrices: det(adj A) = det(A)² every time.)

---

## 10. Next steps

1. Re-derive §2's property table once per property via cofactor expansion — most are one-liners and the exam loves True/False forms.
2. Memorise the 2×2 eigenvalue shortcut (λ² − tr·λ + det) and the 2×2 inverse recipe; together they save minutes on every LA question.
3. Drill the consistency trichotomy until "compare ranks, then count n − ρ" is automatic.
4. Run banks `EM_01` and `EM_02` cold, then continue with `EM_Notes_02_Calculus.md`.
