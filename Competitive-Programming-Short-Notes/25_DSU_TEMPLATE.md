# 25 — DSU (Disjoint Set Union)

> Pre-contest revision lives in `17_PRE_CONTEST_REVISION.md`; the DSU-flavored offline LCA is in `30_SUFFIX_STRUCTURES_AND_PERSISTENCE.md`.

---

## The template

```cpp
struct DSU {
    vector<int> p, sz;
    int comps;
    DSU(int n) : p(n + 1), sz(n + 1, 1), comps(n) {
        iota(p.begin(), p.end(), 0);
    }
    int find(int x) {
        return p[x] == x ? x : p[x] = find(p[x]);      // path compression
    }
    bool unite(int a, int b) {
        a = find(a); b = find(b);
        if (a == b) return false;                       // already together
        if (sz[a] < sz[b]) swap(a, b);                  // union by size
        p[b] = a;
        sz[a] += sz[b];
        comps--;
        return true;
    }
    bool same(int a, int b) { return find(a) == find(b); }
    int size(int x) { return sz[find(x)]; }
};
```

**Three things this gives you for free:**
1. `unite` returns whether anything changed — that's your Kruskal "is this edge in the MST" test **and** your cycle detector.
2. `comps` tracks the number of components with no extra work.
3. `size(x)` is the size of `x`'s component.

---

## Why it's near O(1)

- **Path compression alone:** O(log n) amortized.
- **Union by size/rank alone:** O(log n) worst case.
- **Both together:** O(α(n)) amortized, where α is the inverse Ackermann function. α(n) < 5 for any n you will ever see (α(2^65536) = 5).

Treat it as constant time. It is not literally constant, but the difference never matters.

**Note the recursive `find` with path compression:** `p[x] = find(p[x])` flattens the whole path on the way back up. Depth is bounded by O(log n) before compression, so recursion is safe.

Iterative version if you're paranoid about depth:
```cpp
int find(int x) {
    int r = x;
    while (p[r] != r) r = p[r];
    while (p[x] != r) { int nx = p[x]; p[x] = r; x = nx; }
    return r;
}
```

---

## Applications

### Connected components

```cpp
DSU d(n);
for (auto [u, v] : edges) d.unite(u, v);
cout << d.comps;
```

### Cycle detection in an undirected graph

```cpp
for (auto [u, v] : edges)
    if (!d.unite(u, v)) { /* this edge closes a cycle */ }
```
**Trap:** this is for **undirected** graphs. Directed cycle detection needs DFS colors or Kahn's algorithm.

### Kruskal MST

```cpp
sort(edges.begin(), edges.end());          // array<int,3>{w, u, v}
DSU d(n);
ll total = 0; int used = 0;
for (auto [w, u, v] : edges)
    if (d.unite(u, v)) { total += w; used++; }
if (used != n - 1) { /* graph is disconnected — no spanning tree */ }
```

### Offline connectivity (process queries in reverse)

**Trigger:** "edges are being removed; after each removal, report connectivity."
Removal is hard for DSU; addition is trivial. So:
1. Read all queries.
2. Build the final graph (after all removals).
3. Process queries **backwards**, adding edges back.
4. Reverse the answers.

This is heuristic #2 ("reverse the process") in its purest form.

### Merging accounts / grouping by equivalence

Anything phrased as "a and b are the same" transitively. Grouping anagrams, merging email accounts, "these two cells are connected".

### Counting pairs in components

After all unions: `Σ over roots of C(sz, 2)` = number of connected pairs.

### Small-to-large with extra data

Attach a `vector` or `set` to each root and always merge the smaller into the larger:
```cpp
if (data[a].size() < data[b].size()) swap(data[a], data[b]);
for (int x : data[b]) data[a].insert(x);
data[b].clear();
```
Total O(n log n) — each element moves at most log n times.

---

## Variants

### Weighted DSU (distance to root)

Tracks a value relative to the component's root — solves "a is exactly d more than b, are the constraints consistent?"

```cpp
vector<int> p, w;            // w[x] = weight from x to p[x]
int find(int x) {
    if (p[x] == x) return x;
    int r = find(p[x]);
    w[x] += w[p[x]];         // accumulate on the way back up
    p[x] = r;
    return r;
}
```
**Trap:** the weight accumulation must happen **after** the recursive call, before reassigning `p[x]`.

### Bipartiteness / 2-coloring DSU

Store a parity bit instead of a weight: `w[x] ∈ {0,1}` = color relative to the root. Uniting `u` and `v` with "different colors" checks `find(u) == find(v) && w[u] == w[v]` → contradiction → not bipartite.

### DSU with rollback

Drop path compression (keep union by size, O(log n) find), and push every change onto a stack so you can undo it:
```cpp
vector<pair<int,int>> history;         // push {b, sz[b]} right before each merge
void rollback() {
    auto [b, oldsz] = history.back(); history.pop_back();
    sz[p[b]] -= oldsz; p[b] = b;       // oldsz = size of the attached component at merge time
}
```
**Needed for:** offline dynamic connectivity (divide & conquer over time), "DSU on a segment tree".

---

## Traps

```
[ ] p not initialized with iota (or 0..n loop)
[ ] 1-indexed graph but DSU sized n instead of n+1
[ ] calling find() and then comparing p[a] == p[b] instead of find(a) == find(b)
[ ] forgetting that unite() returns false for already-connected nodes
[ ] not resetting DSU between test cases
[ ] using DSU for directed cycle detection (wrong tool)
[ ] weighted DSU: weight updated before the recursive find
[ ] rollback DSU with path compression (breaks undo)
[ ] expecting DSU to support deletion (it can't — go offline and reverse)
```

---

## Memory hooks

- *`find` returns the root; **never** read `p[x]` directly.*
- *`unite` returns "did something change" — that's your MST test and your cycle test.*
- *DSU adds. It never removes. If the problem removes, run time backwards.*
- *Both optimizations together, or you lose the α(n).*