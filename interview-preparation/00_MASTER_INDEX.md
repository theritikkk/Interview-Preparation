# Ritik Raj — Software Engineering Interview Preparation
## Master Index

This is the control document for your interview-prep knowledge base. It tracks what exists, what's next, and the rules this whole project follows.

**Status: Complete (Phases 0-16, ~82 files). Worked DSA problems (`02_DSA/`) are now included alongside the pattern-recognition map. Nothing is left pending.**

Why phase-by-phase, not all at once: you asked for ~16 folders of deep, interview-grade notes across DSA, systems, three genuinely advanced projects, cloud, and behavioral prep. Trying to produce all of that in a single pass would force a choice between "enormous and unreadable" or "shallow and useless." Neither helps you. Instead, each phase becomes its own reviewable deliverable.

---

## Ground Rules This Project Follows

1. **Nothing is invented.** Every technical claim about your projects comes from your resume text. Nothing about *how* something is implemented gets fabricated.
2. **Verification flags are explicit.** Anywhere the resume states *what* you built but not *how*, it's marked `NEEDS VERIFICATION` — meaning: check your actual code before you say this in an interview, because I can't confirm it for you.
3. **"Listed as a skill" ≠ "demonstrated in a project."** Several resume skills (Spring Boot, MongoDB, Kubernetes, gRPC, WebRTC, GitHub Actions, S3, EC2, API Gateway, Redis, Elasticsearch) don't appear inside any project bullet. That's not a flaw in your resume, but it *is* an interview risk if you're not ready for "tell me about a project where you used X" and X turns out to be a skills-list item, not a shipped-project item. This is called out explicitly in `00_RESUME_ANALYSIS.md`.
4. **Rigor over exaggeration.** Where a resume phrase (e.g., "exactly-once delivery") could be challenged against the strict theoretical definition, you'll be taught the technically precise version to say in an interview — not asked to walk back your resume, just to defend it accurately.

---

## Folder Structure & Status

```
interview-prep/
├── 00_MASTER_INDEX.md .................................. ✅ done (this file)
├── 00_RESUME_ANALYSIS.md .............................. ✅ done
├── 00_INTERVIEW_ROADMAP.md ............................ ✅ done
│
├── 01_PROGRAMMING/  (C, CPP, JAVA, PYTHON, JS, TS, SQL)....... ✅ done
├── 02_DSA/
│   ├── PATTERNS.md (recognition map, all 26 patterns)......... ✅ done
│   └── 7 worked-problem files (see below)...................... ✅ done
├── 03_CS_FUNDAMENTALS/  (OS, DBMS, NETWORKING, OOP)........... ✅ done
├── 04_BACKEND/  (NODE, EXPRESS, FASTAPI, SPRING_BOOT, REST)... ✅ done
├── 05_FRONTEND/  (REACT, JS_BROWSER, TS_REACT, PERF).......... ✅ done
├── 06_DATABASES/  (POSTGRES, MONGO, DYNAMODB, REDIS, ES)...... ✅ done
├── 07_DISTRIBUTED_SYSTEMS/  (FUNDAMENTALS, MESSAGING, ...).... ✅ done
├── 08_AWS/  (LAMBDA, DYNAMODB, SQS, EVENTBRIDGE, ...)......... ✅ done
├── 09_DEVOPS/  (DOCKER, KUBERNETES, TERRAFORM, CICD).......... ✅ done
├── 10_OBSERVABILITY/  (PROMETHEUS, GRAFANA, LOKI, XRAY, K6)... ✅ done
├── 11_SYSTEM_DESIGN/  (FUNDAMENTALS, COMMON_DESIGNS, ...)..... ✅ done
├── 12_SECURITY/  (WEB_SECURITY, AUTH, AUTHORIZATION, CLOUD)... ✅ done
├── 13_PROJECT_DEEP_DIVES/  (HERMES, SCHEDULER, NETSIGHT, INTERN) ✅ done — highest priority
├── 14_BEHAVIORAL/  (HR, STAR_STORIES, RESUME_WALKTHROUGH)..... ✅ done
├── 15_INTERVIEW_PREP/  (SCREENING, MACHINE_CODING, MOCKS)..... ✅ done
└── 16_REVISION/  (CHEATSHEETS, TOP_100s, LAST_DAY)............ ✅ done
```

---

## Your Three Interview Axes

Your resume is not a generic MERN-stack resume — it has three projects that each open a very different, very deep line of questioning.

| Project | Core Domain | What an Interviewer Will Drill Into |
|---|---|---|
| **Hermes** | Serverless + Event-Driven Architecture | Event Sourcing/CQRS correctness, optimistic concurrency, dual-write elimination, multi-tenant security (Cognito/RBAC), polyglot service boundaries |
| **Distributed Task Scheduler** | Messaging + Reliability Engineering | RabbitMQ TTL mechanics, exactly-once vs. effectively-once delivery, failure/DLQ handling, horizontal scaling, PostgreSQL under load |
| **NetSight** | Systems Programming / Linux Internals | Zero-copy packet capture, lock-free concurrency (C11 atomics, Treiber stack), eBPF/BTF CO-RE, memory safety validation |
| **S.N. Engineering Works (Internship)** | Product Engineering / Ownership | Full-stack delivery, Supabase data modeling, scope ownership, stakeholder handling |

Because these three projects go well beyond typical entry-level scope, expect interviewers — especially at more technical companies — to actively probe *whether you actually understand what you built*, not just whether you can describe it. That's the single biggest risk and the single biggest opportunity in your resume.

---

## Next Step

Read `00_RESUME_ANALYSIS.md` and `00_INTERVIEW_ROADMAP.md`. Tell me how you want to proceed — I've put the options in a quick-select below.
