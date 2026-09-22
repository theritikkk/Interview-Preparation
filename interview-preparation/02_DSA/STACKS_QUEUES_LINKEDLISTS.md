# DSA Worked Problems: Stacks, Queues, Linked Lists

---

## Monotonic Stack: Daily Temperatures

**Problem:** given daily temperatures, for each day return how many days until a warmer temperature; 0 if none exists.

**Approach:** maintain a stack of *indices* whose temperatures are not yet resolved, kept in decreasing order of temperature. For each new day, pop every stack entry with a lower temperature than today's — each pop means "today is the answer for that popped day." Push today's index last.

```python
def dailyTemperatures(temperatures):
    result = [0] * len(temperatures)
    stack = []  # indices, temperatures decreasing bottom to top
    for i, t in enumerate(temperatures):
        while stack and temperatures[stack[-1]] < t:
            j = stack.pop()
            result[j] = i - j
        stack.append(i)
    return result
```

**Complexity:** O(n) — despite the nested while loop, each index is pushed once and popped at most once across the entire run (amortized analysis); this exact proof is a very common, expected follow-up.

**Follow-ups:** "What changes for 'next smaller element' instead of 'next greater'?" — flip the comparison (`>` instead of `<`) — same structure, mirror-image problem.

---

## Linked List: Reverse a Linked List (Iterative)

**Problem:** reverse a singly linked list in place.

**Approach:** walk the list once, at each node flipping its `next` pointer to point backward — you must save the original `next` *before* overwriting it, or you lose the rest of the list.

```python
def reverseList(head):
    prev = None
    curr = head
    while curr:
        next_node = curr.next   # save before overwriting
        curr.next = prev
        prev = curr
        curr = next_node
    return prev   # new head
```

**Complexity:** O(n) time, O(1) space.

**Follow-ups:** "Do it recursively instead." —
```python
def reverseList(head):
    if not head or not head.next:
        return head
    new_head = reverseList(head.next)
    head.next.next = head   # the node ahead now points back to head
    head.next = None
    return new_head
```
"What's the tradeoff of recursive vs. iterative here?" — recursive is O(n) call-stack space versus iterative's O(1) — meaningful on a very long list (risk of stack overflow), directly echoing the recursion-depth concern from `02_DSA/PATTERNS.md` Cluster 12.

---

## Linked List: Detect Cycle Start (Floyd's Algorithm)

**Problem:** given a linked list, determine if it has a cycle, and if so, return the node where the cycle begins.

**Approach:** phase 1 — a slow pointer (1 step) and fast pointer (2 steps) will meet somewhere inside the cycle if one exists (fast "laps" slow). Phase 2 — the mathematical property of this meeting point: moving one pointer back to `head` and advancing *both* pointers one step at a time, they meet exactly at the cycle's start. This isn't obvious without derivation — know the "why," not just the two-phase recipe.

```python
def detectCycle(head):
    slow = fast = head
    while fast and fast.next:
        slow, fast = slow.next, fast.next.next
        if slow == fast:
            break
    else:
        return None   # no cycle
    ptr = head
    while ptr != slow:
        ptr, slow = ptr.next, slow.next
    return ptr   # cycle start
```

**Complexity:** O(n) time, O(1) space — the entire point versus a hash-set-of-visited-nodes approach, which works but costs O(n) space.

**Follow-ups:** "Why does moving one pointer back to head and advancing both by 1 find the cycle start?" — let the distance from head to cycle start be `a`, and from cycle start to the meeting point be `b`, with cycle length `c`. The slow pointer has traveled `a+b`; the fast pointer has traveled `2(a+b)` and also `a+b+n·c` for some integer `n` (it lapped the cycle `n` extra times) — setting these equal and simplifying shows `a` equals some multiple of `c` minus `b`, which is exactly the distance a pointer starting at `head` needs to reach the cycle start when the other pointer starts at the meeting point — a genuinely non-obvious derivation worth having ready if pushed for it.

---

## Monotonic Deque: Sliding Window Maximum

**Problem:** given an array and window size `k`, return the maximum of each sliding window as it moves across the array.

**Approach:** maintain a deque of *indices*, kept in decreasing order of their values. Before adding a new index, pop from the back any indices whose values are smaller (they can never be the max again while the new, larger element is still in the window). Pop from the front any index that's fallen outside the current window. The front of the deque is always the current window's max.

```python
from collections import deque

def maxSlidingWindow(nums, k):
    dq = deque()  # indices, values decreasing
    result = []
    for i, x in enumerate(nums):
        while dq and nums[dq[-1]] < x:
            dq.pop()
        dq.append(i)
        if dq[0] <= i - k:          # front has fallen out of the window
            dq.popleft()
        if i >= k - 1:
            result.append(nums[dq[0]])
    return result
```

**Complexity:** O(n) — each index pushed once, popped at most once, same amortized argument as the monotonic stack above.

**Follow-ups:** "Why not just use a max-heap?" — a heap gives O(n log k) since stale (out-of-window) entries need lazy deletion and re-checking; the monotonic deque achieves a tighter true O(n) — know that this is a real, checkable complexity improvement, not just a stylistic choice.

### Quick Revision
- Monotonic stack O(n) proof: each index pushed once, popped at most once — amortized, not per-operation.
- Linked list reversal: save `next` before overwriting, every time.
- Floyd's cycle detection: know the distance-equation derivation for why phase 2 works, not just the two-phase recipe.
- Monotonic deque beats a heap for sliding-window-max: O(n) true bound vs. O(n log k).
