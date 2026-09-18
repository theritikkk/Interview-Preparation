# 05 — Bit Manipulation

---

## The basics

```cpp
x & 1              // lowest bit (parity)
x >> k             // divide by 2^k (floor for non-negative)
x << k             // multiply by 2^k
(x >> i) & 1       // read bit i
x |=  (1 << i)     // set bit i
x &= ~(1 << i)     // clear bit i
x ^=  (1 << i)     // toggle bit i
```

**Trap:** `1 << i` is an `int`. For `i >= 31` this is UB.
```cpp
1LL << 40          // ✔
1 << 40            // ✘ undefined behaviour
```
**Rule: the moment `i` can reach 31, write `1LL`.**

**Trap:** `x >> k` on a **negative** `x` is implementation-defined (arithmetic shift in practice — sign bit propagates), so it is **not** division by 2^k for negatives: `-7 >> 1 == -4`, but `-7 / 2 == -3`.

**Trap:** shifting by `>= width` is UB. `1LL << 64` is not 0.

---

## The two you must know cold

### `x & (x - 1)` — kill the lowest set bit

```cpp
x = 5  (101)  →  x & (x-1) = 4 (100)
x = 12 (1100) →  x & (x-1) = 8 (1000)
```
**Memory:** *"minus one flips the lowest 1 and everything below it; AND kills it."*

Uses: popcount by loop, power-of-two check, iterating set bits.

### `x & -x` — keep only the lowest set bit

```cpp
x = 12 (1100) →  x & -x = 4 (100)
```
Works because `-x = ~x + 1` in two's complement.
**Memory:** *`&(x-1)` **K**ills the lowest 1; `&-x` **K**eeps it.* (Minus sign = keep.)

Uses: Fenwick tree indexing, extracting bits one at a time.

```cpp
// iterate over set bits
for (int t = mask; t; t &= t - 1) {
    int bit = __builtin_ctz(t & -t);   // or just t & -t for the value
}
```

---

## GCC builtins

```cpp
__builtin_popcount(x)     // number of 1 bits — unsigned int
__builtin_popcountll(x)   // long long version
__builtin_clz(x)          // count leading zeros  (31 - floor(log2 x) for int)
__builtin_clzll(x)        // 63 - floor(log2 x)
__builtin_ctz(x)          // count trailing zeros = index of lowest set bit
__builtin_ctzll(x)
__builtin_parity(x)       // popcount % 2
```

**Trap: `clz` and `ctz` are UNDEFINED for x = 0.** `popcount(0) = 0` is fine, the other two are not. Always guard.

**Trap:** the non-`ll` versions take `unsigned int`. Passing a `long long` silently truncates to the low 32 bits. After `#define int long long`, `__builtin_popcount(x)` is wrong.

### Fast integer log2 / floor / ceil to a power of two

```cpp
int lg  = 31 - __builtin_clz(x);        // floor(log2(x)), x > 0
int lgl = 63 - __builtin_clzll(x);
ll  hi  = 1LL << lg;                     // largest power of 2 <= x
ll  up  = (x & (x-1)) ? 1LL << (lg+1) : x;   // smallest power of 2 >= x
```
**Never use `log2(x)`** for integers — it's a double, and `log2(8)` can come back as `2.9999999996`.

---

## Power of two

```cpp
bool isPow2(ll x) { return x > 0 && (x & (x - 1)) == 0; }
```
**Trap:** the `x > 0` guard. Without it, `x = 0` returns true, and negatives misbehave.

---

## Submask enumeration

```cpp
for (int s = mask; s; s = (s - 1) & mask) {
    // s runs over all non-empty submasks of mask, descending
}
// include the empty submask:
int s = mask;
while (true) { /* use s */ if (!s) break; s = (s - 1) & mask; }
```

**Why it works:** `s - 1` borrows through the low zero-run of `s`, producing the "next smaller" pattern; `& mask` discards the bits outside `mask`, landing exactly on the next submask in decreasing order.

**Total cost over all masks of an n-bit universe: 3^n**, not 4^n.
Each bit is independently in {not in mask, in mask but not in s, in both} → 3 choices.

**Use when:** subset-sum DP over partitions, set-cover, SOS-adjacent problems. n ≤ 20 → 3^20 ≈ 3.5e9 is too slow; n ≤ 16 → 43M is fine.

### Supermask enumeration

```cpp
for (int s = mask; s < (1 << n); s = (s + 1) | mask) { ... }
```

---

## Iterating all subsets

```cpp
for (int mask = 0; mask < (1 << n); mask++) {
    for (int i = 0; i < n; i++)
        if (mask >> i & 1) { /* element i is in the set */ }
}
```
Bitmask DP convention: iterate masks **ascending** and every proper submask is processed first (since a submask is numerically smaller). This is why `for (mask = 0 .. )` is the natural DP order.

---

## XOR properties

```
x ^ x = 0
x ^ 0 = x
x ^ y ^ x = y          (XOR is its own inverse)
XOR is commutative and associative
a + b = (a ^ b) + 2*(a & b)
a + b = (a | b) + (a & b)
a ^ b <= a + b
```

**Prefix XOR:** `pre[i] = a[0]^…^a[i-1]`, then XOR of `a[l..r] = pre[r+1] ^ pre[l]`. Exactly like prefix sums — subtraction is replaced by XOR (which is its own inverse).

**Classic uses:**
- One number appears once, others twice → XOR everything.
- Missing number in 1..n → XOR all values with XOR of 1..n.
- Nim game → XOR of pile sizes (see `21_GAME_THEORY.md`).
- Count subarrays with XOR = k → prefix XOR + hash map.

**XOR of 1..n has a period-4 pattern:**
```
n % 4 == 0 → n
n % 4 == 1 → 1
n % 4 == 2 → n + 1
n % 4 == 3 → 0
```

---

## Bit-level identities worth remembering

```
x | y  =  x ^ y ^ (x & y)
x & ~y  =  x - (x & y)        // remove y's bits from x
~x = -x - 1                   // two's complement
x ^ (1 << i)                  // toggle
(x >> i) & 1                  // test
lowbit(x) = x & -x
highest bit index = 31 - __builtin_clz(x)
number of bits to represent x = 32 - __builtin_clz(x)
```

**Trailing-zero trick:** `x & (x-1) == 0` → power of two; `__builtin_ctz(x)` → how many times x is divisible by 2.

---

## `bitset` as a constant-factor weapon

```cpp
bitset<20005> reach[20005];       // 20000 * 2500 B = 50 MB — check your ML!
reach[u] |= reach[v];             // O(N/64) instead of O(N)
```
Turns O(n·m) into O(n·m/64). Common uses:
- transitive closure / reachability
- subset-sum feasibility: `dp |= dp << x`
- bipartite matching adjacency
- "for each i, which j satisfy …" tables

```cpp
bitset<1000> b;
b._Find_first();      // lowest set bit index (N if none) — GNU extension
b._Find_next(i);      // next set bit after index i
b.count();            // popcount, O(N/64)
```

**Constraint:** size must be a compile-time constant. Use `vector<uint64_t>` and hand-rolled ops if it must be dynamic.

---

## Practical thresholds

| n | 2^n | Verdict |
|---|---|---|
| 20 | 1.0e6 | fine |
| 22 | 4.2e6 | fine |
| 24 | 1.7e7 | fine |
| 25 | 3.4e7 | borderline with heavy inner work |
| 30 | 1.1e9 | too slow |

With an inner loop of n: `n·2^n` at n = 20 is 2e7 — fine. At n = 24 it's 4e8 — too slow.
Submask enumeration 3^n: n = 16 → 43M fine, n = 18 → 387M borderline, n = 20 → 3.5e9 no.

**Recognition rule:** *`n ≤ 20` in the constraints is a bitmask signal.*

---

## Memory hooks

- *`x & (x-1)` **K**ills the lowest 1*
- *`x & -x` **K**eeps the lowest 1*
- *`clz`/`ctz` at 0 is undefined*
- *Use `1LL` the moment the shift can reach 31*
- *Submask loop total = 3^n*
- *XOR is its own undo → prefix XOR works like prefix sum*