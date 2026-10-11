# Data Structures & Algorithms → Divide and Conquer

**GATE 2027 syllabus:** Algorithms — algorithm design techniques: divide-and-conquer.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** Divide-and-conquer recurrences are unrolled directly when a question asks for an exact value. Binary search variants below operate on a sorted (non-decreasing) array. Strassen's algorithm multiplies two n×n matrices using 7 recursive multiplications of (n/2)×(n/2) submatrices plus O(n²) additions.

---

## Questions

### Q1 · MCQ · Fundamental
The three standard steps of a divide-and-conquer algorithm are

- (A) sort, search, merge
- (B) divide, conquer (recurse), combine
- (C) partition, recurse, backtrack
- (D) initialize, iterate, terminate

---

### Q2 · NAT · Fundamental
Strassen's algorithm multiplies two 64×64 matrices using ______ recursive scalar-submatrix multiplications in total (i.e., the total count of the smallest-level 1×1 multiplications performed, following the recurrence M(n) = 7M(n/2), M(1) = 1).

---

### Q3 · MSQ · Standard
Which of the following statements about divide-and-conquer algorithms is/are TRUE?

- (A) Merge sort is a divide-and-conquer algorithm with recurrence T(n) = 2T(n/2) + O(n).
- (B) Binary search is a divide-and-conquer algorithm with recurrence T(n) = T(n/2) + O(1).
- (C) Strassen's algorithm multiplies two n×n matrices in O(n^log₂7) ≈ O(n^2.81) time, asymptotically better than the naive O(n³).
- (D) Every divide-and-conquer algorithm necessarily runs faster than the corresponding brute-force algorithm for all input sizes.

---

### Q4 · NAT · Standard
A sorted array (non-decreasing, with duplicates) is [1, 2, 2, 2, 3, 3, 4, 5, 5, 5, 5, 6] (0-indexed, 12 elements). Using a divide-and-conquer (binary-search-based) algorithm to find the **first** and **last** index at which the value 5 occurs, the number of occurrences of 5 is ______.

---

### Q5 · MCQ · Standard
The **closest pair of points** problem in the plane (n points) can be solved by divide-and-conquer in

- (A) O(n)
- (B) O(n log n)
- (C) O(n²)
- (D) O(n² log n)

---

### Q6 · NAT · Moderate
The closest-pair divide-and-conquer algorithm has recurrence T(n) = 2T(n/2) + O(n) (the O(n) term covers the "merge step" scan of the strip near the dividing line), with T(n) = 1 for n ≤ 3. Unrolling this recurrence directly for n = 64 (assuming exact halving throughout), the value of T(64) is ______.

---

### Q7 · MSQ · Moderate
Which of the following statements about the **quickselect** algorithm (finding the k-th smallest element using a quicksort-like partition, recursing into only one side) is/are TRUE?

- (A) Its worst-case time complexity is O(n²), same as quicksort's worst case.
- (B) Its expected (average-case) time complexity with a random pivot is O(n), not O(n log n).
- (C) Using the deterministic median-of-medians pivot-selection strategy gives worst-case O(n) time.
- (D) Quickselect always requires fully sorting the array first.

---

### Q8 · NAT · Moderate
A divide-and-conquer algorithm counts inversions in an array while merge-sorting it (an "inversion" is a pair i < j with a[i] > a[j]; the merge step counts, for each element taken from the right half, the number of remaining elements in the left half — since all of them are larger and appear earlier). Applied to the array [38, 27, 43, 3, 9, 82, 10], the number of inversions counted is ______.

---

### Q9 · MSQ · Hard
Which of the following statements about the **master theorem** (for recurrences of the form T(n) = aT(n/b) + f(n), a ≥ 1, b > 1) is/are TRUE?

- (A) It cannot be applied directly when f(n) is not a polynomial (e.g., f(n) = n / log n), even though such recurrences can still sometimes be solved by other methods.
- (B) The three cases are determined by comparing f(n) to n^(log_b a).
- (C) It can be applied to recurrences where the subproblem sizes are unequal, e.g., T(n) = T(n/3) + T(2n/3) + n, exactly as stated (no modification needed).
- (D) Case 2 of the master theorem (f(n) = Θ(n^(log_b a))) gives the solution T(n) = Θ(n^(log_b a) · log n).

---

### Q10 · NAT · Killer
Strassen's algorithm is used to multiply two n×n matrices, but for this question, assume the **base case** is n = 4 (not n = 1) — i.e., 4×4 matrix multiplication is done directly using the naive method (4³ = 64 scalar multiplications), and Strassen's 7-way recursive split is used only for larger sizes, down to that base case. Using this modified scheme, the total number of scalar multiplications performed while multiplying two 64×64 matrices (using the recurrence M(n) = 7M(n/2) for n > 4, and M(4) = 64) is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | B | The defining three-step structure |
| 2 | 117649 | 7⁶ (six halvings from 64 down to 1) |
| 3 | A, B, C | (D) is false — divide-and-conquer overhead can make it slower for small n in practice |
| 4 | 4 | Indices 7 to 10 hold the value 5 |
| 5 | B | The classic divide-and-conquer closest-pair algorithm |
| 6 | 352 | Verified by direct unrolling: T(4)=6, T(8)=20, T(16)=56, T(32)=144, T(64)=352 |
| 7 | A, B, C | (D) is false — that is exactly what quickselect avoids doing |
| 8 | 11 | Verified via the merge-and-count procedure during merge sort |
| 9 | A, B, D | (C) is false — the master theorem as originally stated requires equal-sized subproblems; unequal splits like this need the Akra–Bazzi method or other techniques |
| 10 | 175616 | 64 → 32 → 16 → 8 → 4 is 4 halvings; M(4)=64, so total = 7⁴ × 64 = 2401 × 64 |
