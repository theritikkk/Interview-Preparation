# Consistency Models

---

## Cluster 1: Strong vs. Eventual Consistency

### What is it?
**Strong consistency**: any read after a write is guaranteed to see that write (or a later one) — simple to reason about, but requires coordination (and thus latency/availability cost) across replicas. **Eventual consistency**: replicas converge to the same value *eventually*, but a read shortly after a write might see a stale value — lower latency, higher availability, harder application-level reasoning.

### Interview Questions
- **L1:** "Strong consistency guarantees reads see the latest write immediately, at the cost of coordination overhead; eventual consistency allows brief staleness in exchange for lower latency and higher availability."
- **L2:** connect directly to your resume: DynamoDB offers both per-request (eventually consistent reads are the default and cheaper; strongly consistent reads cost more and aren't available across regions) — explain why Hermes's read models, built asynchronously off DynamoDB Streams, are *inherently* eventually consistent relative to the event store no matter what consistency setting you use for the store itself, because the read-model-update pipeline (Streams → processing → read model write) takes some non-zero time.
- **L3:** explain the practical implication for a client: if a user appends an event and immediately queries a read model expecting to see it reflected, that's a genuine race the architecture has to account for (e.g., returning the just-appended event directly from the write path's response, rather than requiring a re-read of a read model that may not have caught up yet) — a strong, specific answer to "how do you handle read-your-writes in an eventually consistent read model?"

---

## Cluster 2: Quorum Reads/Writes

### What is it?
In a replicated system with N replicas, a **write quorum (W)** and **read quorum (R)** define how many replicas must acknowledge a write/participate in a read; if `W + R > N`, every read is guaranteed to overlap with the most recent write's replica set, giving strong consistency without requiring *all* N replicas to participate in every operation.

### Interview Questions
- **L1:** "Quorum-based systems require W replicas to ack a write and R replicas to respond to a read; if W+R>N, a read is mathematically guaranteed to see the latest write, balancing consistency against full-replica-set coordination cost."
- **L2:** explain the tradeoff space this opens: `W=N, R=1` (fast reads, slow/fragile writes — any replica down blocks writes), `W=1, R=N` (fast writes, slow reads), `W=R=majority` (balanced) — and that this quorum math is conceptually what's underneath systems like Cassandra/DynamoDB's tunable consistency, even if DynamoDB's actual consistency setting is a simpler "eventual vs. strong" toggle at the API level rather than exposing raw W/R numbers to you directly.

### Quick Revision
- Strong consistency: coordination cost, no staleness. Eventual: lower cost, staleness window.
- Hermes's CQRS read models are eventually consistent by construction, regardless of the event store's own consistency setting — the pipeline itself introduces lag.
- Quorum: W+R>N guarantees a read overlaps the latest write's replica set — the theory underneath tunable-consistency systems.
