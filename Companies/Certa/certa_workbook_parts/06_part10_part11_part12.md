<!-- Certa FDE mastery workbook — split part 6 of 7 (source lines 3267-4054). Concatenating all 7 files in order reproduces certa_fde_interview_mastery.md. -->

## Part 10: Customer discovery, assumptions, prioritization, delivery, stakeholders

> **Where this comes from:** the JD says you will "run discovery and design sessions with Fortune 500 teams" and "learn how their third-party processes actually run today, including the parts that live in email, spreadsheets, and people's heads." This part teaches that skill generically. **[BEST PRACTICE]** everywhere except where a JD phrase is quoted.

### 10.1 First principles: discovery is risk reduction, not requirement collection

A discovery session exists to retire four risks that determine whether an integration project succeeds or quietly dies:

| Risk | The question it answers | Symptom when missed |
|---|---|---|
| **Scope risk** — what exactly are we building? | "Which vendors, which systems, which direction, which fields?" | Every "tiny change" becomes a project; nobody agrees on done |
| **Data risk** — what actually exists? | "Where is the canonical list? What quality? Who owns the IDs?" | Mapping breaks on nulls, duplicates, and renames in week 3 |
| **Adoption risk** — will people use it? | "What does the team's day look like the Monday after go-live?" | A technically perfect pipeline with a human workaround running in parallel |
| **Political risk** — who wins and loses? | "Whose headcount, budget, or control changes?" | A stakeholder with veto power quietly stops responding |

**The mindset shift:** you are not taking an order. You are building a working *model* of how their process runs today — the real one, not the documented one — and then agreeing on what "done" means in numbers. The documented process and the real process are almost never the same. Your job in the session is to find the delta.

### 10.2 The 90-minute discovery session (an agenda you can run) [BEST PRACTICE]

| Min | Segment | What you are doing | Example prompt |
|---|---|---|---|
| 0–5 | **Land the outcome** | State what you'll leave with: a written process model, an assumption list, owners, and next steps | "By the end I want to have your current process on one page, the edge cases, and what success looks like — correct?" |
| 5–35 | **Current-state walkthrough** | Have them *do the task on their screen*, narrating | "Show me the last vendor you onboarded, end to end. Start from the email that triggered it." |
| 35–55 | **Exception paths** | Hunt the edge cases; they never volunteer these | "Tell me about the last time this went wrong. What did you do?" |
| 55–70 | **Data & systems inventory** | Sources of truth, identifiers, volume, frequency, formats | "Which system is the source of truth for legal name? For status? How many records a month?" |
| 70–80 | **Constraints** | Security review, compliance evidence, legal, timing windows | "Who has to approve a new connection? What evidence does audit need?" |
| 80–90 | **Read-back and close** | Recap aloud; state assumptions; assign verification owners; agree next step | "My understanding is X. I'm writing down three assumptions to verify by Friday." |

**Three rules that do the heavy lifting:**

1. **Get them into the tool, not the narrative.** People describe the process they *designed*; the screen shows the process they *run*. "Can you share your screen and show me the last one?" is the single highest-yield question in discovery.
2. **Follow the artifact.** Every time an email, spreadsheet, screenshot, or chat message is mentioned, ask to see it. Artifacts contain the real rules (thresholds, naming conventions, exceptions) that memory omits.
3. **Close with a read-back and written assumptions.** Unspoken assumptions are the most expensive artifact in integration work; say them out loud and send them in writing the same day.

### 10.3 Question bank (ordered by how they open people up)

**Openers (rapport + context)**

- "Walk me through your role and where third-party risk sits in your week."
- "What triggered this project now — what happened, or what deadline is coming?"
- "What have you tried before this, and what disappointed you about it?"

**Current process (get concrete, get screens)**

- "Show me the last three vendors you onboarded — what actually happened, step by step?"
- "Who touches this before it's 'done'? What does each person do to it?"
- "What are the queue and the SLA you're measured on?"

**Edges and exceptions**

- "Tell me about the last time this went sideways. What did you do?"
- "What do you do when a vendor never responds to the questionnaire?"
- "What happens to a vendor we stop using — how does offboarding work?"
- "Is there a case where you deliberately do NOT load a vendor? What's the rule?"

**Data and systems**

- "Where is the canonical list of vendors — if two people answered this question, would they point at the same place?"
- "What identifier is stable forever — legal name changes, does your ID change?"
- "How many records, how often, and how much do they change per week?"
- "What fields does downstream reporting actually consume? Who would notice if two were wrong?"

**Constraints and compliance**

- "What does audit ask you to produce, and where does that evidence come from today?"
- "Who must approve a new data flow, and what does that review need to see?"
- "Are there records that must never leave your environment?" *(This one often reveals constraints nobody else mentioned.)*

**Definition of done**

- "How will you know, three months from now, that this worked?"
- "What would make you turn it off?"
- "If it silently broke for a day, who would notice first, and how?"

**Decision rights**

- "Who decides if the requirements change? Who has to sign off on go-live?"
- "If two teams disagree about a field's meaning, who breaks the tie?"

### 10.4 Surfacing the parts that live in email, spreadsheets, and heads [BEST PRACTICE]

The JD phrase is also the field guide. Each hidden-process archetype has a tell, a risk, and a corresponding engineering requirement:

| Archetype | Tell | Why it matters | What it becomes in the design |
|---|---|---|---|
| **Email approval chains** | "I keep the email as proof" | Audit trail lives in a mailbox; new automation may destroy it | The integration must produce equivalent approval evidence, or the reviewer must keep forwarding |
| **Personal spreadsheet as source of truth** | Two people give different answers about "the list" | Data risk: your sync mirrors one person's copy | Pick an owner for the canonical set; reconcile against it |
| **Dual maintenance** | "We keep both systems in sync by hand" | Drift is guaranteed; both systems are "right" to their users | Reconciliation report is a Phase-1 deliverable, not a nice-to-have |
| **Key-person dependency** | "Only Dana knows how that works" | The process stops scaling and stops when Dana is on leave | Document the rules; review with the team, not just Dana |
| **Screenshot-based evidence** | Compliance mails screenshots around | New process must not lose the evidence path | Emit the artifact (a report, a signed record) the auditor actually needs |
| **Shadow naming conventions** | "We always prefix 'DO NOT USE' —" | Matching will fail on a rule that exists nowhere in writing | Capture the convention as a normalization rule with test cases |

**The two questions that surface the most:** "Show me the last one you did" (finds screenshots and hands) and "What do you do when the tool is down?" (finds the manual fallback, which is your cutover plan's escape hatch).

### 10.5 Stakeholders, decision rights, and conflict

| Stakeholder | Cares about | You need from them | Failure mode if ignored |
|---|---|---|---|
| Executive sponsor | Timeline, risk reduction, budget story | Air cover, tie-breaking, priority calls | Project loses funding or attention silently |
| Process owner | The day-to-day work getting easier | The real acceptance criteria | Sign-off withheld at the end |
| Administrator (day-to-day user) | Not having more work | Workflow reality, edge cases | Workarounds appear post-launch; adoption fails |
| IT / security gatekeeper | Data flows, credentials, residency | The security review, early and often | A "surprise blocker" three days before go-live |
| Counterpart-system owner (e.g., the GRC vendor's admin) | Their product working as sold | Configuration truth, sandbox access | You integrate against documentation that is wrong |

**The "says vs pays" test:** the person who asks for something is not always the person who owned the pain or owns the outcome. Ask "who is affected if this is late?" to find the real owner.

**When stakeholders conflict (they will), run this protocol [BEST PRACTICE]:**

1. **Write both versions down, verbatim.** Conflict often dissolves when two people see their statements side by side.
2. **Convert preferences to requirements** with an objective test: "What downstream decision changes if the status field arrives 10 minutes later?"
3. **Find the decision owner** before the argument deepens: "If we can't agree, who decides?" That question alone resolves most conflicts.
4. **Time-box to the impact.** If both options are reversible and low-impact, pick either and note it; spend your negotiating capital on irreversible or high-impact choices (identifiers, definitions of status, deletion behavior).
5. **Confirm in writing, to everyone, the same day.** Decisions that only exist in the call did not happen.

### 10.6 The assumptions register (the artifact that saves the project)

Every integration runs on assumptions. Successful ones *write them down*:

| # | Assumption | Evidence | Impact if wrong | Owner | Verify by | Status |
|---|---|---|---|---|---|---|
| A1 | Vendor `external_id` is immutable | Process owner, spot-checked 20 records | HIGH — matching and reconciliation both break | Us | Fri | Open |
| A2 | ~200 vendors onboarded/month; peak 40/day | Admin's spreadsheet | MEDIUM — rate-limit pacing and batch size | Customer | Mon | Open |
| A3 | Status vocabulary has exactly 4 values | API docs + sample export | HIGH — mapping rules and UI filters | Customer | Fri | Open |
| A4 | No data may leave the EU region | Security review | HIGH — re-architecture of processing | IT | Thu | Open |

**The habit that gets you hired:** end every call with "here's what I heard, here's what I'm assuming until you tell me otherwise." Then send it. Include a "please correct me" ask with a deadline — a wrong assumption stated in writing gets corrected; a wrong assumption held silently becomes an incident.

**Phrasing uncertainty in front of a customer:**

- Instead of "it will definitely work": *"I'm confident about the mechanism; the thing I want to verify is X. If X holds, this works."*
- Instead of hiding a gap: *"I don't know their API's rate limit yet — I'm treating it as unknown and designing a pacing layer so it's safe either way."*

### 10.7 Definition of done, written with the customer

Vague goals guarantee an unhappy ending. Get these six numbers/sentences agreed in a document *before* build:

1. **Objects and direction:** which records flow, from where to where, one-way or two-way.
2. **Volume and frequency:** records/month, peak/day, freshness required (e.g., "a change should be reflected within 15 minutes").
3. **Quality bar:** measurable, e.g., "≥99.5% of records match at weekly reconciliation; every mismatch has a disposition within 2 business days."
4. **Field ownership:** for each shared field, which system wins. (This is Part 7's chapter, agreed with humans rather than tables.)
5. **Evidence and security:** what audit needs, what review must approve, where data may flow.
6. **Cutover criteria and sign-off:** what must be true to turn it on, who signs, what the fallback is.

> **Interview line:** "My rule is that 'done' is a number and a signature, not a feeling. If I can't measure it, I can't tell the customer whether we're succeeding — and neither can they."

### 10.8 Prioritization: what gets built first

**MoSCoW is often misunderstood. Anchor each bucket to a test:**

| Bucket | Test |
|---|---|
| **Must** | Without it, the end-to-end loop cannot run at all, or a compliance requirement fails |
| **Should** | Real pain, workaround exists, deferral is survivable for one phase |
| **Could** | Nice-to-have; customer would enjoy it but nothing breaks without it |
| **Won't (this phase)** | Written down explicitly, with a reason — this list prevents the "you forgot my thing" conversation |

**Value/effort, with the trap called out:**

|  | Low effort | High effort |
|---|---|---|
| **High value** | Do these now | Do the *load-bearing one* after Phase 1 proves the loop — not before |
| **Low value** | Do if trivially reversible | **Kill these.** They are how integrations run out of time |

**The Phase-1 principle [BEST PRACTICE]:** prove the thinnest end-to-end loop with *real data*, one direction, one system pair, a real subset of records (e.g., a pilot vendor list), with a human fallback agreed. A working narrow loop beats an impressive architecture that has never touched production data. When you say this in an interview, you are demonstrating delivery judgment — the core FDE skill.

### 10.9 Go-live with a customer: the rollout that does not rely on hope

| Stage | What runs | Exit criteria |
|---|---|---|
| 1. **Dry run** | Full pipeline against sandbox or a copy | Zero crashes; metrics visible; mappings reviewed |
| 2. **Shadow mode** | Read-only: the new process *predicts* what should happen; a human still does the real work | Predictions vs. manual reality agree ≥ the agreed bar (e.g., 99% over a week) |
| 3. **Pilot subset** | Go live for a small, real, low-risk subset | Both systems reconciled daily; no unresolved mismatches; users trained |
| 4. **Go / no-go** | Formal read-out against the definition-of-done numbers | Sponsor + process owner sign; fallback plan confirmed |
| 5. **Cutover** | Full population | Watermark lag and error budget within limits for X days |
| 6. **Hypercare** | Daily check-ins, fast lane for issues | Two clean reconciliation cycles; issues trending to zero |

**Rollback is a decision, not a wish:** name, in advance, who can call a rollback and what the manual fallback is. "We'll figure it out if it breaks" means a 2 a.m. improvisation in front of a customer.

### 10.10 Communication cadence

**The weekly written update (send it even when nothing changed):**

```
DONE:     <shipped/verified this week — with numbers where possible>
NEXT:     <planned next week>
RISKS:    <what could hurt us, with severity and mitigation>
NEEDED:   <decisions or access required, by whom, by when>
```

**The 3-line escalation (use this in front of executives):** *State the situation and the impact in business terms → state what you need → state the options you see.* Never escalate a complaint; escalate a decision.

**Bad news protocol [BEST PRACTICE]:**

1. Deliver early — the moment you are confident something is wrong, not after you fix it.
2. Lead with impact in *their* language ("vendors offboarded in Certa may still appear active in your GRC").
3. Bring options, not a mood: "Option A: pause the cutover. Option B: go with a manual check for two days. I recommend B because…"
4. Follow up in writing with the decision and the next check-in.

### 10.11 Scope creep and the phrase that protects the schedule

Separate three things every time a request arrives:

| Type | Example | Response |
|---|---|---|
| **Clarification** | "Status should include 'on hold'" — it exists in the data | Do it; note it; no negotiation needed |
| **Change request** | "Can we also sync contracts?" — new object | "Yes — here's the impact on the current scope. I'll add it to the plan for phase 2, or we trade something out." |
| **New requirement with a deadline** | "This must ship before the audit" | Re-prioritize with the decision owner; put it against the definition of done; document the trade |

**The magic phrase:** *"Yes — and here's what I'd change to keep the date."* It signals willingness (never say "no" flat) while making the cost visible. People rarely mind trade-offs; they mind surprises.

**Parking lot discipline:** keep a visible list of deferred items with reasons. Deferred is not forgotten; it's *decided*.

### 10.12 Common mistakes in customer-facing work, and why they fail

| Mistake | Why it fails |
|---|---|
| Asking "what do you want the system to do?" | You get wishlists, not the real process. Ask what they *did* last time |
| Accepting the first answer about a process | The first answer is the official process; the third question finds the real one |
| Designing for the most sophisticated user | Adoption is decided by the least-sophisticated affected user |
| Saying "that's easy" about anything customer-facing | If it turns out not to be, you have spent trust you will need later |
| Escalating complaints instead of decisions | Sponsors disengage from noise; they engage with options |
| Leaving with only verbal agreements | Nothing said in a call is real until it's written and confirmed |
| Treating security review as a late step | It is a gate, not a formality; involve it in week 1 |
| One champion only | Champions change jobs. Build at least two informed allies |

### 10.13 Model answers (rehearse these)

**M10.1 — Open a discovery call (30 seconds)**

> "Thanks for the time. Here's how I'd like to use it: I want to understand how this process really runs today — including the parts that live in email and spreadsheets — then agree on what success looks like in numbers, and leave with three or four things I'll verify in writing. I'll interrupt to ask 'what happens if…' — that's me pressure-testing my understanding, not doubting yours. Sound right?"

**M10.2 — Two stakeholders disagree on a field's meaning**

> "First I'd get both definitions written down side by side, because half of these conflicts dissolve when you see them in ink. Then I'd convert both into requirements with an objective test — what downstream decision changes based on this field? — and find the decision owner: 'if we can't agree, who decides?' If it's genuinely ambiguous, I'd propose the definition that the compliance use case needs, because that one has the hardest external constraint, and I'd confirm it in writing to both. What I'd avoid is averaging the two definitions or silently picking one — either guarantees a future incident where two reports disagree and everyone blames the integration."

**M10.3 — The customer asks for an impossible deadline**

> "I'd rather give you a smaller thing that works than a bigger thing that breaks. Here's what I can commit to on your date: the core loop — these vendors, this direction, these fields — verified against your own records. Here's what I'd move to a fast-follow, with dates. If the date is fixed by an audit, tell me which evidence the audit actually needs, because designing to the evidence is often narrower than designing to the wishlist. And I'd rather you hear the trade-off now, while we can still choose, than in three weeks when we can't."

**M10.4 — "While you're in there, can you also…"**

> "Yes — and here's what I'd change to keep the date. That's a new object with its own mapping and its own edge cases; here's my estimate. We can add it to this phase and move X out, or park it for phase two with a date. You own the priority — I'll make the cost visible so it's a real choice."

### 10.14 Practice

**Practice 10.1** Write the four discovery questions you would ask first for this scenario: "We need our vendors synced between our procurement system and our GRC tool." Write what each question is *for*.

**Your answer:**
>
>
>
>
>
>

**Expected points:** one current-process question ("show me the last vendor you onboarded — where did it start?"), one source-of-truth question ("where is the canonical list, and who edits it last?"), one data/volume question ("how many, how often, what changes?"), one done-definition question ("how will you know it's working three months out?"). Bonus: ask for the artifacts (export, screenshot, email) and ask who signs off.

**Practice 10.2** A process owner tells you: "We keep the list in the GRC, but ops also maintains a spreadsheet, and we reconcile them every Friday by hand." Write the three follow-up questions that expose the real risk, and name the engineering deliverable this implies.

**Your answer:**
>
>
>
>
>
>

**Expected points:** Who decides when the two disagree? (owner for conflicts) What causes the divergences — what does the spreadsheet have that the tool doesn't? (field ownership, missing data path) What happens to the hand-reconciliation after we automate — who retires it and when? (adoption risk, avoiding silent dual maintenance). Deliverable: a reconciliation report with an owner and a cadence — this is Part 7's work, requested by the customer instead of invented by you.

**Practice 10.3** Mid-project the executive sponsor asks for status in one week's meeting. Write the four-line update format with a realistic "risk" line about a security review that hasn't started.

**Your answer:**
>
>
>
>
>
>

**Expected points:** DONE with numbers; NEXT with dates; RISK/NEEDED naming the security review as a *decision/action with an owner and a deadline*, stated as impact on the go-live date ("if the review starts after the 20th, the cutover moves one week"). No burying it in prose; no drama; offer the mitigation (submit the checklist now, join the queue).

### 10.15 Part 10 scorecard

| Item | Correctness /4 | Production readiness /4 | Notes |
|---|---|---|---|
| Four risks named and applied to a scenario | | | |
| Session agenda with concrete prompts | | | |
| "Show me the last one" + artifact follow-up habit | | | |
| Hidden-process archetypes spotted in a story | | | |
| Stakeholder map incl. security gatekeeper | | | |
| Conflict protocol (write down → objective test → owner → document) | | | |
| Assumptions register with impact/owner/verify-by | | | |
| Definition of done stated as numbers + sign-off | | | |
| Phase-1 thin loop vs. big-bang judgment | | | |
| Rollout stages incl. shadow mode and rollback owner | | | |
| Status update + escalation formats | | | |
| Scope-creep phrase and trade-off handling | | | |

**Readiness bar:** 3+ on both columns. Customer work forgives imperfect engineering drafts; it does not forgive surprises, silent assumptions, or skipping the security gate.

---

## Part 11: Mock interview with progressive difficulty

### 11.1 How to run this mock (read this first)

- **Format:** 8 rounds, ordered by difficulty. Run one round per sitting, or the whole thing in ~3 hours the weekend before the interview.
- **Discipline:** answer out loud, timed. Recording yourself on your phone is the fastest way to hear filler words, hedging, and missing structure. 45–90 seconds per technical answer; 2 minutes for design.
- **Scoring:** use the 0–4 rubric from the front of this workbook (correctness and production readiness). Write the score **before** you read the expected points — self-flattery is the enemy.
- **Round protocol:** read the question → answer out loud → check expected points → note which specific point you missed → move on. Do not re-read the whole part for one missed point; log it in Part 15.
- **Difficulty legend:** R1 warm-up → R8 adversarial. Target scores rise with the round: R1–R2: 3s, R3–R6: 3–4s, R7–R8: 3+ with no unforced errors.

---

### Round 1 — Screen / warm-up (difficulty 1–2)

| # | Question | Interviewer is testing | Expected answer points (condensed) |
|---|---|---|---|
| 1.1 | "Tell me about yourself in 90 seconds." | Structure, relevance, communication | Present → proof → why this role (Part 1 skeleton). No chronology dump. Names backend + integration-adjacent work + interest in customer-facing delivery |
| 1.2 | "Why Forward Deployed Engineering, specifically?" | Motivation authenticity | You like the seam between code and business reality; you prefer owning outcomes end-to-end; you want feedback loops measured in hours not quarters; honest note on zero formal FDE experience, positioned as readiness (Part 1.7) |
| 1.3 | "What do you think an FDE does day to day?" | Role realism | Discovery and design with customers; build integrations/automations; debug across systems you don't own; communicate status to non-engineers; own delivery to a date. Bonus: names the ambiguity — "it's a role where engineering judgment and customer judgment must coexist" |
| 1.4 | "POST vs PUT vs PATCH — when do you use each?" | HTTP semantics [ASSESSMENT] | POST = create in a collection, not idempotent; PUT = full replace, idempotent; PATCH = partial modification, not necessarily idempotent; note that semantics serve retry safety, not textbook purity |
| 1.5 | "What's the difference between 401 and 403?" | Status code precision [ASSESSMENT] | 401 = not authenticated (who are you) — retry after fixing credentials/token; 403 = authenticated but not authorized (you can't do this) — retrying won't help, fix the grant; operational consequence: 401 can be transient after token refresh, 403 is a config/perms change |

**R1 follow-up probes:** "Which of those matters for retry logic, and why?" (answers: 5xx + timeouts + some 429 = retryable; 4xx = fix payload/perms; 401-after-refresh is a special case — treat as failure, alert).

---

### Round 2 — Core technical (difficulty 2–3)

| # | Question | Interviewer is testing | Expected answer points (condensed) |
|---|---|---|---|
| 2.1 | "Design the REST endpoints for a vendors resource." | API design instincts (Part 2) | Nouns not verbs; `GET /vendors` (list, paginated), `POST /vendors` (create, 201 + Location), `GET /vendors/{id}` (single, 404 if missing), `PATCH /vendors/{id}`, `GET /vendors/{id}/events` maybe; consistent error envelope; versioning stance; filtering/sorting params; idempotency key on POST |
| 2.2 | "Offset vs cursor pagination — pick one and defend it for a 5M-row vendor table." | [ASSESSMENT: cursor pagination] | Offset breaks under concurrent writes (skips/duplicates) and degrades O(offset) on deep pages; cursor (keyset on a stable, unique, ordered key like `(updated_at, id)`) is stable and index-friendly; trade-offs: no random page access, cursor opacity/OOM concerns; state that most enterprise import use cases are sequential, so cursor wins |
| 2.3 | "A JSON field was renamed upstream — walk me through schema evolution." | [ASSESSMENT: schema evolution] | Contract detection: validate against a schema, fail loudly at the boundary; additive = tolerant reads, unknown fields ignored not crashed; rename/removal = version or adapter layer; never silently default a renamed required field (that's the hallucination trap of integrations); regression tests with fixtures from old and new versions; deprecation window + comms |
| 2.4 | "Explain how you'd write this query: for each region, how many vendors have at least one active contract, and how many don't." | SQL aggregation + NULL traps [ASSESSMENT] | Row per region via LEFT JOIN from a regions dimension; `COUNT(DISTINCT CASE WHEN c.status='active' THEN v.id END)`; beware `COUNT(*)` inside LEFT JOIN counting the NULL row (the MEA 1-vs-0 trap from Part 3); explain the `GROUP BY` granularity and index usage; run `EXPLAIN` |
| 2.5 | **Coding, live:** Soaring Prices (Part 4) — "given daily price lists per product, return max price per product with ties" | [ASSESSMENT: Python API coding] | Clarify: multiple max ties → all of them; positive ints; absent price = 0 or absent day? Read function contract carefully; dict of product → (best, days); O(n) traversal; handle empty input; then write it live in ≤10 minutes; narrate edge cases |
| 2.6 | "When would you NOT use an ORM?" | Judgment | Bulk ops where per-row overhead and N+1 patterns dominate; complex reporting queries where explicit SQL is clearer and tunable; tight loops in data syncs; note the trade-off is losing type-safety/portability, so use raw SQL surgically with tests |

**R2 note on scoring:** a 4 on 2.5 requires *asking the clarifying question* about ties and running the edge cases out loud. Correct-code-but-silent scores 2.

---

### Round 3 — Reliability deep dive (difficulty 3)

| # | Question | Interviewer is testing | Expected answer points (condensed) |
|---|---|---|---|
| 3.1 | "Design an inbound webhook receiver." | [ASSESSMENT: webhooks; durability] | Verify signature on the raw body (constant-time compare); fast 2xx, process async (queue) — but never acknowledge before durable persistence (inbox table with PK dedupe); per-entity version guard for ordering; retries from sender = at-least-once, so consumers must be idempotent; DLQ + replay tooling; 429 backpressure instead of silent drop; watermark lag metric |
| 3.2 | "What does idempotency mean, concretely, for a POST endpoint?" | [ASSESSMENT: idempotency] | Client supplies Idempotency-Key; server stores key → (status, response body hash) in the same transaction as the side effect; replays return the stored response; same key + different body = 409 `idempotency_key_reuse` (the strict variant — my verified lab behaviour); key scoped per tenant + endpoint; retention window; GET/PUT naturally idempotent, POST is not |
| 3.3 | "A job crashed halfway through pushing 500 records. Walk me through recovery." | Crash matrix thinking | Checkpointing: per-record status rows (pending/sent/confirmed) so restart resumes; for the in-flight batch, uncertainty window — re-query the target by idempotency key or natural key to learn truth; never assume sent≠applied; duplicate-safe writes on target (upsert/put); reconciliation pass after crash; log correlation id per record |
| 3.4 | "Your retry policy just caused a retry storm against a struggling provider. What happened and what do you change?" | [ASSESSMENT: retries] | Missing exponential backoff + jitter + max attempts + circuit breaking; classify errors (only retry transient); retries should respect Retry-After; the deeper fix — bulkhead/queueing with adaptive rate, and alerting on retry rate as a signal; postmortem: cap total in-flight attempts, add load shedding |
| 3.5 | "Exactly-once delivery — possible?" | [ASSESSMENT: exactly-once] | Be precise: delivery is at-least-once (or at-most-once); exactly-once *processing* is achieved via idempotent consumers + effectively-once effects (dedupe keys, transactional outbox, upserts). Anyone who says "the queue gives exactly-once, so my consumer is safe" has said the wrong thing. Mention Kafka transactions/streams give exactly-once *within their system boundary*; crossing systems still needs idempotency |

---

### Round 4 — System design (difficulty 3–4): the full walk

**Q4.1 "Design a nightly bidirectional sync between a vendor-management system (ours) and an external GRC platform. Fields: name, status, region, owner, last-reviewed date. 50k vendors. Both sides can be edited." (2 minutes + 15 minutes of probing)**

Use the fixed structure from Parts 5/7. The scoring table:

| Structure element | What a 3 sounds like | What a 4 adds |
|---|---|---|
| **Requirements** | "Nightly batch is fine; near-real-time optional; both directions" | Quantifies freshness SLO, volume math (50k rows, ~2% touch/day ⇒ ≈1k delta rows), explicitly asks which fields are user-editable where |
| **Assumptions** | States assumptions about IDs, volumes | Writes them down, ties each to the design choice it protects, names how to verify |
| **Architecture** | Scheduler → source read → transform → push; inbound webhook/poll → queue → apply | Same, plus outbox for our-side changes, inbox for theirs, DLQ, reconciliation job, watermark store |
| **Data model** | Sync state per record, mappings table | Version/updated_at columns, idempotency keys, audit log, conflict log with disposition, field-ownership table |
| **Normal flow** | Pull both sides → reconcile → push diffs | Delta sync (not mirror), watermark + cursor pagination, batching within rate limits, per-record status |
| **Failures** | Retry with backoff; DLQ | Crash resume, duplicate-safe replay, provider 429 pacing, partial-batch failure handling, stale/out-of-order record policy, poison-record quarantine with reprocess tool |
| **Security** | Tokens in a secrets manager | Least-privilege scopes, signature verification, PII/region constraints, audit of every change, rotation plan |
| **Observability** | Logs and error alerts | Watermark lag, records-synced-by-direction, mismatch rate, retry rate, DLQ depth and age; alert on progress, not just errors |
| **Testing** | Unit tests for mapping | Contract tests with captured payloads, crash/duplicate/429 chaos drills, reconciliation dry run on history, load test at peak |
| **Rollout** | Start with a subset | Shadow mode + reconciliation gate, pilot vendors, rollback owner, hypercare with daily reconciles |
| **Trade-offs** | "Simple is better" | Names the real ones: delta vs mirror maintenance, cursor vs offset, push vs poll for inbound, sync vs async application, exact-once cost vs at-least-once+idempotency |

**Probing questions to expect (answer each in ≤45 seconds):**

1. "A vendor was deleted on their side. What happens here?" *(Soft-delete/tombstone or hard-delete handling depends on field ownership; reconciliation must detect orphans; decide and document — silent resurrection is the classic bug.)*
2. "Their token expires mid-run at record 30,000 of 50,000." *(Refresh path; if refresh fails, stop cleanly at a checkpoint, alert, resume; don't half-apply batches.)*
3. "They return 429 with Retry-After: 120." *(Honor it; adaptive pacing; queue depth grows — surface it; consider batching more per request.)*
4. "Both sides edited the same vendor's status." *(Conflict policy per field: last-writer-wins with version check → 409 → policy: source-of-truth field ownership or manual review; log every conflict with disposition.)*
5. "How do you prove, on day 30, that the sync is correct?" *(Reconciliation report: counts, hash/field-level diffs, dispositions; plus the ability to re-run against history.)*
6. "What's your rollback plan if the sync corrupts 2,000 records?" *(Stop the sync, restore from the audit log/backup, replay selectively, user comms; rollback owner named in advance.)*

---

### Round 5 — Production debugging (difficulty 3–4)

Run each as a 5-minute spoken drill using Part 8's hop list. The interviewer is testing *method*, not clairvoyance.

**D5.1 "The nightly sync job succeeded, but the customer says 40 vendors are stale."** Grade expecting: symptom definition first (scope/time/blast radius); check watermark lag not just job status ("succeeded" is about the job, the customer cares about freshness); compare counts pulled vs pushed vs skipped; look for poison records — a single failing record that aborts or silently skips the batch tail; check mapping of a recently added field; verify on the *customer's screen*, not your DB. Perfect answer ends with: "and then I'd add the alert that would have caught it — a staleness metric per record, not a job-level heartbeat."

**D5.2 "Duplicate vendors are appearing in the GRC system — but only for one integration."** Expect: find the write path; check idempotency keys or natural-key upserts on that path; check retry behavior — duplicates usually mean "write succeeded, ack lost, retry created a second row"; check matching key normalization (whitespace/case/nulls in the natural key); fix = dedupe target + harden key + backfill reconciliation; guardrail = unique constraint on (tenant, external_id) or idempotent upsert.

**D5.3 "Latency tripled after a release that only 'added logging'."** Expect: don't trust the release description, check the diff; look at the order of operations on the hot path (synchronously-written logs, verbose payload dumps, per-record round trips); compare p50 vs p99 (lock contention shows up in p99 first); use traces/`EXPLAIN`; speculative fix with metrics proof; revert-first discipline when customer impact is live. The teachable line: **"the release note tells you intent; the profiler tells you behavior."**

**Scoring anchors:** 2 = names a component and stops. 3 = bisects hops, names evidence. 4 = bisects, predicts evidence before collecting, then names the guardrail added.

---

### Round 6 — LLM extraction and production judgment (difficulty 3–4)

| # | Question | Expected answer points |
|---|---|---|
| 6.1 | "How do you make LLM document extraction production-grade?" | Constrained structured output + validator + repair loop with specific errors; evidence snippets verified against source (catches fabricated quotes); deterministic checks the model can't be trusted with (dates, sums, IDs, required fields — the hallucination trap); confidence + routing to human review on rules, not vibes; eval harness with golden set and per-field precision/recall + unsupported-claim rate; cost/latency controls via caching and model tiering |
| 6.2 | "The model returned a valid-looking JSON where a required field was missing. How did that pass your schema?" | The `required`-field trap: models can emit schemas that don't enforce required (or the provider silently drops), or the validator treated empty string as present; fix: explicit required-list check, `additionalProperties` handling, treat `""`/null as missing, and the repair loop must be told *which* fields are missing — then verify fix in the harness with that exact case |
| 6.3 | "A reviewer says the model 'invented' a certification number that matched a previous document." | Fabrication/leakage failure: verify evidence snippet exists verbatim in *this* document; add negative fixtures (fields absent from doc); check prompt context hygiene; add a deterministic checksum/format check on IDs; route low-confidence to humans; log the catch and add to the golden set |
| 6.4 | "Rough numbers: extraction costs 8c/doc and 12s latency. The customer has 40k docs. What do you do?" | Arithmetic first: 40k × 8c = $3,200 one-time; 12s × 40k ≈ 133h ≈ 5.5 days single-threaded — so parallelism or batching; then levers: cache duplicates (document hashes), tier models (cheap first pass, strong model on failure), chunk long docs, batch where the provider supports it; validate the acceptance criteria — precision target decides how much you can risk a cheap pass; propose measurement, not guesses |

---

### Round 7 — Customer-facing role play (difficulty 3–4)

Play these with a friend who is instructed to be impatient.

| # | Scenario | Expected behavior |
|---|---|---|
| 7.1 | "You have 20 minutes with the Head of Procurement. Open the discovery meeting." | Land the agenda; ask for the last concrete instance; commit to written read-back; no feature pitching; asks about the artifacts (spreadsheet/email) |
| 7.2 | "The process owner wants a nightly full reload; IT says their window is 1 hour and the export takes 4. Negotiate." | Convert to numbers (volume, window, delta vs full); propose delta + reconciliation; identify who owns the trade-off; offer an interim (split by region); confirm in writing. Never pretend the 4-hour export fits a 1-hour window |
| 7.3 | "Tell the customer: the feature they asked for will miss the audit date by two weeks." | Early, impact in their language, options with recommendation, written follow-up, next check-in time. No hedging, no blame, no false hope |
| 7.4 | "The customer asks: 'Does your product support SSO with our IdP? How does it work internally?'" | If you don't know: don't invent. "I believe standard SAML/OIDC flows are supported — let me confirm the exact configuration in our docs and come back today with the setup steps. What modes does your IdP support?" Turn the unknown into a checklist item with a deadline. **Inventing internals is an instant disqualifier** |

---

### Round 8 — Curveballs and adversarial (difficulty 4)

**Rapid-fire (15 seconds each, no preamble allowed):**

1. Is GET idempotent? Safe? → *Idempotent: yes (repeating has same effect); safe: yes (no intended side effects).*
2. When is POST safe to retry? → *When the endpoint is idempotency-key protected or the write is naturally keyed (natural-key upsert).*
3. What status code for "created"? → *201 with a Location header; 200 if the operation wasn't strictly a creation.*
4. What does 429 tell the client? → *Rate limited; retry with backoff and honor Retry-After.*
5. What is a hash set used for in these integrations? → *O(1) membership — duplicates detection, reconciliation sets, dedupe keys, local dedupe during a batch before hitting the DB.*
6. What's the difference between a DLQ and a retry queue? → *Retry = automated, bounded, transient errors; DLQ = terminal failures that need human triage and a controlled replay path.*
7. First thing to check when a sync stalls? → *Watermark/checkpoint progress and the oldest in-flight item — not the dashboards.* (Second answer acceptable: "last successful hop.")*
8. Where do you put the idempotency key? → *Request header from the client (e.g., Idempotency-Key); stored server-side with a scope and TTL.*
9. Why jitter in backoff? → *Desynchronize retries across clients to prevent synchronized retry storms.*
10. What breaks a unique constraint under retries? → *Reusing a key across different payloads — my lab's 409 `idempotency_key_reuse` case.*

**Adversarial questions (2 minutes each):**

- **A8.1** "You have zero years of customer-facing delivery experience. Why should we take the risk?" → Honest + specific + evidence of transferable behavior (Part 1.7 script): you already translate between technical and non-technical people; you ask before assuming; you own outcomes; and you name the gap with a plan ("I'd ramp on your delivery motion fast, in shadow mode on a real engagement"). Do not claim unearned experience.
- **A8.2** "You built a design for the sync. It's overengineered. Cut it." → Be able to cut to Phase 1 thin loop without defensiveness: one direction, nightly, delta, reconciliation report, manual conflict review. Show you can trade engineering elegance for delivery speed *with eyes open*.
- **A8.3** "The customer insists on using a connector we consider unreliable. What do you do?" → Get the reason (incumbent continuity, budget, compliance); quantify the risk in their terms (volume of manual cleanup per month); propose a contained pilot with reconciliation gates so failure is cheap; document the risk acceptance by the decision owner. You don't refuse the customer; you make the risk visible and bounded.
- **A8.4** "What's a time you were wrong, technically?" → A real story with: what you believed, what the evidence showed, what you changed, what process you added. Any candidate who cannot answer this scores 0 on self-awareness; refusing to answer scores worse.

---

### 11.9 Full mock score sheet (cover before use)

| Round | Focus | Correctness /4 | Production /4 | Filler/hedge count | Revisit |
|---|---|---|---|---|---|
| 1 | Warm-up / role | | | | |
| 2 | Core technical | | | | |
| 3 | Reliability | | | | |
| 4 | System design | | | | |
| 5 | Debugging | | | | |
| 6 | LLM extraction | | | | |
| 7 | Customer role play | | | | |
| 8 | Adversarial | | | | |

**Bands and the action they trigger:**

| Average | Interpretation | Action |
|---|---|---|
| ≥3.5 both columns | Interview-ready | Maintenance only: 30 min/day, Part 13 cheat sheet + one drill |
| 3.0–3.4 | Likely to pass, rough edges | Target the two lowest rounds with trace exercises in Part 15 |
| 2.0–2.9 | Not yet — predictable failure | Two focused days on the lowest two *parts*, then re-run those rounds |
| <2.0 | Foundation gap | Return to the corresponding part end-to-end; redo the lab artifact for that topic first (see Appendix A) |

### 11.10 Debrief template (fill this in within 30 minutes of practicing)

```
Round:           Date:
Score (C/4, P/4):
What I nailed (one line):
Where I actually lost points (specific, not "nervous"):
  - missed point #: 
  - hedge/filler example:
  - structure failure (no read-back / no trade-off / no edge case):
Next drill (one only):
Date of next rep:
```

**One-drill rule:** after each round, pick exactly one thing to fix and re-test it in the next rep. Candidates who "fix everything" fix nothing.

---

## Part 12: MCQ bank with explanations

**How to use:** cover the options, answer in your head, then read the explanation. Mark every miss in Part 15's tracker by topic. Target: ≥ 23/26 before the interview. Each question maps to a part — misses are pointers, not verdicts.

**Scoring bands:** 24–26 ready · 21–23 one focused re-read of the missed topics · 17–20 redo the relevant part's drills · < 17 postpone mock design rounds and rebuild fundamentals first.

---

### Topic 1 — Idempotency (Part 2.2, 6.2)

**Q12.1** Which HTTP method is **not** idempotent by specification?

- A. GET
- B. PUT
- C. DELETE
- D. POST

**Answer: D.** POST creates a new resource on each call. GET is safe and idempotent, PUT replaces the full resource (repeat = same state), DELETE removes (second call may 404, state unchanged). Note: PATCH is *not* guaranteed idempotent either — the common trap in "which of these is idempotent?" questions.

**Q12.2** A server stores `Idempotency-Key → response`. A retry arrives with the **same key but a different body**. Strict, safest behavior?

- A. Execute it — the body is different, so it's a new request
- B. Return the stored response as if it were the same request
- C. Reject with a 4xx/409 conflict indicating key reuse
- D. Queue it for manual review

**Answer: C.** Execution double-applies; returning the old response masks a client bug (the client believes the new body was processed). Strict mode compares a body hash and rejects with a machine-readable error — verified in the companion lab as `409 idempotency_key_reuse`. **B is the sneaky distractor** — replay with the *same* body must return the stored response; replay with a *different* body must be rejected.

**Q12.3** Where must the idempotency record be written, relative to the side effect?

- A. Before, in its own transaction
- B. In the same transaction as the side effect
- C. After, asynchronously
- D. Order doesn't matter

**Answer: B.** If the effect commits but the key doesn't, a retry double-applies. If the key commits but the effect doesn't, a retry returns "success" for work never done — the worse failure (silent data loss). Same-transaction makes the pair atomic; that is the whole point of the pattern.

---

### Topic 2 — HTTP 429 and rate limits (Part 2.3, 7.9)

**Q12.4** A client receives `429` with `Retry-After: 30`. Best action?

- A. Retry immediately in a tight loop
- B. Wait 30 seconds, then retry, and reduce concurrency/pacing adaptively
- C. Treat as a permanent failure and send to the DLQ
- D. Increase batch size to finish the backlog faster

**Answer: B.** 429 is explicit backpressure: the server is telling you it's alive and wants less load. Honor `Retry-After`; add adaptive pacing so the system settles below the limit instead of oscillating. C loses data unnecessarily; D makes the situation worse and can trigger harder throttling.

**Q12.5** Which retry policy best prevents retry storms?

- A. Fixed 1-second retries, unlimited attempts
- B. Exponential backoff with full jitter and a cap, plus max attempts
- C. No retries; alert on first failure
- D. Retry on every error code

**Answer: B.** Exponential growth reduces pressure over time; jitter desynchronizes many workers so they don't retry in lockstep; the cap bounds recovery time; max attempts routes terminal failures to a DLQ. A is a synchronized stampede; C sacrifices availability unnecessarily; D retries permanent errors (4xx) and wastes budget.

---

### Topic 3 — Hash sets and data structures (Part 4.5, 7.5)

**Q12.6** You must dedupe 2 million record IDs in a batch before upserting. Simplest structure with expected O(1) membership?

- A. A list with `if x in my_list`
- B. A `set`
- C. A sorted list plus binary search
- D. A dict mapping each ID to itself

**Answer: B.** Set membership is expected O(1) via hashing; list `in` is O(n) → the loop becomes O(n²) (~2M² comparisons — minutes to hours). Sorting costs O(n log n) plus maintenance on inserts. D works but stores unnecessary values. Real-world nuance: memory grows with the set, so for very large batches process in chunks — and the database unique constraint *remains* the true guard (next question).

**Q12.7** Why is a database unique constraint still needed when the application dedupes with a set first?

- A. Sets are too slow for databases
- B. Multiple workers each dedupe locally but not against each other
- C. Sets cannot hold strings
- D. It isn't needed — the set is sufficient

**Answer: B.** In-process dedupe sees one worker's world; two workers can still race to insert the same natural key. The unique index enforces identity globally; catching the integrity error and treating it as "already applied" completes the idempotent upsert. Belt-and-braces, not redundancy.

---

### Topic 4 — Exactly-once and delivery semantics (Part 5.6, 6.1)

**Q12.8** A vendor advertises "exactly-once webhook delivery." Best interpretation?

- A. Duplicates are impossible; no dedupe needed on your side
- B. Treat it as at-least-once and implement idempotent consumers anyway
- C. Expect at-most-once delivery
- D. It means messages arrive exactly once under retry conditions

**Answer: B.** Across a network boundary with independent failure, exact delivery cannot be guaranteed end-to-end; "exactly-once" usually describes *their internal* processing or their dedupe keys. Your receiver still needs durable dedupe by event ID — and note event IDs must be *in the payload*, since headers can be lost when events are forwarded internally. Verify the claim; design for the failure.

**Q12.9** Which combination yields effectively-once **processing**?

- A. Acknowledge after processing, with no dedupe
- B. At-least-once delivery + dedupe key/unique constraint + idempotent effects, committed atomically
- C. An exactly-once queue alone
- D. Disabling retries entirely

**Answer: B.** You cannot prevent redelivery across system boundaries; you make it harmless. The inbox pattern writes the event ID and the side effect in one transaction, so a redelivery is a no-op. C: broker-level transactions have a boundary — crossing into your database is where exactly-once claims usually break. D trades duplicates for data loss.

**Q12.10** At-least-once delivery, and a consumer crashes **after** applying the effect but **before** acknowledging. What happens without consumer idempotency?

- A. The message is lost
- B. The effect is applied twice on redelivery
- C. Nothing — the broker knows
- D. Only ordering is violated

**Answer: B.** The classic duplicate window: the broker saw no ack, so it redelivers; the effect was already applied. Fix: dedupe record + effect in the same transaction (inbox), or make the effect naturally idempotent (upsert on natural key).

---

### Topic 5 — SQL indexes (Part 3.6)

**Q12.11** Query: `SELECT * FROM vendors WHERE tenant_id = 7 AND status = 'active' ORDER BY updated_at DESC LIMIT 20`. Best index?

- A. `(status)`
- B. `(tenant_id, status, updated_at)`
- C. `(updated_at)`
- D. `(tenant_id, updated_at)`

**Answer: B.** Equality columns first (tenant_id, status), then the sort column — the index satisfies the filter *and* the ordering, so `LIMIT 20` reads ~20 entries. A has poor selectivity alone; C ignores the filters; D filters only on tenant and then has to look up `status` per candidate row. This is the "equality, then range/sort" composite rule.

**Q12.12** Index exists on `(tenant_id, created_at)`. Which query **cannot** use it efficiently for a seek?

- A. `WHERE tenant_id = ? ORDER BY created_at`
- B. `WHERE tenant_id = ? AND created_at > ?`
- C. `WHERE created_at < ?` (no tenant filter)
- D. `WHERE tenant_id = ? ORDER BY created_at LIMIT 10`

**Answer: C.** The leftmost-prefix rule: the index is ordered by tenant_id first, so a query that skips tenant_id cannot seek into the tree — it scans. A, B, and D all start with the prefix and are ideal for this index. Practical takeaway: column *order* in a composite index is a design decision driven by the query workload.

**Q12.13** When should you **not** add an index?

- A. On a foreign-key column used in joins
- B. On a boolean/status column where the query returns a large fraction of the table
- C. On a column filtered in every hot query
- D. On the sort column of a top-N query

**Answer: B.** Every index taxes writes (inserts/updates must maintain it) and consumes storage; on low-cardinality columns where the queried value matches most rows, the planner may ignore it anyway (full scan is cheaper). A, C, D are textbook good candidates. The grown-up answer always ends with: "measure with EXPLAIN before and after."

---

### Topic 6 — Race conditions and transactions (Part 3.7, 6.8)

**Q12.14** Two workers both read a job row with `status='pending'`, then both process it. Root cause and correct fix?

- A. Read-committed isolation is broken; switch to serializable
- B. Check-then-act (TOCTOU) race; fix with an atomic claim — `UPDATE ... SET status='processing' WHERE id=? AND status='pending'` and check affected rows = 1
- C. Add a random sleep between check and act
- D. Retry the whole job

**Answer: B.** The gap between "check" and "act" is where the race lives. The single atomic UPDATE is the claim: exactly one worker can transition the row; affected-rows = 0 means someone else won. Serializable isolation would serialize everything at the cost of throughput and deadlock retries — a sledgehammer for a problem an atomic statement solves.

**Q12.15** What does `SELECT ... FOR UPDATE SKIP LOCKED` primarily enable?

- A. Faster reads
- B. A worker pool claiming disjoint rows without blocking on each other's locks
- C. Complete deadlock elimination
- D. Guaranteed FIFO ordering

**Answer: B.** Each worker locks the rows it claims and *skips* rows already locked by others instead of waiting — so N workers process N disjoint sets. It reduces but does not eliminate deadlocks (C), and it provides no ordering guarantees (D). Always combine with an explicit claim/status update so the lock's lifetime isn't the only protection.

**Q12.16** A money transfer decrements account A and increments account B. What guarantees neither half is applied alone?

- A. Autocommit mode
- B. Both statements inside one transaction — atomicity on commit
- C. Retrying the operation
- D. Logging both statements

**Answer: B.** A transaction's commit is all-or-nothing; a crash mid-transaction rolls both back. The subtle extension: keep *external* side effects (API calls, emails) out of the transaction and use the outbox pattern — you cannot atomically commit a database and an HTTP call.

---

### Topic 7 — OAuth 2.0 (Part 2.4)

**Q12.17** Which OAuth 2.0 flow suits a nightly backend sync with no human present?

- A. Implicit flow
- B. Authorization code flow driven by a browser each night
- C. Client credentials flow
- D. Resource owner password grant

**Answer: C.** Client credentials is machine-to-machine: the service authenticates with its own client ID/secret, no user context. Implicit is deprecated for security reasons. Password grant is legacy/discouraged and needs a stored user password. Caveat worth saying: if the integration must act *as a specific user* (their permissions, their audit trail), use authorization code + PKCE, obtain a refresh token once with the user present, and store it securely.

**Q12.18** What is the refresh token for?

- A. It replaces the access token permanently
- B. It obtains new access tokens without re-authenticating the user
- C. It signs webhook payloads
- D. It encrypts request bodies

**Answer: B.** Access tokens are short-lived by design (limited blast radius). The refresh token, exchanged at the token endpoint, issues new access tokens. Operational rule that separates good candidates: **alert on refresh failures** — a silently dead refresh path is the most common way "working" integrations go stale.

**Q12.19** You refresh the token and the very next call still returns 401. Best reaction?

- A. Retry in a loop until it works
- B. Treat as terminal auth failure: log with correlation ID, alert, pause the sync with a visible status
- C. Switch to unauthenticated calls
- D. Ignore and continue with other records

**Answer: B.** Refresh-then-401 means credentials were revoked, scopes changed, or the endpoint moved — retrying is useless and noisy (and can trip lockouts). Pause *visibly*: the worst state is a sync that "runs" green while processing nothing. C violates the contract; D produces partial silent staleness.

---

### Topic 8 — Schema evolution (Part 2.6)

**Q12.20** Upstream adds a new optional field. Safest consumer behavior?

- A. Reject payloads containing unknown fields so nothing unexpected is stored
- B. Ignore unknown fields, parse the fields you know, and log schema drift
- C. Store raw payloads only
- D. Alert a human on every payload

**Answer: B.** Additive changes are the common, safe case: tolerant reads keep the integration alive. Logging drift means you *discover* a new field worth consuming instead of waiting for a customer to ask. A causes an outage over a harmless addition; D creates alert fatigue. (Where a compliance contract demands strictness, enforce it at a versioned boundary instead.)

**Q12.21** Upstream is renaming `vendor_name` to `legal_name`; during transition, either field may appear. Correct posture?

- A. A single regex fallback, silent
- B. Version detection + adapter mapping with explicit tests for both shapes; never silently default to empty
- C. Freeze the pipeline until the transition completes
- D. Switch to manual CSV loads

**Answer: B.** Read either field, record which one was used, alert when the pattern changes so you know when to remove the legacy branch. The trap to *name in the interview*: silently defaulting a renamed field to `""`/null produces plausible-but-wrong data — the worst failure class in compliance systems, because nothing errors and reports quietly lie.

---

### Topic 9 — HTTP 201 and creation semantics (Part 2.3)

**Q12.22** A POST creates a resource. Correct success response?

- A. `200 OK` with a body
- B. `201 Created` with a `Location` header; body optional
- C. `204 No Content` always
- D. `302 Found`

**Answer: B.** 201 tells the client a creation *occurred* (not just "processed"), and Location points at the canonical URL of the new resource so the client can fetch/confirm it. 200 is acceptable only when "created" isn't guaranteed (e.g., a create-or-return-existing operation). 204 is for success with no body; 302 is a redirect, not a creation signal.

**Q12.23** An idempotent retry replays the **same** POST with the **same** key that originally returned `201`. What should the retry get?

- A. `201` again only if it was actually recreated
- B. `409 Conflict` in all cases
- C. The stored original response (a `201` with the same body), served from the idempotency record
- D. `200 OK` to distinguish replay

**Answer: C.** Replay returns the recorded outcome so the client's retry logic sees a success consistent with what actually happened — no second resource is created. 409 is reserved for *key reuse with a different payload* (Q12.2). Distinguishing replay with 200 breaks naive clients that check for 201.

---

### Topic 10 — Cursor pagination (Part 2.5)

**Q12.24** Keyset pagination uses `WHERE (updated_at, id) > (?, ?)`. Why include `id`?

- A. Style — underscores look better
- B. `updated_at` is not unique; the tie-breaker creates a strict total order, preventing skipped or duplicated rows at page boundaries
- C. It makes the query faster
- D. To obfuscate the cursor

**Answer: B.** Without a unique tie-breaker, rows sharing a timestamp have ambiguous relative order: page 2 may re-serve or skip them. Composite `(updated_at, id)` makes the ordering deterministic; with a matching index it's also efficient. This exact composite is the backbone of the sync watermark in Part 7.

**Q12.25** Which is a legitimate **drawback** of cursor pagination?

- A. It skips rows under concurrent writes
- B. No random access to page N, and cursors must be treated as opaque, validatable state (invalidated when the ordering/mapping changes)
- C. It secretly requires OFFSET anyway
- D. It only works with integer keys

**Answer: B.** You can't jump to "page 40" — irrelevant for sequential syncs, relevant for UIs. Cursors must not be user-forged blindly (base64 ≠ encryption; validate or sign), and an ordering change makes old cursors meaningless — version them.

**Q12.26** Why does offset pagination duplicate or skip rows when records are inserted between page fetches?

- A. A database bug
- B. The window shifts: insertions before your offset push rows across page boundaries
- C. Caching of the result set
- D. `LIMIT` is inherently unstable

**Answer: B.** Fetch page 1 (rows 0–99), someone inserts 10 rows at the top, fetch page 2 (rows 100–199) — ten rows are served twice (and deletions cause skips). That's why record-by-record syncs walk a stable, unique watermark instead of an offset counter. Page-number UIs accept the trade-off because humans tolerate it; pipelines cannot.

---

**Tally:**

| Topic | Questions | Score /n | Missed → revisit |
|---|---|---|---|
| Idempotency | 3 | | Part 2.2 / 6.2 |
| 429 & rate limits | 2 | | Part 7.9 |
| Hash sets | 2 | | Part 4.5 / 7.5 |
| Exactly-once semantics | 3 | | Part 5.6 / 6.1 |
| SQL indexes | 3 | | Part 3.6 |
| Race conditions | 3 | | Part 3.7 / 6.8 |
| OAuth 2.0 | 3 | | Part 2.4 |
| Schema evolution | 2 | | Part 2.6 |
| HTTP 201 | 2 | | Part 2.3 |
| Cursor pagination | 3 | | Part 2.5 |
| **Total** | **26** | | |

---

