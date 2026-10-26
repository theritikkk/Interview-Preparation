# Phase 8 Labs — Part 2 (L8.3–L8.4)

> **Safety:** L8.3 injects errors only into the local lab service. Do not run the load generator against a shared or third-party service. L8.4 modifies a **disposable** Linux VM and an optional kind cluster; snapshot first, keep a console open, and do not run the hardening steps against your laptop or a production host.
>
> **Environment:** L8.3 uses the L8.2 Docker Compose stack. L8.4 uses a disposable Ubuntu 22.04/24.04 VM; macOS alone is not sufficient for the Linux audit, and a kind cluster is optional for the Kubernetes section.

---

# Lab L8.3 — SLO, Error Budget, and an Incident You Can Run

## Objective

Define an SLO, implement a burn-rate alert, exhaust the budget with a controlled failure, and run the incident as a team exercise. By the end you can:

1. Write an SLI/SLO spec with an explicit denominator, window, and budget policy.
2. Convert an error ratio into a burn rate and a time-to-exhaustion.
3. Implement a two-window burn alert and test it with `promtool`.
4. Run an incident with an IC, ops lead, comms lead, scribe, and a timeline.
5. Write a blameless postmortem with verified action items.
6. Do a capacity estimate with Little's law and queueing headroom.

The lab uses a **1-hour SLO window** instead of 28 days so you can observe the budget in minutes. The arithmetic is the same; the compression is the point. Say clearly in your write-up that the window is a lab proxy, not a production SLO.

## Prerequisites

- L8.1 and L8.2 complete; the stack starts cleanly.
- Docker Compose v2, `curl`, `jq`, `python3`, `promtool` (or the Prometheus container).
- A notebook or the write-up template below.
- 90–120 minutes, including the drill and postmortem.

## Setup

Copy the L8.2 project so the drill does not destroy your working stack:

```bash
cp -r ~/opslab/phase8/l8.2 ~/opslab/phase8/l8.3
cd ~/opslab/phase8/l8.3
mkdir -p slo incidents
```

Expected additions:

```text
l8.3/
├── rules/
│   ├── slo.yml
│   ├── alerts.yml
│   └── slo-lab.yml          # new
├── tests/
│   └── slo_lab_test.yml     # new
├── loadgen.py               # new
├── slo/slo.md               # new
└── incidents/incident-log.md
```

### Lab SLO spec

Create `slo/slo.md`:

```markdown
# Checkout Availability SLO — Lab

## User journey
A user requests checkout and receives a successful response within the
service's normal latency. Latency is covered by a separate SLO; this one is
availability.

## SLI
Good: non-5xx responses for job=app, excluding the /healthz route.
Total: all responses for job=app, excluding the /healthz route.
Ratio: good / total, computed over a rolling window.

## SLO
99.0% over a rolling 1 hour.
Error budget: 1% of the window = 36 seconds of full outage, or 1% of requests.

## Why this window
The lab compresses a 28-day production SLO into 1 hour so the budget can be
observed in a drill. The production SLO would use 28 days and a budget policy
reviewed weekly.

## Budget policy
- 50% remaining: normal releases.
- 20-50% remaining: review top error sources; no risky migrations.
- <20% remaining: freeze non-emergency changes; reliability fixes first.
- 0% remaining: emergency/reliability changes only until the window recovers.

## Alerting
Fast burn: 14.4x burn (14.4% error ratio) over 5m and 1m, for 1m.
Page the on-call. Link the runbook.
Absence: AppTargetDown / AppAbsent.
```

Calculate the budget before you run anything:

| Quantity | Value | How |
|---|---:|---|
| Window | 3,600 s | 1 hour |
| Error budget | 1% | `1 - 0.99` |
| Full-outage equivalent | 36 s | `3,600 × 0.01` |
| Fast-burn threshold | 14.4× | 2% of budget in 1 hour |
| Error ratio for fast burn | 14.4% | `14.4 × 0.01` |

The 14.4× threshold is the standard "2% of the monthly budget in one hour" rule. For a 1-hour window, the interpretation is compressed: a 14.4% error ratio exhausts the hour's budget in `60/14.4 ≈ 4.2 minutes`.

### The load generator

Create `loadgen.py`:

```python
#!/usr/bin/env python3
"""Generate a controlled request mix for the SLO drill.

Example:
  python3 loadgen.py --duration 360 --concurrency 20 --error-rate 0.20
"""
import argparse
import random
import threading
import time
import urllib.error
import urllib.request
from collections import Counter

lock = threading.Lock()
counts = Counter()
stop_at = 0.0


def worker(base, error_rate):
    while time.monotonic() < stop_at:
        path = "/fail" if random.random() < error_rate else "/work"
        try:
            with urllib.request.urlopen(base + path, timeout=5) as response:
                status = response.status
        except urllib.error.HTTPError as exc:
            status = exc.code
        except Exception:
            status = "ERR"
        with lock:
            counts[status] += 1


def main():
    global stop_at
    parser = argparse.ArgumentParser()
    parser.add_argument("--base", default="http://127.0.0.1:8000")
    parser.add_argument("--duration", type=int, default=360)
    parser.add_argument("--concurrency", type=int, default=20)
    parser.add_argument("--error-rate", type=float, default=0.20)
    args = parser.parse_args()

    stop_at = time.monotonic() + args.duration
    threads = [
        threading.Thread(target=worker, args=(args.base, args.error_rate), daemon=True)
        for _ in range(args.concurrency)
    ]
    for thread in threads:
        thread.start()

    start = time.monotonic()
    try:
        while time.monotonic() < stop_at:
            time.sleep(10)
            with lock:
                snapshot = counts.copy()
            total = sum(snapshot.values())
            errors = sum(v for k, v in snapshot.items() if k == 500 or k == "ERR")
            ratio = errors / total if total else 0
            print(
                f"t={time.monotonic() - start:5.0f}s total={total:6d} "
                f"errors={errors:5d} ratio={ratio:.3f} counts={dict(snapshot)}",
                flush=True,
            )
    except KeyboardInterrupt:
        stop_at = time.monotonic()

    with lock:
        snapshot = counts.copy()
    total = sum(snapshot.values())
    errors = sum(v for k, v in snapshot.items() if k == 500 or k == "ERR")
    print(
        f"final total={total} errors={errors} ratio={(errors / total if total else 0):.3f} "
        f"counts={dict(snapshot)}",
        flush=True,
    )


if __name__ == "__main__":
    main()
```

The generator sends 80% `/work` and 20% `/fail` by default. `/fail` returns 500 quickly; `/work` sleeps 20–700 ms. The mix by request count is what the SLO measures; do not assume the latency-weighted mix is the same.

### SLO rules

Create `rules/slo-lab.yml`:

```yaml
groups:
  - name: slo-lab
    interval: 15s
    rules:
      - record: slo:http_errors:ratio5m
        expr: |
          sum by (job, service) (
            rate(http_requests_total{job="app",route!="healthz",status=~"5.."}[5m])
          )
          /
          sum by (job, service) (
            rate(http_requests_total{job="app",route!="healthz"}[5m])
          )

      - record: slo:http_errors:ratio1m
        expr: |
          sum by (job, service) (
            rate(http_requests_total{job="app",route!="healthz",status=~"5.."}[1m])
          )
          /
          sum by (job, service) (
            rate(http_requests_total{job="app",route!="healthz"}[1m])
          )

      - alert: CheckoutErrorBudgetFastBurn
        expr: |
          slo:http_errors:ratio5m > 0.144
          and
          slo:http_errors:ratio1m > 0.144
        for: 1m
        labels:
          severity: critical
          service: checkout
        annotations:
          summary: "checkout is burning its error budget too fast"
          description: "Both the 5m and 1m error ratios are above 14.4% (14.4x burn for a 99% / 1h SLO)."
          runbook_url: "https://example.invalid/runbooks/error-budget"
```

Why 0.144? `1 - 0.99 = 0.01` budget rate; `14.4 × 0.01 = 0.144`. If the SLO were 99.9%, the threshold would be `14.4 × 0.001 = 0.0144` (1.44%). The threshold is a function of the SLO, not a universal 5%.

Add the rule file to the `rule_files` glob (already `rules/*.yml`). Reload and check:

```bash
curl -s -X POST http://127.0.0.1:9090/-/reload
curl -s http://127.0.0.1:9090/api/v1/rules | jq -r '
  .data.groups[] | .name as $g | .rules[] |
  [$g, .name, .health, (.lastError // "")] | @tsv'
```

Expected:

```text
app-alerts  AppTargetDown        ok
...
slo-lab     slo:http_errors:ratio5m  ok
slo-lab     slo:http_errors:ratio1m  ok
slo-lab     CheckoutErrorBudgetFastBurn  ok
```

### Test the burn alert

Create `tests/slo_lab_test.yml`:

```yaml
rule_files:
  - ../rules/slo-lab.yml

evaluation_interval: 1m

tests:
  - interval: 1m
    input_series:
      - series: 'http_requests_total{job="app",service="checkout",route="/work",status="200"}'
        values: '0+60x10'
    alert_rule_test:
      - eval_time: 10m
        alertname: CheckoutErrorBudgetFastBurn
        exp_alerts: []

  - interval: 1m
    input_series:
      - series: 'http_requests_total{job="app",service="checkout",route="/work",status="200"}'
        values: '0+48x10'
      - series: 'http_requests_total{job="app",service="checkout",route="/fail",status="500"}'
        values: '0+12x10'
    alert_rule_test:
      - eval_time: 2m
        alertname: CheckoutErrorBudgetFastBurn
        exp_alerts: []
      - eval_time: 10m
        alertname: CheckoutErrorBudgetFastBurn
        exp_alerts:
          - exp_labels:
              severity: critical
              service: checkout
              job: app
            exp_annotations:
              summary: "checkout is burning its error budget too fast"
              description: "Both the 5m and 1m error ratios are above 14.4% (14.4x burn for a 99% / 1h SLO)."
              runbook_url: "https://example.invalid/runbooks/error-budget"
```

Run:

```bash
promtool test rules tests/slo_lab_test.yml
```

Expected:

```text
Unit Testing: tests/slo_lab_test.yml
  SUCCESS
```

The first case asserts that a healthy 60 req/min service does not page. The second asserts that a 20% error ratio does not page at 2 minutes (not enough history / `for`) but does page at 10 minutes. The test is a specification; if you change the threshold, the test should fail until you update it deliberately.

> If `promtool` complains that the recording rules are not found, check the relative path in `rule_files`. `promtool` resolves it relative to the test file's directory, not your shell's working directory.

## Part A — Baseline the SLO

Generate healthy traffic:

```bash
python3 loadgen.py --duration 180 --concurrency 20 --error-rate 0
```

In another terminal, query:

```promql
slo:http_errors:ratio5m{job="app"}
```

Expected:

```text
slo:http_errors:ratio5m{job="app",service="checkout"} 0
```

```promql
sum(rate(http_requests_total{job="app",route!="healthz"}[5m]))
```

Expected: a positive request rate. If it is zero, the denominator is empty and the ratio is not meaningful. Check the load generator output and the target health.

Record in your log:

- request rate (req/s);
- error ratio (should be ~0);
- budget remaining (100% at baseline);
- p95 latency (for context, not the availability SLO).

### Calculate the budget in request terms

If the load generator sends 10,000 requests in the hour and the SLO is 99%, the budget is 100 bad requests. At 20% errors, every 500 requests consumes the entire budget. This is why a 1-hour window with a high error rate exhausts so quickly.

## Part B — Burn the budget

Start the drill clock. Record the UTC start time.

```bash
date -u +'T0=%Y-%m-%dT%H:%M:%SZ'
python3 loadgen.py --duration 420 --concurrency 20 --error-rate 0.20
```

In a second terminal, watch the alert state:

```bash
while true; do
  date -u +%H:%M:%S
  curl -s http://127.0.0.1:9090/api/v1/rules \
    | jq -r '.data.groups[] | .rules[] | select(.name=="CheckoutErrorBudgetFastBurn") | [.state, .health, .lastEvaluation] | @tsv'
  curl -s http://127.0.0.1:9093/api/v2/alerts \
    | jq -r '.[] | select(.labels.alertname=="CheckoutErrorBudgetFastBurn") | [.status.state, .labels.severity] | @tsv'
  sleep 15
done
```

Expected sequence:

| Time | Observation | What it means |
|---:|---|---|
| T+0 | Error ratio rises toward 20% | The load generator is mixing `/fail` |
| T+1m | `slo:http_errors:ratio5m` ~0.2; burn rate ~20× | Budget is being consumed 20× faster than sustainable |
| T+2m | Alert `pending` | `for: 1m` is being satisfied; both windows breach |
| T+3m | Alert `firing`; budget consumed ~100% | At 20% for 3 minutes, a 1-hour budget is exhausted |
| T+3–7m | Error ratio stays ~20%; budget consumed >100% | The SLO is missed for this window |

Calculate and record:

```text
burn_rate = observed_error_ratio / (1 - SLO)
          = 0.20 / 0.01
          = 20x

time_to_exhaust = window / burn_rate
                = 60 min / 20
                = 3 min

budget_consumed_at_5m = burn_rate × elapsed / window
                      = 20 × 5 / 60
                      = 1.667 = 167%
```

Check the recording rules:

```bash
curl -sG http://127.0.0.1:9090/api/v1/query \
  --data-urlencode 'query=slo:http_errors:ratio5m{job="app"}' | jq -r '.data.result[0].value[1]'
```

Expected:

```text
0.2
```

Check the alert:

```bash
curl -s http://127.0.0.1:9090/api/v1/alerts | jq -r '
  .data.alerts[] | select(.labels.alertname=="CheckoutErrorBudgetFastBurn") |
  [.state, .activeAt, .value] | @tsv'
```

Expected:

```text
firing  2026-10-27T...  2.0e-01
```

The `value` is the left-hand side of the alert expression, which is the 5-minute error ratio, not the burn rate. That is a useful distinction: the alert threshold is expressed in burn-rate terms, but the sample value is the underlying ratio.

### Sanity checks

- Is the error ratio really 20%? Compare `sum(rate(...status=~"5.."))` and the denominator. If the ratio is lower, the load generator may be sending fewer `/fail` requests than intended (the `/work` requests are slower, so they occupy workers longer, reducing the `/fail` share by time). Count-based ratio is what the SLI uses; inspect both.
- Is `route!="healthz"` excluding the health check? Yes. If you remove it, the denominator grows by the health-check rate and the ratio is diluted.
- Is the alert threshold 0.144? Check the rule expression; a 0.05 threshold would fire earlier and is not a 14.4× burn for this SLO.
- Is the alert inhibited by `AppTargetDown`? The app is up and returning 500s, so `up` is 1. If `AppTargetDown` were firing, the critical would inhibit... actually inhibition only suppresses warnings; two criticals for the same job are not inhibited by this rule. Check the Alertmanager UI if the notification is missing.

## Part C — Run the incident

This is a role-play, but the evidence is real. Use four roles; if you are alone, rotate hats and write down which hat you are wearing at each step.

| Role | Assignment |
|---|---|
| Incident Commander (IC) | Declares severity, assigns roles, approves mitigation |
| Operations Lead | Runs the queries, proposes the mitigation, executes it |
| Communications Lead | Writes the status update, keeps the cadence |
| Scribe | Timeline, decisions, evidence links, action items |

### T+0 — Detect

The alert fires. The IC acknowledges and declares **SEV2** (significant degradation, not a total outage). Write the first entry in `incidents/incident-log.md`:

```markdown
## Incident 2026-10-27 — checkout elevated errors

- T0 (UTC):
- Severity: SEV2
- IC:
- Ops:
- Comms:
- Scribe:
- Symptom: checkout error ratio ~20%, budget burning at ~20x
- User impact: ~1 in 5 checkout requests failing
- Recent changes: load generator started; no deploy
```

### T+1 to T+3 — Stabilize

The ops lead collects evidence before changing anything:

```promql
# Error ratio by route
sum by (route) (rate(http_requests_total{job="app",status=~"5.."}[5m]))
/
sum by (route) (rate(http_requests_total{job="app"}[5m]))

# Error ratio by status
sum by (status) (rate(http_requests_total{job="app"}[5m]))

# Target health
up{job="app"}

# Latency, for context
histogram_quantile(0.95, sum by (le) (rate(http_request_duration_seconds_bucket{job="app"}[5m])))

# Budget burn
slo:http_errors:ratio5m{job="app"} / 0.01
```

The likely finding: `/fail` is 100% errors and represents ~20% of requests; `/work` is healthy. The mitigation is to disable the failing path (stop the error-generating load) or route around it. In a real incident, that might be a feature flag, a rollback, or removing a bad dependency from the pool.

Record the decision and the evidence. The IC approves the mitigation. The ops lead executes:

```bash
# Stop the drill's error generator (Ctrl-C in its terminal, or kill the PID).
# In a real incident this would be a rollback/flag change, not "stop the load."
```

Do not claim the incident is resolved until the SLI has recovered for a full evaluation window and the alert has resolved.

### T+3 to T+5 — Communicate

The comms lead posts an update:

```text
[SEV2] checkout elevated errors
Status: mitigating
Impact: ~20% of checkout requests failing since 14:05 UTC; browsing unaffected
Action: disabled the failing dependency path; monitoring error ratio for recovery
Next update: 14:20 UTC
Incident commander: @name
```

The update includes impact, action, and a next-update time. It does not speculate about root cause. It does not promise an ETA.

### T+5 to T+10 — Verify recovery

```bash
python3 loadgen.py --duration 120 --concurrency 10 --error-rate 0
```

Query the error ratio and alert state:

```promql
slo:http_errors:ratio5m{job="app"}
```

Expected: falling toward 0 within 5 minutes (the 5m window still contains the bad period). The alert should resolve after the ratio drops and `for`/evaluation allow.

Check Alertmanager and the webhook:

```bash
curl -s http://127.0.0.1:9093/api/v2/alerts | jq -r '.[] | select(.labels.alertname=="CheckoutErrorBudgetFastBurn") | [.status.state] | @tsv'
docker compose logs --tail=30 webhook
```

Expected: a `resolved` notification with the same fingerprint.

### T+10 — Close and preserve

The IC declares the incident resolved when:

- the SLI is back within SLO for the evaluation window;
- the alert is resolved;
- there is no sign of a second failure;
- the timeline and evidence are captured.

Keep the Prometheus snapshot/query results, the alert payloads, and the load generator output. Do not delete the stack until the postmortem is written.

## Part D — Postmortem

Write `incidents/postmortem.md` using the template from the SLO note. The drill's "root cause" is synthetic, so focus on the **process**:

- How long did detection take? Why?
- Did the alert include enough context to act?
- Did the dashboard answer "are users affected?" without arithmetic?
- Did the comms update go out on time?
- What would have made the mitigation faster?
- Which action would prevent a real repeat?

Example action items:

| Action | Owner | Due | Verification |
|---|---|---|---|
| Add a `route` breakdown to the first-row dashboard | you | +7 days | Panel shows `/fail` vs `/work` |
| Add a runbook link that resolves to a real page | you | +7 days | `curl -I` returns 200 |
| Test the burn alert monthly with a controlled error injection | you | +30 days | Drill log with timestamp |
| Add a minimum-traffic guard to the SLO query | you | +14 days | `promtool` test for zero traffic |

"Be more careful" is not an action. "Add a canary gate on error ratio" is.

## Part E — Capacity sanity check

Use the following scenario. Write the arithmetic, not just the answer.

A service peaks at 200 req/s. Marketing forecasts 25% growth next quarter. Each request uses 8 ms of CPU and 30 MB of memory. A pod has 2 vCPU and 2 GiB, and you target 60% CPU utilization. You need to survive the loss of one of three availability zones and keep 25% spare for rolling deploys.

Answer:

1. Forecast peak: `200 × 1.25 = 250 req/s`.
2. CPU demand: `250 × 0.008 = 2.0 cores`.
3. At 60% target: `2.0 / 0.6 = 3.33 → 4 cores`.
4. Pods by CPU: `ceil(4 / 2) = 2`.
5. Memory: `250 × 30 MB = 7.5 GB`; at 2 GiB per pod, `ceil(7.5 / 2) = 4` pods. Memory binds.
6. Survive one AZ of three: you need the remaining two AZs to carry the full load, so `ceil(4 × 3/2) = 6` pods (2 per AZ).
7. 25% deploy spare: `ceil(6 × 1.25) = 8` pods.
8. Per-pod request rate: `250 / 8 ≈ 31 req/s`; CPU per pod: `31 × 0.008 ≈ 0.25 cores`; memory per pod: `7.5 / 8 ≈ 0.94 GiB`. All fit.

Then answer:

- Which resource is the binding constraint before the failure assumption? Memory.
- Which assumption drives the final count? The AZ-loss requirement, not average utilization.
- What would change if the service could run in a degraded mode at 70% capacity? The N+1 requirement could be relaxed; the cost drops.
- What queueing risk remains at 60% CPU? Bursts and dependencies can still queue; CPU headroom is not a guarantee of latency.

Add a short paragraph to the postmortem: "Capacity is not the same as average utilization. The failure and deploy assumptions determine the count."

## Expected observations

| Observation | Expected | Meaning |
|---|---|---|
| Baseline error ratio | 0 | Healthy service |
| Drill error ratio | ~0.20 | 20% `/fail` mix |
| Burn rate | ~20× | `0.20 / 0.01` |
| Time to exhaust 1h budget | ~3 min | `60 / 20` |
| Budget at 5 min | ~167% | The window is already missed |
| Fast-burn alert | Pending ~2 min, firing ~3 min | Two windows + `for` |
| Webhook | Firing then resolved | End-to-end path |
| Postmortem | Timeline + 3–5 actions | Process, not just a fix |
| Capacity | 8 pods | AZ loss + deploy spare, not just CPU |

## Internal explanation

### Why a 1-hour SLO is a lab proxy

A production availability SLO is usually 28 or 30 days because that is the period over which users, support, and the business experience reliability. A 1-hour window resets too quickly to be a fair contract: a bad 10 minutes can be forgotten by the next hour. The lab compresses time so you can see the budget move. Do not copy the 1-hour window into production without a deliberate reason (for example, a batch job with an hourly SLA).

### Burn rate intuition

The error budget is a rate of allowed failure. If the SLO is 99%, you may fail 1% of requests over the window. A 20% error ratio is 20 times the allowed rate, so the budget would be gone in `window / 20`. The burn rate is dimensionless: it compares the observed badness to the allowed badness.

Two windows solve two problems:

- The long window (5m) proves the burn is sustained and not a single scrape.
- The short window (1m) proves it is still happening and gives fast recovery/resolution.

If you only use the long window, the alert stays firing after the problem stops; if you only use the short window, a brief blip pages. Requiring both is a cheap way to get both properties.

### Why the alert value is the ratio

Prometheus stores the left-hand side of the expression as the alert's value. The expression is a boolean comparison (`ratio > 0.144`), but the sample retains the ratio value for the `and` result. That is why the alert value is `0.2`, not `1`. Use the annotations and the dashboard for the burn rate; do not assume the value is the threshold.

### Incident roles as a control system

The IC is not the best debugger; the IC is the person who keeps the response coherent. The ops lead can go deep because someone else is watching the clock, the comms, and the scope. The scribe is not clerical overhead; memory is unreliable under stress, and the timeline is the evidence base for the postmortem.

### Blamelessness is an evidence rule

Blameless does not mean "no accountability." It means the analysis starts from the system and the information available, not from a person's character. "The deploy had no canary gate" is actionable; "the engineer was careless" is not. The action item changes the system so the next person is less likely to make the same error.

## Troubleshooting task

The drill is over, but the burn alert never fired. The error ratio in the dashboard was ~20% for five minutes.

Collect evidence before changing the rule:

1. `/api/v1/rules` — is `CheckoutErrorBudgetFastBurn` `ok`? What is `lastEvaluation`? Is its state `inactive`, `pending`, or `firing`?
2. Run the expression without the `> 0.144` comparison. What is the actual 5m and 1m ratio?
3. Check the rule file is loaded: `rule_files` glob, reload, and `promtool check rules`.
4. Check the denominator: `route!="healthz"`, `job="app"`, and whether the load generator is hitting the same job.
5. Check `for`: is it longer than the drill?
6. Check Alertmanager: active alerts, silences, inhibition, receiver.
7. Check the time range: a 5m rate after a 5m drill has very few samples at the start.

Write a hypothesis and the evidence that would confirm it. Then read the analysis.

<details>
<summary>After you have written your evidence</summary>

Common causes, in order:

- The rule file was created but Prometheus was not reloaded, or the glob does not match the filename. `/api/v1/rules` will not list it.
- The expression uses `job="checkout"` while the scrape label is `job="app"`. The result is empty; an empty vector never satisfies `> threshold`.
- The denominator includes health checks or a different job, diluting the ratio below 14.4%.
- The `for` is 10m and the drill was 5m; the alert stayed pending.
- The alert is firing in Prometheus but Alertmanager has no `alerting` block or the receiver is misconfigured.
- The alert is silenced or inhibited; check the Alertmanager UI, not just the Prometheus UI.
- The ratio is computed over a window longer than the drill; at the start, the window contains mostly healthy samples and the ratio is lower than the instantaneous 20%.

The fix is specific to the broken link. Do not lower the threshold to make the alert fire; that hides the bug and creates a noisy alert.

</details>

## Verification

- [ ] The SLO spec states the SLI, denominator, window, target, and budget policy.
- [ ] The budget arithmetic is correct: 36 seconds for 99% / 1h.
- [ ] The burn-rate arithmetic is correct: 20% errors = 20× burn; 3 minutes to exhaust.
- [ ] The recording rules are `ok` and return the expected ratio.
- [ ] The fast-burn alert requires both windows and `for: 1m`.
- [ ] `promtool test rules` passes for the healthy and burning cases.
- [ ] The drill produces a firing alert and a webhook notification.
- [ ] The incident has a timeline, roles, a comms update, and a mitigation decision.
- [ ] The alert resolves and a resolved notification arrives.
- [ ] The postmortem has 3–5 actions with owners, dates, and verification.
- [ ] The capacity answer identifies memory and AZ loss as the binding constraints.
- [ ] You can explain why the 1-hour window is a lab proxy.

## Cleanup

```bash
docker compose down -v
```

Keep `slo/slo.md`, `incidents/`, and the test files; they are portfolio artifacts. If you want to rerun the drill, `docker compose up -d` starts a clean stack (the volumes were deleted).

## Write-up template

```markdown
# L8.3 — SLO and Incident Drill

## SLO
- User journey:
- SLI (good/total):
- Window and target:
- Error budget (time and requests):
- Budget policy:

## Burn math
- Observed error ratio:
- Burn rate:
- Time to exhaust:
- Budget consumed at 5 minutes:
- Alert threshold and why:

## Drill timeline (UTC)
| Time | Event | Evidence |
|---|---|---|
| T0 | | |
| T+... | | |

## Roles
- IC:
- Ops:
- Comms:
- Scribe:

## Comms update
(paste the message and the time sent)

## Mitigation
- Hypothesis:
- Action:
- Reversible?:
- Result:

## Postmortem
- What went well:
- What went poorly:
- Contributing factors:
- Action items (owner/date/verification):

## Capacity
- Forecast peak:
- CPU demand and pods:
- Memory demand and pods:
- N+1 / AZ loss:
- Deploy spare:
- Binding constraint:
- Queueing risk:

## What I would change in the production SLO
```

---

# Lab L8.4 — Audit a Host and a Namespace Without Breaking Them

## Objective

Audit a disposable Linux VM and, optionally, a kind cluster for the controls that matter: exposed services, SSH and sudo, file permissions, TLS, secrets, container privilege, and audit logs. Then fix a small number of findings with verification and rollback.

By the end you can:

1. Build an evidence table from commands, not assumptions.
2. Distinguish a listening port from a reachable service.
3. Find SUID binaries, capabilities, world-writable files, and over-broad sudo rules.
4. Inspect a TLS chain, hostname, and expiry; explain what TLS does not authorize.
5. Detect a secret in a Git repository and respond in the right order.
6. Identify privileged/root pods and over-broad RBAC.
7. Make one change at a time, verify it, and keep a rollback path.

## Prerequisites

- A **disposable** Ubuntu 22.04/24.04 VM with a snapshot and console access. Do not use your laptop or a production host.
- `sudo`, `curl`, `jq`, `openssl`, `git`, `python3`.
- `lynis`, `gitleaks`, `trivy`, `auditd` (install below).
- Optional: Docker + kind + kubectl for the Kubernetes section. If kind is unavailable, do the paper alternative.
- 90–120 minutes.

> **Safety:** take a VM snapshot before starting. Keep the provider console open in a browser tab. Do not change `sshd_config` on a host you cannot console into. The lab changes only test users, test files, a test listener, and a test namespace; it does **not** require disabling SSH or the host firewall.

## Setup

On the VM:

```bash
sudo apt-get update
sudo apt-get install -y lynis gitleaks trivy auditd openssl jq curl git python3
```

If a package is unavailable in your release, install the upstream binary into `~/.local/bin` and record the version. Verify:

```bash
lynis show version
gitleaks version
trivy --version
auditctl -v
openssl version
```

Create a working directory and an evidence directory:

```bash
mkdir -p ~/opslab/phase8/l8.4/{evidence,labrepo}
cd ~/opslab/phase8/l8.4
```

Record the baseline:

```bash
date -u | tee evidence/00-baseline.txt
uname -a | tee -a evidence/00-baseline.txt
id | tee -a evidence/00-baseline.txt
sudo ss -tulpn | tee -a evidence/00-baseline.txt
```

## Part A — Host exposure

### Listening services

```bash
sudo ss -tulpn
```

Representative output:

```text
Netid State  Local Address:Port   Peer Address:Port  Process
udp   UNCONN 127.0.0.53:53         0.0.0.0:*          systemd-resolve
tcp   LISTEN 0.0.0.0:22            0.0.0.0:*          sshd
tcp   LISTEN 127.0.0.1:631         0.0.0.0:*          cupsd
```

Questions:

- Which listeners are on `0.0.0.0` or `[::]`? Those are reachable on every interface, subject to the firewall.
- Which are on `127.0.0.1`? Those are local-only and usually the right default for admin/debug services.
- Which process owns each port? A listener with no process name means you did not use `sudo` or the process is in another namespace.
- Is the port reachable from outside the VM? A listener is not proof; the cloud security group or host firewall may block it. Test from another host, not from the VM itself.

Write down the exposure table:

| Port | Bind | Process | Needed? | Reachable from outside? | Action |
|---|---|---|---|---|---|
| 22/tcp | 0.0.0.0 | sshd | yes | yes | key-only, allowlist |
| 631/tcp | 127.0.0.1 | cupsd | no | no | disable if unused |

### Firewall

```bash
sudo nft list ruleset | tee evidence/10-nft.txt
sudo ufw status verbose | tee evidence/10-ufw.txt
```

If both are empty, the host relies on the cloud security group. That is not automatically wrong, but it means the host has no local defense in depth. Do not flush a firewall to test reachability; use a second host and a specific port.

### SSH configuration

```bash
sudo sshd -T | grep -E '^(permitrootlogin|passwordauthentication|kbdinteractiveauthentication|pubkeyauthentication|allowusers|maxauthtries|x11forwarding|permittunnel)'
```

Representative output:

```text
permitrootlogin without-password
passwordauthentication yes
kbdinteractiveauthentication yes
pubkeyauthentication yes
maxauthtries 6
x11forwarding yes
permittunnel no
```

Findings:

- `passwordauthentication yes` on an internet-facing host is a brute-force target. Prefer keys and a bastion/session manager.
- `permitrootlogin without-password` still allows root key login; `no` is stricter.
- `x11forwarding yes` is unnecessary on a server.
- `maxauthtries 6` is higher than needed; 3 is common.

**Do not apply an SSH change in this lab unless you have console access and a second session open.** Instead, write the change plan and validate a copy:

```bash
sudo cp /etc/ssh/sshd_config /tmp/sshd_config.lab
# Edit /tmp/sshd_config.lab, not the live file, for the exercise.
sudo sshd -t -f /tmp/sshd_config.lab
```

A config that passes `sshd -t` is syntactically valid; it is not proof that you can still log in. That requires a live test with a rollback path.

### Sudo

```bash
sudo -l
sudo -l -U "$USER"
```

Create a deliberately over-broad test rule to see what a finding looks like:

```bash
sudo useradd -m -s /bin/bash labuser
echo 'labuser ALL=(root) NOPASSWD: /usr/bin/find' | sudo tee /etc/sudoers.d/labuser
sudo chmod 440 /etc/sudoers.d/labuser
sudo visudo -cf /etc/sudoers.d/labuser
sudo -l -U labuser
```

Representative output:

```text
User labuser may run the following commands on this host:
    (root) NOPASSWD: /usr/bin/find
```

`find` can execute commands (`-exec /bin/sh \;`) and write files as root. A sudo rule that allows it is effectively root, even though it looks narrow. Do not exploit it; record the finding and remove the rule after the exercise.

**Fix:** remove the test rule and user at the end of the lab:

```bash
sudo rm /etc/sudoers.d/labuser
sudo userdel -r labuser
```

If you want a safe positive example, create a rule for a specific read-only command and a specific unit, then verify with `sudo -l -U`. The point is scope, not `NOPASSWD` alone.

### SUID, capabilities, and writable files

```bash
sudo find / -xdev -type f -perm -4000 -printf '%m %u %g %p\n' 2>/dev/null | tee evidence/20-suid.txt
sudo getcap -r / 2>/dev/null | tee evidence/21-caps.txt
sudo find /etc /var/spool/cron -type f -perm /o+w 2>/dev/null | tee evidence/22-world-writable.txt
```

Representative SUID output:

```text
4755 root root /usr/bin/sudo
4755 root root /usr/bin/passwd
4755 root root /usr/bin/chsh
```

Most SUID binaries are expected. The finding is an unexpected one, especially in `/tmp`, `/home`, or a custom application directory. Compare against a baseline and monitor changes.

Capabilities:

```text
/usr/bin/ping cap_net_raw=ep
```

`cap_net_raw` on `ping` is expected. `cap_setuid`, `cap_setgid`, `cap_sys_admin`, `cap_sys_ptrace`, or `cap_dac_override` on an unexpected binary is a finding.

World-writable files in `/etc` or cron directories are a persistence path. A world-writable file in `/tmp` is normal only if `/tmp` is sticky and `noexec` where appropriate.

### Service sandboxing and host audit

```bash
sudo systemd-analyze security sshd.service | tail -20
sudo lynis audit system | tee evidence/30-lynis.txt
```

Lynis output includes a hardening index and suggestions. Do not apply every suggestion. Pick three that are relevant to this host, explain the risk, and record the exception for the rest.

Examples of justified exceptions:

- A legacy application requires an older TLS protocol on an internal network.
- A monitoring agent needs a capability that cannot be dropped.
- A package repository requires a specific user.

An exception without an owner and a review date is a finding that has been renamed.

### Auditd

```bash
sudo systemctl enable --now auditd
sudo auditctl -w /etc/passwd -p wa -k identity
sudo auditctl -l | tee evidence/40-audit-rules.txt
sudo useradd -m labuser2
sudo ausearch -k identity -ts recent | tee evidence/41-audit-events.txt
```

Representative output:

```text
type=SYSCALL msg=audit(...): arch=c000003e syscall=257 success=yes ... comm="useradd" ...
type=PATH msg=audit(...): item=0 name="/etc/passwd" ... nametype=CREATE
```

`ausearch` shows the write to `/etc/passwd` and the process. If the log is empty, check `auditctl -l`, the auditd service, and whether the rule persisted across reboot (`/etc/audit/rules.d/`).

Remove the test user and the temporary rule at the end:

```bash
sudo userdel -r labuser2
sudo auditctl -W /etc/passwd
```

## Part B — TLS without the mystery

Generate a self-signed certificate for `lab.local`:

```bash
openssl req -x509 -newkey rsa:2048 -nodes \
  -keyout /tmp/lab.key -out /tmp/lab.crt -days 1 \
  -subj "/CN=lab.local" \
  -addext "subjectAltName=DNS:lab.local"
```

Run a test TLS server in the background:

```bash
openssl s_server -accept 8443 -cert /tmp/lab.crt -key /tmp/lab.key -www &
echo $! > /tmp/lab-s_server.pid
```

Inspect the certificate:

```bash
openssl s_client -connect 127.0.0.1:8443 -servername lab.local -showcerts </dev/null 2>/dev/null \
  | openssl x509 -noout -subject -issuer -dates -ext subjectAltName
```

Expected:

```text
subject=CN = lab.local
issuer=CN = lab.local
notBefore=Oct 27 ...
notAfter=Oct 28 ...
X509v3 Subject Alternative Name:
    DNS:lab.local
```

Verify the hostname with the certificate as the trust anchor:

```bash
openssl s_client -connect 127.0.0.1:8443 -servername lab.local \
  -verify_hostname lab.local -CAfile /tmp/lab.crt </dev/null 2>&1 | grep -E 'Verify return code|Verification'
```

Expected:

```text
Verify return code: 0 (ok)
```

Now use the wrong hostname:

```bash
openssl s_client -connect 127.0.0.1:8443 -servername wrong.local \
  -verify_hostname wrong.local -CAfile /tmp/lab.crt </dev/null 2>&1 | grep -E 'Verify return code|Verification'
```

Expected:

```text
Verify return code: 62 (hostname mismatch)
```

And with `curl`:

```bash
curl --cacert /tmp/lab.crt --resolve lab.local:8443:127.0.0.1 -sS https://lab.local:8443/ -o /dev/null -w '%{http_code}\n'
```

Expected: `200` (the `-www` page). Without `--cacert`, `curl` fails with a self-signed certificate error. That is the correct behavior: a self-signed certificate is not trusted by the system store.

Browser vs `curl` differences:

- A browser may have cached an intermediate certificate or use the OS trust store; `curl` uses its own CA bundle unless told otherwise.
- A browser may allow a user to click through an expired or mismatched certificate; automation does not.
- A missing intermediate can work in a browser that fetches it via AIA and fail in a client that does not. Always test with the client that matters.

Cleanup:

```bash
kill "$(cat /tmp/lab-s_server.pid)"
rm -f /tmp/lab.key /tmp/lab.crt /tmp/lab-s_server.pid
```

## Part C — A secret in Git, found and handled

Create a repository with a fake AWS key. Use a documented example value, not a real credential:

```bash
cd ~/opslab/phase8/l8.4/labrepo
git init -q
cat > prod.env <<'EOF'
AWS_ACCESS_KEY_ID=AKIAIOSFODNN7EXAMPLE
AWS_SECRET_ACCESS_KEY=wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY
EOF
git add prod.env
git -c user.name=lab -c user.email=lab@example.invalid commit -qm "add prod env"
```

Scan:

```bash
gitleaks detect --source . --redact --verbose | tee ../evidence/50-gitleaks.txt
```

Expected: a finding for `AWS_ACCESS_KEY_ID` and/or the secret key, with the file, line, commit, and rule ID. The values are redacted in the output.

Also inspect history:

```bash
git log -p -S 'AKIA' --all -- . | tee ../evidence/51-git-history.txt
```

The right response order:

1. **Rotate/revoke** the credential in the provider, even though this is a fake key. In a real incident, assume it is compromised.
2. **Scope** the use: check provider audit logs, CI logs, and any downstream secret that the credential could read.
3. **Remove** the secret from the working tree.
4. **Purge history** only after rotation, if policy requires it. History rewriting does not un-leak the value.
5. **Prevent** recurrence with a pre-commit hook and CI scanning.

For this lab, the "rotation" is a note in the write-up: `FAKE KEY — revoke in provider, then replace`. Remove the file from the working tree and add a `.gitignore`:

```bash
git rm -q prod.env
printf '*.env\n' > .gitignore
git add .gitignore
git -c user.name=lab -c user.email=lab@example.invalid commit -qm "remove fake env and ignore env files"
```

Scan the working tree again:

```bash
gitleaks detect --source . --redact --verbose || true
```

The working tree is clean; the **history still contains the fake key**. Verify:

```bash
git log -p -S 'AKIA' --all -- . | head -20
```

That is the lesson: deleting the file is not remediation. If this were a real key, it would still be valid until rotated.

## Part D — Containers and Kubernetes (optional)

If you have a kind cluster:

```bash
kind create cluster --name audit-lab
kubectl create namespace audit-lab
```

### A privileged pod

```bash
kubectl -n audit-lab run privileged --image=busybox:1.36 --restart=Never \
  --overrides='{"spec":{"containers":[{"name":"privileged","image":"busybox:1.36","securityContext":{"privileged":true},"command":["sleep","3600"]}]}}'
kubectl -n audit-lab get pod privileged -o jsonpath='{.spec.containers[0].securityContext.privileged}{"\n"}'
```

Expected: `true`.

A privileged container has all capabilities and device access; it can often escape to the node. It is a node-level trust decision, not a normal application setting.

### A root pod and a hostPath

```bash
kubectl -n audit-lab run root --image=busybox:1.36 --restart=Never -- sleep 3600
kubectl -n audit-lab get pod root -o jsonpath='{.spec.containers[0].securityContext}{"\n"}'
```

An empty security context usually means the image's default user, often root. Check the image:

```bash
kubectl -n audit-lab get pod root -o jsonpath='{.spec.containers[0].image}{"\n"}'
```

`busybox` runs as root by default. A pod that does not set `runAsNonRoot` is a finding, even if the image happens to use a non-root user today; the next image tag can change it.

### RBAC from the subject's perspective

```bash
kubectl auth can-i --list --as=system:serviceaccount:audit-lab:default -n audit-lab
kubectl create serviceaccount limited -n audit-lab
kubectl auth can-i get secrets --as=system:serviceaccount:audit-lab:limited -n audit-lab
```

Expected:

```text
no
```

The default service account may have API discovery permissions but should not read secrets. If it can, find the binding:

```bash
kubectl get rolebindings,clusterrolebindings -A -o wide | grep -E 'audit-lab|default'
```

`get secrets` is effectively read access to every credential in the namespace. Treat it as privileged.

### Scan an image

```bash
trivy image --severity HIGH,CRITICAL --ignore-unfixed busybox:1.36 | tee evidence/60-trivy.txt
```

Record the digest you scanned:

```bash
docker inspect --format='{{index .RepoDigests 0}}' busybox:1.36
```

A tag can move; the digest is the artifact you actually deployed. Scan the digest in a real pipeline.

### Network policy caveat

If you apply a default-deny NetworkPolicy, verify enforcement with a test pod. Some CNIs (including some kind defaults) do not enforce NetworkPolicy. A policy that exists but is not enforced is worse than no policy because it creates false confidence.

```bash
kubectl -n audit-lab apply -f - <<'EOF'
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny
spec:
  podSelector: {}
  policyTypes: ["Ingress", "Egress"]
EOF
kubectl -n audit-lab run test --image=busybox:1.36 --restart=Never -- wget -qO- --timeout=3 http://kubernetes.default.svc
kubectl -n audit-lab logs test
```

If the connection succeeds, the CNI is not enforcing the policy. Document it; do not claim the namespace is isolated.

### Paper alternative (no cluster)

If kind is unavailable, answer these without creating resources:

1. Which Kubernetes role would allow a pod to read secrets in its namespace?
2. Why is `create pods` a privilege-escalation path on a cluster with privileged pods allowed?
3. What is the difference between `automountServiceAccountToken: false` and a NetworkPolicy?
4. Why does a `hostPath` mount of `/` bypass most container isolation?
5. What evidence would you collect to prove a pod is privileged?

Write the answers in the findings table. The paper alternative is not as strong as the lab, but it is better than skipping the concept.

## Part E — Fix, verify, and roll back

Pick **three** findings from Parts A–D. For each:

1. State the evidence (command and output).
2. State the risk in one sentence.
3. Make the smallest change that addresses it.
4. Verify with a command that would fail before the change.
5. Record the rollback.

Example findings and fixes:

| Finding | Evidence | Fix | Verification | Rollback |
|---|---|---|---|---|
| Test listener on `0.0.0.0:8088` | `ss -tulpn` | Kill the process or bind to `127.0.0.1` | `ss -tulpn \| grep 8088` shows no `0.0.0.0` | Restart the test service |
| World-readable fake secret | `stat -c '%a %n' /opt/lab/secret.env` | `chmod 600` | `stat` shows `600` | `chmod 644` if needed |
| Over-broad sudo rule | `sudo -l -U labuser` | Remove `/etc/sudoers.d/labuser` | `sudo -l -U labuser` fails or shows no rule | Recreate the file (lab only) |
| Privileged pod | `kubectl get pod -o jsonpath` | Delete the pod; enforce restricted PSS | Pod gone; admission rejects a new privileged pod | Recreate only in a lab namespace |
| Secret in Git | `gitleaks` | Revoke/rotate, remove, add scanning | Scanner clean; provider key revoked | Not applicable; rotation is one-way |

Do not change `sshd_config` or the host firewall as part of this lab. Write the proposed change and its validation plan instead. A lockout would end the lab and teach the wrong lesson.

## Expected observations

| Observation | Expected | Meaning |
|---|---|---|
| `ss -tulpn` | Most services on `127.0.0.1`; SSH on `0.0.0.0` | Exposure baseline |
| `sshd -T` | Password auth may be `yes` on a default cloud image | Finding, not a surprise |
| Test sudo rule | `(root) NOPASSWD: /usr/bin/find` | Looks narrow, is root-equivalent |
| SUID list | Standard binaries plus any custom ones | Unexpected SUID is a finding |
| TLS wrong hostname | `verify return code: 62` | Hostname verification works |
| `gitleaks` | Finding with commit/line | Secret in history, not just the file |
| Privileged pod | `true` | Node-level trust |
| Limited SA | `no` for secrets | RBAC is scoped |
| NetworkPolicy test | May still connect on kind | Enforcement is CNI-dependent |
| Auditd | `/etc/passwd` write event | Identity changes are logged |

## Internal explanation

### A listener is not an exposure

`ss` shows what a process bound to. Reachability also depends on:

- the interface (`127.0.0.1` vs `0.0.0.0`);
- the host firewall (`nftables`, `ufw`, `iptables`);
- the cloud security group / network ACL;
- the route from the client (VPC, VPN, internet);
- a reverse proxy or load balancer in front.

An audit that stops at `ss` overstates or understates risk. Test from the relevant network vantage point, and record the vantage point.

### Sudo rules are code execution

A sudo rule grants a command with the target user's privileges. If the command can:

- spawn a shell (`bash`, `sh`, `python`, `perl`, `awk`, `find -exec`, `vi`, `less`),
- write a file that another privileged process executes (cron, systemd unit, `authorized_keys`),
- change permissions (`chmod`, `chown`),
- edit a config that is later reloaded,

then it can often become full root. Scope the binary, the arguments, and the target; prefer a dedicated automation user with a narrow API over broad sudo.

### TLS verification is three checks

A client verifies:

1. **Chain:** the certificate chains to a trusted CA.
2. **Validity:** the current time is within `notBefore`/`notAfter`.
3. **Identity:** the requested hostname is in `subjectAltName`.

TLS also negotiates a protocol version and cipher. It does **not** check whether the client is authorized to call the endpoint; that is application authorization. mTLS adds client authentication, but still not authorization.

### Secrets in Git are permanent until rotated

Git history is distributed. A secret committed and deleted is still in:

- every clone and fork;
- CI caches and build logs;
- pull request refs;
- backup and mirror systems;
- the object database until garbage collection, if anyone kept the commit.

Rotation is the only reliable remediation. History rewriting reduces future exposure but does not invalidate the credential.

### Privileged pods are node trust

A privileged container can access devices, mount filesystems, and change kernel settings. A `hostPath` mount of `/` gives it the host filesystem. `hostPID` and `hostNetwork` remove namespace isolation. Admission control must block these outside a narrow, audited system namespace; RBAC alone is not enough if any user can create pods.

### Audit logs are evidence, not telemetry

An audit log that lives only on the host is lost when the host is compromised. Ship it to a separate system with restricted deletion. Test that the pipeline works before you need it. A log with no time sync is hard to correlate; a log with no retention is gone when the investigation starts.

## Troubleshooting task

A teammate says: "I ran the audit and found nothing, so the host is secure." You have their commands:

```text
ss -tulpn          (without sudo)
sudo -l            (as their own user)
find / -name '*.pem' -perm -004
lynis audit system
```

What are the gaps? What evidence would you ask for before accepting the conclusion? Write your answer first.

<details>
<summary>After you have written your evidence</summary>

Gaps:

- `ss` without `sudo` may hide process names and sockets owned by other users/namespaces. It still shows listeners, but the attribution is incomplete.
- `sudo -l` as their own user checks only their rights, not the rights of service accounts or other users. `sudo -l -U <user>` is needed for a specific account.
- `find / -name '*.pem' -perm -004` finds world-readable PEM files by name, not keys in other formats, environment variables, agent sockets, or secrets in Git/images.
- `lynis` is a host baseline, not a cloud IAM, Kubernetes RBAC, TLS, or supply-chain audit.
- "Found nothing" may mean the tools are not installed, the output was truncated, or the audit ran in the wrong namespace/account/region.

Ask for: the exact commands and output, the user/role/account used, the scope (host, cluster, cloud account), and the time. A negative finding is only as good as the scope and the tool.

</details>

## Verification

- [ ] You recorded the VM baseline and took a snapshot before changes.
- [ ] You can explain the difference between a listener and a reachable service.
- [ ] You found the over-broad sudo rule and removed it.
- [ ] You inspected SUID/capabilities and identified at least one expected and one unexpected item (or explained why none were unexpected).
- [ ] You verified a TLS certificate with the correct hostname and saw the mismatch failure with the wrong hostname.
- [ ] You found the fake secret in Git history, removed it from the working tree, and stated that rotation is required.
- [ ] You identified a privileged/root pod and checked RBAC from the service account's perspective (or completed the paper alternative).
- [ ] You tested NetworkPolicy enforcement instead of assuming it.
- [ ] You fixed three findings with evidence and rollback.
- [ ] You removed the test user, listener, certificate, and namespace.
- [ ] You did not lock yourself out of SSH or the VM.

## Cleanup

```bash
# Host test artifacts
sudo rm -f /etc/sudoers.d/labuser
sudo userdel -r labuser 2>/dev/null || true
sudo userdel -r labuser2 2>/dev/null || true
kill "$(cat /tmp/lab-s_server.pid)" 2>/dev/null || true
rm -f /tmp/lab.key /tmp/lab.crt /tmp/lab-s_server.pid

# Kubernetes lab
kubectl delete namespace audit-lab --ignore-not-found
kind delete cluster --name audit-lab

# The evidence directory is intentionally kept for the write-up.
```

If you changed any live SSH/firewall setting, restore the snapshot or reapply the original config and verify a new login from a second session. A cleanup that leaves the host unreachable is not complete.

## Write-up template

```markdown
# L8.4 — Security Audit

## Scope and safety
- VM/image:
- Snapshot taken:
- Console available:
- Cluster/account (if any):
- Out of scope:

## Findings
| # | Asset | Evidence (command/output) | Risk | Fix | Verification | Rollback |
|---|---|---|---|---|---|---|
| 1 | | | | | | |
| 2 | | | | | | |
| 3 | | | | | | |

## Host exposure
- Listening ports and bind addresses:
- SSH configuration:
- Sudo rules:
- SUID/capabilities:
- World-writable files:
- Lynis index and accepted exceptions:

## TLS
- Certificate subject/issuer/dates:
- SAN and hostname verification:
- Wrong-hostname result:
- Client vs browser difference:

## Secrets
- Scanner output:
- Commit/line:
- Rotation decision:
- History still contains it?:
- Prevention added:

## Kubernetes (or paper)
- Privileged/root pods:
- hostPath/hostNetwork:
- RBAC for the limited SA:
- NetworkPolicy enforcement:
- Image digest scanned:

## Audit logging
- Rule:
- Event observed:
- Where shipped:
- Retention:

## Three fixes
1.
2.
3.

## What I would do first in a real environment
```

---

## Appendix — Lab checklist

Use this before you start either lab:

- [ ] I know whether I am on a disposable VM, a local machine, or a shared host.
- [ ] I have a snapshot or a rollback path.
- [ ] I have console access if I am changing SSH/firewall/RBAC.
- [ ] I am not sending alerts to a real on-call channel.
- [ ] I am not load-testing a service I do not own.
- [ ] I know how to clean up every resource I create.
- [ ] I have a place to record evidence before I change anything.
