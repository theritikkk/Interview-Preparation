# Messaging & Event-Driven Communication

🔴🔴 The architectural core of the Distributed Task Scheduler (RabbitMQ) and a key piece of Hermes's Outbox chain (SQS/EventBridge, covered in `08_AWS/`). This file goes deep on RabbitMQ specifically, per the "no shallow buzzwords" mandate.

---

## Cluster 1: RabbitMQ Internals — Exchanges, Queues, Bindings

### What is it?
A **producer** doesn't publish directly to a queue — it publishes to an **exchange**, which routes the message to zero or more **queues** based on **bindings** (rules connecting an exchange to a queue, often via a routing key). This indirection is what lets you change routing (add a new consumer queue) without touching producer code.

### How does it work?
Exchange types: **direct** (exact routing-key match), **topic** (pattern match, e.g., `task.*.failed`), **fanout** (broadcast to all bound queues, ignoring routing key), **headers** (match on message headers instead of routing key).

### Interview Questions
- **L1:** "Producers publish to an exchange, not directly to a queue; bindings route messages from the exchange to one or more queues based on the exchange type and routing key."
- **L2:** explain which exchange type your Task Scheduler most likely uses for dispatching a task to the right worker queue, and why (a direct or topic exchange, routing by task type/priority, is the natural fit for "route this task to the queue that should process it").

---

## Cluster 2: TTL + Dead-Letter Exchange for Delayed Dispatch 🔴🔴

### What is it?
RabbitMQ doesn't have native "deliver this message at time X" scheduling — the standard workaround is: publish the message to a queue with a **per-message or per-queue TTL** (time-to-live) equal to the desired delay, but never consume from that queue directly; instead, configure that queue's **dead-letter exchange (DLX)** to be the *real* target exchange. When the TTL expires, RabbitMQ automatically "dead-letters" (moves) the message to the DLX, which routes it to the actual dispatch queue — the TTL expiry becomes your delay trigger.

### How does it work?
```
Producer → [delay-queue, TTL=N, DLX=dispatch-exchange] 
    → (TTL expires, no consumer ever reads this queue directly)
    → RabbitMQ moves message to dispatch-exchange → [dispatch-queue] → Worker consumes
```

### Interview Questions
- **L1:** "A message is published to a queue with a TTL and a dead-letter-exchange configured; when the TTL expires, RabbitMQ moves it to the DLX, which routes it to the real dispatch queue — this creates delayed delivery without polling anything."
- **L2 — the "why not just poll the database" question, directly from your resume:** explain precisely why hot-polling PostgreSQL for due tasks doesn't scale: every polling interval means a query hitting the database regardless of whether any tasks are actually due, wasting database load at scale and adding latency up to one full polling interval before a due task is even noticed. TTL-based dispatch instead makes RabbitMQ itself the scheduler — the message becomes "ready" and is delivered to a consumer at exactly the right moment, event-driven rather than poll-driven.
- **L3:** be precise about a real limitation of this pattern worth knowing: **per-queue TTL only dead-letters messages in FIFO order relative to their expiry within that queue** — if you need many different delays in the same queue, a later-published message with a *shorter* TTL will sit behind an earlier message with a *longer* TTL until the earlier one expires (RabbitMQ only checks the head of the queue for TTL expiry, an implementation detail worth knowing if asked how you'd support many different arbitrary delays cleanly — often solved with per-message TTL and a queue per distinct delay bucket, or a completely different scheduler-plugin approach).

### Follow-up Chain
> **Q: What happens if RabbitMQ itself goes down?**
> A: `NEEDS VERIFICATION` — check your actual setup, but the honest architectural answer: without clustering/mirrored queues, RabbitMQ is a single point of failure for dispatch — in-flight and queued (but not yet consumed) messages could be lost or unavailable until it recovers, unless durable queues + persistent messages + a mirrored/quorum queue setup across multiple nodes is in place.
> **Follow-up: How would you make it more resilient?**
> A: RabbitMQ quorum queues (Raft-based replication across a cluster) for durability, plus durable queues/persistent messages so a broker restart doesn't lose unacknowledged messages, plus the PostgreSQL source-of-truth (your resume's own row-count-convergence design) acting as a recovery/reconciliation mechanism independent of the broker.
> **Trap: "Durable queues mean no message loss."**
> Correct response: durable queues persist to disk, surviving a broker *restart* — they don't protect against message loss during the window between "producer publishes" and "broker acknowledges/persists the write," nor do they help if the entire single-node broker's disk is lost with no replication. Precision here (what durability actually covers) is exactly the kind of distinction interviewers probe for.

---

## Cluster 3: RabbitMQ vs. Kafka (Comparison, Not On Your Resume — Prepare for "Why Not Kafka?")

| | RabbitMQ | Kafka |
|---|---|---|
| Model | Smart broker, dumb consumer (broker tracks delivery/acks) | Dumb broker, smart consumer (consumer tracks its own offset) |
| Message retention | Deleted after ack (by default) | Retained for a configured period regardless of consumption — consumers can replay |
| Ordering | Per-queue | Per-partition (ordering is not global across a topic) |
| Best fit | Complex routing, per-message TTL/delay patterns, task/work queues | High-throughput log/event streaming, multiple independent consumer groups replaying the same stream |

### Interview Questions
- **L2 — the certain follow-up:** "Why RabbitMQ, not Kafka, for the Task Scheduler?" A strong answer: the core mechanism you needed — per-message TTL + dead-letter-exchange for delay-based dispatch — is a RabbitMQ-native feature with no equally direct Kafka equivalent (Kafka has no built-in delayed-delivery primitive; you'd have to build a delay mechanism yourself, e.g., a separate "sleeping" topic + a poller, ironically reintroducing the polling problem RabbitMQ's TTL avoids natively). RabbitMQ's routing flexibility (exchanges/bindings) also maps naturally onto "dispatch this task to the right worker type," where Kafka's per-partition ordering and replay-focused design is aimed at a different problem (event streaming/log processing) than task dispatch.

### Quick Revision
- Producers publish to exchanges, not queues directly; bindings route to queues.
- TTL + DLX = RabbitMQ's native delayed-delivery mechanism — avoids polling entirely.
- Per-queue TTL only expires in FIFO order at the queue head — a real limitation for mixed arbitrary delays.
- RabbitMQ (at-least-once, broker tracks acks) vs. Kafka (retained log, consumer tracks offset, replay-friendly) solve different problems — know why RabbitMQ fit the Task Scheduler's specific delay-dispatch need.
