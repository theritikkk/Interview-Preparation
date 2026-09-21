# DBMS → File Organization & Indexing

**GATE 2027 syllabus:** Databases — file organization, indexing.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** Records are stored in **unspanned** form (a record never crosses a block boundary) unless stated otherwise. A block access reads or writes one block. Blocking factor bf = ⌊block size / record size⌋. An index block holds ⌊block size / entry size⌋ entries. Binary search over b blocks costs ⌈log₂ b⌉ block accesses in the worst case. The primary index is a sparse index with one entry per data block of an ordered file; a secondary index on a non-ordering field is dense (one entry per record).

---

## Questions

### Q1 · NAT · Fundamental
A file has 10,000 records of 100 bytes each, stored unspanned in blocks of 4096 bytes. The number of blocks needed is ______.

---

### Q2 · NAT · Fundamental
A file is sorted on its key and occupies 1500 blocks. The worst-case number of block accesses for a binary search on the key is ______.

---

### Q3 · NAT · Standard
A file of 30,000 records (100 bytes each) is ordered on a key field and stored in 1024-byte blocks. A primary index has entries of 15 bytes (key + pointer). The worst-case number of block accesses to retrieve a record using **binary search on the index** and then the data block is ______.

---

### Q4 · NAT · Standard
10,000 records of 120 bytes each are stored in 512-byte blocks. (Blocks needed with unspanned records) − (blocks needed with spanned records) = ______.

---

### Q5 · MSQ · Standard
Which of the following statements is/are TRUE?

- (A) A primary index on the ordering key field is a sparse index.
- (B) A secondary index on a non-ordering field must be dense.
- (C) A file can have at most one primary (or clustering) index, because it can be physically ordered on only one field.
- (D) A dense index on a file has fewer entries than a sparse index on the same file.

---

### Q6 · NAT · Moderate
A file of 40,000 records (128 bytes each) is stored in 2048-byte blocks. A secondary index on a non-ordering key field has 20-byte entries. The worst-case number of block accesses to retrieve a record by binary search on the index followed by one data-block access is ______.

---

### Q7 · NAT · Moderate
A file has 100,000 records with a blocking factor of 20. Index entries are 16 bytes and blocks are 1024 bytes. (Number of blocks in a dense index) − (number of blocks in a sparse index) = ______.

---

### Q8 · NAT · Moderate
A file of 30,000 records (10 records per block) is ordered on the non-key field DeptNo, which has 100 distinct values, each occurring in exactly the same number of consecutive records. A clustering index has one 8-byte entry per distinct DeptNo, with 512-byte blocks. To read **all** records with a given DeptNo, the index is searched by binary search and then the required data blocks are read, each once. The total number of block accesses is ______.

---

### Q9 · NAT · Hard
A file of 5,000,000 records with a blocking factor of 10 has a multi-level sparse primary index. An index block holds 100 entries. Each level of the index has one entry per block of the level below, and the top level is a single block that is always cached in memory. The number of disk block accesses to retrieve one record is ______.

---

### Q10 · NAT · Killer
A file has a two-level sparse primary index: level 1 has one entry per data block, level 2 has one entry per level-1 block, and level 2 must fit in a **single** block. Blocks are 1024 bytes, index entries are 16 bytes, and a data block holds 8 records. The maximum number of records the file can contain is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | 250 | bf = ⌊4096/100⌋ = 40; 10,000 / 40 |
| 2 | 11 | ⌈log₂ 1500⌉ |
| 3 | 7 | b = 3000; 68 entries per block, 45 index blocks; ⌈log₂ 45⌉ + 1 = 6 + 1 |
| 4 | 156 | Unspanned bf = 4, so 2500 blocks; spanned ⌈1,200,000 / 512⌉ = 2344 |
| 5 | A, B, C | (D) is the opposite |
| 6 | 10 | bf = 16; 102 entries per block; 393 index blocks; ⌈log₂ 393⌉ + 1 = 9 + 1 |
| 7 | 1484 | 64 entries per block; dense ⌈100,000 / 64⌉ = 1563; sparse ⌈5000 / 64⌉ = 79 |
| 8 | 31 | 2 index blocks: ⌈log₂ 2⌉ = 1; 300 records per DeptNo = 30 blocks |
| 9 | 3 | 500,000 data blocks; levels 5000, 50, 1; top cached, so 2 index reads + 1 data read |
| 10 | 32768 | 64 entries per block: 64 × 64 data blocks × 8 records |
