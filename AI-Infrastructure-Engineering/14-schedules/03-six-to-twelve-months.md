# Roadmap C — Six to Twelve Months

> **Outcome:** production-style platform engineering skills, a credible AI-infrastructure specialisation, and a portfolio that supports both infrastructure internships now and ML-platform roles later.
>
> **Time:** 10 h/week ≈ 520 h/year. DSA and core CS continue at 4–6 h/week; they are not replaced.

## The strategic view

```text
Months 1–3    Employable infrastructure generalist
              Linux + networking + KVM + containers + Kubernetes + automation + OpenStack literacy

Months 4–6    Production operator
              AWS VPC/IAM + security + SLOs + capacity + Ceph/NVMe + OpenStack troubleshooting + project v2

Months 7–9    GPU-capable infrastructure engineer
              GPU architecture + CUDA + distributed training + NCCL/RDMA + Slurm + K8s device plugin

Months 10–12  AI platform engineer
              MLOps + model serving + GPU observability + cost/capacity + capstone + interviews
```

The transition is not "learn AI instead of infrastructure". It is "apply infrastructure to accelerators". The same Linux, network, storage, scheduling, and observability skills reappear with different numbers.

---

## Monthly plan

| Month | Hours | Theme | Concepts | Labs / project | Milestone | Deliverable |
|---|---:|---|---|---|---|---|
| 1 | 50 | Linux + networking P0 | files, permissions, processes, memory, disks, systemd, logs, subnetting, TCP, DNS, firewalls | L1.1–L1.7, L2.1–L2.7 | Mock 1 ≥ 75% | Linux runbook + 5 incident notes |
| 2 | 50 | Virtualization, storage, containers | KVM/QEMU/libvirt, qcow2, NFS/iSCSI, namespaces, cgroups, overlayfs, veth | L3.1–L3.3, L4.1–L4.2 | KVM + container demos | "Where the VM/container runs" diagrams |
| 3 | 50 | Kubernetes + automation + OpenStack literacy | API/scheduler/kubelet/CNI/CSI, Services/DNS, Ansible, Prometheus, OpenStack VM flow | L4.3–L4.6, L7.1, L7.3, L8.1, L6.1–L6.2 | Mock 2 + Mock 3 ≥ 75%; project v1 | OpsForge v1 + 10 applications |
| 4 | 45 | AWS networking and identity | VPC, subnets, route tables, IGW/NAT, SG/NACL, ALB, EC2/EBS, IAM, EKS/VPC CNI | L5.1–L5.4 | VPC lab with clean teardown | Architecture diagram + cost note |
| 5 | 45 | Security, reliability, storage depth | least privilege, RBAC/IAM, TLS, secrets, SLO/error budget, capacity, Ceph, NVMe-oF | L8.3, L8.4, L3.4 | Incident drill + postmortem | SLO doc + security checklist |
| 6 | 40 | OpenStack operations + project v2 | Nova scheduler filters, Neutron port binding, Cinder attach, Placement, logs | L6.3–L6.4; project v2 | Diagnose an `ERROR` instance | OpenStack troubleshooting runbook |
| 7 | 45 | GPU architecture and CUDA | SM/warp/kernel, HBM, memory hierarchy, PCIe/NVLink, streams, profiling | L9.1 | Profile a workload; explain bottleneck | GPU mental model + profiling note |
| 8 | 45 | Distributed training and data path | DP/TP/PP, all-reduce, NCCL, RDMA/InfiniBand, checkpointing, data loading | L9.2, L9.3 | Explain why 2 GPUs can be slower than 1 | Training-infra diagram + benchmark |
| 9 | 40 | GPU scheduling: K8s + Slurm | device plugin, GPU operator, MIG, taints, topology, Slurm GRES/partitions | L9.4 | Schedule GPU jobs; read `sinfo`/`squeue` | Scheduling comparison note |
| 10 | 40 | MLOps and model serving | MLflow tracking/registry, artifacts, vLLM/Triton/KServe, autoscaling, latency | L9.5 | Serve a model; measure p95 | Serving runbook + cost model |
| 11 | 40 | AI observability and capstone | GPU metrics, XID/ECC, utilization vs progress, queueing, cost per token/request | Capstone | End-to-end demo | Capstone repo + dashboards |
| 12 | 30 | Interviews, system design, specialisation | infra system design, incident storytelling, AI platform design, open source | Mock interviews | 3 strong loops | Portfolio + target list |

**Total:** 520 hours.

---

## Quarter 1 — become employable

### Weekly goals

| Week | Goal |
|---|---|
| 1–2 | Linux files, permissions, processes, memory; one lab each |
| 3–4 | Disks, systemd, logs, packages, shell; Mock 1 |
| 5–6 | Subnetting, routing, TCP, DNS, firewalls; packet-path drills |
| 7–8 | KVM/QEMU, storage, containers; "what is a container?" from scratch |
| 9–10 | Kubernetes core, Services, DNS, storage, scheduling |
| 11–12 | Kubernetes debugging, Ansible, Prometheus, OpenStack architecture; project v1 |

**Concepts to protect:** causality, not commands. Every week produces one diagram and one incident note.

**Assessment:** Mock 1 by week 4; Mock 2 by week 11; Mock 3 by week 12.

**Deliverable:** OpsForge v1 (Linux + kind + monitoring + 5 failure drills). Apply to 15 roles.

**What you can explain at month 3**

- The full path from `kubectl apply` to a running container.
- Why a Pod is Pending, CrashLooping, OOMKilled, or has no endpoints.
- Where a KVM VM runs and how OpenStack creates one.
- How to troubleshoot Linux, DNS, TCP, and a firewall with evidence.
- How to write a safe script and an idempotent playbook.

**What you can troubleshoot at month 3**

- Failed systemd service, full disk, high load, OOM, permission denied.
- Unreachable port, DNS failure, wrong route, firewall drop.
- Pending/CrashLoop/ImagePull/no-endpoints Kubernetes failures.
- A basic OpenStack instance in `ERROR` (with guidance).

---

## Quarter 2 — become a production operator

### Focus

- **AWS:** VPC design, public/private subnets, NAT, SG vs NACL, ALB, IAM policy evaluation, EKS/VPC CNI/IRSA.
- **Security:** least privilege, TLS, secrets, host hardening, RBAC, network segmentation.
- **Reliability:** SLI/SLO, error budgets, alert quality, incident command, postmortems, capacity.
- **Storage depth:** Ceph architecture and health, NVMe-oF concepts, `fio` benchmarking.
- **OpenStack operations:** `ERROR` instances, scheduler filters, port binding, volume attach, logs.

### Labs

- L5.1–L5.4: VPC with public/private subnets, EC2, ALB, EKS or EKS-style diagram; **destroy everything** and verify.
- L8.3: define an SLI/SLO for the sample app; alert on burn rate, not CPU.
- L8.4: IAM/RBAC least-privilege exercise; secrets handling; TLS verification.
- L3.4: Ceph concepts + `ceph status` interpretation; `fio` IOPS/latency.
- L6.3–L6.4: OpenStack failure diagnosis from CLI/log evidence.

### Milestone

A production-style incident drill: inject a failure in OpsForge, detect it via alert, diagnose from evidence, write a postmortem, and add a prevention item. This is the artifact that separates you from tutorial followers.

### What you can explain at month 6

- Why a private subnet needs a NAT gateway and what happens without one.
- SG vs NACL, stateful vs stateless, and where each is enforced.
- IAM policy evaluation and the difference between a role and a user.
- How an SLO drives alerting and prioritisation.
- Ceph's data path (client → MON → OSD) and what `HEALTH_WARN` means.
- The OpenStack VM creation flow and the first three things to check when it fails.

**What you can troubleshoot at month 6**

- AWS connectivity: route table, SG, NACL, NAT, DNS, instance state.
- A latency regression with USE/RED metrics and a queueing model.
- A Ceph warning or a slow volume (conceptually, with guidance).
- An OpenStack instance `ERROR`, no valid host, port binding failure, volume attach failure.

---

## Quarter 3 — become GPU-capable

### Focus

- **GPU architecture:** SM, warp, block, grid, kernel, HBM, L2, PCIe/NVLink, NUMA.
- **CUDA:** host/device memory, streams, kernel launch, synchronization, profiling with Nsight.
- **Training:** batch size, throughput, latency, MFU, memory pressure, gradient accumulation.
- **Data path:** dataloader workers, storage IOPS, network filesystems, checkpoint writes.
- **Distributed:** data/model/pipeline parallel, all-reduce, NCCL, RDMA/InfiniBand/RoCE, topology.
- **Scheduling:** Kubernetes device plugin, GPU operator, MIG, taints/tolerations, gang scheduling; Slurm GRES/partitions.

### Labs

- L9.1: GPU architecture + CUDA basics; on CPU-only hardware, use PyTorch to illustrate kernels/memory; on a GPU (warning: cost), profile a small model.
- L9.2: measure data-loading stalls; compare batch sizes; identify memory pressure.
- L9.3: distributed training concepts; simulate all-reduce with `torch.distributed` on CPU/gloo; read NCCL logs if GPUs available.
- L9.4: kind + NVIDIA device plugin (requires GPU) or a simulated device-plugin exercise; Slurm single-node in Docker/VM.

**Cost warning:** GPU cloud instances can cost $1–$30+/hour. Set a budget alarm, use spot/preemptible, cap runtime, and destroy immediately. The CPU path teaches 70% of the concepts; rent a GPU only for the final profiling lab.

### What you can explain at month 9

- Why a GPU can be 100% utilized but not making progress.
- The difference between HBM capacity, bandwidth, and compute.
- Why batch size affects latency, throughput, and memory.
- Why distributed training is a network and storage problem as much as a compute problem.
- How Kubernetes and Slurm each allocate GPUs, and when to use which.

**What you can troubleshoot at month 9**

- GPU not visible in a container (`nvidia-smi`, device plugin, runtime).
- GPU OOM vs host OOM vs cgroup OOM.
- Training throughput drop due to data loading, PCIe, NCCL, or stragglers.
- A GPU node that is unschedulable due to taints, labels, or device inventory.

---

## Quarter 4 — become an AI platform engineer

### Focus

- **MLflow:** experiments, params, metrics, artifacts, model registry, reproducibility.
- **Serving:** vLLM/Triton/KServe, batching, concurrency, p95 latency, autoscaling, GPU sharing.
- **Observability:** GPU utilization, memory, temperature, power, ECC/XID, queue depth, token throughput.
- **Cost:** cost per request/token, idle GPU detection, rightsizing, MIG/time-slicing trade-offs.
- **Platform design:** multi-tenancy, quotas, fair scheduling, isolation, upgrade strategy.

### Capstone

Extend OpsForge into a GPU-ready AI platform:

```text
[ ] Kubernetes cluster with a GPU node pool (or a documented CPU simulation)
[ ] device plugin / GPU operator configuration
[ ] MLflow tracking + artifact store (S3/MinIO)
[ ] one model served with vLLM or Triton
[ ] Prometheus GPU dashboards + alerts
[ ] load test with p50/p95/p99 and GPU utilization
[ ] cost model: $/1k requests and $/GPU-hour
[ ] runbooks: GPU OOM, XID error, node drain, model rollback
[ ] failure drills: GPU unavailable, model crash, storage stall, queue buildup
```

### What you can explain at month 12

- End-to-end: request → DNS/LB → serving pod → GPU → response, with metrics at each hop.
- Why a model server is latency-bound vs throughput-bound, and how batching changes the curve.
- How to design a multi-tenant GPU platform with quotas, isolation, and fair scheduling.
- How to estimate capacity and cost for a training or inference workload.
- How Linux, networking, storage, virtualization, scheduling, and observability are prerequisites for all of it.

**What you can troubleshoot at month 12**

- A serving deployment with high p95 but low GPU utilization.
- A training job that stalls on data or checkpoints.
- A GPU node with ECC/XID errors and the safe drain/replace procedure.
- A queue that grows while workers look healthy (throughput vs arrival rate).
- A multi-tenant fairness problem.

---

## If you get placed / hired

Do not stop the roadmap. Convert work into structured learning:

| Work situation | Learning artifact |
|---|---|
| You fix a production incident | Write a sanitised postmortem; add it to your private notes |
| You deploy with Ansible/Terraform | Document the failure modes and rollback |
| You touch OpenStack/AWS | Draw the actual architecture; compare with the lab |
| You review a runbook | Improve one alert or one step; measure the change |
| You on-call | Keep a decision journal; review it weekly |

If the job is not infrastructure, keep 4–6 h/week for the roadmap. One real project plus deliberate practice beats a job title that does not match your target.

---

## If you are still searching at month 6

Do not add more certificates. Do these three things:

1. **Ship a second project** that solves a real operational problem (backup verification, cost report, GPU idle detector).
2. **Contribute one small fix** to an open-source infra project (docs, bug, test, runbook). Learn the review process.
3. **Run mock interviews weekly** with a peer or record yourself. The bottleneck is often explanation, not knowledge.

Apply to adjacent roles: backend/platform, SRE intern, cloud support, Linux admin, DevOps intern, ML platform intern. The first job is a platform for the next one.

---

## 12-month evidence audit

- [ ] P0 + P1 complete; P2 mostly complete; P3 capstone complete.
- [ ] Three mock assessments ≥ 75%, one ≥ 85%.
- [ ] 10+ incident notes with evidence and prevention.
- [ ] OpsForge v1 + AI platform capstone, both reproducible from a README.
- [ ] Dashboards and alerts that someone else can use.
- [ ] 5 runbooks and 3 postmortems.
- [ ] A 90-second and a 5-minute version of every project story.
- [ ] A cost model for one AI workload.
- [ ] 30+ applications, 5+ interview loops, 2+ offers or clear feedback.
- [ ] DSA still warm: 1–2 problems/day or a weekly contest.

---

## The final test

You are ready for an AI infrastructure role when you can be dropped into an unfamiliar Linux host or Kubernetes node and answer, in order:

```text
1. What is this machine supposed to be doing?
2. What is it actually doing?
3. What is it waiting on?
4. What changed recently?
5. What is the smallest safe intervention?
6. How will I know it worked?
7. What will prevent the next occurrence?
```

That sequence works on a laptop, a VM, a Kubernetes node, an OpenStack compute host, and a GPU server. The tools change. The reasoning does not.
