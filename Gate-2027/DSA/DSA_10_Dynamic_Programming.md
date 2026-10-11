# Data Structures & Algorithms → Dynamic Programming

**GATE 2027 syllabus:** Algorithms — algorithm design techniques: dynamic programming.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** LIS = Longest Increasing Subsequence (strictly increasing). LCS = Longest Common Subsequence. Edit distance uses unit cost for insertion, deletion and substitution (Levenshtein distance). 0/1 knapsack: each item taken whole or not at all, maximizing value subject to a weight capacity.

---

## Questions

### Q1 · MCQ · Fundamental
Dynamic programming is applicable to a problem when it exhibits

- (A) greedy-choice property only
- (B) optimal substructure and overlapping subproblems
- (C) a divide-and-conquer structure with non-overlapping subproblems
- (D) NP-completeness

---

### Q2 · NAT · Fundamental
The length of the longest increasing subsequence of [10, 9, 2, 5, 3, 7, 101, 18] is ______.

---

### Q3 · MSQ · Standard
Which of the following statements about the standard O(n²) LIS dynamic-programming algorithm (dp[i] = length of the LIS ending exactly at index i) is/are TRUE?

- (A) dp[i] = 1 + max(dp[j]) over all j < i with a[j] < a[i], or 1 if no such j exists.
- (B) The final answer is max(dp[i]) over all i, not necessarily dp[n−1].
- (C) There is an O(n log n) algorithm for LIS using binary search on a auxiliary "tails" array.
- (D) The LIS problem can be solved by first sorting the array, then no further work is needed.

---

### Q4 · NAT · Standard
The length of the longest common subsequence (LCS) of "ABCBDAB" and "BDCABA" is ______.

---

### Q5 · MCQ · Standard
The standard LCS dynamic-programming table for two strings of lengths m and n has dimensions (m+1) × (n+1) and is filled using the recurrence: dp[i][j] = dp[i−1][j−1] + 1 if the characters match, otherwise dp[i][j] = max(dp[i−1][j], dp[i][j−1]). What is the time complexity of filling this table?

- (A) O(m + n)
- (B) O(mn)
- (C) O(m log n)
- (D) O(2^min(m,n))

---

### Q6 · NAT · Moderate
The edit distance (unit-cost insertion, deletion, substitution) between "SUNDAY" and "SATURDAY" is ______.

---

### Q7 · MSQ · Moderate
Which of the following statements about the 0/1 knapsack DP algorithm (table dp[i][w] = best value using the first i items with capacity w) is/are TRUE?

- (A) Its time complexity is O(nW), where n is the number of items and W is the capacity — this is pseudo-polynomial, not polynomial in the input size (which is O(n log W) bits for W).
- (B) The recurrence is dp[i][w] = max(dp[i−1][w], dp[i−1][w − weight[i]] + value[i]) when weight[i] ≤ w, else dp[i][w] = dp[i−1][w].
- (C) The 0/1 knapsack problem is NP-hard in general, but this DP gives an exact solution in pseudo-polynomial time.
- (D) The same DP table formulation directly works, unmodified, for the fractional knapsack variant too.

---

### Q8 · NAT · Moderate
Items with (weight, value) pairs (10, 60), (20, 100), (30, 120) are available for a **0/1** knapsack of capacity 50. Using the standard DP table, the maximum achievable value is ______.

---

### Q9 · MSQ · Hard
Which of the following statements about **matrix chain multiplication** (finding the optimal parenthesization to minimize scalar multiplications) is/are TRUE?

- (A) The DP recurrence considers every possible split point k for each subrange (i, j), taking O(n) time per subrange, giving O(n³) total time for n matrices.
- (B) A greedy strategy (always multiply the two matrices giving the smallest immediate product cost) always finds the optimal parenthesization.
- (C) The number of distinct ways to parenthesize a product of n matrices is the Catalan number C(n−1).
- (D) The problem has optimal substructure: an optimal parenthesization of the whole chain contains optimal parenthesizations of its sub-chains.

---

### Q10 · NAT · Killer
Matrices A₁, A₂, A₃, A₄ have dimensions 40×20, 20×30, 30×10, 10×30 respectively (i.e. the dimension sequence is 40, 20, 30, 10, 30). Using the standard matrix-chain-multiplication DP, the minimum possible number of scalar multiplications to compute A₁A₂A₃A₄ is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | B | The two defining properties of DP-solvable problems |
| 2 | 4 | e.g. 2, 5, 7, 101 (or 2, 3, 7, 101, or 2, 3, 7, 18) |
| 3 | A, B, C | (D) is false — sorting destroys the notion of "subsequence" of the original array |
| 4 | 4 | e.g. "BCBA" or "BDAB" |
| 5 | B | Each of the (m+1)(n+1) cells takes O(1) work |
| 6 | 3 | Verified by DP table computation |
| 7 | A, B, C | (D) is false — fractional knapsack is solved greedily, not with this integer DP table |
| 8 | 220 | Verified: take items (20,100) and (30,120), skip (10,60): 100 + 120 |
| 9 | A, C, D | (B) is false — the greedy "cheapest immediate product" rule is not guaranteed optimal for matrix-chain multiplication |
| 10 | 26000 | Verified via the DP table: optimal split gives 26,000 scalar multiplications |
