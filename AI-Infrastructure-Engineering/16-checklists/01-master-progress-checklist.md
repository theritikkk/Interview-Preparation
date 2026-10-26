# Master Progress Checklist

> **The rule that makes this checklist honest:** a box is checked only when an **artifact exists** — a lab note, a command transcript, a diagram, a passing mock score, or a recorded explanation. "I read it" is not an artifact. Revisit this file weekly; it is the single source of truth for what you can claim and what you cannot yet.

## How to score yourself

| Status | Meaning |
|---|---|
| `[x]` | Artifact exists and you can demo it without notes |
| `[~]` | In progress — name the missing artifact in the margin |
| `[ ]` | Not started |

Track dates next to each checked item. When an item's date is more than ~6 weeks old and you could not demo it today, uncheck it — skills decay; the checklist should say so.

---

## P0 — Foundations (days 1–30)

### Linux (P1)

- [ ] Explain the boot process and what `systemd` does (artifact: written explanation + `systemd-analyze blame` transcript)
- [ ] Files/permissions: explain 755 vs 644, setuid, sticky bit (artifact: L1.x lab note)
- [ ] Processes and memory: read `vmstat`, `iostat -x`, `ps`, PSI (artifact: INC-01-style drill with your own capture)
- [ ] Storage: `df` vs `du`, inodes, deleted-open files (artifact: INC-02/03 drills)
- [ ] Logs and journald: query, rotate, cap (artifact: journald config + query transcript)
- [ ] Bash field manual: 20 commands with purpose/why/options/output/interpretation (artifact: your command journal)
- [ ] Labs L1.1–L1.8 complete (artifact: notes with verification sections)

### Networking (P2)

- [ ] Subnetting by hand: /24, /26, /27 (artifact: 3 worked problems, no calculator)
- [ ] TCP handshake and states; TIME_WAIT vs CLOSE_WAIT (artifact: `ss -s` capture + explanation)
- [ ] Routing and ARP: `ip route get`, neighbor tables (artifact: L2.x note)
- [ ] DNS: dig +trace, TTL, CNAME chains (artifact: a resolution trace you annotated)
- [ ] TLS: verify a cert chain with `openssl s_client`; explain verify codes (artifact: capture)
- [ ] NAT/conntrack, firewall policies (artifact: an iptables/nftables ruleset you wrote)
- [ ] Labs L2.1–L2.7 complete (artifact: notes)
- [ ] Mock 1 ≥ 75% (artifact: dated score + weak-area list)

### Automation foundations (P7-lite)

- [ ] Bash robustness: `set -euo pipefail`, quoting, traps (artifact: a script that passes a shellcheck-style review)
- [ ] 5 incident stories rehearsed aloud (artifact: recordings or notes in the story template)

### Checkpoint

- [ ] Phase 1 checkpoint passed (target score + date)
- [ ] Phase 2 checkpoint passed

---

## P1 — Kubernetes and cloud fundamentals (days 31–90)

### Virtualization and storage (P3)

- [ ] Explain VM vs container isolation, virtio, copy-on-write images (artifact: explanation + a qcow2/overlay demo)
- [ ] LVM/RAID/filesystem journaling basics (artifact: L3.x note)
- [ ] Shared storage: NFS semantics, what CRUSH does conceptually (artifact: written explanation)
- [ ] Labs L3.1–L3.4 complete

### Kubernetes (P4)

- [ ] Pod → ReplicaSet → Deployment chain explained (artifact: diagram you drew)
- [ ] Scheduling: requests/limits, taints, affinity, probes (artifact: L4.x note)
- [ ] Debug playbook: Pending, CrashLoop, ImagePull, OOM, DNS, no endpoints, NotReady (artifact: INC-08–11 drills + your debug playbook page)
- [ ] Services/kube-proxy/NetworkPolicy (artifact: an enforced NetworkPolicy test)
- [ ] Labs L4.1–L4.7 complete
- [ ] Mock 2 ≥ 75% (artifact: dated score)

### Cloud (P5-lite)

- [ ] VPC/subnets/routing/SG model explained (artifact: a drawn VPC + table)
- [ ] IAM roles/profiles/OIDC for CI (artifact: L7.4 pipeline or a written design)
- [ ] Instance types, storage, and cost arithmetic (artifact: a cost calc you can defend)

### Checkpoint

- [ ] Phase 4 checkpoint passed

---

## P2 — Operations depth (months 3–6)

### Automation (P7)

- [ ] Ansible role: idempotent, handlers, `--check --diff` (artifact: second-run `changed=0` log)
- [ ] Python for ops: virtualenv, pinning, one useful script (artifact: script + test)
- [ ] CI/CD with OIDC, environment gates, artifact integrity (artifact: pipeline run + gate screenshot)
- [ ] Labs L7.1–L7.4 complete

### Observability (P8)

- [ ] PromQL: rate/increase, error ratio, p95, histogram aggregation done right (artifact: L8.1 notes + a query set)
- [ ] Dashboard design: symptom-first rows (artifact: a Grafana screenshot that answers "are users affected?")
- [ ] Alerting: multiwindow burn, runbooks, notification path tested (artifact: firing + resolving evidence)
- [ ] SLO spec + incident drill + postmortem (artifact: your SLO doc + postmortem)
- [ ] Security audit basics: ports, SSH, sudo, RBAC, secrets (artifact: L8.4 findings fixed)
- [ ] Labs L8.1–L8.4 complete
- [ ] Mock 3 ≥ 75% (artifact: dated score)

### OpenStack (P6)

- [ ] Service map: Keystone/Glance/Nova/Neutron/Cinder ownership (artifact: the openstack-vm-flow diagram annotated by you)
- [ ] CLI evidence from a sandbox/DevStack (artifact: command transcript) or the read-only alternative
- [ ] Labs L6.1–L6.4 complete (or read-only path documented)

### Checkpoint

- [ ] Phase 6 checkpoint passed
- [ ] Phase 8 checkpoint passed

---

## P3 — AI infrastructure (months 6–9)

### GPU and distributed systems (P9)

- [ ] GPU execution/memory model; roofline classification (artifact: L9.1 write-up)
- [ ] Training loop, data path, precision, memory budget (artifact: the 13B memory calculation)
- [ ] Parallelism strategies and all-reduce math (artifact: L9.3 scaling table)
- [ ] NCCL/InfiniBand diagnostics (artifact: INC-18-style analysis or a real capture)
- [ ] GPU scheduling: device plugin, MIG vs time-slicing, Slurm GRES (artifact: L9.4 write-up + paper exercise)
- [ ] MLOps loop: tracking, registry, serving, drift, cost (artifact: L9.5 write-up)
- [ ] Capstone (Option A or B) ≥ 112/160 with drills ≥ 21/30 (artifact: capstone repo + rubric self-score)
- [ ] Phase 9 checkpoint passed

---

## Interview readiness

- [ ] Six stories in the STAR+evidence template, rehearsed aloud (artifact: recordings/notes)
- [ ] OpsForge demo rehearsed 3× (`make up` → `make verify` → one drill → `make down`)
- [ ] 60-second and 5-minute project pitches practiced
- [ ] Follow-up armor table memorized per project
- [ ] System design practice: 2 designs (one Phase 8 D-prompt, one Phase 9 D-prompt) written and self-scored
- [ ] Oral exams from Phase 8 and 9 checkpoints: pass 7/10 aloud (artifact: dates)
- [ ] Resume bullets rewritten with evidence
- [ ] GitHub repo cleaned: README with diagram, one "start here" lab, no secrets
- [ ] 5+ applications submitted with tailored notes

---

## Weekly ritual (15 minutes)

1. Check off items with artifacts; uncheck decayed ones.
2. Pick **one** unchecked item to advance this week — not three.
3. Update the schedule's log with one sentence: what moved, what is stuck and why.
4. If nothing moved for two consecutive weeks, cut scope (drop a phase item, keep DSA + labs).

## Milestone tracker

| Milestone | Target date | Actual date | Evidence |
|---|---|---|---|
| Mock 1 ≥ 75% | Day 21 | | |
| 5 applications | Day 28 | | |
| Phase 4 checkpoint | Day 60 | | |
| Mock 2 ≥ 75% | Week 13–16 | | |
| OpsForge v1 demo | Day 90 | | |
| Mock 3 ≥ 75% | Week 17–24 | | |
| First real interview | — | | |
| AI capstone | Month 6–9 | | |
| All phase checkpoints | Month 12 | | |
