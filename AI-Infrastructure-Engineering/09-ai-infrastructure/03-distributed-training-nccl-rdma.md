# Distributed Training, NCCL, and RDMA

> **The one-sentence model:** distributed training is a communication problem disguised as a compute problem; adding GPUs helps only while the work per byte moved across the slowest link stays high enough.

---

## 1. Why distribute, and when it stops helping

You distribute for two reasons:

1. **Capacity:** the model or batch does not fit on one GPU.
2. **Speed:** one GPU cannot produce enough samples/tokens per hour.

Scaling is measured against a single-GPU baseline:

```text
speedup(N) = throughput(N) / throughput(1)
efficiency(N) = speedup(N) / N
```

If 8 GPUs deliver 6× the throughput of 1 GPU, the speedup is 6× and the efficiency is 75%. The missing 25% is communication, synchronization, data imbalance, or memory pressure.

Amdahl's law for the serial fraction `s`:

```text
speedup(N) = 1 / (s + (1 - s) / N)
```

If 10% of the work is serial, the maximum speedup is 10× no matter how many GPUs you add. In distributed training, the "serial" part is often the collective, the optimizer, the data loader, or a straggler.

### The scaling cliff

```text
efficiency
100% │●
     │ ●●
 75% │   ●●●
     │      ●●●●
 50% │          ●●●●●
     │               ●●●●●●
  0% └──────────────────────────► GPUs
          communication dominates
```

The cliff arrives earlier when:

- the model is small (less compute per parameter);
- the batch per GPU is small (less work per collective);
- the interconnect is slow (PCIe or Ethernet instead of NVLink/IB);
- the parallel strategy is chatty (tensor parallel across nodes);
- one rank is slower (straggler).

More GPUs can make a poorly matched job **slower** in wall-clock time because the collective overhead grows while the per-GPU work shrinks.

---

## 2. Parallelism strategies

### Data parallel (DP)

Each GPU has a full copy of the model and processes a different batch shard. After backward, gradients are all-reduced so every replica applies the same update.

```text
GPU0: batch 0 → forward/backward → grads ─┐
GPU1: batch 1 → forward/backward → grads ─┼─ all-reduce → optimizer → next step
GPU2: batch 2 → forward/backward → grads ─┘
```

- **Pros:** simple, scales throughput, works with any model that fits.
- **Cons:** model must fit on each GPU; gradient all-reduce grows with model size; communication every step.
- **Best for:** models that fit with reasonable batch size, connected by IB/NVLink.

### Tensor parallel (TP)

A layer's weight matrix is split across GPUs; each GPU computes a shard and they combine activations.

```text
GPU0: W[:, 0:H/2] ─┐
                   ├─ all-gather / all-reduce → next layer
GPU1: W[:, H/2:H] ─┘
```

- **Pros:** splits a model too large for one GPU; can be efficient within an NVLink domain.
- **Cons:** chatty per layer; latency-sensitive; poor across PCIe or slow Ethernet.
- **Best for:** large layers, few GPUs, high-bandwidth interconnect.

### Pipeline parallel (PP)

Different layers live on different GPUs; activations flow stage to stage, and microbatches keep the pipeline full.

```text
stage 0: layers 0-7   ──► stage 1: layers 8-15 ──► stage 2: layers 16-23
   ▲ microbatch 1            microbatch 0              microbatch ...
   └── bubble: fill/drain the pipeline
```

- **Pros:** splits depth; less per-step communication than TP; can cross slower links.
- **Cons:** pipeline bubbles, complex scheduling, uneven stage balance.
- **Best for:** deep models, multi-node, memory-bound capacity.

Pipeline bubble fraction (rough):

```text
bubble ≈ (stages - 1) / (microbatches + stages - 1)
```

With 4 stages and 16 microbatches: `3 / 19 ≈ 16%` idle. More microbatches reduce the bubble but increase memory and complexity.

### Expert parallel (EP)

A mixture-of-experts layer routes tokens to different experts on different GPUs. It is a form of all-to-all communication and is very sensitive to load balance and network.

- **Pros:** more parameters without proportional compute per token.
- **Cons:** all-to-all, hot experts, routing imbalance, complex serving.
- **Best for:** large MoE models with a network designed for it.

### Sequence/context parallel

Splits the sequence dimension across GPUs to handle long context. Attention needs communication of keys/values or ring-style attention. It is specialized; do not confuse it with data parallel.

### 3D parallelism

Large training runs combine data × tensor × pipeline (and sometimes expert) parallelism. The placement is driven by topology:

- tensor parallel inside an NVLink domain;
- pipeline across nodes;
- data parallel across the remaining dimension;
- expert parallel where the all-to-all fabric supports it.

The diagram in the README shows the communication pattern of each. The key question is not "which is best?" but "which communication pattern does my interconnect make cheap?"

---

## 3. Collectives

| Collective | What it does | Typical use | Data per rank (ring, large N) |
|---|---|---|---|
| **Broadcast** | One root sends to all | Initial weights, config | S |
| **Reduce** | Combine to one root | Scalar metrics | S |
| **All-reduce** | Combine and give every rank the result | Gradient sync | ~2S |
| **All-gather** | Every rank gets all shards | Tensor parallel, FSDP | ~S |
| **Reduce-scatter** | Reduce and scatter shards | ZeRO/FSDP | ~S |
| **All-to-all** | Every rank exchanges with every rank | MoE, some tensor layouts | ~S |

`S` is the message size per rank. The exact volume depends on the algorithm and the number of ranks.

### Ring all-reduce

For `N` ranks and message size `S` bytes, each rank sends/receives approximately:

```text
bytes_per_rank ≈ 2 × (N - 1) / N × S
```

For large `N`, this approaches `2S` per rank. The total traffic across the ring is `2 × (N - 1) × S`.

Example: 1B parameters in FP16 = 2 GB. With 8 ranks:

```text
bytes_per_rank = 2 × 7/8 × 2 GB = 3.5 GB
```

At an effective 100 GB/s interconnect, the all-reduce takes ~35 ms, ignoring latency. If the step compute is 100 ms, communication is 26% of the step; at 8 GPUs, that caps efficiency.

### Latency and bandwidth

A simple ring model:

```text
time ≈ 2 × (N - 1) × α + 2 × (N - 1) / N × S / B
```

- `α` is per-hop latency; it dominates for small messages.
- `B` is effective bandwidth; it dominates for large messages.
- Tree algorithms reduce latency for small messages; ring is bandwidth-efficient for large messages.

NCCL chooses algorithms/protocols based on message size, topology, and environment. Do not hard-code a choice unless you have measured it.

### Overlap

DDP overlaps gradient all-reduce with backward computation by bucketing gradients. The first buckets are ready before the last layer's backward finishes, so communication hides under compute. If the model is small or the buckets are poorly sized, the overlap is ineffective.

---

## 4. DDP, FSDP, and ZeRO

### DDP

```python
import torch.distributed as dist
from torch.nn.parallel import DistributedDataParallel as DDP

dist.init_process_group("nccl")  # or "gloo" on CPU
model = DDP(model, device_ids=[local_rank])
```

- One process per GPU (usually).
- Each rank reads a disjoint shard via `DistributedSampler`.
- Gradients are all-reduced; buffers are broadcast.
- `find_unused_parameters=True` can fix a hang but adds overhead; prefer fixing the model.

### FSDP / ZeRO

Shards optimizer state, gradients, and parameters across ranks. Before a layer computes, its parameters are all-gathered; after backward, gradients are reduce-scattered. This reduces per-GPU memory at the cost of more communication.

| Stage | Shards | Memory saving | Communication |
|---|---|---|---|
| ZeRO-1 | Optimizer state | High | Similar to DDP |
| ZeRO-2 | Optimizer + gradients | Higher | More |
| ZeRO-3 / FSDP | Optimizer + gradients + parameters | Highest | Most |

FSDP is not free: it changes checkpoint format, requires careful wrapping, and can be slower if the interconnect is weak. Measure step time and memory, not just "it fits."

### Checkpointing distributed state

- **Full checkpoint:** rank 0 gathers or each rank writes its shard; resume with the same world size is simplest.
- **Sharded checkpoint:** each rank writes its shard + metadata; resume can reshard if the tool supports it.
- **Async:** copy to CPU/remote while training continues; watch memory and consistency.

A checkpoint that cannot be resumed on a different world size is an operational constraint. Record it in the runbook.

---

## 5. Topology and RDMA

### Inside a node

- **NVLink/NVSwitch:** high bandwidth, low latency; tensor parallel is viable.
- **PCIe:** lower bandwidth, higher latency; P2P may be disabled by the platform/IOMMU; tensor parallel across PCIe is usually a bad idea.
- **NUMA:** a GPU is attached to a CPU socket; host memory transfers are faster when the process and NIC are on the same socket.

### Across nodes

- **InfiniBand:** RDMA, low latency, lossless fabric, common in GPU clusters. Requires HCAs, switches, subnet manager, and correct cabling.
- **RoCE (RDMA over Converged Ethernet):** RDMA over Ethernet; needs PFC/ECN tuning, MTU consistency, and a lossless (or near-lossless) design.
- **TCP:** ubiquitous, higher latency/CPU overhead; NCCL can fall back to TCP sockets, but performance is much lower.

### GPUDirect RDMA

GPUDirect RDMA lets the NIC DMA directly to/from GPU memory, bypassing host memory. It reduces latency and CPU overhead. Requirements:

- compatible GPU, NIC, driver, and topology;
- IOMMU configuration that allows it;
- NCCL built with the right support;
- `NCCL_NET_GDR_LEVEL` high enough (but verify, do not blindly set it).

If GDR is not active, traffic may bounce through host memory, and bandwidth/latency suffer.

### Rail-optimized networks

A rail-optimized design connects GPU `i` on every node to the same leaf/rail. This keeps all-reduce traffic on one rail when possible and reduces spine congestion. The scheduler should place jobs to preserve rail alignment; a topology-unaware scheduler can scatter a job across rails and degrade performance.

### Topology detection

NCCL detects PCIe/NVLink/IB topology at startup. You can dump it:

```bash
NCCL_TOPO_DUMP_FILE=/tmp/nccl-topo.xml python train.py
```

Read the XML or the INFO logs to see which links NCCL chose. A job that thinks it is using NVLink but is actually on PCIe will be slow; verify.

---

## 6. NCCL configuration and logs

### Debug logging

```bash
NCCL_DEBUG=INFO NCCL_DEBUG_SUBSYS=INIT,NET,COLL \
  python train.py
```

Representative log lines (abridged):

```text
NCCL INFO NCCL version 2.21.5+cuda12.4
NCCL INFO NET/IB : Using [0]mlx5_0:1/IB [RO]; OOB ...
NCCL INFO NET/IB : Using [0]mlx5_1:1/IB [RO]; OOB ...
NCCL INFO Channel 00/0 : 0[0] -> 1[1] [send] via NET/IB/0
NCCL INFO Channel 00/0 : 0[0] -> 2[2] [send] via NET/IB/0
NCCL INFO Ring 00 : 0 -> 1 -> 2 -> 3 -> 0
```

Read:

- `NET/IB` vs `NET/Socket`: IB is the fast path; Socket means TCP fallback.
- `via NET/IB/0` and the HCA names: is it using the intended NIC?
- `Ring`/`Tree`: the algorithm and rank order.
- `P2P`/`SHM`/`NVLS`: intra-node transports.
- Warnings about `NET/IB : No device found` or `Failed to initialize`.
- `NCCL WARN` lines are not decorative; they often precede a hang.

### Common environment variables

| Variable | Purpose | Caution |
|---|---|---|
| `NCCL_DEBUG` | `WARN`, `INFO`, `TRACE` | `TRACE` is huge; use for a short repro |
| `NCCL_DEBUG_SUBSYS` | Limit subsystems (`INIT,NET,COLL`) | Reduces noise |
| `NCCL_DEBUG_FILE` | Write logs to a file | One file per rank; watch disk |
| `NCCL_SOCKET_IFNAME` | Interface for TCP bootstrap | Wrong name = hang |
| `NCCL_IB_HCA` | HCAs to use | Must match actual devices |
| `NCCL_IB_GID_INDEX` | RoCE GID index | Wrong index = no connectivity |
| `NCCL_IB_DISABLE=1` | Force TCP | Debug only; slow |
| `NCCL_P2P_DISABLE=1` | Disable GPU P2P | Debug PCIe/NVLink issues |
| `NCCL_NET_GDR_LEVEL` | GPUDirect RDMA threshold | 0 disables; higher levels require topology |
| `NCCL_TOPO_FILE` / `NCCL_TOPO_DUMP_FILE` | Read/write topology XML | Useful for reproducible placement |
| `NCCL_ALGO` / `NCCL_PROTO` | Force algorithm/protocol | Benchmark, do not cargo-cult |
| `NCCL_IB_TIMEOUT` / `NCCL_IB_RETRY_CNT` | IB retransmission | Tuning, not a fix for a broken link |

Framework-level:

- `TORCH_NCCL_BLOCKING_WAIT=1` makes collectives block and can surface a hang as an error, but changes behavior; use for debugging.
- `TORCH_NCCL_ASYNC_ERROR_HANDLING=1` enables asynchronous error handling (default may vary by version).
- `TORCH_NCCL_HEARTBEAT_TIMEOUT_SEC` controls the heartbeat timeout in some versions; check your version.

Do not copy a list of env vars from a blog into production. Each one changes a decision; measure before and after.

### NCCL tests

```bash
# Inside a GPU node/container with nccl-tests built.
all_reduce_perf -b 8 -e 8G -f 2 -g 8
```

Representative output (abridged):

```text
# nThread 1 nGpus 8 minBytes 8 maxBytes 8589934592 step 2
       size (B)   count    type   redop    root     time (us)   algbw (GB/s)  busbw (GB/s)
            8       2     float     sum      -1        ...          ...           ...
      1048576  262144     float     sum      -1        ...          ...           ...
   1073741824 ...       float     sum      -1        ...          ...           ...
```

- `algbw` is algorithmic bandwidth (`size/time`).
- `busbw` accounts for the ring factor and is the number to compare across GPU counts.
- Run with 1 GPU first for a baseline, then 2, 4, 8. A flat busbw as N grows is good; a collapse indicates communication overhead.
- Use the same message sizes as your model's gradient buckets, not just the largest.

### InfiniBand and RoCE tools

```bash
ibstat
ibv_devinfo -v
ibdev2netdev
rdma link show
ip -br link
ethtool -S <interface> | grep -iE 'err|drop|pause|discard'
```

- `ibstat` shows HCA state, rate, and port state. `PORT_ACTIVE` is required.
- `ibv_devinfo` shows firmware, GID table, and link layer.
- `rdma link` shows RDMA devices and netdev mapping.
- `ethtool -S` shows drops, pause frames, and errors on Ethernet/RoCE.
- A link that is `DOWN` or flapping is a physical/cabling/switch issue, not an NCCL bug.

Bandwidth test:

```bash
# server
ib_write_bw -d mlx5_0 -a
# client
ib_write_bw -d mlx5_0 -a <server-ip>
```

Compare to the expected line rate. A 200 Gb/s HCA should deliver roughly 20–24 GB/s with overhead; if you see 5 GB/s, investigate the link, MTU, or PCIe.

---

## 7. Calculations

### All-reduce time

```text
bytes_per_rank = 2 × (N - 1) / N × S
time ≈ bytes_per_rank / effective_bandwidth + latency_terms
```

Example: 7B parameters in BF16 = 14 GB. N=8, effective IB bandwidth 20 GB/s:

```text
bytes_per_rank = 2 × 7/8 × 14 GB = 24.5 GB
time ≈ 24.5 / 20 = 1.225 s
```

If the step compute is 2 s, the all-reduce is ~38% of the step; scaling efficiency will suffer unless the communication overlaps with backward.

### Pipeline bubble

```text
bubble_fraction ≈ (p - 1) / (m + p - 1)
```

`p=8` stages, `m=8` microbatches: `7/15 ≈ 47%` bubble. Increase microbatches or reduce stages, but watch memory and batch size.

### Scaling efficiency

```text
efficiency(N) = throughput(N) / (N × throughput(1))
```

If 8 GPUs deliver 5.6× the single-GPU throughput, efficiency is 70%. At 32 GPUs, 14× is 44%. The numbers are not failures by themselves; they are inputs to a cost/throughput decision.

### Checkpoint write time

```text
time = checkpoint_bytes / storage_bandwidth
```

A 112 GB training state at 5 GB/s takes ~22 s; at 1 GB/s, ~112 s. If you checkpoint every 10 minutes, the overhead is 3.7% vs 18.7% of wall-clock time, before considering synchronization.

---

## 8. Failure scenarios

### A — NCCL hangs during initialization

**Symptoms:** all ranks print the NCCL version, then hang; no progress; `NCCL_DEBUG=INFO` stops at bootstrap or ring setup.

**Evidence:** NCCL logs from every rank, `NCCL_SOCKET_IFNAME`, hostname resolution, firewall rules, IB port state, and whether all ranks use the same NCCL version.

**Likely causes:** wrong network interface, a firewall blocking the bootstrap/control ports, an IB link down, a hostname that resolves differently on each node, or a mismatched NCCL/container version.

**Fix:** set the correct interface/HCA, open the required ports, verify `ibstat`, and use a minimal `all_reduce_perf` across the same nodes. Do not increase timeouts to hide an init failure.

### B — All-reduce is much slower than expected

**Symptoms:** scaling efficiency collapses at 2+ nodes; `busbw` is far below line rate; NCCL logs show `NET/Socket` instead of `NET/IB`.

**Evidence:** `all_reduce_perf` at 1/2/4/8 GPUs, `nvidia-smi topo -m`, `ibstat`, `NCCL_DEBUG=INFO`, and the NCCL topology dump.

**Likely causes:** TCP fallback, wrong HCA, PCIe P2P disabled, a topology mismatch (job spread across rails), congestion, or a slow rank.

**Fix:** fix the fast path first, then placement. Do not tune `NCCL_ALGO` before confirming the transport.

### C — One rank dies, the job hangs

**Symptoms:** one process exits with an Xid/OOM/illegal access; the remaining ranks wait in a collective forever; GPU utilization drops to zero.

**Evidence:** per-rank logs, `dmesg`, exit codes, NCCL timeout messages, and the job's watchdog configuration.

**Likely causes:** GPU failure, host OOM, a bug in one rank's code path, or a network partition. The collective has no consensus on failure, so peers wait.

**Fix:** enable a watchdog/timeout, kill the whole job, preserve logs, and restart from the last checkpoint. A distributed job needs a failure policy, not just a retry loop.

### D — NCCL timeout after several minutes

**Symptoms:** `NCCL WARN Timeout` or a collective timeout; the job may have been slow for a while before failing.

**Evidence:** NCCL logs with timestamps, per-rank progress, straggler metrics, network counters, and the message size at the timeout.

**Likely causes:** a slow rank (thermal, CPU, storage), congestion, a flapping link, or a collective that is simply too large for the timeout.

**Fix:** find the straggler; check thermal/throttle, data imbalance, and network errors. Do not just raise the timeout; that converts a failure into a longer stall.

### E — RoCE packet loss / pause storms

**Symptoms:** intermittent slow collectives, retries, drops; Ethernet counters show pause/discard; performance degrades under load.

**Evidence:** `ethtool -S`, switch counters, PFC/ECN configuration, MTU, and `NCCL_DEBUG=INFO`.

**Likely causes:** lossy Ethernet, PFC misconfiguration, MTU mismatch, or congestion without ECN.

**Fix:** follow the RoCE design (lossless or near-lossless), verify MTU end to end, and monitor pause/drop counters. This is a network engineering problem, not an NCCL flag.

### F — GPUDirect RDMA not active

**Symptoms:** bandwidth lower than expected; NCCL logs show host-memory bounce; CPU usage high during transfers.

**Evidence:** `NCCL_DEBUG=INFO` GDR lines, `NCCL_NET_GDR_LEVEL`, `nvidia-smi topo -m`, IOMMU config, and NIC/GPU topology.

**Likely causes:** unsupported topology, IOMMU blocking, driver/firmware mismatch, or GDR disabled.

**Fix:** place the NIC and GPU on the same PCIe root complex, enable GDR in the driver/firmware, and verify with a bandwidth test. Do not assume the marketing spec equals the active path.

### G — Straggler rank

**Symptoms:** all ranks wait at the collective; one rank's step time is consistently higher; throughput is set by the slowest.

**Evidence:** per-rank step time, GPU clocks/temperature, data loader time, CPU steal, NUMA, and network counters.

**Likely causes:** thermal throttling, data imbalance, a noisy neighbor, a slower node, or a rank doing extra work (logging, evaluation).

**Fix:** balance shards, isolate noisy workloads, fix cooling, or exclude the slow node. A faster average does not help if the slowest rank gates every step.

### H — Checkpoint cannot resume after changing world size

**Symptoms:** resume fails with shape/key errors; optimizer state mismatch; the job restarts from zero.

**Evidence:** checkpoint metadata, world size, sharding layout, framework version, and the resharding path.

**Likely causes:** sharded checkpoint tied to the original world size, different model parallel layout, or incompatible optimizer state.

**Fix:** use a reshardable checkpoint format, record the layout, and test resume at the target world size. A checkpoint is not a backup unless you have restored it.

### I — Container cannot initialize NCCL

**Symptoms:** `unhandled system error`, `/dev/shm` too small, `IPC` errors, or NCCL falls back to a slow path.

**Evidence:** container runtime flags, `/dev/shm` size, `--ipc`, device mounts, `NCCL_DEBUG=INFO`, and the host's `/dev/nvidia*`/`/dev/infiniband`.

**Likely causes:** small `/dev/shm`, missing IPC namespace, missing devices, or a security profile blocking RDMA.

**Fix:** size `/dev/shm`, use the required IPC/device access, and verify with `all_reduce_perf` inside the container. `--ipc=host` is a security trade-off; prefer a sized shared memory mount where possible.

---

## 9. Command field manual

### `nvidia-smi topo -m` — placement

```bash
nvidia-smi topo -m
```

**Purpose:** see GPU-GPU and GPU-NIC link types.

**Why:** the parallel strategy and job placement depend on it.

**Interpretation:** `NV#` is NVLink; `PIX/PXB/PHB/SYS` are increasing distance. A tensor-parallel job spanning `SYS` is likely to underperform.

**Mistakes:** assuming the spec sheet matches the installed topology; ignoring NIC affinity.

### `NCCL_DEBUG=INFO` — transport and algorithm

```bash
NCCL_DEBUG=INFO NCCL_DEBUG_SUBSYS=INIT,NET,COLL python train.py 2>&1 | tee nccl.log
```

**Purpose:** see NCCL version, transport, HCA, ring/tree, and warnings.

**Why:** it is the first evidence for a hang or a slow collective.

**Interpretation:** `NET/IB` is fast; `NET/Socket` is a fallback. Check the HCA names and ring order. `NCCL WARN` lines matter.

**Mistakes:** running with `TRACE` on a large job and filling the disk; collecting logs from only rank 0.

### `all_reduce_perf` — collective baseline

```bash
all_reduce_perf -b 8 -e 8G -f 2 -g 8
```

**Purpose:** measure all-reduce bandwidth/latency at different sizes.

**Why:** it isolates the network from the training code.

**Interpretation:** compare `busbw` across 1/2/4/8 GPUs and against the link's expected bandwidth. A flat curve is good; a collapse at a specific size suggests a protocol/algorithm transition or a topology issue.

**Mistakes:** testing only the largest message; not pinning the same nodes; comparing different GPU generations.

### `ibstat` / `ibv_devinfo` — IB health

```bash
ibstat
ibv_devinfo -v
```

**Purpose:** HCA state, rate, firmware, GIDs, link layer.

**Why:** an IB link that is down or at half rate explains a slow job.

**Interpretation:** `PORT_ACTIVE`, expected rate, and no symbol errors. `ibv_devinfo` shows the GID table used by RoCE.

**Mistakes:** reading only `ibstat` and ignoring switch-side errors; assuming a link is healthy because it is up.

### `rdma link` / `ethtool -S` — RoCE health

```bash
rdma link show
ethtool -S <iface> | grep -iE 'err|drop|pause|discard'
```

**Purpose:** map RDMA devices to netdevs and inspect drops/pause/errors.

**Why:** RoCE performance depends on a lossless/near-lossless Ethernet configuration.

**Interpretation:** growing drops/pause frames under load indicate congestion or PFC/ECN problems.

**Mistakes:** tuning NCCL while the Ethernet fabric is dropping packets.

### `ib_write_bw` — RDMA bandwidth

```bash
# server
ib_write_bw -d mlx5_0 -a
# client
ib_write_bw -d mlx5_0 -a <server-ip>
```

**Purpose:** measure RDMA write bandwidth independent of NCCL.

**Why:** it separates a network problem from a collective problem.

**Interpretation:** compare to line rate; test both directions and multiple queue pairs. A low result points to link, MTU, PCIe, or CPU affinity.

**Mistakes:** running on a production fabric without coordination; testing only one message size.

### `torchrun` — launch

```bash
torchrun --nproc_per_node=8 --nnodes=2 --node_rank=0 \
  --master_addr=10.0.0.10 --master_port=29500 train.py
```

**Purpose:** start one process per GPU across nodes.

**Why:** the launcher sets rank/world size and environment variables.

**Interpretation:** each rank must see the same `MASTER_ADDR`, `MASTER_PORT`, and `WORLD_SIZE`. A mismatch hangs at init.

**Mistakes:** using `localhost` for multi-node; a firewall blocking port 29500; mixing ranks from different code versions.

### `NCCL_TOPO_DUMP_FILE` — topology evidence

```bash
NCCL_TOPO_DUMP_FILE=/tmp/topo.xml python train.py
```

**Purpose:** dump the topology NCCL detected.

**Why:** it shows whether NCCL sees NVLink/IB or fell back to PCIe/Socket.

**Interpretation:** inspect the XML for the expected GPU/NIC links; compare across nodes.

**Mistakes:** assuming the dump is identical on every node; not collecting it when the job is healthy as a baseline.

---

## 10. Design checklist

- [ ] The model fits the chosen parallel strategy; memory is estimated, not guessed.
- [ ] Tensor parallel stays within NVLink domains; pipeline/data parallel crosses nodes.
- [ ] The interconnect (IB/RoCE) is validated with `all_reduce_perf` and `ib_write_bw`.
- [ ] NCCL logs are captured on failure and on a healthy baseline.
- [ ] The job has a collective timeout/watchdog and a checkpoint/restart policy.
- [ ] Straggler detection is in place (per-rank step time, thermal, data wait).
- [ ] Checkpoints are reshardable or the world-size constraint is documented.
- [ ] The scheduler is topology-aware enough not to scatter a job across rails.
- [ ] Security: RDMA devices, `/dev/shm`, and IPC access are scoped; containers are not privileged by default.
- [ ] Cost: scaling efficiency and GPU-hours per experiment are visible.

---

## 11. Checkpoint

1. Why can adding GPUs make a job slower in wall-clock time?
2. Define speedup and scaling efficiency.
3. Compare data, tensor, pipeline, and expert parallelism by communication pattern.
4. Why is tensor parallel usually kept inside an NVLink domain?
5. What is the pipeline bubble, and how do microbatches reduce it?
6. Write the ring all-reduce bytes per rank. Why is it ~2S for large N?
7. What is the difference between `algbw` and `busbw` in NCCL tests?
8. What does `NET/Socket` in NCCL logs tell you?
9. What does `nvidia-smi topo -m` show that `nvidia-smi` does not?
10. What is GPUDirect RDMA, and what does it bypass?
11. Why does RoCE need PFC/ECN, and what happens without it?
12. How would you diagnose a NCCL hang at initialization?
13. One rank dies; why do the others hang, and what should the job do?
14. Why is raising the NCCL timeout not a fix for a straggler?
15. How do you test a checkpoint resume at a different world size?
16. What is the risk of `--ipc=host` in a container?
17. How do you detect a straggler rank?
18. Why is a topology-unaware scheduler a performance problem?
19. Estimate the all-reduce time for 14 GB of gradients over 8 ranks at 20 GB/s.
20. What evidence would you collect before blaming NCCL itself?
