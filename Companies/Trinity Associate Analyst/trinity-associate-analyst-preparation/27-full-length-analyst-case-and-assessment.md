# 27 — Full-Length Trinity-Style Integrated Mock Case & Assessment

**Purpose:** a comprehensive 90-minute timed capstone simulation integrating the entire Trinity Associate Analyst skill stack: Business Problem Deconstruction, Excel Data Hygiene, Analytical SQL Window Functions, Survey Statistical Significance, Power BI Star Schema & DAX, and Boardroom Executive Storytelling.
**Prerequisites:** Modules [01](01-quantitative-aptitude.md) through [26](26-client-communication-and-insight-storytelling.md).
**Time limit:** 90 minutes working time (strict countdown timer) + 45 minutes self-grading.
**Provenance:** **PRACTICE INFERENCE (SIMULATION ONLY)**. This is an original training simulation created to replicate the multi-disciplinary rigor, data complexity, and business judgment evaluated in campus analyst hiring drives. It does NOT reproduce any proprietary Trinity or TGaS test material.

---

## Part A — Simulation Briefing & Candidate Instructions

### 27.1 The Business Scenario
You are an Associate Analyst at TGaS Advisors / Trinity Life Sciences. Your client is **Aethelgard Life Sciences**, a specialty pharmaceutical manufacturer. Nine months ago, Aethelgard launched **CardioMax**, an oral targeted therapy for Heart Failure with Preserved Ejection Fraction (HFpEF) across four geographical zones in India.

- **The Commercial Problem:** Management budgeted ₹60.0 Cr for Year 1 (a Year-to-Date Month 9 target of ₹43.2 Cr). Cumulative Year-to-Date net revenue has reached only **₹32.4 Cr**—a 28.0% shortfall (tracking to a ₹16.8 Cr full-year deficit).
- **Executive Board Conflict:**
  - *The National Sales Director* claims sales reps are overwhelmed and demands a **₹4.0 Cr budget to recruit 20 additional sales reps**.
  - *The Marketing VP* claims doctors don't know the clinical data and requests **₹2.0 Cr for national medical journal advertising**.
  - *The Managed Markets Director* claims government schemes and private insurers are blocking formulary inclusion.
- **Your Mission:** The Engagement Lead hands you three raw datasets. You have **90 minutes** to audit the data, write analytical SQL queries, compute survey significance statistics, build the Power BI model, isolate the true root cause, and present a board-ready turnaround strategy to the CEO.

---

### 27.2 Time Allocation & Deliverables Schedule

Set a 90-minute countdown timer and adhere strictly to this checkpoint budget:

| Section | Focus Area | Recommended Time | Deliverables |
|---|---|---|---|
| **Section 1** | Excel Audit & Performance | 00:00 – 00:20 (20 min) | Data cleaning audit, regional variance table, attainment % |
| **Section 2** | Analytical SQL Architecture | 00:20 – 00:40 (20 min) | Rep productivity query, MoM growth window function (`LAG`) |
| **Section 3** | Survey Analytics & Stats | 00:40 – 01:00 (20 min) | Funnel conversion breakdown, two-proportion z-test |
| **Section 4** | Power BI & DAX Modeling | 01:00 – 01:15 (15 min) | Star schema diagram, 4 production DAX measures |
| **Section 5** | Executive C-Suite Pitch | 01:15 – 01:30 (15 min) | Evaluation of ₹4 Cr request, 90-second boardroom pitch |
| **Review** | Self-Scoring & Rubric | 01:30 – 02:15 (45 min) | Score against 100-point rubric in §27.7 |

---

## Part B — The Multi-Dimensional Datasets

The raw practice CSVs are stored in [`data/`](data/):
1. `data/mock_capstone_sales.csv` (36 rows: 9 Months × 4 Regions)
2. `data/mock_capstone_survey.csv` (Summary: 300 surveyed cardiologists)
3. `data/mock_capstone_access.csv` (National payer coverage and formulary tiers)

### Dataset 1: Commercial Sales Extract (`data/mock_capstone_sales.csv`)
*Grain: Month × Region × Product (9 Months of Actuals)*

| Month | Region | Product | Invoiced_Gross_RsCr | GTN_Deduction_Pct | Physical_Units_Packs | Target_TRx_Packs | Sales_Reps | Completed_Calls |
|---|---|---|---|---|---|---|---|---|
| M1 | North | CardioMax | 1.80 | 20.0% | 2,400 | 2,500 | 15 | 1,200 |
| M2 | North | CardioMax | 2.10 | 20.0% | 2,800 | 2,800 | 15 | 1,250 |
| M3 | north | CardioMax | 2.50 | 22.0% | 3,250 | 3,100 | 15 | 1,220 |
| M4 | North | CardioMax | 2.80 | 22.0% | 3,640 | 3,500 | 15 | 1,240 |
| M5 | North | CardioMax | 3.10 | 22.0% | 4,030 | 3,900 | 15 | 1,260 |
| M6 | North | CardioMax | 3.40 | 25.0% | 4,250 | 4,200 | 15 | 1,280 |
| M7 | North | CardioMax | 3.70 | 25.0% | "4,625" | 4,500 | 15 | 1,250 |
| M8 | North | CardioMax | 4.00 | 25.0% | 5,000 | 4,800 | 15 | 1,270 |
| M9 | North | CardioMax | 4.30 | 25.0% | 5,375 | 5,100 | 15 | 1,290 |
| M1–M9 | South | CardioMax | 18.20 (Total) | 20.0% (Avg) | 24,266 | 24,000 | 18 | 16,200 |
| M1–M9 | East | CardioMax | 7.20 (Total) | 18.0% (Avg) | 10,000 | 12,000 | 10 | 8,500 |
| M1–M9 | West | CardioMax | 10.40 (Total) | 35.0% (Avg) | 11,266 | 20,000 | 16 | 17,600 |

*Data Defects Injected for Cleaning:*
- Row M3 North has region recorded with trailing whitespace (`"north "`).
- Row M7 North has physical packs stored as string text with a comma (`"4,625"`).
- West region exhibits extreme gross-to-net fluctuations rising from 30% to 40% in M8–M9.

---

### Dataset 2: Physician Launch Evaluation Survey (`data/mock_capstone_survey.csv`)
*Sample: 300 Cardiologists surveyed at Month 9 post-launch*

| Metric / Question | Total Sample (N = 300) | Metro Corporate Hospitals (n = 120) | Tier-2 / Independent Clinics (n = 180) |
|---|---|---|---|
| Aided Brand Awareness (%) | 91.0% (273 docs) | 95.0% (114 docs) | 88.3% (159 docs) |
| Stated Willingness to Trial (%) | 74.0% (222 docs) | 85.0% (102 docs) | 66.7% (120 docs) |
| Actual Written Prescriptions (Ever Rx %) | 32.0% (96 docs) | 58.3% (70 docs) | 14.4% (26 docs) |
| Regular Prescribers (>5 Rx/Month) | 14.0% (42 docs) | 29.2% (35 docs) | 3.9% (7 docs) |
| Top Stated Barrier #1 | Prior Auth / Co-Pay (58%) | Hospital Formulary Rejection (64%) | Patient Out-of-Pocket Cost (72%) |
| Top Stated Barrier #2 | Inadequate Efficacy Data (12%)| Lack of Nurse Training (18%) | Local Pharmacy Stockouts (22%) |

---

### Dataset 3: Payer & Formulary Access Matrix (`data/mock_capstone_access.csv`)
| Payer Channel | Universe Size | CardioMax Status | Competing Brand A | Competing Brand B |
|---|---|---|---|---|
| Corporate Private Insurers | 12 Million Lives | Tier 2 (Co-Pay: ₹600/Mo) | Tier 2 (Co-Pay: ₹600/Mo) | Tier 3 (Co-Pay: ₹2,400/Mo) |
| Government Schemes (CGHS/ESI)| 28 Million Lives | Excluded (Pending Tender)| Listed (National Tender) | Listed (National Tender) |
| Major Hospital Formularies | Top 80 Hospital Chains | Listed in 32 of 80 (40%) | Listed in 74 of 80 (92%) | Listed in 65 of 80 (81%) |

---

## Part C — The 10 Candidate Assessment Tasks

> **EXAM INSTRUCTIONS:** Complete Tasks 1 through 10 independently. Write your answers, SQL scripts, formulas, and presentations on paper or in a clean workbook before inspecting the solutions in Part D.

---

### Section 1: Excel Data Hygiene & Commercial Variance (20 Mins | 20 Pts)

#### Task 1: Data Quality Audit & Cleaning Functions (10 Pts)
Inspect `sales_extract` (Dataset 1).
1. Name the **three distinct data hygiene defects** present in the raw data.
2. For each defect, write the exact **Excel formula** or **Power Query transformation** required to clean it.
3. Explain what silent calculation error would occur if these defects were not corrected prior to analysis.

#### Task 2: Regional Performance Reconciliation & Variance Isolation (10 Pts)
Using the clean data:
1. Calculate the 9-month cumulative **Net Revenue** for North, South, East, and West:
   $$\text{Net Revenue} = \text{Gross Invoiced Sales} \times (1 - \text{GTN Deduction Pct})$$
2. Calculate the **Physical Volume Attainment %** against target for each region:
   $$\text{Attainment \%} = \frac{\text{Actual Physical Units}}{\text{Target TRx Packs}} \times 100$$
3. Reconcile total national revenue and volume. Which specific region is responsible for the national deficit?

---

### Section 2: Analytical SQL Architecture (20 Mins | 20 Pts)

#### Task 3: Sales Rep Detailing Productivity SQL (10 Pts)
Write an ANSI SQL query against `sales_extract` (joined with a hypothetical `dim_reps` table on `Region`) that computes for each region:
- Total sales representatives (`COUNT(DISTINCT r.RepID)`).
- Total completed calls (`SUM(s.Completed_Calls)`).
- Total physical units sold (`SUM(s.Physical_Units_Packs)`).
- Average calls completed per sales representative.
- **Detailing Productivity Ratio:** Physical units sold per completed sales call.
Sort the output descending by Detailing Productivity Ratio.

#### Task 4: Month-over-Month Growth Using Window Functions (10 Pts)
Write an ANSI SQL query using the `LAG()` analytic window function to calculate:
- The monthly Net Revenue for the **North region** across Months 1 through 9.
- The previous month's Net Revenue.
- The Month-over-Month (MoM) Net Revenue Growth Rate (percentage formatted to 2 decimal places).

---

### Section 3: Survey Analytics & Statistical Testing (20 Mins | 20 Pts)

#### Task 5: Prescriber Adoption Funnel Attrition Analysis (10 Pts)
Using Dataset 2 (`survey_responses`):
1. Calculate the stage-to-stage transition conversion rate for:
   - Aided Awareness $\rightarrow$ Willingness to Trial.
   - Willingness to Trial $\rightarrow$ Actual Written Rx (Ever).
   - Actual Written Rx $\rightarrow$ Regular Prescriber.
2. Compare Metro Hospitals against Independent Clinics across all three transition stages.
3. Identify the exact stage where the commercial adoption funnel suffers its most catastrophic collapse.

#### Task 6: Two-Proportion Hypothesis Testing (10 Pts)
In Dataset 2, 58.3% of Metro Hospital cardiologists (70 of 120) wrote an actual prescription, compared to only 14.4% of Independent Clinic cardiologists (26 of 180).
1. State the Null ($H_0$) and Alternative ($H_1$) hypotheses.
2. Calculate the pooled sample proportion ($\bar{p}$) and pooled Standard Error ($SE$).
3. Calculate the test statistic ($z$).
4. At a 95% confidence level ($\alpha = 0.05, z_{\text{critical}} = 1.96$), state your statistical conclusion and explain what it means for commercial strategy.

---

### Section 4: Power BI Modeling & Production DAX (15 Mins | 20 Pts)

#### Task 7: Star Schema Architecture Design (10 Pts)
Design an optimal Power BI data model connecting the capstone datasets:
1. Identify the **Fact Table** and its primary grain.
2. Define three supporting **Dimension Tables** and their Primary Keys (PK).
3. Specify the foreign keys connecting each dimension to the fact table.
4. Define the cardinality (1:N or M:N) and filter cross-direction for each relationship.

#### Task 8: Production DAX Formulations (10 Pts)
Write production-ready DAX measure formulas for:
1. `[Net Revenue]` (row-by-row iteration accounting for GTN).
2. `[Attainment Pct]` (safe division handling zero targets).
3. `[Sales per Rep]` (attributable revenue per distinct active representative).
4. `[High Margin Volume Pct]` (percentage of physical units sold where GTN deduction was $< 25\%$).

---

### Section 5: Root-Cause Synthesis & Executive Presentation (15 Mins | 20 Pts)

#### Task 9: Critique of Headcount Expansion Request (10 Pts)
The National Sales Director demands ₹4.0 Cr to hire 20 additional sales reps to fix the revenue shortfall.
1. Use your findings from Tasks 2, 3, and 5 to **critique and reject this request**.
2. Explain why adding 20 reps will worsen company EBIT without solving the commercial gap.

#### Task 10: 90-Second C-Suite Boardroom Pitch (10 Pts)
Deliver a 90-second executive presentation to the CEO and Board of Directors using the **EAR-KPI** consulting structure:
- **Explicit Action:** What must be done, by whom, and within what timeframe.
- **Anchor Evidence:** The specific calculations proving this is the correct diagnosis.
- **Return & Financial Impact:** Quantified revenue recovery and net ROI.
- **Risk & Mitigation:** Anticipated execution risks and how to hedge them.
- **Key Performance Indicators:** 2 leading and 1 lagging governance metrics.

---

## Part D — Detailed Step-by-Step Answer Key & Verified Solutions

---

### §27.4.1 Solution to Section 1: Excel Data Hygiene & Commercial Variance

#### Task 1: Data Quality Audit
1. **Defect 1: Trailing Space in Region Name (`"north "` in Row M3):**
   - *Excel Fix:* `=TRIM(B4)`.
   - *Power Query Fix:* `Table.TransformColumns(Source, {{"Region", Text.Trim, type text}})`.
   - *Impact if ignored:* PivotTables and `SUMIFS` treat `"North"` and `"north "` as two separate categories, under-reporting North sales by ₹2.50 Cr (11.7%).
2. **Defect 2: Number Stored as Text with Comma (`"4,625"` in Row M7):**
   - *Excel Fix:* `=VALUE(SUBSTITUTE(F8, ",", ""))`.
   - *Power Query Fix:* `Table.TransformColumnTypes(Source, {{"Physical_Units_Packs", Int64.Type}})`.
   - *Impact if ignored:* Excel’s `=SUM()` silently ignores text strings, dropping 4,625 packs from national volume totals without throwing an error.
3. **Defect 3: Erratic GTN Margin Drift in West Zone (Surging to 40% in M8–M9):**
   - *Excel Fix:* Add a validation audit column: `=IF(E2>0.30, "REVIEW GTN", "NORMAL")`.
   - *Impact if ignored:* High gross sales mask catastrophic net price erosion, inflating reported performance.

---

#### Task 2: Regional Performance Reconciliation Table

**Calculations by Region:**
- **North (Months 1–9):**
  - Gross Sales = $1.80 + 2.10 + 2.50 + 2.80 + 3.10 + 3.40 + 3.70 + 4.00 + 4.30 = \text{₹}27.70\text{ Cr}$.
  - Net Revenue = $(1.80 \times 0.80) + (2.10 \times 0.80) + (2.50 \times 0.78) + (2.80 \times 0.78) + (3.10 \times 0.78) + (3.40 \times 0.75) + (3.70 \times 0.75) + (4.00 \times 0.75) + (4.30 \times 0.75) = 1.44 + 1.68 + 1.95 + 2.184 + 2.418 + 2.55 + 2.775 + 3.00 + 3.225 = \mathbf{\text{₹}21.222\text{ Cr}}$.
  - Physical Units = $2,400 + 2,800 + 3,250 + 3,640 + 4,030 + 4,250 + 4,625 + 5,000 + 5,375 = \mathbf{35,370\text{ packs}}$.
  - Target Units = $2,500 + 2,800 + 3,100 + 3,500 + 3,900 + 4,200 + 4,500 + 4,800 + 5,100 = \mathbf{34,400\text{ packs}}$.
  - Attainment % = $\frac{35,370}{34,400} = \mathbf{102.8\%}$.
- **South (Months 1–9):**
  - Gross Sales = ₹18.20 Cr; Net Revenue = $18.20 \times (1 - 0.20) = \mathbf{\text{₹}14.56\text{ Cr}}$.
  - Physical Units = 24,266 packs; Target = 24,000 packs; Attainment % = $\frac{24,266}{24,000} = \mathbf{101.1\%}$.
- **East (Months 1–9):**
  - Gross Sales = ₹7.20 Cr; Net Revenue = $7.20 \times (1 - 0.18) = \mathbf{\text{₹}5.904\text{ Cr}}$.
  - Physical Units = 10,000 packs; Target = 12,000 packs; Attainment % = $\frac{10,000}{12,000} = \mathbf{83.3\%}$.
- **West (Months 1–9):**
  - Gross Sales = ₹10.40 Cr; Net Revenue = $10.40 \times (1 - 0.35) = \mathbf{\text{₹}6.760\text{ Cr}}$.
  - Physical Units = 11,266 packs; Target = 20,000 packs; Attainment % = $\frac{11,266}{20,000} = \mathbf{56.3\%}$.

#### Master Reconciled Performance Table:
| Region | Invoiced Gross (₹ Cr) | Blended GTN % | Net Revenue (₹ Cr) | Target TRx (Packs) | Actual TRx (Packs) | Volume Attainment % |
|---|---|---|---|---|---|---|
| **North** | ₹27.70 Cr | 23.4% | ₹21.22 Cr | 34,400 | 35,370 | **102.8%** |
| **South** | ₹18.20 Cr | 20.0% | ₹14.56 Cr | 24,000 | 24,266 | **101.1%** |
| **East** | ₹7.20 Cr | 18.0% | ₹5.90 Cr | 12,000 | 10,000 | **83.3%** |
| **West** | ₹10.40 Cr | 35.0% | ₹6.76 Cr | 20,000 | 11,266 | **56.3%** |
| **Total** | **₹63.50 Cr** | **23.7%** | **₹48.44 Cr** | **90,400** | **80,902** | **89.5%** |

*Deficit Isolation:*
North and South delivered 102% attainment. **The West zone accounts for 83% of the total national volume shortfall (8,734 unearned packs out of 9,498 total gap).**

---

### §27.4.2 Solution to Section 2: Analytical SQL Architecture

#### Task 3: Detailing Productivity SQL
```sql
SELECT 
    s.Region,
    COUNT(DISTINCT r.RepID) AS Total_Reps,
    SUM(s.Completed_Calls) AS Total_Calls,
    SUM(s.Physical_Units_Packs) AS Total_Units_Packs,
    -- Calls per Rep
    ROUND(CAST(SUM(s.Completed_Calls) AS FLOAT) / COUNT(DISTINCT r.RepID), 1) AS Calls_Per_Rep,
    -- TRx per Call Productivity
    ROUND(CAST(SUM(s.Physical_Units_Packs) AS FLOAT) / SUM(s.Completed_Calls), 3) AS TRx_Per_Call
FROM sales_extract s
JOIN dim_reps r ON TRIM(LOWER(s.Region)) = TRIM(LOWER(r.Region))
GROUP BY s.Region
ORDER BY TRx_Per_Call DESC;
```

**Expected Query Output:**
| Region | Total Reps | Total Calls | Total Units Packs | Calls / Rep | TRx per Call |
|---|---|---|---|---|---|
| **North** | 15 | 11,240 | 35,370 | 749.3 | **3.147** |
| **South** | 18 | 16,200 | 24,266 | 900.0 | **1.498** |
| **East** | 10 | 8,500 | 10,000 | 850.0 | **1.176** |
| **West** | 16 | 17,600 | 11,266 | **1,100.0** | **0.640** |

*Insight:* West reps logged **1,100 calls/rep (highest in India)**, but produced only **0.64 TRx/call (5x lower productivity than North)**.

---

#### Task 4: MoM Growth Query Using `LAG()`
```sql
WITH NorthMonthly AS (
    SELECT 
        Month,
        CAST(SUBSTITUTE(Physical_Units_Packs, ',', '') AS INTEGER) AS Clean_Units,
        Invoiced_Gross_RsCr * (1 - GTN_Deduction_Pct) AS Net_Revenue
    FROM sales_extract
    WHERE TRIM(LOWER(Region)) = 'north'
)
SELECT 
    Month,
    ROUND(Net_Revenue, 3) AS Current_Net_Rev_Cr,
    ROUND(LAG(Net_Revenue, 1) OVER (ORDER BY Month), 3) AS Prior_Net_Rev_Cr,
    ROUND(
        (Net_Revenue - LAG(Net_Revenue, 1) OVER (ORDER BY Month)) /
        LAG(Net_Revenue, 1) OVER (ORDER BY Month) * 100, 
        2
    ) AS MoM_Growth_Pct
FROM NorthMonthly;
```

---

### §27.4.3 Solution to Section 3: Survey Analytics & Statistical Testing

#### Task 5: Funnel Conversion Breakdown

$$\text{Transition Conversion Rate} = \frac{\text{Current Stage Count}}{\text{Prior Stage Count}}$$

| Transition Stage | Total Sample (N = 300) | Metro Hospitals (n = 120) | Independent Clinics (n = 180) | Gap / Diagnostic |
|---|---|---|---|---|
| **Aided Awareness $\rightarrow$ Trial Intent** | $74.0\% / 91.0\% = \mathbf{81.3\%}$ | $85.0\% / 95.0\% = \mathbf{89.5\%}$ | $66.7\% / 88.3\% = \mathbf{75.5\%}$ | -14.0 pp gap; trial intent is healthy |
| **Trial Intent $\rightarrow$ Written Rx (Ever)** | $32.0\% / 74.0\% = \mathbf{43.2\%}$ | $58.3\% / 85.0\% = \mathbf{68.6\%}$ | $14.4\% / 66.7\% = \mathbf{21.6\%}$ | **-47.0 pp COLLAPSE IN CLINICS!** |
| **Written Rx $\rightarrow$ Regular Prescriber** | $14.0\% / 32.0\% = \mathbf{43.8\%}$ | $29.2\% / 58.3\% = \mathbf{50.1\%}$ | $3.9\% / 14.4\% = \mathbf{27.1\%}$ | Retention weak in small practices |

*Diagnostic:* The catastrophic funnel collapse occurs at **Trial Intent $\rightarrow$ Written Rx in Clinics (only 21.6% conversion)**.
*Root Cause:* 72% cite "Patient Out-of-Pocket Co-Pay" (retail price exceeds ₹2,000/mo) and 22% cite local retail pharmacy stockouts.

---

#### Task 6: Two-Proportion Hypothesis Testing
- Metro Hospital Proportion ($p_1$): $70 / 120 = 0.5833$.
- Independent Clinic Proportion ($p_2$): $26 / 180 = 0.1444$.
- Observed Difference: $p_1 - p_2 = 0.5833 - 0.1444 = \mathbf{0.4389}$ (43.89 percentage points).

**Pooled Proportion ($\bar{p}$):**
$$\bar{p} = \frac{X_1 + X_2}{n_1 + n_2} = \frac{70 + 26}{120 + 180} = \frac{96}{300} = \mathbf{0.320}$$

**Pooled Standard Error ($SE$):**
$$SE = \sqrt{\bar{p}(1 - \bar{p}) \left(\frac{1}{n_1} + \frac{1}{n_2}\right)} = \sqrt{0.32 \times 0.68 \times \left(\frac{1}{120} + \frac{1}{180}\right)} = \sqrt{0.2176 \times 0.013889} = \sqrt{0.003022} \approx \mathbf{0.05497}$$

**Test Statistic ($z$):**
$$z = \frac{0.4389}{0.05497} = \mathbf{7.984}$$

*Statistical Conclusion:*
At $\alpha = 0.05$, $z_{\text{critical}} = 1.96$. Since $z = 7.984 \gg 1.96$ ($p < 0.0001$), we **reject the null hypothesis**. Metro hospital specialists convert to active trial at a fundamentally, statistically higher rate than clinic practitioners.

---

### §27.4.4 Solution to Section 4: Power BI Modeling & Production DAX

#### Task 7: Star Schema Data Model
```
                  Dim_Product (ProductID PK)
                      |
                      | 1:N (Single Direction)
Dim_Region (RegionID PK) ---- Fact_Sales (SaleID PK) ---- Dim_Calendar (DateKey PK)
                      | 1:N
                      |
                  Dim_Rep (RepID PK)
```
- **Fact_Sales:** Transaction grain = Month × Region × Product. Foreign keys: `RegionID`, `ProductID`, `RepID`, `DateKey`.
- **Dim_Calendar:** DateKey (marked as official Date Table for time intelligence).
- **Enforce Single Direction Filter Flows** (Dimensions $\rightarrow$ Fact) to optimize VertiPaq compression.

#### Task 8: Production DAX Formulations
```dax
// 1. Net Revenue Measure
Net Revenue = 
SUMX(
    Fact_Sales,
    Fact_Sales[Invoiced_Gross_RsCr] * (1 - Fact_Sales[GTN_Deduction_Pct])
)

// 2. Volume Attainment % Measure
Attainment Pct = 
DIVIDE(
    SUM(Fact_Sales[Physical_Units_Packs]),
    SUM(Fact_Sales[Target_TRx_Packs]),
    0
)

// 3. Sales per Representative
Sales Per Rep = 
DIVIDE(
    [Net Revenue],
    DISTINCTCOUNT(Fact_Sales[RepID]),
    0
)

// 4. High Margin Volume % (<25% GTN)
High Margin Volume Pct = 
DIVIDE(
    CALCULATE(
        SUM(Fact_Sales[Physical_Units_Packs]),
        Fact_Sales[GTN_Deduction_Pct] < 0.25
    ),
    SUM(Fact_Sales[Physical_Units_Packs]),
    0
)
```

---

### §27.4.5 Solution to Section 5: Root-Cause Synthesis & Executive Pitch

#### Task 9: Critique of Headcount Expansion Request
**The Sales Director’s request to hire 20 reps for ₹4.0 Cr must be rejected unconditionally.**
*Quantitative Proof:*
1. **Reps Are Not Under-Staffed:** West zone reps completed **1,100 calls/rep**, which is 47% higher than North (749 calls/rep).
2. **Productivity is Collapsing:** West generates only **0.64 TRx/call vs. 3.15 TRx/call in North**. Adding 20 reps to a broken detailing model will merely increase fixed cost burn by ₹4 Cr without generating prescriptions.
3. **True Bottleneck is Hospital Exclusion:** CardioMax is listed in only **40% of major hospital formularies** (Dataset 3). In Mumbai, reps were detailing doctors whose hospital pharmacies literally did not stock the medicine.

---

#### Task 10: 90-Second C-Suite Boardroom Script (EAR-KPI Format)

> *"Good morning, members of the Board. CardioMax’s ₹10.8 Cr revenue shortfall is not a failure of clinical demand, nor is it a sales rep motivation problem. Aided awareness is 91%, and North and South both exceeded 100% attainment.
> 
> The analytical root cause is a dual access bottleneck: **Hospital Formulary Exclusion** and **Clinic Co-Pay Friction**. In the West region, reps completed 17,600 calls—the highest in India—but produced 5x lower TRx per call because CardioMax is excluded from the top 3 Mumbai hospital networks. In independent clinics, 78% of willing prescribers abandon therapy because patients face a ₹2,400 monthly out-of-pocket co-pay.
> 
> **Our strategic recommendation is three-fold:**
> 1. **Reject the ₹4.0 Cr request for 20 additional reps.**
> 2. **Establish a dedicated 4-person Hospital Market Access Team** (Budget: ₹80 Lakhs) to secure formulary listings in the top 35 excluded hospital systems within 120 days.
> 3. **Deploy a Digital Patient Co-Pay Bridge Voucher** (Budget: ₹1.0 Cr) capping clinic patients' out-of-pocket costs at ₹500 for the first month, while partnering with 3 national retail pharmacy chains.
> 
> This targeted ₹1.8 Cr intervention will save ₹2.2 Cr compared to hiring reps and recapture ₹12.0 Cr in annualized net revenue with a 4.5-month payback period. We will govern execution through three KPIs: hospital formulary submission velocity (35 dossiers by Week 6), rep call share in listed accounts (>85%), and monthly net revenue run-rate."*

---

## Part E — 100-Point Detailed Scoring Rubric

Evaluate your performance strictly against this breakdown:

| Section | Tasks | Max Points | Passing Bar | Detailed Evaluation Criteria |
|---|---|---|---|---|
| **1. Excel Hygiene & Variance** | Tasks 1–2 | 20 Pts | 16 Pts | Caught trailing space, comma text-number, and GTN drift (10 pts); correct net revenue math across all 4 regions; identified West as driving 83% of volume gap (10 pts). |
| **2. SQL Analytics** | Tasks 3–4 | 20 Pts | 15 Pts | Correct multi-table join and grouping; accurate TRx/call productivity calculation (10 pts); flawless `LAG()` window function syntax with safe growth rate math (10 pts). |
| **3. Survey Statistics** | Tasks 5–6 | 20 Pts | 15 Pts | Correct stage-to-stage transition conversion rates identifying the 47 pp clinic collapse (10 pts); correct two-proportion z-statistic ($z = 7.98$) with hypothesis and p-value inference (10 pts). |
| **4. Power BI Modeling & DAX** | Tasks 7–8 | 20 Pts | 15 Pts | Clean normalized Star Schema diagram with single-direction flows (10 pts); robust DAX measures using `DIVIDE`, `SUMX`, and `CALCULATE` (10 pts). |
| **5. Synthesis & Boardroom Pitch** | Tasks 9–10 | 20 Pts | 16 Pts | Rejected headcount expansion with quantitative proof (10 pts); delivered crisp, persuasive 90-second EAR-KPI C-suite pitch (10 pts). |
| **TOTAL** | **10 Tasks** | **100 Pts** | **77 Pts** | **Target: $\ge 85$ for Strong Hire recommendation.** |

---

## Part F — Master Simulation Readiness Checklist

- [ ] I attempted the simulation under a strict 90-minute countdown timer.
- [ ] I cleaned text numbers, trailing spaces, and invalid categories in Excel without looking at the key.
- [ ] I wrote multi-table analytical SQL queries using window functions (`LAG`).
- [ ] I performed a two-proportion z-test ($z = 7.98$) and explained its business meaning.
- [ ] I designed a normalized Star Schema and authored production DAX measures.
- [ ] I debunked an operational headcount expansion request using productivity math.
- [ ] I delivered a 90-second executive synthesis aloud using the EAR-KPI architecture.

**Next Module:** [28-weakness-based-practice-and-error-log.md](28-weakness-based-practice-and-error-log.md)
