# 04 — Modular Arithmetic Survival Guide

---

## The four operations

```cpp
const int MOD = 1e9 + 7;

ll add(ll a, ll b) { return (a + b) % MOD; }
ll sub(ll a, ll b) { return (a - b + MOD) % MOD; }
ll mul(ll a, ll b) { return (a % MOD) * (b % MOD) % MOD; }
ll dvd(ll a, ll b) { return mul(a, binpow(b, MOD - 2, MOD)); }   // MOD prime, b % MOD != 0
```

**Remember:** add/sub/mul distribute over `%`. **Division does not.**

---

## Negative modulo in C++

**Since C++11** integer division truncates toward zero, so `%` takes the **sign of the dividend**:

```cpp
(-1) % 7     // == -1, NOT 6
(-8) % 3     // == -2
 8  % (-3)   // ==  2
```

**Normalization:**
```cpp
x = ((x % MOD) + MOD) % MOD;     // always correct
x = (x + MOD) % MOD;             // correct only if x >= -MOD
```

**When one `+MOD` is enough:** after subtracting two values already in `[0, MOD)`, the result is in `(-MOD, MOD)`, so a single `+MOD` fixes it. After a loop of arbitrary arithmetic, use the double form.

**Trap:** `a[(i - 1) % n]` with `i = 0` gives `a[-1]`. Use `a[(i - 1 + n) % n]`.

---

## Overflow inside modular multiplication

```cpp
ll a = 1e9, b = 1e9;
a * b            // 1e18 — fits in long long (max 9.22e18) ✔
(a % M) * (b % M) % M    // safe for M up to ~3e9
```

**Rule:** `x * y` is safe in `long long` iff `x, y < 3.03e9` (since 3.03e9² ≈ 9.2e18). For `MOD ≤ 2^31`, `1LL * a * b % MOD` is always safe.

**For MOD near 2^62** (rare — hashing with a 61-bit prime), you need `__int128`:
```cpp
ll mulmod(ll a, ll b, ll m) { return (__int128)a * b % m; }
```

**Trap:** `a * b % MOD` where `a`, `b` are `int` → the multiply happens in `int` first. Write `1LL * a * b % MOD`.

---

## Binary exponentiation

```cpp
ll binpow(ll a, ll b, ll m) {
    a %= m; if (a < 0) a += m;
    ll r = 1;
    while (b > 0) {
        if (b & 1) r = r * a % m;
        a = a * a % m;
        b >>= 1;
    }
    return r;
}
```
**Complexity:** O(log b).

**Trap:** `b` must be non-negative. `binpow(a, -1, m)` loops forever / returns 1.
**Trap:** if `m == 1`, everything is 0 — the function handles it correctly (`r = 1 % 1 = 0`? no — `r` starts at 1 and may return 1). Set `r = 1 % m` if `m == 1` is possible.

---

## Modular inverse

### Fermat's Little Theorem (modulus prime)

If `p` is prime and `gcd(a, p) = 1`:
```
a^(p-1) ≡ 1 (mod p)     ⟹     a^(-1) ≡ a^(p-2) (mod p)
```
```cpp
ll inv(ll a, ll p) { return binpow(a, p - 2, p); }   // O(log p)
```

**Requires:** `p` prime **and** `a % p != 0`. `inv(0)` is meaningless — 0 has no inverse.

### Extended Euclid (any modulus, gcd(a,m) = 1)

```cpp
ll ext_gcd(ll a, ll b, ll &x, ll &y) {
    if (!b) { x = 1; y = 0; return a; }
    ll x1, y1, g = ext_gcd(b, a % b, x1, y1);
    x = y1; y = x1 - (a / b) * y1;
    return g;
}
ll inv_general(ll a, ll m) {
    ll x, y, g = ext_gcd(a, m, x, y);
    if (g != 1) return -1;                 // no inverse exists
    return ((x % m) + m) % m;
}
```

### All inverses 1..n in O(n)

```cpp
vector<ll> inv(n + 1);
inv[1] = 1;
for (int i = 2; i <= n; i++)
    inv[i] = (MOD - (MOD / i) * inv[MOD % i] % MOD) % MOD;
```
Requires `MOD` prime and `MOD > n`.

---

## Why `(a / b) % MOD` is wrong

`a % MOD` destroys the information needed to divide. Example with MOD = 7:

```
a = 10, b = 2   →   a/b = 5,  5 % 7 = 5
but (a % 7) / (b % 7) = 3 / 2 = 1   ✘
```

Modular arithmetic lives in the ring ℤ/MODℤ, where "divide by b" means "multiply by the element that undoes b". That element is `b^(-1)`.

```cpp
ans = a % MOD * inv(b, MOD) % MOD;
```

**Memory trick:** *There is no division mod p. There is only multiplication by an inverse.*

---

## Factorials and nCr mod p

```cpp
const int MAXN = 2e5 + 5;
ll fact[MAXN], invFact[MAXN];

void precompute() {
    fact[0] = 1;
    for (int i = 1; i < MAXN; i++) fact[i] = fact[i-1] * i % MOD;
    invFact[MAXN-1] = binpow(fact[MAXN-1], MOD - 2, MOD);
    for (int i = MAXN - 1; i > 0; i--) invFact[i-1] = invFact[i] * i % MOD;
}

ll nCr(int n, int r) {
    if (r < 0 || r > n || n < 0) return 0;
    return fact[n] * invFact[r] % MOD * invFact[n-r] % MOD;
}
ll nPr(int n, int r) {
    if (r < 0 || r > n) return 0;
    return fact[n] * invFact[n-r] % MOD;
}
```
**Cost:** O(MAXN + log MOD) preprocessing, O(1) per query.

**Why the backward loop for `invFact`:** only **one** `binpow` is needed. Since `invFact[i-1] = invFact[i] * i`, you get all of them in O(n) instead of O(n log MOD).

**Trap:** guard `r < 0 || r > n` — combinatorics formulas frequently produce out-of-range arguments, and `invFact[-1]` is a crash.

**Trap:** `MAXN` must exceed the largest `n` you'll ever pass, which for "stars and bars" style formulas is often `n + k`, not `n`.

---

## When the modulus isn't prime

Fermat breaks. Options:

1. **`MOD = m` with known factorization** → compute mod each prime power, combine with CRT.
2. **Small n** → Pascal's triangle, which uses only addition:
   ```cpp
   C[0][0] = 1;
   for (int i = 1; i <= n; i++) {
       C[i][0] = 1;
       for (int j = 1; j <= i; j++)
           C[i][j] = (C[i-1][j-1] + C[i-1][j]) % MOD;   // works for ANY modulus
   }
   ```
   O(n²) time and memory — fine up to n ≈ 5000.
3. **Euler's theorem**: if `gcd(a, m) = 1`, `a^φ(m) ≡ 1`, so `a^(-1) = a^(φ(m)-1)`. You need `φ(m)`.

**Common non-prime moduli you'll see:** `1e9` (not prime!), `2^64` (implicit in `unsigned long long` overflow), `10` / `1000` for "last k digits".

---

## Common moduli

| MOD | Prime? | Note |
|---|---|---|
| `1e9 + 7` | yes | the default |
| `998244353` | yes | `= 119·2^23 + 1`, primitive root 3 → NTT-friendly, actual NTT code in `29_POLYNOMIAL_AND_ADVANCED_NUMBER_THEORY.md` |
| `1e9 + 9` | yes | second modulus for double hashing |
| `1e9` | **no** | `= 2^9 · 5^9` — Fermat fails |
| `2^61 - 1` | yes | for hashing; needs `__int128` |

---

## Pitfalls that produce WA

```
[ ] forgot a % MOD somewhere in a chain of multiplications
[ ] subtracted without + MOD
[ ] took mod of the exponent (only valid via Fermat: a^b = a^(b mod (p-1)) when gcd(a,p)=1)
[ ] compared two values after modding (a > b mod p is meaningless)
[ ] the answer should NOT be modded (e.g. it's a count that fits in ll)
[ ] used inverse of 0
[ ] MOD not prime but used binpow(x, MOD-2)
[ ] precomputed factorials too small
```

**The exponent rule, precisely:** for prime `p` and `gcd(a,p)=1`, `a^b mod p = a^(b mod (p-1)) mod p`. If `gcd(a,p) != 1` this is wrong. For general `m`, use the generalized Euler theorem with care — most problems only need the prime case.

---

## Optional: a `Mint` struct

```cpp
struct Mint {
    int v;
    Mint(ll x = 0) { v = int(((x % MOD) + MOD) % MOD); }
    Mint& operator+=(Mint o){ v += o.v; if (v >= MOD) v -= MOD; return *this; }
    Mint& operator-=(Mint o){ v -= o.v; if (v < 0) v += MOD; return *this; }
    Mint& operator*=(Mint o){ v = int(1LL * v * o.v % MOD); return *this; }
    friend Mint operator+(Mint a, Mint b){ return a += b; }
    friend Mint operator-(Mint a, Mint b){ return a -= b; }
    friend Mint operator*(Mint a, Mint b){ return a *= b; }
    Mint pow(ll e) const { Mint r = 1, b = *this; while (e) { if (e&1) r *= b; b *= b; e >>= 1; } return r; }
    Mint inv() const { return pow(MOD - 2); }
    friend Mint operator/(Mint a, Mint b){ return a * b.inv(); }
};
```
**Worth it when:** the problem is heavy in modular algebra (matrix expo, polynomial work, long formulas). **Not worth it when:** you have three `% MOD`s in the whole solution — the struct costs more typing time than it saves.

Note `+=` uses a conditional subtract instead of `%` — measurably faster in hot loops.

---

## Memory hooks

- *Add and multiply freely. Subtract with `+MOD`. Never divide.*
- *Fermat: `a^(p-2)` is the inverse — because `a^(p-1) = 1`, peel off one `a`.*
- *One `binpow` for the last inverse factorial, then walk backwards.*
- *C++ `%` keeps the sign of the left operand.*