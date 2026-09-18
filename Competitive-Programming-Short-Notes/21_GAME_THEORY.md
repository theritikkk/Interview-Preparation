# 21 — Game Theory

Extremely common at Div2 A/B, almost never taught in a DSA course.

---

## The setup that makes all of this work

**Impartial game:** both players have the same moves available from any position (the moves don't depend on whose turn it is). Nim, subtraction games, most "take stones" problems.
**Normal play:** the player who cannot move **loses**.
**Both players play optimally.**

If the game is *partisan* (moves differ by player, like chess) or uses *misère* rules (the player who moves last loses), the theory below does not apply directly.

---

## Win/lose positions

**Definitions:**
- A position is **losing (P-position)** if *every* move leads to a winning position.
- A position is **winning (N-position)** if *some* move leads to a losing position.
- A terminal position (no moves) is **losing**.

**Memory hook:** *I win if I can hand my opponent a loss.*

```cpp
// subtraction game: remove 1, 3, or 4 stones
vector<bool> win(n + 1, false);
for (int i = 1; i <= n; i++)
    for (int m : {1, 3, 4})
        if (i >= m && !win[i - m]) { win[i] = true; break; }
```
**This brute force is the first thing you should write.** Print `win[0..30]`, look for the pattern, then prove it. For Div2 A/B the pattern is almost always short and periodic.

---

## The classic patterns

### "Take 1 to k stones"

Losing iff `n % (k + 1) == 0`.
**Why:** whatever the opponent takes (say `x`), you take `k+1−x`, keeping the multiple of `k+1` invariant.
**Memory hook:** *mirror the opponent to preserve the multiple.*

### "Take exactly 1 or 2"

Losing iff `n % 3 == 0`.

### Parity games

Many problems reduce to "is n even or odd". If each move changes some quantity by exactly 1 and the game ends at 0, the first player wins iff n is odd.
**Always check parity first.** It's the answer more often than it has any right to be.

### Mirroring / symmetry

If the second player can always mirror the first player's move (across a center, on a symmetric board, on the opposite pile), the **second player wins**. Very common in constructive game problems. Recognition: the board or the setup is symmetric and moves preserve symmetry.

---

## Nim

**Rules:** several piles; a move takes any positive number of stones from exactly one pile; last to move wins.

**Theorem (Bouton):** the position is **losing for the player to move** iff
```
a₁ ⊕ a₂ ⊕ … ⊕ aₙ == 0
```
(`⊕` = XOR, called the **nim-sum**.)

```cpp
int x = 0;
for (int a : piles) x ^= a;
cout << (x ? "First" : "Second") << '\n';
```

**Why it works, informally:** if the XOR is non-zero, you can always move to make it zero (find the highest set bit of the XOR, pick a pile with that bit set, reduce it to `a ^ x` — which is smaller). If the XOR is zero, **any** move makes it non-zero. So the losing positions are exactly the zero-XOR ones, and the terminal position (all empty) has XOR 0.

**The winning move:** find a pile with `(a ^ x) < a` and reduce it to `a ^ x`.

**Memory hook:** *XOR the piles. Zero means you're doomed.*

---

## Sprague–Grundy

**Theorem:** every impartial normal-play game position is equivalent to a Nim pile of some size `g`, the **Grundy number** (nimber).

```
G(position) = mex{ G(p) : p reachable in one move }
```
where **mex** (minimum excludant) is the smallest non-negative integer not in the set.

```cpp
int mex(vector<int>& s) {
    set<int> ss(s.begin(), s.end());
    int m = 0;
    while (ss.count(m)) m++;
    return m;
}
```

**Two facts that make it usable:**
1. `G(position) == 0` ⟺ the position is **losing**.
2. For a game that is the **independent sum** of subgames, `G(total) = G(sub₁) ⊕ G(sub₂) ⊕ …`

That second fact is why Nim's XOR rule works — a Nim pile of size `a` has Grundy number `a`.

### Computing Grundy numbers

```cpp
vector<int> g(N, 0);
for (int i = 1; i < N; i++) {
    set<int> s;
    for (int m : moves) if (i >= m) s.insert(g[i - m]);
    int mx = 0; while (s.count(mx)) mx++;
    g[i] = mx;
}
```
**Contest workflow:** compute Grundy numbers for small n by brute force, print them, **find the periodic pattern**, then use the pattern for large n. Grundy sequences for subtraction games are eventually periodic.

---

## Nim variants

| Variant | Rule |
|---|---|
| **Misère Nim** (last to move *loses*) | If all piles are 1: first player wins iff the number of piles is **even**. Otherwise, same as normal Nim (XOR ≠ 0 → first wins). |
| **Nim with a cap** (take ≤ k from a pile) | Replace each pile `a` by `a mod (k+1)`, then normal Nim. |
| **Staircase Nim** | Only the piles at **odd** positions count; XOR those. |
| **Turning Turtles / coin games** | Each coin is an independent subgame; XOR the Grundy numbers. |
| **Green Hackenbush** | Trees reduce to nimbers via the "colon principle". |

**Misère Nim is the one that catches people.** The special case (all piles equal 1) must be handled separately, and it's the exact opposite of normal play.

---

## Games on graphs (DAG)

Every impartial game is a DAG where nodes are positions and edges are moves. Compute Grundy numbers in reverse topological order.

```cpp
int grundy(int u) {
    if (memo[u] != -1) return memo[u];
    set<int> s;
    for (int v : adj[u]) s.insert(grundy(v));
    int m = 0; while (s.count(m)) m++;
    return memo[u] = m;
}
```
**If the graph has cycles**, the game can be a draw and Grundy theory doesn't apply — you need a BFS from terminal positions computing win/lose/draw with out-degree counters.

---

## Recognition checklist

```
[ ] Is it impartial (same moves for both players)?
[ ] Normal play (can't move = lose) or misère?
[ ] Is n small enough to brute force win/lose and spot a pattern?  ← DO THIS FIRST
[ ] Does the answer depend only on parity?
[ ] Is there a mirroring strategy for the second player?
[ ] Does it decompose into independent subgames?  → XOR the Grundy numbers
[ ] Is it literally Nim in disguise?
[ ] Is there an invariant preserved by every move?
[ ] Is the Grundy sequence periodic?  (compute 50 values and look)
```

**The single most valuable habit:** write the O(n²) brute force for n ≤ 30, print the win/lose table, and stare at it. Div2 game problems almost always have a pattern visible in the first 20 values.

---

## Traps

```
[ ] misère rules read as normal play (the statement says "the player who cannot move wins")
[ ] the game is partisan — Sprague-Grundy does not apply
[ ] the game graph has cycles → draws are possible
[ ] "optimal play" assumed but the problem asks for something else (e.g. the number of moves)
[ ] Grundy number confused with "number of moves remaining"
[ ] XOR of piles vs sum of piles
[ ] n = 0 base case
[ ] pattern extrapolated from too few values (compute 50, not 10)
```

---

## Memory hooks

- *Losing = every move hands the opponent a win.*
- *Nim: XOR the piles; zero means you lose.*
- *Grundy = mex of the reachable Grundy values.*
- *Independent subgames → XOR their Grundy numbers.*
- *Symmetric board + mirroring → second player wins.*
- *Brute force small n and look for the pattern.*