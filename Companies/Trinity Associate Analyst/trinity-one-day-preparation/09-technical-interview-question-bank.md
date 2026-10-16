# 09 — Technical Interview Question Bank

> **Learning objectives:** answer any of ~50 likely questions in 30–60 seconds each, with a model answer, while recognizing the weak answer the interviewer hears all day.
> **Estimated study time:** 45 min skim — read the ⭐ marked questions aloud, then re-read your weak spots.
> **Format per question:** *Tests → Model answer → Common weak answer → Possible follow-up.*
> **Source note:** all questions are **PRACTICE** (original) unless tagged **[CR]** = candidate-reported *theme* (unverified public reports: Excel lookups/aggregation, pivots, trend interpretation, case discussion). No specific question is confirmed. Difficulty tags: **E** easy · **M** medium · **C** challenging.

---

## §9.1 Excel (expect the most questions here)

**Q1 ⭐ M — "SUMIFS or VLOOKUP — when do you use which?"**
*Tests:* tool-to-task fit. *Model:* SUMIFS aggregates values that meet conditions (TRx by region); VLOOKUP/XLOOKUP retrieves an attribute for a key (therapeutic area for a product). One rolls up; the other maps across. *Weak:* "I only use VLOOKUP." *Follow-up:* "Count reps above 90% attainment" → COUNTIFS on a computed achievement column.

**Q2 ⭐ M — "VLOOKUP vs XLOOKUP vs INDEX-MATCH?"**
*Tests:* depth beyond one function. *Model:* XLOOKUP is modern — looks left, no column index, built-in not-found. VLOOKUP is legacy — exact match needs `FALSE`, breaks on inserted columns, can't look left. INDEX-MATCH works everywhere and is robust — the safe answer when unsure of version. *Weak:* "They all do the same thing." *Follow-up:* "What happens if you forget FALSE?" → approximate match, silently wrong values.

**Q3 E — "Relative vs absolute references?"**
*Tests:* whether you've actually dragged a formula. *Model:* `A1` shifts when copied; `$A$1` is frozen; mixed `$A1`/`A$1` locks one axis. Freeze lookup tables (`Master!$A$2:$C$4`) and criteria cells. *Weak:* "The dollar sign makes it absolute" with no when/why. *Follow-up:* Show a formula that breaks without `$` and say why.

**Q4 ⭐ M [CR] — "Cell contains `" 1,240 "` — it won't sum. Fix it."**
*Tests:* real-world cleaning. *Model:* text-number: use `=VALUE(SUBSTITUTE(TRIM(A2),",",""))`; check with `ISTEXT()`; alternative: Text-to-Columns. Mention that a whole column of such values makes SUM silently skip rows. *Weak:* "Delete the cell and retype." *Follow-up:* "How would you *find* all affected cells?" → `SUMPRODUCT(--NOT(ISNUMBER(range)))` count.

## §9.2 Data interpretation

**Q5 ⭐ M — "TRx grew 8% but share fell 1.2 pp. What happened?"**
*Tests:* absolute vs relative thinking. *Model:* the market grew faster than 8% (share grows only if brand growth > market growth). Brand is expanding but underperforming the market. *Weak:* "The brand shrank" — contradicted by +8%. *Follow-up:* "If starting share was 12%, where did it land?" → roughly 11.6% (12 × 1.08/1.12).

**Q6 M — "Monthly values: 100, 102, 99, 104, 450, 106. What do you do with 450?"**
*Tests:* anomaly discipline. *Model:* flag and verify — data entry error, unit change, or a genuine one-off event; investigate *before* using it; never silently delete; report both with/without if it materially changes conclusions. *Weak:* "Delete it — it's an outlier." *Follow-up:* "If it was a real one-off, how do you forecast next month?" → exclude from trend baseline, note it.

**Q7 M — "Quarterly revenue: 120, 150, 135, 195. Give me the story in one line."**
*Tests:* narrative from numbers. *Model:* grew to Q2 (+25%), dipped 10% in Q3, then a strong Q4 high — net +62.5% Q4 vs Q1; average 150/quarter. *Weak:* reading numbers back without comparisons. *Follow-up:* "What would you check before presenting?" → seasonality, one-off drivers, comparability of quarters.

## §9.3 Statistics

**Q8 ⭐ M — "Mean vs median — when does it matter?"**
*Tests:* distribution awareness. *Model:* when the tail is skewed — high-decile prescribers pull the mean up; the median describes the typical physician, the mean the total economics. Report both when they diverge. *Weak:* "They're both averages." *Follow-up:* "Prescriptions per physician: 4, 7, 7, 9, 13 — compute both." → mean 8, median 7.

**Q9 ⭐ M — "What does ±6 percentage points mean?"**
*Tests:* uncertainty literacy. *Model:* from a sample of 250 with 60% awareness, the true population value plausibly lies within 53.9–66.1% at 95% confidence; it captures sampling error, not bias. *Weak:* "The answer might be wrong." *Follow-up:* "How do you shrink it?" → 4× the sample halves it (MoE ∝ 1/√n).

**Q10 M — "Awareness 60% vs 54%, n=250 each. Real difference?"**
*Tests:* significance instinct. *Model:* difference is 6 pp; two-group z ≈ 1.36, p ≈ 0.18 → not significant; the interval covers zero — call them equivalent in this wave; suggest a larger sample before segment-targeted action. *Weak:* "6 is bigger than 5, so yes" or invoking p-values incorrectly. *Follow-up:* "What difference size with n=400 would be significant?" → ~7–8 pp.

**Q11 M — "Rep calls correlate with TRx. Should we add calls?"**
*Tests:* causality discipline. *Model:* correlation ≠ causation — high-potential territories may receive both more calls and more sales (confounding); test with a pilot/control or analyze within-territory changes. *Weak:* "Correlation means it works — scale it." *Follow-up:* "Design the test." → matched territories, one group gets added calls, pre-register the metric window.

## §9.4 Survey design

**Q12 ⭐ M — "Write a question to measure prescribing intent for a new drug."**
*Tests:* survey craft. *Model:* specific, behavioral, with timeframe and scale: *"In the next 3 months, how likely are you to prescribe [drug] to eligible patients? (5-point: very unlikely → very likely)"*; add a behavioral cross-check (current prescriptions) because stated intent overstates behavior. *Weak:* "Would you prescribe our new drug? Yes/No." (leading, no window, no scale). *Follow-up:* "How do you convert intent to a forecast?" → calibrate against historical intent-vs-actual ratios.

**Q13 E — "What's wrong with: 'Don't you agree once-weekly dosing is much more convenient?'"**
*Tests:* bias detection. *Model:* leading (presumes agreement), loaded ("much"), no comparison anchor. Neutral rewrite: *"Compared with daily dosing, how would you rate the convenience of once-weekly dosing?"* on a balanced scale. *Weak:* "It's fine if respondents agree." *Follow-up:* "Name two more biases." → social desirability, nonresponse.

**Q14 M — "What is nonresponse bias, why care in physician surveys?"**
*Tests:* sample thinking. *Model:* responders differ from non-responders — busy high-prescribers often skip surveys, so results skew toward the engaged; check response rate, compare responders vs frame on known variables, weight if needed. *Weak:* "Low response just means smaller n." (No — it can mean *biased* n.) *Follow-up:* "How do you boost response?" → shorter survey, incentives, mixed modes, follow-ups.

## §9.5 Power BI

**Q15 ⭐ M — "What is Power Query?"**
*Tests:* ETL understanding. *Model:* the built-in transform step — connect, then visibly record cleaning steps (trim, type conversion, remove duplicates, unpivot); steps re-run on refresh = repeatable, auditable cleaning. *Weak:* "It's where the data is." *Follow-up:* "Why is recorded transformation better than manual cleaning?" → reproducibility + refresh-safe.

**Q16 ⭐ M — "Measure vs calculated column?"**
*Tests:* the #1 Power BI concept. *Model:* measure = aggregate computed at display time under the current filters (Total TRx, % attainment); calculated column = computed row-by-row at refresh, stored (e.g., achievement band per rep). Use measures for KPIs; columns for row attributes or slicers. *Weak:* "Both are formulas." *Follow-up:* "Which one for '% of total by product'?" → measure.

**Q17 M — "What does CALCULATE do?"**
*Tests:* DAX depth (awareness is fine). *Model:* evaluates a measure with filters modified — e.g., CardioX TRx inside a total, or share-of-product: `DIVIDE(CALCULATE([Total TRx], product="CardioX"), [Total TRx])`. *Weak:* "Some advanced function." *Follow-up:* "Why DIVIDE not `/`?" → handles divide-by-zero safely.

**Q18 M — "How would you build the monthly TRx dashboard — and keep it trustworthy?"**
*Tests:* end-to-end thinking. *Model:* connect CSV → clean in Power Query (trim regions, numeric TRx, dedupe) → model relationships (rep, product) → measures (Total TRx, attainment, MoM) → KPI cards + region/product visuals + slicers → scheduled refresh; QC: cross-foot every cut to the same 23,113 total, blank-count card, freshness stamp. *Weak:* jumps straight to visuals. *Follow-up:* "What breaks on refresh?" → schema changes; typed columns going text again.

## §9.6 Tableau

**Q19 ⭐ M — "Dimensions vs measures?"**
*Tests:* core Tableau concept. *Model:* dimensions slice/group qualitative data (Region, Product); measures are numeric and get aggregated (SUM(TRx)); dropping a dimension on color splits bars — the standard move. *Weak:* "Left vs right pane." *Follow-up:* "A rep ID — dimension or measure?" → dimension (identifier, even if numeric).

**Q20 E — "Discrete vs continuous (basic)?"**
*Tests:* awareness. *Model:* discrete = separators/headers (each category its own bar); continuous = an axis (trend lines); dates switched between the two behave differently — continuous for trends. *Weak:* "Same as dimension/measure." *Follow-up:* "Month-by-month line — which?" → continuous date.

**Q21 M — "Client lives in Excel/SharePoint. Power BI or Tableau — and why?"**
*Tests:* judgment, not fandom. *Model:* Power BI — native Microsoft stack, refresh from SharePoint/Excel, cheaper with existing licenses; Tableau if exploration-heavy visual work is the priority. Skill transfers; justify by ecosystem. *Weak:* "Tableau is prettier, so Tableau." *Follow-up:* "What would change your mind?" → non-Microsoft warehouse + visual-first culture.

## §9.7 SQL

**Q22 ⭐ E — "WHERE vs HAVING?"**
*Tests:* aggregation order. *Model:* WHERE filters rows before grouping; HAVING filters groups after aggregation (it can see SUM/COUNT). `SELECT product, SUM(trx) ... GROUP BY product HAVING SUM(trx) > 5000;` *Weak:* using WHERE with SUM(trx). *Follow-up:* "Why does the WHERE version error?" → aggregates aren't available pre-grouping.

**Q23 M — "INNER vs LEFT JOIN — and a use for each?"**
*Tests:* join literacy. *Model:* INNER keeps matches only; LEFT keeps all left rows with NULLs where unmatched — use LEFT + `WHERE right.id IS NULL` to find reps with zero sales. *Weak:* "LEFT is safer, always use it." *Follow-up:* "What does a NULL in the right table mean here?" → no matching activity, not zero necessarily.

**Q24 M — "How do you handle NULLs — carefully?"**
*Tests:* skepticism. *Model:* `IS NULL`/`IS NOT NULL`, `COALESCE(x,0)` to substitute; but filling missing TRx with 0 asserts "sold nothing" — sometimes wrong; often better to exclude and state the count. Always report how many rows were affected. *Weak:* "Replace all nulls with zero." *Follow-up:* "Where would that bias a result?" → averages and attainment rates.

**Q25 M — "Find duplicate rows in SQL."**
*Tests:* QC pattern. *Model:* `SELECT rep_id, month, COUNT(*) FROM sales GROUP BY rep_id, month HAVING COUNT(*) > 1;` then decide delete/keep by evidence — exact duplicates can be real repeat rows; verify with all columns. *Weak:* "SELECT DISTINCT and move on." *Follow-up:* "Same idea in Excel?" → Remove Duplicates or COUNTIFS flag column ([`03` §3.6](03-excel-complete-revision.md)).

## §9.8 Python & automation

**Q26 ⭐ M — "Excel or Python — when?"**
*Tests:* tool judgment. *Model:* Excel for one-off, small, stakeholder-shared analysis; Python for recurring multi-file cleaning/merges where steps repeat monthly and audits matter; SQL when data lives in a warehouse; BI for recurring monitoring. *Weak:* "Python is better, always Python." *Follow-up:* "Weekly merge of 40 branch files?" → Python loop + template.

**Q27 M — "A column has commas and blanks — pandas plan?"**
*Tests:* real cleaning steps. *Model:* `pd.to_numeric(s.astype(str).str.replace(",", ""), errors="coerce")` → blanks/strings become NaN; then decide per column: fill (median) only if justified, else exclude and count. State the decision. *Weak:* `fillna(0)` reflex. *Follow-up:* "How do you report it?" → "n rows excluded; blanks counted before/after."

**Q28 C — "Automate a recurring report end-to-end — describe the pipeline."**
*Tests:* process-improvement thinking (JD language). *Model:* SQL extracts raw tables → scheduled script cleans/merges/validates (row counts, cross-foots) with pandas → writes to a shared location → BI dashboard refreshes → exception log emails the analyst only when checks fail; humans do interpretation, not copy-paste. *Weak:* "I'd do it faster in Excel each time." *Follow-up:* "What do you validate?" → totals vs source, duplicates, blank counts, deltas vs last period.

## §9.9 Market research

**Q29 E — "Qual vs quant — which first?"**
*Tests:* research instinct. *Model:* qual (interviews) to discover *why* and generate hypotheses; quant (surveys) to measure *how many/how much*; sequence: qual → survey design → quant → act. *Weak:* "Surveys are more scientific." *Follow-up:* "Give a pharma example." → FIEs reveal access frustration → survey quantifies its prevalence.

**Q30 M — "Custom vs syndicated research?"**
*Tests:* vocabulary. *Model:* custom = client-specific questions/fielding (your exact segment, your exact question); syndicated = multi-client panels/audits with benchmarks, faster and cheaper but fixed questions. Best practice: use syndicated for context, custom for decisions. *Weak:* "Buy whichever is cheaper." *Follow-up:* "Where would you benchmark first?" → syndicated.

**Q31 ⭐ M — "Survey done — how do you turn responses into insights?"**
*Tests:* the actual job. *Model:* clean + check n → distributions and top-2-box → rank drivers/barriers → segment cross-tabs → the so-what with an action and metric → limitations (self-report, sample). Deliverable: 3 insight-led bullets, not 30 charts. *Weak:* "Make a deck with all the charts." *Follow-up:* "What's the first chart?" → the one that answers the business question (e.g., funnel: awareness → trial → regular use).

## §9.10 Pharma commercial analytics

**Q32 ⭐ E — "TRx vs NRx vs NBRx?"**
*Tests:* domain fluency. *Model:* TRx total scripts; NRx new prescriptions (new patients); NBRx new-to-brand (new to *this* brand — the launch metric). *Weak:* mixing NRx and NBRx. *Follow-up:* "Which moves first after a launch push?" → NBRx/NRx before TRx (stock of chronic patients fills later).

**Q33 M — "What is LoE and how does it change forecasting?"**
*Tests:* lifecycle thinking. *Model:* loss of exclusivity — patents lapse, generics/ biosimilars enter, net price and share collapse within quarters; forecasts become erosion curves (fast/slow by category), and strategy shifts to switching patients and lifecycle extensions. *Weak:* "When the drug gets old." *Follow-up:* "What does the commercial team do 18 months pre-LoE?" → patient switching, contracts, follow-on formulations.

**Q34 M — "Survey says 43% intent to prescribe at launch. Forecast?"**
*Tests:* stated-vs-revealed behavior. *Model:* discount intent — historically a substantial gap between intent and behavior as access settles; triangulate with analogues, constraint-check (formulary?), and frame as a range (e.g., base case ≈ 30% trial). *Weak:* "43% of physicians will prescribe." *Follow-up:* "What raises confidence?" → access status, analogue launches, subsequent survey wave.

## §9.11 Market-entry cases

**Q35 ⭐ C — "Size the market for a new T2D drug in a country."**
*Tests:* structured estimation. *Model:* funnel — adults × diagnosed share × treated × eligible for class × expected penetration × annual net price; state each assumption as a range; sanity-check against a known anchor ("national diabetes drug spend ≈ $X B — my number must sit within an order of magnitude"). *Weak:* "Google says the market is $5B." *Follow-up:* "Penetration drops 15% → 10%: revenue impact?" → −33% for that branch (linear in penetration).

**Q36 M — "PharmaCo has a mid-size drug declining 12% QoQ, concentrated in 3 territories. Diagnose."**
*Tests:* decomposition. *Model:* size and split the decline (3 of 24 territories = 60% of it) → hypotheses: coverage (vacancies/calls −29%), competition (local launches), access (formulary change) → check each with data → fix narrowly (fill roles, reallocate calls, counter detailing) → track weekly. *Weak:* "Run a national campaign to stop the decline." *Follow-up:* "What metric proves the fix?" → affected-territory TRx vs a stable control set.

## §9.12 Graph interpretation

**Q37 ⭐ M — "A bar chart's y-axis starts at 95 instead of 0. Problem?"**
*Tests:* visualization ethics. *Model:* exaggerates small differences; acceptable in some financial charts *if clearly labeled*, but for share/volume comparisons default to zero; if truncation changes the story, the story may not exist. *Weak:* "Whatever looks better." *Follow-up:* "When is truncation defensible?" → index/return series where 0 is meaningless, labeled clearly.

**Q38 E — "Pie chart with 8 slices — improve it."**
*Tests:* chart choice. *Model:* switch to a sorted horizontal bar; pies fail beyond ~3–5 slices (angle perception); if part-of-whole matters, a stacked bar or treemap with the top 3 + "Other". *Weak:* "Use different colors." *Follow-up:* "Best chart for 2 segments over 12 months?" → stacked column/area, or two lines if comparing trends ([`05` §5.6](05-power-bi-tableau-and-data-visualization.md)).

**Q39 M — "Two lines on a dual-axis chart seem to move together — conclusion?"**
*Tests:* misleading-viz vigilance. *Model:* dual axes can manufacture apparent relationships (scales chosen to align); verify the correlation numerically, avoid the chart for causal claims, prefer indexed series (both = 100 at start). *Weak:* "They're correlated, so one drives the other." *Follow-up:* "Index the series — what changes?" → slopes compare fairly on one scale.

## §9.13 Brain teasers (structure > speed)

**Q40 C — "Estimate the number of physicians in India."**
*Tests:* Fermi discipline. *Model:* population ≈ 1.4B → divide by ~1,500 people per physician (density ≈ 0.7/1,000) ≈ **0.9–1.0M physicians**; refine by rural/urban and by specialty share if relevant. Say the range and the assumption; you're tested on structure, not the exact census number. *Weak:* random guess with no chain. *Follow-up:* "How many cardiologists?" → apply specialty share (say ~1%) ≈ 9–10k (state assumption).

**Q41 E — "Bat and ball cost ₹110 together. The bat costs ₹100 more than the ball. Ball's price?"**
*Tests:* snap-answer resistance. *Model:* ball = ₹5 (bat = ₹105; sum 110, difference 100). The intuitive ₹10 fails the difference check. *Weak:* "₹10." *Follow-up:* "How did you verify?" → substitute back into both conditions.

**Q42 M — "₹400 for sure vs 50% chance of ₹1,000 — pick one (risk-neutral)."**
*Tests:* expected value thinking. *Model:* EV of gamble = 0.5 × 1,000 = ₹500 > 400 → take the gamble if the decision repeats and you're risk-neutral; if it's one-shot and large relative to your budget, risk aversion is legitimate — state the criterion. *Weak:* "Always take the sure thing." *Follow-up:* "What certainty would you accept instead of the gamble?" → the certainty-equivalent question (≥ ₹400 for risk-neutral).

## §9.14 Resume-based

**Q43 ⭐ M — "Walk me through your resume (60–90 sec)."**
*Tests:* clarity and relevance. *Model:* present → past → why: current (B.Tech IT 2027, MSIT; projects in Python/SQL/backend) → evidence (2–3 proof points with real numbers from your own projects — never invent) → why Trinity/analyst role (data + business decisions; survey analytics + dashboard building matches your programming-plus-analysis profile). *Weak:* chronology of every semester. *Follow-up:* "Which project best shows analytical rigor?" — have your own project facts ready.

**Q44 M — "A project where you used data to decide something."**
*Tests:* real experience. *Model:* STAR with a metric — Situation (problem), Task, Action (what data, what you computed), Result (number changed, what you'd do differently). *Weak:* describing tech stack with no outcome. *Follow-up:* "What would you measure next time?" → leading indicator.

**Q45 M — "Why commercial analytics as a B.Tech student?"**
*Tests:* motivation coherence. *Model:* your strength is building things with data (SQL/Python/automation), and this role is that at industrial scale *with* business ownership — the JD's process-improvement + dashboards + survey analytics is the intersection of what you do well and what excites you; Trinity's scale (client-facing decisions impacting patients) makes it concrete. *Weak:* "I like Excel and want a job." *Follow-up:* "Where do you want to be in 3 years?" → analyst owning a survey/dashboard pipeline end-to-end (align with JD).

## §9.15 Quality control & attention to detail

**Q46 ⭐ M — "How do you check your own work before it goes out?"**
*Tests:* the JD's *explicit* attention-to-detail requirement (CONFIRMED). *Model:* cross-foot totals (every cut sums to the grand total — like 23,113 in Mock B), recompute two cells independently, check units and decimals, verify lookups fail-zero, confirm n/denominators, re-read the question, state exclusions. *Weak:* "I re-read it." *Follow-up:* "You have 5 minutes for QA on a big model — what first?" → totals and denominators.

**Q47 M — "You spot an error in a report you already sent."**
*Tests:* integrity. *Model:* verify → flag immediately and plainly (impact quantified: which cells, which decisions affected) → fix and resend with a change note → add a check that would have caught it. No quiet edits. *Weak:* "Fix it quietly and hope nobody noticed." *Follow-up:* "How do you prevent recurrence?" → checklist + automation of the failing check.

## §9.16 Behavioral & stakeholder communication (STAR ready)

**Q48 ⭐ M — "Explain margin of error to a non-technical stakeholder."**
*Tests:* translation. *Model:* "If we surveyed all physicians instead of 250, the number would likely sit within about 6 points of 60 — six in a hundred either side. Conclusions this far from the boundary are safe; segments built on 30 responses aren't." *Weak:* reciting the formula. *Follow-up:* "How would you increase precision?" → 4× sample halves the margin.

**Q49 M — "What makes a finding an 'insight'?"**
*Tests:* judgment. *Model:* a fact + magnitude + relevance + implied action: "reimbursement blocks 34% of non-prescribers → fixing access could unlock the largest pool — test it in the 3 uncovered regions." A number alone is data, not insight. *Weak:* "It's something interesting." *Follow-up:* "Weakest part of your insight?" → assumed causality, ask for a test.

**STAR story bank (prepare 4; behavioral round is CONFIRMED 30 min).** ① teamwork conflict → resolution; ② deadline crunch → prioritization; ③ attention-to-detail catch → impact; ④ explaining something technical to a nontechnical person. Each: 30–45 sec, end with the result number and a lesson.

**Q50 ⭐ E — "Any questions for us?"**
*Tests:* genuine interest. Ask two: *"How does the Associate Analyst role grow from survey analysis into owning client deliverables over the first year?"* and *"What tooling is standard on the team — Power BI or Tableau heavy, and how much Python automation is real vs aspirational?"* Avoid: salary, leaves, "nothing".

---

*All model answers above are original (**PRACTICE**) and can be shortened or adapted — say the first sentence, then expand only if asked.* **Next:** [10 — Mock assessment & Excel test](10-mock-assessment-and-excel-test.md)
