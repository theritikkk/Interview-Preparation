# Resume Walkthrough — "Walk Me Through Your Resume"

A ~90-second scripted structure, built only from confirmed facts. Practice this until it's natural, not memorized-sounding.

---

## The Script Structure

**Opening (10s):** who you are, what you're studying, where. *"I'm a B.Tech Information Technology student at Maharaja Surajmal Institute of Technology, graduating in 2027, and I've spent the last while going deep on backend and systems-level engineering — three projects that each pushed into a different hard problem: distributed data correctness, reliable messaging, and low-level systems programming."*

**Internship (15s):** the one real-world, production-ownership credential. *"Over the summer I interned at S.N. Engineering Works, where I took a B2B marketplace from a blank repository to a live production site — including replacing a manual, email-based quoting process with a custom configurator and lead pipeline. They actually asked me to build an additional internal tool after the internship formally ended, which I was glad to do."*

**Hermes (20s):** the serverless/event-driven story. *"On the project side, my biggest one is Hermes — a serverless, multi-tenant workflow platform on AWS, built around Event Sourcing and CQRS. I validated the concurrency model specifically: 100 simultaneous attempts to write conflicting events, exactly one succeeds, the rest cleanly reject — and I built a Transactional Outbox to eliminate dual-write failures between the event store and the downstream event bus."*

**Task Scheduler (15s):** the messaging/reliability story. *"I also built a distributed task scheduler — a cron-as-a-service system on RabbitMQ and PostgreSQL — and load-tested it to 10,000 concurrent tasks with zero task loss, which meant getting precise about what 'reliable delivery' actually guarantees versus what your own processing logic has to guarantee on top of it."*

**NetSight (15s):** the systems-programming story. *"And on the lower-level end, I wrote a zero-copy packet-capture engine in C — lock-free data structures from scratch, eBPF for kernel-level tracing — which was really my deepest dive into systems programming."*

**Closing (10s):** connect to what you're looking for. *"That range — from AWS-native distributed systems down to raw sockets in C — is what I'm looking to keep building on, ideally somewhere I can keep working close to real infrastructure problems."*

---

## Adapting the Script

- **For a frontend-heavy role:** lead with the internship's React/Supabase work, and spend less time on NetSight.
- **For a systems/platform role:** lead with NetSight, spend more time on the lock-free/eBPF specifics.
- **For a general SDE/full-stack screen:** the balanced version above is the right default.
- Always end by connecting to the specific role/company if you know it — a generic closing line is a missed opportunity every time.

---

## If Asked to Go Deeper on an Exposure-Risk Skill Mid-Walkthrough

Pulled directly from `00_RESUME_ANALYSIS.md` §9 — have these ready as natural, un-defensive redirects if a skills-list item comes up:

| If asked about... | Honest, confident redirect |
|---|---|
| Spring Boot | "My hands-on backend project work has been in Node/TypeScript and Python — I know Spring's DI/MVC/JPA model well from study, and I'd be glad to walk through the concepts." |
| Kubernetes | "My Task Scheduler uses Docker directly rather than K8s orchestration — for that project's fixed-size worker fleet, I didn't need the full orchestration surface, but I understand the core objects and where K8s would add value at larger scale." |
| MongoDB / Redis / Elasticsearch | "My hands-on NoSQL experience is specifically DynamoDB, through Hermes — I can speak to the conceptual differences between DynamoDB's model and MongoDB's/Redis's if that's useful." |
| gRPC / WebRTC | "My protocol-level depth is actually below the HTTP layer — NetSight works directly with raw Ethernet/IP/TCP frames — so I know these more conceptually than from a shipped project." |

The pattern in every redirect: **acknowledge plainly, then pivot to the adjacent thing you can actually go deep on.** Never imply project experience you don't have — an interviewer probing one level deeper on a fabricated claim is far worse than a confident, honest "I know this conceptually, here's what I've actually built instead."

### Quick Revision
- ~90 seconds total, weighted toward whichever project matches the role.
- Every exposure-risk skill gets the same pattern: acknowledge honestly, redirect to real depth you do have.
- Practice out loud, not just in your head — the gap between "know the content" and "deliver it smoothly" is real.
