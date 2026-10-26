# Prioritised Master Syllabus

> This is the whole curriculum on one page. The schedules turn it into weeks. The phase folders turn it into notes and labs.

## Design constraints

- **Time budget:** 8–10 hours/week for 12 months, with 60–90 minutes/day protected for DSA and 1–2 hours/week for core CS revision.
- **Assessment-first:** P0 is the minimum viable set for an entry-level infrastructure OA. P1 is what makes you interview-ready. P2 is what makes you useful on the job. P3 is the AI-infrastructure specialisation.
- **Depth over coverage:** every module ends with a lab and a "can I explain this without notes?" test.
- **No tool collecting:** if a tool does not map to a failure mode in the troubleshooting workbook, it is not in the core path.

## Hours and priority

| Priority | Hours | Purpose | When |
|---|---:|---|---|
| **P0** | 140 | Pass the assessment; survive Linux/networking interviews | Days 1–30 |
| **P1** | 180 | Operate and troubleshoot a real platform; RTDS-specific skills | Days 31–90 |
| **P2** | 110 | Production-level cloud, security, reliability, OpenStack operations | Months 4–8 |
| **P3** | 120 | GPU/ML platform specialisation | Months 6–12 |
| **Total** | **550** | At 10 h/week ≈ 55 weeks; at 12 h/week ≈ 46 weeks | 12 months with buffer |

These hours are **focused study + lab** time. They exclude DSA, college, and job applications.

---

## P0 — Foundations that cannot be skipped

| # | Module | Hours | Prerequisites | Labs | Mastery evidence |
|---|---|---|---:|---|---|
| P0.1 | **Linux filesystem, users, permissions, ACLs** | 20 | Basic shell | L1.1, L1.2 | Fix a permission failure without `chmod 777`; explain umask, ACL, SUID, parent-directory execute |
| P0.2 | **Processes, signals, jobs, CPU** | 15 | OS process/thread basics | L1.3 | Read `ps` states; explain D-state, zombie, load average; choose the right signal |
| P0.3 | **Memory, swap, page cache, OOM** | 15 | Virtual memory | L1.4 | Explain `free` output; distinguish leak/cache/limit; find an OOM kill in `dmesg`/journal |
| P0.4 | **Disks, inodes, filesystems, mounts, LVM** | 20 | Filesystem basics | L1.5 | Diagnose disk-full vs inode-full vs deleted-open file; extend an LV safely |
| P0.5 | **systemd, boot, timers, logs, logrotate** | 20 | Processes, filesystem | L1.6, L1.7 | Write a unit + timer; debug a failed service; configure rotation; build a log timeline |
| P0.6 | **Packages, SSH, shell text toolkit** | 15 | Linux basics | L1.8 | Pin/downgrade a package; configure SSH; answer output questions with `awk`/`sed`/`jq` |
| P0.7 | **IP addressing, CIDR, subnetting** | 15 | Binary arithmetic | L2.1 | Subnet a /24 into /26s in under 3 minutes; detect overlap; explain host/network/broadcast |
| P0.8 | **Ethernet, ARP, switching, routing** | 15 | IP/subnetting | L2.2 | Trace a packet on-link and off-link; read `ip route get`; explain ARP failure |
| P0.9 | **TCP/UDP, sockets, DNS, NAT, firewalls** | 25 | IP/routing | L2.3, L2.4, L2.5 | Diagnose refused vs timeout vs reset; read `ss`; use `dig`/`getent`; write a firewall rule |
| P0.10 | **Bash for operations** | 15 | Shell/text tools | L7.1 | Write a safe health-check script with `set -euo pipefail`, trap, exit codes, ShellCheck |
| P0.11 | **Troubleshooting method + timed mocks** | 10 | P0.1–P0.10 | INC-01…05, A1 | Score ≥ 75% on Mock 1; deliver a 5-minute incident walkthrough |

**P0 total: 140 h**

### What "done" means for P0

You can sit at a Linux VM and, without a tutorial:

- Find what is using CPU, memory, disk, and a port.
- Explain why a service failed to start using `systemctl status` and `journalctl`.
- Explain why a file cannot be read using `namei -l`, `getfacl`, `ls -l`, and the parent directory.
- Explain why a host cannot reach another host using `ip addr`, `ip route`, `ss`, `dig`, `ping`, `traceroute`, and `tcpdump`.
- Write a 30-line Bash script that fails safely.
- Answer subnetting questions on paper.

If you cannot do one of these, repeat the lab; do not move to P1.

---

## P1 — Operate and troubleshoot a real platform

| # | Module | Hours | Prerequisites | Labs | Mastery evidence |
|---|---|---|---:|---|---|
| P1.1 | **VLANs, bonding, interfaces, MTU** | 12 | P0.8 | L2.2, L2.3 | Configure access/trunk and a bridge; explain bonding modes; prove MTU with `ip -d link`, `tcpdump` |
| P1.2 | **HTTP, TLS, certificates, load balancing** | 10 | P0.9 | L2.6 | Debug a TLS chain; explain 502/503/504; configure an L7 health check |
| P1.3 | **Virtualization: KVM, QEMU, libvirt** | 20 | P0.2–P0.4, P1.1 | L3.1, L3.2 | Create/inspect a VM; explain libvirt XML, qcow2, virtio, tap device, snapshots |
| P1.4 | **Storage foundations: block/file/object, NFS, iSCSI** | 15 | P0.4, P1.3 | L3.3 | Mount NFS; attach an iSCSI LUN; explain IOPS/throughput/latency; read `iostat` |
| P1.5 | **Container internals** | 15 | P0.2–P0.4, P0.9 | L4.1, L4.2 | Build a container from namespaces/cgroups; inspect veth/bridge/NAT; explain overlayfs |
| P1.6 | **Kubernetes core architecture** | 25 | P1.5 | L4.3 | Explain `kubectl apply` end to end; trace scheduling; draw control plane/node components |
| P1.7 | **Kubernetes networking, services, DNS, ingress** | 15 | P1.6, P0.9 | L4.4 | Debug Service with no endpoints; explain ClusterIP/DNAT/CoreDNS/ingress path |
| P1.8 | **Kubernetes storage, RBAC, scheduling, resources** | 15 | P1.6, P1.4 | L4.5 | Explain PV/PVC/CSI; debug Pending due to requests/taints/PVC; explain QoS/OOM |
| P1.9 | **Kubernetes debugging playbook** | 15 | P1.6–P1.8 | L4.6, INC-08…11 | Diagnose Pending, CrashLoop, ImagePull, no-endpoints, DNS, OOM, NotReady from evidence |
| P1.10 | **Python for operations** | 10 | Python | L7.2 | Write a health-check tool with timeouts, retries, JSON output, structured logging |
| P1.11 | **Ansible and idempotency** | 15 | P0.1, P0.6, SSH | L7.3 | Role converges twice with `changed=0`; check mode; template; handler; Vault |
| P1.12 | **Prometheus, Grafana, alerting** | 15 | P0.2–P0.4, HTTP | L8.1, L8.2 | Write PromQL; add `node_exporter`; design a symptom-based alert with a runbook |
| P1.13 | **OpenStack architecture and VM lifecycle** | 20 | P1.3, P1.4, P0.9 | L6.1, L6.2 | Draw API→Keystone→Nova→Placement→scheduler→compute→Neutron/Glance/Cinder; read CLI evidence |

**P1 total: 182 h**

### What "done" means for P1

- You can break a Kubernetes cluster on purpose and diagnose it from `kubectl` evidence.
- You can explain the OpenStack VM creation flow and name the service that owns each decision.
- You can provision a small Linux service with Ansible and prove idempotency.
- You can build a dashboard and an alert that a human can act on.
- You have at least five incident stories with evidence.

---

## P2 — Production-level infrastructure

| # | Module | Hours | Prerequisites | Labs | Mastery evidence |
|---|---|---|---:|---|---|
| P2.1 | **AWS VPC, subnets, routing, SG/NACL, IAM** | 20 | P0.7–P0.9 | L5.1, L5.2 | Build a public/private VPC; explain why a private subnet needs NAT; debug SG vs NACL |
| P2.2 | **EC2, EBS, load balancers, EKS networking** | 15 | P2.1, P1.6 | L5.3, L5.4 | Explain VPC CNI, IRSA, LB controller, EBS CSI; cost-aware cleanup |
| P2.3 | **Advanced storage: Ceph, NVMe-oF** | 15 | P1.4 | L3.4 | Explain MON/OSD/CRUSH/pools; read `ceph status`; explain NVMe-oF queues |
| P2.4 | **Infrastructure security** | 15 | P0.1, P0.9, P1.8, P2.1 | L8.4 | Least-privilege IAM/RBAC; TLS; secrets; host hardening; audit |
| P2.5 | **SLOs, incident response, capacity** | 20 | P1.12, P0.11 | L8.3, INC-06/07/12 | Write an SLI/SLO; run an incident drill; produce a postmortem and capacity estimate |
| P2.6 | **CI/CD for infrastructure** | 10 | P0.10, P1.11, Git | L7.4 | Pipeline lints/tests/validates; secrets protected; rollback documented |
| P2.7 | **OpenStack operations and troubleshooting** | 20 | P1.13 | L6.3, L6.4 | Diagnose `ERROR` instance, no valid host, port binding, volume attach |
| P2.8 | **Distributed systems for infrastructure** | 10 | Existing backend knowledge | — | Explain etcd/Raft, retries, idempotency, backpressure, split brain in infra terms |

**P2 total: 125 h**

---

## P3 — AI infrastructure specialisation

| # | Module | Hours | Prerequisites | Labs | Mastery evidence |
|---|---|---|---:|---|---|
| P3.1 | **GPU architecture, CUDA, memory hierarchy** | 25 | Linux, OS, P1.3 | L9.1 | Explain SM/warp/kernel, HBM, PCIe/NVLink; profile a CPU/GPU workload |
| P3.2 | **Training vs inference, data loading, storage** | 15 | P3.1, P1.4 | L9.2 | Identify GPU idle due to data/storage; size batch/memory; measure throughput/latency |
| P3.3 | **Distributed training, NCCL, RDMA** | 25 | P3.1, P1.1, P2.1 | L9.3 | Explain DP/TP/PP, all-reduce, topology, checkpointing; read NCCL logs |
| P3.4 | **GPU scheduling: Kubernetes device plugin, MIG, Slurm** | 25 | P1.8, P3.1 | L9.4 | Schedule a GPU Pod; explain device plugin/MIG/taints; submit a Slurm job with GRES |
| P3.5 | **MLOps: MLflow, model serving, monitoring, cost** | 25 | P3.3, P1.12, P2.5 | L9.5 | Track an experiment; serve a model; monitor latency/GPU; estimate cost per 1k requests |
| P3.6 | **AI platform capstone** | 20 | P3.1–P3.5 | Capstone | End-to-end GPU-ready platform with runbooks, dashboards, and a cost model |

**P3 total: 135 h**

---

## The learning sequence

```text
Week 1–4     P0.1–P0.11       Linux + networking + Bash + Mock 1
Week 5–8     P1.1–P1.5        VLAN/bond/TLS + KVM + storage + containers
Week 9–12    P1.6–P1.9        Kubernetes core + networking + storage + debugging
Week 13–16   P1.10–P1.13      Python/Ansible + Prometheus + OpenStack + Mock 2
Week 17–24   P2.1–P2.8        AWS + security + SLO + CI/CD + OpenStack ops + Mock 3
Week 25–36   P3.1–P3.4        GPU + training + scheduling + Slurm
Week 37–48   P3.5–P3.6        MLOps + serving + capstone + interviews
```

This is a guide, not a contract. Slipping one week is normal. Skipping P0 is not.

---

## Minimum viable set for applying

Do not wait for the full syllabus. Apply when you have:

| Capability | Evidence |
|---|---|
| Linux administration | Can diagnose a failed service, full disk, permission error, and high load on a VM |
| Networking | Can subnet, read a route table, explain a packet path, and debug DNS/TCP/firewall |
| Containers/K8s | Can explain a container; can debug Pending/CrashLoop/no-endpoints/OOM |
| Automation | Can write a safe Bash script; can run an Ansible playbook idempotently |
| Observability | Can read a Prometheus metric and correlate it with logs |
| Communication | Can tell five incidents in Symptoms → Evidence → Hypothesis → Test → Cause → Fix → Prevention |
| Projects | One infra-focused project with a README, diagram, runbook, and failure demo |

**Estimated time to minimum viable:** 100–140 hours, i.e. 6–8 weeks at 15–20 h/week or 10–12 weeks at 10–12 h/week. Start applying at that point; continue P1/P2 while interviewing.

---

## What to deliberately not study yet

| Topic | Why not yet |
|---|---|
| Service mesh | You do not have a traffic-management problem yet; learn Services/Ingress first |
| Kubernetes operators/CRDs in depth | Learn the reconciliation loop and controllers first |
| Terraform at scale | Learn VPC/IAM and one small deployment; state/backends later |
| Ceph tuning | Learn block/file/object and a healthy Ceph cluster first |
| GPU operator internals | Learn device plugins, taints, and GPU metrics first |
| Kubeflow | Learn MLflow, serving, and scheduling first |
| Advanced eBPF | Learn TCP/IP, tracing, and performance first |
| Multi-cloud | Depth in one cloud beats breadth in three |

---

## Module completion rule

A module is complete only when all four are true:

1. You can explain the mechanism in your own words, without notes, in under 5 minutes.
2. You completed the lab and have a note with commands, outputs, and a failure you caused.
3. You can answer the checkpoint questions at ≥ 80%.
4. You can connect the module to at least one incident in the troubleshooting workbook.

Otherwise it is "read", not "learned". Mark it accordingly in the progress checklist.
