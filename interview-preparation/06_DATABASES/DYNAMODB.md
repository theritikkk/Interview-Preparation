# DynamoDB (Data Modeling View)

🔴 Companion to `08_AWS/DYNAMODB.md` (which covers capacity modes/Streams). This file covers data-modeling philosophy — the part most different from relational thinking.

---

## Cluster 1: Single-Table Design & Access-Pattern-First Modeling

### What is it?
Unlike relational modeling (model the entities and their relationships first, worry about queries later — joins handle the rest), DynamoDB modeling works backwards: **enumerate your access patterns first**, then design keys (and often store multiple unrelated entity types in one table) specifically to serve those patterns with single-item or single-query lookups — because DynamoDB has no server-side joins.

### How does it work?
```
Partition Key       | Sort Key            | Attributes
TENANT#t1            EVENT#0001            { type: 'TaskCreated', ... }
TENANT#t1            EVENT#0002            { type: 'TaskCompleted', ... }
TENANT#t1            READMODEL#task#5      { status: 'completed', ... }
```
Storing genuinely different entity types (events, read-model rows) under a shared partition-key prefix pattern is a deliberate DynamoDB idiom, not a modeling mistake — it lets a single query (fetch everything under `TENANT#t1`, or a sort-key range within it) retrieve related data across entity types in one round trip.

### Interview Questions
- **L1:** "DynamoDB modeling starts from access patterns, not entity relationships — you design partition/sort keys to serve specific queries directly, often storing multiple entity types in one table since there are no server-side joins."
- **L2:** explain why this is a fundamentally different design process than relational normalization (`03_CS_FUNDAMENTALS/DBMS.md`) — normalize-then-query-with-joins vs. denormalize-and-key-for-the-query-you'll-actually-run — and why getting the access patterns wrong upfront is expensive to fix later (unlike a relational schema, where a new query pattern is usually "just write a new query/join," a new DynamoDB access pattern may require a new GSI or a data-model change).
- **L3 — 🔴 Hermes-relevant:** this is precisely why CQRS's six separate read models (`07_DISTRIBUTED_SYSTEMS/EVENT_DRIVEN.md`) make sense as a DynamoDB-native design, not just a generic architectural choice: rather than trying to force one single-table design to serve six different access patterns efficiently, each read model can have keys shaped exactly for its one query — DynamoDB's "model for the query" philosophy applied at the granularity of an entire read model rather than within one table.

### DO NOT SAY THIS
"DynamoDB is just a slower, more limited version of a relational database." — It's optimized for a different problem (predictable, massive-scale, low-latency access via known keys) at the cost of flexibility (no ad-hoc joins/queries) — the right framing is "different tool for a different access pattern," not "worse."

### Quick Revision
- Access patterns first, then design keys — the opposite order from relational normalization.
- Multiple entity types sharing a table/partition-key prefix is an intentional idiom, not a smell.
- Hermes's six read models are single-table-design thinking applied at the read-model level — each shaped for its own query.
