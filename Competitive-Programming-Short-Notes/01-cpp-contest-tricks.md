# 01 — C++ Contest Tricks

> Assume **GNU C++17 (64 bit)** on Codeforces unless stated otherwise.

---

## Fast I/O

### `sync_with_stdio(false)` + `cin.tie(nullptr)`

**Remember:** Two independent switches. One unties C streams, one unties input from output.

**Use when:** Always, except interactive problems.

```cpp
ios::sync_with_stdio(false);
cin.tie(nullptr);
```

- `sync_with_stdio(false)` — stops `cin`/`cout` from sharing a buffer with `scanf`/`printf`. The sync layer makes every `cin >> x` go through an unbuffered-ish path; turning it off gives cin its own buffer. Typical speedup on 1e6 integers: **3–10×**.
- `cin.tie(nullptr)` — by default `cin` is tied to `cout`, so **every read flushes cout first**. Untying removes one flush per read.

**Trap:** After `sync_with_stdio(false)` you must **not mix** `scanf`/`printf`/`getchar` with `cin`/`cout`. The two buffers are independent → interleaved/lost output. Pick one family.

**Trap:** Never use this in **interactive** problems (see `23_INTERACTIVE_AND_IO.md`) — you need the flushing.

**Memory trick:** *sync = "stop talking to C". tie = "stop flushing before every read".*

---

### `'\n'` vs `endl`

**Remember:** `endl` = `'\n'` + **flush**. The flush is the expensive part.

```cpp
cout << x << '\n';     // fast
cout << x << endl;     // flushes every line — can be 10x slower on 1e6 lines
```

**Use `endl` when:** interactive problems, or debugging a program that might crash (so output isn't lost).

**Trap:** `"\n"` (string literal) is marginally slower than `'\n'` (char). Irrelevant, but `'\n'` is a free habit.

---

### When I/O actually matters

| Tokens | Verdict |
|---|---|
| ≤ 1e5 | Doesn't matter |
| 1e5 – 1e6 | Fast I/O recommended |
| ≥ 1e6 | Fast I/O **required**; consider a custom reader |

At ~1e7 tokens even `cin` with sync off can be a bottleneck → hand-rolled `getchar_unlocked` reader. Rare on CF.

---

## Boilerplate

```cpp
#include <bits/stdc++.h>
using namespace std;

using ll  = long long;
using ull = unsigned long long;
using pii = pair<int,int>;
using pll = pair<ll,ll>;
using vi  = vector<int>;
using vll = vector<ll>;

const int  INF  = 1e9 + 7;          // fits int, INF+INF still fits int? NO — see trap
const int  IINF = 0x3f3f3f3f;       // 1061109567 ≈ 1.06e9, IINF+IINF fits in int
const ll   LINF = 4e18;             // near LLONG_MAX (9.22e18)
const ll   LINF2 = 0x3f3f3f3f3f3f3f3fLL; // ≈ 4.5e18, doubling-safe
const int  MOD  = 1e9 + 7;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int T; cin >> T;
    while (T--) solve();
}
```

### Why `0x3f3f3f3f` instead of `1e9`

**Remember:** `0x3f3f3f3f` is "large, but `INF + INF` doesn't overflow", **and** `memset(a, 0x3f, sizeof a)` fills an int array with exactly it.

```cpp
int dist[N];
memset(dist, 0x3f, sizeof dist);   // every element = 0x3f3f3f3f
```

**Trap:** `memset` writes **bytes**. `memset(a, 1, ...)` gives `0x01010101 = 16843009`, not 1. Only `0`, `-1` (`0xFF…`), and byte-repeating patterns like `0x3f` are safe.

---

### `#include <bits/stdc++.h>`

GCC-only. Pulls the whole standard library → compile time grows (~1s), which matters **only** if you compile locally in a tight loop. Zero runtime cost. Not portable to MSVC/clang-without-libstdc++ — irrelevant for CF, relevant if you paste code into an interview editor.

---

### `#define int long long`

**Remember:** Makes every `int` 64-bit. Requires `int32_t main()`.

```cpp
#define int long long
int32_t main() { ... }     // 'main' must return int, and int is now ll
```

**Why people use it:** kills an entire class of overflow WAs with zero thought.

**Why it's dangerous:**
1. **Memory doubles.** `int dp[5000][5000]` goes from 100 MB to 200 MB → MLE.
2. **Speed drops** ~1.3–2× on multiply/divide-heavy or cache-bound code.
3. `printf("%d", x)` silently breaks (now needs `%lld`).
4. Breaks anything expecting 32-bit: `__builtin_popcount` on a now-64-bit value counts only the low 32 bits unless you switch to `popcountll`.
5. Breaks `vector<int>` interop with library code expecting 32-bit.

**Don't use it when:** DP tables, big arrays, `bitset`-adjacent code, or any problem where n ≥ 1e7.

**Verdict:** fine for ad-hoc Div2 A/B/C. Turn it off for anything memory-heavy. Better long-term habit: use `ll` deliberately and learn `1LL *`.

---

### Macros worth having / worth avoiding

```cpp
// Worth it
#define all(x)  (x).begin(), (x).end()
#define rall(x) (x).rbegin(), (x).rend()

// Dangerous
#define sz(x) x.size()          // unsigned! sz(v) - 1 with empty v = huge number
```

**Fix:** `#define sz(x) (int)(x).size()`

**Trap:** unparenthesized macro args. `#define sq(x) x*x` → `sq(a+b)` = `a+b*a+b`. Always parenthesize: `#define sq(x) ((x)*(x))`.

**Trap:** `v.size() - 1` when `v` is empty → `size_t` wraps to 18446744073709551615. Loops like `for (int i = 0; i < v.size() - 1; i++)` become infinite / out-of-bounds. **Cast to int.**

---

### Local-only debug

```cpp
#ifdef LOCAL
  #define dbg(...) cerr << "[" << #__VA_ARGS__ << "] = ", _dbg(__VA_ARGS__)
#else
  #define dbg(...) 42
#endif
```
Compile locally with `-DLOCAL`. `cerr` is unbuffered and is **not** judged on CF, but it still costs time — never leave a `cerr` inside a hot loop on submission.

---

## Multi-testcase hygiene

### Clear only what you touched

**Remember:** `memset` on a 1e6 global array, 1e5 times, is 1e11 byte-writes → TLE even though "clearing is O(1) conceptually".

```cpp
// BAD across T = 1e5 test cases
memset(adj, 0, sizeof adj);

// GOOD — clear only nodes 1..n of this test
for (int i = 1; i <= n; i++) adj[i].clear();
```

**Rule:** total clearing work must be bounded by *sum of n*, not by *T × N_max*.

**Trap:** `vector<vector<int>> adj(n+1);` constructed fresh each test is usually fine and safest — allocation is O(n), not O(N_max).

---

### Global vs local storage

| | Global array | Local `vector` |
|---|---|---|
| Memory region | static (BSS) | heap |
| Zero-initialized | yes, free | must pay `O(n)` |
| Size | compile-time constant | runtime |
| Stack risk | none | none (data is on heap) |
| Per-test cost | must clear manually | freed automatically |

**Trap:** a big **local non-vector array** inside `main` or a function lives on the **stack**. `int a[1'000'000];` inside a function = 4 MB of stack. CF sets stack = memory limit (usually 256 MB) so it often works, but it's fragile. Prefer global or `vector`.

---

## C++17 features you should actually use

```cpp
// structured bindings
for (auto [u, w] : adj[v]) ...
auto [it, inserted] = s.insert(x);

// if with initializer
if (auto it = mp.find(x); it != mp.end()) ...

// CTAD
pair p{1, 2};              // pair<int,int>
vector v{1, 2, 3};

// <numeric>
int g = gcd(a, b);          // C++17, no __gcd needed
int l = lcm(a, b);          // overflows if a*b overflows — see 19/03

// emplace_back returns a reference (C++17)
auto& e = v.emplace_back();

// std::clamp, std::size
x = clamp(x, lo, hi);
```

**Not available in C++17:** `map::contains`, `starts_with`, `ranges::`, `<bit>` (`popcount`, `bit_width`), `<format>`. Those are C++20 — CF has C++20 compilers, but if you submit under C++17 they won't compile.

**`__int128` caveat:** only on **64-bit** compilers. `GNU G++17 7.3.0` on CF is **32-bit** — no `__int128`, and `long double` behaves differently. Use `GNU G++17 9.2.0 (64 bit)` or newer.

---

## Copying traps (this is the whole point of the handbook)

### Range-for copies by default

```cpp
for (auto x : v)          // COPIES each element
for (auto& x : v)         // reference, can modify
for (const auto& x : v)   // reference, read-only  ← default habit
```

Irrelevant for `int`. **Very** relevant for `string`, `vector<int>`, `pair<string,int>` — a loop over `vector<string>` of 1e5 strings with `auto x` can be a real TLE.

**Memory trick:** *`const auto&` unless you need to write.*

---

### Assignment copies

```cpp
vector<int> a = b;              // O(n) deep copy
const vector<int>& a = b;       // O(1) alias
vector<int> a = move(b);        // O(1), b is now empty
```

**Trap:** passing `vector<int> v` by value to a function inside a loop = O(n) per call. Use `const vector<int>&`. Use plain `vector<int>&` only if you intend to mutate the caller's data.

---

### `reserve` vs `resize`

```cpp
vector<int> v;
v.reserve(n);      // capacity = n, size = 0  → v[0] is UB
v.resize(n);       // size = n, elements value-initialized to 0
```

**Remember:** *reserve buys land, resize builds houses.*

```cpp
vector<int> v(n);        // size n, zeros
v.reserve(2*n);          // now capacity ≥ 2n, size still n, no reallocation for n more push_backs
```

**Use `reserve` when:** you know the final size and will `push_back` — avoids ~log₂(n) reallocations and up to 2n element moves.

---

### `push_back` vs `emplace_back`

```cpp
v.push_back({a, b});        // constructs pair, then moves
v.emplace_back(a, b);       // constructs in place
```
For trivial types: identical. For `pair<string,int>` / structs: `emplace_back` saves a move. Not a TLE-level difference in practice, but free.

---

## Compiler pragmas

```cpp
#pragma GCC optimize("O3","unroll-loops")
#pragma GCC target("avx2")
```

- CF already compiles with `-O2`. `O3` sometimes helps, sometimes hurts.
- `target("avx2")` enables wider SIMD for auto-vectorizable loops (simple array arithmetic). Can give 2–4× on the right loop, **nothing** on pointer-chasing or branchy code.
- Must appear **before** any function you want affected, typically at the very top.

**Trap:** `avx2` on a judge without AVX2 → illegal instruction (RE). Codeforces machines support it; not all judges do.

**Verdict:** never the fix for an algorithmically wrong solution. Use it to rescue a borderline correct one.

---

## Printing

```cpp
printf("%d %lld %llu %.10f %.10Lf\n", i, ll_val, ull_val, dbl, ldbl);
cout << fixed << setprecision(10) << x << '\n';
```

**Trap:** `%d` with a `long long` is UB and prints garbage — a classic after `#define int long long`.

**Trap:** `cout << setprecision(10)` without `fixed` gives 10 **significant** digits; with `fixed` it gives 10 digits **after the decimal point**. Problems almost always want `fixed`.

---

## Pre-contest 20-second ritual

```
[ ] fast I/O line present?
[ ] '\n' not endl?
[ ] MOD / INF constants defined?
[ ] multi-test loop reads T?
[ ] per-test state cleared?
```