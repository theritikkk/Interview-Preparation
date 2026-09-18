# 02 — STL Cheat Sheet

---

## Master container table

| Container | Access | Insert | Erase | Contiguous | Mem / elem (x86-64) | Notes |
|---|---|---|---|---|---|---|
| `int[]` | O(1) | — | — | yes | 4 B | fastest, fixed size |
| `array<int,N>` | O(1) | — | — | yes | 4 B | size in the type |
| `vector<int>` | O(1) | amort O(1) back | O(1) back, O(n) middle | yes | 4 B + slack | default choice |
| `deque<int>` | O(1) | O(1) both ends | O(1) both ends | **no** (chunks) | 4 B + chunk overhead | 0-1 BFS, sliding window |
| `list<int>` | O(n) | O(1) w/ iterator | O(1) w/ iterator | no | 4 + 16 B | almost never |
| `stack` / `queue` | top/front O(1) | O(1) | O(1) | adapter | — | wraps deque |
| `priority_queue` | top O(1) | O(log n) | O(log n) | wraps vector | 4 B | **max-heap by default** |
| `set` / `map` | O(log n) | O(log n) | O(log n) | no | ~40–48 B/node | ordered, `lower_bound` |
| `multiset` / `multimap` | O(log n) | O(log n) | O(log n) | no | ~40–48 B/node | duplicates allowed |
| `unordered_set/map` | avg O(1), worst O(n) | avg O(1) | avg O(1) | no | ~32–56 B/node | **hackable on CF** |
| `bitset<N>` | O(1) | — | — | yes | N/8 bytes | bulk ops in N/64 |

**Rule of thumb:** `map` node overhead is ~10× the payload for `map<int,int>`. A `map<int,int>` with 1e6 entries is ~48 MB. An `int[1e6]` is 4 MB.

---

## `vector`

```cpp
vector<int> v(n);            // n zeros
vector<int> v(n, -1);        // n copies of -1
vector<vector<int>> g(n, vector<int>(m, 0));   // n x m
vector<int> v = {1,2,3};

v.push_back(x); v.pop_back();
v.front(); v.back();          // references — UB if empty
v.size(); v.empty(); v.clear();
v.resize(k); v.reserve(k);
v.assign(n, 0);               // resize + fill in one call
v.insert(v.begin()+i, x);     // O(n)
v.erase(v.begin()+i);         // O(n)
v.erase(v.begin()+l, v.begin()+r);  // erases [l, r)
```

**Iterator invalidation:** any reallocation (`push_back` past capacity, `resize`, `reserve`) invalidates **all** iterators, pointers, and references. `erase`/`insert` invalidate everything from that position onward.

**Trap:**
```cpp
for (auto it = v.begin(); it != v.end(); ) {
    if (bad(*it)) it = v.erase(it);   // erase returns next valid iterator
    else ++it;
}
```
Writing `v.erase(it); ++it;` is UB.

**Better (O(n) instead of O(n²)):**
```cpp
v.erase(remove_if(v.begin(), v.end(), bad), v.end());
```

**Trap:** `vector<bool>` is a **bit-packed proxy**, not a real container. `auto x = vb[i]` gives a proxy object, `&vb[i]` doesn't compile, and it's slower per-element than `vector<char>`. Use `vector<char>` for flags unless you specifically want the 8× memory saving.

---

## `deque`

```cpp
deque<int> dq;
dq.push_front(x); dq.push_back(x);
dq.pop_front();   dq.pop_back();
dq[i];    // O(1) but slower than vector — indirection through a block map
```
**Use when:** 0-1 BFS, sliding-window min/max, anything needing both ends.
**Don't use when:** you only need a stack/queue of ints and speed matters — `vector` with two indices beats it.
**Invalidation:** insert/erase at the ends invalidates **iterators** but **not references** to existing elements. Insert in the middle invalidates everything.

---

## `priority_queue`

```cpp
priority_queue<int> pq;                                   // MAX heap
priority_queue<int, vector<int>, greater<int>> minpq;     // MIN heap
priority_queue<pll, vector<pll>, greater<pll>> dij;       // Dijkstra
pq.push(x); pq.top(); pq.pop(); pq.size(); pq.empty();
```

**Remember:** *`greater` → smaller elements get greater priority → min-heap.*

Custom comparator:
```cpp
struct Cmp {
    bool operator()(const Node& a, const Node& b) const {
        return a.cost > b.cost;      // ">" gives MIN-heap
    }
};
priority_queue<Node, vector<Node>, Cmp> pq;
```

**Trap — comparator sign is inverted vs `sort`.** In `sort`, `cmp(a,b)==true` means *a comes first*. In `priority_queue`, `cmp(a,b)==true` means *a has LOWER priority* (comes out later). Same comparator → opposite order.

**Trap:** `pq.top()` on an empty heap is UB (usually a crash or garbage). Always `while (!pq.empty())`.

**Complexities:** `push` O(log n), `pop` O(log n), `top` O(1). Building from a vector: `priority_queue<int> pq(v.begin(), v.end())` is O(n), not O(n log n).

**No `erase`, no iteration.** Need deletion → use `set`, or use the **lazy deletion** pattern (push duplicates, skip stale entries on pop).

**Applications:** Dijkstra, top-K, k-way merge, scheduling, running median (two heaps).

---

## `set` / `multiset` / `map`

```cpp
set<int> s;
s.insert(x); s.erase(x); s.count(x); s.find(x);
*s.begin();                 // min
*s.rbegin();                // max
s.lower_bound(x);           // O(log n) — member version
auto it = s.upper_bound(x); // NOT std::upper_bound(s.begin(), ...)

map<string,int> mp;
mp[key]++;                  // inserts key with 0 if absent!
mp.at(key);                 // throws if absent
mp.count(key); mp.find(key);
for (auto& [k, v] : mp) ...
```

### The two `erase` overloads (the #1 multiset bug)

```cpp
multiset<int> ms = {5,5,5};
ms.erase(5);                    // erases ALL THREE
ms.erase(ms.find(5));           // erases ONE
```
**Memory trick:** *erase(value) = erase every match. erase(iterator) = erase exactly one.*

**Trap:** `ms.erase(ms.find(x))` when `x` isn't present → `find` returns `end()`, erasing `end()` is UB. Guard it.

### `map[x]` silently inserts

```cpp
if (mp[x] > 0) ...      // just created mp[x] = 0 if it didn't exist
if (mp.count(x)) ...    // safe
auto it = mp.find(x);   // safe, and reuses the lookup
if (it != mp.end()) use(it->second);
```
This matters for correctness (`mp.size()` grows) and for speed (extra node allocations). `mp.contains(x)` is **C++20 only**.

### Member `lower_bound` vs `std::lower_bound`

```cpp
set<int> s;
auto it = s.lower_bound(x);                      // O(log n)   ✔
auto it = lower_bound(s.begin(), s.end(), x);    // O(n)       ✘
```
`std::lower_bound` needs random-access iterators to be O(log n). On a tree it still does log n **comparisons** but O(n) **iterator steps**. Always use the member version for `set`/`map`.

**Invalidation:** `set`/`map` erase invalidates only iterators to the erased element. Insert invalidates nothing. This is why they're safe for "iterate and modify" patterns that would break a vector.

---

## `unordered_map` / `unordered_set`

```cpp
unordered_map<int,int> mp;
mp.reserve(1 << 20);          // avoid rehashing
mp.max_load_factor(0.25);     // fewer collisions, more memory
```

**The CF problem:** GCC's `unordered_map<int,...>` uses the identity hash then mods by a prime bucket count. Anyone can generate 1e5 keys that all land in one bucket → O(n²) → TLE by hacking. This happens in real contests.

### Hardened hash (memorize the shape, keep the template)

```cpp
struct custom_hash {
    static uint64_t splitmix64(uint64_t x) {
        x += 0x9e3779b97f4a7c15ULL;
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
        return x ^ (x >> 31);
    }
    size_t operator()(uint64_t x) const {
        static const uint64_t FIXED =
            chrono::steady_clock::now().time_since_epoch().count();
        return splitmix64(x + FIXED);
    }
};
unordered_map<long long, int, custom_hash> safe_map;
```

**Faster alternative (GNU only, great on CF):**
```cpp
#include <ext/pb_ds/assoc_container.hpp>
using namespace __gnu_pbds;
gp_hash_table<long long, int, custom_hash> h;   // ~2-3x faster than unordered_map
```

**When `map` is the better call:** you need ordering, `lower_bound`, or predictable worst case. O(log n) with n = 1e6 is 20 comparisons — a hardened hash isn't always worth the code.

**When neither:** if keys are small integers, use a **plain array**. `int cnt[1'000'001]` destroys any hash map on speed. See `12_CPP_PERFORMANCE.md`.

---

## `bitset`

```cpp
bitset<1000> b;
b.set(); b.reset(); b.flip();
b.set(i); b.reset(i); b[i];
b.count();                 // popcount, O(N/64)
b |= c; b &= c; b ^= c; b <<= k;
b._Find_first();           // index of lowest set bit, N if none (GNU)
b._Find_next(i);           // next set bit after i (GNU)
b.to_ullong();
```
Size must be a **compile-time constant**. Memory = N/8 bytes → `bitset<100000000>` = 12.5 MB.

**The superpower:** turns O(n²) into O(n²/64). Reachability closures, subset-sum feasibility, string matching:
```cpp
// subset sum: can we hit value s with items a[]?
bitset<100001> dp; dp[0] = 1;
for (int x : a) dp |= dp << x;     // O(n * S / 64)
```

---

## Algorithms

```cpp
sort(v.begin(), v.end());                       // O(n log n), introsort
sort(v.rbegin(), v.rend());                     // descending
sort(v.begin(), v.end(), greater<int>());       // descending
stable_sort(...);                               // O(n log²n), preserves equal order
nth_element(v.begin(), v.begin()+k, v.end());   // O(n) avg, k-th in place
partial_sort(v.begin(), v.begin()+k, v.end());  // smallest k sorted

reverse(all(v));
rotate(v.begin(), v.begin()+k, v.end());        // left rotate by k
fill(all(v), x);
iota(all(v), 0);                                // 0,1,2,3,...

*min_element(all(v)); *max_element(all(v));     // O(n), returns ITERATOR
int mn = *min_element(all(v));
int idx = min_element(all(v)) - v.begin();

accumulate(all(v), 0LL);                        // note the 0LL
count(all(v), x);
find(all(v), x);                                // returns end() if absent
v.erase(unique(all(v)), v.end());
next_permutation(all(v));
```

### `accumulate` overflow

**Remember:** the **third argument's type** is the accumulator type.
```cpp
accumulate(all(v), 0);     // int accumulator → overflows at 2^31
accumulate(all(v), 0LL);   // correct
```

### `min_element` returns an iterator

```cpp
int mn = min_element(all(v));    // COMPILE ERROR / nonsense
int mn = *min_element(all(v));   // correct
```
Also: `min({a,b,c})` with braces works for 3+ arguments.

### What `unique` actually does

**Remember:** `unique` **does not delete anything**. It moves distinct elements to the front, leaves garbage after, and returns the new logical end. It only collapses **adjacent** duplicates.

```cpp
sort(all(v));
v.erase(unique(all(v)), v.end());   // the real dedup idiom
```
Without the `sort`, `{1,2,1}` stays `{1,2,1}`. Without the `erase`, `v.size()` is unchanged.

### `next_permutation`

```cpp
sort(all(v));
do { work(v); } while (next_permutation(all(v)));
```
Returns `false` and wraps to the sorted order when it hits the last permutation. **Must start sorted ascending** to enumerate all n! permutations. Handles duplicates correctly (generates distinct permutations only).

---

## `lower_bound` / `upper_bound` — dedicated section

### The semantics

```
lower_bound(x) = first element >= x
upper_bound(x) = first element >  x
```

**Memory trick:**
- **LOWER** → "the first thing that is **not lower** than x" → `>= x`
- **UPPER** → "the first thing strictly **above** x" → `> x`

Both require the range to be **sorted** (or at least partitioned by the predicate). On unsorted data they return garbage silently — no error.

### Index conversion

```cpp
int idx = lower_bound(all(v), x) - v.begin();
// idx == v.size()  means: no element >= x  (x is bigger than everything)
```

### Counting occurrences

```cpp
int cnt = upper_bound(all(v), x) - lower_bound(all(v), x);
// or
auto [lo, hi] = equal_range(all(v), x);
int cnt = hi - lo;
```

### `binary_search`

```cpp
bool found = binary_search(all(v), x);   // returns bool only, no position
```

### Predecessor / successor, safely

```cpp
// smallest element >= x
auto it = lower_bound(all(v), x);
if (it != v.end()) succ = *it;

// largest element <= x
auto it = upper_bound(all(v), x);
if (it != v.begin()) pred = *prev(it);   // step back from first element > x
```
**Trap:** `*prev(v.begin())` is UB. Always check `it != v.begin()` before stepping back.

### On `set`

```cpp
auto it = s.lower_bound(x);
if (it != s.begin()) { --it; /* largest element < x */ }
```
For "largest element **≤** x" on a set: `auto it = s.upper_bound(x); if (it != s.begin()) --it;`

### Descending arrays

`lower_bound` on a descending array needs an explicit comparator:
```cpp
lower_bound(all(v), x, greater<int>());   // first element <= x
```
Getting this wrong is a silent WA. Easier: sort ascending and flip your logic.

---

## Sorting tricks

```cpp
// pairs: sorts by .first, ties broken by .second — both ascending. Free.
sort(all(vp));

// by second ascending
sort(all(vp), [](const pii& a, const pii& b){ return a.second < b.second; });

// first descending, second ascending
sort(all(vp), [](const pii& a, const pii& b){
    if (a.first != b.first) return a.first > b.first;
    return a.second < b.second;
});

// by absolute value
sort(all(v), [](int a, int b){ return abs(a) < abs(b); });

// sort indices by value (very common)
vector<int> idx(n); iota(all(idx), 0);
sort(all(idx), [&](int i, int j){ return a[i] < a[j]; });

// trick: sort descending by value without a comparator
for (int& x : v) x = -x;  sort(all(v));  for (int& x : v) x = -x;
```

### Strict weak ordering — why `<=` crashes

**Remember:** a comparator must return **false** when the two arguments are equivalent.

```cpp
sort(all(v), [](int a, int b){ return a <= b; });   // ✘ UB — can segfault
sort(all(v), [](int a, int b){ return a <  b; });   // ✔
```
`cmp(a,a) == true` tells `std::sort` that `a` is strictly before itself. Introsort's insertion-sort phase uses this to skip a bounds check → it walks off the end of the array. This is a **real crash**, not a theoretical one, and it often only fires on large inputs.

**Rule:** every comparator ends with `return a.x < b.x;` — never `<=`, never `>=`.

**Same rule applies** to comparators for `set`, `map`, and `priority_queue`. A `set` with a `<=` comparator will lose elements.

### `stable_sort`

Use when equal elements must keep input order (or you sorted by a second key first and are now sorting by the primary key). Slower and allocates, but O(n log n) guaranteed and no comparator surprises.

---

## pb_ds ordered set (GNU only — fine on CF)

```cpp
#include <ext/pb_ds/assoc_container.hpp>
#include <ext/pb_ds/tree_policy.hpp>
using namespace __gnu_pbds;

template<class T>
using ordered_set = tree<T, null_type, less<T>, rb_tree_tag,
                         tree_order_statistics_node_update>;

ordered_set<int> os;
os.insert(5);
os.order_of_key(5);     // number of elements strictly < 5
*os.find_by_order(0);   // 0-indexed k-th smallest
```
O(log n) each. For **multiset** behaviour use `ordered_set<pair<int,int>>` with `(value, unique_id)`, or `less_equal<T>` (but then `find`/`erase` break).

**Use when:** you need "k-th smallest" or "rank of x" with insertions interleaved — inversion counting, dynamic order statistics.

---

## `pair` / `tuple`

```cpp
pair<int,int> p = {a, b};
auto [x, y] = p;
tuple<int,int,int> t = {a,b,c};
auto [x,y,z] = t;
get<0>(t);
tie(x, y) = make_pair(1, 2);
```
Comparison is lexicographic for both. `tuple` with 3+ fields sorts by all fields in order — free multi-key sorting.