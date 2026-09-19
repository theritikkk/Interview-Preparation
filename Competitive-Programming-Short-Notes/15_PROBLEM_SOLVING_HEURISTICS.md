# 15 — Problem Solving Heuristics

---

## Step 0: read the constraints first

The constraint line tells you the intended complexity, which usually tells you the technique.

| You see | Think |
|---|---|
| `n ≤ 20` | bitmask over subsets |
| `n ≤ 40` | meet in the middle |
| `n ≤ 100` / `n ≤ 500` | O(n³) is fine → interval DP, Floyd, matrix |
| `n ≤ 5000` | O(n²) DP |
| `n ≤ 2e5`, `q ≤ 2e5` | O((n+q) log n) → sort / segment tree / binary search on answer |
| `n ≤ 1e6` | must be O(n) or O(n log n) with a small constant |
| `a_i ≤ 1e9`, `n ≤ 1e5` | coordinate compression, or the answer depends on order not value |
| `a_i ≤ 100` | values are small → count array, DP over values |
| `n ≤ 1e18` | math / binary search / matrix power / digit DP |
| "sum of n ≤ 2e5" | per-test O(n log n) is fine |
| "print any valid answer" | constructive — there is freedom, use it |
| output is YES/NO | look for an invariant or a parity argument |

---

## The 20 heuristics

### 1. Sort it

**Trigger:** the problem doesn't depend on the input order.
**Example:** "pair up 2n people to minimize the sum of |a_i − a_j|" → sort, pair adjacent.
Sorting is free at n ≤ 1e5 and turns "for every pair" into "for every adjacent pair" surprisingly often.

### 2. Reverse the process

**Trigger:** deletions, destruction, "the last one standing", merges.
**Example:** "repeatedly delete a node; after each deletion report the number of components." Deletions are hard, additions are easy with DSU → process the queries backwards and **add** nodes.

### 3. Binary search the answer

**Trigger:** "minimize the maximum", "maximize the minimum", "smallest k such that…".
**Example:** "split the array into k parts minimizing the largest part sum" → binary search the largest sum, greedily check feasibility.

### 4. Look for monotonicity

**Trigger:** anything where a larger budget can't hurt.
**Example:** longest subarray with sum ≤ K on positive values — as `r` grows, the minimal valid `l` never decreases → two pointers.

### 5. Prefix / suffix information

**Trigger:** "split the array at some point", "remove one element".
**Example:** "maximum of (max of prefix) + (max of suffix) over all split points" → two arrays, one pass each.

### 6. Count frequencies, not positions

**Trigger:** values are bounded and positions are irrelevant.
**Example:** "minimum swaps to make the array a permutation" — often you only need `cnt[v]`, not where each `v` sits. Frequency arrays also make an O(n²) pair count into an O(V) sum.

### 7. Parity

**Trigger:** operations change something by ±2, or by exactly 1 each time; "is it reachable".
**Example:** you can swap adjacent elements → the parity of the number of inversions is invariant. 15-puzzle solvability is exactly this.

### 8. Find the invariant

**Trigger:** "you may apply operation X any number of times; is state B reachable from A?"
**Example:** an operation preserving the multiset sum mod k, or the XOR of all elements, or the sorted multiset. Compute the invariant for A and B; if they differ, the answer is NO. Often the invariant *is* the whole solution.

### 9. Model it as a graph

**Trigger:** pairwise constraints, "swap positions i and j", dependencies, states with transitions.
**Example:** "you may swap a_i and a_j for given pairs (i,j)" → build a graph on indices; within each connected component you can achieve **any** permutation → sort each component's values independently.

### 10. Greedy with an exchange argument

**Trigger:** "choose a subset / an order to optimize something".
**How to validate:** assume an optimal solution differs from the greedy choice; show you can swap toward the greedy choice without making things worse. If you can't construct that argument, **your greedy is probably wrong** — test it against brute force.
**Example:** scheduling to minimize total lateness → sort by deadline; the exchange argument is two adjacent jobs.

### 11. Take the extremes

**Trigger:** constructive problems, "print any".
**Example:** "make the array have exactly k distinct values" → take the largest and smallest and work inward. The extreme element often has a forced role.

### 12. Count contributions instead of objects

**Trigger:** "sum over all subarrays/subsets of f(...)" — enumerating them is exponential or quadratic.
**Example:** "sum over all subarrays of their minimum". Instead of enumerating subarrays, ask for **each element**: in how many subarrays am I the minimum? → monotonic stack, O(n).
**This is one of the highest-value reframings in competitive programming.**

### 13. Count the complement

**Trigger:** "at least one", "not all".
**Example:** "count subsets containing at least one even number" = total − "subsets with no even number". Almost always easier.

### 14. Process queries offline

**Trigger:** all queries are given upfront and there's no forced online requirement.
**Example:** sort queries by right endpoint and sweep, or sort by answer and use DSU. Offline unlocks sorting, which unlocks everything else. Mo's algorithm is the extreme version.

### 15. Compress coordinates

**Trigger:** values up to 1e9, n up to 1e5, and only relative order matters.
**Example:** a segment tree indexed by value → compress values to ranks first.

### 16. Precompute

**Trigger:** a query repeats the same sub-computation.
**Example:** factorials + inverse factorials for nCr; sparse table for range min; binary lifting for LCA; prefix sums for range sums. Pay O(n log n) once, answer q queries in O(1).

### 17. Is n small enough for bitmasking?

**Trigger:** `n ≤ 20`, or "the set of chosen items matters", or TSP-shaped.
**Example:** assignment problem, TSP, set cover, "partition into groups".

### 18. Is the state space smaller than it looks?

**Trigger:** a DP state with many parameters.
**Example:** `dp[i][j][k]` where `k` is determined by `i` and `j` → drop it. Or: values are bounded by 100 so "sum so far" only needs 100·n states. Or: only the *difference* between two counters matters.

### 19. Does gcd / modular structure matter?

**Trigger:** "you may add x any number of times", "jumps of size a and b".
**Example:** starting at 0 with steps of a and b, you can reach exactly the multiples of `gcd(a,b)`. Cycle-length problems reduce to lcm.

### 20. Turn it into intervals

**Trigger:** ranges, time windows, "active during".
**Example:** "for each person, they're busy from l to r; find the busiest moment" → sweep line with +1/−1 events.

### Bonus: think about the final state, not the process

**Trigger:** a long sequence of operations.
**Example:** instead of simulating 1e9 operations, ask what the result looks like. Often the process stabilizes, cycles, or the answer only depends on a count.

### Bonus: meet in the middle

**Trigger:** `n ≤ 40` with a subset-sum shape. Split into two halves of 20, enumerate 2^20 each, sort one half, binary search from the other. O(2^(n/2) · n/2). Full code in `16_MICRO_TEMPLATES.md`.

---

## The "stuck for 15 minutes" checklist

Run this *out loud*. Most of the time one of these fires.

**About the input**
```
[ ] Did I re-read the statement? (Seriously. Do it now.)
[ ] What exactly are the constraints? Did I miss a bound on values, or "sum of n"?
[ ] What happens for n = 1, n = 2, n = 3? Compute by hand.
[ ] Did I look at the sample explanation, not just the numbers?
[ ] Is there a hidden guarantee ("the array is a permutation", "all distinct", "a_i > 0")?
```

**About the structure**
```
[ ] Can I sort it?
[ ] Is there monotonicity → binary search or two pointers?
[ ] Can I reverse the process?
[ ] Can I use prefix/suffix arrays?
[ ] Can I count frequencies instead of positions?
[ ] Does parity matter?
[ ] Is there an invariant the operation preserves?
[ ] Can I model it as a graph? What are the nodes? What are the edges?
[ ] Can I turn it into intervals / events?
[ ] Does gcd or a cycle length matter?
```

**About the counting**
```
[ ] Can I count contributions per element instead of per object?
[ ] Can I count the complement?
[ ] Can I decompose "exactly K" into "atMost(K) − atMost(K−1)"?
[ ] Can I fix one variable and count the other?
```

**About the algorithm**
```
[ ] Is n small enough for a bitmask? For O(n²)? For brute force?
[ ] Is the state space actually smaller than it looks?
[ ] Can I binary search on the answer?
[ ] Can I process the queries offline?
[ ] Can I compress coordinates?
[ ] Can I precompute something and make each query O(1)?
[ ] Would a greedy work — and can I actually justify it?
```

**About me**
```
[ ] Am I solving a harder version than the one asked?
[ ] Am I assuming a constraint that isn't there?
[ ] Have I been on this problem longer than it's worth? (Check the standings.)
```

---

## Constructive problems

- "Print any valid answer" = you have freedom. **Use the simplest construction you can prove**, not the cleverest.
- Try to build from the extremes (largest/smallest, first/last).
- Try small cases by hand: n = 1, 2, 3, 4. Look for the pattern.
- Handle parity separately — very often even n and odd n need different constructions.
- If a construction exists for n, can you extend it to n+1 or n+2? Induction is the usual shape.
- If you suspect "impossible", find the invariant that proves it.

---

## Greedy: when is it actually correct?

A greedy is correct if you can show the **exchange argument**: any optimal solution can be transformed into the greedy one without getting worse.

**Greedy usually works when:** sorting by one key makes the choice locally forced, there's a matroid structure, or the problem has an obvious "take the best available" shape (interval scheduling, Huffman, Kruskal).

**Greedy usually fails when:** there's a budget/capacity that makes early choices constrain later ones in a non-obvious way (knapsack), or when two criteria compete.

**Fastest validation in a contest:** write the brute force and stress test on n ≤ 6. Ten minutes of stress testing beats forty minutes of "I think it's right".

---

## Reading the statement — the trap list

```
[ ] 1-indexed or 0-indexed?
[ ] inclusive or exclusive ranges?
[ ] "distinct" — guaranteed or not?
[ ] can the array contain zeros? negatives?
[ ] does the output need a specific case ("YES" vs "Yes")?
[ ] multiple test cases — is there a blank line between outputs?
[ ] is the answer modded?
[ ] are the graph edges directed?
[ ] can the graph have self-loops or multi-edges?
[ ] is the tree rooted? at which node?
[ ] "print any" or "print the lexicographically smallest"?
[ ] does the problem ask for a count, an index, or a value?
```

---

## Contest strategy

- **Read all the problems** (or at least skim A–D) before deep-diving. The "hard" letter isn't always the hard problem.
- **Check the standings.** If 2000 people solved C and you're stuck on B, you're missing something simple in B.
- **Penalty math:** on Codeforces (Div. rounds, standard scoring), each wrong submission costs 50 points on that problem. A wrong submission is cheap; 20 minutes of staring is expensive. But a *careless* wrong submission on a problem you then take 30 more minutes on is the worst of both.
- **When to skip:** if you've been on a problem 20+ minutes with no idea, and there's an unread problem, switch. Come back with a fresh head.
- **Before resubmitting after a WA:** re-read the statement first. About a third of WAs are misreadings, and resubmitting the same misunderstanding costs another 50.
- **Last 10 minutes:** don't start a new problem. Stress test what you have, or check edge cases on your submitted solutions.