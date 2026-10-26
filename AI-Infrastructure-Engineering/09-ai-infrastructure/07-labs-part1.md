# Phase 9 Labs — Part 1 (L9.1–L9.3)

> **GPU policy:** no lab in this part assumes you own a GPU. L9.1 is read-only against a provided `nvidia-smi` capture (or a real GPU if you have one); L9.2 and L9.3 run entirely on CPU with PyTorch and `gloo`. The reasoning transfers: the same measurement discipline applies whether the device is a GPU or a CPU core.
>
> **Safety:** do not run GPU stress tests, MIG reconfiguration, or driver installs on a shared or production host. Do not download model weights from untrusted sources. No cloud instances are required for this part.
>
> **Environment:** Linux VM or macOS, 4 vCPU / 8 GB RAM, Python 3.11+, 5 GB free disk. Commands assume Bash; on macOS, `time -p` and `python3 -m pip` behave the same, but check `ulimit` differences if you raise thread counts.

---

# Lab L9.1 — GPU Inspection and the Roofline Question

## Objective

Read GPU telemetry the way you would read `top`, `ss`, and Prometheus: as evidence for a bottleneck hypothesis. By the end you can:

1. Interpret `nvidia-smi` fields: utilization, memory, temperature, power, clocks, ECC, persistence mode.
2. Map processes to GPUs and attribute usage to a container or user.
3. Explain why `GPU-Util 100%` does not mean useful work.
4. Compute a roofline estimate and classify a kernel as compute-bound or memory-bound.
5. Diagnose four captured failure states: idle-but-busy, thermal throttle, ECC fault, and memory exhaustion.

You will work from **evidence first**: each observation has a "what would you check next" question, and the analysis section is separated from the observation section.

## Prerequisites

- A Linux VM or macOS with Python 3.11+, `jq`, and 5 GB free disk.
- The provided telemetry capture in `telemetry/` (created in Setup; includes a real `nvidia-smi` capture from an 8×A100 node if you do not have a GPU).
- If you have a physical NVIDIA GPU: `nvidia-smi` must run without error, and you must **not** own the host (shared lab GPU) without permission for read-only use. Read-only queries are safe; do not run the stress section.
- 60–90 minutes.

Check:

```bash
python3 --version
jq --version
nvidia-smi --version 2>/dev/null || echo "no local GPU — use the capture path"
```

## Setup

Create the project:

```bash
mkdir -p ~/opslab/phase9/l9.1/telemetry
cd ~/opslab/phase9/l9.1
```

### The telemetry capture

If you have a GPU, capture your own (read-only):

```bash
nvidia-smi -q -x > telemetry/nvidia-smi-full.xml
nvidia-smi --query-gpu=index,uuid,name,utilization.gpu,utilization.memory,memory.used,memory.total,temperature.gpu,power.draw,clocks.sm,clocks.mem,clocks_throttle_reasons.active,ecc.mode.current,ecc.errors.uncorrected.volatile.total --format=csv > telemetry/summary.csv
nvidia-smi --query-compute-apps=pid,process_name,used_memory --format=csv > telemetry/processes.csv
nvidia-smi topo -m > telemetry/topo.txt 2>/dev/null || true
```

If you do not have a GPU, create the capture files by hand. Write this exactly as shown (it is a representative 8×A100 SXM node mid-training-run):

```bash
cat > telemetry/summary.csv <<'EOF'
index, uuid, name, utilization.gpu [%], utilization.memory [%], memory.used [MiB], memory.total [MiB], temperature.gpu, power.draw [W], clocks.sm [MHz], clocks.mem [MHz], throttle_reasons.hw_slowdown, ecc.mode.current, ecc.errors.uncorrected.volatile.total
0, GPU-aaaaaaaa-0001, NVIDIA A100-SXM4-80GB, 97, 71, 68420, 81920, 67, 389.45, 1410, 1593, Not Active, Enabled, 0
1, GPU-aaaaaaaa-0002, NVIDIA A100-SXM4-80GB, 96, 68, 67984, 81920, 68, 391.10, 1410, 1593, Not Active, Enabled, 0
2, GPU-aaaaaaaa-0003, NVIDIA A100-SXM4-80GB, 12, 4, 1240, 81920, 41, 68.20, 1410, 1593, Not Active, Enabled, 0
3, GPU-aaaaaaaa-0004, NVIDIA A100-SXM4-80GB, 98, 66, 68901, 81920, 84, 400.02, 1230, 1593, Active, Enabled, 0
4, GPU-aaaaaaaa-0005, NVIDIA A100-SXM4-80GB, 99, 74, 68150, 81920, 66, 393.88, 1410, 1593, Not Active, Enabled, 0
5, GPU-aaaaaaaa-0006, NVIDIA A100-SXM4-80GB, 95, 70, 68422, 81920, 67, 388.90, 1410, 1593, Not Active, Enabled, 2
6, GPU-aaaaaaaa-0007, NVIDIA A100-SXM4-80GB, 0, 0, 402, 81920, 39, 46.55, 1410, 1593, Not Active, Enabled, 0
7, GPU-aaaaaaaa-0008, NVIDIA A100-SXM4-80GB, 47, 39, 31580, 81920, 55, 181.30, 1410, 1593, Not Active, Enabled, 0
EOF

cat > telemetry/processes.csv <<'EOF'
pid, process_name, used_memory [MiB]
4211, python, 34210
4211, python, 33900
4211, python, 0
4211, python, 34410
4211, python, 34100
4211, python, 34220
4211, python, 0
4211, python, 15600
9982, jupyter-lab, 1180
9982, jupyter-lab, 0
9982, jupyter-lab, 1180
9982, jupyter-lab, 0
9982, jupyter-lab, 0
9982, jupyter-lab, 0
9982, jupyter-lab, 0
9982, jupyter-lab, 0
EOF
```

Expected layout:

```text
l9.1/
├── telemetry/
│   ├── summary.csv
│   └── processes.csv
└── roofline.py
```

## Part A — Read the telemetry

Answer these from `telemetry/summary.csv` **before** reading the analysis in the internal explanation. Write your answers in the write-up template.

### A1. Per-GPU triage

For each GPU 0–7, write one line:

```text
GPU0: state=?, evidence=?, question=?
```

Suggested states: `healthy-busy`, `healthy-idle`, `starved`, `thermally-throttled`, `ecc-error`, `suspicious-partial`.

### A2. Process attribution

Sum `used_memory` per PID from `processes.csv`. Two PIDs exist. Which one is the distributed training job and which one is a notebook? Which GPUs does each PID hold, by index? What is inconsistent about the notebook's footprint?

### A3. The arithmetic question

GPU3 shows `clocks.sm 1230 MHz` instead of `1410 MHz`. Using a first-order estimate that SM throughput is proportional to SM clock, what fraction of peak compute does it deliver relative to the others? Is this consistent with its `utilization.gpu` reading of 98%? What does that tell you about using utilization alone?

### A4. The idle-but-allocated question

GPU2 has 12% utilization and 1,240 MiB used; GPU7 has 47% and 31,580 MiB. The training job's collective is all-reduce at the end of each step. Where in the step do you think these GPUs are spending their time, and what evidence would distinguish "straggler rank" from "fragmented placement"?

## Part B — The roofline estimate

Write `roofline.py`:

```python
#!/usr/bin/env python3
"""Roofline and memory-budget estimator for a training run.

Usage:
  python3 roofline.py --flops 312e12 --hbm_bw 1935e9 --params 7e9 \
      --dtype 2 --batch 8 --seq 4096 --grad_per_param 16
"""
import argparse

p = argparse.ArgumentParser()
p.add_argument("--flops", type=float, required=True,
               help="peak achievable FLOP/s (use measured, not datasheet, if you have it)")
p.add_argument("--hbm_bw", type=float, required=True, help="peak HBM bandwidth B/s")
p.add_argument("--params", type=float, required=True, help="parameter count")
p.add_argument("--dtype", type=float, default=2, help="bytes per weight value")
p.add_argument("--batch", type=int, required=True)
p.add_argument("--seq", type=int, required=True)
p.add_argument("--grad_per_param", type=float, default=16,
               help="1/dtype + grad + 2 optimizer states, expressed in weight-units")
args = p.parse_args()

# --- memory budget -------------------------------------------------------
weights_bytes = args.params * args.dtype
optimizer_bytes = args.params * 4 * 2          # AdamW: fp32 m and v
grads_bytes = args.params * args.dtype
fixed = weights_bytes + optimizer_bytes + grads_bytes

# --- arithmetic intensity of one transformer step (crude) ----------------
# 6*N*P for training: 2P forward + 4P backward per parameter per token
tokens = args.batch * args.seq
train_flops = 6 * args.params * tokens
ai = train_flops / (args.grad_per_param * args.dtype)   # FLOP per byte moved

ridge = args.flops / args.hbm_bw
bound = "compute-bound" if ai > ridge else "memory-bound"

# --- step time estimate --------------------------------------------------
step_time_compute = train_flops / args.flops
step_time_memory = (args.grad_per_param * args.dtype) / args.hbm_bw
step_time = max(step_time_compute, step_time_memory)

print(f"tokens/step           = {tokens:,}")
print(f"train FLOPs/step      = {train_flops/1e12:,.1f} TFLOP")
print(f"fixed memory          = {fixed/1e9:,.1f} GB")
print(f"arithmetic intensity  = {ai:,.0f} FLOP/byte")
print(f"ridge point           = {ridge:,.0f} FLOP/byte")
print(f"classification        = {bound}")
print(f"step time est.        = {step_time*1000:,.0f} ms "
      f"(compute {step_time_compute*1000:,.0f} ms, memory {step_time_memory*1000:,.0f} ms)")
print(f"NOTE: activations/KV/fragmentation not included; measure, don't trust.")
```

Run it for three scenarios:

```bash
# 7B model, bf16, batch 8, seq 4096 on one A100 (peak 312 TFLOPS bf16, 1935 GB/s HBM)
python3 roofline.py --flops 312e12 --hbm_bw 1935e9 --params 7e9 --dtype 2 \
  --batch 8 --seq 4096

# same model, batch 1, seq 512 (interactive/small-batch regime)
python3 roofline.py --flops 312e12 --hbm_bw 1935e9 --params 7e9 --dtype 2 \
  --batch 1 --seq 512

# 7B model inference, batch 1, generating tokens (backward never runs)
python3 roofline.py --flops 312e12 --hbm_bw 1935e9 --params 7e9 --dtype 2 \
  --batch 1 --seq 512 --grad_per_param 2
```

### Expected observations

Record in your write-up:

1. The large-batch training case: arithmetic intensity, ridge point, classification, estimated step time.
2. The small-batch case: classification, and why it flips.
3. The inference case: classification, and why `grad_per_param=2` (weights once forward, plus a second pass for the output projection) matters.
4. Compare the batch-8 step time against your measured (or documented) real step time if you have one; note the gap and what the estimate ignored (activations, communication, host time).

### A note on honesty of the model

This estimator is deliberately crude. It ignores activations (which grow with `batch × seq × hidden`), cross-layer KV cache, optimizer warmup, and communication. That is the point: a first-order estimate is for **classification** (compute-bound vs memory-bound), not for a schedule. When the classification and the measured behavior disagree, measure before you tune.

## Internal explanation

### Why 100% utilization lies

`utilization.gpu` in `nvidia-smi` is the fraction of a sample window during which **at least one kernel was resident on the GPU**. It says nothing about:

- whether the kernel was doing tensor math or a 1-element memory copy;
- whether the SMs were occupied or running a grid of 1 block;
- whether the time was spent waiting in a collective.

A training step is `forward + backward + all-reduce + optimizer`. During all-reduce, kernels are often resident (NCCL kernels spin) but no useful compute happens. Six GPUs at 97% and two at 12% with the same job is the signature of a **collective**: the fast ranks are spinning in the all-reduce waiting for the slow ones, showing high "utilization," while the straggler GPU3 (throttled) finishes its work late and the ring's other members wait. GPU2 at 12% is likely idle during most of the window — a fragmented or partially allocated rank.

**The right unit of progress is the application metric:** samples/s, tokens/s, steps/s. Utilization is a correlate, not a goal.

### Why the roofline flips with batch size

Arithmetic intensity is `FLOPs / bytes_moved`. Per parameter per token, training moves roughly `grad_per_param × dtype` bytes (weights once in forward, gradient in backward, plus optimizer traffic amortized) while doing `6 × P` FLOPs. Larger batches multiply the FLOPs per weight-load: the same weights are reused across more tokens, so each byte fetched from HBM buys more arithmetic. At batch 1 × seq 512, the model re-reads weights for very few tokens, so it becomes **memory-bandwidth-bound**: adding FLOPs does nothing, feeding the GPU does everything. That is exactly the large-model-small-batch regime, and why increasing batch size (or using better kernel fusion) is the first lever there.

### Why the throttle state matters

GPU3 shows `clocks.sm 1230 MHz` and `throttle_reasons.hw_slowdown: Active` with temperature 84 °C. Hardware slowdown is the GPU protecting itself; clocks fall, throughput falls, and a synchronized job waits. Fixing this is a cooling/airflow problem, not a code problem. Increasing the NCCL timeout would only make the symptom last longer.

### Why the ECC counter matters

GPU5 shows `ecc.errors.uncorrected.volatile.total = 2`. Two uncorrected ECC errors means data may already be wrong, and the number will grow. The correct action is to finish/checkpoint, drain the node, and run a diagnostic (DCGM level 2/3); a repeat means service the GPU. Silent corruption is worse than a crash: a training run with two bit flips can produce a subtly worse model, and nothing in the loss curve tells you.

## Troubleshooting task

Without looking at the analysis above, re-derive the four fault states from `telemetry/summary.csv` alone:

1. Which GPU is thermally throttled, and what is the evidence?
2. Which GPU has an ECC problem, and what is the immediate action?
3. Which GPU is most likely starved rather than busy, and what would you measure to confirm?
4. What single deployment change would remove the notebook's interference with training?

Then simulate the **straggler test**: if GPU3 were replaced by a healthy GPU, estimate the new step time using the roofline script (the throttled GPU delivered ~87% of peak clock). Compare against the unthrottled ranks' estimate and explain the change in scaling efficiency.

## Verification

- [ ] All eight GPUs triaged with a state, evidence, and open question.
- [ ] Process attribution sums correct and identifies the notebook inconsistency (its memory appears on GPUs where it claims zero usage).
- [ ] Roofline runs for all three scenarios; classifications recorded with the numbers.
- [ ] The batch-8 step-time estimate is compared to a measured or documented real step time, with the gap explained.
- [ ] The four fault states re-derived independently before reading the analysis.
- [ ] No writes, stress tests, or MIG changes were run against any GPU host.

## Cleanup

Nothing persists: the telemetry files and `roofline.py` are yours to keep or delete.

```bash
cd ~ && rm -rf ~/opslab/phase9/l9.1   # only if you do not want the write-up evidence
```

If you used a real GPU host with permission, confirm no process you started is still resident:

```bash
nvidia-smi --query-compute-apps=pid --format=csv,noheader
```

## Write-up template

```markdown
## L9.1 — GPU inspection and roofline

### Environment
- Host: (VM/macOS/GPU host), nvidia-smi capture: own/provided

### Part A triage table
| GPU | state | evidence | open question |
|---|---|---|---|

### Process attribution
- PID 4211 = ... ; PID 9982 = ... ; inconsistency: ...

### Roofline results
| scenario | AI (FLOP/byte) | ridge | classification | step-time est. |
|---|---|---|---|---|

### Fault states (re-derived before reading analysis)
1. throttle → ...
2. ECC → ...
3. starved → ...
4. deployment fix → ...

### What I would automate
...

### One thing that surprised me
...
```

---

# Lab L9.2 — Train and Serve a Tiny Model, With Numbers

## Objective

Run the full loop on CPU — train, track, package, serve, measure — so the operational questions are practiced without a GPU. By the end you can:

1. Train a small PyTorch model with a fixed seed and a defined data split.
2. Track hyperparameters, metrics, and artifacts in MLflow.
3. Package the model with its preprocessing, pinned by hash.
4. Serve it behind a small API with real resource accounting.
5. Measure latency distribution (p50/p95/p99), throughput, and compute cost per 1,000 requests.
6. Diagnose the classic serving faults: queueing, a slow path, and a memory leak.

The model's accuracy does not matter. The **measurement contract** does.

## Prerequisites

- Linux VM or macOS, 4 vCPU / 8 GB.
- Python 3.11+, `pip`.
- No GPU, no cloud, no external model APIs.
- 90 minutes.

Check:

```bash
python3 -c "import sys; print(sys.version)"
python3 -m pip --version
```

Install (use a virtualenv; do not install into your system Python):

```bash
cd ~/opslab/phase9
python3 -m venv venv
source venv/bin/activate
pip install --quiet "torch==2.4.1" --index-url https://download.pytorch.org/whl/cpu
pip install --quiet "mlflow==2.16.2" "fastapi==0.115.0" "uvicorn==0.30.6" \
  "numpy==1.26.4" "scikit-learn==1.5.1" "httpx==0.27.2"
```

If `pip` resolves slowly, pin only what changes: `torch` CPU wheel is ~200 MB; everything else is small.

## Setup

```bash
mkdir -p ~/opslab/phase9/l9.2/{app,artifacts}
cd ~/opslab/phase9/l9.2
```

### The training script

Create `app/train.py`:

```python
#!/usr/bin/env python3
"""Train a tiny MLP classifier with full tracking and a hash-pinned artifact."""
import hashlib
import json
import time
import numpy as np
import torch
import torch.nn as nn
import mlflow

SEED = 42
mlflow.set_tracking_uri("http://127.0.0.1:5000")
mlflow.set_experiment("tiny-classifier")

def make_data(n=20000):
    rng = np.random.default_rng(SEED)
    X = rng.normal(size=(n, 16)).astype(np.float32)
    y = (X[:, 0] * 1.5 + X[:, 1] ** 2 - X[:, 2] + 0.5 * rng.normal(size=n)) > 0
    return X, y.astype(np.int64)

class MLP(nn.Module):
    def __init__(self):
        super().__init__()
        self.net = nn.Sequential(
            nn.Linear(16, 64), nn.ReLU(),
            nn.Linear(64, 64), nn.ReLU(),
            nn.Linear(64, 2),
        )
    def forward(self, x):
        return self.net(x)

def main():
    t0 = time.time()
    torch.manual_seed(SEED)
    np.random.seed(SEED)

    X, y = make_data()
    n_train = int(0.8 * len(X))
    Xtr, ytr = torch.from_numpy(X[:n_train]), torch.from_numpy(y[:n_train])
    Xva, yva = torch.from_numpy(X[n_train:]), torch.from_numpy(y[n_train:])

    model = MLP()
    opt = torch.optim.AdamW(model.parameters(), lr=1e-3, weight_decay=0.01)
    loss_fn = nn.CrossEntropyLoss()

    mlflow.start_run(run_name="mlp-seed42")
    mlflow.log_params({"lr": 1e-3, "wd": 0.01, "epochs": 20, "batch": 512,
                       "seed": SEED, "n_train": n_train})

    for epoch in range(20):
        model.train()
        perm = torch.randperm(n_train)
        for i in range(0, n_train, 512):
            idx = perm[i:i+512]
            opt.zero_grad()
            loss = loss_fn(model(Xtr[idx]), ytr[idx])
            loss.backward()
            opt.step()
        model.eval()
        with torch.no_grad():
            acc = (model(Xva).argmax(1) == yva).float().mean().item()
        mlflow.log_metric("val_accuracy", acc, step=epoch)
        print(f"epoch {epoch:02d}  val_acc {acc:.4f}")

    # --- package ---------------------------------------------------------
    scripted = torch.jit.script(model)
    path = "artifacts/model.pt"
    torch.jit.save(scripted, path)
    with open("artifacts/hash.txt", "w") as f:
        f.write(hashlib.sha256(open(path, "rb").read()).hexdigest())
    mlflow.log_artifact(path)
    mlflow.log_artifact("artifacts/hash.txt")
    mlflow.log_metric("train_seconds", time.time() - t0)
    mlflow.end_run()
    print("artifact:", path, "hash:", open("artifacts/hash.txt").read()[:16], "...")

if __name__ == "__main__":
    main()
```

### The serving app

Create `app/serve.py`:

```python
#!/usr/bin/env python3
"""Serve the trained model with latency accounting and a deliberate slow path."""
import hashlib
import time
from collections import Counter
from fastapi import FastAPI
from pydantic import BaseModel
import torch

app = FastAPI()
MODEL_PATH = "artifacts/model.pt"
EXPECTED = open("artifacts/hash.txt").read().strip()

_start = time.time()
_count = Counter()           # status -> count
_hist = []                   # latencies (s)
_model = None

class Input(BaseModel):
    features: list[float]  # 16 floats
    slow: bool = False

@app.on_event("startup")
def load():
    global _model
    actual = hashlib.sha256(open(MODEL_PATH, "rb").read()).hexdigest()
    if actual != EXPECTED:
        raise RuntimeError(f"artifact hash mismatch: {actual} != {EXPECTED}")
    _model = torch.jit.load(MODEL_PATH)
    _model.eval()

@app.get("/health")
def health():
    return {"status": "ok", "uptime": round(time.time() - _start, 1),
            "loaded": _model is not None}

@app.post("/predict")
def predict(inp: Input):
    t0 = time.perf_counter()
    try:
        if len(inp.features) != 16:
            raise ValueError("need 16 features")
        x = torch.tensor([inp.features], dtype=torch.float32)
        with torch.no_grad():
            prob = torch.softmax(_model(x), dim=1)[0, 1].item()
        if inp.slow:
            time.sleep(0.5)   # deliberate: an injected downstream dependency
        status = 200
        return {"prob": round(prob, 6)}
    except Exception as e:
        status = 500
        return {"error": str(e)}
    finally:
        _count[status] += 1
        _hist.append(time.perf_counter() - t0)

@app.get("/metrics")
def metrics():
    h = sorted(_hist)
    n = len(h)
    def q(p): return h[min(int(p * n), n - 1)] if n else 0.0
    return {
        "requests_total": n,
        "status_counts": dict(_count),
        "latency_p50": round(q(0.50) * 1000, 2),
        "latency_p95": round(q(0.95) * 1000, 2),
        "latency_p99": round(q(0.99) * 1000, 2),
        "rss_kb": open(f"/proc/{__import__('os').getpid()}/status").read()
                 .split("VmRSS:")[1].split()[0] if n else 0,
    }
```

Start the tracking server and train:

```bash
cd ~/opslab/phase9/l9.2
mlflow server --backend-store-uri sqlite:///mlflow.db \
  --default-artifact-root file://$PWD/artifacts --host 127.0.0.1 --port 5000 &
sleep 4
python3 app/train.py
```

Serve:

```bash
uvicorn app.serve:app --host 127.0.0.1 --port 8100 &
sleep 3
curl -s http://127.0.0.1:8100/health | jq
```

Expected:

```json
{"status": "ok", "uptime": 2.3, "loaded": true}
```

## Part A — Verify the measurement contract

### A1. Baseline latency

```bash
for i in $(seq 1 200); do
  curl -s -o /dev/null http://127.0.0.1:8100/predict -H 'Content-Type: application/json' \
    -d "{\"features\": [$(python3 -c 'print(",".join(["0.1"]*16))')]}"
done
curl -s http://127.0.0.1:8100/metrics | jq
```

Expected (4 vCPU VM):

```json
{
  "requests_total": 200,
  "status_counts": {"200": 200},
  "latency_p50": 1.9,
  "latency_p95": 3.2,
  "latency_p99": 4.8
}
```

Your numbers will differ. Record them.

### A2. The deliberate slow path

```bash
curl -s -o /dev/null -w 'total=%{time_total}s\n' http://127.0.0.1:8100/predict \
  -H 'Content-Type: application/json' \
  -d "{\"features\": [$(python3 -c 'print(",".join(["0.1"]*16))')], \"slow\": true}"
```

Expected: `total≈0.51s`. This models a downstream dependency (a feature store, a database). Re-read `/metrics`: p50 should be barely changed, p99 dominated.

### A3. Concurrency and queueing

Fire 20 concurrent requests with `slow: true`:

```bash
seq 20 | xargs -P20 -I{} curl -s -o /dev/null -w '%{http_code}\n' \
  http://127.0.0.1:8100/predict -H 'Content-Type: application/json' \
  -d "{\"features\": [$(python3 -c 'print(",".join(["0.1"]*16))')], \"slow\": true}"
curl -s http://127.0.0.1:8100/metrics | jq '{p95: .latency_p95, p99: .latency_p99, rss_kb}'
```

Note how p99 approaches `20 × 0.5s` when the server processes requests **serially** (single uvicorn worker). That is the queueing lesson: no amount of model tuning fixes serialization; concurrency must be designed.

## Part B — Cost per 1,000 requests

Record from `top` or `ps` the CPU-seconds the server consumed for the 200-request baseline:

```bash
ps -o pid,%cpu,etime,comm -p $(pgrep -f 'uvicorn app.serve')
```

Compute:

```text
cpu_seconds = %cpu/100 × elapsed_seconds
cpu cost per 1000 requests = cpu_seconds × rate
```

With a $0.0445/hour vCPU (a common on-demand cloud rate) the arithmetic is:

```text
cpu_seconds per request ≈ measured
cost per 1000 requests = cpu_seconds_per_request × 1000 × (0.0445/3600)
```

For example, if the server used 4.2 CPU-seconds for 200 requests (21 ms/request):

```text
21 ms/request × 1000 = 21 s per 1000 requests
21 s × $0.0445/3600 ≈ $0.00026 per 1000 requests (CPU only)
```

Now imagine the model is on a $3.20/hour A100 instead. Even at 100% utilization with the same per-request latency (impossible here, but for scale):

```text
$3.20/hour ÷ 3600 = $0.000889/s
21 ms/request × 1000 = 21 s → $0.0187 per 1000 requests
```

**Observation:** the GPU path costs ~70× more per 1,000 requests at these latencies. Small models and short sequences are a CPU (or quantized/small-GPU) workload. The point is not that GPUs are bad; it is that **the choice must be measured, and the unit must be stated** (per request, per 1k, per 1M tokens).

## Expected observations

- Baseline p50 ≈ 2 ms, p99 under 5 ms on 4 vCPU (yours will differ; record it).
- One slow request adds ~500 ms to that single request, barely moves p50, and dominates p99 once it is in the sample.
- 20 concurrent slow requests: p99 ≈ 10 s with a single worker, because requests serialize.
- `/metrics` `status_counts` stays at `200` — HTTP status alone hides the 500 ms dependency.
- The hash check passes; tampering with `artifacts/model.pt` (flip a byte) makes startup fail with `artifact hash mismatch`.

## Internal explanation

### Why measure p50/p95/p99 separately from status counts

HTTP 200 tells you the request completed, not how long it took or whether the answer was useful. Latency percentiles tell you about the tail; the tail is where users feel pain and where capacity problems live. The `slow` flag shows how one slow dependency inflates the tail while leaving the median untouched: at 200 samples, a single 500 ms request is the 100th percentile, invisible to p50.

### Why the hash check is the interesting part

The hash pins **the exact bytes** that were trained and evaluated. If the artifact changes without the hash changing, your evaluation is meaningless; if the hash changes without your knowledge, someone else's model is serving your traffic. In production this check is not a nicety; it is the integrity contract between training, evaluation, and serving. The same discipline applies to container digests and dataset hashes.

### Why cost per 1k requests is the right unit for serving

GPU-hours and CPU-hours are inputs; requests are outputs. Two deployments with identical GPU-hours can differ 10× in cost per request depending on batching, concurrency, and model size. Cost per request (or per 1M tokens for LLMs) is the unit that lets you compare a small model on CPU, a quantized model on one GPU, and a large model on four GPUs.

## Troubleshooting task

Break the app in three ways and fix each from evidence:

1. **Corrupt the artifact:** `printf '\x00' | dd of=artifacts/model.pt bs=1 seek=100 conv=notrunc`. Restart `uvicorn`. It should refuse to start with the hash mismatch. Fix by restoring from MLflow artifacts (`mlflow artifacts list --run-id <id>` and download), not by deleting the check.
2. **Hide the dependency:** set `slow: true` only 5% of the time (edit `serve.py` to sleep randomly). Explain why p50/p95 still look healthy while a few users suffer — and what metric would catch it (p99 per route, or a histogram, not an average).
3. **Serialize on purpose:** run uvicorn with `--workers 1` and fire 20 concurrent normal requests while `time.sleep(0)` is removed. Measure p99. Then run with `--workers 4` and measure again. Explain the result using Little's law (L = λW; with 20 concurrent requests and 2 ms each, concurrency is the constraint, not CPU).

## Verification

- [ ] `train.py` produced a model, logged 20 epochs of `val_accuracy` to MLflow, and wrote the hash.
- [ ] `/health` shows `loaded: true` after the hash check.
- [ ] Baseline metrics recorded with p50/p95/p99 and status counts.
- [ ] Slow-path single request measured (~500 ms) and p99 impact noted.
- [ ] 20-concurrent slow requests measured; queueing explained with Little's law.
- [ ] Cost per 1,000 requests computed for CPU and compared against a GPU estimate, with the utilization assumption stated.
- [ ] All three troubleshooting faults reproduced, diagnosed from evidence, and fixed.

## Cleanup

```bash
pkill -f 'uvicorn app.serve' || true
pkill -f 'mlflow server' || true
```

Keep `~/opslab/phase9/l9.2` if you want the write-up evidence; delete it if not:

```bash
rm -rf ~/opslab/phase9/l9.2  # optional
```

The MLflow SQLite DB lives inside `l9.2/`, so cleanup removes it too. If you want to keep the experiment, move `mlflow.db` and `artifacts/` out first.

## Write-up template

```markdown
## L9.2 — Train and serve with numbers

### Environment
- Host / vCPU / RAM:
- versions: torch, mlflow, fastapi:

### Training
- epochs=20, final val_accuracy=..., train_seconds=...
- run_id: ..., artifact hash: ...

### Serving measurements
| test | p50 | p95 | p99 | notes |
|---|---|---|---|---|
| baseline 200 req | | | | |
| 1 slow req | | | | |
| 20 concurrent slow | | | | |
| 20 concurrent fast, 1 worker | | | | |
| 20 concurrent fast, 4 workers | | | | |

### Cost
- cpu_seconds/request = ...
- cost per 1000 requests (CPU) = ...
- GPU comparison, with stated assumptions: ...

### Faults reproduced and fixed
1. hash mismatch → ...
2. hidden slow path → metric that catches it: ...
3. serialization → ...

### What I would add for production
...
```

---

# Lab L9.3 — Distributed Collectives on CPU (gloo), With the Scaling Math

## Objective

Run a real distributed job with `torchrun` and measure what collectives cost. By the end you can:

1. Explain the roles in `torchrun`: rank, local rank, world size, master.
2. Run all-reduce with gloo over CPU (no GPU, no NCCL) and measure bandwidth.
3. Verify the ring all-reduce bandwidth model (busbw ≈ 2(N−1)/N × algbw) with your own numbers.
4. Demonstrate a straggler and a dead rank, and explain why the healthy ranks hang.
5. Distinguish gloo from NCCL and know when the difference matters.

This lab is CPU-only on purpose: gloo gives you the **collective semantics and the measurement discipline** without an accelerator. The NCCL-specific behavior (GPU-aware RDMA, NVLink) is covered in note 03 and in the checkpoint; here you practice the measurement that you would repeat against a real GPU cluster.

## Prerequisites

- Linux VM or macOS, 4 vCPU / 8 GB (2 ranks minimum; 4 for the scaling test).
- Python 3.11+, the same venv as L9.2 (torch CPU is installed).
- `time` and `ss` for evidence.
- 60–90 minutes.

Check:

```bash
python3 -c "import torch; print(torch.__version__, torch.distributed.is_available())"
nproc
```

## Setup

```bash
mkdir -p ~/opslab/phase9/l9.3
cd ~/opslab/phase9/l9.3
```

### The collective micro-benchmark

Create `collective.py`:

```python
#!/usr/bin/env python3
"""Measure all-reduce cost on CPU with gloo, and verify the ring model."""
import argparse, os, time
import torch
import torch.distributed as dist

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--bytes_per_rank", type=int, default=64 * 1024 * 1024,
                   help="elements are f32; 64 MiB = 16M elements")
    p.add_argument("--iters", type=int, default=5)
    p.add_argument("--straggler_ms", type=int, default=0,
                   help="simulated rank0 extra compute before the collective")
    args = p.parse_args()

    dist.init_process_group(backend="gloo")
    rank, world = dist.get_rank(), dist.get_world_size()
    torch.set_num_threads(max(1, os.cpu_count() // max(1, world)))

    n = args.bytes_per_rank // 4
    x = torch.ones(n) * rank

    alg_bws, wall = [], []
    for i in range(args.iters):
        if rank == 0 and args.straggler_ms:
            time.sleep(args.straggler_ms / 1000.0)
        t0 = time.perf_counter()
        dist.all_reduce(x, op=dist.ReduceOp.SUM)
        dist.barrier()          # include the tail so every rank sees the same end
        dt = time.perf_counter() - t0
        alg_bws.append(args.bytes_per_rank / dt / 1e9)
        wall.append(dt)
    avg_alg = sum(alg_bws) / len(alg_bws)
    avg_wall = sum(wall) / len(wall)
    bus = avg_alg * 2 * (world - 1) / world
    if rank == 0:
        print(f"world={world} bytes={args.bytes_per_rank/1e6:.0f}MB "
              f"straggler={args.straggler_ms}ms")
        print(f"algbw={avg_alg:,.2f} GB/s  busbw≈{bus:,.2f} GB/s  wall={avg_wall*1000:,.1f}ms")
        print(f"ring model expects busbw ≈ 2(N-1)/N × algbw = "
              f"{2*(world-1)/world:.3f} × algbw")
    # correctness: after SUM over ranks 0..N-1, every element must equal N(N-1)/2
    expect = sum(range(world))
    assert torch.allclose(x[0], torch.tensor(float(expect))), f"rank{rank} wrong result"
    dist.destroy_process_group()

if __name__ == "__main__":
    main()
```

Run with 2 ranks:

```bash
torchrun --nproc_per_node=2 collective.py --bytes_per_rank=67108864
```

Expected output (4 vCPU VM; your numbers differ):

```text
world=2 bytes=64MB straggler=0ms
algbw=1.83 GB/s  busbw≈1.83 GB/s  wall=35.1ms
ring model expects busbw ≈ 2(N-1)/N × algbw = 1.000 × algbw
```

With 4 ranks:

```bash
torchrun --nproc_per_node=4 collective.py --bytes_per_rank=67108864
```

Expected:

```text
world=4 bytes=64MB straggler=0ms
algbw=0.94 GB/s  busbw≈1.41 GB/s  wall=68.4ms
ring model expects busbw ≈ 2(N-1)/N × algbw = 1.500 × algbw
```

**Interpretation:** `algbw` falls as you add ranks (each rank must move its data through the ring), but `busbw` rises toward the ring's `2(N−1)/N` factor. If your measured `busbw` does **not** approach `2(N−1)/N × algbw`, something else is in the path: shared memory contention, CPU oversubscription, or a rank arriving late (the barrier hides some of this; compare `wall` across runs).

## Part A — Scaling and the straggler

### A1. Scaling table

Fill this in from runs at 2 and 4 ranks (and 1 rank as the baseline if your launcher supports `--nproc_per_node=1`):

```text
| ranks | algbw GB/s | busbw GB/s | wall ms | efficiency vs 2-rank busbw |
|---|---|---|---|---|
| 2 | | | | 1.00 |
| 4 | | | | |
```

### A2. Inject a straggler

```bash
torchrun --nproc_per_node=4 collective.py --bytes_per_rank=67108864 --straggler_ms=200
```

Expected: `wall` increases by roughly 200 ms (the other three ranks wait inside `all_reduce` for rank 0), and `algbw`/`busbw` collapse accordingly.

**Observation:** one slow rank out of four slowed **every** rank. The collective is synchronous: its speed is the speed of the slowest participant.

### A3. Kill a rank

In one terminal:

```bash
torchrun --nproc_per_node=4 collective.py --bytes_per_rank=67108864 --iters=200
```

While it runs, from another terminal find and kill one child process:

```bash
ps -ef | grep 'collective.py' | grep -v grep
kill -9 <one_child_pid>
```

Watch the first terminal. Expected: the remaining ranks hang in the next `all_reduce`, then fail with a gloo connection error or a watchdog timeout after the default timeout (10 minutes by default; `torchrun` tears down the whole group). The error mentions a connection reset or a timeout, depending on backend.

**Observation:** healthy ranks do not fail fast when a peer dies; they wait. That is why production training uses a watchdog/timeout and why a scheduler that kills the whole job on one failure is the correct behavior for synchronous training.

### A4. Master/rank wiring

Show what `torchrun` exported:

```bash
torchrun --nproc_per_node=2 --no_python bash -c 'env | grep -E "RANK|WORLD|MASTER|LOCAL_RANK"'
```

Expected:

```text
RANK=0
LOCAL_RANK=0
WORLD_SIZE=2
MASTER_ADDR=127.0.0.1
MASTER_PORT=29500
RANK=1
LOCAL_RANK=1
...
```

**Interpretation:** every rank's `MASTER_ADDR`/`MASTER_PORT` must be identical; a mismatch hangs at `init_process_group`. Multi-node changes `MASTER_ADDR` to the first node's IP and `--node_rank` per node.

## Part B — Why gloo, and when NCCL

| Property | gloo (CPU) | NCCL (GPU) |
|---|---|---|
| Device | CPU tensors | CUDA tensors |
| Transport | TCP/sockets, shared memory | NVLink, PCIe P2P, IB/RoCE, sockets fallback |
| Bandwidth scale | GB/s on host memory | hundreds of GB/s |
| Auto-selected algorithms | basic | topology-aware rings/trees |
| GPU-aware RDMA | no | yes (GPUDirect) |
| Use in this lab | measure semantics and straggler cost cheaply | production collective |

The semantics (SUM correctness, synchronous blocking, rank participation) are identical; the performance characteristics are not. When you move to NCCL, the same measurement (algbw vs busbw vs the 2(N−1)/N model) applies via `all_reduce_perf`.

## Expected observations

- `busbw ≈ 2(N−1)/N × algbw` within ~20% for CPU/gloo with equal ranks.
- A 200 ms straggler adds ~200 ms to every rank's collective time.
- Killing one rank makes the rest hang, then fail with a connection/timeout error; the job as a whole is dead.
- Rank wiring env is identical across ranks except `RANK`/`LOCAL_RANK`.
- The assert proves **correctness** of the collective independent of bandwidth: a fast-but-wrong collective is a silent disaster, so benchmark scripts should check the result.

## Internal explanation

### Why busbw has the 2(N−1)/N factor

In a ring all-reduce of message size S across N ranks, each rank sends S/N chunks twice around the ring (reduce-scatter phase, then all-gather phase). Total data moved per rank is `2S(N−1)/N`. Dividing by wall time gives `busbw`; dividing S by wall time gives `algbw`. Hence `busbw = algbw × 2(N−1)/N`. As N grows, the factor approaches 2, which is why ring bandwidth saturates: the algorithm is efficient at large N; what degrades at large N is the per-rank share of the *network* (contention, hops, NIC bandwidth) and the reduction of per-rank compute to overlap with.

### Why the barrier matters in the measurement

Without `dist.barrier()`, each rank measures its own `all_reduce` return time, which can differ (the last rank to arrive measures a longer time). The barrier makes all ranks measure the same window, so the printed `wall` is the collective's true duration from the job's perspective. It slightly overstates a single rank's view but is the right number for capacity planning.

### Why the default 10-minute timeout is a feature, not a bug

A dead rank can look like a slow network. If the timeout were 10 seconds, transient contention would kill healthy jobs. If it were infinite, a dead peer would waste every other GPU indefinitely. Production practice: set an explicit, measured timeout (e.g., `NCCL_TIMEOUT` / `dist.init_process_group(timeout=...)`) based on your largest legitimate collective, and treat a timeout as a node-health event, not a retry.

## Troubleshooting task

1. **Hang at init:** run rank 1 with a different `MASTER_PORT` by overriding env. Diagnose from the log line that says which address it is connecting to. Fix: identical `MASTER_ADDR`/`MASTER_PORT`.
2. **busbw far below the model:** run with `torch.set_num_threads(1)` vs `nproc/world`. Compare and explain CPU oversubscription as the cause.
3. **Silent wrong result:** change `op=dist.ReduceOp.SUM` to `MAX` in one file copy and observe the assert fail. Explain why a benchmark that only measures time and not correctness would have shipped a broken collective.

## Verification

- [ ] 2-rank and 4-rank all-reduce runs completed with printed `algbw`/`busbw`.
- [ ] The ring-model ratio is verified in your numbers (within ~20%).
- [ ] The straggler experiment shows the wall-time penalty and is explained.
- [ ] The killed-rank experiment shows the hang and the timeout/connection error; the error text is captured in the write-up.
- [ ] Env wiring (`RANK`, `WORLD_SIZE`, `MASTER_ADDR`, `MASTER_PORT`) captured.
- [ ] The correctness assert ran (or you can explain what it proves).
- [ ] All three troubleshooting faults reproduced, diagnosed, and fixed.

## Cleanup

```bash
pkill -f 'torchrun' || true
pkill -f 'collective.py' || true
```

No ports beyond localhost were used. If you ran with a real GPU (optional), confirm no lingering CUDA contexts:

```bash
nvidia-smi --query-compute-apps=pid --format=csv,noheader
```

## Write-up template

```markdown
## L9.3 — Distributed collectives (CPU/gloo)

### Environment
- vCPU, RAM, OS:
- torch version, backend: gloo

### Scaling table
| ranks | algbw | busbw | wall | busbw/algbw | model 2(N-1)/N |
|---|---|---|---|---|---|

### Straggler
- straggler_ms=200, world=4 → wall change: ... explanation: ...

### Killed rank
- observed error: ...
- why healthy ranks hung: ...

### Rank wiring
- env captured: RANK/WORLD_SIZE/MASTER_ADDR/MASTER_PORT

### Faults reproduced
1. init hang →
2. busbw deficit →
3. silent wrong collective →

### What changes with NCCL/GPUs
...
```
