# 12 — C++ Performance & Memory Behaviour

> Useful for contests (why your O(n log n) is slower than someone else's O(n log n))
> and for interviews (why asymptotic complexity is not the whole story).

---

## The memory hierarchy, in one table

| Level | Size | Latency (cycles) | Latency (ns) |
|---|---|---|---|
| Register | ~100 B | 0 | 0 |
| L1 cache | 32–64 KB | ~4 | ~1 |
| L2 cache | 256 KB–1 MB | ~12 | ~4 |
| L3 cache | 8–32 MB | ~40 | ~15 |
| Main memory | GBs | ~200–300 | ~80 |

**One RAM access ≈ 50–100 L1 accesses.** That ratio is the entire reason "cache-friendly" matters.

**Cache line = 64 bytes.** Memory moves in 64-byte blocks, never single bytes. A cache line holds **16 ints** or **8 long longs**. Touching `a[i]` brings in `a[i..i+15]` for free.

**Consequence:** a sequential pass over an `int` array costs one cache miss per 16 elements. A random-access pass costs one miss per element. That's a **16× difference** on identical asymptotic complexity.

---

## Row-major traversal

C++ stores 2D arrays **row-major**: `a[i][j]` and `a[i][j+1]` are adjacent in memory.

```cpp
// FAST — sequential, one miss per 16 elements
for (int i = 0; i < n; i++)
    for (int j = 0; j < n; j++)
        sum += a[i][j];

// SLOW — strides by n*4 bytes, one miss per element
for (int j = 0; j < n; j++)
    for (int i = 0; i < n; i++)
        sum += a[i][j];
```
For n = 2000, the column-major version is typically **5–10× slower**. Same instruction count, same complexity.

**Memory hook:** *the innermost loop should vary the last index.*

**This is why matrix multiplication is written `i, k, j`:**
```cpp
for (int i = 0; i < n; i++)
  for (int k = 0; k < n; k++) {
      int aik = A[i][k];
      for (int j = 0; j < n; j++)
          C[i][j] += aik * B[k][j];     // both C and B traverse rows
  }
```
The naive `i, j, k` order strides down a column of `B`. Swapping to `i, k, j` is a one-line 3–5× speedup.

---

## Flat arrays beat nested vectors

```cpp
vector<vector<int>> g(n, vector<int>(m));   // n separate heap blocks, 24 B header each
vector<int> g(n * m);                        // ONE block, fully contiguous
// index with g[i * m + j]
```
The nested version has: n allocations, n extra pointer dereferences per access pattern, rows scattered across memory, and 24n bytes of headers. For an n×m grid you traverse repeatedly, flattening is typically **1.5–3×** faster.

**When nested is fine:** ragged data (adjacency lists with wildly different degrees), or when you don't traverse it hot.

---

## `vector` vs `deque` vs `list` — the locality view

| | Layout | Traversal | Random access | Insert middle |
|---|---|---|---|---|
| `vector` | one contiguous block | **perfect** | O(1), one miss/16 | O(n) memmove — but memmove is very fast |
| `deque` | chunks (~512 B) + index array | good within a chunk, one indirection per chunk | O(1) + indirection | O(n) |
| `list` | one heap node per element, doubly linked | **terrible** — pointer chase, one miss per element | O(n) | O(1) *if you already have the iterator* |

`list<int>` node: 4 bytes of payload, 16 bytes of pointers, plus ~16 bytes of allocator header = **~40 bytes for a 4-byte int**. Ten times the memory and no locality.

---

## The vector-vs-list interview answer

> **"Insertion in the middle of a list is O(1) and in a vector is O(n). So list is faster, right?"**

**Short answer:** No. `vector` usually wins, often by 5–50×, even for insert-heavy workloads.

**Deep answer — four reasons:**

1. **You have to find the position first.** `list::insert` is O(1) *given an iterator*, but getting that iterator is an O(n) **pointer-chasing traversal** with a cache miss at every step. `vector` finds the position with an O(n) **sequential scan** that the prefetcher handles for free. Both are O(n); the constants differ by ~10–50×.

2. **`vector`'s O(n) shift is a `memmove`.** Moving 1000 ints is one SIMD-accelerated block copy — a few hundred nanoseconds. Traversing 1000 list nodes is 1000 dependent loads, each potentially a ~80 ns cache miss.

3. **`list` allocates per element.** Every `push_back` is a `malloc`. `vector` amortizes to roughly log₂(n) allocations total.

4. **`list` destroys the prefetcher.** The CPU can't predict the next node's address until it has loaded the current one — a serialized dependency chain. Sequential array access is prefetched ahead of time.

**Where `list` actually wins:**
- Elements are large and expensive to move (though `vector` of `unique_ptr` or indices usually beats it anyway).
- You hold many stable iterators/references that must survive insertions (`list` never invalidates others; `vector` invalidates everything on reallocation).
- You splice whole ranges between containers in O(1).

**Common wrong answer in interviews:** quoting the complexity table without mentioning locality. The table is correct and the conclusion is wrong.

**The real lesson:** *asymptotic complexity counts operations; performance is determined by the cost of each operation, and that cost varies by two orders of magnitude depending on where the data lives.*

---

## `vector` growth and amortized O(1)

When `push_back` exceeds capacity, `vector`:
1. allocates a new block of `2 × capacity` (libstdc++; MSVC uses 1.5×),
2. moves/copies all existing elements,
3. destroys the old ones and frees the old block.

**Why geometric growth gives amortized O(1):**
Starting from capacity 1 and doubling, inserting n elements copies `1 + 2 + 4 + … + n/2 < n` elements total. So n insertions cost O(n) total work → **O(1) amortized each**.

If it grew by a constant (say +10), inserting n elements would copy `10 + 20 + … + n` = O(n²) → O(n) amortized. Geometric growth is what makes the whole thing work.

**Why 2× vs 1.5×:** with a growth factor ≥ 2, freed blocks can never be reused for the next allocation (1+2+4 < 8), so the allocator fragments. Factor < 2 (golden ratio ≈ 1.618 is the theoretical sweet spot) allows reuse. libstdc++ chose 2 anyway.

**`reserve` eliminates all of this:**
```cpp
vector<int> v;
v.reserve(1'000'000);            // one allocation, zero moves
for (int i = 0; i < 1'000'000; i++) v.push_back(i);
```
Typically a 2–3× speedup on the fill loop, and it eliminates the memory spike (during reallocation you briefly hold **3× the data**: old block + new block).

**What happens to pointers and iterators after reallocation:** all of them dangle. Using them is UB. This is the #1 `vector` bug.

---

## `size` vs `capacity`

```cpp
v.size()       // how many elements exist
v.capacity()   // how many fit before the next reallocation
v.reserve(k)   // capacity >= k, size unchanged, no elements constructed
v.resize(k)    // size == k, new elements value-initialized (zeros for int)
v.clear()      // size = 0, capacity UNCHANGED (memory not returned)
v.shrink_to_fit()  // non-binding request to release the slack
vector<int>().swap(v);   // the guaranteed way to actually free it
```
**Trap:** `clear()` does not free memory. Clearing a 1e7-element vector in a loop leaves 40 MB held.

---

## Stack vs heap

| | Stack | Heap |
|---|---|---|
| Allocation | move a pointer — ~1 ns | `malloc` — ~50–100 ns |
| Deallocation | automatic on scope exit | `free`, or destructor |
| Size | limited (8 MB typical; on CF = memory limit) | limited by the memory limit |
| Locality | excellent — always hot in cache | depends |
| Lifetime | scope-bound | manual / RAII |

```cpp
int a[1000];              // stack — instant
vector<int> v(1000);      // heap allocation + zeroing
static int g[1000000];    // BSS, zero-filled by the loader, free
```

**Contest implication:** a **global** array is the fastest option — no allocation, zero-initialized for free, and its address is known at compile time. This is why competitive programmers still use `int a[200005];` at global scope instead of `vector`.

**Trap:** a large **local** array is on the stack. `int a[1'000'000]` inside a function = 4 MB of stack. Often fine on CF, but it's the classic "works locally, RE on the judge" bug on other judges.

---

## Allocation overhead

Every `new`/`malloc` costs ~50–100 ns plus a header (8–16 bytes). Implications:
- `list<int>` / `map<int,int>` / `set<int>` do one allocation per element.
- `vector<vector<int>>` does one per row.
- Allocating inside a hot loop is a common hidden TLE.

**Fix:** preallocate. A `vector<int>` reused across iterations with `.clear()` (which keeps capacity) allocates once.

**Memory fragmentation:** many small allocations of varying sizes leave unusable gaps. In a contest this rarely bites (the process is short-lived), but it's a real interview topic — it's why arena/pool allocators exist.

---

## Branch prediction

Modern CPUs speculatively execute ~15–20 instructions ahead. A mispredicted branch flushes the pipeline: **~15–20 cycles wasted**.

```cpp
// Sorting the array first makes this loop ~3-6x faster
for (int i = 0; i < n; i++)
    if (a[i] >= 128) sum += a[i];
```
On random data the branch is unpredictable (~50% miss). On sorted data it's a long run of "no" then a long run of "yes" — nearly perfectly predicted. **Same instructions, same complexity, several times the speed.** This is the famous StackOverflow "why is processing a sorted array faster" question.

**Branchless alternatives:**
```cpp
sum += a[i] * (a[i] >= 128);            // no branch
x = (a < b) ? a : b;                     // compiler usually emits a cmov
```
The compiler already does this for simple `min`/`max`. Hand-writing branchless code is rarely worth it in a contest; know the concept for interviews.

**Where it matters in contests:** a tight inner loop with an unpredictable condition over 1e8 iterations. Rare, but it's why some identical-complexity solutions are 3× apart.

---

## Other real costs

| Operation | Approximate cost |
|---|---|
| int add/sub/bit op | 1 cycle |
| int multiply | 3–5 cycles |
| int divide / modulo | **20–40 cycles** |
| float add/mul | 4–5 cycles |
| float divide | 15–20 cycles |
| `sqrt` | ~15 cycles |
| L1 hit | 4 cycles |
| RAM access | 200+ cycles |
| branch misprediction | 15–20 cycles |
| function call (not inlined) | ~5 cycles + register pressure |
| `malloc` | 50–100 ns |

**Practical takeaways:**
- **Modulo is expensive.** In a loop doing 1e8 `% MOD`, replacing it with a conditional subtract (`if (x >= MOD) x -= MOD;`) can be 2× faster.
- **Division by a compile-time constant is optimized** into a multiply + shift. Division by a runtime variable is not.
- `x / 2` on a signed int is slower than `x >> 1` because of the sign rounding. On unsigned they're identical.

---

## Why recursion can be slower than iteration

- Each call pushes a stack frame: return address, saved registers, locals.
- The compiler may not be able to keep values in registers across the call.
- Call/return are branches (usually well-predicted, but not free).
- Deep recursion blows the cache with stack frames.
- No tail-call optimization guarantee in C++ (GCC does it at `-O2` for simple cases).

Typical penalty: **2–3×** for memoized DP vs an iterative version. Worth knowing when you're 20% over the time limit.

**But:** recursion is far easier to get right for tree/graph traversal and for DP where the iteration order is subtle. Don't convert unless you need the speed.

---

## Why `unordered_map` loses to array indexing

```cpp
unordered_map<int,int> cnt;   // hash, modulo by bucket count, pointer chase to the node
int cnt[1'000'001] = {};      // one indexed load
```
The array is **10–100× faster**. The hash map does: hash the key (a few cycles), modulo by bucket count (20–40 cycles), load the bucket pointer (cache miss), follow the chain (cache miss), compare the key. The array does one load from a predictable address.

**Rule: if your keys are integers bounded by ~1e7, use an array.** Even a mostly-empty 4 MB array beats a hash map with 1e5 entries.

Memory tradeoff: `int cnt[1e7]` is 40 MB. Fine under a 256 MB limit; check before committing.

---

## False sharing (interview topic)

Two threads writing to *different variables that share a cache line* force that line to ping-pong between cores. Each write invalidates the other core's copy → performance collapses even though there's no logical contention.

**Fix:** pad each thread's data to 64 bytes (`alignas(64)`).

Irrelevant in single-threaded contests. A standard question in low-latency/quant interviews.

---

## Memory alignment and padding

```cpp
struct A { char c; int i; char d; };     // sizeof = 12, not 6
struct B { int i; char c; char d; };     // sizeof = 8
```
Each member must sit at an address that is a multiple of its alignment (`int` → 4, `double`/pointer → 8). The compiler inserts padding, and the struct's total size is rounded up to its largest member's alignment.

**Rule: declare members largest-first** to minimize padding. At 1e6 structs, 12 vs 8 bytes is 4 MB.

`alignof(T)`, `alignas(N)`, `#pragma pack(1)` (disables padding — makes access slower, and misaligned access is UB on some architectures).

---

## `sizeof(vector<int>)`

**24 bytes on x86-64 libstdc++**, regardless of how many elements it holds. Three pointers:
```
_M_start          → first element
_M_finish         → one past the last  (size = finish - start)
_M_end_of_storage → one past the capacity
```
The **elements are on the heap**. `sizeof` measures only the header object.

A `vector<vector<int>>` with 1000 rows has 24 KB of headers plus the rows' data — and each row is a separate heap allocation somewhere else in memory.

---

## Practical contest optimization checklist

**Do these first (algorithmic):**
```
[ ] is the complexity actually right?
[ ] replace map/set with an array or a sorted vector
[ ] replace unordered_map with an array
[ ] avoid recomputing inside loops
```
**Then (constant factor):**
```
[ ] fast I/O
[ ] reserve() before a big fill
[ ] const auto& in range-for over non-trivial types
[ ] flatten vector<vector<T>> into a 1D vector
[ ] check loop order for row-major traversal
[ ] conditional subtract instead of % in hot modular loops
[ ] int instead of long long where safe
[ ] bitset if the inner loop is boolean
```
**Last resort:**
```
[ ] #pragma GCC optimize / target
```