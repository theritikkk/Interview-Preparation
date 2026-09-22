# DSA Worked Problems: Arrays, Strings, Hashing

Full solutions, not just problem names — pattern context for each lives in `PATTERNS.md`.

---

## Two Pointers: Container With Most Water

**Problem:** given an array `height` where `height[i]` is the height of a vertical line at position `i`, find two lines that together with the x-axis form a container holding the most water. Return the max area.

**Approach:** brute force checks all O(n²) pairs. The two-pointer insight: start with the widest possible container (`left=0, right=n-1`). At each step, the area is `min(height[left], height[right]) * (right - left)`. The container's height is capped by the *shorter* line — so moving the *taller* line's pointer inward can only decrease width without any chance of increasing the limiting height, while moving the *shorter* line's pointer inward is the only move that could possibly find a taller line and increase the area. So: always move the pointer at the shorter line.

```python
def maxArea(height):
    left, right = 0, len(height) - 1
    best = 0
    while left < right:
        h = min(height[left], height[right])
        best = max(best, h * (right - left))
        if height[left] < height[right]:
            left += 1
        else:
            right -= 1
    return best
```

**Complexity:** O(n) time, O(1) space.

**Follow-ups:** "Prove moving the shorter pointer is always safe" — the interview-critical part of this problem; the code alone doesn't demonstrate you understand *why* it's correct. "What if you needed the actual pair of indices, not just the max area?" — track them alongside `best`.

---

## Sliding Window: Longest Substring Without Repeating Characters

**Problem:** given a string, find the length of the longest substring without repeating characters.

**Approach:** a variable-size window `[left, right]`, expanded by moving `right`. Track the last-seen index of each character. When `right` lands on a character already in the window (its last-seen index is `>= left`), jump `left` forward to just past that character's last occurrence — no need to shrink one character at a time.

```python
def lengthOfLongestSubstring(s):
    last_seen = {}
    left = 0
    best = 0
    for right, ch in enumerate(s):
        if ch in last_seen and last_seen[ch] >= left:
            left = last_seen[ch] + 1
        last_seen[ch] = right
        best = max(best, right - left + 1)
    return best
```

**Complexity:** O(n) time — `right` visits each index once; `left` only ever moves forward. O(min(n, alphabet size)) space.

**Follow-ups:** "Why check `last_seen[ch] >= left`, not just `ch in last_seen`?" — a character seen *before* the current window started is irrelevant; without this check you'd incorrectly jump `left` backward-in-effect on a stale occurrence.

---

## Prefix Sums + Hashing: Subarray Sum Equals K

**Problem:** given an array and an integer `k`, return the number of contiguous subarrays that sum to exactly `k`.

**Approach:** brute force checks all O(n²) subarrays. The insight: if `prefix[j] - prefix[i] == k`, then the subarray from `i+1` to `j` sums to `k`. So for each running prefix sum, count how many *earlier* prefix sums equal `running - k` — that's a hash-map lookup, not a nested loop. Seed the map with `{0: 1}` to correctly count subarrays that start at index 0.

```python
def subarraySum(nums, k):
    count = 0
    running = 0
    seen = {0: 1}   # empty prefix — required for subarrays starting at index 0
    for x in nums:
        running += x
        count += seen.get(running - k, 0)
        seen[running] = seen.get(running, 0) + 1
    return count
```

**Complexity:** O(n) time, O(n) space.

**Follow-ups:** "Walk through why `seen = {0: 1}` is necessary" — without it, a subarray `nums[0..j]` that itself sums to exactly `k` (so `running == k`, meaning `running - k == 0`) would never be counted, since there'd be no entry for prefix sum 0. "What if the array could have negative numbers?" — the algorithm is unaffected; it works purely on prefix-sum differences, not on any sign assumption.

---

## Hashing: Group Anagrams

**Problem:** given an array of strings, group the anagrams together.

**Approach:** two strings are anagrams iff their sorted characters are identical — use the sorted string as a hash-map key, bucket every original string under its sorted form.

```python
def groupAnagrams(strs):
    groups = {}
    for s in strs:
        key = ''.join(sorted(s))
        groups.setdefault(key, []).append(s)
    return list(groups.values())
```

**Complexity:** O(n · k log k) time (n strings, each of length up to k, each needing a sort); O(n · k) space.

**Follow-ups:** "Can you avoid the sort to improve the per-string cost?" — yes: use a fixed-size character-count tuple (26 counts for lowercase English letters) as the key instead of a sorted string, dropping the per-string cost from O(k log k) to O(k) and the total to O(n · k) — a strong, specific optimization to offer proactively.

### Quick Revision
- Two Pointers: always move the pointer that *could* improve the bound; prove why that's safe, don't just state the algorithm.
- Sliding Window: track last-seen index per character; jump `left`, don't shrink one step at a time.
- Prefix Sums + Hashing: seed with `{0: 1}` — the single most common bug in this pattern.
- Hashing: sorted-string key works but a character-count key is the stronger O(n·k) optimization.
