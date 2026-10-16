# 10 — Mock Assessment & Excel Test

> **Learning objectives:** rehearse both practical stages under real time pressure: a 60-minute proctored-style online assessment (**Mock A**) and a 45-minute Excel assessment (**Mock B**), then calibrate with full answer keys.
> **Estimated study time:** 105 min (60 + 45) + 25 min key review.
> **Priority:** **MUST DO.**
> **Rules:** one sitting per mock. No peeking at keys until finished. Mock A without a calculator if your test rules so indicate; Mock B in a real spreadsheet (Excel / LibreOffice / Google Sheets).
> ⚠️ The real question distribution is **unknown** — these are **PRACTICE** mocks with a realistic mix, not Trinity's actual paper.

---
---

# §10.1 MOCK A — Proctored Online Assessment (60 minutes · 30 questions)

**Instructions.** 30 questions, 60 minutes (≈ 2 min/question). Section order: Quant (1–8) · Percentages/share/stats (9–14) · Logic (15–20) · Data interpretation (21–26) · Analytical & business reasoning (27–30). Attempt all; flag and skip anything exceeding 3 minutes; return at the end. Record answers before checking §10.3.

### Section A — Quantitative aptitude

**Q1.** 15% of 240 = ?
**Q2.** A price rises 12%, then falls 10%. Net change?
**Q3.** ₹320 is split in ratio 3:5. The larger share is?
**Q4.** The average of 6 numbers is 24. A 7th number, 45, is added. New average?
**Q5.** 40% of patients use 20 units/month; 60% use 45 units/month. Weighted average?
**Q6.** Revenue goes 100 → 144 over 2 years. CAGR?
**Q7.** Revenue 850; COGS 578. Gross margin?
**Q8.** Successive discounts of 20% and 10% on $500. Final price?

### Section B — Percentages, share & statistics

**Q9.** 45% of 2,400 physicians = ?
**Q10.** A value increases 25%, then decreases 25%. Net change?
**Q11.** Brand sales 480 Cr in a 4,000 Cr market. Market share?
**Q12.** Promoters 40%, passives 35%, detractors 25%. NPS?
**Q13.** 3 of 10 vials are defective. Probability a random vial is defective?
**Q14.** Survey n = 500; 64% aware. (a) How many aware? (b) Closest margin of error at 95%: ±2.1 / ±4.2 / ±6.4 / ±10.1 pp?

### Section C — Logical reasoning

**Q15.** Next in 3, 6, 12, 24, …?
**Q16.** Next in 2, 5, 10, 17, 26, …?
**Q17.** Odd one out: 8, 27, 64, 100, 125.
**Q18.** All analysts use Excel. Some Excel users dislike PivotTables. Which must be true? (a) Some Excel users are analysts (b) No analyst dislikes PivotTables (c) All Excel users are analysts (d) Some analysts dislike PivotTables
**Q19.** P, Q, R sit in a row. R is second. Q is not first. Who is first?
**Q20.** "If it rains, client calls drop." Calls did not drop. Valid conclusion?

### Section D — Data interpretation

*Table 1 — Revenue (₹ Cr): Q1 120 · Q2 150 · Q3 135 · Q4 195*

**Q21.** Q4 vs Q1 growth?
**Q22.** Average quarterly revenue?
**Q23.** Which statement is fully supported? (a) Revenue declined all year (b) Q3 dipped 10% vs Q2, then Q4 was the year high (c) Growth was steady every quarter (d) Q2 was the weakest quarter

*Table 2 — Regional TRx: North 320 · South 210 · West 470*

**Q24.** West's share of total TRx (1 dp)?

*Table 3 — Brand units: A 40 · B 25 · C 15 · D 20*

**Q25.** Combined share of B + D?

*Series — monthly reads: 100, 102, 99, 104, 450, 106*

**Q26.** What should you do with 450?

### Section E — Analytical & business reasoning

**Q27.** Prescriptions: 2, 3, 3, 5, 17. Mean and median — which better represents the typical physician, and why?
**Q28.** Your brand's TRx grew 8%, but share fell 1.2 pp. What happened?
**Q29.** A survey frame omits the 20% of physicians without registered mobile numbers. This is: (a) nonresponse bias (b) coverage bias (c) recall bias (d) social desirability bias
**Q30.** You must show the share of 4 products evolving across two months. Best chart? (a) pie (b) 100% stacked column (c) scatter (d) histogram

**STOP. Check your answers in §10.3 only after the timer ends.**

---
---

# §10.2 MOCK B — Excel Assessment (45 minutes · 17 points)

**Instructions.** Open the dataset (§10.2.1) in a spreadsheet. Time checkpoints: cleaning 10 min → aggregations 20 → achievement 28 → lookups 36 → pivot 41 → chart + conclusion 45. Label every answer cell (auditable work earns points). You may use formulas, PivotTables, or Power Query — be ready to explain any method.

### §10.2.1 Dataset — paste as `raw.csv` (30 rows; mess included intentionally)

```csv
RepID,RepName,Region,Specialty,Product,Month,TRx,TargetTRx,Calls
R-001,Aarav,North,Cardiologist,CardioX,Jan-26,1250,1200,40
R-002,Bhavna,North,Cardiologist,CardioX,Jan-26,980,1050,36
R-003,Chirag,north ,Neurologist,NeuroY,Jan-26,640,600,30
R-004,Divya,South,Cardiologist,CardioX,Jan-26,1420,1300,45
R-005,Eshan,South,Neurologist,NeuroY,Jan-26,880,900,38
R-006,Fatima,East,Oncologist,OncoZ,Jan-26,260,300,22
R-007,Gaurav,East,Cardiologist,CardioX,Jan-26,1105,1100,41
R-008,Hina,West,Neurologist,NeuroY,Jan-26,720,750,33
R-009,Irfan,West,Oncologist,OncoZ,Jan-26,410,380,28
R-010,Jaya,N,Cardiologist,CardioX,Jan-26,1320,1250,44
R-011,Kabir,North,Oncologist,OncoZ,Jan-26,355,340,26
R-012,Lavanya,South,Cardiologist,CardioX,Jan-26,,1150,39
R-013,Manish,South,Oncologist,OncoZ,Jan-26,298,320,24
R-014,Neha,East,Neurologist,NeuroY,Jan-26,540,520,31
R-014,Neha,East,Neurologist,NeuroY,Jan-26,540,520,31
R-015,Omkar,West,Cardiologist,CardioX,Jan-26,"1,510",1450,47
R-016,Priya,North,Neurologist,NeuroY,Feb-26,760,700,35
R-017,Qadir,South,Neurologist,NeuroY,Feb-26,915,880,37
R-018,Rhea,East,Cardiologist,CardioX,Feb-26,1180,1200,42
R-019,Sahil,West,Oncologist,OncoZ,Feb-26,455,430,29
R-020,Tara,North,Cardiologist, CardioX,Feb-26,1275,1200,43
R-021,Uday,South,Cardiologist,CardioX,Feb-26,1340,1350,44
R-022,Vidhi,East,Neurologist,NeuroY,Feb-26,605,580,32
R-023,Waseem,West,Neurologist,NeuroY,Feb-26,690,720,34
R-024,Xenia,North,Oncologist,OncoZ,Feb-26,330,350,25
R-025,Yash,South,Oncologist,OncoZ,Feb-26,275,300,23
R-026,Zara,East,Cardiologist,CardioX,Feb-26,1220,1150,40
R-027,Aman,West,Cardiologist,CardioX,Feb-26,1390,1400,46
R-028,Bela ,North,Neurologist,NeuroY,Feb-26,705,680,33
R-029,Chetan,South,Oncologist,OncoZ,Feb-26,285,,27
```

### §10.2.2 Tasks

**Part 1 — Cleaning (4 pts).** 1.1 List every data-quality issue with a count per issue. 1.2 Produce a clean table: regions limited to North/South/East/West; product/rep names trimmed; TRx and Target numeric; duplicate removed. 1.3 State rows remaining and cells fixed/excluded.

**Part 2 — Aggregations (4 pts).** 2.1 Total TRx. 2.2 TRx by region (4 numbers). 2.3 TRx by product (3 numbers). 2.4 TRx by month (2 numbers). 2.5 Average Calls by specialty (3 numbers, 2 dp).

**Part 3 — Achievement (3 pts).** 3.1 Add `Achievement %` = TRx/Target (blank if either missing). 3.2 Count reps at/above 100%, and the average achievement — stating exclusions. 3.3 Band counts: Exceeds ≥110%, On track 90–109.9%, Below <90%.

**Part 4 — Lookups (3 pts).** Master table: CardioX→Cardiovascular→2019 · NeuroY→Neurology→2021 · OncoZ→Oncology→2023. 4.1 Add Therapeutic Area via lookup. 4.2 Count lookup failures (show the formula). 4.3 Count Jan-26 rows for products launched before 2022.

**Part 5 — PivotTable (2 pts).** 5.1 Region × Product, Sum of TRx (+ % of grand total). 5.2 Identify the largest single cell and its share.

**Part 6 — Chart + conclusion (1 pt).** 6.1 One chart answering "which region-product combination carries the business?" — state type and why. 6.2 Insight-led title. 6.3 Three conclusion bullets: finding, risk, limitation.

**STOP. Solutions begin below — do not read until your 45 minutes are up.**

---
---

# §10.3 MOCK A — Answer key (with explanations)

| Q | Answer | Why / trap |
|---|---|---|
| 1 | **36** | 240 × 0.15 |
| 2 | **+0.8%** | 1.12 × 0.90 = 1.008 — successive changes multiply; not +2% |
| 3 | **₹200** | 320 × 5/8. Trap: splitting as 5/3 |
| 4 | **27** | (6 × 24 + 45) ÷ 7 = 189/7. Trap: (24+45)/2 |
| 5 | **35 units** | 0.4×20 + 0.6×45. Trap: simple average 32.5 |
| 6 | **20%** | √(144/100) − 1. Trap: (144−100)/100/2 = 22% |
| 7 | **32%** | (850−578)/850. Trap: dividing by 578 → 47.1% (markup) |
| 8 | **$360** | 500 × 0.8 × 0.9. Trap: 500 × 0.7 = 350 (adding discounts) |
| 9 | **1,080** | 2,400 × 0.45 |
| 10 | **−6.25%** | 1.25 × 0.75 = 0.9375. Trap: "0% — it cancels" |
| 11 | **12%** | 480/4000 |
| 12 | **15** | 40 − 25; passives excluded |
| 13 | **30%** | 3/10 |
| 14 | **(a) 320 (b) ±4.2 pp** | 500 × 0.64; 1.96 × √(0.64×0.36/500) ≈ 4.2% |
| 15 | **48** | ×2 each step |
| 16 | **37** | +3, +5, +7, +9, **+11** |
| 17 | **100** | The others are perfect cubes (2³…5³); 100 is not |
| 18 | **(a) Some Excel users are analysts** | "All analysts use Excel" ⇒ their set overlaps Excel users. (b)/(d) overreach from "some"; (c) reverses the direction |
| 19 | **P** | R = 2nd; Q not 1st ⇒ Q = 3rd ⇒ P = 1st |
| 20 | **It did not rain** | Contrapositive: ¬drop ⇒ ¬rain. Trap: "rain caused no drop" (invalid) |
| 21 | **62.5%** | (195−120)/120 |
| 22 | **150** | 600/4 |
| 23 | **(b)** | Q3: 135/150 − 1 = −10%; Q4 = 195 highest. (d) is wrong (Q1 120 is weakest); (a)/(c) contradicted |
| 24 | **47.0%** | 470/1000 |
| 25 | **45%** | (25+20)/100 |
| 26 | **Flag it and verify** | Check data entry / one-off event before using; don't silently delete |
| 27 | **Mean 6, median 3 → median** | 17 is a high-writing outlier; median describes the typical physician |
| 28 | **Market grew faster than the brand** | Share = brand/market; brand +8% < market growth ⇒ share fell while the brand still expanded |
| 29 | **(b) coverage bias** | Part of the population can't be sampled (frame gap), not a refusal problem |
| 30 | **(b) 100% stacked column** | Part-of-whole across categories over time; pie fails with time, scatter/histogram don't fit |

**Scoring bands.** 27–30: assessment-ready · 24–26: solid, review misses · 20–23: revise files 02/04 and redo · < 20: work the weak sections before the drive.

---

# §10.4 MOCK B — Answer key, expected outputs, and validation

*(Values independently computed and cross-checked.)*

### Part 1 — Cleaning (4 pts)

**1.1 Issues and counts (7 types):**

| # | Issue | Count |
|---|---|---|
| 1 | Exact duplicate row (R-014) | 1 row |
| 2 | Region variants: `north ` (lower + space) and `N` | 2 rows |
| 3 | Product with leading space (` CardioX`) | 1 row |
| 4 | Rep name trailing space (`Bela `) | 1 row |
| 5 | TRx stored as text `"1,510"` | 1 row |
| 6 | Blank TRx | 1 row (R-012) |
| 7 | Blank TargetTRx | 1 row (R-029) |

**1.2 Key formulas (first row, copy down):**
```excel
Region:  =IF(OR(LOWER(TRIM(C2))="n",LOWER(TRIM(C2))="north"),"North",PROPER(TRIM(C2)))
Product: =TRIM(E2)          RepName: =TRIM(B2)
TRx:     =IF(TRIM(G2)="","",VALUE(SUBSTITUTE(TRIM(G2),",","")))
Target:  =IF(TRIM(H2)="","",VALUE(SUBSTITUTE(TRIM(H2),",","")))
DupFlag: =IF(COUNTIFS($A$2:A2,A2,$F$2:F2,F2)>1,"REMOVE","")
```
**1.3** 30 raw rows → **29 clean rows**; **5 cells fixed** (2 regions, 1 product, 1 name, 1 text-number); **2 blanks excluded from matching calculations** (1 TRx, 1 target).

### Part 2 — Aggregations (4 pts)

| Task | Answer |
|---|---|
| 2.1 Total TRx | **23,113** |
| 2.2 By region | North **7,615** · South **5,413** · West **5,175** · East **4,910** |
| 2.3 By product | CardioX **13,990** · NeuroY **6,455** · OncoZ **2,668** |
| 2.4 By month | Jan-26 **11,688** · Feb-26 **11,425** |
| 2.5 Avg Calls | Cardiologist **42.25** · Neurologist **33.67** · Oncologist **25.50** |

**Validation (say this to earn the point):** region sum = product sum = month sum = **23,113** — the cross-foot check. Any mismatch = criteria typo (usually a stray space).

### Part 3 — Achievement (3 pts)

**3.1** `=IF(OR(G2="",H2=""),"",G2/H2)`.
**3.2** Assessable reps: **27** (excluded R-012 blank TRx, R-029 blank target). At/above 100%: **16**; below: **11**. Average achievement **101.1%**. *(State the exclusions.)*
**3.3** Bands: **Exceeds 0 · On track 26 · Below 1** (total 27 ✓). Top achiever R-004 (109.2%); lowest R-006 (86.7%). R-015 has the top absolute TRx (1,510) but 104.1% of target — absolute vs relative distinction.

### Part 4 — Lookups (3 pts)

**4.1** `=XLOOKUP(E2,Master!$A$2:$A$4,Master!$B$2:$B$4,"CHECK")` — or VLOOKUP/INDEX-MATCH equivalents. All rows resolve after cleaning.
**4.2** Failures: **0 after cleaning** — formula `=COUNTIF(<col>,"CHECK")` with `IFNA` wraps, or `COUNTIF(<col>,"#N/A")`. (With the leading space left in, it would be exactly 1 — that's the point.)
**4.3** Jan-26 rows, launch < 2022: CardioX 7 + NeuroY 4 = **11**.

### Part 5 — PivotTable (2 pts)

| Region | CardioX | NeuroY | OncoZ | Total |
|---|---:|---:|---:|---:|
| North | 4,825 | 2,105 | 685 | **7,615** |
| South | 2,760 | 1,795 | 858 | **5,413** |
| West | 2,900 | 1,410 | 865 | **5,175** |
| East | 3,505 | 1,145 | 260 | **4,910** |
| **Total** | **13,990** | **6,455** | **2,668** | **23,113** |

**5.2** Largest cell: **North × CardioX = 4,825 = 20.9%** of the business. Largest region: North (33.0%); largest product: CardioX (60.5%).

### Part 6 — Chart + conclusion (1 pt)

**6.1** Clustered/stacked column, regions on the axis, one series per product — two-dimensional comparison; sort regions by total descending (North, South, West, East). Not a pie (too many categories), not a line (no time scale point here).
**6.2** e.g. *"North CardioX alone = 4,825 TRx (21%) — the business is a concentration, not a spread."*
**6.3** Finding: CardioX 60.5% of TRx; North-CardioX cell 20.9% → protect the core. Risk: 11 of 27 reps below target; one product carries the whole book. Limitation: 2 months only; blanks/duplicate existed; no territory-size or competitor data → descriptive, not causal.

**Grading: 17 points total. Target ≥ 14 before the real Excel test.** Any missed item → redo on a fresh copy with the timer on.
