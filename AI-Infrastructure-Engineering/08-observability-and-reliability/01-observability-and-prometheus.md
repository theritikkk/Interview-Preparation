# Observability and Prometheus

> **The one-sentence model:** Prometheus periodically pulls numeric time series from HTTP endpoints, stores them with labels, and lets you query rates, ratios, and distributions over time. It is excellent at "how much / how many / how fast" and bad at "why" — pair it with logs, traces, and events.

---

## 1. The four signals

| Signal | Answers | Cost | Retention | Example |
|---|---|---|---|---|
| **Metric** | How much, how many, how fast, how full? | Low per sample, grows with series × time | Weeks–months | `http_requests_total`, `node_memory_MemAvailable_bytes` |
| **Log** | What exactly happened in this request/process? | High volume, expensive to index | Days–weeks | Structured JSON with `request_id`, `level`, `error` |
| **Trace** | Where did this request spend time across services? | Sampling + storage | Hours–days | Span `db.query` 240 ms inside request `GET /orders` |
| **Event/audit** | Who changed what, and when? | Low volume, high value | Months–years | Kubernetes event, cloud trail, `sudo` log |

You need all four for a mature system, but not at the same depth. Start with metrics for detection, logs for diagnosis, traces for cross-service latency, and audit events for accountability.

### Golden signals, USE, and RED

| Framework | Components | Best for |
|---|---|---|
| **Golden signals** | Latency, traffic, errors, saturation | User-facing services |
| **RED** | Rate, errors, duration | Request-driven services |
| **USE** | Utilization, saturation, errors | Hosts, disks, network, pools |

They overlap. Use RED for a service, USE for the resource underneath it, and golden signals to keep the user perspective.

| Layer | Metrics |
|---|---|
| Load balancer | Request rate, 4xx/5xx ratio, p50/p95/p99 latency, active connections, upstream health |
| Application | Request rate by route/status, error ratio, latency histogram, queue depth, worker saturation, dependency latency |
| Host | CPU utilization/steal, load average, memory available, swap, disk space/inodes, disk IOPS/latency/queue, network drops/errors |
| Kubernetes | Pod restarts, readiness, pending pods, CPU/memory requests vs usage, throttling, node conditions |
| Database | Connections, query latency, slow queries, replication lag, locks, disk usage |
| Storage | Capacity, IOPS, throughput, latency, errors, rebuild status |

A metric without a unit and a decision is trivia. Name the unit in the metric name (`_seconds`, `_bytes`, `_total`) and write down what action a threshold would trigger.

---

## 2. Pull vs push

Prometheus **pulls** metrics over HTTP on a schedule. The target exposes a text format at `/metrics`; Prometheus stores the samples.

```text
Prometheus ── GET /metrics every 15s ──► node_exporter :9100
           ── GET /metrics every 15s ──► api :8080
           ── GET /metrics every 15s ──► kubelet/cAdvisor
```

| Model | Advantages | Disadvantages |
|---|---|---|
| **Pull** | Prometheus controls scrape rate; target health is visible via `up`; no credentials on every app; easy service discovery | Target must be reachable; short-lived jobs are awkward; NAT/firewall complexity |
| **Push** | Works through NAT; natural for batch jobs | App controls rate; backpressure/retry complexity; a dead app may look "silent" rather than down |

**Pushgateway** is for batch jobs that cannot be scraped. It is not a general-purpose metric relay and it does not expire series automatically. Do not push long-lived service metrics through it.

Prometheus is not a log store, a billing system, or a horizontally scalable cluster by itself. For long-term retention and global query, use remote write to Thanos, Mimir, or VictoriaMetrics; for logs, use Loki/Elasticsearch; for traces, Tempo/Jaeger.

---

## 3. Metric types

| Type | Behavior | Query pattern | Example |
|---|---|---|---|
| **Counter** | Monotonically increases (resets on restart) | `rate()`, `increase()` | `http_requests_total` |
| **Gauge** | Goes up and down | Direct value, `avg_over_time()` | `node_memory_MemAvailable_bytes` |
| **Histogram** | Buckets of observations + sum + count | `histogram_quantile()` | `http_request_duration_seconds_bucket` |
| **Summary** | Client-side quantiles + sum + count | Direct quantile labels | `rpc_duration_seconds{quantile="0.95"}` |

### Exposition format

```text
# HELP http_requests_total Total HTTP requests.
# TYPE http_requests_total counter
http_requests_total{method="GET",route="/health",status="200"} 1027
http_requests_total{method="GET",route="/capacity",status="500"} 3

# HELP http_request_duration_seconds Request latency.
# TYPE http_request_duration_seconds histogram
http_request_duration_seconds_bucket{le="0.05"} 900
http_request_duration_seconds_bucket{le="0.1"} 980
http_request_duration_seconds_bucket{le="0.25"} 1000
http_request_duration_seconds_bucket{le="+Inf"} 1000
http_request_duration_seconds_sum 42.7
http_request_duration_seconds_count 1000
```

The histogram's `le` label means "less than or equal to this many seconds." The `+Inf` bucket equals the count. `histogram_quantile` estimates a quantile by interpolating within the bucket that contains it; coarse buckets produce coarse estimates.

### Labels and cardinality

Labels are dimensions. Every unique combination is a separate time series.

Good labels:

- `method` (GET/POST/PUT/DELETE)
- `status` (200/400/500)
- `route` (a **template**: `/users/{id}`)
- `instance`, `job`, `environment`, `region`
- `version` (bounded, if you need deploy comparison)

Bad labels:

- raw URL path (`/users/12345`, `/orders/9f3...`)
- user ID, email, request ID, session ID
- error message text
- timestamps
- unbounded query parameters
- high-cardinality IPs (unless the cardinality is genuinely bounded)

Estimate series count as the product of label cardinalities. A metric with `method` (5) × `status` (10) × `route` (100) × `instance` (50) = 250,000 series. At 15-second scrapes, that is a lot of memory and disk. Add labels only when you will query/aggregate by them.

Use route templates, status classes, and bounded enums. If you need per-request detail, use logs or traces.

---

## 4. PromQL

### Selectors and vectors

```promql
up
up{job="node"}
up{job=~"node|api"}
up{job!="prometheus"}
http_requests_total{status=~"5.."}[5m]
```

- `=` exact, `!=` not equal, `=~` regex (fully anchored), `!~` negative regex.
- `[5m]` is a **range vector**: many samples per series over the last 5 minutes.
- A bare selector returns an **instant vector**: one sample per series at the evaluation time.

### Counters: `rate` and `increase`

```promql
# Requests per second over the last 5 minutes, per series.
rate(http_requests_total[5m])

# Total requests over the last 5 minutes (extrapolated).
increase(http_requests_total[5m])
```

`rate()` expects a counter and at least two samples in the window. Use a window at least 4× the scrape interval; with a 15s scrape, a 1m window has ~4 samples and is noisy. `5m` is a common default.

`irate()` uses only the last two samples. It reacts quickly but is spiky and can miss a short spike between scrapes. Use it for interactive debugging, not for SLOs or paging alerts.

Counter resets (process restart) are handled by `rate()`; a raw counter graph will show a drop. Do not "fix" a reset with `clamp_min` or by summing raw counters across restarts.

### Aggregation

```promql
sum(rate(http_requests_total[5m]))
sum by (route) (rate(http_requests_total[5m]))
sum without (instance, pod) (rate(http_requests_total[5m]))
avg by (instance) (rate(node_cpu_seconds_total{mode="idle"}[5m]))
topk(5, sum by (route) (rate(http_requests_total[5m])))
```

`by` keeps the listed labels; `without` drops them. Aggregating away `instance` hides a single bad replica; aggregate only after you have looked at per-instance data.

### Error ratio

```promql
sum(rate(http_requests_total{status=~"5.."}[5m]))
/
sum(rate(http_requests_total[5m]))
```

Rules:

- The numerator must be a subset of the denominator.
- Use the same window and filters on both sides.
- Exclude health checks and synthetic probes from the user-facing SLI if they are not real traffic.
- A ratio of two empty vectors is empty, not zero. Use `or vector(0)` only when "no traffic" should be treated as good, and document it.

### Latency percentiles

```promql
histogram_quantile(
  0.95,
  sum by (le) (rate(http_request_duration_seconds_bucket[5m]))
)
```

- Keep `le` in the `by` clause.
- Do not average percentiles across instances; aggregate the buckets first.
- Use enough buckets around the SLO threshold (for example 50ms, 100ms, 250ms, 500ms, 1s, 2.5s).
- `histogram_quantile` extrapolates; a p99 from a histogram with a 5s max bucket is not a precise 5s measurement.

### Saturation and capacity

```promql
# CPU utilization per instance (0–1).
1 - avg by (instance) (rate(node_cpu_seconds_total{mode="idle"}[5m]))

# Memory available ratio.
node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes

# Filesystem usage (exclude tmpfs and read-only mounts).
1 - (
  node_filesystem_avail_bytes{fstype=~"ext4|xfs",mountpoint!~"/run.*"}
  /
  node_filesystem_size_bytes{fstype=~"ext4|xfs",mountpoint!~"/run.*"}
)

# Disk will fill within 4 hours at the current 6-hour trend.
predict_linear(node_filesystem_avail_bytes{fstype=~"ext4|xfs"}[6h], 4*3600) < 0

# Load per CPU (rough; Linux load includes uninterruptible I/O).
node_load1 / count by (instance) (node_cpu_seconds_total{mode="idle"})
```

`predict_linear` is a trend warning, not a guarantee. It is sensitive to the window and to bursts; pair it with a static capacity alert.

### Absence and target health

```promql
up == 0
absent(up{job="api"})
time() - max_over_time(timestamp(up{job="api"})[10m:]) > 600
```

`up == 0` means the scrape failed. `absent(up{job="api"})` means no series at all (target removed, label typo, Prometheus misconfigured). The third query approximates "no successful scrape in 10 minutes" but is subtle; prefer `up == 0` plus a target-list check, or a recording rule with a clear meaning.

### Useful patterns

```promql
# Requests per second by status class.
sum by (status) (rate(http_requests_total[5m]))

# p95 latency by route, only routes with traffic.
histogram_quantile(0.95,
  sum by (route, le) (rate(http_request_duration_seconds_bucket[5m]))
)

# Error ratio over 1h, with a zero fallback.
(
  sum(rate(http_requests_total{status=~"5.."}[1h]))
  /
  sum(rate(http_requests_total[1h]))
) or vector(0)

# Pod restarts in the last hour.
increase(kube_pod_container_status_restarts_total[1h])

# CPU throttling ratio.
sum by (pod) (rate(container_cpu_cfs_throttled_periods_total[5m]))
/
sum by (pod) (rate(container_cpu_cfs_periods_total[5m]))

# Node not ready (kube-state-metrics).
kube_node_status_condition{condition="Ready",status="true"} == 0
```

### Recording rules

A recording rule precomputes an expensive or reused expression:

```yaml
groups:
  - name: slo.rules
    interval: 30s
    rules:
      - record: job:http_requests:rate5m
        expr: sum by (job) (rate(http_requests_total[5m]))

      - record: job:http_errors:ratio5m
        expr: |
          sum by (job) (rate(http_requests_total{status=~"5.."}[5m]))
          /
          sum by (job) (rate(http_requests_total[5m]))

      - record: job:http_request_duration_seconds:p95
        expr: |
          histogram_quantile(0.95,
            sum by (job, le) (rate(http_request_duration_seconds_bucket[5m]))
          )
```

Naming convention: `level:metric:operation` (for example `job:http_errors:ratio5m`). Recording rules reduce query cost and make dashboards consistent, but they also freeze the window and filters; document them.

---

## 5. Scrape configuration

`prometheus.yml`:

```yaml
global:
  scrape_interval: 15s
  scrape_timeout: 10s
  evaluation_interval: 15s
  external_labels:
    cluster: opslab
    environment: lab

rule_files:
  - /etc/prometheus/rules/*.yml

alerting:
  alertmanagers:
    - static_configs:
        - targets: ["alertmanager:9093"]

scrape_configs:
  - job_name: prometheus
    static_configs:
      - targets: ["localhost:9090"]

  - job_name: node
    static_configs:
      - targets: ["node-exporter:9100"]

  - job_name: api
    metrics_path: /metrics
    static_configs:
      - targets: ["api:8080"]
    relabel_configs:
      - target_label: service
        replacement: capacity-api
```

- `scrape_timeout` must be less than `scrape_interval`.
- `evaluation_interval` controls rule evaluation, not scraping.
- `external_labels` are attached to remote-write/federation data, not necessarily to local queries.
- Static configs are fine for a lab; use file_sd, DNS SD, Kubernetes SD, or cloud SD at scale.
- `honor_labels: false` (default) renames conflicting labels; `honor_labels: true` keeps target labels and can create duplicates. Know which you need.

### Service discovery and relabeling

Kubernetes SD example:

```yaml
  - job_name: kubernetes-pods
    kubernetes_sd_configs:
      - role: pod
    relabel_configs:
      - source_labels: [__meta_kubernetes_pod_annotation_prometheus_io_scrape]
        action: keep
        regex: "true"
      - source_labels: [__meta_kubernetes_pod_annotation_prometheus_io_port]
        action: replace
        target_label: __address__
        regex: (.+)
        replacement: ${1}
      - source_labels: [__meta_kubernetes_namespace]
        target_label: namespace
      - source_labels: [__meta_kubernetes_pod_name]
        target_label: pod
```

Relabeling is powerful and easy to break. Inspect the target list and `__meta_*` labels before assuming the query is wrong.

### Validate and reload

```bash
promtool check config /etc/prometheus/prometheus.yml
promtool check rules /etc/prometheus/rules/*.yml

# Only if Prometheus was started with --web.enable-lifecycle.
curl -X POST http://localhost:9090/-/reload

# Readiness/health.
curl -fsS http://localhost:9090/-/ready
curl -fsS http://localhost:9090/-/healthy
```

Expected `promtool` output:

```text
Checking /etc/prometheus/prometheus.yml
  SUCCESS: 1 rule files found
SUCCESS: 1 rule files found
```

A config reload failure leaves the old config running; check the Prometheus log, not just the HTTP response.

---

## 6. Querying the API and debugging targets

```bash
curl -s 'http://localhost:9090/api/v1/query?query=up' | jq
curl -s 'http://localhost:9090/api/v1/query?query=rate(node_cpu_seconds_total%7Bmode%3D%22idle%22%7D%5B5m%5D)' | jq
curl -s 'http://localhost:9090/api/v1/targets' \
  | jq '.data.activeTargets[] | {scrapeUrl, health, lastError, lastScrape}'
curl -s 'http://localhost:9090/api/v1/rules' | jq '.data.groups[].rules[] | {name, state, health}'
```

Representative query response:

```json
{
  "status": "success",
  "data": {
    "resultType": "vector",
    "result": [
      {
        "metric": {"__name__": "up", "instance": "node-exporter:9100", "job": "node"},
        "value": [1729959121.234, "1"]
      }
    ]
  }
}
```

`value` is `[unix_timestamp, "sample"]`; samples are strings to preserve precision. `resultType` can be `vector`, `matrix`, `scalar`, or `string`.

### Target page symptoms

| Field | Meaning | First check |
|---|---|---|
| `health: up` | Last scrape succeeded | Nothing; query data |
| `health: down` | Last scrape failed | `lastError`, network, port, path, TLS |
| `lastError: connection refused` | Nothing listening | `ss -lntp`, process status |
| `lastError: context deadline exceeded` | Scrape timed out | Exporter latency, `scrape_timeout`, load |
| `lastError: 404` | Wrong `metrics_path` | App route/config |
| `lastError: 401/403` | Auth required | Scrape credentials/TLS config |
| `lastScrape` old | Prometheus stuck/restarted | Prometheus logs, target sync |

`up == 0` is a symptom of a failed scrape, not necessarily a failed service. A service can be healthy but unreachable from Prometheus (firewall, DNS, wrong port). Conversely, a service can be broken while `/metrics` still responds.

### PromQL debugging order

1. Query the raw metric by name with no functions: `http_requests_total`.
2. Add one label filter: `{job="api"}`.
3. Check the range vector has samples: `http_requests_total{job="api"}[5m]`.
4. Apply `rate()` and inspect per-series values.
5. Aggregate and compare with the raw per-series view.
6. Check the time range and step in Grafana; a 5m rate on a 30s range may show nothing.

If a query returns "no data," it is usually a label mismatch, a wrong metric name, a target that is not scraped, or a time range with no samples. Do not start by changing the query; start by proving the series exists.

---

## 7. Storage, retention, and scale

Prometheus stores samples in a local TSDB in blocks (default 2-hour blocks), with compaction and a WAL. Retention is time- or size-based:

```bash
prometheus \
  --config.file=/etc/prometheus/prometheus.yml \
  --storage.tsdb.path=/var/lib/prometheus \
  --storage.tsdb.retention.time=15d \
  --storage.tsdb.retention.size=50GB \
  --web.enable-lifecycle
```

- Default retention is 15 days; size retention is a safety cap.
- Disk usage ≈ series × samples/sec × bytes/sample × retention. A rough planning number is 1–2 bytes per sample compressed, but it varies with churn; measure with `promtool tsdb analyze`.
- High cardinality causes memory pressure, slow queries, and compaction issues. It is the most common Prometheus failure mode.
- Prometheus is single-node; it does not replicate. For HA, run two independent servers with the same config and deduplicate at query time (Thanos/Mimir) or accept a gap.
- Remote write ships samples to a long-term store; it is not a backup of the local TSDB. Plan retention and replay.

```bash
promtool tsdb analyze /var/lib/prometheus
du -sh /var/lib/prometheus
```

`tsdb analyze` shows series counts, label cardinality, and chunk usage. Use it before guessing why Prometheus is using memory.

---

## 8. Logs and traces, briefly

### Structured logs

```json
{"ts":"2026-10-27T09:14:22.183Z","level":"error","service":"capacity-api","version":"1.4.2","request_id":"req-7f3c","trace_id":"4bf92f...","route":"/capacity","status":500,"duration_ms":812,"error":"upstream timeout","upstream":"inventory"}
```

- UTC timestamps, ISO 8601.
- One JSON object per line; no multi-line stack traces unless wrapped.
- Include `service`, `version`, `environment`, `request_id`, `trace_id`, `route`, `status`, `duration_ms`.
- Do not log tokens, passwords, full request bodies, or personal data.
- Use log levels consistently: `error` means a user-visible failure or an unexpected condition; `warn` means degraded but handled; `info` means a state change; `debug` is off in production by default.

### Traces

A trace is a tree of spans sharing a trace ID. Each span has a name, start/end time, attributes, and parent. Instrument at boundaries: HTTP handlers, database calls, cache calls, queue publishes, external APIs. Propagate context (`traceparent`) across services.

Sampling:

- Head-based: decide at the start; cheap, but may miss rare errors.
- Tail-based: decide after the trace completes; catches errors/slow traces, but needs a collector and buffering.
- Always sample errors and slow requests if you can; do not sample only 1% of everything and then wonder why the failing trace is missing.

Traces answer "where did the time go?" Metrics answer "how often and how bad?" Logs answer "what exactly was the error?" Use them together.

---

## 9. Failure scenarios

### A — Target is down but the service is up

**Evidence:** `up{job="api"} == 0`; target `lastError`; `curl` from the Prometheus container to the target; `ss -lntp` on the target.

Common causes: wrong port, app bound to `127.0.0.1` inside a container, firewall/security group, DNS, TLS, metrics path, scrape timeout. The service may be serving users while `/metrics` is unreachable.

### B — Query returns no data

**Evidence:** raw metric name, label values, target list, time range, Grafana query inspector.

Common causes: metric not registered, wrong label (`status="500"` vs `status_code="500"`), target not scraped, typo in the metric name, query time range before the series existed, or a `rate()` window shorter than the scrape interval.

### C — Counter graph drops to zero

**Evidence:** process restart time, `process_start_time_seconds`, logs, `resets()`.

A counter reset is normal on restart. `rate()` handles it. Do not sum raw counters across restarts; use `increase()`/`rate()` or record a baseline.

### D — CPU query disagrees with `top`

**Evidence:** `node_cpu_seconds_total` by mode; `top`/`mpstat`; container CPU limits; cgroup throttling.

`node_exporter` reports host CPU; `top` may show a single process or a container namespace. Container CPU is limited by cgroups and can be throttled even when host CPU is idle. Compare the same scope (host vs container) and the same interval.

### E — p95 latency is nonsense

**Evidence:** histogram buckets, `le` aggregation, bucket boundaries, request count.

Common causes: missing `by (le)`, averaging precomputed quantiles, too-coarse buckets, mixing routes with different latency profiles, or a low request count. A p95 over 10 requests is not a stable number; show the request count with the percentile.

### F — Cardinality explosion

**Evidence:** `promtool tsdb analyze`, `/api/v1/status/tsdb`, top label values, recent deploys.

Common causes: raw URL paths, user IDs, error strings, per-request IDs in labels, dynamic cloud resource IDs. Fix the instrumentation; deleting series or increasing memory treats the symptom.

### G — Scrape timeouts

**Evidence:** target `lastError: context deadline exceeded`, exporter latency, `scrape_duration_seconds`, host load.

Common causes: slow exporter (blocking API call, huge metric set), network latency, too many targets per Prometheus, scrape timeout too low. Fix the exporter or shard the scrape load; do not set the interval below what the target can handle.

### H — Duplicate or missing series after relabeling

**Evidence:** target labels, `__address__`, `honor_labels`, `up` by instance, `count by (__name__)`.

Relabeling can collapse two targets into the same label set (last write wins) or drop the wrong label. Inspect the target list before and after the change.

### I — Dashboard shows a different value than the alert

**Evidence:** query text, time range, step, recording rule, `for:` duration, aggregation labels.

Grafana and alert rules can use different windows, filters, or recording rules. Put the query in one place (recording rule) or document the difference. A dashboard that uses `rate(...[5m])` and an alert that uses `[...1m]` will disagree during a spike.

### J — Clock skew

**Evidence:** `timedatectl`, NTP/chrony status, sample timestamps, log timestamps.

Scraped samples get the scrape time; client timestamps are ignored unless push. Skewed host clocks make logs and traces hard to correlate and can break TLS/JWT validation. Run NTP everywhere.

---

## 10. Command field manual

### `promtool`

```bash
promtool check config /etc/prometheus/prometheus.yml
promtool check rules /etc/prometheus/rules/*.yml
promtool query instant http://localhost:9090 'up'
promtool query range http://localhost:9090 'rate(http_requests_total[5m])' \
  --start=2026-10-27T09:00:00Z --end=2026-10-27T09:10:00Z --step=30s
promtool tsdb analyze /var/lib/prometheus
promtool test rules tests/rules.test.yml
```

**Why:** validate config/rules before reload, query without the UI, inspect TSDB cardinality, and unit-test alert rules with synthetic series. `promtool test rules` is the only way to prove an alert fires at the right threshold without waiting for production.

### Prometheus HTTP API

```bash
curl -fsS http://localhost:9090/-/healthy
curl -fsS http://localhost:9090/-/ready
curl -s 'http://localhost:9090/api/v1/targets' | jq '.data.activeTargets[] | {scrapeUrl, health, lastError}'
curl -s 'http://localhost:9090/api/v1/query?query=up' | jq '.data.result'
curl -s 'http://localhost:9090/api/v1/labels' | jq '.data'
curl -s 'http://localhost:9090/api/v1/metadata?metric=http_requests_total' | jq
curl -X POST http://localhost:9090/-/reload
```

**Interpretation:** `/-/ready` returns 200 when Prometheus is ready to serve; `/-/healthy` returns 200 when it is running (not necessarily ingesting). The API is the same one Grafana uses; if the API query is empty, the dashboard will be empty too.

### `node_exporter`

```bash
curl -s http://localhost:9100/metrics | head -40
curl -s http://localhost:9100/metrics | grep '^node_cpu_seconds_total' | head
curl -s http://localhost:9100/metrics | grep '^node_filesystem_avail_bytes'
```

**Why:** prove the exporter is alive and the metric names/labels match your queries. `node_exporter` exposes host metrics; it does not know your application.

### `systemctl` / `journalctl`

```bash
systemctl status prometheus --no-pager
journalctl -u prometheus -n 100 --no-pager
journalctl -u prometheus --since '10 min ago' | grep -i -E 'error|fail|reload'
```

**Why:** a failed config reload or a TSDB error appears in the service log, not in the HTTP API. `--no-pager` keeps output copy-pasteable.

### Docker Compose

```bash
docker compose ps
docker compose logs -n 100 prometheus
docker compose exec prometheus promtool check config /etc/prometheus/prometheus.yml
docker compose restart prometheus
```

**Why:** in the lab stack, the config is mounted into the container; validate it inside the container with the same path Prometheus uses. `docker compose restart` is not a config reload; it restarts the process.

---

## 11. Design checklist

- [ ] Every service exposes `/metrics` on a stable port and path.
- [ ] Metrics have units and bounded labels; no raw paths/IDs.
- [ ] A `up`/target alert exists for every critical scrape job.
- [ ] Dashboards start with user impact (traffic, errors, latency), then resources.
- [ ] Alerts are symptom-based, owned, severity-tagged, and link a runbook.
- [ ] SLOs have a numerator, denominator, window, and owner.
- [ ] Logs are structured and correlated by request/trace ID.
- [ ] Traces sample errors/slow requests, not just 1% of everything.
- [ ] Retention and cardinality are measured, not guessed.
- [ ] The alert path has been tested end to end (rule fires → Alertmanager → receiver).

---

## 12. Checkpoint

1. What question does each of the four signals answer?
2. Why does Prometheus pull instead of requiring apps to push?
3. When is Pushgateway appropriate, and when is it an anti-pattern?
4. What is the difference between a counter and a gauge? Give one query for each.
5. Why does `rate()` handle counter resets?
6. Why is `irate()` a poor choice for a paging alert?
7. What is the minimum useful window for `rate()` given a 15s scrape interval?
8. Why does `histogram_quantile` need `by (le)`?
9. What is cardinality, and how do you estimate it?
10. Name three labels that should never be unbounded.
11. What does `up == 0` prove? What does it not prove?
12. Why can a healthy service have `up == 0`?
13. What is the difference between `sum by (route)` and `sum without (instance)`?
14. Why is an error ratio better than an error count?
15. What does `predict_linear` warn about, and why is it not a guarantee?
16. What does `promtool check config` validate? What does it not validate?
17. Why can a config reload fail while Prometheus keeps serving?
18. What is the difference between local TSDB retention and remote write?
19. Why is high cardinality more dangerous than high sample volume?
20. What should a structured log contain for a failed request?
21. What is the difference between head-based and tail-based trace sampling?
22. Why is "CPU is high" a poor page?
23. What should every alert link to?
24. What is the first thing you check when a PromQL query returns no data?
25. What would you measure to prove the observability stack itself is healthy?

Then do [Lab 8.1](07-labs-part1.md), where you run the stack, instrument a service, and write the queries.
