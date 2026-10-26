# GPU Architecture and CUDA

> **The one-sentence model:** a GPU is a throughput machine that hides memory latency by switching between many warps; it is fast only when there are enough independent warps, enough data in flight, and enough work per byte moved.

---

## 1. CPU vs GPU: latency vs throughput

| Property | CPU | GPU |
|---|---|---|
| Design goal | Minimize latency of a few threads | Maximize throughput of many threads |
| Cores | Few, complex, out-of-order | Many, simple, in-order SIMT |
| Cache | Large, deep hierarchy | Small per-SM, shared L2, massive HBM |
| Control | Branch prediction, speculative execution | Warp scheduling, masked divergence |
| Memory | DDR, NUMA, moderate bandwidth | HBM, very high bandwidth |
| Best at | Branchy, serial, latency-sensitive code | Regular, parallel, data-heavy math |

A CPU core is optimized to finish one instruction stream quickly. A GPU is optimized to keep thousands of threads in flight so that when one warp waits on memory, another can run. That is why a GPU can be "100% utilized" and still deliver a fraction of its peak: utilization means at least one warp was resident, not that the math units were busy.

```text
CPU:  1 thread  ──latency──►  cache miss  ──stall──►  context switch
GPU:  warp 0 ──► memory wait
      warp 1 ──► execute
      warp 2 ──► execute
      warp 3 ──► memory wait
      ... scheduler picks a ready warp every cycle
```

The GPU's bet is that there is enough parallelism to hide latency. If your batch is too small, your sequence is too short, or your kernel has a dependency chain, the bet fails.

---

## 2. The CUDA execution model

### Grid, block, warp, thread

```text
grid
├── block 0
│   ├── warp 0  (32 threads)
│   ├── warp 1
│   └── ...
├── block 1
│   └── ...
└── ...
```

| Term | Meaning | Practical consequence |
|---|---|---|
| **Thread** | One lane of execution | Has registers and local memory |
| **Warp** | 32 threads scheduled together | Divergence within a warp serializes both paths |
| **Block** | Group of warps sharing shared memory | Block size affects occupancy and synchronization |
| **Grid** | All blocks for a kernel | Blocks are independent unless you use cooperative groups |
| **SM** | Streaming multiprocessor | Contains warp schedulers, CUDA cores, tensor cores, shared memory/L1 |
| **Kernel** | A function launched over a grid | Launch overhead matters for small work |

A warp executes one instruction for all 32 lanes. If lanes take different branches, the warp executes each path with the other lanes masked. This is **warp divergence**: branchy code can be much slower than the instruction count suggests.

### Occupancy

Occupancy is the ratio of resident warps to the maximum supported warps per SM. It is limited by:

- registers per thread;
- shared memory per block;
- block size and blocks per SM;
- hardware limits (max warps, max blocks).

High occupancy is not automatically good: a kernel can have high occupancy and still be memory-bound. Low occupancy can be fine for a compute-heavy kernel with enough instruction-level parallelism. Use occupancy as a diagnostic, not a target.

### Why small batches hurt

A matrix multiply with batch size 1 may not have enough parallel work to fill the GPU. The kernel launch and memory latency dominate. This is why inference servers batch requests, and why "GPU utilization 30%" on a batch-1 service is not necessarily a bug — it may be latency-optimized, not throughput-optimized.

---

## 3. Memory hierarchy

```text
thread
  │ registers          fastest, per-thread, limited
  ▼
warp / block
  │ shared memory      programmer-managed, per-block, low latency
  │ L1 cache           per-SM, shared with shared memory
  ▼
GPU
  │ L2 cache           shared across SMs, larger, higher latency
  ▼
HBM                   large, very high bandwidth, highest latency
  ▲
  │ PCIe / NVLink
host RAM
```

| Level | Scope | Typical latency | Bandwidth | Controlled by |
|---|---|---|---|---|
| Registers | Thread | ~1 cycle | Highest | Compiler |
| Shared memory / L1 | Block / SM | ~20–30 cycles | High | Programmer / cache |
| L2 | GPU | ~200 cycles | High | Hardware |
| HBM | GPU | ~400–800 cycles | Very high | Hardware |
| Host RAM over PCIe | Host | Microseconds | ~25–50 GB/s | DMA |
| Remote GPU over IB | Cluster | Tens of microseconds | ~12–50 GB/s | NCCL/MPI |

### Coalescing

Global memory is fetched in cache lines (typically 32/64/128 bytes). If consecutive threads access consecutive addresses, the warp's accesses coalesce into a small number of transactions. If they access strided or scattered addresses, the hardware issues many transactions and effective bandwidth collapses.

```text
coalesced:    thread 0 -> addr 0
              thread 1 -> addr 4
              thread 2 -> addr 8
              ... one contiguous line

strided:      thread 0 -> addr 0
              thread 1 -> addr 1024
              thread 2 -> addr 2048
              ... many lines, most bytes unused
```

A transpose or a badly laid-out tensor can turn a bandwidth-bound kernel into a latency-bound one. This is why tensor layouts (`NCHW` vs `NHWC`, row-major vs column-major) matter in real frameworks.

### Bank conflicts

Shared memory is divided into banks. If multiple threads in a warp access different addresses in the same bank, the accesses serialize. Padding a shared-memory array or changing the access pattern can remove the conflict. This is a micro-optimization; profile before rewriting a kernel.

### HBM capacity and fragmentation

HBM is large but finite. The allocator can fail to find a contiguous block even when total free memory is sufficient. PyTorch's caching allocator reserves blocks; fragmentation can cause an OOM at a lower reported usage. `torch.cuda.memory_summary()` shows allocated vs reserved vs free.

---

## 4. Precision and tensor cores

| Format | Bits | Bytes/value | Range/notes | Typical use |
|---|---:|---:|---|---|
| FP32 | 32 | 4 | Wide range, precise | Baselines, some accumulations |
| TF32 | 19 | 4 (stored) | 10-bit mantissa, 8-bit exponent; tensor-core mode | A100+ matmul acceleration |
| FP16 | 16 | 2 | 5-bit exponent; overflow risk | Mixed-precision training/inference |
| BF16 | 16 | 2 | 8-bit exponent; less precision, more range | Training on Ampere+ |
| FP8 | 8 | 1 | E4M3/E5M2 variants; range/precision trade-offs | Hopper/Blackwell inference/training |
| INT8 | 8 | 1 | Quantized integers | Inference |

Tensor cores perform matrix multiply-accumulate at much higher throughput than CUDA cores, often with FP16/BF16/FP8 inputs and FP32 accumulation. The exact throughput depends on the generation and the shape; small or irregular matrices may not use tensor cores efficiently.

Numerical issues:

- FP16 can overflow in large reductions; use loss scaling or BF16.
- BF16 has less mantissa precision; some models are sensitive.
- FP8 needs per-tensor/per-channel scaling; a bad scale destroys accuracy.
- Accumulating in FP32 is common even when inputs are lower precision.

Precision is an infrastructure decision because it changes memory, bandwidth, throughput, and correctness. A model that fits in FP16 may not fit in FP32; a model that is accurate in BF16 may fail in FP8 without calibration.

---

## 5. Interconnect and topology

| Link | Bandwidth (order) | Latency | Use |
|---|---|---|---|
| PCIe Gen4 x16 | ~32 GB/s per direction | Higher | Host-device, PCIe-only multi-GPU |
| PCIe Gen5 x16 | ~64 GB/s per direction | Higher | Newer hosts |
| NVLink (per GPU) | ~300–900 GB/s | Low | GPU-GPU within a node |
| NVSwitch | Full fabric among GPUs | Low | Large NVLink domains |
| InfiniBand HDR | 200 Gb/s (~25 GB/s) | Low | Cross-node collectives/storage |
| InfiniBand NDR | 400 Gb/s (~50 GB/s) | Low | Newer clusters |
| RoCE (100/400 GbE) | 12.5/50 GB/s | Higher than IB | Ethernet-based RDMA |

Topology matters:

- **Same node, NVLink:** tensor parallel and all-reduce are relatively cheap.
- **Same node, PCIe only:** tensor parallel becomes chatty and slow; prefer data parallel or pipeline.
- **Across nodes, InfiniBand:** data parallel all-reduce is feasible; tensor parallel across nodes is expensive.
- **NUMA:** a GPU attached to CPU socket 0 is closer to that socket's memory. A process pinned to the wrong socket pays a cross-socket penalty on host transfers.

`nvidia-smi topo -m` prints the matrix:

```text
        GPU0    GPU1    GPU2    GPU3    NIC0    CPU Affinity    NUMA Affinity
GPU0     X      NV12    NV12    NV12    PXB     0-31            0
GPU1    NV12     X      NV12    NV12    PXB     0-31            0
GPU2    NV12    NV12     X      NV12    PXB     32-63           1
GPU3    NV12    NV12    NV12     X      PXB     32-63           1
NIC0    PXB     PXB     PXB     PXB      X
```

Legend (varies by version): `NV#` = NVLink with # links, `PIX` = same PCIe switch, `PXB` = different PCIe switch under the same host bridge, `PHB` = same NUMA node via host bridge, `SYS` = across NUMA nodes. Read the legend printed by your version; do not memorize letters.

A common mistake is to assume all 8 GPUs in a node have the same link. They may be split across two CPU sockets, with NVLink only within each group. A tensor-parallel job spread across the slow boundary will underperform.

---

## 6. The software stack

```text
application (PyTorch, TensorFlow, JAX, custom CUDA)
        │
frameworks / libraries (cuDNN, cuBLAS, NCCL, FlashAttention)
        │
CUDA runtime (libcudart) + driver API
        │
NVIDIA driver (kernel module + user-space libraries)
        │
GPU hardware
```

Version compatibility is a real operational problem:

- The **driver** supports a maximum CUDA version. A container built for a newer CUDA than the driver supports will fail at runtime.
- The **CUDA toolkit** ships compiler, libraries, and headers. The runtime version must be compatible with the driver.
- **cuDNN/NCCL** versions must match the framework's expectations; a mismatch can cause a crash or silent performance loss.
- **PyTorch** wheels bundle their own CUDA runtime; the system driver still has to be new enough.
- **Container images** may include a CUDA runtime but not the driver; the driver is mounted from the host by the NVIDIA container runtime.

Check:

```bash
nvidia-smi --query-gpu=driver_version,cuda_version --format=csv
nvcc --version
python -c "import torch; print(torch.__version__, torch.version.cuda, torch.cuda.is_available())"
```

`nvidia-smi` reports the driver's maximum supported CUDA version, not the toolkit version. `nvcc --version` reports the toolkit. `torch.version.cuda` reports the CUDA runtime PyTorch was built against. They can differ within compatibility limits; a major mismatch is a finding.

---

## 7. Measuring the GPU

### `nvidia-smi` — the first look

```bash
nvidia-smi
```

Representative output:

```text
+-----------------------------------------------------------------------------+
| NVIDIA-SMI 550.54.15    Driver Version: 550.54.15    CUDA Version: 12.4     |
|-------------------------------+----------------------+----------------------+
| GPU  Name        Persistence-M| Bus-Id        Disp.A | Volatile Uncorr. ECC |
| Fan  Temp  Perf  Pwr:Usage/Cap|         Memory-Usage | GPU-Util  Compute M. |
|===============================+======================+======================|
|   0  NVIDIA A100-SXM4-80GB  On | 00000000:07:00.0 Off |                    0 |
| N/A   38C    P0   250W / 400W |  68234MiB / 81920MiB |     97%      Default |
+-------------------------------+----------------------+----------------------+
| Processes:                                                                  |
|  GPU   GI   CI        PID   Type   Process name                  GPU Memory |
|       0   N/A  N/A     12345      C   python train.py                68000MiB |
+-----------------------------------------------------------------------------+
```

Read:

- **GPU-Util** is the percentage of time at least one kernel was active, sampled over the interval. It is not a measure of math-unit utilization.
- **Memory-Usage** is allocated by processes, not necessarily actively used.
- **Temp/Pwr** show thermal/power headroom. `P0` is max performance state; `P2` may indicate a lower state.
- **ECC** column: volatile uncorrected errors are a hardware warning.
- **Compute M.** default/exclusive/Prohibited; MIG changes the view.

Common mistake: seeing 97% and concluding the model is efficient. A memory-bound kernel can show 97% while delivering 20% of peak FLOPS.

### Query specific fields

```bash
nvidia-smi --query-gpu=index,name,memory.total,memory.used,utilization.gpu,utilization.memory,temperature.gpu,power.draw,clocks_throttle_reasons.active \
  --format=csv,noheader
```

Representative:

```text
0, NVIDIA A100-SXM4-80GB, 81920 MiB, 68234 MiB, 97 %, 88 %, 38, 250.12 W, 0x0000000000000000
```

`utilization.memory` is the percentage of time the memory controller was active, not the fraction of bandwidth used. A value of 88% with 97% GPU util suggests a memory-heavy workload; confirm with Nsight or a roofline estimate.

### `nvidia-smi topo -m` — topology

```bash
nvidia-smi topo -m
```

Use it before choosing tensor/pipeline parallelism, and when a multi-GPU job scales worse than expected. A `SYS` link between GPUs that are supposed to be NVLink-connected is a red flag (wrong PCIe slot, wrong riser, or a virtualized passthrough limitation).

### `nvidia-smi dmon` — time series

```bash
nvidia-smi dmon -s pucvmet -d 1
```

Fields (varies): `p` power, `u` utilization, `c` clocks, `v` violations, `m` memory, `e` ECC, `t` temperature. Use it to see whether utilization is sustained or bursty, and whether throttling coincides with load.

### `nvidia-smi pmon` — per-process

```bash
nvidia-smi pmon -s um -d 1
```

Shows per-process GPU utilization and memory. Useful when multiple jobs share a GPU and you need to attribute the load. It may not show processes in other containers without the right permissions.

### DCGM

```bash
dcgmi discovery -l
dcgmi dmon -e 203,204,1001,1002,1003,1004,1005
dcgmi diag -r 2
```

- `discovery` lists GPUs.
- `dmon` streams field IDs; field IDs vary by DCGM version. Check `dcgmi dmon --list-fields` or the docs for your version.
- `diag -r 2` runs a diagnostic; higher levels are more intrusive and should not run on a production GPU without a drain.

DCGM is what the DCGM exporter uses to feed Prometheus. It is the right source for GPU metrics at fleet scale; `nvidia-smi` parsing is brittle.

### Nsight Systems — the timeline

```bash
nsys profile -o /tmp/train_profile --trace=cuda,nvtx,osrt python train.py
nsys stats /tmp/train_profile.nsys-rep
```

Nsight Systems shows CPU threads, CUDA API calls, kernels, memory copies, and gaps on a timeline. Look for:

- long `cudaMemcpy` / H2D gaps;
- kernels serialized by dependencies;
- CPU-side data loading that leaves the GPU idle;
- many tiny kernels with launch overhead;
- NCCL calls waiting on peers.

### Nsight Compute — one kernel

```bash
ncu --set full -o /tmp/kernel_profile python train.py
ncu --import /tmp/kernel_profile.ncu-rep
```

Nsight Compute reports memory throughput, compute throughput, occupancy, warp stalls, and roofline data for individual kernels. Use it when you know which kernel is slow. It adds overhead and should not be run on a latency-sensitive production service without care.

### PyTorch memory and profiler

```python
import torch

print(torch.cuda.is_available())
print(torch.cuda.get_device_name(0))
print(torch.cuda.memory_summary(device=0, abbreviated=False))
print("allocated", torch.cuda.memory_allocated(0))
print("reserved ", torch.cuda.memory_reserved(0))
print("max alloc", torch.cuda.max_memory_allocated(0))
```

```python
from torch.profiler import profile, ProfilerActivity

with profile(activities=[ProfilerActivity.CPU, ProfilerActivity.CUDA]) as prof:
    train_step()
print(prof.key_averages().table(sort_by="cuda_time_total", row_limit=20))
```

The memory summary distinguishes allocated (tensors), reserved (caching allocator), and free. A large reserved-but-unallocated block is fragmentation or cache, not necessarily a leak.

---

## 8. Roofline and memory math

### Arithmetic intensity

```text
arithmetic_intensity = FLOPs / bytes_moved
```

A kernel is **memory-bound** if its intensity is below the machine's ridge point; **compute-bound** if above.

```text
ridge_point = peak_FLOPS / peak_bandwidth
```

Approximate A100:

- FP16 tensor: ~312 TFLOPS, HBM ~1.55 TB/s → ridge ≈ 201 FLOP/byte.
- FP32: ~19.5 TFLOPS, HBM ~1.55 TB/s → ridge ≈ 12.6 FLOP/byte.

A vector add (`C = A + B`) moves 3 arrays and does 1 FLOP per element: intensity ≈ 1/12 FLOP/byte in FP32, far below the ridge. It is memory-bound; more FLOPS will not help. A large matmul has high intensity and can use tensor cores.

### Model memory

A rough training footprint:

```text
weights        = params × bytes_per_param
gradients      = params × bytes_per_param
optimizer      = params × optimizer_bytes_per_param
activations    = batch × seq × hidden × layers × bytes × factor
```

Example: 7B parameters, mixed precision:

- FP16 weights: `7e9 × 2 = 14 GB`.
- FP16 gradients: 14 GB.
- Adam FP32 master + m + v: `7e9 × 12 = 84 GB`.
- Total before activations: ~112 GB. It does not fit on one 80 GB GPU. This is why ZeRO/FSDP, optimizer sharding, and offload exist.
- Pure inference FP16 weights: 14 GB, plus KV cache and activations.

These are estimates. Framework overhead, alignment, temporary buffers, and fragmentation add 10–30%.

### KV cache

For a transformer with `L` layers, hidden size `H`, sequence length `S`, batch `B`, and `b` bytes per value:

```text
KV_bytes ≈ 2 × L × H × S × B × b
```

Example: `L=32`, `H=4096`, `S=4096`, `B=8`, FP16 (`b=2`):

```text
2 × 32 × 4096 × 4096 × 8 × 2
= 17,179,869,184 bytes
≈ 16 GiB
```

That is KV cache alone, on top of weights. Long context and large batches are expensive. PagedAttention/vLLM and KV-cache quantization exist because this dominates inference memory.

### Bandwidth time

```text
time = bytes / bandwidth
```

Copy 1 GB over PCIe Gen4 x16 at ~25 GB/s effective: 40 ms. Over NVLink at ~300 GB/s: 3.3 ms. If a training step copies activations or gradients across PCIe every step, the link can become the bottleneck even though each GPU is fast.

---

## 9. Failure scenarios

### A — GPU utilization is 100%, but throughput is low

**Symptoms:** `nvidia-smi` shows 95–100% utilization; tokens/sec or samples/sec is far below expectation; power draw is below the cap.

**Evidence:** `nvidia-smi dmon`, `ncu` roofline, kernel time breakdown, batch size, sequence length, memory bandwidth counters, and whether the kernel uses tensor cores.

**Likely causes:** memory-bound kernels, small matrices, poor occupancy, non-coalesced access, FP32 instead of TF32/FP16, excessive synchronization, or a kernel that is busy waiting.

**Fix:** increase batch/sequence work per kernel where latency allows, use a fused/memory-efficient implementation, change precision, or accept that the workload is memory-bound and optimize bytes moved.

### B — CUDA out of memory despite the model fitting on paper

**Symptoms:** `torch.cuda.OutOfMemoryError`; `nvidia-smi` shows memory used near capacity; the estimate said it should fit.

**Evidence:** `torch.cuda.memory_summary`, allocated vs reserved, batch size, sequence length, activation checkpointing, other processes on the GPU, and whether the model is in training or eval mode.

**Likely causes:** activations dominate, fragmentation, a second process, a memory leak (retaining tensors/graphs), a larger batch than intended, or an optimizer state you forgot.

**Fix:** reduce batch or sequence length, enable activation checkpointing, free cached memory, shard with FSDP/ZeRO, or use a larger GPU. Do not blindly call `torch.cuda.empty_cache()` in a loop; it treats the symptom, not the allocation.

### C — Xid 79: GPU has fallen off the bus

**Symptoms:** `nvidia-smi` fails or shows the GPU missing; `dmesg` shows `NVRM: Xid (PCI:...): 79`; jobs crash.

**Evidence:** `dmesg -T`, `journalctl -k`, `nvidia-smi -q`, `lspci`, BMC/system logs, power/thermal history, and whether the GPU is still visible after a host reboot.

**Likely causes:** hardware failure, power delivery, overheating, riser/PCIe issue, or a driver/firmware bug.

**Fix:** drain the node, preserve logs, power-cycle the host if policy allows, and RMA/replace the GPU if it recurs. Do not keep scheduling jobs on a node with a fallen-off GPU. Xid 79 is a hardware-level signal, not a software bug you can patch around.

### D — Xid 48 / ECC errors

**Symptoms:** `nvidia-smi -q` shows uncorrected ECC errors; Xid 48 or 63; random wrong results or crashes.

**Evidence:** ECC error counts, row remap status, `nvidia-smi -q -d ECC`, DCGM health, and the job's correctness checks.

**Likely causes:** HBM degradation, overheating, or a persistent hardware fault.

**Fix:** drain the GPU, run the vendor diagnostic, and replace/repair if uncorrected errors persist. Do not ignore corrected ECC errors at scale; they are an early warning.

### E — Driver/CUDA mismatch after an upgrade

**Symptoms:** containers fail with `CUDA driver version is insufficient for CUDA runtime version`; PyTorch reports `torch.cuda.is_available() == False`; `nvidia-smi` works but the app cannot initialize CUDA.

**Evidence:** `nvidia-smi` driver/CUDA version, `nvcc --version`, `torch.version.cuda`, container image CUDA tag, and the NVIDIA container runtime logs.

**Likely causes:** the host driver is older than the container's CUDA runtime, the container lacks the driver mount, or the image was built for a different architecture.

**Fix:** align the driver and runtime within the compatibility matrix; use a container built for the host driver; do not install a second driver inside the container. Roll back the driver if the upgrade broke a validated stack.

### F — GPU visible on the host, invisible in the container

**Symptoms:** `nvidia-smi` works on the host; inside the container it says "No devices were found"; PyTorch sees zero GPUs.

**Evidence:** `docker run --rm --gpus all nvidia/cuda:12.4.0-base-ubuntu22.04 nvidia-smi`, NVIDIA container toolkit status, `NVIDIA_VISIBLE_DEVICES`, device cgroup rules, and the container runtime config.

**Likely causes:** the NVIDIA container runtime is not configured, `--gpus` is missing, the device plugin/runtime class is wrong in Kubernetes, or the container is scheduled on a node without GPUs.

**Fix:** install/configure the NVIDIA container toolkit, pass the right runtime/device requests, and verify with a minimal CUDA image before debugging the app.

### G — MIG partition not visible or wrong size

**Symptoms:** the host shows a MIG device, but the container sees a different profile or no device; memory/utilization are confusing.

**Evidence:** `nvidia-smi -L`, `nvidia-smi mig -lgip`, `nvidia-smi mig -lgi`, the container's `NVIDIA_VISIBLE_DEVICES`, Kubernetes MIG labels/resources, and the device plugin logs.

**Likely causes:** the MIG configuration changed, the container requests a full GPU instead of a MIG resource, the device plugin is not MIG-aware, or the node labels are stale.

**Fix:** align the MIG profile, resource name, and container request; drain/reconfigure the node; verify with `nvidia-smi -L` inside the pod.

### H — Thermal or power throttling

**Symptoms:** clocks drop below boost; power draw at the cap; utilization high but throughput low; temperature near the limit.

**Evidence:** `nvidia-smi --query-gpu=clocks_throttle_reasons.active,temperature.gpu,power.draw,power.limit --format=csv`, `dmon`, and the chassis inlet temperature.

**Likely causes:** poor airflow, high ambient temperature, a fan failure, a power cap, or a dense chassis running all GPUs at once.

**Fix:** fix cooling/airflow, adjust the power cap if policy allows, spread workloads, or accept a lower sustained clock. Do not "fix" it by disabling thermal protection.

### I — A job hangs in a collective

**Symptoms:** all ranks wait; GPU utilization drops to zero; NCCL logs show a timeout or a mismatch; the job never completes.

**Evidence:** NCCL logs (`NCCL_DEBUG=INFO`), rank-to-node mapping, `nvidia-smi topo -m`, IB counters, firewall/MTU, and whether one rank crashed or is slower.

**Likely causes:** a rank died, a network partition, a mismatched NCCL version, an MTU/ROCE issue, a blocked port, or a straggler.

**Fix:** kill the job cleanly, preserve logs, test the network path with `ib_write_bw`/`all_reduce_perf`, and fix the failing rank/link. Add a watchdog and checkpointing so a hang does not burn GPU hours.

### J — PCIe/NUMA topology mismatch

**Symptoms:** multi-GPU scaling is poor; host-to-device copies are slow; one GPU pair is much slower than another.

**Evidence:** `nvidia-smi topo -m`, `lscpu`, `numactl -H`, PCIe link speed/width, and per-GPU copy benchmarks.

**Likely causes:** GPUs on different CPU sockets, a PCIe riser with fewer lanes, a process pinned to the wrong NUMA node, or a VM with limited passthrough.

**Fix:** place the job on the right NUMA node, use GPUs with the expected link, and avoid tensor parallel across a slow boundary. Measure the actual topology; do not trust the spec sheet.

---

## 10. Command field manual

### `nvidia-smi` — inventory and state

```bash
nvidia-smi
nvidia-smi -L
nvidia-smi --query-gpu=index,name,uuid,memory.total,memory.used,utilization.gpu,temperature.gpu,power.draw,driver_version --format=csv
```

**Purpose:** see devices, driver, memory, utilization, temperature, power, and processes.

**Why:** it is the lowest-friction check and the first evidence in any GPU incident.

**Options:** `-L` lists GPUs and MIG devices; `--query-gpu` selects fields; `--format=csv` is script-friendly; `-i` selects a GPU.

**Interpretation:** `GPU-Util` is activity, not efficiency; memory used includes cache; `P0` is max performance; ECC/XID fields are health signals.

**Scenario:** a user says "the GPU is broken." Start with `nvidia-smi`; if it fails, check `dmesg` and `lspci` before reinstalling the driver.

**Mistakes:** treating 100% utilization as good; ignoring "N/A" fields on MIG; running `nvidia-smi -r` on a shared host.

### `nvidia-smi -q` — detailed health

```bash
nvidia-smi -q
nvidia-smi -q -d ECC,MEMORY,UTILIZATION,PCI
```

**Purpose:** detailed inventory, ECC, PCIe, power, clocks, and throttle reasons.

**Why:** `nvidia-smi` summary hides ECC and throttle details that explain a failure.

**Interpretation:** look for uncorrected ECC, "Pending Page Blacklist", "Yes" under "Row Remap Failure", and active throttle reasons. A `P0` state with a low clock and a throttle reason is thermal/power, not a driver bug.

**Mistakes:** pasting the full output into a ticket without the relevant section; missing that a GPU is in "Exclusive Process" mode.

### `nvidia-smi topo -m` — topology

```bash
nvidia-smi topo -m
```

**Purpose:** show the link matrix between GPUs and NICs.

**Why:** parallel strategy and job placement depend on it.

**Interpretation:** `NV#` = NVLink, `PIX/PXB/PHB/SYS` = increasing distance; `NIC` rows show which GPU is close to which network card. A `SYS` link across GPUs is a warning for tensor parallel.

**Mistakes:** assuming all GPUs in a node are equally connected; ignoring the NIC affinity when running multi-node jobs.

### `nvidia-smi dmon` — streaming metrics

```bash
nvidia-smi dmon -s pucvmet -d 1
```

**Purpose:** sample power, utilization, clocks, violations, memory, ECC, temperature over time.

**Why:** a single snapshot can miss bursts, throttling, and idle gaps.

**Interpretation:** sustained `sm` near 100 with low power may be memory-bound; periodic drops to 0 may be data loading; `thrm`/`pwr` violations explain clock drops.

**Mistakes:** running it at a 100 ms interval on a large fleet and overwhelming the management interface; not correlating with the job's step time.

### `nvidia-smi nvlink` — link health

```bash
nvidia-smi nvlink --status
nvidia-smi nvlink --errorcounter
```

**Purpose:** check NVLink speed, width, and error counters.

**Why:** a degraded NVLink link can silently reduce multi-GPU bandwidth.

**Interpretation:** compare link speed/width to the expected generation; CRC/data errors that increase during load are a hardware/thermal signal.

**Mistakes:** resetting counters before capturing evidence; assuming a link is healthy because the GPU is visible.

### `dcgmi` — fleet-grade GPU telemetry

```bash
dcgmi discovery -l
dcgmi dmon -e 203,204,1001,1002,1003,1004,1005
dcgmi diag -r 2
```

**Purpose:** inventory, continuous metrics, and diagnostics via DCGM.

**Why:** parsing `nvidia-smi` text is brittle; DCGM is the supported telemetry path and the source for the DCGM exporter.

**Interpretation:** field IDs differ by version; use `dcgmi dmon --list-fields`. Diagnostic levels 1–4 increase intrusiveness; level 3+ should be run on a drained GPU.

**Mistakes:** running diagnostics on a production GPU during peak; assuming a missing metric means zero.

### `nsys` — timeline

```bash
nsys profile -o /tmp/profile --trace=cuda,nvtx,osrt python train.py
nsys stats /tmp/profile.nsys-rep
```

**Purpose:** capture CPU/GPU timeline, CUDA API calls, kernels, copies, and gaps.

**Why:** it answers "what was the GPU waiting for?" better than a utilization number.

**Interpretation:** a long gap between kernels with no GPU work means the host/loader is late; a long H2D copy means PCIe; many tiny kernels mean launch overhead; NCCL waits mean peers/network.

**Mistakes:** profiling a distributed job without tracing all ranks; forgetting that profiling adds overhead; not adding NVTX ranges to label phases.

### `ncu` — kernel analysis

```bash
ncu --set full -o /tmp/kernel python train.py
ncu --import /tmp/kernel.ncu-rep
```

**Purpose:** per-kernel memory/compute throughput, occupancy, stalls, roofline.

**Why:** it tells you whether a specific kernel is memory-bound, compute-bound, or latency-bound.

**Interpretation:** high memory throughput near peak with low compute = memory-bound; low occupancy with high stall = latency; uncoalesced accesses show in memory transactions per request.

**Mistakes:** profiling every kernel (huge overhead); changing code based on one kernel without checking end-to-end time.

### `nvcc` and CUDA runtime

```bash
nvcc --version
python -c "import torch; print(torch.__version__, torch.version.cuda, torch.cuda.is_available())"
```

**Purpose:** verify toolkit/runtime/framework compatibility.

**Interpretation:** `nvidia-smi` CUDA version is the driver's maximum; `nvcc` is the toolkit; `torch.version.cuda` is the runtime. A container runtime newer than the driver is a likely failure.

**Mistakes:** installing a toolkit to "fix" a driver mismatch; mixing conda CUDA packages with system CUDA libraries.

### `dmesg` / `journalctl` — kernel and Xid

```bash
sudo dmesg -T | grep -iE 'nvrm|xid|nvidia|pcie'
sudo journalctl -k --since '1 hour ago' | grep -iE 'nvrm|xid|nvidia'
```

**Purpose:** find kernel-level GPU errors, PCIe AER, Xid, and driver messages.

**Why:** `nvidia-smi` may hide the reason a GPU disappeared.

**Interpretation:** Xid 79 (fallen off bus), 48 (double-bit ECC), 63 (row remap), 13/31 (illegal access) are common. Match the Xid to the vendor's guidance; do not guess.

**Mistakes:** rebooting before capturing logs; ignoring PCIe AER errors that precede a GPU drop.

### PyTorch memory summary

```python
import torch
print(torch.cuda.memory_summary(0, abbreviated=False))
```

**Purpose:** show allocated/reserved/active blocks, fragmentation, and CUDA memory stats.

**Why:** `nvidia-smi` shows process memory, not allocator state.

**Interpretation:** reserved >> allocated means cache/fragmentation; a growing allocated value across steps suggests a leak; `max_memory_allocated` tells you the peak.

**Mistakes:** calling `empty_cache()` every step (slow); assuming reserved memory is leaked; forgetting other processes.

---

## 11. Design checklist

- [ ] The workload's arithmetic intensity and memory footprint are estimated before launch.
- [ ] The GPU generation, HBM capacity, interconnect, and driver/CUDA versions are recorded.
- [ ] The job requests the right GPU count and topology; it does not assume all GPUs are equal.
- [ ] Batch size and sequence length are chosen from memory and latency targets, not defaults.
- [ ] Precision (FP32/TF32/FP16/BF16/FP8) is a deliberate accuracy/throughput trade-off.
- [ ] GPU metrics (utilization, memory, ECC, Xid, throttle, NVLink) are exported to the monitoring system.
- [ ] A low-utilization alert includes the data pipeline and host, not just the GPU.
- [ ] A GPU health failure drains the node and preserves logs; it does not silently reschedule.
- [ ] A hung collective has a timeout, checkpoint, and restart path.
- [ ] Cost per GPU-hour and per experiment is visible to the team.

---

## 12. Checkpoint

1. Why can a GPU show 100% utilization while delivering 20% of peak FLOPS?
2. What is a warp, and why does divergence cost time?
3. What limits occupancy, and why is high occupancy not always the goal?
4. Explain coalesced vs strided memory access with an example.
5. What is the difference between HBM, L2, shared memory, and registers?
6. What is arithmetic intensity, and how do you use the roofline to classify a kernel?
7. Why is a vector add memory-bound on a GPU?
8. Estimate the FP16 weights, gradients, and Adam states for a 7B model. Does it fit on 80 GB?
9. Estimate the KV cache for a 32-layer, 4096-hidden model at batch 8 and 4096 tokens in FP16.
10. Why does NVLink vs PCIe change the best parallelism strategy?
11. What does `nvidia-smi topo -m` tell you that `nvidia-smi` does not?
12. What is the difference between the driver's CUDA version, `nvcc`, and `torch.version.cuda`?
13. What does Xid 79 mean, and what do you do first?
14. Why is `nvidia-smi` GPU utilization not a reliable SLI for a training job?
15. How would you prove a training job is data-starved rather than compute-bound?
16. What does `torch.cuda.memory_summary` show that `nvidia-smi` does not?
17. Why can a model fit in inference but not in training on the same GPU?
18. What is the risk of running `nvidia-smi -r` or MIG reconfiguration on a shared host?
19. How does precision affect memory, bandwidth, and correctness?
20. What evidence would you collect before declaring a GPU failed?
