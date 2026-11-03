# 20 — Power BI Complete Practical Guide

**Purpose:** take you from zero Power BI knowledge to the level expected of an entry-level analyst who can build dashboards, write DAX measures, and present insights from pharma data. The current JD explicitly lists dashboard creation with Power BI/Tableau as a responsibility (CONFIRMED).
**Prerequisites:** [04](04-excel-formulas-cheatsheet.md), [05](05-excel-practical-workbook-guide.md) — you should be comfortable with Excel before starting here.
**Suggested study time:** 12–16 hours across 4–5 sittings.
**Data provenance:** all datasets are **original and fictional (PRACTICE INFERENCE)**. Nothing here describes a real Trinity client or product.

---

## Part A — Power BI Fundamentals

### 20.1 What is Power BI?

Power BI is Microsoft's business intelligence platform for connecting to data, transforming it, modelling relationships, writing calculations, building interactive visuals, and sharing reports.

**Three components you must know:**

| Component | What it does | Where it runs |
|---|---|---|
| **Power BI Desktop** | Build reports: import data, clean it (Power Query), model it, write DAX, create visuals | Your Windows PC (free download) |
| **Power BI Service** | Publish, share, schedule refresh, build dashboards, set alerts, collaborate | Browser at app.powerbi.com (needs licence for sharing) |
| **Power Query** | The data-transformation engine embedded inside Desktop (and Excel). Think of it as your cleaning layer before analysis | Inside Desktop (Edit Queries) |

**Key insight:** Power BI Desktop is where you do 90% of the work as an analyst. The Service is for publishing and sharing. You will spend most of your time in Desktop.

### 20.2 Import vs DirectQuery

| Mode | How it works | When to use | Trade-off |
|---|---|---|---|
| **Import** | Copies the entire dataset into the .pbix file | Files up to ~1 GB, CSV/Excel, most scenarios | Fastest performance; data is a snapshot — must refresh to update |
| **DirectQuery** | Sends queries to the source database live; no data stored in .pbix | Large databases, real-time dashboards | Slower visuals; limited DAX support; requires database access |
| **Dual** | Some tables imported, some DirectQuery | Hybrid scenarios (dimension tables imported, fact tables live) | Complex to manage |

**For an entry-level analyst:** you will almost always use **Import** mode. Your datasets will be CSVs, Excel files, or exports from survey tools. DirectQuery matters when you work with large SQL databases later.

### 20.3 Data sources

Power BI can connect to 100+ sources. The ones relevant to this role:

| Source | How to connect | Typical use |
|---|---|---|
| CSV files | Get Data → Text/CSV | Survey exports, sales extracts |
| Excel files | Get Data → Excel Workbook | Client data, internal trackers |
| SQL Server | Get Data → SQL Server | Enterprise databases |
| Web | Get Data → Web | Public data tables |
| SharePoint | Get Data → SharePoint folder | Team file shares |
| Blank Query | Enter Data manually | Quick lookups, small reference tables |

### 20.4 Data refresh

- **Desktop:** click Refresh to re-import from source files.
- **Service:** schedule automatic refresh (up to 8× daily on Pro licence).
- **Key rule:** if your source CSV changes, Desktop will not auto-update — you must refresh or re-publish.

### 20.5 Report vs Dashboard vs Semantic Model (Dataset)

| Concept | What it is | Analogy |
|---|---|---|
| **Report** | A .pbix file with one or more pages of visuals, all connected to one data model | An Excel workbook with charts |
| **Dashboard** | A single canvas in the Service where you pin individual visuals from one or more reports | A wall of sticky notes from different workbooks |
| **Semantic Model (Dataset)** | The underlying data model (tables + relationships + measures) that powers reports | The database behind the charts |

**Common mistake:** calling a report page a "dashboard." In Power BI, a dashboard is specifically a pinned collection in the Service. In interviews, use the terms precisely.

---

## Part B — Power Query (Data Cleaning and Transformation)

Power Query is the first thing that runs when you load data. Think of it as your data-cleaning pipeline: raw data goes in, clean data comes out. Every step is recorded and re-playable.

### 20.6 Importing CSV and Excel files

**CSV:**
1. Home → Get Data → Text/CSV.
2. Select the file. Preview appears.
3. Check: delimiter (comma, tab, semicolon), encoding (UTF-8), data type detection.
4. Click **Transform Data** (not Load) — always inspect before loading.

**Excel:**
1. Home → Get Data → Excel Workbook.
2. Select the file. Navigator shows sheets and named ranges.
3. Select the sheet(s) you need. Click Transform Data.

**Best practice:** always go through Transform Data first. Loading directly skips your chance to catch problems.

### 20.7 Data cleaning in Power Query

#### Missing values

| Situation | Action in Power Query | Steps |
|---|---|---|
| Entire row is blank | Remove Rows → Remove Blank Rows | Home → Remove Rows → Remove Blank Rows |
| Specific column has nulls | Replace Values → replace `null` with 0 or a default | Right-click column → Replace Values |
| Remove rows where a key column is null | Filter the column → uncheck `(null)` | Click dropdown on column header → uncheck null |

**Pharma example:** A physician survey export has 500 rows, but 23 rows have null for `PhysicianID`. These are incomplete submissions. In Power Query: filter column `PhysicianID` → uncheck null → 477 rows remain. Document: "23 incomplete submissions removed (4.6%)."

#### Duplicate records

1. Select all columns (Ctrl+A).
2. Home → Remove Rows → Remove Duplicates.
3. Check the row count in the status bar before and after.

**Pharma example:** A sales extract has the same rep's January data twice because of a system re-run. Before: 150 rows. After removing duplicates: 148 rows. Two reps had exact duplicate entries.

**Caution:** removing duplicates based on ALL columns is safe. Removing based on a subset (e.g., just RepID) could delete legitimate records (same rep, different months). Always check.

#### Changing data types

Power Query auto-detects types, but often gets them wrong:

| Common problem | Symptom | Fix |
|---|---|---|
| Numbers stored as text | Column icon shows "ABC" instead of "123" | Select column → Transform → Data Type → Whole Number or Decimal |
| Dates as text | "Jan-26" not recognised | Data Type → Date; if that fails, parse with custom column |
| Percentages as text | "45%" stored as string | Replace "%" with "" → convert to decimal → divide by 100 |

**Steps:** Click the column header → Home → Data Type → select the correct type.

#### Splitting columns

**By delimiter:**
Right-click column → Split Column → By Delimiter → choose comma, space, dash, etc.

**Pharma example:** A column contains "North - Cardiologist". Split by " - " to get Region and Specialty in separate columns.

**By number of characters:**
Split Column → By Number of Characters → useful for codes like "P-001" where prefix is always 2 characters.

#### Merging columns

Select two or more columns (Ctrl+click) → Transform → Merge Columns → choose separator.

**Pharma example:** Merge `FirstName` and `LastName` with a space separator to create `FullName`.

#### Conditional columns

Add Column → Conditional Column → set if/then/else rules.

**Pharma example:** Create an "Achievement Band" column:

| Condition | Output |
|---|---|
| If `Achievement` >= 1.1 | "Exceeds" |
| If `Achievement` >= 0.9 | "On Track" |
| Else | "Below Target" |

This is equivalent to nested IF in Excel, but cleaner and applied at the data-loading stage.

#### Grouping and aggregation

Home → Group By → select the column to group by → choose aggregation (Sum, Count, Average, Min, Max).

**Pharma example:** Group a rep-level table by Region:
- Group By: Region
- New column: TotalTRx → Sum of TRx
- New column: RepCount → Count of RepID

Result:

| Region | TotalTRx | RepCount |
|---|---|---|
| North | 5,890 | 8 |
| South | 6,113 | 7 |
| East | 3,730 | 6 |
| West | 4,725 | 6 |

#### Merging queries (joins)

Home → Merge Queries → select the two tables and the key column → choose join type.

| Join type | Equivalent SQL | What it returns |
|---|---|---|
| Left Outer | LEFT JOIN | All rows from first table; matched rows from second |
| Right Outer | RIGHT JOIN | All rows from second; matched rows from first |
| Full Outer | FULL JOIN | All rows from both |
| Inner | INNER JOIN | Only matching rows |
| Left Anti | NOT IN | Rows in first table with NO match in second |
| Right Anti | NOT IN (reverse) | Rows in second table with NO match in first |

**Pharma example:** Merge a `Sales` table with a `RepMaster` table on RepID using Left Outer join. This brings in rep details (name, region, specialty) alongside sales data — like VLOOKUP but done at the data-loading layer.

After merging, expand the merged column by clicking the expand icon → select which fields to bring in.

#### Appending queries

Home → Append Queries → select the table to append.

**Use case:** you receive January sales and February sales as separate files. Append stacks them vertically into one table. Column names must match.

---

## Part C — Data Modelling

### 20.8 Fact tables and dimension tables

| Table type | Contains | Examples | Characteristics |
|---|---|---|---|
| **Fact table** | Measurable events, transactions | Sales transactions, survey responses, call records | Many rows, numeric columns (TRx, Revenue, Calls), foreign keys |
| **Dimension table** | Descriptive attributes | Products, Regions, Physicians, Time | Fewer rows, text/category columns, primary keys |

**Intuition:** fact tables record "what happened" (a rep sold 1,250 prescriptions). Dimension tables describe "who, what, where, when" (the rep is in the North region, selling CardioX, in January 2026).

### 20.9 Star schema

The star schema is the recommended data model for Power BI:

```
                    DimProduct
                        |
                        |
DimRegion ---- FactSales ---- DimDate
                        |
                        |
                    DimRep
```

- **Centre:** one fact table (FactSales) with foreign keys.
- **Points of the star:** dimension tables connected by those keys.
- Each dimension connects to the fact table with a **one-to-many** relationship.

**Why star schema matters:** Power BI's DAX engine is optimised for star schemas. Queries run faster, filters flow correctly, and measures behave predictably.

### 20.10 Relationships

**One-to-many (1:*):** the most common. One row in the dimension matches many rows in the fact table.

Example: One product "CardioX" in DimProduct → many rows in FactSales where Product = "CardioX".

**Many-to-many (*:*):** both tables have duplicate values in the key column. Avoid when possible — it creates ambiguity.

Example: A physician prescribes multiple products, and a product is prescribed by multiple physicians. Solution: create a bridge table (FactPrescriptions) with one row per physician-product combination.

**Setting relationships:**
1. Go to Model view (left sidebar icon).
2. Drag a column from one table to the matching column in another.
3. Power BI creates the relationship. Double-click to edit properties.

### 20.11 Filter direction

| Direction | Meaning | When to use |
|---|---|---|
| **Single** (default) | Filters flow from dimension → fact table only | Almost always. This is the correct default. |
| **Bi-directional** | Filters flow both ways | Rarely. Only when you need a dimension to be filtered by selections in another dimension through the fact table. |

**Rule of thumb:** keep single-direction unless you have a specific reason. Bi-directional can cause unexpected results and performance problems.

### 20.12 Date tables

Power BI works best when you have a dedicated date table with one row per date.

**Why:** time intelligence functions (YoY, MTD, QTD) require a contiguous date column marked as a date table.

**How to create one:**

```dax
DimDate = 
ADDCOLUMNS(
    CALENDAR(DATE(2024, 1, 1), DATE(2026, 12, 31)),
    "Year", YEAR([Date]),
    "Month", FORMAT([Date], "MMM-YY"),
    "MonthNumber", MONTH([Date]),
    "Quarter", "Q" & FORMAT([Date], "Q"),
    "YearMonth", FORMAT([Date], "YYYY-MM")
)
```

Then: select the DimDate table → Table Tools → Mark as Date Table → choose the Date column.

### 20.13 Common model design mistakes

| Mistake | Why it's wrong | Fix |
|---|---|---|
| One giant flat table | No relationships; DAX becomes complex; file bloats | Split into fact + dimension tables |
| Many-to-many without a bridge | Ambiguous counts and sums | Create a bridge/junction table |
| Bi-directional everywhere | Unpredictable filter propagation | Use single-direction; use CROSSFILTER in DAX if needed |
| No date table | Time intelligence functions fail | Create a date table and mark it |
| Circular dependencies | Power BI blocks these | Redesign the model; remove the redundant relationship |
| Calculated columns for aggregations | Bloats the model; recalculates per row | Use measures instead |

---

## Part D — DAX (Data Analysis Expressions)

### 20.14 Calculated columns vs measures vs calculated tables

| Type | When it runs | Where it lives | Use when |
|---|---|---|---|
| **Calculated column** | At data refresh (row by row) | Inside a table as a new column | You need a value per row (e.g., Achievement % per rep) |
| **Measure** | At query time (responds to filters/slicers) | In a table but not a column; shown in visuals | You need an aggregate that changes with context (e.g., Total TRx for the selected region) |
| **Calculated table** | At data refresh | Creates a new table | You need a derived table (e.g., a date table, a distinct list) |

**The most important distinction in DAX:** measures are dynamic (they respond to whatever filters the user applies). Calculated columns are static (they compute once and store the value).

**Rule:** if you're aggregating (SUM, AVERAGE, COUNT), use a **measure**. If you're computing per-row (Achievement = TRx / Target), a **calculated column** is fine.

### 20.15 Row context vs filter context

This is the single most important concept in DAX.

**Row context:** exists when DAX iterates over rows — inside a calculated column or inside an iterator function (SUMX, AVERAGEX). Each row is processed one at a time.

```dax
// Calculated column — has ROW CONTEXT
Achievement = FactSales[TRx] / FactSales[TargetTRx]
```

For each row, DAX knows `[TRx]` and `[TargetTRx]` for THAT specific row.

**Filter context:** exists when a measure is evaluated. Determined by:
- Slicers the user selected.
- Filters on the visual (e.g., a bar chart grouped by Region).
- Row/column headers in a matrix.
- CALCULATE modifiers.

```dax
// Measure — responds to FILTER CONTEXT
Total TRx = SUM(FactSales[TRx])
```

If a slicer is set to "North", this measure returns the sum of TRx only for North. If no filter, it returns the grand total. The formula doesn't change — the context does.

**CALCULATE is the bridge:** it modifies filter context. This is why CALCULATE is the most important DAX function.

### 20.16 SUM

**Syntax:** `SUM(<column>)`

**What it does:** adds up all values in a column, respecting the current filter context.

**Example:**
```dax
Total TRx = SUM(FactSales[TRx])
```

If the visual groups by Region:
| Region | Total TRx |
|---|---|
| North | 5,890 |
| South | 6,113 |
| **Total** | **23,458** |

**Common mistake:** using SUM in a calculated column. SUM in a calculated column sums the ENTIRE column (no row context for aggregation). Use a measure instead.

### 20.17 AVERAGE

**Syntax:** `AVERAGE(<column>)`

**What it does:** arithmetic mean of all non-blank values in the column.

**Example:**
```dax
Avg TRx per Rep = AVERAGE(FactSales[TRx])
```

**Business meaning:** "On average, each rep generates X prescriptions." Useful for benchmarking individual reps against the average.

**Caution:** AVERAGE ignores blanks but not zeros. If a rep has TRx = 0 (e.g., new hire, no sales yet), that zero IS included in the average and drags it down. Decide whether to filter out zeros.

### 20.18 COUNTROWS

**Syntax:** `COUNTROWS(<table>)`

**What it does:** counts the number of rows in a table (or in the current filter context of that table).

**Example:**
```dax
Number of Reps = COUNTROWS(FactSales)
```

If filtered to January: counts only January rows.

**vs COUNT:** `COUNT(FactSales[TRx])` counts non-blank values in ONE column. `COUNTROWS` counts rows regardless of blanks. Use COUNTROWS for row counts; use COUNT only when you specifically want to count non-blanks in a column.

### 20.19 DISTINCTCOUNT

**Syntax:** `DISTINCTCOUNT(<column>)`

**What it does:** counts distinct (unique) values in a column.

**Example:**
```dax
Unique Reps = DISTINCTCOUNT(FactSales[RepID])
```

**Why it matters:** if a rep appears in both January and February data, COUNTROWS would count them twice. DISTINCTCOUNT counts them once.

**Business meaning:** "How many individual reps contributed to this region's sales?" vs "How many sales records exist?"

### 20.20 DIVIDE

**Syntax:** `DIVIDE(<numerator>, <denominator>, <alternate_result>)`

**What it does:** divides numerator by denominator. If denominator is 0 or blank, returns the alternate result (default: BLANK).

**Example:**
```dax
Achievement % = DIVIDE(SUM(FactSales[TRx]), SUM(FactSales[TargetTRx]), 0)
```

**Why DIVIDE is better than `/`:**

| Expression | If denominator = 0 |
|---|---|
| `[TRx] / [Target]` | **Error** (#DIV/0! equivalent) |
| `DIVIDE([TRx], [Target], 0)` | Returns 0 (or any value you specify) |

**Always use DIVIDE for measures.** The `/` operator is fine in calculated columns where you've already filtered out zero targets.

### 20.21 CALCULATE — The Most Important DAX Function

**Syntax:** `CALCULATE(<expression>, <filter1>, <filter2>, ...)`

**What it does:** evaluates an expression in a MODIFIED filter context. It takes the current filter context, applies additional filters (or removes them), and then evaluates.

**Why it matters:** CALCULATE is how you answer questions like "What is Total TRx, but only for CardioX?" or "What is this month's TRx compared to the same period last year?"

**Example 1 — Add a filter:**
```dax
CardioX TRx = 
CALCULATE(
    SUM(FactSales[TRx]),
    FactSales[Product] = "CardioX"
)
```

Even if the user hasn't filtered to CardioX, this measure always returns CardioX's total.

**Example 2 — Multiple filters:**
```dax
North CardioX TRx = 
CALCULATE(
    SUM(FactSales[TRx]),
    FactSales[Product] = "CardioX",
    FactSales[Region] = "North"
)
```

Filters are AND-ed: Product must be CardioX AND Region must be North.

**Example 3 — Use with ALL (remove filters):**
```dax
TRx Share of Total = 
DIVIDE(
    SUM(FactSales[TRx]),
    CALCULATE(SUM(FactSales[TRx]), ALL(FactSales[Region]))
)
```

If the visual groups by Region, each row shows that region's share of the total across ALL regions. ALL removes the Region filter, so the denominator is the grand total.

**Example 4 — Context transition:**
When CALCULATE is used inside an iterator (like SUMX), it converts row context to filter context. This is called **context transition** and is an advanced but crucial concept.

```dax
Weighted Achievement = 
SUMX(
    DimRep,
    CALCULATE(DIVIDE(SUM(FactSales[TRx]), SUM(FactSales[TargetTRx])))
)
```

For each rep (row context), CALCULATE transitions to filter context for that rep, then evaluates the division.

### 20.22 FILTER

**Syntax:** `FILTER(<table>, <condition>)`

**What it does:** returns a table containing only rows that meet the condition. Used inside CALCULATE or as a table argument.

**Example:**
```dax
High Performers TRx = 
CALCULATE(
    SUM(FactSales[TRx]),
    FILTER(FactSales, FactSales[TRx] > 1000)
)
```

**When to use FILTER vs a simple CALCULATE filter:**
- Simple: `CALCULATE(SUM(...), Table[Column] = "Value")` — for exact matches on a single column.
- FILTER: when the condition involves calculations, comparisons between columns, or complex logic.

### 20.23 ALL

**Syntax:** `ALL(<table_or_column>)`

**What it does:** removes all filters from a table or column. Returns the full, unfiltered table/column.

**Example — Percentage of grand total:**
```dax
% of Total TRx = 
DIVIDE(
    SUM(FactSales[TRx]),
    CALCULATE(SUM(FactSales[TRx]), ALL(FactSales))
)
```

No matter what slicers are applied, the denominator is the total of ALL sales.

**Example — Percentage within a dimension:**
```dax
% of Region Total = 
DIVIDE(
    SUM(FactSales[TRx]),
    CALCULATE(SUM(FactSales[TRx]), ALL(FactSales[Product]))
)
```

If the visual shows Region × Product, this calculates each product's share within its region (removes the Product filter but keeps Region).

**ALLEXCEPT:** removes all filters EXCEPT the specified columns.
```dax
% within Region = 
DIVIDE(
    SUM(FactSales[TRx]),
    CALCULATE(SUM(FactSales[TRx]), ALLEXCEPT(FactSales, FactSales[Region]))
)
```

### 20.24 VALUES and SELECTEDVALUE

**VALUES:** returns a one-column table of distinct values in the current filter context.

```dax
VALUES(DimProduct[ProductName])
// If filtered to CardioX, returns a table with one row: "CardioX"
// If no filter, returns all product names
```

**SELECTEDVALUE:** returns a single scalar value if only one value is in context; otherwise returns an alternate result.

```dax
Selected Product = SELECTEDVALUE(DimProduct[ProductName], "Multiple")
```

**Use case:** dynamic titles.
```dax
Dashboard Title = 
"Sales Performance: " & SELECTEDVALUE(DimRegion[Region], "All Regions")
```

When the user selects "North" in a slicer, the title shows "Sales Performance: North". When no region is selected, it shows "Sales Performance: All Regions".

### 20.25 SUMX (iterator pattern)

**Syntax:** `SUMX(<table>, <expression>)`

**What it does:** iterates over each row of the table, evaluates the expression for that row, then sums all the results.

**Why it's different from SUM:** SUM aggregates a single column. SUMX can compute a per-row calculation and then aggregate.

**Example — Revenue (Price × Volume per row):**
```dax
Total Revenue = 
SUMX(
    FactSales,
    FactSales[TRx] * RELATED(DimProduct[PricePerRx])
)
```

For each row in FactSales, multiply TRx by the product's price (pulled from the dimension table via RELATED), then sum all results.

**Why not a calculated column + SUM?** You could create a Revenue calculated column and then SUM it. But SUMX avoids storing redundant columns and the computation responds to filter context.

**Other iterators:** AVERAGEX, COUNTX, MINX, MAXX — same pattern, different aggregation.

### 20.26 RELATED

**Syntax:** `RELATED(<column>)`

**What it does:** fetches a value from a related table (on the "one" side of a one-to-many relationship). Works in calculated columns and inside iterators.

**Example:**
```dax
// Calculated column in FactSales
Product Category = RELATED(DimProduct[TherapeuticArea])
```

For each row in FactSales, RELATED looks up the TherapeuticArea from DimProduct using the relationship.

**Equivalent of:** VLOOKUP / XLOOKUP in Excel, or JOIN in SQL.

**RELATEDTABLE:** the reverse — returns ALL matching rows from the "many" side. Used in dimension tables.

### 20.27 DATEADD

**Syntax:** `DATEADD(<date_column>, <number_of_intervals>, <interval>)`

**What it does:** shifts dates by a specified number of intervals. Returns a table of dates.

**Example — Previous month's TRx:**
```dax
Previous Month TRx = 
CALCULATE(
    SUM(FactSales[TRx]),
    DATEADD(DimDate[Date], -1, MONTH)
)
```

If the current filter context is February 2026, this returns January 2026's TRx.

**Month-over-month growth:**
```dax
MoM Growth % = 
VAR CurrentMonth = SUM(FactSales[TRx])
VAR PreviousMonth = CALCULATE(SUM(FactSales[TRx]), DATEADD(DimDate[Date], -1, MONTH))
RETURN
DIVIDE(CurrentMonth - PreviousMonth, PreviousMonth)
```

### 20.28 SAMEPERIODLASTYEAR

**Syntax:** `SAMEPERIODLASTYEAR(<date_column>)`

**What it does:** returns the same set of dates shifted back by exactly one year.

**Example — Year-over-year comparison:**
```dax
Last Year TRx = 
CALCULATE(
    SUM(FactSales[TRx]),
    SAMEPERIODLASTYEAR(DimDate[Date])
)

YoY Growth % = 
VAR Current = SUM(FactSales[TRx])
VAR LastYear = [Last Year TRx]
RETURN
DIVIDE(Current - LastYear, LastYear)
```

**Business meaning:** "Is this month's performance better or worse than the same month last year?" Essential for pharma brands with seasonal prescribing patterns.

### 20.29 VAR and RETURN

Not a function per se, but essential syntax for readable DAX:

```dax
Achievement Rate = 
VAR Actual = SUM(FactSales[TRx])
VAR Target = SUM(FactSales[TargetTRx])
RETURN
DIVIDE(Actual, Target, 0)
```

**Always use VAR for:**
- Readability (name your intermediate results).
- Performance (a VAR is evaluated once, even if referenced multiple times).
- Debugging (you can change the RETURN to any VAR to see intermediate values).

---

## Part E — Dashboard Development

### 20.30 KPI cards

**What:** a single large number with optional target, trend indicator, and title.

**How:** Visualizations pane → KPI or Card visual → drag a measure.

**Best practice for pharma dashboards:**

| KPI Card | Measure | Format |
|---|---|---|
| Total TRx | SUM of TRx | #,##0 |
| Achievement % | TRx / Target | 0.0% with conditional colour |
| Active Reps | DISTINCTCOUNT of RepID | #,##0 |
| Avg TRx/Rep | AVERAGE or DIVIDE | #,##0 |

**Layout tip:** place 4–5 KPI cards across the top of the dashboard. The user should grasp the headline numbers before scrolling down.

### 20.31 Bar and column charts

| Use case | Chart type |
|---|---|
| Compare categories (regions, products) | **Bar chart** (horizontal) — better for long labels |
| Show values over a few time periods | **Column chart** (vertical) |
| Compare actual vs target | **Clustered bar/column** (two bars per category) |
| Show part-to-whole | **Stacked bar/column** |

**Pharma example:** clustered column chart showing TRx (actual) vs TargetTRx per region. Conditional formatting: red if below target, green if above.

### 20.32 Line charts

**When:** show trends over time — monthly TRx, quarterly revenue, market share trajectory.

**Best practice:**
- X-axis = Date or Month (continuous).
- Y-axis = the measure.
- Add a reference line for target or industry benchmark.
- If comparing brands, use multiple lines with a legend.

### 20.33 Tables and matrices

| Visual | Use when |
|---|---|
| **Table** | Flat list of detail rows (rep-level data) |
| **Matrix** | Cross-tabulation with row and column grouping (Region × Product), with subtotals |

**Matrix example:** rows = Region, columns = Product, values = TRx. Enable subtotals for row and column. Add conditional formatting (colour scale or data bars).

### 20.34 Slicers

Slicers are the interactive filters the user clicks to change what the dashboard shows.

| Slicer type | Use when |
|---|---|
| **List** | Few options (4–8 items) — Region, Product |
| **Dropdown** | Many options — physician names, rep IDs |
| **Between** | Numeric ranges — TRx between 500 and 1500 |
| **Date range** | Time selection — date slider |

**Best practice:** place slicers at the top or left side. Sync slicers across pages if you want a filter to persist when the user navigates.

### 20.35 Drill-down and drill-through

**Drill-down:** within a single visual, go from summary to detail. Example: click on a bar for "North" → see it split by Product → click on "CardioX" → see it split by Rep.

**How:** add multiple fields to the axis well of a chart: Region → Product → RepID. The drill-down icons appear in the visual header.

**Drill-through:** navigate from one report page to a detail page, carrying the filter context.

**How:**
1. Create a detail page (e.g., "Rep Detail").
2. In that page's filters, add RepID to the "Drill-through" filter.
3. On the main page, right-click a rep → Drill Through → Rep Detail.

### 20.36 Tooltips

**Default tooltips:** hover over a data point to see its values.

**Custom tooltip pages:** create a separate report page (Page Size → Tooltip), design it with small visuals, then assign it as the tooltip for a chart.

**Pharma example:** hovering over a rep's bar shows a tooltip page with their monthly trend, achievement %, and top product.

### 20.37 Conditional formatting

**Where:** in table/matrix visuals and bar charts.

**How:** select the visual → Format → Conditional Formatting → choose:
- **Background colour:** red/yellow/green based on value.
- **Font colour:** change text colour.
- **Data bars:** mini bar charts inside table cells.
- **Icons:** ▲▼ arrows for performance direction.

**Pharma example:** in a rep achievement table, format Achievement % column:
- ≥ 110%: green background
- 90–110%: yellow background
- < 90%: red background

### 20.38 Bookmarks

**What:** save the current state of the report (filters, slicer selections, visual visibility) and recall it with a button click.

**Use case:** create a "Revenue View" bookmark and a "Volume View" bookmark. Add buttons that toggle between them. The user switches perspectives without rebuilding filters.

**How:**
1. Set up the view you want.
2. View → Bookmarks → Add Bookmark → name it.
3. Insert → Buttons → Action → Type = Bookmark → select the bookmark.

### 20.39 Interactive filtering and cross-filtering

**Default behaviour:** clicking a data point in one visual filters other visuals on the same page.

**Edit interactions:** select a visual → Format → Edit Interactions → for each other visual, choose:
- **Filter:** show only matching data.
- **Highlight:** dim non-matching data.
- **None:** no interaction.

**Best practice:** use Filter for tables and detailed views. Use Highlight for charts where you want to maintain the overall shape.

---

## Part F — Business Dashboard Projects

### Project 1: Pharmaceutical Sales Performance Dashboard

#### F1.1 Business context (fictional — PRACTICE INFERENCE)

MedVantage Pharma is a mid-sized Indian pharmaceutical company with three branded products across four regions. Management wants a dashboard to track sales performance, identify underperforming regions and reps, and monitor target achievement. The dashboard will be updated monthly and shared with regional managers.

#### F1.2 Dataset schema

**FactSales table:**

| Column | Type | Description |
|---|---|---|
| SaleID | Text | Unique row identifier (S-0001) |
| RepID | Text | Foreign key to DimRep |
| ProductID | Text | Foreign key to DimProduct |
| RegionID | Text | Foreign key to DimRegion |
| DateKey | Date | Foreign key to DimDate |
| TRx | Integer | Total prescriptions written |
| TargetTRx | Integer | Target prescriptions |
| Revenue_Lakhs | Decimal | Revenue in ₹ Lakhs |
| Calls | Integer | Physician calls made |

**DimRep table:**

| Column | Type | Sample values |
|---|---|---|
| RepID | Text | R-001, R-002, ... |
| RepName | Text | Aarav, Bhavna, ... |
| JoinDate | Date | 2024-06-15 |
| Experience_Years | Integer | 1, 2, 3 |

**DimProduct table:**

| Column | Type | Sample values |
|---|---|---|
| ProductID | Text | PROD-01, PROD-02, PROD-03 |
| ProductName | Text | CardioX, NeuroY, OncoZ |
| TherapeuticArea | Text | Cardiovascular, Neurology, Oncology |
| LaunchDate | Date | 2020-01-15 |
| PricePerRx | Decimal | 450, 680, 1200 |

**DimRegion table:**

| Column | Type | Sample values |
|---|---|---|
| RegionID | Text | REG-N, REG-S, REG-E, REG-W |
| RegionName | Text | North, South, East, West |
| HeadCount | Integer | 8, 7, 6, 6 |

**DimDate table:** (generated via DAX CALENDAR)

| Column | Type |
|---|---|
| Date | Date |
| Year | Integer |
| MonthName | Text |
| MonthNumber | Integer |
| Quarter | Text |
| YearMonth | Text |

#### F1.3 Sample data (FactSales — 24 rows across 2 months)

Save as `data/pharma_sales_dashboard.csv`:

```
SaleID,RepID,ProductID,RegionID,DateKey,TRx,TargetTRx,Revenue_Lakhs,Calls
S-0001,R-001,PROD-01,REG-N,2026-01-15,1250,1200,5.63,40
S-0002,R-002,PROD-01,REG-N,2026-01-15,980,1050,4.41,36
S-0003,R-003,PROD-02,REG-N,2026-01-15,640,600,4.35,30
S-0004,R-004,PROD-01,REG-S,2026-01-15,1420,1300,6.39,45
S-0005,R-005,PROD-02,REG-S,2026-01-15,880,900,5.98,38
S-0006,R-006,PROD-03,REG-E,2026-01-15,260,300,3.12,22
S-0007,R-007,PROD-01,REG-E,2026-01-15,1105,1100,4.97,41
S-0008,R-008,PROD-02,REG-W,2026-01-15,720,750,4.90,33
S-0009,R-009,PROD-03,REG-W,2026-01-15,410,380,4.92,28
S-0010,R-010,PROD-01,REG-N,2026-01-15,1320,1250,5.94,44
S-0011,R-011,PROD-03,REG-N,2026-01-15,355,340,4.26,26
S-0012,R-012,PROD-01,REG-S,2026-01-15,1150,1200,5.18,39
S-0013,R-001,PROD-01,REG-N,2026-02-15,1310,1200,5.90,42
S-0014,R-002,PROD-01,REG-N,2026-02-15,1020,1050,4.59,38
S-0015,R-003,PROD-02,REG-N,2026-02-15,700,600,4.76,32
S-0016,R-004,PROD-01,REG-S,2026-02-15,1480,1300,6.66,47
S-0017,R-005,PROD-02,REG-S,2026-02-15,920,900,6.26,40
S-0018,R-006,PROD-03,REG-E,2026-02-15,290,300,3.48,24
S-0019,R-007,PROD-01,REG-E,2026-02-15,1160,1100,5.22,43
S-0020,R-008,PROD-02,REG-W,2026-02-15,760,750,5.17,35
S-0021,R-009,PROD-03,REG-W,2026-02-15,445,380,5.34,30
S-0022,R-010,PROD-01,REG-N,2026-02-15,1380,1250,6.21,46
S-0023,R-011,PROD-03,REG-N,2026-02-15,380,340,4.56,28
S-0024,R-012,PROD-01,REG-S,2026-02-15,1200,1200,5.40,41
```

#### F1.4 Business requirements

The dashboard should answer:
1. What is the overall TRx achievement % this period?
2. Which region is performing best/worst?
3. Which product contributes the most revenue?
4. How many reps are below target?
5. What is the month-over-month trend?
6. What is each rep's productivity (TRx per call)?
7. Which product has the highest achievement rate?

#### F1.5 Data model instructions

1. Import the CSV. Create the DimDate table using DAX CALENDAR.
2. Create DimProduct, DimRep, and DimRegion tables using Enter Data.
3. Set relationships:
   - FactSales[ProductID] → DimProduct[ProductID] (many-to-one)
   - FactSales[RegionID] → DimRegion[RegionID] (many-to-one)
   - FactSales[RepID] → DimRep[RepID] (many-to-one)
   - FactSales[DateKey] → DimDate[Date] (many-to-one)
4. Mark DimDate as date table.

#### F1.6 Required DAX measures

```dax
// 1. Total TRx
Total TRx = SUM(FactSales[TRx])

// 2. Total Target
Total Target = SUM(FactSales[TargetTRx])

// 3. Achievement %
Achievement % = DIVIDE([Total TRx], [Total Target], 0)

// 4. Total Revenue
Total Revenue = SUM(FactSales[Revenue_Lakhs])

// 5. Reps Below Target
Reps Below Target = 
COUNTROWS(
    FILTER(
        VALUES(FactSales[RepID]),
        CALCULATE([Achievement %]) < 1
    )
)

// 6. Total Reps
Total Reps = DISTINCTCOUNT(FactSales[RepID])

// 7. TRx per Call (Productivity)
TRx per Call = DIVIDE([Total TRx], SUM(FactSales[Calls]), 0)

// 8. Previous Month TRx
Prev Month TRx = 
CALCULATE([Total TRx], DATEADD(DimDate[Date], -1, MONTH))

// 9. MoM Growth %
MoM Growth = DIVIDE([Total TRx] - [Prev Month TRx], [Prev Month TRx])

// 10. % of Total TRx (for regional share)
TRx Share = 
DIVIDE(
    [Total TRx],
    CALCULATE([Total TRx], ALL(DimRegion))
)

// 11. Highest Performing Region (text)
Top Region = 
TOPN(1, VALUES(DimRegion[RegionName]), [Achievement %], DESC)

// 12. Revenue per TRx
Revenue per TRx = DIVIDE([Total Revenue], [Total TRx], 0)
```

#### F1.7 Dashboard layout

**Page 1: Executive Summary**

```
┌──────────────────────────────────────────────────────┐
│  [Slicer: Month]  [Slicer: Region]  [Slicer: Product]│
├───────────┬───────────┬───────────┬──────────────────┤
│ Total TRx │ Achieve % │ Revenue   │ Reps Below Target│
│  21,405   │  102.8%   │ ₹120.3L   │     4 / 12       │
├───────────┴───────────┴───────────┴──────────────────┤
│                                                       │
│  [Clustered Column: TRx vs Target by Region]          │
│                                                       │
├───────────────────────────┬───────────────────────────┤
│ [Donut: TRx by Product]  │ [Line: Monthly TRx Trend] │
│                           │                           │
└───────────────────────────┴───────────────────────────┘
```

**Page 2: Rep Performance Detail**

```
┌──────────────────────────────────────────────────────┐
│  [Slicer: Region]  [Slicer: Month]                    │
├──────────────────────────────────────────────────────┤
│  [Matrix: RepName × Month, values: TRx, Target,      │
│   Achievement %, with conditional formatting]          │
├──────────────────────────────────────────────────────┤
│  [Scatter: TRx vs Calls per rep — shows productivity] │
│                                                       │
└──────────────────────────────────────────────────────┘
```

#### F1.8 Business questions to answer

1. Which region has the highest achievement % and which has the lowest?
2. Is there a relationship between number of calls and TRx generated?
3. Which product generates the most revenue per prescription?
4. How many reps improved from Jan to Feb? How many declined?
5. If East is underperforming, what specifically is driving it — fewer reps, fewer calls, or lower conversion?

#### F1.9 Expected analytical conclusions

Based on the sample data:
- **Overall achievement is ~102%** — the team is hitting targets.
- **South region leads** (R-004 and R-005 are strong performers).
- **East is weakest** — only R-007 and R-006; R-006 (OncoZ) is consistently below target.
- **CardioX dominates revenue** (~55% of total TRx) — product concentration risk.
- **OncoZ has highest revenue per TRx** (₹1,200/Rx vs ₹450 for CardioX) — premium product.
- **MoM trend is positive** — most reps improved Jan→Feb.

**So what does this mean for the business?** The team is meeting targets overall, but East needs attention. OncoZ is the most valuable product per prescription but has the lowest volume — explore whether it's an access issue or a physician awareness issue. CardioX's dominance means any competitive disruption in cardiovascular would disproportionately hurt the portfolio.

#### F1.10 Common mistakes

| Mistake | Impact | Prevention |
|---|---|---|
| Using SUM(Achievement) instead of DIVIDE(SUM(TRx), SUM(Target)) | Averages percentages across unequal groups | Always compute % from components |
| Not setting data types in Power Query | Dates treated as text → time intelligence fails | Set types on import |
| Forgetting to mark the date table | DATEADD, SAMEPERIODLASTYEAR return errors | Mark DimDate as date table in Table Tools |
| Showing Total row in a matrix with % | Total row "sums" percentages → meaningless | Format Total row separately or hide it |

---

### Project 2: Survey and Physician Adoption Dashboard

#### F2.1 Business context (fictional — PRACTICE INFERENCE)

NovaBio launched "ImmunoPulse" (an immunology biologic) 18 months ago. A survey of 200 physicians was conducted to understand awareness, trial, and adoption barriers. Management needs a dashboard to visualise the funnel, identify specialty- and region-level gaps, and track adoption drivers and barriers.

#### F2.2 Dataset schema

**FactSurvey table:**

| Column | Type | Description |
|---|---|---|
| ResponseID | Text | Unique response ID |
| PhysicianID | Text | Foreign key |
| AwarenessStage | Text | Unaware / Aware / Considering / Trialled / Regular |
| LikelihoodToPrescribe | Integer | 1–5 Likert scale |
| SatisfactionWithEfficacy | Integer | 1–5 |
| PerceivedAccessBarrier | Integer | 1–5 (5 = major barrier) |
| PrimaryBarrier | Text | Access / Efficacy Data / Safety / Cost / Inertia |
| WouldRecommend | Text | Yes / No / Unsure |

**DimPhysician table:**

| Column | Type | Sample values |
|---|---|---|
| PhysicianID | Text | PHY-001, PHY-002, ... |
| Specialty | Text | Rheumatologist, Dermatologist, Immunologist |
| Region | Text | North, South, East, West |
| PrescribingVolume | Text | High (>100 Rx/month), Medium (50–100), Low (<50) |

#### F2.3 Sample data (20 rows)

```
ResponseID,PhysicianID,AwarenessStage,LikelihoodToPrescribe,SatisfactionWithEfficacy,PerceivedAccessBarrier,PrimaryBarrier,WouldRecommend
RESP-001,PHY-001,Regular,5,5,2,None,Yes
RESP-002,PHY-002,Trialled,4,4,3,Access,Yes
RESP-003,PHY-003,Considering,3,3,4,Access,Unsure
RESP-004,PHY-004,Aware,2,3,4,Efficacy Data,No
RESP-005,PHY-005,Unaware,1,1,5,Cost,No
RESP-006,PHY-006,Regular,5,4,2,None,Yes
RESP-007,PHY-007,Trialled,4,4,3,Safety,Yes
RESP-008,PHY-008,Considering,3,3,4,Access,Unsure
RESP-009,PHY-009,Aware,2,2,5,Cost,No
RESP-010,PHY-010,Trialled,4,5,2,None,Yes
RESP-011,PHY-011,Regular,5,5,1,None,Yes
RESP-012,PHY-012,Considering,3,3,3,Inertia,Unsure
RESP-013,PHY-013,Aware,2,3,4,Efficacy Data,No
RESP-014,PHY-014,Unaware,1,1,5,Cost,No
RESP-015,PHY-015,Trialled,4,4,3,Access,Yes
RESP-016,PHY-016,Regular,5,5,2,None,Yes
RESP-017,PHY-017,Considering,3,4,3,Safety,Unsure
RESP-018,PHY-018,Aware,3,3,4,Access,Unsure
RESP-019,PHY-019,Trialled,4,4,2,None,Yes
RESP-020,PHY-020,Considering,3,3,4,Efficacy Data,Unsure
```

#### F2.4 Required DAX measures

```dax
// 1. Total Responses
Total Responses = COUNTROWS(FactSurvey)

// 2. Funnel counts by stage
Aware or Beyond = 
CALCULATE(
    COUNTROWS(FactSurvey),
    FactSurvey[AwarenessStage] IN {"Aware", "Considering", "Trialled", "Regular"}
)

// 3. Top-2-Box Likelihood (score 4 or 5)
T2B Likelihood = 
DIVIDE(
    CALCULATE(COUNTROWS(FactSurvey), FactSurvey[LikelihoodToPrescribe] >= 4),
    COUNTROWS(FactSurvey)
)

// 4. Mean Likelihood
Avg Likelihood = AVERAGE(FactSurvey[LikelihoodToPrescribe])

// 5. Net Promoter-style: % Yes minus % No for WouldRecommend
Recommend Net = 
VAR YesPct = DIVIDE(
    CALCULATE(COUNTROWS(FactSurvey), FactSurvey[WouldRecommend] = "Yes"),
    [Total Responses])
VAR NoPct = DIVIDE(
    CALCULATE(COUNTROWS(FactSurvey), FactSurvey[WouldRecommend] = "No"),
    [Total Responses])
RETURN YesPct - NoPct

// 6. Top Barrier
Top Barrier = 
TOPN(1, 
    ADDCOLUMNS(
        VALUES(FactSurvey[PrimaryBarrier]),
        "Count", CALCULATE(COUNTROWS(FactSurvey))
    ),
    [Count], DESC
)

// 7. Stage conversion rates
Aware to Consider Rate = 
DIVIDE(
    CALCULATE(COUNTROWS(FactSurvey), 
        FactSurvey[AwarenessStage] IN {"Considering","Trialled","Regular"}),
    CALCULATE(COUNTROWS(FactSurvey), 
        FactSurvey[AwarenessStage] IN {"Aware","Considering","Trialled","Regular"})
)

// 8. Access barrier severity (average among those citing Access)
Avg Access Barrier = 
CALCULATE(
    AVERAGE(FactSurvey[PerceivedAccessBarrier]),
    FactSurvey[PrimaryBarrier] = "Access"
)
```

#### F2.5 Dashboard layout

**Page 1: Adoption Funnel Overview**

```
┌──────────────────────────────────────────────────────┐
│  [Slicer: Specialty]  [Slicer: Region]  [Slicer: Volume]│
├───────────┬───────────┬───────────┬──────────────────┤
│ Responses │ T2B Like% │ Net Rec   │ Avg Barrier Score│
│   200     │   55%     │  +30%     │    3.2 / 5       │
├──────────────────────────────────────────────────────┤
│  [Funnel Chart: Unaware → Aware → Considering →      │
│   Trialled → Regular, with counts and % conversion]   │
├───────────────────────────┬───────────────────────────┤
│ [Bar: Primary Barrier     │ [Stacked Bar: WouldRecommend│
│  counts by barrier type]  │  by Specialty]              │
└───────────────────────────┴───────────────────────────┘
```

**Page 2: Segment Deep Dive**

```
┌──────────────────────────────────────────────────────┐
│  [Matrix: Specialty × AwarenessStage, count + %]      │
├──────────────────────────────────────────────────────┤
│  [Scatter: Likelihood vs Efficacy Satisfaction        │
│   by physician, coloured by barrier]                  │
├──────────────────────────────────────────────────────┤
│  [Table: Top 10 physicians with highest likelihood    │
│   who haven't trialled — conversion targets]          │
└──────────────────────────────────────────────────────┘
```

#### F2.6 Business questions

1. What is the largest funnel leak — where do most physicians drop off?
2. Is Access the #1 barrier across all specialties, or only for some?
3. Which specialty has the highest T2B likelihood but hasn't converted to trial?
4. Among physicians who trialled, what is their satisfaction level? Are they recommending?
5. What action should management take: invest in payer access, physician education, or safety data?

#### F2.7 Expected conclusions

- **Biggest funnel leak is Aware → Considering** (many aware physicians haven't moved to consideration).
- **Access is the #1 barrier** (cited by 5 of 20 physicians; average barrier score of 3.7).
- **Rheumatologists have the highest T2B** (hypothesis: they see more eligible patients).
- **Among trialled physicians, satisfaction is high** (avg 4.2/5) and 80% would recommend → the product converts well once tried.
- **Recommendation:** invest in payer access programs (prior-authorisation support) for the top 3 payers. This addresses the largest barrier for the segment most likely to prescribe.

---

## Part G — Timed Power BI Practical Assessment

**Time limit: 60 minutes.**
**Scenario (PRACTICE INFERENCE — not a Trinity assessment):**

You are a new analyst at PharmaPulse Analytics. Your manager gives you a dataset of sales rep performance and asks you to build a one-page dashboard in Power BI and answer five business questions.

### G.1 Dataset

Use the `data/pharma_sales_dashboard.csv` file created in Project 1 above (24 rows).

Also create this reference table by entering data manually:

**DimProduct:**

| ProductID | ProductName | TherapeuticArea | PricePerRx |
|---|---|---|---|
| PROD-01 | CardioX | Cardiovascular | 450 |
| PROD-02 | NeuroY | Neurology | 680 |
| PROD-03 | OncoZ | Oncology | 1200 |

**DimRegion:**

| RegionID | RegionName |
|---|---|
| REG-N | North |
| REG-S | South |
| REG-E | East |
| REG-W | West |

### G.2 Tasks (60 minutes)

| # | Task | Points | Time |
|---|---|---|---|
| 1 | Import CSV, set correct data types in Power Query | 5 | 5 min |
| 2 | Create DimProduct and DimRegion using Enter Data; set up star schema relationships | 10 | 8 min |
| 3 | Create a DimDate table using DAX CALENDAR covering Jan–Feb 2026; mark as date table | 5 | 5 min |
| 4 | Write these DAX measures: Total TRx, Achievement %, Reps Below Target, TRx per Call | 15 | 10 min |
| 5 | Write a MoM Growth % measure using DATEADD | 10 | 5 min |
| 6 | Write a TRx Share % measure using ALL | 10 | 5 min |
| 7 | Build 4 KPI cards (Total TRx, Achievement %, Revenue, Reps Below Target) | 10 | 5 min |
| 8 | Build a clustered column chart: TRx vs Target by Region | 10 | 5 min |
| 9 | Build a matrix: Region × Product with TRx values and conditional formatting | 10 | 5 min |
| 10 | Write a 3-sentence analytical conclusion answering: "What should the manager focus on?" | 15 | 7 min |

**Total: 100 points.**

### G.3 Answer key

**Task 1:** After import, ensure TRx and TargetTRx are Whole Number, Revenue_Lakhs is Decimal, DateKey is Date. Calls should be Whole Number.

**Task 2:** Enter the data tables. Create relationships:
- FactSales[ProductID] → DimProduct[ProductID]
- FactSales[RegionID] → DimRegion[RegionID]
- All relationships: single direction, one-to-many.

**Task 3:**
```dax
DimDate = 
ADDCOLUMNS(
    CALENDAR(DATE(2026,1,1), DATE(2026,2,28)),
    "Year", YEAR([Date]),
    "Month", FORMAT([Date], "MMM-YY"),
    "MonthNumber", MONTH([Date])
)
```
Set relationship: FactSales[DateKey] → DimDate[Date]. Mark as date table.

**Task 4:**
```dax
Total TRx = SUM(FactSales[TRx])

Achievement % = DIVIDE(SUM(FactSales[TRx]), SUM(FactSales[TargetTRx]), 0)

Reps Below Target = 
SUMX(
    VALUES(FactSales[RepID]),
    IF(CALCULATE([Achievement %]) < 1, 1, 0)
)

TRx per Call = DIVIDE(SUM(FactSales[TRx]), SUM(FactSales[Calls]), 0)
```

**Task 5:**
```dax
Prev Month TRx = CALCULATE([Total TRx], DATEADD(DimDate[Date], -1, MONTH))
MoM Growth % = DIVIDE([Total TRx] - [Prev Month TRx], [Prev Month TRx])
```

**Task 6:**
```dax
TRx Share = DIVIDE([Total TRx], CALCULATE([Total TRx], ALL(DimRegion)))
```

**Task 7–9:** Build visuals as described. Conditional formatting on Achievement %: ≥100% green, <100% red.

**Task 10 — Model answer:**
"Overall achievement is 103%, but this masks significant regional variation: South leads at 107% driven by R-004's consistent outperformance, while East is the weakest region at 98% with only two reps and limited OncoZ volume. The manager should investigate East's low rep count — adding one high-performing CardioX rep to East could close the gap. Additionally, OncoZ generates 2.7× the revenue per prescription versus CardioX (₹1,200 vs ₹450), yet accounts for only 15% of TRx — consider whether OncoZ is under-promoted or access-limited."

**Scoring guide:**
- 90–100: Assessment-ready.
- 75–89: Solid; practise speed.
- 60–74: Revise DAX and model design.
- Below 60: Redo Part D before retrying.

---

## 20.40 Quick reference: DAX patterns for pharma analysts

| Business question | DAX pattern |
|---|---|
| Total prescriptions | `SUM(Fact[TRx])` |
| Achievement % | `DIVIDE(SUM(Fact[TRx]), SUM(Fact[Target]))` |
| Share of total | `DIVIDE([Measure], CALCULATE([Measure], ALL(Dim)))` |
| Year-over-year | `CALCULATE([Measure], SAMEPERIODLASTYEAR(Date[Date]))` |
| Month-over-month | `CALCULATE([Measure], DATEADD(Date[Date], -1, MONTH))` |
| Count above threshold | `COUNTROWS(FILTER(Table, Table[Col] > X))` |
| Weighted metric | `SUMX(Table, Table[Weight] * Table[Value])` |
| Top N | `TOPN(N, Table, [Measure], DESC)` |
| Dynamic title | `"Title: " & SELECTEDVALUE(Dim[Col], "All")` |
| Unique count | `DISTINCTCOUNT(Fact[ID])` |

---

## 20.41 Checklist before moving on

- [ ] I can explain the difference between Import and DirectQuery
- [ ] I can clean data in Power Query (nulls, duplicates, types, splits)
- [ ] I can design a star schema with fact and dimension tables
- [ ] I understand row context vs filter context
- [ ] I can write SUM, AVERAGE, COUNTROWS, DISTINCTCOUNT, DIVIDE
- [ ] I can use CALCULATE with filters and with ALL
- [ ] I can use DATEADD and SAMEPERIODLASTYEAR for time comparisons
- [ ] I can build KPI cards, bar charts, matrices, and slicers
- [ ] I have completed at least one dashboard project end-to-end
- [ ] I can explain a dashboard's findings in business terms

**Next:** [21-case-study-problem-solving-masterclass.md](21-case-study-problem-solving-masterclass.md)
