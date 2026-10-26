# Phase 9 Checkpoint — AI Infrastructure

> **Rules:** closed book, 90 minutes for Parts A–D. No CLI, no notes, no search. Then complete the practical sign-off on the Phase 9 labs. Target: **80% overall (128/160)**, and at least **32/40** on the diagnosis scenarios.
>
> **Assumptions:** NVIDIA A100/H100-class hardware, CUDA 12.x, PyTorch 2.4+, NCCL 2.2x, Kubernetes 1.29+, Slurm 22.05+, MLflow 2.x, vLLM 0.5+. Where an exact number depends on hardware or version, answer with the evidence you would collect and the decision rule, not a memorized string. Where a command depends on a specific GPU you may not own, describe the read-only evidence you would request instead of claiming you ran it.
>
> **Honesty:** the checkpoint does not require that you own a GPU. It requires that you reason about one correctly, and that you can say what evidence would prove or disprove each hypothesis.

---

## Part A — Rapid facts (25 × 2 = 50 points)

1. Name the four levels of GPU memory hierarchy and what each trades for speed.
2. What does an SM do, and what is a warp?
3. Why does a GPU hide latency with many warps, and what happens when there are not enough?
4. What is arithmetic intensity, and what is a roofline?
5. What does `GPU-Util 100%` in `nvidia-smi` actually measure?
6. What is a XID error, and why is XID 48 more serious than a transient XID 13?
7. Name the three memory components in a training step besides activations, and give their sizes in terms of parameters and dtype.
8. What does mixed precision (FP16/BF16) change about memory and bandwidth?
9. What does a mixed-precision optimizer's loss scaler protect against, and why does BF16 mostly not need it?
10. What is gradient accumulation for?
11. What does activation checkpointing trade for memory?
12. Define speedup and scaling efficiency for N GPUs.
13. Name the four parallelism strategies and their communication pattern in one phrase each.
14. Why is tensor parallel usually kept within an NVLink domain?
15. Write the pipeline bubble fraction formula and compute it for 4 stages and 16 microbatches.
16. In a ring all-reduce of size S over N ranks, how many bytes does each rank send?
17. What is the difference between `algbw` and `busbw` in NCCL tests?
18. What does `NCCL_DEBUG=INFO` show that default logs do not?
19. What is GPUDirect RDMA, and what does it bypass?
20. Why does RoCE need PFC/ECN, and what is the symptom without them?
21. What does the Kubernetes device plugin do, and what does it not do?
22. Why must a GPU request equal its limit in Kubernetes?
23. What does MIG provide that time-slicing does not?
24. What is Slurm fair-share, and how does it differ from a hard quota?
25. Define TTFT and TPOT, and name one cause of a rising TTFT with flat TPOT.

---

## Part B — Calculations (6 × 5 = 30 points)

### B1 — Model memory budget

A 13B-parameter model trains with BF16 weights, AdamW (fp32 master weights, m, v), and gradient checkpointing off.

1. Weights: how many GB?
2. Gradients: how many GB?
3. Optimizer states: how many GB?
4. Total fixed memory, before activations?
5. If it does not fit in 80 GB, name three levers that reduce the fixed cost and what each trades away.

### B2 — Arithmetic intensity and the roofline

One training step for a 7B model, batch 16, sequence 4,096. Peak compute 312 TFLOP/s (BF16), peak HBM bandwidth 1,935 GB/s. Use `6NP` for training FLOPs and assume ~16 bytes per parameter moved per step.

1. Tokens per step?
2. Training FLOPs per step (in TFLOP)?
3. Arithmetic intensity (FLOP/byte)?
4. Ridge point for this GPU?
5. Classification, and what it implies about the first lever to pull?

### B3 — All-reduce time

8 GPUs, each with 14 GB of gradients to all-reduce per step. Effective ring bandwidth 20 GB/s per direction (measured `busbw`).

1. Total data moved by the collective across the job (ring model)?
2. Bytes sent by each rank?
3. Lower bound on collective wall time, given the bandwidth?
4. If the model's step time without communication is 900 ms, what fraction of the step is communication?
5. Name two changes that reduce that fraction, and one that does not.

### B4 — Pipeline bubble

A 24-layer model splits into 6 pipeline stages over 6 GPUs, 16 microbatches.

1. Bubble fraction (formula and value)?
2. If a full batch takes 2.4 s of pure compute per stage, how much wall time is lost to bubbles per step?
3. What happens to the bubble when microbatches double to 32?
4. What happens to memory per stage when microbatches double?
5. Name one alternative split that avoids bubbles entirely and its main cost.

### B5 — Scaling efficiency

Baseline: 1 GPU does 100 samples/s. On 8 GPUs you measure 560 samples/s.

1. Speedup?
2. Efficiency?
3. If the job's serial fraction (collective + optimizer + data) is 8%, what is Amdahl's ceiling for speedup on 8 GPUs?
4. You reduce serial fraction to 3%. New Amdahl ceiling?
5. Name two causes of a serial fraction that code review can find.

### B6 — Inference cost

A serving GPU costs $3.20/hour. A model serves 25 requests/s. Each request averages 600 output tokens. The GPU is 45% utilized over the billing window.

1. Tokens served per second?
2. Cost per 1M output tokens at the stated utilization?
3. Cost per 1M tokens if the utilization were 100% (benchmark number)?
4. Why is the 100% number misleading for capacity planning?
5. The team wants cost per 1M tokens under $0.05. Name two levers and their expected magnitude.

---

## Part C — Diagnosis scenarios (8 × 5 = 40 points)

For each scenario, answer:

- What evidence do you collect first?
- What are the two most likely causes?
- What is the smallest safe next action?
- What would you change to prevent recurrence?

### C1 — The training job is slow and the GPUs look busy

A 8-GPU training job shows all eight GPUs at 95–100% `utilization.gpu`, but tokens/s is 40% lower than yesterday. No code change. `nvidia-smi` shows clocks are normal, temperature is fine.

### C2 — The pod is pending while the node has free GPUs

A pod requesting 1 GPU is Pending. `kubectl describe node` shows `nvidia.com/gpu: 3` allocatable and 3 allocated... the `Allocated resources` section shows `nvidia.com/gpu 8 (100%)`. The `capacity` line shows 8.

### C3 — The inference API is slow at peak, fine at low traffic

p95 latency at 50 req/s is 200 ms; at 400 req/s it is 4 s. TTFT rises; TPOT is flat. The GPU shows 60% utilization and `gpu_cache_usage_perc` near 0.95.

### C4 — A NCCL job hangs at initialization

`torchrun` starts 8 ranks. The job hangs after "NCCL INFO Bootstrap : Using..." and never enters the training loop. Logs show each rank waiting for the others. `nvidia-smi` shows nothing running.

### C5 — Accuracy dropped after a routine redeploy

A classifier's offline accuracy on the golden set is unchanged, but the on-call sees a 15% rise in false positives in production over 3 days. The deploy was a library upgrade only. No model change.

### C6 — The GPU memory climbs and the job OOMs

A training job's memory grows by ~200 MB every 50 steps, then OOMs at hour 2. Batch size is fixed, sequence length is fixed.

### C7 — A team's Slurm job is pending for hours

`REASON=Resources` for 3 hours. `sinfo` shows the GPU partition at `idle` for 2 nodes. The user asks whether the cluster is broken.

### C8 — The cost per 1M tokens doubled

The serving team's dashboard shows cost per 1M tokens jumped from $0.03 to $0.06 in a week. Requests and tokens per day are roughly flat. Replica count is unchanged.

---

## Part D — Design questions (2 × 20 = 40 points)

### D1 — A 64-GPU training cluster for a 70B model

Design the training infrastructure for a 70B-parameter model:

- 8 nodes × 8 A100-80GB each, NVLink within node, InfiniBand HDR between nodes;
- the team has never run distributed training at this scale;
- a 30-day deadline for the first full run;
- a shared storage tier with 10 GB/s throughput.

Cover:

1. The memory budget and the parallelism strategy (which dims, which ratios).
2. Which communication goes on NVLink vs InfiniBand, and why.
3. The NCCL/topology validation plan before the first full run.
4. Checkpointing: what, where, how often, and how a resume is tested.
5. The failure/restart policy: watchdog, straggler, elastic restart.
6. Observability: which metrics decide "the run is healthy."
7. The cost model: GPU-hours per experiment, and the decision to stop scaling.
8. Two failure modes you expect in the first week and their mitigations.

### D2 — A multi-tenant inference platform

Design the platform for serving three teams' models on 16 GPUs:

- Team A: a 70B LLM, 20 req/s, p95 TTFT < 1.5 s, budget-capped;
- Team B: a small classifier, 500 req/s, p95 < 100 ms;
- Team C: batch scoring, 10M rows/night, no latency requirement;
- one shared storage tier; a cost model must be visible per team.

Cover:

1. The GPU allocation strategy per team (whole, MIG, time-slicing) and why.
2. Isolation and multi-tenancy: quotas, priority, preemption, and the blast radius of a fault.
3. The serving stack per team (engine choice, batching, autoscaling, cold start).
4. The observability per team (metrics, SLOs, dashboards, alerts).
5. The cost model per team, including allocated vs utilized GPU-hours.
6. The supply-chain controls (weights, containers, credentials).
7. The failure playbook: OOM, GPU fault, overload, and a bad model deploy.
8. One design decision you would revisit in 6 months and the signal that would trigger it.

---

## Answer key

### Part A

1. Registers (fastest, smallest, per-thread) → shared memory/L1 (per-block, programmer-managed) → L2 (per-GPU) → HBM (largest, slowest, off-chip). Trade: capacity vs latency/bandwidth.
2. An SM executes instructions for its resident warps; a warp is 32 threads that execute one instruction in lockstep.
3. Many warps let the SM switch to a ready warp while another waits on memory; with few warps the SM idles during memory stalls.
4. Arithmetic intensity is FLOPs per byte moved from memory; the roofline is the line `performance = min(peak_FLOPs, AI × bandwidth)`.
5. The fraction of a sample window during which at least one kernel was resident on the GPU. It does not measure useful work.
6. A XID is a driver-level error code. XID 48 is a double-bit ECC error (data may be wrong); transient XID 13 can be a single bad kernel. 48 means drain and diagnose.
7. Gradients (`P × dtype`) and optimizer states (AdamW: 2 × `P × 4` bytes for m and v; plus fp32 master weights if training in bf16, `P × 4`).
8. FP16/BF16 halve weight/gradient bytes, roughly double effective bandwidth capacity, and use tensor cores; activations are usually stored in the same precision.
9. The loss scaler prevents underflow when FP16 gradients (min normal ~6e-5) fall below representable range; BF16 has FP32-like exponent range, so underflow is rare.
10. Gradient accumulation runs several microbatches and sums gradients before the optimizer step, simulating a larger batch without the memory.
11. It recomputes activations in backward instead of storing them, trading extra forward compute (typically ~33% more) for much less activation memory.
12. `speedup(N) = throughput(N)/throughput(1)`; `efficiency(N) = speedup(N)/N`.
13. Data parallel: all-reduce gradients every step. Tensor parallel: per-layer all-gather/all-reduce within a layer. Pipeline: point-to-point activations between stages. Expert: all-to-all token routing.
14. Tensor parallel is chatty per layer and latency-sensitive; NVLink's bandwidth and low latency absorb it, PCIe or Ethernet do not.
15. `bubble ≈ (stages−1)/(microbatches+stages−1)`; for 4 stages/16 microbatches: `3/19 ≈ 16%`. (For 6 stages: `5/21 ≈ 24%`.)
16. `2S(N−1)/N` bytes per rank (reduce-scatter + all-gather phases).
17. `algbw = S/time` from one rank's view; `busbw = algbw × 2(N−1)/N` normalizes to the ring's actual link traffic.
18. Algorithm selection, topology detection (NVLink/IB/Socket), channel setup, and per-collective timing that default logs omit.
19. GPUDirect RDMA lets an RDMA NIC read/write GPU memory directly, bypassing the CPU and staged host-memory copies.
20. Lossless Ethernet needs PFC/ECN to avoid congestion collapse for RDMA; without them, drops force retransmits and throughput craters under load.
21. It advertises devices, reports health, and allocates device IDs/env/mounts at pod admission. It does not schedule pods, enforce GPU memory limits, or partition devices.
22. GPU allocation is an exclusive device reservation; a device cannot appear and disappear, so asymmetric requests are invalid.
23. MIG gives hardware-partitioned SM/memory with fault and memory isolation; time-slicing shares one GPU with no memory isolation.
24. Fair-share is a scheduling weight that decays with historical usage; a hard quota (QOS MaxTRES) is an enforced limit independent of history.
25. TTFT is arrival→first token; TPOT is time per output token. Rising TTFT with flat TPOT means queueing or slow prefill (long prompt, cold cache), not slow decode.

### Part B

#### B1

1. `13e9 × 2 = 26 GB`.
2. `13e9 × 2 = 26 GB` (BF16 grads).
3. AdamW m+v: `13e9 × 4 × 2 = 104 GB`; fp32 master weights: `13e9 × 4 = 52 GB` → `156 GB`.
4. `26 + 26 + 156 = 208 GB`. It does not fit one 80 GB GPU even before activations.
5. Levers: (a) ZeRO/optimizer sharding across GPUs (÷N memory, communication cost); (b) 8-bit optimizer states (~÷2, small quality risk, needs validation); (c) FP16 weights with fp32 master kept only per-shard (same as (a)); (d) offload to CPU (very slow bandwidth). State the trade for each.

#### B2

1. `16 × 4096 = 65,536` tokens.
2. `6 × 7e9 × 65,536 = 2.75e15 = 2,753 TFLOP`.
3. AI = `2.75e15 / (7e9 × 16) = 24,600 FLOP/byte`.
4. Ridge = `312e12 / 1935e9 ≈ 161 FLOP/byte`.
5. AI (24,600) ≫ ridge (161): decisively **compute-bound** at this batch size. First lever is kernel efficiency/precision (TF32/FP8, better kernels), not loading more data. (At batch 1 × seq 512 the AI falls ~128× to ~190, near the ridge — the small-batch regime flips to memory-bound.)

#### B3

1. Ring total per rank = `2S(N−1)/N = 2 × 14 GB × 7/8 = 24.5 GB`; across 8 ranks = `196 GB`.
2. Each rank sends `24.5 GB`.
3. Time ≈ `24.5 GB / 20 GB/s = 1.225 s`.
4. `1.225 / (1.225 + 0.9) = 57.6%` of the step is communication.
5. Reduce: (a) higher bandwidth interconnect (IB/NVLink); (b) gradient compression/quantization; (c) fewer/larger all-reduces via gradient bucketing and overlap with backward. Does **not** help: adding more ranks (communication grows), or a bigger batch (per-step gradients unchanged in size).

#### B4

1. `(6−1)/(16+6−1) = 5/21 ≈ 23.8%`.
2. Lost per step ≈ `0.238 × 2.4 s = 0.57 s` per stage (as scheduled; the first stage idles during fill/drain).
3. `(6−1)/(32+5) = 5/37 ≈ 13.5%` — roughly halved.
4. Activation memory per stage roughly doubles (more in-flight microbatches), which can force a smaller per-microbatch size and undo the gain.
5. Data parallel avoids bubbles (every GPU is busy every step); the cost is a full model copy per GPU plus the all-reduce each step.

#### B5

1. `560/100 = 5.6×`.
2. `5.6/8 = 70%`.
3. `1/(0.08 + 0.92/8) = 1/0.195 = 5.13×`. Measured 5.6× exceeds it? That means the "serial fraction" was measured on a smaller configuration or the baseline was inefficient; a real answer should note the inconsistency and re-measure the serial fraction rather than force the model to fit.
4. `1/(0.03 + 0.97/8) = 1/0.151 = 6.61×` ceiling; at 70% efficiency there is ~1.4× headroom left on 8 GPUs.
5. Data loader synchronized on the main process; a host-side metric or tokenizer running serially; per-rank file I/O without parallel reads; a Python-side loop that gathers to rank 0.

#### B6

1. `25 × 600 = 15,000 tok/s`.
2. `($3.20/h ÷ 3600) ÷ 15,000 × 1e6 = $0.0593` per 1M tokens.
3. At 100%: `0.0593 × 0.45 = $0.0267`.
4. Because no real workload runs at 100%; the gap between benchmark and actual is the idle cost you pay anyway. Planning with the benchmark number understates cost by ~2×.
5. Levers: raise utilization via continuous batching/better packing (45%→70% cuts unit cost to ~$0.038); quantize weights/KV cache to raise tokens/s (e.g., FP8 → roughly 1.5–2× throughput → ~$0.03–0.04); both together can reach the target. Smaller model/distillation is a third, bigger lever with a quality trade.

### Part C

Each scenario is scored out of 5: evidence (2), likely causes (1), next action (1), prevention (1).

#### C1

Evidence: per-GPU `DCGM_FI_DEV_SM_CLOCK`/`MEM_CLOCK`, `PIPE_TENSOR_ACTIVE` vs `GPU_UTIL`, memory bandwidth utilization, NCCL logs (algo/protocol, channel count), per-rank step time, `nvidia-smi topo -m` vs yesterday, network counters. Likely causes: (a) a rank is throttled or a link degraded so the collective is slower — "busy" is NCCL spinning; (b) clocks are normal but the kernel changed (a library upgrade) and tensor-core utilization fell; (c) batch/data mix changed. Next action: compare `PIPE_TENSOR_ACTIVE` across GPUs and days; if one rank lags, find it; if all are uniformly lower, diff the library/driver. Prevention: per-rank step-time and tensor-activity dashboards, alert on relative divergence, baseline logs from a healthy run.

#### C2

Evidence: pod requests, node `capacity` vs `allocatable`, running GPU pods and their requests, device plugin logs, events. Likely causes: the 8 GPUs are requested by pods that may not use them (idle holders); a fractional/incorrect request; a taint the pod does not tolerate; allocatable reduced by a plugin restart. Next action: list GPU-holding pods and their actual usage (DCGM per GPU); right-size or evict an idle holder. Prevention: utilization-based reclamation, accurate requests, admission policy requiring a request for GPU visibility.

#### C3

Evidence: queue depth, `num_requests_waiting`, KV-cache usage, batch size, prefill time distribution, GPU utilization and tensor activity, concurrency limits. Likely causes: KV cache near saturation forcing queueing; concurrency limit hit so requests queue; prefill cost growing with prompt length (traffic mix). Next action: cap concurrency/queue, raise replicas or use continuous batching tuning; measure the prefill time split. Prevention: load test at 2× forecast, autoscale on queue depth, monitor KV-cache usage with an alert before saturation.

#### C4

Evidence: `NCCL_DEBUG=INFO/WARN` logs per rank, `nvidia-smi topo -m` on each node, IB/RoCE counters (`ibstat`, `perfquery`), firewall/port reachability (e.g., NCCL sockets), hostname resolution, identical env across ranks, `dmesg` for NIC/GPU errors. Likely causes: ranks cannot reach each other (wrong `MASTER_ADDR`, blocked ports, mismatched NCCL socket interfaces); one rank crashed before init and the others wait; a topology/driver mismatch. Next action: collect logs from all ranks, find the first rank that differs, test pairwise connectivity (`nc -z`), and if a NIC/GPU is missing, fix the node. Prevention: a topology/NCCL validation job before every cluster handoff, and a watchdog timeout with a health alert.

#### C5

Evidence: model version and artifact hash served before/after, library version diff, preprocessing/tokenizer changes, input distribution over time, per-slice accuracy, prediction-log samples replayed offline. Likely causes: training-serving skew introduced by a library change (feature computation or default parameter), or genuine data drift coincident with the deploy. Next action: replay production samples through the old and new pipelines and diff outputs; roll back if the diff is non-trivial. Prevention: package preprocessing with the model, contract tests with golden inputs, canary with a quality guardrail, and drift monitors that are paired with quality metrics.

#### C6

Evidence: `nvidia-smi`/DCGM memory over time, batch/seq fixed, fragmentation report, allocator settings, per-step memory summary from PyTorch (`torch.cuda.memory_summary`), leak repro with a fixed workload. Likely causes: reference/cache growth (e.g., an accumulating tensor), variable-length padding creating fragmentation, a sidecar process on the same GPU, a leak in a custom op. Next action: reproduce with a fixed workload and capture the allocator stats; if fragmentation, enable `expandable_segments` or pad to fixed shapes; if a reference leak, fix the code. Prevention: a memory-growth alert, a stress test in CI that runs 1,000 steps and asserts bounded memory.

#### C7

Evidence: `squeue -j <id>`, `scontrol show job`, `sinfo` state and reason, QOS/partition limits, `sprio`, `sshare`, whether the pending job's `--gres` type matches the idle nodes. Likely causes: the idle nodes are a different GPU type than requested; the job's request exceeds a QOS/partition max; fair-share priority is low and backfill has not found a slot. Next action: read the exact request vs the partition's GRES types; adjust the request or the partition; do not resubmit repeatedly. Prevention: document GRES types per partition; alert on jobs pending beyond a threshold; give users `squeue --start` estimates.

#### C8

Evidence: per-model/version cost series, replica count and autoscaler state, GPU utilization, spot/on-demand mix, token counts per request, model version, instance pricing change. Likely causes: a utilization drop (idle capacity) at the same replica count, a spot interruption replaced by on-demand, a model/quantization change slowing tokens/s, or a pricing change. Next action: decompose cost = replicas × price × hours / tokens; identify which factor moved; fix the largest. Prevention: cost per 1M tokens as a deploy guardrail, budget alerts, and a dashboard that shows the decomposition, not just the total.

### Part D

#### D1 — Model answer

**Memory/parallelism.** 70B in BF16: weights 140 GB, grads 140 GB, AdamW fp32 m+v 560 GB, master weights 280 GB → ~1.12 TB fixed, before activations. One 80 GB GPU cannot hold it; 8 GPUs cannot either without sharding. Strategy: ZeRO-3/FSDP or 3D parallelism. A common plan: tensor parallel 8 within a node (NVLink), pipeline parallel 4 across nodes (16 GPUs), data parallel 4 (64 GPUs) with ZeRO-1 for optimizer states. Memory per GPU: weights ~140/16 ≈ 8.75 GB, grads ~8.75 GB, optimizer states sharded 560/64 ≈ 8.75 GB, activations per stage with microbatching ~20–40 GB. Fits with headroom.

**Interconnect mapping.** Tensor-parallel all-gather/all-reduce stays intra-node on NVLink (600 GB/s, microseconds). Pipeline send/recv crosses nodes on IB (200 Gb/s ≈ 25 GB/s, microseconds of latency per hop, hundreds per step). Data-parallel gradient all-reduce overlaps with backward and uses IB; bucket gradients and start the all-reduce as soon as each bucket is ready.

**Validation before the first run.** (1) `nccl-tests all_reduce_perf` at 14 GB × 8 ranks intra-node, expecting near-NVLink busbw; (2) the same across 2 nodes on IB; (3) `nvidia-smi topo -m` on every node, identical layouts; (4) a 1-layer training smoke test with `NCCL_DEBUG=INFO` capturing the chosen algorithm/protocol; (5) a 1,000-step dry run at full scale measuring tokens/s and per-rank divergence; (6) InfiniBand counters (`perfquery`) confirming no retransmits/PFC storms.

**Checkpointing.** Sharded checkpoint every 30–60 min to the shared storage (10 GB/s → a 70B sharded checkpoint is ~1.1–1.5 TB, ~2–3 min to write). Keep the last 3. Test the resume at every world size you might restart with (reshardable optimizer state or a fixed world-size constraint documented). Resume test is part of the run, not a separate exercise: kill and restart a scaled-down config weekly.

**Failure/restart.** Watchdog with an explicit timeout; on a rank failure, the job exits non-zero and the scheduler restarts from the last checkpoint. Straggler detection from per-rank step time; a rank >10% slower for 5 min triggers an alert, and sustained divergence triggers a drain-and-replace. Elastic restart is a stretch goal; do not promise it for the first run.

**Observability.** tokens/s, per-rank step time (max/min ratio), memory per rank, NVLink/IB throughput and errors, DCGM (temp, power, clocks, ECC, XID), collective time from NCCL logs, data-loader wait. Health = tokens/s within 10% of plan, no rank divergence, no XID, no thermal throttle.

**Cost.** 64 A100s ≈ $32/h → $2,048/hour of wall time. At a realistic 40% MFU on 70B (≈ 312 TFLOP/s × 64 × 0.40 ≈ 8 TFLOP/s effective... using 6NP per token, ≈ 8e12/6/7e10 ≈ 19,000 tok/s ≈ 1.6B tokens/day ≈ 48B tokens/30 days), the full run is ~1,500 GPU-hours × $... actually `64 × 720 h = 46,080 GPU-hours ≈ $147k`. State it, then show the stop-scaling rule: if doubling GPUs adds <20% throughput, the bottleneck is elsewhere (data, kernel, or collective) and the money is better spent there.

**First-week failure modes.** (a) NCCL timeout from a single slow NIC → mitigation: pre-run topology validation, NIC firmware uniformity, and a watchdog that names the slow rank. (b) OOM from activation growth with long sequences → mitigation: sequence-length buckets, activation checkpointing on the heaviest layers, and a memory dry-run at the longest sequence. (c) Data loader starves the GPUs → mitigation: measure tokens/s with the data pipeline on and off; if it drops, shard the loader and cache to local NVMe.

Scoring: 20 points; full credit requires concrete numbers (memory per GPU, busbw targets, checkpoint sizes, dollar figures), not just strategy names.

#### D2 — Model answer

**Allocation.** Team A (70B LLM): 6 GPUs tensor-parallel (or 8 with a 2×4 plan), whole-GPU, dedicated — TP is latency-sensitive, needs NVLink. Team B (classifier): 2 GPUs under MIG `3g.40gb` × 2 (or 1 GPU time-sliced) — latency-tolerant at 100 ms p95, small model. Team C (batch): the remaining GPUs, spot/preemptible, scale-to-zero after the nightly window. Total: 6 + 2 + 8 = 16. Document that Team C's GPUs are the elastic buffer.

**Isolation.** Namespaces per team; ResourceQuota per namespace; Team A taint `dedicated=llm` so nothing else lands there; MIG for Team B (memory isolation); Team C gets no isolation guarantees beyond spot economics. Priorities: Team A serving > Team B > Team C; Team C's training/batch never preempts serving. Fault blast radius: MIG contains a Team B OOM/fault to its slice; a Team A TP rank failure fails the whole Team A serving group (mitigate with 2 replicas across nodes and a load balancer).

**Serving.** Team A: vLLM with continuous batching, `--tensor-parallel-size 6/8`, prefix caching for shared system prompts, concurrency capped at what fits KV cache; autoscale on queue depth with a 5-min scale-out delay (cold start ~2 min). Team B: Triton with dynamic batching (preferred batch 16, max delay 2 ms), 2 instances per GPU slice. Team C: a batch job per night, checkpoints intermediate results, no autoscaler.

**Observability.** Per team: request rate, p50/p95/p99 (TTFT and TPOT for A), error rate, queue depth, KV-cache usage (A), GPU utilization and tensor activity, memory per instance, cost per 1k requests. SLOs: A — 99.9% valid responses, p95 TTFT < 1.5 s; B — 99.95% valid, p95 < 100 ms; C — job completes before 08:00, 99% row success. Alerts: p95 breach (A/B), GPU unavailable, queue depth > threshold, KV cache > 90%, nightly batch not started by 22:00.

**Cost.** Per team: allocated GPU-hours (replicas × GPUs × hours), utilized GPU-hours (integral of active work), price, cost per 1k requests. Team A's cost per 1M tokens is the primary metric; Team B's cost per 1k requests; Team C's cost per 10M rows. Report both allocated and utilized; the gap is the efficiency conversation.

**Supply chain.** Weights pinned by hash in a registry, containers pinned by digest, `safetensors` preferred, artifact scan on ingest, serving pods with no cloud write credentials, egress restricted to required endpoints, model registry writes only from CI with an approval.

**Failure playbook.** OOM (A): reduce `max_model_len`/concurrency, alert before saturation. GPU fault: DCGM XID alert → drain node → replace → health-check before return. Overload: cap concurrency, shed load with a 429 and a retry-after, autoscale if GPUs are free (they usually are not — hence Team C's elastic buffer). Bad model deploy: canary with a quality guardrail, one-action rollback via alias.

**Revisit in 6 months.** Team B's 2×MIG slices vs moving to time-slicing/MPS as the classifier's model grows, or Team C's elastic buffer becoming a reserved pool if spot becomes unreliable. Trigger: either team's p95 or Team C's completion time breaching SLO twice in a month.

Scoring: 20 points; full credit requires specific numbers and the reason each control exists, not just the control name.

---

## Practical sign-off (must pass, on the Phase 9 labs)

Bring evidence; "I did it" is not evidence. GPU-optional paths are accepted when the lab provides them.

### L9.1 — GPU inspection and roofline

- [ ] All eight GPUs triaged with state, evidence, and an open question (from the capture or your own host).
- [ ] Process attribution is correct and the notebook's inconsistency is identified.
- [ ] Roofline run for three scenarios; classifications and step-time estimates recorded.
- [ ] The four fault states re-derived before reading the analysis.
- [ ] No GPU stress/MIG/driver changes run on any host you do not own.

### L9.2 — Train and serve

- [ ] Training produced an artifact with a hash, tracked in MLflow.
- [ ] The hash check fails correctly when the artifact is tampered with.
- [ ] Baseline p50/p95/p99 recorded from `/metrics`.
- [ ] Slow-path and concurrency queueing measured and explained with Little's law.
- [ ] Cost per 1,000 requests computed for CPU, compared to a GPU estimate with stated assumptions.

### L9.3 — Distributed collectives

- [ ] 2-rank and 4-rank all-reduce measured; `busbw ≈ 2(N−1)/N × algbw` verified.
- [ ] Straggler experiment shows the wall-time penalty on all ranks.
- [ ] Killed-rank experiment shows the hang and the timeout/error; captured verbatim.
- [ ] Rank wiring env captured; identical master across ranks.

### L9.4 — GPU scheduling semantics

- [ ] kind cluster created and torn down cleanly.
- [ ] Fake GPUs advertised and confirmed in `allocatable`.
- [ ] Fractional request rejected; requests-equal-limits enforced; 9th pod pending.
- [ ] Taint, quota, priority/preemption behavior each demonstrated.
- [ ] Gang-scheduling waste demonstrated; single-pod alternative shown.
- [ ] Time-slicing simulated and its isolation trade-off explained.

### L9.5 — MLOps, serving, cost

- [ ] Model registered with an alias; rollback target defined.
- [ ] Canary serving with per-version metrics; the split ratio visible.
- [ ] Load test table recorded; the queueing point identified.
- [ ] Cost per 1M tokens computed with stated assumptions and a utilization factor.
- [ ] Drift probe run and interpreted as prediction drift, not model failure.

---

## Oral exam (10 questions, pass 7/10)

Answer aloud, in under two minutes each, without notes.

1. A training job shows 100% GPU utilization but throughput dropped 40%. What do you check, in order?
2. Explain why adding GPUs can make a job slower, with one concrete number.
3. Your pod is pending with free GPUs. Walk the diagnostic path.
4. What does the device plugin not do, and what fills each gap?
5. MIG vs time-slicing: pick one for a two-team inference node and defend it.
6. Explain `algbw` vs `busbw` and what a discrepancy tells you.
7. Your checkpoint write succeeded but the resume started from step 0. What happened?
8. A model's online accuracy dropped but offline eval is unchanged. Give the first three things you check.
9. Cost per 1M tokens doubled with flat traffic. Where do you look, in order?
10. You have 30 minutes and no GPU access. How do you still prepare for a GPU interview?

---

## Retest log

| Date | Part A | Part B | Part C | Part D | Total | Practical | Notes |
|---|---:|---:|---:|---:|---:|---|---|
| | /50 | /30 | /40 | /40 | /160 | pass/fail | |
| | | | | | | | |
| | | | | | | | |

A retest is a new set of questions or a new drill, not the same paper memorized. If you miss the same concept twice, write a one-page explanation and teach it back before the next attempt.

---

## Scoring guide

| Band | Score | What it means |
|---|---:|---|
| Strong | 144–160 | Can design and defend an AI platform; ready to operate with supervision |
| Pass | 128–143 | Solid fundamentals; revisit the missed scenarios |
| Near | 112–127 | Knows the vocabulary, weak on failure modes and trade-offs |
| Not yet | <112 | Re-study the phase; do not substitute tool tutorials for the fundamentals |

A pass requires at least 32/40 on Part C. A candidate who can recite CUDA terms but cannot diagnose a hang, a drift alert, or a cost spike is not ready.
