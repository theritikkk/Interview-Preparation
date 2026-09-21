# OS → File Systems (allocation, inodes, free space, links)

**GATE 2027 syllabus:** Operating System — file systems.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** K = 2¹⁰, M = 2²⁰, G = 2³⁰, T = 2⁴⁰. Block addresses / pointers are 4 bytes unless stated otherwise. Ignore all metadata overhead not mentioned in the question.

---

## Questions

### Q1 · NAT · Fundamental
An inode has 12 direct pointers, 1 single-indirect, 1 double-indirect and 1 triple-indirect pointer. The block size is 1 KB and a block address is 4 bytes. The maximum file size in GB (rounded to two decimal places, 1 GB = 2³⁰ bytes) is ______.

---

### Q2 · NAT · Fundamental
A file system uses **linked allocation**: the last 4 bytes of every 4 KB disk block hold the address of the next block, the rest holds data. A file is 10 MB (10 × 2²⁰ bytes) long. The number of disk blocks the file occupies is ______.

---

### Q3 · MSQ · Standard
Which of the following statements is/are TRUE?

- (A) Contiguous allocation supports efficient direct (random) access as well as sequential access.
- (B) In linked allocation (without FAT), reaching the k-th block of a file requires following k pointers from the first block.
- (C) Indexed allocation suffers from external fragmentation of the disk.
- (D) A FAT improves random access over pure linked allocation because the links are kept in a table that can be cached in memory.

---

### Q4 · NAT · Standard
A 128 GB disk has 8 KB blocks and uses a FAT with one entry per disk block. Each entry is the smallest whole number of bytes that can address any block. The size of the FAT in MB is ______.

---

### Q5 · NAT · Standard
A file has 200 blocks and its FCB (directory entry) is already in memory. Count the disk accesses needed to read the **75th block** (blocks numbered from 1, including the access to the block itself) under

1. contiguous allocation (FCB holds start address and length),
2. linked allocation without FAT (FCB holds the address of the first block, every block holds the address of the next),
3. indexed allocation with one index block (FCB holds the index block's address; nothing is cached).

The sum of the three counts is ______.

---

### Q6 · MSQ · Moderate
Which of the following statements about links in a UNIX-like file system is/are TRUE?

- (A) A hard link cannot refer to a file in a different file system.
- (B) Deleting the target of a symbolic link leaves a dangling symbolic link.
- (C) `unlink` decrements the inode's link count; the file's data blocks are freed only when the count reaches 0 and no process has the file open.
- (D) Ordinary users may freely create hard links to directories.

---

### Q7 · NAT · Moderate
An inode has 12 direct pointers, 1 single-indirect and 1 double-indirect pointer. The block size is 4 KB and a block address is 4 bytes. A file is exactly 2 GB + 10 KB (2³¹ + 10 × 2¹⁰ bytes) long. The total number of blocks the file uses, counting data blocks **and** index (indirect) blocks, is ______.

---

### Q8 · NAT · Moderate
An inode has 12 direct pointers, 1 single-indirect and 1 double-indirect pointer; block addresses are 4 bytes. The block size must be a power of two. The smallest block size, in KB, for which the maximum file size is at least 1 TB is ______.

---

### Q9 · NAT · Hard
A file system uses **linked index blocks**. Each 1 KB index block holds 255 pointers to data blocks and 1 pointer to the next index block (null in the last index block). Data blocks are 1 KB. A file is 1 MB + 1 byte (2²⁰ + 1 bytes) long. The total number of blocks (data + index) the file occupies is ______.

---

### Q10 · MSQ · Killer
An inode has 12 direct pointers, one single-, one double- and one triple-indirect pointer. The block size is 1 KB and a block address is 4 bytes. The inode is already in memory and nothing else is cached. Byte offsets are numbered from 0. Reading the byte at which offset(s) requires **exactly 3** disk block accesses?

- (A) 274,431
- (B) 274,432
- (C) 67,383,295
- (D) 67,383,296

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | 16.06 | (12 + 256 + 256² + 256³) KB = 16,843,020 KB |
| 2 | 2563 | ⌈10,485,760 / 4092⌉ |
| 3 | A, B, D | (C) is false; indexed allocation has no external fragmentation |
| 4 | 48 | 2²⁴ blocks → 24-bit addresses → 3 B per entry |
| 5 | 78 | 1 + 75 + 2 |
| 6 | A, B, C | Hard links to directories are restricted |
| 7 | 524804 | Data 524,291; index 1 + 1 + 511 = 513 |
| 8 | 32 | 16 KB blocks allow about 0.25 TB; 32 KB blocks allow about 2 TB |
| 9 | 1030 | 1025 data blocks; ⌈1025 / 255⌉ = 5 index blocks |
| 10 | B, C | Direct ends at 12,287; single at 274,431; double at 67,383,295 |
