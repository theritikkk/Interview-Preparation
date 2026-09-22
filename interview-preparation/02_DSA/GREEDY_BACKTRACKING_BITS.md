# DSA Worked Problems: Greedy, Backtracking, Bit Manipulation

---

## Greedy: Jump Game II (Minimum Jumps to Reach the End)

**Problem:** given an array where `nums[i]` is the max jump length from index `i`, find the minimum number of jumps to reach the last index.

**Approach:** think in terms of "levels" (like BFS on an implicit graph): from your current jump's range `[start, end]`, track the farthest reachable index across all positions in that range. When you've exhausted the current range, you must have taken another jump — increment the count and advance the range to `[end+1, farthest]`.

```python
def jump(nums):
    jumps = 0
    current_end = 0
    farthest = 0
    for i in range(len(nums) - 1):
        farthest = max(farthest, i + nums[i])
        if i == current_end:
            jumps += 1
            current_end = farthest
    return jumps
```

**Complexity:** O(n) time, O(1) space.

**Follow-ups:** "Prove the greedy choice (always track the farthest reachable point) is safe." — this is exactly the exchange-argument obligation from `PATTERNS.md` Cluster 15: at each jump, you want to end up as far as possible, because any position reachable from a *less* far endpoint is also reachable from the farthest one — so there's never a reason to prefer a shorter reach; the greedy choice dominates every alternative, which is what makes it safe.

---

## Backtracking: Subsets

**Problem:** given an array of distinct integers, return all possible subsets (the power set).

**Approach:** at each element, branch into two choices — include it, or don't — and recurse. The base case (all elements considered) appends the current partial subset to the result.

```python
def subsets(nums):
    result = []
    def backtrack(start, path):
        result.append(path[:])          # every partial state is itself a valid subset
        for i in range(start, len(nums)):
            path.append(nums[i])
            backtrack(i + 1, path)
            path.pop()                   # undo — this line IS backtracking
    backtrack(0, [])
    return result
```

**Complexity:** O(2ⁿ) subsets generated, each taking O(n) to copy — O(n · 2ⁿ) total.

**Follow-ups:** "What if the input has duplicates and you need unique subsets only?" — sort first, then skip a candidate at the same recursion depth if it equals the previous candidate *and* the previous one was already considered at this depth (`i > start and nums[i] == nums[i-1]: continue`) — a very common, specific extension worth having memorized.

---

## Backtracking: Permutations

**Problem:** given an array of distinct integers, return all permutations.

**Approach:** at each recursion level, try every not-yet-used element as the next position, track usage with a boolean array (or by swapping in place), recurse, then undo.

```python
def permute(nums):
    result = []
    used = [False] * len(nums)
    def backtrack(path):
        if len(path) == len(nums):
            result.append(path[:])
            return
        for i, x in enumerate(nums):
            if used[i]:
                continue
            used[i] = True
            path.append(x)
            backtrack(path)
            path.pop()
            used[i] = False
    backtrack([])
    return result
```

**Complexity:** O(n! · n) — n! permutations, O(n) to copy each.

**Follow-ups:** "How would you generate permutations without extra space for the `used` array?" — swap-based in-place permutation generation (swap the current index with each candidate, recurse, swap back) avoids the auxiliary array entirely — a genuine space optimization worth offering.

---

## Bit Manipulation: Single Number

**Problem:** given an array where every element appears twice except one, find that one element, in O(n) time and O(1) space.

**Approach:** XOR every element together. XOR is commutative/associative, and `x ^ x == 0`, `x ^ 0 == x` — so every paired element cancels itself out, leaving only the unpaired one.

```python
def singleNumber(nums):
    result = 0
    for x in nums:
        result ^= x
    return result
```

**Complexity:** O(n) time, O(1) space — a hash-set approach also achieves O(n) time but needs O(n) space; XOR's entire value is beating that space bound.

**Follow-ups:** "What if every element appears three times except one?" — plain XOR no longer works (it doesn't cancel a triple). The standard extension tracks bit-counts modulo 3 across two bitmask variables (`ones`/`twos`) simulating a base-3 counter per bit position — a genuinely harder variant worth knowing exists, even if not memorized to the exact implementation.

### Quick Revision
- Jump Game II: always track the farthest reachable point within the current jump's range — the exchange-argument proof is the real interview content, not just the code.
- Subsets/Permutations: the backtracking template is identical in shape — append at every valid state (subsets) vs. only at full-length states (permutations); know the duplicate-skipping extension for both.
- Single Number: XOR cancels pairs; the "appears three times" variant needs a fundamentally different (bit-counting) technique — don't try to force XOR onto it.
