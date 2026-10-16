# 11 — Technical Interview Question Bank (60-minute mock)

**Purpose:** a full question bank for the **60-minute technical interview** (CONFIRMED, current TGaS JD), covering Excel, data interpretation, analysis, statistics, case justification, quantitative reasoning, validation, survey analysis, recommendations, brain teasers, resume topics and automation.
**Prerequisites:** [04](04-excel-formulas-cheatsheet.md), [05](05-excel-practical-workbook-guide.md), [06](06-statistics-and-survey-analytics.md), [07](07-business-analytics-and-market-research.md).
**Suggested study time:** 3–4 hours. Answer **aloud** — this interview is spoken.
**Provenance:** all questions are written for practice. Topics marked **[CR]** align with **CANDIDATE-REPORTED** subjects (May 2026 report: case-study justification, graphs, Excel VLOOKUP/XLOOKUP/INDEX/SUMIFS, brain teasers, resume questions; Nov 2025 report: aptitude plus data-heavy market-entry cases with visualisations). They are **not** confirmed official questions, and the reports concern the *Analyst* role, not necessarily this campus role.
**Model answers begin at §11.2.** Attempt first.

---

## §11.1 Questions

### A. Excel and spreadsheets

| # | Level | Question |
|---|---|---|
| A1 | Easy | What is the difference between COUNT and COUNTA? Give a case where they disagree. |
| A2 | Easy | Explain the argument order of SUMIF versus SUMIFS. [CR] |
| A3 | Easy | In `=VLOOKUP(key, table, 4, FALSE)`, what does FALSE do, and what happens if you omit it? [CR] |
| A4 | Medium | Why might you prefer XLOOKUP to VLOOKUP, and what do you use if the machine has Excel 2016? [CR] |
| A5 | Medium | When is a weighted average required instead of a simple average? Give a one-line example with numbers. |
| A6 | Medium | A column of revenue is left-aligned and your SUM is short by ₹1,510 in a 30-row file. What happened, and how do you fix it? |
| A7 | Medium | Explain `$A$1`, `A$1`, `$A1` and give a formula that breaks without the dollar signs. |
| A8 | Medium | What does IFERROR hide, and when is using it a bad practice? |
| A9 | Medium | A PivotTable total does not match your SUMIFS. List the four most likely causes. |
| A10 | Medium | How do you detect duplicate rows, and how do you decide which ones to remove? |
| A11 | Hard | You need an achievement column and a count of reps below 90%. Describe the exact formulas, including why `COUNTIFS(range,">="&otherRange)` is unreliable. |
| A12 | Hard | Two files (sales and finance) differ by 3% in total revenue. Walk through your reconciliation. |

### B. Graph and data interpretation

| # | Level | Question |
|---|---|---|
| B1 | Easy | What do you check first when reading any chart? |
| B2 | Medium | A brand's share went 18% → 21% while the market grew 10%. Describe the change and estimate the brand's revenue growth. |
| B3 | Medium | What is wrong with a bar chart whose y-axis starts at 90 instead of 0? |
| B4 | Medium | When are dual-axis charts dangerous? |
| B5 | Medium | Histogram versus bar chart — what is the difference? |
| B6 | Medium | A scatter of spend vs TRx shows r = 0.94 (r² = 0.88). What can and cannot you conclude? [CR] |
| B7 | Hard | Funnel: 64% aware; 46.9% of aware consider; 70% of considerers trial; 42.9% of triallists become regular. Where do you focus and why? [CR] |
| B8 | Hard | West grew fastest (+30.8%) but South added the most revenue (+₹144 Cr). Which do you report to the CEO, and why? |

### C. Data analysis, validation and quality

| # | Level | Question |
|---|---|---|
| C1 | Easy | What checks do you run before answering any data question? |
| C2 | Medium | Blank values: when do you exclude them, when do you treat them as zero? |
| C3 | Medium | How do you validate a survey data file before analysis? |
| C4 | Medium | What is straight-lining in survey responses and why does it matter? |
| C5 | Medium | How do you handle an outlier like T6 in the spend-versus-TRx dataset? |
| C6 | Hard | Apply a data-quality framework (accuracy, completeness, consistency, timeliness, uniqueness) to the 30-row rep dataset. |
| C7 | Hard | You receive a file with no documentation. How do you build a data dictionary and prove your understanding? |
| C8 | Hard | How would you automate a weekly report and ensure the automation does not silently corrupt outputs? |

### D. Statistics and survey analysis

| # | Level | Question |
|---|---|---|
| D1 | Easy | Mean or median for "typical prescriptions per physician", and why? |
| D2 | Easy | What does a 95% confidence interval mean? |
| D3 | Medium | How many survey responses do you need for ±5 pp at 95% confidence, and for ±4 pp? |
| D4 | Medium | What is a p-value, and what is it not? |
| D5 | Medium | Difference between statistical and practical significance, with an example. |
| D6 | Medium | What is nonresponse bias, and what would you do about it? |
| D7 | Medium | Explain survey weighting with a simple example. |
| D8 | Medium | A marketing spend series correlates with prescriptions at r = 0.94. How do you test whether spend drives prescriptions? |
| D9 | Hard | You ran 20 significance tests and found 7 "significant" drivers. What is the risk and how do you handle it? |
| D10 | Hard | Design a driver-and-barrier study for a new injectable: sample, questions, analysis plan. |

### E. Quantitative reasoning

| # | Level | Question |
|---|---|---|
| E1 | Easy | Revenue rose from ₹840 Cr to ₹1,050 Cr. Percentage increase? |
| E2 | Easy | Company revenue ₹180 Cr in a ₹1,200 Cr market. Market grows 10%, the company 20%. New share? |
| E3 | Medium | 40 reps write 120 Rx each; 60 reps write 180 each. Weighted average? Why not 150? |
| E4 | Medium | A market goes from ₹1,000 Cr to ₹1,331 Cr in 3 years. CAGR? |
| E5 | Medium | Fixed cost ₹60 Cr, contribution ₹1,500 per patient. Break-even patients? |
| E6 | Hard | Price cut 10% with volume up 12%: revenue effect? What volume uplift makes a 10% cut revenue-neutral? |
| E7 | Hard | Estimate 17% of 4,980 in under 15 seconds and describe the method. |

### F. Case justification, recommendations and communication

| # | Level | Question |
|---|---|---|
| F1 | Easy | How do you structure a case answer? [CR] |
| F2 | Medium | "What would change your mind?" — how do you answer? |
| F3 | Medium | In a forecast, which assumption do you test first? |
| F4 | Medium | The client says your forecast is too optimistic. What do you say? |
| F5 | Medium | How do you prioritise between two initiatives with different payback periods? |
| F6 | Hard | Defend an assumption you had to estimate (e.g., a 40% diagnosis rate). [CR] |
| F7 | Hard | Justify a staged market entry using numbers (SAM ₹2,160 Cr; break-even 9.9% of eligible patients; planned 12% share). |
| F8 | Hard | A competitor responds to your launch with a 20% price cut. What do you do? |
| F9 | Hard | You must present a finding that contradicts the client's long-held belief. How? |

### G. Brain teasers and estimation **[CR]**

| # | Level | Question |
|---|---|---|
| G1 | Medium | A bat and ball cost ₹110 in total; the bat costs ₹100 more than the ball. What does the ball cost? |
| G2 | Medium | Eight identical pills, one is heavier. Using a balance scale, find it in the minimum number of weighings. |
| G3 | Medium | Two 60-minute ropes burn unevenly. Measure 45 minutes. |
| G4 | Hard | Estimate the number of prescriptions written in India per day. |
| G5 | Hard | Your 1-million-row dataset crashes Excel. What do you do? |

### H. Resume, tools and automation

| # | Level | Question |
|---|---|---|
| H1 | Medium | How would you automate a monthly survey report pack? [CR] |
| H2 | Medium | Excel, Python, Power BI — when do you use each in this role? |
| H3 | Medium | How do you check your own work before submitting a deliverable? [CR] |
| H4 | Medium | Walk me through a time you protected data quality in a project. [CR] |

---
---

# §11.2 Model answers

> Format: **key answer**, then the **likely follow-up**. Speak for 60–90 seconds per answer.

### A. Excel

**A1 —** COUNT counts **numeric** cells; COUNTA counts **non-empty** cells. They disagree when a cell contains text such as `"N/A"` or `"1,510"` — COUNTA counts it, COUNT does not. *Follow-up:* "Which would you use for response rate?" → COUNTA for responses submitted, COUNT for valid numeric responses.

**A2 —** `SUMIF(range, criteria, [sum_range])` puts the sum range **last**; `SUMIFS(sum_range, criteria_range1, criteria1, …)` puts it **first**. Swapping them is the most common silent error. [CR]

**A3 —** FALSE forces **exact match**. If omitted (TRUE/default), Excel assumes the first column is sorted and returns the nearest match — often a *wrong value with no error*. [CR]

**A4 —** XLOOKUP: any direction, exact match by default, no column index to maintain, built-in if-not-found, returns multiple columns. Legacy fallback: `=IFNA(INDEX(D:D,MATCH(key,A:A,0)),"Not found")` or `VLOOKUP(...,FALSE)`. [CR]

**A5 —** When group sizes differ. Example: 40 reps at 120 Rx and 60 reps at 180 Rx → weighted = 156, simple average = 150 (biased toward the smaller, lower group).

**A6 —** The value is stored as **text** (`"1,510"`), so SUM ignores it; the shortfall exactly equals that value. Fix: `=VALUE(SUBSTITUTE(A2,",",""))` or Data → Text to Columns → Finish. Detect beforehand with `=SUMPRODUCT(--NOT(ISNUMBER(G2:G31)))`.

**A7 —** `$A$1` locks column and row; `A$1` locks the row; `$A1` locks the column. A formula like `=D2/$D$9` copied down breaks (D9 → D15, empty) without the `$`.

**A8 —** It hides **all** errors (`#N/A`, `#REF!`, `#DIV/0!`, type errors). Bad when it converts a data-quality problem into a plausible-looking number. Use IFNA for lookups and always **count** the trapped errors (e.g., `=COUNTIF(range,"Not found")`).

**A9 —** (1) Stale pivot cache (refresh); (2) blanks shown as "(blank)" excluded from criteria; (3) text-vs-number mismatch in the criteria range; (4) different source ranges/rows than the SUMIFS range (e.g., totals included).

**A10 —** `=COUNTIFS($A$2:$A$31,A2,$F$2:$F$31,F2)` or `=COUNTIF(range,key)>1`, or Remove Duplicates with the right key. Decide by business meaning: exact duplicate rows → delete one; same rep in two months → keep both; near-duplicates (spelling variants) → standardise, not delete.

**A11 —** Helper column `=IF(OR(TRx="",Target=""),"",TRx/Target)`; then `=COUNTIF(Ach,"<0.9")` and `=COUNTIF(Ach,">=1")`. `COUNTIFS(range,">="&otherRange)` is unreliable because COUNTIF criteria compare to **single values**, not arrays — the safe pattern is a computed helper column.

**A12 —** Steps: (1) totals by month/region/product on both sides; (2) reconcile at the most granular common dimension (find the first level where they agree/diverge); (3) classify the variance — timing (cut-off differences), returns/credits, missing records, duplicates, rounding; (4) quantify each cause so the residual is ~0; (5) document the reconciliation and which system is authoritative. *Key line:* "I would reconcile bottom-up, not argue about the 3% top-level gap."

### B. Charts

**B1 —** Units, the base (share of what?), axis start and scale, time period, and whether the series are comparable. [CR]

**B2 —** Share rose **3 percentage points** — i.e. 3/18 = **+16.7% relative**. If the market grew 10%, brand revenue grew 1.10 × 1.1667 − 1 = **+28.3%**.

**B3 —** It exaggerates small differences (a 2 pp change can look like a doubling). Bars must start at 0; if truncation is necessary, label it and annotate the actual change.

**B4 —** Two axes can be rescaled to manufacture co-movement; it implies relationships that are pure artifacts. Use them only when units genuinely differ and you state both scales — or use indexed series (base 100).

**B5 —** A histogram bins **continuous** data to show distribution (no gaps, ordered bins); a bar chart compares **categories** (gaps meaningful).

**B6 —** Can conclude: strong linear association in this sample; roughly 88% of TRx variance is linearly associated with spend. Cannot conclude: causation, or that spend *explains* TRx. T6 is an above-trend outlier worth investigating. [CR]

**B7 —** 0.64 × 0.46875 × 0.70 × 0.4286 = **0.09 → 9% of physicians become regular prescribers**; the largest *proportional* leak is trial → regular (57.1% drop-off), the largest *absolute* leak is awareness → consideration (340 of 1,000). Focus: first diagnose whether the consideration leak is access/cost (usually) versus evidence; then fix the trial → regular leak (side effects, cost, follow-up). Say both, and name which you would test first. [CR]

**B8 —** Report **both**, framed as a dynamic: "South is the largest contributor (+₹144 Cr) and West the fastest grower (+30.8%) — West deserves the next investment increment, South funds the plan today." Never let a CEO hear only one.

### C. Data quality

**C1 —** Units and definitions; total reconciliation; blanks; duplicates; text-vs-number; date coverage; and whether the sample/base matches the question.

**C2 —** Exclude when the blank means "unknown" (and count it, to report completeness); treat as zero only when absence genuinely means none (e.g., no prescriptions written that month). Document the choice — this is a decision, not a detail.

**C3 —** Range checks (Likert 1–5; no 7s or "N/A" in numerics), completeness by question, deduplicate IDs, check completion time (too fast = careless), straight-lining, inconsistent cross-tab responses, and whether quotas were met.

**C4 —** Answering identically down a matrix (e.g., all 5s or all 3s) — often indicates careless or insincere response. Detect with variance across items; consider exclusion or flagging, and report how many were affected.

**C5 —** Investigate first: is it a data error or a real high performer? Check physician mix, KOL presence, recent campaigns. Report results with and without it; never delete silently. Use a **pilot** to test whether the mechanism is reproducible before reallocating spend.

**C6 —** **Accuracy:** verify text-stored numbers and blanks (found 1 of each). **Completeness:** 29 of 30 usable rows, 27 with both TRx and target (90%). **Consistency:** region variants (`north `, `N`) and product name with a leading space. **Timeliness:** two months present (Jan/Feb-26) — is that the expected window? **Uniqueness:** 1 exact duplicate. Report each with a count.

**C7 —** Inspect first 20 and last 20 rows; profile each column (type, blanks, distinct values, range, min/max); check date ranges; compare totals against any known number; write a data dictionary (column, definition, type, allowed values, source); then confirm definitions with the requester in one message.

**C8 —** Build with Power Query (ETL) → structured sheet → PivotTable/template; add **validation gates** that fail loudly (row count vs expected ±5%; total TRx reconciliation; duplicate and blank counts; lookup error count = 0). Version the file and log changes. If Python: pandas pipeline with `assert` checks on row counts, dtypes and totals, and a run log.

### D. Statistics

**D1 —** Median: prescriptions per physician are right-skewed, so a few high writers drag the mean above the typical physician. Report mean **and** median, and state which represents "typical".

**D2 —** If the survey were repeated many times, ~95% of intervals constructed this way would contain the true population value. It is not "a 95% probability that the true value is in this interval" and says nothing about individuals.

**D3 —** **384** for ±5 pp, **600** for ±4 pp (95% confidence, p = 0.5 worst case). Adjust upward for the expected response rate: 384 completes at a 25% response rate → 1,536 invitations.

**D4 —** The probability of seeing data at least as extreme as yours if H₀ (no effect) were true. Not: the probability that H₀ is true; not the effect size; not proof of an effect.

**D5 —** Example: with n = 25,000, a 1 pp difference in awareness can be statistically significant yet commercially irrelevant. Report effect size, CI and decision relevance — not just p < 0.05.

**D6 —** Responders differ systematically from non-responders (busy high prescribers don't answer). Mitigate with shorter surveys, incentives, reminders, early-vs-late responder comparison, and weighting to known population characteristics; always report the response rate.

**D7 —** If academics are 30% of the population but 50% of your sample, weight them down: weight = population share ÷ sample share. Worked example: unweighted 4.00 vs weighted 0.30×4.5 + 0.70×3.5 = **3.80**.

**D8 —** Randomised test: assign incremental spend to matched territories, hold controls, and compare TRx changes (difference-in-differences); check lag structure and confounders (physician mix, competitor activity). A correlation with 10 territories cannot separate "spend drives volume" from "volume attracts spend".

**D9 —** Multiple-comparison risk: ~1 false positive expected per 20 tests at α = 0.05. Confirm findings in an independent sample or split-sample, apply a correction (e.g., Bonferroni: α = 0.05/20 = 0.0025), and prioritise by effect size and business meaning.

**D10 —** Sample: n ≈ 400 complete responses for ±5 pp, fielded to a representative specialist list (stratified by specialty and volume decile), with quotas and a response-rate target (~20–30%). Questions: aided/unaided awareness, current prescribing behaviour (patients/month), trial, drivers (importance battery), barriers (reason-not-to-prescribe), access questions, and a behavioural intent question. Analysis: distributions with CIs, segment cross-tabs, driver prioritisation by importance × current performance, and segment-level sample sizes reported.

### E. Quantitative

**E1 —** **+25%** (210/840).

**E2 —** **16.4%** (180 × 1.2 = 216 ÷ 1,320). Increased because the company outgrew the market.

**E3 —** **156** ((40×120 + 60×180)/100). The simple 150 under-weights the larger group of 180-writers — a weighted average is mandatory when group sizes differ.

**E4 —** **10%** ((1,331/1,000)^(1/3) − 1).

**E5 —** **4,00,000 patients** (6 × 10⁸ ÷ 1,500).

**E6 —** Revenue multiplier = 0.9 × 1.12 = 1.008 → **+0.8%**. For revenue-neutrality, volume must rise by 1/0.9 − 1 = **11.1%**.

**E7 —** **≈847**: 17% of 5,000 = 850, minus 17% of 20 = 3.4. Method: round the base, adjust the remainder, and sanity-check the direction.

### F. Cases and communication

**F1 —** Objective → framework → analysis → key findings → recommendation → risks → next steps. [CR]

**F2 —** Name the specific data or result that would flip the conclusion, and how you would get it: "If the payer study shows <30% of covered lives with feasible access, I would not recommend the full launch — I'd recommend the pilot."

**F3 —** The assumption with the highest **impact × uncertainty** — usually access/coverage, diagnosis rate or persistence. State why, and test it with a range rather than a point.

**F4 —** Do not defend reflexively. Offer the bear case, quantify it, state the triggers, and propose a staged decision: "If Y1 adoption is 1.5% instead of 3%, break-even moves to Year 2 — the plan survives, so I'd proceed but hold the second capital tranche until Q2 access metrics are met."

**F5 —** Compare on payback, NPV/ROI, strategic value (access/data/capability), and risk. If payback is equal but one builds a reusable capability, that breaks the tie; if not, prefer the faster payback unless capital is cheap.

**F6 —** State the number, its source (or "estimate"), why it matters (sensitivity), and how you would validate it (claims data, KOL interviews, pilot). Quantify the impact: "A 10 pp error in the diagnosis rate moves SAM by ₹540 Cr." [CR]

**F7 —** SAM ₹2,160 Cr; contribution ₹84,000 per patient; break-even 17,857 patients = **9.9% of eligible**, versus a planned 12% share → thin (2.1 pp) margin of safety in a market where the top two competitors hold 75%. Hence: stage the entry, gate scale-up on ≥50% commercial lives with feasible access, and re-price or re-scope if unmet. [CR]

**F8 —** Quantify first (a 20% cut takes their net price to $6,240 vs your $9,600 — a 54% premium you must justify on value, not price). Avoid matching: run the break-even maths (a 20% cut needs >25% volume uplift merely to hold revenue), then respond with access contracts, patient-support programs, evidence and targeted defence of high-value accounts; consider a value-based contract rather than a list-price war.

**F9 —** Lead with the decision impact, then the evidence, then the reconciliation: "The data contradicts our assumption — here is what it means for this quarter's plan." Acknowledge their prior, present the data with its limitations, propose a test to resolve the conflict rather than demanding belief.

### G. Brain teasers

**G1 —** **₹5.** If the bat is ₹100 more and the total is ₹110: ball + (ball + 100) = 110 → 2 × ball = 10. The intuitive answer ₹10 fails the check (bat would be ₹100, total ₹110 gives ball ₹10 — but then the bat is only ₹90 more). [CR]

**G2 —** **2 weighings.** Weigh 3 vs 3: if equal, the heavy pill is in the remaining 2 (weigh them); if not, take the heavier trio and weigh 1 vs 1 (if equal, the third is heavy).

**G3 —** Light rope A at both ends and rope B at one end simultaneously. When A burns out (30 min), light B's other end; B now has 30 minutes of burn remaining but two flames → finishes in 15 more minutes = **45 minutes**.

**G4 —** Structure: population 1.4B → ~60% seek allopathic care (~840M) → ~20% have a chronic condition needing regular therapy (~168M) → average ~4 prescriptions/person-year (mix of chronic monthly refills and acute courses) → ~672M prescriptions/year → ~**1.8M prescriptions/day**. State assumptions, show the chain, sanity-check against pharmacy counts. The structure is what is being assessed, not the exact number. [CR]

**G5 —** Move to Power Query/Python (pandas) or load via the Data Model/Power Pivot; work with a filtered subset for exploration; or aggregate at source. Never sort/filter the full sheet interactively. Deliver the aggregate and document the method.

### H. Resume, tools and automation

**H1 —** Power Query (ingest → clean → type) → pivot/model → templated sheet → export; or Python (pandas) with assertion checks; schedule it; add validation gates and a change log. Mention the manual checks you keep even when automated (row counts, totals, spot checks). [CR]

**H2 —** Excel for analysis, pivots and the client-facing workbook; Python for repeatable cleaning/automation and large files; Power BI/Tableau for live, shareable dashboards. In this role Excel + Power BI matter most (per the JD's mention of dashboards); Python is the differentiator for process improvement.

**H3 —** Recompute totals independently, cross-foot parts vs whole, verify lookups returned no errors, spot-check 5 rows manually, re-read the question to confirm I answered what was asked, and get a second pair of eyes on client-facing numbers. [CR]

**H4 —** Use your own project with placeholders: "[In PROJECT X] I validated [input dataset] with [specific check], found [specific defect], fixed it with [method], and added [check] so it could not recur." (See [13](13-resume-and-project-discussion.md) for how to build this from real work.) [CR]

---

## §11.3 Follow-up drill list

Pick any five answers above and answer the follow-up **without notes**. If you cannot, that topic goes into your next revision block.

- Which answers contained a number? Those are the ones interviewers remember.
- Which answers named a limitation or risk? Those signal analyst maturity.
- Which answers took longer than 90 seconds? Trim them.

**Next:** [13-resume-and-project-discussion.md](13-resume-and-project-discussion.md) → [12-behavioral-interview-and-hr.md](12-behavioral-interview-and-hr.md)
