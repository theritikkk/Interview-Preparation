# 📚 Ritik Raj — Distributed Systems Interview Prep: Master Index

> **Generated:** October 7, 2026  
> **Total Study Material:** 6,898 lines / 344 KB across 6 files (in this folder alone)  
> **Plus:** 70+ existing notes files across 16 categories in the parent repo

---

## 🗂️ Files in This Folder (`distributed-systems/`)

| # | File | Lines | Size | What It Covers |
|:--|:-----|:------|:-----|:---------------|
| 1 | [distributed-systems-notes.md](file:///Users/ritikraj/Documents/GitHub/Interview-Preparation/distributed-systems/distributed-systems-notes.md) | 335 | 20 KB | Dynamo-style KV store, LSM-Trees, consistent hashing, quorums, vector clocks, gossip, hinted handoff, Merkle trees |
| 2 | [microservices-backend-interview-notes.md](file:///Users/ritikraj/Documents/GitHub/Interview-Preparation/distributed-systems/microservices-backend-interview-notes.md) | 365 | 24 KB | CAP/PACELC, Raft/Paxos, 2PC/Saga, caching patterns, load balancing, resilience, observability, system design framework |
| 3 | [kafka-event-driven-deep-dive.md](file:///Users/ritikraj/Documents/GitHub/Interview-Preparation/distributed-systems/kafka-event-driven-deep-dive.md) | 1,547 | 64 KB | Kafka internals (commit log, ISR, KRaft), Spring Boot + Kafka, threading model, EOS, transactional outbox, DLT, CQRS, event sourcing |
| 4 | [java-concurrency-spring-boot-deep-dive.md](file:///Users/ritikraj/Documents/GitHub/Interview-Preparation/distributed-systems/java-concurrency-spring-boot-deep-dive.md) | 1,138 | 48 KB | Java threading, locks, `java.util.concurrent`, CompletableFuture, Virtual Threads, Spring IoC/AOP/@Transactional internals, HikariCP, interview problems |
| 5 | [docker-kubernetes-aws-production.md](file:///Users/ritikraj/Documents/GitHub/Interview-Preparation/distributed-systems/docker-kubernetes-aws-production.md) | 2,362 | 136 KB | Docker internals, Kubernetes architecture, all AWS services (Lambda/DDB/EventBridge/Step Functions/SQS/S3/ECS/Cognito), Terraform, CI/CD, deployment patterns |
| 6 | [system-design-case-studies.md](file:///Users/ritikraj/Documents/GitHub/Interview-Preparation/distributed-systems/system-design-case-studies.md) | 1,151 | 52 KB | 10 fully-worked system designs: URL Shortener, Rate Limiter, Task Scheduler, Chat, Notifications, News Feed, KV Store, Video Streaming, E-Commerce, Autocomplete |

---

## 📖 Your Full Study Inventory (Entire Repo)

### Deep-Dive Book Notes (Already Completed)
| Folder | Book / Topic | Files |
|:-------|:-------------|:------|
| `ddia-book-notes/` | *Designing Data-Intensive Applications* — Kleppmann | 14 chapters (344 KB total) |
| `database-internals-book-notes/` | *Database Internals* — Petrov (B-Trees, LSM, distributed consensus) | 14 chapters |
| `ostep-book-notes/` | *Operating Systems: Three Easy Pieces* (processes, scheduling, memory, concurrency, persistence) | 12 chapters |
| `tcp-ip-architecture-design-and-implementation-in-linux-book-notes/` | TCP/IP Linux kernel implementation | 20 chapters |
| `BPF-Performance-Tools/` | *BPF Performance Tools* — Brendan Gregg | 29 chapters + 31 SVG diagrams |
| `Hands-On-Network-Programming-with-C-book-notes/` | Network programming in C (sockets, TCP, UDP, TLS, SSH) | 16 chapters |
| `Linux-Kernel-Development-book-notes/` | Linux kernel internals (process, scheduling, interrupts, synchronization) | 33 chapters |
| `terminal-linux-notes/` | Linux terminal internals (TTY, shells, syscalls, processes, signals) | 40 files |
| `cpu-memory-notes/` | CPU caches, virtual memory, NUMA programming | 7 chapters |
| `rpc_internal_documentation/` | RPC from scratch → ONC/Sun RPC → gRPC, idempotency, distributed tracing | 8 docs + C/Python code |
| `operating-system-from-scratch/` | OS development (boot, x86, assembly, memory, drivers, filesystems) | 49 files |
| `bgnet-book-notes/` | *Beej's Guide to Network Programming* (Berkeley sockets) | 11 chapters |
| `Competitive-Programming-Short-Notes/` | CP tricks, STL, math, graphs, DP, advanced DS, geometry | 33 files |

### Interview-Specific Prep (Already Completed)
| Folder | Category | Files |
|:-------|:---------|:------|
| `Interview-Preparation/01_PROGRAMMING/` | Java, Python, TypeScript | 3 files |
| `Interview-Preparation/02_DSA/` | Arrays, Trees, Graphs, DP, Patterns | 7 files |
| `Interview-Preparation/03_CS_FUNDAMENTALS/` | OS, Networking, DBMS, OOP | 4 files |
| `Interview-Preparation/04_BACKEND/` | Spring Boot, Node, Express, gRPC, REST, WebSockets | 7 files |
| `Interview-Preparation/05_FRONTEND/` | React, JavaScript, TypeScript, Performance | 4 files |
| `Interview-Preparation/06_DATABASES/` | PostgreSQL, MongoDB, DynamoDB, Redis, Elasticsearch | 5 files |
| `Interview-Preparation/07_DISTRIBUTED_SYSTEMS/` | Concurrency, Consistency, Event-Driven, Messaging | 5 files |
| `Interview-Preparation/08_AWS/` | Lambda, DynamoDB, EventBridge, Step Functions, SQS, API Gateway, Cognito | 8 files |
| `Interview-Preparation/09_DEVOPS/` | Docker, Kubernetes, Terraform, CI/CD | 4 files |
| `Interview-Preparation/10_OBSERVABILITY/` | Prometheus, Grafana, Loki, CloudWatch, k6, X-Ray | 6 files |
| `Interview-Preparation/11_SYSTEM_DESIGN/` | Fundamentals, Common Designs, Interview Framework | 3 files |
| `Interview-Preparation/12_SECURITY/` | Auth, Authorization, Web Security, Cloud Security | 4 files |
| `Interview-Preparation/13_PROJECT_DEEP_DIVES/` | Hermes, Task Scheduler, WebChat, NetSight, Internship | 5 files |
| `Interview-Preparation/14_BEHAVIORAL/` | HR, STAR stories, Resume Walkthrough | 3 files |
| `Interview-Preparation/15_INTERVIEW_PREP/` | Debugging, Machine Coding, Mock Interviews, Screening | 6 files |
| `Interview-Preparation/16_REVISION/` | Last Day Revision, Cheatsheets, Top 100 Questions | 4 files |

---

## 🎯 Recommended Study Order (October 2026)

### Week 1: Core Theory Revision
1. ✅ `distributed-systems-notes.md` — Dynamo, quorums, vector clocks
2. ✅ `microservices-backend-interview-notes.md` — CAP, Raft, patterns
3. 📖 `kafka-event-driven-deep-dive.md` — Kafka + Spring Boot threading
4. 📖 `java-concurrency-spring-boot-deep-dive.md` — Java concurrency + Spring internals

### Week 2: System Design & Infrastructure
5. 📖 `system-design-case-studies.md` — Practice all 10 designs
6. 📖 `docker-kubernetes-aws-production.md` — Docker, K8s, AWS, Terraform
7. Revise `ddia-book-notes/` chapters 5-9 (replication, partitioning, transactions, consistency)

### Week 3: Project Defense & DSA
8. Read `13_PROJECT_DEEP_DIVES/` — Hermes, Task Scheduler, WebChat
9. Practice DSA from `02_DSA/` and `Competitive-Programming-Short-Notes/`
10. Mock interviews using `15_INTERVIEW_PREP/MOCK_INTERVIEWS.md`

### Week 4: Revision & Polish
11. `16_REVISION/RAPID_REVISION.md` + `LAST_DAY_REVISION.md`
12. `14_BEHAVIORAL/STAR_STORIES.md` — Prepare 5 behavioral stories
13. Final pass through all scenario-based Q&A sections in the new files

---

## 🔗 How The New Files Connect To Your Projects

```
Your Resume Projects          New Notes That Defend Them
─────────────────────         ─────────────────────────
Hermes (Event Sourcing)  ───► kafka-event-driven-deep-dive.md (EOS, outbox, CQRS)
                         ───► java-concurrency-spring-boot-deep-dive.md (Spring internals)
                         ───► docker-kubernetes-aws-production.md (Lambda, DDB, Step Functions)
                         ───► system-design-case-studies.md (#9: E-Commerce Order System)

Task Scheduler           ───► kafka-event-driven-deep-dive.md (DLQ, async processing)
                         ───► docker-kubernetes-aws-production.md (Docker Compose, CI/CD)
                         ───► system-design-case-studies.md (#3: Distributed Task Scheduler)

WebChat                  ───► kafka-event-driven-deep-dive.md (pub/sub, thread handling)
                         ───► docker-kubernetes-aws-production.md (K8s, HPA, services)
                         ───► system-design-case-studies.md (#4: Real-Time Chat System)
```
