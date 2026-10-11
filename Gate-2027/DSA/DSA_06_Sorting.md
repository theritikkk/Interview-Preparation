# Data Structures & Algorithms → Sorting

**GATE 2027 syllabus:** Algorithms — searching, sorting, hashing.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** "Comparison" means one element-to-element `<` or `>` comparison. Insertion sort scans right-to-left within the sorted prefix, stopping as soon as it finds an element ≤ the key. Quicksort partitions using the stated pivot rule; Lomuto-style partitioning (elements strictly less than pivot go left, others go right) is used, and the pivot is placed once between the two partitions. Merge sort compares one element from each half per merge step until one half is exhausted.

---

## Questions

### Q1 · MCQ · Fundamental
Which one of the following sorting algorithms is **not** stable in its standard, textbook implementation?

- (A) Insertion sort
- (B) Bubble sort
- (C) Selection sort
- (D) Merge sort

---

### Q2 · NAT · Fundamental
The array [38, 27, 43, 3, 9, 82, 10] is sorted using **insertion sort**. The total number of key-to-element comparisons performed is ______.

---

### Q3 · MSQ · Standard
Which of the following statements about sorting algorithms is/are TRUE?

- (A) Selection sort always performs exactly n(n−1)/2 comparisons, regardless of the input order.
- (B) Selection sort performs at most n − 1 swaps.
- (C) Insertion sort's comparison count depends on the input order (fewer comparisons on a nearly-sorted array).
- (D) Bubble sort can be modified with an early-exit flag to run in O(n) time on an already-sorted array.

---

### Q4 · NAT · Standard
The array [38, 27, 43, 3, 9, 82, 10] is sorted using **merge sort** (standard top-down, splitting into two roughly equal halves). The total number of comparisons performed during all the merge steps is ______.

---

### Q5 · MCQ · Standard
Quicksort with the **last element** as pivot (Lomuto partition) is run on an array that is already sorted in increasing order. Its time complexity for this specific input is

- (A) O(n)
- (B) O(n log n)
- (C) O(n²)
- (D) O(log n)

---

### Q6 · NAT · Moderate
The array [38, 27, 43, 3, 9, 82, 10] is sorted using **quicksort with the last element as the pivot** at every recursive call (Lomuto partition). The total number of comparisons performed is ______.

---

### Q7 · MSQ · Moderate
Which of the following statements about the relationship between an array's **number of inversions** (pairs i < j with a[i] > a[j]) and sorting is/are TRUE?

- (A) Insertion sort's number of element shifts equals the number of inversions in the input array.
- (B) An array with zero inversions is already sorted.
- (C) Any comparison-based sorting algorithm must perform at least as many comparisons as there are inversions, in the worst case over all inputs of that size.
- (D) The maximum possible number of inversions in an array of n distinct elements is n(n−1)/2.

---

### Q8 · NAT · Moderate
Quicksort uses the **median-of-three** pivot rule (median of first, middle and last elements) instead of always picking the last element, on the same already-sorted array [1, 2, 3, 4, 5, 6, 7]. The total number of comparisons performed (comparisons used to find the median-of-three itself are not counted, only the partitioning comparisons) is ______.

---

### Q9 · MSQ · Hard
Which of the following statements about **heapsort** is/are TRUE?

- (A) It runs in O(n log n) time in the worst case.
- (B) It is not stable in its standard implementation.
- (C) It uses O(1) additional space (in-place), unlike standard merge sort.
- (D) Building the initial heap (heapify) takes O(n log n) time in the worst case, which dominates heapsort's overall running time.

---

### Q10 · NAT · Killer
The array [38, 27, 43, 3, 9, 82, 10] is sorted using **heapsort** (build a max-heap bottom-up, then repeatedly swap the root with the last unsorted element and sift down). The total number of key comparisons performed across the build-heap phase and the extraction phase combined is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | C | Selection sort can swap a later-equal element past an earlier one |
| 2 | 15 | Verified by simulation: the running comparison count while inserting each element |
| 3 | A, B, C, D | All four are standard, correct facts about these algorithms |
| 4 | 13 | Verified by simulating the merge steps on the split [38,27,43] / [3,9,82,10] |
| 5 | C | Every partition is maximally unbalanced (pivot is always the largest remaining), giving n(n−1)/2 comparisons |
| 6 | 13 | Verified by simulating Lomuto partitioning with the last-element pivot throughout |
| 7 | A, B, C, D | All four are standard, correct facts relating inversions to comparison-based sorting |
| 8 | 10 | Verified by simulation: median-of-three avoids the worst-case unbalanced splits that plague the last-element rule on sorted input |
| 9 | A, B, C | (D) is false — build-heap is O(n), not O(n log n); it is the n extractions, each O(log n), that dominate |
| 10 | 19 | Verified by simulation: comparisons made while sifting down during both the build-heap phase and the n − 1 extractions |
