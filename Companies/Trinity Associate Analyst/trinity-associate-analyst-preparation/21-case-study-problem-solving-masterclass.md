# 21 — Case Study Problem-Solving Masterclass

**Purpose:** transform your approach to business case interviews from ad-hoc guesswork into structured, hypothesis-driven, evidence-based problem solving. This module is designed specifically for an analyst facing ambiguous pharmaceutical and healthcare commercial business cases.
**Prerequisites:** [01-quantitative-aptitude.md](01-quantitative-aptitude.md), [03-data-interpretation-and-graphs.md](03-data-interpretation-and-graphs.md), [06-statistics-and-survey-analytics.md](06-statistics-and-survey-analytics.md), [07-business-analytics-and-market-research.md](07-business-analytics-and-market-research.md), [08-life-sciences-industry-fundamentals.md](08-life-sciences-industry-fundamentals.md).
**Suggested study time:** 16–20 hours across 5–6 focused study blocks. Solve each case independently on paper before checking the answer key.
**Provenance:** All cases, companies, products, and survey datasets in this module are **original fictional scenarios created for practice (PRACTICE INFERENCE)**. None of these represent actual confidential client data or official Trinity interview papers.

---

## Part A — Problem Structuring Masterclass

### 21.1 How to Handle Unfamiliar Business Problems
When an interviewer hands you a problem like *"Our specialty neurology brand's sales dropped 18% last year—find out why and fix it"*, technical candidates often panic because they don't know neurology or pharmaceutical distribution.

**The Golden Rule of Case Interviews:**
> **You are not being evaluated on whether you know the trivia of a specific disease state. You are being evaluated on how methodically you break down an ambiguous business situation, generate testable hypotheses, isolate root causes with quantitative rigor, and formulate defensible commercial recommendations.**

The universal 5-step problem-solving loop:
1. **Understand & Clarify:** Listen actively, take structured notes, verify the objective, constraints, and scope.
2. **Deconstruct & Structure:** Build a MECE (Mutually Exclusive, Collectively Exhaustive) issue tree before looking at numbers.
3. **Hypothesize & Prioritize:** Formulate an early, testable hypothesis about where the problem lies.
4. **Analyze & Isolate:** Request data, perform calculations, decompose variance, and apply root-cause frameworks.
5. **Synthesize & Recommend:** Deliver an action-oriented recommendation with clear logic, quantified impact, risks, and next steps.

---

### 21.2 Clarifying Questions: Good vs. Bad Examples
Clarifying questions must clarify the **scope, objective, and boundaries** of the decision. They should never ask the interviewer to do the thinking for you.

| Dimension | Poor Question (Do Not Ask) | Strong Clarifying Question (Ask This) | Why It Is Strong |
|---|---|---|---|
| **Objective** | *"What should I do first?"* | *"To confirm the core objective: Is our primary goal purely profit recovery within the next fiscal year, or are we prioritizing market share defense and long-term brand equity?"* | Distinguishes between short-term margin optimization and long-term volume strategy. |
| **Scope** | *"Can you tell me about the market?"* | *"Is the 18% decline specific to our product, or is the entire therapeutic class experiencing a contraction? And is this localized to a particular geography or channel?"* | Immediately separates internal performance problems from external market shocks. |
| **Product / Therapy** | *"What does this medicine do?"* | *"Is this drug an acute treatment or a chronic therapy, and is it primarily reimbursed through commercial payers, hospital tenders, or retail cash pay?"* | Directly informs prescription dynamics: chronic drugs rely on refill persistency; hospital drugs rely on formulary inclusion. |
| **Constraints** | *"Do we have enough money?"* | *"Are there any capital expenditure boundaries, regulatory timelines, or existing contractual agreements with distributors that constrain our options?"* | Shows business maturity by respecting operational guardrails. |

---

### 21.3 Defining Objectives and Identifying Constraints
Every case problem must be anchored to explicit parameters:
- **Primary Metric:** Revenue (Gross vs. Net), Prescriptions (TRx vs. NRx), Operating Profit, Market Share (% volume vs. % value), or Patient Conversion.
- **Hurdle Rate / Benchmark:** Not just "increase sales", but "restore revenue to ₹120 Cr within 12 months" or "achieve break-even by Year 2".
- **Internal Constraints:** Sales force headcount cap, compliance rules (e.g., promotional gift bans), production lead times, minimum gross margin requirements (e.g., must stay >65%).
- **External Constraints:** Competitor patent exclusivity, regulatory pricing caps (e.g., DPCO/NPPA in India), payer formulary tier placements and prior-authorization hurdles.

---

### 21.4 Building Issue Trees and MECE Decomposition
MECE stands for **Mutually Exclusive, Collectively Exhaustive**:
- **Mutually Exclusive:** No overlaps between branches (no double counting).
- **Collectively Exhaustive:** No gaps left out (every plausible explanation is captured).

#### Issue Tree Architecture for Revenue Decline:
```
                                        Total Revenue
                                              |
                   -------------------------------------------------------
                   |                                                     |
           Price / Net Realization                              Total Prescription Volume (TRx)
                   |                                                     |
        -----------------------                           -------------------------------
        |                     |                           |                             |
    List Price          Gross-to-Net (GTN)           New Prescriptions (NRx)    Refill Prescriptions (RRx)
                            Deductions                    |                             |
                                  |                 -------------                 -------------
                         - Mandatory Rebates        |           |                 |           |
                         - Commercial Discounts   Naive       Brand             Adherence   Treatment
                         - Wholesaler Fees       Patients   Switching            (PDC)       Duration
```

#### Diagnostic Check: Is Your Tree MECE?
Test your branches against simple algebra:
- Total Revenue = Volume * Net Price
- Volume (TRx) = NRx + RRx
- NRx = Newly Diagnosed Untreated Patients + Competitor Brand Switches
- RRx = Prior Patients * Refill Compliance / Persistency Rate
Because every term algebraically reconstructs the parent metric without overlap or omitted terms, the decomposition is strictly MECE.

---

### 21.5 Hypothesis-Driven Analysis
Never analyze data randomly. Formulate an initial hypothesis based on industry intuition and test it systematically.

**The Hypothesis Template:**
> *"Based on the initial briefing that revenue fell 18% while patient diagnosis rates remained steady, my initial hypothesis is that the decline is driven by either a loss of Tier-2 formulary access leading to retail prescription abandonment, or aggressive pricing discounts from a newly launched biosimilar. To test this, I would first like to inspect our Gross-to-Net discount trend and our TRx share among top-decile prescribing specialists."*

If the data refutes your hypothesis, state it clearly:
> *"The data shows that our Gross-to-Net discount was flat at 28% and our price was unchanged, which refutes the pricing hypothesis. However, TRx dropped 20% primarily in refills (RRx down 34% while NRx was stable). This redirects our focus entirely to patient persistency and treatment drop-off."*

---

### 21.6 Choosing Relevant Commercial KPIs
Select KPIs tailored to the business question:

| Problem Type | Core Primary KPIs | Diagnostic Secondary KPIs |
|---|---|---|
| **Commercial Health** | Net Revenue, TRx, Gross Margin % | GTN %, NRx-to-TRx ratio, Average Selling Price (ASP) |
| **Sales Force Effectiveness** | Calls per Day, Coverage % of Tier-1 Targets | Frequency on Top Deciles, TRx per Call, Share of Voice (SOV) |
| **Market Access / Payers** | % Lives Covered on Formulary Tier 1/2 | Prior Authorization Rejection %, Co-pay step-downs |
| **Patient Brand Journey** | Diagnosis Rate, Treatment Rate | Time-to-Treatment, Refill Compliance (PDC/MPR), Discontinuation % |
| **Marketing Campaign** | Incremental TRx, Return on Investment (ROI) | Cost Per Prescriber Acquired, Digital Click-to-Detail Conversion |

---

## Part B — Quantitative Analysis Toolkit for Analysts

### 21.7 Revenue Decomposition: Price-Volume-Mix Analysis
When revenue shifts between periods, changes occur across three distinct dimensions:
1. **Price Effect:** Change in selling price holding volume constant.
2. **Volume Effect:** Change in total unit volume sold at base-period prices.
3. **Mix Effect:** Shift in product portfolio toward higher- or lower-priced SKUs.

#### Mathematical Formulation:
Let:
- V0, V1 = Total units across all products in base and current periods.
- s_{i,0}, s_{i,1} = Share of product i in total volume.
- P_{i,0}, P_{i,1} = Price of product i in base and current periods.
- P_bar_0 = Weighted average base price = sum(s_{i,0} * P_{i,0}).

The three components:
- Volume Effect = (V1 - V0) * P_bar_0
- Price Effect = sum( V_{i,1} * (P_{i,1} - P_{i,0}) )
- Mix Effect = V1 * sum( (s_{i,1} - s_{i,0}) * (P_{i,0} - P_bar_0) )

Sum of effects: Total Delta Revenue = Volume Effect + Price Effect + Mix Effect.

#### Pharma Worked Example:
A company sells two formulations of an antibiotic:
- **Oral Tablet (Low price):** Base: 60,000 packs @ ₹200. Current: 80,000 packs @ ₹200.
- **IV Injectable (High price):** Base: 40,000 vials @ ₹800. Current: 20,000 vials @ ₹800.

**Base Period:**
Total volume V0 = 100,000 packs. Weighted average price P_bar_0 = (0.60 * 200) + (0.40 * 800) = 120 + 320 = ₹440.
Base Revenue = 100,000 * ₹440 = ₹4.40 Cr.

**Current Period:**
Total volume V1 = 100,000 packs. Tablet share rose from 60% to 80%; IV share fell from 40% to 20%.
Current Revenue = (80,000 * 200) + (20,000 * 800) = 1.60 Cr + 1.60 Cr = ₹3.20 Cr.

**Decomposition:**
- Volume Effect: (100,000 - 100,000) * 440 = ₹0.
- Price Effect: Individual prices did not change = ₹0.
- Mix Effect: 100,000 * [ (0.80 - 0.60)*(200 - 440) + (0.20 - 0.40)*(800 - 440) ]
  = 100,000 * [ (0.20 * -240) + (-0.20 * 360) ]
  = 100,000 * [ -48 - 72 ] = -₹1.20 Cr.

**So what does this mean for the business?**
Total volume remained unchanged at 100,000 units, but revenue fell 27.3% (-₹1.20 Cr). A naive manager might assume sales reps lost motivation. The analyst demonstrates that the entire decline is driven by an adverse mix shift from hospital IVs to retail tablets. The strategic question is: Why did hospitals cut IV usage? (e.g., tender loss, protocol change, or biosimilar substitution).

---

### 21.8 Growth, Market Share, and Share of Voice (SOV)
#### Market Share Math:
- New Market Share = Old Market Share * [ (1 + Brand Growth Rate) / (1 + Market Growth Rate) ]
- If Brand Growth > Market Growth -> Share Expands.
- If Brand Growth = Market Growth -> Share Holds Constant.
- If Brand Growth < Market Growth -> Share Contracts (even if brand sales grew in absolute terms!).

#### Share of Voice (SOV) and Investment Efficiency:
- Share of Voice (SOV) = Brand Promotional Calls or Spend / Total Market Promotional Calls or Spend
- Promotional Efficiency Index (EI) = Market Share (SOM) / Share of Voice (SOV)
- EI > 1.0: Brand is gaining more share per dollar of promotion than competitors (strong brand equity, superior efficacy, or entrenched habit).
- EI < 1.0: Inefficient commercial spend; calls or ads are failing to convert prescribers.

---

### 21.9 Conversion Funnel Mathematics
Commercial life sciences relies on a cascading conversion funnel:
Population -> Prevalent Cases -> Diagnosed -> Treated -> Brand Share -> Adherence / Refill Volume

**Identifying the Bottleneck (The Elasticity of Conversion):**
When analyzing where to allocate promotional capital, determine the stage with the largest drop-off relative to industry benchmarks.
- Stage Conversion Rate_i = (Count at Stage i) / (Count at Stage i-1)
A 5 percentage point improvement at an early, wide stage (e.g., diagnosis rate rising from 20% to 25%) produces far more absolute patients than a 10 percentage point improvement at a downstream stage (e.g., brand trial rising from 80% to 90%).

---

## Part C — Root-Cause Analysis Frameworks

### 21.10 The Five Whys (Pharma Commercial Application)
Never accept the immediate operational complaint as the root cause. Drill down 5 levels:

```
Observation: CardioVax quarterly sales missed target by ₹8.5 Cr.
|
+-- Why 1? Hospital re-orders declined 32% across Western zone institutions.
    |
    +-- Why 2? Chief Pharmacists delayed purchase orders due to excess ward inventory.
        |
        +-- Why 3? Wards reported high patient abandonment after the first dose.
            |
            +-- Why 4? Patients experienced severe gastrointestinal side-effects when
                |      taken on an empty stomach and discontinued medication.
                |
                +-- Why 5 (Root Cause)? Discharge instruction sheets failed to clearly state
                                       "Must take with food", and reps never detailed nurse
                                       educators on co-administration protocols.
```
**Analyst Insight:** The commercial fix is not a 10% price discount or firing the sales reps; it is redesigning the patient packaging insert and funding nurse-led administration education programs.

---

### 21.11 Fishbone (Ishikawa) Framework for Commercial Analytics
When decomposing complex commercial issues, categorize potential drivers across five distinct operational domains:

```
                     Ishikawa Commercial Diagnostic
                     
   PAYER & ACCESS                    SALES FORCE EFFECTIVENESS
   - Tier 3 co-pay barrier            - Call frequency on Deciles 1-3
   - Prior authorization delays      - Rep vacancy / turnover in territory
   - Hospital tender exclusion        - Ineffective core visual aid (CVA)
                 \                      /
                  \                    /
                   ------------------->  PROBLEM: Brand TRx
                  /                    \         Stagnation
                 /                      \
   PRODUCT & COMPETITION             PHYSICIAN & PATIENT
   - Competitor head-to-head trial    - Entrenched prescribing habits
   - Adverse safety signal in media   - High out-of-pocket patient co-pay
   - Shelf-life / stockouts           - Complex dosing schedule
```

---

### 21.12 Correlation vs. Causation & Simpson's Paradox
In healthcare analytics, observational data frequently deceives the unwary analyst:

1. **Reverse Causality Trap:**
   - *Data:* Sales reps who visit physicians 6 times a month see 40 TRx/month, while reps visiting twice see 10 TRx/month.
   - *Erroneous Conclusion:* "Mandate 6 visits for all doctors to quadruple sales!"
   - *Reality:* Reps naturally spend their time visiting high-volume prescribers who already write huge volumes of prescriptions regardless of promotional frequency.

2. **Simpson's Paradox in Field Force Productivity:**
   - Overall company data might show that Rep Team A has higher average sales per rep than Rep Team B.
   - However, when segmented by territory tier (Urban vs. Rural), Team B outperforms Team A in both urban and rural territories.
   - *Why?* Team A has 80% of its reps stationed in high-density urban areas, whereas Team B was assigned to underdeveloped rural zones. Aggregation masked the true performance.

---

## Part D — Developing Executive Recommendations

Every strong recommendation must adhere to the **EAR-KPI** architecture:
1. **Explicit Action:** What must be done, by whom, and within what timeframe.
2. **Anchor Evidence:** The specific calculations and data points proving this is the optimal choice.
3. **Return & Impact:** Quantified revenue/profit uplift (provide base, conservative, and optimistic bounds).
4. **Risks & Mitigation:** What could derail this initiative and how we hedge against it.
5. **Key Performance Indicators:** 2–3 leading and lagging metrics to monitor execution.

### Recommendation Template Table:
```
+----------------------------------------------------------------------------------------+
| STRATEGIC RECOMMENDATION: Restructure East Zone Commercial Model                       |
+-------------------+--------------------------------------------------------------------+
| Action            | Reallocate 12 hospital reps from Tier-3 retail clinics to Top-40   |
|                   | regional oncology centers and fund dedicated reimbursement         |
|                   | navigators.                                                        |
+-------------------+--------------------------------------------------------------------+
| Supporting Data   | Hospital oncology accounts generate 78% of TRx with 3.2x higher   |
|                   | persistency, but currently receive only 24% of promotional calls.  |
+-------------------+--------------------------------------------------------------------+
| Financial Impact  | Year 1 Net Revenue uplift: ₹14.5 Cr (Range: ₹11.2 Cr - ₹18.0 Cr).  |
|                   | Payback period: 4.5 months. Incremental ROI: 240%.                 |
+-------------------+--------------------------------------------------------------------+
| Risk & Mitigation | Risk: Temporary retail prescription drop of ~5%.                   |
|                   | Mitigation: Transition retail clinics to digital remote detailing. |
+-------------------+--------------------------------------------------------------------+
| Governance & KPIs | Leading KPI: Reach on Tier-1 Oncologists (>85% monthly coverage).  |
|                   | Lagging KPI: Hospital Prior Authorization approval rate (>75%).    |
+-------------------+--------------------------------------------------------------------+
```

---

## Part E — Communicating Case Findings Across Time Scales

To test communication agility, interviewers frequently ask you to summarize your findings in 60 seconds, 3 minutes, 5 minutes, or 10 minutes.

### The Uniform Scenario:
*You diagnosed that Brand NeuroCalm dropped 15% in revenue due to pharmacy benefit manager (PBM) formulary exclusion in 2 major health plans, which forced patients to pay a ₹4,500 monthly co-pay instead of ₹500.*

#### 1. The 60-Second Elevator Pitch (C-Suite Synthesis):
> *"NeuroCalm's 15% revenue decline is entirely driven by access friction rather than clinical dissatisfaction. When HealthPlans Alpha and Beta relegated our brand to Tier 3 last October, patient co-pays jumped from ₹500 to ₹4,500, causing a 42% prescription abandonment rate at retail pharmacies. 
> 
> My recommendation is to negotiate a supplemental 6% rebate with these two plans in exchange for Tier 2 restoration, while launching an instant co-pay bridge savings card next month. This will recapture ₹18 Cr in annual net sales at a cost of ₹4.5 Cr in rebates, delivering a net profit gain of ₹13.5 Cr. Success will be measured by pharmacy claim rejection rates falling below 12% within 90 days."*

#### 2. The 3-Minute Summary (Standard Interview Answer):
- **Minute 1 — Situation & Diagnostic:** State the headline metric, dismiss non-drivers (clinical perception was positive; sales calls were up 4%), and highlight the isolated bottleneck (PBM exclusion).
- **Minute 2 — Economics & Variance Breakdown:** Walk through the math: 20,000 affected patients; 42% drop-off = 8,400 lost TRx/month; financial erosion of ₹1.5 Cr/month. Explain why discounting list price across the board would destroy ₹30 Cr in unconstrained segments.
- **Minute 3 — Action Plan & Risk Hedge:** Present the two-pronged solution (targeted rebate negotiation + interim co-pay card), outline the 6-month roadmap, and address the risk of payer cannibalization.

#### 3. Weak vs. Strong Candidate Answer Comparison:

| Attribute | Weak Candidate Answer | Strong Candidate Answer |
|---|---|---|
| **Structure** | *"We looked at a lot of charts. Sales were down in the South, and doctors were unhappy with price. Maybe marketing should run an ad campaign or we can lower the price."* | *"I evaluated three potential drivers: prescriber demand, sales force execution, and payer access. The analysis conclusively eliminates demand and sales execution, isolating 85% of the variance to PBM formulary tiering in two accounts."* |
| **Quantification** | *"A lot of patients stopped buying because it got too expensive."* | *"Co-pay elasticity analysis reveals that abandonment triples from 14% to 42% once out-of-pocket costs exceed the ₹2,000 threshold, costing us 8,400 TRx monthly."* |
| **Business Judgment**| *"We should cut our national price by 50% to make it affordable for everyone."* | *"A blanket price reduction would erode ₹32 Cr across our secure commercial base. Instead, we must deploy a targeted co-pay assistance program capping patient cost at ₹750, preserving our gross-to-net realization."* |

---

## Part F — Masterclass Case Study Exercises (12 Complete Cases)

> **Important Study Note:** All 12 case statements, datasets, questions, and frameworks are provided below. **The detailed step-by-step worked solutions, mathematical proofs, and interviewer evaluation criteria are compiled in the dedicated Answer Key at the end of this module (§21.13).**
> Attempt each case with pen, paper, and calculator for 25–30 minutes before inspecting the solutions.

---

### Case 1: Revenue Decline — Oncology Brand OncoShield
**Case Context:**
MedVantage Pharma manufactures **OncoShield**, an oral targeted therapy for renal cell carcinoma launched in 2021. For three consecutive years, OncoShield grew at 25%+ YoY. However, in FY 2025, net revenue plunged 18.2% from ₹110 Cr to ₹90 Cr. The VP of Commercial Analytics needs an urgent diagnostic of why revenue contracted and what actions must be taken in FY 2026.

#### Dataset 1.1: Financial and Operational Breakdown (FY24 vs. FY25)
| Metric | FY 2024 | FY 2025 |
|---|---|---|
| Gross Invoiced Sales | ₹146.67 Cr | ₹144.00 Cr |
| Gross-to-Net (GTN) Deductions | 25.0% | 37.5% |
| Net Revenue | ₹110.00 Cr | ₹90.00 Cr |
| Total Units Sold (packs) | 22,000 | 24,000 |
| Invoiced List Price per Pack | ₹66,667 | ₹60,000 |
| Effective Net Price per Pack | ₹50,000 | ₹37,500 |

#### Dataset 1.2: Channel Volume and Realization Breakdown
| Channel | FY24 Units | FY24 Net Price | FY25 Units | FY25 Net Price |
|---|---|---|---|---|
| Private Corporate Hospitals | 14,000 | ₹55,000 | 12,000 | ₹52,000 |
| Government Tenders / ESI / CGHS | 5,000 | ₹42,000 | 9,000 | ₹24,000 |
| Retail Specialty Pharmacies | 3,000 | ₹40,000 | 3,000 | ₹39,000 |

#### Case Questions:
1. Decompose the revenue decline: How much of the ₹20 Cr drop is driven by volume change, price change, and channel mix change?
2. What happened to Gross-to-Net deductions, and what operational factor caused this shift?
3. What is the fundamental business dilemma facing MedVantage Pharma, and what is your recommendation?

**Recommended Framework:** Price-Volume-Mix (PVM) Decomposition -> Channel Profitability Audit -> Strategic Tender Policy.

---

### Case 2: Regional Sales Performance Discrepancy — CardioFix
**Case Context:**
CardioFix is an established cardiovascular therapy sold across four zones in India (North, South, East, West). Total national sales met budget exactly at ₹100 Cr, but leadership noted extreme divergence among regions. South and North exceeded targets, while West missed catastrophically. The National Sales Manager claims West failed because of low sales rep headcount and fierce local competition.

#### Dataset 2.1: Regional Performance & Commercial Effort
| Region | Target (₹ Cr) | Actual Sales (₹ Cr) | Rep Headcount | Target Cardiologists | Calls Completed |
|---|---|---|---|---|---|
| North | 28.0 | 32.0 | 25 | 1,200 | 15,000 |
| South | 32.0 | 38.0 | 30 | 1,500 | 18,000 |
| East | 18.0 | 17.5 | 18 | 900 | 10,800 |
| West | 22.0 | 12.5 | 22 | 1,100 | 16,500 |

#### Dataset 2.2: West Zone Hospital Formulary Status
| Territory in West | Cardiologists | Calls / Doctor / Mo | Formulary Listing Status | Local Distributor Margin |
|---|---|---|---|---|
| West Metro 1 (Mumbai) | 500 | 1.8 | Excluded from Top 3 Chains | Standard (8%) |
| West Metro 2 (Pune) | 300 | 1.2 | Included in 80% Hospitals | Standard (8%) |
| West Rest of State | 300 | 0.9 | Included in 90% Nursing Homes | Standard (8%) |

#### Case Questions:
1. Calculate sales rep productivity (Sales per rep) and call activity (Calls per rep) across all 4 regions. Does the data support the manager's claim that West had too few reps?
2. Calculate the detailing frequency (calls per target doctor) in West vs. other regions.
3. Identify the true root cause of West's ₹9.5 Cr target shortfall and outline an operational remedy.

**Recommended Framework:** Sales Force Sizing & Productivity Matrix -> Account-Level Access Audit -> Remediation Plan.

---

### Case 3: Pharmaceutical Market Entry — Biosimilar Immunology
**Case Context:**
Aethelgard Life Sciences is evaluating whether to launch a biosimilar of **Adalimumab** (anti-TNF biologic for rheumatoid arthritis and plaque psoriasis) in India. The originator brand has lost patent exclusivity. Aethelgard can manufacture the drug at ₹7,500 per pre-filled syringe (COGS). Management will only approve entry if the product can achieve annual net profit of at least ₹15 Cr by Year 3.

#### Dataset 3.1: India Target Patient Population & Pricing
| Parameter | Value |
|---|---|
| Total Adult Population Diagnosed with Target Conditions | 1,200,000 |
| % Clinically Eligible for Biologic Therapy | 15% |
| % Actually Able to Afford / Access Biologics | 8% |
| Average Syringes Required per Patient per Year | 18 |
| Originator Net Selling Price per Syringe | ₹24,000 |
| Proposed Biosimilar Net Selling Price (50% discount) | ₹12,000 |
| Fixed Annual Commercial & Medical Affairs Spend | ₹12.0 Cr |

#### Case Questions:
1. Calculate the Total Addressable Market (TAM), Serviceable Available Market (SAM), and Serviceable Obtainable Market (SOM) in both patient counts and market value (₹ Cr) assuming Aethelgard targets 20% volume share of accessible patients by Year 3.
2. Calculate the gross profit per syringe and the total net operating profit in Year 3.
3. Does this opportunity meet the management hurdle rate of ₹15 Cr net profit? What critical risks must be monitored?

**Recommended Framework:** Epidemiological Patient-Flow Build-up -> P&L Pro-Forma Modeling -> Breakeven & Sensitivity Assessment.

---

### Case 4: Market Share Contraction in a Booming Class — PulmoClear
**Case Context:**
**PulmoClear** is an inhaled triple-combination therapy for Chronic Obstructive Pulmonary Disease (COPD). In FY24, the total market for this therapy class was ₹200 Cr, and PulmoClear held a 25.0% market share (₹50 Cr). In FY25, PulmoClear's sales rose to ₹56 Cr. The brand manager declared victory due to 12% revenue growth. However, the Chief Commercial Officer fired the brand manager. Why?

#### Dataset 4.1: Therapy Class Market Dynamics (FY24 vs. FY25)
| Brand | FY24 Revenue (₹ Cr) | FY25 Revenue (₹ Cr) | Primary Customer Segment |
|---|---|---|---|
| PulmoClear (Our Brand) | 50.0 | 56.0 | Maintenance COPD (Elderly) |
| AeroMax (Competitor A) | 70.0 | 105.0 | Newly Diagnosed Smokers |
| Respira (Competitor B) | 50.0 | 75.0 | Severe Asthmatic Overlap |
| Generic / Local Inhalers | 30.0 | 44.0 | Price-Sensitive Retail Cash |
| **Total Market** | **200.0** | **280.0** | |

#### Case Questions:
1. What was the class growth rate between FY24 and FY25?
2. Calculate PulmoClear's market share in FY24 and FY25. How many percentage points of market share were gained or lost?
3. If PulmoClear had simply maintained its baseline market share, what would its FY25 revenue have been? Quantify the "lost opportunity cost".
4. Diagnose why PulmoClear underperformed the broader class.

**Recommended Framework:** Relative Market Share Analysis -> Opportunity Loss Quantification -> Segment Inflow/Outflow Audit.

---

### Case 5: Physician Awareness & Adoption Funnel Bottleneck — NeuroSoothe
**Case Context:**
BioHeal launched **NeuroSoothe**, a first-in-class oral migraine-preventive drug. A survey of 500 targeted high-prescribing neurologists was conducted 9 months post-launch to evaluate commercial traction. Marketing leadership believes the product suffers from an "awareness problem" and wants to spend ₹5 Cr on broad medical journal advertising. You must audit the survey data.

#### Dataset 5.1: 500-Physician Funnel Audit vs. Industry Launch Benchmarks
| Funnel Stage | NeuroSoothe Actual (%) | Industry Launch Benchmark (%) | Definition |
|---|---|---|---|
| 1. Aided Brand Awareness | 88% (440 doctors) | 80% | Doctor recognizes brand name and MOA |
| 2. Perceived Clinical Superiority | 75% of aware (330) | 70% | Believes data shows better efficacy than old drugs |
| 3. Willingness to Trial | 80% of superior (264) | 75% | Expresses intent to prescribe in next 30 days |
| 4. Actual Patient Trial (Ever Rx) | 30% of willing (79) | 65% | Has written at least one prescription |
| 5. Repeat / Regular Prescriber | 35% of triallists (28) | 50% | Prescribes weekly (>5 patients) |

#### Dataset 5.2: Top Stated Barriers Among Physicians Who Did Not Trial (n = 185)
| Stated Barrier | % Citing Barrier |
|---|---|
| "Prior authorization and insurance approval takes >3 weeks" | 62% |
| "Patient co-pay is too high compared to generic triptans" | 54% |
| "Unconvinced regarding cardiovascular safety" | 8% |
| "Sales representative has never visited my clinic" | 6% |
| "Dosage titration instructions are confusing" | 5% |

#### Case Questions:
1. Identify the exact funnel leak. Where is the largest deviation from industry benchmarks?
2. Critique the Marketing Director’s proposal to spend ₹5 Cr on journal advertising to raise awareness.
3. Formulate an evidence-based commercial action plan to unblock the bottleneck.

**Recommended Framework:** Funnel Leakage & Conversion Efficiency -> Root-Cause Survey Triangulation -> Commercial Reallocation.

---

### Case 6: Physician Survey Cross-Tabulation & Statistical Analysis — GlycoCare
**Case Context:**
**GlycoCare** is a novel oral SGLT2 inhibitor for Type-2 Diabetes. The commercial team surveyed 250 endocrinologists to assess prescribing likelihood and understand whether institutional setting (Academic Medical Center vs. Private Clinic) influences brand preference.

#### Dataset 6.1: Survey Cross-Tabulation (Prescribing Likelihood by Setting)
| Prescribing Likelihood Rating | Academic Hospital (n = 100) | Private Practice / Clinic (n = 150) | Total (N = 250) |
|---|---|---|---|
| 5 — Definitely will prescribe | 42 | 30 | 72 |
| 4 — Probably will prescribe | 33 | 45 | 78 |
| 3 — Might or might not prescribe | 15 | 35 | 50 |
| 2 — Probably will not prescribe | 7 | 25 | 32 |
| 1 — Definitely will not prescribe | 3 | 15 | 18 |

#### Case Questions:
1. Calculate the Top-2-Box (T2B) score (ratings 4 and 5 combined) for Academic doctors vs. Private Practice doctors.
2. Calculate the mean Likelihood score for each group and the overall population.
3. Conduct a percentage-point gap analysis. What hypotheses explain the difference between the two segments?
4. How should the sales and medical affairs teams tailor their engagement strategies for each segment?

**Recommended Framework:** Likert Distribution Analysis -> Cross-Tab Comparison -> Segmented Detailing Strategy.

---

### Case 7: Marketing Campaign ROI & Detailing Optimization — DermaCalm
**Case Context:**
DermaCalm (for severe atopic dermatitis) ran a ₹6.0 Cr multi-channel promotional campaign in FY25 combining in-person detailing, webinars, and digital sampling across 4,000 dermatologists. Leadership wants to determine whether the campaign delivered positive financial ROI and how detailing calls should be targeted next year.

#### Dataset 7.1: Physician Decile Detail & Response to Promotion
| Decile Tier | Doctors in Tier | Baseline TRx / Doc / Yr | Incremental TRx Generated | Margin per TRx | Cost of Detailing / Doc |
|---|---|---|---|---|---|
| Tier 1 (Deciles 9–10) | 800 | 120 | +25 TRx | ₹1,500 | ₹25,000 |
| Tier 2 (Deciles 6–8) | 1,200 | 40 | +12 TRx | ₹1,500 | ₹18,000 |
| Tier 3 (Deciles 1–5) | 2,000 | 8 | +2 TRx | ₹1,500 | ₹10,000 |

#### Case Questions:
1. Calculate the gross incremental profit generated by each tier and the total incremental profit across all 4,000 physicians.
2. Calculate the net ROI for each tier: ROI = (Incremental Gross Margin - Spend) / Spend.
3. Which tier destroyed shareholder value? What should be done with field force allocation for next year?

**Recommended Framework:** Decile Responsiveness Modeling -> Tiered P&L Calculation -> SFE Optimization.

---

### Case 8: Customer Segmentation & Call Frequency Rebalancing
**Case Context:**
A commercial team covers 1,000 neurologists for an epilepsy drug. Reps currently visit every doctor once a month (12 calls/year). Total field force capacity is 12,000 calls annually across 12 reps (1,000 calls/rep/year). Sales are flat. The commercial analyst must segment physicians and design a differentiated call plan.

#### Dataset 8.1: Physician Prescribing Volume vs. Brand Loyalty Segmentation
| Segment Name | Characteristics | No. of Doctors | Current TRx / Doc / Mo | Potential TRx / Doc / Mo |
|---|---|---|---|---|
| Segment A: High-Volume Champions | High volume, 80% our brand | 150 | 50 | 55 |
| Segment B: High-Volume Splitters | High volume, 20% our brand | 250 | 12 | 60 |
| Segment C: Moderate-Volume Loyalists | Medium volume, 70% our brand | 300 | 15 | 18 |
| Segment D: Low-Volume Skeptics | Low volume, 10% our brand | 300 | 2 | 5 |

#### Case Questions:
1. Under the current uniform model (12 calls/year to all 1,000 doctors), how many calls are spent on Segment D vs. Segment B?
2. Propose a segmented call plan (assign specific call frequencies to Segments A, B, C, D) that fully uses the 12,000 annual call capacity.
3. Justify the reallocation and project the potential commercial uplift.

**Recommended Framework:** Prescriber Portfolio Matrix (Volume x Share) -> Call Rebalancing -> Opportunity Assessment.

---

### Case 9: Operational Inefficiency & Sales Force Detailing Drift
**Case Context:**
Vanguard Pharma noticed that despite maintaining 50 full-time reps detailing anti-infectives, overall TRx dropped 8% over the past two quarters. The National Sales Director demanded budget for 10 more reps. Analytics undertook a CRM audit of 45,000 detailing interactions.

#### Dataset 9.1: CRM Detailing Audit Data
| Operational Metric | Q1 Last Year | Q1 Current Year | Industry Benchmark |
|---|---|---|---|
| Total Reps | 50 | 50 | 50 |
| Total Recorded Calls | 22,500 | 23,000 | 22,000 |
| Average Call Duration (mins) | 6.2 mins | 2.8 mins | 5.5 mins |
| Primary Detail Position (P1) % | 72% | 38% | 65% |
| Reach on Tier 1 Targets (>10 calls/yr) | 88% | 51% | 85% |
| Frequency on Non-Target Doctors | 12% of calls | 42% of calls | <15% |

#### Case Questions:
1. What does "Primary Detail Position (P1)" mean in pharmaceutical sales, and what happened to it?
2. Critique the request to hire 10 additional reps based on this operational evidence.
3. What behavioral root causes explain why reps shifted from Tier 1 to non-target doctors, and how should incentive compensation be restructured?

**Recommended Framework:** SFE Operational Metric Diagnostic -> Detailing Hygiene Analysis -> Incentive Realignment.

---

### Case 10: Product Launch Performance vs. Analogue Benchmarks — VeloHeal
**Case Context:**
**VeloHeal** (novel injectable for severe asthma) launched 6 months ago. Month 6 net revenue reached ₹2.8 Cr. The commercial VP is anxious because the budget was ₹4.0 Cr. The team needs to know whether VeloHeal is failing or experiencing normal adoption curvature.

#### Dataset 10.1: Cumulative Monthly Net Revenue (₹ Cr) vs. Analogue Launches
| Month Post Launch | VeloHeal (Actual) | Analogue 1: Top-Tier Biologic | Analogue 2: Moderate Launch | Analogue 3: Slow Payer Access |
|---|---|---|---|---|
| Month 1 | 0.20 | 0.50 | 0.30 | 0.15 |
| Month 2 | 0.50 | 1.20 | 0.70 | 0.35 |
| Month 3 | 0.90 | 2.20 | 1.30 | 0.65 |
| Month 4 | 1.40 | 3.50 | 2.00 | 1.10 |
| Month 5 | 2.00 | 5.10 | 2.90 | 1.80 |
| Month 6 | 2.80 | 7.00 | 4.00 | 2.70 |

#### Dataset 10.2: Patient Tracking Metrics at Month 6
| Metric | VeloHeal Value | Analogue 2 Value |
|---|---|---|
| Total Patients Started (NRx cumulative) | 1,400 | 1,500 |
| Average 90-Day Refill Persistency | 42% | 78% |
| Payer Prior-Authorization Rejection Rate | 18% | 20% |
| Physician Stated Satisfaction with Efficacy | 86% | 82% |

#### Case Questions:
1. Which analogue does VeloHeal most closely resemble in revenue trajectory?
2. Analyze the patient tracking metrics: Is VeloHeal's problem generating new patients (NRx) or retaining existing patients (persistency)?
3. What clinical or patient support interventions should be recommended to bridge the revenue gap?

**Recommended Framework:** Analogue Launch Curve Benchmarking -> Patient Persistency / Refill Cascade -> PSP Intervention.

---

### Case 11: Conflicting Data Sources & Channel Inventory Distortions
**Case Context:**
At year-end review, the Finance Director, Supply Chain Head, and Analytics Lead present contradictory data for Cardiovascular drug **CardioPlus**:
- **Finance (Ex-Factory Primary Invoices):** Sales are up +16% YoY (₹116 Cr vs. ₹100 Cr).
- **Market Research (IQVIA / Secondary Retail Audits):** Prescriptions written are down -6% YoY.
- **Field CRM Logs:** Physician calls are up +10% YoY with stable detail feedback.
Leadership is baffled: *"How can factory shipments rise 16% when doctor prescriptions fell 6%?"*

#### Dataset 11.1: Supply Chain Channel Inventory Audit (Months of Stock on Hand)
| Stakeholder / Channel Node | Normal Inventory Level | Actual Year-End Inventory Level |
|---|---|---|
| Primary Clearing & Forwarding Agents (CFAs) | 1.0 month | 2.8 months |
| Regional Stockists / Wholesalers | 0.75 months | 2.2 months |
| Retail / Hospital Pharmacies | 0.50 months | 0.55 months |

#### Case Questions:
1. Explain the mechanism that allows primary factory sales to grow while patient prescriptions decline.
2. What happened in the distribution channel at year-end? Identify the commercial incentive behind this phenomenon.
3. Forecast what will happen to factory sales in Q1 of next year if patient prescription demand does not accelerate.
4. How should management reconcile internal ex-factory data with secondary audit data for executive reporting?

**Recommended Framework:** Multi-Tier Channel Pipeline Dynamics -> Inventory Burn-Down Modeling -> Audit Governance.

---

### Case 12: Comprehensive Commercial Strategy — Rare Disease Biologic
**Case Context:**
TheraRare Therapeutics is preparing the 3-year commercialization plan for **NervoZyme**, an enzyme replacement therapy for a rare metabolic neuropathy in India. The drug has received regulatory approval. Management needs a complete strategy: patient epidemiology, pricing model, field force sizing, P&L forecast, and risk mitigation.

#### Dataset 12.1: Epidemiological & Access Funnel
| Parameter | Value |
|---|---|
| Total Target Population in India | 1.4 Billion |
| Disease Prevalence | 1 in 100,000 |
| Diagnostic Rate (Specialist Genetic Centers) | 25% |
| Accessible / Treatable Patients (Severe Phenotype) | 60% of diagnosed |
| Average Vials Required per Patient per Year | 52 vials (weekly infusion) |
| Target Market Share by Year 3 | 40% of treatable patients |

#### Dataset 12.2: Pricing, Cost, and Commercial Parameters
| Parameter | Value |
|---|---|
| Proposed Ex-Factory Net Price per Vial | ₹45,000 |
| Manufacturing Cost per Vial (COGS) | ₹9,000 |
| Target Specialist Centers (Hospital Accounts) | 60 centers nationwide |
| Planned Dedicated Key Account Managers (KAMs) | 10 reps |
| Fully Loaded Cost per KAM (Salary, Travel, Medical) | ₹30 Lakhs / year |
| Patient Support Program (PSP) & Infusion Logistics | ₹1.5 Cr / year |
| Medical Affairs & Diagnostic Subsidies | ₹2.0 Cr / year |

#### Case Questions:
1. Calculate the total patient cascade: Prevalent Patients -> Diagnosed -> Treatable -> Year 3 NervoZyme Patients.
2. Calculate Year 3 Gross Revenue, Net Revenue, COGS, Operating Expenses (OpEx), and Operating Profit (₹ Cr and Operating Margin %).
3. How many accounts will each Key Account Manager handle? Is the sales force size appropriate?
4. Formulate the commercial launch strategy, identifying the single largest risk and the optimal mitigation structure.

**Recommended Framework:** End-to-End Rare Disease Commercial Blueprint -> Account Coverage Modeling -> Risk-Sharing Agreement.

---

## Part G — Answer Keys, Worked Solutions & Evaluation Rubrics

> **STUDY NOTICE:** Review this section **only after** you have independently solved the cases above.

---

### §21.13.1 Answer Key — Case 1: Revenue Decline (OncoShield)

#### 1. Mathematical Revenue Decomposition (PVM)
Using the formulas from §21.7:
- **Baseline FY24:** V0 = 22,000, Net Revenue = ₹110.0 Cr, P_bar_0 = ₹50,000/pack.
- **Current FY25:** V1 = 24,000, Net Revenue = ₹90.0 Cr, P_bar_1 = ₹37,500/pack.
- Total Revenue Change = ₹90.0 Cr - ₹110.0 Cr = -₹20.0 Cr.

**Step-by-step breakdown:**
1. **Volume Effect:**
   Delta V * P_bar_0 = (24,000 - 22,000) * ₹50,000 = +2,000 * ₹50,000 = +₹10.0 Cr.
   *Interpretation:* Volume alone grew by 9.1%, which added ₹10 Cr of revenue.

2. **Price Effect (within channels):**
   - Private Hospitals: 12,000 * (52,000 - 55,000) = 12,000 * (-3,000) = -₹3.60 Cr.
   - Govt Tenders: 9,000 * (24,000 - 42,000) = 9,000 * (-18,000) = -₹16.20 Cr.
   - Retail: 3,000 * (39,000 - 40,000) = 3,000 * (-1,000) = -₹0.30 Cr.
   - Total Pure Price Effect = -₹3.60 - ₹16.20 - ₹0.30 = -₹20.10 Cr.

3. **Mix Effect:**
   - Base shares: Private = 63.64%, Govt = 22.73%, Retail = 13.64%.
   - FY25 shares: Private = 50.00%, Govt = 37.50%, Retail = 12.50%.
   - Mix Effect = -₹9.90 Cr due to significant volume shift into the low-price Govt tender channel.
   - Reconciled Total = +10.0 - 20.10 - 9.90 = -₹20.0 Cr.

#### 2. Root Cause & GTN Shift:
GTN jumped from 25.0% to 37.5%. The company aggressively pursued government institutional volume (growing Govt packs from 5,000 to 9,000), but won the bid by slashing tender net prices from ₹42,000 down to ₹24,000 (a 43% cut). This massive institutional dilution severely eroded profitability and dragged total net revenue down by ₹20 Cr despite delivering 2,000 more physical medicine packs.

#### 3. Recommendation:
Establish a strict tender floor price of ₹38,000. Do not chase unprofitable public volume at a 60%+ discount to private retail. Reallocate sales representatives back to Private Corporate Hospitals, where net realization is ₹52,000/pack and margins exceed 70%.

---

### §21.13.2 Answer Key — Case 2: Regional Discrepancy (CardioFix)

#### 1. Productivity and Effort Calculations:
| Region | Sales / Rep (₹ Cr) | Target Cardiologists / Rep | Calls / Rep / Year | Calls / Doctor / Month |
|---|---|---|---|---|
| North | ₹1.28 Cr | 48 | 600 | 1.04 |
| South | ₹1.27 Cr | 50 | 600 | 1.00 |
| East | ₹0.97 Cr | 50 | 600 | 1.00 |
| West | **₹0.57 Cr** | 50 | **750** | **1.25** |

*Analysis:* Reps in the West were **more active** (750 calls/rep vs. 600 national average) and detailed doctors more frequently (1.25 calls/mo vs. 1.00). The manager's claim that West had too few reps or low effort is flatly contradicted by data.

#### 2. Root Cause:
Formulary Exclusion in Mumbai (West Metro 1). Mumbai contains 500 cardiologists (45% of the entire zone). Reps in Mumbai made 10,800 calls to doctors who like the drug, but could not write hospital-dispensed prescriptions because the top 3 hospital chains excluded CardioFix from their commercial formulary. Detailing physicians who are physically blocked from prescribing produces zero ROI.

#### 3. Recommendation:
Do not hire more sales reps in the West. Form a dedicated **Market Access & Institutional Formulary Team** to negotiate listing in the top 3 Mumbai hospital chains. Temporarily shift 6 reps from detailing to formulary support. Once listed, Mumbai's ₹9.5 Cr deficit can be recovered within 2 quarters.

---

### §21.13.3 Answer Key — Case 3: Market Entry (Biosimilar Immunology)

#### 1. Patient Cascade & Market Sizing:
1. Prevalent Diagnosed: 1,200,000.
2. Clinically Eligible (15%): 1,200,000 * 0.15 = 180,000 patients.
3. Accessible / Able to Afford (8%): 180,000 * 0.08 = 14,400 patients (**Total Serviceable Pool**).
4. SOM Target Volume (20% share by Year 3):
   Year 3 Patients = 14,400 * 0.20 = 2,880 patients.
5. Syringes Required: 2,880 patients * 18 syringes/yr = 51,840 syringes.
6. Year 3 Net Revenue: 51,840 syringes * ₹12,000 = ₹62.21 Cr.

#### 2. Financial P&L Pro-Forma (Year 3):
- Gross Revenue = ₹62.21 Cr.
- COGS (51,840 * ₹7,500) = ₹38.88 Cr.
- Gross Profit = ₹62.21 - ₹38.88 = ₹23.33 Cr (Gross Margin = 37.5%).
- Fixed Commercial & Medical Spend = ₹12.00 Cr.
- **Operating Profit (EBIT)** = ₹23.33 - ₹12.00 = **₹11.33 Cr**.

#### 3. Decision & Strategic Recommendation:
**Reject or Restructure the Launch as Currently Configured.**
The projected Year 3 operating profit of **₹11.33 Cr falls short** of management’s ₹15.0 Cr hurdle rate.
*Why?* The COGS of ₹7,500 per syringe is excessively high, leaving a slim gross margin of only ₹4,500/syringe (37.5%).
*Actionable Pivot:* Re-negotiate contract manufacturing or optimize fermentation yields to reduce COGS below ₹6,000/syringe, OR partner with an established dermatology sales force to reduce the ₹12 Cr fixed commercial overhead. Without COGS reduction, entering this crowded biosimilar space is financially unviable.

---

### §21.13.4 Answer Key — Case 4: Market Share Contraction (PulmoClear)

#### 1. Market Growth Rate:
Class Growth Rate = (280 - 200) / 200 = 80 / 200 = +40.0%.

#### 2. Market Share Calculations:
- FY24 Market Share: 50 / 200 = 25.0%.
- FY25 Market Share: 56 / 280 = 20.0%.
- Market share **contracted by 5.0 percentage points** (-20% relative loss).

#### 3. Opportunity Cost Calculation:
Had PulmoClear maintained its 25.0% baseline share in the expanded ₹280 Cr market:
Potential Revenue = 25.0% * ₹280 Cr = ₹70.0 Cr.
Lost Opportunity Cost = ₹70.0 Cr - ₹56.0 Cr = ₹14.0 Cr.
The brand manager celebrated +₹6 Cr growth, but in reality, the brand forfeited **₹14.0 Cr in potential revenue** to competitors.

#### 4. Diagnosis:
Competitors AeroMax and Respira capitalized on newly diagnosed patient cohorts (which grew 50%+), while PulmoClear remained narrowly detailed to elderly maintenance patients. PulmoClear missed the major expansion wave of the therapy class.

---

### §21.13.5 Answer Key — Case 5: Physician Funnel Bottleneck (NeuroSoothe)

#### 1. Identifying the Funnel Leak:
Compare stage-to-stage conversion rates against industry benchmarks:
- Awareness: 88% (Exceeds benchmark of 80% — **No awareness problem!**).
- Superiority of Aware: 330 / 440 = 75.0% (Benchmark 70% — strong clinical credibility).
- Willingness to Trial: 264 / 330 = 80.0% (Benchmark 75% — strong physician interest).
- **Trial Conversion:** 79 / 264 = **29.9%** (**Benchmark is 65.0%! Catastrophic 35.1 pp collapse**).

#### 2. Critique of Advertising Spend:
Spending ₹5 Cr on journal advertising to raise awareness is an egregious waste of capital. Aided awareness is already 88%. 264 neurologists actively want to prescribe the drug today, but only 79 succeeded in writing an Rx.

#### 3. Root Cause & Action Plan:
Survey Dataset 5.2 reveals that **62% of non-triallists cite 3-week prior authorization delays** and 54% cite patient co-pay friction.
*Recommendation:* Cancel the ₹5 Cr advertising campaign. Reallocate ₹3 Cr to:
1. A **Patient Prior-Authorization Hub** (dedicated clinical navigators who process paperwork for clinics within 24 hours).
2. An **Instant Digital Co-pay Bridge Voucher** covering the patient's out-of-pocket expense for the first 30 days while insurance processes.
This directly attacks the 29.9% conversion bottleneck and unblocks the 185 willing prescribers.

---

### §21.13.6 Answer Key — Case 6: Survey Cross-Tabulation (GlycoCare)

#### 1. Top-2-Box (T2B) Likelihood:
- Academic Hospital: (42 + 33) / 100 = 75 / 100 = **75.0%**.
- Private Practice: (30 + 45) / 150 = 75 / 150 = **50.0%**.
- Total Population: (72 + 78) / 250 = 150 / 250 = **60.0%**.

#### 2. Mean Likelihood Scores:
Mean = sum(Rating * Count) / n.
- Academic: [ (5*42) + (4*33) + (3*15) + (2*7) + (1*3) ] / 100 = [ 210 + 132 + 45 + 14 + 3 ] / 100 = 404 / 100 = **4.04** / 5.0.
- Private Practice: [ (5*30) + (4*45) + (3*35) + (2*25) + (1*15) ] / 150 = [ 150 + 180 + 105 + 50 + 15 ] / 150 = 500 / 150 = **3.33** / 5.0.
- Overall Sample Mean: (404 + 500) / 250 = 904 / 250 = **3.62** / 5.0.

#### 3. Strategic Interpretation:
There is a massive **25 percentage point gap** in T2B intent between Academic and Private doctors. Academic endocrinologists focus heavily on cardiovascular-renal clinical outcome trials (hard endpoint data), where GlycoCare excels. Private clinic doctors are far more sensitive to price, ease of patient self-titration, and commercial insurance coverage.

#### 4. Tailored Commercial Strategy:
- **Academic Segment:** Deploy Medical Science Liaisons (MSLs) with head-to-head cardiovascular trial data and symposium sponsorships.
- **Private Segment:** Pivot detailing away from clinical trial graphs to co-pay cards, patient affordability kits, and simple once-daily dosing convenience.

---

### §21.13.7 Answer Key — Case 7: Marketing ROI (DermaCalm)

#### 1. Tier Profitability Calculations:
| Tier | Doctors | Incr. TRx / Doc | Total Incr. TRx | Gross Margin (₹1,500/TRx) | Detailing Spend | Net Margin Contribution |
|---|---|---|---|---|---|---|
| Tier 1 | 800 | +25 | 20,000 | ₹3.00 Cr | ₹2.00 Cr | +₹1.00 Cr |
| Tier 2 | 1,200 | +12 | 14,400 | ₹2.16 Cr | ₹2.16 Cr | ₹0.00 Cr |
| Tier 3 | 2,000 | +2 | 4,000 | ₹0.60 Cr | ₹2.00 Cr | **-₹1.40 Cr** |
| **Total** | **4,000** | — | **38,400** | **₹5.76 Cr** | **₹6.16 Cr** | **-₹0.40 Cr** |

#### 2. Net ROI by Tier:
- Tier 1 ROI: (3.00 - 2.00) / 2.00 = +1.00 / 2.00 = **+50.0%**.
- Tier 2 ROI: (2.16 - 2.16) / 2.16 = **0.0%** (Break-even).
- Tier 3 ROI: (0.60 - 2.00) / 2.00 = -1.40 / 2.00 = **-70.0%**.
- Overall Campaign ROI: (5.76 - 6.16) / 6.16 = **-6.5%**.

#### 3. Business Implication & Action:
The campaign **destroyed ₹40 Lakhs of shareholder capital** overall because **₹2.0 Cr was squandered on Tier 3 physicians**, where incremental response (+2 TRx) generated only ₹60 Lakhs against ₹2 Cr in detailing expense.
*Action:* Immediately eliminate in-person detailing for Tier 3. Transition Tier 3 to low-cost digital detailing/emails (cost: ₹500/doc/year instead of ₹10,000). Reallocate ₹1.0 Cr to Tier 1 to increase detailing frequency, generating an estimated additional ₹1.5 Cr profit.

---

### §21.13.8 Answer Key — Case 8: Customer Segmentation & Call Rebalancing

#### 1. Evaluation of Current Uniform Model:
- Total capacity = 12,000 calls.
- Segment D receives 300 * 12 = 3,600 calls (30% of total commercial effort!).
- Segment B receives 250 * 12 = 3,000 calls.
Currently, 3,600 calls are wasted on low-volume skeptics who produce a negligible 2 TRx/month.

#### 2. Proposed Optimized Call Allocation:
| Segment | Doctors | Proposed Calls / Year | Total Calls | Strategy / Justification |
|---|---|---|---|---|
| Segment B (High Volume Splitters) | 250 | **20** (every 2.5 wks) | 5,000 | Prime conversion target: massive volume, headroom to grow share from 20% to 50%+. |
| Segment A (High Volume Champions) | 150 | **16** (every 3 wks) | 2,400 | Defensive retention: protect our 80% share against competitor poaching. |
| Segment C (Moderate Loyalists) | 300 | **12** (monthly) | 3,600 | Maintain steady flow; solid baseline margin. |
| Segment D (Low Volume Skeptics) | 300 | **3.3** (quarterly digital) | 1,000 | Deprioritize; convert to digital webinars and sample mailings. |
| **Total** | **1,000** | — | **12,000** | Perfectly balances the 12,000 call capacity constraint. |

#### 3. Expected Commercial Impact:
If high-frequency detailing converts Segment B doctors from 20% to just 35% brand share (an incremental +9 TRx/doc/mo across 250 doctors):
Incremental TRx = 250 * 9 * 12 = 27,000 TRx annually.
At ₹500 contribution margin per TRx, this unlocks **₹1.35 Cr in incremental margin** with zero additional sales headcount.

---

### §21.13.9 Answer Key — Case 9: Detailing Inefficiency (Vanguard)

#### 1. What is Primary Detail Position (P1)?
In multi-product detailing, reps present products in sequence. **P1 (First Position)** receives the doctor's full attention in the first 2 minutes. Products relegated to P2 or P3 receive hasty 30-second reminders as the doctor heads to the next patient.
*What happened:* P1 detailing for this brand collapsed from **72% down to 38%**. Reps pushed the anti-infective to secondary and tertiary positions to promote a newly launched respiratory drug with higher bonus commissions.

#### 2. Critique of Headcount Expansion Request:
Hiring 10 more reps would add ₹1.5 Cr in annual fixed costs while treating the symptom rather than the disease. Total calls are already at benchmark (23,000 calls). The breakdown is operational:
1. Average call duration crashed from 6.2 mins to 2.8 mins (rushed, superficial calls).
2. Reps shifted calls to non-target doctors (rose from 12% to 42%) because low-volume doctors are easier to meet without appointments.
3. Reach on Tier 1 prescribers plummeted from 88% down to 51%.

#### 3. Remediation & Incentive Compensation Plan:
Do not hire new reps. Restructure the field force incentive program:
- 40% of quarterly bonus must be tied to **Tier-1 Target Prescriber Coverage (>85%)**.
- Detail tracking software must enforce mandatory P1 position in target oncology/infectious clinics.
- Strip call credit for visits lasting under 4 minutes.

---

### §21.13.10 Answer Key — Case 10: Launch Curve Performance (VeloHeal)

#### 1. Analogue Launch Comparison:
Comparing Month 6 revenues:
- Analogue 1: ₹7.0 Cr (Top-tier blockbuster).
- Analogue 2: ₹4.0 Cr (Budget target).
- Analogue 3: ₹2.7 Cr (Slow payer access).
- **VeloHeal Actual:** ₹2.8 Cr.
VeloHeal’s trajectory tracks **Analogue 3** almost identically, missing the targeted Analogue 2 curve.

#### 2. Diagnostic: NRx vs. Persistency:
- Cumulative NRx: VeloHeal has enrolled **1,400 patients**, virtually identical to Analogue 2 (1,500 patients). New patient trial and physician willingness to start therapy are exceptional!
- Payer Prior-Authorization Rejection: 18% (in line with 20% benchmark).
- **90-Day Refill Persistency:** VeloHeal is at **42% vs. 78% for Analogue 2**.
*Insight:* VeloHeal is filling a "leaky bucket". Patients start therapy, but 58% discontinue within 90 days.

#### 3. Actionable Recommendation:
Clinical satisfaction with efficacy is 86%, indicating the drug works. Patient interviews indicate self-injection site reactions and fear of reconstitution needles.
*Solution:* Launch a **Patient Support Program (PSP)** offering:
1. Free at-home injection training by certified clinical nurses for the first 2 doses.
2. Automated SMS refill adherence reminders at Day 25, 55, and 85.
Improving persistency from 42% to 65% will automatically generate ₹1.2 Cr in incremental recurring refill revenue without spending a rupee to acquire new patient trials.

---

### §21.13.11 Answer Key — Case 11: Conflicting Data Sources (CardioPlus)

#### 1. The Discrepancy Mechanism:
Primary factory shipments reflect **sales from the manufacturer to wholesale distributors/stockists**. Secondary IQVIA audits reflect **actual retail sales to patients based on doctor prescriptions**.
When factory sales rise (+16%) while patient prescriptions fall (-6%), the excess volume is **accumulating as unsold pipeline inventory** in distributor warehouses.

#### 2. Root Cause: "Channel Stuffing":
Dataset 11.1 confirms that distributor inventory surged from a normal 0.75 months up to **2.2 months**, and CFA warehouses surged to **2.8 months**.
*The Commercial Motive:* Ahead of Q4 financial closes, sales leadership offered massive 8% bulk trade discounts to stockists to encourage huge forward buying so the company could hit its annual ex-factory revenue targets and trigger executive bonuses.

#### 3. Q1 Forecast:
In Q1, stockists will sit on 2.2 months of excess inventory. Because patient demand is negative (-6%), distributors will freeze all new factory orders until warehouse inventory burns down to normal levels. **Ex-factory sales in Q1 will collapse by 30% to 50%**, creating an artificial corporate cash flow crisis.

#### 4. Governance & Executive Reporting:
Management must immediately mandate **Demand-True Secondary Reporting**. Executive performance bonuses must be pegged to **IQVIA/Secondary Rx Audits**, not primary factory shipments. Establish strict distributor inventory caps (maximum 1.0 month of stock allowed).

---

### §21.13.12 Answer Key — Case 12: Comprehensive Commercial Strategy (NervoZyme)

#### 1. Epidemiological Patient Build-up:
1. Total Population: 1,400,000,000.
2. Prevalent Cases (1 in 100,000): 1,400,000,000 / 100,000 = 14,000 patients.
3. Diagnosed (25%): 14,000 * 0.25 = 3,500 patients.
4. Treatable / Accessible Severe Cohort (60%): 3,500 * 0.60 = 2,100 patients.
5. **Year 3 Target Patients (40% Share):** 2,100 * 0.40 = **840 patients**.

#### 2. Year 3 Pro-Forma Financial Model:
- Annual Vials per Patient: 52 weekly infusions.
- Total Vials Required: 840 patients * 52 vials = 43,680 vials.
- **Gross Revenue:** 43,680 * ₹45,000 = ₹196.56 Cr.
- **Net Revenue (assuming 10% institutional discount):** 196.56 * 0.90 = **₹176.90 Cr**.
- **COGS (43,680 * ₹9,000):** ₹39.31 Cr.
- **Gross Profit:** ₹176.90 - ₹39.31 = ₹137.59 Cr (Gross Margin = 77.8%).

**Operating Expenses (OpEx):**
- 10 Key Account Managers @ ₹30 Lakhs: ₹3.00 Cr.
- Patient Support Program (PSP) & Cold-Chain Delivery: ₹1.50 Cr.
- Medical Affairs & Genetic Testing Subsidies: ₹2.00 Cr.
- Administrative & Regulatory Compliance: ₹1.50 Cr.
- **Total OpEx:** ₹3.00 + ₹1.50 + ₹2.00 + ₹1.50 = ₹8.00 Cr.

**Operating Profitability:**
- **Operating Profit (EBIT):** ₹137.59 - ₹8.00 = **₹129.59 Cr**.
- **Operating Margin:** 129.59 / 176.90 = **73.3%**.

#### 3. Sales Force Workload Analysis:
60 specialist hospital centers nationwide divided by 10 KAMs = **6 hospital accounts per KAM**.
In rare diseases, this workload is ideal. Unlike primary care reps making 8 rapid calls a day, rare disease KAMs must spend 1–2 full days per week inside each genetic center coordinating multi-disciplinary teams (geneticists, neurologists, hospital pharmacists, and government reimbursement boards).

#### 4. Commercial Strategy & Key Risk:
- **Core Strategy:** Build a **Hub-and-Spoke Rare Disease Center Network**. Fund regional genetic diagnostic testing (subsidizing the diagnostic expands the 25% diagnosis funnel).
- **The #1 Risk:** Patient affordability. At ₹23.4 Lakhs annual cost per patient (52 * ₹45,000), virtually no Indian family can pay out-of-pocket long term.
- **Mitigation:** Partner with State Rare Disease Committees and Corporate CSR Funds under the National Policy for Rare Diseases. Structure **Outcomes-Based Annuity Contracts** where government schemes reimburse upon demonstrated neurological functional improvement.

---

## 21.14 Interviewer Evaluation Rubric for Case Studies

When evaluating candidates on these 12 cases, interviewers score across five criteria (Total: 25 Points):

| Criterion | Poor (1–2 Pts) | Competent (3 Pts) | Client-Ready (4–5 Pts) |
|---|---|---|---|
| **1. Problem Structuring** | Dives into numbers immediately without clarifying questions or a framework. | Uses a basic framework (e.g. 4Ps), but applies it mechanically. | Constructs a custom, strictly MECE issue tree tailored to the specific business model. |
| **2. Quantitative Rigor** | Struggles with percentage changes; drops units; makes basic arithmetic errors. | Calculates numbers correctly with guidance; forgets to reconcile parts to whole. | Flawless mental math; decomposes variances (PVM); checks sanity of outputs independently. |
| **3. Root-Cause Logic** | Stops at the visible symptom (e.g., "sales are down"); confuses correlation with causation. | Identifies direct driver, but misses secondary systemic factors. | Uses Five-Whys to uncover institutional/policy root causes; explains "so what". |
| **4. Commercial Judgment** | Proposes unrealistic blanket solutions (e.g. "cut price 50%"); ignores operational limits. | Recommends logical steps, but fails to prioritize or estimate financial impact. | Formulates phased, evidence-based recommendations with risk hedges and governance KPIs. |
| **5. Synthesis & Communication** | Rambles; requires constant redirection; presents findings as a list of raw tables. | Clearly summarizes answers, but lacks executive polish. | Delivers a crisp, top-down Pyramid Principle synthesis (Action -> Evidence -> Impact). |

---

## 21.15 Masterclass Summary Checklist

Before moving to Module 22, verify your readiness:
- [ ] I can write a MECE revenue decomposition tree without hesitation.
- [ ] I can calculate Price-Volume-Mix effects and explain the business meaning of mix shift.
- [ ] I never confuse market share growth with absolute sales volume growth.
- [ ] I can analyze a physician adoption funnel and pinpoint the exact conversion bottleneck.
- [ ] I know how to calculate Top-2-Box (T2B) scores and mean Likert ratings from cross-tabs.
- [ ] I can evaluate marketing campaign ROI by physician deciles and eliminate value-destroying spend.
- [ ] I can diagnose channel stuffing by comparing primary ex-factory vs. secondary audit trends.
- [ ] I can structure an executive recommendation in 60 seconds using the EAR-KPI format.

**Next Module:** [22-advanced-excel-for-analyst-assessments.md](22-advanced-excel-for-analyst-assessments.md)
