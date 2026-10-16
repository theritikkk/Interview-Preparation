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
- [ ] **2 behavioral stories** — one teamwork, one deadline/attention-to-detail ([`09`](09-technical-interview-question-bank.md) §9.15)
- [ ] **Questions to ask them** — have 2 ready ([`09`](09-technical-interview-question-bank.md) §9.16)

### What to skip if you're behind schedule
In priority order, sacrifice: (1) case Case-2 deep work → replace with reading the solution; (2) SQL/Python snippets → read only; (3) PowerPoint file → read only the 4-question framework; (4) DI practice set → cheatsheet formulas only. **Never skip:** Excel hands-on block, the cheatsheet, and one timed mock.
