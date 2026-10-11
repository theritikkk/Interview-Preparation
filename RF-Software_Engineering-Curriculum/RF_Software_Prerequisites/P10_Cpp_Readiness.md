# P10 — C++ Readiness for Python/NumPy Programmers

*Prerequisite module 10 · ⏱ about 8–10 hours (mostly typing and compiling) · assumes P09 and basic programming · every program below was **compiled and run with g++ 13.3 on Linux** (`-std=c++17`); the output shown is the real output.*

> **Where this unlocks the curriculum.** Lesson 22 opens the C++ track ("From Python to C++") and says every earlier lesson used Python/NumPy for convenience; Lessons 23–27, 31, 32, 36, 37 and 43 are then written in C++. They use `std::vector` and `std::complex`, `std::chrono` timing, `std::atomic`, AVX2 intrinsics (`-mavx2`), and compile with `-O0`/`-O3` and `-fsanitize=address`. They **assume you can read and write small C++ programs**. This module gets you there; it deliberately stops before what those lessons teach (memory layout and cache behaviour — Lesson 22; ring buffers — 23; threads and lock-free code — 24; fixed-point — 25; SIMD — 26).

---

## 10.1 Why C++ for real-time radio — and what changes from Python

| | Python/NumPy | C++ |
|---|---|---|
| Typing | dynamic (types discovered at run time) | **static** (types fixed at compile time) |
| Execution | interpreter; NumPy's inner loops are C | compiled to machine code |
| Memory | automatic (garbage collection) | **you decide** stack vs heap; deterministic lifetimes |
| Timing | variable (GC pauses, interpreter overhead) | **predictable** — essential for real-time deadlines |
| Errors | exceptions at run time | many caught at compile time; some become **undefined behaviour** |

Real-time PHY code must meet hard deadlines (a 1 ms LTE subframe, Lesson 2's 30,720 samples), so it is written where **you control memory and timing**. The price: more rules, and mistakes can corrupt memory silently.

---

## 10.2 Compile, link, run 🔥

```bash
g++ -std=c++17 -Wall -Wextra -O2 hello.cpp -o hello && ./hello
```

**hello.cpp**
```cpp
#include <cstdio>

int main() {
    std::printf("Hello, RF!\n");
    return 0;
}
```
Output: `Hello, RF!`

**What the command does:** *preprocessor* (`#include` pastes headers) → *compiler* (C++ → object code, type checking) → *linker* (combines object code and libraries into an executable). Three kinds of failure: **compile errors** (syntax/type errors — fix the line the compiler names), **link errors** ("undefined reference" — a function declared but not defined), and **run-time errors** (crash or wrong results).

**Flags you will see in the lessons** (a scan found `-O0`, `-O3`, `-mavx2`, `-fsanitize=address`, `-fsanitize=thread`, `-O2`):

| Flag | Meaning |
|---|---|
| `-std=c++17` | language standard (use C++17; `std::span` needs `-std=c++20`, verified to work in g++ 13) |
| `-O0` / `-O2` / `-O3` | optimisation level: none / good / aggressive — **speed differences are large** (§10.9) |
| `-Wall -Wextra` | turn on useful warnings — **always** use them; add `-Werror` to make warnings fatal |
| `-g` | include debug information |
| `-mavx2` / `-march=native` | allow SIMD instructions (Lesson 26) |
| `-fsanitize=address` / `undefined` / `thread` | runtime checkers for memory bugs / undefined behaviour / data races |

---

## 10.3 Types: sizes, fixed-width integers, conversions 🔥

C++ numeric types have sizes that matter for IQ data (P08). Use the **fixed-width types** from `<cstdint>` when the width matters: `int8_t`, `uint8_t`, `int16_t`, `int32_t`, … and `std::size_t` for sizes and indices.

**sizes.cpp**
```cpp
#include <cstdint>
#include <complex>
#include <cstddef>
#include <cstdio>
int main() {
    std::printf("char=%zu short=%zu int=%zu long=%zu long long=%zu\n", sizeof(char), sizeof(short), sizeof(int), sizeof(long), sizeof(long long));
    std::printf("int8_t=%zu int16_t=%zu int32_t=%zu uint8_t=%zu size_t=%zu pointer=%zu\n", sizeof(int8_t), sizeof(int16_t), sizeof(int32_t), sizeof(uint8_t), sizeof(std::size_t), sizeof(void*));
    std::printf("float=%zu double=%zu complex<float>=%zu complex<double>=%zu\n", sizeof(float), sizeof(double), sizeof(std::complex<float>), sizeof(std::complex<double>));
    // wrap vs conversion
    uint8_t u8 = 250; u8 += 10;
    uint16_t u16 = 65535; u16++;
    std::printf("uint8 250+10 -> %d ; uint16 65535+1 -> %d\n", (int)u8, (int)u16);
    std::printf("7/2=%d  7/2.0=%.1f  (int)3.99=%d  (int)-3.99=%d\n", 7/2, 7/2.0, (int)3.99, (int)-3.99);
    float f = 16777217; std::printf("float(16777217)=%.1f\n", f);
    int s = -1; unsigned int one = 1u;
    std::printf("-1 < 1u ? %s (signed/unsigned comparison)\n", (s < one) ? "true" : "false");
    int16_t h = (int16_t)40000; std::printf("(int16_t)40000 = %d (implementation-defined narrowing)\n", (int)h);
    return 0;
}
```
Output (the sign-compare warning appears at compile time under `-Wall -Wextra`):
```
char=1 short=2 int=4 long=8 long long=8
int8_t=1 int16_t=2 int32_t=4 uint8_t=1 size_t=8 pointer=8
float=4 double=8 complex<float>=8 complex<double>=16
uint8 250+10 -> 4 ; uint16 65535+1 -> 0
7/2=3  7/2.0=3.5  (int)3.99=3  (int)-3.99=-3
float(16777217)=16777216.0
-1 < 1u ? false (signed/unsigned comparison)
(int16_t)40000 = -25536 (implementation-defined narrowing)
```
**Lessons from that output**
* `std::complex<float>` is **8 bytes** (two floats) — the `complex64` of NumPy (P09 §9.3); 10⁶ samples = 8 MB, as `complex<double>` 16 MB.
* **Unsigned** integers wrap (250 + 10 → 4) — well-defined, as in NumPy (P08 §8.2).
* **Integer division truncates:** `7/2 = 3`; make one operand floating (`7/2.0`) to get 3.5; casting a float to int **truncates toward zero** (`-3.99 → -3`).
* **Converting between types can lose information:** a float can't hold 16,777,217 (P08 §8.5); `int16_t` from 40000 wraps.
* **Signed vs unsigned comparison is a trap:** `-1 < 1u` is **false** (the −1 is converted to 4,294,967,295). `-Wall -Wextra` warns about it.

### Undefined behaviour (UB) 🔥 — the biggest conceptual difference from NumPy
Some operations have **no defined result**; the compiler may assume they never happen and produce *anything*. The classic: **signed integer overflow** is UB (unlike NumPy's wrap-around):

**ub.cpp**
```cpp
#include <climits>
#include <cstdio>
int main(int argc, char**) {
    int x = INT_MAX;
    x += argc;               // argc == 1 at runtime: signed overflow is undefined behaviour
    std::printf("x = %d\n", x);
    return 0;
}
```
```bash
g++ -std=c++17 -O1 -g -fsanitize=undefined ub.cpp -o ub && ./ub
```
Real output:
```
ub.cpp:5:7: runtime error: signed integer overflow: 1 + 2147483647 cannot be represented in type 'int'
x = -2147483648
```
The program "worked" and printed a wrapped value — but only luck; another compiler/flag could do something else. The **UBSan** runtime check names the problem. (`argc` is used so the compiler can't fold the sum at compile time.)

---

## 10.4 Functions, references, `const` 🔥

* **Pass by value** copies the argument. **By reference** (`T&`) lets the callee modify the original. **By `const` reference** (`const T&`) avoids the copy *and* forbids modification — **the default for read-only large arguments** such as sample buffers.
* `auto` lets the compiler deduce a type; **range-for** (`for (const auto& s : x)`) loops over a container.

(These are all exercised in the combined program `demo.cpp` in §10.6.)

---

## 10.5 Memory: stack, heap, pointers, RAII 🔥

* **Stack:** automatic storage for local variables; fast; freed when the scope ends; **small** (typically megabytes) — a huge local array overflows it.
* **Heap (free store):** large allocations (`new`, or inside `std::vector`); you control lifetime.
* **Pointer:** a variable holding a **memory address**; `*p` is the value at that address; `p[i]` is the `i`-th element after it; `nullptr` means "points to nothing". An array name decays to a pointer to its first element.

### RAII — the idiom that replaces garbage collection 🔥
**R**esource **A**cquisition **I**s **I**nitialisation: tie a resource's lifetime to an object's **scope**; the destructor releases it automatically. `std::vector` and `std::unique_ptr` do this — you almost never write `new`/`delete` by hand.

**raii.cpp**
```cpp
#include <chrono>
#include <cstdio>
#include <memory>
#include <vector>

// RAII: the destructor runs automatically when the object leaves scope.
struct ScopedTimer {
    const char* name;
    std::chrono::steady_clock::time_point t0;
    explicit ScopedTimer(const char* n) : name(n), t0(std::chrono::steady_clock::now()) {}
    ~ScopedTimer() {
        auto dt = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        std::printf("[%s] took %.3f ms\n", name, dt);
    }
};

int main() {
    {
        ScopedTimer t("fill 1M floats");
        std::vector<float> v(1'000'000, 1.0f);      // memory freed automatically at the closing brace
        std::printf("size=%zu\n", v.size());
    }                                                // <- timer prints here, vector freed here
    auto p = std::make_unique<int[]>(4);             // heap array owned by a smart pointer
    p[0] = 7;
    std::printf("p[0]=%d\n", p[0]);
    return 0;                                        // p freed automatically
}
```
Real output (the time varies by machine):
```
size=1000000
[fill 1M floats] took 1.366 ms
p[0]=7
```
The destructor of `ScopedTimer` fired at the closing brace, *after* the vector's last use — a pattern worth reusing for benchmarking.

### The three memory bugs the lessons teach you to catch
1. **Out-of-bounds access** (heap-buffer-overflow). 2. **Use-after-free / dangling pointers.** 3. **Leaks / uninitialised reads.**

**asan.cpp** (off-by-one write)
```cpp
#include <cstdio>
int main() {
    int* p = new int[4];
    for (int i = 0; i <= 4; ++i) p[i] = i;      // BUG: i == 4 writes one element past the end
    std::printf("p[4] = %d\n", p[4]);
    delete[] p;
    return 0;
}
```
```bash
g++ -std=c++17 -g -O1 -fsanitize=address asan.cpp -o asan && ./asan
```
Real output begins:
```
ERROR: AddressSanitizer: heap-buffer-overflow on address 0x502000000020 ...
WRITE of size 4 at 0x502000000020 thread T0
    #0 ... in main asan.cpp:4
```
**Without the sanitizer the same program prints `p[4] = 4` and exits with code 0** — the bug is completely silent. *That* is why you run with `-fsanitize=address` during development. (Note: when I first made the out-of-bounds write without ever *reading* it back, the optimiser deleted the write and ASan reported nothing — undefined behaviour and optimisation interact.)

**uaf.cpp** (dangling pointer after a `vector` reallocates)
```cpp
#include <vector>
#include <cstdio>
int main() {
    std::vector<int> v(4, 1);
    int* p = v.data();
    v.push_back(2);                 // may reallocate -> p now dangles
    std::printf("%d\n", p[0]);      // use-after-free
    return 0;
}
```
Real output begins `ERROR: AddressSanitizer: heap-use-after-free on address 0x502000000010 ... READ of size 4 ... uaf.cpp:7`.

---

## 10.6 `std::vector`, `std::complex`, references, templates — one program 🔥

`std::vector<T>` is a **dynamic array**: elements are **contiguous** in memory, it grows as needed, and it frees itself. `std::vector<std::complex<float>>` is the standard container for IQ samples (Lesson 22).

**demo.cpp**
```cpp
#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <cstdio>
#include <numeric>
#include <vector>

// pass by value copies, const& does not, & lets the callee modify
void by_value(std::vector<float> v) { v[0] = 99; }
void by_ref(std::vector<float>& v)  { v[0] = 99; }
float energy(const std::vector<std::complex<float>>& x) {   // read-only, no copy
    float e = 0.f;
    for (const auto& s : x) e += std::norm(s);               // norm = |z|^2 (no sqrt)
    return e;
}
template <typename T>
T sum_squares(const std::vector<T>& v) {
    return std::accumulate(v.begin(), v.end(), T{0}, [](T acc, T s) { return acc + s * s; });
}
int main() {
    std::vector<float> a{1, 2, 3};
    by_value(a); std::printf("after by_value: a[0]=%.0f\n", a[0]);
    by_ref(a);   std::printf("after by_ref:   a[0]=%.0f\n", a[0]);

    std::vector<int> v; std::size_t last = 0;
    std::printf("capacity growth:");
    for (int i = 0; i < 40; ++i) { v.push_back(i); if (v.capacity() != last) { last = v.capacity(); std::printf(" %zu", last); } }
    std::printf("\n");
    std::vector<int> w(4, 1); const int* before = w.data(); w.push_back(2);
    std::printf("data() changed after push_back (reallocation): %s\n", before != w.data() ? "yes" : "no");
    std::vector<int> r; r.reserve(100); const int* p0 = r.data(); for (int i = 0; i < 100; ++i) r.push_back(i);
    std::printf("with reserve(100): pointer stable = %s, capacity=%zu\n", p0 == r.data() ? "yes" : "no", r.capacity());

    std::complex<float> z(3.f, 4.f), q(0.f, 1.f);
    std::printf("abs=%.1f norm=%.1f arg=%.4f conj=(%.0f,%.0f) z*q=(%.0f,%.0f)\n", std::abs(z), std::norm(z), std::arg(z), std::conj(z).real(), std::conj(z).imag(), (z*q).real(), (z*q).imag());
    std::vector<std::complex<float>> iq{{1, 1}, {0, 2}, {-3, 0}};
    const float* raw = reinterpret_cast<const float*>(iq.data());     // [re,im,re,im,...] layout is guaranteed
    std::printf("raw floats: %.0f %.0f %.0f %.0f %.0f %.0f ; energy=%.0f\n", raw[0], raw[1], raw[2], raw[3], raw[4], raw[5], energy(iq));
    std::vector<int> ints{1, 2, 3, 4};
    std::printf("sum_squares<int>=%d  sum_squares<double>=%.2f\n", sum_squares(ints), sum_squares(std::vector<double>{0.5, 1.5}));
    std::vector<float> m{3, 1, 2}; std::sort(m.begin(), m.end()); std::printf("sorted: %.0f %.0f %.0f\n", m[0], m[1], m[2]);
    return 0;
}
```
Real output:
```
after by_value: a[0]=1
after by_ref:   a[0]=99
capacity growth: 1 2 4 8 16 32 64
data() changed after push_back (reallocation): yes
with reserve(100): pointer stable = yes, capacity=100
abs=5.0 norm=25.0 arg=0.9273 conj=(3,-4) z*q=(-4,3)
raw floats: 1 1 0 2 -3 0 ; energy=15
sum_squares<int>=30  sum_squares<double>=2.50
sorted: 1 2 3
```
**What to take from it**
* **By value vs by reference:** `by_value` changed only its copy (`a[0]` stayed 1); `by_ref` modified the original (99). Copying a big sample buffer by accident is a silent performance bug.
* **Capacity growth:** `push_back` doubles capacity (1, 2, 4, …, 64 on this compiler); each growth **reallocates and moves** the data. `reserve(n)` allocates once — *verified* pointer stable with `reserve(100)`.
* **Reallocation invalidates pointers/iterators** (`data()` changed) — the root of `uaf.cpp`'s bug.
* **`std::complex<float>`:** `abs` = $\sqrt{re^2+im^2}=5$; **`norm` = $re^2+im^2=25$ (no square root)** — use `std::norm` for power (like `np.abs(x)**2` without the sqrt); `arg` = phase in radians (P02: $\arctan(4/3)=0.9273$); `conj`; `z*q` = $(3+4j)\cdot j=-4+3j$ ✓.
* **Layout guarantee:** a `std::complex<float>` array can be read as interleaved `float` pairs — `raw` printed `1 1 0 2 -3 0` = re,im,re,im,… — exactly the IQ file layout of P08 §8.3. (This is what Lesson 22 builds on when it compares AoS with SoA.)
* **Templates** (`template <typename T>`) write one function for many types: `sum_squares` worked for `int` and `double`. **Lambdas** (`[](T acc, T s){…}`) are small anonymous functions, and `std::accumulate`/`std::sort` are STL algorithms.
* `[]` does **no bounds checking** (like a raw pointer); `v.at(i)` does and throws — slower but safer when debugging.

---

## 10.7 Python/NumPy → C++ cheat-sheet 🔥

| NumPy / Python | C++ |
|---|---|
| `np.zeros(N, np.complex64)` | `std::vector<std::complex<float>> x(N);` |
| `x[i]`, `len(x)` | `x[i]`, `x.size()` |
| `x[a:b]` (a view) | `x.data() + a` with a length (pointer + size), or `std::span` (C++20) |
| `np.abs(x)**2` | `std::norm(x[i])` in a loop |
| `np.sum(v)` | `std::accumulate(v.begin(), v.end(), 0.0)` (note the **`0.0`** — see §10.9) |
| `np.conj(z)`, `np.angle(z)` | `std::conj(z)`, `std::arg(z)` |
| `x.astype(np.float32)` | `static_cast<float>(v)` |
| `np.fromfile(..., dtype=np.int16)` | `std::fread` into a `std::vector<int16_t>` |
| `time.perf_counter()` | `std::chrono::steady_clock::now()` |
| exception on a bad index | **silent undefined behaviour** (use ASan, `.at()`) |
| arrays copy on `copy()` only | `=` on a `std::vector` **copies the whole thing** |

---

## 10.8 "Compiled" does not automatically mean "fast" 🔥 (the point of Lesson 22)

Benchmark: energy $\sum|x|^{2}$ of **20 million `complex<float>`** samples, best of 5 runs, on this machine.

**bench2.cpp**
```cpp
#include <algorithm>
#include <chrono>
#include <complex>
#include <cstdio>
#include <vector>
template <typename F>
double best_ms(F f, int reps = 5) {
    double best = 1e9;
    for (int r = 0; r < reps; ++r) {
        auto t0 = std::chrono::steady_clock::now(); f();
        auto t1 = std::chrono::steady_clock::now();
        best = std::min(best, std::chrono::duration<double, std::milli>(t1 - t0).count());
    }
    return best;
}
int main() {
    const std::size_t N = 20'000'000;
    std::vector<std::complex<float>> x(N);
    for (std::size_t i = 0; i < N; ++i) x[i] = {float(i % 7) - 3.f, float(i % 5) - 2.f};
    float ef = 0; double ed = 0;
    double t_float  = best_ms([&] { float  a = 0.f; for (std::size_t i = 0; i < N; ++i) a += std::norm(x[i]); ef = a; });
    double t_double = best_ms([&] { double a = 0.0; for (std::size_t i = 0; i < N; ++i) a += (double)std::norm(x[i]); ed = a; });
    std::printf("float  accumulator: energy=%.0f  %.1f ms\n", ef, t_float);
    std::printf("double accumulator: energy=%.0f  %.1f ms\n", ed, t_double);
    return 0;
}
```
```bash
g++ -std=c++17 -O2 bench2.cpp -o b2 && ./b2
```
**Real results**

| Version | Result | Time |
|---|---|---|
| `-O0`, `float` accumulator | 116,647,576 | **138.4 ms** |
| `-O2`, `float` accumulator | 116,647,576 | 26.0 ms |
| `-O3`, `float` accumulator | 116,647,576 | 26.7 ms (no better than `-O2`) |
| `-O2`, **`double`** accumulator | **119,999,995** | 57.8 ms |
| `-O3 -march=native -ffast-math`, `float` | 120,000,000 | 14.6 ms |
| NumPy `np.sum(np.abs(x)**2)` | 120,000,000 | 47.0 ms |
| NumPy `np.vdot(x, x).real` | 120,000,000 | 15.8 ms |

*(Times are machine-dependent; the **ratios and the correctness lesson** are the point.)*

**Four lessons, all verified:**
1. **Unoptimised C++ (`-O0`) is slower than NumPy** here (138 vs 47 ms) — a literal line-by-line port with no optimisation is *not* automatically faster. `-O2` made it ~5× faster than `-O0`.
2. **The "fast" `float` result is wrong.** The exact energy is **119,999,995** (computed independently with 64-bit integers). A sequential `float` accumulator reached only 116,647,576 — **2.8 % too low** — because once the running sum is ~10⁸, adding small terms (≤ 18) loses low-order bits (P08 §8.5). The `double` accumulator is exact; NumPy's pairwise float32 sum is within 4×10⁻⁶ %. **Correctness first: accumulate in `double`.**
3. **`-O3` ≠ automatically faster than `-O2`:** the strict left-to-right floating-point reduction can't be vectorised, so both took ~26 ms. `-ffast-math` lets the compiler **reorder** additions, which enables vectorisation (14.6 ms) *and* happened to give a better sum here — but it changes floating-point semantics (e.g. NaN handling), so use it deliberately. `[FOUNDATION ADDED — general compiler behaviour; Lessons 22 and 26 take this further]`
4. **A library routine (`np.vdot`) can beat a hand-written loop** — measure before assuming.

---

## 10.9 Build tools and debugging

**Single file:** `g++ -std=c++17 -O2 -Wall -Wextra -g prog.cpp -o prog`.
**A minimal `Makefile`** (verified with `make`; recipe lines **must start with a tab**):
```make
CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -g

demo: demo.cpp
	$(CXX) $(CXXFLAGS) demo.cpp -o demo

clean:
	rm -f demo
```
`make` rebuilds only what changed. **CMake** is a higher-level build generator common in larger projects — *it was not installed in my environment, so I did not verify any CMake example; learn it when a project needs it.* Likewise **gdb**, **Valgrind** and **clang** were not available to me; the lessons mention Valgrind and `perf`, so install them on your own machine when you get there.

**Debugging toolkit (all verified above except where noted):** compile with `-Wall -Wextra` and read *every* warning; run with `-fsanitize=address,undefined` (you can combine them) during development; use `assert` and `printf`; read compiler errors from the *first* one downward (later ones are often consequences).

---

## 10.10 Common mistakes ⚠️
1. **Ignoring warnings** (`-Wsign-compare` caught the `-1 < 1u` trap).
2. **Passing big containers by value** instead of `const&`.
3. **Using `==`/`<` between signed and unsigned**, or mixing `int` with `size_t` indices.
4. **Integer division** where you wanted floating point (`1/2 = 0`).
5. **Signed overflow** (UB) assumed to wrap like NumPy.
6. **Dangling pointers** after `push_back`/`resize` — keep indices, not pointers, or `reserve` first.
7. **Off-by-one on array bounds** (silent without ASan).
8. **Accumulating in `float`** over long streams (§10.8).
9. **Benchmarking at `-O0`**, or letting the optimiser delete your measured code (use the result, as `bench2.cpp` does with `ef`/`ed`).
10. **Using `std::abs` where `std::norm` suffices** (extra sqrt).
11. **Reading `[]` as bounds-checked.**
12. **Forgetting `#include`s** (`<cstdint>`, `<complex>`, `<vector>`, `<cstdio>`).

---

## 10.11 Practice problems (answers at the end)

1. How many bytes do 5×10⁶ samples occupy as `std::complex<float>`? As `std::complex<double>`? As `int16_t` I/Q pairs?
2. What does `uint8_t x = 250; x += 10;` leave in `x`? What does `int x = INT_MAX; x += 1;` do, formally?
3. Why is `-1 < 1u` false? What flag warns you?
4. For `z = 3 + 4j` give `std::abs(z)` and `std::norm(z)`. Which one do you use to compute power?
5. Predict the capacity sequence printed if you `push_back` 40 elements one at a time (g++ typical). Why might it differ on another compiler?
6. Which one-line change prevents `uaf.cpp`'s bug? Give two different fixes.
7. Classify each as heap-buffer-overflow, use-after-free or leak: (a) `delete[] p; p[0]=1;` (b) `p = new int[3]; p = nullptr;` (c) writing `v[v.size()]` on a `std::vector`.
8. Translate `y = np.abs(x)**2` (x a `complex64` array) into a C++ loop; give the output type.
9. Write the compile command for a program you want to test for memory errors, and another for maximum speed (no sanitizers).
10. Why did the `-O2` `float` energy equal 116,647,576 instead of 119,999,995? Give two fixes.
11. Why is `-O3` not faster than `-O2` in the benchmark, and what flag allowed vectorisation?
12. *Think:* a colleague "optimised" a Python loop by porting it line-by-line to C++ and compiled without `-O`. It got slower than NumPy. Explain.

## 10.12 Active recall
1. What are the three stages of building a C++ program? 2. List the fixed-width types and `sizeof(std::complex<float>)`. 3. State three conversion pitfalls. 4. What is undefined behaviour? Give an example. 5. By value vs `&` vs `const&`. 6. Stack vs heap. 7. What is RAII? 8. What do ASan and UBSan detect? 9. Why does `push_back` invalidate pointers? 10. `abs` vs `norm`. 11. Why can a `float` accumulator give a wrong sum? 12. What do `-O0`, `-O2`, `-ffast-math` do?

## 10.13 Self-check — ready for P11 if you can…
- [ ] compile, run and debug a small program with `-Wall -Wextra`;
- [ ] read and write a function taking `const std::vector<std::complex<float>>&`;
- [ ] explain stack vs heap, pointers and RAII;
- [ ] reproduce a heap-overflow and a use-after-free under ASan and read the report;
- [ ] explain why the benchmark's `float` result was wrong and fix it;
- [ ] use the Python → C++ cheat-sheet.

---

## ✅ Answer key

1. `complex<float>`: 5×10⁶ × 8 = **40 MB**; `complex<double>`: **80 MB**; `int16_t` pairs: 5×10⁶ × 4 = **20 MB**. 2. `x` becomes **4** (unsigned wrap-around, well-defined). `INT_MAX + 1` is **undefined behaviour** (signed overflow). 3. The −1 is converted to an unsigned value (4,294,967,295) before comparison; `-Wall -Wextra` gives `-Wsign-compare`. 4. `abs = 5`, `norm = 25`; use **`norm`** for power (no sqrt). 5. `1 2 4 8 16 32 64` on g++ here; the growth factor (doubling vs 1.5×) is **implementation-defined**, so other standard libraries differ. 6. Fixes: (a) `v.reserve(...)` before taking the pointer so no reallocation occurs; (b) re-read `v.data()` after `push_back` (or use an index `v[0]` instead of a stored pointer). 7. (a) **use-after-free**; (b) **leak** (the allocated memory is no longer reachable); (c) **heap-buffer-overflow** (one past the end). 8. `std::vector<float> y(N); for (std::size_t i = 0; i < N; ++i) y[i] = std::norm(x[i]);` — output type `float`. 9. Memory check: `g++ -std=c++17 -g -O1 -fsanitize=address,undefined prog.cpp -o prog`; speed: `g++ -std=c++17 -O3 -march=native prog.cpp -o prog` (add `-ffast-math` only if you accept changed FP semantics). 10. Sequential `float` accumulation loses low-order bits when the sum is ~10⁸ and the terms are small; fixes: **accumulate in `double`**, or use pairwise/blocked partial sums (several accumulators). 11. The strictly ordered floating-point reduction cannot be vectorised without reordering; `-ffast-math` permits reordering. 12. At `-O0` the compiler does no optimisation (variables kept in memory, no inlining/vectorisation), so the line-by-line port ran ~5× slower than `-O2` and slower than NumPy's vectorised C loops; the fix is to build with `-O2`/`-O3` (and think about layout and precision, Lesson 22).

---

*Next: **P11 — Systems baseline: OS, Linux, concurrency and networking** (processes vs threads, scheduling, interrupts, the memory hierarchy, data races and locks, the Linux command line, layered protocols and packet headers, byte order) — the vocabulary for Lessons 24, 27, 33 and the Linux chapter.*
