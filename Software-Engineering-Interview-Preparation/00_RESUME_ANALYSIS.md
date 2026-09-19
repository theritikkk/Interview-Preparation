# Resume Analysis — Ritik Raj

Source: resume PDF as uploaded. B.Tech Information Technology, Maharaja Surajmal Institute of Technology, New Delhi (2023–2027), CGPA 8.6/10. Targeting internships and entry-level SWE roles.

This document is the complete inventory the rest of the knowledge base is built from. Nothing here is invented — every line traces back to a specific resume statement. Items the resume doesn't explain are explicitly marked.

---

## 1. Programming Languages

| Language | Where it appears | Expected interview depth | Notes |
|---|---|---|---|
| C | NetSight (primary language, ~9,700 LOC) | High — systems-level, memory, concurrency | Your deepest, most defensible language given NetSight's scope |
| C++ | Skills list only | Medium — fundamentals (OOP, STL, memory) expected, no project ties to it specifically | No project evidence — be ready to say so honestly if pushed |
| Java | Hermes (part of polyglot stack; 42 JUnit5 tests) | Medium-High | Resume doesn't say which Hermes components are Java — `NEEDS VERIFICATION` |
| Python | Hermes (9 pytest tests) + Distributed Task Scheduler (listed language) | Medium-High | Same caveat — which specific services are Python is unstated |
| JavaScript | Implied via Node.js/React usage | Medium | Foundation under TypeScript work |
| TypeScript | Hermes (primary: 234 of 285 tests), Distributed Task Scheduler | High | Your most heavily-tested language across projects |
| SQL | PostgreSQL usage (Distributed Task Scheduler) | Medium-High | Concurrency/locking questions likely given the scheduler's DB-under-load claims |

---

## 2. Frameworks & Libraries

| Item | Tied to a project bullet? | Risk if asked "tell me about your project using this" |
|---|---|---|
| React.js | ✅ Yes — Internship | Safe, you have a real story |
| Node.js | ✅ Yes — Hermes, Distributed Task Scheduler | Safe |
| Express.js | ⚠️ Skills list only, no project bullet names it explicitly | Likely used inside Node services but unconfirmed by resume text — `NEEDS VERIFICATION` |
| Spring Boot | ❌ Skills list only — **no project or experience bullet mentions it** | **High exposure risk.** See §4 |
| REST | ✅ Implied across all three projects (APIs, API gateway) | Safe conceptually |
| WebSockets | ❌ Skills list only, not tied to any bullet | Exposure risk |
| gRPC | ❌ Skills list only, not tied to any bullet | Exposure risk |
| WebRTC | ❌ Skills list only, not tied to any bullet | Exposure risk — this is a fairly specialized skill to list with zero project evidence; expect it to be probed if you keep it on your resume |

---

## 3. Databases & Messaging

| Item | Tied to a project bullet? | Notes |
|---|---|---|
| PostgreSQL | ✅ Yes — Distributed Task Scheduler (source of truth, row-count convergence claims) | Deep prep needed — this project makes specific concurrency/consistency claims about it |
| DynamoDB | ✅ Yes — Hermes (event store + Streams) | Deep prep needed — single-table design, streams, optimistic concurrency all implicated |
| RabbitMQ | ✅ Yes — Distributed Task Scheduler (TTL + dead-lettering) | Deep prep needed |
| SQS | ✅ Yes — Hermes (Transactional Outbox target) | Needed for Hermes deep dive |
| MongoDB | ❌ Skills list only | Exposure risk |
| Redis | ❌ Skills list only | Exposure risk — notable because caching questions are extremely common in backend interviews; you'll need conceptual fluency even without a project |
| Elasticsearch | ❌ Skills list only | Exposure risk |

---

## 4. Cloud (AWS)

| Service | Tied to a project bullet? | Notes |
|---|---|---|
| Lambda | ✅ Hermes | Core to serverless story |
| DynamoDB | ✅ Hermes | Core |
| EventBridge | ✅ Hermes (both as Outbox target and orchestration) | Core |
| Step Functions | ✅ Hermes | Core — "why Step Functions not raw Lambda orchestration" is a certain question |
| X-Ray | ✅ Hermes | Tracing story |
| Cognito | ✅ Hermes (JWT tenant isolation, RBAC) | Security story |
| EC2 | ❌ Skills list only | Not evidenced in any bullet — likely irrelevant to Hermes (which is serverless by design), so if asked "why not EC2" you actually have a strong *architectural* answer even without EC2 project experience |
| S3 | ❌ Skills list only | Same — probably used for something (static hosting? Terraform state?) but unconfirmed — `NEEDS VERIFICATION` |
| API Gateway | ❌ Skills list only, but Hermes is Lambda-based and almost certainly fronted by *something* — `NEEDS VERIFICATION`: confirm whether API Gateway actually fronts Hermes's Lambdas before claiming it in an interview |

---

## 5. DevOps / Infrastructure

| Item | Tied to a project bullet? | Notes |
|---|---|---|
| Docker | ✅ Distributed Task Scheduler | Confirmed |
| Terraform | ✅ Hermes ("18 Terraform modules") | Confirmed — IaC questions likely |
| Kubernetes | ❌ Skills list only — **no project uses it (Task Scheduler uses Docker, not stated as K8s-orchestrated)** | **High exposure risk.** See §4 below the table |
| GitHub Actions | ❌ Skills list only | Exposure risk |

---

## 6. Systems / Linux Tools

| Item | Tied to a project bullet? |
|---|---|
| Linux | ✅ NetSight (kernel 5.4–6.x targeting) |
| eBPF, BTF, CO-RE | ✅ NetSight |
| AF_PACKET, TPACKET_V3 | ✅ NetSight |
| Lock-free data structures (C11 atomics, Treiber stack) | ✅ NetSight |
| pthreads | ✅ NetSight (implied by concurrency claims) |
| Valgrind, GDB | ✅ NetSight (explicit validation methodology) |
| Git | General — no specific claims to defend |

This is your strongest, most fully-evidenced technical column — every systems skill you list has a matching, specific project claim.

---

## 7. Observability

| Item | Tied to a project bullet? | Notes |
|---|---|---|
| Prometheus, Grafana, Loki | ✅ Distributed Task Scheduler, and Prometheus also in NetSight | Confirmed |
| Alertmanager | ✅ Appears **in the project bullet** for Distributed Task Scheduler even though it's not in the skills header — worth adding to your skills list for consistency | — |
| CloudWatch | ❌ Skills list only, not named in Hermes bullets despite Hermes being AWS-native | `NEEDS VERIFICATION`: if Hermes actually emits to CloudWatch, that's a stronger story than the resume currently states |
| X-Ray | ✅ Hermes | Confirmed |
| k6 | ⚠️ Skills list only — Hermes and the Scheduler both link to "Load Test" evidence but **the resume text never names the tool used** | `NEEDS VERIFICATION`: confirm k6 (or whatever tool) was actually used before describing your load-testing methodology in detail |

---

## 8. Project-by-Project Concept & Claim Inventory

### 8.1 Hermes — Serverless Workflow Orchestration Platform

**Stated architecture concepts:** polyglot multi-tenant serverless (TypeScript/Node 20, Java, Python) · Event Sourcing · CQRS · append-only DynamoDB event store · 6 query-optimized read models · optimistic concurrency control · Transactional Outbox pattern · DynamoDB Streams → SQS → EventBridge · AWS Step Functions orchestration · X-Ray tracing · Cognito JWT-based tenant isolation · RBAC (claims extraction, tenant-boundary enforcement, role checks) · Terraform IaC (18 modules)

**Every quantitative claim:**
- 100 concurrent append attempts on the same sequence → exactly 1 succeeds, 99 rejected with `ConcurrencyException`
- 285 automated tests total: 234 TypeScript + 42 Java/JUnit5 + 9 Python/pytest
- 51+ test suites
- 18 Terraform modules
- 0 test failures

**`NEEDS VERIFICATION` (implementation-specific, not stated by resume):**
- Which specific services/components are written in TypeScript vs. Java vs. Python (the test-count split is a hint, not a confirmation of service boundaries)
- Exact mechanism by which the 6 read models are updated (Lambda triggers off DynamoDB Streams? Off EventBridge?)
- How the optimistic concurrency check is implemented at the DynamoDB API level (conditional writes on a version attribute is the standard pattern — confirm this is actually what your code does)
- Exact claims-extraction and role-check implementation for RBAC (custom middleware? a library?)
- What API Gateway does, if anything, in front of the Lambdas (see §4)
- Whether the system is actually deployed/live or exists as IaC + tests only
- Cost model / who pays for AWS usage during testing

### 8.2 Distributed Task Scheduler — Cron-as-a-Service

**Stated architecture concepts:** distributed cron · three-tier design (API gateway, coordinator, worker fleet) · RabbitMQ TTL + dead-letter-exchange used to trigger dispatch (avoiding DB polling) · horizontal worker scaling · PostgreSQL as source of truth · empirical delivery-correctness verification via row-count convergence · full observability stack across API/coordinator/3 worker replicas

**Every quantitative claim:**
- 10,000 simultaneous tasks in the load test
- RabbitMQ peaked at 8,562 messages
- 1,600 msg/s burst at TTL expiry
- Queue drained in ~7 min 46 sec
- 0 DLQ messages, 0 task loss
- PostgreSQL row counts converged 1:1 with consumed messages; post-drain query for `status='scheduled'` returned 0 rows
- 3 independently metriced worker replicas
- p95 latency: 160ms burst spike → 76ms floor

**Important rigor note:** the resume's phrase "confirmed exactly-once delivery empirically" needs careful handling in an interview. RabbitMQ's native delivery guarantee is *at-least-once* (with acknowledgments) or *at-most-once* (without). True exactly-once delivery is not something a message broker alone can guarantee — what you actually demonstrated is that your **end-to-end system, including idempotent consumer-side handling, produced an exactly-once *effect*** on the data (verified via row-count convergence), which is the correct and much stronger engineering claim. Learn to say *that* precisely rather than "exactly-once delivery" as a broker property — this exact distinction is a very likely interviewer trap and is covered in depth in `13_PROJECT_DEEP_DIVES/DISTRIBUTED_TASK_SCHEDULER.md` once we get there.

**`NEEDS VERIFICATION`:**
- Retry policy and backoff strategy on task failure
- How duplicate delivery is actually prevented/detected at the consumer (idempotency key? DB unique constraint?)
- Division of responsibility between Node.js and Python across gateway/coordinator/workers
- Coordinator's own failure mode — is it a single point of failure?
- Database schema for tasks

### 8.3 NetSight — Linux Network Telemetry & Traffic Analysis Engine

**Stated architecture concepts:** zero-copy packet capture via AF_PACKET/TPACKET_V3 · full protocol stack parsing (Ethernet, VLAN, IPv4, TCP/UDP/ICMP) · lock-free concurrent structures built from scratch: C11-atomic ring buffer, Treiber-stack memory pool, 1M-entry hash-based flow table · eBPF kernel observability via BTF CO-RE (portable across kernels 5.4–6.x, single binary, no per-kernel recompilation) · ncurses UI · Prometheus metrics export

**Every quantitative claim:**
- ~9,700 lines of C across 11 independently testable modules
- Validated via Unity test suites, cppcheck static analysis, Valgrind leak checks
- 43,442 TCP connection attempts at ~1,448 conn/s sustained, 0 failures
- 60-minute endurance test: <0.2% memory growth, 0 file-descriptor leaks
- 1M-entry hash table for flow tracking

**`NEEDS VERIFICATION`:**
- Hash function and collision-resolution strategy for the flow table
- How the Treiber stack avoids the ABA problem (tagged pointers? hazard pointers? Are you certain it's ABA-safe, or just lock-free in the common case?)
- eBPF attach mechanism (kprobe, tracepoint, or tc/XDP hook?)
- Exact metrics exported to Prometheus
- Load-test harness details (what generated 43,442 connections?)

This is your most technically defensible project *if* the above gaps are closed with real answers from your code — it's also the one most likely to draw a skeptical "did you really write this from scratch" line of questioning, precisely because it's advanced for the stated experience level.

### 8.4 S.N. Engineering Works — Full Stack Developer Intern

**Stated facts:** React.js + Supabase B2B machinery marketplace, taken from blank repo to live production, owned end-to-end · replaced manual email-based quoting with a custom project configurator backed by a Supabase lead pipeline with status tracking · shipped an internal ticketing module *after* the internship ended, at the company's request

**`NEEDS VERIFICATION`** (this project relies most heavily on personal recall since the resume gives outcomes, not mechanisms):
- Supabase schema design (tables, relationships)
- Auth mechanism (Supabase Auth vs. custom)
- How the configurator computes pricing/specs
- Deployment target (Vercel/Netlify/other)
- What made a bug "the hardest one" — this is a behavioral answer only you can supply

---

## 9. Skill-Gap / Exposure Risk Summary

These are skills listed in your header that **no project bullet demonstrates**. This isn't a resume flaw — technical-skills sections are conventionally broader than the project list — but each one is a plausible interviewer question with no ready-made resume story behind it. Decide in advance, honestly, how you'll answer if asked directly.

| Skill | Suggested honest framing if asked "tell me about a project using this" |
|---|---|
| Spring Boot | Conceptual/coursework knowledge of the framework's DI, MVC, and JPA model; no shipped project yet — pivot to what you *do* know (Spring fundamentals) rather than implying project experience |
| Kubernetes | Same pattern — you've used Docker in production-style testing (Task Scheduler); K8s knowledge is conceptual until demonstrated |
| MongoDB, Redis, Elasticsearch | Conceptual database/caching/search knowledge; your real project DB experience is PostgreSQL + DynamoDB |
| gRPC, WebRTC, WebSockets | Conceptual protocol knowledge; your real network-protocol depth is in NetSight (raw sockets, AF_PACKET) — lean on that instead when protocol questions come up |
| EC2, S3, API Gateway | You have real AWS depth (Lambda, DynamoDB, Step Functions, EventBridge, Cognito, X-Ray) — be ready to say plainly which specific services you've hands-on-built with |
| GitHub Actions | Conceptual CI/CD knowledge unless you confirm it was actually used somewhere |

We'll build honest, non-defensive scripted answers for each of these in `14_BEHAVIORAL/RESUME_WALKTHROUGH.md`.

---

## 10. Certifications & Activities

- **Gen AI Study Jams — Outstanding Achievement Award (Feb 2025):** 15 Google Cloud Skill Badges + 1 Arcade Game, via GDG on Campus MSIT. Useful as evidence of self-directed cloud learning; keep it framed that way (breadth of exposure) rather than as deep GCP production experience.
- **Geek Room MSIT (2024–25):** Member, helped organize DSA/coding competitions, received a Letter of Appreciation. This is solid raw material for behavioral questions about community, initiative, and collaboration (see Phase 21).

---

## 11. Overall Read on Your Resume

Two things are true at once, and both matter for how you prepare:

1. **Your three projects are unusually advanced for a pre-2027-graduate resume** — Event Sourcing/CQRS, lock-free systems programming, and eBPF are graduate/senior-engineer topics in most contexts. This is a real strength.
2. **That same advancement is exactly what will get stress-tested hardest.** An interviewer who's seen a hundred "built a CRUD app" resumes will treat claims like this with more scrutiny, not less. The gap between *describing* these systems and *defending* them under rapid-fire follow-ups is where most candidates with ambitious projects lose points — not because the work isn't real, but because they memorized a description instead of understanding the mechanism.

The project deep-dive phase (`13_PROJECT_DEEP_DIVES/`) is where this gets addressed directly, and it should be treated as the highest-priority phase in your prep, alongside DSA fundamentals.