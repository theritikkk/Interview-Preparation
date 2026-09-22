# Prometheus

🔴 Core metrics pipeline for both the Distributed Task Scheduler and NetSight.

---

## Cluster 1: Metric Types & the Pull-Based Scraping Model

### What is it?
Prometheus **pulls** (scrapes) metrics from targets at a regular interval via an HTTP `/metrics` endpoint each service exposes, rather than services pushing metrics to it — a deliberate design choice (Prometheus controls scrape timing, can detect a target that's stopped responding by a failed scrape, and targets don't need to know anything about where metrics ultimately go).

Four core metric types: **Counter** (monotonically increasing, e.g., total requests processed — never decreases, so rate-of-change over time is the meaningful signal), **Gauge** (goes up or down, e.g., current queue depth), **Histogram** (buckets observations, e.g., request-latency distribution — lets you compute percentiles), **Summary** (similar to histogram, but computes percentiles client-side rather than letting the server compute them from buckets — less flexible for aggregation across instances).

### Interview Questions
- **L1:** "Prometheus scrapes metrics from a `/metrics` HTTP endpoint on each target at a regular interval; the four core metric types are counters (monotonic), gauges (up/down), histograms, and summaries (both for latency-style distributions)."
- **L2:** explain why your Task Scheduler's **queue depth** (8,562 peak) is naturally a **gauge** (it goes up and down) while **total messages processed** would naturally be a **counter** — and why p95 latency specifically requires a **histogram**, not a gauge or counter, since percentiles need the underlying distribution, not just a running total or current value.
- **L3:** explain why pull-based scraping is a good fit for your Task Scheduler's 3 independently metriced worker replicas specifically: Prometheus can scrape each replica's own `/metrics` endpoint independently, immediately revealing per-replica differences (useful for diagnosing "is only one replica struggling") — versus a push model where all replicas' metrics would need explicit per-replica labeling to achieve the same visibility.

### Quick Revision
- Pull-based scraping, not push — Prometheus controls timing and detects dead targets via failed scrapes.
- Counter: monotonic, look at rate. Gauge: point-in-time value, up/down. Histogram: bucketed distribution, enables percentile queries.
- p95/p99 latency needs a histogram — a gauge or counter alone can't produce a percentile.
