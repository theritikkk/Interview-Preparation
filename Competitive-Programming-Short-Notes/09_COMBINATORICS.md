# 09 — Combinatorics

Templates for `fact[]` / `invFact[]` / `nCr` live in `04_MODULAR_ARITHMETIC.md`.

---

## The core three

```
n!          arrangements of n distinct objects
nPr = n!/(n-r)!         ordered selections
nCr = n!/(r!(n-r)!)     unordered selections
```

**Identities to know cold:**

```
C(n, r) = C(n, n-r)                       symmetry
C(n, r) = C(n-1, r-1) + C(n-1, r)         Pascal
C(n, 0) = C(n, n) = 1
Σ_{r=0}^{n} C(n, r) = 2^n                 all subsets
Σ_{r=0}^{n} (-1)^r C(n, r) = 0            for n ≥ 1
Σ_{r} C(n, r)² = C(2n, n)                 Vandermonde special case
Σ_{i=r}^{n} C(i, r) = C(n+1, r+1)         hockey stick
r·C(n, r) = n·C(n-1, r-1)                 absorption
C(n, r) = 0 when r < 0 or r > n           GUARD THIS
```

**Hockey stick** is the one that quietly solves a lot of problems: a sum over a range of binomials collapses into one binomial.

---

## Growth — when does nCr overflow?

| | Value |
|---|---|
| 20! | 2.4e18 — the **largest factorial fitting in `long long`** |
| 21! | 5.1e19 — overflows |
| C(60, 30) | 1.18e17 — fits |
| C(62, 31) | 4.7e17 — fits |
| C(66, 33) | 7.2e18 — barely fits |
| C(68, 34) | 2.8e19 — overflows |

**Rule:** `n!` fits in `long long` only up to n = 20. If the problem says "the answer fits in 64 bits" and involves factorials, n is small.

**Computing nCr without modulo, overflow-safe:**
```cpp
ll nCr_exact(int n, int r) {           // assumes the result fits
    if (r < 0 || r > n) return 0;
    r = min(r, n - r);
    ll res = 1;
    for (int i = 1; i <= r; i++) {
        res = res / i * (n - r + i) + res % i * (n - r + i) / i;  // exact, avoids overflow
    }
    return res;
}
// simpler and usually enough:
ll nCr_simple(int n, int r) {
    r = min(r, n - r);
    ll res = 1;
    for (int i = 0; i < r; i++) { res *= (n - i); res /= (i + 1); }
    return res;   // the intermediate is always divisible — safe
}
```
`res *= (n-i); res /= (i+1);` is exact because after multiplying by `i+1` consecutive integers, the product is divisible by `(i+1)!`. But the intermediate can still overflow — cap `n` around 60.

---

## Stars and bars

**Problem:** distribute `n` identical items into `k` distinct boxes.

| Constraint | Formula |
|---|---|
| boxes may be empty | `C(n + k - 1, k - 1)` |
| every box ≥ 1 | `C(n - 1, k - 1)` |
| box i ≥ `lᵢ` | subtract `Σ lᵢ` from n, then use the "may be empty" formula |
| box i ≤ `uᵢ` | inclusion–exclusion over which boxes are over-full |

**Memory hook:** *n stars, k−1 bars, arrange them all → `C(n+k−1, k−1)`. If every box needs one, hand one out first → `C(n−1, k−1)`.*

Same formula counts: non-negative integer solutions to `x₁ + … + x_k = n`, monotone non-decreasing sequences, multisets of size n from k types.

**Trap:** if `k` can be larger than `n`, `C(n-1, k-1)` is 0 (correct — you can't give everyone one). Make sure your `nCr` guard handles `r > n`.
**Trap:** your `fact[]` must be precomputed to at least `n + k`, not `n`.

---

## Inclusion–exclusion

```
|A ∪ B| = |A| + |B| − |A ∩ B|
|A ∪ B ∪ C| = |A|+|B|+|C| − |A∩B| − |A∩C| − |B∩C| + |A∩B∩C|
```

General form over `m` sets:
```cpp
ll ans = 0;
for (int mask = 1; mask < (1 << m); mask++) {
    ll sub = countSatisfying(mask);            // size of the intersection
    int bits = __builtin_popcount(mask);
    if (bits & 1) ans += sub; else ans -= sub;
}
```
**Sign rule:** odd number of sets → add, even → subtract.
**Complexity:** O(2^m · cost). Needs m ≤ ~20.

**Most common contest use:** "count numbers in [1, n] divisible by none of p₁..p_k" →
```cpp
for each non-empty subset: n / (product of primes), signed by popcount parity
```

**Complementary counting is inclusion–exclusion with one set:** "at least one X" = total − "no X". Always try this first.

---

## Pigeonhole

> n items in k boxes with n > k ⟹ some box holds at least ⌈n/k⌉ items.

**Recognition:** the problem guarantees a solution exists, or asks you to *find* a repeat/collision, or has a bound suspiciously equal to n+1.

**Examples:**
- Among any n+1 numbers from [1, 2n], two are coprime / one divides another.
- Among any n integers, some non-empty subset has a sum divisible by n (look at the n prefix sums mod n — if none is 0, two are equal).
- Any sequence of length `n²+1` contains a monotone subsequence of length `n+1` (Erdős–Szekeres).

---

## Catalan numbers

```
C₀ = 1,  Cₙ = C(2n, n) / (n + 1) = C(2n, n) − C(2n, n+1)
Cₙ₊₁ = Σ_{i=0}^{n} Cᵢ · C_{n−i}
1, 1, 2, 5, 14, 42, 132, 429, 1430, 4862, ...
```

```cpp
ll catalan(int n) { return nCr(2*n, n) * inv(n + 1) % MOD; }
```

**What it counts** (recognize these shapes):
- balanced bracket sequences of length 2n
- binary trees with n nodes
- monotone lattice paths from (0,0) to (n,n) never crossing the diagonal
- ways to triangulate a convex (n+2)-gon
- ways to fully parenthesize n+1 factors
- non-crossing chord matchings of 2n points on a circle

**Ballot / cycle lemma generalization:** paths from (0,0) to (a,b) staying strictly above the diagonal = `((a−b)/(a+b))·C(a+b, a)`.

---

## Derangements

Permutations with no fixed point.
```
D₀ = 1, D₁ = 0
Dₙ = (n − 1)(D_{n−1} + D_{n−2})
Dₙ = n! · Σ_{k=0}^{n} (−1)^k / k!    ≈ n!/e
1, 0, 1, 2, 9, 44, 265, ...
```
```cpp
d[0] = 1; d[1] = 0;
for (int i = 2; i <= n; i++) d[i] = (i-1) * (d[i-1] + d[i-2]) % MOD;
```
"Exactly k fixed points" = `C(n, k) · D_{n−k}`.

---

## Permutations with repetition

Multiset permutations of `n` items where item `i` appears `cᵢ` times:
```
n! / (c₁! · c₂! · … · c_k!)
```
```cpp
ll res = fact[n];
for (int c : cnt) res = res * invFact[c] % MOD;
```

Circular permutations of n distinct objects: `(n−1)!` (fix one to kill rotations). If reflections are also equivalent: `(n−1)!/2` for n ≥ 3.

---

## Counting by contribution

**The single most useful reframing in contest combinatorics.**

Instead of "sum over all objects of f(object)", ask "for each *element*, in how many objects does it contribute, and how much?"

**Example:** sum over all subarrays of their minimum.
- Enumerating subarrays: O(n²).
- Per element: element `a[i]` is the minimum in `(i − prevSmaller) × (nextSmaller − i)` subarrays → O(n) with a monotonic stack.

**Example:** sum of `|a_i − a_j|` over all pairs.
- Sort, then `a[i]` is the larger element in `i` pairs and the smaller in `n−1−i` → `Σ a[i]·(2i − n + 1)`. O(n log n).

**Example:** number of subarrays containing index i = `(i+1) · (n−i)`.

**Example:** in how many subsets does element i appear? `2^(n−1)`. So "sum over all subsets of their sum" = `2^(n−1) · Σ a[i]`.

**Trigger phrase:** "the sum over all subarrays / subsets / pairs of …". Immediately flip to per-element contribution.

---

## Burnside / Pólya (know it exists)

Number of distinct objects under a group of symmetries = average number of objects fixed by each symmetry:
```
|X/G| = (1/|G|) · Σ_{g ∈ G} |Fix(g)|
```
**Typical use:** necklaces with n beads and k colors under rotation:
```
(1/n) · Σ_{d=0}^{n-1} k^gcd(d, n)  =  (1/n) · Σ_{d | n} φ(n/d) · k^d
```
Rare in Div2. Recognize the phrase "two configurations are the same if one can be rotated/reflected into the other" and reach for a template.

---

## Common WA sources

```
[ ] nCr called with r < 0 or r > n and not guarded → array out of bounds
[ ] fact[] precomputed to n but the formula needs n + k
[ ] MOD not prime but used binpow(x, MOD-2)
[ ] forgot that the answer needs mod
[ ] double-counting: did I count ordered pairs when the problem wants unordered? (divide by 2)
[ ] the empty set / empty subsequence — included or not?
[ ] "arrangements" vs "selections" — is order relevant?
[ ] overflow in a non-modular answer (20! is the ceiling)
[ ] inclusion-exclusion sign flipped
[ ] circular arrangements: (n-1)! not n!
```

---

## Memory hooks

- *`C(n,r) = C(n,n-r)` — pick what you take or what you leave*
- *stars and bars: n stars, k−1 bars*
- *"at least one" → complement*
- *"sum over all subarrays" → per-element contribution*
- *inclusion–exclusion sign = parity of popcount*
- *20! is the last factorial in a `long long`*