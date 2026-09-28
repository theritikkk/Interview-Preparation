# SQL

🔴 Directly load-bearing for your Distributed Task Scheduler claims: PostgreSQL row-count convergence under concurrent load is a core piece of your "exactly-once effect" evidence.

---

## Cluster 1: Query Fundamentals — JOINs, GROUP BY, Subqueries, CTEs, Window Functions

### What is it?
The relational query toolkit: JOINs combine rows across tables by a matching condition; `GROUP BY`/`HAVING` aggregate; CTEs (`WITH`) name a subquery for readability/reuse; window functions compute per-row values *across* a related set of rows without collapsing them like `GROUP BY` does.

### How does it work?
```sql
-- Window function: rank each task's attempt without collapsing rows, unlike GROUP BY
SELECT id, worker_id, attempt_count,
       RANK() OVER (PARTITION BY worker_id ORDER BY attempt_count DESC) AS rnk
FROM tasks;

-- CTE for readability
WITH failed_tasks AS (
    SELECT * FROM tasks WHERE status = 'failed'
)
SELECT worker_id, COUNT(*) FROM failed_tasks GROUP BY worker_id HAVING COUNT(*) > 5;
```

### Interview Questions
- **L1:** "JOINs combine tables by matching keys; GROUP BY aggregates and collapses rows; window functions compute aggregates per-row without collapsing; CTEs name a subquery for reuse and readability."
- **L2:** explain `INNER` vs `LEFT` vs `FULL OUTER` JOIN precisely (what happens to unmatched rows on each side), and `WHERE` vs `HAVING` (WHERE filters rows before grouping, HAVING filters groups after aggregation).
- **L3:** explain a correlated subquery vs. an uncorrelated one, and why a correlated subquery can be re-executed per outer row (performance risk) — one of the reasons window functions or a JOIN are often preferred.

---

## Cluster 2: Transactions, Isolation Levels, Locking 🔴

### What is it?
A transaction is a unit of work that's atomic (all-or-nothing), consistent, isolated (from concurrent transactions), and durable (ACID). Isolation levels trade correctness guarantees for concurrency: `READ UNCOMMITTED` < `READ COMMITTED` < `REPEATABLE READ` < `SERIALIZABLE`.

### How does it work?
- **Dirty read** (only possible below `READ COMMITTED`): reading another transaction's uncommitted changes.
- **Non-repeatable read** (possible below `REPEATABLE READ`): re-reading the same row twice in one transaction gives different values because another transaction committed in between.
- **Phantom read** (possible below `SERIALIZABLE`): re-running the same query returns a *different set of rows* because another transaction inserted matching rows.
- PostgreSQL specifically uses **MVCC** (Multi-Version Concurrency Control): readers never block writers and vice versa, because each transaction sees a consistent snapshot of the data as of its start — this is why PostgreSQL's default `READ COMMITTED` is often "good enough" without heavy locking.

### Interview Questions
- **L1:** "Isolation levels control what concurrency anomalies (dirty reads, non-repeatable reads, phantoms) are possible; higher isolation means more correctness, less concurrency."
- **L2:** explain PostgreSQL's MVCC model and why it means most reads don't block on writes.
- **L3 — 🔴 directly tied to your Task Scheduler claim:** explain precisely how you'd guarantee two workers never process the same task row twice under concurrent access. The standard pattern is `SELECT ... FOR UPDATE SKIP LOCKED` (lock the row you're claiming, and skip — don't wait on — rows other workers have already locked) so concurrent workers never double-claim a task without serializing all of them behind one lock. **`NEEDS VERIFICATION`: confirm whether your scheduler actually uses this pattern (or an equivalent, like a `claimed_by`/`claimed_at` compare-and-set update) before describing it as your mechanism in an interview — don't claim `FOR UPDATE SKIP LOCKED` unless you know that's really what's happening.**

### Follow-up Chain
> **Q: How does your PostgreSQL row-count convergence claim actually prove exactly-once processing?**
> A: After the load test drained, the count of rows in a terminal state matched the count of consumed messages 1:1, and a post-drain query for still-`scheduled` rows returned zero.
> **Follow-up: Couldn't two workers have double-processed a task, then one write "lost" the race, still giving you a matching final count?**
> A: Only if the write pattern makes double-processing *idempotent* or *impossible* — i.e., you need a locking or upsert-with-a-guard strategy (see `FOR UPDATE SKIP LOCKED` above) so a second worker literally cannot claim an already-claimed row, not just "probably won't in practice."
> **Trap: "The row counts matching is proof of exactly-once delivery."**
> Correct response: it's evidence of an exactly-once *effect on the data*, contingent on the claiming mechanism actually preventing double-claims — that mechanism, not the row-count check alone, is the real guarantee. State it in that order.

### DO NOT SAY THIS
"`SERIALIZABLE` prevents all concurrency bugs with no downside." — It prevents the most anomalies but can cause serialization failures (transactions aborted and forced to retry) under contention — a real tradeoff, not a free upgrade.

---

## Cluster 3: Indexing & Query Optimization

### What is it?
An index (typically a B-tree) is an auxiliary sorted structure that lets the database find rows without scanning the whole table — the classic tradeoff is faster reads, slower writes (every insert/update must also update the index), plus storage overhead.

### How does it work?
```sql
EXPLAIN ANALYZE SELECT * FROM tasks WHERE status = 'scheduled' AND run_at < now();
-- Look for: Seq Scan (bad on a large table for a selective filter) vs. Index Scan (good)
CREATE INDEX idx_tasks_status_runat ON tasks (status, run_at);  -- composite index matching the query
```

### Interview Questions
- **L1:** "An index is a B-tree structure that avoids full table scans for selective queries, at the cost of slower writes and extra storage."
- **L2:** explain column order in a composite index (`(status, run_at)` serves queries filtering on `status` alone or `status + run_at`, but not `run_at` alone efficiently) — a very common practical question.
- **L3:** explain how you'd diagnose a slow query with `EXPLAIN ANALYZE` — read the actual vs. estimated row counts, look for sequential scans on large tables, and check whether an existing index is even being used (stale statistics can cause the planner to skip a valid index).

### Comparison With Alternatives
B-tree (range queries, equality, sorting) vs. hash index (equality only, no range support) vs. GIN/GiST (full-text search, arrays, JSON in Postgres) — pick based on the query pattern, not by default habit.

## Cluster 4: Normalization & Denormalization (SQL-Level View)

Theory and the Hermes-connection are in `03_CS_FUNDAMENTALS/DBMS.md` Cluster 1 — this is the same concept from the query-writing side: what normalized vs. denormalized schemas actually look like and cost in SQL.

### How does it work?
```sql
-- Unnormalized: customer info repeated on every order row — update anomaly waiting to happen
CREATE TABLE orders_bad (
    order_id INT, customer_name TEXT, customer_email TEXT, item TEXT, amount DECIMAL
);

-- Normalized (3NF): customer info lives once, referenced by foreign key
CREATE TABLE customers (customer_id INT PRIMARY KEY, name TEXT, email TEXT);
CREATE TABLE orders (
    order_id INT PRIMARY KEY, customer_id INT REFERENCES customers(customer_id),
    item TEXT, amount DECIMAL
);

-- Querying normalized data costs a JOIN:
SELECT o.item, o.amount, c.name FROM orders o JOIN customers c ON o.customer_id = c.customer_id;
```

### Interview Questions
- **L1:** "A normalized schema avoids repeating data (fixing update anomalies) at the cost of needing joins to reassemble it at query time; a denormalized schema repeats data deliberately to avoid those joins, at the cost of update complexity and storage."
- **L2:** explain precisely when denormalizing a specific table is the right SQL-level call: a read-heavy reporting table that's rebuilt periodically (not updated in place) is a much safer place to denormalize than a table with frequent, scattered updates — because the update-anomaly risk that normalization protects against is far less relevant when the data isn't updated in place at all.
- **L3:** connect back to `03_CS_FUNDAMENTALS/DBMS.md`'s framing of Hermes's read models as denormalization-as-CQRS: even within a single relational database, a materialized view or a deliberately denormalized reporting table is doing the *same conceptual job* as one of Hermes's DynamoDB read models — pre-shaping data for a specific query pattern instead of computing it fresh via joins every time.

### Quick Revision
- WHERE filters rows before grouping; HAVING filters groups after.
- MVCC (Postgres): readers don't block writers via snapshot isolation.
- `FOR UPDATE SKIP LOCKED` is the standard pattern for safe concurrent task-claiming — verify this is actually what your scheduler does before claiming it.
- Composite index column order matters — leftmost-prefix rule.
- `EXPLAIN ANALYZE` is your first move diagnosing a slow query.
- Normalized: no repeated data, needs joins to reassemble. Denormalized: repeated data, no joins needed, riskier on frequent in-place updates — safest to denormalize read-heavy, rebuilt-not-updated tables.
