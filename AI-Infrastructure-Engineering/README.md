# AI Infrastructure Engineering — Role-Based Practice

> **Goal:** move from *"I can deploy an app with Docker and Kubernetes"* to *"I can explain, operate, and debug the Linux, network, virtualization, and orchestration layers underneath it — and later run GPU/ML infrastructure."*

This repository is a prerequisite-first curriculum built around one real target: the **Intern Infrastructure Engineering** role at **Real Time Data Services (RTDS)**, and a longer-term target of **AI Infrastructure / ML Platform Engineering**.

It is not a list of DevOps tools. It is a set of notes, labs, incident drills, and assessments that force you to reason from first principles.

---

## The contract

If you complete the P0 and P1 material honestly, you should be able to:

1. Walk into a Linux box you have never seen and answer: *what is running, what is it waiting on, what is it allowed to do, and what is broken?*
2. Trace a packet from a process to a socket, through routing, NAT, a firewall, and a load balancer — and name the command that proves each hop.
3. Explain what happens between `kubectl apply` and a running container, including the API server, etcd, scheduler, kubelet, runtime, CNI, and kube-proxy.
4. Diagnose the common Kubernetes failures from evidence: Pending, CrashLoopBackOff, ImagePullBackOff, no endpoints, DNS failure, OOMKilled, node NotReady.
5. Explain KVM/QEMU and the OpenStack VM creation path without treating OpenStack as a list of service names.
6. Write safe Bash/Python automation and an idempotent Ansible playbook.
7. Run a structured incident: stabilize, preserve evidence, bound the fault, test hypotheses, fix, verify, prevent.
8. Explain how GPU memory, PCIe/NVLink, storage, scheduling, and observability determine AI workload performance.

The first checkpoint is not "know everything". It is: **pass an entry-level infrastructure assessment and survive a technical interview.** The roadmap deliberately defines a minimum viable set.

---

## Start here

| If you have | Read | Do |
|---|---|---|
| 10 minutes | [00-role-and-strategy/02-gap-analysis.md](00-role-and-strategy/02-gap-analysis.md) | Find your top five red rows |
| 30 minutes | [00-role-and-strategy/03-prerequisite-dependency-graph.md](00-role-and-strategy/03-prerequisite-dependency-graph.md) | Follow the critical path |
| 1 hour | [00-role-and-strategy/04-master-syllabus.md](00-role-and-strategy/04-master-syllabus.md) | Pick the P0 block |
| Today | [14-schedules/01-first-30-days.md](14-schedules/01-first-30-days.md) | Put the first week in your calendar |
| This week | [01-linux-fundamentals/README.md](01-linux-fundamentals/README.md) | Start Lab L1.1 |

---

## Repository map

```text
AI-Infrastructure-Engineering/
├── 00-role-and-strategy/      JD analysis, gap table, dependency graph, master syllabus
├── 01-linux-fundamentals/     files, permissions, processes, memory, disks, systemd, logs
├── 02-networking/             subnetting, Ethernet, routing, TCP, DNS, NAT, firewalls, TLS
├── 03-virtualization-and-storage/  VMs, KVM/QEMU, Linux storage, NFS/iSCSI/Ceph/NVMe-oF
├── 04-containers-and-kubernetes/   namespaces, cgroups, images, K8s internals, debugging
├── 05-cloud-aws/              VPC, subnets, routing, IAM, EC2/EBS, EKS
├── 06-openstack/              Keystone, Glance, Nova, Neutron, Cinder, VM lifecycle
├── 07-automation/             Bash, Python, Ansible, CI/CD, secrets
├── 08-observability-and-reliability/  metrics, logs, traces, SLOs, incidents, capacity
├── 09-ai-infrastructure/      GPU architecture, CUDA, distributed training, Slurm, MLOps
├── 10-troubleshooting-workbook/    evidence-first incident drills
├── 11-assessments/            question bank + three timed mock assessments
├── 12-interviews/             technical bank, project deep dives, storytelling
├── 13-flagship-project/       one project that proves operational ability
├── 14-schedules/              30-day, 90-day, 12-month plans
├── 15-resources/              small, high-value resource list
├── 16-checklists/             objective progress and interview-readiness checklists
└── images/                    vector diagrams used throughout the notes
```

Every phase follows the same shape:

```text
README.md          why this phase exists, prerequisites, outcomes, order
0x-topic.md        first-principles notes + commands + outputs + failure modes
0x-labs.md         labs with objective, setup, steps, expected output, troubleshooting
checkpoint         questions and a "can I explain this without notes?" test
```

---

## The learning loop (do not skip step 3)

```text
READ          one topic, 30–45 min. Do not highlight. Write the mechanism in your own words.
LAB           disposable VM or kind cluster, 60–120 min. Break it on purpose.
NOTE          one Markdown file per lab: hypothesis, commands, evidence, fix, prevention.
DRILL         one timed incident or 15-question quiz per week.
TEACH         explain it out loud in 3 minutes, then in 30 seconds.
REVIEW        spaced repetition: 1 day, 7 days, 30 days.
```

The note is the deliverable. If you cannot write the causal chain, you do not know it yet.

---

## Environment matrix: what runs where

| Workload | macOS (your MacBook) | Linux VM | Cloud | Notes |
|---|---|---|---|---|
| Shell, SSH, git, editors | Yes | Yes | Yes | macOS is BSD-flavoured; flags differ from GNU/Linux |
| Linux permissions, ACLs, systemd, journald | No (macOS uses launchd) | **Required** | Possible | Use Ubuntu 24.04 or Rocky/Alma 9 |
| `iptables`, `nftables`, `firewalld`, network namespaces | No / limited | **Required** | Yes | Linux VM with 2 NICs is ideal |
| Docker | Yes (Docker Desktop/Colima) | Yes | Yes | Docker Desktop adds a VM boundary; be aware of it |
| kind / minikube | Yes (Docker required) | Yes | Yes | 8 GB RAM minimum; 16 GB comfortable |
| KVM/QEMU nested virtualization | Apple Silicon: limited; x86 Mac: possible | **Best** on bare metal or nested VM | Yes (bare-metal cloud) | Do not pretend Docker is virtualization |
| full OpenStack | No | DevStack possible (8–16 GB) | Yes | Start with architecture + CLI, not a production install |
| AWS VPC labs | Yes (CLI) | Yes | **Yes** | Set a budget alarm before creating anything |
| GPU/CUDA | CPU-only PyTorch; no real GPU | Depends on host GPU | GPU instance costs money | Warn before renting; use CPU labs first |
| Slurm | No | Yes (single-node) | Yes | Docker or VM |
| Prometheus/Grafana | Yes (Docker) | Yes | Yes | Run the stack locally |

### Recommended disposable Linux VM

- **Ubuntu Server 24.04 LTS** (most tutorials, apt) or **Rocky Linux 9 / AlmaLinux 9** (RHEL-compatible, dnf/yum, closer to RTDS's RHEL/RHOSO world).
- 4 vCPU, 8 GB RAM, 40 GB disk for Phases 1–3.
- 8 vCPU, 16 GB RAM, 80 GB disk for Kubernetes/OpenStack labs.
- Take a snapshot before every destructive lab.
- Keep a second VM for networking labs (two machines, same virtual network).

> **Do not run `rm -rf`, `mkfs`, `dd`, firewall flushes, or partition edits on your primary machine.** Every lab in this repo assumes a disposable VM or a test cluster. If a command can destroy data, the lab says so explicitly.

---

## About the RTDS job description

The original attachment is not present in this workspace. The working copy in [00-role-and-strategy/01-rtds-job-description-working-copy.md](00-role-and-strategy/01-rtds-job-description-working-copy.md) is reconstructed from the requirements in your message. It is marked as a working copy, not as the original document. If you have the exact JD, replace that file and re-run the gap table; the priorities are unlikely to change, but the emphasis might.

We do **not** invent the RTDS online-assessment format. The three mock assessments are built around the skills the JD names. The assumptions are stated in [11-assessments/README.md](11-assessments/README.md).

---

## Non-negotiables

1. **Evidence before change.** Capture `date`, `hostname`, `uptime`, `systemctl --failed`, `df -h`, `df -i`, `free -m`, `ss -tulpn`, and the exact error before restarting anything.
2. **One variable at a time.** If you change two things and it works, you learned nothing.
3. **Read the man page.** At least the NAME, SYNOPSIS, and the options you use. `man 7 signal`, `man 5 proc`, `man 8 ip`, `man 5 systemd.service`.
4. **Understand the failure, not just the fix.** Every lab has a "break it" step.
5. **Prefer the smallest safe change.** Reload before restart. Drain before reboot. Snapshot before upgrade.
6. **Do not skip DSA or core CS.** The schedules keep 60–90 minutes/day for DSA and 1–2 hours/week for OS/DBMS/CN revision. Infrastructure is an addition, not a replacement.
7. **Apply before you feel ready.** The minimum viable set is at the end of the 30-day plan. Waiting for OpenStack and GPUs is a mistake.

---

## Progress

Use [16-checklists/01-master-progress-checklist.md](16-checklists/01-master-progress-checklist.md). A checkbox is only complete when you have an artifact: a lab note, a command transcript, a diagram, a passing mock, or a recorded explanation.

```text
[ ] I can draw the Linux layer diagram from memory.
[ ] I can trace a packet and name the command that proves each hop.
[ ] I can explain kubectl apply end to end.
[ ] I have broken and fixed at least five services on purpose.
[ ] I have a 90-second story for each incident I have debugged.
```

---

## A note on depth

This repository is deliberately uneven. Phase 1 and Phase 2 are deeper than a typical roadmap because the RTDS assessment and almost every infrastructure interview start there. OpenStack and AI infrastructure are introduced with strong foundations and scoped labs, not production installs. You will add depth on the job.

The order is the point:

```text
architecture → OS → Linux → networking → virtualization/storage
            → containers → Kubernetes → cloud/OpenStack
            → automation → observability → AI infrastructure
```

If a later topic feels confusing, the missing prerequisite is almost always earlier in that chain. Go back one layer. That is not a detour; it is the work.
