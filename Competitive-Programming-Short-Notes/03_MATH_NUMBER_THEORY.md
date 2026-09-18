# 03 — Math & Number Theory Toolkit

---

## GCD / LCM

```cpp
#include <numeric>
int g = gcd(a, b);       // C++17
int l = lcm(a, b);       // C++17 — CAN OVERFLOW
int g = __gcd(a, b);     // GNU, works pre-C++17
```

```cpp
ll my_gcd(ll a, ll b) { return b ? my_gcd(b, a % b) : a; }   // O(log min(a,b))
ll my_lcm(ll a, ll b) { return a / my_gcd(a, b) * b; }       // divide FIRST
```

**Remember:** `a / g * b`, never `a * b / g`. With `a = b = 1e9`, `a*b = 1e18` still fits, but with `a = b = 1e18` it doesn't. Free habit, no downside.

**Facts worth memorizing:**
- `gcd(a, 0) = a`, `gcd(0, 0) = 0`
- `gcd(a, b) = gcd(a, b - a) = gcd(a, b mod a)` — the whole Euclidean algorithm
- `gcd(a, b) · lcm(a, b) = a · b`
- gcd of a range is **idempotent** → sparse table gives O(1) range gcd
- gcd over a growing suffix takes at most O(log max) distinct values (each change at least halves it) — this powers "count subarrays with gcd = k" in O(n log² n)

---

## Extended Euclid

Solves `a·x + b·y = gcd(a, b)`.

```cpp
ll ext_gcd(ll a, ll b, ll &x, ll &y) {
    if (!b) { x = 1; y = 0; return a; }
    ll x1, y1;
    ll g = ext_gcd(b, a % b, x1, y1);
    x = y1;
    y = x1 - (a / b) * y1;
    return g;
}
```
**Use when:** modular inverse with non-prime modulus, solving linear Diophantine equations, CRT.

`ax + by = c` has a solution **iff** `gcd(a,b) | c`. Scale the returned `(x,y)` by `c/g`. General solution: `x + k·(b/g)`, `y - k·(a/g)`.

---

## Primality

### Trial division — O(√n)

```cpp
bool isPrime(ll n) {
    if (n < 2) return false;
    if (n % 2 == 0) return n == 2;
    for (ll i = 3; i * i <= n; i += 2)
        if (n % i == 0) return false;
    return true;
}
```
**Trap:** write `i * i <= n`, not `i <= sqrt(n)` — `sqrt` on `ll` loses precision near 1e18 and is slower.
**Trap:** `i * i` overflows if `n` is near `LLONG_MAX`. Use `i <= n / i` in that case.

### Miller–Rabin — deterministic for 64-bit

```cpp
using u64 = unsigned long long;
using u128 = __uint128_t;
u64 pw(u64 b, u64 e, u64 m){ u64 r=1; b%=m; while(e){ if(e&1) r=(u128)r*b%m; b=(u128)b*b%m; e>>=1;} return r; }
bool check(u64 n, u64 a, u64 d, int s) {
    u64 x = pw(a, d, n);
    if (x == 1 || x == n-1) return true;
    for (int i = 1; i < s; i++) { x = (u128)x*x%n; if (x == n-1) return true; }
    return false;
}
bool isPrimeMR(u64 n) {
    if (n < 2) return false;
    for (u64 p : {2,3,5,7,11,13,17,19,23,29,31,37}) { if (n % p == 0) return n == p; }
    u64 d = n - 1; int s = 0;
    while (!(d & 1)) d >>= 1, s++;
    for (u64 a : {2,3,5,7,11,13,17,19,23,29,31,37})
        if (!check(n, a, d, s)) return false;
    return true;
}
```
**These 12 bases are deterministic for all n < 2^64.** Needs a 64-bit compiler (`__int128`).
**Use when:** n > 1e12 and you need primality, or Pollard-rho factorization.

---

## Sieve of Eratosthenes — O(n log log n)

```cpp
const int N = 1e6 + 5;
vector<bool> comp(N, false);
vector<int> primes;
void sieve() {
    for (int i = 2; i < N; i++) {
        if (!comp[i]) {
            primes.push_back(i);
            for (ll j = (ll)i * i; j < N; j += i) comp[j] = true;
        }
    }
}
```
**Trap:** start the inner loop at `i*i`, and make it `ll` if `N` is near 1e9 (`i*i` overflows int at i > 46341).
**Memory:** `vector<bool>` for 1e7 = 1.25 MB. `bool[]` for 1e7 = 10 MB. Here the bit-packing actually helps.

### Smallest prime factor sieve — factorize in O(log n)

```cpp
int spf[N];
void spfSieve() {
    for (int i = 1; i < N; i++) spf[i] = i;
    for (int i = 2; (ll)i * i < N; i++)
        if (spf[i] == i)
            for (int j = i * i; j < N; j += i)
                if (spf[j] == j) spf[j] = i;
}
vector<pii> factorize(int n) {          // returns (prime, exponent)
    vector<pii> f;
    while (n > 1) {
        int p = spf[n], c = 0;
        while (n % p == 0) n /= p, c++;
        f.push_back({p, c});
    }
    return f;
}
```
**Use when:** you factorize many numbers (up to 1e5 queries). One O(n) sieve, then O(log n) each.

### Linear sieve — O(n), also gives φ, μ, d

```cpp
int spf[N]; vector<int> pr;
for (int i = 2; i < N; i++) {
    if (!spf[i]) { spf[i] = i; pr.push_back(i); }
    for (int p : pr) {
        if (p > spf[i] || (ll)p * i >= N) break;
        spf[p * i] = p;
    }
}
```
Rarely necessary — the log-log sieve is fast enough to 1e7.

---

## Factorization of a single number — O(√n)

```cpp
vector<pll> factorize(ll n) {
    vector<pll> f;
    for (ll p = 2; p * p <= n; p++) {
        if (n % p) continue;
        int c = 0;
        while (n % p == 0) n /= p, c++;
        f.push_back({p, c});
    }
    if (n > 1) f.push_back({n, 1});     // remaining large prime — DON'T FORGET
    return f;
}
```
**Trap:** the `if (n > 1)` line at the end. Without it, `n = 14` reports only `2`.

---

## Divisors

### Enumerate all divisors — O(√n)

```cpp
vector<ll> divisors(ll n) {
    vector<ll> d;
    for (ll i = 1; i * i <= n; i++)
        if (n % i == 0) {
            d.push_back(i);
            if (i != n / i) d.push_back(n / i);
        }
    sort(d.begin(), d.end());
    return d;
}
```
**Trap:** the `i != n/i` guard, or perfect squares get their root twice.

### Counts and sums from the factorization

If `n = p₁^a₁ · p₂^a₂ · … · pₖ^aₖ`:

- **number of divisors** `d(n) = ∏ (aᵢ + 1)`
- **sum of divisors** `σ(n) = ∏ (pᵢ^(aᵢ+1) − 1) / (pᵢ − 1)`
- **Euler phi** `φ(n) = n · ∏ (1 − 1/pᵢ)`

**Size bounds worth knowing:**

| n ≤ | max d(n) |
|---|---|
| 1e5 | 128 |
| 1e6 | 240 |
| 1e9 | 1344 |
| 1e18 | 103680 |

So "iterate over all divisors of every number ≤ 1e6" is fine; "store all divisors of 1e5 numbers" is also fine.

### Divisors of every number up to N — O(N log N)

```cpp
vector<vector<int>> div(N);
for (int i = 1; i < N; i++)
    for (int j = i; j < N; j += i)
        div[j].push_back(i);
```
**Remember the harmonic sum:** `N/1 + N/2 + N/3 + … = N·ln N ≈ N log N`. This pattern is **not** O(N²) — a very common misjudgement in both directions.

---

## Euler's totient φ

```cpp
ll phi(ll n) {
    ll r = n;
    for (ll p = 2; p * p <= n; p++)
        if (n % p == 0) {
            while (n % p == 0) n /= p;
            r -= r / p;
        }
    if (n > 1) r -= r / n;
    return r;
}
```
Sieve version:
```cpp
for (int i = 0; i < N; i++) phi[i] = i;
for (int i = 2; i < N; i++)
    if (phi[i] == i)                    // i is prime
        for (int j = i; j < N; j += i) phi[j] -= phi[j] / i;
```
**Facts:** `φ(p) = p−1`; `φ` is multiplicative for coprime arguments; `Σ_{d|n} φ(d) = n`.
**Euler's theorem:** `gcd(a,m)=1 ⟹ a^φ(m) ≡ 1 (mod m)`.

---

## Floor-division blocks — O(√n) distinct values

**Remember:** `n / i` takes only ~2√n distinct values as `i` runs 1..n.

```cpp
for (ll i = 1, j; i <= n; i = j + 1) {
    j = n / (n / i);            // largest j with n/j == n/i
    ll val = n / i;             // constant on [i, j]
    // contribute val * (j - i + 1)
}
```
**Use when:** `Σ_{i=1}^{n} f(n/i)`, counting multiples, divisor-sum problems, "how many i give quotient q".

---

## Chinese Remainder Theorem

Two congruences: `x ≡ r₁ (mod m₁)`, `x ≡ r₂ (mod m₂)`.

```cpp
// returns {r, m} meaning x ≡ r (mod m), or {-1,-1} if no solution
pll crt(ll r1, ll m1, ll r2, ll m2) {
    ll p, q, g = ext_gcd(m1, m2, p, q);
    if ((r2 - r1) % g) return {-1, -1};
    ll lcm = m1 / g * m2;
    ll t = ((r2 - r1) / g) % (m2 / g) * p % (m2 / g);
    ll r = ((r1 + (__int128)t * m1) % lcm + lcm) % lcm;
    return {r, lcm};
}
```
Fold a list by repeatedly applying this. Solution exists iff `r₁ ≡ r₂ (mod gcd(m₁,m₂))`.
If all `mᵢ` are pairwise coprime, a solution always exists and is unique mod `∏ mᵢ`.

---

## Matrix exponentiation

**Use when:** a linear recurrence with small order and a huge index (Fibonacci at n = 1e18, counting walks of length k in a graph).

```cpp
using Mat = array<array<ll, K>, K>;
Mat mul(const Mat&A, const Mat&B) {
    Mat C{};
    for (int i = 0; i < K; i++)
      for (int k = 0; k < K; k++) if (A[i][k])
        for (int j = 0; j < K; j++)
            C[i][j] = (C[i][j] + A[i][k] * B[k][j]) % MOD;
    return C;
}
Mat mpow(Mat A, ll e) {
    Mat R{}; for (int i = 0; i < K; i++) R[i][i] = 1;
    while (e) { if (e & 1) R = mul(R, A); A = mul(A, A); e >>= 1; }
    return R;
}
```
**Complexity:** O(K³ log n). Note the `i, k, j` loop order — cache-friendly (see `12_CPP_PERFORMANCE.md`).

Fibonacci: `[[1,1],[1,0]]^n` gives `F(n)` in position `[0][1]`.

### General k-th order linear recurrence — the companion matrix

**The gap the 2×2 Fibonacci example doesn't show:** how to build the matrix for an *arbitrary* order-k recurrence, not just order 2.

For `a_n = c_1·a_{n-1} + c_2·a_{n-2} + … + c_k·a_{n-k}` with known base cases `a_0 … a_{k-1}`:

**State vector** `S_n = (a_n, a_{n-1}, …, a_{n-k+1})^T` — the last k terms, newest first.

**Companion matrix** (k×k): row 0 holds the coefficients, every other row shifts the state down by one.
```
        c_1  c_2  c_3  ...  c_{k-1}  c_k
         1    0    0   ...    0       0
M   =    0    1    0   ...    0       0
         0    0    1   ...    0       0
              ...
         0    0    0   ...    1       0
```
so that `S_n = M · S_{n-1}` — row 0 recomputes `a_n` from the recurrence, and every row below just copies the previous row's entry down by one, sliding the window.

```cpp
const int K = /* recurrence order */;
using Mat = array<array<ll, K>, K>;

Mat buildCompanion(const array<ll, K>& c) {    // c[0..K-1] = c_1 .. c_K
    Mat M{};
    for (int j = 0; j < K; j++) M[0][j] = c[j] % MOD;
    for (int i = 1; i < K; i++) M[i][i-1] = 1;
    return M;
}

// a[0..K-1] = known base cases a_0 .. a_{K-1}
ll nthTerm(const array<ll, K>& c, const array<ll, K>& a, ll n) {
    if (n < K) return a[n];
    Mat M = buildCompanion(c);
    Mat Mp = mpow(M, n - (K - 1));              // S_{K-1} = (a_{K-1}, ..., a_0), newest first
    array<ll, K> S0{};
    for (int i = 0; i < K; i++) S0[i] = a[K - 1 - i];
    ll res = 0;
    for (int j = 0; j < K; j++) res = (res + Mp[0][j] * S0[j]) % MOD;
    return res;
}
```
**Complexity:** O(K³ log n) — identical to the 2×2 case, just with K set to the recurrence's actual order.

**Use when:** "tribonacci"-style recurrences (k=3), linear recurrences that fall out of a DP with a small, fixed window (`dp[n]` depends on `dp[n-1..n-k]` linearly), or counting walks of length n in a graph with k ≤ ~50 states (there `M` is the graph's adjacency matrix directly, not a companion matrix, and `mpow(M, n)[i][j]` = number of walks of length n from i to j).

**Trap:** `K` must be a compile-time constant for the `array<array<ll,K>,K>` template — if the order varies across test cases, switch to `vector<vector<ll>>` (same code, K³ cost is the same either way, just a slower constant factor from the extra indirection).
**Trap:** indexing `S0` — the state vector is newest-first (`a_{K-1}, a_{K-2}, …, a_0`), so `S0[i] = a[K-1-i]`, not `a[i]`. This is the line most likely to be transcribed backwards under time pressure.

---

## Memory hooks

- *Divide before you multiply* (LCM, nCr, anything with a gcd)
- *`i*i <= n`, never `i <= sqrt(n)`*
- *Don't forget the leftover prime after the √n loop*
- *Harmonic loop = n log n, not n²*
- *`n/i` has only √n distinct values*
- *Sieve inner loop starts at `i*i`*