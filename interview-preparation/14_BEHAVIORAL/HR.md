# Behavioral / HR

Every answer framework below is built to be filled with **your actual experience** — nothing here invents an achievement. Where the resume gives enough to suggest a strong story, it's noted; where it doesn't, that's flagged so you know to prepare it from memory rather than expect it here.

---

## The STAR Framework (Quick Reference)

**S**ituation — brief context, 1-2 sentences. **T**ask — what you specifically needed to do. **A**ction — what *you* did (not "we" — interviewers are evaluating your individual contribution). **R**esult — the outcome, ideally with a concrete detail, and (bonus) what you learned or would do differently.

Common mistake: spending 80% of the answer on Situation/Task and rushing Action/Result — Action is where the actual signal is; budget your time accordingly (roughly S:10%, T:10%, A:60%, R:20%).

---

## Question Bank

### "Tell me about yourself" / "Walk me through your resume"
See `RESUME_WALKTHROUGH.md` for a full scripted structure built from your confirmed timeline and projects.

### "Why software engineering?"
This one is genuinely yours to answer — resist the urge to give a generic "I love solving problems" answer; the strongest version of this answer is specific to something that actually pulled you in (a first project, a specific kind of problem you find satisfying) rather than a template.

### "Why this company?" / "Why this role?"
These require research specific to each company you interview with — a template answer signals low genuine interest. The one durable component you can prepare in advance: connecting the role's likely technical challenges to your actual strengths (e.g., if a role touches distributed systems or cloud infrastructure, your Hermes/Task Scheduler background is directly relevant — say so specifically, not generically).

### "Tell me about a difficult technical problem"
🔴 Strong candidate from your resume: the concurrency/correctness challenges in either Hermes (the 100-concurrent-append optimistic-concurrency test) or the Task Scheduler (achieving the exactly-once *effect* despite at-least-once delivery — see `13_PROJECT_DEEP_DIVES/DISTRIBUTED_TASK_SCHEDULER.md`). Structure via STAR: Situation (building the event store/scheduler), Task (guarantee correctness under concurrency), Action (the specific technical choice — conditional writes, or claim-locking), Result (the verified outcome — 1 succeeds/99 rejected, or 0 orphaned rows).

### "Tell me about a failure"
This must come from your own memory — a genuine failure, not a humble-brag disguised as one ("my biggest weakness is I work too hard"). A real technical or judgment misstep, what you learned, and concretely how you've applied that lesson since.

### "Tell me about a disagreement"
Same — must be genuine and specific. If you don't have a clear one from your internship or project work in memory yet, spend time recalling one before the interview rather than improvising in the moment.

### "Tell me about something you learned quickly"
🔴 Strong candidates from your resume: any of NetSight's genuinely advanced systems topics (eBPF/BTF CO-RE, lock-free programming) if you learned them specifically for this project rather than from prior coursework — a concrete "I didn't know X, here's how I learned it well enough to build with it" story is compelling precisely because these are hard topics.

### "Tell me about a project you're proud of"
You have three strong, genuinely distinct choices (Hermes, Task Scheduler, NetSight) — pick based on what the interviewer/role seems to value most (ask if unsure, or read the JD), and be ready to go deep immediately, since "proud of" answers often trigger the most aggressive technical follow-up questioning.

### "Tell me about leadership" / "Tell me about ownership"
🔴 Two strong resume-backed answers: (1) your internship's "shipped an internal ticketing module *after* the internship ended, at the company's request" is explicitly framed by your own resume as an ownership signal — use it. (2) Geek Room MSIT's DSA/coding competition organizing is a genuine, if lighter-weight, leadership/community story.

### "Tell me about debugging something unfamiliar"
See `15_INTERVIEW_PREP/DEBUGGING.md` for the systematic framework — pair it with a real story, likely from NetSight (C debugging via GDB/Valgrind) or your internship (the "hardest bug" question in `13_PROJECT_DEEP_DIVES/INTERNSHIP.md`).

### "Tell me about working under pressure"
Your load-testing work on the Task Scheduler and NetSight (getting to a stable, validated result under load) is a plausible technical-pressure story if there's a genuine "this needed to work and I wasn't sure it would" moment in your memory of building it — fill in the real specifics.

### "What is your biggest weakness?"
Must be genuine, specific, and paired with concrete action taken to address it — generic "I'm a perfectionist" answers read as evasive to experienced interviewers.

### "Why should we hire you?"
Synthesize, don't repeat: the strongest version of this answer names 2-3 specific, differentiated strengths (e.g., genuine systems-programming depth most entry-level candidates don't have, demonstrated end-to-end ownership from the internship, breadth across the full stack) rather than a generic "I'm hardworking and a fast learner."

### "Where do you see yourself in five years?"
Should connect to a genuine trajectory (deepening in backend/distributed systems? broadening toward staff-level full-stack ownership?) rather than a vague non-answer — pick something honestly consistent with what your resume's project choices already suggest about your interests.

### Quick Revision
- STAR: budget most of your time on Action and Result, not Situation/Task.
- Ownership and technical-difficulty questions have strong, resume-backed answers ready — use them.
- "Why this company/role" needs company-specific research every time — never fully pre-scriptable.
- Failure/disagreement/weakness questions must be genuine — this file can't supply them, only the structure to tell them well.
