# DynamoDB (AWS Operational View)

🔴 Core data store of Hermes. This file covers AWS-operational specifics; single-table design and denormalization-as-CQRS is covered in `06_DATABASES/DYNAMODB.md`.

---

## Cluster 1: Keys, Capacity Modes, Indexes

### What is it?
Every item is located by its **partition key** (determines which physical partition stores it — good distribution here is critical) plus an optional **sort key** (orders items within a partition, enabling range queries within one partition key). **On-demand** capacity mode scales automatically and bills per request; **provisioned** capacity mode requires specifying read/write capacity units (RCU/WCU) ahead of time, cheaper at steady, predictable load but requires capacity planning.

### How does it work?
A **Global Secondary Index (GSI)** lets you query by an *different* attribute than the base table's partition key, at the cost of eventual consistency (by default) and its own separate capacity. A **Local Secondary Index (LSI)** shares the base table's partition key but allows a different sort key — must be created at table-creation time, unlike a GSI.

### Interview Questions
- **L1:** "Partition key determines physical distribution and lookup; sort key orders items within a partition; on-demand mode auto-scales and bills per-request, provisioned mode requires pre-specifying capacity."
- **L2:** explain why a poorly chosen partition key (e.g., a low-cardinality value, or one with highly skewed access) creates a "hot partition" — DynamoDB's throughput is distributed *per-partition*, so uneven access concentrates load on one physical partition regardless of the table's overall provisioned capacity.
- **L3 — 🔴 Hermes-relevant:** explain how Hermes's event store's partition key likely needs to balance two competing goals: even distribution (avoiding hot partitions under heavy write load) and efficient querying (being able to fetch all events for one aggregate/entity in order, which needs them to share a partition key with a sequence-numbered sort key). `NEEDS VERIFICATION`: confirm your actual key schema before describing it as a deliberate tradeoff in an interview.

---

## Cluster 2: DynamoDB Streams

### What is it?
A time-ordered log of item-level changes (insert/modify/remove) on a table, consumable by Lambda triggers or other consumers — the mechanism your Transactional Outbox pattern depends on.

### Interview Questions
- **L1:** "DynamoDB Streams captures a time-ordered change log per table, which Lambda (or other consumers) can process — this is what powers event-driven reactions to data changes without polling."
- **L2:** connect directly to your Transactional Outbox: writing the event *and* an outbox marker happens atomically within one DynamoDB transaction (avoiding the classic dual-write problem of writing to two separate systems non-atomically); DynamoDB Streams then picks up that write and forwards it downstream (to SQS, per your resume) — this decouples "commit the state change" from "notify the rest of the system," with Streams guaranteeing the notification eventually happens *because* it's driven directly off the committed write, not a separate, potentially-failing network call made by application code at write time.

### Quick Revision
- Partition key = physical distribution; sort key = ordering within a partition.
- Hot partitions come from uneven key distribution, independent of total provisioned capacity.
- DynamoDB Streams is the mechanism underneath a Transactional Outbox built on DynamoDB — solves the dual-write problem by making the notification derive from the already-committed write.
