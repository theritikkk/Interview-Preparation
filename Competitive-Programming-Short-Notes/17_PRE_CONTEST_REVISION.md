# 17 — Pre-Contest Revision

Four tiers, plus the memorize/template split. Pick the tier that fits the time you have.

---

# 5-Minute Revision

Read this and nothing else if the contest starts in five minutes.

```
lower_bound  →  first >= x
upper_bound  →  first >  x
count of x   =  upper_bound - lower_bound

priority_queue<int>                              → MAX heap
priority_queue<int, vector<int>, greater<int>>   → MIN heap

sort(pair) → first asc, then second asc
comparator MUST use <, never <=

multiset.erase(value)    → removes ALL
multiset.erase(iterator) → removes ONE
map[key]                 → INSERTS the key

sort(all(v)); v.erase(unique(all(v)), v.end());

long long x = 1LL * a * b;        not a * b
accumulate(all(v), 0LL);          not 0
lcm = a / gcd(a,b) * b;           divide first
(a - b + MOD) % MOD               not (a-b) % MOD
ceil(a/b) = (a + b - 1) / b       for a,b > 0
1LL << k                          whenever k >= 31

x & (x-1)  kills the lowest set bit
x & -x     keeps the lowest set bit
__builtin_clz/ctz(0) is UNDEFINED

pre[i] = sum of first i;  range = pre[r+1] - pre[l]
2D query: big - left - top + corner

n <= 20  → bitmask
n <= 40  → meet in the middle
n <= 500 → O(n^3)
n <= 5e3 → O(n^2)
n <= 2e5 → O(n log n)
n <= 1e7 → O(n)

1e6 int = 4 MB, 1e6 ll = 8 MB
weighted graph distances → LONG LONG

READ "the sum of n over all test cases"
```

---

# 15-Minute Revision

The 5-minute block, plus:

## STL

```cpp
ios::sync_with_stdio(false); cin.tie(nullptr);   // never in interactive
'\n' not endl                                    // except interactive

v.reserve(n)  vs  v.resize(n)          // capacity vs size
for (const auto& x : v)                // avoid copies
vector<int> a = b;                     // O(n) COPY

set.lower_bound(x)                     // member = O(log n)
std::lower_bound(s.begin(),...)        // O(n) — wrong tool

unique() only collapses ADJACENT duplicates and doesn't erase
min_element returns an ITERATOR — dereference it
v.size() is UNSIGNED — cast before subtracting

unordered_map on CF is hackable → custom_hash, or use map, or use an array
```

## Binary search

```cpp
// first true
while (lo < hi) { int m = lo + (hi-lo)/2; if (ok(m)) hi = m; else lo = m+1; }
// last true — note the +1
while (lo < hi) { int m = lo + (hi-lo+1)/2; if (ok(m)) lo = m; else hi = m-1; }
// doubles: 100 fixed iterations, never while(hi-lo > eps)
```
Recognition: **monotonic feasibility → binary search the answer.**

## Modular

```cpp
(a+b)%M   (a-b+M)%M   1LL*a*b%M
inv(a) = binpow(a, M-2, M)          // M PRIME only
nCr = fact[n] * invFact[r] % M * invFact[n-r] % M    // guard r<0 || r>n
(-1) % 7 == -1 in C++
```

## Graph recognition

```
unweighted  → BFS
0/1 weights → 0-1 BFS (deque: 0 to front, 1 to back)
non-negative→ Dijkstra (dist must be long long)
negative    → Bellman-Ford
all pairs, n<=500 → Floyd (loop order k,i,j; watch INF+INF)
MST         → Kruskal + DSU
```

## Bit

```cpp
1LL << k                     __builtin_popcountll(x)
x & (x-1)  kill lowest 1     x & -x  keep lowest 1
31 - __builtin_clz(x)        // floor(log2)
for (int s = m; s; s = (s-1) & m)    // submasks, 3^n total
```

## Before submitting
```
Complexity? Overflow? Bounds? n=1? Reset per test? Mod negative?
Sorted? Heap direction? Binary-search bounds? Output format?
```

---

# 30-Minute Revision

The 15-minute block, plus:

## Skim these files, in this order
1. `18_THINGS_I_ALWAYS_FORGET.md` — **always first, it's your own failure log**
2. `14_CONTEST_BUGS.md` — the WA and TLE lists
3. `15_PROBLEM_SOLVING_HEURISTICS.md` — the 20 heuristics and the constraint table
4. `11_COMPLEXITY_MEMORY.md` — the N→complexity table
5. `02_STL_CHEATSHEET.md` — the lower_bound and sorting sections

## Re-derive these from memory (don't just read them)
- Fenwick `add` and `sum` (the `i & -i` loops)
- DSU `find` and `unite`
- `binpow`
- Dijkstra with lazy deletion
- The first-true binary search template
- Prefix sum indexing: `pre[r+1] - pre[l]`

If you can't write these from memory in 2 minutes each, that's your practice list.

## Re-read the heuristic list out loud
Sort it. Reverse it. Binary search the answer. Prefix/suffix. Frequencies. Parity. Invariant. Graph. Greedy exchange. Contributions. Complement. Offline. Compress. Precompute. Bitmask. Smaller state. gcd. Intervals.

---

# Things to Memorize Cold

You should be able to produce these without thinking, and without looking anything up.

**Semantics**
- `lower_bound` = first `>= x`; `upper_bound` = first `> x`
- `priority_queue` default = max heap; `greater<>` = min heap
- `sort` on pairs = first asc, then second asc
- `multiset::erase(value)` removes all; `erase(iterator)` removes one
- `map[key]` inserts
- `unique` needs a sort and doesn't erase
- comparator strictness (`<`, never `<=`)
- iterator invalidation for `vector` vs `set`/`map`

**Numbers**
- `int` max ≈ 2.1e9; `long long` max ≈ 9.2e18
- 1e6 int = 4 MB; 1e6 `long long` = 8 MB
- `double` is exact for integers only up to 2^53 ≈ 9e15
- 20! is the last factorial fitting in `long long`
- 2^10 = 1024, 2^20 ≈ 1e6, 2^30 ≈ 1e9, 2^60 ≈ 1e18
- CF does ~1e8–1e9 simple ops/second

**Rules**
- `1LL * a * b` before assigning to `long long`
- `a / gcd(a,b) * b` for lcm
- `(a - b + MOD) % MOD`
- `(a + b - 1) / b` for ceil of positives
- `1LL << k` for k ≥ 31
- `__builtin_clz/ctz(0)` is UB
- `accumulate(..., 0LL)`
- weighted graph → `long long` distances
- `INF + INF` must not overflow → use `0x3f3f3f3f`

**Bit tricks**
- `x & (x-1)` kills the lowest set bit
- `x & -x` keeps it
- `x > 0 && (x & (x-1)) == 0` → power of two
- submask loop `s = (s-1) & mask`, total 3^n

**Reasoning shapes**
- The constraint → complexity table
- Binary search on answer: "can I ask YES/NO about a candidate?"
- Sliding window works for positive values, not mixed signs
- "exactly K" = `atMost(K) − atMost(K−1)`
- "sum over all subarrays" → per-element contribution
- "at least one" → complement
- Nim: XOR the piles, zero means you lose
- Removal is hard, addition is easy → process backwards

**Indexing**
- `pre[i]` = sum of the first i elements; query `pre[r+1] − pre[l]`
- 2D rectangle: big − left − top + corner
- Difference array: `d[l] += v; d[r+1] -= v`
- Fenwick is 1-indexed

---

# Things NOT Worth Memorizing — Keep Templates

Trying to write these from memory under time pressure costs more than the 30 seconds it takes to paste them, and a subtle transcription error costs you the problem.

- **Segment tree** (especially lazy propagation)
- **KMP prefix function** and **Z-function** — short, but the exact index arithmetic is easy to fumble
- **Manacher**
- **Rolling hash** with a 2^61−1 modulus
- **Miller–Rabin** base list
- **Extended Euclid** and CRT
- **Matrix exponentiation** scaffolding
- **LCA binary lifting**
- **Mo's algorithm** block ordering
- **Heavy-light decomposition**
- **Convex hull** (the `<=` vs `<` decision is the only part you must think about)
- **`__int128` printer**
- **Custom hash / splitmix64 constants**
- **Fast custom reader**
- **Stress-test bash script**
- **Tarjan's / Kosaraju's SCC** — the topological-order direction is easy to get backwards from memory
- **2-SAT** implication-graph construction and the assignment-extraction comparison
- **Bridges / articulation points** — the strict-`>` vs `>=` distinction between the two
- **Aho-Corasick** build step
- **Centroid decomposition** skeleton and its inclusion-exclusion step
- **Convex hull trick** and **Li Chao tree**
- **Dinic's max flow** and **Kuhn's bipartite matching**

**The distinction that matters:** memorize *semantics and reasoning*, template *implementations*. You need to know **that** `lower_bound` returns the first `>= x` in order to think; you do **not** need to know the exact loop body of a lazy segment tree in order to think.

---

# The 30-second pre-submission check

```
1. Complexity matches the constraint?          (and the SUM of n)
2. Overflow — any int*int, or a sum over 2e9?
3. Bounds — array sized n+1 if 1-indexed?
4. n = 1 / empty / all-equal handled?
5. State reset between test cases?
6. Modulo can go negative?
7. Did something need sorting first?
8. Heap direction correct?
9. Binary search boundaries and loop style consistent?
10. Output format, case, and newline correct?
```

Say them as a list, not as questions you answer carefully. Ten seconds each is too slow — this should be a rhythm you can run in half a minute.