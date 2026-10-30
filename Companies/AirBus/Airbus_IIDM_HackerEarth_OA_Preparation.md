# Airbus Campus Internship 2027 — Digital for Corporate Analytics (IIDM)
## HackerEarth Online Assessment — Complete Preparation Document

**Candidate profile:** B.Tech IT, 2027 batch · strong C++/DSA (450+ LeetCode) · gaps to fix: Python-specific behaviour, SQL depth, analytics/statistics MCQ reasoning, pandas.

---

## 1. Cover Page and Assessment Facts

| Item | Detail | Status |
|---|---|---|
| Organization | Airbus Campus | Confirmed (invitation) |
| Role | Internship 2027 – Digital for Corporate Analytics (IIDM) | Confirmed |
| Platform | HackerEarth | Confirmed |
| Duration of actual assessment | **1 hour** | Confirmed |
| Assessment window | **Oct 27 2026, 6:00 PM IST → Oct 28 2026, 3:00 PM IST** | Confirmed |
| Planned attempt | Oct 28 2026, ~12:15 PM IST (deadline buffer ≈ 1 h 45 min) | Confirmed (your plan) |
| Practice-test setup (as shown to you) | 5 MCQs · 14 points total · 45 min · multiple attempts · no pause · proctoring disabled | Confirmed **for the practice test only** |
| Negative marking | Not disclosed. Assume **none**, but answer every question anyway | Assumption |

> **Timeline note (written for the plan above):** this document was prepared against the 27–28 Oct 2026 window. If your window has shifted, reuse the schedules in Part 14 relative to your new attempt time (T-minus framing already built in). Everything else in the document is date-independent.

**What "Digital for Corporate Analytics (IIDM)" plausibly tests** — an MCQ-style HackerEarth assessment (MCQs, possibly some coding) covering Python, SQL, data analytics/statistics, pandas-style data questions, and general programming logic. This document prepares for **all** of those areas; nothing here is a "confirmed syllabus".

---

## 2. Confirmed Information versus Assumptions

| Confirmed facts (invitation/practice setup) | Educated preparation priorities (NOT confirmed test contents) |
|---|---|
| Airbus Campus, HackerEarth, 1 hour actual duration | Python output tracing & control flow |
| Window: Oct 27 6:00 PM IST – Oct 28 3:00 PM IST | SQL joins, aggregation, NULLs, subqueries, window functions |
| Practice test showed: 5 MCQs, 14 points, 45 min, multiple attempts, no pause, no proctoring | Statistics / data interpretation (mean, median, variance, % change, joins of numbers) |
| No pause during an attempt; plan a quiet, uninterrupted hour | Pandas-level data questions (filter, groupby, merge, NaN) |
| | Programming logic / complexity MCQs |
| | Data-engineering vocabulary (ETL/ELT, batch vs streaming, Spark terms) |

**Deliberately excluded:** Airbus corporate history, guessed question counts, guessed cutoffs, alleged "Airbus previous-year questions". None of these exist in the public record and inventing them wastes your remaining hours.

---

## 3. One-Page Last-Minute Cheat Sheet

> Revise this entire page in 10–15 minutes right before the assessment. Everything below is verified Python 3 / standard-SQL behaviour.

### Python — the traps that decide marks

1. **Mutable default arguments** are created **once** at `def` time and persist: `def f(x, acc=[])` keeps appending across calls.
2. `is` = identity (same object); `==` = value equality. `x is y` for lists/bigger ints is effectively always False.
3. **`+=` on a list mutates in place** (aliases change too); `x = x + [1]` rebinds only the last name.
4. `[[]] * 3` gives **three references to the same inner list**. Use `[[] for _ in range(3)]`.
5. Shallow copy `b = a[:]` / `a.copy()` copies only the top level — inner lists still shared. `copy.deepcopy` copies everything.
6. Scalars: **str, int, float, bool, tuple are immutable**; **list, dict, set are mutable**.
7. `bool` is a subclass of `int`: `True + True == 2`, `isinstance(True, int) == True`, `{True, 1, 1.0}` collapses to `{True}`.
8. Dict/set keys: `True == 1 == 1.0` (same hash) — `{True:'a', 1:'b', 1.0:'c'}` → `{True: 'c'}`. `(1,)` is a tuple; `(1)` is int `1`.
9. Floor division rounds toward **−∞**: `7 // 2 == 3` but `-7 // 2 == -4`. Modulo follows sign of divisor: `7 % -3 == -2`, `-7 % 3 == 2`.
10. `round()` uses **banker's rounding** (to even): `round(2.5)==2, round(3.5)==4, round(0.5)==0`.
11. `0.1 + 0.2 != 0.3`; the value is `0.30000000000000004`.
12. `**` is **right-associative**: `2**3**2 == 512`; `-3**2 == -9` (unary minus binds AFTER pow).
13. Chained comparisons evaluate left→right with no double evaluation: `3 > 2 == 1` → `False` (because `2 == 1` fails); `1 < 2 < 3` → `True`.
14. Truthy: all non-zero / non-empty. `'' , 0, 0.0, None, [], {}, set()` → False. `'False'`, `0.5`, `[0]` → True.
15. `and` returns the first falsy operand (or last), `or` returns the first truthy operand (or last): `0 or 'x' → 'x'`, `0 and 5 → 0`, `3 and 5 → 5`.
16. Slices: `[start:stop:step]` — **stop excluded**, out-of-range never errors: `[1,2,3][5:] == []`. `s[::-1]` reverses; negative index −1 = last element.
17. `str`, `tuple` are immutable — `s[0]='x'` raises `TypeError`; `t=1` is int, `t=1,` is `(1,)`.
18. Naming: `b = a` is a reference, not a copy. `list(map(...))`, generators are **single-use**: after `list(g)`, `list(g)` is `[]`.
19. Loop `else` runs only when **no `break`** occurs. `try`/`finally`: `finally` always runs, and a `return` inside `finally` **overrides** the `try` return.
20. Exceptions that matter: `ValueError` (int('abc')), `KeyError` (missing dict key), `IndexError` (list too small), `ZeroDivisionError`, `TypeError`, `NameError` (undefined name), `AttributeError`.
21. Late binding: `[lambda: i for i in range(3)]` → every lambda sees final `i == 2`. Fix: `lambda i=i: i`.
22. Comprehension variable does **not** leak (Python 3): `n = 3; _ = [n for n in ['a']]` → `n` stays 3.
23. `13 % 4 == 1`, `13 // 4 == 3` (positive ×). Truncate toward zero is `int()`: `int(-3.9) == -3`.
24. `range(5)` → 0..4. `enumerate(iter, start=1)` labels from 1. `zip` stops at **shortest**.
25. `sorted(data, key=f)` orders by the key, returns original elements. Verified examples: `min([-10,-1,5,15], key=abs) → -1`, `sorted(['b','A','c'], key=str.lower) → ['A','b','c']`, `sorted([-10,-1,5,15], key=str) → [-1, -10, 15, 5]` (`'-1'` sorts before `'-10'`).
26. String * is repetition: `'ab' * 3 == 'ababab'`. `sep`/`end` control `print`.

### SQL — the traps that decide marks

- `COUNT(*)` counts rows; `COUNT(col)` ignores **NULL**; `COUNT(DISTINCT col)` also collapses duplicates.
- `WHERE x = NULL` matches **nothing** (NULL is UNKNOWN); use `IS NULL` / `IS NOT NULL`.
- `NOT IN (…, NULL)` returns **zero rows** unless something matches exactly.
- `WHERE` filters input rows; `HAVING` filters groups (can use aggregates). Logical order: FROM → JOIN → WHERE → GROUP BY → HAVING → SELECT → ORDER BY → LIMIT.
- Aggregates `SUM/AVG/MAX/MIN` ignore NULLs; `SUM` of no (NULL) rows = NULL while `COUNT` = 0.
- `INNER` keeps only matches; `LEFT` keeps all left rows (right side = NULL); `RIGHT` mirrored; `FULL OUTER` keeps both sides.
- `BETWEEN a AND b` is **inclusive** on both ends.
- `UNION` deduplicates (and sorts on some engines), `UNION ALL` keeps duplicates (faster).
- `GROUP BY` returns **one group for all NULLs** (one NULL group per column).
- Window functions: `ROW_NUMBER` = 1..n with ties broken arbitrarily; `RANK` ties share number, next jumps (`1,1,3`); `DENSE_RANK` ties share, no jump (`1,1,2`). `LAG(col)` previous row, `LEAD` next row, `SUM(x) OVER (ORDER BY …)` running total. **Window functions run after `WHERE`/`GROUP BY` — cannot be used inside `WHERE`.**
- Engine differences worth one line each: PostgreSQL `ASC` puts NULLs **last** (say `NULLS FIRST` to change); MySQL/SQLite put NULLs **first** in ASC. `LIMIT/OFFSET` — absent in SQL Server (use `TOP`/`OFFSET-FETCH`). String `'a' < 'b'`, `'A' < 'a'` is engine collation-dependent.
- Integer division: `7/2` → 3.5 in PostgreSQL/MySQL; in many engines `7/2` returns 3.5 too, but SQLite `7/2` → 3 (integer division). Know the engine note, don't guess.
- 2nd-highest salary: `SELECT MAX(salary) FROM emp WHERE salary < (SELECT MAX(salary) FROM emp)` or `DENSE_RANK() … ORDER BY salary DESC = 2`.
- First n per group: `ROW_NUMBER() OVER (PARTITION BY g ORDER BY sort) IN (subquery)`, cannot use directly in `WHERE`.

### Statistics / analytics — numbers worth remembering

- Mean/median/mode; median robust to outliers, mean is not. Even-length data → mean of the two middle values.
- Population variance divides by **n**; sample variance divides by **n−1** (Bessel). Sample SD of [2,4,6,8]: pop 2.24, sample 2.58.
- **% change = (new − old) / old × 100**. 40→50 is +25%, but 50→40 is −20% (not symmetric).
- **Percentage point** vs percent: 30%→35% is +5 pp (+16.7% relative).
- Weighted mean = Σ(value × weight)/Σ(weight).
- IQR outlier rule: outlier if value < Q1 − 1.5·IQR or > Q3 + 1.5·IQR.
- Correlation ≠ causation. Confounders, selection bias, survivorship bias.
- Two rates: rise-to-rise percentages only comparable through **base values**.
- Median split: percentiles position, not percentages of success.

### Pandas — 8 things

1. `df.loc[1:3]` label slice is **inclusive of 3**; `df.iloc[0:3]` positional excludes 3rd row.
2. Boolean masks need parentheses when combined: `df[(df.x > 2) & (df.y < 5)]`.
3. `df[mask]['col'] = v` silently does nothing (chained assignment). Use `df.loc[mask, 'col'] = v`.
4. `groupby('k')['v'].mean()` returns a Series indexed by k; `reset_index()` back to a DataFrame.
5. `merge` default `how='inner'`; `concat` default `axis=0`.
6. NaN: `sum/min/max` **skip** NaN; all-NaN `sum()` → 0, all-NaN `mean()` → NaN. `dropna()` default drops rows with **any** NaN.
7. `duplicated()` marks later duplicates True; `drop_duplicates()` keeps the first.
8. `value_counts()` counts unique values of a **Series**, sorted by count descending.

### Complexity quick table

`dict/set` lookup/insert ~O(1) avg · `list` index O(1) / append O(1) / insert(0) & `in` O(n) · `sort` O(n log n) · `counter` O(n) · binary search O(log n) · merge sorted lists O(n+m) · nested loop O(n²) · recursion fib(n) O(2ⁿ) · heap push/pop O(log n).

### Test-taking tactics (memorize)

- Negative marking is not disclosed; still answer **every** question.
- Flag ambiguous ones and return; never lose >3 min on one MCQ on the first pass.
- **Trace code on paper line by line**; after each assignment, note the variable(s) it changed or created. For shared objects (lists/dicts) track the *object*, not the name.
- For SQL MCQs, **write the mini-table and join rows in your head before reading the options**.
- If a question looks "too easy", re-read — HackerEarth often hides a negation ("Which is NOT correct").
- Manage the 60 minutes: do easy questions first, hard ones at the 2-minute mark; keep ⏳ sight of the timer.

---

## 4. Module A — High-Yield Python Revision (Output Tracing)

> Every code block below was executed under Python 3.13 and the output shown is the **exact** output. Trace rules: track *objects* (not names), evaluate expressions right-to-left where precedence demands, and never assume "it probably works like C++".

### 4.1 Numbers: `//`, `%`, `/`, `**`, precedence

**Concept.** `/` is always true division (float result). `//` floors toward −∞. `%` inherits the divisor's sign (`a % m = a - m * floor(a/m)`). `**` is right-associative; unary minus binds *less* tightly than `**`.

```python
print(7//2, -7//2, 7%-3, -7%3, 10/5, 10/4)
print(2**3**2, -3**2, (-3)**2)
print(10 - 2*3 % 4, (10-2)*3 % 4)
```
**Output**
```
3 -4 -2 2 2.0 2.5
512 -9 9
8 0
```
**Trace (line 1).** `7//2=3`; `-7//2` → floor(−3.5) = −4; `7%-3` → 7 − (−3)·floor(7/−3)= 7 −(−3)(−3)= 7−9 = −2; `-7%3` → −7 − 3·floor(−7/3) = −7 − 3(−3) = 2; `10/5 = 2.0`; `10/4 = 2.5`.

**Misconception.** "-7//2 = -3" (C-style truncation). Python floors, so −4. And `2*3 % 4` evaluates left-to-right: `(2*3) % 4 = 2`.

### 4.2 Immutable vs mutable, references and aliasing

**Concept.** Assignment never copies an object; it binds a name to an object. Mutating a shared mutable object (list/dict/set) is visible through every alias.

```python
a = [1, 2]
b = a          # alias, same object
b += [3]       # in-place mutation
print(a)
b = b + [4]    # rebinds b to a NEW list
print(a, b)

x = [1, 2, 3]
y = [1, 2, 3]
print(x == y, x is y, sum(x) == sum(y))
```
**Output**
```
[1, 2, 3]
[1, 2, 3] [1, 2, 3, 4]
True False True
```
**Trace.** `a is b` → both point to one list → `[1,2,3]`. After `b = b + [4]`, `a` keeps 3 elements; `b` has 4. `==` compares values (True), `is` compares identity (False).

**Misconception.** "`+=` is just sugar for `x = x + y`". For mutable objects it is NOT — it mutates in place (`__iadd__`).

### 4.3 Shallow vs deep copy

**Concept.** `a[:]`, `list(a)`, `a.copy()`, `dict.copy()` are **shallow**: one new container, inner objects shared. `copy.deepcopy` copies nested objects too.

```python
import copy
a = [[1, 2], [3, 4]]
b = a[:]              # shallow
b[0].append(9)
print(a)
c = copy.deepcopy(a)
c[1][0] = 0
print(a, c)
```
**Output**
```
[[1, 2, 9], [3, 4]]
[[1, 2, 9], [3, 4]] [[1, 2, 9], [0, 4]]
```
**Trace.** `b` is a new outer list; `b[0]` is the *same* inner `[1,2]` as `a[0]`, so the append shows in `a`. `c` is fully independent, so `c[1][0]=0` does not touch `a`.

**Misconception.** "`copy()` protects me from any later mutation" — only for scalar elements; nested mutables are still shared.

### 4.4 Multiplication of lists and shared rows

```python
r = [[]] * 3
r[0].append(1)
print(r)

m = [[0]*2]*2
m[0][1] = 5
print(m)

grid = [[0]*2 for _ in range(2)]   # correct idiom
grid[0][1] = 5
print(grid)
```
**Output**
```
[[1], [1], [1]]
[[0, 5], [0, 5]]
[[0, 5], [0, 0]]
```
**Trace.** `[[]]*3` repeats the *same* list object 3 times; `[0]*2` (integers) is safe — ints are immutable, so reassigning `m[0][1]` replaces only a slot of the inner list... but *both* outer rows share that inner list, hence `[[0,5],[0,5]]`.

**Misconception.** "`[0]*n` and `[[0]*k]*n` behave the same" — they don't once mutation is involved.

### 4.5 Dict/set keys, `True==1`, hashing

**Concept.** `True == 1 == 1.0` and they have equal hashes → dict/set treats them as the *same key*; the **last inserted value** wins, the first key object is kept.

```python
d = {True: 'a', 1: 'b', 1.0: 'c'}
print(d)
print({0, False, 1, True}, len({1, 1.0, 2}))
dn = {1: 'a', 1.0: 'b'}
print(list(dn.keys()))
```
**Output**
```
{True: 'c'}
{0, 1} 2
[1]
```
**Trace.** Three keys collapse into one slot → `{True: 'c'}`. In the set, `False↔0` and `True↔1` collapse. Key display keeps the *first* key object (`1`), value becomes `'b'`.

**Misconception.** "Dicts distinguish `True` and `1`" — they don't; same hash and equality.

### 4.6 Slicing and negative indexing

**Concept.** `seq[start:stop:step]`: `start` included, `stop` excluded, negative indices count from the end, out-of-range never raises.

```python
s = 'abcdef'
print(s[1:5:2], s[-1], s[::-1], s[-4:-2])

L = [1, 2, 3]
print(L[5:], L[1:99], L[-10:2], L[-1])

print('abc'[::-1] == list(reversed('abc')))   # str vs list!!
t = 'COMPANION'
print(t[1:6:2], t[:-4], t[::-1][:3])
```
**Output**
```
bd f fedcba cd
[] [2, 3] [1, 2] 3
False
OPN COMPA NOI
```
**Trace (first line).** indices 1,3 → 'b','d'. `s[-4:-2]` → 'c','d'. `t[1:6:2]` → indices 1,3,5 → 'O','P','N'. `abc` reversed (str) ≠ list `['a','b','c']` → False.

**Misconception.** "Slicing out-of-range raises IndexError" — only direct indexing `L[5]` raises; slicing clamps.

### 4.7 Comprehensions and generators

**Concept.** List comprehension is eager and leaks no loop variable in Python 3. Generator expressions are lazy and single-use. Comprehension bodies execute at iteration time — side effects are visible.

```python
a = [1, 2, 3]
b = [a.pop() for _ in range(2)]
print(b, a)

i = 'outer'
_ = [i for i in range(2)]
print(i)

g = (x*2 for x in range(3))
print(next(g))
print(list(g))
print(list(g))

m = map(lambda v: v*v, [1,2,3])
print(list(m))
print(list(m))

g2 = (n for n in range(10) if n % 3 == 0)
print(next(g2), sum(g2))
```
**Output**
```
[3, 2] [1]
outer
0
[2, 4]
[]
[1, 4, 9]
[]
0 18
```
**Trace.** Comprehension pops 3 then 2 → `b=[3,2]`, `a=[1]`. The leaked-name check: `i` stays `'outer'` (Python 3 scoping). Generators are consumed: second `list(g)` is `[]`. After `next(g2)` consumes 0, `sum(g)` adds 3+6+9=18.

**Misconception.** "You can iterate a generator twice" — no; build a `list`/`tuple` if you need repeats.

### 4.8 Tuples, unpacking, `divmod`, starred assignment

```python
a, *b = [1, 2, 3]
c, d = divmod(7, 2)
print(a, b, c, d)
t = 1
print(type(t).__name__)
t = 1,
print(t, type(t).__name__)
x, y = 3, 4
x, y = y, x + 1
print(x, y)
```
**Output**
```
1 [2, 3] 3 1
int
(1,) tuple
4 4
```
**Trace.** `b` collects the rest `[2,3]`. `(1)` is just int 1; `(1,)` is a 1-tuple. `x, y = y, x+1` evaluates the whole right side FIRST with the old values → (4, 3+1) = (4,4).

**Misconception.** "Writing `(1)` creates a tuple" — only the comma makes the tuple. And "tuple assignment is sequential" — it is simultaneous; the right side is built first.

### 4.9 Truthiness, `and`/`or` chains, ternary, chained comparisons

```python
print(bool(''), bool('False'), bool(0.0), bool(None), bool(-1), bool([0]))
print('A' if 2>1 else 'B', 0 or 'x', [] and 'y', 'a' and 'b' or 'z', not 1==2)
print(2 or 0, (0 or 2) and 0, bool([]) or 3, 1 and 2 or 20)
print(3>2==1, 1<2<3, (1==1)==1, 5==5==5)
```
**Output**
```
False True False False True True
A x [] b True
2 0 3 2
False True True True
```
**Trace.** `[0]` is a non-empty list → truthy. `0 or 'x'` → first truthy operand `'x'`; `[] and 'y'` → first falsy operand `[]`. `3>2==1` is `(3>2) and (2==1)` → False. `1 and 2 or 20` → `(1 and 2)=2` then `2 or 20` → 2.

**Misconception.** "`and`/`or` return booleans" — they return one of the operands.

### 4.10 Rounding and float representation

```python
print(round(2.5), round(3.5), round(-2.5), round(2.675, 2))
print(round(0.5), round(1.5), round(-0.5), round(4.5))
print(0.1 + 0.2, 0.1 + 0.2 == 0.3)
print(int(3.99), int(-3.9), int('7'))
```
**Output**
```
2 4 -2 2.67
0 2 0 4
0.30000000000000004 False
3 -3 7
```
**Trace.** Banker's rounding: 2.5→2 (even), 3.5→4 (even), 0.5→0, −0.5→0(=-0 → prints 0). `round(2.675, 2)` → 2.67 because 2.675 is stored as slightly less than 2.675. `int()` truncates toward zero.

**Misconception.** "round(2.5)=3 like school math."

### 4.11 Strings and immutability

```python
s = 'ab'
for ch in s:
    s = s + ch.upper()
print(s)
print('ab'*3, ''.join(['h','i']), '-'.join('abc'))
print(max('apple','banana'), min('zebra','apple'))
print(sorted('dbac'), ''.join(sorted('dbac')))
print(f"{3.14159:.2f}", f"{7:04d}", f"{7:>4}")
```
**Output**
```
abAB
ababab hi a-b-c
banana apple
['a', 'b', 'c', 'd'] abcd
3.14 0007    7
```
*(last line: `7` is right-aligned in width 4 — three leading spaces, no quotes)*

**Trace.** Each `s = s + ch` creates a **new** string; loop over the *original* `s` (2 iterations) → `abAB`. Uppercase of 'a' sorted after lowercase? Here 'A' < 'a' — but with `''.join(sorted(...))` we sorted chars 'd','b','a','c' → 'a','b','c','d'.

**Misconception.** "Chaining `s = s + x` inside a loop that iterates over `s` adds characters mid-iteration" — no; the iteration collection is fixed at loop start.

### 4.12 Functions: mutable defaults, scope, global

```python
def f(x, acc=[]):
    acc.append(x)
    return acc
print(f(1), f(2), f(1, []))

calls = 0
def g(k, cache={}):
    global calls
    if k not in cache:
        cache[k] = k*k
        calls += 1
    return cache[k]
print(g(3), g(3), g(4), calls)

def g2(k, seen=set()):
    seen.add(k)
    return len(seen)
print(g2(1), g2(2), g2(3), g2(1))

n = 3
def h1():
    n = 6               # local: no effect
h1()
print(n)
def h2():
    global n
    n = n * 2
h2()
print(n)
```
**Output**
```
[1, 2] [1, 2] [1]
9 9 16 2
1 2 3 3
3
6
```
**Trace.** The `def` builds `acc`/`cache`/`seen` **once**; calls without that argument reuse it. `g(4)` executes counting once → calls==2 (3 once, 4 once; second `g(3)` hits cache). `g2(1)` repeated: 1 already in set → len stays 3. Local `n = 6` in `h1` doesn't touch the global.

**Misconception.** "Default arguments are re-created every call" / "assignment inside a function always changes the global" — both wrong.

### 4.13 Lambdas, map/filter/zip/enumerate/sorted

```python
fs = [lambda: i for i in range(3)]
print([f() for f in fs], [f() for f in fs][0])
gs = [lambda i=i: i for i in range(3)]
print([g() for g in gs])
print(list(zip('ab', [1,2,3])), list(enumerate('ab', 1)))
a = [10, 20, 30, 40]
b = [a.pop() for _ in range(len(a))]
print(b, a)
print(sorted([3, 111, 2], key=str))
print(max([(-3,1), (-2,5)]), max([(1,'b'), (1,'a')]))
```
**Output**
```
[2, 2, 2] 2
[0, 1, 2]
[('a', 1), ('b', 2)] [(1, 'a'), (2, 'b')]
[40, 30, 20, 10] []
[111, 2, 3]
(-2, 5) (1, 'b')
```
**Trace.** Lambdas capture the *variable* `i`, not its value → after the comprehension `i==2`. Default-arg fix captures per-iteration values. `zip` ends at the shortest input. `key=str` compares strings: `'111' < '2' < '3'`. Tuples compare element-by-element.

**Misconception.** "The lambda inside the comprehension remembers each `i` as it was" — it sees the *final* `i`, unless you bind it via a default argument.

### 4.14 Loops: break/continue/else, while

```python
for i in range(3):
    if i == 1:
        break
else:
    print('no break')
print('done', i)

for j in range(2, 6):
    if j % 3 == 0:
        break
else:
    print('none matched', j)
print('last', j)

i = 0; total = 0
while i < 5:
    if i == 2:
        i += 1
        continue
    total += i
    i += 1
print(total, i)
```
**Output**
```
done 1
last 3
8 5
```
**Trace.** First loop breaks at 1 → `else` skipped. Second breaks at 3 → else skipped. While: i=2 skipped by `continue`; total = 0+1+0+3+4 = 8; ends at i=5.

**Misconception.** "for/else runs when the loop *breaks*" — the opposite; else belongs to the no-break case.

### 4.15 Exceptions: matching types, finally, return-in-finally

```python
for code in ["int('abc')", "{}['k']", "[][5]", "1/0", "undefined_name"]:
    try:
        exec(code, {})
    except ValueError:
        print('ValueError')
    except KeyError:
        print('KeyError')
    except IndexError:
        print('IndexError')
    except ZeroDivisionError:
        print('ZeroDivisionError')
    except Exception:
        print('Other')

def f():
    try:
        return 'try'
    finally:
        print('finally runs')
print(f())

def g():
    try:
        return 1
    finally:
        return 2
print(g())

def h():
    try:
        return 1/0
    except ZeroDivisionError:
        return 'zero'
    finally:
        return 'finally'
print(h())
```
**Output**
```
ValueError
KeyError
IndexError
ZeroDivisionError
Other
finally runs
try
2
finally
```
**Trace.** First matching `except` wins; `undefined_name` is a NameError → caught by generic `Exception`. `f()` runs `finally` after computing the return → prints then returns `'try'`. In `g()`/`h()` the `finally` return **overrides** any earlier return (1/0 → ZeroDivisionError → except returns 'zero' → finally replaces it with 'finally').

**Misconception.** "return in try can't be overridden" — a return (or exception) in `finally` wins.

### 4.16 print(), f-strings, sep/end

```python
print(1, 2, 3, sep='*', end='!')
print('next')
print(*[1,2,3], sep='|')
print(*divmod(29, 4))
print(f"{10/4:.2f}")
```
**Output**
```
1*2*3!next
1|2|3
7 1
2.50
```
**Note.** `print(x)` adds a space between arguments by default (sep=' '); `end` defaults to `'\n'`; `f"{v:.2f}"` rounds half-to-even at the string level.

### 4.17 Python operation complexity (memorize)

| Operation | Cost | Note |
|---|---|---|
| `x in list` | O(n) | `in set/dict` → O(1) avg |
| `list.append` | O(1) amortized | `insert(0, v)` → O(n) |
| `list[i]` | O(1) | slicing k elements → O(k) |
| `list.index/count/remove` | O(n) | |
| `sorted(l)` | O(n log n) | `heapq` push/pop O(log n) |
| `dict.get/setdefault` | O(1) avg | worst-case rare |
| string `+` in loop | O(n²) total | use `''.join` |
| building set/Counter | O(n) | |

**High-yield Python summary:** mutable defaults · aliasing with `+=` · `True==1` collapsing keys · banker's rounding · floor division toward −∞ · `is` vs `==` · slice clamping · generator single-use · late-binding lambdas · `finally` return wins · loop-else meaning · chain rule `3>2==1` → False.

### 4.18 Reference semantics inside a function: `+=` vs reassignment by type

```python
def f1(L):
    L += [9]        # mutates caller's list
def f2(L):
    L = L + [9]     # rebinds a local name
def f3(s):
    s += 'x'        # str is immutable: rebinds a local name
def f4(d):
    d['x'] = 1      # mutates caller's dict
def f5(L):
    L.append(7)     # mutates caller's list
a = [1]
f1(a); print(a)
b = [1]
f2(b); print(b)
c = 'a'
f3(c); print(c)
dd = {}
f4(dd); print(dd)
e = []
f5(e); print(e)
```
**Output**
```
[1, 9]
[1]
a
{'x': 1}
[7]
```
**Trace.** For mutable objects, `+=` and `.append` mutate the object the caller sees; `L = L + [9]` and immutable `s += 'x'` only rebind the local name.

**Misconception.** "`+=` is always equivalent to assignment" — for lists/dicts it is `__iadd__` (in-place).

### 4.19 Dictionary essentials: get, setdefault, iterating, update

```python
d = {}
print(d.get('x'), d.get('x', 0))
d.setdefault('k', []).append(1)
print(d)
counts = {}
for w in 'to be or not to be'.split():
    counts[w] = counts.get(w, 0) + 1
print(counts, sum(counts.values()), len(counts))
d2 = {'a': 1, 'b': 2}
for k in list(d2):
    d2[k] = d2[k] * 10
print(d2)
print(sorted({'b': 1, 'a': 2}))   # sorted over dict -> sorted keys
```
**Output**
```
None 0
{'k': [1]}
{'to': 2, 'be': 2, 'or': 1, 'not': 1} 6 4
{'a': 10, 'b': 20}
['a', 'b']
```
**Trace.** `.get` returns None/default without adding keys; `setdefault` inserts then uses. Word count via `get(w,0)+1`. Iterating a *copy* (`list(d2)`) while writing same keys is safe; adding NEW keys during iteration is not.

**Misconception.** "`for k in d:` iterates (key, value) pairs" — it iterates keys; use `d.items()` for pairs.

### 4.20 Sets and their operators

```python
A = {1, 2, 3}
B = {2, 3, 4}
print(A | B, A & B, A - B, B - A, A ^ B)
print(len(A) , 3 in A, A.add(4), sorted(A), len({1,2}, ))
```
**Output**
```
{1, 2, 3, 4} {2, 3} {1} {4} {1, 4}
3 True None [1, 2, 3, 4] 2
```
**Trace.** Union/intersection/symmetric difference (`^` = elements in exactly one set); difference is not symmetric. `add()` returns None (mutates). Note `sorted(A)` is evaluated *after* `A.add(4)`, so it shows 4 items; the final `2` is the extra `len({1,2})`. Sets have no order guarantee beyond hash-based display.

**Misconception.** "`A - B` and `B - A` are the same" — they are opposite directions.

### 4.21 `sort()` vs `sorted()`: the None trap & sort stability

```python
L = [3, 1, 2]
res = L.sort()
print(res)
print(L)
M = [(1, 'b'), (1, 'a'), (2, 'a')]
print(sorted(M))                 # stable: equal keys keep insertion order
N = ['b', 'A', 'c']
print(sorted(N, key=str.lower), sorted(N, reverse=True))
nums = [-10, -1, 5, 15]
print(sorted(nums, key=abs), min(nums, key=abs), max(nums, key=abs), sorted(nums, key=str))
```
**Output**
```
None
[1, 2, 3]
[(1, 'a'), (1, 'b'), (2, 'a')]
['A', 'b', 'c'] ['c', 'b', 'A']
[-1, 5, -10, 15] -1 15 [-1, -10, 15, 5]
```
**Trace.** `L.sort()` mutates and returns None (classic MCQ). `sorted` returns a new list. Stability: equal-key elements keep original order. `key=abs` → order |1| < |5| < |10| < |15| → −1, 5, −10, 15 (original signs kept). `key=str` → strings `'-1'`, `'-10'`, `'15'`, `'5'` → `'-1' < '-10' < '15' < '5'`.

**Misconception.** "`list.sort()` can be chained: `L.sort().append(x)`" — sort() returns None → AttributeError.

### 4.22 Slicing produces independent copies — but only one level

```python
xs = [1, 2, 3]
ys = xs[:]
ys[0] = 99
print(xs, ys)

xss = [[1, 2], [3, 4]]
yss = xss[:]
yss[1][0] = 42
print(xss)                 # inner list shared!

ss = 'abcdef'
print(ss[1:4], ss[4:1], ss[1:4][::-1])
```
**Output**
```
[1, 2, 3] [99, 2, 3]
[[1, 2], [42, 4]]
bcd  dcb
```
**Trace.** Top level: independent. Inner lists: shared → the mutation leaks into `xss` (its second inner list becomes `[42, 4]`). `ss[4:1]` with default step +1 is empty, so the printed line has two spaces between `bcd` and `dcb`.

**Misconception.** "Slicing = deep copy."

### 4.23 `any`/`all`, `sum`, list membership, iterators

```python
print(any([0, '', None]), all([1, 'x', 2]), all([1, '', 2]))
print(sum(range(5)), sum([1, 2, 3], 10))       # start value
print([1, 2, 2, 3].count(2), [1, 2, 2].index(2))
r = range(3)
print(list(r), 2 in r, len(r))
```
**Output**
```
False True False
10 16
2 1
[0, 1, 2] True 3
```
**Trace.** `any` → True if **≥1** truthy (all falsy → False); `all` → True only if **0 falsy** (the `''` makes it False). `sum(it, start)` adds the start. `index` finds the FIRST occurrence.

**Misconception.** "sum(list, 5) means add 5 last / positional args to sum are separate data" — it's the start value.

### 4.24 Common errors → exception table

| Code | Raises |
|---|---|
| `int('abc')` | `ValueError` |
| `int('3.5')` | `ValueError` |
| `"a" + 1` | `TypeError` |
| `[1,2][5]` | `IndexError` |
| `d['missingkey']` | `KeyError` |
| `1/0`, `1//0`, `1%0` | `ZeroDivisionError` |
| `x + "y"` where x undefined | `NameError` |
| `s[0] = 'a'` (str) / `t[0] = 1` (tuple) | `TypeError` |
| `None.x()` / missing method | `AttributeError` |
| `max([])`, `min([])` | `ValueError` |
| `list.pop()` on empty | `IndexError` |
| `dict.pop(k)` missing without default | `KeyError` |
| function `del s[0]` on str | `TypeError` |
| `list.remove(value)` missing | `ValueError` |

**Inline MCQs for Module A (answers right below):**

1. `print(len({1, 1.5, 1.0}))` → `2`? **No:** `1.0` and `1` collapse (equal, same hash); `1.5` and `1.0` are distinct → `2` items: `{1, 1.5}` → the print shows `{1, 1.5}`; `len` = **2**.
2. `print((3, 1) < (3, 2), [1, 2] < [1, 2, 3], 'ab' > 'aa')` → both prefix-extended comparisons: **True True True** (lexicographic; shorter equal prefix is smaller).
3. `print(5 == 5.0)` → **True** (numeric equality: int vs float equal in value).

**High-yield recap of 4.18–4.24:** mutation by `+=`/append; `.get` vs `[]`; set ops pair asymmetry; `sort()` returns None; slice-copy is one level deep; `any/all` semantics; `sum(x, start)`.

---

## 5. Module B — High-Yield SQL Revision

### 5.1 The sample schema (used all through this module)

```sql
CREATE TABLE departments (
  dept_id    INTEGER PRIMARY KEY,
  dept_name  TEXT,
  location   TEXT
);

CREATE TABLE employees (
  emp_id     INTEGER PRIMARY KEY,
  name       TEXT,
  dept_id    INTEGER,     -- may be NULL; no FK constraint assumed
  salary     INTEGER,     -- may be NULL
  hire_date  TEXT,        -- YYYY-MM-DD
  manager_id INTEGER      -- may be NULL
);

CREATE TABLE customers (
  cust_id INTEGER PRIMARY KEY,
  name    TEXT,
  city    TEXT
);

CREATE TABLE orders (
  order_id   INTEGER PRIMARY KEY,
  cust_id    INTEGER,   -- may be NULL; 99 is an orphan (no such customer)
  amount     INTEGER,   -- may be NULL
  order_date TEXT,
  status     TEXT       -- Delivered / Cancelled / Pending
);
```

**departments**

| dept_id | dept_name | location |
|---|---|---|
| 1 | Engineering | Mumbai |
| 2 | Sales | Delhi |
| 3 | HR | Mumbai |
| 4 | Marketing | Bengaluru |
| 5 | Finance | NULL |

**employees**

| emp_id | name | dept_id | salary | hire_date | manager_id |
|---|---|---|---|---|---|
| 1 | Ankit | 1 | 90000 | 2021-01-15 | NULL |
| 2 | Bhavna | 1 | 85000 | 2022-03-01 | 1 |
| 3 | Chetan | 2 | 55000 | 2020-07-19 | NULL |
| 4 | Divya | 2 | 45000 | 2023-01-10 | 3 |
| 5 | Esha | NULL | 60000 | 2019-11-05 | NULL |
| 6 | Farah | 3 | 52000 | 2022-10-21 | NULL |
| 7 | Kiran | NULL | NULL | 2023-02-02 | 1 |
| 8 | Harsh | 1 | 90000 | 2018-09-30 | 1 |
| 9 | Isha | NULL | 40000 | 2024-06-17 | NULL |
| 10 | Jay | 4 | 78000 | 2025-02-01 | NULL |

**customers**

| cust_id | name | city |
|---|---|---|
| 1 | Meera | Delhi |
| 2 | Rohan | Delhi |
| 3 | Sneha | Mumbai |
| 4 | Arjun | Chennai |
| 5 | Kiran | Pune |

**orders**

| order_id | cust_id | amount | order_date | status |
|---|---|---|---|---|
| 101 | 1 | 1200 | 2024-01-05 | Delivered |
| 102 | 1 | 700 | 2024-01-20 | Cancelled |
| 103 | 2 | 300 | 2024-02-11 | Delivered |
| 104 | 3 | NULL | 2024-03-01 | Pending |
| 105 | 3 | 450 | 2024-03-15 | Delivered |
| 106 | NULL | 900 | 2024-04-02 | Delivered |
| 107 | 2 | 300 | 2024-04-10 | Delivered |
| 108 | 99 | 500 | 2024-04-25 | Delivered |
| 109 | 4 | 1500 | 2024-05-05 | Cancelled |
| 110 | 2 | 600 | 2024-06-01 | Delivered |

> All queries below were executed against exactly these tables (SQLite 3.51, standard SQL). Engine-only differences are flagged inline.

### 5.2 SELECT / DISTINCT / LIMIT / ORDER BY (+ NULL ordering)

```sql
SELECT DISTINCT status FROM orders;                    -- → Cancelled, Delivered, Pending (3 rows)
SELECT DISTINCT city FROM customers;                   -- → 4 rows (Delhi, Mumbai, Chennai, Pune)
SELECT COUNT(*) FROM (SELECT DISTINCT status FROM orders);   -- → 3
SELECT name, salary FROM employees ORDER BY salary ASC LIMIT 3;
```
Last query output (this engine):
```
Kiran | (NULL)      -- ASC puts NULLs FIRST in SQLite/MySQL
Isha  | 40000
Divya | 45000
```
`ORDER BY salary DESC LIMIT 3` → `Ankit 90000, Harsh 90000, Bhavna 85000` (NULLs LAST in DESC here).

**Engine note.** PostgreSQL: NULLs are **largest** → NULLs last in ASC, first in DESC. SQLite/MySQL: NULLs smallest → first in ASC, last in DESC. Control explicitly with `NULLS FIRST/LAST` (PostgreSQL; not in MySQL).

**Tie order is not guaranteed without a tiebreaker** — standard MCQ trap when two rows share the sort key.

### 5.3 WHERE, BETWEEN, IN, LIKE — with the NULL traps

```sql
SELECT COUNT(*) FROM employees WHERE salary BETWEEN 45000 AND 60000;   -- → 4 (inclusive both ends)
SELECT COUNT(*) FROM employees WHERE salary IN (90000, 40000);         -- → 3 (Ankit, Harsh, Isha)
SELECT COUNT(*) FROM employees WHERE salary NOT IN (90000, NULL);      -- → 0 !!
SELECT COUNT(*) FROM employees WHERE name LIKE 'A%';                   -- → 1 (Ankit)
SELECT COUNT(*) FROM customers WHERE city = NULL;                      -- → 0 (never TRUE)
SELECT COUNT(*) FROM customers WHERE city IS NULL;                     -- → 0 here, but the correct NULL filter
SELECT 1 IN (NULL), 2 IN (1, 3, NULL), 2 IN (2, NULL);                 -- → NULL, NULL, 1
```

**Three-valued logic.** Every comparison with NULL is UNKNOWN, not FALSE. `WHERE` keeps a row only when the predicate is TRUE. So: `= NULL` never matches; `NOT IN (…, NULL)` returns rows only if an exact match exists, else zero rows; `x IN (a, NULL)` behaves as `x=a OR x IS NULL`… precisely: TRUE if x=a, else UNKNOWN.

### 5.4 Aggregates with NULL — COUNT variants and empty-set behaviour

```sql
SELECT COUNT(*), COUNT(amount), COUNT(DISTINCT amount) FROM orders;
SELECT SUM(amount), AVG(amount), MAX(amount), MIN(amount) FROM orders;
SELECT SUM(amount), COUNT(amount) FROM orders WHERE 1 = 0;
```
**Verified output**
```
10 | 9 | 8
6450 | 716.67 | 1500 | 300
(NULL) | 0
```
- `COUNT(*)` = 10 rows; `COUNT(amount)` = 9 (skips order 104's NULL); `COUNT(DISTINCT amount)` = 8 distinct non-NULL values (300 appears twice).
- `AVG(amount)` = 6450 / 9 = **716.67** (SUM skips NULLs too).
- Aggregates over an **empty set**: `COUNT` → 0, others → NULL.

**Misconception.** "AVG includes NULL rows as zero" — aggregates ignore NULLs entirely.

### 5.5 GROUP BY / HAVING — and the single NULL group

```sql
SELECT dept_id, COUNT(*) FROM employees GROUP BY dept_id ORDER BY dept_id;
```
```
(NULL) | 3          ← ALL NULL-dept employees form ONE group
1 | 3
2 | 2
3 | 1
4 | 1
```
```sql
SELECT c.city, COUNT(*) AS n
FROM orders o JOIN customers c ON o.cust_id = c.cust_id
WHERE o.status = 'Delivered'
GROUP BY c.city
HAVING COUNT(*) >= 2;
-- → Delhi | 4    (Delivered orders to Delhi customers; Mumbai has only 1 -> filtered out)
```

- `WHERE` filters rows before grouping; `HAVING` filters after aggregation.
- Logical order: FROM → JOIN → WHERE → GROUP BY → HAVING → SELECT → ORDER BY → LIMIT.
- `GROUP BY` makes **one group containing all NULLs** of that column.

### 5.6 JOIN types — exact row counts and the anti-join lesson

```sql
-- inner: only matches (employees with a valid dept)
SELECT COUNT(*) FROM employees e JOIN departments d ON e.dept_id = d.dept_id;   -- 7

SELECT COUNT(*) FROM employees e LEFT JOIN departments d ON e.dept_id = d.dept_id; -- 10 (all employees)
SELECT COUNT(*) FROM departments d LEFT JOIN employees e ON e.dept_id = d.dept_id; -- 8 (Finance gets one NULL row)
SELECT COUNT(*) FROM employees e RIGHT JOIN departments d ON e.dept_id = d.dept_id;-- 8 (same as above)
SELECT COUNT(*) FROM employees e FULL OUTER JOIN departments d ON e.dept_id = d.dept_id; -- 11
SELECT COUNT(*) FROM employees CROSS JOIN departments;                              -- 50
SELECT COUNT(*) FROM orders o JOIN customers c ON o.cust_id = c.cust_id;            -- 8
```
Why 7/10/8/11? Employees with NULL dept (Esha, Kiran, Isha) drop from the inner join; in a LEFT join from employees they survive with `dept_name = NULL`; Finance has no employees → a NULL-extended row; FULL keeps both sets → 10 + 1 = 11. Orders: 106 (no customer) and 108 (customer 99) drop from an inner order-customer join.

**Trap: `COUNT(*)` vs `COUNT(col)` after a LEFT JOIN.**

```sql
SELECT c.cust_id, c.name, COUNT(o.order_id) AS n_orders, COALESCE(SUM(o.amount), 0) AS total
FROM customers c LEFT JOIN orders o ON c.cust_id = o.cust_id
GROUP BY c.cust_id, c.name
ORDER BY c.cust_id;
```
```
1 | Meera | 2 | 1900
2 | Rohan | 3 | 1200
3 | Sneha | 2 | 450    ← SUM skips the NULL amount of order 104
4 | Arjun | 1 | 1500
5 | Kiran | 0 | 0      ← COUNT(o.order_id)=0 for the NULL-extended row; COUNT(*) would have said 1!
```
- Customer with no orders (Kiran) still appears exactly once in a LEFT join — but `COUNT(*)` = 1 there, so construct "no orders" filters with `o.order_id IS NULL` (anti-join) or `COUNT(o.id) = 0`.

**FULL OUTER demo (who survives):**
```
(no emp) | Finance        ← department with no employees
Esha     | (null dept)
Kiran    | (null dept)
Isha     | (null dept)
```

### 5.7 SELF JOIN — manager names

```sql
SELECT e.name AS emp, m.name AS mgr
FROM employees e LEFT JOIN employees m ON e.manager_id = m.emp_id;
```
Verified pairs: `Bhavna→Ankit`, `Kiran→Ankit`, `Harsh→Ankit`, `Divya→Chetan`; all others show NULL managers. Two table aliases over the same table; NULL `manager_id` never matches (NULL = NULL is UNKNOWN).

### 5.8 Subqueries & correlated subqueries

```sql
-- employees earning > their own department's average
SELECT e.name, e.salary
FROM employees e
WHERE e.salary > (SELECT AVG(e2.salary) FROM employees e2 WHERE e2.dept_id = e.dept_id);
```
→ **Ankit (90000 vs 88333.33), Chetan (55000 vs 50000), Harsh (90000 vs 88333.33)**. A correlated subquery runs once per outer row and references the outer alias `e`.

Second-highest salary (classic):
```sql
SELECT MAX(salary) FROM employees WHERE salary < (SELECT MAX(salary) FROM employees);
-- → 85000    (DENSE_RANK version: WHERE dr = 2)
```

### 5.9 CTEs and the deduplication pattern

```sql
WITH t AS (
  SELECT name, dept_id, salary,
         ROW_NUMBER() OVER (PARTITION BY dept_id ORDER BY salary DESC, emp_id) AS rn
  FROM employees
  WHERE dept_id IS NOT NULL)
SELECT name, dept_id, salary FROM t WHERE rn <= 2
ORDER BY dept_id, salary DESC;
```
→ dept 1: Ankit, Harsh (both 90000 — the `emp_id` tiebreaker makes it deterministic), dept 2: Chetan, Divya, dept 3: Farah, dept 4: Jay.

Deduplication pattern (duplicate rows across the data columns; keep one):
```sql
-- Step 1: see the duplicates (keep the smallest id per (a, b))
WITH w AS (
  SELECT id, a, b,
         ROW_NUMBER() OVER (PARTITION BY a, b ORDER BY id) AS rn
  FROM dup_rows)
SELECT * FROM w WHERE rn = 1;          -- the rows to KEEP, delete the rest

-- Step 2: delete as a plain correlated statement (engines differ on self-reference;
-- on MySQL wrap the subquery in a derived table, e.g. NOT IN (SELECT MIN(id) FROM (SELECT ...) x GROUP BY a, b))
DELETE FROM dup_rows
WHERE id NOT IN (SELECT MIN(id) FROM dup_rows GROUP BY a, b);
```

### 5.10 Window functions — ranks, LAG/LEAD, running totals

```sql
SELECT name, salary,
       RANK()        OVER (ORDER BY salary DESC) AS rk,
       DENSE_RANK()  OVER (ORDER BY salary DESC) AS drk,
       ROW_NUMBER()  OVER (ORDER BY salary DESC) AS rn
FROM employees ORDER BY rn;
```
```
Ankit  90000 | 1 | 1 | 1
Harsh  90000 | 1 | 1 | 2
Bhavna 85000 | 3 | 2 | 3     ← RANK skips to 3; DENSE_RANK is 2
Jay    78000 | 4 | 3 | 4
Esha   60000 | 5 | 4 | 5
Chetan 55000 | 6 | 5 | 6
Farah  52000 | 7 | 6 | 7
Divya  45000 | 8 | 7 | 8
Isha   40000 | 9 | 8 | 9
Kiran  (NULL)|10 | 9 | 10    ← NULL = smallest here -> last in DESC (engine note!)
```

```sql
-- running total (skips NULL amounts because WHERE removed order 104)
SELECT order_id, amount, SUM(amount) OVER (ORDER BY order_id) AS rt
FROM orders WHERE amount IS NOT NULL;
-- 101:1200, 102:1900, 103:2200, 105:2650, 106:3550, 107:3850, 108:4350, 109:5850, 110:6450

-- previous order amount per customer (LAG)
SELECT cust_id, order_id, amount,
       LAG(amount) OVER (PARTITION BY cust_id ORDER BY order_id) AS prev
FROM orders WHERE cust_id IS NOT NULL;
-- Meera (cust 1): 101 prev=NULL, 102 prev=1200
-- Rohan  (cust 2): 103 prev=NULL, 107 prev=300, 110 prev=300
-- Sneha  (cust 3): 104 prev=NULL, 105 prev=NULL (LAG sees the NULL of 104 as a real row)
-- Arjun  (cust 4): 109 prev=NULL   | orphan cust 99: 108 prev=NULL (kept by WHERE cust_id IS NOT NULL? yes! 99 is not NULL)
```
Note LAG sees the NULL of 104, so order 105's `prev` is NULL too.
- Windows are computed **after** WHERE/GROUP BY → window functions can't appear in `WHERE` (use subquery/CTE + outer filter: `WHERE rn = 2`).
- `ROW_NUMBER` breaks ties arbitrarily (or with a tiebreaker); `RANK`/`DENSE_RANK` don't.

### 5.11 CASE — and the swallowed NULL

```sql
SELECT order_id,
       CASE WHEN amount >= 1200 THEN 'High'
            WHEN amount >= 500 THEN 'Medium'
            ELSE 'Low' END AS bucket
FROM orders ORDER BY order_id;
```
```
101 High | 102 Medium | 103 Low | 104 Low | 105 Low | 106 Medium
107 Low  | 108 Medium | 109 High | 110 Medium
```
Order 104 (amount NULL) becomes **'Low'** — NULL fails every `WHEN` (UNKNOWN ≠ TRUE), falling to ELSE. Guard with `WHEN amount IS NULL THEN 'Unknown'` FIRST when needed. `CASE` evaluates conditions in order — order your ranges from most specific.

### 5.12 UNION vs UNION ALL vs INTERSECT

```sql
SELECT COUNT(*) FROM (SELECT name FROM employees UNION ALL SELECT name FROM customers); -- 15
SELECT COUNT(*) FROM (SELECT name FROM employees UNION SELECT name FROM customers);     -- 14
SELECT COUNT(*) FROM (SELECT name FROM employees INTERSECT SELECT name FROM customers); -- 1
```
Why 15 → 14: employee 7 and customer 5 are both "Kiran" → UNION collapses the duplicate. `UNION ALL` keeps duplicates and is the faster default for count-style aggregation.

### 5.13 Keys, constraints, normalization, indexes — one-liners that get asked

- **Primary key** = unique + NOT NULL; enforces entity integrity. A table can have at most one PK; it may cover multiple columns (composite).
- **Foreign key** = column referencing another table's PK/unique enforces referential integrity; NULL FKs are allowed (employee without dept).
- **Candidate key** any unique minimal identifier; **surrogate** = artificial ID.
- 1NF: atomic values, no repeating groups. 2NF: 1NF + no partial dependency (non-key attribute depending on only part of a composite key). 3NF: 2NF + no transitive dependency (`A → B → C`). BCNF: every determinant is a candidate key. Higher normal forms reduce update anomalies but may need more joins.
- Index = separate ordered structure (commonly B-tree) enabling O(log n) lookups; speeds `WHERE` on indexed col, range scans, ORDER BY with matching order; costs write time/storage; **low-selectivity columns (e.g., 'status' with 3 values) rarely benefit**; `LIKE '%x'` cannot use a normal B-tree index; functions on the column defeat the index unless a functional index exists.
- `JOIN` performance: index both join keys; avoid `SELECT *`.

### 5.14 Engine-difference cheat table (know these, be safe)

| Feature | PostgreSQL | MySQL | SQL Server | Oracle |
|---|---|---|---|---|
| ASC NULL order | last | first | first | last |
| LIMIT | `LIMIT n` | `LIMIT n` | `TOP n` / `OFFSET … FETCH` | `FETCH FIRST n ROWS` |
| `''` vs NULL | distinct | distinct | distinct | NULL **is** `''`! |
| unquoted compare `'A'='a'` | depends collation | often case-insensitive (utf8mb4_*ci) | case-insensitive by collation | usually case-sensitive or per-config |
| `FULL OUTER JOIN` | yes | no (emulate with LEFT+UNION RIGHT) | yes | yes |
| window functions | yes (8.x+) | yes (8.0+) | yes | yes |
| `7/2` integer division result | 3.5 (numeric) | 3.5 | 3 (int!) | 3 |

**Module B recap:** joins drop/include rows in the patterns above · COUNT(*) vs COUNT(col) · NULL logic (IS NULL, NOT IN with NULL → empty) · WHERE vs HAVING order · UNION vs UNION ALL · RANK vs DENSE_RANK on ties · window-after-WHERE · NULLs as one GROUP BY group · CASE ELSE catches NULLs · tiebreakers for determinism.

---

## 6. Module C — Statistics and Data Analytics

### 6.1 Descriptive statistics — worked example

Data: `4, 8, 6, 5, 3, 9, 5` (n = 7)

| Statistic | Calculation | Result |
|---|---|---|
| Mean | (4+8+6+5+3+9+5) / 7 = 40 / 7 | **≈ 5.71** |
| Median | sorted: 3,4,**5**,5,6,8,9 → middle | **5** |
| Mode | most frequent value | **5** (appears twice) |
| Range | max − min = 9 − 3 | **6** |

**Even n:** `21, 34, 12, 9, 27, 15` → sorted `9,12,15,21,27,34` → median = (15+21)/2 = **18**.

**Misconception.** "Mode is always unique" — data can be bimodal or have no repeated value at all. "Median needs sorting description" — remember it's the 50th percentile, robust to outliers.

### 6.2 Variance & SD — population vs sample

Data: `2, 4, 6, 8` → mean = 5. Squared deviations: 9, 1, 1, 9 → sum = 20.

| Measure | Formula | Result |
|---|---|---|
| Population variance σ² | Σ(x−μ)² / n = 20/4 | **5** (σ ≈ 2.24) |
| Sample variance s² | Σ(x−x̄)² / (n−1) = 20/3 | **6.67** (s ≈ 2.58) |

- `n−1` (Bessel's correction) corrects for estimating the mean from the sample → sample variance is always ≥ population variance for the same data.
- SD has the data's units; variance is squared units.

### 6.3 Percentages: change, points, ratios

- **% change** = (new − old) / old × 100. Sales 40 → 50 = **+25%**; 50 → 40 = **−20%** (not symmetric!).
- **Percentage point (pp)** = absolute difference of two percentages: 30% → 35% = **+5 pp** = **+16.7% relative**.
- Ratio: 120 : 80 = 3 : 2 = 1.5. Doubling is +100%.
- "Increase by a factor of 1.5" = ×1.5 = +50% (≠ +150%).
- A % of a %: 20% increase on 30% = 36% (30 × 1.2), NOT 50%.

**Worked weighted average.** Scores 80 (weight 30%), 90 (weight 70%) → 0.3×80 + 0.7×90 = 24 + 63 = **87**. Credits: grade 60 × 3 credits, 80 × 2 credits → (180+160)/5 = **68** — never the simple mean (70).

### 6.4 Quartiles, percentiles, outliers

- Q1 = 25th percentile, Q2 = median, Q3 = 75th percentile. **IQR = Q3 − Q1.**
- Outlier fences (Tukey): below `Q1 − 1.5·IQR` or above `Q3 + 1.5·IQR`; extreme if beyond 3×.
- Example: Q1 = 25, Q3 = 45 → IQR = 20 → fences = 25 − 30 = **−5** and 45 + 30 = **75**; value 80 is an outlier, value 8 is not (it's within; low values below −5 would be).
- Percentile rank of a value: % of observations strictly below (+ half of equal).
- ⚠️ Quartile *values* depend on interpolation convention (numpy "linear" vs the median-of-halves/Tukey school grades differ on small datasets). MCQs usually give Q1/Q3 or use big clean sets; if a question compares conventions, trust the *stated* convention.

### 6.5 Probability and conditional probability

- `P(A or B) = P(A) + P(B) − P(A and B)` (addition rule; subtract double-counted intersection).
- Independent: `P(A and B) = P(A)·P(B)`.
- **Conditional:** `P(A|B) = P(A and B) / P(B)`.

Worked example (Bayes-style, exam favourite):
- 100 modules: 60 by Team-P (defect rate: 6 of 60 defective), 40 by Team-Q (8 of 40 defective).
- `P(defect) = 14/100 = 0.14`
- `P(P | defect) = 6/14 = 3/7 ≈ 0.429` ← not 0.6! Base rates matter.
- `P(defect | P) = 0.10` vs `P(defect | Q) = 0.20` → Q riskier despite P making more modules.

Dice/cards quickies: two dice sum to 7 in exactly 6 ways: (1,6),(6,1),(2,5),(5,2),(3,4),(4,3). `P(first die even | sum = 7)` = 3 of those 6 → **1/2**.

### 6.6 Correlation, causation, confounding

- Correlation (−1..1) measures linear association only; near-zero ≠ no relationship (non-linear possible).
- Correlation ≠ causation: possible confounder (population growth "causes" both API traffic and hires), reverse causality, selection bias, coincidence (spurious with small n).
- Simpson's paradox mini: the overall dashboard shows Product A converts 5% vs B's 3%, yet within every platform A is worse — the aggregate is confounded by traffic mix. Always segment suspicious aggregates.
- Practical rule for OA: when options include "correlation does not imply causation", it is usually the intended statistical-soundness answer.

### 6.7 Sampling, bias, missing data

| Bias | One-line meaning |
|---|---|
| Selection bias | sample drawn in a way that over-represents some group (web-only survey) |
| Survivorship bias | only "still alive" units observed (only current users rated churn) |
| Non-response bias | who answers differs systematically from who doesn't |
| Voluntary response | only motivated parties respond |

- Missing data patterns: random-ish (solve with deletion/imputation by column mean/median), vs systematic missing (must NOT just drop — it distorts; e.g., income missing more often for low-income). Missingness itself can be a signal.
- Random sampling gives unbiased estimates; larger n reduces variance ∝ 1/√n. Median imputation is more robust than mean with outliers.

### 6.8 Data cleaning & validation — what analysts actually do

1. Parse types (dates, numbers-as-text "1,200"), 2. handle NULLs/distinct from '' or 0, 3. de-duplicate (full-row and key-level), 4. validate ranges/domains (negative sales? dates in future? country not in list), 5. referential integrity (orphan cust_id = 99), 6. unit/currency consistency (INR vs USD mixed), 7. standardize text (case, spaces 'Mumbai '), 8. document decisions.
- Data validation rule examples: `0 ≤ amount ≤ 10_000_000`, `status ∈ {Delivered, Cancelled, Pending}`, `order_date ≤ today`, unique `(cust_id, order_id)`.
- Data quality dimensions: **accuracy, completeness, consistency, timeliness, uniqueness, validity**.

### 6.9 Business metrics you must compute fast

| Metric | Formula | Example |
|---|---|---|
| Conversion rate | conversions / visitors | 50 / 2000 = **2.5%** |
| Retention (cohort) | users active in month n+1 / active in month n | 600/1000 = **60%** |
| Churn | 1 − retention | **40%** |
| MoM growth | (this − last)/last | 100 → 110 → 121 = **10% per month** |
| CAGR | (V_end/V_start)^(1/n) − 1 | 100 → 121 in 2 yrs → (1.21)^(1/2)−1 = **10%** |
| AOV | revenue / orders | ₹1,20,000 / 100 = ₹1,200 |
| Share of sales | segment / total | 300/1200 = 25% |
| Run rate | period average × periods | first-quarter ₹30L total → ₹1.2Cr/yr |

Interpretation traps: percentage changes stacked on different bases; averages hide distribution (average salary rising while median falls); rate vs absolute count ("5% rise of a small number may be < 1% rise of a big number"); "growth slowed" ≠ "growth negative".

### 6.10 Choosing visualizations

| Question | Chart |
|---|---|
| Compare categories | bar chart |
| Trend over time | line chart |
| Part-to-whole (few slices) | pie/donut (≤5 slices) or 100% stacked bar |
| Distribution | histogram / box plot |
| Two-variable relationship | scatter |
| Composition over time | stacked area / line |
| Correlation matrix | heatmap |
| Process / funnel | funnel chart |

Table-vs-chart rule: exact values → table; pattern/comparison → chart. Log scale for very skewed spans; sorted bars beat alphabetical bars for ranking; stacked bars hard to compare middle segments.

### 6.11 Interpreting summarised data — classic MCQ shapers

- Mean ≠ typical when skewed; median + IQR describe skewed data better.
- Overlapping distributions can still have very different medians; identical mean & SD can hide totally different shapes (that's the point of box plots).
- Percentages require knowing the base ("sales up 20%" of WHICH number).
- Aggregates hide outliers (a couple of huge orders can make a segment "look" great).
- Time series: compare like for like (same month next year handles seasonality); MoM vs YoY confusion is a favourite distractor.
- Rounding percentages can exceed 100% when components rounded independently (25% + 30% + 45.5% of categories).

**Module C recap:** anti-symmetric % change · pp vs % · Bessel n−1 · Tukey 1.5·IQR fences · conditional probability with base rates · "correlation ≠ causation" · median for skew · conversion/retention formulas · segment the aggregate.

---

## 7. Module D — Pandas and Data Manipulation

> All results below were produced with pandas 2.3.3. DataFrames print with integer index unless reindexed (e.g., `set_index`).

### 7.1 Building blocks and the two indexers

```python
import pandas as pd
df = pd.DataFrame({'id': [1,2,3,4],
                   'dept': ['A','B','A','C'],
                   'sal': [10,20,30,40]})

# loc = LABEL-based (inclusive at the stop), iloc = POSITION-based (exclusive)
s = df.set_index('id')
print(s.loc[1:3])        # ids 1..3 -> THREE rows
print(df.iloc[0:2])      # rows 0..1 -> TWO rows
print(df.loc[df.sal > 15, 'dept'].tolist())
```
Output:
```
   dept  sal
id
1     A   10
2     B   20
3     A   30
   id dept  sal
0   1    A   10
1   2    B   20
['B', 'A', 'C']
```
**Misconception.** "`df.loc[1:3]` behaves like Python slicing" — with a **label** index it *includes* the stop label.

### 7.2 Boolean filtering — parentheses are mandatory

```python
df = pd.DataFrame({'dept':['A','A','B','B','C','C','A'],
                   'sales':[10,30,50,70,110,10,90],
                   'q':[1,2,1,2,1,None,2]})
mask = (df.sales > 40) & (df.q == 1)
print(df[mask])
```
Output:
```
  dept  sales    q
2    B     50  1.0
4    C    110  1.0
```
- `|`, `&`, `~` replace `or/and/not`; without parentheses `&` binds **tighter** than `>`, so `df.sales > 40 & df.q == 1` is invalid (`40 & df.q` evaluated first) → errors like `TypeError`.
- `.query('sales > 40 and q == 1')` is the string alternative; NaN rows drop under `.query()` conditions (NaN ≠ NaN semantics).

### 7.3 Missing values — skipna, fillna, dropna

```python
import numpy as np
dfx = pd.DataFrame({'a':[1,np.nan,3], 'b':[np.nan,np.nan,np.nan]})
print(dfx.isnull().sum().tolist())       # NaN count per column: [1, 3]
print(dfx['a'].sum(), dfx['b'].sum(), dfx['b'].mean())   # 4.0  0.0  NaN
print(len(dfx.dropna()), len(dfx.dropna(how='all')))     # 0  2
print(dfx.fillna(0).to_numpy().tolist())
```
Output:
```
[1, 3]
4.0 0.0 nan
0 2
[[1.0, 0.0], [0.0, 0.0], [3.0, 0.0]]
```
- All-NaN column `.sum()` → 0.0 but `.mean()` → NaN (0/0). "skipna" default True.
- `dropna()` default `how='any'` → row dropped if ANY cell NaN; `how='all'` only fully-NaN rows.
- `fillna(method='ffill')` is superseded by `.ffill()`; forward-fills over rows/columns; use `inplace=True` or reassign to keep.

### 7.4 Duplicates

```python
dd = pd.DataFrame({'a':[1,1,2,2,2,3]})
print(dd.duplicated().tolist())          # [False, True, False, True, True, False]
print(len(dd.drop_duplicates()))         # 3
print(dd['a'].unique().tolist())         # [1, 2, 3]
```
- `duplicated(keep='first')` default; `keep='last'` flips which becomes False; `keep=False` marks ALL dups True.
- Key-level dupes: `df.drop_duplicates(subset=['cust','date'])`.

### 7.5 groupby — aggregation and reset_index

```python
df = pd.DataFrame({'dept':['A','A','B','B','C'], 'sales':[10,30,50,70,20]})
g = df.groupby('dept')['sales'].mean()
print(g['B'], g.max(), len(g), g.min())
print(df.groupby('dept', as_index=False)['sales'].sum())
```
Output:
```
60.0 60.0 3 20.0
  dept  sales
0    A     40
1    B    120
2    C     20
```
- Result Series is indexed by the group keys — compare with `.loc['B']` not positional.
- Multiple aggs: `df.groupby('dept').agg(total=('sales','sum'), n=('sales','size'))`.
- `agg` on NaN: `sum/min/max/mean` skip NaNs (see 7.3); `count()` counts non-NaN, `size()` counts rows.

### 7.6 merge / join / concat

```python
a = pd.DataFrame({'id':[1,2,3], 'v':[10,20,30]})
b = pd.DataFrame({'id':[2,3,4], 'w':[100,200,300]})
print(pd.merge(a, b))                    # default how='inner'
print(pd.merge(a, b, how='left'))
m = pd.merge(a, b, how='outer')
print(m.isnull().sum().tolist())

left  = pd.DataFrame({'k':[1,2,3], 'v':[10,20,30]})
right = pd.DataFrame({'k':[3,4], 'w':[7,8]})
ml = pd.merge(left, right, how='left')
print(len(ml), int(ml['w'].isnull().sum()))   # 3 2
```
Output:
```
   id   v    w
0   2  20  100
1   3  30  200
   id   v      w
0   1  10    NaN
1   2  20  100.0
2   3  30  200.0
[0, 1, 1]
3 2
```
- Same-name key column: merged automatically on `id`; differing names → `left_on=…, right_on=…`.
- Overlapping non-key columns get `_x`/`_y` suffixes.
- `join()` is index-based shorthand (defaults LEFT). `concat` glues DataFrames: default `axis=0` rows, keeps original indices (duplicates!) → use `ignore_index=True`.

### 7.7 Sorting, ranking, top-n

```python
r = pd.Series([100, 200, 300, 40, 60])
print(r.rank().tolist())                 # default method='average'
ss = pd.DataFrame({'x':[1,2,1,2], 'y':[9,8,7,6]})
print(ss.sort_values(['x','y'], ascending=[True,False]).index.tolist())
print(ss.x.rank().tolist())              # ties: two 1s -> (1+2)/2 = 1.5 each; two 2s -> (3+4)/2 = 3.5
```
Output:
```
[3.0, 4.0, 5.0, 1.0, 2.0]
[0, 2, 1, 3]
[1.5, 3.5, 1.5, 3.5]
```
**Trace of the sort.** Rows `(x,y)` = (1,9) idx0, (2,8) idx1, (1,7) idx2, (2,6) idx3 → x ascending: group x=1 then x=2; within group y **descending**: (1,9), (1,7), (2,8), (2,6) → indices `[0, 2, 1, 3]`.
- `rank(method='min')` → ties all get smallest rank (like RANK in SQL); `'dense'` like DENSE_RANK; `'average'` is default.
- `nlargest(3,'col')` / `nsmallest`; stable `sort_values(kind='mergesort')`.

### 7.8 Datetimes

```python
ddate = pd.DataFrame({'d': pd.to_datetime(['2024-01-05','2024-03-20','2024-02-11'])})
print(ddate['d'].dt.month.tolist())
print(ddate['d'].dt.strftime('%Y-%m').tolist())
print(ddate.sort_values('d')['d'].dt.day.tolist())
```
Output:
```
[1, 3, 2]
['2024-01', '2024-03', '2024-02']
[5, 11, 20]
```
- Accessors live on `.dt` only after `to_datetime`; `pd.to_datetime(mixed, errors='coerce')` → NaT for garbage strings (safe alternative to exceptions).
- Month arithmetic: Period types or offset months; comparing years: `.dt.year == 2024`.

### 7.9 Strings (`.str`) and categorical counts

```python
s = pd.Series(['Xplore','apex','Era','alpha'])
print(s.str.startswith('a').tolist())
print(s[s.str.len() == 4].tolist())
vc = pd.Series(['A','B','A','C','A'])
print(vc.value_counts().index.tolist())   # verified: ties resolve by first appearance
```
Output:
```
[False, True, False, True]
['apex']
['A', 'B', 'C']
```
(`value_counts` sorts descending by count: A=3 first; B and C both 1 → B before C by first appearance — an implementation detail, don't bet an MCQ on it.)

### 7.10 The chained-assignment trap (top MCQ material)

```python
df2 = df.copy()
df2[df2.sales > 40]['sales'] = 0        # NO effect (+ SettingWithCopyWarning)
print(df2['sales'].tolist())
df2.loc[df2.sales > 40, 'sales'] = 0
print(df2['sales'].tolist())
```
Output (df = the 7-row frame from 7.2):
```
[10, 30, 50, 70, 110, 10, 90]          ← unchanged! row 5 (sales 10) never matched anyway
[10, 30,  0,  0,  0, 10,  0]           ← correct way: everything >  zeroed; the 10 stays
```

### 7.11 Vectorization vs loops & a clean mini workflow

```python
df['rev'] = df['sales'] * 3                       # vectorized, fast
df['band'] = df['sales'].apply(lambda v: 'Big' if v > 90 else 'Normal')
total_big = df.loc[df['band'] == 'Big', 'sales'].sum()
```
- Vectorized ops make no Python-level loops → ~10-100× faster than `.apply`/`iterrows`; `iterrows()` returns (index, Series) copies — never mutate data through it.
- Typical cleaning: read `pd.read_csv('x.csv')` → inspect `df.head()`, `df.info()`, `df.describe()` → fix dtypes (`astype`) → handle NaN → drop dupes → filter/type-cast → group/aggregate.

**Module D recap:** loc inclusive / iloc exclusive · `( )` around & | · loc[mask, col] to assign · groupby Series key-index · merge default inner · concat default axis 0 with kept indices · NaN sums skip → 0 on all-NaN · duplicated keep='first' · `.dt` for datetimes · `.str` for strings.

---

## 8. Module E — Programming Logic and Complexity (Concise)

### 8.1 Complexity table you must answer instantly

| Topic | Best | Average | Worst | Notes |
|---|---|---|---|---|
| Linear/binary search | O(1) | O(log n) | O(n) / O(log n) | binary needs sorted data |
| Bubble/insertion/selection sort | O(n²) | O(n²) | O(n²) | bubble best O(n) only with early exit |
| Merge sort / heapsort | O(n log n) | O(n log n) | O(n log n) | merge needs O(n) extra |
| Quicksort | O(n log n) | O(n log n) | O(n²) | adversarial input |
| Hashing (dict/set) | O(1) | O(1) | O(n) | collisions |
| Heap insert/pop | O(1)/O(log n) | O(log n) | O(log n) | |
| Two nested independent loops | — | — | — | O(n²); inner `range(i, n)` → n(n+1)/2 iterations |
| Recursion without memo (fib) | — | — | O(2ⁿ) | ~2 calls, depth n |
| Memoized DP fib | O(1) | O(n) | O(n) | |

### 8.2 Loop-count tracing (favourite MCQ territory)

```python
count = 0
for i in range(5):
    for j in range(i, 5):
        count += 1
print(count)          # 5+4+3+2+1 = 15

count = 0
i = 1
while i < 100:
    i *= 2             # 1,2,4,...,128 -> 7 iterations
    count += 1
print(count, i)
```
**Output:** `15` then `7 128`. Rule: multiplicative loops are O(log n); triangular iterated loops are O(n²) with count n(n+1)/2.

### 8.3 Binary search mechanics

```python
a = [1, 2, 3, 4, 5, 6, 7, 8]   # 0-indexed
lo, hi, target, seen = 0, len(a) - 1, 3, 0
while lo <= hi:
    mid = (lo + hi) // 2
    seen += 1
    if a[mid] == target: break
    elif a[mid] < target: lo = mid + 1
    else: hi = mid - 1
print(seen, a[mid])
```
**Output:** `3 3` — iterations: mid index 3 (value 4) → hi=2 → mid index 1 (2) → lo=2 → mid index 2 (3) found. 3 iterations to search 8 elements = ⌈log₂ 8⌉ on this path.
Off-by-one rules: `hi = len-1` with `lo <= hi` is the "inclusive" convention; forgetting `-1` or using `<` carelessly is the classic bug.
**Misconception.** "`mid` must be `(lo+hi+1)//2` to avoid cycles" — only in some "move right" variants; with `lo/hi = mid±1` the plain floor mid terminates correctly.

### 8.4 Prefix sums & frequency counting

```python
a = [3, 1, 4, 1, 5]
pre = [0]
for v in a:
    pre.append(pre[-1] + v)
print(pre)                      # [0, 3, 4, 8, 9, 14]
print(pre[3] - pre[1])          # sum a[1..2] = 1 + 4 = 5

cnt = {}
for v in a:
    cnt[v] = cnt.get(v, 0) + 1
print(cnt)
print(max(cnt.items(), key=lambda kv: kv[1]))   # (1, 2) — value 1 occurs twice
```
**Output:** `[0, 3, 4, 8, 9, 14]`, `5`, `{3: 1, 1: 2, 4: 1, 5: 1}`, `(1, 2)`.
Note `1` occurs twice → it wins the max-count vote. If several values tie, `max` returns the FIRST one encountered in iteration order; count-based questions without a stated tie rule should avoid ties.

### 8.5 Two pointers reasoning (sorted data)

```python
a = [2, 7, 11, 15]; target = 9
lo, hi, steps, pair = 0, len(a) - 1, 0, None
while lo < hi:
    s = a[lo] + a[hi]
    steps += 1
    if s == target: pair = (a[lo], a[hi]); break
    elif s < target: lo += 1
    else: hi -= 1
print(pair, steps)
```
**Output:** `(2, 7) 3` — sums tried: 17 (>9 → hi--), 13 (>9 → hi--), 9 (found). O(n) for a sorted array vs O(n²) naive.

### 8.6 Recursion patterns

```python
def fib(n):
    return n if n < 2 else fib(n-1) + fib(n-2)
print(fib(5))   # 5

def calls(n):
    return 1 if n < 2 else 1 + calls(n-1) + calls(n-2)
print(calls(5))  # 15
```
**Output:** `5`, `15`. Non-memoized tree calls grow exponentially; memoization → linear.
Recursion depth: Python default limit ~1000 (`sys.getrecursionlimit()`), `RecursionError` past it — a favourite "what error" MCQ.

### 8.7 Edge cases to scan for in every code MCQ

- Empty list/`max([])` → ValueError; `[1,2] ` with i == len(arr)
- Integer division & negative numbers (see Python module)
- `range` excludes stop value
- Duplicates breaking `index`/`count` assumptions
- Sorting that must be secondary-keyed (stable sort tiebreak)
- String immutability inside loops
- Set/dict `in` vs list `in` complexity assumptions
- Off-by-one on `len(...) - 1`, `<` vs `<=`
- Reading beyond EOF/first element of empty sequence

**Module E recap:** n(n+1)/2 triangular · mult loops log · binary-search iteration math · prefix difference · two-pointer O(n) on sorted · recursion call counting (calls(n)=1+calls(n−1)+calls(n−2)) · tie voting errors · off-by-one.

---

## 9. Module F — Data Engineering & Digital Analytics Fundamentals

### 9.1 ETL vs ELT (know both directions)

| | ETL | ELT |
|---|---|---|
| Where transform happens | outside warehouse (staging tool) | inside warehouse, on load |
| Typical store | row-based OLTP or data mart | cloud DW (Snowflake, BigQuery) |
| Raw retention | transformed only | raw kept → late re-transforms possible |
| Latency target | batch-centric | flexible, supports schema-on-read |

### 9.2 Batch vs streaming

| | Batch | Streaming |
|---|---|---|
| Trigger | scheduled window | continuous events |
| Latency | minutes–hours | seconds |
| Replay | easy (re-run job) | needs event retention / idempotency |
| Typical tool | Airflow orchestration + DW load | Kafka + Flink/Spark Structured Streaming |
| Data shape | large immutable files (Parquet) | small messages |

### 9.3 Data shape taxonomy

- **Structured**: rows/typed columns — relational tables, CSVs.
- **Semi-structured**: tags/keys without fixed schema — JSON, XML, Avro; missing fields repeatable at record level.
- **Unstructured**: free text, images, PDFs, logs' natural text.
- Warehouse (Analytics, columnar, SQL, OLAP) vs operational DB (transactional, row-based, high-write integrity, OLTP). Data lake = raw blob storage (Parquet/JSON) for flexibility; lakehouse = both worlds.

### 9.4 Formats & ingestion notes

- **CSV**: text with delimiter, quoting; header row assumed; NULL representation ambiguous (empty vs `'NULL'` string) → parse explicitly; numeric columns may hold formatted values (`"1,200"`) that break naive parsers.
- **JSON**: objects `{}`, arrays `[]`, nested; null ≠ "" (missing vs zero); arrays of objects map to tables by flattening; date strings may stay ISO text.
- **Relational**: constraints keep quality (PK/FK/CHECK); denormalized warehouse tables trade redundancy for read speed (star schema: fact + dimension tables).
- **API ingestion**: REST + JSON; pagination cursors; rate limits → retry/backoff; auth tokens; incremental pulls since last run (watermarks) vs full reload; idempotency (a repeated same-day reload must not change results).

### 9.5 Quality, validation, lineage, governance (vocabulary that scores)

- **Data quality dimensions**: accuracy, completeness (missing values %), consistency (same truth across systems), timeliness (fresh enough), uniqueness (duplicates), validity (format/range).
- **Data validation** gates: schema check, nulls, referential integrity (no orphan cust_id=99), business rules (non-negative revenue, order after customer signup).
- **Data lineage**: track where each field/table came from and what consumed it — impact analysis for changes; graph of sources → transformations → consumers.
- **Governance**: access control (row/column-level), catalog with metadata/owners, retention & PII policy (masking, anonymization), audit trails.
- Master data management: unique customer identity across systems (dedupe joins).

### 9.6 Spark/PySpark terminology — one-liners (optional depth)

- Lazy evaluation: transformations (`select/filter/groupby`) build a plan; **actions** (`count/show/collect`) trigger execution.
- Narrow transformation: data stays on its partition (`filter`, `map`); wide transformation requires **shuffle** across nodes (`groupByKey`, `orderBy`, joins).
- DataFrame API = optimized (Catalyst plan); RDD = lower-level. Driver plans; executors run tasks; partitions are the unit of parallelism (~128 MB each).
- `df.cache()`/`persist()` keeps an intermediate result in memory across actions (avoid recompute).
- Word count mental model: `flatMap(words) → (word, 1) → reduceByKey(add)` — a wide transformation (shuffle by key).

**Optional-advanced line (skim only if time):** Kafka topics/partitions/offsets; checkpointing in streaming; lakehouse table formats (Delta/Iceberg) enabling ACID on files; cloud columnar DW compute/storage separation.

---

## 10. Common MCQ Traps and Debugging Checklist

### The 30-second trap checklist (before answering)

| Area | Trap |
|---|---|
| Python int ops | `-7//2 == -4` (floors), `int(-3.9) == -3` truncates |
| Python floats | `0.1+0.2 != 0.3`; `round(2.5) == 2` (banker's) |
| Python bools | `True == 1` → dict/set key collapse |
| Python strings | immutable; `s[0]='x'` → TypeError |
| Python lists | `[[]]*3` shared rows; sort() returns None |
| Python references | `x += [...]` mutates aliases; `x = x + [...]` doesn't |
| Generators | single-use; `sum(gen)` after `next(gen)` continues |
| Lambdas | late binding (closure of the variable, not the value) |
| SQL | `COUNT(*)` ≠ `COUNT(col)` with NULLs |
| SQL | `x = NULL` never TRUE; `NOT IN (NULL…)` → 0 rows |
| SQL | `BETWEEN` inclusive; `UNION` dedupes |
| SQL | window functions not usable in WHERE |
| SQL | LEFT JOIN + COUNT(*) may count a NULL-ext row as 1 |
| Stats | % change asymmetric; pp vs %; n−1 for samples |
| Stats | conditional prob needs base rates (P(A|B) ≠ P(B|A)) |
| Pandas | loc label-slice inclusive; assign via `df.loc[mask, col]` |
| Pandas | NaN sums skip; all-NaN mean → NaN |
| Logic | off-by-one (`<` vs `<=`); negative division |
| Logic | tie-vote assumptions; empty input edges |

### Debugging mindsets that fit an MCQ room

1. Find the FIRST line where program state differs from what you'd expect; step forward line by line.
2. For "what's printed?" questions, close the options for the first 30 seconds: compute the answer yourself, then read the options — this kills trap options.
3. For "which is wrong?" questions, test smallest counterexample you remember from this doc.
4. If two options seem right, look for the one that is "generally true vs always true" — MCQs exploit the gap.

---

## 11. Practice Questions (80 total — attempt before the answer key)

> All questions are original practice material designed for HackerEarth-style MCQs. None of them are actual Airbus questions. Single correct answer unless stated. The SQL questions reuse the **Module B sample schema** (5.1) — key rows are restated where needed.

### 11.1 Python — output tracing (PY1–PY20)

**PY1.** What is printed?
```python
print(13 // 4, -13 // 4, 13 % -4, -13 % 4)
```
A) `3 -3 -3 -3`  B) `3 -4 -3 3`  C) `3.25 -3.25 -3 -3`  D) `3 -4 -1 1`

**PY2.**
```python
print(2**2**3, (-2)**2, -2**2)
```
A) `256 4 -4`  B) `64 4 -4`  C) `256 4 4`  D) `64 -4 -4`

**PY3.**
```python
print(4 > 3 == 1, 2 < 3 < 4, (3 == 3) == True)
```
A) `True True True`  B) `False True True`  C) `False True False`  D) `True False True`

**PY4.**
```python
print(round(1.5), round(2.5), round(0.5), round(-1.5))
```
A) `2 3 1 -2`  B) `1 2 0 -1`  C) `2 2 0 -2`  D) `2 2 1 -1`

**PY5.**
```python
p = [1, 2]
q = p
q += [3]
r = p + [4]
p.append(5)
print(p, q, r)
```
A) `[1,2,5] [1,2,3] [1,2,3,4]`  B) `[1,2,3,5] [1,2,3,5] [1,2,3,4]`
C) `[1,2,3,4,5] [1,2,3,4,5] [1,2,3,4,5]`  D) `[1,2,5] [1,2,3,5] [1,2,3,5]`

**PY6.**
```python
m = [[0, 0]] * 2
m[0].append(9)
print(m)
```
A) `[[0,0,9], [0,0]]`  B) `[[0,0,9], [0,0,9]]`  C) `[[0,0], [0,0,9]]`  D) `TypeError`

**PY7.**
```python
d = {1: 'x', True: 'y', '1': 'z'}
print(d, len(d))
```
A) `{1:'x', True:'y', '1':'z'} 3`  B) `{1:'y', '1':'z'} 2`  C) `{True:'y', '1':'z'} 2`  D) `{1:'y'} 1`

**PY8.**
```python
s = 'SEQUENCE'
print(s[2:7:2], s[::-2], s[-3:])
```
A) `QEC ENUE NCE`  B) `QUE ENCE UCE`  C) `QEC ECNE NCE`  D) `ECN ENUE ECE`

**PY9.**
```python
nums = [5, 6, 7, 8]
picked = [nums.pop() for _ in range(2)]
print(picked, nums)
```
A) `[8, 7] [5, 6]`  B) `[5, 6] [7, 8]`  C) `[8, 7] [5, 6, 7]`  D) `[6, 8] [5, 7]`

**PY10.**
```python
fs = [lambda: i for i in range(4)]
print(fs[0]() + fs[3]())
```
A) `3`  B) `4`  C) `6`  D) `0`

**PY11.**
```python
total = 0
for n in range(5, 1, -1):
    if n == 3:
        break
    total += n
else:
    total += 100
print(total, n)
```
A) `14 3`  B) `9 3`  C) `109 3`  D) `114 2`

**PY12.**
```python
g = (x ** 2 for x in range(5))
print(next(g) + next(g))
print(list(g))
```
A) `1` and `[4, 9, 16]`  B) `1` and `[0, 1, 4, 9, 16]`  C) `1` and `[]`  D) `0` and `[4, 9, 16]`

**PY13.**
```python
hits = 0
def f(v, cache={}):
    global hits
    if v not in cache:
        cache[v] = v * 10
        hits += 1
    return cache[v]
print(f(3), f(3), f(4), f(3), hits)
```
A) `30 30 40 30 2`  B) `30 30 40 30 4`  C) `30 30 40 30 3`  D) `30 30 40 30 1`

**PY14.**
```python
def f():
    try:
        return 'A'
    except ZeroDivisionError:
        return 'B'
    finally:
        return 'C'
print(f())
```
A) `A`  B) `B`  C) `C`  D) `None`

**PY15.**
```python
a, *b = range(4)
q, r = divmod(47, 5)
print(a, b, q, r)
```
A) `0 [1, 2, 3] 9 2`  B) `0 [1, 2, 3] 9.4 2`  C) `0 [1,2,3,4] 9 2`  D) `0 (1,2,3) 9 2`

**PY16.**
```python
n = 10
_ = [n for n in range(3)]
print(n)
def boost():
    global n
    n += 5
boost()
print(n)
```
A) `3` and `15`  B) `10` and `15`  C) `2` and `15`  D) `10` and `10`

**PY17.**
```python
nums = [-9, -1, 20, 2]
print(sorted(nums, key=str), min(nums, key=abs), max(nums, key=abs))
```
A) `[-9, -1, 2, 20] -1 20`  B) `[-1, -9, 2, 20] -1 20`
C) `[-1, -9, 2, 20] 1 20`  D) `[-9, -1, 2, 20] -1 2`

**PY18.**
```python
print(5 or 'a', '' and 1, 0 or [], 1 and 2 and 3)
```
A) `5 1 [] 3`  B) `5 '' [] 3`  C) `5 [] [] 3`  D) `5 None [] 3`

**PY19.**
```python
t = (2)
print(type(t).__name__, type((2,)).__name__)
print([1, 2] == [1, 2], [1, 2] is [1, 2])
```
A) `int tuple` / `True False`  B) `tuple tuple` / `True False`
C) `int tuple` / `True True`  D) `tuple tuple` / `True True`

**PY20.**
```python
s = 'ok'
out = ''
for ch in s:
    out += ch.upper() * 2
print(out, len(s), s)
```
A) `OoKk 2 ok`  B) `OOKK 2 ok`  C) `OOKK 4 ok`  D) `OoKk 4 ok`

### 11.2 SQL (SQL1–SQL20) — sample tables of Module B apply

Counting reminder for the orders table: order 104 has amount NULL, order 106 has cust_id NULL, order 108 has cust_id 99 (no such customer), amount 300 appears twice (orders 103 and 107).

**SQL1.** `SELECT COUNT(*), COUNT(amount), COUNT(DISTINCT amount) FROM orders;` →
A) `10 10 9`  B) `10 9 8`  C) `10 9 9`  D) `9 9 8`

**SQL2.** `SELECT COUNT(*) FROM orders o JOIN customers c ON o.cust_id = c.cust_id;` →
A) `10`  B) `9`  C) `8`  D) `7`

**SQL3.** With the customers LEFT JOIN orders ant-join pattern below, which customer(s) come out with **zero** orders?
`SELECT c.name FROM customers c LEFT JOIN orders o ON c.cust_id = o.cust_id WHERE o.order_id IS NULL;`
A) `Meera`  B) `Sneha`  C) `Kiran`  D) `Arjun`

**SQL4.** `SELECT c.city, COUNT(*) FROM orders o JOIN customers c ON o.cust_id = c.cust_id WHERE o.status='Delivered' GROUP BY c.city HAVING COUNT(*) >= 2;` →
A) one row: `Delhi | 4`  B) two rows: `Delhi 4` and `Mumbai 1`  C) one row: `Delhi | 3`  D) no rows

**SQL5.** `SELECT COUNT(*) FROM employees WHERE salary NOT IN (90000, NULL);` →
A) `7`  B) `9`  C) `1`  D) `0`

**SQL6.** `SELECT COUNT(*) FROM employees WHERE salary BETWEEN 45000 AND 60000;` →
A) `3`  B) `4`  C) `5`  D) `6`

**SQL7.** In the window ranking `RANK() OVER (ORDER BY salary DESC)` on employees, what ranks do **Harsh (90000)** and **Bhavna (85000)** get?
A) `1` and `2`  B) `1` and `3`  C) `2` and `3`  D) `1` and `4`

**SQL8.** `SELECT e.name AS emp, m.name AS mgr FROM employees e LEFT JOIN employees m ON e.manager_id = m.emp_id;` — how many rows have `mgr = NULL`?
A) `4`  B) `5`  C) `6`  D) `10`

**SQL9.** Employees earning strictly more than their own department average — how many?
A) `2`  B) `3`  C) `4`  D) `1`

**SQL10.** `SELECT COUNT(*) FROM (SELECT name FROM employees UNION SELECT name FROM customers);` vs the same with `UNION ALL`:
A) `14 and 15`  B) `15 and 15`  C) `14 and 14`  D) `15 and 20`

**SQL11.** In the CASE bucket query of 5.11 (`amount>=1200 → High, >=500 → Medium, ELSE Low`), what does order **104** (amount NULL) map to?
A) `High`  B) `Medium`  C) `Low`  D) `NULL`

**SQL12.** With the running-total window `SUM(amount) OVER (ORDER BY order_id)` filtered to non-NULL amounts, what is the running total printed at order **107**?
A) `3600`  B) `3850`  C) `3250`  D) `2200`

**SQL13.** `DENSE_RANK()` (salary DESC) = 2 corresponds to which employee?
A) `Harsh`  B) `Bhavna`  C) `Jay`  D) `Esha`

**SQL14.** Which query counts customers with **no** orders?
A) `SELECT COUNT(*) FROM customers c WHERE NOT EXISTS (SELECT 1 FROM orders o WHERE o.cust_id <> c.cust_id);`
B) `SELECT COUNT(*) FROM customers c LEFT JOIN orders o ON c.cust_id = o.cust_id WHERE o.order_id IS NULL;`
C) `SELECT COUNT(*) FROM customers c INNER JOIN orders o ON c.cust_id = o.cust_id WHERE o.order_id IS NULL;`
D) `SELECT COUNT(*) FROM customers c WHERE c.cust_id NOT IN (SELECT DISTINCT cust_id FROM orders);`

**SQL15.** `SELECT dept_id, COUNT(*) FROM employees GROUP BY dept_id;` — how many groups?
A) `4`  B) `5`  C) `6`  D) `9`

**SQL16.** `SELECT COUNT(*) FROM customers WHERE city = NULL;` →
A) same as IS NULL (0 here)  B) `5`  C) error in all engines  D) `0` always (predicate never TRUE)

**SQL17.** In **PostgreSQL**, `SELECT name FROM employees ORDER BY salary ASC;` — where do NULL salaries appear?
A) first rows  B) last rows  C) middle  D) excluded entirely

**SQL18.** In PostgreSQL (numeric column type), what does `SELECT 7/2;` return?
A) `3`  B) `3.5`  C) `3.5` rounded to `4`  D) engine-error

**SQL19.** `SELECT COUNT(*) FROM employees e FULL OUTER JOIN departments d ON e.dept_id = d.dept_id;` →
A) `10`  B) `11`  C) `12`  D) `15`

**SQL20.** Which returns the **second highest distinct salary**?
A) `SELECT MAX(salary) FROM employees;`
B) `SELECT MAX(salary) FROM employees WHERE salary < (SELECT MAX(salary) FROM employees);`
C) `SELECT MIN(salary) FROM employees WHERE salary > (SELECT MIN(salary) FROM employees);`
D) `SELECT MAX(salary) FROM employees WHERE salary > (SELECT MAX(salary) FROM employees);`

### 11.3 Statistics / data interpretation (ST1–ST15)

**ST1.** Data: `21, 34, 12, 9, 27, 15`. The median is:
A) 15  B) 18  C) 19  D) 21

**ST2.** Revenue rose from 480 to 600. The percentage change is:
A) +125%  B) +20%  C) +25%  D) +150%

**ST3.** Approval rating moved 42% → 48%. The relative increase (in %) is closest to:
A) 6%  B) 14.3%  C) 4.8%  D) 114.3%

**ST4.** For data `2, 4, 6, 8` treated as a **sample**, the sample variance is:
A) 5  B) 6.67  C) 2.58  D) 8

**ST5.** Dataset `5, 5, 5, 5, 50`: mean = 14, median = 5. Which claim is correct?
A) mean is more robust to the outlier  B) median is more robust to the outlier
C) both change equally  D) mode is 50

**ST6.** Weighted average of 82 (weight 4), 76 (weight 2), 91 (weight 6):
A) 83.0  B) 85.5  C) 85.0  D) 87.5

**ST7.** Q1 = 30, Q3 = 50 (IQR = 20). Which values are outliers by the 1.5·IQR rule?
I. 85   II. 15   III. 78
A) only I  B) I and II  C) I and III  D) none

**ST8.** 100 flights: airline A operates 60 with 6 delayed; airline B operates 40 with 12 delayed. `P(delayed | A)` and `P(A | delayed)`:
A) 0.10 and 0.429  B) 0.10 and 0.333  C) 0.18 and 0.333  D) 0.12 and 0.429

**ST9.** 800 clicks → 12 purchases. The conversion rate is:
A) 1.5%  B) 0.15%  C) 6.7%  D) 15%

**ST10.** A cohort of 500 users saw 350 active the next month. Retention & churn:
A) 70% / 30%  B) 35% / 65%  C) 30% / 70%  D) 143% / −43%

**ST11.** Sales grew 10% in year 1 and 20% in year 2 (compounded). The two-year growth is:
A) +30%  B) +32%  C) +15%  D) +132%

**ST12.** Group X: mean 50, n=20. Group Y: mean 70, n=30. The combined mean is:
A) 60  B) 62  C) 63  D) 55

**ST13.** Data `8, 3, 8, 5, 8, 2, 3`. The mode and range are:
A) 8 and 6  B) 3 and 6  C) 8 and 3  D) 5 and 6

**ST14.** Best chart for comparing the revenue of 8 product categories:
A) horizontal bar chart  B) pie chart  C) scatter plot  D) line chart

**ST15.** A dashboard zooms the y-axis from 55% to 65% to expose a metric's rise from 57% to 62%. The main visual risk:
A) the change becomes hidden  B) small change appears dramatic (exaggerated slope on a truncated axis)
C) percentages cannot be plotted  D) the axis must always start at 0, so nothing is shown

### 11.4 Pandas (PD1–PD10)

**PD1.**
```python
df = pd.DataFrame({'a':[1,2,3,4,5]})
s = df.set_index('a')
print(len(s.loc[2:4]))
```
A) `2`  B) `3`  C) `4`  D) `KeyError`

**PD2.**
```python
df2 = df.copy()
df2[df2.sales > 40]['sales'] = 0
```
What is `df2['sales']` afterwards?
A) modified: values > 40 set to 0  B) unchanged (+ SettingWithCopyWarning possible)
C) NaN for rows > 40  D) raises KeyError

**PD3.**
```python
df = pd.DataFrame({'dept':['A','A','B','B','C'], 'sales':[10,30,50,70,20]})
g = df.groupby('dept')['sales'].mean()
print(g['B'], g.max(), len(g))
```
A) `60.0 60.0 3`  B) `60.0 60.0 5`  C) `60.0 70 3`  D) `35.0 60.0 3`

**PD4.**
```python
left  = pd.DataFrame({'k':[1,2,3], 'v':[10,20,30]})
right = pd.DataFrame({'k':[3,4],   'w':[7,8]})
m = pd.merge(left, right, how='left')
print(len(m), int(m['w'].isnull().sum()))
```
A) `2 1`  B) `3 1`  C) `3 2`  D) `4 2`

**PD5.**
```python
d1 = pd.DataFrame({'k':[1,2]})
d2 = pd.DataFrame({'k':[3,4]})
c = pd.concat([d1, d2])
print(len(c), c.index.tolist())
```
A) `4 [0, 1, 2, 3]`  B) `4 [0, 1, 0, 1]`  C) `2 [0, 1]`  D) `4 [0, 0, 1, 1]`

**PD6.**
```python
dd = pd.DataFrame({'a':[1,1,2,2,2,3]})
print(dd.duplicated().tolist(), len(dd.drop_duplicates()))
```
A) `[F,T,F,T,T,F] 3`  B) `[F,T,F,T,T,F] 2`  C) `[T,T,T,T,T,F] 3`  D) `[F,F,F,T,T,F] 4`

**PD7.** A column of all NaNs: are `col.sum()` and `col.mean()`?
A) `0.0` and `0.0`  B) `NaN` and `NaN`  C) `0.0` and `NaN`  D) `NaN` and `0.0`

**PD8.** Which line raises a `TypeError` most often (operational pitfall)?
A) `df[df.x > 2 & df.y < 1]` (no parentheses)  B) `df[(df.x > 2) & (df.y < 1)]`
C) `df.query('x > 2')`  D) `df.loc[df.x > 2, 'y']`

**PD9.**
```python
ss = pd.DataFrame({'x':[2,3,2,3], 'y':[5,4,6,3]})
print(ss.sort_values(['x','y'], ascending=[True, False]).index.tolist())
```
A) `[0, 2, 1, 3]`  B) `[2, 0, 1, 3]`  C) `[0, 2, 3, 1]`  D) `[1, 3, 2, 0]`

**PD10.**
```python
ddate = pd.DataFrame({'d': pd.to_datetime(['2024-01-05', '2024-03-20', '2024-02-11'])})
print(ddate['d'].dt.month.tolist())
```
A) `[1, 3, 2]`  B) `[5, 20, 11]`  C) `[1, 2, 3]`  D) `error: use .dt.month on datetime64 only`

### 11.5 Programming logic (LC1–LC10)

**LC1.**
```python
count = 0
for i in range(4):
    for j in range(i, 4):
        count += 1
print(count)
```
A) `16`  B) `10`  C) `12`  D) `6`

**LC2.**
```python
i = 1; k = 0
while i < 50:
    i *= 3
    k += 1
print(k, i)
```
A) `3 27`  B) `4 81`  C) `5 243`  D) `4 27`

**LC3.** Binary search on `[2,4,6,8,10,12]` searching 12 with the `lo<=hi` inclusive convention: how many iterations and what is `a[mid]` after the loop?
A) `2 12`  B) `3 12`  C) `3 10`  D) `2 10`

**LC4.** `a = [1,3,5,7]`, prefix `pre = [0,1,4,9,16]`. Sum of `a[1..2]` (inclusive on both ends) via the prefix-difference is:
A) `8`  B) `9`  C) `7`  D) `16`

**LC5.** Fastest average membership test in a large collection:
A) list `in`  B) set membership  C) tuple `in`  D) sorted list scan from front

**LC6.** Two pointers on sorted `[3,8,12,19]`, target 11 — how many sums are tried, and which pair wins?
A) `3 (3,8)`  B) `2 (3,8)`  C) `3 (12,19)`  D) `4 (3,8)`

**LC7.** `calls(n) = 1` if n<2 else `1 + calls(n-1) + calls(n-2)`. Find `calls(4)` and `calls(3)`:
A) `9 5`  B) `8 4`  C) `9 4`  D) `11 6`

**LC8.** Which gives the best average performance for counting word frequencies in a document?
A) nested linear search per word  B) dict/Counter accumulation  C) sort then count  D) membership test in list

**LC9.**
```python
print(sum(range(1, 10)), sum(range(10)))
```
A) `55 45`  B) `45 45`  C) `45 55`  D) `54 45`

**LC10.** What's the average complexity of `for v in range(0, n, 3):`?
A) O(1)  B) O(log n)  C) O(n)  D) O(n²)

### 11.6 Data engineering (DE1–DE5)

**DE1.** Transformations happen **inside the warehouse after loading raw data** — this pattern is:
A) ETL  B) ELT  C) ETL-in-reverse  D) none — that is streaming

**DE2.** Fraud scoring within milliseconds of a transaction is a natural fit for:
A) nightly batch ETL  B) stream processing  C) monthly snapshots  D) manual export

**DE3.** Which type of data is "semi-structured"?
A) JSON records with optional nested fields  B) relational table  C) JPEG image  D) plain table CSV

**DE4.** Which Spark transformation is **wide** (needs a shuffle)?
A) `filter`  B) `select`  C) `groupBy(...).sum()`  D) `map`

**DE5.** The same customer record shows two different birth dates in two internal systems. Which data-quality dimension is violated first?
A) timeliness  B) consistency  C) uniqueness  D) volume

```
(End of questions — answers and explanations follow in Section 12.)
```

---

## 12. Answer Key with Detailed Explanations

### 12.1 Python (PY1–PY20)

- **PY1 → B** `3 -4 -3 3`. `13//4=3`; floor(−3.25) = −4; `13 % -4` → sign follows divisor: 13 − (−4)·floor(13/−4) = 13 − (−4)(−4) = −3; `−13 % 4` → −13 − 4·floor(−13/4) = −13 − 4(−4) = 3.
- **PY2 → A** `256 4 -4`. `**` right-assoc: 2**(2**3) = 2**8 = 256; `(-2)**2 = 4`; unary minus binds after `**`: `−2**2 = −(2²) = −4`.
- **PY3 → B** `False True True`. Chained `4>3==1` = `(4>3) and (3==1)` → False. `2<3<4` → True. `(3==3) == True` → `True == True` → True.
- **PY4 → C** `2 2 0 -2`. Banker's rounding to the even neighbour: 1.5→2, 2.5→2, 0.5→0, −1.5→−2.
- **PY5 → B** `q = p` is an alias; `q += [3]` mutates the shared list → p and q both `[1,2,3]`; `r = p + [4]` created a new list `[1,2,3,4]` (takes p's old state); `p.append(5)` → p (and q) are `[1,2,3,5]`.
- **PY6 → B** `[[]]*2` repeats ONE list twice → both rows show the append: `[[0,0,9], [0,0,9]]`.
- **PY7 → B** `{1:'y', '1':'z'} 2`. `1`, `True` are the same key (equal & same hash) → key stays `1`, value last-wins `'y'`; `'1'` is distinct. len=2.
- **PY8 → A** `QEC ENUE NCE`. Indices (0-based): S E Q U E N C E → step-2 from 2..6 → 2,4,6 = Q,E,C. Reversed step-2 → indices 7,5,3,1 = E,N,U,E. Last 3 chars = N,C,E.
- **PY9 → A** `[8, 7] [5, 6]`. `pop()` removes from the END twice → 8 then 7; remaining `[5,6]`.
- **PY10 → C** `6`. Late binding: both lambdas see the final loop value `i == 3` → 3 + 3 = 6.
- **PY11 → B** `9 3`. Break at n=3 → the `else` (runs only without break) is skipped; total keeps 5+4=9; loop variable `n` remains 3.
- **PY12 → A** `1` then `[4, 9, 16]`. First two pulls are 0² and 1² → 0+1 = 1; the rest of the generator then yields 4, 9, 16 (it's exhausted afterwards).
- **PY13 → A** `30 30 40 30 2`. Cache persists across calls: computed for 3 and 4 only → hits = 2; `f(3)` again returns the stored 30.
- **PY14 → C** `'C'`. A `return` in `finally` overrides the return of the try/except body (works even without an exception).
- **PY15 → A** `0 [1, 2, 3] 9 2`. `a, *b` → a=0, rest `[1,2,3]`; `divmod(47,5)` = (9, 2).
- **PY16 → B** `10` then `15`. Comprehension loop variable does NOT leak in Python 3 → n stays 10; `global n; n += 5` → 15.
- **PY17 → B** `[-1, -9, 2, 20] -1 20`. `key=str` compares strings `'-1' < '-9' < '2' < '20'` (character '1' < '9' at position 1). `min/max(key=abs)` → |−1|=1 smallest, |20|=20 largest.
- **PY18 → B** operands passed through: `5 or 'a'` → 5; `'' and 1` → `''`; `0 or []` → `[]`; `1 and 2 and 3` → 3. Printed line reads `5  [] 3` (double space where the empty string sits).
- **PY19 → A** `t = (2)` is int; `(2,)` is a tuple; lists compare by value (True) but not identity (False).
- **PY20 → B** `OOKK 2 ok`. Each char upper ×2 appends: 'O' + 'O' + 'K' + 'K' = OOKK. `s` itself is never changed (string immutability), len 2.

### 12.2 SQL (SQL1–SQL20)

- **SQL1 → B** `10 9 8`. COUNT(*) = all rows; COUNT(amount) skips order 104's NULL; DISTINCT collapses the duplicate 300 (103, 107) → 8.
- **SQL2 → C** `8`. Orders 106 and 108 never match a customer (NULL-join fails, cust 99 missing) → 8 rows.
- **SQL3 → C** `Kiran`. The anti-join (LEFT JOIN + IS NULL) returns exactly the customers without orders: Kiran.
- **SQL4 → A** `Delhi | 4`. Delivered → joined cities: Delhi 4, Mumbai 1 → HAVING ≥ 2 keeps only Delhi.
- **SQL5 → D** `0`. `NOT IN (…, NULL)` makes every row UNKNOWN unless an exact IN match exists — here exactly nothing can be TRUE → zero rows.
- **SQL6 → B** `4`. BETWEEN 45000–60000 inclusive: Divya 45000, Farah 52000, Chetan 55000, Esha 60000 → 4.
- **SQL7 → B** `1 and 3`. RANK skips: 90000 ties both get 1; the next distinct salary 85000 → 3.
- **SQL8 → C** `6`. NULL managers: Ankit, Chetan, Esha, Farah, Isha, Jay (Bhavna, Divya, Harsh, Kiran have managers — verified).
- **SQL9 → B** `3`. Dept averages: Eng (90000+85000+90000)/3 = 88333.33 → Ankit, Harsh > avg; Sales avg 50000 → Chetan > avg; HR/Marketing: none above equal-to-own-average → 3 total.
- **SQL10 → A** `14 and 15`. UNION dedups employee-7/customer-5 "Kiran" → 14; UNION ALL keeps → 15.
- **SQL11 → C** `Low`. NULL ≥ 1200 and ≥ 500 are UNKNOWN (not TRUE) → falls through to ELSE.
- **SQL12 → B** `3850`. Running totals (non-NULL): 101:1200 → 102:1900 → 103:2200 → 105:2650 → 106:3550 → **107:3850**.
- **SQL13 → B** `Bhavna` (85000). Dense ranks: 1 (90000), 2 (85000), 3 (78000)…
- **SQL14 → B**. B is the standard anti-join. D is a trap: the orders table contains a NULL `cust_id` (order 106), so `NOT IN (SELECT DISTINCT cust_id ...)` sees a NULL and returns 0 rows. A has the operator backwards; C uses an inner join, so the IS NULL filter can never fire.
- **SQL15 → B** `5`. Numeric groups 1–4 plus ONE group for all three NULL-dept employees.
- **SQL16 → D** `0` always — `city = NULL` is UNKNOWN for every row (NULL != NULL in SQL's 3VL); the correct predicate is `IS NULL`.
- **SQL17 → B** last. PostgreSQL treats NULL as largest by default in ASC.
- **SQL18 → B** `3.5`. PostgreSQL integer/integer is true (numeric) division. (SQLite would show 3 — an engine difference worth knowing.)
- **SQL19 → B** `11`. Inner-matched 7 + unmatched employees 3 + unmatched department (Finance) 1 = 11.
- **SQL20 → B**. MAX over salaries less than the overall MAX → 85000. D contradicts itself (nothing > MAX). C is second-lowest logic. A is the highest itself.

### 12.3 Statistics (ST1–ST15)

- **ST1 → B** `18`. Sorted: 9, 12, [15, 21], 27, 34 → even n → (15+21)/2.
- **ST2 → C** `+25%`. (600−480)/480 = 120/480 = 0.25. (Not +150%! and note reverse move would be −20%.)
- **ST3 → B** `14.3%`. Relative: (48−42)/42 = 6/42 ≈ 0.1428. Option A (6%) is the **percentage-point** change — classic trap.
- **ST4 → B** `6.67`. Deviations 9,1,1,9 → SS=20; sample: 20/(n−1)=20/3. (5(pop), 2.58(SD), 8 distractors.)
- **ST5 → B** median is robust — stays 5; mean jumps to 14. Mode is 5, not 50.
- **ST6 → B** `85.5`. (82·4 + 76·2 + 91·6)/12 = (328+152+546)/12 = 1026/12.
- **ST7 → A** only 85. Fences: 30 − 1.5·20 = 0, 50 + 1.5·20 = 80. 85 > 80 → outlier; 15 ≥ 0 and 78 ≤ 80 → within.
- **ST8 → B** `0.10 and 0.333`. P(delayed|A) = 6/60; P(A|delayed) = 6/(6+12) = 6/18 = 1/3 — the base-rate order matters.
- **ST9 → A** `1.5%`. 12/800 = 0.015. 15% would be overcounts by 10× (12/80).
- **ST10 → A** `70% / 30%`. 350/500 = 0.70; churn = 1 − retention.
- **ST11 → B** `+32%`. 1.10 × 1.20 = 1.32 → +32% (not 1.10+0.20!). 
- **ST12 → B** `62`. (20·50 + 30·70)/50 = (1000+2100)/50 = 62 — never the plain average of 50 and 70.
- **ST13 → A** mode 8 (×3), range 8−2 = 6.
- **ST14 → A** horizontal bar — category comparison with readable labels; pie with 8 slices is poor, scatter/line are for 2-variable/trend data.
- **ST15 → B** truncating the axis exaggerates small differences (classic misleading-dashboard move).

### 12.4 Pandas (PD1–PD10)

- **PD1 → B** `3`. `loc[2:4]` is label-based and INCLUDES the stop label → ids 2,3,4.
- **PD2 → B** unchanged. Chained indexing assigns into a *copy*; the fix is `df2.loc[df2.sales > 40, 'sales'] = 0`.
- **PD3 → A** `60.0 60.0 3`. B mean = (50+70)/2 = 60; means are 20, 60, 20 → max 60; 3 groups.
- **PD4 → C** `3 2`. Left merge keeps all 3 left rows; only k=3 finds a right match → NaN count 2.
- **PD5 → B** `4 [0, 1, 0, 1]`. concat default axis=0 rows, keeps original indices (duplicate labels!) — use `ignore_index=True` for `[0,1,2,3]`.
- **PD6 → A** `[F,T,F,T,T,F] 3`. `duplicated()` flags subsequent repeats (keep='first'); drop keeps 3 unique rows.
- **PD7 → C** `0.0 and NaN`. skipna=True: summing nothing contributes 0; mean = 0/0 → NaN.
- **PD8 → A**. `&` binds tighter than `>`; without parentheses it's an error. B–D are the correct/preferred forms.
- **PD9 → B** `[2, 0, 1, 3]`. x ascending → x=2 group first; inside group y desc (idx2 y=6 before idx0 y=5); then x=3: idx1 (y4) before idx3 (y3).
- **PD10 → A** `[1, 3, 2]` — `.dt.month` per row; NOT sorted output, not days.

### 12.5 Logic/LC (LC1–LC10)

- **LC1 → B** `10`. 4+3+2+1 triangular sum.
- **LC2 → B** `4 81`. i: 3, 9, 27, 81 → 4 iterations; loop exits with i=81.
- **LC3 → B** `3 12`. Searches 12 over 6 elements: mid idx2 (6) → go right → idx4 (10) → right → idx5 (12) ✓ → 3 iterations.
- **LC4 → A** `8`. pre[3] − pre[1] = 9 − 1 = 8 = a[1]+a[2] = 3+5.
- **LC5 → B** set — O(1) average vs O(n) list scans.
- **LC6 → A** `3 (3,8)`. Sums: 3+19=22 (>, hi–), 3+12=15 (>, hi–), 3+8=11 ✓.
- **LC7 → A** `9 5`. calls: c0=1, c1=1, c2=3, c3=5, c4=9.
- **LC8 → B** Counter/dict — single O(n) pass with O(1) updates.
- **LC9 → B** `45 45`. range(1,10) sums 1..9 = 45; range(10) sums 0..9 = 45 (same!); range(11) would be 55.
- **LC10 → C** O(n) — step doesn't change the linear class.

### 12.6 Data engineering (DE1–DE5)

- **DE1 → B** ELT = extract, load, then transform inside the warehouse.
- **DE2 → B** streaming (subsecond reaction needs event-flow processing, not batch).
- **DE3 → A** JSON with optional nested fields = semi-structured; relational/CSV flat = structured; JPEG = unstructured.
- **DE4 → C** groupBy keys must be co-partitioned → shuffle (wide). filter/select/map are narrow.
- **DE5 → B** consistency — same fact disagrees across systems (accuracy is about being wrong vs the real world; here both can't be right).

---

## 13. Timed Mock Assessment (60 minutes — simulation only)

> ⚠️ This mock is a **practice simulation** designed by this document. It does not reproduce Airbus's actual question count or distribution (which were never disclosed). Grade yourself, then use the mistake template.

**Suggested 60-minute allocation:**

| Segment | Questions | Budget |
|---|---|---|
| Python tracing | MOCK PY1–PY10 | 16 min |
| SQL with sample tables | MOCK SQL1–SQL8 | 16 min |
| Statistics / data interpretation | MOCK ST1–ST5 | 8 min |
| Programming logic | MOCK LC1–LC5 | 9 min |
| Revisit flagged questions | — | 6 min |
| Final sweep & submit | — | 5 min |

### 13.1 Questions

**PY1.**
```python
vals = [5, 1, 7, 3, 9, 2]
out = [v * 2 for v in vals if v % 2 == 1]
print(out[::-1])
```
A) `[18, 6, 14, 2, 10]`  B) `[10, 14, 6, 18, 2]`  C) `[10, 2, 14, 6, 18]`  D) `[18, 2, 14, 6, 10]`

**PY2.**
```python
x = 'analytics'
y = x.upper()
print(x[2:5], y[::2], len(x))
```
A) `aly AAYIS 9`  B) `aly AAYIS 8`  C) `ana AAIS 9`  D) `aly AAIS 9`

**PY3.**
```python
counts = {}
for w in 'to be or not to be'.split():
    counts[w] = counts.get(w, 0) + 1
print(sum(counts.values()), len(counts))
```
A) `6 4`  B) `7 4`  C) `6 5`  D) `5 4`

**PY4.**
```python
def g(k, seen=set()):
    seen.add(k)
    return len(seen)
print(g(1), g(2), g(3), g(1))
```
A) `1 2 3 4`  B) `1 2 3 3`  C) `1 2 3 1`  D) `1 2 2 3`

**PY5.**
```python
x = [1, 2, 3]
y = [1, 2, 3]
print(x == y, x is y, sum(x) == sum(y))
```
A) `True True True`  B) `True False True`  C) `False False True`  D) `True True False`

**PY6.**
```python
for i in range(3):
    if i == 7:
        break
else:
    print('no break')
print('done', i)
```
A) `done 2` only  B) `no break` then `done 2`  C) `no break` then `done 3`  D) `done 3` only

**PY7.** In Python, what is `type(('a')).__name__`?
A) `tuple`  B) `list`  C) `str`  D) `set`

**PY8.**
```python
d = {}
print(d.get('x'), d.get('x', 0))
```
A) `0 0`  B) `None None`  C) `None 0`  D) `KeyError`

**PY9.**
```python
print(1, 2, 3, sep='*', end='!')
print('next')
```
A) `1*2*3!next`  B) `1*2*3! next`  C) `1*2*3 next`  D) `1,2,3!next`

**PY10.**
```python
s = 'COMPUTER'
print(s[1:6:2], s[::-1][:4])
```
A) `OPT RETU`  B) `OPT ERTU`  C) `OPU RETU`  D) `OPU TURE`

**SQL FAQ (counts from Module B data).** orders: 101(c1,1200,D), 102(c1,700,X), 103(c2,300,D), 104(c3,NULL,P), 105(c3,450,D), 106(cNULL,900,D), 107(c2,300,D), 108(c99,500,D), 109(c4,1500,X), 110(c2,600,D). D=Delivered, X=Cancelled, P=Pending.

**SQL1.** `SELECT status, COUNT(*) FROM orders GROUP BY status;` — the counts (by status) are:
A) Delivered 7, Cancelled 2, Pending 1  B) Delivered 6, Cancelled 3, Pending 1
C) Delivered 7, Cancelled 1, Pending 2  D) Delivered 8, Cancelled 2, Pending 1

**SQL2.** `SELECT SUM(amount) FROM orders WHERE cust_id = 1;` (customer 1 = Meera) →
A) `1900`  B) `1200`  C) `2500`  D) `NULL`

**SQL3.** How many orders does **Rohan** (cust 2) have?
A) `2`  B) `3`  C) `4`  D) `1`

**SQL4.** In Engineering (dept 1: Ankit 90000, Bhavna 85000, Harsh 90000):
`SELECT name FROM employees WHERE dept_id = 1 AND salary = (SELECT MAX(salary) FROM employees WHERE dept_id = 1);`
— how many rows?
A) `1`  B) `2`  C) `3`  D) `0`

**SQL5.** For employees sharing the max salary under `ROW_NUMBER() OVER (ORDER BY salary DESC)`, which statement is TRUE?
A) both get row 2  B) both get row 1  C) one gets 1 and one gets 2, deterministic by name D) one gets 1 and one gets 2, tie-break order not guaranteed unless a tiebreaker is added

**SQL6.** For **Sneha** (cust 3: orders 104 amount NULL, 105 amount 450), what is `LAG(amount) OVER (PARTITION BY cust_id ORDER BY order_id)` at order **105**?
A) `450`  B) `0`  C) `NULL`  D) `1200`

**SQL7.** `SELECT COUNT(*) FROM employees WHERE salary NOT IN (90000, NULL);` →
A) `9`  B) `7`  C) `1`  D) `0`

**SQL8.** `SELECT COUNT(*) FROM (SELECT name FROM employees UNION SELECT name FROM customers);` →
A) `15`  B) `14`  C) `10`  D) `11`

**ST1.** Data: `12, 7, 22, 15, 25, 4`. Median = ?
A) 13.5  B) 15  C) 12  D) 14.2

**ST2.** Production moved from 6400 units to 8000 units. Percentage change:
A) −25%  B) +20%  C) +25%  D) +15%

**ST3.** Average orders/day: weekdays 120 (5 days), weekends 200 (2 days). The true weekly average is:
A) 160  B) 143  C) 150  D) 124

**ST4.** 300 emails sent; 60 opened; of the 60 opened, 9 clicked. Overall click rate on sent emails is:
A) 3%  B) 15%  C) 20%  D) 12%

**ST5.** A metric shows: average session 6.5 min, median session 3.1 min. The best immediate read:
A) the data is symmetric  B) the average is inflated by outlier sessions (right-skewed)
C) medians are always wrong  D) the mean should be reported for skew

**LC1.**
```python
count = 0
for i in range(6):
    for j in range(i):
        count += 1
print(count)
```
A) `21`  B) `15`  C) `36`  D) `20`

**LC2.**
```python
x = 13; k = 0
while x > 3:
    x //= 2
    k += 1
print(k, x)
```
A) `2 3`  B) `3 1`  C) `2 6`  D) `3 3`

**LC3.** Binary search (inclusive `lo<=hi`) over 8 sorted elements searching the FIRST element — how many iterations?
A) `1`  B) `2`  C) `3`  D) `4`

**LC4.** Building a dict of item→count from a list of n items costs, on average:
A) O(1)  B) O(n)  C) O(n log n)  D) O(n²)

**LC5.** `a = [2,2,2,2]`, prefix `pre = [0,2,4,6,8]`. Sum of a[1..3] = `pre[4] − pre[1]`:
A) `6`  B) `4`  C) `8`  D) `2`

### 13.2 Mock answer key + explanations

| Q | Ans | Reasoning |
|---|---|---|
| PY1 | A | odds 5,1,7,3,9 → ×2 = [10,2,14,6,18]; reversed → [18,6,14,2,10] (verified) |
| PY2 | A | x[2:5]='aly'; 'ANALYTICS' step-2 picks indices 0,2,4,6,8 → A,Y,I,S + first A → AAYIS? trace: chars 0='A',2='A',4='Y',6='I',8='S' → 'AAYIS'; len=9 |
| PY3 | A | seen words: to2, be2, or1, not1 → sum 6, unique 4 |
| PY4 | B | shared `seen` set persists across calls (mutable default) → sizes 1,2,3,3 |
| PY5 | B | == is value → True; `is` on two separately built lists → False; sums equal → True |
| PY6 | B | i never equals 7 → no break → else runs; final i = 2 |
| PY7 | C | ('a') is just the string 'a'; a 1-tuple needs a comma ('a',) |
| PY8 | C | `.get` default None without default; with default → 0 (key not added) |
| PY9 | A | sep='*' between args; end='!' suppresses newline → 'next' continues on the same line |
| PY10 | A | indices 1,3,5 → O,P,T → 'OPT' (C0 O1 M2 P3 U4 T5 E6 R7); reversed first 4 → 'RETU' |
| SQL1 | A | Delivered 7 (101,103,105,106,107,108,110), Cancelled 2 (102,109), Pending 1 (104) |
| SQL2 | A | Meera: 1200 + 700 = 1900 (both orders count, cancelled included) |
| SQL3 | B | Rohan: 103, 107, 110 → 3 |
| SQL4 | B | MAX = 90000 → both Ankit and Harsh match = 2 rows |
| SQL5 | D | ROW_NUMBER breaks ties arbitrarily; add a deterministic tiebreaker (e.g., name) for reproducibility |
| SQL6 | C | LAG at 105 sees order 104 whose amount is NULL → NULL (LAG doesn't skip NULLs) |
| SQL7 | D | NOT IN with a NULL inside the list → no row can satisfy TRUE → 0 |
| SQL8 | B | UNION dedups employee-7/customer-5 'Kiran' → 14 (UNION ALL would be 15) |
| ST1 | A | sorted [4,7,12,15,22,25] → (12+15)/2 = 13.5 |
| ST2 | C | (8000−6400)/6400 = 1600/6400 = 0.25 → +25% |
| ST3 | B | (120·5 + 200·2)/7 = 1000/7 ≈ 142.9 → ≈143 (not the plain 160) |
| ST4 | A | 9/300 = 3% clicks from send base (15% & 20% are stage rates — mixed-base trap) |
| ST5 | B | mean >> median signals right skew/outliers; report median/box for skew |
| LC1 | B | 0+1+2+3+4+5 = 15 |
| LC2 | A | 13→6 (k=1), 6→3 (k=2); 3>3 false stops → 2 iterations, x=3 |
| LC3 | C | mid 3 (4) → hi=2 → mid 1 (2) → hi=0 → mid 0 (1) ✓ → 3 |
| LC4 | B | single O(n) pass of O(1) updates |
| LC5 | A | pre[4] − pre[1] = 8 − 2 = 6 = 2+2+2 |

### 13.3 Score sheet

| Section | Questions | Correct | Marks (1 each) | Notes |
|---|---|---|---|---|
| Python | PY1–PY10 | ____ | /10 | |
| SQL | SQL1–SQL8 | ____ | /8 | |
| Statistics | ST1–ST5 | ____ | /5 | |
| Logic | LC1–LC5 | ____ | /5 | |
| **Total** | 28 | | **/28** | |

**Self-evaluation bands (informal):** 24–28 sharp → go with confidence · 18–23 → revise flagged topics only · 11–17 → redo the marked modules tonight · <11 → prioritize cheat sheet + Module A/B examples, sleep well, steady pace tomorrow.

### 13.4 Mistake-analysis template

Fill after grading (keep one line each mistake):

| # | Q no | Topic | Chosen vs correct | Root cause (tick) | Fix |
|---|---|---|---|---|---|
| 1 | | | | [] concept gap [] trap option [] misread [] arithmetic [] panic | |
| 2 | | | | | |
| 3 | | | | | |

Rules of thumb: ≥3 arithmetic slips → slow down, recompute before marking. Trap options → review Section 10 trap list. Concept gaps → open the mapped module below.

### 13.5 Remediation map

- Python basics/mutability → 4.1–4.6, 4.12, 4.18
- Python control flow/loops/exceptions → 4.14, 4.15
- Python slicing/generators/lambdas → 4.7, 4.13
- SQL joins & NULLs → 5.3, 5.6, 5.7, 10
- SQL aggregation/GROUP BY/HAVING → 5.4, 5.5
- SQL windows → 5.10
- Stats % change/pp/weighted → 6.2, 6.3, 6.9
- Stats distribution/outliers → 6.4, 6.11
- Pandas semantics → 7.1–7.10
- Logic/complexity → 8.1–8.7

---

## 14. Preparation Schedules

### 14.1 Schedule A — the evening before (adapt "tonight" to T-minus if your window moved)

| Time (IST) | Block | What exactly to do |
|---|---|---|
| 19:00–19:25 | Platform reality check | Open the HackerEarth **practice test** once more, run through navigation: question palette, flag/mark-for-review, save-and-next behaviour, timer position. Note: practice says 45 min/5 MCQs; the real test is 60 min — do not over-index on either. |
| 19:25–20:15 | Python traps — Module A | Re-read 4.1–4.15. Solve PY1–PY20 **without** the key first; check answers; log mistakes in the template (13.4). |
| 20:15–21:00 | SQL — Module B | Read 5.2–5.12 actively: re-derive each output before looking. Solve SQL1–SQL20 the same way. |
| 21:00–21:15 | Break | Walk, water, no phone scrolling. |
| 21:15–21:45 | Statistics — Module C | 6.1–6.11 + ST1–ST15. Focus: % change asymmetry, conditional probability base rates, weighted averages. |
| 21:45–22:10 | Mixed speed quiz | LC1–LC10 + PD1–PD10 + DE1–DE5 quickly (uninterrupted, ~15 min), then key-check. |
| 22:10–22:25 | Cheat sheet | Read Section 3 in full, slowly. |
| 22:25–23:00 | Wind-down | Prepare tomorrow's log-in details, quiet room, charger, water. **Sleep ≥ 7 hours.** No late-night mock — accuracy tonight matters more than volume. |

Optional tail (if energy): redo only the questions you got wrong today, from the key explanations.

### 14.2 Schedule B — assessment day (target start ≈ 12:15 PM IST)

| Time (IST) | Block | Notes |
|---|---|---|
| Morning (college) | Attend as planned | Keep phone on low; no last-minute cramming needed. |
| 12:00–12:10 | Reach home, settle | Eat something light (a heavy meal kills alertness at 13:00). |
| 12:10–12:18 | System & environment check | Laptop plugged in; stable internet (phone hotspot as backup); close ALL other apps/tabs; browser updated; quiet room; phone on silent & away; keep a glass of water. |
| 12:18–12:28 | Cheat sheet only | Section 3, one pass. **No new material, no fresh mock.** |
| 12:28–12:33 | Login ready | Open the assessment link **before** 12:33; complete the system scan/consent screens calmly; keep candidate ID/roll no. handy. |
| 12:35 | Start | Clock the timer: 60 min ends 13:35. |
| 12:35–13:30 | Attempt | Easy first pass (~35 min), flagged revisit (~15 min), arithmetic re-check of traced answers (~5 min). Answer everything. |
| 13:30–13:35 | Final review + submit | Only change answers for caught mistakes, not second-guessing nerves. |
| 13:35–15:00 | Buffer | Stay reachable (some assessments issue a link for the interview round shortly after). Deadline for the attempt is 15:00 — do not start later than ~13:45 even in the worst case. |

> If anything delays you: the latest sensible start is **13:45** (60-min timer ends 14:45, inside the 15:00 deadline, but keeps your buffer thin). Avoid starting after that.

---

## 15. Final Assessment-Day Checklist

### Technical
- [ ] Laptop on power + ≥ 30% battery remaining
- [ ] Internet primary + mobile-hotspot fallback tested
- [ ] Updated Chromium/Chrome/Firefox; pop-ups allowed for the test tab
- [ ] All other applications and chat clients closed; do-not-disturb on
- [ ] Quiet room; headphones if the interface needs audio instructions
- [ ] Phone on silent and physically out of the workspace unless proctoring rules require it visible (follow the live instructions — the practice test had proctoring OFF, but the real one may differ)
- [ ] Water within reach; do not leave the tab unless permitted
- [ ] Clock visible (HackerEarth shows its own timer — trust it)

### Strategy while attempting
- [ ] First pass: answer everything you can do in < 2 min; **flag** the rest
- [ ] Trance-on-paper: trace code variable-by-variable; prefer writing a small table
- [ ] Read stems for words like **NOT / EXCEPT / INCORRECT / least**
- [ ] SQL: sketch the join rows before touching options
- [ ] Keep rough math on paper (small trace tables); recompute % changes twice
- [ ] Never leave a blank — no negative marking is disclosed, blanks are already "wrong"
- [ ] Watch the timer distribution you planned (mock-style)

### After submission
- [ ] Screenshot/confirm the submission-success screen when allowed
- [ ] Note actual question count/topics in your preparation log (helps the next round)
- [ ] Send follow-up thanks only if an HR contact exists; otherwise wait politely

---

## 16. Sources & Further Reading (official documentation only)

- Python language & tutorial — https://docs.python.org/3/tutorial/ (see especially: data structures, exceptions, classes & scope)
- Python `copy` module — https://docs.python.org/3/library/copy.html
- Python `round()` (banker's rounding) — https://docs.python.org/3/library/functions.html#round
- pandas user guide (Indexing/loc/iloc; merge; missing data) — https://pandas.pydata.org/pandas-docs/stable/user_guide/indexing.html
- PostgreSQL tutorial: window functions; SELECT; value expressions — https://www.postgresql.org/docs/
- SQLite docs (used to verify this document's SQL output; NULL ordering rules) — https://www.sqlite.org/lang_select.html
- Database normalization overview — https://en.wikipedia.org/wiki/Database_normalization
- HackerEarth assessment help centre — https://support.hackerearth.com/
- Apache Spark SQL & Structured Streaming guide — https://spark.apache.org/docs/latest/sql-programming-guide.html
- Kafka introduction (optional depth) — https://kafka.apache.org/intro

*Every code output, SQL result, and numeric statistic in this file was re-executed/verified on Python 3.13, SQLite 3.51, and pandas 2.3.3 before writing. Engine-specific behaviours are flagged inline where they matter.*

> Good luck — steady tracing beats last-minute theory. Trust your DSA instincts, watch NULLs, watch `is` vs `==`, and sleep tonight. 🚀
