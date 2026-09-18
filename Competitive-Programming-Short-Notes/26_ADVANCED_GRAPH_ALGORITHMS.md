# 26 — Advanced Graph Algorithms

SCC, bridges/articulation points, 2-SAT, Eulerian paths. All appear regularly from mid-Div2 onward and none of them are covered in `07_GRAPH_TEMPLATES.md`.

---

## Strongly Connected Components (SCC)

**What it is:** a maximal set of nodes in a **directed** graph where every node can reach every other node in the set. Collapsing each SCC to a single node gives the **condensation DAG** — this is the whole reason SCCs matter: it turns a graph with cycles into a DAG you can run DP/topological reasoning on.

**Recognition:** "strongly connected", "can reach each other", 2-SAT (below), "collapse cycles then do DP on the result", dependency resolution with cyclic dependencies.

### Tarjan's algorithm — one DFS

```cpp
vector<int> adj[MAXN];
int disc[MAXN], low[MAXN], comp[MAXN], timer = 0, compCount = 0;
bool onStack[MAXN];
stack<int> stk;

void tarjan(int u) {
    disc[u] = low[u] = timer++;
    stk.push(u); onStack[u] = true;
    for (int v : adj[u]) {
        if (disc[v] == -1) { tarjan(v); low[u] = min(low[u], low[v]); }
        else if (onStack[v]) low[u] = min(low[u], disc[v]);
    }
    if (low[u] == disc[u]) {               // u is the root of an SCC — pop it off
        while (true) {
            int v = stk.top(); stk.pop(); onStack[v] = false;
            comp[v] = compCount;
            if (v == u) break;
        }
        compCount++;
    }
}
// init disc[] to -1 for all nodes; call tarjan(u) for every unvisited u
```
**Complexity:** O(n + m), single pass.

**Topological order for free:** Tarjan assigns `comp` ids in the order SCCs are **completed**, which is exactly **reverse topological order** of the condensation DAG (sinks complete first). So: **edges in the condensation DAG always go from a higher `comp` id to a lower one.** To read the condensation in normal topological order (source first), iterate `comp` ids from high to low — no separate topological sort needed.

### Kosaraju's algorithm — two DFS

```cpp
vector<int> adj[MAXN], radj[MAXN];       // radj = transpose graph
vector<int> order_;
bool vis[MAXN];
int comp[MAXN];

void dfs1(int u) {
    vis[u] = true;
    for (int v : adj[u]) if (!vis[v]) dfs1(v);
    order_.push_back(u);
}
void dfs2(int u, int c) {
    comp[u] = c;
    for (int v : radj[u]) if (comp[v] == -1) dfs2(v, c);
}
// main:
for (int i = 0; i < n; i++) if (!vis[i]) dfs1(i);
fill(comp, comp + n, -1);
int c = 0;
for (int i = n - 1; i >= 0; i--) {
    int u = order_[i];
    if (comp[u] == -1) dfs2(u, c++);
}
```
**Complexity:** O(n + m), needs the transpose graph built explicitly.

**Topological order:** Kosaraju gives `comp` ids in **forward** topological order directly (comp 0 = a source). Opposite convention from Tarjan — pick one and remember which.

**Which to use:** Tarjan is one DFS and doesn't need a transpose graph — the usual contest choice. Kosaraju is easier to reason about and gives topological order without the "read it backwards" caveat. Both share the same recursion-depth caveat as any DFS (see `07_GRAPH_TEMPLATES.md`).

---

## Bridges and articulation points

**Bridge:** an edge whose removal disconnects the graph. **Articulation point (cut vertex):** a node whose removal disconnects the graph. Both computed with one DFS using `disc`/`low`, the same machinery as Tarjan's SCC.

**Recognition:** "critical edges/roads", "minimum edges to add for 2-edge-connectivity", "network reliability", "points of failure".

```cpp
vector<pair<int,int>> adj[MAXN];   // (neighbor, EDGE ID — see trap below)
int disc[MAXN], low[MAXN], timer = 0;
bool visited[MAXN], isArt[MAXN];
vector<pair<int,int>> bridges;

void dfs(int u, int parentEdge) {
    visited[u] = true;
    disc[u] = low[u] = timer++;
    int children = 0;
    for (auto [v, id] : adj[u]) {
        if (id == parentEdge) continue;              // skip the edge we arrived on
        if (visited[v]) {
            low[u] = min(low[u], disc[v]);            // back edge
        } else {
            children++;
            dfs(v, id);
            low[u] = min(low[u], low[v]);
            if (low[v] > disc[u]) bridges.push_back({u, v});                 // BRIDGE: strict >
            if (parentEdge != -1 && low[v] >= disc[u]) isArt[u] = true;      // ARTICULATION: >=
        }
    }
    if (parentEdge == -1 && children > 1) isArt[u] = true;    // root special case
}
```

**Trap — multi-edges.** `if (v != parent)` (the pattern used everywhere else in this handbook) silently breaks here: two parallel edges between `u` and its parent look like a back edge to the parent and produce a false bridge/false negative. Give every undirected edge a **unique id** at input time (push `(v, id)` into `adj[u]` and `(u, id)` into `adj[v]` with the same `id`), and skip by **id**, not by node.

**Trap — bridge vs articulation condition.** Bridge uses strict `low[v] > disc[u]`; articulation uses `low[v] >= disc[u]`. Mixing these up is a one-character bug that passes small tests.

**Trap — the root.** A non-root node is an articulation point if *any* child has `low[v] >= disc[u]`. The root has no `disc[u]` to compare against — it's an articulation point iff it has **more than one** DFS-tree child.

**Biconnected components / block-cut tree:** contracting each 2-edge-connected component to a node (using the bridges found above) turns the graph into a tree of bridges — useful when a problem wants "the graph after repeatedly asking about 2-edge-connectivity". Build it by DSU: union every non-bridge edge's endpoints. Full block-cut tree construction (for vertex-biconnectivity) is more code than fits here — recognize the shape and look it up if a problem needs it.

---

## 2-SAT

**What it models:** `n` boolean variables, `m` clauses each of the form "at least one of two literals is true" — `(x_i op1) ∨ (x_j op2)` where each literal is a variable or its negation. Decide satisfiability and, if satisfiable, produce an assignment. This is the boolean-constraint version of "each of n items has exactly two states and pairs of choices constrain each other" — a very common Div1 B/C shape.

**The construction:** build an **implication graph** on `2n` nodes, one per literal. A clause `(a ∨ b)` is logically the same as "¬a implies b" **and** "¬b implies a" — add both directed edges. Then: **the formula is satisfiable iff no variable has both its literals in the same SCC.**

```cpp
int NODE(int var, bool val) { return 2 * var + (val ? 0 : 1); }   // even = true-literal, odd = false-literal

// clause (var_u == valU) OR (var_v == valV):
void addClause(int u, bool valU, int v, bool valV) {
    int notU = NODE(u, !valU), U = NODE(u, valU);
    int notV = NODE(v, !valV), V = NODE(v, valV);
    adj[notU].push_back(V);      // ¬u -> v
    adj[notV].push_back(U);      // ¬v -> u
}
```

**Useful gadgets built from `addClause`:**
```cpp
// force x true:               addClause(x, true,  x, true);
// force x false:               addClause(x, false, x, false);
// x implies y  (x -> y):        addClause(x, false, y, true);      // clause: ¬x ∨ y
// x XOR y:                      addClause(x, true, y, true); addClause(x, false, y, false);
// at most one of x, y:          addClause(x, false, y, false);     // clause: ¬x ∨ ¬y
```

**Solve:** run Tarjan's SCC on the `2n`-node implication graph.

```cpp
bool satisfiable = true;
for (int i = 0; i < n; i++)
    if (comp[NODE(i,true)] == comp[NODE(i,false)]) satisfiable = false;

// if satisfiable, extract an assignment:
vector<bool> value(n);
for (int i = 0; i < n; i++)
    value[i] = comp[NODE(i, true)] < comp[NODE(i, false)];
```

**Why that comparison, precisely:** using Tarjan's convention (higher `comp` id = earlier in topological order = closer to being a "cause"), the safe assignment picks whichever literal's SCC is more sink-ward — i.e. has the **smaller** `comp` id. That's exactly `comp[NODE(i,true)] < comp[NODE(i,false)]`. If you build SCCs with Kosaraju instead, this comparison flips (Kosaraju ids run the opposite direction) — pick the rule that matches whichever SCC routine you actually used, and sanity-check it once on a one-variable forced-true clause before trusting it under contest pressure.

**Complexity:** O(n + m) — one SCC pass.

**Recognition checklist:**
```
[ ] n items, each with exactly two mutually exclusive states
[ ] constraints of the form "at least one of A, B", "if A then B", "not both A and B"
[ ] asked for existence of a valid assignment, or one specific valid assignment
[ ] n, m up to ~1e5–2e5 (SCC-sized, not brute-force-sized)
```

---

## Eulerian path / circuit

**Eulerian circuit:** a closed walk using every **edge** exactly once. **Eulerian path:** an open version (start ≠ end).

**Existence, undirected:** connected (ignoring isolated vertices) and the number of odd-degree vertices is 0 (circuit) or 2 (path, must start/end at the two odd vertices).
**Existence, directed:** `in-degree == out-degree` for every vertex (circuit), or exactly one vertex with `out - in == 1` (start) and one with `in - out == 1` (end) and all others balanced (path); underlying graph connected.

### Hierholzer's algorithm

```cpp
vector<int> adj[MAXN];             // for a multigraph, store (neighbor, edgeId) and mark edges used instead of pop_back
vector<int> circuit;
void hierholzer(int u) {
    while (!adj[u].empty()) {
        int v = adj[u].back(); adj[u].pop_back();
        hierholzer(v);
    }
    circuit.push_back(u);
}
// circuit is produced in REVERSE — reverse() it for the walk in order
```
**Trap:** for an **undirected** multigraph, popping an edge from `adj[u]` must also invalidate its paired copy in `adj[v]`. The robust fix is edge indices with a `used[]` array instead of `pop_back` on raw adjacency lists.
**Trap:** recursion depth equals the number of edges — convert to an explicit stack for `m` beyond ~1e5–1e6.
**Start node:** for a path (not circuit), start Hierholzer at one of the two odd-degree (undirected) / imbalanced (directed) vertices.

---

## Traps

```
[ ] used "v != parent" instead of edge ids for bridges/articulation points — breaks on multi-edges
[ ] bridge condition used >= instead of the correct strict >
[ ] articulation root case forgotten (needs children > 1, not the low/disc comparison)
[ ] SCC comp-id topological direction assumed backwards (Tarjan vs Kosaraju differ)
[ ] 2-SAT: forgot to add BOTH implication edges per clause
[ ] 2-SAT: assignment comparison direction flipped for the SCC routine actually used
[ ] Eulerian: existence not checked before running Hierholzer (garbage output on an inexistent circuit)
[ ] Eulerian: recursion depth on a graph with many edges
[ ] any of these run on a disconnected graph without handling every component
```

---

## Memory hooks

- *SCC collapses cycles into one node so you can DP on the resulting DAG.*
- *Tarjan: comp ids come out in reverse topological order — higher id is more "source".*
- *Bridge: strict `>`. Articulation: `>=`. The root is special — count its children.*
- *2-SAT: a clause `(a ∨ b)` becomes two implications, `¬a → b` and `¬b → a`.*
- *2-SAT is unsatisfiable iff some variable's two literals land in the same SCC.*
- *Eulerian: check existence first; 0 or 2 odd-degree vertices (undirected).*