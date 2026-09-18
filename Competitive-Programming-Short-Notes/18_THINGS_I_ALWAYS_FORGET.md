# 18 — Things I Always Forget

> **This file is yours, not mine.**
> Seeded with the usual suspects. After every contest, add the thing that cost you a problem.
> Format: the rule, the code, the memory hook. Nothing else.
> After 20–30 contests this becomes the single most valuable file here, because it records
> *your* failure patterns instead of somebody's syllabus.

---

## `lower_bound` / `upper_bound`

> `lower_bound` = first element **>= x**
> `upper_bound` = first element **> x**

```cpp
int idx = lower_bound(a.begin(), a.end(), x) - a.begin();
int cnt = upper_bound(all(a), x) - lower_bound(all(a), x);
```

**Memory:** LOWER = first thing that isn't *lower* than x. UPPER = first thing strictly *above* x.

---

## Multiplication before conversion

❌
```cpp
long long x = a * b;      // a, b are int → overflow happens BEFORE the assignment
```

✅
```cpp
long long x = 1LL * a * b;
```

**Memory:** promote **before** you multiply, not after.

---

## Default heap is a MAX heap

```cpp
priority_queue<int> pq;                                 // MAX
priority_queue<int, vector<int>, greater<int>> pq;      // MIN
```

**Memory:** `greater` → the *smallest* element gets the *greatest* priority.

---

## Modular subtraction

```cpp
(a - b + MOD) % MOD        // ✅
(a - b) % MOD              // ❌ can be negative in C++
```

Fully safe form when you don't know the range: `((x % MOD) + MOD) % MOD`.

---

## LCM without overflow

```cpp
ll l = a / gcd(a, b) * b;      // ✅ divide first
ll l = a * b / gcd(a, b);      // ❌ a*b can overflow
```

---

## `multiset::erase`

```cpp
ms.erase(x);              // removes ALL copies of x
ms.erase(ms.find(x));     // removes exactly ONE
```

---

## `map[key]` inserts

```cpp
if (mp[x]) ...            // just created mp[x] = 0
if (mp.count(x)) ...      // safe
```

---

## `unique` doesn't erase

```cpp
sort(all(v));
v.erase(unique(all(v)), v.end());     // both lines required
```
`unique` only collapses **adjacent** duplicates and returns the new logical end.

---

## Comparator must be strict

```cpp
return a <= b;    // ❌ UB — std::sort can segfault
return a <  b;    // ✅
```

---

## `1LL << k`

```cpp
1 << 40           // ❌ UB, shifting a 32-bit int by 40
1LL << 40         // ✅
```
Anything with `k >= 31`, write `1LL`.

---

## `__builtin_popcount` vs `popcountll`

```cpp
__builtin_popcount(x)      // unsigned int — only low 32 bits
__builtin_popcountll(x)    // long long
```
`__builtin_clz(0)` and `__builtin_ctz(0)` are **undefined**. Guard the zero case.

---

## `memset`

```cpp
memset(a, 0,   sizeof a);     // ✅ zeros
memset(a, -1,  sizeof a);     // ✅ all bytes 0xFF → int -1
memset(a, 0x3f,sizeof a);     // ✅ int 0x3f3f3f3f ≈ 1.06e9
memset(a, 1,   sizeof a);     // ❌ gives 0x01010101 = 16843009, not 1
```
`memset` writes **bytes**. Only byte-repeating patterns work.

---

## Ceil division

```cpp
(a + b - 1) / b        // ✅ for a >= 0, b > 0
```
For negatives, or when `a + b` might overflow:
```cpp
a / b + (a % b != 0)
```

---

## `accumulate` needs `0LL`

```cpp
accumulate(all(v), 0);     // ❌ int accumulator
accumulate(all(v), 0LL);   // ✅
```

---

## Prefix sum indexing

```cpp
pre[0] = 0;
for (int i = 0; i < n; i++) pre[i+1] = pre[i] + a[i];
// sum of a[l..r] inclusive, 0-indexed:
ll s = pre[r+1] - pre[l];
```
**Memory:** `pre[i]` = sum of the first `i` elements. Then the answer is always `pre[hi+1] - pre[lo]`.

---

## `v.size()` is unsigned

```cpp
for (int i = 0; i < (int)v.size() - 1; i++)   // ✅
for (int i = 0; i < v.size() - 1; i++)        // ❌ infinite loop when v is empty
```

---

## Reset between test cases

Things that leak: `visited[]`, `adj[]`, global `ans`, `cnt[]`, `dist[]`, `dp[]`, `used` sets, static counters inside functions.
Clear only indices `1..n`, never `memset` the full array per test.

---

## Sorting pairs

`sort` on `pair` sorts by `.first` ascending, ties by `.second` ascending. Free — don't write a comparator for that case.

---

## Your entries start here

<!-- Template:

## <Contest ID, Problem letter> — <what went wrong>

> The rule in one line.

```cpp
// minimal code
```

**Memory:** the hook.

-->