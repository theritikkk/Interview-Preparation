# DSA Notes 06 — Time Complexity, Loops & Recurrences (High-Yield Deep Dive)

> **Why this file exists:** "Time Complexity" is the second video topic for Algorithms and the single most reusable skill in the paper — it quietly powers OS scheduling, DB indexing, CN window math, and every "what is the complexity" MCQ in other subjects. GATE tests it in four shapes: (1) **growth comparisons** (which f is O(g)); (2) **loop counting** (exact iterations at a given n); (3) **recurrence solving** (substitution, recursion tree, master theorem — both the asymptotic class and **exact small-n values**); (4) **space of recursion** (stack depth). Every numeric answer below was **verified by direct computation**; every master-theorem case was applied mechanically and cross-checked.
>
> **Companion question bank:** `../DSA_07_Searching_Asymptotic_Complexity.md`. PW source: `../Reference/PW_Subject_Handbooks/PW_Algorithms.txt` (§1.3–1.9: complexity definitions, types of analysis, asymptotic notation + properties table §1.7.4, iterative analysis, substitution & recursion tree, **Master's method §1.9.2**, standard recurrences).

---

## 0. The mental model: complexity = a *counting machine*

```
   code  ──►  count how many times the DOMINANT operation runs as a function of n
              │
              ├─ loop analysis:   each loop = a count function  (arithmetic / geometric / harmonic)
              ├─ recursion:       T(n) = a·T(n/b) + (work outside calls)
              └─ answer shape:    EXACT value at specific n   OR   asymptotic class (O/Θ/Ω)
```

**Rule #1 of GATE complexity questions:** read what is being asked — *exact number* (plug in n and count) versus *asymptotic order* (drop constants and lower terms). They are different questions and the exam alternates between them.

---

## 1. Asymptotic notation — definitions you must be able to state

For functions f, g : ℕ → ℝ⁺ (eventually positive):

| Notation | Meaning | Quantifier form |
|---|---|---|
| f = O(g) | f grows **no faster than** g | ∃ c > 0, n₀ : f(n) ≤ c·g(n) ∀ n ≥ n₀ |
| f = Ω(g) | f grows **at least as fast as** g | ∃ c > 0, n₀ : f(n) ≥ c·g(n) ∀ n ≥ n₀ |
| f = Θ(g) | same order | f = O(g) **and** f = Ω(g) |
| f = o(g) | **strictly** slower | f(n)/g(n) → 0 (e.g. n = o(n log n)) |
| f = ω(g) | **strictly** faster | f(n)/g(n) → ∞ (e.g. n² = ω(n log n)) |

**Properties (PW §1.7.4 — a recurring MSQ):**

- **O, Ω, Θ**: reflexive ✔, transitive ✔. **Θ** is also *symmetric* (f = Θ(g) ⇔ g = Θ(f)) — so Θ is an **equivalence relation**.
- **o, ω**: transitive but **not reflexive** (strict inequality) and not symmetric.
- **Transpose symmetry**: f = O(g) ⇔ g = Ω(f); f = o(g) ⇔ g = ω(f).
- **Θ ⇒ O and Ω together**: f = Θ(g) means *both* f = O(g) and f = Ω(g).
- Two-function recipe: compare lim f(n)/g(n) → 0, c, or ∞ to pick o / Θ / ω.

---

## 2. The growth ladder (know the gaps, not just the order)

```
   1  ≺  log log n  ≺  log n  ≺  n^ε  (any ε>0)  ≺  n  ≺  n log n  ≺  n²  ≺  n³  ≺  ...  ≺  2ⁿ  ≺  n!  ≺  nⁿ
```

Standard facts that show up as options:

- **Any polylog loses to any polynomial**: (log n)¹⁰⁰⁰ = o(n^0.001). Conversely n^(0.001) = o(log n)? **No** — swap the direction: n^0.001 grows *faster* than (log n)^1000 for large n.
- **log bases are constants**: log₂ n = Θ(log₁₀ n) = Θ(ln n) (factor 1/ln 2 etc.).
- **log(n!) = Θ(n log n)** (Stirling) — the natural bridge between sorting's lower bound and merge sort.
- **Harmonic sum**: Hₙ = 1 + 1/2 + … + 1/n = Θ(log n).
- **Power sums**: Σ i = Θ(n²), Σ i² = Θ(n³), Σ iᵏ = Θ(n^{k+1}).
- **Geometric sums**: Σ_{i=0}^{n} 2ⁱ = Θ(2ⁿ); Σ (3/2)ⁱ = Θ((3/2)ⁿ) — a geometric series is dominated by its **last** term.
- **Knife-edge comparisons** (classic GATE): n^{log₂ 3} vs n^{1.58} — since log₂ 3 ≈ 1.585 > 1.58, n^{log₂ 3} is the bigger (ω). Compare 2ⁿ vs n^{100}: exponential wins for large n (but n^{100} wins for "small" n — hence n₀ in every definition).

---

## 3. Loop analysis — a recipe plus five patterns

**Recipe:** write each loop's iteration count as a function, then multiply (nested independent) or sum (nested dependent). A loop whose counter multiplies/divides by a constant is **logarithmic**; adding a constant is **arithmetic**; stepping by i gives **harmonic** sums.

| Pattern | Iterations | Order |
|---|---|---|
| `for (i=1; i<=n; i++)` | n | Θ(n) |
| `for (i=1; i<=n; i*=2)` | ⌊log₂ n⌋ + 1 | Θ(log n) |
| `for (i=1; i<=n; i*=4)` | ⌊log₄ n⌋ + 1 | Θ(log n) |
| `for(i=1..n) for(j=1..n)`, indep. | n² | Θ(n²) |
| `for(i=1;i<=n;i*=2) for(j=1;j<=i;j*=2)` | Σ_{k=0}^{L}(k+1) = (L+1)(L+2)/2 | Θ(log² n) |
| `for(i=1..n) for(j=i;j<=n;j+=i)` | Σ ⌊n/i⌋ = n·Hₙ | Θ(n log n) |

**Worked micro-examples:**

1. `for (i=1; i<=4096; i*=4)`: values 1, 4, 16, 64, 256, 1024, 4096 → **7** iterations (⌊log₄ 4096⌋ + 1 = 6 + 1).
2. The log²n loop: with n = 8, outer i = 1, 2, 4, 8 and inner runs 1, 2, 3, 4 times → 10 = (3+1)(3+2)/2 with L = 3 ✔.
3. The harmonic loop: with n = 4, inner counts 4 + 2 + 1 + 1 = 8, and 4·H₄ ≈ 8.3 ✔.

**While-loop sleeper:** `while (n > 1) n = n/2;` → ⌊log₂ n⌋ iterations. `while (i*i <= n) i++;` → Θ(√n).

---

## 4. Recurrences I — substitution and recursion trees

### 4.1 Substitution (unrolling) — fully worked

```
   T(1) = 1
   T(n) = T(n−1) + 2n
```

Unroll: T(n) = T(n−1) + 2n = T(n−2) + 2(n−1) + 2n = … = T(1) + 2·(2 + 3 + … + n)

```
   = 1 + 2·( n(n+1)/2 − 1 ) = n² + n − 1        → T(n) = Θ(n²)
   Check n = 10: 100 + 10 − 1 = 109 ✔
```

The **step function matters**: T(n−1) + *2n* telescopes to a sum of an arithmetic series (Θ(n²)); T(n−1) + *1* would give Θ(n); T(n−1) + *2ⁿ* would give Θ(2ⁿ) (geometric series dominated by its last term).

### 4.2 Recursion tree — the general bookkeeping

For T(n) = aT(n/b) + f(n):

```
   level 0:  1 node  · f(n)                work = f(n)
   level 1:  a nodes · f(n/b)              work = a·f(n/b)
   level k:  aᵏ nodes · f(n/bᵏ)            work = aᵏ·f(n/bᵏ)
   …
   leaves at depth log_b n:  a^{log_b n} = n^{log_b a} leaves, each base cost Θ(1)
```

The three cases of the master theorem are just three answers to "which row of this tree dominates?".

**Fully worked finite example — T(n) = 3T(n/2) + n, T(1) = 1:**

```
   T(2)  = 3·T(1) + 2   = 5
   T(4)  = 3·5 + 4      = 19
   T(8)  = 3·19 + 8     = 65
   T(16) = 3·65 + 16    = 211
```

(Level sums n·(3/2)ᵏ *increase* with depth → leaves dominate asymptotically: n^{log₂ 3} = n^{1.585}. But note the exact value at n=16 is the finite number **211** — exact-value and asymptotic questions are different.)

### 4.3 Standard recurrence table (learn as pairs)

| Recurrence | Solution | Where it comes from |
|---|---|---|
| T(n) = T(n/2) + 1 | Θ(log n) | binary search |
| T(n) = T(n/2) + n | Θ(n) | geometric decay (n + n/2 + … = 2n) |
| T(n) = 2T(n/2) + 1 | Θ(n) | tree counting / leaf-dominated |
| T(n) = 2T(n/2) + n | Θ(n log n) | merge sort |
| T(n) = 2T(n/2) + n² | Θ(n²) | root-dominated: n²(1 + ½ + ¼ + …) |
| T(n) = 4T(n/2) + n | Θ(n²) | a = 4 leaves beat the root |
| T(n) = 7T(n/2) + n² | Θ(n^{log₂ 7}) ≈ n^{2.81} | Strassen |
| T(n) = 9T(n/3) + n | Θ(n²) | n^{log₃ 9} = n² leaves |
| T(n) = T(n−1) + 1 | Θ(n) | linear recursion |
| T(n) = T(n−1) + n | Θ(n²) | quicksort worst case |
| T(n) = 2T(n−1) + 1 | Θ(2ⁿ) | Tower of Hanoi |
| T(n) = T(n−1) + T(n−2) + 1 | Θ(φⁿ), φ = (1+√5)/2 ≈ 1.618 | exponential branching |
| T(n) = T(√n) + 1 | Θ(log log n) | shrink-by-square-root (substitute m = log n) |
| T(n) = T(n/3) + T(2n/3) + n | **Θ(n log n)** | unbalanced-but-additive split (see §5) |

---

## 5. Recurrences II — the Master Theorem (and when it refuses to apply)

**Standard form** T(n) = a·T(n/b) + Θ(nᵏ), a ≥ 1, b > 1. Compare the leaves n^{log_b a} against the root nᵏ:

| Case | Condition | Dominant | Answer |
|---|---|---|---|
| 1 | log_b a > k | leaves | Θ(n^{log_b a}) |
| 2 | log_b a = k | balanced | Θ(nᵏ log n) |
| 3 | log_b a < k | root | Θ(nᵏ) |

**Extended form** T(n) = a·T(n/b) + Θ(nᵏ logᵖ n) (PW §1.9.2) adds the log-exponent:

```
   log_b a > k                 →  Θ(n^{log_b a})                  (p changes nothing)
   log_b a = k  and p > −1     →  Θ(n^k · log^{p+1} n)
   log_b a = k  and p = −1     →  Θ(n^k · log log n)
   log_b a = k  and p < −1     →  Θ(n^k)
   log_b a < k  and p ≥ 0      →  Θ(n^k · log^p n)
   log_b a < k  and p < 0      →  Θ(n^k)
```

**Worked applications:**

1. **T(n) = 2T(n/4) + n^{1.5}:** log₄ 2 = 0.5 < 1.5 → case 3. Regularity: 2·(n/4)^{1.5} = n^{1.5}/4 ≤ c·n^{1.5} ✔ → **Θ(n^{1.5})**.
2. **T(n) = 3T(n/2) + n:** log₂ 3 ≈ 1.585 > 1 → case 1 → **Θ(n^{log₂ 3})**.
3. **T(n) = 2T(n/2) + n log n:** a = b = 2, k = 1, p = 1 → log_b a = k and p > −1 → Θ(n¹ log^{1+1} n) = **Θ(n log² n)**. (The *basic* master theorem alone cannot see the extra log — this is a favourite trap.)
4. **T(n) = T(n/3) + T(2n/3) + n:** *master theorem does not apply* (subproblems have different sizes). Direct argument: every level of the recursion tree partitions the array into pieces summing to exactly n → Θ(n) work per level; the deepest branch shrinks by (2/3) each time → depth Θ(log n) (base 3/2) → **Θ(n log n)**.

**When master fails — recognize immediately:** unequal subproblem sizes (T(n/3)+T(2n/3)), subtraction inside (T(n−1)), products (T(n/2)·T(n/2)), or non-polynomial f (T(n/2) + n/log n — extended form needed).

---

## 6. Space complexity of recursion = depth × frame size

- A recursion that calls itself **once on n−1** stacks Θ(n) frames: `sum(n) = n + sum(n−1)` → Θ(n) space.
- **Halving** recursion (`T(n/2)+…`) stacks Θ(log n): binary search, merge sort's control stack.
- Merge sort: Θ(log n) *stack* but Θ(n) *auxiliary array* → total Θ(n). Distinguish "stack depth" from "total extra space".
- Quicksort: stack = recursion depth = O(log n) expected, **O(n) worst** (bad pivots).
- Iterative algorithms (heapsort's sift-down) add only Θ(1).
- Tower of Hanoi: 2ⁿ calls but only n+1 frames deep → Θ(n) space, Θ(2ⁿ) time.

---

## 7. Traps checklist

1. **Exact value vs asymptotic class.** "T(16) = ?" wants 211, not "Θ(n^{1.585})". Read the blank.
2. **Basic master theorem can't handle n^k log^p n at the critical line** — use the extended form (case 2 with log p → log^{p+1}).
3. **T(n/3) + T(2n/3) + n is Θ(n log n)**, not Θ(n) (some think each level is n and ignore depth) and not Θ(n²) (some apply master wrongly). Master is *not applicable at all* here.
4. **Base change confusion**: log₄ 2 = ½ (not 2). Compute log_b a as ln a / ln b carefully — a = 2, b = 4 → 0.5.
5. **n! is not exponential**: n! = ω(2ⁿ); conversely n! = o(nⁿ). log(n!) = Θ(n log n).
6. **(log n)^100 vs n^0.01**: polynomial wins; "polylog beats any polynomial" is the *reversed* lie examiners plant.
7. **O vs Θ in the answer options**: f = Θ(g) is stronger than f = O(g). If the question asks "tight bound", Θ answers beat O answers.
8. **Constants vanish in class but not in exact counts**: T(n) = 2T(n/2) + c is Θ(n) for every constant c — but the value at n = 16 depends on c.

---

## 8. Practice set (10 questions, all values computed exactly)

**Q1 · MCQ · Fundamental** — Which statement is TRUE?
(A) 3ⁿ = Θ(2ⁿ)  (B) log₂ n = Θ(log₁₀ n)  (C) n! = Θ(2ⁿ)  (D) n² + n = Θ(n³)

**Q2 · NAT · Fundamental** — How many times does the loop body execute?
`for (i = 1; i <= 4096; i = i * 4)  count++;` → ______

**Q3 · NAT · Standard** — For T(1) = 1 and T(n) = T(n−1) + 2n, the value of T(10) is ______. (Also give the closed form.)

**Q4 · MCQ · Standard** — The recurrence T(n) = 2T(n/4) + n^{1.5} solves to:
(A) Θ(n^{0.5})  (B) Θ(n^{1.5})  (C) Θ(n^{1.5} log n)  (D) Θ(n²)

**Q5 · NAT · Standard** — For T(1) = 1 and T(n) = 3T(n/2) + n (n a power of 2), the value of T(16) is ______.

**Q6 · NAT · Moderate** — The recurrence T(n) = 2T(n/2) + n log n (T(1) = 1) has the tight bound T(n) = Θ(n · log^q n). The integer q is ______.

**Q7 · MSQ · Moderate** — Which statements are TRUE (for eventually-positive functions)?
(A) If f = O(g) then g = Ω(f).
(B) If f = o(g) then f = Θ(g).
(C) Θ is an equivalence relation (reflexive, symmetric, transitive).
(D) n! = O(2ⁿ).

**Q8 · MCQ · Moderate** — The recurrence T(n) = T(n/3) + T(2n/3) + n (T(1) = 1) solves to:
(A) Θ(n)  (B) Θ(n log n)  (C) Θ(n²)  (D) Θ(2ⁿ)

**Q9 · MSQ · Hard** — Which statements are TRUE?
(A) The recursion `T(n) = T(n/2) + 1` uses Θ(log n) stack frames.
(B) `sum(n) = n + sum(n−1)` (base `sum(0)=0`) uses Θ(n) stack frames.
(C) Recursive merge sort's recursion stack is Θ(log n), even though its auxiliary array makes total extra space Θ(n).
(D) Iterative heapsort uses Θ(log n) recursion stack space in its extraction phase.

**Q10 · NAT · Killer** — For T(1) = T(2) = 1 and T(n) = T(n−1) + T(n−2) + 1, the value of T(6) is ______. (Bonus: identify the exponential base.)

---

### Answer key

| Q | Ans | Q | Ans |
|---|---|---|---|
| 1 | B | 6 | 2 |
| 2 | 7 | 7 | A, C |
| 3 | 109 | 8 | B |
| 4 | B | 9 | A, B, C |
| 5 | 211 | 10 | 15 |

### Full solutions

**Q1 — B.** Base change is a constant factor: log₁₀ n = log₂ n / log₂ 10. (A) ✘ 3ⁿ/2ⁿ = (3/2)ⁿ → ∞. (C) ✘ n!/2ⁿ → ∞ (n! = ω(2ⁿ)). (D) ✘ n² + n = Θ(n²), and n² = o(n³).

**Q2 — 7.** Iterations: i = 1, 4, 16, 64, 256, 1024, 4096 → 7 (⌊log₄ 4096⌋ + 1 = 6 + 1).

**Q3 — 109; T(n) = n² + n − 1.** T(10) = 1 + 2(2+3+…+10) = 1 + 2·54 = 109. Closed form: 1 + 2·(n(n+1)/2 − 1) = n² + n − 1 ✔ (10² + 10 − 1 = 109).

**Q4 — B.** a = 2, b = 4 → log_b a = ln2/ln4 = 0.5. k = 1.5 > 0.5 → case 3 (root dominates). Regularity: 2(n/4)^{1.5} = n^{1.5}/4 ≤ c·n^{1.5} ✔. Answer Θ(n^{1.5}).

**Q5 — 211.** T(2) = 3·1 + 2 = 5; T(4) = 3·5 + 4 = 19; T(8) = 3·19 + 8 = 65; T(16) = 3·65 + 16 = **211**. (Asymptotically this is Θ(n^{log₂ 3}) — "exact vs class" trap.)

**Q6 — q = 2.** a = b = 2, k = 1, p = 1 → critical line (log₂ 2 = 1 = k), extended case 2: exponent p+1 = 2 → Θ(n log² n). So q = 2. (Basic master theorem would wrongly stop at Θ(n log n).)

**Q7 — A, C.** (A) ✔ the quantifiers are mirror images (transpose symmetry). (B) ✘ o is *strictly* smaller; Θ is two-sided. (C) ✔ Θ(f,f) holds; symmetric by definition; transitive by constants. (D) ✘ n! grows faster than any fixed exponential: n!/2ⁿ → ∞.

**Q8 — B.** Master doesn't apply. Tree argument: level k work = exactly n (the pieces n/3ᵏ + 2ᵏ·n/3ᵏ of the two branches sum to n across each level); depth until leaf size 1 on the heavier branch = log_{3/2} n → Θ(n log n).

**Q9 — A, B, C.** (A) ✔ depth ⌊log₂ n⌋. (B) ✔ one frame per n-value: n frames. (C) ✔ top-down mergesort recurses log n deep; the Θ(n) is the merge buffer, not the stack. (D) ✘ (sift-down is iterative — Θ(1) aux).

**Q10 — 15; base φ.** T(3) = 1+1+1 = 3; T(4) = 3+1+1 = 5; T(5) = 5+3+1 = 9; T(6) = 9+5+1 = **15**. Pattern: T(n) = 2·Fib(n) − 1 (Fibonacci 1,1,2,3,5,8,13) → T(n) = Θ(φⁿ) with φ = (1+√5)/2 ≈ 1.618 — exponential, not polynomial.

---

## 9. Next steps

1. Re-derive §4.1 (substitution) and the §4.2 tree for 3T(n/2)+n *by hand*; these two mechanics cover most recurrence questions.
2. Drill the §4.3 table to recall-level — half of GATE's recurrence MCQs are seated in it.
3. Then `DSA_Notes_07_Graph_Algorithms.md` — where BFS/DFS/MST/Dijkstra complexities consume exactly these recipes.
