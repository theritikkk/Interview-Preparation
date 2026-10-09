# Project Deep Dive: Distributed Task Scheduler — Cron-as-a-Service

🔴🔴🔴 **This file has been verified directly against your source code** (`distributed-scheduler-main.zip`). Every mechanism below is what the code actually does, not a plausible guess — where the earlier draft of this file guessed wrong, that's called out explicitly so you don't accidentally memorize the wrong version.

---

## Corrections From the Earlier (Pre-Verification) Draft

- **Idempotency mechanism was wrong.** The earlier draft guessed `FOR UPDATE SKIP LOCKED`. The real mechanism is different and, honestly, cleaner: each task execution gets a row in `task_executions` keyed by `execution_id`; the worker checks `execution_already_finished(execution_id)` (i.e., `completed_at IS NOT NULL`) **before** running the command, and on a redelivered message, does an **idempotent skip** — logs it, increments a dedicated Prometheus counter (`TASKS_IDEMPOTENT_SKIP`), acks without re-executing. `FOR UPDATE` (not `SKIP LOCKED`) is used separately, by the coordinator, when it claims a due task and creates the execution row.
- **Load-test tool was wrong.** The earlier draft guessed k6. The actual load test (documented in `screenshots/deployment/test-5/README.md`, 52 timestamped screenshots) used a **plain bash `for` loop with `curl`** — 10,000 sequential `curl` POSTs, each scheduling a task for the same instant two minutes out, specifically to force every TTL to expire simultaneously ("the hardest possible scenario... a single burst rather than a gradual ramp" — your own words in the test doc).
- **New finding, not in the earlier draft at all:** there's a **slow reconciliation poller** (every 120s by default, `SCHEDULER_RECONCILE_INTERVAL_SEC`), scanning PostgreSQL directly for due-but-unpublished tasks (`LIMIT 100` per cycle) and re-publishing them straight to `scheduler.due`. This means the honest answer to "why not polling?" isn't "we never poll" — it's "TTL-based dispatch is the fast path; a slow reconciliation poll exists as a safety net for the delay mechanism's edge cases (broker restarts clearing undelivered wakeups, the max-TTL cap below, etc.)." That's a *more* sophisticated answer than "zero polling," not a weaker one.
- **A repo-hygiene issue worth knowing about, not a resume-accuracy problem:** your repo has two documents about the load test that contradict each other. `Benchmark.md` frames the 10k-task/8,562-peak/1,600-msg/s/~7m46s numbers as a **"Suggested Load Test (future)"** you hadn't run yet, and even suggests a resume bullet of "1,000+ tasks" — much lower than what's on your actual resume. But `screenshots/deployment/test-5/README.md` proves the real 10k test was actually run, with every single number matching your resume exactly. `Benchmark.md` is simply stale — written before test-5, never updated afterward. **Worth fixing in the repo** (an interviewer skimming your GitHub could get confused by the contradiction), but your resume numbers are fully verified and real.

---

## Architecture (Verified)

```
Client → API Gateway (Node.js/Express/TypeScript, port 3000)
              │  JWT auth (bcrypt, 10 salt rounds, 7-day token, no refresh rotation)
              │  Rate limit: 10,000 req / 15 min per IP · body limit 10kb
              │  Every task query scoped by WHERE ... AND user_id = $N
              ▼
        PostgreSQL (tasks, task_executions, users, workers — full schema below)
              ▲
              │  FOR UPDATE row lock + conditional UPDATE (rowcount-checked)
        Coordinator (Python) ── consumes scheduler.due
              │
              ▼
   scheduler.delay (TTL=per-message expiration, max 2,147,483,647 ms ≈ 24.8 days,
                    x-dead-letter-exchange="" / x-dead-letter-routing-key=scheduler.due)
              │  (TTL expiry → native RabbitMQ dead-lettering)
              ▼
        scheduler.due → Coordinator claims → scheduler.tasks
              │
              ▼
   Worker Fleet (Python; worker-1/2/3 are the standard, Prometheus-scraped set —
                 docker-compose.yml also defines worker-4..10 for extra load-test
                 capacity, not part of the standard 3-replica/observability story)
              │  runs task.command via subprocess.run(argv_list) — NOT shell=True
              │  (avoids classic shell injection; no validation on what the
              │  command itself can be — see Security note below)
              │
              ├─ success → task_executions updated, ack
              ├─ failure → scheduler.retry_delay (TTL=backoff, same DLX pattern
              │            as above, dead-letters back into scheduler.tasks)
              │            exponential backoff: base 5,000ms, cap 300,000ms (5min)
              └─ exceeded TASK_MAX_RETRIES (default 3) → explicit application-level
                 publish to scheduler.dlx exchange / scheduler.tasks.dlq

Reconciliation poller (Python, every 120s): SELECT due-but-unpublished tasks
   LIMIT 100 → republish directly to scheduler.due (bypasses the delay queue —
   the safety net for TTL-mechanism edge cases)

Observability: Prometheus scrapes api-gateway:3000, coordinator:9090, worker-1/2/3:9100,
   rabbitmq-exporter:9419 · Loki + Promtail (Docker-socket discovery, 7-day retention)
   Grafana dashboard "Distributed Scheduler v2" · Alertmanager rules incl.
   DLQMessagesPresent, WorkerConsumerDown

Deployment (as actually run): single EC2 t3.large instance, Docker Compose —
   NOT a multi-host/K8s fleet. Horizontal "scaling" today means more worker
   containers on the same host, not more hosts.
```

---

## Verified Database Schema

```sql
users            (id, email, password_hash, created_at)
tasks            (id, user_id, command, schedule_type, next_execution_time,
                  status, retry_count, max_retries, created_at, updated_at)
task_executions  (id [execution_id], task_id, status, started_at, completed_at,
                  result, error, worker_id)
workers          (id, last_heartbeat, status)

Indexes:
  tasks(next_execution_time) — PARTIAL: WHERE status IN ('pending','scheduled')
  tasks(user_id, status)
  task_executions(task_id)
  workers(last_heartbeat)
```
The partial index is a nice, specific detail worth mentioning proactively if asked about indexing choices: it excludes completed/failed tasks from the index entirely, since those rows are never queried by "what's due now," keeping the index smaller and cheaper to maintain than a full index over the whole table.

---

## The Named Questions — Now With Verified Answers

**"Why RabbitMQ? Why TTL? Why not polling?"** → Confirmed as designed: `scheduler.delay`'s per-message TTL + native dead-lettering triggers dispatch with zero hot-polling. **New nuance:** a 120s reconciliation poll exists as a deliberate safety net, not the primary mechanism — mention this proactively, it's a stronger answer than claiming zero polling anywhere in the system.

**"How does delayed scheduling work?"** → `pika.BasicProperties(expiration=str(ms))` sets the TTL in milliseconds, capped at 2,147,483,647ms (~24.8 days — RabbitMQ's practical per-message TTL ceiling). **Verified edge-case handling:** a task scheduled further out than that gets its TTL clamped to the max; when that capped delay expires and lands in `scheduler.due`, the coordinator checks `next_execution_time` again — if it's still in the future, it re-publishes a fresh wakeup with a newly computed delay. This is a genuinely elegant recursive re-arming mechanism for far-future tasks, and a strong answer to "what's your max delay and what happens beyond it?"

**"What happens if a worker dies?"** → Message isn't acked, RabbitMQ redelivers to another consumer on the queue. The worker's `execution_already_finished()` check means a redelivery of a task that *did* complete before the crash gets skipped cleanly (idempotent skip); a redelivery of a task that *didn't* complete gets legitimately retried.

**"How are duplicate tasks handled?"** → **Verified, precise answer, two layers:** (1) Coordinator-side: `SELECT ... FOR UPDATE` on the task row plus a conditional `UPDATE ... WHERE id = %s AND status = 'scheduled'` with a rowcount check — belt-and-suspenders against the same task being claimed twice. (2) Worker-side: the `execution_id`/`completed_at` check described above prevents re-executing a redelivered message for an already-finished execution.

**"Is the append/claim atomic?"** — not directly asked in the original brief, but a natural follow-up now that you know the mechanism: the coordinator's claim is a single conditional `UPDATE` with a rowcount check, which *is* atomic at the database level — a strong, correct answer if pushed on this.

**"What does 'exactly-once' mean? Is it really guaranteed?"** 🔴 — unchanged in substance, now with verified specifics to cite: RabbitMQ's real guarantee is at-least-once (with acks). Your system achieves an exactly-once *effect* through the `execution_id`-keyed idempotent-skip check at the worker, verified in your load test by PostgreSQL row-count convergence with zero orphaned `scheduled` rows and (per test-5) **zero actual redeliveries occurred** during that specific run — meaning idempotency wasn't even exercised by the headline 10k test; it was validated separately via a dedicated duplicate-delivery chaos test documented in `Benchmark.md`'s reliability table. Keep these two test efforts distinct if asked: the 10k run proved throughput/drain/loss; a separate, smaller chaos test proved the idempotency mechanism itself under deliberate duplicate delivery.

**"How did the load test work? What does 8,562 peak / 1,600 msg/s / ~7m46s mean?"** → Fully verified via `screenshots/deployment/test-5/`: a bash loop firing 10,000 sequential `curl` POSTs, each task scheduled for the identical instant (+2 minutes), deliberately forcing simultaneous TTL expiry rather than a gradual ramp — the queue peaked at 8,562 in-flight messages, drained from 14:55 to 15:02:46 (≈7m46s), zero DLQ messages, zero task loss, p95 latency recovering from a 160ms spike to a 76ms floor.

**"What's the bottleneck?"** → Not explicitly logged in the test doc as a single named bottleneck; a reasoned, honest answer given the architecture: with 3 Prometheus-scraped workers processing a shared queue and PostgreSQL as the single source of truth for both claim-locking and execution records, the two most likely candidates are (a) PostgreSQL's row-lock contention on the `tasks` table if claim rate is very high, or (b) raw worker throughput if command execution itself is slow — back-calculating from the numbers: 8,562 messages drained in ~466 seconds ≈ **~18 msg/s sustained** across 3 workers, i.e., roughly 6 msg/s per worker — a specific number worth having ready.

---

## New Security Detail Worth Knowing (Not in the Original Brief)

Tasks execute as literal shell commands: `{"command": "echo hello"}` → `subprocess.run(cmd.strip().split(), ...)`. Two things worth being precise about if security comes up:
- **What's protected:** using `subprocess.run` with an argv list (not `shell=True`) means classic shell-metacharacter injection (`; rm -rf /`, backticks, pipes) is **not** interpreted by a shell — the "command" is split on whitespace and executed directly as a program + arguments.
- **What's not protected:** there's no validation or sandboxing of *what* the command/program itself can be — a user can schedule any executable reachable on the worker's `PATH` with any arguments. Authorization is enforced at the task-ownership level (a user can only schedule/see their own tasks via the `user_id` JWT-derived scoping), not at the command-content level. This is an honest, specific thing to say if asked "how would you secure this further" — sandboxing (containers-per-execution, a strict allowlist of permitted commands, or dropping to a restricted, non-privileged execution user) would be the natural next step.

### Quick Revision
- Idempotency is `execution_id` + `completed_at`-based, not row-locking-based — row-locking (`FOR UPDATE`) is the *coordinator's* claim mechanism, a separate layer.
- The load test used bash + curl, not k6 — say so if asked about tooling.
- `Benchmark.md` is a stale draft; `screenshots/deployment/test-5/` is the real, verified evidence — know where the receipts actually are.
- A 120s reconciliation poll is a deliberate safety net alongside TTL dispatch, not a contradiction of "no polling."
- Deployed today as Docker Compose on a single EC2 host, not a multi-host cluster — be accurate if asked how it's actually running.
- Shell commands run via argv (no shell interpretation) but with no content validation — a specific, honest answer to a security follow-up.
