# 18 — Final Revision Cheatsheet (30–45 Minutes)

**Purpose:** the last document you read before the assessment or interview. Everything here has appeared elsewhere in this repository in more detail — this is the compressed version, not new material.
**Prerequisites:** having worked through the main files.
**How to use:** 30–45 minutes total. Read each section, then cover it and recall it out loud. Do not re-derive anything.
**Timing guide:** formulas 10 min · Excel 8 min · statistics 7 min · DI checklist 5 min · cases 8 min · traps + behavioral 7 min.

---

## 1. Quantitative formulas (10 minutes)

| Concept | Formula | One-line example |
|---|---|---|
| % change | (New − Old)/**Old** | 840 → 1,050 = +25% |
| Successive changes | Multiply multipliers | −15% then +15% = 0.85 × 1.15 = −2.25% |
| Reverse | Original = New/(1 + change) | 1,140 after +20% → 950 |
| Weighted average | Σ(w × x)/Σw | 40@120 + 60@180 → 156 |
| Average speed (equal distance) | 2ab/(a+b) | 60 & 80 → 68.6 |
| Combined mean (two groups) | (n₁x̄₁ + n₂x̄₂)/(n₁+n₂) | 200@4.2 + 50@3.6 → 4.08 |
| Gross margin | (Rev − COGS)/Rev | (500−200)/500 = 60% |
| Operating margin | (Rev − COGS − SG&A − R&D)/Rev | 150/500 = 30% |
| Markup vs margin | Markup = profit/cost | 25% markup = 20% margin |
| Break-even units | Fixed cost ÷ contribution per unit | ₹60 Cr ÷ ₹1,500 = 4 lakh |
| Work together | 1/(1/a + 1/b) | 12 & 24 → 8 days |
| Probability "at least one" | 1 − P(none) | 2/10, two draws → 37.8% |
| Rules of thumb | 10% of X, ×2, ×3; ×0.25 = ÷4 | 17% of 4,980 ≈ 847 |

---

## 2. Growth, share and funnel formulas (part of §1 and §7)

| Concept | Formula |
|---|---|
| CAGR | (End/Begin)^(1/n) − 1, where **n = number of intervals** |
| Compound growth | Begin × (1+g)^n |
| Rule of 72 | Years to double ≈ 72/growth% |
| Share | Brand revenue ÷ Market revenue |
| Share drift | Old share × (1 + brand growth)/(1 + market growth) |
| Percentage points | Current % − prior % (shares, awareness, penetration) |
| Revenue-neutral volume after a price cut | 1/(1 − cut%) − 1 → a 10% cut needs **11.1%** volume |
| Price × volume revenue change | (1 + Δp)(1 + Δv) → −10% & +12% = **+0.8%** |
| ROI | (Gross profit − spend)/spend |
| Funnel volume | Base × conversion₁ × conversion₂ … |
| Sales-force sizing | (Physicians × calls)/rep capacity → 4,800 × 8/800 = 48 |

**Funnel reference numbers (from [03](03-data-interpretation-and-graphs.md)):** awareness 64% → consideration 46.9% of aware (30% of all) → trial 70% of considerers (21% of all) → regular 42.9% of triallists (**9% of all**).

---

## 3. Excel syntax (8 minutes)

```excel
=SUM(D2:D8)                                   =AVERAGE(D2:D8)          =MIN/MAX(D2:D8)
=COUNT(D2:D8)          numbers only           =COUNTA(A2:A8)           non-empty
=COUNTIF(B2:B8,"North")                        =COUNTIFS(B:B,"North",C:C,"CardioX")
=SUMIF(C2:C8,"CardioX",D2:D8)                  ← sum range LAST
=SUMIFS(D2:D8,B2:B8,"North",C2:C8,"CardioX")   ← sum range FIRST
=AVERAGEIF(C2:C8,"CardioX",D2:D8)              =AVERAGEIFS(D2:D8,B2:B8,"North")
=IF(D2/E2>=1,"Above","Below")                  =IFS(a>=1.1,"Exceeds",a>=0.9,"On track",TRUE,"Below")
=AND(x,y)   =OR(x,y)                           =IFERROR(v,"")   =IFNA(v,"Not found")
=VLOOKUP("R-003",A2:E8,4,FALSE)                =XLOOKUP("R-003",A2:A8,D2:D8)
=INDEX(D2:D8,MATCH("R-003",A2:A8,0))
=ROUND(D2/E2*100,1)    =TEXT(D2,"#,##0")       =EOMONTH(A2,0)   =DATEDIF(a,b,"M")   =NETWORKDAYS(a,b)
=LEFT(A2,6)  =RIGHT(A2,3)  =MID(A2,3,3)  =LEN(A2)  =TRIM(A2)  =PROPER(A2)  =SUBSTITUTE(A2,",","")
=A2&" - "&C2            =TEXTJOIN(", ",TRUE,A2:A8)       =TEXTJOIN
=COUNTBLANK(G2:G31)    =SUMPRODUCT(--NOT(ISNUMBER(G2:G31)))    ← text-number detector
```
**References:** `A1` relative · `$A$1` absolute · `A$1` row locked · `$A1` column locked. **F4 cycles.**

---

## 4. Lookup comparison (2 minutes)

| | VLOOKUP | XLOOKUP | INDEX-MATCH |
|---|---|---|---|
| Direction | Right only | Any | Any |
| Default match | **Approximate** (omit = danger) | Exact | Exact (with 0) |
| Column insert risk | Yes (index shifts) | None | None |
| Version | All | 365/2021+ only | All |
| Fallback code | `VLOOKUP(k,t,n,FALSE)` | — | `IFNA(INDEX(r,MATCH(k,l,0)),"x")` |

**Rules:** exact match by default; count lookup failures (`COUNTIF(range,"#N/A")`); TRIM keys before matching.

---

## 5. Statistics in one line each (7 minutes)

- **Mean vs median:** mean follows outliers; median resists → report both for skewed data (prescriptions per physician).
- **SD:** `STDEV.P` = whole population; `STDEV.S` = sample. Variance = 12.8 → SD = 3.58 (dataset 10,12,12,16,20).
- **Quartiles:** `QUARTILE.INC`; IQR = Q3 − Q1 (robust spread).
- **Margin of error (95%):** 1.96 × √(p(1−p)/n) → 52%, n = 300 → ±5.7 pp (CI 46.3–57.7%).
- **Sample size:** n = 1.96² × 0.25/e² → ±5 pp = **384**; ±4 pp = **600**.
- **Response rate:** responses ÷ invitations (1,200/4,000 = 30%).
- **Weighting:** weight = population share ÷ sample share (0.30/0.50) → unweighted 4.00 vs weighted **3.80**.
- **CI meaning:** repeats of the study would produce intervals containing the truth ~95% of the time. Not a probability about this one interval.
- **p-value:** P(data this extreme | H₀ true). Not the probability H₀ is true, not effect size.
- **Significance:** p < α rejects H₀; always add effect size and business relevance (n = 25,000 can make 1 pp "significant" and useless).
- **Correlation:** r = strength of linear association; r² = share of variance linearly associated. **Neither implies causation.**
- **Bias types:** selection, nonresponse, survivorship, social-desirability, acquiescence, leading questions.
- **Multiple comparisons:** 20 tests at α = 0.05 → ~1 false positive; correct or replicate.

---

## 6. Data interpretation checklist (5 minutes)

1. Units and **base** (share of what? volume or value?).
2. Absolute change **and** % change.
3. Largest contributor **vs** fastest grower — state both, name which matters.
4. Check the axis: truncation, dual axis, time spacing, 3-D pies.
5. One main finding → one implication → one missing data item.
6. Percentage points for shares; % for quantities.
7. Weighted average whenever group sizes differ.
8. Correlation ≠ causation; outliers are investigated, not deleted.

**Standard five questions:** main finding · what changed most · which segment contributes most · business implication · what else do I need.

---

## 7. Market-entry framework (8 minutes)

**10 steps:** problem → decision → metrics → components (MECE) → data quality → calculate/compare → findings → implications → recommendation → risks & missing info.

**Market maths to have ready**
- Top-down chain: population → prevalence → diagnosed → treated → eligible → price.
- Bottom-up: physicians × patients × share × price.
- TAM ≠ SAM ≠ SOM; TAM is a boundary, never a forecast.
- Switchers vs naive adopters; persistence reduces revenue quality.
- Access is usually the binding constraint (formulary, prior authorisation, step therapy, co-pay).
- Report **net price**, not list (gross-to-net 30–60% in US brands).

**Always end with:** *recommendation + assumption it depends on + the gate you would set + the next step.*

---

## 8. Common traps — the ten that cost marks

1. Percent change with the **new** value as denominator.
2. **pp vs %** confusion (3 pp on an 18% share = 16.7% relative).
3. Simple average over **unequal groups**.
4. Quoting the wrong **margin** (gross vs operating) or revenue as profit.
5. Missing **units** (₹ vs $, lakh vs crore, volume vs value).
6. **Causation** from a correlation.
7. **Axis abuse** (truncation, dual axes) and cherry-picked base years.
8. Lookup ranges: `SUMIFS` order, `VLOOKUP` without `FALSE`, relative references in copied formulas.
9. **Blanks and duplicates** silently distorting totals (and text-numbers ignored by `SUM`).
10. Recommendation without **risk, assumption or condition**.

Full catalogue with corrections: [19-errors-mistakes-and-traps.md](19-errors-mistakes-and-traps.md).

---

## 9. Case interview structure (part of §7)

**7 parts, in order:** Objective → Framework → Analysis → Key findings (max 3, quantified) → Recommendation (committed) → Risks → Next steps.

**Sentence starters:**
- "The decision is whether to…"
- "I'll break this into… (4 parts)"
- "Using the data provided, … (units)"
- "Three findings stand out: one, … two, … three, …"
- "I recommend… because…"
- "The main risks are…"
- "To de-risk this, the next step is…"

**Defence lines:** "That's a fair challenge — I'd validate it with X before committing." · "If access is below 30% of lives, I would downgrade to a pilot." · "A 10% price cut needs >11.1% volume just to hold revenue."

---

## 10. Behavioral reminders (7 minutes)

- **STAR with one metric per story**; 90 seconds maximum.
- **Five stories ready** (delivery, quality catch, teamwork, fast learning, explaining technical work) → reusable across most questions.
- **Tell me about yourself:** present → two evidence points with numbers → why this role. (60–90 s)
- **Why analytics after software:** measurement and decisions, plus automation as an extra; never "I lost interest in coding".
- **Development area:** commercial/domain context, plus the action you are taking. Never claim a fatal gap.
- **Quality control answer:** validate inputs → cross-check outputs → re-read the question → automate the check. *This is the highest-value answer for this role.*
- **Logistics:** comfortable with Gurgaon/Bangalore and the **₹12 LPA campus CTC** (do not quote ₹17.4 LPA — that was an **OLDER JD** for a different role).
- **Ask two questions** from [12](12-behavioral-interview-and-hr.md) §12.4.
- Never invent life-sciences consulting experience.

---

## 11. On-the-day sequence

| When | Do |
|---|---|
| Night before | Read §1, §3, §5 here; sleep; no new topics |
| 60 min before | 5 quick mental-maths questions; skim §8 traps |
| During the assessment | Pass 1 fast → pass 2 marked → pass 3 check; answer everything |
| During Excel | Clean → aggregate → lookup → pivot → chart → conclusion; reconcile at the end |
| During the interview | Structure first, numbers with units, one recommendation, one risk, one next step |
| After | Write down every question you saw (honestly) and score with [17](17-mock-interview-evaluation-rubric.md) |

---

**Final reminder:** Trinity has not published an assessment syllabus; nothing in this repository is a leaked question. What you have here is the skill set that the JD, the candidate reports and general market practice all point to — prepared thoroughly enough to be useful under time pressure.

Good luck. Now close this file and go and solve something.
