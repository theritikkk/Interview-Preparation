# 13 — Resume and Project Discussion

**Purpose:** turn your real software engineering work into evidence of *analytical* capability — without inventing experience or inflating anything.
**Prerequisites:** have your resume in front of you.
**Suggested study time:** 1.5–2 hours (mostly writing).
**Ground rule:** this file contains **placeholders** for details only you know. Nothing here asks you to claim client work, business metrics or life-sciences experience you do not have. Candidate reports mention resume-based questions in the interview (CANDIDATE-REPORTED, May 2026), so prepare this properly.

---

## 13.1 The translation principle

You do not need business experience to demonstrate analyst competencies. You need to **name the competency** behind your engineering work.

| What you did (SWE) | Analyst competency it proves | How to phrase it |
|---|---|---|
| Broke an ambiguous feature into modules and interfaces | Problem decomposition | "I decomposed an ambiguous requirement into components with clear inputs and outputs before writing code." |
| Added load tests, monitored p95 latency and queue depth | Measurement and metric design | "I defined the metrics first — throughput, p95 latency, queue depth — then measured before and after the change." |
| Found and fixed a race condition causing duplicate messages | Root-cause analysis | "I reproduced the defect deterministically, isolated it to a state-synchronisation bug, and verified the fix under load." |
| Validated inputs, secret checks, rate limits, tests | Data/quality validation mindset | "I refuse to trust unvalidated inputs: I validate at the boundary and check totals after." |
| Built CI tests and checks | Attention to detail, error prevention | "I put the check in the pipeline so the mistake could not recur silently." |
| Instrumented dashboards (Prometheus/Grafana) | Business-intelligence thinking | "I built the dashboard so the team could see the problem before it became an incident." |
| Wrote migration scripts, fixed deployment defects | Process improvement & automation | "I replaced a manual, error-prone process with a version-controlled one." |
| Load tested, measured, documented | Structured communication | "I reported results as a before/after with numbers, not opinions." |

**The one-sentence formula for every project:** *Problem → what I measured → what I changed → the number that moved → why that generalises to analytics work.*

---

## 13.2 Project talking-point sheets (from your own resume)

> The metrics quoted below are taken from your resume. Fill in the bracketed details yourself, and be ready for a follow-up question on every number.

### A. Distributed Task Scheduler (Cron-as-a-Service)
**Analyst-relevant points:** measurement design, capacity planning, reconciliation, exactly-once guarantees.

- **60-second pitch:** "I built a scheduler that runs user-defined tasks at scale. I load-tested 10,000 tasks scheduled for the same instant and measured the queue behaviour: 8,562 messages at peak, 74 msg/s ingest, a ~1,600 msg/s expiry burst, drained in ~7 minutes 46 seconds. p95 latency recovered from a 160 ms spike to a 76 ms floor with zero task loss, and I cut steady-state database load by replacing polling with TTL-based scheduling plus a reconciliation pass."
- **Analyst angle to say explicitly:** "The part I enjoyed most was defining the metrics and proving correctness — that's the same discipline as validating a dataset before reporting it."
- **Likely questions and frameworks:**
  1. *"How did you know the system was healthy?"* → Named metrics (queue depth, p95, DLQ count), baselines, thresholds, alerting.
  2. *"How would you present this to a non-technical stakeholder?"* → "Throughput and delay in plain language, with one chart: tasks completed per second, and the one risk — the expiry burst — with the mitigation."
  3. *"What would you do differently?"* → Have a target number to state (e.g., "[TARGET] ms p95 under [LOAD]"), because a target turns a measurement into a decision.
  4. *"How does 'exactly once' relate to data quality?"* → Duplicates are the same failure in both worlds: counting the same record twice inflates every total. `[Add: how you detected duplicate executions]`

### B. WebChat — Distributed Real-Time Chat Platform
**Analyst-relevant points:** defect investigation, deployment hygiene, verification gates.

- **60-second pitch:** "A six-service real-time platform where I fixed two race conditions that were silently dropping and duplicating messages, replaced an unsafe auto-migration setup with version-controlled migrations, and gated releases behind a 22-test CI suite."
- **Analyst angle:** "'Silently' is the important word — the system looked fine while producing wrong data. That's exactly why I now check totals and duplicates before trusting any analysis output."
- **Likely questions:**
  1. *"How did you discover the race condition?"* → Symptom → reproduction → isolation → fix → verification. Use this same structure for any "walk me through a problem" question.
  2. *"What did the migration fix change?"* → `[ONE LINE about the defect and the measurable improvement]`
  3. *"How do you prioritise quality versus speed?"* → "Stability of the interface first, features second — because a wrong data pipeline costs more to fix later than to build correctly."

### C. Hermes — Event-Sourced Multi-Tenant Workflow Platform
**Analyst-relevant points:** auditability, reproducibility, process reliability.

- **60-second pitch:** "A multi-tenant workflow engine built with event sourcing and CQRS, so every execution is replayable and auditable, with a transactional outbox for reliable delivery, saga-based compensation and a replay engine to rebuild state from checkpoints."
- **Analyst angle:** "Event sourcing means no data is lost — every state has a history. In analytics terms, that's auditability: you can always reconstruct how a number was produced."
- **Likely questions:**
  1. *"What's the analyst parallel?"* → Reproducibility: any reported number should be reconstructable from raw data.
  2. *"Tell me about the deployment defect you fixed."* → `[SPECIFIC: what was skipped, how you found it, the control you added]`
  3. *"How would you apply event-sourcing thinking to a survey pipeline?"* → Keep raw responses immutable; apply cleaning transformations as a documented, reproducible step (the same reason you never overwrite raw data — see [05](05-excel-practical-workbook-guide.md)).

### D. Full Stack Developer Internship (S.N. Engineering Works)
**Analyst-relevant points:** business process improvement, replacing manual workflows, pipeline tracking.

- **60-second pitch:** "I built a B2B machinery marketplace from an empty repository to a live production site, and replaced a manual email-based quoting process with a structured configurator. I also built a lead pipeline with status tracking and ownership — essentially an internal CRM — which the company used after my internship ended."
- **Analyst angle:** "The most useful part wasn't the code — it was removing a manual, ambiguous process and making the status of every enquiry visible. That is the same thing as replacing a manual Excel report with a validated, repeatable one."
- **Likely questions:**
  1. *"What business problem did the configurator solve?"* → Back-and-forth on every enquiry → structured quote requests; state the change qualitatively (you are not asked to invent a saving figure).
  2. *"How did you decide what to track in the lead pipeline?"* → By asking the people who used it and by what drove their next action.
  3. *"What would you measure if you had the data?"* → Enquiry-to-quote conversion, time-to-quote, drop-off stage. *(These are the metrics; you can say they were not measured — honesty is stronger than invention.)*

### E. Community activity (Geek Room MSIT)
**Analyst-relevant points:** collaboration, organising, communication.

- Use for teamwork, organising DSA/coding competitions and receiving a Letter of Appreciation. Keep to facts; state your specific role. `[WHAT DID YOU OWN — registrations, problem setting, logistics?]`

---

## 13.3 Resume-based questions and answer frameworks

| Question | Framework |
|---|---|
| "Walk me through your resume in 2 minutes." | Present → two strongest evidence points → why this role. Never chronological narration. |
| "Why did you build distributed systems if you want analytics?" | Genuine interest in systems + preference for the decision side of data + the transferable habit of measurement. Do **not** say you lost interest in software. |
| "Which project are you most proud of, and why?" | Pick the one where you *measured* something. Name the metric. |
| "How do you know your systems were correct?" | Verification: tests, reconciliation, monitoring, and independent checks. |
| "What was the hardest bug you fixed?" | Symptom → reproduction → isolation → root cause → fix → regression prevention. |
| "You've never done market research. Why should we take you?" | Honest gap + your demonstrated learning speed with evidence + the parts you already bring (data handling, automation, dashboards). |
| "What would you do in your first month here?" | Learn the data and tools (Qualtrics, Excel templates, Power BI) → reproduce an existing deliverable → then improve one recurring process. |
| "How do you handle a task you don't know how to do?" | Break it down, find the smallest working example, build a draft, validate with someone senior; never deliver silence. |
| "Your projects use Python. Why should we care about Excel?" | Excel is the client-facing analytical layer in this role; you have deliberately practised it ([04](04-excel-formulas-cheatsheet.md), [05](05-excel-practical-workbook-guide.md)) and you can show specific formulas/tasks you can do. |
| "What's on your resume that we should be sceptical about?" | Have a defensible answer for every number — or remove the claim. Never let an unverifiable metric stay. |

---

## 13.4 "Analyst angle" lines you can reuse safely

These are all true for a technical candidate and require no invented experience:

- "I'm comfortable with large, messy data — I've dealt with queues, duplicates and idempotency in systems; the same discipline applies to a messy spreadsheet."
- "I default to measuring before and after, because otherwise you can't tell whether a change helped."
- "I write checks rather than promising to be careful."
- "I'm early in the commercial domain and I'm treating the industry and statistics as my current learning projects."
- "I can automate recurring analysis with Python or VBA, which the JD lists as process-improvement tools."

**Sentences to avoid:**
- "I'm basically already an analyst." (You are not — and it invites a hostile question.)
- "I've done market research" unless you have (a course project is fine *if labelled as a course project*).
- Any number not in your resume.

---

## 13.5 Optional 24-hour resume upgrades (honest and fast)

If the interview is imminent, you can add one line of *real* analyst evidence in a day:

1. **Mini analysis project (3–4 hours).** Take a public dataset (or `data/trinity_excel_mock.csv` from this repo), clean it in Excel, build a PivotTable and one chart, and write a five-line summary: finding, evidence, implication, limitation, next step. Add one resume line: *"Cleaned and analysed a 30-row rep-level dataset in Excel; identified 2 data-quality defects affecting 7% of totals and reported findings in a one-page summary."* — only if you actually did it.
2. **Excel skills line** (defensible version): *"Microsoft Excel: SUMIFS, COUNTIFS, XLOOKUP/VLOOKUP, INDEX-MATCH, PivotTables, conditional formatting, data validation."* Keep only what you can demonstrate live.
3. **Do not add Qualtrics, Power BI or VBA as skills unless you have used them.** The JD lists them as tools used in the role; familiarity is a plus, false fluency is a trap.

---

## 13.6 The 60-second project pitch template

> "**Problem:** [One line — whose problem, what decision it affected]. **What I did:** [Two actions, mechanism-level]. **How I measured it:** [The metric and the before/after]. **Result:** [One number, from your resume]. **Analyst relevance:** [One line connecting it to data validation / measurement / communication]."

Rehearse this for two projects only. Depth on two beats shallow coverage of four.

---

## 13.7 Pre-interview checklist

- [ ] Every number on my resume has a 30-second explanation behind it
- [ ] I have two project pitches using the template above
- [ ] I can explain *why* each project mattered, not only how it worked
- [ ] I can name the analyst competency each project proves
- [ ] I have an honest answer for the domain-experience gap
- [ ] I have one question ready about the role's first 90 days

**Next:** [12-behavioral-interview-and-hr.md](12-behavioral-interview-and-hr.md) → [17-mock-interview-evaluation-rubric.md](17-mock-interview-evaluation-rubric.md)
