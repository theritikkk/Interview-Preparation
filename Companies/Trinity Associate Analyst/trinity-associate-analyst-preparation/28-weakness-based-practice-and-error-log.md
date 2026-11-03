# 28 — Weakness-Based Practice and Error Log

**Purpose:** a systematic framework for tracking, categorising, and eliminating your mistakes across all preparation areas. File [19](19-errors-mistakes-and-traps.md) names the traps — this file gives you the system to track YOUR specific errors and turn them into targeted practice.
**Prerequisites:** [19-errors-mistakes-and-traps.md](19-errors-mistakes-and-traps.md).
**Suggested study time:** 30 minutes to set up; then 10 minutes after every practice session.
**Provenance:** all examples are **PRACTICE INFERENCE**.

---

## 28.1 Why track mistakes systematically?

Most candidates make the same 5–7 errors repeatedly. Without a log, you practise randomly. With a log, you practise the exact skill that failed — and you can measure whether it's improving.

**The loop:** Practise → Record mistake → Categorise → Understand root cause → Targeted drill → Re-test → Confirm fixed.

---

## 28.2 Mistake categories

### Category A — Calculation errors

| Code | Mistake type | Example | Root cause |
|---|---|---|---|
| A1 | Wrong denominator | Used new value instead of old for % change | Habit: not saying denominator aloud |
| A2 | Arithmetic slip | 1,250 × 0.85 = 1,025 (correct: 1,062.5) | Speed over accuracy; skipping verification |
| A3 | Unit confusion | Mixed ₹ Cr and ₹ Lakhs in same calculation | Not labelling units on every line |
| A4 | Rounding too early | Rounded 16.36% to 16% mid-calculation, final answer off by 0.5 pp | Keep full precision until the final answer |
| A5 | CAGR vs simple growth | Used (end−start)/start instead of the nth-root formula | Forgot that CAGR uses intervals, not endpoints |

### Category B — Percentage and share errors

| Code | Mistake type | Example | Root cause |
|---|---|---|---|
| B1 | % vs percentage points | "Share grew 3%" when it grew 3 pp (from 12% to 15%) | Not checking if the base is already a percentage |
| B2 | Share direction wrong | "Brand grew 5% so share must have grown" — but market grew 8% | Forgot share = brand growth / market growth |
| B3 | Simple average of percentages | Averaged 80% and 120% to get 100%, ignoring group sizes | Need weighted average when groups differ |

### Category C — Data interpretation errors

| Code | Mistake type | Example | Root cause |
|---|---|---|---|
| C1 | Misreading axis | Assumed Y-axis starts at 0 when it starts at 500 | Didn't check axis labels first |
| C2 | Confusing absolute and relative | "North is the fastest growing" when it's the largest in absolute terms | Report both: largest contributor AND fastest grower |
| C3 | Correlation as causation | "Higher spend → higher TRx, so spend more" | No controlled test; confounders not considered |
| C4 | Missing the base | Compared two percentages without checking sample sizes | Always ask "percentage of what?" |

### Category D — Problem structuring errors

| Code | Mistake type | Example | Root cause |
|---|---|---|---|
| D1 | Jumping to conclusion | "Revenue is down → cut costs" without diagnosing | No structure; didn't break problem into components |
| D2 | Not MECE | Analysis covers price and promotion but misses access | Didn't use a complete framework |
| D3 | Wrong KPI | Tracked revenue when the question was about profitability | Didn't clarify the objective first |
| D4 | Ignoring constraints | Recommended hiring 50 reps without checking budget | Didn't ask about constraints |

### Category E — Root cause analysis failures

| Code | Mistake type | Example | Root cause |
|---|---|---|---|
| E1 | Stopped at symptom | "Sales are down in East" — didn't ask why | Need to go at least 3 levels deep |
| E2 | Ignored confounders | Attributed TRx decline to fewer calls, but competitor launched | Didn't consider external factors |
| E3 | Confirmation bias | Only looked at data supporting hypothesis | Need to actively seek disconfirming evidence |

### Category F — Recommendation weaknesses

| Code | Mistake type | Example | Root cause |
|---|---|---|---|
| F1 | No evidence cited | "I recommend expanding the sales force" — no numbers | Every recommendation needs a supporting calculation |
| F2 | No risks mentioned | Gave a recommendation without limitations | Template: recommendation + evidence + impact + risk |
| F3 | Vague action | "Improve coverage" instead of "Add 8 reps to East targeting top-decile cardiologists" | Be specific: who, what, where, when, how much |
| F4 | No success metric | Didn't define how to measure if the recommendation worked | Always include: "Measure success by [KPI] reaching [target] by [date]" |

### Category G — Communication errors

| Code | Mistake type | Example | Root cause |
|---|---|---|---|
| G1 | No structure in verbal answer | Rambling for 3 minutes without a clear point | Use: "My recommendation is X, for three reasons: 1, 2, 3" |
| G2 | Numbers without context | "TRx is 1,250" — is that good or bad? | Always compare: vs target, vs last period, vs peers |
| G3 | Jargon without explanation | Used "T2B" without defining it | Define acronyms on first use |
| G4 | Exceeded time | 5-minute answer to a 1-minute question | Practise timed responses |

### Category H — Excel formula errors

| Code | Mistake type | Example | Root cause |
|---|---|---|---|
| H1 | SUMIFS argument order | Put criteria range first instead of sum range | SUMIFS = sum range FIRST; SUMIF = sum range LAST |
| H2 | Missing absolute reference | Copied VLOOKUP down without \$A\$1:\$D\$10 | Lock the lookup array with \$ |
| H3 | VLOOKUP without FALSE/0 | Got wrong match because approximate match was used | Always use FALSE (exact match) unless deliberately doing range lookup |
| H4 | Text-number mismatch | SUMIFS returned 0 because IDs were stored as text | Check with ISNUMBER(); fix with VALUE() |

### Category I — SQL logic errors

| Code | Mistake type | Example | Root cause |
|---|---|---|---|
| I1 | GROUP BY missing column | Selected Region and Product but only grouped by Region | Every non-aggregate SELECT column must be in GROUP BY |
| I2 | WHERE vs HAVING | Filtered aggregated results with WHERE instead of HAVING | WHERE filters rows before aggregation; HAVING filters after |
| I3 | JOIN producing duplicates | Inner join on non-unique key multiplied rows | Check cardinality before joining |
| I4 | NULL handling | COUNT(column) excluded NULLs when total was needed | Use COUNT(*) for row counts; COALESCE for NULL replacement |

### Category J — Statistical interpretation errors

| Code | Mistake type | Example | Root cause |
|---|---|---|---|
| J1 | p-value misinterpretation | "p = 0.03 means 3% chance the null is true" | p-value = probability of data given null, not probability of null |
| J2 | CI misinterpretation | "95% chance the true value is in this interval" | The interval either contains it or doesn't; 95% is the method's long-run rate |
| J3 | Small sample generalisation | Drew conclusion from n = 5 | State sample size and confidence interval width |
| J4 | Multiple comparisons | Ran 20 tests, found 1 significant at p < 0.05 | Expected 1 false positive in 20 tests; apply Bonferroni or state the limitation |

### Category K — Time management errors

| Code | Mistake type | Example | Root cause |
|---|---|---|---|
| K1 | Spent too long on one question | Used 8 minutes on a 2-minute question | Set per-question time budgets; mark and move on |
| K2 | Didn't attempt all questions | Left 5 questions blank in a 30-question test | Do easy questions first; guess rather than leave blank |
| K3 | Perfectionism on cleaning | Spent 20 of 45 minutes cleaning when 10 would suffice | Set a cleaning time limit; good enough is enough |

---

## 28.3 The mistake log template

Copy this table into a separate document or spreadsheet. Fill it after every practice session.

| # | Date | Source | Question / Task | What I did (wrong) | Category code | What I should have done | Why I made the error | Prevention habit | Similar practice Q | Retry date | Fixed? |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | | | | | | | | | | | |
| 2 | | | | | | | | | | | |
| 3 | | | | | | | | | | | |
| 4 | | | | | | | | | | | |
| 5 | | | | | | | | | | | |

### How to fill each column

- **Source:** file number and question number (e.g., "14-Q6" or "21-Case3-Q2")
- **What I did:** the exact wrong step, not "I got it wrong"
- **Category code:** use the codes above (A1, B2, C3, etc.)
- **Why I made the error:** concept gap, carelessness, time pressure, misread, or habit
- **Prevention habit:** one specific action (e.g., "say denominator aloud before dividing")
- **Similar practice Q:** find one similar question from the repo and note its location
- **Retry date:** schedule 2–3 days later
- **Fixed?:** only "Yes" if you solve the retry correctly without looking at the answer

---

## 28.4 Worked examples of logged mistakes

### Example 1

| Field | Entry |
|---|---|
| Date | 3 Nov 2026 |
| Source | 14-Q1 |
| Question | "Drug's monthly sales grew from ₹840L to ₹1,050L. What is the % change?" |
| What I did | Divided 210 by 1,050 → got 20% |
| Category | A1 (wrong denominator) |
| Should have done | 210 / 840 = 25% — divide by the OLD value |
| Why | Autopilot; grabbed the larger number |
| Prevention | Say aloud: "change divided by the STARTING number" before writing |
| Similar Q | 01-Q1, 19-Trap1 |
| Retry date | 5 Nov 2026 |
| Fixed? | |

### Example 2

| Field | Entry |
|---|---|
| Date | 4 Nov 2026 |
| Source | 21-Case4-Q2 |
| Question | "Brand X's sales grew 5%. Market grew 8%. Did share increase or decrease?" |
| What I did | Said "share increased because sales grew" |
| Category | B2 (share direction wrong) |
| Should have done | Share fell: brand growth (5%) < market growth (8%). New share = old share × (1.05/1.08) |
| Why | Confused absolute growth with relative share |
| Prevention | Share ↑ only if brand growth > market growth. Always compute the ratio. |
| Similar Q | 01-Q50, 03-C1, 19-Trap2 |
| Retry date | 6 Nov 2026 |
| Fixed? | |

### Example 3

| Field | Entry |
|---|---|
| Date | 5 Nov 2026 |
| Source | 15-Task2 |
| Question | "Calculate total TRx by region using SUMIFS" |
| What I did | SUMIFS(C2:C30, D2:D30, "North", B2:B30) — wrong argument order |
| Category | H1 (SUMIFS argument order) |
| Should have done | SUMIFS(B2:B30, D2:D30, "North") — sum range FIRST |
| Why | Mixed up SUMIF order (sum range last) with SUMIFS order (sum range first) |
| Prevention | SUMIFS: "Sum first, then criteria pairs." Write it on a sticky note. |
| Similar Q | 04-§4.3, 19-Trap9 |
| Retry date | 7 Nov 2026 |
| Fixed? | |

---

## 28.5 Weekly review protocol

Every Sunday (or your chosen review day), spend 20 minutes on this:

### Step 1: Frequency analysis (5 min)

Count how many times each category code appears in the past week.

| Category | Count this week | Trend (↑↓→) |
|---|---|---|
| A1 | | |
| A2 | | |
| B1 | | |
| ... | | |

**Rule:** the category that appears ≥ 3 times is your #1 revision priority for next week.

### Step 2: Concept vs careless classification (5 min)

For each mistake, classify:
- **Concept error:** I didn't understand the method → needs study
- **Careless error:** I knew the method but slipped → needs a checklist/habit
- **Time error:** I knew the method but ran out of time → needs speed drills

| Error type | Count | Action |
|---|---|---|
| Concept | | Study the source file section |
| Careless | | Add a prevention step to your checklist |
| Time | | Do 5 timed questions on that topic |

### Step 3: Retry check (5 min)

Look at all entries where the retry date has passed. Have you retried them?
- If not: do them now.
- If retried and correct: mark "Fixed? = Yes"
- If retried and wrong: the error is deeper than you thought. Schedule a full study session on that topic.

### Step 4: Update prevention checklist (5 min)

Add any new prevention habits to your personal checklist (see §28.6). Remove habits that have become automatic (no errors in 2+ weeks).

---

## 28.6 Personal prevention checklist template

Print or copy this and review before every timed exercise:

**Before starting any calculation:**
- [ ] What is the denominator? Say it aloud.
- [ ] Are the units consistent? Label every line.
- [ ] Is the base already a percentage? If yes, use percentage points.
- [ ] Am I using weighted or simple average? Check group sizes.

**Before submitting Excel work:**
- [ ] Do SUMIFS totals match the grand total?
- [ ] Are lookup failure counts zero?
- [ ] Do all percentages add to 100% (or close)?
- [ ] Have I checked for text-stored numbers?

**Before presenting a recommendation:**
- [ ] Do I have evidence (a number) supporting this?
- [ ] Have I named at least one risk?
- [ ] Have I defined a success metric?
- [ ] Is my recommendation specific (who, what, when, how much)?

**Before submitting a timed assessment:**
- [ ] Have I answered every question (no blanks)?
- [ ] Have I double-checked the 3 hardest questions?
- [ ] Did I reconcile parts with the whole?

---

## 28.7 Milestone tracker

Track your overall error rate over time:

| Week | Total practice Qs attempted | Total errors | Error rate | Top 2 categories | Improvement from last week |
|---|---|---|---|---|---|
| Week 1 | | | | | — |
| Week 2 | | | | | |
| Week 3 | | | | | |
| Week 4 | | | | | |

**Target:** reduce error rate by 30–50% over 4 weeks of consistent practice.

**Readiness signal:** when your top error categories have been "Fixed" for 2 consecutive weeks and your error rate on timed mocks is below 15%.

---

## 28.8 Category-specific practice drills

When a category appears ≥ 3 times, do the targeted drill below:

| Category | Drill | Source |
|---|---|---|
| A1–A5 (Calculation) | Redo 10 questions from [01](01-quantitative-aptitude.md) with a 60-second timer per question | 01, 14 |
| B1–B3 (% and share) | Rework datasets C and H from [03](03-data-interpretation-and-graphs.md); state pp vs % for every answer | 03, 19 |
| C1–C4 (Data interpretation) | Read 5 datasets from [03](03-data-interpretation-and-graphs.md) using the 60-second chart-reading routine | 03 |
| D1–D4 (Problem structuring) | Take 3 cases from [21](21-case-study-problem-solving-masterclass.md) and write only the structure (no calculations) in 3 minutes each | 21 |
| E1–E3 (Root cause) | For 3 cases, write the Five Whys chain before looking at the answer key | 21 |
| F1–F4 (Recommendations) | Take 5 findings from any exercise and convert each into: recommendation + evidence + impact + risk + KPI | 10, 21 |
| G1–G4 (Communication) | Record yourself answering 3 case questions; listen back; time each; cut to 90 seconds | 12, 21 |
| H1–H4 (Excel) | Redo the Excel mock [15](15-mock-excel-assessment-45-min.md) from scratch with a 35-minute time limit | 15 |
| I1–I4 (SQL) | Redo 10 SQL questions from [23](23-sql-for-business-and-survey-analytics.md) focusing on the error category | 23 |
| J1–J4 (Statistics) | Redo questions S6–S15 from [06](06-statistics-and-survey-analytics.md); write the interpretation before the number | 06 |
| K1–K3 (Time management) | Do the mock assessment [14](14-mock-proctored-assessment-60-min.md) again with strict per-section time limits | 14 |

---

## 28.9 The "three strikes" rule

If the same specific mistake (not just category, but the exact same error pattern) appears three times:

1. **Stop practising new material.**
2. **Study the underlying concept** from the source file for 30 minutes.
3. **Create 3 custom practice questions** that target exactly that error.
4. **Solve all 3 correctly** before resuming other practice.
5. **Retest in 3 days and 7 days.**

This is uncomfortable but effective. The mistake that repeats is the one that will cost you in the actual assessment.

---

## 28.10 Checklist

- [ ] I have a mistake log document/spreadsheet set up
- [ ] I fill it after every practice session (not the next day — immediately)
- [ ] I do the weekly review every Sunday
- [ ] I know my top 3 error categories this week
- [ ] I have scheduled targeted drills for those categories
- [ ] My prevention checklist is updated and I review it before every timed exercise

**Next:** [29-integrated-study-roadmap.md](29-integrated-study-roadmap.md)
