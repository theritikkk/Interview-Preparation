# 02 — Aptitude, Reasoning & Data Interpretation

> **Learning objectives:** compute and interpret the 10 numbers business analysts argue about all day (percentages, ratios, weighted averages, growth, margins, probability, share); read any table or chart; spot trends and anomalies; state conclusions that the data actually supports.
> **Estimated study time:** 55 min (25 min reading + 25 min timed set + 5 min key).
> **Priority:** **MUST KNOW** §§2.1–2.5 and 2.9; **SHOULD KNOW** the rest.
> **Source:** **PRACTICE** — question style mirrors standard analyst campus assessments. The real distribution is **not confirmed**.

Calculator discipline: for the real proctored test, use only what the rules allow. Practice mental shortcuts: 10% = shift decimal; 5% = half of 10%; 15% = 10% + 5%; 25% = ÷4; 20% = ÷5.

---

## §2.1 Percentages and percentage changes — MUST KNOW

- **Rule:** change % = (new − old) ÷ old × 100. "x is y% of z" ⇔ x = y/100 × z.
- **Percentage points (pp) vs percent (%):** a move from 46% to 52% is **+6 pp** but **+13.0% relative** (6 ÷ 46). Exams test this distinction constantly.

**Worked example.** Revenue 850 → 918: change = 68 ÷ 850 = **+8%**. Awareness 46% → 52%: **+6 pp**, or **+13.0%** relatively.

**Traps.** ① Computing % change against the *new* value. ② Saying "+6%" for a 46→52 pp move. ③ Combining successive changes additively: +10% then +10% = +21%, not +20% (1.1 × 1.1 = 1.21).

**Practice P1.** A price rises 20% then falls 20%. Net change? *(Answer §2.12)*

## §2.2 Ratios, proportions, and simple averages — MUST KNOW

- **Rule:** a:b divides a total into parts a/(a+b) and b/(a+b). Average = sum ÷ count.
- **Proportion change:** to keep a ratio while scaling, multiply both sides equally.

**Worked example.** Ratio 3:5 of 320 → parts 320 × 3/8 = 120 and 320 × 5/8 = **200**. Average of 12, 18, 24, 30 = 84/4 = **21**.

**Traps.** ① Splitting a total as a/b instead of a/(a+b). ② Averaging percentages of different-sized groups (that's §2.3's job). ③ Forgetting that averages are pulled by extremes.

**Practice P2.** Split ₹500 in the ratio 3:7; what is the smaller share? *(Answer §2.12)*

## §2.3 Weighted averages — MUST KNOW

- **Rule:** weighted avg = Σ(weight × value) ÷ Σ(weights). With proportions: 0.4 × v₁ + 0.6 × v₂.
- **Intuition:** when group sizes differ, the bigger group dominates. A simple average of group averages treats a 10-person group like a 1,000-person group — wrong.

**Worked example.** 40% of patients use 20 units/month, 60% use 45 → weighted = 0.4 × 20 + 0.6 × 45 = 8 + 27 = **35**. The simple average (32.5) understates it because the heavier group uses more.

**Traps.** ① Averaging two group percentages equally when n₁ ≠ n₂. ② Using survey-weighted data vs raw counts: official statistics are usually weighted; unweighted can mislead.

**Practice P3.** Segment A (70% of respondents) scores 60; segment B (30%) scores 100. Weighted mean? *(Answer §2.12)*

## §2.4 Growth rates and CAGR — MUST KNOW

- **Simple growth (period over period):** (this ÷ previous) − 1.
- **CAGR** (compound annual growth rate): `CAGR = (End ÷ Start)^(1/n) − 1`, n = number of *years/periods between* valuations.
- **Rule of thumb:** doubling in 10 years ≈ 7.2% CAGR (72 ÷ 10).

**Worked example.** Revenue 100 → 144 over 2 years: (144/100)^(1/2) − 1 = 1.2 − 1 = **20%**. Quarterly, Q1 120 → Q2 150 = **+25%** (no compounding needed for a single step).

**Traps.** ① Dividing total growth by n (144/100 = 44%; 44/2 = 22% ≠ 20%). ② Off-by-one on n: 2023 → 2026 is **3** growth periods, not 4. ③ Mixing quarterly and annual rates — always label the period.

**Practice P4.** Revenue grows 125 → 180 over 3 years. CAGR (1 dp)? *(Answer §2.12)*

## §2.5 Revenue, costs, and margins — MUST KNOW

- **Gross margin** = (Revenue − COGS) ÷ Revenue. **Operating margin** = Operating profit ÷ Revenue.
- **Markup** = profit ÷ **cost**; **margin** = profit ÷ **revenue**. Same profit, different denominators.
- **Break-even volume** = fixed costs ÷ contribution margin per unit (price − variable cost).

**Worked example.** Revenue 850, COGS 578 → margin = 272/850 = **32%**. The same 272 as a markup on cost = 272/578 = **47.1%** — say "margin" or "markup" precisely.

**Traps.** ① Swapping margin and markup. ② Applying margin to cost ("32% of cost" instead of revenue). ③ Forgetting that discounting cuts margin disproportionately: a 10% price cut on a 32%-margin product removes ~31% of the profit.

**Practice P5.** Revenue 1,250, total cost 1,000. Margin? *(Answer §2.12)*

## §2.6 Basic probability — SHOULD KNOW

- **Rule:** P = favourable ÷ total (equally likely outcomes). Independent events: multiply. P(A and B) = P(A) × P(B|A).
- **Complement:** P(not A) = 1 − P(A).

**Worked example.** 3 of 10 vials are defective → P(defective) = **30%**. Two physicians sampled (no replacement) from 5 where 2 are aware → P(both aware) = 2/5 × 1/4 = **10%**.

**Traps.** ① Adding probabilities of overlapping events. ② Sampling *with* replacement when the question implies without (2/5 × 1/4, not 2/5 × 2/5). ③ Confusing "at least one" (use 1 − P(none)) with "exactly one".

**Practice P6.** 3 of 25 samples fail QC. Probability a random sample fails? *(Answer §2.12)*

## §2.7 Number sequences and logical reasoning — MUST KNOW

- **Sequences:** look for differences, ratios, alternating patterns, squares/cubes, primes.
- **Syllogisms:** only "must be true" conclusions count. "All analysts use Excel" ⇒ "some Excel users are analysts" (valid); "some analysts dislike pivots" (invalid).
- **Conditionals:** "If rain → calls drop" logically gives the *contrapositive* "no drop → no rain". It does NOT give "calls drop → rain".

**Worked example.** 3, 6, 12, 24, **48** (×2). 2, 5, 10, 17, 26, **37** (+3, +5, +7, +9, +11 — differences grow by 2).

**Traps.** ① Assuming arithmetic when the pattern is multiplicative (or ×2 then +2 alternating). ② Over-inferring from "some" statements. ③ In 3-person arrangements, forgetting constraints combine.

**Practice P7.** 1, 4, 9, 16, 25, … next? *(Answer §2.12)*

## §2.8 Tables and charts (data interpretation) — MUST KNOW

- **Table reading routine:** read titles + units → identify the asked comparison → compute → sanity-check magnitude.
- Growth between non-adjacent periods uses the same formula: (later − earlier) ÷ earlier.
- **Average of a series** = total ÷ count (rewrite the whole table into one number, then divide).

**Worked example.** Quarterly revenue 120, 150, 135, 195 (₹ Cr): total 600, average **150**; Q4 vs Q1 = 75/120 = **62.5%**; Q3 vs Q2 = 135/150 − 1 = **−10%**.

**Traps.** ① Comparing a quarter to the wrong base. ② Reading the axis scale wrongly (tick marks ≠ values). ③ Averages of ratios are not ratios of totals ("average growth" ≈ wrong).

**Practice P8.** Quarterly revenue 90, 110, 105, 130: (a) average, (b) Q4 vs Q1 growth? *(Answer §2.12)*

## §2.9 Market share and segment contribution — MUST KNOW

- **Share** = brand ÷ market × 100. **Segment contribution** = segment ÷ total × 100.
- **Share change is stated in pp:** 12% → 10.4% is "down 1.6 pp".
- **Share math:** if the brand grows 8% and the market grows 12%, the share must fall (brand growth < market growth), regardless of absolutes.

**Worked example.** Brand 480 in a 4,000 market → **12%**. If the market grows 12% but the brand only 8%: new shares = (480 × 1.08)/(4000 × 1.12) = 518.4/4480 = 11.57% → **−0.43 pp**.

**Traps.** ① Saying share "fell 0.43%" (it's pp). ② Computing share against the *segment* instead of the market when the question asks for overall share. ③ Summing shares across non-comparable scopes.

**Practice P9.** Brand sales 156 in a 1,200 market. Share? *(Answer §2.12)*

## §2.10 Trends, anomalies, and conclusions — MUST KNOW

- **Trend:** direction + magnitude + period ("TRx rose 12% QoQ for two quarters").
- **Anomaly:** a value inconsistent with the pattern (e.g., 100, 102, 99, 104, **450**, 106). Rule: **flag it, don't delete it** — verify (data error vs real event) before using it.
- **Conclusion discipline:** only say what the data supports. "Sales fell in Q3" (fact) ≠ "the price cut caused the dip" (needs evidence).

**Worked example.** TRx +8% while share fell 1.2 pp ⇒ correct conclusion: *"the brand grew, but the market grew faster — a relative underperformance despite absolute growth."* The tempting wrong conclusion: "the brand is shrinking" (absolute numbers say otherwise).

**Traps.** ① Calling share *growth* "growth of the brand" — check absolutes. ② Treating correlation as causation ("prices rose when sales rose → price raise worked" — maybe; maybe the market grew). ③ Concluding from a single data point.

**Practice P10.** Brand grew 6%, market grew 12%. What happened to share — and by roughly what pp if the starting share was 10%? *(Answer §2.12)*

---

## §2.11 Mixed timed set (25 minutes, no calculator where possible)

> **PRACTICE.** Aim ≥ 10/12 before the real thing.

**M1.** 18% of 350 = ?
**M2.** A value goes 400 → +15% → −10%. Final value and net % change from 400?
**M3.** ₹640 split 7:9. Larger share?
**M4.** Average of 5 numbers is 32. A 6th number, 50, is added. New average?
**M5.** 60% of respondents score 75; 40% score 45. Weighted mean?
**M6.** Revenue 200 → 242 over 2 years. CAGR?
**M7.** Revenue 960, cost 720. Margin?
**M8.** 3 of 25 samples fail. Failure probability?
**M9.** Brand sales 91; market 700. Share?
**M10.** Monthly reads: 50, 52, 51, 53, 210, 54. What do you do with 210?
**M11.** Market grew 5%, our brand 9%. Did we gain or lose share?
**M12.** Awareness went 46% → 52%. State the change in pp and in %.

### Answer key — mixed set

| Q | Answer | Note |
|---|---|---|
| M1 | **63** | 350 × 0.18 |
| M2 | **414; +3.5%** | 400 × 1.15 × 0.90 = 414; 14/400 |
| M3 | **360** | 640 × 9/16 |
| M4 | **35** | (160 + 50) ÷ 6 |
| M5 | **63** | 0.6×75 + 0.4×45 = 45 + 18 |
| M6 | **10%** | √(242/200) − 1 |
| M7 | **25%** | 240 ÷ 960 |
| M8 | **12%** | 3/25 |
| M9 | **13%** | 91 ÷ 700 |
| M10 | **Flag & verify before using** | Don't silently delete; check for data entry error or one-off event |
| M11 | **Gained** | Brand growth (9%) > market growth (5%) → share rises |
| M12 | **+6 pp; +13.0%** | 6/46 for the relative version |

---

## §2.12 Practice answers (P1–P10)

| Q | Answer | Explanation |
|---|---|---|
| P1 | **−4%** | 1.20 × 0.80 = 0.96 |
| P2 | **₹150** | 500 × 3/10 |
| P3 | **72** | 0.7×60 + 0.3×100 = 42 + 30 |
| P4 | **12.9%** | (180/125)^(1/3) − 1 = 1.44^(1/3) − 1 |
| P5 | **20%** | (1250 − 1000) ÷ 1250 |
| P6 | **12%** | 3/25 |
| P7 | **36** | Perfect squares 1², 2², … 6² |
| P8 | **108.75; +44.4%** | 435/4; 40/90 |
| P9 | **13%** | 156 ÷ 1200 |
| P10 | **Share fell ≈ −0.5 pp** | 10% × 1.06/1.12 = 9.46% |

**Next:** [03 — Excel complete revision](03-excel-complete-revision.md) · **Master formulas:** [11 — cheatsheet](11-final-revision-cheatsheet.md)
