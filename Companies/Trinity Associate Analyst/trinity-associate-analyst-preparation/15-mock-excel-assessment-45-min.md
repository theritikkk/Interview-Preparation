# 15 — Mock Excel Assessment (45 Minutes)

**Purpose:** simulate the **30–45 minute Excel assessment** (CONFIRMED duration, current TGaS JD; format unknown — **PRACTICE INFERENCE**). You work in real Excel on a deliberately messy dataset.
**Prerequisites:** [04-excel-formulas-cheatsheet.md](04-excel-formulas-cheatsheet.md), [05-excel-practical-workbook-guide.md](05-excel-practical-workbook-guide.md).
**Data:** [`data/trinity_excel_mock.csv`](data/trinity_excel_mock.csv) (30 rows, real-world mess included).
**Time:** 45 minutes working + 30 minutes reviewing solutions.
**Rule:** you must complete edits in the spreadsheet. Reading formulas is not the assessment.

---

## Instructions

1. Open Excel → **Data → From Text/CSV** → select `data/trinity_excel_mock.csv` → Load (or paste the CSV and use Data → Text to Columns with a comma delimiter).
2. Create a sheet named **`Clean`** and do all cleaning there. Never work on the raw import.
3. Answer every question in a labelled cell (e.g., `Clean!K2`) so your work is auditable.
4. You may use any method (formulas, PivotTable, Power Query, filters) — but you must be able to explain it.
5. Time checkpoints: after **10 min** cleaning must be done; after **20 min** aggregations; after **30 min** lookups; after **38 min** pivot; last **7 min** chart + conclusion.

**Columns:** A `RepID`, B `RepName`, C `Region`, D `Specialty`, E `Product`, F `Month`, G `TRx`, H `TargetTRx`, I `Calls`.

---

## §15.1 Tasks

### Part 1 — Data cleaning (10 minutes)

**Task 1.1** Load the file and list every data-quality issue you find, with a count for each.
**Task 1.2** Produce a clean table where: Region contains only `North/South/East/West`; Product names and rep names have no stray spaces; TRx and TargetTRx are numeric; and the duplicated record is removed.
**Task 1.3** Report how many rows remain and how many values you had to fix or exclude.

### Part 2 — Aggregations (10 minutes)

**Task 2.1** Total TRx across all reps.
**Task 2.2** TRx by region (four numbers).
**Task 2.3** TRx by product (three numbers).
**Task 2.4** TRx by month (two numbers).
**Task 2.5** Average `Calls` by specialty (three numbers, 2 dp).

### Part 3 — Achievement analysis (8 minutes)

**Task 3.1** Add an `Achievement %` column using TRx ÷ TargetTRx, leaving a blank where either value is missing.
**Task 3.2** Count reps at/above 100% achievement and the average achievement, **stating which rows you excluded**.
**Task 3.3** Add a band column: `≥110% = "Exceeds"`, `90–109.9% = "On track"`, `<90% = "Below"`, blank if undefined.

### Part 4 — Lookups (7 minutes)

**Master table (paste into a sheet `Master`):**

| Product | Therapeutic Area | Launch Year |
|---|---|---|
| CardioX | Cardiovascular | 2019 |
| NeuroY | Neurology | 2021 |
| OncoZ | Oncology | 2023 |

**Task 4.1** Add a `Therapeutic Area` column to `Clean` using a lookup (any of VLOOKUP, XLOOKUP, or INDEX-MATCH). Cleaned product names must match the master exactly.
**Task 4.2** Count lookup failures; show the formula you used to count them.
**Task 4.3** Add a `Launch Year` column with the same lookup, and count how many Jan-26 rows belong to products launched before 2022.

### Part 5 — PivotTable (6 minutes)

**Task 5.1** Build a PivotTable: Rows = Region, Columns = Product, Values = Sum of TRx.
**Task 5.2** Add a second value showing % of grand total.
**Task 5.3** Sort regions by total TRx descending, and identify the largest single region-product cell.

### Part 6 — Chart and conclusion (4 minutes)

**Task 6.1** Build **one** chart that answers: "Which region-product combination carries the business?" State the chart type you chose and why.
**Task 6.2** Write an insight-led slide title for the chart (format: finding + number + so-what).
**Task 6.3** Write three conclusion bullets: main finding, risk, and one limitation of this dataset.

---
---

# §15.2 Solutions, expected outputs and validation checks

> Exact values below were computed independently and cross-checked; see also [Exercise 1 of file 05](05-excel-practical-workbook-guide.md), which uses the same file.

### Part 1 — Cleaning

**Task 1.1 — Issues and counts (verified)**

| # | Issue | Count | Detection |
|---|---|---|---|
| 1 | Exact duplicate row (R-014, both columns identical) | **1 row** | `=COUNTIFS($A$2:$A$31,A2,$F$2:$F$31,F2)>1` |
| 2 | Region variants: `north ` (lowercase + trailing space) and `N` | **2 rows** | `=LEN(C2)<>LEN(TRIM(C2))`; `=COUNTIF(allowed,C2)=0` |
| 3 | Product with leading space (` CardioX`) | **1 row** | Same TRIM test on column E |
| 4 | Rep name with trailing space (`Bela `) | **1 row** | TRIM test on column B |
| 5 | TRx stored as text (`"1,510"`) | **1 row** | `=SUMPRODUCT(--NOT(ISNUMBER(G2:G31)))` |
| 6 | Blank TRx | **1 row** (R-012) | `=COUNTBLANK(G2:G31)` |
| 7 | Blank TargetTRx | **1 row** (R-029) | `=COUNTBLANK(H2:H31)` |

**Task 1.2 — Key formulas** (paste in the first row of `Clean` and copy down):

```excel
Region:  =IF(OR(LOWER(TRIM(Raw!C2))="n",LOWER(TRIM(Raw!C2))="north"),"North",PROPER(TRIM(Raw!C2)))
Product: =TRIM(Raw!E2)
RepName: =TRIM(Raw!B2)
TRx:     =IF(TRIM(Raw!G2)="","",VALUE(SUBSTITUTE(TRIM(Raw!G2),",","")))
Target:  =IF(TRIM(Raw!H2)="","",VALUE(SUBSTITUTE(TRIM(Raw!H2),",","")))
DupFlag: =IF(COUNTIFS(Raw!$A$2:A2,A2,Raw!$F$2:F2,F2)>1,"REMOVE","")
```

Then filter `DupFlag = "REMOVE"` and delete those rows (or use Data → Remove Duplicates on all columns of the clean sheet).

**Task 1.3 — Rows:** 30 raw → **29 rows** after removing the duplicate. **Values fixed:** 5 cells (2 regions, 1 product, 1 name, 1 text-number). **Values excluded from calculations:** 1 blank TRx, 1 blank target (excluded only where the relevant calculation needs them).

**Validation:** duplicate flags = 1; `=COUNTA(Clean!A2:A30)` = 29; allowed-region violations = 0.

### Part 2 — Aggregations (verified)

| Task | Answer | Formula used |
|---|---|---|
| 2.1 Total TRx | **23,113** | `=SUM(G2:G30)` |
| 2.2 By region | North **7,615**; South **5,413**; West **5,175**; East **4,910** | `=SUMIFS(G:G,C:C,"North")` |
| 2.3 By product | CardioX **13,990**; NeuroY **6,455**; OncoZ **2,668** | `=SUMIFS(G:G,E:E,"CardioX")` |
| 2.4 By month | Jan-26 **11,688**; Feb-26 **11,425** | `=SUMIFS(G:G,F:F,"Feb-26")` |
| 2.5 Avg calls | Cardiologist **42.25**; Neurologist **33.67**; Oncologist **25.50** | `=AVERAGEIFS(I:I,D:D,"Cardiologist")` |

**Validation checks**
- Region sum = product sum = month sum = **23,113** (cross-foot check: `=SUM(regions)-SUM(products)` must equal 0).
- `=SUM(G:G)` on the clean sheet equals `=SUMIFS(...)` totals; if it does not, a criteria mismatch exists (usually a stray space that TRIM missed).

### Part 3 — Achievement

**Task 3.1 — Formula:** `=IF(OR(G2="",H2=""),"",G2/H2)` formatted as a percentage.

**Task 3.2 — Results (verified):**
- Reps with a valid TRx **and** target: **27** (excluded: R-012 blank TRx, R-029 blank target; R-014's duplicate row was already removed).
- Reps at/above 100%: **16**; below 100%: **11**.
- Average achievement: **101.1%**.
- Formulas: `=COUNTIF(J2:J30,">=1")`, `=COUNTIF(J2:J30,"<1")`, `=AVERAGE(J2:J30)` (AVERAGE ignores the "" text results).
- **State the exclusions** in your answer: "2 of 29 rows could not be assessed for achievement (one missing TRx, one missing target)."

**Task 3.3 — Bands:** use `IFS` (365/2019+) or nested IF:
```excel
=IF(J2="","",IFS(J2>=1.1,"Exceeds",J2>=0.9,"On track",TRUE,"Below"))
```
Band counts on this dataset (verified): **Exceeds (≥110%): 0; On track (90–109.9%): 26; Below (<90%): 1** — total **27**, which must equal the task 3.2 denominator. *(The absence of any "Exceeds" row is itself a finding worth stating: the top performers sit just above target, not materially above it.)*

**Extremes (for sanity):** highest achievement = R-004 (109.2%); lowest = R-006 (86.7%). R-015 is the top rep by absolute TRx (1,510) but only 104.1% of target — a good illustration of absolute-versus-relative performance.

### Part 4 — Lookups

**Task 4.1 — Approaches**
```excel
VLOOKUP:      =VLOOKUP($E2,Master!$A$2:$C$4,2,FALSE)
XLOOKUP:      =XLOOKUP($E2,Master!$A$2:$A$4,Master!$B$2:$B$4)
INDEX-MATCH:  =INDEX(Master!$B$2:$B$4,MATCH($E2,Master!$A$2:$A$4,0))
```
Expected: every row resolves — CardioX → **Cardiovascular**, NeuroY → **Neurology**, OncoZ → **Oncology**.

**Task 4.2 — Failure count:** `=COUNTIF(K2:K30,"#N/A")` or, better, wrap the lookup `=IFNA(VLOOKUP(...),"CHECK")` and count with `=COUNTIF(K2:K30,"CHECK")`. **Expected failures: 0 after cleaning** — and this is the point of the task: with the leading space left in the product name, exactly **1 lookup would fail**. Demonstrate both states if your time allows.

**Task 4.3 — Launch Year:** same lookup returning column 3. Jan-26 rows with launch year < 2022 → CardioX (2019): **7 rows**; NeuroY (2021): **4 rows** → **11 rows** total. *(Count with `=COUNTIFS(F:F,"Jan-26",M:M,"<2022")` where column M holds Launch Year.)*

### Part 5 — PivotTable

**Task 5.1–5.2 — Expected pivot (Sum of TRx):**

| Region | CardioX | NeuroY | OncoZ | Grand Total |
|---|---:|---:|---:|---:|
| North | 4,825 | 2,105 | 685 | **7,615** |
| South | 2,760 | 1,795 | 858 | **5,413** |
| West | 2,900 | 1,410 | 865 | **5,175** |
| East | 3,505 | 1,145 | 260 | **4,910** |
| **Grand Total** | **13,990** | **6,455** | **2,668** | **23,113** |

**Task 5.3 — Largest single cell:** **North × CardioX = 4,825 TRx** (20.9% of total). Largest row total: North (7,615 = 33.0%). Largest column total: CardioX (13,990 = 60.5%).

**Validation:** pivot grand total = 23,113 (equals the `SUMIFS` total); row totals match Part 2.2; column totals match Part 2.3. Use Value Field Settings → Show Values As → % of Grand Total for 5.2.

### Part 6 — Chart and conclusion

**Task 6.1 — Recommended chart:** a **clustered (or stacked) column chart** with Region on the category axis and one series per product, because the question is a two-dimensional comparison (region × product). A pie chart is wrong here (too many categories); a line chart implies time. Sort regions by total descending (North, South, West, East) so the eye reads rank.

**Task 6.2 — Example insight-led title:** *"North CardioX alone contributes 4,825 TRx (21% of total) — product concentration, not regional spread, is the headline."* (Weak version to avoid: "TRx by Region and Product".)

**Task 6.3 — Example conclusion bullets**
1. **Main finding:** CardioX is **60.5%** of total TRx (13,990/23,113), and North is the largest region (**33.0%**); the North-CardioX cell alone is **20.9%** of the business.
2. **Risk:** 11 of 27 assessable reps (40.7%) are below target — performance is concentrated in a few reps, and CardioX's dominance means any competitive event in that product hits the whole number.
3. **Limitation:** two rows had missing values and one duplicate existed; the dataset covers only two months and one quarter of reps' potential (no territory size, physician counts, or competitor activity), so it supports descriptive conclusions, not causal ones.

---

## §15.3 Scoring

| Points | Item |
|---|---|
| 3 | Cleaning correct (all 5 issue types found; 29 rows) |
| 3 | Aggregations correct and cross-footed (all four cuts = 23,113) |
| 2 | Achievement column + bands + exclusions stated |
| 2 | Lookups correct with a failure-count check |
| 2 | PivotTable correct with % of grand total |
| 2 | Chart type justified + insight-led title |
| 2 | Conclusion with finding, risk and limitation |
| 1 | Every answer in a labelled cell (auditability) |

**Total: 17.** Target ≥14 before the real Excel assessment. Anything you missed, redo on a fresh copy tomorrow with the timer running.

**Next:** [16-mock-case-study-with-dataset.md](16-mock-case-study-with-dataset.md)
