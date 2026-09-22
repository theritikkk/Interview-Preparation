# PostgreSQL

🔴 Source of truth for the Distributed Task Scheduler. Transaction/isolation-level theory is in `01_PROGRAMMING/SQL.md`; this file covers Postgres-specific internals.

---

## Cluster 1: MVCC in Practice — Tuples & VACUUM

### What is it?
PostgreSQL implements MVCC (`03_CS_FUNDAMENTALS/DBMS.md`) by never overwriting a row in place on `UPDATE` — instead, it writes a *new tuple* (row version) and marks the old one as no-longer-visible-to-new-transactions once nothing needs it, rather than deleting it immediately. `VACUUM` is the background process that reclaims space from these dead tuples.

### How does it work?
```sql
UPDATE tasks SET status = 'completed' WHERE id = 5;
-- Under the hood: a NEW tuple is written (status='completed'), the OLD tuple is marked
-- dead once no active transaction still needs to see it. VACUUM later reclaims that space.
```

### Interview Questions
- **L1:** "PostgreSQL's MVCC writes new row versions instead of overwriting in place; VACUUM reclaims space from old versions no longer needed by any active transaction."
- **L2:** explain **table bloat**: a high-`UPDATE`-rate table (directly relevant to your Task Scheduler's tasks table under a 10,000-task load test) accumulates dead tuples faster than autovacuum may keep up with under sustained heavy load, growing the table's on-disk size and slowing scans until vacuum catches up — a legitimate operational concern for exactly the kind of write-heavy workload your load test represents.
- **L3:** connect this to your concurrent task-claiming design (`01_PROGRAMMING/SQL.md`): `SELECT ... FOR UPDATE SKIP LOCKED` interacts with MVCC directly — locked-but-uncommitted rows are simply skipped by other transactions rather than blocking them, which is precisely why this pattern scales well for concurrent worker task-claiming versus a naive `SELECT ... FOR UPDATE` (which would make competing workers queue up waiting on the lock instead of moving on to a different available row).

---

## Cluster 2: Connection Limits & Pooling

### What is it?
Each PostgreSQL connection consumes real server-side memory and a backend process — `max_connections` is a hard ceiling, and it's typically far lower than the number of concurrent application-level requests a busy system handles, because connections are meant to be pooled and reused, not opened per-request.

### Interview Questions
- **L1:** "PostgreSQL's max_connections is a hard limit on concurrent backend processes; a connection pool (PgBouncer, or an app-level pool) reuses a smaller number of real connections across many more logical requests."
- **L2 — 🔴 directly relevant to your load test:** explain why your Task Scheduler's 3 worker replicas, each potentially opening many connections under a 10,000-task burst, needed connection pooling to avoid exhausting `max_connections` before the application logic itself became the bottleneck — a very plausible root cause if you were ever asked "what was the actual bottleneck in your load test?"

### Quick Revision
- MVCC = new tuple per update, not in-place overwrite; VACUUM reclaims dead tuples.
- Table bloat is a real risk under sustained high-UPDATE load — relevant to your own load test's write pattern.
- `FOR UPDATE SKIP LOCKED` + MVCC = why concurrent workers don't block each other claiming different rows.
- Connection pooling is what prevents `max_connections` from becoming the load test's real bottleneck.
