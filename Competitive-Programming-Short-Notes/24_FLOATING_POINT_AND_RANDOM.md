# 24 — Floating Point & Randomization

---

# Floating point

## The types

| Type | Bytes | Mantissa bits | Decimal digits | Max exact integer |
|---|---|---|---|---|
| `float` | 4 | 24 | ~7 | 2^24 ≈ 1.7e7 |
| `double` | 8 | 53 | ~15–16 | 2^53 ≈ 9.0e15 |
| `long double` (x86 GCC) | 16 (10 used) | 64 | ~18–19 | 2^64 ≈ 1.8e19 |

**Never use `float` in a contest.** It buys nothing and loses precision.
**`long double` on Codeforces:** available on 64-bit compilers, uses the x87 80-bit format. Costs ~2× the time of `double`. Print with `%Lf`.

**The key line:** a `double` holds integers **exactly** only up to 2^53 ≈ 9e15. Beyond that, `(double)(x)` and `x+1` can be the same value. This is why `long long` arithmetic must never be routed through a `double`.

---

## Why `a == b` fails

`0.1` is not representable in binary. It's stored as the nearest double, which is slightly off. Accumulated error means:

```cpp
0.1 + 0.2 == 0.3       // FALSE
```

**Always compare with a tolerance:**
```cpp
const double EPS = 1e-9;
bool eq(double a, double b) { return fabs(a - b) < EPS; }
bool lt(double a, double b) { return a < b - EPS; }
bool le(double a, double b) { return a < b + EPS; }
```

## Choosing EPS

| Situation | EPS |
|---|---|
| Values ~1, answer needs 1e-6 | `1e-9` |
| Values ~1e9, answer needs 1e-6 | `1e-6` or relative error |
| Geometry with coordinates ~1e9 | prefer **integers**; if forced, `1e-6` |
| Binary search on a double | fixed iteration count, no EPS |
| Comparing very large values | **relative** error |

**Absolute vs relative:** `fabs(a-b) < EPS` fails for large values (the gap between adjacent doubles near 1e18 is ~256). Use:
```cpp
bool eq(double a, double b) {
    return fabs(a - b) <= EPS * max({1.0, fabs(a), fabs(b)});
}
```

**Codeforces problems usually say** "your answer is accepted if the absolute **or relative** error is at most 1e-6". Printing with `setprecision(9)` or more and not accumulating error is normally enough.

**Trap: EPS too large is as bad as too small.** With `EPS = 1e-6` and true values differing by 1e-7, you'll call distinct things equal.

---

## Integer functions that lie

```cpp
(int)sqrt(1e18)        // may be off by one
(int)(a / b)           // fine for ints, but (int)(0.1*30) can be 2, not 3
round(x), floor(x), ceil(x)
(int)pow(10, 2)        // can be 99!
```

**`sqrt` fix:**
```cpp
ll isqrt(ll n) {
    ll r = (ll)sqrtl((long double)n);
    while (r > 0 && r * r > n) r--;
    while ((r+1) * (r+1) <= n) r++;
    return r;
}
```

**`pow` fix:** never use `pow` for integer exponentiation. Use `binpow`, or a loop, or `1LL << k` for powers of two.

**`log2` fix:** use `__lg(x)` or `31 - __builtin_clz(x)`.

**Casting truncates toward zero**, it doesn't round. `(int)(-2.7) == -2`, `(int)2.7 == 2`. Use `llround(x)` when you want nearest.

---

## Output precision

```cpp
cout << fixed << setprecision(10) << x << '\n';    // 10 digits AFTER the point
cout << setprecision(10) << x << '\n';             // 10 SIGNIFICANT digits
printf("%.10f\n", (double)x);
printf("%.10Lf\n", (long double)x);
```

**Distinguish:** `setprecision(6)` alone prints `1.23457e+08` for 123456789. With `fixed`, it prints `123456789.000000`. Problems asking for "6 decimal places" want `fixed`.

**Sticky:** both `fixed` and `setprecision` persist on the stream until changed.

**Trap:** printing `-0.000000`. Some checkers accept it, some don't. Add `if (fabs(x) < EPS) x = 0;` before printing.

---

## Avoiding floating point entirely

Most of the time you can:

| Instead of | Do |
|---|---|
| `sqrt(d) < sqrt(e)` | compare `d < e` |
| `a / b > c / d` | compare `a*d > c*b` (watch signs and overflow) |
| average `(a+b)/2.0` | keep `a+b` and compare doubled values |
| `ceil(a / (double)b)` | `(a + b - 1) / b` for positives |
| `floor(log2(x))` | `__lg(x)` |
| fractional coordinates | multiply everything by the denominator |
| `pow(a, b)` | `binpow` |

**This is the single biggest precision win.** When the input is integral, keep it integral.

**Comparing fractions safely:**
```cpp
// a/b vs c/d with b, d > 0
// cross-multiply: a*d vs c*b  — use __int128 if the products can exceed 9e18
```

---

## Accumulated error

```cpp
double s = 0;
for (int i = 0; i < 1e7; i++) s += 0.1;      // drifts noticeably from 1e6
```
**Mitigations:** sum in sorted order (smallest first), use Kahan summation, or just use `long double`. In contests, the usual fix is to not sum floats at all — sum integers and divide once at the end.

---

## Traps

```
[ ] used == on doubles
[ ] EPS wrong by orders of magnitude for the value range
[ ] (int)sqrt(n) off by one
[ ] pow() used for integer powers
[ ] long long routed through a double (loses exactness past 2^53)
[ ] setprecision without fixed
[ ] printed -0.000000
[ ] float instead of double
[ ] accumulated error from summing many values
[ ] binary search on doubles with while(hi-lo>eps) → infinite loop
[ ] compared distances with sqrt instead of squared values
```

---

# Randomization

## Use `mt19937`, not `rand()`

```cpp
mt19937 rng(chrono::steady_clock::now().time_since_epoch().count());
mt19937_64 rng64(chrono::steady_clock::now().time_since_epoch().count());

int x = uniform_int_distribution<int>(1, n)(rng);
ll  y = uniform_int_distribution<ll>(0, LLONG_MAX)(rng64);
shuffle(v.begin(), v.end(), rng);
```

**Why not `rand()`:**
- On some platforms `RAND_MAX` is only **32767**, so `rand() % n` for n > 32767 can never produce most values.
- `rand() % n` is **biased** unless `n` divides `RAND_MAX + 1`.
- The underlying generator is low quality and its behaviour differs between platforms.
- `random_shuffle` (which uses it) was deprecated in C++14 and **removed in C++17**.

**`mt19937`** is the Mersenne Twister — period 2^19937−1, fast, uniform, and `uniform_int_distribution` is unbiased.

## Why seed with `chrono`

A fixed seed is deterministic, so a hacker can reproduce your exact random choices and construct a worst case. Seeding from the clock makes each run different.

```cpp
mt19937 rng(chrono::steady_clock::now().time_since_epoch().count());
```
**Use `steady_clock`, not `system_clock`** — it's monotonic and higher resolution in practice. Some people add `(uintptr_t)make_unique<char>().get()` to mix in ASLR entropy.

**Trap:** `mt19937 rng(time(0))` has one-second resolution. Two submissions in the same second get the same seed. Use `chrono`.

## Where randomization actually helps in contests

### 1. Hardened hash maps

The default `unordered_map` hash is attackable on Codeforces. See `02_STL_CHEATSHEET.md` for the splitmix64 wrapper with a clock-derived constant.

### 2. String hashing

A fixed base with `MOD = 1e9+7` is attackable. Pick the base at random from `[256, MOD-2)` at runtime. See `08_STRING_TRICKS.md`.

### 3. Zobrist hashing

Assign each distinct value a random 64-bit number; represent a multiset by the XOR (or sum) of its elements' randoms. Then "do these two ranges contain the same multiset?" becomes a single 64-bit comparison.

```cpp
mt19937_64 rng(...);
map<int, uint64_t> z;
for (int v : distinctValues) z[v] = rng();
// XOR-prefix over the array → O(1) multiset comparison for ranges
```
Collision probability per comparison ≈ 2^-64. Also used for hashing tree shapes and game states.

### 4. Randomized algorithms

- **Random pivot** in quickselect / quicksort to defeat adversarial input.
- **Shuffle before a greedy or a heuristic** to avoid worst-case input order.
- **Random sampling:** "does some element appear more than n/2 times?" — sample 30 random elements; if one is the majority you'll almost certainly hit it (failure probability 2^-30).
- **Pollard's rho** factorization uses a random polynomial — full code in `29_POLYNOMIAL_AND_ADVANCED_NUMBER_THEORY.md`.

### 5. Breaking ties / randomized restarts

For construction problems where a greedy sometimes works: shuffle and retry until it works, if the success probability is decent and the time limit allows.

## Traps

```
[ ] rand() % n — biased, and RAND_MAX may be tiny
[ ] random_shuffle removed in C++17
[ ] fixed seed → hackable
[ ] time(0) seed → same seed within one second
[ ] uniform_int_distribution bounds are INCLUSIVE on both ends
[ ] reseeding the generator inside a loop (destroys randomness AND is slow)
[ ] creating the distribution object inside a hot loop (minor, but free to avoid)
[ ] assuming a randomized algorithm is always correct (state the failure probability)
```

---

## Memory hooks

- *`double` is exact for integers only up to 2^53.*
- *Never `==` on floats; never `sqrt` to compare; never `pow` for integers.*
- *`fixed` = digits after the point; without it = significant digits.*
- *Keep integer input in integers. Every cast to `double` is a risk.*
- *`mt19937` + `chrono` seed. `rand()` is biased and small.*
- *Fixed seeds and fixed hash bases get hacked.*