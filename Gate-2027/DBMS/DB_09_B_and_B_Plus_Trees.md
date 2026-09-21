# DBMS → B and B+ Trees

**GATE 2027 syllabus:** Databases — indexing (e.g., B and B+ trees).
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:**

- B+ tree node structure: an internal node holds up to n keys and n + 1 tree pointers; a leaf holds up to n_L keys, each with a record pointer, plus one next-leaf pointer. Nodes are disk blocks.
- Minimum occupancy: a non-root internal node has at least ⌈(n + 1)/2⌉ children; a non-root leaf has at least ⌈n_L/2⌉ keys; the root of a tree with more than one node has at least 2 children.
- **Insertion rules used in Q5, Q6, Q10** (trees with at most 3 keys per node, initially empty): a leaf that would hold 4 keys is split — the first 2 keys stay, the last 2 go to a new right leaf, and the smallest key of the right leaf is **copied** to the parent. An internal node that would hold 4 keys is split — the first 2 keys stay, the 3rd key **moves up** to the parent, and the 4th key goes to a new right node. Keys equal to a separator go to the right subtree.

---

## Questions

### Q1 · NAT · Fundamental
A B+ tree is stored in 1024-byte blocks. A search key is 10 bytes, a tree (block) pointer is 8 bytes and a record pointer is 12 bytes. The maximum number of keys in an internal node is ______.

---

### Q2 · NAT · Fundamental
With the sizes of Q1, a leaf node holds n_L keys, n_L record pointers and one next-leaf pointer. The minimum number of keys in a non-root leaf node is ______.

---

### Q3 · NAT · Standard
Using the node capacities of Q1–Q2 (internal: 56 keys; leaf: as computed in Q2), the minimum number of levels (counting root and leaf levels) of a B+ tree that stores 1,000,000 keys is ______.

---

### Q4 · MSQ · Standard
Which of the following statements is/are TRUE?

- (A) The leaf nodes of a B+ tree are linked to support range queries.
- (B) In a B tree, internal nodes can hold data (record) pointers.
- (C) In a B+ tree, every key that appears in an internal node also appears in some leaf.
- (D) Range queries are faster on a B tree than on a B+ tree.

---

### Q5 · NAT · Standard
The keys 1, 2, 3, …, 12 are inserted in this (ascending) order into an initially empty B+ tree with at most 3 keys per node. The total number of nodes (internal and leaf) in the final tree is ______.

---

### Q6 · NAT · Moderate
The keys 20, 10, 30, 5, 15, 25, 35, 3, 7, 12, 17 are inserted in this order into an initially empty B+ tree with at most 3 keys per node. The total number of nodes in the final tree is ______.

---

### Q7 · MSQ · Moderate
Which of the following statements is/are TRUE?

- (A) All leaf nodes of a B+ tree are at the same depth.
- (B) When a key is deleted from a B+ tree, its copy in an internal node is always removed as well.
- (C) The height of a B+ tree that stores N keys is O(log N).
- (D) In a B tree of minimum degree t, every non-root node has at least t − 1 keys.

---

### Q8 · NAT · Moderate
A B tree has minimum degree t = 3 (every non-root node has at least 2 keys and at least 3 children; at most 5 keys). The root is at depth 0 and all leaves are at depth 3. The minimum possible number of keys in the tree is ______.

---

### Q9 · NAT · Hard
Using the capacities of Q1–Q2 (non-root internal nodes: at least ⌈57/2⌉ = 29 children; non-root leaves: at least ⌈n_L/2⌉ keys; root with at least 2 children), the minimum number of keys that a B+ tree with exactly **3 levels** (root, one internal level, leaves) can store is ______.

---

### Q10 · NAT · Killer
The keys 1, 2, 3, …, 30 are inserted in ascending order into an initially empty B+ tree with at most 3 keys per node. The total number of nodes (internal and leaf) in the final tree is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | 56 | (n + 1)·8 + n·10 ≤ 1024, so 18n ≤ 1016 |
| 2 | 23 | n_L·(10 + 12) + 8 ≤ 1024 gives n_L = 46; minimum ⌈46/2⌉ |
| 3 | 4 | Capacity with 3 levels: 57² × 46 = 149,454 < 10⁶; with 4 levels 57³ × 46 = 8,518,878 |
| 4 | A, B, C | Range scans follow the leaf chain in a B+ tree |
| 5 | 9 | 6 leaves ([1,2] … [11,12]), 3 internal nodes |
| 6 | 8 | 5 leaves, 3 internal nodes; root [20], internal [10,15] and [30] |
| 7 | A, C, D | (B): a separator may stay after its key is deleted from the leaf |
| 8 | 53 | Nodes: 1 + 2 + 6 + 18 = 27; keys 1 + 26·2 |
| 9 | 1334 | Root 2 children, each with 29 children: 58 leaves; 58 × 23 |
| 10 | 23 | 15 leaves, 8 internal nodes, 4 levels |
