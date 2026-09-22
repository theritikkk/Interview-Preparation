# DSA Worked Problems: Trees, Heaps, Tries

---

## Tree Traversal: Binary Tree Level Order Traversal

**Problem:** given a binary tree, return the values level by level (each level as its own list).

**Approach:** BFS with a queue, but process one full level at a time by snapshotting the queue's length *before* the inner loop starts — this is what separates levels from each other, versus a plain BFS that just flattens everything into one list.

```python
from collections import deque

def levelOrder(root):
    if not root:
        return []
    result = []
    queue = deque([root])
    while queue:
        level_size = len(queue)     # snapshot — THIS is what separates levels
        level = []
        for _ in range(level_size):
            node = queue.popleft()
            level.append(node.val)
            if node.left: queue.append(node.left)
            if node.right: queue.append(node.right)
        result.append(level)
    return result
```

**Complexity:** O(n) time, O(n) space (worst case, the last level of a wide tree).

**Follow-ups:** "Do it in a zigzag (alternating left-to-right, right-to-left) order." — same structure, reverse alternating levels before appending, or build each level as a deque and append to whichever end alternates.

---

## BST: Kth Smallest Element in a BST

**Problem:** given a BST, find the kth smallest value.

**Approach:** the BST invariant means an *inorder* traversal visits nodes in sorted order — so the kth smallest is simply the kth value visited by an inorder traversal. Stop early once you've counted k, rather than traversing the whole tree.

```python
def kthSmallest(root, k):
    stack = []
    node = root
    count = 0
    while stack or node:
        while node:
            stack.append(node)
            node = node.left
        node = stack.pop()
        count += 1
        if count == k:
            return node.val
        node = node.right
```

**Complexity:** O(h + k) — h to descend to the leftmost node initially, then k steps of the traversal; O(n) worst case if k is close to n, but meaningfully better than a full traversal when k is small.

**Follow-ups:** "What if the BST is modified frequently (inserts/deletes) and you need this repeatedly?" — augment each node with a subtree-size count, letting you navigate directly to the kth smallest in O(h) without a full traversal each time — a strong, proactive optimization to offer.

---

## Heap: Top K Frequent Elements

**Problem:** given an array, return the k most frequent elements.

**Approach:** count frequencies with a hash map, then maintain a min-heap of size k over `(frequency, value)` pairs — pushing each new element and popping whenever the heap exceeds size k keeps only the k largest frequencies seen so far, in O(n log k) rather than sorting all n unique elements in O(n log n).

```python
import heapq
from collections import Counter

def topKFrequent(nums, k):
    counts = Counter(nums)
    heap = []
    for val, freq in counts.items():
        heapq.heappush(heap, (freq, val))
        if len(heap) > k:
            heapq.heappop(heap)
    return [val for freq, val in heap]
```

**Complexity:** O(n log k) — a meaningful improvement over O(n log n) full sort when k is much smaller than the number of unique elements.

**Follow-ups:** "Can you do better than O(n log k)?" — yes, with bucket sort: since frequency is bounded by `n` (array length), create buckets indexed by frequency, place each value in its frequency's bucket, then read off the top k from the highest-frequency buckets downward — O(n) overall, a genuinely stronger answer if you can derive it.

---

## Trie: Implement Trie (Prefix Tree)

**Problem:** implement `insert(word)`, `search(word)` (exact match), and `startsWith(prefix)` (any word has this prefix).

**Approach:** each node holds a dictionary of child characters and an `is_end` flag marking a complete word. Insertion walks/creates nodes character by character; search/prefix-check walks existing nodes and fails fast if any character is missing.

```python
class TrieNode:
    def __init__(self):
        self.children = {}
        self.is_end = False

class Trie:
    def __init__(self):
        self.root = TrieNode()

    def insert(self, word):
        node = self.root
        for ch in word:
            node = node.children.setdefault(ch, TrieNode())
        node.is_end = True

    def _find(self, word):
        node = self.root
        for ch in word:
            if ch not in node.children:
                return None
            node = node.children[ch]
        return node

    def search(self, word):
        node = self._find(word)
        return node is not None and node.is_end

    def startsWith(self, prefix):
        return self._find(prefix) is not None
```

**Complexity:** O(L) per operation, where L is the word/prefix length — independent of how many words are stored, which is the entire point over scanning a list of strings.

**Follow-ups:** "How would you support wildcard search (e.g., `.` matches any character)?" — DFS through the trie, branching over all children at a `.` position instead of a single lookup — a natural backtracking-over-a-trie extension.

### Quick Revision
- Level order: snapshot queue length before the inner loop — that's what separates levels.
- BST inorder = sorted order — the single fact that unlocks most BST problems.
- Top-K with a heap: O(n log k); bucket sort achieves O(n) by exploiting frequency being bounded by array length.
- Trie: O(L) per operation regardless of how many words are stored — the core payoff over a plain hash set of strings.
