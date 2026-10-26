# Phase 4 Checkpoint — Containers and Kubernetes

> **Rules:** closed book, 90 minutes for Parts A–D. No `kubectl`, no notes, no search. Then do the practical sign-off on the kind cluster. Target: **80% overall**, and at least **30/40** on the diagnosis scenarios.
>
> **Assumption:** questions use a modern Kubernetes (1.28+) with containerd, CoreDNS, kube-proxy in iptables mode, and a policy-capable CNI where stated. If your lab used kindnet without policy enforcement, answer the policy questions conceptually and note the limitation.

---

## Part A — Rapid facts (25 × 2 = 50 points)

1. What is the difference between a container and a VM?
2. Name the six classic Linux namespaces and what each isolates.
3. What is the difference between `memory.max` and `memory.high` in cgroup v2?
4. What are the four directories in an overlayfs mount, and what does each do?
5. What is the difference between containerd and runc?
6. What is an OCI image? What is an OCI runtime?
7. What does `CAP_SYS_ADMIN` effectively allow, and why is it dangerous?
8. What does a seccomp profile do?
9. Why is root inside a container not necessarily root on the host?
10. What are the three guarantees of the Kubernetes Pod network model?
11. What does a CNI plugin do, and who calls it?
12. Which component writes to etcd?
13. Why does etcd need an odd number of members?
14. What does the scheduler use to filter nodes?
15. What is the difference between a request and a limit?
16. What are the three QoS classes, and which is evicted first?
17. What does a readiness probe failure do? A liveness probe failure?
18. Why does a Deployment create a ReplicaSet?
19. What does a StatefulSet provide that a Deployment does not?
20. What is an EndpointSlice, and who writes it?
21. What is the difference between a ClusterIP and a headless Service?
22. What does kube-proxy do in iptables mode?
23. What does `ndots:5` do to an external DNS lookup?
24. Is NetworkPolicy enforced by kube-proxy? Explain.
25. What is the difference between a PV, a PVC, and a StorageClass?

---

## Part B — Calculations (6 × 5 = 30 points)

### B1 — Capacity and failure

A cluster has 3 nodes, each with 16 vCPU and 64 GiB allocatable. You want 20 Pods, each requesting 2 vCPU and 4 GiB.

1. Do the requests fit with all 3 nodes healthy? Show totals.
2. If one node fails, can the remaining 2 nodes hold all 20 Pods at their requests?
3. What is the maximum number of Pods per node if you want to survive one node failure at the same requests?
4. Name one reason the scheduler might still leave a Pod Pending even though the aggregate totals fit.

### B2 — HPA

A Deployment has 4 replicas. Each container requests 500m CPU. The HPA target is 70% CPU utilization.

1. What is the total requested CPU?
2. If each Pod uses 400m, what is the utilization, and what does the HPA desire (formula: `ceil(currentReplicas × currentUtilization / targetUtilization)`)?
3. If each Pod uses 200m, what is the desired replica count before `minReplicas` is applied?
4. Why can the HPA report `<unknown>` even when the Pods are running?

### B3 — Endpoints and failure rate

A Service selects 6 Pods. Two fail their readiness probe.

1. How many endpoints does the EndpointSlice contain?
2. If traffic is evenly distributed per connection, what fraction of new connections fails?
3. The two unready Pods are still `Running`. Why are they not endpoints?
4. What would you check first to confirm the readiness failure is real, not a probe misconfiguration?

### B4 — Memory limit

A Java container has a 512 MiB memory limit. The JVM uses `-Xmx384m`, metaspace ~96 MiB, thread stacks ~64 MiB, and native/direct buffers ~40 MiB.

1. What is the approximate total footprint?
2. What happens when it exceeds 512 MiB?
3. What is the correct fix: raise the limit, lower `-Xmx`, or both? Explain.
4. Where would you confirm the kernel OOM event?

### B5 — etcd quorum

A cluster has 3 etcd members.

1. How many failures can it tolerate and still accept writes?
2. What happens with 2 members down?
3. Would 4 members improve fault tolerance? Why or why not?
4. What is the first command you run to check quorum?

### B6 — Storage

A StatefulSet has 12 replicas, each with a 200 GiB PVC.

1. What is the total requested capacity?
2. The StorageClass uses `WaitForFirstConsumer`. When is the first PV provisioned?
3. The class has `reclaimPolicy: Delete`. What happens when you delete a PVC?
4. You scale to 6 replicas. What happens to the PVCs for replicas 6–11?

---

## Part C — Diagnosis scenarios (8 × 5 = 40 points)

For each, name the **fault domain**, the **next two commands**, and the **fix**. Do not guess before naming the evidence.

### C1

```text
$ kubectl -n app get pod web-abc
NAME      READY   STATUS    RESTARTS   AGE
web-abc   0/1     Pending   0          3m

$ kubectl -n app describe pod web-abc
Events:
  Warning  FailedScheduling  ...  0/3 nodes are available: 3 Insufficient cpu.
```

### C2

```text
$ kubectl -n app get pod api-0
NAME    READY   STATUS                       RESTARTS
api-0   0/1     CreateContainerConfigError   0

$ kubectl -n app describe pod api-0
Events:
  Warning  Failed  ...  configmap "api-config" not found
```

### C3

```text
$ kubectl -n app get pod worker-0
NAME       READY   STATUS             RESTARTS
worker-0   0/1     CrashLoopBackOff   6

$ kubectl -n app describe pod worker-0
Last State:  Terminated
  Reason:    OOMKilled
  Exit Code: 137
```

### C4

```text
$ kubectl -n app get svc web
NAME   TYPE        CLUSTER-IP     PORT(S)
web    ClusterIP   10.96.142.31   80/TCP

$ kubectl -n app get endpointslices -l kubernetes.io/service-name=web
NAME        ADDRESSTYPE   PORTS   ENDPOINTS
web-abc12   IPv4          8080    <unset>

$ kubectl -n app get pods --show-labels
NAME      READY   STATUS    LABELS
web-abc   1/1     Running   app=web-v2
```

### C5

```text
$ kubectl -n team-a run dns --rm -it --image=busybox:1.36 -- nslookup api.team-b.svc.cluster.local
Server:    10.96.0.10
Address:   10.96.0.10:53

** server can't find api.team-b.svc.cluster.local: NXDOMAIN
```

### C6

```text
$ kubectl -n app describe pod api-0
Events:
  Warning  Failed  ...  Failed to pull image "ghcr.io/acme/api:1.0.0":
  rpc error: code = Unknown desc = ... unauthorized
```

### C7

```text
$ kubectl get nodes
NAME     STATUS     ROLES           AGE
node1    NotReady   control-plane   10d
node2    Ready      <none>          10d

$ kubectl -n app get pods -o wide
NAME    READY   STATUS    NODE
api-0   0/1     Unknown   node1
```

### C8

```text
$ kubectl -n app get pvc
NAME   STATUS    VOLUME   CAPACITY   ACCESS MODES   STORAGECLASS
data   Pending                                      fast-missing

$ kubectl -n app get pod db-0
NAME   READY   STATUS    RESTARTS
db-0   0/1     Pending   0
```

---

## Part D — Design and explain (2 × 20 = 40 points)

### D1 — Multi-team namespace design (20 points)

Design a namespace model for three teams (`payments`, `search`, `platform`) on one cluster.

Requirements:

- Each team can deploy and debug its own workloads.
- Teams cannot read each other's Secrets.
- Each team has a CPU/memory quota.
- The `platform` team can read Pods/logs in all namespaces but cannot delete workloads.
- Ingress is shared; each team exposes one HTTP service.
- Storage is block-backed with per-replica PVCs for stateful services.
- One team's runaway workload must not starve the others.

Answer:

1. Namespaces and RBAC objects (be specific about Role/ClusterRole and bindings).
2. Quotas and LimitRanges, with example numbers.
3. NetworkPolicy model: default deny, DNS, ingress controller, cross-team access.
4. StorageClass and PVC model for stateful workloads.
5. The top three ways this design can fail operationally, and the monitoring signal for each.

### D2 — Zero-downtime rollout (20 points)

A Deployment serves 6 replicas behind a Service and an Ingress. You need to ship a new image with no 5xx errors.

Answer:

1. The Deployment strategy fields and values you would set, and why.
2. The probes you would configure, and what each checks.
3. The role of `preStop`, `terminationGracePeriodSeconds`, and SIGTERM handling.
4. How a PDB and HPA interact with the rollout.
5. A failure mode that still causes 5xx despite a "successful" rollout, and how you would detect it.

---

## Answer key

### Part A

1. A container is a process tree isolated by namespaces and bounded by cgroups, sharing the host kernel. A VM has its own kernel and virtual hardware, managed by a hypervisor. Containers are lighter and faster; VMs provide stronger isolation and a different OS.
2. `mnt` (mount table/rootfs), `pid` (PID numbers), `net` (interfaces/routes/ports), `uts` (hostname), `ipc` (SysV/POSIX IPC), `user` (UID/GID mapping and capabilities). `cgroup` and `time` also exist in modern kernels.
3. `memory.max` is the hard limit; exceeding it invokes the cgroup OOM killer. `memory.high` is a throttle/reclaim threshold; the kernel applies pressure but does not necessarily kill.
4. `lowerdir` read-only image layers; `upperdir` writable changes; `workdir` overlayfs scratch on the same filesystem as upper; `merged` the combined view presented to the container.
5. containerd is a daemon that pulls images, manages snapshots, and supervises container lifecycle; runc is the OCI runtime that creates namespaces/cgroups/mounts and execs the process, then exits.
6. An OCI image is a manifest + config + layer blobs addressed by digest; an OCI runtime is the low-level executor (runc/crun) that turns an unpacked rootfs + config into a running process.
7. It bypasses most privilege checks: mount, namespace manipulation, device access, kernel keyring, and more. A container with it can often escape or damage the host.
8. It filters syscalls, returning an error or killing the process for disallowed calls (e.g. `mount`, `ptrace`, `bpf`). It reduces the kernel attack surface.
9. User namespaces map container UID 0 to an unprivileged host UID; without them, UID 0 in the container is UID 0 on the host (subject to capabilities/LSM).
10. Every Pod has a unique IP; any Pod can reach any Pod IP without NAT; nodes can reach Pod IPs and vice versa.
11. It configures the Pod's network namespace (veth, IP, routes, policy) per the CNI spec; the container runtime calls it when creating the Pod sandbox.
12. `kube-apiserver` is the only component that talks to etcd; controllers/kubelets use the API.
13. Raft requires a majority quorum; 3 tolerates 1 failure, 5 tolerates 2. An even number does not increase fault tolerance and can split evenly.
14. Resource requests, node selectors/affinity, taints/tolerations, volume topology, host ports, Pod affinity/anti-affinity, and unschedulable nodes.
15. A request is used for scheduling and is the guaranteed share; a limit is the runtime ceiling (CPU throttling, memory OOM).
16. Guaranteed, Burstable, BestEffort; BestEffort is evicted first.
17. Readiness failure removes the Pod from Service endpoints; liveness failure restarts the container.
18. The Deployment manages versions/rollouts; the ReplicaSet maintains a stable replica count for one Pod template, enabling history and rollback.
19. Stable network identity, ordered lifecycle, and per-replica PVCs.
20. A slice of Service endpoints with readiness and ports; the EndpointSlice controller writes it from the Service selector and Pod readiness.
21. ClusterIP has a virtual IP and kube-proxy load balancing; headless has no virtual IP and DNS returns Pod IPs for client-side balancing.
22. It watches Services/EndpointSlices and programs NAT rules (DNAT to endpoints) plus conntrack; it does not proxy packets in the data path.
23. Names with fewer than 5 dots are tried with each search domain first, causing extra queries and latency before the absolute name.
24. No; NetworkPolicy is enforced by the CNI at the Pod network boundary. kube-proxy implements Services, not policy. Without CNI support, the policy is inert.
25. A PV is the cluster-wide storage resource; a PVC is a namespaced request; a StorageClass defines the provisioner/parameters/binding mode used to create a PV dynamically.

### Part B

**B1**

1. Requests: `20 × 2 = 40 vCPU`, `20 × 4 = 80 GiB`. Capacity: `3 × 16 = 48 vCPU`, `3 × 64 = 192 GiB`. Fits in aggregate.
2. Remaining 2 nodes: `32 vCPU`, `128 GiB`. Requests need `40 vCPU`, `80 GiB`. CPU does not fit; 8 vCPU short. Memory fits.
3. To survive one node failure, each node must be able to host all 20 Pods? No: the remaining 2 nodes must hold 20 Pods, so per node `20/2 = 10` Pods = 20 vCPU, which exceeds 16. The largest Pod count that survives one failure is `2 × 16 / 2 = 16` Pods total (8 per node), or add capacity. Another way: with 3 nodes, each can host at most 8 Pods at 2 vCPU, total 24, but failure leaves 16; to run 20 after failure, need 10 per node → 20 vCPU per node, so each node needs at least 20 vCPU.
4. Fragmentation, taints/affinity, a single large Pod, hostPort conflicts, PVC topology, quota, or a node that is cordoned. Aggregate capacity does not guarantee a feasible placement.

**B2**

1. `4 × 500m = 2000m = 2 CPU` requested.
2. Utilization = `400/500 = 80%`. Desired = `ceil(4 × 80/70) = ceil(4.57) = 5`.
3. Utilization = `200/500 = 40%`. Desired = `ceil(4 × 40/70) = ceil(2.29) = 3`; with `minReplicas` it may stay at the minimum.
4. Metrics-server is missing/unhealthy, the containers lack CPU requests, or the metrics API is unavailable; utilization is undefined.

**B3**

1. Four ready endpoints.
2. `2/6 = 33.3%` of new connections, assuming even distribution and no retry.
3. Readiness failure removes them from endpoints; `Running` only means the process is up, not that it can serve traffic.
4. Check the probe path/port and the app's `/ready` response; `kubectl describe pod` shows the status code. A 404 is a probe/config mismatch, not necessarily an app failure.

**B4**

1. `384 + 96 + 64 + 40 = 584 MiB`, already above 512 MiB, before other native allocations.
2. The cgroup OOM killer kills a process in the container; the container exits 137 and restarts, likely in a loop.
3. Both: lower `-Xmx` so the total fits, and raise the limit only if the node has capacity and the workload genuinely needs it. The JVM's non-heap memory must be included in the limit.
4. `kubectl describe pod` (`OOMKilled`), cgroup `memory.events` (`oom_kill`), and node `dmesg`/`journalctl -k`.

**B5**

1. One failure; 2 of 3 members still form a quorum.
2. No quorum; the API becomes read-only for writes (existing workloads may keep running, but no scheduling/changes).
3. No; 4 members require 3 for quorum and tolerate only 1 failure, same as 3, with more overhead and no benefit. Use 3 or 5.
4. `etcdctl endpoint health --cluster` and `endpoint status --cluster -w table`.

**B6**

1. `12 × 200 GiB = 2400 GiB = 2.34 TiB`.
2. When the first Pod using the PVC is scheduled; the provisioner creates the PV in the Pod's topology.
3. The PV and backing volume are deleted. Data is gone unless backed up elsewhere.
4. They are retained by default; scaling down does not delete PVCs. You must delete them explicitly after verifying the data is no longer needed.

### Part C

**C1 — Scheduling/capacity**

- **Fault domain:** scheduler/capacity (requests).
- **Next commands:** `kubectl describe node <node> | sed -n '/Allocated resources/,/Events/p'`; `kubectl get pods -A -o wide`; `kubectl get resourcequota -n app`.
- **Fix:** lower the Pod's CPU request if it is inflated, add nodes/capacity, or remove a constraint. Do not delete the Pod; it will return Pending.

**C2 — Configuration**

- **Fault domain:** missing ConfigMap/key (admission/kubelet config).
- **Next commands:** `kubectl -n app get configmap`; `kubectl -n app get pod api-0 -o yaml | grep -A10 env`.
- **Fix:** create the ConfigMap/key or correct the reference. The container never started, so app logs do not exist.

**C3 — Memory limit**

- **Fault domain:** container cgroup memory limit.
- **Next commands:** `kubectl -n app describe pod worker-0 | grep -A6 'Last State'`; `kubectl -n app top pod worker-0`; check the app's heap/limit.
- **Fix:** reduce usage or raise the limit after confirming the node has capacity; add a memory profile/alert. Restarting alone will loop.

**C4 — Service selector/endpoints**

- **Fault domain:** Service selector/labels (and possibly readiness).
- **Next commands:** `kubectl -n app get svc web -o yaml | sed -n '/selector/,/ports/p'`; `kubectl -n app get pods --show-labels`; `kubectl -n app get endpointslices -l kubernetes.io/service-name=web -o yaml`.
- **Fix:** make the Service selector match `app=web-v2` (or relabel the Pods). The app is Ready; the Service is selecting nothing.

**C5 — DNS/name/namespace**

- **Fault domain:** DNS name/namespace (or CoreDNS/policy).
- **Next commands:** `kubectl -n team-b get svc`; `kubectl -n kube-system logs deploy/coredns --tail=50`; test the FQDN from a Pod in `team-b`.
- **Fix:** correct the Service name/namespace; if the name is right, check CoreDNS endpoints and NetworkPolicy egress to DNS. NXDOMAIN means the name does not exist from the resolver's perspective.

**C6 — Registry/auth**

- **Fault domain:** image registry authentication/authorization.
- **Next commands:** `kubectl -n app get pod api-0 -o yaml | grep -A5 imagePullSecrets`; `kubectl -n app get secret <name> -o jsonpath='{.data.\.dockerconfigjson}' | base64 -d`; check registry credentials/expiry.
- **Fix:** create/attach a valid imagePullSecret in the namespace, or fix the registry permissions. `unauthorized` is not a network timeout.

**C7 — Node**

- **Fault domain:** node/kubelet/runtime/network.
- **Next commands:** `kubectl describe node node1`; on node1 `systemctl status kubelet`, `journalctl -u kubelet -n 100`, `systemctl status containerd`, `df -h`.
- **Fix:** restore kubelet/runtime/disk/network; the Pods become Unknown because the control plane cannot confirm their state. Do not delete the Node object first.

**C8 — Storage provisioning**

- **Fault domain:** StorageClass/provisioner.
- **Next commands:** `kubectl get storageclass`; `kubectl -n app describe pvc data`; CSI controller logs.
- **Fix:** create the missing StorageClass or change the PVC to an existing class. If `WaitForFirstConsumer`, a Pending PVC before scheduling is normal; here the class itself is missing.

### Part D

**D1 — Example design**

1. **Namespaces/RBAC:**
   - `payments`, `search`, `platform` namespaces.
   - In each team namespace, a `Role` allowing `get/list/watch/create/update/patch/delete` on workloads, ConfigMaps, Services, PVCs, and `get` on Pods/logs; bind to the team Group via `RoleBinding`. Do not grant Secrets cluster-wide; use per-namespace `get` on Secrets only if the team owns them.
   - `platform` gets a `ClusterRole` with `get/list/watch` on Pods, Pods/logs, Events, and Nodes, bound with a `ClusterRoleBinding`; no delete/update on workloads.
   - A `RoleBinding` for the ingress controller's ServiceAccount in each namespace if it needs to read Endpoints/Services.
   - Avoid `cluster-admin`; use `kubectl auth can-i --list` to verify.
2. **Quotas/LimitRanges:** e.g. `payments` 20 vCPU/40Gi requests, 40 vCPU/80Gi limits; `search` 10/20; `platform` 5/10. Default container requests 100m/128Mi, limits 500m/512Mi; max 2 vCPU/2Gi. Quotas prevent one team from consuming the cluster; LimitRanges prevent unbounded Pods.
3. **NetworkPolicy:** default-deny ingress and egress in each namespace; allow DNS egress to `kube-system`; allow ingress from the ingress controller namespace to the team's Service ports; allow explicit cross-team flows (e.g. `search` → `payments` API) by namespace+pod selector. Test enforcement with the CNI, not just the object.
4. **Storage:** a block StorageClass with `WaitForFirstConsumer`, `allowVolumeExpansion: true` (if the driver supports it), and `Retain` for stateful data or `Delete` plus backups for disposable data. StatefulSets use `volumeClaimTemplates`; each replica gets its own PVC. Do not share one RWO PVC across replicas.
5. **Failure modes:** (a) quota exhaustion → alert on `kube_resourcequota` used/hard; (b) a noisy neighbor at the node level despite namespace quotas → alert on CPU throttling, node pressure, and Pod eviction; (c) policy misconfiguration blocking DNS/ingress → synthetic probes and CoreDNS/ingress error rates. Add a fourth: storage capacity exhaustion → PV/PVC usage and CSI errors.

**D2 — Example rollout**

1. `strategy: RollingUpdate`, `maxSurge: 1`, `maxUnavailable: 0`; with 6 replicas, one new Pod at a time, and no reduction in ready capacity. `minReadySeconds` to avoid marking a flapping Pod ready immediately. `progressDeadlineSeconds` to fail the rollout instead of hanging forever.
2. `startupProbe` for slow initialization (disables liveness/readiness until success), `readinessProbe` for traffic eligibility (dependency checks, warm caches), `livenessProbe` for process health only. Readiness must fail during shutdown/draining, not just on startup.
3. `preStop` sleeps/drains in-flight requests (or calls the app's shutdown endpoint); `terminationGracePeriodSeconds` must exceed the preStop + drain time; the app handles SIGTERM by stopping acceptance, finishing requests, then exiting. If it ignores SIGTERM, it gets SIGKILL and in-flight requests fail.
4. A PDB with `minAvailable: 4` (or `maxUnavailable: 2`) prevents drain/eviction from taking too many replicas; HPA can add replicas during the rollout, but it does not guarantee capacity. Ensure the HPA's `minReplicas` is at least the PDB's `minAvailable` plus surge.
5. A failure mode: the new Pods pass readiness but have a latent bug that only appears under production traffic (e.g. a cache stampede, a DB migration incompatibility, or a slow dependency). The rollout is "successful" by Kubernetes' definition, but 5xx rises. Detect with canary/error-rate SLO, latency percentiles, and per-version metrics; roll back on the SLO burn rate, not on Pod status alone. Another: the old Pods are terminated before the new ones are actually ready due to a bad readiness probe; `maxUnavailable: 0` plus a correct readiness gate prevents it.

---

## Scoring

| Section | Points | Your score | Target |
|---|---:|---:|---:|
| A — facts | 50 | | ≥ 40 |
| B — calculations | 30 | | ≥ 24 |
| C — diagnosis | 40 | | ≥ 32 |
| D — design | 40 | | ≥ 32 |
| **Total** | **160** | | **≥ 128 (80%)** |

If you score below 80%, do not move to Phase 5. Re-read the specific note, redo the matching lab (L4.1–L4.7), and retake after 48 hours. A conceptual miss in networking or storage is a prerequisite gap; fix it before adding cloud complexity.

---

## Practical sign-off

Do these on the kind cluster without notes. Each is pass/fail.

- [ ] Build a container by hand with `unshare`, a cgroup limit, and overlayfs; explain each namespace.
- [ ] Run a container, inspect its namespaces/cgroup, and prove the veth/bridge/NAT path with `tcpdump`.
- [ ] Create a kind cluster, deploy a workload, and trace Deployment → ReplicaSet → Pod → node → container.
- [ ] Break a Service selector and prove the EndpointSlice is empty; fix it.
- [ ] Resolve a Service by short name, FQDN, and headless name; explain the search path.
- [ ] Create a NetworkPolicy default-deny and a minimal allow; determine whether your CNI enforces it.
- [ ] Provision a PVC, persist data across Pod replacement, and explain the reclaim policy.
- [ ] Run a StatefulSet with per-replica PVCs and stable DNS.
- [ ] Force Pending via requests, taints, and node selector; fix each from events.
- [ ] Create an OOMKilled Pod and read the cgroup evidence.
- [ ] Prove RBAC with `auth can-i` before and after adding `pods/log`.
- [ ] Drain a worker, observe rescheduling and PDB behavior.
- [ ] Simulate NotReady (pause a node) and explain what stays up.
- [ ] Diagnose all five capstone failures from evidence.

---

## Oral exam (12 minutes)

Answer out loud, without notes. Record yourself.

1. "What is a container, and what does Docker add beyond namespaces and cgroups?"
2. "Walk me through `kubectl apply -f deployment.yaml` until the Pod is Running."
3. "A Pod is Pending. What are the first five things you check?"
4. "A Service resolves but connections fail. Walk me through the packet path."
5. "Why can a Pod be Running but not Ready, and why does that matter?"
6. "Explain the difference between a request, a limit, and a QoS class."
7. "Why does a StatefulSet need a headless Service?"
8. "A node is NotReady. What do you check, and what happens to its Pods?"
9. "How would you prove an RBAC failure rather than an application failure?"
10. "Tell me about a failure you caused in these labs and how you found the cause."

If you use a term you cannot define (`endpoint`, `QoS`, `finalizer`, `CNI`, `reclaim policy`), stop and define it.

---

## Retest log

| Date | A | B | C | D | Total | Weak areas | Remediation | Retest date |
|---|---:|---:|---:|---:|---:|---|---|---|
| | | | | | | | | |
| | | | | | | | | |

When you pass, continue to [Phase 5 — Cloud and AWS](../05-cloud-aws/README.md).
