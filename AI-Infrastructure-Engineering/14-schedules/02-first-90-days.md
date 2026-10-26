# Roadmap B — First 90 Days

> **Outcome:** P0 complete, core P1 complete, one deployable flagship project, three mock assessments passed, and enough evidence to interview for infrastructure/platform internships.
>
> **Assumption:** 15–18 focused hours/week. If you have 10, use the 6-month variant at the end. If you have 25, add depth, not new tools.

## The 90-day shape

```text
Days 1–30    P0 sprint: Linux + networking + Bash + Mock 1 + first applications
Days 31–45   Finish P0 gaps; KVM/QEMU; storage; containers
Days 46–60   Kubernetes core + networking + storage
Days 61–75   Kubernetes debugging; Ansible; Prometheus; OpenStack architecture
Days 76–90   Flagship project v1; Mock 2 + Mock 3; interview reps; applications
```

You apply at **day 30–45**, not day 90. The remaining 45 days are for depth while you interview. See [minimum viable set](../00-role-and-strategy/04-master-syllabus.md#minimum-viable-set-for-applying).

---

## Weekly plan

| Week | Hours | Learning goals | Concepts | Labs | Assessment / milestone | Deliverable |
|---|---:|---|---|---|---|---|
| 1 | 15 | Own the Linux filesystem, permissions, processes | FHS, inodes, links, umask, ACL, SUID/SGID/sticky, sudo, process states, signals | L1.1, L1.2, L1.3 | 20-question Linux quiz | VM + permission/process notes |
| 2 | 16 | Own memory, disks, systemd, logs | page cache, RSS/PSS, swap, OOM, partitions, LVM, ext4/XFS, units, journald, logrotate | L1.4, L1.5, L1.6, L1.7 | Incident 1 walkthrough | Disk/OOM/systemd/log notes |
| 3 | 16 | Networking fundamentals | binary, CIDR, VLSM, ARP, switching, VLAN, routing, `ip route get` | L2.1, L2.2 | Subnetting test ≥ 90% | 30-problem worksheet + packet diagram |
| 4 | 16 | TCP, DNS, NAT, firewalls, packet paths | handshake, states, sockets, DNS records, NAT/conntrack, iptables/nftables/firewalld | L2.3, L2.4, L2.5 | **Mock 1** (target ≥ 75%) | 5 incident stories + score review |
| 5 | 16 | TLS/HTTP/LB; finish P0 gaps; start virtualization | TLS handshake, cert chain, 502/503/504, L4/L7, KVM/QEMU/libvirt | L2.6, L2.7, L3.1 | **Apply to 5 roles** | Resume v2 + 5 applications |
| 6 | 17 | KVM/QEMU in practice; storage foundations | VM lifecycle, qcow2, virtio, snapshots, block/file/object, NFS, iSCSI, IOPS/latency | L3.2, L3.3 | KVM lab demo (5 min) | VM + NFS/iSCSI note |
| 7 | 17 | Container internals | namespaces, cgroups, capabilities, overlayfs, veth/bridge/NAT, OCI, containerd | L4.1, L4.2 | Container-from-scratch explanation | Container internals diagram + lab |
| 8 | 18 | Kubernetes core architecture | API/etcd/scheduler/controller-manager, kubelet/runtime/CNI, Pod lifecycle, `kubectl apply` flow | L4.3 | Describe a Pod end to end | Control-plane diagram from memory |
| 9 | 18 | Kubernetes networking and services | Services, EndpointSlice, kube-proxy/eBPF, CoreDNS, ingress, NetworkPolicy | L4.4 | Debug a no-endpoints Service | Service traffic-path note |
| 10 | 18 | Kubernetes storage, scheduling, resources | PV/PVC/CSI, requests/limits, QoS, OOM, taints/tolerations, affinity, HPA | L4.5 | Schedule a Pod with constraints | Scheduling decision table |
| 11 | 18 | Kubernetes debugging | Pending, CrashLoop, ImagePull, OOM, DNS, no endpoints, NotReady | L4.6, INC-08…11 | **Mock 2** (target ≥ 75%) | Debug playbook + 4 incident notes |
| 12 | 17 | Automation and monitoring | Bash safety, Python subprocess/retries, Ansible idempotency, PromQL, alerts | L7.1, L7.3, L8.1, L8.2 | Ansible second run `changed=0` | Script + playbook + dashboard |
| 13 | 17 | OpenStack architecture; project integration | Keystone, Glance, Nova, Neutron, Cinder, Placement, VM creation flow | L6.1, L6.2 | **Mock 3** (target ≥ 75%) | VM-flow diagram + project v1 demo |

**Total:** ~216 hours (including labs and review). At 16–17 h/week, 13 weeks.

---

## Phase detail

### Days 1–30 — assessment readiness

Follow [Roadmap A](01-first-30-days.md). Do not add KVM or Kubernetes early. The point is to make Linux and networking automatic.

**Milestone:** Mock 1 ≥ 75%, five incident stories, five applications.

**If Mock 1 < 75%:** do not start KVM. Spend week 5 repairing the two weakest areas. A low score is information, not failure.

### Days 31–45 — finish P0, start virtualization

**Concepts**

- Remaining P0 gaps: package management, SSH hardening, shell text processing.
- KVM as a kernel module; QEMU as the device model; libvirt as the management API.
- vCPU vs pCPU, memory overcommit, virtio, qcow2, snapshots, tap devices.
- Block vs file vs object storage; NFS and iSCSI basics; IOPS vs throughput vs latency.

**Labs**

- L3.1: install KVM/libvirt, create an Ubuntu VM, inspect the domain XML, find the qcow2 and tap device.
- L3.2: snapshot, clone, corrupt a disk image (in the lab), recover.
- L3.3: NFS export/mount; iSCSI target/initiator; `fio` read/write/latency.

**Deliverable:** a 1-page "where the VM actually runs" diagram with the QEMU process, libvirt XML, disk, tap, bridge, and guest.

### Days 46–60 — containers and Kubernetes core

**Concepts**

- A container is a process with namespaces, cgroups, capabilities, seccomp, and a rootfs.
- Image layers, overlayfs, registries, manifests, pull policy.
- veth pairs, bridge, NAT, port publishing, CNI.
- Kubernetes control plane vs node components; reconciliation; watch/list.
- The `kubectl apply` path: authn → authz → admission → validation → etcd → controllers/scheduler → kubelet → runtime.

**Labs**

- L4.1: create a container manually with `unshare`/`chroot`/cgroups (or inspect `runc`).
- L4.2: `docker run`, inspect veth/bridge/iptables, publish a port, capture traffic.
- L4.3: kind cluster; deploy a Pod; trace it with `kubectl get -w`, `describe`, `logs`, `events`.

**Deliverable:** a written answer to "what happens when I run `kubectl apply -f pod.yaml`?" with no hand-waving.

### Days 61–75 — Kubernetes operations and debugging

**Concepts**

- Services, EndpointSlices, kube-proxy iptables/IPVS/eBPF, CoreDNS, ingress.
- PV/PVC/StorageClass/CSI; attach vs mount.
- Requests/limits, QoS classes, eviction, OOMKilled.
- Taints/tolerations, affinity, topology spread, PDB, HPA, cluster autoscaler.
- RBAC, service accounts, NetworkPolicy.

**Labs**

- L4.4: Service with no endpoints; fix labels/readiness; trace DNS → ClusterIP → Pod.
- L4.5: Pending due to requests/taints/PVC; fix each cause.
- L4.6: CrashLoop due to missing ConfigMap/Secret, bad probe, wrong command; OOMKilled due to limit.
- L4.7: node NotReady simulation (stop kubelet/containerd in kind; inspect events).

**Deliverable:** a debugging playbook with a decision tree for Pending / CrashLoop / ImagePull / no endpoints / DNS / OOM / NotReady.

### Days 76–90 — automation, observability, OpenStack, project

**Concepts**

- Bash strict mode, quoting, traps, idempotent-ish scripts.
- Python `subprocess.run(timeout=...)`, retries with backoff, JSON/YAML, structured logging.
- Ansible inventory, playbook, role, handler, template, facts, check mode, idempotency.
- Prometheus metric types, PromQL, `node_exporter`, alert `for`, Alertmanager.
- OpenStack architecture and VM lifecycle; RHEL/RHOSO context.

**Labs**

- L7.1: health-check script with disk/service/HTTP checks, JSON output, exit codes.
- L7.3: Ansible role that installs/configures a service; second run `changed=0`.
- L8.1/L8.2: Prometheus + Grafana + node_exporter; CPU/memory/disk alert; runbook.
- L6.1/L6.2: OpenStack architecture mapping; VM creation flow; CLI evidence from a sandbox/DevStack if available.

**Milestone:** Mock 3 ≥ 75%; project v1 demo; 10 applications total; two mock interviews.

---

## The flagship project (v1 by day 90)

Build **OpsForge** — a reproducible Linux/Kubernetes platform with observability and failure drills. Full spec: [13-flagship-project](../13-flagship-project/README.md).

Minimum v1:

```text
[ ] One Linux VM provisioned by Ansible (users, SSH, firewall, packages, chrony)
[ ] kind cluster with a sample app, Service, Ingress, PVC
[ ] Prometheus + Grafana + Alertmanager + node_exporter
[ ] 5 alert rules with runbook links
[ ] 5 deliberate failures, documented with evidence and fixes
[ ] Makefile / scripts to create, test, and destroy
[ ] README with architecture diagram and "what I would do at 10x scale"
```

This project is interview material because you can say: "I broke it five ways and here is the evidence."

---

## Assessment milestones

| Day | Assessment | Target | If below target |
|---|---|---|---|
| 30 | Mock 1: Linux + networking | ≥ 75% | Repair top 2 weak areas; retake day 37 |
| 60 | Kubernetes practical checklist | 8/10 tasks | Re-do L4.4–L4.6; no new topics |
| 75 | Mock 2: operations + troubleshooting | ≥ 75% | Incident drills; review evidence method |
| 90 | Mock 3: RTDS-mixed | ≥ 75% | Targeted revision; retake day 97 |
| 90 | Oral mock interview | 3/5 rubric | Record, review, repeat |

A score below target is not a reason to delay applications. It is a reason to change the next week's plan.

---

## DSA and core CS balance

| Period | DSA | Core CS | Infra |
|---|---|---|---|
| Days 1–30 | 1 h/day, 5 days/week | 2 h/weekend | 12–15 h/week |
| Days 31–60 | 45 min/day, 4 days/week | 1–2 h/weekend | 15–18 h/week |
| Days 61–90 | 45 min/day, 3–4 days/week | 1 h/weekend | 15–18 h/week |

Do not stop DSA. Campus placements often gate on it, and the habit keeps your problem-solving sharp for incident reasoning. If a week is overloaded, cut reading, not DSA or labs.

---

## Weekly operating rhythm

```text
Monday      Plan the week: one concept, one lab, one drill, one application
Tue–Thu     Two 90-minute blocks: read + lab; 45–60 min DSA
Friday      Write the lab note; review wrong answers; 30-min flashcard pass
Saturday    3–4 hour lab or project block; 1 hour DSA
Sunday      1 hour mock/drill; 1 hour core CS; 30 min weekly review
```

The weekly review answers three questions:

1. What can I now explain that I could not explain last week?
2. What did I break, and what evidence did I use?
3. What is the one thing I am avoiding? (Usually the answer is the next lab.)

---

## If you get an interview

Pause new material for 3 days. Do not pause labs entirely.

| Day | Focus |
|---|---|
| 1 | Re-read your project README, diagrams, and incident notes; prepare 5 stories |
| 2 | Mock interview: Linux, networking, K8s, one incident; record it |
| 3 | Company/JD-specific revision; prepare 5 questions to ask; light DSA |

Bring evidence: a repo, a diagram, a runbook, a dashboard screenshot, an incident timeline. "I built it" is weaker than "here is the failure I diagnosed and how I verified the fix".

---

## 6-month variant (if you have 10 h/week)

Stretch each 2-week block to 3 weeks:

- Weeks 1–6: P0 Linux + networking.
- Weeks 7–10: KVM + storage + containers.
- Weeks 11–16: Kubernetes + automation.
- Weeks 17–20: OpenStack + observability.
- Weeks 21–24: project v1 + mocks + applications.

Apply at week 8–10, not week 24. The project grows while you interview.

---

## 90-day review: the evidence audit

At day 90, you should be able to point to:

- [ ] A Linux VM you configured, hardened, and broke on purpose.
- [ ] A permission/ACL case explained without `chmod 777`.
- [ ] A disk-full incident solved with `lsof +L1` or inode evidence.
- [ ] A systemd unit + timer + journal timeline.
- [ ] A subnetting worksheet at ≥ 90%.
- [ ] A packet-path diagram with commands at every hop.
- [ ] A firewall/NAT lab with `tcpdump` evidence.
- [ ] A Docker container inspected at the namespace/cgroup/veth level.
- [ ] A kind cluster with a working Service, DNS, ingress, and PVC.
- [ ] Five Kubernetes failures diagnosed from evidence.
- [ ] An Ansible role that is idempotent on the second run.
- [ ] A Prometheus alert with a runbook.
- [ ] An OpenStack VM creation diagram.
- [ ] Three mock scores ≥ 75%.
- [ ] One project demo and five incident stories.
- [ ] At least 15 applications submitted.

If an item is missing, that is your next 30 days. Do not start P3 because it feels more interesting. The foundation is the differentiator.
