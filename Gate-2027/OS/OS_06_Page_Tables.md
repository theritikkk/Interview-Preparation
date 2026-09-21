# OS → Page Tables (single-level, multi-level, inverted)

**GATE 2027 syllabus:** Operating System — memory management and virtual memory.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** Memory is byte-addressable unless stated otherwise. K = 2¹⁰, M = 2²⁰, G = 2³⁰, T = 2⁴⁰. Each page-table node of a multi-level scheme occupies exactly one page unless stated otherwise.

---

## Questions

### Q1 · NAT · Fundamental
Virtual addresses are 46 bits, physical addresses are 40 bits and the page size is 8 KB. A single-level page table holds one PTE per virtual page; each PTE stores the frame number plus 3 control bits, rounded up to a whole number of bytes. The size of the page table in GB is ______.

---

### Q2 · NAT · Fundamental
Virtual addresses are 38 bits, the page size is 8 KB and each PTE is 8 bytes. In a multi-level page table every table must fit in one page. The minimum number of levels required is ______.

---

### Q3 · NAT · Standard
A 32-bit system uses 4 KB pages, 4-byte PTEs and a two-level page table (10 | 10 | 12 bits). The root table is always allocated (one page); a second-level table is allocated only if at least one of its pages is in use. A process uses exactly the following virtual regions:

| Region | Range |
|---|---|
| Text | 0x003FF000 – 0x00401FFF |
| Heap | 0x10000000 – 0x107FFFFF |
| Stack | 0xBFFF0000 – 0xBFFFFFFF |

The total memory occupied by the page tables of this process, in KB, is ______.

---

### Q4 · MSQ · Standard
Which of the following statements about multi-level page tables is/are TRUE?

- (A) Second-level tables for unused regions of the address space need not be allocated.
- (B) With L levels and no TLB hit (all tables in memory, no caching), a memory reference needs L page-table accesses plus one data access.
- (C) A multi-level page table always occupies less memory than a single-level page table for the same process.
- (D) In every multi-level scheme, every level's table must occupy exactly one page.

---

### Q5 · NAT · Standard
The memory is **word-addressable** with 4-byte words, and virtual addresses are 40 bits. The page size is 32 KB and each PTE is 4 bytes. Every table of a multi-level page table must fit in one page. The minimum number of levels is ______.

---

### Q6 · NAT · Moderate
A byte-addressable system uses a **3-level** page table. The page size is 8 KB, each PTE is 8 bytes and every table occupies exactly one page (each level is fully indexed). The number of bits in the virtual address is ______.

---

### Q7 · MCQ · Moderate
A UNIX-like OS implements `fork()` with copy-on-write. Immediately after `fork()` returns in both processes, which one of the following is TRUE?

- (A) The child has its own page table whose entries point to the same physical frames as the parent's; writable pages are marked read-only in both.
- (B) All data pages of the parent have been physically copied for the child.
- (C) The child has no page table until its first memory access.
- (D) The child shares the parent's page table itself, so a later write by either process is automatically visible to the other.

---

### Q8 · MCQ · Moderate
Which one of the following statements about an inverted page table is **FALSE**?

- (A) It has one entry per physical frame.
- (B) A hash of (process ID, virtual page number) is commonly used to locate the entry.
- (C) Its size grows with the size of the virtual address space of each process.
- (D) Sharing a physical frame between two processes is harder to support.

---

### Q9 · NAT · Hard
A system uses a **4-level** page table. Each level is indexed by 9 bits, the page size is 4 KB and each PTE is 8 bytes. Every table (at every level) occupies exactly one 4 KB page and is allocated only if at least one of its entries is in use; the root is always allocated. A process maps exactly **1025** virtual pages (their positions are arbitrary). The difference between the **maximum** and the **minimum** possible total page-table memory, in KB, is ______.

---

### Q10 · NAT · Killer
A **3-level** page table uses 9 index bits per level, a 12-bit page offset and 8-byte PTEs. Every table occupies one 4 KB page and is allocated only when needed (the root is always allocated). The page tables of a process may occupy at most 100 KB in total. The maximum number of virtual pages that the process can have mapped is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | 32 | 2³³ entries; frame 27 + 3 = 30 bits → 4 B; 2³⁵ B |
| 2 | 3 | 13 offset bits, 10 bits per level, 25 bits remain → ⌈25/10⌉ |
| 3 | 24 | Second-level tables for 4 MB chunks 0, 1, 64, 65, 767 = 5; plus root = 6 pages |
| 4 | A, B | (C) fails for a fully used address space |
| 5 | 3 | Page = 8192 words → 13 offset bits; 8192 PTEs per page → 13 bits per level; 27 bits remain |
| 6 | 43 | 13 + 3 × 10 |
| 7 | A | Separate page tables, shared frames, write-protected |
| 8 | C | Inverted table size depends on physical memory |
| 9 | 10228 | Min 3 + 1 + 1 + 1 = 6 tables; max 1025 + 1025 + 512 + 1 = 2563 tables; (2563 − 6) × 4 KB |
| 10 | 11776 | 25 tables: root + one level-2 + 23 full leaf tables; 23 × 512 |
