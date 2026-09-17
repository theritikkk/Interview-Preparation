# RF / Cellular Modem Software Engineering — Master Curriculum

A living roadmap from backend/systems engineering to job-readiness for **PHY, baseband, modem, and protocol-stack software** roles (Qualcomm, MediaTek, Samsung, Apple, Nokia, Ericsson, Broadcom, Keysight, Rohde & Schwarz, and similar). Built from a real software background and from current job postings researched in September 2026. This is meant to be revisited and edited as you progress — not read once and forgotten.

**Explicitly out of scope:** analog RF IC design. **In scope:** the software/DSP/embedded layer that sits between "IQ samples" and "a working cellular link."

**How to use this book:** the numbered chapters (1–16) are reference material — read them once for orientation, then return to them as needed. The `lessons/` folder is where the actual teaching happens, one topic at a time, each following a fixed 17-part format (intuition → math → derivation → code → debugging exercise → interview question → exit test). New lesson files get added here as you progress — this folder is meant to grow.

---

## Table of Contents

### Reference chapters
1. [Career Map](01-career-map.md) — role families, skill clusters by seniority, company-specific notes, the AI/automation angle
2. [Linux for RF Software](02-linux-for-rf-software.md) — exactly which kernel concepts matter, mapped to *Linux Kernel Development* (Robert Love)
3. [Roadmap Overview](03-roadmap-overview.md) — the 14-stage curriculum, with a diagram and a stage-by-stage table
4. [Book & Resource Stack](04-book-and-resource-stack.md) — a small, curated, classified book list
5. [3GPP Specification Reading Guide](05-3gpp-specification-reading-guide.md) — how engineers actually use the standards
6. [SDR Laboratory Progression](06-sdr-laboratory-progression.md) — from pure simulation to hardware-in-the-loop, with hardware/legal guidance
7. [Major Projects & Portfolio](07-major-projects-and-portfolio.md) — 12 projects, 5 flagged as the flagship portfolio
8. [Source-Code Reading Guide](08-source-code-reading-guide.md) — how to navigate srsRAN, OpenAirInterface, GNU Radio, Osmocom
9. [Debugging Training Framework](09-debugging-training-framework.md) — the symptom-first method used throughout the lessons
10. [Interview Preparation Framework](10-interview-preparation-framework.md) — how to reason through questions, not memorize answers
11. [AI & Automation](11-ai-and-automation.md) — durable reference version of the automation-resistance analysis
12. [Job-Description Decoder](12-job-description-decoder.md) — how to tell what a posting actually is, plus search keywords
13. [12–18 Month Plan](13-twelve-to-eighteen-month-plan.md) — month-by-month execution plan
14. [Weekly Study System](14-weekly-study-system.md) — the day-by-day rotation
15. [Progress Checklist](15-progress-checklist.md) — stage-by-stage checkboxes
16. [Glossary](16-glossary.md) — core terms, grows as lessons introduce new ones

### Lessons (taught one at a time, in order)
- [Lesson 1 — Complex Numbers, Phasors, and Why Radio Needs Two Dimensions](lessons/lesson-01-complex-numbers-and-phasors.md) *(Stage 0)*

---

*Start at [Chapter 1](01-career-map.md) for orientation, or jump straight to [Lesson 1](lessons/lesson-01-complex-numbers-and-phasors.md) if you already know why you're here.*
