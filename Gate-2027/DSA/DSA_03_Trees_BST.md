# Data Structures & Algorithms → Trees & Binary Search Trees

**GATE 2027 syllabus:** Programming and Data Structures — trees, binary search trees.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** Height of a single-node tree is 0; height of an empty tree is −1. A BST is built by inserting keys one at a time in the given order, using the standard rule (go left if smaller, right otherwise; new key becomes a leaf). AVL insertion uses the standard single/double rotations (LL → single right rotation, RR → single left rotation, LR and RL → double rotations).

---

## Questions

### Q1 · MCQ · Fundamental
In a binary search tree, the **inorder** traversal always visits the keys

- (A) in no particular guaranteed order
- (B) in non-decreasing (sorted) order
- (C) from the root outward, level by level
- (D) in the exact order the keys were inserted

---

### Q2 · NAT · Fundamental
A BST is built by inserting the keys 45, 20, 60, 10, 30, 50, 70, 5, 15, 25, 35 in this order. The height of the resulting tree is ______.

---

### Q3 · MSQ · Standard
Which of the following statements about binary search trees is/are TRUE?

- (A) The tree's shape (and hence its height) depends on the order in which keys are inserted, not just on the set of keys.
- (B) Searching, inserting and deleting are all O(h), where h is the current height of the tree.
- (C) An inorder traversal followed by a preorder traversal together always determine the tree uniquely, for any binary tree (not just a BST).
- (D) The predecessor of a node with a left child is the maximum key in that node's left subtree.

---

### Q4 · NAT · Standard
The number of structurally distinct binary search trees possible on 7 distinct keys is ______.

---

### Q5 · MCQ · Standard
An AVL tree's balance factor at a node is defined as (height of left subtree) − (height of right subtree). A node becomes unbalanced with its left child's **right** subtree being the taller one that caused the imbalance. Which rotation restores balance?

- (A) Single left rotation
- (B) Single right rotation
- (C) Left-Right (LR) double rotation
- (D) Right-Left (RL) double rotation

---

### Q6 · NAT · Moderate
Keys 10, 20, 30, 40, 50, 25 are inserted **in this order** into an initially empty AVL tree, with rebalancing after every insertion. The number of insertions (out of these 6) that trigger at least one rotation is ______.

---

### Q7 · MSQ · Moderate
Which of the following statements is/are TRUE about the minimum possible number of nodes in an AVL tree of height h (h ≥ 0), where N(h) = N(h−1) + N(h−2) + 1, N(0) = 1, N(−1) = 0?

- (A) N(h) grows exponentially in h (specifically, on the order of φ^h for the golden ratio φ).
- (B) N(4) = 12.
- (C) The maximum possible number of nodes in an AVL tree of height h is 2^(h+1) − 1, same as for any binary tree of that height.
- (D) An AVL tree of height h can have as few as h + 1 nodes.

---

### Q8 · NAT · Moderate
The 6 keys inserted in Q6 (10, 20, 30, 40, 50, 25) produce a final AVL tree. Its height (root height, single node = 0) is ______.

---

### Q9 · MSQ · Hard
Which of the following statements about B-trees (as a generalization, distinct from binary search trees) and self-balancing binary search trees (like AVL or red-black trees) is/are TRUE?

- (A) A red-black tree guarantees height O(log n), though typically with a larger constant than a perfectly balanced tree.
- (B) An AVL tree is always at least as strictly balanced as a red-black tree with the same keys, meaning its height is never larger.
- (C) Red-black trees generally require fewer rotations per insertion/deletion than AVL trees on average, making them common in practice for frequently-mutated structures.
- (D) A B-tree of minimum degree t ≥ 2 has height O(log_t n).

---

### Q10 · NAT · Killer
A binary tree's **preorder** traversal is 40, 20, 10, 5, 30, 25, 60, 50, 45, 70, 65 and its **inorder** traversal is 5, 10, 20, 25, 30, 40, 45, 50, 60, 65, 70 (this is in fact a BST built by inserting 40, 20, 60, 10, 30, 50, 70, 5, 25, 45, 65 in order). The number of nodes in this tree that have **exactly one child** is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | B | Defining property of a BST's inorder traversal |
| 2 | 3 | Verified by construction: levels 45 / 20,60 / 10,30,50,70 / 5,15,25,35 |
| 3 | A, B, C, D | All four are standard, correct facts (preorder + inorder together always determine a unique binary tree when all keys are distinct) |
| 4 | 429 | Catalan number C₇ |
| 5 | C | Left child's right subtree is heavier: LR double rotation |
| 6 | 3 | Rotations occur when inserting 30, 40 and 25 (verified by simulation); inserting 10, 20, 50 triggers none |
| 7 | A, B, D | (C) is false — the maximum is 2^(h+1) − 1 for any binary tree, but an AVL tree cannot achieve that except for very small h; the real max for large h is far less than 2^(h+1)−1 |
| 8 | 2 | Final tree (verified): root 20, left 10, right subtree rooted at 30 with left 25 and right 40 (whose right child is 50) |
| 9 | A, C, D | (B) is false — a red-black tree can be up to about twice as tall as an AVL tree on the same keys |
| 10 | 4 | The tree is a zig-zag chain: nodes 10, 30, 50 and 70 each have exactly one child; the rest are leaves or have two children |
