# AI Infrastructure Incidents (INC-18 – INC-20)

> **Evidence first.** Same method. AI incidents add two layers that most engineers have not operated: the **accelerator** (its own telemetry: XID, ECC, memory, clocks) and the **math** (scaling, memory budgets, utilization vs throughput). The discipline is the same as every other incident: evidence, hypotheses, a discriminating test, and a prevention. Never tune a number before naming the bottleneck.

---

## INC-18 — Distributed training hangs at collective initialization

### The page

> **02:10 SEV1** — An 8-node × 8-GPU training job launched at 02:00 hangs after ~30 seconds. Yesterday the same job on the same cluster ran. `nvidia-smi` shows the GPUs allocated and idle. The team wants to blame NCCL.

### The evidence (raw, as collected)

```bash
# On the launcher node, per-rank logs are collected to files:
tail -30 logs/node-0/rank_0.log
tail -30 logs/node-3/rank_3.log
# NCCL_DEBUG=INFO was set in yesterday's run; today's run used default logging. Re-run with it:

NCCL_DEBUG=INFO NCCL_DEBUG_FILE=/tmp/nccl-%h-%p.log torchrun ... train.py   # (short re-run)

grep -E "Bootstrap|NCCL INFO.*Connected|Channel|Using|NET" /tmp/nccl-*.log | head -20

# Per-node hardware state:
ssh node-3 'nvidia-smi topo -m | head -12; ibstat | grep -A3 "CA type\|State"; ibdev2netdev'
ssh node-0 'nvidia-smi topo -m | head -12; ibstat | grep -A3 "CA type\|State"'

# Network reachability between ranks:
ssh node-3 'ping -c 3 -I ib0 10.40.0.10; ss -i | head'
```

```text
Yesterday's healthy log (node-0, rank_0):
NCCL INFO Bootstrap : Using ib0:10.40.0.10<38214>
NCCL INFO NET/IB : Using [0]ib0:RoCE; OOB ib0:...
NCCL INFO Channel 00/32 : 0[7] -> 1[7] via P2P/NVL
... (channels across all 8 GPUs, NVLink paths)

Today's log (node-3, rank_3):
NCCL INFO Bootstrap : Using lo:127.0.0.1<44902>
NCCL INFO NET/Socket : Using [0]lo:127.0.0.1<44902>   <-- fell back to loopback/sockets
(then silence; ranks wait forever)

$ ssh node-3 'nvidia-smi topo -m | head -4'
GPU0 ... PIX PIX ... 
(looks normal)

$ ssh node-3 'ibstat | grep -A3 State'
CA 'mlx5_0': State: Down      <-- the HCA link is DOWN on node-3
CA 'mlx5_1': State: Down

$ ssh node-3 'ibdev2netdev'
mlx5_0 (port 1) ==> ib0 (Down)
mlx5_1 (port 1) ==> ib1 (Down)

$ ssh node-3 'ping -c 3 -I ib0 10.40.0.10'
ping: connect: Network is unreachable
```

### Your turn (before the line)

- [ ] Timeline.
- [ ] Three hypotheses ranked with evidence.
- [ ] The one discriminating test.
- [ ] First 10 minutes.

---
---

# ANALYSIS — INC-18

### Timeline

Node-3's InfiniBand HCAs went `Down` (a switch port/config change or link-layer issue during the previous night's maintenance; the "no changes" claim was about code). At job start, NCCL on node-3 cannot find an IB device in the `Up` state, so it falls back to `NET/Socket` — and, finding no usable network interface in its allowed set (the job's env restricts NCCL to `ib0`-class interfaces via `NCCL_SOCKET_IFNAME=ib0`), it binds bootstrap to loopback. Ranks on other nodes try to reach node-3's bootstrap at its *routed* address; node-3's bootstrap is on `lo`, so the handshake never completes, and every rank waits in the init barrier. The job hangs at collective initialization with all GPUs allocated and idle.

### Hypotheses ranked

1. **One node's IB link is down; NCCL fell back to sockets/loopback and the bootstrap cannot connect.** `ibstat` shows both HCAs Down on node-3; today's NCCL log shows `Bootstrap: Using lo` and `NET/Socket`, versus yesterday's `NET/IB` with NVLink channels. Matches.
2. **NCCL version/driver regression.** Rejected: no software change; yesterday's healthy log exists as a baseline, and the fallback to Socket is a topology-driven decision, not a version behavior.
3. **Wrong `MASTER_ADDR`/port wiring.** Rejected: the launcher env is unchanged; and the per-rank logs show bootstrap on loopback only on node-3, which no master-address mistake would produce selectively.

### The discriminating test

Compare the NCCL bootstrap/`NET` lines between a healthy node and node-3 (`Bootstrap: Using ib0...` vs `Using lo...`), plus `ibstat` per node. One command pair names the failing node and the failing layer. Yesterday's healthy log is the baseline that makes today's diff readable — **capturing healthy logs is part of the job design**, and it is what turns a mystery into a diff.

### Root cause

An IB link-layer outage on one node (HCA state Down) forced a topology fallback that broke the bootstrap handshake. The hang is NCCL's honest behavior in a broken environment: it waits for peers it cannot reach. The fix is at the link layer, not in NCCL settings, and the timeout ("it will eventually fail") is a symptom, not a cause.

### Fix (smallest safe first)

1. Bring node-3's IB links up: check the switch port state and the host's `mlxconfig`/driver state (`ibstat`, `ibdev2netdev`, switch `show interfaces`); re-seat/re-configure or reboot the node into a known-good state if the HCA is wedged.
2. Verify with `ib_write_bw` node-0 ↔ node-3 before rerunning the job (a 30-second test that proves the link, not just the state).
3. Rerun with `NCCL_DEBUG=INFO` and confirm `NET/IB` with NVLink channels as in the healthy baseline.
4. Keep the job's checkpoint policy: after a hang-kill, restart from the last checkpoint, not from scratch.

### Prevention

- A pre-flight job (or a cluster-level canary) that runs `all_reduce_perf` between adjacent nodes hourly; a link that goes Down is caught before a 2-hour training run starts.
- Capture NCCL logs on the **healthy** baseline and diff them on failure — the `NET/IB` vs `NET/Socket` line is the fastest diagnosis in this domain.
- Alert on IB link state and `ib_*` counters from every node; a Down HCA is a node event, not a job event.
- Keep the job's NCCL timeout explicit and modest; a hang should fail in minutes, not hours.

### Lessons

- **Topology is part of the contract.** NCCL chooses transports from what it can see; a missing IB link silently changes the algorithm to one that cannot work.
- **Healthy-baseline logs turn diagnosis into diffing.** Capture `NET/IB`, channels, and busbw on a good day.
- **The GPUs being allocated and idle is the signature of a communication-layer hang,** not a compute-layer problem. Different layer, different tools.

### 5-minute walkthrough script

> "An 8-node training job hung 30 seconds after launch with all GPUs allocated and idle — which says communication layer, not compute. We had NCCL_DEBUG logs from a healthy run the day before, so I diffed: node-3's bootstrap said `Using lo` with `NET/Socket` where healthy logs said `NET/IB` with NVLink channels. `ibstat` on node-3 showed both HCA links Down — the IB link had gone down overnight, so NCCL fell back to a transport that couldn't complete the bootstrap, and every rank waited in the init barrier. The fix was at the link layer: bring the HCA/switch port up, verify with `ib_write_bw` between nodes, rerun with NCCL_DEBUG confirming `NET/IB`. Prevention: a pre-flight `all_reduce_perf` canary and link-state alerting per node, plus healthy-baseline logs to diff against. The lesson: NCCL choosing sockets instead of IB is a topology symptom — fix the link, not the timeout."

---

## INC-19 — Inference p95 spikes at peak while the GPU shows 100% utilization

### The page

> **17:50 SEV2** — A chat assistant's p95 end-to-end latency jumps from 2.1 s to 9 s every weekday evening. The GPU dashboard shows 100% utilization all day. Error rates are normal. The team is considering adding a GPU.

### The evidence (raw, as collected)

```promql
# Serving metrics (vLLM-style), evening window vs midday:
vllm:num_requests_waiting{quantile="0.95"}      evening: 22     midday: 1
vllm:gpu_cache_usage_perc                       evening: 0.97   midday: 0.42
vllm:request_prompt_tokens:mean                 evening: 1850   midday: 640
vllm:time_to_first_token_seconds_bucket{le="0.5"} rate: 0.34     midday: 0.91
vllm:time_per_output_token_seconds              evening: 0.048  midday: 0.041
DCGM_FI_DEV_GPU_UTIL                            evening: 1.00   midday: 0.63
DCGM_FI_PROF_PIPE_TENSOR_ACTIVE                 evening: 0.71   midday: 0.58
```

```bash
# The server's launch configuration (unchanged):
vllm serve ... --max-model-len 16384 --gpu-memory-utilization 0.92 --max-num-seqs 64
```

Request logs show the evening traffic has longer prompts (users paste documents after work) and similar output lengths. No config change preceded the pattern; it appeared after the product's document-chat feature shipped.

### Your turn (before the line)

- [ ] Timeline.
- [ ] Three hypotheses ranked with evidence.
- [ ] The one discriminating test.
- [ ] First 10 minutes.

---
---

# ANALYSIS — INC-19

### Timeline

The document-chat feature increased the **prompt length distribution** in the evening. Long prompts consume KV-cache proportional to tokens; at `--max-model-len 16384`, the pre-allocated cache budget for long contexts plus 64 concurrent sequences saturates `gpu_cache_usage_perc` (0.97 at peak). vLLM's scheduler can only admit requests that fit in the cache, so the rest queue — `num_requests_waiting` p95 is 22 in the evening. TTFT balloons (queue + prefill over long prompts) while per-token decode time barely moves (0.048 s vs 0.041 s), because decode is still fast once a request starts. GPU "utilization" reads 100% because decode/prefill kernels are resident almost continuously — utilization is not throughput.

### Hypotheses ranked

1. **KV-cache saturation from longer evening prompts, causing queueing.** Cache at 0.97, waiting p95 at 22, TTFT up, TPOT flat. Matches.
2. **Decode throughput collapsed (thermal/throttle/interconnect).** Rejected: TPOT is nearly flat, tensor-core activity is up, clocks normal.
3. **A downstream dependency (tokenizer/embedding service).** Rejected: the latency decomposition shows queue+prefill; and the dependency's own metrics are flat.

### The discriminating test

The decomposition: TTFT up + TPOT flat = arrival/queue/prefill problem, not decode. Confirm with the two series (`num_requests_waiting` and `gpu_cache_usage_perc`) rising together in the evening — cache saturation is the mechanism, the queue is the observable.

### Root cause

Capacity planning used a token budget (`--max-model-len 16384`, `--max-num-seqs 64`) sized for short prompts; the evening traffic's longer prompts redefined the memory need per request. The GPU is not slow — it is **full**. Adding a GPU is a valid long-term fix, but the immediate levers are: cap concurrent sequences to what the cache sustains, bound the context, and enable prefix caching so shared system prompts/repeated documents do not re-pay the cache cost.

### Fix (smallest safe first)

1. Reduce `--max-num-seqs` (e.g., 64 → 24) and/or `--max-model-len` to the actual P99 prompt+output need (e.g., 16384 → 8192). This bounds per-request cache cost so admitted requests run without queueing storms; measure TTFT before/after at peak.
2. Enable prefix caching (`--enable-prefix-caching`) if prompts share a template — evening document-chat with a common system prompt benefits heavily.
3. Recompute the honest concurrency: `max_num_seqs ≈ KV_budget_bytes / (2 × layers × kv_heads × head_dim × dtype_bytes × p99_context_tokens)`; document the arithmetic so the next config change starts from a number.
4. If TTFT still misses the SLO at peak, **then** add capacity — with the measured number showing the shortfall, not a feeling.

### Prevention

- Monitor `gpu_cache_usage_perc` and `num_requests_waiting` with an alert (cache >0.9 sustained, or waiting >5) — saturation is visible minutes before TTFT breaches.
- Re-derive serving capacity after any **traffic-mix** change (prompt length is traffic); add a prompt-length histogram to the serving dashboard.
- Load-test with the realistic token mix (long prompts), not a synthetic short-prompt benchmark.
- Record the capacity arithmetic in the runbook so configs are chosen, not inherited.

### Lessons

- **Utilization is not throughput.** 100% `GPU_UTIL` with rising queue depth means the GPU is the constraint — but the constraint is memory/cache, not compute.
- **The KV cache is the capacity unit.** Context length × concurrency is the real budget; `max_num_seqs` without context math is a guess.
- **TTFT vs TPOT is the decomposition that names the layer.** Rising TTFT with flat TPOT = queueing/prefill; both rising = decode/thermal/interconnect.

### 5-minute walkthrough script

> "Evening p95 went from 2.1 s to 9 s with the GPU at 100% utilization and no errors. The decomposition told the story: TTFT ballooned while TPOT stayed flat — queueing or prefill, not slow decode. `gpu_cache_usage_perc` sat at 0.97 and waiting requests p95 at 22 in the evening; the document-chat feature had shifted prompt lengths up, and the serving config — max context 16k, 64 concurrent sequences — had been sized for short prompts, so the KV cache saturated and the scheduler queued everything else. The GPU isn't slow, it's full. The fix was capping concurrency and context to the actual p99 need plus enabling prefix caching; adding a GPU only if the measured shortfall persisted. Prevention: alerts on cache usage and queue depth, and a prompt-length histogram on the serving dashboard. The lesson: the KV cache is the capacity unit — context times concurrency is the real budget."

---

## INC-20 — Accuracy drops after a library-only upgrade

### The page

> **Support escalation, SEV2** — A classifier's production false-positive rate rose ~15% over three days. The last change was a library upgrade (transformers/tokenizers) in the **serving container** — the model weights and preprocessing code are unchanged. Offline evaluation on the golden set is unchanged.

### The evidence (raw, as collected)

```bash
# The serving container diff (what changed):
diff requirements-old.txt requirements-new.txt | head
# transformers 4.40.2 -> 4.44.2, tokenizers 0.19.1 -> 0.20.0

# Reproduce: run the same 1,000 production payloads through both containers
python3 replay.py --old-image registry/app:serving-1.4 --new-image registry/app:serving-1.5 \
  --payloads payloads-1000.jsonl --out diff.jsonl

python3 - <<'PY'
import json
rows = [json.loads(l) for l in open("diff.jsonl")]
diff = [r for r in rows if r["old_pred"] != r["new_pred"]]
print(f"predictions differ on {len(diff)}/{len(rows)} payloads")
for r in diff[:5]:
    print(json.dumps({k: r[k] for k in ("text","old_tokens","new_tokens","old_pred","new_pred")}, ensure_ascii=False))
PY
```

```text
predictions differ on 143/1000 payloads

{"text": "refund the double charge on card ...8231", "old_tokens": [1024, 88, 2109], "new_tokens": [1024, 318, 2109], "old_pred": "REFUND", "new_pred": "OTHER"}

(Inspection: token IDs changed for a few token patterns — the tokenizer's
pre-tokenization/normalization defaults changed between versions, shifting
IDs for contractions and some punctuation. The model is fine; its inputs changed.)
```

Metrics over the 3 days: input token-count distribution shifted slightly (mean 14.8 → 14.1); prediction label distribution shifted (`OTHER` up 9%); the golden-set eval is unchanged because it was run with the **old** preprocessing pinned in the eval harness.

### Your turn (before the line)

- [ ] Timeline.
- [ ] Three hypotheses ranked with evidence.
- [ ] The one discriminating test.
- [ ] First 10 minutes.

---
---

# ANALYSIS — INC-20

### Timeline

The serving container's `tokenizers` upgrade changed default pre-tokenization/normalization, altering token IDs for common patterns. The model (whose vocabulary was trained under the old tokenizer) received slightly different inputs; 143/1,000 sampled payloads produced different predictions. Production false positives rose because the affected patterns skewed toward the FP-prone class. The offline golden-set evaluation was blind to it because the harness pinned the old tokenizer inside its own image — the eval passed while production changed.

### Hypotheses ranked

1. **Training-serving skew from a preprocessing change in the serving container.** A replay through old vs new images shows 14.3% differing predictions with tokenizer evidence; golden eval was computed with the old preprocessing. Matches.
2. **Genuine data drift coincident with the deploy.** Considered: the token-count shift could be traffic; but the replay test isolates the cause — **the same payloads produce different predictions on the two images**, which cannot be traffic drift.
3. **A model/weights regression.** Rejected: weights are unchanged (hash verified); only the container changed.

### The discriminating test

The **replay diff**: identical payloads through the old and new serving images, diffing predictions. It directly proves the serving path changed behavior without touching the model, and it isolates the tokenizer as the mechanism (the token-ID diff shows where).

### Root cause

The tokenizer version became part of the model's effective behavior, but it was not versioned with the model. The upgrade changed inputs silently; no contract test existed between the tokenizer version and the model.

### Fix (smallest safe first)

1. Roll back the serving image to `serving-1.4` (the tokenizer pair it pins) and verify the false-positive rate returns to baseline over the next traffic window.
2. Fix forward deliberately: if the new tokenizer is wanted, **re-evaluate the model under the new tokenizer** on the golden set (and ideally retrain/calibrate); ship it as a model version with its own eval, not as a library bump.
3. Pin and verify: the serving image pins the exact tokenizer version **and** records a tokenizer fingerprint (hash of tokenizer files) at model load; log it with predictions.

### Prevention

- **The preprocessing/tokenizer is part of the deployable unit.** Pin it in the model artifact's manifest; a model version references its tokenizer version.
- A contract test in CI: run a fixed payload set through the serving image and assert outputs match the recorded baseline (any diff fails the build).
- Log the tokenizer/preprocessing fingerprint with predictions; a fingerprint-change alert catches this class in production within minutes.
- Golden-set evaluation must run **through the serving image**, not beside it, so the eval and the service share the exact path.

### Lessons

- **A model's behavior includes its preprocessing.** A "library-only" upgrade is a model change when it alters inputs.
- **The replay diff is the universal tool** for "it was fine before": identical inputs through both versions, diff the outputs. It converts a vague regression into a count and a mechanism.
- **Evaluation harnesses must share the serving path.** An eval that pins its own preprocessing validates a different model than the one serving traffic.

### 5-minute walkthrough script

> "False positives rose 15% after a serving-container library upgrade — weights unchanged, offline eval unchanged. The discriminator was a replay diff: the same 1,000 production payloads through the old and new images produced different predictions on 143 of them, with token IDs differing on contractions and punctuation — the tokenizers upgrade had changed default normalization. The model was fine; its inputs had changed, and the golden-set eval was blind because the harness pinned its own old tokenizer rather than running through the serving image. We rolled back the image, then treated the upgrade properly: re-evaluate under the new tokenizer, ship it as a model version with its own eval. Prevention: the tokenizer fingerprint is pinned and logged with predictions, a fixed-payload contract test runs in CI, and evals go through the serving image. The lesson: preprocessing is part of the model, and a replay diff turns 'it got worse' into 'here is the exact mechanism.'"
