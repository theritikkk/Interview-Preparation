# E. Project Deep Dives

> ION interviews include "detailed inquiries based on projects listed in resume." Practice these spoken explanations aloud.

---

## E.1 Distributed Task Scheduler (Cron-as-a-Service) ⭐

### 30-Second Introduction
> "I built a distributed cron-as-a-service that lets users schedule tasks via REST APIs. It uses a coordinator-worker architecture with RabbitMQ for task distribution. The system scales horizontally — just add workers. I load-tested it with 10,000 tasks scheduled for the same instant and it processed them all with zero task loss, zero crashes, and p95 latency recovering to 76 milliseconds."

### 2-Minute Deep Dive
> "The system has three main components: an API gateway that handles JWT authentication and rate limiting, a coordinator that manages scheduling in PostgreSQL, and a pool of worker replicas that execute tasks.
>
> Instead of polling the database for due tasks — which creates constant DB load — I used RabbitMQ's TTL-based dead-letter routing. When a task is scheduled, I publish a message with a TTL equal to the delay until execution time. When the TTL expires, RabbitMQ automatically routes it to the execution queue where workers pick it up. This eliminated polling entirely.
>
> For exactly-once execution, each task gets a unique execution ID. Before executing, a worker atomically checks and claims the task using this ID. A reconciliation pass periodically scans for tasks that were scheduled but never executed — covering edge cases like worker crashes mid-execution.
>
> I load-tested with 10,000 tasks all targeting the same second. RabbitMQ hit an 8,562-message peak, ingesting at 74 messages per second with a TTL-expiry burst of about 1,600 messages per second. The entire queue drained in about 7 minutes 46 seconds at 50-61 tasks per second. The p95 latency spiked to 160ms under peak load but recovered to a 76ms floor. Zero messages hit the dead-letter queue, zero tasks were lost, zero workers crashed.
>
> Observability uses Prometheus for metrics, Grafana for dashboards, Loki for logs, and Alertmanager for alerts — monitoring queue depth, worker health, and latency in real time. CI runs on every push via GitHub Actions, and it deploys on AWS EC2 with Docker Compose."

### Architecture

```
Client → [API Gateway (JWT + Rate Limit)]
              ↓
         [Coordinator / Scheduler]
              ↓
         [PostgreSQL] ←→ [Reconciliation Worker]
              ↓ (publish with TTL)
         [RabbitMQ]
         ├── Delay Exchange (TTL)
         └── Execution Queue → [Worker 1] [Worker 2] [Worker 3]
              ↓                         ↓
         [DLQ (failures)]      [Result/Callback]

Observability: Prometheus → Grafana + Alertmanager, Loki for logs
```

### Key Technical Decisions & Trade-offs

| Decision | Why | Alternative Considered |
|----------|-----|----------------------|
| RabbitMQ TTL instead of DB polling | Eliminates constant DB queries, event-driven | Polling: simple but wasteful; Redis ZSET: viable but adds another data store |
| PostgreSQL for task metadata | ACID guarantees for scheduling state | MongoDB: flexible but weaker consistency |
| Execution ID for idempotency | Prevents duplicate execution after worker crash/retry | DB lock: works but creates contention |
| Horizontal worker scaling | Throughput grows linearly with workers | Vertical: limited ceiling |
| Reconciliation pass | Catches edge cases TTL-based approach misses | Without it: risk of lost tasks on infrastructure failures |

### 10+ Challenging Follow-Up Questions

1. **What happens if a worker crashes mid-execution?** — The execution ID is claimed but not marked complete. The reconciliation pass detects this and re-enqueues the task. The new execution uses a fresh execution ID to avoid conflicts.

2. **How do you prevent duplicate execution?** — Each task run gets a unique execution ID. Workers atomically claim tasks using compare-and-swap on this ID in PostgreSQL. If two workers race, only one succeeds.

3. **Why not use Redis sorted sets for scheduling?** — Redis ZSET with ZPOPMIN is viable. I chose RabbitMQ TTL because: (a) RabbitMQ already handles the execution queue, (b) TTL + DLX gives native delay semantics, (c) Redis adds a separate data store to manage.

4. **What if RabbitMQ goes down?** — Tasks are persisted in PostgreSQL. The reconciliation worker detects unprocessed tasks and re-publishes them when RabbitMQ recovers. The source of truth is the database, not the queue.

5. **How does rate limiting work?** — Token bucket algorithm at the API gateway. Each authenticated user gets a rate limit. Implemented as middleware before any business logic.

6. **What's the bottleneck at 10K concurrent tasks?** — RabbitMQ ingestion was the bottleneck (74 msg/s ingest). Workers drained at 50-61 tasks/s. Could improve with batch publishing and more workers.

7. **How would you add cron expression support?** — Parse the cron expression, compute the next execution time, schedule with that TTL. After execution, compute the next occurrence and re-schedule.

8. **How do you handle task timeouts?** — Workers have a configurable execution timeout. If exceeded, the task is marked failed and optionally retried with exponential backoff.

9. **What's in your Grafana dashboard?** — Queue depth over time, worker throughput (tasks/sec), p50/p95/p99 latency, DLQ message count, worker health status (up/down), error rate.

10. **How would you make this multi-tenant?** — Add tenant ID to every task. Separate RabbitMQ queues per tenant for isolation. Rate limit per tenant. Tenant-scoped API keys.

11. **What's exponential backoff?** — Retry delays increase exponentially: 1s, 2s, 4s, 8s... with optional jitter to avoid thundering herd. Prevents overloading a failing downstream service.

---

## E.2 WebChat — Distributed Real-Time Chat Platform ⭐

### 30-Second Introduction
> "I built a distributed real-time chat platform with 6 microservices. It supports messaging, file uploads, and peer-to-peer video calling. Messages sync across multiple pods so the system can autoscale without dropping connections. I found and fixed two critical race conditions that were silently dropping and duplicating messages."

### 2-Minute Deep Dive
> "The platform has six services: an auth service for login and token management, a chat service handling Socket.io connections and real-time messaging, a persistence service for async message storage, a search service powered by Elasticsearch, a file service for uploads to S3-compatible storage, and a signaling service for WebRTC video calls.
>
> The main architectural challenge was scaling Socket.io across multiple pods. I used Redis as a pub-sub adapter — when a user connected to Pod A sends a message to someone on Pod B, the message flows through Redis pub-sub to reach the right pod. This lets Kubernetes scale the chat service horizontally.
>
> I decoupled message persistence from the real-time path using an async pipeline through RabbitMQ. The chat service publishes messages to the queue, and the persistence service consumes and writes to MongoDB. This keeps the WebSocket path fast.
>
> During load testing, I discovered two race conditions: one where concurrent Socket.io events caused messages to be dropped because of a shared mutable state issue, and another where retry logic in the persistence pipeline was duplicating messages. I fixed the first with proper event serialization and the second with an idempotency key.
>
> I also replaced an unsafe auto-migration setup — where the app ran migrations on every startup — with a version-controlled migration system. This fixed 9 deployment defects that were silently breaking production, like schema conflicts when multiple pods started simultaneously.
>
> Security includes JWT-based auth with token revocation on logout, rate limiting, XSS protection, and secret validation. The CI suite has 22 automated tests gated before every release."

### Architecture

```
[React Client]
    ↕ Socket.io / HTTPS
[API Gateway / Load Balancer]
    ↓
[Auth Service] ←→ [PostgreSQL] (users, sessions)
[Chat Service] ←→ [Redis Pub/Sub] (cross-pod sync)
    ↓ (async via RabbitMQ)
[Persistence Service] → [MongoDB] (messages)
[Search Service] ←→ [Elasticsearch] (message search)
[File Service] → [S3/MinIO] (file uploads)
[Signaling Service] ←→ [WebRTC] (video calls)

Infra: Kubernetes (multi-pod), Docker, GitHub Actions CI
```

### Follow-Up Questions

1. **Describe the two race conditions.** — (1) Shared mutable state in Socket.io event handlers: concurrent emit/receive on same connection caused messages to interleave and drop. Fixed with event serialization per connection. (2) RabbitMQ consumer retry: if processing failed and message was requeued, it could be consumed twice. Fixed with idempotency key (message ID) — check before insert.

2. **Why PostgreSQL AND MongoDB?** — PostgreSQL for structured relational data (users, auth, sessions) where ACID matters. MongoDB for messages which are write-heavy, document-shaped, and don't need complex joins.

3. **How does WebRTC signaling work?** — The signaling service exchanges SDP offers/answers and ICE candidates between peers via WebSocket. Once the peer connection is established, video/audio flows directly peer-to-peer, not through the server.

4. **What was the auto-migration problem?** — Multiple pods starting simultaneously each tried to run migrations, causing race conditions on schema changes. One pod might see a half-migrated schema. Fixed by running migrations as a separate Kubernetes Job before deploying app pods.

5. **How does token revocation work?** — On logout, the JWT is added to a Redis blacklist with TTL equal to the token's remaining lifetime. Every authenticated request checks the blacklist.

6. **What if Redis goes down?** — Short term: messages only reach users on the same pod. I'd add health checks and Redis Sentinel/Cluster for HA. The system degrades gracefully — messages are buffered locally.

7. **How would you add read receipts?** — Emit a "read" event via Socket.io. Persist read status in MongoDB per message per user. Fan out read receipt to sender via Redis pub-sub.

8. **What's ImagePullBackOff?** — Kubernetes can't pull the container image — usually wrong image name, missing credentials for private registry, or registry is down. Debug: `kubectl describe pod`, check `Events` section, verify imagePullSecrets.

9. **How do you handle message ordering?** — Messages are timestamped server-side. Within a single chat, the persistence service processes messages in queue order. Clients sort by timestamp.

10. **How would you add end-to-end encryption?** — Client-side encryption using Diffie-Hellman key exchange. The server never sees plaintext. This breaks server-side search — would need client-side search or searchable encryption.

---

## E.3 Hermes — Event-Sourced Workflow Orchestration ⭐

### 30-Second Introduction
> "Hermes is a multi-tenant workflow orchestration platform using event sourcing and CQRS. Instead of storing mutable state, every state change is an immutable event — making the full execution history replayable and auditable. It uses the transactional outbox pattern to guarantee reliable event delivery, and the saga pattern for distributed compensation and rollback."

### 2-Minute Deep Dive
> "The core idea is event sourcing: instead of updating a workflow's status in place, every transition — started, step-completed, step-failed, compensated — is appended as an immutable event to DynamoDB. The current state is derived by replaying events from the beginning or from a checkpoint.
>
> CQRS separates the write model — which only appends events — from the read model, which is a materialized projection optimized for queries. This means writes are fast appends and reads can be denormalized for the specific query patterns.
>
> The transactional outbox pattern solves the dual-write problem: when a service needs to both update its database and publish an event, there's a risk one succeeds and the other fails. Instead, the event is written to an outbox table in the same database transaction. A separate process polls the outbox and publishes to EventBridge. This guarantees atomicity.
>
> For multi-step workflows, I implemented the saga pattern. If step 3 of a 5-step workflow fails, compensation actions automatically roll back steps 2 and 1 in reverse order. Each step defines its own compensation logic.
>
> The time-travel replay engine can rebuild workflow state from any checkpoint. This is useful for debugging — you can replay events up to a specific point to see exactly what the state was.
>
> Mid-project, I migrated core services from a Spring Boot JVM-based architecture to AWS serverless — Lambda functions, Step Functions, API Gateway. This removed 100+ files of infrastructure code. I also found and fixed a deployment pipeline bug where Terraform was silently skipping a critical service on every release due to a misconfigured dependency graph."

### Key Concepts to Explain

| Concept | Simple Explanation |
|---------|-------------------|
| Event Sourcing | Store every state change as an immutable event. Current state = replay all events. |
| CQRS | Separate write operations (commands) from read operations (queries). Different models for each. |
| Transactional Outbox | Write event to outbox table in same DB transaction as business data. Separate process publishes to message bus. Prevents dual-write failures. |
| Saga Pattern | Multi-step distributed transaction. Each step has a compensation (undo). On failure, compensate in reverse order. |
| Time-Travel Replay | Rebuild state by replaying events from a checkpoint. Useful for debugging, auditing, and recovery. |

### Follow-Up Questions

1. **Why event sourcing over traditional CRUD?** — Full audit trail, temporal queries ("what was the state at 3pm?"), no data loss from updates, natural fit for distributed systems. Trade-off: more complex reads (need projections), larger storage.

2. **How do you handle event schema evolution?** — Version events. New consumers handle both v1 and v2. Use upcasting to transform old events to new schema during replay.

3. **What if the outbox poller crashes?** — Events stay in the outbox table (persistent). When the poller restarts, it picks up unprocessed events. Downstream consumers must be idempotent.

4. **How does saga compensation handle partial failures?** — Each step registers a compensating action. On failure, the saga coordinator calls compensations in reverse order. If a compensation itself fails, it's retried with backoff. After max retries, it's flagged for manual intervention.

5. **Why did you migrate from JVM to serverless?** — Reduced operational overhead (no server management), pay-per-use cost model, faster deployments. Trade-off: cold starts, 15-min Lambda timeout, less control over runtime.

6. **What was the deployment pipeline bug?** — Terraform's dependency graph had a missing `depends_on` for one service module. It was being planned but not applied because Terraform saw no state change. Fixed by adding explicit dependency and running `terraform apply -target` to initialize it.

7. **What is DynamoDB and why use it here?** — AWS managed NoSQL database. Event sourcing needs fast, append-heavy writes with partition-key-based reads — DynamoDB's design is perfect for this. The partition key is the workflow ID, sort key is the event sequence number.

8. **How does multi-tenancy work?** — Tenant ID is embedded in every event's partition key. IAM policies restrict access to tenant-specific data. Shared infrastructure but logically isolated data.

9. [NEEDS VERIFICATION] **Is the time-travel engine fully implemented?** — The resume mentions it but verify the current implementation status and any limitations.

10. [NEEDS VERIFICATION] **How many workflows has this handled in production?** — Verify if this has been deployed to production or is a portfolio/learning project.

---

## E.4 EngineCore Pro (Internship) ⭐

### 30-Second Introduction
> "During my internship at S.N. Engineering Works, I built EngineCore Pro — a B2B machinery marketplace from scratch. It lets buyers browse, filter, and configure precision-engineering products through a React and Supabase platform. I replaced their manual email-based quoting with a custom configurator that generates structured quote requests instantly. After my internship ended, the company asked me to build an internal ticketing module — which I did, demonstrating ownership beyond the original scope."

### Follow-Up Questions

1. **Why Supabase?** — Supabase provides PostgreSQL, auth, storage, and real-time subscriptions in one platform. For a startup project with a small team, it reduced infrastructure setup time significantly compared to self-hosting.

2. **How does the project configurator work?** — Buyers select product categories, input specifications (dimensions, material, tolerance), and submit. The system creates a structured lead in the database with all specs, auto-assigns it to the relevant sales team member, and tracks status through the pipeline.

3. **What was the lead pipeline?** — Supabase table with status enum (new → contacted → quoted → negotiating → won/lost). Dashboard shows pipeline metrics. Status transitions trigger notifications.

4. **What does the ticketing module do?** — Internal support tickets for customers post-sale. Create, assign, prioritize, track resolution. Built at the company's request after my internship ended.

5. **What did this teach you?** — End-to-end ownership from blank repo to production. Stakeholder communication — translating business needs into technical requirements. The importance of shipping quickly and iterating.

---

# F. Core CS Fundamentals

> Concise, interview-ready. 3-5 sentences per topic max.

---

## F.1 DBMS

### Keys
| Key Type | Definition |
|----------|-----------|
| Primary Key | Uniquely identifies each row. Cannot be NULL. One per table. |
| Foreign Key | References primary key of another table. Enforces referential integrity. |
| Candidate Key | Any column(s) that could be the primary key. |
| Composite Key | Primary key made of 2+ columns together. |
| Super Key | Any set of columns that uniquely identifies rows (includes candidate keys + extras). |

### Normalization
| Form | Rule | Example Violation |
|------|------|-------------------|
| 1NF | Atomic values only, no repeating groups | Column "phones" = "123,456" |
| 2NF | 1NF + no partial dependency on composite key | Non-key depends on part of composite key |
| 3NF | 2NF + no transitive dependency | A→B→C means C transitively depends on A |
| BCNF | Every determinant is a candidate key | Stricter than 3NF |

### Joins
| Join | Returns |
|------|---------|
| INNER | Only matching rows from both tables |
| LEFT | All rows from left + matching from right (NULL if no match) |
| RIGHT | All rows from right + matching from left |
| FULL OUTER | All rows from both, NULL where no match |
| CROSS | Cartesian product (every row × every row) |

### ACID
- **Atomicity**: All or nothing. Transaction either fully completes or fully rolls back.
- **Consistency**: DB moves from one valid state to another. Constraints are maintained.
- **Isolation**: Concurrent transactions don't interfere with each other.
- **Durability**: Once committed, data survives crashes (written to disk/WAL).

### Isolation Levels
| Level | Dirty Read | Non-repeatable Read | Phantom Read |
|-------|-----------|-------------------|-------------|
| Read Uncommitted | ✅ | ✅ | ✅ |
| Read Committed | ❌ | ✅ | ✅ |
| Repeatable Read | ❌ | ❌ | ✅ |
| Serializable | ❌ | ❌ | ❌ |

### Indexes
- **B-tree index**: Default. Good for range queries and equality. O(log N) lookup.
- **Hash index**: O(1) equality lookup only, no range queries.
- Trade-off: Indexes speed reads but slow writes (must update index on insert/update/delete).

### SQL vs NoSQL
| Aspect | SQL (PostgreSQL) | NoSQL (MongoDB) |
|--------|-----------------|-----------------|
| Schema | Fixed, enforced | Flexible, schema-less |
| Joins | Native | Manual (embed or reference) |
| Scaling | Vertical (read replicas) | Horizontal (sharding) |
| ACID | Full | Document-level (MongoDB 4.0+ has multi-doc) |
| Best for | Complex queries, relationships | Write-heavy, flexible documents |

### Deadlocks
- **What**: Two transactions each hold a lock the other needs → both wait forever.
- **Prevention**: Lock ordering (always acquire locks in same order), timeouts, deadlock detection with victim selection.
- **In PostgreSQL**: Automatic detection → one transaction is rolled back.

---

## F.2 Operating Systems

### Process vs Thread
| Aspect | Process | Thread |
|--------|---------|--------|
| Memory | Own address space | Shared address space |
| Creation | Heavyweight (fork) | Lightweight |
| Communication | IPC (pipes, sockets, shared mem) | Shared variables |
| Crash impact | Other processes unaffected | Crash kills all threads in process |

### Context Switching
Saving the state (registers, program counter, stack pointer) of current process/thread and loading the state of the next one. Cost: ~1-10 microseconds. Involves flushing CPU caches, making it expensive.

### Virtual Memory
OS gives each process the illusion of a large contiguous address space. Uses paging: virtual pages map to physical frames via page table. **Page fault**: requested page not in RAM → OS loads from disk (very slow, ~10ms).

### Stack vs Heap
| Aspect | Stack | Heap |
|--------|-------|------|
| Allocation | Automatic (function scope) | Manual (new/malloc) |
| Speed | Very fast (pointer bump) | Slower (fragmentation) |
| Size | Limited (~1-8 MB) | Large (limited by RAM) |
| Deallocation | Automatic (function returns) | Manual (delete/free) or GC |

### Mutex vs Semaphore
- **Mutex**: Binary lock. Only one thread enters critical section. Owner must unlock.
- **Semaphore**: Counter-based. Allows N concurrent accesses. sem_wait() decrements, sem_post() increments.
- **When to use**: Mutex for mutual exclusion (1 at a time). Semaphore for resource pools (N connections).

### Deadlock (Coffman Conditions — ALL 4 must hold)
1. **Mutual exclusion** — resource held exclusively
2. **Hold and wait** — hold one, wait for another
3. **No preemption** — can't forcibly take a resource
4. **Circular wait** — A waits for B, B waits for A

**Prevention**: Break ANY one condition. Most practical: impose lock ordering (breaks circular wait).

### Race Condition
Two threads access shared data concurrently, and at least one writes. Result depends on execution order. Fix: mutexes, atomic operations, or lock-free data structures.

---

## F.3 Computer Networks

### OSI Model (7 Layers)
| Layer | Name | Purpose | Protocol Examples |
|-------|------|---------|-------------------|
| 7 | Application | User interface | HTTP, FTP, SMTP, DNS |
| 6 | Presentation | Data format, encryption | SSL/TLS, JPEG |
| 5 | Session | Session management | NetBIOS |
| 4 | Transport | Reliable delivery | TCP, UDP |
| 3 | Network | Routing | IP, ICMP |
| 2 | Data Link | Framing, MAC | Ethernet, WiFi |
| 1 | Physical | Bits on wire | Cables, signals |

### TCP vs UDP
| Aspect | TCP | UDP |
|--------|-----|-----|
| Connection | Connection-oriented (3-way handshake) | Connectionless |
| Reliability | Guaranteed delivery, ordering, retransmission | Best-effort, no guarantees |
| Speed | Slower (overhead) | Faster (no overhead) |
| Use case | HTTP, file transfer, email | Video streaming, DNS, gaming |

### TCP 3-Way Handshake
1. Client → Server: **SYN** (seq=x)
2. Server → Client: **SYN-ACK** (seq=y, ack=x+1)
3. Client → Server: **ACK** (ack=y+1)

Connection established. Both sides have confirmed they can send and receive.

### DNS Resolution
Browser → Local DNS cache → Recursive resolver → Root server (.com) → TLD server (example.com) → Authoritative server → IP address returned → cached at each level.

### HTTP vs HTTPS
HTTPS = HTTP + TLS encryption. TLS provides: encryption (confidentiality), authentication (server certificate), integrity (MAC). Uses port 443 vs 80.

### REST API Status Codes
| Code | Meaning |
|------|---------|
| 200 | OK — success |
| 201 | Created — resource created |
| 400 | Bad Request — client error |
| 401 | Unauthorized — not authenticated |
| 403 | Forbidden — authenticated but not authorized |
| 404 | Not Found |
| 409 | Conflict — resource conflict |
| 500 | Internal Server Error |

### Authentication vs Authorization
- **Authentication**: Who are you? (login, JWT, API key)
- **Authorization**: What can you do? (roles, permissions, RBAC)

### Load Balancing
Distributes traffic across multiple servers. Algorithms: Round Robin (simple rotation), Least Connections (send to least busy), IP Hash (sticky sessions). Reverse proxy (e.g., Nginx) sits in front of servers, forwards requests, hides internal topology.

---

## F.4 Software Engineering

### SDLC Phases
Requirements → Design → Implementation → Testing → Deployment → Maintenance

### Agile vs Waterfall
| Aspect | Agile | Waterfall |
|--------|-------|-----------|
| Approach | Iterative, incremental | Sequential, linear |
| Flexibility | Embraces change | Rigid after requirements |
| Delivery | Working software every sprint | At the end |
| Best for | Evolving requirements | Well-defined, fixed scope |

### Scrum
- **Sprint**: 2-4 week iteration
- **Daily Standup**: 15 min — what I did, what I'll do, blockers
- **Sprint Retrospective**: What went well, what to improve

### Git Workflow
- **Feature branch**: Branch per feature, merge to main via PR
- **Rebase vs Merge**: Rebase = linear history; Merge = preserves branch history
- **PR review**: At least 1 approval before merge

### Testing Types
| Type | Scope | Speed | Example |
|------|-------|-------|---------|
| Unit | Single function/class | Fast | Test a sorting function |
| Integration | Multiple components | Medium | Test API + database |
| E2E | Entire system | Slow | Simulate user flow in browser |

### CI/CD
- **CI (Continuous Integration)**: Automatically build and test on every commit
- **CD (Continuous Deployment)**: Automatically deploy to production after tests pass
- Purpose: Catch bugs early, deploy frequently, reduce manual work
