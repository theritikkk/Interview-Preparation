# Discrete Mathematics → Graphs: Matching

**GATE 2027 syllabus:** Engineering Mathematics — Discrete Mathematics: graphs (matching).
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** A matching is a set of pairwise vertex-disjoint edges. ν(G) is the size of a maximum matching. N(S) is the set of neighbours of S.

---

## Questions

### Q1 · NAT · Fundamental
The number of perfect matchings in K₆ is ______.

---

### Q2 · MCQ · Fundamental
A bipartite graph has parts X = {1, 2, 3, 4} and Y = {a, b, c, d} with neighbourhoods N(1) = {a, b}, N(2) = {b}, N(3) = {a, b}, N(4) = {c, d}. Which one of the following subsets S of X violates Hall's condition |N(S)| ≥ |S|?

- (A) {1, 2}
- (B) {1, 3}
- (C) {1, 2, 3}
- (D) {2, 3, 4}

---

### Q3 · NAT · Standard
The number of perfect matchings in the 3-dimensional hypercube Q₃ (8 vertices, 12 edges) is ______.

---

### Q4 · MSQ · Standard
Which of the following statements is/are TRUE?

- (A) Every k-regular bipartite graph (k ≥ 1) has a perfect matching.
- (B) A graph that has a perfect matching has an even number of vertices.
- (C) A tree has at most one perfect matching.
- (D) Every 3-regular graph has a perfect matching.

---

### Q5 · NAT · Standard
A bipartite graph has left vertices 1–6 and right vertices a–d with N(1) = N(2) = N(3) = {a, b}, N(4) = {c}, N(5) = {c, d}, N(6) = {d}. The size of a maximum matching is ______.

---

### Q6 · NAT · Moderate
A 2 × 6 board of squares is to be covered exactly by 1 × 2 dominoes (each covering two adjacent squares) without overlap. The number of ways is ______.

---

### Q7 · MSQ · Moderate
Let G be a bipartite graph with n vertices and no isolated vertices, and let ν be the size of a maximum matching. Which of the following is/are TRUE?

- (A) The minimum size of a vertex cover is ν.
- (B) The maximum size of an independent set is n − ν.
- (C) The minimum size of an edge cover is n − ν.
- (D) The minimum size of an edge cover is ν.

---

### Q8 · NAT · Moderate
The total number of matchings (including the empty matching) in K₅ is ______.

---

### Q9 · NAT · Hard
A graph has a hub vertex v and three vertex-disjoint triangles; v is adjacent to exactly one vertex of each triangle (10 vertices in total). The size of a maximum matching is ______.

---

### Q10 · NAT · Killer
The number of perfect matchings in the Petersen graph is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | 15 | 5 · 3 · 1 |
| 2 | C | N({1, 2, 3}) = {a, b} has size 2 < 3 |
| 3 | 9 | Enumeration: 3 matchings use one edge direction only, 6 use two directions |
| 4 | A, B, C | (D) fails for a cubic graph with a bridge |
| 5 | 4 | {1, 2, 3} can use only a, b (2 edges); 4 → c, 5 or 6 → d |
| 6 | 13 | Fibonacci: tilings of 2 × n are F(n+1) |
| 7 | A, B, C | König and Gallai; (D) holds only if a perfect matching exists |
| 8 | 26 | Involutions on 5 points |
| 9 | 4 | Removing v leaves 3 odd components, so ν ≤ (10 − 2)/2 = 4, and 4 is attained |
| 10 | 6 | Enumeration over the 15 edges of the Petersen graph |
