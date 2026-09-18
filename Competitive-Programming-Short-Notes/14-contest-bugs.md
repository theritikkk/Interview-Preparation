# 14 — Contest Bugs, Edge Cases, Stress Testing

> Run the matching verdict list **before** you start reading your code line by line.
> 80% of contest bugs are on one of these lists.

---

## WA checklist

```
[ ] int overflow — any product of two values up to 1e9?
[ ] 1LL * a * b, not a * b
[ ] accumulate(..., 0) should be 0LL
[ ] sum of array fits in int? (1e5 * 1e9 = 1e14 → NO)
[ ] modulo went negative — (a - b + MOD) % MOD
[ ] final answer printed without normalizing mod
[ ] 0-indexed vs 1-indexed mismatch (input is 1-indexed, array is 0-indexed)
[ ] <= vs < in a loop bound
[ ] off-by-one in prefix sums: pre[r+1] - pre[l]
[ ] lower_bound where upper_bound was needed
[ ] forgot to sort before binary_search / unique / two-pointers
[ ] comparator not a strict weak ordering (used <=)
[ ] priority_queue direction wrong (default is MAX heap)
[ ] integer division truncating when you wanted ceil
[ ] ceil division on negatives — (a+b-1)/b is only right for a,b > 0
[ ] uninitialized variable / array
[ ] n = 1 case
[ ] answer is 0 / empty / "no solution" case
[ ] duplicates in input assumed distinct
[ ] negative numbers assumed non-negative
[ ] printed the wrong variable (ans vs best vs cur)
[ ] printed "YES"/"NO" with wrong case, or Yes/No mismatch
[ ] read n but looped to m
[ ] greedy that isn't actually correct — did I prove the exchange argument?
[ ] used a stale value after updating it (order of two lines swapped)
[ ] visited[] not reset between test cases
[ ] answer needs long long but printed with %d
```

---

## TLE checklist

```
[ ] complexity actually matches the constraint (check SUM of n, not just n)
[ ] endl instead of '\n'
[ ] missing ios::sync_with_stdio(false); cin.tie(nullptr);
[ ] memset / full-array clear inside the per-testcase loop
[ ] map/unordered_map where a plain array works
[ ] unordered_map hacked (anti-hash test) → use custom_hash or map
[ ] passing vector by value into a function called n times
[ ] for (auto x : v) copying strings/vectors
[ ] string concatenation with s = s + c inside a loop (O(n^2)) → s += c
[ ] erase/insert in the middle of a vector inside a loop (O(n^2))
[ ] recomputing something loop-invariant inside the loop
[ ] nested loop where the inner bound is n, not something smaller
[ ] recursion without memoization
[ ] std::set where a sorted vector + two pointers works
[ ] pow() / sqrt() inside a hot loop
[ ] cerr debug output left in
[ ] O(n) std::lower_bound on a set instead of the member function
```

**The sum-of-n trap:** `t ≤ 1e5` and `n ≤ 1e5` does **not** mean 1e10 work — read the line "it is guaranteed that the sum of n over all test cases does not exceed 2·10^5". If that line exists, per-test O(n log n) is fine. If it **doesn't** exist, you must be O(n) per test or better, or the intended solution is different.

---

## MLE checklist

```
[ ] DP table size — did I multiply the dimensions and by sizeof?
[ ] long long where int suffices (2x memory)
[ ] #define int long long doubling everything
[ ] vector<vector<int>> allocated per test case and never freed
[ ] map<int, vector<int>> — ~48 B/node plus vector headers
[ ] adjacency list for a dense graph
[ ] storing all queries when streaming would do
[ ] recursion depth eating stack (each frame ~50-100 B)
[ ] 2D array indexed [n][n] when only [n][k] is needed
```
Quick math in `11_COMPLEXITY_MEMORY.md`. Rule: **1e6 ints = 4 MB, 1e6 long long = 8 MB.**

---

## RE checklist

```
[ ] array index out of bounds (especially v[i+1] at i = n-1)
[ ] v[-1] from an index computed as i-1 at i = 0
[ ] negative index from a modulo: (x % n) can be negative if x < 0
[ ] .front() / .back() / .top() on an empty container
[ ] *prev(v.begin()) or --s.begin()
[ ] s.erase(s.find(x)) when x is absent → erasing end()
[ ] division / modulo by zero
[ ] recursion depth > stack (1e5 deep DFS on a path graph)
[ ] comparator using <= → sort walks off the array
[ ] iterator invalidated by push_back inside the loop over the same vector
[ ] array sized N but n can be N+1 (or 1-indexed needs N+1)
[ ] stoi on a string that doesn't fit int → use stoll
[ ] reading past EOF
```

**Iterator invalidation, the classic:**
```cpp
for (int x : v) v.push_back(x);   // UB — reallocation invalidates the range-for iterators
```

---

## The INF + INF bug

```cpp
const int INF = 1e9;
if (d[i][k] + d[k][j] < d[i][j])   // 1e9 + 1e9 = 2e9 > INT_MAX → negative → "shortest path"
```
Fixes: use `0x3f3f3f3f` (doubling stays in int), use `long long`, or guard `if (d[i][k] < INF && d[k][j] < INF)`.

Same bug appears in Dijkstra when `dist` is `int` and edge weights sum past 2^31.

---

## Modulo gotchas

```cpp
(-1) % 7            // == -1 in C++ (not 6)
(a - b) % MOD       // can be negative
((x % MOD) + MOD) % MOD   // always safe
(a - b + MOD) % MOD       // safe IF a, b are already in [0, MOD)
```
One `+MOD` is enough only when you know the value is ≥ `-MOD`. After a subtraction of two normalized values, it is. After an arbitrary computation, use the double-mod form.

---

## Edge case checklist (run mentally before submitting)

**Size**
```
[ ] n = 1
[ ] n = 2
[ ] n = maximum
[ ] empty input / empty string / k = 0
```

**Array shape**
```
[ ] all elements equal
[ ] strictly increasing
[ ] strictly decreasing
[ ] all zeros
[ ] all negative
[ ] mixed signs
[ ] many duplicates
[ ] single distinct value repeated n times
[ ] values at the extreme bound (1e9, -1e9)
```

**Answer position**
```
[ ] answer at index 0
[ ] answer at index n-1
[ ] answer is the whole array
[ ] answer is a single element
[ ] no valid answer exists
[ ] multiple valid answers ("print any")
```

**Graphs**
```
[ ] disconnected
[ ] single node, zero edges
[ ] complete graph
[ ] tree that is a path (chain) → recursion depth
[ ] tree that is a star → one node with degree n-1
[ ] self-loops
[ ] multi-edges (duplicate edges)
[ ] directed vs undirected — did I add both directions?
[ ] n nodes but m = 0
```

**Strings**
```
[ ] empty string
[ ] length 1
[ ] all same character
[ ] uppercase/lowercase mix
[ ] characters outside a-z (digits, spaces)
```

**Numeric**
```
[ ] overflow boundary (product of two maximums)
[ ] zero in a denominator or gcd
[ ] gcd(0, x) = x
[ ] x = 0 in bit tricks (__builtin_clz(0) is UB)
[ ] negative modulo
```

---

## Stress testing recipe

`brute.cpp` (obviously correct, slow), `sol.cpp` (fast), `gen.py` (random small input).

```python
# gen.py
import random, sys
seed = int(sys.argv[1]); random.seed(seed)
n = random.randint(1, 6)
print(n)
print(*[random.randint(-5, 5) for _ in range(n)])
```

```bash
#!/bin/bash
g++ -O2 -o sol sol.cpp && g++ -O2 -o brute brute.cpp
for i in $(seq 1 1000); do
    python3 gen.py $i > in.txt
    ./sol   < in.txt > out1.txt
    ./brute < in.txt > out2.txt
    if ! diff -q out1.txt out2.txt > /dev/null; then
        echo "DIFF on seed $i"; cat in.txt; break
    fi
done
```

**Key point:** generate **tiny** inputs (n ≤ 6, values ≤ 5). Bugs surface at n = 2 with duplicates, not at n = 1000.

**For "print any valid answer" problems:** you can't diff. Write a *checker* that validates `out1.txt` against `in.txt` instead.

---

## Debug compile flags

```bash
g++ -std=c++17 -O2 -Wall -Wextra -Wshadow -fsanitize=address,undefined \
    -D_GLIBCXX_DEBUG -g sol.cpp -o sol
```

| Flag | Catches |
|---|---|
| `-fsanitize=address` | out-of-bounds, use-after-free, stack overflow |
| `-fsanitize=undefined` | signed overflow, bad shifts, null deref |
| `-D_GLIBCXX_DEBUG` | STL misuse: bad iterators, invalid ranges, out-of-range `v[i]` |
| `-Wshadow` | a local shadowing a global (silent WA factory) |
| `-Wall -Wextra` | unused/uninitialized variables, sign comparison |

These slow the binary 2–20×. Never submit with them; always run your local tests with them.

**`-fsanitize=undefined` catches signed integer overflow** — which is exactly the bug you can't see. Worth running once on every WA.

---

## When you have a WA and no idea why

1. Re-read the statement. Especially the output format and the constraints line.
2. Check the WA checklist above (60 seconds).
3. Run the provided samples locally — did you actually test all of them?
4. Test n = 1 and the all-equal case by hand.
5. Recompile with sanitizers and run the samples.
6. Write the brute force and stress test. This is faster than staring, almost always.
7. If it's WA on test 1 with a big test: probably output format or a crash printing nothing.
8. If it's WA on a late test: probably overflow or an edge case, not core logic.

**Signal:** WA on test 2 usually means a logic bug. WA on test 40 usually means overflow or an edge case.