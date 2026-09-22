# Resume Claim Defense — Index & Template

Full defense sheets for every quantitative claim on your resume already live inside each project's deep-dive file — this file is the index, plus a reusable template for defending any claim (including ones that come up conversationally that aren't pre-written anywhere).

---

## Where Each Claim's Full Defense Lives

| Claim | File |
|---|---|
| 100 concurrent appends, 1 succeeds / 99 rejected | `13_PROJECT_DEEP_DIVES/HERMES.md` |
| 285 automated tests, 51+ suites, 0 failures | `13_PROJECT_DEEP_DIVES/HERMES.md` |
| 18 Terraform modules | `13_PROJECT_DEEP_DIVES/HERMES.md` |
| 10,000 simultaneous tasks, 8,562 peak queue depth, 1,600 msg/s burst | `13_PROJECT_DEEP_DIVES/DISTRIBUTED_TASK_SCHEDULER.md` |
| ~7m46s drain, 0 DLQ, 0 task loss | `13_PROJECT_DEEP_DIVES/DISTRIBUTED_TASK_SCHEDULER.md` |
| "Exactly-once" delivery (the most important precision question in this whole knowledge base) | `13_PROJECT_DEEP_DIVES/DISTRIBUTED_TASK_SCHEDULER.md` |
| p95 latency 160ms → 76ms | `13_PROJECT_DEEP_DIVES/DISTRIBUTED_TASK_SCHEDULER.md` |
| ~9,700 LOC / 11 modules | `13_PROJECT_DEEP_DIVES/NETSIGHT.md` |
| 43,442 connections @ ~1,448 conn/s, 0 failures | `13_PROJECT_DEEP_DIVES/NETSIGHT.md` |
| 60-min endurance, <0.2% memory growth, 0 FD leaks | `13_PROJECT_DEEP_DIVES/NETSIGHT.md` |

---

## The Generic Defense Template — Use This for Any Claim Not Listed Above

For any number on your resume (or one an interviewer surfaces from your GitHub/linked evidence), walk through these ten questions before the interview:

1. **How was the test/measurement actually generated?** (tool, script, methodology)
2. **What machine/environment did it run on?** (matters for whether the number is meaningful in absolute terms or only relatively)
3. **What was the actual workload/input?** (synthetic? realistic? adversarial?)
4. **What metrics were captured, and how?**
5. **How was correctness verified** — not just "it completed," but *what specifically was checked* to confirm the result was correct?
6. **How would loss/failure have been detected**, if the claim is about zero loss/zero failures — what's the actual detection mechanism, and could it have missed something?
7. **What would happen at 10x the scale?** — an honest, reasoned extrapolation, naming the component you'd expect to become the new bottleneck.
8. **What assumptions does the number depend on?** (e.g., network conditions, a specific hardware profile, a specific data shape)
9. **How reproducible is it?** — deterministic outcome, or does it vary run to run (and if it varies, what's the actual claim — a typical case, a best case, a worst observed case)?
10. **What's the precise, technically defensible way to state the claim**, versus the resume's necessarily-compressed bullet-point phrasing? (This is the "exactly-once" lesson generalized — a resume bullet is optimized for brevity, not precision; your job in an interview is to supply the precision the bullet couldn't.)

### Interview Questions
- **L2:** "Why does question 10 matter so much?" — because the gap between a resume's compressed phrasing and the technically precise truth is exactly where interviewers probe hardest, and where candidates most often get caught either overclaiming or unable to articulate the distinction — see the Task Scheduler's "exactly-once" treatment as the canonical example of closing this gap well.

### Quick Revision
- Every major quantitative claim already has a full defense sheet in its project file — read those before an interview, don't rebuild them from scratch.
- For anything new, the same ten questions apply — methodology, environment, workload, verification, detection, extrapolation, assumptions, reproducibility, and precise restatement.
- The single most valuable habit from this whole file: always know the technically precise version of your resume's compressed claim, and lead with precision rather than the bullet's exact wording when asked to defend it.
