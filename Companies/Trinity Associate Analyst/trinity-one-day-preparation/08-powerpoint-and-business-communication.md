# 08 — PowerPoint & Business Communication

> **Learning objectives:** build the slide an interviewer would praise — answer first, insight-led titles, one message per slide — and phrase findings, uncertainty, and recommendations the way consultants do.
> **Estimated study time:** 30 min (reading + practice the 4-question framework aloud twice).
> **Priority:** **SHOULD KNOW** — the JD (CONFIRMED) lists *"generate client deliverables using … PowerPoint"* and *"collaborate … to present information in an easy-to-understand format."* The technical interview often ends with "how would you present this?"

---

## §8.1 The five laws of analyst presentations

1. **Answer first (pyramid principle).** Slide 1 = the conclusion. Executives read top-down, then stop; assume they read only the titles.
2. **Insight-led titles.** A title is a sentence with a number and a so-what — not a label.
3. **One message per slide.** If a slide supports two conclusions, it's two slides.
4. **Show the evidence, not your journey.** No methodology dwell unless asked; put it in the appendix. Your 40 tabs of work ≠ their 3 minutes of attention.
5. **Design quietly.** Consistent fonts/colors, sort bars by value, label units and n, cite sources and sample size, no 3-D, no rainbow palettes.

**Weak vs strong titles (the classic interview test):**

| Weak (label) | Strong (insight) |
|---|---|
| "Survey results" | "Awareness is high (60%); the constraint is reimbursement, not education" |
| "TRx by region" | "North CardioX = 21% of total TRx — protect the concentration" |
| "Adoption barriers" | "Cost/reimbursement is the #1 barrier (28%), ahead of safety (22%)" |

## §8.2 Executive summary formula (write it last, place it first)

Three lines, in this order:
1. **Finding** — the single most important result, with the number ("Awareness 60 ± 6pp; adoption 31%").
2. **Implication** — why it matters ("reach is not the bottleneck; access is").
3. **Action** — what you recommend ("prioritize the 3 uncovered regions; reallocate $X / Y calls").

Then the deck proves each line, in order. If a slide doesn't prove a summary line — cut it.

## §8.3 The 4-question framework (use it in interviews and slides)

> **1. What did I find?** → fact with number.
> **2. Why does it matter?** → implication for the business.
> **3. What should be done next?** → specific action + owner + metric.
> **4. What limits remain?** → assumptions, sample, data gaps — unprompted.

Practice aloud with: *"60% of cardiologists are aware (±6pp at n=250); adoption sits at 31%, concentrated in covered segments; I'd test the access hypothesis before spending more on education; limits: intent overstates behavior and we can't attribute causality from one wave."*

## §8.4 Evidence vs interpretation (keep lanes clean)

| Type | Example | Rule |
|---|---|---|
| **Fact / evidence** | "Awareness was 60% (n=250, ±6.1pp)." | Numbers with base and uncertainty |
| **Interpretation** | "Education is not the bottleneck." | Must be labeled as a judgment ("this suggests…") |
| **Recommendation** | "Reallocate the next campaign to access." | Action + rationale + metric to watch |

**Mistake:** selling interpretation as fact ("physicians dislike the drug" when the data only show low adoption, with access unmeasured).

## §8.5 Communicating uncertainty (say it confidently)

| Situation | Phrase |
|---|---|
| Wide CI / small sample | "Directionally X, but with n=30 I would not act on the segment cut yet." |
| Two segments differ by less than noise | "The 6-point gap is within margin of error — I'd call it equivalent." |
| Forecast | "Base case $265M, range $176–320M depending on penetration." |
| Correlation only | "The association tells us where to look — not what caused it." |

Never fake precision ("$264.6M" alone); always honest range ("≈ $265M, ±20% on penetration assumptions").

## §8.6 Stakeholder translation (nontechnical audience)

- **Answer first**, details after — reverse of analytical order.
- Replace stats jargon: "statistically significant" → "the difference is larger than noise"; "IQR" → "the middle half".
- One number per sentence; round loudly ("about 6 in 10").
- Attach decisions to numbers: every insight ends with "so we should…".
- Anticipate the follow-up question for each slide ("what would change your mind?").

## §8.7 Sample 5-slide outline — "Cardiology prescribing study" (fictional, consistent with files [`04`](04-statistics-survey-and-market-research.md)/[`05`](05-power-bi-tableau-and-data-visualization.md))

**Study:** CardioX awareness & adoption survey among cardiologists, n = 250 (PRACTICE scenario).

1. **Executive summary** — Awareness 60 ± 6pp; adoption 31%; constraint = reimbursement (34% of barrier mentions), not education. *Recommend: access sprint in 3 uncovered regions; re-measure in 90 days.*
2. **Awareness is broad, adoption is narrow** — funnel: aware 60% → ever-prescribed 45% → regularly prescribe 31% (line: "the drop happens after awareness"). Chart: funnel bar + CI note under the key number.
3. **Reimbursement is the #1 barrier among non-prescribers** — horizontal sorted bar: reimbursement 34%, safety 22%, efficacy 18%, other 26%. Footnote: multi-select question, non-prescribers only (n = 96).
4. **Segments diverge on access, not attitude** — grouped bars: covered vs uncovered segments (prescribing 38% vs 19%); note "attitude scores equal across segments."
5. **Recommendation & next steps** — 3 actions (access sprint; keep education flat; 90-day re-measure), owner + metric per action; limitations box: intent vs behavior, one wave, n per segment.

*(Slides 3–4 numbers are illustrative; every real number would carry n and MoE.)*

## §8.8 How this maps to the interview stages

| Stage | What they test with communication |
|---|---|
| Technical interview | Explain a formula/insight to a nontechnical stakeholder — use §8.6 |
| Case discussion | Structure + recommendation — use §8.3 aloud |
| Behavioral | "Tell me about explaining something complex simply" — prep a STAR story ([`09` §9.15](09-technical-interview-question-bank.md)) |
| Any deck mentioned in the JD | Titles like §8.1: if asked to sketch a slide, write the title first |

**Common mistakes list.** Burying the answer · label titles · data dump slides · no units/sample size · "the survey shows mixed results, more research is needed" (says nothing) · hiding uncertainty instead of bounding it · presenting correlation as cause.

**Next:** [09 — Technical interview question bank](09-technical-interview-question-bank.md)
