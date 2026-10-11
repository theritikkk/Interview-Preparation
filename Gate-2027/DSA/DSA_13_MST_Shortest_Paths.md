# Data Structures & Algorithms → Minimum Spanning Trees & Shortest Paths

**GATE 2027 syllabus:** Algorithms — minimum spanning trees, shortest paths.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** Graphs are connected and undirected for MST questions unless stated otherwise. Kruskal's algorithm processes edges in non-decreasing weight order, adding an edge only if it doesn't form a cycle (using union-find), breaking ties by the order edges are listed. Prim's algorithm grows a tree from a start vertex, always adding the cheapest edge crossing the current cut. Dijkstra's algorithm requires all edge weights to be non-negative. Bellman-Ford handles negative edge weights and detects negative-weight cycles reachable from the source.

---

## Questions

### Q1 · MCQ · Fundamental
Which one of the following algorithms correctly computes shortest paths from a single source when the graph has some **negative** edge weights but no negative cycle?

- (A) Dijkstra's algorithm
- (B) Bellman-Ford algorithm
- (C) Both work equally correctly
- (D) Neither works — shortest paths are undefined with negative edges

---

### Q2 · NAT · Fundamental
The classic 9-vertex weighted graph (vertices 0–8) below has edges: 0-1(4), 0-7(8), 1-2(8), 1-7(11), 2-3(7), 2-5(4), 2-8(2), 3-4(9), 3-5(14), 4-5(10), 5-6(2), 6-7(1), 6-8(6), 7-8(7). Using Kruskal's or Prim's algorithm, the total weight of the minimum spanning tree is ______.

---

### Q3 · MSQ · Standard
Which of the following statements about MST algorithms is/are TRUE?

- (A) If all edge weights in a connected graph are distinct, the MST is unique.
- (B) Kruskal's algorithm runs in O(E log E) time, dominated by sorting the edges.
- (C) Prim's algorithm, implemented with a binary heap, runs in O(E log V) time.
- (D) Both Kruskal's and Prim's algorithms are examples of greedy algorithms.

---

### Q4 · NAT · Standard
For the graph of Q2, running Dijkstra's algorithm from vertex 0, the shortest-path distance to vertex 4 is ______.

---

### Q5 · MCQ · Standard
A graph has 4 vertices {0,1,2,3} and edges (weight, u, v): (1,0,1), (1,1,2), (2,2,3), (1,0,2), (2,1,3), (1,0,3). The **number of distinct minimum spanning trees** (spanning trees achieving the minimum possible total weight) this graph has is

- (A) 1
- (B) 2
- (C) 3
- (D) 4

---

### Q6 · NAT · Moderate
A directed graph has 4 vertices {0,1,2,3} and edges (from, to, weight): (0,1,4), (0,2,5), (1,3,3), (2,1,−3), (2,3,9). Running **Dijkstra's algorithm** from vertex 0 (even though it has a negative edge, applying the algorithm mechanically as usual), the computed distance to vertex 3 is ______  (this may be an incorrect answer due to the negative edge — that is the point of this question).

---

### Q7 · MSQ · Moderate
Using the same graph as Q6, which of the following statements is/are TRUE?

- (A) The value Dijkstra's algorithm computes for the distance to vertex 3 is strictly greater than the true shortest-path distance.
- (B) The Bellman-Ford algorithm, run on the same graph from vertex 0, computes the correct shortest-path distance to vertex 3.
- (C) The graph contains a negative-weight cycle.
- (D) Dijkstra's algorithm fails here because it finalizes each vertex's distance permanently the first time it is settled, and never revisits it — but a later-discovered negative edge can still improve that distance.

---

### Q8 · NAT · Moderate
Using the same graph as Q6, the **correct** shortest-path distance from vertex 0 to vertex 3 (computed by Bellman-Ford) is ______.

---

### Q9 · MSQ · Hard
Which of the following statements about **Bellman-Ford** is/are TRUE?

- (A) It performs V − 1 rounds of relaxing every edge, which suffices because any shortest path (if no negative cycle exists) has at most V − 1 edges.
- (B) An optional extra (V-th) round can detect whether a negative-weight cycle reachable from the source exists (if any edge can still be relaxed).
- (C) Its time complexity is O(VE).
- (D) It can be used to detect a negative cycle anywhere in the graph, even one not reachable from the given source vertex, without any modification.

---

### Q10 · NAT · Killer
A directed graph has 5 vertices {0,1,2,3,4} and edges (from, to, weight): (0,1,6), (0,2,7), (1,2,8), (1,3,5), (1,4,−4), (2,3,−3), (2,4,9), (3,1,−2), (4,0,2), (4,3,7). Using Bellman-Ford from source 0, the shortest-path distance to vertex 3 is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | B | Dijkstra requires non-negative weights to be correct |
| 2 | 37 | Verified via Kruskal's algorithm on this classic graph |
| 3 | A, B, C, D | All four are standard, correct facts |
| 4 | 21 | Verified via Dijkstra: shortest path is 0→7→6→5→4, with weights 8 + 1 + 2 + 10 |
| 5 | C | Verified by brute force: minimum weight 3, achieved by exactly 3 different spanning trees |
| 6 | 7 | Verified: Dijkstra computes dist[3] = 7 via the path 0→1→3 (4+3), never revisiting vertex 1 after a cheaper path through 2 is later found |
| 7 | A, B, D | (C) is false — there is no cycle at all in this DAG-like graph, let alone a negative one |
| 8 | 5 | True shortest path is 0→2→1→3: 5 + (−3) + 3 |
| 9 | A, B, C | (D) is false — standard Bellman-Ford only detects negative cycles reachable from the source |
| 10 | 4 | Verified via Bellman-Ford: shortest path is 0→2→3, with weight 7 + (−3) |
