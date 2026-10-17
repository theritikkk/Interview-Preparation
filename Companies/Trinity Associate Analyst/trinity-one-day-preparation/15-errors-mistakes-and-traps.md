# 15 — Errors, Mistakes & Traps

> **Learning objectives:** know the 20 mistakes that actually cost marks in assessments and credibility in interviews — each with an example, the correction, and a prevention habit.
> **Estimated study time:** 45 min; then re-read only the prevention checklist the night before the assessment.
> **Priority:** **MUST KNOW.**
> **How to use:** after every mock, write the **trap number** next to each wrong answer. If one trap appears three times, that is your revision priority.

---

## Trap 1 — Wrong denominator for percentage change

**Example:** Revenue goes 840 → 1,050. "It grew 210/1,050 = 20%."
**Correction:** Divide by the **starting** value: 210/840 = **+25%**. The new value is the base only when the question says "as a share of current revenue".
**Prevention:** Say the denominator out loud before writing: "change divided by the *old* number".
**Where it appears:** assessments ([`02` §2.1](02-aptitude-reasoning-and-data-interpretation.md); Mock C Q1), share calculations, growth-versus-target conversations.

---

## Trap 2 — Percentage change vs percentage points

**Example:** "Awareness increased 3% from 28% to 35%."
**Correction:** 28% → 35% is **+7 percentage points**, which is **+25% relative**. Both statements are true; they answer different questions.
**Prevention:** If the number is already a percent (share, awareness, penetration, response rate), start with **pp**; use "%" only for the relative version, and label it.
**Where it appears:** every share-based dataset ([`02` §2.9](02-aptitude-reasoning-and-data-interpretation.md)); slide titles; [`11` §11.2](11-final-revision-cheatsheet.md).

---

## Trap 3 — Simple average when a weighted average is required

**Example:** 40 reps write 120 Rx and 60 reps write 180 Rx → "average 150".
**Correction:** The group sizes differ, so the weighted average is (40×120 + 60×180)/100 = **156**. The unweighted 150 over-weights the smaller group.
**Prevention:** Before averaging, ask: "are the groups the same size?" If not, weight. Same rule for portfolio growth, satisfaction scores, and segment awareness.
**Where it appears:** [`02` §2.3](02-aptitude-reasoning-and-data-interpretation.md), [`02` §2.8](02-aptitude-reasoning-and-data-interpretation.md), weighted survey results ([`04` §4.9](04-statistics-survey-and-market-research.md)).

---

## Trap 4 — Confusing revenue with profit (and margin types)

**Example:** "Revenue grew 20%, so profit grew 20%." Or: "Margin is 60%" when the question asked about operating margin.
**Correction:** Revenue − COGS = gross profit; after SG&A and R&D you get operating profit. Revenue 500, COGS 200, SG&A 150 → gross margin **60%**, operating margin **30%**.
**Prevention:** Always name the margin: gross, operating or net. Never assume a revenue increase converts to profit at the same rate.
**Where it appears:** [`02` §2.5](02-aptitude-reasoning-and-data-interpretation.md), Mock C Q5, business-reasoning questions.

---

## Trap 5 — Ignoring units (and mixing volume with value)

**Example:** Adding ₹ crore to $ millions; comparing TRx growth with revenue growth; averaging a share with a count.
**Correction:** Convert to one unit before any operation; state units in every intermediate line. Volume shares and value shares routinely diverge — a brand with 38% of prescriptions held only 11% of revenue in the [`16`](16-case-practice-workbook.md) case.
**Prevention:** Write units at the top of your working; if two numbers are in different units, do not touch them until converted.
**Where it appears:** every case ([`16` §16.3](16-case-practice-workbook.md) Q4 is the classic version); DI tables; client deliverables.

---

## Trap 6 — Causal conclusions from correlations

**Example:** "Marketing spend drives prescriptions — correlation is 0.94."
**Correction:** r = 0.94 (r² ≈ 0.88) describes a strong linear association in the sample. Causation requires temporal order, mechanism, confounding control and ideally a randomised or quasi-experimental test. High-volume physicians may attract spend rather than respond to it.
**Prevention:** Use association language ("is associated with"), name one confounder, and propose a test (matched control territories, difference-in-differences).
**Where it appears:** [`04` §4.8](04-statistics-survey-and-market-research.md), [`02` §2.10](02-aptitude-reasoning-and-data-interpretation.md).

---

## Trap 7 — Misreading chart axes and bases

**Example:** Concluding "share fell 5%" from a chart whose axis starts at 90 and whose labels use a different base year.
**Correction:** Check axis start, scale, interval spacing and the base of every percentage. A truncated axis exaggerates a 2 pp change into a visual collapse.
**Prevention:** Before interpreting: units → base → axis start → time spacing (4 steps, 10 seconds).
**Where it appears:** [`02` §2.8](02-aptitude-reasoning-and-data-interpretation.md); any client deck.

---

## Trap 8 — Choosing (or trusting) a misleading chart

**Example:** A dual-axis chart "proving" that spend and TRx move together; a 6-slice pie; a 3-D bar chart.
**Correction:** Match chart to question — bar for comparison, line for time, histogram for distribution, scatter for relationships, funnel for conversion. Dual axes are rescaled at will; prefer indexed series.
**Prevention:** Ask "what is the one message?" before choosing; annotate the point you are making.
**Where it appears:** [`05` §5.6](05-power-bi-tableau-and-data-visualization.md), [`08` §8.3](08-powerpoint-and-business-communication.md).

---

## Trap 9 — Incorrect lookup ranges

**Example:** `=VLOOKUP(key, table, 4)` returns a plausible but wrong value; `=SUMIFS` arguments in `SUMIF` order; a table range that shifts when a column is inserted.
**Correction:** Always `FALSE`/`0` for exact match; remember **SUMIFS puts the sum range first**; use absolute references (`$A$2:$E$31`); prefer XLOOKUP or INDEX-MATCH where available.
**Prevention:** Test on a known row ("does this key return the value I already know?"), and count failures with `IFNA` + `COUNTIF`.
**Where it appears:** [`03` §3.4](03-excel-complete-revision.md), Mock B ([`10`](10-mock-assessment-and-excel-test.md)) Part 4.

---

## Trap 10 — Forgetting absolute references

**Example:** `=D2/D9` copied down produces `#DIV/0!` because D9 became D15.
**Correction:** `=D2/$D$9`. Lock whichever part of the address must not move; use F4/Cmd+T to cycle.
**Prevention:** After writing any formula you will copy, ask "which address must stay fixed?" — and test two cells before filling.
**Where it appears:** every Excel assessment ([`03` §3.5](03-excel-complete-revision.md)); share and %-of-total columns.

---

## Trap 11 — Mishandling blanks and duplicates

**Example:** `SUM` silently ignores a text-stored `"1,510"` (short by exactly that amount); Remove Duplicates with the wrong key deletes legitimate monthly rows; a blank target becomes a fake 100%.
**Correction:** Decide explicitly whether a blank means "zero" or "unknown" and **count** it (`COUNTBLANK`). Delete exact duplicates only, or use RepID+Month. Detect text-numbers with `=SUMPRODUCT(--NOT(ISNUMBER(range)))`.
**Prevention:** Run the reconciliation checklist before every analysis: row count, total audit, duplicate count, blank count, text-number count, lookup error count.
**Where it appears:** [`03` §3.10](03-excel-complete-revision.md), Mock B ([`10`](10-mock-assessment-and-excel-test.md)) Part 1.

---

## Trap 12 — Overstating what survey data proves

**Example:** "52% of physicians will prescribe the product." (It is 52% *stated intent*, with a ±5.7 pp margin of error, from a specific sample.)
**Correction:** State the base, the sample size, the margin of error, and the behavioural limitation. Anchor forecasts on behaviour ("4.1 starts per physician last month"), not on intent.
**Prevention template:** "[X%] of [n = N] [population] report [measure] (±MOE). This is stated intent; the behavioural proxy is [Y]. Forecasts use [Z] discount."
**Where it appears:** [`04` §4.6](04-statistics-survey-and-market-research.md), [`16` §16.3](16-case-practice-workbook.md) Q7–Q8.

---

## Trap 13 — Recommendation without risks or conditions

**Example:** "Launch nationally because the market is large."
**Correction:** Recommendation + the assumption it depends on + the gate that would change your mind. Example: "Stage the launch; break-even mid-Year 3; gate scale-up on ≥50% of covered lives with feasible access."
**Prevention:** Never end an answer with a recommendation; always add "the main risk is…" and "the next step is…".
**Where it appears:** the case framework ([`07` §7.3](07-life-sciences-business-analytics-and-cases.md)), [`16` §16.3](16-case-practice-workbook.md) Q12.

---

## Trap 14 — Unstructured case answer

**Example:** Jumping into arithmetic, then drifting between issues, ending with "so, yeah, it depends".
**Correction:** Objective → framework → analysis → findings → recommendation → risks → next steps. State the structure *before* calculating, and signpost each move ("let me quantify that, then interpret").
**Prevention:** Keep a one-line skeleton on paper; tick items as you complete them.
**Where it appears:** case and technical interviews ([`07` §7.3](07-life-sciences-business-analytics-and-cases.md)).

---

## Trap 15 — Rounding too early (the invisible error)

**Example:** Rounding 9.34% to 9% mid-calculation, then compounding it across three years → a materially wrong answer.
**Correction:** Keep full precision through the chain; round only the final reported number, and state the rounding convention (1 dp / 2 dp).
**Prevention:** Use Excel cells (not mental arithmetic) for chains; if estimating, say "approximately" and re-check with one exact calculation.
**Where it appears:** CAGRs ([`02` §2.4](02-aptitude-reasoning-and-data-interpretation.md)), weighted averages, funnel chains.

---

## Trap 16 — Mixing CAGR with simple growth

**Example:** "The market grew 20.75% in two years, so the annual growth is 10.4%."
**Correction:** CAGR = (2,415/2,000)^(1/2) − 1 = **9.89%**. Simple division ignores compounding; averaging annual rates is also wrong.
**Prevention:** Count **intervals**, not data points (2023→2025 = 2). Use the power formula.
**Where it appears:** [`02` §2.4](02-aptitude-reasoning-and-data-interpretation.md), Mock C ([`14`](14-second-mock-assessment-mcq-60-min.md)) Q6 and Q19.

---

## Trap 17 — Ignoring persistence, access and net price in forecasts

**Example:** Forecast = eligible patients × share × **list** price, with 100% persistence and no access friction.
**Correction:** Apply persistence (a 65% 12-month rate cuts the patient base), access factors (prior authorisation, step therapy), and **net** price (gross-to-net is often 30–60% in the US).
**Prevention:** Write the full chain: eligible → access-adjusted → started → persistent → net price. Every factor you leave out inflates the answer.
**Where it appears:** [`07` §7.4–7.5](07-life-sciences-business-analytics-and-cases.md), [`16`](16-case-practice-workbook.md).

---

## Trap 18 — Confusing absolute and relative importance

**Example:** Reporting only "West grew fastest (+30.8%)" when the CEO's decision depends on contribution ("South added ₹144 Cr").
**Correction:** Report both: fastest grower (relative) and largest contributor (absolute); state which one drives the decision.
**Prevention:** For every ranked metric, ask "biggest by how much, and biggest relative to what base?"
**Where it appears:** [`02` §2.9](02-aptitude-reasoning-and-data-interpretation.md), Mock C ([`14`](14-second-mock-assessment-mcq-60-min.md)) Q20.

---

## Trap 19 — Answering a different question than the one asked

**Example:** Being asked for the *median* and reporting the mean; asked for volume share and reporting revenue share; asked for percentage points and answering in percent.
**Correction:** Re-read the stem and underline the delimiter: median/mean, volume/value, pp/%, change/level, year-on-year/CAGR.
**Prevention:** Copy the question's key noun into your answer line ("The median time-to-fill is…").
**Where it appears:** everywhere; it is the most common cause of "I knew this" losses.

---

## Trap 20 — Not checking your own work

**Example:** Submitting a PivotTable total that does not reconcile with the SUM of the source column; a lookup returning `#N/A` that nobody noticed.
**Correction:** Run the four standard checks: (1) parts sum to the whole; (2) row counts match expectations; (3) no lookup errors; (4) a manual spot-check of two known rows.
**Prevention:** Build the checks as formulas so they are visible on the sheet — Trinity's JD explicitly lists "check own work for errors" as a required skill.
**Where it appears:** Excel assessments ([`10` Mock B](10-mock-assessment-and-excel-test.md)), case deliverables, interview answers.

---

## Mistake log (copy this table into your notes)

| Date | Source file | Question | What I did | Trap # | Correct approach | Retry date | Retry passed? |
|---|---|---|---|---|---|---|---|
| | | | | | | | |

**Weekly review questions**
1. Which trap appeared most often this week? (Fix that one first.)
2. Which traps were *concept* errors vs *careless* errors? (Careless errors need checklists, concept errors need study.)
3. Which trap has appeared three or more times? (That is your revision plan for tomorrow.)

---

## The one-page prevention checklist

- [ ] Denominator check: is it the **old** value?
- [ ] pp or %? Label it.
- [ ] Group sizes equal? If no → weight.
- [ ] Which margin? Name it.
- [ ] Units on every line; volume vs value labelled.
- [ ] Association or causation? Language matches evidence.
- [ ] Axis start, scale, base — checked before interpreting.
- [ ] Lookups: exact match, absolute ranges, failure count.
- [ ] Blanks, duplicates, text-numbers audited and reported.
- [ ] Survey claims carry base, n, MOE and the intent-vs-behaviour caveat.
- [ ] Recommendation includes assumption, risk and next step.
- [ ] Structure stated before analysis.
- [ ] Precision kept until the final answer.
- [ ] CAGR uses intervals, not data points.
- [ ] Absolute and relative importance both reported.
- [ ] Answer matches the question's exact ask.
- [ ] Parts reconcile with the whole before submitting.

**Next:** [11 — Final revision cheatsheet](11-final-revision-cheatsheet.md) · [17 — Score rubric](17-mock-and-interview-score-rubric.md)
