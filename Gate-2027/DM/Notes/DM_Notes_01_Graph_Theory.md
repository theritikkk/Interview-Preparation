# DM Notes 01 — Graph Theory (High-Yield Deep Dive)

> **Why this file exists:** "Graph Theory" is the first video topic of Discrete Mathematics and the densest scoring area of the subject: degree counting, connectivity thresholds, spanning-tree counts, Euler/Hamilton conditions, Hall's theorem, chromatic numbers and planarity bounds. Every count and chromatic number below was **verified by running a graph simulator** (Matrix–Tree determinant, brute-force colouring, exhaustive matching enumeration) before writing.
>
> **Companion question banks:** `../DM_06_Graph_Connectivity_Trees.md`, `../DM_07_Graph_Matching.md`, `../DM_08_Graph_Colouring.md`. **PW source:** `../Reference/PW_Subject_Handbooks/PW_DiscreteMath.txt` §1.1–§1.5 (pages 2.1–2.10).

---

## 0. What GATE actually asks

Nine archetypes cover almost every question:

```
(A) Degree counting        Σdeg = 2e; parity; extremal edge counts
(B) Degree sequences       graphical or not (Erdős–Gallai / Havel–Hakimi)
(C) Connectivity           κ ≤ λ ≤ δ; edge thresholds; components
(D) Trees & spanning trees characterizations + COUNTING (Cayley, Prüfer, K_{m,n})
(E) Euler / Hamilton       existence conditions and extremal edge counts
(F) Bipartite & matching   Hall, König, SDRs, perfect matchings
(G) Colouring              χ of standard graph families; χ' (edge colouring)
(H) Planarity              n − e + r = 2; e ≤ 3n − 6; Kuratowski
(I) Isomorphism            invariants; degree sequence is NOT enough
```

You do not need algorithms here — only **counting and classification**. (Algorithms on graphs live in `../../DSA/Notes/DSA_Notes_07_Graph_Algorithms.md`.)

---

## 1. Degrees: the currency of graph counting (PW §1.1–§1.1.5)

**Graph types (PW §1.1):** *simple* = no self-loops, no parallel edges; *multigraph* = parallel allowed; *pseudograph* = both. GATE means simple graph unless stated otherwise.

**Handshake lemma (PW §1.1.1).** Σ deg(v) = 2·|E|.

- Therefore the number of **odd-degree vertices is always even** (PW Theorem 1).
- A *k-regular* graph (all degrees k) on n vertices has e = nk/2 edges — an integer, so nk must be even. (3-regular on 8 vertices ⇒ 12 edges; 3-regular on 7 vertices ⇒ impossible.)

**Edge bounds (PW §1.1.2, Theorem 4).** For a simple graph on n vertices:

- e ≤ C(n,2); the maximum is Kₙ. Also: a total of 2^C(n,2) labelled graphs on n vertices, and C(C(n,2), e) of them have exactly e edges.
- δ(G) ≤ 2e/n ≤ Δ(G) ≤ n − 1 — the average degree squeezes between δ and Δ.

**Pigeonhole (PW Theorem 3).** In any simple graph on n ≥ 2 vertices, **at least two vertices have the same degree**: degrees live in {0,…,n−1}, but 0 and n−1 cannot coexist.

**Degree sequences.** A sequence is *graphical* if some simple graph realizes it. Two tools:

1. **Erdős–Gallai:** sort d₁ ≥ d₂ ≥ … ≥ dₙ. Graphical ⟺ Σdᵢ even and for every k:
   Σ_{i≤k} dᵢ ≤ k(k−1) + Σ_{i>k} min(dᵢ, k).
   Typically only the first one or two inequalities matter — check k small first.
2. **Havel–Hakimi (constructive):** repeatedly take the largest degree d, connect that vertex to the next d largest, delete it, decrement. The sequence is graphical iff this process ends all-zero.

Trap (PW §1.1.8): an all-2 degree sequence does **not** force a cycle — (2,2,2,2,2,2) is realized by C₆ *and* by two disjoint triangles K₃ ∪ K₃.

**The four extremal counts you must know cold:**

| Question | Answer | Witness |
|---|---|---|
| Max edges, bipartite, n vertices | ⌊n²/4⌋ (PW Theorem 5) | K_{⌊n/2⌋,⌈n/2⌉} |
| Min edges, connected, n vertices | n − 1 | any tree |
| Max edges, disconnected, n vertices | C(n−1, 2) | K_{n−1} + isolated vertex |
| Guarantee connected | e > (n−1)(n−2)/2 | PW §1.2 fact 6 |

For k components: n − k ≤ e ≤ C(n−k+1, 2) (PW §1.2.5) — the max is one big clique plus k−1 isolated vertices.

---

## 2. The graph families GATE expects you to recognise (PW §1.1.6–§1.1.15)

| Family | Vertices | Edges | Key facts |
|---|---|---|---|
| Kₙ | n | C(n,2) | regular, χ = n, χ′ = n−1 (n even) / n (n odd) |
| Cₙ (n ≥ 3) | n | n | 2-regular, χ = 2 if n even else 3 |
| Pₙ | n | n−1 | two odd-degree endpoints |
| K_{m,n} | m+n | mn | bipartite; regular iff m = n |
| Star K_{1,n−1} | n | n−1 | the min-edge connected graph with max degree |
| Wheel Wₙ = hub + C_{n−1} (n ≥ 4) | n | 2(n−1) | χ = 3 (even rim) / 4 (odd rim) |
| Hypercube Qₙ | 2ⁿ | n·2ⁿ⁻¹ | n-regular, bipartite, χ = 2 (n ≥ 1) |
| Petersen | 10 | 15 | 3-regular, girth 5, χ = 3, χ′ = 4, non-Hamiltonian |

- **Complement** Ḡ: same vertices, edges = C(n,2) − m. Isomorphism is preserved by complementation. If G is **disconnected**, its complement is **connected** (PW Theorem 6); the converse direction may or may not hold — P₄ is its own complement (connected, connected).
- **Line graph** L(G): vertices are the *edges* of G, adjacent if the edges share an endpoint. L(K_{1,3}) = K₃ — the star becomes a triangle (PW §1.1.10).

---

## 3. Connectivity and spanning trees (PW §1.2)

| Object | Repeated vertex? | Repeated edge? | Open/closed |
|---|---|---|---|
| Walk | yes | yes | both |
| Trail | yes | **no** | both |
| Path | **no** | no | open |
| Circuit | yes | no | closed |
| Cycle | no | no | closed |

- **Connected** = a path between every pair. Components count k. Removing a *bridge* (cut edge) increases the number of components by exactly 1; a bridge lies on **no cycle**. A *cut vertex* splits its component.
- **The κ–λ–δ chain (PW §1.2 facts 5 & 12):** κ(G) ≤ λ(G) ≤ δ(G), where κ = minimum vertices whose removal disconnects (κ(Kₙ) = n−1), λ = minimum edges whose removal disconnects, δ = min degree. For disconnected or single-vertex graphs all three are 0.
- **Spanning tree:** a connected graph on n vertices *always* contains one (delete edges from cycles); it has exactly n−1 edges and a unique path between any two vertices. Trees with n ≥ 2 have ≥ 2 leaves (degree-1 vertices). Sum of degrees of a tree = 2(n−1).

---

## 4. Counting trees and spanning trees (the classic hard-question factory)

1. **Cayley's formula.** Labelled trees on n vertices: **nⁿ⁻²**. (n = 3 → 3; n = 4 → 16; n = 5 → 125.)
2. **Prüfer sequences.** Bijection between labelled trees and sequences of length n−2 over the n labels. The key dictionary entry:
   **deg(v) = (number of times v appears in the Prüfer sequence) + 1.**
   So "trees where vertex 1 has degree exactly k" = "words of length n−2 with the symbol 1 exactly k−1 times": C(n−2, k−1)·(n−1)^{n−k−1} (for n ≥ 2; also equals the multinomial (n−2)!/Π(dᵢ−1)! for a fully specified degree sequence).
3. **Complete bipartite:** τ(K_{m,n}) = **mⁿ⁻¹·nᵐ⁻¹**. Two verified anchors: τ(K₂,₃) = 2²·3¹ = **12**; τ(K₃,₃) = 3²·3² = **81**.
4. **Deletion–contraction:** τ(G) = τ(G − e) + τ(G/e) (contract e). Good for small graphs by hand.
5. **Matrix–Tree theorem:** any cofactor of the Laplacian L = D − A has determinant τ(G). (This is what the simulator used for every count above.)
6. Quick anchors: τ(Cₙ) = n (a cycle loses exactly one edge) so τ(C₅) = 5; τ(K₄) = 4² = 16 by Cayley.

Do **not** apply nⁿ⁻² to non-complete graphs — it is Cayley for Kₙ only.

---

## 5. Euler and Hamilton (PW §1.2 facts 7–9, 11)

**Euler (uses every edge exactly once; vertices may repeat):**

- **Euler circuit** ⟺ graph *connected* (isolated vertices ignored) **and every degree even**.
- **Euler trail** (open) ⟺ connected and **exactly two odd-degree vertices** — the trail starts at one odd vertex and ends at the other (PW fact 11).
- Trap: two disjoint triangles have all degrees even but **no** Euler circuit — connectivity check is mandatory. And an odd-degree vertex *anywhere* kills the circuit.

**Hamilton (visits every vertex exactly once — no clean characterization):**

- **Dirac:** n ≥ 3 and δ ≥ n/2 ⇒ Hamilton cycle.
- **Ore:** n ≥ 3 and deg(u) + deg(v) ≥ n for every non-adjacent pair u,v ⇒ Hamilton cycle.
- Hamilton path in G on n ≥ 2 vertices exists if δ ≥ (n−1)/2 (PW fact 7).
- **Extremal pair (memo-ise):** on n vertices, the maximum edges in a graph with **no Hamilton cycle** is C(n−1,2) + 1 (take K_{n−1} plus a pendant vertex), and C(n−1,2) + 2 edges **force** a Hamilton cycle.
- Petersen is the standard "sufficient conditions not necessary" example: δ = 3, non-Hamiltonian.

---

## 6. Bipartite graphs, matchings, Hall's theorem (PW §1.3.2, §1.4, §1.5)

- **G is bipartite ⟺ G has no odd cycle.** (χ ≥ 3 needed exactly when an odd cycle exists.)
- A **matching** M is a set of pairwise non-adjacent edges. **ν** = maximum matching size. A matching is *maximal* if no edge can be added, *maximum* if largest. Every maximum is maximal; not conversely.
- **Maximal ≥ half of maximum:** for every graph, any inclusion-maximal matching M satisfies ν ≤ 2|M|. *Proof:* by maximality every edge of a maximum matching M\* touches some vertex covered by M; each edge of M has 2 endpoints, and each endpoint can "own" at most one edge of M\* (matchings are disjoint), so |M\*| ≤ 2|M|.
- **Hall's theorem.** A bipartite graph with parts X, Y has a matching saturating X ⟺ **|N(S)| ≥ |S| for every S ⊆ X**. Checking only "obvious" subsets is not enough — violations hide in small subsets (all of x₁,x₂,x₃ sharing one neighbour, etc.). If additionally |X| = |Y|, Hall gives a **perfect matching**.
- **König:** in bipartite graphs, maximum matching = minimum vertex cover (τ = ν). Then α = n − τ (Gallai: α + τ = n for **every** graph, PW Theorem 7 + §1.3.2).
- **Edge cover** (vertex cover by *edges*): min edge cover = n − ν for graphs with no isolated vertices (PW §1.4; covers need every vertex touched by ≥ 1 edge).
- **Perfect matchings.** Exists ⇒ n even (converse fails: star K₁,₃). ν(K_{m,n}) = min(m,n). Every k-regular bipartite graph (k ≥ 1) has a perfect matching. Every *bridgeless* cubic graph has a perfect matching (Petersen's theorem) — cubic alone is not enough.
- **Counting perfect matchings of K_{2n} (PW §1.5):** (2n)! / (2ⁿ·n!). For K₄: 4!/(2²·2!) = 3.
- **SDRs:** a system of distinct representatives for a family F₁,…,Fₖ picks xᵢ ∈ Fᵢ with all xᵢ distinct — exactly a matching saturating the "left side" of the incidence bipartite graph, so Hall decides existence; counting SDRs is a small case-by-case enumeration.

---

## 7. Colouring (PW §1.3.1; banks DM_08)

- χ(G) = minimum colours for a proper colouring. χ = 1 ⟺ no edges; χ = 2 ⟺ bipartite with ≥ 1 edge ⟺ no odd cycle.
- Anchors: χ(Kₙ) = n; χ(Cₙ) = 2/3 by parity; χ(Wₙ) = 3 (n even) / 4 (n odd); χ(Qₙ) = 2; χ(K_{m,n}) = 2; χ(Petersen) = 3.
- Always **ω(G) ≤ χ(G) ≤ Δ(G) + 1**, where ω = clique number. Brooks: a *connected* graph has χ ≤ Δ **unless** it is Kₙ or an odd cycle (those need Δ + 1).
- **Edge colouring χ′:** Δ ≤ χ′ ≤ Δ + 1 (Vizing). Bipartite graphs always achieve the lower end: χ′ = Δ. Complete graphs: χ′(Kₙ) = n − 1 for **even** n, χ′(Kₙ) = n for **odd** n ≥ 3. A graph with χ′ = Δ is class 1, else class 2 (Petersen is class 2).
- Greedy colouring uses at most Δ + 1 colours **for every ordering** (worst case), but can exceed χ for particular orderings; it never uses fewer than χ, and specific orderings can achieve χ.
- **Kneser graph K(n,k):** vertices = k-subsets of an n-set, adjacent iff disjoint. χ(K(n,k)) = n − 2k + 2. E.g. χ(K(5,2)) = 3, χ(K(6,2)) = 4.
- Every planar graph is 4-colourable (4-colour theorem); for the exam, remember it as an upper bound, and remember K₄ needs all 4.

---

## 8. Planarity and isomorphism (PW §1.3)

- **Planar** = drawable in the plane with edges meeting only at vertices. **Kuratowski:** non-planar ⟺ contains a *subdivision* of K₅ or K₃,₃ (PW §1.3). So K₅, K₃,₃ and any supergraph containing a subdivided copy are non-planar; K₄, Cₙ, trees, wheels, Qₙ are planar.
- **Euler's formula** (connected planar, n vertices, e edges, r regions): **n − e + r = 2**, i.e. r = e − n + 2. Regions include the unbounded one.
- Edge bounds for simple planar graphs (n ≥ 3): **e ≤ 3n − 6** (and 3r ≤ 2e). If also bipartite: **e ≤ 2n − 4**. Both bounds are *tight* — triangulations of the sphere attain 3n − 6.
- So: max edges of a simple planar graph on 12 vertices = 3·12 − 6 = 30; on 10 vertices = 24; bipartite planar on 12 vertices = 20.
- **Isomorphism invariants:** n, m, degree sequence (sorted), number of components, triangles/short cycles, χ, connectivity, bipartiteness. None of them is sufficient — the classic pair is **C₆ vs K₃ ∪ K₃** (both (2,2,2,2,2,2), not isomorphic since one is connected / has 2 triangles vs 0). Complements of isomorphic graphs are isomorphic, hence also: G ≅ H ⟺ Ḡ ≅ H̄.

---

## 9. Trap list (read before the exam)

1. Odd-degree count is even — but even-degree count has no parity rule.
2. All-2 sequence ⇏ cycle; same degree sequence ⇏ isomorphic.
3. ⌊n²/4⌋ (floor!) for max bipartite edges.
4. Sum of degrees in a tree = 2n − 2 and #leaves ≥ 2 (n ≥ 2).
5. Connected ⇔ spanning tree exists; min edges n−1; forced-connected threshold e > (n−1)(n−2)/2.
6. Euler needs the *global* parity condition **and** connectivity; exactly-2-odd gives a trail, not a circuit.
7. Max edges without a Hamilton cycle = C(n−1,2) + 1; with C(n−1,2) + 2 forces one.
8. Hall: check **every** subset of one side; equal sides + Hall ⇒ perfect matching.
9. χ ≤ Δ + 1, and χ = Δ + 1 only for complete graphs and odd cycles (Brooks).
10. Planar bounds need *simple* and n ≥ 3; bipartite planar is 2n − 4; Euler's r = e − n + 2.
11. Cayley nⁿ⁻² is for complete graphs only; Prüfer: degree = appearances + 1; τ(K_{m,n}) = mⁿ⁻¹nᵐ⁻¹.
12. κ ≤ λ ≤ δ — worth one mark if you memorised it.

---

## 10. Practice questions (fresh — not in the banks)

### Q1 · MCQ · Fundamental

Which one of the following statements is TRUE for every simple graph?

- (A) The number of vertices of odd degree is even.
- (B) The number of vertices of even degree is even.
- (C) If every vertex has degree at least 1, then the graph is connected.
- (D) The total number of edges is even.

### Q2 · NAT · Fundamental

The maximum possible number of edges in a simple **bipartite** graph on 10 labelled vertices is ______.

### Q3 · MSQ · Standard

Which of the following statements is/are TRUE?

- (A) For every graph G, κ(G) ≤ λ(G) ≤ δ(G), where κ, λ, δ are the vertex connectivity, edge connectivity and minimum degree respectively.
- (B) Every connected graph contains a spanning tree.
- (C) Every connected graph on n ≥ 1 vertices has at least n − 1 edges.
- (D) Two simple graphs with the same number of vertices and the same degree sequence are necessarily isomorphic.

### Q4 · NAT · Standard

The complete bipartite graph K₂,₃ has parts {u₁, u₂} and {v₁, v₂, v₃}. The number of distinct spanning trees of K₂,₃ is ______.

### Q5 · MCQ · Standard

G is a connected graph in which exactly two vertices u and v have odd degree, and every other vertex has even degree. Which one of the following is TRUE?

- (A) G has an Euler circuit.
- (B) G has an Euler trail whose two end vertices are u and v.
- (C) G has no Euler trail, because having two odd-degree vertices blocks every Euler trail.
- (D) G has an Euler trail only if u and v are adjacent.

### Q6 · NAT · Moderate

The maximum number of edges in a simple planar graph on 12 vertices is ______.

### Q7 · MSQ · Moderate

Which of the following statements is/are TRUE?

- (A) If a bipartite graph with parts X and Y, |X| = |Y|, satisfies |N(S)| ≥ |S| for every S ⊆ X, then it has a perfect matching.
- (B) Every inclusion-maximal matching (no edge can be added to it) has at least half as many edges as a maximum matching.
- (C) In every tree on at least two vertices, some maximum matching leaves at most one vertex unmatched.
- (D) If a bipartite graph has |X| < |Y| and every vertex has degree at least 1, then there is a matching that saturates X.

### Q8 · NAT · Moderate

A system of distinct representatives (SDR) of a family of sets F₁, F₂, F₃ is an ordered triple (r₁, r₂, r₃) with rᵢ ∈ Fᵢ and r₁, r₂, r₃ pairwise distinct. For the family F₁ = {1, 2}, F₂ = {2, 3}, F₃ = {1, 3}, the number of SDRs is ______.

### Q9 · MCQ · Hard

The circulant graph C₉(1, 2) has vertices 0, 1, …, 8, and two vertices i, j are adjacent iff j − i ≡ ±1 or ±2 (mod 9). Its chromatic number is

- (A) 2
- (B) 3
- (C) 4
- (D) 5

### Q10 · NAT · Killer

Consider the complete bipartite graph K₃,₃ with parts {a, b, c} and {x, y, z}. Let

- s = number of distinct spanning trees of K₃,₃,
- m = number of perfect matchings of K₃,₃,
- k = chromatic number of K₃,₃.

Then s − m·k = ______.

---

## 11. Answer key & full solutions

**Answer key:** 1-A · 2-25 · 3-A,B,C · 4-12 · 5-B · 6-30 · 7-A,B · 8-2 · 9-B · 10-69

**Q1 — A.** Handshake lemma (PW §1.1.1): Σdeg = 2e, an even number, and a sum of integers is even only if it contains an even number of odd terms. (B) ✘ P₃ has degrees (1,2,1): exactly **one** even-degree vertex. (C) ✘ two disjoint edges: all degrees 1 ≥ 1, disconnected. (D) ✘ K₃ has 3 edges.

**Q2 — 25.** Split the vertices into parts of sizes k and 10 − k; edges ≤ k(10 − k), maximised at k = 5: 5·5 = **25**, attained by K₅,₅. This is the floor(n²/4) bound (PW §1.1.13, Theorem 5) with n = 10.

**Q3 — A, B, C.** (A) ✔ PW §1.2 facts 5 and 12. (B) ✔ keep deleting edges lying on a cycle until acyclic — still connected, a spanning tree. (C) ✔ PW §1.2.4: a connected graph needs at least n − 1 edges (the "minimally connected" bound). (D) ✘ the C₆ vs K₃ ∪ K₃ pair: both degree sequences (2,2,2,2,2,2); C₆ is connected with no triangle, K₃ ∪ K₃ is disconnected with two triangles.

**Q4 — 12.** Two independent ways:
- Formula τ(K_{m,n}) = mⁿ⁻¹·nᵐ⁻¹: τ(K₂,₃) = 2³⁻¹·3²⁻¹ = 4·3 = **12** (verified by Matrix–Tree).
- Elementary: a 5-vertex tree has 4 edges; choose 4 of the 6 edges: C(6,4) = 15. Exactly C(2,2)·C(3,2) = 3 of these 4-edge sets are 4-cycles (choose both vertices of the size-2 part and 2 of the size-3 part). 15 − 3 = 12. ✔

**Q5 — B.** Exactly two odd vertices u, v ⇔ an Euler *trail* exists, running u to v (PW fact 11). (A) ✘ a circuit needs all degrees even. (C) ✘ exactly two odd vertices is the condition for a trail, not against it. (D) ✘ adjacency is irrelevant: the path P₃ = a–b–c has odd vertices a, c not adjacent, yet a→b→c is an Euler trail.

**Q6 — 30.** Simple planar, n = 12 ≥ 3 ⇒ e ≤ 3n − 6 = 3·12 − 6 = **30** (PW §1.3). Tight: any maximal planar (triangulation) graph on 12 vertices attains it.

**Q7 — A, B.** (A) ✔ Hall's theorem gives a matching saturating X; |X| = |Y| makes it perfect. (B) ✔ let M\* be maximum. By maximality of M every edge of M\* has at least one endpoint covered by M; assign each M\*-edge to such a covered endpoint. Each of the 2|M| covered vertices receives at most one M\*-edge (M\* is a matching), so |M\*| ≤ 2|M|, i.e. |M| ≥ ν/2. (C) ✘ star K₁,₃: n = 4, maximum matching = 1 edge, so 2 vertices stay unmatched. (D) ✘ let x₁, x₂, x₃ be adjacent only to y₁, and x₄ adjacent to y₂, …, y₆ (and y₁ to x₁…x₃): |X| = 4 < |Y| = 6, all degrees ≥ 1, but x₁, x₂, x₃ can only match to y₁ — no matching saturates X (Hall violation |N({x₁,x₂,x₃})| = 1 < 3).

**Q8 — 2.** Enumerate by r₁:
- r₁ = 1: r₂ ∈ {2,3}. r₂ = 2 ⇒ r₃ = 3 (the only element of F₃ outside {1,2}): **(1,2,3)**. r₂ = 3 ⇒ {r₃} ⊆ {1,3}∖{1,3} = ∅ ✘.
- r₁ = 2: r₂ = 2 repeats ✘; r₂ = 3 ⇒ r₃ = 1: **(2,3,1)**.
Two SDRs (verified by exhaustive search).

**Q9 — B.** C₉(1,2) contains the triangle 0–1–2 (0–1: difference 1; 1–2: difference 1; 2–0: difference 2 mod 9) so χ ≥ 3. Colour i with i mod 3: adjacent vertices differ by ±1 or ±2 mod 9, hence their residues differ mod 3, never equal — a proper 3-colouring. χ = **3** (brute-force confirmed not 2-colourable since it has an odd cycle).

**Q10 — 69.** Verified components:
- Spanning trees: τ(K₃,₃) = 3³⁻¹·3³⁻¹ = 3²·3² = **81** (= mⁿ⁻¹nᵐ⁻¹; brute-force Matrix–Tree agrees).
- Perfect matchings: each is a bijection {a,b,c} → {x,y,z} = 3! = **6**.
- Chromatic number: K₃,₃ is bipartite with edges ⇒ **2** (colour each part).
- s − m·k = 81 − 6·2 = **69**.

---

## 12. Next steps

1. Re-derive §1's four extremal counts and §4's three counting tools from a blank page — they generate most "killer" questions.
2. Memorise the trap list in §9, then run the three banks (`DM_06`, `DM_07`, `DM_08`) cold.
3. Continue to **File 02 — Propositional & Predicate Logic** (`DM_Notes_02_Logic_Propositional_Predicate.md`, banks `DM_01`–`DM_02`), then **File 03 — Sets, Relations & Functions** (banks `DM_03`–`DM_04`).
