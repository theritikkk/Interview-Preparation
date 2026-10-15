<!-- Certa FDE mastery workbook — split part 4 of 7 (source lines 1891-2643). Concatenating all 7 files in order reproduces certa_fde_interview_mastery.md. -->

## Part 6: Idempotency, transactions, outbox, queues, retries, DLQ, replay

> This is the part of the interview where "can you build it" turns into "can you run it". Every scenario below is analysed against the same seven questions, because the user asked for exactly that: **duplicate request? concurrent workers? process crash? network timeout? partial failure? retry safety? recovery?**

### 6.1 Failure taxonomy: name the failure before you fix it

| Failure class | Definition | Is it retryable? | Detection |
|---|---|---|---|
| Transient infrastructure | Timeout, connection reset, 429, 502/503/504 | Yes, with backoff | Exception/timeout, status code |
| **Unknown outcome** | The request may or may not have been applied | Only with an idempotency key | Timeout on a non-idempotent call |
| Permanent contract error | 400/401/403/404/409*/422 | No (409 needs a re-read first) | Status code + body |
| Data-quality error | A field the destination's enum rejects | No — quarantine | 422 with a field-level message, or a local validator |
| Poison message | Always fails, deterministically | No — DLQ after N attempts | Same error on every attempt |
| Partial failure | Some items in a batch succeeded | Per item | Per-item results (207-style) |
| Logic/regression error | Your code is wrong | No — roll forward with a fix | Error rate change correlated with a deploy |
| Capacity failure | Queue growth, worker starvation, lock waits | Yes, but the *cause* needs fixing | Queue depth, oldest-age, DB lock metrics |

**The most important row is "unknown outcome"** — it is where duplicates come from, and it is why non-idempotent POSTs need keys.

### 6.2 Idempotency keys: the full design [ASSESSMENT: idempotency]

**First principles:** an idempotency key converts "did this happen?" into a question the database can answer. Without it, a timeout leaves you guessing forever; with it, the retry is a *lookup*.

**Server-side contract:**

```sql
CREATE TABLE idempotency_keys (
  key              VARCHAR(80)  NOT NULL,        -- client-supplied, high entropy
  tenant_id        VARCHAR(40)  NOT NULL,        -- scope: keys are per tenant!
  request_hash     CHAR(64)     NOT NULL,        -- sha256 of the canonical body
  status           ENUM('in_progress','completed','failed') NOT NULL,
  response_code    INT           NULL,
  response_body    JSON          NULL,
  created_at       TIMESTAMP     NOT NULL DEFAULT CURRENT_TIMESTAMP,
  expires_at       TIMESTAMP     NOT NULL,
  PRIMARY KEY (tenant_id, key),                  -- the dedupe guarantee
  KEY idx_expires (expires_at)
) ENGINE=InnoDB;
```

**The algorithm, including the concurrency case:**

```
1. Receive request with Idempotency-Key: K and body B. Compute hash(B).
2. INSERT INTO idempotency_keys (tenant, K, hash, 'in_progress', expires_at=now+7d)
     -> success: I own this request. Proceed.
     -> duplicate key error: someone else has K. Continue to 3.
3. Read the existing row:
   a. request_hash != hash(B) -> 409 idempotency_key_reuse   (never return the other
      request's response -- that is silent corruption)
   b. status = 'completed'    -> return the stored response_code/body verbatim
   c. status = 'in_progress'  -> the first attempt is still running. Return 409 with
      Retry-After (or block briefly and re-read). Do NOT process concurrently.
   d. status = 'failed'       -> policy decision: allow the retry (typical for 5xx) or
      return the stored failure (typical for 4xx). Document it.
4. On completion, UPDATE the row to 'completed' with the response, in the SAME
   transaction as the business effect.
```

**Verified in the lab (Part 7):** the destination applied a batch, then the response was lost as a 500. The client retried with the **same key** and received the cached result — one write, and the client learned the truth. The lab additionally proves the guard: reusing that key with a *different* body returns **409 `idempotency_key_reuse`** rather than the first request's answer.

**Verified in SQL (Part 3):** the same protection at the data layer — replaying a stale version through the upsert leaves `SourceVer = 5` intact, and the second `INSERT IGNORE` of a processed event ID reports `ROW_COUNT() = 0` with exactly one row stored.

| Design decision | Options | Recommendation and reasoning |
|---|---|---|
| Key generation | Client UUIDv4/ULID vs server-assigned | Client generates. The client is the one that knows a retry is a retry. Never derive it from the payload alone — two legitimate identical requests would collide |
| Scope | Global vs per tenant vs per endpoint | **Per tenant, per operation type.** Global keys across tenants let one tenant's key block another's (a denial-of-service vector) |
| TTL | 1 hour / 24 hours / 7 days | ≥ the provider's maximum retry window. If a provider retries for 24 h, a 1-hour TTL is not enough. 24 h–7 d is typical; the natural unique key is the permanent backstop |
| Storage | DB table vs Redis | DB for the guarantee (unique constraint + same-transaction effect); Redis as a fast-path cache in front. Redis alone is not a correctness guarantee — eviction and failover lose keys |
| Response caching | Store status + body | Yes — the client deserves the same answer, and it lets you return the *original* 201 with the original `Location` |
| Failure retry | Allow re-use after a failed attempt? | Allow for 5xx/timeouts (the outcome is unknown), refuse for 4xx (the request is wrong, a retry with the same key would fail identically) |
| What if the client omits the key? | Reject vs accept | Accept, but the endpoint is now at-most-once-safe only if a natural unique key exists. Prefer a natural unique key (e.g. `(tenant, source_system, external_vendor_id)`) **in addition to** the header |

**The natural key is the permanent guarantee.** Idempotency keys expire; a unique constraint on the business identity does not. Best practice is both:

```sql
ALTER TABLE certa_vendors
  ADD UNIQUE KEY uq_vendor_external (tenant_id, source_system, external_vendor_id);
```

Now, even if a key expires or a client forgets it, the database refuses the duplicate: **the constraint is the contract, and the key is the optimization.**

### 6.3 Transactions: the boundary is a design decision

**Rules that prevent most incidents:**

1. **One transaction = one atomic state change.** Two independent writes that must both happen belong in the same transaction; two writes that can tolerate a gap belong apart (longer locks cost throughput).
2. **Never hold a transaction open across a network call.** Locks held for the duration of an HTTP request to a third party turn their latency into your lock contention (and a timeout can leave a lock held for seconds). Do the external call outside the transaction, then open a short transaction to record the outcome.
3. **Idempotent effects inside the transaction, external side effects through an outbox.** The database can roll back; an email cannot.
4. **Commit status with the effect.** "Mark the job done" and "apply the state change" must be one commit, or a crash between them either loses work or duplicates it.
5. **Keep transactions short and lock in a deterministic order** (`ORDER BY id`) to avoid deadlocks.
6. **Use savepoints for partial rollback inside long batch transactions** — or better, one transaction per item, so one bad item does not discard 499 good ones.

**The dual-write problem (state this explicitly in the interview):**

```
WRONG:                                   RIGHT: transactional outbox
BEGIN                                    BEGIN
  UPDATE vendor SET tier='high'            UPDATE vendor SET tier='high'
COMMIT                                     INSERT INTO outbox (event_type, payload, ...)
publish to queue  <-- crash here             (same transaction)
  => state changed, no event sent          COMMIT      <-- atomic
                                           [publisher worker] reads outbox -> publish -> mark sent
```

Between `COMMIT` (state) and `publish` (queue) the process can die, and the event is lost forever — the state says "high", the downstream still says "low", and nothing will ever reconcile them because nobody knows an event was supposed to exist.

### 6.4 Transactional outbox (the pattern you should be able to write from memory)

```sql
CREATE TABLE outbox (
  id             BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  aggregate_type VARCHAR(40)  NOT NULL,        -- 'vendor'
  aggregate_id   VARCHAR(40)  NOT NULL,        -- 'V001'
  event_type     VARCHAR(60)  NOT NULL,        -- 'vendor.tier_changed'
  payload        JSON         NOT NULL,
  dedupe_key     VARCHAR(120) NOT NULL,        -- consumer-side idempotency
  status         ENUM('pending','published','failed') NOT NULL DEFAULT 'pending',
  attempts       INT  NOT NULL DEFAULT 0,
  next_attempt_at TIMESTAMP NULL,
  created_at     TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  published_at   TIMESTAMP NULL,
  UNIQUE KEY uq_outbox_dedupe (dedupe_key),    -- one event per business change
  KEY idx_outbox_drain (status, next_attempt_at)
) ENGINE=InnoDB;
```

**Producer:** state change + outbox insert in one transaction (no queue call inside the transaction).
**Publisher worker:** claim a batch with `SELECT ... WHERE status='pending' AND next_attempt_at <= NOW() ORDER BY id LIMIT 100 FOR UPDATE SKIP LOCKED`, publish, then mark published — publishing is retried until it succeeds, so nothing is lost, and the consumer's `dedupe_key` makes the resulting duplicates harmless.
**Inbox (consumer side):** the mirror image — insert the event key, apply the effect, commit, ack.

| Alternative to polling the outbox | When to choose it | Trade-off |
|---|---|---|
| CDC (Debezium-style log tailing) | High volume, want zero polling latency, can operate Kafka Connect | Operational complexity; you now depend on the DB's binlog format |
| Broker transaction (Kafka EOS) | Already on Kafka; need exactly-once inside the Kafka boundary | Ties you to one broker; external effects still need idempotency |
| Poll the outbox | Low-to-moderate volume, simplest operationally, idempotent by design | Added latency (poll interval) and DB load |

**`FOR UPDATE SKIP LOCKED`** (MySQL 8+) is the key primitive for a multi-worker queue table: each worker takes rows nobody else has locked, so no worker blocks another and nothing is double-claimed.

### 6.5 Queues: what the broker guarantees and what it does not

| Concept | Meaning | Why interviewers ask |
|---|---|---|
| At-least-once | Message is redelivered until acked | Your consumer must be idempotent, always |
| Visibility timeout / lease | A claimed message becomes invisible for N seconds; if not acked, it returns | Too short → duplicate processing of long jobs; too long → slow recovery from a crash |
| Ack / nack | Success → delete; failure → requeue (often with a delay or to a DLQ) | Every system names this differently (SQS visibility, RabbitMQ ack/nack, Kafka offset commit) |
| FIFO vs standard | Ordering guarantee vs throughput | True global ordering does not scale; use per-entity ordering with a partition key |
| Message key / partition key | Routes related messages to the same consumer | **Vendor ID as the partition key gives you per-vendor ordering without a global bottleneck** |
| Prefetch / concurrency | Unacked messages held per consumer | Too high → bursty failure amplification; too low → idle capacity |
| Backoff, then DLQ | Delayed retry, then quarantine | Retries without a ceiling become a poison loop |
| Dead-letter queue | Where exhausted messages go | Preservation of context is what makes replay possible |
| Exactly-once (Kafka EOS) | Transactional producer+consumer offsets | Powerful *inside* Kafka; does not make your HTTP call idempotent |
| Queue sizing | Little's law: `L = λ × W` | 20 msg/s × 30 s per message = 600 in flight; concurrency must be ≥ λ × W / target latency |

**Sizing example to say out loud:** *"If the peak is 20 events/second per tenant and each takes 300 ms, I need about 6 concurrent workers to keep up — 20 × 0.3 — so I would provision 6–8 with headroom and alert at sustained 80% utilisation, rather than discovering it from the queue-depth graph."*

**Why not just process synchronously in the webhook handler?** Answer: *"Because the provider's timeout is not my time budget. If the downstream takes 3 s and the provider times out at 2 s, it retries while I am still working, and now I am processing the same event concurrently with myself. Accept, persist, ack, then process asynchronously."*

### 6.6 Retries: the difference between resilience and a self-inflicted outage

**Classification first, then policy:**

```python
RETRYABLE = {429, 500, 502, 503, 504}          # plus timeouts and connection errors
NEVER_RETRY = {400, 401, 403, 404, 405, 422}   # fixing the request is the only fix
CONDITIONAL = {409, 412}                       # re-read, re-apply policy, retry once
```

**Backoff math (be able to state the numbers):**

```python
delay = min(cap, base * 2 ** (attempt - 1))          # exponential, capped
delay = random.uniform(0, delay)                     # "full jitter" -- spreads the herd
# base=0.5s, cap=30s  ->  attempt 1: 0-0.5s, 2: 0-1s, 3: 0-2s, 4: 0-4s, 5: 0-8s ... capped
```

| Jitter strategy | Formula | Character |
|---|---|---|
| None | `delay` | Synchronised retry storms |
| Full jitter | `random.uniform(0, delay)` | Best spread, highest variance in latency (used in the lab) |
| Equal jitter | `delay/2 + random.uniform(0, delay/2)` | Middle ground |
| Decorrelated | `random.uniform(base, delay*3)` | Good for long-lived clients with unknown load |

**Retry budget** [BEST PRACTICE]: cap retries as a *fraction* of total traffic (e.g. retries ≤ 10% of requests per minute per downstream). Without a budget, a degraded downstream receives 3× load from retries and never recovers — the classic metastable failure. Implement as a shared counter, not per-call max attempts alone.

**Never retry** when: the operation is non-idempotent and has no key; the error is a contract error; the error is a data-quality error (quarantine instead); or the budget is exhausted (fail fast and let reconciliation catch up).

### 6.7 Dead-letter queues: preserve, alert, triage, replay

**What must be preserved per dead message:** the message key, the full payload (or a durable reference), the failure reason from the *last* attempt, the attempt count, the first-seen and last-attempt timestamps, the source system and tenant, and a correlation ID. Without the payload you cannot reproduce; without the tenant you cannot prioritise; without the attempt history you cannot tell "always broken" from "broke once".

**Triage flow:** alert on *any* growth in a compliance-critical queue (not just a threshold); group by error signature so 400 DLQ entries with one cause look like one problem; classify poison (fix code/data) vs capacity (fix the retry budget) vs downstream outage (replay later); fix; replay in a controlled batch with an audit trail; verify both sides afterwards; and feed the top signature into a Product/root-cause action.

**Replay rules:** replay is a *state transition*, not a resend — it must go back through the same idempotent path; it must be rate-limited and auditable; and it must be *safe to run twice* (the lab verifies the second replay requeues 0).

### 6.8 Concurrency: races, leases, and the exactly-once illusion [ASSESSMENT: race conditions]

| Race | Broken code | Fix |
|---|---|---|
| Duplicate intake | `if not exists(id): insert(id)` | `INSERT` with a unique key; catch the duplicate error |
| Job double-claim | `SELECT` then `UPDATE` separately | `UPDATE ... WHERE status='pending'` and check `rowcount`, or `SELECT ... FOR UPDATE SKIP LOCKED` |
| Lost update | `read value; write value+1` | Atomic `UPDATE ... SET v = v + 1` |
| Stale overwrite | `UPDATE ... SET tier=?` unconditionally | `WHERE version = ?`, check `rowcount`, re-read on 0 |
| Double side effect | `send email; mark sent` | Outbox row in the same transaction as the state change; consumer idempotent by message key |
| Check-then-act on the destination | `GET family; GET parts; POST order` | Conditional request (`If-Match`/version) or provider-side idempotency key |
| Cache stampede | All workers miss and recompute | Single-flight/lock per key, or a shorter TTL with jitter |
| Lease expiry mid-work | Two workers on one job | Make the handler idempotent anyway; renew the lease for long jobs; keep leases comfortably longer than p99 processing time |

**On "exactly-once" [ASSESSMENT]:** *"Exactly-once processing is achievable inside a single transactional boundary — a database transaction, or Kafka's transactional producer/consumer. It is not achievable as an end-to-end property across a network, because the acknowledgement itself can be lost; a sender that retries after a lost ack is indistinguishable from a genuine second send. So I design for at-least-once with idempotent effects and reconciliation, and I say 'effectively exactly once at the business level' rather than claiming a guarantee I cannot deliver. If someone insists on exactly-once end to end, I would ask what the actual business requirement is — usually it turns out to be 'do not double-notify the reviewer' or 'do not double-count the vendor', both of which idempotency solves."*

### 6.9 The crash matrix (walk it for any job you design)

Assume a job that: (1) claims work, (2) calls a downstream API, (3) writes state, (4) acks.

| Crash point | State on disk | Duplicate risk | Lost-work risk | Recovery |
|---|---|---|---|---|
| Before claiming | Untouched, claimable | None | None | Restart; the job is claimed |
| After claiming, before the downstream call | Job in `processing` with a lease | None | None if the lease expires and the job returns to `pending` | Lease expiry re-queues |
| During the downstream call (unknown outcome) | Job `processing` | **Yes** — the downstream may have applied it | None | Re-run with the same idempotency key; the downstream dedupes |
| After the downstream call, before the state write | Job `processing` | **Yes** — the effect is not recorded locally | None | Same as above; verify downstream, then write |
| After the state write, before the ack | State applied, job `processing` | Yes on re-run | None | Version guard makes the replay a no-op; **only possible because the state write and the effect are in one transaction** |
| After the ack (job `done`) | Complete | None | None | Nothing to do |

**The lesson to state:** *"Every 'yes' in the duplicate-risk column is why the consumer must be idempotent. Every 'no' in the lost-work column comes from persisting the work before acknowledging it. That is the whole reliability story in two sentences."*

### 6.10 Partial failure: the batch trap

A batch endpoint returning a single 200 does not mean every item succeeded. Three shapes exist in the wild:

1. **Per-item results** (best): `{"results": [{"id": "V1", "status": "created"}, {"id": "V2", "status": "invalid", "errors": [...]}]}` — handle each item; retry only transient items.
2. **All-or-nothing**: one bad item fails the batch. If the batch is large, that is a scaling defect you should raise with the provider; the workaround is to split batches and isolate failures, which costs throughput.
3. **Ambiguous**: 200 with no per-item detail. You must then verify each item by reading it back (a reconciliation pass) — expensive but the only correct option.

**The rule:** *the batch is a transport optimization, not a transaction boundary.* Never mark a batch complete based on the HTTP status alone; count and reconcile the per-item outcomes.

Verified in the lab (Part 7): a batch of 3 returned `updated, updated, conflict`; the conflict was re-planned from fresh data and applied exactly once on retry, while a separate vendor was rejected as `invalid` and quarantined rather than retried.

### 6.11 Common mistakes, and why they fail

| Mistake | Why it fails |
|---|---|
| Retrying a non-idempotent POST without a key | Duplicate vendors, duplicate notifications, duplicate charges |
| Storing idempotency keys only in a cache | Cache eviction or failover loses the guarantee; the DB constraint is the contract |
| Reusing one idempotency key for *different* payloads | Returns another request's cached result — silent corruption. (The lab's mock returns 409 for this) |
| Key scoped globally or per host instead of per tenant | Cross-tenant collisions; one tenant can block another |
| TTL shorter than the provider's retry window | The retry arrives after expiry and creates a duplicate — exactly the case the key existed for |
| Publishing to a queue inside the DB transaction | Either the publish is rolled back (event lost) or you hold a lock across the network |
| Marking a job done before the effect commits | Crash → the effect is lost and never retried |
| Unbounded retries | Poison message burns the worker pool; queue never drains |
| Retry without jitter | All workers retry in the same millisecond, re-triggering the outage |
| Retry budget absent | 3× load on a degraded system prevents recovery |
| DLQ without context (payload, attempt history) | Cannot reproduce, cannot prioritise, cannot fix |
| Replay that bypasses the idempotency path | Double-applies state; the one operation that must never be "special" is replay |
| Time-based (LWW) conflict resolution without ownership policy | Silently discards a legitimate edit |
| Alerting on retry count instead of queue age | You learn about the outage from the customer |

### 6.12 Model answers

**"How do you make an operation idempotent?"**

> "Two layers. First, a natural unique key on the business identity — tenant plus source system plus external ID — because a database constraint is the only thing that survives concurrency, key expiry and client bugs. Second, an idempotency key for the retry window: the client sends a high-entropy key, I insert it into a table keyed by `(tenant, key)` with a hash of the request body, and the business effect plus the stored response commit in the same transaction. A retry with the same key and body gets the original response; the same key with a different body gets a 409, because returning the first response would be silent corruption. And where the effect leaves the database — an email, a partner API — I go through an outbox with a stable dedupe key, and I accept a small duplicate window there because an external system can always fail after accepting my call."

**"Walk me through a crash between two writes."**

> "It depends which two writes, and that's the design question. If state and the event that announces it are two separate commits, a crash between them loses the event permanently — nobody knows it should exist — so I put them in one transaction using a transactional outbox. If the two writes are local state and an external call, I order them so the external call is idempotent, then I write the local state, then I ack: a crash before the ack causes a retry, and the retry is safe; a crash before the state write causes a retry where the external effect may already be applied, which the external system's idempotency key absorbs. What I never do is ack before the effect is durable."

**"The downstream is timing out — is it down?"**

> "Not necessarily, and I treat it as unknown rather than failed. A timeout on a non-idempotent call is the dangerous case: the write may have landed. So I retry with the same idempotency key, and I check my own metrics first — if *all* calls are timing out, it is probably the downstream or the network path; if a subset is timing out, look at payload size and specific tenants, which usually means a slow query or a specific record. Meanwhile I cap concurrency so I do not convert their problem into a retry storm, and I alert on queue age so the customer impact is visible."

### 6.13 Practice

**Practice 6.1** Design the intake for a `vendor.approved` event that must trigger (a) a vendor state change, (b) an email to the requester, (c) an update to an external GRC. Cover all seven reliability questions.

**Your answer:**
>
>
>

<details><summary>Expected points and edge cases</summary>

- **Duplicate request:** event ID unique in the inbox; duplicate returns 200 and does not re-enqueue. Email is *not* sent twice because the outbox row has a dedupe key.
- **Concurrent workers:** the inbox INSERT is the dedupe (no check-then-act); the job claim uses `UPDATE ... WHERE status='pending'` with a rowcount check (or `SKIP LOCKED`).
- **Process crash:** state change + outbox row committed together for (a)+(b). For (c), the same outbox drives an HTTP call that is idempotent at the GRC via a key like `vendor:V001:approved:v7`.
- **Network timeout:** the GRC call is retried with the same key; the email provider is retried with a message key (most providers support this) and otherwise accepted as at-least-once with duplicate suppression at the gateway.
- **Partial failure:** (a) committed, (b) failed → the outbox row for (b) remains pending and retries independently; the vendor state is *not* rolled back. Order matters: never roll back a committed state change because a notification failed.
- **Retry safety:** all three effects are idempotent; emails are the weakest link and are made safe by a stable message key or provider dedupe.
- **Recovery:** DLQ per effect type, an audited replay, and a reconciliation query that finds vendors approved in state with no corresponding notification record.
- Edge cases: the requester's email changes between attempts (resolve the recipient at send time from the vendor record, and record which address was used); the GRC rejects the update with 422 (quarantine, do not retry); the vendor is deleted between (a) and (c) (the GRC call must handle a missing entity).
</details>

**Practice 6.2** Your retry policy sends 5 attempts with 30-second waits. The provider is down for 20 minutes. What happens, and what should you have done?

**Your answer:**
>
>
>

**Practice 6.3** Two workers process the same job because a lease expired. The handler ran twice and sent two emails. List the three design fixes, ordered by how early they would have caught it.

**Your answer:**
>
>
>

**Practice 6.4** Write the SQL for a worker that claims 10 jobs safely with multiple concurrent workers on MySQL 8.

**Your answer:**
> ```sql
>
> ```

<details><summary>Expected answer</summary>

```sql
START TRANSACTION;
SELECT job_id
FROM jobs
WHERE status = 'pending' AND next_attempt_at <= NOW()
ORDER BY job_id
LIMIT 10
FOR UPDATE SKIP LOCKED;          -- other workers skip these rows instead of blocking

UPDATE jobs
SET status = 'processing', attempts = attempts + 1, leased_until = NOW() + INTERVAL 5 MINUTE
WHERE job_id IN (...);           -- the ids just claimed
COMMIT;
```
Why it is correct: `SKIP LOCKED` prevents worker contention; the `WHERE status='pending'` guard inside the UPDATE means a row already taken is not re-claimed even if two transactions raced; `attempts` increments at claim time (so a crash mid-processing still counts the attempt and eventually dead-letters); and `leased_until` supports a reaper query (`status='processing' AND leased_until < NOW()`) to recover from crashed workers.
</details>

### 6.14 Part 6 scorecard

| Item | Correctness /4 | Production readiness /4 | Notes |
|---|---|---|---|
| Seven-question failure walkthrough | | | |
| Idempotency key lifecycle incl. scope, TTL, reuse, concurrency | | | |
| Natural unique key vs idempotency key | | | |
| Transaction boundary discipline (no network calls inside) | | | |
| Outbox from memory (DDL + drain + inbox) | | | |
| Queue guarantees, visibility timeout, partition key ordering | | | |
| Backoff + jitter math and retry budgets | | | |
| DLQ content, triage, replay safety | | | |
| Crash matrix at each step | | | |
| Exactly-once phrasing | | | |
| Partial-failure handling for batches | | | |

---

## Part 7: Synchronization, conflict resolution, rate limits, batching, reconciliation

> **Companion file:** `certa_lab/vendor_sync.py` — a runnable two-system sync with a mock GRC: cursor pagination, a token bucket, real 429s, a lost response, an idempotency-key-reuse guard, a concurrent edit producing a genuine 409, field-ownership conflict resolution, quarantine instead of retry, reconciliation, and a second run that is verified as a no-op.

### 7.1 First principles: a sync is two systems disagreeing about the same noun

**Prompt (as given) [ASSESSMENT topic 3]:** vendor synchronization between Certa and an existing GRC/ServiceNow system.

Before designing anything, the questions that decide the architecture:

| Question | Why it changes the design |
|---|---|
| Which system is the **system of record** for which field? | Determines who wins a conflict; without it you get silent data loss |
| One-way, two-way, or one-way with derived data written back? | Two-way doubles the conflict surface and needs ownership per field |
| Push, pull, or both? | Push gives latency but needs an endpoint and retry logic; pull gives control, replay and reconciliation |
| What is the identity of a vendor? | Name matching is not identity. You need a stable external ID |
| What is the volume and the freshness requirement? | Decides polling interval, batch size, and whether CDC is warranted |
| What happens to deletes and merges? | Deletes are the #1 silent drift source in vendor systems |
| Who is allowed to create a vendor? | Auto-creation from the other side may bypass onboarding and approvals |

**The sentence that frames the whole answer:** *"A two-way sync is a distributed write problem with two systems of record and no shared clock, so the design has to answer three things explicitly: who owns each field, how conflicts are detected, and how drift is found and repaired when everything else fails."*

### 7.2 The artefact that makes or breaks the project: field ownership

Produce this table **with the customer, in the discovery session**. It is one page, and it removes the majority of sync arguments.

| Field | Owner | Direction | Conflict rule | Why |
|---|---|---|---|---|
| `vendor_id` (internal) | Certa | never synced | immutable | Identity must not change |
| `external_vendor_id` | Source of creation | written once | immutable | The join key between systems |
| `legal_name` | Certa | Certa → GRC | Certa wins | Legal name is a compliance record (contracts, DPA) |
| `display_name` | — | newest | LWW | Presentation only, no compliance impact |
| `tier` (risk tier) | Certa | Certa → GRC | Certa wins | A compliance judgement, not a procurement one |
| `data_processing` | Certa | Certa → GRC | Certa wins | Derived from the DPA/assessment |
| `owner` (business owner) | GRC | GRC → Certa | GRC wins | Procurement owns the relationship |
| `status` (lifecycle) | GRC | GRC → Certa | GRC wins | Onboarding/offboarding is a procurement process |
| `cost_center` | GRC | GRC → Certa | GRC wins | Finance data |
| `subprocessors` | Certa | Certa → GRC | Certa wins | Derived from the security review |
| *(anything unlisted)* | — | — | **quarantine for review** | An unlisted field means nobody has decided; do not guess |

**Rule:** if a field has no owner, it has no conflict rule, and it must not be auto-merged. "Nobody owns it, so last-write-wins" is how legitimate edits disappear.

### 7.3 Sync topologies and when each is right

| Topology | Latency | Failure story | Use when |
|---|---|---|---|
| Scheduled pull (delta) | Minutes | Easy: re-run the window; idempotent by design | Default for most enterprise integrations |
| Provider push (webhook) into your intake | Seconds | Needs signature verification, dedupe, DLQ (Part 5) | You need near-real-time and the provider supports it |
| Both (push for latency, pull for reconciliation) | Seconds | Best of both; more code | The professional default for compliance-critical data |
| CDC / log tailing | Sub-second | Strong (replay from the log), but the source's log is an internal contract you may not be allowed to read | High volume inside your own estate |
| Middleware (iPaaS) | Varies | Fast to build, hard to version-control and test, and a third party now owns your correctness | Temporary bridges, never for the system of record |

**Recommendation to state out loud:** *"I default to scheduled delta pull plus reconciliation, add webhooks if the latency requirement demands it, and never let the middleware own the dedupe or the conflict policy."*

### 7.4 Identity and matching: the hard problem hiding in "vendor sync"

You cannot sync two records unless you can decide they are the same record. Vendors are notoriously duplicated: "Acme GmbH", "ACME GmbH", "Acme G.m.b.H.", "Acme (Germany)".

**Hierarchy of identity, best first:**

1. **Shared external ID** — the destination's GUID/UUID stored on our record. Deterministic, cheap, stable. Requires discipline: reserve the ID at creation time, and never let a human re-point it.
2. **Strong business key** — a tax ID / VAT number / DUNS / LEI plus country. Deterministic when present; validate the check digit and normalize formatting (spaces, dashes, leading zeros).
3. **Domain** — the vendor's primary domain from the contract, not from a typo'd email domain. Normalize (`www.`, case, trailing dot).
4. **Normalized name + country** — normalize (casefold, strip legal suffixes like "GmbH/Ltd/Inc/LLC", collapse punctuation and whitespace), then require *both* name and country to match. Fuzzy matching only as a *candidate generator*, never as an auto-merge decision.
5. **Manual review** — for everything else. Always have this path; a queue of 12 uncertain vendors is cheap, a merged vendor with two systems' assessments is a compliance incident.

**Model answer on matching:** *"I would not match on name alone — I have seen three different 'Acme's in one customer's data. My order is: shared external ID first, then a strong business key with check-digit validation, then a normalized name plus country pair. Fuzzy matching produces candidates for a human queue, never automatic merges, because a wrong merge is very expensive to unwind; the assessments and documents are attached to both records. I'd also add a unique constraint on `(tenant, source_system, external_id)` so the database prevents the duplicate in the first place."*

**Deletes, merges, and tombstones** (the most commonly forgotten part):

- A hard delete in the source is **invisible** in an `updated_since` delta — the row simply stops appearing. Without tombstones you will never notice.
- Options: soft deletes (`is_deleted` flag, still emitted in the delta), tombstones (a delete event with a version), or a periodic full-scan reconciliation that computes the set difference (expensive but definitive).
- **Policy question for the customer:** does deleting a vendor in the GRC mean it should be deleted in Certa? Almost never — in a TPRM system, history matters (a vendor that once processed data is still a historical fact). The usual answer is: mark it `offboarded_in_source` and keep the record, with the mapping documented.
- Merges: if the source merges A into B and deletes A, your system must decide whether to re-point references or keep both with a note. Decide *before* it happens; write the decision in the runbook.

### 7.5 Delta sync and the mirror (the subtle part)

**Rule:** *a delta tells you what changed; it does not tell you what exists.* Planning against only the changed rows makes every unchanged, already-synced record look "missing on the destination" — and the sync tries to recreate it. This exact bug appeared in the first draft of the lab program and is now fixed and commented:

```python
def run_sync(client, certa, mirror, *, updated_since, stats, label):
    """The mirror is the key to incremental sync: a delta pull tells you what *changed*,
    not what *exists*. Planning against only the delta rows would make every unchanged,
    already-synced vendor look like 'exists only in Certa' and re-create it."""
    delta = {row["vendor_id"]: row for row in client.iter_vendors(updated_since=updated_since, stats=stats)}
    mirror.update(delta)          # the local cache of the destination's state
    plan = plan_sync(certa, mirror)
    ...
```

**The delta window rules:**

```
watermark = max(updated_at) of everything we have *confirmed* applied
pull      = rows where updated_at > watermark - overlap      (overlap = clock skew + late writes)
dedupe    = by (id, version|hash) on apply
after     = persist the data first, then advance the watermark in the same transaction
deletes   = handled by tombstones, a soft-delete flag, or a periodic full reconcile
```

Three ways a naive watermark silently loses rows (this is Practice 3.3): equal timestamps at the boundary; a writer whose clock lags; and a crash between applying data and advancing the watermark. The overlap window plus idempotent apply fixes the first two; persisting data before the watermark fixes the third.

### 7.6 Conflict resolution strategies, ranked

| Strategy | Correctness | When it is appropriate | Failure mode |
|---|---|---|---|
| **Field ownership** (per-field authority) | High | Default for two-way sync between systems with genuine ownership boundaries | Requires the ownership table; unowned fields have no rule |
| Source-of-record per entity (one side always wins) | High | When one system is unambiguously master for the entity | Loses legitimate edits made by the other side |
| Version / optimistic concurrency | High (as a detector) | Always, alongside another rule — it detects the conflict, it does not resolve it | Treating "higher version wins" as a *policy* rather than a detector |
| Last-write-wins by timestamp | Low | Only for truly low-stakes fields (display name) | Clock skew and equal timestamps silently discard an edit; "newer" ≠ "better" |
| Merge per field | High | Two-way sync where edits are independent | Needs full change tracking to know *which* field changed |
| Manual review queue | Highest | Identity conflicts, unowned fields, equal-version contradictions | Throughput; needs an owner and an SLA |
| Newest-value-by-business-rule (e.g. critical tier always wins) | Medium | Risk-weighted fields | Encodes a policy that must be documented and defensible |

**The one-line statement to use:** *"Version numbers detect conflicts; they do not resolve them. Resolution is a business decision per field, and the ones I cannot resolve automatically go to a review queue rather than being coin-flipped."*

### 7.7 Inbound (pull) planning: the verified algorithm

```python
def plan_sync(certa, grc) -> Plan:
    """
    Per vendor:
      1. present on one side only  -> create (if allowed) or escalate
      2. per field: the owning system wins; LWW only where nobody owns it
      3. equal-version disagreement is undecidable -> escalate, never coin-flip
      4. validate the outgoing payload BEFORE sending it
    """
    plan = Plan()
    for vendor_id in sorted(set(certa) | set(grc)):
        c, g = certa.get(vendor_id), grc.get(vendor_id)

        if c and not g:
            errors = validate_payload(c)
            if errors:
                plan.review.append({"vendor_id": vendor_id, "reason": "validation_failed",
                                    "detail": "; ".join(errors)})
            else:
                plan.create_in_grc.append(vendor_id)
            continue
        if g and not c:
            # Creating a third party is an onboarding decision, not a sync side effect.
            plan.review.append({"vendor_id": vendor_id, "reason": "only_in_grc",
                                "detail": "new third party requires onboarding approval"})
            continue

        patch = {}
        for field_name, owner in FIELD_OWNERSHIP.items():
            if field_name in IDENTITY_KEYS or c.get(field_name) == g.get(field_name):
                continue
            if owner == OWNER_CERTA:
                patch[field_name] = c.get(field_name)                 # push
            elif owner == OWNER_GRC:
                plan.pull_fields.setdefault(vendor_id, {})[field_name] = g.get(field_name)
            else:                                                     # LWW
                if int(c.get("version", 0)) > int(g.get("version", 0)):
                    patch[field_name] = c.get(field_name)
                elif int(g.get("version", 0)) > int(c.get("version", 0)):
                    plan.pull_fields.setdefault(vendor_id, {})[field_name] = g.get(field_name)
                else:
                    plan.review.append({"vendor_id": vendor_id, "reason": "undecidable_lww",
                                        "detail": f"{field_name}: versions equal but values differ"})
        if patch:
            errors = validate_payload({**c, **patch})
            if errors:
                plan.review.append({"vendor_id": vendor_id, "reason": "validation_failed",
                                    "detail": "; ".join(errors)})
            else:
                plan.push_fields[vendor_id] = patch
    return plan
```

**Verified outcome of one run** (mock GRC seeded with realistic disagreements):

| Vendor | Situation | Action taken by policy | Result |
|---|---|---|---|
| V001 | `tier` differs (Certa `high`, GRC `low`) | Certa owns tier → push | GRC updated to `high` |
| V001 | `owner` differs | GRC owns owner → pull | Certa updated from the GRC |
| V002 | `display_name` differs, GRC version newer | LWW → GRC wins | Certa updated |
| V003 | Exists only in Certa | Create in the GRC (valid payload) | Created |
| V004 | Exists only in the GRC | **Escalate** — new third party needs onboarding approval | Review queue |
| V005 | `tier = 'unknown'`, outside the destination enum | **Quarantine locally** — never push invalid data | Review queue, no API call made |
| V006 | A concurrent writer bumped the version between read and write | Push sees 409, re-pull, re-apply policy, retry **once** | Applied exactly once at v14 |

Verified reconciliation afterwards: **4 in sync, 0 unexpected drift**, with `only_in_certa = [V005]` (blocked by validation, by design) and `only_in_grc = [V004]` (awaiting onboarding, by design). The second run: **0 pulled, 0 pushed, 0 drift** — a verified no-op, which is the strongest evidence that a sync is correct.

**Why quarantine beats retry:** a 422 from an enum mismatch never succeeds on retry. Retrying it burns quota, pollutes error metrics, and hides a data-quality defect that a human should fix. Verified in `vendor_sync.py`: the invalid vendor is caught *locally* before the API call, and the destination-422 path exists for anything the local validator misses.

### 7.8 Push mechanics: batching, concurrency, and the version contract

```python
def push(self, items, idempotency_key, stats):
    """Each batch gets a key derived from the run key AND the batch content hash, so
    retrying a batch is safe but two different batches can never share a key."""
    results = []
    for start in range(0, len(items), self.batch_size):
        batch = items[start : start + self.batch_size]
        body = {"items": batch}
        digest = hashlib.sha256(json.dumps(body, sort_keys=True).encode()).hexdigest()[:16]
        key = f"{idempotency_key}-{start // self.batch_size}-{digest}"
        response = self._request("POST", "/grc/vendors:batch", body=body,
                                 headers={"Idempotency-Key": key}, stats=stats)
        results.extend(response.get("results", []))
    return results
```

> **This code is the fixed version of a real bug I introduced while writing the lab.** My first version used one key for the whole run, so the *second* batch received the *first* batch's cached response — the destination never created those records, and reconciliation caught it. The lesson is worth memorising: **an idempotency key is scoped to one logical request, not to a run or a session.** The mock now also refuses to serve a cached response when the same key arrives with a different body (409), which is the guard that turns a silent corruption into a loud error.

**The version contract (state it explicitly):** the client sends *the version it read* (`version: 11`), not a locally incremented number. The destination compares against its own current version:

```python
if incoming_version != current["version"]:
    return {"status": "conflict", "errors": [f"based on v{incoming_version}, current is v{current['version']}"],
            "current_version": current["version"], "current": current}
merged.update({k: v for k, v in item.items() if k in FIELD_OWNERSHIP})
merged["version"] = incoming_version + 1
```

**Verified:** the concurrent edit bumped V006 to v13 after the client read v11, so the push returned `conflict ['based on v11, current is v13']`. The recovery is bounded and specific: refetch → re-apply the policy to fresh data → push once → if it conflicts again, escalate to review (never an unbounded retry loop).

**Batch sizing trade-offs:** bigger batches mean fewer calls and better throughput, but coarser failure granularity and longer transactions on the destination; smaller batches mean more calls against the rate limit and more overhead. Typical enterprise sweet spot is 50–200 records per call, with per-item results. Also **update your mirror from the write acknowledgement** (`current_version`, `updated_at`) rather than assuming what you sent is what now exists — otherwise the next run re-pushes from a stale version and 409s.

### 7.9 Rate limits: respect, pace, and prove it

**The rate-limit contract to confirm with the provider (write it down):** the limit (per second/minute/day), its scope (per tenant, per API key, per endpoint, global), the burst allowance, the exact headers (`X-RateLimit-Limit`, `X-RateLimit-Remaining`, `X-RateLimit-Reset`, `Retry-After`), whether `Retry-After` is seconds or an HTTP date, and the cost of heavy endpoints (a bulk read may count as 10).

```python
class TokenBucket:
    """Client-side pacing so we spend our quota instead of discovering it via 429."""
    def __init__(self, rate_per_second, capacity):
        self.rate, self.capacity, self.tokens, self.updated = rate_per_second, capacity, float(capacity), time.monotonic()

    def acquire(self):
        with self.lock:
            while True:
                now = time.monotonic()
                self.tokens = min(self.capacity, self.tokens + (now - self.updated) * self.rate)
                self.updated = now
                if self.tokens >= 1:
                    self.tokens -= 1
                    return
                time.sleep((1 - self.tokens) / self.rate)
```

```python
if exc.code == 429:
    retry_after = exc.headers.get("Retry-After")
    delay = float(retry_after) if retry_after and retry_after.replace(".", "", 1).isdigit() else 0.5 * 2 ** (attempt - 1)
    # Jitter on top of Retry-After: without it every client retries in the same
    # millisecond and re-triggers the same limit.
    time.sleep(min(delay, 30.0) + 0.05 * attempt)
```

**Verified:** the mock GRC injects a 429 on every fourth GET; the client absorbed them (`http_429s_handled: 1` in run 1, `1` in run 2) and both runs completed. The server-side counter confirms the limits were actually hit (`server_side_rate_limit_hits: 2`) — so this is a real exercise of the path, not a mocked no-op.

| Rate-limit technique | Effect | Cost |
|---|---|---|
| Token bucket / leaky bucket | Smooth pacing; prevents most 429s | Adds latency; needs tuning to the real limit |
| Worker concurrency cap | Bounds in-flight requests | Throughput ceiling |
| Adaptive backoff (halve concurrency on 429, ramp slowly) | Recovers automatically from a limit change | More complex; slower to ramp |
| Request budget per tenant | Prevents one tenant starving others | Needs per-tenant accounting |
| Prioritisation (hotfixes before bulk backfill) | Keeps the critical path responsive during a backfill | Requires a priority model |
| Read caching / ETag (`If-None-Match`) | Less traffic, fewer 304s counted against some limits | Cache invalidation |
| Off-peak scheduling for backfills | Avoids competing with business traffic | Freshness delay |

**Also mention the 429 that is *your* side:** when a provider calls *you* too fast, return 429 with `Retry-After` — but only after you have persisted or explicitly refused the work. Never 429 after partially processing.

### 7.10 Reconciliation: how you prove the sync is correct

**First principles:** every sync will drift — hard deletes, a missed webhook during an outage, a manual edit in a UI you did not know existed, a version conflict escalated to a human who never answered. Reconciliation is the *detector*, and it is the difference between "we think it's in sync" and "we can prove it".

**Design:**

| Element | Choice | Reasoning |
|---|---|---|
| Frequency | Nightly full + hourly spot checks | Full scans are expensive; spots catch regressions sooner |
| Scope | Per tenant, partitioned by vendor ID ranges | Bounded memory and resumable |
| Comparison | Field-level, ownership-aware | A difference in a GRC-owned field is *expected*, not drift |
| Output | Counts by category + sampled records + a machine-readable artifact | A report that cannot be diffed over time is useless |
| Actions | Auto-repair only for owned-field drift; everything else queues for review | Never let reconciliation make an unresolvable decision |
| Metric | `unexplained_drift_count` with an SLO (e.g. 0 for 24 h) | This is the number that tells the customer the integration works |
| Retention | Keep the last N reports | Trend analysis ("drift started on the 14th") is how you find the cause |

**A reconciliation report from the lab (real output):**

```
in sync                  : 4
unexpected drift         : 0
only in Certa            : ['V005']  (blocked by validation/review)
only in GRC              : ['V004']  (awaiting onboarding approval)
review queue:
  V004: only_in_grc -- new third party requires onboarding approval
  V005: validation_failed -- tier 'unknown' is outside the destination enum
```

Note the honesty of this report: it does not claim "everything is fine". It says exactly which records are not synced **and why**. Every unresolved record has a named reason and an owner-able next action. That is what makes a reconciliation report usable in a customer review meeting.

**The SQL for it** (Part 3.9): a presence diff in both directions with `UNION ALL` and two anti-joins, plus a field-level drift query using NULL-safe comparison — with the `COALESCE(x,'') <> COALESCE(y,'')` pattern that naive comparisons get wrong.

### 7.11 Rollout: how a sync goes live without a big-bang risk

| Phase | What runs | Exit criteria |
|---|---|---|
| 1. Read-only | Pull both sides, compute the diff, write nothing | The diff is explainable; conflicts are known and few |
| 2. Dry-run plan | Produce the full action plan + report; still write nothing | The customer reviews the plan and agrees the rules |
| 3. Shadow writes | Write to a staging/pilot vendor set only | No unexpected drift for a week; DLQ empty; error budget respected |
| 4. Limited production | One tenant / one vendor category, with a kill switch | Metrics stable, review queue trending to zero |
| 5. Full production | All vendors, reconciliation nightly | `unexplained_drift_count = 0` sustained |
| 6. Backfill | Historical records in batches, off-peak, rate-limit aware | Backfill complete and reconciled; no impact on business traffic |

**The three questions the customer will ask, and the answers:**

1. *"What if it goes wrong?"* → A kill switch that stops writes while intake continues; a documented rollback (which is usually "stop writing, keep reading, fix forward", not "undo 4,000 updates"), and a reconciliation report to prove the end state.
2. *"How do we know it worked?"* → The reconciliation report: in-sync count, drift count, unresolved items with reasons, and the trend over the last 7 days.
3. *"Can you guarantee no duplicates?"* → "No — I can guarantee the duplicate is harmless and measurable: a unique key on the external ID, an idempotency key per push, and a nightly check that reports any vendor existing twice. I'd rather give you a number you can verify than a guarantee I can't prove."

### 7.12 Common mistakes, and why they fail

| Mistake | Why it fails |
|---|---|
| Syncing every field bidirectionally | Every field becomes a conflict; you lose the compliance record to a procurement edit |
| No external ID; matching on name | Duplicates multiply; the sync creates a second "Acme GmbH" |
| Auto-creating a third party from the other system | Bypasses onboarding and approvals — a governance failure, not just a data issue |
| Planning against the delta only (no mirror) | Unchanged records look missing and get re-created |
| Watermark advanced before the data is durable | A crash replays the window twice (harmless) or skips it (harmful) — order the writes |
| Timestamp-only ordering | Equal timestamps and clock skew silently drop edits |
| Blind LWW | Discards legitimate edits; the losing system's user is told their change saved |
| Retrying a 422/validation error | Burns quota and hides the defect; quarantine and report instead |
| Retrying a 409 without re-reading | The retry conflicts again; always re-read and re-apply the policy first |
| One idempotency key for a whole run | Different batches collide and receive each other's cached responses |
| Trusting a batch 200 as per-item success | Partial failures disappear |
| Not updating the mirror from write acknowledgements | The next run pushes from a stale version and 409s |
| No reconciliation | Drift is discovered by the customer, during an audit |
| Hard deletes propagated automatically | Destroys history in a compliance system; needs an explicit policy |
| Backfills run in business hours at full speed | Rate-limits the production path and delays the customer's live work |

### 7.13 Model answer (system design structure, ~2 minutes)

> **Requirements.** Two-way vendor sync between Certa and the customer's GRC. Fields are owned per side; the customer needs vendor identity to be stable, a nightly provable reconciliation, and no silent data loss. Freshness target for the main path is 15 minutes, and the volume is tens of thousands of vendors, not millions.
> **Assumptions.** The GRC exposes REST with an `updated_since` delta and a batch upsert with idempotency support; it has a version column; it rate-limits per tenant; and it owns the procurement lifecycle status. I'd verify each of those against the real sandbox before committing to a design.
> **Architecture.** Scheduled pull into a local mirror of the GRC, planning performed against the mirror; pushes batched through a client with a token bucket; webhooks added later if the latency requirement tightens; a nightly full reconciliation per tenant.
> **Data model.** A `vendor_link` table holding `(tenant, certa_vendor_id, external_id, source_system, last_seen_version, last_synced_hash)`, plus an outbox for pushes and an inbox for inbound events. The unique key on `(tenant, source_system, external_id)` makes duplicate creation impossible.
> **Normal flow.** Delta pull → mirror update → plan against the mirror → apply the pull side locally → batch push the push side with per-batch idempotency keys → update the mirror from the acknowledgements → advance the watermark → emit metrics.
> **Failure scenarios.** Duplicate push: idempotency key + the natural unique key. Concurrent edit: version mismatch → 409 → re-read, re-plan, retry once, else escalate. Crash mid-run: the watermark only advances after the data is durable, so the worst case is a repeated (idempotent) window. Network timeout on a push: unknown outcome, retried with the same key. Partial batch failure: per-item handling, invalid items quarantined, transient items retried with backoff. Missed webhook: reconciliation detects it within 24 hours.
> **Security.** Per-tenant credentials in a secret manager, least privilege, no PII in logs, field-level authorisation, audit every write with the actor, and outbound TLS with certificate validation.
> **Observability.** Records pushed/pulled, conflict rate, quarantine count, retry rate, 429 count, watermark lag, reconciliation drift count, and DLQ size — with alerts on drift and on watermark lag, not on individual HTTP errors.
> **Testing.** Contract tests against recorded real payloads; a conflict test for every rule in the ownership table; an "invalid enum" test proving quarantine; a duplicate test proving the unique key; a replay test; and reconciliation asserted to be zero after a full run.
> **Rollout.** Read-only diff, then dry-run plans reviewed with the customer, then shadow writes on a pilot set, then one tenant live with a kill switch, then all tenants, then the historical backfill off-peak.
> **Trade-offs.** Pull-only is simpler but has latency; adding webhooks buys seconds and costs an intake service with dedupe, signatures and a DLQ. Field-ownership resolution is more code than LWW but it is the only version that does not silently discard edits. Keeping a mirror costs storage and a schema but makes incremental sync correct.

### 7.14 Practice

**Practice 7.1 [ASSESSMENT]** Design the Certa ↔ ServiceNow/GRC vendor sync. Include the ownership table, conflict rules, and the reconciliation plan.

**Your answer:**
>
>
>

**Practice 7.2** The customer's GRC team edits `tier` on their side, arguing that procurement should decide risk tiers. How do you handle it — technically and socially?

**Your answer:**
>
>
>

<details><summary>Expected points</summary>

Technically: ownership is a policy input, so the change is one line in the ownership table plus a one-time reconciliation of the existing disagreements (and a decision about which side wins for each already-divergent vendor — that is a data-governance decision, not a technical one). Socially: do not argue about turf in the room; reframe to the risk. "If procurement sets the tier and the assessment says otherwise, which number does the auditor see? Let's agree that Certa holds the assessed tier and the GRC holds a `procurement_rating` field with a different name, so both parties keep their number and the audit trail stays clean." That is the FDE move: change the data model so the conflict disappears, instead of winning the argument.
</details>

**Practice 7.3** A vendor is deleted in the GRC. What should happen in Certa? Give three defensible options and the information you would need to choose.

**Your answer:**
>
>
>

**Practice 7.4** A sync runs every 15 minutes. The customer asks for real-time. What changes, and what does the change cost?

**Your answer:**
>
>
>

**Practice 7.5 (coding)** Using `certa_lab/vendor_sync.py` as a base, add a third field owner: a `procurement_rating` field owned by the GRC, and prove with the reconciliation report that a divergence in it is auto-repaired rather than reported as drift.

### 7.15 Part 7 scorecard

| Item | Correctness /4 | Production readiness /4 | Notes |
|---|---|---|---|
| Field-ownership table produced unprompted | | | |
| Identity strategy (external ID, then business key) | | | |
| Delta + mirror correctness | | | |
| Watermark rules and the three failure modes | | | |
| Conflict detection vs resolution distinction | | | |
| 409 handling (re-read, re-plan, retry once, escalate) | | | |
| Idempotency key scoping per batch | | | |
| Rate limiting: bucket, `Retry-After`, jitter, budgets | | | |
| Reconciliation design with metrics and categories | | | |
| Rollout phases incl. dry-run and kill switch | | | |
| Deletes/tombstones addressed | | | |

---

