# Phase 8 Checkpoint — Observability and Reliability

> **Rules:** closed book, 90 minutes for Parts A–D. No CLI, no notes, no search. Then complete the practical sign-off on the lab stack and a disposable VM. Target: **80% overall (128/160)**, and at least **32/40** on the diagnosis scenarios.
>
> **Assumptions:** Prometheus 2.5x, Grafana 11.x, Alertmanager 0.27.x, Kubernetes 1.29+, Docker Compose v2, Ubuntu 22.04/24.04. Where a command's exact output is environment-specific, answer with the evidence you would collect and the decision rule, not a memorized string.
>
> **Safety:** the practical sign-off uses only the local lab stack and a disposable VM. Do not send test alerts to a real on-call channel; do not run L8.4 hardening against your laptop or production; do not load-test a service you do not own.

---

## Part A — Rapid facts (25 × 2 = 50 points)

1. What question does each of the four signals — metrics, logs, traces, events — answer best?
2. Why does Prometheus pull instead of requiring every service to push? Name one cost of the pull model.
3. What does `up == 0` prove, and what does it not prove?
4. Why is a counter's raw value a poor graph, and what replaces it?
5. What is the difference between `rate()` and `irate()`, and which one belongs in an SLO?
6. Why can `histogram_quantile(0.95, ...)` be wrong even when the query is syntactically correct?
7. What is cardinality, and why are user IDs, request IDs, and raw paths dangerous as metric labels?
8. What does `sum(rate(errors[5m])) / sum(rate(total[5m]))` require of the numerator and denominator?
9. What is a recording rule for, and what new failure mode does it introduce?
10. Why is a Kubernetes Secret not secret by default?
11. What is the difference between a Grafana data source `proxy` and `direct` access?
12. Why should a dashboard's first row answer "are users affected?" before showing host CPU?
13. What does `for: 5m` mean in an alert rule, and how does it interact with the evaluation interval?
14. What do `group_by`, `group_wait`, `group_interval`, and `repeat_interval` each control?
15. What is the difference between an inhibition rule and a silence?
16. Define SLI, SLO, SLA, and error budget in one sentence each.
17. Why is a p95 latency target not the same as a latency SLI expressed as a ratio?
18. What does a 99.9% SLO over 30 days allow in minutes?
19. What is a burn rate? Write the formula.
20. Why does a multiwindow burn alert require both a long and a short window?
21. State Little's law and give one capacity decision it supports.
22. Why does latency rise non-linearly as utilization approaches 100%?
23. What are the IC, ops lead, comms lead, and scribe responsible for?
24. Why is "human error" not a root cause, and what replaces it in a blameless postmortem?
25. What three checks does a TLS client perform on a server certificate?

---

## Part B — Calculations (6 × 5 = 30 points)

### B1 — Error budget and recovery

A service has a 99.95% availability SLO over a 30-day window. It serves 40 million requests in the window. In the first 10 days it serves 12 million requests and records 10,000 failures.

Answer:

1. What is the error budget in minutes and in requests?
2. What is the observed error ratio and burn rate for the first 10 days?
3. What fraction of the 30-day budget has been consumed after 10 days?
4. What is the maximum error ratio for the remaining 20 days that still meets the 30-day SLO?
5. A deploy raises the error ratio to 0.1% for 5 of the remaining 20 days, then returns to 0.025%. Does the service still meet the SLO? Show the arithmetic.

### B2 — Multwindow burn

A 99.9% SLO over 28 days.

Answer:

1. What is the error budget in minutes?
2. The 1-hour error ratio is 3%. What is the burn rate, the time to exhaust the 28-day budget at that rate, and the percentage of the budget consumed in that hour?
3. The 5-minute error ratio spikes to 20%. What is the short-window burn rate, and what percentage of the 28-day budget is consumed by those 5 minutes?
4. The 1-hour ratio is 0.5% while the 5-minute ratio is 20%. Does a fast-burn alert requiring both windows above 14.4× fire? Explain.
5. What is the maximum 1-hour error ratio that keeps the burn rate below 14.4× for this SLO?

### B3 — Capacity with failure and deploy headroom

A service peaks at 800 req/s. Demand grows 40% next quarter. Each request uses 10 ms of CPU and 60 MiB of memory. A pod has 4 vCPU and 8 GiB. Target CPU utilization is 60%. The service runs in three availability zones and must survive the loss of one AZ, with 25% spare for rolling deploys.

Answer:

1. What is the forecast peak?
2. How many cores are required at the 60% target, and how many pods by CPU?
3. How many pods by memory?
4. How many pods after the one-AZ failure requirement and the 25% deploy spare?
5. Which resource binds, and what is the per-pod request rate and memory at the final count?

### B4 — Little's law and queueing

A service receives 500 req/s with an average latency of 120 ms. Each request holds a database connection for the full 120 ms. The connection pool has 40 connections.

Answer:

1. What is the average concurrency `L` by Little's law?
2. What happens when 60 requests are in flight but the pool has 40 connections?
3. The service time is 80 ms. What is the average time in system at 85% utilization? At 95%? At 99%?
4. The p95 latency target is 500 ms. Is 85% utilization obviously safe? Explain.
5. What utilization would you target if you wanted the queueing average near 267 ms?

### B5 — Series and storage estimate

A scrape has 200 pods, 20 normalized routes, 5 status classes, and a latency histogram with 10 finite buckets. `http_requests_total` has one series per pod/route/status. The histogram has one series per pod/route/bucket plus `_sum` and `_count`. Scrape interval is 15 seconds. Assume 1.5 bytes per compressed sample.

Answer:

1. How many `http_requests_total` series are there?
2. How many histogram series are there, including `+Inf`, `_sum`, and `_count`?
3. What is the total series count, and how many samples per second are ingested?
4. How many samples are stored in 30 days, and what is the estimated compressed size?
5. What happens to the series count if the `route` label is replaced by the raw path and there are 10,000 unique paths? What would you do instead?

### B6 — Alert timing and false pages

A check runs 1,000 times per day. Each check has a 0.5% probability of a false positive, independent. The alert rule uses a 3-consecutive-failure requirement, evaluated every 15 seconds.

Answer:

1. What is the probability of at least one false positive in a day **without** the consecutive rule?
2. With the 3-consecutive rule, what is the probability that any given group of three consecutive checks is all false?
3. What is the expected number of false 3-check runs per day (approximately, treating runs as independent)?
4. If a true failure starts immediately after an evaluation, what is the minimum detection delay with `for: 3m` and a 15-second evaluation interval?
5. Why is a 0.5% per-check false-positive rate still a problem for a paging alert, even with a consecutive rule?

---

## Part C — Diagnosis scenarios (8 × 5 = 40 points)

For each scenario, answer:

- What evidence do you collect first?
- What are the two most likely causes?
- What is the smallest safe next action?
- What would you change to prevent recurrence?

### C1 — The dashboard is empty, but the app is running

A teammate says the checkout dashboard has no data. `curl http://app:8000/metrics` works from their laptop. The app container is healthy. Prometheus shows the `app` target as `down` with `connection refused`.

### C2 — p95 is 0.3 seconds, then 2.5 seconds, then 0

The p95 panel jumps between 0.3, 2.5, and 0. The service receives about 20 requests per minute. The histogram has buckets at 0.1, 0.5, 1, 2.5, and +Inf. The 300 ms SLO threshold is not a bucket boundary.

### C3 — The alert fires, but nobody is notified

Prometheus shows `AppHighErrorRatio` as `firing`. The on-call says they received nothing. Alertmanager shows no active alerts. The receiver is a webhook.

### C4 — The SLO is green, but users are complaining

The availability SLO is 99.98%, but support tickets say checkout is slow. The SLI is HTTP 200 rate on `/health`. The latency SLO is p95 < 2 seconds, measured over all routes.

### C5 — The burn-rate alert never fires

During a 10-minute drill, the error ratio is 30%. The fast-burn alert threshold is 14.4×, with `for: 2m` and a 5m/1m window. The alert stays inactive. The rule shows `health=ok`.

### C6 — Capacity says 70%, but latency is failing

CPU is 70% on every node. p99 latency is four times the SLO. Pod CPU requests are 200m and limits are 500m. `kubectl top` shows 450m per pod. The database shows high connection wait.

### C7 — TLS works in the browser, fails in `curl`

`https://api.example.com` opens in a browser. `curl` fails with `unable to get local issuer certificate`. The certificate is not expired, and the hostname matches.

### C8 — A privileged pod and a leaked key

A namespace contains a pod with `privileged: true` and a mounted service account token. A separate repository has an AWS access key in its Git history, deleted from the working tree but still present in an old commit.

---

## Part D — Design questions (2 × 20 = 40 points)

### D1 — Observability and reliability for checkout

Design observability and reliability for a checkout service with:

- 2,000 req/s peak, 10 million requests/day;
- three availability zones;
- a 99.9% availability SLO and a 300 ms p95 latency target;
- dependencies: payment provider, database, inventory service;
- a team of six engineers and a weekly release cadence.

Cover:

1. The SLIs, exact good/total definitions, and exclusions.
2. The metric names, types, and labels you would instrument; what you refuse to put in labels.
3. Scrape/evaluation intervals, retention, and recording rules.
4. The dashboard's first two rows and the question each panel answers.
5. The alert set, including symptom vs cause, multiwindow burn, absence, and dependency alerts.
6. The error budget policy and release consequences.
7. A capacity estimate with headroom and failure assumptions.
8. The incident roles, communication cadence, and postmortem inputs.

### D2 — Security for a multi-tenant Kubernetes platform

Design the security controls for a platform that runs 30 teams' workloads on one Kubernetes cluster, with a CI/CD pipeline deploying to production.

Cover:

1. The threat model: assets, trust boundaries, and the top three blast-radius scenarios.
2. Human and workload identity: SSO, RBAC, service accounts, cloud roles, and CI OIDC.
3. Network isolation and egress control, including the CNI enforcement caveat.
4. Pod security and admission control.
5. Secrets management and rotation.
6. Image and supply-chain controls.
7. Audit logging and detection.
8. A security incident runbook for a compromised pod and a leaked CI token.
9. Three controls you would verify first, and how you would test them.

---

## Answer key

Stop here if you have not attempted Parts A–D. Score each answer against the key, not against your intent.

### Part A

1. Metrics: aggregate trends and thresholds ("is it fast/erroring/saturated?"). Logs: detailed events and context ("why did this request fail?"). Traces: request path and dependency latency ("where did the time go?"). Events: discrete changes and audit ("who changed what, when?").
2. Pull makes target health observable, avoids exposing a push endpoint, and lets the server control scrape cadence. Cost: the server needs network reachability to every target, and short-lived jobs need a push gateway or a different design.
3. `up == 0` proves the last scrape failed. It does not prove users are affected; a load balancer may serve from other instances, or the app may be up but returning errors (`up == 1`).
4. A counter's raw value shows uptime, not rate. Use `rate()` or `increase()` over a window; use the raw value only for "has it ever moved" or reset detection.
5. `rate()` averages over the window and handles resets; `irate()` uses the last two samples and is noisier. Use `rate()` for SLOs and alerts; `irate()` for a "right now" diagnostic panel.
6. Bucket layout and sample count. `histogram_quantile` interpolates within a bucket; if the threshold is not a boundary or there are few samples, the estimate is unreliable. Aggregating percentiles across instances is also invalid.
7. Cardinality is the number of unique label combinations (series). User IDs, request IDs, and raw paths create unbounded series, increasing memory, disk, scrape cost, and query time. Put them in logs/traces.
8. The numerator and denominator must cover the same population (same job/service, same exclusions, same window). Otherwise the ratio is diluted or inflated.
9. A recording rule precomputes an expression and writes a named series. It can fail or go stale independently of the raw data; check rule health and `lastEvaluation`.
10. A Kubernetes Secret is base64-encoded, stored in etcd, and readable by anyone with `get secrets` in the namespace. Encryption at rest and RBAC are separate controls.
11. `proxy` sends the query from the Grafana server; `direct` sends it from the browser. `direct` needs browser network reachability and can expose the data source; `proxy` is the usual choice.
12. Because host CPU is a cause, not a symptom. Users care about errors, latency, and availability. The first row should show user impact; lower rows narrow the fault domain.
13. The expression must be true for the entire `for` duration, evaluated at each interval. With a 15s interval and `for: 5m`, that is about 20 consecutive evaluations; a single false evaluation resets the timer.
14. `group_by` chooses the labels that define a notification group; `group_wait` waits before the first notification; `group_interval` controls updates to an existing group; `repeat_interval` controls reminders for a still-firing alert.
15. Inhibition suppresses a target alert while a related source alert is active (a causal rule); a silence suppresses matching alerts for a fixed duration with a comment (a human override).
16. SLI: a quantitative measure of service behavior. SLO: a target range for an SLI over a window. SLA: a contract with a customer, often with penalties. Error budget: `1 - SLO`, expressed as allowed bad events or time.
17. A p95 is a point in the distribution; an SLI ratio says what fraction of requests met the threshold. The ratio is directly comparable to the SLO and has a clear denominator; the p95 can be below target while 10% of users are far above it.
18. `0.1% × 30 × 24 × 60 = 43.2 minutes`.
19. `burn_rate = observed_error_ratio / (1 - SLO)`. It compares the current badness to the allowed badness.
20. The long window proves the burn is sustained; the short window proves it is still happening and allows fast resolution. Requiring both reduces flapping and stale pages.
21. `L = λW`: concurrency equals arrival rate times time in system. Example: 500 req/s × 120 ms = 60 concurrent requests, which sets the minimum connection pool/worker count.
22. Queueing: as utilization approaches 1, the wait time `S/(1-ρ)` grows without bound. Small load increases near saturation produce large latency increases.
23. IC: owns the incident, severity, priorities, roles, and risky mitigation approval. Ops lead: technical investigation and mitigation. Comms lead: status updates and stakeholder/customer messaging. Scribe: timeline, decisions, evidence, action items.
24. "Human error" stops analysis at the person. Replace it with contributing factors: what made the action easy, likely, undetected, or hard to mitigate. Fix the system, not the character.
25. Chain to a trusted CA, validity period, and hostname/SAN match. Optionally protocol/cipher and revocation, depending on the client.

### Part B

#### B1

1. Budget = `0.05% × 30 days = 0.0005 × 43,200 min = 21.6 minutes`. In requests: `0.0005 × 40,000,000 = 20,000` bad requests.
2. Observed error ratio = `10,000 / 12,000,000 = 0.000833 = 0.0833%`. Burn = `0.000833 / 0.0005 = 1.667×`.
3. Request-based: `10,000 / 20,000 = 50%` of the budget is consumed. The burn-rate estimate `burn × elapsed / window = 1.667 × 10/30 = 55.6%` assumes traffic continues at the rate that produced the burn; the first 10 days served 12M requests, below the 30-day average of 1.333M/day, so the actual request-based consumption is lower. This gap is why you should state whether the budget is tracked in requests or in time.
4. Remaining budget = `20,000 - 10,000 = 10,000` bad requests over the remaining 28 million requests. Max ratio = `10,000 / 28,000,000 = 0.000357 = 0.0357%`. (Using the full 40M total: remaining requests = 28M, not 30M, because 12M were in the first 10 days.)
5. The remaining 20 days carry 28M requests. The 5-day deploy serves `28M × 5/20 = 7M` requests at 0.1% = 7,000 bad. The other 15 days serve `28M × 15/20 = 21M` requests at 0.025% = 5,250 bad. Total = `10,000 + 7,000 + 5,250 = 22,250` bad, which exceeds the 20,000 budget. The service misses the SLO by 2,250 bad requests (0.005625% of 40M). If the 0.025% rate continued for all 20 days instead, bad = `10,000 + 28M × 0.00025 = 17,000`, which is under budget; the 5-day deploy is what pushes it over.

#### B2

1. Budget = `0.1% × 28 days = 0.001 × 40,320 min = 40.32 minutes`.
2. Burn = `3% / 0.1% = 30×`. Time to exhaust = `28 days / 30 = 0.933 days = 22.4 hours`. Budget consumed in 1 hour = `30 × 1 / (28×24) = 30/672 = 0.0446 = 4.46%`.
3. Burn = `20% / 0.1% = 200×`. Budget consumed in 5 minutes = `200 × 5 / 40,320 = 0.0248 = 2.48%`.
4. No. The 1-hour ratio is 0.5%, so its burn is `0.5% / 0.1% = 5×`, below 14.4×. The long window fails, so the `and` condition does not fire. The 5-minute spike may be a blip, a load-generator artifact, or a short outage; the long window says it is not yet a sustained budget burn. A slow-burn/ticket rule or a shorter-window warning may still catch it, depending on policy.
5. Max ratio = `14.4 × 0.1% = 1.44%`.

#### B3

1. Forecast = `800 × 1.40 = 1,120 req/s`.
2. CPU demand = `1,120 × 0.010 = 11.2 cores`. At 60%: `11.2 / 0.60 = 18.67 → 19 cores`. Pods by CPU = `ceil(19/4) = 5`.
3. Memory = `1,120 × 60 MiB = 67,200 MiB = 65.625 GiB`. At 8 GiB per pod: `ceil(65.625/8) = 9` pods.
4. One-AZ failure: total pods N are spread across 3 AZs, so losing one AZ leaves `2N/3` capacity. Need `2N/3 ≥ 9` → `N ≥ 13.5` → 14 pods. Spread 14 as 5/5/4; losing a 5-pod AZ leaves 9, exactly the requirement.
5. Deploy spare: `ceil(14 × 1.25) = 18` pods. Per-pod request rate = `1,120/18 = 62.2 req/s`; CPU per pod = `62.2 × 0.010 = 0.622 cores`; memory per pod = `65.625/18 = 3.65 GiB`. The pod has 4 vCPU/8 GiB, so it fits, but memory is the binding resource and the utilization is ~46% (3.65/8) before other overhead. The final count is driven by memory plus the AZ/deploy assumptions, not CPU.

#### B4

1. `L = 500 × 0.120 = 60` concurrent requests.
2. The pool of 40 cannot serve 60 concurrent requests; 20 requests wait for a connection. Latency rises above 120 ms even if the database is healthy. The pool is a concurrency bottleneck, not a CPU bottleneck.
3. `W = S/(1-ρ)` with `S = 0.08s`:
   - 85%: `0.08/0.15 = 0.533s` (533 ms).
   - 95%: `0.08/0.05 = 1.6s`.
   - 99%: `0.08/0.01 = 8s`.
4. No. At 85%, the average wait is already 533 ms, above the 500 ms p95 target. An average above the target means a large fraction is above it. Even 80% gives `0.08/0.2 = 400 ms` average, leaving little room for the tail.
5. `0.08/(1-ρ) = 0.267` → `1-ρ = 0.08/0.267 = 0.30` → `ρ = 0.70`. Target around 70%, with additional headroom for bursts.

#### B5

1. `200 × 20 × 5 = 20,000` series.
2. Per pod/route: 10 finite buckets + 1 `+Inf` + `_sum` + `_count` = 13 series. `200 × 20 × 13 = 52,000` series.
3. Total = `72,000` series. Samples/sec = `72,000 / 15 = 4,800`.
4. 30 days = `2,592,000` seconds; samples = `4,800 × 2,592,000 = 12,441,600,000` (12.44 billion). At 1.5 bytes/sample = `18.66 GB` compressed, before index/WAL/overhead. Treat this as a lower bound.
5. Raw paths: `200 × 10,000 × 5 = 10,000,000` series for the counter alone, plus the histogram explosion. That is a cardinality incident. Normalize the path to a route template (`/users/:id`) and put the raw path in logs/traces. Add a series-count alert and a label budget.

#### B6

1. `1 - 0.995^1000 ≈ 1 - 0.00665 = 0.99335`, about 99.3%. A false page is almost certain.
2. `0.005^3 = 1.25 × 10^-7`.
3. There are about 1,000 overlapping 3-check windows per day (roughly; checks are not independent across overlapping windows). Expected false runs ≈ `1,000 × 1.25e-7 = 1.25e-4`, about one every 8,000 days. Treating overlapping runs as independent is an approximation; the true rate is of the same order.
4. The condition can start just after an evaluation, so the next evaluation is up to 15s later; then `for: 3m` must be satisfied, so the minimum is about `3m + 15s = 3m15s`. In practice, add scrape/evaluation jitter.
5. A 0.5% per-check rate is high for a paging signal because there are many checks and many alerts. The consecutive rule reduces independent flips, but correlated false positives (a shared dependency, a bad deploy, a scrape timeout) are not independent and can still page. Also, a 0.5% rate on 1,000 checks means ~5 false positives/day before the consecutive rule; if they cluster, the rule does not save you. Measure and reduce the per-check false rate; do not rely only on `for`.

### Part C

Each scenario is scored out of 5: evidence (2), likely causes (1), next action (1), prevention (1). Award partial credit for a coherent decision rule.

#### C1

Evidence: target `lastError`, scrape URL, DNS from the Prometheus container, app logs/port, network policy, and whether the app is listening on `0.0.0.0` vs `127.0.0.1`. `curl` from a laptop can succeed while the Prometheus container cannot reach `app:8000` due to DNS/network namespace. Likely causes: wrong target (`localhost` inside the container), app bound to loopback, or a network policy. Next action: fix the scrape address/DNS, not restart Prometheus. Prevention: a target-health alert and a synthetic query in CI.

#### C2

Evidence: sample count (`_count` rate), bucket layout, scrape interval, whether the 300 ms threshold is a boundary, and whether the p95 is computed from aggregated buckets. Likely causes: too few samples, bucket interpolation, and a window shorter than the scrape/query dynamics. Next action: use a longer window or a ratio SLI with a minimum traffic guard; add a 0.3 bucket. Prevention: a dashboard panel that shows request count next to p95, and a rule test with low-traffic series.

#### C3

Evidence: Prometheus alert state, `alerting.alertmanagers`, Alertmanager `/api/v2/alerts`, Alertmanager logs, silences/inhibitions, receiver URL/DNS, webhook logs. Likely causes: missing/incorrect `alerting` block, wrong receiver URL from inside the container, or a silence. Next action: fix the first broken arrow (Prometheus → Alertmanager → receiver), then send a test alert. Prevention: a synthetic canary alert that runs daily and asserts a notification; monitor Alertmanager's notification failures.

#### C4

Evidence: SLI definition, route coverage, status-code treatment, synthetic vs real traffic, client-side latency, health-check exclusion, and the latency SLO's aggregation. Likely causes: the availability SLI measures `/health` (which is always 200), the latency SLO is p95 over all routes and hides checkout, or slow requests still return 200. Next action: redefine the SLI around the checkout journey and a latency threshold; add a synthetic checkout probe. Prevention: review SLIs against support tickets quarterly; require an SLI spec for every user journey.

#### C5

Evidence: rule health/last evaluation, the raw ratio, the threshold, the `for`, both windows, labels, and Alertmanager. Likely causes: the threshold is for a different SLO (0.144 applies to 99%, not 99.9%), the denominator includes health checks, the `for` is longer than the drill, or one window never breaches. Next action: run the expression without the comparison to see the value; fix the denominator/threshold. Prevention: `promtool` tests for the exact SLO; a drill that asserts firing and resolution.

#### C6

Evidence: CPU throttling (`container_cpu_cfs_throttled_seconds_total`), requests/limits, actual usage, run queue, memory/GC, disk IOPS/latency, connection pool wait, and per-pod traffic distribution. Likely causes: CPU limits causing throttling at 500m while usage is 450m; a connection pool bottleneck; a hot shard; queueing at 70% average because the average hides bursts. Next action: find the saturated resource with USE; raise the limit or add capacity only if CPU is the bottleneck. Prevention: alert on throttling and saturation, not just average CPU; load-test at the target utilization.

#### C7

Evidence: `openssl s_client -showcerts`, the chain order, `curl -v` trust store, and whether the server sends the intermediate. Likely causes: missing intermediate certificate; the browser cached it or fetched it via AIA, while `curl` did not. Next action: install the full chain (leaf + intermediate) on the server/load balancer. Prevention: automate chain validation from an external probe with the same client stack; monitor expiry and chain completeness.

#### C8

Evidence: pod spec (`privileged`, hostPath, hostNetwork, token automount), RBAC for the service account, node role/instance profile, Git history (`gitleaks`, `git log -S`), and provider audit logs for the key. Likely causes: a debug pod left running, over-broad node role, and a secret committed then deleted without rotation. Next action: isolate/delete the pod, revoke the key, rotate everything it could reach, and audit usage. Prevention: restricted pod security/admission, workload identity with scoped roles, pre-commit/CI secret scanning, and a revoke-first runbook.

### Part D

#### D1 — Model answer

**SLIs.** Availability: `non-5xx / all requests` for the checkout route, excluding health checks and synthetic probes; include retries and timeouts in the denominator. Latency: fraction of checkout requests under 300 ms, measured from the client or the edge, not only the server. Dependency SLIs: payment success rate, DB query latency, inventory error ratio. A correctness SLI: orders acknowledged vs orders persisted (audited, not continuously computed).

**Metrics.** `http_requests_total{route,status,method,version}` counter; `http_request_duration_seconds` histogram with buckets around 0.1, 0.2, 0.3, 0.5, 1, 2, 5; `http_inflight_requests` gauge; `dependency_requests_total{name,status}`; `dependency_duration_seconds`; `queue_depth`; `db_pool_in_use`; `build_info{version}`. Do not label by user ID, request ID, raw path, email, or error text. Keep `route` normalized. Include `version` for canary comparison, but bound the cardinality.

**Scrape/retention.** 15s scrape/evaluation for 2,000 req/s; 30–90 days local retention or remote write for a year; recording rules for the SLI ratios and burn rates; `promtool` tests. Retain raw metrics long enough for a quarterly trend, and keep incident snapshots for the postmortem.

**Dashboard.** Row 1: availability, error ratio, p95/p99 latency, request rate, budget remaining. Row 2: errors by route/status/version, latency by route, dependency error/latency, saturation (CPU throttling, memory, DB pool, queue). The top row answers "are users affected?"; the second row narrows the fault domain.

**Alerts.** Page on fast burn (2% budget in 1h, 14.4×, 5m/1m confirmation), slow burn (5% in 6h, 6×, 30m/6h), target absence, and dependency SLO breach. Ticket on capacity trends and slow burn. Do not page on CPU alone. Every alert has severity, owner, runbook, and a tested notification path. Inhibit warnings when a critical for the same service is active; silence with an expiry.

**Error budget policy.** 99.9% over 28 days = 40.3 minutes. Freeze non-emergency changes when the budget is exhausted; review weekly; exceptions require an exec sponsor and a documented expiry. The SLO is stricter than the payment provider's SLA; a dependency outage consumes the budget and triggers a dependency review.

**Capacity.** 2,000 req/s peak; 10ms CPU/request → 20 cores; at 60% → 34 cores; 4 vCPU pods → 9 pods; memory at 60 MiB/request → 117.2 GiB → 15 pods at 8 GiB; N+1 across 3 AZs → 23 pods; 25% deploy spare → 29 pods. The exact numbers depend on measurements; state the binding resource and the assumptions. Add queueing headroom and load-test the autoscaling path.

**Incident.** IC/ops/comms/scribe; 15-minute updates for SEV1, 30 for SEV2; status page; evidence preservation; rollback/feature flag first; postmortem within 5 business days with 3–5 verified actions.

Scoring: 20 points across the eight areas; full credit requires specific numbers/labels and a failure mode for each control, not just a tool list.

#### D2 — Model answer

**Threat model.** Assets: tenant data, cluster credentials, image registry, CI tokens, cloud account. Trust boundaries: internet → ingress; tenant namespace → kubelet/API; CI → registry → production; pod → metadata service. Top blast radii: a privileged pod escaping to the node and reaching other tenants; a leaked CI token deploying a malicious image; a public bucket or secret exposing tenant data.

**Identity.** Human SSO with MFA and short-lived sessions; RBAC with namespace-scoped roles, no wildcards for tenant users; separate cluster-admin break-glass with alerting. Workload identity (IRSA/Workload Identity) per service, not node-wide instance roles. CI uses OIDC federation with a `sub` restricted to the protected branch/environment, environment approvals, and no static keys. Service accounts disable automount unless the pod needs the API.

**Network.** Default-deny ingress/egress per namespace; allow DNS, required services, and package/registry egress; block metadata where possible; require IMDSv2 with hop limit 1. Test that the CNI enforces NetworkPolicy; if it does not, use a CNI that does or a service mesh. Separate system and tenant namespaces; no cross-tenant `hostNetwork`/`hostPath`.

**Pod security.** Enforce restricted PSS in tenant namespaces: non-root, no privilege escalation, drop ALL capabilities, read-only root FS, seccomp RuntimeDefault, resource limits. Admission policy blocks privileged, hostPath, hostPID/hostNetwork, `latest` tags, and unsigned images outside an audited system namespace. PodDisruptionBudgets and quotas per tenant.

**Secrets.** External secrets from a manager with per-namespace access; etcd encryption at rest; no secrets in images/Git/logs; short-lived tokens; rotation automated and tested; `get secrets` restricted and audited.

**Supply chain.** Build once, pin by digest, sign images (cosign/notation), verify at admission, generate an SBOM, scan for CVEs and secrets, use a private registry/proxy with allowlisting, and rotate build credentials. Do not let a PR from a fork run with production secrets.

**Audit/detection.** Kubernetes API audit (including exec and secret access), cloud audit logs, admission decisions, image deploys, and network flow logs, shipped off-cluster with retention. Alert on privileged pod creation, new cluster-admin binding, public bucket, secret access spikes, unusual egress, and unsigned images.

**Incident.** Compromised pod: isolate the namespace/node, preserve logs and pod YAML, revoke the service account and cloud role, hunt for other pods with the same image/token, rebuild from a trusted image. Leaked CI token: revoke first, audit deployments/registry pushes, rotate dependent secrets, restrict the trust policy, and verify with a dry-run. Chain of custody and need-to-know communication.

**First three controls to verify.** (1) Can a tenant create a privileged/hostPath pod? Try it and confirm admission denies it. (2) Can a pod reach the metadata service or another namespace? Test with a network probe. (3) Can a CI job from an unprotected branch assume the deploy role? Attempt it in a sandbox and confirm denial. Each test has a clear pass/fail and produces evidence.

Scoring: 20 points; full credit requires specific controls, a failure mode for each, and verification tests.

---

## Practical sign-off (must pass all four)

The written score is necessary but not sufficient. Bring evidence; "I did it" is not evidence.

### L8.1 — Metrics and PromQL

- [ ] `up` is 1 for Prometheus, node-exporter, and app; you can explain what `up=0` does and does not prove.
- [ ] You wrote and interpreted `rate`, `increase`, error ratio, p95, and the 300 ms good ratio.
- [ ] You demonstrated a counter reset and explained why `rate` still works.
- [ ] You added a recording rule, reloaded Prometheus, and showed `health=ok`.
- [ ] `promtool test rules` passes; you can explain what the test asserts.
- [ ] You diagnosed the target-down or NaN fault with evidence, not a restart.

### L8.2 — Dashboards and alerting

- [ ] The dashboard's first row answers "are users affected?" in under 30 seconds.
- [ ] The dashboard uses a variable and a 5-minute time range without breaking.
- [ ] `AppTargetDown` fires after ~1 minute and resolves after restart.
- [ ] `AppHighErrorRatio` requires both windows and fires after `for`.
- [ ] Alertmanager routes to the local webhook; you showed the firing and resolved payloads.
- [ ] A silence suppresses the alert for a fixed duration and expires.
- [ ] Inhibition suppresses a warning while the critical is active.
- [ ] You can explain why "firing in Prometheus" ≠ "human notified."

### L8.3 — SLO and incident drill

- [ ] The SLO spec has SLI, denominator, window, target, and budget policy.
- [ ] The budget arithmetic (36 seconds for 99% / 1h) is correct.
- [ ] The burn arithmetic (20% = 20×, 3 minutes to exhaust) is correct.
- [ ] The fast-burn alert fires in the drill and resolves after recovery.
- [ ] The incident has a timeline, roles, a comms update, a mitigation, and a postmortem.
- [ ] The postmortem has 3–5 actions with owners, dates, and verification.
- [ ] The capacity answer identifies the binding resource and the failure assumption.

### L8.4 — Security audit

- [ ] You audited listening ports, SSH config, sudo, SUID/capabilities, and writable files on a disposable VM.
- [ ] You found and removed the over-broad sudo test rule.
- [ ] You verified a TLS certificate with the correct hostname and saw the wrong-hostname failure.
- [ ] You found a secret in Git history and stated that rotation comes first.
- [ ] You identified a privileged/root pod and checked RBAC from the service account's perspective (or completed the paper alternative).
- [ ] You tested NetworkPolicy enforcement rather than assuming it.
- [ ] You fixed three findings with evidence, verification, and rollback.
- [ ] You did not lock yourself out; cleanup is complete.

---

## Oral exam (10 questions, pass 7/10)

Answer aloud, in under two minutes each, without notes.

1. A dashboard shows CPU at 95% but the SLO is green. What do you do, and what do you not do?
2. Your p95 latency alert fires every night at 02:00 for 10 minutes. How do you decide whether to tune the alert or fix the system?
3. Explain how you would test that an alert reaches a human, end to end.
4. An engineer proposes adding `user_id` to every metric to debug a problem. What do you say, and what do you offer instead?
5. Your 99.9% SLO has 10 minutes of budget left with 10 days to go. What changes about releases, and who decides?
6. A dependency with a 99.9% SLA is part of your 99.99% SLO. Is the SLO achievable? What would you do?
7. Walk through the first 10 minutes of a SEV1 where checkout is returning 500s and the last deploy was 5 minutes ago.
8. A pod is compromised. What do you preserve, what do you revoke, and what do you not do?
9. How would you detect a cardinality explosion before it takes down Prometheus?
10. Your capacity model says 60% utilization, but the service falls over at 70%. What did the model miss?

---

## Retest log

| Date | Part A | Part B | Part C | Part D | Total | Practical | Notes |
|---|---:|---:|---:|---:|---:|---|---|
| | /50 | /30 | /40 | /40 | /160 | pass/fail | |
| | | | | | | | |
| | | | | | | | |

A retest is a new set of questions or a new drill, not the same paper memorized. If you miss the same concept twice, write a one-page explanation and teach it back before the next attempt.

---

## Scoring guide

| Band | Score | What it means |
|---|---:|---|
| Strong | 144–160 | Can design and defend; ready to operate with supervision |
| Pass | 128–143 | Solid fundamentals; revisit the missed scenarios |
| Near | 112–127 | Knows the tools, weak on failure modes and trade-offs |
| Not yet | <112 | Re-study the phase; do not substitute tool tutorials for the fundamentals |

A pass requires at least 32/40 on Part C. A candidate who can recite PromQL but cannot diagnose a missing notification or a capacity cliff is not ready.
