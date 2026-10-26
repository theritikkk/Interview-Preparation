# Phase 4 Labs — Part 2 (L4.5–L4.7)

> **Safety:** L4.5–L4.7 create PVCs, quotas, taints, and intentionally broken workloads. Use the disposable kind cluster. Do not drain or taint a real cluster. Delete the cluster when finished.

---

# Lab L4.5 — Storage, Scheduling, Resources, and RBAC

## Objective

Provision and consume a PVC, persist data across Pod replacement, run a StatefulSet with per-replica storage, force and fix scheduling failures, observe an OOM kill, and prove least-privilege RBAC with `kubectl auth can-i`.

## Prerequisites

- L4.3/L4.4 kind cluster (`opslab`) running.
- `kubectl`, 5 GB free disk in the kind node.
- 40–50 minutes.

## Setup

```bash
kubectl get storageclass
kubectl get nodes --show-labels
```

Expected (kind):

```text
NAME                 PROVISIONER             RECLAIMPOLICY   VOLUMEBINDINGMODE      ALLOWVOLUMEEXPANSION
standard (default)   rancher.io/local-path   Delete          WaitForFirstConsumer   false
```

`standard` uses the local-path provisioner, which creates a directory on the node. It is fine for a lab; it is not replicated storage.

Create a namespace:

```bash
kubectl create namespace storage-lab
```

## Part A — PVC and persistence

```bash
kubectl -n storage-lab apply -f - <<'EOF'
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: data
spec:
  accessModes: ["ReadWriteOnce"]
  storageClassName: standard
  resources:
    requests:
      storage: 1Gi
---
apiVersion: v1
kind: Pod
metadata:
  name: writer
spec:
  containers:
    - name: app
      image: busybox:1.36
      command: ["sh", "-c", "sleep 3600"]
      volumeMounts:
        - name: data
          mountPath: /data
  volumes:
    - name: data
      persistentVolumeClaim:
        claimName: data
EOF
kubectl -n storage-lab get pvc,pod
kubectl -n storage-lab describe pvc data
```

Because the StorageClass uses `WaitForFirstConsumer`, the PVC may stay `Pending` until the Pod is scheduled. That is expected. Once the Pod is Running:

```bash
kubectl -n storage-lab exec writer -- sh -c 'echo persisted > /data/file.txt; cat /data/file.txt'
kubectl -n storage-lab delete pod writer
kubectl -n storage-lab apply -f - <<'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: reader
spec:
  containers:
    - name: app
      image: busybox:1.36
      command: ["sh", "-c", "cat /data/file.txt; sleep 3600"]
      volumeMounts:
        - name: data
          mountPath: /data
  volumes:
    - name: data
      persistentVolumeClaim:
        claimName: data
EOF
kubectl -n storage-lab logs reader
```

Expected: `persisted`. The PVC/PV outlived the Pod.

Now delete the PVC and prove the data is gone (the reclaim policy is `Delete`):

```bash
kubectl -n storage-lab delete pod reader
kubectl -n storage-lab delete pvc data
kubectl -n storage-lab get pv
```

The PV should be deleted by the provisioner. If you had used `Retain`, the PV would remain `Released` and you would have to clean it up manually.

## Part B — StatefulSet storage

```bash
kubectl -n storage-lab apply -f - <<'EOF'
apiVersion: v1
kind: Service
metadata:
  name: db
spec:
  clusterIP: None
  selector:
    app: db
  ports:
    - port: 5432
      targetPort: 5432
---
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: db
spec:
  serviceName: db
  replicas: 2
  selector:
    matchLabels:
      app: db
  template:
    metadata:
      labels:
        app: db
    spec:
      containers:
        - name: db
          image: busybox:1.36
          command: ["sh", "-c", "echo $(hostname) > /data/id; sleep 3600"]
          volumeMounts:
            - name: data
              mountPath: /data
  volumeClaimTemplates:
    - metadata:
        name: data
      spec:
        accessModes: ["ReadWriteOnce"]
        storageClassName: standard
        resources:
          requests:
            storage: 1Gi
EOF
kubectl -n storage-lab get pods,pvc
kubectl -n storage-lab exec db-0 -- cat /data/id
kubectl -n storage-lab exec db-1 -- cat /data/id
kubectl -n storage-lab run dns --rm -it --restart=Never --image=busybox:1.36 -- \
  nslookup db-0.db.storage-lab.svc.cluster.local
```

Expected:

```text
db-0
db-1
```

Each replica has its own PVC and stable DNS name. Scale down and observe the PVCs:

```bash
kubectl -n storage-lab scale statefulset db --replicas=1
kubectl -n storage-lab get pods,pvc
```

`db-1` is gone, but `data-db-1` remains. That is intentional; delete it explicitly if you want the storage back.

## Part C — Scheduling constraints

Label the node and use `nodeSelector`:

```bash
NODE=$(kubectl get nodes -o jsonpath='{.items[0].metadata.name}')
kubectl label node "$NODE" disk=fast
kubectl -n storage-lab apply -f - <<'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: pinned
spec:
  nodeSelector:
    disk: fast
  containers:
    - name: app
      image: busybox:1.36
      command: ["sh", "-c", "sleep 3600"]
EOF
kubectl -n storage-lab get pod pinned -o wide
```

Now force a mismatch:

```bash
kubectl -n storage-lab apply -f - <<'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: unschedulable
spec:
  nodeSelector:
    disk: slow
  containers:
    - name: app
      image: busybox:1.36
      command: ["sh", "-c", "sleep 3600"]
EOF
kubectl -n storage-lab describe pod unschedulable | sed -n '/Events/,$p'
```

Expected event: `node(s) didn't match Pod's node affinity/selector`.

Taint and tolerate:

```bash
kubectl taint node "$NODE" dedicated=lab:NoSchedule
kubectl -n storage-lab apply -f - <<'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: not-tolerated
spec:
  containers:
    - name: app
      image: busybox:1.36
      command: ["sh", "-c", "sleep 3600"]
EOF
kubectl -n storage-lab describe pod not-tolerated | sed -n '/Events/,$p'
```

Expected: `node(s) had untolerated taint {dedicated: lab}`. Add the toleration:

```bash
kubectl -n storage-lab apply -f - <<'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: tolerated
spec:
  tolerations:
    - key: dedicated
      operator: Equal
      value: lab
      effect: NoSchedule
  containers:
    - name: app
      image: busybox:1.36
      command: ["sh", "-c", "sleep 3600"]
EOF
kubectl -n storage-lab get pod tolerated -o wide
```

Remove the taint when done:

```bash
kubectl taint node "$NODE" dedicated=lab:NoSchedule-
```

## Part D — Requests, quotas, and OOM

Impossible request:

```bash
kubectl -n storage-lab apply -f - <<'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: too-big
spec:
  containers:
    - name: app
      image: busybox:1.36
      command: ["sh", "-c", "sleep 3600"]
      resources:
        requests:
          memory: 100Gi
EOF
kubectl -n storage-lab describe pod too-big | sed -n '/Events/,$p'
```

Expected: `Insufficient memory`. The scheduler uses requests, not the image's actual usage.

Quota:

```bash
kubectl -n storage-lab apply -f - <<'EOF'
apiVersion: v1
kind: ResourceQuota
metadata:
  name: small
spec:
  hard:
    requests.cpu: "1"
    requests.memory: 1Gi
    pods: "5"
EOF
kubectl -n storage-lab describe resourcequota small
kubectl -n storage-lab apply -f - <<'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: over-quota
spec:
  containers:
    - name: app
      image: busybox:1.36
      command: ["sh", "-c", "sleep 3600"]
      resources:
        requests:
          cpu: "2"
          memory: 2Gi
EOF
```

Expected: `exceeded quota: small`. Delete the quota before continuing if it blocks later Pods:

```bash
kubectl -n storage-lab delete resourcequota small
```

OOM:

```bash
kubectl -n storage-lab apply -f - <<'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: oom
spec:
  restartPolicy: Never
  containers:
    - name: app
      image: python:3.12-alpine
      command: ["python3", "-c", "a=bytearray(200*1024*1024); print(len(a))"]
      resources:
        limits:
          memory: 64Mi
EOF
kubectl -n storage-lab get pod oom
kubectl -n storage-lab describe pod oom | grep -A6 'Last State'
kubectl -n storage-lab get pod oom -o jsonpath='{.status.containerStatuses[0].lastState.terminated.reason}{"\n"}'
```

Expected: `OOMKilled`, exit 137. The `restartPolicy: Never` keeps the failed Pod so you can inspect it.

## Part E — RBAC

```bash
kubectl -n storage-lab create serviceaccount observer
kubectl -n storage-lab create role pod-reader --verb=get,list,watch --resource=pods
kubectl -n storage-lab create rolebinding observer --role=pod-reader --serviceaccount=storage-lab:observer
kubectl auth can-i list pods -n storage-lab --as=system:serviceaccount:storage-lab:observer
kubectl auth can-i delete pods -n storage-lab --as=system:serviceaccount:storage-lab:observer
kubectl auth can-i get pods/log -n storage-lab --as=system:serviceaccount:storage-lab:observer
```

Expected:

```text
yes
no
no
```

Add the log subresource and prove the change:

```bash
kubectl -n storage-lab create role pod-log-reader --verb=get --resource=pods/log
kubectl -n storage-lab create rolebinding observer-logs --role=pod-log-reader --serviceaccount=storage-lab:observer
kubectl auth can-i get pods/log -n storage-lab --as=system:serviceaccount:storage-lab:observer
```

```text
yes
```

This is the correct pattern: one minimal Role and one binding per capability, not `cluster-admin`.

## Expected observations

| Check | Expected |
|---|---|
| PVC before Pod | `Pending` with `WaitForFirstConsumer` |
| PVC after Pod | `Bound` |
| Data after Pod replacement | Persisted |
| PVC deleted | PV deleted (`Delete` reclaim) |
| StatefulSet | `db-0`, `db-1`, separate PVCs, stable DNS |
| Node selector mismatch | Pending, affinity event |
| Taint without toleration | Pending, untolerated taint event |
| 100Gi request | Pending, insufficient memory |
| Quota overrun | Forbidden, exceeded quota |
| Memory limit 64Mi + 200M alloc | OOMKilled, 137 |
| RBAC list/delete/log | yes/no/no then yes for logs |

## Internal explanation

- A PVC is a request; the StorageClass/CSI provisioner creates the PV. `WaitForFirstConsumer` defers provisioning until scheduling, so topology matches.
- The local-path provisioner uses a directory on one node; it is not replicated and not suitable for production data.
- StatefulSet PVCs are stable per ordinal and survive scale-down; that is why storage cleanup is explicit.
- Scheduling uses requests and constraints; limits are enforced later by cgroups.
- RBAC is additive and namespace-scoped; subresources are separate resources.

## Troubleshooting task

Three tickets, evidence first:

1. A PVC with `storageClassName: fast-missing` stays `Pending`, and the Pod is `Pending`. Name the fault domain and the next two commands.
2. A Pod with `nodeSelector: disk=slow` is Pending, but the node has plenty of CPU/memory. Why?
3. A Pod is `OOMKilled` every few minutes, but the node has 60% memory free. Why?

Write your answers before reading the debrief.

## Verification

- [ ] You showed a PVC binding and data persistence across Pod deletion.
- [ ] You ran a StatefulSet with two PVCs and stable DNS.
- [ ] You forced Pending via selector, taint, and requests, and fixed each.
- [ ] You observed an OOMKilled Pod and read the reason.
- [ ] You proved RBAC with `auth can-i` before and after adding a subresource.
- [ ] You can explain why local-path storage is not production-grade.

## Cleanup

```bash
kubectl delete namespace storage-lab --wait=true
kubectl label node "$NODE" disk- 2>/dev/null || true
kubectl taint node "$NODE" dedicated=lab:NoSchedule- 2>/dev/null || true
```

## Write-up

```markdown
# L4.5 — Storage, scheduling, RBAC
## PVC lifecycle and evidence
## StatefulSet identity/storage
## Three Pending causes and their events
## OOM evidence
## RBAC before/after matrix
## What I would monitor in production
```

## Debrief

1. **Missing StorageClass:** the PVC cannot be provisioned; `describe pvc` shows `storageclass.storage.k8s.io "fast-missing" not found`. Check `kubectl get storageclass` and the CSI provisioner logs. Fix the class name or create the class.
2. **Node selector mismatch:** labels are exact; `disk=slow` does not exist. `describe pod` shows the affinity event. Fix the label/selector, not the resources.
3. **OOMKilled despite free node memory:** the container's cgroup memory limit (64Mi) is lower than its allocation (200M). The node being free is irrelevant; the limit is local. Raise the limit or reduce usage.

---

# Lab L4.6 — Workload Failure Injection

## Objective

Diagnose six broken workloads from evidence only: missing configuration, bad command, failed init container, bad probes, image pull failure, and OOM. Produce an evidence table and fix each with the smallest change.

## Prerequisites

- L4.3 cluster running.
- `kubectl`.
- 35–45 minutes.

## Setup

```bash
kubectl create namespace broken
```

Apply the broken manifests:

```bash
kubectl -n broken apply -f - <<'EOF'
apiVersion: apps/v1
kind: Deployment
metadata:
  name: missing-config
spec:
  replicas: 1
  selector:
    matchLabels: {app: missing-config}
  template:
    metadata:
      labels: {app: missing-config}
    spec:
      containers:
        - name: app
          image: busybox:1.36
          command: ["sh", "-c", "echo start; sleep 3600"]
          envFrom:
            - configMapRef:
                name: app-config
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: bad-command
spec:
  replicas: 1
  selector:
    matchLabels: {app: bad-command}
  template:
    metadata:
      labels: {app: bad-command}
    spec:
      containers:
        - name: app
          image: busybox:1.36
          command: ["/bin/sh", "-c", "echo about to fail; exit 7"]
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: bad-init
spec:
  replicas: 1
  selector:
    matchLabels: {app: bad-init}
  template:
    metadata:
      labels: {app: bad-init}
    spec:
      initContainers:
        - name: init
          image: busybox:1.36
          command: ["sh", "-c", "echo init; exit 1"]
      containers:
        - name: app
          image: busybox:1.36
          command: ["sh", "-c", "sleep 3600"]
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: bad-probe
spec:
  replicas: 1
  selector:
    matchLabels: {app: bad-probe}
  template:
    metadata:
      labels: {app: bad-probe}
    spec:
      containers:
        - name: app
          image: nginx:1.27
          ports:
            - containerPort: 80
          readinessProbe:
            httpGet:
              path: /does-not-exist
              port: 80
            periodSeconds: 3
          livenessProbe:
            httpGet:
              path: /healthz
              port: 80
            periodSeconds: 5
            failureThreshold: 3
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: bad-image
spec:
  replicas: 1
  selector:
    matchLabels: {app: bad-image}
  template:
    metadata:
      labels: {app: bad-image}
    spec:
      containers:
        - name: app
          image: nginx:this-tag-does-not-exist
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: oom
spec:
  replicas: 1
  selector:
    matchLabels: {app: oom}
  template:
    metadata:
      labels: {app: oom}
    spec:
      containers:
        - name: app
          image: python:3.12-alpine
          command: ["python3", "-c", "a=bytearray(200*1024*1024); print(len(a)); import time; time.sleep(3600)"]
          resources:
            limits:
              memory: 64Mi
EOF
```

Wait and inventory:

```bash
kubectl -n broken get pods
kubectl -n broken get deploy
kubectl -n broken get events --sort-by=.lastTimestamp | tail -30
```

Expected (names will vary):

```text
NAME                              READY   STATUS                       RESTARTS
missing-config-...                0/1     CreateContainerConfigError   0
bad-command-...                   0/1     CrashLoopBackOff             3
bad-init-...                      0/1     Init:CrashLoopBackOff         2
bad-probe-...                     0/1     Running                       1
bad-image-...                     0/1     ImagePullBackOff             0
oom-...                           0/1     CrashLoopBackOff             3
```

## Part A — Missing configuration

```bash
POD=$(kubectl -n broken get pod -l app=missing-config -o jsonpath='{.items[0].metadata.name}')
kubectl -n broken describe pod "$POD"
kubectl -n broken logs "$POD" --previous 2>&1 | head
kubectl -n broken get configmap
```

Expected: `CreateContainerConfigError`, event `configmap "app-config" not found`. The container never starts, so there are no app logs. Fix by creating the missing ConfigMap:

```bash
kubectl -n broken create configmap app-config --from-literal=APP_MODE=lab
kubectl -n broken get pod -l app=missing-config
```

## Part B — Bad command

```bash
POD=$(kubectl -n broken get pod -l app=bad-command -o jsonpath='{.items[0].metadata.name}')
kubectl -n broken describe pod "$POD" | grep -A8 'Last State'
kubectl -n broken logs "$POD" --previous
kubectl -n broken get pod "$POD" -o jsonpath='{.status.containerStatuses[0].restartCount}{"\n"}'
```

Expected: exit code 7, logs `about to fail`. The app is doing exactly what the command says. Fix the command/args, or replace the workload with the real image entrypoint.

```bash
kubectl -n broken patch deploy/bad-command --type=json \
  -p='[{"op":"replace","path":"/spec/template/spec/containers/0/command","value":["sh","-c","echo ok; sleep 3600"]}]'
```

## Part C — Failed init container

```bash
POD=$(kubectl -n broken get pod -l app=bad-init -o jsonpath='{.items[0].metadata.name}')
kubectl -n broken describe pod "$POD" | sed -n '/Init Containers/,/Containers/p'
kubectl -n broken logs "$POD" -c init --previous
```

Expected: `Init:CrashLoopBackOff`, init logs `init`, exit 1. The app container has not started. Fix the init command or the dependency it is waiting for.

```bash
kubectl -n broken patch deploy/bad-init --type=json \
  -p='[{"op":"replace","path":"/spec/template/spec/containers/0/command","value":["sh","-c","sleep 3600"]},{"op":"add","path":"/spec/template/spec/initContainers/0/command","value":["sh","-c","echo init-ok"]}]'
```

## Part D — Probes

```bash
POD=$(kubectl -n broken get pod -l app=bad-probe -o jsonpath='{.items[0].metadata.name}')
kubectl -n broken get pod "$POD"
kubectl -n broken describe pod "$POD" | sed -n '/Events/,$p'
kubectl -n broken logs "$POD" --tail=20
```

Expected: `Running` but `0/1 Ready`; readiness probe gets 404; liveness probe also 404 and restarts the container. This is a double failure: readiness removes it from endpoints, liveness restarts it. Fix both probe paths/ports (or use `/` for readiness and `/` for liveness in this lab):

```bash
kubectl -n broken patch deploy/bad-probe --type=json \
  -p='[{"op":"replace","path":"/spec/template/spec/containers/0/readinessProbe/httpGet/path","value":"/"},{"op":"replace","path":"/spec/template/spec/containers/0/livenessProbe/httpGet/path","value":"/"}]'
```

## Part E — Image pull

```bash
POD=$(kubectl -n broken get pod -l app=bad-image -o jsonpath='{.items[0].metadata.name}')
kubectl -n broken describe pod "$POD" | sed -n '/Events/,$p'
kubectl -n broken get pod "$POD" -o jsonpath='{.spec.containers[0].image}{"\n"}'
```

Expected: `Failed to pull image ... not found` or `manifest unknown`. Fix the tag:

```bash
kubectl -n broken set image deploy/bad-image app=nginx:1.27
```

## Part F — OOM

```bash
POD=$(kubectl -n broken get pod -l app=oom -o jsonpath='{.items[0].metadata.name}')
kubectl -n broken describe pod "$POD" | grep -A6 'Last State'
kubectl -n broken get pod "$POD" -o jsonpath='{.status.containerStatuses[0].lastState.terminated.reason}{"\n"}'
kubectl -n broken top pod "$POD" 2>/dev/null || true
```

Expected: `OOMKilled`, 137. Fix by raising the limit only after understanding the allocation, or reducing the workload:

```bash
kubectl -n broken patch deploy/oom --type=json \
  -p='[{"op":"replace","path":"/spec/template/spec/containers/0/resources/limits/memory","value":"256Mi"}]'
```

## Expected observations

| Workload | Status | Key evidence | Root cause |
|---|---|---|---|
| `missing-config` | `CreateContainerConfigError` | `configmap "app-config" not found` | Missing ConfigMap |
| `bad-command` | `CrashLoopBackOff` | exit 7, logs `about to fail` | Intentional command failure |
| `bad-init` | `Init:CrashLoopBackOff` | init exit 1 | Init container fails |
| `bad-probe` | `Running`, `0/1` | 404 probe events, restarts | Wrong probe path |
| `bad-image` | `ImagePullBackOff` | manifest unknown | Bad tag |
| `oom` | `CrashLoopBackOff` | `OOMKilled`, 137 | Limit below allocation |

## Internal explanation

- `CreateContainerConfigError` happens before the container starts: the kubelet cannot build the container's config (missing ConfigMap/Secret key, invalid env).
- `Init:CrashLoopBackOff` means the app container has not started; debugging the app logs is futile until the init container succeeds.
- Readiness controls endpoints; liveness restarts. A wrong liveness path can turn a config error into a restart loop.
- `ImagePullBackOff` is a registry/auth/tag problem; the container runtime never starts.
- OOM is a cgroup limit, not a node-wide memory shortage.

## Troubleshooting task

Create a seventh broken workload of your own that produces `Pending` rather than `CrashLoop`. It must include an event that names the exact cause. Then write the one-line fix and prove it. Suggested causes: impossible request, nodeSelector mismatch, missing PVC, or a taint without a toleration.

## Verification

- [ ] All six workloads reach `Running`/`Ready` after fixes (or `Completed` where appropriate).
- [ ] You captured each failure's status, event, and exit code before fixing.
- [ ] You explained why `CreateContainerConfigError` has no app logs.
- [ ] You explained the difference between readiness and liveness impact.
- [ ] You created and fixed a seventh `Pending` failure.
- [ ] You did not use `kubectl delete pod` as the first response.

## Cleanup

```bash
kubectl delete namespace broken --wait=true
```

## Write-up

```markdown
# L4.6 — Workload failure injection
| Workload | Symptom | Evidence | Fault domain | Fix | Verification |
|---|---|---|---|---|---|
## The failure that took longest to diagnose
## What I would alert on to catch these earlier
## The seventh failure I created
```

## Debrief

The six failures map to six different layers: configuration (missing ConfigMap), process (exit 7), init lifecycle, probe semantics, image distribution, and cgroup memory. The `kubectl describe` event names the layer in most cases. The skill is reading the event and stopping at the right layer instead of restarting the Deployment and hoping.

---

# Lab L4.7 — Node Failure, Drain, and Capstone

## Objective

Run a multi-node kind cluster, drain a worker, simulate a NotReady node, observe rescheduling and PDB behavior, and solve a capstone with five independent broken workloads.

## Prerequisites

- L4.3–L4.6 completed.
- Docker/Podman, `kind`, `kubectl`.
- 8 GB RAM free; 3 kind nodes.
- 50–60 minutes.

## Setup

Create a three-node cluster (one control plane, two workers):

```bash
mkdir -p /tmp/l4.7 && cd /tmp/l4.7
cat > kind-ha.yaml <<'EOF'
kind: Cluster
apiVersion: kind.x-k8s.io/v1alpha4
nodes:
  - role: control-plane
    extraPortMappings:
      - containerPort: 30080
        hostPort: 30080
        protocol: TCP
  - role: worker
  - role: worker
EOF
kind create cluster --name opslab-ha --config kind-ha.yaml
kubectl get nodes -o wide
```

```text
NAME                    STATUS   ROLES           AGE   VERSION
opslab-ha-control-plane Ready    control-plane   60s   v1.30.0
opslab-ha-worker        Ready    <none>          45s   v1.30.0
opslab-ha-worker2       Ready    <none>          45s   v1.30.0
```

Create a namespace and a resilient Deployment:

```bash
kubectl create namespace ha-lab
kubectl -n ha-lab create deployment web --image=nginx:1.27 --replicas=6
kubectl -n ha-lab expose deployment web --port=80 --target-port=80
kubectl -n ha-lab create pdb web-pdb --min-available=3 --selector=app=web
kubectl -n ha-lab get pods -o wide
```

## Part A — Cordon and drain

```bash
WORKER=opslab-ha-worker
kubectl cordon "$WORKER"
kubectl get nodes
kubectl -n ha-lab get pods -o wide
```

Expected: the worker is `SchedulingDisabled`; existing Pods keep running. Now drain:

```bash
kubectl drain "$WORKER" --ignore-daemonsets --delete-emptydir-data
kubectl -n ha-lab get pods -o wide
kubectl -n ha-lab get pdb web-pdb
```

Expected: Pods on the worker are evicted and rescheduled to the other worker/control plane (depending on taints). The PDB may slow eviction but should not block it if `minAvailable=3` and 6 replicas exist.

Check whether the control plane accepts workloads:

```bash
kubectl describe node opslab-ha-control-plane | grep Taints
```

If it is tainted `NoSchedule`, the remaining worker must hold all 6 replicas. If the drain hangs, inspect `kubectl get pdb`, `kubectl get pods -o wide`, and the eviction events. Do not use `--force` until you understand why.

Uncordon and watch:

```bash
kubectl uncordon "$WORKER"
kubectl -n ha-lab get pods -o wide
```

The scheduler does not automatically rebalance existing Pods; it only places new ones. That is expected.

## Part B — Simulate NotReady

The cleanest kind simulation is to freeze the worker container. This is like a node/network partition: kubelet stops sending heartbeats and every process on the node stops, but the container is not deleted.

```bash
docker pause opslab-ha-worker
kubectl get nodes -w
# Ctrl-C when the worker is NotReady
kubectl describe node opslab-ha-worker | sed -n '/Conditions/,/Addresses/p'
kubectl -n ha-lab get pods -o wide
kubectl get events -A --sort-by=.lastTimestamp | tail -20
```

Expected: the node becomes `NotReady`; Pods on it remain `Running` in the API for a while, then the node controller marks them for deletion/eviction. They cannot be rescheduled until the node is fenced or the Pods are deleted, because the control plane cannot confirm whether they are still running.

Recover:

```bash
docker unpause opslab-ha-worker
kubectl get nodes
kubectl -n ha-lab get pods -o wide
```

If the kubelet does not recover, the node stays NotReady; inspect `docker logs opslab-ha-worker --tail=50` and `docker exec -it opslab-ha-worker crictl ps`. A kind node is disposable: `kind delete cluster --name opslab-ha` and recreate is a valid lab recovery.

**Alternative (kubelet-specific):** if the node image runs systemd, `docker exec -it opslab-ha-worker systemctl stop kubelet` simulates a kubelet crash; start it again with `systemctl start kubelet`. Do not use both methods; record which one you used.

## Part C — Control plane evidence (read-only)

Do not stop the control plane in this lab. Collect:

```bash
kubectl get --raw='/readyz?verbose'
kubectl -n kube-system get pods
docker exec -it opslab-ha-control-plane crictl ps
docker exec -it opslab-ha-control-plane crictl pods
kubectl -n kube-system logs kube-apiserver-opslab-ha-control-plane --tail=30
kubectl -n kube-system logs etcd-opslab-ha-control-plane --tail=30
```

Write one sentence for each: what would break if this component were unavailable? What would still work?

## Part D — Capstone: five broken workloads

Apply all five, then diagnose and fix each. Do not read the debrief until you have filled the evidence table.

```bash
kubectl create namespace capstone
kubectl -n capstone apply -f - <<'EOF'
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: data
spec:
  accessModes: ["ReadWriteOnce"]
  storageClassName: fast-missing
  resources:
    requests:
      storage: 1Gi
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: cap-image
spec:
  replicas: 1
  selector:
    matchLabels: {app: cap-image}
  template:
    metadata:
      labels: {app: cap-image}
    spec:
      containers:
        - name: app
          image: nginx:no-such-tag
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: cap-config
spec:
  replicas: 1
  selector:
    matchLabels: {app: cap-config}
  template:
    metadata:
      labels: {app: cap-config}
    spec:
      containers:
        - name: app
          image: busybox:1.36
          command: ["sh", "-c", "sleep 3600"]
          env:
            - name: MODE
              valueFrom:
                configMapKeyRef:
                  name: missing-config
                  key: MODE
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: cap-probe
spec:
  replicas: 2
  selector:
    matchLabels: {app: cap-probe}
  template:
    metadata:
      labels: {app: cap-probe}
    spec:
      containers:
        - name: app
          image: nginx:1.27
          ports:
            - containerPort: 80
          readinessProbe:
            httpGet:
              path: /missing
              port: 80
            periodSeconds: 3
---
apiVersion: v1
kind: Service
metadata:
  name: cap-probe
spec:
  selector:
    app: cap-probe-wrong
  ports:
    - port: 80
      targetPort: 80
---
apiVersion: v1
kind: Pod
metadata:
  name: cap-pvc
spec:
  containers:
    - name: app
      image: busybox:1.36
      command: ["sh", "-c", "sleep 3600"]
      volumeMounts:
        - name: data
          mountPath: /data
  volumes:
    - name: data
      persistentVolumeClaim:
        claimName: data
EOF
kubectl -n capstone get pods,deploy,svc,pvc
kubectl -n capstone get events --sort-by=.lastTimestamp | tail -30
```

Deliver an evidence table:

| Workload | Status | Event/exit | Fault domain | Fix | Verification |
|---|---|---|---|---|---|
| cap-image | | | | | |
| cap-config | | | | | |
| cap-probe | | | | | |
| cap-probe Service | | | | | |
| cap-pvc | | | | | |

Then fix all five. Suggested verification:

```bash
kubectl -n capstone get pods
kubectl -n capstone get endpointslices -l kubernetes.io/service-name=cap-probe
kubectl -n capstone run curl --rm -it --restart=Never --image=curlimages/curl -- \
  curl -sS --max-time 5 http://cap-probe.capstone.svc.cluster.local
```

## Expected observations

| Check | Expected |
|---|---|
| Cordon | Node `SchedulingDisabled`, Pods stay |
| Drain | Pods evicted to other schedulable node(s) |
| PDB | May delay eviction; does not prevent node failure |
| Kubelet stopped / node paused | Node `NotReady`; Pods remain for a grace period |
| Control plane read-only | API/etcd healthy, all components visible |
| Capstone | Five distinct failure layers |

## Internal explanation

- `drain` cordons and evicts; it is a voluntary disruption, so PDBs apply.
- A NotReady node does not instantly kill containers; the kubelet is gone, but the runtime may keep them running. The control plane waits, then marks Pods for deletion; rescheduling depends on the controller and eviction.
- PDBs protect availability during voluntary operations, not against a node disappearing.
- The capstone combines image, config, probe, Service selector, and storage faults; each has a distinct evidence path.

## Troubleshooting task

Write a one-page incident review for the NotReady simulation:

- Timeline (node stopped → NotReady → Pod status → recovery).
- What stayed up and why.
- What would have happened with a PDB and 6 replicas on 2 workers.
- The first three commands you would run next time.
- One monitoring signal that would have caught it sooner.

## Verification

- [ ] You drained a worker and observed rescheduling.
- [ ] You observed `NotReady` and explained why Pods did not die immediately.
- [ ] You collected control plane evidence without breaking it.
- [ ] You filled the capstone evidence table.
- [ ] All five capstone workloads are fixed and verified.
- [ ] You wrote the incident review.

## Cleanup

```bash
kubectl delete namespace ha-lab capstone --wait=true 2>/dev/null || true
kind delete cluster --name opslab-ha
rm -rf /tmp/l4.7
```

## Write-up

```markdown
# L4.7 — Node failure and capstone
## Drain timeline and evidence
## NotReady timeline and evidence
## What stayed up / what did not
## Capstone evidence table
## The fix I am least confident about
## Three commands I will run first next time
```

## Debrief — capstone

1. **cap-image:** `ImagePullBackOff`, `manifest unknown` for `nginx:no-such-tag`. Fix the tag (`nginx:1.27`).
2. **cap-config:** `CreateContainerConfigError`, `configmap "missing-config" not found` (or missing key). Create the ConfigMap with `MODE=lab`, or remove the env reference.
3. **cap-probe:** Pods `Running` but `0/2 Ready`; readiness probe 404. Fix the path to `/` (or the correct health endpoint). The Service also selects `app: cap-probe-wrong`, so even after readiness, endpoints are empty. Fix both the probe and the Service selector.
4. **cap-probe Service:** endpoints empty because the selector does not match the Pod labels. Patch the selector to `app: cap-probe`.
5. **cap-pvc:** PVC `Pending` because `storageClassName: fast-missing` does not exist; the Pod is Pending. Change to `standard` (kind) or create the class.
