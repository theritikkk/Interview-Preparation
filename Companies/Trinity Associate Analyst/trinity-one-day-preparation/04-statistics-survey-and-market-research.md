# 04 — Statistics, Survey & Market Research

> **Learning objectives:** interpret survey numbers correctly *out loud* — central tendency, spread, sample size, confidence intervals, significance, bias — and speak fluently about how market research is designed (surveys, samples, screening, qual/quant) using pharma/physician examples.
> **Estimated study time:** 50 min (reading + reproducing one CI + practice set).
> **Priority:** **MUST KNOW** §§4.1, 4.3–4.7; **SHOULD KNOW** the rest.
> **Why it matters:** the JD (CONFIRMED) explicitly requires *"ability to conduct statistical analysis on survey data"* and survey creation/reporting — this is the intellectual core of the role.

Rule for the interview: **state the number, state the uncertainty, state what you'd do next.** Never present a survey % without its base (n) and margin.

---

## §4.1 Central tendency and spread

| Concept | Definition | Pharma example | Interview line |
|---|---|---|---|
| Mean | Sum ÷ count | Avg TRx per rep | "Mean is pulled by high prescribers" |
| Median | Middle value (sort first) | Typical physician's monthly scripts | "Median describes the typical; mean describes the total" |
| Mode | Most frequent value | Most common Likert response | Mostly for counts / discrete data |
| Variance / SD | Average squared deviation / its √ | Spread of TRx across territories | "SD tells interviewers you understand *consistency*, not just averages" |
| Outlier / skew | Extreme value / asymmetric tail | Few "high-decile" physicians prescribing far more | Right-skewed → use median or trim; always *investigate*, never auto-delete |

**Worked example.** Prescriptions per physician: 4, 7, 7, 9, 13 → mean **8**, median **7**, mode **7**. The mean exceeds the median → slight right skew (a few bigger writers).

**SD intuition.** `{5,5,5,5}` and `{2,5,5,8}` share a mean of 5, but only the second has spread (population SD ≈ **2.12** vs **0**). Two regions can have the same average TRx and completely different risk profiles.

**Traps.** ① Reporting a mean for a skewed metric without noting skew. ② "Most physicians prescribe 7" — that's the mode, not the mean. ③ Comparing SDs across different units.

## §4.2 Percentiles, quartiles, and concentration

- **Pth percentile** = value below which P% of observations fall. **Q1/P25, median/P50, Q3/P75**. **IQR = Q3 − Q1** (robust spread).
- **Pharma reality:** prescribing is heavily concentrated — often the **top decile (P90+)** of physicians drives a very large share of TRx. Strategy is usually built around these writers.
- Interview line: *"Before averaging, check concentration: if the top 10% carries 40% of TRx, an average is a misleading planning number — segment first."* (**ADDITIONAL** — standard industry pattern.)

## §4.3 Samples: populations, techniques, bias

- **Sample vs population:** the population is everyone of interest (all physicians in a specialty); the sample is who you reached. Survey stats = *sample estimate ± uncertainty*.
- **Techniques (AWARENESS in depth; know 1-liners):** random (everyone equal chance); stratified (sample within subgroups — e.g., by specialty or region — then weight); cluster (sample groups, e.g., 20 hospitals); quota/convenience (fast but biased — common in commercial panels).

| Bias | What it looks like | Pharma example | Direction |
|---|---|---|---|
| Coverage / frame | Some population members can't be sampled | Survey by email misses non-online physicians | Unknown, often under-represents rural |
| Nonresponse | Who answers differs from who doesn't | Busiest high writers skip surveys | Usually *overstates* engagement |
| Selection / self-selection | Participation tied to interest | "Adoption survey" attracts adopters | Overstates adoption |
| Social desirability | Flattering answers | Over-reporting guideline adherence | Overstates "good" behavior |
| Recall | Memory error | "How many scripts last quarter?" | Noisy, regressive toward the mean |

**Interview line:** *"Any survey result is only as good as the sample frame — I'd ask who was in frame, who responded, and whether non-responders differ."*

## §4.4 Survey question design and Likert scales

**Design rules:** one idea per question · neutral wording · no leading questions · avoid double-barreled questions · balanced response options · include screeners (qualify the respondent) · keep it as short as the objective allows.

**Worked example — leading vs neutral:**
- Weak (leading): *"Don't you agree that the new once-weekly dosing is more convenient?"*
- Better (comparative, neutral): *"Compared with the previous daily dosing, how would you rate the convenience of once-weekly dosing? (Much worse → Much better, 5 points)"*

**Likert scales:** agreement or rating scales (e.g., 1–5). Report the **% top-2-box** ("very satisfied" + "satisfied") because it maps to action; also report the **mean score** for comparisons over time. Distribution matters: two statements can share the same mean (3.0) with very different spread and different **net positive** scores.

**Worked example.** Satisfaction 5-point distribution: 5s = 30%, 4s = 25% → **top-2-box = 55%**.

**Reliability vs validity:** reliability = consistency (same answer on repeat); validity = measuring the right thing (satisfaction scale truly captures satisfaction, not habit). Fine in interviews: *"reliable but not valid = consistently measuring the wrong thing."*

## §4.5 Response distributions → insight

Distributions, not just averages: percent agreement, top-2-box, net scores (e.g., **NPS = %promoters − %detractors**; passives ignored). Cross-tabulate by segment (specialty, region, adoption stage) to find where the action is.

**Worked example.** Adoption-barrier question (multi-select): awareness 34%, cost/reimbursement 28%, safety concerns 22%, other 16%. Insight, not just report: *"awareness is the largest single barrier, but ~a quarter cite safety — those two need different interventions (education vs evidence)."*

**Common mistake:** reporting every question's % in isolation instead of ranking barriers and linking to recommended action.

## §4.6 Sample size, uncertainty, and confidence intervals — MUST KNOW

- **Margin of error (95%):** `MoE ≈ 1.96 × √(p(1−p)/n)`. CI = % ± MoE.
- **Worst case p = 0.5** maximizes MoE for a given n.
- **Sample size formula:** `n = z² × p(1−p) ÷ e²` (z = 1.96 for 95%).

**Worked example 1 (compute this by hand once).** Physician survey, n = 250, awareness = 60% → MoE = 1.96 × √(0.6 × 0.4 / 250) = 1.96 × 0.031 = **±6.1 pp** → CI **[53.9%, 66.1%]**.
**Worked example 2.** n = 500, adoption = 64% → 1.96 × √(0.64 × 0.36/500) = **±4.2 pp** → [59.8%, 68.2%].
**Sample-size example.** Want ±5 pp at 95%: n = (1.96² × 0.25)/0.05² = **385** (≈ 400 as standard practice).

**Traps.** ① "60% aware" → no, "60% ± 6 pp". ② More precision → quadruple the sample (MoE ∝ 1/√n — halving the MoE needs 4× the n). ③ Sub-group CIs balloon: quarter of the sample → roughly double the MoE.

## §4.7 Hypothesis testing, p-values, significance (conceptual level is enough)

- **H₀ (null):** "no difference" (e.g., awareness is the same across two segments). **p-value:** probability of data this extreme *if H₀ were true*. **p < 0.05** → conventionally "statistically significant" → reject H₀.
- **Significant ≠ important.** With huge n, a trivial 0.5 pp difference can be significant. Always pair it with **practical significance** (is the difference big enough to change a decision?).

**Worked example (not significant).** Awareness: 60% (n = 250) vs 54% (n = 250). Difference 6 pp; two-group z ≈ **1.36**, p ≈ **0.18** → not significant; the CIs overlap. Interview answer: *"the 6-point gap is within noise — I wouldn't target one segment differently without a larger sample."*
**Worked example (significant).** 55% vs 47%, n = 400 each: z ≈ **2.27**, p ≈ **0.023** → significant at 95% (barely). Practical read: an 8 pp gap on adoption warrants action.

**Interview vocabulary:** "statistically significant", "confidence we'd see this by chance", "directionally interesting but under-powered" (for small samples).

## §4.8 Correlation vs causation

Three explanations for any A–B association: A → B; B → A (reverse); C → both (confounder). **Pharma example:** rep calls correlate with TRx — but territories with more potential get more reps (confounding by potential); that's why causal claims need experiments (e.g., pilot territories) or careful controls (**ADDITIONAL**).

**Interview line:** *"The correlation tells me where to look; it doesn't tell me what to do yet."*

## §4.9 Weighted survey results

When your sample's composition ≠ the population's (e.g., you over-sampled a small region), **weight** each respondent by inverse selection probability so the overall number reflects the population.

**Worked example.** Region response rates: North 60% (n = 300), South 45% (n = 200), West 55% (n = 500).
- Pooled (correct if these are true counts): (180 + 90 + 275) / 1000 = **54.5%**.
- Simple average of the three rates: (60 + 45 + 55)/3 = **53.3%** — wrong because n differs.
**Mistake:** averaging percentages across strata of different sizes (same trap as [`02` §2.3](02-aptitude-reasoning-and-data-interpretation.md)).

## §4.10 Market research toolkit (how the role actually works)

| Pair | Options | One-liner |
|---|---|---|
| Qualitative vs quantitative | Interviews/FIEs/roundtables vs structured surveys | Qual = *why*, quant = *how many/how much*; qual drives hypotheses, quant measures them |
| Primary vs secondary | You field it vs existing/ syndicated data | Primary = control & specificity; secondary = fast & cheap |
| Custom vs syndicated | Bespoke client survey vs multi-client panels/audits | Custom answers client-specific questions; syndicated gives benchmarks (ADDITIONAL) |
| Screening | Qualification questions/segments | Ensure respondents match the target (e.g., treat ≥ X patients/month) before counting |
| Reporting | Top-2-box, nets, cross-tabs | Insight = finding + so-what + action |

**Limitations of self-reported data (say this in any interview):** recall bias, social desirability, question framing effects, non-response, small subgroup samples. **Turning responses into insight:** rank drivers/barriers, cross-tab by segment, tie to behavior (adoption), state the limitation and the next study.

---

## §4.11 Quick practice (12 min) — answers in §4.12

**S1.** Physician scores: 4, 7, 7, 9, 13. Mean, median, mode?
**S2.** Which dataset has larger spread: {5,5,5,5} or {2,5,5,8} — and why?
**S3.** Survey n = 400, awareness = 55%. MoE and 95% CI (1 dp)?
**S4.** Two segments: 55% vs 47%, n = 400 each. Significant at 95%? (z = 2.27)
**S5.** 80% of respondents scored 6/10, 20% scored 9/10. Weighted mean?
**S6.** Fix this question: *"Don't you agree once-weekly dosing is much more convenient?"* — name the flaw, propose a neutral version.

### §4.12 Answers

| Q | Answer |
|---|---|
| S1 | Mean **8**, median **7**, mode **7** — right skew |
| S2 | **{2,5,5,8}** — same mean, nonzero deviation (pop. SD ≈ 2.12 vs 0) |
| S3 | MoE = 1.96 × √(0.55×0.45/400) = **±4.9 pp** → CI **[50.1%, 59.9%]** |
| S4 | z = 2.27 → p ≈ 0.023 < 0.05 → **significant** (but narrowly — note the practical size, 8 pp) |
| S5 | 0.8×6 + 0.2×9 = **6.6** |
| S6 | Flaw: **leading question** (presumes agreement + "much"). Neutral: *"Compared with daily dosing, how would you rate the convenience of once-weekly dosing?"* with a balanced 5-point scale |

**Next:** [05 — Power BI, Tableau & data visualization](05-power-bi-tableau-and-data-visualization.md) · **Interview lines:** [09](09-technical-interview-question-bank.md) §§9.3–9.4
