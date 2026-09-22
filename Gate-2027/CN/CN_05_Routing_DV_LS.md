# Computer Networks → Routing: Distance Vector & Link State

**GATE 2027 syllabus:** Computer Networks — distance vector and link state routing.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** Graphs are undirected and weighted (weight = cost of the link, symmetric in both directions) unless stated otherwise. Distance-vector nodes exchange full distance vectors with their direct neighbours once per round (synchronous rounds) and immediately apply the Bellman-Ford update. Link-state nodes have full topology knowledge and run Dijkstra's algorithm.

---

## Questions

### Q1 · MCQ · Fundamental
Link-state routing (as in OSPF) differs from distance-vector routing (as in RIP) mainly in that

- (A) link-state nodes flood topology information to all nodes and each computes shortest paths locally, while distance-vector nodes exchange only their own distance estimates with direct neighbours
- (B) link-state routing cannot handle link-cost changes
- (C) distance-vector routing requires each node to know the entire network topology
- (D) link-state routing never suffers from routing loops during convergence

---

### Q2 · NAT · Fundamental
The graph has vertices A, B, C, D, E and edges A–B (2), A–C (5), B–C (1), B–D (4), C–D (2), C–E (7), D–E (1). Running Dijkstra's algorithm from source A, the shortest-path distance to E is ______.

---

### Q3 · MSQ · Standard
Which of the following statements about the distance-vector algorithm (Bellman-Ford style) is/are TRUE?

- (A) Each node needs to know only the costs to its directly connected neighbours to begin the algorithm.
- (B) A node updates its distance to a destination only if a neighbour offers a strictly shorter path.
- (C) The algorithm can suffer from the "count-to-infinity" problem when a link cost increases sharply or a link fails.
- (D) Split horizon (a node never advertises a route back to the neighbour it learned that route from) completely eliminates all routing loops in every topology.

---

### Q4 · NAT · Standard
Using the graph of Q2, running Dijkstra's algorithm from source A, the shortest-path distance to D is ______.

---

### Q5 · MCQ · Standard
Three routers form a line: A–B (cost 1), B–C (cost 1). Initially all distances have converged correctly. The link B–C then fails. Using plain distance-vector routing **without** split horizon or poison reverse, which one of the following can happen?

- (A) A and B converge to the correct new distances within one round.
- (B) B's route to C, and A's route to C, oscillate upward for several rounds before settling (count-to-infinity), because A still advertises a (now stale) route to C via B.
- (C) The network partitions immediately and no further updates are exchanged.
- (D) Link-state routing would have the same problem in this topology.

---

### Q6 · NAT · Moderate
For the graph of Q2, the distance-vector algorithm is run synchronously starting from the direct-neighbour costs (round 0). The **number of additional rounds** needed for every node's distance vector to converge to the correct shortest-path values is ______.

---

### Q7 · MSQ · Moderate
Which of the following statements about routing metrics and protocols is/are TRUE?

- (A) RIP typically uses hop count as its metric and limits paths to at most 15 hops.
- (B) OSPF is a link-state protocol that can use configurable link costs (e.g., based on bandwidth).
- (C) A distance-vector protocol requires every router to run a shortest-path algorithm over the full topology graph, exactly like a link-state protocol.
- (D) Poison reverse means a node advertises an infinite distance for a route back to the neighbour from which it learned that route.

---

### Q8 · NAT · Moderate
A network has 6 routers arranged so that every pair of routers is directly connected (a complete graph K₆) with distinct positive integer link costs. Running link-state routing, each router computes shortest paths using Dijkstra's algorithm. The maximum possible number of hops in any router's shortest path to any other router in this topology is ______.

---

### Q9 · MSQ · Hard
In a link-state protocol, every router floods a Link State Packet (LSP) describing its direct links to all other routers in the network. Which of the following statements about this process is/are TRUE?

- (A) Every router eventually has an identical view of the complete network topology (assuming no further changes and reliable flooding).
- (B) Flooding requires each router to know the entire topology in advance, before it can flood.
- (C) A sequence number (or similar mechanism) in each LSP is used to prevent old, stale LSPs from being propagated indefinitely.
- (D) Each router computes its own shortest-path tree independently after receiving all LSPs, so different routers may legitimately compute different next hops for the same destination even with a consistent topology view.

---

### Q10 · NAT · Killer
Three routers A, B, C form a triangle with direct links A–B (cost 1), B–C (cost 1) and A–C (cost 10). Distance-vector routing (no split horizon, no poison reverse) has fully converged, so A and B each route to C via B at cost 2 and 1 respectively, and A's table shows a route to C of cost 2. The link B–C then fails (the direct link A–C, cost 10, is unaffected, and B and C both immediately detect their link is down). At each synchronous round from then on, every node recomputes its distance to every destination as the minimum, over all its direct neighbours, of (that neighbour's most recently advertised distance to the destination) + (cost of the link to that neighbour) — also comparing against its own direct link where one exists — using the neighbours' values from the **immediately preceding** round. The smallest round number at which **A's** distance to C first reaches its final, correct value of 10 is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | A | Core distinction between the two families |
| 2 | 6 | A–B–C–D–E: 2+1+2+1 |
| 3 | A, C | (D): split horizon does not fix all loops (e.g., 3-node loops) |
| 4 | 5 | A–B–C–D: 2+1+2 |
| 5 | B | The classic count-to-infinity scenario |
| 6 | 3 | Diameter of the graph in hops is 4, so 3 more rounds after round 0 |
| 7 | A, B, D | (C) is false — that is exactly what link-state does, not distance-vector |
| 8 | 1 | Every pair is directly connected, so the shortest path is always the single direct edge |
| 9 | A, C, D | (B) is false — flooding needs no prior topology knowledge, only its own direct links |
| 10 | 8 | A's distance to C climbs 2, 4, 4, 6, 6, 8, 8, 10 over rounds 1–8, settling at the direct-link cost of 10 only in round 8 |
