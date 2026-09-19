# 20 — Trees

A tree on n nodes has exactly **n − 1** edges, is connected, and is acyclic. Any two of those three imply the third.

---

## Facts worth having reflexively

- n nodes → n − 1 edges. If the input gives you n − 1 edges and says "connected", it's a tree.
- There is **exactly one** simple path between any two nodes.
- Removing any edge splits it into exactly two components of sizes `s` and `n − s`.
- A tree is bipartite (2-colour by depth parity).
- Sum of all degrees = 2(n − 1).
- A tree with ≥ 2 nodes has at least 2 leaves.
- Number of labelled trees on n nodes = `n^(n−2)` (Cayley).
- A path graph is the worst case for recursion depth.

---

## Rooting and basic DFS

```cpp
vector<int> par(n+1), dep(n+1), sz(n+1);
void dfs(int u, int p) {
    par[u] = p; sz[u] = 1;
    for (int v : adj[u]) if (v != p) {
        dep[v] = dep[u] + 1;
        dfs(v, u);
        sz[u] += sz[v];
    }
}
```
**Trap:** `if (v != p)` fails with **multi-edges** (two edges between u and p). Track the edge index instead of the parent node when the graph might have parallel edges.

**Trap:** depth 1e6 on a path graph. Convert to an iterative DFS or increase your confidence in the judge's stack.

---

## Subtree sizes and the split trick

`sz[u]` = number of nodes in u's subtree. Removing the edge `(u, par[u])` splits the tree into `sz[u]` and `n − sz[u]`.

**Use for:** counting pairs separated by an edge (`sz[u] · (n − sz[u])`), sum of distances, "how many subtrees have property P".

**Classic:** the sum over all edges of `sz[u] · (n − sz[u])` = the sum of distances over all pairs (each edge is counted once per pair that crosses it).

---

## Tree diameter

### Two-BFS method (unweighted, or non-negative weights)

```cpp
// 1. BFS from any node → find the farthest node a
// 2. BFS from a → the farthest node b; dist(a,b) is the diameter
```
**Why it works:** the farthest node from *any* start is always an endpoint of some diameter. (Proof by contradiction on the paths' intersection.)
**Trap:** this fails on general graphs and on trees with **negative** edge weights. Trees with non-negative weights are fine.

### DP method (works with anything, gives more info)

```cpp
int diam = 0;
int dfsD(int u, int p) {                 // returns the deepest descent from u
    int best1 = 0, best2 = 0;
    for (int v : adj[u]) if (v != p) {
        int d = dfsD(v, u) + 1;
        if (d > best1) { best2 = best1; best1 = d; }
        else if (d > best2) best2 = d;
    }
    diam = max(diam, best1 + best2);     // path through u
    return best1;
}
```
**The pattern:** keep the **two deepest** children; the answer through `u` is their sum. This "top-2 children" shape recurs constantly in tree DP.

---

## LCA — binary lifting

```cpp
int LOG, up[20][MAXN], dep[MAXN];
void dfs(int u, int p) {
    up[0][u] = p;
    for (int k = 1; k < LOG; k++) up[k][u] = up[k-1][ up[k-1][u] ];
    for (int v : adj[u]) if (v != p) { dep[v] = dep[u] + 1; dfs(v, u); }
}
int lca(int a, int b) {
    if (dep[a] < dep[b]) swap(a, b);
    int d = dep[a] - dep[b];
    for (int k = 0; k < LOG; k++) if (d >> k & 1) a = up[k][a];
    if (a == b) return a;
    for (int k = LOG-1; k >= 0; k--)
        if (up[k][a] != up[k][b]) { a = up[k][a]; b = up[k][b]; }
    return up[0][a];
}
```
**Init:** `LOG = __lg(n) + 1;` and `dfs(root, root)` — the root is its own parent, which makes the lifting loop terminate cleanly.
**Build:** O(n log n) time and memory. **Query:** O(log n).
**Distance:** `dep[a] + dep[b] − 2·dep[lca(a,b)]`.
**k-th ancestor:** the first loop of `lca`, alone.

**Trap:** the second loop goes **downward from LOG−1** and only jumps when the ancestors **differ**. Jumping when they're equal overshoots past the LCA.

**Alternatives:** Euler tour + sparse table (O(1) query, O(n log n) build); offline Tarjan with DSU (O(n α) — full code in `30_SUFFIX_STRUCTURES_AND_PERSISTENCE.md`).

---

## Euler tour — subtree becomes a contiguous range

```cpp
int timer = 0;
vector<int> tin(n+1), tout(n+1), flat(n+1);
void dfs(int u, int p) {
    tin[u] = timer;
    flat[timer++] = u;
    for (int v : adj[u]) if (v != p) dfs(v, u);
    tout[u] = timer;              // subtree of u = flat[ tin[u] .. tout[u]-1 ]
}
```
**This is the key idea for subtree queries:** after flattening, "update everything in u's subtree" is a **range update** on an array, so a Fenwick or segment tree handles it.

**Ancestor test:** `u` is an ancestor of `v` iff `tin[u] <= tin[v] && tout[v] <= tout[u]`. O(1), no LCA needed.

**Variant for path queries:** record `tin` on entry and `tout` on exit as two separate positions (an Euler tour of length 2n), then a path-to-root sum becomes a prefix query. Full path queries between arbitrary nodes need HLD.

---

## Rerooting (all-roots DP in O(n))

**Trigger:** "compute f(v) for every node v as the root", where the naive approach is O(n²).

Two passes:
1. **Down:** `down[u]` = the answer within u's subtree. Standard post-order DFS.
2. **Up:** `up[u]` = the answer from everything *outside* u's subtree, computed from the parent's `up` plus the parent's other children's `down`.

```cpp
// example: sum of distances from each node to all others
void dfs1(int u, int p) {
    sz[u] = 1; down[u] = 0;
    for (int v : adj[u]) if (v != p) {
        dfs1(v, u);
        sz[u] += sz[v];
        down[u] += down[v] + sz[v];
    }
}
void dfs2(int u, int p) {
    for (int v : adj[u]) if (v != p) {
        ans[v] = ans[u] - sz[v] + (n - sz[v]);
        dfs2(v, u);
    }
}
// ans[root] = down[root]; then dfs2(root, root);
```
**The general trick for combining "all children except v":** prefix and suffix aggregates over the child list, so you can exclude one child in O(1). Subtraction works only when the combine operation is invertible (sum yes, max no).

---

## Small-to-large merging

**Trigger:** "for each subtree, report something about the multiset of values in it."

```cpp
map<int,int>* dfs(int u, int p) {
    auto* res = new map<int,int>();
    (*res)[val[u]]++;
    for (int v : adj[u]) if (v != p) {
        auto* child = dfs(v, u);
        if (child->size() > res->size()) swap(res, child);   // ALWAYS merge small into large
        for (auto& [k, c] : *child) (*res)[k] += c;
        delete child;
    }
    // answer query for u using *res
    return res;
}
```
**Complexity O(n log² n)** with a map (O(n log n) with a hash set). Each element moves only when it joins a set at least twice as large, so it moves ≤ log n times.

**The swap line is the whole algorithm.** Without it, it's O(n²).

**DSU on tree (sack)** is the array-based version of the same idea: keep one global counter array, recurse into light children and clear them, recurse last into the heavy child and keep it. Same O(n log n), much faster constant.

---

## Heavy-light decomposition (know the shape)

Decompose the tree into vertical chains so that any root-to-node path crosses **O(log n)** chains. Each chain is contiguous in the Euler order, so a segment tree over the flattened array answers path queries in O(log² n).

**Use when:** path updates *and* path queries on a tree with updates. If there are no updates, LCA + prefix sums from the root is far simpler.

Keep it as a template; don't try to write it from memory in a contest. Full code in `27_ADVANCED_STRINGS_AND_STRUCTURES.md`, alongside centroid decomposition (also full code there — this file only gives the centroid-finding function above).

---

## Common tree problems and their shapes

| Problem | Approach |
|---|---|
| Distance between two nodes | LCA + depths |
| Sum of all pairwise distances | per-edge contribution `sz·(n−sz)` |
| Diameter | two BFS, or top-2 children DP |
| Max independent set | `dp[u][0/1]`, take/skip |
| Min vertex cover | n − max independent set (on trees) |
| Centroid (removing it leaves all parts ≤ n/2) | walk from the root toward the heaviest child |
| Count paths with property P | centroid decomposition, or per-edge/per-node contribution |
| Subtree sum/update queries | Euler tour + Fenwick |
| Path queries with updates | HLD |
| k-th ancestor | binary lifting |
| Colours in each subtree | small-to-large / DSU on tree |
| Is u an ancestor of v | `tin`/`tout` comparison |

---

## Centroid

A centroid is a node whose removal leaves every component of size ≤ n/2. Every tree has one or two.

```cpp
int findCentroid(int u, int p, int n) {
    for (int v : adj[u]) if (v != p && sz[v] > n / 2) return findCentroid(v, u, n);
    return u;
}
```
**Use for:** centroid decomposition (counting paths with a property in O(n log n)), and for "root the tree so it's balanced".

---

## Traps

```
[ ] tree given as n-1 edges, but you read n
[ ] 1-indexed input, 0-indexed arrays
[ ] recursion depth on a path graph (n = 1e5 or 1e6)
[ ] "if (v != p)" broken by parallel edges
[ ] n = 1 (no edges at all — loops that read n-1 edges do nothing, and that's correct)
[ ] forgot the tree can be a star (one node with degree n-1)
[ ] LCA arrays not sized LOG x n, or LOG too small
[ ] root not set as its own parent in binary lifting
[ ] tree DP answer read from the root only, when the answer can be at any node
[ ] diameter DP: forgot to combine the two deepest children
[ ] rerooting with a non-invertible operation (max) using subtraction
```