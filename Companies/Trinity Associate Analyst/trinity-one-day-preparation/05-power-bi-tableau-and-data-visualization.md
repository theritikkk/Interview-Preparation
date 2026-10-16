# 05 — Power BI, Tableau & Data Visualization

> **Learning objectives:** explain what Power BI and Tableau are, when each is used, and how a dashboard is built — from data connection to a shareable report. Enough to answer conceptual and scenario interview questions confidently, and to build a first dashboard if you get 30 spare minutes.
> **Estimated study time:** 45 min.
> **Priority:** **SHOULD KNOW** (conceptual) + **AWARENESS ONLY** for tool mechanics you haven't touched. The JD (CONFIRMED) explicitly lists *"creating dynamic dashboards with live data connections in visualization tools like Power BI and Tableau"* — expect at least one conceptual question.
> **Honesty rule:** reading this does **not** make you proficient in either tool. In interviews, say what you've built (even one practice dashboard) and stay conceptual beyond that.

---

## §5.1 What BI tools are for (shared foundation)

A BI tool turns raw tables into **live, interactive views**: connect → clean → model → visualize → share/refresh. Opposite of a static PowerPoint chart: **one definition of a metric, reused everywhere, updating as data updates**.

**ETL (Extract–Transform–Load):** get data from sources (Extract), clean/shape it (Transform), load into the model. In Power BI, the Transform step is **Power Query**; in Tableau it's **Data Interpreter / Prep**.

**Data model & relationships:** separate tables (sales, reps, products) linked by keys (RepID, Product). Star schema intuition: fact table (TRx by rep-month) + dimension tables (rep, product, region). Good models avoid duplicated data and enable one filter to affect every visual.

## §5.2 Power BI from first principles

| Piece | What it is | Interview line |
|---|---|---|
| Power BI Desktop | Free Windows app: build reports locally | "I'd prototype in Desktop" |
| Reports & pages | A report = canvas of visuals; pages = tabs | — |
| **Published dashboard** | Pinned visuals from one/more reports, in the Service | Dashboards = monitored KPIs; reports = exploration |
| Power BI Service | Cloud: share, schedule refresh, permissions | "Live connection + scheduled refresh" |
| Connect to data | Excel, CSV, SQL, web, APIs | Get Data → Excel/Text-CSV |
| Power Query | Visual data-cleaning step recorder (rename, split, unpivot, filter, remove dup) | "The Transform in ETL — repeatable, not manual" |
| Relationships | Drag key-to-key links between tables in Model view | "One filter, all visuals" |
| **Measure vs calculated column** | Measure = computed on the fly at display time (not stored); calculated column = computed per row at refresh | "Use measures for aggregates; columns for row-level attributes" |
| Refresh | Scheduled/live refresh from source | Answer to "is the data current?" |

**DAX fundamentals (conceptual + syntax recognition):**

| DAX | What it does | Example |
|---|---|---|
| `SUM` | Total of a column | `Total TRx = SUM(sales[TRx])` |
| `COUNTROWS` | Row count of a table | `# Reps = COUNTROWS(reps)` |
| `DISTINCTCOUNT` | Unique count | `Active Products = DISTINCTCOUNT(sales[Product])` |
| `DIVIDE` | Safe division (handles /0) | `MoM % = DIVIDE([Feb TRx] − [Jan TRx], [Jan TRx])` |
| `CALCULATE` | Re-evaluates a measure under modified filters — *the* power function | `CardioX TRx = CALCULATE([Total TRx], products[Product]="CardioX")` |

**Visuals & interactions:** KPI card (single number + trend), bar/column (comparisons), line (time trend), stacked bar (part-of-whole for few categories), matrix (pivot-like grid), slicers & filters (user slicing), drill-down (region → territory).

**Report design rules:** one page = one question; KPI cards on top, breakdowns below; ≤ 5–7 visuals per page; consistent color meaning (one color = one dimension, e.g., product); label units and freshness ("Jan–Feb 2026, refreshed daily"); title = insight.

## §5.3 A realistic pharma sales dashboard (example build)

Scenario (**PRACTICE** but modeled on the role): monthly rep performance report replacing an Excel+email cycle.
- **Data:** `sales.csv` (RepID, Product, Month, TRx, TargetTRx, Calls) + product & region tables.
- **KPIs:** Total TRx **23,113**; % vs target **92.5%** (23,113/25,000); MoM growth **−2.3%** (Jan 11,688 → Feb 11,425); CardioX mix **60.5%**.
- **Flow:** Get Data → Power Query (trim, fix region variants, type TRx numeric, remove a duplicate) → model relationships (RepID, Product) → measures (Total TRx, Attainment, MoM %) → page: KPI cards row; column chart TRx by Region; stacked column Region × Product; line Jan–Feb by product; matrix Rep × Product with data bars; slicers Region + Product.
- **Refresh story:** files on SharePoint → scheduled refresh daily; data-quality check card (blank TRx count) on the page.

## §5.4 Tableau from first principles

| Piece | What it is | Interview line |
|---|---|---|
| Tableau Desktop | Local authoring app; drag-and-drop | "Fast exploratory analysis" |
| Connect | CSV, Excel, SQL, cloud warehouses. **Data Interpreter** auto-cleans headers | — |
| **Dimensions vs measures** | Dimensions = qualitative fields you slice by (Region, Product — usually blue); Measures = numbers you aggregate (TRx — usually green) | The #1 conceptual question |
| Discrete vs continuous | Discrete = separate values/headers; continuous = an axis | Dates: prefer continuous for trends |
| Marks / shelves | Rows, Columns, Marks card (color, size, label, detail) | — |
| Filters | Dimensions/measures; context vs normal filters; interactive filter actions on dashboards | "Users slice themselves" |
| Calculated fields | Formulas (e.g., `SUM([TRx])/SUM([Target])`) | — |
| Aggregation | Default SUM; change to AVG/COUNTD in the pill | — |
| Charts | Bars, lines, scatter, heatmap, maps (geography), tree maps | Pick by question type |
| Dashboards & stories | Multiple views + actions (filter, highlight, URL); stories = narrative pages | "Dashboards for monitoring, stories for telling" |

## §5.5 Power BI vs Tableau (memorize this table)

| Aspect | Power BI | Tableau |
|---|---|---|
| Owner | Microsoft | Salesforce (Tableau) |
| Strength | Tight Office/Excel/SharePoint integration; DAX; cheaper at scale | Visual exploration speed; richer chart polishing; drag-and-drop |
| Data prep | Power Query (best-in-class built-in ETL) | Data Interpreter + Tableau Prep |
| Language | DAX | Calculated fields (similar feel to Excel) |
| Cost signal | Bundled with enterprise Microsoft deals | Historically premium pricing |
| Community / jobs | Huge, enterprise-heavy | Huge, analytics/visualization-heavy |
| When chosen | Microsoft-centric client, scheduled reporting | Fast ad-hoc exploration & visual design |

**Safe interview answer:** *"Both do the same job — live, interactive reporting. I'd choose Power BI when the client is Microsoft-heavy and reports need scheduled refresh; Tableau when exploration speed and visual polish matter. The skill transfers between them."*

## §5.6 Chart selection guide (also used in files [`08`](08-powerpoint-and-business-communication.md) and [`11`](11-final-revision-cheatsheet.md))

| Question | Chart |
|---|---|
| Trend over time | Line |
| Compare categories | Horizontal/vertical bar (sorted!) |
| Part of whole | Stacked bar (≤ 5 segments); pie only for 2–3 slices |
| Two-dimensional comparison | Clustered/stacked column (region × product) |
| Relationship between two metrics | Scatter |
| Distribution | Histogram / box plot |
| Rank / concentration | Sorted bar or Pareto (bars + cumulative line) |
| Geography | Map (only if location matters) |

**Misleading visuals to call out in interviews:** truncated y-axis (exaggerates change), pie with 8 slices, 3-D anything, inconsistent scales across small multiples, dual axes implying a relationship, cherry-picked time windows.

## §5.7 Dashboard specification exercise (do this from memory)

**Spec: "Monthly TRx performance" (3 min to write):**
- **KPIs:** Total TRx · % of target · MoM growth · top product share.
- **Dimensions:** Region, Product, Specialty, Month.
- **Filters:** Region, Product, Month. **Drill-down:** Region → Rep.
- **Charts:** KPI cards (top row); column by region; stacked column region×product; line by month.
- **Data checks:** duplicates removed, blanks counted and excluded, region values standardized.
- **Story:** "CardioX = 60.5% of TRx; attainment 92.5%; Feb dip −2.3% driven by North." (Numbers match Mock B/[`10`](10-mock-assessment-and-excel-test.md).)

## §5.8 Likely interview questions (concise answers)

1. **What is a measure vs a calculated column?** Measure = aggregate computed at query time on filters; column = row-by-row stored at refresh. Use measures for KPIs, columns for row attributes/flags.
2. **What does CALCULATE do?** Changes the filter context of a measure — powers "share of product", "this year vs last year" style calcs.
3. **Dimensions vs measures in Tableau?** Dimensions slice/group (products, regions); measures are numeric and aggregated (SUM of TRx).
4. **How would you build a monthly sales dashboard?** (Use §5.7 spec — connect → clean → model → measures → visuals → slicers → refresh + QC card.)
5. **How do you keep dashboards trustworthy?** One metric definition source; freshness stamp; blank/duplicate checks; validation against a manual total (like Mock B's cross-foot: every cut sums to 23,113).
6. **Power BI vs Tableau?** §5.5.

## §5.9 Beginner vs hands-on checklist

| Know without touching (this file) | Requires actual practice |
|---|---|
| What the tools do; Desktop vs Service; report vs dashboard | Building relationships and measures in the real UI |
| Dimensions vs measures; DAX reading (`SUM`, `DIVIDE`, `CALCULATE`) | Writing complex DAX debugging filter context |
| Chart choice & dashboard design rules | Advanced interactions, bookmarks, performance tuning |
| ETL concept, refresh concept | Power Query transformation authoring |

**If you have 30 free minutes:** install Power BI Desktop (free) → Get Data → CSV ([`10`](10-mock-assessment-and-excel-test.md) dataset) → one KPI card, one column chart, one slicer. That's a real "I built X" answer.

**Next:** [06 — SQL, Python & automation](06-sql-python-and-automation-for-analysts.md)
