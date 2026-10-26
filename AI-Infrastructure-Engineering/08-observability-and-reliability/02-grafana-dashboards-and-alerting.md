# Grafana, Dashboards, and Alerting

> **The one-sentence model:** a dashboard is a decision surface, and an alert is a request for a human to act. Grafana renders PromQL; Alertmanager decides who is told, how often, and when to stop. Neither is useful without an owner, a threshold tied to user impact, and a runbook.

---

## 1. Grafana's model

```text
data source (Prometheus/Loki/Tempo)
        │
        ▼
dashboard ──► panels ──► queries ──► visualization
        │
        ├── variables (instance, namespace, route)
        ├── annotations (deploys, incidents)
        └── links (runbook, logs, traces)
```

| Concept | Meaning | Gotcha |
|---|---|---|
| **Data source** | Connection to Prometheus/Loki/Tempo | `proxy` access runs from the Grafana server; `direct` runs from the browser and needs network reachability |
| **Dashboard** | Collection of panels and variables | JSON model is the source of truth; UI edits can be overwritten by provisioning |
| **Panel** | One visualization + one or more queries | A panel with five unrelated queries is a dashboard in a panel |
| **Variable** | Query parameter (`$instance`, `$namespace`) | A variable with `All` can generate a huge query; bound it |
| **Annotation** | Event marker on a time series | Deploy annotations turn "what changed?" into a visual |
| **Folder** | Organization/permissions boundary | Folder permissions matter in multi-team Grafana |
| **Provisioning** | Dashboards/data sources from files | `allowUiUpdates` decides whether UI edits survive |
| **Alert rule** | Grafana-managed or Prometheus rule | Do not duplicate the same alert in both systems |

### Grafana vs Prometheus alerting

| Approach | Strengths | Weaknesses | Use for |
|---|---|---|---|
| **Prometheus rules + Alertmanager** | GitOps-friendly, `promtool` unit tests, mature routing/inhibition | No native UI for silences (use `amtool`/Alertmanager UI) | Infrastructure/product alerts |
| **Grafana-managed alerts** | UI, multiple data sources, unified alerting | State lives in Grafana DB; harder to review/test as code | Grafana-specific or multi-source alerts |
| **Both** | — | Duplicate pages, split ownership, inconsistent thresholds | Avoid |

Pick one owner per alert. If Prometheus evaluates it, Alertmanager routes it; Grafana only visualizes it. If Grafana evaluates it, document why and export the rule as code.

---

## 2. Dashboard design

### The first row answers "are users affected?"

```text
┌─────────────────────┬─────────────────────┬─────────────────────┬─────────────────────┐
│ Availability / SLO  │ Error ratio (5m)    │ p95 latency (5m)    │ Request rate        │
│ 99.94%              │ 0.8%                │ 240 ms              │ 1.2k req/s          │
└─────────────────────┴─────────────────────┴─────────────────────┴─────────────────────┘
```

Then narrow the fault domain:

```text
Row 2: errors by route, latency by route, top failing dependencies
Row 3: per-instance request rate, CPU throttling, memory, restarts
Row 4: node CPU/memory/disk/network, database/storage signals
```

If the first row is green, the user impact is probably fine even if a host is busy. If the first row is red, the lower rows help locate the cause. Do not make the viewer scroll through 40 host graphs to discover that users are seeing 500s.

### Panel choices

| Question | Panel | Notes |
|---|---|---|
| Is the value good/bad now? | Stat | Set unit, thresholds, and a clear "no data" state |
| How has it changed over time? | Time series | One unit, limited series, legend with labels |
| What is the distribution? | Heatmap | Histogram buckets over time; good for latency |
| Which route/instance is worst? | Table + `topk` | Sort descending; include request count |
| Is the queue growing? | Time series + gauge | Show absolute depth and rate of change |
| Did a deploy change behavior? | Time series + annotation | Mark the deploy time and version |
| Is the SLO burning? | Stat/gauge + burn-rate time series | Show budget remaining and burn rate |

Avoid:

- dual Y axes with unrelated units (it invites false correlation);
- pie charts for time series;
- more than ~7 series in one panel without a `topk`/legend strategy;
- raw counters without `rate()`/`increase()`;
- a dashboard with no time picker or refresh policy;
- red/green thresholds without a documented reason.

### Variables

```text
$instance   label_values(up{job="node"}, instance)
$namespace  label_values(kube_pod_info, namespace)
$route      label_values(http_requests_total{job="api"}, route)
```

- Use `$__rate_interval` in `rate()` so the window adapts to the scrape interval and dashboard range.
- Use `$__range` for "over the dashboard time range" calculations.
- Bound "All" values; a query with `instance=~".*"` over 500 instances can time out.
- Use chained variables only when the dependency is real (namespace → pod), and test the empty case.

Example panel query:

```promql
sum by (status) (
  rate(http_requests_total{job="api",instance=~"$instance"}[$__rate_interval])
)
```

Legend: `{{status}}`. Unit: `reqps`. Min step: `15s` or let Grafana calculate; do not set a step smaller than the scrape interval.

### Dashboard as code

`provisioning/datasources/prometheus.yml`:

```yaml
apiVersion: 1
datasources:
  - name: Prometheus
    type: prometheus
    access: proxy
    url: http://prometheus:9090
    isDefault: true
    editable: false
    jsonData:
      timeInterval: 15s
```

`provisioning/dashboards/default.yml`:

```yaml
apiVersion: 1
providers:
  - name: opslab
    orgId: 1
    folder: OpsLab
    type: file
    disableDeletion: false
    updateIntervalSeconds: 30
    allowUiUpdates: false
    options:
      path: /var/lib/grafana/dashboards
```

A minimal panel JSON (inside a dashboard model):

```json
{
  "type": "timeseries",
  "title": "Request rate by status",
  "datasource": {"type": "prometheus", "uid": "prometheus"},
  "targets": [
    {
      "refId": "A",
      "expr": "sum by (status) (rate(http_requests_total{job=\"api\"}[$__rate_interval]))",
      "legendFormat": "{{status}}"
    }
  ],
  "fieldConfig": {
    "defaults": {"unit": "reqps", "min": 0},
    "overrides": []
  }
}
```

`allowUiUpdates: false` means UI edits are overwritten on the next provision cycle. That is good for reviewed dashboards and bad for an incident hotfix; decide deliberately. Keep the dashboard JSON in Git, review changes, and use the Grafana API for export.

```bash
# Export a dashboard by UID (requires auth).
curl -fsS -u admin:admin \
  http://localhost:3000/api/dashboards/uid/opslab-overview \
  | jq '.dashboard' > dashboards/opslab-overview.json

# Import/update.
curl -fsS -u admin:admin -X POST \
  -H 'Content-Type: application/json' \
  --data @dashboards/import.json \
  http://localhost:3000/api/dashboards/db
```

Do not commit the Grafana admin password or API key. Use a scoped service account token in CI.

---

## 3. Alert design

### Symptom vs cause

| Alert | Type | Page? | Why |
|---|---|---|---|
| `ApiHighErrorRate` | Symptom | Yes | Users see failures |
| `ApiHighLatencyP95` | Symptom | Yes | Users wait too long |
| `TargetDown` | Cause/availability | Yes if user-facing | Scrape failure may mean the service is unreachable |
| `NodeCpuHigh` | Cause/saturation | Usually no | High CPU may be normal; page only if latency/errors follow |
| `DiskWillFillIn4h` | Predictive cause | Ticket/page depending on margin | Actionable before outage |
| `PodRestarted` | Cause | No (ticket) | One restart is often normal |
| `CertificateExpiresIn14d` | Predictive | Ticket | Enough time to renew |
| `SLOBudgetBurn` | Symptom/aggregate | Yes at high burn | Directly tied to user reliability |

A page should mean "a human must act now." Everything else is a ticket, a dashboard, or a log entry.

### Rule anatomy

```yaml
groups:
  - name: api.rules
    interval: 30s
    rules:
      - alert: ApiHighErrorRate
        expr: |
          (
            sum(rate(http_requests_total{job="api",status=~"5.."}[5m]))
            /
            sum(rate(http_requests_total{job="api"}[5m]))
          ) > 0.05
        for: 5m
        labels:
          severity: critical
          service: capacity-api
          team: platform
        annotations:
          summary: "capacity-api 5xx ratio above 5%"
          description: "5xx ratio is {{ $value | humanize }} over 5m."
          runbook_url: "https://example.com/runbooks/capacity-api-high-error-rate"
          dashboard_url: "https://grafana.example.com/d/opslab-overview"
```

- `for: 5m` requires the condition to hold for 5 minutes before firing; it suppresses single-scrape spikes.
- `keep_firing_for` (newer Prometheus) keeps an alert firing for a grace period after the condition clears, reducing flapping.
- Labels are for routing/grouping; annotations are for humans.
- `severity`, `service`, `team`, and `runbook_url` should be present on every page-worthy alert.
- Do not put a high-cardinality value (pod name, request ID) in labels. Put it in the annotation if needed.
- The expression should be a ratio/percentile tied to user impact, not a raw count.

### More rule examples

```yaml
      - alert: ApiHighLatencyP95
        expr: |
          histogram_quantile(
            0.95,
            sum by (le) (rate(http_request_duration_seconds_bucket{job="api"}[5m]))
          ) > 0.5
        for: 10m
        labels:
          severity: warning
          service: capacity-api
          team: platform
        annotations:
          summary: "capacity-api p95 latency above 500ms"
          description: "p95 is {{ $value | humanizeDuration }} over 5m."
          runbook_url: "https://example.com/runbooks/capacity-api-high-latency"

      - alert: TargetDown
        expr: up{job=~"api|node"} == 0
        for: 2m
        labels:
          severity: critical
          team: platform
        annotations:
          summary: "Scrape target {{ $labels.instance }} is down"
          description: "Prometheus has failed to scrape {{ $labels.job }} for 2m."
          runbook_url: "https://example.com/runbooks/target-down"

      - alert: DiskWillFillIn4Hours
        expr: |
          predict_linear(
            node_filesystem_avail_bytes{fstype=~"ext4|xfs",mountpoint="/"}[6h],
            4 * 3600
          ) < 0
        for: 30m
        labels:
          severity: warning
          team: platform
        annotations:
          summary: "Filesystem {{ $labels.mountpoint }} on {{ $labels.instance }} will fill in <4h"
          description: "Current available: {{ $value | humanize }} bytes (trend-based)."
          runbook_url: "https://example.com/runbooks/disk-fill"
```

`humanizeDuration` expects seconds; for a ratio use `humanizePercentage` only if your template environment supports it, otherwise use `humanize` and label the unit in the description. Test the rendered notification, not just the rule.

### `for`, evaluation interval, and flapping

- `evaluation_interval` (Prometheus global) controls how often rules run; `groups[].interval` overrides it.
- `for` must be a multiple of the evaluation interval, or the effective delay is rounded up.
- A `for: 0` alert fires on the first evaluation and can flap on every scrape.
- If the condition oscillates around the threshold, add hysteresis: use different thresholds for firing and resolving, or a longer window, or `keep_firing_for`.
- Do not hide a flapping alert with a long silence; fix the threshold or the underlying instability.

### Mult-window burn-rate alerts

For SLO-based paging, use a fast and a slow window so a brief spike does not page and a slow burn is not missed. The full math is in [SLOs, error budgets, capacity, and incidents](03-slos-incidents-and-capacity.md). The shape:

```promql
# Fast burn: 14.4x budget consumption over 1h and 5m.
(
  job:slo_errors:ratio_rate1h > (14.4 * 0.001)
  and
  job:slo_errors:ratio_rate5m > (14.4 * 0.001)
)
```

Alert on the combination, not on a single window.

---

## 4. Alertmanager

Prometheus evaluates rules and sends firing/resolved alerts to Alertmanager. Alertmanager deduplicates, groups, inhibits, silences, and routes them to receivers.

```text
Prometheus rule fires
        │
        ▼
Alertmanager ── dedupe/group/inhibit/silence
        │
        ├──► critical receiver (pager)
        ├──► warning receiver (chat/ticket)
        └──► default receiver (email/webhook)
```

### Routing tree

```yaml
global:
  resolve_timeout: 5m

route:
  receiver: default
  group_by: [alertname, cluster, service]
  group_wait: 30s
  group_interval: 5m
  repeat_interval: 4h
  routes:
    - matchers:
        - severity="critical"
      receiver: critical
      group_wait: 10s
      group_interval: 2m
      repeat_interval: 1h
      continue: false
    - matchers:
        - severity="warning"
      receiver: warning
      group_wait: 1m
      group_interval: 10m
      repeat_interval: 12h

inhibit_rules:
  - source_matchers:
      - severity="critical"
    target_matchers:
      - severity="warning"
    equal: [alertname, cluster, service]

  - source_matchers:
      - alertname="TargetDown"
    target_matchers:
      - alertname=~"ApiHighErrorRate|ApiHighLatencyP95"
    equal: [cluster, job]

receivers:
  - name: default
    webhook_configs:
      - url: http://receiver:5001/default
        send_resolved: true
  - name: critical
    webhook_configs:
      - url: http://receiver:5001/critical
        send_resolved: true
  - name: warning
    webhook_configs:
      - url: http://receiver:5001/warning
        send_resolved: true
```

- `group_by` decides which alerts share a notification. Grouping by `alertname, cluster, service` prevents 50 pods from producing 50 pages.
- `group_wait` delays the first notification to collect related alerts.
- `group_interval` controls updates for a group.
- `repeat_interval` re-sends a still-firing alert; too short causes fatigue, too long lets it be forgotten.
- `continue: false` stops matching after the first route; `true` allows fall-through to later routes.
- Inhibition suppresses a target alert when a higher-severity source is active. `equal` must list labels present on both alerts.
- A silence is manual and time-bound; an inhibition is automatic and rule-based.

### Silence and test

```bash
amtool check-config alertmanager.yml
amtool config routes show --alertmanager.url=http://localhost:9093
amtool alert query --alertmanager.url=http://localhost:9093

amtool silence add \
  alertname=TargetDown \
  instance=node-exporter:9100 \
  --duration=2h \
  --author=opslab \
  --comment="planned maintenance" \
  --alertmanager.url=http://localhost:9093

amtool silence query --alertmanager.url=http://localhost:9093
amtool silence expire <silence-id> --alertmanager.url=http://localhost:9093
```

Add a synthetic alert:

```bash
amtool alert add \
  alertname=LabTestAlert \
  severity=warning \
  service=capacity-api \
  --annotation=summary="synthetic alert from L8.2" \
  --alertmanager.url=http://localhost:9093
```

Expected: the receiver receives the alert; the Alertmanager UI shows it; after the resolve timeout or a manual delete, it resolves. If nothing arrives, check the route matcher, receiver URL, Alertmanager logs, and whether the alert was silenced/inhibited.

### Runbook template

Every page should link a runbook with:

1. **Impact:** what users see, which SLO is at risk.
2. **Confirm:** the exact query/dashboard and what "bad" looks like.
3. **Mitigate:** rollback, failover, scale, disable flag, drain, restart (in that order of preference).
4. **Verify:** the metric/query and smoke test that prove recovery.
5. **Escalate:** who, when, and with what evidence.
6. **Postmortem:** what to capture and where.

A runbook that says "check the logs" is not a runbook. It should name the command, the expected output, and the decision.

---

## 5. Failure scenarios

### A — Alert never fires

**Evidence:** rule state in Prometheus (`/api/v1/rules`), `promtool test rules`, raw query at the evaluation time, `for` duration, evaluation interval, target health, traffic volume.

Common causes: query returns empty (label mismatch/wrong metric), `for` longer than the incident, rule file not loaded, `evaluation_interval` not what you think, denominator zero, or the condition is below threshold. Test the rule with synthetic series; do not wait for a real outage.

### B — Alert fires but no notification

**Evidence:** Prometheus `/api/v1/alerts`, Alertmanager UI/API, `amtool alert query`, Alertmanager logs, route config, receiver response, silence/inhibition list.

Common causes: Alertmanager target wrong, no matching route, receiver auth/TLS failure, alert silenced/inhibited, `group_wait` still pending, or the receiver accepted but dropped the message. Trace the alert through each hop.

### C — Alert fires forever

**Evidence:** current query value, resolve condition, `for`, `keep_firing_for`, alert start time, recent changes.

Common causes: the expression never goes below threshold (e.g., a gauge with a stale value), `for: 0` and a flapping condition, a counter used as a gauge, or a threshold set below normal. Fix the query/threshold; do not silence it permanently.

### D — Duplicate pages

**Evidence:** Prometheus rules and Grafana alert rules with the same name; Alertmanager routes with multiple receivers; `group_by`; notification history.

Common causes: the same condition defined in both Prometheus and Grafana, multiple matching routes with `continue: true`, or grouping by a high-cardinality label. Choose one owner, remove duplicates, and group correctly.

### E — Alert storm

**Evidence:** alert count by name, dependency graph, inhibition rules, `group_by`, start times.

One cause (a node down) can trigger dozens of symptom alerts. Use inhibition (node down suppresses pod alerts on that node), grouping, and dependency-aware routing. Do not page on every downstream symptom.

### F — Silence forgotten

**Evidence:** `amtool silence query`, expiry, author, comment, linked change.

Silences must expire. Use a maximum duration, an owner, and a ticket/change reference. Review active silences weekly. A permanent silence is a disabled alert with extra steps.

### G — Dashboard is slow or times out

**Evidence:** Grafana query inspector, Prometheus query stats, cardinality, time range, number of panels, `$__rate_interval`.

Common causes: high-cardinality regex, no recording rule, huge time range, too many series, `rate()` over a tiny step, or a datasource timeout. Reduce the query, add a recording rule, or bound the variables.

### H — Dashboard and alert disagree

**Evidence:** exact PromQL in both, time range, step, recording rule, `for`, filters.

Different windows (`5m` vs `1m`), filters (one includes health checks), or aggregation labels produce different numbers. Put shared logic in a recording rule or document the difference.

### I — Grafana cannot reach Prometheus

**Evidence:** Grafana datasource test, `proxy` vs `direct`, URL/DNS, network policy, TLS, auth.

With `access: proxy`, Grafana's server must reach Prometheus; with `direct`, the browser must. In Docker Compose, `localhost` inside the Grafana container is not the host. Use the service name (`http://prometheus:9090`).

### J — Provisioned dashboard changes disappear

**Evidence:** provisioning config, `allowUiUpdates`, `disableDeletion`, Grafana logs, file mtime.

If `allowUiUpdates: false`, UI edits are overwritten. Edit the JSON in Git, or change the policy deliberately. Do not debug a "Grafana bug" that is provisioning doing its job.

---

## 6. Command field manual

### `amtool`

```bash
amtool check-config alertmanager.yml
amtool config routes show --alertmanager.url=http://localhost:9093
amtool alert query --alertmanager.url=http://localhost:9093
amtool alert add alertname=LabTest severity=warning --alertmanager.url=http://localhost:9093
amtool silence add alertname=LabTest --duration=1h --author=opslab --comment="test" \
  --alertmanager.url=http://localhost:9093
amtool silence query --alertmanager.url=http://localhost:9093
amtool silence expire <id> --alertmanager.url=http://localhost:9093
```

**Why:** validate config before reload, inspect the routing tree, inject a test alert, and manage silences without the UI. `check-config` catches YAML/schema errors but not a route that matches nothing; use `config routes show` and a synthetic alert.

### Prometheus alert API

```bash
curl -s http://localhost:9090/api/v1/rules | jq '.data.groups[].rules[] | {name, state, health, lastError}'
curl -s http://localhost:9090/api/v1/alerts | jq '.data.alerts[] | {labels, state, activeAt, value}'
```

**Interpretation:** `state` is `inactive`, `pending`, or `firing`. `health` is `ok`, `err`, or `unknown`; a rule with `health: err` is not evaluating. A pending alert that never fires may have a `for` longer than the incident or a condition that flaps.

### Alertmanager API

```bash
curl -s http://localhost:9093/api/v2/status | jq
curl -s http://localhost:9093/api/v2/alerts | jq
curl -s http://localhost:9093/api/v2/silences | jq
```

**Why:** see what Alertmanager actually received, not what Prometheus intended to send. If Prometheus shows firing but this API is empty, the connection/route is broken.

### Grafana API

```bash
curl -fsS http://localhost:3000/api/health | jq
curl -fsS -u admin:admin http://localhost:3000/api/datasources | jq
curl -fsS -u admin:admin 'http://localhost:3000/api/search?query=OpsLab' | jq
curl -fsS -u admin:admin http://localhost:3000/api/dashboards/uid/opslab-overview | jq '.dashboard.title'
```

**Why:** verify Grafana is up, the datasource exists, and the dashboard is provisioned. Do not use `admin:admin` outside the lab; use a scoped service account.

### `promtool`

```bash
promtool check rules /etc/prometheus/rules/*.yml
promtool test rules tests/rules.test.yml
promtool query instant http://localhost:9090 'ALERTS{alertstate="firing"}'
```

**Why:** unit-test alert expressions with synthetic series and confirm the alert state. A rule that parses is not a rule that fires.

---

## 7. Alert quality checklist

- [ ] The alert describes a user-visible symptom or a predictive capacity risk, not a raw resource value.
- [ ] The expression has a denominator and a bounded label set.
- [ ] `for` is longer than a scrape flake and shorter than the acceptable detection delay.
- [ ] The alert has `severity`, `service`, `team`, `runbook_url`, and a dashboard link.
- [ ] The route sends it to the right receiver; a synthetic alert proves it.
- [ ] Grouping/inhibition prevents a storm; silences expire and have an owner.
- [ ] The runbook has confirm/mitigate/verify/escalate steps.
- [ ] The rule is tested with `promtool test rules`.
- [ ] The alert resolves when the condition clears; `send_resolved` is on where appropriate.
- [ ] The threshold is reviewed after every incident and every major launch.

---

## 8. Checkpoint

1. What is the difference between a dashboard and an alert?
2. Why should the first dashboard row answer "are users affected?"
3. What is the difference between Grafana `proxy` and `direct` data source access?
4. Why is `allowUiUpdates: false` both useful and risky?
5. What does `group_by` do in Alertmanager?
6. What is the difference between `group_wait`, `group_interval`, and `repeat_interval`?
7. What is the difference between an inhibition and a silence?
8. Why is a permanent silence dangerous?
9. Why does `for: 0` cause flapping?
10. What does `health: err` on a Prometheus rule mean?
11. Why can an alert be firing in Prometheus but absent in Alertmanager?
12. Why is "CPU > 90%" usually a bad page?
13. What labels should every page-worthy alert carry?
14. What belongs in a runbook?
15. Why is a duplicate alert in Grafana and Prometheus a problem?
16. What is the first thing you check when a dashboard is empty?
17. Why can a dashboard and an alert show different values?
18. How do you test an alert without waiting for a real outage?
19. Why is `repeat_interval` a fatigue knob?
20. What would you change if an alert storm pages 40 times for one node failure?

Then do [Lab 8.2](07-labs-part1.md), where you provision a dashboard, write alert rules, route a synthetic alert, and prove the runbook path.
