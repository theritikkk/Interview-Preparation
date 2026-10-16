# 04 — Excel Formula Cheatsheet and Lookup Mastery

**Purpose:** a practical, test-oriented formula reference. Every formula has syntax, argument meanings, a realistic example with expected output, when to use it, common errors, and an alternative.
**Prerequisites:** basic navigation of Excel (typing in cells, selecting ranges).
**Suggested study time:** 2–3 hours, but **type every formula** into Excel as you read. Reading alone will not stick.
**Assessment context:** the current process includes a **30–45 minute Excel assessment** (CONFIRMED, TGaS Campus JD). Candidate reports mention **VLOOKUP, XLOOKUP, INDEX and SUMIFS** as discussion topics (CANDIDATE-REPORTED). Nothing here is a leaked question.

---

## 4.0 The sample dataset used in all examples

Paste this into a sheet named `Reps`, starting at cell **A1**:

| | A | B | C | D | E |
|---|---|---|---|---|---|
| **1** | Rep ID | Region | Product | TRx | Target |
| **2** | R-001 | North | CardioX | 1250 | 1200 |
| **3** | R-002 | North | CardioX | 980 | 1050 |
| **4** | R-003 | South | NeuroY | 640 | 600 |
| **5** | R-004 | South | CardioX | 1420 | 1300 |
| **6** | R-005 | East | OncoZ | 260 | 300 |
| **7** | R-006 | West | NeuroY | 720 | 750 |
| **8** | R-007 | North | OncoZ | 355 | 340 |

Verified reference values for this block: SUM(D2:D8) = **5,625**; AVERAGE(D2:D8) = **803.57**; MAX = 1,420; MIN = 260.

A small lookup table on another sheet, `Products`:

| | A | B |
|---|---|---|
| **1** | Product | Therapeutic Area |
| **2** | CardioX | Cardiovascular |
| **3** | NeuroY | Neurology |
| **4** | OncoZ | Oncology |

---

## 4.1 Cell references — the part that silently breaks formulas

| Type | Example | Behaviour when copied | Use for |
|---|---|---|---|
| Relative | `A1` | Both column and row shift | Moving formulas calculated row-by-row |
| Absolute | `$A$1` | Never shifts ("locked") | Fixed constants, lookup tables, tax/discount rates |
| Mixed (column fixed) | `$A1` | Column locked, row shifts | Table keyed by a fixed column |
| Mixed (row fixed) | `A$1` | Row locked, column shifts | Header rows, growth factors across columns |

**Example:** `=D2/$D$9` — the numerator moves row by row, the denominator stays at the total in D9. Copying this down gives every row the correct share.

**Rule of thumb:** if you copy the formula anywhere, ask "which of the two addresses must not move?" Put `$` in front of exactly that part. Press **F4** (Windows) / **Cmd+T** (Mac, or Fn+F4 on some layouts) to cycle `A1 → $A$1 → A$1 → $A1`.

**Most common error:** writing `=D2/D9`, copying down, and getting `#DIV/0!` in D8 because D9 became D15 (empty).

---

## 4.2 Aggregation functions

### SUM
**Syntax:** `=SUM(number1, [number2], …)`
**Arguments:** numbers, or ranges containing numbers. Text and blanks are ignored.
**Example:** `=SUM(D2:D8)` → **5,625**
**Use when:** totals, market sizes, headcount weighting bases.
**Common errors:** summing a range that includes a total row (double counting); summing text-numbers (they are ignored silently — see TRIM/VALUE fixes below).
**Alternative:** `=SUBTOTAL(109, D2:D8)` — totals only *visible* rows after filtering. Use SUBTOTAL in filtered analysis; `=AGGREGATE(9, 5, D2:D8)` also ignores hidden rows **and** errors.

### AVERAGE
**Syntax:** `=AVERAGE(number1, …)`
**Example:** `=AVERAGE(D2:D8)` → **803.57** (5625/7)
**Use when:** the group is comparable and no outliers dominate. Otherwise prefer AVERAGEIF/weighted average or MEDIAN.
**Common errors:** assuming AVERAGE ignores blanks *without shrinking the denominator* — it does shrink (blanks are excluded), which is usually what you want but is a trap in completion-rate problems.
**Alternative:** `=AVERAGEIF()`, `=MEDIAN()`, or a manual weighted average `=SUMPRODUCT(D2:D8,E2:E8)/SUM(E2:E8)`.

### MIN / MAX
**Syntax:** `=MIN(range)`, `=MAX(range)` → **260** and **1,420** in this block.
**Use when:** range of a distribution, best/worst performer, sanity checks on ranges.
**Alternative:** `=SMALL(range,2)`, `=LARGE(range,2)` for second-highest/lowest.

### COUNT / COUNTA
**Syntax:** `=COUNT(range)` counts **numeric** cells only; `=COUNTA(range)` counts **non-empty** cells (numbers, text, errors).
**Example:** `=COUNT(D2:D8)` → **7**; `=COUNTA(A2:A8)` → **7**. If a row had the text `"N/A"`, COUNT stays 7 but COUNTA becomes 8 — that difference is a fast data-quality diagnostic.
**Common errors:** using COUNTA to count responses when blanks were entered as `" "` (space) — those count as non-empty. Clean with TRIM first.

---

## 4.3 Conditional aggregation — SUMIF/SUMIFS and friends

### SUMIF
**Syntax:** `=SUMIF(range, criteria, [sum_range])`
**Arguments:** `range` = cells tested; `criteria` = number, text, or expression (`">1000"`, `"North"`, `">="&F2`); `sum_range` = cells to add (defaults to `range`).
**Example:** `=SUMIF(C2:C8,"CardioX",D2:D8)` → **3,650** (1250 + 980 + 1420)
**Use when:** one condition only.
**Common errors:** (1) mismatched sizes between range and sum_range; (2) text criteria with wildcards unintentionally matching; (3) criteria like `">1200"` typed without quotes.
**Alternative:** SUMIFS with a single criterion is identical and easier to extend.

### SUMIFS
**Syntax:** `=SUMIFS(sum_range, criteria_range1, criteria1, [criteria_range2, criteria2], …)`
**Example:** `=SUMIFS(D2:D8, B2:B8, "North", C2:C8, "CardioX")` → **2,230** (1250 + 980)
**Use when:** two or more conditions (region × product, month × brand, segment × channel).
**Critical difference:** **SUMIFS puts the sum range first**; SUMIF puts it last. Mixing this up is the #1 SUMIFS error.
**Alternative:** `=SUMPRODUCT((B2:B8="North")*(C2:C8="CardioX")*D2:D8)` works in very old Excel versions and handles some array logic.

### AVERAGEIF / AVERAGEIFS
**Syntax:** `=AVERAGEIF(range, criteria, [average_range])`; `=AVERAGEIFS(average_range, criteria_range1, criteria1, …)`
**Examples:** `=AVERAGEIF(C2:C8,"CardioX",D2:D8)` → **1,216.67**; `=AVERAGEIFS(D2:D8,B2:B8,"North")` → **861.67** (2,585/3)
**Common errors:** expecting AVERAGEIF to ignore blank/zero cells included in the criteria range — it averages only matching, non-blank cells, and averaging a criterion that matches zero rows returns `#DIV/0!`.
**Alternative:** `=SUMIFS(...)/COUNTIFS(...)` when you need to show the numerator and denominator separately (recommended in an assessment so your work is auditable).

### COUNTIF / COUNTIFS
**Syntax:** `=COUNTIF(range, criteria)`, `=COUNTIFS(criteria_range1, criteria1, …)`
**Examples:** `=COUNTIF(B2:B8,"North")` → **3**; `=COUNTIFS(B2:B8,"North",C2:C8,"CardioX")` → **2**
**Use when:** counting reps, responses, territories, physicians meeting a threshold: `=COUNTIFS(D2:D8,">="&E2:E8)` — careful: that array-style comparison works in modern Excel but is not a normal COUNTIFS pattern; the safe version uses a helper column: `=COUNTIFS(F2:F8,">=100%")`.
**Common errors:** criteria with `>=` need quotes: `">=1200"`; combining a cell reference needs concatenation: `">="&F1`.

**SUMIF vs SUMIFS / COUNTIF vs COUNTIFS comparison**

| Aspect | SUMIF / COUNTIF | SUMIFS / COUNTIFS |
|---|---|---|
| Conditions | Exactly 1 | 1 or more (unlimited) |
| Argument order | criteria range, criteria, **[sum_range last]** | **[sum_range first]**, then criteria pairs |
| Excel version | All versions | Excel 2007+ (and Excel for Mac 2011+) |
| Best practice | Fine for quick single-condition checks | Use by default — extensible and readable |
| Typical assessment use | "How much did North sell?" | "How much CardioX did North sell in Feb-26?" |

---

## 4.4 Logical functions

### IF
**Syntax:** `=IF(logical_test, value_if_true, [value_if_false])`
**Example:** `=IF(D2>=E2,"At/Above target","Below target")` → **"At/Above target"** for R-001 (1,250 ≥ 1,200)
**Use when:** banding, flags, conditional labels (achievement, above/below average, risk tier).
**Common errors:** nesting beyond 3 levels without planning; forgetting that text results need quotes; using `=` for "greater than or equal" (`>=`).
**Alternative:** `IFS()` (Excel 2019+/365) for cleaner multi-band logic: `=IFS(D2/E2>=1.1,"Exceeds",D2/E2>=0.9,"On track",TRUE,"Below")`.

### AND / OR
**Syntax:** `=AND(condition1, condition2, …)` → TRUE only if all are TRUE. `=OR(...)` → TRUE if any is TRUE.
**Example:** `=IF(AND(D2>=1000,E2<=1200),"Priority rep","Monitor")`
**Common errors:** using AND/OR outside IF and expecting a sentence rather than TRUE/FALSE; deep nesting that becomes unreadable.
**Alternative:** multiply conditions in SUMPRODUCT for counting: `=SUMPRODUCT((D2:D8>=1000)*(E2:E8<=1200))`.

### IFERROR
**Syntax:** `=IFERROR(value, value_if_error)`
**Example:** `=IFERROR(VLOOKUP(A2,Products!$A$2:$B$4,2,FALSE),"Not found")`
**Use when:** lookups that may miss, divisions that may hit zero, `INDEX/MATCH` that may fail.
**Common errors:** hiding genuine data-quality problems — in an assessment, produce the error *and* a count of errors: `=COUNTIF(F2:F20,"Not found")`. Prefer a specific label like `"CHECK: not in master list"` over `""`.
**Alternative:** `=IFNA(...)` only traps `#N/A` (better for lookups); `=IFERROR` traps all errors.

---

## 4.5 Lookups — the core skill

### XLOOKUP (modern Excel: 365, Excel 2021+)
**Syntax:** `=XLOOKUP(lookup_value, lookup_array, return_array, [if_not_found], [match_mode], [search_mode])`
**Arguments:** `match_mode` — 0 exact (default), −1 exact or next smaller, 1 exact or next larger, 2 wildcard; `search_mode` — 1 first-to-last (default), −1 last-to-first, 2/−2 binary (sorted data).
**Example:** `=XLOOKUP("R-003",A2:A8,D2:D8)` → **640**
**Use when:** always, if available — no column counting, works left-to-right and right-to-left, returns multiple columns with one formula: `=XLOOKUP("R-003",A2:A8,B2:D8)` returns Region, Product, TRx.
**Common errors:** `lookup_array` and `return_array` must be the same length; using `-1`/`1` match modes accidentally (they produce approximate matches silently).
**Compatibility fallback (REQUIRED for older Excel):** `=INDEX(D2:D8,MATCH("R-003",A2:A8,0))` or `=VLOOKUP("R-003",A2:E8,4,FALSE)`.

### VLOOKUP
**Syntax:** `=VLOOKUP(lookup_value, table_array, col_index_num, [range_lookup])`
**Arguments:** `table_array` = the block to search (first column must contain the key); `col_index_num` = which column to return (counted from the table's first column); `range_lookup` = **FALSE for exact match** (TRUE/1 for approximate).
**Examples:**
- `=VLOOKUP("R-003",A2:E8,4,FALSE)` → **640**
- `=VLOOKUP("NeuroY",Products!$A$2:$B$4,2,FALSE)` → **"Neurology"**
- Approximate: `=VLOOKUP(1040,$J$2:$K$5,2,TRUE)` with a sorted band table (0→"Low", 1000→"Medium", 1200→"High") returns **"Medium"**.
**Use when:** the current ecosystem/colleague expects VLOOKUP, or you are working in a legacy workbook.
**Common errors:**
1. Omitting `FALSE` → approximate matching on unsorted data returns **wrong** values without any error. Approximate mode requires the first column sorted ascending.
2. `col_index_num` is out of range → `#REF!`; typed as text `"4"` → `#VALUE!`.
3. Inserting a column inside the table shifts `col_index_num` silently. Absolute references (`$A$2:$E$8`) plus INDEX/MATCH or XLOOKUP avoid this.
4. The key must be in the **first** column of `table_array`; VLOOKUP cannot look left.
5. Trailing spaces in the key cause `#N/A` — clean with `TRIM()`.

### INDEX and MATCH
**Syntax:** `=INDEX(array, row_num, [column_num])`, `=MATCH(lookup_value, lookup_array, [match_type])`
**Example:** `=INDEX(D2:D8, MATCH("R-003", A2:A8, 0))` → **640**
**Use when:** you need left lookups, dynamic column selection, or maximum robustness: `=INDEX(D2:D8,MATCH(1,(B2:B8="North")*(C2:C8="CardioX"),0))` (array-entered in legacy Excel) returns the first matching TRx.
**Common errors:** `match_type` 1 (approximate) requires sorted data; 0 is exact. INDEX row/column numbers are **relative to the array**, not to the sheet.
**Alternative:** XLOOKUP replaces most INDEX/MATCH patterns; INDEX/MATCH remains the compatibility champion and the standard answer if asked "what do you use when XLOOKUP isn't available?"

**Lookup comparison table**

| Aspect | VLOOKUP | XLOOKUP | INDEX-MATCH |
|---|---|---|---|
| Direction | Right only (key must be leftmost) | Any direction | Any direction |
| Column insertion risk | High (index shifts) | None | None |
| Default match | Approximate (if omitted) | Exact | Exact (with 0) |
| Return multiple columns | No | Yes | Via array |
| Return to the left | No | Yes | Yes |
| Legacy compatibility | Excellent | Poor (365/2021+ only) | Excellent |
| Readability | Medium | High | Medium |
| Assessment recommendation | Know it cold anyway | Use if available | Best fallback |

---

## 4.6 Rounding, dates, text and percentages

### ROUND / ROUNDUP / ROUNDDOWN / MROUND
**Syntax:** `=ROUND(number, num_digits)`; negative digits round left of the decimal (`=ROUND(2345,-2)` → 2,300).
**Examples:** `=ROUND(D2/E2*100,1)` → **104.2**; `=ROUND(5625/7,0)` → **804**
**Use when:** reporting only; keep raw values for calculations (rounding mid-chain compounds error).
**Common errors:** formatting to 2 decimals (display) and believing the value changed — `ROUND` changes the value, number formatting does not.
**Alternative:** `=TEXT(value,"0.0")` for display strings; `=CEILING(x,1)`/`=FLOOR(x,1)` for whole-unit constraints in sales-force math.

### Date functions
| Function | Syntax | Example | Output / use |
|---|---|---|---|
| TODAY | `=TODAY()` | `=TODAY()` | Current date (volatile) |
| YEAR/MONTH/DAY | `=YEAR(A2)` | `=YEAR("2026-02-14")` | 2026 — extract components for pivots |
| EOMONTH | `=EOMONTH(start_date, months)` | `=EOMONTH("2026-01-15",0)` | 31-Jan-2026 — month-end reporting |
| EDATE | `=EDATE(date, months)` | `=EDATE("2026-01-15",3)` | 15-Apr-2026 |
| DATEDIF | `=DATEDIF(start,end,"M")` | Tenure in months | "Y","M","D" — careful, not in autocomplete |
| NETWORKDAYS | `=NETWORKDAYS(start,end)` | Business days | Planning and SLA checks |
| DATE | `=DATE(2026,2,1)` | 01-Feb-2026 | Building period labels |
**Common error:** dates stored as text (left-aligned, no date arithmetic). Fix with `=DATEVALUE(TRIM(A2))`.

### LEFT / RIGHT / MID / LEN / TRIM
| Function | Syntax | Example | Output |
|---|---|---|---|
| LEFT | `=LEFT(text, n)` | `=LEFT("CardioX",6)` | Cardio |
| RIGHT | `=RIGHT(text, n)` | `=RIGHT("R-001",3)` | 001 |
| MID | `=MID(text, start, n)` | `=MID("R-001",3,3)` | 001 |
| LEN | `=LEN(text)` | `=LEN("CardioX ")` | 8 |
| TRIM | `=TRIM(text)` | `=TRIM(" North ")` | North |
**Use when:** cleaning codes, extracting IDs, standardising region names, detecting trailing spaces (`=LEN(A2)<>LEN(TRIM(A2))` flags dirty cells).
**Common errors:** assuming TRIM removes interior double spaces — TRIM *does* collapse interior multiples in Excel (unlike some expectations); it does **not** remove non-breaking spaces (ASCII 160) — use `=SUBSTITUTE(A2,CHAR(160)," ")`.

### Concatenation
**Syntax:** `=A2&" - "&C2` or `=CONCAT(A2:C2)` or `=TEXTJOIN(", ",TRUE,A2:A8)`
**Example:** `="Rep "&A2&" ("&B2&")"` → **Rep R-001 (North)**
**Use when:** building keys (`=TRIM(B2)&"|"&TRIM(C2)`), labels, and narrative strings.
**Common error:** numbers displaying oddly when concatenated (use `=TEXT(D2,"#,##0")`), and TEXTJOIN not existing in Excel 2016 or older.

### Percentage calculations
| Task | Formula | Example result |
|---|---|---|
| Share of total | `=D2/$D$9` formatted % | 1250/5625 = 22.2% |
| % change | `=(D2-E2)/E2` | (1250−1200)/1200 = 4.2% |
| Achievement | `=D2/E2` | 104.2% |
| Growth over 2 years (CAGR) | `=(End/Begin)^(1/n)-1` | — |
| Percentage of a target gap | `=(D2-E2)/E2` with IF flag | — |

---

## 4.7 Error reference

| Error | Typical cause | First fix |
|---|---|---|
| `#N/A` | Lookup value not found (often whitespace or type mismatch) | `TRIM`, check number-vs-text, add `IFNA` |
| `#VALUE!` | Text where a number is needed (`col_index_num` as text) | `VALUE()`, remove quotes |
| `#REF!` | Deleted referenced cells; index out of range | Undo, fix the index |
| `#DIV/0!` | Denominator is zero or blank (very common in share/% change) | `IFERROR(...,"")` or `IF(denominator=0,...)` |
| `#NAME?` | Misspelled function or missing `_xlfn.` prefix for newer functions in old Excel | Correct spelling; check version |
| `#####` | Column too narrow / negative date | Widen column; check date logic |
| Silent wrong answer | VLOOKUP without `FALSE`; SUMIFS with swapped arguments | Recheck arguments, use exact match |

---

## 4.8 Version compatibility (important — do not assume XLOOKUP exists)

| Feature | Excel 365 / 2021 | Excel 2019 | Excel 2016 | Excel 2013 and older |
|---|---|---|---|---|
| XLOOKUP | ✅ | ❌ | ❌ | ❌ |
| IFS / TEXTJOIN / CONCAT | ✅ | ✅ | ❌ | ❌ |
| SUMIFS/COUNTIFS/AVERAGEIFS | ✅ | ✅ | ✅ | ✅ |
| IFNA | ✅ | ✅ | ✅ | ✅ (2013+) |
| Dynamic arrays (FILTER, SORT, UNIQUE) | ✅ | ❌ | ❌ | ❌ |
| Power Query / PivotTable | ✅ | ✅ | ✅ | Partial |

**If XLOOKUP is unavailable, use:** `=IFNA(INDEX(return_range,MATCH(key,lookup_range,0)),"Not found")`.
**If FILTER/UNIQUE are unavailable, use:** Advanced Filter ("Unique records only"), `Remove Duplicates` on a copied column, or helper-column + COUNTIF flags.

---

## 4.9 Practice set (do these in Excel, not on paper)

Using the `Reps` and `Products` sheets above:

1. Total TRx across all reps. *(Expected 5,625)*
2. Average TRx per rep, rounded to 0 decimals. *(Expected 804)*
3. Number of North reps. *(3)*
4. Number of North CardioX reps. *(2)*
5. Total CardioX TRx. *(3,650)*
6. Total North + CardioX TRx. *(2,230)*
7. Average TRx for CardioX. *(1,216.67)*
8. Average TRx for North reps. *(861.67)*
9. For each rep, a column showing "At/Above" or "Below" target basis TRx vs Target. *(At/Above: R-001 1,250≥1,200, R-003 640≥600, R-004 1,420≥1,300, R-007 355≥340. Below: R-002 980<1,050, R-005 260<300, R-006 720<750)*
10. Achievement % per rep, 1 decimal. *(R-001 104.2%, R-002 93.3%, R-003 106.7%, R-004 109.2%, R-005 86.7%, R-006 96.0%, R-007 104.4%)*
11. Retrieve R-005's TRx with VLOOKUP, with XLOOKUP, and with INDEX-MATCH. *(260 in all three)*
12. Map each rep's product to a therapeutic area from the `Products` sheet, with `IFNA` fallback showing "Not found". *(CardioX→Cardiovascular, NeuroY→Neurology, OncoZ→Oncology)*
13. Build the string "Rep R-002 (North) - 980 TRx". *(=`"Rep "&A3&" ("&B3&") - "&TEXT(D3,"#,##0")&" TRx"`)*
14. Flag any rep whose achievement is below 90% with "REVIEW". *(Only R-005: 86.7%)*
15. Count how many reps are both below target and in a region other than North. *(R-005 East 86.7% and R-006 West 96.0% → count = 2)*

**Validation checks for any answer:** (1) do conditions' totals reconcile with the grand total? (2) does COUNTIFS match the number of visible rows after filtering? (3) are all lookups returning non-error values? (4) does the sum of parts equal the whole within rounding?

---

## 4.10 Trap list for the Excel assessment

- `SUMIF` = sum range **last**; `SUMIFS` = sum range **first**.
- `VLOOKUP` without `FALSE` silently returns wrong matches.
- Absolute references for anything you copy down.
- Count lookups that fail instead of hiding them.
- Text that looks like a number (left-aligned) breaks SUM/COUNTIF equality — fix with TRIM/VALUE or Text-to-Columns.
- Blanks are not zeros: decide explicitly whether to exclude or treat as 0, and state your choice.

**Next:** [05-excel-practical-workbook-guide.md](05-excel-practical-workbook-guide.md) → [15-mock-excel-assessment-45-min.md](15-mock-excel-assessment-45-min.md)
