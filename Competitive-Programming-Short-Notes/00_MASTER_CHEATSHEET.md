# 00 — Master Cheat Sheet

Single-page lookup. No teaching here — every line is a fact you check, not something you read to learn.
Open this before every contest. Everything is explained in the file named at the end of each section.

---

## Integer ranges

| Type | Bytes | Range | Remember it as |
|---|---|---|---|
| `char` | 1 | −128 … 127 | ±127 |
| `unsigned char` | 1 | 0 … 255 | 255 |
| `short` | 2 | −32,768 … 32,767 | ±3.2e4 |
| `int` | 4 | −2,147,483,648 … 2,147,483,647 | **±2.147e9** |
| `unsigned int` | 4 | 0 … 4,294,967,295 | **4.29e9** |
| `long` (Linux 64-bit) | 8 | same as `long long` | — |
| `long long` | 8 | −9,223,372,036,854,775,808 … 9,223,372,036,854,775,807 | **±9.22e18** |
| `unsigned long long` | 8 | 0 … 18,446,744,073,709,551,615 | **1.84e19** |
| `__int128` | 16 | ≈ ±1.7e38 | ±1.7e38 (64-bit compilers only) |
| `float` | 4 | exact integers to 2^24 ≈ 1.7e7 | never use it |
| `double` | 8 | exact integers to 2^53 ≈ **9.0e15** | 15–16 digits |
| `long double` | 16 (10 used) | exact integers to 2^64 ≈ 1.8e19 | 18–19 digits |

```cpp
INT_MAX   = 2147483647          INT_MIN  = -2147483648
UINT_MAX  = 4294967295
LLONG_MAX = 9223372036854775807 LLONG_MIN = -9223372036854775808
ULLONG_MAX= 18446744073709551615
```

**Literal suffixes:** `1LL` (long long), `1ULL` (unsigned long long), `1e9` is a **double** — `(int)1e9` to keep it integral.

---

## Overflow rules

```cpp
long long x = 1LL * a * b;        // ✅ promote BEFORE multiplying
long long x = a * b;              // ❌ int*int overflows first, then widens

ll l = a / gcd(a,b) * b;          // ✅ lcm: divide first
accumulate(all(v), 0LL);          // ✅ accumulator type = 3rd argument
int mid = lo + (hi - lo) / 2;     // ✅ midpoint, no (lo+hi) overflow
1LL << k                          // ✅ whenever k can reach 31
```

**Where overflow actually bites:**

| Situation | Max value | Type needed |
|---|---|---|
| Sum of 1e5 values ≤ 1e9 | 1e14 | `long long` |
| Sum of 2e5 values ≤ 1e9 | 2e14 | `long long` |
| Product of two values ≤ 1e9 | 1e18 | `long long` |
| Product of two values ≤ 1e18 | 1e36 | `__int128` |
| Graph distance, n ≤ 2e5, w ≤ 1e9 | 2e14 | `long long` |
| Floyd `INF + INF` with `INF = 1e9` | 2e9 | **overflows `int`** → use `0x3f3f3f3f` |
| `lcm(a, b)` unguarded | a·b | divide first |
| `nCr` without a modulus | 20! = 2.4e18 is the ceiling | `long long` up to n=20 |
| Squared distance, coords ≤ 1e9 | 8e18 | `long long`, barely |
| Cross product, coords ≤ 1e9 | 2e18 | `long long` |
| `i * i` in a sieve with N ≈ 1e9 | overflows at i > 46341 | cast to `ll` |

**Useful INF constants:**
```cpp
const int  IINF = 0x3f3f3f3f;              // 1.06e9  — IINF + IINF still fits in int
const ll   LINF = 0x3f3f3f3f3f3f3f3fLL;    // 4.5e18  — LINF + LINF still fits in ll
memset(dist, 0x3f, sizeof dist);           // fills with exactly IINF (memset writes BYTES)
```

**`__int128` is necessary when:** multiplying two values > 3e9 under a modulus, hashing mod 2^61−1, or comparing fractions `a*d` vs `c*b` with large terms. Printer in `16_MICRO_TEMPLATES.md`. **Needs a 64-bit compiler** — CF's `GNU G++17 7.3.0` is 32-bit and has no `__int128`.

---

## N → complexity

| n | Allowed | Technique |
|---|---|---|
| ≤ 10 | O(n!) | permutations |
| ≤ 20 | O(2ⁿ·n) | **bitmask DP** |
| ≤ 24 | O(2ⁿ) | subsets |
| ≤ 40 | O(2^(n/2)) | **meet in the middle** |
| ≤ 100 | O(n⁴) | 4 loops |
| ≤ 500 | O(n³) | Floyd, interval DP, matrix |
| ≤ 5,000 | O(n²) | 2D DP, LCS |
| ≤ 1e5 | O(n log² n) | segtree + inner search |
| ≤ 2e5 | O(n log n) | sort, segtree, Dijkstra, BS on answer |
| ≤ 1e6 | O(n log n), small constant | — |
| ≤ 1e7 | O(n) | single pass, sieve |
| ≤ 1e9 | O(√n) / O(log n) | math, digit DP |
| ≤ 1e18 | O(log n) | binpow, matrix power, digit DP |

CF machines do **~1e8–1e9 simple ops/sec**. Modulo, `map`, and cache misses each cost 10–40×.

**⚠ Always read: "the sum of n over all test cases does not exceed …"** — that line decides whether per-test O(n log n) is legal. → `11_COMPLEXITY_MEMORY.md`

---

## Memory

```
1e6 int   = 4 MB      1e6 long long = 8 MB      1e7 int = 40 MB
1e6 pair<int,int> = 8 MB                        bitset<1e6> = 125 KB
map<int,int> with 1e6 entries ≈ 50 MB  (~48-56 B/node)
vector<T> header = 24 B     string header = 32 B (≤15 chars inline)
```

**Will this DP fit?** `dims × dims × sizeof ≤ ~200 MB` (CF limit is usually 256 MB).

```
dp[2000][2000]   int  = 16 MB   ✅
dp[5000][5000]   int  = 100 MB  ⚠ roll it
dp[10000][10000] int  = 400 MB  ❌
dp[1<<20][20]    int  = 84 MB   ⚠
dp[1<<18][18]    int  = 19 MB   ✅
```
Doesn't fit → rolling array, backward in-place knapsack, smaller type, or **rethink the state**.
→ `11_COMPLEXITY_MEMORY.md`, `19_DP_PATTERNS.md`

---

## Decision tables

### Shortest path
| Situation | Use |
|---|---|
| unweighted | BFS |
| weights 0/1 | 0-1 BFS (deque: 0 → front, 1 → back) |
| non-negative weights | Dijkstra (`dist` must be `long long`) |
| negative edges | Bellman–Ford |
| all pairs, n ≤ 500 | Floyd–Warshall (`k,i,j`; watch INF+INF) |
| MST | Kruskal + DSU |
→ `07_GRAPH_TEMPLATES.md`

### Range queries
| Updates | Query | Structure |
|---|---|---|
| none | range sum | prefix sum |
| none | range min/max/gcd | sparse table, O(1) |
| range add | point read | difference array |
| point add | range sum | **Fenwick** |
| range add | range sum | 2 Fenwicks / lazy segtree |
| point set | range anything | segment tree |
| range assign | range anything | lazy segtree |
| offline, no updates | weird aggregate | Mo's, O((n+q)√n) |
| union only | connectivity | DSU |
→ `10_ADVANCED_DATA_STRUCTURES.md`, `06_PREFIX_BINARY_SEARCH_TRICKS.md`

### Which container
| Need | Use |
|---|---|
| integer keys bounded by ~1e7 | **plain array** (10–100× faster than any map) |
| ordered, `lower_bound`, min/max | `set` / `map` |
| unordered lookup | `unordered_map` **with `custom_hash`** (hackable otherwise) |
| both ends | `deque` |
| top-K / Dijkstra | `priority_queue` |
| k-th smallest with inserts | Fenwick descent or pb_ds `ordered_set` |
| boolean inner loop over n² | `bitset` (n²/64) |
→ `02_STL_CHEATSHEET.md`, `12_CPP_PERFORMANCE.md`

### Which search
| Signal | Use |
|---|---|
| sorted array, find a value | `lower_bound` |
| monotonic feasibility `FFFTTT` | binary search the answer |
| unimodal function | ternary search |
| positive values, contiguous window | two pointers |
| window min/max | monotonic deque |
| next greater / contribution counting | monotonic stack |
→ `06_PREFIX_BINARY_SEARCH_TRICKS.md`, `15_PROBLEM_SOLVING_HEURISTICS.md`

---

## The 25 one-liners you keep forgetting

```
 1  lower_bound = first >= x      upper_bound = first > x
 2  count of x  = upper_bound - lower_bound
 3  priority_queue<int> is a MAX heap; greater<int> makes it MIN
 4  priority_queue comparator sign is INVERTED vs sort
 5  sort on pairs = first asc, then second asc (free, no comparator)
 6  comparator must use <, never <= (UB, real segfaults)
 7  multiset.erase(value) removes ALL; erase(iterator) removes ONE
 8  map[key] INSERTS the key with 0
 9  unique() only collapses ADJACENT duplicates and does NOT erase
10  sort(all(v)); v.erase(unique(all(v)), v.end());
11  min_element / max_element return ITERATORS — dereference them
12  v.size() is UNSIGNED — (int) before subtracting
13  set: use the MEMBER lower_bound; std::lower_bound on a set is O(n)
14  long long x = 1LL * a * b;   never a * b
15  accumulate(all(v), 0LL);     never 0
16  lcm = a / gcd(a,b) * b;      divide first
17  (a - b + MOD) % MOD;         C++ % keeps the sign of the dividend, (-1)%7 == -1
18  ceil(a/b) = (a + b - 1) / b  for a,b > 0
19  1LL << k whenever k can reach 31
20  __builtin_clz(0) and __builtin_ctz(0) are UNDEFINED
21  __builtin_popcount takes unsigned int; use popcountll for long long
22  memset writes BYTES: 0, -1, 0x3f are safe; memset(a,1,..) gives 16843009
23  x & (x-1) KILLS the lowest set bit; x & -x KEEPS it
24  pre[i] = sum of first i;  range [l,r] = pre[r+1] - pre[l]
25  2D rectangle = big - left - top + corner
```

Bonus, because they cost whole problems:
```
26  weighted graph → long long distances
27  undirected edge → push_back BOTH directions
28  clear only 1..n between test cases, never memset the whole array
29  knapsack: BACKWARD loop = 0/1, FORWARD loop = unbounded
30  LIS: lower_bound = strictly increasing, upper_bound = non-decreasing
```

---

## 30-second pre-submission check

```
1.  Complexity matches the constraint?   (and the SUM of n)
2.  Overflow — any int*int, or a sum past 2e9?
3.  Bounds — array sized n+1 if 1-indexed?
4.  n = 1 / empty / all-equal handled?
5.  State reset between test cases?
6.  Modulo can go negative?
7.  Did something need sorting first?
8.  Heap direction correct?
9.  Binary search bounds and loop style consistent?
10. Output format, case, and trailing newline?
```
→ `14_CONTEST_BUGS.md` for the full WA / TLE / MLE / RE lists.

---

## Index

| File | What's in it |
|---|---|
| `00_MASTER_CHEATSHEET.md` | This page — ranges, overflow, complexity, decision tables, the 25 one-liners |
| `01_CPP_CONTEST_TRICKS.md` | Fast I/O, boilerplate, `0x3f3f3f3f`, `#define int long long` dangers, copy traps, `reserve` vs `resize`, pragmas |
| `02_STL_CHEATSHEET.md` | Every container with complexity and memory, `lower_bound`/`upper_bound` in depth, sorting tricks, strict weak ordering, hardened `unordered_map`, pb_ds |
| `03_MATH_NUMBER_THEORY.md` | gcd/lcm, extended Euclid, sieves, SPF, factorization, divisors, Euler φ, floor-division blocks, CRT, matrix power |
| `04_MODULAR_ARITHMETIC.md` | Negative `%`, binpow, Fermat inverse, `fact`/`invFact`, non-prime moduli, `Mint` |
| `05_BIT_MANIPULATION.md` | `x&(x-1)` vs `x&-x`, builtins and their UB, submask enumeration (3ⁿ), XOR properties, `bitset` |
| `06_PREFIX_BINARY_SEARCH_TRICKS.md` | Prefix/difference arrays 1D & 2D, two pointers and when they fail, binary search templates, ternary search, coordinate compression, sweep line, intervals |
| `07_GRAPH_TEMPLATES.md` | BFS/DFS, topo sort, Dijkstra + its 5 traps, 0-1 BFS, Bellman–Ford, Floyd, bipartite, Kruskal |
| `08_STRING_TRICKS.md` | Char arithmetic, frequency arrays, `getline` traps, SSO, Z-function, KMP + the period trick, safe rolling hash, Trie, palindromes |
| `09_COMBINATORICS.md` | Identities, stars and bars, inclusion–exclusion, pigeonhole, Catalan, derangements, counting by contribution |
| `10_ADVANCED_DATA_STRUCTURES.md` | Fenwick (all variants), segment trees, lazy propagation, sparse table, sqrt decomposition, Mo's, binary trie |
| `11_COMPLEXITY_MEMORY.md` | Ops/sec, the N→complexity table, the sum-of-n trap, container overheads, DP sizing ritual |
| `12_CPP_PERFORMANCE.md` | Cache hierarchy, row-major order, vector growth, stack vs heap, branch prediction, why arrays beat hash maps |
| `13_QUANT_CPP_INTERVIEW.md` | ~30 Q&A in short/deep/common-wrong-answer format: containers, vector internals, iterator invalidation, RAII, move semantics, UB, `inline`, `constexpr`, SSO |
| `14_CONTEST_BUGS.md` | WA / TLE / MLE / RE checklists, edge cases, stress-test recipe, sanitizer flags |
| `15_PROBLEM_SOLVING_HEURISTICS.md` | Constraint→technique table, the 20 heuristics, the stuck-for-15-minutes checklist, contest strategy |
| `16_MICRO_TEMPLATES.md` | Every copy-paste template in one place |
| `17_PRE_CONTEST_REVISION.md` | 5 / 15 / 30-minute revision blocks, memorize-cold vs keep-templates |
| `18_THINGS_I_ALWAYS_FORGET.md` | **Yours.** Seeded, then append your own failure after every contest |
| `19_DP_PATTERNS.md` | State design, knapsack directions, LIS, interval/bitmask/digit/tree DP, optimizations, memory sizing |
| `20_TREES.md` | Subtree sizes, diameter, LCA, Euler tour, rerooting, small-to-large, centroid, HLD |
| `21_GAME_THEORY.md` | Win/lose positions, Nim, Sprague–Grundy, variants, recognition checklist |
| `22_GEOMETRY_BASICS.md` | Stay-in-integers, cross product, shoelace, Pick's theorem, segment intersection, convex hull, angle sorting |
| `23_INTERACTIVE_AND_IO.md` | Flushing, query budgets, adaptive judges, custom fast reader, output precision |
| `24_FLOATING_POINT_AND_RANDOM.md` | Precision limits, EPS choice, avoiding floats, `mt19937`, Zobrist hashing |
| `25_DSU_TEMPLATE.md` | DSU with all variants and applications |
| `26_ADVANCED_GRAPH_ALGORITHMS.md` | SCC (Tarjan's & Kosaraju's), bridges & articulation points, 2-SAT, Eulerian paths |
| `27_ADVANCED_STRINGS_AND_STRUCTURES.md` | Manacher's, Aho-Corasick, heavy-light decomposition, centroid decomposition, convex hull trick, Li Chao tree |
| `28_FLOWS_AND_MATCHING.md` | Dinic's max flow, Kuhn's bipartite matching, König's theorem, flow modeling patterns |
| `29_POLYNOMIAL_AND_ADVANCED_NUMBER_THEORY.md` | FFT, NTT, Pollard's rho, Möbius function & inversion, discrete log (BSGS) |
| `30_SUFFIX_STRUCTURES_AND_PERSISTENCE.md` | Suffix array + Kasai's LCP, suffix automaton, persistent segment tree, offline LCA via Tarjan's + DSU |
| `31_EXAMPLE_PROBLEMS_BY_TECHNIQUE.md` | 27 original practice problems, one per major technique, for recognition drilling |

**Revision cadence:** before every contest read `00`, `18`, and the 5-minute block of `17`. Weekly rotate `02`, `06`, `15`, `14`. Monthly `12`, `13`. After every contest, append to `18`.