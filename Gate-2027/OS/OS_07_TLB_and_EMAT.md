# OS → TLB & Effective Memory Access Time

**GATE 2027 syllabus:** Operating System — memory management and virtual memory.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** TLB lookup and memory access are sequential unless stated otherwise. A TLB miss requires reading the page-table levels from memory, then the data. K = 2¹⁰, M = 2²⁰.

---

## Questions

### Q1 · NAT · Fundamental
A TLB has 128 entries and the page size is 16 KB. The TLB reach in MB is ______.

---

### Q2 · NAT · Fundamental
A system with a single-level page table in memory and a TLB has: TLB access time 10 ns (performed on every reference), memory access time 100 ns, TLB hit ratio 90%. On a hit only the data access follows; on a miss the page table is read from memory and then the data. The effective memory access time in ns is ______.

---

### Q3 · NAT · Standard
A system uses a **3-level** page table (all levels in main memory). TLB access time is 5 ns (every reference), memory access time is 80 ns, TLB hit ratio is 95%. On a miss, the three page-table levels are read from memory and then the data is accessed. The effective memory access time in ns is ______.

---

### Q4 · NAT · Standard
TLB access time is 10 ns (every reference) and memory access time is 100 ns. The page table has **2 levels** (a TLB miss needs 2 memory accesses for translation, then 1 for data). What is the minimum TLB hit ratio, in percent, needed to keep the effective access time at or below 130 ns?

---

### Q5 · MSQ · Standard
Which of the following statements is/are TRUE?

- (A) The TLB is a cache of page-table entries.
- (B) A TLB miss implies a page fault.
- (C) A TLB without ASIDs must be flushed (or its entries invalidated) when the OS switches from one process to another.
- (D) A TLB hit implies that the requested data is present in the L1 cache.

---

### Q6 · NAT · Moderate
A paging system has a single-level page table in memory and a TLB.

- TLB lookup: 2 ns on every reference. Main-memory access: 60 ns.
- TLB hit ratio: 98%. On a hit, only the data access (60 ns) follows.
- On a TLB miss the page table is read (60 ns). With probability 5% (of the TLB misses) the page is not in memory and a **page fault** occurs; its service time is 5 ms if the victim page is clean and 10 ms if it is dirty, and 40% of the page faults have a dirty victim. After the fault is serviced, the data access (60 ns) is made. Without a fault, the data access (60 ns) follows the page-table read.

Ignore all other overheads. The effective memory access time in ns (rounded to one decimal place) is ______.

---

### Q7 · NAT · Moderate
A 4-way set-associative TLB has 64 entries. Virtual addresses are 48 bits, physical addresses are 40 bits and the page size is 4 KB. Each TLB entry stores the tag (the part of the virtual page number not used for set indexing), the frame number, 2 control bits and an 8-bit ASID. The total storage of the TLB in bytes is ______.

---

### Q8 · NAT · Moderate
A program repeatedly scans a 64-page array sequentially (page 0, 1, …, 63, 0, 1, …). While visiting a page it makes 16 consecutive memory references to that page. The TLB has 32 entries, is fully associative and uses LRU replacement. In steady state, the TLB miss ratio (in percent) is ______.

---

### Q9 · NAT · Hard
A system has a 2-level page table (a TLB miss needs two page-table reads from memory before the data access). The TLB lookup (t ns) is done before the memory access on every reference; a TLB hit needs only the data access. The memory access time m ns and t are unknown. Measurements: the effective access time is 150 ns when the TLB hit ratio is 0.8 and 130 ns when it is 0.9. The TLB lookup time t in ns is ______.

---

### Q10 · NAT · Killer
A TLB has 16 entries organised as 8 sets of 2 ways; the set index is (virtual page number mod 8) and each set uses LRU. A program makes exactly one reference to each of the pages 0, 1, 2, …, 19 in this order and then repeats this sequence indefinitely. In steady state, the TLB hit ratio (in percent) is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | 2 | 128 × 16 KB |
| 2 | 120 | 10 + 0.9·100 + 0.1·200 |
| 3 | 97 | 5 + 0.95·80 + 0.05·(3·80 + 80) |
| 4 | 90 | 10 + h·100 + (1 − h)·300 ≤ 130 |
| 5 | A, C | TLB hit does not say anything about the data cache |
| 6 | 7063.2 | 2 + 0.98·60 + 0.02·(60 + 0.05·7 ms + 60); average service time 0.6·5 + 0.4·10 = 7 ms |
| 7 | 560 | 16 sets → tag 32 bits; frame 28 bits; entry 32 + 28 + 2 + 8 = 70 bits; 64 × 70 = 4480 bits |
| 8 | 6.25 | 64 pages > 32 entries, so LRU misses on the first reference of every page; 1 miss per 16 references |
| 9 | 10 | EAT = t + m(1 + 2(1 − h)): 1.4m + t = 150, 1.2m + t = 130 → m = 100 |
| 10 | 40 | Sets 0–3 receive 3 pages (thrash under LRU), sets 4–7 receive 2 pages (always hit); 8 of 20 references hit |
