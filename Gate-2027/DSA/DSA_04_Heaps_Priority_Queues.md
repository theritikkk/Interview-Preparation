# Data Structures & Algorithms → Binary Heaps & Priority Queues

**GATE 2027 syllabus:** Programming and Data Structures — binary heaps.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** A binary heap is stored in an array, 0-indexed, with children of index i at 2i+1 and 2i+2 (equivalently 1-indexed with children 2i, 2i+1). "Build-heap" means the standard bottom-up heapify (sift-down from the last internal node to the root), not repeated insertion. A min-heap satisfies parent ≤ children.

---

## Questions

### Q1 · MCQ · Fundamental
In an array-based binary min-heap with n elements, the parent of the element at (0-indexed) index i (i > 0) is located at index

- (A) i / 2 (integer division)
- (B) (i − 1) / 2 (integer division)
- (C) 2i
- (D) i − 1

---

### Q2 · NAT · Fundamental
A binary heap with n = 100 elements is stored as a complete binary tree. The number of leaf nodes is ______.

---

### Q3 · MSQ · Standard
Which of the following statements about binary heaps is/are TRUE?

- (A) Build-heap (bottom-up heapify) runs in O(n) time in the worst case, not O(n log n).
- (B) Finding the maximum element in a min-heap takes O(1) time in general.
- (C) A heap can be used to implement heapsort in O(n log n) time in place.
- (D) Every level-order traversal of a min-heap's array is also a valid "sorted-enough" sequence in the sense that no element is smaller than its parent.

---

### Q4 · NAT · Standard
The array [4, 10, 3, 5, 1, 15, 9, 2, 8, 7] is converted into a min-heap using the standard bottom-up build-heap procedure. The value at index 0 (the root) of the resulting heap array is ______.

---

### Q5 · MCQ · Standard
Extracting the minimum from a binary min-heap of n elements (removing the root, moving the last element to the root, then sifting down) takes worst-case time

- (A) O(1)
- (B) O(log n)
- (C) O(n)
- (D) O(n log n)

---

### Q6 · NAT · Moderate
The number of distinct min-heap arrangements (as arrays) possible using the 7 distinct keys {1, 2, …, 7}, all placed in a complete binary tree of 7 nodes, is ______.

---

### Q7 · MSQ · Moderate
Four sorted lists, each of length m, are merged into one sorted list of length 4m using a min-heap of size 4 (holding the current front element of each list). Which of the following statements about this k-way merge (here k = 4) is/are TRUE?

- (A) Each of the 4m output elements requires one extract-min and (except for the very last extraction) one subsequent insert, each costing O(log 4) = O(1) here since k is fixed.
- (B) The total time to merge is O(4m) when k = 4 is treated as a constant.
- (C) For general k lists of total length N, the merge takes O(N log k) time using a heap of size k.
- (D) A k-way merge using a heap is asymptotically worse than repeatedly doing pairwise 2-way merges of the k lists.

---

### Q8 · NAT · Moderate
The number of distinct min-heap arrangements possible using 10 distinct keys placed in a complete binary tree of 10 nodes is ______.

---

### Q9 · MSQ · Hard
Which of the following statements about a **max-heap** with n distinct elements is/are TRUE?

- (A) The second-largest element must be one of the root's two children.
- (B) The smallest element must be at one of the leaves.
- (C) Any root-to-leaf path, read from root to leaf, is in non-increasing order.
- (D) The third-largest element could be at depth 2 (root is depth 0) but can never be at the root or a child of the root... i.e., it is always at depth ≥ 2.

---

### Q10 · NAT · Killer
The number of distinct min-heap arrangements possible using 15 distinct keys {1, 2, …, 15} placed in a **complete** binary tree of 15 nodes (a perfect binary tree of height 3) is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | B | Standard 0-indexed parent formula |
| 2 | 50 | ⌈n/2⌉ leaves for a complete binary tree of 100 nodes |
| 3 | A, C, D | (B) is false — the maximum of a min-heap must be a leaf, and there are about n/2 leaves, so finding it needs O(n) in general; (D) is just the min-heap invariant restated, which is true by definition |
| 4 | 1 | Verified by simulating the standard build-heap sift-down procedure |
| 5 | B | One sift-down, height O(log n) |
| 6 | 80 | n! divided by the product of every node's subtree size, for n = 7 |
| 7 | A, B, C | (D) is false — a heap-based k-way merge is asymptotically at least as good as, typically better than, naive repeated pairwise merging |
| 8 | 3360 | Same formula as Q6, evaluated for n = 10 |
| 9 | A, B, C | (D) is false — the third-largest element can be a direct child of the root (e.g. heap array 10,9,8,7,6,5,4), so it is not always confined to depth ≥ 2 |
| 10 | 21964800 | Same formula as Q6/Q8, evaluated for n = 15 |
