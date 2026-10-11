# Data Structures & Algorithms → Greedy Algorithms

**GATE 2027 syllabus:** Algorithms — algorithm design techniques: greedy.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** Activity selection: choose the maximum number of non-overlapping activities from given (start, finish) pairs; an activity with start ≥ the finish time of the last chosen activity is compatible. Fractional knapsack allows taking any fraction of an item. 0/1 knapsack (referenced for contrast) requires taking an item whole or not at all. Huffman coding builds an optimal prefix-free binary code by repeatedly merging the two least-frequent nodes.

---

## Questions

### Q1 · MCQ · Fundamental
The greedy strategy for the **activity selection** problem (choosing the maximum number of mutually non-overlapping activities) is to repeatedly pick the next compatible activity with the

- (A) earliest start time
- (B) earliest finish time
- (C) shortest duration
- (D) largest number of overlaps with other activities

---

### Q2 · NAT · Fundamental
Activities with (start, finish) pairs (1,4), (3,5), (0,6), (5,7), (3,9), (5,9), (6,10), (8,11), (8,12), (2,14), (12,16) are given. Using the greedy earliest-finish-time strategy, the maximum number of mutually non-overlapping activities that can be selected is ______.

---

### Q3 · MSQ · Standard
Which of the following statements about the greedy approach to **fractional knapsack** is/are TRUE?

- (A) Sorting items by value-to-weight ratio and greedily taking as much as possible of the highest-ratio item first always gives an optimal solution.
- (B) The same greedy strategy (sort by value-to-weight ratio) is also always optimal for the 0/1 knapsack problem.
- (C) Fractional knapsack can be solved in O(n log n) time (dominated by the sort).
- (D) An exchange argument is the standard way to prove the fractional-knapsack greedy strategy is optimal.

---

### Q4 · NAT · Standard
Items with (weight, value) pairs (10, 60), (20, 100), (30, 120) are available for a **fractional** knapsack of capacity 50. Using the greedy value-to-weight-ratio strategy, the maximum total value achievable is ______.

---

### Q5 · MCQ · Standard
Huffman coding builds an optimal prefix-free code by

- (A) always merging the two nodes with the **largest** frequencies first
- (B) always merging the two nodes with the **smallest** frequencies first
- (C) assigning shorter codes to less frequent symbols
- (D) using a fixed-length code determined by the alphabet size

---

### Q6 · NAT · Moderate
Six symbols have frequencies 45, 13, 12, 16, 9, 5. Using standard Huffman coding, the total number of bits needed to encode a message containing each symbol exactly (its frequency) many times — i.e., Σ(frequency × code length) — is ______.

---

### Q7 · MSQ · Moderate
Which of the following statements about greedy algorithms in general is/are TRUE?

- (A) A greedy algorithm makes a locally optimal choice at each step, without reconsidering previous choices.
- (B) Every problem that can be solved by dynamic programming can also be solved optimally by some greedy algorithm.
- (C) The greedy-choice property and optimal-substructure property are the two conditions typically used to prove a greedy algorithm's correctness.
- (D) Dijkstra's shortest-path algorithm (for non-negative edge weights) is an example of a greedy algorithm.

---

### Q8 · NAT · Moderate
Five jobs have (deadline, profit) pairs: job a (2, 100), job b (1, 19), job c (2, 27), job d (1, 25), job e (3, 15). Each job takes 1 unit of time, and at most one job can be scheduled in any time slot, with a job only earning its profit if scheduled at or before its deadline. Using the standard greedy strategy (sort jobs by decreasing profit, schedule each as late as possible before its deadline), the maximum total profit achievable is ______.

---

### Q9 · MSQ · Hard
Which of the following statements about **Huffman coding** is/are TRUE?

- (A) The resulting code is always a prefix-free code (no codeword is a prefix of another).
- (B) If all n symbols have equal frequency and n is a power of 2, every codeword has the same length log₂ n — the Huffman tree is perfectly balanced in this case.
- (C) The Huffman tree is unique for a given set of frequencies — there is exactly one possible optimal tree structure.
- (D) Huffman coding achieves the information-theoretic entropy lower bound exactly, for every possible frequency distribution.

---

### Q10 · NAT · Killer
Items with (weight, value) pairs (10, 60), (20, 100), (30, 120) are available for a **fractional** knapsack of capacity 50, exactly as in Q4. Separately, the same three items are considered for a **0/1 knapsack** of the same capacity 50 (each item taken whole or not at all; the optimal 0/1 solution for this classic instance is known to take items (20,100) and (30,120) for a total value of 220). The difference (fractional-knapsack optimal value) − (0/1-knapsack optimal value) is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | B | Earliest finish time is the classic optimal greedy rule |
| 2 | 4 | Verified selection: (1,4), (5,7), (8,11), (12,16) |
| 3 | A, C, D | (B) is false — the ratio-greedy rule is not generally optimal for 0/1 knapsack |
| 4 | 240 | Take all of items 1 and 2 (160), then 20/30 of item 3 (80): 160 + 80 |
| 5 | B | Merging smallest frequencies first builds the optimal tree bottom-up |
| 6 | 224 | Verified Huffman construction: codeword lengths 1,3,3,3,4,4 for frequencies 45,16,13,12,9,5 respectively |
| 7 | A, C, D | (B) is false — many DP problems (e.g., 0/1 knapsack) have no optimal greedy solution |
| 8 | 142 | Verified: jobs c, a, e are scheduled (deadlines 2, 2, 3), giving 27 + 100 + 15 |
| 9 | A, B | (C) is false — ties during merging can produce multiple optimal trees with the same total cost; (D) is false — Huffman coding is optimal among prefix codes but does not always exactly hit the entropy bound |
| 10 | 20 | Fractional optimum is 240 (from Q4); 0/1 optimum is 220; difference 240 − 220 |
