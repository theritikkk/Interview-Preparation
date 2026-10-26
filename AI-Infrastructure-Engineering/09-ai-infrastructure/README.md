# Phase 9 — AI Infrastructure

> **Why this phase matters:** an AI platform is not a GPU with a login. It is a pipeline that keeps accelerators fed, a scheduler that isolates expensive devices, a network that moves gradients and tokens without stalling, and an operations practice that can explain why a job is slow or a model is wrong. This phase connects the Linux, network, Kubernetes, observability, and security foundations to GPU workloads.

## Outcomes

By the end of this phase you can:

1. Explain the GPU execution model — SM, warp, block, grid, memory hierarchy — and why a GPU can be "busy" while doing little useful work.
2. Estimate whether a workload is compute-bound, memory-bandwidth-bound, or interconnect-bound, and size model memory before you launch it.
3. Read `nvidia-smi`, DCGM, Nsight, and PyTorch memory reports well enough to locate a bottleneck.
4. Explain the training loop, data pipeline, mixed precision, checkpointing, and the difference between training and inference economics.
5. Distinguish data, tensor, pipeline, and expert parallelism; explain all-reduce and why topology decides scaling efficiency.
6. Read NCCL logs and InfiniBand/RoCE counters; diagnose a hang, a timeout, or a straggler.
7. Schedule a GPU workload on Kubernetes with device plugins, taints, affinity, MIG, and quotas; compare with Slurm GRES, partitions, and fair-share.
8. Track an experiment, package a model, serve it, monitor latency/GPU/drift, and calculate cost per 1,000 requests.
9. Design a multi-tenant AI platform with quotas, isolation, observability, and a cost model.
10. Explain the failure modes of an AI platform, not just the happy path.

## Prerequisites

- Phase 1 Linux: processes, memory, cgroups, filesystems, `systemd`, logs.
- Phase 2 networking: TCP, DNS, TLS, bandwidth, latency.
- Phase 4 Kubernetes: Pods, scheduling, requests/limits, device plugins.
- Phase 5 AWS or Phase 6 OpenStack: instances, GPUs, quotas, storage, networking.
- Phase 8 observability: Prometheus, Grafana, SLOs, capacity.
- Basic Python and NumPy; comfort with the command line. Deep learning theory is **not** a prerequisite. You need to operate the system, not derive backpropagation.

## Environment and cost

| Lab | Needs | Cost/safety |
|---|---|---|
| L9.1 GPU inspection | Optional NVIDIA GPU or a provided `nvidia-smi` capture; Linux VM | Free if read-only; no GPU stress on a shared host |
| L9.2 Train and serve | 4 vCPU/8 GB Linux VM or macOS; Python 3.11+ | Free; CPU-only path |
| L9.3 Distributed collectives | 4 vCPU/8 GB; `torchrun` with gloo; optional 2 GPUs/NCCL | Free on CPU; GPU path needs a disposable instance |
| L9.4 GPU scheduling | kind cluster; optional real GPU node or cloud GPU | kind is free; a cloud GPU can cost $1–$30+/hour |
| L9.5 MLOps | Docker Compose or local Python; 4 vCPU/8 GB | Free; no external model API keys required |

> **Cost warning:** GPU instances are billed by the second or hour and can fail to stop if a job hangs. Set a budget alert, use a disposable project/account, prefer spot for labs, and put a hard timeout on every job. A single forgotten 8×A100 node can cost more than a month of CPU labs. If you do not have a GPU, do the **CPU/read-only path** in each lab; the reasoning is the same, and the checkpoint accepts the paper evidence.

> **Safety:** do not run `nvidia-smi -r`, MIG reconfiguration, driver installs, or GPU stress tests on a shared or production host. Do not run multi-node NCCL experiments on a network you do not own. Do not download untrusted model weights or run them with your cloud credentials mounted.

## Study order

| Order | Note | Key question |
|---|---|---|
| 1 | [GPU architecture and CUDA](01-gpu-architecture-and-cuda.md) | Where does the time go, and what is the bottleneck? |
| 2 | [Training, inference, and the data path](02-training-inference-and-data.md) | Is the accelerator fed, and what does a token/request cost? |
| 3 | [Distributed training, NCCL, and RDMA](03-distributed-training-nccl-rdma.md) | Does the interconnect make more GPUs slower? |
| 4 | [GPU scheduling: Kubernetes, MIG, Slurm](04-gpu-scheduling-kubernetes-mig-slurm.md) | Who gets the GPU, and how is it isolated? |
| 5 | [MLOps, serving, monitoring, cost](05-mlops-serving-monitoring-cost.md) | Can I ship, observe, and pay for this model? |
| 6 | [Labs part 1](07-labs-part1.md) | Can I inspect, train, and serve? |
| 7 | [Labs part 2](08-labs-part2.md) | Can I schedule, track, and cost a workload? |
| 8 | [Capstone](06-capstone.md) | Can I design a platform and defend the trade-offs? |
| 9 | [Checkpoint](09-checkpoint.md) | Can I diagnose and design under pressure? |

## The stack

```text
┌──────────────────────────────────────────────────────────────────┐
│ Users / applications: notebooks, APIs, batch jobs, agents        │
├──────────────────────────────────────────────────────────────────┤
│ MLOps: experiment tracking, registry, serving, monitoring, cost  │
├──────────────────────────────────────────────────────────────────┤
│ Scheduling: Kubernetes device plugin, MIG, Slurm, quotas         │
├──────────────────────────────────────────────────────────────────┤
│ Distributed runtime: NCCL, MPI, gloo, checkpointing, elastic     │
├──────────────────────────────────────────────────────────────────┤
│ Accelerator: SM, tensor cores, HBM, NVLink/PCIe, driver/CUDA     │
├──────────────────────────────────────────────────────────────────┤
│ Host: CPU, RAM, NUMA, storage, network, kernel, containers       │
└──────────────────────────────────────────────────────────────────┘
```

Every layer can be the bottleneck. The platform engineer's job is to find the layer that is **not** keeping up, prove it with evidence, and fix it without breaking isolation or reproducibility.

## The mental model

1. **A GPU is a throughput device.** It hides latency by running many warps. If there are not enough warps, or they all wait on memory, utilization is low.
2. **Memory is the first constraint.** Weights, gradients, optimizer states, activations, KV cache, and fragmentation all compete for HBM. Estimate before you launch.
3. **Data starvation looks like a slow GPU.** Low utilization with idle gaps usually means the loader, storage, or host-to-device copy is behind.
4. **The interconnect is part of the computer.** NVLink, PCIe, InfiniBand, and RoCE have different bandwidth/latency; a parallel strategy that ignores topology will not scale.
5. **Scheduling is isolation.** A GPU advertised by a device plugin is not automatically isolated; MIG, MPS, taints, and quotas decide who shares what.
6. **Inference is a latency-and-cost problem.** Training optimizes throughput; serving optimizes time-to-first-token, tokens/sec, p95, and cost per request.
7. **A model is an artifact with a supply chain.** Weights, tokenizers, code, container, driver, and CUDA version are all part of the deployable unit.
8. **Observability must include the accelerator.** GPU utilization, memory, ECC/XID, temperature, throttle, NVLink, and collective time are first-class signals.
9. **Failure is normal at scale.** A rank dies, a link flaps, a node loses a GPU, a job hangs in a collective. Design checkpoints and restart paths.
10. **Cost is a reliability property.** A platform that is correct but unaffordable will be shut down; model cost per request and per experiment.

## Core numbers to internalize

| Quantity | Typical order | Why it matters |
|---|---|---|
| HBM capacity per GPU | 16–80 GB (A100 40/80, H100 80, newer higher) | Model + optimizer + activations must fit, or you shard/offload |
| HBM bandwidth | ~0.5–3 TB/s per GPU | Memory-bound kernels are limited by this, not FLOPS |
| PCIe Gen4 x16 | ~32 GB/s per direction (theoretical) | Host-to-device copies and PCIe-only multi-GPU are slow |
| NVLink | ~300–900 GB/s per GPU (generation-dependent) | Tensor parallel within a node is much cheaper than across nodes |
| InfiniBand HDR/NDR | 200/400 Gb/s per port (~25/50 GB/s) | Cross-node all-reduce and storage traffic |
| Ethernet 100 GbE | ~12.5 GB/s | RoCE and storage; latency higher than IB |
| FP32 / TF32 / FP16 / BF16 / FP8 | 4 / 4 / 2 / 2 / 1 bytes per value | Precision changes memory, bandwidth, and compute |
| Model params | billions | 7B FP16 ≈ 14 GB just for weights |
| Tokens | input + output | Inference cost and KV-cache size |

These are orders of magnitude, not a shopping list. Always read the actual device's specification and measure.

## Diagrams

- [GPU memory hierarchy and bottlenecks](../images/gpu-hierarchy.svg)
- [The training pipeline](../images/training-loop.svg)
- [Data, tensor, and pipeline parallelism](../images/parallelism.svg)
- [GPU scheduling: Kubernetes vs Slurm](../images/gpu-scheduling.svg)

## Completion criterion

You can:

- Given a model size, precision, batch size, and sequence length, estimate whether it fits and what the dominant bottleneck is.
- Read `nvidia-smi`, `dmon`, PyTorch memory summary, and an Nsight trace to explain low GPU utilization.
- Explain all-reduce cost and why NVLink vs InfiniBand changes the parallel strategy.
- Diagnose a NCCL hang or timeout with logs, topology, and network counters.
- Schedule a GPU pod with requests, limits, taints, affinity, and MIG; explain what the device plugin does and does not do.
- Submit a Slurm GPU job with GRES, inspect `sinfo`/`squeue`, and explain fair-share/backfill.
- Track an experiment, serve a model, expose metrics, and calculate cost per 1,000 requests.
- Design a multi-tenant AI platform with quotas, isolation, observability, and a cost model.
- Explain the security and supply-chain risks of model weights, containers, and CI credentials.

Then take [the checkpoint](09-checkpoint.md).
