# Phase 4 Labs — Part 1 (L4.1–L4.4)

> **Safety:** these labs create namespaces, cgroups, mounts, containers, and a disposable kind cluster. Work on a VM you can rebuild. Do not run the cgroup or overlay steps on a production host. Clean up before moving on; a forgotten kind cluster consumes RAM and disk.

---

# Lab L4.1 — Build a Container From Scratch

## Objective

Create an isolated process tree with namespaces, a cgroup memory/PID limit, a chroot root filesystem, and an overlayfs writable layer. Then explain which piece Docker/containerd adds that you did not build.

## Prerequisites

- Ubuntu/Debian VM with root (or a user with `sudo`).
- cgroup v2 mounted at `/sys/fs/cgroup`.
- `busybox-static`, `python3`, `util-linux` (`unshare`, `nsenter`), `iproute2`.
- 15–20 minutes.

## Setup

```bash
sudo apt-get update
sudo apt-get install -y busybox-static python3 util-linux iproute2
stat -fc %T /sys/fs/cgroup          # expect cgroup2fs
uname -r
```

Create a minimal root filesystem:

```bash
sudo rm -rf /tmp/ctrlab
sudo mkdir -p /tmp/ctrlab/rootfs/{bin,proc,sys,dev,tmp,etc}
sudo cp /bin/busybox /tmp/ctrlab/rootfs/bin/
for a in sh ls ps mount cat hostname ifconfig sleep echo; do
  sudo ln -sf busybox /tmp/ctrlab/rootfs/bin/$a
done
sudo chmod 1777 /tmp/ctrlab/rootfs/tmp
```

```text
$ ls -l /tmp/ctrlab/rootfs/bin
lrwxrwxrwx ... cat -> busybox
lrwxrwxrwx ... echo -> busybox
...
-rwxr-xr-x ... busybox
```

## Part A — Namespaces and chroot

Run a shell with new PID, mount, UTS, IPC, and network namespaces, mount a fresh `/proc` inside the rootfs, then chroot:

```bash
sudo unshare --fork --pid --mount --uts --ipc --net \
  --mount-proc=/tmp/ctrlab/rootfs/proc \
  chroot /tmp/ctrlab/rootfs /bin/sh
```

Inside:

```bash
hostname ctr-lab
hostname
ps -ef
ifconfig -a
cat /proc/1/status | head -5
```

Expected observations:

```text
ctr-lab
PID   USER     TIME  COMMAND
    1 root      0:00 /bin/sh
    7 root      0:00 ps -ef

lo        Link encap:Local Loopback
          inet addr:127.0.0.1  Mask:255.0.0.0
          UP LOOPBACK RUNNING
```

- PID 1 is your shell, not the host's `systemd`. That is a new PID namespace.
- Only `lo` exists and it may be down. There is no `eth0`, no route, and no DNS. A container runtime would create a veth pair and configure these.
- The hostname is `ctr-lab`, isolated by the UTS namespace.
- The root filesystem has only `busybox`; there is no package manager, no libc (busybox-static), and no `/etc/passwd`. This is a rootfs, not a full distribution.

Bring up loopback:

```bash
ifconfig lo 127.0.0.1 up
ifconfig -a
```

Exit:

```bash
exit
```

## Part B — cgroup v2 limit

Check controllers:

```bash
cat /sys/fs/cgroup/cgroup.controllers
sudo mkdir -p /sys/fs/cgroup/ctrlab
echo "+memory +pids" | sudo tee /sys/fs/cgroup/cgroup.subtree_control
cat /sys/fs/cgroup/ctrlab/cgroup.controllers
```

```text
cpuset cpu io memory hugetlb pids rdma misc

cpuset cpu io memory hugetlb pids rdma misc
```

Set limits:

```bash
echo "64M" | sudo tee /sys/fs/cgroup/ctrlab/memory.max
echo "32"  | sudo tee /sys/fs/cgroup/ctrlab/pids.max
cat /sys/fs/cgroup/ctrlab/memory.max
cat /sys/fs/cgroup/ctrlab/pids.max
```

Start a process inside the cgroup:

```bash
sudo sh -c 'echo $$ > /sys/fs/cgroup/ctrlab/cgroup.procs; exec sleep 600' &
sleep 1
cat /sys/fs/cgroup/ctrlab/cgroup.procs
cat /sys/fs/cgroup/ctrlab/memory.current
```

Test the memory limit:

```bash
sudo sh -c 'echo $$ > /sys/fs/cgroup/ctrlab/cgroup.procs; exec python3 -c "a=bytearray(128*1024*1024); print(len(a))"'
echo $?
cat /sys/fs/cgroup/ctrlab/memory.events
```

Expected:

```text
Killed
137

low 0
high 0
max 0
oom 1
oom_kill 1
```

`memory.events` is the proof. The shell/python process was killed by the cgroup OOM killer because it exceeded `memory.max`. The host did not run out of memory; the limit was local to the cgroup.

Test the PID limit (careful; run in a subshell):

```bash
sudo sh -c 'echo $$ > /sys/fs/cgroup/ctrlab/cgroup.procs; for i in $(seq 1 40); do sleep 60 & done; wait'
```

Expected: `fork: Resource temporarily unavailable` or a subset of sleeps; `pids.current` approaches 32. Clean up the sleeps:

```bash
sudo pkill -f 'sleep 60' || true
```

## Part C — overlayfs writable layer

On the host (outside the namespace):

```bash
sudo rm -rf /tmp/ovl && mkdir -p /tmp/ovl/{lower,upper,work,merged}
echo "base image content" | sudo tee /tmp/ovl/lower/file.txt
sudo mount -t overlay overlay \
  -o lowerdir=/tmp/ovl/lower,upperdir=/tmp/ovl/upper,workdir=/tmp/ovl/work \
  /tmp/ovl/merged
cat /tmp/ovl/merged/file.txt
echo "container wrote this" | sudo tee /tmp/ovl/merged/file.txt
cat /tmp/ovl/lower/file.txt
cat /tmp/ovl/upper/file.txt
sudo rm /tmp/ovl/merged/file.txt
ls -la /tmp/ovl/upper/
```

Expected:

```text
base image content
base image content          # lower is unchanged
container wrote this        # upper has the new content
c--------- 1 root root 0, 0 ... file.txt   # whiteout after deletion
```

- The lower layer is immutable; the upper layer captures changes.
- A delete creates a whiteout (character device 0/0), hiding the lower file.
- The `merged` view is what the container sees.

Unmount:

```bash
sudo umount /tmp/ovl/merged
```

## Expected observations (summary)

| Check | Expected |
|---|---|
| `ps` in new PID namespace | PID 1 is your shell; few processes |
| `ip`/`ifconfig` | Only `lo`, no `eth0` |
| `hostname` | `ctr-lab`, not the host |
| cgroup `memory.events` | `oom_kill 1` after the 128M allocation |
| cgroup `pids.current` | Approaches 32, forks fail |
| overlay `upper/file.txt` | New content; lower unchanged |
| overlay delete | Whiteout in `upper` |

## Internal explanation

You built the four pieces a container runtime assembles:

1. **Namespaces** isolate the process's view: PID, mount, UTS, IPC, network.
2. **cgroups** bound and account for memory/PIDs. The kernel OOM killer acts on the cgroup, not the whole host.
3. **Root filesystem** via `chroot` gives the process a different `/`; a real runtime uses `pivot_root`/`mount` and a mount namespace to prevent escape.
4. **Overlayfs** provides a read-only image plus a writable layer.

What you did **not** build: veth/bridge/NAT and DNS (networking), capabilities/seccomp/AppArmor (security), image pull/unpack (distribution), a shim/reaper (lifecycle), volumes, health checks, and a Kubernetes API. That gap is why "containers are just namespaces" is incomplete: the runtime and the security profile matter.

## Troubleshooting task

You run the same steps on a second VM and see:

```text
$ sudo unshare --fork --pid --mount --uts --ipc --net \
    --mount-proc=/tmp/ctrlab/rootfs/proc \
    chroot /tmp/ctrlab/rootfs /bin/sh
unshare: unshare failed: Invalid argument
```

And after a different attempt, the cgroup limit has no effect:

```text
$ cat /sys/fs/cgroup/ctrlab/memory.max
64M
$ sudo sh -c 'echo $$ > /sys/fs/cgroup/ctrlab/cgroup.procs; exec python3 -c "a=bytearray(128*1024*1024)"'
$ cat /sys/fs/cgroup/ctrlab/memory.events
low 0
high 0
max 0
oom 0
oom_kill 0
```

Do not change random flags. Collect evidence:

1. `uname -r`, `cat /proc/self/status | grep -E 'Cap|Seccomp'`, `ls -l /proc/self/ns`.
2. `cat /proc/<pid>/cgroup` for the process you think is limited.
3. `cat /sys/fs/cgroup/ctrlab/cgroup.controllers`, `cgroup.subtree_control`, `cgroup.procs`.
4. `dmesg | tail`, `journalctl -k | tail`.

Write your hypothesis before reading the debrief at the end of this lab.

## Verification

- [ ] `ps` inside the namespace shows PID 1 as your shell.
- [ ] `hostname` is isolated.
- [ ] `ifconfig` shows only `lo` (and you can bring it up).
- [ ] `memory.events` shows `oom_kill` after the allocation.
- [ ] `pids.current` stops near 32.
- [ ] `upper/file.txt` differs from `lower/file.txt`.
- [ ] You can explain why the writable layer is not persistence.

## Cleanup

```bash
sudo umount /tmp/ovl/merged 2>/dev/null || true
sudo rm -rf /tmp/ovl /tmp/ctrlab
sudo rmdir /sys/fs/cgroup/ctrlab 2>/dev/null || true
sudo pkill -f 'sleep 600' 2>/dev/null || true
```

If `rmdir` fails with `Device or resource busy`, a process is still in the cgroup; find it with `cat /sys/fs/cgroup/ctrlab/cgroup.procs` and kill it.

## Write-up

```markdown
# L4.1 — Container from scratch
## What I built
## Evidence (paste key outputs: ps, cgroup events, overlay whiteout)
## The four isolation pieces and what each proves
## What Docker adds that I did not build
## The failure I caused and how I diagnosed it
## One thing I still cannot explain
```

## Debrief for the troubleshooting task

- `unshare: Invalid argument` on PID namespaces usually means the kernel/config does not support it (containerized host, missing `CONFIG_PID_NS`, or a restricted seccomp profile in the outer container). Check `uname -r`, `/proc/self/status` (`Seccomp`), and whether you are already inside a container. You cannot nest some namespace types under a restrictive outer sandbox.
- A cgroup limit that has no effect means the process is not in the cgroup you are reading. `cat /proc/<pid>/cgroup` shows the actual path; writing `$$` in a shell that then spawns a child does not guarantee the child stays if something moves it. Also confirm the controller is enabled in the parent (`cgroup.subtree_control`) and that you are not looking at a sibling cgroup. `memory.events` only counts events in that cgroup, not descendants unless you read the descendant's file.

---

# Lab L4.2 — Container Runtime and Networking

## Objective

Run and inspect a real container, prove its namespaces and cgroup limits, trace its veth/bridge/NAT path, publish a port, build a small image, and diagnose the three most common runtime failures.

## Prerequisites

- L4.1 or equivalent Linux knowledge.
- Docker Engine, Podman, or nerdctl. Commands below use Docker; Podman equivalents are noted.
- Root/sudo, `iproute2`, `iptables` or `nftables`, `tcpdump`, `curl`.
- 20–30 minutes.

## Setup

```bash
docker version
docker info --format '{{.ServerVersion}} {{.CgroupDriver}} {{.CgroupVersion}}'
```

```text
27.3.1 systemd 2
```

If `docker info` fails, start the daemon (`sudo systemctl enable --now docker`) or use `podman`. Do not install Docker Desktop on the Linux VM; use the engine.

Create a working directory and a tiny image:

```bash
mkdir -p /tmp/l4.2 && cd /tmp/l4.2
cat > Dockerfile <<'EOF'
FROM python:3.12-alpine
WORKDIR /app
RUN echo "hello from the image" > index.html
EXPOSE 8000
CMD ["python3", "-m", "http.server", "8000"]
EOF
docker build -t lab-http:1.0 .
```

## Part A — Run, inspect, and enter

```bash
docker run -d --name lab-web -p 8080:8000 lab-http:1.0
docker ps
curl -sS http://127.0.0.1:8080/
docker logs lab-web
docker inspect lab-web --format '{{.State.Pid}} {{.State.Status}} {{.NetworkSettings.IPAddress}}'
```

Expected:

```text
hello from the image

172.17.0.2
```

Enter the container's namespaces from the host:

```bash
PID=$(docker inspect -f '{{.State.Pid}}' lab-web)
sudo ls -l /proc/$PID/ns
sudo nsenter -t $PID -n ip addr
sudo nsenter -t $PID -n ip route
sudo nsenter -t $PID -m -p --mount-proc ps -ef
```

Expected: `eth0` with `172.17.0.2/16`, a default route via `172.17.0.1`, and a process tree whose PID 1 is `python3 -m http.server`.

```bash
docker exec -it lab-web sh
# inside:
hostname
cat /etc/hosts
cat /etc/resolv.conf
ps -ef
exit
```

The container has its own hostname, `/etc/hosts`, resolver config, and PID namespace. It shares the kernel with the host.

## Part B — Image layers and writable layer

```bash
docker history --no-trunc lab-http:1.0
docker image inspect lab-http:1.0 --format '{{.Id}} {{.Architecture}} {{.Os}}'
docker inspect lab-web --format '{{json .GraphDriver.Data}}' | python3 -m json.tool
```

```text
IMAGE          CREATED         CREATED BY                                      SIZE
...            ...             CMD ["python3" "-m" "http.server" "8000"]       0B
...            ...             RUN /bin/sh -c echo "hello from the image" ...   22B
...            ...             WORKDIR /app                                    0B
```

Write inside the container, then remove and recreate it:

```bash
docker exec lab-web sh -c 'echo scratch > /app/scratch.txt'
docker exec lab-web cat /app/scratch.txt
docker rm -f lab-web
docker run -d --name lab-web -p 8080:8000 lab-http:1.0
docker exec lab-web ls /app
```

`scratch.txt` is gone. The writable layer died with the container. A named volume is the persistence fix.

## Part C — cgroup limits and OOM

```bash
docker run -d --name lab-limited --memory 64m --memory-swap 64m --cpus 0.5 --pids-limit 64 lab-http:1.0
docker inspect lab-limited --format '{{.HostConfig.Memory}} {{.HostConfig.NanoCpus}} {{.HostConfig.PidsLimit}}'
docker stats --no-stream lab-limited
```

```text
67108864 500000000 64
CONTAINER ID   NAME          CPU %   MEM USAGE / LIMIT   MEM %   ...
...            lab-limited   0.00%   12.4MiB / 64MiB     19.38%  ...
```

Trigger an OOM inside the cgroup:

```bash
docker exec lab-limited python3 -c 'a=bytearray(128*1024*1024)'
echo $?
docker inspect lab-limited --format '{{.State.OOMKilled}} {{.State.ExitCode}}'
```

Expected: exit `137`, `OOMKilled=true`. The container may restart depending on its restart policy; `docker inspect` still shows the last state.

Find the cgroup and read the kernel's evidence:

```bash
CID=$(docker inspect -f '{{.Id}}' lab-limited)
cat /sys/fs/cgroup/system.slice/docker-$CID.scope/memory.events 2>/dev/null || \
  cat /sys/fs/cgroup/docker/$CID/memory.events
```

## Part D — Network path: veth, bridge, NAT, port publish

```bash
ip link show master docker0
sudo iptables -t nat -S | grep -E 'DOCKER|MASQUERADE'
sudo conntrack -L 2>/dev/null | grep 8080 || true
```

Expected (trimmed):

```text
-A POSTROUTING -s 172.17.0.0/16 ! -o docker0 -j MASQUERADE
-A DOCKER -d 172.17.0.2/32 -p tcp --dport 8000 -j DNAT --to-destination 172.17.0.2:8000
```

Capture the traffic while curling:

```bash
sudo tcpdump -ni docker0 -c 20 port 8000 &
curl -sS http://127.0.0.1:8080/ >/dev/null
wait
```

You should see the SYN/ACK and HTTP request/response on `docker0`. The destination is the container IP, not `127.0.0.1`.

Check outbound NAT:

```bash
docker exec lab-web wget -qO- https://example.com | head -2
sudo conntrack -L | grep 172.17.0.2 | head
```

The container's outbound traffic is masqueraded to the host's address.

## Part E — Volumes

```bash
docker volume create labdata
docker run -d --name lab-vol -v labdata:/data lab-http:1.0
docker exec lab-vol sh -c 'echo persisted > /data/file.txt'
docker rm -f lab-vol
docker run --rm -v labdata:/data lab-http:1.0 cat /data/file.txt
```

Expected: `persisted`. The named volume outlives the container.

## Expected observations

| Check | Expected |
|---|---|
| `curl` host:8080 | `hello from the image` |
| `nsenter -n ip addr` | `eth0 172.17.0.2/16` |
| `ps` in container | PID 1 is the app, not host systemd |
| `docker history` | Small layers; base image dominates |
| Recreate container | Writable-layer file gone |
| `--memory 64m` + 128M alloc | Exit 137, `OOMKilled=true` |
| `iptables -t nat` | DNAT + MASQUERADE |
| `tcpdump docker0` | Traffic to 172.17.0.2:8000 |
| Named volume after recreate | File persists |

## Internal explanation

- A container is a process with namespaces and cgroups, not a VM. `docker exec` uses `setns`/`nsenter`.
- The image is a stack of read-only layers; the container adds a writable overlay layer. Deleting the container deletes that layer.
- The veth pair connects the container's network namespace to `docker0`; the host routes between the bridge and the outside, with NAT for outbound traffic and DNAT for published ports.
- The cgroup limit is enforced by the kernel; `OOMKilled` is the runtime reporting the cgroup OOM event.

## Troubleshooting task

Three failures are staged for you. Reproduce each, collect evidence, and write the fault domain before fixing.

1. **Port conflict:** start a second container with `-p 8080:8000`. It fails. Which process owns 8080, and what are your options?
2. **Loopback bind:** run a container whose server binds `127.0.0.1:8000` inside the container (use `python3 -m http.server --bind 127.0.0.1 8000`). Publish `-p 8081:8000`. `curl` from the host fails, but `docker exec ... curl 127.0.0.1:8000` works. Why?
3. **Broken DNS:** run a container with `--dns 203.0.113.53`. `ping 1.1.1.1` works; `getent hosts example.com` fails. What is the fault domain, and what would you check on the host?

Do not read the debrief until you have written your hypotheses.

## Verification

- [ ] You can show the container's PID, network namespace, and cgroup.
- [ ] You can explain why the writable layer is ephemeral.
- [ ] You reproduced an OOMKilled container and read `memory.events`.
- [ ] You captured DNAT traffic on `docker0`.
- [ ] You persisted data in a named volume across container deletion.
- [ ] You diagnosed all three staged failures from evidence.

## Cleanup

```bash
docker rm -f lab-web lab-limited lab-vol 2>/dev/null || true
docker volume rm labdata 2>/dev/null || true
docker rmi lab-http:1.0 2>/dev/null || true
rm -rf /tmp/l4.2
```

## Write-up

```markdown
# L4.2 — Runtime and networking
## Container identity (PID, namespaces, cgroup)
## Image/layer evidence
## OOM evidence (exit code, memory.events)
## Packet path evidence (veth, bridge, DNAT, conntrack)
## Three failures: symptom → evidence → fault domain → fix
## What surprised me
```

## Debrief

1. **Port conflict:** `ss -lntp | grep 8080` shows the existing Docker proxy/process. Options: stop the other container, choose another host port, or use a different bind address. Docker's userland proxy or iptables owns the port; the container itself is not listening on the host.
2. **Loopback bind:** the app is bound to the container's loopback, but DNAT sends traffic to the container's `eth0` IP. The kernel does not accept a connection to `eth0:8000` when the socket is bound to `127.0.0.1:8000`. Fix: bind `0.0.0.0` inside the container.
3. **Broken DNS:** the container's `/etc/resolv.conf` points at an unreachable resolver. `ping 1.1.1.1` proves L3 works; DNS is the fault domain. Check `docker exec cat /etc/resolv.conf`, the host resolver, Docker's `--dns`/daemon config, and whether UDP/TCP 53 is blocked. Fix the resolver or remove the override.

---

# Lab L4.3 — kind Cluster and Kubernetes Core

## Objective

Create a disposable kind cluster, deploy a workload, trace a Pod from YAML to running container, inspect the control plane and node components, and perform a rollout and rollback.

## Prerequisites

- Docker/Podman running (L4.2).
- `kind` and `kubectl` installed.
- 4 vCPU, 8 GB RAM, 20 GB free disk.
- 30–40 minutes.

## Setup

```bash
kind version
kubectl version --client
```

Create a kind config with a NodePort mapping:

```bash
mkdir -p /tmp/l4.3 && cd /tmp/l4.3
cat > kind-config.yaml <<'EOF'
kind: Cluster
apiVersion: kind.x-k8s.io/v1alpha4
nodes:
  - role: control-plane
    extraPortMappings:
      - containerPort: 30080
        hostPort: 30080
        protocol: TCP
      - containerPort: 30081
        hostPort: 30081
        protocol: TCP
EOF
kind create cluster --name opslab --config kind-config.yaml
kubectl cluster-info
kubectl get nodes -o wide
```

```text
NAME                 STATUS   ROLES           AGE   VERSION
opslab-control-plane Ready    control-plane   45s   v1.30.0
```

Check the control plane:

```bash
kubectl get pods -n kube-system
kubectl get --raw='/readyz?verbose'
kubectl get --raw='/livez?verbose'
kubectl api-resources | head -20
```

Inspect the node container:

```bash
docker ps --filter name=opslab
docker exec -it opslab-control-plane crictl ps
docker exec -it opslab-control-plane crictl pods
```

You will see the API server, etcd, scheduler, controller-manager, CoreDNS, kindnet, and kube-proxy as containers/Pods. The "node" is a Docker container running a kubelet and containerd.

## Part A — Deploy and trace

```bash
kubectl create namespace app
kubectl -n app create deployment web --image=nginx:1.27 --replicas=2
kubectl -n app expose deployment web --port=80 --target-port=80
kubectl -n app get deploy,rs,pods -o wide
```

Watch the rollout:

```bash
kubectl -n app get pods -w
# Ctrl-C when both are Running
kubectl -n app rollout status deploy/web
```

Trace one Pod:

```bash
POD=$(kubectl -n app get pod -l app=web -o jsonpath='{.items[0].metadata.name}')
kubectl -n app describe pod "$POD"
kubectl -n app get pod "$POD" -o jsonpath='{.spec.nodeName} {.status.podIP} {.status.qosClass}'
kubectl -n app get events --sort-by=.lastTimestamp
```

Explain each field:

- `spec.nodeName` was set by the scheduler.
- `status.podIP` was assigned by the CNI.
- `status.qosClass` reflects requests/limits (BestEffort here, because none were set).
- The ReplicaSet's `ownerReferences` points to the Deployment.

```bash
kubectl -n app get rs -o yaml | grep -A5 ownerReferences
kubectl -n app logs "$POD" --tail=20
kubectl -n app exec "$POD" -- curl -sS localhost:80 | head -3
```

Test the Service:

```bash
kubectl -n app get svc web
kubectl -n app get endpointslices -l kubernetes.io/service-name=web -o wide
kubectl -n app run curlpod --rm -it --restart=Never --image=curlimages/curl -- \
  curl -sS http://web.app.svc.cluster.local | head -3
```

## Part B — Rollout and rollback

```bash
kubectl -n app set image deploy/web nginx:1.27-alpine
kubectl -n app rollout status deploy/web
kubectl -n app rollout history deploy/web
kubectl -n app rollout undo deploy/web
kubectl -n app rollout status deploy/web
kubectl -n app get rs
```

Observe the old and new ReplicaSets. `rollout undo` does not delete the old ReplicaSet; it scales it back up and scales the other down, preserving history.

Scale and inspect:

```bash
kubectl -n app scale deploy/web --replicas=4
kubectl -n app get pods -o wide
kubectl -n app scale deploy/web --replicas=2
```

## Part C — NodePort

```bash
kubectl -n app patch svc web -p '{"spec":{"type":"NodePort","ports":[{"port":80,"targetPort":80,"nodePort":30080}]}}'
curl -sS http://127.0.0.1:30080/ | head -3
kubectl -n app get svc web
```

The kind port mapping forwards host:30080 to the node container:30080; kube-proxy then forwards to the Service endpoints.

## Expected observations

| Check | Expected |
|---|---|
| `kubectl get nodes` | One Ready control-plane node |
| `kube-system` Pods | API server, etcd, scheduler, controller-manager, CoreDNS, kindnet, kube-proxy |
| `get --raw /readyz` | `ok` for each check |
| Pod `spec.nodeName` | `opslab-control-plane` |
| Pod QoS | `BestEffort` (no requests/limits) |
| Service endpoints | Two Pod IPs |
| `curl` Service DNS | nginx welcome page |
| Rollout/undo | Two ReplicaSets, old one scaled back |
| NodePort curl | nginx welcome page |

## Internal explanation

- kind runs Kubernetes nodes as containers, but the control plane and kubelet are real processes. The node container shares the host kernel but has its own namespaces.
- The Deployment controller creates a ReplicaSet; the ReplicaSet creates Pods; the scheduler binds them; the kubelet starts them via containerd; kindnet assigns IPs; CoreDNS and kube-proxy make the Service name work.
- A Service is not a process; the ClusterIP is a virtual IP implemented by kube-proxy rules.
- `rollout undo` is a controller operation on ReplicaSets, not a file restore.

## Troubleshooting task

Break the Deployment in three ways, one at a time. After each, capture `get pods`, `describe`, `events`, and `logs --previous` before fixing.

1. Set the image to `nginx:does-not-exist`.
2. Set the readiness probe to `httpGet /missing` on port 80.
3. Set `resources.requests.memory: 100Gi`.

Write the fault domain and the exact event message for each. Do not delete the namespace between attempts; use `kubectl -n app rollout undo` or patch back.

## Verification

- [ ] Cluster is Ready and `/readyz` passes.
- [ ] You traced a Pod from Deployment → ReplicaSet → Pod → node → container.
- [ ] You explained the Pod IP, node name, and QoS class.
- [ ] Service DNS and NodePort both return the app.
- [ ] You performed a rollout and rollback and saw two ReplicaSets.
- [ ] You diagnosed image, probe, and scheduling failures from events.

## Cleanup

```bash
kind delete cluster --name opslab
rm -rf /tmp/l4.3
```

## Write-up

```markdown
# L4.3 — kind and core Kubernetes
## Cluster topology and versions
## The apply → running chain (with evidence at each step)
## Deployment/ReplicaSet/Service relationships
## Three failures: event message → fault domain → fix
## What I would check first next time
```

## Debrief

1. **Bad image:** `ErrImagePull`/`ImagePullBackOff`, event `Failed to pull image ... not found`. Fault domain: image reference/registry. Fix the tag.
2. **Bad readiness:** Pod `Running` but `0/1 Ready`; event `Readiness probe failed: HTTP probe failed with statuscode: 404`; the Service loses that endpoint. Fault domain: probe path/port or app route. Fix the probe or add the route.
3. **Huge request:** `Pending`; event `0/1 nodes are available: 1 Insufficient memory`. Fault domain: scheduling/capacity. Lower the request or add a node (not possible in a one-node kind cluster without more resources).

---

# Lab L4.4 — Services, DNS, and NetworkPolicy

## Objective

Deploy a client/server pair, prove ClusterIP, headless, and NodePort behavior, trace DNS resolution and kube-proxy rules, create a no-endpoints failure, and test a default-deny NetworkPolicy.

## Prerequisites

- L4.3 cluster running (recreate it if deleted).
- `kubectl`, `curl`.
- A policy-capable CNI for the enforcement part (see the note below).
- 30–40 minutes.

## Setup

```bash
if ! kubectl cluster-info >/dev/null 2>&1; then
  cat > /tmp/kind-opslab.yaml <<'EOF'
kind: Cluster
apiVersion: kind.x-k8s.io/v1alpha4
nodes:
  - role: control-plane
    extraPortMappings:
      - containerPort: 30080
        hostPort: 30080
      - containerPort: 30081
        hostPort: 30081
EOF
  kind create cluster --name opslab --config /tmp/kind-opslab.yaml
fi
kubectl create namespace netlab
kubectl -n netlab create deployment server --image=hashicorp/http-echo:1.0 --replicas=2 -- -text=hello-from-server -listen=:5678
kubectl -n netlab expose deployment server --port=80 --target-port=5678
kubectl -n netlab get pods,svc,endpointslices -o wide
```

If `hashicorp/http-echo` is unavailable in your registry, use `nginx:1.27` and `--port=80 --target-port=80`; the networking behavior is the same.

## Part A — ClusterIP and DNS

```bash
kubectl -n netlab run client --rm -it --restart=Never --image=nicolaka/netshoot -- bash
# inside:
cat /etc/resolv.conf
nslookup server.netlab.svc.cluster.local
getent hosts server.netlab
curl -sS http://server.netlab.svc.cluster.local
curl -sS http://server
exit
```

Expected:

```text
Server:   10.96.0.10
Address:  10.96.0.10:53

Name:     server.netlab.svc.cluster.local
Address:  10.96.142.31

hello-from-server
```

The short name `server` works inside `netlab` because of the search path. From another namespace it would need `server.netlab`.

## Part B — EndpointSlice and kube-proxy

```bash
kubectl -n netlab get endpointslices -l kubernetes.io/service-name=server -o yaml
kubectl -n netlab get pods -l app=server --show-labels
```

Find the Service IP and trace the rules on the node:

```bash
SVCIP=$(kubectl -n netlab get svc server -o jsonpath='{.spec.clusterIP}')
echo "$SVCIP"
docker exec -it opslab-control-plane iptables-save -t nat | grep -A4 "$SVCIP"
docker exec -it opslab-control-plane conntrack -L 2>/dev/null | grep "$SVCIP" || true
```

Expected: `KUBE-SERVICES` → `KUBE-SVC-*` → `KUBE-SEP-*` with DNAT to one of the Pod IPs.

## Part C — Headless Service

```bash
kubectl -n netlab create service clusterip server-headless --clusterip=None --tcp=80:5678
kubectl -n netlab get svc server-headless
kubectl -n netlab get endpointslices -l kubernetes.io/service-name=server-headless -o wide
kubectl -n netlab run dns2 --rm -it --restart=Never --image=nicolaka/netshoot -- \
  nslookup server-headless.netlab.svc.cluster.local
```

Expected: no ClusterIP (`None`), and DNS returns both Pod IPs. The client chooses one; there is no kube-proxy load balancing.

## Part D — Break the selector (no endpoints)

```bash
kubectl -n netlab patch svc server -p '{"spec":{"selector":{"app":"wrong"}}}'
kubectl -n netlab get endpointslices -l kubernetes.io/service-name=server -o wide
kubectl -n netlab run fail --rm -it --restart=Never --image=nicolaka/netshoot -- \
  curl -v --max-time 5 http://server.netlab.svc.cluster.local
```

Expected: the EndpointSlice has no addresses; curl resolves the ClusterIP but fails to connect (refused or timeout depending on the proxy mode). Fix it:

```bash
kubectl -n netlab patch svc server -p '{"spec":{"selector":{"app":"server"}}}'
kubectl -n netlab get endpointslices -l kubernetes.io/service-name=server
```

## Part E — NodePort

```bash
kubectl -n netlab patch svc server -p '{"spec":{"type":"NodePort","ports":[{"port":80,"targetPort":5678,"nodePort":30081}]}}'
curl -sS http://127.0.0.1:30081/
```

If your kind config only maps 30080, patch to 30080 or recreate the cluster with both mappings. The NodePort is a port on every node; kind maps it to the host.

## Part F — NetworkPolicy

**Important:** kind's default CNI (`kindnet`) may not enforce NetworkPolicy. First test whether it does:

```bash
kubectl -n netlab apply -f - <<'EOF'
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny
  namespace: netlab
spec:
  podSelector: {}
  policyTypes: ["Ingress", "Egress"]
EOF
kubectl -n netlab run blocked --rm -it --restart=Never --image=nicolaka/netshoot -- \
  curl -v --max-time 5 http://server.netlab.svc.cluster.local
```

If the policy is enforced, the curl times out (including DNS, because egress is denied). If it still succeeds, your CNI does not enforce policy; the object is declarative only. Record which result you got.

Add a minimal allow policy and an egress policy for the client:

```bash
kubectl -n netlab apply -f - <<'EOF'
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-client
  namespace: netlab
spec:
  podSelector:
    matchLabels:
      app: server
  policyTypes: ["Ingress"]
  ingress:
    - from:
        - podSelector:
            matchLabels:
              role: client
      ports:
        - protocol: TCP
          port: 5678
---
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-client-egress
  namespace: netlab
spec:
  podSelector:
    matchLabels:
      role: client
  policyTypes: ["Egress"]
  egress:
    - to:
        - namespaceSelector:
            matchLabels:
              kubernetes.io/metadata.name: kube-system
      ports:
        - protocol: UDP
          port: 53
        - protocol: TCP
          port: 53
    - to:
        - podSelector:
            matchLabels:
              app: server
      ports:
        - protocol: TCP
          port: 5678
EOF
kubectl -n netlab run client2 --rm -it --restart=Never --labels=role=client --image=nicolaka/netshoot -- \
  curl -sS --max-time 5 http://server.netlab.svc.cluster.local
```

The client works because both directions are allowed: ingress to the server and DNS/server egress from the client. Remove the egress policy and the same command times out at DNS, which proves why default-deny egress must be paired with an explicit DNS rule.

To get real enforcement on kind, create a separate cluster with `disableDefaultCNI: true` and install a policy-capable CNI (Calico or Cilium), or run this lab on a managed cluster. Do not conclude "NetworkPolicy is broken" from a kindnet cluster; verify the CNI's capability.

## Expected observations

| Check | Expected |
|---|---|
| DNS FQDN | ClusterIP for the normal Service; Pod IPs for headless |
| EndpointSlice | Two ready endpoints |
| iptables | KUBE-SVC/KUBE-SEP DNAT rules |
| Selector broken | Empty endpoints; connection failure |
| NodePort | App response on host:30081 |
| Default-deny | Timeout if CNI enforces; success means no enforcement |
| Allow policy | Only labeled clients reach the port; DNS needs its own egress rule |

## Internal explanation

- DNS resolves names to ClusterIPs; it does not health-check the backend. A Service with no endpoints still resolves.
- kube-proxy programs DNAT; it does not inspect HTTP. The endpoint selection is per-connection, not per-request.
- Headless Services move load balancing to the client and are required for StatefulSet identity.
- NetworkPolicy is enforced by the CNI at the Pod's network boundary; kube-proxy and CoreDNS are not policy engines.

## Troubleshooting task

You receive three reports. For each, name the fault domain and the first two commands:

1. "`server` works from `netlab` but not from `default`."
2. "`server` resolves, but 1 out of 3 requests fails."
3. "After adding default-deny, everything times out, including DNS."

Do not fix anything until you have written the hypotheses.

## Verification

- [ ] You resolved a Service by short name, FQDN, and headless name.
- [ ] You showed the DNAT rule for the ClusterIP.
- [ ] You broke and restored the selector and proved endpoints changed.
- [ ] You reached the app through a NodePort.
- [ ] You determined whether your CNI enforces NetworkPolicy.
- [ ] You explained why default-deny egress breaks DNS.

## Cleanup

```bash
kubectl delete namespace netlab --wait=true
# Keep the kind cluster for L4.5, or delete it:
# kind delete cluster --name opslab
```

## Write-up

```markdown
# L4.4 — Services, DNS, policy
## Service types and when I would use each
## DNS resolution path (resolv.conf → CoreDNS → ClusterIP)
## DNAT evidence
## No-endpoints diagnosis
## NetworkPolicy result and CNI capability
## The report I would send to the app team
```

## Debrief

1. **Works in one namespace only:** DNS short names are namespace-relative. From `default`, use `server.netlab` or the FQDN. A NetworkPolicy may also allow only same-namespace traffic.
2. **One of three requests fails:** one endpoint is bad or not ready. Test each Pod IP directly, inspect EndpointSlice readiness, and check that Pod's logs/restarts.
3. **Default-deny breaks DNS:** egress is denied, including UDP/TCP 53 to CoreDNS. Add an explicit egress rule to `kube-system` (or the DNS Service IP) before debugging application connectivity.
