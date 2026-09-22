# Project Deep Dive: Distributed Task Scheduler — Cron-as-a-Service

🔴🔴🔴 Same priority tier as Hermes. This project's defining rigor challenge is precision about what "exactly-once" actually means — get this right and you turn your resume's most challengeable claim into your strongest answer.

---

## Architecture (Reconstructed From Resume Claims)

```
Client → API Gateway → PostgreSQL (source of truth: task definitions, status)
                              │
                              ▼
                        Coordinator (finds due tasks, dispatches)
                              │
                              ▼
                RabbitMQ (delay-queue, TTL, DLX → dispatch-exchange)   [MESSAGING.md]
                              │
                              ▼
                   Worker Fleet (3 replicas, independently metriced)
                              │
                              ▼
                  PostgreSQL (status update, claimed via FOR UPDATE SKIP LOCKED)

Observability: Prometheus + Grafana + Loki + Alertmanager, scraping API/coordinator/all 3 workers
Load test: 10,000 tasks → 8,562 peak queue depth, 1,600 msg/s burst, drained ~7m46s, 0 DLQ, 0 loss
```

---

## Component-by-Component Justification

| Component | Why it exists | Why chosen (vs. alternative) | Failure mode | Scaling | Consistency implication |
|---|---|---|---|---|---|
| **API Gateway** | Client-facing task creation/query | Standard entry-point pattern | Overload if unrate-limited (`12_SECURITY/CLOUD_SECURITY.md`) | Horizontally scalable, stateless | N/A |
| **PostgreSQL (source of truth)** | Durable task state, queryable | ACID transactions + strong consistency for task state, vs. eventual-consistency stores — matters because you need to know definitively whether a task is done | Connection exhaustion under high concurrent load (`06_DATABASES/POSTGRESQL.md`) — a plausible real bottleneck in your 10,000-task test | Read replicas possible for query load; writes bound by single-primary throughput | Strong (CP-leaning) — a deliberate choice, contrasted with Hermes's AP-leaning DynamoDB |
| **Coordinator** | Finds due tasks, initiates dispatch | Centralizes the "what's due now" decision | **Honest weakness to name proactively:** likely a single point of failure unless clustered/leader-elected — `NEEDS VERIFICATION` | Can shard by task-ID hash across multiple coordinator instances (see `11_SYSTEM_DESIGN/COMMON_DESIGNS.md` Design 3) | N/A |
| **RabbitMQ (TTL + DLX)** | Event-driven delayed dispatch, avoiding hot-polling | See `MESSAGING.md` Cluster 2 in full — this is the project's core architectural decision | Broker down = dispatch stalls unless clustered with quorum queues (`MESSAGING.md` follow-up chain) | Vertical (bigger broker) or clustered (quorum queues) | At-least-once delivery — NOT exactly-once at the broker level (see Claim Defense below) |
| **Worker Fleet (3 replicas)** | Executes dispatched tasks | Separate processes for fault isolation (`03_CS_FUNDAMENTALS/OS.md` Cluster 1) | One worker crashing shouldn't take down others — verify this is actually true of your process model | Horizontal — add more worker processes/replicas | Idempotent claiming (`FOR UPDATE SKIP LOCKED`) required for correctness under concurrent workers |
| **Prometheus/Grafana/Loki/Alertmanager** | Full observability across API/coordinator/3 workers | Self-hosted stack, since this project isn't AWS-native (contrast with Hermes's CloudWatch-by-default + X-Ray) | Silent failure if scrape targets aren't correctly configured per-replica | Scales with the number of scrape targets, generally not a bottleneck at this scale | N/A |

---

## The Named Questions From Your Own Prep Brief — Direct Answers

**"Why RabbitMQ?"** → Native TTL + dead-letter-exchange gives you event-driven delayed dispatch with no polling — see `MESSAGING.md` Cluster 2.

**"Why TTL?"** → It's the mechanism that turns "deliver this later" into "let the broker expire it at the right moment, then route it to the real queue" — no cron/poller needed to check "is anything due yet."

**"Why not polling?"** → Every polling interval means a database query regardless of whether anything is actually due — wasted load at scale, plus up to one full interval of latency before a due task is noticed; TTL-based dispatch is event-driven and exact.

**"How does delayed scheduling work?"** → Message published to a delay-queue with TTL = desired delay; on expiry, RabbitMQ dead-letters it to the real dispatch exchange — see `MESSAGING.md` for the exact mechanism and its known limitation (per-queue TTL only expires in FIFO order at the queue head).

**"What happens if a worker dies?"** → Depends on the ack model: if the message wasn't acked before the crash, RabbitMQ redelivers it (to another worker) once the connection/consumer is detected as gone — the task isn't lost, but this again requires the eventual processing to be idempotent (or your PostgreSQL claiming mechanism to prevent true double-execution — be precise about which mechanism is doing the actual work here).

**"What happens if RabbitMQ goes down?"** → `NEEDS VERIFICATION` — honestly: without clustering/mirrored queues, it's a single point of failure for dispatch; in-flight unacked messages could be lost on a hard crash without durable queues + persistent messages. See `MESSAGING.md`'s follow-up chain for the full precise answer and its resilience mitigations.

**"How are duplicate tasks handled?"** → `NEEDS VERIFICATION` on the exact mechanism — the expected/standard answer is a claiming pattern (`FOR UPDATE SKIP LOCKED` or a `claimed_by`/`claimed_at` compare-and-set) at the PostgreSQL layer, so even if RabbitMQ redelivers a message to two workers, only one successfully claims the underlying task row.

**"How do you avoid task loss?"** → Durable queues + persistent messages (survive broker restart) + PostgreSQL as the ultimate source of truth (a task's status is what PostgreSQL says it is, not just "was a message consumed") — the row-count convergence check your resume describes is precisely this cross-check.

**"How do you scale workers?"** → Add more worker processes/replicas consuming from the same dispatch queue — RabbitMQ distributes messages across all connected consumers on a queue automatically (round-robin by default).

**"What is the bottleneck?"** `NEEDS VERIFICATION` — candidates: PostgreSQL connection limits under high concurrent claiming (`06_DATABASES/POSTGRESQL.md`), or the coordinator's own throughput if it's a single unsharded process. Know which one your load test actually revealed, if either — a specific, evidence-based answer beats a generic one.

**"How does PostgreSQL behave under concurrent scheduling?"** → MVCC means readers don't block writers, but concurrent `UPDATE`s to the same row (or lock contention from `FOR UPDATE`) do need `SKIP LOCKED` to avoid workers queueing up behind each other — see `06_DATABASES/POSTGRESQL.md` Cluster 1.

**"How do retries work?"** `NEEDS VERIFICATION` — the shape of a good answer: failed task execution triggers a requeue (possibly with a backoff delay, itself implementable via the same TTL mechanism) up to a max-retry count, after which it's dead-lettered to a DLQ for manual inspection.

**"How does the DLQ work?"** → Messages that fail delivery/processing repeatedly (exceeding a max-retry threshold) land in a dead-letter queue instead of being silently dropped or retried forever — your load test's "zero DLQ messages" result is evidence that no task exceeded that threshold during the test, not that the DLQ mechanism was never exercised at all.

**"What does 'exactly once' mean? Is it really guaranteed?" 🔴 — the most important question in this entire file.**
The precise, defensible answer: RabbitMQ itself guarantees **at-least-once delivery** (with acks) — it does not, and cannot alone, guarantee exactly-once delivery, because a network partition between a worker acking and RabbitMQ receiving that ack is fundamentally indistinguishable from the worker having crashed before processing, so RabbitMQ's only safe default is to redeliver. **What your system actually achieves is an exactly-once *effect*** — the end state (PostgreSQL row counts converging 1:1, zero orphaned `scheduled` rows) is correct — and that correctness comes from the *combination* of at-least-once delivery **plus** idempotent/claim-guarded processing at the PostgreSQL layer, not from the broker's delivery guarantee alone. State it in exactly this order if asked: broker guarantee (at-least-once) → why that alone isn't enough → what closes the gap (idempotent claiming) → the observed effect (row-count convergence) as evidence the combination worked. This full reasoning also lives in `01_PROGRAMMING/SQL.md`'s follow-up chain and `07_DISTRIBUTED_SYSTEMS/FUNDAMENTALS.md` Cluster 2.

**"How did the load test work?"** `NEEDS VERIFICATION` — confirm the actual tool (k6? a custom script?) before describing specific tooling, per `10_OBSERVABILITY/K6.md`'s exposure-risk note.

**"What does 8,562 peak queue depth mean?"** → At the busiest single moment, 8,562 messages were sitting in the dispatch queue waiting for a worker to consume them — evidence of a real burst the system absorbed (queued, not dropped) rather than rejecting or crashing under.

**"What does 1,600 msg/s burst mean?"** → The peak throughput rate at which messages were being dead-lettered from the delay-queue into the dispatch path at once — since all 10,000 tasks were presumably scheduled close together, their TTLs expired in a tight cluster, producing this burst.

**"Why did the system drain in ~7m46s?"** → The time from peak queue depth back to zero reflects the sustained consumption rate of the worker fleet — `NEEDS VERIFICATION`: you should be able to back-calculate an approximate sustained messages/sec processing rate from 8,562 messages ÷ ~466 seconds ≈ ~18 msg/s sustained drain rate, and have a view on whether that's worker-CPU-bound, PostgreSQL-write-bound, or RabbitMQ-consumer-count-bound.

**"How would you improve it?"** → Honest, specific answers score far higher than vague ones: more worker replicas (if drain rate was the bottleneck), a connection pool tuning pass (if PostgreSQL connections were the limit), or clustering RabbitMQ (if broker durability/single-point-of-failure was the concern raised earlier in the interview).

### "Why not Kafka?" (comparison question, not on your resume)
Same shape of answer as Hermes's version: your core need — native delayed/TTL-based dispatch with flexible exchange-based routing — is a RabbitMQ-native feature with no direct Kafka equivalent; Kafka's strengths (high-throughput log retention, replay, consumer-group-managed offsets) target a different problem (event streaming) than task dispatch. See `MESSAGING.md` Cluster 3 for the full comparison table.

### Quick Revision
- The "exactly-once" question is THE question for this project — always answer with: broker guarantee (at-least-once) → the gap → idempotent claiming closes it → row-count convergence is the *evidence*, not the *mechanism*.
- Name the coordinator's likely single-point-of-failure status proactively — a strong, honest answer beats being caught not having considered it.
- Back-calculate the ~18 msg/s sustained drain rate from your own numbers before the interview — a specific number beats "it drained quickly."
- `FOR UPDATE SKIP LOCKED` (or equivalent) is the actual mechanism preventing duplicate task execution — RabbitMQ redelivery alone would cause duplicates without it.
