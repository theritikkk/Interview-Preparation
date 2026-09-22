# Loki

---

## Cluster 1: Log Aggregation — Indexed Labels, Not Full-Text

### What is it?
Loki indexes only metadata **labels** (e.g., `service=worker-1`, `level=error`) rather than the full text of every log line (unlike Elasticsearch, which builds a full-text inverted index over log content) — making it far cheaper to run at scale, at the cost of text search within a label-filtered set being a grep-style scan rather than an indexed lookup.

### Interview Questions
- **L1:** "Loki indexes log labels/metadata, not full log text — you filter by labels first (like a service or severity), then it scans/greps within that filtered set, which is much cheaper to store and run than a full-text index over every log line."
- **L2:** explain why this design fits naturally alongside Prometheus: Loki deliberately mirrors Prometheus's label model, so the same labels (`service`, `instance`) that identify a metric's time series also identify that service's logs — letting you pivot directly from a metric spike in Grafana to that exact service's logs in the same time window, using the same label vocabulary.
- **L3:** explain when you'd need Elasticsearch instead: if you need fast full-text search/relevance ranking *across* log content itself (not just filtered-then-grepped), Loki's design deliberately doesn't optimize for that — it's a cost/capability tradeoff, not an oversight.

### Quick Revision
- Loki indexes labels, not full text — cheap at scale, but text search within a label filter is a scan, not an index lookup.
- Shares Prometheus's label model on purpose — enables metric → log correlation using the same labels.
