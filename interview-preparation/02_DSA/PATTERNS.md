# DSA Patterns — Master Reference

The goal here is **recognition, not memorization**. Every coding problem you'll see in a screen is a disguised version of one of the patterns below. The skill that actually transfers between problems is: read the constraints and shape of the problem, and know within 30 seconds which pattern it's testing. This file is the recognition map; dedicated files with fully worked easy/medium/hard problems for each cluster follow in later files.

**How to use this under interview pressure:** before coding anything, say out loud (to the interviewer, this is expected) which pattern you think applies and why — the recognition signals below are literally the words to listen for in the problem statement.

---

## 1. Two Pointers
- **Core intuition:** two indices move through a (usually sorted, or otherwise structured) sequence, converging or sliding to avoid the O(n²) brute force of checking every pair.
- **Recognition signals:** "sorted array," "pair that sums to X," "palindrome check," "remove duplicates in place."
- **Template:**
```python
left, right = 0, len(arr) - 1
while left < right:
    s = arr[left] + arr[right]
    if s == target: return [left, right]
    elif s < target: left += 1
    else: right -= 1
```
- **Typical mistakes:** forgetting the array must be sorted for the sum-based version; off-by-one on the `while` condition (`<` vs `<=`).
- **Complexity:** O(n) time, O(1) space — the entire value proposition over brute force O(n²).
- **Problems:** Easy — Two Sum II (sorted input). Medium — Container With Most Water. Hard — Trapping Rain Water.
- **Follow-ups:** "What if the array isn't sorted?" (sort first, O(n log n), or use hashing instead — see §2). "What if you need all pairs, not just one?" (need to handle duplicates carefully while advancing pointers).
- **Variations:** fast/slow pointers (cycle detection — see Linked Lists), three pointers (3Sum).

## 2. Hashing
- **Core intuition:** trade space for time — an O(1) average lookup replaces an O(n) linear scan.
- **Recognition signals:** "have you seen this before," "count of," "complement," "anagram," "duplicate."
- **Template:**
```python
seen = {}
for i, x in enumerate(arr):
    if target - x in seen: return [seen[target - x], i]
    seen[x] = i
```
- **Typical mistakes:** using a hash set when you need to recover an *index* or *count* (need a dict, not a set); forgetting hash-based structures don't preserve order (use `OrderedDict`/insertion-ordered dict, or a separate structure, if order matters).
- **Complexity:** O(n) time average, O(n) space.
- **Problems:** Easy — Two Sum. Medium — Group Anagrams. Hard — Longest Consecutive Sequence (O(n) using a hash set, not sorting).
- **Follow-ups:** "Can you do it in O(1) space?" (usually no, for pure hashing problems — that's the tradeoff you're making). "What if the hash function has many collisions?" (worst case degrades to O(n) per operation — mention this only if pushed on hash table internals).
- **Variations:** hashing with a sliding window (see §4), hashing pairs/tuples as keys.

## 3. Sliding Window
- **Core intuition:** maintain a contiguous window with two pointers that only move forward, avoiding recomputation from scratch for every window position.
- **Recognition signals:** "contiguous subarray/substring," "at most K," "longest/shortest substring with property X."
- **Template:**
```python
left = 0
window_state = {}
for right, x in enumerate(arr):
    # expand: add arr[right] to window_state
    while window_invalid(window_state):
        # shrink: remove arr[left] from window_state
        left += 1
    # window [left, right] is valid here — update the answer
```
- **Typical mistakes:** shrinking the window with an `if` instead of a `while` (only handles one violation, not cumulative ones); forgetting to update the answer at the right point (after expand, after shrink, or both — depends on the problem).
- **Complexity:** O(n) — each pointer moves forward at most n times, total O(n) not O(n²).
- **Problems:** Easy — Maximum Sum Subarray of Size K (fixed window). Medium — Longest Substring Without Repeating Characters. Hard — Minimum Window Substring.
- **Follow-ups:** "Fixed vs. variable window — how does the template change?" (fixed: slide by exactly one each step; variable: shrink conditionally). "Can this be done without a window at all?" (sometimes prefix sums are enough — know the difference).
- **Variations:** at-most-K distinct characters, minimum window containing all of a target set.

## 4. Prefix Sums
- **Core intuition:** precompute cumulative sums once so any range-sum query becomes O(1) instead of re-summing the range every time.
- **Recognition signals:** "sum of subarray from i to j," "number of subarrays summing to K," multiple range queries on a static array.
- **Template:**
```python
prefix = [0] * (len(arr) + 1)
for i, x in enumerate(arr): prefix[i+1] = prefix[i] + x
range_sum = prefix[j+1] - prefix[i]   # sum of arr[i..j] inclusive

# "count subarrays summing to K" — hash the prefix sums seen so far
count, running, seen = 0, 0, {0: 1}
for x in arr:
    running += x
    count += seen.get(running - k, 0)
    seen[running] = seen.get(running, 0) + 1
```
- **Typical mistakes:** off-by-one on the prefix array's indexing convention; forgetting to seed `seen = {0: 1}` for the subarray-sum-to-K variant (misses subarrays starting at index 0).
- **Complexity:** O(n) preprocessing, O(1) per query.
- **Problems:** Easy — Range Sum Query, Immutable. Medium — Subarray Sum Equals K. Hard — Maximum Size Subarray Sum Equals K.
- **Follow-ups:** "What if the array is mutable?" (prefix sums break — need a Fenwick tree/BIT or segment tree instead, see §27).
- **Variations:** 2D prefix sums (matrix range-sum queries), prefix XOR.

## 5. Binary Search
- **Core intuition:** repeatedly halve a *monotonic* search space — not just literal sorted arrays, but any space where you can answer "is the answer ≤ X?" with a yes/no that flips exactly once.
- **Recognition signals:** "sorted array," "find minimum/maximum such that condition holds," "search in rotated array," "on the answer" (binary search over a value range, not an array index).
- **Template:**
```python
lo, hi = 0, len(arr) - 1
while lo <= hi:
    mid = (lo + hi) // 2
    if arr[mid] == target: return mid
    elif arr[mid] < target: lo = mid + 1
    else: hi = mid - 1
return -1

# "Binary search on the answer" pattern:
lo, hi = min_possible, max_possible
while lo < hi:
    mid = (lo + hi) // 2
    if feasible(mid): hi = mid       # shrink toward the smallest feasible value
    else: lo = mid + 1
return lo
```
- **Typical mistakes:** infinite loop from `mid = (lo+hi)//2` combined with `lo = mid` instead of `mid + 1` (or vice versa) — always double check the loop provably shrinks; integer overflow in other languages (`(lo+hi)//2` vs `lo + (hi-lo)//2`, matters in C/C++/Java, not Python).
- **Complexity:** O(log n).
- **Problems:** Easy — Binary Search. Medium — Search in Rotated Sorted Array. Hard — Median of Two Sorted Arrays (binary search on the partition point, O(log(min(m,n)))).
- **Follow-ups:** "How do you binary search when the array isn't literally sorted but is rotated?" (compare against one boundary to determine which half is sorted, recurse into the correct half). "What makes a problem binary-searchable even without a sorted array?" (monotonicity of a feasibility function — this is the deeper, more interview-impressive framing).
- **Variations:** first/last occurrence, binary search on answer (capacity/speed/allocation problems), search in 2D matrix.

## 6. Sorting as a Tool
- **Core intuition:** many problems become trivial once sorted — the sort itself (O(n log n)) is rarely the hard part; recognizing *that* sorting unlocks a simpler pattern is.
- **Recognition signals:** "intervals," "meeting rooms," "closest pair," problems where order among elements (not their original positions) is what matters.
- **Typical mistakes:** sorting when you didn't need to (destroys original index information you needed) — a hash map alongside the sort, or sorting indices instead of values, fixes this.
- **Complexity:** O(n log n) — the ceiling most "sort first" solutions run into; know when a problem's true optimal is actually faster (see counting sort/bucket sort for bounded-range integers).
- **Problems:** Easy — Merge Sorted Array. Medium — Merge Intervals. Hard — Count of Smaller Numbers After Self (merge-sort-based counting).
- **Follow-ups:** "Can you do better than O(n log n)?" (only if the input has special structure — bounded integer range → counting sort O(n+k); mostly-sorted input → insertion sort behaves near O(n)).

## 7. Intervals
- **Core intuition:** sort by start (or end) time, then sweep once, merging or counting overlaps as you go.
- **Recognition signals:** "meetings," "intervals," "overlapping," "merge," "free time."
- **Template:**
```python
intervals.sort(key=lambda x: x[0])
merged = [intervals[0]]
for start, end in intervals[1:]:
    if start <= merged[-1][1]: merged[-1][1] = max(merged[-1][1], end)  # overlap: merge
    else: merged.append([start, end])
```
- **Typical mistakes:** sorting by the wrong key (start vs. end depends on the problem — "minimum rooms needed" often needs both, processed as separate sorted event streams).
- **Complexity:** O(n log n) for the sort, O(n) for the sweep.
- **Problems:** Easy — Merge Intervals. Medium — Meeting Rooms II (min rooms needed). Hard — Employee Free Time.
- **Follow-ups:** "How would you handle intervals arriving in a stream, not all at once?" (a min-heap of end times instead of a full sort — see §15).

## 8. Stacks
- **Core intuition:** LIFO order naturally models nested/matching structure — "most recent unmatched thing."
- **Recognition signals:** "valid parentheses," "matching brackets," "nested," "undo," expression evaluation.
- **Complexity:** O(n) time, O(n) space.
- **Problems:** Easy — Valid Parentheses. Medium — Evaluate Reverse Polish Notation. Hard — Basic Calculator (with nested parens and +/-).
- **Follow-ups:** "How would you implement a stack that also returns the min in O(1)?" (an auxiliary stack tracking running minimums alongside the main one).

## 9. Monotonic Stack
- **Core intuition:** keep the stack's elements in strictly increasing or decreasing order by popping whenever the incoming element would violate that order — each pop is you discovering that element's "next greater/smaller" neighbor.
- **Recognition signals:** "next greater element," "next smaller," "daily temperatures," "largest rectangle in histogram."
- **Template:**
```python
stack = []  # indices, kept monotonically decreasing in value
result = [0] * len(arr)
for i, x in enumerate(arr):
    while stack and arr[stack[-1]] < x:
        j = stack.pop()
        result[j] = i - j   # e.g., "days until warmer"
    stack.append(i)
```
- **Typical mistakes:** pushing values instead of indices (you usually need the index to compute a distance or to look up the original value later); getting the inequality direction backwards (increasing vs. decreasing monotonic stack solve mirror-image problems).
- **Complexity:** O(n) — looks like nested loops but each element is pushed and popped at most once, total work is O(n), a very common interview follow-up to justify.
- **Problems:** Easy — Daily Temperatures. Medium — Next Greater Element II (circular array). Hard — Largest Rectangle in Histogram.
- **Follow-ups:** "Prove this is O(n) and not O(n²) despite the nested while loop." (amortized analysis — each index is pushed once and popped at most once across the entire run).

## 10. Queues & Deque
- **Core intuition:** FIFO for order-preserving processing (BFS); a deque (double-ended queue) generalizes this to let you pop from either end, which is what a sliding-window-maximum needs.
- **Recognition signals:** "level order," "sliding window maximum/minimum," "first come first served."
- **Complexity:** O(n) amortized for a monotonic deque solving sliding-window-max.
- **Problems:** Easy — Implement Queue Using Stacks. Medium — Sliding Window Maximum. Hard — Shortest Subarray with Sum at Least K (monotonic deque + prefix sums combined).
- **Follow-ups:** "Why a deque instead of a heap for sliding window max?" (heap gives O(n log n) with lazy deletion of stale entries; a monotonic deque gives true O(n) — know the tighter bound is achievable and why).

## 11. Linked Lists
- **Core intuition:** pointer manipulation without random access — most problems reduce to careful bookkeeping of `prev`/`curr`/`next`, or to fast/slow pointers for cycle-related problems.
- **Recognition signals:** "reverse a linked list," "detect a cycle," "middle of a list," "merge k sorted lists."
- **Template:**
```python
# Fast/slow pointers — cycle detection (Floyd's algorithm)
slow = fast = head
while fast and fast.next:
    slow, fast = slow.next, fast.next.next
    if slow == fast: return True  # cycle found
return False
```
- **Typical mistakes:** losing the reference to the rest of the list while reversing (always save `next` before overwriting `curr.next`); off-by-one when finding the "middle" (depends on whether you want the first or second middle of an even-length list).
- **Complexity:** O(n) time, O(1) space for pointer-manipulation problems (the whole appeal vs. array-based approaches needing O(n) extra space).
- **Problems:** Easy — Reverse Linked List. Medium — Linked List Cycle II (find the cycle's start node). Hard — Merge k Sorted Lists (heap-based, O(n log k)).
- **Follow-ups:** "Why does Floyd's algorithm's fast pointer moving 2x guarantee it catches the slow pointer if there's a cycle?" (relative speed inside the cycle is 1 step/iteration — it's equivalent to catching up on a circular track, guaranteed within one full cycle length).

## 12. Trees — Traversals
- **Core intuition:** most tree problems are traversal order plus a small amount of state carried along (a running sum, a depth, a parent reference).
- **Recognition signals:** anything on a binary tree — depth, path sum, view, serialization.
- **Template:**
```python
def inorder(node, out):
    if not node: return
    inorder(node.left, out); out.append(node.val); inorder(node.right, out)

# Iterative BFS (level order)
from collections import deque
q = deque([root])
while q:
    level = []
    for _ in range(len(q)):
        node = q.popleft()
        level.append(node.val)
        if node.left: q.append(node.left)
        if node.right: q.append(node.right)
```
- **Typical mistakes:** confusing inorder/preorder/postorder use-cases (inorder on a BST gives sorted order — this single fact unlocks several BST problems); forgetting the base case (`if not node: return`) causing infinite recursion or crashes on leaves.
- **Complexity:** O(n) time, O(h) recursion stack space (h = height; O(log n) balanced, O(n) worst case degenerate/skewed tree — a very common follow-up).
- **Problems:** Easy — Maximum Depth of Binary Tree. Medium — Binary Tree Level Order Traversal. Hard — Serialize and Deserialize Binary Tree.
- **Follow-ups:** "Convert your recursive solution to iterative — why would you need to?" (avoiding stack overflow on very deep/degenerate trees; explicit stack simulates the call stack).

## 13. Binary Search Trees (BST)
- **Core intuition:** the BST invariant (left < node < right) means inorder traversal yields sorted order, and search/insert/delete are O(h).
- **Recognition signals:** "BST," "kth smallest," "validate BST," "closest value."
- **Typical mistakes:** validating a BST by only checking the immediate children against the node (must check against the *running min/max bound inherited from ancestors*, not just the direct parent).
- **Complexity:** O(h) per operation — O(log n) if balanced, O(n) if degenerate (this is exactly why self-balancing variants like AVL/Red-Black trees exist, worth mentioning).
- **Problems:** Easy — Search in a BST. Medium — Kth Smallest Element in a BST (inorder traversal, stop early). Hard — Recover Binary Search Tree (two nodes swapped, fix via inorder).
- **Follow-ups:** "What happens to your O(log n) claim on a degenerate (linked-list-shaped) BST?" (it degrades to O(n) — this is precisely why Java's `TreeMap` and similar production structures use self-balancing trees).

## 14. Heaps / Priority Queues
- **Core intuition:** always need the min or max efficiently, and the set changes over time — a heap gives O(log n) insert/extract vs. O(n log n) re-sorting every time.
- **Recognition signals:** "kth largest/smallest," "top K," "merge K sorted," "median of a stream."
- **Template:**
```python
import heapq
heap = []
for x in stream:
    heapq.heappush(heap, x)
    if len(heap) > k: heapq.heappop(heap)  # keep only the top-k smallest seen (min-heap of size k)
# heap[0] is now the kth largest element
```
- **Typical mistakes:** using a max-heap when you need the smallest of the top-K (Python's `heapq` is min-heap only — negate values for a max-heap); rebuilding the heap from scratch each time instead of maintaining it incrementally.
- **Complexity:** O(log n) per insert/extract, O(n log k) for "top k of n" using a size-bounded heap (better than O(n log n) full sort when k << n).
- **Problems:** Easy — Kth Largest Element in a Stream. Medium — Top K Frequent Elements. Hard — Find Median from Data Stream (two heaps, one max-heap for the lower half, one min-heap for the upper half).
- **Follow-ups:** "Why is a size-bounded heap better than sorting the whole array for 'top K'?" (O(n log k) vs O(n log n) — meaningful when k is much smaller than n).

## 15. Greedy
- **Core intuition:** make the locally-optimal choice at each step and trust it leads to a globally optimal solution — valid only when the problem has the right structural property (exchange argument / matroid structure), which you should be able to justify, not just assert.
- **Recognition signals:** "minimum number of," "maximum number of non-overlapping," "assign," "jump game," interval scheduling.
- **Typical mistakes:** applying greedy where it doesn't actually hold (classic trap: 0/1 knapsack is NOT greedy-solvable even though fractional knapsack is — know exactly why: fractional allows partial items, so sorting by value/weight ratio works; 0/1's all-or-nothing constraint breaks that).
- **Complexity:** usually O(n log n) (dominated by an initial sort).
- **Problems:** Easy — Assign Cookies. Medium — Jump Game II (minimum jumps). Hard — Candy (two-pass greedy).
- **Follow-ups:** "Prove your greedy choice is safe" — this is the single most important greedy follow-up; be ready with an exchange-argument sketch ("if an optimal solution didn't make this choice, we can swap it in without making the solution worse").

## 16. Backtracking
- **Core intuition:** explore a decision tree depth-first, undoing (backtracking) a choice when it can't lead to a valid/complete solution — systematic brute force with early pruning.
- **Recognition signals:** "all permutations/combinations/subsets," "N-Queens," "Sudoku solver," "generate all valid."
- **Template:**
```python
def backtrack(path, choices):
    if is_complete(path):
        result.append(path[:]); return
    for choice in choices:
        if not is_valid(choice, path): continue
        path.append(choice)
        backtrack(path, next_choices(choices, choice))
        path.pop()   # undo — this line IS backtracking
```
- **Typical mistakes:** forgetting to undo the choice (`path.pop()`) — corrupts every subsequent branch; not pruning early enough, causing correct-but-too-slow solutions (add `is_valid` checks as early as possible in the recursion, not only at the leaf).
- **Complexity:** typically exponential (O(2ⁿ) for subsets, O(n!) for permutations) — the goal of pruning is reducing the *practical* branching factor, not changing the theoretical worst case.
- **Problems:** Easy — Subsets. Medium — Permutations. Hard — N-Queens.
- **Follow-ups:** "How would you prune this further?" (constraint propagation — check partial validity before recursing deeper, not just at complete solutions).

## 17. Graphs — BFS
- **Core intuition:** explore level by level using a queue — guarantees the *shortest path in an unweighted graph* the first time you reach a node.
- **Recognition signals:** "shortest path" + unweighted, "minimum number of steps/moves," "levels."
- **Complexity:** O(V + E).
- **Problems:** Easy — Number of Islands (BFS/DFS flood fill). Medium — Rotting Oranges (multi-source BFS). Hard — Word Ladder (BFS over an implicit graph of one-letter-different words).
- **Follow-ups:** "Why BFS and not DFS for shortest path?" (BFS explores in increasing distance order — the first time you reach a node is guaranteed shortest; DFS gives no such guarantee).

## 18. Graphs — DFS
- **Core intuition:** explore as deep as possible before backtracking — natural for connectivity, cycle detection, and exhaustive path enumeration.
- **Recognition signals:** "connected components," "cycle detection," "all paths," "flood fill."
- **Typical mistakes:** forgetting to mark nodes visited (infinite loop on cyclic graphs); in directed graphs, confusing "visited" with "currently in the recursion stack" — cycle detection in a directed graph specifically needs the latter (a node revisited *while still on the current path* is a cycle; a node visited via a different, already-finished path is not).
- **Complexity:** O(V + E).
- **Problems:** Easy — Number of Provinces (connected components). Medium — Course Schedule (cycle detection in a directed graph). Hard — Critical Connections in a Network (Tarjan's bridge-finding algorithm).
- **Follow-ups:** "How do you detect a cycle in a directed vs. undirected graph differently?" (directed: track recursion-stack membership; undirected: track the parent to avoid falsely flagging the edge you just came from as a cycle).

## 19. Topological Sort
- **Core intuition:** produce a linear ordering of nodes in a DAG such that every edge points forward — only possible if there's no cycle.
- **Recognition signals:** "course prerequisites," "build order," "task scheduling with dependencies" (directly relevant vocabulary to your own Distributed Task Scheduler project, though that project's scheduling is time-based, not dependency-based — a good contrast point if asked).
- **Template (Kahn's algorithm — BFS-based):**
```python
from collections import deque
in_degree = {n: 0 for n in nodes}
for u, v in edges: in_degree[v] += 1
queue = deque([n for n in nodes if in_degree[n] == 0])
order = []
while queue:
    u = queue.popleft(); order.append(u)
    for v in graph[u]:
        in_degree[v] -= 1
        if in_degree[v] == 0: queue.append(v)
if len(order) != len(nodes): raise ValueError("cycle detected")
```
- **Complexity:** O(V + E).
- **Problems:** Easy — Course Schedule (does a valid order exist?). Medium — Course Schedule II (return the order). Hard — Alien Dictionary (build the graph from ordering constraints implicit in a word list).
- **Follow-ups:** "How do you detect that no valid order exists?" (if the output order has fewer nodes than the graph — the remainder are in a cycle).

## 20. Union-Find (Disjoint Set)
- **Core intuition:** efficiently track which elements belong to the same group under repeated "merge these two groups" operations, using path compression + union by rank/size to keep operations nearly O(1).
- **Recognition signals:** "connected components with dynamic edges," "redundant connection," "accounts merge," "minimum spanning tree" (Kruskal's uses this directly).
- **Template:**
```python
parent = list(range(n))
def find(x):
    if parent[x] != x: parent[x] = find(parent[x])  # path compression
    return parent[x]
def union(x, y):
    rx, ry = find(x), find(y)
    if rx != ry: parent[rx] = ry
```
- **Complexity:** O(α(n)) amortized per operation with path compression + union by rank — effectively constant for any practical n (α is the inverse Ackermann function).
- **Problems:** Easy — Number of Provinces (union-find alternative to DFS). Medium — Redundant Connection. Hard — Accounts Merge.
- **Follow-ups:** "Why is union-find better than repeated BFS/DFS for this?" (avoids re-scanning the whole graph on every incremental edge addition — near-O(1) amortized per union/find vs. O(V+E) per full traversal).

## 21. Shortest Paths (Weighted Graphs)
- **Core intuition:** BFS only works unweighted — with weights, greedily expand the *currently cheapest known* frontier node (Dijkstra), or relax all edges repeatedly to tolerate negative weights (Bellman-Ford).
- **Recognition signals:** "shortest path" + weighted edges/costs; "can weights be negative?" is the deciding question between the two algorithms.
- **Complexity:** Dijkstra: O((V + E) log V) with a min-heap. Bellman-Ford: O(V·E) — much slower, but handles negative weights and detects negative cycles.
- **Problems:** Easy — Network Delay Time (textbook Dijkstra). Medium — Cheapest Flights Within K Stops (Bellman-Ford-style, bounded relaxations). Hard — Path With Minimum Effort (Dijkstra variant with a different "cost" definition).
- **Follow-ups:** "Why doesn't Dijkstra work with negative edge weights?" (its greedy assumption — once a node is finalized, its distance can't improve — breaks if a later negative edge could still reduce it).

## 22. Minimum Spanning Tree (MST)
- **Core intuition:** connect all nodes with the minimum total edge weight, no cycles — Kruskal's (sort edges, union-find to avoid cycles) or Prim's (grow one tree greedily from a start node using a heap).
- **Complexity:** Kruskal's: O(E log E) (dominated by the sort). Prim's: O(E log V) with a heap.
- **Problems:** Medium — Min Cost to Connect All Points. Hard — Optimize Water Distribution in a Village (MST with a virtual source node).
- **Follow-ups:** "Kruskal's vs. Prim's — when would you prefer one?" (Kruskal's is simpler with a good union-find and works well on sparse graphs; Prim's is often preferred on dense graphs where E is close to V²).

## 23. Dynamic Programming
- **Core intuition:** break a problem into overlapping subproblems, solve each exactly once, and reuse the result — the difference between DP and plain recursion is memoization or a bottom-up table exploiting **optimal substructure**.
- **Recognition signals:** "number of ways to," "maximum/minimum," "can you reach/partition," and critically: brute-force recursion re-solving the same subproblem repeatedly.
- **Template:**
```python
# Top-down (memoized)
from functools import lru_cache
@lru_cache(None)
def solve(i, remaining):
    if base_case(i, remaining): return base_value
    return combine(solve(i+1, remaining), solve(i+1, remaining - cost[i]))

# Bottom-up (tabulation) — usually preferred in interviews for explicit space-complexity control
dp = [0] * (n + 1)
dp[0] = base_value
for i in range(1, n + 1):
    dp[i] = transition(dp[i-1], ...)
```
- **Typical mistakes:** not identifying the *state* correctly (missing a dimension the recurrence actually depends on — the most common DP bug); recomputing without memoizing (defeats the entire point); off-by-one in base cases.
- **Complexity:** O(states × transition cost) — always state this explicitly; e.g., knapsack is O(n·W).
- **Problems:** Easy — Climbing Stairs. Medium — Coin Change. Hard — Edit Distance / Regular Expression Matching.
- **Follow-ups:** "Can you reduce the space complexity?" (very common — if `dp[i]` only depends on `dp[i-1]` (or a fixed small window), you don't need the full table, just a rolling variable — O(n) space → O(1)).
- **Variations:** 1D DP, 2D DP (grid/two-sequence problems like edit distance), DP on trees, DP on bitmasks (subset states), interval DP.

## 24. Bit Manipulation
- **Core intuition:** use bitwise operations for O(1) per-bit work, compact set representation (bitmask), or to exploit properties like XOR's self-cancellation.
- **Recognition signals:** "single number," "subsets" (bitmask enumeration), "count set bits," "power of two."
- **Template:**
```python
x & (x - 1)      # clears the lowest set bit — useful for counting set bits, checking power of two (x & (x-1) == 0)
x ^ x == 0        # XOR self-cancels — classic for "find the single non-duplicate" problems
1 << k            # bitmask with bit k set — used to enumerate subsets: for mask in range(1 << n)
```
- **Typical mistakes:** forgetting operator precedence (`&`/`|`/`^` bind looser than comparisons in many languages — always parenthesize); sign-extension surprises with right-shift on negative numbers in languages with fixed-width signed integers (less of an issue in Python, real in C/C++/Java).
- **Complexity:** O(1) per operation, O(2ⁿ) to enumerate all subsets via bitmask.
- **Problems:** Easy — Single Number (XOR). Medium — Counting Bits. Hard — Maximum XOR of Two Numbers in an Array (bitwise trie — ties directly into §26).
- **Follow-ups:** "How would you count set bits faster than checking each bit?" (`x & (x-1)` trick removes exactly one set bit per iteration — loop runs only as many times as there are set bits, not the bit-width).

## 25. Tries
- **Core intuition:** a tree where each path from root to a marked node spells out a string — shared prefixes are stored once, making prefix search O(length of query) regardless of how many strings are stored.
- **Recognition signals:** "prefix," "autocomplete," "word search with a dictionary," "longest common prefix."
- **Complexity:** O(L) per insert/search, where L is the string length — independent of the number of stored strings, which is the entire point versus scanning a list of strings.
- **Problems:** Easy — Implement Trie. Medium — Word Search II (trie + backtracking on a grid). Hard — Maximum XOR of Two Numbers in an Array (bitwise trie).
- **Follow-ups:** "How does a trie's space usage compare to a hash set of the same strings?" (higher constant-factor overhead per node, but shared-prefix strings cost much less incrementally — a real tradeoff to name, not just "tries are better").

## 26. Advanced Data Structures — Segment Tree / Fenwick Tree (BIT)
- **Core intuition:** support *both* range queries (sum/min/max over a range) *and* point updates in O(log n) — prefix sums alone (§4) give O(1) queries but break entirely under updates.
- **Recognition signals:** "range sum/min/max query" **combined with** "update a value" — this combination is the specific signal that rules out plain prefix sums and rules in a Fenwick tree or segment tree.
- **Complexity:** O(log n) per update and per query, O(n) space.
- **Problems:** Medium — Range Sum Query — Mutable (canonical Fenwick tree/segment tree problem). Hard — Count of Smaller Numbers After Self (Fenwick tree used for counting, not summing).
- **Follow-ups:** "Segment tree vs. Fenwick tree — when would you choose one over the other?" (Fenwick tree: simpler, smaller constant factor, but limited to invertible operations like sum; segment tree: more general — handles min/max/gcd, anything associative, at the cost of more code).

---

## Full Pattern Recognition Cheat Table

| Signal in the problem statement | Likely pattern |
|---|---|
| "sorted array," pair/triplet sum | Two Pointers |
| "have I seen this," count/complement | Hashing |
| "contiguous subarray/substring," "at most K" | Sliding Window |
| multiple static range-sum queries | Prefix Sums |
| range query **+** updates | Segment Tree / Fenwick Tree |
| "find X in sorted/rotated," "minimize the maximum" | Binary Search |
| "intervals," "meetings," "merge" | Intervals |
| "matching," "nested," "undo" | Stack |
| "next greater/smaller element" | Monotonic Stack |
| "sliding window maximum" | Monotonic Deque |
| "reverse/cycle in a list" | Linked List (+ fast/slow pointers) |
| binary tree anything | Tree Traversal (in/pre/post/level order) |
| "kth smallest in a BST" | BST inorder property |
| "top K," "kth largest," "median of a stream" | Heap |
| "minimum/maximum number of X, choose greedily" | Greedy (justify with an exchange argument) |
| "all permutations/combinations/subsets" | Backtracking |
| "shortest path," unweighted | BFS |
| "connected components," "cycle," "all paths" | DFS |
| "prerequisites," "build order" | Topological Sort |
| "dynamically merge groups," "redundant connection" | Union-Find |
| "shortest path," weighted | Dijkstra / Bellman-Ford |
| "connect all nodes, minimum total cost" | MST (Kruskal's/Prim's) |
| "number of ways," "min/max," overlapping subproblems | Dynamic Programming |
| "single number," subset enumeration, XOR tricks | Bit Manipulation |
| "prefix," "autocomplete" | Trie |

Memorize this table's *left column vocabulary*, not the right column's answers — the vocabulary is what actually appears under interview pressure.
