# 11 — Complexity & Memory Estimation

> **Look at the constraints BEFORE you design the algorithm.**
> The constraints are the problem setter telling you the intended complexity.

---

## Operations per second

Codeforces machines do roughly **1e8 to 1e9 simple operations per second** within a typical 1–2 s limit.

These are heuristics, not guarantees. The constant factor moves the line by 10× in either direction:

| Makes it slower | Makes it faster |
|---|---|
| `map` / `set` lookups | array indexing |
| modulo and division | addition, multiplication, bit ops |
| cache misses (random access into 1e7 array) | sequential access |
| recursion, virtual calls | tight loops |
| `long long` when `int` would do | `bitset` bulk ops |
| unpredictable branches | branch-free code |
| allocations in a loop | preallocated buffers |

**Practical calibration:**
- 1e8 simple `int` additions over a contiguous array: ~0.1 s
- 1e8 modulo operations: ~0.5–1 s
- 1e7 `map<int,int>` insertions: ~2–4 s (usually TLE)
- 1e6 `sort` of ints: ~0.1 s
- 2e5 `sort` with a lambda comparator on pairs: negligible

---

## N → complexity table

| n | Acceptable complexity | Typical technique |
|---|---|---|
| ≤ 10 | O(n!) ≈ 3.6e6 | brute-force permutations |
| ≤ 12 | O(n! ) ≈ 4.8e8 | borderline permutations |
| ≤ 20 | O(2ⁿ · n) ≈ 2e7 | bitmask DP, subset enumeration |
| ≤ 24 | O(2ⁿ) ≈ 1.7e7 | subsets |
| ≤ 40 | O(2^(n/2)) ≈ 1e6 | **meet in the middle** |
| ≤ 100 | O(n⁴) = 1e8 | 4 nested loops, Floyd on n=500 |
| ≤ 500 | O(n³) = 1.25e8 | Floyd–Warshall, matrix ops, interval DP |
| ≤ 5,000 | O(n²) = 2.5e7 | 2D DP, LCS, all-pairs on a small set |
| ≤ 1e5 | O(n log² n) ≈ 3e7 | segment tree with inner binary search, CDQ |
| ≤ 2e5 | O(n log n) ≈ 3.5e6 | sort, segment tree, Dijkstra, binary search on answer |
| ≤ 1e6 | O(n log n) ≈ 2e7 | sort still fine; be careful with constants |
| ≤ 1e7 | O(n) | single pass, sieve, counting |
| ≤ 1e9 | O(√n) or O(log n) | math, digit DP, binary search |
| ≤ 1e18 | O(log n) | binary exponentiation, matrix power, digit DP |

**The reverse lookup is the skill.** You see `n ≤ 40` → *meet in the middle*. You see `n ≤ 20` → *bitmask*. You see `n ≤ 500` → *O(n³) is allowed, so an "expensive" DP is fine*. You see `n ≤ 1e5` and `q ≤ 1e5` → *O((n+q) log n)*.

---

## THE SUM-OF-N TRAP

```
t (1 ≤ t ≤ 1e4) test cases
n (1 ≤ n ≤ 2e5)
It is guaranteed that the sum of n over all test cases does not exceed 2·10^5.
```

**That last line changes everything.** Total work is bounded by the *sum*, so O(n log n) per test is fine. Without that line, `t · n_max` = 2e9 and you need O(1) or O(log n) per test.

**Always read it.** Also look for "sum of n·m", "sum of q", "sum of the lengths of all strings".

**Corollary:** if the sum is bounded, you may **not** allocate `O(n_max)` per test case. `vector<int> cnt(200001)` inside a loop of 1e4 tests is 2e9 writes. Allocate once globally, or allocate `O(n)` for *this* test.

---

## Memory math

### Per-element sizes

| Type | Bytes |
|---|---|
| `bool`, `char` | 1 |
| `short` | 2 |
| `int`, `float` | 4 |
| `long long`, `double`, pointer | 8 |
| `long double` | 16 (10 used) on 64-bit GCC |
| `__int128` | 16 |
| `pair<int,int>` | 8 |
| `pair<int,ll>` | 16 (padding!) |

### Quick reference

```
1e6 int        =  4 MB
1e6 long long  =  8 MB
1e7 int        = 40 MB
1e6 pair<int,int> = 8 MB
bitset<1e6>    = 125 KB
bool a[1e7]    = 10 MB
vector<bool>(1e7) = 1.25 MB
```

**Typical Codeforces limit: 256 MB.** Stay under ~200 MB to be safe.

### Container overhead

| | Overhead |
|---|---|
| `vector<T>` header | 24 B (3 pointers) + capacity slack up to 2× |
| `vector<vector<int>>` | 24 B **per row** + each row's data |
| `set<int>` / `map<int,int>` node | ~40 B struct + ~16 B allocator header ≈ **48–56 B** |
| `unordered_map<int,int>` | ~32–40 B/node + bucket array (8 B × buckets) |
| `string` | 32 B header, ≤15 chars inline (SSO) |

**The killer:** `map<int,int>` with 1e6 entries ≈ **50 MB**. The same data in `int[1e6]` is 4 MB. Twelve times the memory and a hundred times slower. This is why array-indexed counting beats hash maps whenever keys are bounded.

**`vector<vector<int>>` for an n×m grid:** `n` separate heap allocations, each with a 24 B header and its own cache line. For a fixed-size grid, prefer a flat `vector<int> g(n*m)` with `g[i*m+j]` — one allocation, contiguous, ~2× faster to traverse.

### Adjacency list memory

For `n` nodes and `m` edges, undirected:
```
vector<vector<int>> adj(n);
≈ 24n (headers) + 8m (2m ints, plus capacity slack)
```
n = 2e5, m = 2e5 → 4.8 MB + 1.6 MB ≈ 7 MB. Fine.
With `vector<vector<pair<int,int>>>` for weights: 8 bytes per entry → 2× that.

---

## DP table sizing — the 60-second ritual

**Multiply the dimensions. Multiply by sizeof. Compare to the limit. Do this before writing a line of code.**

```
dp[10000][10000] of int = 1e8 × 4 B = 400 MB      → IMPOSSIBLE
dp[5000][5000]   of int = 2.5e7 × 4 = 100 MB      → risky but possible
dp[2000][2000]   of int = 4e6 × 4  = 16 MB        → fine
dp[1e5][20]      of ll  = 2e6 × 8  = 16 MB        → fine
dp[1e6][2]       of int = 2e6 × 4  = 8 MB         → fine
```

**And check the time too.** `dp[5000][5000]` is 2.5e7 states — fine if each is O(1), dead if each is O(n).

### Escape hatches when the table doesn't fit

1. **Rolling array.** If `dp[i][*]` depends only on `dp[i-1][*]`:
   ```cpp
   vector<int> prev(m), cur(m);
   // ... compute cur from prev ...
   swap(prev, cur);
   ```
   Or `dp[2][m]` with `dp[i & 1]`. Drops memory from O(n·m) to O(m).

2. **Iterate backwards in place** (0/1 knapsack):
   ```cpp
   for (int i = 0; i < n; i++)
     for (int w = W; w >= wt[i]; w--)          // BACKWARD = each item used once
       dp[w] = max(dp[w], dp[w - wt[i]] + val[i]);
   ```
   Forward iteration gives unbounded knapsack. **This direction is the entire difference between the two problems.**

3. **Shrink the type.** `int` instead of `ll`; `short` or `char` if values are tiny; `bitset` for boolean DP.

4. **Reconsider the state.** If you need the path, store parent pointers as `short`/`char`, or recompute by divide-and-conquer.

---

## Hidden costs

```
[ ] allocating a vector inside a loop → n allocations
[ ] vector<vector<int>> rebuilt per test case
[ ] recursion: each frame ~50-100 B; depth 1e6 ≈ 100 MB of stack
[ ] string concatenation s = s + c → O(n^2) time AND churn
[ ] passing containers by value
[ ] map<int, vector<int>> — node + vector header per key
[ ] std::function<...> has indirect-call overhead vs a lambda passed as a template arg
```

**Stack on Codeforces:** the stack limit equals the memory limit (256 MB), so deep recursion usually survives. But a DFS of depth 1e6 with big local arrays in the frame will not. If you're worried, convert to an iterative DFS.

---

## Estimating before implementing — the checklist

```
1. What is n? What is the sum of n?
2. What complexity does that allow?  (use the table)
3. What is my candidate algorithm's complexity?
4. Does the DP table fit in memory?  (dims × sizeof)
5. Is my constant factor bad?  (map? modulo? cache misses?)
6. Is there a per-test allocation that ignores the sum bound?
```

If step 3 doesn't match step 2, **stop and think again** — don't start coding hoping it passes. A wrong complexity almost never squeaks through.

---

## Complexity of things you use constantly

| Operation | Complexity |
|---|---|
| `sort` | O(n log n), ~1e6 in 0.1 s |
| `stable_sort` | O(n log² n), allocates |
| `nth_element` | O(n) average |
| `set`/`map` insert/find/erase | O(log n) |
| `unordered_map` avg / worst | O(1) / O(n) |
| `priority_queue` push/pop | O(log n) |
| build heap from vector | O(n) |
| `vector::push_back` | amortized O(1) |
| `vector::insert` at middle | O(n) |
| `lower_bound` on vector | O(log n) |
| `lower_bound` on set (member) | O(log n) |
| `std::lower_bound` on set | **O(n)** |
| `bitset` op on N bits | O(N/64) |
| Fenwick update/query | O(log n) |
| Segment tree query/update | O(log n) |
| Segment tree build | O(n) |
| Sparse table build / query | O(n log n) / O(1) |
| DSU (path compression + union by size) | O(α(n)) ≈ O(1) |
| BFS / DFS | O(n + m) |
| Dijkstra (binary heap) | O((n + m) log n) |
| Floyd–Warshall | O(n³) |
| Bellman–Ford | O(n·m) |
| Kruskal | O(m log m) |
| Sieve to N | O(N log log N) |
| binpow | O(log e) |
| KMP / Z-function | O(n) |