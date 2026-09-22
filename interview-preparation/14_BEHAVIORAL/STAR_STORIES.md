# STAR Stories — Worksheet

A working document, not a finished one — fill in the bracketed prompts with real specifics before your interviews. Having 5-6 of these fully written out (not just outlined in your head) measurably improves delivery under pressure.

---

## Story 1: Ownership Beyond Scope (Internship Ticketing Module)
- **Situation:** [What was the state of the company's internal process before the ticketing module?]
- **Task:** [What specifically was requested, and when — after the internship had formally ended]
- **Action:** [What you actually built, and how you managed doing it outside a formal engagement]
- **Result:** [Concrete outcome — was it adopted, what changed operationally]
- **Maps to:** ownership, initiative, "why should we hire you"

## Story 2: Solving a Genuinely Hard Concurrency Problem (Hermes)
- **Situation:** building an event-sourced store that needed to handle concurrent writers safely
- **Task:** guarantee exactly one writer wins on a version conflict, cleanly reject the rest
- **Action:** [your actual implementation process — did you research the pattern first, iterate on the conditional-write logic, how did you validate it]
- **Result:** the verified 100-concurrent-append test: exactly 1 success, 99 clean `ConcurrencyException` rejections
- **Maps to:** difficult technical problem, project you're proud of

## Story 3: Achieving Correctness Under Unreliable Delivery (Task Scheduler)
- **Situation:** [what prompted the load test — was this planned validation, or did an early version show a problem?]
- **Task:** prove the system didn't lose or duplicate tasks under real concurrent load
- **Action:** [the specific engineering — the claiming mechanism, how you instrumented the row-count convergence check]
- **Result:** 10,000-task load test, 0 DLQ messages, 0 task loss, 0 orphaned rows
- **Maps to:** difficult technical problem, working under pressure (if the load test had a tense debugging period)

## Story 4: Learning a Genuinely Unfamiliar Domain Fast (NetSight)
- **Situation:** [what was your starting knowledge of eBPF/lock-free programming/AF_PACKET before this project?]
- **Task:** build a zero-copy, lock-free packet-capture engine from that starting point
- **Action:** [how did you actually learn this — specific resources, what you built first as a smaller proof of concept, what broke along the way]
- **Result:** the validated system — 43,442 connections at ~1,448 conn/s, 0 failures
- **Maps to:** learning quickly, difficult technical problem, project you're proud of

## Story 5: Translating a Messy Business Process (Internship Configurator)
- **Situation:** the manual, email-based quoting workflow
- **Task:** [who asked for this — was it your own initiative or a stated need? what was the actual pain being felt?]
- **Action:** [how you gathered requirements from a non-technical stakeholder, designed the configurator + lead pipeline]
- **Result:** [concrete before/after — response time, error rate, whatever was actually measured or observed]
- **Maps to:** ownership, working with stakeholders, "why this role" (if the role involves cross-functional work)

## Story 6: Community / Leadership (Geek Room MSIT)
- **Situation:** [what was Geek Room's role/activity level when you joined]
- **Task:** [your specific role in organizing DSA/coding competitions]
- **Action:** [what you actually did — organizing, judging, mentoring, promoting]
- **Result:** the Letter of Appreciation, and — more importantly — [any concrete outcome: turnout, participant feedback, repeat events]
- **Maps to:** leadership, collaboration, "tell me about yourself"

---

## A Story You Still Need to Supply Entirely From Memory: Failure / Disagreement
Neither your resume nor this knowledge base can supply this — no achievement bullet reads as a "failure" story, by design. Spend 15 minutes before your first interview recalling a genuine one (a wrong technical call, a missed deadline, a disagreement with a teammate or stakeholder you didn't handle perfectly at first) and write it out in full STAR form here.

- **Situation:**
- **Task:**
- **Action:**
- **Result (including what you changed afterward):**

### Quick Revision
- Six strong story skeletons above are resume-backed — the missing pieces are yours to fill from real memory, not invent.
- A genuine failure/disagreement story is the one gap this document can't close for you — prepare it deliberately, don't improvise it live.
- Write these out in full sentences before the interview, not just bullet points in your head — it measurably changes delivery quality under pressure.
