# MLOps, Serving, Monitoring, and Cost

> **The one-sentence model:** a model is not a deliverable; a reproducible artifact plus a serving path plus an observability contract plus a cost model is the deliverable.

---

## 1. The lifecycle, and where it breaks

```text
data → features → train → evaluate → register → package → deploy → serve → observe
  ▲                                                                        │
  └──────────────── retrain / rollback / drift response ◄─────────────────┘
```

Every arrow is a handoff that can lose information:

| Handoff | What is lost if you are careless |
|---|---|
| Data → train | Dataset version, filtering rules, label definitions, split leakage |
| Train → evaluate | Hyperparameters, random seeds, code commit, environment |
| Evaluate → register | Which metric, on which slice, with which threshold |
| Register → package | Preprocessing code, tokenizer, dependency versions |
| Package → deploy | Resource requests, runtime flags, model revision |
| Deploy → serve | Traffic split, warmup, timeout, retry policy |
| Serve → observe | Ground truth, prediction logs, latency decomposition |
| Observe → retrain | Label delay, selection bias, feedback loops |

The platform engineer's job is to make these handoffs **explicit, versioned, and testable**.

### The deployable unit

A model is not just weights. The minimum reproducible unit is:

```text
model artifact (weights)
+ preprocessing/tokenizer
+ inference code
+ dependency lock (Python, CUDA, libraries)
+ container image digest
+ runtime configuration (batch size, max tokens, precision)
+ model card / intended use / evaluation slices
+ signature/schema of inputs and outputs
```

If any of these lives only on someone's laptop, the deployment is not reproducible. Pin the container by digest, not `latest`; pin the model by content hash or registry version, not a mutable path.

---

## 2. Experiment tracking and the model registry

### What to track

For every run:

- **Identity:** run ID, user, timestamp, git commit, branch, dirty flag.
- **Data:** dataset name, version/hash, split, row count, filters.
- **Code/environment:** commit, container digest, Python/CUDA/library versions.
- **Hyperparameters:** learning rate, batch size, epochs, optimizer, precision, seed.
- **Metrics:** loss per step, validation metrics, throughput, GPU-hours, peak memory.
- **Artifacts:** weights, checkpoints, tokenizer, evaluation reports, confusion matrices.
- **Lineage:** parent run (fine-tuning), dataset version, model version.

If you cannot answer "what exactly produced this artifact?" from the tracking system, it is a log, not experiment tracking.

### MLflow in practice

MLflow has four moving parts:

| Component | Purpose |
|---|---|
| Tracking server | Stores runs, params, metrics, artifacts |
| Backend store | Metadata database (SQLite for dev, Postgres for shared) |
| Artifact store | Weights and files (local path, S3, GCS, Azure Blob) |
| Model Registry | Versions, stages/aliases, lineage, approvals |

Run a local server (no external services):

```bash
pip install mlflow==2.16.2
mkdir -p ~/opslab/phase9/mlflow/artifacts
mlflow server \
  --backend-store-uri sqlite:///$HOME/opslab/phase9/mlflow/mlflow.db \
  --default-artifact-root file://$HOME/opslab/phase9/mlflow/artifacts \
  --host 127.0.0.1 --port 5000
```

Log a run:

```python
import mlflow

mlflow.set_tracking_uri("http://127.0.0.1:5000")
mlflow.set_experiment("tiny-classifier")

with mlflow.start_run(run_name="baseline-lr1e-3") as run:
    mlflow.log_params({"lr": 1e-3, "batch_size": 32, "epochs": 3, "seed": 42})
    mlflow.log_metric("val_accuracy", 0.91, step=3)
    mlflow.log_artifact("model.pt")
    mlflow.set_tag("git_commit", "a1b2c3d")
    print(run.info.run_id)
```

Register a model:

```python
mlflow.register_model(
    model_uri=f"runs:/{run_id}/model",
    name="tiny-classifier",
)
```

Promote by alias, not by mutable stage names:

```bash
mlflow models update-model-version --name tiny-classifier --version 3 --alias champion
```

**Why aliases:** "staging" and "production" as stages encouraged in-place mutation and made rollback ambiguous. An alias points at an immutable version; moving the alias is an auditable action. If your MLflow version only supports stages, document the mapping and still pin the version in deployment.

### Registry contract

A registry entry is deployable only if it has:

- a version and content hash;
- the training run and dataset version;
- evaluation results per slice, not just an aggregate;
- the input/output schema;
- resource requirements and tested hardware;
- an owner and an approval;
- a rollback target.

A model without a rollback target is not approved for production.

### Artifact integrity

```bash
sha256sum model.pt
```

Store the hash next to the registry version. At load time, verify it. A mutable artifact path (`s3://models/latest/model.pt`) is a supply-chain risk: the bytes can change without a version change.

---

## 3. Serving patterns

### 3.1 Batch (offline)

Score a large dataset on a schedule, write results to a table/warehouse.

- **Optimized for:** throughput and cost per row; latency irrelevant.
- **Typical stack:** Spark/Ray Data, batch inference job, GPU pool that scales to zero.
- **Failure mode:** a partial output is treated as complete. Write to a staging path, validate row counts/checksums, then atomically publish.

### 3.2 Online (real-time)

A request/response API with a latency SLO.

- **Optimized for:** p95/p99 latency, availability, cost per request.
- **Typical stack:** FastAPI/Triton/vLLM behind a load balancer, autoscaled GPU pods.
- **Failure mode:** cold start and queueing. A new replica takes minutes to load weights; a traffic spike hits a queue before autoscaling reacts.

### 3.3 Streaming

Consume events, score, emit to a topic.

- **Optimized for:** sustained throughput, ordering/backpressure, exactly-once semantics where needed.
- **Failure mode:** consumer lag and poison messages; a slow model backs up the topic.

### 3.4 Serverless / scale-to-zero

- **Optimized for:** sporadic traffic and cost.
- **Failure mode:** cold start dominates; GPU scale-to-zero is often slower than CPU.

### 3.5 Edge

- **Optimized for:** latency, privacy, offline operation.
- **Failure mode:** model size, power, thermal, and update/rollback across a fleet.

### Choosing a pattern

| Question | Batch | Online | Streaming |
|---|---|---|---|
| Is a human waiting? | No | Yes | No |
| Is the input bounded? | Yes | Per request | Unbounded |
| Is latency a product requirement? | No | Yes | Soft |
| Is cost per row the main metric? | Yes | Sometimes | Sometimes |
| Can you tolerate a delayed result? | Yes | No | Seconds/minutes |

Do not put a batch workload behind a synchronous API just because the team knows HTTP. Do not put a latency-critical API on a queue without a deadline and backpressure.

---

## 4. Inference engines: vLLM and Triton

### 4.1 Why LLM serving is different

A traditional model does one forward pass per request. An LLM generates tokens autoregressively: each output token requires a forward pass, and the KV cache grows with context length. Naive batching wastes GPU memory and serializes requests.

Key techniques:

- **Continuous batching:** add/remove requests from the running batch at each decoding step instead of waiting for the longest sequence.
- **PagedAttention:** store the KV cache in fixed-size blocks, reducing fragmentation and allowing prefix sharing.
- **Prefix caching:** reuse KV blocks for a shared system prompt.
- **Quantization:** FP8/INT8/INT4 weights and/or KV cache to fit more on a GPU.
- **Tensor parallel serving:** split a large model across GPUs; the interconnect becomes part of the latency path.
- **Speculative decoding:** a small draft model proposes tokens, the large model verifies; can reduce latency at the cost of complexity.

### 4.2 vLLM concepts

```bash
vllm serve mistralai/Mistral-7B-Instruct-v0.3 \
  --host 0.0.0.0 --port 8000 \
  --max-model-len 8192 \
  --gpu-memory-utilization 0.90 \
  --tensor-parallel-size 1 \
  --max-num-seqs 64
```

| Flag | Effect | Trap |
|---|---|---|
| `--max-model-len` | Max context; sizes the KV cache | Too high reserves memory and lowers concurrency |
| `--gpu-memory-utilization` | Fraction of GPU memory vLLM may use | Too high can starve other processes; too low limits KV cache |
| `--tensor-parallel-size` | Splits the model across GPUs | Must divide attention heads; cross-node TP is latency-sensitive |
| `--max-num-seqs` | Max concurrent sequences | Higher can improve throughput but increases latency and memory |
| `--quantization` | Weight quantization | Quality and kernel support vary |

OpenAI-compatible endpoint:

```bash
curl -s http://127.0.0.1:8000/v1/chat/completions \
  -H 'Content-Type: application/json' \
  -d '{
    "model": "mistralai/Mistral-7B-Instruct-v0.3",
    "messages": [{"role":"user","content":"Explain a KV cache in one paragraph."}],
    "max_tokens": 128,
    "temperature": 0
  }' | jq '.usage, .choices[0].message.content'
```

Representative usage:

```json
{
  "prompt_tokens": 18,
  "completion_tokens": 96,
  "total_tokens": 114
}
```

**Capacity implication:** each concurrent request holds a KV cache proportional to context length. Doubling context can halve the number of concurrent requests, even though the weights are unchanged.

### 4.3 Triton Inference Server concepts

Triton is a general model server with backends (TensorRT, ONNX Runtime, PyTorch, Python, TensorFlow) and a model repository.

```text
model_repository/
└── resnet/
    ├── config.pbtxt
    └── 1/
        └── model.onnx
```

```text
name: "resnet"
platform: "onnxruntime_onnx"
max_batch_size: 32
input [
  { name: "input" data_type: TYPE_FP32 dims: [3, 224, 224] }
]
output [
  { name: "output" data_type: TYPE_FP32 dims: [1000] }
]
dynamic_batching { preferred_batch_size: [8, 16, 32] max_queue_delay_microseconds: 5000 }
```

| Concept | Meaning |
|---|---|
| Model repository | Versioned directory structure; version `1`, `2`, ... |
| `config.pbtxt` | Inputs, outputs, batching, instance groups |
| Dynamic batching | Server-side batching within a delay budget |
| Instance group | Number of model instances per GPU/CPU |
| Ensemble | A DAG of models/preprocessing |
| Model control API | Load/unload/reload without restarting the server |

**Trade-off:** dynamic batching raises throughput and lowers cost per request but adds queue delay. A 5 ms batching window can be invisible for image classification and unacceptable for an interactive API with a 50 ms budget.

### 4.4 Serving SLOs and metrics

Define these precisely:

| Metric | Definition | Why it matters |
|---|---|---|
| **TTFT** | Time from request arrival to first output token | Dominates perceived latency for chat |
| **TPOT / ITL** | Time per output token / inter-token latency | Determines streaming smoothness |
| **End-to-end latency** | Arrival to last token | The user-visible number |
| **Throughput** | Requests/s or output tokens/s | Capacity and cost |
| **Goodput** | Requests/s that meet the SLO | The only throughput that counts |
| **Queue time** | Arrival to model start | Separates overload from slow model |
| **Error rate** | 5xx, timeouts, invalid output | Reliability |
| **GPU utilization / KV-cache usage** | Resource state | Headroom and saturation |

A useful latency decomposition:

```text
E2E = network_in + queue + prefill + decode × output_tokens + network_out
```

If TTFT rises but TPOT is flat, the problem is queueing or prefill (long prompt, cold cache, overloaded scheduler). If TPOT rises but TTFT is flat, the problem is decode (large batch, memory bandwidth, thermal throttle, tensor-parallel communication).

### 4.5 An inference SLO

```text
Availability: 99.9% of requests return a valid response (non-5xx, non-timeout)
TTFT: p95 < 800 ms for prompts <= 2,000 tokens
TPOT: p95 < 50 ms per token
End-to-end: p95 < 8 s for 128 output tokens
Quality: offline eval >= baseline on the golden set; online thumbs-down < 2%
```

State the traffic mix: prompt length, output length, concurrency, and model. An SLO measured on 20-token prompts does not predict 4,000-token prompts. Report the mix next to the latency.

---

## 5. Monitoring a model in production

### 5.1 The four layers

```text
┌────────────────────────────────────────────────────────────┐
│ Business: conversion, revenue, task success                │
├────────────────────────────────────────────────────────────┤
│ Model quality: accuracy, drift, bias, hallucination rate   │
├────────────────────────────────────────────────────────────┤
│ Serving: latency, errors, throughput, queue, saturation    │
├────────────────────────────────────────────────────────────┤
│ Infrastructure: GPU, CPU, memory, network, storage, power  │
└────────────────────────────────────────────────────────────┘
```

Most teams monitor the bottom two and call it "ML monitoring." The top two are what the product cares about, and they fail more slowly and more silently.

### 5.2 Drift, skew, and quality

| Term | Definition | Detection |
|---|---|---|
| **Data drift** | Input distribution `P(X)` changes | Compare feature distributions to a reference window (PSI, KS, Jensen-Shannon) |
| **Concept drift** | Relationship `P(Y|X)` changes | Performance degrades with fresh labels; residual analysis |
| **Prediction drift** | Output distribution changes | Monitor score histograms and label rates |
| **Training-serving skew** | Features computed differently in training and serving | Log serving features, replay through the training pipeline, compare |
| **Label drift** | Label definition or acquisition changes | Data contract tests, label audits |
| **Feedback loop** | Predictions influence future training data | Holdout, exploration, causal review |

**A drift alert is not a model-failure alert.** Drift means "the world changed"; the model may still be correct. Alert on quality where labels are available, and use drift as an early warning with a human review.

### 5.3 Delayed labels

For fraud, conversion, or medical outcomes, the ground truth arrives hours or weeks later. You cannot compute real-time accuracy. Options:

- **Proxy metrics:** model confidence, prediction distribution, business KPIs.
- **Delayed evaluation:** join labels later and backfill quality metrics.
- **Human review sampling:** a small labeled sample per day.
- **Shadow mode:** run a candidate model alongside production and compare.

State the label delay in the monitoring design. A dashboard that shows "accuracy" computed only on the 2% of requests with labels is a biased sample, not the model's accuracy.

### 5.4 Logging without leaking

Log:

- request ID, timestamp, model version, latency, token counts, status;
- a hash or redacted form of sensitive inputs;
- prediction and confidence (subject to policy);
- sampling flags.

Do not log:

- raw PII, secrets, full prompts containing customer data, or auth tokens;
- unbounded text that turns a log store into a data lake.

Use sampling and retention limits. Restrict access. A prediction log is production data with the same privacy obligations as the input.

### 5.5 Shadow, canary, and rollback

```text
             ┌──────────────► model v1 (90%) ──► response
request ─────┤
             └──────────────► model v2 (10%) ──► response (canary)

shadow: request ──► v1 response
                 └► v2 prediction (logged, not served)
```

- **Shadow:** no user impact, but doubles compute and needs a comparison method.
- **Canary:** real traffic to the new model; requires a traffic split, a guardrail metric, and automatic rollback.
- **A/B:** a controlled experiment with a hypothesis and a sample-size plan; not the same as a canary.

Rollback must be one action: move the alias/traffic back. If rollback requires a rebuild and a database migration, it is not a rollback.

---

## 6. Cost: the metric that decides survival

### 6.1 The basic arithmetic

```text
GPU-hour cost = instance_price_per_hour
Cost per request = GPU-hours × price / requests_served
Cost per 1M tokens = GPU-hours × price / (tokens / 1,000,000)
```

Example: an 8×A100 node at $32/hour serves 12 million output tokens in an hour.

```text
cost per 1M tokens = 32 / 12 = $2.67
```

If utilization is 40%, the effective cost per useful token is `2.67 / 0.40 = $6.67`. Idle capacity is the hidden multiplier.

### 6.2 Allocated vs utilized

```text
allocated GPU-hours = sum over pods of (gpu_request × runtime)
utilized GPU-hours  = integral of GPUs with active work over time
utilization         = utilized / allocated
```

A team can hold 32 GPU-hours while using 8. Chargeback based on allocation encourages hoarding; chargeback based on utilization can be gamed by keeping a process alive. Use allocation for capacity planning and utilization for efficiency review, and publish both.

### 6.3 Break-even and batch economics

For a fixed monthly cost `C` and a price per request `p`, the break-even request rate is `C / p`. For a training cluster, the useful metric is **cost per successful experiment**, not cost per GPU-hour:

```text
cost per experiment = GPU-hours × price + engineering hours × loaded rate + failed-run cost
```

A cheaper GPU that takes 3× longer and fails twice as often is not cheaper.

### 6.4 Levers, in order of impact

1. **Utilization:** consolidate workloads, right-size requests, reclaim idle GPUs, use MIG/time-slicing for small jobs.
2. **Model efficiency:** quantization, distillation, pruning, better kernels, continuous batching.
3. **Serving efficiency:** max batch size, KV-cache tuning, prefix caching, autoscaling on queue depth.
4. **Purchasing:** spot/preemptible for fault-tolerant batch, reserved/savings plans for baseline, right instance type.
5. **Scheduling:** bin-packing, gang scheduling, avoiding fragmentation, topology-aware placement.
6. **Data movement:** cache datasets near compute; egress and storage can exceed compute for small jobs.

### 6.5 Spot and preemption economics

Spot is cheap because it can disappear. It is appropriate when:

- the job checkpoints frequently and can resume;
- the job is batch, not serving;
- the scheduler can tolerate losing a node;
- the saved cost exceeds the engineering cost of interruption handling.

It is not appropriate for a single-replica production API without a fallback.

### 6.6 A cost dashboard

| Panel | Question |
|---|---|
| Allocated vs utilized GPU-hours by team | Who is holding capacity vs using it? |
| Cost per 1k requests / 1M tokens by model | Which model is expensive? |
| Idle GPU count and cost | What are we paying for nothing? |
| Queue depth vs GPU utilization | Are we capacity-bound or inefficient? |
| Spot interruption rate and recovery time | Is the cheap capacity reliable enough? |
| Cost per successful training run | Is research throughput improving? |

Put a dollar sign on the dashboard. A utilization graph without cost does not change behavior.

---

## 7. Security and supply chain for models

- **Weights are code-like artifacts.** A pickle-based checkpoint can execute code on load. Prefer `safetensors`, scan artifacts, and load untrusted models in a sandbox without cloud credentials.
- **Pin and verify.** Container digest, model hash, dependency lock. Do not pull `latest` in production.
- **Least privilege.** The serving pod should not have permission to write the model registry, read all training data, or assume an admin role.
- **Network egress.** A model server that can call arbitrary internet endpoints can exfiltrate prompts. Restrict egress; allow only required dependencies.
- **Prompt injection and tool use.** If the model can call tools, treat its output as untrusted input to those tools; require authorization and confirmation for destructive actions.
- **Multi-tenant isolation.** One tenant's prompt/KV cache must not be readable by another; separate processes or instances, not just a shared batch.
- **Licensing.** Model and dataset licenses constrain commercial use; record them in the model card.

---

## 8. Command field manual

### MLflow

```bash
mlflow server --backend-store-uri sqlite:///mlflow.db \
  --default-artifact-root file:///srv/mlflow/artifacts --host 127.0.0.1 --port 5000
```

**Purpose:** run a tracking server with a local metadata DB and artifact store.

**Why:** separates metadata from artifacts; makes runs queryable.

**Interpretation:** if the UI loads but artifacts 404, the artifact root is a path the *server* cannot read, not the client.

**Mistake:** using SQLite on a network filesystem for a shared server; use Postgres for multi-user.

```bash
mlflow runs list --experiment-id 1
mlflow artifacts list --run-id <run_id>
```

**Purpose:** inspect runs and artifacts without the UI.

**Interpretation:** missing artifacts usually mean the client logged to a different tracking URI than the server.

### Model serving smoke test

```bash
curl -sS -o /dev/null -w 'code=%{http_code} total=%{time_total}s\n' \
  http://127.0.0.1:8000/v1/models
```

**Purpose:** verify the server is up and measure baseline latency.

**Interpretation:** `code=200` proves the API is reachable, not that inference works. Follow with a real request.

```bash
curl -s http://127.0.0.1:8000/metrics | grep -E 'vllm:(num_requests|gpu_cache|time_to_first|time_per_output)'
```

**Purpose:** read vLLM's own metrics.

**Interpretation:** `gpu_cache_usage_perc` near 1.0 means KV cache is saturated; `num_requests_waiting` rising means queueing; TTFT/TPOT histograms show where latency goes.

### GPU and process mapping

```bash
nvidia-smi --query-compute-apps=pid,process_name,used_memory --format=csv
nvidia-smi --query-gpu=index,uuid,utilization.gpu,memory.used,memory.total,temperature.gpu,power.draw --format=csv
```

**Purpose:** map PIDs to GPUs and see per-device state.

**Interpretation:** a PID not in any container is a host process or a leak; `used_memory` growing without a matching request rate suggests a leak or a stuck request.

```bash
# map a host PID to a container (containerd)
sudo crictl ps -q | xargs -r -n1 sudo crictl inspect --output json | \
  jq -r 'select(.info.pid == <PID>) | .status.metadata.name'
```

**Purpose:** attribute a GPU process to a pod/container.

**Interpretation:** if no container matches, the process escaped the orchestrator; treat it as an incident until proven benign.

### Cost query (conceptual PromQL)

```promql
sum by (namespace) (
  kube_pod_container_resource_requests{resource="nvidia_com_gpu"}
  * on (namespace) group_left
  namespace_gpu_hour_price
)
```

**Purpose:** estimate allocated GPU cost by namespace.

**Interpretation:** this is a request-based estimate, not actual usage. Multiply by runtime and reconcile with the cloud bill; label cardinality and price mapping are common sources of error.

---

## 9. Failure scenarios

### F1 — The model works in the notebook, fails in the container

Evidence: image digest, Python/CUDA versions, preprocessing code, tokenizer files, input schema, logs at load vs inference.

Likely causes: missing tokenizer/config, different library version, CPU-only wheel, path assumptions, feature computation divergence.

Next action: reproduce with the exact image and the same input; diff the preprocessing outputs.

Prevention: a packaged inference pipeline, a contract test with golden inputs, and a container smoke test in CI.

### F2 — Latency is fine at low traffic, fails at peak

Evidence: queue time, batch size, KV-cache usage, GPU utilization, autoscaler behavior, cold-start time.

Likely causes: queueing at the model server, insufficient replicas, batching delay, KV-cache saturation, a slow dependency.

Next action: separate queue from compute; scale on queue depth/latency, not CPU; cap concurrency and shed load.

Prevention: load test at 2× forecast, define a concurrency limit, and pre-warm replicas.

### F3 — Accuracy dropped after a "no-op" deploy

Evidence: model version, feature pipeline version, training/serving skew check, input distribution, label delay.

Likely causes: a preprocessing change, a new tokenizer, a different default parameter, a feature store schema change, a data pipeline backfill.

Next action: compare a sample of requests through the old and new pipelines; roll back if the guardrail is breached.

Prevention: version preprocessing with the model, contract tests, and a canary with a quality guardrail.

### F4 — GPU memory grows until OOM

Evidence: memory over time, request concurrency, sequence lengths, cache settings, leak repro.

Likely causes: KV cache not freed, unbounded batch, a cache keyed by request, fragmentation, a long-running process with a leak.

Next action: cap concurrency/context, restart on a memory threshold as a stopgap, reproduce with a fixed workload.

Prevention: memory limits at the application level, load tests with long contexts, and a memory-growth alert.

### F5 — The cost per request doubled overnight

Evidence: instance type/price, replica count, utilization, token counts, model version, batch size, spot mix.

Likely causes: a new model with longer outputs, a traffic mix change, autoscaling stuck at max, idle replicas, lost spot capacity replaced by on-demand.

Next action: attribute cost by model/version/route; fix the largest driver; do not just cap replicas blindly.

Prevention: cost per 1k requests as a deploy guardrail, budget alerts, and per-model cost dashboards.

### F6 — The drift alert fires every day

Evidence: reference window, test statistic, sample size, seasonality, a known product change.

Likely causes: a threshold too sensitive, a reference window that no longer represents normal, a legitimate seasonal shift, a data pipeline change.

Next action: inspect the feature contributions; decide whether the model needs retraining or the alert needs a seasonality-aware baseline.

Prevention: backtest the drift detector, use multiple signals, and pair drift with quality where possible.

### F7 — Model registry says version 3, production serves version 2

Evidence: deployment manifest/image tag, model load logs, registry alias, artifact hash, pod start time.

Likely causes: a mutable tag, a cached model on the node, a rollback that was not recorded, a deployment that failed but the old pod kept serving.

Next action: compare the loaded artifact hash with the registry; redeploy by immutable version; record the rollback.

Prevention: deploy by digest/hash, log the loaded model version at startup, and alert on version mismatch.

### F8 — A model artifact executes code during load

Evidence: artifact format (`pickle` vs `safetensors`), source, scan results, process behavior, network egress.

Likely causes: an untrusted pickle, a malicious dependency, a compromised registry.

Next action: isolate the host, revoke credentials, preserve the artifact, rebuild from a trusted source.

Prevention: prefer safe formats, scan artifacts, sandbox loads, and restrict egress/credentials.

---

## 10. Design checklist

- [ ] Every model version traces to a run, dataset version, code commit, and container digest.
- [ ] The deployable unit includes preprocessing/tokenizer and a pinned environment.
- [ ] The registry has immutable versions, an owner, evaluation slices, and a rollback target.
- [ ] The serving pattern matches the latency/throughput requirement.
- [ ] SLOs define TTFT, TPOT, end-to-end, availability, and the traffic mix.
- [ ] The server exposes queue, KV-cache, and token metrics, not just GPU utilization.
- [ ] Autoscaling uses latency/queue signals and accounts for cold start.
- [ ] Drift, skew, and quality are monitored with label-delay caveats.
- [ ] Prediction logs are sampled, redacted, access-controlled, and retained deliberately.
- [ ] Canary and rollback are one action, with a guardrail metric.
- [ ] Cost per 1k requests and per 1M tokens are visible by model/version/team.
- [ ] Allocated and utilized GPU-hours are both reported.
- [ ] Model artifacts are verified, scanned, and loaded with least privilege.
- [ ] A load test at 2× forecast exists and is run before major launches.

---

## 11. Checkpoint

1. What is the minimum reproducible unit for a model deployment?
2. Why is `latest` a supply-chain risk for weights and images?
3. What does an MLflow run need to answer "what produced this artifact?"
4. Why are registry aliases safer than mutable stage names?
5. Compare batch, online, and streaming serving by latency and cost.
6. What problem does continuous batching solve?
7. What is PagedAttention optimizing?
8. Define TTFT, TPOT, and goodput.
9. TTFT rises, TPOT is flat. What are the likely causes?
10. Why can doubling context length halve concurrency?
11. What is the difference between data drift and concept drift?
12. Why is a drift alert not necessarily a model failure?
13. What is training-serving skew, and how do you test for it?
14. Why is accuracy on labeled requests a biased metric?
15. How do you calculate cost per 1M output tokens?
16. Why is allocated GPU-hour cost different from utilized GPU-hour cost?
17. When is spot appropriate, and what must the workload support?
18. What would you put on a cost dashboard, and why?
19. What is the risk of loading an untrusted pickle checkpoint?
20. Design a canary and rollback for a new model version with a quality guardrail.
