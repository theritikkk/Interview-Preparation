# DSA Notes 07 — Graph Algorithms: Traversal, Topological Sort, MST & Shortest Paths (High-Yield Deep Dive)

> **Why this file exists:** "Graph Algorithms" is the third video topic for Algorithms — and the most reliably scoring one, because every algorithm reduces to a **trace** (run it on a small graph and read numbers off) or a **one-line complexity**. GATE asks: BFS/DFS behaviour and orders, topological-order counting, SCC counts + condensation structure, MST weight/count with tie-breaking traps, and Dijkstra/Bellman-Ford traces including the negative-edge failure. Every numeric answer below was **verified by executing the algorithm** (BFS, exhaustive topological enumeration, Kruskal, Bellman-Ford pass simulation, Dijkstra, Kosaraju).
>
> **Companion question banks:** `../DSA_12_Graph_Traversal_Topological_Sort.md`, `../DSA_13_MST_Shortest_Paths.md`. PW source: `../Reference/PW_Subject_Handbooks/PW_Algorithms.txt` (§3.8 representation & spanning trees, §3.8.5 Prim, §3.8.6 Kruskal, §3.9.1 Dijkstra, §3.9.2 Bellman-Ford).

---

## 0. The mental model: graphs have *no* universal "best" — only fit-to-shape

```
   question wants…                use…                       cost (adjacency list)
   ─────────────────────────────────────────────────────────────────────────────
   fewest edges path (unit)       BFS                        Θ(V+E)
   any path / structure / cycles  DFS                        Θ(V+E)
   a valid build order (DAG)      Kahn / DFS finish times    Θ(V+E)
   mutual reachability groups     Kosaraju / Tarjan          Θ(V+E)
   cheapest connection (all V)    Prim / Kruskal             O(E log V) / O(E log E)
   cheapest routes (1 source)     Dijkstra (w ≥ 0)           O((V+E) log V)
                                  Bellman-Ford (negatives)   O(VE)
   all pairs                      Floyd–Warshall             O(V³)
   DAG one-source (even neg.)     relax in topological order Θ(V+E)
```

**Rule #1:** decide the algorithm from the graph's *allowances* (weights? negative? DAG? unweighted?) and the *question asked* (fewest edges vs cheapest vs just traversal). Half the MCQs are exactly this table.

---

## 1. Graph micro-facts GATE assumes you know

- **Handshake lemma:** Σ deg(v) = 2m ⇒ the number of odd-degree vertices is **even**.
- **Max edges:** undirected simple graph n(n−1)/2; directed arcs n(n−1).
- **Connected minimum:** n−1 edges (a tree). Tree ⇔ connected + acyclic ⇔ connected with exactly n−1 edges.
- **Guaranteed-connection fact:** with (n−1)(n−2)/2 + 1 edges a simple graph **must** be connected *(max edges while disconnected = K_{n−1} + isolated vertex)*.
- **Cayley:** the number of spanning trees of Kₙ = n^{n−2}.
- **Bipartite:** 2-colourable ⇔ no odd cycle; detectable by BFS colouring in Θ(V+E).

### Representations — the table behind easy marks (PW §3.8)

| Operation | Adjacency matrix | Adjacency list |
|---|---|---|
| Space | Θ(V²) — *always* | Θ(V + E) |
| Test edge (u,v) | Θ(1) | best O(1), worst Θ(deg u) |
| Iterate neighbours of u | Θ(V) | Θ(deg u) |
| BFS / DFS | Θ(V²) | Θ(V + E) |
| Add edge | Θ(1) | Θ(1) |

**Trap:** "adjacency-list edge testing is O(1)" is *false in the worst case* — you may scan all of u's list.

---

## 2. BFS — the layered machine

```
   queue ← {s};  d[s] = 0
   pop u → for each neighbour v with d[v] unset: d[v] = d[u] + 1; push v
```

- Visits vertices in **non-decreasing order of distance** from s; when v is popped, d[v] is final.
- With **unweighted** edges (or all weights 1), d[v] = true shortest-path distance (fewest edges).
- Θ(V+E) with adjacency lists; Θ(V²) with a matrix.
- Uses: shortest unweighted paths, connected components, bipartite check, multi-source BFS.

**Worked trace — graph: vertices {0…6}, edges 0-1, 0-2, 1-4, 2-4, 4-5, 3-5, 5-6, source 0:**

| pop | new distances assigned |
|---|---|
| 0 | d(1)=1, d(2)=1 |
| 1 | d(4)=2 |
| 2 | (nothing new — 4 already 2) |
| 4 | d(5)=3 |
| 5 | d(3)=4, d(6)=4 |
| 3, 6 | nothing new |

**d = [0, 1, 1, 4, 2, 3, 4]** — note vertices 3 and 6 hang two hops past 5.

---

## 3. DFS — structure, edge classes, and what back edges mean

Recursive/stack version with discovery time d[u] and finish time f[u] (clocks 1…2V).

**Directed graph — four edge types** (u→v):
- **Tree:** v first discovered through u.
- **Forward:** u ancestor of v, not the tree edge (d[u] < d[v] < f[v] < f[u]).
- **Back:** v is an ancestor of u (d[v] < d[u] < f[u] < f[v]) ⇒ **cycle**. A digraph is acyclic ⇔ some DFS has no back edges (⇔ every DFS does, for *that* graph: a DAG can never produce a back edge).
- **Cross:** everything else (between subtrees or trees).

**Undirected graph — only tree + back edges** (there are *no* cross edges; a non-tree edge always reaches an ancestor). Cross edge in undirected DFS = classic false option.

**Complexity:** Θ(V+E) lists / Θ(V²) matrix; stack depth Θ(V).

**Applications:** cycle detection, topological sort (reverse finish order), SCC, bridges/articulation points (low-link), maze/path existence.

---

## 4. Topological sort — build orders and how to *count* them

Definitions: exists **iff the digraph is a DAG**; order = every edge u→v has u before v.

- **Kahn's algorithm:** repeatedly output a vertex with in-degree 0 (queue), decrement its out-neighbours — Θ(V+E). Cycle ⇔ some vertex never reaches in-degree 0.
- **DFS method:** output in **decreasing finish time** (equivalently: push on finish, reverse at the end).
- **Uniqueness:** the order is unique ⇔ at every step exactly one candidate exists ⇔ the DAG contains a Hamiltonian path.
- **Counting orders:** think "at each position, which zero-indegree vertex?" and recurse — count = sum over choices. Use this method:

```
   DAG: 0→1, 0→2, 1→2
   position 1: {0}
   position 2: {1}      (0 removed ⇒ 1 has indeg 0; 2 still needs 1)
   position 3: {2}
   → exactly 1 order: [0,1,2]
```

**Worked bigger example (verified by exhaustive enumeration) — DAG 0→2, 1→2, 1→3, 2→4, 3→4, 3→5:**
first position is 0 or 1. First = 0 ⇒ 5 orders (branch on 2 vs 3 choices); first = 1 ⇒ 9 orders (four sub-branches). **Total 14 orders.**

### Strongly connected components (SCC)

- Two vertices are in the same SCC iff **mutually reachable**. Every digraph = DAG of SCCs.
- **Kosaraju:** (1) DFS on G, order vertices by **decreasing finish time**; (2) DFS on the transpose G^T processing vertices in that order — each DFS tree is exactly one SCC. Θ(V+E).
- **Tarjan:** single DFS, low-link values, stack of active vertices. Θ(V+E).
- **Condensation:** contract each SCC to a super-vertex — the result is **always a DAG** (that's why topological order always exists there).

**Worked example:** edges 0→1, 1→0, 1→2, 2→3, 3→2, 3→4, 4→5, 5→6, 6→5.

```
   SCCs:  {0,1}   {2,3}   {4}⁰   {5,6}          → 4 SCCs
   condensation:  {0,1} → {2,3} → {4} → {5,6}    → 3 edges (a chain, hence a DAG)
```

---

## 5. MST — cheapest way to connect everything

**Cut property (the theorem behind both algorithms):** for *any* cut (S, V∖S), a minimum-weight edge crossing the cut belongs to **at least one** MST.
**Cycle property:** the **unique heaviest** edge of any cycle belongs to **no** MST.

| | Prim | Kruskal |
|---|---|---|
| Strategy | grow one tree from any start vertex | sort edges, accept if it merges two components |
| Machinery | priority queue of crossing edges | union–find |
| Grows | a single connected tree that swallows vertices one by one | a forest that gradually merges into one tree |
| Complexity | O(E log V) heap + list; **O(V²)** with matrix (dense-friendly) | O(E log E) = O(E log V) (sorting dominates); already-sorted edges Θ(E + V) |
| Ties | any tie choice still yields *an* MST | same — **never fails** on ties |

**Facts GATE counts on:**
- MST has exactly **n−1 edges** for connected n-vertex graphs; its *weight* is unique even when the MST itself isn't.
- All weights distinct ⇒ MST is **unique**. Ties ⇒ possibly many MSTs, all with the same weight.
- All weights equal w ⇒ every spanning tree is an MST, weight (n−1)·w.
- MST ≠ shortest-path tree! Classic witness — triangle: 0-1 (1), 0-2 (1), 1-2 (0.5): MST = {0-1, 1-2} (weight 1.5); shortest-path tree from 0 = {0-1, 0-2} (weight 2).

**Worked trace (verified) — Kruskal on this graph (vertices 0–4):**

```
   edges:  0-1 (2)   1-2 (3)   0-3 (6)   1-4 (5)   2-4 (7)   1-3 (8)   3-4 (9)

   sorted: 2(0-1) ✔  3(1-2) ✔  5(1-4) ✔  6(0-3) ✔  → 4 edges = n−1, stop
   MST weight = 2 + 3 + 5 + 6 = 16       (7, 8, 9 would each close a cycle)
```

*(Prim from any vertex gives the same weight 16 — verify once by hand; it's an excellent self-test.)*

---

## 6. Shortest paths — which algorithm when

```
   unweighted            → BFS                      Θ(V+E)
   weights ≥ 0           → Dijkstra                 O((V+E) log V) heap | O(V²) array
   negative edges,       → Bellman–Ford             O(VE)
     no neg. cycle         relax ALL E edges V−1 times; pass V detects neg. cycle
   DAG (even negative)   → topo order + relax       Θ(V+E)
   all pairs             → Floyd–Warshall           Θ(V³), Θ(V²) space — negative OK,
                                                     neg. cycle ⇔ negative diagonal
```

**Dijkstra, mechanized:** maintain settled set S; repeatedly settle the unsettled vertex with minimum tentative distance and relax its edges. Correct only when all weights ≥ 0 — with a negative edge, a vertex already *settled* with a larger value can later be reachable more cheaply (a negative edge "reaches back"), and Dijkstra never reopens settled vertices → wrong distances.

**Bellman-Ford mechanized:** after pass k, d[v] = best over paths using **≤ k edges**; so V−1 passes suffice (any shortest path is simple). If pass V still relaxes an edge → negative cycle reachable from source. Complexity O(VE). Edges relaxed in a fixed order per pass; **intermediate** values depend on the order, the final values don't.

**Worked Bellman-Ford (verified) — edges listed in this order: (0→1, 6), (0→2, 5), (1→3, 2), (2→1, −2), (2→3, 4):**

| pass 1 (in order) | d = [d0,d1,d2,d3] |
|---|---|
| (0→1): d1=6 | [0, 6, ∞, ∞] |
| (0→2): d2=5 | [0, 6, 5, ∞] |
| (1→3): d3=8 | [0, 6, 5, 8] |
| (2→1): d1=min(6, 5−2)=3 | [0, 3, 5, 8] |
| (2→3): min(8, 9)=8 | [0, 3, 5, 8] |

After pass 1: **d(3) = 8**. Pass 2 re-relaxes (1→3) with d1 = 3 → **d(3) = 5** = true shortest (0→2→1→3 = 5−2+2). Note: had the order been (2→1) before (1→3) *within* pass 1, d(3) would already be 5 — intermediate values are order-dependent (state the order in the question!).

**Worked Dijkstra (verified) — undirected: 0-1(4), 0-2(2), 1-2(1), 1-3(5), 2-3(8), 2-4(10), 3-4(2), 3-5(6), 4-5(3), source 0:**

| settle | because | relaxations |
|---|---|---|
| 0 (d=0) | start | d1=4, d2=2 |
| 2 (d=2) | min unsettled | d1=min(4, 2+1)=**3**, d3=10, d4=12 |
| 1 (d=3) | comes via 2, not the direct edge! | d3=min(10, 3+5)=**8** |
| 3 (d=8) | | d4=min(12, 8+2)=**10**, d5=14 |
| 4 (d=10) | | d5=min(14, 10+3)=**13** |
| 5 (d=13) | | — |

**d = [0, 3, 2, 8, 10, 13]; Σ d(v) = 36.** (Spicy detail: the direct edge 0-1 has weight 4, but the shortest route to 1 is 0→2→1 = 3.)

---

## 7. Traps checklist

1. **Dijkstra and negatives:** it fails with plain negative *edges* — no negative *cycle* required. "Works if no negative cycle" is the planted falsehood.
2. **Bellman-Ford count:** V−1 relaxation passes for correctness, the V-th **only** to detect negative cycles.
3. **BFS/DFS complexity depends on representation:** Θ(V²) on a matrix, not Θ(V+E).
4. **Undirected DFS has no cross edges** (tree + back only).
5. **Topological order exists ⇔ DAG** — but the *condensation* of any digraph is a DAG and always has one.
6. **MST ≠ shortest-path tree** (see §5 witness) — and Kruskal/Prim never "fail" on equal weights.
7. **SSSP on a DAG is Θ(V+E)** — no Dijkstra needed, negatives welcome.
8. **Floyd–Warshall is O(V³)** (three nested loops over V), not O(V·E).
9. **"Fewest edges" ≠ "cheapest":** BFS answers the first, Dijkstra the second.
10. **Counting topological orders:** do not multiply degrees blindly — count recursively position by position (indegree hits zero only after *all* predecessors are placed).

---

## 8. Practice set (10 questions, all answers verified by execution)

> Conventions: undirected unless arrows shown; adjacency processed in increasing vertex order (matters only for tie orderings, never for distances/weights). Bellman-Ford relaxes edges in the **listed order**.

**Q1 · MCQ · Fundamental** — Which statement is **FALSE**?
(A) An adjacency matrix of an n-vertex simple graph uses Θ(n²) memory regardless of the number of edges.
(B) An adjacency list uses Θ(n + m) memory for an n-vertex, m-edge graph.
(C) Testing "is (u,v) an edge?" in an adjacency list always takes O(1) time.
(D) BFS on an adjacency-list graph takes Θ(n + m) time.

**Q2 · NAT · Fundamental** — Undirected graph on vertices {0,…,6} with edges 0-1, 0-2, 1-4, 2-4, 4-5, 3-5, 5-6. Running BFS from source 0, the distance d(6) is ______.

**Q3 · MSQ · Standard** — Which statements are TRUE?
(A) BFS from a source visits vertices in non-decreasing order of distance from the source.
(B) DFS on an undirected graph produces no cross edges.
(C) On an adjacency-list graph both BFS and DFS run in Θ(V+E).
(D) Every DAG's depth-first search produces exactly one DFS tree.

**Q4 · NAT · Standard** — For the DAG with edges 0→2, 1→2, 1→3, 2→4, 3→4, 3→5, the number of distinct valid topological orderings of {0,1,2,3,4,5} is ______.

**Q5 · NAT · Standard** — Undirected weighted graph on {0,…,4}: edges 0-1 (2), 0-3 (6), 1-2 (3), 1-3 (8), 1-4 (5), 2-4 (7), 3-4 (9). The weight of a minimum spanning tree is ______.

**Q6 · MCQ · Moderate** — Which statement is TRUE?
(A) Dijkstra's algorithm fails only if the graph contains a negative-weight cycle.
(B) Floyd–Warshall requires all edge weights to be non-negative.
(C) On a DAG, single-source shortest paths can be computed in Θ(V+E) time using a topological order.
(D) The FIFO-queue optimization of Bellman–Ford guarantees Θ(V+E) worst-case time.

**Q7 · NAT · Moderate** — Directed graph, source 0, edges listed in this order: (0→1, 6), (0→2, 5), (1→3, 2), (2→1, −2), (2→3, 4). After the **first full pass** of Bellman–Ford (relaxing all edges once in the listed order), d(3) = ______.

**Q8 · MSQ · Moderate** — Which statements about MSTs are TRUE?
(A) Prim's algorithm can start from any chosen vertex and still produce an MST.
(B) An MST of a connected n-vertex graph contains exactly n − 1 edges.
(C) Kruskal's algorithm can fail to return an MST when some edge weights are equal.
(D) A minimum-weight edge crossing any cut belongs to at least one MST.

**Q9 · NAT · Hard** — Undirected weighted graph: 0-1 (4), 0-2 (2), 1-2 (1), 1-3 (5), 2-3 (8), 2-4 (10), 3-4 (2), 3-5 (6), 4-5 (3). Run Dijkstra from source 0; let S = sum of shortest-path distances to all six vertices (d(0) = 0). Then S = ______.

**Q10 · NAT · Killer** — Directed graph on {0,…,6} with edges 0→1, 1→0, 1→2, 2→3, 3→2, 3→4, 4→5, 5→6, 6→5. Compute the number of strongly connected components **plus** the number of edges in the condensation DAG. The sum is ______.

---

### Answer key

| Q | Ans | Q | Ans |
|---|---|---|---|
| 1 | C | 6 | C |
| 2 | 4 | 7 | 8 |
| 3 | A, B, C | 8 | A, B, D |
| 4 | 14 | 9 | 36 |
| 5 | 16 | 10 | 7 |

### Full solutions

**Q1 — C.** In a list, finding edge (u,v) requires scanning u's list — worst Θ(deg u), i.e. Θ(n). (A) matrix always stores n² cells. (B) list stores each edge twice in undirected graphs → Θ(n + m). (D) standard BFS bound.

**Q2 — 4.** BFS layers from 0: {1,2} at 1, {4} at 2, {5} at 3, {6} at 4 (and {3} at 4 via 5). Path realizing it: 0→1→4→5→6. So d(6) = **4**.

**Q3 — A, B, C.** (A) ✔ queue discipline. (B) ✔ undirected non-tree edges always hit an ancestor (back edge); forward/cross classifications are a directed-graph phenomenon. (C) ✔ each edge scanned O(1) times (twice). (D) ✗ a DAG with two source components (e.g. 0 and 1 with no edges) needs two DFS trees — only *one tree per connected piece*, not one overall.

**Q4 — 14.** First position ∈ {0,1} (only vertices with in-degree 0).
- First **0**: then only 1 has in-degree 0 → second = 1; next {2,3}: branch 2-first → forces 3 → {4,5} free (2 orders); branch 3-first → then {2,5}: 2-first → {4,5} free (2 orders), 5-first → forces 2 then 4 (1 order) ⇒ 5 orders total.
- First **1**: next ∈ {0,3}. Via 0 → 5 orders (same shapes as above); via 3 → 4 orders (branches: 0-first (3 orders), 5-first (1 order)) ⇒ 9 orders.
Total 5 + 9 = **14**.

**Q5 — 16.** Kruskal sorted: 2(0-1) ✔, 3(1-2) ✔, 5(1-4) ✔, 6(0-3) ✔ → 4 edges = 5−1, stop. Weight = 2+3+5+6 = **16**. (Edges 7, 8, 9 would each close a cycle.) Prim from 4: greedily adds 5(1-4)?? — check carefully: cheapest edge out of {4} is 5(1-4), then from {1,4}: 2(0-1), then 3(1-2), then 6(0-3) → same 16 ✔.

**Q6 — C.** (C) ✔ relax vertices in topological order; every path respects the order, one pass suffices, Θ(V+E). (A) ✗ even a single negative edge breaks Dijkstra's settle-and-freeze invariant (no cycle needed). (B) ✗ Floyd–Warshall handles negative edges fine (breaks only on negative cycles — diagonals go negative). (D) ✗ SPFA-style queue versions have the same O(VE) worst case.

**Q7 — 8.** Pass 1 step by step: d1 = 6 → d2 = 5 → d3 = 6+2 = 8 → d1 = min(6, 5−2) = 3 → d3 = min(8, 9) = **8**. (Pass 2 would fix d3 = 3+2 = 5 — the true shortest path, 0→2→1→3. The question's point: one pass is not enough, and intermediate values depend on relaxation order.)

**Q8 — A, B, D.** (A) ✔ every vertex is a valid Prim start (one tree grows to cover all). (B) ✔ spanning tree of n vertices always has n−1 edges. (C) ✗ ties never break correctness — you may get *a* different MST, same weight. (D) ✔ the cut property.

**Q9 — 36.** Dijkstra table (settle order 0, 2, 1, 3, 4, 5):
d after settles: d1 = min(4, 2+1) = 3 (via 2!), d2 = 2, d3 = min(2+8, 3+5) = 8, d4 = min(2+10, 8+2) = 10, d5 = min(8+6, 10+3) = 13.
Sum = 0 + 3 + 2 + 8 + 10 + 13 = **36**. Traps inside: d(1) is 3, not the direct edge's 4; d(5) is 13 via 4, not 14 via 3.

**Q10 — 7.** Cycles: {0,1}, {2,3}, {5,6}; vertex 4 is a singleton → **4 SCCs**. Condensation edges: {0,1}→{2,3} (1→2), {2,3}→{4} (3→4), {4}→{5,6} (4→5) → **3 edges**. Sum = 4 + 3 = **7**. (The condensation is the chain C₁→C₂→C₃→C₄ — its topological order is unique, a useful sanity check.)

---

## 9. Next steps

1. Re-run Q4's counting recursion once by hand — it's the highest-yield skill in this file.
2. Memorize §6's decision table + §7's trap list; then attempt `DSA_12` and `DSA_13` banks cold.
3. **Algorithms notes complete (3 files).** Subject order next: **Digital Logic** (number systems → Boolean algebra → combinational circuits).
