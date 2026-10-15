<!-- Certa FDE mastery workbook — split part 7 of 7 (source lines 4055-4482). Concatenating all 7 files in order reproduces certa_fde_interview_mastery.md. -->

## Part 13: Final revision cheat sheet

> Print or copy this part alone for the last 48 hours. Everything here is a compression of the earlier parts — if a line doesn't trigger the memory behind it, go back to the referenced section.

### 13.1 HTTP at a glance (Part 2)

| Method | Safe | Idempotent | Use for |
|---|---|---|---|
| GET | ✅ | ✅ | Read; never mutate |
| POST | ❌ | ❌ | Create in a collection; needs an idempotency key to be retry-safe |
| PUT | ❌ | ✅ | Full replace at a known URL |
| PATCH | ❌ | ❌ (usually) | Partial update |
| DELETE | ❌ | ✅ | Remove; second call may 404, state unchanged |

| Status | Meaning to a client | Retryable? |
|---|---|---|
| 400 | I sent a malformed/invalid request | No — fix payload |
| 401 | Not authenticated | Only after fixing/refreshing credentials |
| 403 | Authenticated, not allowed | No — fix the grant |
| 404 | No such resource | No (fix the ID) |
| 409 | State conflict / key reuse with different body | Depends — resolve the conflict |
| 429 | Slow down | Yes — honor `Retry-After` + backoff + jitter |
| 500/502/503/504 | Server-side | Yes — bounded retries |
| 201 | Created | Success — capture `Location` |

### 13.2 Auth quick rules (Part 2.4)

- **OAuth 2.0:** client credentials (machine-to-machine) · authorization code + PKCE (acts as a user; get a refresh token once) · implicit & password grants are deprecated/legacy.
- **Refresh-then-401 = terminal.** Pause visibly, alert, do not loop.
- **Least privilege:** request only the scopes needed; document each scope's purpose.
- **Secrets:** secret manager, rotation plan, never in code or logs.
- **Webhooks are inbound auth:** HMAC signature over the *raw* body, constant-time compare.

### 13.3 Pagination decision (Part 2.5)

- **Offset:** simple, random-access, breaks under concurrent writes (dupes/skips), degrades on deep pages. Fine for human UIs on stable data.
- **Cursor/keyset:** `WHERE (updated_at, id) > (last_updated_at, last_id) ORDER BY updated_at, id LIMIT n` — stable, index-friendly, no random access, cursors are opaque state.
- **Sync rule:** pipelines walk a watermark, not a page counter.

### 13.4 SQL quick rules (Part 3)

- Logical evaluation: `FROM → WHERE → GROUP BY → HAVING → SELECT → ORDER BY → LIMIT` (window functions after HAVING, before ORDER BY).
- `NULL` is not a value: `col = NULL` never matches; use `IS NULL`; `NOT IN` with NULLs yields no rows — use `NOT EXISTS`.
- `COUNT(*)` counts rows (including NULL-padded join rows); `COUNT(col)` counts non-NULLs; `COUNT(DISTINCT col)` counts distinct non-NULLs. **The LEFT JOIN trap that faked the assessment answer: Europe 3/5 vs the COUNT(*) variant.**
- Join grain: know the join's cardinality before aggregating; fan-out duplicates rows (`GROUP BY v.id` or pre-aggregate).
- Composite index rule: **equality columns first, then the range/sort column** — `(tenant_id, status, updated_at)` for `WHERE tenant_id=? AND status=? ORDER BY updated_at DESC`.
- Anti-join pattern: `LEFT JOIN x ON … WHERE x.id IS NULL`.
- Window: `ROW_NUMBER() OVER (PARTITION BY … ORDER BY …)` for latest-per-group; `RANK` vs `ROW_NUMBER` (ties!).
- Read `EXPLAIN` before and after any index change.

### 13.5 Python interview routine (Part 4)

1. **Restate** the task in one sentence.
2. **Clarify** (write the questions down before coding): ties? empty input? duplicates? types? absent = 0 or absent? mutation allowed? return shape?
3. **State the plan** + complexity target (usually one pass O(n), O(k) space).
4. **Code the skeleton**, then edge cases.
5. **Dry-run** two tiny examples by hand, out loud — including the tie and empty cases.
6. **State complexity** and one upgrade path ("would become O(n log n) if we needed ranked output").
7. Never leave an untested mental branch; a hash-set/dict is usually the move.

### 13.6 Reliability: problem → mechanism (Parts 5–7)

| Problem | Mechanism |
|---|---|
| Duplicate delivery | Unique key / inbox table with PK dedupe; idempotent effects |
| Same key, different payload | Strict mode: 409 `idempotency_key_reuse` (body hash compare) |
| Out-of-order events | Per-entity version/sequence; ignore stale (v6 after v7 → `stale_ignored`); never global order |
| Crash before ack | At-least-once redelivery + idempotent consumer |
| Crash mid-batch | Per-record status + checkpoints; resume; re-verify in-flight |
| Provider overload | Backoff + full jitter + cap + max attempts; honor `Retry-After`; circuit breaker |
| Terminal failures | DLQ with preserved payload + context; triage; controlled replay (replay is idempotent) |
| Partial batch failure | Retry the failed subset only; per-record outcomes, never "batch failed" as the only signal |
| Lost side effects between DB and queue | Transactional outbox (DB commit includes outbox row; publisher drains it) |
| Multiple workers, same row | Atomic claim: `UPDATE … WHERE status='pending'` affected-rows check; `FOR UPDATE SKIP LOCKED` |
| Silent auth death | Alert on refresh failure; pause with visible status |
| "Sync correct?" question | Reconciliation report: counts + field-level diffs + dispositions |

**Backoff sequence (remember one):** base 0.5s, cap 30s → 0–0.5s, 0–1s, 0–2s, 0–4s, 0–8s, … capped. Jitter desynchronizes workers so retries don't stampede.

**Exactly-once language:** delivery is at-least-once; *processing* is effectively-once via dedupe + idempotency; never claim "the queue makes it exactly-once and we're safe."

### 13.7 Sync and integration rules (Part 7)

- **Field ownership table first** — every shared field: who wins, what happens on conflict.
- **Delta, then reconcile:** pull deltas, but reconcile periodically against a full comparison — deltas miss what they never heard about.
- **Matching is the hard problem:** stable external IDs > composite natural keys > fuzzy names. Normalize before comparing (case, whitespace, punctuation).
- **Conflicts:** detect via version/`updated_at` mismatch → 409 → policy (source-of-truth, field-level merge, or human queue). Log every conflict *with its disposition*.
- **Deletes:** tombstones or explicit delete events; silent resurrection is the classic bug.
- **Rollout:** dry run → shadow mode → pilot subset → go/no-go → cutover → hypercare. Name the rollback owner in advance.

### 13.8 Debugging: hop list + signatures (Part 8)

**Hops:** scheduler → source API → transport → transform → our DB → outbound API → target → target's downstream view. **Bisect the hops; predict evidence before collecting it.**

| Symptom | First suspects |
|---|---|
| Job green, data stale | Watermark lag; poison record; mapping of new field; skipped subset |
| Duplicates appearing | Write path idempotency; retry after lost ack; natural-key normalization |
| Everything slow after a deploy | Log volume on hot path; sync I/O; N+1; p50 vs p99 split |
| 429s / throttling | Burst batching; no adaptive pacing; unhonored Retry-After |
| Auth errors after working weeks | Refresh path swallowed; token rotation; silent scope removal |
| Only one customer affected | Config/mapping drift; tenant-specific data shape; their admin changed something |

Always: define symptom scope/timing/blast radius first; alert on *progress* (watermark age), not only on errors.

**Incident comms:** impact in customer language → what's known/unknown → next update time. Bad news: early, options, recommendation, written follow-up.

### 13.9 LLM extraction checklist (Part 9)

- Constrained output + explicit required-field validation (the model will emit schema-violating JSON).
- Repair loop that lists *exactly* which fields failed — general feedback wastes attempts.
- Evidence snippets verbatim-checked against the source; fabricated quote = failure (caught in the lab).
- Deterministic checks the model can't be trusted with: IDs, dates, sums, cross-field consistency.
- Confidence + routing rules to human review; reviewer UX shows evidence, deep link, previous value.
- Golden set with per-field precision/recall + **unsupported-claim rate**; every production bug becomes a fixture.
- Cost/latency: cache by document hash, tier models, chunk, batch; measure before promising.
- Prompt injection: documents are untrusted input — instructions inside documents are data, never commands.

### 13.10 Discovery and delivery phrases (Part 10)

- Open: *"I want to understand how this really runs today, agree what success looks like in numbers, and leave with assumptions to verify in writing."*
- Best question: *"Show me the last one you did."*
- Hidden process: *"What do you do when the tool is down?"*
- Done: *"How will you know, three months from now, that this worked?"*
- Conflict: *"If we can't agree, who decides?"* then write it down same day.
- Scope: *"Yes — and here's what I'd change to keep the date."*
- Escalate a **decision**, never a complaint: situation/impact → what I need → options.
- Rollout: shadow mode + reconciliation gate + pilot + named rollback owner + defined hypercare exit.

### 13.11 Numbers and one-liners worth having ready

- 50k vendors, ~2% touched/day ⇒ ~1k delta rows/night — trivial for delta, dangerous for full reloads done wrong.
- 40k docs × 8¢ = $3,200; × 12s ≈ 133h of serial compute ⇒ parallelism/batching is a design input, not an afterthought.
- "Delivery is at-least-once; **processing** is what I make exactly-once."
- "The release note tells you intent; the profiler tells you behavior."
- "Succeeded" is a statement about the job; "fresh" is a statement about the data. Alert on the one the customer feels.
- "I'd rather give you a smaller thing that works than a bigger thing that breaks."

### 13.12 Domain vocabulary: third-party risk in one table

| Term | One-line meaning | The trap |
|---|---|---|
| **Tiering** | Ranking vendors by risk (data access, spend, criticality) to decide how much diligence each gets | Tier definitions differ per customer — always ask for theirs in discovery |
| **Onboarding** | Intake → due diligence → risk review → approval → record creation | This is the cycle-time metric customers quote in the target-state conversation |
| **Continuous monitoring** | Ongoing refresh of risk signals (expiring certificates, new breaches, periodic reassessment) | Where processes decay; "set and forget" fails audits |
| **Offboarding** | Termination checks: access revocation, data return/deletion, final records | The most-forgotten phase, and auditors ask for its evidence |
| **Subprocessor / fourth party** | A vendor's own vendor that touches your data | Extends your risk surface; must be declared in contracts |
| **DPA** | Data Processing Agreement — contractual terms for processing personal data | Often gates the integration's legality, not just paperwork |
| **Attestation** | Evidence a vendor's controls work: SOC 2 report, ISO 27001 certificate, questionnaire response | Attestations expire — that is *why* continuous monitoring exists |
| **Inherent vs residual risk** | Risk before vs. after controls and tiering | Reviewers argue about scores; this distinction resolves half of those arguments |
| **ESG** | Environmental/social/governance questionnaires in supplier review | A separate workflow from security review — don't conflate them |

*[BEST PRACTICE] vocabulary only — definitions differ slightly by program. Confirm the customer's own definitions during discovery instead of assuming.*

---

## Part 14: Answer scripts library

> Rehearse these until they sound spoken, not read. Bracketed `[facts]` are yours to fill with real numbers — invented numbers are worse than no numbers, because follow-ups expose them.

### 14.1 "Tell me about yourself" (90 seconds)

> "I'm a final-year B.Tech IT student, and I've spent the last two years building backend systems rather than coursework demos — a distributed task scheduler and a microservices chat platform, both Node/TypeScript/Postgres. The task scheduler taught me reliability the hard way: durability before acknowledgement, leased workers, retries with backoff, and a dead-letter path when retries are exhausted — a crashed worker doesn't lose work, it reappears and must be safe to re-run. The chat platform taught me cross-system debugging: tracing one message through three services, handling duplicate delivery after restarts, and thinking in per-conversation order instead of global order. What pulls me toward forward-deployed engineering is that both of those skills — building the reliable core, and debugging across boundaries you don't own — only matter if the thing actually lands with the customer's process, and that combination is what I want to do full-time. I'm early in the customer-facing part of my career, and I've been deliberately practicing it — running discovery-style conversations, writing current-state maps, and staying honest about what I know versus what I'm assuming."

**Why it works:** proof before claims; names the gap without apologizing; ends on the role, not on a resume chronology. **Don't:** list every technology, recount your degree, or say "I'm passionate about technology."

### 14.2 Project deep dive — distributed task scheduler (60 seconds)

> "The system runs background jobs across a pool of workers. The design decision I'd highlight is that a task is written durably *before* it's acknowledged, and workers claim tasks with a lease — a visibility timeout. If a worker crashes mid-execution, the lease expires and the task reappears; because that redelivery is guaranteed by design, the contract with task authors is that handlers must be idempotent, and the scheduler gives them tools: a claim step that's atomic, and a dedupe key convention. Retries use exponential backoff with jitter, and after the attempt budget is exhausted the task moves to a dead-letter path with its failure context preserved — payload, error, attempt history — so it can be triaged and replayed without guesswork. The metrics I'd look at in an incident are exactly the ones I built: queue depth, oldest task age, retry rate, and dead-letter depth."

**Why it works:** every sentence maps to a mechanism an FDE interviewer probes (Parts 5–7). **Interviewer follow-up guaranteed:** "What happens if the ack is lost after the work is done?" → redelivery; that's why handlers are idempotent; the dedupe key makes the re-run a no-op.

### 14.3 Project deep dive — chat platform (60 seconds)

> "It's a microservices chat platform — publishing, fan-out through a broker, delivery, persistence. The interesting problems were all distributed-systems problems. Ordering: there is no global order, so I reasoned per-conversation — messages carry a sequence, and receiving services ignore anything older than what they've seen, which is the same stale-guard pattern you need in any event-driven integration. Duplicates: a consumer restart can cause redelivery, so persistence uses an idempotent write keyed by message ID. And debugging: when a message doesn't arrive, I bisect the hops — was it published, did the broker fan it out, did the consumer process it, is it in the database, did the client see it — and each hop has evidence. That hop-bisection habit is the thing I'd bring to an integration between two systems I *don't* own, where you can't read the code on the far side, only observe it."

**Why it works:** it converts coursework into the exact competencies Parts 7–8 test. **Don't:** explain the app's features; nobody grades the UI.

### 14.4 "Why forward-deployed engineering? Why this role?"

> "Two reasons, and they're specific. First, I like owning outcomes, not tickets — in both my projects the interesting work was the part where the system met reality: retries, duplicates, partial failures, the parts nobody specifies. That's most of an FDE's day. Second, I want a tight feedback loop with the people the software serves. The fastest I've learned was when a stakeholder told me my design was wrong about how *they* actually worked — it's uncomfortable and it's the highest-signal feedback that exists. The parts I need to grow into are the Fortune 500 stakeholder dynamics — compliance, procurement, security review — and I'd rather learn that with real customers than speculate about it from an engineering seat."

**Why it works:** names the actual job content (outcome ownership, ambiguity, customer feedback), and states a growth area with direction. **Don't:** say "I want to travel" or "it's a mix of engineering and business" without specifics.

### 14.5 "You have zero years of customer-facing delivery experience. Why should we take the risk?" (the honest script)

> "I don't have paid forward-deployed years — that's true, and I'd rather be precise than pretend. What I can show you is transferable evidence and a plan. Evidence: I've built and operated systems where reliability — durability, retries, idempotency — was my problem, not someone else's, and I can explain every trade-off in them. I've done the customer-facing work at student scale — running review sessions where I had to negotiate scope against a fixed date with faculty stakeholders, and mentoring juniors by making them debug rather than answering for them. Plan: I'd start by shadowing discovery on a real engagement, I'd write up every call as a current-state map plus assumptions and read it back, and I'd keep a RAID log honest from week one. I'd expect my first 30 days to be heavy on listening and light on proposing. If you need someone who already knows the Fortune 500 dynamics, I'm not that person yet — but if you need someone who will be rigorous about the process and learn your motion fast, that's the role I've been training toward."

**Why it works:** honesty + evidence + a first-30-days plan. Interviewers can smell rehearsed bravado; they reward calibrated candidates. **Never:** claim experience you don't have — the follow-up questions will find it.

### 14.6 "What is idempotency?" (with a concrete example)

> "An operation is idempotent if repeating it produces the same end state as doing it once. It matters because every network call has a window where the work succeeded but the acknowledgement didn't arrive — so clients *must* retry, and servers must be safe when they do. Concretely: a client posts a payment with an `Idempotency-Key`; the server stores that key with the outcome in the same transaction as the payment. If the response is lost and the client retries with the same key, the server returns the stored response and no second payment is created. If the same key arrives with a *different* body, that's a client bug, so strict implementations reject it — I verified that case in my lab as a 409. And it applies beyond payments: webhook receivers dedupe by event ID, sync jobs upsert by natural key, and queue consumers dedupe so at-least-once delivery becomes effectively-once processing."

### 14.7 "How do you handle duplicate webhook deliveries?"

> "Assume they will happen — at-least-once is the only honest guarantee across a network. The receiver verifies the signature on the raw body, then durably inserts the event into an inbox table keyed by the provider's event ID; the unique constraint makes the second copy a no-op, and the insert happens in the same transaction as the state change it triggers. If the provider sends version numbers, I guard per entity: a v6 arriving after a v7 is ignored as stale, and I count those — a rising stale count tells me about ordering problems before they become incidents. The endpoint acknowledges fast but only after the durable write, and heavier processing runs async with retries, backoff with jitter, and a DLQ for terminal failures that has a tested replay path. And replay itself goes through the same dedupe, so re-running is boring — which is the goal."

### 14.8 "How would you design a sync between two systems?" (2 minutes)

> "I'd structure it in five moves. First, requirements: which objects, which direction, which fields are owned by whom, freshness target, volumes, and what 'done' means as a number. Second, the model: a stable external ID as the matching key, a per-field ownership table, watermarks and versions for change detection, and an audit trail for every change we make. Third, the normal flow: read deltas from each side using a cursor on `(updated_at, id)`, transform through a mapping layer with a validator that fails loudly at the boundary, and push changes idempotently — writes keyed so a retry can't double-create. Fourth, the failure story, which is where most designs fall: retries with exponential backoff and jitter, adaptive pacing to respect rate limits, per-record status so a crash resumes at the right point, conflicts detected by version mismatch and routed through the ownership policy, and a DLQ with replay for poison records. Fifth, proof: a reconciliation job that compares the two systems — counts and field-level diffs — with every mismatch getting a disposition, plus shadow-mode rollout so we compare predictions against reality before we trust it, then a pilot subset, then cutover with a named rollback owner. The trade-offs I'd state out loud: delta sync needs periodic reconciliation because it can't see what it never heard about; cursor pagination over offset; and at-least-once plus idempotent writes over chasing exactly-once."

### 14.9 "How do you debug a failure across two systems you didn't build?" (90 seconds)

> "First I refuse to start from 'it's broken' — I get the symptom precise: which records, since when, who noticed, what changed around then. Then I list the hops the data travels through and bisect: I check the midpoint and ask 'did the data get this far?' which halves the search instead of walking every component. At each hop I look for evidence that already exists — correlation IDs in logs, queue age, error-class counters, the provider's request logs — and I form one hypothesis at a time, predicting what I'd see before I look, so evidence is a test and not a hunt. Once I know where it stops, the class of fix depends on what failed: a contract change means fixing the mapping, saturation means shedding load and pacing, a logic bug means a deploy, and a provider-side failure means escalating with a correlation ID and a minimal reproduction. Then I replay safely — idempotent by design — and verify in *both* systems, because 'the job succeeded' is not the same as 'the customer's record is right.' Finally I add the guardrail that would have caught it: usually an alert on progress, like watermark age, not on the absence of errors."

### 14.10 "How do you approach SQL performance?"

> "I start by reading the query for its shape: what's the join grain, which predicates are selective, what's being sorted, is there a fan-out that duplicates rows. Then I check the indexes against the access path — equality columns first, then the range or sort column, so the index can serve both the filter and the ordering. Then I look at `EXPLAIN`: is it doing a scan where I expect a seek, is the sort spilling, is the row estimate wildly off. The classic wins are adding the right composite index, rewriting `NOT IN` with NULL-hazard into `NOT EXISTS`, pre-aggregating before a join instead of after, and removing functions on indexed columns in `WHERE`. But I also weigh the cost: every index taxes writes and storage, so I measure before and after instead of indexing reflexively — and if the query is a reporting pattern that only runs at night, I'd rather keep it off the hot path than add indexes that slow the application."

### 14.11 "How do you make LLM extraction production-grade?" (90 seconds)

> "I treat the model as a probabilistic mapper and wrap it in deterministic engineering. First, output is constrained to a schema, and I validate it explicitly — including the required fields, because models will happily return valid-looking JSON with something missing; that's the failure I specifically test for. When validation fails, the repair loop tells the model exactly what failed and where, which converges far faster than 'try again.' Second, grounding: every extracted field carries an evidence snippet, and I verify those snippets exist verbatim in the source document — my lab caught the model quoting a plausible line that was actually from another document, which is exactly the hallucination class that matters in compliance. Third, deterministic checks the model can't be trusted with: IDs, dates, sums, cross-field consistency. Fourth, measurement: a golden set with per-field precision and recall plus an unsupported-claim rate, and every production bug becomes a new fixture. Fifth, routing: low-confidence or rule-violating extractions go to human review, and the reviewer sees the evidence and the previous value, not just the field. The net effect: the model does the mapping, and no unsupported claim can reach the customer's system as data."

### 14.12 "Tell me about a time you handled ambiguity or learned something fast"

> "In the [project/course], the requirement was one sentence — '[actual requirement]' — and everyone assumed it meant the obvious thing. I wrote down the three interpretations, showed each one's consequences, and asked the stakeholder to pick; two of the interpretations would have shipped the wrong artifact. We settled it in ten minutes with a concrete example instead of a debate. The general habit I took from it: when a requirement is ambiguous, generate two or three concrete examples, show the difference in outcomes, and get a decision in writing — ambiguity survives in the abstract and dies in the specific."

**Why it works:** shows the discovery skill (concrete examples → decision → written) at a scale you can honestly claim. **Fill in a real story** — this must be true, and small is fine; the method is the point.

### 14.13 Explaining a technical trade-off to a non-technical stakeholder (mini-script)

> "There are two ways to build this. Option A is faster to go live but needs someone to check for mismatches every week — about [N] minutes of a person's time. Option B costs [X] more days and removes most of that checking, but it's more complex for us to support. If the audit is your deadline, I'd take A now and schedule B right after; if correctness-at-low-effort is the priority, B is the better buy. Which trade matters more to you?"

**Why it works:** two options, cost/benefit in *their* units, a recommendation, and the decision handed back. **Don't:** explain the mechanism, use the word "just," or present one option as obviously right when a trade exists.

### 14.14 Delivering bad news (script)

> "I want to give you an update while I still have options for you. The cutover date is at risk: the security review hasn't started, and it's a hard gate — if it starts after the 20th, we move one week. What I've done: submitted the full checklist today and asked to join the next review slot [date]. What I need from you: [specific ask — a nudge to the reviewer / a decision on scope]. Options: keep the date and reduce the pilot to [subset], or keep the pilot and move the date a week. I recommend the second because [reason]. I'll update you [day] either way."

**Why it works:** early, impact first, options, recommendation, owner, next deadline. **Never:** "we'll try," blame another team, or deliver it after the situation became a crisis.

### 14.15 Questions to ask the interviewer (choose 4–5; never "nothing")

1. "For the first 90 days of this role, what does a successful FDE here have that someone still ramping doesn't?"
2. "What does the handoff look like between the deployed team and the customer's steady-state team — who owns what after go-live?"
3. "What's the most common way an integration project gets 'technically done' but fails to land with the customer?"
4. "How does discovery work in practice — who runs it, and how are assumptions tracked across a long engagement?"
5. "Where does the team feel the sharpest engineering pain right now: reliability of the connectors, data quality from customers, or the delivery process itself?"
6. "How is success measured for an FDE against the customer's own numbers, versus internal delivery metrics?"
7. "What would make you, personally, confident that a candidate with a strong backend background but less customer exposure can ramp fast here?" *(bold, direct, and it invites your strongest territory — your plan)*

**Rules:** ask about the *work*, not perks, in the technical round; take notes; follow up on their answers ("what does that look like in a bad case?") — the follow-up is the signal.

### 14.16 Closing statement ("anything else?" — 30 seconds)

> "One thing I'd add. The parts of this role I'm best prepared for are the ones you were probing today — reliability design, debugging across boundaries, and being rigorous about assumptions. The part I'm early on is time in front of Fortune 500 stakeholders, and I'm not going to pretend otherwise; I'm going to be the person who writes everything down, reads it back, and gets the numbers agreed before we build. If I get the chance here, that's the way I'd start."

**Why it works:** it's the same honest-positioning frame, delivered with calm. It closes on your strongest professional identity: rigorous delivery.

---

## Part 15: Weak-area tracker and 14-day plan

### 15.1 How to use this part

The tracker below is the only "study plan" that matters: it is driven by *evidence* from your own practice attempts, not by reading time. Fill in a row every time you attempt a section drill, a mock round, or a lab run. A topic leaves the list only when it passes **twice, on different days, out loud**.

### 15.2 The tracker

| # | Topic | Source | Attempt 1 (C/P) | Date | Attempt 2 (C/P) | Date | Remaining gap | Fixed by |
|---|---|---|---|---|---|---|---|---|
| 1 | HTTP semantics, status codes | Part 2, MCQ T2/T9 | | | | | | |
| 2 | Auth / OAuth 2.0 | Part 2.4, MCQ T7 | | | | | | |
| 3 | Pagination (offset vs cursor) | Part 2.5, MCQ T10 | | | | | | |
| 4 | Schema validation & evolution | Part 2.6, MCQ T8 | | | | | | |
| 5 | SQL joins, NULLs, aggregation | Part 3.2–3.4, MCQ T5 | | | | | | |
| 6 | Window functions | Part 3.5 | | | | | | |
| 7 | Indexes & EXPLAIN | Part 3.6, MCQ T5 | | | | | | |
| 8 | Transactions & race conditions | Part 3.7, 6.8, MCQ T6 | | | | | | |
| 9 | The assessment SQL query (regional vendors) | Part 3.8 | | | | | | |
| 10 | Soaring Prices (write from scratch) | Part 4, MCQ T3 | | | | | | |
| 11 | Webhook receiver design | Part 5, MCQ T4 | | | | | | |
| 12 | Signature verification | Part 5.3 | | | | | | |
| 13 | Idempotency end-to-end | Part 6.2, MCQ T1 | | | | | | |
| 14 | Outbox & crash matrix | Part 6.4, 6.9 | | | | | | |
| 15 | Retries, DLQ, replay | Part 6.6–6.7, MCQ T2 | | | | | | |
| 16 | Exactly-once language | Part 5.6, MCQ T4 | | | | | | |
| 17 | Sync design (delta, ownership, conflicts) | Part 7 | | | | | | |
| 18 | Rate limits & pacing | Part 7.9, MCQ T2 | | | | | | |
| 19 | Reconciliation | Part 7.10 | | | | | | |
| 20 | Debugging drills (5-hop bisection) | Part 8.6 | | | | | | |
| 21 | Incident comms & bad news | Part 8.7, 14.14 | | | | | | |
| 22 | LLM extraction pipeline | Part 9, MCQ T8 | | | | | | |
| 23 | Evidence verification / hallucination | Part 9.5 | | | | | | |
| 24 | Discovery & assumptions register | Part 10.2–10.6, MCQ — | | | | | | |
| 25 | System design full structure | Part 11 Round 4 | | | | | | |
| 26 | Customer role play | Part 11 Round 7 | | | | | | |
| 27 | "Tell me about yourself" & scripts | Part 14 | | | | | | |
| 28 | MCQ bank overall | Part 12 | | | | | | |

### 15.3 Diagnostic routing

| Evidence | What it means | Do this (nothing else) |
|---|---|---|
| MCQ topic < 2/3 | Concept gap | Re-read the exact section listed in the tracker; write the missed distinction from memory, then re-quiz |
| Correct approach but missed edge cases (score 2) | Recognition ≠ recall under pressure | Redo the part's practice questions aloud, writing answers before checking expected points |
| Lab runs pass but you can't explain the output | Memorized artifact, not owned knowledge | Re-run the lab file and narrate every step; the file is a prop, your explanation is the product |
| Design answers lack failure modes | Happy-path thinking | Use the fixed structure aloud: requirements → assumptions → architecture → data → flow → **failures** → security → observability → testing → rollout → trade-offs. Never skip failures |
| Customer rounds feel hardest | Normal for a technical candidate | Redo Part 10 practice + Part 11 Round 7 role plays with an impatient friend; rehearse 14.5 and 14.12 until boredom |
| You freeze on "I don't know" questions | Fear, not ignorance | Practice the honest bridge: "I don't know yet — here's how I'd find out in [timeframe], and here's my initial hypothesis." Then move on. Hedge-free honesty scores better than invented answers |

### 15.4 The 14-day plan

Rules for the whole plan: every day has a **review block** (20–30 min, from memory, out loud) and a **drill block** (60–90 min, doing, not reading). One rest day. No new material after day 13.

| Day | Focus | Drills and proof | Done |
|---|---|---|---|
| **1** | HTTP core + MCQ baseline | Parts 2.2–2.6 skim; take the full MCQ bank cold (Part 12) and record the baseline; re-read only missed topics | ☐ |
| **2** | SQL I | Run `sql_exercises.sql` (Appendix A); re-derive the regional-vendors query from memory before comparing; write out the 1-vs-0 COUNT trap explanation aloud | ☐ |
| **3** | SQL II + Python | Index/window exercises (Parts 3.5–3.7); rewrite Soaring Prices from scratch in 25 minutes; compare to the lab and name every edge case you almost missed | ☐ |
| **4** | Webhooks | Run `webhook_receiver.py`; for each of the 11 scenarios say out loud: the failure, the mechanism, the evidence it worked (`stale_ignored`, DLQ replay requeues 0, …) | ☐ |
| **5** | Idempotency + outbox | Whiteboard the idempotency record design from memory (same transaction; 409 on key reuse); walk the crash matrix for a batch job; write the outbox pattern from memory | ☐ |
| **6** | Sync + reconciliation | Run `vendor_sync.py` twice; explain run-2's 0/0/0 no-op; defend delta-vs-mirror and field ownership; sketch the reconciliation report | ☐ |
| **7** | Light day | Cheat sheet (Part 13) once; 15 MCQ flashcards; one timed "tell me about yourself" (14.1). Stop by 60–75 minutes | ☐ |
| **8** | Debugging | All five drills in Part 8.6, timed 5 minutes each, spoken; then write one incident update from 8.7 and one bad-news delivery (14.14) | ☐ |
| **9** | LLM extraction | Run `llm_extraction.py`; narrate the repair loop and the fabricated-evidence catch; deliver 14.11 as a 90-second answer | ☐ |
| **10** | Discovery | Run two real discovery interviews (friend's shop, college office, NGO) about a spreadsheet process; write a one-page current-state map + assumptions register for each; log in the tracker | ☐ |
| **11** | Mock Rounds 1–3 | Score with Part 11.9; one-drill rule; re-test the single fixed point the same evening | ☐ |
| **12** | Mock Rounds 4–5 | Full 2-minute system design with a timer, then all six probes; debugging drills unscripted | ☐ |
| **13** | Mock Rounds 6–8 + scripts | Adversarial questions (A8.1–A8.4); record 14.1, 14.5, 14.16; listen back only for filler words and missing structure | ☐ |
| **14** | Taper | Cheat sheet + the three closing scripts; prepare your 4–5 interviewer questions; logistics check (link, room, backup internet); early night. **No new material** | ☐ |

**If you only have 7 days:** compress by pairing — (1) HTTP+MCQ baseline, (2) SQL run + query rebuild, (3) Python rebuild + webhooks run, (4) idempotency + sync runs, (5) debugging + LLM, (6) mock Rounds 1–5, (7) mock Rounds 6–8 + taper. Keep the out-loud rule; it is the plan.

### 15.5 Final 48 hours

- Re-run `webhook_receiver.py` and `vendor_sync.py` once each — not to learn, to keep the vocabulary warm.
- Re-read Part 13 (cheat sheet) and the failure-signatures table (8.5) — nothing else.
- Rehearse exactly three scripts: 14.1 (intro), 14.5 (the honest gap answer), 14.16 (closing).
- Prepare your 4–5 interviewer questions and your logistics checklist. Sleep ≥ 7 hours. No new topics; no last-minute cramming of things that are not in this workbook.

### 15.6 Day-of checklist

- [ ] Read the question fully; restate it before answering. Ten seconds of silence to think is allowed and reads as senior, not slow.
- [ ] Clarify before coding; state assumptions out loud *as assumptions*.
- [ ] Narrate while you work: plan → code → dry-run → complexity → edge cases.
- [ ] In design questions, use the fixed structure and **never skip failure modes, security, observability, rollout, trade-offs**.
- [ ] In customer scenarios, get concrete ("show me the last one"), write down, read back.
- [ ] When you don't know: say what you'd check, in what order, and how you'd know you were right. Never invent.
- [ ] Ask for feedback-friendly endings: "Anything you'd probe further?" — it shows security and invites the close.
- [ ] Thank them, ask about next steps and timeline.

### 15.7 Post-interview debrief (same day)

```
Date / round:
Questions asked (list them all):
Answers that landed (the interviewer's follow-up showed interest):
Answers that missed (what specifically):
New patterns to add to the tracker:
One thing to say better next time (write the exact new sentence):
```

Most interviews here will ask variations of the same six assessment patterns plus discovery and debugging. Every debrief should shrink the list of things that can surprise you.

---

## Appendix A: Companion lab files

The workbook ships with a runnable lab in `Companies/Certa/certa_lab/`. Every number and behavior quoted in Parts 3–9 was produced by running these files; they are the empirical backbone of this workbook. All Python is **standard library only** (no network required — the files spin up their own local mock servers), and the SQL is MySQL 8.0-compatible. The files were verified on Python 3 (macOS) and MySQL 9.7.1.

| File | What it proves | Run command (from `certa_lab/`) |
|---|---|---|
| `soaring_prices.py` | The assessment coding task solved the production way: clarifying-question handling, tie behavior, absent values, complexity narration; includes a self-test and an end-to-end run against a mock HTTP API | `python3 soaring_prices.py --selftest` and `python3 soaring_prices.py --mock` |
| `llm_extraction.py` | The Part 9 pipeline: constrained output, validator-driven repair loop (repaired on attempt 2 in the verified run), evidence verification that caught a fabricated snippet, confidence routing, and an eval harness | `python3 llm_extraction.py` (add `--json` for machine-readable output) |
| `webhook_receiver.py` | All eight webhook properties: HMAC verification on the raw body, inbox dedupe by event ID, per-entity version guard (stale v6 after v7 → `stale_ignored`), backoff + DLQ + replay (second replay requeues 0), 429 backpressure, and crash recovery — 11 scenarios, all passing | `python3 webhook_receiver.py` (add `--json`; `--db path` to relocate the SQLite file it creates) |
| `vendor_sync.py` | The Part 7 sync end-to-end between two mock systems: cursor pagination, token-bucket pacing, idempotency-key guard (same key + different body → 409 `idempotency_key_reuse`), 409 conflict quarantine, and reconciliation — the second run is a verified 0 pulled / 0 pushed / 0 drift no-op | `python3 vendor_sync.py` (add `--json`) |
| `sql_exercises.sql` | 21 exercises on a real MySQL database, including the assessment's regional-vendors query and the `LEFT JOIN` `COUNT(*)` vs `COUNT(v.col)` trap (MEA 1-vs-0) | `mysql -u root --table < sql_exercises.sql` — add `--socket=/path/to/mysql.sock` (or `-h 127.0.0.1 -P 3306`) if your client defaults differ |
| `sql_out.txt` | Captured verified output of the full SQL suite, quoted in Part 3 (assessment result: Europe 3/5, Americas 3/4, APAC 2/2, NULL 1/1) | Read alongside `sql_exercises.sql`; do not edit |

**How to use the lab in practice:**

1. **Before reading a part:** attempt the drill cold (SQL query, Python function, whiteboard design).
2. **After reading the part:** run the corresponding file and predict each line of output *before* it prints. Prediction is the exercise; the console is the answer key.
3. **The day before the interview:** run the two heaviest files (`webhook_receiver.py`, `vendor_sync.py`) once each to keep the vocabulary warm.

> Note: the lab deliberately contains a few "learnable bugs" converted into teaching moments — the missing-required-field hallucination case, the idempotency-key reuse rejection, and the delta-vs-mirror planning difference. If you find yourself debugging one of them during practice, that is the file working as intended.

## Appendix B: Assumptions, disclaimers, and how to phrase uncertainty

### B.1 The disclaimer, restated

This workbook contains **no claims about Certa's internal architecture, APIs, data model, hiring process, or undisclosed assessment content**. Where Certa appears by name, it is from the public job description: the role's customer-facing mission, the no-code **Studio** and **Vibe Configuration℠** configuration story, and the third-party lifecycle (onboarding → continuous monitoring → offboarding). Everything else is generic enterprise-integration engineering and delivery practice, assembled so you can demonstrate transferable competence — which is what the role's assessment actually tests.

### B.2 What each claim is sourced from

| Source | How to treat it | Examples in this workbook |
|---|---|---|
| **The job description as quoted** | High confidence about *scope*, not about internals | "discovery with Fortune 500 teams", "email, spreadsheets, people's heads", lifecycle phases, delivery vocabulary (RAID, SIT/UAT, cutover, hypercare) |
| **The six assessment patterns you supplied** | High confidence about *question shapes* | SQL (regional vendors), Python API coding (Soaring Prices), webhooks, idempotency/transactions, sync/rate-limits/batching, LLM extraction, OAuth/schema/cursor/201/429 |
| **Widely accepted engineering practice [BEST PRACTICE]** | State it as your own reasoning — that is expected | Backoff + jitter, outbox pattern, inbox dedupe, field-ownership tables, reconciliation, shadow-mode rollout |
| **Workbook assumptions [ASSUMPTION]** | Say them out loud as assumptions when you use the examples | Specific volumes (50k vendors, 2%/day churn), specific vendors/regions, exact retry numbers |

### B.3 How to phrase uncertainty (say these almost verbatim)

| Situation | Say this |
|---|---|
| You don't know a spec detail | "I don't know their exact behavior there — my assumption is X; I'd verify it against their docs and a sandbox call before I committed to the design." |
| You don't know an internal system's design | "I haven't seen how that service is built, so I'd design to the contract I can observe — status codes, timing, failure behavior — and confirm with the owner." |
| You're about to be wrong | "I might be off on this; here's my reasoning, and here's the cheapest test that would tell us if I'm wrong." |
| Trade-offs are real | "There's no version of this that's free: A costs money, B costs latency, C costs complexity. I'd choose B here because [customer constraint]." |
| The interviewer pushes hard | "Fair push — let me check that against the failure case. If [X] happens, my design does [Y]. Is that the direction you're probing?" |
| Asked for a number you don't have | "Order of magnitude: tens of thousands of rows, low single-digit thousands of changes a day. If the real number is 10x that, the design lever that moves first is batching, not architecture." |

### B.4 Words to avoid, and what to say instead

| Avoid | Why | Say instead |
|---|---|---|
| "Just" / "simply" / "obviously" | Implies the listener is behind; usually precedes hand-waving | "The cleanest approach here is …" |
| "It depends" (alone) | Vague; reads as evasion | "It depends on X — if X, then A; otherwise B." |
| "Exactly-once" (unqualified) | Technically sloppy; interviewers pounce | "At-least-once delivery with idempotent, effectively-once processing." |
| "I'll figure it out" | Promises without method | "I'd bisect it: check the midpoint hop, then …" |
| "Best practice" (overused as proof) | Authority-free | Name the mechanism and the reason: "backoff with jitter, because synchronized retries amplify an outage." |
| "We can do that easily" (to a customer) | Spends trust you may need | "That sounds achievable — let me confirm the details and come back today with the impact." |

### B.5 Final integrity checklist (for the candidate, not the workbook)

- [ ] Every number I plan to say about *my own* projects is true and checkable.
- [ ] Every claim about *Certa's product* is something I could point to in public material.
- [ ] Every design answer I give includes at least one failure mode and one trade-off.
- [ ] Every customer scenario answer includes a question I'd ask *before* proposing.
- [ ] Every "I don't know" ends with a way to find out and a timeframe.
- [ ] I can explain every line of the lab files I plan to reference.

---

*End of workbook. Version: October 2026. Lab verified on MySQL 9.7.1 and Python 3 standard library; all mock runs offline and reproducible via Appendix A.*
