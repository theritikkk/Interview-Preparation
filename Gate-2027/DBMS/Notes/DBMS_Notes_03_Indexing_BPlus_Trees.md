# DBMS Notes 03 — File Organization, Indexing & B/B+ Trees (High-Yield Deep Dive)

> **Why this file exists:** "Indexing (preferred) / Transactions" is on the high-yield list — *indexing is the preferred half* (highest marks-per-hour in DBMS). GATE's questions are **arithmetic with a fixed recipe**: block factors (floor vs ceiling!), index block counts (dense/sparse), I/O cost via index levels, **order of B / B+ tree nodes from byte equations**, **min/max keys for a given height**, node counts after insertions, and range-scan costs. Every number below was computed and cross-checked; B+ insertion questions were verified with a simulator.
>
> **Companion question banks:** `DB_08_File_Organization_Indexing.md`, `DB_09_B_and_B_Plus_Trees.md`. PW source: `../Reference/PW_Subject_Handbooks/PW_DBMS.txt` (§4, pages 10.26–10.31). *Hashing (§8 here) is not in the PW extract — it is standard GATE file-organization content, included for completeness.*

---

## 0. The mental model: every question is about disk accesses

```
   DATABASE  →  FILE (table)  →  BLOCK (page)  →  RECORD  →  FIELD

   The disk moves data BLOCK BY BLOCK. The cost of any operation =
   number of block transfers (I/O). An INDEX is just an extra, small,
   sorted file: (search-key, pointer) pairs that lets you jump instead of scan.
```

Two derived quantities control everything:

```
   BF_data  = records per data block       (blocking factor)
   BF_index = index entries per index block
```

---

## 1. Record placement & blocking (PW §4.1–4.3)

**Spanned vs unspanned organisation:**

| | Spanned | Unspanned |
|---|---|---|
| Can a record cross a block boundary? | Yes | No (entire record in one block) |
| Internal fragmentation | none | some (tail of each block wasted) |
| Access cost per record | higher (a record may span blocks) | lower |
| Typical use | variable-length records | fixed-length records |

```
   Unspanned:   BF = ⌊ (Block size − Header) / Record size ⌋     ← FLOOR
   Spanned:     BF = ⌈ (Block size − Header) / Record size ⌉     ← CEIL (byte packing)
   #blocks to hold r records:
        unspanned = ⌈ r / BF ⌉        spanned = ⌈ r · recordSize / (BlockSize − Header) ⌉
```

> **Header defaults to 0** unless the problem gives a header size (PW note).

**Hashing (supplementary, not in PW extract).** Static hashing: bucket = block; `h(k) = k mod m` (often prime m); collisions handled by **overflow chaining**. Performance degrades as the file grows (overflow chains lengthen) — dynamic (extendible) hashing fixes this by doubling the directory (global depth) and splitting only full buckets (local depth). Good for **equality lookup**, bad for **range queries**.

---

## 2. Dense vs sparse indexes (PW §§4.4)

```
   DENSE  : one index entry PER DATA RECORD            (can be built on any search key)
   SPARSE : one index entry per BLOCK, keyed by the
            first key in that block                    (needs the file ORDERED on that key)
```

```
   #sparse entries < #records       (PW §4.4, Important Point I)
```

**Index block factor (PW §4.4.3):**

```
   BF_index = ⌊ (BlockSize − Header) / (Key size + Pointer size) ⌋
```

**I/O cost catalog (PW §4.5–4.6).** With `n` data blocks and `k` index levels:

| Scenario | Block accesses |
|---|---|
| No index, field **ordered** (binary search) | `⌈log₂ n⌉` |
| No index, field unordered (linear) | `n` |
| Dense index, single level | 1 (index) + 1 (data) = 2 |
| Dense index, multilevel (`k` levels) | `k + 1` |
| Sparse primary index, single level | 1 + 1 = 2 (sparse dyadic size) |
| Primary index multilevel | `k + 1` (PW §4.6.1) |
| Secondary index **over key** (always dense) | `k + 1` (PW §4.6.3) |
| Clustered index (ordered non-key) | `k` index blocks + **several consecutive data blocks** (rest of the cluster) |
| Secondary index over **non-key** | `k` index blocks + one data block **per matching record** |

**Number of first-level index blocks:**

```
   dense  (entries = r)          →  ⌈ r / BF_index ⌉
   sparse (entries = #data blocks) →  ⌈ #data_blocks / BF_index ⌉
```

**Building a multilevel index:** while the current level has more than 1 block, add a level: `nj+1 = ⌈ nj / BF_index ⌉`. Stop when the top has 1 block. If the level sizes are `n1 → n2 → … → nk = 1`, the index has **k levels**, and a record lookup costs `k + 1` (add the data block).

---

## 3. The four index types (PW §4.6)

| Type | Search key | Ordered? | Dense/sparse | How many per relation |
|---|---|---|---|---|
| **Primary** | key (candidate key) field | ✔ ordered | sparse **preferred** | at most 1 |
| **Clustering** | non-key field | ✔ ordered | sparse (one entry per distinct value region) | at most 1 (and never with a primary index) |
| **Secondary over key** | key field | ✘ unordered | **must be dense** | many |
| **Secondary over non-key** | non-key field | ✘ unordered | dense (distinct key + bucket of pointers) | many |

> A file can be **physically ordered on only one field** — hence at most one primary *or* clustering index, but arbitrarily many secondary indexes.

---

## 4. B tree (PW §4.7.2)

**Definition.** Order `P` = maximum number of child (block) pointers in a node. A node of order P holds

```
   P block pointers, P − 1 keys, P − 1 record pointers          (record pointers in EVERY node)

   P · (block pointer size)  +  (P − 1)·(key size + record pointer size)  ≤  Block size
```

**Occupancy constraints:**
- Every internal node **except the root**: at least `⌈P/2⌉` block pointers and `⌈P/2⌉ − 1` keys.
- Root: at least **2** block pointers and **1** key.
- Every leaf node at the same level (perfectly height-balanced).

**Search cost:** best case `2` blocks (root + data block — a hit at the root), worst case `k + 1` blocks for `k` index levels. **Strength:** single-record random access. **Weakness:** range/sequential scans (keys scattered across levels).

---

## 5. B+ tree (PW §4.7.3)

**Definition.** Order `P` = maximum pointers per node. Two node types:

```
   INTERNAL node:  P block pointers + (P − 1) keys            (keys are separators; no record ptrs)
                   P · BP  +  (P − 1) · K  ≤  Block size

   LEAF node:      (key, record pointer) pairs + 1 sibling pointer to the NEXT leaf
                   (P − 1)·(K + RP)  +  1 · BP  ≤  Block size
```

- **All record pointers live in the leaves**; the leaves are **linked** (sequential/range access is one pass).
- Internal keys are only routing separators — a key can appear both in an internal node and in a leaf.
- Every leaf is at the same depth. Search always terminates at a leaf: `k` index levels + 1 data block = `k + 1` accesses.

**Capacity formulas (h = number of levels, i.e. root level + … + leaf level):**

```
   MAX keys storable (all nodes full):   (P − 1) · P^(h − 1)
   MIN keys (occupancy rules):           ≈ 2 · ⌈P/2⌉^(h − 1) − 1   (leaf-level accounting; conventions vary — read the question!)
```

**Why B/B+ trees and not AVL/BST (PW §4.7.1):** one key per node wastes a whole block → enormous height → enormous I/O; packing ~100 keys per node makes trees 2–4 levels deep for millions of records. That's the entire point.

**B vs B+ when block sizes are equal (PW Important Points 2–3):**

```
   order(B tree) < order(B+ tree)                        (B tree spends bytes on record pointers)
   #blocks(B tree) ≥ #blocks(B+ tree)   for the same n keys
   I/O cost(B tree) ≥ I/O cost(B+ tree) for range scans
```

---

## 6. Worked recipes (write these like a robot on exam day)

**Recipe A — order of a B tree from sizes.**
`B = 2048 B`, `BP = 8`, `K = 12`, `RP = 6`:
`8P + 18(P − 1) ≤ 2048 → 26P ≤ 2066 → P = 79` (max keys per node = 78).

**Recipe B — B+ internal vs leaf order.**
`B = 4096`, `K = 16`, `BP = 8`, `RP = 8`:
internal: `8P + 16(P−1) ≤ 4096 → P = 171`; leaf: `24(P−1) + 8 ≤ 4096 → P − 1 = 170` keys.
So internal order 171, leaf capacity **170 keys**. A 3-level B+ tree holds at most `171 × 171 × 170 = 4,970,970` keys.

**Recipe C — minimum keys from occupancy.**
Same B tree as Recipe A (`P = 79`, min pointers `⌈P/2⌉ = 40`, min keys per node `= 39`), height 3:
root 1 key; level 2: ≥ 2 nodes × 39 keys; level 3: ≥ 2 × 40 = 80 nodes × 39 keys ⇒
`1 + 78 + 3120 = 3199` keys minimum.

**Recipe D — multilevel index cost.**
Index entries 12 B in 512 B blocks (`BF = ⌊480/12⌋ = 40`), dense index over 100,000 records:
level sizes `2500 → 63 → 2 → 1` ⇒ 4 index levels ⇒ any lookup = `4 + 1 = 5` block reads.

**Recipe E — range scan on a B+ tree.**
3 levels, leaves hold 170 entries, no blocks cached, range covers exactly 500 keys:
search path (root, internal, first leaf) = 3 blocks; remaining `⌈500/170⌉ − 1 = 2` leaves via sibling links ⇒ **5 index blocks**. (Add one data block per record only for a secondary index.)

---

## 7. Trap sheet

| # | Trap | Fact |
|---|---|---|
| 1 | Blocking factor rounding | **Floor** for unspanned (records can't split), **ceil** for spanned/byte packing |
| 2 | Header | subtract it *before* dividing; default 0 |
| 3 | Sparse index preconditions | requires the **data file ordered** on that key |
| 4 | Secondary over key | **always dense**; over non-key also dense |
| 5 | Primary vs clustering | mutually exclusive on one file (both need the physical order) |
| 6 | B tree order equation | record pointers appear in EVERY node (`K + RP` per key slot) |
| 7 | B+ leaf equation | leaf has ONE extra block pointer (next leaf) separate from the P pointer budget |
| 8 | B+ internal keys | are copies/separators — not stored record pointers; internal order computed separately from leaf order |
| 9 | "Height h" counting | confirm whether h counts root level only or all levels — GATE always specifies; match it |
| 10 | Min-keys questions | must state occupancy + split conventions; plug them in, never assume |
| 11 | Range-scan counting | count each leaf ONCE (the first leaf is part of the search path) |
| 12 | Max-keys questions | assume all nodes completely full — it's just `(P−1)·P^(h−1)` |

---

## 8. Extra drills you can do in 60 seconds

```
   1. File of 30,000 records × 100 B, blocks 1024 B, header 40 B, unspanned:
        BF = ⌊984/100⌋ = 9 ; blocks = ⌈30000/9⌉ = 3334 ; 1 spare byte per record already counted.
   2. Index entries 15 B, same blocks: BF = ⌊984/15⌋ = 65 ; dense index over 3334 records:
        ⌈3334/65⌉ = 52 index blocks → next level ⌈52/65⌉ = 1  ⇒ 2-level index ⇒ lookups = 2 + 1 = 3.
   3. B+ tree with B = 512, K = 8, BP = 4, RP = 4:
        internal: 4P + 8(P−1) ≤ 512 → P = 43 ; leaf: 12(P−1) + 4 ≤ 512 → 42 keys per leaf.
```

*(These follow the exact recipes above; if your numbers differ, re-check floor/ceil and the header.)*

---

# Practice set — DBMS Notes 03

*Numeric answers computed exactly; B+ insertion answer verified by simulation. Conventions are stated inside every question.*

### Q1 · NAT · Fundamental

A static hash file uses `h(k) = k mod 7` with bucket capacity 2 records. Overflow records are chained. The keys `23, 16, 42, 8, 35, 29, 50` are inserted (in this order). The number of records that end up in **overflow** (i.e., beyond the first two records of a bucket) is ______.

### Q2 · NAT · Fundamental

Index entries are 8-byte keys + 4-byte pointers, stored in blocks of 512 bytes with a 32-byte header. For a data file of 100,000 records, a **dense** index needs ______ blocks at its first (lowest) level.

### Q3 · NAT · Standard

For the index in Q2, the first level has the number of blocks you computed, and each higher level is built with the same block capacity until a single block remains. The number of block accesses to fetch one record through this multilevel index (counting every index block and the final data block) is ______.

### Q4 · NAT · Standard

A B+ tree is stored in 4096-byte blocks. Keys are 16 bytes, block pointers 8 bytes, record pointers 8 bytes. Assume: internal nodes use `P·BP + (P−1)·K ≤ Block`, leaf nodes use `(P−1)(K + RP) + BP ≤ Block` (the extra pointer is the next-leaf link), and "height 3" means root + one internal level + leaf level, with every node completely full. The maximum number of keys the tree can hold is ______.

### Q5 · NAT · Moderate

A B tree (record pointers in every node) is stored in 2048-byte blocks; keys are 12 bytes, block pointers 8 bytes, record pointers 6 bytes. Conventions: every node except the root has at least `⌈P/2⌉` block pointers and `⌈P/2⌉ − 1` keys; the root has at least 2 block pointers and 1 key; all leaves at the same level. The **minimum** number of keys in such a tree of height 3 is ______.

### Q6 · NAT · Moderate

Using the full 3-level B+ tree of Q4 (leaves hold 170 entries each, 3 index levels, nothing cached), a query fetches all keys in a range that contains exactly 500 keys. Counting each index block once, the number of index blocks read is ______.

### Q7 · MCQ · Standard

Why does a multilevel index reduce the cost of a record lookup?

- (A) It stores fewer index entries than a single-level index on the same file
- (B) It removes the need for the underlying data file to be ordered
- (C) Its top levels are few blocks, so they can be kept in memory / read cheaply; each level then costs about one block read, giving ≈ (number of index levels + 1) accesses
- (D) It can only be built over secondary (non-key) indexes, which are smaller

### Q8 · NAT · Moderate

A data file has 100,000 records stored with a blocking factor of 25 records per block. Index entries have the same size as in Q2 (block factor 40). A **sparse primary index** on the ordering key needs ______ blocks at its first level.

### Q9 · MSQ · Standard

Which of the following statements are TRUE?

- (A) With equal block size, the maximum order of an internal B+ tree node is larger than that of a B tree node
- (B) For the same number of keys and equal block size, a B+ tree needs at most as many index blocks as a B tree
- (C) With the same node size and height, a B tree stores more keys than a B+ tree
- (D) A successful search in a B tree can locate a record pointer before reaching a leaf, while a B+ tree keeps record pointers only in leaves

### Q10 · NAT · Killer

Keys `1, 2, 3, …, 15` are inserted in ascending order into an initially empty B+ tree with **at most 4 keys per node** (order 5). Split rule to use: a node overflows at 5 keys and splits into a **left node with the first ⌊5/2⌋ = 2 keys** and a right node with the remaining 3; in a leaf, a copy of the first key of the right node is promoted into the parent; in an internal node, the middle key moves up. The total number of nodes (internal + leaf) in the final tree is ______.

---

## Answer key (quick)

| Q | Ans | Q | Ans |
|---|---|---|---|
| 1 | **1** | 6 | **5** |
| 2 | **2500** | 7 | **C** |
| 3 | **5** | 8 | **100** |
| 4 | **4,970,970** | 9 | **A, B, D** |
| 5 | **3199** | 10 | **10** |

---

## Detailed solutions

**Q1.** Buckets (`k mod 7`):
```
   42, 35 → bucket 0 : 2 records (full, no overflow)
    8, 29, 50 → bucket 1 : 3 records → 1 overflows
   23, 16 → bucket 2 : 2 records (full)
```
Overflow records = **1**.

**Q2.** `BF_index = ⌊(512 − 32)/(8 + 4)⌋ = ⌊480/12⌋ = 40`. Dense index ⇒ one entry per record: `⌈100,000/40⌉ =` **2500** blocks.

**Q3.** Level sizes: `2500 → ⌈2500/40⌉ = 63 → ⌈63/40⌉ = 2 → ⌈2/40⌉ = 1`. Four index levels (the last is the root). Lookup = every level visited once + the data block = `4 + 1 =` **5** block accesses.

**Q4.** 
- Internal: `8P + 16(P−1) ≤ 4096 → 24P ≤ 4112 → P = 171`.
- Leaf: `24(P−1) + 8 ≤ 4096 → P − 1 ≤ 170.33 → 170 keys`.
- Height 3 with all nodes full: root has 171 children → 171 internal children, each with 171 children → `171 × 171 = 29,241` leaves × 170 keys = **4,970,970**.

**Q5.** `8P + 18(P−1) ≤ 2048 → 26P ≤ 2066 → P = 79`. Min pointers per non-root node `= ⌈79/2⌉ = 40` ⇒ min keys per node `= 39`.
Height 3 minimum: root = 1 key; level 2 = 2 nodes × 39 = 78 keys; level 3 = `2 × 40 = 80` nodes × 39 = 3120 keys. Total `1 + 78 + 3120 =` **3199**.

**Q6.** Search path to the first leaf = root + internal + leaf = 3 index blocks. The 500 keys span `⌈500/170⌉ = 3` leaves, so 2 further leaves are read via sibling pointers. Total index blocks = `3 + 2 =` **5**. (Counting each leaf once — the first leaf is already part of the path.)

**Q7.** **C** — the number of entries is the same as a single-level index (only the *structure* changes); the win is that the top levels are tiny (level sizes shrink by the fan-out each time), so reaching a record costs about one block per level. A and B are false by definition; D is backwards (primary/secondary indexes are orthogonal to "multilevel").

**Q8.** Data blocks `= ⌈100,000/25⌉ = 4000`. Sparse primary index entry per data block ⇒ 4000 entries ÷ 40 per block `=` **100** blocks. (Contrast: dense needs 2500 — sparse is 25× smaller here, exactly the blocking factor of the data file.)

**Q9.** 
- (A) ✔ — a B tree node spends `K + RP` bytes per key slot; a B+ internal node spends only `K` → larger P fits.
- (B) ✔ — PW Important Point 2: for equal block sizes, `#blocks(B tree) ≥ #blocks(B+ tree)`.
- (C) ✘ — the opposite: a B+ tree's larger order lets it store *more* keys at the same height.
- (D) ✔ — B tree carries record pointers at all levels, so a search may finish early (best case: root); B+ always descends to a leaf.

**Q10.** Simulation (keys 1…15, max 4 keys/node, stated split rule):
```
   1–4 : leaf [1,2,3,4]
   5   : split → [1,2] | [3,4,5]; root [3]
   6,7 : [3,4,5,6] then split → [3,4] | [5,6,7]; root [3,5]
   8,9 : [5,6,7,8] then split → [5,6] | [7,8,9]; root [3,5,7]
   10,11: [7,8,9,10] then split → [7,8] | [9,10,11]; root [3,5,7,9]
   12,13: [9,10,11,12] then split → [9,10] | [11,12,13]; root [3,5,7,9,11] overflows
          → internal split: left [3,5], up 7, right [9,11]; new root [7]
   14,15: right side splits once more → leaves [11,12] | [13,14,15]
   final: root [7]; internals [3,5] and [9,11,13]; leaves [1,2][3,4][5,6][7,8][9,10][11,12][13,14,15]
```
Internal nodes = 3, leaves = 7 ⇒ total = **10** nodes (height 3). *(Different split conventions — e.g. left node takes 3 keys — give a different tree, which is why the problem states the rule explicitly.)*

---

*Cross-practice: `DB_08` (blocking factors, dense/sparse, clustered/secondary costs), `DB_09` (B/B+ orders, min keys, insertion node counts). Next file: transactions, concurrency control & recovery.*
