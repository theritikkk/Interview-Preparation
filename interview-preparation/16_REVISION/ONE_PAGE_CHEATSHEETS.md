# One-Page Cheatsheets

Ultra-condensed, one line per fact — for a final scan, not for learning something new. Full explanations live in each topic's own file.

---

## Programming
- Stack: automatic, scope-bound. Heap: manual, programmer-owned lifetime. UB ≠ crash.
- Lock-free ≠ wait-free. ABA problem: value changes and changes back, CAS can't detect without tags/hazard pointers.
- Java `equals()`/`hashCode()`: must override both together or `HashMap`/`HashSet` silently breaks.
- Python: object references passed by value; mutable default args are created once, shared across calls.
- JS: sync code → all microtasks → next macrotask, always in that order.
- TS: structural typing (shape matters, not name); types erased at compile time, no runtime existence.
- SQL: `FOR UPDATE SKIP LOCKED` = safe concurrent row-claiming; MVCC = readers never block writers.

## DSA
- Two Pointers: sorted array, converge inward. Sliding Window: contiguous + "at most K." Binary Search: monotonic feasibility, not just sorted arrays.
- Monotonic Stack: O(n) amortized despite the nested-looking while loop (each element pushed/popped once).
- DP: identify state correctly first; can you drop a dimension for O(1) space?
- Union-Find: near-O(1) amortized with path compression + union by rank.

## CS Fundamentals
- Deadlock needs all 4 conditions (mutual exclusion, hold-and-wait, no preemption, circular wait) — break one to prevent it.
- `mmap` + `TPACKET_V3` = NetSight's zero-copy mechanism.
- B+ tree > B-tree for DB indexes: linked leaves enable range scans.
- Four OOP pillars: encapsulation, abstraction, inheritance, polymorphism. SOLID's "D" → Dependency Injection is the technique.

## Backend / Frontend
- Idempotent HTTP methods: GET, PUT, DELETE. Not: POST. PATCH: depends.
- Idempotency keys make non-idempotent operations retry-safe.
- React `useEffect`: `[]` once on mount, omitted every render, `[x]` when x changes. Stale closures = missing dependency.
- `key` prop must be a stable per-item ID, never array index, when list can reorder.

## Distributed Systems (🔴 highest-yield section)
- CAP: partition tolerance isn't optional in practice — the real choice is C vs A during a partition.
- At-least-once delivery + idempotent processing = correct system. "Exactly-once delivery" as a broker property is a myth for both RabbitMQ and SQS.
- Event Sourcing: events are truth, state is derived. CQRS: separate write model (commands) from read model (queries).
- Transactional Outbox: solve the dual-write problem by writing to only ONE system atomically, deriving the notification from the committed write (e.g., DynamoDB Streams), not a second failable call.
- Optimistic concurrency: version-tagged conditional write; store guarantees exactly one winner on a conflicting write.

## AWS
- Lambda: init code outside the handler = cold-start-only, reused on warm invocations. JVM Lambdas have worse cold starts.
- DynamoDB: partition key = distribution, sort key = ordering within a partition. Hot partition = uneven key distribution.
- SQS: at-least-once, not exactly-once. Visibility timeout = redelivery mechanism on consumer failure.
- Cognito: JWT verified locally (cached public keys), no per-request call back to Cognito. TenantId must come from the verified claim, never client-supplied data.
- Step Functions = orchestration (explicit, directive). EventBridge = choreography (reactive, decoupled).

## DevOps / Observability
- Docker layer caching: order Dockerfile instructions rarely-changing → frequently-changing.
- Terraform: state = belief about reality; `plan` = diff; `apply` = execute.
- Prometheus: pull-based scraping. Counter (monotonic) vs Gauge (up/down) vs Histogram (enables percentiles).
- p95/p99 latency needs a histogram — not a gauge or counter.

## Security
- Passwords: slow, salted hash (bcrypt/argon2) — never plaintext, never a fast general hash alone.
- JWT: stateless, hard revocation. Session: stateful, instant revocation.
- SQLi: parameterized queries, never string-concatenate input into SQL.
- CORS is browser-enforced, not a substitute for server-side auth.

## Your Three Projects — One-Line Defenses
- **Hermes:** know your actual six read models by name — the single highest-risk generic-answer trap.
- **Task Scheduler:** "exactly-once" = at-least-once delivery + idempotent claiming, not a broker guarantee — state it in that order.
- **NetSight:** zero-copy = one specific eliminated copy (kernel→mmap'd shared memory), state it precisely; verify your actual ABA mitigation before claiming one.
