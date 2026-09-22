# DSA Worked Problems: Binary Search, Sorting, Intervals

---

## Binary Search: Search in Rotated Sorted Array

**Problem:** a sorted array has been rotated at an unknown pivot (e.g., `[4,5,6,7,0,1,2]`). Find the index of a target value in O(log n), or return -1.

**Approach:** standard binary search assumes the whole range is sorted; here, at each midpoint, *one* of the two halves (left-of-mid or mid-to-right) is guaranteed to still be normally sorted, even though the whole array isn't. Identify which half is sorted by comparing `arr[left]` and `arr[mid]`, then check whether the target falls within that sorted half's range — if so, recurse there; otherwise recurse into the other half.

```python
def search(nums, target):
    left, right = 0, len(nums) - 1
    while left <= right:
        mid = (left + right) // 2
        if nums[mid] == target:
            return mid
        if nums[left] <= nums[mid]:               # left half is sorted
            if nums[left] <= target < nums[mid]:
                right = mid - 1
            else:
                left = mid + 1
        else:                                      # right half is sorted
            if nums[mid] < target <= nums[right]:
                left = mid + 1
            else:
                right = mid - 1
    return -1
```

**Complexity:** O(log n) time, O(1) space.

**Follow-ups:** "What if there are duplicates?" — `nums[left] <= nums[mid]` can no longer reliably tell you which half is sorted (e.g., `[1,1,1,0,1]`); the standard fix is to shrink the range by one from whichever side is ambiguous (`left += 1` or `right -= 1`) when `nums[left] == nums[mid]`, degrading worst-case to O(n) — know this degradation and why it's unavoidable with duplicates.

---

## Binary Search on the Answer: Minimum Capacity to Ship Packages Within D Days

**Problem:** given package weights and a number of days `D`, find the minimum ship capacity such that all packages can be shipped (in given order, one day's worth loaded until capacity is hit) within `D` days.

**Approach:** this isn't searching an array — it's searching a *range of possible answers* (capacity from `max(weights)` to `sum(weights)`) for the smallest value where a feasibility check (`canShipWithinDays(capacity)`) returns true. The feasibility function is monotonic: if capacity C works, any capacity > C also works — that monotonicity is exactly what makes binary search valid here, not sortedness of any array.

```python
def shipWithinDays(weights, D):
    def days_needed(capacity):
        days, current = 1, 0
        for w in weights:
            if current + w > capacity:
                days += 1
                current = 0
            current += w
        return days

    lo, hi = max(weights), sum(weights)
    while lo < hi:
        mid = (lo + hi) // 2
        if days_needed(mid) <= D:
            hi = mid              # mid is feasible — try to shrink toward a smaller feasible capacity
        else:
            lo = mid + 1
    return lo
```

**Complexity:** O(n log(sum(weights))) — each feasibility check is O(n), run O(log(range)) times.

**Follow-ups:** "How do you recognize a problem is 'binary search on the answer' rather than a direct search?" — the signal is a monotonic feasibility function over a range of candidate answers ("minimize/maximize X such that condition holds"), not necessarily any sorted array at all — see `PATTERNS.md` Cluster 5 for the general framing.

---

## Intervals: Meeting Rooms II (Minimum Rooms Needed)

**Problem:** given meeting time intervals, find the minimum number of conference rooms required.

**Approach:** the number of rooms needed at any moment equals the number of meetings currently overlapping. Rather than merging (which loses count information), separate start times and end times into two sorted streams and sweep through them together: every start increments the room count, every end (that occurs at or before the current start) decrements it — track the running maximum.

```python
def minMeetingRooms(intervals):
    starts = sorted(i[0] for i in intervals)
    ends = sorted(i[1] for i in intervals)
    rooms = max_rooms = 0
    s = e = 0
    while s < len(starts):
        if starts[s] < ends[e]:
            rooms += 1
            s += 1
        else:
            rooms -= 1
            e += 1
        max_rooms = max(max_rooms, rooms)
    return max_rooms
```

**Complexity:** O(n log n) for the sorts, O(n) for the sweep.

**Follow-ups:** "Why can't you just use the basic Merge Intervals approach here?" — merging tells you whether intervals overlap *at all*, but not how many are simultaneously active at the peak, which is what "rooms needed" actually asks — a subtly different question requiring the separate-streams sweep, not a simple merge.

### Quick Revision
- Rotated-array search: identify which half is sorted first, then check if target is in that half's range.
- Binary search on the answer: recognize via a monotonic feasibility function over a range, not a sorted array.
- Meeting Rooms II: separate start/end streams, sweep together, track running max — merging alone loses the "how many simultaneously" information.
