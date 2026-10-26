# Flagship Project — OpsForge

> **One-line pitch:** a reproducible Linux/Kubernetes platform built with Ansible, observed with Prometheus, and **proven by failure drills** — one command to build it, one command to break it on purpose, one runbook to fix it.
>
> **Why this project exists:** most student projects show that you can build the happy path. Infrastructure roles are about the other 99% — what breaks, how you know, and how you fix it. OpsForge is designed so its README can say: *"I broke it five ways; here is the evidence and the runbook."*

## The pitch you'll give in interviews

> "OpsForge is a platform I built to practice operations the way production works. One Ansible playbook provisions a disposable Linux VM — users, SSH hardening, firewall, time sync, packages. From that VM, a second playbook brings up a kind Kubernetes cluster with a demo app, PVC, and ingress, plus a full observability stack: Prometheus, Grafana, Alertmanager, and node_exporter. Five alerts are wired to a local webhook with runbook links. The part I'm proudest of is the failure drills: five deliberate faults — kill a pod, fill a disk, throttle CPU, drop DNS, break an alert path — each with the evidence, the diagnosis, and the fix documented in the repo. Convergence is idempotent: the second run changes nothing."

Every sentence there is defensible because the repo contains the evidence.

## Deliverables and success criteria

| # | Deliverable | Success criterion | Evidence artifact |
|---|---|---|---|
| D1 | Provisioning playbook (VM) | Bare VM → configured in one command; re-run reports `changed=0` | `vagrant/` or cloud-init + `ansible/site.yml`, idempotence logs |
| D2 | kind cluster bootstrap | Playbook installs kind, creates a cluster, deploys the demo app | `k8s/` manifests, `kind get clusters` output |
| D3 | Demo app with real shape | Deployment + Service + Ingress + PVC; readiness/liveness; resource requests | `k8s/app/` manifests |
| D4 | Observability stack | Prometheus scraping node + app; Grafana provisioned from config; Alertmanager routing to a local webhook | `observability/` configs, screenshot of Grafana |
| D5 | Alert set | 5+ alerts, each with severity, runbook link, and a `promtool test` | `observability/rules/*.yml`, `promtool test rules` passing |
| D6 | Failure drills | 5 faults, each: repro command, evidence, diagnosis, fix, prevention | `drills/DRILL-0x.md` |
| D7 | Automation UX | `make up`, `make down`, `make drill-01`... `make verify` all work from clean state | `Makefile` + recorded output |
| D8 | Documentation | README with architecture diagram, quickstart, and a "at 10× scale" section | `README.md`, `docs/` |
| D9 | Cleanup and safety | Teardown returns the machine to a clean state; no secrets committed; cost-aware notes for any cloud usage | `scripts/teardown.sh`, `.gitignore` |

**Total time estimate:** 25–40 hours spread over 2–4 weeks. Do not build it in one sitting; the drills require the platform to be *stable* first.

## Repository layout

```text
opsforge/
├── README.md                  pitch, quickstart, architecture, drills index
├── Makefile                   up / down / verify / drill-0X
├── ansible/
│   ├── site.yml               VM provisioning entry point
│   ├── inventory/             lab hosts
│   └── roles/
│       ├── common/            packages, time sync, users
│       ├── sshd/              hardening (with a rollback note!)
│       ├── firewall/          nftables/ufw allow-list
│       └── kind/              kind install + cluster create + app deploy
├── k8s/
│   ├── app/                   deployment, service, ingress, pvc, hpa
│   └── monitoring/            namespace, kube-prometheus values or plain manifests
├── observability/
│   ├── prometheus.yml         scrape config
│   ├── rules/                 alert rules (+ rules tests)
│   ├── alertmanager.yml       local webhook receiver only
│   └── grafana/               dashboards + datasources as provisioned files
├── drills/
│   ├── DRILL-01-pod-kill.md
│   ├── DRILL-02-disk-full.md
│   ├── DRILL-03-cpu-throttle.md
│   ├── DRILL-04-dns-failure.md
│   └── DRILL-05-alert-path.md
├── scripts/
│   ├── up.sh / down.sh / verify.sh
│   └── load.sh                a small load generator for the demo app
└── docs/
    ├── architecture.md        with the diagram
    └── at-ten-x.md            the scale section
```

## The five drills (the heart of the project)

Each drill document follows the same shape (mirroring the phase lab template):

```text
## Fault            what you break, exactly
## Reproduce        the one command that breaks it
## Expected page    which alert fires, after how long
## Evidence         commands + captured output (before touching anything)
## Diagnosis        the reasoning from evidence to cause
## Fix              the smallest safe change
## Verification     the check that proves the cause is gone, not just the symptom
## Prevention       the guard/alert/test that would have caught it earlier
## Runbook snippet  the exact steps a stranger would follow at 3 a.m.
```

### DRILL-01 — Kill the app pod

- **Fault:** `kubectl delete pod -l app=demo` (or crash it with an env flag).
- **Expected page:** `AppTargetDown` or `KubePodCrashLooping` after ~1 min.
- **The lesson:** what a Deployment actually protects against (pod loss) and what it doesn't (bad config, dependency loss); why readiness matters during recovery.
- **Evidence to capture:** pod events, restart timing, alert firing/resolution timestamps.

### DRILL-02 — Fill the disk

- **Fault:** `fallocate -l <size> /var/lib/docker-fill` (on the VM, not your laptop) until the kubelet reports DiskPressure, or fill the app's PVC until writes fail.
- **Expected page:** `NodeFilesystemAlmostFull` / `KubePersistentVolumeFillingUp`.
- **The lesson:** disk alerts must fire *before* ENOSPC; the inode vs bytes two-budget lesson; safe cleanup ordering.
- **Evidence:** `df -h`, `df -i`, kubelet events, eviction timeline.

### DRILL-03 — Throttle the CPU

- **Fault:** run a busy-loop pod with CPU limits set low (`limits.cpu: 100m`) and load the app.
- **Expected page:** latency SLO burn-rate alert (not CPU — you page on the symptom).
- **The lesson:** CPU limits cause throttling; `container_cpu_cfs_throttled_periods_total` is the evidence; why you alert on latency, not CPU.
- **Evidence:** throttling metric, latency histogram, load-generator p95.

### DRILL-04 — Break DNS

- **Fault:** deploy a broken CoreDNS config in the kind cluster (or point the app at a bogus resolver).
- **Expected page:** app error-rate alert; DNS resolution failures in logs.
- **The lesson:** DNS failure modes inside Kubernetes (ndots, search domains, CoreDNS health); why "it resolves on my machine" is not evidence.
- **Evidence:** `kubectl logs -n kube-system coredns-*`, app error logs, a dig from inside the cluster.

### DRILL-05 — Break the alert path

- **Fault:** stop Alertmanager (or point it at a dead webhook).
- **Expected page:** **nothing** — that is the lesson.
- **The lesson:** "firing in Prometheus ≠ human notified"; the `AlertmanagerDown`/notification-failure alert that catches the catcher; test the path with a synthetic alert.
- **Evidence:** Prometheus alert state vs Alertmanager API state vs webhook logs; the synthetic canary alert's absence.

## The Makefile contract

```makefile
up:          ## provision VM (or assume VM), bootstrap kind, deploy stack
down:        ## destroy kind cluster and optionally the VM
verify:      ## run the acceptance checks (see below)
drill-01:    ## run drill 1 and tail the webhook
...
smoke:       ## curl the app, check alerts are green
```

`make verify` runs an acceptance script that asserts, in order:

1. `kubectl get nodes` shows the expected count, all Ready.
2. The demo app returns 200 through the ingress.
3. Prometheus `up == 1` for all expected targets.
4. All 5 alert rules exist and their tests pass (`promtool test rules`).
5. The webhook receiver is reachable and can accept a test alert.

A `make verify` that passes is your demo's opening slide; a drill that fires is the closing one.

## The "at 10× scale" section (what interviewers love)

Write ~300 words on what breaks at scale and what you'd change:

- **Observability:** single Prometheus won't scale → federated/sharded Prometheus or Thanos/Mimir, remote storage, retention tiers.
- **Alerts:** static thresholds → SLO-based multiwindow burn; alert review process to keep precision.
- **Cluster:** single kind → multi-node with taints/pools, HPA, PodDisruptionBudgets, and a real CNI with NetworkPolicy enforcement (tested, not assumed).
- **Provisioning:** one VM → fleet with per-role groups, secrets management, staged rollouts, drift detection.
- **The honest limits:** what this lab cannot teach (real multi-tenant security, real on-call, real billing) — naming them is a credibility signal, not a weakness.

## Safety and scope rules

- **Never** run the hardening role against your laptop or a machine you care about; the project assumes a disposable VM.
- **Never** commit secrets; use placeholders and document where real values would come from (a vault, CI variables).
- The Alertmanager receiver is a **local webhook only** — never point drills at email/Slack/pager channels.
- Any cloud usage is optional and cost-capped: prefer the local VM; if you use a cloud VM for the "real cloud" variant, set a budget alert and put teardown in the Makefile.
- Keep the scope tight: 5 good drills beat 15 half-documented ones.

## Build order (recommended)

| Week | Milestone |
|---|---|
| 1 | D1 + D2: VM playbook idempotent; kind up with the demo app |
| 2 | D3 + D4: app shape + observability stack, `make verify` green |
| 3 | D5: alert rules with tests; DRILL-01 and DRILL-05 (the two that teach the most) |
| 4 | DRILL-02/03/04; polish README, record the demo, write `at-ten-x.md` |

The demo itself is 10 minutes: `make up` (timed), `make verify` (green), one drill fired live with the alert page shown, `make down`. Practice it three times before using it in an interview.

## Stretch goals (only after v1 is solid)

- A second environment (staging/prod) with different alert thresholds — demonstrates environment-aware config.
- An SLO burn-rate alert set with `promtool` tests (from Phase 8) replacing the static CPU alert.
- A GitOps loop: the deploy job renders and applies manifests from a git repo (Phase 7's optional stretch).
- The AI extension: a small inference service in the cluster with token/latency metrics and a cost panel (Phase 9's L9.5 serving metrics, in-cluster).
