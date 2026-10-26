# SLOs, Error Budgets, Capacity, and Incidents

> **The one-sentence model:** an SLO turns "healthy" into a ratio with a deadline; the error budget is the permission to change things; capacity planning decides whether the system can keep that promise; and incident response is how you spend the budget deliberately instead of discovering the balance after the outage.

---

## 1. The vocabulary, without the marketing

| Term | Definition | Example | Common mistake |
|---|---|---|---|
| **SLI** | A quantitative measure of service behavior | proportion of requests served < 300 ms | Measuring CPU and calling it reliability |
| **SLO** | A target range for an SLI over a window | 99.9% of requests < 300 ms over 28 days | Setting a target with no consequence |
| **SLA** | A contract with a customer, often with penalties | 99.9% monthly uptime, service credits | Treating the SLA as the internal target |
| **Error budget** | `1 - SLO` expressed as allowed bad events or time | 0.1% of 30 days = 43.2 minutes | Spending it without review |
| **Burn rate** | How fast the budget is being consumed relative to the SLO window | 14.4× means the month's budget would be gone in ~2 days | Alerting on raw error count |
| **Good event** | A request/operation that meets the SLI definition | HTTP 200 within 300 ms | Counting 4xx client errors as failures |
| **Total event** | The valid denominator | all non-health-check requests | Excluding slow requests from the denominator |
| **Window** | The period over which the ratio is computed | rolling 28 days | Comparing a 7-day SLO to a 30-day budget |

### SLI, SLO, SLA in one picture

```text
          measured                         promised internally              promised to customer
   ┌──────────────────┐              ┌──────────────────────┐          ┌──────────────────────┐
   │ good / total     │─────────────►│ SLO 99.9% / 28 days  │─────────►│ SLA 99.5% / month    │
   │ (the SLI)        │              │ error budget 0.1%    │          │ credits if missed    │
   └──────────────────┘              └──────────────────────┘          └──────────────────────┘
            ▲                                  ▲                                  ▲
       instrumentation                  internal agreement                 legal/commercial
```

The SLO should be **stricter than the SLA**, because the SLA is the point where money changes hands, not the point where you start caring. The gap is your operational margin.

### An SLO specification

```yaml
service: checkout-api
owner: payments-platform
user_journey: "A signed-in user completes checkout"
sli:
  type: ratio
  good: "requests with status !~ 5.. AND duration <= 300ms (see the PromQL below)"
  total: "all requests for route=/checkout, excluding health checks"
  unit: request
slo:
  target: 0.999
  window: 28d
  compliance: rolling
error_budget_policy:
  freeze: "at 100% budget consumed, no non-emergency deploys to checkout until the window recovers"
  review: "weekly budget review; monthly trend review"
alerting:
  fast_burn: "2% of budget in 1h AND 5m confirmation"
  slow_burn: "5% of budget in 6h AND 30m confirmation"
runbook: runbooks/checkout-slo.md
```

If you cannot fill every field, you have a dashboard, not an SLO.

---

## 2. Choosing an SLI users would recognize

### The five SLI families

| Family | Good event | Use when | Watch out |
|---|---|---|---|
| **Availability** | request succeeded | request/response service | Define which status codes count as good |
| **Latency** | request completed within threshold | interactive service | Percentile and threshold must match user expectation |
| **Throughput** | job processed at least N items/sec | batch/streaming | A fast but incomplete job is not good |
| **Freshness** | data is newer than X | pipelines, caches, dashboards | Measure event time vs processing time |
| **Durability/correctness** | acknowledged write survives / result matches | storage, payments | Hard to measure continuously; use audits and probes |

### Ratio, not average

An average latency of 200 ms can hide a 10-second p99. Users do not experience averages; they experience their own request. Define the SLI as a **ratio of requests meeting a threshold**:

```promql
# Good: proportion of requests served under 300 ms
sum(rate(http_request_duration_seconds_bucket{job="checkout",le="0.3"}[5m]))
/
sum(rate(http_request_duration_seconds_count{job="checkout"}[5m]))
```

This is better than `histogram_quantile(0.95, ...)` for an SLO because it directly answers "what fraction of users got the experience we promised?" Percentiles are still useful for diagnosis.

### What belongs in the denominator

Decide explicitly:

- Include requests from real users, including retries? Usually yes for availability, because retries are load.
- Exclude synthetic health checks? Usually yes; they are not user journeys.
- Exclude `4xx`? Usually yes for availability, because a bad password is not an outage — but **not** for a "successful checkout" SLI, where a rejected payment is a failed journey.
- Include requests that never reached the server? If the load balancer has the data, yes; otherwise you are measuring only the part that survived.
- Include requests with no response due to timeout? Yes. A timeout is a bad event, not a missing one.

The denominator is a design decision. Write it down and keep it stable; changing it silently resets the meaning of the SLO.

### Latency thresholds

Pick the threshold from the user journey, not from the current p95. If checkout feels broken at 2 seconds, a 300 ms SLO is aspiration, not measurement. A useful progression:

1. Measure the current distribution for a week.
2. Choose a threshold that separates "fine" from "annoying" for the journey.
3. Set a target slightly better than current, with a cost estimate for getting there.
4. Revisit quarterly or after a major architecture change.

### Percentile caveats

- `histogram_quantile` interpolates within buckets. With buckets at 0.1, 0.5, 1, 5 seconds, a "p95 of 0.9 s" may be an artifact of the 0.5–1 s bucket.
- Aggregating percentiles across instances is invalid. `avg(p95)` is not the fleet p95. Aggregate the buckets, then compute the quantile.
- A p99 with 100 requests/hour is statistically noisy. Use ratios over longer windows or a coarser threshold.

---

## 3. Windows and the arithmetic of error budgets

### Budget table

For a 30-day window (43,200 minutes):

| SLO | Error budget | Equivalent downtime | Bad events per 1M |
|---:|---:|---:|---:|
| 99% | 1% | 7 h 12 m | 10,000 |
| 99.5% | 0.5% | 3 h 36 m | 5,000 |
| 99.9% | 0.1% | 43 m 12 s | 1,000 |
| 99.95% | 0.05% | 21 m 36 s | 500 |
| 99.99% | 0.01% | 4 m 19 s | 100 |
| 99.999% | 0.001% | 26 s | 10 |

For a 28-day window, multiply the 30-day minutes by `28/30`:

- 99.9% × 28 days = 40 min 19 s.
- 99.95% × 28 days = 20 min 10 s.

For a 7-day window, 99.9% = 10 min 5 s. A short window makes the budget easier to exhaust and reacts faster; a long window smooths noise but hides regressions. Most teams run a **long window for the contract** and **short windows for alerts**.

### Rolling vs calendar

| Window | Behavior | Good for | Bad for |
|---|---|---|---|
| **Rolling 28d** | Always looks back 28 days | Stable contract, fair to recent recovery | Slow to recover after a bad week |
| **Calendar month** | Resets on the 1st | Reporting, billing alignment | A outage on the 1st affects the whole month; a outage on the 30th barely counts |
| **Request-based** | Budget per N requests | High-traffic services, statistically stable | Low-traffic services; a quiet period hides issues |
| **Fixed sprint** | Resets each sprint | Team cadence | Gaming the reset |

Pick one and state it. Do not switch windows when the number looks bad.

### Error budget as a percentage of requests

If the SLO is 99.9% and you serve 10 million requests in the window, the budget is 10,000 bad requests. If you have already served 8,000 bad requests, you have 2,000 left, regardless of how much time remains. Time-based and request-based budgets are two views of the same ratio; use the one your team can act on.

### Worked example

A service has a 99.9% availability SLO over 28 days. In the first 7 days it serves 20 million requests and 60,000 fail.

1. Budget for 28 days at 99.9% = 0.1% of total requests. You do not know the 28-day total yet; use the time-based equivalent for a quick check: 40.3 minutes.
2. Error ratio so far = 60,000 / 20,000,000 = 0.3%.
3. Burn rate = 0.3% / 0.1% = 3×.
4. At 3× burn for 7 days, you consumed 21 days' worth of budget (3 × 7 = 21 of 28). Remaining ≈ 7 days of budget.
5. Projected 28-day error ratio if the rate continues ≈ 0.3%, which is 3× the SLO. You will miss the SLO unless the rate drops.
6. Decision: freeze non-emergency deploys, find the top error source, and set a recovery target (for example, error ratio < 0.05% for the next 7 days) that brings the window back under budget.

Notice that you did not need a perfect forecast. You needed the ratio, the burn rate, and a decision rule.

---

## 4. Burn-rate alerting

### The formula

```text
burn_rate = observed_error_ratio / (1 - SLO)

time_to_exhaust_budget = window / burn_rate
```

For a 30-day window and a 99.9% SLO:

| Observed error ratio | Burn rate | Time to exhaust 30d budget |
|---:|---:|---:|
| 0.1% | 1× | 30 days |
| 0.2% | 2× | 15 days |
| 1% | 10× | 3 days |
| 2% | 20× | 36 hours |
| 5% | 50× | 14.4 hours |
| 10% | 100× | 7.2 hours |
| 100% | 1000× | 43.2 minutes |

### Multwindow alerts

A single long window is slow; a single short window is noisy. Combine a long window (to prove the burn is real) with a short window (to prove it is still happening). A common policy for a 30-day SLO:

| Severity | Long window | Short window | Burn threshold | Budget consumed | Action |
|---|---|---:|---:|---:|---|
| Page | 1 h | 5 m | 14.4× | 2% in 1 h | Wake someone now |
| Page | 6 h | 30 m | 6× | 5% in 6 h | Wake someone now |
| Ticket | 3 d | 6 h | 1× | 10% in 3 d | Investigate next business day |
| Ticket | 7 d | 1 d | 1× | ~23% in 7 d | Trend/capacity review |

Both windows must breach. The short window prevents a page for a problem that already stopped; the long window prevents a page for a 30-second blip.

```promql
# Fast burn: 2% of a 30-day budget in 1 hour, confirmed over 5 minutes.
(
  sum(rate(http_requests_total{job="checkout",status=~"5.."}[1h]))
  /
  sum(rate(http_requests_total{job="checkout"}[1h]))
) > (14.4 * 0.001)
and
(
  sum(rate(http_requests_total{job="checkout",status=~"5.."}[5m]))
  /
  sum(rate(http_requests_total{job="checkout"}[5m]))
) > (14.4 * 0.001)
```

For latency SLOs, replace the numerator with the count of requests above the threshold:

```promql
(
  sum(rate(http_request_duration_seconds_count{job="checkout"}[1h]))
  -
  sum(rate(http_request_duration_seconds_bucket{job="checkout",le="0.3"}[1h]))
)
/
sum(rate(http_request_duration_seconds_count{job="checkout"}[1h]))
```

### Missing data is not success

A ratio query returns no data when the service stops exporting metrics. An alert that only checks `error_ratio > threshold` will be silent during a total outage. Add:

```promql
# Absence of any successful request for 5 minutes is a page.
sum(rate(http_requests_total{job="checkout",status!~"5.."}[5m])) == 0
```

and a target-health alert:

```promql
up{job="checkout"} == 0
```

Treat "no data" explicitly in dashboards and alert rules. Grafana's "No data" state should be visually distinct from "0".

### Budget policy

An error budget without a policy is a number. Write the policy before the incident:

- **Budget > 50% remaining:** normal release cadence; reliability work is planned, not urgent.
- **Budget 20–50%:** review the top error sources; add a canary or rollout gate; no risky migrations.
- **Budget < 20%:** freeze non-emergency changes to the affected journey; reliability fixes take priority; daily review.
- **Budget exhausted:** only emergency fixes and reliability work; the team and its stakeholders agree on the exit criteria (for example, 7 consecutive days under 0.05% error ratio).
- **Exception process:** who can override, for how long, and what evidence they must record.

The policy must have an executive sponsor. Otherwise the first deadline will override it, and the SLO becomes theater.

---

## 5. Capacity planning

### The demand model

Capacity planning is four questions:

1. **What is the demand unit?** requests/sec, concurrent users, jobs/hour, GB/day, GPU-hours.
2. **How does demand grow?** linear, exponential, step function (launch), seasonal (daily/weekly), or spiky (marketing).
3. **How much does one unit cost in resources?** CPU-seconds, memory, IOPS, bandwidth, connections.
4. **What headroom is required?** for growth, failure, maintenance, and queueing.

```text
demand (peak) ──► per-unit resource ──► total resource ──► headroom ──► instances
      │                  │                    │                │
      │                  │                    │                └── N+1, maintenance, growth
      │                  │                    └── CPU, RAM, disk, network, connections
      │                  └── measured, not guessed
      └── peak, not average
```

### Little's law

```text
L = λ × W

L = average number of items in the system (concurrency)
λ = arrival rate (requests per second)
W = average time in the system (seconds)
```

Example: 200 req/s and 250 ms average latency means `L = 200 × 0.25 = 50` concurrent requests. If each request holds a database connection for the full 250 ms, you need at least 50 connections, plus headroom. If you have a pool of 20, requests queue before they even reach the database, and latency rises even though the database is idle.

Little's law also works in reverse: if you cap concurrency at 50 and arrivals are 200/s, the average time in the system cannot be below 250 ms. Adding CPU will not fix a concurrency limit that is already saturated.

### CPU demand

```text
CPU cores required = arrival_rate × CPU_seconds_per_request / target_utilization
```

Example: 1,200 req/s at 8 ms CPU per request = 9.6 cores of demand. At a 60% target utilization:

```text
9.6 / 0.60 = 16 cores
```

Then add failure headroom: if one of four nodes can fail, you need the remaining three to carry the load, so provision `16 × 4/3 ≈ 21.3 → 22 cores`. This is N+1, not "20% extra."

### Queueing: why 70% utilization can be slow

For a simple single-queue system, wait time grows non-linearly with utilization:

```text
W = S / (1 - ρ)

W = average time in system
S = average service time
ρ = utilization (0 to 1)
```

With `S = 100 ms`:

| Utilization ρ | Average time W | Wait beyond service |
|---:|---:|---:|
| 50% | 200 ms | 100 ms |
| 70% | 333 ms | 233 ms |
| 80% | 500 ms | 400 ms |
| 90% | 1,000 ms | 900 ms |
| 95% | 2,000 ms | 1,900 ms |
| 99% | 10,000 ms | 9,900 ms |

Real systems are not single-server M/M/1 queues, but the shape is universal: **near saturation, small load increases produce large latency increases**. This is why a CPU dashboard at 75% can look "fine" while p99 latency is failing. Plan to run at a utilization that leaves room for bursts, not at the cliff edge.

The formula also explains why autoscaling on CPU is reactive: by the time CPU is at 90%, the queue has already formed. Scale on a leading signal (request rate, queue depth, concurrency) or keep enough headroom.

### Worked capacity plan

A checkout service currently peaks at 1,200 req/s. Marketing forecasts 30% growth next quarter and a launch that adds a step of 20% on top. Each request uses 8 ms CPU and 40 MiB of memory; a pod has 2 vCPU and 2 GiB, and you target 60% CPU utilization. You need N+1 across 3 availability zones and 30% spare capacity for rolling deploys.

1. Forecast peak: `1,200 × 1.30 × 1.20 = 1,872 req/s`.
2. CPU demand: `1,872 × 0.008 = 14.98 cores`.
3. At 60% target: `14.98 / 0.60 = 24.97 → 25 cores`.
4. Pods by CPU: `ceil(25 / 2) = 13`.
5. Memory: `1,872 × 40 MiB = 74,880 MiB = 73.1 GiB`; at 2 GiB per pod, `ceil(73.1 / 2) = 37` pods. Memory is the binding constraint.
6. N+1 across 3 AZs: if one AZ fails, the remaining two-thirds of capacity must carry the full load. With 37 pods, you need `ceil(37 × 3/2) = 56` pods so that losing a third still leaves 37. Spread 56 as 19/19/18; losing a 19-pod AZ leaves 37.
7. Add 30% rolling-deploy spare: `ceil(56 × 1.30) = 73` pods.
8. Sanity check the request rate per pod: `1,872 / 73 = 25.6 req/s` per pod, well below the measured safe rate — but memory is now 146 GiB allocated for 73.1 GiB used. That is the cost of the failure and deploy assumptions. Revisit whether full N+1 across AZs is required for this service, or whether a degraded mode is acceptable.

The arithmetic is not the hard part. The hard part is writing down the assumptions and noticing when one resource (memory) dominates the others.

### Headroom categories

| Headroom | Purpose | Typical |
|---|---|---|
| **Growth** | demand increase before the next provisioning cycle | 20–30% |
| **Failure** | N+1 node/AZ, or degraded mode | 1/N of capacity |
| **Maintenance** | rolling restarts, upgrades, drains | 10–30% |
| **Burst** | queueing and traffic spikes | 20–50% |
| **Reserve** | cloud quota/instance availability | provider-specific |

Do not add all percentages blindly; model the failure scenario. "30% extra" and "survive one AZ loss" are different requirements and have different costs.

### Kubernetes capacity details

- **Requests** are the scheduler's promise. A pod with no CPU request is scheduled on a node that may already be full, then throttled.
- **Limits** cap usage. CPU limits cause throttling; memory limits cause OOM kills. A memory limit below the application's peak is a crash, not a safety net.
- **Allocatable** is less than node capacity after kubelet/system reservation. Check `kubectl describe node`.
- **Fragmentation:** 10 nodes with 1 free CPU each cannot schedule a 4-CPU pod. Bin-packing matters.
- **Cluster autoscaler** adds nodes, but not instantly. A traffic spike can hit a pending pod for minutes.
- **PodDisruptionBudgets** protect availability during drains, but they can also block maintenance forever if misconfigured.
- **Vertical vs horizontal:** more replicas help stateless throughput; a single-threaded, memory-heavy service may need a bigger pod.

### Forecasting discipline

1. Plot demand at the same granularity you provision (daily peak, not monthly average).
2. Separate trend, seasonality, and one-off events.
3. Fit a simple model; do not use a complex one you cannot explain.
4. Show a range, not a single line. "1,872 req/s, with a plausible range of 1,600–2,200" is more honest than a false-precision number.
5. Set a review trigger: "re-forecast when actual peak exceeds forecast by 15% for 3 days."
6. Track forecast error. A model that is always 40% low is worse than a rule of thumb with a known bias.

---

## 6. Incident response

### Severity

| Severity | Definition | Examples | Response |
|---|---|---|---|
| **SEV1** | Broad user impact, data loss, or active security incident | checkout down for all users; customer data exposed | Page immediately; IC; status page; exec notification |
| **SEV2** | Major degradation or significant subset affected | 20% error rate; one region down; latency 10× | Page; IC; status page if external |
| **SEV3** | Minor degradation or workaround exists | one route slow; a single tenant affected | Ticket/chat; fix in business hours |
| **SEV4** | No current user impact | a redundant component down; disk trending full | Backlog; no page |

Severity is about **impact**, not about how interesting the bug is. A silent data corruption with no visible errors can be SEV1. A dramatic-looking pod crash loop with automatic failover and no user impact is SEV4.

### Roles

| Role | Responsibility | Does not do |
|---|---|---|
| **Incident Commander (IC)** | Owns the incident; decides priorities; assigns roles; declares severity changes; approves risky mitigations | Debug the system personally |
| **Operations Lead** | Runs the technical investigation and mitigation | Communicate externally |
| **Communications Lead** | Status page, stakeholder updates, customer messaging | Change production |
| **Scribe** | Timeline, decisions, evidence links, action items | Investigate |
| **Subject Matter Experts** | Deep debugging in their area | Bypass the IC |

Small teams can combine roles, but someone must be explicitly the IC and someone must communicate. The failure mode is three engineers debugging in silence while nobody tells support what to say.

### Lifecycle

```text
detect ──► triage ──► mitigate ──► resolve ──► learn
  │          │           │            │           │
  │          │           │            │           └── postmortem, actions, verification
  │          │           │            └── confirm steady state, close comms, preserve evidence
  │          │           └── stop the bleeding: rollback, drain, scale, failover, feature flag
  │          └── severity, roles, scope, first customer impact estimate
  └── alert, customer report, synthetic check, or a human noticing
```

**Mitigate before you diagnose.** If a deploy is the suspected cause and rollback is cheap, roll back first and investigate the artifact afterward. If you do not know the cause, stabilize with the least risky lever: shift traffic, disable a feature, scale out, or fail over. Do not "try a restart" on a stateful system without understanding what state you might lose.

### The first 10 minutes

1. **Acknowledge** the page and declare the incident in the incident channel.
2. **Assign** IC and comms; if you are alone, say so and ask for help.
3. **State** the observed symptom, start time, scope, and current severity.
4. **Check** recent changes: deploys, config, feature flags, infrastructure, certificates, DNS, dependencies.
5. **Check** the SLO dashboard and the top error/latency route.
6. **Choose** a mitigation hypothesis and a reversible action; write it down.
7. **Communicate** the first update within 15 minutes for SEV1, 30 for SEV2, even if the update is "still investigating, no cause yet."
8. **Preserve** evidence before cleanup: logs, metrics snapshots, affected request IDs, pod descriptions, `kubectl get -o yaml`, audit logs.

### Communication

```text
[SEV2] checkout-api elevated errors
Status: investigating
Impact: ~25% of checkout requests failing since 14:05 UTC; browsing unaffected
Current action: rolled back deploy 2026.10.27-3; monitoring error rate
Next update: 14:45 UTC
Incident commander: @name
```

Rules:

- State facts and time stamps; label hypotheses as hypotheses.
- Give a next-update time and keep it, even if nothing changed.
- Do not speculate about cause or blame in customer-facing communication.
- Do not promise an ETA you cannot defend.
- Keep one authoritative channel; link the dashboard and runbook.
- For security incidents, do not put sensitive details in broad channels; follow the security escalation path.

### Handoff

A handoff is a structured transfer, not "you take over":

```text
Incident: checkout-api SEV2, started 14:05 UTC
Current state: error rate 25% -> 4% after rollback; latency normal
What we know: errors began 3 min after deploy; rollback at 14:22
What we tried: rollback (effective), scaled to 20 pods (no change)
What is next: verify canary, inspect the deploy diff, decide on re-deploy
Open questions: did the DB migration run before rollback?
Evidence: dashboard link, deploy ID, affected request sample
```

### Evidence preservation

Before you delete a pod, rotate logs, or revert a config:

```bash
kubectl get pod -n checkout -o yaml > incident-$(date +%s)-pod.yaml
kubectl logs -n checkout deploy/checkout --since=2h > incident-checkout.log
kubectl get events -n checkout --sort-by=.lastTimestamp > incident-events.txt
```

Capture metrics with a snapshot or export the query results; a live dashboard changes as the window moves. Record the exact query, the time range, and the Prometheus data source. For cloud incidents, note the account, region, resource IDs, and request IDs. For security incidents, preserve audit logs and do not tip off the actor.

### Incident timeline

The scribe records, in UTC:

```text
14:05  Alert: checkout error ratio > 5%
14:06  @oncall acknowledged; declared SEV2
14:08  Error ratio 25%; all routes affected; deploy 2026.10.27-3 at 14:02
14:10  Rollback started
14:14  Rollback complete; error ratio 4% and falling
14:20  Root-cause investigation begins; DB migration check
14:35  Error ratio 0.3%; steady state; downgraded to SEV3
15:10  Incident resolved; postmortem owner assigned
```

The timeline is a factual record, not a narrative. It is the input to the postmortem and the source of truth when memories disagree.

### Postmortem

A blameless postmortem assumes people acted reasonably given the information and incentives they had. It asks how the system made the error easy, hard to detect, and hard to mitigate.

```markdown
# Postmortem — checkout-api SEV2, 2026-10-27

## Summary
A deploy introduced a database query that scanned an unindexed table. Checkout
error rate reached 25% for 17 minutes. Rollback restored service.

## Impact
- 17 minutes of elevated errors (14:05–14:22 UTC)
- ~38,000 failed checkout attempts
- 0.09% of the 28-day error budget consumed (of 0.1% total)

## Timeline
(UTC, from the incident channel)

## What went well
- Alert fired in 90 seconds.
- Rollback was one command and completed in 4 minutes.
- Comms update went out at 14:12, within the 15-minute target.

## What went poorly
- The migration ran before the canary check; rollback did not revert it.
- The dashboard had no query-plan or DB latency panel.

## Contributing factors
- The new query passed tests with a 1,000-row fixture; production had 40M rows.
- The canary analyzed only HTTP status, not database latency.
- The migration process has no "expand/contract" rule for indexes.

## Action items
| Action | Owner | Due | Priority | Verification |
|---|---|---|---|---|
| Add index; test with production-scale fixture | @db | 2026-11-03 | P1 | Query plan shows index scan |
| Canary gate on p95 DB latency | @platform | 2026-11-10 | P1 | Canary blocks on threshold |
| Document expand/contract migration rule | @eng | 2026-11-17 | P2 | Runbook reviewed |

## Lessons
Systems, not people: the fixture size and the canary signal made the failure invisible.
```

Action-item rules:

- Every action has an owner, a due date, a priority, and a **verification** method.
- Prefer systemic fixes (gates, defaults, automation) over "be more careful" or "add a checklist."
- Limit the list to what the team can actually finish; 20 action items is a wish list.
- Review open action items weekly; an incident is not closed when the service recovers, but when the prevention is verified.
- A repeat incident with the same contributing factor is a process failure, not bad luck.

### Postmortem anti-patterns

| Anti-pattern | Why it fails | Replace with |
|---|---|---|
| "Human error" as root cause | Stops analysis at the person | Ask what made the action easy, likely, or undetected |
| Single root cause | Complex systems have contributing factors | List factors and their relative contribution |
| No timeline | Memory is unreliable | Scribe during the incident |
| Action items with no owner/date | Nothing happens | Owner, date, verification |
| Blame or punishment | People hide information | Blameless framing, protected reporting |
| "Add more monitoring" | Not specific or testable | Name the signal, threshold, and runbook |
| 100% utilization as a goal | Queueing makes latency explode | Target headroom and load test |

---

## 7. Putting it together: the reliability review

Run a monthly review with four artifacts:

1. **SLO report:** each SLI, target, actual, budget remaining, and trend.
2. **Incident summary:** count by severity, time to detect, time to mitigate, repeat causes.
3. **Capacity forecast:** peak vs forecast, headroom by resource, upcoming launches.
4. **Action-item burn-down:** open items, overdue items, verification evidence.

Questions the review must answer:

- Which user journeys are at risk of missing their SLO?
- Where did we spend the error budget, and was it worth it?
- Which alerts were noisy, missed, or unactioned?
- What will break first as demand grows?
- What did we learn that changed a default, a gate, or a runbook?

If the review does not change a decision, shorten it or stop holding it.

---

## 8. Failure scenarios

### A — The SLO is green but users are complaining

**Symptoms:** 99.98% availability, but support tickets about slow checkout.

**Evidence to collect:** the SLI definition, the latency threshold, the route coverage, synthetic vs real traffic, client-side timings, and the percentage of requests that never reach the server.

**Likely causes:** the SLI measures only HTTP 200s on one endpoint; slow requests still return 200; the CDN or load balancer absorbs failures; mobile clients time out before the server records a request; the denominator excludes retried requests.

**Fix:** redefine the SLI around the user journey, include client-observed latency, and add a synthetic journey that follows the real path.

### B — The burn-rate alert never fired during an outage

**Symptoms:** 100% errors for 10 minutes; no page.

**Evidence:** alert rule state, `for` duration, evaluation interval, query result at the time, whether the short and long windows both breached, whether the metric disappeared (no data), and Alertmanager delivery logs.

**Likely causes:** the rule uses a ratio that returns no data when there are zero successes; the `for` is longer than the outage; only one window is required and it is the long one; the alert is inhibited by a broader alert; the receiver is misrouted.

**Fix:** add an absence alert, shorten `for` for fast burn, test with a deliberate failure, and verify the full path to the receiver.

### C — The error budget is exhausted in the first week

**Symptoms:** 99.9% SLO, 5% error ratio for two days after a deploy.

**Evidence:** deploy timeline, error ratio by route/version, retry rate, dependency errors, and whether the denominator changed.

**Likely causes:** a bad canary that was promoted; retries amplifying load; a dependency outage; a new endpoint with a high error rate included in the denominator; a migration that doubled traffic.

**Fix:** freeze releases under the budget policy, roll back or disable the feature, fix the top error source, and re-baseline only after the window recovers. Do not reset the SLO to hide the spend.

### D — Capacity says 70% but latency is failing

**Symptoms:** CPU at 70%, p99 latency 4× the SLO.

**Evidence:** per-instance CPU, throttling, run queue, memory pressure, GC pauses, disk IOPS/latency, connection pool saturation, queue depth, and load-balancer distribution.

**Likely causes:** queueing near a bottleneck that is not CPU; CPU limits causing throttling; a hot shard; a connection pool exhaustion; garbage collection; noisy neighbor; a single instance receiving most traffic.

**Fix:** find the saturated resource with USE, not the average CPU. Add headroom or fix the bottleneck; scaling the wrong resource adds cost without improving latency.

### E — The incident has no commander

**Symptoms:** three engineers changing different things; support improvising messages; no timeline; rollback conflicts.

**Evidence:** incident channel, change log, deploy history, and the sequence of mitigations.

**Likely causes:** no on-call rotation, no severity declaration, fear of "taking over," or a culture that treats coordination as overhead.

**Fix:** declare the role explicitly, even in a small team. The IC can also be the ops lead, but must announce it. Write the first update and assign a scribe. Coordination is not a meeting; it is a control loop.

### F — The postmortem action items never ship

**Symptoms:** the same incident recurs; the postmortem document is thorough; the action list is stale.

**Evidence:** action owners, due dates, ticket status, and whether the fix was verified.

**Likely causes:** too many actions, no prioritization, no management support, or actions that are too vague to start.

**Fix:** cap the list at 3–5 actions, assign P1s to the next sprint, and review them in the reliability review. Verify the fix with a test that would have caught the original failure.

### G — Monitoring is down during the outage

**Symptoms:** the service is down; the monitoring dashboard is also down; nobody is paged.

**Evidence:** Prometheus target health, alert delivery path, DNS, network path, and whether monitoring shares the same cluster/region/account.

**Likely causes:** monitoring runs in the same failure domain; the alertmanager receiver depends on the same network; the absence alert is missing; the on-call phone number is stale.

**Fix:** use an external dead-man's-switch (a heartbeat to an outside service), monitor the monitoring, and test the path quarterly. A monitoring system that fails silently is worse than no monitoring because it creates false confidence.

### H — The SLO target is impossible

**Symptoms:** the team misses 99.99% every month; the dependency's own SLA is 99.9%; the cost of the last 0.09% is disproportionate.

**Evidence:** dependency SLAs, architecture single points of failure, incident history, and the cost of redundancy.

**Likely causes:** the target was copied from a template; the user journey spans multiple dependencies; the business does not actually need that level; the SLA and SLO are confused.

**Fix:** set the SLO from user need and architecture, not aspiration. A 99.9% SLO with a clear budget and a plan is better than a 99.99% target everyone ignores. If the business needs 99.99%, invest in the architecture and measure the dependencies.

### I — The error budget policy is ignored under deadline pressure

**Symptoms:** budget exhausted; releases continue; the SLO is quietly redefined.

**Evidence:** release log vs budget timeline, exception approvals, and stakeholder messages.

**Likely causes:** no executive sponsor, no pre-agreed consequence, or a policy that blocks all work instead of prioritizing reliability fixes.

**Fix:** get the policy signed before the incident, define the exception process, and make the trade-off visible in business terms ("each additional 9 costs X and delays the launch by Y"). A policy that cannot survive a deadline is not a policy.

### J — The forecast is linear but growth is a step

**Symptoms:** capacity plan says three months of headroom; a launch doubles traffic in a day; the autoscaler cannot keep up.

**Evidence:** launch calendar, daily peak series, new-tenant onboarding, and the difference between organic and step growth.

**Likely causes:** the model used monthly averages; launches were not in the forecast; the autoscaler has a slow node-provisioning path; quotas cap the scale-out.

**Fix:** maintain a launch calendar, model step changes explicitly, pre-provision for known events, and load-test the autoscaling path including node provisioning.

---

## 9. Command field manual

### `hey` — HTTP load and latency distribution

```bash
hey -z 60s -c 50 -q 20 -m GET https://api.example.com/health
```

| Flag | Meaning | Why it matters |
|---|---|---|
| `-z 60s` | Run for 60 seconds | Time-based runs are easier to compare than a fixed count |
| `-c 50` | 50 concurrent workers | Concurrency, not requests/sec; Little's law connects them |
| `-q 20` | 20 requests/sec per worker | Caps the rate; omit for closed-loop max throughput |
| `-m GET` | HTTP method | Match the real journey |
| `-o csv` | CSV output | Post-process the distribution instead of reading a summary |

Representative output:

```text
Summary:
  Total:        60.0123 secs
  Slowest:      2.4310 secs
  Fastest:      0.0120 secs
  Average:      0.1830 secs
  Requests/sec: 998.4120

Response time histogram:
  0.012 [1]     |
  0.254 [58231] |■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■
  0.496 [1204]  |■
  ...
  Latency distribution:
    10% in 0.0810 secs
    50% in 0.1520 secs
    90% in 0.3010 secs
    95% in 0.4120 secs
    99% in 1.2040 secs

Status code distribution:
  [200] 59904 responses
  [503] 18 responses
```

Interpretation:

- The average (183 ms) is below the 300 ms threshold, but p99 is 1.2 s. An average-based SLO would call this healthy; a ratio SLO would not.
- 18 × 503 is a 0.03% error rate. At a 99.9% SLO, that is 30% of a 1M-request budget in one minute.
- `Requests/sec` is a result, not a target. With `-c 50` and 183 ms average, Little's law predicts `50 / 0.183 ≈ 273 req/s`; the higher number means the tool is not strictly closed-loop or the latency distribution shifted. Treat load-generator numbers as evidence, not truth.
- Run against a lab endpoint you own. A load test against a third-party service is a denial-of-service attack.

Common mistakes:

- Using `-c` as "requests per second." It is concurrency.
- Testing `/health` and concluding the checkout path is fast.
- Ignoring client-side limits (DNS, TLS handshake, connection pool) that the tool may reuse.
- Running from a laptop over Wi-Fi and blaming the server for the network.

### `wrk` — sustained load with latency percentiles

```bash
wrk -t4 -c100 -d60s --latency -s script.lua https://api.example.com/checkout
```

Representative output:

```text
Running 1m test @ https://api.example.com/checkout
  4 threads and 100 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   210.43ms  180.22ms   2.10s   88.12%
    Req/Sec    118.20     31.40   220.00    72.31%
  Latency Distribution
     50%  150.12ms
     75%  280.44ms
     90%  450.10ms
     99%    1.20s
  28120 requests in 60.02s, 42.10MB read
  Socket errors: connect 0, read 0, write 0, timeout 12
Requests/sec:    468.45
Transfer/sec:    718.20KB
```

Reading it:

- `Stdev` close to `Avg` means a long-tailed distribution; the mean is not representative.
- `Socket errors: timeout 12` matters even when the HTTP status distribution looks clean.
- `--latency` is required for the percentile distribution; without it, you get throughput only.
- `-t4` threads is a client-side setting; four threads may be the bottleneck before the server is.

### `stress-ng` — controlled resource pressure

```bash
stress-ng --cpu 4 --cpu-load 80 --vm 2 --vm-bytes 1G --timeout 60s --metrics-brief
```

Representative output:

```text
stress-ng: info:  [12345] setting to a 60 second run per stressor
stress-ng: info:  [12345] dispatching hogs: 4 cpu, 2 vm
stress-ng: info:  [12345] successful run completed in 60.02s
stress-ng: info:  [12345] cpu: 4.00 4.00 4.00 4.00 ...
```

Use it to:

- verify that CPU alerts and dashboards respond;
- test throttling by running inside a CPU-limited container;
- test OOM behavior with a memory limit;
- observe how latency changes as utilization rises.

Safety: run only in a disposable VM/container, bound the time, and never on a shared production host. `--vm-bytes` can trigger the OOM killer; that is the point in a lab and an outage in production.

### `kubectl top` and node allocatable

```bash
kubectl top pods -n checkout --containers
kubectl top nodes
kubectl describe node ip-10-0-1-25 | sed -n '/Allocated resources/,/Events/p'
```

Representative output:

```text
NAME                        CPU(cores)   MEMORY(bytes)
checkout-7d9f8c6b5-2xk4l    120m         340Mi
checkout-7d9f8c6b5-9q2mv    98m          310Mi

NAME           CPU(cores)   CPU%   MEMORY(bytes)   MEMORY%
ip-10-0-1-25   1450m        72%    6.2Gi           78%

Allocated resources:
  Resource           Requests      Limits
  cpu                3500m (87%)   6000m (150%)
  memory             12Gi (75%)    16Gi (100%)
```

Interpretation:

- `kubectl top` is recent usage, not a request or a limit. A pod at 98m may still have a 500m request.
- `Allocated requests` at 87% means the scheduler is near full even if actual usage is 72%. New pods may stay Pending.
- Limits above 100% are allowed (overcommit); they are a risk, not a scheduling guarantee.
- Metrics come from metrics-server; if it is down, `kubectl top` fails while the pods are fine.

### Prometheus API — query the exact SLO

```bash
curl -sG http://localhost:9090/api/v1/query \
  --data-urlencode 'query=sum(rate(http_requests_total{job="checkout",status=~"5.."}[5m])) / sum(rate(http_requests_total{job="checkout"}[5m]))' \
  | jq -r '.data.result[0].value[1]'
```

Representative output:

```text
0.0234
```

Use the API when you need evidence at a specific time or want to script a report. Always record the query string, the evaluation time, and the data source; a number without its query is not evidence.

For a range:

```bash
curl -sG http://localhost:9090/api/v1/query_range \
  --data-urlencode 'query=sum(rate(http_requests_total{job="checkout",status=~"5.."}[5m])) / sum(rate(http_requests_total{job="checkout"}[5m]))' \
  --data-urlencode "start=$(date -u -v-1H +%s)" \
  --data-urlencode "end=$(date -u +%s)" \
  --data-urlencode 'step=60' \
  | jq -r '.data.result[0].values[-5:]'
```

On Linux, replace `date -u -v-1H +%s` with `date -u -d '1 hour ago' +%s`.

### `promtool` — test the SLO rules before the incident

```bash
promtool check rules rules/slo.yml
promtool test rules tests/slo_test.yml
```

Representative output:

```text
Checking rules/slo.yml
  SUCCESS: 4 rules found

Unit Testing: tests/slo_test.yml
  SUCCESS
```

A rule test feeds synthetic series and asserts the alert fires (or does not) at specific times. It is the only way to know that the "no data" case and the multiwindow condition behave as intended. If you cannot test an alert, you cannot trust it.

### Capacity evidence from the node

```bash
nproc
free -h
df -h /
vmstat 1 5
iostat -xz 1 3
ss -s
```

Representative `vmstat`:

```text
procs -----------memory---------- ---swap-- -----io---- -system-- ------cpu-----
 r  b   swpd   free   buff  cache   si   so    bi    bo   in   cs us sy id wa st
 2  0      0 842312  32104 4120032    0    0    12    84  920 1420 18  4 77  1  0
 1  0      0 840120  32104 4121200    0    0     0    96  880 1380 16  3 80  1  0
```

Read `r` (run queue), `wa` (IO wait), `si/so` (swap), and `st` (steal) before blaming CPU. A run queue above the number of CPUs means requests are waiting for CPU; high `wa` means the disk is the bottleneck; `st` means the hypervisor is taking CPU away.

---

## 10. Design and review checklist

### SLO

- [ ] The SLI is a ratio of good events to valid events, with a stated denominator.
- [ ] The threshold and window match the user journey.
- [ ] The target is stricter than the SLA and achievable with the current architecture.
- [ ] The error budget is expressed in both percentage and user-visible time/events.
- [ ] A budget policy exists with a freeze threshold and an exception process.
- [ ] Fast and slow burn alerts use two windows and are tested.
- [ ] Absence of data and total outage are covered by separate alerts.
- [ ] The dashboard shows the SLI, budget remaining, burn rate, and top contributors.

### Capacity

- [ ] Demand is modeled at peak, not average, with seasonality and launches.
- [ ] Per-unit resource use is measured, not guessed.
- [ ] The binding resource is identified (CPU, memory, IOPS, connections, bandwidth).
- [ ] Headroom covers growth, failure, maintenance, and burst separately.
- [ ] Queueing behavior is understood near the target utilization.
- [ ] The failure scenario (N+1, AZ loss, dependency loss) is explicitly modeled.
- [ ] Autoscaling has been tested, including node provisioning time and quotas.
- [ ] Forecast error is tracked and triggers a re-forecast.

### Incident response

- [ ] Severity definitions are agreed and visible.
- [ ] An on-call rotation and escalation path exist.
- [ ] IC/comms/scribe roles are assigned at declaration.
- [ ] A communication template and cadence exist.
- [ ] Evidence preservation commands are in the runbook.
- [ ] Rollback and feature-flag mitigations are tested.
- [ ] Every SEV1/SEV2 gets a blameless postmortem within 5 business days.
- [ ] Action items have owners, dates, and verification.
- [ ] Monitoring has an external dead-man's switch.

---

## 11. Checkpoint

Answer without notes, then check against the sections above.

1. Define SLI, SLO, SLA, and error budget in one sentence each.
2. Why should an availability SLI be a ratio rather than a count of errors?
3. What are three valid choices for the denominator, and what does each change about the SLO's meaning?
4. A 99.95% SLO over 30 days allows how much downtime? Over 28 days?
5. If a service has a 99.9% SLO and observes a 2% error rate, what is the burn rate and the time to exhaust a 30-day budget?
6. Why does a multiwindow burn-rate alert require both a long and a short window?
7. What does Little's law tell you about a service with 300 req/s and 400 ms latency?
8. Why does latency rise sharply at 90% utilization even if service time is constant?
9. Name the four headroom categories and give a failure scenario for each.
10. Why is a pod without a CPU request dangerous on a shared node?
11. What are the first five actions in the first 10 minutes of a SEV1?
12. What is the IC responsible for, and what should the IC not do?
13. Why is "human error" not a root cause?
14. What makes a postmortem action item actionable?
15. How would you detect that your monitoring is down during the outage it should be alerting on?
16. Your SLO is green but users report slow checkout. List five things you would check.
17. Your capacity model says 70% CPU but p99 latency is 4×. What do you check before adding replicas?
18. Why can a 99.99% SLO be the wrong target even if the business "wants reliability"?
19. How does an error budget policy change release behavior, and what makes it credible?
20. What evidence would you preserve before deleting a failing pod?
