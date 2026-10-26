# Phase 8 — Observability and Reliability

> **Why this phase matters:** infrastructure is not "up" because a process is running. It is healthy when users can do what they came to do, within an agreed latency and error budget, and when you can prove it from signals rather than hope. This phase turns metrics, logs, traces, alerts, SLOs, incidents, and security controls into one operating loop.

## Outcomes

By the end of this phase you can:

1. Choose metrics, logs, traces, and events for a question instead of collecting everything.
2. Explain pull vs push, scrape intervals, retention, cardinality, and remote write.
3. Instrument a service and a Linux host; write PromQL for rate, error ratio, latency percentiles, saturation, and absence.
4. Build a Grafana dashboard that answers an operational question in under 30 seconds.
5. Write a symptom-based alert with a runbook, severity, grouping, inhibition, and a silence policy.
6. Define an SLI/SLO, calculate an error budget, and alert on multiwindow burn rate.
7. Run an incident with roles, a timeline, evidence preservation, communication, and a blameless postmortem.
8. Do a capacity estimate with headroom and a queueing sanity check.
9. Audit a Linux host and a Kubernetes namespace for least privilege, exposed services, weak TLS, and secret leakage.
10. Explain how observability and security controls fail, not just how to install them.

## Prerequisites

- Phase 1 Linux: processes, memory, disks, systemd, logs.
- Phase 2 networking: HTTP, TLS, DNS, TCP.
- Phase 4 Kubernetes: Pods, Services, probes, `kubectl` evidence.
- Phase 7 automation: a safe script, a pipeline, and artifact promotion.
- A Linux VM (4 vCPU/8 GB is enough) with Docker/Compose **or** local binaries for Prometheus, Grafana, Alertmanager, and `node_exporter`.
- Optional: a kind cluster for the Kubernetes alert/security exercises.

## Environment and safety

| Lab | Needs | Cost/safety |
|---|---|---|
| L8.1 Metrics and PromQL | Linux VM, Docker Compose or binaries, `node_exporter`, a sample app | Free; local ports only |
| L8.2 Dashboards and alerts | L8.1 stack, Grafana, Alertmanager, a load generator | Free; do not send test alerts to a real on-call channel |
| L8.3 SLO and incident drill | L8.1 stack, `curl`/`hey`/`wrk`, a notebook | Free; inject failure only into the lab service |
| L8.4 Security audit | Disposable VM; optional kind cluster; `lynis`, `trivy`, `kube-bench` | Free; do not run hardening against your laptop or production |

> **Safety:** L8.2 can send real email/Slack messages if you configure a real receiver. Use a local webhook/`null` receiver for the lab, or a dedicated test channel. L8.4 changes SSH, sudo, firewall, and RBAC settings; snapshot the VM, keep a console open, and test one change at a time. A hardening script that locks you out is a failed lab, not a rite of passage.

## Study order

| Order | Note | Key question |
|---|---|---|
| 1 | [Observability and Prometheus](01-observability-and-prometheus.md) | What question does this signal answer, and what does it cost? |
| 2 | [Grafana, dashboards, and alerting](02-grafana-dashboards-and-alerting.md) | Can a tired human act on this in two minutes? |
| 3 | [SLOs, error budgets, capacity, and incidents](03-slos-incidents-and-capacity.md) | What does "healthy" mean numerically, and what do we do when it is not? |
| 4 | [Infrastructure security](04-infrastructure-security.md) | Who can reach this, what can they do, and how would I know? |
| 5 | [Labs part 1](07-labs-part1.md) | Can I instrument, query, dashboard, and alert? |
| 6 | [Labs part 2](08-labs-part2.md) | Can I run an SLO breach and audit a host without breaking it? |
| 7 | [Checkpoint](09-checkpoint.md) | Can I defend the design and diagnose the failure? |

## The operating loop

```text
        ┌───────────────┐
        │   Users       │
        └───────┬───────┘
                │ request
                ▼
        ┌───────────────┐        ┌────────────────┐
        │   Service     │───────►│ Metrics/Logs/  │
        │               │        │ Traces/Events  │
        └───────┬───────┘        └────────┬───────┘
                │                         │
                ▼                         ▼
        ┌───────────────┐        ┌────────────────┐
        │   SLO/error   │◄───────│ Dashboards +   │
        │   budget      │        │ alerts         │
        └───────┬───────┘        └────────┬───────┘
                │                         │
                ▼                         ▼
        ┌───────────────┐        ┌────────────────┐
        │  Incident     │───────►│  Postmortem +  │
        │  response    │        │  prevention    │
        └───────────────┘        └────────────────┘
```

The loop is the point. A dashboard nobody opens, an alert nobody actions, and an SLO nobody reviews are decoration. Each artifact must change a decision.

## Signal selection

| Question | Best signal | Why not the others |
|---|---|---|
| Is the service fast enough? | Histogram latency metric | Logs are sampled/expensive; traces are sampled |
| Why did this one request fail? | Trace + correlated logs | Metrics are aggregated |
| Is the disk filling? | Gauge metric + filesystem stats | Logs only tell you after the fact |
| Who changed what? | Audit/event log | Metrics cannot identify a person |
| Is the error budget burning? | SLO ratio over windows | A raw error count lacks the denominator |
| Is the host saturated? | CPU/load/memory/IO/queue metrics | A single "CPU %" hides wait and steal |
| Is the dependency down? | `up`, probe success, error ratio | A process check may pass while requests fail |

Start from the question. If you cannot name the decision the signal will change, do not collect it.

## Principles

1. **Symptom-based alerting:** page on user-visible symptoms (error rate, latency, availability), not on every cause (CPU, disk, pod restart). Cause-based alerts belong on a dashboard or a ticket.
2. **Every alert has an owner, a runbook, and a severity.** If it has none, it is noise.
3. **Denominators matter:** "500 errors" is not an SLI. "500 errors / valid requests" is.
4. **Cardinality is a cost:** every unique label combination is a time series. Do not put user IDs, request IDs, raw paths, or unbounded error strings in labels.
5. **Dashboards answer questions:** the top row tells you if users are affected; the next rows narrow the fault domain. A wall of 40 graphs is not observability.
6. **Logs are for details, not metrics:** log structured events with request/resource IDs; do not compute rates by grepping logs unless you have no alternative.
7. **SLOs are contracts, not vanity:** choose a user journey, a good/total ratio, a window, and a consequence for missing it.
8. **Incidents are run, not improvised:** assign roles, stabilize first, preserve evidence, communicate on a cadence, then fix and prevent.
9. **Security is a property of the whole path:** identity, network, host, image, secret, and audit. A firewall cannot fix an over-privileged token.
10. **Test the observability and the rollback:** break the service, confirm the alert fires and the runbook works. An untested alert is a hypothesis.

## Completion criterion

You can:

- Explain why a counter needs `rate()` and why `irate()` can mislead.
- Write a PromQL query for error ratio and p95 latency, and explain the denominator and window.
- Build a dashboard whose first row answers "are users affected?"
- Write an alert that fires for a real symptom, includes a runbook, and does not fire for a single flake.
- Compute an error budget and a burn rate, and explain the multiwindow rule.
- Run an incident drill with a timeline and a blameless postmortem.
- Audit a host for listening ports, SSH exposure, sudo rights, file permissions, and a weak TLS configuration.
- Explain the blast radius of a compromised CI token or a privileged pod.

Then take [the checkpoint](09-checkpoint.md).
