# Interview Preparation Roadmap — 22 Phases

This is the learning sequence, not the deep content itself. Each phase below becomes a full file (or folder of files) with the format specified in the master prompt (What/Why/How/Example/Interview Questions/Follow-ups/Mistakes/Resume Connection/Exercises) once we generate it. This file exists so you can see and approve the whole plan before any of that deep content gets written.

**Legend:** 🔴 High priority for your resume · 🟡 Standard priority · Resume Connection shows which of your artifacts each phase defends.

---

## Phase 1 — Programming Fundamentals 🔴
**Covers:** C, C++, Java, Python, JavaScript, TypeScript, SQL — language internals, not syntax.
**You need to understand:** memory model differences across languages (manual in C/C++, GC in Java, refcounted+GC in Python, engine-managed in JS); why TypeScript's type system is erased at runtime; SQL as a declarative query language vs. procedural code.
**Resume connection:** every project — this is the substrate under Hermes (TS/Java/Python), NetSight (C), Task Scheduler (Node/Python/SQL).
**Prerequisites:** none — this is foundation.
**Output file(s):** `01_PROGRAMMING/{C,CPP,JAVA,PYTHON,JAVASCRIPT,TYPESCRIPT,SQL}.md`

## Phase 2 — DSA 🔴
**Covers:** patterns-first approach across arrays, strings, hashing, two pointers, sliding window, binary search, trees, heaps, graphs, DP, bit manipulation, tries.
**You need to understand:** pattern recognition (signals that tell you which technique applies), not memorized solutions.
**Resume connection:** indirect — this is the standard screening gate for every company type listed in your target roles, independent of your projects.
**Prerequisites:** Phase 1 (comfort in at least one language).
**Output file(s):** `02_DSA/{PATTERNS,ARRAYS,STRINGS,TREES,GRAPHS,DP,...}.md`

## Phase 3 — Operating Systems 🟡
**Covers:** processes/threads, scheduling, synchronization primitives (mutex/semaphore/condition variable), deadlock, virtual memory, paging, TLB, system calls, epoll, I/O models.
**You need to understand:** this is the theory layer under NetSight's concurrency work (lock-free structures only make sense once you understand what locks solve and cost) and Task Scheduler's worker-process model.
**Resume connection:** 🔴 NetSight (concurrency, memory) · Task Scheduler (worker processes)
**Prerequisites:** Phase 1.
**Output file:** `03_CS_FUNDAMENTALS/OS.md`

## Phase 4 — Computer Networks 🟡
**Covers:** OSI/TCP-IP stack, TCP vs UDP, DNS, HTTP/1.1–HTTP/3, TLS, WebSockets, gRPC, load balancers, reverse proxies, NAT, latency/packet loss/retransmission.
**Resume connection:** 🔴 NetSight (you built a packet-capture engine — this phase is non-negotiable depth for you specifically, more than for a typical candidate) · general API/backend work everywhere else.
**Prerequisites:** Phase 3 helpful but not required.
**Output file:** `03_CS_FUNDAMENTALS/NETWORKING.md`

## Phase 5 — DBMS & SQL 🔴
**Covers:** relational model, ACID, isolation levels, MVCC, locking, indexing (B-tree/B+tree), replication, sharding, plus hands-on SQL (joins, window functions, CTEs, EXPLAIN).
**Resume connection:** 🔴 Task Scheduler (PostgreSQL under concurrent load, row-count convergence claims) · 🔴 Hermes (DynamoDB — a deliberate contrast case: no ACID transactions across items the way Postgres has them, why that tradeoff was chosen).
**Prerequisites:** Phase 1 (SQL).
**Output file:** `03_CS_FUNDAMENTALS/DBMS.md` + `06_DATABASES/*.md`

## Phase 6 — OOP & Design Principles 🟡
**Covers:** encapsulation/abstraction/inheritance/polymorphism, composition over inheritance, SOLID, dependency injection, common design patterns.
**Resume connection:** relevant to Java portions of Hermes and to machine-coding rounds generally.
**Output file:** `03_CS_FUNDAMENTALS/OOP.md`

## Phase 7 — Backend Development 🔴
**Covers:** Node.js/Express (middleware, routing, auth, rate limiting), FastAPI/Pydantic, Spring Boot (controllers/services/repositories/JPA), REST design, background jobs, connection pooling.
**Resume connection:** 🔴 Hermes and Task Scheduler backend layers · Spring Boot section double-checked against the exposure-risk flag in the Resume Analysis (§9) — taught at a "confident fundamentals" level appropriate to an unlisted-project skill.
**Prerequisites:** Phase 1, Phase 6.
**Output file(s):** `04_BACKEND/{NODE,EXPRESS,FASTAPI,SPRING_BOOT,REST,WEBSOCKETS,GRPC}.md`

## Phase 8 — Frontend Development 🟡
**Covers:** React fundamentals, hooks, rendering/reconciliation, state management, performance (code splitting, memoization), auth/CORS/XSS/CSRF from the frontend side.
**Resume connection:** 🔴 S.N. Engineering Works internship (your only React project) — this phase is where that project gets its technical backbone.
**Output file(s):** `05_FRONTEND/*.md`

## Phase 9 — APIs & Distributed Communication 🟡
**Covers:** REST constraints, idempotency, pagination/versioning, gRPC/protobuf, retries/timeouts/circuit breakers/backpressure.
**Resume connection:** ties Hermes's Step Functions orchestration and Task Scheduler's API gateway together.
**Output file:** folded into `04_BACKEND/REST.md`, `07_DISTRIBUTED_SYSTEMS/`

## Phase 10 — Distributed Systems Fundamentals 🔴
**Covers:** consistency models, CAP-adjacent tradeoffs, idempotency, exactly-once vs. at-least-once vs. effectively-once semantics, event-driven architecture, outbox pattern, event sourcing/CQRS theory.
**Resume connection:** 🔴🔴 This is the theory phase that makes Hermes and Task Scheduler *defensible* rather than *describable*. The "exactly-once" rigor issue flagged in the Resume Analysis gets its full treatment here.
**Prerequisites:** Phase 3, Phase 5.
**Output file(s):** `07_DISTRIBUTED_SYSTEMS/*.md`

## Phase 11 — Cloud & AWS 🔴
**Covers:** Lambda, DynamoDB, EventBridge, Step Functions, SQS, Cognito, X-Ray, IAM/least-privilege, plus the AWS services on your skills list without project evidence (EC2, S3, API Gateway) taught at a "know it, don't overclaim it" depth.
**Resume connection:** 🔴 Hermes end-to-end.
**Prerequisites:** Phase 10.
**Output file(s):** `08_AWS/*.md`

## Phase 12 — Docker, Kubernetes, Terraform, CI/CD 🟡
**Covers:** images/containers/networking/volumes, K8s objects (pod/deployment/service/ingress), Terraform state/plan/apply, GitHub Actions pipeline stages.
**Resume connection:** 🔴 Task Scheduler (Docker) and Hermes (Terraform, 18 modules) confirmed; Kubernetes and GitHub Actions taught at exposure-risk-aware depth (see §9 of Resume Analysis).
**Output file(s):** `09_DEVOPS/*.md`

## Phase 13 — Databases & Caching Deep Dive 🔴
**Covers:** PostgreSQL internals, DynamoDB partition/sort keys and single-table design, Redis patterns (cache-aside, TTL, eviction), MongoDB document model — the latter two taught conceptually per the exposure-risk flag.
**Resume connection:** 🔴 Task Scheduler, 🔴 Hermes.
**Output file(s):** `06_DATABASES/*.md`

## Phase 14 — Messaging & Event-Driven Systems 🔴
**Covers:** RabbitMQ (exchanges, TTL, dead-lettering, ack/nack), SQS, EventBridge, and — since it will likely come up as a comparison question even though it's not on your resume — enough Kafka theory (partitions, consumer groups, offsets) to answer "why not Kafka?" credibly.
**Resume connection:** 🔴🔴 Task Scheduler (RabbitMQ is the architectural core) · 🔴 Hermes (SQS/EventBridge in the Outbox chain).
**Output file:** `07_DISTRIBUTED_SYSTEMS/MESSAGING.md`

## Phase 15 — Security 🟡
**Covers:** JWT/OAuth/Cognito, RBAC, password hashing, SQL injection/XSS/CSRF/SSRF, secrets management, tenant isolation, rate limiting.
**Resume connection:** 🔴 Hermes (Cognito JWT tenant isolation + RBAC is a named architectural component — expect deep questioning here).
**Output file(s):** `12_SECURITY/*.md`

## Phase 16 — Observability & Production Debugging 🔴
**Covers:** logs/metrics/traces, RED/USE methods, latency percentiles (p50/p95/p99), alerting, and structured incident-diagnosis reasoning (not guessing).
**Resume connection:** 🔴 Task Scheduler (Prometheus/Grafana/Loki/Alertmanager, the p95 160ms→76ms recovery claim) · 🔴 Hermes (X-Ray) · 🔴 NetSight (Prometheus export).
**Output file(s):** `10_OBSERVABILITY/*.md`

## Phase 17 — System Design 🔴
**Covers:** requirements → scale estimation → API → data model → architecture → bottlenecks → failure modes, applied to URL shortener, chat app, notification system, rate limiter, job scheduler, event-driven workflow engine (the last two map directly onto your own projects).
**Resume connection:** designing "a job scheduler" or "an event-driven workflow engine" from scratch is essentially re-deriving your own two backend projects — huge synergy.
**Prerequisites:** Phases 5, 10, 11, 14.
**Output file(s):** `11_SYSTEM_DESIGN/*.md`

## Phase 18 — Project Deep Dives 🔴🔴🔴 (Highest priority)
**Covers:** Hermes, Distributed Task Scheduler, NetSight, and the internship — each pushed to whiteboard-level, "explain like the interviewer read your code" depth, with every architectural choice justified via Why/Alternatives/Failure modes/Scaling/Security/Cost, and every quantitative claim given a full defense sheet (Phase 22 concept, applied here).
**Resume connection:** this phase *is* your resume.
**Prerequisites:** Phases 1, 3, 5, 10, 11, 14, 16.
**Output file(s):** `13_PROJECT_DEEP_DIVES/*.md`

## Phase 19 — Machine Coding 🟡
**Covers:** parking lot, rate limiter, splitwise, elevator, notification service — built live, from requirements, using OOP/SOLID/clean architecture.
**Resume connection:** exercises the OOP (Phase 6) and backend (Phase 7) muscles directly.
**Output file:** `15_INTERVIEW_PREP/MACHINE_CODING.md`

## Phase 20 — Debugging Training 🟡
**Covers:** reading unfamiliar codebases systematically — entry point, request tracing, dependency mapping, hypothesis formation — applied to React/Node/Python/Java/SQL/Docker/K8s/distributed-systems/C bugs.
**Output file:** `15_INTERVIEW_PREP/DEBUGGING.md`

## Phase 21 — Behavioral / HR 🟡
**Covers:** STAR-formatted answers built only from things that actually happened (the internship's scope-ownership story, the Geek Room community-organizing angle, and honest framing for the exposure-risk skills from §9 of the Resume Analysis).
**Output file(s):** `14_BEHAVIORAL/*.md`

## Phase 22 — Mock Interviews 🔴
**Covers:** 12 mock interview sets (General SDE, Full Stack, Backend, React, Node, Java/Spring, Python/FastAPI, AWS, Distributed Systems, Resume Deep Dive, Systems/Linux/C, Final Mixed) — run interactively, one question at a time, with follow-ups that escalate based on your answers.
**Output file(s):** `15_INTERVIEW_PREP/MOCK_INTERVIEWS.md` (framework) — mocks themselves run live in conversation, not pre-written, since the escalation depends on your actual answers.

---

## Recommended Sequencing (given your 2027 graduation timeline and internship/entry-level target)

```
Phase 1 (Programming) ─┬─→ Phase 2 (DSA)                    ─┐
                        └─→ Phase 6 (OOP)                     │
Phase 3 (OS) ───────────┬─→ Phase 10 (Distributed Systems)    ├─→ Phase 18
Phase 4 (Networking) ───┤        ↑                            │  (PROJECT
Phase 5 (DBMS/SQL) ─────┴─→ Phase 11 (AWS)                    │  DEEP DIVES)
                             Phase 14 (Messaging) ─────────────┤   🔴🔴🔴
                             Phase 16 (Observability) ─────────┘
Phase 7 (Backend) + Phase 8 (Frontend) → Phase 9 (APIs)
Phase 12 (Docker/K8s/Terraform) + Phase 13 (DB/Caching) + Phase 15 (Security) — feed into 17 & 18
Phase 17 (System Design) — after 5, 10, 11, 14
Phase 19 (Machine Coding), Phase 20 (Debugging) — can run in parallel with anything above
Phase 21 (Behavioral) — anytime, low dependency
Phase 22 (Mock Interviews) — continuous, once each domain phase lands
```

DSA (Phase 2) and Project Deep Dives (Phase 18) are the two threads that should run continuously from day one rather than waiting their turn — everything else is in service of one of those two.
