# Project Deep Dive: Hermes — Serverless Workflow Orchestration Platform

🔴🔴🔴 Treat this as the single highest-priority document in the whole knowledge base. Every mechanism referenced here has its own deep-dive file — this document's job is to assemble them into one coherent, defensible narrative and give you the exact answers to the exact questions your resume invites.

---

## Architecture (Reconstructed From Resume Claims)

```
Client → API Gateway (?) → Lambda (TS/Java/Python, polyglot)
                               │
                               ├── Cognito JWT verification (COGNITO.md)
                               │     → tenantId + role claims extracted
                               │
                               ├── Command handling → conditional write (optimistic concurrency)
                               │     → DynamoDB Event Store (append-only)   [CONCURRENCY.md]
                               │
                               ├── DynamoDB Streams (change capture)         [08_AWS/DYNAMODB.md]
                               │     → SQS (Transactional Outbox target)     [SQS.md]
                               │     → EventBridge (fan-out)                 [EVENTBRIDGE.md]
                               │           ├──→ Read Model Updater #1..6 → 6 DynamoDB Read Model tables
                               │           └──→ Step Functions trigger (multi-step pipelines)
                               │
                               └── Step Functions (orchestration, X-Ray traced)  [STEP_FUNCTIONS.md, XRAY.md]

Infra: 18 Terraform modules · Tests: 285 (234 TS / 42 Java-JUnit5 / 9 Python-pytest), 51+ suites
```

Everything above the "Infra" line is reconstructed *from your resume's own claims* — cross-check each arrow against your actual code before an interview; anywhere this diagram guesses at a mechanism not explicitly stated in your resume, it's flagged `NEEDS VERIFICATION` below.

---

## Component-by-Component Justification

| Component | Why it exists | Why chosen (vs. alternative) | Failure mode | Scaling | Cost |
|---|---|---|---|---|---|
| **Lambda (polyglot)** | Compute, no server management | Serverless = pay-per-invocation, auto-scales; alternative = EC2/ECS (always-on cost, manual scaling) | Cold starts (worse for Java — `LAMBDA.md`); 15-min max execution | Auto-scales per invocation; can overwhelm downstream connection limits (`LAMBDA.md`) | Pay-per-invocation — cheap at low/spiky volume, can exceed always-on compute cost at very high sustained volume |
| **DynamoDB (event store)** | Durable, scalable append-only log | Native Streams (Outbox mechanism), predictable low-latency at scale; alternative = PostgreSQL (ACID, but the Task Scheduler already demonstrates that path — a deliberate polyglot-persistence choice) | Hot partition if event-store partition key is poorly chosen (`06_DATABASES/DYNAMODB.md`) | Scales near-linearly with good key design; poor key design caps throughput regardless of provisioned capacity | On-demand or provisioned RCU/WCU (`08_AWS/DYNAMODB.md`) |
| **Event Sourcing + CQRS** | Durable audit trail + read-optimized projections | Alternative = conventional CRUD (loses history, one model serves both read/write badly) | Read models eventually consistent — a query right after a write may miss it (`CONSISTENCY.md`) | Read models scale independently of the write path | 6x the storage/write-amplification of a single-model design — a real cost tradeoff |
| **Transactional Outbox (Streams→SQS→EventBridge)** | Solves the dual-write problem | Alternative = direct SQS publish after DynamoDB write (broken — can silently drop the notification on a crash, `EVENT_DRIVEN.md`) | SQS redelivery on consumer failure (idempotent processing required, `SQS.md`) | Scales with Streams' native throughput; SQS/EventBridge scale independently | Streams + SQS + EventBridge each bill separately — a real per-event cost stack, worth being honest about if asked about cost tradeoffs |
| **Step Functions** | Explicit multi-step orchestration with retry/catch | Alternative = hand-rolled orchestration in one Lambda (no visual debugging, bound by 15-min Lambda timeout, `STEP_FUNCTIONS.md`) | A misconfigured Catch can silently swallow a failure into the wrong branch | Not bound by a single Lambda's timeout — can span long-running workflows | Billed per state transition — can add up for very high-frequency, many-step workflows |
| **Cognito + JWT + RBAC** | Multi-tenant auth/authorization | Alternative = custom auth service (more code, more attack surface to maintain yourself) | A missed tenant-scoping check at the data-access layer is the single highest-severity possible bug in this whole system (`COGNITO.md`) | Stateless JWT verification scales trivially (no shared session store) | Cognito's per-MAU pricing — negligible until very large user bases |
| **Terraform (18 modules)** | Reproducible, version-controlled infra | Alternative = manual console clicks (undocumented, unreproducible, no drift detection, `09_DEVOPS/TERRAFORM.md`) | Drift if manual changes bypass Terraform | N/A (infra-as-code doesn't "scale" in the runtime sense) | Prevents costly manual-configuration mistakes more than it has a direct cost itself |

---

## The Named Questions From Your Own Prep Brief — Direct Answers

**"Why Event Sourcing?"** → Full audit trail as a structural property (not bolted on), and the ability to build new read models later by replaying history — see `EVENT_DRIVEN.md` Cluster 1.

**"Why CQRS?"** → Write and read access patterns are fundamentally different shapes; one model serving both compromises on both — see `EVENT_DRIVEN.md` Cluster 2.

**"Why DynamoDB?"** → Native Streams gives you the Transactional Outbox mechanism directly; predictable low-latency at scale for a key-based access pattern that event-sourced aggregates naturally have (fetch this aggregate's events, in order) — see `06_DATABASES/DYNAMODB.md`.

**"Why six read models?"** → Six distinct query patterns, each denormalized for its own access shape rather than one general-purpose model needing joins/filters at query time. **`NEEDS VERIFICATION`: you must be able to name what the six actually are** — this is the single most likely place a generic answer gets caught out.

**"How does optimistic concurrency work?"** → Version-tagged conditional writes; only one of N competing writers' expected-version check can match — see `CONCURRENCY.md` for the exact mechanism and the full follow-up chain.

**"How do you prevent concurrent writes?"** → You don't prevent them from *attempting* — you let them race and use the store's atomic conditional-write guarantee to ensure only one succeeds; the other 99 fail cleanly and can retry. Precision matters here: "prevent" is the wrong verb; "resolve safely" is correct.

**"How does the Transactional Outbox solve dual writes?"** → By writing to only one system (DynamoDB) atomically, then deriving the notification from the already-committed write (Streams) rather than making a second, independently-failable call — see `EVENT_DRIVEN.md` Cluster 3.

**"Why DynamoDB Streams → SQS → EventBridge?"** → Streams closes the dual-write gap; SQS adds durable, at-least-once, retryable delivery with a DLQ safety net; EventBridge adds pattern-matched fan-out to arbitrary future consumers without touching upstream code — each hop earns its place for a distinct reason, not redundantly.

**"What happens if SQS delivery fails?"** → Visibility timeout expires, message redelivers — the EventBridge-publish step is naturally retried; your downstream logic must be idempotent to handle a possible duplicate publish (see `SQS.md`).

**"What happens if a consumer crashes?"** → Depends on *when*: mid-processing before ack → message redelivers after visibility timeout, safe if idempotent. After processing but before ack → same outcome, same requirement. This is exactly why idempotency isn't optional anywhere in this chain.

**"How is idempotency achieved?"** `NEEDS VERIFICATION` — the standard mechanisms (an idempotency key stored and checked before reprocessing, or a naturally idempotent operation like a conditional/upsert write) are the expected answer shape; confirm which one your actual consumers use.

**"How does tenant isolation work?"** → Two independent layers: application-level (JWT tenantId claim scoping every data-access call) and, if implemented, IAM-level conditions on DynamoDB access — see `COGNITO.md` and `AWS_FUNDAMENTALS.md`. **Never let the answer imply the tenant ID comes from anywhere other than the verified token.**

**"How does Cognito JWT validation work?"** → Local signature verification against Cognito's public keys (JWKS, cached), no per-request network call back to Cognito — see `COGNITO.md` Cluster 1.

**"How does RBAC work?"** → Role claim extracted post-verification, checked against the specific action being attempted — a second, independent check from tenant-boundary enforcement, not the same check — see `COGNITO.md` Cluster 2.

**"Why Step Functions?"** → Explicit ordering + built-in retry/catch per step + not bound by a single Lambda's 15-minute timeout + visual execution history for debugging — see `STEP_FUNCTIONS.md`.

**"Why not Kafka?"** (not on your resume, but a certain comparison question) → Kafka is built for high-throughput log streaming with consumer-managed offsets and replay; your Outbox chain's actual requirement — reliable, ordered-enough, DLQ-backed delivery of discrete domain events with AWS-native integration to Lambda/Step Functions — is what SQS/EventBridge are purpose-built for, with far less operational overhead (no cluster to run/tune) than self-managing Kafka. This mirrors the Task Scheduler's "why RabbitMQ not Kafka" answer in shape, `MESSAGING.md`.

**"Why Lambda?"** → No server management, scales to zero, pay-per-invocation fits a workflow-orchestration platform's naturally spiky, event-triggered load pattern far better than provisioning always-on compute for unpredictable demand.

**"Why not Kubernetes?"** → K8s solves a different problem (orchestrating always-running containers) than a naturally event-driven, spiky workload needs — you'd be paying for and operating a cluster (control plane, node management, scaling policies) to replicate what Lambda gives you natively; a legitimate, honest tradeoff answer, not a dismissal of K8s's value in other contexts (see `09_DEVOPS/KUBERNETES.md`'s honest exposure-risk framing).

**"What are the tradeoffs of serverless?"** → Upside: no server management, auto-scaling, pay-per-use. Downside: cold starts (worse for JVM-based components), 15-minute execution ceiling, potential for Lambda fan-out to overwhelm a downstream resource's connection limit (`LAMBDA.md`), and less control over the exact runtime environment than a container/VM gives you — a fair, two-sided answer beats a one-sided sales pitch.

---

## Quantitative Claim Defense Sheet

### "100 concurrent append attempts, exactly 1 succeeds, 99 rejected with ConcurrencyException"
- **How was the test generated?** `NEEDS VERIFICATION` — likely 100 concurrent async calls (Promise.all-style, or a thread pool) all targeting the same aggregate/sequence.
- **What proves correctness, not just "no crash"?** The assertion that exactly 1 succeeded and 99 threw the specific exception type — not just that the test completed.
- **What would happen at 1,000 concurrent attempts?** Same mechanism, same guarantee (the conditional-write primitive doesn't degrade with more competitors) — but 999 wasted round-trips is a real efficiency question worth acknowledging (see `CONCURRENCY.md`'s optimistic-vs-pessimistic tradeoff under heavy contention).
- **How reproducible is this test?** Deterministic in outcome (exactly 1 winner, by design) even if *which* attempt wins isn't deterministic — a good distinction to draw if asked.

### "285 automated tests (234 TS, 42 Java/JUnit5, 9 Python/pytest), 51+ suites, 0 failures"
- **What do the language splits suggest?** TS carries the bulk of test coverage — plausibly the bulk of the application logic; Java and Python's smaller counts suggest narrower, more focused components. `NEEDS VERIFICATION`: know what those Java/Python components actually are.
- **Does "0 failures" mean 100% correctness?** No — it means the tests that exist all pass; it says nothing about untested code paths or edge cases the suite doesn't cover. Precision here (what a passing test suite does and doesn't prove) mirrors the Valgrind precision point in `C.md`.

### "18 Terraform modules"
- **What's the likely breakdown?** `NEEDS VERIFICATION` — a plausible structure given the architecture: modules for Lambda+IAM role bundles (possibly one per language runtime or per function group), DynamoDB tables (event store + 6 read models), SQS, EventBridge rules, Step Functions state machines, Cognito, networking/IAM baseline — know your actual breakdown, don't invent one under pressure.

### Quick Revision
- Six read models: know what they actually are — the single highest-risk generic-answer trap in this whole project.
- "Prevent concurrent writes" is imprecise — you resolve races safely via conditional writes, you don't prevent the attempt.
- Tenant ID authorization must always trace back to the verified JWT claim, never client-supplied data.
- Every "why X" question has a real alternative and a real cost/tradeoff — never present a choice as costless.
