# 31 — Example Problems By Technique

Every problem below is written for this handbook — not copied from any judge — specifically to demonstrate one recognition pattern at a time. Read the problem, cover the Approach line, and try to name the technique from the constraints and phrasing alone before checking. That recognition skill — not the code — is what `15_PROBLEM_SOLVING_HEURISTICS.md` and this file are training.

Format: **Problem** (self-contained, with constraints) → **Recognition** (what tips you off) → **Approach** (the technique + file).

---

### Binary search on the answer
**Problem:** n workers, worker `i` produces `goods[i]` items per hour. All workers run in parallel for the same integer number of hours `h`. Find the minimum `h` such that total production is at least `K`. `n ≤ 2·10^5`, `goods[i] ≤ 10^9`, `K ≤ 10^18`.
**Recognition:** "minimum X such that f(X) ≥ target", and `f` only gets larger as `X` grows.
**Approach:** binary search `h`; the check `Σ goods[i]·h ≥ K` is O(n) and monotonic. → `06_PREFIX_BINARY_SEARCH_TRICKS.md`.

### Two pointers / sliding window
**Problem:** array of `n` positive integers and a target `S`. Find the length of the shortest contiguous subarray with sum ≥ `S`. `n ≤ 2·10^5`, `a_i ≤ 10^9`.
**Recognition:** contiguous subarray, sum-based condition, all values positive.
**Approach:** expand the right pointer, shrink the left while still valid, track the minimum window. → `06_PREFIX_BINARY_SEARCH_TRICKS.md`.

### Prefix sums / difference array
**Problem:** `q` queries of the form "add `v` to every element in `[l, r]`" applied to an array of `n` zeros, in order. Output the final array. `n, q ≤ 2·10^5`.
**Recognition:** many range updates, one final read, no updates interleaved with reads.
**Approach:** difference array — `d[l] += v; d[r+1] -= v`, then one prefix-sum pass at the end. → `06_PREFIX_BINARY_SEARCH_TRICKS.md`.

### DSU / Kruskal
**Problem:** `n` cities, `m` candidate roads each with a cost. Choose a minimum-cost subset of roads so every city can reach every other; output the cost, or −1 if impossible. `n, m ≤ 2·10^5`.
**Recognition:** minimum spanning tree, worded as connecting everything as cheaply as possible.
**Approach:** sort edges by weight, Kruskal + DSU, check `used == n-1` at the end. → `07_GRAPH_TEMPLATES.md`, `25_DSU_TEMPLATE.md`.

### Dijkstra
**Problem:** weighted directed graph, `n` nodes, `m` edges, non-negative weights. Output the shortest distance from node 1 to every node. `n, m ≤ 2·10^5`, weights `≤ 10^9`.
**Recognition:** single-source shortest path, non-negative weights.
**Approach:** Dijkstra with a min-heap; `dist` must be `long long` (paths up to ~2·10^14). → `07_GRAPH_TEMPLATES.md`.

### 0/1 Knapsack
**Problem:** `n` items each with a weight and a value, capacity `W`; each item usable at most once. Maximize total value. `n ≤ 1000`, `W ≤ 10^5`.
**Recognition:** the canonical knapsack shape — bounded capacity, each item once.
**Approach:** `dp[w]` over capacity, looped **backward** per item (forward would allow reuse). → `19_DP_PATTERNS.md`.

### Longest increasing subsequence
**Problem:** array of `n` integers; find the length of the longest strictly increasing subsequence. `n ≤ 2·10^5`.
**Recognition:** classic LIS shape, `n` too large for the O(n²) DP.
**Approach:** patience-sorting with `lower_bound` (strict) for O(n log n). → `19_DP_PATTERNS.md`.

### Bitmask DP — assignment problem
**Problem:** `n ≤ 18` workers and `n` tasks, `cost[i][j]` to assign worker `i` to task `j`. Find the minimum-cost perfect assignment.
**Recognition:** `n ≤ 20`, a permutation/matching structure between two equal-size sets.
**Approach:** `dp[mask]` = min cost to assign the first `popcount(mask)` workers to the task-set `mask`. → `19_DP_PATTERNS.md`.

### Modular exponentiation + nCr
**Problem:** `q` queries, each giving `(n_i, r_i)` with `n_i ≤ 2·10^5`; output `C(n_i, r_i) mod (10^9+7)` for each. `q ≤ 2·10^5`.
**Recognition:** many binomial-coefficient queries under one prime modulus.
**Approach:** precompute `fact[]`/`invFact[]` once (one `binpow` call), then O(1) per query. → `04_MODULAR_ARITHMETIC.md`.

### Combinatorics — stars and bars
**Problem:** count the number of ways to give `n` identical candies to `k` distinct children so every child gets at least one, mod `10^9+7`. `n, k ≤ 2·10^5`.
**Recognition:** identical items into distinct boxes with a minimum-per-box constraint.
**Approach:** `C(n-1, k-1)`. → `09_COMBINATORICS.md`.

### Fenwick tree — counting inversions
**Problem:** array of `n` distinct integers; count pairs `(i, j)` with `i < j` and `a[i] > a[j]`. `n ≤ 2·10^5`.
**Recognition:** counting inversions — pairs out of order.
**Approach:** coordinate-compress, sweep left to right, Fenwick counts how many already-inserted values exceed the current one. → `10_ADVANCED_DATA_STRUCTURES.md`.

### Segment tree — range min with updates
**Problem:** array of `n` integers, `q` queries each either "set `a[i] = v`" or "report the minimum in `[l, r]`". `n, q ≤ 2·10^5`.
**Recognition:** point updates interleaved with range queries — a static prefix-sum won't do, and min isn't invertible so a Fenwick won't either.
**Approach:** segment tree, point update / range query. → `10_ADVANCED_DATA_STRUCTURES.md`.

### KMP — pattern counting
**Problem:** text of length `n`, pattern of length `m`; count (possibly overlapping) occurrences of the pattern in the text. `n, m ≤ 10^6`.
**Recognition:** single-pattern search that must run in O(n + m), not O(n·m).
**Approach:** KMP prefix function on `pattern + '#' + text`, count positions where `pi[i] == m`. → `08_STRING_TRICKS.md`.

### Rolling hash — substring equality
**Problem:** string `s` of length `n` and `q` queries, each asking whether two given substrings (by index ranges) are equal. `n, q ≤ 2·10^5`.
**Recognition:** many O(1) substring-equality checks needed.
**Approach:** safe rolling hash with a random base; compare `hash(l1,r1)` and `hash(l2,r2)`. → `08_STRING_TRICKS.md`.

### Game theory — Nim
**Problem:** `k` piles of stones; players alternate removing any positive number of stones from a single pile; the player who cannot move loses. Determine the winner under optimal play. `k ≤ 10^5`.
**Recognition:** the textbook Nim shape.
**Approach:** XOR all pile sizes — non-zero means the first player wins. → `21_GAME_THEORY.md`.

### Geometry — convex hull area
**Problem:** `n` points; find the area of their convex hull. `n ≤ 10^5`, coordinates `≤ 10^9`.
**Recognition:** convex hull construction followed by an area computation.
**Approach:** Andrew's monotone chain, then the shoelace formula — keep it as 2×area to stay in integers until the final print. → `22_GEOMETRY_BASICS.md`.

### Greedy — interval scheduling
**Problem:** `n` intervals `[l_i, r_i]`; select the maximum number that don't overlap. `n ≤ 2·10^5`.
**Recognition:** classic activity-selection phrasing.
**Approach:** sort by **right endpoint**, greedily take the next interval that starts after the last one taken ends. → `06_PREFIX_BINARY_SEARCH_TRICKS.md`, `15_PROBLEM_SOLVING_HEURISTICS.md`.

### Bit manipulation — maximum XOR pair
**Problem:** `n` non-negative integers, each `< 2^30`; find the maximum XOR of any pair. `n ≤ 10^5`.
**Recognition:** maximum-XOR-of-a-pair, `n` too large for O(n²).
**Approach:** binary trie over bits, greedily prefer the opposite bit from the high end down. → `05_BIT_MANIPULATION.md`, `10_ADVANCED_DATA_STRUCTURES.md`.

### Tree — diameter
**Problem:** a tree on `n` nodes; find the number of edges on the longest path. `n ≤ 2·10^5`.
**Recognition:** the classic tree-diameter phrasing.
**Approach:** BFS from any node to find a farthest node `a`, then BFS from `a` — the farthest node from `a` is diameter-distant. → `20_TREES.md`.

### Sweep line — maximum overlap
**Problem:** `n` meeting time intervals; find the maximum number that are simultaneously active at any single instant. `n ≤ 2·10^5`.
**Recognition:** "maximum simultaneous" over intervals.
**Approach:** convert to +1/−1 events at start/end+1, sort, sweep with a running counter. → `06_PREFIX_BINARY_SEARCH_TRICKS.md`.

### 2-SAT
**Problem:** `n` boolean variables and `m` constraints, each "at least one of these two literals is true" (a literal is a variable or its negation). Decide if a satisfying assignment exists, and produce one if so. `n, m ≤ 2·10^5`.
**Recognition:** exactly the 2-SAT shape — two mutually-constraining boolean choices per clause.
**Approach:** build the implication graph, run SCC, check no variable's two literals share a component. → `26_ADVANCED_GRAPH_ALGORITHMS.md`.

### Bipartite matching
**Problem:** `n` applicants, `m` jobs; applicant `i` is qualified for a given subset of jobs. Find the maximum number of applicants that can be matched to distinct jobs. `n, m ≤ 500`.
**Recognition:** maximum matching between two distinct groups.
**Approach:** Kuhn's algorithm (small enough here that O(V·E) is fine; use Dinic's as unit-capacity flow if this scaled to thousands). → `28_FLOWS_AND_MATCHING.md`.

### Meet in the middle
**Problem:** `n ≤ 40` items, each with an integer weight, and a target `T`. Determine whether some subset sums to exactly `T`.
**Recognition:** `n ≤ 40` is the signature — too big for `2^n`, too small to need anything cleverer.
**Approach:** split into two halves, enumerate `2^20` sums per half, sort one half, binary search from the other. → `16_MICRO_TEMPLATES.md`.

### Mo's algorithm
**Problem:** array of `n` integers, `q` offline queries "how many distinct values appear in `[l, r]`", no updates. `n, q ≤ 2·10^5`.
**Recognition:** offline range-distinct-count with no updates — a segment tree can't merge "distinct count" directly, and there's no online requirement forcing per-query independence.
**Approach:** Mo's algorithm, moving `l`/`r` one step at a time with a frequency array tracking the current distinct count. → `10_ADVANCED_DATA_STRUCTURES.md`.

### Digit DP
**Problem:** count integers in `[L, R]` whose decimal digit sum is divisible by `k`. `L, R ≤ 10^18`, `k ≤ 100`.
**Recognition:** "count numbers in a range with a property depending on digits", range far too large to iterate.
**Approach:** digit DP with state `(position, tight, digitSum mod k)`, answer `f(R) - f(L-1)`. → `19_DP_PATTERNS.md`.

### Strongly connected components
**Problem:** a directed graph of `n` nodes and `m` edges. Count how many pairs of nodes can each reach the other. `n, m ≤ 2·10^5`.
**Recognition:** mutual reachability in a directed graph — the definition of an SCC.
**Approach:** Tarjan's or Kosaraju's, then for each SCC of size `s` add `s·(s-1)` to the answer (ordered pairs) or `C(s,2)` (unordered). → `26_ADVANCED_GRAPH_ALGORITHMS.md`.

### Sieve + factorization
**Problem:** `q` queries, each giving an integer `x ≤ 10^6`; output the number of divisors of `x`. `q ≤ 2·10^5`.
**Recognition:** many divisor-count queries over a bounded range — a per-query O(√x) factorization would be too slow in aggregate.
**Approach:** smallest-prime-factor sieve up to `10^6` once, factorize each query in O(log x), multiply `(exponent+1)` over the prime factorization. → `03_MATH_NUMBER_THEORY.md`.

---

## How to use this file

Don't read it top to bottom once and move on. Instead: skim just the **Problem** lines, cover everything else, and for each one say out loud which file you'd open and why — that's the actual skill `00_MASTER_CHEATSHEET.md`'s decision tables and `15_PROBLEM_SOLVING_HEURISTICS.md`'s constraint table are trying to train. If a problem's recognition doesn't come immediately, that pinpoints exactly which file needs another pass before your next contest.