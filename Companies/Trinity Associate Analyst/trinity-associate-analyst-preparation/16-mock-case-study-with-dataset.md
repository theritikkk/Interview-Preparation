# 16 — Mock Case Study with Dataset (Market Entry)

**Purpose:** one complete, data-heavy market-entry case in the style the recruitment documents and candidate reports suggest (older JD: case interviews; candidate reports: data-heavy market-entry cases with visualisations). This is the closest thing in the repository to a full case-interview simulation.
**Prerequisites:** [07](07-business-analytics-and-market-research.md), [09](09-market-entry-and-business-case-interviews.md), [06](06-statistics-and-survey-analytics.md), [08](08-life-sciences-industry-fundamentals.md).
**Suggested study time:** 25 minutes analysis (timed) + 10 minutes verbal walkthrough + 30 minutes reviewing the worked solution.
**Provenance:** fictional company, fictional product, fictional data — **PRACTICE INFERENCE**. Nothing here describes a real Trinity client or a real case.

---

## §16.1 The case

**Client.** Aurelia Pharma is a mid-sized specialty company with one marketed product and no presence in neurology. The executive committee is deciding whether to launch **Relivax**, a monthly injectable preventive therapy for **chronic migraine**, in the United States.

**Decision.** By the end of the quarter the committee must decide: **launch nationally, launch in a staged way, or do not launch.** They also want the revenue the launch could credibly deliver in years 1–3 and the assumptions those numbers rest on.

**Your job.** Analyse the data below, quantify the opportunity and the economics, sanity-check the demand forecast against the survey, identify risks, and make a recommendation with explicit assumptions.

### Data 1 — US chronic-migraine market structure (2026)

| Metric | Value |
|---|---|
| Diagnosed adult migraine patients | 39.0 M |
| Share eligible for preventive therapy | 40% |
| Of eligible: chronic migraine | 20% |
| Of eligible: episodic migraine | 80% |
| Currently on any preventive therapy | 30% of all eligible |
| Of chronic eligible: currently treated | 35% |

### Data 2 — Preventive-therapy market and competition (2026)

| Company | Class | Share of Rx volume | Annual net price per patient |
|---|---|---:|---:|
| CompA | Oral preventive (small molecule) | 38% | $1,200 |
| CompB | Oral CGRP (small molecule) | 22% | $6,000 |
| CompC | Injectable CGRP (quarterly) | 16% | $7,800 |
| CompD | Botulinum toxin | 12% | $9,000 |
| Others (off-label, generics) | Various | 12% | $400 |

Market revenue 2026: **$11.4 B**, expected to grow at **8% per year** through 2031.

### Data 3 — Relivax economics (plan)

| Assumption | Value |
|---|---|
| Annual net price per patient | $9,600 |
| COGS + distribution | 35% of net price |
| Launch investment (cumulative, 3 years) | $1.4 B |
| Target population for launch | Untreated chronic-migraine patients |
| Penetration of that population: Y1 / Y2 / Y3 | 2% / 5% / 8% |

### Data 4 — Physician survey (n = 300 neurologists)

| Metric | Value |
|---|---|
| Aided awareness (of the class, not the brand) | 64% |
| Top-2-box "likely/very likely to prescribe" | 52% |
| — Academic neurologists (n = 90) | 63% |
| — Community neurologists (n = 210) | 47% |
| Cite prior authorisation as a major barrier | 38% |
| Mean patient starts in the last month (behavioural) | 4.1 per physician |

**Context for scaling:** the US neurology call universe is approximately **20,000** physicians who treat chronic migraine.

---

## §16.2 Questions

1. Compute the eligible preventive-therapy population and split it into chronic and episodic.
2. Compute the number of **untreated chronic-migraine patients** — Relivax's launch target population.
3. Compute the market size in 2031 at 8% CAGR, and compute the theoretical "everything at Relivax's price" figure. Why is the second number not an opportunity?
4. Build the **revenue share** table from the volume shares and prices. Which competitor is the most valuable commercially, and what does the comparison with CompA tell you?
5. Compute Relivax revenue for Y1, Y2 and Y3 at the planned penetration rates.
6. Compute contribution per patient, cumulative revenue across three years, the break-even revenue, and the point at which break-even is reached.
7. Sanity-check the Y1 plan against the survey: how many active prescribers does Y1 require, and how does that compare with the survey-implied pool of likely prescribers (apply a 50% intent discount)? Is the plan conservative or aggressive?
8. Compute the 95% confidence interval for the overall 52% intent figure, and test whether the academic–community difference (63% vs 47%) is statistically significant. State the practical implication.
9. Scope the competitive response: if CompC cuts its price 20% and 30% of intended prescribers switch, what Y2 revenue is at risk? What does Relivax's price premium look like before and after the cut?
10. Run the downside price scenario: Relivax cuts its price 15%. What happens to revenue and to break-even timing?
11. What data would you request before signing off? Name the three that matter most.
12. **Recommendation task:** recommend national launch, staged launch, or no launch. Your answer must include: the decision, the three numbers that drive it, at least three explicit assumptions, the main risks, and the next step.

### Data limitations (state these in your answer)

- Volume shares and prices are **assumed to combine** into revenue shares; actual net prices vary by plan and rebate.
- The survey measures **stated intent**, which historically overstates behaviour.
- "Untreated chronic population" assumes treatment rates and diagnosis rates are stable.
- No persistence data for Relivax (a monthly injectable has adherence advantages and administration burdens).
- Competitor response is modelled as a single scenario, not a probability-weighted range.
- Relivax's adoption is assumed to come from untreated patients; in reality much early uptake is **switching**.

---
---

# §16.3 Worked solution

### Q1 — Eligible pool: 15.6 M (chronic 3.12 M; episodic 12.48 M)

39.0 M × 40% = **15.6 M** eligible.
- Chronic: 15.6 M × 20% = **3.12 M**
- Episodic: 15.6 M × 80% = **12.48 M**

### Q2 — Untreated chronic population: 2.028 M

Chronic eligible 3.12 M; treated 35% → 1.092 M; untreated = 3.12 − 1.092 = **2.028 M**.
*(Also check: 30% of all eligible on treatment = 4.68 M, so 1.092 M of them are chronic — consistent.)*

### Q3 — 2031 market: $16.75 B; theoretical maximum $149.8 B — not an opportunity

- 2031 = 11.4 × 1.08⁵ = **$16.75 B**.
- Theoretical maximum = 15.6 M eligible × $9,600 = **$149.8 B**.
- Why it is not an opportunity: it assumes 100% of eligible patients are treated, all with Relivax, at full net price, with no competition and no access friction. **TAM is a boundary, not a forecast** — the practical number is the treated-and-accessible population.

### Q4 — Volume share vs revenue share: CompB and CompC are the value prizes

Weighted index = share × price; total = 415,200 (in "share × $" units):

| Company | Volume share | Weighted value | Revenue share |
|---|---:|---:|---:|
| CompA | 38% | 45,600 | **11.0%** |
| CompB | 22% | 132,000 | **31.8%** |
| CompC | 16% | 124,800 | **30.1%** |
| CompD | 12% | 108,000 | **26.0%** |
| Others | 12% | 4,800 | **1.2%** |

**Insight:** CompA has the largest *volume* share (38%) but only 11% of the *value*. The commercial battle is with CompB and CompC, who hold 62% of revenue between them with only 38% of volume. Any analysis that used volume share as a proxy for revenue would misidentify Aurelia's real competitors.

### Q5 — Relivax revenue: $389.4 M / $973.4 M / $1,557.5 M

- Y1: 2.028 M × 2% = **40,560** patients × $9,600 = **$389.4 M**
- Y2: × 5% = **101,400** × $9,600 = **$973.4 M**
- Y3: × 8% = **162,240** × $9,600 = **$1,557.5 M**
- Three-year cumulative: **$2,920.3 M**

### Q6 — Contribution $6,240; break-even $2.15 B; reached mid-Year 3

- Contribution per patient = 9,600 × (1 − 0.35) = **$6,240** (65% margin).
- Break-even revenue = $1.4 B ÷ 0.65 = **$2,153.8 M**.
- Cumulative by end of Y2 = 389.4 + 973.4 = **$1,362.8 M** → shortfall of **$791.0 M**.
- $791.0 M ÷ $1,557.5 M = **50.8% into Y3** → break-even around **the end of Q2 / early Q3 of Year 3**.

### Q7 — Sanity check: the plan is conservative, which is a positive (and a warning)

- Required Y1 patient starts = 40,560 ÷ 12 = **3,380 per month**.
- At the survey's behavioural rate of 4.1 starts per physician per month, that requires **≈824 active prescribers** (3,380 ÷ 4.1).
- Survey-implied likely prescriber pool: 20,000 physicians × 52% × 50% discount = **5,200**.
- So Y1 needs **824 ÷ 5,200 = 15.9%** of likely prescribers to convert within a year.
- **Interpretation:** on these survey inputs, the plan is not aggressive for demand — the binding constraint is **access and activation**, not physician willingness. The warning: the survey's own intent-implied volume (5,200 physicians × 4.1 × 12 ≈ 256,000 starts/year) is far above the plan; if the plan is underselling, the risk is that Aurelia underinvests in access infrastructure and the launch underperforms for operational, not clinical, reasons.

### Q8 — CI 46.3–57.7%; the segment difference is significant

- Overall: SE = √(0.52 × 0.48/300) = 2.88 pp → MOE = **5.65 pp** → 95% CI = **46.3% to 57.7%**.
- Segment counts: academic 63% × 90 ≈ **57**; community 47% × 210 ≈ **99**; pooled p = 156/300 = **0.52**.
- SE of difference = √(0.52 × 0.48 × (1/90 + 1/210)) = **6.29 pp**; z = (63 − 47)/6.29 = **2.54** → p ≈ **0.011** (two-sided).
- **Practical implication:** the segment gap is real, not sampling noise. Academic physicians are 16 pp more likely to say they will prescribe; segment-specific messaging and evidence programs are justified. Caveat: the academic estimate carries a wide CI (53.0–73.0%) and academics are only n = 90 — report the range, and note that academic intent often overstates community behaviour.

### Q9 — Competitive response: $292 M of Y2 revenue at risk; a 54% premium after the cut

- CompC price after a 20% cut = $7,800 × 0.80 = **$6,240**.
- Relivax premium: $9,600 vs $7,800 = **+23.1%** today; after the cut, $9,600 vs $6,240 = **+53.8%**.
- 30% of intended prescribers switching → at-risk revenue = 30% × $973.4 M = **$292.0 M** in Y2 (≈30% of Y2 revenue).
- Implication: pricing power depends on the value story (monthly dosing, chronic-segment efficacy). A 54% premium is defensible only with differentiated data, access support and a clear patient-benefit narrative; otherwise expect share loss and rebate pressure.

### Q10 — Price cut of 15%: revenue falls to $331.0 M / $827.4 M / $1,323.9 M; break-even slips

- Multiplier 0.85: Y1 $331.0 M, Y2 $827.4 M, Y3 $1,323.9 M; three-year cumulative **$2,482.3 M**.
- Cumulative by end of Y2 = $331.0 + $827.4 = $1,158.4 M; shortfall to $2,153.8 M = **$995.4 M**.
- $995.4 M ÷ $1,323.9 M = **75.2% into Y3** → break-even moves from mid-Y3 to **late Q4 of Y3**.
- **Takeaway:** a 15% price cut delays break-even by roughly two quarters, which is a real cost — but it may be cheaper than losing 30% of revenue to a competitor price war. The decision hinges on whether access (not price) is the binding constraint, which is exactly what Q11's requested data would resolve.

### Q11 — Three data requests that matter most

1. **Access reality:** % of covered lives with unrestricted coverage, prior-authorisation rate and step-therapy conditions for injectable CGRPs (the survey says 38% see PA as a major barrier — quantify it).
2. **Persistence/adherence:** real-world 6- and 12-month persistence for monthly injectables in chronic migraine (drives revenue quality, and the plan assumes full-year revenue per patient).
3. **Switching behaviour:** how much early uptake comes from patients already on CompB/CompC rather than untreated patients (changes the competitive response and the true addressable base).
   *Runner-up:* payer-level price expectations and rebate benchmarks to validate the $9,600 net price.

### Q12 — Recommendation

**Decision: staged launch.**

**The three numbers that drive it:**
1. **Break-even at $2.15 B cumulative revenue — reached mid-Year 3** on base-case penetration (2%/5%/8%). This is a viable plan, but it has a thin cushion.
2. **2.028 M untreated chronic patients** is a large, genuinely underserved target — the opportunity is real, not manufactured.
3. **15.9% conversion of likely prescribers needed in Y1** — the demand side is feasible; the constraint is access (38% cite prior authorisation), which is controllable and testable in a pilot.

**Explicit assumptions**
1. Net price of $9,600 holds (no additional rebates beyond the assumed 35% cost load; no price erosion).
2. Treatment/diagnosis rates stay stable, so the untreated chronic pool stays ≈2.03 M.
3. Access is achievable in ≥50% of commercial lives within 12 months of launch.

**Risks**
- **Access risk (highest):** if PA restrictions are not resolved, Y1 penetration could halve, pushing break-even into Year 4.
- **Competitive price response:** CompC's price cut could put ~$292 M of Y2 revenue at risk.
- **Forecast risk:** the survey measures intent; the plan's conservatism provides some protection, but the "switching vs untreated" mix is unverified.
- **Measurement risk:** 8% penetration by Y3 rests on persistence assumptions with no Relivax data yet.

**Next step (90 days)**
1. Run a 3-metro pilot with a small specialist sales force and a full PA-support program.
2. Gate national scale-up on two metrics: **≥50% of covered lives with feasible access** and **persistence ≥65% at 6 months**.
3. Negotiate payer contracts in parallel; hold the second capital tranche until the gates are met.

**How to defend the recommendation if challenged:** "If the payer study shows access below 30% of lives or persistence below 55%, I would downgrade to a narrower specialty launch or delay — and I would rather learn that in a 3-metro pilot than after committing the full $1.4 B."

---

## §16.4 Follow-up questions (practise aloud, ≤60 seconds each)

1. "You said staged — what does staging actually cost us?" *(Slower share accumulation, roughly two quarters of missed revenue at ~$240 M per quarter at Y3 run rate, versus a materially lower risk of a $1.4 B write-off.)*
2. "Why not price at $7,800 to match CompC?" *(Revenue at Y3 would fall to ~$1,266 M — 15% lower — and break-even slips to late Y3; unless the price gap is the *reason* for non-prescribing, discounting destroys value.)*
3. "The survey says 52% intend to prescribe. Why is your forecast only 2% penetration in Y1?" *(Because penetration is measured against 2.03 M untreated chronic patients, not against physicians; the plan requires only 824 prescribing physicians — the conservatism is deliberate.)*
4. "What if chronic migraine is 30% of eligible patients rather than 20%?" *(Target pool rises to 3.04 M untreated chronic; Y1 revenue rises to $584 M and break-even moves into Y2 — but the weathervane is that population estimates rarely move that much, so treat it as upside, not base case.)*
5. "How would you present this in one slide?" *(Title: "Enter, but stage it: break-even mid-Year 3 on conservative penetration; access — not demand — is the risk" with three bullets: size, economics, gating metrics.)*

---

## §16.5 Self-score

| Dimension | 1–5 |
|---|---|
| Structure stated before calculation | |
| Units and bases correct throughout | |
| Arithmetic correct (check against §16.3) | |
| Findings prioritised (three, quantified) | |
| Recommendation committed and conditional | |
| Assumptions explicit | |
| Risks named with magnitudes | |
| Next steps actionable | |

Rubric detail: [17-mock-interview-evaluation-rubric.md](17-mock-interview-evaluation-rubric.md). Traps catalogue: [19-errors-mistakes-and-traps.md](19-errors-mistakes-and-traps.md).

**Next:** [17-mock-interview-evaluation-rubric.md](17-mock-interview-evaluation-rubric.md)
