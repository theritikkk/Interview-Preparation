# 01 — Quantitative Aptitude for Business Analytics

**Purpose:** build calculation speed and accuracy for the 60-minute proctored assessment and for case/Excel maths. Everything here is business-flavoured: revenue, prescriptions, market share, margins, survey rates.
**Prerequisites:** none (school-level arithmetic is enough).
**Suggested study time:** 4–6 hours (split into 3–4 sittings; do not read it end-to-end in one go).
**How to use:** read a concept box → study one worked example → solve that topic's questions closed-book → check the key at the bottom → log every mistake in [19-errors-mistakes-and-traps.md](19-errors-mistakes-and-traps.md).

> All numbers in this file were computed and cross-checked. Percentages are rounded to 1–2 decimals where needed; choose the closest option in a timed test.

---

## 1.1 Percentages and percentage change

**Core formulas**

| Concept | Formula |
|---|---|
| x% of N | (x/100) × N |
| Percentage change | (New − Old) / **Old** × 100 |
| Reverse (find original) | Original = New / (1 ± change) |
| Successive changes a% then b% | Net multiplier = (1 + a/100)(1 + b/100) |
| Share of total | (Part / Total) × 100 |

**Intuition:** every percentage change is a multiplier. +25% → ×1.25. −15% → ×0.85. Two changes in sequence → multiply the multipliers. This single habit prevents the most common error: *using the new value as the denominator for a change that should be measured from the old value.*

**Worked example 1 — Growth.** A brand's revenue rose from ₹840 Cr to ₹1,050 Cr. Percentage increase?
- Change = 1,050 − 840 = 210
- 210/840 = 0.25 → **+25%**

**Worked example 2 — Discount then price rise.** A product is discounted 15%, then the list price rises 15%. Net effect?
- Multiplier = 0.85 × 1.15 = 0.9775 → **−2.25%** (not 0%). The rise applies to a smaller base.

**Common traps**
- Dividing by the **new** value instead of the old (e.g., 210/1,050 = 20% — wrong).
- Assuming +15% then −15% cancels out.
- Mixing up percentage change with **percentage points** (see [03](03-data-interpretation-and-graphs.md) and [19](19-errors-mistakes-and-traps.md)).

**Practice (Q1–Q6)**
1. A drug's revenue grew from ₹840 Cr to ₹1,050 Cr. What is the % increase?
2. Physician awareness of a brand rose from 28% to 35%. How many percentage points did it rise, and what is the relative (%) increase?
3. A price is cut 15% and later raised 15%. Net change?
4. 18% of 2,400 surveyed physicians were high prescribers. How many physicians is that?
5. After a 20% increase, a market is worth ₹1,140 Cr. What was it worth before?
6. Prescriptions fell from 45,000 to 36,000. What is the % decline?

---

## 1.2 Ratios and proportions

**Core formulas**

| Concept | Formula |
|---|---|
| Ratio a : b, total T | Part1 = T × a/(a+b); Part2 = T × b/(a+b) |
| Scaling | If 1 unit represents k objects, n units = n × k |
| Maintaining a ratio | If b → b′, then a → a × (b′/b) |

**Intuition:** a ratio is a recipe. Whatever you scale one side by, you scale the other side by the same factor. In business work ratios appear as branded:generic splits, staffing ratios (reps : managers), and survey scaling (respondents : population).

**Worked example 1.** Branded:generic market split is 7 : 3 in a ₹2,400 Cr market. Generic value?
- Total parts = 10; generic = ₹2,400 × 3/10 = **₹720 Cr**

**Worked example 2.** Two segments are in ratio 3 : 5; the larger is ₹1,250 Cr. Total market?
- 5 parts = 1,250 → 1 part = 250 → total = 8 parts = **₹2,000 Cr**

**Common traps**
- Adding absolute values to a ratio (e.g., raising a 1:4 mix to "1:3" needs algebra, not just adding 1).
- Dividing the wrong side: always check "does the answer make the bigger share bigger?"

**Practice (Q7–Q11)**
7. Branded:generic = 7 : 3 in a ₹2,400 Cr market. Generic market value?
8. One survey respondent represents 400 physicians. 250 respondents → how many physicians?
9. A 500 ml solution contains 100 ml active ingredient (rest excipient). What is the active concentration? If 100 ml of excipient is added, what is the new concentration?
10. Two segments are in ratio 3 : 5; the larger is ₹1,250 Cr. Total market and smaller segment?
11. A team has 24 analysts and 6 managers (4 : 1). Managers rise to 9; how many analysts keep the ratio?

---

## 1.3 Averages and weighted averages

**Core formulas**

| Concept | Formula |
|---|---|
| Simple average | Sum / Count |
| Weighted average | Σ(wᵢ × xᵢ) / Σwᵢ |
| Combined average of two groups | (n₁x̄₁ + n₂x̄₂)/(n₁ + n₂) |
| Average speed (equal distances) | 2ab/(a+b) (harmonic mean) |

**Intuition:** an average is a *balance point*. A weighted average says "bigger groups pull the average towards themselves". If growth rates are being averaged across products with very different revenue, a simple average is misleading — that is a favourite interview probe.

**Worked example 1 — Weighted vs simple.** 40 reps write 120 Rx each; 60 reps write 180 Rx each.
- Simple average = (120 + 180)/2 = 150 (wrong for business purposes)
- Weighted = (40×120 + 60×180)/100 = (4,800 + 10,800)/100 = **156 Rx/rep**

**Worked example 2 — Weighted growth.** Product A revenue ₹400 Cr growing 12%; Product B ₹600 Cr growing 5%. Portfolio growth?
- (400×12 + 600×5)/1,000 = (4,800 + 3,000)/1,000 = **7.8%** (simple average would wrongly give 8.5%)

**Common traps**
- Simple-averaging percentages across unequal groups.
- Forgetting that removing/adding an item changes the count (denominator).

**Practice (Q12–Q17)**
12. 40 reps write 120 Rx each, 60 reps write 180 each. Weighted average Rx/rep?
13. What does the simple average of those two numbers give, and why is it wrong here?
14. Product A ₹400 Cr at +12%, Product B ₹600 Cr at +5%. Weighted portfolio growth?
15. The average of 5 numbers is 46. One number (26) is removed. New average?
16. Survey satisfaction: group 1 (n = 200) mean 4.2; group 2 (n = 50) mean 3.6. Overall weighted mean?
17. A rep travels 240 km at 60 km/h and 240 km at 80 km/h. Average speed?

---

## 1.4 Profit, revenue, cost and margin

**Core formulas**

| Concept | Formula |
|---|---|
| Gross profit | Revenue − COGS |
| Gross margin % | (Revenue − COGS)/Revenue × 100 |
| Operating profit | Revenue − COGS − SG&A − R&D |
| Operating margin % | Operating profit / Revenue × 100 |
| Contribution | Price − variable cost |
| Break-even units | Fixed cost / contribution per unit |
| Markup vs margin | Margin = Profit/Revenue; Markup = Profit/Cost |

**Intuition:** **revenue is not profit**. Margin answers "of every ₹100 of sales, how much survives after costs?" Pharma has high gross margins but heavy SG&A (sales forces) and R&D, so gross margin can look excellent while operating margin is thin. Explain which margin you are quoting.

**Worked example 1.** Revenue ₹500 Cr, COGS ₹200 Cr, SG&A ₹150 Cr.
- Gross margin = (500 − 200)/500 = **60%**
- Operating margin = (500 − 200 − 150)/500 = **30%**

**Worked example 2 — Break-even.** Fixed launch cost ₹60 Cr; contribution per patient ₹1,500.
- Break-even = 6,00,00,000 / 1,500 = **4,00,000 patients** (4 lakh)

**Common traps**
- Quoting a margin without naming its type (gross vs operating vs net).
- Confusing markup (relative to cost) with margin (relative to revenue): a 25% markup = 20% margin.
- Comparing revenue growth with profit growth and treating them as the same thing.

**Practice (Q18–Q23)**
18. Revenue ₹500 Cr, COGS ₹200 Cr, SG&A ₹150 Cr. Gross margin and operating margin?
19. Gross margin is 70%; revenue ₹1,200 Cr. COGS?
20. Product costs ₹80 and sells for ₹100. Gross margin? What price gives a 25% gross margin at the same cost?
21. Revenue ₹2,000 Cr; marketing is 15% of revenue. If marketing must fall to 12% of revenue, how much is cut?
22. Revenue ₹1,000; COGS ₹600; SG&A ₹250; R&D ₹100. Operating profit and margin?
23. Fixed cost ₹60 Cr, contribution per patient ₹1,500. Break-even patients?

---

## 1.5 Fractions, decimals and quick conversions

**Conversions worth memorising**

| Fraction | Decimal | Percent |
|---|---|---|
| 1/8 | 0.125 | 12.5% |
| 1/6 | 0.1667 | 16.7% |
| 1/7 | 0.1429 | 14.3% |
| 3/8 | 0.375 | 37.5% |
| 5/8 | 0.625 | 62.5% |
| 2/3 | 0.6667 | 66.7% |
| 5/6 | 0.8333 | 83.3% |

**Intuition:** fractions are divisions, and in a timed test converting to decimals makes comparisons fast. Comparing 7/12 vs 11/18 is quicker as 0.583 vs 0.611 than with cross-multiplication.

**Worked example 1.** Order 2/3, 5/8, 7/12, 11/18 from largest to smallest.
- 0.6667, 0.625, 0.5833, 0.6111 → **2/3 > 5/8 > 11/18 > 7/12**

**Worked example 2.** A number is multiplied by 1/4 and then increased by 50%. Net multiplier?
- 0.25 × 1.5 = **0.375** (a 62.5% reduction)

**Common traps**
- Comparing fractions by numerator or denominator alone.
- Losing a factor of 100 between decimals and percentages (0.08 = 8%).

**Practice (Q24–Q28)**
24. Express 3/8 as a percentage and 0.625 as a percentage.
25. What is 5/6 of 1,440?
26. Rank 2/3, 5/8, 7/12, 11/18 from largest to smallest.
27. What is 1/7 as a percentage (1 dp)?
28. A quantity is multiplied by 1/4 then increased by 50%. Express the net change as a multiple of the original.

---

## 1.6 Growth rates and CAGR

**Core formulas**

| Concept | Formula |
|---|---|
| Simple growth | (New − Old)/Old |
| CAGR | (End/Begin)^(1/n) − 1 |
| Compound growth | End = Begin × (1+g)^n |
| Share drift | New share = Old share × (1 + segment growth)/(1 + market growth) |
| Rule of 72 | Years to double ≈ 72 / growth % |

**Intuition:** CAGR is the *constant* annual rate that would take you from start to end. It is a smoothing device, not a prediction. Never average yearly growth rates arithmetically to get CAGR.

**Worked example 1.** Market grows from ₹1,000 Cr to ₹1,331 Cr in 3 years.
- (1,331/1,000)^(1/3) − 1 = 1.10 − 1 = **10% CAGR**

**Worked example 2 — Share drift.** A segment grows 15% while the total market grows 5%. Segment share was 20%.
- New share = 20 × (1.15/1.05) = **21.9%**

**Common traps**
- Using the number of *data points* instead of the number of *intervals* (2021→2025 is 4 intervals, not 5).
- Averaging annual growth percentages to get a CAGR.
- Saying "market grew 20% in 4 years, so 5% per year" — that ignores compounding.

**Practice (Q29–Q34)**
29. ₹1,000 Cr → ₹1,331 Cr in 3 years. CAGR?
30. A ₹640 Cr market grows 25% per year for 2 years. Size after 2 years?
31. Revenue ₹800 Cr → ₹1,250 Cr in 5 years. CAGR (2 dp)?
32. A segment growing 15% inside a market growing 5%. If its share was 20%, what is the new share?
33. Values 100 → 130 → 169 over 2 years. CAGR?
34. At 12% growth, using the rule of 72, how long to double? Compare with the exact figure.

---

## 1.7 Time, speed and work (rates)

**Core formulas**

| Concept | Formula |
|---|---|
| Work rate | Rate = 1/time; combined = 1/a + 1/b |
| Days together | ab/(a+b) |
| Time | Distance / Speed |
| Meeting time | Total distance / sum of speeds |

**Intuition:** treat "work" as a tank being filled. Each resource adds a filling rate. These questions test rate discipline — identical to "how many reps × how many calls to cover a target list".

**Worked example 1.** Task A takes 12 days, task B takes 24 days.
- 1/12 + 1/24 = 3/24 = 1/8 → **8 days together**

**Worked example 2.** Two analysts start 800 km apart at 90 km/h and 110 km/h towards each other.
- Time = 800/(90+110) = 4 hours; they meet 90 × 4 = 360 km from the first analyst.

**Common traps**
- Adding *times* instead of *rates*.
- For a "some work done, then one leaves" question, forgetting to subtract completed work.

**Practice (Q35–Q39)**
35. A does a job in 12 days and B in 24 days. Working together?
36. Two reps process 2,400 records in 3 hours. At the same rate, how long do 4 reps take for 4,800 records?
37. A takes 10 days, B takes 15 days. They work together for 4 days, then A leaves. How long does B take to finish?
38. Two analysts are 800 km apart, moving towards each other at 90 and 110 km/h. When and where do they meet?
39. A rep covers two equal 240 km legs at 60 km/h and 80 km/h. Average speed?

---

## 1.8 Basic probability

**Core formulas**

| Concept | Formula |
|---|---|
| P(event) | Favourable / Total |
| Independent events | P(A and B) = P(A) × P(B) |
| P(at least one) | 1 − P(none) |
| Without replacement | Multiply conditional probabilities |
| Exactly one of two | 2 × p × (1 − p) when independent and symmetric |

**Intuition:** probability questions in assessments are usually counting questions in disguise. If a phrase says "at least one", compute the complement.

**Worked example 1.** 3 questions, each answered correctly with 90% probability, independently. Probability all three correct?
- 0.9 × 0.9 × 0.9 = **72.9%**

**Worked example 2.** A product succeeds with probability 0.35. Two independent products. P(exactly one succeeds)?
- 2 × 0.35 × 0.65 = **45.5%**

**Common traps**
- Forgetting "without replacement" changes the denominator (39, not 40, on the second draw).
- Adding probabilities for "at least one" instead of using the complement.

**Practice (Q40–Q44)**
40. 2 of 10 physicians are early adopters. Two are picked at random (no repeats). P(at least one is an early adopter)?
41. Three survey items, each answered correctly with 90% probability, independently. P(all correct)?
42. A product has a 0.35 chance of success; two independent products. P(exactly one succeeds)?
43. 6 of 40 patients respond to a drug. Two patients chosen without replacement. P(both respond)?
44. Same setup as Q43: P(at least one of two responds)?

---

## 1.9 Estimation and mental arithmetic

**Techniques**
1. **Round the base, adjust the remainder:** 17% of 4,980 = 17% of 5,000 − 17% of 20 = 850 − 3.4 ≈ 847.
2. **Anchor on 10% then scale:** 23.5% of 800 = (20% + 3.5%) of 800 = 160 + 28 = 188.
3. **Use quarter/eighth landmarks:** ×0.25 = ÷4; ×0.125 = ÷8.
4. **Cross-check direction:** if a share rises, the new share must exceed the old one. If your answer breaks an obvious boundary, recompute.

**Worked example 1.** 4,860 × 0.25 = 4,860/4 = **1,215**
**Worked example 2.** 312 / 0.4 = 312 / (2/5) = 312 × 2.5 = **780**

**Common traps**
- Estimating and then reporting the estimate as an exact figure without saying so.
- Losing a scale factor (lakh vs crore vs million).

**Practice (Q45–Q49)**
45. Estimate 17% of 4,980.
46. What is 23.5% of 800?
47. 4,860 × 0.25?
48. 312 / 0.4?
49. 12% of ₹2.5 crore, expressed in lakh?

---

## 1.10 Business numerical reasoning

**Core formulas**

| Concept | Formula |
|---|---|
| Market share | Company revenue / Market revenue |
| Share after growth | Share × (1 + company growth)/(1 + market growth) |
| Funnel volume | Base × conversion₁ × conversion₂ … |
| Revenue per rep | Revenue / Number of reps |
| Sales-force sizing | (Physicians × calls per physician) / calls per rep per year |
| Revenue impact of price/volume | (1 + Δprice%)(1 + Δvolume%) − 1 |
| ROI | (Gain − Spend)/Spend |

**Intuition:** business questions are chains: population → diagnosed → treated → brand share → revenue per patient. Write the chain explicitly in the answer; interviewers score the chain more than the final number.

**Worked example 1 — Share drift.** Company revenue ₹180 Cr in a ₹1,200 Cr market (15% share). Market grows 10%, company 20%.
- New share = 180×1.2 / (1,200×1.1) = 216/1,320 = **16.4%**

**Worked example 2 — Funnel.** 100,000 diagnosed patients, 60% treated, 25% of treated go on the new class.
- 100,000 × 0.60 × 0.25 = **15,000 patients**

**Common traps**
- Using revenue where the question asks for volume (or vice versa) — pharma volume (Rx) shares can differ sharply from revenue shares.
- Ignoring that a price cut needs a specific volume uplift to break even.

**Practice (Q50–Q55)**
50. Company revenue ₹180 Cr in a ₹1,200 Cr market. Market +10%, company +20%. New share?
51. 100,000 diagnosed; 60% treated; 25% of treated on the new class. Patients on the class?
52. ₹15 Cr revenue across 25 reps. Revenue per rep?
53. 4,800 physicians, 8 calls per physician per year, rep capacity 800 calls/year. Reps needed?
54. Price −10%, volume +12%. Revenue change?
55. Campaign spend ₹40 lakh produced ₹1.2 Cr incremental revenue at 65% gross margin. ROI?

---
---

# Answer key and explanations

> Attempt everything first. Then check each answer and write the *reason* for every miss.

### Percentages (Q1–Q6)

**Q1 — 25%.** (1,050 − 840)/840 = 210/840 = 0.25. Trap: 210/1,050 = 20% (wrong denominator).

**Q2 — 7 percentage points; +25% relative.** 35 − 28 = 7 pp; 7/28 = 25%. Use "pp" for the gap and "%" only for the relative change.

**Q3 — −2.25%.** 0.85 × 1.15 = 0.9775. Not 0%: the second change applies on a smaller base.

**Q4 — 432.** 2,400 × 0.18 = 432.

**Q5 — ₹950 Cr.** 1,140/1.2 = 950.

**Q6 — −20%.** (36,000 − 45,000)/45,000 = −0.20.

### Ratios (Q7–Q11)

**Q7 — ₹720 Cr.** 2,400 × 3/10.

**Q8 — 100,000 physicians.** 250 × 400.

**Q9 — 20%, then 20%.** 100/500 = 20%. After adding 100 ml excipient: 100/600 = 16.7%. *(Careful: only the volume is diluted, the active amount stays 100 ml.)*

**Q10 — Total ₹2,000 Cr; smaller ₹750 Cr.** 1 part = 250; 3 parts = 750.

**Q11 — 36 analysts.** 6 managers → 9 is ×1.5, so analysts = 24 × 1.5 = 36.

### Averages (Q12–Q17)

**Q12 — 156 Rx/rep.** (40×120 + 60×180)/100.

**Q13 — 150; wrong** because the two groups are unequal in size (40 vs 60). The bigger group must pull the average towards 180.

**Q14 — 7.8%.** (400×12 + 600×5)/1,000.

**Q15 — 51.** (5×46 − 26)/4 = (230 − 26)/4.

**Q16 — 4.08.** (200×4.2 + 50×3.6)/250 = 1,020/250.

**Q17 — 68.6 km/h.** 480 km / (4 h + 3 h) = 68.571. Harmonic mean 2ab/(a+b), **not** 70.

### Profit and margin (Q18–Q23)

**Q18 — Gross 60%, operating 30%.** (500−200)/500; (500−200−150)/500.

**Q19 — ₹360 Cr.** Revenue × (1 − 0.70).

**Q20 — 20% margin; ₹106.67.** 20/100 = 20%. For 25% margin: price = 80/0.75 = 106.67.

**Q21 — ₹60 Cr cut.** 15% → ₹300 Cr; 12% → ₹240 Cr; difference ₹60 Cr.

**Q22 — ₹50 operating profit; 5% margin.** 1,000 − 600 − 250 − 100 = 50.

**Q23 — 4,00,000 patients.** ₹60 Cr = 6 × 10⁸; ÷1,500.

### Fractions (Q24–Q28)

**Q24 — 37.5% and 62.5%.**

**Q25 — 1,200.** 1,440 × 5/6.

**Q26 — 2/3 (0.6667) > 5/8 (0.625) > 11/18 (0.6111) > 7/12 (0.5833).**

**Q27 — 14.3%.** 1/7 ≈ 0.142857.

**Q28 — 0.375×.** 0.25 × 1.5.

### Growth and CAGR (Q29–Q34)

**Q29 — 10%.** (1,331/1,000)^(1/3) − 1.

**Q30 — ₹1,000 Cr.** 640 × 1.25² = 640 × 1.5625.

**Q31 — 9.34%.** (1,250/800)^(1/5) − 1 = 1.0934 − 1.

**Q32 — 21.9%.** 20 × 1.15/1.05 = 21.90.

**Q33 — 30%.** (169/100)^(1/2) − 1 = 1.3 − 1.

**Q34 — ≈6 years by the rule of 72; exact 6.12 years.** ln2/ln1.12 = 6.116.

### Time, speed and work (Q35–Q39)

**Q35 — 8 days.** 1/(1/12 + 1/24) = 8.

**Q36 — 3 hours.** Rate per rep = 2,400/(2×3) = 400 records/h; 4 reps → 1,600/h; 4,800/1,600 = 3 h.

**Q37 — 5 more days.** In 4 days they finish 4 × (1/10 + 1/15) = 4/6 = 2/3. Remaining 1/3 at B's rate (1/15 per day) = 5 days.

**Q38 — 4 hours, 360 km from the 90 km/h analyst.** Time = 800/200.

**Q39 — 68.6 km/h.** 480/(4 + 3).

### Probability (Q40–Q44)

**Q40 — 37.8% (17/45).** 1 − (8/10)(7/9) = 1 − 56/90 = 34/90.

**Q41 — 72.9%.** 0.9³.

**Q42 — 45.5%.** 2 × 0.35 × 0.65.

**Q43 — 1.92%.** (6/40)(5/39) = 30/1,560.

**Q44 — 28.1%.** 1 − (34/40)(33/39) = 1 − 0.7192.

### Estimation (Q45–Q49)

**Q45 — ≈847.** 850 − 3.4.

**Q46 — 188.** 160 + 28.

**Q47 — 1,215.** Divide by 4.

**Q48 — 780.** ×2.5.

**Q49 — ₹30 lakh.** 12% of ₹2.5 Cr = ₹0.3 Cr = ₹30 lakh.

### Business reasoning (Q50–Q55)

**Q50 — 16.4%.** 216/1,320. Share rose because company growth (20%) exceeded market growth (10%).

**Q51 — 15,000 patients.** 100,000 × 0.6 × 0.25.

**Q52 — ₹0.6 Cr per rep.** 15/25.

**Q53 — 48 reps.** (4,800 × 8)/800 = 38,400/800.

**Q54 — +0.8%.** 0.9 × 1.12 = 1.008. A 10% price cut needs >11.1% volume growth just to hold revenue flat (1/0.9 − 1 = 11.1%).

**Q55 — 95% ROI.** Gross profit = 120 × 0.65 = ₹78 lakh; (78 − 40)/40 = 0.95.

---

## Checklist before moving on

- [ ] I can compute a percentage change without hesitating about the denominator
- [ ] I default to weighted averages whenever group sizes differ
- [ ] I always name the margin type (gross/operating)
- [ ] I can estimate a % of any number within 10 seconds
- [ ] I write funnel chains explicitly in business questions
- [ ] Every mistake above is logged in [19-errors-mistakes-and-traps.md](19-errors-mistakes-and-traps.md)

**Next:** [02-logical-reasoning.md](02-logical-reasoning.md) → [03-data-interpretation-and-graphs.md](03-data-interpretation-and-graphs.md)
