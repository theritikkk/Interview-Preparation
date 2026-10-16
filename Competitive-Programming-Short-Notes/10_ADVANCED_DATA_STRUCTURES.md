# 10 — Advanced Data Structures

**First question: do I actually need this?** Most "range query" problems are solved by a prefix sum or a sorted vector. Reach for a segment tree only after checking the decision table in `06`.

| Updates | Query | Structure | Build | Query |
|---|---|---|---|---|
| none | prefix/range sum | prefix sum array | O(n) | O(1) |
| none | range min/max/gcd | sparse table | O(n log n) | O(1) |
| point add | prefix sum | **Fenwick** | O(n) | O(log n) |
| range add | point read | difference array / Fenwick | O(n) | O(log n) |
| range add | range sum | 2 Fenwicks / lazy segtree | O(n) | O(log n) |
| point set | range min/max/any monoid | segment tree | O(n) | O(log n) |
| range assign/add | range anything | lazy segment tree | O(n) | O(log n) |
| offline, no update | range distinct-count etc. | Mo's algorithm | — | O((n+q)√n) |
| union only | connectivity | DSU | O(n) | O(α(n)) |

**Fenwick vs segment tree:** Fenwick is ~4× shorter to write, ~2–3× faster, and uses `n` ints instead of `4n`. Use it whenever the operation is invertible (sum, XOR). Use a segment tree for min/max/gcd, or when you need lazy propagation or descent.

---

## Fenwick tree (BIT)

```cpp
struct BIT {
    int n; vector<ll> t;
    BIT(int n) : n(n), t(n + 1, 0) {}
    void add(int i, ll v) {            // 1-indexed
        for (; i <= n; i += i & -i) t[i] += v;
    }
    ll sum(int i) {                    // prefix sum [1, i]
        ll s = 0;
        for (; i > 0; i -= i & -i) s += t[i];
        return s;
    }
    ll range(int l, int r) { return sum(r) - sum(l - 1); }
};
```
**1-indexed, always.** `i & -i` with `i = 0` is 0 → infinite loop in `add`. This is the #1 Fenwick bug.

**O(n) build:** copy `a` into `t`, then push each entry up to its parent in one pass — O(n) instead of n `add` calls:

```cpp
vector<ll> t(a.begin(), a.end());         // t[i] = a[i] initially (a must be size n+1)
for (int i = 1; i <= n; i++) {
    int j = i + (i & -i);
    if (j <= n) t[j] += t[i];
}
```
**Trap:** this is not prefix sums — `t[i-1]` plays no role in a Fenwick.

### Range update, point query

```cpp
// add v to [l, r]:
bit.add(l, v); bit.add(r + 1, -v);
// point value at i:
ll val = bit.sum(i);
```
Difference array inside a BIT.

### Range update, range query — two BITs

```cpp
BIT B1(n), B2(n);
void rangeAdd(int l, int r, ll v) {
    B1.add(l, v);      B1.add(r + 1, -v);
    B2.add(l, v*(l-1)); B2.add(r + 1, -v*r);
}
ll prefix(int i) { return B1.sum(i) * i - B2.sum(i); }
ll rangeSum(int l, int r) { return prefix(r) - prefix(l - 1); }
```

### k-th element (BIT descent) — O(log n)

```cpp
int kth(ll k) {                       // BIT holds counts
    int pos = 0;
    for (int pw = 1 << __lg(n); pw; pw >>= 1)
        if (pos + pw <= n && t[pos + pw] < k) { pos += pw; k -= t[pos]; }
    return pos + 1;
}
```
**Use when:** you need order statistics with insertions — an alternative to pb_ds `ordered_set`, and faster.

### 2D BIT

```cpp
void add(int x, int y, ll v) {
    for (int i = x; i <= n; i += i & -i)
        for (int j = y; j <= m; j += j & -j) t[i][j] += v;
}
```
O(log n · log m) per op, O(n·m) memory. Fine up to 1000×1000.

**Classic use of a BIT: counting inversions.** Compress values, sweep left to right, `ans += (number already inserted that are greater than a[i])`.

---

## Segment tree (iterative, non-lazy)

```cpp
struct SegTree {
    int n; vector<ll> t;
    SegTree(int n) : n(n), t(2 * n, 0) {}
    void build(vector<ll>& a) {
        for (int i = 0; i < n; i++) t[n + i] = a[i];
        for (int i = n - 1; i > 0; i--) t[i] = t[2*i] + t[2*i+1];
    }
    void update(int i, ll v) {                 // 0-indexed point assign
        for (t[i += n] = v; i > 1; i >>= 1) t[i>>1] = t[i] + t[i^1];
    }
    ll query(int l, int r) {                   // [l, r)  HALF-OPEN
        ll res = 0;
        for (l += n, r += n; l < r; l >>= 1, r >>= 1) {
            if (l & 1) res += t[l++];
            if (r & 1) res += t[--r];
        }
        return res;
    }
};
```
**2n memory, no recursion, very fast.** Query is `[l, r)` — half-open. Getting this wrong is the usual bug.
**Trap:** for non-commutative merges (like matrix product or string concat) this iterative form needs separate left/right accumulators.

---

## Segment tree with lazy propagation (recursive)

```cpp
struct LazySeg {
    int n; vector<ll> t, lz;
    LazySeg(int n) : n(n), t(4*n, 0), lz(4*n, 0) {}
    void push(int v, int l, int r) {
        if (!lz[v]) return;
        t[v] += lz[v] * (r - l + 1);
        if (l != r) { lz[2*v] += lz[v]; lz[2*v+1] += lz[v]; }
        lz[v] = 0;
    }
    void update(int v, int l, int r, int ql, int qr, ll val) {
        push(v, l, r);
        if (qr < l || r < ql) return;
        if (ql <= l && r <= qr) { lz[v] += val; push(v, l, r); return; }
        int m = (l + r) / 2;
        update(2*v, l, m, ql, qr, val);
        update(2*v+1, m+1, r, ql, qr, val);
        t[v] = t[2*v] + t[2*v+1];
    }
    ll query(int v, int l, int r, int ql, int qr) {
        push(v, l, r);
        if (qr < l || r < ql) return 0;
        if (ql <= l && r <= qr) return t[v];
        int m = (l + r) / 2;
        return query(2*v, l, m, ql, qr) + query(2*v+1, m+1, r, ql, qr);
    }
};
```
**Memory: `4n`.** Not `2n`, not `2^⌈log n⌉`. Allocate `4n` and stop worrying.
**Trap:** `push` must be called at the **start** of both update and query, and after applying a lazy tag to a fully-covered node.
**Trap:** `t[v] = t[2*v] + t[2*v+1]` after the recursive calls — children must be pushed first (they are, inside their own calls).
**Trap:** range **assign** and range **add** need different lazy semantics (assign overwrites the pending add). Don't mix them without thinking.

**Merges that work:** sum, min, max, gcd, "count of maximum", matrix product, any associative operation with an identity.

---

## Sparse table

```cpp
int LOG = __lg(n) + 1;
vector<vector<int>> sp(LOG, vector<int>(n));
sp[0] = a;
for (int k = 1; k < LOG; k++)
    for (int i = 0; i + (1 << k) <= n; i++)
        sp[k][i] = min(sp[k-1][i], sp[k-1][i + (1 << (k-1))]);

int query(int l, int r) {              // [l, r] inclusive
    int k = __lg(r - l + 1);
    return min(sp[k][l], sp[k][r - (1 << k) + 1]);
}
```
**O(1) query, O(n log n) build and memory.** Works only for **idempotent** operations (min, max, gcd, AND, OR) where overlapping the two halves is harmless. **Not for sum.**
**No updates.** If you need updates, segment tree.
**Memory:** n = 2e5, LOG = 18 → 3.6e6 ints = 14 MB. Fine.

---

## DSU (see `17_` for the full template)

```cpp
struct DSU {
    vector<int> p, sz;
    DSU(int n) : p(n + 1), sz(n + 1, 1) { iota(p.begin(), p.end(), 0); }
    int find(int x) { return p[x] == x ? x : p[x] = find(p[x]); }
    bool unite(int a, int b) {
        a = find(a); b = find(b);
        if (a == b) return false;
        if (sz[a] < sz[b]) swap(a, b);
        p[b] = a; sz[a] += sz[b];
        return true;
    }
};
```
O(α(n)) amortized — effectively constant. Details, variants, and applications in `25_DSU_TEMPLATE.md`.

### DSU with rollback

Union by size **without** path compression (so the parent changes are undoable), plus a stack of changes. O(log n) per operation. Needed for offline dynamic connectivity and "DSU on segment tree".

### Small-to-large merging

When merging sets, always merge the smaller into the larger. Total cost O(n log n) because each element moves at most log n times. Used for "count distinct colors in each subtree" and similar.

---

## Sqrt decomposition

```cpp
int B = sqrt(n) + 1;
vector<ll> block(n / B + 1, 0);
// point update: a[i] += v; block[i / B] += v;
// range query [l, r]: full blocks from block[], partial ends element by element
```
O(√n) per query. **Use when:** the operation doesn't decompose nicely for a segment tree, or you need to write something fast under time pressure. Slower than a segment tree but much easier to get right for weird operations.

---

## Mo's algorithm

**Use when:** offline range queries, no updates, and you can move a window endpoint by one in O(1).

```cpp
int B;                                   // block size ~ n / sqrt(q)
sort(queries.begin(), queries.end(), [&](const Q& a, const Q& b) {
    if (a.l / B != b.l / B) return a.l / B < b.l / B;
    return (a.l / B & 1) ? a.r > b.r : a.r < b.r;     // alternating order
});

int curL = 0, curR = -1;
for (auto& q : queries) {
    while (curR < q.r) add(++curR);
    while (curL > q.l) add(--curL);
    while (curR > q.r) remove(curR--);
    while (curL < q.l) remove(curL++);
    ans[q.idx] = current;
}
```
**Complexity:** O((n + q)√n) with O(1) add/remove.
**Trap:** the pointer-move order above is the safe one — expanding before contracting avoids a transiently empty/inverted range.
**Trap:** the alternating `r` direction (the `& 1` line) is worth ~2× — don't skip it.
**Trap:** if `add`/`remove` are O(log n), the total becomes O(n√n log n), usually too slow.

---

## Trie (array-backed)

See `08_STRING_TRICKS.md`. The **binary trie** for XOR queries:

```cpp
const int LOG = 30;
int nxt[MAXN * LOG][2], cnt = 1;
void insert(int x) {
    int v = 0;
    for (int b = LOG - 1; b >= 0; b--) {
        int d = x >> b & 1;
        if (!nxt[v][d]) nxt[v][d] = cnt++;
        v = nxt[v][d];
    }
}
int maxXor(int x) {                     // max x ^ y over inserted y
    int v = 0, res = 0;
    for (int b = LOG - 1; b >= 0; b--) {
        int d = (x >> b & 1) ^ 1;       // prefer the opposite bit
        if (nxt[v][d]) { res |= 1 << b; v = nxt[v][d]; }
        else v = nxt[v][d ^ 1];
    }
    return res;
}
```
**Greedy from the high bit down** — this is the key idea.

---

## pb_ds ordered set

```cpp
#include <ext/pb_ds/assoc_container.hpp>
#include <ext/pb_ds/tree_policy.hpp>
using namespace __gnu_pbds;
template<class T> using oset = tree<T, null_type, less<T>, rb_tree_tag,
                                    tree_order_statistics_node_update>;
oset<int> s;
s.order_of_key(x);        // count of elements < x
*s.find_by_order(k);      // k-th smallest, 0-indexed
```
Convenient but ~2–3× slower than a BIT for the same job. Use it when the values aren't easily compressible or you want the code short.

---

## Implicit treap — array with O(log n) insert / erase / reverse-range

**Recognition:** the problem needs an array-like sequence supporting insert/erase **anywhere** and range operations (like reversing a subrange) that a plain segment tree can't express — segment trees are indexed by a fixed position, and "insert in the middle" or "reverse this range" don't fit that shape cleanly.

**The idea:** a treap (randomized BST) where the "key" is **implicit** — a node's position is just the size of its left subtree, not a stored value — so splitting/merging by *position* instead of by *key* gives array-like insert/erase/slice, all in expected O(log n) thanks to the random priorities keeping the tree balanced.

```cpp
mt19937 treapRng(chrono::steady_clock::now().time_since_epoch().count());

struct Node {
    int prior, size;
    ll val;
    bool rev = false;
    Node *l = nullptr, *r = nullptr;
    Node(ll v) : prior((int)treapRng()), size(1), val(v) {}
};
using pNode = Node*;

int getSize(pNode t) { return t ? t->size : 0; }
void pull(pNode t) { if (t) t->size = getSize(t->l) + getSize(t->r) + 1; }

void push(pNode t) {
    if (t && t->rev) {
        t->rev = false;
        swap(t->l, t->r);
        if (t->l) t->l->rev ^= true;
        if (t->r) t->r->rev ^= true;
    }
}

void split(pNode t, int k, pNode& l, pNode& r) {      // l gets the first k elements (0-indexed count)
    if (!t) { l = r = nullptr; return; }
    push(t);
    if (getSize(t->l) < k) {
        split(t->r, k - getSize(t->l) - 1, t->r, r);
        l = t;
    } else {
        split(t->l, k, l, t->l);
        r = t;
    }
    pull(t);
}
pNode merge(pNode l, pNode r) {
    if (!l || !r) return l ? l : r;
    push(l); push(r);
    if (l->prior > r->prior) { l->r = merge(l->r, r); pull(l); return l; }
    else                     { r->l = merge(l, r->l); pull(r); return r; }
}

void reverseRange(pNode& root, int l, int r) {         // reverse [l, r], 0-indexed inclusive
    pNode a, b, c;
    split(root, l, a, b);
    split(b, r - l + 1, b, c);
    b->rev ^= true;
    root = merge(merge(a, b), c);
}
void insertAt(pNode& root, int pos, ll val) {
    pNode a, b;
    split(root, pos, a, b);
    root = merge(merge(a, new Node(val)), b);
}
void eraseAt(pNode& root, int pos) {
    pNode a, b, c;
    split(root, pos, a, b);
    split(b, 1, b, c);
    root = merge(a, c);                                 // the size-1 middle piece is discarded
}
```
**Complexity:** O(log n) expected per operation — "expected" because it relies on random priorities; an adversary can't defeat it the way a naive unbalanced BST could be.
**Extending it:** add a `sum` field alongside `val`, update it in `pull()`, and you get range-sum queries that stay correct across reverses for free — that combination (range reverse *and* range aggregate) is the main reason to reach for this over a segment tree.
**Trap:** seed the priority generator with `mt19937` + `chrono` (`24_FLOATING_POINT_AND_RANDOM.md`), not `rand()` — a weak or fixed-seed generator undermines the balance guarantee the whole structure depends on.

---

## Chtholly tree (Old Driver Tree / ODT)

**Recognition:** the problem does **range-assign** operations ("set every element in `[l,r]` to `v`") mixed with range queries, **and explicitly states the data is randomly generated** — that phrase is the actual signal; without it, this technique can be adversarially defeated into O(n) per operation.

**The idea:** store the array as a `map<int, ll>` mapping *segment start → value*, so runs of equal consecutive values collapse into a single entry. A range-assign wipes out every boundary strictly inside `[l, r]` and replaces the whole range with one new entry — under random data, assigns tend to merge far more segments than they create, keeping the total segment count low on average.

```cpp
map<int, ll> tree;                    // tree[l] = value of the segment starting at l

auto split(int pos) {                 // ensure a boundary exists exactly at pos; return iterator to it
    auto it = tree.upper_bound(pos);
    --it;
    if (it->first == pos) return it;
    ll val = it->second;
    tree[pos] = val;                  // creates the new boundary; does not change any value
    return tree.find(pos);
}
void assign(int l, int r, ll val) {   // set every element in [l, r] to val
    auto itr = split(r + 1);
    auto itl = split(l);
    tree.erase(itl, itr);             // collapse everything strictly inside [l, r] into nothing
    tree[l] = val;                    // ...and replace it with one segment
}
```
**Complexity:** amortized fast under the random-data guarantee (each `assign` typically removes more segments than any operation ever creates, so the total segment count stays bounded); **genuinely bad worst case** on adversarial input (alternating values that never merge) — this is not a drop-in replacement for a real lazy segment tree when the "random data" guarantee isn't explicitly stated.
**Use when:** exactly that phrase appears in the statement alongside range-assign. Otherwise, use a proper lazy segment tree (`10_ADVANCED_DATA_STRUCTURES.md`, the lazy propagation section above).

---

## Memory budget

| Structure | Memory for n = 2e5 |
|---|---|
| Fenwick (`ll`) | 1.6 MB |
| Segment tree `4n` (`ll`) | 6.4 MB |
| Lazy segtree (`t` + `lz`, `ll`) | 12.8 MB |
| Sparse table (18 × n, `int`) | 14 MB |
| DSU (2 × `int`) | 1.6 MB |
| pb_ds ordered set | ~50 B/node = 10 MB |
| Trie 26-ary, 2e5 total chars | 2e5 × 26 × 4 = 21 MB |
| Binary trie, 30 levels, 2e5 values | 6e6 × 2 × 4 = 48 MB |

**Tries are memory hogs.** Always size the node array to the *actual* total, not a safe-looking constant.