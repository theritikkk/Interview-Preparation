# Assessments — Question Bank and Mock Timed Tests

> **Purpose:** build exam speed and interview recall on the same material you learned in the phases. Three timed mocks plus a large question bank, with answers you can score yourself against.

## Stated assumptions about the RTDS online assessment

The actual RTDS OA format is **not known from primary evidence** — the original job-description attachment is not in this repository, and no official OA sample is available. Everything below is a stated assumption, not a claim about the real test. The working JD (a reconstruction, labeled as such in [00-role-and-strategy](../00-role-and-strategy/)) names skills in Linux, networking, cloud/virtualization, and scripting; the mocks are built around **those skills**, not around an invented format.

| Assumption | Basis | If it turns out wrong |
|---|---|---|
| Duration 60–120 minutes, online, proctored or not | Standard for infra internships | Adjust pacing; the question bank drills work at any length |
| Mix of MCQ + short answer + one or two scenario questions | Common OA shape for infra roles | Scenario practice transfers to either MCQ or written |
| Passing bar around 70–80% | Typical screening bar | Target ≥75% on mocks regardless |
| Tools: a browser-based editor; no real VM access | Most OAs are not hands-on | If a real terminal is provided, the phase labs are the prep |
| No GPU/AI-specific hands-on in the OA | The JD reads as infra, not ML | Phase 9 still matters for interviews, which go deeper than OAs |

**What we do not claim:** the number of questions, the exact topics, negative marking, or the platform. When you learn the real format, update this table — and use the mocks for speed practice, which transfers either way.

## How to use this folder

1. **Question bank first** (`01-question-bank.md`) — as recall drills during the phase study, not as a cram sheet. Cover the answer, answer aloud, then check.
2. **Mock 1** (`02-mock-assessment-1-linux-networking.md`) at the end of week 4 — Linux, networking, Bash. 60 minutes, closed book. Target ≥75%.
3. **Mock 2** (`03-mock-assessment-2-automation-openstack.md`) around week 13–16 — Python/Ansible, observability, OpenStack. Target ≥75%.
4. **Mock 3** (`04-mock-assessment-3-full-stack.md`) around week 17–24 — AWS, security, SLOs, CI/CD, plus the strongest scenarios from every phase. This one is the "final": treat it as the interview rehearsal.
5. After every mock: write the **top 3 weak areas** in your progress log, and repair them with the corresponding phase notes + labs before the next attempt. A mock without a repair pass is entertainment.

## Scoring discipline

| Rule | Why |
|---|---|
| Closed book, timer visible | OA simulation; open-book study builds the wrong reflex |
| Score with the answer key's *reasoning*, not just the letter | Interviews probe reasoning; letters are forgettable |
| A wrong answer on a concept (not a typo) means re-reading that phase's note | Prevents systematic gaps |
| Log every attempt (date, score, weak areas) | The progress checklist tracks readiness by evidence |
| Never take two mocks on the same day | Retention beats volume |

## Files

| File | Scope | Time | Points |
|---|---|---|---|
| [01-question-bank.md](01-question-bank.md) | All phases, ~140 questions with answer keys | Untimed drills | n/a |
| [02-mock-assessment-1-linux-networking.md](02-mock-assessment-1-linux-networking.md) | Linux + networking + Bash | 60 min | 100 |
| [03-mock-assessment-2-automation-openstack.md](03-mock-assessment-2-automation-openstack.md) | Python/Ansible + observability + OpenStack | 75 min | 100 |
| [04-mock-assessment-3-full-stack.md](04-mock-assessment-3-full-stack.md) | AWS + security + SLO + CI/CD + all-phase scenarios | 90 min | 120 |

## The link to interviews

Every mock question maps to a likely interview question. When you miss one, write the interview phrasing of the same concept in [12-interviews](../12-interviews/README.md). Example: a mock question about `ss -s` becomes the interview question "walk me through how you'd check connection states on a box under load." The mocks build recall; the interviews build narrative. Both come from the same notes.
