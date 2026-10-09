# Project Deep Dive: WebChat — Real-Time Chat Platform (Not Currently on Your Resume)

⚠️ **This project doesn't appear on your resume.** Built and verified the same way as your other three, so it's ready if you add it — but until you do, don't casually bring it up as if it were part of your resume's story; an interviewer's first move after "tell me about a project" is often "is this on your resume?" Treat it as reserve material for now.

Your own repo has three excellent self-authored docs — `docs/ENGINEERING_AUDIT.md`, `docs/ARCHITECTURE.md`, `docs/PROJECT_GUIDE.md` — written with real rigor (a graded, honest self-review with a "Broken Features" table tracking fixed vs. open issues). Read them directly; they're better than any secondhand summary.

---

## Why This Project Matters for Your Resume Gaps

From `00_RESUME_ANALYSIS.md` §9, these skills are currently "listed but not demonstrated in any project": **MongoDB, Redis, Elasticsearch, WebRTC, Kubernetes.** WebChat gives you real, working evidence for every single one of them. If you add this project, several honest-redirect scripts in `14_BEHAVIORAL/RESUME_WALKTHROUGH.md` stop being necessary — you'd have real project experience to point to instead of "conceptual knowledge only."

---

## Architecture (Verified)

```
Client (React + Vite) ──HTTP/JWT──→ Gateway :4000 (reverse proxy, rate limit, CORS)
        │                                │
        │←──WebSocket──→ Chat :5000 ─────┼──→ Auth :3001 (PostgreSQL: users, JWT, rooms)
        │   (Socket.io,                  ├──→ Messages :3003 (MongoDB + Elasticsearch,
        │    WebRTC signaling)           │    regex fallback if ES unavailable)
        │                                └──→ Uploads :3004 (S3 presigned PUT — binary
        │                                     never touches app servers)
        │
        └──HTTPS PUT──→ S3 (direct upload via presigned URL)

Async persistence pipeline:
  send_message → optimistic broadcast (instant, temp ID)
              → publish to RabbitMQ "webchat.messages" exchange
              → message-worker consumes → MongoDB.create() + ES.index()
              → Redis PUBLISH webchat:message:persisted
              → Chat service emits message_confirmed {tempId, message} → client swaps temp ID for real Mongo _id

Resilience: if RabbitMQ is unavailable, Chat falls back to SYNCHRONOUS HTTP persist
  via the Messages service directly — a deliberate degrade-gracefully path, not a crash.

Real-time fan-out: Socket.io + Redis adapter — lets multiple Chat pod replicas share
  room membership/broadcast state, which is what makes horizontal scaling of the
  WebSocket layer possible at all (a single in-memory Socket.io instance can't span pods).

K8s: HPA on gateway (2-6 replicas, 60% CPU), chat (2-10 replicas, 60% CPU),
  message-worker (2-8 replicas, 70% CPU) — three independently tuned autoscaling profiles.
```

---

## Component-by-Component Justification

| Component | Why it exists | Why chosen | Failure mode / resilience |
|---|---|---|---|
| RabbitMQ async pipeline | Decouple "message delivered to room" (instant) from "message durably persisted" (can take longer) | Lets the real-time UX stay instant via optimistic broadcast while persistence happens asynchronously | **Verified fallback**: broker unavailable → synchronous HTTP persist instead — a real, tested degrade path, not just a documented aspiration |
| Redis (Socket.io adapter) | Multi-pod WebSocket fan-out | Socket.io's in-memory state doesn't span processes/pods by default; Redis adapter is the standard fix | Without it, a message from a client on pod A would never reach a client connected to pod B — directly why this exists |
| Redis (presence) | Track online/offline across instances | TTL (300s) + heartbeat (60s) — a lease-renewal pattern, not a single boolean flag | **Known open issue** (per your own audit): multi-tab presence marks a user offline early — a real, honestly-documented edge case |
| MongoDB + Elasticsearch | Message history (Mongo) + full-text search (ES) | Regex fallback on Mongo if ES is unavailable — same "degrade gracefully" philosophy as the RabbitMQ fallback | ES index-naming mismatch was a real bug you found and fixed (`webchat-messages` vs `webchat_messages`) |
| S3 presigned uploads | Keep binary data off app servers entirely | Client uploads directly to S3; app servers only ever handle metadata + presigned URL issuance | Room-membership check added on presign (was previously missing — a real, fixed security gap) |
| JWT access (15min) + refresh (7day) | Short-lived access tokens limit exposure; refresh token avoids re-login | Standard access/refresh split (`12_SECURITY/AUTHENTICATION.md`) | Logout/revocation endpoint destroys the refresh token server-side — closes the "JWT can't be revoked" gap discussed generically in that file |

---

## Real Bugs You Found and Fixed (Excellent, Specific "Tell Me About a Bug" Material)

Pulled directly from your own `ENGINEERING_AUDIT.md` — these are far stronger than a generic answer because they're real, specific, and already graded by severity:

- **"Worker retry nack+republish duplicated messages"** (High, Fixed) — a genuine idempotency bug: a naive nack-and-republish retry strategy caused the same message to be persisted twice. This is a perfect, concrete story for "tell me about a concurrency/idempotency bug you fixed" — you have the exact failure mode and fix in your own audit doc.
- **"K8s worker missing `REDIS_URL`" / "K8s chat missing `RABBITMQ_URL`"** (Critical, Fixed) — real Kubernetes config-management bugs (env vars misaligned between manifests and code) — good material for "tell me about a deployment issue you debugged."
- **"ES index name mismatch"** (High, Fixed) — a naming-inconsistency bug across services, the kind of thing that's invisible until search silently returns nothing.
- **Security fixes**: default `JWT_SECRET` now hard-fails startup in production (`process.exit(1)`) — a strong, specific "fail loud, not silent" security pattern worth citing if asked about secrets management (`12_SECURITY/CLOUD_SECURITY.md`).

## Honestly Open Issues (Good Material for "What Would You Improve")

Per your own audit: per-recipient read status is still document-global (not per-recipient), multi-tab presence has a known early-offline bug, there's no graceful shutdown on services yet, and k6 WebSocket tests currently use the wrong auth. Citing these specifically — rather than a generic "I'd add more tests" — is a strong, self-aware answer.

### Quick Revision
- This project is NOT on your resume — don't reference it as if it were until you decide to add it.
- It directly resolves five "listed but not demonstrated" skill gaps: MongoDB, Redis, Elasticsearch, WebRTC, Kubernetes.
- RabbitMQ-unavailable → synchronous HTTP fallback, and ES-unavailable → Mongo regex fallback, are both real, deliberate "degrade gracefully" patterns — a strong system-design answer if asked about resilience.
- Your own `ENGINEERING_AUDIT.md` gives you specific, real, already-graded bugs (fixed) and gaps (open) — better interview material than anything generic, because it's genuinely yours and genuinely specific.
