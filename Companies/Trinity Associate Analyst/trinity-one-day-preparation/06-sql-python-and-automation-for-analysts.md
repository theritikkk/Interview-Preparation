# 06 — SQL, Python & Automation for Analysts

> **Learning objectives:** write the 8 SQL patterns analysts actually use on sales/survey tables; run the 8 pandas steps that turn a messy CSV into a clean summary; and articulate *when* to use Excel vs SQL vs Python vs a BI tool.
> **Estimated study time:** 45 min (your programming background makes this the fastest file — focus on the analyst flavor, not the syntax).
> **Priority:** **SHOULD KNOW** — the JD (CONFIRMED) lists *"process improvements … using tools like Excel, VBA, Python and Microsoft Power Tools."* You won't be asked to invert a binary tree; you *will* be asked to sum TRx by region.
> **Your edge:** hundreds of LeetCode problems means the logic is free. Spend the time on business framing, not code.

---

## §6.1 The fictional schema (same data as Mock B, in database form)

| Table | Columns |
|---|---|
| `reps` | rep_id, rep_name, region |
| `products` | product, therapeutic_area, launch_year |
| `sales` | sale_id, rep_id, product, month, trx, target_trx, calls |
| `respondents` | respondent_id, specialty, region |
| `survey_responses` | response_id, respondent_id, question, response (1–5) |

## §6.2 SQL — the 8 patterns (MUST be able to write these cold)

**1) Filter + sort.**
```sql
SELECT product, trx FROM sales
WHERE month = 'Jan-26' AND trx > 1000
ORDER BY trx DESC;
```

**2) GROUP BY — the single most used pattern.**
```sql
SELECT region, SUM(trx) AS total_trx
FROM sales s
JOIN reps r ON s.rep_id = r.rep_id
GROUP BY region
ORDER BY total_trx DESC;
-- Expected: North 7615 · South 5413 · West 5175 · East 4910
```

**3) HAVING — filter *after* aggregation (WHERE runs before and cannot see SUM).**
```sql
SELECT product, SUM(trx) AS total
FROM sales
GROUP BY product
HAVING SUM(trx) > 5000;
-- Expected rows: CardioX 13990 · NeuroY 6455 (OncoZ 2668 filtered out)
```

**4) Aggregates:** `COUNT(*)` rows vs `COUNT(DISTINCT rep_id)` unique reps vs `SUM/AVG/MIN/MAX`. *Mistake:* `AVG(trx)` vs "average per rep" (`SUM/COUNT(DISTINCT)`) — different questions.

**5) JOINs.** `INNER JOIN` keeps matches only; `LEFT JOIN` keeps all left rows and fills unmatched right columns with **NULL** — use LEFT JOIN to *find gaps*:
```sql
SELECT r.rep_id, r.rep_name
FROM reps r
LEFT JOIN sales s ON s.rep_id = r.rep_id
WHERE s.rep_id IS NULL;   -- reps with zero sales rows
```

**6) NULL handling.** `IS NULL` / `IS NOT NULL` (never `= NULL`). `COALESCE(x, 0)` substitutes. *Analyst caution:* filling a missing TRx with 0 is a claim that the rep sold nothing — sometimes wrong; better to exclude and state it (same rule as Excel blanks in [`03` §3.6](03-excel-complete-revision.md)).

**7) CASE WHEN — banding and top-2-box.**
```sql
SELECT question,
       AVG(response) AS mean_score,
       1.0 * SUM(CASE WHEN response >= 4 THEN 1 ELSE 0 END) / COUNT(*) AS top2box
FROM survey_responses
GROUP BY question;
```

**8) Subquery — compare to a benchmark (reps above the average).**
```sql
SELECT rep_id, total
FROM (SELECT rep_id, SUM(trx) AS total FROM sales GROUP BY rep_id) t
WHERE t.total > (SELECT AVG(total) FROM
       (SELECT rep_id, SUM(trx) AS total FROM sales GROUP BY rep_id) x);
```

**Data-quality checks (attention to detail — they love these):**
```sql
-- duplicate detection
SELECT rep_id, month, COUNT(*) n FROM sales GROUP BY rep_id, month HAVING COUNT(*) > 1;
-- blank/missing values
SELECT COUNT(*) FROM sales WHERE trx IS NULL;
-- category standardization (find non-conforming values)
SELECT DISTINCT region FROM reps
WHERE UPPER(TRIM(region)) NOT IN ('NORTH','SOUTH','EAST','WEST');
```

## §6.3 Python / pandas — the 8 analyst steps

Assume a laptop at 14:15 on the plan. Verify your own output; run snippet 9.

**1–3) Read, inspect, clean.**
```python
import pandas as pd
df = pd.read_csv("trinity_excel_mock.csv")
df.head()          # eyeball the first rows
df.info()          # dtypes — text-numbers stand out (object vs int64)
df.describe()      # count/mean/min/max — catches blanks and outliers

df["TRx"] = pd.to_numeric(df["TRx"].astype(str).str.replace(",", ""), errors="coerce")
df["Region"] = df["Region"].str.strip().str.title().replace({"N": "North"})
df["Product"] = df["Product"].str.strip()
df = df.drop_duplicates()
```
`errors="coerce"` turns unparseable values into NaN (missing) instead of crashing — then you *decide* to fix or exclude. `drop_duplicates()` removes the exact duplicate row (30 → 29 rows).

**4–5) Filter and group.**
```python
big = df[df["TRx"] > 1000]
by_region = df.groupby("Region")["TRx"].sum().sort_values(ascending=False)
by_specialty = df.groupby("Specialty").agg(total=("TRx", "sum"), avg_calls=("Calls", "mean"))
```
Output of `by_region`: North 7615 · South 5413 · West 5175 · East 4910 — same numbers as Excel/SQL, which is exactly the cross-check you'd mention in an interview.

**6) Percentages and growth.**
```python
months = df.groupby("Month")["TRx"].sum().reindex(["Jan-26", "Feb-26"])
mom = months.pct_change() * 100   # Feb-26 = -2.25%
```
*Pitfall:* `groupby` order follows the data; `reindex` forces chronological order so `pct_change` compares correctly.

**7) Merge (Excel lookups, pandas style).**
```python
master = pd.DataFrame({"Product": ["CardioX", "NeuroY", "OncoZ"],
                       "TA": ["Cardiovascular", "Neurology", "Oncology"]})
df = df.merge(master, on="Product", how="left")
df["TA"].isna().sum()             # lookup-failure count — must be 0 after cleaning
```

**8) Export + automate.**
```python
df.to_csv("clean_sales.csv", index=False)

for f in ["jan_sales.csv", "feb_sales.csv"]:
    d = pd.read_csv(f)
    d.groupby("Product")["TRx"].sum().to_csv(f.replace("sales", "summary"))
```
This loop replaces an hour of copy-paste each month — that's "process improvement" in JD language.

## §6.4 Automation decision guide (short scenarios get asked in interviews)

| Situation | Tool | Why |
|---|---|---|
| One-off analysis, < ~100k rows, needs stakeholder-friendly output | **Excel** | Fastest to model and share; formulas + charts |
| Data lives in a database; repeatable pulls; joins across tables | **SQL** | Aggregation before download; single source of truth |
| Recurring multi-file cleaning/merging; complex transforms; scheduled reports | **Python (pandas)** | Scripted, repeatable, auditable; no manual steps |
| Recurring interactive monitoring by non-analysts | **Power BI / Tableau** | Scheduled refresh + self-serve slicing |
| Every week you manually clean 20 regional CSVs then paste into a deck | **Python + template** | Automate the pipeline, keep judgment for the analysis |

**How they complement each other:** SQL extracts and pre-aggregates → Python cleans/merges/model-checks → BI serves it live → Excel/PowerPoint for ad-hoc deep dives and the narrative. Say this chain in an interview and you sound like someone who has shipped a pipeline.

## §6.5 Quick practice

**S1 (SQL).** Write a query: average TRx per product, highest first.
**S2 (SQL).** Find the bug: `SELECT region, SUM(trx) FROM sales GROUP BY region WHERE SUM(trx) > 100000;`
**S3 (SQL).** Count unique reps per region from `sales` + `reps`.
**S4 (pandas).** Predict the output:
```python
d = pd.DataFrame({"g": ["A","A","B"], "v": [10, 20, 5]})
print(d.groupby("g")["v"].sum())
```
**S5 (tool choice).** For each, pick Excel/SQL/Python/BI: (a) weekly dashboard for 50 managers; (b) one-time pricing sensitivity model; (c) monthly merge of 40 branch files into one report; (d) pulling 10M rows of prescription data from a warehouse.

### §6.6 Answers

| Q | Answer |
|---|---|
| S1 | `SELECT product, AVG(trx) FROM sales GROUP BY product ORDER BY AVG(trx) DESC;` |
| S2 | `WHERE` runs before aggregation and cannot see `SUM(trx)`. Use `HAVING SUM(trx) > 100000`. |
| S3 | `SELECT r.region, COUNT(DISTINCT s.rep_id) FROM sales s JOIN reps r ON s.rep_id=r.rep_id GROUP BY r.region;` |
| S4 | A → 30, B → 5 (group then sum) |
| S5 | (a) **BI tool**; (b) **Excel**; (c) **Python**; (d) **SQL** (aggregate before pulling) |

**Next:** [07 — Life sciences, business analytics & cases](07-life-sciences-business-analytics-and-cases.md)
