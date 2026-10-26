# Phase 9 Capstone — A Tiny Multi-Tenant AI Platform

> **The one-sentence brief:** design and partially build a small AI platform that serves two teams on a shared compute pool, with isolation, observability, and a cost model — then prove the design by breaking it.

This capstone is the Phase 9 exit exercise. It uses everything in the phase: GPU/accelerator economics (note 01), the training and serving loop (note 02), collectives and topology (note 03), scheduling and isolation (note 04), and the MLOps/cost loop (note 05). It is deliberately **tiny and partially real**: the multi-tenant GPU platform you would run at a company is out of scope for a laptop, but the design decisions and the failure drills are not.

You have two options:

| Option | Hardware | What you build | What you design on paper |
|---|---|---|---|
| **A (implemented)** | kind cluster, fake GPUs, CPU training | Scheduling, quotas, serving, monitoring, failure drills | The real-GPU translation |
| **B (design-only)** | none | Nothing runs | Everything, with the same rubric applied to the written design |

Option A is strongly preferred; you cannot defend a design you have never operated. Option B exists for when you have no Docker or Linux VM available this week.

---

## 1. The scenario

**Two teams share one 8-GPU node (Option A: one kind node advertising 8 fake GPUs).**

- **Team Training** runs a synchronous data-parallel training job. It wants all 8 GPUs, several hours at a time, and checkpoints to shared storage.
- **Team Inference** runs a model server that serves a few requests per second. It needs 1 GPU, 24/7, and p95 latency under a stated budget.
- The platform (you) owns scheduling, isolation, observability, and cost reporting. Both teams are charged for what they hold.

**Constraints:**

1. Total GPUs: 8. No autoscaling to more.
2. Training cannot run while inference is using the GPU it wants (they are different teams; their priorities differ).
3. A training job that dies must resume from its last checkpoint, not restart from zero.
4. Cost per 1,000 inference requests must be reported weekly, from telemetry, not guessed.
5. A GPU fault (XID-style event) must drain the node and page someone — in this lab, "page" is a log line and an alert, not a real channel.

---

## 2. Architecture (Option A)

```text
┌───────────────────────────────────────────────────────────────────┐
│ kind node "gpu-node"                                              │
│  fake GPUs: example.com/fake-gpu = 8  (patched node status)       │
│  taint: dedicated=fake-gpu:NoSchedule                             │
├───────────────────────────────────────────────────────────────────┤
│ namespaces                                                        │
│  team-training        team-inference        platform              │
│  ────────────         ──────────────        ─────────             │
│  train-job            infer-api             prometheus            │
│  (8 GPU request,      (1 GPU request,       grafana (optional)    │
│   checkpoint PVC,     deployment,           alert-receiver        │
│   priority high,      readiness,            (local webhook)       │
│   gang by design:     metrics,              cost-exporter         │
│   single pod)         HPA on queue)         (simple script)       │
├───────────────────────────────────────────────────────────────────┤
│ admission: labels required, no GPU without request,               │
│            no privileged pods, no hostPath to /dev/*              │
├───────────────────────────────────────────────────────────────────┤
│ monitoring: node GPU alloc (fake), pod CPU/mem, queue depth,      │
│             alerts on: GPU unavailable, training pending >10m,    │
│             inference p95 breach, quota near full                 │
└───────────────────────────────────────────────────────────────────┘
```

### Why these choices

| Decision | Choice | Alternative rejected | Why |
|---|---|---|---|
| Training shape | Single pod with all 8 GPUs | 8 pods × 1 GPU | Default Kubernetes is not gang-scheduled; a single pod cannot be split by the scheduler. For a lab, one pod is the honest approximation. |
| Inference shape | Deployment with 1 replica × 1 GPU | DaemonSet | It needs autoscaling on load, not per-node presence. |
| Isolation | Namespace + ResourceQuota + taint | Pod Security Admission only | Quota is the budget enforcement; taint keeps non-GPU work off the node. |
| Priorities | Training `PreemptLowerPriority`, inference lower | Both same priority | Training jobs are expensive to restart; preemption is deliberate and documented. |
| Checkpointing | PVC + a script writing every N steps | Node-local disk | Node-local dies with the node. |
| Monitoring | Prometheus with fake GPU metric + pod metrics | `kubectl top` only | Alerts must be declarative and testable. |
| Cost | Script that reads allocation and emits cost series | Manual spreadsheet | Cost must be reproducible and comparable over time. |

---

## 3. Deliverables

| # | Deliverable | Evidence |
|---|---|---|
| D1 | Kind cluster with 8 fake GPUs advertised, GPU node tainted | `kubectl describe node` output in the repo |
| D2 | Two namespaces with quotas; training/inference manifests | YAML files |
| D3 | Training job that writes checkpoints to a PVC and resumes | Log excerpt showing resume from checkpoint |
| D4 | Inference deployment with readiness/liveness, metrics, HPA | YAML + `curl /metrics` output |
| D5 | Prometheus scraping both, with 4 alerts defined and tested | Alert rules + firing/resolution screenshots or logs |
| D6 | Cost exporter producing `platform_gpu_allocated_hours` and `platform_gpu_used_hours` by namespace | Prometheus series + a weekly "cost per 1k requests" table |
| D7 | Failure drills: (a) GPU fault drains the node, (b) inference overload, (c) training preempts inference, (d) quota exhaustion | Drill transcripts |
| D8 | A one-page design document translating every fake-GPU decision to a real 8×A100 node (MIG, device plugin, DCGM, topology) | Design doc |
| D9 | A runbook for the on-call engineer covering the four drills | Runbook file |
| D10 | A written retrospective: what broke, what you would change, what you would automate | Retrospective file |

**Time estimate:** 6–10 hours total. If you take longer, cut scope (D6–D7 can be simplified), not quality.

---

## 4. Step-by-step (Option A)

### Step 1 — Cluster and fake GPUs

```bash
kind create cluster --name ai-platform --config kind-config.yaml
WORKER=$(kubectl get nodes | awk '/^ai-platform-worker/{print $1}')
kubectl patch node "$WORKER" --type=merge -p '{"status":{"capacity":{"example.com/fake-gpu":"8"},"allocatable":{"example.com/fake-gpu":"8"}}}'
kubectl taint node "$WORKER" dedicated=fake-gpu:NoSchedule
```

Verify (all three checks; a missing allocatable is a silent failure):

```bash
kubectl get node "$WORKER" -o json | jq '.status.allocatable["example.com/fake-gpu"]'
# expect "8"
```

### Step 2 — Namespaces, quotas, priorities

```bash
kubectl create ns team-training
kubectl create ns team-inference
kubectl create ns platform

# quotas: training may hold 8 GPUs but only in one pod; inference may hold 1
kubectl apply -f - <<'EOF'
apiVersion: v1
kind: ResourceQuota
metadata: {name: q, namespace: team-training}
spec:
  hard:
    requests.example.com/fake-gpu: "8"
    limits.example.com/fake-gpu: "8"
    pods: "2"
---
apiVersion: v1
kind: ResourceQuota
metadata: {name: q, namespace: team-inference}
spec:
  hard:
    requests.example.com/fake-gpu: "2"
    limits.example.com/fake-gpu: "2"
    pods: "5"
EOF
```

Priorities:

```bash
kubectl apply -f - <<'EOF'
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata: {name: training-high}
value: 10000
globalDefault: false
preemptionPolicy: PreemptLowerPriority
---
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata: {name: serving-medium}
value: 1000
globalDefault: false
preemptionPolicy: Never
EOF
```

**Decision to defend:** training preempts inference; inference never preempts training. Write the reasoning in the design doc (checkpointed training loses minutes; inference losing its only GPU loses availability).

### Step 3 — Training job

Create a `PVC` and a training pod that:

- requests all 8 fake GPUs;
- mounts the PVC;
- writes a checkpoint file every 30 seconds (a stub: `checkpoint.json` with a step count);
- on startup, reads the latest checkpoint and resumes from it;
- runs for a fixed wall time (5 minutes), so it can be killed and resumed.

```bash
kubectl apply -f - <<'EOF'
apiVersion: v1
kind: PersistentVolumeClaim
metadata: {name: checkpoints, namespace: team-training}
spec:
  accessModes: [ReadWriteOnce]
  resources: {requests: {storage: 1Gi}}
---
apiVersion: v1
kind: Pod
metadata: {name: train, namespace: team-training, labels: {app: train}}
spec:
  priorityClassName: training-high
  tolerations: [{key: dedicated, operator: Equal, value: fake-gpu, effect: NoSchedule}]
  containers:
    - name: trainer
      image: python:3.11-slim
      command: ["/bin/bash", "-c"]
      args:
        - |
          set -e
          pip install --quiet numpy
          python /train/train.py
      volumeMounts: [{name: ckpt, mountPath: /checkpoints}]
  volumes:
    - name: ckpt
      persistentVolumeClaim: {claimName: checkpoints}
  restartPolicy: Never
EOF
```

`/train/train.py` (ConfigMap or a small image; write it):

```python
import json, os, time
p = "/checkpoints/state.json"
step = 0
if os.path.exists(p):
    step = json.load(open(p))["step"]
    print(f"resumed from step {step}", flush=True)
end = time.time() + 300
while time.time() < end:
    step += 1
    if step % 10 == 0:
        json.dump({"step": step}, open(p, "w"))
        print(f"checkpoint step={step}", flush=True)
    time.sleep(1)
print("done", flush=True)
```

Evidence to capture:

- first run: checkpoints every 30 s;
- delete the pod mid-run, recreate it: the log must show `resumed from step N` where N is the last written checkpoint, not 0.

### Step 4 — Inference deployment

```bash
kubectl apply -f - <<'EOF'
apiVersion: apps/v1
kind: Deployment
metadata: {name: infer, namespace: team-inference, labels: {app: infer}}
spec:
  replicas: 1
  selector: {matchLabels: {app: infer}}
  template:
    metadata: {labels: {app: infer, version: v1}}
    spec:
      priorityClassName: serving-medium
      tolerations: [{key: dedicated, operator: Equal, value: fake-gpu, effect: NoSchedule}]
      containers:
        - name: api
          image: python:3.11-slim
          command: ["/bin/bash","-c"]
          args: ["pip install --quiet fastapi uvicorn numpy && python /srv/app.py"]
          ports: [{containerPort: 8000}]
          resources:
            requests: {cpu: "250m", memory: "256Mi", example.com/fake-gpu: "1"}
            limits:   {cpu: "500m", memory: "512Mi", example.com/fake-gpu: "1"}
          readinessProbe: {httpGet: {path: /health, port: 8000}, initialDelaySeconds: 2}
          livenessProbe:  {httpGet: {path: /health, port: 8000}, periodSeconds: 10}
---
apiVersion: v1
kind: Service
metadata: {name: infer, namespace: team-inference}
spec:
  selector: {app: infer}
  ports: [{port: 80, targetPort: 8000}]
EOF
```

Expose with a port-forward and confirm:

```bash
kubectl port-forward -n team-inference svc/infer 8200:80 &
curl -s http://127.0.0.1:8200/health | jq
curl -s http://127.0.0.1:8200/predict -H 'Content-Type: application/json' \
  -d '{"features":[0.1,0.2]}' | jq
```

### Step 5 — Monitoring and alerts

Install a small Prometheus (kube-prometheus-stack is heavy; a plain Prometheus pod with a config ConfigMap is enough for this capstone). Scrape:

- kubelet/cAdvisor for pod CPU/memory;
- a small exporter for the fake GPU count (`platform_fake_gpu_allocatable`, `platform_fake_gpu_used`);
- the inference API's own `/metrics`.

Four alerts (test each; do not just write them):

```yaml
groups:
  - name: ai-platform
    rules:
      - alert: GpuUnavailable
        expr: platform_fake_gpu_allocatable == 0 or absent(platform_fake_gpu_allocatable)
        for: 2m
      - alert: TrainingPending
        expr: kube_pod_status_phase{phase="Pending",namespace="team-training"} == 1
        for: 10m
      - alert: InferenceP95Breach
        expr: histogram_quantile(0.95, rate(infer_request_duration_seconds_bucket[5m])) > 0.3
        for: 5m
      - alert: QuotaNearFull
        expr: kube_resourcequota{type="used",resource="requests.example.com/fake-gpu"} / kube_resourcequota{type="hard",resource="requests.example.com/fake-gpu"} > 0.9
        for: 5m
```

### Step 6 — Cost exporter

A 20-line script (cron or a small Deployment) that:

1. reads `kubectl describe node` allocatable and each pod's GPU request;
2. emits two series: `platform_gpu_allocated_hours{namespace=...}` and `platform_gpu_used_hours{namespace=...}`;
3. publishes a weekly table: for each namespace, allocated GPU-hours, utilized GPU-hours, cost at a stated price, and cost per 1,000 inference requests.

You do not need a real exporter; a script that prints the table and writes a Prometheus text file is enough evidence.

### Step 7 — Failure drills (the heart of the capstone)

**Drill 1 — GPU fault drains the node.**
Simulate by patching allocatable to 0 and watching `GpuUnavailable` fire. The runbook step should say: cordon, notify owners, preserve logs, diagnose, return only after a clean health check.

**Drill 2 — Inference overload.**
Load the API past its concurrency limit. Watch `InferenceP95Breach`. The fix in the lab is to cap concurrency and shed load, not to add GPUs (there are none free while training runs).

**Drill 3 — Training preempts inference.**
With inference running (1 GPU) and training wanting 8, training cannot fit unless a GPU frees up. Delete inference's pod manually to simulate preemption, restart training, and record the availability impact and the recovery time. The design question: should inference have two replicas so it survives one GPU being taken? Decide, then document.

**Drill 4 — Quota exhaustion.**
Create enough inference pods to hit the namespace quota, then try to create one more. Record the error text and the user-visible consequence.

### Step 8 — Design translation (D8)

For each fake-GPU decision, write the real-GPU equivalent:

| Fake (lab) | Real (8×A100 node) | Why it changes |
|---|---|---|
| `example.com/fake-gpu` | `nvidia.com/gpu` via device plugin | A real plugin handles health, MIG, allocation |
| 8 units patched | 8 real devices discovered | Driver + toolkit required |
| No MIG | MIG profiles for inference | Isolation between tenants on shared GPU |
| No DCGM | DCGM metrics for GPU health | XID/ECC/thermal are real signals |
| Single-pod gang | Single-pod or Volcano gang | Topology and multi-node change the answer |
| PVC on local PV | PVC on EFS/NFS/Ceph | Checkpoint must survive node loss |
| CPU inference | GPU inference with batching | Latency and cost change completely |
| Fake GPU fault | DCGM XID alert → drain | Real faults are nondeterministic |
| Prometheus scrape | Same + DCGM + RDMA counters | The metric set grows |

---

## 5. Rubric (160 points)

| Area | Points | What full credit looks like |
|---|---:|---|
| D1–D2 setup correctness | 20 | All three checks pass; quotas and priorities are justified in writing |
| D3 checkpoint/resume | 20 | Resume demonstrated; the log shows the resumed step |
| D4 inference service | 15 | Readiness/liveness, metrics, real requests served |
| D5 monitoring and alerts | 25 | Each of the 4 alerts fires in a drill and resolves; rule health shown |
| D6 cost model | 20 | Allocated vs utilized series; a weekly table with a stated unit and assumptions |
| D7 failure drills | 30 | All four drills run, transcripts captured, each has a decision and a runbook step |
| D8 design translation | 20 | Every fake decision has a real equivalent with the reason it changes |
| D9 runbook | 10 | Covers all four drills, has owners, and is readable by someone who did not build it |
| D10 retrospective | 10 | Specific, not generic; names at least three things you would automate |

**Pass threshold:** 112/160 (70%), with **at least 21/30 on the drills**. The drills are the part you cannot fake from a textbook; they are the interview stories.

### Common failure patterns

| Pattern | Points lost | Fix |
|---|---|---|
| Alerts defined but never fired | 15–25 | Fire each one in a drill; screenshots or logs |
| Checkpoint not actually resumed | 20 | Kill and rerun; show the log line |
| Cost model with no utilization factor | 10 | Report allocated AND utilized, with the ratio |
| Design doc copies the lab unchanged | 10–20 | Each row must state why the real thing differs |
| Runbook that only the author can read | 5–10 | Write it for a stranger; include the exact commands |
| Retrospective that says "learned a lot" | 5–10 | Name specific failures and specific automations |

---

## 6. Oral defense (15 minutes)

Be ready to answer, without notes:

1. Why is your training job one pod with 8 GPUs instead of 8 pods?
2. When would you reverse that choice?
3. Training preempts inference. Defend or attack that policy.
4. Where does your cost model overestimate, and by how much?
5. Which alert would fire first during a real incident, and why?
6. What breaks first when traffic triples?
7. What does your fake-GPU platform **not** model about a real GPU?
8. If the node had 4 GPUs instead of 8, what would change about the gang-scheduling problem?
9. Which single control, if removed, causes the most damage?
10. Your checkpoint write failed silently. How would you detect it in production?

---

## 7. Cleanup

```bash
kind delete cluster --name ai-platform
rm -rf ~/opslab/phase9/l9.4 ~/opslab/phase9/l9.5-artifacts
```

Keep the manifests, the runbook, the design doc, and the drill transcripts in your repo — they are interview evidence, not just lab artifacts.
