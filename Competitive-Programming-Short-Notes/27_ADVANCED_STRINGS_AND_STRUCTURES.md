# 27 — Advanced Strings & Structures

Manacher's, Aho-Corasick, heavy-light decomposition, centroid decomposition, convex hull trick, Li Chao tree.
Several of these were named as "keep it as a template" elsewhere in this handbook without ever being given — this is where the actual code lives.

---

## Manacher's algorithm — all palindromic substrings in O(n)

Gives, for every center, the radius of the longest palindrome there — without the `'#'`-padding trick, using two arrays for odd and even lengths.

```cpp
vector<int> manacherOdd(const string& s) {          // d[i] = 1 + radius of the longest ODD palindrome centered at i
    int n = s.size();
    vector<int> d(n);
    int l = 0, r = -1;
    for (int i = 0; i < n; i++) {
        int k = (i > r) ? 1 : min(d[l + r - i], r - i + 1);
        while (i - k >= 0 && i + k < n && s[i - k] == s[i + k]) k++;
        d[i] = k--;
        if (i + k > r) { l = i - k; r = i + k; }
    }
    return d;                                        // longest odd palindrome at i has length 2*d[i]-1
}
vector<int> manacherEven(const string& s) {          // d[i] = radius of the longest EVEN palindrome centered between i-1, i
    int n = s.size();
    vector<int> d(n);
    int l = 0, r = -1;
    for (int i = 0; i < n; i++) {
        int k = (i > r) ? 0 : min(d[l + r - i + 1], r - i + 1);
        while (i - k - 1 >= 0 && i + k < n && s[i - k - 1] == s[i + k]) k++;
        d[i] = k--;
        if (i + k > r) { l = i - k - 1; r = i + k; }
    }
    return d;                                        // longest even palindrome at i has length 2*d[i]
}
```
**Complexity:** O(n), amortized — the same "never shrink the right boundary" argument as the Z-function.
**Use when:** "longest palindromic substring", "count palindromic substrings" (`Σ d1[i] + Σ d2[i]`), "is every prefix/suffix a palindrome". Beats the O(n²) expand-around-center approach shown in `08_STRING_TRICKS.md` when n exceeds ~5000.
**Trap:** the odd and even arrays are indexed differently — `manacherEven[i]` is centered **between** `i-1` and `i`, not at `i`. Off-by-one here is the standard bug.

---

## Aho-Corasick — multi-pattern matching in one pass

The natural generalization of KMP to many patterns at once: a Trie (see `08_STRING_TRICKS.md`) with **failure links**, turned into a full automaton so every character of the text costs exactly O(1).

```cpp
const int ALPHA = 26;
struct AhoCorasick {
    struct Node {
        int next[ALPHA];
        int fail = 0;
        bool terminal = false;
        vector<int> wordIds;                  // which patterns end here
        Node() { fill(next, next + ALPHA, -1); }
    };
    vector<Node> t;
    AhoCorasick() { t.emplace_back(); }

    void insert(const string& s, int id) {
        int v = 0;
        for (char c : s) {
            int ch = c - 'a';
            if (t[v].next[ch] == -1) { t[v].next[ch] = t.size(); t.emplace_back(); }
            v = t[v].next[ch];
        }
        t[v].terminal = true;
        t[v].wordIds.push_back(id);
    }

    void build() {                             // BFS: fail links + convert trie edges into automaton edges
        queue<int> q;
        for (int c = 0; c < ALPHA; c++) {
            if (t[0].next[c] == -1) t[0].next[c] = 0;
            else { t[t[0].next[c]].fail = 0; q.push(t[0].next[c]); }
        }
        while (!q.empty()) {
            int u = q.front(); q.pop();
            for (int c = 0; c < ALPHA; c++) {
                int v = t[u].next[c];
                if (v == -1) t[u].next[c] = t[t[u].fail].next[c];         // automaton fallback
                else { t[v].fail = t[t[u].fail].next[c]; q.push(v); }
            }
        }
    }
};
```

**Scanning the text:**
```cpp
int v = 0;
for (char c : text) {
    v = ac.t[v].next[c - 'a'];         // O(1) per character — the -1 slots were replaced during build()
    // ac.t[v].terminal tells you a pattern ends exactly here;
    // to catch patterns ending here via a SHORTER match, walk the fail chain, or precompute
    // "nearest terminal ancestor along fail links" with one more BFS pass if you need every match.
}
```

**Complexity:** O(Σ|pattern|·26) to build (dominated by the automaton fill), O(|text|) to scan — no `ALPHA` factor at scan time, because the trie's missing edges were pre-filled with their automaton fallback during `build()`.

**Use when:** "does the text contain any of these k patterns", "count total occurrences of any pattern", "match multiple keywords simultaneously". A generalization of both the Trie (`08_STRING_TRICKS.md`) and KMP (same file) — KMP is Aho-Corasick with one pattern.

**Trap:** same memory sizing rule as the Trie — size the node count to the **total length of all patterns**, not a guess; `ALPHA` × 4 bytes per node adds up fast (26 × 4 = 104 bytes/node).
**Trap:** `build()` must run before any query — the automaton fallback edges don't exist until then.

---

## Heavy-light decomposition

Decomposes a tree into O(log n) vertical chains so any root-to-node (or node-to-node) path touches at most O(log n) chains, each contiguous in an array — letting a segment tree answer **path** queries, not just subtree queries.

```cpp
vector<int> adj[MAXN];
int par[MAXN], depth_[MAXN], heavy[MAXN], head[MAXN], pos[MAXN], sz[MAXN];
int curPos = 0;

int dfsSize(int u, int p) {
    par[u] = p; sz[u] = 1; int maxSub = 0; heavy[u] = -1;
    for (int v : adj[u]) if (v != p) {
        depth_[v] = depth_[u] + 1;
        sz[u] += dfsSize(v, u);
        if (sz[v] > maxSub) { maxSub = sz[v]; heavy[u] = v; }
    }
    return sz[u];
}
void decompose(int u, int h) {
    head[u] = h; pos[u] = curPos++;
    if (heavy[u] != -1) decompose(heavy[u], h);                       // continue the SAME chain
    for (int v : adj[u]) if (v != par[u] && v != heavy[u]) decompose(v, v);  // each starts a NEW chain
}
// init: dfsSize(root, -1); decompose(root, root);
// build a segment tree over an array of size n, placing value[u] at pos[u]

ll pathQuery(int u, int v, SegTree& seg) {          // merge must be commutative/order-independent here
    ll res = 0;                                      // or the identity of your merge operation
    while (head[u] != head[v]) {
        if (depth_[head[u]] < depth_[head[v]]) swap(u, v);
        res = merge(res, seg.query(pos[head[u]], pos[u]));
        u = par[head[u]];
    }
    if (depth_[u] > depth_[v]) swap(u, v);
    res = merge(res, seg.query(pos[u], pos[v]));
    return res;
}
```
**Subtree queries** use the range `[pos[u], pos[u] + sz[u] - 1]` — same contiguity idea as the Euler tour in `20_TREES.md`.
**Complexity:** O(n) build (after the O(n) sizing pass), O(log² n) per path query (O(log n) chain jumps × O(log n) segment tree query).
**Trap — edge weights:** if values live on edges rather than nodes, store each edge's weight at its **child's** position, and when querying a path, exclude the LCA's own position (it doesn't represent an edge). Compute the LCA first, or handle it by never including `pos[lca]` in the query range.
**Use when:** path updates *and* path queries on a tree that also changes over time. With no updates, LCA + prefix sums from the root (`20_TREES.md`) is far simpler — don't reach for HLD by default.

---

## Centroid decomposition

Repeatedly finds the centroid of the (remaining) tree, "solves" everything routed through it, then recurses into the pieces left after removing it. Because a centroid split at least halves every remaining piece, the recursion has depth O(log n), giving O(n log n) total for many "count paths with property P" problems that would otherwise be O(n²).

```cpp
int sz[MAXN]; bool removed[MAXN];

int getSize(int u, int p) {
    sz[u] = 1;
    for (int v : adj[u]) if (v != p && !removed[v]) sz[u] += getSize(v, u);
    return sz[u];
}
int getCentroid(int u, int p, int treeSize) {
    for (int v : adj[u]) if (v != p && !removed[v] && sz[v] > treeSize / 2) return getCentroid(v, u, treeSize);
    return u;
}
void decompose(int u) {
    int treeSize = getSize(u, -1);
    int c = getCentroid(u, -1, treeSize);
    removed[c] = true;

    // ---- problem-specific work goes here: process every path passing through c ----
    // typical shape: one DFS from c collecting (depth or accumulated sum) per node in EVERY
    // subtree combined, then one more pass PER CHILD SUBTREE to subtract paths that stay
    // entirely within that one child (inclusion–exclusion — otherwise a path through two
    // children of c gets double counted as if it also stayed inside a single child).

    for (int v : adj[c]) if (!removed[v]) decompose(v);
}
// call decompose(0) once; the implicit recursion IS the centroid tree
```
**Complexity:** O(n log n) total, since each node participates in O(log n) different centroid subproblems (the "centroid tree" formed by parent-pointers between successive centroids has depth O(log n)).
**Use when:** "count/find paths in a tree satisfying P" (length exactly k, sum divisible by m, sum in a range) where a per-pair O(n²) check is too slow. Also underlies the tree-distance-query structures used in some Div1 problems.
**Trap:** forgetting the inclusion–exclusion subtraction step double-counts every path that passes through the centroid via two different children as if the whole path lived in one child's subtree.
**Trap:** `getSize`/`getCentroid` must respect `removed[]`, or later calls will wander into already-decomposed pieces.

---

## Virtual tree (auxiliary tree)

**Recognition:** multiple queries, each naming a small subset of "important" nodes in a large fixed tree (with the **sum of subset sizes across all queries** bounded, e.g. `Σk ≤ 2·10^5`, even though `n` itself is large) — and for each query you need a tree DP or computation that would cost O(n) if run on the whole tree.

**The idea:** build the smallest tree that contains the `k` marked nodes plus every pairwise LCA needed to connect them, with each virtual edge representing a compressed path in the real tree. This virtual tree has only O(k) nodes, so a DP restricted to it costs O(k) instead of O(n) — and building it costs only O(k log k) despite considering a tree of size n.

**The key fact that makes O(k log k) possible:** you do **not** need all `C(k,2)` pairwise LCAs — only the LCAs of **consecutive** marked nodes after sorting by Euler-tour entry time (`tin[]`, from `20_TREES.md`) are ever needed as extra junction nodes.

```cpp
vector<int> nodes = markedNodes;
sort(nodes.begin(), nodes.end(), [](int a, int b){ return tin[a] < tin[b]; });
int k = nodes.size();
for (int i = 0; i + 1 < k; i++) nodes.push_back(lca(nodes[i], nodes[i+1]));   // consecutive LCAs only
sort(nodes.begin(), nodes.end(), [](int a, int b){ return tin[a] < tin[b]; });
nodes.erase(unique(nodes.begin(), nodes.end()), nodes.end());

vector<int> stk = {nodes[0]};
vector<pair<int,int>> virtualEdges;
bool isAncestor(int u, int v) { return tin[u] <= tin[v] && tout[v] <= tout[u]; }   // from 20_TREES.md

for (int i = 1; i < (int)nodes.size(); i++) {
    int u = nodes[i];
    while (!isAncestor(stk.back(), u)) stk.pop_back();
    virtualEdges.push_back({stk.back(), u});
    stk.push_back(u);
}
```
**Complexity:** O(k log k) — dominated by the two sorts and `k` LCA queries (O(log n) each via binary lifting, `20_TREES.md`).
**Use when:** the same large tree is queried many times, each query only caring about a small, different subset of nodes, and a per-query O(n) DP is too slow in aggregate but a per-query O(k log k) build + O(k) DP fits the sum-of-k bound.
**Trap:** this is one of the fiddlier constructions in this handbook — the "consecutive LCAs are sufficient" step and the stack-based edge-building step both have off-by-one/ordering traps that only show up on specific tree shapes. Stress-test against a brute-force O(n) DP on small random trees before trusting it.

---

## Convex hull trick (CHT) — monotonic case

**Use when:** DP transitions of the shape `dp[i] = min_j (m_j · x_i + b_j)` (or `max`), turning an O(n²) DP into O(n) or O(n log n). See the DP optimization table in `19_DP_PATTERNS.md`.

The simple, contest-common version: lines are added with **monotonically decreasing slope**, and queries come in **monotonically increasing x** — both monotonic means a moving pointer gives O(1) amortized per query.

```cpp
struct CHT {
    vector<ll> M, B;
    int ptr = 0;
    bool bad(int l1, int l2, int l3) {                                  // is line l2 ever optimal? if not, drop it
        return (B[l3] - B[l1]) * (M[l1] - M[l2]) <= (B[l2] - B[l1]) * (M[l1] - M[l3]);
    }
    void add(ll m, ll b) {                                              // slopes must arrive DECREASING
        M.push_back(m); B.push_back(b);
        while (M.size() >= 3 && bad(M.size() - 3, M.size() - 2, M.size() - 1)) {
            M.erase(M.end() - 2); B.erase(B.end() - 2);
        }
    }
    ll query(ll x) {                                                    // x must be non-decreasing across calls
        if (ptr >= (int)M.size()) ptr = (int)M.size() - 1;
        while (ptr + 1 < (int)M.size() && M[ptr+1]*x + B[ptr+1] <= M[ptr]*x + B[ptr]) ptr++;
        return M[ptr] * x + B[ptr];
    }
};
```
**Trap:** if either monotonicity requirement doesn't actually hold in your problem, this silently gives wrong answers — use the Li Chao tree below instead.

---

## Li Chao tree — arbitrary insertion and query order

Same family of problems as CHT, but lines can be added and queried in **any** order, at the cost of O(log C) per operation (`C` = size of the x-coordinate range) instead of amortized O(1).

```cpp
struct Line { ll m, b; ll eval(ll x) const { return m * x + b; } };
struct LiChao {
    struct Node { Line line{0, LLONG_MAX}; Node *l = nullptr, *r = nullptr; };
    Node* root = new Node();
    ll XLO, XHI;
    LiChao(ll lo, ll hi) : XLO(lo), XHI(hi) {}

    void add(Node* node, ll lo, ll hi, Line nw) {                       // minimizes; negate for maximum
        ll mid = lo + (hi - lo) / 2;
        bool leftBetter = nw.eval(lo)  < node->line.eval(lo);
        bool midBetter  = nw.eval(mid) < node->line.eval(mid);
        if (midBetter) swap(node->line, nw);
        if (lo == hi) return;
        if (leftBetter != midBetter) {
            if (!node->l) node->l = new Node();
            add(node->l, lo, mid, nw);
        } else {
            if (!node->r) node->r = new Node();
            add(node->r, mid + 1, hi, nw);
        }
    }
    ll query(Node* node, ll lo, ll hi, ll x) {
        if (!node) return LLONG_MAX;
        ll res = node->line.eval(x);
        if (lo == hi) return res;
        ll mid = lo + (hi - lo) / 2;
        return x <= mid ? min(res, query(node->l, lo, mid, x)) : min(res, query(node->r, mid + 1, hi, x));
    }
    void add(Line nw) { add(root, XLO, XHI, nw); }
    ll query(ll x) { return query(root, XLO, XHI, x); }
};
```
**Set `XLO`/`XHI` to the actual range of query x-values** — an unnecessarily wide range just adds depth for nothing.
**Use when:** CHT's monotonicity doesn't hold (offline problems where you'd need to sort twice in incompatible orders, or online problems where queries and insertions interleave unpredictably).

---

## Traps

```
[ ] Manacher: mixed up which array is centered where (odd at i, even between i-1 and i)
[ ] Aho-Corasick: queried before calling build()
[ ] Aho-Corasick: only checked .terminal at the current node, missing shorter matches ending here
[ ] Aho-Corasick / Trie: node array sized as a guess instead of total pattern length
[ ] HLD: path query merge assumed order-independent when the operation isn't commutative
[ ] HLD: edge-weighted tree query included the LCA's own array position
[ ] Centroid decomposition: forgot the inclusion-exclusion subtraction per child, double-counting paths
[ ] Centroid decomposition: getSize/getCentroid didn't respect removed[]
[ ] CHT: slopes not actually monotonic decreasing, or queries not actually monotonic increasing
[ ] Li Chao: XLO/XHI too narrow for the actual query range
```

---

## Memory hooks

- *Manacher odd is centered ON i; Manacher even is centered BETWEEN i-1 and i.*
- *Aho-Corasick = Trie + fail links + automaton fallback — KMP generalized to many patterns.*
- *HLD: heavy child continues the chain; every other child starts a new one.*
- *Centroid decomposition: split, solve through the centroid, subtract per-child double-counts, recurse.*
- *CHT needs both sides monotonic; Li Chao needs neither, at a log-factor cost.*