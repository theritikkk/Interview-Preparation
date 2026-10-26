# Mock Assessment 2 — Automation, Observability, and OpenStack

> **Format:** 75 minutes, closed book. 100 points: rapid questions (40), calculations (20), scenarios (40). Target: **≥75**.
>
> **Scope:** Python/Bash/Ansible (P7), Prometheus/SLOs/incidents (P8), OpenStack services (P6). This corresponds to syllabus weeks 13–16.

---

## Part A — Rapid questions (20 × 2 = 40 points)

1. What does idempotence mean for an Ansible task, and how do you prove it in a lab?
2. What do `--check` and `--diff` each show, and what does neither prove?
3. When does an Ansible handler run, and what happens if two tasks notify the same handler?
4. What does `become: true` do, and what is the least-privilege alternative to blanket root?
5. Why pin package versions in playbooks instead of `state: latest`?
6. What does `set -euo pipefail` do, and what bug class does it catch?
7. In Bash, why does a counter inside `cmd | while read x; do ...; done` not persist?
8. What is a Python virtualenv, and what do you lock in it?
9. What does `xargs -P 4` do, and what does it break?
10. What is a GitHub Actions OIDC trust policy, and which two claims would you restrict?
11. What is an environment approval gate, and what does it enforce?
12. What does `terraform plan` show, and why is it not proof?
13. What are the four Prometheus metric types?
14. What is the difference between `rate()` and `irate()`?
15. What does `absent()` catch that a plain query cannot?
16. What is cardinality, and name two label values that explode it?
17. Why is `avg(quantile_over_time(...))` across instances invalid?
18. What is the difference between a symptom alert and a cause alert?
19. In OpenStack, which service owns instance scheduling, and which owns IP addressing?
20. What does `openstack volume service list` show, and why does it matter for attach failures?

## Part B — Calculations (5 × 4 = 20 points)

### B1 — Error budget and burn rate

A service has a 99.9% availability SLO over a 28-day window. Over the first 7 days it records 4,000 failed requests out of 2 million total.

1. What is the error budget in minutes?
2. What is the observed error ratio?
3. What is the burn rate?
4. At this burn rate, when would the 28-day budget be exhausted?

### B2 — Series and storage

A Prometheus scrape covers 60 pods × 12 routes × 4 status classes for a request counter, plus a latency histogram with 8 finite buckets (plus `+Inf`, `_sum`, `_count`) per pod × route. Scrape interval 15 s, 1.5 bytes per compressed sample.

1. How many counter series?
2. How many histogram series (including `+Inf`, `_sum`, `_count`)?
3. Total series and samples/second?
4. If retention is 15 days, what is the rough compressed storage?

### B3 — Little's law and capacity

A service receives 250 req/s. Average latency is 180 ms; the p99 is 900 ms. Each request uses 30 ms CPU and 80 MiB RAM.

1. Average concurrency?
2. If each pod has 2 vCPU and 4 GiB and targets 60% CPU utilization, how many pods by CPU?
3. How many pods by memory (at the 60% target)?
4. Which resource binds, and what is the per-pod request rate at that count?

### B4 — Alert math

A monitoring check runs every 60 s. Per-run false-positive probability is 0.8%, independent. The alert requires 2 consecutive failures.

1. Probability that any single pair of consecutive runs is both false?
2. Expected false 2-run events per day (approximate, treating overlapping pairs as independent)?
3. Minimum detection delay for a real failure that starts just after a check, with the 2-consecutive rule?
4. What is the precision/recall tension here, in one sentence?

### B5 — OpenStack capacity

A compute cell has 3 nodes, each with 64 vCPU, 256 GB RAM, and overcommit ratio CPU=4:1, RAM=1.5:1. Each instance uses 4 vCPU and 8 GB.

1. Effective CPU slots per node (4:1)?
2. Effective RAM slots per node (1.5:1)?
3. Instances per node (binding resource)?
4. Cluster total instances, and what does the overcommit ratio trade away?

## Part C — Scenarios (4 × 10 = 40 points)

For each: evidence first (3), two likely causes (2), smallest safe next action (3), prevention (2).

### C1 — The playbook that "works" but leaves drift

An Ansible playbook runs green (all `ok`), but a config file on some hosts has an older timestamp than expected, and behavior differs between hosts. The playbook uses `copy` for the file with a `notify` handler. Some hosts were hand-edited last month.

### C2 — The alert that fired at 3 a.m. for a non-issue

A "HighCPU" alert (>90% for 10 min) pages nightly at 03:00 for 12 minutes. The service is healthy; error rates and latency are normal. The engineer wants to delete the alert.

### C3 — The SLO that is green while users complain

The availability SLI is `non-5xx / all requests` measured server-side, SLO 99.9%, currently 99.97%. Support tickets say the app is slow and sometimes stuck. Latency p95 is 1.2 s (target: 1 s).

### C4 — A new OpenStack instance cannot get an IP

A newly launched VM boots but has no IP. The port exists; security groups allow the traffic. Old instances on the same network work. The DHCP agent was restarted recently.

---

# ANSWER KEY — Mock 2

## Part A

1. Re-running changes nothing further. Prove it: converge twice; the second run must report `ok` (no `changed`) for every task.
2. `--check` simulates without changes; `--diff` shows the would-be file/config diffs. Neither proves the real run is safe — the world can change, and some effects only exist at apply time.
3. At the end of the play, once per host, only if notified. Multiple notify calls deduplicate to a single handler run.
4. Escalates to another user (usually root). Least privilege: `become_user` with narrowly scoped sudo rules, separate admin identity, and no shared root.
5. `latest` hides the version in use and may upgrade unpredictably, breaking reproducibility and rollback; a pinned version makes state explicit.
6. Exit on error, error on unset variables, fail the pipeline if any stage fails. Catches silent partial failures and unset-variable surprises.
7. The `while` runs in a subshell (part of the pipeline); variables set there don't persist in the parent. Use process substitution, a temp file, or `lastpipe`.
8. An isolated interpreter + site-packages per project. Lock the resolved dependency versions (a lock file or `pip freeze`).
9. Runs up to 4 processes concurrently. Breaks order-dependent, stdin-consuming, or result-aggregating commands.
10. The trust policy that lets GitHub exchange an OIDC token for cloud credentials. Restrict at minimum the repository (`sub`/repo claim) and the branch/ref or environment.
11. A named environment requiring human approval (and optionally protected branches) before the deploy job runs — enforces a human gate before production.
12. The diff between declared config and actual state. Not proof: the world changes between plan and apply, and some effects only appear at apply time.
13. Counter, gauge, histogram, summary.
14. `rate()` averages across the window and handles resets (alert-friendly); `irate()` uses the last two samples (instantaneous, spiky).
15. A series that has disappeared entirely (target gone, metric renamed, exporter dead) — normal comparisons return "no data" silently.
16. The number of unique label-value combinations (series). Raw paths/URLs and per-request IDs are the classic explosions.
17. Quantiles are not additive; the average of per-instance quantiles describes no real distribution. Aggregate buckets first, then compute one quantile.
18. A symptom alert reflects user-visible conditions (latency/errors); a cause alert reflects a mechanism (CPU, queue, connection count). Page on symptoms; keep causes as diagnostics.
19. Nova owns scheduling/placement; Neutron owns networking (IPs via DHCP, ports, security groups).
20. Per-host cinder service state (binary/host/zone/status/state). A down `cinder-volume` in the volume's zone is the classic "No valid host" cause for attach failures.

## Part B

### B1

1. `0.1% × 28 days = 0.001 × 40,320 = 40.32 minutes`.
2. `4,000 / 2,000,000 = 0.2%`.
3. `0.2% / 0.1% = 2×`.
4. At 2×, the budget burns in half the window: `28 / 2 = 14 days` from the start; with 7 days elapsed, the budget is exhausted in ~7 more days if the rate persists.

### B2

1. `60 × 12 × 4 = 2,880` series.
2. Per pod×route: `8 + 1 + 1 + 1 = 11` series → `60 × 12 × 11 = 7,920`.
3. Total = `10,800` series; samples/s = `10,800 / 15 = 720`.
4. `720 × 15 × 86,400 × 1.5 bytes ≈ 1.4 GB` — small, which is the point: cardinality discipline keeps Prometheus cheap.

### B3

1. `L = 250 × 0.18 = 45` concurrent.
2. CPU demand = `250 × 0.030 = 7.5 cores`; at 60% → `12.5 cores` → `ceil(12.5/2) = 7` pods.
3. Memory = `250 × 80 MiB = 20,000 MiB = 19.53 GiB`; at 60% target → `32.55 GiB` → `ceil(32.55/4) = 9` pods.
4. Memory binds (9 pods). Per-pod rate = `250/9 ≈ 27.8 req/s`; CPU per pod = `27.8 × 0.03 = 0.83 cores` (42% of 2 vCPU); memory per pod = `19.53/9 ≈ 2.17 GiB` (54% of 4 GiB).

### B4

1. `0.008² = 6.4e-5`.
2. Checks/day = 1,440; overlapping 2-run pairs ≈ 1,440; expected false pairs ≈ `1,440 × 6.4e-5 ≈ 0.092` — about one every 11 days.
3. The failure must persist through 2 checks: worst case just after a check → next check at ≤60 s, then one more at ≤60 s → ≈ 2 minutes (plus evaluation timing).
4. Requiring consecutive failures raises precision (fewer false pages) but adds detection delay and slightly reduces recall for very brief true outages — the threshold is a policy choice, not a math fact.

### B5

1. `64 × 4 = 256` CPU slots.
2. `256 × 1.5 = 384` RAM slots (in GB-equivalents: 256 GB × 1.5 = 384 GB).
3. CPU binds: `min(256/4, 384/8) = min(64, 48) = 48` instances per node.
4. `48 × 3 = 144` instances. Overcommit trades guaranteed performance for density: memory pressure → swap/OOM; CPU contention → throttling. It is a policy bet that not all instances peak simultaneously; batch/interactive mixes can violate that bet.

## Part C

### C1

Evidence: run the playbook with `--check --diff` on one drifted host; compare the file's content/stat against the playbook source; check the handler's condition and `when` clauses; check variable precedence (host vars, group vars, extra vars). Likely causes: variable precedence making the template/content differ per host, or a `when`/`failed_when` guard silently skipping the task, or the handler not being notified due to a `changed_when` override. Next action: make the task's change visible (`--diff`), fix the precedence/guard, then converge the drifted hosts and verify behavior (not just the run's exit code). Prevention: run a periodic check-mode diff job that alerts on drift; ban hand edits (audit via file mtime/ownership or a config-management report); test convergence on a disposable VM after every playbook change.

### C2

Evidence: what runs at 03:00 (batch, backup, index, cron), the metric's per-host breakdown, and whether 90%+ is sustained or a scrape artifact. Likely causes: a scheduled job legitimately using CPU (a cause, not a symptom), or a scrape/evaluation artifact. Next action: do not delete the alert yet — either scope it to exclude the scheduled window (or to the service's *symptom* metrics instead), or fix the schedule. The right target is a **symptom alert** (latency/errors) plus a ticket-level alert for sustained saturation. Prevention: every alert states what page it justifies; review alert precision monthly; annotate scheduled load so dashboards and alerts share the calendar.

### C3

Evidence: the SLI's denominator (does it include timeouts/stuck requests? health checks?), what "stuck" looks like in logs/traces (queue? dependency? GC?), and the latency SLO's aggregation (all routes? p95 across the whole service?). Likely causes: the SLI counts "returned 200" which includes stuck-slow-but-eventually-200 requests, and the latency SLO aggregates over all routes hiding the checkout path. Next action: add a latency-threshold SLI (fraction of requests < 1 s on the checkout route) and a synthetic probe; investigate the stuck pattern via traces (where does the time go?). Prevention: review SLIs against support tickets quarterly; every user journey gets an explicit SLI; slow-but-200 is not availability.

### C4

Evidence: the port's status and `binding:host_id`, the DHCP agent's logs for that network (`dnsmasq` state), whether `dnsmasq` is running with the right network config, and `neutron agent-list` health. Likely causes: the DHCP agent restart left the port without a DHCP binding (agent recovered but the port is in a bad state), or the DHCP agent on the right host is not listening on that network (a wiring/bridge issue). Next action: check the agent logs for that port; if the binding is stale, re-associate the port (`neutron port-update`/port re-bind) or restart the agent cleanly; verify with `dhcpdump`/tcpdump whether DHCP offers reach the VM. Prevention: alert on DHCP agent down and on ports stuck in `DOWN`/`BUILD`; canary a launch after agent restarts; document the "no IP" runbook (port state → agent logs → offers on the wire).

---

## Scoring

| Part | Your score | Max |
|---|---:|---:|
| A | | 40 |
| B | | 20 |
| C | | 40 |
| **Total** | | **100** |

**Pass ≥75.** Below 75: repair with `07-automation`, `08-observability-and-reliability`, and `06-openstack` notes + labs; the incidents in `10-troubleshooting-workbook/05-observability-incidents.md`, `06-aws-incidents.md` (B-side), and `07-openstack-incidents.md` map to Part C misses.
