# Kubernetes Storage, RBAC, Scheduling, and Resources

> **The one-sentence model:** storage is a claim (PVC) satisfied by a volume (PV) through a driver (CSI) and a class (StorageClass); authorization is a verb on a resource for a subject, bound in a namespace or cluster; scheduling is a filter-and-score decision based on requests and constraints; and resources are enforced by cgroups, not by the YAML alone.

---

## 1. Storage objects

```text
Pod
 └── volumeMounts → volume (persistentVolumeClaim)
                        │
                     PVC (claim: size, class, access mode)
                        │ binds
                     PV (actual volume: CSI driver, handle, capacity)
                        │ provisioned by
                     StorageClass → CSI driver → array/cloud disk
```

| Object | Namespaced? | Purpose |
|---|---|---|
| `PersistentVolume` (PV) | No | Cluster-wide storage resource |
| `PersistentVolumeClaim` (PVC) | Yes | A Pod's request for storage |
| `StorageClass` | No | Provisioner, parameters, binding mode, expansion policy |
| `CSIDriver` / `CSINode` | No | Driver capabilities and node topology |
| `VolumeSnapshot` / `VolumeSnapshotClass` | Yes / No | Point-in-time snapshots (if the driver supports them) |

### Access modes

| Mode | Meaning | Typical backend |
|---|---|---|
| `ReadWriteOnce` (RWO) | Mounted read-write by a single **node** | Block disk, EBS, local NVMe |
| `ReadOnlyMany` (ROX) | Mounted read-only by many nodes | NFS, CephFS, object-backed |
| `ReadWriteMany` (RWX) | Mounted read-write by many nodes | NFS, CephFS, SMB |
| `ReadWriteOncePod` (RWOP) | Mounted by a single **Pod** | Block disk; strict single-writer |

RWO is per node, not per Pod: two Pods on the same node can mount the same RWO volume. That is why a Deployment with a single RWO PVC and replicas on different nodes produces `Multi-Attach error`.

### Binding and provisioning

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: fast
provisioner: ebs.csi.aws.com
parameters:
  type: gp3
  iops: "6000"
  throughput: "250"
reclaimPolicy: Delete
allowVolumeExpansion: true
volumeBindingMode: WaitForFirstConsumer
```

- **Immediate** binding provisions the volume as soon as the PVC is created. If the volume is zonal, it may be provisioned in a zone where no Pod can run.
- **WaitForFirstConsumer** delays provisioning until a Pod using the PVC is scheduled, so the volume lands in the right zone/topology. This is the usual choice for zonal block storage.
- **Static** provisioning: an admin creates a PV; the PVC binds to a matching PV by class, size, access mode, and selector.
- **Dynamic** provisioning: the StorageClass's CSI driver creates the volume.

### Reclaim policy

| Policy | What happens when the PVC is deleted |
|---|---|
| `Delete` | The PV and the backing volume are deleted |
| `Retain` | The PV remains, `Released`; manual cleanup/reuse required |

`Retain` is not a backup. It preserves a volume that may still be tied to a deleted claim; you must clean it up or re-import it deliberately.

### The volume lifecycle

```text
PVC created
  → provisioner creates PV/backing disk
  → PVC Bound
  → Pod scheduled (topology must match)
  → CSI controller attaches volume to node
  → CSI node plugin stages/publishes it
  → kubelet mounts it into the container
  → Pod runs
  → Pod deleted: unmount, detach
  → PVC deleted: PV reclaimed (Delete/Retain)
```

Each arrow can fail. `kubectl describe pvc` and Pod events tell you which one.

### CSI in one page

The Container Storage Interface defines three services:

| Service | Runs on | Responsibility |
|---|---|---|
| Identity | All plugins | Driver name, capabilities, probe |
| Controller | Control plane (or central) | Create/delete volume, attach/detach, snapshot, resize |
| Node | Every node | Stage/publish/unpublish, mount, node capabilities |

A CSI driver is usually a Deployment (controller) plus a DaemonSet (node plugin). The external-provisioner, external-attacher, external-resizer, and external-snapshotter sidecars translate Kubernetes objects into CSI calls.

```bash
kubectl get csidrivers
kubectl get csinodes
kubectl get pods -n kube-system -l app=ebs-csi-driver
kubectl describe pvc data
```

### StatefulSet storage

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: db
spec:
  serviceName: db
  replicas: 3
  selector:
    matchLabels: {app: db}
  template:
    metadata:
      labels: {app: db}
    spec:
      containers:
        - name: db
          image: postgres:16
          volumeMounts:
            - name: data
              mountPath: /var/lib/postgresql/data
  volumeClaimTemplates:
    - metadata:
        name: data
      spec:
        accessModes: ["ReadWriteOnce"]
        storageClassName: fast
        resources:
          requests:
            storage: 100Gi
```

- `db-0`, `db-1`, `db-2` each get `data-db-0`, `data-db-1`, `data-db-2`.
- The headless Service `db` gives stable DNS: `db-0.db.app.svc.cluster.local`.
- Scaling down does **not** delete PVCs by default; this is intentional but can leak storage.
- `OrderedReady` starts/stops in order; a stuck `db-0` blocks the rest.

---

## 2. RBAC

Authentication answers **who** you are (client cert, token, OIDC, ServiceAccount). Authorization answers **what** you may do (RBAC, webhook, ABAC). Admission can then mutate/validate the request.

```text
kubectl request
  → authentication (cert/token/OIDC)
  → impersonation checks
  → authorization (RBAC: verb + resource + namespace)
  → admission (mutating, validating)
  → etcd
```

### The four objects

| Object | Scope | Binds |
|---|---|---|
| `Role` | Namespace | Permissions |
| `ClusterRole` | Cluster | Permissions (including cluster-scoped resources) |
| `RoleBinding` | Namespace | Role or ClusterRole → subjects in that namespace |
| `ClusterRoleBinding` | Cluster | ClusterRole → subjects cluster-wide |

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  namespace: app
  name: pod-reader
rules:
  - apiGroups: [""]
    resources: ["pods", "pods/log"]
    verbs: ["get", "list", "watch"]
  - apiGroups: ["apps"]
    resources: ["deployments"]
    verbs: ["get", "list"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  namespace: app
  name: read-pods
subjects:
  - kind: ServiceAccount
    name: observer
    namespace: app
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: Role
  name: pod-reader
```

Rules are additive; there are no deny rules. A `ClusterRoleBinding` to `cluster-admin` is effectively unrestricted.

### Verbs and subresources

Common verbs: `get`, `list`, `watch`, `create`, `update`, `patch`, `delete`, `deletecollection`. Subresources are separate resources: `pods/log`, `pods/exec`, `pods/portforward`, `deployments/scale`, `nodes/status`.

```yaml
- apiGroups: [""]
  resources: ["pods/exec"]
  verbs: ["create"]        # exec is a create on pods/exec
```

**Mistake:** granting `get pods` and expecting `kubectl logs` to work; logs require `get` on `pods/log`. `kubectl exec` requires `create` on `pods/exec`.

### ServiceAccounts and tokens

```bash
kubectl create serviceaccount observer -n app
kubectl create rolebinding observer --role=pod-reader --serviceaccount=app:observer -n app
kubectl auth can-i list pods -n app --as=system:serviceaccount:app:observer
```

Modern Kubernetes uses projected, audience-bound, time-limited tokens; the legacy long-lived Secret token is no longer created automatically. `automountServiceAccountToken: false` on the Pod or ServiceAccount avoids mounting a token when the workload does not call the API.

```yaml
spec:
  automountServiceAccountToken: false
```

### Least privilege in practice

1. Start with `kubectl auth can-i --list` for the identity.
2. Grant the smallest verb/resource pair that satisfies the use case.
3. Prefer a Role in a namespace over a ClusterRoleBinding.
4. Avoid wildcards (`*`) and `escalate`/`bind`/`impersonate` unless required.
5. Separate human access from workload identities.
6. Review `system:basicauth`? No — review the default `system:authenticated` group bindings and any `cluster-admin` bindings.
7. Use `kubectl auth reconcile` for declarative RBAC changes.

```bash
kubectl get clusterrolebindings -o json | jq -r '.items[] | select(.roleRef.name=="cluster-admin") | .metadata.name'
kubectl auth can-i --list --as=system:serviceaccount:app:observer -n app
```

### Common RBAC failures

| Symptom | Cause |
|---|---|
| `forbidden: User ... cannot get resource "pods"` | No RoleBinding in the namespace, or wrong subject namespace |
| `cannot list resource "pods" at the cluster scope` | A namespaced RoleBinding does not grant cluster-wide list; use `-n` or a ClusterRoleBinding |
| `cannot get resource "pods/log"` | Missing subresource permission |
| `cannot create resource "pods/exec"` | `exec` is `create`, not `get` |
| ServiceAccount works locally but not in the Pod | Wrong ServiceAccount (`serviceAccountName`), token not mounted, or wrong namespace |
| `cannot patch resource "deployments/scale"` | HPA/scale subresource is separate |

---

## 3. Scheduling and resource management

### Requests, limits, and QoS

```yaml
resources:
  requests:
    cpu: 250m
    memory: 256Mi
  limits:
    cpu: "1"
    memory: 512Mi
```

- Scheduler uses **requests**.
- CPU limit → CFS throttling; memory limit → OOM kill.
- `Guaranteed`: requests = limits for every container (CPU and memory).
- `Burstable`: at least one request/limit, not all equal.
- `BestEffort`: none set.

Eviction under node pressure: BestEffort → Burstable exceeding requests → Guaranteed. A Pod's `QoSClass` is visible in `kubectl get pod -o jsonpath`.

### LimitRange and ResourceQuota

```yaml
apiVersion: v1
kind: LimitRange
metadata:
  name: defaults
  namespace: app
spec:
  limits:
    - type: Container
      default:
        cpu: 500m
        memory: 512Mi
      defaultRequest:
        cpu: 100m
        memory: 128Mi
      max:
        cpu: "2"
        memory: 2Gi
---
apiVersion: v1
kind: ResourceQuota
metadata:
  name: app-quota
  namespace: app
spec:
  hard:
    requests.cpu: "10"
    requests.memory: 20Gi
    limits.cpu: "20"
    limits.memory: 40Gi
    pods: "30"
    persistentvolumeclaims: "10"
```

`LimitRange` supplies defaults and bounds; `ResourceQuota` limits the aggregate. A Pod that exceeds the quota is rejected at admission with a clear message.

### Scheduling constraints

| Mechanism | Use |
|---|---|
| `nodeSelector` | Simple label match |
| Node affinity (required/preferred) | Richer expressions, zones, instance types |
| Pod affinity/anti-affinity | Co-locate or spread relative to other Pods |
| Topology spread constraints | Even spread across zones/nodes |
| Taints/tolerations | Reserve nodes for special workloads |
| PriorityClass | Preempt lower-priority Pods |
| PDB | Protect availability during voluntary disruption |

```bash
kubectl get nodes --show-labels
kubectl describe node node1 | sed -n '/Allocated resources/,/Events/p'
kubectl get pod web-abc -o jsonpath='{.spec.nodeName}'
kubectl get events --field-selector involvedObject.name=web-abc
```

### Priority and preemption

A Pod with a higher `priorityClassName` can preempt lower-priority Pods when no node fits. Preemption evicts victims (respecting PDBs where possible) and schedules the higher-priority Pod. This is powerful and dangerous: a runaway high-priority workload can evict production.

### HPA and VPA

HPA:

```bash
kubectl autoscale deployment web --cpu-percent=70 --min=2 --max=10
kubectl get hpa
kubectl describe hpa web
```

Requirements: metrics-server (or custom metrics), resource requests on the containers, and a scalable workload. HPA changes `replicas`; it does not add nodes. Cluster autoscaler/Karpenter adds nodes when Pods are unschedulable.

VPA recommends or applies resource requests; it can evict Pods to apply changes. Do not run HPA and VPA on the same metric for the same workload.

### Node pressure and eviction

```bash
kubectl describe node node1 | grep -A5 Conditions
kubectl get pods -A --field-selector status.phase=Failed
kubectl describe pod evicted-pod
```

`Evicted` Pods show a message such as `The node was low on resource: memory.` The Pod is gone; the ReplicaSet creates a replacement. Fix the workload's requests/limits or add capacity; do not just delete the evicted Pod.

---

## 4. Failure scenarios

### A — PVC Pending

```text
$ kubectl get pvc
NAME   STATUS    VOLUME   CAPACITY   ACCESS MODES   STORAGECLASS   AGE
data   Pending                                      fast           2m

$ kubectl describe pvc data
Events:
  Warning  ProvisioningFailed  ...  storageclass.storage.k8s.io "fast" not found
```

Fault domain: StorageClass/provisioner/capacity/topology. Check `kubectl get storageclass`, the CSI controller logs, events, requested size vs available quota, and access mode support. If the class uses `WaitForFirstConsumer`, the PVC stays Pending until a Pod is scheduled — that is normal, not a failure.

### B — Pod Pending because of volume topology

```text
Events:
  Warning  FailedScheduling  ...  3 node(s) had volume node affinity conflict
```

Fault domain: scheduling + storage topology. The PV is in zone A, but the Pod's node affinity/other constraints only allow zone B. Fix the node affinity, use a StorageClass with `WaitForFirstConsumer`, or restore the volume in the right zone. Do not delete the PVC; you may lose data.

### C — Multi-Attach error

```text
Events:
  Warning  FailedAttachVolume  ...  Multi-Attach error for volume "pvc-..." 
  Volume is already exclusively attached to one node and can't be attached to another
```

Fault domain: RWO volume + multiple nodes. A Deployment with replicas and a single RWO PVC is the usual cause. Use a StatefulSet with per-replica PVCs, a RWX backend, or a single replica with a rolling strategy that detaches first. Killing the old Pod/node may be necessary, but confirm the old node is truly gone.

### D — Permission denied on a mounted volume

Fault domain: UID/GID, `fsGroup`, SELinux, or the CSI driver's mount options. Check the container's `securityContext` (`runAsUser`, `fsGroup`), the volume's ownership inside the Pod, and the driver's `fsType`/mount flags. `fsGroup` changes group ownership for many volume types; it does not fix every CSI driver.

```bash
kubectl exec web-abc -- id
kubectl exec web-abc -- ls -ln /data
kubectl get pod web-abc -o yaml | grep -A10 securityContext
```

### E — OOMKilled

Covered in Phase 4 note 2; the storage-specific variant is a database with a memory limit below its shared buffers/cache. Check `kubectl describe pod` (`OOMKilled`, exit 137), `kubectl top pod`, and the app's memory settings. Raising the limit without fixing the app just delays the kill.

### F — CPU throttling

```text
$ kubectl top pod api-0
NAME    CPU(cores)   MEMORY(bytes)
api-0   998m         300Mi
```

A container at its CPU limit is throttled, not killed. Check `container_cpu_cfs_throttled_periods_total` / `container_cpu_cfs_throttled_seconds_total`, latency, and whether the limit is below the app's burst need. Remove the CPU limit or raise it if the node has capacity; keep requests for scheduling.

### G — Evicted due to DiskPressure

```text
Status: Failed
Reason: Evicted
Message: The node was low on resource: nodefs.available.
```

Fault domain: node disk. Check `df -h`, `df -i`, image/container log growth, and emptyDir usage. Clean up safely (`crictl rmi --prune`, log rotation), then fix the workload's ephemeral storage requests/limits. `emptyDir` counts against ephemeral storage.

### H — RBAC Forbidden

```bash
kubectl auth can-i get pods -n app --as=system:serviceaccount:app:api
kubectl describe rolebinding -n app
kubectl get role pod-reader -n app -o yaml
```

Fault domain: authorization. Check the verb, resource, subresource, namespace, and subject. A RoleBinding in namespace `app` cannot grant access in `prod`; a ClusterRoleBinding can, but is broader.

### I — Quota exceeded

```text
Error from server (Forbidden): pods "web-abc" is forbidden:
exceeded quota: app-quota, requested: requests.cpu=500m, used: requests.cpu=10, limited: requests.cpu=10
```

Fault domain: namespace quota. Check `kubectl describe resourcequota -n app`, current usage, and whether the Pod's requests are larger than expected. Fix the request or raise the quota deliberately.

### J — HPA not scaling

Fault domain: metrics/requests/HPA. `kubectl describe hpa` shows `<unknown>` targets when metrics-server is missing; missing requests make CPU utilization undefined; a max replica count or quota can block scale-up. Check `kubectl top pods`, `kubectl get apiservice v1beta1.metrics.k8s.io`, and HPA events.

### K — PVC resize stuck

```text
$ kubectl get pvc data
NAME   STATUS   VOLUME   CAPACITY   ACCESS MODES   STORAGECLASS   AGE
data   Bound    pvc-...  100Gi      RWO            fast           10d

$ kubectl describe pvc data
Conditions: ... FileSystemResizePending
```

Fault domain: StorageClass/driver/filesystem. Check `allowVolumeExpansion: true`, the CSI driver's resize support, the PV's `capacity`, and the filesystem inside the Pod. Some drivers require a Pod restart to finish node-level expansion; some cannot shrink at all. Never assume `kubectl edit pvc` grows the underlying disk.

---

## 5. Command field manual

### `kubectl get pv,pvc,sc`

```bash
kubectl get pv
kubectl get pvc -A
kubectl get storageclass
kubectl describe pvc data -n app
```

**Interpretation:** `STATUS` (`Pending`, `Bound`, `Lost`, `Released`) and `Events` tell you the lifecycle stage. `storageclass` shows `PROVISIONER`, `RECLAIMPOLICY`, `VOLUMEBINDINGMODE`, `ALLOWVOLUMEEXPANSION`.

### `kubectl auth can-i`

```bash
kubectl auth can-i create pods/exec -n app
kubectl auth can-i --list --as=system:serviceaccount:app:api -n app
```

**Why:** tests the actual authorization decision, including default roles and bindings, instead of reading YAML.

**Mistake:** testing as your admin user and concluding the ServiceAccount can do it.

### `kubectl create rolebinding` / `role`

```bash
kubectl create role pod-reader --verb=get,list,watch --resource=pods -n app
kubectl create rolebinding pod-reader --role=pod-reader --serviceaccount=app:observer -n app
```

**Why:** quick, correct scaffolding; use YAML for review/version control.

### `kubectl describe node`

```bash
kubectl describe node node1
```

**Interpretation:** `Allocated resources` shows requests vs allocatable; `Conditions` shows pressure; `Taints` shows scheduling restrictions; `Events` shows kubelet problems.

### `kubectl top`

```bash
kubectl top nodes
kubectl top pods -n app --sort-by=memory
```

**Why:** live usage vs requests/limits. Requires metrics-server; absence is a cluster addon issue, not a kubectl bug.

### `kubectl explain`

```bash
kubectl explain pvc.spec
kubectl explain pod.spec.topologySpreadConstraints
```

**Why:** version-accurate schema; avoids copying fields from an older API version.

---

## 6. Checkpoint

1. What is the difference between a PV and a PVC?
2. Who provisions a PV in dynamic provisioning?
3. What does `WaitForFirstConsumer` solve?
4. What does `ReadWriteOnce` actually mean?
5. Why does a Deployment with replicas and one RWO PVC fail?
6. What is the difference between `Delete` and `Retain` reclaim policies?
7. What are the three CSI services, and where does each run?
8. Why does a StatefulSet use a headless Service?
9. What happens to a StatefulSet's PVCs when you scale down?
10. What is the difference between a Role and a ClusterRole?
11. What permission does `kubectl logs` need? `kubectl exec`?
12. Why is `cluster-admin` a bad debugging default?
13. What is the difference between a request and a limit at scheduling time?
14. Which QoS class is evicted first under node pressure?
15. What does a `NoExecute` taint do?
16. What is a PDB, and what does it not protect against?
17. Why can HPA fail with `<unknown>` CPU targets?
18. What causes `Multi-Attach error`?
19. What does `fsGroup` do?
20. What is the first check for a PVC stuck in `Pending`?

### Answers

1. A PV is the cluster-wide storage resource; a PVC is a namespaced request that binds to a PV (or triggers dynamic provisioning).
2. The StorageClass's CSI provisioner (external-provisioner + driver), not the kubelet.
3. It delays volume provisioning until a Pod is scheduled, so zonal/topology-constrained volumes are created in the right place.
4. It can be mounted read-write by a single node; multiple Pods on that node may still mount it.
5. Each replica may land on a different node; the RWO volume cannot be attached to multiple nodes simultaneously.
6. `Delete` removes the backing volume when the PVC is deleted; `Retain` keeps the PV/backing volume for manual handling.
7. Identity (every plugin), Controller (control plane/central), Node (every node); identity/controller are often in a Deployment, node in a DaemonSet.
8. To provide stable DNS names (`db-0.db...`) for each replica; a normal ClusterIP Service would load-balance and hide identity.
9. They are retained by default; deleting the StatefulSet does not delete the PVCs unless you remove them explicitly.
10. A Role is namespaced; a ClusterRole is cluster-scoped (and can be bound into a namespace).
11. `logs`: `get` on `pods/log`; `exec`: `create` on `pods/exec`.
12. It grants every verb on every resource, including secrets and RBAC changes; a mistake or compromised token becomes a cluster takeover.
13. Requests are used for scheduling and guaranteed share; limits are runtime ceilings. A Pod can be scheduled with requests below its limit.
14. BestEffort, then Burstable, then Guaranteed.
15. It evicts running Pods that do not tolerate it and prevents new ones from scheduling.
16. A PodDisruptionBudget limits voluntary evictions (drain); it does not prevent node failure, kernel panic, or involuntary eviction.
17. Metrics-server is missing/unhealthy, or the containers lack CPU requests so utilization cannot be computed.
18. An RWO volume is still attached to another node (old Pod not terminated, node NotReady, or a Deployment with multiple replicas sharing one PVC).
19. It sets group ownership/permissions on the volume for the Pod's containers (support depends on the volume plugin).
20. `kubectl describe pvc` events, the StorageClass/provisioner, requested size, access mode, and whether `WaitForFirstConsumer` is waiting for a Pod.
