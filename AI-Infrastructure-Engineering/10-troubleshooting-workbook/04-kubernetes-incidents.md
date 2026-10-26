# Kubernetes Incidents (INC-08 – INC-11)

> **Evidence first.** Write your timeline, hypotheses, and discriminating test before the analysis marker. For Kubernetes, the canonical diagnostic order is: **events → pod spec → node state → controller logs → data plane**. Events are the scheduler's and kubelet's honest testimony; read them before forming theories.

---

## INC-08 — Pods Pending on nodes with plenty of free CPU

### The page

> **03:40 SEV2** — A new batch deployment of 6 replicas is Pending on a 12-node cluster. Nodes report ~55% CPU usage. The on-call previously "fixed" this by deleting the pods, which worked for a few minutes, then Pending returned.

### The evidence (raw, as collected)

```bash
kubectl get pods -n batch | grep pend
kubectl describe pod -n batch batch-7d9f-x1 | sed -n '/Events/,$p'
kubectl get nodes -o custom-columns=\
NAME:.metadata.name,CPU_ALLOC:.status.allocatable.cpu
kubectl describe nodes | sed -n '/Allocated resources/,/Events/p' | head -60
kubectl get pods -A -o json | jq -r '
  .items[] | select(.status.phase=="Running") |
  [.metadata.namespace, .metadata.name, .spec.nodeName,
   ([.spec.containers[].resources.requests.cpu // "0"] | add)] | @tsv' | sort
```

```text
Events:
  Warning  FailedScheduling  3m  default-scheduler
    0/12 nodes are available: 12 Insufficient cpu.

$ kubectl describe nodes (one node, abridged)
Allocated resources:
  Resource    Requests     Limits
  cpu         3900m (97%)  4800m (120%)
  memory      7.2Gi (72%)  9Gi (90%)
```

Per-node: every node has `requests.cpu` at 90–100%, while actual usage (from `kubectl top`) is ~55%.

### Your turn (before the line)

- [ ] Timeline.
- [ ] Three hypotheses ranked with evidence.
- [ ] The one discriminating test.
- [ ] First 10 minutes.

---
---

# ANALYSIS — INC-08

### Timeline

Over the previous weeks, teams gradually raised CPU **requests** (a scaling exercise set generous defaults). Requests across the cluster reached ~97% of allocatable on every node while actual usage stayed near 55%. The new batch pods' requests could not fit anywhere, so the scheduler marked `Insufficient cpu` on all 12 nodes. Deleting pods released capacity briefly — which is why the "fix" appeared to work — but the rescheduled old pods re-claimed their requests within minutes and the new pods went Pending again.

### Hypotheses ranked

1. **Request overcommit: the cluster's CPU *requests* are saturated even though usage is not.** The `Allocated resources` section shows requests at 97%+ everywhere; the scheduler schedules on requests, not usage. Matches exactly.
2. **Actual CPU saturation.** Rejected: `kubectl top` shows ~55%.
3. **Taints/affinity blocking the batch nodes.** Rejected: the event text says `Insufficient cpu`, not untolerated taints or affinity failures.

### The discriminating test

Compare the scheduler's view (`requests`) with reality (`kubectl top`): requests at ~97% while usage is ~55% proves the cluster is **allocation-saturated, not load-saturated**. The event text `Insufficient cpu` names the resource; the allocated-resources table names the cause.

### Root cause

CPU requests were set far above actual usage across the fleet, consuming the scheduling budget. The scheduler will not overcommit requests (by design; that is what `limits` are for). Nothing was broken — the cluster was honestly full by the rules it enforces.

### Fix (smallest safe first)

1. Find the biggest request/usage gaps and right-size them:

```bash
kubectl get pods -A -o json | jq -r '
  .items[] | select(.status.phase=="Running") |
  [.metadata.namespace, .metadata.name,
   ([.spec.containers[].resources.requests.cpu // "0"] | add),
   ([.status.containerStatuses[0].resources] | length)] | @tsv' | sort -k3 -rn | head
```

   Cross-check with `kubectl top pods -A` and reduce requests to P95 of observed usage over two weeks (VPA in recommendation mode does this analysis for you).
2. For the batch pods, either reduce their requests to what they actually use, or move them to a node pool with lower packing.
3. Only as a last resort: add nodes — but if requests were 97% and usage 55%, adding nodes buys expensive idleness.

### Prevention

- Alert on `requests.cpu` utilization per node (>85% warning) **and** on Pending pods with `Insufficient` reasons.
- Set requests from measured P95, not from peak fear; use VPA in recommendation mode to review monthly.
- Record the difference between `requests`, `limits`, and `top` in the team runbook — this confusion causes an incident roughly once per team.
- Capacity planning for batch: separate node pool with a known request/usage ratio.

### Lessons

- **The scheduler schedules on requests, not usage.** "Free CPU" on a monitoring graph is not the same as "allocatable minus requests."
- **Deleting pods is not a fix** — it releases allocation briefly, which is why it feels like it works, and then fails again.
- **The event text names the resource; the allocated table names the cause.** Read both.

### 5-minute walkthrough script

> "Six batch pods were Pending while node graphs showed 55% CPU. The scheduler event said `Insufficient cpu` on all 12 nodes — that names the resource, and the allocated-resources table named the cause: CPU requests were at 97% of allocatable on every node while actual usage was 55%. Requests had drifted upward over weeks of generous defaults; the scheduler refuses to overcommit requests by design. Someone had deleted the pods as a fix — that releases the allocation briefly, which is why it seemed to work. The fix was right-sizing requests to observed P95 (VPA recommendations as the source) and moving batch to a pool with a known packing ratio. Prevention: alert on request saturation and Pending pods with Insufficient reasons. The lesson: the scheduler lives in the requests world, monitoring lives in the usage world — when they disagree, it's a capacity-accounting incident."

---

## INC-09 — CrashLoopBackOff after a routine deploy

### The page

> **12:15 SEV1** — After a routine deploy of `checkout` (a config change), all pods enter CrashLoopBackOff. Rollback is available but the deploy is "just config," so the team wants to understand before rolling back.

### The evidence (raw, as collected)

```bash
kubectl get pods -n checkout
kubectl describe pod -n checkout checkout-66d8-x2 | sed -n '/State\|Last State\|Events/,$p'
kubectl logs -n checkout checkout-66d8-x2 --previous --tail=20
kubectl get configmap checkout-config -n checkout -o yaml | head -20
kubectl get secret checkout-secrets -n checkout -o json | jq -r '.data | keys[]'
kubectl get deployment checkout -n checkout -o jsonpath='{.spec.template.spec.containers[0].envFrom}'
```

```text
$ kubectl get pods -n checkout
NAME                    READY   STATUS             RESTARTS   AGE
checkout-66d8-x1        0/1     CrashLoopBackOff   6          12m
checkout-66d8-x2        0/1     CrashLoopBackOff   6          12m

$ kubectl logs --previous --tail=20
INFO  loading configuration from env
ERROR configuration error: required environment variable PAYMENT_API_KEY is not set
FATAL exiting: config validation failed

$ kubectl get secret checkout-secrets -o json | jq -r '.data | keys[]'
payment_api_key

$ kubectl get deployment checkout -o jsonpath='{.spec.template.spec.containers[0].envFrom}'
[{"secretRef":{"name":"checkout-secrets","optional":false}}]
```

The env var name in the app's config loader is `PAYMENT_API_KEY`; the secret's key was renamed to `payment_api_key` in the same PR ("consistency rename").

### Your turn (before the line)

- [ ] Timeline.
- [ ] Three hypotheses ranked with evidence.
- [ ] The one discriminating test.
- [ ] First 10 minutes.

---
---

# ANALYSIS — INC-09

### Timeline

The PR renamed the secret's key from `PAYMENT_API_KEY` to `payment_api_key` for style consistency, assuming `envFrom` maps keys case-sensitively to env names and everything would "just work." The app's config loader expects `PAYMENT_API_KEY` and fails validation at startup, exiting 1. Kubernetes restarts the container; after several fast failures it applies the CrashLoopBackOff delay. The rollback is available, and after diagnosis it is the right immediate action.

### Hypotheses ranked

1. **Config contract break: the app expects a key the secret no longer provides.** `--previous` logs show the exact missing variable; the secret's keys are visible; the env name does not match. Matches.
2. **Bad image / runtime crash.** Rejected: the failure is a config validation error with a clean message, not a segfault or missing binary.
3. **Secret not mounted / RBAC.** Rejected: `envFrom` resolves the secret fine (otherwise the pod would be stuck in `ContainerCreating` or `CreateContainerConfigError`, not CrashLoop).

### The discriminating test

`kubectl logs --previous` gives the app's own error, and `kubectl get secret ... | jq -r '.data | keys[]'` gives the actual keys. Comparing the two sets is the test: the app requires `PAYMENT_API_KEY`; the secret provides `payment_api_key`.

### Root cause

A rename of a secret key broke the configuration contract between the secret and the app's config schema. The rename passed review because "it's just a naming change," and no test asserted the set of required keys.

### Fix (smallest safe first)

1. **Roll back the deploy** (or add a compatibility key). Rollback first, then fix forward — the team's own rule ("understand before rolling back") has been satisfied by the diagnosis above.
2. Fix forward by adding `PAYMENT_API_KEY: payment_api_key` mapping via `env:` + `valueFrom.secretKeyRef`, or rename the app's expected key and **deploy both in one atomic PR** (secret key + app schema), never as two steps.
3. Restart and watch one pod reach Ready before rolling the rest out.

### Prevention

- A config schema test in CI: start the app with the real secret shape (or a fixture) and assert it boots. This is the "golden input" test for configuration.
- Never rename a secret key and change app code in separate PRs.
- Prefer `env[].valueFrom.secretKeyRef` (explicit mapping) over `envFrom` (implicit bulk) for anything the app requires; implicit mapping hides contract breaks.
- Add a `startupProbe` so a slow-to-boot-but-valid config does not get killed during init, and a distinct error message for missing config.

### Lessons

- **Configuration is an interface.** Renaming one side without the other is a breaking change even when nothing else changed.
- **`--previous` logs are the pod's dying words.** For CrashLoopBackOff, `kubectl logs --previous` is the first command, before any theory.
- **CrashLoopBackOff is a symptom, not a diagnosis.** The restart delay makes it *look* worse; the cause is in the exit code and logs.

### 5-minute walkthrough script

> "After a config-only deploy, every checkout pod went CrashLoopBackOff. First command is `kubectl logs --previous`, which said the app exits because `PAYMENT_API_KEY` is unset. The secret's keys were right there: the PR had renamed the key to `payment_api_key` for style, breaking the contract with the app's config loader. The envFrom bulk mapping hid the mismatch; the pod was reaching the container and dying in the app, not failing to mount. We rolled back immediately, then fixed forward atomically — renamed both the secret key and the app's expected name in one PR — and added a CI test that boots the app against the real secret shape. The lesson: configuration is an interface, renames on one side of it are breaking changes, and `--previous` logs tell you the app's own opinion first."

---

## INC-10 — Service has no endpoints after a deploy

### The page

> **15:45 SEV1** — After deploying v2 of `search`, the Service returns `connection refused`. The pods are Running and Ready. There were no errors in the deploy pipeline.

### The evidence (raw, as collected)

```bash
kubectl get endpoints search -n search
kubectl get pods -n search -o wide --show-labels
kubectl describe svc search -n search
kubectl get deploy search -n search -o jsonpath='{.spec.selector.matchLabels}{"\n"}{.spec.template.metadata.labels}{"\n"}'
kubectl get rs -n search
kubectl get pods -n search -o json | jq -r '.items[] | [.metadata.name, .metadata.labels.app, .metadata.labels.version] | @tsv'
```

```text
$ kubectl get endpoints search -n search
NAME      ENDPOINTS   AGE
search    <none>      45d

$ kubectl get pods --show-labels
search-v2-abc   1/1  Running  app=search,version=v2,track=blue
search-v2-def   1/1  Running  app=search,version=v2,track=blue

$ kubectl describe svc search -n search
Selector:          app=search-svc,version=v1     <-- from the old Service YAML

$ kubectl get deploy search -o jsonpath=...
Selector:  {app: search, version: v2}
Labels:    {app: search, version: v2}
```

### Your turn (before the line)

- [ ] Timeline.
- [ ] Three hypotheses ranked with evidence.
- [ ] The one discriminating test.
- [ ] First 10 minutes.

---
---

# ANALYSIS — INC-10

### Timeline

The v2 deploy changed the pod labels from `app=search-svc` to `app=search`. The Service's `spec.selector` still selects `app=search-svc,version=v1`. Pods are Running and Ready, but no pod matches the Service's selector, so the Endpoints object has no addresses and every request is refused (no endpoints → kube-proxy has nothing to route to).

### Hypotheses ranked

1. **Service selector no longer matches pod labels.** Endpoints `<none>` + pods Running/Ready + selector `app=search-svc,version=v1` vs pod label `app=search`. Matches.
2. **Readiness probe failing.** Rejected: pods are 1/1 Ready; a failing readiness would still show Running pods but the *endpoints* would be empty only while Ready is false — here Ready is true, so labels are the remaining candidate.
3. **Wrong namespace / kube-proxy issue.** Rejected: the Service and pods share a namespace, and the selector mismatch explains everything without invoking the data plane.

### The discriminating test

Compare `spec.selector.matchLabels` on the Service with the actual pod labels: they must be a **subset match** (every selector label/value must exist on the pod). One character (`search-svc` vs `search`) is the whole incident.

### Root cause

A label rename in the deployment was not mirrored in the Service selector. Kubernetes does not warn about this: a Service with no matching pods is a valid state (`endpoints: <none>`).

### Fix (smallest safe first)

1. Update the Service's `spec.selector` to match the v2 labels (`app=search`), keeping `version` out of the selector unless you intend to pin it (pinning `version=v2` would break the next deploy — decide deliberately).
2. Verify `kubectl get endpoints search` shows pod IPs within seconds.
3. Remove the stale `version=v1` from the selector; make the label contract explicit in a lint check.

### Prevention

- A CI check that every Service's selector matches its target Deployment's `template.metadata.labels` (the `helm`/`kustomize` graph or a simple jq/kubectl check in the pipeline).
- Never rename `app` labels casually; they are the Service's addressing contract.
- Alert on a Service with zero endpoints for >1 min (this is a 5-line Prometheus rule and it catches the whole class).
- Prefer immutable `app` labels plus a separate `version`/`track` label for canary, and keep canary routing at the Ingress/Gateway, not in the Service selector.

### Lessons

- **A Service selects; it does not auto-discover.** If the labels change, the Service goes deaf silently.
- **`endpoints: <none>` with Ready pods is a label mismatch until proven otherwise.** Check the selector before the data plane.
- **The fastest incident in this workbook** — and the one most likely to appear in an interview as "what's the first thing you check when a service is unreachable in Kubernetes?"

### 5-minute walkthrough script

> "After a deploy, the search Service refused connections while its pods were Running and Ready. `kubectl get endpoints` showed `<none>` — that combination, Ready pods plus no endpoints, is a selector mismatch until proven otherwise. The deploy had renamed the pod label from `search-svc` to `search` and the Service selector still said `search-svc,version=v1`. Fix was updating the selector, endpoints populated in seconds, and I removed the pinned version from the selector. Prevention: a CI check that Service selectors match their Deployment labels, and a zero-endpoints alert. The lesson: a Service is a selector, not a discovery service — change the labels and you must change the selector."

---

## INC-11 — Node NotReady with ImagePullBackOff everywhere

### The page

> **19:05 SEV1** — Node `node-14` went NotReady; pods on other nodes start showing ImagePullBackOff for private images. The registry is up (public pulls work).

### The evidence (raw, as collected)

```bash
kubectl get node node-14 -o json | jq '.status.conditions[] | select(.type!="Ready") | {type,reason,message}'
kubectl describe node node-14 | grep -A6 'Conditions\|Events' | head -30
kubectl get events --field-selector involvedObject.name=node-14 --sort-by=.lastTimestamp | tail
ssh node-14
df -h /var/lib/containerd /var/lib/kubelet /var/log
du -xh --max-depth=1 /var/log | sort -h | tail -5
crictl images | head
crictl ps -a | head
journalctl -u containerd -u kubelet --since "1 hour ago" | grep -iE 'no space|error|evict' | tail -10
```

```text
DiskPressure=True reason: NodeHasDiskPressure message: ... kubelet has disk pressure

$ df -h /var/lib/containerd
Filesystem   Size  Used Avail Use%  Mounted on
/dev/sda1    100G  100G   512K 100% /var/lib/containerd (root fs)

$ du -xh --max-depth=1 /var/log | sort -h | tail -3
38G   /var/log/journal
 2.1G /var/log/containers
 ...

$ crictl images | head   (works; public images present; private image layers absent)

$ journalctl ... | grep -i 'no space' | tail -3
... "failed to pull image ... write /var/lib/containerd/...: no space left on device"
```

### Your turn (before the line)

- [ ] Timeline.
- [ ] Three hypotheses ranked with evidence.
- [ ] The one discriminating test.
- [ ] First 10 minutes.

---
---

# ANALYSIS — INC-11

### Timeline

`node-14`'s root filesystem filled to 100%. The kubelet detected DiskPressure, tainted the node, started evicting pods, and went NotReady under load. On other nodes, pods that needed to **pull private images** failed with ImagePullBackOff — not because the registry was down, but because... (two candidate mechanisms below; both were checked).

Mechanism A (what actually happened): the image pull **secret** was fine, but the nodes that went NotReady included the one hosting the **pull-through cache / registry mirror pod**; private pulls on remaining nodes went through that mirror, which was gone, and the registry's public endpoint does not serve the private repo. Mechanism B was considered and rejected: private pulls do not depend on a specific node. The evidence (`crictl` on a healthy node pulling the private image successfully with the same secret) rejected B.

### Hypotheses ranked

1. **Node disk full (DiskPressure) causing local failures plus a mirror outage cascading to other nodes.** Supported: DiskPressure condition, 100% disk, `no space left on device` in containerd logs, journal at 38 GB.
2. **Registry credentials expired globally.** Rejected: public pulls work and the same secret works on a healthy node.
3. **Network/DNS to the registry.** Rejected: pulls work on healthy nodes.

### The discriminating test

Try the same private pull on a healthy node with the same credentials: if it succeeds, the registry/credentials are fine and the problem is node-local or mirror-local. `crictl pull` with the same image on two nodes is a 30-second test that splits the hypothesis space.

### Root cause

Two stacked causes: (a) `node-14`'s disk filled with 38 GB of journald logs, triggering DiskPressure and eviction; (b) the cluster's registry mirror ran on that node, so the disk incident became a cluster-wide private-pull incident. Public images were unaffected, which is why the "registry is up" first impression was misleading.

### Fix (smallest safe first)

1. Reclaim space on node-14: `journalctl --vacuum-size=2G` (or `--vacuum-time=2d`), prune unused images (`crictl rmi --prune`), remove rotated logs. Cordon first if you plan a drain.
2. Let the kubelet clear DiskPressure; the taint auto-expires when the condition clears.
3. Restart the mirror pod (or move it to a node with reserved disk) and verify private pulls work cluster-wide.
4. Uncordon node-14 after a clean disk check.

### Prevention

- Persistent journald caps: `SystemMaxUse=2G` in `/etc/systemd/journald.conf`; log rotation for everything under `/var/log`.
- Kubelet image GC thresholds (`--image-gc-high-threshold`) reviewed per node size.
- DiskPressure/allocatable-disk alerts per node (Prometheus `kubelet_volume_stats` and node exporter `node_filesystem_avail`).
- The registry mirror must not run on a best-effort node without a health alert; a mirror outage looks like a registry outage to every user.
- Image pull success rate as a cluster-level SLO; alert on a rise in ImagePullBackOff events.

### Lessons

- **DiskPressure is a node-level condition with node-level fixes.** The fix is on the node (journald, image GC), not in the cluster.
- **One overloaded node can masquerade as a global outage** when it hosts shared infrastructure (a mirror, a DNS pod, a proxy).
- **`crictl pull` from a healthy node is the discriminator** for any pull failure — it splits registry/credentials from node-local causes in one command.

### 5-minute walkthrough script

> "Node 14 went NotReady and other nodes started ImagePullBackOff on private images while public pulls worked. The node conditions showed DiskPressure, and `df` showed the root filesystem at 100% — 38 GB of it was journald logs. The private-pull failures were the second layer: the cluster's registry mirror pod ran on that node, so its loss broke private pulls everywhere while the registry itself was fine. The 30-second discriminator was `crictl pull` with the same credentials on a healthy node — it succeeded, clearing the registry and clearing my initial theory. Fix was journald vacuum and image prune, the mirror pod restarted elsewhere, then uncordon. Prevention: journald size caps, per-node disk alerts, and never letting a shared mirror live on a best-effort node without a health alert. The lesson: an overloaded node can masquerade as a global outage when it hosts shared infrastructure."
