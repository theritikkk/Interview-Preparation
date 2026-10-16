# COA Notes 02 — Memory Hierarchy & Cache (High-Yield Deep Dive)

> **Why this file exists:** "Cache Memory" is on the high-yield list. GATE asks three shapes almost every year: (1) split a physical address into tag/index/offset for a given organization, (2) compute AMAT with one or two cache levels, (3) count hits/misses while traversing an array. All three are pure arithmetic once the mental model is right. Built using the PW GATE handbook's framing (locality, mapping, replacement list, write policies, the array example) plus derivations and a verified 10-question practice set.
>
> **Companion question bank:** `COA_04_Memory_Hierarchy_Cache_Mapping.md`. PW source: `../Reference/PW_Subject_Handbooks/PW_COA.txt` (§8).

---

## 0. The mental model: the pyramid and why it works

```
        registers   ← fastest, tiniest, per-instruction
          cache L1   (ns)
            cache L2/L3
              main memory (DRAM, ~100 ns)
                disk/SSD   ← huge, slow
```

The gap between CPU speed and memory speed is bridged by a small fast memory that holds **the actively used portions** of the program. Cache effectiveness rests on **locality of reference** (PW):

- **Temporal:** a recently accessed item is likely to be accessed again soon (loop body, same variable).
- **Spatial:** items near a recently accessed item are likely to be accessed soon (sequential code, array elements in a row).

Every technique in this file exists to exploit those two lines.

---

## 1. Measuring cache performance

```
  Hit ratio  h = hits / total references

  AMAT (average memory access time) = h × (hit time) + (1 − h) × (miss time)
```

With a miss penalty M (time to bring the block from the next level):

```
  AMAT = hit_time + miss_rate × miss_penalty
```

**Two-level cache worked example.** L1: 1 ns, 90% hit. L2: 10 ns, 70% hit (of L1 misses). Memory: 100 ns.

```
  AMAT = 1 + 0.10 × [10 + 0.30 × 100]
       = 1 + 0.10 × 40
       = 5 ns
```

Key habit: the L2 *miss penalty* is only paid on L1 misses, so it multiplies the L1 miss *rate* (0.10) — nest the brackets instead of adding flat terms.

---

## 2. Cache organization and address splitting

Memory is divided into **blocks** (the unit of transfer); cache is divided into **lines** (a block's slot). A physical address is read as:

```
  [ tag | index (set/line number) | block offset ]
```

| Mapping | Index bits | Block placement | Comparator count |
|---|---|---|---|
| **Direct-mapped** | log₂(#lines) | block j → line `j mod (#lines)` | **1** |
| **k-way set-associative** | log₂(#sets) | any of k ways in the chosen set | **k** |
| **Fully associative** | 0 | anywhere | **#blocks** (check everything) |

**Derivation of the splits (do it in this order, every time):**

1. `offset bits = log₂(block size)` (byte-addressable).
2. `#sets = cache size / (block size × associativity)` → `index bits = log₂(#sets)`.
3. `tag bits = address bits − index bits − offset bits`.

**Worked example A.** 32-bit addresses; 64 KB cache, 8-way, 32 B blocks:

```
  offset = log₂32 = 5
  #sets  = 64KB / (32B × 8) = 256 → index = 8
  tag    = 32 − 8 − 5 = 19
```

**Worked example B.** Same sizes, **fully associative** → no index at all: `tag = 32 − 5 = 27`. (Fully associative = one big set; tag must identify the block uniquely.)

**Worked example C.** 40-bit addresses; 512 KB cache, 16-way, 32 B blocks:

```
  #sets = 512KB/(32B×16) = 1024 → index = 10;  offset = 5
  tag   = 40 − 10 − 5 = 25
```

**Why tag lookup scales as listed:** to decide *hit/miss*, every candidate line in the addressed set must have its tag compared in parallel — direct-mapped has exactly one candidate; a k-way set has k; fully associative has every line.

---

## 3. Replacement, write policies, and traffic

**Replacement policies (PW list):** FIFO, LRU (least recently used — exploits temporal locality best), LFU, Random. Direct-mapped has *no* choice — the mapping forces the victim.

**Write policies:**

| Policy | On write hit | Memory traffic | Consistency |
|---|---|---|---|
| **Write-through** | write cache AND memory | every write goes out | memory always current (DMA-friendly) |
| **Write-back** | write cache only; set **dirty bit** | only on eviction | memory stale until eviction |

- **Write-allocate:** on a write *miss*, fetch the block into cache first (typical with write-back).
- **No-write-allocate:** write straight to memory, don't disturb the cache (typical with write-through; streaming writes).
- **DMA interaction (PW point):** DMA transfers go to memory directly. With write-back, memory may be stale → cache must be flushed or made coherent. Write-through keeps memory consistent for free.

**Why write-through can still be slow:** one memory write per store; mitigated by write buffers.

---

## 4. Worked array example (the PW archetype, made rigorous)

**Setup:** `int A[100]`, each element occupies **4 words**; cache = **32 words**; block size = **8 words**; direct-mapped. Loop: `for i = 0..99: A[i] = A[i] + 10`.

Derived quantities:

```
  elements per block = 8/4  = 2        (block k holds A[2k], A[2k+1])
  #lines             = 32/8 = 4        (line = block number mod 4)
```

Execution per block (2 elements):

| Access | R/W | Result | Why |
|---|---|---|---|
| A[2k] | R | **miss** | first touch of block k → fetch |
| A[2k] | W | hit | block now resident |
| A[2k+1] | R | hit | same block, spatial locality |
| A[2k+1] | W | hit | resident |

Per block: 1 miss + 3 hits; over 50 blocks: **50 misses, 150 hits → hit ratio = 150/200 = 75%**.

**Which line gets thrashed?** Block b → line `b mod 4`. Line 0 serves blocks 0, 4, 8, …, 48 → **13 distinct blocks pass through line 0** (13 loads, **12 evictions**). This is why "counting line replacement" questions appear — and why loop patterns with strides that alias into one line (e.g., stride 16 elements with 4 lines) collapse performance.

---

## 5. Miss types, multi-level AMAT, and terminology

**Three C's:**
- **Compulsory** (cold): first ever touch — unavoidable.
- **Capacity:** working set larger than cache.
- **Conflict:** too many blocks map to the same line/set (fix: higher associativity). *Direct-mapped never has capacity misses that associativity couldn't also fix some of — but the naming is what GATE tests.*

**Multi-level AMAT (nested form):**

```
  AMAT = t1 + m1 × (t2 + m2 × (t3 + … + mN × tMem))
```

**Quick vocabulary:** block/line; hit time; miss penalty; write buffer; dirty bit; LRU stack; victim.

---

## 6. Traps & exam patterns

1. **Byte-addressable vs word-addressable.** If the question says word-addressable, the offset formula divides by word size. Default in GATE: byte addresses unless stated.
2. **Associativity appears in the #sets denominator** — more ways ⇒ fewer sets ⇒ fewer index bits ⇒ **more tag bits**. A classic "what changes?" question.
3. **Fully associative: tag = address − offset (no index).**
4. **AMAT nesting:** L2 miss penalty is conditioned on an L1 miss — multiply by the L1 miss rate, don't add unconditionally.
5. **Write allocation:** a write miss *may or may not* fetch the block depending on policy; state your assumption if the question doesn't.
6. **Array problems:** the only things that matter are (a) how many elements per block, (b) the access pattern vs the mapping. Do them with the "per block" table like §4 — never by simulating 100 iterations in your head.
7. **Comparator counts** = lines compared in parallel (1 / k / #lines).

---

## 7. Practice set (verified by simulation)

> Assume byte-addressable memory, LRU where a policy is needed, and write-allocate with write-back unless stated otherwise.

### Q1 · NAT · Fundamental
32-bit addresses; 64 KB cache, 8-way set-associative, 32 B blocks. The number of **tag bits** is ______.

### Q2 · NAT · Fundamental
Same cache size/block size (64 KB, 32 B) but **fully associative**, 32-bit addresses. The number of tag bits is ______.

### Q3 · NAT · Standard
L1: hit time 1 ns, hit ratio 90%. L2: hit time 10 ns, hit ratio 70% (of L1 misses). Main memory: 100 ns. The AMAT is ______ ns.

### Q4 · MCQ · Standard
The number of tag comparators required, in order (direct-mapped, 4-way set-associative, fully associative), is:

- (A) 1, 4, 4
- (B) 1, 4, number of lines
- (C) 4, 1, number of lines
- (D) 1, number of sets, number of lines

### Q5 · MSQ · Standard
Which of the following is/are TRUE?

- (A) Write-back reduces memory traffic compared to write-through for a block written many times.
- (B) A dirty bit records that a cached block differs from its copy in memory.
- (C) FIFO, LRU, LFU and Random are all valid cache replacement policies.
- (D) Write-through makes main memory permanently stale until eviction.

### Q6 · NAT · Moderate
`int A[100]` with 4 words per element; direct-mapped cache of 32 words; block size 8 words. Execute `for i = 0..99: A[i] = A[i] + 10` (read then write each element; initially empty cache). The number of **read/write accesses that miss** is ______.

### Q7 · NAT · Moderate
Same setup as Q6. The number of times the cache line that initially receives `A[0]` (line 0) is **replaced** (evicted and refilled by a different block) is ______.

### Q8 · MSQ · Moderate
Which of the following statements about locality is/are TRUE?

- (A) Re-executing the body of a loop demonstrates temporal locality.
- (B) Accessing consecutive array elements demonstrates spatial locality.
- (C) A larger block size exploits spatial locality but may increase conflict/capacity pressure.
- (D) Stack operations (push/pop) exploit spatial locality only.

### Q9 · NAT · Hard
40-bit addresses; 512 KB cache, 16-way set-associative, 32 B blocks. The number of **tag bits** is ______.

### Q10 · NAT · Killer
`int A[64][64]` (4-byte ints), stored **row-major**, accessed in **column-major** order (j outer, i inner). Direct-mapped cache: 8 lines × 16 B blocks (4 ints per block), initially empty. The total number of cache **hits** during the full traversal is ______.

---

## Answer Key

| Q | Answer | Why (condensed) |
|---|---|---|
| 1 | 19 | 32 − 8 − 5 |
| 2 | 27 | 32 − 5 (no index bits) |
| 3 | 5 | 1 + 0.1×(10 + 0.3×100) |
| 4 | B | 1 for direct; k for k-way; one per line for FA |
| 5 | A, B, C | (D) false — that's write-back |
| 6 | 50 | one miss per 2-element block |
| 7 | 12 | 13 blocks pass through line 0 → 13 loads, 12 evictions |
| 8 | A, B, C | (D) false — push/pop is mainly temporal |
| 9 | 25 | 40 − 10 − 5 |
| 10 | 0 | column stride aliases everything into one line → all misses |

### Full solutions

**Q1.** offset = log₂(32 B) = 5. sets = 64 KB/(32 B × 8 ways) = 256 → index = 8. tag = 32 − 8 − 5 = **19**. (More associativity → fewer sets → *more* tag bits than direct-mapped.)

**Q2.** Fully associative = every line is a candidate; no index field at all. tag = 32 − 5 = **27**.

**Q3.** `AMAT = 1 + 0.10 × (10 + 0.30 × 100) = 1 + 0.10 × 40 = 1 + 4 = 5 ns`. Trap: adding `0.3×100` to every access instead of conditioning on the L1 miss.

**Q4.** Direct-mapped: exactly one line per address → 1 comparator. 4-way: the 4 ways of the chosen set are compared → 4. Fully associative: every line (no set restriction) → number of lines. **B**.

**Q5.** A ✔ — write-back writes to memory only on eviction (once), write-through writes every store. B ✔ — definition of dirty. C ✔ — standard set (PW's list). D ✘ — reversed: it's write-*back* that leaves memory stale; write-through keeps memory current (its virtue with DMA).

**Q6.** 8 words/block ÷ 4 words/element = **2 elements per block** → 50 blocks. Per block: first read misses (fetch), everything else hits → **50 misses** (and 150 hits; hit ratio 75%). The write to the just-read element cannot miss because the block was already fetched.

**Q7.** Block b maps to line `b mod 4`. Line 0 serves b = 0, 4, 8, …, 48 → 13 distinct loads of line 0. The first load is not a replacement; each of the remaining 12 loads evicts the previous block → **12 replacements**.

**Q8.** A ✔ — same instructions/data reused across iterations. B ✔ — one block brings along neighbours (the block holds 2+ consecutive elements). C ✔ — bigger blocks amortize misses but hold fewer distinct blocks / disturb the cache. D ✘ — stack top accesses are the textbook *temporal* locality case (spatial is weak: only the top few cells).

**Q9.** offset = 5. sets = 512 KB/(32 B × 16) = 1024 → index = 10. tag = 40 − 10 − 5 = **25**.

**Q10.** Row-major layout: element (i, j) sits at index `64i + j`; block = `⌊(64i+j)/4⌋ = 16i + ⌊j/4⌋`; line = block mod 8.

- Column-major order visits (0,j), (1,j), …, (63,j). For fixed j, successive accesses differ by `16i` blocks — and `16i mod 8 = 0` → **every block of the column maps to the same line** (line ⌊j/4⌋ mod 8).
- Each access evicts the line's previous block; the next access always finds a different block → **every access misses: 4096 accesses, 0 hits**.
- (Row-major for contrast: consecutive elements share a block → 1 miss + 3 hits per 4 accesses → 1024 misses, 3072 hits.)

Answer: **0**.

---

## 8. Next steps

1. Re-derive every split as: offset → sets → tag. Do Q1/Q2/Q9 twice more mentally.
2. Practice reps: `COA_04_Memory_Hierarchy_Cache_Mapping.md`.
3. Continue to `COA_Notes_03_IEEE754_Floating_Point.md`.
