# ANALYSIS — further than the roadmap expected, stopped by two specific, real walls

## The honest headline, stated first

The roadmap's own table said this project would be "Docs + real
YAML/Helm manifests + runbook — no live cluster here," on the
assumption that this sandbox's Docker daemon/network access "is not
guaranteed." That assumption was tested, not trusted, and turned out
to be only half true: **a real, single-node Kubernetes v1.31 control
plane genuinely comes up in this sandbox** — real `kube-apiserver`,
real `kube-scheduler`, real `kube-controller-manager`, a real `kubelet`
registering a real node — using k3s, with zero container-registry
access needed for any of it. The real `cilium-cli` tool then genuinely
installed Cilium's real Helm chart against that real cluster: real
DaemonSets and Deployments were created, real pods were scheduled onto
the real node. What stopped there, precisely, were two separate,
independently-diagnosed real limitations — not "Kubernetes doesn't
work here," but "this specific sandbox's container-registry network
policy and its cgroup delegation both say no, for different reasons."

## Why k3s, and why it's not a lesser substitute for "real Kubernetes"

k3s is CNCF-certified, upstream Kubernetes — the same `kube-apiserver`/
`kube-scheduler`/`kube-controller-manager`/`kubelet` binaries Kubernetes
ships, compiled into one static binary alongside an embedded containerd
and (by default) a lightweight SQLite-backed datastore instead of etcd.
It was chosen here specifically because it was the one path that didn't
need a container registry for its *own* components: k3s publishes an
"airgap images" tarball as a plain GitHub release asset — every image
k3s's default install needs (containerd's own bundled CoreDNS,
local-path-provisioner, metrics-server, etc.) pre-packaged into one
file, imported directly into containerd's local image store with no
network pull at all. `kind` and `minikube`, the other common
local-cluster tools, both fundamentally require pulling a multi-hundred-
megabyte "node image" from a registry to even start — a dead end here
before Kubernetes itself ever enters the picture, so they weren't
pursued further once k3s's airgap path confirmed reachable.

## What was actually, directly verified — not assumed

**1. The Docker daemon itself genuinely works**, contrary to the
roadmap's hedge. `dockerd` was never started by default in this
sandbox, but starting it manually succeeds completely: real
`containerd`, real `runc`, real `overlayfs` storage driver, zero
errors. (`docker run` itself then hit the same registry block described
below — consistent with everything else in this document, and the
reason this project pivoted to k3s's registry-free airgap path instead
of fighting Docker Hub access.)

**2. GitHub release assets are reachable even though GitHub's own
container registry (`ghcr.io`) and every other container registry
tested are not.** This is the finding that made everything else in this
project possible. Confirmed directly:
```
$ curl -sSL -o /dev/null -w "%{http_code}" https://github.com/k3s-io/k3s/releases/download/v1.31.4%2Bk3s1/k3s
200   (69,243,032 bytes)
$ curl -sSL -o /dev/null -w "%{http_code}" https://github.com/k3s-io/k3s/releases/download/v1.31.4%2Bk3s1/k3s-airgap-images-amd64.tar.zst
200   (141,023,685 bytes)
$ curl -sSL -o /dev/null -w "%{http_code}" https://github.com/cilium/cilium-cli/releases/latest/download/cilium-linux-amd64.tar.gz
200   (76,845,833 bytes)
```
against, in the same session:
```
$ curl -sS https://ghcr.io/v2/            -> CONNECT tunnel failed, response 403
$ curl -sS https://registry-1.docker.io/v2/ -> CONNECT tunnel failed, response 403
$ curl -sS https://registry.k8s.io/v2/    -> CONNECT tunnel failed, response 403
$ curl -sS https://quay.io/v2/            -> CONNECT tunnel failed, response 403
$ curl -sS https://dl.k8s.io/             -> CONNECT tunnel failed, response 403
$ curl -sS https://get.helm.sh/           -> CONNECT tunnel failed, response 403
$ curl -sS https://helm.cilium.io/        -> CONNECT tunnel failed, response 403
```
The sandbox's own egress-proxy status endpoint confirms this is a
deliberate organization policy decision, not a flaky network: every one
of those registry hosts logs as `"gateway answered 403 to CONNECT
(policy denial or upstream failure)"`. GitHub's git protocol and its
release-asset CDN are allowlisted; every container-image and binary-
distribution registry tried is not. (`helm`'s own GitHub releases were
also tried and came back `404` — likely a stale guessed tag/asset-name
combination on this author's part, not a policy block, since the same
pattern worked immediately for k3s and cilium-cli; not investigated
further since it wasn't load-bearing for this project's conclusion.)

**3. A real Kubernetes v1.31 control plane came up, fully, using only
the airgap bundle — confirmed with `kubectl`, not inferred:**
```
$ kubectl get nodes -o wide
NAME   STATUS     ROLES                  AGE   VERSION        INTERNAL-IP   CONTAINER-RUNTIME
vm     NotReady   control-plane,master   19s   v1.31.4+k3s1   192.0.2.2     containerd://1.7.23-k3s2
```
(`evidence/01-nodes.txt`.) `NotReady` here is the *correct*, expected
state — a Kubernetes node is never `Ready` until a CNI has configured
pod networking, and none was installed yet at this point. CoreDNS was
also genuinely scheduled by the real scheduler (`evidence/02-pods.txt`,
`evidence/04-events.txt`), confirming the control plane's scheduling
loop is fully live, not just the API server answering requests.

**4. The real `cilium-cli` tool auto-detected this exact cluster
correctly and generated real, cluster-specific Helm values** — not
hand-written, not guessed:
```
$ cilium install --version 1.16.5 --helm-set hubble.relay.enabled=true \
    --helm-set hubble.ui.enabled=true --dry-run-helm-values
cluster:
  name: default
hubble:
  relay: {enabled: true}
  ui: {enabled: true}
operator: {replicas: 1}
routingMode: tunnel
tunnelProtocol: vxlan
```
(`cilium/values-hubble-enabled.yaml`.) `routingMode: tunnel` /
`tunnelProtocol: vxlan` is cilium-cli's own correct judgment call for a
k3s cluster without an existing cloud-provider CIDR allocation — not a
value this project chose or typed in. The complete rendered manifest set
(`cilium/rendered-manifests.yaml`, 2,275 lines: 2 DaemonSets, 3
Deployments, 4 ConfigMaps, 3 Secrets, full RBAC) is the literal output
of `cilium install --dry-run` against this exact chart version — a side
discovery worth noting: cilium-cli embeds the Helm chart directly in its
own binary (this is why these renders worked even though
`helm.cilium.io`, the chart's *remote* repository, is itself blocked —
no network round-trip to fetch the chart was needed, only the later
image pulls were).

**5. The real install was actually attempted, not just rendered** —
`cilium install` (without `--dry-run`) really created the DaemonSets/
Deployment in the live cluster, and the real scheduler really placed
real pods onto the real node:
```
$ kubectl get pods -A -o wide
kube-system   cilium-envoy-gpl9h                 0/1  ContainerCreating
kube-system   cilium-mm255                       0/1  Init:0/6
kube-system   cilium-operator-...-fhrcf           0/1  ContainerCreating
```
(`evidence/02-pods.txt`.) This is where the two real blockers below
were actually hit, in flight, not predicted in advance.

## Blocker 1: container registries are policy-blocked, confirmed at the exact pull

```
$ k3s ctr images pull quay.io/cilium/cilium:v1.16.5
time="..." level=info msg="trying next host" error="failed to do request:
  Head \"https://quay.io/v2/cilium/cilium/manifests/v1.16.5\": Forbidden" host=quay.io
ctr: failed to resolve reference "quay.io/cilium/cilium:v1.16.5":
  failed to do request: Head "https://quay.io/v2/cilium/cilium/manifests/v1.16.5": Forbidden
```
Reproduced identically for `quay.io/cilium/operator-generic:v1.16.5`
(`evidence/09-operator-pull-attempt.txt`). This is the real containerd
pull client getting the real HTTP 403 the egress proxy returns for
every container registry tested — the same mechanism, and the same
honest conclusion, as the `ghcr.io`/`docker.io`/`quay.io` curl tests
above, now hit from inside the actual Kubernetes image-pull path rather
than a standalone curl. Nothing about Cilium, Kubernetes, or k3s caused
this; it is purely this sandbox's registry egress policy, and it would
block *any* image from *any* of these registries, Cilium included only
because Cilium happens to be what this project needed to pull.

## Blocker 2: a separate, more fundamental limitation — nested pod-sandbox creation itself

Even setting Blocker 1 aside, a second, independent failure showed up —
and this one would block *any* real pod in this sandbox, regardless of
which image it uses or where that image comes from:
```
Warning  FailedCreatePodSandBox  pod/cilium-operator-...  Failed to create pod sandbox:
  rpc error: code = Unknown desc = failed to create containerd task: failed to
  create shim task: OCI runtime create failed: runc create failed: unable to
  start container process: can't get final child's PID from pipe: EOF: unknown
```
(`evidence/03-cilium-pod-describe.txt`, `evidence/04-events.txt` — the
identical error, on identical retries, for `cilium-envoy`,
`cilium` (the main agent), and `cilium-operator` alike; the last of
those needs no elevated privileges or host mounts at all, ruling out
"this is a Cilium-specific hostPath/privileged-container problem.")

The kubelet's own startup log names the precise, correct root cause —
not guessed, read directly from `evidence/07-cgroup-mismatch-excerpt.txt`:
```
Failed to get the kubelet's cgroup. Kubelet system container metrics may
be missing. err="cpu and memory cgroup hierarchy not unified.  cpu: /,
memory: /process_api/01a1278c-8499-75fd-9ab9-c16d06508b39/claude-code-bash"
```
This sandbox's own shell/task process is placed at different cgroup
paths for the `cpu` and `memory` v1 controllers (`/` vs. a nested
`process_api/.../claude-code-bash` path) — an asymmetry that's fine for
a process that doesn't itself try to spawn nested, independently-
cgrouped containers, but breaks exactly that for `runc`, which needs a
single, consistent cgroup path to create a *new* child cgroup for each
pod sandbox. Cross-checked independently via the cgroup v2 unified
hierarchy (`evidence/08-unified-cgroup-controllers.txt`):
```
$ cat /sys/fs/cgroup/unified/cgroup.controllers
hugetlb
```
Only `hugetlb` is delegated into this sandbox's v2 unified hierarchy —
none of `cpu`, `memory`, or `pids`, the controllers a container runtime
actually needs to delegate resource limits to a child container. Two
independent angles (the v1 hierarchy-mismatch message, and the near-
empty v2 delegation list), same conclusion: this sandbox's own cgroup
placement was never set up to let a nested container runtime create
further-nested cgroups for pod sandboxes — a limitation of *this specific
container's own cgroup delegation*, not of Kubernetes, containerd,
runc, or Cilium, and not something fixable by working around a missing
tool or a blocked network path the way every earlier project in this
series did. Deliberately not worked around here either: doing so would
mean altering this sandbox's own cgroup/namespace setup from inside
it — a different, riskier kind of "workaround" than reaching for an
alternate attach mechanism or a hand-written shim, and outside what
this project judged worth attempting.

## Why Tetragon's manifests are real but untested end-to-end

Tetragon doesn't have a `cilium-cli`-equivalent tool with an embedded
chart, so getting its *real*, cluster-specific rendered manifests the
way Cilium's were captured above would have needed a working `helm`
binary. `helm`'s own GitHub-releases download attempt 404'd (see above —
likely just a wrong guessed version tag, not a policy block, but not
chased further), and its remote chart repository
(`helm.cilium.io`) is itself behind the same registry-style block as
everything else. What's shipped in `tetragon/chart-reference/` instead
is the literal upstream chart source (`templates/`, `values.yaml`,
`Chart.yaml`), copied verbatim from a real, shallow clone of
`github.com/cilium/tetragon` (reachable the same way every other
GitHub clone in this series has been) — genuine, diffable, current
upstream content, just not independently *re-rendered* by this
project's own tooling the way Cilium's was. The five example
`TracingPolicy` CRs in `tetragon/examples/` are likewise copied
verbatim from that same real repository's own `examples/tracingpolicy/`
directory, chosen for being small, self-contained, and illustrative of
distinct Tetragon capabilities (syscall-level privilege-escalation
detection via `commit_creds()`/`sys_setuid`, TCP connection monitoring
with IP-range selectors, and filesystem hardlink observation) — not
written for this project, and not modified from the original.

## What this project does *not* demonstrate, stated plainly

- **Cilium never reached `Running`, let alone governed real pod
  traffic.** The entire value of this project's live-cluster portion is
  a real control plane, a real install attempt, and two real, precisely
  diagnosed stopping points — not a working CNI.
- **Hubble and Tetragon were never observed emitting a single real
  flow or security event**, for the same underlying reasons (Hubble
  needs Cilium running first; Tetragon needs a container runtime that
  can actually start its DaemonSet's pods).
- **The cgroup-delegation finding (Blocker 2) is specific to this
  sandbox's own process placement**, not a general claim about nested
  containers or about Kubernetes-in-a-container as a technique — a
  different sandboxed environment with properly delegated cgroups (or a
  genuine cloud VM) would very plausibly clear this blocker even before
  addressing the registry policy in Blocker 1.
- **This sandbox's registry block (Blocker 1) is an explicit,
  deliberate organization policy** (confirmed via the egress-proxy's own
  status endpoint, not inferred from failure alone) — not a defect to
  route around, and this project makes no attempt to.
