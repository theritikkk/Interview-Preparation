# Screening Rounds by Company Type

---

## What to Expect, By Company Type

| Company Type | Typical Emphasis | Likely Format |
|---|---|---|
| **Product Companies** | Deep technical rounds, DSA + system design + resume deep-dive | Multiple rounds: DSA screen → technical deep-dive → system design (for more senior) → behavioral |
| **Startups** | Practical ability, breadth, speed of learning | Often fewer, more conversational rounds; heavier weight on project deep-dive and machine coding over pure DSA; may ask you to build something small live |
| **Service Companies** | Broader CS fundamentals, aptitude, sometimes less DSA-intensive | May include an aptitude/MCQ round before any coding; strong fundamentals (`03_CS_FUNDAMENTALS/`) matter more than niche algorithm knowledge |
| **Big Tech** | Standardized, DSA-heavy, structured rubrics | Multiple DSA rounds with consistent format, at least one system design round, a dedicated behavioral round often scored separately |
| **Fintech** | Correctness, precision, security awareness | Expect extra scrutiny on your "exactly-once"-type claims (`13_PROJECT_DEEP_DIVES/DISTRIBUTED_TASK_SCHEDULER.md`) — precision under questioning matters more here than almost anywhere else |
| **Cloud Companies** | Systems depth, AWS/infrastructure fluency | Your Hermes/AWS depth (`08_AWS/`) is directly relevant; expect deeper infrastructure-tradeoff questions than a typical product company would ask |
| **Enterprise Software** | Broad stack fluency, some legacy-system reasoning | May probe SQL/DBMS fundamentals harder than newer companies; process/documentation-mindedness sometimes valued explicitly |

---

## Common Screening Formats

- **Aptitude/MCQs:** more common at service companies and some big-tech initial screens — quantitative reasoning, basic CS trivia, sometimes not coding at all.
- **CS Fundamentals round:** OS/DBMS/Networking/OOP questions, often verbal, no coding — `03_CS_FUNDAMENTALS/` is your direct prep.
- **Coding assessment (often via HackerRank/Codility/similar):** timed, autograded — `02_DSA/PATTERNS.md` pattern recognition is your speed advantage here.
- **Debugging round:** given broken/unfamiliar code, find and fix the bug — `15_INTERVIEW_PREP/DEBUGGING.md`.
- **SQL round:** live query-writing, sometimes against a real schema — `01_PROGRAMMING/SQL.md`.
- **Frontend / Backend round:** framework-specific live coding or design — `04_BACKEND/`, `05_FRONTEND/`.
- **Machine coding:** build a working, reasonably complete mini-system in 45-90 minutes — `15_INTERVIEW_PREP/MACHINE_CODING.md`.
- **LLD (Low-Level Design):** class/interface design for a system, without necessarily writing full working code — same prep as machine coding, less implementation, more design-and-justify.
- **System design:** `11_SYSTEM_DESIGN/`.
- **Resume deep-dive:** `13_PROJECT_DEEP_DIVES/`.
- **Behavioral:** `14_BEHAVIORAL/`.

### Interview Questions
- **L2:** "How would you prepare differently for a fintech screen versus a startup screen, given the same resume?" — for fintech, over-prepare the precision/rigor angle on your quantitative claims (`13_PROJECT_DEEP_DIVES/`'s claim-defense sheets); for a startup, be ready to talk speed of execution and breadth, and expect the interview to feel more like a technical conversation than a rubric-driven exam.

### Quick Revision
- Big tech: standardized, DSA-heavy, multiple structured rounds.
- Startups: practical, conversational, project-deep-dive-weighted.
- Fintech: expect the highest scrutiny on your exactly-once/correctness claims specifically.
- Cloud companies: your Hermes/AWS depth is a direct, strong match.
