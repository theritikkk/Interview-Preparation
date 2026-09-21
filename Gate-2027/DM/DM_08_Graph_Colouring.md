# Discrete Mathematics → Graphs: Colouring

**GATE 2027 syllabus:** Engineering Mathematics — Discrete Mathematics: graphs (colouring).
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** A proper colouring gives adjacent vertices different colours. χ(G) is the chromatic number, ω(G) the clique number, Δ(G) the maximum degree. A proper edge colouring gives edges sharing a vertex different colours.

---

## Questions

### Q1 · MCQ · Fundamental
The wheel graph W₇ consists of a hub vertex joined to every vertex of a 7-cycle. Its chromatic number is

- (A) 2
- (B) 3
- (C) 4
- (D) 5

---

### Q2 · NAT · Fundamental
The number of proper colourings of the 5-cycle C₅ with 3 available colours is ______.

---

### Q3 · NAT · Standard
Let G be K₄ with one edge removed. The number of proper colourings of G with 4 available colours is ______.

---

### Q4 · MSQ · Standard
Which of the following statements is/are TRUE?

- (A) A graph is bipartite if and only if it contains no odd cycle.
- (B) Every planar graph is 4-colourable.
- (C) For every graph G, χ(G) ≥ ω(G).
- (D) For every connected graph G, χ(G) ≤ Δ(G).

---

### Q5 · NAT · Standard
The chromatic number of the complement of the 7-cycle C₇ is ______.

---

### Q6 · NAT · Moderate
The minimum number of colours needed to properly colour the **edges** of K₉ is ______.

---

### Q7 · MSQ · Moderate
Which of the following statements is/are TRUE?

- (A) For every vertex ordering, greedy colouring uses at most Δ(G) + 1 colours.
- (B) There are graphs and vertex orderings for which greedy colouring uses more than χ(G) colours.
- (C) For every graph and every vertex ordering, greedy colouring uses exactly χ(G) colours.
- (D) If χ(G) = k, then G contains K_k as a subgraph.

---

### Q8 · NAT · Moderate
The path P₅ (5 vertices, 4 edges) is to be properly coloured using **exactly 3** colours chosen from a palette of 4 colours (all 3 chosen colours must appear). The number of such colourings is ______.

---

### Q9 · NAT · Hard
The Kneser graph K(6, 2) has the 15 two-element subsets of {1, …, 6} as vertices, two vertices being adjacent if and only if the subsets are disjoint. Its chromatic number is ______.

---

### Q10 · NAT · Killer
The number of proper colourings of the Petersen graph with 3 colours is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | C | Hub needs its own colour; odd cycle needs 3 more |
| 2 | 30 | (k − 1)⁵ − (k − 1) at k = 3: 32 − 2 |
| 3 | 48 | k(k − 1)(k − 2)² at k = 4 |
| 4 | A, B, C | (D) fails for K_n and odd cycles (Brooks) |
| 5 | 4 | ω = α(C₇) = 3, and χ = clique cover number of C₇ = ⌈7/2⌉ = 4 |
| 6 | 9 | Odd n: n colours (each colour class has at most (n − 1)/2 edges) |
| 7 | A, B | (D) fails for odd cycles of length ≥ 5 and other triangle-free graphs of χ ≥ 3 |
| 8 | 168 | C(4,3) · (3·2⁴ − 3·2) = 4 · 42 |
| 9 | 4 | Independent sets have at most 5 vertices (a star); three colour classes cannot cover all 15 vertices |
| 10 | 120 | Enumeration: the chromatic polynomial of the Petersen graph at 3 |
