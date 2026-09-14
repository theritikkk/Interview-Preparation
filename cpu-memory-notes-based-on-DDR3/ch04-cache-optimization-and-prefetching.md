# CPU Memory Notes 4 — Bypassing the Cache, Optimizing Cache Access, and Prefetching

> Detailed notes on Ulrich Drepper's *What Every Programmer Should Know About Memory*, Ch6.1-6.3 (Bypassing the Cache, Cache Access optimization, Prefetching) -- the beginning of the paper's central "What Programmers Can Do" chapter. See `00-index.md` for the full series map.

---

## 6.1 Bypassing the Cache -- Non-Temporal Writes

⭐ **The problem this section solves:** an ordinary memory store instruction, if the target cache line isn't already present, first **reads the entire cache line**, then modifies it -- even though the program is *purely writing*, never reading, that data. For data that's **produced but not soon reused** (the paper's example: filling a large matrix that will only be read much later), this read-before-write behavior actively **evicts other useful data** from cache for no benefit -- the matrix fill itself gains nothing from caching, since by the time the last element is written, the first elements have long since been evicted anyway.

⭐ **The fix: non-temporal store instructions.** "Non-temporal" signals *"this data won't be reused soon, don't bother caching it."* These write **directly to memory**, skipping the read-modify step, and where possible use **write-combining** (recall notes file 2) to batch several word-writes into one full cache-line transfer -- avoiding both the unnecessary read *and* the cache pollution, in one move.

### The GCC Intrinsics
```c
#include <emmintrin.h>
void _mm_stream_si32(int *p, int a);
void _mm_stream_si128(int *p, __m128i a);
void _mm_stream_pd(double *p, __m128d a);
#include <xmmintrin.h>
void _mm_stream_pi(__m64 *p, __m64 a);
void _mm_stream_ps(float *p, __m128 a);
```
⭐ **Practical requirement:** the target address must be **aligned** (8 or 16 bytes, depending on the variant), and -- crucially -- the write-combining hardware buffer can only hold a partial cache-line write open for so long, so all instructions modifying a *single* cache line should be issued **back-to-back**, or the combining won't actually happen. Worked example -- filling an entire cache line with a constant byte:
```c
#include <emmintrin.h>
void setbytes(char *p, int c) {
    __m128i i = _mm_set_epi8(c,c,c,c, c,c,c,c, c,c,c,c, c,c,c,c);
    _mm_stream_si128((__m128i *)&p[0],  i);
    _mm_stream_si128((__m128i *)&p[16], i);
    _mm_stream_si128((__m128i *)&p[32], i);
    _mm_stream_si128((__m128i *)&p[48], i);
}
```
⭐ **This is exactly the technique real-world `memset()` implementations use for large blocks** -- a concrete, everyday example of non-temporal stores paying off in practice.

### Measured Payoff -- the Matrix Initialization Benchmark
Initializing a 3000x3000 matrix, comparing normal (cached) stores vs. non-temporal stores, row-major vs. column-major inner loop:

| Inner loop increment | Normal | Non-Temporal |
|---|---|---|
| Row (sequential) | 0.048s (~750MB/s) | 0.048s |
| Column (strided) | 0.127s (~280MB/s) | 0.160s |

⭐ **The genuinely surprising result:** for **sequential** access, non-temporal stores perform **identically** to normal cached stores -- because write-combining fully compensates, *and* non-temporal writes have **relaxed memory-ordering rules** (requiring explicit `sfence` barriers when ordering matters), giving the processor more freedom to schedule the actual writes optimally. For **strided/column-wise** access, though, non-temporal stores are actually **worse** than normal writes -- no write-combining is possible when each write touches a different, unrelated row, so every store pays the full RAM row-selection cost individually.

### Non-Temporal Reads (a newer, more limited capability)
⭐ Unlike writes, there historically was **no read equivalent to write-combining** -- a real gap, especially painful for uncacheable memory-mapped I/O. **Intel's SSE4.1 `_mm_stream_load_si128` (movntdqa)** addresses this via a small number of dedicated **streaming load buffers**: the first load of a cache line fills a buffer; subsequent aligned accesses to the *same* line are served cheaply from that buffer, without ever populating the normal cache hierarchy -- letting large data be read once without polluting cache.

> **Practical takeaway:** modern CPUs optimize sequential uncached access (both write, and increasingly read) very well -- but **random access still pays a real cost** (the paper measures ~70% slower for random vs. sequential in this section's tests) that caching can only partially hide.

---

## 6.2 Cache Access -- Optimizing Level 1 Data Cache Access

⭐ **Framing:** L1d optimizations matter most, since they're closest to the CPU and every technique here also benefits higher cache levels. The unifying theme, stated directly: **"improve locality (spatial and temporal), and align the code and data."**

### The Worked Example: Matrix Multiplication
```c
// naive implementation
for (i = 0; i < N; ++i)
  for (j = 0; j < N; ++j)
    for (k = 0; k < N; ++k)
      res[i][j] += mul1[i][k] * mul2[k][j];
```
⭐ **Diagnosing the problem:** `mul1` is accessed **row-wise (sequentially)** -- good, matches memory layout. But `mul2` is accessed **column-wise** in the inner loop -- each successive access jumps a full row's width in memory, touching a **new cache line every single time**, and by the time the inner loop finishes one pass, `mul2`'s early cache lines have long since been evicted (1000x1000 doubles at 64B/line needs far more than a typical 32KB L1d).

### Optimization 1 -- Transpose First
```c
double tmp[N][N];
for (i = 0; i < N; ++i)
  for (j = 0; j < N; ++j)
    tmp[i][j] = mul2[j][i];
for (i = 0; i < N; ++i)
  for (j = 0; j < N; ++j)
    for (k = 0; k < N; ++k)
      res[i][j] += mul1[i][k] * tmp[j][k];
```
⭐ **Mathematical justification:** matrix multiplication only requires that every product term appears exactly once -- the *order* of accumulation doesn't matter (ignoring floating-point rounding effects), so transposing `mul2` up front, then reading it **row-wise** (now matching its transposed layout) in the inner loop, is mathematically equivalent but far more cache-friendly.

**Measured result (Intel Core 2, 2666MHz, 1000x1000 doubles):**
| | Original | Transposed |
|---|---|---|
| Cycles | 16,765,297,870 | 3,922,373,010 |
| Relative | 100% | **23.4%** |

⭐ A **76.6% speedup** from one transformation -- the extra copy operation is more than paid for by eliminating 1000 non-sequential accesses per column.

### Optimization 2 -- Sub-Matrix Blocking ("Tiling"), Avoiding the Copy Entirely
⭐ **Motivation:** the transpose approach needs extra memory and an up-front copy pass -- not always affordable (very large matrices, limited memory). **The real fix works directly on cache-line granularity, not on the whole matrix.**

**Key insight:** if the *middle* loop is unrolled to process multiple `j` values using data from the **same already-loaded cache line**, the L1d miss rate for `mul2` drops proportionally. With `sizeof(double)=8` and a 64-byte cache line, a line holds 8 doubles -- so the middle loop should ideally be unrolled **8-fold** to fully exploit one loaded line; extending this reasoning to the output matrix `res` too motivates unrolling the **outer** loop 8-fold as well.

```c
#define SM (CLS / sizeof(double))   // CLS = cache line size, e.g. 64
for (i = 0; i < N; i += SM)
  for (j = 0; j < N; j += SM)
    for (k = 0; k < N; k += SM)
      for (i2 = 0, rres = &res[i][j], rmul1 = &mul1[i][k]; i2 < SM;
           ++i2, rres += N, rmul1 += N)
        for (k2 = 0, rmul2 = &mul2[k][j]; k2 < SM; ++k2, rmul2 += N)
          for (j2 = 0; j2 < SM; ++j2)
            rres[j2] += rmul1[k2] * rmul2[j2];
```
⭐ **What's really happening:** the outer three loops carve the full multiplication into small `SM x SM` **sub-block** problems, each of which fits comfortably in L1d; the inner three loops do the actual multiply-accumulate work within one such block, with **pointer variables** (`rres`, `rmul1`, `rmul2`) manually hoisted out to help the compiler avoid recomputing array-indexing arithmetic (a real limitation the paper notes: C/C++'s default aliasing rules make this optimization hard for the compiler to do safely on its own, unlike Fortran, which has stronger no-aliasing guarantees baked into the language).

⭐ **Getting the cache line size right, portably:**
```bash
gcc -DCLS=$(getconf LEVEL1_DCACHE_LINESIZE) ...
```
or at runtime: `sysconf(_SC_LEVEL1_DCACHE_LINESIZE)`. For binaries that must run on varied hardware, the paper recommends using the **largest** likely cache line size -- worst case, some L1d capacity goes unused on smaller-line machines, which is a far smaller cost than getting it wrong on the common case.

### Optimization 3 -- Vectorization
⭐ **SIMD (Single Instruction, Multiple Data):** modern CPUs can process 2, 4, 8+ values in one instruction (SSE2 handles two `double`s at once). Applying SSE2 intrinsics on top of the sub-matrix-blocked version adds a further speedup.

**Full results across all three optimizations:**
| | Original | Transposed | Sub-Matrix | Vectorized |
|---|---|---|---|---|
| Cycles | 16,765,297,870 | 3,922,373,010 | 2,895,041,480 | 1,588,711,750 |
| Relative | 100% | 23.4% | **17.3%** | **9.47%** |

⭐ **In human terms: 318 MFLOPS -> 3.35 GFLOPS**, roughly a **10x** improvement, achieved purely through **memory-access-pattern optimization plus vectorization** -- no algorithmic change, same asymptotic complexity, same single-threaded execution. ⭐ **The paper's own honest caveat:** even this final version still has a residual cache weakness with `mul2` that prefetching **cannot** fix without an actual transpose -- the sub-matrix approach reduces the *damage*, but doesn't eliminate the underlying access-pattern mismatch entirely.

### Struct Layout -- the `pahole` Tool
⭐ **Beyond loop restructuring, data structure *layout itself* affects cache efficiency** -- the paper introduces `pahole`, a tool that inspects a compiled binary's struct layouts to reveal padding/alignment waste:
```c
struct foo {
    int a;
    long fill[7];
    int b;
};
```
`pahole`'s output:
```
struct foo {
    int          a;              /*    0    4 */
    /* XXX 4 bytes hole, try to pack */
    long int     fill[7];        /*    8   56 */
    /* --- cacheline 1 boundary (64 bytes) --- */
    int          b;              /*   64    4 */
}; /* size: 72, cachelines: 2 */
```
⭐ This immediately reveals: a **4-byte padding hole** after `a` (alignment requirement of `long`), and -- more importantly -- the struct spans **two cache lines**, with `b` isolated alone on the second one. If `a` and `b` are frequently accessed *together*, this layout forces **two** cache-line loads for what could, with better field ordering, fit in one.

**The paper's two concrete rules of thumb for struct layout:**
1. **Move the field most likely to be the "critical word"** (the one actually needed first/most) **to the beginning of the structure.**
2. **When access order isn't dictated by the algorithm, access fields in the order they're declared** -- matching declaration order to access order helps the prefetcher and minimizes the number of distinct cache lines touched per logical operation.

### Measured Effect of Spreading Data Across Cache Lines
⭐ A further controlled experiment: accessing two fields of the same list element, varying whether they land in the **same** cache line or **different, possibly distant** cache lines. Results, once the working set exceeds L1d:
- Same cache line: no penalty (obviously -- one line load covers both).
- Split across 2 adjacent lines (sequential layout): **~17%** penalty once L2-resident, **~27%** once main-memory-resident.
- **Random** placement across 2-4 lines: **25-35%** penalty for L2-resident working sets -- worse than the sequential-adjacent case, because there's no guarantee the prefetcher can even recognize a pattern across genuinely scattered lines.

---

## 6.3 Prefetching

⭐ **The purpose:** hide memory latency. Out-of-order execution and instruction pipelining can hide *some* latency, but only for accesses that **hit cache** -- covering a full main-memory-latency stall this way would require an impractically deep instruction queue. Prefetching instead tries to have the needed data **already in cache by the time it's actually requested.**

### Hardware Prefetching
⭐ **Trigger condition:** the CPU watches for **two or more cache misses in a recognizable pattern** (adjacent lines, or a fixed stride -- older CPUs only recognized strictly adjacent misses; modern ones recognize constant strides too) before it starts prefetching -- a **single** cache miss is deliberately **not** enough to trigger it, since most single misses (e.g. touching a scattered global variable) are one-offs that would just waste FSB bandwidth if chased.
- Modern CPUs can track **8-16 separate streams simultaneously** for higher-level caches -- though this pool is **shared** across all cores/hyper-threads using that cache level, so it's effectively divided further in a busy multi-core system.
- ⭐ **The hard, unavoidable limitation: prefetching cannot cross page boundaries.** Doing so would risk triggering a full OS page-fault/demand-paging event purely as an unintended side effect of speculative hardware behavior -- potentially for a page the program would *never* actually have touched. This means **every page boundary crossing incurs at least one guaranteed cache miss**, no matter how good the prefetcher is -- reinforcing, yet again, why minimizing a working set's total footprint (fewer pages touched) is valuable independent of raw cache-line-level locality.
- **Programmer-visible consequence:** if profiling reveals hardware prefetching is *hurting* a workload (fetching data that pollutes cache without being used), it can be **disabled** via a model-specific register (Intel: `IA32_MISC_ENABLE`) -- a privileged operation, typically requiring kernel-level access.

### Software Prefetching
⭐ **When hardware prefetching isn't enough (non-obvious access patterns, or when the programmer knows more than the automatic pattern detector can infer):** explicit prefetch instructions, exposed via intrinsics like `_mm_prefetch`, with several distinct **hint levels**:

| Hint | Target |
|---|---|
| `_MM_HINT_T0` | All cache levels (inclusive caches) / lowest level (exclusive caches) -- use when data will be needed **immediately** |
| `_MM_HINT_T1` | L2 only, not L1d |
| `_MM_HINT_T2` | L3 (where present) |
| `_MM_HINT_NTA` | Non-temporal -- avoid cache pollution, matching the "won't be reused soon" semantics from section 6.1 |

⭐ **The key practical caveat:** `_MM_HINT_T0` is only the right choice if L1d is actually large enough to **hold** everything being prefetched this way without evicting other still-needed data -- if the immediately-used working set is too big, `T1`/`T2` (targeting a higher, larger cache level) are the more appropriate choice. Choosing the wrong hint can actively make things worse by evicting genuinely useful data to make room for speculatively-prefetched data.

---

## Summary Table

| Technique | Solves | Cost / caveat |
|---|---|---|
| **Non-temporal stores** | Avoids read-before-write + cache pollution for write-once data | Requires alignment + back-to-back writes to one line for write-combining to actually trigger; worse than normal stores for strided/random access |
| **Matrix transpose** | Converts column-wise access into row-wise (sequential) access | Needs extra memory and an up-front copy pass |
| **Sub-matrix blocking (tiling)** | Keeps working set within L1d without any extra copy | More complex code; needs the correct cache-line-size constant baked in |
| **Vectorization (SIMD)** | Processes multiple values per instruction | Requires intrinsics or a vectorizing compiler; independent of, and stacks with, the above |
| **`pahole` / struct layout rules** | Minimizes cache lines touched per struct access | Needs deliberate field ordering by access frequency, not just logical grouping |
| **Hardware prefetching** | Automatic latency hiding for simple, recognized patterns | Can't cross page boundaries; can be counter-productive for irregular access -- disable via MSR if so |
| **Software prefetching (T0/T1/T2/NTA hints)** | Explicit latency hiding for patterns hardware can't infer | Wrong hint level (e.g. T0 with insufficient L1d) can actively hurt by evicting needed data |

## Key Talking Points

- Walk through the **matrix multiplication optimization progression** from memory: naive (100%) -> transposed (23.4%) -> sub-matrix blocked (17.3%) -> vectorized (9.47%) -- and explain *why* each step helps, not just the numbers.
- Explain **why non-temporal stores perform identically to cached stores for sequential access but worse for strided access** -- tie this directly to write-combining's dependence on spatial locality.
- Explain the **sub-matrix blocking loop structure**, including why the unroll factor is tied to `cache_line_size / sizeof(element)`, and why pointer hoisting matters given C's aliasing rules.
- Use `pahole`'s output to explain **why struct field ordering affects cache efficiency**, and state the two concrete layout rules the paper gives.
- Explain **precisely why prefetching cannot cross page boundaries**, and connect this to why minimizing total pages touched matters independently of cache-line-level locality.
- Distinguish the **four software prefetch hint levels (T0/T1/T2/NTA)** and explain when each is the right choice, including the failure mode of choosing T0 when L1d is too small for the prefetched data.