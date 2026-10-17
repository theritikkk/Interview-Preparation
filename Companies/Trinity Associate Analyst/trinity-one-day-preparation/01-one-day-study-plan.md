# 01 — One-Day Study Plan

> **Learning objective:** know exactly what to do, in what order, hour by hour — and what to cut if the day goes sideways.
> **Estimated time:** 5 min to read · the plan itself structures your whole day.

Tags: schedule is **PRACTICE** (built around the **CONFIRMED** process: proctored online 60 min → Excel 30–45 min → technical 60 min → behavioral 30 min).

---

## §1.1 The full day (~8 hours active study + breaks)

**Design principle:** Excel gets the biggest single block (hands-on, 75 min), but no topic gets abandoned. Reading is capped; doing is prioritized. The mock is late in the day so everything you revised is fresh, and the cheatsheet is last so it is freshest.

| Clock | Time | Block | What you do | Output to produce |
|---|---|---|---|---|
| 09:00 | 20 min | **Orientation** | Read [`README`](README.md); skim [`11-cheatsheet`](11-final-revision-cheatsheet.md) once | Mental map of the day |
| 09:20 | 55 min | **[`02` Aptitude + DI](02-aptitude-reasoning-and-data-interpretation.md)** | Read formula tables fast; then do the mixed set (25 min, timed); check answers | ≥ 10/12 on the mixed set |
| 10:15 | 10 min | Break | Water, walk, no screen | — |
| 10:25 | 75 min | **[`03` Excel](03-excel-complete-revision.md) + [Mock B](10-mock-assessment-and-excel-test.md)** | Scan syntax tables (20 min); run Mock B on a real spreadsheet (45 min); quick self-check (10 min) | Completed workbook + score |
| 11:40 | 15 min | Break | Snack | — |
| 11:55 | 50 min | **[`04` Stats + survey](04-statistics-survey-and-market-research.md)** | Read concept tables; reproduce the confidence-interval example by hand; do the practice set | One CI computed manually |
| 12:45 | 45 min | **[`05` Power BI + Tableau](05-power-bi-tableau-and-data-visualization.md)** | Read both tool sections; memorize the comparison table; cover the dashboard spec and re-state the 5 KPIs from memory | Dashboard spec re-drawn from memory |
| 13:30 | 45 min | **Lunch break** | Eat away from the desk | — |
| 14:15 | 45 min | **[`06` SQL + Python](06-sql-python-and-automation-for-analysts.md)** | Read SQL patterns; write 2 queries from memory; read pandas snippets; run one snippet if a laptop is free | 2 queries written without peeking |
| 15:00 | 55 min | **[`07` Life sciences + cases](07-life-sciences-business-analytics-and-cases.md)** | Read domain essentials + framework (15 min); read Case 1 with solution (15 min); **solve Case 2 unaided (≤ 25 min)** | Case 2 answer + recommendation |
| 15:55 | 10 min | Break | Stand up, breathe | — |
| 16:05 | 60 min | **Mock A (timed)** | [`10`](10-mock-assessment-and-excel-test.md) — one sitting, no peeking at the key | Raw score |
| 17:05 | 20 min | **Mock A review** | Check all 30 answers; write down every miss and its trap | List of weak areas |
| 17:25 | 30 min | **[`08` Communication](08-powerpoint-and-business-communication.md) + [`09` Q-bank](09-technical-interview-question-bank.md)** | Read the 4-question framework; skim the 15 marked high-yield Qs in 09 | 3 insight-led titles practiced |
| 17:55 | 20 min | **Final revision** | [`11`](11-final-revision-cheatsheet.md) full pass + checklist below | Checklist ticked |

**Active study: ~7 h 55 m. Elapsed incl. breaks: ~9 h 15 m. Adjust start time freely — keep the order.**

---

## §1.2 Emergency plan — 4 hours (tests tomorrow / no time)

| Time | Block | Do only this |
|---|---|---|
| 30 min | [`11`](11-final-revision-cheatsheet.md) + [`02` formula tables](02-aptitude-reasoning-and-data-interpretation.md) | The cheatsheet is specifically built for this — every formula you need |
| 60 min | [`03` Excel](03-excel-complete-revision.md) syntax tables + **Mock B tasks 1–5 only** (skip chart) | Clean, aggregate, achievement, lookup, pivot — the 80% of the Excel test |
| 40 min | [`04` Key sections](04-statistics-survey-and-market-research.md): MoE, weighting, biases, top-2-box + practice key | Read answers actively, don't attempt under time pressure |
| 30 min | [`07` framework + Case 1](07-life-sciences-business-analytics-and-cases.md) | The 8-step framework + one full case narrative |
| 30 min | [`05` comparison table + chart guide](05-power-bi-tableau-and-data-visualization.md) | Power BI vs Tableau; chart-selection rules |
| 20 min | [`06` SQL patterns](06-sql-python-and-automation-for-analysts.md) | GROUP BY / HAVING / JOIN / CASE WHEN patterns only |
| 30 min | [`09` high-yield Qs](09-technical-interview-question-bank.md) + [`08` 4-question framework](08-powerpoint-and-business-communication.md) + checklist | Skim, speak out loud |

Total: 240 min = exactly 4 hours.

**If the behavioral round could also land this week:** add 30 minutes tonight — write the two behavioral stories from [`12` §12.1](12-behavioral-interview-and-hr.md) and one project pitch from [`13` §13.6](13-resume-and-project-discussion.md). These are the two cheapest points in the whole process.

---

## §1.3 Final 30-minute review checklist

Do this immediately before the assessment (or the night before, then sleep — sleep beats cramming).

- [ ] **% change vs percentage points** — can state the difference and compute both ([`02`](02-aptitude-reasoning-and-data-interpretation.md) §2.1)
- [ ] **CAGR formula** — `(end/start)^(1/n) − 1`, and when to use it vs simple growth ([`02`](02-aptitude-reasoning-and-data-interpretation.md) §2.4)
- [ ] **Weighted vs simple average** — when the base sizes differ ([`02`](02-aptitude-reasoning-and-data-interpretation.md) §2.3)
- [ ] **SUMIFS / COUNTIFS / XLOOKUP syntax** — write each on blank paper ([`03`](03-excel-complete-revision.md) §§3.3–3.4)
- [ ] **Margin of error** — `1.96 × √(p(1−p)/n)`; how to read it ([`04`](04-statistics-survey-and-market-research.md) §4.6)
- [ ] **Survey pitfalls** — leading questions, nonresponse bias, coverage bias ([`04`](04-statistics-survey-and-market-research.md) §§4.7–4.9)
- [ ] **Power BI terms** — measure vs calculated column; DAX `CALCULATE`, `DISTINCTCOUNT`, `DIVIDE` ([`05`](05-power-bi-tableau-and-data-visualization.md) §5.3)
- [ ] **Tableau terms** — dimension vs measure; discrete vs continuous ([`05`](05-power-bi-tableau-and-data-visualization.md) §5.5)
- [ ] **SQL pattern** — `SELECT region, SUM(trx) FROM sales GROUP BY region HAVING SUM(trx) > 5000;` ([`06`](06-sql-python-and-automation-for-analysts.md) §6.2)
- [ ] **pandas pattern** — `df.groupby("Region")["TRx"].sum()` ([`06`](06-sql-python-and-automation-for-analysts.md) §6.3)
- [ ] **Chart selection** — trend→line; comparison→bar; part-of-whole→stacked bar (only few categories); relationship→scatter ([`05`](05-power-bi-tableau-and-data-visualization.md) §5.8)
- [ ] **Case framework** — the 8 steps, in order ([`07`](07-life-sciences-business-analytics-and-cases.md) §7.3)
- [ ] **Communication pattern** — What did I find? Why does it matter? What should we do? What are the limits? ([`08`](08-powerpoint-and-business-communication.md) §8.3)
- [ ] **2 behavioral stories** — one teamwork, one deadline/attention-to-detail ([`12` §12.1](12-behavioral-interview-and-hr.md), [`09`](09-technical-interview-question-bank.md) §9.15)
- [ ] **1 project pitch, spoken aloud** — 60 seconds, with one real number ([`13` §13.6](13-resume-and-project-discussion.md))
- [ ] **Questions to ask them** — have 2 ready ([`09`](09-technical-interview-question-bank.md) §9.16)
- [ ] **Traps prevention checklist** skimmed ([`15`](15-errors-mistakes-and-traps.md))

### What to skip if you're behind schedule
In priority order, sacrifice: (1) case Case-2 deep work → replace with reading the solution; (2) SQL/Python snippets → read only; (3) PowerPoint file → read only the 4-question framework; (4) DI practice set → cheatsheet formulas only. **Never skip:** Excel hands-on block, the cheatsheet, and one timed mock.

---

## §1.4 Extended plan — when the drive is later in the week

> The drive can land any day this week, so don't stop at one day. This schedule spreads the core (§1.1) over two days, then adds three consolidation days using files [`12`](12-behavioral-interview-and-hr.md)–[`17`](17-mock-and-interview-score-rubric.md). Keep the order; move the dates freely.
> **Already done 02, 03 + Mock B?** Start at Day 2.

| Day | Blocks | What you do | Output to produce |
|---|---|---|---|
| **Day 1** (core, part 1) | [`02`](02-aptitude-reasoning-and-data-interpretation.md) → [`03`](03-excel-complete-revision.md) + Mock B | Per §1.1 morning blocks | Mixed set ≥ 10/12; completed workbook + score |
| **Day 2** (core, part 2) | [`04`](04-statistics-survey-and-market-research.md) → [`05`](05-power-bi-tableau-and-data-visualization.md) → [`06`](06-sql-python-and-automation-for-analysts.md) | CI by hand; Power BI vs Tableau table from memory; 2 SQL queries from memory | One CI computed; dashboard spec re-drawn; queries written |
| **Day 3** (core, part 3) | [`07`](07-life-sciences-business-analytics-and-cases.md) (framework + Case 1, then **Case 2 unaided**) → **Mock A timed** ([`10`](10-mock-assessment-and-excel-test.md)) → review with [`17`](17-mock-and-interview-score-rubric.md) | Case 2 ≤ 25 min; Mock A without peeking; score and log misses | Case recommendation; Mock A ≥ 24/30 or a trap list |
| **Day 4** (consolidation 1) | [`16`](16-case-practice-workbook.md) → [`15`](15-errors-mistakes-and-traps.md) | Relivax case: 25 min timed, then compare with §16.3; read all traps | Case self-score (§16.5); prevention checklist read |
| **Day 5** (consolidation 2) | [`12`](12-behavioral-interview-and-hr.md) → [`13`](13-resume-and-project-discussion.md) | Write the 15 answer skeletons; record 4 answers; fill both pitch sheets | 5 stories, 2 project pitches, 2 questions to ask |
| **Day 6** (consolidation 3) | **Mock C timed** ([`14`](14-second-mock-assessment-mcq-60-min.md)) → key review → redo Mock B from `data/trinity_excel_mock.csv` (time permitting) | Second 30-Q mock in MCQ format; real CSV import | Mock C ≥ 24/30; misses logged by trap number |
| **Day 7 / final** | [`11`](11-final-revision-cheatsheet.md) cover-to-cover + [`09`](09-technical-interview-question-bank.md) starred Qs aloud + 2 stories aloud + [`15`](15-errors-mistakes-and-traps.md) checklist | Light, spoken, no new material | Confidence + freshness |

**Priority if the week compresses to 2–3 days:** [`12`](12-behavioral-interview-and-hr.md) (behavioral answers) → [`15`](15-errors-mistakes-and-traps.md) (traps) → [`13`](13-resume-and-project-discussion.md) (two pitches) → [`14`](14-second-mock-assessment-mcq-60-min.md) (Mock C) → [`16`](16-case-practice-workbook.md) → [`17`](17-mock-and-interview-score-rubric.md).

**Never skip the behavioral stories** — it is a scored round with no other coverage in this pack. If the drive is announced for tomorrow, do §1.2 plus 30 minutes of [`12` §12.1](12-behavioral-interview-and-hr.md) + [`13` §13.6](13-resume-and-project-discussion.md).
