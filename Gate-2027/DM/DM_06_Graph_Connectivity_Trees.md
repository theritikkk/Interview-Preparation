# Discrete Mathematics → Graphs: Connectivity, Trees & Degree Sequences

**GATE 2027 syllabus:** Engineering Mathematics — Discrete Mathematics: graphs (connectivity).
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** Graphs are simple and undirected unless stated otherwise. K_n is the complete graph, K_{m,n} the complete bipartite graph. Vertices of labelled trees are distinguishable.

---

## Questions

### Q1 · NAT · Fundamental
A simple graph has 12 vertices: 4 vertices of degree 5 and all the others of degree 3. The number of edges is ______.

---

### Q2 · MCQ · Fundamental
Which one of the following degree sequences is realisable by a simple graph on 7 vertices?

- (A) 6, 5, 5, 4, 3, 3, 1
- (B) 5, 5, 4, 3, 2, 2, 1
- (C) 6, 6, 5, 4, 3, 2, 2
- (D) 6, 6, 6, 2, 2, 1, 1

---

### Q3 · NAT · Standard
A tree has 4 vertices of degree 4, 3 vertices of degree 3, 2 vertices of degree 2, and all remaining vertices have degree 1. The number of vertices of degree 1 is ______.

---

### Q4 · MSQ · Standard
Which of the following graphs has an Euler circuit?

- (A) K₇
- (B) K₆
- (C) K_{3,4}
- (D) K_{4,6}

---

### Q5 · NAT · Standard
The number of spanning trees of the graph obtained from K₅ by deleting one edge is ______.

---

### Q6 · NAT · Moderate
The minimum number of edges e such that **every** simple graph on 10 vertices with e edges is connected is ______.

---

### Q7 · MSQ · Moderate
Which of the following statements is/are TRUE?

- (A) Every edge of a tree is a bridge.
- (B) In a connected graph, deleting a vertex of maximum degree never disconnects the graph.
- (C) A connected graph with at least 3 vertices that has a bridge also has a cut vertex.
- (D) A graph with no cut vertex has no bridge.

---

### Q8 · NAT · Moderate
A graph has the vertices 1, 2, …, 60, and two distinct vertices i and j are adjacent if and only if gcd(i, j) > 1. The number of connected components is ______.

---

### Q9 · NAT · Hard
The maximum number of edges in a simple graph on 8 vertices that has **no** Hamiltonian cycle is ______.

---

### Q10 · NAT · Killer
The number of labelled trees on the vertex set {1, 2, …, 7} in which vertex 1 has degree exactly 3 is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | 22 | (4·5 + 8·3) / 2 |
| 2 | B | (A) odd degree sum; (C) fails Erdős–Gallai at k = 3 (17 > 16); (D) fails at k = 3 (18 > 12) |
| 3 | 13 | With L leaves: L + 29 = 2(L + 8) |
| 4 | A, D | All degrees even in K₇ (6) and K_{4,6} (6 and 4) |
| 5 | 75 | 125 spanning trees of K₅; each edge lies in 125·4/10 = 50 of them |
| 6 | 37 | Largest disconnected graph is K₉ plus an isolated vertex: 36 edges |
| 7 | A, C | (B) star centre; (D) K₂ has a bridge but no cut vertex |
| 8 | 9 | Vertex 1 and the seven primes 31…59 are isolated; the rest form one component |
| 9 | 22 | K₇ plus a pendant vertex: C(7,2) + 1 |
| 10 | 2160 | Prüfer sequences of length 5 with the symbol 1 exactly twice: C(5,2)·6³ |
