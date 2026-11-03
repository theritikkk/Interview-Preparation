# 23 — SQL for Business and Survey Analytics

**Purpose:** take you from “I have never written SQL” to “I can pull, clean, join, aggregate, and explain commercial and physician-survey data under time pressure.” SQL is the repeatable layer between a raw extract and the Excel/Power BI view a client sees. This module is application-first: every query is attached to a pharma commercial or survey decision.

**Prerequisites:** [04-excel-formulas-cheatsheet.md](04-excel-formulas-cheatsheet.md), [05-excel-practical-workbook-guide.md](05-excel-practical-workbook-guide.md), and [06-statistics-and-survey-analytics.md](06-statistics-and-survey-analytics.md). You do **not** need prior SQL. You do need to be comfortable with rows, columns, keys, and denominators.

**Suggested study time:** 10–14 hours across four sessions, all with the practice database open. Type every query. Break it on purpose. Check the row count before you trust the number.

**Data files (all fictional):**

| File | What it is | Rows |
|---|---|---:|
| `data/sql/trinity_sql_practice.db` | Ready-to-query SQLite database | 10 tables |
| `data/sql/trinity_sql_practice.sql` | SQLite dump; rebuild the database | — |
| `data/sql/csv/*.csv` | One CSV per table, for inspection or another engine | 10 files |
| `data/sql/README.md` | Setup, row counts, and design notes | — |

> **Setup before you start.** Open `data/sql/trinity_sql_practice.db` in DB Browser for SQLite, or run `sqlite3 data/sql/trinity_sql_practice.db` from the pack directory. In the SQLite shell, type `.headers on`, `.mode column`, and `.tables`. The full setup is in `data/sql/README.md`. Every query below has been run against this database.

> **Source labels.** SQL is a **PRACTICE INFERENCE** for this role. The pack lists SQL/database familiarity as a strength ([README.md](README.md)), and module 28 has SQL error categories. The current technical question bank ([11-technical-interview-question-bank.md](11-technical-interview-question-bank.md)) is light on SQL, so this module fills that gap. The exact SQL round, dialect, and difficulty are **not confirmed**. If your interview invitation or recruiter confirms SQL, upgrade this to **CONFIRMED** and still practise the same skills. The 45-minute assessment is a practice simulation, not an official test.

> **All data is fictional.** The company, products, regions, physicians, sales, prescriptions, and survey responses were generated for practice. They are not Trinity data, not real company data, and not indicative of real market size. Revenue and TRx figures are deliberately small enough for mental arithmetic.

---

## 23.0 The data model, grain, and setup

### 23.0.1 What SQL is for — and what it is not

SQL is the language of **questions against tables**. In an analyst role, it usually does four jobs:

1. **Extract** the right rows from a larger database (date window, region, product, respondent type).
2. **Clean** keys, categories, duplicates, and invalid values before analysis.
3. **Aggregate** to the grain the business needs (month, region, product, physician, specialty).
4. **Reconcile** one source against another and explain the difference.

SQL is **not** a replacement for Excel or Power BI. A useful division of labour:

| Layer | Tool | Job |
|---|---|---|
| Raw data | Database | Store rows, keys, dates, transactions |
| Transformation | SQL | Filter, join, dedupe, aggregate, QA |
| Presentation | Excel / Power BI | Pivot, chart, format, annotate, tell the story |
| Decision | You | Interpret, challenge, recommend |

The interview skill is not memorising `SELECT`. It is proving that you know **what one row means**, **whether a join changed the grain**, and **whether the denominator is correct**.

### 23.0.2 The practice database

The database is a small commercial and survey warehouse:

```
regions 1 ───< reps 1 ───< calls >─── 1 physicians
   │                                      │
   └──< sales >── 1 products              ├──< survey 1 ───< survey_items
                        │                 │
                        └──< prescriptions >── 1 products
```

Standalone dimension: `quotas` (one row per awareness stage).

| Table | Rows | Grain | Primary key |
|---|---:|---|---|
| `regions` | 4 | one row per region | `region_id` |
| `products` | 3 | one row per product | `product_id` |
| `reps` | 12 | one row per sales representative | `rep_id` |
| `sales` | 72 | month × region × product | `sale_id` |
| `physicians` | 42 | one row per physician | `physician_id` |
| `survey` | 42 raw | one row per survey response | `response_id` |
| `survey_items` | 126 | response × Likert item | (`response_id`, `item`) |
| `calls` | 120 | one row per rep call | `call_id` |
| `prescriptions` | 276 | physician × product × month with TRx > 0 | `rx_id` |
| `quotas` | 5 | one row per awareness stage | `awareness_stage` |

**Why the row counts matter.** `sales` has 72 rows because 6 months × 4 regions × 3 products = 72. If a query returns 144 rows after joining `regions`, you have a fan-out problem, not a bigger market. `survey` has 42 responses but only 40 unique physicians because two respondents appear twice. `survey_items` has 126 rows because 42 responses × 3 items = 126. `prescriptions` is **not** a full grid: it only stores physician × product × month rows where TRx is positive, and two physicians stop prescribing after April.

### 23.0.3 Data dictionary

**`regions`**

| Column | Type | Meaning |
|---|---|---|
| `region_id` | text | `N`, `S`, `E`, `W` |
| `region_name` | text | North, South, East, West |
| `regional_manager` | text | Fictional manager |
| `hq_city` | text | Fictional HQ city |

**`products`**

| Column | Type | Meaning |
|---|---|---|
| `product_id` | text | `CARD`, `NEUR`, `ONCO` |
| `product_name` | text | CardioX, NeuroY, OncoZ |
| `therapeutic_area` | text | Cardiovascular, Neurology, Oncology |
| `launch_year` | integer | Fictional launch year |

**`reps`**

| Column | Type | Meaning |
|---|---|---|
| `rep_id` | text | Representative ID |
| `rep_name` | text | Fictional name |
| `region_id` | text | FK to `regions` |
| `hire_date` | text | ISO date `YYYY-MM-DD` |

**`sales`**

| Column | Type | Meaning |
|---|---|---|
| `sale_id` | text | Row ID |
| `month` | text | First day of month, ISO `YYYY-MM-DD` |
| `region_id` | text | FK to `regions` |
| `product_id` | text | FK to `products` |
| `units` | integer | Units sold |
| `unit_price` | numeric | Revenue per unit |
| `revenue` | numeric | `units × unit_price` |
| `target_revenue` | numeric, nullable | Target; blank for two rows |
| `calls` | integer, nullable | Call count; blank for three rows |

**`physicians`**

| Column | Type | Meaning |
|---|---|---|
| `physician_id` | text | `P-001` … `P-042` |
| `physician_name` | text | Fictional name |
| `specialty` | text | Raw specialty; one value has a trailing space |
| `region_id` | text | Clean FK to `regions` |
| `years_in_practice` | integer | Years |
| `channel` | text | Email, Phone, In-person |
| `practice_type` | text | Solo, Group, Hospital |

**`survey`**

| Column | Type | Meaning |
|---|---|---|
| `response_id` | text | `SR-001` … `SR-042` |
| `physician_id` | text | FK to `physicians`; duplicates exist |
| `response_date` | text | ISO date |
| `region_raw` | text | Raw region, including `north ` and `N` variants |
| `awareness_stage` | text | Five-stage adoption ladder |
| `recommend_score` | integer | 0–10 NPS question |
| `monthly_trx` | integer, nullable | Monthly TRx; blank for one trial physician |
| `weight` | numeric | Survey weight |

**`survey_items`**

| Column | Type | Meaning |
|---|---|---|
| `response_id` | text | FK to `survey` |
| `item` | text | `Intent`, `Access`, `Efficacy` |
| `score_raw` | text | Raw Likert code, including `7` and `N/A` |

**`calls`**

| Column | Type | Meaning |
|---|---|---|
| `call_id` | text | Call ID |
| `rep_id` | text | FK to `reps` |
| `physician_id` | text | FK to `physicians` |
| `product_id` | text | FK to `products` |
| `call_date` | text | ISO date |
| `call_type` | text | `F2F` or `Remote` |
| `duration_min` | integer | Minutes |

**`prescriptions`**

| Column | Type | Meaning |
|---|---|---|
| `rx_id` | text | Row ID |
| `physician_id` | text | FK to `physicians` |
| `product_id` | text | FK to `products` |
| `month` | text | First day of month |
| `trx` | integer | Total prescriptions |
| `new_rx` | integer | New prescriptions; `new_rx <= trx` |

**`quotas`**

| Column | Type | Meaning |
|---|---|---|
| `awareness_stage` | text | Stage name |
| `target_n` | integer | Target completes per stage |

### 23.0.4 Grain, keys, and cardinality

Before writing a query, answer three questions:

1. **What is one row?** In `sales`, one row is a month × region × product. In `prescriptions`, one row is a physician × product × month with positive TRx. In `survey`, one row is a response, not a physician.
2. **Is the key unique?** `physician_id` is unique in `physicians`; it is not unique in `survey` (duplicates). `response_id` is unique in `survey`; it is not unique in `survey_items` (three rows per response).
3. **What happens when I join?** One-to-many joins multiply rows. A join is safe only when you know the cardinality on both sides.

| Relationship | Cardinality | Example |
|---|---|---|
| `regions` → `sales` | one-to-many | One region has many sales rows |
| `products` → `sales` | one-to-many | One product has many sales rows |
| `physicians` → `survey` | one-to-many | One physician can have multiple responses |
| `survey` → `survey_items` | one-to-many | One response has three items |
| `reps` → `calls` | one-to-many | One rep has many calls |
| `physicians` → `prescriptions` | one-to-many | One physician has many Rx rows |

### 23.0.5 Setup: three ways

**Option A — DB Browser for SQLite (GUI).** Open `data/sql/trinity_sql_practice.db`, choose **Execute SQL**, and paste queries. This is the easiest option if you are new to the command line.

**Option B — SQLite CLI.**

```bash
cd "Companies/Trinity Associate Analyst/trinity-associate-analyst-preparation"
sqlite3 data/sql/trinity_sql_practice.db
```

Then:

```text
.headers on
.mode column
.tables
.schema sales
SELECT COUNT(*) FROM sales;
.quit
```

**Option C — rebuild from the dump.**

```bash
sqlite3 /tmp/trinity_sql_practice.db < data/sql/trinity_sql_practice.sql
```

**If you use PostgreSQL, MySQL, or SQL Server.** Load the CSVs in `data/sql/csv/` into your engine. The queries in this module are standard SQL except for SQLite-specific date and string functions; 23.12 maps the differences. Do not spend time fighting a database install. SQLite is enough for the interview patterns.

### 23.0.6 SQL execution order

SQL is written in one order and executed in another. This is the single most useful thing to memorise:

| Logical step | Clause | What it does |
|---:|---|---|
| 1 | `FROM` / `JOIN` | Build the working row set |
| 2 | `WHERE` | Filter rows **before** aggregation |
| 3 | `GROUP BY` | Collapse rows into groups |
| 4 | `HAVING` | Filter groups **after** aggregation |
| 5 | `SELECT` | Choose columns, aliases, window functions |
| 6 | `DISTINCT` | Remove duplicate result rows |
| 7 | `ORDER BY` | Sort the result |
| 8 | `LIMIT` | Keep the first *n* rows |

Consequences:

- You cannot use a `SELECT` alias in `WHERE` in most engines; use a CTE or repeat the expression.
- `WHERE` cannot filter an aggregate; use `HAVING`.
- Window functions are evaluated after `WHERE`, `GROUP BY`, and `HAVING`, but before `ORDER BY`. You cannot put `ROW_NUMBER()` directly in `WHERE`; wrap it in a CTE and filter outside.
- `LIMIT` without `ORDER BY` is not a “top” query; it is an arbitrary sample.

### 23.0.7 Style rules for analyst SQL

Use these from day one. They make your SQL reviewable and reduce silent errors.

```sql
-- Good: one clause per line, explicit joins, clear aliases
SELECT
    r.region_name,
    SUM(s.revenue) AS revenue,
    ROUND(100.0 * SUM(s.revenue) / NULLIF(SUM(s.target_revenue), 0), 2) AS achievement_pct
FROM sales AS s
JOIN regions AS r
    ON r.region_id = s.region_id
WHERE s.month >= '2026-04-01'
  AND s.month <  '2026-07-01'
GROUP BY r.region_name
ORDER BY revenue DESC;
```

Rules:

1. **Uppercase keywords, lowercase identifiers.** `SELECT`, `FROM`, `GROUP BY`; `region_name`, `sales`.
2. **Alias every table.** `sales AS s`, not `sales s` alone if the query is long.
3. **Use explicit `JOIN … ON`.** Avoid `FROM a, b WHERE a.id = b.id`; it hides the join.
4. **Never use `SELECT *` in a final answer.** Name the columns.
5. **Qualify ambiguous columns.** `s.revenue`, not `revenue`.
6. **Comment the grain.** `-- one row per region` is a useful guard.
7. **Use `NULLIF` for division.** `SUM(x) / NULLIF(SUM(y), 0)` avoids divide-by-zero.
8. **Use `1.0 *` for percentages.** Integer division can silently truncate.
9. **Name CTEs after the grain.** `monthly_rx`, `clean_survey`, `region_product`, not `cte1`.
10. **Check row counts before and after joins.** A number is not trustworthy until the grain is.

---
## Part A — Foundations

### 23.1 SELECT: ask for columns, not “everything”

`SELECT` chooses columns. `FROM` chooses the table. `ORDER BY` sorts. `LIMIT` keeps the first rows.

```sql
SELECT region_name, regional_manager, hq_city
FROM regions
ORDER BY region_name;
```

**Expected output (4 rows):**

| region_name | regional_manager | hq_city |
|---|---|---|
| East | Farah Khan | Kolkata |
| North | Meera Kapoor | Delhi |
| South | Arjun Iyer | Chennai |
| West | Nikhil Desai | Mumbai |

```sql
SELECT product_name, therapeutic_area, launch_year
FROM products
ORDER BY launch_year;
```

| product_name | therapeutic_area | launch_year |
|---|---|---:|
| CardioX | Cardiovascular | 2018 |
| NeuroY | Neurology | 2021 |
| OncoZ | Oncology | 2024 |

**`SELECT *` is for exploration, not for answers.** In an interview, `SELECT *` tells the interviewer you have not decided what the question needs. It also breaks when a table gains a column. Use it once to inspect a table, then name the columns.

#### 23.1.1 DISTINCT is not a cleaning tool

```sql
SELECT DISTINCT specialty
FROM physicians
ORDER BY specialty;
```

This returns **five** raw values:

```text
Cardiologist
Neurologist
Oncologist
Oncologist      ← trailing space
Primary Care
```

The two “Oncologist” values are different strings. If you group by `specialty` without cleaning, you split oncology into two groups. The fix is `TRIM`:

```sql
SELECT DISTINCT TRIM(specialty) AS specialty
FROM physicians
ORDER BY specialty;
```

Now there are four: Cardiologist, Neurologist, Oncologist, Primary Care.

**Lesson:** `DISTINCT` shows you the dirt; it does not remove it. Clean the value, then aggregate.

#### 23.1.2 LIMIT is not “top”

```sql
SELECT physician_id, physician_name, specialty, region_id
FROM physicians
ORDER BY physician_id
LIMIT 5;
```

| physician_id | physician_name | specialty | region_id |
|---|---|---|---|
| P-001 | Dr. Aarav Agarwal | Cardiologist | N |
| P-002 | Dr. Aditi Gupta | Neurologist | S |
| P-003 | Dr. Akash Mehta | Oncologist | E |
| P-004 | Dr. Ananya Roy | Primary Care | W |
| P-005 | Dr. Arjun Vora | Cardiologist | N |

`LIMIT 5` without `ORDER BY` returns five arbitrary rows. For a top-5 question, always specify the sort:

```sql
SELECT sale_id, month, region_id, product_id, revenue
FROM sales
ORDER BY revenue DESC
LIMIT 5;
```

| sale_id | month | region_id | product_id | revenue |
|---|---|---|---|---:|
| S-063 | 2026-06-01 | N | ONCO | 70,080 |
| S-061 | 2026-06-01 | N | CARD | 66,276 |
| S-049 | 2026-05-01 | N | CARD | 58,338 |
| S-066 | 2026-06-01 | S | ONCO | 58,080 |
| S-051 | 2026-05-01 | N | ONCO | 57,600 |

Notice that the largest **row** is not the largest **product** or **region**. A row is one month × region × product. A top-5 row list is a different question from a top-5 product list. Always state the grain before ranking. 
#### Check yourself — 23.1

1. How many rows are in `products`? Write the query.
2. Why does `SELECT DISTINCT specialty FROM physicians` return five values when there are only four specialties?
3. Write a query that returns the three products from newest to oldest launch year.
4. Why is `LIMIT 5` without `ORDER BY` not a top-5 query?

Answers are in 23.15.

---

### 23.2 NULLs, filtering, and boolean logic

`WHERE` filters rows. Most interview mistakes are not syntax errors; they are **filter logic** errors.

#### 23.2.1 NULL is “unknown”, not zero and not blank

```sql
SELECT
    COUNT(*) AS all_rows,
    COUNT(target_revenue) AS non_null_targets,
    COUNT(*) - COUNT(target_revenue) AS null_targets
FROM sales;
```

| all_rows | non_null_targets | null_targets |
|---:|---:|---:|
| 72 | 70 | 2 |

`COUNT(*)` counts rows. `COUNT(column)` counts **non-NULL** values. This is the cleanest way to profile a nullable column.

```sql
SELECT sale_id, month, region_id, product_id, revenue
FROM sales
WHERE target_revenue IS NULL
ORDER BY sale_id;
```

| sale_id | month | region_id | product_id | revenue |
|---|---|---|---|---:|
| S-009 | 2026-01-01 | E | ONCO | 14,400 |
| S-012 | 2026-01-01 | W | ONCO | 19,200 |

Do **not** write `WHERE target_revenue = NULL`. Nothing equals NULL, not even NULL. Use `IS NULL` or `IS NOT NULL`.

#### 23.2.2 The `<>` trap

```sql
SELECT COUNT(*) AS rows_with_nonzero_target
FROM sales
WHERE target_revenue <> 0;
```

Result: **70**, not 72. The two NULL rows are excluded because `NULL <> 0` evaluates to unknown, and `WHERE` keeps only rows where the condition is true. If the business question is “all rows except zero targets, including blanks”, write:

```sql
SELECT COUNT(*) AS rows_except_zero
FROM sales
WHERE target_revenue <> 0 OR target_revenue IS NULL;
```

Result: **72** (there are no zero targets, only blanks). Better still, make the intent explicit:

```sql
SELECT COUNT(*) AS rows_except_zero
FROM sales
WHERE COALESCE(target_revenue, 0) <> 0;
```

#### 23.2.3 COALESCE and NULLIF

`COALESCE(a, b, c)` returns the first non-NULL value. It is useful for display, not for silently changing the denominator.

```sql
SELECT
    sale_id,
    revenue,
    COALESCE(target_revenue, 0) AS target_display,
    revenue - COALESCE(target_revenue, 0) AS variance_display
FROM sales
WHERE target_revenue IS NULL;
```

| sale_id | revenue | target_display | variance_display |
|---|---:|---:|---:|
| S-009 | 14,400 | 0 | 14,400 |
| S-012 | 19,200 | 0 | 19,200 |

That variance is **not** a real overachievement. A blank target means “no target”, not “target = 0”. For achievement, exclude blanks or show “n/a”:

```sql
SELECT
    ROUND(100.0 * SUM(revenue) / NULLIF(SUM(target_revenue), 0), 2) AS achievement_pct
FROM sales
WHERE target_revenue IS NOT NULL;
```

`NULLIF(SUM(target_revenue), 0)` returns NULL if the denominator is zero, avoiding a divide-by-zero error. `COALESCE` is for presentation; `NULLIF` is for safe division. Do not confuse them.

#### 23.2.4 Boolean precedence and parentheses

`AND` binds tighter than `OR`. This:

```sql
WHERE region_id = 'N' OR region_id = 'S' AND product_id = 'CARD'
```

means `N OR (S AND CARD)`, not `(N OR S) AND CARD`. Use parentheses:

```sql
WHERE (region_id = 'N' OR region_id = 'S')
  AND product_id = 'CARD';
```

#### 23.2.5 BETWEEN, IN, and LIKE

```sql
-- Q2 2026: inclusive on both ends because dates are ISO text
SELECT COUNT(*) AS q2_rows
FROM sales
WHERE month BETWEEN '2026-04-01' AND '2026-06-01';
```

Result: **36** rows (3 months × 4 regions × 3 products).

```sql
-- Two products
SELECT COUNT(*) AS cardio_or_onco_rows
FROM sales
WHERE product_id IN ('CARD', 'ONCO');
```

Result: **48** rows (2 products × 6 months × 4 regions).

```sql
-- Names starting with "Dr. A"
SELECT physician_id, physician_name
FROM physicians
WHERE physician_name LIKE 'Dr. A%'
ORDER BY physician_id;
```

`LIKE` is case-insensitive for ASCII in SQLite by default. In PostgreSQL, `LIKE` is case-sensitive; use `ILIKE`. In SQL Server, the default collation usually makes it case-insensitive. Do not rely on default case behaviour in an interview; state it.

#### 23.2.6 The `NOT IN` + NULL trap

This is a classic:

```sql
SELECT product_name
FROM products
WHERE product_id NOT IN (
    SELECT product_id
    FROM sales
    WHERE target_revenue IS NULL
);
```

If the subquery returned a NULL, the whole `NOT IN` could evaluate to unknown and return **no rows**. In this dataset it returns the products not associated with blank targets, but the pattern is fragile. Prefer `NOT EXISTS`:

```sql
SELECT p.product_name
FROM products AS p
WHERE NOT EXISTS (
    SELECT 1
    FROM sales AS s
    WHERE s.product_id = p.product_id
      AND s.target_revenue IS NULL
);
```

`NOT EXISTS` is NULL-safe and usually easier to reason about.

#### Check yourself — 23.2

1. How many `sales` rows have a blank `calls` value? Write the query.
2. What does `COUNT(calls)` return, and why is it not 72?
3. Why does `WHERE calls <> 0` return 69 rows rather than 72?
4. Rewrite `WHERE target_revenue = NULL` correctly.
5. What is the difference between `COALESCE(target_revenue, 0)` and `NULLIF(target_revenue, 0)`?

Answers are in 23.15.

---

### 23.3 Aggregation: GROUP BY and HAVING

Aggregation collapses many rows into one row per group. The three questions are always:

1. **What is the group?** Region? Product? Month? Physician?
2. **What is the measure?** Sum, count, average, min, max?
3. **What is the denominator?** Rows, units, target, respondents, weighted base?

#### 23.3.1 The headline KPI

```sql
SELECT
    COUNT(*) AS rows,
    SUM(revenue) AS revenue,
    SUM(target_revenue) AS target,
    ROUND(100.0 * SUM(revenue) / NULLIF(SUM(target_revenue), 0), 2) AS achievement_pct
FROM sales;
```

| rows | revenue | target | achievement_pct |
|---:|---:|---:|---:|
| 72 | 2,683,320 | 2,717,035 | 98.76 |

This is the whole-business view. It is not a region view, a product view, or a monthly view. The grain of the result is “all sales rows”. If you add a column that is not aggregated and not in `GROUP BY`, you will either get an error or an arbitrary value.

#### 23.3.2 Group by region

```sql
SELECT
    r.region_name,
    SUM(s.revenue) AS revenue,
    SUM(s.target_revenue) AS target,
    ROUND(100.0 * SUM(s.revenue) / NULLIF(SUM(s.target_revenue), 0), 2) AS achievement_pct
FROM sales AS s
JOIN regions AS r
    ON r.region_id = s.region_id
GROUP BY r.region_name
ORDER BY revenue DESC;
```

| region_name | revenue | target | achievement_pct |
|---|---:|---:|---:|
| North | 864,954 | 886,608 | 97.56 |
| South | 724,284 | 747,844 | 96.85 |
| West | 612,882 | 622,128 | 98.51 |
| East | 481,200 | 460,455 | 104.51 |

**Read it like an analyst.** North is the largest region but under target. East is the smallest region but the only one above target. A single “98.76% overall” hides that split. The next question is whether East’s overachievement is broad or concentrated in one product/cell.

#### 23.3.3 Group by product

```sql
SELECT
    p.product_name,
    SUM(s.revenue) AS revenue,
    SUM(s.target_revenue) AS target,
    ROUND(100.0 * SUM(s.revenue) / NULLIF(SUM(s.target_revenue), 0), 2) AS achievement_pct
FROM sales AS s
JOIN products AS p
    ON p.product_id = s.product_id
GROUP BY p.product_name
ORDER BY revenue DESC;
```

| product_name | revenue | target | achievement_pct |
|---|---:|---:|---:|
| CardioX | 1,051,470 | 1,117,831 | 94.06 |
| OncoZ | 842,880 | 804,121 | 104.82 |
| NeuroY | 788,970 | 795,083 | 99.23 |

CardioX is the largest product but 5.94 points below target. OncoZ is the only product above target. NeuroY is essentially on target. The portfolio result is a mix effect: the largest product is dragging the total down.

#### 23.3.4 Group by month

```sql
SELECT
    month,
    SUM(revenue) AS revenue,
    SUM(target_revenue) AS target,
    ROUND(100.0 * SUM(revenue) / NULLIF(SUM(target_revenue), 0), 2) AS achievement_pct
FROM sales
GROUP BY month
ORDER BY month;
```

| month | revenue | target | achievement_pct |
|---|---:|---:|---:|
| 2026-01-01 | 378,000 | 360,696 | 104.80 |
| 2026-02-01 | 397,470 | 429,695 | 92.50 |
| 2026-03-01 | 424,980 | 417,314 | 101.84 |
| 2026-04-01 | 454,770 | 477,543 | 95.23 |
| 2026-05-01 | 487,800 | 497,178 | 98.11 |
| 2026-06-01 | 540,300 | 534,609 | 101.06 |

The month view shows a different story from the total: January and March were above target, February and April were below, and June recovered. A quarterly view would smooth this; a monthly view shows the execution pattern. Neither is “wrong”; they answer different questions.

#### 23.3.5 HAVING filters groups

```sql
SELECT
    r.region_name,
    SUM(s.revenue) AS revenue
FROM sales AS s
JOIN regions AS r
    ON r.region_id = s.region_id
GROUP BY r.region_name
HAVING SUM(s.revenue) > 600000
ORDER BY revenue DESC;
```

| region_name | revenue |
|---|---:|
| North | 864,954 |
| South | 724,284 |
| West | 612,882 |

`WHERE` would filter **rows** before aggregation; `HAVING` filters **groups** after aggregation. You cannot write `WHERE SUM(revenue) > 600000`.

#### 23.3.6 COUNT, COUNT(DISTINCT), and average

```sql
SELECT
    COUNT(*) AS sales_rows,
    COUNT(DISTINCT region_id) AS regions,
    COUNT(DISTINCT product_id) AS products,
    COUNT(DISTINCT month) AS months,
    ROUND(AVG(revenue), 2) AS avg_row_revenue
FROM sales;
```

| sales_rows | regions | products | months | avg_row_revenue |
|---:|---:|---:|---:|---:|
| 72 | 4 | 3 | 6 | 37,268.33 |

`AVG(revenue)` is the average **row** revenue, not the average monthly revenue, average regional revenue, or average product revenue. If you want the average monthly revenue, aggregate to month first, then average:

```sql
WITH monthly AS (
    SELECT month, SUM(revenue) AS revenue
    FROM sales
    GROUP BY month
)
SELECT ROUND(AVG(revenue), 2) AS avg_monthly_revenue
FROM monthly;
```

Result: **447,220.00** (2,683,320 / 6).

This is the “average of averages” trap. The average of monthly totals is not the average of rows unless every month has the same number of rows. Here it does, but the principle still matters.

#### 23.3.7 Integer division

In SQLite, `SUM(revenue) / SUM(target_revenue)` with two integers can return an integer. Always multiply by `1.0`:

```sql
ROUND(100.0 * SUM(revenue) / NULLIF(SUM(target_revenue), 0), 2)
```

In PostgreSQL, integer division also truncates. In MySQL, `DECIMAL` behaviour depends on the types. Make the cast explicit:

```sql
ROUND(100.0 * SUM(revenue)::numeric / NULLIF(SUM(target_revenue), 0), 2)
```

#### Check yourself — 23.3

1. What is total unit volume? (Hint: `SUM(units)`.)
2. Which month has the highest revenue?
3. Which region has the highest achievement?
4. How many sales rows are at or above target? What is the denominator?
5. What is the average monthly revenue, and why is it not the same as the average row revenue?

Answers are in 23.15.

---
## Part B — Combining tables

### 23.4 Joins and cardinality

A join combines rows from two tables using a key. The syntax is easy. The risk is **row multiplication**. Before you join, ask:

1. What is the key on the left?
2. Is it unique on the right?
3. What is the expected row count after the join?
4. What should happen to unmatched rows?

#### 23.4.1 The safe join

```sql
SELECT
    s.sale_id,
    s.month,
    r.region_name,
    p.product_name,
    s.units,
    s.revenue,
    s.target_revenue
FROM sales AS s
JOIN regions AS r
    ON r.region_id = s.region_id
JOIN products AS p
    ON p.product_id = s.product_id
ORDER BY s.sale_id
LIMIT 5;
```

`regions` and `products` are unique on their keys, so the join is **one-to-many from the dimension to the fact**. The result has the same 72 rows as `sales`.

**Cardinality check:**

```sql
SELECT
    (SELECT COUNT(*) FROM sales) AS sales_rows,
    (SELECT COUNT(*) FROM sales s JOIN regions r ON r.region_id = s.region_id) AS after_region,
    (SELECT COUNT(*) FROM sales s JOIN products p ON p.product_id = s.product_id) AS after_product;
```

| sales_rows | after_region | after_product |
|---:|---:|---:|
| 72 | 72 | 72 |

If `after_region` were 144, the join key would not be unique in `regions` (or the join condition would be wrong). Always run a count check before you debug the numbers.

#### 23.4.2 The region × product matrix

```sql
SELECT
    r.region_name,
    p.product_name,
    SUM(s.revenue) AS revenue
FROM sales AS s
JOIN regions AS r
    ON r.region_id = s.region_id
JOIN products AS p
    ON p.product_id = s.product_id
GROUP BY r.region_name, p.product_name
ORDER BY r.region_name, p.product_name;
```

**Expected output (12 rows):**

| region_name | product_name | revenue |
|---|---|---:|
| East | CardioX | 192,510 |
| East | NeuroY | 148,050 |
| East | OncoZ | 140,640 |
| North | CardioX | 337,134 |
| North | NeuroY | 246,540 |
| North | OncoZ | 281,280 |
| South | CardioX | 280,944 |
| South | NeuroY | 209,580 |
| South | OncoZ | 233,760 |
| West | CardioX | 240,882 |
| West | NeuroY | 184,800 |
| West | OncoZ | 187,200 |

The top cell is **North × CardioX = 337,134**. That is 12.56% of total revenue and 38.98% of North revenue. A region total alone would hide this concentration.

#### 23.4.3 LEFT JOIN and anti-joins

A `LEFT JOIN` keeps every row from the left table, even when there is no match on the right. This is how you find **missing** data.

```sql
SELECT p.physician_id, p.physician_name
FROM physicians AS p
LEFT JOIN calls AS c
    ON c.physician_id = p.physician_id
WHERE c.call_id IS NULL
ORDER BY p.physician_id;
```

| physician_id | physician_name |
|---|---|
| P-041 | Dr. Dev Malhotra |
| P-042 | Dr. Ritu Bansal |

These two physicians are in the dimension but have no calls. They are also the two historical prescribers who stop after April. The anti-join finds them without a subquery.

The equivalent `NOT EXISTS`:

```sql
SELECT p.physician_id, p.physician_name
FROM physicians AS p
WHERE NOT EXISTS (
    SELECT 1
    FROM calls AS c
    WHERE c.physician_id = p.physician_id
)
ORDER BY p.physician_id;
```

**Products with no sales:**

```sql
SELECT p.product_name
FROM products AS p
LEFT JOIN sales AS s
    ON s.product_id = p.product_id
WHERE s.sale_id IS NULL;
```

Result: **0 rows**. Every product has sales. A zero-row result is a valid answer; do not “fix” it by changing the join.

#### 23.4.4 The LEFT JOIN filter trap

This keeps all physicians and then filters to those with no calls:

```sql
SELECT p.physician_id
FROM physicians AS p
LEFT JOIN calls AS c
    ON c.physician_id = p.physician_id
WHERE c.call_id IS NULL;
```

This **does not** keep all physicians:

```sql
SELECT p.physician_id
FROM physicians AS p
LEFT JOIN calls AS c
    ON c.physician_id = p.physician_id
WHERE c.call_type = 'F2F';
```

The `WHERE` filter on the right table removes the unmatched rows, effectively turning the `LEFT JOIN` into an `INNER JOIN`. If you want to filter the right side but keep unmatched left rows, put the condition in the `ON` clause:

```sql
SELECT p.physician_id, c.call_id
FROM physicians AS p
LEFT JOIN calls AS c
    ON c.physician_id = p.physician_id
   AND c.call_type = 'F2F';
```

#### 23.4.5 Self-join

A self-join joins a table to itself. Use it for pairs, hierarchies, or comparisons.

```sql
SELECT
    a.region_id,
    a.rep_name AS rep_a,
    b.rep_name AS rep_b
FROM reps AS a
JOIN reps AS b
    ON a.region_id = b.region_id
   AND a.rep_id < b.rep_id
ORDER BY a.region_id, a.rep_name, b.rep_name;
```

There are three reps per region, so each region produces three pairs; total **12 pairs**. The `a.rep_id < b.rep_id` condition avoids duplicate pairs and self-pairs.

#### 23.4.6 Cross join: build a scaffold

A cross join returns every combination. It is useful for a complete matrix, even when some combinations have no rows.

```sql
SELECT
    r.region_name,
    p.product_name,
    COALESCE(SUM(s.revenue), 0) AS revenue
FROM regions AS r
CROSS JOIN products AS p
LEFT JOIN sales AS s
    ON s.region_id = r.region_id
   AND s.product_id = p.product_id
GROUP BY r.region_name, p.product_name
ORDER BY r.region_name, p.product_name;
```

This returns **12 rows** even if a region-product combination had no sales. In this dataset all 12 cells have sales, but the pattern is essential for a complete matrix.

#### 23.4.7 Fan-out: the join that changes the answer

```sql
WITH clean AS (
    SELECT s.*
    FROM survey AS s
    JOIN (
        SELECT physician_id, MIN(response_id) AS keep_id
        FROM survey
        GROUP BY physician_id
    ) AS d
        ON d.keep_id = s.response_id
    WHERE s.response_date BETWEEN '2026-01-15' AND '2026-03-31'
)
SELECT
    COUNT(*) AS joined_rows,
    COUNT(DISTINCT c.response_id) AS responses,
    ROUND(SUM(c.weight), 2) AS weight_sum
FROM clean AS c
JOIN survey_items AS i
    ON i.response_id = c.response_id;
```

| joined_rows | responses | weight_sum |
|---:|---:|---:|
| 117 | 39 | 127.80 |

The clean base has 39 responses and a weight sum of **42.60**. After joining to three items per response, the row count is 117 and the weight sum is **127.80** — exactly three times too large. If you average a respondent-level variable after this join, you weight each respondent three times. Fix it by aggregating items first, or by using `COUNT(DISTINCT)` / a separate CTE at the correct grain.

**Rule:** a join is not “safe” because it runs. It is safe when the row count and the denominator still mean what you think they mean.

#### Check yourself — 23.4

1. How many rows does `sales JOIN regions USING(region_id)` return? Why?
2. How many rows does `survey JOIN survey_items USING(response_id)` return? Why?
3. Which physicians have no calls? Write the anti-join.
4. Why does a `WHERE` filter on the right table turn a `LEFT JOIN` into an `INNER JOIN`?
5. What is the fan-out risk of joining `survey` to `survey_items` before computing an average?

Answers are in 23.15.

---

### 23.5 Subqueries and CTEs

A subquery is a query inside another query. A CTE (Common Table Expression) is a named subquery at the top of the statement. CTEs are usually easier to read, debug, and reuse.

#### 23.5.1 Scalar subquery

```sql
SELECT sale_id, month, region_id, product_id, revenue
FROM sales
WHERE revenue > (SELECT AVG(revenue) FROM sales)
ORDER BY revenue DESC;
```

The subquery returns one number: the overall average row revenue, **37,268.33**. The outer query returns **32 rows** above that average.

This is not “above-average performance” in a business sense; it is above the average **row**. A large region-product-month row can be above average simply because the cell is large. State the denominator.

#### 23.5.2 IN subquery

```sql
SELECT p.product_name, SUM(s.revenue) AS q2_revenue
FROM sales AS s
JOIN products AS p
    ON p.product_id = s.product_id
WHERE s.month BETWEEN '2026-04-01' AND '2026-06-01'
GROUP BY p.product_name
HAVING SUM(s.revenue) > 500000
ORDER BY q2_revenue DESC;
```

| product_name | q2_revenue |
|---|---:|
| CardioX | 563,790 |
| OncoZ | 531,840 |

NeuroY is below the 500,000 threshold. This is a `HAVING` filter, not a `WHERE` filter, because the condition applies to the aggregate.

#### 23.5.3 EXISTS

```sql
SELECT COUNT(*) AS physicians_with_calls
FROM physicians AS p
WHERE EXISTS (
    SELECT 1
    FROM calls AS c
    WHERE c.physician_id = p.physician_id
);
```

Result: **40**. The two non-respondent physicians (P-041, P-042) have no calls.

`EXISTS` stops at the first match; it does not care how many matching rows there are. That makes it useful for “has at least one” questions.

#### 23.5.4 Correlated subquery

A correlated subquery references the outer query. This one compares each sales row with its **region** average:

```sql
SELECT s.sale_id, s.region_id, s.product_id, s.revenue
FROM sales AS s
WHERE s.revenue > (
    SELECT AVG(s2.revenue)
    FROM sales AS s2
    WHERE s2.region_id = s.region_id
)
ORDER BY s.region_id, s.revenue DESC;
```

Result: **34 rows** above their region average. The subquery runs conceptually once per outer row, so it is slower than a window function on large data. For an interview, either is acceptable; for production, prefer the window function.

The window equivalent:

```sql
WITH x AS (
    SELECT
        s.*,
        AVG(s.revenue) OVER (PARTITION BY s.region_id) AS region_avg
    FROM sales AS s
)
SELECT sale_id, region_id, product_id, revenue
FROM x
WHERE revenue > region_avg
ORDER BY region_id, revenue DESC;
```

#### 23.5.5 CTEs: name the step

The clean survey base is used repeatedly in this module. Put it in a CTE:

```sql
WITH dedup AS (
    SELECT
        physician_id,
        MIN(response_id) AS keep_id
    FROM survey
    GROUP BY physician_id
),
clean_survey AS (
    SELECT s.*
    FROM survey AS s
    JOIN dedup AS d
        ON d.keep_id = s.response_id
    WHERE s.response_date BETWEEN '2026-01-15' AND '2026-03-31'
)
SELECT COUNT(*) AS clean_n
FROM clean_survey;
```

Result: **39**.

Why `MIN(response_id)`? Both duplicate rows have the same physician and the same values; keeping the first response ID is a deterministic rule. In a real dataset, you would define the rule explicitly: latest response, highest weight, first complete response, etc. The rule must be documented.

The same CTE can be referenced multiple times:

```sql
WITH dedup AS (
    SELECT physician_id, MIN(response_id) AS keep_id
    FROM survey
    GROUP BY physician_id
),
clean_survey AS (
    SELECT s.*
    FROM survey AS s
    JOIN dedup AS d
        ON d.keep_id = s.response_id
    WHERE s.response_date BETWEEN '2026-01-15' AND '2026-03-31'
)
SELECT
    (SELECT COUNT(*) FROM clean_survey) AS clean_n,
    (SELECT SUM(monthly_trx) FROM clean_survey) AS total_trx,
    (SELECT ROUND(SUM(weight), 2) FROM clean_survey) AS weight_sum;
```

| clean_n | total_trx | weight_sum |
|---:|---:|---:|
| 39 | 344 | 42.60 |

#### 23.5.6 CTE vs subquery vs temp table

| Technique | Best for | Caution |
|---|---|---|
| Scalar subquery | One-number comparison | Runs per row if correlated |
| `IN` / `EXISTS` | Membership / existence tests | `NOT IN` + NULL trap |
| Derived table | One-off intermediate step | Hard to reuse |
| CTE | Multi-step logic, readability, reuse | Some engines materialise; check performance |
| Temp table | Very large intermediate result reused many times | More setup; not always allowed in interviews |

For an interview, a CTE is usually the best communication tool. It lets you say: “First I dedupe the survey, then I restrict the field window, then I aggregate.”

#### Check yourself — 23.5

1. How many sales rows are above the overall average row revenue?
2. How many sales rows are above their region average?
3. What is the clean survey base, and why is it 39 rather than 42?
4. Which products have Q2 revenue above 500,000?
5. Why is `NOT EXISTS` safer than `NOT IN` when the subquery can return NULL?

Answers are in 23.15.

---
## Part C — Analytical SQL

### 23.6 Window functions: analytics without collapsing rows

A window function performs a calculation across a set of rows **without collapsing them**. This is the difference between:

- `GROUP BY region` → one row per region.
- `SUM(revenue) OVER (PARTITION BY region)` → every sales row, with the region total attached.

Window functions are how you do rankings, running totals, month-over-month growth, moving averages, percent of total, and “top N per group” in SQL.

#### 23.6.1 The syntax

```sql
function_name(...) OVER (
    PARTITION BY ...
    ORDER BY ...
    ROWS BETWEEN ... AND ...
)
```

| Part | Meaning | Example |
|---|---|---|
| `PARTITION BY` | Reset the calculation for each group | `PARTITION BY region_id` |
| `ORDER BY` | Order within the partition | `ORDER BY month` |
| Frame | Which rows the function sees | `ROWS BETWEEN 2 PRECEDING AND CURRENT ROW` |

If you omit `PARTITION BY`, the window is the whole result. If you include `ORDER BY` for an aggregate, the default frame is `RANGE BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW` in most engines. For running totals and moving averages, write the frame explicitly; it is easier to review.

#### 23.6.2 Rank products by revenue

```sql
SELECT
    p.product_name,
    SUM(s.revenue) AS revenue,
    RANK() OVER (ORDER BY SUM(s.revenue) DESC) AS revenue_rank
FROM sales AS s
JOIN products AS p
    ON p.product_id = s.product_id
GROUP BY p.product_name
ORDER BY revenue_rank;
```

| product_name | revenue | revenue_rank |
|---|---:|---:|
| CardioX | 1,051,470 | 1 |
| OncoZ | 842,880 | 2 |
| NeuroY | 788,970 | 3 |

The `ORDER BY` inside `OVER` is separate from the final `ORDER BY`. The window rank is computed after aggregation; the final sort only displays the result.

#### 23.6.3 Top 3 region-product cells

```sql
WITH cells AS (
    SELECT
        r.region_name,
        p.product_name,
        SUM(s.revenue) AS revenue
    FROM sales AS s
    JOIN regions AS r
        ON r.region_id = s.region_id
    JOIN products AS p
        ON p.product_id = s.product_id
    GROUP BY r.region_name, p.product_name
),
ranked AS (
    SELECT
        region_name,
        product_name,
        revenue,
        ROW_NUMBER() OVER (ORDER BY revenue DESC) AS rn
    FROM cells
)
SELECT region_name, product_name, revenue, rn
FROM ranked
WHERE rn <= 3
ORDER BY rn;
```

| region_name | product_name | revenue | rn |
|---|---|---:|---:|
| North | CardioX | 337,134 | 1 |
| North | OncoZ | 281,280 | 2 |
| South | CardioX | 280,944 | 3 |

You cannot put `ROW_NUMBER()` directly in `WHERE`; the window is evaluated after `WHERE`. The CTE is not optional decoration; it is the correct logical order.

#### 23.6.4 Top 3 physicians per specialty

This is the classic “top N per group” pattern.

```sql
WITH june AS (
    SELECT
        ph.physician_id,
        ph.physician_name,
        TRIM(ph.specialty) AS specialty,
        SUM(r.trx) AS trx
    FROM prescriptions AS r
    JOIN physicians AS ph
        ON ph.physician_id = r.physician_id
    WHERE r.month = '2026-06-01'
    GROUP BY ph.physician_id, ph.physician_name, TRIM(ph.specialty)
),
ranked AS (
    SELECT
        specialty,
        physician_name,
        trx,
        ROW_NUMBER() OVER (
            PARTITION BY specialty
            ORDER BY trx DESC, physician_id
        ) AS rn
    FROM june
)
SELECT specialty, rn, physician_name, trx
FROM ranked
WHERE rn <= 3
ORDER BY specialty, rn;
```

**Expected output (12 rows):**

| specialty | rn | physician_name | trx |
|---|---:|---|---:|
| Cardiologist | 1 | Dr. Varun Kulkarni | 36 |
| Cardiologist | 2 | Dr. Shreya Mehta | 20 |
| Cardiologist | 3 | Dr. Rahul Pillai | 8 |
| Neurologist | 1 | Dr. Vikram Rao | 40 |
| Neurologist | 2 | Dr. Siddharth Roy | 24 |
| Neurologist | 3 | Dr. Ravi Singh | 11 |
| Oncologist | 1 | Dr. Yash Sinha | 44 |
| Oncologist | 2 | Dr. Sneha Vora | 28 |
| Oncologist | 3 | Dr. Riya Bose | 14 |
| Primary Care | 1 | Dr. Zoya Chandra | 48 |
| Primary Care | 2 | Dr. Tanvi Desai | 32 |
| Primary Care | 3 | Dr. Sara Gupta | 17 |

The tie-breaker `physician_id` makes the result deterministic. Without it, two physicians with the same TRx could swap places between runs. In an interview, say why you added the tie-breaker.

#### 23.6.5 Month-over-month growth

```sql
WITH monthly AS (
    SELECT month, SUM(trx) AS trx
    FROM prescriptions
    GROUP BY month
)
SELECT
    month,
    trx,
    LAG(trx) OVER (ORDER BY month) AS previous_month,
    ROUND(
        100.0 * (trx - LAG(trx) OVER (ORDER BY month))
        / NULLIF(LAG(trx) OVER (ORDER BY month), 0),
        2
    ) AS mom_growth_pct
FROM monthly
ORDER BY month;
```

| month | trx | previous_month | mom_growth_pct |
|---|---:|---:|---:|
| 2026-01-01 | 208 | NULL | NULL |
| 2026-02-01 | 241 | 208 | 15.87 |
| 2026-03-01 | 273 | 241 | 13.28 |
| 2026-04-01 | 306 | 273 | 12.09 |
| 2026-05-01 | 327 | 306 | 6.86 |
| 2026-06-01 | 344 | 327 | 5.20 |

The first month has no previous month. Do not replace that NULL with 0; “no comparison” is different from “0% growth”. The growth is strong but decelerating: 15.87% → 13.28% → 12.09% → 6.86% → 5.20%.

#### 23.6.6 Running total

```sql
WITH monthly AS (
    SELECT month, SUM(trx) AS trx
    FROM prescriptions
    GROUP BY month
)
SELECT
    month,
    trx,
    SUM(trx) OVER (
        ORDER BY month
        ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW
    ) AS running_total
FROM monthly
ORDER BY month;
```

| month | trx | running_total |
|---|---:|---:|
| 2026-01-01 | 208 | 208 |
| 2026-02-01 | 241 | 449 |
| 2026-03-01 | 273 | 722 |
| 2026-04-01 | 306 | 1,028 |
| 2026-05-01 | 327 | 1,355 |
| 2026-06-01 | 344 | 1,699 |

The final running total equals the total TRx, **1,699**. This is a useful reconciliation check: if the last running total does not equal `SUM(trx)`, the frame or the ordering is wrong.

#### 23.6.7 Three-month moving average

```sql
WITH monthly AS (
    SELECT month, SUM(trx) AS trx
    FROM prescriptions
    GROUP BY month
)
SELECT
    month,
    trx,
    ROUND(
        AVG(trx) OVER (
            ORDER BY month
            ROWS BETWEEN 2 PRECEDING AND CURRENT ROW
        ),
        2
    ) AS ma_3m
FROM monthly
ORDER BY month;
```

| month | trx | ma_3m |
|---|---:|---:|
| 2026-01-01 | 208 | 208.00 |
| 2026-02-01 | 241 | 224.50 |
| 2026-03-01 | 273 | 240.67 |
| 2026-04-01 | 306 | 273.33 |
| 2026-05-01 | 327 | 302.00 |
| 2026-06-01 | 344 | 325.67 |

The first two rows use fewer than three observations because there is no earlier data. That is correct. Do not divide by 3 manually; the window frame handles the denominator.

#### 23.6.8 Percent of total

```sql
SELECT
    p.product_name,
    SUM(r.trx) AS trx,
    ROUND(
        100.0 * SUM(r.trx) / SUM(SUM(r.trx)) OVER (),
        2
    ) AS share_pct
FROM prescriptions AS r
JOIN products AS p
    ON p.product_id = r.product_id
WHERE r.month = '2026-06-01'
GROUP BY p.product_name
ORDER BY trx DESC;
```

| product_name | trx | share_pct |
|---|---:|---:|
| OncoZ | 124 | 36.05 |
| CardioX | 115 | 33.43 |
| NeuroY | 105 | 30.52 |

`SUM(SUM(trx)) OVER ()` is an aggregate inside a window over the aggregated result. It works in SQLite and PostgreSQL. If your engine complains, compute the total in a CTE and cross join it.

#### 23.6.9 ROW_NUMBER vs RANK vs DENSE_RANK

These are not interchangeable:

| Function | Ties | Gaps | Use |
|---|---|---|---|
| `ROW_NUMBER()` | Arbitrary unless you add a tie-breaker | None | Deduplication, top-N with deterministic order |
| `RANK()` | Same rank for ties | Yes (1, 2, 2, 4) | “Position in a league table” |
| `DENSE_RANK()` | Same rank for ties | No (1, 2, 2, 3) | “How many distinct levels above me?” |

Example using average call duration by rep:

```sql
WITH rep_avg AS (
    SELECT
        rp.rep_name,
        ROUND(AVG(c.duration_min), 2) AS avg_min
    FROM calls AS c
    JOIN reps AS rp
        ON rp.rep_id = c.rep_id
    GROUP BY rp.rep_name
)
SELECT
    rep_name,
    avg_min,
    RANK() OVER (ORDER BY avg_min DESC) AS rnk,
    DENSE_RANK() OVER (ORDER BY avg_min DESC) AS dense_rnk,
    ROW_NUMBER() OVER (ORDER BY avg_min DESC, rep_name) AS rn
FROM rep_avg
ORDER BY avg_min DESC, rep_name;
```

The top is **Rohan Das 40.5**, then **Sneha Pillai 40.0**, then **Vikram Reddy 39.5**. Kabir Singh and Priya Shah both have **34.0**; `RANK` gives both 7, `DENSE_RANK` gives both 7, and `ROW_NUMBER` splits them 7 and 8. The tie is the point.

#### 23.6.10 NTILE: quartiles

```sql
WITH june AS (
    SELECT physician_id, SUM(trx) AS trx
    FROM prescriptions
    WHERE month = '2026-06-01'
    GROUP BY physician_id
)
SELECT
    physician_id,
    trx,
    NTILE(4) OVER (ORDER BY trx) AS quartile
FROM june
ORDER BY trx;
```

The 14 physicians split into quartiles of 4, 4, 3, and 3 rows. The lowest quartile ranges from 8 to 14 TRx; the highest ranges from 40 to 48. `NTILE` is useful for segmentation, but the bucket sizes can differ when the row count is not divisible by the number of buckets. State that.

#### Check yourself — 23.6

1. Rank products by revenue. Which product is second?
2. What are the top 3 region-product cells?
3. What is the top 3 per specialty in June? How many rows does the answer have?
4. What is the running total at the end of June? Why should it equal `SUM(trx)`?
5. What is the three-month moving average in June?
6. What is the difference between `ROW_NUMBER`, `RANK`, and `DENSE_RANK`?

Answers are in 23.15.

---
### 23.7 CASE, pivoting, and conditional aggregation

`CASE` is SQL's if/then/else. It is used for banding, cleaning, flags, and — most importantly — **pivoting** with conditional aggregation.

#### 23.7.1 Banding

```sql
SELECT
    sale_id,
    revenue,
    CASE
        WHEN revenue >= 60000 THEN 'High'
        WHEN revenue >= 30000 THEN 'Medium'
        ELSE 'Low'
    END AS revenue_band
FROM sales
ORDER BY revenue DESC
LIMIT 5;
```

The top rows are High. A summary:

```sql
SELECT
    CASE
        WHEN revenue >= 60000 THEN 'High'
        WHEN revenue >= 30000 THEN 'Medium'
        ELSE 'Low'
    END AS revenue_band,
    COUNT(*) AS rows
FROM sales
GROUP BY revenue_band
ORDER BY revenue_band;
```

| revenue_band | rows |
|---|---:|
| High | 2 |
| Low | 20 |
| Medium | 50 |

`CASE` evaluates in order. The first true condition wins. Put the most specific condition first.

#### 23.7.2 Pivot: month × product

SQL does not have a universal `PIVOT` keyword. The portable pattern is conditional aggregation:

```sql
SELECT
    p.product_name,
    SUM(CASE WHEN s.month = '2026-01-01' THEN s.revenue ELSE 0 END) AS jan,
    SUM(CASE WHEN s.month = '2026-02-01' THEN s.revenue ELSE 0 END) AS feb,
    SUM(CASE WHEN s.month = '2026-03-01' THEN s.revenue ELSE 0 END) AS mar,
    SUM(CASE WHEN s.month = '2026-04-01' THEN s.revenue ELSE 0 END) AS apr,
    SUM(CASE WHEN s.month = '2026-05-01' THEN s.revenue ELSE 0 END) AS may,
    SUM(CASE WHEN s.month = '2026-06-01' THEN s.revenue ELSE 0 END) AS jun
FROM sales AS s
JOIN products AS p
    ON p.product_id = s.product_id
GROUP BY p.product_name
ORDER BY p.product_name;
```

| product_name | jan | feb | mar | apr | may | jun |
|---|---:|---:|---:|---:|---:|---:|
| CardioX | 157,200 | 155,400 | 175,080 | 175,080 | 182,070 | 206,640 |
| NeuroY | 134,400 | 138,390 | 128,940 | 130,410 | 132,930 | 123,900 |
| OncoZ | 86,400 | 103,680 | 120,960 | 149,280 | 172,800 | 209,760 |

The `ELSE 0` matters. Without it, a non-matching row returns NULL, and `SUM` ignores NULL. For a pivot, both often produce the same total, but `ELSE 0` makes the intent explicit and behaves correctly in `COUNT` and arithmetic.

**Dynamic columns:** plain SQL cannot create a new column for every month without dynamic SQL. In an interview, use conditional aggregation for a fixed set of columns, or return long-format data and pivot in Excel/Power BI. Do not pretend a static query will adapt to next month.

#### 23.7.3 Pivot: survey stage counts

```sql
WITH dedup AS (
    SELECT physician_id, MIN(response_id) AS keep_id
    FROM survey
    GROUP BY physician_id
),
clean_survey AS (
    SELECT s.*
    FROM survey AS s
    JOIN dedup AS d
        ON d.keep_id = s.response_id
    WHERE s.response_date BETWEEN '2026-01-15' AND '2026-03-31'
)
SELECT
    SUM(CASE WHEN awareness_stage = 'Unaided awareness' THEN 1 ELSE 0 END) AS unaided,
    SUM(CASE WHEN awareness_stage = 'Aided awareness' THEN 1 ELSE 0 END) AS aided,
    SUM(CASE WHEN awareness_stage = 'Consideration' THEN 1 ELSE 0 END) AS consideration,
    SUM(CASE WHEN awareness_stage = 'Trial prescribed' THEN 1 ELSE 0 END) AS trial,
    SUM(CASE WHEN awareness_stage = 'Regular prescriber' THEN 1 ELSE 0 END) AS regular,
    COUNT(*) AS total
FROM clean_survey;
```

| unaided | aided | consideration | trial | regular | total |
|---:|---:|---:|---:|---:|---:|
| 8 | 8 | 8 | 7 | 8 | 39 |

This is a one-row summary. If you want one row per specialty, add `TRIM(p.specialty)` and join `physicians`.

#### 23.7.4 Top-2-box with conditional aggregation

```sql
WITH dedup AS (
    SELECT physician_id, MIN(response_id) AS keep_id
    FROM survey
    GROUP BY physician_id
),
clean_survey AS (
    SELECT s.*
    FROM survey AS s
    JOIN dedup AS d
        ON d.keep_id = s.response_id
    WHERE s.response_date BETWEEN '2026-01-15' AND '2026-03-31'
)
SELECT
    i.item,
    COUNT(*) AS n,
    ROUND(
        100.0 * SUM(CASE WHEN i.score_raw IN ('4', '5') THEN 1 ELSE 0 END)
        / COUNT(*),
        2
    ) AS top2_pct
FROM survey_items AS i
JOIN clean_survey AS c
    ON c.response_id = i.response_id
WHERE i.score_raw IN ('1', '2', '3', '4', '5')
GROUP BY i.item
ORDER BY i.item;
```

| item | n | top2_pct |
|---|---:|---:|
| Access | 38 | 36.84 |
| Efficacy | 38 | 47.37 |
| Intent | 39 | 35.90 |

The `WHERE i.score_raw IN (...)` excludes `N/A` and `7` **before** the percentage. This is the correct order: define the valid base, then calculate. If you include invalid codes in the denominator but not the numerator, you understate the score.

#### 23.7.5 NPS with conditional aggregation

```sql
WITH dedup AS (
    SELECT physician_id, MIN(response_id) AS keep_id
    FROM survey
    GROUP BY physician_id
),
clean_survey AS (
    SELECT s.*
    FROM survey AS s
    JOIN dedup AS d
        ON d.keep_id = s.response_id
    WHERE s.response_date BETWEEN '2026-01-15' AND '2026-03-31'
)
SELECT
    SUM(CASE WHEN recommend_score >= 9 THEN 1 ELSE 0 END) AS promoters,
    SUM(CASE WHEN recommend_score BETWEEN 7 AND 8 THEN 1 ELSE 0 END) AS passives,
    SUM(CASE WHEN recommend_score <= 6 THEN 1 ELSE 0 END) AS detractors,
    ROUND(
        100.0 * (
            SUM(CASE WHEN recommend_score >= 9 THEN 1 ELSE 0 END)
            - SUM(CASE WHEN recommend_score <= 6 THEN 1 ELSE 0 END)
        ) / COUNT(*),
        2
    ) AS nps
FROM clean_survey;
```

| promoters | passives | detractors | nps |
|---:|---:|---:|---:|
| 12 | 12 | 15 | -7.69 |

NPS = % promoters − % detractors. Passives are in the denominator but not the numerator. With 39 respondents, each respondent is 2.56 percentage points; treat small differences cautiously.

#### 23.7.6 CAST and GLOB for mixed-type survey codes

`survey_items.score_raw` is text because it contains `N/A`. To average valid numeric codes:

```sql
SELECT
    item,
    ROUND(AVG(CAST(score_raw AS REAL)), 2) AS mean_score
FROM survey_items
WHERE score_raw IN ('1', '2', '3', '4', '5')
GROUP BY item
ORDER BY item;
```

| item | mean_score |
|---|---:|
| Access | 3.03 |
| Efficacy | 3.45 |
| Intent | 3.10 |

`CAST('N/A' AS REAL)` returns 0 in SQLite; in PostgreSQL it raises an error. Always filter valid codes first. `GLOB '[1-5]'` is a SQLite pattern for exactly one digit from 1 to 5; it is less portable than `IN`.

#### 23.7.7 COALESCE and NULLIF in a pivot

```sql
SELECT
    r.region_name,
    COALESCE(SUM(CASE WHEN p.product_name = 'CardioX' THEN s.revenue END), 0) AS cardiox,
    COALESCE(SUM(CASE WHEN p.product_name = 'OncoZ' THEN s.revenue END), 0) AS oncoz,
    ROUND(
        100.0 * SUM(CASE WHEN p.product_name = 'CardioX' THEN s.revenue END)
        / NULLIF(SUM(s.revenue), 0),
        2
    ) AS cardiox_share_pct
FROM sales AS s
JOIN regions AS r
    ON r.region_id = s.region_id
JOIN products AS p
    ON p.product_id = s.product_id
GROUP BY r.region_name
ORDER BY r.region_name;
```

`COALESCE` fills a missing cell with 0 for display. `NULLIF` protects the share denominator. Do not use `COALESCE` on the denominator unless the business rule says a zero denominator should become a different number; a share with no revenue is undefined, not 0%.

#### Check yourself — 23.7

1. How many sales rows are in the High revenue band? What is the threshold?
2. Write the month × product pivot. What is CardioX June revenue?
3. What is the top-2-box for Access, and what is the valid n?
4. What is NPS, and how many promoters/passives/detractors are there?
5. Why does `ELSE 0` matter in a conditional aggregation?

Answers are in 23.15.

---

### 23.8 Dates, cohorts, and time series

Dates are the most common source of silent errors. The practice database stores dates as ISO text (`YYYY-MM-DD`), which sorts correctly and compares lexicographically. That is a deliberate simplification; real warehouses use `DATE`, `TIMESTAMP`, or epoch integers.

#### 23.8.1 Date arithmetic in SQLite

```sql
SELECT
    date('2026-01-01', '+1 month') AS next_month,
    date('2026-06-01', '-1 day') AS day_before,
    julianday('2026-06-01') - julianday('2026-01-01') AS days_between,
    strftime('%Y-%m', '2026-06-15') AS year_month,
    CAST(strftime('%m', '2026-06-15') AS INTEGER) AS month_number;
```

| next_month | day_before | days_between | year_month | month_number |
|---|---|---|---:|---|---:|
| 2026-02-01 | 2026-05-31 | 151.0 | 2026-06 | 6 |

`julianday` returns a floating-point day number; the difference between two dates is 151 days for 1 January to 1 June 2026 (January 31 + February 28 + March 31 + April 30 + May 31). `strftime` is SQLite-specific; 23.12 maps it to other engines.

#### 23.8.2 The field window

```sql
SELECT COUNT(*) AS fielded_n
FROM survey
WHERE response_date BETWEEN '2026-01-15' AND '2026-03-31';
```

Result: **41** rows before deduplication. After deduplication, **39**. The one out-of-window response is **SR-032 / P-030**, dated **2025-12-15**. The field window is a business rule, not a suggestion: including a December response in a Q1 survey changes the base.

**Boundary rule:** `BETWEEN` is inclusive on both ends. If `response_date` had a time component, `BETWEEN '2026-01-15' AND '2026-03-31'` would exclude most of 31 March after midnight. Use a half-open range for timestamps:

```sql
WHERE response_date >= '2026-01-15'
  AND response_date <  '2026-04-01'
```

#### 23.8.3 Active months and retention

```sql
WITH active AS (
    SELECT DISTINCT physician_id, month
    FROM prescriptions
),
january AS (
    SELECT physician_id FROM active WHERE month = '2026-01-01'
),
june AS (
    SELECT physician_id FROM active WHERE month = '2026-06-01'
)
SELECT
    COUNT(*) AS january_active,
    COUNT(j.physician_id) AS retained_in_june,
    ROUND(100.0 * COUNT(j.physician_id) / COUNT(*), 1) AS retention_pct
FROM january AS a
LEFT JOIN june AS j
    ON j.physician_id = a.physician_id;
```

| january_active | retained_in_june | retention_pct |
|---:|---:|---:|
| 16 | 14 | 87.5 |

Two physicians (**P-041**, **P-042**) prescribed in January–April but not in May–June. They are the deliberate churn in the dataset. The other 14 are active in all six months.

**Do not confuse “active in June” with “retained from January”.** The query above anchors on the January cohort. A simpler `COUNT(DISTINCT physician_id)` by month would show 16 → 14 but would not prove that the same 14 are retained. Cohort anchoring matters.

#### 23.8.4 First and last activity

```sql
SELECT
    physician_id,
    MIN(month) AS first_month,
    MAX(month) AS last_month,
    COUNT(DISTINCT month) AS months_active
FROM prescriptions
GROUP BY physician_id
ORDER BY physician_id;
```

| physician_id | first_month | last_month | months_active |
|---|---|---|---:|
| P-025 | 2026-01-01 | 2026-06-01 | 6 |
| P-026 | 2026-01-01 | 2026-06-01 | 6 |
| P-027 | 2026-01-01 | 2026-06-01 | 6 |
| P-029 | 2026-01-01 | 2026-06-01 | 6 |
| P-031 | 2026-01-01 | 2026-06-01 | 6 |
| P-032 | 2026-01-01 | 2026-06-01 | 6 |
| P-033 | 2026-01-01 | 2026-06-01 | 6 |
| P-034 | 2026-01-01 | 2026-06-01 | 6 |
| P-035 | 2026-01-01 | 2026-06-01 | 6 |
| P-036 | 2026-01-01 | 2026-06-01 | 6 |
| P-037 | 2026-01-01 | 2026-06-01 | 6 |
| P-038 | 2026-01-01 | 2026-06-01 | 6 |
| P-039 | 2026-01-01 | 2026-06-01 | 6 |
| P-040 | 2026-01-01 | 2026-06-01 | 6 |
| P-041 | 2026-01-01 | 2026-04-01 | 4 |
| P-042 | 2026-01-01 | 2026-04-01 | 4 |

This is a compact physician-level activity profile. It is also the base for a churn list: `WHERE last_month < '2026-06-01'` returns P-041 and P-042.

#### 23.8.5 Cohort matrix

```sql
WITH first_month AS (
    SELECT physician_id, MIN(month) AS cohort_month
    FROM prescriptions
    GROUP BY physician_id
),
activity AS (
    SELECT DISTINCT physician_id, month
    FROM prescriptions
)
SELECT
    f.cohort_month,
    a.month,
    COUNT(DISTINCT a.physician_id) AS active_physicians
FROM first_month AS f
JOIN activity AS a
    ON a.physician_id = f.physician_id
GROUP BY f.cohort_month, a.month
ORDER BY f.cohort_month, a.month;
```

| cohort_month | month | active_physicians |
|---|---|---:|
| 2026-01-01 | 2026-01-01 | 16 |
| 2026-01-01 | 2026-02-01 | 16 |
| 2026-01-01 | 2026-03-01 | 16 |
| 2026-01-01 | 2026-04-01 | 16 |
| 2026-01-01 | 2026-05-01 | 14 |
| 2026-01-01 | 2026-06-01 | 14 |

All 16 physicians start in January, so there is only one cohort. In a real dataset with staggered starts, this query produces a triangular cohort table. The pattern is the same; the data is just richer.

#### 23.8.6 Gaps and islands

A “gaps and islands” query finds consecutive runs of activity. The trick is to subtract a row number from a month number: consecutive months produce a constant group key.

```sql
WITH months AS (
    SELECT DISTINCT physician_id, month
    FROM prescriptions
),
numbered AS (
    SELECT
        physician_id,
        month,
        CAST(strftime('%Y', month) AS INTEGER) * 12
            + CAST(strftime('%m', month) AS INTEGER) AS month_num,
        ROW_NUMBER() OVER (
            PARTITION BY physician_id
            ORDER BY month
        ) AS rn
    FROM months
)
SELECT
    physician_id,
    MIN(month) AS start_month,
    MAX(month) AS end_month,
    COUNT(*) AS consecutive_months
FROM numbered
GROUP BY physician_id, month_num - rn
ORDER BY physician_id, start_month;
```

For this dataset, every physician has one island: 14 physicians have six consecutive months, and P-041/P-042 have four. If a physician had Jan–Feb and April–June, the query would return two islands. This is a common senior-level interview pattern; practise explaining the `month_num - rn` trick rather than memorising it.

#### 23.8.7 Date spine

If a month has no prescriptions, a `GROUP BY month` simply omits it. For a complete trend, build a calendar spine and `LEFT JOIN` the facts:

```sql
WITH RECURSIVE months(month) AS (
    SELECT '2026-01-01'
    UNION ALL
    SELECT date(month, '+1 month')
    FROM months
    WHERE month < '2026-06-01'
),
monthly AS (
    SELECT month, SUM(trx) AS trx
    FROM prescriptions
    GROUP BY month
)
SELECT
    m.month,
    COALESCE(x.trx, 0) AS trx
FROM months AS m
LEFT JOIN monthly AS x
    ON x.month = m.month
ORDER BY m.month;
```

This returns six rows even if a month has zero TRx. The `RECURSIVE` CTE is SQLite/PostgreSQL syntax; SQL Server uses a different recursive CTE style, and some warehouses have a calendar table. In an interview, say why the spine is necessary; do not silently drop zero months.

#### Check yourself — 23.8

1. How many days are between 2026-01-01 and 2026-06-01?
2. How many survey rows are in the field window before deduplication? After deduplication?
3. What is the January-to-June retention rate? Which physicians churned?
4. What does the cohort matrix show for May and June?
5. Why does a `GROUP BY month` trend sometimes need a date spine?

Answers are in 23.15.

---
## Part D — Applied analytics

### 23.9 Data quality and reconciliation in SQL

Data quality is not a final check. It is a set of queries you run **before** the analysis, and again after every join. The goal is not “perfect data”; it is **known data**.

#### 23.9.1 Row-count audit

```sql
SELECT 'regions' AS table_name, COUNT(*) AS rows FROM regions
UNION ALL SELECT 'products', COUNT(*) FROM products
UNION ALL SELECT 'reps', COUNT(*) FROM reps
UNION ALL SELECT 'sales', COUNT(*) FROM sales
UNION ALL SELECT 'physicians', COUNT(*) FROM physicians
UNION ALL SELECT 'survey', COUNT(*) FROM survey
UNION ALL SELECT 'survey_items', COUNT(*) FROM survey_items
UNION ALL SELECT 'calls', COUNT(*) FROM calls
UNION ALL SELECT 'prescriptions', COUNT(*) FROM prescriptions
UNION ALL SELECT 'quotas', COUNT(*) FROM quotas
ORDER BY table_name;
```

| table_name | rows |
|---|---:|
| calls | 120 |
| physicians | 42 |
| prescriptions | 276 |
| products | 3 |
| quotas | 5 |
| regions | 4 |
| reps | 12 |
| sales | 72 |
| survey | 42 |
| survey_items | 126 |

Run this at the start of every session. If a row count changes unexpectedly, stop and investigate before computing a KPI.

#### 23.9.2 Duplicate detection

```sql
SELECT
    physician_id,
    COUNT(*) AS responses,
    GROUP_CONCAT(response_id) AS response_ids
FROM survey
GROUP BY physician_id
HAVING COUNT(*) > 1
ORDER BY physician_id;
```

| physician_id | responses | response_ids |
|---|---:|---|
| P-007 | 2 | SR-007,SR-008 |
| P-023 | 2 | SR-024,SR-025 |

Two physicians have duplicate responses. The duplicate rows are exact copies on every respondent-level field; only `response_id` differs. That is why `COUNT(DISTINCT physician_id)` is 40 while `COUNT(*)` is 42.

**Do not dedupe by `response_id`.** It is unique by construction; deduping on it removes nothing. Dedupe by the entity (physician) and all the fields that define the response, or use a documented rule such as “keep the first complete response”.

#### 23.9.3 Invalid categories

```sql
SELECT
    i.response_id,
    s.physician_id,
    i.item,
    i.score_raw
FROM survey_items AS i
JOIN survey AS s
    ON s.response_id = i.response_id
WHERE i.score_raw NOT IN ('1', '2', '3', '4', '5')
ORDER BY i.response_id, i.item;
```

| response_id | physician_id | item | score_raw |
|---|---|---|---|
| SR-020 | P-019 | Access | 7 |
| SR-021 | P-020 | Efficacy | N/A |

A 7 on a 1–5 scale is out of range; `N/A` is missing, not a score. Both must be excluded from the valid base **and** reported. If you silently drop them, the client cannot see the data-quality cost.

#### 23.9.4 Orphan keys

An orphan is a foreign key with no matching dimension row.

```sql
SELECT
    (SELECT COUNT(*)
     FROM sales AS s
     LEFT JOIN regions AS r ON r.region_id = s.region_id
     WHERE r.region_id IS NULL) AS orphan_sales_region,
    (SELECT COUNT(*)
     FROM sales AS s
     LEFT JOIN products AS p ON p.product_id = s.product_id
     WHERE p.product_id IS NULL) AS orphan_sales_product,
    (SELECT COUNT(*)
     FROM survey AS s
     LEFT JOIN physicians AS p ON p.physician_id = s.physician_id
     WHERE p.physician_id IS NULL) AS orphan_survey_physician,
    (SELECT COUNT(*)
     FROM calls AS c
     LEFT JOIN reps AS r ON r.rep_id = c.rep_id
     WHERE r.rep_id IS NULL) AS orphan_calls_rep,
    (SELECT COUNT(*)
     FROM prescriptions AS r
     LEFT JOIN physicians AS p ON p.physician_id = r.physician_id
     WHERE p.physician_id IS NULL) AS orphan_rx_physician;
```

All five counts are **0**. In a real extract, a non-zero orphan count is a finding: either the dimension is incomplete or the fact has a bad key. Do not drop the rows without quantifying them.

#### 23.9.5 Range and logic checks

```sql
SELECT
    (SELECT COUNT(*) FROM survey
     WHERE recommend_score NOT BETWEEN 0 AND 10) AS bad_nps,
    (SELECT COUNT(*) FROM calls
     WHERE duration_min NOT BETWEEN 15 AND 60) AS bad_duration,
    (SELECT COUNT(*) FROM prescriptions
     WHERE new_rx > trx OR trx < 0) AS bad_rx,
    (SELECT COUNT(*) FROM survey
     WHERE monthly_trx < 0) AS bad_trx,
    (SELECT COUNT(*) FROM sales
     WHERE ABS(revenue - units * unit_price) > 1) AS bad_revenue;
```

| bad_nps | bad_duration | bad_rx | bad_trx | bad_revenue |
|---:|---:|---:|---:|---:|
| 0 | 0 | 0 | 0 | 0 |

These checks are cheap and catch the errors that destroy trust: a negative prescription count, a call longer than a day, a revenue row that does not equal units × price.

#### 23.9.6 Reconciliation: revenue vs units × price

```sql
SELECT
    SUM(revenue) AS revenue,
    SUM(units * unit_price) AS computed_revenue,
    SUM(revenue) - SUM(units * unit_price) AS difference
FROM sales;
```

| revenue | computed_revenue | difference |
|---:|---:|---:|
| 2,683,320 | 2,683,320 | 0 |

A zero difference means the revenue column is internally consistent with units and price. It does **not** mean the revenue is correct in the real world; it means the arithmetic reconciles. That is a narrower, honest claim.

#### 23.9.7 Variance to target

```sql
SELECT
    r.region_name,
    SUM(s.revenue) AS revenue,
    SUM(s.target_revenue) AS target,
    SUM(s.revenue) - SUM(s.target_revenue) AS variance
FROM sales AS s
JOIN regions AS r
    ON r.region_id = s.region_id
GROUP BY r.region_name
ORDER BY r.region_name;
```

| region_name | revenue | target | variance |
|---|---:|---:|---:|
| East | 481,200 | 460,455 | +20,745 |
| North | 864,954 | 886,608 | -21,654 |
| South | 724,284 | 747,844 | -23,560 |
| West | 612,882 | 622,128 | -9,246 |

The total variance is **-33,715**: the sum of the four regional variances. East is the only region with a positive variance, but it is not large enough to offset North and South. A variance bridge should show that explicitly rather than saying “98.76% overall”.

#### 23.9.8 Quota reconciliation

```sql
WITH dedup AS (
    SELECT physician_id, MIN(response_id) AS keep_id
    FROM survey
    GROUP BY physician_id
),
clean_survey AS (
    SELECT s.*
    FROM survey AS s
    JOIN dedup AS d
        ON d.keep_id = s.response_id
    WHERE s.response_date BETWEEN '2026-01-15' AND '2026-03-31'
)
SELECT
    q.awareness_stage,
    q.target_n,
    COUNT(c.response_id) AS actual_n,
    COUNT(c.response_id) - q.target_n AS variance
FROM quotas AS q
LEFT JOIN clean_survey AS c
    ON c.awareness_stage = q.awareness_stage
GROUP BY q.awareness_stage, q.target_n
ORDER BY q.awareness_stage;
```

| awareness_stage | target_n | actual_n | variance |
|---|---:|---:|---:|
| Aided awareness | 8 | 8 | 0 |
| Consideration | 8 | 8 | 0 |
| Regular prescriber | 8 | 8 | 0 |
| Trial prescribed | 8 | 7 | -1 |
| Unaided awareness | 8 | 8 | 0 |

The trial cell is one respondent short. With a base of 7, a single respondent is 14.3% of the cell, so any trial-stage percentage is fragile. This is a “flag the base” situation, not a “hide the result” situation.

#### 23.9.9 Freshness

```sql
SELECT
    (SELECT MIN(month) FROM sales) AS sales_min,
    (SELECT MAX(month) FROM sales) AS sales_max,
    (SELECT MIN(month) FROM prescriptions) AS rx_min,
    (SELECT MAX(month) FROM prescriptions) AS rx_max,
    (SELECT MIN(call_date) FROM calls) AS call_min,
    (SELECT MAX(call_date) FROM calls) AS call_max,
    (SELECT MIN(response_date) FROM survey) AS survey_min,
    (SELECT MAX(response_date) FROM survey) AS survey_max;
```

| sales_min | sales_max | rx_min | rx_max | call_min | call_max | survey_min | survey_max |
|---|---|---|---|---|---|---|---|
| 2026-01-01 | 2026-06-01 | 2026-01-01 | 2026-06-01 | 2026-01-04 | 2026-06-27 | 2025-12-15 | 2026-03-12 |

The survey minimum is **2025-12-15** because of the out-of-window response. The field window removes it. Freshness is not just “what is the max date”; it is “what is the max date **within the valid window**”.

#### 23.9.10 The QA checklist in SQL

| Check | Query pattern | Expected here |
|---|---|---|
| Row counts | `SELECT COUNT(*)` per table | See 23.9.1 |
| Duplicates | `GROUP BY entity HAVING COUNT(*) > 1` | P-007, P-023 |
| Invalid categories | `WHERE value NOT IN (valid list)` | SR-020, SR-021 |
| Orphans | `LEFT JOIN … WHERE dimension key IS NULL` | 0 |
| Range | `WHERE value NOT BETWEEN min AND max` | 0 |
| Reconciliation | `SUM(a) - SUM(b)` | 0 |
| Fan-out | Count before/after join | 42 → 126 after items |
| Freshness | `MIN/MAX(date)` within window | 2026-01-15 to 2026-03-12 |
| Blank critical fields | `COUNT(*) - COUNT(column)` | 2 blank targets, 3 blank calls |

#### Check yourself — 23.9

1. How many duplicate survey respondents are there? Which ones?
2. What are the two invalid Likert records?
3. What is the total sales variance to target? Which region is positive?
4. What is the trial-stage quota variance?
5. Why is a zero revenue reconciliation not proof that the data is correct?

Answers are in 23.15.

---
### 23.10 Survey analytics: from 42 raw rows to 39 defensible respondents

Survey data is where SQL earns its keep. The raw table is messy by design: duplicate respondents, an out-of-window response, invalid Likert codes, a blank TRx, a straight-liner, and dirty region strings. The analysis is only as good as the cleaning rules.

#### 23.10.1 The cleaning pipeline

| Step | Rule | Rows remaining |
|---:|---|---:|
| 0 | Raw survey responses | 42 |
| 1 | Keep one response per physician (first `response_id`) | 40 |
| 2 | Restrict to field window 2026-01-15 to 2026-03-31 | 39 |
| 3 | Valid Likert codes only (1–5) | 39 / 38 / 38 by item |
| 4 | Report invalid, blank, and straight-liner flags | — |

**Why dedupe first, then window?** The duplicate rows are exact copies, so either order gives 39 here. But the rule matters in general: if a physician has one in-window and one out-of-window response, deduping first could keep the wrong one. Prefer “filter to the valid window, then dedupe by the business rule” unless the business rule explicitly says otherwise. Document the order.

#### 23.10.2 Dedupe with ROW_NUMBER

`MIN(response_id)` is simple, but `ROW_NUMBER` is more flexible because you can choose the latest, the most complete, or the highest-weight response.

```sql
WITH ranked AS (
    SELECT
        s.*,
        ROW_NUMBER() OVER (
            PARTITION BY physician_id
            ORDER BY response_id
        ) AS rn
    FROM survey AS s
    WHERE response_date BETWEEN '2026-01-15' AND '2026-03-31'
)
SELECT COUNT(*) AS clean_n
FROM ranked
WHERE rn = 1;
```

Result: **39**. The `ORDER BY response_id` is the tie-breaker. If you wanted the latest response, use `ORDER BY response_date DESC, response_id DESC`.

#### 23.10.3 Stage counts

```sql
WITH ranked AS (
    SELECT
        s.*,
        ROW_NUMBER() OVER (
            PARTITION BY physician_id
            ORDER BY response_id
        ) AS rn
    FROM survey AS s
    WHERE response_date BETWEEN '2026-01-15' AND '2026-03-31'
),
clean_survey AS (
    SELECT * FROM ranked WHERE rn = 1
)
SELECT awareness_stage, COUNT(*) AS n
FROM clean_survey
GROUP BY awareness_stage
ORDER BY n DESC, awareness_stage;
```

| awareness_stage | n |
|---|---:|
| Aided awareness | 8 |
| Consideration | 8 |
| Regular prescriber | 8 |
| Unaided awareness | 8 |
| Trial prescribed | 7 |

The total is 39. The trial cell is one short of the quota of 8. The other four cells are exactly 8. This is a clean, balanced sample except for the trial shortfall.

#### 23.10.4 Top-2-box by item

```sql
WITH ranked AS (
    SELECT
        s.*,
        ROW_NUMBER() OVER (
            PARTITION BY physician_id
            ORDER BY response_id
        ) AS rn
    FROM survey AS s
    WHERE response_date BETWEEN '2026-01-15' AND '2026-03-31'
),
clean_survey AS (
    SELECT * FROM ranked WHERE rn = 1
)
SELECT
    i.item,
    COUNT(*) AS valid_n,
    ROUND(
        100.0 * SUM(CASE WHEN i.score_raw IN ('4', '5') THEN 1 ELSE 0 END)
        / COUNT(*),
        2
    ) AS top2_pct,
    ROUND(AVG(CAST(i.score_raw AS REAL)), 2) AS mean_score
FROM survey_items AS i
JOIN clean_survey AS c
    ON c.response_id = i.response_id
WHERE i.score_raw IN ('1', '2', '3', '4', '5')
GROUP BY i.item
ORDER BY i.item;
```

| item | valid_n | top2_pct | mean_score |
|---|---:|---:|---:|
| Access | 38 | 36.84 | 3.03 |
| Efficacy | 38 | 47.37 | 3.45 |
| Intent | 39 | 35.90 | 3.10 |

**Reading:** Efficacy is the strongest item (47.37% top-2-box, mean 3.45). Access is the weakest (36.84%, mean 3.03). Intent is also weak (35.90%). The pattern suggests a product that physicians believe in but cannot easily access — a hypothesis, not a proven causal claim. The next analysis would cut Access by specialty, region, and channel, and check whether the weakness is concentrated.

#### 23.10.5 NPS

```sql
WITH ranked AS (
    SELECT
        s.*,
        ROW_NUMBER() OVER (
            PARTITION BY physician_id
            ORDER BY response_id
        ) AS rn
    FROM survey AS s
    WHERE response_date BETWEEN '2026-01-15' AND '2026-03-31'
),
clean_survey AS (
    SELECT * FROM ranked WHERE rn = 1
)
SELECT
    SUM(CASE WHEN recommend_score >= 9 THEN 1 ELSE 0 END) AS promoters,
    SUM(CASE WHEN recommend_score BETWEEN 7 AND 8 THEN 1 ELSE 0 END) AS passives,
    SUM(CASE WHEN recommend_score <= 6 THEN 1 ELSE 0 END) AS detractors,
    ROUND(
        100.0 * (
            SUM(CASE WHEN recommend_score >= 9 THEN 1 ELSE 0 END)
            - SUM(CASE WHEN recommend_score <= 6 THEN 1 ELSE 0 END)
        ) / COUNT(*),
        2
    ) AS nps
FROM clean_survey;
```

| promoters | passives | detractors | nps |
|---:|---:|---:|---:|
| 12 | 12 | 15 | -7.69 |

NPS is negative. The detractors outnumber the promoters by three respondents. With n=39, that is a meaningful directional signal but not a precise estimate; one respondent moves NPS by 2.56 points.

#### 23.10.6 Weighted estimates

The `weight` column adjusts each respondent to a target population. Always show both unweighted and weighted results; the difference tells you whether the sample composition is distorting the headline.

```sql
WITH ranked AS (
    SELECT
        s.*,
        ROW_NUMBER() OVER (
            PARTITION BY physician_id
            ORDER BY response_id
        ) AS rn
    FROM survey AS s
    WHERE response_date BETWEEN '2026-01-15' AND '2026-03-31'
),
clean_survey AS (
    SELECT * FROM ranked WHERE rn = 1
)
SELECT
    i.item,
    ROUND(
        100.0 * SUM(CASE WHEN i.score_raw IN ('4', '5') THEN c.weight ELSE 0 END)
        / SUM(c.weight),
        2
    ) AS weighted_top2_pct
FROM survey_items AS i
JOIN clean_survey AS c
    ON c.response_id = i.response_id
WHERE i.score_raw IN ('1', '2', '3', '4', '5')
GROUP BY i.item
ORDER BY i.item;
```

| item | weighted_top2_pct | unweighted_top2_pct | difference |
|---|---:|---:|---:|
| Access | 37.68 | 36.84 | +0.84 |
| Efficacy | 47.94 | 47.37 | +0.57 |
| Intent | 34.98 | 35.90 | -0.92 |

The differences are small (under 1 percentage point), so the unweighted story survives weighting. That is a useful robustness check, not a guarantee.

**Weighted NPS:**

```sql
WITH ranked AS (
    SELECT
        s.*,
        ROW_NUMBER() OVER (
            PARTITION BY physician_id
            ORDER BY response_id
        ) AS rn
    FROM survey AS s
    WHERE response_date BETWEEN '2026-01-15' AND '2026-03-31'
),
clean_survey AS (
    SELECT * FROM ranked WHERE rn = 1
)
SELECT
    ROUND(
        100.0 * (
            SUM(CASE WHEN recommend_score >= 9 THEN weight ELSE 0 END)
            - SUM(CASE WHEN recommend_score <= 6 THEN weight ELSE 0 END)
        ) / SUM(weight),
        2
    ) AS weighted_nps
FROM clean_survey;
```

Result: **-6.81**. The unweighted NPS is **-7.69**; weighting moves it by +0.88 points toward zero. The direction is unchanged.

**Effective sample size:**

```sql
WITH ranked AS (
    SELECT
        s.*,
        ROW_NUMBER() OVER (
            PARTITION BY physician_id
            ORDER BY response_id
        ) AS rn
    FROM survey AS s
    WHERE response_date BETWEEN '2026-01-15' AND '2026-03-31'
),
clean_survey AS (
    SELECT * FROM ranked WHERE rn = 1
)
SELECT
    ROUND(SUM(weight), 2) AS weight_sum,
    ROUND(SUM(weight * weight), 2) AS weight_sq_sum,
    ROUND(SUM(weight) * SUM(weight) / SUM(weight * weight), 2) AS effective_n
FROM clean_survey;
```

| weight_sum | weight_sq_sum | effective_n |
|---:|---:|---:|
| 42.60 | 48.04 | 37.78 |

The effective sample size is **37.78**, slightly below the actual 39 because the weights are uneven. This is the number to use when you want to be honest about precision; do not report n=39 as if it were a simple random sample.

#### 23.10.7 Cross-tab: specialty × stage

```sql
WITH ranked AS (
    SELECT
        s.*,
        ROW_NUMBER() OVER (
            PARTITION BY physician_id
            ORDER BY response_id
        ) AS rn
    FROM survey AS s
    WHERE response_date BETWEEN '2026-01-15' AND '2026-03-31'
),
clean_survey AS (
    SELECT * FROM ranked WHERE rn = 1
)
SELECT
    TRIM(p.specialty) AS specialty,
    SUM(CASE WHEN awareness_stage = 'Unaided awareness' THEN 1 ELSE 0 END) AS unaided,
    SUM(CASE WHEN awareness_stage = 'Aided awareness' THEN 1 ELSE 0 END) AS aided,
    SUM(CASE WHEN awareness_stage = 'Consideration' THEN 1 ELSE 0 END) AS consideration,
    SUM(CASE WHEN awareness_stage = 'Trial prescribed' THEN 1 ELSE 0 END) AS trial,
    SUM(CASE WHEN awareness_stage = 'Regular prescriber' THEN 1 ELSE 0 END) AS regular,
    COUNT(*) AS total
FROM clean_survey AS c
JOIN physicians AS p
    ON p.physician_id = c.physician_id
GROUP BY TRIM(p.specialty)
ORDER BY specialty;
```

| specialty | unaided | aided | consideration | trial | regular | total |
|---|---:|---:|---:|---:|---:|---:|
| Cardiologist | 2 | 2 | 2 | 2 | 2 | 10 |
| Neurologist | 2 | 2 | 2 | 1 | 2 | 9 |
| Oncologist | 2 | 2 | 2 | 2 | 2 | 10 |
| Primary Care | 2 | 2 | 2 | 2 | 2 | 10 |

The design is almost perfectly balanced: two per stage per specialty, except neurology has one trial rather than two. That is why the trial cell is 7, not 8. The cross-tab is a **sample-design** check, not a market finding. Do not over-interpret a one-respondent difference.

#### 23.10.8 TRx by stage and specialty

```sql
WITH ranked AS (
    SELECT
        s.*,
        ROW_NUMBER() OVER (
            PARTITION BY physician_id
            ORDER BY response_id
        ) AS rn
    FROM survey AS s
    WHERE response_date BETWEEN '2026-01-15' AND '2026-03-31'
),
clean_survey AS (
    SELECT * FROM ranked WHERE rn = 1
)
SELECT
    awareness_stage,
    COUNT(*) AS n,
    SUM(monthly_trx) AS trx,
    ROUND(100.0 * SUM(monthly_trx) / SUM(SUM(monthly_trx)) OVER (), 2) AS share_pct
FROM clean_survey
GROUP BY awareness_stage
ORDER BY trx DESC, awareness_stage;
```

| awareness_stage | n | trx | share_pct |
|---|---:|---:|---:|
| Regular prescriber | 8 | 272 | 79.07 |
| Trial prescribed | 7 | 72 | 20.93 |
| Aided awareness | 8 | 0 | 0.00 |
| Consideration | 8 | 0 | 0.00 |
| Unaided awareness | 8 | 0 | 0.00 |

The trial row has 7 respondents but only 72 TRx because one trial physician has a blank TRx. The **valid TRx base** for trial is 6 respondents, not 7. This is the kind of denominator note that separates a careful analyst from a fast one.

By specialty:

```sql
WITH ranked AS (
    SELECT s.*, ROW_NUMBER() OVER (
        PARTITION BY physician_id ORDER BY response_id
    ) AS rn
    FROM survey AS s
    WHERE response_date BETWEEN '2026-01-15' AND '2026-03-31'
),
clean_survey AS (SELECT * FROM ranked WHERE rn = 1)
SELECT
    TRIM(p.specialty) AS specialty,
    COUNT(*) AS n,
    SUM(c.monthly_trx) AS trx
FROM clean_survey AS c
JOIN physicians AS p
    ON p.physician_id = c.physician_id
GROUP BY TRIM(p.specialty)
ORDER BY trx DESC;
```

| specialty | n | trx |
|---|---:|---:|
| Oncologist | 10 | 100 |
| Primary Care | 10 | 97 |
| Neurologist | 9 | 75 |
| Cardiologist | 10 | 72 |

Oncology and primary care account for the majority of TRx. Again, n is small; the ranking is directional.

#### 23.10.9 Straight-liners and invalid responses

```sql
WITH ranked AS (
    SELECT s.*, ROW_NUMBER() OVER (
        PARTITION BY physician_id ORDER BY response_id
    ) AS rn
    FROM survey AS s
    WHERE response_date BETWEEN '2026-01-15' AND '2026-03-31'
),
clean_survey AS (SELECT * FROM ranked WHERE rn = 1)
SELECT
    c.response_id,
    c.physician_id,
    GROUP_CONCAT(i.item || '=' || i.score_raw, ', ') AS scores
FROM clean_survey AS c
JOIN survey_items AS i
    ON i.response_id = c.response_id
GROUP BY c.response_id, c.physician_id
HAVING SUM(CASE WHEN i.score_raw = '3' THEN 1 ELSE 0 END) = 3;
```

Result: **SR-022 / P-021**, with `Intent=3, Access=3, Efficacy=3`.

A straight-liner is a **flag**, not an automatic delete. The respondent may genuinely rate all three items neutral. The right action is:

1. Flag the response.
2. Run the headline with and without it.
3. If the conclusion changes, report the sensitivity.
4. Do not silently drop it to make the numbers cleaner.

#### 23.10.10 Response-level vs respondent-level

`survey_items` has 126 rows; `survey` has 42 responses. If you join them and compute `AVG(weight)`, you get three times the respondent-level average. If you compute `COUNT(*)`, you count items, not respondents.

**Rule:** decide the unit of analysis before you write the query.

| Unit | Table | Count | Use |
|---|---|---|---:|---|
| Response | `survey` | 42 raw / 39 clean | Response rate, quotas, NPS |
| Respondent | `survey` (deduped) | 40 unique / 39 clean | Physician-level analysis |
| Item response | `survey_items` | 126 raw / 115 valid | Top-2-box, item means |
| Weighted respondent | `survey` + weight | effective n 37.78 | Weighted estimates |

Valid item responses: Intent 39, Access 38, Efficacy 38. The difference is the two invalid records (Access 7, Efficacy N/A), not missing respondents.

#### 23.10.11 What to say in the interview

A strong survey readout has five sentences:

1. **Base:** “39 fielded respondents after removing two exact duplicates and one out-of-window response; effective weighted n is 37.8.”
2. **Headline:** “Top-2-box is 35.9% for Intent, 36.8% for Access, and 47.4% for Efficacy.”
3. **NPS:** “NPS is -7.69 unweighted and -6.81 weighted; 12 promoters, 12 passives, 15 detractors.”
4. **Segment:** “Oncology and primary care account for 100 and 97 TRx respectively; neurology and cardiology are lower.”
5. **Action:** “Access is the weakest item; test whether the issue is concentrated in a channel or region before changing the message.”

That is a defensible readout with the base, the number, the uncertainty, and the next question.

#### Check yourself — 23.10

1. What is the clean survey base after dedupe and field-window filtering?
2. What are the stage counts? Which stage is short of quota?
3. What is the top-2-box for Access, and what is the valid n?
4. What is NPS, and how many promoters/passives/detractors?
5. What is the weighted NPS, and how much does weighting move it?
6. What is the straight-liner response ID, and should it be deleted?
7. Why is `COUNT(*)` after joining `survey` to `survey_items` not the number of respondents?

Answers are in 23.15.

---
### 23.11 Commercial analytics: sales, prescriptions, and calls

Commercial analytics is not a pile of KPIs. It is a sequence of questions:

1. **Are we ahead or behind?** Revenue vs target.
2. **Where?** Region, product, cell.
3. **Why?** Price, volume, mix, calls, adoption.
4. **Is it durable?** Trend, retention, concentration.
5. **What should we do?** Recommendation, risk, next check.

#### 23.11.1 The headline KPI

```sql
SELECT
    SUM(revenue) AS revenue,
    SUM(target_revenue) AS target,
    ROUND(100.0 * SUM(revenue) / NULLIF(SUM(target_revenue), 0), 2) AS achievement_pct,
    SUM(CASE WHEN revenue >= target_revenue THEN 1 ELSE 0 END) AS rows_at_or_above_target,
    SUM(CASE WHEN target_revenue IS NOT NULL THEN 1 ELSE 0 END) AS rows_with_target
FROM sales;
```

| revenue | target | achievement_pct | rows_at_or_above_target | rows_with_target |
|---:|---:|---:|---:|---:|
| 2,683,320 | 2,717,035 | 98.76 | 28 | 70 |

The overall achievement is **98.76%**. Only **28 of 70** rows with a target are at or above target (40.0%). The total is close to target, but the row-level execution is mixed. That is a more useful insight than the headline alone.

#### 23.11.2 Q1 vs Q2

```sql
SELECT
    SUM(CASE WHEN month BETWEEN '2026-01-01' AND '2026-03-01' THEN revenue END) AS q1_revenue,
    SUM(CASE WHEN month BETWEEN '2026-04-01' AND '2026-06-01' THEN revenue END) AS q2_revenue,
    ROUND(
        100.0 * (
            SUM(CASE WHEN month BETWEEN '2026-04-01' AND '2026-06-01' THEN revenue END)
            - SUM(CASE WHEN month BETWEEN '2026-01-01' AND '2026-03-01' THEN revenue END)
        ) / SUM(CASE WHEN month BETWEEN '2026-01-01' AND '2026-03-01' THEN revenue END),
        2
    ) AS q1_to_q2_growth_pct
FROM sales;
```

| q1_revenue | q2_revenue | q1_to_q2_growth_pct |
|---:|---:|---:|
| 1,200,450 | 1,482,870 | 23.53 |

Q2 revenue is **23.53%** above Q1. Q2 achievement is **98.25%** (1,482,870 / 1,509,330). The growth is real, but the target also steps up; growth alone does not prove overperformance.

#### 23.11.3 Region and product achievement

```sql
SELECT
    r.region_name,
    SUM(s.revenue) AS revenue,
    SUM(s.target_revenue) AS target,
    ROUND(100.0 * SUM(s.revenue) / NULLIF(SUM(s.target_revenue), 0), 2) AS achievement_pct
FROM sales AS s
JOIN regions AS r
    ON r.region_id = s.region_id
GROUP BY r.region_name
ORDER BY achievement_pct DESC;
```

| region_name | revenue | target | achievement_pct |
|---|---:|---:|---:|
| East | 481,200 | 460,455 | 104.51 |
| West | 612,882 | 622,128 | 98.51 |
| North | 864,954 | 886,608 | 97.56 |
| South | 724,284 | 747,844 | 96.85 |

```sql
SELECT
    p.product_name,
    SUM(s.revenue) AS revenue,
    SUM(s.target_revenue) AS target,
    ROUND(100.0 * SUM(s.revenue) / NULLIF(SUM(s.target_revenue), 0), 2) AS achievement_pct
FROM sales AS s
JOIN products AS p
    ON p.product_id = s.product_id
GROUP BY p.product_name
ORDER BY achievement_pct DESC;
```

| product_name | revenue | target | achievement_pct |
|---|---:|---:|---:|
| OncoZ | 842,880 | 804,121 | 104.82 |
| NeuroY | 788,970 | 795,083 | 99.23 |
| CardioX | 1,051,470 | 1,117,831 | 94.06 |

The portfolio story:

- **East** is the only region above target, but it is the smallest region (17.93% of revenue).
- **CardioX** is the largest product (39.19% of revenue) but 5.94 points below target.
- **OncoZ** is the growth engine: 31.41% of revenue and 104.82% achievement.
- **NeuroY** is essentially on target (99.23%) but declining in the latest months: June revenue 123,900 vs January 134,400.

#### 23.11.4 Top cells and concentration

```sql
WITH cells AS (
    SELECT
        r.region_name,
        p.product_name,
        SUM(s.revenue) AS revenue
    FROM sales AS s
    JOIN regions AS r
        ON r.region_id = s.region_id
    JOIN products AS p
        ON p.product_id = s.product_id
    GROUP BY r.region_name, p.product_name
),
ranked AS (
    SELECT *,
        ROW_NUMBER() OVER (ORDER BY revenue DESC) AS rn,
        ROUND(100.0 * revenue / SUM(revenue) OVER (), 2) AS share_pct
    FROM cells
)
SELECT region_name, product_name, revenue, share_pct
FROM ranked
WHERE rn <= 3
ORDER BY rn;
```

| region_name | product_name | revenue | share_pct |
|---|---|---:|---:|
| North | CardioX | 337,134 | 12.56 |
| North | OncoZ | 281,280 | 10.48 |
| South | CardioX | 280,944 | 10.47 |

The top three cells account for **33.52%** of total revenue. That is a meaningful concentration: a problem in North CardioX or North OncoZ would move the total. The next question is whether the concentration is growing or stable, and whether the top cells are over-reliant on a few physicians or reps.

#### 23.11.5 Price–volume decomposition

A revenue change can come from price, volume, or the interaction. For CardioX April → May:

```sql
WITH p AS (
    SELECT
        month,
        SUM(units) AS units,
        SUM(revenue) AS revenue
    FROM sales AS s
    JOIN products AS pr
        ON pr.product_id = s.product_id
    WHERE pr.product_name = 'CardioX'
      AND month IN ('2026-04-01', '2026-05-01')
    GROUP BY month
),
x AS (
    SELECT
        MAX(CASE WHEN month = '2026-04-01' THEN units END) AS q1,
        MAX(CASE WHEN month = '2026-05-01' THEN units END) AS q2,
        MAX(CASE WHEN month = '2026-04-01' THEN revenue * 1.0 / units END) AS p1,
        MAX(CASE WHEN month = '2026-05-01' THEN revenue * 1.0 / units END) AS p2
    FROM p
)
SELECT
    q1, q2, p1, p2,
    (p2 - p1) * q1 AS price_effect,
    (q2 - q1) * p1 AS volume_effect,
    (p2 - p1) * (q2 - q1) AS interaction,
    (p2 * q2) - (p1 * q1) AS total_change
FROM x;
```

| q1 | q2 | p1 | p2 | price_effect | volume_effect | interaction | total_change |
|---:|---:|---:|---:|---:|---:|---:|---:|
| 1,459 | 1,445 | 120.0 | 126.0 | +8,754 | -1,680 | -84 | +6,990 |

The revenue increase is **price-led**: price +₹6/unit contributed +₹8,754, while volume fell 14 units (−₹1,680) and the interaction was −₹84. The net was +₹6,990. This is a crucial commercial insight: the growth is not demand-led. If the price increase is not sustainable, the revenue base may be fragile.

**Definition check:** this is a symmetric decomposition. Different textbooks allocate the interaction differently. State the convention. Do not present a price effect without saying whether the interaction is separate, allocated to price, or allocated to volume.

#### 23.11.6 Prescription trend and retention

```sql
WITH monthly AS (
    SELECT month, SUM(trx) AS trx
    FROM prescriptions
    GROUP BY month
)
SELECT
    month,
    trx,
    LAG(trx) OVER (ORDER BY month) AS previous_month,
    ROUND(
        100.0 * (trx - LAG(trx) OVER (ORDER BY month))
        / NULLIF(LAG(trx) OVER (ORDER BY month), 0),
        2
    ) AS mom_growth_pct,
    SUM(trx) OVER (
        ORDER BY month
        ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW
    ) AS running_total
FROM monthly
ORDER BY month;
```

| month | trx | previous_month | mom_growth_pct | running_total |
|---|---:|---:|---:|---:|
| 2026-01-01 | 208 | NULL | NULL | 208 |
| 2026-02-01 | 241 | 208 | 15.87 | 449 |
| 2026-03-01 | 273 | 241 | 13.28 | 722 |
| 2026-04-01 | 306 | 273 | 12.09 | 1,028 |
| 2026-05-01 | 327 | 306 | 6.86 | 1,355 |
| 2026-06-01 | 344 | 327 | 5.20 | 1,699 |

Total TRx is **1,699**; June is **344**. January-to-June growth is **65.38%** (344 / 208 − 1). The growth rate is decelerating: 15.87% → 13.28% → 12.09% → 6.86% → 5.20%. That does not mean the brand is failing; it means the base is getting larger and the incremental growth is harder. The right question is whether the June level is sustainable.

**Retention:**

```sql
WITH active AS (
    SELECT DISTINCT physician_id, month FROM prescriptions
),
january AS (
    SELECT physician_id FROM active WHERE month = '2026-01-01'
),
june AS (
    SELECT physician_id FROM active WHERE month = '2026-06-01'
)
SELECT
    COUNT(*) AS january_active,
    COUNT(j.physician_id) AS retained_in_june,
    ROUND(100.0 * COUNT(j.physician_id) / COUNT(*), 1) AS retention_pct
FROM january AS a
LEFT JOIN june AS j
    ON j.physician_id = a.physician_id;
```

| january_active | retained_in_june | retention_pct |
|---:|---:|---:|
| 16 | 14 | 87.5 |

Two physicians stop after April. The 14 retained physicians are the core prescriber base. The business question is whether the two lapsed physicians are a churn signal or a data artifact; in this fictional dataset, they are deliberate churn.

#### 23.11.7 June product mix

```sql
SELECT
    p.product_name,
    SUM(r.trx) AS trx,
    ROUND(
        100.0 * SUM(r.trx) / SUM(SUM(r.trx)) OVER (),
        2
    ) AS share_pct
FROM prescriptions AS r
JOIN products AS p
    ON p.product_id = r.product_id
WHERE r.month = '2026-06-01'
GROUP BY p.product_name
ORDER BY trx DESC;
```

| product_name | trx | share_pct |
|---|---:|---:|
| OncoZ | 124 | 36.05 |
| CardioX | 115 | 33.43 |
| NeuroY | 105 | 30.52 |

OncoZ has the highest June TRx share, consistent with its sales overachievement. CardioX has the largest sales revenue but a lower TRx share because its price per unit is lower than OncoZ. Revenue share and TRx share are different denominators; do not conflate them.

#### 23.11.8 Calls: activity, not impact

```sql
SELECT
    COUNT(*) AS calls,
    SUM(CASE WHEN call_type = 'F2F' THEN 1 ELSE 0 END) AS f2f_calls,
    SUM(CASE WHEN call_type = 'Remote' THEN 1 ELSE 0 END) AS remote_calls,
    ROUND(100.0 * SUM(CASE WHEN call_type = 'F2F' THEN 1 ELSE 0 END) / COUNT(*), 1) AS f2f_pct,
    ROUND(AVG(duration_min), 2) AS avg_duration_min,
    COUNT(DISTINCT physician_id) AS physicians,
    ROUND(COUNT(*) * 1.0 / COUNT(DISTINCT physician_id), 2) AS calls_per_physician
FROM calls;
```

| calls | f2f_calls | remote_calls | f2f_pct | avg_duration_min | physicians | calls_per_physician |
|---:|---:|---:|---:|---:|---:|---:|
| 120 | 80 | 40 | 66.7 | 36.13 | 40 | 3.00 |

The call base is evenly distributed: 20 calls per month, 30 per region, 10 per rep, 3 per physician. That evenness is a design feature of the fictional dataset, not a market finding. In a real dataset, call coverage is usually skewed.

**By call type:**

| call_type | n | pct | avg_min |
|---|---:|---:|---:|
| F2F | 80 | 66.7 | 36.56 |
| Remote | 40 | 33.3 | 35.25 |

**By region:**

| region_name | calls | f2f | remote | avg_min |
|---|---:|---:|---:|---:|
| East | 30 | 20 | 10 | 38.00 |
| North | 30 | 20 | 10 | 33.50 |
| South | 30 | 20 | 10 | 39.50 |
| West | 30 | 20 | 10 | 33.50 |

South has the longest average call (39.50 minutes); North and West have the shortest (33.50). Do **not** claim that longer calls cause higher sales. The dataset is synthetic and the call plan is balanced. The correct statement is: “South has longer calls; the next check is whether those calls are concentrated on high-value physicians or products.”

#### 23.11.9 Putting the commercial story together

A defensible one-page readout:

1. **Total:** ₹2,683,320 revenue, 98.76% of target, 28 of 70 rows at or above target.
2. **Where:** East is 104.51%; South is 96.85%. CardioX is 94.06%; OncoZ is 104.82%.
3. **Growth:** Q2 is 23.53% above Q1; June is the strongest month at ₹540,300.
4. **Concentration:** the top three region-product cells are 33.52% of revenue; North CardioX alone is 12.56%.
5. **Price/volume:** CardioX April→May growth is price-led (+₹8,754 price, −₹1,680 volume, −₹84 interaction).
6. **Rx:** 1,699 total TRx; June 344; 65.38% growth January→June; 87.5% retention.
7. **Survey:** Access is the weakest top-2-box item (36.84%); NPS is -7.69.
8. **Action:** protect the OncoZ growth, diagnose CardioX’s 94.06% achievement, and test the access barrier before changing the promotional message.

That is not a data dump. It is a decision-oriented narrative with numbers, caveats, and a next step.

#### Check yourself — 23.11

1. What is the overall achievement? How many rows are at or above target?
2. What is Q2 revenue, and how much did it grow vs Q1?
3. Which region and which product are the only ones above target?
4. What are the top three region-product cells, and what share of total revenue do they represent?
5. What is the price effect and volume effect for CardioX April→May?
6. What is June TRx, and what is the January→June growth?
7. What is the retention rate from January to June?
8. What is the F2F call share, and why should you not claim causation?

Answers are in 23.15.

---
## Part E — Performance and interview craft

### 23.12 Performance, readability, and dialect map

You will rarely be asked to tune a query in an entry-level interview. You will be asked why a query is slow, why it returns the wrong number, or how you would make it production-safe. The answers are usually about **grain and filters**, not exotic tuning.

#### 23.12.1 Sargable predicates

A predicate is **sargable** if the database can use an index to evaluate it. Wrapping a column in a function usually makes it non-sargable.

```sql
-- Sargable: the column is bare
WHERE month >= '2026-04-01'
  AND month <  '2026-07-01'
```

```sql
-- Non-sargable: the column is wrapped in a function
WHERE strftime('%Y-%m', month) = '2026-04'
```

The second version prevents a normal index on `month` from being used efficiently. If you need a month label, compute it in `SELECT`, not in `WHERE`.

#### 23.12.2 Indexes

```sql
CREATE INDEX idx_sales_month ON sales(month);
CREATE INDEX idx_rx_phys_month ON prescriptions(physician_id, month);
CREATE INDEX idx_calls_phys ON calls(physician_id);
```

Indexes speed up reads and slow down writes. For an analytics interview, the important point is the **column order** in a composite index: put the equality filter first, then the range/sort column. For `prescriptions`, `(physician_id, month)` supports “this physician over time”; `(month, physician_id)` supports “this month across physicians” better.

#### 23.12.3 EXPLAIN

SQLite:

```sql
EXPLAIN QUERY PLAN
SELECT *
FROM prescriptions
WHERE physician_id = 'P-040'
  AND month >= '2026-01-01';
```

Look for `SEARCH` (index used) vs `SCAN` (full table). PostgreSQL uses `EXPLAIN ANALYZE`; SQL Server uses `SET SHOWPLAN_ALL ON`; MySQL uses `EXPLAIN`. The exact output differs; the question is the same: is the engine reading the whole table or narrowing first?

#### 23.12.4 Readability rules that also help performance

1. **Filter early.** Put the date window in the first CTE, not after three joins.
2. **Aggregate before joining** when the join would multiply rows. For example, aggregate `survey_items` to response level before joining to `survey`.
3. **Avoid `SELECT DISTINCT` as a join fix.** If you need `DISTINCT` after a join, you probably have a fan-out.
4. **Use `UNION ALL` unless you need deduplication.** `UNION` sorts/hashes to remove duplicates.
5. **Avoid leading wildcards.** `LIKE '%onco%'` cannot use a normal index.
6. **Do not use `SELECT *` in a production query.** It reads columns you do not need and breaks when the schema changes.
7. **Use `EXISTS` for existence checks.** It can stop at the first match.
8. **Use `COUNT(DISTINCT)` deliberately.** It is more expensive than `COUNT(*)`; only use it when the question is distinct entities.
9. **Use a CTE per grain.** `clean_survey` (respondent), `monthly_rx` (month), `top_cells` (region × product).
10. **Comment the business rule.** “Field window 2026-01-15 to 2026-03-31” is more important than a comment that repeats the syntax.

#### 23.12.5 Dialect map

| Feature | SQLite (this pack) | PostgreSQL | MySQL 8+ | SQL Server |
|---|---|---|---|---|
| Limit rows | `LIMIT 10` | `LIMIT 10` | `LIMIT 10` | `TOP 10` / `OFFSET … FETCH` |
| Date truncation | `strftime('%Y-%m', d)` | `DATE_TRUNC('month', d)` | `DATE_FORMAT(d, '%Y-%m')` | `DATEFROMPARTS` / `FORMAT` |
| Current date | `date('now')` | `CURRENT_DATE` | `CURDATE()` | `CAST(GETDATE() AS date)` |
| String concat | `||` | `||` | `CONCAT()` | `+` / `CONCAT()` |
| Cast | `CAST(x AS REAL)` | `CAST(x AS numeric)` / `x::numeric` | `CAST(x AS DECIMAL)` | `CAST(x AS DECIMAL)` |
| Null fallback | `COALESCE` / `IFNULL` | `COALESCE` | `COALESCE` / `IFNULL` | `COALESCE` / `ISNULL` |
| Case-insensitive LIKE | `LIKE` (ASCII default) | `ILIKE` | `LIKE` (collation-dependent) | `LIKE` (collation-dependent) |
| Group concat | `GROUP_CONCAT` | `STRING_AGG` | `GROUP_CONCAT` | `STRING_AGG` |
| Window functions | 3.25+ | Yes | 8.0+ | Yes |
| `QUALIFY` | No | No | No | No (Snowflake/BigQuery yes) |
| Full outer join | 3.39+ | Yes | Emulate with `UNION` | Yes |
| Recursive CTE | `WITH RECURSIVE` | `WITH RECURSIVE` | `WITH RECURSIVE` | `WITH` (recursive by default) |
| Explain | `EXPLAIN QUERY PLAN` | `EXPLAIN ANALYZE` | `EXPLAIN` | `SET SHOWPLAN_ALL ON` |

**Interview rule:** if the interviewer asks for a specific dialect, use it. If not, say “I’ll write standard SQL; in SQLite the date function would be X, in PostgreSQL it would be Y.” That is better than silently writing syntax that does not run.

#### 23.12.6 How to talk through a query

Use a four-beat structure:

1. **Restate the question and grain.** “I’m returning one row per region-product for Q2, with revenue and achievement.”
2. **State the cleaning rules.** “I’m excluding the two blank targets from achievement, but keeping them in revenue.”
3. **Write the query in steps.** “First the Q2 filter, then the join, then the aggregate, then the rank.”
4. **Check the result.** “The row count is 12, which matches 4 regions × 3 products; the total reconciles to 1,482,870.”

If you get stuck, narrate the grain and the join. Interviewers care more about structured thinking than perfect syntax.

---

### 23.13 Twenty interview patterns with solutions

These are the patterns that recur in analyst SQL screens. Each is shown against the practice database where possible. For patterns that are generic, the query is the answer; the expected output is marked “pattern only”.

#### Pattern 1 — Second-highest product by revenue

```sql
WITH product_revenue AS (
    SELECT
        p.product_name,
        SUM(s.revenue) AS revenue
    FROM sales AS s
    JOIN products AS p
        ON p.product_id = s.product_id
    GROUP BY p.product_name
),
ranked AS (
    SELECT
        product_name,
        revenue,
        DENSE_RANK() OVER (ORDER BY revenue DESC) AS rnk
    FROM product_revenue
)
SELECT product_name, revenue
FROM ranked
WHERE rnk = 2;
```

**Expected:** OncoZ, **842,880**. CardioX is first (1,051,470); NeuroY is third (788,970). Use `DENSE_RANK` if ties should share a rank; use `ROW_NUMBER` if you need exactly one row.

#### Pattern 2 — Nth-highest with a parameter

```sql
WITH product_revenue AS (
    SELECT p.product_name, SUM(s.revenue) AS revenue
    FROM sales AS s
    JOIN products AS p ON p.product_id = s.product_id
    GROUP BY p.product_name
)
SELECT product_name, revenue
FROM product_revenue
ORDER BY revenue DESC
LIMIT 1 OFFSET 1;   -- 0 = highest, 1 = second-highest
```

**Expected for OFFSET 1:** OncoZ, 842,880. The `OFFSET` approach is simple but does not handle ties consistently. For ties, use `DENSE_RANK` and filter `rnk = :n`.

#### Pattern 3 — Find duplicates

```sql
SELECT
    physician_id,
    COUNT(*) AS n,
    GROUP_CONCAT(response_id) AS response_ids
FROM survey
GROUP BY physician_id
HAVING COUNT(*) > 1;
```

**Expected:** P-007 (SR-007, SR-008) and P-023 (SR-024, SR-025). The `HAVING` is essential; `WHERE COUNT(*) > 1` is invalid because `WHERE` runs before aggregation.

#### Pattern 4 — Delete duplicates, keep one

Do this in a transaction, and only after you have a backup.

```sql
BEGIN;

DELETE FROM survey
WHERE rowid NOT IN (
    SELECT MIN(rowid)
    FROM survey
    GROUP BY physician_id
);

SELECT COUNT(*) FROM survey;  -- 40
ROLLBACK;                      -- undo in practice; COMMIT in production
```

**Expected inside the transaction:** 40 rows. The `rowid` is SQLite-specific; in PostgreSQL use `ctid` or a primary key, and in SQL Server use a CTE with `ROW_NUMBER()`.

**Interview warning:** deleting duplicates is a destructive operation. Say “I would first select the duplicates, confirm the rule with the business, then delete in a transaction.” Do not start with `DELETE`.

#### Pattern 5 — Rows in A but not B

```sql
SELECT p.physician_id, p.physician_name
FROM physicians AS p
LEFT JOIN calls AS c
    ON c.physician_id = p.physician_id
WHERE c.call_id IS NULL
ORDER BY p.physician_id;
```

**Expected:** P-041, P-042. The `NOT EXISTS` version is safer when the right side can have NULL keys:

```sql
SELECT p.physician_id, p.physician_name
FROM physicians AS p
WHERE NOT EXISTS (
    SELECT 1 FROM calls AS c
    WHERE c.physician_id = p.physician_id
)
ORDER BY p.physician_id;
```

#### Pattern 6 — Above the group average

```sql
SELECT s.sale_id, s.region_id, s.product_id, s.revenue
FROM sales AS s
WHERE s.revenue > (
    SELECT AVG(s2.revenue)
    FROM sales AS s2
    WHERE s2.region_id = s.region_id
)
ORDER BY s.region_id, s.revenue DESC;
```

**Expected:** **34 rows** above their region average. The window version is faster:

```sql
WITH x AS (
    SELECT s.*, AVG(s.revenue) OVER (PARTITION BY region_id) AS region_avg
    FROM sales AS s
)
SELECT sale_id, region_id, product_id, revenue
FROM x
WHERE revenue > region_avg
ORDER BY region_id, revenue DESC;
```

#### Pattern 7 — Top N per group

See 23.6.4 for the full query. The shape is:

```sql
WITH ranked AS (
    SELECT
        group_col,
        value_col,
        ROW_NUMBER() OVER (
            PARTITION BY group_col
            ORDER BY value_col DESC
        ) AS rn
    FROM base
)
SELECT *
FROM ranked
WHERE rn <= 3;
```

**Expected:** 12 rows for top 3 physicians per specialty in June. The tie-breaker in `ORDER BY` is what makes it reproducible.

#### Pattern 8 — Running total

```sql
WITH monthly AS (
    SELECT month, SUM(trx) AS trx
    FROM prescriptions
    GROUP BY month
)
SELECT
    month,
    trx,
    SUM(trx) OVER (
        ORDER BY month
        ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW
    ) AS running_total
FROM monthly
ORDER BY month;
```

**Expected:** 208 → 449 → 722 → 1,028 → 1,355 → 1,699. The last value must equal `SUM(trx)`.

#### Pattern 9 — Month-over-month growth

```sql
WITH monthly AS (
    SELECT month, SUM(trx) AS trx
    FROM prescriptions
    GROUP BY month
)
SELECT
    month,
    trx,
    LAG(trx) OVER (ORDER BY month) AS previous_month,
    ROUND(
        100.0 * (trx - LAG(trx) OVER (ORDER BY month))
        / NULLIF(LAG(trx) OVER (ORDER BY month), 0),
        2
    ) AS mom_growth_pct
FROM monthly
ORDER BY month;
```

**Expected:** NULL, 15.87, 13.28, 12.09, 6.86, 5.20. The first NULL is correct; do not force it to zero.

#### Pattern 10 — Moving average

```sql
WITH monthly AS (
    SELECT month, SUM(trx) AS trx
    FROM prescriptions
    GROUP BY month
)
SELECT
    month,
    trx,
    ROUND(AVG(trx) OVER (
        ORDER BY month
        ROWS BETWEEN 2 PRECEDING AND CURRENT ROW
    ), 2) AS ma_3m
FROM monthly
ORDER BY month;
```

**Expected June:** **325.67**. The first two months use partial windows; that is correct.

#### Pattern 11 — Percent of total

```sql
SELECT
    p.product_name,
    SUM(r.trx) AS trx,
    ROUND(100.0 * SUM(r.trx) / SUM(SUM(r.trx)) OVER (), 2) AS share_pct
FROM prescriptions AS r
JOIN products AS p
    ON p.product_id = r.product_id
WHERE r.month = '2026-06-01'
GROUP BY p.product_name
ORDER BY trx DESC;
```

**Expected:** OncoZ 36.05%, CardioX 33.43%, NeuroY 30.52%. The denominator is June total TRx (344), not annual TRx.

#### Pattern 12 — Pivot with conditional aggregation

```sql
SELECT
    p.product_name,
    SUM(CASE WHEN s.month = '2026-01-01' THEN s.revenue ELSE 0 END) AS jan,
    SUM(CASE WHEN s.month = '2026-02-01' THEN s.revenue ELSE 0 END) AS feb,
    SUM(CASE WHEN s.month = '2026-03-01' THEN s.revenue ELSE 0 END) AS mar
FROM sales AS s
JOIN products AS p
    ON p.product_id = s.product_id
GROUP BY p.product_name
ORDER BY p.product_name;
```

**Expected CardioX Q1:** January 157,200; February 155,400; March 175,080. For June, CardioX is 206,640.

#### Pattern 13 — Median

SQLite has no `MEDIAN` function. Use a window:

```sql
WITH june AS (
    SELECT physician_id, SUM(trx) AS trx
    FROM prescriptions
    WHERE month = '2026-06-01'
    GROUP BY physician_id
),
ranked AS (
    SELECT
        trx,
        ROW_NUMBER() OVER (ORDER BY trx) AS rn,
        COUNT(*) OVER () AS n
    FROM june
)
SELECT ROUND(AVG(trx), 2) AS median_trx
FROM ranked
WHERE rn IN ((n + 1) / 2, (n + 2) / 2);
```

**Expected:** **22.00** for June physician-level TRx. The `(n+1)/2` and `(n+2)/2` pair handles both odd and even row counts. PostgreSQL has `PERCENTILE_CONT`, but the window pattern is portable.

#### Pattern 14 — NTILE quartiles

```sql
WITH june AS (
    SELECT physician_id, SUM(trx) AS trx
    FROM prescriptions
    WHERE month = '2026-06-01'
    GROUP BY physician_id
)
SELECT
    physician_id,
    trx,
    NTILE(4) OVER (ORDER BY trx) AS quartile
FROM june
ORDER BY trx;
```

**Expected:** 14 physicians split 4/4/3/3. The lowest quartile is 8–14 TRx; the highest is 40–48. Uneven buckets are normal when `n` is not divisible by 4.

#### Pattern 15 — Self-join pairs

```sql
SELECT
    a.region_id,
    a.rep_name AS rep_a,
    b.rep_name AS rep_b
FROM reps AS a
JOIN reps AS b
    ON a.region_id = b.region_id
   AND a.rep_id < b.rep_id
ORDER BY a.region_id, a.rep_name, b.rep_name;
```

**Expected:** **12 pairs** (3 pairs per region × 4 regions). The inequality avoids self-pairs and duplicate reversed pairs.

#### Pattern 16 — Gaps and islands

See 23.8.6 for the full query. The key idea:

```sql
month_num - ROW_NUMBER() OVER (PARTITION BY physician_id ORDER BY month)
```

Consecutive months produce a constant value. Group by that value to get each island. **Expected:** 14 physicians with one 6-month island; P-041 and P-042 with one 4-month island.

#### Pattern 17 — Cohort retention

```sql
WITH active AS (
    SELECT DISTINCT physician_id, month FROM prescriptions
),
january AS (
    SELECT physician_id FROM active WHERE month = '2026-01-01'
),
june AS (
    SELECT physician_id FROM active WHERE month = '2026-06-01'
)
SELECT
    COUNT(*) AS january_active,
    COUNT(j.physician_id) AS retained_in_june,
    ROUND(100.0 * COUNT(j.physician_id) / COUNT(*), 1) AS retention_pct
FROM january AS a
LEFT JOIN june AS j
    ON j.physician_id = a.physician_id;
```

**Expected:** 16 → 14, **87.5%**. Anchor on the cohort, not on the later month.

#### Pattern 18 — First and last activity

```sql
SELECT
    physician_id,
    MIN(month) AS first_month,
    MAX(month) AS last_month,
    COUNT(DISTINCT month) AS months_active
FROM prescriptions
GROUP BY physician_id
ORDER BY physician_id;
```

**Expected:** 14 physicians with January–June (6 months); P-041 and P-042 with January–April (4 months). This is the churn list.

#### Pattern 19 — Fan-out detection

```sql
SELECT
    (SELECT COUNT(*) FROM survey) AS survey_rows,
    (SELECT COUNT(*) FROM survey_items) AS item_rows,
    (SELECT COUNT(*) FROM survey s JOIN survey_items i ON i.response_id = s.response_id) AS joined_rows;
```

**Expected:** 42, 126, 126. The join is one-to-many, so the row count is correct for an item-level question but wrong for a respondent-level average. Always compare the join count with the expected grain.

#### Pattern 20 — Reconciliation

```sql
SELECT
    (SELECT SUM(revenue) FROM sales) AS revenue,
    (SELECT SUM(units * unit_price) FROM sales) AS computed_revenue,
    (SELECT SUM(revenue) FROM sales) - (SELECT SUM(units * unit_price) FROM sales) AS difference,
    (SELECT SUM(revenue) FROM sales) - (SELECT SUM(target_revenue) FROM sales) AS target_variance;
```

**Expected:** revenue 2,683,320; computed 2,683,320; difference **0**; target variance **-33,715**. The first difference checks arithmetic; the second checks performance vs target. They answer different questions.

---
## Part F — Practice

### 23.14 Timed drills (questions only)

Do these with a timer and without looking at 23.15. Write your query, run it, and check the row count before comparing the number. If you finish early, rewrite the query with a CTE and add a data-quality check.

#### Drill 1 — Foundations and aggregation (20 minutes)

1. List all products with their therapeutic area and launch year, newest first.
2. Return the number of sales rows, total revenue, and total target revenue.
3. Return revenue, target, and achievement % by region, sorted by achievement descending.
4. Return total units and the realized revenue per unit (`SUM(revenue) / SUM(units)`).
5. How many sales rows have a blank target? How many have a blank call count?
6. Which month has the highest revenue, and what is that revenue?
7. Which product has the highest revenue, and what share of total revenue is it?
8. How many sales rows are at or above target, and what percentage of rows with a target is that?

#### Drill 2 — Joins and CTEs (25 minutes)

1. Build the region × product revenue matrix. Which cell is largest?
2. Which physicians have no calls? Return their IDs and names.
3. Return calls, F2F calls, Remote calls, and F2F % by region.
4. Which products have Q2 revenue above 500,000?
5. How many sales rows are above their own region’s average revenue?
6. Write a CTE that returns the clean survey base (dedupe + field window). How many rows?
7. Join the clean survey base to `survey_items`. How many rows and what is the weight sum? Why is the weight sum not 42.60?

#### Drill 3 — Windows and trends (30 minutes)

1. Rank products by total revenue. Which product is second?
2. Return the top three region-product cells by revenue with a rank.
3. Return monthly TRx, previous month, and month-over-month growth for January–June.
4. Return monthly TRx and a running total. What is the final running total?
5. Return monthly TRx and a three-month moving average. What is the June value?
6. Return June TRx by product and each product’s share of June TRx.
7. Return the top three physicians per specialty by June TRx.
8. What is the January-to-June retention rate? Which physicians churned?

#### Drill 4 — Survey, QA, and commercial (35 minutes)

1. Find the duplicate survey respondents. Return the physician ID, response count, and response IDs.
2. Find the invalid Likert records. Return response ID, physician ID, item, and raw score.
3. Find the out-of-window response and the blank TRx response.
4. Return stage counts and quota variance. Which stage is short?
5. Return top-2-box and NPS from the clean survey base.
6. Return Q2 revenue, target, and achievement by region, sorted by achievement descending.
7. Decompose the CardioX April→May revenue change into price, volume, and interaction effects.
8. Write three insights with numbers: one commercial, one survey, one data-quality/next step.

---

### 23.15 Answer key: section checks and drills

This section is deliberately after the drills. Run your query first; then compare the **grain**, the **row count**, and the **number**.

#### 23.15.1 Answers to 23.1 check

**1.** `SELECT COUNT(*) FROM products;` → **3**.

**2.** `SELECT DISTINCT specialty` returns five raw values because **`Oncologist` and `Oncologist ` (trailing space)** are different strings. `SELECT DISTINCT TRIM(specialty)` returns four.

**3.**

```sql
SELECT product_name, therapeutic_area, launch_year
FROM products
ORDER BY launch_year DESC;
```

Expected: OncoZ 2024, NeuroY 2021, CardioX 2018.

**4.** `LIMIT 5` without `ORDER BY` returns an arbitrary five rows; there is no defined “top”. A top-5 query needs a metric and a sort.

#### 23.15.2 Answers to 23.2 check

**1.**

```sql
SELECT COUNT(*) AS blank_calls
FROM sales
WHERE calls IS NULL;
```

→ **3**.

**2.** `COUNT(calls)` → **69**. It counts non-NULL values; three rows are NULL.

**3.** `WHERE calls <> 0` → **69** because the three NULL rows evaluate to unknown and are excluded. `WHERE calls <> 0 OR calls IS NULL` would return 72.

**4.** `WHERE target_revenue IS NULL` (not `= NULL`).

**5.** `COALESCE(target_revenue, 0)` replaces NULL with 0 for display. `NULLIF(target_revenue, 0)` returns NULL when the value is 0, which is used to avoid division by zero. They solve different problems.

#### 23.15.3 Answers to 23.3 check

**1.** `SELECT SUM(units) FROM sales;` → **14,121**.

**2.** June, **540,300**.

**3.** East, **104.51%**.

**4.** **28** rows at or above target out of **70** rows with a target → **40.0%**.

**5.** Average monthly revenue = **447,220.00** (2,683,320 / 6). Average row revenue = **37,268.33** (2,683,320 / 72). They differ because one averages months and the other averages rows; the denominators are different.

#### 23.15.4 Answers to 23.4 check

**1.** **72** rows. `regions` is unique on `region_id`, so the join is one-to-many from region to sales and does not multiply the fact.

**2.** **126** rows. `survey` has 42 responses; `survey_items` has 3 items per response; 42 × 3 = 126.

**3.**

```sql
SELECT p.physician_id, p.physician_name
FROM physicians AS p
LEFT JOIN calls AS c ON c.physician_id = p.physician_id
WHERE c.call_id IS NULL
ORDER BY p.physician_id;
```

→ **P-041, P-042**.

**4.** A `WHERE` condition on the right table evaluates to unknown for unmatched left rows, so those rows are removed. Put the right-side filter in the `ON` clause if you want to keep unmatched left rows.

**5.** The join multiplies each respondent by three items. A respondent-level average or weight sum will be counted three times unless you aggregate items first or use `COUNT(DISTINCT)`.

#### 23.15.5 Answers to 23.5 check

**1.** **32** rows above the overall average row revenue (37,268.33).

**2.** **34** rows above their own region average.

**3.** **39**. Raw 42 → remove two duplicate respondents (P-007, P-023) → 40 → remove one out-of-window response (P-030) → 39.

**4.** CardioX (563,790) and OncoZ (531,840).

**5.** If the subquery returns a NULL, `NOT IN` can evaluate to unknown for every row and return no rows. `NOT EXISTS` is a direct existence test and is NULL-safe.

#### 23.15.6 Answers to 23.6 check

**1.** CardioX 1,051,470 (1); OncoZ 842,880 (2); NeuroY 788,970 (3). Second is **OncoZ**.

**2.** North CardioX 337,134; North OncoZ 281,280; South CardioX 280,944.

**3.** **12 rows** (3 per specialty × 4 specialties). The full table is in 23.6.4.

**4.** **1,699**. The running total at the last month must equal `SUM(trx)` because the window frame includes all prior rows and the current row.

**5.** **325.67** (average of April 306, May 327, June 344).

**6.** `ROW_NUMBER` gives a unique sequence (ties broken arbitrarily unless you add a tie-breaker); `RANK` gives ties the same rank and leaves gaps; `DENSE_RANK` gives ties the same rank without gaps.

#### 23.15.7 Answers to 23.7 check

**1.** **2** rows in the High band (revenue ≥ 60,000).

**2.** CardioX June = **206,640**.

**3.** Access top-2-box = **36.84%**, valid n = **38**.

**4.** NPS = **-7.69**; 12 promoters, 12 passives, 15 detractors.

**5.** Without `ELSE 0`, non-matching rows return NULL. `SUM` ignores NULL, which often gives the same total, but `COUNT` and arithmetic behave differently. `ELSE 0` makes the intent explicit and avoids surprises.

#### 23.15.8 Answers to 23.8 check

**1.** **151 days**.

**2.** Before dedupe: **41** rows in the field window; after dedupe: **39**.

**3.** **87.5%** (14 / 16). P-041 and P-042 churned after April.

**4.** May and June each have **14** active physicians, down from 16 in January–April.

**5.** A `GROUP BY month` omits months with no rows. A date spine forces a row for every month, so zero months appear as zero rather than disappearing.

#### 23.15.9 Answers to 23.9 check

**1.** Two duplicate respondents: **P-007** (SR-007, SR-008) and **P-023** (SR-024, SR-025).

**2.** **SR-020 / P-019 / Access / 7** and **SR-021 / P-020 / Efficacy / N/A**.

**3.** Total variance **-33,715**. East is the only positive region: **+20,745**.

**4.** Trial stage actual 7 vs target 8 → **-1**.

**5.** A zero reconciliation only proves that `revenue = units × unit_price` internally. It does not prove that the units, price, or revenue are correct, complete, or comparable across sources.

#### 23.15.10 Answers to 23.10 check

**1.** **39**.

**2.** 8 unaided, 8 aided, 8 consideration, 7 trial, 8 regular. **Trial** is short by one.

**3.** Access top-2-box **36.84%**, valid n **38**.

**4.** NPS **-7.69**; 12 promoters, 12 passives, 15 detractors.

**5.** Weighted NPS **-6.81**; weighting moves it **+0.88** toward zero.

**6.** **SR-022 / P-021**, scores 3/3/3. It should be flagged and tested for sensitivity, not automatically deleted.

**7.** The join has three item rows per response. `COUNT(*)` counts item rows; use `COUNT(DISTINCT response_id)` or aggregate items first to count respondents.

#### 23.15.11 Answers to 23.11 check

**1.** **98.76%**; **28 of 70** rows at or above target (40.0%).

**2.** Q2 revenue **1,482,870**; Q1→Q2 growth **23.53%**.

**3.** Region: **East** (104.51%). Product: **OncoZ** (104.82%).

**4.** North CardioX 337,134 (12.56%); North OncoZ 281,280 (10.48%); South CardioX 280,944 (10.47%). Top three = **33.52%** of total.

**5.** Price effect **+8,754**; volume effect **-1,680**; interaction **-84**; total **+6,990**.

**6.** June TRx **344**; January→June growth **65.38%**.

**7.** **87.5%** (14 / 16).

**8.** F2F share **66.7%** (80 / 120). Do not claim causation: calls are not randomly assigned, the dataset is synthetic, and high-value physicians may receive more calls.

#### 23.15.12 Drill 1 answers

**1.**

```sql
SELECT product_name, therapeutic_area, launch_year
FROM products
ORDER BY launch_year DESC;
```

→ OncoZ 2024, NeuroY 2021, CardioX 2018.

**2.**

```sql
SELECT COUNT(*) AS rows, SUM(revenue) AS revenue, SUM(target_revenue) AS target
FROM sales;
```

→ 72; 2,683,320; 2,717,035.

**3.**

```sql
SELECT r.region_name, SUM(s.revenue) AS revenue, SUM(s.target_revenue) AS target,
       ROUND(100.0 * SUM(s.revenue) / NULLIF(SUM(s.target_revenue), 0), 2) AS achievement_pct
FROM sales AS s
JOIN regions AS r ON r.region_id = s.region_id
GROUP BY r.region_name
ORDER BY achievement_pct DESC;
```

→ East 104.51; West 98.51; North 97.56; South 96.85.

**4.**

```sql
SELECT SUM(units) AS units, ROUND(SUM(revenue) * 1.0 / SUM(units), 2) AS revenue_per_unit
FROM sales;
```

→ 14,121 units; **190.02** revenue per unit.

**5.**

```sql
SELECT
  SUM(target_revenue IS NULL) AS blank_targets,
  SUM(calls IS NULL) AS blank_calls
FROM sales;
```

→ 2 blank targets; 3 blank calls.

**6.**

```sql
SELECT month, SUM(revenue) AS revenue
FROM sales
GROUP BY month
ORDER BY revenue DESC
LIMIT 1;
```

→ June 2026, **540,300**.

**7.**

```sql
WITH t AS (SELECT SUM(revenue) AS total FROM sales)
SELECT p.product_name, SUM(s.revenue) AS revenue,
       ROUND(100.0 * SUM(s.revenue) / (SELECT total FROM t), 2) AS share_pct
FROM sales AS s
JOIN products AS p ON p.product_id = s.product_id
GROUP BY p.product_name
ORDER BY revenue DESC
LIMIT 1;
```

→ CardioX, **1,051,470**, **39.19%**.

**8.**

```sql
SELECT
  SUM(revenue >= target_revenue) AS at_or_above,
  SUM(target_revenue IS NOT NULL) AS with_target,
  ROUND(100.0 * SUM(revenue >= target_revenue) / SUM(target_revenue IS NOT NULL), 1) AS pct
FROM sales;
```

→ 28 / 70 = **40.0%**.

#### 23.15.13 Drill 2 answers

**1.** The matrix query is in 23.4.2. Largest cell: **North × CardioX = 337,134**.

**2.**

```sql
SELECT p.physician_id, p.physician_name
FROM physicians AS p
LEFT JOIN calls AS c ON c.physician_id = p.physician_id
WHERE c.call_id IS NULL
ORDER BY p.physician_id;
```

→ P-041, P-042.

**3.**

```sql
SELECT r.region_name,
       COUNT(*) AS calls,
       SUM(c.call_type = 'F2F') AS f2f,
       SUM(c.call_type = 'Remote') AS remote,
       ROUND(100.0 * SUM(c.call_type = 'F2F') / COUNT(*), 1) AS f2f_pct
FROM calls AS c
JOIN reps AS rp ON rp.rep_id = c.rep_id
JOIN regions AS r ON r.region_id = rp.region_id
GROUP BY r.region_name
ORDER BY r.region_name;
```

→ East 30/20/10/66.7; North 30/20/10/66.7; South 30/20/10/66.7; West 30/20/10/66.7.

**4.** CardioX 563,790; OncoZ 531,840. NeuroY 387,240 is below 500,000.

**5.** **34 rows** above their region average.

**6.** The clean base is **39** rows. Query in 23.5.5 or 23.10.2.

**7.** Joined rows **126**; weight sum **127.80**. The clean base weight sum is 42.60; the join multiplies each respondent by three items, so 42.60 × 3 = 127.80.

#### 23.15.14 Drill 3 answers

**1.** CardioX 1,051,470 (1); OncoZ 842,880 (2); NeuroY 788,970 (3). Second: **OncoZ**.

**2.** North CardioX 337,134; North OncoZ 281,280; South CardioX 280,944.

**3.** 208, 241, 273, 306, 327, 344; MoM NULL, 15.87, 13.28, 12.09, 6.86, 5.20.

**4.** Running total 208, 449, 722, 1,028, 1,355, **1,699**.

**5.** **325.67** in June.

**6.** OncoZ 124 (36.05%); CardioX 115 (33.43%); NeuroY 105 (30.52%).

**7.** The 12-row table is in 23.6.4.

**8.** 16 January active → 14 June retained = **87.5%**. P-041 and P-042 churned after April.

#### 23.15.15 Drill 4 answers

**1.** P-007 (2; SR-007, SR-008); P-023 (2; SR-024, SR-025).

**2.** SR-020 / P-019 / Access / 7; SR-021 / P-020 / Efficacy / N/A.

**3.** Out-of-window: **SR-032 / P-030**, response date **2025-12-15**. Blank TRx: **SR-030 / P-028**, stage **Trial prescribed**.

**4.** Stage counts 8/8/8/7/8; trial variance **-1**; all other stages 0.

**5.** Top-2-box: Intent 35.90 (n=39); Access 36.84 (n=38); Efficacy 47.37 (n=38). NPS **-7.69** (12 promoters, 12 passives, 15 detractors).

**6.** Q2 region: East 264,690 / 258,216 / **102.51%**; North 479,334 / 487,076 / **98.41%**; South 400,884 / 412,085 / **97.28%**; West 337,962 / 351,953 / **96.02%**.

**7.** Price **+8,754**; volume **-1,680**; interaction **-84**; total **+6,990**.

**8.** A strong answer includes three numbered insights, each with a number and an implication. Example:

- Commercial: “Overall achievement is 98.76%, but CardioX is 94.06% and South is 96.85%; the gap is concentrated, not broad.”
- Survey: “Access is the weakest top-2-box item at 36.84% (n=38), while efficacy is 47.37%; the barrier is access, not belief.”
- Data quality: “The trial cell is one respondent short of quota and has one blank TRx; treat trial-stage percentages as directional and fix the blank in the next wave.”

---
### 23.16 The 45-minute SQL assessment (questions only)

**Scenario.** You are the analyst on the NovaCure account. The brand team meets tomorrow to review Q2. You have 45 minutes and the `trinity_sql_practice.db` database. Write the queries, run them, and prepare a short written readout. The data is fictional.

**Rules.**

- You may use the schema, the data dictionary, and any SQL you have practised. You may not use the answer key.
- For every answer, state the grain and any cleaning rule.
- If a question is ambiguous, state your assumption and proceed.
- A correct number with the wrong denominator is not a correct answer.
- Save your queries in a file or a scratch tab so you can explain them.

**Suggested time plan.**

| Minutes | Task |
|---:|---|
| 0–5 | Read the questions; inspect the schema and row counts |
| 5–15 | Tasks 1–2 (survey base, Q2 region) |
| 15–25 | Tasks 3–5 (product mix, top cells, survey scores) |
| 25–35 | Tasks 6–7 (calls, Rx trend) |
| 35–42 | Task 8 (top 3 per specialty) and Task 9 (written readout) |
| 42–45 | Sanity checks: row counts, totals, denominators |

#### Task 1 — Survey base (4 marks)

Return three numbers in one result:

- raw survey rows,
- unique physicians,
- clean fielded respondents after keeping one response per physician and restricting to `response_date` between `2026-01-15` and `2026-03-31`.

Explain in one sentence why the third number is not 42.

#### Task 2 — Q2 region performance (4 marks)

For Q2 2026 (April–June), return region name, revenue, target, achievement %, and a rank by achievement descending. Include all four regions.

#### Task 3 — Q2 product mix (3 marks)

For Q2 2026, return product name, revenue, and share of Q2 revenue, sorted by revenue descending. Include all three products.

#### Task 4 — Q2 top cells (3 marks)

Return the top three region × product cells for Q2 revenue, with a rank. Include the revenue and the cell’s share of Q2 revenue.

#### Task 5 — Survey top-2-box and NPS (4 marks)

Using the clean survey base, return:

- top-2-box % for Intent, Access, and Efficacy (unweighted),
- valid n for each item,
- NPS (promoters − detractors, as a percentage of respondents),
- promoter, passive, and detractor counts.

#### Task 6 — Calls (3 marks)

Return total calls, F2F calls, Remote calls, F2F %, and average duration by call type. Round duration to two decimals.

#### Task 7 — Rx trend (5 marks)

Return monthly TRx for January–June, previous-month TRx, month-over-month growth %, and a running total. Also state the June total and the January→June growth %.

#### Task 8 — Top 3 physicians per specialty (2 marks)

For June 2026, return the top three physicians by TRx within each specialty. Include specialty, rank, physician name, and TRx.

#### Task 9 — Written readout (2 marks)

Write three bullets:

1. One commercial insight with a number.
2. One survey insight with a number.
3. One data-quality or next-step insight with a number.

Each bullet must include an implication, not just a number. Suggested length: 3–5 sentences total.

**Total: 30 marks.**

---

### 23.17 Assessment answer key and marking guide

#### Task 1 — Survey base (4 marks)

```sql
WITH dedup AS (
    SELECT physician_id, MIN(response_id) AS keep_id
    FROM survey
    GROUP BY physician_id
),
clean_survey AS (
    SELECT s.*
    FROM survey AS s
    JOIN dedup AS d
        ON d.keep_id = s.response_id
    WHERE s.response_date BETWEEN '2026-01-15' AND '2026-03-31'
)
SELECT
    (SELECT COUNT(*) FROM survey) AS raw_rows,
    (SELECT COUNT(DISTINCT physician_id) FROM survey) AS unique_physicians,
    (SELECT COUNT(*) FROM clean_survey) AS clean_fielded;
```

| raw_rows | unique_physicians | clean_fielded |
|---:|---:|---:|
| 42 | 40 | 39 |

**Explanation:** 42 raw rows → 40 unique physicians (two exact duplicate respondents, P-007 and P-023) → 39 after removing the out-of-window response P-030 (2025-12-15).

**Marking:** 1 mark for raw 42; 1 for unique 40; 1 for clean 39; 1 for naming the duplicate and out-of-window rules.

#### Task 2 — Q2 region performance (4 marks)

```sql
SELECT
    r.region_name,
    SUM(s.revenue) AS revenue,
    SUM(s.target_revenue) AS target,
    ROUND(100.0 * SUM(s.revenue) / NULLIF(SUM(s.target_revenue), 0), 2) AS achievement_pct,
    RANK() OVER (
        ORDER BY 1.0 * SUM(s.revenue) / NULLIF(SUM(s.target_revenue), 0) DESC
    ) AS rnk
FROM sales AS s
JOIN regions AS r
    ON r.region_id = s.region_id
WHERE s.month BETWEEN '2026-04-01' AND '2026-06-01'
GROUP BY r.region_name
ORDER BY rnk;
```

| region_name | revenue | target | achievement_pct | rnk |
|---|---:|---:|---:|---:|
| East | 264,690 | 258,216 | 102.51 | 1 |
| North | 479,334 | 487,076 | 98.41 | 2 |
| South | 400,884 | 412,085 | 97.28 | 3 |
| West | 337,962 | 351,953 | 96.02 | 4 |

**Marking:** 1 mark for the Q2 filter; 1 for correct revenue/target; 1 for achievement; 1 for rank. A common error is using full-year revenue or ranking by revenue instead of achievement.

#### Task 3 — Q2 product mix (3 marks)

```sql
WITH q2 AS (
    SELECT product_id, SUM(revenue) AS revenue
    FROM sales
    WHERE month BETWEEN '2026-04-01' AND '2026-06-01'
    GROUP BY product_id
),
total AS (
    SELECT SUM(revenue) AS q2_total FROM q2
)
SELECT
    p.product_name,
    q.revenue,
    ROUND(100.0 * q.revenue / (SELECT q2_total FROM total), 2) AS share_pct
FROM q2 AS q
JOIN products AS p
    ON p.product_id = q.product_id
ORDER BY q.revenue DESC;
```

| product_name | revenue | share_pct |
|---|---:|---:|
| CardioX | 563,790 | 38.02 |
| OncoZ | 531,840 | 35.87 |
| NeuroY | 387,240 | 26.11 |

**Marking:** 1 for Q2 filter; 1 for correct revenue; 1 for share denominator = Q2 total (1,482,870). Using full-year total is wrong.

#### Task 4 — Q2 top cells (3 marks)

```sql
WITH cells AS (
    SELECT
        r.region_name,
        p.product_name,
        SUM(s.revenue) AS revenue
    FROM sales AS s
    JOIN regions AS r ON r.region_id = s.region_id
    JOIN products AS p ON p.product_id = s.product_id
    WHERE s.month BETWEEN '2026-04-01' AND '2026-06-01'
    GROUP BY r.region_name, p.product_name
),
ranked AS (
    SELECT
        region_name,
        product_name,
        revenue,
        ROW_NUMBER() OVER (ORDER BY revenue DESC) AS rn,
        ROUND(100.0 * revenue / SUM(revenue) OVER (), 2) AS share_pct
    FROM cells
)
SELECT region_name, product_name, revenue, share_pct
FROM ranked
WHERE rn <= 3
ORDER BY rn;
```

| region_name | product_name | revenue | share_pct |
|---|---|---:|---:|
| North | CardioX | 180,774 | 12.19 |
| North | OncoZ | 177,600 | 11.98 |
| South | CardioX | 150,624 | 10.16 |

**Marking:** 1 for Q2 filter and cell grain; 1 for correct top 3; 1 for rank/share. The top 3 are close; a small filter error changes the order.

#### Task 5 — Survey top-2-box and NPS (4 marks)

```sql
WITH ranked AS (
    SELECT
        s.*,
        ROW_NUMBER() OVER (
            PARTITION BY physician_id
            ORDER BY response_id
        ) AS rn
    FROM survey AS s
    WHERE response_date BETWEEN '2026-01-15' AND '2026-03-31'
),
clean_survey AS (
    SELECT * FROM ranked WHERE rn = 1
),
items AS (
    SELECT
        i.item,
        COUNT(*) AS valid_n,
        ROUND(
            100.0 * SUM(CASE WHEN i.score_raw IN ('4', '5') THEN 1 ELSE 0 END)
            / COUNT(*),
            2
        ) AS top2_pct
    FROM survey_items AS i
    JOIN clean_survey AS c
        ON c.response_id = i.response_id
    WHERE i.score_raw IN ('1', '2', '3', '4', '5')
    GROUP BY i.item
),
nps AS (
    SELECT
        SUM(CASE WHEN recommend_score >= 9 THEN 1 ELSE 0 END) AS promoters,
        SUM(CASE WHEN recommend_score BETWEEN 7 AND 8 THEN 1 ELSE 0 END) AS passives,
        SUM(CASE WHEN recommend_score <= 6 THEN 1 ELSE 0 END) AS detractors,
        ROUND(
            100.0 * (
                SUM(CASE WHEN recommend_score >= 9 THEN 1 ELSE 0 END)
                - SUM(CASE WHEN recommend_score <= 6 THEN 1 ELSE 0 END)
            ) / COUNT(*),
            2
        ) AS nps
    FROM clean_survey
)
SELECT * FROM items, nps;
```

| item | valid_n | top2_pct | promoters | passives | detractors | nps |
|---|---:|---:|---:|---:|---:|---:|
| Access | 38 | 36.84 | 12 | 12 | 15 | -7.69 |
| Efficacy | 38 | 47.37 | 12 | 12 | 15 | -7.69 |
| Intent | 39 | 35.90 | 12 | 12 | 15 | -7.69 |

The NPS columns repeat because they are cross-joined to each item row. In a cleaner output, return two result sets or put NPS in a separate query. For the assessment, a single row with the item table and a separate NPS query is perfectly acceptable.

**Marking:** 1 for clean base; 1 for valid n by item; 1 for top-2-box; 1 for NPS and band counts. A common error is including `N/A`/7 in the denominator.

#### Task 6 — Calls (3 marks)

```sql
SELECT
    call_type,
    COUNT(*) AS calls,
    ROUND(AVG(duration_min), 2) AS avg_duration_min
FROM calls
GROUP BY call_type
ORDER BY call_type;

SELECT
    COUNT(*) AS total_calls,
    SUM(call_type = 'F2F') AS f2f_calls,
    SUM(call_type = 'Remote') AS remote_calls,
    ROUND(100.0 * SUM(call_type = 'F2F') / COUNT(*), 1) AS f2f_pct
FROM calls;
```

| call_type | calls | avg_duration_min |
|---|---:|---:|
| F2F | 80 | 36.56 |
| Remote | 40 | 35.25 |

| total_calls | f2f_calls | remote_calls | f2f_pct |
|---:|---:|---:|---:|
| 120 | 80 | 40 | 66.7 |

**Marking:** 1 for total; 1 for F2F/Remote split; 1 for average duration by type.

#### Task 7 — Rx trend (5 marks)

```sql
WITH monthly AS (
    SELECT month, SUM(trx) AS trx
    FROM prescriptions
    GROUP BY month
)
SELECT
    month,
    trx,
    LAG(trx) OVER (ORDER BY month) AS previous_month,
    ROUND(
        100.0 * (trx - LAG(trx) OVER (ORDER BY month))
        / NULLIF(LAG(trx) OVER (ORDER BY month), 0),
        2
    ) AS mom_growth_pct,
    SUM(trx) OVER (
        ORDER BY month
        ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW
    ) AS running_total
FROM monthly
ORDER BY month;
```

| month | trx | previous_month | mom_growth_pct | running_total |
|---|---:|---:|---:|---:|
| 2026-01-01 | 208 | NULL | NULL | 208 |
| 2026-02-01 | 241 | 208 | 15.87 | 449 |
| 2026-03-01 | 273 | 241 | 13.28 | 722 |
| 2026-04-01 | 306 | 273 | 12.09 | 1,028 |
| 2026-05-01 | 327 | 306 | 6.86 | 1,355 |
| 2026-06-01 | 344 | 327 | 5.20 | 1,699 |

June total **344**; January→June growth **65.38%** (344/208 − 1).

**Marking:** 1 for monthly totals; 1 for `LAG`; 1 for MoM formula; 1 for running total; 1 for June total and 65.38% growth.

#### Task 8 — Top 3 physicians per specialty (2 marks)

```sql
WITH june AS (
    SELECT
        ph.physician_id,
        ph.physician_name,
        TRIM(ph.specialty) AS specialty,
        SUM(r.trx) AS trx
    FROM prescriptions AS r
    JOIN physicians AS ph
        ON ph.physician_id = r.physician_id
    WHERE r.month = '2026-06-01'
    GROUP BY ph.physician_id, ph.physician_name, TRIM(ph.specialty)
),
ranked AS (
    SELECT
        specialty,
        physician_name,
        trx,
        ROW_NUMBER() OVER (
            PARTITION BY specialty
            ORDER BY trx DESC, physician_id
        ) AS rn
    FROM june
)
SELECT specialty, rn, physician_name, trx
FROM ranked
WHERE rn <= 3
ORDER BY specialty, rn;
```

| specialty | rn | physician_name | trx |
|---|---:|---|---:|
| Cardiologist | 1 | Dr. Varun Kulkarni | 36 |
| Cardiologist | 2 | Dr. Shreya Mehta | 20 |
| Cardiologist | 3 | Dr. Rahul Pillai | 8 |
| Neurologist | 1 | Dr. Vikram Rao | 40 |
| Neurologist | 2 | Dr. Siddharth Roy | 24 |
| Neurologist | 3 | Dr. Ravi Singh | 11 |
| Oncologist | 1 | Dr. Yash Sinha | 44 |
| Oncologist | 2 | Dr. Sneha Vora | 28 |
| Oncologist | 3 | Dr. Riya Bose | 14 |
| Primary Care | 1 | Dr. Zoya Chandra | 48 |
| Primary Care | 2 | Dr. Tanvi Desai | 32 |
| Primary Care | 3 | Dr. Sara Gupta | 17 |

**Marking:** 1 for the correct partition and top-3 filter; 1 for the correct rows. A common error is using `RANK` without handling ties or forgetting `TRIM(specialty)`, which creates a fifth “Oncologist ” group.

#### Task 9 — Written readout (2 marks)

A strong answer:

- **Commercial:** “Overall achievement is 98.76%, but CardioX is 94.06% and South is 96.85%; the gap is concentrated in the largest product and the second-largest region, so the fix should be cell-specific rather than a broad price action.”
- **Survey:** “Access is the weakest top-2-box item at 36.84% (n=38), while efficacy is 47.37%; the barrier looks like access, not belief, so the next check is access by channel and region.”
- **Data quality / next step:** “The trial cell is one respondent short of quota and has one blank TRx; treat trial-stage percentages as directional and fix the blank in the next wave before comparing trial vs regular.”

**Marking:** 1 mark for two accurate, quantified insights; 1 mark for a third insight with an implication or next step. A list of numbers without implications gets at most 1 mark.

---

### 23.18 Common SQL mistakes and final checklist

#### 23.18.1 The ten mistakes that cost the most marks

| # | Mistake | Why it is wrong | Fix |
|---:|---|---|---|
| 1 | Missing `GROUP BY` column | The query is ambiguous or errors | Every non-aggregate `SELECT` column goes in `GROUP BY` |
| 2 | `WHERE` instead of `HAVING` | `WHERE` filters rows before aggregation | Use `HAVING` for aggregate conditions |
| 3 | Fan-out | A one-to-many join multiplies rows and denominators | Check counts before/after; aggregate first |
| 4 | `NULL` comparison | `= NULL` returns unknown | Use `IS NULL` / `IS NOT NULL` |
| 5 | `NOT IN` with NULL | The result can be empty | Use `NOT EXISTS` |
| 6 | `COUNT(*)` vs `COUNT(col)` | They answer different questions | State whether NULLs count |
| 7 | Integer division | A percentage truncates to 0 or 1 | Multiply by `1.0`; cast |
| 8 | `ROW_NUMBER()` in `WHERE` | Window functions run after `WHERE` | Wrap in a CTE |
| 9 | Wrong date boundary | `BETWEEN` is inclusive; timestamps need half-open ranges | `>= start AND < next_day` |
| 10 | `SELECT DISTINCT` to hide duplicates | It masks a join or grain problem | Fix the join or dedupe explicitly |

#### 23.18.2 The ten-second sanity check

Before you submit any query:

1. **Row count:** Does it match the expected grain? 4 regions, 3 products, 12 cells, 39 respondents?
2. **Totals:** Does the sum reconcile to the headline? 2,683,320 revenue; 1,699 TRx.
3. **Denominator:** Is the percentage base the one the question asked for? Rows, respondents, units, target, or valid item responses?
4. **Nulls:** Did `COUNT(column)` exclude NULLs? Did `WHERE col <> 0` drop blanks?
5. **Duplicates:** Did the join multiply respondents or items?
6. **Date window:** Is it inclusive/exclusive as intended? Does it include the out-of-window row?
7. **Aliases:** Are you sorting by the alias you think? Is the alias ambiguous?
8. **Tie-breaker:** Does the top-N query have a deterministic order?
9. **Formatting:** Are percentages rounded consistently? Are currency values in the right unit?
10. **Assumption:** Can you say the cleaning rule and the grain out loud in one sentence?

#### 23.18.3 Final checklist for the interview

- [ ] I restated the question and the grain.
- [ ] I checked the table row counts.
- [ ] I inspected the key columns and NULLs.
- [ ] I wrote the query in steps (CTE per grain).
- [ ] I checked the row count after every join.
- [ ] I used `NULLIF` for division.
- [ ] I used `HAVING` for aggregate filters.
- [ ] I handled ties and NULLs.
- [ ] I reconciled the total to a known number.
- [ ] I stated one assumption and one limitation.
- [ ] I gave a recommendation, not just a number.

#### 23.18.4 Where to go next

- Use [28-weakness-based-practice-and-error-log.md](28-weakness-based-practice-and-error-log.md) for SQL error categories I1–I4 and a spaced-review schedule.
- Use [22-advanced-excel-for-analyst-assessments.md](22-advanced-excel-for-analyst-assessments.md) for the Excel side of the same commercial and survey workflow.
- Use [20-power-bi-complete-practical-guide.md](20-power-bi-complete-practical-guide.md) for the dashboard layer that sits on top of SQL.
- Use [29-integrated-study-roadmap.md](29-integrated-study-roadmap.md) once available for the full revision sequence.

**Next:** [28-weakness-based-practice-and-error-log.md](28-weakness-based-practice-and-error-log.md) for SQL error drills, then the integrated roadmap in module 29.

---

*End of module 23. All data is fictional and was generated for practice.*
