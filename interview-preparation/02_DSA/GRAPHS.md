# DSA Worked Problems: Graphs

---

## BFS: Rotting Oranges (Multi-Source BFS)

**Problem:** a grid has fresh oranges (1), rotten oranges (2), and empty cells (0). Each minute, a rotten orange rots its adjacent fresh oranges. Return the minimum minutes until no fresh orange remains, or -1 if impossible.

**Approach:** start BFS from *all* rotten oranges simultaneously (multi-source), not one at a time — this naturally gives the correct minute-by-minute spread, since BFS explores in increasing-distance order and all sources start at distance 0 together.

```python
from collections import deque

def orangesRotting(grid):
    rows, cols = len(grid), len(grid[0])
    queue = deque()
    fresh = 0
    for r in range(rows):
        for c in range(cols):
            if grid[r][c] == 2: queue.append((r, c, 0))
            elif grid[r][c] == 1: fresh += 1

    minutes = 0
    while queue:
        r, c, t = queue.popleft()
        minutes = max(minutes, t)
        for dr, dc in [(-1,0),(1,0),(0,-1),(0,1)]:
            nr, nc = r+dr, c+dc
            if 0 <= nr < rows and 0 <= nc < cols and grid[nr][nc] == 1:
                grid[nr][nc] = 2
                fresh -= 1
                queue.append((nr, nc, t+1))
    return minutes if fresh == 0 else -1
```

**Complexity:** O(rows × cols) — each cell enqueued at most once.

**Follow-ups:** "Why multi-source instead of running BFS from each rotten orange separately?" — separate BFS runs would need to be merged/compared afterward and would redo overlapping work; starting all sources in the queue together lets one BFS pass naturally compute the correct simultaneous spread.

---

## DFS: Number of Islands

**Problem:** given a grid of '1' (land) and '0' (water), count the number of islands (connected land regions, 4-directionally).

**Approach:** scan the grid; on finding an unvisited '1', DFS (or BFS) to mark the entire connected region as visited, incrementing the island count once per such discovery.

```python
def numIslands(grid):
    rows, cols = len(grid), len(grid[0])
    def dfs(r, c):
        if r < 0 or r >= rows or c < 0 or c >= cols or grid[r][c] != '1':
            return
        grid[r][c] = '0'   # mark visited by sinking the land — avoids a separate visited set
        for dr, dc in [(-1,0),(1,0),(0,-1),(0,1)]:
            dfs(r+dr, c+dc)

    count = 0
    for r in range(rows):
        for c in range(cols):
            if grid[r][c] == '1':
                count += 1
                dfs(r, c)
    return count
```

**Complexity:** O(rows × cols) — each cell visited a constant number of times.

**Follow-ups:** "What if the grid is huge and DFS's recursion depth risks a stack overflow?" — convert to iterative DFS with an explicit stack, or BFS with a queue — same correctness, avoids Python's/any language's call-stack depth limit on a pathologically long, thin island.

---

## Topological Sort: Course Schedule II

**Problem:** given `numCourses` and prerequisite pairs `[a, b]` (must take `b` before `a`), return a valid course order, or an empty array if impossible (a cycle exists).

**Approach:** Kahn's algorithm — compute in-degree for every course, start a queue with all in-degree-0 courses (no prerequisites), repeatedly pop a course, add it to the order, and decrement its neighbors' in-degrees, enqueueing any that hit zero. If the final order doesn't include all courses, a cycle blocked some of them.

```python
from collections import deque

def findOrder(numCourses, prerequisites):
    graph = {i: [] for i in range(numCourses)}
    in_degree = [0] * numCourses
    for course, prereq in prerequisites:
        graph[prereq].append(course)
        in_degree[course] += 1

    queue = deque([c for c in range(numCourses) if in_degree[c] == 0])
    order = []
    while queue:
        c = queue.popleft()
        order.append(c)
        for nxt in graph[c]:
            in_degree[nxt] -= 1
            if in_degree[nxt] == 0:
                queue.append(nxt)

    return order if len(order) == numCourses else []
```

**Complexity:** O(V + E).

**Follow-ups:** "How do you detect the cycle specifically, not just that one exists?" — the courses never added to `order` (in_degree never reached 0) are exactly the ones involved in (or dependent on) a cycle — you can report them directly as the diagnostic, rather than only a boolean impossible/possible.

---

## Union-Find: Number of Provinces

**Problem:** given an adjacency matrix of cities (`isConnected[i][j] == 1` means directly connected), find the number of provinces (connected groups).

**Approach:** union every directly-connected pair; the final answer is the number of distinct roots remaining.

```python
def findCircleNum(isConnected):
    n = len(isConnected)
    parent = list(range(n))
    def find(x):
        if parent[x] != x:
            parent[x] = find(parent[x])   # path compression
        return parent[x]
    def union(x, y):
        rx, ry = find(x), find(y)
        if rx != ry:
            parent[rx] = ry

    for i in range(n):
        for j in range(i+1, n):
            if isConnected[i][j] == 1:
                union(i, j)

    return len({find(i) for i in range(n)})
```

**Complexity:** O(n² · α(n)) — dominated by scanning the adjacency matrix; the union-find operations themselves are near-O(1) amortized each.

**Follow-ups:** "Why not just DFS/BFS from each unvisited city instead?" — both work at the same overall complexity here since the input is already a full adjacency matrix; union-find's real advantage shows up when edges arrive *incrementally over time* (a dynamic graph), where DFS/BFS would need a full re-traversal per update but union-find handles each new edge in near-O(1) — a strong, precise answer to "why one over the other."

### Quick Revision
- Multi-source BFS: seed the queue with all sources at once — one pass gives the correct simultaneous spread.
- DFS "sinking" a visited cell in-place avoids a separate visited set — a common, clean trick.
- Kahn's algorithm: courses left out of the final order are exactly the ones involved in a cycle — a strong diagnostic, not just pass/fail.
- Union-Find's real advantage over repeated DFS/BFS is for *incrementally arriving* edges, not a static, fully-known graph.
