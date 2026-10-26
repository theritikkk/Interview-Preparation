# Gap Analysis Against the RTDS Infrastructure Internship

> Read this as a map, not a verdict. The purpose is to separate **what you have demonstrated** from **what you have only used**, so you do not spend three months re-learning Docker and zero days learning `systemd`, subnetting, or KVM.

## How to read the table

**Priority definitions**

| Priority | Meaning | When |
|---|---|---|
| **P0** | Foundational and essential. Without it, the rest is memorisation. | First 30 days |
| **P1** | High value for RTDS and similar infrastructure roles. | Days 31–90, and while interviewing |
| **P2** | Important for production-level infrastructure engineering. | Months 4–8, or on the job |
| **P3** | Specialisation for AI infrastructure. | Months 6–12 |

**Importance for an entry-level assessment**

- **Critical** — very likely to appear, in some form, in an OA or interview.
- **High** — common; expect it in a technical round.
- **Medium** — differentiating; may appear as one or two questions.
- **Low** — unlikely in an entry-level OA, useful later.

Priority is not the same as study order. A P1 skill can still have P0 prerequisites. The dependency graph decides the order; the priority decides what to protect when time is short.

---

## The gap table

| Required skill or knowledge area | Why it matters for infrastructure engineering | What I already know from my stated background | What I probably need to learn more deeply | Prerequisites required before learning it | Importance for entry-level assessment | Priority |
|---|---|---|---|---|---|---|
| **Linux filesystem and navigation** | Every investigation starts somewhere on the filesystem; FHS knowledge tells you where configuration, state, logs, and binaries live | Comfortable in a terminal; uses paths, `cd`, `ls`, `find`, editors | FHS layout, mount points, inodes, hard/soft links, `findmnt`, `lsblk`, pseudo-filesystems (`/proc`, `/sys`), deleted-open files | None beyond basic shell use | **Critical** — command/output questions are easy to set | **P0** |
| **Users, groups, permissions, ACLs, sudo** | Most "permission denied" incidents are ownership, parent-directory execute, ACL, umask, or a security module — not `chmod 777` | Knows `rwx`, `chmod`, `chown` conceptually; has fixed file permissions | Umask, SUID/SGID/sticky, ACLs (`getfacl`/`setfacl`), default ACLs, `sudo` policy, capabilities, SELinux/AppArmor context, why root can still be denied | Filesystem basics | **Critical** — a classic OA and interview topic | **P0** |
| **Processes, threads, signals, jobs** | You cannot debug a service without knowing what state its process is in and what it is waiting on | Knows `fork`, `exec`, threads, signals, scheduling from OS/DSA; uses `ps`, `kill` | `R/S/D/T/Z/I` states, run queue vs load average, `D` state, zombies/orphans, `/proc/<pid>`, `pgrep`, `pstree`, `nice`, `renice`, job control, signal choice, cgroup context | OS process/thread fundamentals | **Critical** — "process is running but unresponsive" scenarios | **P0** |
| **Memory, virtual memory, swap, OOM** | Memory pressure is a leading cause of latency and kills; "free memory is low" is usually normal, not a bug | Knows virtual memory/paging from OS; has seen `free`, `top` | RSS vs VSZ vs PSS, page cache, buffers, swap in/out, overcommit, `vmstat`, `/proc/meminfo`, OOM score, cgroup `memory.max`, OOMKilled vs host OOM, leak vs cache vs limit | Processes, virtual memory, filesystem cache | **Critical** — output interpretation and incident reasoning | **P0** |
| **Disks, inodes, filesystems, mounts, LVM** | Disk-full incidents are daily operations; inode exhaustion and deleted-open files are classic traps | Uses `df`, `du`, `lsblk`; has partitioned a disk casually | ext4 vs XFS, inodes, mount options, `fstab`, `findmnt`, LVM (PV/VG/LV), RAID concepts, `iostat`, `iotop`, read-only remount, `fsck` safety, `smartctl` | Filesystem and block-device basics | **Critical** — "disk full but `du` says otherwise" is a favourite | **P0** |
| **systemd, services, timers, boot** | Production Linux is systemd-managed; service failures, ordering, and boot problems are core ops | Has started/stopped services; has seen unit files | Unit types, dependencies, targets, `After`/`Requires`/`Wants`, drop-ins, `systemctl` lifecycle, timers, socket activation, boot chain, `systemd-analyze`, `journalctl -u` | Processes, filesystem, permissions | **Critical** — likely scenario question | **P0** |
| **Logs: journald, journalctl, `/var/log`, rotation** | Logs are the primary evidence source; missing or rotated logs can hide the cause | Uses `journalctl`, application logs, and Loki | Persistent journal, rate limiting, priorities, fields, `-b`, `--since`, unit/kernel filters, rsyslog, `/var/log` layout, `logrotate` (copytruncate/create/rotate), log levels | systemd, filesystem | **Critical** — log interpretation is explicitly in the JD | **P0** |
| **Package management: apt, dnf, yum** | You must install, pin, roll back, and diagnose dependency/repo failures on RHEL and Debian families | Uses `apt` on Ubuntu/Debian; probably `brew` on macOS | Repositories, GPG keys, metadata cache, version pinning, `dnf history`, `apt-mark`, dependency conflicts, offline installs, `rpm -V`, package ownership (`rpm -qf`, `dpkg -S`) | Linux filesystem, users/permissions | **High** — likely MCQ; practical in interview | **P1** |
| **SSH and remote operations** | Almost all infrastructure work is remote; SSH misconfiguration is a common lockout | Uses SSH keys and GitHub; can log in to servers | `~/.ssh/config`, agent forwarding, `ProxyJump`, local/remote/dynamic forwarding, `sshd_config`, key permissions, `scp` vs `rsync`, `tmux`/`screen`, host key verification | Networking basics, permissions | **High** — practical and troubleshooting | **P1** |
| **Shell text processing: grep, sed, awk, sort, find, jq** | Most operational evidence is text; speed and precision matter | Comfortable with pipes, `grep`, basic `awk`/`sed` | `awk` fields/patterns/arrays, `sed` addressing, `find -exec`/`-mtime`/`-size`, `xargs` safety, `sort -k/-n/-h`, `uniq -c`, `jq` filters, `cut`, `tr`, `paste`, `column` | Shell fundamentals | **Critical** — command-output and scripting questions | **P0** |
| **IP addressing, CIDR, subnetting** | Every network design and failure starts with addresses and masks; OA subnetting is common | Studied CN; can convert basic binary; knows private ranges | Fast subnet math, VLSM, host/network/broadcast, /31 and /30, overlap detection, route summarisation, IPv6 basics | Binary arithmetic | **Critical** — almost certain in an infrastructure OA | **P0** |
| **Ethernet, ARP, switching, VLANs** | Layer 2 explains "same subnet works, different subnet fails" and many VM/container connectivity issues | Knows MAC, ARP, VLAN concepts from CN | Frame format, ARP request/reply, switch MAC table, access vs trunk, 802.1Q tag, native VLAN, Linux bridge and VLAN interfaces, inter-VLAN routing | IP addressing | **High** — VLAN and ARP scenarios are common | **P1** |
| **IP routing and Linux route tables** | A wrong route or gateway makes a healthy service unreachable; longest-prefix match is fundamental | Knows routing concepts; has seen `ip route` | Connected/static/default routes, longest prefix match, `ip route get`, forwarding, policy routing, asymmetric routing, ICMP redirects, route metrics | IP addressing, interfaces | **Critical** — packet-path troubleshooting | **P0** |
| **TCP/UDP, ports, sockets** | Most application "network" failures are socket, state, or transport problems | Knows TCP handshake, UDP, ports from CN and backend work | State machine (`LISTEN`, `SYN-SENT`, `ESTABLISHED`, `TIME_WAIT`, `CLOSE_WAIT`), `ss` output, ephemeral ports, backlog, retransmits, RST vs timeout, MSS/MTU, conntrack | IP, routing | **Critical** — `ss`/`netstat` interpretation | **P0** |
| **DNS resolution** | "Works by IP, fails by name" is one of the most common incidents; Kubernetes depends on it | Uses DNS; knows A/AAAA/CNAME conceptually | Recursive vs iterative resolution, resolver config, `dig`/`getent`, TTL/caching, search domains/`ndots`, split-horizon, PTR, DNSSEC basics, CoreDNS | IP, UDP/TCP, `/etc/hosts` | **Critical** — guaranteed troubleshooting topic | **P0** |
| **DHCP** | Address assignment failures look like "the network is down"; private cloud and VM networks rely on it | Knows DORA conceptually | DORA, lease/renewal, relay, scope, reservations, conflicts, client debugging, DHCP vs static | IP, Ethernet, routing | **Medium** — one or two questions; practical in ops | **P1** |
| **NAT, gateways, firewalls (firewalld, iptables/nftables)** | Connectivity is often blocked, not broken; you must distinguish drop vs reject vs no route | Uses AWS security groups; knows NAT conceptually | SNAT/DNAT/MASQUERADE, conntrack, iptables tables/chains, nftables, firewalld zones/services/rich rules, default policies, stateful filtering, hairpin NAT | Routing, TCP/UDP | **High** — firewall scenarios and `iptables` basics | **P1** |
| **Linux interface configuration and bonding** | RHEL shops configure bonds, VLANs, bridges, and MTU on hosts; VM/host networking depends on it | Basic `ip addr`/`ip link`; Docker networking exposure | NetworkManager (`nmcli`), `ip` vs legacy `ifconfig`, VLAN subinterfaces, bonds (active-backup, LACP), bridges, MTU, `ethtool`, `ip -s link` | Ethernet, routing, permissions | **High** — RTDS explicitly names bonding/VLANs | **P1** |
| **HTTP, TLS, certificates, load balancing** | Most services are HTTP; TLS and LB failures present as "the app is down" | Strong HTTP/REST backend experience; knows status codes | TLS handshake, certificate chain, SNI, expiry, `openssl s_client`, HTTP/1.1 vs 2, keep-alive, L4 vs L7 LB, health checks, `X-Forwarded-For`, 502/503/504 | TCP, DNS | **High** — practical debugging | **P1** |
| **Virtualization: VMs, hypervisors, KVM, QEMU, libvirt** | RTDS runs private cloud; you must know where a VM actually executes and how it is defined | Uses Docker; knows VMs conceptually; has used cloud instances | Type 1/2 hypervisors, KVM kernel module, QEMU device model, libvirt XML, virtio, qcow2, snapshots, CPU/memory overcommit, nested virtualization, VM lifecycle | Linux kernel, processes, memory, storage, networking | **High** — core RTDS differentiator | **P1** |
| **Storage: block/file/object, NFS, iSCSI, Ceph, NVMe-oF** | Compute is useless without storage; latency and attachment failures dominate private cloud incidents | Uses local disks and S3; knows object storage conceptually | Block vs file vs object, NFS exports/mounts/locking, iSCSI target/initiator/LUN/CHAP, Ceph MON/MGR/OSD/MDS/RGW/CRUSH, NVMe/NVMe-oF queues, IOPS vs throughput vs latency, `fio` | Linux storage stack, networking, virtualization | **Medium** — concepts likely; deep troubleshooting later | **P2** (RTDS: **P1**) |
| **Container internals: namespaces, cgroups, capabilities, images** | Docker familiarity is not container internals; K8s debugging depends on them | Builds and runs Docker images; uses Compose; knows layers conceptually | Namespaces (pid/net/mnt/uts/ipc/user/cgroup), cgroups v2, capabilities, seccomp, AppArmor/SELinux, overlayfs, OCI images/manifests, runc/containerd, veth/bridge/NAT, rootless | Linux processes, filesystem, networking, permissions | **High** — "how does a container work?" is common | **P1** |
| **Kubernetes architecture, scheduling, networking, storage, debugging** | The JD lists K8s/OpenShift; most platform roles expect operational debugging, not just YAML | Deploys services to K8s; uses Deployments, Services, ConfigMaps, Ingress; has Prometheus/Grafana | API server/etcd/scheduler/controller-manager, kubelet/runtime/CNI/CSI, Pod lifecycle, probes, QoS, requests/limits, scheduling constraints, Service/EndpointSlice/kube-proxy, CoreDNS, NetworkPolicy, PV/PVC, RBAC, HPA, node failure, `kubectl` evidence gathering | Containers, Linux networking, storage, distributed systems | **High** — likely a subset; debugging is key | **P1** (P0 subset: `kubectl get/describe/logs`, Pod lifecycle) |
| **OpenStack and private cloud** | Explicitly named (RHEL/RHOSO); the role is private-cloud operations | None stated; may know "OpenStack is a cloud platform" | Keystone, Glance, Nova (API/conductor/scheduler/compute), Neutron (ML2/OVS/OVN), Cinder, Placement, Horizon, VM creation flow, tenant/provider networks, floating IPs, quotas, failure domains | Linux, networking, virtualization, storage | **Medium/High** — RTDS-specific; likely architecture/lifecycle questions | **P1** |
| **Bash scripting for operations** | Automation is explicitly required; unsafe scripts cause outages | Writes shell commands; knows variables/loops/conditionals | Quoting, arrays, `set -euo pipefail`, traps, exit codes, functions, `getopts`, process substitution, here-docs, safe temp files, ShellCheck, testing | Shell, text processing, processes | **Critical** — practical scripting exercise | **P0** |
| **Python for operational automation** | Parsing APIs/JSON/logs and handling timeouts/retries is a core ops skill | Strong Python backend; FastAPI/Node; scripting | `subprocess` with timeout, `pathlib`, `argparse`, `logging`, `json`/`yaml`/`csv`, `requests` with retries, dataclasses, structured output, idempotent operations, tests | Python, Linux, HTTP | **High** — likely a practical task | **P1** |
| **Ansible and configuration management** | The JD names playbooks; idempotency is the concept interviewers test | None stated; may have used ad-hoc commands | Inventory, modules, playbooks, roles, handlers, templates, vars/facts, `check_mode`, `--diff`, idempotency, Vault, `ansible-lint`, Molecule | SSH, YAML, Linux administration | **High** — likely practical/scenario | **P1** |
| **CI/CD: GitHub Actions, Jenkins, GitLab CI** | Infrastructure changes need automated validation; the JD names CI/CD | Uses GitHub Actions for app CI | Pipeline stages, secrets, matrix builds, caching, self-hosted runners, lint/test/validate for infra, deployment gates, rollback | Git, Bash, testing | **Medium** — supporting skill | **P2** |
| **Prometheus, Grafana, alerting** | Monitoring is explicitly required; the job is to detect and explain, not just draw dashboards | Instruments services; runs Prometheus/Grafana/Loki/Alertmanager | Metric types, PromQL, `node_exporter`, scrape/config, recording/alerting rules, `for`, Alertmanager routing, golden signals, USE/RED, dashboard design, missing-data alerts | Linux metrics, HTTP, YAML | **High** — metric/log interpretation | **P1** |
| **Log analysis and root-cause analysis** | The JD asks for RCA, runbooks, and incident documentation | Debugs application issues; writes postmortems informally | Evidence-first method, timelines, correlation, hypothesis testing, contributing factors, 5 Whys vs causal chain, runbook structure, blameless postmortem | Logs, metrics, Linux/network fundamentals | **Critical** — scenario interviews | **P0** |
| **Cloud infrastructure and AWS networking** | The JD lists cloud; AWS is the most likely public-cloud reference | Deploys apps to AWS; knows EC2/S3/IAM at a high level | VPC CIDR/subnets, route tables, IGW/NAT, SG vs NACL, DNS/Route 53, ALB/NLB, EBS, IAM policy evaluation, EKS/VPC CNI, cost and quotas | IP/routing, TCP, Linux, containers | **High** — VPC and SG questions are common | **P2** (AWS-specific; **P1** for cloud roles) |
| **Infrastructure security** | Private cloud is multi-tenant; least privilege and isolation are operational requirements | App-level auth; knows secrets basics | IAM/RBAC, service accounts, TLS, key management, hardening, sudo/capabilities, SELinux, network segmentation, auditd, supply chain | Linux, networking, K8s, cloud | **Medium** — concepts and least privilege | **P2** |
| **Distributed systems fundamentals** | The role supports distributed control planes (OpenStack, K8s) and stateful workloads | Strong: queues, workers, retries, scheduling, concurrency, service communication; built a task scheduler | Formalise: partial failure, idempotency, at-least-once vs exactly-once, consensus (etcd/Raft), CAP/PACELC, backpressure, clock skew, retry storms | OS, networking | **Medium** — leverage your strength; use it to explain infra | **P2** |
| **GPU architecture, CUDA, distributed training, GPU scheduling, MLOps** | Long-term AI infrastructure goal; not required for the RTDS internship | Conceptual interest; ML project exposure; no GPU operations | SM/warp/kernel model, HBM, PCIe/NVLink, GPU metrics, training vs inference, data loading, NCCL/all-reduce, Kubernetes device plugin/MIG, Slurm, MLflow, vLLM/Triton | Linux, containers, K8s, networking, storage, scheduling, observability | **Low** for RTDS; **High** for AI infra roles | **P3** |

---

## The honest verdict

### What is genuinely strong

| Strength | Why it matters | How to use it |
|---|---|---|
| Python/Node backend engineering | Automation, APIs, tooling, debugging application behavior | Write operational tools, not just scripts; read service code during incidents |
| Distributed systems project | Queues, retries, DLQs, scheduling, concurrency | Explain infrastructure through failure modes you have already implemented |
| Docker/Kubernetes usage | You can get to a running workload | Go one layer down: namespaces, cgroups, CNI, kubelet |
| Prometheus/Grafana/Loki/Alertmanager | Observability vocabulary and instrumentation | Learn metric design, PromQL, alert quality, capacity |
| AWS/Terraform/GitHub Actions exposure | Cloud and IaC familiarity | Add networking depth and IAM reasoning; do not claim deep cloud architecture yet |
| 450+ DSA problems | Debugging discipline and interview stamina | Keep it warm; do not let infra study erase it |

### What is currently a claim, not a demonstrated skill

- "I know Kubernetes" → you can deploy, but can you debug a Pending pod, a missing endpoint, or a node that is NotReady?
- "I know AWS" → you can launch, but can you design a VPC and explain why a private subnet has no internet?
- "I know Linux" → you can use a shell, but can you own users, services, disks, and host networking on RHEL?
- "I know monitoring" → you can instrument, but can you design an actionable alert and capacity plan?
- "I know virtualization" → you have used cloud VMs, but have you run KVM/QEMU and read a libvirt domain?
- "I know OpenStack" → not yet; you know the name.

### The top ten gaps to close first

1. **Linux service and process debugging** — `systemd` + `journalctl` + process state + cgroups, as one causal chain.
2. **Permissions beyond `chmod`** — umask, ACLs, special bits, parent-directory execute, SELinux/AppArmor.
3. **Memory interpretation** — page cache vs anonymous memory, RSS/PSS, swap, OOM score, container limits.
4. **Disk and inode exhaustion** — `df` vs `du`, deleted-open files, LVM, read-only filesystems.
5. **Subnetting speed and accuracy** — no calculator, under time pressure.
6. **Packet-path debugging** — name → route → socket → firewall → transport → application, with one command per hop.
7. **KVM/QEMU mental model** — where the VM runs, what libvirt owns, how the disk and tap device appear.
8. **Kubernetes internals and evidence gathering** — not more YAML; `describe`, events, logs, endpoints, DNS, OOM.
9. **OpenStack VM lifecycle** — API → Keystone → Nova → Placement → scheduler → compute → Neutron/Glance/Cinder.
10. **Structured RCA and communication** — symptoms → evidence → hypotheses → tests → cause → fix → verification → prevention.

---

## Study allocation for the first 90 days

If you have 10 hours/week:

| Area | Share | Hours | Why |
|---|---:|---:|---|
| Linux administration | 30% | 36 | The base layer; highest OA weight |
| Networking | 25% | 30 | The most common source of "unreachable" incidents |
| Kubernetes/containers | 15% | 18 | High value, but after Linux/networking |
| Virtualization/storage | 10% | 12 | RTDS differentiator; KVM first, Ceph later |
| Automation | 10% | 12 | Bash and Ansible; Python is already strong |
| Monitoring/RCA | 10% | 12 | Turn knowledge into incident stories |

If you have 15 hours/week, add a second weekly lab and one timed mock. Do not add more reading. The bottleneck is reps, not content.

---

## What not to do

- Do not start with OpenStack. It will feel like memorising service names. Do Linux → networking → virtualization → storage first.
- Do not start with GPU/CUDA. You cannot debug a GPU node if you cannot debug a Linux node.
- Do not collect 40 courses. One note, one lab, one incident at a time.
- Do not memorise commands without the failure they diagnose. `ss -tulpn` is not trivia; it answers "is anything listening, and on which address?"
- Do not wait until the roadmap is "finished" to apply. The minimum viable set is in the 30-day plan.

---

## Completion criterion for this document

You can close this file when you can answer, without notes:

1. Which three gaps are most likely to cost you the RTDS OA?
2. Which two strengths will you use to compensate in interviews?
3. What is the first lab you will run, and what evidence will it produce?

Write the answers in your own progress note. Then start [Phase 1](../01-linux-fundamentals/README.md).
