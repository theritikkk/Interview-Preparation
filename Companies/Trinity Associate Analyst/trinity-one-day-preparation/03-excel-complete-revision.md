# 03 — Excel Complete Revision

> **Learning objectives:** write every formula the assessment can ask for from memory; clean a dirty dataset; build a PivotTable; read a chart; avoid the classic silent-error traps.
> **Estimated study time:** 75 min hands-on (20 min syntax scan + 45 min Mock B in [`10`](10-mock-assessment-and-excel-test.md) + 10 min check).
> **Priority:** **MUST KNOW** everything in §§3.3–3.4 and §3.7. Candidate-reported signal: Excel is the most heavily tested area (lookups, conditional aggregation, pivots) — **CANDIDATE-REPORTED**, treat as directional only.

The Excel assessment is **30–45 min (CONFIRMED)**; format unknown (**PRACTICE** assumption: a small messy dataset + tasks like Mock B). Work in a real spreadsheet — reading formulas is not practice.

---

## §3.1 Practice dataset (memorize the shape, not the values)

Paste into a sheet named `Data` starting at A1. Every question in §3.9 refers to it.

| Rep | Region | Product | TRx | Target | Calls |
|---|---|---|---|---|---|
| R-01 | North | CardioX | 1100 | 1000 | 38 |
| R-02 | North | NeuroY | 640 | 700 | 30 |
| R-03 | South | CardioX | 1350 | 1250 | 42 |
| R-04 | South | OncoZ | 300 | 320 | 22 |
| R-05 | East | CardioX | 980 | 950 | 34 |
| R-06 | East | NeuroY | 520 | 560 | 28 |
| R-07 | West | CardioX | 1240 | 1200 | 40 |
| R-08 | West | OncoZ | 260 | 300 | 20 |

**Master table** (sheet `Master`): CardioX → Cardiovascular → 2019 · NeuroY → Neurology → 2021 · OncoZ → Oncology → 2023.

## §3.2 The core functions (syntax → example → result → use → classic mistake)

| Function | Syntax | Example | Result here | Use when | Classic mistake |
|---|---|---|---|---|---|
| SUM | `SUM(range)` | `=SUM(D2:D9)` | 6,390 | Totals | Including the header in a text column → no error, silent 0 |
| AVERAGE | `AVERAGE(range)` | `=AVERAGE(E2:E9)` | 785 | Central tendency | AVERAGE ignores blanks — count may not be what you think |
| COUNT | `COUNT(range)` | `=COUNT(D2:D9)` | 8 | Count *numbers only* | Using COUNT on text (→ 0) when you want COUNTA |
| COUNTA | `COUNTA(range)` | `=COUNTA(A2:A9)` | 8 | Count non-empty cells | Counting formatting-only cells as "filled" |
| MIN / MAX | `MIN(range)` / `MAX(range)` | `=MAX(D2:D9)` | 1,350 | Extremes / range checks | Ignoring hidden rows manually — functions do include hidden rows |
| IF | `IF(test, true_val, false_val)` | `=IF(D2>=E2,"On","Below")` | "On" | Logic branch | Forgetting quotes around text results |
| AND / OR | `AND(c1,c2,…)` | `=IF(AND(D2>=E2,F2>=35),"A","B")` | "A" | Multi-condition logic | Nesting conditions in the wrong order (put the strictest first) |
| IFS | `IFS(t1,v1,t2,v2,TRUE,else)` | `=IFS(D2/E2>=1.1,"Exceeds",D2/E2>=0.9,"On track",TRUE,"Below")` | "Exceeds" | Banding (avoids nested IF) | Missing the final `TRUE` catch-all → #N/A |
| IFERROR | `IFERROR(value, fallback)` | `=IFERROR(D2/E2,"")` | – | Suppress #DIV/0! etc. | Hiding real errors you should fix |

## §3.3 Conditional aggregation — the heart of the Excel test (MUST KNOW cold)

**Mind the argument ORDER — SUMIFS takes the *sum* range first; SUMIF takes the *criteria* range first.**

| Function | Syntax (memorize) | Example | Result here |
|---|---|---|---|
| SUMIF | `SUMIF(criteria_range, criteria, sum_range)` | `=SUMIF(B:B,"North",D:D)` | **1,740** |
| SUMIFS | `SUMIFS(sum_range, crit_range1, crit1, …)` | `=SUMIFS(D:D,C:C,"CardioX")` | **4,670** |
| COUNTIF | `COUNTIF(range, criteria)` | `=COUNTIF(F:F,">=35")` | **3** |
| COUNTIFS | `COUNTIFS(range1, crit1, …)` | `=COUNTIFS(B:B,"East",D:D,">500")` | **2** (R-05, R-06) |
| AVERAGEIFS | `AVERAGEIFS(avg_range, crit_range1, crit1, …)` | `=AVERAGEIFS(D:D,B:B,"East")` | **750** |

Rules: text criteria need quotes (`">=35"`, `"North"`); wildcards `*` `?` work in text (`"Card*"`); to count blanks use `COUNTBLANK`; criteria can reference cells (`">="&G1`).

## §3.4 Lookups (MUST KNOW — practice all three)

| Method | Syntax | Example | Result |
|---|---|---|---|
| XLOOKUP (modern; Excel 365/2021+) | `XLOOKUP(value, lookup_array, return_array, [if_not_found])` | `=XLOOKUP(C2,Master!$A$2:$A$4,Master!$C$2:$C$4,"CHECK")` for R-04 | **2023** |
| VLOOKUP (legacy but tested) | `VLOOKUP(value, table, col_index, FALSE)` | `=VLOOKUP(C2,Master!$A$2:$C$4,2,FALSE)` | "Oncology" |
| INDEX-MATCH (most flexible) | `INDEX(return_range, MATCH(value, lookup_range, 0))` | `=INDEX(Master!$B$2:$B$4,MATCH(C2,Master!$A$2:$A$4,0))` | "Oncology" |

**Why XLOOKUP is better:** looks left, no column counting, built-in "not found". **Why INDEX-MATCH still wins cases:** works in any version. **VLOOKUP traps:** forgetting `FALSE` (approximate match returns wrong values silently), column index shifts when columns are inserted, cannot look left.

**Lookup failure check (attention-to-detail points):** `=COUNTIF(H2:H9,"#N/A")` or wrap with `IFNA(...,"CHECK")` and `COUNTIF(...,"CHECK")`. Expected failures **0** — and the more interesting drill: leave one product with a stray space (`" CardioX"`) and show the count becomes 1. Cleaning is what makes lookups work.

## §3.5 Relative vs absolute references (30 seconds to master)

`A1` = relative (changes when pasted) · `$A$1` = absolute (frozen) · `$A1` / `A$1` = mixed (freeze column / row only).
Rule: freeze lookup tables and criteria cells: `Master!$A$2:$C$4`, `$G$1`. **Mistake:** forgetting `$` when copying `=D2/E2` style formulas across a table or dragging a criteria cell down.

## §3.6 Cleaning, sorting, filtering, conditional formatting

| Task | Recipe | Watch out |
|---|---|---|
| Remove extra spaces | `=TRIM(A2)` | TRIM removes regular spaces, **not** non-breaking spaces (CHAR(160)) — use `SUBSTITUTE(A2,CHAR(160)," ")` first |
| Standardize case | `=PROPER(TRIM(A2))` | PROPER capitalizes after every space; `north` / `N` → fix with nested IF or lookup |
| Text-stored numbers ("1,240") | `=VALUE(SUBSTITUTE(TRIM(A2),",",""))` | Left-aligned numbers are text — a telltale sign; `ISTEXT()` confirms |
| Remove duplicates | Data → Remove Duplicates (select all columns) or `=COUNTIFS($A$2:A2,A2,$F$2:F2,F2)>1` flag | Two *identical* rows in a real dataset are often real duplicates; verify before deleting |
| Blanks | Keep blanks where "not reported" is meaningful; use `COUNTBLANK`, `IF(cell="",...)`. For numeric math exclude them | Don't blanket-replace blanks with 0 — it tilts AVERAGE and margins |
| Sort | Data → Sort (multi-level) | Sort by the metric *after* computing it; saves the visual rework |
| Filter | Ctrl+Shift+L | Filters hide rows; SUM still counts hidden rows (use SUBTOTAL 109 if needed) |
| Conditional formatting | Home → Conditional Formatting; formula rules use the **top-left cell** with relative refs | Rule applies via relative formulas: `=$D2<$E2` red-fills only the current row |

## §3.7 PivotTables (MUST KNOW — 4 clicks, 4 questions)

**Build:** select data → Insert → PivotTable → set Rows / Columns / Values. **Read it as a matrix:** rows = one dimension, columns = the second, values = SUM of metric.
- **Second metric:** drag TRx to Values again → Show Values As → **% of Grand Total**.
- **Sort:** right-click row header → Sort → Largest to Smallest (by the value).
- **Interpretation questions always asked:** largest cell, largest row total, largest column total, concentration (one cell's % of total).
- **Watch out:** refresh after data changes (right-click → Refresh); blanks appear as "(blank)" — clean first.

## §3.8 Charts — make and read

Create: select region + metric → Insert → **clustered column** for cross-sections, **line** for time series. Name the chart with an insight, not the axes (weak: "TRx by Product"; strong: "CardioX = 73% of TRx — concentration risk"). Full chart-selection rules: [`05` §5.8](05-power-bi-tableau-and-data-visualization.md).
Read: check axis scale/units first, then magnitude, then compare only comparable bars, then state finding + implication.

---

## §3.9 Practical questions (answer these in a real spreadsheet; 30 min)

**Q1.** Total TRx.
**Q2.** Total TRx for North.
**Q3.** Total TRx for CardioX.
**Q4.** How many reps made ≥ 35 calls?
**Q5.** Average TRx of the East region.
**Q6.** Achievement = TRx ÷ Target. Which rep has the highest achievement %?
**Q7.** Using the Master table and XLOOKUP: launch year of OncoZ (as used in row R-04).
**Q8.** How many reps are below 90% achievement?
**Q9.** A raw cell contains `" 1,240 "` (text). Write one formula that returns the number **1240**.
**Q10.** A column chart of TRx by product shows CardioX = 4,670 of 6,390 total. Write a one-line insight-led title and one business implication.

### Answer key

| Q | Expected | Formula / method |
|---|---|---|
| Q1 | **6,390** | `=SUM(D2:D9)` |
| Q2 | **1,740** | `=SUMIF(B:B,"North",D:D)` (R-01 1100 + R-02 640) |
| Q3 | **4,670** | `=SUMIF(C:C,"CardioX",D:D)` |
| Q4 | **3** | `=COUNTIF(F:F,">=35")` → R-01 (38), R-03 (42), R-07 (40) |
| Q5 | **750** | `=AVERAGEIFS(D:D,B:B,"East")` → (980+520)/2 |
| Q6 | **R-01, 110.0%** | All: R-01 110.0 · R-02 91.4 · R-03 108.0 · R-04 93.8 · R-05 103.2 · R-06 92.9 · R-07 103.3 · R-08 86.7 |
| Q7 | **2023** | `=XLOOKUP(C5,Master!$A$2:$A$4,Master!$C$2:$C$4,"CHECK")` |
| Q8 | **1** | R-08 only (86.7%); `=COUNTIFS(F9… ` — better: compute achievement column then `=COUNTIF(G2:G9,"<0.9")` |
| Q9 | **1240** | `=VALUE(SUBSTITUTE(TRIM(A2),",",""))` |
| Q10 | e.g. *"CardioX carries 73% of TRx (4,670/6,390) — protect it"* | Implication: field effort on the other products is diversification, but any CardioX share loss hits ~3/4 of the book |

## §3.10 Common mistakes checklist

1. `SUMIF` vs `SUMIFS` argument order (criteria-first vs sum-first).
2. VLOOKUP without `FALSE`.
3. Forgetting `$` on lookup tables when copying down.
4. Treating text-numbers (`"1,240"`, `" 1100 "`) as numbers — sums silently skip them.
5. `COUNT` vs `COUNTA` confusion.
6. Replace-blanks-with-zero tilting averages.
7. Deleting duplicates without checking they're truly identical rows.
8. Not stating **which rows you excluded** from a calculation (graders look for it).
9. Chart with a truncated axis (misleading) or a pie with 8 slices.
10. Reporting "share fell 0.4%" instead of "0.4 pp" (see [`02` §2.9](02-aptitude-reasoning-and-data-interpretation.md)).

**Next:** [04 — Statistics, survey & market research](04-statistics-survey-and-market-research.md) · **Practice:** [Mock B in 10](10-mock-assessment-and-excel-test.md)
