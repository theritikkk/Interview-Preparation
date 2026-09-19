# 30 — Suffix Structures & Persistence

Suffix array + Kasai's LCP, suffix automaton, persistent segment tree, offline LCA. The Trie/KMP/Z-function/hashing in `08_STRING_TRICKS.md` cover most string needs — reach for these when a problem needs something those can't give you.

---

## Suffix array — O(n log² n) construction

**What it is:** the starting indices of every suffix of `s`, sorted lexicographically. Enables binary search for a pattern in O(|pattern| log n), and — combined with the LCP array below — a huge range of substring problems.

```cpp
vector<int> buildSuffixArray(const string& s) {
    int n = s.size();
    vector<int> sa(n), rnk(n), tmp(n);
    for (int i = 0; i < n; i++) { sa[i] = i; rnk[i] = s[i]; }
    for (int k = 1; k < n; k <<= 1) {
        auto cmp = [&](int a, int b) {
            if (rnk[a] != rnk[b]) return rnk[a] < rnk[b];
            int ra = (a + k < n) ? rnk[a+k] : -1;
            int rb = (b + k < n) ? rnk[b+k] : -1;
            return ra < rb;
        };
        sort(sa.begin(), sa.end(), cmp);
        tmp[sa[0]] = 0;
        for (int i = 1; i < n; i++)
            tmp[sa[i]] = tmp[sa[i-1]] + (cmp(sa[i-1], sa[i]) ? 1 : 0);
        rnk = tmp;
        if (rnk[sa[n-1]] == n - 1) break;          // all ranks distinct — done early
    }
    return sa;
}
```
**Complexity:** O(n log² n) — `log n` doublings, each an O(n log n) sort. An O(n log n) version exists using radix sort instead of `std::sort`; this simpler version is fast enough up to n ≈ 1e5–2e5, which covers most contest needs.

**Pattern search:** binary search over `sa` comparing `s.substr(sa[mid])` against the pattern — O(|pattern| log n) per query (avoid materializing the substring; compare character by character with an early exit).

---

## Kasai's algorithm — LCP array in O(n)

`lcp[i]` = length of the longest common prefix between the suffixes at `sa[i-1]` and `sa[i]` (adjacent in sorted order).

```cpp
vector<int> kasaiLCP(const string& s, const vector<int>& sa) {
    int n = s.size();
    vector<int> rnk(n), lcp(n, 0);
    for (int i = 0; i < n; i++) rnk[sa[i]] = i;
    int h = 0;
    for (int i = 0; i < n; i++) {
        if (rnk[i] > 0) {
            int j = sa[rnk[i] - 1];
            while (i + h < n && j + h < n && s[i+h] == s[j+h]) h++;
            lcp[rnk[i]] = h;
            if (h > 0) h--;
        } else h = 0;
    }
    return lcp;
}
```
**Complexity:** O(n) amortized — `h` drops by at most 1 per outer iteration but can only rise a total of O(n) times across the whole run.

**What this unlocks:**
- **Longest repeated substring:** `max(lcp)`.
- **Number of distinct substrings:** `n(n+1)/2 − Σ lcp[i]` (every substring is a prefix of some suffix; the LCP sum removes the double-counted repeats).
- **Longest common substring of two strings:** concatenate `s + '#' + t` (`'#'` not in either), build the suffix array and LCP array of the combined string, and take the max `lcp[i]` where `sa[i-1]` and `sa[i]` originate from **different** original strings.
- **A sorted-suffix segment tree/sparse table over `lcp`** gives O(1) LCP between *any* two suffixes (range-min over the LCP array between their ranks) — the suffix-array equivalent of LCA.

---

## Suffix automaton

**What it is:** the smallest automaton (DAG of states) that accepts exactly the substrings of `s` — a single structure with O(n) states and O(n) transitions (not O(n²)) representing all O(n²) substrings implicitly. Denser and more powerful than a suffix array for some problems, but the most error-prone template in this file — stress-test before trusting it under contest pressure.

```cpp
struct SuffixAutomaton {
    struct State { int len, link; map<char,int> next; };
    vector<State> st;
    int last, sz;

    SuffixAutomaton(int maxLen) {
        st.resize(2 * maxLen);            // a string of length n needs at most 2n-1 states
        st[0].len = 0; st[0].link = -1;
        sz = 1; last = 0;
    }
    void extend(char c) {
        int cur = sz++;
        st[cur].len = st[last].len + 1;
        int p = last;
        while (p != -1 && !st[p].next.count(c)) { st[p].next[c] = cur; p = st[p].link; }
        if (p == -1) st[cur].link = 0;
        else {
            int q = st[p].next[c];
            if (st[p].len + 1 == st[q].len) st[cur].link = q;
            else {
                int clone = sz++;
                st[clone] = st[q];               // copies len, link, next
                st[clone].len = st[p].len + 1;
                while (p != -1 && st[p].next[c] == q) { st[p].next[c] = clone; p = st[p].link; }
                st[q].link = st[cur].link = clone;
            }
        }
        last = cur;
    }
};
// build: SuffixAutomaton sam(s.size()); for (char c : s) sam.extend(c);
```
**`len[v]`** = length of the longest string represented by state `v`. **`link[v]`** (the "suffix link") points to the state representing the longest **proper suffix** of that string that is itself a distinct state — the suffix links form a tree.

**Complexity:** O(n log σ) build with `map` (σ = alphabet size), or O(n·σ) with a fixed `array<int,26>` instead of `map` for a small fixed alphabet.

**Common uses:**
- **Count distinct substrings:** `Σ_v (len[v] − len[link[v]])` over all states except the root.
- **Number of occurrences of each substring:** mark every state created directly by `extend` (not a clone) with count 1, then sum counts up the suffix-link tree from leaves to root (a states-in-decreasing-`len`-order pass, or a proper tree DFS on the link tree).
- **Longest common substring of `s` and `t`:** build the automaton on `s`, then feed `t` through it character by character, tracking the current state and match length; drop to a suffix-link ancestor (shortening the match) whenever the next character has no transition, exactly like the KMP failure-function idea.

**When to reach for this instead of suffix array + LCP:** occurrence-counting problems, or when the alphabet and structure make an automaton-style transition walk more natural than sorted-order binary search. For most substring problems, suffix array + Kasai's LCP is simpler to get right — prefer it unless the problem specifically needs automaton transitions.

---

## Persistent segment tree

**What it is:** a segment tree where every update creates a **new version** instead of mutating the old one, by allocating new nodes only along the O(log n) path that actually changed and reusing every other subtree unchanged. Lets you query *any past version* in O(log n).

```cpp
struct PersistentSegTree {
    struct Node { ll val; int l, r; };
    vector<Node> t;
    vector<int> roots;
    int n;

    PersistentSegTree(int n) : n(n) { roots.push_back(build(0, n - 1)); }

    int build(int lo, int hi) {
        int id = t.size(); t.push_back({0, -1, -1});
        if (lo == hi) return id;
        int mid = (lo + hi) / 2;
        t[id].l = build(lo, mid);
        t[id].r = build(mid + 1, hi);
        return id;
    }
    int update(int prev, int lo, int hi, int pos, ll val) {         // O(log n) NEW nodes, rest shared with prev
        int id = t.size(); t.push_back(t[prev]);
        if (lo == hi) { t[id].val = val; return id; }
        int mid = (lo + hi) / 2;
        if (pos <= mid) t[id].l = update(t[prev].l, lo, mid, pos, val);
        else            t[id].r = update(t[prev].r, mid + 1, hi, pos, val);
        t[id].val = t[t[id].l].val + t[t[id].r].val;
        return id;
    }
    ll query(int node, int lo, int hi, int ql, int qr) {
        if (qr < lo || hi < ql) return 0;
        if (ql <= lo && hi <= qr) return t[node].val;
        int mid = (lo + hi) / 2;
        return query(t[node].l, lo, mid, ql, qr) + query(t[node].r, mid + 1, hi, ql, qr);
    }
};
// new version after a point update:  roots.push_back(pst.update(roots.back(), 0, n-1, pos, val));
// query a SPECIFIC historical version: pst.query(roots[version], 0, n-1, l, r);
```
**Memory:** O((n + q) log n) nodes total, each a small fixed struct — the whole point is that unchanged subtrees are shared, not copied.
**Use when:** "k-th smallest value in range [l, r]" (build over coordinate-compressed values, one version per prefix of the array, answer via binary search on the tree using `query(roots[r]) − query(roots[l-1])`), "answer a query as of an earlier point in time", persistent Fenwick/DSU-flavored problems.
**Trap:** every `update` call must be seeded from the correct previous version's root, not always from `roots[0]` or the latest — mixing these up silently gives a tree that doesn't represent the version you think it does.

---

## Offline LCA — Tarjan's + DSU

When **all** LCA queries are known upfront (no online requirement), this avoids binary lifting's O(n log n) preprocessing in favor of O((n+q)·α(n)) — DSU-speed, one DFS.

```cpp
vector<pair<int,int>> queries[MAXN];     // queries[u] += {v, queryIndex} for every query (u, v) — add to BOTH u's and v's lists
int lcaAns[MAXQ];
int anc[MAXN];
bool done[MAXN];
DSU dsu(n);                               // from 25_DSU_TEMPLATE.md

void tarjanLCA(int u, int p) {
    anc[u] = u;
    for (int v : adj[u]) if (v != p) {
        tarjanLCA(v, u);
        dsu.unite(u, v);
        anc[dsu.find(u)] = u;
    }
    done[u] = true;
    for (auto [v, idx] : queries[u])
        if (done[v]) lcaAns[idx] = anc[dsu.find(v)];
}
```
**Trap:** each query `(u, v)` must be pushed into **both** `queries[u]` and `queries[v]` with the same index — you don't know in advance which endpoint's subtree finishes DFS-processing last, so whichever one does is the one that actually resolves the answer (guarded by the `done[v]` check on the other).
**Use when:** you have a batch of LCA queries known before the tree is even fully built, and don't need per-query online answers. If queries arrive interleaved with tree modifications, or you need online answers, use binary lifting instead (`20_TREES.md`).

---

## Traps

```
[ ] suffix array: comparator recomputed rnk lookups inconsistently between sort and rank-assignment passes
[ ] Kasai: forgot to decrement h by at most 1 (not reset to 0) between iterations — breaks the O(n) bound
[ ] suffix automaton: state array not sized to 2*maxLen, running out mid-build
[ ] suffix automaton: clone's `next` map not copied before mutating it
[ ] persistent segtree: update() seeded from the wrong previous root
[ ] persistent segtree: forgot the whole point is NOT calling build() again per version
[ ] offline LCA: query only added to one endpoint's list instead of both
```

---

## Memory hooks

- *Suffix array = sorted list of suffix starting indices; LCP = how much adjacent sorted suffixes share.*
- *Distinct substrings = n(n+1)/2 minus the LCP sum — the LCP sum is exactly the double-counted overlap.*
- *Suffix automaton state = an equivalence class of substrings sharing the same set of ending positions.*
- *Persistent structures share everything unchanged; they only pay for the path that actually changed.*
- *Offline LCA: DSU absorbs a subtree into its parent as soon as that subtree finishes.*