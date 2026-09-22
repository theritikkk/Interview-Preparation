# Grafana

Visualization layer over Prometheus/Loki in the Task Scheduler's observability stack.

---

## Cluster 1: Dashboards & Data Source Agnosticism

### What is it?
Grafana is a visualization layer, not a metrics store itself — it queries data sources (Prometheus, Loki, CloudWatch, and others) and renders the results as panels/dashboards. This separation means the same dashboard tooling works whether your underlying data lives in Prometheus (metrics) or Loki (logs).

### Interview Questions
- **L1:** "Grafana queries and visualizes data from sources like Prometheus and Loki — it's the dashboard/visualization layer, not the metrics storage itself."
- **L2:** explain why your Task Scheduler's p95 latency recovery (160ms spike → 76ms floor) is a natural Grafana panel: a time-series graph querying Prometheus's histogram data via PromQL (e.g., `histogram_quantile(0.95, ...)`), letting you visually correlate that recovery against the same timeline as queue-depth and throughput panels — this kind of *cross-metric, same-timeline correlation* is Grafana's core value versus looking at each metric in isolation.

### Quick Revision
- Grafana visualizes; Prometheus/Loki store. Keep this separation clear if asked "what does Grafana actually do."
- Its real value is correlating multiple metrics/logs on the same timeline, not any single panel in isolation.
