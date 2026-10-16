# DSA Notes 02 — Trees: Traversals, BST, Heaps, AVL (High-Yield Deep Dive)

> **Why this file exists:** "Trees" is on the high-yield list and GATE's most reliable DSA hunting ground: counting formulas, traversal reconstruction, BST operations, and heap/AVL properties. The counting questions are all one of five derivations — do those once and the topic is banked. All numeric practice answers verified by simulation.
>
> **Companion question banks:** `DSA_03_Trees_BST.md`, `DSA_04_Heaps_Priority_Queues.md`. PW source: `../Reference/PW_Subject_Handbooks/PW_Programming_DS.txt` (§8).

---

## 0. The mental model and the definitions that must be exact

A binary tree: each node has ≤ 2 children. Vocabulary edge cases GATE exploits (state the convention in your answer):

| Term | Definition | Trap |
|---|---|---|
| **Binary tree** | each node has 0, 1, or 2 children | — |
| **Full / strict** | every node has **0 or 2** children | height is NOT fixed by node count |
| **Complete** | all levels full except possibly the last, last level filled **left to right** | array-representable, no gaps |
| **Perfect** | all internal nodes have 2 children **and** all leaves at the same depth | = complete + last level full |
| **Height/Depth** | number of edges on root→deepest leaf (state "root = level 0") | some books use node-count; the question usually states which |

**Relationships (derivations, not memorization):**

- **Perfect tree, height h:** levels 0..h hold 1+2+4+…+2^h = **2^(h+1) − 1** nodes.
- **Any binary tree, height h:** max nodes = 2^(h+1) − 1 (perfect); min nodes = **h + 1** (a path).
- **Complete tree, height h:** min nodes = **2^h** (perfect through level h−1 = 2^h − 1, plus one node at level h).
- **Leaves vs 2-children nodes (full tree):** count edges. Every node except the root is someone's child, and only 2-child nodes contribute 2 children:

```
  n − 1 = 2·D  →  D = (n−1)/2,  L = n − D = D + 1
  ⇒  leaves = (nodes with exactly 2 children) + 1
```

- **Catalan numbers** — one formula, four disguises:

```
  C(n) = (1/(n+1)) · binom(2n, n)
  counts:  binary-tree SHAPES with n nodes
           BSTs on n distinct keys
           stack permutations of n elements
           valid parenthesis strings with n pairs
```

**Worked values:** C(3) = 5, C(4) = 14, C(5) = **42**, C(7) = 429.

---

## 1. Traversals and reconstruction

```
  Preorder:  root, left, right      (print on the way DOWN)
  Inorder:   left, root, right
  Postorder: left, right, root      (print on the way UP)
  Level-order: BFS with a queue
```

**The two facts that solve reconstruction questions:**

1. From **preorder + inorder**: preorder's first element = root; find it in inorder; everything left/right of it splits the subtrees. Recurse.
2. From **postorder + inorder**: same, with postorder's **last** element as root.
3. **Inorder + one of pre/post → unique tree.** Pre + post (without inorder) → NOT unique in general.
4. In a **BST**, inorder = sorted keys.

**Worked example.** Preorder `30,20,10,15,25,40,35,50` (a BST): inorder is sorted `10,15,20,25,30,35,40,50`. Root 30; left part `10,15,20,25` → root 20, its left `10,15` (root 10, right child 15), right `25`; right part `35,40,50` → root 40 with left 35, right 50. Postorder = `15,10,25,20,35,50,40,30`. *(Verified by simulation.)*

---

## 2. Binary Search Tree (BST)

**Property:** left subtree keys < node < right subtree keys (all of them, not just the children!).

| Operation | Idea | Time |
|---|---|---|
| Search | walk down using the ordering | O(h): best ⌈log₂(n+1)⌉ levels, worst **O(n)** (skewed insertion 1,2,3,…) |
| Insert | search, attach at the NULL you fall off | same |
| Delete (leaf) | remove directly | same |
| Delete (1 child) | splice: parent points to the child | same |
| Delete (2 children) | replace with **inorder successor** (min of right subtree) or predecessor, then delete that node | same |

Confirmed GATE facts: deletion order affects the final tree; both successor and predecessor replacement are valid; BST insert never restructures on its own (that's AVL/red-black).

---

## 3. Heaps and priority queues

**Definition:** a **complete** binary tree with the heap property (min-heap: parent ≤ children; max-heap: parent ≥ children). Completeness is what makes the **array representation** exact (no pointers):

```
  1-indexed: children of i = 2i, 2i+1;  parent of i = ⌊i/2⌋
  0-indexed: children of i = 2i+1, 2i+2;  parent of i = ⌊(i−1)/2⌋
```

**Properties:**

- Leaves in a heap-array of n nodes: ⌈n/2⌉ (indices ⌊n/2⌋+1 … n in 1-indexed).
- **Height** of an n-node heap = ⌊log₂ n⌋; extract-max (swap root with last, shrink, sift down) takes ≤ height swaps = **O(log n)**.
- **Heapify** (bottom-up build): O(n) — half the nodes are leaves and move 0 levels; Σ heights over nodes telescopes to O(n).
- Insert one-by-one into a heap: O(log n) each → O(n log n) for n keys (PW's contrast).
- Heapsort: build max-heap O(n), then n extracts O(n log n); in-place.
- Heap ≠ sorted: only parent-child ordering. Two different arrays can be heaps of the same set.
- **Number of heaps** with n distinct keys (shape fixed by completeness): with left-subtree size L of the root:

```
  H(n) = binom(n−1, L) · H(L) · H(n−1−L)
```

  (choose which keys go left, recursively) → H(3)=2, H(4)=3, H(5)=8, H(6)=20, H(7)=**80**.

---

## 4. AVL trees (height-balanced BST)

**Balance factor:** BF(node) = height(left) − height(right) ∈ {−1, 0, +1}.

On insert, walk up from the new node; the first ancestor with BF = ±2 is fixed by a rotation (PW's four cases):

```
  LL (Left-Left)   → single RIGHT rotation
  RR (Right-Right) → single LEFT rotation
  LR → left-rotate child, then right-rotate (double)
  RL → right-rotate child, then left-rotate (double)
```

**Minimum nodes** in an AVL tree of height h (both subtrees must themselves be AVL, with the tallest allowed pair of heights):

```
  N(h) = 1 + N(h−1) + N(h−2),  N(0) = 1, N(1) = 2
  → 1, 2, 4, 7, 12, 20, …   (closely tracks Fibonacci: N(h) ≈ φ^h)
```

**Consequence:** height of AVL with n nodes ≤ 1.44·log₂(n+2) — search stays O(log n) worst case (unlike plain BST).

---

## 5. Traps & exam patterns

1. **State the height convention** if the question doesn't; "root height 0, single node 0" is the default in modern GATE.
2. **Full vs complete vs perfect** — the question hinges on which one is meant; complete is the strictest shape-wise (and determines heights/counts).
3. **Leaves = (2-children nodes) + 1** works for *full* (and any) binary trees — but only counts 2-child nodes, not 1-child.
4. **Catalan vs powers of 2:** number of BSTs on n keys = Catalan(n), not n! and not 2^n−1.
5. **Heap array is not sorted** — inorder of a heap is meaningless; only parent-child comparisons hold.
6. **BST worst case is a chain (O(n))** — that's why AVL exists.
7. **Reconstruction needs inorder** — pre+post alone can give multiple trees.

---

## 6. Practice set (verified by simulation)

> Conventions: root at height/level 0; "height" = number of edges. Heaps are 1-indexed arrays unless stated.

### Q1 · NAT · Fundamental
A **complete** binary tree has 31 nodes. Its height is ______.

### Q2 · NAT · Fundamental
In a binary tree, every internal node has exactly 2 children. If there are 10 such nodes, the number of leaves is ______.

### Q3 · MCQ · Standard
Keys `8, 3, 10, 1, 6, 14, 4, 7, 13` are inserted **in this order** into an initially empty BST. Its **postorder** traversal is:

- (A) 1, 4, 7, 6, 3, 13, 14, 10, 8
- (B) 1, 6, 4, 7, 3, 13, 14, 10, 8
- (C) 1, 4, 7, 6, 3, 14, 13, 10, 8
- (D) 8, 3, 1, 6, 4, 7, 10, 14, 13

### Q4 · NAT · Standard
The **minimum** number of nodes in an AVL tree of height 4 is ______.

### Q5 · MCQ · Standard
A BST's preorder is `30, 20, 10, 15, 25, 40, 35, 50`. Its postorder is:

- (A) 15, 10, 20, 25, 35, 40, 50, 30
- (B) 15, 10, 25, 20, 35, 50, 40, 30
- (C) 10, 15, 25, 20, 35, 50, 40, 30
- (D) 15, 25, 10, 20, 35, 50, 40, 30

### Q6 · MCQ · Moderate
Keys `10, 5, 20, 2, 15` are inserted **one after another** into an initially empty **min-heap** (build by successive insertion). The resulting array is:

- (A) 2, 5, 10, 15, 20
- (B) 2, 5, 20, 10, 15
- (C) 2, 10, 5, 20, 15
- (D) 2, 5, 10, 20, 15

### Q7 · NAT · Moderate
A max-heap with 1000 elements is stored as a 1-indexed array. The **maximum** number of swaps performed by a single extract-max (delete the root: move the last element to the root, then sift down) is ______.

### Q8 · MSQ · Moderate
Which of the following is/are TRUE about a binary search tree with n nodes?

- (A) Its inorder traversal lists keys in sorted order.
- (B) Search and insert each take O(h) time, where h is the height.
- (C) Deleting a node with two children can be done by replacing it with its inorder successor, then deleting that successor.
- (D) In the worst case, search always takes Θ(log n) time.

### Q9 · NAT · Hard
A complete binary tree has 25 nodes. The number of nodes on its **last level** (the deepest level) is ______.

### Q10 · NAT · Killer
The number of structurally different **binary search trees** that can be built on 5 distinct keys (arranged to satisfy the BST property) is ______.

---

## Answer Key

| Q | Answer | Why (condensed) |
|---|---|---|
| 1 | 4 | 31 = 2⁵ − 1 |
| 2 | 11 | leaves = 2-child nodes + 1 |
| 3 | A | root 8; left {3,1,6,4,7}; right {10,14,13} |
| 4 | 12 | N(4)=1+N(3)+N(2) |
| 5 | B | root 30; left root 20; right root 40 |
| 6 | B | sift-ups: 2 swaps to root |
| 7 | 9 | ≤ height of 999-node heap = 9 |
| 8 | A, B, C | (D) false — skewed BST is O(n) |
| 9 | 10 | 25 − 15 |
| 10 | 42 | Catalan(5) |

### Full solutions

**Q1.** 31 = 2⁵ − 1 → perfect through level 4 → height **4**. (General: height = ⌊log₂ 31⌋ = 4.)

**Q2.** Every node except the root is a child; only 2-child nodes add 2 children: n − 1 = 2·10 → n = 21; leaves = 21 − 10 = **11** (or directly: leaves = 10 + 1).

**Q3.** Build the BST: 8 root; 3,1,6,4,7 left; 10 → right, 14 → right of 10, 13 → left of 14. Postorder (left subtree, right subtree, root): left of 8 gives **1,4,7,6,3** (4 and 7 under 6; 1 under 3); right of 8 gives **13,14,10**; root **8**. Sequence: 1,4,7,6,3,13,14,10,8 → **A**. (B is the preorder with a mislabeled pair; C swaps the 13/14 order; D is the preorder.) *(Verified by simulation.)*

**Q4.** N(0)=1, N(1)=2, N(2)=1+2+1=4, N(3)=1+4+2=7, N(4)=1+7+4=**12**. Build idea: root + AVL(height 3) + AVL(height 2).

**Q5.** For a BST, inorder = the sorted keys `10,15,20,25,30,35,40,50`. Preorder's first key 30 is the root; splitting the inorder around 30, the left block is `10,15,20,25` and the right block `35,40,50`. Recursing on the preorder blocks: left subtree root 20 (its left `10,15` → root 10 with right child 15; its right 25); right subtree root 40 (left 35, right 50). Postorder: `15,10,25,20` + `35,50,40` + `30` → **B**. *(Verified by simulation.)*

**Q6.** Insertions: 10 → [10]; 5 → [5,10]; 20 → [5,10,20]; 2 → append [5,10,20,2], sift up past 10 and 5 → [2,5,20,10]; 15 → append [2,5,20,10,15] (15 > parent 5, stop). Result: **2,5,20,10,15 (B)**. *(Verified by simulation.)*

**Q7.** Extract-max: last element (index 1000) moves to root; heap has 999 elements after removal; sift-down swaps ≤ height(999) = ⌊log₂ 999⌋ = **9**. (2⁹ = 512 ≤ 999 < 1024.)

**Q8.** A ✔, B ✔ — definitions. C ✔ — successor = min of right subtree; copy value, then delete the successor (which has ≤ 1 child — why the case collapses). D ✘ — insert 1,2,…,n gives a chain with O(n) search; that's AVL's motivation.

**Q9.** Levels 0–3 hold 1+2+4+8 = 15 nodes (perfect). The remaining 25 − 15 = **10** nodes sit on level 4 (last level, filled left to right).

**Q10.** Number of BSTs on n distinct keys = Catalan(n) = (1/(n+1))·C(2n,n). n = 5: C(10,5)/6 = 252/6 = **42**. (Choose the root's key (5 ways), then recursively left/right — that recurrence *is* the Catalan recurrence.)

---

## 7. Next steps

1. Redraw Q3's BST and produce all four traversals from the drawing.
2. Re-derive N(h) and the Catalan formula from their recurrences once.
3. Practice reps: `DSA_03_Trees_BST.md`, `DSA_04_Heaps_Priority_Queues.md`.
4. Continue to `DSA_Notes_03_Hashing.md`.
