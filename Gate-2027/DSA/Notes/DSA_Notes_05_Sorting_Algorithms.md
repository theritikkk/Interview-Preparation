# DSA Notes 05 — Sorting Algorithms: Properties, Counting & Traces (High-Yield Deep Dive)

> **Why this file exists:** "Sorting" is the first of the three video topics for Algorithms. GATE asks four kinds of questions here: (1) **trace/count** — how many comparisons/swaps/moves does this algorithm make on this array; (2) **property recall** — stable? in-place? which case is which; (3) **worst/best input recognition** — especially quicksort's behaviour on sorted input; (4) **lower bound + non-comparison sorts** — why Ω(n log n), and how counting/radix/bucket escape it. Every numeric answer below was **verified by running the algorithm** (exact simulation).
>
> **Companion question bank:** `../DSA_06_Sorting.md`. PW source: `../Reference/PW_Subject_Handbooks/PW_Algorithms.txt` (§2.5–2.7: merge, merge sort, quick sort, randomized quick sort, inversions; summary comparison table at the end of §2).

---

## 0. The mental model: three axes behind every "which sort" question

```
   (1) WHEN it does its work       → best / average / worst case
   (2) WHERE it moves data         → in-place (O(1) aux) vs outplace (O(n) aux)
   (3) HOW it treats equal keys    → stable (order preserved) vs unstable
   How many comparisons/swaps/moves depend on the INPUT:
        sorted input   → insertion/bubble are fast, quick (bad pivot) is slowest
        random input   → everyone behaves "average"
        reverse order  → everyone is near worst; insertion = maximum inversions
```

GATE's counting questions always specify **the convention**, so before counting, fix: (i) which partition scheme (Lomuto/Hoare), (ii) whether "moves" = shifts or writes, (iii) whether leftover copies after one run empties count as comparisons (they don't).

---

## 1. The master table (memorize this — it answers half the MCQs)

| Algorithm | Best | Average | Worst | Aux space | Stable | In-place | One-line reason to detect it |
|---|---|---|---|---|---|---|---|
| Bubble (with early-exit flag) | O(n) | O(n²) | O(n²) | O(1) | ✅ | ✅ | adjacent swaps = #inversions |
| Selection | O(n²) | O(n²) | O(n²) | O(1) | ❌ | ✅ | comparisons **always** n(n−1)/2 |
| Insertion | O(n) | O(n²) | O(n²) | O(1) | ✅ | ✅ | total work ∝ (n + #inversions) |
| Merge | O(n log n) | O(n log n) | O(n log n) | **O(n)** | ✅ | ❌ | only stable O(n log n) comparison sort here |
| Quick | O(n log n) | O(n log n) | **O(n²)** | O(log n) stack | ❌ | ✅ | sorted input + last-element pivot = disaster |
| Heap | O(n log n) | O(n log n) | O(n log n) | O(1) | ❌ | ✅ | build-heap is O(n), not O(n log n) |
| Counting | O(n+k) | O(n+k) | O(n+k) | O(n+k) | ✅ | ❌ | k = range of keys, no key comparisons |
| Radix LSD | — | O(d(n+b)) | O(d(n+b)) | O(n+b) | ✅ | ❌ | needs a *stable* digit sort inside |
| Bucket | — | **O(n)** uniform | O(n²) | O(n) | ✅ | ❌ | uniform key distribution assumption |

*(PW §2 summary table conventions: GATE treats merge sort as **outplace** — "In GATE exam if merge sort given then always consider outplace.")*

---

## 2. Stability — the 30-second proof for each algorithm

- **Insertion ✅** — an element is placed *after* equal elements already in the sorted prefix (condition is `a[j] > key`, not `≥`).
- **Bubble ✅** — swaps only when strictly greater, so equal keys never cross.
- **Merge ✅** — when `L[i] == R[j]`, take from the **left** run first (`<=`). Equal keys from the left half (earlier positions) stay first.
- **Selection ❌** — the swap that brings the minimum to the front can teleport an element far to the right, past equal keys. Witness: `[3ᵃ, 3ᵇ, 1]` → swap 1 to front → `[1, 3ᵇ, 3ᵃ]` (the two 3s reversed).
- **Quick ❌** — partition swaps distant elements; equal keys get shuffled.
- **Heap ❌** — root/last swap moves records long distances; equal keys re-order.

**Why "stable" matters at all:** sorting records by (key, extra fields) — stability keeps the previous order for ties, letting you chain sorts (sort by secondary key, then by primary key).

---

## 3. The elementary sorts (with exact counting rules)

### 3.1 Insertion sort

```
for i = 1 .. n−1:
    key = a[i];  j = i−1
    while j ≥ 0 and a[j] > key:        # each check = 1 comparison
        a[j+1] = a[j]; j−−             # each check that passes = 1 shift
    a[j+1] = key
```

Counting identities (state them in solutions):

```
   #shifts      = #inversions of the input array
   #comparisons = #shifts + (#iterations where the scan stopped because a[j] ≤ key)
                = #inversions + (n−1) − (#scans that ran off the start)
   total work   = Θ(n + #inversions)   →  best n−1 comps (sorted), worst n(n−1)/2 (reverse)
```

**Worked trace — `[29, 14, 37, 8, 21, 17]` (convention: comparison happens whenever `j ≥ 0`, before deciding shift/stop):**

| i | key | comparisons this pass | what happens | array after |
|---|---|---|---|---|
| 1 | 14 | 1 (`29>14`, shift; scan hits start) | 1 shift | `[14, 29, 37, 8, 21, 17]` |
| 2 | 37 | 1 (`29<37`, stop) | 0 shifts | unchanged |
| 3 | 8 | 3 (`37,29,14` all shift; hits start) | 3 shifts | `[8, 14, 29, 37, 21, 17]` |
| 4 | 21 | 3 (`37,29` shift; `14<21` stop) | 2 shifts | `[8, 14, 21, 29, 37, 17]` |
| 5 | 17 | 4 (`37,29,21` shift; `14<17` stop) | 3 shifts | `[8, 14, 17, 21, 29, 37]` |

Total comparisons = 1+1+3+3+4 = **12**; total shifts = 9 = #inversions of the input ✔ (check: 29>14, 29>8, 29>21, 29>17, 14>8, 37>8, 37>21, 37>17, 21>17 = 9).

### 3.2 Selection sort

- Pass i scans the remaining `n−1−i` elements to find the minimum → comparisons = Σ (n−1−i) = **n(n−1)/2 exactly, for every input**.
- Swaps: at most n−1 (exactly n−1 if you always perform the swap, even a self-swap).
- No adaptivity (sorted input costs the same), not stable.

### 3.3 Bubble sort

- Pass k pushes the k-th largest to its final slot; comparisons n(n−1)/2 in the plain version.
- **Adjacent swaps = #inversions** (each swap kills exactly one inversion). With the early-exit flag, already-sorted input = n−1 comparisons, zero swaps → O(n).

---

## 4. Merging and merge sort — exact cost formulas

**Merging two sorted runs of sizes m and n** (the atomic operation — PW §2.5):

```
   worst-case comparisons = m + n − 1     (one run is drained one element before the other)
   best-case  comparisons = min(m, n)     (one run exhausts immediately, e.g. all of L < all of R)
   moves                  = m + n         (ALWAYS — every element is copied once, outplace)
```

**Merge sort** (top-down, split at mid = ⌊n/2⌋):

- Recurrence `T(n) = 2T(n/2) + Θ(n)` → **Θ(n log n) in all cases** (master theorem case 2). Work per *level* of the recursion tree = Θ(n); number of levels = ⌈log₂ n⌉.
- **Total moves** for n elements = `n · ⌈log₂ n⌉` — because every level moves all n elements once. (n = 8 → 3 levels × 8 = **24 moves**.)
- Stable (see §2), **outplace** — Θ(n) auxiliary buffer (GATE convention).
- PW rule of thumb: large n → merge sort; very small n → insertion sort is faster in practice.

---

## 5. Quicksort — partition cost is the whole story

Lomuto partition on a subarray of size k performs exactly **k − 1 comparisons** (each non-pivot element is compared with the pivot exactly once).

```
   T(n) = T(k−1) + T(n−k) + Θ(n)     where k = pivot's final rank

   balanced (k ≈ n/2):        T(n) = 2T(n/2) + Θ(n)  → Θ(n log n)
   worst   (k = 1 or k = n):  T(n) = T(n−1) + Θ(n)  → Θ(n²)
```

**Worst case in the wild:** input sorted (or reverse-sorted) and pivot = first/last element ⇒ every partition is maximally unbalanced ⇒ **Θ(n²)**. Total comparisons in this pattern for n = 8: 7+6+5+4+3+2+1+0 = **28** = n(n−1)/2.

**Fixes GATE loves to mention:**
- **Randomized quicksort** (PW §2.6.1): pick pivot uniformly at random ⇒ no input family is adversarial; *expected* Θ(n log n) for every input. (The Θ(n²) worst case still exists with tiny probability.)
- **Median-of-three** (first, middle, last): on already-sorted input it picks the true median → perfectly balanced → Θ(n log n).
- Stack space: recursion depth = O(log n) expected, **O(n) worst** (sorted input). Recursing on the smaller side first (tail-call style) guarantees O(log n) stack.

**Inversions link (PW §2.7):** for arrays with just a few out-of-place pairs, running insertion sort on top of a quicksort of depth O(log n) — i.e. stopping quicksort early — gives O(n log n); and any comparison sort must make at least log₂(#permutations of the inversions +1)-ish comparisons to distinguish orderings (see §7's decision-tree bound).

---

## 6. Heapsort — two phases, only one of them is O(n log n)

1. **Build max-heap (bottom-up, Floyd's method):** sift down from index ⌊n/2⌋−1 down to 0. The total cost telescopes to **O(n)** (sift cost = height of the node; most nodes sit near the bottom).
2. **Sort phase:** repeat n−1 times: swap root with last element, shrink heap, sift the new root down — O(log n) each ⇒ **Θ(n log n)** total. In-place, unstable.

**Worked build-heap — `[9, 4, 7, 1, 8, 3, 6]`** (indices 0–6; children of i are 2i+1, 2i+2):

- i = 2 (value 7): children are 3 and 6 → no swap.
- i = 1 (value 4): children are 1 and 8 → swap with 8 → `[9, 8, 7, 1, 4, 3, 6]`; node 4 is a leaf → stop.
- i = 0 (value 9): children are 8 and 7 → 9 is largest → no swap.

**Array after the build phase: `[9, 8, 7, 1, 4, 3, 6]`** — a valid max-heap.

---

## 7. The lower bound, and the sorts that beat it

**Decision-tree argument:** any comparison sort on n distinct elements must distinguish n! input permutations. A binary comparison tree with height h has ≤ 2ʰ leaves, so

```
   2ʰ ≥ n!   ⇒   h ≥ ⌈log₂(n!)⌉ = Θ(n log n)   worst-case comparisons
```

Example: n = 5 → log₂ 120 ≈ 6.9 → **at least 7** comparisons in the worst case (merge sort uses ≤ 8).

**How the "linear" sorts dodge it — they never compare keys to keys:**
- **Counting sort:** count occurrences of each value in [0, k], prefix-sum, place. O(n+k) time, stable if placement goes right-to-left. Useless when k ≫ n.
- **Radix sort (LSD):** d passes of a stable counting sort on one digit (base b) ⇒ O(d(n+b)). Stability of the digit sort is *mandatory*, else higher digits scramble lower ones.
- **Bucket sort:** scatter into ~n buckets, sort each, concatenate. **Expected O(n)** when keys are uniform, O(n²) worst.

---

## 8. Traps checklist (re-read before every test)

1. **Merge sort is not in-place** in the GATE standard convention — it needs Θ(n) aux. Don't pick it for "which sorts in O(1) space".
2. **n−1 vs n(n−1)/2:** selection sort's comparison count never changes with input; insertion's does.
3. **"Quick sort on sorted input"** is the trap answer: Θ(n²) with first/last pivot; Θ(n log n) with median-of-three or random pivot.
4. **Build-heap is O(n)** — writing O(n log n) for the *build phase alone* is the classic distractor (the whole heapsort is O(n log n)).
5. **Counting/radix/bucket are not comparison sorts** — "counting sort is a comparison sort" is always the false option.
6. **Moves ≠ comparisons ≠ swaps.** Numbers of moves for merge = m+n *always*; comparisons vary. State which you count.
7. **Stability is not about complexity:** even Θ(n log n) sorts (quick, heap) can be unstable.

---

## 9. Practice set (10 questions, all answers verified by simulation)

> Conventions in force: insertion sort compares `a[j] > key` starting from the end of the sorted prefix; merge counts comparisons only while both runs are non-empty; quicksort = Lomuto partition with the **last** element as pivot unless stated; "moves" for merge sort = element copies (PW outplace convention).

**Q1 · MCQ · Fundamental** — Which one of the following is **not** an in-place sorting algorithm in its standard implementation?
(A) Quicksort  (B) Heapsort  (C) Merge sort  (D) Selection sort

**Q2 · NAT · Fundamental** — The array `[29, 14, 37, 8, 21, 17]` is sorted with **insertion sort**. The total number of element-to-key comparisons performed is ______.

**Q3 · NAT · Standard** — The same array `[29, 14, 37, 8, 21, 17]` is sorted with **top-down merge sort** (split at mid = ⌊n/2⌋; count a comparison only while both merged runs are non-empty). The total number of comparisons is ______.

**Q4 · NAT · Standard** — For the array `[5, 3, 8, 1, 4]`, the number of **inversions** (pairs i < j with a[i] > a[j]) is ______; this also equals the number of element shifts insertion sort performs.

**Q5 · NAT · Standard** — Quicksort with the **last element as pivot** (Lomuto) runs on an array of 8 elements chosen so that at every recursive call the pivot is the **smallest** element of its subarray (one comparison per non-pivot element per partition). The total number of element comparisons is ______.

**Q6 · NAT · Standard** — The array `[9, 4, 7, 1, 8, 3, 6]` undergoes the bottom-up **build-max-heap** phase of heapsort (sift-down from index ⌊n/2⌋−1 to 0). Write the array after the build phase (indices 0–6, space-separated).

**Q7 · MSQ · Moderate** — Which statements about **merge sort** are TRUE?
(A) It is stable.
(B) It is in-place in its standard implementation.
(C) Its worst-case running time is Θ(n log n).
(D) Merging two sorted runs of m elements each always performs exactly 2m − 1 comparisons.

**Q8 · NAT · Moderate** — Standard (outplace) top-down merge sort sorts n = 8 elements. Counting *moves* as element copies (a merge of runs of sizes m, n always performs m + n moves), the total number of moves across the whole sort is ______.

**Q9 · MSQ · Moderate** — Which statements are TRUE?
(A) Any comparison-based sorting algorithm makes at least ⌈log₂(n!)⌉ comparisons on some input of n distinct elements.
(B) Counting sort is a comparison-based algorithm.
(C) LSD radix sort is stable provided the per-digit sort used inside it is stable.
(D) In selection sort, the number of element comparisons is data-dependent.

**Q10 · NAT · Killer** — For the array `[7, 2, 9, 4, 1, 6]`: let I = number of inversions, and let C = total comparisons made by insertion sort (same convention as Q2). Then I + C = ______.

---

### Answer key

| Q | Ans | Q | Ans |
|---|---|---|---|
| 1 | C | 6 | 9 8 7 1 4 3 6 |
| 2 | 12 | 7 | A, C |
| 3 | 9 | 8 | 24 |
| 4 | 6 | 9 | A, C |
| 5 | 28 | 10 | 21 |

### Full solutions

**Q1 — C.** Merge sort copies into an auxiliary array: Θ(n) extra space (PW: "In GATE exam if merge sort given then always consider outplace"). Quicksort, heapsort, selection sort all work within the array.

**Q2 — 12.** Pass-by-pass (from §3.1's table): 1 + 1 + 3 + 3 + 4 = 12 comparisons. Cross-check: comparisons = shifts (9) + stopping comparisons (3) = 12.

**Q3 — 9.** Left half `[29,14,37]`: merge `[14],[37]` → 1 comp; merge `[29]` with `[14,37]` → 2 comps (29 vs 14 → take 14; 29 vs 37 → take 29). Subtotal 3. Right half `[8,21,17]`: merge `[21],[17]` → 1; merge `[8]` with `[17,21]` → 1 (8 vs 17 → take 8, left exhausted). Subtotal 2. Final merge `[14,29,37]` with `[8,17,21]`: 14v8→8, 14v17→14, 29v17→17, 29v21→21 (right exhausted) → 4 comps. Total 3+2+4 = **9**.

**Q4 — 6.** Pairs: (5,3), (5,1), (5,4), (3,1), (8,1), (8,4) → 6. (Insertion sort shifts exactly 6 elements on this array — one shift per inversion: pass trace gives comparisons 1, 1, 3, 3 = 8 = 6 shifts + 2 stopping comparisons.)

**Q5 — 28.** Pivot smallest at every call ⇒ split (0, k−1); a partition of size k does k−1 comparisons. Chain: 7+6+5+4+3+2+1+0 = **28** = n(n−1)/2 — the maximally unbalanced recursion, i.e. the Θ(n²) worst case.

**Q6 — `9 8 7 1 4 3 6`.** Sift-down from i = 2: node 7 > children 3, 6 → no move. i = 1: node 4 < child 8 → swap → array `[9,8,7,1,4,3,6]`; the displaced 4 lands at index 4 (leaf). i = 0: node 9 > children 8 and 7 → no move. Result is a valid max-heap (9 ≥ 8,7; 8 ≥ 1,4; 7 ≥ 3,6).

**Q7 — A, C.** (A) ✔ equal keys: merge takes from the left run first. (B) ✘ needs Θ(n) auxiliary. (C) ✔ recurrence 2T(n/2)+Θ(n). (D) ✘ 2m−1 is the **worst** case; the **best** is min(m,m) = m (one run fully smaller — only m comparisons).

**Q8 — 24.** Levels of merging for n = 8: sizes 1→2→4→8 = 3 merge levels; each level copies all 8 elements exactly once (moves = m+n per merge) → 3 × 8 = **24** = n⌈log₂ n⌉.

**Q9 — A, C.** (A) ✔ decision tree: n! leaves need height ≥ ⌈log₂(n!)⌉. (B) ✘ counting sort never compares keys — that's *why* it can beat n log n with bounded k. (C) ✔ each digit pass must preserve the order established by lower digits; a stable counting sort inside is the standard way. (D) ✘ selection sort performs n(n−1)/2 comparisons on **every** input (only its swaps/moves can be skipped).

**Q10 — 21.** Inversions of `[7,2,9,4,1,6]`: (7,2), (7,4), (7,1), (7,6), (2,1), (9,4), (9,1), (9,6), (4,1) → **I = 9**. Insertion sort pass-by-pass: key 2 → 1 comp; key 9 → 1; key 4 → 3 (9,7 shift; 2 stops); key 1 → 4 (7,4,2 shift; 9 was before 7? trace: array after i=3 is [2,4,7,9,1,6]; key 1 compares 9,7,4,2 → 4 comps, hits start); key 6 → 3 (9,7 shift; 4 stops). C = 1+1+3+4+3 = **12**. I + C = 9 + 12 = **21**. (Consistency check in the general identity: C = inversions + (n−1) − (#scans hitting the start) = 9 + 5 − 2 = 12 ✔.)

---

## 10. Next steps

1. Flashcard the §1 table + §2 stability reasons — those two cover most MCQs.
2. Re-trace Q2/Q3/Q6 on paper once each; counting questions are pure mechanics after that.
3. Then move to `DSA_Notes_06_Time_Complexity_Recurrences.md` (the asymptotic engine behind every complexity answer).
