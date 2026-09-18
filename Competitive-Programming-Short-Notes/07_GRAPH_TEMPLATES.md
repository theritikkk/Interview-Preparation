# 07 — Graph Implementation Tricks

---

## Which algorithm? (recognition table)

| Situation | Use | Complexity |
|---|---|---|
| Unweighted shortest path | BFS | O(n + m) |
| Weights are only 0 or 1 | **0-1 BFS** (deque) | O(n + m) |
| Weights in a tiny set {0..k} | Dial's / k+1 deques | O(n + m·k) |
| Non-negative weights | Dijkstra | O((n+m) log n) |
| Negative edges, no negative cycle | Bellman–Ford | O(n·m) |
| Detect a negative cycle | Bellman–Ford (n-th relaxation) | O(n·m) |
| All-pairs, n ≤ 500 | Floyd–Warshall | O(n³) |
| MST | Kruskal (DSU) or Prim | O(m log m) |
| Connectivity / components | DSU or BFS/DFS | ~O(n + m) |
| Dependency order | topological sort | O(n + m) |
| Cycle in a directed graph | Kahn's (leftover nodes) or DFS colors | O(n + m) |
| Cycle in an undirected graph | DSU or DFS with parent | O(n + m) |
| Bipartite check | BFS 2-coloring | O(n + m) |
| Shortest path on a grid | BFS with dx/dy | O(rows·cols) |
| Tree path queries | LCA / binary lifting | O(n log n) build |
| Strongly connected components | Tarjan's / Kosaraju's — `26_ADVANCED_GRAPH_ALGORITHMS.md` | O(n + m) |
| Critical edges / vertices | Bridges / articulation points — `26_ADVANCED_GRAPH_ALGORITHMS.md` | O(n + m) |
| Boolean satisfiability, 2 states per variable | 2-SAT — `26_ADVANCED_GRAPH_ALGORITHMS.md` | O(n + m) |
| Bipartite matching / max flow | Kuhn's / Dinic's — `28_FLOWS_AND_MATCHING.md` | O(V·E) / O(E√V) |

---

## Representation

```cpp
// adjacency list — the default
vector<vector<int>> adj(n + 1);
adj[u].push_back(v); adj[v].push_back(u);       // undirected: BOTH directions

// weighted
vector<vector<pair<int,int>>> adj(n + 1);        // (neighbour, weight)
adj[u].push_back({v, w});

// edge list — for Kruskal / Bellman-Ford
vector<array<int,3>> edges;                      // {w, u, v}
```

**Trap:** undirected graphs need **both** `adj[u].push_back(v)` and `adj[v].push_back(u)`. Forgetting one is a silent WA that passes small samples.

**Trap:** size `n + 1` if the input is 1-indexed (it usually is on Codeforces).

**Trap (multi-test):** don't `memset`/reallocate the whole `N_max` structure per test. Either build a fresh `vector<vector<int>> adj(n+1)` (O(n)) or clear only `1..n`.

**When to use a flat CSR layout:** n, m ≈ 1e6 and you're cache-bound. Two arrays: `start[]` (prefix sums of degrees) and `to[]`. One allocation, perfectly contiguous, ~2× faster than vector-of-vectors. Worth it only when it's the bottleneck.

---

## BFS

```cpp
vector<int> dist(n + 1, -1);
queue<int> q;
dist[s] = 0; q.push(s);
while (!q.empty()) {
    int u = q.front(); q.pop();
    for (int v : adj[u]) if (dist[v] == -1) {
        dist[v] = dist[u] + 1;
        q.push(v);
    }
}
```
**Use when:** unweighted shortest path, level-order, "minimum number of moves".
**Remember:** mark visited **when you push**, not when you pop — otherwise nodes enter the queue multiple times.

### Multi-source BFS

Push all sources with distance 0 before the loop starts. Gives "distance to the nearest source" in one pass — a very common trick for grids ("nearest 0 to each cell").

### Grid BFS

```cpp
const int dx[4] = {0,0,1,-1}, dy[4] = {1,-1,0,0};
// 8-directional:
// const int dx[8]={1,1,1,0,0,-1,-1,-1}, dy[8]={1,0,-1,1,-1,1,0,-1};

for (int d = 0; d < 4; d++) {
    int nx = x + dx[d], ny = y + dy[d];
    if (nx < 0 || nx >= R || ny < 0 || ny >= C) continue;
    if (grid[nx][ny] == '#' || vis[nx][ny]) continue;
    ...
}
```
Flatten to 1D with `id = x * C + y` when you want a single `vector<int>` instead of 2D.

---

## DFS

```cpp
void dfs(int u, int p) {
    vis[u] = true;
    for (int v : adj[u]) if (v != p && !vis[v]) dfs(v, u);
}
```

### Iterative DFS (when recursion depth is a risk)

```cpp
vector<int> st{s};
vector<bool> vis(n + 1);
while (!st.empty()) {
    int u = st.back(); st.pop_back();
    if (vis[u]) continue;
    vis[u] = true;
    for (int v : adj[u]) if (!vis[v]) st.push_back(v);
}
```
**Note:** this gives a DFS *order*, not a proper DFS tree with enter/exit times. For enter/exit you need an explicit iterator-per-node stack.

**Recursion depth:** a path graph with n = 1e6 gives 1e6 nested calls. On CF (stack = memory limit) this usually survives, but it's fragile. Convert to iterative if n ≥ 1e6 or the frame is heavy.

---

## Topological sort

### Kahn's (BFS)

```cpp
vector<int> indeg(n + 1, 0);
for (int u = 1; u <= n; u++) for (int v : adj[u]) indeg[v]++;
queue<int> q;
for (int u = 1; u <= n; u++) if (!indeg[u]) q.push(u);
vector<int> order;
while (!q.empty()) {
    int u = q.front(); q.pop();
    order.push_back(u);
    for (int v : adj[u]) if (--indeg[v] == 0) q.push(v);
}
bool hasCycle = ((int)order.size() != n);
```
**Bonus:** use a `priority_queue` instead of `queue` for the lexicographically smallest topological order.
**Cycle detection comes free** — if fewer than n nodes come out, there's a cycle.

### DFS colors (for directed cycle detection)

```cpp
// 0 = unvisited, 1 = in stack, 2 = done
bool dfs(int u) {
    color[u] = 1;
    for (int v : adj[u]) {
        if (color[v] == 1) return true;              // back edge → cycle
        if (color[v] == 0 && dfs(v)) return true;
    }
    color[u] = 2;
    return false;
}
```
**Trap:** "visited" alone is not enough for directed cycles — you need the three-state coloring. A node already finished (`color 2`) is not a cycle.

---

## Dijkstra

```cpp
vector<ll> dist(n + 1, LLONG_MAX);
priority_queue<pll, vector<pll>, greater<pll>> pq;   // (dist, node) — MIN heap
dist[s] = 0; pq.push({0, s});
while (!pq.empty()) {
    auto [d, u] = pq.top(); pq.pop();
    if (d > dist[u]) continue;                       // lazy deletion — STALE entry
    for (auto [v, w] : adj[u]) {
        if (dist[u] + w < dist[v]) {
            dist[v] = dist[u] + w;
            pq.push({dist[v], v});
        }
    }
}
```

**Trap 1 — `dist` must be `long long`.** n = 2e5 nodes, weights up to 1e9 → paths up to 2e14. `int` overflows.
**Trap 2 — `(dist, node)` order in the pair.** The heap sorts by `.first`, so distance must come first.
**Trap 3 — the `if (d > dist[u]) continue;` line.** Without it you reprocess stale entries and the complexity degrades (though it stays correct).
**Trap 4 — negative weights break Dijkstra.** Silently. It will produce a wrong answer, not an error.
**Trap 5 — `LLONG_MAX` and then `dist[u] + w`** overflows. Either skip unreachable nodes (the `continue` handles it since they're never popped) or use `LINF = 4e18`... which still overflows on `+w`. Safest: `const ll INF = 2e18;` or check `dist[u] != INF` before relaxing.

### 0-1 BFS

```cpp
deque<int> dq;
vector<int> dist(n, INT_MAX);
dist[s] = 0; dq.push_front(s);
while (!dq.empty()) {
    int u = dq.front(); dq.pop_front();
    for (auto [v, w] : adj[u]) {
        if (dist[u] + w < dist[v]) {
            dist[v] = dist[u] + w;
            if (w == 0) dq.push_front(v);
            else        dq.push_back(v);
        }
    }
}
```
**Memory hook:** *free edge → jump to the front of the line. Costly edge → go to the back.*
O(n + m) — no log factor. Use whenever weights are only 0 and 1 (very common in grid problems: "moving in the current direction is free, turning costs 1").

---

## Bellman–Ford

```cpp
vector<ll> dist(n + 1, LINF);
dist[s] = 0;
for (int i = 0; i < n - 1; i++)
    for (auto [w, u, v] : edges)
        if (dist[u] < LINF && dist[u] + w < dist[v]) dist[v] = dist[u] + w;

// negative cycle detection: one more pass
bool neg = false;
for (auto [w, u, v] : edges)
    if (dist[u] < LINF && dist[u] + w < dist[v]) neg = true;
```
**Use when:** negative edges exist, or you need to detect a negative cycle.
**Complexity:** O(n·m) — only viable for n·m ≤ ~1e7.
**Trap:** the `dist[u] < LINF` guard, or you relax from unreachable nodes and get garbage (and overflow).

**SPFA** (queue-based Bellman–Ford) is O(n·m) worst case and there are anti-SPFA tests on Codeforces. Don't rely on it.

---

## Floyd–Warshall

```cpp
// d[i][j] initialized to INF, d[i][i] = 0, edges filled in
for (int k = 1; k <= n; k++)
  for (int i = 1; i <= n; i++)
    for (int j = 1; j <= n; j++)
      if (d[i][k] + d[k][j] < d[i][j])
          d[i][j] = d[i][k] + d[k][j];
```
**Trap 1 — the loop order is `k, i, j`.** `k` outermost. Any other order is wrong. Memory hook: *the intermediate node goes on the outside.*
**Trap 2 — INF overflow.** With `int INF = 1e9`, `d[i][k] + d[k][j]` = 2e9 → overflow → negative → false "shortest path". Use `0x3f3f3f3f`, or `long long`, or guard `if (d[i][k] < INF && d[k][j] < INF)`.
**Complexity:** O(n³). n = 500 → 1.25e8, fine. n = 1000 → 1e9, too slow (and 8 MB with int, 4 MB... actually `int d[1000][1000]` = 4 MB, fine memory-wise, but too slow).

**Also computes:** transitive closure (with `|=` and bool/bitset), min-max path (replace `+` with `max`, `min` with `min`), and any semiring variant.

---

## Bipartite check

```cpp
vector<int> col(n + 1, -1);
bool bfs(int s) {
    queue<int> q; col[s] = 0; q.push(s);
    while (!q.empty()) {
        int u = q.front(); q.pop();
        for (int v : adj[u]) {
            if (col[v] == -1) { col[v] = col[u] ^ 1; q.push(v); }
            else if (col[v] == col[u]) return false;
        }
    }
    return true;
}
```
**Trap:** run it from **every** uncolored node — the graph may be disconnected.
A graph is bipartite iff it has no odd cycle.

---

## Connected components

```cpp
int comps = 0;
for (int i = 1; i <= n; i++)
    if (!vis[i]) { comps++; bfs(i); }
```
Or DSU: `comps = n - (number of successful unions)`.

---

## Kruskal (MST)

```cpp
sort(edges.begin(), edges.end());      // by weight — array<int,3>{w,u,v} sorts correctly
DSU dsu(n);
ll total = 0; int used = 0;
for (auto [w, u, v] : edges) {
    if (dsu.unite(u, v)) { total += w; used++; }
}
bool connected = (used == n - 1);
```
**Complexity:** O(m log m) dominated by the sort.
**Trap:** check `used == n - 1` — the graph might be disconnected and there's no spanning tree.

---

## Common graph traps

```
[ ] undirected edge added in only one direction
[ ] 1-indexed input into a 0-indexed array
[ ] graph is disconnected — loop over all starting nodes
[ ] self-loops (u == v)
[ ] multi-edges (same pair appears twice) — breaks "adj[u].size() == degree" assumptions
[ ] n nodes, m = 0 edges
[ ] dist array not long long
[ ] INF + w overflowing
[ ] visited not reset between test cases
[ ] recursion depth on a path graph
[ ] Dijkstra with negative weights (silently wrong)
[ ] tree assumed connected when it's a forest
[ ] m can exceed n-1 even when the problem says "tree" (read carefully)
```

---

## Distances and overflow — the rule

| Setting | Max distance | Type |
|---|---|---|
| unweighted, n ≤ 1e6 | 1e6 | `int` |
| weights ≤ 1e9, n ≤ 2e5 | 2e14 | **`long long`** |
| weights ≤ 1e9, n ≤ 1000 (Floyd) | 1e12 | **`long long`** |
| grid BFS | rows·cols | `int` |

**Default habit: weighted graph → `long long` distances.** The cost of being wrong is a WA on test 30.