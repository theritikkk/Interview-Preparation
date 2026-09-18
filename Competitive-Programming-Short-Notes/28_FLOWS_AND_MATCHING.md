# 28 — Flows & Matching

Less frequent than graph/DP staples, but a recurring Div1 B–E shape. Covers enough to recognize a flow problem and implement a working solution under contest time pressure.

---

## Recognition

```
[ ] "maximum matching" between two groups                     → bipartite matching
[ ] "minimum vertex cover" on a bipartite graph                → max matching (König's theorem)
[ ] "maximum independent set" on a bipartite graph              → n - min vertex cover
[ ] "assign workers to tasks", "pair up X with Y"               → bipartite matching / flow
[ ] "maximum edge-disjoint paths from s to t"                   → max flow, unit capacities
[ ] "maximum number of paths such that no edge/vertex is reused"→ max flow (edge or vertex capacities)
[ ] "minimum cut", "minimum edges to disconnect s from t"        → min cut = max flow (duality)
[ ] "select items to maximize profit subject to conflicts"       → project selection / min-cut modeling
[ ] capacities or "at most k times" on a resource                → flow with edge capacities
```
If you can phrase the problem as "push units from a source to a sink through capacitated pipes, maximize throughput," it's a flow problem.

---

## Dinic's algorithm — max flow

```cpp
struct Dinic {
    struct Edge { int to; ll cap, flow; };
    vector<Edge> edges;
    vector<vector<int>> adj;
    vector<int> level, it;
    int n, s, t;

    Dinic(int n) : n(n), adj(n), level(n), it(n) {}

    void addEdge(int u, int v, ll cap, ll capRev = 0) {
        adj[u].push_back(edges.size()); edges.push_back({v, cap, 0});
        adj[v].push_back(edges.size()); edges.push_back({u, capRev, 0});   // reverse edge (0 cap unless the graph is undirected)
    }
    bool bfs() {
        fill(level.begin(), level.end(), -1);
        queue<int> q; level[s] = 0; q.push(s);
        while (!q.empty()) {
            int u = q.front(); q.pop();
            for (int id : adj[u]) {
                auto& e = edges[id];
                if (e.cap - e.flow > 0 && level[e.to] == -1) { level[e.to] = level[u] + 1; q.push(e.to); }
            }
        }
        return level[t] != -1;
    }
    ll dfs(int u, ll pushed) {
        if (u == t || pushed == 0) return pushed;
        for (int& i = it[u]; i < (int)adj[u].size(); i++) {
            int id = adj[u][i]; auto& e = edges[id];
            if (level[e.to] != level[u] + 1 || e.cap - e.flow <= 0) continue;
            ll d = dfs(e.to, min(pushed, e.cap - e.flow));
            if (d > 0) { e.flow += d; edges[id ^ 1].flow -= d; return d; }
        }
        return 0;
    }
    ll maxflow(int s_, int t_) {
        s = s_; t = t_; ll flow = 0;
        while (bfs()) {
            fill(it.begin(), it.end(), 0);
            while (ll pushed = dfs(s, LLONG_MAX)) flow += pushed;
        }
        return flow;
    }
};
```
**Complexity:** O(V² · E) general graphs; **O(E · √V)** on unit-capacity graphs — which is exactly what bipartite matching reduces to, making Dinic's a fast alternative to Kuhn's for larger n.

**The `id ^ 1` trick:** each `addEdge` call pushes a forward edge then its paired reverse edge consecutively, so their indices always differ by exactly 1 and `id ^ 1` toggles between them — this is how "undoing" flow along the reverse edge works.

**Trap:** `cap - flow` must never go negative; always route flow through the residual capacity, never the original capacity.
**Trap:** for an **undirected** edge, add capacity in both directions (`capRev = cap`), not zero.
**Min cut:** after `maxflow`, the min cut is exactly the set of edges from a BFS-reachable node (from `s`, in the final residual graph) to a non-reachable node — reuse the last `level[]`/BFS state to read it off for free.

---

## Bipartite matching — Kuhn's algorithm

Simpler to write than reducing to Dinic's; fine for n up to a few thousand.

```cpp
vector<int> adj[MAXN];              // left-node -> list of compatible right-node indices
int matchR[MAXN];                    // matchR[v] = left node matched to right node v, or -1
bool used[MAXN];

bool tryKuhn(int u) {
    for (int v : adj[u]) {
        if (used[v]) continue;
        used[v] = true;
        if (matchR[v] == -1 || tryKuhn(matchR[v])) { matchR[v] = u; return true; }
    }
    return false;
}
// main:
fill(matchR, matchR + R, -1);
int matching = 0;
for (int u = 0; u < L; u++) {
    fill(used, used + R, false);
    if (tryKuhn(u)) matching++;
}
```
**Complexity:** O(V · E) — for L, R ≈ 500–2000 and moderate edge counts this is fast enough; beyond that, model it as unit-capacity max flow and run Dinic's instead (O(E√V) beats O(V·E) once V gets large).

**König's theorem:** in a bipartite graph, **maximum matching size = minimum vertex cover size**. The minimum vertex cover itself is recoverable from the matching plus an alternating-path search from unmatched left nodes (standard but more code than fits here — the size equality is usually all a problem needs). **Maximum independent set = n − minimum vertex cover.**

**Hopcroft–Karp** finds the same matching in O(E√V) by augmenting along multiple shortest paths per phase instead of one at a time — better asymptotics than Kuhn's, more code to get right; most contest matches are small enough that Kuhn's is the practical choice.

---

## Common modeling patterns

```
Bipartite matching as flow:
  source -> each left node (cap 1)
  left node -> right node (cap 1) for every compatible pair
  each right node -> sink (cap 1)
  maxflow(source, sink) = size of maximum matching

Vertex capacities (limit how many times a NODE can be used, not an edge):
  split node v into v_in and v_out, edge v_in -> v_out with cap = the vertex's limit,
  redirect all incoming edges to v_in and all outgoing edges from v_out

Multiple sources / sinks:
  add a super-source connected to all real sources (cap = each source's supply),
  add a super-sink connected from all real sinks (cap = each sink's demand)
```
**Min-cost max-flow** (flow problems with a per-unit cost to minimize alongside maximizing flow) is a real and recurring topic but enough additional machinery (SPFA/Bellman-Ford-based augmenting, or Johnson's reweighting + Dijkstra) that it doesn't fit in a "micro template" — recognize the shape ("maximum flow with minimum cost", "assignment problem with costs") and look up a dedicated MCMF template if a problem needs it.

---

## Traps

```
[ ] reverse edge capacity forgotten (0 for directed graphs, = cap for undirected)
[ ] id^1 pairing broken by not adding forward/reverse edges consecutively
[ ] BFS level graph not rebuilt (or `it[]` not reset) between Dinic phases
[ ] matching modeled with cap > 1 edges when the problem wants a simple 1-1 matching
[ ] vertex-capacity problems solved without splitting nodes — silently allows reuse
[ ] Kuhn's used on a graph large enough that Dinic's O(E√V) was actually needed
[ ] flow value overflow — sum of capacities can exceed int range on large graphs
```

---

## Memory hooks

- *Flow problem shape: source, sink, capacitated pipes, maximize throughput.*
- *Dinic's `id^1`: forward and reverse edges are always added as a consecutive pair.*
- *König: max bipartite matching = min vertex cover.*
- *Vertex capacity → split the node into in/out with an edge between them.*
- *Kuhn's for small graphs, Dinic's (as unit-cap flow) when it needs to scale.*