# Observability Incidents (INC-12 – INC-13)

> **Evidence first.** Write your timeline, hypotheses, and discriminating test before the analysis marker. Observability incidents are special: the monitoring system itself is the patient. The danger is a blind spot — you cannot see what the broken system is not telling you, so prefer **raw evidence** (logs, `/proc`, TSDB status pages) over the dashboards built on the broken system.

---

## INC-12 — Prometheus memory climbs until OOM, on a schedule

### The page

> **04:00 for four nights running, SEV1** — Prometheus OOM-killed at ~04:00, restarts, survives until the next 04:00. Dashboards lose data each night. Deploy history shows a new service deployed Tuesday; the OOMs started Wednesday.

### The evidence (raw, as collected)

```bash
kubectl -n monitoring get pod prometheus-0 -o jsonpath='{.status.containerStatuses[0].lastState.terminated.reason}{"\n"}'
# OOMKilled

# Prometheus's own TSDB status endpoint (raw, not a dashboard):
curl -s http://prometheus:9090/api/v1/status/tsdb | jq '{
  headStats: .data.headStats,
  top_labels: (.data.topLabels[0:5]),
  top_series: (.data.topSeriesCounters[0:5])
}'
# (v2.x: /api/v1/status/tsdb; on older versions use the admin TSDB stats page)

curl -s http://prometheus:9090/api/v1/label/__name__/values | jq '.data | length'
curl -s 'http://prometheus:9090/api/v1/query?query= count(count by (__name__)({__name__=~".+"}))' | jq
```

```json
{
  "headStats": {
    "numSeries": 48210000,
    "numLabelPairs": 31200000,
    "chunkCount": 120300000,
    "minTime": 1730000000000
  },
  "top_labels": [
    {"name": "path", "value": "/api/v1/checkout", "count": 9210000},
    {"name": "path", "value": "/api/v1/cart", "count": 5100000}
  ]
}
```

Series count at the deploy was ~3.2M; it is now 48.2M. A new service began exporting `http_requests_total` with a **raw `path` label** containing full URLs including IDs (`/api/v1/checkout/82731/items/...`).

### Your turn (before the line)

- [ ] Timeline.
- [ ] Three hypotheses ranked with evidence.
- [ ] The one discriminating test.
- [ ] First 10 minutes.

---
---

# ANALYSIS — INC-12

### Timeline

Tuesday: a service deployed with an unnormalized `path` label. Every unique URL (including resource IDs and query variants) became a distinct time series per route/status/method. Series count grew from 3.2M to 48M in a day. Prometheus's head block memory grows with active series and label pairs; nightly, the 04:00 long-range queries plus WAL replay pushed it over the container limit, and the kernel OOM-killed it. Each restart replayed the WAL (large now), survived a few hours, and died again at the next heavy evaluation window.

### Hypotheses ranked

1. **Cardinality explosion from a new label.** TSDB status shows 48M head series (vs a 3.2M baseline), with `path` dominating; a deploy preceded the change. Matches.
2. **A slow query / recording rule loop.** Possible, but rule evaluations are periodic and would show in rule health, and the memory growth trend with the deploy is a stronger signal.
3. **A scrape target duplicating itself (e.g., scraping a debug endpoint).** Rejected: `__name__` count and `topLabels` point at label cardinality, not target count.

### The discriminating test

The TSDB status endpoint's `topLabels`/`topSeriesCounters` — it names the exact label and value consuming the series budget. If cardinality were flat and memory still grew, the next test would be `topQueries` and rule health. Here the label evidence is direct.

### Root cause

An unbounded label value (`path` with raw URLs) multiplied every other label combination. In Prometheus, each unique label-value combination is a distinct series with its own storage, index entry, and scrape cost; 48M series × 1.5 bytes/sample at 15 s is ~4,600 samples/s ingested into a head that was sized for 3.2M series.

### Fix (smallest safe first)

1. Stop the bleeding: relabel the offending service's metrics at scrape time to drop/normalize `path`:

```yaml
metric_relabel_configs:
  - source_labels: [path]
    regex: '^(/api/v1/checkout)/[^/]+.*'
    target_label: path
    replacement: '${1}/:id'
    action: replace
  - action: labeldrop
    regex: '(request_id|user_id)'
```

2. Verify the series count falls after two scrape intervals and the head stabilizes.
3. Fix the exporter to normalize routes at the source (route templates, not raw paths), and redeploy.
4. Delete the accumulated garbage once the source is fixed (`--storage.tsdb.retention` time will age it out; for immediate relief, use the TSDB admin delete API on the label matcher — document this, as it is destructive and easy to misuse).

### Prevention

- **Series-count budget and alert:** `prometheus_tsdb_head_series > 20M` pages someone; a 48M spike is caught at 20M.
- Per-target cardinality top-N panel reviewed weekly.
- A CI/exporter lint rule: no `path`, `request_id`, `user_id`, `error_text` as label values; raw identifiers belong in logs/traces.
- Scrape-time relabeling as a guardrail is fine, but the exporter is where the fix belongs.
- Load-test cardinality: a synthetic exporter with 1M unique paths should trip the alert, not the OOM killer.

### Lessons

- **Cardinality is a budget, not a style preference.** Every unique label combination is a series; the series count is the cost driver.
- **The TSDB status endpoint is the diagnostic,** not the dashboards (which are built on the patient).
- **Raw identifiers in labels are the most common way to spend the whole budget at once.** Normalize to route templates.

### 5-minute walkthrough script

> "Prometheus was OOM-killed at 04:00 four nights running, starting the day after a deploy. The patient was the monitoring system, so I went to its own status endpoints rather than the dashboards. The TSDB status page showed head series had grown from 3.2M to 48M, with a `path` label dominating — a new service was exporting raw URLs including resource IDs as a label value. Each unique URL became a series; nightly evaluation pushed memory over the limit. Immediate fix was scrape-time relabeling to normalize the path and drop identifier labels, verified by watching head series fall; the real fix was normalizing in the exporter. Prevention: a head-series alert at 20M, a weekly top-N cardinality panel, and a lint rule against identifier labels. The lesson: cardinality is a budget, and one unbounded label can spend it in a day."

---

## INC-13 — p95 looks fine on the dashboard, users disagree

### The page

> **Support escalation, SEV2** — Users report the checkout is slow several times a day. The latency dashboard shows p95 ≈ 180 ms, under the 250 ms target. The deploy history is clean; error rates are normal.

### The evidence (raw, as collected)

```promql
# The dashboard's p95 query (as written):
avg(
  histogram_quantile(0.95, sum by (le, instance) (rate(http_request_duration_seconds_bucket[5m])))
)

# A correct aggregation (for comparison):
histogram_quantile(0.95,
  sum by (le) (rate(http_request_duration_seconds_bucket[5m]))
)
```

```text
Query A (dashboard, avg of per-instance quantiles): 0.181 s
Query B (aggregate then quantile):                  0.87 s

Per-instance quantiles (the components being averaged):
instance-1: 0.10 s   (healthy replica, most traffic)
instance-2: 0.12 s
instance-3: 0.98 s   (one replica stuck in GC)
instance-4: 2.10 s   (one replica with a connection-pool stall)

Sample counts (5m rate): instance-1: 480/s, instance-2: 470/s,
instance-3: 18/s, instance-4: 9/s    <-- the slow instances serve almost no traffic
```

### Your turn (before the line)

- [ ] Timeline.
- [ ] Three hypotheses ranked with evidence.
- [ ] The one discriminating test.
- [ ] First 10 minutes.

---
---

# ANALYSIS — INC-13

### Timeline

The dashboard's p95 query computed the 95th percentile **per instance**, then averaged the four numbers with `avg()`. Two instances were nearly dead in terms of traffic (18 req/s and 9 req/s out of ~980 req/s total) but very slow. Their high quantiles were averaged with the healthy instances' low quantiles, producing a "p95" of 181 ms that described no actual request distribution. Users hitting the two stuck replicas experienced ~1–2 s latencies.

### Hypotheses ranked

1. **Wrong aggregation: averaging per-instance quantiles.** The two queries disagree by ~5×; quantiles are not additive, and `avg()` of quantiles is mathematically meaningless. Matches.
2. **A real latency problem hidden by traffic weighting.** Related, but it is the same root: once you aggregate the buckets correctly (query B), the 0.87 s p95 appears — the dashboard was hiding a real problem, not just mislabeling a fake one.
3. **Client-side latency / a dependency.** Rejected: server-side buckets, correctly aggregated, show the same slowness; no dependency error rate.

### The discriminating test

Run the correctly-aggregated query next to the dashboard's query. If they disagree materially, the dashboard is wrong. The deeper test: per-instance traffic alongside per-instance latency — the two slow instances serve 2.7% of requests, so any per-instance percentile is built on ~5,400 samples per 5 minutes (enough) but the aggregate is what describes users.

### Root cause

Two mistakes compounded: (a) `avg()` over `histogram_quantile` results — quantiles cannot be averaged; (b) no view of **which instances are slow**, so a stuck replica with a tiny traffic share was invisible. The correct method is to aggregate the bucket counts (`sum by (le)`) across all instances **first**, then compute the quantile once over the whole distribution.

### Fix (smallest safe first)

1. Fix the dashboard query to aggregate first, quantile second (query B).
2. Add a per-instance latency panel **with traffic** (a second series on the same panel), so a low-traffic, high-latency instance is visible rather than averaged away.
3. Investigate the two stuck replicas (GC stall, connection-pool saturation — `db_pool_in_use` at max, wait time rising) — the correctly-aggregated p95 shows the user pain is real.
4. Add a recording rule for the correct p95 so dashboards and alerts share one definition.

### Prevention

- A team rule, enforced in dashboard review: **never put `histogram_quantile` inside `avg()`**; lint dashboards if you provision them as code.
- Recording rules for every SLO metric so there is exactly one definition.
- A "slowest instance" panel (max latency with traffic overlay) — it is the panel that catches this class.
- Load-balance check: the two slow replicas received ~2% of traffic, which is itself a finding (a least-conn balancer would send more; verify the balancer's algorithm and health checks).

### Lessons

- **Quantiles are not additive.** `avg(quantile(x))` produces a number that describes nothing. Aggregate the histogram buckets, then take the quantile once.
- **Traffic-weighted blindness:** a per-instance view with traffic is the antidote to "the average hides the tail."
- **When users and dashboards disagree, believe the users and audit the query.** The query was the bug — and it hid a real incident (two stuck replicas) behind a fake healthy number.

### 5-minute walkthrough script

> "Users reported slow checkouts while the p95 panel showed 180 ms against a 250 ms target. The dashboard computed the 95th percentile per instance and then averaged the four results — quantiles can't be averaged, so that number described nothing. When I ran the correct aggregation — sum the buckets across instances, then quantile once — p95 was 870 ms. The per-instance panel with traffic showed why: two replicas stuck in GC/connection-wait served only 2–3% of traffic but dragged the tail; the healthy replicas' low quantiles averaged their high ones away. Fix: correct the query, add a recording rule as the single definition, add a slowest-instance panel, and lint dashboards against quantile-in-avg. The lesson: aggregate first, quantile second — and when users and dashboards disagree, the dashboard query is a suspect."
