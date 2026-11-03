# 22 — Advanced Excel for Analyst Assessments

**Purpose:** move you from “I know VLOOKUP and PivotTables” to “I can clean a messy extract, reconcile two sources, build a defensible analysis, and explain it in a timed assessment.” This module is deliberately *application-first*: every function is attached to a pharma commercial or physician-survey decision.

**Prerequisites:** [04-excel-formulas-cheatsheet.md](04-excel-formulas-cheatsheet.md), [05-excel-practical-workbook-guide.md](05-excel-practical-workbook-guide.md), and [15-mock-excel-assessment-45-min.md](15-mock-excel-assessment-45-min.md). You should already be comfortable with SUMIFS, basic VLOOKUP, IF, and creating a PivotTable.

**Suggested study time:** 10–12 hours across three sessions, all with Excel open. Do not read this like an article. Type every formula, break it on purpose, and inspect the result.

**Data files (all fictional):**

| File | Rows | Grain | What it is for |
|---|---:|---|---|
| `data/excel_advanced_sales.csv` | 72 | region × product × month | Lookups, SUMIFS, PivotTables, price–volume, what-if |
| `data/excel_advanced_survey.csv` | 42 raw | physician respondent | Cleaning, cross-tabs, weighting, NPS, survey quality |
| `data/excel_recon_ledger.csv` | 20 | invoice | Reconciliation source A |
| `data/excel_recon_finance.csv` | 20 | invoice | Reconciliation source B, with deliberate exceptions |
| `data/excel_assessment_raw.csv` | 37 raw | territory × product × month | The 45-minute assessment |
| `data/excel_product_master.csv` | 3 | product | Lookup dimension |
| `data/excel_region_master.csv` | 4 | region | Lookup dimension |

> **Setup before you start.** Import each CSV with **Data → From Text/CSV**. Convert each range to a table (`Ctrl+T`) and name them `tblSales`, `tblProduct`, `tblRegion`, `tblSurvey`, `tblLedger`, and `tblFinance`. The formulas below use structured references; if you prefer A1-style ranges, substitute them. Keep the raw import untouched on a `Raw` sheet and do the work on a `Clean` copy. For the banding example in 22.1.2, create a small table named `tblBand`.

> **Source labels.** Everything in this file is **PRACTICE INFERENCE** unless it cites a document already labelled in the pack. The datasets are **fictional** and were generated for practice. They are not Trinity data, not real company data, and not indicative of real market size. The 45-minute format is a reasonable simulation of the confirmed 30–45 minute Excel assessment window (CONFIRMED duration, TGaS Campus JD); the content is not a leaked or official assessment.

> **Data scale.** Revenue figures are in ₹ and are intentionally small enough for mental arithmetic. Do not infer real pharma market size from them. The analytical logic — denominators, mix, targets, reconciliation — is the point.

---

## 22.0 What this module adds (and what it deliberately does not repeat)

| Topic | Already covered in 04/05 | What is new here |
|---|---|---|
| Lookups | VLOOKUP/XLOOKUP/INDEX-MATCH syntax, basic examples | Multi-criteria joins, two-way lookups, duplicate keys, latest-effective-date lookups, dynamic columns, failure auditing, performance |
| SUMIFS/COUNTIFS | Basic single-condition and two-condition examples | Date windows, OR logic, wildcard and blank criteria, array criteria, weighted averages, share denominators, distinct counts, top-N |
| Error handling | IFERROR basics | IFNA vs IFERROR, error taxonomy, error budgets, fallback chains, visible QA flags |
| Dynamic arrays | Mentioned only | FILTER/SORT/SORTBY/UNIQUE/SEQUENCE, spill ranges, live exception lists, compatibility fallbacks |
| Text/dates | TEXT, CONCAT, basic date formats | Parsing codes, normalising dirty keys, cohort months, fiscal quarters, field windows, date traps |
| PivotTables | Basic build and grouping | Calculated fields vs calculated items, show-values-as, running totals, GETPIVOTDATA, top-N, slicers, data model |
| Conditional formatting | Basic highlighting | Formula-based row rules, duplicates, blanks, outliers, rule order |
| Data validation | Basic lists | Dependent dropdowns, custom formulas, circle-invalid, why validation is not a QA substitute |
| Reconciliation | Basic checks | Two-way reconciliation, tolerance policy, exception classification, variance bridge, survey quota reconciliation |
| What-if | Not covered | Goal Seek, one- and two-variable data tables, Scenario Manager, sensitivity, break-even, tornado |
| Survey analysis | Statistical concepts in 06 | Applied workflow: straight-liners, invalid codes, weighting, top-2-box, NPS, cross-tabs, small-base caution |
| Reporting | Charts and slide basics in 05/10 | Raw-data-to-one-page-memo workflow, insight titles, assumption log, audit trail |

**How to work through it.** For each section: read the intuition → reproduce the worked example on the CSV → do the “Check yourself” questions without looking at 22.13 → then attempt the timed drills in 22.12. The 45-minute assessment in 22.14 is the final gate. Answers are deliberately separated.

---

## Part A — Lookup engineering

### 22.1 A lookup is a join, not a magic function

Every lookup answers one question: **given a key, return an attribute from another table.** The function is the easy part. The analyst work is:

1. **Is the key unique on the lookup side?** If not, which row should win?
2. **Is the key formatted identically on both sides?** Trailing spaces, case, leading zeros and text-vs-number all break joins silently.
3. **What should happen when there is no match?** A blank, a zero, a visible flag, or a hard stop?
4. **Is the lookup returning one value or an aggregation?** XLOOKUP returns a row; SUMIFS returns a sum. Confusing the two is a classic assessment error.

A useful mental model:

```
source table                         lookup table
key | attribute        +             key | attribute 1 | attribute 2
```

The join is reliable only when `key` is clean and unique. Everything else is error handling.

#### 22.1.1 XLOOKUP: the six arguments that matter

```excel
=XLOOKUP(lookup_value, lookup_array, return_array,
         [if_not_found], [match_mode], [search_mode])
```

| Argument | What it controls | Analyst advice |
|---|---|---|
| `lookup_value` | The key you are searching for | Clean it first: `TRIM`, `UPPER`, or a constructed key |
| `lookup_array` | The column/range that must contain the key | Prefer structured table references; avoid whole-column references in large models |
| `return_array` | The column(s) you want back | Can be multiple columns; the result spills |
| `if_not_found` | What to return when there is no match | Use a visible flag such as `"CHECK: unmapped"`, never a silent `0` |
| `match_mode` | `0` exact (default), `-1` exact or next smaller, `1` exact or next larger, `2` wildcard | Use `-1` only on a sorted ascending table; use `2` sparingly |
| `search_mode` | `1` first-to-last (default), `-1` last-to-first, `2` binary ascending, `-2` binary descending | `-1` is useful when duplicate keys exist and the latest row wins |

**Worked example 1 — simple attribute lookup.**

Using `excel_product_master.csv`:

```excel
=XLOOKUP("OncoZ", tblProduct[Product], tblProduct[TherapeuticArea])
```

Result: `Oncology`.

```excel
=XLOOKUP("OnoZ", tblProduct[Product], tblProduct[TherapeuticArea], "CHECK: unmapped")
```

Result: `CHECK: unmapped`. This is better than `#N/A` in a client-facing workbook because the reader sees that a check is needed rather than a broken cell.

**Worked example 2 — return multiple attributes at once.**

```excel
=XLOOKUP("CardioX", tblProduct[Product],
         tblProduct[[TherapeuticArea]:[LaunchYear]])
```

Result spills two cells: `Cardiovascular` and `2018`. This is the cleanest way to enrich a sales extract with a small dimension table.

**Worked example 3 — region manager.**

```excel
=XLOOKUP("North", tblRegion[Region], tblRegion[RegionalManager])
```

Result: `Meera Kapoor`. The same pattern maps `Region → HQ`, `Product → LaunchYear`, or `RespondentID → Weight`.

**Worked example 4 — a two-key lookup that is genuinely unique.**

Suppose you build a key column in the sales table:

```excel
=Region&"|"&Product&"|"&TEXT(Date,"mmm-yy")
```

Then:

```excel
=XLOOKUP("North|CardioX|May-26", tblSales[Key], tblSales[Revenue])
```

Result: `58,338`.

This works because the key combination identifies exactly one row. If you drop the month and use `"North|CardioX"`, the key is no longer unique — XLOOKUP returns the **first** matching row, not the sum. That is the trap in the next example.

**Worked example 5 — lookup is not aggregation.**

```excel
=XLOOKUP("North|CardioX", tblSales[Key], tblSales[Revenue])
```

This returns the January value (₹50,400), not the Q2 total (₹180,774). The correct question — “what was North CardioX revenue in Q2?” — is a SUMIFS question:

```excel
=SUMIFS(tblSales[Revenue],
        tblSales[Region],"North",
        tblSales[Product],"CardioX",
        tblSales[Date],">="&DATE(2026,4,1),
        tblSales[Date],"<="&DATE(2026,6,30))
```

Result: `180,774`.

> **Assessment lens.** Interviewers deliberately include a “lookup vs sum” trap. If the question says *total*, *share*, *average*, or *how many*, you almost certainly need an aggregation function, not a lookup. If it says *what is the value for this exact record*, a lookup is appropriate.

**Worked example 6 — latest effective price (approximate match on a date).**

A price history table:

| EffectiveDate | Product | Price |
|---|---|---:|
| 2026-01-01 | CardioX | 120 |
| 2026-05-01 | CardioX | 126 |

To find the price in force on 15 May 2026:

```excel
=XLOOKUP(DATE(2026,5,15), tblPrice[EffectiveDate], tblPrice[Price], "CHECK", -1)
```

Result: `126`.

Why `-1`? It means “exact match, or the next smaller value if there is no exact match.” The table must be sorted ascending by date. This is the same logic as a VLOOKUP with `TRUE`, but safer because you can see the match mode explicitly. Do **not** use `search_mode -1` as a substitute for `match_mode -1`; they control different things.

**Worked example 7 — wildcard lookup (use with care).**

```excel
=XLOOKUP("Cardio*", tblProduct[Product], tblProduct[TherapeuticArea], "CHECK", 2)
```

Result: `Cardiovascular`. Wildcards are useful for messy category names, but they can match more than you expect. In an assessment, exact matches plus a cleaning step are almost always safer.

#### 22.1.2 VLOOKUP when you are forced to use it

VLOOKUP is not obsolete, but it is brittle. The three advanced patterns worth knowing:

**A. Dynamic column selection with MATCH.**

If a master table’s columns may move, do not hard-code `3`:

```excel
=VLOOKUP($A2, tblProduct,
         MATCH("LaunchYear", tblProduct[#Headers], 0), FALSE)
```

`MATCH` finds the column number at runtime. If someone inserts a column, the formula still works. This is the single most useful VLOOKUP upgrade.

**B. Approximate-match banding.**

Create a sorted band table:

| LowerBound | Band |
|---:|---|
| 0% | Below target |
| 90% | On track |
| 100% | At or above target |
| 110% | Exceeds |

Then:

```excel
=VLOOKUP(0.856, tblBand, 2, TRUE)
```

Result: `Below target`. For `1.1298`, the result is `Exceeds`. The table **must** be sorted ascending on the first column, and the first column must contain the lower bound of each band.

**C. Two-key lookup with a helper column.**

```excel
=VLOOKUP($A2&"|"&$B2, tblSales[[Key]:[Revenue]], 2, FALSE)
```

This is functionally the same as XLOOKUP but requires the key to be the first column of the lookup range. In modern Excel, prefer XLOOKUP; in an older-version assessment, this helper-key pattern is the reliable fallback.

**Why VLOOKUP fails silently:** omitting `FALSE` makes Excel use approximate match. If the table is unsorted, you get a plausible but wrong answer. Always type `FALSE` (or `0`) unless you are deliberately banding.

#### 22.1.3 INDEX-MATCH: the two-way and multi-criteria workhorse

**A. Two-way lookup.**

Suppose a matrix has region-product keys down the rows and months across the columns:

```excel
=INDEX(tblMatrix, MATCH("North|CardioX", tblMatrix[Key], 0),
       MATCH("May-26", tblMatrix[#Headers], 0))
```

Result: `58,338`. This is the cleanest way to pull a value from a cross-tab without hard-coding a cell address.

**B. Multiple criteria without a helper column.**

```excel
=INDEX(tblSales[Revenue],
       MATCH(1,
             (tblSales[Region]="North")*
             (tblSales[Product]="CardioX")*
             (TEXT(tblSales[Date],"mmm-yy")="May-26"),
             0))
```

Result: `58,338`. In Microsoft 365 this works as a normal formula; in older Excel it may need `Ctrl+Shift+Enter`. The multiplication acts as AND: each condition returns TRUE/FALSE, which becomes 1/0.

**C. XMATCH when you only need the position.**

```excel
=XMATCH("May-26", tblMatrix[#Headers], 0, 1)
```

Returns the column position. Useful for building dynamic ranges or feeding `INDEX`.

#### 22.1.4 Lookup failure engineering (the part most candidates skip)

A lookup that returns `#N/A` is not “finished.” The analyst’s job is to make failures **visible, counted, and explained**.

**Step 1 — normalise the key before joining.**

```excel
=TRIM(CLEAN(A2))
```

For product names, also consider `PROPER` or `UPPER` if case is inconsistent. For IDs, preserve leading zeros by storing both sides as text.

**Step 2 — use IFNA, not IFERROR, for joins.**

```excel
=IFNA(XLOOKUP(TRIM($D2), tblProduct[Product],
              tblProduct[TherapeuticArea], "CHECK: unmapped"),
      "CHECK: unmapped")
```

`IFNA` only traps “no match.” `IFERROR` traps everything, including a broken formula, a wrong-range error, or a division by zero. Use `IFERROR` only when you genuinely want to suppress all error types.

**Step 3 — count the failures.**

```excel
=COUNTIF(tblEnriched[TherapeuticArea],"CHECK: unmapped")
```

In a clean extract this should be `0`. If it is not zero, list the failing keys:

```excel
=FILTER(tblEnriched[Product],
        tblEnriched[TherapeuticArea]="CHECK: unmapped")
```

**Step 4 — fix the source, not the output.** A trailing space in `"OncoZ "` is a data-entry problem. `TRIM` fixes the symptom; data validation and a controlled product list fix the cause.

> **So what does this mean for the business?** An unmapped product is not a cosmetic error. It drops out of the therapeutic-area totals, so the commercial team understates Oncology revenue and may reallocate budget incorrectly. The lookup-failure count belongs in the data-quality section of every report.

#### 22.1.5 Lookup decision table

| Situation | Best tool | Why |
|---|---|---|
| One key, one attribute, modern Excel | XLOOKUP | Clear arguments, built-in not-found handling, no column counting |
| One key, one attribute, older Excel | INDEX-MATCH | Robust to column insertion |
| One key, several attributes | XLOOKUP with multi-column return | Spills the attributes together |
| Two keys, unique combination | Helper key + XLOOKUP | Simple and auditable |
| Two keys, no helper allowed | INDEX/MATCH with Boolean array | Works but harder to debug |
| Approximate banding | XLOOKUP `match_mode -1` or VLOOKUP `TRUE` | Requires sorted ascending table |
| Total/average/count by criteria | SUMIFS/AVERAGEIFS/COUNTIFS | A lookup returns a row, not an aggregate |
| Latest record by date | XLOOKUP `match_mode -1` on a sorted date table | Clear “as of” logic |
| Duplicate keys, first/last matters | XLOOKUP `search_mode 1` or `-1` | Makes the choice explicit |

#### Check yourself — 22.1

1. Return `LaunchYear` for `NeuroY` using XLOOKUP and using INDEX-MATCH. What is the answer?
2. What does `=XLOOKUP("OnoZ", Products, Areas, "CHECK")` return, and why is that better than `#N/A`?
3. Why is `=XLOOKUP("North|CardioX", Keys, Revenue)` not the right formula for Q2 revenue?
4. Write a two-way formula that returns North CardioX May-26 revenue. What is the value?
5. In `excel_assessment_raw.csv`, how many product lookup failures exist before and after `TRIM`?

*Answers in 22.13.*

---

## Part B — Multi-condition aggregation at analyst level

### 22.2 SUMIFS and COUNTIFS: the engine of commercial analysis

The syntax is simple:

```excel
=SUMIFS(sum_range, criteria_range1, criteria1, criteria_range2, criteria2, ...)
=COUNTIFS(criteria_range1, criteria1, criteria_range2, criteria2, ...)
```

The difficulty is not the syntax. It is choosing the right **grain**, **denominator**, and **date window**. Most wrong answers in Excel assessments are not formula errors; they are denominator errors.

#### 22.2.1 Date windows

Q2 2026 revenue:

```excel
=SUMIFS(tblSales[Revenue],
        tblSales[Date],">="&DATE(2026,4,1),
        tblSales[Date],"<="&DATE(2026,6,30))
```

Result: `1,482,870`.

A more robust month-end pattern:

```excel
=SUMIFS(tblSales[Revenue],
        tblSales[Date],">="&DATE(2026,4,1),
        tblSales[Date],"<="&EOMONTH(DATE(2026,6,1),0))
```

Why `EOMONTH`? It avoids hard-coding `30` and survives month-length changes. For a half-open interval (recommended for timestamps), use `>= start` and `< day after end`.

**Q1 vs Q2 growth:**

```excel
=SUMIFS(rev, date, ">="&DATE(2026,4,1), date, "<="&DATE(2026,6,30))
 / SUMIFS(rev, date, ">="&DATE(2026,1,1), date, "<="&DATE(2026,3,31)) - 1
```

Result: `23.53%`. The business reads this as “Q2 grew 23.5% over Q1.” Always state the comparison basis: quarter-over-quarter, not year-over-year. There is no 2025 data in this dataset, so a YoY claim would be fabricated.

#### 22.2.2 OR logic

SUMIFS treats multiple criteria as AND. For OR, add separate SUMIFS:

```excel
=SUMIFS(rev, region,"North") + SUMIFS(rev, region,"South")
```

Result: `1,589,238`.

Or use an array constant:

```excel
=SUM(SUMIFS(rev, region, {"North","South"}))
```

Result: `1,589,238`. The array version is compact but can confuse readers; the two-term version is easier to audit.

For more complex OR/AND combinations, `SUMPRODUCT` with Boolean arrays is flexible:

```excel
=SUMPRODUCT((region="North")+(region="South"), rev)
```

Be careful: `+` is OR but can double-count if a row satisfies both conditions. For mutually exclusive regions it is safe; for overlapping categories it is not.

#### 22.2.3 Criteria with cells, wildcards, blanks, and “not”

| Task | Criteria |
|---|---|
| Product starts with “Cardio” | `"Cardio*"` |
| Product is exactly “CardioX” | `"CardioX"` |
| Region is not North | `"<>North"` |
| Calls cell is blank | `""` |
| Calls cell is not blank | `"<>"` |
| Date on or after 1 Apr 2026 | `">="&DATE(2026,4,1)` |
| Value equals a cell | `">="&$B$2` |
| Literal asterisk in text | `"~*"` |

Example: CardioX revenue:

```excel
=SUMIFS(rev, product, "Cardio*")
```

Result: `1,051,470`.

Count blank calls:

```excel
=COUNTIFS(calls, "")
```

Result: `3`. This is a data-quality metric, not a performance metric: three region-product-month rows did not report calls.

#### 22.2.4 Weighted averages (the most common “average” trap)

Suppose you want the average price of CardioX. The naive formula is:

```excel
=AVERAGE(tblSales[UnitPrice])   → 122.00
```

But that gives every row equal weight, even though rows have very different unit volumes. The correct volume-weighted price is:

```excel
=SUMIFS(tblSales[Revenue], tblSales[Product],"CardioX")
 / SUMIFS(tblSales[Units], tblSales[Product],"CardioX")
```

Result: `122.15`.

For Q2 only:

```excel
=563,790 / 4,544 = 124.07
```

Why the difference? April was priced at ₹120; May and June at ₹126. Q2 unit mix is heavier in the higher-priced months, so the weighted price is above the simple average. If you report “average price” without saying weighted or unweighted, you have not defined the metric.

> **So what does this mean for the business?** A price increase can raise revenue even if volume falls. In April→May, CardioX units fell from 1,459 to 1,445, but revenue rose from ₹175,080 to ₹182,070 because price rose from ₹120 to ₹126. A simple “units are down” headline would miss the revenue story. See 22.9 for the full price–volume decomposition.

#### 22.2.5 Shares and denominators

Share questions are denominator questions. Three common variants:

1. **Share of total:** `product revenue / total revenue`.
2. **Share of segment:** `North CardioX / North total`.
3. **Share of target:** `revenue / target` (achievement, not share).

Examples:

- OncoZ share of Q2 revenue: `531,840 / 1,482,870 = 35.87%`.
- CardioX share of Q2: `563,790 / 1,482,870 = 38.02%`.
- NeuroY share of Q2: `387,240 / 1,482,870 = 26.11%`.
- North CardioX share of North: `337,134 / 864,954 = 38.98%`.
- North CardioX share of total: `337,134 / 2,683,320 = 12.56%`.

All three are correct answers to different questions. In an assessment, write the denominator explicitly next to the number, e.g. “35.9% of Q2 revenue (denominator: Q2 total, ₹14.83 lakh).”

#### 22.2.6 Achievement and row-level counts

Overall achievement:

```excel
=SUM(Revenue) / SUM(TargetRevenue)
=2,683,320 / 2,717,035 = 98.76%
```

But “70% of rows missed target” is a different metric from “98.8% of revenue achieved.” Both can be true: a few large rows can carry the total while many small rows miss.

To count rows at or above target, use a helper column:

```excel
=Revenue - TargetRevenue
```

Then:

```excel
=COUNTIFS(tblSales[Variance],">=0",tblSales[TargetRevenue],"<>")
```

Result: `28` of `70` rows with a target = `40.0%`.

Why not `=COUNTIFS(Revenue,">="&TargetRevenue)`? COUNTIFS compares each range to a scalar criterion; it does not compare two ranges row by row. A helper column (or SUMPRODUCT) makes the row-level comparison explicit:

```excel
=SUMPRODUCT((TargetRevenue<>"")*(Revenue>=TargetRevenue))
```

Result: `28`. The `TargetRevenue<>""` guard prevents blank targets from being treated as zero.

By region:

| Region | Rows at/above target | Rows with target | Hit rate |
|---|---:|---:|---:|
| North | 5 | 18 | 27.8% |
| South | 8 | 18 | 44.4% |
| East | 10 | 17 | 58.8% |
| West | 5 | 17 | 29.4% |
| **Total** | **28** | **70** | **40.0%** |

Note the apparent contradiction: East has the highest hit rate (58.8%) and the highest achievement (104.51%), while North has the largest revenue but a lower hit rate (27.8%) and 97.56% achievement. North carries large rows that are close to target; East has smaller rows that beat easier targets. This is exactly the kind of “segment-level comparison” an interviewer wants you to notice.

#### 22.2.7 Distinct counts

```excel
=SUMPRODUCT(1/COUNTIF(tblSales[Product],tblSales[Product]))
```

Result: `3` products. In Microsoft 365:

```excel
=COUNTA(UNIQUE(tblSales[Product]))
```

Result: `3`.

Distinct count is useful for “how many physicians prescribed?”, “how many territories are active?”, and “how many products are in the launch portfolio?”. The `SUMPRODUCT(1/COUNTIF(...))` version breaks if the range contains blanks (division by zero); the `UNIQUE` version handles blanks differently, so filter them first.

#### 22.2.8 Top-N and Pareto

Build a 12-row summary (region × product), then:

```excel
=SUM(LARGE(cell_revenue, {1,2,3})) / SUM(cell_revenue)
```

Result: `33.52%`. The top three region-product cells — North CardioX (₹337,134), North OncoZ (₹281,280), South CardioX (₹280,944) — generate one-third of revenue. This is a legitimate Pareto observation, but do not overclaim “80/20” unless you have actually computed the cumulative curve.

#### Check yourself — 22.2

1. What is Q2 revenue, and what is Q1→Q2 growth?
2. What is North + South revenue?
3. What is CardioX Q2 revenue and its share of Q2?
4. Weighted average CardioX price overall vs simple average?
5. How many calls cells are blank?
6. How many rows are at/above target, and what percentage is that?
7. How many distinct products and regions are in the sales file?
8. What share of total revenue comes from the top three region-product cells?

*Answers in 22.13.*

---

## Part C — Error handling as a control

### 22.3 Errors are information, not embarrassment

An Excel error is a signal. The question is whether you handle it visibly or hide it.

#### 22.3.1 Error taxonomy

| Error | Typical cause | Analyst response |
|---|---|---|
| `#N/A` | Lookup key not found | Clean key; use IFNA with a visible flag; count failures |
| `#VALUE!` | Text where a number is required | Check data type; convert text numbers; trace the cell |
| `#DIV/0!` | Denominator is zero or blank | Guard with `IF(den=0,"",...)`; investigate why denominator is zero |
| `#REF!` | Deleted row/column or invalid reference | Undo; repair formula; check named ranges |
| `#NAME?` | Misspelled function or undefined name | Fix spelling; define the name; check add-ins |
| `#NUM!` | Invalid numeric operation (e.g., negative square root) | Check inputs and assumptions |
| `#SPILL!` | Spill range is blocked | Clear the spill range or move the formula |
| `#CALC!` | Dynamic array calculation issue (e.g., empty array) | Check `if_empty`; inspect the array |

#### 22.3.2 IFERROR vs IFNA

```excel
=IFERROR(A2/B2, "")          ' hides every error type
=IFNA(XLOOKUP(...), "CHECK") ' hides only “not found”
```

Use `IFNA` for joins. Use `IFERROR` only when you can name every error you expect and you are comfortable hiding the rest. In a client deliverable, a silent blank is worse than a visible `CHECK` because it conceals a data problem.

**Bad pattern:**

```excel
=IFERROR(VLOOKUP(A2, Products, 2, FALSE), 0)
```

This returns `0` for an unmapped product. Downstream, the product looks like it had zero revenue, and the error disappears.

**Better pattern:**

```excel
=IFNA(VLOOKUP(A2, Products, 2, FALSE), "CHECK: unmapped")
```

Then count the flags:

```excel
=COUNTIF(tblEnriched[TherapeuticArea], "CHECK: unmapped")
```

#### 22.3.3 Error budget

An **error budget** is a small QA block at the top of a workbook:

| Check | Formula | Expected | Actual | Status |
|---|---|---|---:|---|
| Unmapped products | `COUNTIF(...,"CHECK*")` | 0 | 0 | PASS |
| Blank targets | `COUNTIF(Target,"")` | 2 (known) | 2 | EXPLAINED |
| Blank calls | `COUNTIF(Calls,"")` | 3 (known) | 3 | EXPLAINED |
| Revenue ≠ units × price | `SUMPRODUCT(--(ABS(Rev-Units*Price)>1))` | 0 | 0 | PASS |
| Duplicate keys | `SUMPRODUCT(--(COUNTIF(Key,Key)>1))` | 0 | 0 | PASS |

The point is not to achieve all zeros. The point is to **know** what is non-zero and to explain it. In an assessment, a visible QA block signals maturity.

#### 22.3.4 Division guards

```excel
=IF(B2=0,"",A2/B2)
```

This returns a blank when the denominator is zero. If you want a visible flag:

```excel
=IF(B2=0,"CHECK: zero denominator",A2/B2)
```

For achievement with a blank target:

```excel
=IF([@TargetRevenue]="","",[@Revenue]/[@TargetRevenue])
```

Result for the two OncoZ launch rows with no target: blank, not `#DIV/0!`. Then the achievement average excludes them deliberately.

> **So what does this mean for the business?** A blank target is not the same as a target of zero. If you treat it as zero, achievement becomes infinite or undefined; if you treat the row as zero revenue, you understate the launch. The correct treatment is to exclude the row from achievement, report the exclusion, and follow up with finance.

#### 22.3.5 Fallback chains

When a lookup must never fail silently, chain fallbacks:

```excel
=IFNA(XLOOKUP(A2, NewMaster[Code], NewMaster[Name]),
      IFNA(XLOOKUP(A2, LegacyMaster[Code], LegacyMaster[Name]),
           "CHECK: not in any master"))
```

This is useful during a master-data migration. It is also a clear interview talking point: you understand that joins fail and you have a controlled fallback.

#### Check yourself — 22.3

1. When should you use IFNA instead of IFERROR?
2. Write an achievement formula that returns blank when the target is blank.
3. How many blank targets are in the sales file, and how should they be treated?
4. Why is `=IFERROR(1/0,0)` a bad habit?

*Answers in 22.13.*

---

## Part D — Dynamic arrays

### 22.4 One formula, many answers

A dynamic array formula returns a **range**, and Excel “spills” the result into neighbouring cells. The spill range is referenced with `#`. This is the biggest productivity upgrade in modern Excel, and it is increasingly common in analyst assessments because it tests whether you can build a live exception list instead of manually filtering.

Key concepts:

- **Spill range:** the block of cells populated by one formula, e.g. `F2#`.
- **`#` operator:** refers to the entire spill range, so downstream formulas update automatically.
- **`@` operator:** implicit intersection; usually a sign that a legacy formula is being coerced to a single value.
- **`#SPILL!`:** the spill is blocked by existing content. Clear the range; do not paste over it.

#### 22.4.1 UNIQUE — build a dimension list

```excel
=SORT(UNIQUE(tblSales[Region]))
```

Result: `East, North, South, West` (alphabetical).

```excel
=COUNTA(UNIQUE(tblSales[Product]))
```

Result: `3`.

Use `UNIQUE` to build validation lists, dimension tables, or a quick list of exception keys. If the source range contains blanks, `UNIQUE` may return a `0`; filter first:

```excel
=UNIQUE(FILTER(tblSales[Product], tblSales[Product]<>""))
```

#### 22.4.2 FILTER — the live exception list

List all sales rows with achievement below 90%:

```excel
=FILTER(tblSales[[Date]:[TargetRevenue]],
        (tblSales[Revenue]/tblSales[TargetRevenue]<0.9)*
        (tblSales[TargetRevenue]<>""),
        "No exceptions")
```

Result: 7 rows:

| Date | Region | Product | Revenue | Target | Achievement |
|---|---|---|---:|---:|---:|
| 2026-02-01 | West | CardioX | 35,640 | 41,636 | 85.60% |
| 2026-02-01 | South | CardioX | 41,520 | 48,483 | 85.64% |
| 2026-04-01 | West | CardioX | 40,080 | 45,523 | 88.04% |
| 2026-04-01 | South | CardioX | 46,800 | 53,130 | 88.09% |
| 2026-01-01 | West | CardioX | 36,000 | 40,499 | 88.89% |
| 2026-01-01 | South | CardioX | 42,000 | 47,227 | 88.93% |
| 2026-02-01 | North | NeuroY | 43,260 | 48,594 | 89.02% |

Count them without a helper column:

```excel
=ROWS(FILTER(tblSales[Revenue],
             (tblSales[Revenue]/tblSales[TargetRevenue]<0.9)*
             (tblSales[TargetRevenue]<>"")))
```

Result: `7`.

Why the `TargetRevenue<>""` condition? If a target is blank, `Revenue/Blank` is `#DIV/0!`, and the FILTER may error or include unintended rows. Always guard the denominator.

**Multiple conditions** use multiplication (AND) or addition (OR). For example, rows in North or South with achievement below 95%:

```excel
=FILTER(tblSales[[Date]:[TargetRevenue]],
        ((tblSales[Region]="North")+(tblSales[Region]="South"))*
        (tblSales[Revenue]/tblSales[TargetRevenue]<0.95)*
        (tblSales[TargetRevenue]<>""),
        "No exceptions")
```

#### 22.4.3 SORT and SORTBY — ranked views

Top five rows by revenue:

```excel
=TAKE(SORT(tblSales[[Date]:[Revenue]], 6, -1), 5)
```

`SORT(array, sort_index, sort_order)` sorts by the sixth column of the selected range (Revenue), descending (-1). `TAKE(...,5)` keeps the first five rows.

The top five are:

| Rank | Date | Region | Product | Revenue |
|---:|---|---|---|---:|
| 1 | 2026-06-01 | North | OncoZ | 70,080 |
| 2 | 2026-06-01 | North | CardioX | 66,276 |
| 3 | 2026-05-01 | North | CardioX | 58,338 |
| 4 | 2026-06-01 | South | OncoZ | 58,080 |
| 5 | 2026-05-01 | North | OncoZ | 57,600 |

If you need a multi-level sort (region ascending, then revenue descending), use `SORTBY`:

```excel
=SORTBY(tblSales, tblSales[Region], 1, tblSales[Revenue], -1)
```

#### 22.4.4 SEQUENCE — generate time periods

A sequence of first-of-month dates:

```excel
=EDATE(DATE(2026,1,1), SEQUENCE(6,1,0,1))
```

This returns 1 Jan 2026 through 1 Jun 2026. `SEQUENCE(rows, columns, start, step)` is useful for building a date dimension, a scenario index, or numbered ranks.

A simple 1–12 month index:

```excel
=SEQUENCE(12,1,1,1)
```

#### 22.4.5 CHOOSECOLS, TAKE, DROP, VSTACK

| Function | Use | Example |
|---|---|---|
| `CHOOSECOLS` | Select columns by position | `=CHOOSECOLS(tblSales, 2, 3, 6)` → Region, Product, Revenue |
| `TAKE` | First/last n rows | `=TAKE(sorted, 5)` |
| `DROP` | Remove first/last n rows | `=DROP(sorted, 5)` |
| `VSTACK` | Stack arrays vertically | Combine Jan–Mar and Apr–Jun extracts |
| `HSTACK` | Stack arrays horizontally | Build a side-by-side scenario table |

These are convenient, but do not use them just to show off. In a timed assessment, a clean helper column is often faster and easier to explain.

#### 22.4.6 LET — name intermediate calculations

`LET` makes complex formulas readable and faster because Excel calculates each named value once:

```excel
=LET(
  rev, SUMIFS(tblSales[Revenue], tblSales[Product],"CardioX"),
  units, SUMIFS(tblSales[Units], tblSales[Product],"CardioX"),
  IF(units=0,"",rev/units)
)
```

Result: `122.15`. Without `LET`, the same formula repeats the SUMIFS calls and is harder to audit. In an interview, `LET` is a strong signal that you write maintainable formulas.

**Compatibility note.** Dynamic arrays and `LET` require Microsoft 365 / Excel 2021+. If the assessment machine has an older version, use helper columns, Advanced Filter, Remove Duplicates, and standard formulas. Do not submit a workbook that shows `#NAME?` because you used a function the interviewer’s Excel does not support.

#### Check yourself — 22.4

1. What does `=UNIQUE(Region)` return for this dataset?
2. How many sales rows have achievement below 90%? Which regions/products dominate the list?
3. What is the top row by revenue, and what is its value?
4. What does the `#` operator do?
5. How do you count the rows returned by a FILTER without adding a helper column?

*Answers in 22.13.*

---

## Part E — Text and date engineering

### 22.5 Dirty keys and date logic: the invisible 30% of the work

Most “analysis” time is spent making text and dates usable. A product code with a trailing space, a date stored as text, or a region written as “N” instead of “North” will break every downstream pivot. The analyst who fixes the keys first finishes faster and with fewer wrong numbers.

#### 22.5.1 Text cleaning

| Function | What it does | Example | Result |
|---|---|---|---|
| `TRIM` | Removes leading/trailing spaces and collapses internal multiples | `=TRIM("  OncoZ  ")` | `OncoZ` |
| `CLEAN` | Removes non-printable characters | `=CLEAN(A2)` | clean text |
| `PROPER` | Title case | `=PROPER("cardiox")` | `Cardiox` (not the brand spelling) |
| `UPPER` / `LOWER` | Case normalisation | `=UPPER("north")` | `NORTH` |
| `SUBSTITUTE` | Replace specific text | `=SUBSTITUTE(A2,CHAR(160)," ")` | replaces non-breaking spaces |
| `TEXTJOIN` | Join with a delimiter, ignore blanks | `=TEXTJOIN(" / ",TRUE,A2:C2)` | `North / CardioX / May-26` |

`PROPER` is not a product-name fixer: `PROPER("cardiox")` returns `Cardiox`, not `CardioX`. Use a master table for brand names; do not rely on case functions to invent the correct capitalisation.

**Non-breaking spaces** are a classic hidden cause of failed lookups. `TRIM` does not remove `CHAR(160)`. Use:

```excel
=TRIM(SUBSTITUTE(A2,CHAR(160)," "))
```

#### 22.5.2 Parsing codes

Suppose a SKU has the form `CARD-120-30` (product-strength-pack). In Microsoft 365:

```excel
=TEXTBEFORE(A2,"-")                 → CARD
=TEXTBEFORE(TEXTAFTER(A2,"-"),"-")  → 120
=TEXTAFTER(A2,"-",2)                → 30
```

Legacy-compatible versions:

```excel
=LEFT(A2,FIND("-",A2)-1)                                  → CARD
=MID(A2,FIND("-",A2)+1,FIND("-",A2,FIND("-",A2)+1)-FIND("-",A2)-1) → 120
=RIGHT(A2,LEN(A2)-FIND("-",A2,FIND("-",A2)+1))            → 30
```

`TEXTSPLIT` is even more direct:

```excel
=TEXTSPLIT(A2,"-")
```

Result spills `CARD`, `120`, `30` across three cells. Use it when you need all parts; use `TEXTBEFORE`/`TEXTAFTER` when you need one part and want a single cell.

**Building a join key** from dirty fields:

```excel
=UPPER(TRIM(Region))&"|"&UPPER(TRIM(Product))&"|"&TEXT(Date,"yyyy-mm")
```

This normalises case and whitespace before concatenation. If the key still fails, compare lengths (`LEN`) and character codes (`CODE`) to find the invisible character.

#### 22.5.3 Date basics that matter

Excel stores dates as serial numbers. A “date” that is left-aligned is usually text. Check with:

```excel
=ISNUMBER(A2)   → TRUE for a real date, FALSE for text
```

Convert text dates:

```excel
=DATEVALUE("2026-06-01")
=DATE(LEFT(A2,4),MID(A2,6,2),1)
```

**Month label:**

```excel
=TEXT(A2,"mmm-yy")        → Jun-26
```

**First of month:**

```excel
=DATE(YEAR(A2),MONTH(A2),1)
```

**Month-end:**

```excel
=EOMONTH(A2,0)
```

**Fiscal quarter (April–March fiscal year):**

```excel
="FY"&IF(MONTH(A2)>=4,YEAR(A2)+1,YEAR(A2))&"-Q"&ROUNDUP(IF(MONTH(A2)>=4,MONTH(A2)-3,MONTH(A2)+9)/3,0)
```

For 15 May 2026, this returns `FY2027-Q1`. If the company uses a calendar fiscal year, the simpler formula is:

```excel
="Q"&ROUNDUP(MONTH(A2)/3,0)
```

For May 2026, that returns `Q2`. Always ask which fiscal calendar the client uses; do not assume.

**Cohort month:**

```excel
=TEXT(ResponseDate,"yyyy-mm")
```

**Tenure in months:**

```excel
=DATEDIF(StartDate,TODAY(),"M")
```

`DATEDIF` is undocumented in some Excel versions but works. For a deterministic report, use a fixed “as of” date rather than `TODAY()`, so the numbers do not change when the file is reopened.

#### 22.5.4 Date traps

| Trap | Symptom | Fix |
|---|---|---|
| Date stored as text | Left-aligned; `ISNUMBER` FALSE; sorting is alphabetical | `DATEVALUE`, Text to Columns, or Power Query |
| Time fraction included | A date looks like `2026-06-01 00:00`; equality tests fail | `INT(A2)` to strip time |
| Month-end overflow | `DATE(2026,13,1)` rolls to Jan 2027 | Validate month input; use `EOMONTH` |
| 1900 leap-year bug | Excel treats 1900 as a leap year | Do not compute date differences across 1900 |
| Locale format | `01/02/2026` means Jan 2 or Feb 1 depending on locale | Use ISO `yyyy-mm-dd` in imports |
| `TODAY()` volatility | Report changes every day | Use a fixed as-of date for auditability |

#### 22.5.5 Applied example: survey field window

The survey has a field window of **15 Jan 2026 to 31 Mar 2026**. Add a flag:

```excel
=AND(ResponseDate>=DATE(2026,1,15), ResponseDate<=DATE(2026,3,31))
```

Result for `RS-030` (2025-12-15): `FALSE`. Count out-of-window responses:

```excel
=COUNTIFS(tblSurvey[InWindow], FALSE)
```

Result: `1`. The cleaned analysis set is 39 respondents (40 unique minus 1 out-of-window). This is not a cosmetic exclusion: an out-of-window response may reflect a different market conditions or a pilot wave. Document the reason.

#### Check yourself — 22.5

1. What does `=TRIM(" OncoZ ")` return?
2. Extract the middle segment of `CARD-120-30` using both TEXTBEFORE/TEXTAFTER and MID/FIND.
3. What fiscal quarter is 15 May 2026 under an April–March fiscal year?
4. How many survey responses fall outside the field window?
5. Why is `TODAY()` risky in a client report?

*Answers in 22.13.*

---

## Part F — Advanced PivotTables, PivotCharts, and calculated fields

### 22.6 PivotTables are a summarisation engine, not a chart wizard

A PivotTable answers a **two-dimensional aggregation** question: “revenue by region and product”, “respondents by specialty and awareness stage”, “monthly revenue with a running total.” The value is in the speed of iteration, not in the default layout.

**When to use a PivotTable vs formulas:**

| Use a PivotTable when… | Use formulas when… |
|---|---|
| You need to explore multiple cuts quickly | The output must update in a fixed layout |
| The audience wants slicers/interactivity | The workbook must work in older Excel or Google Sheets |
| You need show-values-as (share, running total) | You need a specific audit trail per cell |
| The data is clean and tabular | The data needs complex row-level logic first |

#### 22.6.1 Building the core sales pivot

Source: `excel_advanced_sales.csv`. Rows: `Region`, then `Product`. Columns: `Date` grouped by month. Values: `Sum of Revenue`.

Expected matrix (₹):

| Region | CardioX | NeuroY | OncoZ | Total |
|---|---:|---:|---:|---:|
| North | 337,134 | 246,540 | 281,280 | 864,954 |
| South | 280,944 | 209,580 | 233,760 | 724,284 |
| East | 192,510 | 148,050 | 140,640 | 481,200 |
| West | 240,882 | 184,800 | 187,200 | 612,882 |
| **Total** | **1,051,470** | **788,970** | **842,880** | **2,683,320** |

This matrix is the single most useful object in the module. Almost every later question can be answered by pivoting it or filtering it.

#### 22.6.2 Show Values As

Right-click the value field → **Show Values As**:

| Option | What it answers | Example |
|---|---|---|
| % of Grand Total | What share of the whole? | North CardioX = 12.56% of total |
| % of Parent Row Total | What share of its region? | North CardioX = 38.98% of North |
| % of Parent Column Total | What share of its product? | North CardioX = 32.06% of CardioX |
| Running Total In | Cumulative over a sequence | June cumulative = ₹2,683,320 |
| Difference From | Absolute change vs a base | June vs May by product |
| % Difference From | Percentage change vs a base | CardioX June vs May |

**Verify the parent-row share:**

```
North CardioX / North total = 337,134 / 864,954 = 38.98%
```

**Verify the parent-column share:**

```
North CardioX / CardioX total = 337,134 / 1,051,470 = 32.06%
```

**Running total by month:**

| Month | Revenue | Cumulative |
|---|---:|---:|
| Jan-26 | 378,000 | 378,000 |
| Feb-26 | 397,470 | 775,470 |
| Mar-26 | 424,980 | 1,200,450 |
| Apr-26 | 454,770 | 1,655,220 |
| May-26 | 487,800 | 2,143,020 |
| Jun-26 | 540,300 | 2,683,320 |

The running total is a useful sanity check: the final cumulative must equal the grand total. If it does not, the pivot source range is missing rows.

#### 22.6.3 Calculated fields vs calculated items

This is the most misunderstood PivotTable feature.

**Calculated field** — a new value field computed from other value fields, at the aggregate level:

```text
Achievement % = Revenue / TargetRevenue
```

If Revenue = ₹2,683,320 and Target = ₹2,717,035, the calculated field returns `98.76%`. This is the correct aggregate achievement.

But it is **not** the average of row-level achievement percentages. The pivot sums revenue and sums target, then divides. That weights large rows more heavily. If you want the unweighted average of row percentages, add a helper column in the source and use `Average of RowAchievement`.

**Calculated item** — a new row/column item inside an existing field. For example, create a “Total prescribers” item that combines “Trial prescribed” and “Regular prescriber.” Calculated items are fragile when the underlying field has many members; use a helper column in the source when possible.

**Limitations of calculated fields:**

- They operate on the **sum** of the underlying fields, not on individual rows.
- They cannot reference a cell, a named range, or another pivot.
- They do not work with all data models (especially when using OLAP or Power Pivot).
- They can produce misleading results when the denominator is blank or zero.

**Revenue per call:**

```text
Revenue per Call = Revenue / Calls
= 2,683,320 / 5,791 = 463.36
```

But be careful: `Calls` is allocated at the product-line level in this dataset, and three rows are blank. The pivot’s `Sum of Calls` ignores blanks, so the ratio is based on reported calls only. If the business wants revenue per **total** call, you need a complete call denominator; otherwise the metric is biased upward.

#### 22.6.4 Grouping, top-N, and sorting

- **Group dates:** right-click a date → Group → Months / Quarters / Years. Create a separate “Month” grouping so you can show Jan–Jun and Q1/Q2 in the same pivot.
- **Group numeric bands:** right-click `YearsInPractice` → Group → starting at 0, ending at 40, by 10. This creates 0–9, 10–19, etc.
- **Top-N filter:** filter rows by value → Top 10. For this dataset, “Top 3 products” is trivial (only three products); “Top 5 region-product-month rows” is more useful.
- **Sort by value:** sort the row field by `Sum of Revenue` descending, not alphabetically. Alphabetical sorting hides the story.

**Top 5 rows by revenue (pivot):**

| Rank | Region | Product | Month | Revenue |
|---:|---|---|---|---:|
| 1 | North | OncoZ | Jun-26 | 70,080 |
| 2 | North | CardioX | Jun-26 | 66,276 |
| 3 | North | CardioX | May-26 | 58,338 |
| 4 | South | OncoZ | Jun-26 | 58,080 |
| 5 | North | OncoZ | May-26 | 57,600 |

#### 22.6.5 GETPIVOTDATA

`GETPIVOTDATA` retrieves a value from a pivot by field/item names, so the formula survives layout changes:

```excel
=GETPIVOTDATA("Revenue", $A$3, "Region","North", "Product","CardioX")
```

Result: `337,134`.

Use it when you need a fixed KPI cell on a dashboard. Disable automatic generation (PivotTable Analyze → Options → Generate GetPivotData) when you want a normal cell reference instead.

**Caution:** `GETPIVOTDATA` fails if the item name changes (e.g., “CardioX ” with a trailing space). It is a lookup, so the same key-cleaning discipline applies.

#### 22.6.6 Slicers, timelines, and connected pivots

- **Slicer:** a visual filter for a field (Region, Product). Place it next to the pivot, not on top of the data.
- **Timeline:** a date filter for month/quarter/year. Useful for a dashboard.
- **Report connections:** one slicer can control multiple pivots if they share the same pivot cache. Right-click the slicer → Report Connections → tick the pivots.
- **Pivot cache:** all connected pivots must be based on the same source and refreshed together. If you add rows to the source, refresh (Alt+F5) and verify the grand total.

#### 22.6.7 PivotCharts

A PivotChart is linked to a PivotTable. It is useful for interactive exploration, but it has limitations:

- Chart types are restricted compared with normal charts.
- Calculated fields/items can behave unexpectedly.
- Formatting may reset when the pivot refreshes.
- A combo chart with a secondary axis can exaggerate a small series; use it only when the units genuinely differ and label both axes.

**Recommended chart for the region × product matrix:** a clustered column chart with Region on the category axis and Product as series. It answers “which region-product combination is largest?” without implying a trend.

**Recommended chart for the monthly trend:** a line chart with Month on the x-axis and Revenue on the y-axis. Add a target line only if the target is comparable at the same grain.

#### 22.6.8 Data model and relationships

Instead of VLOOKUP-ing attributes into the fact table, load the sales, product, and region tables into the Data Model and create relationships:

- `Sales[Product]` → `Product[Product]` (many-to-one)
- `Sales[Region]` → `Region[Region]` (many-to-one)

Then a pivot can use `Product[TherapeuticArea]` as a row field even though it is not in the sales table. This is the Excel version of the star schema taught in [20-power-bi-complete-practical-guide.md](20-power-bi-complete-practical-guide.md). Benefits: smaller file, no repeated lookups, consistent dimensions.

**Common model mistakes:**

- Joining on a non-unique key.
- Creating a relationship between two fact tables.
- Using a date column without a proper date table.
- Assuming filter direction is bidirectional when it is single.
- Forgetting to mark the date table as a date table.

#### Check yourself — 22.6

1. What is the calculated-field achievement for the full sales file?
2. What is North CardioX as a percentage of North, and as a percentage of total?
3. What is the cumulative revenue through June?
4. Write a `GETPIVOTDATA` formula for North CardioX revenue.
5. Why can a calculated field’s achievement differ from the average of row-level achievement?

*Answers in 22.13.*

---

## Part G — Conditional formatting and data validation

### 22.7 Make the workbook audit itself

Conditional formatting (CF) and data validation (DV) are not decoration. They are control systems: CF surfaces exceptions, DV prevents them at entry. In a timed assessment, a well-built CF rule can save you minutes of manual scanning.

#### 22.7.1 Formula-based conditional formatting

The key to formula-based CF is **mixed referencing**: the formula is written for the top-left cell of the selected range, and the row/column locks determine how it fills.

**A. Highlight an entire row where achievement is below 90%.**

Select `A2:H73`, then use:

```excel
=AND($G2<>"", $F2/$G2<0.9)
```

- `$G2` locks the target column (G) but lets the row move.
- `$F2` locks the revenue column (F).
- The formula is evaluated for row 2, then applied relatively to rows 3–73.
- The `$G2<>""` guard prevents blank targets from producing `#DIV/0!`.

**B. Highlight duplicate respondent IDs.**

Select `A2:A43` (survey raw), then:

```excel
=COUNTIF($A$2:$A2,$A2)>1
```

This highlights the **second and later** occurrences, not the first. If you want to highlight all occurrences of any duplicated ID, use:

```excel
=COUNTIF($A$2:$A$43,$A2)>1
```

**C. Highlight rows with a blank required field.**

Select `A2:H73`, then:

```excel
=AND($A2<>"", $H2="")
```

This highlights rows that have a key but are missing calls. Be explicit about whether “blank” is allowed; in this dataset, blank calls are a known data-quality issue, not a formula error.

**D. Highlight outliers using a helper threshold.**

Put lower and upper bounds in cells (e.g., `K1`, `K2`), then:

```excel
=OR($F2<$K$1, $F2>$K$2)
```

A simple outlier rule is mean ± 3 standard deviations, or the IQR rule (below Q1 − 1.5×IQR or above Q3 + 1.5×IQR). For revenue, a fixed business threshold is often more useful than a statistical one.

**E. Data bars, icon sets, and colour scales.**

- Data bars: good for comparing revenue across rows; keep the axis visible.
- Icon sets: use only three icons and define the thresholds; otherwise they imply precision that is not there.
- Colour scales: good for heatmaps (region × product); avoid red–green as the only encoding because of colour-blindness.

**Rule order and “Stop If True.”** Excel evaluates rules in the order shown. If a row matches multiple rules, later rules can override earlier formatting. Use “Stop If True” when the first rule should win. Put the most specific rule at the top.

#### 22.7.2 Data validation

**A. List validation from a named range.**

- Select the input cells → Data → Data Validation → Allow: List → Source: `=tblRegion[Region]`.
- Better: create a named range `RegionList` and use `=RegionList`. Named ranges are easier to audit.

**B. Dependent dropdowns with INDIRECT.**

Suppose `B2` contains a therapeutic area (`Cardiovascular`, `Neurology`, `Oncology`) and you want `C2` to show only products in that area.

1. Create three named ranges: `Cardiovascular`, `Neurology`, `Oncology`, each containing the relevant products.
2. In `C2`, Data Validation → List → Source: `=INDIRECT($B2)`.
3. Ensure the names are valid (no spaces, no punctuation). `INDIRECT` is volatile and breaks if the name changes; document it.

A more robust modern alternative is a filtered list:

```excel
=FILTER(tblProduct[Product], tblProduct[TherapeuticArea]=$B2)
```

This is dynamic and avoids volatile `INDIRECT`, but it requires Microsoft 365.

**C. Custom formula validation.**

Unique ID (no duplicates):

```excel
=COUNTIF($A$2:$A$100,$A2)=1
```

Date within the field window:

```excel
=AND($C2>=DATE(2026,1,15), $C2<=DATE(2026,3,31))
```

Whole number 0–10 for NPS:

```excel
=AND(ISNUMBER($J2), $J2>=0, $J2<=10, INT($J2)=$J2)
```

**D. Input messages and error alerts.**

Use an input message to tell the user the expected format (“Enter a whole number from 0 to 10”). Use a stop-style error alert for critical fields; use a warning style when the value is unusual but allowed. A validation rule with no message is a trap for the next user.

**E. Circle invalid data.**

Data → Data Validation → Circle Invalid Data. This is useful for reviewing an existing sheet. It does not fix anything; it only marks cells.

> **Critical limitation:** data validation does not apply to values pasted into the cell, and it does not clean existing data. A validated sheet can still contain invalid values if someone pastes over it. Always run the QA formulas in 22.8 after entry.

#### Check yourself — 22.7

1. Write a CF formula that highlights an entire row when achievement is below 90%.
2. How do you highlight all occurrences of a duplicated ID, not just the second one?
3. What is the custom validation formula for a unique ID?
4. Why is `INDIRECT` fragile, and what is the modern alternative?
5. Why is data validation not a substitute for a QA check?

*Answers in 22.13.*

---

## Part H — Reconciliation and data-quality assurance

### 22.8 Reconciliation: explain the difference, do not force it

Reconciliation is the process of comparing two sources that should agree, classifying every difference, and explaining the net variance. It is a core analyst skill because client data rarely arrives as one clean source. In an Excel assessment, a reconciliation task tests discipline: can you find missing rows, duplicates, and amount mismatches without losing the totals?

#### 22.8.1 The reconciliation workflow

1. **Normalise the key.** `TRIM`, case, leading zeros, date format. A key that looks the same but is stored differently will create false “missing” rows.
2. **Check uniqueness on both sides.** `COUNTIF` > 1 flags duplicates. Decide which record is authoritative.
3. **Outer-join the key sets.** Use `COUNTIF`/`XLOOKUP` to flag:
   - in ledger, not in finance (missing in finance);
   - in finance, not in ledger (extra in finance);
   - in both (common).
4. **Compare amounts.** Compute `Finance − Ledger`. Apply a tolerance (e.g., ₹5 for rounding).
5. **Classify every exception.** Exact match, within tolerance, amount mismatch, missing, extra, duplicate.
6. **Bridge the totals.** Start with source A total, add/subtract each exception category, and arrive at source B total.
7. **Report gross and net.** A small net variance can hide large offsetting errors.

#### 22.8.2 Worked reconciliation: ledger vs finance

**Source totals:**

| Source | Rows | Total amount |
|---|---:|---:|
| Sales ledger | 20 | ₹318,600 |
| Finance extract | 20 | ₹321,251 |
| **Net variance** | | **+₹2,651** |

At first glance, a ₹2,651 difference on ₹3.19 lakh looks small (0.83%). That is exactly why a net-only check is dangerous. The gross exceptions are much larger.

**Exception classification:**

| Exception | Invoice(s) | Amount | Count |
|---|---|---:|---:|
| Missing in finance | INV-1004 | ₹7,080 | 1 |
| Missing in finance | INV-1017 | ₹5,460 | 1 |
| Extra in finance | INV-1021 | ₹8,400 | 1 |
| Duplicate in finance | INV-1009 | ₹11,040 | 1 |
| Amount mismatch (above tolerance) | INV-1006 | +₹250 | 1 |
| Amount mismatch (within tolerance) | INV-1012 | +₹1 | 1 |
| Amount mismatch (above tolerance) | INV-1015 | −₹4,500 | 1 |

**Common IDs:** 18 unique invoices appear in both sources. Of these, 15 match exactly; 3 have amount differences. The duplicate INV-1009 adds a second row to finance, so the finance row count is 20 even though there are only 19 unique finance invoices.

**Tolerance policy:** ₹5. INV-1012 (+₹1) is within tolerance and can be accepted as rounding. INV-1006 (+₹250) and INV-1015 (−₹4,500) are material and require investigation.

**Variance bridge:**

```
Ledger total                                318,600
− Missing in finance (INV-1004)              −7,080
− Missing in finance (INV-1017)              −5,460
+ Extra in finance (INV-1021)                +8,400
+ Duplicate in finance (INV-1009)           +11,040
+ Amount mismatch (INV-1006)                   +250
+ Amount mismatch (INV-1012)                     +1
+ Amount mismatch (INV-1015)                 −4,500
= Finance total                              321,251
```

Check: `318,600 − 12,540 + 8,400 + 11,040 + 250 + 1 − 4,500 = 321,251`. ✓

**Gross exception value:**

```
12,540 + 8,400 + 11,040 + 250 + 1 + 4,500 = 36,731
```

The net variance is ₹2,651, but the gross exceptions are ₹36,731 — fourteen times larger. The missing invoices and the duplicate/extra invoices nearly offset. A process that only checks the net would miss both a duplicate payment and two unpaid invoices.

> **So what does this mean for the business?** The CFO should not hear “the difference is only ₹2,651.” The correct message is: “Net variance is ₹2,651, but we have two missing invoices (₹12,540), one duplicate (₹11,040), one extra invoice (₹8,400), and two material amount mismatches. We need to confirm the duplicate before payment and trace the missing invoices.” That is an analyst answer, not a bookkeeping answer.

#### 22.8.3 QA checklist for any dataset

Build a QA block at the top of the workbook. For each check, record the expected value, actual value, and status.

| Check | Formula pattern | Sales | Survey | Assessment |
|---|---|---|---|---|
| Row count | `COUNTA(key)` | 72 | 42 raw | 37 raw |
| Duplicate rows (removed) | `COUNTIF(key,key)>1` flags both copies | 0 | 2 removed (4 flagged: RS-007, RS-023) | 1 removed (2 flagged) |
| Blank key | `COUNTBLANK(key)` | 0 | 0 | 0 |
| Blank required numeric | `COUNTBLANK(value)` | 2 targets, 3 calls | 1 TRx | 1 units, 1 target |
| Invalid category | `COUNTIFS(category,"<>allowed")` | 0 | 2 region variants, 1 specialty space | 3 region variants, 1 product space, 1 rep space |
| Negative value | `COUNTIF(value,"<0")` | 0 | 0 | 1 return (−12 units) |
| Out-of-range Likert | `COUNTIFS(value,"<>",value,"<1")+...` | n/a | 1 (7) | n/a |
| Date outside window | `COUNTIFS(date,"<start")+...` | 0 | 1 (RS-030) | 0 |
| Revenue ≠ units × price | `SUMPRODUCT(--(ABS(rev−units*price)>1))` | 0 | n/a | 0 numeric rows; 1 blank-units row excluded (expected) |
| Lookup failures | `COUNTIF(attribute,"CHECK*")` | 0 | n/a | 0 after TRIM |
| Duplicate ID in master | `COUNTIF(master[key],master[key])>1` | 0 | 0 | 0 |

**The three questions for every QA item:**

1. Is the value expected? (Document known blanks/exceptions.)
2. If not expected, what is the impact on the analysis?
3. What is the corrective action and owner?

#### 22.8.4 Survey quota reconciliation

The survey was designed with a target of **8 respondents per awareness stage** (40 total). After cleaning:

| Stage | Target | Actual | Variance |
|---|---:|---:|---:|
| Unaided awareness | 8 | 8 | 0 |
| Aided awareness | 8 | 8 | 0 |
| Consideration | 8 | 8 | 0 |
| Trial prescribed | 8 | 7 | −1 |
| Regular prescriber | 8 | 8 | 0 |
| **Total** | **40** | **39** | **−1** |

The shortfall is RS-030, whose response date is outside the field window. Options: (a) accept 39 and report the shortfall; (b) replace with a new respondent from the same specialty/region; (c) weight the trial stage up. Do not silently keep the out-of-window response just to hit quota — that contaminates the field-period definition.

#### 22.8.5 Cross-foot and referential integrity

- **Cross-foot:** ensure row totals equal column totals. In the region × product matrix, both sum to ₹2,683,320.
- **Referential integrity:** every `Product` in sales must exist in the product master; every `Region` must exist in the region master. `COUNTIF` of unmatched keys should be zero.
- **Unit-price consistency:** `Revenue = Units × UnitPrice` for every row (tolerance ₹1). If not, either units, price, or revenue is wrong.
- **Date completeness:** every month in the analysis window should have 12 region-product rows. Missing months create false trend breaks.

#### Check yourself — 22.8

1. What are the ledger and finance totals, and what is the net variance?
2. Which invoices are missing in finance, and for how much?
3. What is the duplicate invoice and its amount?
4. Write the variance bridge from ledger to finance.
5. Which amount differences are within the ₹5 tolerance, and which are material?
6. Why is the net variance misleading?
7. How many survey respondents fall short of the trial-stage quota, and why?

*Answers in 22.13.*

---

## Part I — What-if, scenario, and sensitivity analysis

### 22.9 Model the decision, not just the spreadsheet

What-if analysis answers: **“What happens to the output if an input changes?”** In an analyst assessment, it tests whether you can separate assumptions from calculations and communicate a range instead of a single false-precision number.

#### 22.9.1 Goal Seek

Goal Seek finds the input needed to hit a target output. Example: June CardioX revenue is ₹206,640 at 1,640 units and ₹126 per unit. What unit volume is needed for ₹223,171 (8% above June)?

Set up:

| Cell | Value |
|---|---:|
| Units | 1,640 |
| Price | 126 |
| Revenue | `=Units*Price` → 206,640 |
| Target revenue | 223,171 |

Data → What-If Analysis → Goal Seek:

- Set cell: Revenue
- To value: 223,171
- By changing cell: Units

Result: **1,771.2 units**, approximately **1,772 units**. That is a **+8.05%** volume lift (`132 / 1,640`). Since price is unchanged, revenue scales one-for-one with volume.

**Business interpretation:** an 8% revenue target requires an 8% volume increase if price is held. If the market cannot absorb 132 extra units, the team needs price, mix, or a new indication — not a sales-force pep talk.

#### 22.9.2 One-variable data table

A one-variable data table shows how an output changes as one input moves.

Setup for June CardioX:

- Input cell: price change % (e.g., `-10%` to `+10%`).
- Output: revenue, holding units constant at 1,640.

| Price change | Price | Revenue |
|---:|---:|---:|
| −10% | 113.40 | 185,976 |
| −5% | 119.70 | 196,308 |
| 0% | 126.00 | 206,640 |
| +5% | 132.30 | 216,972 |
| +10% | 138.60 | 227,304 |

Formula: `Revenue = 1,640 × 126 × (1 + price change)`.

This is a **partial** analysis: it assumes volume does not respond to price. That is rarely true. The two-variable table adds the volume response.

#### 22.9.3 Two-variable data table

Rows: price change. Columns: volume change. Output: revenue.

| Price ↓ / Volume → | −5% | 0% | +5% | +10% | +15% |
|---|---:|---:|---:|---:|---:|
| **−10%** | 176,677 | 185,976 | 195,275 | 204,574 | 213,872 |
| **−5%** | 186,493 | 196,308 | 206,123 | 215,939 | 225,754 |
| **0%** | 196,308 | 206,640 | 216,972 | 227,304 | 237,636 |
| **+5%** | 206,123 | 216,972 | 227,821 | 238,669 | 249,518 |
| **+10%** | 215,939 | 227,304 | 238,669 | 250,034 | 261,400 |

Formula: `Revenue = 1,640 × 126 × (1 + price change) × (1 + volume change)`.

**Reading the table:**

- At −10% price and +12% volume (not shown; interpolate between +10% and +15%), revenue is ₹208,293 — only +0.80% versus base. A 10% price cut barely pays for itself even with a 12% volume lift.
- At −5% price and +5% volume, revenue is ₹206,123, slightly **below** base. The price cut destroys more revenue than the volume adds.
- At +5% price and +5% volume, revenue is ₹227,821 (+10.26%). This is the best of the “realistic” cells if volume can grow despite a price rise.

> **So what does this mean for the business?** “Raise price” is not automatically good, and “cut price” is not automatically bad. The answer depends on the volume response (elasticity), competitor response, payer access, and the margin impact. A two-way table makes the assumption explicit; a single-point forecast hides it.

#### 22.9.4 Break-even analysis

**Revenue break-even for a price cut:** if price falls by 10%, volume must rise by:

```
1 / (1 − 0.10) − 1 = 11.11%
```

So a 10% price cut needs +11.11% volume just to hold revenue constant.

**Profit break-even is higher.** Suppose variable cost is ₹56.70 per unit, fixed regardless of price:

- Current contribution per unit = ₹126.00 − ₹56.70 = ₹69.30.
- Current contribution = 1,640 × ₹69.30 = ₹113,652.
- New price after 10% cut = ₹113.40.
- New contribution per unit = ₹113.40 − ₹56.70 = ₹56.70.
- Break-even volume = ₹113,652 / ₹56.70 = **2,004.4 units**, approximately **2,005 units**.
- Required lift = `(2,005 − 1,640) / 1,640 = 22.26%`, approximately **22.2%**.

This is the key insight: **revenue break-even is 11.1%, but profit break-even is 22.2%** when variable cost is fixed per unit. A price cut that looks acceptable on revenue can destroy contribution.

**Elasticity cases:**

| Volume response to −10% price | New volume | New revenue | Change vs base |
|---|---:|---:|---:|
| +8% (low elasticity) | 1,771.2 | ₹200,854 | −2.80% |
| +12% (moderate) | 1,836.8 | ₹208,293 | +0.80% |
| +15% (high) | 1,886.0 | ₹213,872 | +3.50% |

Even the high-elasticity case produces only +3.5% revenue, and it may still be profit-negative because of the margin hit. Always show revenue and contribution side by side.

#### 22.9.5 Scenario analysis

Scenarios are coherent stories about the future, not arbitrary point changes. For H2 2026, use June as the exit run-rate:

| Scenario | Assumption | H2 revenue |
|---|---|---:|
| Worst | June run-rate −8% | ₹11.41 lakh |
| Base | June run-rate flat | ₹12.40 lakh |
| Best | June run-rate +8% | ₹13.39 lakh |

Calculation: `June revenue × 6 × (1 + adjustment)`.

- Base: `206,640 × 6 = 1,239,840`.
- Worst: `206,640 × 0.92 × 6 = 1,140,653`.
- Best: `206,640 × 1.08 × 6 = 1,339,027`.

Probability-weighted value (30% worst, 50% base, 20% best):

```
0.30 × 1,140,653 + 0.50 × 1,239,840 + 0.20 × 1,339,027
= 342,196 + 619,920 + 267,805
= 1,229,921
```

The weighted value is **below** the base case because the downside is weighted more heavily. That is a deliberate risk posture, not a mathematical error. State the probabilities and the rationale; do not present the weighted value as objective truth.

#### 22.9.6 Sensitivity and tornado charts

A tornado chart ranks drivers by their impact on the output. For June CardioX contribution:

| Driver | Change | Impact on contribution |
|---|---:|---:|
| Volume | ±10% | ±₹20,664 |
| Price | ±5% | ±₹10,332 |
| Variable cost per unit | ±10% | ±₹9,299 |

Volume is the biggest driver, then price, then variable cost. But the ranking depends on the ranges chosen: a ±10% price move would dominate. Always state the range next to the bar.

**How to build it:**

1. Set a base case.
2. Change one driver at a time, holding others constant.
3. Record the output.
4. Sort by absolute impact.
5. Plot a horizontal bar chart with the base in the centre.

**What not to do:** do not vary all drivers simultaneously and call it sensitivity. That is a scenario, not a one-at-a-time sensitivity. And do not show a tornado without the base value; the reader cannot interpret the bars.

#### 22.9.7 Presenting uncertainty

Weak: “Revenue will be ₹12.4 lakh.”
Strong: “Base case is ₹12.4 lakh for H2; the realistic range is ₹11.4–13.4 lakh, with a probability-weighted value of ₹12.3 lakh. The biggest driver is volume: a ±10% volume change moves contribution by ±₹20.7k, twice the impact of a ±5% price change.”

The strong version gives a point estimate, a range, the key driver, and the assumption. That is what a client pays for.

#### Check yourself — 22.9

1. What unit volume is needed for June CardioX to hit ₹223,171?
2. What is the revenue break-even volume lift for a 10% price cut?
3. What is the profit break-even lift if variable cost is ₹56.70 per unit?
4. What is June CardioX revenue at −10% price and +12% volume?
5. What is the probability-weighted H2 revenue under 30/50/20?
6. Rank the three drivers in the tornado and state the ranges.

*Answers in 22.13.*

---

## Part J — Survey analysis in Excel

### 22.10 From respondent rows to a client recommendation

A survey dataset is not a list of answers; it is a set of evidence. The analyst’s job is to turn it into **validated percentages, segment differences, and implications**. Excel is enough for most physician surveys if you are disciplined about cleaning, denominators, and weighting.

#### 22.10.1 The workflow

1. **Read the codebook.** What does each column mean? Which scale direction is positive? What are the missing codes (`99`, `N/A`, blank)?
2. **Clean and de-duplicate.** Remove exact duplicates; standardise region/specialty; trim whitespace; convert text numbers.
3. **Apply the field window.** Exclude responses outside the defined dates, with a documented reason.
4. **Flag quality issues.** Straight-liners, invalid codes, inconsistent answers, blank key fields.
5. **Compute frequencies and cross-tabs.** Always show the base (`n`) next to every percentage.
6. **Weight if the design requires it.** Compare weighted and unweighted; explain material differences.
7. **Interpret with caution.** Small bases, non-response bias, and question wording can all change the story.

#### 22.10.2 Cleaning the fictional physician survey

Raw file: 42 rows. After removing the two exact duplicates (RS-007 and RS-023): 40 unique respondents. After excluding RS-030 (response date 2025-12-15, outside the 15 Jan–31 Mar 2026 field window): **39 analysis respondents**.

| Issue | Respondent(s) | Treatment | Impact |
|---|---|---|---|
| Exact duplicate rows | RS-007, RS-023 | Remove second occurrence | Prevents double-counting |
| Out-of-window response | RS-030 | Exclude from analysis; report | Trial stage falls from 8 to 7 |
| Likert value 7 (out of 1–5) | RS-019 (Access) | Exclude from Access base | Access n = 38 |
| `"N/A"` in Likert | RS-020 (Efficacy) | Exclude from Efficacy base | Efficacy n = 38 |
| Straight-liner (all 3s) | RS-021 | Flag; sensitivity check | Does not change top-2-box materially |
| Region variants | RS-022 `"north "`, RS-024 `"N"` | Map to North | Prevents a false fifth region |
| Specialty trailing space | RS-027 `"Oncologist "` | Trim | Prevents a false specialty |
| Blank TRx | RS-028 | Exclude from TRx sums; include in Likert | TRx n = 38 |
| Negative/zero TRx | Pre-trial respondents | Valid zero, not missing | Do not drop |

> **Do not “fix” a straight-liner by deleting it without a rule.** A straight-liner may be a genuinely neutral respondent or a careless one. The defensible approach is to flag it, run the analysis with and without it, and report whether the conclusion changes. In this dataset, removing RS-021 changes the top-2-box by only a small amount, so the conclusion is robust.

#### 22.10.3 Frequencies and the awareness funnel

After cleaning (n = 39):

| Stage | Respondents | Share |
|---|---:|---:|
| Unaided awareness | 8 | 20.5% |
| Aided awareness | 8 | 20.5% |
| Consideration | 8 | 20.5% |
| Trial prescribed | 7 | 17.9% |
| Regular prescriber | 8 | 20.5% |
| **Total** | **39** | **100.0%** |

**Important:** these are **current-status segments**, not cumulative funnel stages. A respondent in “Regular prescriber” is not also counted in “Trial prescribed.” Do not add the stages to create a funnel; that would double-count. If you want a cumulative funnel, the questionnaire must ask “ever aware / ever trial / current use” as separate questions.

What you can say:

- 38.5% of respondents are current prescribers (trial + regular: 15/39).
- Among current prescribers, 8/15 = **53.3%** have converted to regular prescribing.
- Regular prescribers account for **79.1%** of monthly TRx (272/344), despite being only 20.5% of the sample.

> **So what does this mean for the business?** The commercial opportunity is not evenly distributed. Eight regular prescribers generate four-fifths of TRx. A launch team should protect and expand this cohort first, while diagnosing why the 7 trial prescribers have not converted. Mass awareness spend may be less efficient than access and conversion work.

#### 22.10.4 Likert analysis: top-2-box and means

For a 5-point Likert scale, the **top-2-box** is the percentage who selected 4 or 5. It is easier for executives to interpret than a mean, and it is less sensitive to the middle category.

| Item | n | Top-2-box (unweighted) | Top-2-box (weighted) | Mean (unweighted) | Mean (weighted) |
|---|---:|---:|---:|---:|---:|
| Intent to prescribe | 39 | 35.90% | 34.98% | 3.10 | 3.11 |
| Access / reimbursement | 38 | 36.84% | 37.68% | 3.03 | 3.05 |
| Efficacy satisfaction | 38 | 47.37% | 47.94% | 3.45 | 3.47 |

**Interpretation:**

- Efficacy is the strongest driver (47.4% top-2-box), but still fewer than half of respondents are positive.
- Access is the weakest (36.8%), and the gap versus efficacy is about 10.5 percentage points. That is a signal to investigate reimbursement, formulary position, and patient support.
- Intent (35.9%) is close to access, suggesting that access may be constraining intent.
- Weighting changes the estimates by less than 1.5 percentage points. In this dataset, the conclusion is not weight-sensitive; report that as a robustness check.

**Caveat:** with n ≈ 38, a difference of 2–3 respondents is 5–8 percentage points. Do not call a 5-point difference “significant” without a statistical test and a reasonable base. The module 06 concepts apply; Excel can compute the percentages, but significance requires the right test and assumptions.

#### 22.10.5 NPS

NPS uses a 0–10 recommendation question:

- Promoters: 9–10
- Passives: 7–8
- Detractors: 0–6
- NPS = % promoters − % detractors

Results (n = 39):

| Segment | Count | Share |
|---|---:|---:|
| Promoters | 12 | 30.8% |
| Passives | 12 | 30.8% |
| Detractors | 15 | 38.5% |
| **NPS** | | **−7.69** |

Weighted NPS: **−6.81**. The negative NPS means detractors outnumber promoters. For a new brand, that is a warning: trial alone is not creating advocacy.

**Do not present NPS as a single gauge without the components.** A gauge hides the fact that 30.8% are passive — a group that could move either way. Show the stacked bar and the base.

#### 22.10.6 Cross-tabulation: specialty × awareness stage

Cross-tabs reveal whether a segment is over- or under-represented at each stage. Counts (n = 39):

| Specialty | Unaided | Aided | Consideration | Trial | Regular | Total |
|---|---:|---:|---:|---:|---:|---:|
| Cardiologist | 2 | 2 | 2 | 2 | 2 | 10 |
| Neurologist | 2 | 2 | 2 | 1 | 2 | 9 |
| Oncologist | 2 | 2 | 2 | 2 | 2 | 10 |
| Primary Care | 2 | 2 | 2 | 2 | 2 | 10 |
| **Total** | **8** | **8** | **8** | **7** | **8** | **39** |

The design was balanced by specialty, so the main deviation is the missing trial-stage neurologist (RS-030, out of window). With cells of 1–2 respondents, do not interpret specialty differences; the base is too small. This table is useful for **quota monitoring**, not for segment conclusions.

**TRx by specialty among current prescribers:**

| Specialty | TRx (monthly) | Share of TRx |
|---|---:|---:|
| Cardiologist | 72 | 20.9% |
| Neurologist | 75 | 21.8% |
| Oncologist | 100 | 29.1% |
| Primary Care | 97 | 28.2% |
| **Total** | **344** | **100.0%** |

Oncologists and Primary Care generate 57.3% of TRx among current prescribers. But again, n = 14 current prescribers; this is a directional signal, not a precise segmentation.

#### 22.10.7 Weighting

A weight column adjusts each respondent’s contribution to the estimate. Reasons for weighting:

- The sample over-represents a specialty or region relative to the population.
- Post-stratification to known physician counts.
- Design weights from unequal probability of selection.

In this dataset, weights range from 0.8 to 1.4 and sum to 42.6 (not 39, because the weights are illustrative). Weighted top-2-box differs from unweighted by less than 1.5 points for all three items. That is a useful robustness statement: **the conclusion does not hinge on the weights.**

**How to compute weighted top-2-box in Excel:**

```excel
=SUMPRODUCT((Likert>=4)*Weight) / SUM(Weight)
```

Guard against blank/invalid Likert values:

```excel
=SUMPRODUCT((Likert>=4)*(Likert<>"")*(Likert<=5)*Weight)
 / SUMPRODUCT((Likert<>"")*(Likert<=5)*Weight)
```

**Effective sample size:** weighting reduces the effective sample size when weights are unequal. A rough measure is `(Σw)² / Σw²`. Do not report a weighted percentage without acknowledging that the effective n is smaller than the raw n.

#### 22.10.8 From findings to recommendations

Weak finding: “NPS is −7.7.”
Strong finding: “NPS is −7.7 (12 promoters, 15 detractors, 12 passives), so trial is not converting into advocacy. The access top-2-box is 36.8% versus efficacy 47.4%, suggesting that reimbursement friction is the most likely blocker. Recommendation: prioritise access support for the 7 trial prescribers and run a follow-up with the 15 detractors to identify the specific access failure.”

The strong version connects three numbers, offers a mechanism, and recommends an action with a target group.

**Three recommendations with evidence:**

1. **Protect the regular-prescriber base.** 8 regular prescribers generate 79.1% of TRx. Assign key-account support and monitor TRx retention monthly.
2. **Fix access before increasing awareness.** Access top-2-box is 36.8%, 10.5 points below efficacy. Audit reimbursement journeys for the 7 trial prescribers; set a 90-day access improvement target.
3. **Convert trial to regular.** Only 53.3% of current prescribers are regular. Interview the 7 trial prescribers to identify the conversion barrier; test a patient-support or prior-authorisation assistance programme.

Each recommendation has a number, a target group, and a measurable action. That is the standard the case interview rewards.

#### Check yourself — 22.10

1. How many raw rows, unique respondents, and analysis respondents are there?
2. What are the stage counts after cleaning?
3. What is the regular-prescriber share of TRx?
4. What are the unweighted and weighted top-2-box values for intent?
5. What is NPS, and what are the component counts?
6. What is the trial-to-regular conversion rate among current prescribers?
7. What is the access-vs-efficacy gap, and what does it imply?

*Answers in 22.13.*

---

## Part K — Building the analytical report from raw data

### 22.11 The last mile: from spreadsheet to decision

An assessment does not end when the formula works. It ends when you can answer: **What did you find, why does it matter, and what should the business do?** The report is the deliverable.

#### 22.11.1 The seven-step workflow

1. **Define the decision.** Who is the audience, and what will they do differently? “VP Commercial wants to know where to deploy the H2 field force.”
2. **Inspect the data.** Row count, grain, date range, key fields, missing values, obvious duplicates.
3. **Clean and QA.** Normalise keys; document exclusions; build the QA block.
4. **Analyse.** Start with the headline KPI, then segment, then trend, then root cause.
5. **Synthesise.** Write the three findings that change the decision. Not ten.
6. **Visualise.** One chart per finding; choose the chart that matches the comparison.
7. **Write.** Headline, context, findings, recommendation, risks, next steps, appendix.

#### 22.11.2 The one-page memo skeleton

```text
HEADLINE: [Finding + number + implication]
CONTEXT: [Business problem, scope, period, data sources]
METHOD: [What was analysed; key definitions; exclusions]
FINDINGS:
  1. [Finding with number]
  2. [Finding with number]
  3. [Finding with number]
RECOMMENDATION: [Action, owner, timing, expected impact]
RISKS / LIMITATIONS: [What could change the answer]
NEXT STEPS: [Specific, measurable, dated]
APPENDIX: [Data quality, formulas, definitions]
```

#### 22.11.3 Insight-led titles

A title should state the finding, not the topic.

| Weak title | Strong title |
|---|---|
| “Revenue analysis” | “H1 revenue is 98.8% of target, but CardioX is 5.9% short” |
| “Regional performance” | “North contributes 32% of revenue but only 28% of rows hit target” |
| “Survey results” | “Access, not efficacy, is the conversion bottleneck (36.8% vs 47.4% top-2-box)” |
| “NPS” | “NPS is −7.7: trial is not converting into advocacy” |
| “Recommendations” | “Protect the 8 regular prescribers who generate 79% of TRx” |

The strong title passes the “so what?” test without the reader having to read the chart.

#### 22.11.4 Chart choice

| Comparison | Best chart | Avoid |
|---|---|---|
| Revenue by region × product | Clustered bar/column | Pie with 12 slices |
| Trend over time | Line | Column with 3D effects |
| Achievement vs target by row | Bullet chart or dumbbell | Two pie charts |
| Share of total (≤5 categories) | Stacked bar or pie | 3D pie |
| Distribution of Likert responses | Stacked bar (100%) | Radar chart |
| NPS components | Stacked bar with promoters/passives/detractors | Speedometer gauge |
| Correlation | Scatter | Dual-axis line |
| Ranking | Sorted horizontal bar | Alphabetical bar |

**Misleading visuals to avoid:**

- Truncated y-axis on a line chart.
- Dual axes with different scales that imply a relationship.
- 3D charts that distort area.
- Pie charts with more than five slices.
- Colour scales that imply precision (e.g., 12 shades for 12 rows).
- A “before/after” chart with different time windows.

#### 22.11.5 Cell-linked narrative

Instead of hard-coding numbers in a memo, link them:

```excel
="H1 revenue was ₹"&TEXT(SUM(tblSales[Revenue]),"#,##0")
&" ("&TEXT(SUM(tblSales[Revenue])/SUM(tblSales[TargetRevenue]),"0.0%")
&" of target), with Q2 growth of "&TEXT(Q2/Q1-1,"0.0%")&" QoQ."
```

This produces: “H1 revenue was ₹2,683,320 (98.8% of target), with Q2 growth of 23.5% QoQ.”

**Caution:** `TEXT` rounds for display, but the underlying value remains precise. If a number is used in a decision, show the same rounding everywhere. Do not write “₹26.8 lakh” in the title and “₹26.83 lakh” in the table unless you explain the rounding.

#### 22.11.6 Worked one-page memo: NovaCure H1 2026

**HEADLINE:** H1 revenue is ₹26.83 lakh, 98.8% of target. Growth is carried by OncoZ (+71.0% QoQ), while CardioX misses target at 94.1%.

**CONTEXT:** Fictional NovaCure portfolio (CardioX, NeuroY, OncoZ) across four regions, January–June 2026. 72 region-product-month records. Target and revenue in ₹; scale is illustrative.

**METHOD:** Cleaned 72 rows; no duplicates; two OncoZ launch rows have no target and are excluded from achievement; three blank calls cells are reported as a data-quality issue. Achievement = revenue / target at the aggregate level.

**FINDINGS:**

1. **Total revenue ₹26.83 lakh vs ₹27.17 lakh target (98.8%).** Q2 revenue ₹14.83 lakh, +23.5% QoQ. The trend is positive, but the target gap is real.
2. **OncoZ is the growth engine.** ₹8.43 lakh in H1 (31.4% of total), Q2 +71.0% QoQ, 104.8% of target. North OncoZ is the largest single cell at ₹2.81 lakh.
3. **CardioX is the drag.** ₹10.51 lakh (39.2% of total) but only 94.1% of target. Seven rows are below 90%; six of the seven are South/West CardioX rows, mainly January–April, and the seventh is North NeuroY in February. The worst row is West CardioX in February at 85.6%.
4. **North is large but not efficient.** North contributes 32.2% of revenue but only 27.8% of its rows hit target. East has the highest hit rate (58.8%) and 104.5% achievement, partly because its targets are easier.

**RECOMMENDATION:** Protect OncoZ momentum (supply, access, key accounts); diagnose the seven CardioX below-90% rows with the South and West leads (pricing, coverage, access, competition); rebalance H2 targets so stretch is consistent across regions. Do not cut CardioX investment solely because its achievement is lower; it still generates the largest revenue pool.

**RISKS / LIMITATIONS:** Short six-month window; no causal data; targets are set with region/product stretch and row-level noise, so achievement comparisons are directional; revenue scale is fictional; blank calls and blank targets are known data-quality issues.

**NEXT STEPS:** (1) Root-cause interviews with South/West CardioX leads by 15 July. (2) Weekly OncoZ supply/access tracker. (3) Reconcile targets with finance before H2 reallocation. (4) Re-run the analysis after the next month close.

#### 22.11.7 Assumption log

Every analysis has assumptions. Write them down:

| Assumption | Why it matters | How to test |
|---|---|---|
| Revenue = units × price | If revenue includes discounts/rebates, the price–volume decomposition is wrong | Reconcile to finance gross-to-net |
| Target is a fair stretch | Achievement comparisons depend on target-setting | Compare target growth to market growth |
| Blank target = not applicable | Treating it as zero would distort achievement | Confirm with finance |
| Calls are allocated to product lines | Revenue per call depends on the denominator | Confirm call-reporting rules |
| Survey weights are design weights | Weighted estimates depend on the weighting scheme | Compare weighted vs unweighted |
| Field window is 15 Jan–31 Mar | Including out-of-window responses changes the base | Confirm with the research lead |

#### 22.11.8 Final QA before submission

- [ ] Every number in the memo ties to a cell in the workbook.
- [ ] Every percentage has a stated denominator.
- [ ] Every chart has a title that states the finding, not the topic.
- [ ] Every exclusion (duplicate, out-of-window, blank target) is documented.
- [ ] The QA block shows actual vs expected counts.
- [ ] The recommendation names an owner, a timing, and a measurable outcome.
- [ ] The limitations are honest and specific.
- [ ] The workbook opens without errors, broken links, or `#REF!`.
- [ ] The file is saved with a clear name and version.

#### Check yourself — 22.11

1. Turn “regional performance” into an insight-led title using the sales numbers.
2. Why is “average price” ambiguous, and how do you fix it?
3. What are the three components of a strong recommendation?
4. Which chart would you use for region × product revenue, and why?
5. Name three limitations of the H1 memo.

*Answers in 22.13.*

---

## Part L — Timed drills (questions only)

### 22.12 Progressive drills

Do these with a timer and a blank workbook. The answer key starts in 22.13. Do not scroll ahead.

#### Drill 1 — Lookup and aggregation (20 minutes)

Use `excel_advanced_sales.csv`, `excel_product_master.csv`, `excel_region_master.csv`.

1. Add `TherapeuticArea` and `LaunchYear` to the sales table using XLOOKUP. How many rows are Oncology?
2. Add `RegionalManager`. Which manager owns the region with the highest revenue, and what is that revenue?
3. Build a unique key `Region|Product|Date`. How many duplicate keys are there?
4. Return North CardioX May-26 revenue using a two-way INDEX/MATCH. State the value.
5. Calculate Q2 revenue and Q2 achievement (revenue ÷ target). 
6. Calculate OncoZ Q1→Q2 growth and OncoZ’s share of Q2 revenue.
7. Count the blank `Calls` cells. What is the average North Q2 calls value (excluding blanks)?
8. List the top three region-product cells by revenue and their share of total revenue.

**Deliverable:** one sheet with the eight answers, formulas visible, and a one-line interpretation for questions 5–8.

#### Drill 2 — Pivot and reconciliation (25 minutes)

Use `excel_advanced_sales.csv`, `excel_recon_ledger.csv`, `excel_recon_finance.csv`.

1. Build a PivotTable: Region rows, Product columns, Sum of Revenue. Reproduce the full matrix.
2. Add North CardioX as a percentage of North and as a percentage of total. State both.
3. Add a running total by month. State the cumulative value at June.
4. Reconcile the ledger and finance files: row counts, totals, net variance.
5. Classify every exception: missing, extra, duplicate, amount mismatch. Apply a ₹5 tolerance.
6. Write the variance bridge from ledger total to finance total.
7. Write a three-sentence message to the finance manager. It must distinguish net variance from gross exceptions.

**Deliverable:** pivot sheet + reconciliation sheet with a bridge table.

#### Drill 3 — Survey analysis (30 minutes)

Use `excel_advanced_survey.csv`.

1. Clean the file: remove exact duplicates, standardise regions/specialties, apply the 15 Jan–31 Mar 2026 field window. State raw rows, unique rows, and analysis rows.
2. Produce the stage frequency table.
3. Calculate total monthly TRx and the regular-prescriber share of TRx.
4. Produce the top-2-box table for Intent, Access, and Efficacy — unweighted and weighted. State the base for each.
5. Calculate NPS: promoters, passives, detractors, NPS value.
6. Build the specialty × stage cross-tab.
7. Write three findings, each with a number and a “so what.”
8. Write three recommendations, each with a target group, an action, and a metric.

**Deliverable:** a clean data sheet and a one-page findings/recommendations sheet.

#### Drill 4 — What-if and sensitivity (20 minutes)

Use the June CardioX numbers from the sales file: 1,640 units, ₹126 price, ₹206,640 revenue.

1. Confirm June CardioX revenue and units.
2. Use Goal Seek to find the units needed for ₹223,171 revenue. State the lift.
3. Calculate revenue at −10% price and +12% volume. State the percentage change vs base.
4. Calculate the revenue break-even volume lift for a 10% price cut. Then calculate the profit break-even lift assuming a fixed variable cost of ₹56.70 per unit.
5. Build a two-variable table for price change (−10%, −5%, 0, +5%, +10%) × volume change (−5%, 0, +5%, +10%, +15%). Report the four corner values and the base.
6. Calculate the probability-weighted H2 revenue under 30% worst, 50% base, 20% best. State whether it is above or below the base case and why.
7. Write a 3-sentence recommendation on whether to cut price by 10%.

**Deliverable:** a scenario sheet with the two-way table and a short recommendation.

---

## Part M — Answer key

### 22.13 Answers: section checks and drills

#### 22.1 Lookups

1. XLOOKUP: `=XLOOKUP("NeuroY",tblProduct[Product],tblProduct[LaunchYear])` → **2021**. INDEX-MATCH: `=INDEX(tblProduct[LaunchYear],MATCH("NeuroY",tblProduct[Product],0))` → **2021**.
2. `"CHECK"`. It is better than `#N/A` because it is readable, countable, and does not look like a broken workbook. The correct follow-up is to count and fix the unmapped key.
3. It returns the **first matching row** (January, ₹50,400), not the Q2 sum. The Q2 total requires SUMIFS; the answer is **₹180,774**.
4. `=INDEX(data,MATCH("North|CardioX",keys,0),MATCH("May-26",months,0))` → **₹58,338**.
5. Before `TRIM`: **1 failure** (the `"OncoZ "` row). After `TRIM`: **0 failures**.

#### 22.2 Aggregation

1. Q2 revenue **₹1,482,870**; Q1→Q2 growth **23.53%**.
2. North + South = **₹1,589,238**.
3. CardioX Q2 **₹563,790**; share of Q2 **38.02%**.
4. Weighted average CardioX price **₹122.15**; simple average **₹122.00**.
5. Blank calls: **3**.
6. Rows at/above target: **28 of 70 = 40.0%**.
7. Distinct products **3**; distinct regions **4**.
8. Top three region-product cells: **33.52%** of total.

#### 22.3 Error handling

1. Use `IFNA` for lookups: it traps only “not found” and lets real formula errors surface. Use `IFERROR` only when you can name every error you intend to hide.
2. `=IF([@TargetRevenue]="","",[@Revenue]/[@TargetRevenue])`.
3. **2** blank targets (OncoZ East and West in January). Exclude them from achievement; do not treat them as zero.
4. It hides every error type, including a broken reference or a logic mistake. The workbook looks clean while the analysis is wrong.

#### 22.4 Dynamic arrays

1. **East, North, South, West** (4 regions).
2. **7 rows** below 90%: six CardioX rows (three West, three South) and one North NeuroY row.
3. **North OncoZ, Jun-26, ₹70,080**.
4. It references the entire spill range produced by a dynamic array formula.
5. `=ROWS(FILTER(...))`.

#### 22.5 Text and dates

1. `OncoZ`.
2. `=TEXTBEFORE(TEXTAFTER(A2,"-"),"-")` → `120`; legacy `=MID(A2,FIND("-",A2)+1,FIND("-",A2,FIND("-",A2)+1)-FIND("-",A2)-1)` → `120`.
3. April–March fiscal: **FY2027-Q1**. Calendar: **Q2**. State which calendar you are using.
4. **1** response (RS-030).
5. `TODAY()` changes every day, so the report is not reproducible. Use a fixed as-of date for auditability.

#### 22.6 PivotTables

1. Calculated-field achievement = **98.76%**.
2. North CardioX = **38.98% of North** and **12.56% of total**.
3. Cumulative through June = **₹2,683,320**.
4. `=GETPIVOTDATA("Revenue",$A$3,"Region","North","Product","CardioX")` → **₹337,134**.
5. A calculated field sums revenue and target, then divides. An average of row-level percentages gives each row equal weight, so small rows distort it.

#### 22.7 Conditional formatting and validation

1. Select `A2:H73`; formula `=AND($G2<>"",$F2/$G2<0.9)`.
2. `=COUNTIF($A$2:$A$43,$A2)>1` highlights all occurrences. `=COUNTIF($A$2:$A2,$A2)>1` highlights the second and later occurrences only.
3. `=COUNTIF($A$2:$A$100,$A2)=1`.
4. `INDIRECT` is volatile and breaks if the named range changes. Use a filtered list (`FILTER`) or a data model.
5. Pasting bypasses validation, and existing data may already be invalid. Validation prevents some future errors; QA detects current ones.

#### 22.8 Reconciliation

1. Ledger **₹318,600**; finance **₹321,251**; net variance **+₹2,651**.
2. Missing: **INV-1004 ₹7,080** and **INV-1017 ₹5,460**.
3. Duplicate: **INV-1009 ₹11,040**.
4. Bridge: `318,600 − 7,080 − 5,460 + 8,400 + 11,040 + 250 + 1 − 4,500 = 321,251`.
5. Within ₹5: **INV-1012 (+₹1)**. Material: **INV-1006 (+₹250)** and **INV-1015 (−₹4,500)**.
6. The net variance is small because missing, extra, duplicate, and mismatch items nearly offset. Gross exceptions are **₹36,731**, so a net-only check would miss material process failures.
7. Trial stage is **7 vs target 8**; the shortfall is RS-030 (out of field window).

#### 22.9 What-if

1. **1,771.2 units** (~1,772), a **+8.05%** lift.
2. **+11.11%**.
3. **+22.22%** (~2,005 units) with fixed variable cost of ₹56.70.
4. **₹208,293**, **+0.80%** vs base.
5. **₹1,229,921**, below the base case because the downside is weighted 30% vs 20% upside.
6. Volume ±10% = ±₹20,664; price ±5% = ±₹10,332; variable cost ±10% = ±₹9,299.

#### 22.10 Survey

1. **42 raw, 40 unique, 39 analysis**.
2. **8 / 8 / 8 / 7 / 8** (unaided, aided, consideration, trial, regular).
3. **272 / 344 = 79.07%**.
4. **35.90% unweighted; 34.98% weighted** (n = 39).
5. **NPS −7.69**: 12 promoters, 12 passives, 15 detractors.
6. **8 / 15 = 53.33%**.
7. **36.84% vs 47.37% = −10.53 percentage points**. Access is the weaker driver and a plausible conversion barrier.

#### 22.11 Reporting

1. “North contributes 32.2% of H1 revenue but only 27.8% of its rows hit target.”
2. “Average price” is ambiguous between simple and volume-weighted. Report the weighted price (₹122.15) and state the weighting.
3. Recommendation + evidence/expected impact + implementation (owner, timing, metric). A recommendation without an owner and a measure is an opinion.
4. Clustered bar/column: it compares 12 region-product cells without implying a time trend.
5. Short six-month window; no causal data; target-setting noise; fictional scale; blank calls/targets.

---

#### Drill 1 — Lookup and aggregation: answers

**1. Oncology rows.** Add:

```excel
=XLOOKUP([@Product],tblProduct[Product],tblProduct[TherapeuticArea],"CHECK")
=XLOOKUP([@Product],tblProduct[Product],tblProduct[LaunchYear],"CHECK")
```

OncoZ is Oncology for **24 rows** (4 regions × 6 months). Check: `COUNTIF(Product,"OncoZ") = 24`.

**2. Regional manager.** Add `=XLOOKUP([@Region],tblRegion[Region],tblRegion[RegionalManager])`. Revenue by region:

| Region | Manager | Revenue |
|---|---|---:|
| North | Meera Kapoor | ₹864,954 |
| South | Arjun Iyer | ₹724,284 |
| East | Farah Khan | ₹481,200 |
| West | Nikhil Desai | ₹612,882 |

**Meera Kapoor** owns the largest region (North, ₹864,954).

**3. Duplicate keys.** `=SUMPRODUCT(--(COUNTIF(Key,Key)>1))` → **0**. Each `Region|Product|Date` combination is unique.

**4. Two-way lookup.** With a matrix of 12 region-product rows and 6 month columns:

```excel
=INDEX(data, MATCH("North|CardioX", keys, 0), MATCH("May-26", months, 0))
```

→ **₹58,338**.

**5. Q2.** Revenue **₹1,482,870**; target **₹1,509,330**; achievement **98.25%**.

**6. OncoZ.** Q1 **₹311,040**; Q2 **₹531,840**; growth **+70.99%**. Q2 share **35.87%**.

**7. Calls.** Blank cells **3**. North Q2 calls: 9 rows, total 885, average **98.33**.

**8. Top three cells.** North CardioX ₹337,134; North OncoZ ₹281,280; South CardioX ₹280,944. Combined **₹899,358**, which is **33.52%** of ₹2,683,320.

**Interpretation:** North is the largest region, but OncoZ is growing fastest; the top three cells are one-third of revenue. The commercial team should protect North/OncoZ momentum while diagnosing CardioX in South and West (six of the seven below-90% rows).

---

#### Drill 2 — Pivot and reconciliation: answers

**1. Region × product matrix.**

| Region | CardioX | NeuroY | OncoZ | Total |
|---|---:|---:|---:|---:|
| North | 337,134 | 246,540 | 281,280 | 864,954 |
| South | 280,944 | 209,580 | 233,760 | 724,284 |
| East | 192,510 | 148,050 | 140,640 | 481,200 |
| West | 240,882 | 184,800 | 187,200 | 612,882 |
| **Total** | **1,051,470** | **788,970** | **842,880** | **2,683,320** |

**2. North CardioX shares.** Of North: `337,134 / 864,954 = 38.98%`. Of total: `337,134 / 2,683,320 = 12.56%`.

**3. Running total.**

| Month | Revenue | Cumulative |
|---|---:|---:|
| Jan | 378,000 | 378,000 |
| Feb | 397,470 | 775,470 |
| Mar | 424,980 | 1,200,450 |
| Apr | 454,770 | 1,655,220 |
| May | 487,800 | 2,143,020 |
| Jun | 540,300 | 2,683,320 |

June cumulative **₹2,683,320**, equal to the grand total. If it does not tie, the pivot source is missing rows.

**4. Reconciliation totals.** Ledger 20 rows, ₹318,600. Finance 20 rows, ₹321,251. Net variance **+₹2,651**.

**5. Exception classification.**

| Exception | Invoice | Amount | Count |
|---|---|---:|---:|
| Missing in finance | INV-1004 | ₹7,080 | 1 |
| Missing in finance | INV-1017 | ₹5,460 | 1 |
| Extra in finance | INV-1021 | ₹8,400 | 1 |
| Duplicate in finance | INV-1009 | ₹11,040 | 1 |
| Mismatch > tolerance | INV-1006 | +₹250 | 1 |
| Mismatch > tolerance | INV-1015 | −₹4,500 | 1 |
| Mismatch within tolerance | INV-1012 | +₹1 | 1 |

Common unique IDs: 18. Exact matches: 15. Amount differences: 3.

**6. Bridge.**

```
318,600
− 7,080 − 5,460
+ 8,400
+ 11,040
+ 250 + 1 − 4,500
= 321,251
```

**7. Client message (model).**

> “The ledger and finance extract differ by ₹2,651 net, but that understates the issue: two invoices totalling ₹12,540 are missing from finance, one duplicate invoice of ₹11,040 is present, one extra invoice of ₹8,400 is present, and two invoices have material amount differences (+₹250 and −₹4,500). Please confirm the duplicate before payment, trace the two missing invoices, and approve the treatment of the two material mismatches. The ₹1 difference on INV-1012 is within our ₹5 rounding tolerance.”

This message does four things: states the net, quantifies the gross, assigns actions, and applies the tolerance policy.

---

#### Drill 3 — Survey analysis: answers

**1. Cleaning.**

| Step | Rows | Action |
|---|---:|---|
| Raw file | 42 | As provided |
| Remove exact duplicates | 40 | RS-007 and RS-023 each appeared twice |
| Apply field window 15 Jan–31 Mar 2026 | 39 | Exclude RS-030 (2025-12-15) |

This is the analysis base. Report the three numbers in the memo: 42 → 40 → 39.

**2. Stage frequency.**

| Stage | n | Share |
|---|---:|---:|
| Unaided awareness | 8 | 20.5% |
| Aided awareness | 8 | 20.5% |
| Consideration | 8 | 20.5% |
| Trial prescribed | 7 | 17.9% |
| Regular prescriber | 8 | 20.5% |
| **Total** | **39** | **100.0%** |

**3. TRx.**

- Total monthly TRx: **344**.
- Trial: 72 (20.9%).
- Regular: 272 (79.1%).
- Regular share: **79.07%**.

**4. Top-2-box.**

| Item | n | Top-2 unweighted | Top-2 weighted | Mean unweighted | Mean weighted |
|---|---:|---:|---:|---:|---:|
| Intent | 39 | 35.90% | 34.98% | 3.10 | 3.11 |
| Access | 38 | 36.84% | 37.68% | 3.03 | 3.05 |
| Efficacy | 38 | 47.37% | 47.94% | 3.45 | 3.47 |

Bases: Intent n = 39 (all analysis respondents); Access n = 38 (RS-019 value 7 excluded); Efficacy n = 38 (RS-020 `"N/A"` excluded). Weighted and unweighted differ by ≤1.5 points, so the conclusion is robust to the illustrative weights.

**5. NPS.**

| Segment | n | Share |
|---|---:|---:|
| Promoters (9–10) | 12 | 30.8% |
| Passives (7–8) | 12 | 30.8% |
| Detractors (0–6) | 15 | 38.5% |
| **NPS** | | **−7.69** |

Weighted NPS: **−6.81**.

**6. Cross-tab.**

| Specialty | Unaided | Aided | Consideration | Trial | Regular | Total |
|---|---:|---:|---:|---:|---:|---:|
| Cardiologist | 2 | 2 | 2 | 2 | 2 | 10 |
| Neurologist | 2 | 2 | 2 | 1 | 2 | 9 |
| Oncologist | 2 | 2 | 2 | 2 | 2 | 10 |
| Primary Care | 2 | 2 | 2 | 2 | 2 | 10 |
| **Total** | **8** | **8** | **8** | **7** | **8** | **39** |

The design was balanced; the only shortfall is one trial-stage neurologist (RS-030). Do not interpret 1–2 respondent cell differences.

**7. Three findings with “so what.”**

1. **Access is the weakest driver.** Access top-2-box is 36.84% vs efficacy 47.37%, a 10.53-point gap. *So what:* if access is the constraint, more awareness spend will not convert; the team should audit reimbursement and prior-authorisation journeys first.
2. **TRx is concentrated.** Eight regular prescribers are 20.5% of the sample but 79.07% of TRx. *So what:* protect this cohort with key-account support; do not spread field resources evenly across all 39 respondents.
3. **Advocacy is negative.** NPS is −7.69 (12 promoters, 15 detractors). *So what:* trial is not creating advocacy; the 15 detractors need a follow-up to identify the specific access or efficacy disappointment.

**8. Three recommendations.**

1. **Protect the regular-prescriber base.** Target: 8 regular prescribers. Action: key-account plan and monthly TRx retention review. Metric: ≥90% TRx retention over 90 days.
2. **Fix access for trial prescribers.** Target: 7 trial prescribers. Action: access audit and patient-support/prior-authorisation assistance. Metric: access top-2-box from 36.8% to ≥45% in the next wave.
3. **Convert trial to regular.** Target: 7 trial prescribers. Action: barrier interviews and a conversion pilot. Metric: trial-to-regular conversion from 53.3% to ≥65% in 6 months.

---

#### Drill 4 — What-if and sensitivity: answers

**1. June CardioX.** Units **1,640**; price **₹126**; revenue **₹206,640**.

**2. Goal Seek.** Target **₹223,171** (8% above base). Required units:

```
223,171 / 126 = 1,771.2 → ~1,772 units
Lift = (1,771.2 − 1,640) / 1,640 = 8.05%
```

Goal Seek returns the fractional 1,771.2; round up for an operational plan.

**3. −10% price, +12% volume.**

```
1,640 × 126 × 0.90 × 1.12 = 208,293
Change vs 206,640 = +0.80%
```

**4. Break-evens.**

- Revenue break-even: `1/0.9 − 1 = 11.11%`.
- Profit break-even with fixed variable cost ₹56.70:
  - Current contribution/unit = 126 − 56.70 = 69.30.
  - Current contribution = 1,640 × 69.30 = 113,652.
  - New price = 113.40; new contribution/unit = 113.40 − 56.70 = 56.70.
  - Break-even units = 113,652 / 56.70 = 2,004.4 → ~2,005.
  - Lift = (2,004.4 − 1,640) / 1,640 = 22.22%.

**5. Two-way table corners.**

| Price ↓ / Volume → | −5% | 0% | +5% | +10% | +15% |
|---|---:|---:|---:|---:|---:|
| **−10%** | 176,677 | 185,976 | 195,275 | 204,574 | 213,872 |
| **−5%** | 186,493 | 196,308 | 206,123 | 215,939 | 225,754 |
| **0%** | 196,308 | 206,640 | 216,972 | 227,304 | 237,636 |
| **+5%** | 206,123 | 216,972 | 227,821 | 238,669 | 249,518 |
| **+10%** | 215,939 | 227,304 | 238,669 | 250,034 | 261,400 |

Corners: top-left **₹176,677**; top-right **₹213,872**; bottom-left **₹215,939**; bottom-right **₹261,400**. Base **₹206,640**.

**6. Scenario weighted value.**

- Worst: `206,640 × 0.92 × 6 = 1,140,653`.
- Base: `206,640 × 6 = 1,239,840`.
- Best: `206,640 × 1.08 × 6 = 1,339,027`.
- Weighted: `0.30×1,140,653 + 0.50×1,239,840 + 0.20×1,339,027 = 1,229,921`.

The weighted value is **below the base** because the downside probability (30%) is larger than the upside probability (20%).

**7. Recommendation (model).**

> “A 10% price cut requires +11.1% volume just to hold revenue and +22.2% volume to hold contribution at a fixed ₹56.70 variable cost. At a plausible +12% volume response, revenue rises only 0.8% and contribution falls. Do not cut price by 10% without evidence that elasticity exceeds the profit break-even; test a smaller 3–5% access-led discount or a patient-support programme instead.”

The recommendation states the threshold, the evidence, and the alternative. It does not claim to know the true elasticity.

---

## Part N — The 45-minute practical assessment (questions only)

### 22.14 Assessment: NovaCure Q2 launch review

**This is a practice simulation, not an actual Trinity assessment.** The company, products, and numbers are fictional. The format is a reasonable simulation of a 30–45 minute Excel assessment; it is not a leaked paper.

#### Scenario

You are an Associate Analyst at **NovaCure** (fictional), a mid-size pharmaceutical company. The commercial team has just closed Q2 2026 for its oncology launch, **OncoZ**. The VP Commercial has 45 minutes before a review and wants a one-page readout:

> “Is the launch on track, where is it weak, and what should we do in Q3?”

You have a raw territory extract (`data/excel_assessment_raw.csv`), the product master, and the region master. The raw extract was assembled by three different field teams; it has not been cleaned.

#### Data dictionary

| Column | Meaning |
|---|---|
| `TerritoryID` | Territory code (N-01, S-01, E-01, W-01) |
| `Region` | North, South, East, West (dirty in the raw file) |
| `RepName` | Territory representative |
| `Product` | CardioX, NeuroY, OncoZ (dirty in the raw file) |
| `Month` | Apr-26, May-26, Jun-26 (dirty in the raw file) |
| `Units` | Units sold; negative = return; blank = missing |
| `UnitPrice` | ₹ per unit |
| `Revenue` | ₹ revenue; one value is stored as text |
| `TargetRevenue` | ₹ target; one blank |
| `Calls` | Detailing calls |

Masters: `excel_product_master.csv` (Product, TherapeuticArea, LaunchYear); `excel_region_master.csv` (Region, RegionalManager, HQ).

#### Instructions

1. Work in a new workbook. Keep the raw data on a sheet called `Raw`; never edit it.
2. Create `Clean`, `QA`, `Pivot`, and `Memo` sheets.
3. Use formulas where practical; hard-coded answers lose marks if the underlying data changes.
4. State every assumption and every exclusion.
5. You may use PivotTables, dynamic arrays, or SUMIFS. The method is your choice; the numbers and the reasoning are what matter.
6. Suggested time plan: **10 min clean, 5 min QA, 10 min aggregate, 8 min pivot, 7 min insight, 5 min memo.**
7. Total: **25 marks**.

#### Part 1 — Data quality (5 marks)

1. How many raw rows are there? How many exact duplicate rows? After removing duplicates, how many clean rows? *(1 mark)*
2. List the data-quality issues in the raw file. For each, state the treatment and the risk if untreated. *(1 mark)*
3. Which row has negative units and negative revenue? What does it represent, and how should it be treated in the analysis? *(1 mark)*
4. Which row has blank units but valid revenue? What is the implied unit count at the stated price? *(1 mark)*
5. Which row has a blank target? How should it be treated in the achievement calculation? *(1 mark)*

#### Part 2 — Enrichment (3 marks)

1. Add `TherapeuticArea` and `LaunchYear` using a lookup. How many Oncology rows are there? *(1 mark)*
2. Add `RegionalManager`. How many rows does each manager own? *(1 mark)*
3. How many lookup failures remain after cleaning? What would you do if the answer were not zero? *(1 mark)*

#### Part 3 — Core KPIs (5 marks)

1. Total revenue. *(1 mark)*
2. Total units. State whether the return is included and whether the blank-units row is included or recovered. *(1 mark)*
3. Total target revenue and overall achievement. *(1 mark)*
4. How many rows are at or above target? What percentage of rows with a target is that? *(1 mark)*
5. Average revenue per clean row. *(1 mark)*

#### Part 4 — Segment analysis (5 marks)

1. Build the Region × Product revenue matrix. *(2 marks)*
2. Calculate achievement by region. *(1 mark)*
3. Calculate achievement by product. *(1 mark)*
4. Identify the largest region-product cell and the top three cells’ share of total revenue. *(1 mark)*

#### Part 5 — Trend (3 marks)

1. Monthly revenue for April, May, June. *(1 mark)*
2. Month-over-month growth for April→May and May→June, and April→June. *(1 mark)*
3. Which product drove the June change? Quantify. *(1 mark)*

#### Part 6 — Root cause and insight (2 marks)

1. Why is NeuroY achievement below target? Which region/rows are the biggest drag? *(1 mark)*
2. Why might East be above target? Is it execution, target-setting, or both? What cautions apply? *(1 mark)*

#### Part 7 — Recommendation and communication (2 marks)

1. Write three recommendations. Each must have a target group, an action, an owner, and a metric. *(1 mark)*
2. Create one chart for the VP readout and justify it. Write a one-sentence insight-led headline for the memo. *(1 mark)*

#### Deliverables

- `Clean` sheet with 36 rows and documented treatments.
- `QA` block with actual vs expected counts.
- `Pivot` sheet with the region × product matrix and achievement tables.
- `Memo` sheet with a headline, three findings, three recommendations, risks, and next steps.
- One chart on the `Memo` sheet: a clustered bar of revenue by region with product series, or a bullet chart of achievement by product.

**Stop writing at 45 minutes.** A complete, defensible 80% answer beats an incomplete 100% attempt. Answers and marking guide: 22.15.

---

### 22.15 Assessment answer key and marking guide

#### Part 1 — Data quality (5 marks)

**1.1 Row count.** Raw: **37 data rows**. Exact duplicate: **1** (a copy of South OncoZ May-26). Clean: **36 rows**.

**1.2 Data-quality issues.**

| # | Issue | Row(s) | Treatment | Risk if untreated |
|---:|---|---|---|---|
| 1 | Exact duplicate | South OncoZ May-26, appended at bottom | Remove one | Double-counts revenue, units, and target |
| 2 | Region `"north "` | North CardioX May-26 | `TRIM` → North | Creates a fifth “region”; totals split |
| 3 | Region `"N"` | North OncoZ Jun-26 | Map to North | Lost row in North totals |
| 4 | Region `"south"` | South CardioX Apr-26 | Map to South | Lost row in South totals |
| 5 | Product `"OncoZ "` | East OncoZ May-26 | `TRIM` → OncoZ | Lookup/pivot misses the row |
| 6 | Rep `"Rahul Menon "` | West CardioX May-26 | `TRIM` | Duplicate rep name in reports |
| 7 | Revenue text `"31,560"` | South CardioX Jun-26 | Convert to number | SUM ignores it; understates revenue |
| 8 | Month `"May 2026"` | North NeuroY May-26 | Standardise to May-26 | Lost month in trend |
| 9 | Month `"2026-06"` | South OncoZ Jun-26 | Standardise to Jun-26 | Lost month in trend |
| 10 | Negative units/revenue | West NeuroY Jun-26: −12 units, −₹2,520 | Retain as return; flag; annotate | Distorts achievement and rankings if unexplained |
| 11 | Blank units | East CardioX Jun-26; revenue ₹21,600 | Recover 180 units at ₹120, or exclude from unit totals and document | Understates units by 180 |
| 12 | Blank target | West OncoZ Apr-26 | Exclude from achievement; report | `#DIV/0!` or false zero |

**Affected rows:** 12 (including the duplicate). After removing the duplicate, 36 clean rows; 11 rows carry a documented issue.

**1.3 Negative row.** West NeuroY Jun-26: `Units = −12`, `Revenue = −₹2,520` at ₹210/unit. It represents a **return/credit**. Retain it in revenue (it is a real economic event), but flag it: it makes the row’s achievement −13.78% versus an ₹18,285 target. Do not use that percentage in a ranking without a footnote. Check whether it is a one-off return or a recurring credit.

**1.4 Blank units.** East CardioX Jun-26: revenue ₹21,600 at ₹120/unit → **180 units**. Either recover the units (`Revenue / UnitPrice`) and mark them as derived, or exclude the row from unit totals. Do not treat blank as zero. Unit totals: **4,223** as reported (including the −12 return, excluding the blank); **4,403** if the 180 implied units are recovered.

**1.5 Blank target.** West OncoZ Apr-26. Exclude from achievement (do not divide by zero, do not treat as zero). The product/region achievement is calculated on the 35 rows with a target. Report the exclusion.

#### Part 2 — Enrichment (3 marks)

**2.1 Oncology rows.** OncoZ → Oncology, LaunchYear 2024. There are **12 OncoZ rows** (4 regions × 3 months). Check: `COUNTIF(Product,"OncoZ") = 12`.

**2.2 Manager rows.** Each region has 9 rows (3 products × 3 months): Meera Kapoor 9, Arjun Iyer 9, Farah Khan 9, Nikhil Desai 9.

**2.3 Lookup failures.** After trimming, **0**. If not zero: list the failing keys with `FILTER`, check the master for missing/duplicate entries, correct the source, and document the fix. Do not silently return blank or zero.

#### Part 3 — Core KPIs (5 marks)

| KPI | Value | Working |
|---|---:|---|
| Total revenue | **₹858,390** | Sum of cleaned revenue, including the −₹2,520 return and the ₹21,600 blank-units row |
| Total units | **4,223** | Reported units, including −12, excluding blank; 4,403 if 180 implied units recovered |
| Total target | **₹877,216** | 35 rows with a target |
| Overall achievement | **97.85%** | 858,390 / 877,216 |
| Rows at/above target | **14 of 35 = 40.0%** | Row-level comparison |
| Average revenue per row | **₹23,844.17** | 858,390 / 36 |

**Sanity checks:** Revenue = units × price for every numeric row (tolerance ₹1); region totals sum to 858,390; product totals sum to 858,390; target totals sum to 877,216.

#### Part 4 — Segment analysis (5 marks)

**4.1 Region × product matrix (₹).**

| Region | CardioX | NeuroY | OncoZ | Total |
|---|---:|---:|---:|---:|
| North | 105,000 | 72,660 | 106,080 | 283,740 |
| South | 87,480 | 61,740 | 88,800 | 238,020 |
| East | 59,880 | 43,470 | 53,280 | 156,630 |
| West | 75,000 | 34,440 | 70,560 | 180,000 |
| **Total** | **327,360** | **212,310** | **318,720** | **858,390** |

**4.2 Achievement by region.**

| Region | Revenue | Target | Achievement |
|---|---:|---:|---:|
| North | 283,740 | 292,374 | 97.05% |
| South | 238,020 | 248,722 | 95.70% |
| East | 156,630 | 150,749 | 103.90% |
| West | 180,000 | 185,371 | 97.10% |
| **Total** | **858,390** | **877,216** | **97.85%** |

**4.3 Achievement by product.**

| Product | Revenue | Target | Achievement |
|---|---:|---:|---:|
| CardioX | 327,360 | 338,375 | 96.74% |
| NeuroY | 212,310 | 244,356 | 86.89% |
| OncoZ | 318,720 | 294,485 | 108.23% |
| **Total** | **858,390** | **877,216** | **97.85%** |

**4.4 Largest cell and concentration.** Largest cell: **North OncoZ, ₹106,080**. Top three cells: North OncoZ ₹106,080, North CardioX ₹105,000, South OncoZ ₹88,800 = **₹299,880**, or **34.94%** of total.

#### Part 5 — Trend (3 marks)

**5.1 Monthly revenue.**

| Month | Revenue |
|---|---:|
| Apr-26 | 272,520 |
| May-26 | 287,640 |
| Jun-26 | 298,230 |

**5.2 Growth.** Apr→May **+5.55%**; May→Jun **+3.68%**; Apr→Jun **+9.43%**.

**5.3 June driver.** June vs May by product:

| Product | May | Jun | Change | % |
|---|---:|---:|---:|---:|
| CardioX | 104,160 | 118,080 | +13,920 | +13.36% |
| NeuroY | 79,800 | 54,390 | −25,410 | −31.84% |
| OncoZ | 103,680 | 125,760 | +22,080 | +21.30% |
| **Total** | **287,640** | **298,230** | **+10,590** | **+3.68%** |

OncoZ and CardioX grew, but NeuroY fell sharply. The headline “Q2 grew 9.4%” hides the NeuroY deterioration.

#### Part 6 — Root cause and insight (2 marks)

**6.1 NeuroY.** Overall **86.89%** (₹212,310 vs ₹244,356). Region breakdown:

| Region | Revenue | Target | Achievement |
|---|---:|---:|---:|
| North | 72,660 | 80,651 | 90.09% |
| South | 61,740 | 63,941 | 96.56% |
| East | 43,470 | 44,560 | 97.55% |
| West | 34,440 | 55,204 | 62.39% |

West is the biggest drag. It includes the **−₹2,520 return** in June. Without the return, West revenue is ₹36,960 and achievement is **66.95%**; the return explains about 4.6 percentage points of the 62.39% figure, but the remaining gap is genuine underperformance across April and May (₹18,270 and ₹18,690; with a quarterly target of ₹55,204, even before the return the quarter is well short). North is also below target at 90.09%, driven by April (₹24,360) and June (₹23,310). Do not attribute the whole NeuroY gap to the return.

**6.2 East.** East achievement is **103.90%** (₹156,630 vs ₹150,749). CardioX (106.66%) and OncoZ (106.46%) are above target; NeuroY is 97.55%. The target-setting in this fictional dataset gives East an easier stretch (0.97 multiplier) and row-level noise, so part of the overachievement is target-setting, not execution. The correct conclusion is: “East is above target, but confirm the target basis before rewarding the team; the mix is strong in CardioX and OncoZ, with NeuroY slightly below.”

#### Part 7 — Recommendation and communication (2 marks)

**7.1 Three recommendations.**

1. **West NeuroY recovery plan.** Target: West NeuroY (62.39% achievement; 66.95% excluding the return). Action: access/coverage audit, key-account review, confirm whether the −₹2,520 is a one-off return. Owner: West Regional Manager. Timing: 30/60/90 days. Metric: NeuroY West achievement ≥90% by September.
2. **Protect OncoZ momentum.** Target: OncoZ (108.23%) and the South OncoZ cell (96.41%). Action: supply/access check, key-account expansion, replicate North’s OncoZ playbook. Owner: OncoZ Brand Lead. Timing: Q3. Metric: maintain ≥100% achievement; South OncoZ ≥100%.
3. **Fix commercial data capture.** Target: blank units (1 row, 180 implied units), blank target (1 row), formatting variants. Action: validation lists, required-field checks, monthly QA block. Owner: Commercial Operations. Timing: before the July close. Metric: zero blank units/targets and zero unmapped keys next cycle.

**7.2 Chart and headline.**

- **Chart:** a clustered bar chart of revenue by region with product series, or a bullet chart of achievement by product. The clustered bar answers “where is the revenue concentrated?”; the bullet answers “which product is off target?”. For a one-chart readout, use the **clustered bar** because the VP’s first question is “where is the business?”
- **Headline:** “Q2 revenue is ₹8.58 lakh, 97.9% of target: OncoZ outperforms at 108.2%, but NeuroY at 86.9% — led by West — is the gap to close.”

#### Marking guide (25 marks)

| Part | Criteria | Marks |
|---|---|---:|
| 1 | Duplicate identified and removed; 36 rows | 1 |
| 1 | Issue list covers formatting, blanks, return, target; treatments correct | 1 |
| 1 | Return identified and treated as a flagged economic event | 1 |
| 1 | Blank units identified; 180 implied units recovered or excluded with rationale | 1 |
| 1 | Blank target excluded from achievement, not treated as zero | 1 |
| 2 | Oncology 12 rows; correct lookups | 1 |
| 2 | Manager rows 9 each | 1 |
| 2 | Lookup failures 0; escalation logic stated | 1 |
| 3 | Revenue ₹858,390 | 1 |
| 3 | Units ₹4,223 (or ₹4,403 with recovery) and treatment explained | 1 |
| 3 | Target ₹877,216; achievement 97.85% | 1 |
| 3 | 14/35 = 40.0% rows at/above target | 1 |
| 3 | Average revenue per row ₹23,844.17 | 1 |
| 4 | Full region × product matrix correct | 2 |
| 4 | Region achievement correct | 1 |
| 4 | Product achievement correct | 1 |
| 4 | Largest cell and top-3 share correct | 1 |
| 5 | Monthly revenue correct | 1 |
| 5 | MoM and Apr–Jun growth correct | 1 |
| 5 | June product driver quantified | 1 |
| 6 | NeuroY root cause: West, return vs underlying, North | 1 |
| 6 | East caution: target-setting vs execution | 1 |
| 7 | Three recommendations with target/action/owner/metric | 1 |
| 7 | Chart choice justified; insight-led headline | 1 |
| **Total** | | **25** |

**Bands:**

| Score | Interpretation | Next action |
|---:|---|---|
| 22–25 | Strong; ready for a real timed assessment with polish | Redo under 40 minutes; focus on speed |
| 18–21 | Solid; data quality or root-cause specificity is the gap | Repeat Drills 1–3; rewrite the memo |
| 14–17 | Developing; mechanics are there but the insight is thin | Re-read 22.2, 22.8, 22.10; redo the assessment |
| <14 | Fundamentals need another pass | Re-read 22.1–22.9 and redo the section checks before retrying |

#### Model one-page memo

> **Q2 revenue is ₹8.58 lakh, 97.9% of target: OncoZ outperforms at 108.2%, but NeuroY at 86.9% — led by West — is the gap to close.**
>
> **Context.** NovaCure’s Q2 2026 extract covers 36 clean territory-product-month rows across four regions and three products. One duplicate was removed; one return, one blank-units row, one blank-target row, and several formatting variants were documented.
>
> **Findings.**
> 1. Total revenue is ₹858,390 vs a ₹877,216 target (97.85%). Only 14 of 35 rows with a target are at or above target (40.0%). Growth is positive but below plan.
> 2. OncoZ is the growth engine: ₹318,720 (37.1% of revenue), 108.23% of target. North OncoZ is the largest cell at ₹106,080. June OncoZ rose ₹22,080 (+21.30%) vs May.
> 3. NeuroY is the drag: ₹212,310 (24.7% of revenue), 86.89% of target. West NeuroY is 62.39% (66.95% excluding the ₹2,520 return), and North NeuroY is 90.09%. June NeuroY fell ₹25,410 (−31.84%) vs May.
> 4. East is above target at 103.90%, but its target stretch is easier; CardioX and OncoZ are strong there, NeuroY is 97.55%. Confirm the target basis before rewarding the result.
>
> **Recommendation.** (1) Launch a West NeuroY recovery plan with the West Regional Manager, targeting ≥90% achievement by September. (2) Protect OncoZ momentum with supply/access checks and a South OncoZ push (96.41%). (3) Fix blank units/targets and formatting through validation and a monthly QA block, owned by Commercial Operations.
>
> **Risks.** Three-month window; no causal or competitor data; target noise; a one-off return; 180 implied units; fictional scale. The return is flagged rather than removed, so the West number is deliberately conservative.
>
> **Next steps.** Confirm the return; root-cause interviews with West and North NeuroY leads; weekly OncoZ tracker; re-run after the July close.

This memo is not the only correct answer. A strong candidate may prioritise differently — for example, CardioX’s 96.74% and its ₹327k revenue base — provided the recommendation is evidence-based, owned, and measurable.

---

### 22.16 Common advanced-Excel mistakes (and the fix)

| Mistake | Why it happens | Fix |
|---|---|---|
| Using XLOOKUP for a total | Confusing lookup with aggregation | Ask “one row or many?”; use SUMIFS for totals |
| Hard-coding numbers in the memo | Faster in the moment | Link to cells; use TEXT for formatting |
| Ignoring duplicates | The raw file looks plausible | COUNTIF key > 1; remove/document |
| Treating blank target as zero | `#DIV/0!` avoidance | `IF(target="","",...)`; report exclusion |
| Denominator drift | Different totals for different questions | Write the denominator next to every % |
| Averaging percentages | Intuitive but wrong | Weight by the denominator; use aggregate sums |
| Hiding errors with IFERROR | Makes the sheet look clean | Use IFNA for joins; visible CHECK flags |
| Pivoting dirty data | Pivot is fast | Clean/QA first; refresh; verify grand total |
| Forgetting to refresh | Source changed, pivot did not | Alt+F5; check row count and total |
| Percentage vs percentage point | Casual language | Say “10.5 percentage points,” not “10.5%” |
| Over-interpreting a small survey base | Percentages look precise | Show n; suppress or caveat n < 30 |
| Truncated y-axis | Makes small changes look huge | Start at zero for bars; justify any truncation |
| One chart with two unrelated axes | Looks sophisticated | Use two charts or index both series to 100 |
| Recommendation without owner/metric | Feels complete but is not actionable | Add target, action, owner, timing, metric |
| No QA block | Time pressure | Build the QA block first; it saves time later |
| Not stating assumptions | Assumed obvious | Assumption log in the appendix |

#### Final checklist before you close the workbook

- [ ] Raw data preserved on a separate sheet.
- [ ] Clean sheet has 36 rows (assessment) and documented treatments.
- [ ] QA block: row count, duplicates, blanks, invalid categories, lookup failures.
- [ ] Every percentage has a denominator and a base.
- [ ] Every lookup has a visible failure flag.
- [ ] Pivot refreshed; grand total ties to the clean total.
- [ ] No hard-coded numbers in the memo.
- [ ] One chart per finding; no misleading axes.
- [ ] Recommendation has an owner, timing, and metric.
- [ ] Limitations are specific and honest.
- [ ] File named clearly; version noted.

#### Where to go next

- Re-take [15-mock-excel-assessment-45-min.md](15-mock-excel-assessment-45-min.md) after this module; compare your score and time.
- Use [19-errors-mistakes-and-traps.md](19-errors-mistakes-and-traps.md) and [28-weakness-based-practice-and-error-log.md](28-weakness-based-practice-and-error-log.md) to log every mistake from the drills.
- Move to [20-power-bi-complete-practical-guide.md](20-power-bi-complete-practical-guide.md) for the dashboard version of the same data. The Excel work here is the semantic layer; Power BI is the presentation layer.
- When the case-study modules are added (`21`), use the same discipline: define the decision, QA the data, quantify, segment, root-cause, recommend.

---

*End of module 22. All datasets are fictional and were generated for practice. The assessment format is a practice inference, not an official Trinity paper.*
