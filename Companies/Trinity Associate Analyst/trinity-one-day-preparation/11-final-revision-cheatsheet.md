# 11 — Final Revision Cheatsheet (30 minutes)

> One flagship page per topic. If you only have 30 minutes before the drive, read this file top to bottom. Everything here was deliberately selected: if it's in this file, you're expected to know it.
> **Estimated time:** 30 min read · 10 min active recall (cover the right column, say the answer).

---

## 11.1 Essential aptitude formulas

| Formula | One-liner |
|---|---|
| % change | (new − old)/old × 100 — denominator is **old** |
| pp vs % | 46% → 52% = **+6 pp** = **+13.0%** relative (6/46) |
| Successive changes | multiply: +12% then −10% → 1.12 × 0.90 = **+0.8%** |
| +25% then −25% | **−6.25%** (1.25 × 0.75), never "0%" |
| Ratio a:b of total T | parts T·a/(a+b), T·b/(a+b) |
| Weighted average | Σ(w·v)/Σw — e.g., 0.4×20 + 0.6×45 = 35 |
| Compound growth | CAGR = (End/Start)^(1/n) − 1; 100→144 over 2y = 20% |
| Doubling rule | ~72 ÷ years = % needed; 7.2%/yr doubles in 10y |
| Gross margin | (Rev − COGS)/Rev; margin vs **markup** (÷ cost) — 272/850 = 32% vs 272/578 = 47.1% |
| Break-even volume | Fixed costs ÷ (price − variable cost) |
| Probability | favourable/total; independent: multiply; P(≥1) = 1 − P(none) |
| Anomaly rule | flag → verify → decide; never silently delete |

## 11.2 Market share & performance

| Item | Rule |
|---|---|
| Share | brand ÷ market × 100; changes in **pp** |
| Share direction | brand growth > market growth ⇒ share ↑; +8% brand vs +12% market ⇒ share lost |
| Segment contribution | segment ÷ total; concentration example: top decile can drive ~40% of TRx |
| NPS | %promoters − %detractors (passives excluded); 40−25 = **15** |

## 11.3 Excel — the syntax you must write cold

```excel
=SUM(D:D)                     =AVERAGE(D:D)          =COUNT(D:D) / =COUNTA(A:A)
=IF(cond, a, b)               =IFERROR(x, "")        =IFS(t1,v1, t2,v2, TRUE,else)
=SUMIF(range,crit,sum)        =SUMIFS(sum, r1,c1, r2,c2)      ← SUMIFS: sum FIRST
=COUNTIF(range,crit)          =COUNTIFS(r1,c1, r2,c2)
=AVERAGEIFS(avg, r1,c1, …)
=XLOOKUP(val, lookup_arr, return_arr, "CHECK")      =VLOOKUP(val, tbl, col, FALSE)
=INDEX(return, MATCH(val, lookup, 0))
=TRIM(A2)   =VALUE(SUBSTITUTE(TRIM(A2),",",""))      ← fix " 1,240 "
=IF(COUNTIFS($A$2:A2,A2,$F$2:F2,F2)>1,"DUPE","")     ← duplicate flag
```
Traps: VLOOKUP without FALSE; missing `$` on lookup tables; text-numbers skipped by SUM; blanks≠0; state exclusions. Pivot: rows × columns × values → % of grand total; refresh after changes. Cross-foot every cut to one grand total (Mock B: 23,113).

## 11.4 Statistics — definitions in interview English

| Term | Say this |
|---|---|
| Mean/median/mode | total-averaged / middle value / most frequent; median resists outliers (2,3,3,5,17 → mean 6, median 3) |
| SD | consistency, not average; two regions, same mean, different risk |
| Percentile | top decile ≈ heaviest prescribers — segment before averaging |
| MoE (95%) | 1.96 × √(p(1−p)/n); n=250, 60% → **±6.1 pp** → CI [53.9, 66.1] |
| Precision | halve MoE → 4× sample; ±5 pp at p=0.5 → n ≈ 385 (use 400) |
| p-value | chance of data this extreme if no real difference; p<0.05 "significant" |
| Two groups | 60% vs 54% (n=250) → z≈1.36, p≈0.18 → **not significant**; 55% vs 47% (n=400) → p≈0.023 → significant |
| Weighted results | pooled (180+90+275)/1000 = **54.5%**, not simple avg 53.3% |
| Correlation | ≠ causation: could be reverse or a confounder (calls ↔ TRx ≠ calls cause TRx) |

## 11.5 Survey pitfalls (name these before they ask)

Leading questions ("Don't you agree…") · double-barreled · unbalanced scales · **nonresponse bias** (busy high-writers skip) · **coverage/frame bias** (20% unreachable) · social desirability · recall error · stated-intent > behavior (discount intent!) · small subgroup n (say "under-powered", don't over-read) · missing n/MoE in the report. Design: one idea, neutral, behavioral timeframe, screener, top-2-box reporting, weight when composition differs. Qual = why (FIEs) → quant = how many (survey).

## 11.6 Power BI terms

Get Data (Excel/CSV/SQL) → **Power Query** (repeatable cleaning/ETL) → **Model** (relationships, star schema) → **Measures** (live aggregates: Total TRx) vs **Calculated columns** (stored, row-level) → visuals → Publish to **Service** → **scheduled refresh**.
DAX recall: `SUM(col)` · `COUNTROWS(t)` · `DISTINCTCOUNT(col)` · `DIVIDE(a,b)` · `CALCULATE([Measure], filter)` (changes filter context — share-of-product etc.). KPI card / bar / line / stacked column / matrix / slicers / drill-down. Report = multi-page canvas; **Dashboard** = pinned monitored KPIs.

## 11.7 Tableau terms

Connect (CSV/Excel/SQL; Data Interpreter) → drag fields → marks. **Dimensions** slice (Region, Product, IDs) vs **Measures** aggregate (SUM TRx). Discrete = headers/separate bars; continuous = axis (use for dates/trends). Filters (incl. interactive filter actions), calculated fields, aggregation pills. Bars/lines/scatter/heatmap/maps; dashboards + stories.
**vs Power BI:** same job; Power BI wins in Microsoft ecosystems & scheduled enterprise reporting (DAX, Power Query); Tableau wins in fast visual exploration & design polish. Skill transferable.

## 11.8 SQL — the 6 patterns

```sql
SELECT region, SUM(trx) AS total FROM sales s JOIN reps r ON s.rep_id = r.rep_id
GROUP BY region ORDER BY total DESC;                     -- aggregate + rank
… GROUP BY product HAVING SUM(trx) > 5000;               -- HAVING filters groups (WHERE cannot see SUM)
LEFT JOIN … WHERE s.rep_id IS NULL;                      -- find gaps (no sales)
SUM(CASE WHEN response>=4 THEN 1 ELSE 0 END)*1.0/COUNT(*) -- top-2-box %
GROUP BY rep_id, month HAVING COUNT(*)>1;                -- duplicate check
COALESCE(x,0) — substitute with care; filling 0 asserts "sold nothing"; report excluded rows
```

## 11.9 pandas — the 4 moves

```python
df = pd.read_csv("raw.csv")                    # .head() .info() .describe()
df["TRx"] = pd.to_numeric(df["TRx"].astype(str).str.replace(",",""), errors="coerce")
df = df.drop_duplicates()                      # clean
df.groupby("Region")["TRx"].sum()              # aggregate (North 7615 … East 4910)
df.merge(master, on="Product", how="left")     # Excel lookup, pandas style
```
Tool choice: one-off small → **Excel**; warehouse/join/scale → **SQL**; recurring multi-file automation → **Python**; recurring monitoring → **BI tool**.

## 11.10 Chart-selection guide

| Question | Chart | Never |
|---|---|---|
| Trend over time | Line (continuous date) | Pie |
| Compare categories | Sorted bar | Unsorted bars |
| Part of whole | Stacked bar (≤ 5) / 100% stacked over time | Pie with 8 slices |
| Region × product | Clustered/stacked column | 3-D anything |
| Two metrics relate | Scatter | Dual-axis causal claims |
| Distribution | Histogram/box | Averaging away spread |

Misleading: truncated y-axis, cherry-picked windows, inconsistent scales. Title = insight + number + so-what ("CardioX = 60.5% of TRx — protect the core").

## 11.11 Market-entry framework (8 steps)

1 Clarify objective → 2 Structure (tree: patients × diagnosed × treated × eligible × penetration × price) → 3 Metrics/data needed → 4 Calculate (round loudly; base/low/high) → 5 Insights (so-what) → 6 Implications → 7 One recommendation + metric to track → 8 Risks/assumptions/missing data (say unprompted).
Anchor example: 6.0M diagnosed → ×70% treated → ×35% eligible → ×15% penetration = 220.5k patients → ×$1,200 = **≈$264.6M** peak; 10% penetration → $176.4M (**−33%**) — sensitivity is the case.

## 11.12 Presentation structure (what/why/do/limits)

Answer first: ① What did I find? (number + n + MoE) ② Why does it matter? ③ What should be done next? (owner + metric) ④ What limits remain? Slides: insight-led titles, one message each, evidence vs interpretation labeled, appendix for method. Exec summary = finding → implication → action (3 lines).

## 11.13 Interview reminders (60 seconds before you enter)

- STAR for behaviorals: 30–45 sec stories, result + lesson; have teamwork + deadline + detail + explain-simply stories ready.
- Case: clarify first, structure aloud, round numbers loudly, "so-what" after every number, recommendation + risks, ask for missing data.
- Excel: know SUMIFS order (sum first), XLOOKUP, dedupe flags; state rows excluded; cross-foot.
- Stats: quote n and ±MoE with every %; significant ≠ important; intent > behavior is wrong direction.
- Attention to detail is an **explicit JD requirement** — mention your QA habits (cross-foot, recompute, exclusion notes).
- "Why Trinity?": survey analytics + dashboards + process improvement (Excel/VBA/Python/Power BI/Tableau) = exactly the JD; TGaS works with pharma commercial leaders.
- Questions to ask: growth path in year one; Power BI vs Tableau stack; how Python automation is used today.
- Logistics: ID/laptop per instructions, stable internet, quiet room, water, timer discipline (flag & skip > 3 min).

## 11.14 Mini glossary (analyst terms)

| Term | Meaning |
|---|---|
| TRx / NRx / NBRx | Total / new / new-to-brand prescriptions |
| Attainment | Actual ÷ target |
| MoE / CI | Margin of error / confidence interval |
| Top-2-box | % choosing top two scale points |
| ETL | Extract–Transform–Load (data prep pipeline) |
| DAX | Power BI's formula language |
| KPI | Key performance indicator (monitored metric) |
| Slicer / filter | Interactive dashboard control |
| Decile | Prescriber rank band (1–10) |
| LoE | Loss of exclusivity (patent cliff) |
| HCP / Payer | Physician / who pays (insurer, PBM) |
| FIE / Qual / Quant | Depth interview / why / how many |
| **Labels in this pack** | CONFIRMED = from your docs · OLDER JD = 2022 doc · CANDIDATE-REPORTED = public posts, unverified · ADDITIONAL = extra industry prep · PRACTICE = original, not official |

---

**Final ritual (2 min):** re-read §11.13, drink water, breathe. You've covered the process end-to-end. Go get it. 🎯
