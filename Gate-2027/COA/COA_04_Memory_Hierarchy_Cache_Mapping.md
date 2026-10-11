# Computer Architecture → Memory Hierarchy & Cache Mapping

**GATE 2027 syllabus:** Computer Organization and Architecture — memory interfacing and hierarchy: performance, cache memory mapping.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** Memory is byte-addressable. A physical address splits into tag, index (set number) and block offset fields: offset bits = log₂(block size); for direct-mapped, index bits = log₂(number of cache lines); for k-way set-associative, index bits = log₂(number of sets) = log₂(number of lines / k); tag bits = address bits − index bits − offset bits. Fully associative caches have 0 index bits. AMAT (Average Memory Access Time) = hit time + miss rate × miss penalty (single level); for two levels, AMAT = T₁ + (1−h₁)×(T₂ + (1−h₂)×T_mem).

---

## Questions

### Q1 · MCQ · Fundamental
In a **direct-mapped** cache, a given main-memory block can be placed

- (A) in any cache line
- (B) in exactly one specific cache line, determined by the block's address
- (C) in any line within a fixed set of k lines
- (D) nowhere — direct-mapped caches don't cache individual blocks

---

### Q2 · NAT · Fundamental
A cache has a block (line) size of 64 bytes. The number of bits used for the block offset field of the address is ______.

---

### Q3 · MSQ · Standard
Which of the following statements about cache mapping schemes is/are TRUE?

- (A) A fully associative cache has no index bits — any block can go in any line.
- (B) A direct-mapped cache is a special case of k-way set-associative with k = 1.
- (C) Increasing associativity (for the same total cache size and block size) generally reduces conflict misses but can increase hit time and hardware complexity.
- (D) A fully associative cache never has conflict misses.

---

### Q4 · NAT · Standard
A system has 32-bit addresses, a direct-mapped cache of size 32 KB and a block size of 64 bytes. The number of bits in the **tag** field is ______.

---

### Q5 · MCQ · Standard
Compared to a direct-mapped cache of the same total size and block size, an 8-way set-associative cache has

- (A) fewer index bits and more tag bits
- (B) more index bits and fewer tag bits
- (C) the same index and tag bit split
- (D) no index bits at all

---

### Q6 · NAT · Moderate
A system has 32-bit addresses, an 8-way set-associative cache of size 32 KB and a block size of 64 bytes. The number of bits in the **tag** field is ______.

---

### Q7 · MSQ · Moderate
Which of the following statements about cache write policies is/are TRUE?

- (A) Write-through immediately updates main memory on every write, keeping cache and memory consistent at all times.
- (B) Write-back updates main memory only when the modified (dirty) block is evicted from the cache.
- (C) Write-back generally produces less memory traffic than write-through for workloads with repeated writes to the same block.
- (D) A "dirty bit" is used specifically to track whether a write-through cache line differs from memory.

---

### Q8 · NAT · Moderate
A two-level cache system has: L1 hit time 1 ns, L1 hit rate 90%; L2 hit time 8 ns (from the point the L1 miss is detected), L2 hit rate (of L1 misses) 80%; main memory access time 80 ns (from the point the L2 miss is detected). The AMAT, in ns (rounded to two decimal places), is ______.

---

### Q9 · MSQ · Hard
Which of the following statements about virtual memory and cache interaction is/are TRUE?

- (A) A TLB (Translation Lookaside Buffer) caches recent virtual-to-physical address translations to speed up address translation.
- (B) A "physically indexed, physically tagged" cache requires the TLB translation to complete before the cache lookup can begin.
- (C) A "virtually indexed, physically tagged" cache can, in some designs, begin the cache lookup (index into the cache) in parallel with the TLB translation, only needing the translated physical tag for the final comparison.
- (D) Cache memory and virtual memory (paging) serve fundamentally unrelated purposes and share no common design concepts (like associativity, replacement policy or address partitioning).

---

### Q10 · NAT · Killer
A single-level cache has a hit time of 1 ns and a miss penalty of 100 ns (from the point of the miss, on top of the hit time already incurred). The minimum hit rate required to keep the AMAT at or below 3 ns is ______%  (give as a percentage, e.g. 90 for 90%).

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | B | Definition of direct mapping |
| 2 | 6 | log₂(64) |
| 3 | A, B, C, D | All four are standard, correct facts — a fully associative cache can still have compulsory and capacity misses, but never conflict misses specifically, by definition |
| 4 | 17 | 32 KB / 64 B = 512 lines → 9 index bits, 6 offset bits; 32 − 9 − 6 |
| 5 | A | 8-way has fewer sets than direct-mapped for the same total size, so fewer index bits and correspondingly more tag bits (verified: 6 index/20 tag vs. 9 index/17 tag in Q4/Q6) |
| 6 | 20 | 32 KB / 64 B = 512 lines / 8 ways = 64 sets → 6 index bits, 6 offset bits; 32 − 6 − 6 |
| 7 | A, B, C | (D) is false — a dirty bit is used in write-back caches, not write-through (which has no need for one, since memory is always current) |
| 8 | 2.54 | 1 + 0.1×(8 + 0.2×80) = 1 + 0.1×24 |
| 9 | A, B, C | (D) is false — both systems share core ideas like address partitioning and associativity-like lookup structures (e.g., TLBs are themselves often set-associative) |
| 10 | 98 | 1 + (1−h)×100 ≤ 3 → (1−h) ≤ 0.02 → h ≥ 0.98 |
