# 29 — Polynomial & Advanced Number Theory

Convolution (FFT/NTT), full 64-bit factorization (Pollard's rho), Möbius inversion, and discrete log (BSGS). This is the home of the "full code" that `03_MATH_NUMBER_THEORY.md` (Miller–Rabin / Pollard), `04_MODULAR_ARITHMETIC.md` (NTT-friendly 998244353), and `24_FLOATING_POINT_AND_RANDOM.md` (rho's random polynomial) point to.

---

## What convolution is

Polynomial multiplication: `c[k] = Σ a[i]·b[k−i]`, result length `|a| + |b| − 1`. Naive is O(n²); FFT/NTT make it O(n log n). Reach for it when:

- multiply two polynomials / big integers
- **count** combinations: `a[i]` = ways to use i of item A, `b[j]` for item B → `c` counts sums
- string matching with mismatches / wildcards (indicator arrays × reversed pattern)
- knapsack-style "number of subsets with sum s" in one shot

---

## NTT mod 998244353 — the default

```cpp
const ll MOD = 998244353, G = 3;              // = 119·2^23 + 1, primitive root 3 (04)

ll binpow(ll a, ll b, ll m) { a %= m; if (a < 0) a += m; ll r = 1 % m; for (; b; b >>= 1, a = a * a % m) if (b & 1) r = r * a % m; return r; }

void ntt(vector<ll>& a, bool invert) {
    int n = a.size();
    for (int i = 1, j = 0; i < n; i++) {                // bit-reversal permutation
        int bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) swap(a[i], a[j]);
    }
    for (int len = 2; len <= n; len <<= 1) {
        ll w = binpow(G, (MOD - 1) / len, MOD);         // primitive len-th root of unity
        if (invert) w = binpow(w, MOD - 2, MOD);
        for (int i = 0; i < n; i += len) {
            ll wn = 1;
            for (int j = 0; j < len / 2; j++) {
                ll u = a[i + j], v = a[i + j + len/2] * wn % MOD;
                a[i + j]           = (u + v) % MOD;
                a[i + j + len/2]   = (u - v + MOD) % MOD;
                wn = wn * w % MOD;
            }
        }
    }
    if (invert) {
        ll nInv = binpow(n, MOD - 2, MOD);
        for (ll& x : a) x = x * nInv % MOD;
    }
}

vector<ll> multiply(vector<ll> a, vector<ll> b) {        // coefficients are mod MOD
    int need = a.size() + b.size() - 1;
    int n = 1; while (n < need) n <<= 1;                 // n must divide MOD − 1 = 2^23·119
    a.resize(n); b.resize(n);
    ntt(a, false); ntt(b, false);
    for (int i = 0; i < n; i++) a[i] = a[i] * b[i] % MOD;
    ntt(a, true);
    a.resize(need);
    return a;
}
```

**Size limit:** `n` must be a power of two dividing `MOD − 1` → max `n = 2^23 ≈ 8.4e6`. Longer convolutions need a bigger prime (table below) or CRT.

**Other NTT-friendly primes** (all with root 3):

| MOD | = k·2^m + 1 | Max n |
|---|---|---|
| `998244353` | 119·2^23 + 1 | 2^23 |
| `1004535809` | 479·2^21 + 1 | 2^21 |
| `469762049` | 7·2^26 + 1 | 2^26 |
| `167772161` | 5·2^25 + 1 | 2^25 |

**Other target modulus?** Either three NTTs under the primes above + CRT, or split coefficients into 15-bit halves and run each through a single NTT. If the coefficients are small (products < ~1e15) plain FFT below is easier.

---

## FFT with complex doubles

When results are reals, or coefficients are small enough to round safely:

```cpp
using cd = complex<double>;
const double PI = acos(-1.0);

void fft(vector<cd>& a, bool invert) {
    int n = a.size();
    for (int i = 1, j = 0; i < n; i++) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) swap(a[i], a[j]);
    }
    for (int len = 2; len <= n; len <<= 1) {
        double ang = 2 * PI / len * (invert ? -1 : 1);
        cd wlen(cos(ang), sin(ang));
        for (int i = 0; i < n; i += len) {
            cd w(1);
            for (int j = 0; j < len / 2; j++) {
                cd u = a[i + j], v = a[i + j + len/2] * w;
                a[i + j]            = u + v;
                a[i + j + len/2]    = u - v;
                w *= wlen;
            }
        }
    }
    if (invert)
        for (cd& x : a) x /= n;
}

vector<ll> multiplyF(const vector<ll>& a, const vector<ll>& b) {
    vector<cd> fa(a.begin(), a.end()), fb(b.begin(), b.end());
    int need = a.size() + b.size() - 1;
    int n = 1; while (n < need) n <<= 1;
    fa.resize(n); fb.resize(n);
    fft(fa, false); fft(fb, false);
    for (int i = 0; i < n; i++) fa[i] *= fb[i];
    fft(fa, true);
    vector<ll> res(need);
    for (int i = 0; i < need; i++) res[i] = llround(fa[i].real());
    return res;
}
```

**Precision rule of thumb:** keep the largest answer coefficient under ~1e15 with `double`; use `long double`/splitting beyond. Integer NTT has no precision at all — prefer it for counting mod 998244353.

---

## Pollard's rho — full factorization of n < 2^64

Miller–Rabin first (`03_MATH_NUMBER_THEORY.md` explains the 12 bases), then rho for the composite pieces:

```cpp
using u64 = unsigned long long; using u128 = __uint128_t;

u64 mulmod(u64 a, u64 b, u64 m) { return (u128)a * b % m; }
u64 powmod(u64 base, u64 e, u64 m) { u64 r = 1; for (base %= m; e; e >>= 1, base = mulmod(base, base, m)) if (e & 1) r = mulmod(r, base, m); return r; }

bool isPrime(u64 n) {
    if (n < 2) return false;
    for (u64 p : {2,3,5,7,11,13,17,19,23,29,31,37}) if (n % p == 0) return n == p;
    u64 d = n - 1; int s = 0;
    while (!(d & 1)) d >>= 1, s++;
    for (u64 a : {2,3,5,7,11,13,17,19,23,29,31,37}) {
        u64 x = powmod(a, d, n);
        if (x == 1 || x == n - 1) continue;
        bool ok = false;
        for (int i = 1; i < s; i++) { x = mulmod(x, x, n); if (x == n - 1) { ok = true; break; } }
        if (!ok) return false;
    }
    return true;
}

mt19937_64 rng(chrono::steady_clock::now().time_since_epoch().count());

u64 pollard(u64 n) {                                  // n odd composite; returns a non-trivial factor
    if (n % 2 == 0) return 2;
    if (n % 3 == 0) return 3;
    while (true) {
        u64 c = 1 + rng() % (n - 1);                  // random polynomial x² + c
        u64 x = 2, y = 2, d = 1;
        auto f = [&](u64 v) { return (mulmod(v, v, n) + c) % n; };
        while (d == 1) {                              // Floyd cycle detection
            x = f(x);
            y = f(f(y));
            u64 diff = x > y ? x - y : y - x;
            d = gcd(diff, n);
        }
        if (d != n) return d;                         // d == n: cycle closed without splitting → retry with new c
    }
}

void factorize(u64 n, map<u64, int>& f) {             // f = {prime → exponent}
    if (n == 1) return;
    if (isPrime(n)) { f[n]++; return; }
    u64 d = pollard(n);
    factorize(d, f);
    factorize(n / d, f);
}
// usage: map<u64,int> f; factorize(1000000000000000003ULL, f);
```

**Complexity:** expected O(n^(1/4)) per split — seconds for n near 1e18 with hard semiprimes, instant for usual contest inputs. The `gcd` every iteration is the simple Floyd variant; Brent's improvement batches it, but this version is rarely the bottleneck.

**Use when:** n > 1e12 (below that the SPF sieve in `03` is simpler), counting divisors / Euler φ of huge n, or "find a factor" problems. For a single primality question just use `isPrime` alone.

---

## Möbius function & inversion

**μ(n):** `0` if n has a squared prime factor; otherwise `(−1)^(#distinct primes)`. `μ(1) = 1`.

Linear sieve (O(n), same loop structure as SPF):

```cpp
const int MAXN = 1e7;                          // adjust
vector<int> mu(MAXN + 1), primes;
vector<bool> comp(MAXN + 1);
void mobiusSieve() {
    mu[1] = 1;                                 // empty product — easy to forget
    for (int i = 2; i <= MAXN; i++) {
        if (!comp[i]) { primes.push_back(i); mu[i] = -1; }
        for (int p : primes) {
            if ((ll)i * p > MAXN) break;
            comp[i * p] = true;
            if (i % p == 0) { mu[i * p] = 0; break; }   // squared prime
            mu[i * p] = -mu[i];
        }
    }
}
```

**The key identity:** `Σ_{d|n} μ(d) = 1` if `n = 1`, else `0`. This is the inversion switch:

- **Divisor form:** if `F(n) = Σ_{d|n} f(d)`, then `f(n) = Σ_{d|n} μ(d)·F(n/d)`.
- **Multiple form:** if `F(n) = Σ_{n|m} f(m)`, then `f(n) = Σ_{n|m} μ(m/n)·F(m)`.

**Workhorse application — count by gcd.** For pairs `1 ≤ i, j ≤ N`:

- coprime pairs: `#{(i,j) : gcd = 1} = Σ_{d≤N} μ(d)·⌊N/d⌋²`
- exactly gcd g: `Σ_d μ(d)·⌊N/(g·d)⌋²` — subtract g-multiples to get ranges, e.g. gcd in [L,R]
- neighbor identity (contrast): `Σ_{i,j} gcd(i,j) = Σ_{d≤N} φ(d)·⌊N/d⌋²` — **φ**, not μ

Compute with prefix sums of μ and floor-division blocks, O(√N):

```cpp
vector<ll> prefMu(MAXN + 1);                   // prefMu[i] = mu[1] + ... + mu[i], built after the sieve
ll coprimePairs(ll N) {
    ll res = 0;
    for (ll l = 1; l <= N; ) {
        ll q = N / l, r = N / q;               // ⌊N/d⌋ is constant on [l, r]
        res += (prefMu[r] - prefMu[l - 1]) * q * q;
        l = r + 1;
    }
    return res;
}
```

**Recognition:** "coprime", "gcd equals", "divisible by", counting pairs/tuples by gcd, sum of `f(gcd(i,j))` for any f built from μ or φ. Invert the condition into a sum over divisors/multiples.

---

## Discrete log — baby-step giant-step (BSGS)

Smallest `x ≥ 0` with `a^x ≡ b (mod m)`, requiring `gcd(a, m) = 1` (any prime m qualifies):

```cpp
ll bsgs(ll a, ll b, ll m) {                    // smallest x >= 0, or -1
    a %= m; b %= m;
    if (m == 1) return 0;
    if (b == 1) return 0;                      // x = 0
    ll n = sqrtl((long double)m) + 1;
    unordered_map<ll, ll> table;               // b·a^j -> largest j < n
    ll cur = b;
    for (ll j = 0; j < n; j++) {
        table[cur] = j;                        // overwrite on purpose: largest j gives smallest x
        cur = (__int128)cur * a % m;
    }
    ll an = 1;                                 // a^n
    for (ll i = 0; i < n; i++) an = (__int128)an * a % m;
    ll best = -1;
    ll step = 1;                               // a^(i·n)
    for (ll i = 0; i <= n; i++) {
        auto it = table.find(step);
        if (it != table.end()) {
            ll x = i * n - it->second;         // x = i·n − j  (write a^x = a^(in−j) = b ⟺ a^(in) = b·a^j)
            if (x >= 0 && (best < 0 || x < best)) best = x;
        }
        step = (__int128)step * an % m;
    }
    return best;
}
```

**Why it works:** every exponent `x < m` can be written `x = i·n − j` with `0 ≤ j < n`, `i ≤ n` — so `a^(i·n) = b·a^j` is a lookup between √m precomputed values and √m sweeps. **O(√m)** time.

**Notes:**
- `gcd(a, m) > 1` needs the extended variant (divide out common factors first) — rare in contests; check the constraint.
- `unordered_map` is fine per problem (only √m entries); a sorted vector + binary search is an alternative.
- BSGS on prime modulus = discrete log in 𝔽_p — the classic use ("find x with a^x ≡ b mod p").

---

## Traps

```
[ ] NTT: transform length not a power of two, or > 2^23 under 998244353 (root of unity doesn't exist)
[ ] NTT: forgot the final 1/n scaling in the inverse transform
[ ] NTT: inverse root computed with the wrong exponent (it's the modular inverse of the forward root)
[ ] FFT: answer coefficients beyond ~1e15 → rounding errors; switch to NTT / long double / splitting
[ ] NTT/FFT: result length is |a| + |b| − 1, not max(|a|, |b|)
[ ] Pollard: seeded rng once with a FIXED value and hit a bad c — seed from chrono, and retry on d == n
[ ] Pollard: forgot base cases n % 2 / n % 3 (rho can spin on even numbers)
[ ] Pollard: isPrime must run before pollard, or recursion doesn't terminate on primes
[ ] Möbius: forgot mu[1] = 1
[ ] Möbius: used the sieve's mu for the floor-block sum without prefix sums → O(N), not O(√N)
[ ] BSGS: gcd(a, m) ≠ 1 (needs extended variant)
[ ] BSGS: returned a negative x (i·n − j < 0) — filter it
[ ] BSGS: "some x" vs "smallest x" — the problem usually wants the smallest
```

## Memory hooks

- *Convolution = coefficient products; length a + b − 1; FFT/NTT turn O(n²) into O(n log n).*
- *998244353 = 119·2^23 + 1 → transform sizes up to 2^23; root 3.*
- *Rho = Floyd cycle-finding with x² + c on Z_n; expected O(n^(1/4)) per factor.*
- *μ is the parity of prime factors on squarefree numbers — the inversion switch for "d | n" problems.*
- *Σ μ(d)⌊N/d⌋² counts coprime pairs; Σ φ(d)⌊N/d⌋² sums gcd.*
- *BSGS: √m baby steps, √m giant steps, x = i·n − j.*
