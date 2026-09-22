# Distributed Systems — Fundamentals

The theory underneath both Hermes and the Task Scheduler.

---

## Cluster 1: CAP Theorem

### What is it?
Under a network **P**artition, a distributed system must choose between **C**onsistency (every read sees the latest write) and **A**vailability (every request gets a response) — you cannot have all three of C, A, and P simultaneously when a partition actually occurs. In practice, partitions *will* happen, so the real design question is "CP or AP during a partition," not "which of the three do I pick."

### Interview Questions
- **L1:** "During a network partition, a distributed system trades off consistency against availability — DynamoDB defaults to AP-leaning (eventually consistent reads by default, though strongly consistent reads are available on request); a single-primary PostgreSQL setup leans CP (unavailable for writes if the primary is unreachable, rather than risk inconsistency)."
- **L2:** explain why CAP is often misunderstood as "pick 2 of 3" — partition tolerance isn't really optional for any real distributed system (partitions happen whether you plan for them or not); the actual choice is what happens *during* a partition, between the other two.
- **L3:** connect to your own projects: Hermes's DynamoDB-backed event store defaults to eventual consistency for reads (AP-leaning) unless you explicitly request a strongly consistent read — a deliberate tradeoff for availability/throughput that your CQRS read models must be designed to tolerate (read models built async off Streams are, by construction, eventually consistent relative to the event store).

### DO NOT SAY THIS
"My system is CP and AP." — Not simultaneously, during an actual partition — this is exactly the "pick 2 of 3" misconception; be precise about which one your system favors when a partition occurs, and why that's the right tradeoff for the specific use case.

---

## Cluster 2: Idempotency (Distributed Systems Framing)

### What is it?
The property that processing the same operation multiple times produces the same result as processing it once — the load-bearing concept that makes "at-least-once delivery + safe retries" add up to a correct system overall, covered concretely in `04_BACKEND/REST.md` (idempotency keys) and `08_AWS/SQS.md` (redelivery).

### Interview Questions
- **L1:** "Idempotency means repeating an operation doesn't change the outcome beyond the first application — essential in distributed systems because network failures make 'exactly-once' delivery alone essentially unachievable; idempotent processing is what makes at-least-once delivery safe."
- **L2:** state the general principle that both your resume projects rely on: neither RabbitMQ (Task Scheduler) nor SQS (Hermes) natively guarantees exactly-once delivery — both guarantee (at best) at-least-once; the *system's* end-to-end correctness comes from idempotent processing logic layered on top, not from the broker itself.

---

## Cluster 3: Fallacies of Distributed Computing (Quick-Fire)

A classic list worth having ready, because interviewers use it to probe whether you think about distributed systems realistically: the network is NOT reliable; latency is NOT zero; bandwidth is NOT infinite; the network is NOT secure; topology doesn't stay fixed; there isn't one administrator; transport cost isn't zero; the network isn't homogeneous.

### Interview Questions
- **L2:** pick one and connect it to a project decision — e.g., "the network is not reliable" is directly why the Task Scheduler needs retries/DLQ at all, and why Hermes's Transactional Outbox exists specifically to avoid a dual-write across two systems where the second write's network call might simply fail.

### Quick Revision
- CAP: partition tolerance is a given in practice — the real tradeoff is C vs. A during a partition.
- At-least-once delivery (the real guarantee both your message systems provide) + idempotent processing = the correct end-to-end system, not "exactly-once delivery" as a broker property.
- "The network is not reliable" is the single fallacy that justifies the existence of retries, DLQs, and the Outbox pattern across your projects.
