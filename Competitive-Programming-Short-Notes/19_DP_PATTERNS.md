# 19 — DP Patterns & State Design

> Not a DP course. This is *state design*, *memory sizing*, and the shapes you should recognize instantly.

---

## The ritual, every time

```
1. What is the state?          (the minimum information to decide the future)
2. What is the transition?     (how does state i depend on smaller states?)
3. What is the base case?
4. What is the answer?          (a single cell, or a max/sum over cells?)
5. How many states × cost per state?   → time
6. states × sizeof            → memory (do this BEFORE coding)
7. What order do I iterate?    (so dependencies are already computed)
```

**If step 6 fails, you have the wrong state.** Don't start coding and hope.

---

## Finding the state

The state must capture **everything about the past that affects the future**, and nothing else.

Questions that shrink a state:
- Is this parameter determined by the others? (`k = i - j` → drop it)
- Do I need the exact value, or just a bounded/capped version? ("sum so far" capped at S)
- Do I need the exact set, or just its size? (n! → n)
- Do I need the whole prefix, or just the last element? (usually the last element)
- Can I store a *difference* instead of two absolute values?
- Is the parameter's range actually bounded by the problem's values, not by n?

**Signal you have the wrong state:** the transition needs information the state doesn't carry, and you're tempted to add "just one more dimension" that makes memory explode.

---

## Iteration order

The rule: **when you compute `dp[x]`, everything it depends on must already be final.**

- Depends on smaller `i` → loop `i` ascending.
- Depends on larger `i` → loop descending.
- Depends on shorter intervals → loop by **length**, not by endpoints.
- Depends on submasks → loop masks ascending (a submask is numerically smaller).
- Unclear → use memoized recursion, which gets the order right automatically.

**Memoized recursion vs iterative:**

| | Recursion + memo | Iterative |
|---|---|---|
| Order | automatic | you must get it right |
| Speed | ~2–3× slower (call overhead) | faster |
| Memory | + stack frames | tighter |
| Rolling arrays | hard | easy |
| Debugging | easier | harder |

**Rule:** prototype with recursion when the order is confusing; rewrite iteratively if you need the constant factor or a rolling array.

---

## Knapsack shapes

### 0/1 knapsack (each item once)

```cpp
vector<int> dp(W + 1, 0);
for (int i = 0; i < n; i++)
    for (int w = W; w >= wt[i]; w--)            // BACKWARD
        dp[w] = max(dp[w], dp[w - wt[i]] + val[i]);
```

### Unbounded knapsack (unlimited copies)

```cpp
for (int i = 0; i < n; i++)
    for (int w = wt[i]; w <= W; w++)            // FORWARD
        dp[w] = max(dp[w], dp[w - wt[i]] + val[i]);
```

**The entire difference is the loop direction.** Backward = `dp[w - wt[i]]` is still from the previous item (used at most once). Forward = it's already updated for this item (reusable).

**Memory hook:** *backward = one copy; forward = infinite copies.*

### Bounded knapsack (at most cᵢ copies)

Binary splitting: split `cᵢ` into powers of two (1, 2, 4, …, remainder) and treat each as a separate 0/1 item. Turns O(n·W·c) into O(n·W·log c).

### Subset sum (feasibility only)

```cpp
bitset<100001> dp; dp[0] = 1;
for (int x : a) dp |= dp << x;         // O(n·S/64)
```
The `bitset` version is ~64× faster than the boolean DP. Use it whenever you only need "is this sum reachable".

### Counting (instead of optimizing)

Replace `max` with `+` and take everything mod MOD. Same loop structure.

---

## Classic 1D shapes

### Longest Increasing Subsequence — O(n log n)

```cpp
vector<int> tails;
for (int x : a) {
    auto it = lower_bound(tails.begin(), tails.end(), x);   // strictly increasing
    if (it == tails.end()) tails.push_back(x);
    else *it = x;
}
int lis = tails.size();
```
**`lower_bound` → strictly increasing. `upper_bound` → non-decreasing.** This one line is the whole difference, and it's a frequent WA.
**Trap:** `tails` is **not** the LIS itself, only its length is correct. Reconstructing the actual subsequence needs parent pointers.

### Kadane (max subarray sum)

```cpp
ll best = a[0], cur = a[0];
for (int i = 1; i < n; i++) {
    cur = max((ll)a[i], cur + a[i]);
    best = max(best, cur);
}
```
**Trap:** initialize with `a[0]`, not 0 — otherwise an all-negative array returns 0.

### House robber / no two adjacent

```cpp
ll take = 0, skip = 0;
for (int x : a) {
    ll ntake = skip + x;
    ll nskip = max(skip, take);
    take = ntake; skip = nskip;
}
ans = max(take, skip);
```
Two rolling variables, O(1) memory. This shape (take/skip) covers a huge number of Div2 B/C problems.

### Coin change (min coins)

```cpp
vector<int> dp(S + 1, INF); dp[0] = 0;
for (int s = 1; s <= S; s++)
    for (int c : coins) if (c <= s && dp[s-c] != INF)
        dp[s] = min(dp[s], dp[s-c] + 1);
```

---

## 2D shapes

### Grid paths

```cpp
dp[0][0] = 1;
for (int i = 0; i < R; i++)
  for (int j = 0; j < C; j++) {
      if (i) dp[i][j] += dp[i-1][j];
      if (j) dp[i][j] += dp[i][j-1];
  }
```
Rolling to 1D: `dp[j] += dp[j-1]` with a single row. Memory O(C).

### Edit distance / LCS

```cpp
for (int i = 1; i <= n; i++)
  for (int j = 1; j <= m; j++)
    if (a[i-1] == b[j-1]) dp[i][j] = dp[i-1][j-1] + 1;
    else dp[i][j] = max(dp[i-1][j], dp[i][j-1]);
```
O(n·m). At n = m = 5000 that's 2.5e7 states × 4 B = 100 MB → **use a rolling array** (2 rows, 40 KB).

### Interval DP

```cpp
for (int len = 2; len <= n; len++)
  for (int l = 0; l + len - 1 < n; l++) {
      int r = l + len - 1;
      for (int k = l; k < r; k++)
          dp[l][r] = min(dp[l][r], dp[l][k] + dp[k+1][r] + cost(l, r));
  }
```
**Loop by length first.** O(n³) → n ≤ 500.
Shapes: matrix chain, burst balloons, optimal BST, "merge adjacent piles", palindrome partitioning.

---

## Bitmask DP

**Trigger: `n ≤ 20`.**

```cpp
vector<int> dp(1 << n, INF);
dp[0] = 0;
for (int mask = 0; mask < (1 << n); mask++) {
    int i = __builtin_popcount(mask);        // number already placed
    for (int j = 0; j < n; j++)
        if (!(mask >> j & 1))
            dp[mask | (1 << j)] = min(dp[mask | (1 << j)], dp[mask] + cost[i][j]);
}
ans = dp[(1 << n) - 1];
```
**Trick:** `popcount(mask)` often encodes "how many steps have happened", saving a whole dimension. Use it whenever the step index equals the number of set bits.

**TSP:** `dp[mask][last]` — O(2ⁿ·n²), n ≤ 20 gives 4e8… too slow. n ≤ 18 gives 8.5e7, fine.
**Memory:** `dp[1<<20][20]` of int = 84 MB. Check before coding.

**Subset-partition DP:** `dp[mask] = min over submasks s of (dp[mask^s] + cost(s))` — O(3ⁿ). n ≤ 16.

---

## Digit DP

**Trigger:** "count numbers in [L, R] with property P", R up to 1e18.

```cpp
string s;                       // the upper bound as a string
ll memo[20][2][SOMETHING];
ll go(int pos, bool tight, int state) {
    if (pos == (int)s.size()) return isGood(state);
    if (!tight && memo[pos][0][state] != -1) return memo[pos][0][state];
    int lim = tight ? s[pos] - '0' : 9;
    ll res = 0;
    for (int d = 0; d <= lim; d++)
        res += go(pos + 1, tight && (d == lim), next(state, d));
    if (!tight) memo[pos][0][state] = res;
    return res;
}
// answer for [L, R] = f(R) - f(L-1)
```
**Standard state:** `(position, tight, leadingZero, problem-specific)`.
**Trap:** memoize only when `!tight` — tight states are visited once each and caching them across different bounds is wrong.
**Trap:** `f(R) - f(L-1)`, and handle `L = 0` carefully.
**Trap:** leading zeros matter for digit-sum / distinct-digit problems.

---

## Tree DP

```cpp
void dfs(int u, int p) {
    dp[u][0] = 0; dp[u][1] = 1;            // e.g. independent set
    for (int v : adj[u]) if (v != p) {
        dfs(v, u);
        dp[u][0] += max(dp[v][0], dp[v][1]);
        dp[u][1] += dp[v][0];
    }
}
```
**Shape:** compute children first, combine at the parent. The state is usually `(node, small extra info)`.

**Tree knapsack** (`dp[u][k]` = best using k nodes in u's subtree) merged child by child is **O(n²)**, not O(n·k²) — a classic and non-obvious result. Bound the inner loops by subtree sizes:
```cpp
for (int v : children) {
    for (int i = min(sz[u], K); i >= 0; i--)
      for (int j = min(sz[v], K - i); j >= 0; j--)
          tmp[i+j] = max(tmp[i+j], dp[u][i] + dp[v][j]);
    sz[u] += sz[v];
}
```

**Rerooting:** compute `down[u]` (answer within the subtree) with one DFS, then `up[u]` (answer from the rest of the tree) with a second DFS from the root. Gives the answer for **every** node as a root in O(n) instead of O(n²). See `20_TREES.md`.

---

## DP on DAG / memoized graph

Any DP is shortest-path-on-a-DAG in disguise. If your transitions form a DAG, topological order **is** the iteration order. If they have cycles, DP doesn't apply directly — you need Dijkstra/Bellman-Ford or a fixed-point iteration.

---

## Optimizations (know they exist; reach for them rarely)

| Technique | Turns | Into | Trigger |
|---|---|---|---|
| Rolling array | O(n·m) memory | O(m) | dp[i] depends only on dp[i−1] |
| Prefix sums over dp | O(n²) | O(n) | transition is a contiguous range sum |
| Monotonic deque | O(n·k) | O(n) | transition is a sliding-window min/max |
| Convex hull trick | O(n²) | O(n log n) | transition is `min(a[j]·x + b[j])` — code in `27_ADVANCED_STRINGS_AND_STRUCTURES.md` |
| Divide & conquer opt | O(n²k) | O(nk log n) | opt[i] is monotonic |
| Knuth opt | O(n³) | O(n²) | quadrangle inequality |
| SOS DP | O(3ⁿ) | O(2ⁿ·n) | sum over all submasks |
| Bitset | O(n·S) | O(n·S/64) | boolean DP |

The top three cover 95% of contest needs. The bottom four are Div1 territory — know the names so you can recognize the shape, keep templates.

**Prefix-sum-over-dp, the most useful one:**
```cpp
// dp[i] = sum of dp[j] for j in [i-k, i-1]
pre[i] = pre[i-1] + dp[i];
dp[i] = pre[i-1] - pre[max(0, i-k-1)];
```

---

## Memory sizing table

| Table | Memory (int) | Verdict |
|---|---|---|
| `dp[1e6]` | 4 MB | fine |
| `dp[1e5][20]` | 8 MB | fine |
| `dp[5000][5000]` | 100 MB | risky → roll it |
| `dp[10000][10000]` | 400 MB | **impossible** |
| `dp[1<<20]` | 4 MB | fine |
| `dp[1<<20][20]` | 84 MB | risky |
| `dp[1<<18][18]` | 19 MB | fine |
| `dp[500][500][500]` | 500 MB | impossible → rethink state |

**With `long long`, double every number above.** This is the single strongest argument against `#define int long long` in DP-heavy problems.

---

## DP debugging checklist

```
[ ] base case initialized (and with the right value — 0, INF, or -INF?)
[ ] INF used for min-DP; adding to INF overflows → guard or use 0x3f3f3f3f
[ ] iteration order matches the dependency direction
[ ] knapsack loop direction (backward = 0/1, forward = unbounded)
[ ] dp array reset between test cases
[ ] answer read from the right cell (dp[n] vs dp[n-1], or max over the last row)
[ ] 0-indexed vs 1-indexed inside the transition
[ ] memoization sentinel (-1) can't collide with a real answer of -1
[ ] LIS: lower_bound (strict) vs upper_bound (non-strict)
[ ] counting DP: took mod everywhere, including in the accumulation
[ ] state genuinely captures everything (check by hand on n = 3)
```