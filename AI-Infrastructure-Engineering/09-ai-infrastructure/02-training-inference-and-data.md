# Training, Inference, and the Data Path

> **The one-sentence model:** training is a throughput pipeline that must keep accelerators fed for many steps; inference is a latency-and-cost pipeline that must produce the next token before the user (or the batch) gives up. Both fail first at the data path, memory, or batching — not at the matrix multiply.

---

## 1. Training vs inference

| Dimension | Training | Inference |
|---|---|---|
| Goal | Minimize loss over a dataset | Serve predictions within latency/cost targets |
| Workload | Forward + backward + optimizer | Forward only (usually) |
| Batch | Large, throughput-oriented | Small/latency-oriented, or dynamically batched |
| Memory | Weights + gradients + optimizer + activations | Weights + KV cache + activations |
| Duration | Hours–weeks, checkpointed | Milliseconds–seconds, stateless or sessionful |
| Scaling | Data/tensor/pipeline parallel | Replicas, batching, quantization, cache |
| Key metric | Samples/sec, step time, loss | TTFT, tokens/sec, p95 latency, cost/request |
| Failure cost | Wasted GPU-hours, lost progress | User-visible errors, timeout, cost spike |
| Precision | FP32/TF32/FP16/BF16, loss scaling | FP16/BF16/FP8/INT8, calibration |

A training job can tolerate a slow request; it cannot tolerate a stalled step. An inference service can tolerate a batch waiting a few milliseconds; it cannot tolerate a 30-second p99.

---

## 2. The training loop

```text
dataset / shards
      │
      ▼
sampler ──► DataLoader workers ──► collate ──► pinned memory
      │                                            │
      │                                            ▼
      │                                     H2D copy (PCIe/NVLink)
      │                                            │
      ▼                                            ▼
  epoch loop ──► forward ──► loss ──► backward ──► all-reduce (if distributed)
                                                   │
                                                   ▼
                                             optimizer step
                                                   │
                                                   ▼
                                             scheduler / logging
                                                   │
                                                   ▼
                                             checkpoint (periodic)
```

A minimal PyTorch loop with mixed precision and checkpointing:

```python
import torch
from torch import nn
from torch.utils.data import DataLoader

device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
model = nn.Sequential(
    nn.Linear(1024, 4096),
    nn.GELU(),
    nn.Linear(4096, 1024),
).to(device)

optimizer = torch.optim.AdamW(model.parameters(), lr=3e-4)
scaler = torch.cuda.amp.GradScaler(enabled=device.type == "cuda")
loader = DataLoader(dataset, batch_size=64, shuffle=True, num_workers=8,
                    pin_memory=True, persistent_workers=True, prefetch_factor=4)

for step, batch in enumerate(loader):
    inputs = batch["input_ids"].to(device, non_blocking=True)
    targets = batch["labels"].to(device, non_blocking=True)

    optimizer.zero_grad(set_to_none=True)
    with torch.autocast(device_type=device.type, dtype=torch.float16,
                        enabled=device.type == "cuda"):
        logits = model(inputs)
        loss = nn.functional.cross_entropy(logits, targets)

    scaler.scale(loss).backward()
    scaler.step(optimizer)
    scaler.update()

    if step % 100 == 0:
        print({"step": step, "loss": loss.item()})
    if step % 1000 == 0:
        torch.save({"model": model.state_dict(),
                    "optimizer": optimizer.state_dict(),
                    "step": step}, f"ckpt-{step}.pt")
```

This loop is intentionally small. The operational questions are:

- Is the GPU idle between steps? Profile.
- Is the DataLoader the bottleneck? Check worker CPU and queue time.
- Is the H2D copy the bottleneck? Check PCIe throughput and pinned memory.
- Is the optimizer step the bottleneck? Check the timeline.
- Is the checkpoint write stalling the job? Measure.

### Step time decomposition

A healthy training step should be dominated by GPU compute for compute-bound models. A useful decomposition:

```text
step_time = data_wait + h2d_copy + forward + backward + collective + optimizer + logging/checkpoint
```

Measure each phase with NVTX ranges and `torch.profiler`; do not guess. A step time of 500 ms with 200 ms of data wait means the GPU is idle 40% of the time, no matter what `nvidia-smi` says at the sample instant.

---

## 3. The data path

### Storage and formats

| Format | Strengths | Weaknesses | Use for |
|---|---|---|---|
| JSONL | Simple, line-oriented, streamable | Parse overhead, large | Small/medium text datasets |
| Parquet | Columnar, compressed, predicate pushdown | Random row access cost, decode | Tabular/feature data |
| WebDataset (tar shards) | Sequential reads, streaming, sharding | Less random access | Images/audio at scale |
| Mosaic/array shards | Efficient sequential reads | Custom tooling | Large pretraining corpora |
| TFRecord | Framework-native | Ecosystem-specific | TensorFlow pipelines |
| Raw files | Simple | Many small files kill metadata | Small datasets only |

The enemy of a GPU pipeline is **many small random reads**. Object storage and network filesystems have high per-request latency; a dataset with millions of tiny files spends its time on metadata, not data. Shard, pack, and read sequentially.

### DataLoader workers

```python
loader = DataLoader(
    dataset,
    batch_size=64,
    num_workers=8,          # CPU processes for decode/augment
    pin_memory=True,        # pinned host memory for faster H2D
    persistent_workers=True,# avoid respawn each epoch
    prefetch_factor=4,      # batches per worker in flight
    drop_last=True,
)
```

- `num_workers=0` runs in the main process and blocks the training loop.
- Too many workers can saturate CPU or contend with the training process; measure.
- `pin_memory=True` uses page-locked host memory, enabling asynchronous H2D copies; it costs RAM.
- `persistent_workers=True` avoids worker startup cost between epochs.
- `prefetch_factor` controls how far ahead each worker reads; too high wastes memory, too low starves the GPU.
- On Linux, worker processes inherit the parent; be careful with CUDA initialization in workers (do not initialize CUDA in a forked worker).

### Tokenization and preprocessing

For text, tokenization can be a CPU bottleneck. Cache tokenized shards instead of re-tokenizing every epoch. For images, decode/resize/augment on CPU can be the bottleneck; consider GPU augmentation or a preprocessed cache.

A data pipeline is correct when:

- the GPU waits only at known synchronization points;
- the input is deterministic per seed (or intentionally stochastic);
- sharding gives each rank a disjoint, balanced subset;
- the last batch is handled consistently (`drop_last` or padding);
- resume reproduces the same order (or documents that it does not).

### Host-to-device transfer

```python
inputs = batch.to(device, non_blocking=True)
```

`non_blocking=True` only helps with pinned memory. Without `pin_memory=True`, the copy may still block. A 1 GB batch over PCIe at 20 GB/s takes 50 ms; over NVLink at 300 GB/s, 3 ms. For small batches, the copy may be dominated by latency, not bandwidth.

---

## 4. Batch size, memory, and precision

### Batch size trade-offs

| Larger batch | Smaller batch |
|---|---|
| Better GPU utilization | Lower latency per request |
| Fewer optimizer steps for the same samples | More steps, noisier gradients |
| More activation memory | Less memory |
| May need learning-rate scaling | Easier to fit |
| Can hurt generalization if too large | Can underutilize the GPU |

Batch size is not a free knob. For training, it interacts with learning rate, gradient accumulation, and convergence. For inference, it interacts with queueing latency and KV-cache memory.

### Gradient accumulation

If the desired effective batch is larger than memory allows:

```python
accum_steps = 4
for step, batch in enumerate(loader):
    with autocast(...):
        loss = criterion(model(batch), target) / accum_steps
    scaler.scale(loss).backward()
    if (step + 1) % accum_steps == 0:
        scaler.step(optimizer)
        scaler.update()
        optimizer.zero_grad(set_to_none=True)
```

This increases the effective batch without increasing activation memory. It does not reduce the time per sample; it reduces the number of optimizer steps. Batch normalization statistics need care (sync across accumulation or use a batch-independent norm).

### Mixed precision

- **FP16:** faster and smaller, but range is limited; use loss scaling to avoid underflow.
- **BF16:** wider range, less mantissa; often the default for training on Ampere+.
- **TF32:** tensor-core mode for FP32 matmuls on A100+; a good default for many workloads.
- **FP8:** Hopper/Blackwell; requires scaling/calibration; validate accuracy.

Mixed precision changes numerics. A loss that becomes NaN after enabling FP16 may be an overflow; check the loss scale and gradient norms. A model that degrades in BF16 may need FP32 for specific layers (layer norm, softmax, accumulations).

### Activation checkpointing

Trade compute for memory: recompute activations during backward instead of storing them.

```python
from torch.utils.checkpoint import checkpoint

def block(x):
    return layer2(layer1(x))

y = checkpoint(block, x, use_reentrant=False)
```

This can cut activation memory substantially at the cost of ~20–30% more compute. It is a standard tool for fitting long sequences or large batches.

### Optimizer memory

Adam stores first and second moments per parameter. In mixed precision, frameworks often keep an FP32 master copy. A rough training footprint per parameter:

```text
FP16 weight (2) + FP16 gradient (2) + FP32 master (4) + m (4) + v (4) = 16 bytes/param
```

For 7B parameters: ~112 GB before activations. This is why a 7B model does not train on one 80 GB GPU with Adam in mixed precision, even though the weights alone are 14 GB.

### ZeRO/FSDP in one paragraph

Data parallel replicates the model on every GPU. ZeRO/FSDP shards optimizer state, gradients, and optionally parameters across ranks, gathering them just in time for compute. It reduces per-GPU memory at the cost of more communication. The details belong in the distributed note; the operational point is that "it fits with FSDP" is a communication and checkpointing decision, not just a memory trick.

---

## 5. Checkpointing

A checkpoint is both a safety net and a storage workload.

| Type | What it stores | Cost | Resume |
|---|---|---|---|
| Full state | Weights, optimizer, scheduler, step, RNG | Large, synchronized | Exact |
| Sharded | Per-rank shards + metadata | Large, parallel writes | Needs same world size or resharding |
| Weights only | Model parameters | Smaller | Fine-tune/inference, not exact resume |
| Async | Copy to CPU/remote while training continues | CPU/network overhead | Complex consistency |

Checkpoint math:

- 7B mixed precision training state ≈ 14 GB weights + 14 GB gradients + 84 GB optimizer = 112 GB.
- At 1 GB/s storage write, a checkpoint takes ~112 s; at 5 GB/s, ~22 s.
- If you checkpoint every 10 minutes and the job runs 24 hours, that is 144 checkpoints; at 112 GB each, 16 TB written. Retention policy matters.

Operational rules:

- Write to a staging path, fsync/close, then atomically rename; a partial checkpoint is not a checkpoint.
- Keep the last N and the best-by-metric; do not fill the filesystem.
- Record the code commit, config, tokenizer, and dataset version with the checkpoint.
- Test resume on a fresh node, not just on the node that wrote it.
- For sharded checkpoints, verify the world size and resharding path.
- Async checkpointing must handle backpressure; if storage is slow, the copy queue grows until memory pressure.

---

## 6. Inference

### Prefill and decode

```text
prompt tokens ──► prefill (parallel over tokens) ──► first token
                                                      │
                                                      ▼
                         decode loop: one token at a time, KV cache grows
```

- **Prefill** processes the prompt in parallel; it is compute-heavy and determines TTFT (time to first token).
- **Decode** generates one token per step per sequence; it is memory-bandwidth-heavy and determines inter-token latency.
- **KV cache** stores past keys/values so decode does not recompute the prefix. It grows with sequence length and batch.

### Metrics

| Metric | Definition | Why it matters |
|---|---|---|
| **TTFT** | Time from request to first token | User-perceived responsiveness |
| **TPOT / ITL** | Time per output token / inter-token latency | Streaming smoothness |
| **Throughput** | Tokens/sec or requests/sec | Capacity and cost |
| **p95/p99 latency** | Tail latency | SLO and user experience |
| **Queue wait** | Time before inference starts | Autoscaling signal |
| **Error rate** | Failed requests / total | Reliability |
| **Cost/token** | GPU cost / tokens produced | Economics |
| **Cache hit rate** | Prefix/KV reuse | Efficiency |

Averages hide the tail. Report TTFT p50/p95/p99 and TPOT p50/p95, not just tokens/sec.

### Batching

- **Static batching:** fixed batch, wait for all requests; simple, can add latency.
- **Dynamic batching:** collect requests for a short window; better GPU utilization, adds queue delay.
- **Continuous batching:** add/remove sequences from a running batch as they finish; standard for LLM serving (vLLM, TensorRT-LLM, TGI).
- **Microbatching:** small batches to balance latency and throughput.

The trade-off is explicit: larger batches improve throughput and reduce cost per token, but increase queueing latency. The right point depends on the SLO.

### KV cache and memory

KV cache formula (FP16, `L` layers, hidden `H`, sequence `S`, batch `B`):

```text
KV_bytes ≈ 2 × L × H × S × B × 2
```

For `L=32`, `H=4096`, `S=4096`, `B=8`: ~16 GiB. Long context, large batch, and many concurrent users can exhaust HBM even when the weights fit. PagedAttention, KV quantization, prefix caching, and eviction policies are memory-management techniques, not just optimizations.

### Quantization

| Method | What it reduces | Accuracy risk | Notes |
|---|---|---|---|
| Weight-only INT8/INT4 | Weight memory, some bandwidth | Moderate | Good for memory-bound decode |
| Activation quantization | Compute/memory | Higher | Needs calibration |
| KV-cache quantization | Cache memory | Moderate | Long-context enabler |
| FP8 | Memory/compute | Model-dependent | Hopper/Blackwell |
| Distillation | Model size | Task-dependent | Retrain, not just compress |
| Speculative decoding | Latency (tokens/step) | None if verification is exact | Extra draft model/compute |

Quantization is a deployment decision with an accuracy evaluation, not a checkbox. Measure task metrics, not just perplexity.

### Serving patterns

| Pattern | Latency | Throughput | Use |
|---|---|---|---|
| Batch offline | Hours | Highest | Nightly scoring, embeddings |
| Online request/response | Milliseconds–seconds | Medium | APIs, chat |
| Streaming | First token fast | Medium | Chat, generation |
| Async queue | Seconds–minutes | High | Document processing |
| Edge | Local | Low | Privacy/latency |

### Autoscaling

Scale on:

- queue depth / waiting requests (leading);
- TTFT or p95 latency (symptom);
- GPU utilization (lagging, and misleading for memory-bound decode);
- tokens/sec per replica (throughput).

GPU nodes are expensive and slow to provision. Keep a warm pool or a minimum replica count for latency-sensitive services; scale to zero only for batch/internal workloads.

---

## 7. Cost math

### GPU-hour cost

```text
cost_per_hour = GPU_hourly_rate × GPU_count
cost_per_request = cost_per_hour / requests_per_hour
cost_per_1k_tokens = cost_per_hour / (tokens_per_hour / 1000)
```

Example: one A100 at $3/hour, serving 20 requests/s with 500 output tokens each:

- requests/hour = `20 × 3600 = 72,000`.
- cost/request = `3 / 72,000 = $0.0000417` (0.00417 cents).
- tokens/hour = `72,000 × 500 = 36,000,000`.
- cost per 1M tokens = `3 / 36 = $0.0833`.

At 20% GPU utilization, the effective cost per useful token is 5× higher. Utilization is an economic metric.

### Training cost

```text
cost = GPU_count × hours × rate × (1 / utilization_efficiency)
```

A job that runs 10% slower because of data starvation costs 10% more. A job that fails at hour 20 without a checkpoint wastes 20 GPU-hours.

### Capacity

```text
replicas = ceil(peak_tokens_per_second / tokens_per_second_per_replica / target_utilization)
```

Add headroom for burst, model loading, and node failure. LLM decode throughput is often memory-bandwidth-bound, so a bigger GPU may not scale linearly with batch; measure.

---

## 8. Measuring the pipeline

### Training signals

| Signal | Where | Interpretation |
|---|---|---|
| Step time | App/PyTorch | End-to-end; compare to GPU compute time |
| Samples/sec, tokens/sec | App | Throughput |
| Data wait | Profiler/NVTX | GPU idle before forward |
| H2D copy time | Nsight | PCIe/NVLink bottleneck |
| GPU utilization | DCGM | Activity, not efficiency |
| GPU memory | DCGM/PyTorch | Capacity/fragmentation |
| CPU workers | `pidstat`, `top` | Data pipeline bottleneck |
| Storage IOPS/throughput | `iostat`, `fio` | Dataset bottleneck |
| Loss scale/skipped steps | App | FP16 instability |
| Checkpoint duration | App | Storage stall |
| NCCL time | Nsight/NCCL logs | Collective bottleneck |

### Inference signals

| Signal | Where | Interpretation |
|---|---|---|
| TTFT p50/p95/p99 | App/gateway | Prefill + queue |
| TPOT/ITL | App | Decode speed |
| Queue depth | App | Autoscaling |
| Batch size | App | Batching efficiency |
| KV-cache usage | App/GPU | Memory pressure |
| GPU utilization/memory | DCGM | Capacity |
| Error rate | App | Reliability |
| Tokens/sec | App | Throughput |
| Cost/request | Billing + app | Economics |

### A simple step-time experiment

Run the same model with:

1. `num_workers=0` vs `8`;
2. `pin_memory=False` vs `True`;
3. batch size 16 vs 64;
4. FP32 vs BF16/FP16.

Measure step time, GPU utilization, and data wait for each. The point is to see which change moves the bottleneck, not to collect a table of numbers.

---

## 9. Failure scenarios

### A — GPU idle gaps between steps

**Symptoms:** `nvidia-smi` shows bursts of 100% then 0%; step time is much higher than kernel time; CPU workers are busy.

**Evidence:** Nsight timeline, DataLoader worker CPU, queue wait, storage latency, and prefetch settings.

**Likely causes:** too few workers, synchronous decode/augment, many small files on object storage, `num_workers=0`, or a slow tokenizer.

**Fix:** increase workers, cache/preprocess, shard files, use pinned memory and prefetch, move augmentation to GPU. Measure the data wait after each change.

### B — Storage throughput is the ceiling

**Symptoms:** GPUs idle in sync; `iostat` shows high `%util` and long await; dataset reads dominate step time; network filesystem latency spikes.

**Evidence:** `iostat -xz`, `fio` benchmark, object-store request latency, dataset file sizes, and per-rank read patterns.

**Likely causes:** random small reads, no sharding, all ranks reading the same files, or a remote filesystem with high latency.

**Fix:** shard by rank, pack into large sequential files, cache locally, prefetch, or use a dataset format designed for streaming. Do not "add more GPU" to a storage bottleneck.

### C — CPU is the bottleneck

**Symptoms:** GPU utilization low; CPU at 100%; DataLoader workers saturate; Python GIL contention; tokenization is slow.

**Evidence:** `pidstat -u -p ALL`, `py-spy dump`, `perf top`, worker count, and profiler CPU time.

**Likely causes:** expensive Python preprocessing, too many workers causing contention, tokenizer overhead, or a single-threaded decode.

**Fix:** vectorize, cache, use a faster tokenizer, move work to C++/Rust, or use GPU preprocessing. More workers can make contention worse.

### D — OOM at the same batch size that worked yesterday

**Symptoms:** `CUDA out of memory`; memory usage higher than before; a new process or a retained graph.

**Evidence:** `torch.cuda.memory_summary`, `nvidia-smi` process list, recent code changes, batch/sequence length, and whether evaluation accumulates tensors.

**Likely causes:** a leaked tensor (storing loss/graph), a larger batch from a config change, fragmentation, another user on the GPU, or activation checkpointing disabled.

**Fix:** find the retained reference, reduce batch/sequence, enable checkpointing, or isolate the GPU. Do not just add `empty_cache()`.

### E — Loss becomes NaN or diverges

**Symptoms:** loss jumps to NaN/Inf; skipped optimizer steps; training stalls.

**Evidence:** loss scale, gradient norms, learning rate, precision mode, data outliers, and the step where it started.

**Likely causes:** FP16 overflow, too-high learning rate, bad data, a broken mask, or a numerical instability in a custom kernel.

**Fix:** use BF16 or loss scaling, clip gradients, inspect the batch, lower the LR, and add a finite-check. A NaN is a symptom; find the operation and the input that caused it.

### F — Checkpointing stalls training

**Symptoms:** step time spikes every N steps; storage write throughput saturated; GPUs idle during checkpoint.

**Evidence:** checkpoint duration, bytes written, storage latency, async vs sync, and retention count.

**Likely causes:** synchronous write of a large state, slow shared storage, too-frequent checkpoints, or no staging/atomic rename.

**Fix:** async checkpoint, write sharded/parallel, reduce frequency, stage locally then upload, and set retention. Test resume.

### G — Inference queue grows under load

**Symptoms:** TTFT rises, queue depth grows, GPU utilization may be high or low, p95 breaches SLO.

**Evidence:** queue depth, batch size, TTFT/TPOT, GPU memory, KV-cache usage, and arrival rate.

**Likely causes:** insufficient replicas, decode-bound workload, large batches causing queueing, KV-cache eviction, or a slow model.

**Fix:** add replicas, continuous batching, quantization, prefix caching, or a smaller model. Scale on queue depth, not just GPU utilization.

### H — KV cache OOM at long context

**Symptoms:** requests fail or are evicted as context grows; GPU memory near capacity; weights fit but cache does not.

**Evidence:** KV-cache size, sequence length distribution, batch/concurrency, and allocator stats.

**Likely causes:** long prompts, too many concurrent sequences, no paging/eviction, or FP16 cache with a large model.

**Fix:** cap context, quantize the cache, use paged attention, reduce concurrency, or add memory. Budget KV cache explicitly in capacity planning.

### I — Model loads but outputs are wrong after serving

**Symptoms:** service returns garbage/truncated output; offline evaluation was fine; a new tokenizer or chat template.

**Evidence:** model/tokenizer versions, chat template, prompt formatting, dtype, quantization, and a golden request comparison.

**Likely causes:** tokenizer mismatch, wrong chat template, different precision/quantization, or a preprocessing difference.

**Fix:** version the tokenizer and template with the model, add golden tests, and compare logits/generation before and after deployment.

### J — Cost per request is far above forecast

**Symptoms:** GPU bill rises; tokens/sec per GPU is lower than expected; utilization is low or the batch is tiny.

**Evidence:** GPU-hours, utilization, batch size, tokens/sec, queue wait, and model/quantization version.

**Likely causes:** over-provisioned replicas, low batch utilization, a memory-bound decode, idle warm pools, or a regression in a kernel/library.

**Fix:** right-size replicas, continuous batching, quantize, improve utilization, or renegotiate the SLO. Cost is a design metric, not an afterthought.

---

## 10. Command field manual

### `nvidia-smi dmon` — is the GPU actually busy?

```bash
nvidia-smi dmon -s pucvmet -d 1
```

**Purpose:** sample utilization, memory, power, clocks, and throttle reasons over time.

**Why:** a single snapshot can catch an idle or busy instant; training is a time series.

**Interpretation:** alternating 100/0 suggests data starvation; sustained 100 with low power suggests memory-bound; throttle reasons explain clock drops.

**Scenario:** a user reports "slow training." Compare GPU utilization over a minute with step time; if utilization is bursty, look at the loader.

**Mistakes:** sampling once and concluding; ignoring memory utilization; not correlating with the step log.

### `pidstat` — CPU-side pressure

```bash
pidstat -u -r -d -p ALL 1
```

**Purpose:** per-process CPU, memory, and I/O.

**Why:** the DataLoader and tokenizer often compete with the training process.

**Interpretation:** high `%CPU` in workers with low GPU utilization means the data path is the bottleneck; high `%system` may mean syscall/IO overhead.

**Mistakes:** looking only at aggregate CPU; missing worker processes that are short-lived.

### `iostat` — storage pressure

```bash
iostat -xz 1 5
```

**Purpose:** device utilization, throughput, latency, and queue depth.

**Why:** datasets on local or network storage can starve the GPU.

**Interpretation:** `%util` near 100 with high `await` means the device is saturated; `r/s` high with small `rkB/s` means many small reads.

**Mistakes:** running `iostat` without `-x` and missing latency; blaming the GPU for a storage bottleneck.

### `fio` — storage baseline

```bash
fio --name=read --rw=read --bs=1M --size=4G --numjobs=4 --iodepth=16 --time_based --runtime=30 --group_reporting
```

**Purpose:** measure sequential/random throughput and latency of the storage path.

**Why:** you need a baseline to know whether the dataset is slow or the application is inefficient.

**Interpretation:** compare `bw` and `lat` to the dataset's needs; a 1 GB/s sequential device with 5 ms random latency is normal for some storage and terrible for many small reads.

**Mistakes:** benchmarking the page cache instead of the device (`direct=1` when appropriate); running on a shared production volume.

### `py-spy` — Python hotspots

```bash
py-spy top --pid <pid>
py-spy dump --pid <pid>
```

**Purpose:** sample Python stacks without modifying the process.

**Why:** it finds tokenizer/preprocessing hotspots that a GPU profiler cannot see.

**Interpretation:** a stack stuck in `PIL.Image.resize`, `tokenizers`, or a Python loop is a data-path finding.

**Mistakes:** attaching to a process you do not own without permission; treating a sample as a complete profile.

### `torch.profiler` — step breakdown

```python
from torch.profiler import profile, ProfilerActivity

with profile(activities=[ProfilerActivity.CPU, ProfilerActivity.CUDA],
             record_shapes=True, profile_memory=True) as prof:
    train_step()
print(prof.key_averages().table(sort_by="self_cuda_time_total", row_limit=20))
```

**Purpose:** CPU/CUDA time by operator, memory, and shapes.

**Why:** it connects a slow step to specific operations and copies.

**Interpretation:** high `cudaMemcpy` time means H2D; high `aten::` CPU time with low GPU time means host-bound; a single kernel dominating suggests a compute/memory bottleneck.

**Mistakes:** profiling one step and generalizing; ignoring warmup; not exporting a trace for the timeline.

### `curl` / `hey` — inference latency

```bash
curl -s -o /dev/null -w 'ttfb=%{time_starttransfer}s total=%{time_total}s\n' \
  -X POST http://localhost:8000/predict \
  -H 'Content-Type: application/json' \
  -d '{"prompt":"hello","max_tokens":64}'

hey -z 60s -c 10 -m POST \
  -H 'Content-Type: application/json' \
  -d '{"prompt":"hello","max_tokens":64}' \
  http://localhost:8000/predict
```

**Purpose:** measure latency and throughput from the client's perspective.

**Why:** server-side GPU time is not the user's TTFT; queueing and network matter.

**Interpretation:** `time_starttransfer` is a proxy for TTFT for non-streaming responses; `hey` gives a distribution, not just an average. For streaming, use a client that records first-chunk time.

**Mistakes:** testing with a single request and calling it capacity; ignoring connection reuse/TLS; load-testing a shared service.

### `python -c` — memory estimate

```python
params = 7e9
bytes_per_param = 2
print("weights GB", params * bytes_per_param / 1e9)
print("Adam mixed GB", params * 16 / 1e9)
```

**Purpose:** quick order-of-magnitude memory estimate before launch.

**Why:** it prevents an obvious OOM and frames the sharding decision.

**Interpretation:** add 10–30% overhead; activations and KV cache are separate. A result near the GPU capacity is a warning, not a green light.

**Mistakes:** treating the estimate as exact; forgetting gradients/optimizer/activations; mixing decimal GB and GiB.

---

## 11. Design checklist

- [ ] The workload is classified as training or inference, with the right metric (throughput vs latency).
- [ ] The data path is measured separately from the GPU: storage, CPU, H2D, compute.
- [ ] Batch size and sequence length are chosen from memory and SLO, not defaults.
- [ ] Precision and quantization have an accuracy evaluation.
- [ ] Checkpoint frequency, retention, and resume are tested.
- [ ] The training job has a hard timeout and a watchdog for hangs.
- [ ] Inference reports TTFT, TPOT, p95, queue depth, and tokens/sec.
- [ ] KV cache is budgeted explicitly for long context and concurrency.
- [ ] Cost per request/token is calculated and reviewed.
- [ ] A model artifact includes weights, tokenizer, template, code, and environment versions.

---

## 12. Checkpoint

1. What are the key differences between training and inference workloads?
2. Draw the training loop and name the stages that can starve the GPU.
3. Why can `num_workers=0` be a bottleneck, and why can too many workers also hurt?
4. What does `pin_memory=True` enable, and what does it cost?
5. Why are many small files bad for a GPU data pipeline?
6. How does gradient accumulation change memory and step count?
7. What does activation checkpointing trade, and when would you use it?
8. Estimate the mixed-precision training memory per parameter for Adam. Why is a 7B model hard on one 80 GB GPU?
9. What is the difference between TTFT and TPOT, and which one does prefill affect?
10. Write the KV-cache formula and estimate it for a 32-layer, 4096-hidden model at batch 8, 4096 tokens, FP16.
11. Why is decode often memory-bandwidth-bound?
12. What is continuous batching, and what latency/throughput trade-off does it make?
13. How would you detect a data-starvation problem with three commands?
14. Why can a checkpoint stall a training job, and how do you reduce the stall?
15. What is the risk of quantizing a model without an accuracy evaluation?
16. How do you calculate cost per 1M tokens from GPU-hour cost and throughput?
17. Why is GPU utilization a poor autoscaling signal for LLM decode?
18. What would you preserve when a training job hangs in a collective?
19. How do you version a model artifact so serving matches training?
20. What is the first thing you check when inference p95 doubles after a deploy?
