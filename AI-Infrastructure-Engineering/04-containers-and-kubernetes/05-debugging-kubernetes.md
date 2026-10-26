# Debugging Kubernetes

> **The rule:** do not restart, delete, or `kubectl edit` anything until you have captured the evidence that explains the failure. A deleted Pod takes its logs, events, and exit code with it. The cluster is usually telling you the answer; your job is to read the right object at the right layer.

---

## 1. The triage ladder

```text
0. Define the symptom and scope.
   What is broken, for whom, since when, and what changed?

1. Is the cluster healthy?
   API reachable, nodes Ready, control plane components up.

2. Is the object correct?
   Deployment/Pod spec, labels, selector, image, command, env.

3. Is it scheduled?
   Pending → scheduler events, requests, taints, affinity, PVC, quota.

4. Is the container starting?
   ImagePull, ContainerCreating, FailedMount, CrashLoop, exit code.

5. Is it ready?
   Probes, logs, app dependencies, startup time.

6. Is it reachable?
   Service selector → EndpointSlice → DNS → policy → packet.

7. Is the node healthy?
   kubelet, runtime, disk, memory, PID, certificates, clock.

8. Is the control plane healthy?
   apiserver, etcd, scheduler, controller-manager, addons.

9. Fix the smallest cause, verify with the same evidence, then prevent.
```

At every step, write down **what you expected**, **what you observed**, and **what that rules out**. "It's a network issue" is not a diagnosis; "SYNs leave the client but never arrive at the server, and the CNI policy drop counter increments" is.

---

## 2. First ten commands

```bash
kubectl config current-context
kubectl get nodes -o wide
kubectl get pods -A -o wide | grep -v Running
kubectl get events -A --sort-by=.lastTimestamp | tail -40
kubectl get deploy,rs,sts,ds -A
kubectl get svc,endpointslices -A
kubectl get pvc -A
kubectl get --raw='/readyz?verbose'
kubectl top nodes
kubectl version --short
```

Why this order:

- **Context first:** the most common "cluster is broken" incident is the wrong kubeconfig.
- **Nodes before Pods:** a NotReady node explains many Pod symptoms at once.
- **Events are perishable:** default TTL is about one hour; capture them before they disappear.
- **Workload controllers:** a missing ReplicaSet or a paused rollout explains missing Pods.
- **Endpoints:** a Service with no endpoints explains connection failures without touching the app.
- **PVC:** storage is a common cause of Pending and ContainerCreating.
- **Readiness:** `/readyz?verbose` names the failing control plane component.
- **Capacity:** `top` distinguishes "no requests fit" from "node is actually full".

---

## 3. Symptom → evidence table

| Symptom | First evidence | Likely layers |
|---|---|---|
| Pod `Pending` | `kubectl describe pod` events, `describe node` allocatable | Scheduler, requests, taints, affinity, PVC, quota |
| `ContainerCreating` for minutes | Pod events, `describe pvc`, CNI/CSI logs | CNI, CSI, image pull, volume attach |
| `ImagePullBackOff` | Pod events, image name/tag, `imagePullSecrets` | Registry, auth, network, arch, rate limit |
| `CrashLoopBackOff` | `logs --previous`, `describe` exit code, config | App, command, env, permission, probe, OOM |
| `Running` but `0/1 Ready` | `describe` probe failures, app logs | Readiness, dependency, startup, port |
| `OOMKilled` | `describe` last state, `top`, memory limit | App memory, limit, node pressure |
| `Evicted` | Pod status/message, node conditions | Node disk/memory/PID pressure |
| Service refuses/times out | EndpointSlice, selector, Pod IP test | Service, endpoints, policy, CNI, app |
| DNS NXDOMAIN/timeouts | `/etc/resolv.conf`, CoreDNS logs, test Pod | DNS, policy, ndots, Service name |
| Ingress 502/503/504 | Controller logs, Ingress describe, endpoints | Ingress, Service, app, timeout |
| Node `NotReady` | `describe node`, kubelet/runtime logs, disk | Node, kubelet, CRI, network, certs |
| `kubectl` times out | `/readyz`, apiserver/etcd logs, certs | Control plane, etcd quorum, LB |
| `Forbidden` | `auth can-i`, RoleBindings | RBAC, admission |
| PVC `Pending` | `describe pvc`, StorageClass, CSI logs | Storage provisioning/topology |
| `Multi-Attach` | Pod events, PV node affinity | RWO + multi-node |
| Rollout stuck | `rollout status`, RS/Pod events | Readiness, quota, image, scheduling |

---

## 4. Deep dives

### 4.1 Pending

```bash
kubectl describe pod web-abc
kubectl get events --field-selector involvedObject.name=web-abc
kubectl describe node <candidate-node> | sed -n '/Allocated resources/,/Events/p'
kubectl get pvc -n app
kubectl get resourcequota,limitrange -n app
```

Read the `FailedScheduling` message literally:

| Message | Meaning | Next check |
|---|---|---|
| `Insufficient cpu/memory` | Requests do not fit | Node allocatable vs requests; lower requests or add capacity |
| `node(s) had untolerated taint` | Taint/toleration mismatch | `kubectl describe node` taints; add toleration only if intended |
| `node(s) didn't match Pod's node affinity/selector` | Label/affinity mismatch | Node labels, affinity terms |
| `node(s) had volume node affinity conflict` | PV zone vs node | PV node affinity, StorageClass binding mode |
| `persistentvolumeclaim not found` | Wrong namespace/name | `kubectl get pvc -n <ns>` |
| `exceeded quota` | Namespace quota | `describe resourcequota` |
| `host port ... is already in use` | hostPort conflict | `kubectl get pods -o wide` on that node |
| `0/N nodes are available` + no specific reason | Often taints/cordon | `get nodes`, `describe node` |

If the scheduler message is generic, check whether all nodes are cordoned:

```bash
kubectl get nodes -o custom-columns=NAME:.metadata.name,UNSCHEDULABLE:.spec.unschedulable,READY:.status.conditions[?(@.type=="Ready")].status
```

### 4.2 CrashLoopBackOff

```bash
kubectl get pod api-0 -o wide
kubectl describe pod api-0
kubectl logs api-0 --previous
kubectl logs api-0 --previous --all-containers
kubectl get pod api-0 -o jsonpath='{.status.containerStatuses[*].lastState.terminated}'
```

Exit codes:

| Code | Meaning | Typical cause |
|---:|---|---|
| 0 | Success | Process exited; a long-running container should not do this |
| 1 | Application error | Unhandled exception, config error |
| 2 | Shell misuse/misinterpreted builtin | Bad script |
| 126 | Command not executable | Permission/entrypoint |
| 127 | Command not found | Wrong binary/PATH |
| 137 | SIGKILL | OOMKilled or forced stop |
| 139 | SIGSEGV | Native crash |
| 143 | SIGTERM | Graceful stop, or liveness kill |

Then check:

- `command`/`args` in the Pod spec (does it override the image entrypoint?).
- ConfigMap/Secret existence and keys.
- `securityContext` (UID, read-only root, capabilities).
- Probes: a liveness probe that is too aggressive causes restarts even when the app is starting normally.
- Resource limits: `OOMKilled` in `lastState`.
- Init containers: a failing init container keeps the Pod in `Init:CrashLoopBackOff`; the app container has not started.

**Trap:** `kubectl logs` without `--previous` during a crash loop often shows nothing because the current container has not produced logs yet.

### 4.3 ImagePullBackOff

```bash
kubectl describe pod web-abc | sed -n '/Events/,$p'
kubectl get pod web-abc -o jsonpath='{.spec.containers[*].image}'
kubectl get secret regcred -n app -o jsonpath='{.data.\.dockerconfigjson}' | base64 -d
docker manifest inspect ghcr.io/example/app:1.0.0
```

Causes:

- Typo/tag does not exist (most common).
- Private registry without `imagePullSecrets` or with a secret in the wrong namespace.
- Node cannot reach the registry (egress, proxy, DNS, firewall).
- Registry rate limit (`toomanyrequests`).
- Wrong architecture (`exec format error` appears later, at start, not pull).
- Image was deleted/retagged.

`ErrImagePull` is the first failure; `ImagePullBackOff` is the retry state. Read the full event message; it usually contains the registry's HTTP status.

### 4.4 OOMKilled and CPU throttling

```bash
kubectl describe pod api-0 | grep -A6 'Last State'
kubectl top pod api-0
kubectl get pod api-0 -o jsonpath='{.spec.containers[*].resources}'
kubectl get pod api-0 -o jsonpath='{.status.qosClass}'
```

- Memory limit exceeded → kernel OOM kills the process; container restarts; `OOMKilled`, exit 137.
- Node memory pressure → kubelet evicts Pods; status `Failed`, reason `Evicted`.
- CPU limit → CFS throttling; the app is slow but not killed.

For a JVM, `-Xmx` must leave room for metaspace, stacks, and off-heap; a 512Mi limit with `-Xmx512m` is a guaranteed OOM. For Go/Python/Node, find the actual RSS and the allocation pattern before raising the limit.

### 4.5 Service has no endpoints

```bash
kubectl get svc web -o yaml | sed -n '/selector/,/ports/p'
kubectl get pods -n app --show-labels
kubectl get endpointslices -l kubernetes.io/service-name=web -o yaml
kubectl get pod web-abc -o jsonpath='{.status.conditions[?(@.type=="Ready")].status}'
```

Check, in order:

1. Selector labels match Pod labels exactly.
2. The Pod is `Ready` (readiness probe passes).
3. The Service `targetPort` matches the container port/name.
4. The EndpointSlice controller is running (`kube-controller-manager`).
5. The Service is not headless with zero ready Pods.

A Service can also select Pods in another namespace? No — selectors are namespace-scoped for Pods. A Service in `app` cannot select Pods in `prod`.

### 4.6 DNS

```bash
kubectl run dnstest --rm -it --restart=Never --image=busybox:1.36 -- sh
# inside:
cat /etc/resolv.conf
nslookup kubernetes.default.svc.cluster.local
nslookup web.app.svc.cluster.local
getent hosts web.app
```

Then:

```bash
kubectl -n kube-system get pods -l k8s-app=kube-dns -o wide
kubectl -n kube-system logs -l k8s-app=kube-dns --tail=50
kubectl -n kube-system get svc kube-dns
kubectl -n kube-system get endpointslices -l kubernetes.io/service-name=kube-dns
```

If the `kube-dns` Service has no endpoints, every Pod's DNS fails. If only one namespace fails, check NetworkPolicy egress to DNS. If only external names fail, check the `forward` upstream and node resolv.conf. If names resolve slowly, check `ndots` and search paths.

### 4.7 Node NotReady

On the node:

```bash
systemctl status kubelet
journalctl -u kubelet -n 100 --no-pager
systemctl status containerd
journalctl -u containerd -n 100 --no-pager
df -h
df -i
free -m
dmesg -T | tail -50
crictl info
```

On the control plane:

```bash
kubectl describe node node1
kubectl get events --field-selector involvedObject.name=node1
kubectl get lease -n kube-node-lease node1 -o yaml
```

Common causes:

- kubelet not running / crash looping (config error, certificate expired).
- container runtime down or disk full.
- node clock skew (certificate validation, lease renewal).
- network partition between node and API server.
- disk/PID/memory pressure.
- kubelet cannot reach the container runtime socket.
- cloud instance stopped or network interface detached.

Do not delete the Node object as a first step. If it recovers, it re-registers; deleting it can orphan Pods and confuse the scheduler.

### 4.8 Control plane

```bash
kubectl get --raw='/readyz?verbose'
kubectl get --raw='/livez?verbose'
kubectl get componentstatuses        # deprecated, but still seen
kubectl -n kube-system get pods
kubectl -n kube-system logs kube-apiserver-<node> --tail=100
```

Faults in etcd:

```bash
sudo etcdctl endpoint health --cluster
sudo etcdctl endpoint status --cluster -w table
sudo journalctl -u etcd -n 100 --no-pager
```

Symptoms:

- `etcdserver: no leader` / `request timed out` → quorum loss, slow disk, network partition.
- API server 500s → admission webhook timeout, etcd unavailable, certificate issue.
- `Unable to authenticate the request` → expired client cert, clock skew, wrong CA.
- Scheduler/controller-manager not leading → leader election, API connectivity.

A single control plane node is a lab, not production. If it dies, the API is gone even though existing Pods keep running.

### 4.9 Storage

```bash
kubectl get pvc -A
kubectl describe pvc data -n app
kubectl get pv
kubectl get storageclass
kubectl -n kube-system logs deploy/ebs-csi-controller --tail=50
kubectl get events -A --field-selector reason=FailedMount
```

`ContainerCreating` for a long time usually means volume attach/mount. Check `FailedAttachVolume`, `FailedMount`, `Multi-Attach`, and the CSI node plugin logs on the node (`/var/log/` or `journalctl -u csi-node`).

---

## 5. Node-level debugging

When the API says "Running" but the container is not behaving, go to the node.

```bash
sudo crictl ps -a
sudo crictl inspect <container-id> | jq '.status, .info.runtimeSpec'
sudo crictl logs <container-id>
sudo crictl stats
```

Find the Pod's cgroup and namespaces:

```bash
sudo crictl pods --name web-abc
PID=$(sudo crictl inspectp <pod-id> | jq -r .info.pid)
sudo ls -l /proc/$PID/ns
sudo nsenter -t $PID -n ip addr
sudo nsenter -t $PID -m -p --mount-proc ps -ef
```

Check kubelet's view:

```bash
sudo journalctl -u kubelet -f
sudo ls /var/lib/kubelet/pods/<uid>/
sudo cat /var/lib/kubelet/pods/<uid>/volumes/kubernetes.io~csi/.../mount
```

Check disk and inodes:

```bash
df -h
df -i
sudo du -sh /var/lib/containerd /var/lib/kubelet /var/log
sudo crictl rmi --prune     # after confirming what will be removed
```

Check the container's cgroup:

```bash
systemd-cgls /sys/fs/cgroup/system.slice
cat /sys/fs/cgroup/kubepods.slice/kubepods-burstable.slice/.../memory.events
cat /sys/fs/cgroup/kubepods.slice/.../memory.max
```

---

## 6. Worked incident — a 503 after a rollout

**Symptom:** `GET https://app.example.com/` returns 503 for about 30% of requests, starting after a Deployment update.

**Evidence, in order:**

```text
$ kubectl rollout status deploy/web
Waiting for deployment "web" rollout to finish: 1 out of 3 new replicas have been updated...

$ kubectl get pods -l app=web -o wide
NAME                   READY   STATUS    RESTARTS   AGE   IP
web-7d9f8c6b5-abcde    1/1     Running   0          2m    10.244.2.11
web-7d9f8c6b5-fghij    0/1     Running   0          2m    10.244.1.12
web-7d9f8c6b5-klmno    1/1     Running   0          2m    10.244.2.13
web-6b8c7d5f4-oldab    1/1     Running   0          3h    10.244.1.9

$ kubectl describe pod web-7d9f8c6b5-fghij
Readiness probe failed: HTTP probe failed with statuscode: 500

$ kubectl get endpointslices -l kubernetes.io/service-name=web -o wide
NAME         ENDPOINTS
web-abc12    10.244.2.11,10.244.2.13
```

**Interpretation:** the Service only routes to ready Pods. Two new Pods are ready, one is not. The 503 is not "the app is down"; it is the ingress/Service seeing fewer ready backends and possibly a rollout stuck at `maxUnavailable`.

**Hypothesis:** the new Pod cannot reach a dependency (database/cache) or its readiness path is failing due to config.

**Test:** `kubectl logs web-...-fghij`, `kubectl exec` to the Pod and `curl localhost:8080/ready`, check the ConfigMap diff and the dependency's health. If readiness returns 500 because the DB connection pool is exhausted, the rollout exposed a capacity problem, not a code bug.

**Fix:** roll back (`kubectl rollout undo deploy/web`) to restore capacity, then fix the new version's readiness/dependency behavior. **Verify:** all three new Pods `1/1`, EndpointSlice has three IPs, 503 rate returns to zero. **Prevent:** readiness should not fail permanently on a transient dependency; add a startupProbe and a preStop drain; alert on ready-replica count, not just Pod phase.

The point: the 503 was a symptom three layers above the actual failing readiness probe. The evidence chain — Deployment status → Pod readiness → EndpointSlice — localized it without guessing.

---

## 7. Dangerous commands and safer alternatives

| Dangerous | Why | Safer |
|---|---|---|
| `kubectl delete pod` first | Destroys logs/events/exit code | Capture `describe`, `logs --previous`, then delete |
| `kubectl delete node` | Orphans Pods, loses scheduling state | Fix kubelet/runtime; let it re-register |
| `kubectl drain --force --ignore-daemonsets` on a whim | Evicts workloads, ignores PDBs with `--force` | Plan maintenance, use PDBs, drain normally |
| `kubectl edit` in production | Untracked drift, no review | Change the manifest in Git, apply, diff |
| `kubectl apply -f` with an old file | Reverts fields/rolls back config | `kubectl diff` first |
| `kubectl exec` and modify files | Lost on restart, drift | Build a new image/config |
| `kubectl delete pvc` | Deletes data with `Delete` reclaim | Snapshot/backup, verify, then delete |
| `kubectl delete namespace` | Deletes everything, including PVCs | Delete individual objects, verify |
| `kubectl port-forward` as a fix | Bypasses Service/policy, not durable | Fix the Service/Ingress |
| `kubectl scale --replicas=0` to "reset" | Causes an outage | Roll back or fix readiness |
| `--force --grace-period=0` | Skips graceful termination, risks corruption | Use only for a truly stuck Pod, after evidence |

---

## 8. Incident write-up template

```markdown
# INC-<id> — <one-line symptom>

## Impact
- Who/what was affected:
- Start/end time and duration:
- Customer-visible symptom:
- Scope (namespace, cluster, region):

## Timeline (UTC)
| Time | Event | Evidence |
|---|---|---|
| | | |

## Symptoms
- What was observed (status codes, Pod states, alerts):

## Evidence
- `kubectl get/describe` output (trimmed):
- Logs (with timestamps):
- Metrics/graphs:
- Node/runtime output:

## Hypothesis and tests
| Hypothesis | Test | Result |
|---|---|---|
| | | |

## Root cause
- The mechanism, not "human error":
- Why it was not caught earlier:

## Fix
- Immediate mitigation:
- Permanent fix:

## Verification
- The exact check that proved recovery:
- Time recovered:

## Prevention
- Detection:
- Automation/test:
- Runbook update:
- Owner and due date:
```

The difference between a junior and a senior write-up is the **evidence table** and the **mechanism**. "The Pod was OOMKilled because the limit was 256Mi and the new cache allocated 400Mi; the readiness probe restarted it, causing a 30% 503 rate" is a mechanism. "We restarted it and it worked" is not.

---

## 9. Command field manual

### `kubectl get --raw`

```bash
kubectl get --raw='/readyz?verbose'
kubectl get --raw='/livez?verbose'
kubectl get --raw='/metrics'
```

**Purpose:** query the API server's own endpoints, bypassing resource RBAC (but not authentication). `readyz` names the failing component; `/metrics` exposes API latency and request counts.

**Mistake:** treating a `readyz` failure as an etcd failure; read the individual check names.

### `kubectl events`

```bash
kubectl events -A --sort-by=.lastTimestamp
kubectl events --for pod/web-abc
```

**Why:** newer, more reliable than `kubectl get events` for sorting/filtering; events expire, so capture them.

### `kubectl describe`

Already covered; the key is reading `Events` and `Conditions`, not just `Status`.

### `kubectl logs --previous`

Covered; the single most important command for CrashLoopBackOff.

### `kubectl debug`

```bash
kubectl debug -it web-abc --image=nicolaka/netshoot --target=app
kubectl debug node/node1 -it --image=busybox
```

**Why:** ephemeral containers for distroless workloads; node debug for host-level inspection. Node debug mounts the host at `/host` and is privileged — treat it as root on the node.

### `crictl`

```bash
sudo crictl ps -a
sudo crictl inspect <id> | jq '.status'
sudo crictl logs <id>
sudo crictl pods
```

**Why:** the node's CRI view when the API is unavailable or the kubelet is confused. It shows the actual sandbox/container state and exit codes.

### `journalctl -u kubelet`

```bash
sudo journalctl -u kubelet -n 100 --no-pager
sudo journalctl -u kubelet -f | grep -i -E 'error|fail|evict'
```

**Interpretation:** kubelet logs contain image pull, CNI, CSI, probe, eviction, and certificate errors. The API may only show a generic `ContainerCreating`.

### `etcdctl`

```bash
sudo etcdctl endpoint health --cluster
sudo etcdctl endpoint status --cluster -w table
```

**Why:** distinguishes an API server problem from an etcd/quorum problem. `no leader` is a quorum/network/disk issue, not a kubectl issue.

### `kubectl auth can-i`

Covered in note 4; use it before changing RBAC.

---

## 10. Rapid drills

For each, write the **fault domain** and the **first three commands**. Answers follow.

1. Every Pod in the cluster is `Pending`, and `kubectl get nodes` shows all `NotReady`.
2. One Deployment's new Pods are `ImagePullBackOff`; old Pods are fine.
3. A Pod is `Running`, `0/1 Ready`, and the Service has no endpoints.
4. A Pod is `CrashLoopBackOff` after a ConfigMap change.
5. A Pod is `OOMKilled` every 10 minutes under load.
6. `kubectl` returns `Unable to connect to the server: dial tcp ... i/o timeout`.
7. One node is `NotReady`; Pods on it are `Unknown`.
8. A Service works from one namespace but not another.
9. A PVC is `Pending`; the Pod is `Pending`.
10. A Pod is `Running` but DNS returns NXDOMAIN for a Service that exists.
11. An Ingress returns 502 while the Pod is Ready.
12. A node reports `DiskPressure` and Pods are `Evicted`.
13. `kubectl logs` is forbidden, but `kubectl get pods` works.
14. A rollout is stuck with old Pods serving and new Pods `0/1`.
15. `kubectl top pods` returns `error: Metrics API not available`.
16. A Pod is `ContainerCreating` for 10 minutes; the image is pulled.
17. `kubectl get pods` shows `Terminating` for 20 minutes.
18. A StatefulSet's second replica is Pending; the first is Running.
19. A node has `PIDPressure`.
20. After a certificate rotation, kubelets cannot connect to the API server.

### Drill answers

1. Node/control-plane: `kubectl get nodes -o wide`, `kubectl describe node`, `kubectl get --raw='/readyz?verbose'`; then kubelet/runtime logs on a node. All NotReady usually means a shared dependency (network, DNS, certificate, control plane), not 3 independent node failures.
2. Registry/auth: `kubectl describe pod`, `kubectl get secret`, `kubectl logs` (pull events); check image/tag, `imagePullSecrets`, registry reachability.
3. Readiness/endpoints: `kubectl describe pod`, `kubectl get endpointslices`, `kubectl logs`; check probe path/port and dependencies.
4. Config: `kubectl describe pod`, `kubectl logs --previous`, `kubectl get configmap -o yaml`; compare keys/values and restart behavior.
5. Memory limit/leak: `kubectl describe pod` (last state), `kubectl top pod`, app memory profile; limit vs actual usage.
6. API connectivity: `kubectl config current-context`, `kubectl get --raw='/readyz'`, check API server/etcd/LB/certificates; from a node, `curl -k https://API:6443/healthz`.
7. Node: `kubectl describe node`, `journalctl -u kubelet`, `systemctl status containerd`; then `df`, `free`, `dmesg`.
8. Policy/DNS: `kubectl exec` from both namespaces, `getent`, `NetworkPolicy`, CoreDNS logs; compare namespace labels and egress rules.
9. Storage: `kubectl describe pvc`, `kubectl get storageclass`, CSI controller logs; check `WaitForFirstConsumer` and topology.
10. DNS: `cat /etc/resolv.conf`, `nslookup FQDN`, CoreDNS logs/endpoints, NetworkPolicy to 53.
11. Ingress/backend: controller logs, `describe ingress`, EndpointSlice, direct `curl` to Service/Pod; 502 = connection refused/bad response.
12. Disk: `df -h`, `df -i`, `du`, image/log cleanup, ephemeral storage limits; then reschedule.
13. RBAC: `kubectl auth can-i get pods/log -n <ns>`, Role/RoleBinding; add the subresource permission minimally.
14. Readiness/rollout: `rollout status`, `get rs`, `describe pod`, probe/logs; do not delete the old Pods until the new ones are ready.
15. metrics-server: `kubectl -n kube-system get deploy/metrics-server`, `get apiservice`, logs; install/fix it.
16. Volume/CNI: Pod events (`FailedMount`, `FailedAttachVolume`), `describe pvc`, CSI/CNI logs; image is not the problem once pulled.
17. Finalizer/grace: `kubectl get pod -o yaml` (`finalizers`, `deletionGracePeriodSeconds`), `describe`, node/kubelet; a stuck finalizer or unreachable node.
18. StatefulSet/PVC: `describe pod db-1`, `get pvc`, node affinity/zone, ordered startup; `db-0` must be Ready first.
19. PID exhaustion: `kubectl describe node` (`PIDPressure`), node `ps -eLf | wc -l`, `pid_max`, fork bombs/leaks; identify the process.
20. Certificates: kubelet logs (`x509: certificate has expired`), `kubeadm certs check-expiration`, API server logs, clock skew; rotate/renew and restart kubelet.

---

## 11. Checkpoint

1. What are the first three things you check when "the cluster is down"?
2. Why are events time-sensitive?
3. What is the difference between `Pending` and `ContainerCreating`?
4. What does `kubectl logs --previous` show, and why does it matter?
5. What exit code indicates OOM, and where do you confirm it?
6. What is the difference between `OOMKilled` and `Evicted`?
7. What are the four checks for a Service with no endpoints?
8. What does `kubectl get --raw='/readyz?verbose'` tell you?
9. Why should you not delete a NotReady node immediately?
10. What is the first check for `ImagePullBackOff`?
11. What command shows the actual container state on a node?
12. What is the risk of `kubectl drain --force`?
13. Why can `kubectl port-forward` mislead you?
14. What does `Multi-Attach error` mean?
15. What evidence do you need before changing a memory limit?
16. What is the difference between a liveness and readiness failure in production impact?
17. What does `kubectl auth can-i --list` show?
18. What is the safest way to inspect a distroless container?
19. What is the first thing to check when one node is NotReady but the rest are fine?
20. What belongs in an incident write-up that a fix summary usually omits?

### Answers

1. Context/kubeconfig, node status, API `/readyz` (and events); then control plane/etcd if all nodes are affected.
2. Kubernetes events have a TTL (often 1 hour) and are overwritten; after deletion/restart, the evidence is gone.
3. `Pending` means not scheduled; `ContainerCreating` means scheduled but the sandbox/image/volume/network is not ready.
4. Logs from the previous terminated container instance; essential for CrashLoopBackOff where the current attempt has no logs.
5. 137; confirm with `describe pod` (`Last State: Terminated, Reason: OOMKilled`) and cgroup `memory.events`.
6. OOMKilled is the container exceeding its cgroup memory limit; Evicted is the kubelet removing a Pod due to node pressure (disk/memory/PID), with status `Failed`.
7. Selector/labels, readiness, targetPort/port names, EndpointSlice controller/objects.
8. Which API server health checks pass/fail (etcd, admission, etc.), localizing a control plane problem.
9. It may recover and re-register; deleting it can orphan Pods and confuse scheduling. Fix kubelet/runtime first.
10. The exact image/tag and the event message; then `imagePullSecrets`, registry reachability, and architecture.
11. `crictl ps -a` / `crictl inspect` / `crictl logs` on the node.
12. It ignores PDBs and can evict workloads without availability guarantees; use only for a stuck, understood situation.
13. It bypasses the Service/Ingress/NetworkPolicy path, so it can work while real traffic fails.
14. An RWO volume is still attached to another node, usually because a Pod has not terminated or a Deployment shares one PVC across nodes.
15. Actual usage (`top`, metrics, app heap), the limit, the node's pressure, and whether the process is leaking or legitimately needs more.
16. Readiness failure removes the Pod from endpoints (capacity loss); liveness failure restarts the container (possible outage/loop).
17. The effective permissions of the current identity in the current namespace, by verb/resource.
18. `kubectl debug` with an ephemeral container and `--target`, or a privileged node debug Pod; `exec` needs a shell/binary in the image.
19. That node's kubelet/runtime/disk/network, and whether it is a control plane node; compare with healthy nodes.
20. The evidence chain, the mechanism, the detection gap, and the prevention/owner — not just "restarted the Pod".
