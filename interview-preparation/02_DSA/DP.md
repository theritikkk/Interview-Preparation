# DSA Worked Problems: Dynamic Programming

---

## 1D DP: Coin Change

**Problem:** given coin denominations and a target amount, find the fewest coins needed to make that amount, or -1 if impossible.

**Approach:** define `dp[a]` = fewest coins to make amount `a`. Base case `dp[0] = 0`. For each amount from 1 upward, try every coin: `dp[a] = min(dp[a], dp[a - coin] + 1)` for every coin ≤ a. This is bottom-up tabulation — building up from smaller amounts, each using already-computed smaller results.

```python
def coinChange(coins, amount):
    dp = [float('inf')] * (amount + 1)
    dp[0] = 0
    for a in range(1, amount + 1):
        for coin in coins:
            if coin <= a:
                dp[a] = min(dp[a], dp[a - coin] + 1)
    return dp[amount] if dp[amount] != float('inf') else -1
```

**Complexity:** O(amount × len(coins)) time, O(amount) space.

**Follow-ups:** "Why doesn't a greedy approach (always take the largest coin that fits) work here in general?" — it fails for denominations like `[1, 3, 4]` targeting 6: greedy takes 4+1+1 (3 coins), but the optimal is 3+3 (2 coins) — a concrete counterexample is a far stronger answer than just asserting "greedy doesn't work for coin change in general."

---

## 2D DP: Longest Common Subsequence

**Problem:** given two strings, find the length of their longest common subsequence (not necessarily contiguous).

**Approach:** `dp[i][j]` = LCS length using the first `i` characters of string A and first `j` characters of string B. If the characters match, extend the diagonal's LCS by 1; if not, take the best of excluding one character from either string.

```python
def longestCommonSubsequence(text1, text2):
    m, n = len(text1), len(text2)
    dp = [[0] * (n + 1) for _ in range(m + 1)]
    for i in range(1, m + 1):
        for j in range(1, n + 1):
            if text1[i-1] == text2[j-1]:
                dp[i][j] = dp[i-1][j-1] + 1
            else:
                dp[i][j] = max(dp[i-1][j], dp[i][j-1])
    return dp[m][n]
```

**Complexity:** O(m × n) time and space.

**Follow-ups:** "Can you reduce the space complexity?" — yes: `dp[i][j]` only ever depends on the *previous row* (`dp[i-1][...]`) and the current row's already-computed earlier values — rolling to two 1D arrays (or even one, processed carefully) drops space from O(m×n) to O(n). This is one of the most common DP follow-ups across almost every 2D DP problem — always be ready to offer it.

---

## Knapsack-Style DP: House Robber

**Problem:** given amounts of money in houses along a street, find the max amount robbable without robbing two adjacent houses.

**Approach:** at each house, you either skip it (carry forward the best result without it) or rob it (its value plus the best result from two houses back, since the adjacent one is now off-limits). `dp[i] = max(dp[i-1], dp[i-2] + nums[i])`.

```python
def rob(nums):
    prev2, prev1 = 0, 0
    for x in nums:
        prev2, prev1 = prev1, max(prev1, prev2 + x)
    return prev1
```

**Complexity:** O(n) time, O(1) space — already rolled down from a conceptual `dp` array to two running variables, since each state only needs the two previous values.

**Follow-ups:** "What changes if the houses are arranged in a circle (first and last are adjacent)?" — run the same linear solution twice: once excluding the first house, once excluding the last, and take the max — a clean reduction to the already-solved subproblem rather than a fundamentally new algorithm.

### Quick Revision
- Always state the DP state precisely (`dp[i]` means what, exactly) before writing the recurrence — the most common way DP answers go wrong is skipping this.
- Greedy fails on Coin Change in general — have the `[1,3,4]` targeting 6 counterexample ready.
- 2D → 1D space reduction is the single most common DP follow-up — check whether `dp[i][j]` only depends on the previous row before writing the 2D version, and mention it proactively.
- House Robber's circular variant reduces to two linear runs — recognizing a reduction to an already-solved subproblem is a strong signal.
