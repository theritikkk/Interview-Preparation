# CloudWatch

⚠️ Skills-list item — not named in the Hermes project bullets despite Hermes being AWS-native. `NEEDS VERIFICATION`: confirm whether Hermes actually emits to CloudWatch (Lambda does so automatically for basic invocation/error/duration metrics by default, so some CloudWatch data almost certainly exists even if not explicitly engineered) before claiming deep CloudWatch usage.

---

## Cluster 1: Native AWS Metrics, Logs, Alarms

### What is it?
CloudWatch automatically collects basic metrics (invocations, errors, duration, throttles) for most AWS services, including Lambda, with zero extra setup — every Lambda invocation's logs also land in CloudWatch Logs by default. CloudWatch Alarms trigger actions (notifications, auto-scaling) when a metric crosses a threshold.

### Interview Questions
- **L1:** "CloudWatch automatically collects basic operational metrics and logs for AWS services like Lambda with no extra setup — Alarms trigger on threshold breaches, e.g., for notifications or auto-scaling."
- **L2:** explain honestly, if asked about Hermes specifically: baseline Lambda metrics (invocation count, error count, duration, throttles) are captured by CloudWatch automatically regardless of whether you built dashboards or alarms around them — so "does Hermes use CloudWatch" has a nuanced true answer (yes, at minimum, by default) even if you didn't build custom monitoring on top of it. Be precise about which of these is actually true for your project rather than either overclaiming custom CloudWatch engineering or denying any CloudWatch involvement at all.

### Comparison With Alternatives
CloudWatch (AWS-native, zero-setup baseline, tightly integrated with AWS services) vs. Prometheus/Grafana (self-hosted or managed, more flexible querying via PromQL, used explicitly in your Task Scheduler) — a reasonable "why different observability stacks for different projects" answer: Hermes's AWS-native services get baseline CloudWatch essentially for free, while the Task Scheduler's self-managed Docker-based services needed you to stand up Prometheus/Grafana yourself since there's no AWS-managed equivalent watching non-AWS infrastructure.

### Quick Revision
- Lambda gets baseline CloudWatch metrics/logs automatically — true by default, regardless of extra engineering effort.
- Be precise about "used CloudWatch" (true, at minimum, by default) vs. "built CloudWatch dashboards/alarms" (verify before claiming).
