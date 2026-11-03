# 24 — Python for Data Analysis and Analyst Automation

**Purpose:** move you from “I have written a little Python” to “I can load a messy extract, clean it, join it, aggregate it, check it, and export a decision-ready table under time pressure.” This is the Python/pandas layer that sits beside Excel ([22](22-advanced-excel-for-analyst-assessments.md)), SQL ([23](23-sql-for-business-and-survey-analytics.md)), and Power BI ([20](20-power-bi-complete-practical-guide.md)). The goal is not software engineering. The goal is repeatable analyst work.

**Prerequisites:** basic programming (variables, loops, functions in any language), [04-excel-formulas-cheatsheet.md](04-excel-formulas-cheatsheet.md), [05-excel-practical-workbook-guide.md](05-excel-practical-workbook-guide.md), and [06-statistics-and-survey-analytics.md](06-statistics-and-survey-analytics.md). [23-sql-for-business-and-survey-analytics.md](23-sql-for-business-and-survey-analytics.md) is helpful because the data model and the business questions are the same; it is not required.

**Suggested study time:** 12–15 hours across four sessions, all with Python open. Type every snippet. Run it. Break it. Read the traceback. Do not read this like an article.

**Data files (all fictional):**

| File | Rows | Grain | What it is for |
|---|---:|---|---|
| `data/sql/csv/sales.csv` | 72 | month × region × product | KPI, groupby, pivot, price–volume |
| `data/sql/csv/regions.csv` | 4 | region | Lookup |
| `data/sql/csv/products.csv` | 3 | product | Lookup |
| `data/sql/csv/reps.csv` | 12 | rep | Call analysis |
| `data/sql/csv/physicians.csv` | 42 | physician | Survey and Rx dimensions |
| `data/sql/csv/survey.csv` | 42 | response | Cleaning, quotas, NPS, weighting |
| `data/sql/csv/survey_items.csv` | 126 | response × item | Top-2-box, item means |
| `data/sql/csv/calls.csv` | 120 | call | Activity analysis |
| `data/sql/csv/prescriptions.csv` | 276 | physician × product × month | Trend, retention, mix |
| `data/sql/csv/quotas.csv` | 5 | awareness stage | Quota reconciliation |
| `data/python/requirements.txt` | — | — | Environment |
| `data/python/README.md` | — | — | Setup and data notes |

> **Setup before you start.** From the pack root, create a virtual environment and install the requirements:
>
> ```bash
> python3 -m venv .venv
> source .venv/bin/activate
> python -m pip install --upgrade pip
> python -m pip install -r data/python/requirements.txt
> ```
>
> Then check:
>
> ```bash
> python -c "import pandas as pd, numpy as np; print(pd.__version__, np.__version__)"
> ```
>
> The examples were run with Python 3.13, pandas 2.3.3, and NumPy 2.x. If your versions are close, the code should run. The full setup is in `data/python/README.md`.

> **Source labels.** Python is **CONFIRMED** as a tool in the current JD: the role explicitly mentions operationalising process improvements with Excel, VBA, Python, and Microsoft Power Tools. A dedicated Python coding round is **not confirmed**; treat the interview format as **PRACTICE INFERENCE**. The 45-minute assessment is a practice simulation, not an official test. If your invitation confirms a Python round, upgrade that part to **CONFIRMED**.

> **All data is fictional.** The company, products, regions, physicians, sales, prescriptions, and survey responses were generated for practice. They are not Trinity data, not real company data, and not indicative of real market size. Revenue and TRx figures are deliberately small enough for mental arithmetic.

---

## 24.0 Why Python, and where it fits

### 24.0.1 The analyst stack

A practical analyst stack is not “one tool to rule them all”. It is a set of layers:

| Layer | Typical tool | Job |
|---|---|---|
| Storage | Database / warehouse | Hold raw rows, keys, dates, transactions |
| Extraction and transformation | SQL / Python | Filter, join, clean, aggregate, QA |
| Analysis | Excel / Python / Power BI | Explore, calculate, test, reconcile |
| Presentation | Excel / Power BI / PowerPoint | Format, chart, annotate, tell the story |
| Automation | Python / Power Query / scheduler | Repeat the same steps reliably |

Python is strongest when the work is **repeatable**, **multi-file**, **too large for a comfortable spreadsheet**, or **needs assertions**. Excel is stronger for a quick pivot and a client-facing workbook. SQL is stronger for pulling the right rows from a large database. Power BI is stronger for a live, shareable dashboard. The interview answer is not “Python is best”; it is “I choose the layer that makes the result reliable and easy to explain.”

### 24.0.2 What Python is good at — and what it is not

**Good at:**

- Reading several CSVs and combining them.
- Cleaning text, dates, categories, and missing values.
- Repeating the same monthly report without manual steps.
- Writing checks that fail loudly when the data is wrong.
- Handling data that is too large or too awkward for Excel.
- Prototyping a calculation before it becomes a dashboard measure.

**Not automatically good at:**

- Business context. A pandas pipeline can produce a wrong denominator very efficiently.
- Presentation. A DataFrame is not a client-ready slide.
- Governance. A script on your laptop is not a production data pipeline.
- Speed on very large data. pandas loads data into memory; a warehouse may be better.
- Complex statistics. pandas gives you the mechanics; you still need the statistics from [06](06-statistics-and-survey-analytics.md).

### 24.0.3 The data model

The practice data is the same commercial and survey warehouse used in module 23:

```text
regions 1 ───< reps 1 ───< calls >─── 1 physicians
   │                                      │
   └──< sales >── 1 products              ├──< survey 1 ───< survey_items
                        │                 │
                        └──< prescriptions >── 1 products
```

| Table | Grain | Rows | Key |
|---|---|---:|---|
| `regions` | one row per region | 4 | `region_id` |
| `products` | one row per product | 3 | `product_id` |
| `reps` | one row per rep | 12 | `rep_id` |
| `sales` | month × region × product | 72 | `sale_id` |
| `physicians` | one row per physician | 42 | `physician_id` |
| `survey` | one row per response | 42 raw | `response_id` |
| `survey_items` | response × item | 126 | (`response_id`, `item`) |
| `calls` | one row per call | 120 | `call_id` |
| `prescriptions` | physician × product × month with TRx > 0 | 276 | `rx_id` |
| `quotas` | one row per awareness stage | 5 | `awareness_stage` |

The important columns:

| Table | Columns used most |
|---|---|
| `sales` | `month`, `region_id`, `product_id`, `units`, `unit_price`, `revenue`, `target_revenue`, `calls` |
| `survey` | `response_id`, `physician_id`, `response_date`, `region_raw`, `awareness_stage`, `recommend_score`, `monthly_trx`, `weight` |
| `survey_items` | `response_id`, `item`, `score_raw` |
| `prescriptions` | `physician_id`, `product_id`, `month`, `trx`, `new_rx` |
| `calls` | `rep_id`, `physician_id`, `product_id`, `call_date`, `call_type`, `duration_min` |
| `physicians` | `physician_id`, `physician_name`, `specialty`, `region_id`, `channel`, `practice_type` |

### 24.0.4 Python vs SQL vs Excel

The same question can be answered in four tools. The skill is choosing and translating.

| Task | SQL (module 23) | pandas (this module) | Excel (module 22) |
|---|---|---|---|
| Filter rows | `WHERE` | boolean mask / `.query()` | Filter / Advanced Filter |
| Choose columns | `SELECT` | `df[["a", "b"]]` | Select columns |
| Aggregate | `GROUP BY` | `.groupby().agg()` | PivotTable |
| Join | `JOIN` | `.merge()` | XLOOKUP / Power Query |
| Rank within group | `ROW_NUMBER()` | `.groupby().rank()` / `.head()` | Sort + helper column |
| Running total | `SUM() OVER` | `.cumsum()` | Running SUM |
| Month-over-month | `LAG()` | `.shift(1)` / `.pct_change()` | Prior-row formula |
| Pivot | `CASE` + `SUM` | `.pivot_table()` | PivotTable |
| QA | `COUNT`, `LEFT JOIN` | `.isna()`, `.duplicated()`, `assert` | COUNTIFS / filters |

**Rule:** use SQL to get the right rows; use pandas to automate the transformation and checks; use Excel/Power BI to present. Do not use pandas to rebuild a dashboard that Power BI already does well.

### 24.0.5 Reproducibility from day one

A notebook that works once is not a deliverable. A reproducible analysis has:

1. **A data snapshot.** The CSVs in `data/sql/csv/` are the snapshot.
2. **A fixed environment.** `data/python/requirements.txt`.
3. **A clear entry point.** A script or a notebook that runs top-to-bottom.
4. **Checks.** Assertions on row counts, totals, duplicates, and ranges.
5. **An output.** A named CSV or Excel file, not just an on-screen DataFrame.
6. **A log.** What ran, when, on which snapshot, and what changed.

The module returns to these six items in 24.19.

### 24.0.6 How to use this module

Four sessions:

| Session | Focus | Output |
|---|---|---|
| 1 | Python foundations + pandas inspection/filtering | A clean sales DataFrame and a five-row answer |
| 2 | groupby, joins, pivots, dates | A region-product matrix and a monthly trend |
| 3 | survey and commercial analytics | A survey readout and a commercial KPI table |
| 4 | automation, drills, assessment | A reusable script and a 45-minute timed attempt |

Do not skip the drills. Reading pandas syntax feels productive; typing it under a timer is the actual skill.

---

## Part A — Python foundations

### 24.1 Values, variables, strings, and f-strings

Python is dynamically typed: a variable can point to an integer, then a string. That is convenient for exploration and dangerous for a pipeline. Use clear names and check types at boundaries.

```python
region_name = "North"
months = 6
revenue = 2_683_320
achievement = 0.9876
is_above_target = False

print(f"{region_name}: {months} months, revenue ₹{revenue:,.0f}, achievement {achievement:.2%}")
```

Output:

```text
North: 6 months, revenue ₹2,683,320, achievement 98.76%
```

**Why f-strings matter.** They put the formatting next to the number, so the reader sees the unit and precision. `f"{achievement:.2%}"` is clearer than `round(achievement * 100, 2)` followed by a separate `"%"`.

Common types:

| Type | Example | Analyst use |
|---|---|---|
| `int` | `72` | Row counts, units |
| `float` | `98.76` | Revenue, percentages |
| `str` | `"North"` | Categories, dates in ISO text |
| `bool` | `True` | Flags |
| `None` | `None` | Missing Python value |
| `float("nan")` | `NaN` | Missing numeric value in pandas |
| `list` | `["North", "South"]` | Ordered collection |
| `dict` | `{"N": "North"}` | Key-value lookup |
| `set` | `{"N", "S"}` | Unique values, membership |

**Numbers are not money.** Python floats are binary approximations; `0.1 + 0.2` is not exactly `0.3`. For display and interview arithmetic, round at the end. For accounting-grade currency, use `Decimal` or integer minor units; this module uses floats because the data is small and fictional.

### 24.2 Lists, tuples, dictionaries, and sets

```python
products = ["CardioX", "NeuroY", "OncoZ"]
regions = ("North", "South", "East", "West")
manager = {"North": "Meera Kapoor", "South": "Arjun Iyer"}
unique_region_ids = {"N", "S", "E", "W"}

print(products[0])          # CardioX
print(regions[-1])          # West
print(manager["North"])     # Meera Kapoor
print("N" in unique_region_ids)  # True
```

| Structure | Ordered? | Mutable? | Duplicates? | Use |
|---|---|---|---|---|
| `list` | Yes | Yes | Yes | A sequence you will append to |
| `tuple` | Yes | No | Yes | A fixed record |
| `dict` | Yes (insertion order) | Yes | Keys unique | Lookup table |
| `set` | No | Yes | No | Unique values, membership |

**Analyst pattern:** a dictionary is a lookup table. Instead of writing five `if` statements to map `N` to `North`, use a dict:

```python
region_map = {"N": "North", "S": "South", "E": "East", "W": "West"}
print(region_map["N"])
```

In pandas, this becomes `.map(region_map)` or a merge to a dimension table.

### 24.3 Control flow and functions

```python
def achievement_pct(revenue, target):
    """Return achievement as a percentage, or None if target is missing or zero."""
    if target is None or target == 0:
        return None
    return 100 * revenue / target

for region, revenue in [("East", 481_200), ("North", 864_954)]:
    print(region, "above target" if revenue > 500_000 else "below 500k")
```

**A function is a contract.** It should have:

- a clear name (`achievement_pct`, not `calc`),
- typed-looking inputs (revenue, target),
- an explicit missing-value behaviour,
- one job,
- a docstring if the rule is not obvious.

**Do not use a function to hide a business rule.** If the target is blank, should achievement be `None`, `0`, or excluded? The function must say. In this module, a blank target returns `None` and is excluded from achievement, matching module 23.

### 24.4 Comprehensions and iteration

```python
products = ["CardioX", "NeuroY", "OncoZ"]
lengths = {p: len(p) for p in products}
long_names = [p for p in products if len(p) > 5]
```

Comprehensions are concise, but they are not always more readable. Use them for a simple transform or filter. Use a loop when there are multiple steps, logging, or error handling.

**Do not iterate over DataFrame rows** unless you have no vectorised alternative. `for index, row in df.iterrows()` is slow and easy to get wrong. Use column operations, `.groupby()`, `.merge()`, or `.apply()` on a Series.

### 24.5 Errors, debugging, and virtual environments

Read a traceback from the **bottom up**:

```text
KeyError: 'region_name'
```

means a column named `region_name` is not in the DataFrame. Check `df.columns` before guessing.

| Error | Common cause | Fix |
|---|---|---|
| `KeyError` | Column name typo, or the column was renamed | `print(df.columns)` |
| `TypeError` | Adding a string to a number, or comparing incomparable types | Check `dtypes`; cast explicitly |
| `ValueError` | Wrong shape, bad parse, or ambiguous merge | Check `shape` and `validate=` |
| `SettingWithCopyWarning` | Assigning to a slice | Use `.copy()` or `.loc` on the original |
| `ModuleNotFoundError` | Package not installed in the active environment | Activate `.venv`; `pip install` |
| `FileNotFoundError` | Wrong working directory | Run from the pack root; use `Path` |

**Virtual environment:** a per-project sandbox for packages. It prevents “it worked yesterday” when another project upgrades pandas.

```bash
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -r data/python/requirements.txt
```

**Debugging checklist:**

1. What is the shape? `df.shape`
2. What are the columns and types? `df.info()`
3. What are the first rows? `df.head()`
4. What is missing? `df.isna().sum()`
5. What is duplicated? `df.duplicated().sum()`
6. Does the key match? `df['key'].nunique()` vs `len(df)`
7. Is the filter returning zero rows? Print the filter counts.

---

### Check yourself — 24.1–24.5

1. What is the difference between a list and a tuple?
2. Why is a dictionary useful for a region lookup?
3. What does `f"{0.9876:.2%}"` return?
4. Why is `iterrows()` usually a bad idea?
5. What is the first thing you check after a `KeyError`?

Answers are in 24.24.

---
## Part B — pandas foundations

### 24.6 Series and DataFrame

A **Series** is one labelled column. A **DataFrame** is a table of Series that share an index.

```python
import pandas as pd

revenue = pd.Series([481_200, 864_954, 724_284, 612_882],
                    index=["East", "North", "South", "West"],
                    name="revenue")
print(revenue)
```

| Index | revenue |
|---|---:|
| East | 481,200 |
| North | 864,954 |
| South | 724,284 |
| West | 612,882 |

```python
print(revenue.mean())        # 670830.0
print(revenue.idxmax())      # North
print(revenue.sort_values(ascending=False).head(2))
```

A DataFrame adds columns:

```python
regions = pd.DataFrame({
    "region_id": ["N", "S", "E", "W"],
    "region_name": ["North", "South", "East", "West"],
    "regional_manager": ["Meera Kapoor", "Arjun Iyer", "Farah Khan", "Nikhil Desai"],
})
```

**The index is not a column.** It is a label attached to each row. After `read_csv`, the index is a default `RangeIndex` (0, 1, 2, …). After `groupby`, the group key may become the index unless you use `as_index=False`. After `merge`, pandas creates a new `RangeIndex`.

**Analyst consequence:** always know whether your group key is a column or the index. If it is the index, `reset_index()` turns it back into a column.

```python
summary = (sales.groupby("region_id")
                .agg(revenue=("revenue", "sum"))
                .reset_index())
```

### 24.7 Reading and inspecting data

```python
from pathlib import Path
import pandas as pd

DATA = Path("data/sql/csv")
sales = pd.read_csv(DATA / "sales.csv")
```

**Standard load block.** The rest of the module assumes these names. Run this once at the top of a notebook or script:

```python
from pathlib import Path
import pandas as pd
import numpy as np

DATA = Path("data/sql/csv")

sales_raw = pd.read_csv(DATA / "sales.csv")
sales = sales_raw.copy()
regions = pd.read_csv(DATA / "regions.csv")
products = pd.read_csv(DATA / "products.csv")
reps = pd.read_csv(DATA / "reps.csv")
physicians = pd.read_csv(DATA / "physicians.csv")
survey = pd.read_csv(DATA / "survey.csv", parse_dates=["response_date"])
items = pd.read_csv(
    DATA / "survey_items.csv",
    dtype={"score_raw": "string"},
    keep_default_na=False,
)
calls = pd.read_csv(DATA / "calls.csv", parse_dates=["call_date"])
prescriptions = pd.read_csv(DATA / "prescriptions.csv")
quotas = pd.read_csv(DATA / "quotas.csv")
```

If a snippet below re-reads a table with different options, it is showing a specific point about parsing; the standard block is the default. `sales_raw` is the untouched source copy; `sales` is the working copy that later sections enrich with calculated columns. Structural checks use `sales_raw` so their shape does not change when you add a column.

The first five things to run:

```python
sales.shape          # (72, 9)
sales.head()         # first five rows
sales.info()         # columns, non-null counts, dtypes
sales.describe()     # numeric summary
sales.isna().sum()   # missing values per column
```

**Expected shape:** `(72, 9)`.

**Expected dtypes:**

| Column | dtype |
|---|---|
| `sale_id` | `object` |
| `month` | `object` |
| `region_id` | `object` |
| `product_id` | `object` |
| `units` | `int64` |
| `unit_price` | `int64` |
| `revenue` | `int64` |
| `target_revenue` | `float64` |
| `calls` | `float64` |

`target_revenue` and `calls` are `float64` because they contain blanks. Pandas represents a blank numeric field as `NaN`. That is correct for arithmetic, but it means `count()` and `sum()` will ignore those rows.

**Missing values:**

```python
sales.isna().sum()
```

| Column | Missing |
|---|---:|
| `sale_id` | 0 |
| `month` | 0 |
| `region_id` | 0 |
| `product_id` | 0 |
| `units` | 0 |
| `unit_price` | 0 |
| `revenue` | 0 |
| `target_revenue` | 2 |
| `calls` | 3 |

**Read dates explicitly.** If a column is a date, parse it on read or immediately after:

```python
survey = pd.read_csv(DATA / "survey.csv", parse_dates=["response_date"])
calls = pd.read_csv(DATA / "calls.csv", parse_dates=["call_date"])
```

If you leave dates as strings, sorting and comparison still work for ISO `YYYY-MM-DD`, but date arithmetic does not. Parse once at the boundary.

**Read raw survey codes explicitly.** `survey_items.csv` contains `N/A` and numeric codes. By default, pandas converts `N/A` to `NaN` and the column to `float64`:

```python
items_default = pd.read_csv(DATA / "survey_items.csv")
print(items_default["score_raw"].dtype)  # float64
```

To preserve the raw code for QA:

```python
items = pd.read_csv(
    DATA / "survey_items.csv",
    dtype={"score_raw": "string"},
    keep_default_na=False,
)
```

Now `score_raw` is a string column containing `"7"` and `"N/A"`. You decide the valid codes explicitly:

```python
items["score_num"] = pd.to_numeric(items["score_raw"], errors="coerce")
valid = items[items["score_num"].isin([1, 2, 3, 4, 5])]
```

**Lesson:** `read_csv` makes assumptions. Inspect `dtypes` and the unique values of every categorical or code column before you aggregate.

### 24.8 Selecting, filtering, and sorting

#### 24.8.1 Select columns

```python
sales[["sale_id", "month", "revenue"]].head()
```

Use a list for multiple columns. A single string returns a Series:

```python
sales["revenue"].head()       # Series
sales[["revenue"]].head()     # DataFrame
```

#### 24.8.2 `.loc` and `.iloc`

| Accessor | Selects by | Example |
|---|---|---|
| `.loc` | label / boolean mask | `sales.loc[sales.revenue > 60000, ["sale_id", "revenue"]]` |
| `.iloc` | integer position | `sales.iloc[:5, :3]` |

```python
sales.loc[sales["revenue"] > 60_000, ["sale_id", "revenue"]]
sales.iloc[:5, :4]
```

Use `.loc` for business logic. Use `.iloc` for “first five rows and first four columns” in exploration. Do not mix them.

#### 24.8.3 Boolean masks

```python
q2 = sales[(sales["month"] >= "2026-04-01") & (sales["month"] < "2026-07-01")]
```

Parentheses matter. `&` binds tighter than comparison in Python? Actually comparison binds tighter than `&`, but pandas requires parentheses around each condition to avoid ambiguity. Write them.

Use `.query()` for a readable subset:

```python
q2 = sales.query("month >= '2026-04-01' and month < '2026-07-01'")
```

`.query()` is convenient but can be slower and less portable than a mask. For an interview, either is fine; explain the filter.

#### 24.8.4 `isin`, `between`, and negations

```python
sales[sales["region_id"].isin(["N", "S"])]
sales[sales["month"].between("2026-04-01", "2026-06-01")]
sales[~sales["product_id"].isin(["CARD"])]
```

`~` is boolean NOT. `between` is inclusive on both ends, like SQL `BETWEEN`.

#### 24.8.5 Sorting and top-N

```python
sales.sort_values("revenue", ascending=False).head(5)
sales.nlargest(5, "revenue")[["sale_id", "month", "region_id", "product_id", "revenue"]]
```

| sale_id | month | region_id | product_id | revenue |
|---|---|---|---|---:|
| S-063 | 2026-06-01 | N | ONCO | 70,080 |
| S-061 | 2026-06-01 | N | CARD | 66,276 |
| S-049 | 2026-05-01 | N | CARD | 58,338 |
| S-066 | 2026-06-01 | S | ONCO | 58,080 |
| S-051 | 2026-05-01 | N | ONCO | 57,600 |

`nlargest` is not a business ranking. It ranks **rows**. The largest row is not the largest product or region. State the grain.

#### 24.8.6 A warning about chained assignment

```python
# Avoid
q2 = sales[sales.month >= "2026-04-01"]
q2["achievement"] = q2.revenue / q2.target_revenue
```

This can raise `SettingWithCopyWarning` and may not modify what you think. Use `.copy()`:

```python
q2 = sales.loc[sales.month >= "2026-04-01"].copy()
q2["achievement"] = q2["revenue"] / q2["target_revenue"]
```

Or use `.assign()`:

```python
q2 = (sales.loc[sales.month >= "2026-04-01"]
           .assign(achievement=lambda d: d.revenue / d.target_revenue))
```

### 24.9 Creating and transforming columns

#### 24.9.1 Arithmetic

```python
sales = sales.assign(
    computed_revenue=lambda d: d.units * d.unit_price,
    achievement_pct=lambda d: 100 * d.revenue / d.target_revenue,
)
```

A blank target produces `NaN`, not an error. That is the correct default; do not fill it with zero for achievement.

#### 24.9.2 `np.where` and `np.select`

```python
import numpy as np

sales["revenue_band"] = np.where(
    sales["revenue"] >= 60_000, "High",
    np.where(sales["revenue"] >= 30_000, "Medium", "Low")
)
```

| revenue_band | rows |
|---|---:|
| High | 2 |
| Low | 20 |
| Medium | 50 |

For three or more conditions, `np.select` is cleaner:

```python
conditions = [
    sales["revenue"] >= 60_000,
    sales["revenue"] >= 30_000,
]
choices = ["High", "Medium"]
sales["revenue_band"] = np.select(conditions, choices, default="Low")
```

`np.select` evaluates in order. Put the most specific condition first.

#### 24.9.3 Mapping a lookup

```python
product_map = products.set_index("product_id")["product_name"]
sales["product_name"] = sales["product_id"].map(product_map)
```

`map` is fast and readable when the lookup is small. For multiple columns, use a merge.

#### 24.9.4 Renaming and reordering

```python
summary = summary.rename(columns={"revenue": "revenue_inr"})
summary = summary[["region_name", "revenue_inr", "target", "achievement_pct"]]
```

Rename at the end, not in the middle of a calculation. Short names are fine for intermediate steps; final outputs should be self-explanatory.

### 24.10 Missing data and strings

#### 24.10.1 Missing data

```python
sales["target_revenue"].isna().sum()   # 2
sales["calls"].isna().sum()            # 3
sales["target_revenue"].notna().sum()  # 70
```

| Operation | Effect |
|---|---|
| `df.isna()` | Boolean mask of missing values |
| `df.isna().sum()` | Missing count per column |
| `df.dropna(subset=["target_revenue"])` | Drop rows missing that column |
| `df.fillna({"calls": 0})` | Replace missing calls with 0 |
| `df["calls"].fillna(df["calls"].median())` | Impute with median |
| `df["target_revenue"].fillna(0)` | **Danger:** turns “no target” into “target = 0” |

**Do not fill a blank target with zero for achievement.** It creates a false overachievement. Either exclude the row or show “n/a”.

```python
valid_target = sales.dropna(subset=["target_revenue"])
achievement = 100 * valid_target["revenue"].sum() / valid_target["target_revenue"].sum()
```

#### 24.10.2 Strings

```python
physicians = pd.read_csv(DATA / "physicians.csv")
physicians["specialty_clean"] = physicians["specialty"].str.strip()
```

Raw specialty counts:

| specialty | count |
|---|---:|
| Cardiologist | 11 |
| Neurologist | 11 |
| Primary Care | 10 |
| Oncologist | 9 |
| `Oncologist ` (trailing space) | 1 |

After `.str.strip()`:

| specialty_clean | count |
|---|---:|
| Cardiologist | 11 |
| Neurologist | 11 |
| Oncologist | 10 |
| Primary Care | 10 |

**Raw region variants:**

```python
survey["region_raw"].value_counts()
```

| region_raw | count |
|---|---:|
| East | 12 |
| North | 10 |
| South | 9 |
| West | 9 |
| `north ` | 1 |
| `N` | 1 |

Map them explicitly:

```python
region_fix = {"north ": "North", "N": "North"}
survey["region_clean"] = survey["region_raw"].replace(region_fix)
```

Or use `.str.strip().replace({"N": "North"})`. Do not silently drop the variants; count them first.

**String methods you will use:**

| Method | Example | Result |
|---|---|---|
| `.str.strip()` | `" Oncologist ".str.strip()` | `"Oncologist"` |
| `.str.lower()` | `"North".str.lower()` | `"north"` |
| `.str.upper()` | `"north".str.upper()` | `"NORTH"` |
| `.str.replace()` | `"north ".str.replace(" ", "")` | `"north"` |
| `.str.contains()` | `df.name.str.contains("Dr.")` | Boolean mask |
| `.str.startswith()` | `df.id.str.startswith("P-")` | Boolean mask |
| `.str.split()` | `"a b".str.split()` | List |

### 24.11 Aggregation with groupby

#### 24.11.1 The headline KPI

```python
kpi = pd.Series({
    "revenue": sales["revenue"].sum(),
    "target": sales["target_revenue"].sum(),
    "rows": len(sales),
    "rows_with_target": sales["target_revenue"].count(),
})
kpi["achievement_pct"] = 100 * kpi["revenue"] / kpi["target"]
print(kpi)
```

| Metric | Value |
|---|---:|
| revenue | 2,683,320 |
| target | 2,717,035 |
| rows | 72 |
| rows_with_target | 70 |
| achievement_pct | 98.76 |

`size` counts rows, including rows with missing values. `count` counts non-missing values. That is why `rows` is 72 and `rows_with_target` is 70.

#### 24.11.2 Group by one column

```python
region_summary = (
    sales.merge(regions, on="region_id", how="left", validate="many_to_one")
         .groupby("region_name", as_index=False)
         .agg(revenue=("revenue", "sum"),
              target=("target_revenue", "sum"))
)
region_summary["achievement_pct"] = 100 * region_summary["revenue"] / region_summary["target"]
region_summary = region_summary.sort_values("achievement_pct", ascending=False)
```

| region_name | revenue | target | achievement_pct |
|---|---:|---:|---:|
| East | 481,200 | 460,455 | 104.51 |
| West | 612,882 | 622,128 | 98.51 |
| North | 864,954 | 886,608 | 97.56 |
| South | 724,284 | 747,844 | 96.85 |

**Why `as_index=False`?** It keeps `region_name` as a column, so the result is easier to export and merge. If you leave it as the index, call `.reset_index()` before writing to CSV.

#### 24.11.3 Multiple aggregations

```python
sales.groupby("region_id", as_index=False).agg(
    rows=("sale_id", "size"),
    revenue=("revenue", "sum"),
    avg_row_revenue=("revenue", "mean"),
    max_row_revenue=("revenue", "max"),
    products=("product_id", "nunique"),
)
```

| region_id | rows | revenue | avg_row_revenue | max_row_revenue | products |
|---|---:|---:|---:|---:|---:|
| E | 18 | 481,200 | 26,733.33 | 37,800 | 3 |
| N | 18 | 864,954 | 48,053.00 | 70,080 | 3 |
| S | 18 | 724,284 | 40,238.00 | 58,080 | 3 |
| W | 18 | 612,882 | 34,049.00 | 47,376 | 3 |

The row counts are 18 per region because the sales file is a balanced month × region × product grid: 6 months × 3 products = 18 rows per region. The target blanks are in East and West (`S-009`, `S-012`); they do not remove rows from a `size` count. Always check whether a region is missing a month/product combination.

#### 24.11.4 `transform`: group-level value on every row

```python
sales["region_avg"] = sales.groupby("region_id")["revenue"].transform("mean")
sales["above_region_avg"] = sales["revenue"] > sales["region_avg"]
print(sales["above_region_avg"].sum())  # 34
```

`transform` returns a Series aligned to the original rows. `agg` returns one row per group. That is the difference.

#### 24.11.5 `groupby` + `rank` + `head`

Top three physicians per specialty in June:

```python
june = (prescriptions[prescriptions["month"] == "2026-06-01"]
        .merge(physicians, on="physician_id", how="left", validate="many_to_one")
        .assign(specialty=lambda d: d["specialty"].str.strip())
        .groupby(["specialty", "physician_id", "physician_name"], as_index=False)
        .agg(trx=("trx", "sum")))

top3 = (june.sort_values(["specialty", "trx", "physician_id"],
                         ascending=[True, False, True])
             .groupby("specialty", as_index=False)
             .head(3))
```

| specialty | physician_name | trx |
|---|---|---:|
| Cardiologist | Dr. Varun Kulkarni | 36 |
| Cardiologist | Dr. Shreya Mehta | 20 |
| Cardiologist | Dr. Rahul Pillai | 8 |
| Neurologist | Dr. Vikram Rao | 40 |
| Neurologist | Dr. Siddharth Roy | 24 |
| Neurologist | Dr. Ravi Singh | 11 |
| Oncologist | Dr. Yash Sinha | 44 |
| Oncologist | Dr. Sneha Vora | 28 |
| Oncologist | Dr. Riya Bose | 14 |
| Primary Care | Dr. Zoya Chandra | 48 |
| Primary Care | Dr. Tanvi Desai | 32 |
| Primary Care | Dr. Sara Gupta | 17 |

`head(3)` after a sort is easier to read than a window function. It is also deterministic if the sort includes a tie-breaker (`physician_id`).

### 24.12 Merging and concatenating

#### 24.12.1 The safe merge

```python
sales_r = sales_raw.merge(regions, on="region_id", how="left", validate="many_to_one")
sales_rp = sales_r.merge(products, on="product_id", how="left", validate="many_to_one")
print(sales_r.shape)   # (72, 12)
print(sales_rp.shape)  # (72, 15)
```

`validate="many_to_one"` raises an error if `regions` has duplicate `region_id` values. That is a **good** failure: it catches a broken dimension before it doubles your revenue.

| `validate=` | Meaning |
|---|---|
| `"one_to_one"` | Keys unique on both sides |
| `"one_to_many"` | Keys unique on the left, not on the right |
| `"many_to_one"` | Keys unique on the right, not on the left |
| `"many_to_many"` | No uniqueness check; use only when intended |

#### 24.12.2 Anti-join

```python
physician_calls = physicians.merge(
    calls[["physician_id"]].drop_duplicates(),
    on="physician_id",
    how="left",
    indicator=True,
)
no_calls = physician_calls[physician_calls["_merge"] == "left_only"]
print(no_calls[["physician_id", "physician_name"]])
```

| physician_id | physician_name |
|---|---|
| P-041 | Dr. Dev Malhotra |
| P-042 | Dr. Ritu Bansal |

`indicator=True` adds `_merge` with `both`, `left_only`, or `right_only`. It is the cleanest way to audit a join.

#### 24.12.3 Fan-out detection

```python
clean_survey = (
    survey.loc[survey["response_date"].between("2026-01-15", "2026-03-31")]
          .sort_values(["physician_id", "response_id"])
          .drop_duplicates("physician_id", keep="first")
)

print(clean_survey.shape)  # (39, 8)
print(clean_survey.merge(items, on="response_id", how="inner", validate="one_to_many").shape)  # (117, 10)
```

The clean base has 39 respondents. Joining to three items per respondent gives 117 rows. If you compute `clean_survey.merge(items).weight.mean()`, you weight each respondent three times. Aggregate items first or use a separate respondent-level table.

#### 24.12.4 Concatenate

```python
q1 = sales[sales["month"].between("2026-01-01", "2026-03-01")]
q2 = sales[sales["month"].between("2026-04-01", "2026-06-01")]
both = pd.concat([q1, q2], ignore_index=True)
print(both.shape)  # (72, 9)
```

`concat` stacks rows. `merge` combines columns by key. Do not confuse them.

Use `ignore_index=True` when the old index has no meaning. Otherwise, duplicate index labels can cause confusing `.loc` results.

### 24.13 Pivot tables, crosstab, and melt

#### 24.13.1 `pivot_table`

```python
matrix = sales.pivot_table(
    index="product_id",
    columns="month",
    values="revenue",
    aggfunc="sum",
    fill_value=0,
)
```

| product_id | 2026-01 | 2026-02 | 2026-03 | 2026-04 | 2026-05 | 2026-06 |
|---|---:|---:|---:|---:|---:|---:|
| CARD | 157,200 | 155,400 | 175,080 | 175,080 | 182,070 | 206,640 |
| NEUR | 134,400 | 138,390 | 128,940 | 130,410 | 132,930 | 123,900 |
| ONCO | 86,400 | 103,680 | 120,960 | 149,280 | 172,800 | 209,760 |

`pivot_table` is the pandas PivotTable. `aggfunc="sum"` is explicit; without it, pandas defaults to `mean`, which is rarely what you want for revenue.

#### 24.13.2 `crosstab`

```python
clean_survey = clean_survey.merge(
    physicians[["physician_id", "specialty"]],
    on="physician_id",
    how="left",
    validate="many_to_one",
)
clean_survey["specialty"] = clean_survey["specialty"].str.strip()

pd.crosstab(clean_survey["specialty"], clean_survey["awareness_stage"], margins=True)
```

| specialty | Aided | Consideration | Regular | Trial | Unaided | All |
|---|---:|---:|---:|---:|---:|---:|
| Cardiologist | 2 | 2 | 2 | 2 | 2 | 10 |
| Neurologist | 2 | 2 | 2 | 1 | 2 | 9 |
| Oncologist | 2 | 2 | 2 | 2 | 2 | 10 |
| Primary Care | 2 | 2 | 2 | 2 | 2 | 10 |
| All | 8 | 8 | 8 | 7 | 8 | 39 |

`crosstab` is for counts/frequencies. `pivot_table` is for a numeric measure with an aggregation.

#### 24.13.3 `melt`

```python
long = matrix.reset_index().melt(
    id_vars="product_id",
    var_name="month",
    value_name="revenue",
)
print(long.shape)  # (18, 3)
```

Wide data is easy to read; long data is easy to filter, group, and chart. Most pandas operations expect long data. Use `pivot_table` to go long → wide and `melt` to go wide → long.

### 24.14 Dates and time series

#### 24.14.1 Parse and extract

```python
sales["month_date"] = pd.to_datetime(sales["month"])
sales["year"] = sales["month_date"].dt.year
sales["month_num"] = sales["month_date"].dt.month
sales["month_label"] = sales["month_date"].dt.strftime("%Y-%m")
```

`dt` is the date accessor. It only works on datetime dtype. If the column is a string, parse it first.

#### 24.14.2 Date arithmetic

```python
start = pd.Timestamp("2026-01-01")
end = pd.Timestamp("2026-06-01")
print((end - start).days)  # 151
```

#### 24.14.3 `resample`

```python
monthly = (sales.set_index("month_date")
                .resample("MS")
                .agg(revenue=("revenue", "sum"),
                     target=("target_revenue", "sum")))
```

`MS` means month start. `resample` is for a datetime index; `groupby` is for a column. If the data already has one row per month, `groupby` is simpler.

#### 24.14.4 `shift`, `pct_change`, `rolling`, `cumsum`

```python
rx_monthly = prescriptions.groupby("month", as_index=False).agg(trx=("trx", "sum"))
rx_monthly["previous_trx"] = rx_monthly["trx"].shift(1)
rx_monthly["mom_pct"] = rx_monthly["trx"].pct_change() * 100
rx_monthly["running_total"] = rx_monthly["trx"].cumsum()
rx_monthly["ma_3m"] = rx_monthly["trx"].rolling(3).mean()
```

| month | trx | previous_trx | mom_pct | running_total | ma_3m |
|---|---:|---:|---:|---:|---:|
| 2026-01-01 | 208 | NaN | NaN | 208 | NaN |
| 2026-02-01 | 241 | 208 | 15.87 | 449 | NaN |
| 2026-03-01 | 273 | 241 | 13.28 | 722 | 240.67 |
| 2026-04-01 | 306 | 273 | 12.09 | 1,028 | 273.33 |
| 2026-05-01 | 327 | 306 | 6.86 | 1,355 | 302.00 |
| 2026-06-01 | 344 | 327 | 5.20 | 1,699 | 325.67 |

**The first values are NaN for a reason.** There is no previous month and no full three-month window. Do not replace them with zero unless the business rule says so.

### 24.15 `apply`, `map`, `where`, and performance

#### 24.15.1 `map` on a Series

```python
product_map = products.set_index("product_id")["product_name"]
sales["product_name"] = sales["product_id"].map(product_map)
```

#### 24.15.2 `apply` on a Series

```python
def call_band(minutes):
    if minutes < 30:
        return "<30"
    if minutes < 45:
        return "30-44"
    return "45+"

calls["duration_band"] = calls["duration_min"].apply(call_band)
```

`apply` on a Series is acceptable for a small custom function. `apply` on a DataFrame row by row is slow; prefer vectorised operations.

#### 24.15.3 `apply` on a groupby

```python
def top2_share(group):
    return 100 * group["score_num"].isin([4, 5]).mean()

top2 = (items[items["score_num"].isin([1, 2, 3, 4, 5])]
        .groupby("item")
        .apply(top2_share, include_groups=False))
```

Use `include_groups=False` in pandas 2.2+ to avoid the deprecated behaviour of passing the grouping columns. If your pandas version is older, omit it.

#### 24.15.4 `where` and `mask`

```python
sales["revenue_high"] = sales["revenue"].where(sales["revenue"] >= 60_000, 0)
```

`where(cond, other)` keeps values where the condition is true and replaces the rest with `other`. `mask` is the inverse.

#### 24.15.5 Performance rules

1. **Vectorise.** `df["a"] + df["b"]` is faster than `df.apply(lambda row: row.a + row.b, axis=1)`.
2. **Use `category`** for low-cardinality text columns repeated many times.
3. **Avoid `iterrows()`.** It is slow and converts types.
4. **Filter early.** Read only needed columns with `usecols=` when the file is large.
5. **Use `merge(validate=...)`.** It catches cardinality errors before they become silent.
6. **Do not `copy()` by reflex.** Copy when you need independence; otherwise, a view is fine until you assign.
7. **Profile before optimizing.** `df.info(memory_usage="deep")` and `%timeit` in Jupyter.

---

### Check yourself — 24.6–24.15

1. What is the difference between a Series and a DataFrame?
2. What does `sales.shape` return for the practice file?
3. Why are `target_revenue` and `calls` `float64` rather than `int64`?
4. What is the difference between `.loc` and `.iloc`?
5. What does `groupby().agg()` return, and what does `groupby().transform()` return?
6. Why does joining `survey` to `survey_items` multiply the row count?
7. What is the difference between `pivot_table` and `crosstab`?
8. What does `shift(1)` do, and why is the first value NaN?
9. Why is `apply(axis=1)` slower than a vectorised operation?
10. What does `validate="many_to_one"` protect against?

Answers are in 24.24.

---
## Part C — Applied analytics

### 24.16 Data quality audit in pandas

Data quality is a set of checks, not a feeling. The goal is **known data**, not perfect data.

#### 24.16.1 A reusable audit function

```python
def audit(df, name):
    print(f"--- {name} ---")
    print("shape:", df.shape)
    print("duplicate rows:", df.duplicated().sum())
    print("missing values:")
    print(df.isna().sum()[df.isna().sum() > 0])
    print()

audit(sales_raw, "sales")
audit(survey, "survey")
audit(items, "survey_items")
```

Expected:

| Table | Shape | Exact duplicate rows |
|---|---|---|
| `sales` | (72, 9) | 0 |
| `survey` | (42, 8) | 0 |
| `survey_items` | (126, 3) | 0 |

The survey has **duplicate respondents**, not duplicate rows. `response_id` differs, so `df.duplicated()` is zero. Use a subset:

```python
survey.duplicated(subset=["physician_id"]).sum()  # 2
```

#### 24.16.2 Duplicate respondents

```python
dups = (survey[survey.duplicated("physician_id", keep=False)]
        .sort_values(["physician_id", "response_id"]))
```

| response_id | physician_id | response_date | region_raw | awareness_stage | recommend_score | monthly_trx | weight |
|---|---|---|---|---|---|---:|---:|
| SR-007 | P-007 | 2026-02-07 | East | Unaided awareness | 4 | 0 | 1.4 |
| SR-008 | P-007 | 2026-02-07 | East | Unaided awareness | 4 | 0 | 1.4 |
| SR-024 | P-023 | 2026-02-23 | East | Consideration | 7 | 0 | 0.9 |
| SR-025 | P-023 | 2026-02-23 | East | Consideration | 7 | 0 | 0.9 |

The two rows per physician are exact copies on every respondent field. The only difference is `response_id`. The duplicate count is **2 extra rows** (42 raw → 40 unique physicians).

#### 24.16.3 Invalid codes

```python
items["score_num"] = pd.to_numeric(items["score_raw"], errors="coerce")
invalid = items[~items["score_num"].isin([1, 2, 3, 4, 5])]
```

| response_id | item | score_raw | score_num |
|---|---|---|---:|
| SR-020 | Access | 7 | 7 |
| SR-021 | Efficacy | N/A | NaN |

A 7 on a 1–5 scale is out of range. `N/A` is missing, not a score. Both must be excluded from the valid base and reported.

#### 24.16.4 Orphan keys

```python
def orphan_count(left, right, key):
    merged = left.merge(right[[key]], on=key, how="left", indicator=True)
    return (merged["_merge"] == "left_only").sum()

print(orphan_count(sales, regions, "region_id"))        # 0
print(orphan_count(sales, products, "product_id"))      # 0
print(orphan_count(survey, physicians, "physician_id")) # 0
print(orphan_count(calls, reps, "rep_id"))              # 0
print(orphan_count(prescriptions, physicians, "physician_id"))  # 0
```

All zero. If an orphan count is non-zero, the dimension is incomplete or the fact has a bad key. Do not drop the rows without quantifying them.

#### 24.16.5 Range and logic checks

```python
checks = {
    "bad_nps": (~survey["recommend_score"].between(0, 10)).sum(),
    "bad_duration": (~calls["duration_min"].between(15, 60)).sum(),
    "bad_rx": ((prescriptions["new_rx"] > prescriptions["trx"]) |
               (prescriptions["trx"] < 0)).sum(),
    "bad_trx": (survey["monthly_trx"] < 0).sum(),
    "bad_revenue": ((sales["revenue"] - sales["units"] * sales["unit_price"]).abs() > 1).sum(),
}
print(checks)
```

| Check | Count |
|---|---:|
| bad_nps | 0 |
| bad_duration | 0 |
| bad_rx | 0 |
| bad_trx | 0 |
| bad_revenue | 0 |

#### 24.16.6 Reconciliation

```python
revenue = sales["revenue"].sum()
computed = (sales["units"] * sales["unit_price"]).sum()
print(revenue, computed, revenue - computed)
```

| revenue | computed | difference |
|---:|---:|---:|
| 2,683,320 | 2,683,320 | 0 |

A zero difference proves the revenue column is internally consistent with units and price. It does not prove the revenue is correct, complete, or comparable to another source.

#### 24.16.7 Quota reconciliation

```python
stage_counts = clean_survey.groupby("awareness_stage", as_index=False).agg(
    actual_n=("response_id", "size")
)
quota_check = quotas.merge(stage_counts, on="awareness_stage", how="left")
quota_check["actual_n"] = quota_check["actual_n"].fillna(0).astype(int)
quota_check["variance"] = quota_check["actual_n"] - quota_check["target_n"]
```

| awareness_stage | target_n | actual_n | variance |
|---|---:|---:|---:|
| Aided awareness | 8 | 8 | 0 |
| Consideration | 8 | 8 | 0 |
| Regular prescriber | 8 | 8 | 0 |
| Trial prescribed | 8 | 7 | -1 |
| Unaided awareness | 8 | 8 | 0 |

The trial cell is one short. With n=7, one respondent is 14.3% of the cell. Flag the base; do not hide the result.

#### 24.16.8 Assertions

Assertions turn a silent wrong number into a loud failure. Put them at the end of the pipeline.

```python
assert sales.shape == (72, 9), "sales row count changed"
assert survey["physician_id"].nunique() == 40, "unique physician count changed"
assert clean_survey.shape[0] == 39, "clean survey base changed"
assert abs(sales["revenue"].sum() - 2_683_320) < 1, "revenue total changed"
assert (sales["revenue"] - sales["units"] * sales["unit_price"]).abs().max() == 0
assert clean_survey["weight"].sum().round(2) == 42.60
```

**Assertion rules:**

- Assert on facts that must not change without a business decision.
- Use a message that tells the next person what broke.
- Do not assert on a value you have not verified.
- In production, raise a clear exception or write to a validation log; do not rely on `assert` alone if the script runs with `-O` (assertions are removed).

#### 24.16.9 The QA checklist

| Check | pandas pattern | Expected here |
|---|---|---|
| Row counts | `df.shape` | 72 / 42 / 126 / 120 / 276 |
| Duplicates | `df.duplicated(subset=...)` | 2 survey respondents |
| Invalid codes | `~series.isin(valid)` | SR-020, SR-021 |
| Orphans | `merge(..., indicator=True)` | 0 |
| Ranges | `.between(min, max)` | 0 bad |
| Reconciliation | `sum(a) - sum(b)` | 0 |
| Fan-out | shape before/after merge | 42 → 126 |
| Freshness | `series.min()`, `series.max()` | 2026-01 to 2026-06 |
| Blanks | `series.isna().sum()` | 2 targets, 3 calls |

### 24.17 Survey analytics in pandas

#### 24.17.1 The cleaning pipeline

| Step | Rule | Rows |
|---:|---|---:|
| 0 | Raw survey responses | 42 |
| 1 | Unique physicians | 40 |
| 2 | Field window 2026-01-15 to 2026-03-31 | 41 |
| 3 | Dedupe by physician after window | 39 |
| 4 | Valid Likert codes only | 39 / 38 / 38 by item |

```python
survey = pd.read_csv(DATA / "survey.csv", parse_dates=["response_date"])

window = survey[survey["response_date"].between("2026-01-15", "2026-03-31")]
clean_survey = (window.sort_values(["physician_id", "response_id"])
                      .drop_duplicates("physician_id", keep="first"))
```

Expected: `window.shape == (41, 8)`, `clean_survey.shape == (39, 8)`.

**Why sort by `response_id` before dedupe?** `drop_duplicates(keep="first")` keeps the first row in the current order. Sorting makes “first” deterministic. If the business rule is “latest response”, sort by `response_date` descending, then `response_id` descending.

#### 24.17.2 Stage counts

```python
stage_counts = (clean_survey["awareness_stage"]
                .value_counts()
                .rename_axis("awareness_stage")
                .reset_index(name="n"))
```

| awareness_stage | n |
|---|---:|
| Unaided awareness | 8 |
| Aided awareness | 8 |
| Consideration | 8 |
| Regular prescriber | 8 |
| Trial prescribed | 7 |

The trial cell is one short of the quota of 8. The other four are exactly 8.

#### 24.17.3 Top-2-box

```python
items = pd.read_csv(
    DATA / "survey_items.csv",
    dtype={"score_raw": "string"},
    keep_default_na=False,
)
items["score_num"] = pd.to_numeric(items["score_raw"], errors="coerce")

valid_items = items[items["score_num"].isin([1, 2, 3, 4, 5])]

# Keep only fielded respondents: the clean 39, not all 42 raw responses.
valid_items = valid_items.merge(
    clean_survey[["response_id"]],
    on="response_id",
    how="inner",
    validate="many_to_one",
)

top2 = (valid_items.groupby("item", as_index=False)
        .agg(valid_n=("score_num", "size"),
             top2_pct=("score_num", lambda s: 100 * s.isin([4, 5]).mean()),
             mean_score=("score_num", "mean")))
```

| item | valid_n | top2_pct | mean_score |
|---|---:|---:|---:|
| Access | 38 | 36.84 | 3.03 |
| Efficacy | 38 | 47.37 | 3.45 |
| Intent | 39 | 35.90 | 3.10 |

**Reading:** Efficacy is strongest (47.37% top-2-box, mean 3.45). Access is weakest (36.84%, mean 3.03). Intent is also weak (35.90%). The pattern suggests belief in the product but a possible access barrier. That is a hypothesis, not a causal claim.

#### 24.17.4 NPS

```python
promoters = (clean_survey["recommend_score"] >= 9).sum()
passives = clean_survey["recommend_score"].between(7, 8).sum()
detractors = (clean_survey["recommend_score"] <= 6).sum()
nps = 100 * (promoters - detractors) / len(clean_survey)
print(promoters, passives, detractors, nps)
```

| promoters | passives | detractors | nps |
|---:|---:|---:|---:|
| 12 | 12 | 15 | -7.69 |

NPS = % promoters − % detractors. Passives are in the denominator but not the numerator. With n=39, one respondent is 2.56 points.

#### 24.17.5 Weighted estimates

```python
weighted_items = valid_items.merge(
    clean_survey[["response_id", "weight"]],
    on="response_id",
    how="inner",
    validate="many_to_one",
)

top2_weighted = (
    weighted_items.groupby("item")
    .apply(
        lambda g: 100
        * g.loc[g["score_num"].isin([4, 5]), "weight"].sum()
        / g["weight"].sum(),
        include_groups=False,
    )
    .rename("weighted_top2")
    .reset_index()
)
```

| item | weighted_top2 | unweighted_top2 | difference |
|---|---:|---:|---:|
| Access | 37.68 | 36.84 | +0.84 |
| Efficacy | 47.94 | 47.37 | +0.57 |
| Intent | 34.98 | 35.90 | -0.92 |

The differences are under 1 percentage point. The unweighted story survives weighting. That is a robustness check, not a guarantee.

```python
weighted_nps = 100 * (
    clean_survey.loc[clean_survey["recommend_score"] >= 9, "weight"].sum()
    - clean_survey.loc[clean_survey["recommend_score"] <= 6, "weight"].sum()
) / clean_survey["weight"].sum()
print(weighted_nps)  # -6.81
```

| NPS | Value |
|---|---:|
| Unweighted | -7.69 |
| Weighted | -6.81 |
| Difference | +0.88 |

**Effective sample size:**

```python
weight_sum = clean_survey["weight"].sum()
effective_n = weight_sum ** 2 / (clean_survey["weight"] ** 2).sum()
print(weight_sum, effective_n)
```

| weight_sum | effective_n |
|---:|---:|
| 42.60 | 37.78 |

Effective n is slightly below 39 because the weights are uneven. Report it when you want to be honest about precision.

#### 24.17.6 Crosstab by specialty

```python
clean_survey = clean_survey.merge(
    physicians[["physician_id", "specialty"]],
    on="physician_id", how="left", validate="many_to_one",
)
clean_survey["specialty"] = clean_survey["specialty"].str.strip()

pd.crosstab(clean_survey["specialty"], clean_survey["awareness_stage"], margins=True)
```

| specialty | Aided | Consideration | Regular | Trial | Unaided | All |
|---|---:|---:|---:|---:|---:|---:|
| Cardiologist | 2 | 2 | 2 | 2 | 2 | 10 |
| Neurologist | 2 | 2 | 2 | 1 | 2 | 9 |
| Oncologist | 2 | 2 | 2 | 2 | 2 | 10 |
| Primary Care | 2 | 2 | 2 | 2 | 2 | 10 |
| All | 8 | 8 | 8 | 7 | 8 | 39 |

The sample is almost perfectly balanced: two per stage per specialty, except neurology has one trial rather than two. That is a sample-design check, not a market finding.

#### 24.17.7 TRx by stage and specialty

```python
trx_stage = (clean_survey.groupby("awareness_stage", as_index=False)
             .agg(n=("response_id", "size"), trx=("monthly_trx", "sum")))
trx_stage["share_pct"] = 100 * trx_stage["trx"] / trx_stage["trx"].sum()
trx_stage = trx_stage.sort_values(["trx", "awareness_stage"], ascending=[False, True])
```

| awareness_stage | n | trx | share_pct |
|---|---:|---:|---:|
| Regular prescriber | 8 | 272 | 79.07 |
| Trial prescribed | 7 | 72 | 20.93 |
| Aided awareness | 8 | 0 | 0.00 |
| Consideration | 8 | 0 | 0.00 |
| Unaided awareness | 8 | 0 | 0.00 |

The trial row has 7 respondents but only 72 TRx because one trial physician has a blank TRx. The **valid TRx base** for trial is 6 respondents, not 7.

```python
trx_specialty = (clean_survey.groupby("specialty", as_index=False)
                 .agg(n=("response_id", "size"), trx=("monthly_trx", "sum"))
                 .sort_values("trx", ascending=False))
```

| specialty | n | trx |
|---|---:|---:|
| Oncologist | 10 | 100 |
| Primary Care | 10 | 97 |
| Neurologist | 9 | 75 |
| Cardiologist | 10 | 72 |

#### 24.17.8 Straight-liners

```python
straight = (valid_items.groupby("response_id")
            .filter(lambda g: len(g) == 3 and (g["score_num"] == 3).all()))
```

| response_id | item | score_raw | score_num |
|---|---|---|---:|
| SR-022 | Intent | 3 | 3 |
| SR-022 | Access | 3 | 3 |
| SR-022 | Efficacy | 3 | 3 |

A straight-liner is a **flag**, not an automatic delete. Run the headline with and without it; report the sensitivity if the conclusion changes. Do not silently drop it to make the numbers cleaner.

#### 24.17.9 Response-level vs respondent-level

| Unit | Table | Count | Use |
|---|---|---:|---|
| Response | `survey` | 42 raw / 39 clean | Response rate, quotas, NPS |
| Respondent | `survey` deduped | 40 unique / 39 clean | Physician-level analysis |
| Item response | `survey_items` | 126 raw / 115 valid | Top-2-box, item means |
| Weighted respondent | `survey` + weight | effective n 37.78 | Weighted estimates |

Valid item responses: Intent 39, Access 38, Efficacy 38. The difference is the two invalid records (Access 7, Efficacy N/A), not missing respondents.

#### 24.17.10 The five-sentence survey readout

1. **Base:** “39 fielded respondents after removing two exact duplicates and one out-of-window response; effective weighted n is 37.8.”
2. **Headline:** “Top-2-box is 35.9% for Intent, 36.8% for Access, and 47.4% for Efficacy.”
3. **NPS:** “NPS is -7.69 unweighted and -6.81 weighted; 12 promoters, 12 passives, 15 detractors.”
4. **Segment:** “Oncology and primary care account for 100 and 97 TRx respectively; neurology and cardiology are lower.”
5. **Action:** “Access is the weakest item; test whether the issue is concentrated in a channel or region before changing the message.”

That is a defensible readout with base, number, uncertainty, and next question.

---

### Check yourself — 24.16–24.17

1. Why is `survey.duplicated().sum()` zero when there are duplicate respondents?
2. What are the two invalid Likert records?
3. What is the clean survey base, and why is it 39 rather than 42?
4. What is the top-2-box for Access, and what is the valid n?
5. What is NPS, and how many promoters/passives/detractors?
6. What is the weighted NPS, and how much does weighting move it?
7. What is the straight-liner response ID, and should it be deleted?
8. Why is `COUNT(*)`-style `len()` after joining `survey` to `survey_items` not the number of respondents?
9. What does effective n tell you that raw n does not?

Answers are in 24.24.

---
### 24.18 Commercial analytics in pandas

#### 24.18.1 Headline KPI

```python
kpi = {
    "revenue": sales["revenue"].sum(),
    "target": sales["target_revenue"].sum(),
    "units": sales["units"].sum(),
    "rows": len(sales),
    "rows_with_target": sales["target_revenue"].notna().sum(),
    "rows_at_or_above_target": (sales["revenue"] >= sales["target_revenue"]).sum(),
}
kpi["achievement_pct"] = 100 * kpi["revenue"] / kpi["target"]
kpi["revenue_per_unit"] = kpi["revenue"] / kpi["units"]
```

| Metric | Value |
|---|---:|
| Revenue | ₹2,683,320 |
| Target | ₹2,717,035 |
| Achievement | 98.76% |
| Units | 14,121 |
| Revenue per unit | ₹190.02 |
| Rows | 72 |
| Rows with a target | 70 |
| Rows at or above target | 28 |

**Read it carefully:** 28 of 70 rows with a target are at or above target (40.0%). That is not the same as 28 of 72 rows (38.9%), because two rows have no target. Always state the denominator.

#### 24.18.2 Region and product performance

```python
sales_rp = (
    sales.merge(regions, on="region_id", how="left", validate="many_to_one")
         .merge(products, on="product_id", how="left", validate="many_to_one")
)

region_summary = (
    sales_rp.groupby("region_name", as_index=False)
            .agg(revenue=("revenue", "sum"),
                 target=("target_revenue", "sum"),
                 units=("units", "sum"))
)
region_summary["achievement_pct"] = 100 * region_summary["revenue"] / region_summary["target"]
region_summary["share_pct"] = 100 * region_summary["revenue"] / region_summary["revenue"].sum()
region_summary = region_summary.sort_values("share_pct", ascending=False)
```

| region_name | revenue | target | units | achievement_pct | share_pct |
|---|---:|---:|---:|---:|---:|
| North | 864,954 | 886,608 | 4,520 | 97.56 | 32.23 |
| South | 724,284 | 747,844 | 3,785 | 96.85 | 26.99 |
| West | 612,882 | 622,128 | 3,242 | 98.51 | 22.84 |
| East | 481,200 | 460,455 | 2,574 | 104.51 | 17.93 |

```python
product_summary = (
    sales_rp.groupby("product_name", as_index=False)
            .agg(revenue=("revenue", "sum"),
                 target=("target_revenue", "sum"),
                 units=("units", "sum"))
)
product_summary["achievement_pct"] = 100 * product_summary["revenue"] / product_summary["target"]
product_summary["share_pct"] = 100 * product_summary["revenue"] / product_summary["revenue"].sum()
product_summary = product_summary.sort_values("revenue", ascending=False)
```

| product_name | revenue | target | units | achievement_pct | share_pct |
|---|---:|---:|---:|---:|---:|
| CardioX | 1,051,470 | 1,117,831 | 8,608 | 94.06 | 39.19 |
| OncoZ | 842,880 | 804,121 | 1,756 | 104.82 | 31.41 |
| NeuroY | 788,970 | 795,083 | 3,757 | 99.23 | 29.40 |

**The two-way read:** North is the largest region but slightly below target. East is the smallest region but comfortably above target. CardioX is the largest product but below target; OncoZ is smaller but above target. A single “98.76% overall” hides both stories.

#### 24.18.3 Quarter comparison

```python
q1 = sales[sales["month"].between("2026-01-01", "2026-03-01")]
q2 = sales[sales["month"].between("2026-04-01", "2026-06-01")]

q1_revenue = q1["revenue"].sum()
q2_revenue = q2["revenue"].sum()
q2_target = q2["target_revenue"].sum()
q2_achievement = 100 * q2_revenue / q2_target
q2_growth = 100 * (q2_revenue - q1_revenue) / q1_revenue
```

| Metric | Q1 | Q2 | Change |
|---|---:|---:|---:|
| Revenue | ₹1,200,450 | ₹1,482,870 | +23.53% |
| Target | — | ₹1,509,330 | — |
| Achievement | — | 98.25% | — |

Q2 grew 23.53% over Q1 but still missed target by 1.75%. Growth and achievement are different questions. The first asks “are we moving?”; the second asks “are we moving fast enough?”.

#### 24.18.4 Q2 region × product matrix

```python
q2_rp = (
    q2.merge(regions, on="region_id", how="left", validate="many_to_one")
      .merge(products, on="product_id", how="left", validate="many_to_one")
)
q2_matrix = q2_rp.pivot_table(
    index="region_name",
    columns="product_name",
    values="revenue",
    aggfunc="sum",
    fill_value=0,
    margins=True,
    margins_name="Total",
)
```

| region_name | CardioX | NeuroY | OncoZ | Total |
|---|---:|---:|---:|---:|
| East | 103,230 | 72,660 | 88,800 | 264,690 |
| North | 180,774 | 120,960 | 177,600 | 479,334 |
| South | 150,624 | 102,900 | 147,360 | 400,884 |
| West | 129,162 | 90,720 | 118,080 | 337,962 |
| Total | 563,790 | 387,240 | 531,840 | 1,482,870 |

**Matrix reading:** North is the largest cell in every product, but its Q2 achievement is 98.41%. East is the only region above 100% in Q2 (102.51%), but it contributes only 17.85% of Q2 revenue. The matrix prevents the mistake of reading the total and assuming every cell is healthy.

#### 24.18.5 Concentration

```python
cells = (
    sales_rp.groupby(["region_name", "product_name"], as_index=False)
            .agg(revenue=("revenue", "sum"))
            .sort_values("revenue", ascending=False)
)
cells["share_pct"] = 100 * cells["revenue"] / cells["revenue"].sum()
top3_share = cells.head(3)["share_pct"].sum()
```

| region_name | product_name | revenue | share_pct |
|---|---|---:|---:|
| North | CardioX | 337,134 | 12.56 |
| North | OncoZ | 281,280 | 10.48 |
| South | CardioX | 280,944 | 10.47 |
| North | NeuroY | 246,540 | 9.19 |
| West | CardioX | 240,882 | 8.98 |

Top three cells: **33.52%** of revenue. That is a useful concentration fact, not automatically a risk. The next question is whether those cells are growing or whether the concentration is stable.

#### 24.18.6 Price–volume decomposition

CardioX, April to May 2026:

```python
card = sales_rp[
    (sales_rp["product_name"] == "CardioX")
    & (sales_rp["month"].isin(["2026-04-01", "2026-05-01"]))
]
card_pv = card.groupby("month", as_index=False).agg(
    units=("units", "sum"),
    revenue=("revenue", "sum"),
)
card_pv["price"] = card_pv["revenue"] / card_pv["units"]
```

| month | units | revenue | price |
|---|---:|---:|---:|
| 2026-04-01 | 1,459 | 175,080 | 120.00 |
| 2026-05-01 | 1,445 | 182,070 | 126.00 |

Revenue change: `182,070 − 175,080 = +6,990`.

| Effect | Calculation | Value |
|---|---|---:|
| Price | `(126 − 120) × 1,459` | +8,754 |
| Volume | `(1,445 − 1,459) × 120` | −1,680 |
| Interaction | `(126 − 120) × (1,445 − 1,459)` | −84 |
| **Total** |  | **+6,990** |

**Read:** the price increase added ₹8,754, but volume fell by 14 units and subtracted ₹1,680; the interaction is −₹84. Net revenue rose ₹6,990. The next question is whether the volume decline is a price response or a mix/availability effect. Do not call it elasticity from one month of data.

#### 24.18.7 TRx trend

```python
rx_monthly = (
    prescriptions.groupby("month", as_index=False)
                 .agg(trx=("trx", "sum"), new_rx=("new_rx", "sum"))
)
rx_monthly["mom_pct"] = rx_monthly["trx"].pct_change() * 100
rx_monthly["running_total"] = rx_monthly["trx"].cumsum()
rx_monthly["ma_3m"] = rx_monthly["trx"].rolling(3).mean()
rx_monthly["new_rx_share_pct"] = 100 * rx_monthly["new_rx"] / rx_monthly["trx"]
```

| month | trx | new_rx | new_rx_share_pct | mom_pct | running_total | ma_3m |
|---|---:|---:|---:|---:|---:|---:|
| 2026-01-01 | 208 | 66 | 31.73 | — | 208 | — |
| 2026-02-01 | 241 | 72 | 29.88 | 15.87 | 449 | — |
| 2026-03-01 | 273 | 75 | 27.47 | 13.28 | 722 | 240.67 |
| 2026-04-01 | 306 | 87 | 28.43 | 12.09 | 1,028 | 273.33 |
| 2026-05-01 | 327 | 90 | 27.52 | 6.86 | 1,355 | 302.00 |
| 2026-06-01 | 344 | 96 | 27.91 | 5.20 | 1,699 | 325.67 |

**Read:** TRx grew every month, but the month-over-month rate decelerated from 15.87% to 5.20%. The 3-month average is still rising (325.67 in June). New Rx share is stable around 28%. That is a “growth, but slowing” story — not a “decline” story and not a “boom” story.

#### 24.18.8 June product mix

```python
june = prescriptions[prescriptions["month"] == "2026-06-01"]
june_product = (
    june.merge(products, on="product_id", how="left", validate="many_to_one")
        .groupby("product_name", as_index=False)
        .agg(trx=("trx", "sum"), new_rx=("new_rx", "sum"))
)
june_product["share_pct"] = 100 * june_product["trx"] / june_product["trx"].sum()
```

| product_name | trx | new_rx | share_pct |
|---|---:|---:|---:|
| OncoZ | 124 | 34 | 36.05 |
| CardioX | 115 | 33 | 33.43 |
| NeuroY | 105 | 29 | 30.52 |

June is balanced across products: no product is above 37% or below 30%. That is a portfolio story, not a single-product story.

#### 24.18.9 Physician concentration and retention

```python
june_doc = (
    june.groupby("physician_id", as_index=False)
        .agg(trx=("trx", "sum"))
        .sort_values("trx", ascending=False)
)
june_doc["share_pct"] = 100 * june_doc["trx"] / june_doc["trx"].sum()
```

| Metric | Value |
|---|---:|
| Active physicians in June | 14 |
| June TRx | 344 |
| Median June TRx per physician | 22 |
| Mean June TRx per physician | 24.57 |
| Top 3 share | 38.37% |
| Top 5 share | 58.14% |
| Top 10 share | 88.08% |

```python
active = prescriptions[["physician_id", "month"]].drop_duplicates()
jan = set(active.loc[active["month"] == "2026-01-01", "physician_id"])
jun = set(active.loc[active["month"] == "2026-06-01", "physician_id"])
retention = 100 * len(jan & jun) / len(jan)
```

| Metric | Value |
|---|---:|
| Active in January | 16 |
| Active in June | 14 |
| Active in both | 14 |
| January-to-June retention | 87.50% |
| Lost physicians | P-041, P-042 |

P-041 and P-042 are historical/lapsed prescribers in the physician dimension. They have January–April prescriptions but no May–June prescriptions. The 87.5% retention is therefore a **cohort** statistic for the 16 January prescribers, not a statement about all 42 physicians in the dimension.

#### 24.18.10 Calls

```python
call_summary = pd.Series({
    "calls": len(calls),
    "minutes": calls["duration_min"].sum(),
    "avg_minutes": calls["duration_min"].mean(),
})
```

| Metric | Value |
|---|---:|
| Calls | 120 |
| Total minutes | 4,335 |
| Average minutes | 36.13 |
| Physicians called | 40 |
| Calls per physician | 3.0 |

| call_type | calls | share | avg_minutes | total_minutes |
|---|---:|---:|---:|---:|
| F2F | 80 | 66.67% | 36.56 | 2,925 |
| Remote | 40 | 33.33% | 35.25 | 1,410 |

| region_name | calls | F2F | avg_minutes |
|---|---:|---:|---:|
| East | 30 | 20 | 38.0 |
| North | 30 | 20 | 33.5 |
| South | 30 | 20 | 39.5 |
| West | 30 | 20 | 33.5 |

**Read:** activity is balanced at 30 calls per region and 20 F2F per region. The average duration is highest in South (39.5 minutes) and lowest in North/West (33.5). A balanced call plan does not prove effective coverage; it only proves the plan was balanced. Pair calls with TRx and physician potential before drawing a productivity conclusion.

#### 24.18.11 The five-sentence commercial readout

1. **Headline:** “Revenue is ₹2.68m against a ₹2.72m target, 98.76% achievement, with Q2 growth of 23.53% but Q2 achievement of 98.25%.”
2. **Region:** “North is 32.23% of revenue but 97.56% achievement; East is only 17.93% of revenue but 104.51% achievement.”
3. **Product:** “CardioX is 39.19% of revenue but 94.06% achievement; OncoZ is 31.41% of revenue and 104.82% achievement.”
4. **Trend:** “TRx grew from 208 to 344, but month-over-month growth slowed from 15.87% to 5.20%; June new Rx share is 27.91%.”
5. **Action:** “Protect OncoZ and East, diagnose CardioX and North, and check whether the slowing TRx growth is access, coverage, or base effect before changing the plan.”

---

### Check yourself — 24.18

1. What is overall achievement, and what is the denominator?
2. How many rows are at or above target, and how many have a target?
3. Which region has the highest achievement, and which has the largest revenue share?
4. Which product has the highest achievement, and which has the largest revenue share?
5. What is Q2 growth, and what is Q2 achievement?
6. What are the top three region–product cells, and what share do they represent?
7. What drives the CardioX April–May revenue change?
8. What is the June TRx, and how has month-over-month growth changed?
9. What is January-to-June physician retention, and what is the denominator?
10. What is the call mix, and why is a balanced call plan not proof of productivity?

Answers are in 24.24.

---
### 24.19 Automation and reproducible scripts

#### 24.19.1 Why automate this report

The commercial pack has the same shape every month:

1. Load sales, survey, prescriptions, and dimensions.
2. Validate row counts and totals.
3. Build KPI, region, product, survey, and Rx tables.
4. Export a workbook and tidy CSVs.
5. Log what ran.

If you do that by hand, you will eventually forget a filter, paste a stale month, or overwrite a sheet. A script makes the steps explicit and repeatable. The JD mentions process improvement; a small, reliable report script is a better interview story than “I know pandas”.

A ready-to-run example is in **`data/python/build_report.py`**. The rest of this section explains its structure. Run it from anywhere:

```bash
python data/python/build_report.py
```

Expected log:

```text
INFO loading data from .../data/sql/csv
INFO all 11 validation checks passed
INFO wrote trinity_report.xlsx and 8 CSVs to .../outputs
INFO report complete
```

#### 24.19.2 Project layout

```text
trinity-associate-analyst-preparation/
├── data/
│   ├── sql/csv/                 # shared fictional source data
│   └── python/
│       ├── build_report.py      # entry point
│       ├── requirements.txt
│       └── README.md
├── outputs/                     # generated, not source
│   ├── trinity_report.xlsx
│   ├── kpi.csv
│   ├── region.csv
│   ├── product.csv
│   ├── survey_stage.csv
│   ├── survey_top2.csv
│   ├── survey_nps.csv
│   ├── rx_monthly.csv
│   └── qa_log.csv
└── 24-python-for-data-analysis.md
```

**Rule:** data is input; outputs are disposable. Never edit a source CSV in place to make a report look right. Fix the pipeline or document the exclusion.

#### 24.19.3 The five functions

| Function | Input | Output | Job |
|---|---|---|---|
| `load_tables` | data directory | dict of DataFrames | Read with explicit parsing |
| `validate` | tables | QA DataFrame | Fail before export if a check fails |
| `build_report` | tables | dict of tidy DataFrames | Compute KPI, region, product, survey, Rx |
| `export` | report + QA | workbook + CSVs | Write named outputs |
| `main` | — | log | Orchestrate the four steps |

This is deliberately small. You do not need a class hierarchy for a monthly report. You need clear boundaries and a single entry point.

#### 24.19.4 Load with explicit rules

```python
def load_tables(data_dir: Path = DATA_DIR) -> dict[str, pd.DataFrame]:
    return {
        "sales": pd.read_csv(data_dir / "sales.csv"),
        "regions": pd.read_csv(data_dir / "regions.csv"),
        "products": pd.read_csv(data_dir / "products.csv"),
        "physicians": pd.read_csv(data_dir / "physicians.csv"),
        "survey": pd.read_csv(data_dir / "survey.csv", parse_dates=["response_date"]),
        "items": pd.read_csv(
            data_dir / "survey_items.csv",
            dtype={"score_raw": "string"},
            keep_default_na=False,
        ),
        "prescriptions": pd.read_csv(data_dir / "prescriptions.csv"),
    }
```

**Why a dict?** It keeps table names visible and makes it hard to confuse `survey` with `items`. It also makes the validation function easy to test.

#### 24.19.5 Validate before you compute

```python
def validate(tables):
    sales = tables["sales"]
    survey = tables["survey"]
    items = tables["items"]
    rx = tables["prescriptions"]

    clean_survey = (
        survey.loc[survey["response_date"].between("2026-01-15", "2026-03-31")]
        .sort_values(["physician_id", "response_id"])
        .drop_duplicates("physician_id", keep="first")
    )
    items = items.assign(score_num=pd.to_numeric(items["score_raw"], errors="coerce"))
    invalid_items = int((~items["score_num"].isin([1, 2, 3, 4, 5])).sum())

    checks = [
        ("sales_rows", len(sales), 72, len(sales) == 72),
        ("sales_revenue", int(sales["revenue"].sum()), 2_683_320,
         int(sales["revenue"].sum()) == 2_683_320),
        ("revenue_reconciliation",
         int((sales["revenue"] - sales["units"] * sales["unit_price"]).abs().sum()),
         0,
         int((sales["revenue"] - sales["units"] * sales["unit_price"]).abs().sum()) == 0),
        ("survey_clean_rows", len(clean_survey), 39, len(clean_survey) == 39),
        ("invalid_survey_items", invalid_items, 2, invalid_items == 2),
        ("rx_total", int(rx["trx"].sum()), 1_699,
         int(rx["trx"].sum()) == 1_699),
    ]

    qa = pd.DataFrame(checks, columns=["check", "actual", "expected", "passed"])
    if not qa["passed"].all():
        raise AssertionError(qa.loc[~qa["passed"]].to_string(index=False))
    return qa
```

The real script has 11 checks, including survey duplicates, unique physicians, invalid items, and June TRx. The pattern is:

- **actual** value,
- **expected** value,
- **passed** boolean,
- raise if any fail,
- export the log with the report.

A check that only logs a warning is easy to ignore. A check that stops the export is a control.

#### 24.19.6 Build tidy tables

Each output should have one grain and one row per unit:

| Output | Grain | Key columns |
|---|---|---|
| `kpi` | metric | `metric`, `value` |
| `region` | region | `region_name`, `revenue`, `target`, `achievement_pct`, `share_pct` |
| `product` | product | `product_name`, `revenue`, `target`, `achievement_pct`, `share_pct` |
| `survey_stage` | awareness stage | `awareness_stage`, `n`, `trx` |
| `survey_top2` | item | `item`, `valid_n`, `top2_pct`, `mean_score` |
| `survey_nps` | metric | `metric`, `value` |
| `rx_monthly` | month | `month`, `trx`, `new_rx`, `mom_pct`, `running_total`, `ma_3m` |
| `qa_log` | check | `check`, `actual`, `expected`, `passed` |

A tidy table is long, not wide. It is easy to filter, concatenate, and chart. A presentation table can be wide; build that at the end, not in the middle.

#### 24.19.7 Export the workbook and CSVs

```python
with pd.ExcelWriter(out_dir / "trinity_report.xlsx", engine="openpyxl") as writer:
    report["kpi"].to_excel(writer, sheet_name="KPI", index=False)
    report["region"].to_excel(writer, sheet_name="Region", index=False)
    report["product"].to_excel(writer, sheet_name="Product", index=False)
    report["survey_stage"].to_excel(writer, sheet_name="Survey_Stage", index=False)
    report["survey_top2"].to_excel(writer, sheet_name="Survey_Top2", index=False)
    report["survey_nps"].to_excel(writer, sheet_name="Survey_NPS", index=False)
    report["rx_monthly"].to_excel(writer, sheet_name="Rx_Monthly", index=False)
    qa.to_excel(writer, sheet_name="QA_Log", index=False)
```

**Why both Excel and CSV?**

- Excel is for a human reviewer; sheets are separated and readable.
- CSV is for another script, a database load, or a reproducibility check.
- The QA log travels with both.

Do not email a CSV with no README and no QA log. The recipient cannot tell what was excluded.

#### 24.19.8 Logging

```python
import logging

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s %(levelname)s %(message)s",
)
logger = logging.getLogger(__name__)

logger.info("loading data from %s", DATA_DIR)
logger.info("all %d validation checks passed", len(qa))
logger.info("wrote %s", workbook.name)
```

A log answers: what ran, when, on which path, and whether validation passed. It does not replace the QA table; it records the run.

#### 24.19.9 `main` and the entry point

```python
def main() -> None:
    tables = load_tables()
    qa = validate(tables)
    report = build_report(tables)
    export(report, qa)
    logger.info("report complete")


if __name__ == "__main__":
    main()
```

`if __name__ == "__main__"` means the script runs when executed directly, but can be imported and tested without side effects. That is a small habit with a large payoff.

#### 24.19.10 Notebook vs script

| Use | Notebook | Script |
|---|---|---|
| Explore a new dataset | Yes | No |
| Explain a chart to a stakeholder | Yes | No |
| Repeat a monthly report | No | Yes |
| Code review | Harder | Easier |
| Scheduling | Awkward | Natural |
| Testing | Awkward | Natural |
| Accidental out-of-order execution | Common | Impossible |

**Best practice:** explore in a notebook, then move the stable steps into a script. Do not schedule a notebook with 40 hidden cells and a manual “run all”.

#### 24.19.11 Scheduling

On macOS/Linux, `cron` can run the script monthly:

```cron
0 7 1 * * cd /path/to/pack && /path/to/.venv/bin/python data/python/build_report.py >> outputs/report.log 2>&1
```

On Windows, use Task Scheduler with the virtual environment’s `python.exe`. In a company, the scheduler is usually Airflow, Azure Data Factory, or a Power Automate flow; the principle is the same: fixed environment, fixed input, logged output, alert on failure.

#### 24.19.12 What to automate — and what not to

**Automate:**

- Repeated loading and joining.
- Fixed QA checks.
- Recurring exports.
- A report that takes more than 15 minutes by hand.
- A step that has caused a past error.

**Do not automate:**

- A process nobody has defined.
- A decision that requires judgment (e.g., whether to discontinue a product).
- A one-off analysis with no repeat.
- A dashboard that Power BI already refreshes.
- A workflow with no owner and no monitoring.

**Interview line:** “I automate the repeated, checkable part and keep the judgment visible. The script produces the QA log and the tables; the analyst writes the recommendation.”

#### 24.19.13 Mini-project: extend the report

Add one of these to `build_report.py`:

1. **Region × product sheet:** Q2 matrix with margins.
2. **Specialty × stage crosstab:** counts and row percentages.
3. **Top-10 physician sheet:** June TRx, share, cumulative share.
4. **Exception sheet:** rows where revenue is below 80% of target, with a reason column.
5. **Call coverage sheet:** calls and F2F share by region.

For each, add a validation check. For example, the region × product sheet must sum to total revenue. If it does not, the pivot is wrong.

---

### Check yourself — 24.19

1. Why does the script validate before it builds the report?
2. What is the difference between a log and a QA table?
3. Why export both Excel and CSV?
4. What does `if __name__ == "__main__"` do?
5. Name two things you should not automate.
6. What should happen if a validation check fails?
7. Why is a tidy table easier to export than a wide presentation table?
8. What is one check you would add to the region × product sheet?

Answers are in 24.24.

---

### 24.20 Visualization handoff

Python is not the final presentation layer in most analyst interviews. It is the place to make a clean table and a quick diagnostic chart; Excel, Power BI, or PowerPoint is where the story is formatted. Still, you should be able to produce a readable chart and explain your choices.

#### 24.20.1 Chart choice

| Question | Chart | Avoid |
|---|---|---|
| Compare categories | Horizontal bar | Pie with many slices |
| Show a trend over time | Line | 3D line |
| Show a distribution | Histogram / box | Bar of averages only |
| Show a relationship | Scatter | Dual-axis chart with no reason |
| Show composition | Stacked bar (few categories) | Pie with 10 slices |
| Show a ranking | Sorted bar | Alphabetical bar |
| Show a waterfall | Waterfall / bridge | Stacked bar pretending to be a bridge |

#### 24.20.2 A diagnostic bar chart

```python
import matplotlib.pyplot as plt

q2_region = (
    q2_rp.groupby("region_name", as_index=False)
         .agg(revenue=("revenue", "sum"),
              target=("target_revenue", "sum"))
)
q2_region["achievement_pct"] = 100 * q2_region["revenue"] / q2_region["target"]

region_plot = q2_region.sort_values("achievement_pct")
fig, ax = plt.subplots(figsize=(8, 4.5))
bars = ax.barh(region_plot["region_name"], region_plot["achievement_pct"], color="#4C78A8")
ax.axvline(100, color="#D64545", linestyle="--", linewidth=1)
ax.set_xlabel("Q2 achievement vs target (%)")
ax.set_title("Q2 achievement by region — fictional practice data")
ax.set_xlim(90, 106)
ax.bar_label(bars, fmt="%.1f%%", padding=3)
ax.spines[["top", "right"]].set_visible(False)
fig.tight_layout()
fig.savefig("outputs/q2_region_achievement.png", dpi=150)
```

**Why this works:** it answers one question, sorts the categories, starts the axis at a defensible value, marks 100%, labels the values, and removes chartjunk. It does not use a 3D effect, a rainbow palette, or an unlabeled axis.

**Axis caution:** starting at 90 exaggerates differences. That is acceptable for an achievement chart where 100% is the reference, but say so. For a revenue chart, start at zero.

#### 24.20.3 A trend chart

```python
fig, ax = plt.subplots(figsize=(8, 4.5))
ax.plot(rx_monthly["month"], rx_monthly["trx"], marker="o", color="#2F6B4F")
ax.set_ylabel("TRx")
ax.set_title("Monthly TRx — fictional practice data")
ax.spines[["top", "right"]].set_visible(False)
fig.tight_layout()
fig.savefig("outputs/rx_trend.png", dpi=150)
```

Add the 3-month moving average only if it clarifies the signal. Two lines are fine; five lines are usually a dashboard, not a slide.

#### 24.20.4 Chart checklist

- One message per chart.
- Title states the takeaway, not the metric name.
- Axis labels include units.
- Source and date are visible.
- Categories sorted by the measure.
- Reference line at 100% or target where relevant.
- No more than four colors.
- No 3D, shadows, or gradient fills.
- The chart still makes sense in grayscale.
- The underlying table is available.

#### 24.20.5 Handoff table

| Destination | What to send | What not to send |
|---|---|---|
| Excel | Tidy CSV + a formatted summary sheet | A screenshot of a notebook |
| Power BI | Clean fact/dimension tables + measures | A pivot table with merged cells |
| PowerPoint | One chart per slide + one-line takeaway | A 12-chart dashboard slide |
| Email | Headline, number, caveat, next question | An unexplained attachment |

**Interview line:** “I use Python to produce the clean table and diagnostic chart, then move the final view to the tool the audience will actually use. The analysis is not finished until the handoff is readable.”

---

### 24.21 Performance and interview craft

#### 24.21.1 pandas vs SQL vs Excel: choose the layer

| Situation | Best first tool | Why |
|---|---|---|
| 50 million rows in a warehouse | SQL | Push filtering/aggregation to the database |
| 50,000 rows in a CSV | pandas | Fast enough, easy to inspect |
| A one-off pivot for a manager | Excel | Fastest path to a formatted answer |
| A monthly repeatable report | Python + Excel/Power BI | Automatable, auditable |
| A live shared dashboard | Power BI | Refresh, permissions, distribution |
| A statistical test | Python/R | Reproducible, more methods |
| A quick sanity check | Excel | Immediate visual inspection |

**Do not say** “Python can do everything.” **Say** “I use the lightest tool that makes the result reliable, and I move the data to the presentation layer when the analysis is stable.”

#### 24.21.2 Ten interview questions with model answers

| Question | Model answer |
|---|---|
| `loc` vs `iloc`? | `.loc` selects by label or boolean mask; `.iloc` selects by integer position. Use `.loc` for business logic, `.iloc` for positional exploration. |
| `merge` vs `join`? | `merge` combines on one or more keys and gives control over suffixes and validation; `join` is a convenience method that defaults to the index. Prefer `merge` for explicit keys. |
| `apply` vs vectorization? | Vectorized operations run in C and are faster; `apply` is a Python loop under the hood. Use `apply` for a small custom function; vectorize for row-wise arithmetic. |
| What is `SettingWithCopyWarning`? | It warns that you may be assigning to a temporary slice rather than the original DataFrame. Use `.copy()` or assign with `.loc` on the original. |
| How do you handle missing values? | Inspect first, decide the business rule, then exclude, impute, or flag. Never fill a blank target with zero for achievement. |
| What does `groupby().transform()` do? | Returns a group-level result aligned to every original row; `agg` returns one row per group. |
| Why is the first `shift(1)` value NaN? | There is no previous row. Do not fill it with zero unless zero is the correct business value. |
| How do you deduplicate? | Define the key and the keep rule, sort deterministically, then `drop_duplicates(subset=key, keep=...)`. Report how many rows were removed. |
| How do you validate a join? | Use `validate="many_to_one"` (or the correct cardinality) and `indicator=True` to audit unmatched rows. Check row counts before and after. |
| How do you make a report reproducible? | Fixed data snapshot, pinned environment, one entry point, assertions, named outputs, and a log. |

#### 24.21.3 The 60-second analysis explanation

Use this structure in a technical interview:

1. **Clarify the grain:** “One row per month × region × product; 72 rows.”
2. **State the cleaning rules:** “Two blank targets excluded from achievement; three blank call counts left missing.”
3. **State the calculation:** “Revenue sum divided by target sum, not the average of row-level achievement.”
4. **State the check:** “Revenue reconciles to units × price; no orphan keys.”
5. **State the finding:** “98.76% overall, but East is 104.51% and CardioX is 94.06%.”
6. **State the caveat:** “Small fictional sample; achievement is not statistical significance.”

That is more impressive than a silent notebook with 40 cells.

#### 24.21.4 Code review checklist

- [ ] Column names are explicit; no `df.columns = [...]` without a check.
- [ ] Dates are parsed once.
- [ ] Missing-value rules are stated.
- [ ] Joins use `validate=`.
- [ ] Aggregations use `sum`/`size`/`mean` deliberately.
- [ ] Denominators are stated.
- [ ] Row counts are checked before and after joins.
- [ ] Outputs have one grain.
- [ ] Assertions cover the headline totals.
- [ ] The script runs top-to-bottom in a fresh environment.
- [ ] A README or module explains how to run it.
- [ ] No source data was edited in place.

---

### 24.22 Twenty pandas interview patterns

Each pattern is a small, reusable answer. The code assumes the standard load block.

#### Pattern 1 — Load and inspect

```python
sales = pd.read_csv(DATA / "sales.csv")
print(sales.shape)
print(sales.head())
print(sales.dtypes)
print(sales.isna().sum())
```

**Line:** “I check shape, types, and missingness before I compute anything.”

#### Pattern 2 — Filter and select

```python
q2 = sales.loc[
    sales["month"].between("2026-04-01", "2026-06-01"),
    ["sale_id", "month", "region_id", "product_id", "revenue"],
]
```

**Line:** “I filter rows first, then select columns; the parentheses make the boolean logic explicit.”

#### Pattern 3 — Derived column

```python
sales = sales.assign(
    achievement_pct=lambda d: 100 * d["revenue"] / d["target_revenue"]
)
```

**Line:** “The blank target becomes NaN, which is the correct ‘not applicable’ value; I do not fill it with zero.”

#### Pattern 4 — Clean text

```python
physicians["specialty_clean"] = physicians["specialty"].str.strip()
survey["region_clean"] = survey["region_raw"].replace({"north ": "North", "N": "North"})
```

**Line:** “I count the raw variants first, map them explicitly, and keep an audit trail.”

#### Pattern 5 — Deduplicate

```python
clean_survey = (
    survey.loc[survey["response_date"].between("2026-01-15", "2026-03-31")]
          .sort_values(["physician_id", "response_id"])
          .drop_duplicates("physician_id", keep="first")
)
```

**Line:** “I define the key, the window, and the keep rule; 42 raw becomes 39 fielded.”

#### Pattern 6 — Safe join

```python
sales_r = sales.merge(regions, on="region_id", how="left", validate="many_to_one")
```

**Line:** “The validation protects the revenue total from a duplicated dimension row.”

#### Pattern 7 — Anti-join

```python
audit = physicians.merge(
    calls[["physician_id"]].drop_duplicates(),
    on="physician_id", how="left", indicator=True,
)
no_calls = audit.loc[audit["_merge"] == "left_only", ["physician_id", "physician_name"]]
```

**Line:** “The anti-join shows P-041 and P-042 have no calls; I would not silently drop them.”

#### Pattern 8 — Groupby KPI

```python
region = (
    sales.merge(regions, on="region_id", validate="many_to_one")
         .groupby("region_name", as_index=False)
         .agg(revenue=("revenue", "sum"), target=("target_revenue", "sum"))
)
region["achievement_pct"] = 100 * region["revenue"] / region["target"]
```

**Line:** “I sum revenue and target separately, then divide; I do not average row-level percentages.”

#### Pattern 9 — Multiple aggregations

```python
profile = sales.groupby("region_id", as_index=False).agg(
    rows=("sale_id", "size"),
    revenue=("revenue", "sum"),
    avg_row=("revenue", "mean"),
    products=("product_id", "nunique"),
)
```

**Line:** “`size` counts rows; `nunique` counts distinct products; they answer different questions.”

#### Pattern 10 — Top N per group

```python
top3 = (
    june.sort_values(["specialty", "trx", "physician_id"],
                     ascending=[True, False, True])
        .groupby("specialty", as_index=False)
        .head(3)
)
```

**Line:** “I sort with a tie-breaker, then take the first three per group; the result is deterministic.”

#### Pattern 11 — Percent of total

```python
cells["share_pct"] = 100 * cells["revenue"] / cells["revenue"].sum()
```

**Line:** “The denominator is total revenue, not the number of rows; the shares sum to 100%.”

#### Pattern 12 — Pivot table

```python
matrix = sales_rp.pivot_table(
    index="region_name", columns="product_name",
    values="revenue", aggfunc="sum", fill_value=0, margins=True,
)
```

**Line:** “I specify the aggregation; the default mean would be wrong for revenue.”

#### Pattern 13 — Crosstab

```python
pd.crosstab(clean_survey["specialty"], clean_survey["awareness_stage"], margins=True)
```

**Line:** “A crosstab counts combinations; it shows the sample is balanced by design.”

#### Pattern 14 — Running total

```python
rx_monthly["running_total"] = rx_monthly["trx"].cumsum()
```

**Line:** “The running total is 1,699 by June; it is a cumulative view, not a monthly KPI.”

#### Pattern 15 — Month-over-month growth

```python
rx_monthly["mom_pct"] = rx_monthly["trx"].pct_change() * 100
```

**Line:** “The first month has no prior period, so it is NaN; growth decelerated to 5.20% in June.”

#### Pattern 16 — Rolling average

```python
rx_monthly["ma_3m"] = rx_monthly["trx"].rolling(3).mean()
```

**Line:** “The first two values are NaN because a 3-month window is not complete; June’s MA3 is 325.67.”

#### Pattern 17 — Weighted average

```python
weighted_items = valid_items.merge(
    clean_survey[["response_id", "weight"]],
    on="response_id", how="inner", validate="many_to_one",
)
weighted_top2 = (
    100 * weighted_items.loc[weighted_items["score_num"].isin([4, 5]), "weight"].sum()
    / weighted_items["weight"].sum()
)
```

**Line:** “The weight column changes the denominator; I report both weighted and unweighted because they tell different stories.”

#### Pattern 18 — Survey top-2-box and NPS

```python
top2 = 100 * valid_items["score_num"].isin([4, 5]).mean()
promoters = (clean_survey["recommend_score"] >= 9).sum()
detractors = (clean_survey["recommend_score"] <= 6).sum()
nps = 100 * (promoters - detractors) / len(clean_survey)
```

**Line:** “Top-2-box uses the valid item base; NPS uses the respondent base; passives stay in the denominator.”

#### Pattern 19 — Cohort retention

```python
active = prescriptions[["physician_id", "month"]].drop_duplicates()
jan = set(active.loc[active["month"] == "2026-01-01", "physician_id"])
jun = set(active.loc[active["month"] == "2026-06-01", "physician_id"])
retention = 100 * len(jan & jun) / len(jan)
```

**Line:** “Retention is 14 of the 16 January prescribers, 87.5%; the denominator is the starting cohort.”

#### Pattern 20 — Assert and export

```python
assert sales.shape == (72, 9)
assert abs(sales["revenue"].sum() - 2_683_320) < 1
region.to_csv("outputs/region_summary.csv", index=False)
```

**Line:** “The assertions make the report fail loudly if the input changes; the export is a named, reusable output.”

---

### Check yourself — 24.22

1. Which pattern would you use to find physicians with no calls?
2. Which pattern computes a share of total?
3. Which pattern handles a top-N per group?
4. Which pattern would you use for a month-over-month growth question?
5. Which pattern combines a fact with a dimension safely?
6. Which pattern would you use for a survey item score?
7. Which pattern would you use to check the final revenue total?
8. Which pattern would you use to show a region × product matrix?

Answers are in 24.24.

---
## Part D — Practice

### 24.23 Timed drills

Use a timer. Write the code in a fresh notebook or script. Do not look at the answer key until the timer ends. If you finish early, explain your output out loud in one sentence.

| Drill | Time | Marks | Focus |
|---|---:|---:|---|
| 1 | 20 min | 12 | Python foundations |
| 2 | 25 min | 16 | pandas inspection, filtering, derived columns |
| 3 | 30 min | 20 | groupby, joins, pivots |
| 4 | 35 min | 20 | survey, commercial, automation |

**Scoring:** 1 mark for a correct result, 1 mark for a correct explanation/denominator. A wrong number with the right method can still earn the explanation mark.

---

#### Drill 1 — Python foundations (20 minutes, 12 marks)

1. **(2 marks)** Given `revenue = 2_683_320` and `target = 2_717_035`, print the achievement to two decimal places with a `%` sign.
2. **(2 marks)** Build a dictionary mapping `N`, `S`, `E`, `W` to `North`, `South`, `East`, `West`. Print the value for `S`.
3. **(3 marks)** Write a function `achievement_pct(revenue, target)` that returns `None` when the target is `None` or zero, otherwise returns `100 * revenue / target`. Test it with `(481_200, 460_455)` and `(100, 0)`.
4. **(2 marks)** Use a list comprehension to keep product names longer than five characters from `["CardioX", "NeuroY", "OncoZ"]`.
5. **(2 marks)** Loop over `[("East", 481_200), ("North", 864_954)]` and print `East: ₹481,200` using an f-string.
6. **(1 mark)** Why is `df.iterrows()` usually a bad choice for a 100,000-row DataFrame?

---

#### Drill 2 — pandas inspection and filtering (25 minutes, 16 marks)

Use `data/sql/csv/sales.csv`.

1. **(2 marks)** Load the file and print its shape.
2. **(2 marks)** Print the dtypes of `target_revenue` and `calls`. Explain why neither is `int64`.
3. **(2 marks)** Count missing values in `target_revenue` and `calls`.
4. **(2 marks)** Filter to Q2 (April–June 2026) and print the row count.
5. **(2 marks)** Show the top three rows by revenue with `sale_id` and `revenue`.
6. **(2 marks)** Count rows with revenue above ₹60,000.
7. **(3 marks)** Create `achievement_pct = 100 * revenue / target_revenue`. Count rows with achievement at or above 100%. State the denominator.
8. **(1 mark)** What causes `SettingWithCopyWarning`, and how do you avoid it?

---

#### Drill 3 — groupby, joins, pivots (30 minutes, 20 marks)

Use `sales.csv`, `regions.csv`, and `products.csv`.

1. **(2 marks)** Merge sales to regions and products. Print the shape after each merge.
2. **(3 marks)** Build a region table with revenue, target, and achievement. Sort by achievement descending.
3. **(2 marks)** Build a product table with revenue, units, and revenue per unit. Sort by revenue descending.
4. **(3 marks)** Build the Q2 region table with revenue, target, and achievement.
5. **(2 marks)** Pivot Q2 revenue with region as rows and product as columns; include margins.
6. **(3 marks)** Find the top three region–product cells by revenue and their share of total revenue.
7. **(2 marks)** Find physicians with no calls. Name them.
8. **(2 marks)** Join the clean survey base (39 respondents) to `survey_items`. How many rows result, and why?
9. **(1 mark)** What does `validate="many_to_one"` protect against?

---

#### Drill 4 — survey, commercial, automation (35 minutes, 20 marks)

Use the full practice data.

1. **(3 marks)** Clean the survey: raw 42, unique 40, field 41, deduped 39. Show the stage counts.
2. **(3 marks)** Build the top-2-box table for Intent, Access, and Efficacy with valid n and mean score.
3. **(2 marks)** Compute NPS: promoters, passives, detractors, and the NPS value.
4. **(2 marks)** Compute weighted NPS and effective n.
5. **(3 marks)** Build the monthly TRx table with MoM growth, running total, and 3-month moving average.
6. **(2 marks)** Build the June product mix with TRx and share.
7. **(2 marks)** Compute January-to-June physician retention. State the denominator.
8. **(1 mark)** Compute the F2F/Remote call mix and average duration by type.
9. **(2 marks)** Decompose the CardioX April–May revenue change into price, volume, and interaction.
10. **(2 marks)** Write one assertion that would fail if the sales revenue total changed.

---

### Check yourself — 24.23

1. Which drill took the longest, and why?
2. Which question did you get numerically right but explain poorly?
3. Which pandas operation still feels slow to write?
4. What will you review before the next attempt?

Answers are in 24.24.

---

### 24.24 Answer key

#### 24.24.1 Check yourself 24.1–24.5

1. A list is mutable and ordered; a tuple is immutable and ordered.
2. A dict gives O(1)-style lookup from a code to a label, avoiding repeated `if` statements.
3. `98.76%`.
4. `iterrows()` converts each row to a Series, is slow, and can change dtypes.
5. Check `df.columns` for the exact name; then check spelling and case.

#### 24.24.2 Check yourself 24.6–24.15

1. A Series is one labelled column; a DataFrame is a table of columns sharing an index.
2. `(72, 9)`.
3. They contain blanks, so pandas uses `float64` and `NaN`.
4. `.loc` is label/boolean; `.iloc` is integer position.
5. `agg` returns one row per group; `transform` returns a value aligned to every original row.
6. One survey response has three items, so the join is one-to-many and triples the respondent rows.
7. `pivot_table` aggregates a numeric measure; `crosstab` counts frequency combinations.
8. `shift(1)` moves the previous row’s value down; the first row has no predecessor, so it is `NaN`.
9. `apply(axis=1)` runs a Python function per row; vectorised operations run in compiled code.
10. It raises when the right-side key is not unique, preventing a silent fan-out.

#### 24.24.3 Check yourself 24.16–24.17

1. The duplicate respondents have different `response_id` values, so the full rows are not exact duplicates.
2. `SR-020` Access = 7; `SR-021` Efficacy = N/A.
3. 39: 42 raw − 2 duplicate rows − 1 out-of-window response.
4. 36.84% with valid n=38.
5. NPS = 12 promoters − 15 detractors, divided by 39 = −7.69; 12 passives.
6. Weighted NPS = −6.81; weighting moves it +0.88 points.
7. `SR-022`; flag it and run sensitivity, do not automatically delete it.
8. The join multiplies one respondent by three items; `len()` counts item rows, not respondents.
9. Effective n accounts for uneven weights; it is slightly below raw n (37.78 vs 39).

#### 24.24.4 Check yourself 24.18

1. 98.76%; denominator is the sum of non-missing targets (₹2,717,035).
2. 28 at or above; 70 have a target.
3. East highest achievement (104.51%); North largest share (32.23%).
4. OncoZ highest achievement (104.82%); CardioX largest share (39.19%).
5. Q2 growth +23.53%; Q2 achievement 98.25%.
6. North–CardioX, North–OncoZ, South–CardioX; 33.52%.
7. Price +₹8,754, volume −₹1,680, interaction −₹84; net +₹6,990.
8. 344; MoM growth decelerated from 15.87% to 5.20%.
9. 87.5%; denominator is the 16 physicians active in January.
10. F2F 80 (66.67%), Remote 40 (33.33%); a balanced plan does not prove effective coverage or productivity.

#### 24.24.5 Check yourself 24.19

1. To stop a wrong report before it is exported.
2. A log records the run; a QA table records each check and its result.
3. Excel for human review; CSV for reuse and reproducibility.
4. It runs `main()` only when the file is executed directly, not when imported.
5. A process nobody has defined; a judgment-only decision.
6. Raise an exception and stop the export; investigate the input or the rule.
7. Tidy data has one grain and one row per unit, so it filters, joins, and exports cleanly.
8. The pivot’s grand total must equal total revenue.

#### 24.24.6 Check yourself 24.22

1. Pattern 7, anti-join.
2. Pattern 11, percent of total.
3. Pattern 10, top N per group.
4. Pattern 15, month-over-month growth.
5. Pattern 6, safe join.
6. Pattern 18, survey top-2-box.
7. Pattern 20, assert and export.
8. Pattern 12, pivot table.

#### 24.24.7 Drill 1 answers

1. ```python
print(f"{100 * revenue / target:.2f}%")  # 98.76%
```
2. ```python
region_map = {"N": "North", "S": "South", "E": "East", "W": "West"}
print(region_map["S"])  # South
```
3. ```python
def achievement_pct(revenue, target):
    if target is None or target == 0:
        return None
    return 100 * revenue / target

print(achievement_pct(481_200, 460_455))  # 104.505...
print(achievement_pct(100, 0))            # None
```
4. ```python
[p for p in ["CardioX", "NeuroY", "OncoZ"] if len(p) > 5]
# ['CardioX', 'NeuroY']
```
5. ```python
for region, revenue in [("East", 481_200), ("North", 864_954)]:
    print(f"{region}: ₹{revenue:,}")
```
6. `iterrows()` creates a Series per row, is slower than vectorised operations, and can coerce mixed types. Use column arithmetic, `groupby`, `merge`, or `apply` on a Series.

#### 24.24.8 Drill 2 answers

1. `sales.shape` → `(72, 9)`.
2. Both `float64`, because `target_revenue` has 2 blanks and `calls` has 3 blanks; `NaN` forces a float column.
3. `target_revenue` 2; `calls` 3.
4. 36 rows (3 months × 4 regions × 3 products).
5. ```python
sales.nlargest(3, "revenue")[["sale_id", "revenue"]]
```
| sale_id | revenue |
|---|---:|
| S-063 | 70,080 |
| S-061 | 66,276 |
| S-049 | 58,338 |

6. 2 rows.
7. ```python
sales["achievement_pct"] = 100 * sales["revenue"] / sales["target_revenue"]
print((sales["achievement_pct"] >= 100).sum())  # 28
```
Denominator: 70 rows with a non-missing target, not 72.
8. Assigning to a slice that may be a view. Avoid by using `.copy()` or assigning with `.loc` on the original.

#### 24.24.9 Drill 3 answers

1. `sales.merge(regions)` → `(72, 12)`; then merge products → `(72, 15)`.
2. | region | revenue | target | achievement |
|---|---:|---:|---:|
| East | 481,200 | 460,455 | 104.51 |
| West | 612,882 | 622,128 | 98.51 |
| North | 864,954 | 886,608 | 97.56 |
| South | 724,284 | 747,844 | 96.85 |
3. | product | revenue | units | revenue/unit |
|---|---:|---:|---:|
| CardioX | 1,051,470 | 8,608 | 122.15 |
| OncoZ | 842,880 | 1,756 | 480.00 |
| NeuroY | 788,970 | 3,757 | 210.00 |
4. | region | Q2 revenue | Q2 target | Q2 achievement |
|---|---:|---:|---:|
| East | 264,690 | 258,216 | 102.51 |
| North | 479,334 | 487,076 | 98.41 |
| South | 400,884 | 412,085 | 97.28 |
| West | 337,962 | 351,953 | 96.02 |
5. The Q2 matrix has 4 region rows × 3 product columns plus margins; North–CardioX is ₹180,774. (The separate full-period product × month matrix in 24.13.1 is a different pivot; CardioX January there is ₹157,200.)
6. North–CardioX ₹337,134 (12.56%), North–OncoZ ₹281,280 (10.48%), South–CardioX ₹280,944 (10.47%); top three = 33.52%.
7. P-041 Dr. Dev Malhotra and P-042 Dr. Ritu Bansal.
8. 117 rows: 39 clean respondents × 3 items. The join is one-to-many.
9. It protects against a duplicated dimension key causing row fan-out and inflated totals.

#### 24.24.10 Drill 4 answers

1. Stage counts: Unaided 8, Aided 8, Consideration 8, Regular 8, Trial 7.
2. | item | valid_n | top2 | mean |
|---|---:|---:|---:|
| Access | 38 | 36.84% | 3.03 |
| Efficacy | 38 | 47.37% | 3.45 |
| Intent | 39 | 35.90% | 3.10 |
3. 12 promoters, 12 passives, 15 detractors; NPS −7.69.
4. Weighted NPS −6.81; effective n 37.78.
5. Monthly TRx: 208, 241, 273, 306, 327, 344; MoM: —, 15.87%, 13.28%, 12.09%, 6.86%, 5.20%; running total: 208, 449, 722, 1,028, 1,355, 1,699; MA3: —, —, 240.67, 273.33, 302.00, 325.67.
6. OncoZ 124 (36.05%), CardioX 115 (33.43%), NeuroY 105 (30.52%).
7. 14/16 = 87.5%; denominator is the 16 physicians active in January.
8. F2F 80 (66.67%), average 36.56 min; Remote 40 (33.33%), average 35.25 min.
9. Price +₹8,754; volume −₹1,680; interaction −₹84; net +₹6,990.
10. ```python
assert abs(sales["revenue"].sum() - 2_683_320) < 1
```

#### 24.24.11 Drill reflection

There are no unique answers to the reflection questions. A good answer names a specific operation (e.g., “top N per group”), a specific error (e.g., “I forgot the denominator”), and a specific next action (e.g., “rewrite the groupby from memory twice”).

---

### 24.25 45-minute assessment

**Scenario.** You are an analyst at a fictional pharmaceutical company. The commercial lead has 45 minutes before a review and wants a June readout from the practice extract. Use pandas. Produce the tables and a five-bullet readout.

**Rules:**

- Time: 45 minutes.
- Total: 30 marks.
- Work in a notebook or a single script. Run top-to-bottom at the end.
- Use `data/sql/csv/` only. Do not edit the source files.
- State every exclusion and denominator.
- A correct number with no explanation earns partial credit. A correct explanation with a small arithmetic slip can still earn most of the method marks.
- The data is fictional. Do not present it as real market evidence.

**Suggested timing:** 5 min load/QA, 5 min KPI, 6 min region/product, 5 min Q2 matrix, 4 min survey clean, 6 min top-2/NPS, 6 min Rx, 2 min price–volume, 1 min readout, 5 min review.

---

#### Task 1 — Load and QA (4 marks)

Load `sales`, `survey`, `survey_items`, and `prescriptions`.

1. Print the shape of each table. (1)
2. Count missing `target_revenue` and `calls` in sales. (1)
3. Count duplicate `physician_id` rows in survey and invalid Likert records in `survey_items`. (1)
4. State the cleaning rules you will use for the survey base and the Likert items. (1)

---

#### Task 2 — Commercial KPI (4 marks)

Using `sales`:

1. Compute total revenue, total target, achievement %, units, and row count. (2)
2. Count rows at or above target and rows with a target. (1)
3. Explain why achievement uses `sum(revenue) / sum(target)`, not the average of row-level achievement. (1)

---

#### Task 3 — Region and product (4 marks)

1. Build a region table with revenue, target, achievement %, and share of total revenue. Sort by share descending. (2)
2. Build a product table with revenue, target, achievement %, units, and revenue per unit. Sort by revenue descending. (2)

---

#### Task 4 — Q2 matrix and concentration (3 marks)

1. Build a Q2 region × product revenue pivot with margins. (2)
2. Identify the top three region–product cells for the full period and their combined share of revenue. (1)

---

#### Task 5 — Survey cleaning (3 marks)

1. Show the funnel: raw 42 → unique 40 → in-window 41 → clean 39. (1)
2. Show the stage counts. (1)
3. Which stage is short of quota, and by how many? (1)

---

#### Task 6 — Top-2-box and NPS (4 marks)

1. Build a table for Intent, Access, and Efficacy with valid n, top-2-box %, and mean score. (2)
2. Compute promoters, passives, detractors, and unweighted NPS. (1)
3. Compute weighted NPS and effective n. (1)

---

#### Task 7 — Rx trend and mix (4 marks)

1. Build monthly TRx, new Rx, MoM growth, running total, and 3-month moving average. (2)
2. Build the June product mix with TRx and share. (1)
3. Compute January-to-June physician retention and state the denominator. (1)

---

#### Task 8 — Price–volume (2 marks)

For CardioX, decompose the April-to-May revenue change into price, volume, and interaction. Show the three effects and the net change.

---

#### Task 9 — Five-bullet readout (2 marks)

Write five bullets:

1. Headline KPI.
2. Region insight.
3. Product insight.
4. Trend insight.
5. Action or caveat.

Each bullet must contain a number and a business implication. Do not write more than five bullets.

---

**End of assessment.** The answer key is in 24.26. Do not read it until you have finished or the timer has ended.

---

### 24.26 Assessment answer key

This is a model solution, not the only correct one. Marks are for the method and the explanation, not for matching every word.

#### 24.26.1 Task 1 — Load and QA (4 marks)

```python
from pathlib import Path
import pandas as pd

DATA = Path("data/sql/csv")

sales = pd.read_csv(DATA / "sales.csv")
survey = pd.read_csv(DATA / "survey.csv", parse_dates=["response_date"])
items = pd.read_csv(
    DATA / "survey_items.csv",
    dtype={"score_raw": "string"},
    keep_default_na=False,
)
rx = pd.read_csv(DATA / "prescriptions.csv")

print("sales", sales.shape)       # (72, 9)
print("survey", survey.shape)     # (42, 8)
print("items", items.shape)       # (126, 3)
print("rx", rx.shape)             # (276, 6)

print("missing targets", sales["target_revenue"].isna().sum())  # 2
print("missing calls", sales["calls"].isna().sum())             # 3

print("duplicate physicians", survey.duplicated("physician_id").sum())  # 2
items["score_num"] = pd.to_numeric(items["score_raw"], errors="coerce")
invalid = items[~items["score_num"].isin([1, 2, 3, 4, 5])]
print("invalid items", len(invalid))  # 2
```

**Cleaning rules:**

- Survey: keep responses in the 2026-01-15 to 2026-03-31 field window; sort by `physician_id`, `response_id`; keep the first response per physician. Result: 39.
- Items: valid Likert codes are 1–5; `7` and `N/A` are invalid and excluded from item means/top-2-box. Join items to the clean 39 respondents.
- Sales: do not drop blank targets from revenue; exclude them only from achievement denominators. Do not fill blank calls with zero for duration analysis.

**Marking:** 1 for shapes, 1 for missing counts, 1 for duplicate/invalid counts, 1 for explicit rules. A answer that drops the 2 blank targets from revenue loses the interpretation mark.

#### 24.26.2 Task 2 — Commercial KPI (4 marks)

```python
revenue = sales["revenue"].sum()
target = sales["target_revenue"].sum()
achievement = 100 * revenue / target
units = sales["units"].sum()
rows = len(sales)
at_or_above = (sales["revenue"] >= sales["target_revenue"]).sum()
with_target = sales["target_revenue"].notna().sum()
```

| Metric | Value |
|---|---:|
| Revenue | ₹2,683,320 |
| Target | ₹2,717,035 |
| Achievement | 98.76% |
| Units | 14,121 |
| Rows | 72 |
| Rows at or above target | 28 |
| Rows with a target | 70 |

**Why sum/sum:** a row-level achievement average gives each small and large row equal weight. The business achievement is total revenue divided by total target. If you average row-level percentages, a ₹10,000 row has the same influence as a ₹70,000 row.

**Marking:** 1 for revenue/target/achievement, 1 for units/rows, 1 for the two counts, 1 for the denominator explanation.

#### 24.26.3 Task 3 — Region and product (4 marks)

```python
sales_rp = (
    sales.merge(pd.read_csv(DATA / "regions.csv"),
                on="region_id", how="left", validate="many_to_one")
         .merge(pd.read_csv(DATA / "products.csv"),
                on="product_id", how="left", validate="many_to_one")
)

region = (
    sales_rp.groupby("region_name", as_index=False)
            .agg(revenue=("revenue", "sum"),
                 target=("target_revenue", "sum"),
                 units=("units", "sum"))
)
region["achievement_pct"] = 100 * region["revenue"] / region["target"]
region["share_pct"] = 100 * region["revenue"] / region["revenue"].sum()
region = region.sort_values("share_pct", ascending=False)

product = (
    sales_rp.groupby("product_name", as_index=False)
            .agg(revenue=("revenue", "sum"),
                 target=("target_revenue", "sum"),
                 units=("units", "sum"))
)
product["achievement_pct"] = 100 * product["revenue"] / product["target"]
product["revenue_per_unit"] = product["revenue"] / product["units"]
product = product.sort_values("revenue", ascending=False)
```

| region | revenue | target | achievement | share |
|---|---:|---:|---:|---:|
| North | 864,954 | 886,608 | 97.56% | 32.23% |
| South | 724,284 | 747,844 | 96.85% | 26.99% |
| West | 612,882 | 622,128 | 98.51% | 22.84% |
| East | 481,200 | 460,455 | 104.51% | 17.93% |

| product | revenue | target | units | achievement | revenue/unit |
|---|---:|---:|---:|---:|---:|
| CardioX | 1,051,470 | 1,117,831 | 8,608 | 94.06% | 122.15 |
| OncoZ | 842,880 | 804,121 | 1,756 | 104.82% | 480.00 |
| NeuroY | 788,970 | 795,083 | 3,757 | 99.23% | 210.00 |

**Marking:** 2 for the region table, 2 for the product table. A missing `validate=` is not fatal here, but a missing denominator explanation is.

#### 24.26.4 Task 4 — Q2 matrix and concentration (3 marks)

```python
q2 = sales[sales["month"].between("2026-04-01", "2026-06-01")]
q2_rp = (
    q2.merge(pd.read_csv(DATA / "regions.csv"),
             on="region_id", how="left", validate="many_to_one")
      .merge(pd.read_csv(DATA / "products.csv"),
             on="product_id", how="left", validate="many_to_one")
)
q2_matrix = q2_rp.pivot_table(
    index="region_name", columns="product_name",
    values="revenue", aggfunc="sum", fill_value=0,
    margins=True, margins_name="Total",
)

cells = (
    sales_rp.groupby(["region_name", "product_name"], as_index=False)
            .agg(revenue=("revenue", "sum"))
            .sort_values("revenue", ascending=False)
)
cells["share_pct"] = 100 * cells["revenue"] / cells["revenue"].sum()
top3 = cells.head(3)
```

| region | CardioX | NeuroY | OncoZ | Total |
|---|---:|---:|---:|---:|
| East | 103,230 | 72,660 | 88,800 | 264,690 |
| North | 180,774 | 120,960 | 177,600 | 479,334 |
| South | 150,624 | 102,900 | 147,360 | 400,884 |
| West | 129,162 | 90,720 | 118,080 | 337,962 |
| Total | 563,790 | 387,240 | 531,840 | 1,482,870 |

Top three full-period cells: North–CardioX ₹337,134 (12.56%), North–OncoZ ₹281,280 (10.48%), South–CardioX ₹280,944 (10.47%). Combined: **33.52%**.

**Marking:** 2 for a correct pivot with margins; 1 for the top-three names and share. A pivot that sums to 1,482,870 but is labeled “annual” loses the interpretation mark.

#### 24.26.5 Task 5 — Survey cleaning (3 marks)

```python
window = survey[survey["response_date"].between("2026-01-15", "2026-03-31")]
clean_survey = (
    window.sort_values(["physician_id", "response_id"])
          .drop_duplicates("physician_id", keep="first")
)
print(len(survey), survey["physician_id"].nunique(), len(window), len(clean_survey))
# 42 40 41 39

print(clean_survey["awareness_stage"].value_counts())
```

| Stage | n |
|---|---:|
| Unaided awareness | 8 |
| Aided awareness | 8 |
| Consideration | 8 |
| Regular prescriber | 8 |
| Trial prescribed | 7 |

Trial prescribed is short of the quota of 8 by **1 respondent**. With n=7, one respondent is 14.3% of the cell.

**Marking:** 1 for the funnel, 1 for the stage counts, 1 for the shortfall and its size.

#### 24.26.6 Task 6 — Top-2-box and NPS (4 marks)

```python
valid_items = items[items["score_num"].isin([1, 2, 3, 4, 5])]
valid_items = valid_items.merge(
    clean_survey[["response_id"]],
    on="response_id", how="inner", validate="many_to_one",
)

top2 = (
    valid_items.groupby("item", as_index=False)
    .agg(valid_n=("score_num", "size"),
         top2_pct=("score_num", lambda s: 100 * s.isin([4, 5]).mean()),
         mean_score=("score_num", "mean"))
)
```

| item | valid_n | top2_pct | mean_score |
|---|---:|---:|---:|
| Access | 38 | 36.84 | 3.03 |
| Efficacy | 38 | 47.37 | 3.45 |
| Intent | 39 | 35.90 | 3.10 |

```python
promoters = int((clean_survey["recommend_score"] >= 9).sum())
passives = int(clean_survey["recommend_score"].between(7, 8).sum())
detractors = int((clean_survey["recommend_score"] <= 6).sum())
nps = 100 * (promoters - detractors) / len(clean_survey)
```

| Promoters | Passives | Detractors | NPS |
|---:|---:|---:|---:|
| 12 | 12 | 15 | −7.69 |

```python
weighted_nps = 100 * (
    clean_survey.loc[clean_survey["recommend_score"] >= 9, "weight"].sum()
    - clean_survey.loc[clean_survey["recommend_score"] <= 6, "weight"].sum()
) / clean_survey["weight"].sum()

effective_n = clean_survey["weight"].sum() ** 2 / (clean_survey["weight"] ** 2).sum()
```

| Weighted NPS | Effective n |
|---:|---:|
| −6.81 | 37.78 |

**Marking:** 2 for top-2-box with valid n; 1 for NPS and its three components; 1 for weighted NPS/effective n. A top-2-box computed on all 42 raw responses has the wrong base.

#### 24.26.7 Task 7 — Rx trend and mix (4 marks)

```python
rx_monthly = (
    rx.groupby("month", as_index=False)
      .agg(trx=("trx", "sum"), new_rx=("new_rx", "sum"))
)
rx_monthly["mom_pct"] = rx_monthly["trx"].pct_change() * 100
rx_monthly["running_total"] = rx_monthly["trx"].cumsum()
rx_monthly["ma_3m"] = rx_monthly["trx"].rolling(3).mean()
```

| month | trx | new_rx | MoM % | running | MA3 |
|---|---:|---:|---:|---:|---:|
| 2026-01 | 208 | 66 | — | 208 | — |
| 2026-02 | 241 | 72 | 15.87 | 449 | — |
| 2026-03 | 273 | 75 | 13.28 | 722 | 240.67 |
| 2026-04 | 306 | 87 | 12.09 | 1,028 | 273.33 |
| 2026-05 | 327 | 90 | 6.86 | 1,355 | 302.00 |
| 2026-06 | 344 | 96 | 5.20 | 1,699 | 325.67 |

```python
june = rx[rx["month"] == "2026-06-01"]
june_product = (
    june.merge(pd.read_csv(DATA / "products.csv"),
               on="product_id", how="left", validate="many_to_one")
        .groupby("product_name", as_index=False)
        .agg(trx=("trx", "sum"))
)
june_product["share_pct"] = 100 * june_product["trx"] / june_product["trx"].sum()
```

| product | June TRx | share |
|---|---:|---:|
| OncoZ | 124 | 36.05% |
| CardioX | 115 | 33.43% |
| NeuroY | 105 | 30.52% |

```python
active = rx[["physician_id", "month"]].drop_duplicates()
jan = set(active.loc[active["month"] == "2026-01-01", "physician_id"])
jun = set(active.loc[active["month"] == "2026-06-01", "physician_id"])
retention = 100 * len(jan & jun) / len(jan)  # 87.5%
```

Retention: 14 of the 16 January prescribers = **87.5%**. P-041 and P-042 are the lost physicians.

**Marking:** 2 for the trend table, 1 for June mix, 1 for retention with denominator.

#### 24.26.8 Task 8 — Price–volume (2 marks)

```python
card = sales_rp[
    (sales_rp["product_name"] == "CardioX")
    & (sales_rp["month"].isin(["2026-04-01", "2026-05-01"]))
]
card_pv = card.groupby("month", as_index=False).agg(
    units=("units", "sum"), revenue=("revenue", "sum")
)
card_pv["price"] = card_pv["revenue"] / card_pv["units"]
```

| month | units | revenue | price |
|---|---:|---:|---:|
| April | 1,459 | 175,080 | 120 |
| May | 1,445 | 182,070 | 126 |

- Price: `(126 − 120) × 1,459 = +8,754`
- Volume: `(1,445 − 1,459) × 120 = −1,680`
- Interaction: `(126 − 120) × (1,445 − 1,459) = −84`
- Net: **+6,990**

**Marking:** 1 for the three effects; 1 for the net and a one-line interpretation. Calling the volume change “elasticity” from one month loses the interpretation mark.

#### 24.26.9 Task 9 — Five-bullet readout (2 marks)

Model answer:

- **Headline:** Revenue is ₹2.68m against a ₹2.72m target (98.76%), with Q2 growth of 23.53% but Q2 achievement of 98.25%.
- **Region:** North is 32.23% of revenue but only 97.56% achievement; East is 17.93% of revenue but 104.51% achievement.
- **Product:** CardioX is 39.19% of revenue but 94.06% achievement; OncoZ is 31.41% of revenue and 104.82% achievement.
- **Trend:** TRx grew from 208 to 344, but MoM growth slowed from 15.87% to 5.20%; June new Rx share is 27.91%.
- **Action:** Protect OncoZ and East, diagnose CardioX and North, and test whether the slowing TRx growth is access, coverage, or base effect before changing the plan.

**Marking:** 1 for five bullets with numbers; 1 for at least two business implications and one caveat. A list of six bullets loses the format mark.

#### 24.26.10 Marking summary

| Task | Marks | Must-have |
|---|---:|---|
| 1 Load/QA | 4 | shapes, missing, duplicates/invalid, rules |
| 2 KPI | 4 | revenue/target/achievement, counts, sum/sum explanation |
| 3 Region/product | 4 | both tables, shares, units/price |
| 4 Q2 matrix | 3 | pivot with margins, top-3 share |
| 5 Survey clean | 3 | 42/40/41/39, stages, trial shortfall |
| 6 Top-2/NPS | 4 | valid n, top2, NPS, weighted/effective n |
| 7 Rx trend | 4 | monthly table, June mix, retention |
| 8 Price–volume | 2 | three effects + net |
| 9 Readout | 2 | five bullets, numbers, implications |
| **Total** | **30** |  |

**Score guide:**

| Score | Interpretation |
|---:|---|
| 27–30 | Interview-ready; focus on speed and presentation |
| 23–26 | Strong; fix the specific calculation or explanation gap |
| 18–22 | Passable; redo the weak task and one drill |
| 13–17 | Repeat the applied sections before another timed attempt |
| 0–12 | Rebuild the pandas foundations; do not attempt a full mock yet |

---

### 24.27 Common mistakes and final checklist

#### 24.27.1 Fifteen mistakes that cost marks

| Mistake | Why it is wrong | Fix |
|---|---|---|
| Averaging row-level achievement | Small and large rows get equal weight | `sum(revenue) / sum(target)` |
| Filling blank targets with zero | Creates false overachievement | Exclude from achievement; show n/a |
| Dropping blank targets from revenue | Understates revenue | Keep revenue; exclude only from target |
| Counting `len()` after a one-to-many join | Counts item rows, not respondents | Count unique `response_id` or use the base |
| Dropping duplicate respondents before the window | Wrong exclusion order | Window first, then dedupe; document |
| Treating `N/A` as a score | Skews the mean | `pd.to_numeric(errors="coerce")`; valid 1–5 |
| Computing top-2-box on all 42 raw responses | Includes duplicates/out-of-window | Join to the clean 39 |
| Using `mean` as the default pivot aggfunc | Revenue default is mean, not sum | `aggfunc="sum"` |
| Forgetting `validate=` on a merge | Silent fan-out | `validate="many_to_one"` |
| Using `.iloc` for a label filter | Positional, not label | `.loc` with a boolean mask |
| Filling `shift(1)` NaN with zero | Invents a prior period | Leave NaN or state the rule |
| Reporting NPS without passives in the denominator | Wrong NPS | All respondents in denominator |
| Saying “significant” without a test | Overclaims | “Directional” or run a test |
| Presenting a notebook screenshot | Not reproducible | Export CSV/Excel + QA log |
| Automating an undefined process | Scales a mistake | Define and validate first |

#### 24.27.2 The final 10-minute checklist

- [ ] Does the script run top-to-bottom in a fresh kernel?
- [ ] Are all dates parsed?
- [ ] Are all joins validated?
- [ ] Are missing-value rules explicit?
- [ ] Is every denominator stated?
- [ ] Do the headline totals reconcile?
- [ ] Does the survey base match the stated n?
- [ ] Are the top-2-box and NPS bases different, and labeled?
- [ ] Is the pivot summing to the total?
- [ ] Is there a named output file?
- [ ] Is there a QA log?
- [ ] Is there one caveat in the readout?

#### 24.27.3 What to do if you have five minutes left

1. Run the script end-to-end.
2. Check the headline total against a simple `sum`.
3. Check the survey n against the stated cleaning rule.
4. Add one assertion.
5. Write the headline bullet and one caveat.

Do not start a new analysis. Finish and explain what you have.

#### 24.27.4 The one-sentence standard

> “A good Python analysis is reproducible, validated, and explainable: the reader can see the grain, the exclusions, the denominator, and the check that would have caught the error.”

---

### Module 24 completion checklist

- [ ] I can load a CSV and inspect shape, dtypes, and missingness.
- [ ] I can filter, select, sort, and create a column without chained assignment.
- [ ] I can clean text, dates, and invalid codes.
- [ ] I can `groupby`, `agg`, `transform`, and `rank`.
- [ ] I can merge with `validate=` and audit an anti-join.
- [ ] I can pivot, crosstab, and melt.
- [ ] I can compute MoM, running total, and moving average.
- [ ] I can clean a survey base and compute top-2-box and NPS.
- [ ] I can compute a weighted estimate and effective n.
- [ ] I can build a commercial KPI table and read it two ways.
- [ ] I can decompose price–volume.
- [ ] I can write assertions and export a report.
- [ ] I can explain every denominator in 60 seconds.
- [ ] I scored at least 23/30 on the 45-minute assessment.

If you cannot tick an item, return to the section and redo the drill. Do not move to module 25 with a shaky foundation.

---

*End of Module 24. All data is fictional and for interview practice only.*
