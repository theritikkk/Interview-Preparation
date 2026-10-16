# 06 — Statistics and Survey Analytics

**Purpose:** the statistics an Associate Analyst actually needs — descriptive stats, survey mechanics, weighted results, confidence intervals, significance testing, and the limitations of each. Trinity's current JD explicitly requires "ability to conduct statistical analysis on survey data" (CONFIRMED), so this file is high priority.
**Prerequisites:** [01-quantitative-aptitude.md](01-quantitative-aptitude.md), [03-data-interpretation-and-graphs.md](03-data-interpretation-and-graphs.md).
**Suggested study time:** 3–4 hours.
**How to use:** read the intuition first, then the formula. If you remember the formula but not the caveat, you will lose marks in a case interview.

---

## 6.1 Descriptive statistics

### Mean, median, mode

| Statistic | Definition | Formula / method |
|---|---|---|
| Mean | Arithmetic balance point | Σx / n |
| Median | Middle value when sorted | Middle of sorted list (average of two middles if n even) |
| Mode | Most frequent value | Count frequencies; highest count wins (can be none or multiple) |

**Intuition:** the mean is pulled by extremes; the median resists them. In pharma data (prescriptions per physician), a few high writers can drag the mean far above what a "typical" physician does — report both.

**Worked example.** Prescriptions per physician for 5 physicians: 10, 12, 12, 16, 20.
- Mean = 70/5 = **14**; Median = **12**; Mode = **12**; Range = 10.
- Mean (14) > median (12) → distribution is **right-skewed** (a few high prescribers).

### Range, variance, standard deviation

| Statistic | Formula | Meaning |
|---|---|---|
| Range | Max − Min | Crude spread, sensitive to one outlier |
| Variance (population) | Σ(x − μ)² / N | Average squared distance from the mean |
| Variance (sample) | Σ(x − x̄)² / (n − 1) | Corrects for estimating the mean from the sample |
| SD | √variance | Spread in the original units |

**Worked example (same data).** Deviations from 14: −4, −2, −2, +2, +6 → squares 16, 4, 4, 4, 36 = 64.
- Population variance = 64/5 = **12.8** → sd = **3.58**
- Sample variance = 64/4 = **16.0** → sd = **4.00**
- In Excel: `=STDEV.P()` vs `=STDEV.S()` (older: `STDEVP` / `STDEV`).

**Which one?** If your data is *the whole population you care about* (all 57 territories, all survey respondents) → `STDEV.P`. If it is a *sample used to estimate a wider population* (300 surveyed physicians out of thousands) → `STDEV.S`.

### Percentiles and quartiles

- The p-th percentile is the value below which p% of the data lies.
- **Q1** = 25th percentile, **Q2** = median, **Q3** = 75th percentile; **IQR** = Q3 − Q1 (robust spread).
- Excel: `=QUARTILE.INC(range,1)`, `=PERCENTILE.INC(range,0.9)` (inclusive method — matches hand computation on small samples).

**Worked example (4, 9, 12, 16, 20):** Q1 = 9, Q2 = 12, Q3 = 16, IQR = 7.
**Use in business:** "the top quartile of reps delivers X% of TRx"; "90th percentile call time is 4.2 minutes".

---

## 6.2 Sampling and sample size

**Vocabulary**
- **Population:** everyone you want to conclude about (e.g., all US neurologists).
- **Sample:** the subset you actually measure.
- **Sampling frame:** the list you draw from (a directory, panel, congress attendee list).
- **Census:** everyone (rare and expensive).

**Key formulas**

| Concept | Formula |
|---|---|
| Margin of error (95%) for a proportion | 1.96 × √(p(1−p)/n) |
| Sample size for MOE e at 95% (p = 0.5 worst case) | n = (1.96² × 0.25)/e² |
| Standard error of a mean | s/√n |
| Response rate | Responses ÷ Invitations |
| Completion/cooperation adjustment required | n_required = n_effective ÷ expected response rate |

**Sample-size table (95% confidence, p = 0.5 worst case)**

| Desired margin of error | Required n |
|---|---|
| ±10 pp | 96 |
| ±7 pp | 196 |
| ±5 pp | **384** |
| ±4 pp | **600** |
| ±3 pp | 1,067 |
| ±2 pp | 2,401 |

**Worked example.** You need ±5 pp precision at 95%. Expect a 30% survey response rate.
- Required completes = 384 → invitations = 384/0.30 = **1,280 invitations**.

### Sampling bias and nonresponse bias

| Bias | What happens | Example | Mitigation |
|---|---|---|---|
| Selection bias | Frame excludes part of the population | Only congress attendees invited → over-represents academic physicians | Weight to known population mix; diversify frame |
| Nonresponse bias | Responders differ systematically from non-responders | Busy high prescribers don't respond; results skew to lower prescribers | Compare early vs late responders; short surveys; reminders; weight by specialty/volume |
| Survivorship bias | Only successful cases are observed | Studying only brands that launched successfully | Include failures; define the frame before the outcome |
| Social-desirability bias | Answers that look good | Overstating "intent to prescribe" | Indirect questions, behavioural anchors, discount stated intent |
| Acquiescence bias | Agreement with everything | All-positive responses on Likert scales | Mix reverse-coded items; balanced scales |
| Leading question bias | Wording cues the answer | "How much do you value our industry-leading dosing?" | Neutral wording; pilot test |

**Intuition to remember:** a huge sample does not fix a biased sample. n = 10,000 from a skewed frame is worse than n = 400 from a random one.

---

## 6.3 Survey question design and Likert scales

**Question-design rules**
1. One idea per question (no double-barrelled items).
2. Plain language; define clinical terms you must use.
3. Avoid leading, loaded and hypothetical-inflation wording.
4. Match the scale to the decision: awareness → aided/unaided; adoption → behavioural frequency; drivers → importance/satisfaction batteries.
5. Include "Don't know / Not applicable" options but track them separately — never force an opinion.
6. Pre-test with 5–10 respondents; fix wording before fielding.
7. Keep it short: completion falls non-linearly after ~10–12 minutes.

**Likert scales**

| Scale | Example | Best for |
|---|---|---|
| Agreement | Strongly disagree → strongly agree | Attitudes, perceptions |
| Likelihood | Very unlikely → very likely | Intent, adoption forecasts |
| Importance / satisfaction | Not important → critical | Driver analysis |
| 5-point vs 7-point | 5 simpler; 7 more discriminating | Balanced scales with a clear midpoint label |

**Analysis conventions**
- **Net Top-Box (T2B):** % choosing the top 2 points (e.g., 4–5 of 5) — the most common client-reading metric.
- **Net Promoter Score:** % (9–10) − % (0–6) on a 0–10 recommendation scale; range −100 to +100.
- **Mean scores:** fine for importance/satisfaction grids; state the scale (e.g., mean 4.08 on a 5-point scale).
- **Never:** average ordinal labels across questions with different scales, or add percentages from different bases.

**Worked example (from `data/survey_likert.csv`).** 20 respondents; 2 invalid entries (one "N/A", one out-of-range 7) → 18 valid. Top-2-box on Q1 = 12/18 = **66.7%**; mean = **3.83**; median = 4.
- Client phrasing: "Two-thirds of neurologists (67%) indicate they are likely or very likely to prescribe, with a mean likelihood of 3.8 on a 5-point scale (2 of 20 responses excluded as invalid)."

---

## 6.4 Weighted survey results

Surveys rarely mirror the population. If your sample is 60% community neurologists but the population is 45%, results must be **weighted** (post-stratification).

| Group | Population share | Sample share | Sample mean (1–5) |
|---|---|---|---|
| Academic | 30% | 50% | 4.5 |
| Community | 70% | 50% | 3.5 |

- Unweighted mean = (4.5 + 3.5)/2 = **4.00** (overstates, because academics are over-sampled)
- Weighted = 0.30 × 4.5 + 0.70 × 3.5 = **3.80**

**Weight formula:** weightᵢ = population shareᵢ / sample shareᵢ. Then weighted mean = Σ(wᵢ × x̄ᵢ)/Σwᵢ.

**Common traps:** weighting by a variable you did not measure; weights so large that a few respondents dominate (cap the weights); forgetting to weight standard errors (effective sample size shrinks with unequal weights).

---

## 6.5 Confidence intervals

**Meaning (read carefully — this is what interviewers probe):** a 95% confidence interval means that if you repeated the survey many times, approximately 95% of the intervals built this way would contain the true population value. It does **not** mean "there is a 95% chance the true value is in this particular interval" — strictly, the interval either contains it or not; the 95% refers to the procedure.

**Proportion (survey %):** p̂ ± 1.96 × √(p̂(1−p̂)/n)

**Worked example.** Top-2-box = 52%, n = 300.
- SE = √(0.52 × 0.48/300) = √0.000832 = 0.0288 → 2.88 pp
- MOE = 1.96 × 2.88 = **5.65 pp**
- 95% CI = **46.3% to 57.7%**

**Mean (e.g., Likert score):** x̄ ± t × s/√n (use ≈1.96 for large n).

**What widens the interval:** smaller n, p closer to 50%, more variable data, clustered sampling.
**What it does not fix:** bias. A biased estimate gives a precise interval around the wrong value.

---

## 6.6 Hypothesis testing, p-values and significance

**Structure of a test**
1. **H₀ (null):** no difference / no relationship ("new drug has the same prescribing rate as the old one").
2. **H₁ (alternative):** what you want to detect.
3. Choose significance level α (commonly 0.05).
4. Compute the test statistic (z/t/chi-square) or read the p-value from software.
5. Decide: **p < α → reject H₀** (statistically significant); p ≥ α → fail to reject (not "prove H₀").

**Common comparisons in survey work**

| Question | Test |
|---|---|
| Is brand awareness different from 50%? | One-sample z-test for a proportion |
| Do two segments differ in awareness? | Two-proportion z-test (or chi-square on the 2×2 table) |
| Are Likert means different across 3+ groups? | ANOVA (Kruskal–Wallis if ordinal/small) |
| Is specialty associated with adoption stage? | Chi-square test of independence |

**What a p-value is:** the probability of observing data at least as extreme as yours *if the null hypothesis were true*. It is **not** the probability that the null is true, and it is **not** effect size.

**Practical vs statistical significance** — the distinction that separates good analysts:
- With n = 10,000 you can "significantly" detect a 0.2 pp difference in awareness — statistically significant, commercially irrelevant.
- With n = 40, a 15 pp difference may not reach significance — commercially large, statistically noisy.
- Report: **effect size + confidence interval + business relevance**, not just "p < 0.05".

**Multiple comparisons:** test 20 hypotheses at α = 0.05 and you expect ~1 false positive. Adjust (Bonferroni) or interpret with caution — a classic trap in "we found 7 significant drivers" decks.

---

## 6.7 Correlation, causation, outliers and skew

- **Correlation (r):** strength and direction of a *linear* association, from −1 to +1. **r²** = share of variance in one variable *linearly associated* with the other.
- **Causation requires:** temporal order, a plausible mechanism, exclusion of confounders, ideally a controlled experiment (or strong quasi-experimental design such as difference-in-differences).
- **Confounder example:** physician volume correlates with both marketing spend received and prescriptions written — spending may follow high-volume physicians rather than create volume.
- **Outliers:** investigate before removing; they may be errors, or the most informative observations in the dataset. Report results with and without them.
- **Skew:** mean > median → right-skew; mean < median → left-skew. For skewed data prefer median/IQR, and log transforms for some analyses.

---

## 6.8 Practice set (S1–S15)

1. For 10, 12, 12, 16, 20: compute mean, median, mode, range.
2. Compute the population and sample standard deviations for the same data.
3. Compute Q1, Q2, Q3 and the IQR (inclusive method).
4. Which of mean/median is affected more by adding an outlier of 500 to this dataset?
5. Group A (n = 200) mean 4.2; Group B (n = 50) mean 3.6. Overall mean?
6. Show that the unweighted mean differs, and explain the direction of the error.
7. Invitations 4,000, responses 1,200. Response rate?
8. Sample size needed for ±5 pp at 95% confidence (p = 0.5)? And for ±4 pp?
9. With a 25% expected response rate, how many invitations for ±5 pp?
10. Top-2-box = 52%, n = 300. Compute the 95% CI.
11. Interpret: "awareness is 64% ± 5.5 pp". What is the precise meaning?
12. A survey reports p = 0.03 for a 1 pp difference in prescribing with n = 25,000. What should you say?
13. Marketing spend and TRx have r = 0.94 in 10 territories. Give one confounder and one design to test causality.
14. A 24-physician dataset shows 5 unaided-aware, 5 aided, 5 consideration, 5 triallists, 4 regular prescribers. What is the conversion from trial to regular, and what caveat applies to the number?
15. Nonresponse: a 20-minute survey of 500 busy oncologists gets a 12% response rate. Name the bias and two mitigations.

---

# Answers

**S1 — mean 14, median 12, mode 12, range 10.** Sum 70/5. Mean > median signals right skew.

**S2 — population sd 3.58; sample sd 4.00.** Squared deviations sum 64; divide by 5 or 4 respectively, then square-root.

**S3 — Q1 = 12, Q2 = 12, Q3 = 16, IQR = 4.** (Excel `QUARTILE.INC` gives Q1 = 12 for this 5-value set.) Q1 = Q2 here because the lower half is tight — a useful reminder that quartiles can coincide.

**S4 — the mean.** 10+12+12+16+20+500 = 570 → mean 95 while median moves only to (12+16)/2 = 14.

**S5 — 4.08.** (200 × 4.2 + 50 × 3.6)/250 = 1,020/250.

**S6 — unweighted = 3.90.** (4.2 + 3.6)/2. It **overstates** satisfaction because the smaller (and happier) group gets equal weight.

**S7 — 30%.** 1,200/4,000.

**S8 — 384 for ±5 pp; 600 for ±4 pp.** n = 1.96² × 0.25/e².

**S9 — 1,536 invitations.** 384/0.25. *(Show your over-recruitment assumption; if response is 30%, you need only 1,280.)*

**S10 — 46.3% to 57.7%.** SE = √(0.52 × 0.48/300) = 2.88 pp; MOE = 5.65 pp.

**S11 —** "If we fielded this survey repeatedly, ~95% of the intervals constructed this way would contain the true population awareness level. Our best estimate is 64%, with a margin of error of ±5.5 pp." Not a statement of certainty about this single sample, and not a statement about individual physicians.

**S12 —** Statistically significant but almost certainly **not practically significant**: 1 pp is below any actionable threshold, and with n = 25,000 tiny differences become significant. Report the effect size and its CI, and ask what decision would change at 1 pp. (Also flag multiple-comparison risk if many tests were run.)

**S13 —** Confounder: territory/physician mix (high-volume territories attract both spend and prescriptions) or the brand's historical presence. Design: a randomised controlled test — randomly assign incremental spend/targeted reps to matched territories and compare TRx changes against control territories (difference-in-differences).

**S14 — 80% (4/5).** Caveat: n = 5 triallists; the CI is enormous (±~35 pp), so the estimate is directionally suggestive, not decision-grade. Also the sample is not necessarily representative.

**S15 —** Nonresponse bias. Mitigations: shorten the survey, offer incentives, follow up with personalised reminders, recruit via a physician panel with known participation rates, compare early vs late responders, and reweight to known population characteristics (specialty mix, prescribing volume). Reporting the response rate and the limitation explicitly is mandatory.

---

## 6.9 Checklist

- [ ] I report median alongside mean when data is skewed
- [ ] I know STDEV.P vs STDEV.S and can say which applies
- [ ] I can compute a CI for a proportion and state what it does/does not mean
- [ ] I can compute required sample size for a target MOE
- [ ] I always connect p-values to effect size and business relevance
- [ ] I never claim causation from a correlation
- [ ] I state sample size, response rate and exclusions in any survey finding

**Next:** [07-business-analytics-and-market-research.md](07-business-analytics-and-market-research.md) → [08-life-sciences-industry-fundamentals.md](08-life-sciences-industry-fundamentals.md)
