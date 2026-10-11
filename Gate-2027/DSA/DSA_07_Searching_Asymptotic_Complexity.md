# Data Structures & Algorithms → Searching & Asymptotic Complexity

**GATE 2027 syllabus:** Algorithms — searching; asymptotic worst-case time and space complexity.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** Binary search worst-case comparisons on n elements = ⌈log₂(n+1)⌉. f(n) = O(g(n)) means f grows no faster than g asymptotically; f(n) = Θ(g(n)) means f and g grow at the same asymptotic rate; f(n) = ω(g(n)) means f grows strictly faster than g. "Space complexity" refers to auxiliary space beyond the input, unless stated otherwise.

---

## Questions

### Q1 · MCQ · Fundamental
The worst-case number of comparisons needed by binary search on a sorted array of 1000 elements is

- (A) 9
- (B) 10
- (C) 999
- (D) 1000

---

### Q2 · NAT · Fundamental
The worst-case number of comparisons needed by binary search on a sorted array of 255 elements is ______.

---

### Q3 · MSQ · Standard
Which of the following pairs (f(n), g(n)) satisfy f(n) = O(g(n))?

- (A) f(n) = n log n, g(n) = n²
- (B) f(n) = n², g(n) = n log n
- (C) f(n) = 2ⁿ, g(n) = n!
- (D) f(n) = √n, g(n) = log n

---

### Q4 · NAT · Standard
A recurrence T(n) = 2T(n/2) + n, with T(1) = 1, is unrolled directly (not via the master theorem shortcut) for n = 64 (a power of 2, dividing exactly at each level). The value of T(64) is ______.

---

### Q5 · MCQ · Standard
For the recurrence T(n) = 4T(n/2) + n, with T(1) = 1, which case of the master theorem applies, and what is the asymptotic solution?

- (A) Case 1 (leaves dominate): T(n) = Θ(n²)
- (B) Case 2 (balanced): T(n) = Θ(n² log n)
- (C) Case 3 (root dominates): T(n) = Θ(n)
- (D) The master theorem does not apply to this recurrence

---

### Q6 · NAT · Moderate
A recurrence T(n) = 3T(n/3) + n², with T(1) = 1, is unrolled directly for n = 81 (a power of 3, dividing exactly at each level). The value of T(81) is ______.

---

### Q7 · MSQ · Moderate
Which of the following statements is/are TRUE?

- (A) An algorithm with worst-case time Θ(n log n) always runs at least as fast as one with worst-case time Θ(n²), for all input sizes n without exception.
- (B) O(1) space complexity means the algorithm's memory use does not grow with the input size (beyond the input itself).
- (C) The recurrence T(n) = T(n/2) + 1 solves to Θ(log n).
- (D) If f(n) = Θ(g(n)), then also f(n) = O(g(n)) and f(n) = Ω(g(n)).

---

### Q8 · NAT · Moderate
A recurrence T(n) = T(n/2) + 1, with T(1) = 1, is unrolled directly for n = 256 (a power of 2). The value of T(256) is ______.

---

### Q9 · MSQ · Hard
Which of the following statements about ternary search (splitting the search range into three parts instead of two, at each step comparing against two midpoints) versus binary search, both on a sorted array, is/are TRUE?

- (A) Ternary search uses more comparisons per level (up to 2) than binary search's 1 comparison per level.
- (B) Ternary search has a smaller asymptotic worst-case comparison count than binary search, i.e., strictly fewer than Θ(log n).
- (C) Both ternary and binary search have Θ(log n) worst-case comparisons, but binary search typically needs fewer total comparisons in practice for a comparison-based search on a general array.
- (D) Ternary search is asymptotically Θ(log₃ n) recursive calls, but each call can cost more comparisons, so the total comparison count is not necessarily better than binary search's.

---

### Q10 · NAT · Killer
A recurrence T(n) = 4T(n/2) + n, with T(1) = 1, is unrolled directly (not via a shortcut formula) for n = 128 (a power of 2). The value of T(128) is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | B | ⌈log₂(1001)⌉ = 10 |
| 2 | 8 | ⌈log₂(256)⌉ = 8 |
| 3 | A, C, D | (B) is false — n² is not O(n log n) |
| 4 | 448 | Verified by direct unrolling: T(1)=1, T(2)=4, T(4)=12, T(8)=32, T(16)=80, T(32)=192, T(64)=448 |
| 5 | A | a=4, b=2: n^(log₂4) = n²; since f(n) = n is polynomially smaller than n², this is Master Theorem Case 1, giving Θ(n²) |
| 6 | 6049 | Verified by direct unrolling of T(n) = 3T(n/3) + n² down to n=1 |
| 7 | B, C, D | (A) is false — asymptotic dominance only holds for sufficiently large n, not "for all n without exception" (small n can go either way) |
| 8 | 9 | Verified by direct unrolling: each halving adds 1, and 256 = 2⁸, so 8 halvings plus the base case |
| 9 | A, C, D | (B) is false — ternary search is still Θ(log n), not asymptotically better than binary search |
| 10 | 32640 | Verified by direct unrolling: T(1)=1, T(2)=6, T(4)=28, T(8)=120, T(16)=496, T(32)=2016, T(64)=8128, T(128)=32640 |
