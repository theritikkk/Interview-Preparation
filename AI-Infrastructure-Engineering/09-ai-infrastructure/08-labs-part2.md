# Phase 9 Labs — Part 2 (L9.4–L9.5)

> **GPU policy:** L9.4 demonstrates the *scheduling semantics* of GPUs on a kind cluster using **fake extended resources** — no real GPU is needed, and every behavior (integer requests, no overcommit, taints, quotas, fragmentation) is identical to the real thing. A paper alternative covers the full MIG/device-plugin path if you cannot run kind. L9.5 runs the MLOps loop and cost model on CPU.
>
> **Cost:** kind is free. No cloud instances, no GPUs, no external APIs.
>
> **Safety:** do not patch node status on a real cluster you do not own; a wrong patch can make capacity look wrong. Everything here is a disposable kind node.
>
> **Environment:** Linux VM with Docker Engine 24+ (kind does not run without Docker), 4 vCPU / 8 GB, 20 GB free disk, `kubectl` and `kind` installed. 90–120 minutes for both labs.

---

# Lab L9.4 — GPU Scheduling Semantics on kind (Fake GPUs)

## Objective

Build a fake-GPU Kubernetes node and drive the scheduling behaviors that matter in production. By the end you can:

1. Create a cluster and advertise an extended resource by patching node status.
2. Show that GPU requests are integer, equal to limits, and never overcommitted.
3. Use taints, node labels, quotas, and priorities to control who gets the fake GPUs.
4. Demonstrate gang-scheduling waste: a 4-pod distributed job on a node with fewer GPUs than it needs.
5. Simulate time-slicing by advertising multiple replicas, and explain the isolation trade-off.
6. Design (on paper) the real thing: MIG profiles, device plugin, topology labels, and the checks that verify them.

## Prerequisites

- Linux VM (or any Docker host) with Docker Engine and Compose v2.
- `kind` 0.23+, `kubectl` 1.29+, `jq`.
- 20 GB free disk.
- 60 minutes.

Check:

```bash
docker version --format '{{.Server.Version}}'
kind version
kubectl version --client=true --output=yaml | head -3
jq --version
```

## Setup

```bash
mkdir -p ~/opslab/phase9/l9.4
cd ~/opslab/phase9/l9.4

cat > kind-config.yaml <<'EOF'
kind: Cluster
apiVersion: kind.x-k8s.io/v1alpha4
nodes:
  - role: control-plane
  - role: worker
EOF

kind create cluster --name gpu-sched --config kind-config.yaml
kubectl cluster-info --context kind-gpu-sched
```

Expected:

```text
Kubernetes control plane is running at https://127.0.0.1:port
```

### Advertise fake GPUs

The Kubernetes documentation supports advertising extended resources by patching the node's status directly; the kubelet leaves unknown extended resources alone. This is how you get a GPU-shaped scheduler without a GPU:

```bash
WORKER=$(kubectl get nodes -o json | jq -r '.items[] | select(.metadata.labels["node.kubernetes.io/role"] != "control-plane" and .metadata.labels["kubernetes.io/hostname"] != null) | select(.metadata.name != (kubectl get nodes -l node-role.kubernetes.io/control-plane -o jsonpath="{.items[0].metadata.name}")) | .metadata.name' | head -1)
echo "worker node: $WORKER"

kubectl patch node "$WORKER" --subresource=status --type=merge -p '
{
  "status": {
    "capacity": {
      "example.com/fake-gpu": "8"
    },
    "allocatable": {
      "example.com/fake-gpu": "8"
    }
  }
}'
```

If your `kubectl` version rejects `--subresource=status`, patch without the flag (it patches the status subresource by default for `node/status` paths):

```bash
kubectl patch node "$WORKER" --type=merge -p '{"status":{"capacity":{"example.com/fake-gpu":"8"},"allocatable":{"example.com/fake-gpu":"8"}}}'
```

Verify:

```bash
kubectl get node "$WORKER" -o json | \
  jq '.status.allocatable | with_entries(select(.key|test("example.com")))'
kubectl describe node "$WORKER" | sed -n '/Allocatable/,/System Info/p'
```

Expected:

```text
Allocatable:
  cpu:                63500m
  memory:             ...
  example.com/fake-gpu: 8
```

**Important:** if `kubectl` shows `capacity` but not `allocatable`, the kubelet may not have adopted the value; patch both fields as shown. If a real device plugin were running it would overwrite this; none is, so the value persists until you remove it.

## Part A — The admission rules

### A1. Integer and equal to limits

```bash
cat > pod-frac.yaml <<'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: frac
spec:
  containers:
    - name: c
      image: registry.k8s.io/pause:3.10
      resources:
        limits:
          example.com/fake-gpu: "0.5"
        requests:
          example.com/fake-gpu: "0.5"
EOF
kubectl apply -f pod-frac.yaml
kubectl get pod frac
```

Expected event (from `kubectl describe pod frac`):

```text
Warning  FailedScheduling  ... Invalid resource requests: requests and limits must be equal for example.com/fake-gpu
```

Kubernetes **rejects fractional extended resources**; only integers are valid. Fix the YAML to `1` and it schedules. This is the first admission rule you would enforce for real GPUs too.

### A2. Overcommit is impossible

```bash
# consume all 8
for i in $(seq 1 8); do
  kubectl run busy$i --image=registry.k8s.io/pause:3.10 \
    --overrides='{"spec":{"containers":[{"name":"c","image":"registry.k8s.io/pause:3.10","resources":{"limits":{"example.com/fake-gpu":"1"}}}]}}'
done
kubectl get pods -o wide | grep busy
kubectl describe node "$WORKER" | sed -n '/Allocated resources/,/Events/p'
```

Expected `Allocated resources` line shows `example.com/fake-gpu  8 (100%)  8 (100%)`:

```text
Non-terminated Pods:  (10 in total)
  Namespace  Name        CPU Requests  ...  example.com/fake-gpu
  default    busy0       0 (0%)        ...  1
  ...
Allocated resources:
  (Total limits may be over 100 percent, i.e., overcommitted.)
  Resource           Requests     Limits
  --------           --------     ----
  cpu                ...          ...
  example.com/fake-gpu  8 (100%)  8 (100%)
```

Now try a ninth:

```bash
kubectl run busy9 --image=registry.k8s.io/pause:3.10 \
  --overrides='{"spec":{"containers":[{"name":"c","image":"registry.k8s.io/pause:3.10","resources":{"limits":{"example.com/fake-gpu":"1"}}}]}}'
kubectl describe pod busy9 | grep -A3 Events
```

Expected:

```text
Warning  FailedScheduling  0/2 nodes are available: 1 node(s) had untolerated taint
                             {node-role.kubernetes.io/control-plane: }, 1 Insufficient example.com/fake-gpu.
```

The message is a **filter trace**, not a single cause: it lists every node and why it failed. Clean up before Part B:

```bash
kubectl delete pod busy9
```

## Part B — Taints, labels, and quotas

### B1. Taint the GPU node

```bash
kubectl taint node "$WORKER" dedicated=fake-gpu:NoSchedule
kubectl run no-toleration --image=registry.k8s.io/pause:3.10 --restart=Never
kubectl describe pod no-toleration | grep -A2 Events   # pending, untolerated taint
kubectl delete pod no-toleration
```

A pod that tolerates it schedules:

```bash
kubectl run tolerated --image=registry.k8s.io/pause:3.10 --restart=Never \
  --overrides='{"spec":{"tolerations":[{"key":"dedicated","operator":"Equal","value":"fake-gpu","effect":"NoSchedule"}]}}'
kubectl get pod tolerated -o wide    # lands on the worker
kubectl delete pod tolerated
```

### B2. Quota per namespace

```bash
kubectl create namespace team-a
kubectl create namespace team-b

cat > quota-a.yaml <<'EOF'
apiVersion: v1
kind: ResourceQuota
metadata:
  name: gpu-quota
  namespace: team-a
spec:
  hard:
    requests.example.com/fake-gpu: "2"
    limits.example.com/fake-gpu: "2"
EOF
kubectl apply -f quota-a.yaml

# team-a tries 3 GPUs
kubectl run over --namespace team-a --image=registry.k8s.io/pause:3.10 --restart=Never \
  --overrides='{"spec":{"containers":[{"name":"c","image":"registry.k8s.io/pause:3.10","resources":{"limits":{"example.com/fake-gpu":"3"}}}]}}'
kubectl get pod over -n team-a
```

Expected: the pod is created but **not scheduled**; `kubectl describe pod over -n team-a` shows a quota violation (the API rejects over-quota creation for some resources, or the scheduler reports Insufficient). Observe which and record it. Clean up:

```bash
kubectl delete pod over -n team-a
```

### B3. Priority and preemption (behavioral)

```bash
cat > pc-low.yaml <<'EOF'
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: batch-low
value: 100
globalDefault: false
preemptionPolicy: Never
EOF
kubectl apply -f pc-low.yaml
```

Fill the node with 8 `batch-low` pods, then create a `system-cluster-critical` pod needing 1 GPU and observe that with `preemptionPolicy: Never` it stays Pending. Change to `PreemptLowerPriority` and watch preemption delete a low-priority pod to make room. Capture the `pod deleted due to preemption` event with `kubectl get events --field-selector reason=Preempted` or the victim's `Evicted` state.

**Observation:** preemption is a policy, not a courtesy. For training jobs this means **checkpointing before the cluster schedules anything at a higher priority** — the scheduler will not wait for you.

## Part C — Gang-scheduling waste

This is the failure mode that makes distributed training on default Kubernetes expensive:

```bash
kubectl delete pod busy0 busy1 busy2 busy3 busy4 busy5 busy6 busy7 2>/dev/null || true
```

Node now has 8 free fake GPUs. Create a "4-GPU training job" with two 4-GPU pods and run them one at a time by naming them `rank0`, `rank1` but scheduling rank1 first:

```bash
cat > gang.yaml <<'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: train-rank1
  labels: {app: train}
spec:
  tolerations: [{key: dedicated, operator: Equal, value: fake-gpu, effect: NoSchedule}]
  containers:
    - name: c
      image: registry.k8s.io/pause:3.10
      resources: {limits: {example.com/fake-gpu: "4"}}
---
apiVersion: v1
kind: Pod
metadata:
  name: train-rank0
  labels: {app: train}
spec:
  tolerations: [{key: dedicated, operator: Equal, value: fake-gpu, effect: NoSchedule}]
  containers:
    - name: c
      image: registry.k8s.io/pause:3.10
      resources: {limits: {example.com/fake-gpu: "4"}}
EOF
kubectl apply -f gang.yaml
kubectl get pods -l app=train -o wide
kubectl describe node "$WORKER" | sed -n '/Allocated resources/,/Events/p'
```

Expected: `train-rank1` is Running (4/8 GPUs), `train-rank0` is **Pending** (only 4 free — insufficient). You have burned 4 GPUs for a job that cannot start.

**The point:** default Kubernetes does not gang-schedule. In a real cluster the fix is one of:

- a single pod with `example.com/fake-gpu: 8` (one pod, all GPUs — simplest for small jobs);
- Volcano/Kueue gang scheduling with a PodGroup of 2;
- a scheduler with `MinAvailable`/topology constraints.

Show the single-pod alternative works:

```bash
kubectl delete -f gang.yaml
kubectl run train-single --image=registry.k8s.io/pause:3.10 --restart=Never \
  --overrides='{"spec":{"tolerations":[{"key":"dedicated","operator":"Equal","value":"fake-gpu","effect":"NoSchedule"}],"containers":[{"name":"c","image":"registry.k8s.io/pause:3.10","resources":{"limits":{"example.com/fake-gpu":"8"}}}]}}'
kubectl get pod train-single -o wide
kubectl delete pod train-single
```

## Part D — Simulate time-slicing

Patch the node to advertise 32 fake GPUs (8 physical × 4 replicas):

```bash
kubectl patch node "$WORKER" --type=merge -p '{"status":{"capacity":{"example.com/fake-gpu-sliced":"32"},"allocatable":{"example.com/fake-gpu-sliced":"32"}}}'
```

Create 32 pods requesting 1 slice each; all schedule. Explain the difference in **isolation** between `example.com/fake-gpu: 8` (whole devices) and `example.com/fake-gpu-sliced: 32` (shared): a real time-sliced GPU has no memory isolation, so two tenants can OOM each other and a fault on one affects all. The Kubernetes resource count is a lie about capacity that must be paired with real telemetry (DCGM `FB_USED` per instance) and admission policy.

Clean up the sliced experiment:

```bash
kubectl delete pods -l 'app in (sliced)' --all 2>/dev/null || true
kubectl patch node "$WORKER" --type=merge -p '{"status":{"capacity":{"example.com/fake-gpu-sliced":null},"allocatable":{"example.com/fake-gpu-sliced":null}}}' 2>/dev/null || true
```

## Part E — Paper exercise: the real thing

For each of the following, write the YAML or command and the expected evidence (do not run against a real GPU without permission):

1. A node with 2×A100 running MIG `1g.10gb` × 7: the resource names, the pod request, and `nvidia-smi mig -lgi` output shape.
2. The admission policy that forbids `NVIDIA_VISIBLE_DEVICES` without a GPU request.
3. A topology-aware pool: node labels for `topology.nvidia.com/nvlink=true`, and a pod selecting it for tensor parallel.
4. A Draining procedure for a GPU node: the exact commands, the owner notification, the checkpoint precondition.
5. The equivalent Slurm job: `sbatch` script with `--gres=gpu:a100:2`, `--cpus-per-task=16`, `--time=04:00:00`, and the `squeue`/`sacct` commands to inspect it.
6. The monitoring query for per-instance GPU memory (MIG) and per-pod attribution.

## Expected observations

- Fractional extended resources are rejected at admission.
- Requests and limits must be equal; there is no burst.
- 8 GPUs → 9th pod is Pending with `Insufficient example.com/fake-gpu`.
- Taint blocks non-tolerating pods; a tolerating pod schedules.
- Quota caps a namespace regardless of free cluster capacity.
- `preemptionPolicy: Never` leaves a critical pod Pending even when a lower-priority pod could be evicted.
- Two 4-GPU pods on one node: the first runs, the second pends; 4 GPUs are wasted. This is the gang-scheduling problem, demonstrated.
- Time-sliced advertising lets 32 pods schedule onto 8 devices; capacity is a lie, isolation is the price.

## Internal explanation

### Why patching node status works

The kubelet owns `status.capacity` for standard resources (cpu, memory, pods) and for resources advertised by registered device plugins. **Extended resources that no plugin claims are not overwritten** — the API server accepts a PATCH and the kubelet leaves them. That is why this trick is documented as a legitimate way to test extended-resource scheduling without hardware. It is a testing trick, not a deployment pattern: a real GPU node gets its capacity from a device plugin, which also handles health and allocation.

### Why requests must equal limits

GPU allocation is a fixed device assignment. A "limit higher than request" would mean "sometimes I need two GPUs, sometimes one" — but the device cannot appear and disappear. Kubernetes rejects the asymmetric form to make the allocation an **exclusive reservation**. This is why right-sizing requests is a real operational task: an over-request holds a whole device for nothing, and an under-request means the job fails at CUDA time rather than admission time.

### Why the gang-scheduling problem is worth burning a lab on

On an 8-GPU node, a 4-GPU job that cannot start costs 4 idle GPUs for as long as it waits. Multiply by a cluster and a queue: the default scheduler can waste more GPU-hours than it saves by packing. Gang scheduling (all-or-nothing admission) trades a little latency for a lot of utilization. The same argument drives Slurm's default behavior, which is why HPC clusters never adopted default-Kubernetes-style best-effort scheduling for synchronous jobs.

### Why time-slicing is a capacity lie

Advertising `N × replicas` tells the scheduler there is capacity that does not exist. Every pod that requests 1 slice believes it has a GPU; physically they share. For dev/test this is fine. For multi-tenant production, it removes memory isolation, so an OOM in one tenant can kill another's request. If you must share, prefer MIG (hardware partition, isolated memory) for anything that matters, and time-slicing only where a fault or OOM is acceptable.

## Troubleshooting task

Reproduce these three faults and fix each from evidence:

1. **Capacity vanishes:** patch the node to `8`, create a pod, then patch to `0`. Observe the running pod continues (allocatable is checked at admission, not continuously) but new pods pend. Explain why "allocatable" is an admission-time fact and why monitoring should show actual device usage, not just the advertised count.
2. **Wrong resource name:** request `nvidia.com/gpu: 1` on the fake node. Diagnose from the scheduler event (`Insufficient nvidia.com/gpu`) that the node has no such resource, and explain why the error is a filter trace, not a single cause.
3. **Quota vs capacity confusion:** with `team-a` quota of 2, create two 1-GPU pods in `team-a` and one 1-GPU pod in `default`. Show all three schedule if the node has free GPUs, and explain the difference between a quota (a policy on a namespace) and capacity (a physical fact on a node).

## Verification

- [ ] kind cluster created and torn down cleanly.
- [ ] Node patched to advertise 8 fake GPUs; `allocatable` confirmed via `describe`.
- [ ] Fractional request rejected; equal-request-limit pod scheduled.
- [ ] 9th pod pending with `Insufficient` (captured in the write-up).
- [ ] Taint blocks/permits as expected; tolerated pod lands on the worker.
- [ ] Quota for `team-a` enforced; the exact rejection path recorded.
- [ ] Priority/preemption behavior observed with `Never` vs `PreemptLowerPriority`.
- [ ] Gang-scheduling waste demonstrated with two 4-GPU pods; single-pod alternative shown.
- [ ] Time-slicing simulated; isolation trade-off explained in writing.
- [ ] Paper exercise completed for all 6 items.

## Cleanup

```bash
kind delete cluster --name gpu-sched
docker system df   # confirm space reclaimed
```

## Write-up template

```markdown
## L9.4 — GPU scheduling semantics on kind

### Environment
- kind version, kubectl version, node count

### Fake GPU setup
- patch command used:
- allocatable after patch: ...

### Admission rules
| test | expected | observed |
|---|---|---|
| fractional request | rejected | |
| requests != limits | rejected | |
| 9th pod | pending | |
| taint no-toleration | pending | |
| quota over | rejected/pending | |
| preemption | ... | |

### Gang-scheduling waste
- two 4-GPU pods: ... (what ran, what pended, GPU-hours wasted)

### Time-slicing
- 32 slices scheduled; isolation consequence: ...

### Paper exercise answers
1. MIG resource names and pod request: ...
2. admission policy: ...
3. topology labels: ...
4. drain procedure: ...
5. slurm job: ...
6. monitoring query: ...

### What I would automate
...
```

---

# Lab L9.5 — MLOps Loop, Serving Metrics, and the Cost Model

## Objective

Close the loop: register a trained model, serve it behind an API with real resource accounting, load-test it, expose metrics, and compute cost per 1,000 requests and per 1M tokens. By the end you can:

1. Register a model version in MLflow with an alias and a rollback target.
2. Serve it behind FastAPI with a canary traffic split.
3. Expose Prometheus metrics for latency, concurrency, and token counts.
4. Load test and identify the queueing point.
5. Compute cost per 1,000 requests and per 1M tokens from measured CPU/GPU time.
6. Detect a drift signal from prediction logs and decide the next action.

This lab continues from L9.2's `tiny-classifier` (you can also retrain it quickly). No GPU, no cloud.

## Prerequisites

- L9.2 completed, or a retrain of `app/train.py` (10 minutes).
- Linux VM or macOS, 4 vCPU / 8 GB, the Phase 9 venv.
- `curl`, `jq`, `python3`.
- 90 minutes.

Check:

```bash
source ~/opslab/phase9/venv/bin/activate
python3 -c "import mlflow, fastapi, httpx; print('deps ok')"
```

## Setup

```bash
cd ~/opslab/phase9/l9.2   # reuse the trained artifact
mkdir -p l9.5-artifacts   # for this lab's outputs
```

## Part A — Register and alias

```bash
mlflow server --backend-store-uri sqlite:///mlflow.db \
  --default-artifact-root file://$PWD/artifacts --host 127.0.0.1 --port 5000 &
sleep 4
```

Register the trained model (assumes the L9.2 run; replace `<run_id>` with the printed ID):

```bash
RUN_ID=$(mlflow runs list --experiment-id 1 --output json 2>/dev/null | jq -r '.[0].info.run_id' 2>/dev/null || echo "<paste-your-run-id>")
python3 - <<'PY'
import mlflow
mlflow.set_tracking_uri("http://127.0.0.1:5000")
mv = mlflow.register_model("runs:/<RUN_ID>/model", "tiny-classifier")
print("registered version", mv.version)
PY
```

Point the alias at version 1:

```bash
mlflow models update-model-version --name tiny-classifier --version 1 --alias champion
```

Verify:

```bash
mlflow models describe --name tiny-classifier --version 1 | head -20
```

Expected:

```text
name: tiny-classifier
version: 1
aliases: [champion]
source: runs:/<run_id>/model
run_id: <run_id>
```

**Observation:** the alias, not the version, is what deployments reference. Rollback means moving the alias.

## Part B — Serve with a canary split

Create `app/serve_canary.py`:

```python
#!/usr/bin/env python3
"""Serve two model versions with a weighted split and per-version metrics."""
import hashlib, os, random, time
from collections import Counter, defaultdict
from fastapi import FastAPI, Request
from pydantic import BaseModel
import torch

app = FastAPI()
ART = os.environ.get("ART_DIR", "artifacts")

def load(path):
    actual = hashlib.sha256(open(path, "rb").read()).hexdigest()
    expected = open(path.replace("model.pt", "hash.txt")).read().strip()
    if actual != expected:
        raise RuntimeError(f"hash mismatch: {actual}")
    m = torch.jit.load(path); m.eval(); return m

V1 = load(f"{ART}/model.pt")     # champion
# In a real canary V2 is a different file; for the lab, same weights, different tag.
V2 = V1

class Input(BaseModel):
    features: list[float]

SPLIT_V2 = float(os.environ.get("SPLIT_V2", "0.1"))

@app.post("/predict")
def predict(inp: Input):
    t0 = time.perf_counter()
    version = "v2" if random.random() < SPLIT_V2 else "v1"
    model = V2 if version == "v2" else V1
    try:
        if len(inp.features) != 16:
            raise ValueError("need 16 features")
        x = torch.tensor([inp.features], dtype=torch.float32)
        with torch.no_grad():
            prob = torch.softmax(model(x), dim=1)[0, 1].item()
        _stats[version]["ok"] += 1
        _stats[version]["lat"].append(time.perf_counter() - t0)
        return {"prob": round(prob, 6), "version": version}
    except Exception as e:
        _stats[version]["err"] += 1
        return {"error": str(e)}

_stats = {v: {"ok": 0, "err": 0, "lat": []} for v in ("v1", "v2")}

@app.get("/metrics")
def metrics():
    out = {}
    for v, s in _stats.items():
        lat = sorted(s["lat"])
        n = len(lat)
        out[v] = {
            "ok": s["ok"], "err": s["err"],
            "p50": round(lat[min(int(0.5*n), n-1)]*1000, 2) if n else 0,
            "p95": round(lat[min(int(0.95*n), n-1)]*1000, 2) if n else 0,
            "p99": round(lat[min(int(0.99*n), n-1)]*1000, 2) if n else 0,
        }
    return out
```

Run:

```bash
uvicorn app.serve_canary:app --host 127.0.0.1 --port 8100 &
sleep 3
for i in $(seq 1 300); do
  curl -s http://127.0.0.1:8100/predict -H 'Content-Type: application/json' \
    -d "{\"features\": [$(python3 -c 'print(",".join(["0.1"]*16))')]}" > /dev/null
done
curl -s http://127.0.0.1:8100/metrics | jq
```

Expected (numbers vary):

```json
{
  "v1": {"ok": 271, "err": 0, "p50": 1.8, "p95": 3.1, "p99": 4.6},
  "v2": {"ok": 29, "err": 0, "p50": 1.8, "p95": 3.0, "p99": 4.4}
}
```

The split is visible: ~90/10. Per-version metrics let you compare the canary directly.

## Part C — Load test and find the queueing point

```bash
# 200 concurrent requests, 20 at a time
cat > loadtest.py <<'PY'
import concurrent.futures as cf, time, statistics, json, httpx

URL = "http://127.0.0.1:8100/predict"
PAYLOAD = {"features": [0.1] * 16}

def one(_):
    t0 = time.perf_counter()
    r = httpx.post(URL, json=PAYLOAD, timeout=30)
    return time.perf_counter() - t0, r.status_code

for conc in (1, 4, 8, 16, 32, 64):
    with cf.ThreadPoolExecutor(conc) as ex:
        results = list(ex.map(one, range(200)))
    lat = sorted(r[0] for r in results)
    errs = sum(1 for r in results if r[1] != 200)
    n = len(lat)
    print(f"conc={conc:2d} p50={lat[n//2]*1000:6.1f}ms "
          f"p95={lat[int(n*0.95)]*1000:6.1f}ms "
          f"p99={lat[int(n*0.99)]*1000:6.1f}ms errs={errs}")
PY
python3 loadtest.py
```

Expected shape (4 vCPU, single uvicorn worker):

```text
conc= 1 p50=   2.1ms p95=   3.4ms p99=   4.9ms errs=0
conc= 4 p50=   2.3ms p95=   3.8ms p99=   5.2ms errs=0
conc= 8 p50=   3.9ms p95=   6.8ms p99=   9.1ms errs=0
conc=16 p50=   8.2ms p95=  14.5ms p99=  19.8ms errs=0
conc=32 p50=  18.1ms p95=  31.7ms p99=  42.6ms errs=0
conc=64 p50=  41.5ms p95=  72.4ms p99=  95.2ms errs=0
```

**Interpretation:** latency grows roughly linearly with concurrency once the single worker saturates — this is queueing, not model slowdown. `p99 ≈ p50 × 2.3` at high concurrency is the signature of a single queue with FIFO behavior. In a real deployment, the fix is concurrency limits, more workers/replicas, and an SLO-based autoscaler.

## Part D — Cost per 1,000 requests and per 1M tokens

### D1. CPU cost, measured

Measure CPU time consumed by the server during a known request count:

```bash
PID=$(pgrep -f 'uvicorn app.serve_canary' | head -1)
# run 500 requests
for i in $(seq 1 500); do
  curl -s -o /dev/null http://127.0.0.1:8100/predict -H 'Content-Type: application/json' \
    -d "{\"features\": [$(python3 -c 'print(",".join(["0.1"]*16))')]}"
done
# read cumulative CPU time from /proc
awk '/^Utime:/ {print $2}' /proc/$PID/stat
```

Or use `time` around the whole loop:

```bash
/usr/bin/time -p sh -c 'for i in $(seq 1 500); do curl -s -o /dev/null http://127.0.0.1:8100/predict -H "Content-Type: application/json" -d "{\"features\": [1.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0]}"; done'
```

Expected shape:

```text
real 4.82
user 3.95
sys  0.42
```

**Arithmetic:**

```text
CPU seconds per request = (user + sys) / 500
cost per 1000 requests = CPU_sec_per_req × 1000 × vCPU_hour_rate / 3600
```

With $0.0445/vCPU-hour and, say, 8.74 ms CPU per request:

```text
8.74 ms × 1000 = 8.74 s per 1000 requests
8.74 × 0.0445 / 3600 = $0.000108 per 1000 requests (CPU only)
```

### D2. GPU cost, estimated

For a hypothetical A100 serving a 7B model at 1,000 output tokens per request and 30 requests/s per GPU (measured or vendor-published):

```text
GPU throughput = 30 req/s × 1000 tok = 30,000 tok/s
cost per 1M output tokens = ($3.20/h ÷ 3600) ÷ 30,000 × 1e6 = $0.0296
```

If the GPU utilization is 40% (real workload, not a benchmark):

```text
effective cost per 1M tokens = 0.0296 / 0.40 = $0.074
```

**Observation:** utilization multiplies cost. A benchmark number without the utilization factor is marketing, not a cost model. In your write-up, state the measured CPU number, the assumed GPU throughput, and the utilization assumption separately.

## Part E — Drift from prediction logs

Add a log of predictions and probabilities (reuse the canary server), then simulate drift:

```bash
cat > drift_probe.py <<'PY'
import numpy as np, httpx, json

URL = "http://127.0.0.1:8100/predict"
rng = np.random.default_rng(7)

# reference distribution: standard normal
def sample(scale=1.0):
    return [float(x) * scale for x in rng.normal(size=16)]

def collect(scale, n=500):
    ps = []
    for _ in range(n):
        r = httpx.post(URL, json={"features": sample(scale)}, timeout=30).json()
        ps.append(r.get("prob", -1))
    return np.array(ps)

ref = collect(1.0)
drift = collect(2.5)   # 2.5× the feature scale: the input distribution moved

def stats(a, name):
    print(f"{name}: mean={a.mean():.3f} p50={np.percentile(a,50):.3f} "
          f"p95={np.percentile(a,95):.3f} frac>0.9={(a>0.9).mean():.3f}")

stats(ref, "reference")
stats(drift, "drifted  ")
PY
python3 drift_probe.py
```

Expected shape:

```text
reference: mean=0.487 p50=0.492 p95=0.881 frac>0.9=0.062
drifted  : mean=0.713 p50=0.768 p95=0.998 frac>0.9=0.541
```

**Interpretation:** the prediction distribution moved (mean 0.49 → 0.71, frac>0.9 jumps from 6% to 54%). This is **prediction drift**, an early warning that the input distribution changed. It does not prove the model is wrong; it proves the world changed. The next step is to compare against a labeled quality metric if labels exist, and otherwise to investigate the feature pipeline.

## Expected observations

- The registry has version 1 with the `champion` alias; the alias, not the version, is referenced.
- The canary split is visible per version in `/metrics`, with independent p50/p95/p99.
- Load test shows near-linear latency growth beyond ~8–16 concurrent requests on a single worker.
- CPU cost per 1,000 requests is measured, not assumed; the GPU comparison states the throughput and utilization assumptions separately.
- The drift probe shows a distribution shift in predictions; the response (retrain vs investigate pipeline) is a decision, not an automatic action.

## Internal explanation

### Why the canary split is the honest test of a new model

A/B without a split is a hope. A split with per-version metrics gives you the comparison at the same moment under the same traffic. If v2's p95 is fine but its error rate is higher, you catch it in the split before the rollout. If v2's p95 is higher but its errors are lower, you decide with numbers whether the trade is acceptable. The lab's split is random per request; production canaries are usually sticky per user/session to avoid inconsistent experiences.

### Why cost must state its unit

"Cost per GPU-hour" is a purchase. "Cost per request" is a service. The two diverge by a factor equal to utilization. A dashboard that reports only GPU-hours rewards hoarding (holding GPUs to avoid contention) and punishes efficiency. Reporting both, and the ratio between them, is what makes capacity decisions real. In your write-up, always name the unit, the window, and the utilization assumption.

### Why drift detection is a triage signal, not a verdict

Drift says "the input distribution moved." It does not say "the model is wrong" — the model may handle the shift fine, and the detector may be too sensitive. A drift alert that pages a human every day is worse than none: the human stops reading it. Treat drift as an investigation trigger with a defined playbook: inspect feature contributions, compare against labeled quality if available, and check for a data pipeline change before assuming a model problem.

## Troubleshooting task

1. **Registry unreachable:** stop the MLflow server, run `mlflow models update-model-version`. Diagnose from the connection refused error; restart the server; explain why artifact URI and registry URI are separate configuration.
2. **Canary skew:** set `SPLIT_V2=0.9` and observe the metrics; then set it to 0.5 with `slow` injected only for v2. Explain how a shared slow path can make the canary look guilty and why per-version latency attribution matters.
3. **Hash mismatch in canary:** copy `artifacts/model.pt` to `artifacts/model_v2.pt` and modify one byte, then set `V2 = load(f"{ART}/model_v2.pt")` — the server refuses to start. Diagnose from the printed hash; restore from MLflow artifacts.

## Verification

- [ ] Model registered as version 1 with `champion` alias; rollback target defined.
- [ ] Canary serving with per-version metrics; the split ratio matches the configured SPLIT_V2.
- [ ] Load test table (6 concurrency levels) recorded and the queueing point identified.
- [ ] CPU cost per 1,000 requests computed from measured CPU time; GPU cost per 1M tokens computed with stated assumptions.
- [ ] Drift probe run; the distribution shift observed and interpreted as prediction drift.
- [ ] All three troubleshooting faults reproduced, diagnosed, fixed.

## Cleanup

```bash
pkill -f 'uvicorn app.serve_canary' || true
pkill -f 'mlflow server' || true
```

## Write-up template

```markdown
## L9.5 — MLOps, serving, cost

### Registry
- model: tiny-classifier, version 1, alias champion, run_id: ...

### Canary serving
| version | ok | err | p50 | p95 | p99 |
|---|---|---|---|---|---|

### Load test
| conc | p50 | p95 | p99 | errs |
|---|---|---|---|---|
| 1 | | | | |
| ... | | | | |

Queueing point: ...

### Cost
| unit | value | assumptions |
|---|---|---|
| CPU cost per 1000 requests | | |
| GPU cost per 1M tokens | | |

### Drift probe
- reference: mean=..., frac>0.9=...
- drifted: mean=..., frac>0.9=...
- interpretation: ...

### Faults reproduced
1. registry unreachable →
2. canary skew →
3. hash mismatch →

### What I would add for production
...
```
