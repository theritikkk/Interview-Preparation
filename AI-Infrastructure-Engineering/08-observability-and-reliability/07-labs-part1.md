# Phase 8 Labs — Part 1 (L8.1–L8.2)

> **Safety:** both labs bind services to localhost or a private lab network. L8.2 sends alerts to a local webhook, not to email, Slack, or a real on-call channel. Do not point the load generator at a service you do not own; a load test against a third party is a denial-of-service attack.
>
> **Environment:** a Linux VM with Docker Engine and Compose v2 is the primary path. If Docker is unavailable, use the local-binary appendix at the end of L8.1 and L8.2. macOS can run the binaries, but Linux gives you the full `node_exporter` metric set. Commands assume Bash; on macOS, `date -v` differs from GNU `date -d` and the labs call that out.

---

# Lab L8.1 — Metrics, PromQL, and the Shape of a Signal

## Objective

Run a small observability stack, instrument a service, and answer operational questions with PromQL rather than guesses. By the end you can:

1. Explain why Prometheus pulls, what a target is, and what `up` actually means.
2. Read the four metric types and write correct `rate`/`increase` queries.
3. Compute error ratio, p95 latency, and the fraction of requests under an SLO threshold.
4. Explain bucket layout, counter resets, staleness, and why a query returns no data.
5. Add a recording rule and test it with `promtool`.
6. Diagnose a broken scrape from evidence, not by restarting things.

The app is deliberately small. The point is the **measurement contract**: what is counted, what is labeled, what the denominator is, and what the number cannot tell you.

## Prerequisites

- Linux VM with 4 vCPU, 8 GB RAM, 20 GB free disk; Docker Engine 24+ and Compose v2.
- `curl`, `jq`, `python3` (only if you use the local appendix).
- Ports `9090`, `9100`, `8000` free on the host.
- 60–90 minutes.

Check:

```bash
docker version --format '{{.Server.Version}}'
docker compose version
curl --version | head -1
jq --version
```

If `docker compose version` fails but `docker-compose` works, substitute `docker-compose` in the commands. Do not install a second Docker daemon just for this lab.

## Setup

Create the project:

```bash
mkdir -p ~/opslab/phase8/l8.1/{rules,tests,data}
cd ~/opslab/phase8/l8.1
```

Expected layout:

```text
l8.1/
├── compose.yaml
├── prometheus.yml
├── app.py
├── rules/
│   └── slo.yml
└── tests/
    └── slo_test.yml
```

### The sample application

Create `app.py`:

```python
#!/usr/bin/env python3
"""Tiny instrumented service for the observability lab.

No third-party packages: the Prometheus text exposition is written by hand so
you can see exactly what the scrape receives.
"""
import random
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

BUCKETS = [0.01, 0.025, 0.05, 0.1, 0.25, 0.3, 0.5, 1.0, 2.5]
LOCK = threading.Lock()
COUNTS = {}          # (route, status) -> count
HISTS = {}           # route -> {"buckets": [...], "sum": float, "count": int}
INFLIGHT = 0
START = time.time()
VERSION = "1.0.0"


def observe(route, status, seconds):
    with LOCK:
        COUNTS[(route, status)] = COUNTS.get((route, status), 0) + 1
        h = HISTS.setdefault(route, {"buckets": [0] * len(BUCKETS), "sum": 0.0, "count": 0})
        for i, b in enumerate(BUCKETS):
            if seconds <= b:
                h["buckets"][i] += 1
        h["sum"] += seconds
        h["count"] += 1


def render():
    lines = [
        "# HELP http_requests_total Total HTTP requests by route and status.",
        "# TYPE http_requests_total counter",
    ]
    with LOCK:
        for (route, status), count in sorted(COUNTS.items()):
            lines.append(f'http_requests_total{{route="{route}",status="{status}"}} {count}')

        lines += [
            "# HELP http_request_duration_seconds Request duration by route.",
            "# TYPE http_request_duration_seconds histogram",
        ]
        for route, h in sorted(HISTS.items()):
            for i, b in enumerate(BUCKETS):
                lines.append(
                    f'http_request_duration_seconds_bucket{{route="{route}",le="{b}"}} {h["buckets"][i]}'
                )
            lines.append(
                f'http_request_duration_seconds_bucket{{route="{route}",le="+Inf"}} {h["count"]}'
            )
            lines.append(f'http_request_duration_seconds_sum{{route="{route}"}} {h["sum"]}')
            lines.append(f'http_request_duration_seconds_count{{route="{route}"}} {h["count"]}')
        lines += [
            "# HELP app_inflight_requests Requests currently being served.",
            "# TYPE app_inflight_requests gauge",
            f"app_inflight_requests {INFLIGHT}",
            "# HELP app_uptime_seconds Seconds since process start.",
            "# TYPE app_uptime_seconds gauge",
            f"app_uptime_seconds {time.time() - START:.3f}",
            "# HELP app_build_info Build information.",
            "# TYPE app_build_info gauge",
            f'app_build_info{{version="{VERSION}"}} 1',
        ]
    return "\n".join(lines) + "\n"


class Handler(BaseHTTPRequestHandler):
    def log_message(self, fmt, *args):
        pass

    def _respond(self, code, body):
        data = body.encode()
        self.send_response(code)
        self.send_header("Content-Type", "text/plain")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def do_GET(self):
        global INFLIGHT
        route = self.path.split("?")[0]
        if route == "/metrics":
            self._respond(200, render())
            return
        if route not in ("/", "/work", "/fail", "/healthz"):
            route = "unknown"

        with LOCK:
            INFLIGHT += 1
        start = time.time()
        status = 404
        body = "not found\n"
        try:
            if route == "/healthz":
                status, body = 200, "ok\n"
            elif route == "/fail":
                time.sleep(0.02)
                status, body = 500, "internal error\n"
            elif route == "/work":
                time.sleep(random.choice([0.02, 0.04, 0.06, 0.09, 0.15, 0.25, 0.4, 0.7]))
                status, body = 200, "ok\n"
            elif route == "/":
                time.sleep(0.005)
                status, body = 200, "ok\n"
        finally:
            elapsed = time.time() - start
            with LOCK:
                INFLIGHT -= 1
            observe(route, status, elapsed)
        self._respond(status, body)


if __name__ == "__main__":
    server = ThreadingHTTPServer(("0.0.0.0", 8000), Handler)
    print("app listening on :8000", flush=True)
    server.serve_forever()
```

Why hand-written exposition? Because the scrape format is the contract. You will see every `# HELP`, `# TYPE`, bucket, and label. A client library hides this, which is good in production and unhelpful on the first day.

### Prometheus configuration

Create `prometheus.yml`:

```yaml
global:
  scrape_interval: 5s
  evaluation_interval: 5s
  external_labels:
    env: lab

rule_files:
  - /etc/prometheus/rules/*.yml

scrape_configs:
  - job_name: prometheus
    static_configs:
      - targets: ["localhost:9090"]

  - job_name: node
    static_configs:
      - targets: ["node-exporter:9100"]

  - job_name: app
    metrics_path: /metrics
    static_configs:
      - targets: ["app:8000"]
        labels:
          service: checkout
```

`scrape_interval: 5s` is aggressive for production but makes a lab observable in minutes. It also means a 1-minute `rate` has about 12 samples, which is enough to see the behavior without waiting an hour.

### The compose file

Create `compose.yaml`:

```yaml
services:
  app:
    image: python:3.12-slim
    command: ["python", "/app/app.py"]
    working_dir: /app
    volumes:
      - ./app.py:/app/app.py:ro
    ports:
      - "127.0.0.1:8000:8000"
    healthcheck:
      test: ["CMD", "python", "-c", "import urllib.request; urllib.request.urlopen('http://localhost:8000/healthz')"]
      interval: 10s
      timeout: 3s
      retries: 3

  prometheus:
    image: prom/prometheus:v2.55.1
    command:
      - --config.file=/etc/prometheus/prometheus.yml
      - --storage.tsdb.path=/prometheus
      - --storage.tsdb.retention.time=6h
      - --web.enable-lifecycle
    volumes:
      - ./prometheus.yml:/etc/prometheus/prometheus.yml:ro
      - ./rules:/etc/prometheus/rules:ro
      - promdata:/prometheus
    ports:
      - "127.0.0.1:9090:9090"
    depends_on:
      - app

  node-exporter:
    image: prom/node-exporter:v1.8.2
    args:
      - --path.rootfs=/host
    pid: host
    volumes:
      - /:/host:ro,rslave
    ports:
      - "127.0.0.1:9100:9100"

volumes:
  promdata:
```

Notes:

- Binding to `127.0.0.1` keeps the lab off your LAN. If you run the VM headless and browse from the host, use `0.0.0.0:9090` and a host firewall rule, or an SSH tunnel: `ssh -L 9090:127.0.0.1:9090 user@vm`.
- `node-exporter` with `pid: host` and `/:/host:ro` gives host metrics. The `:ro` is important; a compromised exporter should not be able to write the host filesystem.
- The retention is 6 hours. Prometheus is not a long-term store; do not use this stack for capacity history.

Start:

```bash
docker compose up -d
docker compose ps
```

Expected:

```text
NAME                  SERVICE         STATUS              PORTS
l81-app-1             app             running (healthy)   127.0.0.1:8000->8000/tcp
l81-prometheus-1      prometheus      running             127.0.0.1:9090->9090/tcp
l81-node-exporter-1   node-exporter   running             127.0.0.1:9100->9100/tcp
```

Verify the app directly:

```bash
curl -s http://127.0.0.1:8000/metrics | head -20
curl -s -o /dev/null -w '%{http_code}\n' http://127.0.0.1:8000/work
```

Representative metrics before traffic:

```text
# HELP http_requests_total Total HTTP requests by route and status.
# TYPE http_requests_total counter
# HELP http_request_duration_seconds Request duration by route.
# TYPE http_request_duration_seconds histogram
...
app_inflight_requests 0
app_uptime_seconds 12.431
app_build_info{version="1.0.0"} 1
```

No `http_requests_total` series exist until the first non-`/metrics` request. That is normal: a counter with no observations has no series. A dashboard that expects a zero will show "no data."

Check targets:

```bash
curl -s http://127.0.0.1:9090/api/v1/targets | jq -r '.data.activeTargets[] | [.labels.job, .scrapeUrl, .health, .lastError] | @tsv'
```

Expected:

```text
prometheus  http://localhost:9090/metrics  up
node        http://node-exporter:9100/metrics  up
app         http://app:8000/metrics  up
```

If `app` is `down`, stop and diagnose before continuing. The `lastError` field is the first evidence, not a reason to restart.

## Part A — Counters, gauges, and the meaning of `up`

Generate a little traffic:

```bash
for i in $(seq 1 100); do curl -s -o /dev/null http://127.0.0.1:8000/work; done
for i in $(seq 1 20); do curl -s -o /dev/null http://127.0.0.1:8000/fail; done
```

If `hey` is installed, use it instead so the load is concurrent:

```bash
hey -z 30s -c 20 http://127.0.0.1:8000/work
hey -z 10s -c 5 http://127.0.0.1:8000/fail
```

Query in the Prometheus UI (`http://127.0.0.1:9090`) or via the API:

```promql
up
```

Expected:

```text
up{job="app",instance="app:8000"} 1
up{job="node",instance="node-exporter:9100"} 1
up{job="prometheus",instance="localhost:9090"} 1
```

`up` is generated by Prometheus, not the target. It is 1 if the last scrape succeeded, 0 if it failed. It says nothing about whether the application is serving users correctly: a process that returns `500` to every request is still `up`.

```promql
http_requests_total{job="app"}
```

Expected (labels vary):

```text
http_requests_total{job="app",instance="app:8000",route="/work",service="checkout",status="200"} 100
http_requests_total{job="app",instance="app:8000",route="/fail",service="checkout",status="500"} 20
```

`http_requests_total` is a counter: it only goes up (except on process restart). A raw counter graph is almost useless for rate; it shows uptime, not traffic.

```promql
rate(http_requests_total{job="app"}[1m])
```

Expected after steady traffic:

```text
{route="/work",status="200",...} 3.2
{route="/fail",status="500",...} 0.6
```

`rate()` returns per-second averages over the window. With 5-second scrapes, a 1-minute window has ~12 samples. The result is smoothed: a 10-second burst is spread across the minute.

Compare:

```promql
irate(http_requests_total{job="app"}[1m])
```

`irate()` uses only the last two samples. It reacts faster and is noisier; it is useful for a "what is happening right now" panel, not for an SLO or an alert. If the last two samples straddle a counter reset, `irate` can produce a misleading spike; `rate` handles resets across the whole window.

```promql
increase(http_requests_total{job="app"}[5m])
```

`increase` is `rate × window seconds`, approximately the number of requests in the window. It extrapolates to the window boundaries, so it can return a non-integer estimate even though the underlying counter is integer.

### Gauges

```promql
app_inflight_requests
node_memory_MemAvailable_bytes
node_filesystem_avail_bytes{job="node",mountpoint="/"}
node_load1
```

Gauges can go up and down. Do not apply `rate()` to a gauge; use `delta()`, `deriv()`, or compare to a threshold. `rate(node_memory_MemAvailable_bytes[5m])` is a unit error (bytes per second), not a memory trend.

```promql
node_filesystem_avail_bytes{job="node",mountpoint="/"} / node_filesystem_size_bytes{job="node",mountpoint="/"}
```

This is the fraction free. On macOS with a local `node_exporter`, the mountpoint may be `/` but the container/host labels differ; inspect `node_filesystem_*` first.

### Counter resets

Restart the app and query:

```bash
docker compose restart app
sleep 15
curl -s 'http://127.0.0.1:9090/api/v1/query' \
  --data-urlencode 'query=resets(http_requests_total{job="app"}[10m])' \
  | jq -r '.data.result[] | [.metric.route, .metric.status, .value[1]] | @tsv'
```

Expected: one or more `1` values, because the counter reset when the process restarted.

Prometheus detects a reset when the value decreases and treats the post-reset value as an increase. That is why `rate` remains usable across restarts, but the **total** is not the same as "requests since the beginning of time." If you need a lifetime total, persist it or use a durable event store.

## Part B — Error ratio and latency

### Error ratio

```promql
sum(rate(http_requests_total{job="app",status=~"5.."}[5m]))
/
sum(rate(http_requests_total{job="app"}[5m]))
```

Expected with the load above:

```text
0.1666...
```

Approximately 20 failures out of 120 requests, or a 16.7% error ratio. The exact value depends on timing and retries.

Why the `sum` on both sides? Without it, you get one ratio per route/status label combination, and the denominator may be a single series. The SLO ratio must aggregate the numerator and denominator over the same population.

A common mistake:

```promql
sum(rate(http_requests_total{status=~"5.."}[5m]))
/
sum(rate(http_requests_total[5m]))
```

This includes every job, including Prometheus and node-exporter, in the denominator. The error ratio is diluted by unrelated traffic. Always constrain the denominator to the same service/job as the numerator.

### p95 latency

```promql
histogram_quantile(
  0.95,
  sum by (le) (rate(http_request_duration_seconds_bucket{job="app",route="/work"}[5m]))
)
```

Expected:

```text
0.4
```

The value is an estimate from the bucket boundaries, not an exact percentile. With buckets at `0.25, 0.3, 0.5`, a p95 in the 0.3–0.5 bucket is interpolated somewhere in that range. If the true p95 is 0.31, the estimate may be 0.33; if the distribution is concentrated at the bucket edge, the error can be larger.

### Fraction under the SLO threshold

```promql
sum(rate(http_request_duration_seconds_bucket{job="app",route="/work",le="0.3"}[5m]))
/
sum(rate(http_request_duration_seconds_count{job="app",route="/work"}[5m]))
```

Expected:

```text
0.6
```

This is the SLI: the proportion of requests served within 300 ms. The p95 tells you the tail; the ratio tells you how much of the population is inside the promise. For an SLO, prefer the ratio.

Why `_bucket` for the numerator and `_count` for the denominator? The `le="0.3"` bucket is cumulative: it counts all requests `<= 0.3`. `_count` is the total. Their ratio is the fraction good.

### Average latency (and why it lies)

```promql
sum(rate(http_request_duration_seconds_sum{job="app",route="/work"}[5m]))
/
sum(rate(http_request_duration_seconds_count{job="app",route="/work"}[5m]))
```

Expected:

```text
0.18
```

The average is below 300 ms, but 40% of requests may be above it. This is the exact reason an average-based SLO hides user pain. Keep the average for cost/capacity, not for the promise.

### Bucket layout

List the buckets:

```bash
curl -s http://127.0.0.1:8000/metrics | grep 'duration_seconds_bucket.*route="/work"'
```

Expected:

```text
http_request_duration_seconds_bucket{route="/work",le="0.01"} 0
http_request_duration_seconds_bucket{route="/work",le="0.025"} 0
http_request_duration_seconds_bucket{route="/work",le="0.05"} 2
...
http_request_duration_seconds_bucket{route="/work",le="2.5"} 100
http_request_duration_seconds_bucket{route="/work",le="+Inf"} 100
```

- Buckets are cumulative. Each `le` includes everything below it.
- The `+Inf` bucket equals the total count.
- Bucket boundaries should cover the SLO threshold. A 300 ms SLO with buckets at 100 ms and 1 s cannot tell you the fraction under 300 ms without interpolation.
- Too many buckets increase series count; too few make percentiles meaningless. Put boundaries around the thresholds you actually alert on.

### Low-traffic percentile noise

Stop traffic for a few minutes, send 10 requests, and query p95. It will often be 0 or jump between buckets. A p95 estimated from 10 samples is the 9.5th request; it is not a stable operational signal. Use a longer window or a ratio with a minimum request count.

## Part C — Recording rules and rule tests

Create `rules/slo.yml`:

```yaml
groups:
  - name: app-slo
    interval: 5s
    rules:
      - record: job:http_requests:rate5m
        expr: sum by (job, service) (rate(http_requests_total[5m]))

      - record: job:http_errors:ratio5m
        expr: |
          sum by (job, service) (rate(http_requests_total{status=~"5.."}[5m]))
          /
          sum by (job, service) (rate(http_requests_total[5m]))

      - record: job:http_latency:good_ratio5m
        expr: |
          sum by (job, service) (rate(http_request_duration_seconds_bucket{le="0.3"}[5m]))
          /
          sum by (job, service) (rate(http_request_duration_seconds_count[5m]))
```

Reload:

```bash
curl -s -X POST http://127.0.0.1:9090/-/reload
curl -s http://127.0.0.1:9090/api/v1/rules | jq -r '.data.groups[].rules[] | [.name, .health, .lastError] | @tsv'
```

Expected:

```text
job:http_requests:rate5m  ok
job:http_errors:ratio5m   ok
job:http_latency:good_ratio5m  ok
```

Query the recorded series:

```promql
job:http_errors:ratio5m{job="app"}
```

Recording rules are not just for performance. They give a query a **name**, which makes dashboards and alerts easier to review and keeps the definition in one place. If five dashboards each compute error ratio differently, you have five definitions of "healthy."

### Test the rule without a live service

Create `tests/slo_test.yml`:

```yaml
rule_files:
  - ../rules/slo.yml

evaluation_interval: 1m

tests:
  - interval: 1m
    input_series:
      - series: 'http_requests_total{job="app",service="checkout",status="200"}'
        values: '0+60x10'
      - series: 'http_requests_total{job="app",service="checkout",status="500"}'
        values: '0+6x10'
    promql_expr_test:
      - expr: job:http_errors:ratio5m
        eval_time: 10m
        exp_samples:
          - labels: 'job:http_errors:ratio5m{job="app",service="checkout"}'
            value: 0.1
```

Run:

```bash
promtool check rules rules/slo.yml
promtool test rules tests/slo_test.yml
```

If `promtool` is not installed on the host, use the container:

```bash
docker run --rm -v "$PWD/rules:/rules:ro" -v "$PWD/tests:/tests:ro" \
  --entrypoint promtool prom/prometheus:v2.55.1 \
  check rules /rules/slo.yml
```

Expected:

```text
Checking rules/slo.yml
  SUCCESS: 3 rules found

Unit Testing: tests/slo_test.yml
  SUCCESS
```

Why test a rule? Because the alert path is code, and code has edge cases. A test can assert that a 10% error ratio fires and a 0.05% error ratio does not, at a specific evaluation time, without waiting for a real incident.

## Part D — Break it on purpose

Do not read the analysis until you have collected evidence in the write-up.

### Fault 1 — The scrape target is down

```bash
docker compose stop app
sleep 20
curl -s http://127.0.0.1:9090/api/v1/targets | jq -r '.data.activeTargets[] | select(.labels.job=="app") | [.health, .lastError, .lastScrape] | @tsv'
curl -s 'http://127.0.0.1:9090/api/v1/query' --data-urlencode 'query=up{job="app"}' | jq
```

**Your task:** explain what `up` returns, what happens to `rate(http_requests_total{job="app"}[5m])`, and why a dashboard may show a flat line instead of zero. Then restart the app and explain why the error ratio may briefly be misleading.

<details>
<summary>After you have written your evidence</summary>

`up{job="app"}` returns `0` while the target is down. The last successful sample remains in the TSDB until it becomes stale (about 5 minutes by default). During that window, `rate()` over a range that includes the last samples can still return a value, then the series disappears. A dashboard may show a gap or the last value, depending on panel settings (`Connect null values`, `No value`). It will not automatically show `0`; "no data" and "zero" are different states.

After restart, the counter resets. `rate()` handles the reset, but the first window after restart may mix pre-restart and post-restart samples and produce a lower or noisy rate. Wait one full window before trusting the ratio. The absence of series is why you need an explicit `absent(up{job="app"})` or `up == 0` alert, not just an error-ratio alert.

</details>

### Fault 2 — The query returns `NaN`

```bash
docker compose stop app
sleep 6m
curl -s 'http://127.0.0.1:9090/api/v1/query' \
  --data-urlencode 'query=sum(rate(http_requests_total{job="app",status=~"5.."}[5m])) / sum(rate(http_requests_total{job="app"}[5m]))' | jq
```

**Your task:** why is the result `NaN` or empty rather than `0`? What would you change so an idle service does not look healthy or broken by accident?

<details>
<summary>After you have written your evidence</summary>

The denominator has no samples after staleness, so the division has no defined value. Prometheus returns an empty result for a vector with no series; if both sides exist but the denominator is zero, you can get `NaN`/`+Inf`. Neither means "0% errors." For an SLO, add a minimum-traffic condition and an absence alert:

```promql
(
  sum(rate(http_requests_total{job="app",status=~"5.."}[5m]))
  /
  sum(rate(http_requests_total{job="app"}[5m]))
)
and
sum(rate(http_requests_total{job="app"}[5m])) > 0.1
```

Plus a separate alert for `up == 0` or `absent(up{job="app"})`. Do not coerce missing data to zero; that hides an outage.

</details>

### Fault 3 — p95 looks impossible

Generate 5 requests with a long pause, then query p95 over 1 minute:

```bash
docker compose start app
for i in 1 2 3 4 5; do curl -s -o /dev/null http://127.0.0.1:8000/work; sleep 10; done
```

Query:

```promql
histogram_quantile(0.95, sum by (le) (rate(http_request_duration_seconds_bucket{job="app",route="/work"}[1m])))
```

**Your task:** is the result trustworthy? What evidence would you collect before putting it on a dashboard or using it in an alert?

<details>
<summary>After you have written your evidence</summary>

With ~5 samples, the 95th percentile is essentially the maximum, and bucket interpolation can return a value at a bucket boundary (for example, exactly `0.3` or `0.5`). It is not a stable estimate. Check `sum(rate(..._count[1m]))` for the sample count, use a longer window, and consider a ratio SLI instead of a percentile for low-traffic services. A percentile without a sample count is a number without context.

</details>

### Fault 4 — Cardinality

Query:

```promql
count by (__name__) ({__name__=~"http_.*"})
prometheus_tsdb_head_series
```

Then deliberately add a high-cardinality label to the app (for example, include a request ID in `route`) **in a copy**, not in the running lab, and reason about what would happen. Do not actually ship unbounded labels to the shared stack.

**Your task:** which labels in this lab are bounded, and which would explode if you added user input?

<details>
<summary>After you have written your evidence</summary>

`route`, `status`, `job`, `instance`, and `service` are bounded. A raw path like `/users/12345`, a request ID, a session ID, an email, or an error message is unbounded. Each unique combination creates a new series, and each series costs memory, disk, and query time. Put high-cardinality data in logs or traces, not metric labels. If you need per-route detail, normalize the route (`/users/:id`) before labeling.

</details>

## Expected observations

After Parts A–C, you should see:

| Observation | Expected shape | Why |
|---|---|---|
| `up{job="app"}` | 1 while running, 0 after stop | Scrape success, not application correctness |
| `http_requests_total` | monotonically increasing per route/status | Counters |
| `rate(...[1m])` | per-second rate, smooth | Averaged over the window |
| `increase(...[5m])` | approximate count | Extrapolated rate × time |
| Error ratio | ~0.15–0.2 with `/fail` load | Numerator/denominator aligned |
| p95 | ~0.3–0.5 s | Bucket interpolation |
| Good ratio at 300 ms | ~0.6 | SLI as a ratio |
| Average | lower than p95 | The tail is hidden by the mean |
| Recording rule | `ok` health | Valid expression |
| `promtool` test | `SUCCESS` | Rule behaves on synthetic series |
| After `stop app` | `up=0`, series go stale, ratio disappears | Missing data is not zero |

## Internal explanation

### Pull, targets, and `up`

Prometheus discovers targets (static config or service discovery), scrapes an HTTP endpoint on an interval, and stores samples with the target's labels. The pull model makes target health observable: if the scrape fails, Prometheus knows. It also means the monitoring system needs network reachability to every target; a target behind a firewall is invisible unless you use a proxy or push gateway.

`up` is a synthetic series per target. `up == 0` means the last scrape failed, not that the service is down for users. A service can be `up` and return errors, or `down` while a load balancer serves from another instance. Alert on user symptoms and use `up` as supporting evidence.

### The TSDB and staleness

Prometheus writes samples to a write-ahead log and periodic blocks. It retains data for the configured time and deletes old blocks. It does not store one value per series per scrape forever; that is why long-term retention needs remote write or a different system.

When a target disappears, Prometheus marks the series stale after a lookback delta (default 5 minutes). Queries over a range that includes the last samples may still return values until staleness is applied. This is why a graph can show a line after the process died, then a gap. Do not interpret the last value as current health.

### Counters and `rate`

A counter is monotonic except at process restart. `rate()` computes the increase per second across the samples in the window and handles resets by treating a decrease as a new counter starting at zero. It extrapolates to the window boundaries, which is why the result may not match a hand count exactly.

With a 5-second scrape and a 1-minute window, there are ~12 samples. A 1-minute rate is smoother than `irate` but slower to react. For alerts, use a window long enough to be stable and short enough to catch the symptom; for SLOs, use the SLO window.

### Histograms

A histogram exposes cumulative buckets (`le`), a sum, and a count. `histogram_quantile(φ, sum by (le) (...))` estimates the φ quantile by finding the bucket that contains it and interpolating within the bucket, assuming a uniform distribution inside the bucket. The estimate is only as good as the bucket layout and the sample count.

For an SLO, the ratio `bucket{le="threshold"} / count` is exact with respect to the bucket boundary: it counts the requests that were observed at or below the threshold, assuming the threshold is a bucket boundary. If the threshold is not a boundary, you are interpolating and the ratio is an estimate. Add the boundary.

### Recording rules

A recording rule evaluates an expression on a schedule and writes the result as a new series. It reduces query cost, gives a stable name, and centralizes a definition. It also introduces a dependency: if the rule evaluation fails, the recorded series is stale even though the raw data is present. Check rule health, not just the dashboard.

### Cardinality

Every unique label combination is a series. Series count drives memory, index size, scrape cost, and query time. The dangerous labels are the ones that look useful in a debugging session: user ID, request ID, full URL, error text, container ID. Use logs/traces for those. Bound metric labels to a known set (route template, status class, region, version) and monitor `prometheus_tsdb_head_series` for unexpected growth.

## Troubleshooting task

A teammate reports: "The app dashboard is empty, but the app is running." You have five minutes and no permission to restart anything.

Collect, in order:

1. `curl -s http://127.0.0.1:9090/api/v1/targets | jq` — is the target up? What is `lastError`?
2. `curl -s http://127.0.0.1:8000/metrics | head` — does the app expose metrics from the host?
3. `docker compose exec prometheus wget -qO- http://app:8000/metrics | head` — can Prometheus reach the app by service name?
4. `curl -s http://127.0.0.1:9090/api/v1/query --data-urlencode 'query=up{job="app"}' | jq` — what does Prometheus think?
5. `docker compose logs --tail=50 app prometheus` — what changed?

Write a hypothesis and the evidence that would confirm or refute it before you change a config. Common causes: the app crashed after a code edit; the scrape target uses `localhost` inside the Prometheus container (which is Prometheus itself, not the app); the port mapping is wrong; the metrics path is different; a network policy blocks the scrape; the dashboard queries a different job/label.

<details>
<summary>After you have written your hypothesis</summary>

The most common lab cause is a target of `localhost:8000` in `prometheus.yml`. Inside the Prometheus container, `localhost` is the Prometheus container, not the app. The target must be `app:8000` on the compose network. The second most common is editing `app.py` and restarting only Prometheus; the app container still runs the old code. Check the container start time and the file mount.

Do not "fix" it by adding `network_mode: host` unless you understand the port and DNS implications. Fix the address.

</details>

## Verification

- [ ] All three targets are `up`.
- [ ] `http_requests_total` has bounded `route` and `status` labels.
- [ ] `rate()` and `increase()` return plausible values under load.
- [ ] Error ratio uses the same service in numerator and denominator.
- [ ] p95 is computed from aggregated buckets, not averaged per-instance percentiles.
- [ ] The 300 ms good ratio uses the `le="0.3"` bucket and `_count`.
- [ ] A counter reset is detected with `resets()` and does not break `rate()`.
- [ ] The recording rules are `ok` and the `promtool` test passes.
- [ ] You can explain why `up=1` does not mean the service is healthy.
- [ ] You can explain why missing data is not zero.

## Cleanup

```bash
docker compose down -v
```

`-v` removes the Prometheus volume. Without it, the next lab starts with old samples and confusing time ranges. Keep the files; L8.2 extends the stack.

If you used the local-binary path, stop the foreground processes with `Ctrl-C` and remove the data directory:

```bash
rm -rf ~/opslab/phase8/l8.1/data
```

## Write-up template

```markdown
# L8.1 — Metrics and PromQL

## Setup
- Environment (VM/macOS, Docker/binaries):
- Versions:
- Targets and health:

## Queries
| Question | Query | Result | Interpretation |
|---|---|---|---|
| Is the target up? | | | |
| Request rate by status | | | |
| Error ratio | | | |
| p95 latency | | | |
| Fraction under 300 ms | | | |
| Counter resets | | | |

## Fault 1 — target down
- Evidence:
- Why the graph did not show zero:
- What alert would have caught it:

## Fault 2 — NaN / no data
- Evidence:
- Root cause:
- Fix and test:

## Fault 3 — p95 with 5 samples
- Evidence:
- Why the number is unstable:
- Better SLI:

## Cardinality
- Bounded labels:
- Dangerous labels:
- Series count before/after:

## Rule test
- `promtool` output:
- What the test would catch:

## One thing I would change in production
```

---

# Lab L8.2 — Dashboards, Alerts, and a Human Who Can Act

## Objective

Turn the L8.1 metrics into a dashboard and an alert path that a tired human can use. By the end you can:

1. Provision a Grafana data source and dashboard from files.
2. Build a first row that answers "are users affected?" in under 30 seconds.
3. Write symptom-based Prometheus alert rules with `for`, severity, and a runbook.
4. Route, group, inhibit, and silence alerts through Alertmanager.
5. Deliver a real webhook notification to a local receiver and inspect the payload.
6. Diagnose an alert that fires but does not notify.

## Prerequisites

- L8.1 stack running and healthy.
- Docker Compose v2, `curl`, `jq`, `hey` (optional).
- A local webhook receiver (provided below).
- 75–120 minutes.

## Setup

Work in the same project, or copy the L8.1 files:

```bash
mkdir -p ~/opslab/phase8/l8.2/{rules,grafana/provisioning/datasources,grafana/provisioning/dashboards,grafana/dashboards}
cd ~/opslab/phase8/l8.2
```

If you are continuing from L8.1, copy the app, Prometheus config, and compose file, then extend them:

```bash
cp -r ~/opslab/phase8/l8.1/{app.py,prometheus.yml,compose.yaml,rules} .
mkdir -p grafana/provisioning/{datasources,dashboards} grafana/dashboards
```

Expected layout:

```text
l8.2/
├── compose.yaml
├── prometheus.yml
├── app.py
├── alertmanager.yml
├── webhook.py
├── rules/
│   ├── slo.yml
│   └── alerts.yml
├── tests/
│   └── alerts_test.yml
└── grafana/
    ├── provisioning/
    │   ├── datasources/prometheus.yml
    │   └── dashboards/default.yml
    └── dashboards/opslab-overview.json
```

### Add Grafana, Alertmanager, and a webhook

Create `webhook.py`:

```python
#!/usr/bin/env python3
"""Local Alertmanager receiver. Prints a compact summary and the full JSON."""
import json
from http.server import BaseHTTPRequestHandler, HTTPServer

class Handler(BaseHTTPRequestHandler):
    def log_message(self, fmt, *args):
        pass

    def do_POST(self):
        length = int(self.headers.get("Content-Length", "0"))
        body = self.rfile.read(length)
        try:
            payload = json.loads(body)
        except json.JSONDecodeError:
            print("invalid JSON", flush=True)
            self.send_response(400)
            self.end_headers()
            return

        print("=== alert batch ===", flush=True)
        print("status:", payload.get("status"), flush=True)
        for alert in payload.get("alerts", []):
            labels = alert.get("labels", {})
            print(
                f"{labels.get('alertname')} | {labels.get('severity')} | "
                f"{labels.get('job')} | {alert.get('status')} | "
                f"{labels.get('summary', '')}",
                flush=True,
            )
        print(json.dumps(payload, indent=2), flush=True)

        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.end_headers()
        self.wfile.write(b'{"ok":true}')

if __name__ == "__main__":
    print("webhook listening on :9000", flush=True)
    HTTPServer(("0.0.0.0", 9000), Handler).serve_forever()
```

Create `alertmanager.yml`:

```yaml
global:
  resolve_timeout: 5m

route:
  receiver: lab-webhook
  group_by: ["alertname", "severity"]
  group_wait: 10s
  group_interval: 30s
  repeat_interval: 4h
  routes:
    - matchers: ['severity="critical"']
      receiver: lab-webhook
      group_wait: 5s
      repeat_interval: 1h

receivers:
  - name: lab-webhook
    webhook_configs:
      - url: http://webhook:9000/alerts
        send_resolved: true

inhibit_rules:
  - source_matchers: ['severity="critical"']
    target_matchers: ['severity="warning"']
    equal: ["job"]
```

Why these values:

- `group_wait: 10s` gives related alerts a chance to batch before the first notification. In production it is often 30–60s; in the lab, 10s keeps you from waiting.
- `group_interval: 30s` controls updates to an existing group. Too short creates noise; too long delays new information.
- `repeat_interval: 4h` is the "still broken" reminder. A 5-minute repeat is a page storm.
- `send_resolved: true` closes the loop; a receiver that only sees firing alerts cannot tell when to stop.
- The inhibition rule suppresses warnings for the same `job` while a critical alert is active. It does not suppress a different job or an unrelated alert.

Create the Grafana data source `grafana/provisioning/datasources/prometheus.yml`:

```yaml
apiVersion: 1
datasources:
  - name: Prometheus
    uid: prometheus
    type: prometheus
    access: proxy
    url: http://prometheus:9090
    isDefault: true
    editable: false
    jsonData:
      timeInterval: 5s
```

Create the dashboard provider `grafana/provisioning/dashboards/default.yml`:

```yaml
apiVersion: 1
providers:
  - name: opslab
    orgId: 1
    folder: OpsLab
    type: file
    disableDeletion: false
    updateIntervalSeconds: 10
    allowUiUpdates: false
    options:
      path: /var/lib/grafana/dashboards
```

Create `grafana/dashboards/opslab-overview.json`:

```json
{
  "uid": "opslab-overview",
  "title": "OpsLab — Checkout Overview",
  "tags": ["opslab", "slo"],
  "timezone": "utc",
  "schemaVersion": 39,
  "refresh": "10s",
  "time": {"from": "now-30m", "to": "now"},
  "templating": {
    "list": [
      {
        "name": "route",
        "label": "Route",
        "type": "query",
        "datasource": {"type": "prometheus", "uid": "prometheus"},
        "query": "label_values(http_requests_total{job=\"app\"}, route)",
        "includeAll": true,
        "multi": true,
        "allValue": ".*",
        "current": {"text": "All", "value": "$__all"}
      }
    ]
  },
  "panels": [
    {
      "id": 1,
      "type": "stat",
      "title": "Availability (5m)",
      "gridPos": {"h": 6, "w": 6, "x": 0, "y": 0},
      "datasource": {"type": "prometheus", "uid": "prometheus"},
      "targets": [
        {
          "refId": "A",
          "expr": "sum(rate(http_requests_total{job=\"app\",route=~\"$route\",status!~\"5..\"}[5m])) / sum(rate(http_requests_total{job=\"app\",route=~\"$route\"}[5m]))",
          "legendFormat": "availability"
        }
      ],
      "fieldConfig": {
        "defaults": {
          "unit": "percentunit",
          "min": 0,
          "max": 1,
          "thresholds": {
            "mode": "absolute",
            "steps": [
              {"color": "red", "value": null},
              {"color": "orange", "value": 0.99},
              {"color": "green", "value": 0.999}
            ]
          }
        },
        "overrides": []
      },
      "options": {"reduceOptions": {"calcs": ["lastNotNull"], "fields": "", "values": false}}
    },
    {
      "id": 2,
      "type": "stat",
      "title": "Error ratio (5m)",
      "gridPos": {"h": 6, "w": 6, "x": 6, "y": 0},
      "datasource": {"type": "prometheus", "uid": "prometheus"},
      "targets": [
        {
          "refId": "A",
          "expr": "sum(rate(http_requests_total{job=\"app\",route=~\"$route\",status=~\"5..\"}[5m])) / sum(rate(http_requests_total{job=\"app\",route=~\"$route\"}[5m]))",
          "legendFormat": "error ratio"
        }
      ],
      "fieldConfig": {
        "defaults": {
          "unit": "percentunit",
          "min": 0,
          "thresholds": {
            "mode": "absolute",
            "steps": [
              {"color": "green", "value": null},
              {"color": "orange", "value": 0.001},
              {"color": "red", "value": 0.01}
            ]
          }
        },
        "overrides": []
      },
      "options": {"reduceOptions": {"calcs": ["lastNotNull"], "fields": "", "values": false}}
    },
    {
      "id": 3,
      "type": "stat",
      "title": "p95 latency (5m)",
      "gridPos": {"h": 6, "w": 6, "x": 12, "y": 0},
      "datasource": {"type": "prometheus", "uid": "prometheus"},
      "targets": [
        {
          "refId": "A",
          "expr": "histogram_quantile(0.95, sum by (le) (rate(http_request_duration_seconds_bucket{job=\"app\",route=~\"$route\"}[5m])))",
          "legendFormat": "p95"
        }
      ],
      "fieldConfig": {
        "defaults": {
          "unit": "s",
          "thresholds": {
            "mode": "absolute",
            "steps": [
              {"color": "green", "value": null},
              {"color": "orange", "value": 0.3},
              {"color": "red", "value": 1}
            ]
          }
        },
        "overrides": []
      },
      "options": {"reduceOptions": {"calcs": ["lastNotNull"], "fields": "", "values": false}}
    },
    {
      "id": 4,
      "type": "stat",
      "title": "Request rate",
      "gridPos": {"h": 6, "w": 6, "x": 18, "y": 0},
      "datasource": {"type": "prometheus", "uid": "prometheus"},
      "targets": [
        {
          "refId": "A",
          "expr": "sum(rate(http_requests_total{job=\"app\",route=~\"$route\"}[5m]))",
          "legendFormat": "req/s"
        }
      ],
      "fieldConfig": {"defaults": {"unit": "reqps"}, "overrides": []},
      "options": {"reduceOptions": {"calcs": ["lastNotNull"], "fields": "", "values": false}}
    },
    {
      "id": 5,
      "type": "timeseries",
      "title": "Requests by status",
      "gridPos": {"h": 8, "w": 12, "x": 0, "y": 6},
      "datasource": {"type": "prometheus", "uid": "prometheus"},
      "targets": [
        {
          "refId": "A",
          "expr": "sum by (status) (rate(http_requests_total{job=\"app\",route=~\"$route\"}[1m]))",
          "legendFormat": "{{status}}"
        }
      ],
      "fieldConfig": {"defaults": {"unit": "reqps"}, "overrides": []}
    },
    {
      "id": 6,
      "type": "timeseries",
      "title": "Latency p95 by route",
      "gridPos": {"h": 8, "w": 12, "x": 12, "y": 6},
      "datasource": {"type": "prometheus", "uid": "prometheus"},
      "targets": [
        {
          "refId": "A",
          "expr": "histogram_quantile(0.95, sum by (le, route) (rate(http_request_duration_seconds_bucket{job=\"app\",route=~\"$route\"}[5m])))",
          "legendFormat": "{{route}}"
        }
      ],
      "fieldConfig": {"defaults": {"unit": "s"}, "overrides": []}
    }
  ]
}
```

This JSON is deliberately small. It contains no `__inputs`, no data source IDs from another Grafana, and no hidden dashboard version. Import it, edit one panel, reload the page, and confirm the edit disappears because `allowUiUpdates: false`. That is provisioning working as intended.

### Alert rules

Create `rules/alerts.yml`:

```yaml
groups:
  - name: app-alerts
    interval: 15s
    rules:
      - alert: AppTargetDown
        expr: up{job="app"} == 0
        for: 1m
        labels:
          severity: critical
          service: checkout
        annotations:
          summary: "checkout app target is down"
          description: "Prometheus cannot scrape {{ $labels.instance }} for 1 minute."
          runbook_url: "https://example.invalid/runbooks/app-down"

      - alert: AppAbsent
        expr: absent(up{job="app"})
        for: 2m
        labels:
          severity: critical
          service: checkout
        annotations:
          summary: "checkout app metrics are absent"
          description: "No up series exists for job=app. The target may have been removed."
          runbook_url: "https://example.invalid/runbooks/app-down"

      - alert: AppHighErrorRatio
        expr: |
          (
            sum(rate(http_requests_total{job="app",status=~"5.."}[5m]))
            /
            sum(rate(http_requests_total{job="app"}[5m]))
          ) > 0.05
          and
          (
            sum(rate(http_requests_total{job="app",status=~"5.."}[1m]))
            /
            sum(rate(http_requests_total{job="app"}[1m]))
          ) > 0.05
        for: 2m
        labels:
          severity: critical
          service: checkout
        annotations:
          summary: "checkout error ratio above 5%"
          description: "5m and 1m error ratios are both above 5%. Current value: {{ $value | humanizePercentage }}."
          runbook_url: "https://example.invalid/runbooks/high-error-ratio"

      - alert: AppHighLatency
        expr: |
          histogram_quantile(
            0.95,
            sum by (le) (rate(http_request_duration_seconds_bucket{job="app",route="/work"}[5m]))
          ) > 0.5
        for: 2m
        labels:
          severity: warning
          service: checkout
        annotations:
          summary: "checkout p95 latency above 500ms"
          description: "p95 has been above 500ms for 2 minutes. Current value: {{ $value }}s."
          runbook_url: "https://example.invalid/runbooks/high-latency"

      - alert: AppNoTraffic
        expr: sum(rate(http_requests_total{job="app"}[5m])) == 0
        for: 5m
        labels:
          severity: warning
          service: checkout
        annotations:
          summary: "checkout is receiving no traffic"
          description: "No requests observed for 5 minutes. Confirm whether this is expected (idle) or a routing failure."
          runbook_url: "https://example.invalid/runbooks/no-traffic"
```

Notes:

- `AppTargetDown` is cause-based but directly actionable: a scrape failure is a monitoring gap, not a user symptom. Keep it, but do not let it replace the SLO alerts.
- `AppHighErrorRatio` uses a long and a short window. Both must breach. This is a simplified burn-rate condition; in a real SLO, replace `0.05` with `14.4 × (1 - SLO)` for the fast burn.
- `AppNoTraffic` is a warning, not a page. An idle internal service is not necessarily broken. For a public checkout, no traffic is a critical symptom.
- `for: 2m` prevents a single bad scrape from paging. It also delays detection; choose it with the SLO and the traffic rate in mind.
- `runbook_url` is a placeholder. A real runbook URL must resolve for the on-call human, not just look good in a review.

Test the rules:

```bash
promtool check rules rules/alerts.yml
```

Or with the container:

```bash
docker run --rm -v "$PWD/rules:/rules:ro" --entrypoint promtool prom/prometheus:v2.55.1 check rules /rules/alerts.yml
```

Expected:

```text
Checking rules/alerts.yml
  SUCCESS: 5 rules found
```

### Update Prometheus config

Add the alert rules and the Alertmanager target:

```yaml
rule_files:
  - /etc/prometheus/rules/*.yml

alerting:
  alertmanagers:
    - static_configs:
        - targets: ["alertmanager:9093"]
```

The glob already includes `alerts.yml` if the directory is mounted. Do not add a second `rule_files` or `alerting` key; YAML will keep only one.

Without the `alerting` block, Prometheus evaluates rules and shows them as firing in its own UI, but never sends them to Alertmanager. That is a real failure mode, and it is why "the rule is firing" and "the human was notified" are different claims.

### Test the alert rule

Create `tests/alerts_test.yml`:

```yaml
rule_files:
  - ../rules/alerts.yml

evaluation_interval: 1m

tests:
  - interval: 1m
    input_series:
      - series: 'up{job="app",instance="app:8000"}'
        values: '1 1 0 0 0'
    alert_rule_test:
      - eval_time: 2m
        alertname: AppTargetDown
        exp_alerts: []
      - eval_time: 5m
        alertname: AppTargetDown
        exp_alerts:
          - exp_labels:
              severity: critical
              service: checkout
              job: app
              instance: app:8000
            exp_annotations:
              summary: "checkout app target is down"
              description: "Prometheus cannot scrape app:8000 for 1 minute."
              runbook_url: "https://example.invalid/runbooks/app-down"
```

Run it:

```bash
promtool test rules tests/alerts_test.yml
```

Or with the container:

```bash
docker run --rm -v "$PWD/rules:/rules:ro" -v "$PWD/tests:/tests:ro" \
  --entrypoint promtool prom/prometheus:v2.55.1 \
  test rules /tests/alerts_test.yml
```

Expected:

```text
Unit Testing: tests/alerts_test.yml
  SUCCESS
```

The first assertion proves the alert does **not** fire on a healthy target; the second proves it fires only after `for: 1m`. A test that only checks the firing case can pass while the alert pages on every normal scrape.

### Update the compose file

Add Grafana, Alertmanager, and the webhook:

```yaml
services:
  app:
    image: python:3.12-slim
    command: ["python", "/app/app.py"]
    working_dir: /app
    volumes:
      - ./app.py:/app/app.py:ro
    ports:
      - "127.0.0.1:8000:8000"

  prometheus:
    image: prom/prometheus:v2.55.1
    command:
      - --config.file=/etc/prometheus/prometheus.yml
      - --storage.tsdb.path=/prometheus
      - --storage.tsdb.retention.time=6h
      - --web.enable-lifecycle
    volumes:
      - ./prometheus.yml:/etc/prometheus/prometheus.yml:ro
      - ./rules:/etc/prometheus/rules:ro
      - promdata:/prometheus
    ports:
      - "127.0.0.1:9090:9090"
    depends_on:
      - app

  alertmanager:
    image: prom/alertmanager:v0.27.0
    command:
      - --config.file=/etc/alertmanager/alertmanager.yml
      - --storage.path=/alertmanager
    volumes:
      - ./alertmanager.yml:/etc/alertmanager/alertmanager.yml:ro
      - amdata:/alertmanager
    ports:
      - "127.0.0.1:9093:9093"
    depends_on:
      - webhook

  webhook:
    image: python:3.12-slim
    command: ["python", "/app/webhook.py"]
    working_dir: /app
    volumes:
      - ./webhook.py:/app/webhook.py:ro
    ports:
      - "127.0.0.1:9000:9000"

  grafana:
    image: grafana/grafana:11.2.0
    environment:
      GF_SECURITY_ADMIN_USER: admin
      GF_SECURITY_ADMIN_PASSWORD: labadmin
      GF_AUTH_ANONYMOUS_ENABLED: "true"
      GF_AUTH_ANONYMOUS_ORG_ROLE: Viewer
      GF_ANALYTICS_REPORTING_ENABLED: "false"
      GF_ANALYTICS_CHECK_FOR_UPDATES: "false"
    volumes:
      - ./grafana/provisioning:/etc/grafana/provisioning:ro
      - ./grafana/dashboards:/var/lib/grafana/dashboards:ro
      - grafanadata:/var/lib/grafana
    ports:
      - "127.0.0.1:3000:3000"
    depends_on:
      - prometheus

  node-exporter:
    image: prom/node-exporter:v1.8.2
    args: ["--path.rootfs=/host"]
    pid: host
    volumes:
      - /:/host:ro,rslave
    ports:
      - "127.0.0.1:9100:9100"

volumes:
  promdata:
  amdata:
  grafanadata:
```

If you are extending the L8.1 compose file, merge the services and volumes rather than creating duplicate keys. YAML duplicate keys silently override; `docker compose config` will show the merged result.

Validate and start:

```bash
docker compose config >/dev/null && echo "compose OK"
docker compose up -d
docker compose ps
```

Expected:

```text
alertmanager   running   127.0.0.1:9093->9093/tcp
app            running   127.0.0.1:8000->8000/tcp
grafana        running   127.0.0.1:3000->3000/tcp
node-exporter  running   127.0.0.1:9100->9100/tcp
prometheus     running   127.0.0.1:9090->9090/tcp
webhook        running   127.0.0.1:9000->9000/tcp
```

Readiness:

```bash
curl -sf http://127.0.0.1:9090/-/ready && echo "prometheus ready"
curl -sf http://127.0.0.1:9093/-/ready && echo "alertmanager ready"
curl -sf http://127.0.0.1:3000/api/health | jq
```

Expected Grafana health:

```json
{
  "commit": "...",
  "database": "ok",
  "version": "11.2.0"
}
```

## Part A — Dashboard as a decision surface

Open `http://127.0.0.1:3000`, log in as `admin` / `labadmin` (or browse anonymously as Viewer). Find **OpsLab → OpsLab — Checkout Overview**.

Generate load:

```bash
hey -z 60s -c 20 http://127.0.0.1:8000/work &
hey -z 60s -c 4 http://127.0.0.1:8000/fail &
wait
```

If `hey` is not installed:

```bash
seq 1 400 | xargs -P 20 -I{} curl -s -o /dev/null http://127.0.0.1:8000/work
seq 1 80 | xargs -P 4 -I{} curl -s -o /dev/null http://127.0.0.1:8000/fail
```

Observe the first row:

| Panel | What it answers | What it does not answer |
|---|---|---|
| Availability | Are requests succeeding? | Why they are failing |
| Error ratio | How much of the traffic is bad? | Which route or dependency |
| p95 latency | Is the tail slow? | Whether the mean is fine |
| Request rate | Is the traffic normal? | Whether the traffic is useful |

The second row narrows the fault domain: status distribution and p95 by route. If the first row is green, a busy host is not automatically an incident. If the first row is red, the second row tells you where to look next.

### Exercise: make the dashboard answer a question

Add one panel (or edit the JSON) that answers: **"What fraction of `/work` requests met the 300 ms SLO in the last 5 minutes?"**

Query:

```promql
sum(rate(http_request_duration_seconds_bucket{job="app",route="/work",le="0.3"}[5m]))
/
sum(rate(http_request_duration_seconds_count{job="app",route="/work"}[5m]))
```

Set the unit to `percentunit`, thresholds at 0.99 (orange) and 0.999 (green). Then answer:

- Why is this panel more directly tied to the SLO than p95?
- What happens when there is no traffic? (No data, not 100%.)
- Why is the `le="0.3"` bucket essential?

### Exercise: variables and time range

Change the `$route` variable to `/fail` only. The first-row panels should show the error ratio for that route, not the whole service. Then set the time range to the last 5 minutes and refresh. A dashboard that only works on a 6-hour range is not an incident tool.

### Provisioning behavior

Edit the dashboard title in the UI, save, then reload. With `allowUiUpdates: false`, the file version wins. Change the JSON file, wait 10 seconds, and refresh; the provisioned title returns. This is the trade-off: Git is the source of truth, but UI edits are not durable.

## Part B — Fire a real alert

### Check rule health

```bash
curl -s http://127.0.0.1:9090/api/v1/rules | jq -r '
  .data.groups[] | .name as $g | .rules[] |
  [$g, .name, .health, (.lastError // ""), (.state // "")] | @tsv'
```

Expected:

```text
app-alerts  AppTargetDown       ok
app-alerts  AppAbsent           ok
app-alerts  AppHighErrorRatio   ok
app-alerts  AppHighLatency      ok
app-alerts  AppNoTraffic        ok
app-slo     job:http_requests:rate5m  ok
...
```

### Trigger `AppTargetDown`

```bash
docker compose stop app
sleep 75
curl -s http://127.0.0.1:9090/api/v1/alerts | jq -r '.data.alerts[] | [.labels.alertname, .state, .activeAt] | @tsv'
```

Expected:

```text
AppTargetDown  firing  2026-10-27T...
```

You may also see `AppNoTraffic` as `pending`; its `for` is 5 minutes, so a 75-second outage is not long enough to fire it. That is intentional: the target-down alert already covers the outage, and a second page for "no traffic" would be redundant. In Alertmanager, the critical `AppTargetDown` also inhibits the warning `AppNoTraffic` for the same job.

Why is `AppNoTraffic` not the primary signal here? Because an idle service and a dead service both have no requests. `AppTargetDown` distinguishes the scrape failure; `AppNoTraffic` is for a live process that is receiving no routed traffic.

Check Alertmanager:

```bash
curl -s http://127.0.0.1:9093/api/v2/alerts | jq -r '.[] | [.labels.alertname, .status.state, .labels.severity] | @tsv'
```

Expected:

```text
AppTargetDown  active  critical
```

Check the webhook logs:

```bash
docker compose logs --tail=40 webhook
```

Expected:

```text
=== alert batch ===
status: firing
AppTargetDown | critical | app | firing | checkout app target is down
{
  "receiver": "lab-webhook",
  "status": "firing",
  "alerts": [
    {
      "status": "firing",
      "labels": {
        "alertname": "AppTargetDown",
        "severity": "critical",
        "job": "app",
        "service": "checkout"
      },
      "annotations": {
        "summary": "checkout app target is down",
        "runbook_url": "https://example.invalid/runbooks/app-down"
      },
      ...
    }
  ],
  ...
}
```

The payload is the contract between Prometheus/Alertmanager and every receiver. Read the labels and annotations; a receiver that cannot see `severity`, `service`, or `runbook_url` cannot route or act.

### Resolve

```bash
docker compose start app
sleep 90
docker compose logs --tail=20 webhook
```

Expected: a second batch with `status: resolved` and the same fingerprint. If `send_resolved` is false, the receiver never learns that the incident ended; the on-call human may keep investigating a resolved issue.

### Trigger `AppHighErrorRatio`

```bash
hey -z 3m -c 10 http://127.0.0.1:8000/fail
```

In another terminal:

```bash
watch -n 5 'curl -s http://127.0.0.1:9093/api/v2/alerts | jq -r ".[] | [.labels.alertname, .status.state, .labels.severity] | @tsv"'
```

Expected sequence:

1. `AppHighErrorRatio` pending after both windows breach.
2. `firing` after `for: 2m`.
3. Alertmanager groups it with any other firing alerts for the same `alertname`/`severity` (in this lab, one group).
4. Webhook receives the batch.

If it never fires, check:

- Is the app returning 500s? `curl -s -o /dev/null -w '%{http_code}\n' http://127.0.0.1:8000/fail`
- Is the rule evaluating? `/api/v1/rules` health and `lastEvaluation`.
- Is the expression true in the query browser? Run it without the `> 0.05` to see the value.
- Is the alert inhibited? Alertmanager UI → Silences/Inhibitions.

### Silence

```bash
docker exec l82-alertmanager-1 amtool silence add \
  alertname=AppHighErrorRatio \
  --alertmanager.url=http://localhost:9093 \
  --duration=10m \
  --comment="lab: testing the dashboard"
```

Container names depend on the compose project. If the name is wrong, use `docker compose exec alertmanager amtool ...` instead:

```bash
docker compose exec alertmanager amtool silence add \
  alertname=AppHighErrorRatio \
  --alertmanager.url=http://localhost:9093 \
  --duration=10m \
  --comment="lab: testing the dashboard"
```

List and expire:

```bash
docker compose exec alertmanager amtool silence query --alertmanager.url=http://localhost:9093
docker compose exec alertmanager amtool silence expire <silence-id> --alertmanager.url=http://localhost:9093
```

A silence is not a fix. It has an owner, a reason, and an expiry. A silence without an expiry is a permanently muted alarm.

### Inhibition

With the app stopped, `AppTargetDown` (critical) should inhibit `AppNoTraffic` (warning) for the same `job`. Check the Alertmanager UI (`http://127.0.0.1:9093`) or:

```bash
curl -s http://127.0.0.1:9093/api/v2/alerts | jq -r '.[] | [.labels.alertname, .status.state, .status.inhibitedBy] | @tsv'
```

Why inhibit? Because a single cause (target down) would otherwise produce a page plus a warning plus a "no traffic" ticket. Inhibition reduces correlated noise, but it can hide a second, independent problem. Use it only when the causal relationship is real.

## Part C — Break the notification path

Do not read the analysis until you have written your evidence.

### Fault 1 — Alert fires but the webhook receives nothing

Stop the webhook:

```bash
docker compose stop webhook
docker compose stop app
sleep 75
curl -s http://127.0.0.1:9090/api/v1/alerts | jq -r '.data.alerts[] | [.labels.alertname, .state] | @tsv'
curl -s http://127.0.0.1:9093/api/v2/alerts | jq -r '.[] | [.labels.alertname, .status.state] | @tsv'
docker compose logs --tail=30 alertmanager
```

**Your task:** the alert is firing in Prometheus. Is it firing in Alertmanager? If not, where is the break? If yes, where is the notification stuck? Write the evidence before changing anything.

<details>
<summary>After you have written your evidence</summary>

Prometheus evaluates rules and sends firing alerts to Alertmanager. Alertmanager routes and delivers them. If the alert is firing in Prometheus but absent in Alertmanager, verify that the `alerting` block exists and points at the Alertmanager service:

```yaml
alerting:
  alertmanagers:
    - static_configs:
        - targets: ["alertmanager:9093"]
```

Then check `prometheus.yml` was reloaded, and that the target is reachable from the Prometheus container (`wget -qO- http://alertmanager:9093/-/ready`). A configuration file on disk is not an applied configuration.

If it is active in Alertmanager but the webhook has no request, check the receiver URL (`webhook:9000` from inside the Alertmanager container, not `localhost`), the webhook container health, network DNS, and Alertmanager logs for `Notify for alerts failed`. If the webhook is stopped, Alertmanager retries with backoff; the alert remains active, and notifications may be delayed after the receiver returns. Restart the webhook and watch the retry. The alert firing is not the same as the human being notified.

</details>

### Fault 2 — The dashboard is empty after provisioning

```bash
curl -s http://127.0.0.1:3000/api/datasources | jq -r '.[] | [.name, .type, .url, .uid] | @tsv'
docker compose logs --tail=30 grafana
```

**Your task:** is the data source present, is the URL reachable from Grafana, and does the dashboard query use the same UID? What would you check before editing the JSON?

<details>
<summary>After you have written your evidence</summary>

Check the data source URL from inside Grafana:

```bash
docker compose exec grafana wget -qO- http://prometheus:9090/-/ready
```

A common failure is `url: http://localhost:9090` inside the Grafana container, which points to Grafana itself. Another is a dashboard datasource UID that does not match the provisioned UID (`prometheus`). Grafana logs show provisioning errors, but a query error appears in the panel, not always in the log. Test the query in Explore with the same data source before blaming the dashboard.

</details>

### Fault 3 — Alert storm

Create a temporary rule that fires for every route, or send a burst of errors while the error-ratio alert is active. Observe how many notifications arrive.

**Your task:** which Alertmanager settings would reduce the number of messages without hiding the incident? What is the risk of each?

<details>
<summary>After you have written your evidence</summary>

`group_by` merges alerts with the same labels; `group_wait` batches new alerts; `group_interval` controls updates; `repeat_interval` controls reminders. `inhibit_rules` suppresses a lower-severity alert when a related higher-severity one is active. Each has a cost: grouping can hide a distinct alert in a batch, a long `group_wait` delays the first page, and inhibition can suppress an independent problem. Tune with a real incident review, not by guessing.

</details>

## Expected observations

| Observation | Expected | Meaning |
|---|---|---|
| Grafana data source | `Prometheus`, URL `http://prometheus:9090`, UID `prometheus` | Provisioning worked |
| Dashboard first row | Four stats populate under load | Queries and time range are correct |
| `AppTargetDown` | Firing after ~1m, resolved after restart | `for` and scrape interval interact |
| `AppHighErrorRatio` | Firing after ~2m of 5%+ errors | Both windows must breach |
| Alertmanager active alert | Same labels as Prometheus, plus receiver | Routing path works |
| Webhook payload | `status`, `alerts[]`, labels, annotations | Receiver contract |
| Silence | Alert suppressed for the duration, then returns | Silence is time-bound |
| Inhibition | Warning suppressed while critical active | Correlated noise reduction |
| Provisioned dashboard edit | Reverts after reload | Git/file is source of truth |

## Internal explanation

### The alert path

```text
Prometheus rule evaluation
        │  firing / pending
        ▼
Alertmanager (group, inhibit, silence, route)
        │  webhook/email/Slack/PagerDuty
        ▼
Receiver (human or automation)
        │  acknowledge, act, resolve
        ▼
Prometheus resolves -> Alertmanager sends resolved
```

Every arrow can fail independently. A rule that never evaluates is not an alert. A firing alert that is not routed is not a page. A page that no one acknowledges is not a response. Test the whole path, not just the PromQL.

### `for` and evaluation interval

`for: 2m` means the expression must be true for every evaluation in a 2-minute window. With a 15-second evaluation interval, that is about eight consecutive evaluations. If the expression flaps, the timer resets. A long `for` reduces false pages but delays real ones; choose it from the symptom's acceptable detection time, not from a template.

### Grouping, inhibition, and silences

- **Grouping** combines alerts with the same label set into one notification. `group_by: [alertname, severity]` groups all instances of the same alert; `group_by: [cluster, severity]` groups by cluster. Group too much and you hide distinct problems; group too little and you page per instance.
- **Inhibition** suppresses a target alert while a source alert is active and the `equal` labels match. It encodes a causal assumption. If the assumption is wrong, you lose a page.
- **Silence** suppresses matching alerts for a fixed duration with a comment. It is a human override, not a routing rule. It must expire.

### Dashboard design as compression

A good dashboard compresses many series into a decision: "users are affected" or "users are not affected." The first row is the compression; the lower rows are the expansion for diagnosis. If the first row requires reading a legend or doing arithmetic, it is not doing its job.

The dashboard also encodes assumptions: the error ratio threshold, the latency SLO, the traffic baseline. Put those assumptions in the panel description or the runbook, not only in someone's head.

### Provisioning and drift

Grafana provisioning reads files at startup and on an interval. `allowUiUpdates: false` makes the file authoritative. This prevents drift but also means a quick UI fix is lost on reload. The workflow is: edit the JSON in Git, review it, deploy it. If you need a UI edit, export it and commit it.

## Troubleshooting task

A teammate reports: "The alert fired, but I got no notification." You have the Prometheus UI, Alertmanager UI, and webhook logs.

Collect evidence in this order:

1. `/api/v1/rules` — is the rule `ok`, and is the alert `firing` or `pending`?
2. `/api/v1/alerts` — does Prometheus have the alert?
3. `prometheus.yml` `alerting.alertmanagers` — is Alertmanager configured?
4. `/api/v2/alerts` in Alertmanager — did it arrive?
5. Alertmanager logs — any `Notify for alerts failed`?
6. Receiver URL and DNS from inside the Alertmanager container.
7. Webhook logs — request received? Status code?
8. Silences and inhibition — is the alert suppressed?

Write a one-paragraph incident note with the first broken arrow and the evidence. Only then change a config.

## Verification

- [ ] Grafana health is `ok`; the data source UID is `prometheus`.
- [ ] The dashboard first row answers "are users affected?" without arithmetic.
- [ ] The dashboard uses `$route` and a 5-minute range without breaking.
- [ ] `promtool check rules` passes for both rule files.
- [ ] `AppTargetDown` fires after ~1 minute and resolves after restart.
- [ ] `AppHighErrorRatio` requires both windows and fires after `for`.
- [ ] Alertmanager routes to the local webhook; the payload includes labels and annotations.
- [ ] A resolved notification arrives when the app restarts.
- [ ] A silence suppresses the alert for a fixed duration and expires.
- [ ] Inhibition suppresses the warning while the critical is active.
- [ ] A UI edit to a provisioned dashboard reverts on reload.
- [ ] You can explain why "alert firing" does not mean "human notified."

## Cleanup

```bash
docker compose down -v
```

Remove the lab directory only after you have captured the write-up and any dashboard JSON you want to keep. The volumes contain Prometheus TSDB, Alertmanager state, and Grafana's database; `-v` deletes them.

## Write-up template

```markdown
# L8.2 — Dashboards and Alerting

## Dashboard
- First row panels and the question each answers:
- The SLO panel query:
- What changes when `$route=/fail`:
- What happens with no traffic:

## Alert rules
| Alert | Symptom or cause | `for` | Severity | Runbook |
|---|---|---|---|---|
| | | | | |

## Alert path evidence
- Prometheus rule state:
- Alertmanager active alerts:
- Webhook payload (status, labels, annotations):
- Resolved notification:
- Silence/inhibition behavior:

## Fault 1 — no notification
- First broken arrow:
- Evidence:
- Fix:
- Test that would catch it next time:

## Fault 2 — empty dashboard
- Data source check:
- Query check:
- Root cause:

## Alert quality review
- Which alert would wake you up unnecessarily?
- Which alert would miss a real outage?
- What would you change first?

## Runbook draft (for the highest-severity alert)
- Symptom:
- First 5 checks:
- Mitigation:
- Escalation:
- Evidence to preserve:
```

---

## Appendix — Local binaries (no Docker)

Use this track if Docker is unavailable. The queries, rules, dashboard JSON, and alert semantics are unchanged; only the hostnames and process management differ.

### Install (macOS, Homebrew)

```bash
brew install prometheus node-exporter grafana alertmanager hey jq
```

On Debian/Ubuntu, install the binaries from your distribution or download the release archives. Verify:

```bash
prometheus --version
node_exporter --version
grafana --version
alertmanager --version
```

### L8.1 local stack

Create `prometheus.local.yml`:

```yaml
global:
  scrape_interval: 5s
  evaluation_interval: 5s

rule_files:
  - rules/*.yml

scrape_configs:
  - job_name: prometheus
    static_configs:
      - targets: ["localhost:9090"]
  - job_name: node
    static_configs:
      - targets: ["localhost:9100"]
  - job_name: app
    static_configs:
      - targets: ["localhost:8000"]
        labels:
          service: checkout
```

Run each in its own terminal from `~/opslab/phase8/l8.1`:

```bash
# terminal 1
python3 app.py

# terminal 2
node_exporter

# terminal 3
prometheus \
  --config.file=prometheus.local.yml \
  --storage.tsdb.path=./data \
  --web.enable-lifecycle
```

Check:

```bash
curl -s http://localhost:9090/-/ready
curl -s http://localhost:8000/metrics | head
curl -s http://localhost:9090/api/v1/targets | jq -r '.data.activeTargets[] | [.labels.job,.health,.lastError] | @tsv'
```

macOS `node_exporter` exposes a smaller metric set and some collectors are disabled by default. If a `node_*` query from the lab returns no data, list the available names first:

```bash
curl -s http://localhost:9100/metrics | grep '^node_' | cut -d'{' -f1 | sort -u | head -40
```

### L8.2 local stack

Create `alertmanager.local.yml` with the receiver pointing at `localhost:9000`:

```yaml
route:
  receiver: lab-webhook
  group_by: ["alertname", "severity"]
  group_wait: 10s
  group_interval: 30s
  repeat_interval: 4h

receivers:
  - name: lab-webhook
    webhook_configs:
      - url: http://localhost:9000/alerts
        send_resolved: true

inhibit_rules:
  - source_matchers: ['severity="critical"']
    target_matchers: ['severity="warning"']
    equal: ["job"]
```

Run:

```bash
# terminal 4
python3 webhook.py

# terminal 5
alertmanager --config.file=alertmanager.local.yml --storage.path=./amdata

# terminal 6
GF_PATHS_PROVISIONING="$PWD/grafana/provisioning" \
GF_PATHS_DATA="$PWD/grafana/data" \
GF_AUTH_ANONYMOUS_ENABLED=true \
GF_AUTH_ANONYMOUS_ORG_ROLE=Viewer \
GF_SECURITY_ADMIN_PASSWORD=labadmin \
grafana server --homepath "$(brew --prefix grafana)"
```

Add the alerting block to `prometheus.local.yml`:

```yaml
alerting:
  alertmanagers:
    - static_configs:
        - targets: ["localhost:9093"]
```

Reload Prometheus:

```bash
curl -s -X POST http://localhost:9090/-/reload
```

The provisioned data source URL must be `http://localhost:9090`, not `http://prometheus:9090`, when Grafana runs as a local process. Either edit the provisioning file or create a separate `grafana.local/provisioning/datasources/prometheus.yml` and point `GF_PATHS_PROVISIONING` at it. Do not run both stacks at once; ports will collide.

Grafana's macOS app/Homebrew paths vary by version. If `grafana server` cannot find its home path, run `brew info grafana` and use the printed `--homepath`, or run Grafana with `--config` pointing at a minimal config. The lab is about the data source and dashboard, not about packaging.
