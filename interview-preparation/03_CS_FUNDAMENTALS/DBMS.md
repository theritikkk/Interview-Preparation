# DBMS Fundamentals

This file covers the *theory* layer — relational modeling, index internals, and distribution (replication/sharding). For transactions, isolation levels, and query-writing, see `01_PROGRAMMING/SQL.md`; for hands-on PostgreSQL/DynamoDB/Redis specifics, see `06_DATABASES/`.

---

## Cluster 1: Relational Model, Normalization, Denormalization

### What is it?
The relational model organizes data into tables (relations) of rows and columns, with relationships expressed via foreign keys rather than nested structures. Normalization removes redundancy by decomposing tables (1NF → 2NF → 3NF, roughly: atomic columns → no partial dependency on part of a composite key → no transitive dependency on non-key columns). Denormalization deliberately reintroduces redundancy to avoid expensive joins at read time.

### Interview Questions
- **L1:** "Normalization reduces redundancy and update anomalies by splitting data into related tables; denormalization trades some redundancy for faster reads by avoiding joins."
- **L2:** give a concrete anomaly example: an unnormalized `orders` table storing `customer_address` on every row means updating a customer's address requires updating every historical order row — normalization (a separate `customers` table, referenced by `customer_id`) fixes this.
- **L3:** explain when denormalization is the *right* call, not just a shortcut: read-heavy workloads where join cost dominates, or systems (like DynamoDB) that don't support efficient joins at all by design — directly relevant to why Hermes's DynamoDB read models are described as "query-optimized": they're denormalized, pre-shaped views built specifically to answer particular queries without joins.

### Resume Connection
🔴 Hermes's "6 query-optimized read models" fed by CQRS *is* denormalization as an architectural pattern — each read model is shaped for a specific query, not for a normalized "single source of truth" (that's the event store's job instead).

---

## Cluster 2: Indexing Internals — B-Tree vs. B+ Tree

### What is it?
Both are balanced, sorted tree structures used for database indexes. The distinction that actually matters in interviews: a **B-tree** stores data (or pointers to data) in *internal* nodes as well as leaves; a **B+ tree** stores data only in leaf nodes, with internal nodes holding only keys for navigation — and, critically, B+ tree leaves are linked together in a sorted list.

### Why does it matter?
That leaf-linked-list property is *why* almost every production relational database (PostgreSQL, MySQL/InnoDB) uses B+ trees for its indexes: a range query (`WHERE x BETWEEN a AND b`) can find the starting leaf via the tree, then walk the linked leaves directly — no need to re-traverse the tree for each subsequent value.

### Interview Questions
- **L1:** "Both are balanced tree structures for fast lookups; B+ trees keep all data in leaves with the leaves linked together, which makes range scans efficient — that's why databases prefer them."
- **L2:** explain why tree height stays small even for millions of rows (a high branching factor — each node holds many keys, matched to a disk-page size, so even a huge table might need only 3-4 levels to reach any row).
- **L3:** explain the practical cost of an index: every `INSERT`/`UPDATE`/`DELETE` must also update every index on that table, and why over-indexing a write-heavy table is a real, common production performance mistake.

### DO NOT SAY THIS
"An index makes every query faster." — An index helps *selective* queries (filtering to a small fraction of rows); on a low-selectivity filter (e.g., a boolean column that's 95% one value), the query planner may rationally choose a full table scan over the index, because random-access index lookups can be slower than a sequential scan when you're going to touch most of the table anyway.

---

## Cluster 3: Replication, Sharding, Partitioning

### What is it?
**Replication**: copying the same data to multiple nodes (for availability and read scaling) — typically one primary accepting writes, with replicas applying the same changes asynchronously or synchronously. **Sharding/partitioning**: splitting *different* data across multiple nodes (for write scaling and storage beyond a single machine's capacity), typically by a partition/shard key.

### How does it work?
- **Synchronous replication:** a write isn't acknowledged until replicas confirm — strong consistency, higher write latency.
- **Asynchronous replication:** a write is acknowledged immediately, replicas catch up shortly after — lower latency, but a replica can serve stale reads, and a primary failure can lose the most recent unreplicated writes.
- **Sharding key choice** determines everything: a bad key (e.g., a monotonically increasing ID) can create a "hot shard" where all recent writes land on one node; a good key spreads load evenly, but can make cross-shard queries expensive or impossible without an application-level fan-out.

### Interview Questions
- **L1:** "Replication copies the same data across nodes for availability/read scaling; sharding splits different data across nodes for write scaling and storage capacity."
- **L2:** explain the sync-vs-async replication tradeoff (consistency and durability vs. latency) and connect it to Hermes: DynamoDB's own internal replication (across Availability Zones) is part of why it can offer high availability, at eventual-consistency-by-default read semantics unless you explicitly request strongly consistent reads.
- **L3 — 🔴 Hermes-specific:** explain how *multi-tenancy* interacts with partitioning: if Hermes's DynamoDB tables use a tenant ID as part of the partition key, that's a deliberate sharding strategy that both spreads load across tenants **and** gives you a natural tenant-isolation boundary at the storage layer, reinforcing the Cognito/RBAC isolation at the application layer. `NEEDS VERIFICATION`: confirm this is actually how your DynamoDB partition keys are structured before stating it as fact in an interview — it's the standard, expected pattern, but you should know your own schema.

### Comparison With Alternatives
Vertical scaling (bigger machine) avoids all of this complexity but hits a hard ceiling and a single point of failure; horizontal scaling (replication + sharding) has no hard ceiling but introduces the consistency and operational complexity discussed above — a classic system-design tradeoff to be ready to narrate.

---

## Cluster 4: The NoSQL Landscape (Conceptual Overview)

| Type | Example | Data model | Good fit when |
|---|---|---|---|
| Relational | PostgreSQL | Tables, joins, ACID transactions | Strong consistency, complex relationships, ad-hoc queries |
| Key-value / wide-column | DynamoDB | Partition key (+ optional sort key), item-level access | Predictable access patterns, massive scale, low-latency point lookups |
| Document | MongoDB | JSON-like documents, flexible schema | Semi-structured data, evolving schemas, document-shaped access |
| In-memory key-value | Redis | Key-value + rich data structures (lists, sets, sorted sets) | Caching, rate limiting, leaderboards, session storage — speed over durability by default |

⚠️ MongoDB and Redis are skills-list items with no project bullet backing them — hold this table's contrast (especially DynamoDB vs. MongoDB, since both are "NoSQL" but structurally very different) as your answer if asked to compare, and be honest that your hands-on NoSQL experience is specifically DynamoDB via Hermes.

### Quick Revision
- B+ tree > B-tree for DB indexes because of linked leaves enabling efficient range scans.
- Denormalization = deliberate redundancy for read speed — this is what Hermes's read models are.
- Replication = same data, multiple nodes (availability/read scale). Sharding = different data, multiple nodes (write scale/capacity).
- Partition key choice determines hot-shard risk and (in Hermes's case) doubles as a tenant-isolation mechanism.
