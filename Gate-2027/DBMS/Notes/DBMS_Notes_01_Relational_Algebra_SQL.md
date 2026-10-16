# DBMS Notes 01 — Relational Algebra & SQL (High-Yield Deep Dive)

> **Why this file exists:** "Relational Algebra & SQL" is on the high-yield list. GATE's questions here are formula-driven: **operator expansions** (what can be built from what), **cardinality bounds**, the **division / "for all" pattern**, **SQL clause semantics** (execution order, GROUP BY rules), **NULL three-valued logic**, and **set-operation results on concrete tables**. Every practice answer below was verified by exact computation on the sample relations.
>
> **Companion question banks:** `DB_03_Relational_Algebra.md`, `DB_05_SQL_Queries_Joins_Aggregation.md`, `DB_06_SQL_Subqueries_SetOps_Views.md`. PW source: `../Reference/PW_Subject_Handbooks/PW_DBMS.txt` (§3, pages 10.14–10.25).

---

## 0. The mental model: two languages for one job

```
   Relational Algebra (RA)          Relational Calculus / SQL
   ─────────────────────            ──────────────────────────
   PROCEDURAL: "how" to compute     NON-PROCEDURAL: "what" you want
   σ, π, ×, ∪, −, ρ  (primitive)    SQL is the practical declarative language
   ∩, ⋈, ÷          (derived)       (calculus is its theoretical root)
   Input = relation(s), output = ONE relation
```

Three facts that drive every GATE question:

1. **RA is set-based** — the result is always a *set* of tuples: **duplicates never survive** any operator (in particular `π` de-duplicates).
2. **A query = an expression tree** of operators; SQL is compiled into such a tree.
3. **Closure**: every operator takes relation(s) and returns a relation — so you can chain any operators.

---

## 1. The operator toolbox (PW §3.2)

### 1.1 Primitive (fundamental) operators — the "axioms"

```
   σ   selection         pick ROWS satisfying a condition        σ_c(R)
   π   projection        pick COLUMNS (duplicates removed)       π_A(R)
   ×   cross product     every pair of rows                      R × S
   ∪   union             rows in R or S   (R, S union-compatible)
   −   set difference    rows in R but not S  (R, S union-compatible)
   ρ   rename            rename tables/attributes                ρ_x(R)
```

Non-obvious facts GATE loves:

- `σ` **does not** remove duplicates (input is already a set; filtering a set keeps it a set).
- `π` **can reduce cardinality** — projecting `B` from `R(A,B)` gives *distinct* B values.
- `degree(R × S) = deg(R) + deg(S)`; `|R × S| = |R| · |S|`.
- If either relation is empty, `R × S` is empty.

### 1.2 Derived operators — expressible, hence NOT fundamental

```
   ∩   R ∩ S   = R − (R − S)                       (or S − (S − R))
   ⋈   R ⋈ S   = π_{all attrs, deduplicated}( σ_{R.A1=S.A1 ∧ R.A2=S.A2 ∧ ...}(R × S) )
   ÷   R ÷ S   = π_A(R) − π_A( ( π_A(R) × S ) − R )        ← memorize this shape
```

- **Conditional (theta) join:** `R ⋈_c S = σ_c(R × S)`.
- **Natural join with no common attributes:** `R ⋈ S = R × S` (join condition empty).
- The minus-based expansion of `÷` above is *the* classic GATE question ("what does this expression compute?"). Read it bottom-up:

```
   (π_A(R) × S)            all "candidate pairs" that COULD appear in R
   − R                     candidates that DO NOT appear in R  → the "missing pairs"
   π_A( ... )              A-values that are missing some pair → "incomplete A-values"
   π_A(R) − (that)         A-values with NO missing pair       → the quotient
```

**Consequence (expressibility):** the set of things you can write with `{σ, π, ×, ∪, ρ}` = relational algebra *without* `−` — since `⋈` and `∩` are made from `−`, dropping `−` costs you exactly the operators that need it. Queries like "who works on **every** project" or "who works on **no** project" (pure negation/universality) need `−`/`¬` and cannot be done otherwise (outside of a finite-domain trick).

---

## 2. Joins and the cardinality table (PW §3.3, §3.7)

With `|R| = n`, `|S| = m` (distinct tuples):

| Expression | Result cardinality | Why (extremes) |
|---|---|---|
| `R × S` | `n · m` | every pair |
| `R ⋈ S` (natural) | `0 … n·m` | min: no common value; max: one shared value in all rows |
| `R ⟕ S` (left outer) | `n … n·m` | every R-row survives ≥ once; plus extra matches up to m each |
| `R ⟖ S` (right outer) | `m … n·m` | symmetric |
| `R ⟗ S` (full outer) | `max(n,m) … n·m` | both sides padded with NULLs; matched tuples counted once |
| `R ∪ S` | `max(n,m) … n+m` | min: one set contained in the other; max: disjoint |
| `R ∩ S` | `0 … min(n,m)` | disjoint ⇒ 0; nested ⇒ min |
| `R − S` | `0 … n` | all removed ⇒ 0; disjoint ⇒ n |
| `R ÷ S` | `0 … ⌊n/m⌋` | at most n/m complete "groups" of m (careful when m = 0) |

**Outer join semantics (PW §3.3):**

```
   R ⟕ S  =  (R ⋈ S)  ∪  { r ∘ NULLs : r ∈ R, r matched nothing }
   R ⟖ S  =  (R ⋈ S)  ∪  { NULLs ∘ s : s ∈ S, s matched nothing }
   R ⟗ S  =  (R ⟕ S)  ∪  (R ⟖ S)     — matched tuples appear ONCE
```

**Traps:**
- A `left outer join` row is padded with NULLs only when unmatched — matched rows are normal.
- If the join condition is *empty* (no common attributes), *inner* natural join = cross product, but **outer** joins still pad nothing (every row matches something), so `R ⟕ S = R × S` too.
- `max(n,m) ≤ |R ⟗ S| ≤ n·m` — the lower bound is `max(n,m)`, not `n + m` (matches are shared).

---

## 3. Division = "for all" (the single most-asked pattern)

`A ÷ B` where `R(A, B)` and `S(B)`: returns those `A`-values that are paired (in R) with **every** `B`-value in S.

```
   Store(sid, item) ÷ π_item(Item)     →  "students who bought EVERY item"
   Enroll(sid, cid) ÷ π_cid(Course)    →  "students enrolled in EVERY course"
```

Hand method ("count and compare"): for each `A`-value, count **distinct** B-values it has, and check whether that count equals `|S|` **after restricting to B-values of S**. In SQL this becomes:

```sql
-- Form 1: counting
SELECT sid FROM Enroll
GROUP BY sid
HAVING COUNT(DISTINCT cid) = (SELECT COUNT(*) FROM Course);
-- (correct as long as every enrolled cid exists in Course)

-- Form 2: double NOT EXISTS  ("no course exists that the student did NOT take")
SELECT sid FROM Student s
WHERE NOT EXISTS (SELECT 1 FROM Course c
                  WHERE NOT EXISTS (SELECT 1 FROM Enroll e
                                    WHERE e.sid = s.sid AND e.cid = c.cid));
```

`NOT EXISTS` is the reliable form; the counting form silently breaks if the tables contain rows outside the universe (e.g., enrollments in `cid` values not in Course) or duplicate enrollments (unless you use `COUNT(DISTINCT)`).

---

## 4. SQL core: the clause skeleton

```sql
SELECT   [DISTINCT] A1, A2, ..., [agg(expr)]
FROM     R1, R2, ...
WHERE    row-condition
GROUP BY grouping-attributes
HAVING   group-condition
ORDER BY attributes [DESC];
```

**Logical execution flow (PW §3.8.4 — the order GATE tests):**

```
   FROM  →  WHERE  →  GROUP BY  →  HAVING  →  SELECT  →  DISTINCT  →  ORDER BY
   (make     (filter     (partition)   (filter    (compute     (de-dup)   (sort;
   cross      rows)                     groups)    outputs)               last)
   product)
```

Consequences:

- `WHERE` sees **single rows**; `HAVING` sees **groups**. WHERE cannot contain aggregates; HAVING is applied *after* grouping.
- An alias defined in `SELECT` cannot be used in `WHERE` (WHERE runs first); it can be used in `ORDER BY` (runs after SELECT).
- `DISTINCT` is applied after SELECT expressions are computed.

**GROUP BY rules:**
1. Every attribute you keep *ungrouped* in the SELECT list must be a grouping attribute.
2. Aggregates (`COUNT`, `SUM`, `AVG`, `MIN`, `MAX`) are the only "extra" items allowed.
3. `GROUP BY A` + `SELECT A, AVG(c)` is fine; `SELECT B` when grouping by `A` is an error.
4. A bare non-grouped attribute in HAVING (`HAVING C > 60` while grouping by A) is an error — you may test aggregates and grouping attributes.

**The one-liner translation to RA (PW §3.8.2):**

```
   SELECT A1,...,An FROM R1,...,Rk WHERE P
   ⇔  π_{A1..An}( σ_P( R1 × R2 × ... × Rk ) )
```

---

## 5. NULLs: three-valued logic (top GATE trap source)

SQL conditions evaluate to **TRUE / FALSE / UNKNOWN**; only TRUE passes `WHERE`.

```
   NULL = NULL      → UNKNOWN   (not TRUE!)
   NULL <> NULL     → UNKNOWN
   NULL + 5         → NULL
   TRUE OR UNKNOWN  → TRUE       FALSE AND UNKNOWN → FALSE
   WHERE keeps only rows whose condition is TRUE  (UNKNOWN and FALSE are dropped)
```

**Aggregate behavior:**

| Construct | NULL behavior |
|---|---|
| `COUNT(*)` | counts **rows** (NULLs included) |
| `COUNT(col)` | counts **non-NULL** values of col |
| `SUM(col)`, `AVG(col)`, `MIN`, `MAX` | **ignore** NULLs (AVG = sum of non-nulls / count of non-nulls) |
| `SUM` of all-NULL group | returns NULL (not 0) |
| `DISTINCT col` | many NULLs collapse to **one** NULL (NULLs are duplicates of each other) |
| `WHERE col = NULL` | **never** selects anything — must use `col IS NULL` |

**The `NOT IN` trap (remember this exact fact):**

```
   x NOT IN (SELECT b FROM S)
   ≡  x <> ALL (SELECT b FROM S)
```

If the subquery returns **any NULL**, then for every `x` that is *not among the non-NULL values*, `x <> NULL` is UNKNOWN ⇒ the whole `NOT IN` is UNKNOWN ⇒ row filtered. For `x` equal to a listed value it is FALSE ⇒ filtered. Net effect:

> **If the subquery of `NOT IN` contains a NULL, `NOT IN` returns no rows at all.** (Empty subquery: `NOT IN` is TRUE for everything.)

`IN` vs `= ANY`: identical for single-attribute lists; they diverge only when a *row constructor* (multiple attributes) is involved.

---

## 6. Nested queries: uncorrelated vs correlated (PW §3.8.8–3.8.9)

```
   Uncorrelated: inner query uses nothing from outside → runs ONCE (bottom-up evaluation)
   Correlated:   inner query references the outer row → runs PER ROW (WHERE)
                 or PER GROUP (HAVING)
```

**The function zoo:**

| Construct | Tests | Equivalent form |
|---|---|---|
| `IN (subquery)` | membership | `= ANY (subquery)` |
| `NOT IN (subquery)` | non-membership | `<> ALL (subquery)` — NULL trap above |
| `> ANY (subquery)` | greater than **some** | `> (SELECT MIN(...))` pattern |
| `> ALL (subquery)` | greater than **every** | `> (SELECT MAX(...))` pattern |
| `EXISTS (subquery)` | subquery non-empty (TRUE/FALSE — never UNKNOWN) | — |
| `NOT EXISTS` | subquery empty | universal quantifiers |

**Canonical transformations GATE asks about:**

```sql
-- "female employees earning more than SOME male" (3 equivalent forms)
SELECT eid FROM Emp WHERE gen='F' AND sal > ANY (SELECT sal FROM Emp WHERE gen='M');
SELECT eid FROM Emp WHERE gen='F' AND sal > (SELECT MIN(sal) FROM Emp WHERE gen='M');

-- "more than EVERY male"
SELECT eid FROM Emp WHERE gen='F' AND sal > ALL (SELECT sal FROM Emp WHERE gen='M');
SELECT eid FROM Emp WHERE gen='F' AND sal > (SELECT MAX(sal) FROM Emp WHERE gen='M');
```

`EXISTS` never returns UNKNOWN (it is TRUE/FALSE), which is why `NOT EXISTS` is immune to the `NOT IN` NULL trap.

---

## 7. Set operations & union compatibility (PW §3.6)

**Union-compatible** ⇔ same **arity** AND corresponding attributes come from the same **domain**.

| Operation | Duplicates? | Note |
|---|---|---|
| `UNION` | removed (set) | max rows = n + m |
| `UNION ALL` | kept (bag) | rows = n + m exactly |
| `INTERSECT` | removed (set) | ≤ min(n, m) |
| `EXCEPT` | removed (set) | ≤ n; rows of first not in second |

(Strictly, `ALL` variants of INTERSECT/EXCEPT exist but GATE sticks to the set versions.)

---

## 8. Views (condensed — PW §3.8.7 `WITH` and standard view rules)

```
   View = named stored QUERY (virtual table)          Materialized view = physically stored result
```

- A simple view (single base table, no aggregates/DISTINCT/grouping) is **updatable**; joins, aggregates and DISTINCT generally make a view **non-updatable**.
- `WITH CHECK OPTION` rejects inserts/updates through the view that would produce rows not satisfying the view's `WHERE`.
- The `WITH` clause (CTE) is a *query-local* named subquery — recomputed each time it's referenced (in standard SQL), unlike a physically stored materialized view.

---

## 9. Worked examples (hand drills)

**Example A — evaluate on sight.** `R(A,B) = {(1,a), (2,b), (3,c)}`, `S(B,C) = {(a,x), (a,y), (c,z), (d,w)}`.

```
   |R ⋈ S|  : matches on B: a→2 rows, b→0, c→1  ⇒ 3
   |R ⟕ S|  : matched 3 + unmatched R-rows: (2,b) ⇒ 3 + 1 = 4
   |R ⟖ S|  : matched 3 + unmatched S-row (d,w) ⇒ 4      (symmetric here by luck)
   |R ⟗ S|  : matched 3 + 1 padded-R + 1 padded-S ⇒ 5
   π_C(R ⋈ S) : {x, y, z} ⇒ 3        (projection de-duplicates)
```

**Example B — division with a computed divisor.** Who took **every course that S7 took**?

```
   divisor  D = π_cid(σ_{sid=7}(Enroll))          ← compute first, then divide!
   answer   = π_sid(Enroll) ÷ D  (students, S7 need not be included unless stated)
```

**Example C — "more than all males" in SQL, 3 ways** (see §6). "More than some" = `> ANY` = `> MIN`; "more than all" = `> ALL` = `> MAX`. Over 90% of GATE's ANY/ALL questions are this pair.

**Example D — NULL arithmetic.** With `sal = NULL`:
`sal > 100` → UNKNOWN (row dropped); `sal IS NULL` → TRUE (row kept); `sal + 10` → NULL; `COALESCE(sal,0)` → 0 (not in core GATE, but handy).

---

## 10. Trap sheet (review before every mock)

| # | Trap | Fact |
|---|---|---|
| 1 | "Projection keeps duplicates?" | `π` always de-duplicates (RA is set-based) |
| 2 | "∩ and ⋈ are primitive?" | No — both derivable (`∩` from `−`; `⋈` from `×, σ, π`) |
| 3 | Division formula | `π_A(R) − π_A((π_A(R) × S) − R)` — inner subtraction order matters |
| 4 | Natural join, no common attrs | equals `R × S` |
| 5 | Left-outer-join minimum | ≥ `n` (each left row appears at least once) |
| 6 | `R − S` vs `π` | `π` cannot be pushed into `−`: `π_A(R−S) = π_A(R) − π_A(S)` is **FALSE** in general |
| 7 | SQL execution order | WHERE before GROUP BY/H**AVING**; SELECT before DISTINCT/ORDER BY |
| 8 | Aggregate in `WHERE` | illegal — use HAVING |
| 9 | `COUNT(*)` vs `COUNT(col)` | rows vs non-NULL values |
| 10 | `AVG` and NULLs | denominator counts only non-NULLs |
| 11 | `NOT IN` with NULL in subquery | returns **empty** result |
| 12 | `EXISTS` result | always TRUE/FALSE (never UNKNOWN) |
| 13 | `IN` ≡ `= ANY` | true for single-attribute; breaks for multi-attribute row comparisons |
| 14 | UNION vs UNION ALL on duplicates | set removes, ALL keeps |
| 15 | Correlated subquery cost | re-evaluated per row (WHERE) / per group (HAVING) |

---

# Practice set — DBMS Notes 01

*Same format as the banks: `Qn · type · difficulty`. All numeric answers verified by exact evaluation on the given data.*

### Q1 · NAT · Fundamental

`R(A, B) = {(1, 2), (2, 1), (3, 3), (4, 2), (5, 5)}`. The number of tuples in `π_B(R)` is ______.

### Q2 · NAT · Fundamental

`R(A, B)` has 15 tuples and `S(B, C)` has 20 tuples, with attribute-domains allowed to overlap arbitrarily. Let `M` and `m` be the maximum and minimum possible number of tuples in the natural join `R ⋈ S`. Then `M + m =` ______.

### Q3 · NAT · Standard

`R(A, B) = {(1, a), (2, b), (3, c)}` and `S(B, C) = {(a, x), (a, y), (c, z), (d, w)}` (B values are the letters shown). The number of tuples in the left outer join `R ⟕ S` is ______.

### Q4 · MCQ · Standard

Which one of the following identities holds for **all** relations `R`, `S` (union-compatible where required) and conditions `c`?

- (A) `σ_{A>5}(R ⋈ S) = σ_{A>5}(R) ⋈ S`, where `A` is an attribute of `R` that is not an attribute of `S`
- (B) `R ÷ S` can be expressed using only `σ`, `π`, `×` and `ρ` (without `∪` or `−`)
- (C) `σ_c(R × S) = σ_c(R) × σ_c(S)` for every condition `c`
- (D) `R − S = S − R` for all union-compatible `R` and `S`

### Q5 · NAT · Standard

```
Enroll(sid, cid):   s1: {c1, c2, c3}     s2: {c1, c2}
                    s3: {c1, c2, c3, c4} s7: {c1, c2}
```

The number of students **other than s7** who have enrolled in **every course that s7 has enrolled in** is ______.

### Q6 · MCQ · Fundamental

Which one of the following is the correct logical order of SQL clause evaluation (PW §3.8.4)?

- (A) FROM → WHERE → GROUP BY → HAVING → SELECT → DISTINCT → ORDER BY
- (B) FROM → GROUP BY → WHERE → HAVING → SELECT → DISTINCT → ORDER BY
- (C) FROM → WHERE → GROUP BY → SELECT → HAVING → DISTINCT → ORDER BY
- (D) FROM → WHERE → SELECT → GROUP BY → HAVING → ORDER BY → DISTINCT

### Q7 · NAT · Standard

```
Emp(eid, dno, sal):
  (1, 10, 100)  (2, 10, NULL)  (3, 20, 200)
  (4, 20, 300)  (5, 20, NULL)  (6, 30, 400)
```

The number of rows returned by

```sql
SELECT dno FROM Emp GROUP BY dno HAVING COUNT(sal) >= 2;
```

is ______.

### Q8 · MSQ · Standard

Which of the following statements about NULLs in SQL are TRUE?

- (A) `WHERE sal = NULL` selects no rows, even for rows where `sal IS NULL`
- (B) `COUNT(*)` counts rows whose columns contain NULLs
- (C) `AVG(sal)` ignores rows where `sal` is NULL (they do not enter numerator or denominator)
- (D) If the subquery of `x NOT IN (subquery)` returns any NULL, the expression evaluates to TRUE for values missing from the subquery

### Q9 · MCQ · Moderate

`Emp(eid, dno, sal)`. Which query correctly returns the `dno` of every department that has **more employees than department 10**?

- (A) `SELECT dno FROM Emp GROUP BY dno HAVING COUNT(*) > (SELECT COUNT(*) FROM Emp WHERE dno = 10);`
- (B) `SELECT dno FROM Emp GROUP BY dno HAVING COUNT(*) > (SELECT COUNT(*) FROM Emp GROUP BY dno);`
- (C) `SELECT dno FROM Emp WHERE COUNT(*) > (SELECT COUNT(*) FROM Emp WHERE dno = 10) GROUP BY dno;`
- (D) `SELECT dno FROM Emp GROUP BY dno HAVING COUNT(*) > COUNT(*);`

### Q10 · NAT · Killer

`Emp(eid, sal)` has 7 rows with salary values (in row order) `100, 200, 200, NULL, 300, 300, 300`. Let

- `a =` value returned by `SELECT COUNT(*) FROM Emp;`
- `b =` value returned by `SELECT COUNT(DISTINCT sal) FROM Emp;`
- `c =` value returned by `SELECT COUNT(sal) − COUNT(DISTINCT sal) FROM Emp;`

Then `a + b + c =` ______.

---

## Answer key (quick)

| Q | Ans | Q | Ans |
|---|---|---|---|
| 1 | **4** | 6 | **A** |
| 2 | **300** | 7 | **1** |
| 3 | **4** | 8 | **A, B, C** |
| 4 | **A** | 9 | **A** |
| 5 | **3** | 10 | **13** |

---

## Detailed solutions

**Q1.** `π_B(R)` keeps distinct B-values: `{2, 1, 3, 5}` → **4** tuples. (Note 2 appears twice in R but once in the projection.)

**Q2.** Maximum: all 15 R-rows share one B-value that also appears in all 20 S-rows → `15 × 20 = 300`. Minimum: disjoint B-domains → `0`. So `M + m = 300`.

**Q3.** Matches on B: `(1,a,x)`, `(1,a,y)`, `(3,c,z)` → 3 matched tuples. R-row `(2,b)` matches nothing → padded with NULL → 1 more. `|R ⟕ S| = 3 + 1 =` **4**. (The dangling S-row `(d,w)` would only matter for the *right*/full outer join.)

**Q4.** 
- (A) TRUE: since the condition uses only `R`'s attribute `A` (not shared with S), filtering R before or after the natural join gives the same result — matched rows preserve R's A-values, and unmatched R-rows are dropped in both orders.
- (B) FALSE: division needs set difference; `÷` was defined as derived exactly *because* it requires `−`.
- (C) FALSE: `σ_c(R × S)` can select rows where the condition spans both relations (or holds only after pairing); `σ_c(R) × σ_c(S)` tests each relation alone — e.g. condition `R.A = S.B` with a match across different rows.
- (D) FALSE (difference is not commutative).

**Q5.** First compute the divisor: `π_cid(σ_{sid=s7}(Enroll)) = {c1, c2}`. Students (other than s7) whose course sets contain both: 
- s1 `{c1,c2,c3}` ✔; s2 `{c1,c2}` ✔; s3 `{c1,c2,c3,c4}` ✔ → **3**. (s7 itself excluded by the wording.)

**Q6.** Logical evaluation: `FROM` builds the cross product, `WHERE` filters rows, `GROUP BY` partitions, `HAVING` filters groups, `SELECT` computes output expressions, `DISTINCT` de-duplicates, `ORDER BY` sorts last → **A**.

**Q7.** `COUNT(sal)` counts non-NULL salaries per department: dno 10 → 1, dno 20 → 2, dno 30 → 1. Only dno 20 satisfies `>= 2` → **1 row**.

**Q8.** 
- (A) TRUE — `sal = NULL` is UNKNOWN for every row; WHERE keeps only TRUE.
- (B) TRUE — `COUNT(*)` counts rows regardless of NULLs.
- (C) TRUE — all aggregate functions except `COUNT(*)` skip NULLs; `AVG` = (sum of non-NULLs)/(count of non-NULLs).
- (D) FALSE — it evaluates to UNKNOWN (so the row is filtered); the result set is *empty*, not TRUE-permissive.

**Q9.** 
- (A) correct: `GROUP BY dno`, groups compared against the scalar subquery (count of dept 10), which is evaluated once — returns exactly the departments with more employees.
- (B) the subquery returns *one row per department* → a scalar comparison with multiple rows is a runtime error.
- (C) aggregate `COUNT(*)` in `WHERE` is illegal (WHERE sees single rows, before grouping).
- (D) `COUNT(*) > COUNT(*)` is always FALSE → empty result.

**Q10.** `a = COUNT(*) = 7`. Distinct non-NULL salaries `{100, 200, 300}` → `b = 3`. `COUNT(sal) = 6` (one NULL skipped); `c = 6 − 3 = 3`. Total `a + b + c = 7 + 3 + 3 =` **13**.

---

*Cross-practice: `DB_03` (relational algebra counting/equivalences), `DB_05` (joins & aggregation), `DB_06` (subqueries, set ops, views). Next file: functional dependencies & normalization.*
