# Mock Assessment 3 — Full Stack (The Final)

> **Format:** 90 minutes, closed book. 120 points: rapid questions (40), calculations (30), scenarios (50). Target: **≥90 (75%)**. This is the interview rehearsal: treat every scenario as a story you could tell out loud.
>
> **Scope:** AWS (P5), security (P8.4), SLOs/incidents (P8.3), CI/CD (P7.4), plus cross-phase synthesis. Phase 9 knowledge appears in two scenarios (GPU scheduling, serving cost) — answerable from the notes, no GPU required.

---

## Part A — Rapid questions (20 × 2 = 40 points)

1. What is the difference between a security group and a network ACL in AWS?
2. What is an IAM role vs an IAM user, and which should CI use?
3. What does an instance profile do?
4. What is IMDSv2's main security improvement over IMDSv1?
5. What is a VPC endpoint, and when does it save data-transfer cost?
6. What does an EBS gp3 volume let you tune that gp2 does not?
7. What is a spot interruption notice, and how much warning do you get?
8. What does an ALB health check gate, and what does it not test?
9. What is the difference between horizontal and vertical scaling, and which does an ASG do?
10. What is an S3 lifecycle rule for?
11. What is a least-privilege policy, and name one pattern for scoping S3 access?
12. What does a signed URL do, and when is it the right tool?
13. What is etcd encryption at rest, and what does it not protect against?
14. What is a Kubernetes Secret's default encoding, and why is it not encryption?
15. What is the difference between authentication and authorization in cluster access?
16. What is a Pod Security Admission profile, and which one should tenants get?
17. What is a secret-scanning tool's main limitation?
18. What is an SLO, and how does it differ from an SLA?
19. What is an error budget policy, and who enforces it?
20. What is a runbook, and what makes one useful under pressure?

## Part B — Calculations (6 × 5 = 30 points)

### B1 — Capacity with failure headroom

A service peaks at 600 req/s and grows 30% next quarter. Each request uses 12 ms CPU and 50 MiB memory. Pods have 4 vCPU / 8 GiB. Target utilization 60%. Three AZs, must survive one AZ loss, plus 20% deploy spare.

1. Forecast peak?
2. Cores at target, and pods by CPU?
3. Pods by memory?
4. Pods after the one-AZ requirement?
5. Pods after deploy spare, and the binding resource?

### B2 — SLO and budget

A 99.95% SLO over 30 days on a service handling 50M requests/month. So far this month (20 days), 30,000 requests failed.

1. Budget in requests and in minutes (availability)?
2. Observed error ratio and burn rate?
3. Fraction of budget consumed?
4. Max error ratio for the remaining 10 days to still meet the SLO?
5. What does this imply about deploys in the remaining window?

### B3 — Burn-rate alert design

A 99.9% SLO over 28 days. Fast-burn threshold: 14.4× over 1h (5m confirmation). Slow-burn: 6× over 6h (30m confirmation).

1. Budget in minutes?
2. The 1h error ratio is 2%. Does the fast-burn fire?
3. The 6h error ratio is 0.5%. Does the slow-burn fire?
4. What error ratio does 14.4× correspond to?
5. Why does the slow-burn exist if the fast-burn catches real outages?

### B4 — GPU inference cost

A serving GPU costs $3.60/hour. It serves 40 req/s with 500 output tokens each, at 55% utilization over the billing window.

1. Output tokens per second?
2. Cost per 1M output tokens at stated utilization?
3. The "benchmark" cost at 100% utilization?
4. The utilization-adjusted multiplier, and why it matters?
5. Two levers to reach $0.05 per 1M tokens, with expected magnitude?

### B5 — Storage growth

An S3 bucket receives 800 GB/day of new logs. A lifecycle rule transitions to a cheaper tier at 30 days and expires at 180 days. Assume flat daily volume and no versioning.

1. Data present at steady state?
2. Rough monthly growth of total bytes stored (steady state)?
3. If early deletion fees apply to the archive tier (90-day minimum), what is the risk of expiring at 180 days?
4. If queries read 2% of the archive tier monthly, does that change the tiering decision?
5. Name one cost-vs-latency trade this design accepts.

### B6 — CI/CD lead time

A pipeline has stages: build 6 min, test 12 min, security scan 4 min, deploy 3 min, and a required 15-min soak before promotion. It runs for every PR (5/day average) and for every main merge (3/day).

1. Total pipeline time for one run?
2. If stages are sequential, what is the critical path?
3. If build and security scan can run in parallel with test, what is the new critical path?
4. Daily compute-minutes for PR runs (worst case, sequential)?
5. Two changes that cut lead time without cutting safety?

## Part C — Scenarios (5 × 10 = 50 points)

For each: evidence first (3), two likely causes (2), smallest safe next action (3), prevention (2).

### C1 — A pod in production can read every namespace's secrets

During a routine audit, a pod in namespace `team-a` has a service account whose token can `get secrets` cluster-wide. The pod itself needs none.

### C2 — The pipeline deploys to production from a forked PR

A PR from a fork ran a workflow that had access to the deployment environment. The PR was a dependency-bump with a suspicious post-install script. The environment gate did not fire.

### C3 — The load balancer shows healthy, but half the users see 500s

One availability zone's instances are unhealthy; the ALB health checks pass because they hit `/health` which returns 200 without touching the database. Errors come only from the AZ whose DB connection pool is exhausted.

### C4 — Cost per request doubled after a traffic shift

A serving fleet's cost per 1,000 requests doubled in two weeks. Requests/day and replicas are unchanged; the model version is unchanged. Instance type is unchanged. Utilization dropped from 65% to 32%.

### C5 — The new GPU node's pods are pending while GPUs sit idle

A new GPU node joined the cluster. `nvidia-smi` on the node shows 8 healthy GPUs. Pods requesting 1 GPU each are Pending with `Insufficient nvidia.com/gpu`. The device plugin pod is Running.

---

# ANSWER KEY — Mock 3

## Part A

1. Security group: stateful, attached to instances, allow-only. Network ACL: stateless, attached to subnets, ordered allow/deny rules.
2. A role is assumable identity with temporary credentials (best for workloads and CI); a user is a long-lived identity with static keys (avoid for automation). CI should assume a role via OIDC.
3. Attaches an IAM role to an EC2 instance so applications on it can obtain temporary credentials via IMDS.
4. Token-based (PUT to get a session token) with hop limits, so SSRF-style unauthenticated reads of credentials are much harder.
5. A private path from a VPC to an AWS service without public internet; gateway endpoints (S3/DynamoDB) avoid NAT gateway data-processing charges for that traffic.
6. Provisioned IOPS and throughput independently of size (gp2 ties IOPS to size).
7. AWS reclaims spot capacity; you get a 2-minute notice (EventBridge/interrupt signal) to checkpoint and exit.
8. Instance-level health for routing decisions (marks targets healthy/unhealthy). It does not test dependencies, business flows, or user-visible latency.
9. Horizontal adds instances (what an ASG does); vertical changes instance size. ASG scales horizontally (plus target-tracking policies).
10. Transition objects between storage classes / expire them on a schedule to match access patterns and cost.
11. Grant only the permissions needed for the identity's actual operations; e.g., S3 policies scoped to a prefix (`bucket/prefix/*`) with specific actions (get/put/list), not `s3:*` on `*`.
12. Grants time-limited access to an object without AWS credentials on the client — right for direct browser/app downloads/uploads where the backend mediates authorization.
13. Encrypts etcd's backing store so etcd compromise at rest does not expose secrets in plaintext. It does not protect against someone with API access (`get secrets`) or a compromised kubelet.
14. Base64. It is encoding, not encryption — readable by anyone with the RBAC permission to read secrets.
15. Authentication: who you are (OIDC, certs, tokens). Authorization: what you may do (RBAC/policies). Cluster access needs both; most incidents are authorization gaps.
16. A policy tier (privileged/baseline/restricted) enforced at admission. Tenants should get `restricted` (non-root, no priv-esc, dropped caps, seccomp).
17. It only catches known patterns/committed secrets in what it can see — it cannot catch secrets minted outside the repo, dynamic generation, or exfiltration after commit.
18. SLO: an internal target for an SLI over a window (e.g., 99.9% of requests < 300 ms over 28 days). SLA: a contractual commitment with customers, usually with penalties, and typically looser than the SLO.
19. The rule for what happens when the budget is spent (freeze deploys, review risks). Enforced by the team leadership/IC process, not by tooling alone.
20. A step-by-step procedure for a specific failure: symptom, diagnosis, mitigation, verification, escalation. Useful = a stranger can follow it under stress.

## Part B

### B1

1. `600 × 1.30 = 780 req/s`.
2. CPU demand = `780 × 0.012 = 9.36 cores`; at 60% → `15.6 cores` → pods by CPU = `ceil(15.6/4) = 4`.
3. Memory = `780 × 50 MiB = 39,000 MiB = 38.09 GiB`; at 60% → `63.48 GiB` → pods = `ceil(63.48/8) = 8`.
4. One-AZ survival with 3 AZs: `N × 2/3 ≥ 8` → `N ≥ 12` → 12 pods (4/4/4).
5. Deploy spare: `ceil(12 × 1.20) = 15` pods. Per-pod rate = `780/15 = 52 req/s`; CPU/pod = `0.624 cores`; memory/pod = `38.09/15 ≈ 2.54 GiB`. Memory was the binding resource before the failure/spare rules; after them the layout is capacity-shaped by resilience, not by CPU.

### B2

1. Requests: `0.05% × 50M = 25,000` bad requests. Minutes: `0.05% × 43,200 = 21.6 min`.
2. Ratio = `30,000/50M × (50M/33.3M actual so far...)` — using elapsed-window traffic: if 50M/month implies ~1.67M/day, 20 days ≈ 33.3M requests; ratio = `30,000/33,300,000 = 0.09%`. Burn = `0.09%/0.05% = 1.8×`. (If the 50M is whole-month and 30,000 is the whole-month count, the ratio is `0.06%`, burn 1.2× — state your assumption; the method is what's graded.)
3. Request-based: `30,000/25,000 = 120%` — over budget already. (This is why request-based budgets are stricter than time-based when traffic is uneven; state which basis you use.)
4. If over budget, no ratio saves the window without an SLO revision; if under (say the 1.2× basis: consumed `0.0006 × 50M = 30,000`? — with the elapsed-traffic basis, remaining budget = `25,000 − 30,000 = negative`). Take the 1.2× basis: consumed = `30,000` (same) — the point is: compute remaining budget explicitly and do not hand-wave.
5. If the budget is exhausted, the error-budget policy applies: freeze non-emergency deploys, investigate the error sources, or renegotiate the SLO explicitly (with stakeholders) — not silently.

### B3

1. `0.1% × 40,320 = 40.32 minutes`.
2. Burn = `2%/0.1% = 20×` ≥ 14.4× → yes, if sustained for the 5m confirmation window.
3. Burn = `0.5%/0.1% = 5×` < 6× → no.
4. `14.4 × 0.1% = 1.44%` (1-hour ratio).
5. The fast-burn catches severe outages; the slow-burn catches chronic degradation that consumes the budget over days without ever spiking the 1h window. Without it, a service can fail its SLO for a month with no page.

### B4

1. `40 × 500 = 20,000 tok/s`.
2. `($3.60/3600) ÷ 20,000 × 1e6 / 0.55 = $0.0004545... ` → compute: `$0.001/s ÷ 20,000 tok/s = $5e-8/tok` → `× 1e6 = $0.05/1M` at 100%; adjusted for 55% utilization: `0.05/0.55 ≈ $0.0909/1M`.
3. At 100%: `$0.05/1M`.
4. `1/0.55 ≈ 1.82×` — the gap between the benchmark number and reality; planning with the benchmark number understates cost by ~82%.
5. Levers: raise utilization via better batching/consolidation (55% → 80% ⇒ `0.05/0.80 = $0.0625`); quantize/serve a smaller model to raise tok/s (e.g., 1.4× ⇒ `0.05/0.55/1.4 ≈ $0.065`); combined ⇒ ~$0.045, under target. State assumptions for each.

### B5

1. `800 GB × 180 = 144 TB` (before tiering discounts, storage bytes are the same; only the price differs).
2. At steady state, total bytes are constant (inflow = expiry outflow); monthly *spend* changes only via tier prices. Growth of bytes: ~0.
3. Expiring objects before the archive tier's minimum duration (90 days) incurs early-deletion fees; objects stored 30–90 days in archive would be charged as if stored 90 days. At 180-day expiry this design is safe (180 > 90).
4. Yes: if 2% of archive objects are read monthly, access latency/frequency may justify a warmer tier (or keeping recent hot data in standard); tiering is an access-pattern decision, not a default.
5. The design accepts retrieval latency and per-GB retrieval costs in exchange for much lower storage cost — right for audit logs, wrong for anything queried interactively.

### B6

1. `6 + 12 + 4 + 3 + 15 = 40 minutes`.
2. Build → test → scan → deploy → soak (sequential) = 40 min.
3. Build → max(test, scan) → deploy → soak = `6 + 12 + 3 + 15 = 36 min`.
4. `5 × 40 = 200` minutes/day (sequential worst case).
5. Parallelize independent stages (36 min); run the security scan only on changed paths or on main (scan PRs with a fast subset); cache build artifacts; use a PR-level smoke test and reserve the soak for main merges (halves PR cost). Do not cut the soak on main — it is the safety net.

## Part C

### C1

Evidence: the service account's RBAC bindings (`kubectl auth can-i --as system:serviceaccount:team-a:default get secrets -A`), the ClusterRoleBinding that grants it, who created it, and whether the pod needs the API at all (`automountServiceAccountToken`). Likely causes: a copied example binding granting `cluster-admin`/wildcard, or a DefaultSA with an over-broad ClusterRole. Next action: remove the binding (revoke first), disable token automount for the pod, and audit API logs for actual secret reads by that identity. Prevention: namespace-scoped roles, deny-by-default secret reads, admission policy rejecting wildcard RBAC for tenant namespaces, alert on new cluster-wide bindings.

### C2

Evidence: the workflow's trigger and permissions (`pull_request_target` vs `pull_request`), whether the checkout uses the PR's code, the environment's protection rules (required reviewers, branch filters), and the post-install script's behavior. Likely causes: `pull_request_target` running PR code with write permissions, or the environment missing required reviewers/branch restrictions. Next action: revoke the run's credentials/permissions immediately (rotate the token/OIDC trust, audit cloud logs for assumed-role actions), disable the workflow, and fix the trigger to not execute PR code with write access. Prevention: never run PR code with deployment permissions; require reviewers on production environments; restrict OIDC trust to specific branches/refs; scan dependency PRs before granting any pipeline access.

### C3

Evidence: ALB target health per AZ (all healthy), actual request logs by AZ/instance, DB connection-pool metrics per instance, and the health-check path/response. Likely causes: the health check tests liveness only (returns 200 without a DB call), and the DB pool exhaustion is AZ-specific (a hot AZ, an AZ-local dependency, or asymmetric network). Next action: make the health check meaningful (a dependency-aware check or a canary request), and drain/replace the failing AZ's instances; if the pool is the constraint, cap concurrency and shed load. Prevention: health checks that exercise the real dependency path (carefully — avoid check storms); per-AZ error/latency panels; alert on AZ asymmetry, not just aggregate.

### C4

Evidence: utilization series (65% → 32%), request/token mix by model version, replica count and autoscaler state, instance type and pricing (spot vs on-demand), and per-model cost attribution. Likely causes: a traffic-mix change that lowered GPU efficiency (e.g., shorter requests → lower batching efficiency), or the fleet kept the same replica count while per-replica throughput fell (utilization drop at constant cost = per-request cost up). Next action: decompose cost = replicas × price × hours / requests; identify which factor moved; if throughput per replica fell, investigate batching/config for the new traffic shape (e.g., adjust max batch or enable continuous batching tuning) before adding capacity. Prevention: cost per 1k requests as a deploy guardrail; a traffic-mix dashboard (prompt/output lengths, request size); utilization-based right-sizing reviews.

### C5

Evidence: the device plugin pod's logs (registration lines, device discovery), the node's `allocatable` for `nvidia.com/gpu` (vs capacity), the plugin's health state, driver/toolkit presence (`nvidia-smi` works on the host but the plugin may not have registered), and the pod's exact request/limits. Likely causes: the plugin is Running but failed to register devices (driver/library mismatch, container toolkit issue, MIG config mismatch), or the pods request a resource name/profile that this node does not advertise (e.g., MIG profile names vs `nvidia.com/gpu`). Next action: read the plugin logs for the registration/discovery lines; fix the plugin/driver/toolkit mismatch (not the scheduler); verify `allocatable` shows 8 before re-testing pods. Prevention: a GPU smoke-test DaemonSet (the GPU Operator's validator does this) that asserts device advertisement; alert on nodes with GPU capacity but zero allocatable; document the resource-name contract per node pool.

---

## Scoring

| Part | Your score | Max |
|---|---:|---:|
| A | | 40 |
| B | | 30 |
| C | | 50 |
| **Total** | | **120** |

**Pass ≥90.** A pass here means you are ready for the interview loop; the scenarios in Part C are the stories to rehearse in [12-interviews](../12-interviews/README.md). Below 90: identify the missed **concept**, not the question — then take the corresponding phase's lab again (labs beat re-reading).
