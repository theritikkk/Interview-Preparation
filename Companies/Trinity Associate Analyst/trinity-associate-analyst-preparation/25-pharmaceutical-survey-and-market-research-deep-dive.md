# 25 — Pharmaceutical Survey and Market Research Deep Dive

**Purpose:** equip you with the end-to-end commercial market research, survey methodology, and physician analytics toolkit utilized by TGaS Advisors and Trinity Life Sciences advisory teams. The Associate Analyst JD explicitly requires: *"secondary research on the pharmaceutical commercial landscape, design survey questions, conduct and analyze surveys, work with survey datasets using Excel and analytical tools, identify trends and recommend solutions, create client-facing reports"* (CONFIRMED).
**Prerequisites:** [06-statistics-and-survey-analytics.md](06-statistics-and-survey-analytics.md), [07-business-analytics-and-market-research.md](07-business-analytics-and-market-research.md), [08-life-sciences-industry-fundamentals.md](08-life-sciences-industry-fundamentals.md).
**Suggested study time:** 12–15 hours across 4 focused study blocks.
**Provenance:** All survey methodologies, questionnaires, physician panels, and cross-tab tables are **PRACTICE INFERENCE** built on standard pharmaceutical market research practices. Specific brand, disease, and study figures are fictional for practice.

---

## Part A — The Commercial Healthcare Research Landscape

### 25.1 Pharmaceutical Market Research Overview
Pharmaceutical market research (PMR) differs fundamentally from consumer goods (FMCG) or technology research because the **decision-maker (the physician) is separated from the consumer (the patient) and the payer (the insurer / government).**

```
                     The Commercial Healthcare Triad
                     
                              [ PHYSICIAN ]
                        Prescribes therapy; driven
                        by clinical efficacy, safety,
                        guidelines & habit
                               /       \
                              /         \
                             /           \
                            /             \
                  [ PATIENT ] ----------- [ PAYER / PBM ]
            Consumes therapy; bears      Reimburses cost; restricts
            symptoms & out-of-pocket     formulary via prior-auth,
            co-pay; drives adherence     step therapy & tiers
```

In management consulting and advisory engagements (such as Trinity Life Sciences and TGaS), commercial research answers five high-stakes business questions:
1. **Landscape & Opportunity Assessment:** What unmet clinical needs exist in a disease area, and what is the commercially treatable patient pool?
2. **Awareness, Trial, and Usage (AT&U):** How are in-line brands performing across the prescriber adoption funnel, and where is volume leaking?
3. **Target Product Profile (TPP) Testing & Demand Forecasting:** How will physicians react to a pipeline drug's clinical trial results, and what market share will it capture?
4. **Message Testing & Sales Force Detailing:** Are sales reps delivering messages that actually change prescribing behavior, or are they detailing irrelevant attributes?
5. **Commercial Benchmarking (TGaS core specialty):** How does our client's commercial spending, field force sizing, digital marketing mix, and vendor allocation compare against industry peer benchmarks?

---

### 25.2 Quantitative vs. Qualitative Methodologies in Life Sciences

| Dimension | Qualitative Research (Qual) | Quantitative Research (Quant) |
|---|---|---|
| **Primary Methodologies** | 60-minute In-Depth Interviews (IDIs), Advisory Boards (AdBoards), Focus Groups. | Online web surveys (CAWI), Patient Chart Audits (PRAs), Conjoint / Discrete Choice Experiments (DCE). |
| **Typical Sample Size ($n$)** | $n = 15$ to $40$ Key Opinion Leaders (KOLs) / Specialists. | $n = 150$ to $500+$ practicing physicians; $n = 500+$ patients. |
| **Primary Purpose** | **Hypothesis Generation & Deep Dive:** Uncover emotional drivers, understand complex patient journeys, and explore why doctors hesitate. | **Hypothesis Validation & Measurement:** Quantify market share, measure brand awareness, evaluate attribute importance, and size market opportunity. |
| **Typical Output** | Verbatim quotes, patient journey flowcharts, perceptual frameworks. | Statistical cross-tabulation banner tables, mean scores, Top-2-Box percentages, regression models. |
| **Analyst Role** | Synthesize interview transcripts; map patient pathways; pull illustrative verbatims. | Audit survey datasets; write cleaning code; calculate post-stratification weights; run significance tests; build executive decks. |

---

### 25.3 Healthcare Professional (HCP) Segmentation Frameworks

Physicians are not a homogeneous group. Treating every doctor the same leads to massive commercial inefficiency. In pharma analytics, we segment HCPs across four complementary dimensions:

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│ HCP SEGMENTATION TAXONOMY                                                       │
├───────────────────┬─────────────────────────────────────────────────────────────┤
│ 1. Volume Deciles │ Categorized by total category prescription volume (TRx).    │
│    (Behavioral)   │ Deciles 9–10 represent the top 20% of doctors who write 70%+│
│                   │ of all prescriptions. Prime targets for field force calls.  │
├───────────────────┼─────────────────────────────────────────────────────────────┤
│ 2. Practice Type  │ Academic Medical Centers (AMCs) vs. Private Group Clinics   │
│    (Structural)   │ vs. Solo Practitioners vs. Integrated Delivery Networks.    │
│                   │ AMCs drive guidelines; clinics drive commercial volume.     │
├───────────────────┼─────────────────────────────────────────────────────────────┤
│ 3. Adoption Style │ Innovators / Early Adopters (adopt new drugs at launch) vs. │
│    (Attitudinal)  │ Pragmatists (wait for peer consensus) vs. Conservatives     │
│                   │ (stick to generic stalwarts until forced to switch).        │
├───────────────────┼─────────────────────────────────────────────────────────────┤
│ 4. Clinical Focus │ Sub-specialists (e.g. Heart Failure Cardiologists) vs.      │
│    (Phenotype)    │ General Cardiologists vs. Primary Care Physicians (PCPs).   │
└───────────────────┴─────────────────────────────────────────────────────────────┘
```

#### The 2×2 Prescriber Portfolio Matrix:
```
               High │  QUADRANT 2:                │  QUADRANT 1:
                    │  HIGH-VOLUME SPLITTERS      │  CORE CHAMPIONS
                    │  (Huge patient volume;      │  (Huge patient volume;
                    │   low brand loyalty/share)  │   high brand loyalty/share)
                    │  -> PRIMARY CONVERSION GOAL │  -> DEFEND & NURTURE
     Physician      ├─────────────────────────────┼─────────────────────────────
     Category Volume│  QUADRANT 3:                │  QUADRANT 4:
                    │  LOW-VOLUME SKEPTICS        │  NICHE LOYALISTS
                    │  (Low volume; low share;    │  (Low volume; high share;
                    │   price/access sensitive)   │   entrenched loyalists)
               Low  │  -> DIGITAL / LOW DETAIL    │  -> MAINTAIN VIA SAMPLES
                    └─────────────────────────────┴─────────────────────────────
                         Low (<30%)                    High (>70%)
                                 Brand Share of Prescriptions
```
**Analyst Strategic Insight:**
Field sales reps love visiting Quadrant 4 doctors because they are friendly and prescribe our brand. However, commercial growth comes almost entirely from **Quadrant 2 (High-Volume Splitters)**. Moving a Quadrant 2 doctor from 20% to 40% share delivers 10x more volume than converting three Quadrant 3 doctors.

---

## Part B — Survey Questionnaire Design & Bias Elimination

### 25.4 The 7-Stage Questionnaire Architecture

A professionally fielded commercial healthcare survey adheres to strict structural modularity:

```
┌──────────────────────────────────────────────────────────────────────────┐
│ STAGE 1: Screener & Qualification Filters (3–5 Questions)                │
│ Disqualify off-target specialties, low patient volumes, or conflicts.    │
├──────────────────────────────────────────────────────────────────────────┤
│ STAGE 2: Quota Stratification & Demographics (2–3 Questions)             │
│ Enforce sample balance across region, practice setting, and volume.      │
├──────────────────────────────────────────────────────────────────────────┤
│ STAGE 3: Landscape & Unaided / Aided Brand Awareness (4–6 Questions)     │
│ Measure top-of-mind recall before any product names are mentioned.       │
├──────────────────────────────────────────────────────────────────────────┤
│ STAGE 4: Current Prescribing Practice & Treatment Algorithm (6–8 Qs)     │
│ Baseline patient volume, line-of-therapy breakdown (1L, 2L, 3L).         │
├──────────────────────────────────────────────────────────────────────────┤
│ STAGE 5: Attribute Battery & Driver/Barrier Ratings (Grid Questions)     │
│ 5-point or 7-point Likert scales evaluating efficacy, safety, and access.│
├──────────────────────────────────────────────────────────────────────────┤
│ STAGE 6: Concept Testing / Target Product Profile (TPP) (3–5 Questions) │
│ Blinded or branded clinical scenario evaluation; future prescribing.     │
├──────────────────────────────────────────────────────────────────────────┤
│ STAGE 7: Honorarium & Verification (2 Questions)                         │
│ Medical license ID verification and honorarium disbursement.             │
└──────────────────────────────────────────────────────────────────────────┘
```

---

### 25.5 Designing Strict Screeners
Physician online surveys pay substantial honoraria ($150 to $350 in the US, ₹3,000 to ₹8,000 in India). This attracts fraudulent respondents and unqualified generalists. A robust screener acts as a security filter.

#### Full Production Screener Example:
```
[SCREENER BLOCK: Oncology Renal Cell Carcinoma Study]

S1. What is your primary area of medical practice?
    [ ] Medical Oncology ...................................... [CONTINUE]
    [ ] Hematology-Oncology ................................... [CONTINUE]
    [ ] Surgical Oncology / Urology ........................... [TERMINATE]
    [ ] Radiation Oncology .................................... [TERMINATE]
    [ ] Internal Medicine / General Practice .................. [TERMINATE]

S2. Approximately how many years have you been in active clinical practice 
    post-fellowship / post-residency?
    [ ] Less than 2 years ..................................... [TERMINATE - Too junior]
    [ ] 2 to 30 years ......................................... [CONTINUE]
    [ ] More than 30 years .................................... [CHECK QUOTA - Max 15%]

S3. In a typical month, approximately how many total adult patients with 
    Advanced / Metastatic Renal Cell Carcinoma (mRCC) do you personally manage?
    [ ] Fewer than 5 patients ................................. [TERMINATE - Insufficient volume]
    [ ] 5 to 14 patients ...................................... [CONTINUE - Low Volume Quota]
    [ ] 15 to 29 patients ..................................... [CONTINUE - Mid Volume Quota]
    [ ] 30 or more patients ................................... [CONTINUE - High Volume Decile]

S4. What percentage of your working time is dedicated to direct patient care?
    [ ] Less than 50% ......................................... [TERMINATE - Pure academic/admin]
    [ ] 50% or more ........................................... [CONTINUE]

S5. Are you, or is any immediate family member, employed by a pharmaceutical 
    company, biotechnology manufacturer, market research firm, or advertising agency?
    [ ] Yes ................................................... [TERMINATE - Conflict of Interest]
    [ ] No .................................................... [CONTINUE]
```

---

### 25.6 Eliminating Survey Bias: 10 Flawed vs. Fixed Examples

Writing unbiased survey questions is an essential consulting skill. Biased questions lead to inflated results and flawed client investments.

| # | Bias Pattern | Flawed Question (Biased) | Correct Question (Objective) | Mechanism of Distortion |
|---|---|---|---|---|
| 1 | **Leading / Assumptive** | *"Given OncoShield's breakthrough 4.5-month overall survival advantage, how likely are you to prescribe it?"* | *"Considering all clinical and safety attributes of OncoShield, how likely are you to prescribe it for 1L mRCC?"* | Flawed version anchors the doctor on positive survival, priming them to ignore toxicities. |
| 2 | **Double-Barreled** | *"How satisfied are you with the efficacy and tolerability profile of CardioFix?"* | Split into two distinct items: (a) Satisfaction with clinical efficacy; (b) Satisfaction with tolerability/safety. | Doctors often love efficacy but hate side effects. Grouping them forces an uninterpretable compromise score. |
| 3 | **Unbalanced Scale** | *Options: [ ] Excellent, [ ] Very Good, [ ] Good, [ ] Fair* | *Options: [ ] Very satisfied, [ ] Satisfied, [ ] Neutral, [ ] Dissatisfied, [ ] Very dissatisfied* | Flawed scale provides 3 positive options and only 1 negative/neutral option, artificially forcing positive skew. |
| 4 | **Acquiescence Bias** | *"Do you agree that biologic therapy should be initiated earlier in moderate asthma?"* | *"In patients with moderate asthma, at what disease severity do you typically initiate biologic therapy?"* | Respondents naturally tend to agree with declarative statements presented by authority. |
| 5 | **Social Desirability** | *"Do you always read the full clinical trial publication before prescribing a new medicine?"* | *"When evaluating a newly approved drug, which information sources do you typically consult first?"* | Doctors will never admit to skipping trial publications; they will falsely claim compliance. |
| 6 | **Ambiguous Anchor** | *"How often do you switch patients due to high co-pay: Frequently, Sometimes, Rarely, Never?"* | *"In the past 6 months, approximately what percentage of your patients were switched due to co-pay: [ ] 0%, [ ] 1-10%, [ ] 11-25%, [ ] >25%?"* | "Frequently" means twice a day to a city clinic, but twice a year to a rural doctor. |
| 7 | **Recall Degradation** | *"Exactly how many vials of Biologic X did you prescribe in calendar year 2024?"* | *"In a typical month over the past quarter, approximately how many patients did you initiate on Biologic X?"* | Human memory cannot reconstruct exact annual prescription tallies without chart audits. |
| 8 | **Order Effect Bias** | *Always listing Client Brand A as the first option in a 10-brand dropdown.* | *Randomize or rotate the display order of all listed competitor brands across respondents.* | The first option in a list receives 10–15% higher selection rates (primacy effect). |
| 9 | **Presumptive Share** | *"Why do you prefer Competitor Drug Y over our brand?"* | *"Which therapy do you consider first-line for this patient type, and what key clinical factors drive that choice?"* | Presumes the doctor prefers Competitor Y before verifying their true prescribing share. |
| 10| **Hypothetical Pricing**| *"Would you prescribe this drug if it were reasonably priced?"* | *"At a net patient out-of-pocket co-pay of ₹1,500/month, what percentage of your eligible patients would receive this drug?"* | "Reasonable" is undefined; doctors answer yes, but abandon the drug when real pricing is announced. |

---

## Part C — Sampling, Weighting & Statistical Analysis

### 25.7 Sampling Strategy: Probability vs. Commercial Panels
In consumer statistics, random digit dialing or census sampling is common. In pharmaceutical research, **physician research is conducted via proprietary opt-in panels** (e.g., Sermo, M3 Global Research, ZoomRx, Medscape).

#### The Challenge of Opt-in Healthcare Panels:
Opt-in panels suffer from inherent selection biases:
1. **Academic Over-Representation:** Younger, digitally active hospital fellows complete surveys rapidly.
2. **Community Under-Representation:** High-volume solo practitioners managing 50 patients a day rarely have 20 minutes to complete web surveys.
3. **Response Rate Realities:** Commercial physician email surveys have low response rates (often 3% to 8%).
Therefore, an unweighted sample **never mirrors the true prescribing population**.

---

### 25.8 Sample Size Determination & Finite Population Correction
Because specialist universes are small (e.g., only 1,200 practicing pediatric oncologists in a country), we must apply the **Finite Population Correction (FPC)**:

#### Standard Sample Size Formula:
$$n_0 = \frac{z^2 \cdot p(1 - p)}{e^2}$$

#### Adjusted Sample Size for Finite Population:
$$n = \frac{n_0}{1 + \frac{n_0 - 1}{N}}$$

Where:
- $z = 1.96$ (for 95% confidence).
- $p = 0.50$ (maximum variability assumption).
- $e = \text{Margin of Error}$ (e.g. $0.05$ for $\pm 5\%$, $0.07$ for $\pm 7\%$).
- $N = \text{Total Universe Size}$.

#### Sample Size Comparison Table:
| Universe ($N$) | Specialist Category | Required $n$ for $\pm 5\%$ MOE | Required $n$ for $\pm 7\%$ MOE | Required $n$ for $\pm 10\%$ MOE |
|---|---|---|---|---|
| **100,000+** | Primary Care / General Practice | 384 | 196 | 96 |
| **5,000** | General Cardiologists | 357 | 189 | 94 |
| **1,500** | Medical Oncologists | 306 | 174 | 90 |
| **400** | Pediatric Hematologists (Rare) | 196 | 132 | 77 |

**Analyst Insight:**
In rare diseases ($N = 400$), surveying $n = 132$ captures **33% of the entire national medical universe**, yielding a highly reliable $\pm 7\%$ margin of error.

---

### 25.9 Post-Stratification Survey Weighting Mathematics

When survey respondent proportions do not match known national population benchmarks, you must compute **cell weights**.

#### Weight Formula:
$$w_i = \frac{P_i}{S_i} = \frac{\text{Population Share of Segment } i}{\text{Sample Share of Segment } i}$$

#### Weighted Metric Calculations:
$$\bar{X}_{\text{weighted}} = \frac{\sum_{i=1}^{n} (w_i \cdot x_i)}{\sum_{i=1}^{n} w_i}$$
$$P_{\text{weighted}} = \sum_{k} (w_k \cdot p_k) \quad \text{where } \sum w_k = 1$$

#### Worked Example: 3-Segment Decile Weighting
A pharmaceutical brand surveys 200 neurologists regarding prescribing intent for a novel Alzheimer's therapy.

| Decile Segment | Universe Population ($N$) | Population Share ($P$) | Survey Sample ($n$) | Sample Share ($S$) | Weight Factor ($w = P/S$) | Segment Intent to Prescribe (%) |
|---|---|---|---|---|---|---|
| **Tier 1 (High Vol)** | 300 | 15.0% | 60 | 30.0% | $0.15 / 0.30 = \mathbf{0.50}$ | 85.0% |
| **Tier 2 (Mid Vol)** | 700 | 35.0% | 80 | 40.0% | $0.35 / 0.40 = \mathbf{0.875}$| 60.0% |
| **Tier 3 (Low Vol)** | 1,000 | 50.0% | 60 | 30.0% | $0.50 / 0.30 = \mathbf{1.667}$| 30.0% |
| **Total** | **2,000** | **100.0%** | **200** | **100.0%** | — | — |

**1. Naive (Unweighted) Overall Intent:**
$$\text{Naive Intent} = \frac{(60 \times 85.0\%) + (80 \times 60.0\%) + (60 \times 30.0\%)}{200} = \frac{51 + 48 + 18}{200} = \frac{117}{200} = \mathbf{58.5\%}$$

**2. Correct Weighted Overall Intent:**
$$\text{Weighted Intent} = (0.15 \times 85.0\%) + (0.35 \times 60.0\%) + (0.50 \times 30.0\%) = 12.75\% + 21.0\% + 15.0\% = \mathbf{48.75\%}$$

**So what does this mean for the business?**
The unweighted survey reported a 58.5% intent to prescribe. But because high-volume doctors were over-represented in the survey (30% in sample vs. 15% in reality), the raw data **inflated true national market demand by nearly 10 percentage points (58.5% vs. 48.75%)**. 
If the commercial team built a revenue forecast on 58.5%, they would over-manufacture inventory and set unachievable sales rep quotas.

---

### 25.10 Cross-Tabulation & Reading Banner Tables

A **Banner Table** is the standard deliverable in commercial market research. It displays questions as rows cross-tabulated across multiple audience cuts (columns).

```
                            ILLUSTRATIVE BANNER TABLE
                         
                         TOTAL       PRACTICE SETTING          REGION
                        SAMPLE    Academic   Community     North      South
                        (N=250)    (n=100)    (n=150)     (n=130)    (n=120)
                          (A)        (B)        (C)         (D)        (E)
Q: Likelihood to Rx
Top-2-Box (4 or 5)       62.0%      76.0%*     52.7%       63.1%      60.8%
- 5 (Definitely will)    28.0%      40.0%*     20.0%       29.2%      26.7%
- 4 (Probably will)      34.0%      36.0%      32.7%       33.8%      34.2%
Neutral (3)              20.0%      14.0%      24.0%*      19.2%      20.8%
Bottom-2-Box (1 or 2)    18.0%      10.0%      23.3%*      17.7%      18.3%
-----------------------------------------------------------------------------
Mean Score (1–5 scale)   3.70       4.02*      3.49        3.72       3.68
```
*\* indicates statistically significant difference at 95% confidence ($p < 0.05$) against the comparison column.*

#### How Analysts Extract Commercial Insights from Banner Tables:
1. **Look for Column-Letter Notation:** If column B has a letter or asterisk next to 76.0%, it means Academic doctors have a statistically significant lead over Community doctors (52.7%).
2. **Examine the Distribution Shape:** Look at the Bottom-2-Box. Community doctors have 23.3% resistance vs. only 10.0% in Academics. This shows private doctors have structural barriers (co-pay, access) that don't affect hospital doctors.
3. **Audit Cell Bases ($n$):** Never quote a percentage from a banner column where $n < 30$. Small cells have wide confidence intervals ($\pm 18\%+$) and are statistically unreliable.

---

## Part D — Translating Survey Insights into Commercial Action

### 25.11 Stated vs. Derived Importance (The Key Driver Model)
Physicians are notoriously poor at self-reporting what truly drives their behavior.
- **Stated Importance:** Doctor rates: *"How important is Clinical Efficacy when choosing a drug?"* $\rightarrow$ Result: **9.8 / 10**. *"How important is Patient Co-Pay?"* $\rightarrow$ Result: **5.2 / 10** (Doctors want to be viewed as clinical scientists, not financial accountants).
- **Derived Importance:** The analyst runs a **multivariate regression** linking attribute ratings to actual prescription share:
  $$\text{Brand Share} = \beta_0 + \beta_1(\text{Efficacy}) + \beta_2(\text{Safety}) + \beta_3(\text{Co-Pay Tier}) + \beta_4(\text{Dosing})$$
  The resulting standardized $\beta$ coefficients reveal that **Co-Pay Tier ($\beta = +0.42$)** drives prescribing decisions far more than Efficacy ($\beta = +0.15$), because all marketed drugs in the class already meet the minimum efficacy threshold!

#### The Importance-Performance Action Matrix:
```
                      The Importance-Performance Grid
                      
      High Derived │  QUADRANT 2:            │  QUADRANT 1:
      Importance   │  KEY VULNERABILITIES    │  CORE HERO DRIVERS
      (What drives │  (High impact on share, │  (High impact on share,
       prescribing)│   our brand scores low) │   our brand scores high)
                   │  -> MUST FIX IMMEDIATELY│  -> LEAD EVERY SALES CALL
                   ├─────────────────────────┼─────────────────────────
      Low Derived  │  QUADRANT 3:            │  QUADRANT 4:
      Importance   │  SECONDARY NOISE        │  OVER-INVESTMENT TRAP
      (What does   │  (Low impact on share,  │  (Low impact on share,
       not drive   │   our brand scores low) │   our brand scores high)
       prescribing)│  -> IGNORE / LOW BUDGET │  -> CEASE DETAIL PROMOTION
                   └─────────────────────────┴─────────────────────────
                        Low Performance            High Performance
                                Our Brand Attribute Score
```

---

### 25.12 The Intent-to-Prescribe "Haircut" (Discounting) Model
When surveyed, physicians express aspirational optimism. If an analyst directly feeds survey intent into a financial forecast, the client will over-invest and fail.

Industry standard **Haircut Models** apply empirically validated conversion factors:

| Survey Response Category | Stated Intention | Standard Industry Conversion Factor | Discounted Forecasted Adoption |
|---|---|---|---|
| **Definite Prescribers** (Score 5 / 5) | "Definitely will prescribe" | **60% to 75%** | $100 \text{ doctors} \rightarrow \mathbf{65 \text{ actual prescribers}}$ |
| **Probable Prescribers** (Score 4 / 5) | "Probably will prescribe" | **20% to 30%** | $100 \text{ doctors} \rightarrow \mathbf{25 \text{ actual prescribers}}$ |
| **Neutral Prescribers** (Score 3 / 5) | "Might or might not" | **5% to 10%** | $100 \text{ doctors} \rightarrow \mathbf{5 \text{ actual prescribers}}$ |
| **Bottom-2-Box** (Scores 1–2 / 5) | "Unlikely to prescribe" | **0%** | $100 \text{ doctors} \rightarrow \mathbf{0 \text{ actual prescribers}}$ |

#### Worked Example: Haircut Demand Forecast
A survey of 500 targeted dermatologists for a novel psoriasis biologic yields:
- 150 doctors (30%) select "Definitely will prescribe".
- 200 doctors (40%) select "Probably will prescribe".
- 100 doctors (20%) select "Might or might not".
- 50 doctors (10%) select "Unlikely to prescribe".

**Naive Prescriber Pool:** $150 + 200 = 350 \text{ doctors}$ (70.0% of market).

**Realistic Haircut Prescriber Pool:**
$$\text{Expected Prescribers} = (150 \times 0.65) + (200 \times 0.25) + (100 \times 0.05) + (50 \times 0.00)$$
$$\text{Expected Prescribers} = 97.5 + 50.0 + 5.0 + 0 = \mathbf{152.5 \text{ doctors (30.5\% of market)}}$$

**So what does this mean for the business?**
The raw survey indicates 350 prescribers, but reality will deliver ~152 prescribers. Applying the haircut prevents the commercial team from over-hiring sales reps and anchors manufacturing forecasts to realistic adoption velocity.

---

## Part E — Practical Exercises & Verification Drills

### Exercise 1: Post-Stratification Survey Weighting
You surveyed 300 Diabetologists regarding adoption of a novel oral SGLT2/DPP4 combination drug in India. 
- High-Volume Metro Endocrinologists: 120 surveys completed (Population universe = 800; total universe share = 20.0%).
- Mid-Volume City Diabetologists: 120 surveys completed (Population universe = 1,600; total universe share = 40.0%).
- Semi-Urban Primary Care Diabetologists: 60 surveys completed (Population universe = 1,600; total universe share = 40.0%).

Survey Question: *"Do you intend to prescribe this drug within 90 days of launch?"*
- High-Volume Metro: 75.0% Yes.
- Mid-Volume City: 50.0% Yes.
- Semi-Urban PCP: 25.0% Yes.

**Tasks:**
1. Compute the sample share for each segment.
2. Calculate the post-stratification weight factor ($w_i = P_i / S_i$) for each group.
3. Compute the naive (unweighted) overall intent % vs. the true weighted overall intent %.
4. What is the business implication of reporting the unweighted number to the commercial VP?

---

### Exercise 2: Banner Table Significance & Commercial Decision
You analyzed a banner table comparing two detailing messages delivered by sales representatives:
- Message A: *"Superior 24-hour Glycemic Control and HbA1c reduction (-1.8%)"* (Tested with $n = 150$ doctors).
- Message B: *"Proven Cardiovascular Event Risk Reduction and Renal Protection"* (Tested with $n = 150$ doctors).

Results:
- Message A: T2B Prescribing Intent = 58.0% (87 of 150).
- Message B: T2B Prescribing Intent = 72.0% (108 of 150).

**Tasks:**
1. Conduct a two-proportion z-test at $\alpha = 0.05$ to determine if Message B is statistically superior.
2. If Message B is superior, why do sales reps currently spend 65% of their detailing time delivering Message A?
3. Formulate an actionable operational recommendation for sales leadership.

---

## Part F — Complete Answer Keys & Worked Solutions

---

### §25.13.1 Answer Key — Exercise 1: Post-Stratification Weighting

#### 1. Sample Shares ($S_i = n_i / n_{\text{total}}$):
- High-Volume Metro: $120 / 300 = \mathbf{40.0\%}$.
- Mid-Volume City: $120 / 300 = \mathbf{40.0\%}$.
- Semi-Urban PCP: $60 / 300 = \mathbf{20.0\%}$.

#### 2. Post-Stratification Weights ($w_i = P_i / S_i$):
- High-Volume Metro Weight: $20.0\% / 40.0\% = \mathbf{0.50}$.
- Mid-Volume City Weight: $40.0\% / 40.0\% = \mathbf{1.00}$.
- Semi-Urban PCP Weight: $40.0\% / 20.0\% = \mathbf{2.00}$.

#### 3. Intent Calculations:
- **Naive (Unweighted) Intent:**
  $$\text{Naive Intent} = \frac{(120 \times 0.75) + (120 \times 0.50) + (60 \times 0.25)}{300} = \frac{90 + 60 + 15}{300} = \frac{165}{300} = \mathbf{55.0\%}$$
- **True Weighted Intent:**
  $$\text{Weighted Intent} = (0.20 \times 75.0\%) + (0.40 \times 50.0\%) + (0.40 \times 25.0\%) = 15.0\% + 20.0\% + 10.0\% = \mathbf{45.0\%}$$

#### 4. Business Implication:
Reporting the unweighted 55.0% inflates national commercial readiness by **10 full percentage points** (+22.2% relative error). High-volume metro doctors were over-sampled by 2x (representing 40% of the survey vs. 20% of the true population). Management would set revenue targets that are mathematically unattainable.

---

### §25.13.2 Answer Key — Exercise 2: Banner Table Significance Testing

#### 1. Statistical Hypothesis Testing:
- Message A: $p_1 = 87 / 150 = 0.580$.
- Message B: $p_2 = 108 / 150 = 0.720$.
- Difference: $p_2 - p_1 = 0.720 - 0.580 = +0.140$ (+14.0 percentage points).

**Pooled Proportion ($\bar{p}$):**
$$\bar{p} = \frac{87 + 108}{150 + 150} = \frac{195}{300} = 0.650$$

**Standard Error ($SE$):**
$$SE = \sqrt{0.650 \times (1 - 0.650) \times \left(\frac{1}{150} + \frac{1}{150}\right)} = \sqrt{0.650 \times 0.350 \times 0.01333} = \sqrt{0.003033} \approx \mathbf{0.05507}$$

**Test Statistic ($z$):**
$$z = \frac{0.140}{0.05507} = \mathbf{2.542}$$

*Statistical Conclusion:*
Since $z = 2.542$ exceeds the critical threshold of $1.96$ ($p = 0.011 < 0.05$), the difference is **statistically significant at the 95% confidence level**. Message B (Cardiovascular/Renal Protection) is definitively superior to Message A.

#### 2. Root Cause of Rep Detailing Drift:
Sales reps default to Message A (HbA1c efficacy) because reps are historically conditioned to sell acute numbers, and glycemic control is easier to explain in a 2-minute rushed clinic visit. Cardiovascular and renal outcome data requires discussing complex trial designs, which reps avoid unless trained and incentivized.

#### 3. Commercial Recommendation:
1. **Mandate Detailing Script Overhaul:** Re-certify all 60 field reps on the cardiovascular outcomes study within 30 days.
2. **Restructure Core Visual Aids (CVA):** Position the Cardiovascular & Renal Protection graphic on Page 1 of the tablet detailing aid, pushing HbA1c to the backup appendix.
3. **Incentive Compensation:** Tie 25% of quarterly field bonuses to demonstrated mastery and delivery of Message B during accompanied manager field rides.

---

## 25.14 Master Checklist: Pharmaceutical Survey Proficiency

Before moving to Module 26, ensure you can execute all core market research tasks:
- [ ] I can design an unqualified physician screener with specialty, volume, and conflict filters.
- [ ] I can identify and re-write leading, double-barreled, and unbalanced survey questions.
- [ ] I know how to calculate sample sizes using the Finite Population Correction (FPC).
- [ ] I can compute post-stratification cell weights ($w = P/S$) and calculate weighted means.
- [ ] I can read a multi-column banner table and identify statistically significant gaps ($z \ge 1.96$).
- [ ] I understand the difference between Stated Importance and Derived Importance.
- [ ] I can apply an Intent-to-Prescribe Haircut Model to convert survey scores into realistic forecasts.

**Next Module:** [26-client-communication-and-insight-storytelling.md](26-client-communication-and-insight-storytelling.md)
