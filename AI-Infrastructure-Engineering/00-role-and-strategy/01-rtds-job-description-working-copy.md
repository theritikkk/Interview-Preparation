# RTDS Intern Infrastructure Engineering — Job Description Working Copy

> **Status:** working copy reconstructed from the requirements in your message. The original attachment is not in this workspace. Do not treat this as the literal RTDS document. If you have the exact JD, replace this file and re-check the gap table.

## Role snapshot

| Field | Reading |
|---|---|
| Company | Real Time Data Services (RTDS) |
| Role | Intern — Infrastructure Engineering |
| Domain | Private cloud / managed infrastructure / data-centre operations |
| Stack named in the JD | RHEL/RHOSO, OpenStack, KVM/QEMU, Linux, networking, Ansible, Prometheus/Grafana, Docker/Kubernetes/OpenShift, CI/CD, storage |
| Nature of work | Operate and troubleshoot infrastructure; deploy, configure, validate; write automation and runbooks |
| Entry point | Online assessment → technical interview → likely ops/scenario discussion |

The role is **not** a frontend or application-development internship. The centre of gravity is:

```text
Linux administration
    + networking
    + virtualization (KVM/QEMU)
    + private cloud (OpenStack on RHEL)
    + automation (Bash/Python/Ansible)
    + monitoring and root-cause analysis
```

Docker/Kubernetes, storage, security, and CI/CD are supporting skills. They matter, but they are not a substitute for Linux and networking.

---

## Requirements as stated, decoded

The left column is the requirement from the JD summary. The middle column is what an interviewer can actually test. The right column is the evidence that would prove it.

| JD requirement | What it is really testing | Evidence you should be able to produce |
|---|---|---|
| Linux command-line proficiency | Can you navigate and inspect unfamiliar systems without a GUI? | A transcript: find large files, identify a process, inspect a socket, read a service log |
| Package management with `dnf`, `yum`, `apt` | Do you understand repositories, dependencies, GPG keys, versions, and rollback? | Install/pin/downgrade a package; explain what happens when a repo is unreachable |
| Users, groups, permissions, filesystems | Can you reason about access control and storage layout? | Fix a permission failure without `chmod 777`; explain ACLs, umask, SUID, inodes |
| Process management | Can you tell what a process is doing and why it is stuck? | Read `ps` state, `D` state, zombies, load average, cgroup limits, signals |
| `systemd` and service management | Can you own a service lifecycle? | Write a unit, use a drop-in, debug a failed start, use a timer |
| Log rotation, disk space, system health | Can you prevent the next outage? | Find deleted-open files, fix inode exhaustion, configure `logrotate`, set a disk alert |
| Network interfaces, bonding, VLANs, firewalld, iptables | Can you configure and debug host networking? | Add a VLAN, bond two NICs, write a firewall rule, prove it with `tcpdump` |
| Log analysis with `journalctl` and `/var/log` | Can you reconstruct an incident from logs? | A timeline with timestamps, units, PIDs, and correlation across files |
| Cloud and private cloud | Do you understand the abstraction, not just the console? | Explain tenancy, virtualization, SDN, block storage, quota, failure domains |
| OpenStack on RHEL/RHOSO | Can you map a VM request to Nova, Neutron, Cinder, Glance, Keystone? | Draw the VM creation flow; read `openstack server show` and `nova instance-action` |
| KVM and QEMU | Do you know where the VM actually runs? | Explain `libvirt` → QEMU → KVM; inspect a domain XML; find a VM's disk and tap device |
| Compute/network/storage troubleshooting | Can you isolate which layer failed? | Evidence table for an `ERROR` instance or a volume that will not attach |
| IP addressing and subnetting | Can you calculate and debug addressing quickly? | Subnet a /24 into /26s; explain why two hosts cannot communicate |
| DNS, DHCP, routing, load balancing | Can you follow name → address → route → socket? | `dig`, `getent`, `ip route get`, `ss`, `curl`, `tcpdump` chain |
| VLANs, interfaces, firewall rules | Can you implement segmentation safely? | A working trunk/access port lab; a default-deny firewall with one allowed service |
| Bash scripting | Can you automate safely, not just concatenate commands? | A script with `set -euo pipefail`, quoting, traps, exit codes, and tests |
| Python scripting | Can you parse structured data and handle failures? | A health-check script with timeouts, retries, JSON output, and logging |
| Ansible playbooks | Do you understand idempotency and configuration management? | A role that converges twice with no changes on the second run |
| Prometheus and Grafana | Can you design a signal, not just a dashboard? | A PromQL query, an alert rule with a `for` duration, a runbook link |
| System logs and metrics | Can you correlate cause and effect? | An incident timeline with metric + log evidence |
| Root-cause analysis | Can you separate symptom, cause, and contributing factor? | A postmortem with a causal chain and a prevention item |
| Alerting rules and runbooks | Can you make an alert actionable? | An alert that says what to check and what to do, not just "CPU high" |
| Docker and Kubernetes/OpenShift | Can you operate containers, not just build them? | Debug a Pending/CrashLoop/no-endpoints pod; explain namespaces/cgroups/CNI |
| CI/CD with GitHub Actions/Jenkins/GitLab CI | Can you validate infrastructure changes automatically? | A pipeline that lints, tests, and validates an Ansible/script change |
| NFS, iSCSI, Ceph, NVMe-oF, S3/Swift | Do you understand storage protocols and failure modes? | Explain block vs file vs object; mount NFS; attach an iSCSI LUN; read Ceph health |
| Cloud and infrastructure security | Can you apply least privilege and isolation? | IAM policy, security group, RBAC, service account, secrets handling |
| Distributed systems fundamentals | Can you reason about partial failure, retries, queues, consistency? | Explain your task scheduler's delivery semantics and failure modes |

---

## What the JD emphasises (working weights)

These weights are an inference from the requirement list, not a claim about the OA blueprint. They drive the priority column in the gap analysis.

| Area | Weight | Why |
|---|---:|---|
| Linux administration | 25% | Named first and in the most detail; every other layer depends on it |
| Networking | 20% | Subnetting, DNS, DHCP, routing, VLANs, firewalls, bonding are all explicit |
| Virtualization + private cloud | 20% | KVM/QEMU + OpenStack on RHEL/RHOSO is the differentiator of this role |
| Automation | 10% | Bash, Python, Ansible, CI/CD |
| Monitoring + troubleshooting | 10% | Prometheus/Grafana, logs, RCA, runbooks |
| Containers/Kubernetes | 10% | Docker, Kubernetes/OpenShift; supporting, not the centre |
| Storage + security | 5% | NFS/iSCSI/Ceph/NVMe-oF/S3/Swift, least privilege |

> Implication: do not spend 60% of your preparation on Kubernetes because it feels familiar. The assessment is more likely to ask why a host is unreachable, why a disk is full, or how a packet gets from a VM to the internet.

---

## Day-to-day work this role implies

A realistic week might include:

- A tenant reports a VM is unreachable. You check the instance state, port binding, security group, router, floating IP, and the host bridge — in that order, with evidence.
- A compute node reports `No valid host was found`. You check placement inventory, aggregate membership, CPU/RAM overcommit, and scheduler filters.
- A service fails after a package update. You compare the unit file, drop-ins, environment, and journal; roll back if needed.
- A disk fills at 03:00. You find a log file, a deleted-but-open file, or a runaway container; you fix the cause and add a monitor.
- A new cluster needs a playbook. You write an idempotent Ansible role, test it in check mode, then run it against staging.
- A dashboard is green but users are unhappy. You add a latency/queue metric and rewrite the alert around the symptom.

The common thread is **not** "know every command". It is "form a hypothesis, choose the cheapest test, and read the evidence correctly".

---

## Online assessment: what we know and what we assume

We do **not** know the RTDS OA format. Do not memorise a guessed pattern.

| Known | Unknown | How the preparation handles it |
|---|---|---|
| The role is infrastructure; Linux and networking are explicit | Number of questions, duration, negative marking | Three timed mocks with different formats |
| The JD lists command-line, subnetting, troubleshooting, OpenStack, automation | MCQ vs command-output vs scenario vs coding | Question bank includes all four |
| The first round is an online assessment | Proctoring, webcam, calculator policy | Practice without external help; learn to do subnet math on paper |
| It is entry-level/internship | Depth of OpenStack/K8s questions | Prioritise P0/P1; OpenStack at architecture/lifecycle level |

**Assumption used in the mocks:** 60 minutes, 30–40 questions, mixed MCQ and short answer, no coding IDE. This is a training assumption, not a claim about RTDS.

---

## What I will not assume about your background

| You have used | What that proves | What it does not prove |
|---|---|---|
| Docker | You can build and run containers | You understand namespaces, cgroups, overlayfs, veth, or CNI |
| Kubernetes | You can deploy manifests | You can explain the API server, scheduler, kubelet, or debug a Pending pod |
| AWS | You can launch resources | You can design a VPC, explain SG vs NACL, or debug a private-subnet route |
| Terraform | You can declare resources | You can reason about state, drift, provider auth, or blast radius |
| Prometheus/Grafana | You can instrument a service | You can design an SLO, write a useful alert, or capacity-plan |
| Linux terminal | You are comfortable in a shell | You can administer users, disks, services, and networking on RHEL |
| Distributed systems project | You understand queues and retries | You can operate the infrastructure it runs on |

This distinction is the core of the gap analysis. It is not a criticism; it is how you avoid studying things you already know and missing the things the assessment actually tests.
