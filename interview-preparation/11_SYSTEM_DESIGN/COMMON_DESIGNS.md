# System Design — Common Designs

Two of these (Job Scheduler, Event-Driven Workflow Engine) are essentially your own projects in disguise — treat them as a chance to re-derive Hermes and the Task Scheduler from first principles, which is excellent practice for defending the real thing (`13_PROJECT_DEEP_DIVES/`).

---

## Design 1: URL Shortener (Baseline — Know This Cold)

**Requirements:** shorten a long URL → short code; redirect short code → original URL; high read:write ratio (reads >> writes).

**API:** `POST /shorten {url} → {shortCode}` · `GET /{shortCode} → 302 redirect`

**Data model:** `shortCode (PK) | longUrl | createdAt | expiresAt`

**Architecture:**
```
Client → Load Balancer → App Servers → Cache (Redis) → Database
                                          ↑ cache-aside, since reads dominate
```

**Key design decision — short code generation:** base62-encode an auto-incrementing ID (simple, guaranteed unique, but reveals approximate creation order/volume) vs. a hash of the URL truncated to N chars (no central counter needed, but must handle collisions) — know both and their tradeoffs.

**Bottlenecks & scaling:** reads dominate heavily → cache aggressively (`06_DATABASES/REDIS.md`); the database can be simple key-value at scale (no complex queries needed) → a natural fit for DynamoDB-style storage, not necessarily relational.

**Interview Questions:**
- **L2:** "How do you handle a custom/vanity short code request colliding with an existing one?" — check-then-reserve with a conditional write, return a 409 Conflict (`04_BACKEND/REST.md`) if taken.

---

## Design 2: Rate Limiter

**Requirements:** limit requests per client per time window; must work correctly across multiple distributed API server instances (not just in-memory per-instance state, which would let a client exceed the limit by hitting different instances).

**Architecture:**
```
Client → API Servers (each checks) → Shared Redis (atomic counter/token bucket state)
```

**Key design decision:** the rate-limit *state* must be shared/centralized (Redis is the standard choice, using `INCR` + `EXPIRE` for fixed window, or a Lua script for atomic token-bucket logic) — an in-memory counter per API server is wrong the moment you have more than one server, since each server would independently allow the full limit.

**Interview Questions:**
- **L2:** "Why Redis specifically, not just a database table with a counter column?" — Redis's in-memory speed and atomic increment operations (`INCR`) are essential given a rate limiter sits on the hot path of *every* request; a relational database round-trip per request for this check would itself become a bottleneck (`06_DATABASES/REDIS.md`'s cluster on rate limiting as a use case).
- **L3:** connect to `12_SECURITY/CLOUD_SECURITY.md`'s algorithm comparison (fixed window vs. sliding window vs. token bucket) — be ready to justify a specific choice given stated requirements (bursty traffic tolerance vs. strict smoothing).

---

## Design 3: Job Scheduler (Cron-as-a-Service) — Mirrors Your Task Scheduler 🔴

**Requirements:** schedule a job to run at a specific time (or on a recurring cron-style schedule); guarantee it runs (retry on failure); handle horizontal scaling of workers; avoid double-execution.

**Architecture (design-from-scratch version — compare against your actual implementation once done):**
```
Client → API (create job) → Database (source of truth: job definitions + next-run time)
                                    ↓
                            Coordinator (finds due jobs)
                                    ↓
                          Message Queue (dispatch)
                                    ↓
                          Worker Fleet (execute, ack)
```

**Key design decision — how does the coordinator find due jobs without hot-polling?** Exactly the problem your actual Task Scheduler solves with RabbitMQ TTL + dead-lettering (`07_DISTRIBUTED_SYSTEMS/MESSAGING.md`) instead of a naive `SELECT * FROM jobs WHERE next_run <= NOW()` on a tight polling loop.

**Failure modes to name explicitly:**
- Worker crashes mid-job → needs a visibility-timeout-style mechanism (`08_AWS/SQS.md`'s pattern, or RabbitMQ's ack/requeue) so the job gets retried, not lost.
- Duplicate execution → needs idempotent job execution or a claiming mechanism (`01_PROGRAMMING/SQL.md`'s `FOR UPDATE SKIP LOCKED`).
- Coordinator itself crashes → is it a single point of failure? (a real weakness worth naming proactively, per the Interview Framework's pitfall-avoidance advice)

**Interview Questions:**
- **L3:** "How would you scale this to millions of scheduled jobs?" — partition/shard jobs (e.g., by a hash of job ID) across multiple coordinator instances, each responsible for a subset, avoiding one coordinator needing to scan the entire job table.

---

## Design 4: Event-Driven Workflow Engine — Mirrors Hermes 🔴

**Requirements:** execute a multi-step workflow where each step may depend on the previous one's outcome; must be resumable/durable (a crash mid-workflow shouldn't lose progress); needs an audit trail of what happened.

**Architecture (design-from-scratch, then compare to Hermes):**
```
Trigger → Workflow Engine (state machine) → Step 1 → Step 2 → Step 3
                    ↓ (persists progress after EVERY step — this is the durability mechanism)
              Event Store / State Table
```

**Key design decision — how do you make this durable/resumable?** Two broad approaches: (a) persist the *current state* after each step (simple, but loses history of how you got there), or (b) persist every *event* that occurred and derive current state by replay — i.e., Event Sourcing (`07_DISTRIBUTED_SYSTEMS/EVENT_DRIVEN.md`), which is exactly what Hermes does, and which additionally gives you a full audit trail "for free" as a side effect of the durability mechanism itself.

**Key design decision — orchestration vs. choreography:** a central engine explicitly directing each step (orchestration — like Step Functions, `08_AWS/STEP_FUNCTIONS.md`) vs. each step reacting independently to events from the previous one (choreography — like EventBridge, `08_AWS/EVENTBRIDGE.md`). Hermes uses both, at different points, and being able to say precisely *where* each is used and why is a strong, specific answer.

**Interview Questions:**
- **L3:** "How do you handle a step that needs to call an external, unreliable service?" — retries with backoff, a timeout, and a circuit breaker if the external service is degraded (avoid hammering an already-failing dependency) — and critically, the step's *own* execution must be idempotent, since a retry might re-run a step whose side effect (e.g., "charge the customer") already partially succeeded once.

---

## Other Designs (Lighter Treatment — Know the Shape, Not Full Depth)

- **Chat application:** WebSockets for real-time delivery (`04_BACKEND/WEBSOCKETS.md`), a message store (append-heavy, read-recent-heavy access pattern), presence tracking (who's online), and the multi-instance WebSocket scaling problem (shared pub/sub backplane) solved identically to the general WebSockets case.
- **Notification system:** fan-out from an event to multiple channels (push, email, SMS) — a natural fit for a pub/sub/queue architecture (conceptually similar to Hermes's EventBridge fan-out), with per-channel retry and a user-preference/dedup layer to avoid notification spam.
- **File upload system:** direct-to-storage upload (client gets a pre-signed URL, uploads directly to S3-equivalent storage, bypassing your API servers as a bottleneck for the actual bytes) + an async processing pipeline (virus scan, thumbnail generation) triggered off the upload event — the same event-driven-trigger shape as Hermes's Streams-driven pipeline.
- **Social feed:** the classic fan-out tradeoff — fan-out-on-write (precompute each follower's feed when a post is made; fast reads, expensive/slow for accounts with huge follower counts) vs. fan-out-on-read (compute the feed at read time by querying followed accounts; slow reads, cheap writes) — most real systems use a hybrid based on follower count.
- **Payment system:** idempotency keys are non-negotiable here (`04_BACKEND/REST.md`) — a payment must never be double-charged on a client retry; typically a two-phase design (reserve/authorize, then capture) with a durable ledger as the source of truth, conceptually similar to Event Sourcing's "the log is the truth" philosophy.
- **Ride matching:** geospatial indexing (geohashing or a quadtree) to efficiently find nearby drivers, plus a matching algorithm balancing wait time against driver utilization — a genuinely different problem domain (spatial data) from your resume's strengths, worth knowing the shape of but not a priority to go deep on given your project background.

### Quick Revision
- URL Shortener: cache-aside, high read:write ratio, simple key-value storage is enough.
- Rate Limiter: shared centralized state (Redis) is mandatory across multiple API server instances.
- Job Scheduler: your own Task Scheduler, from scratch — TTL/dead-lettering avoids polling, idempotent claiming avoids double-execution.
- Event-Driven Workflow Engine: your own Hermes, from scratch — Event Sourcing for durable/resumable/auditable state; orchestration vs. choreography as complementary tools.
