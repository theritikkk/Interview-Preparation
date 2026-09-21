# OS → Memory Management & Paging (Address Translation)

**GATE 2027 syllabus:** Operating System — memory management and virtual memory.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** Memory is byte-addressable unless stated otherwise. K = 2¹⁰, M = 2²⁰, G = 2³⁰. Page size equals frame size.

---

## Questions

### Q1 · NAT · Fundamental
A system has a **word-addressable** memory with 4-byte words. Virtual addresses are 36 bits long and the page size is 16 KB. The number of bits in the virtual page number is ______.

---

### Q2 · MSQ · Fundamental
Which of the following statements is/are TRUE?

- (A) Paging can cause internal fragmentation.
- (B) Contiguous allocation with variable-sized partitions suffers from external fragmentation.
- (C) Compaction of memory requires that processes can be relocated at run time.
- (D) Paging eliminates both internal and external fragmentation.

---

### Q3 · MCQ · Standard
Free holes in memory (in address order, sizes in KB): **250, 510, 230, 450, 130**. Five requests arrive in the order **220, 210, 140, 310, 330** (KB). Each allocation is carved from the chosen hole and the remainder stays as a hole at the same place. A request that cannot be satisfied is rejected and the next request is processed.

The numbers of requests satisfied under **first fit**, **best fit** and **worst fit** are, respectively,

- (A) 4, 5, 3
- (B) 5, 5, 3
- (C) 4, 5, 4
- (D) 3, 5, 3

---

### Q4 · MCQ · Standard
The page size is 1 KB. Part of a page table (byte-addressable memory):

| Page | 8 | 9 | 10 | 11 |
|---|---|---|---|---|
| Frame (hex) | 0F | 21 | 1D | 2C |

The physical address (in hex) for the virtual address **0x2B5C** is

- (A) 0x1D35C
- (B) 0x775C
- (C) 0x7400
- (D) 0xB35C

---

### Q5 · MSQ · Standard
Which of the following statements about the choice of page size is/are TRUE?

- (A) Increasing the page size reduces the size of a single-level page table.
- (B) Increasing the page size tends to increase internal fragmentation.
- (C) Increasing the page size always reduces the page-fault rate.
- (D) For a fixed number of TLB entries, increasing the page size increases the TLB reach.

---

### Q6 · NAT · Moderate
A process has four regions: text 18,000 bytes, data 10,000 bytes, heap 5,000 bytes and stack 3,000 bytes. Each region is placed in its own set of whole pages (two regions never share a page). The page size is 4096 bytes. The total internal fragmentation of the process (in bytes) is ______.

---

### Q7 · MSQ · Moderate
Segmentation: the segment table is

| Segment | 0 | 1 | 2 |
|---|---|---|---|
| Base | 2000 | 8000 | 5000 |
| Limit | 500 | 1200 | 100 |

The segment-table length register holds 3. A logical address is (segment, offset), offsets counted from 0. Which of the following logical addresses cause a **trap**?

- (A) (0, 500)
- (B) (1, 1199)
- (C) (2, 100)
- (D) (3, 10)

---

### Q8 · NAT · Moderate
Physical memory has 512 frames. Three processes P, Q and R each map exactly 64 distinct frames (no frame is mapped twice within one process). Let P, Q, R also denote the sets of frames mapped by the processes. |P ∩ Q| = 16, |Q ∩ R| = 24, |P ∩ R| = 10 and |P ∩ Q ∩ R| = 6. No frame is used for anything else. The number of free frames is ______.

---

### Q9 · NAT · Hard
An **inverted page table** has one entry per physical frame. Physical memory is 2 GB, the page size is 4 KB and virtual addresses are 48 bits. Each entry stores the process ID (16 bits), the virtual page number and 4 control bits, and the entry size is rounded up to a whole number of bytes. The size of the inverted page table in KB is ______.

---

### Q10 · NAT · Killer
Virtual addresses are 48 bits and physical addresses are 36 bits. A **single-level** page table has one PTE per virtual page; each PTE stores the frame number plus 4 control bits and is rounded up to a whole number of bytes. The page table of a process must not exceed 768 MB. The smallest possible page size (a power of two), in KB, is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | 24 | Page = 4096 words, so 12 offset bits; 36 − 12 |
| 2 | A, B, C | Paging still has internal fragmentation |
| 3 | A | First fit rejects 330; worst fit rejects 310 and 330; best fit places all five |
| 4 | B | Page 10, offset 0x35C, frame 0x1D: 0x1D·0x400 + 0x35C |
| 5 | A, B, D | (C) is false |
| 6 | 9056 | Pages 5 + 3 + 2 + 1 = 11 → 45,056 − 36,000 |
| 7 | A, C, D | Valid offsets are 0…limit−1; segment 3 does not exist |
| 8 | 364 | Used = 192 − (16 + 24 + 10) + 6 = 148; 512 − 148 |
| 9 | 3584 | 2¹⁹ frames; entry = 16 + 36 + 4 = 56 bits = 7 bytes |
| 10 | 1024 | 2²⁸ entries × 3 B = 768 MB at 1 MB pages (PTE = 16 + 4 bits → 3 B); 512 KB pages give 2²⁹ × 3 B = 1.5 GB |
