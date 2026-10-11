# Data Structures & Algorithms → Graph Traversal, Topological Sort & Strongly Connected Components

**GATE 2027 syllabus:** Algorithms — graph traversals.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** Graphs are 0-indexed with vertices visited/explored in increasing numeric order (adjacency lists are processed in ascending order) whenever a tie-break is needed. DFS classifies directed edges as tree, back, forward or cross, using discovery/finish times. A DAG (directed acyclic graph) has at least one valid topological order iff it has no cycle.

---

## Questions

### Q1 · MCQ · Fundamental
Breadth-first search (BFS) from a source s in an unweighted graph computes, for every reachable vertex v,

- (A) the lexicographically smallest path from s to v
- (B) the shortest path (fewest edges) from s to v
- (C) a path from s to v, but not necessarily the shortest one
- (D) all paths from s to v

---

### Q2 · NAT · Fundamental
An undirected graph has 6 vertices {0,...,5} and edges (0,1), (0,2), (1,3), (2,3), (3,4), (4,5). Running BFS from vertex 0, the shortest-path distance (number of edges) to vertex 5 is ______.

---

### Q3 · MSQ · Standard
Which of the following statements about DFS on a **directed** graph is/are TRUE?

- (A) A back edge (from a vertex to one of its ancestors in the DFS tree) indicates the presence of a cycle.
- (B) A directed graph is acyclic (a DAG) if and only if a DFS from every unvisited vertex produces no back edges.
- (C) A forward edge always goes from a vertex to one of its DFS-tree descendants that is not a direct child.
- (D) DFS can produce cross edges only within a single DFS tree, never between two different DFS trees in the DFS forest.

---

### Q4 · NAT · Standard
For the DAG with edges 0→1, 0→2, 1→3, 2→3, the number of distinct valid topological orderings is ______.

---

### Q5 · MCQ · Standard
Topological sort of a DAG can be computed using DFS by

- (A) outputting each vertex when it is first discovered (grey), in that order
- (B) outputting each vertex when it finishes (turns black), then reversing that output order
- (C) running BFS layer by layer and outputting each layer in order
- (D) sorting vertices by their in-degree, smallest first, without needing DFS or BFS at all

---

### Q6 · NAT · Moderate
For the DAG with edges 0→1, 1→2, 2→4, 3→2, the number of distinct valid topological orderings is ______.

---

### Q7 · MSQ · Moderate
Which of the following statements about **strongly connected components (SCCs)** of a directed graph is/are TRUE?

- (A) Every vertex belongs to exactly one SCC.
- (B) Kosaraju's algorithm finds all SCCs using two DFS passes: one on the graph, one on its transpose (reversed-edge) graph.
- (C) Contracting each SCC to a single vertex always yields a DAG (the "condensation" of the graph).
- (D) A directed graph with no cycles has exactly one SCC.

---

### Q8 · NAT · Moderate
A directed graph has 6 vertices {0,...,5} and edges 0→1, 1→2, 2→0, 2→3, 3→4, 4→5, 5→3. Running Kosaraju's algorithm, the number of strongly connected components is ______.

---

### Q9 · MSQ · Hard
Which of the following statements about DFS **edge classification** in an undirected graph is/are TRUE?

- (A) An undirected graph's DFS produces only tree edges and back edges — never forward or cross edges.
- (B) A back edge in an undirected graph's DFS always connects a vertex to one of its DFS-tree ancestors (not a descendant that finished first, as could occur in directed graphs).
- (C) The number of DFS trees in the DFS forest equals the number of connected components of the graph.
- (D) An undirected graph is acyclic (a forest) if and only if its DFS produces no back edges.

---

### Q10 · NAT · Killer
A directed graph has 8 vertices {0,...,7} and edges 0→1, 1→2, 2→0 (a 3-cycle among 0,1,2), 2→3, 3→4, 4→5, 5→3 (a 3-cycle among 3,4,5), 5→6, 6→7 (7 has no outgoing edges). After computing the strongly connected components and contracting each into a single "super-vertex" to form the condensation DAG, the number of distinct topological orderings of this condensation DAG is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | B | Defining property of BFS on unweighted graphs |
| 2 | 3 | 0→1→3→4→5 or 0→2→3→4→5, both length 3 |
| 3 | A, B, C | (D) is false — cross edges can also connect two different DFS trees in the forest |
| 4 | 2 | 0,1,2,3 or 0,2,1,3 |
| 5 | B | The standard DFS-based topological sort construction |
| 6 | 3 | Orderings: 0,1,3,2,4 / 0,3,1,2,4 / 3,0,1,2,4 |
| 7 | A, B, C | (D) is false — a DAG's SCCs are its individual vertices, so it has as many SCCs as vertices, not necessarily one |
| 8 | 3 | SCC {0,1,2}, SCC {3,4,5}, SCC {6} |
| 9 | A, B, C, D | All four are standard, correct facts about DFS on undirected graphs |
| 10 | 1 | The condensation has 4 super-vertices in a single chain: {0,1,2} → {3,4,5} → {6} → {7}, forcing a unique order |
