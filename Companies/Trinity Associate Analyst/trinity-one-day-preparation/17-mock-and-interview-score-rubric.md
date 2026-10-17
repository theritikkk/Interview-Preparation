# 17 — Mock & Interview Score Rubric

> **Learning objectives:** score yourself consistently after every mock (assessment, Excel task, case, technical and behavioral interview) and convert the score into a revision plan.
> **Estimated study time:** 30 min per review cycle.
> **Use it immediately after:** Mock A / Mock B ([`10`](10-mock-assessment-and-excel-test.md)), Mock C ([`14`](14-second-mock-assessment-mcq-60-min.md)), the case workbook ([`16`](16-case-practice-workbook.md)), or a spoken run of the Q-bank ([`09`](09-technical-interview-question-bank.md)) or behavioral answers ([`12`](12-behavioral-interview-and-hr.md)).

---

## §17.1 The nine dimensions

Score each **1–5**. Be harsh: a 5 means "would not embarrass me in front of a client".

| # | Dimension | What a 5 looks like | Watch for |
|---|---|---|---|
| 1 | **Numerical accuracy** | All arithmetic correct; units and bases right | Wrong denominator, pp vs %, unit slips |
| 2 | **Analytical reasoning** | Breaks problems down; tests hypotheses; reaches defensible conclusions | Jumping to a conclusion; no comparison |
| 3 | **Data interpretation** | Reads tables/charts correctly; names the main finding | Describes instead of interpreting; misses the axis/base |
| 4 | **Excel proficiency** | Writes the right function first time; validates outputs | Guessing functions; no validation; hardcoded numbers |
| 5 | **Business judgment** | Connects numbers to decisions; prioritises implications | Stops at the number; no "so what" |
| 6 | **Communication** | Concise, structured, audience-aware; numbers spoken clearly | Ramble; jargon; buries the headline |
| 7 | **Structure** | Objective → framework → analysis → findings → recommendation → risks → next steps | Stream of consciousness |
| 8 | **Confidence** | Asserts assumptions, welcomes challenges, corrects gracefully | Over-apologising; defending a wrong number |
| 9 | **Attention to detail** | Checks totals, reads the question fully, states exclusions | Skips validation; ignores a caveat in the stem |

---

## §17.2 Anchor descriptions

| Score | Meaning | Typical symptom |
|---|---|---|
| **1 — Poor** | Wrong or unusable | Wrong answer, no method, or cannot explain own steps |
| **2 — Developing** | Partially right, major gaps | Right direction, wrong number or missing key element |
| **3 — Adequate** | Correct core answer, thin on insight | Answer right, "so what" missing, one process gap |
| **4 — Strong** | Correct, structured, quantified, with limitations | Minor polish issues only |
| **5 — Client-ready** | Insight-led, prioritised, quantified, risk-aware | Would survive a client challenge unedited |

**Score bands (total out of 45):**

| Total | Reading | Action |
|---|---|---|
| 40–45 | Assessment-ready | Maintain; shift to speed and polish |
| 33–39 | On track | Fix the two lowest dimensions only |
| 25–32 | Not yet | Re-run the topic drills before another mock |
| < 25 | Foundation gap | Return to concepts before any more timed practice |

---

## §17.3 Worked examples of weak / average / strong answers

### Example A (data interpretation)
**Prompt:** "Here is the region table. What do you notice?"

- **Weak (1–2):** "North is the biggest and East is the smallest." *(Reads the table; no change, no interpretation, no units.)*
- **Average (3):** "The market grew 20.75% from 2023 to 2025, and West grew fastest at 30.8%." *(Correct and quantified, but no implication.)*
- **Strong (4–5):** "The market grew 20.75% to ₹2,415 Cr. West grew fastest (+30.8%), but South contributed the most absolute growth (+₹144 Cr, now 31.6% of the market). East is the smallest and slowest (+4.8%), so it is a *mix* problem rather than a decline — I would check whether East's underperformance is coverage or pricing before shifting investment. To decide, I'd need East's rep coverage and share of voice." *(Number + differentiation + implication + missing data.)*

### Example B (Excel)
**Prompt:** "Total TRx for North CardioX."

- **Weak:** Wrong range, or adds rows manually and gets a different number, or uses `SUMIF` with arguments swapped.
- **Average:** `=SUMIFS(D2:D8,B2:B8,"North",C2:C8,"CardioX")` → 2,230, but does not check the total.
- **Strong:** Same formula **plus** cross-foot validation ("region totals sum to 5,625, which matches SUM(D2:D8)") and a note that one row had a blank TRx excluded.

### Example C (statistics)
**Prompt:** "The survey says 52% intend to prescribe. Can we rely on it?"

- **Weak:** "Yes, 52% is a majority." *(Treats a stated-intent statistic as behaviour.)*
- **Average:** "It has a margin of error of ±5.7 pp, so it is between 46% and 58%." *(Correct CI, no behavioural check.)*
- **Strong:** "Statistically it is precise (±5.7 pp, n = 300), but statistically precise does not mean predictive: intent typically overstates behaviour. The behavioural question in the same survey says 4.1 starts per physician per month; I would anchor the forecast on that, apply a conservative discount to intent, and validate with claims data or a pilot." *(Mechanics + limitation + what to do.)*

### Example D (case recommendation)
**Prompt:** "So what do you recommend?"

- **Weak:** "Launch, because the market is big." *(No numbers, no conditions.)*
- **Average:** "Launch, and stage it because of access risk." *(Right shape, no quantification or gates.)*
- **Strong:** "Stage the launch: break-even is $2.15 B cumulative revenue, reached mid-Year 3, and Y1 needs only 15.9% of likely prescribers — so demand is not the constraint. I would gate scale-up on two metrics — access for ≥50% of covered lives and ≥65% six-month persistence — because a 30% switch to a discounted competitor puts ~$292 M of Y2 revenue at risk." *(Decision + three numbers + gates + risk magnitude.)*

### Example E (behavioral)
**Prompt:** "Tell me about a mistake you made."

- **Weak:** "I'm a perfectionist; I don't really make mistakes." *(Non-answer.)*
- **Average:** Tells a story with no consequence and no preventive change.
- **Strong:** Owns the mistake, states the impact, shows the fix, and names the control added so it cannot recur — in under 90 seconds.

---

## §17.4 Score sheet (copy per mock)

| Dimension | Score /5 | Evidence (what I actually said/did) | One concrete fix |
|---|---|---|---|
| Numerical accuracy | | | |
| Analytical reasoning | | | |
| Data interpretation | | | |
| Excel proficiency | | | |
| Business judgment | | | |
| Communication | | | |
| Structure | | | |
| Confidence | | | |
| Attention to detail | | | |
| **Total** | **/45** | | |

---

## §17.5 From score to revision plan

| Lowest dimension | Revise first | Specific drill |
|---|---|---|
| Numerical accuracy | [`02`](02-aptitude-reasoning-and-data-interpretation.md), [`15`](15-errors-mistakes-and-traps.md) | 20 mixed questions, 60 seconds each, log every trap by name |
| Analytical reasoning | [`07`](07-life-sciences-business-analytics-and-cases.md), [`16`](16-case-practice-workbook.md) | Run the 8-step framework on 3 short cases, out loud |
| Data interpretation | [`02` §2.8–2.10](02-aptitude-reasoning-and-data-interpretation.md) | 5 datasets: one-sentence finding + implication + missing data for each |
| Excel proficiency | [`03`](03-excel-complete-revision.md), [`10` Mock B](10-mock-assessment-and-excel-test.md) | Rebuild the Excel mock from a clean sheet, 30 minutes |
| Business judgment | [`07`](07-life-sciences-business-analytics-and-cases.md), [`08`](08-powerpoint-and-business-communication.md) | Convert 5 findings into recommendations with action/evidence/impact/risk |
| Communication | [`08`](08-powerpoint-and-business-communication.md) | Record a 2-minute verbal readout; cut it to 90 seconds |
| Structure | [`07` §7.3](07-life-sciences-business-analytics-and-cases.md) | Memorise the 8-step framework; apply to 3 questions with no data |
| Confidence | [`12`](12-behavioral-interview-and-hr.md) | Rehearse the 5 hardest answers aloud, twice, recorded |
| Attention to detail | [`03` §3.10](03-excel-complete-revision.md), [`15`](15-errors-mistakes-and-traps.md) | Add a validation gate to every future answer before finalising |

**Rule:** fix at most **two** dimensions per cycle. Chasing everything produces no improvement.

---

## §17.6 Review protocol (30 minutes)

1. **Re-read your answers/notes** without the key (5 min).
2. **Score with the key** — mark every answer right/wrong/partial (10 min).
3. **For each miss, name the cause**: concept, arithmetic, misread, time, or presentation (5 min).
4. **Fill the score sheet** and pick the two lowest dimensions (5 min).
5. **Schedule the drills** in your calendar; set the retry date for the specific questions you missed (5 min).

**Retry policy:** a question is only "done" when you answer it correctly, from scratch, on a later day, without the key.

**Next:** [11 — Final revision cheatsheet](11-final-revision-cheatsheet.md) → back to [README](README.md)
