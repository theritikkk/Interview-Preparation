# Event-Driven Architecture: Event Sourcing, CQRS, Transactional Outbox

🔴🔴🔴 This is the single most conceptually dense cluster behind Hermes. Full architectural justification (why chosen, alternatives, failure modes) lives in `13_PROJECT_DEEP_DIVES/HERMES.md`; this file is the pattern-level theory.

---

## Cluster 1: Event Sourcing

### What is it?
Instead of storing an entity's *current state* (the conventional approach — a row you overwrite on each update), Event Sourcing stores the full sequence of **events** that led to that state — the state is never stored directly; it's *derived* by replaying events. The append-only event store is the single source of truth; everything else (including "current state") is a projection of it.

### Why does it exist?
Conventional CRUD storage discards history — once you `UPDATE` a row, the prior value is gone unless you separately built an audit log. Event Sourcing makes history the *primary* artifact: you get a complete audit trail for free, the ability to answer "what did this look like at time T" by replaying events up to that point, and the ability to build entirely new projections/read models later by replaying the same historical events through new logic — none of which conventional state-only storage supports without bolt-on extra infrastructure.

### How does it work?
```
Traditional:  UPDATE tasks SET status = 'completed' WHERE id = 5;   -- prior status is gone
Event-sourced: APPEND TaskCompleted{ taskId: 5, completedAt: ..., completedBy: ... }
               -- current state = replay ALL events for taskId 5, apply each in order
```

### Interview Questions
- **L1:** "Event Sourcing stores the sequence of events that happened, not just current state — current state is derived by replaying events, giving a complete audit trail and the ability to rebuild new views from history at any time."
- **L2:** explain the real cost this trades against the benefit: reading "current state" now requires replaying (or reading a pre-built projection of) potentially many events, instead of a single row read — which is *exactly why* CQRS (Cluster 2) exists as its natural pairing: maintain pre-computed read models so you don't replay history on every read.
- **L3:** explain **snapshotting** as the standard mitigation for long event streams: periodically persist a computed state snapshot plus the version it represents, so replay only needs to start from the last snapshot forward, not from the beginning of time. `NEEDS VERIFICATION`: confirm whether Hermes implements snapshotting, or relies entirely on the 6 read models to avoid ever needing full replay for normal operation — both are legitimate designs, but you should know which one yours is.

### DO NOT SAY THIS
"Event Sourcing is the same as just keeping an audit log." — An audit log is typically a side effect bolted onto conventional state storage; in Event Sourcing, the event log **is** the storage — there is no separate "real" state being audited, which is the actual architectural distinction.

---

## Cluster 2: CQRS (Command Query Responsibility Segregation)

### What is it?
Separate the **write model** (handles commands, enforces business rules, appends events) from the **read model** (optimized purely for querying, often pre-computed/denormalized) — instead of one model trying to serve both well.

### Why does it exist?
Write-optimized and read-optimized data shapes are often fundamentally different (normalized vs. denormalized, per `03_CS_FUNDAMENTALS/DBMS.md`) — a single model serving both tends to compromise on both. CQRS lets each side be optimized independently, and pairs naturally with Event Sourcing: the event store is the write side's source of truth; read models are projections built by consuming those events.

### How does it work?
```
Command → Write Model (validates, appends event to store)
                              ↓ (DynamoDB Streams)
                        Event Consumer/Projector
                              ↓
                    Read Model 1, Read Model 2, ... Read Model 6
                    (each shaped for a specific query pattern)
Query → reads directly from the appropriate pre-built read model — NOT from the event store
```

### Interview Questions
- **L1:** "CQRS separates the write path (commands, business rules, event append) from the read path (pre-built, query-optimized projections) instead of using one model for both."
- **L2 — 🔴 the certain "why six?" question:** explain that six separate read models means six distinct query patterns the system needs to serve efficiently — each one denormalized specifically for its own access pattern (e.g., "tasks by tenant," "tasks by assignee," "audit trail by entity," etc.) rather than one general-purpose model requiring joins or filtering at query time. `NEEDS VERIFICATION`: know your actual six and be ready to name what query each one serves — "why six" without being able to name even a few of them plausibly is a weak answer.
- **L3:** explain the consistency implication directly, tying back to `07_DISTRIBUTED_SYSTEMS/CONSISTENCY.md`: because read models are built asynchronously from events, they are *eventually* consistent relative to the write side — a query immediately after a command might not yet reflect it. Be ready to explain how your system handles (or explicitly accepts) this — e.g., returning the freshly-appended event's data directly from the command's response rather than requiring the caller to immediately re-query a read model.

### Follow-up Chain
> **Q: Why not just query the event store directly instead of maintaining six separate read models?**
> A: The event store is optimized for appending and replaying a single aggregate's history in order — it's a poor fit for cross-aggregate queries (e.g., "all tasks for this tenant, sorted by priority"), which is exactly what a purpose-built read model is for.
> **Follow-up: Isn't maintaining six read models a lot of operational overhead and potential inconsistency surface, compared to one general-purpose queryable store?**
> A: Yes — that's the real cost of CQRS, and it's a deliberate tradeoff: more moving parts and an eventual-consistency window, in exchange for each read model being simple, fast, and purpose-built rather than one model trying to serve six different access patterns adequately.
> **Trap: "CQRS means using two different databases."**
> Correct response: CQRS is about separating the *models* (write logic vs. read logic), not necessarily the physical database technology — you can implement CQRS within a single database. In Hermes's case, DynamoDB likely hosts both the event store and the read model tables (possibly as separate tables), but the pattern itself doesn't mandate different database *technologies* — don't conflate "CQRS" with "polyglot persistence," which is a related but separate decision.

---

## Cluster 3: Transactional Outbox Pattern

### What is it?
The **dual-write problem**: if you need to both (a) update your own database and (b) publish a message to an external system (a queue, another service) as a result of the same logical operation, doing these as two separate, non-atomic operations means one can succeed while the other fails — leaving your data and your downstream notification permanently inconsistent, with no way to know which failed without extra machinery. The Transactional Outbox pattern solves this by writing *only* to your own database, atomically, including a record of "this needs to be published" — then a separate mechanism reliably picks that up and publishes it.

### How does it work? (Hermes's specific implementation)
```
1. Single atomic write to DynamoDB: append the event (this IS the "outbox" — there's no
   separate outbox table needed, because the event store itself serves that role)
2. DynamoDB Streams (a native, guaranteed-to-fire mechanism tied directly to the committed
   write — not a separate network call your application code could fail to make) picks up
   the change
3. Streams → Lambda → publishes to SQS → SQS → EventBridge (fan-out to consumers)
```
The critical insight: **the "outbox check" is DynamoDB Streams itself** — because Streams is driven directly off the already-committed write (not a second, independently-failable network call your application makes), there is no window where the event is durably stored but the "notify downstream" step is silently skipped. If the write commits, Streams *will* eventually see it.

### Interview Questions
- **L1:** "The Transactional Outbox pattern avoids the dual-write problem by writing only to your own database atomically — including a durable record of what needs to be published — instead of writing to your database and a separate message broker as two independently-failable operations."
- **L2:** explain precisely why a naive alternative (write to DynamoDB, then directly call SQS to publish) is broken: if the process crashes *between* the DynamoDB write succeeding and the SQS publish call, the event is durably stored but never published — permanently. No retry logic on the SQS call alone fixes this, because the crash could happen at any point, including after SQS accepts the message but before your code confirms it (an ack-loss scenario) — the fundamental problem is doing two independent operations where only one has to fail for the system to end up in an inconsistent state.
- **L3 — 🔴 the deepest version of this question:** explain why DynamoDB Streams specifically closes this gap where a naive approach can't: Streams isn't "your application code calling out to another system" — it's a change-data-capture mechanism operating directly on the table's committed write log, guaranteed by DynamoDB itself to eventually reflect every committed write. There's no code path where a commit succeeds but Streams simply "doesn't get around to" seeing it — this is what makes it architecturally sound as an Outbox implementation rather than just "polling and hoping."

### Comparison With Alternatives
A traditional relational-database Outbox implementation (common in a Postgres-based system) uses an explicit `outbox` table written in the *same transaction* as the business data, then a separate poller (or Debezium-style CDC reading the WAL) publishes from that table and marks rows processed — conceptually identical to Hermes's design, just with DynamoDB Streams standing in for the WAL-tailing CDC process, since DynamoDB has no external WAL to tail directly.

### Quick Revision
- Event Sourcing: events are the source of truth; state is derived by replay (or served via a projection/read model).
- CQRS: separate write model (commands, event append) from read model (pre-built, query-optimized projections) — six read models = six distinct query patterns, know what they are.
- Dual-write problem: two independent operations, only one needs to fail for inconsistency — solved by writing only to your own store atomically.
- Hermes's Outbox = DynamoDB Streams itself, driven directly off the committed write, not a second failable network call.
