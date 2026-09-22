# Cloud Security

IAM/least privilege is in `08_AWS/AWS_FUNDAMENTALS.md`. This file covers secrets management and rate limiting.

---

## Cluster 1: Secrets Management

### What is it?
Secrets (API keys, DB passwords, signing keys) should never live in source code or plain environment-variable config committed to a repo — a dedicated secrets manager (AWS Secrets Manager, Parameter Store) stores them encrypted, controls access via IAM, and can support automatic rotation.

### Interview Questions
- **L1:** "Secrets belong in a dedicated secrets manager, encrypted at rest, access-controlled via IAM, ideally with rotation support — never hardcoded or committed to source control."
- **L2:** explain the practical difference between Secrets Manager (built-in rotation support, costs more per secret) and Parameter Store (cheaper, no built-in rotation, fine for less sensitive config) — a real cost/capability tradeoff worth knowing if asked to justify a choice for Hermes's Terraform-managed infrastructure.

---

## Cluster 2: Rate Limiting

### What is it?
Restricting how many requests a client can make in a given window — protects against abuse, accidental traffic storms, and one tenant/client degrading service for others (directly relevant to Hermes's multi-tenancy).

### How does it work?
Common algorithms: **fixed window** (simple, but allows a burst of 2x the limit right at a window boundary), **sliding window** (smoother, more accurate, more state to track), **token bucket** (tokens refill at a steady rate, allows some burst capacity up to the bucket size — a good general-purpose default), **leaky bucket** (processes at a strictly constant rate, smooths bursts entirely).

### Interview Questions
- **L1:** "Rate limiting restricts requests per client per window; token bucket is a common general-purpose algorithm allowing controlled bursts, while fixed-window is simpler but has a boundary-burst weakness."
- **L2:** explain the fixed-window boundary problem precisely: a limit of 100 requests/minute with fixed windows lets a client send 100 requests at 11:00:59 and another 100 at 11:01:00 — 200 requests in 2 seconds, technically within both individual windows' limits, but clearly not the intended behavior; sliding-window or token-bucket approaches avoid this.
- **L3 — 🔴 Hermes-relevant:** explain why rate limiting matters specifically for a multi-tenant system: without per-tenant limits, one tenant's traffic spike (or bug causing a retry storm) can degrade the experience for every other tenant sharing the same infrastructure — a rate limiter scoped per-tenant (using the same tenant ID extracted from the verified JWT, per `08_AWS/COGNITO.md`) is the natural implementation.

### Quick Revision
- Secrets: dedicated manager, encrypted, IAM-controlled, never in source/plain env vars committed to a repo.
- Token bucket: good general default, allows controlled bursts. Fixed window: simple but has a boundary-burst weakness.
- Per-tenant rate limiting protects tenant isolation at the traffic-shaping layer, not just at the data-access layer.
