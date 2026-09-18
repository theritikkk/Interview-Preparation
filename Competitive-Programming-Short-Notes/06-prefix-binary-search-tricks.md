# 06 — Prefix, Two Pointers, Binary Search, Sweep, Intervals

The densest file here. Everything that turns a range query or a search into O(1) or O(log n).

---

# Prefix sums

### 1D prefix sum

**Remember:** `pre[i]` = sum of the **first i** elements. Then every query is `pre[hi+1] - pre[lo]`.

```cpp
vector<ll> pre(n + 1, 0);
for (int i = 0; i < n; i++) pre[i+1] = pre[i] + a[i];
ll sum_lr = pre[r+1] - pre[l];       // inclusive [l, r], 0-indexed
```

**Why size n+1:** so `l = 0` needs `pre[0] = 0` without a special case. Always allocate `n+1`.
**Trap:** `pre` must be `ll`. 1e5 elements × 1e9 = 1e14.

### Prefix XOR

```cpp
pre[i+1] = pre[i] ^ a[i];
ll xor_lr = pre[r+1] ^ pre[l];
```

### Prefix min/max and suffix min/max

```cpp
vector<int> pmax(n), smax(n);
pmax[0] = a[0]; for (int i = 1; i < n; i++) pmax[i] = max(pmax[i-1], a[i]);
smax[n-1] = a[n-1]; for (int i = n-2; i >= 0; i--) smax[i] = max(smax[i+1], a[i]);
```
**Use when:** "best split point" problems — for each i, combine the best of the prefix with the best of the suffix. Turns O(n²) into O(n).

**Note:** prefix min/max is **not** invertible — you cannot get `min(l..r)` by combining. For that you need a sparse table or segment tree.

### 2D prefix sum

```cpp
// pre is (n+1) x (m+1), zero-filled
for (int i = 1; i <= n; i++)
  for (int j = 1; j <= m; j++)
    pre[i][j] = a[i-1][j-1] + pre[i-1][j] + pre[i][j-1] - pre[i-1][j-1];

// sum of rectangle rows [r1..r2], cols [c1..c2], 0-indexed inclusive
ll s = pre[r2+1][c2+1] - pre[r1][c2+1] - pre[r2+1][c1] + pre[r1][c1];
```
**Memory hook:** *build: add two, subtract the overlap. query: big − left − top + corner.*

### Difference array (range update, point query)

```cpp
vector<ll> d(n + 1, 0);
// add v to [l, r] inclusive:
d[l] += v; d[r+1] -= v;
// recover:
for (int i = 1; i < n; i++) d[i] += d[i-1];
```
**Remember:** *difference array is the inverse of prefix sum.* Range update O(1), then one O(n) pass to materialize.
**Trap:** size `n+1` so `d[r+1]` at `r = n-1` is valid.

### 2D difference array

```cpp
// add v to rectangle [r1..r2] x [c1..c2]
d[r1][c1] += v; d[r1][c2+1] -= v; d[r2+1][c1] -= v; d[r2+1][c2+1] += v;
// then prefix-sum twice (rows, then columns)
```

**Decision table:**

| Updates | Queries | Structure |
|---|---|---|
| none | range sum | prefix sum |
| range add | point read | difference array |
| point add | range sum | Fenwick tree |
| range add | range sum | Fenwick ×2 / lazy segtree |
| none | range min/max/gcd | sparse table |
| point update | range min/max | segment tree |

---

# Two pointers / sliding window

### Recognition rules

- *contiguous subarray* + *condition is monotonic in window length* → sliding window
- two sorted arrays being merged/matched → two pointers
- "longest/shortest subarray such that P" where extending the window can only make P harder/easier → sliding window
- "count pairs (i, j) with i < j and f(i,j) ≤ k" on sorted data → two pointers

### When sliding window does NOT work

The window technique needs: *if a window is valid, every sub-window is valid* (or the mirrored version).

- **Array has negative numbers and the condition is `sum ≤ k`.** Shrinking from the left can *increase* the sum. Use prefix sums + a map/set, or a monotonic deque.
- **Condition is non-monotonic** (e.g. "sum is exactly k" with mixed signs) → prefix sum + hash map.
- **You need min/max in the window** → the window itself is fine, but you need a monotonic deque to maintain the extremum.

**Rule of thumb:** *all values positive → sliding window is probably valid. Mixed signs → probably not.*

### Variable window template

```cpp
int l = 0; ll cur = 0; int best = 0;
for (int r = 0; r < n; r++) {
    cur += a[r];                       // extend
    while (cur > K) cur -= a[l++];     // shrink while invalid
    best = max(best, r - l + 1);
}
```

### Fixed window template

```cpp
ll cur = 0;
for (int i = 0; i < n; i++) {
    cur += a[i];
    if (i >= k) cur -= a[i - k];
    if (i >= k - 1) best = max(best, cur);
}
```

### Counting windows

"Number of subarrays with at most K distinct" — for each `r`, the count of valid `l` is `r - l + 1`.
"Exactly K" = `atMost(K) - atMost(K-1)`. Memorize this decomposition; it appears constantly.

### Monotonic stack — next greater / smaller

```cpp
// nge[i] = index of next element strictly greater than a[i], or n
vector<int> nge(n, n);
stack<int> st;
for (int i = 0; i < n; i++) {
    while (!st.empty() && a[st.top()] < a[i]) { nge[st.top()] = i; st.pop(); }
    st.push(i);
}
```

**The contribution pattern** (very high value): for each element, find the range where it is the minimum, using previous-smaller and next-smaller. Then "sum over all subarrays of their minimum" becomes a single O(n) pass:
```cpp
ans += a[i] * (i - prevSmaller[i]) * (nextSmaller[i] - i);
```
**Tie-breaking:** use strict on one side and non-strict on the other, or you double-count equal elements.

Also solves: largest rectangle in histogram, maximal rectangle, stock span.

### Monotonic deque — sliding window min/max in O(n)

```cpp
deque<int> dq;                    // stores indices, values decreasing → window MAX
for (int i = 0; i < n; i++) {
    while (!dq.empty() && a[dq.back()] <= a[i]) dq.pop_back();
    dq.push_back(i);
    if (dq.front() <= i - k) dq.pop_front();
    if (i >= k - 1) out[i-k+1] = a[dq.front()];
}
```
**Memory hook:** *the deque holds the candidates that could still become the answer — anything dominated is popped.*

---

# Binary search

### Recognition rule

> If feasibility is **monotonic** — `FFFFFTTTTT` or `TTTTTFFFFF` — binary search the answer.

Ask: **"Can I answer YES/NO for a fixed candidate answer X, and is the answer for X+1 at least as easy?"** If yes, binary search.

Classic shapes: "minimize the maximum", "maximize the minimum", "smallest capacity such that…", "least time such that…".

### First true (FFFTTT) — the template to memorize

```cpp
int lo = 0, hi = n;            // hi = n means "no true exists"
while (lo < hi) {
    int mid = lo + (hi - lo) / 2;
    if (ok(mid)) hi = mid;
    else lo = mid + 1;
}
// lo == hi == first index where ok is true, or n
```

### Last true (TTTFFF)

```cpp
int lo = -1, hi = n - 1;       // lo = -1 means "no true exists"
while (lo < hi) {
    int mid = lo + (hi - lo + 1) / 2;    // NOTE the +1 — round UP
    if (ok(mid)) lo = mid;
    else hi = mid - 1;
}
// lo == last index where ok is true, or -1
```

**Trap — the infinite loop.** With `lo = mid` you *must* round the midpoint up. Otherwise `lo = 0, hi = 1` gives `mid = 0`, `lo = 0` forever.

**Memory hook:** *if an assignment can leave the pointer where it was (`lo = mid`), bias the midpoint away from it (`+1`).*

### Overflow in the midpoint

```cpp
int mid = (lo + hi) / 2;        // ✘ overflows when lo + hi > INT_MAX
int mid = lo + (hi - lo) / 2;   // ✔
```
Only bites when bounds approach 2^31 (binary searching on answers up to 1e9 with both ends large). Free habit.

**Also:** `lo + (hi - lo) / 2` is wrong for **negative** ranges in the "last true" variant — `(hi - lo + 1) / 2` handles it correctly because the offset is non-negative.

### `while (lo <= hi)` style

```cpp
int lo = 0, hi = n - 1, ans = -1;
while (lo <= hi) {
    int mid = lo + (hi - lo) / 2;
    if (ok(mid)) { ans = mid; lo = mid + 1; }   // last true
    else hi = mid - 1;
}
```
Slower to write but harder to get wrong — it never leaves the pointers in an ambiguous state. **Pick one style and always use it.** Mixing styles is where the bugs come from.

### Floating-point binary search

```cpp
double lo = 0, hi = 1e9;
for (int iter = 0; iter < 100; iter++) {      // fixed count, not while(hi-lo>eps)
    double mid = (lo + hi) / 2;
    if (ok(mid)) hi = mid; else lo = mid;
}
double ans = lo;
```
**Why fixed iterations:** `while (hi - lo > 1e-9)` can loop forever when the values are large (the gap can't shrink below the float resolution). 100 iterations halves the range 100 times — more than enough for any range, and it's ~100 operations.

**Rule:** 100 iterations for doubles, 200 for long double if you're paranoid. Cost is negligible.

### Ternary search (unimodal functions)

```cpp
double lo = L, hi = R;
for (int i = 0; i < 200; i++) {
    double m1 = lo + (hi - lo) / 3, m2 = hi - (hi - lo) / 3;
    if (f(m1) < f(m2)) lo = m1; else hi = m2;   // finds MAXIMUM
}
```
Integer version:
```cpp
while (hi - lo > 2) {
    int m1 = lo + (hi - lo) / 3, m2 = hi - (hi - lo) / 3;
    if (f(m1) < f(m2)) lo = m1 + 1; else hi = m2 - 1;
}
// then brute-force lo..hi
```
**Trap:** the integer version has off-by-one landmines. Safest: shrink to a range of ≤ 3 and check all of them explicitly.
**Requires strict unimodality** — flat regions break it.

### Binary search on a predicate you must think about

The common failure is not the loop, it's `ok()`. Write `ok()` as its own function, and sanity check:
- `ok(lo_bound)` should be the "obvious" value
- `ok(hi_bound)` should be the other one
- If both ends give the same answer, your search range is wrong

---

# Coordinate compression

**Use when:** values are up to 1e9 but only their **relative order** matters, and n is small (1e5). Turns "array indexed by value" into "array indexed by rank".

```cpp
vector<int> vals = a;                       // copy
sort(vals.begin(), vals.end());
vals.erase(unique(vals.begin(), vals.end()), vals.end());

auto rank_of = [&](int x) {
    return int(lower_bound(vals.begin(), vals.end(), x) - vals.begin());
};
for (int i = 0; i < n; i++) b[i] = rank_of(a[i]);   // b[i] in [0, m)
// vals[r] recovers the original value from rank r
```

**Four steps:** *copy, sort, unique, lower_bound.*

**Trap:** if you also need the values **between** the ones present (for interval problems), compress `l`, `r`, **and** `r+1`.
**Trap:** keep `vals` around — you almost always need to map back.

---

# Sweep line / events

**Remember:** convert intervals into events, sort, sweep with a running counter.

```cpp
vector<pii> ev;
for (auto [l, r] : segs) {
    ev.push_back({l, +1});
    ev.push_back({r + 1, -1});     // half-open: interval ends BEFORE r+1
}
sort(ev.begin(), ev.end());
int cur = 0, best = 0;
for (auto [x, delta] : ev) { cur += delta; best = max(best, cur); }
```

**Tie-ordering is the whole game:**
- Intervals `[1,3]` and `[3,5]` — do they overlap? If yes (closed intervals, touching counts), process **starts before ends** at the same coordinate.
- If touching does *not* count, process **ends before starts**.
- With `pii{x, delta}` and `delta ∈ {+1, -1}`, default sorting puts `-1` before `+1` at equal `x` → **ends first** → touching does not count.
- The `r + 1` trick above sidesteps this entirely for integer coordinates. **Prefer it.**

**Connection to difference arrays:** a sweep is just a difference array when coordinates are small enough to index directly. If coordinates are up to 1e9, sweep; if up to 1e6, difference array is simpler and faster.

**Other sweeps:** rectangle union area (sweep x, segment tree over y), closest pair of points, counting inversions, "for each point, how many rectangles contain it".

---

# Intervals

### Overlap test

```cpp
// closed [l1,r1] and [l2,r2] overlap iff:
bool overlap = max(l1, l2) <= min(r1, r2);

// intersection (if it exists):
int L = max(l1, l2), R = min(r1, r2);      // valid iff L <= R
int len = max(0, R - L + 1);               // closed, integer
```

**For half-open `[l, r)`:** overlap iff `max(l1,l2) < min(r1,r2)`, length `max(0, R - L)`.

**Memory hook:** *max of the lefts vs min of the rights.*

### Merging intervals

```cpp
sort(iv.begin(), iv.end());          // by left endpoint
vector<pii> merged;
for (auto [l, r] : iv) {
    if (!merged.empty() && l <= merged.back().second)     // touching counts
        merged.back().second = max(merged.back().second, r);
    else
        merged.push_back({l, r});
}
```
**Trap:** `max(merged.back().second, r)` — a later interval can be entirely contained. Writing `= r` is a classic bug.
**Trap:** `l <= back.second` merges touching intervals `[1,3],[3,5]`. Use `l < back.second` if touching shouldn't merge, `l <= back.second + 1` if adjacent integers should merge.

### Choosing maximum non-overlapping intervals

Sort by **right endpoint**, greedily take the first that starts after the last taken end. This is the classic exchange-argument greedy — sorting by left endpoint or by length is **wrong**.

### Conventions

Pick **half-open `[l, r)`** when you can. It makes length `r - l`, concatenation trivial, and eliminates most `+1`/`-1` bugs. Switch to closed only when the problem forces it.

---

## Memory hooks

- *`pre[i]` = first i elements → query is `pre[hi+1] - pre[lo]`*
- *2D query: big − left − top + corner*
- *difference array is the inverse of a prefix sum*
- *positive values → sliding window works; mixed signs → it doesn't*
- *exactly K = atMost(K) − atMost(K−1)*
- *`lo = mid` needs `mid` rounded up*
- *binary search on answer: "can I ask YES/NO about a candidate?"*
- *doubles: 100 iterations, never `while (hi-lo > eps)`*
- *compress = copy, sort, unique, lower_bound*
- *overlap: max of lefts ≤ min of rights*