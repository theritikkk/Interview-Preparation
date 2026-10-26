# Container Internals

> **The one-sentence model:** a container is a normal Linux process (or process tree) whose view of the system is restricted by namespaces, whose resource use is bounded by cgroups, whose filesystem is an overlay mount, and whose privileges are reduced by capabilities, seccomp, and LSMs. It is not a lightweight VM, and it does not have its own kernel.

---

## 1. What Docker actually starts

```text
docker run nginx
  → dockerd receives the API request
  → containerd pulls the image and prepares a snapshot
  → containerd-shim starts runc
  → runc creates namespaces + cgroups + rootfs + security profile
  → runc execs nginx as PID 1 inside the container
  → runc exits; the shim becomes the container's parent/reaper
```

The kernel does the isolation. Docker, containerd, and runc are orchestration around kernel features.

| Piece | Responsibility |
|---|---|
| `dockerd` | API, image build, volumes, networks, auth; not the runtime itself |
| `containerd` | Image pull/unpack, snapshots, container lifecycle, CRI implementation |
| `containerd-shim` | Keeps the container alive if containerd restarts; reaps zombies; holds stdio |
| `runc` | Creates the namespaces, cgroups, mounts, and `exec`s the process; exits |
| Linux kernel | Namespaces, cgroups, overlayfs, netfilter, capabilities, seccomp |

**Consequence:** if the container's PID 1 exits, the container exits. If PID 1 ignores `SIGTERM`, `docker stop` waits 10 seconds and sends `SIGKILL`. If the process forks children and does not reap them, zombies accumulate inside the container.

---

## 2. Namespaces

A namespace wraps a global kernel resource and gives processes inside it a private view.

| Namespace | Isolates | Flag | Typical symptom when wrong |
|---|---|---|---|
| `mnt` | Mount table, rootfs | `CLONE_NEWNS` | Container sees host paths; `pivot_root` fails |
| `pid` | PID numbers, `/proc` | `CLONE_NEWPID` | Container sees host PIDs; cannot kill PID 1 |
| `net` | Interfaces, routes, ports, netfilter | `CLONE_NEWNET` | Two containers share an IP/port |
| `uts` | Hostname, domain name | `CLONE_NEWUTS` | Container hostname is the host's |
| `ipc` | SysV IPC, POSIX message queues | `CLONE_NEWIPC` | Shared memory leaks between containers |
| `user` | UID/GID mappings, capabilities | `CLONE_NEWUSER` | Rootless mapping wrong; permission denied |
| `cgroup` | cgroup view | `CLONE_NEWCGROUP` | Container sees host cgroup tree |
| `time` | Boot time, monotonic clock offsets | `CLONE_NEWTIME` | Rare; used by time namespacing |

Inspect:

```bash
lsns
lsns -p $$
ls -l /proc/$$/ns
readlink /proc/$$/ns/net
```

```text
$ ls -l /proc/self/ns
lrwxrwxrwx 1 root root 0 ... cgroup -> 'cgroup:[4026531835]'
lrwxrwxrwx 1 root root 0 ... ipc    -> 'ipc:[4026531839]'
lrwxrwxrwx 1 root root 0 ... mnt    -> 'mnt:[4026531841]'
lrwxrwxrwx 1 root root 0 ... net    -> 'net:[4026531840]'
lrwxrwxrwx 1 root root 0 ... pid    -> 'pid:[4026531836]'
lrwxrwxrwx 1 root root 0 ... user   -> 'user:[4026531837]'
lrwxrwxrwx 1 root root 0 ... uts    -> 'uts:[4026531838]'
```

If two processes show the same inode number for a namespace, they share it. That is how you prove whether a container is really isolated.

### Creating namespaces by hand

```bash
sudo unshare --fork --pid --mount-proc --uts --ipc --net --mount /bin/bash
hostname sandbox
ps -ef
ip link
```

- `--fork` is required for a new PID namespace: the shell must be the child that becomes PID 1 in the new namespace.
- `--mount-proc` mounts a fresh `/proc` so `ps` sees the new PID namespace.
- `--net` gives you a new network namespace with only a down loopback. You must bring it up:

```bash
ip link set lo up
ip addr
```

```text
1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 ...
    inet 127.0.0.1/8 scope host lo
```

No `eth0` exists. A container runtime connects a veth pair and configures the interface; that is the CNI/runtime's job.

### Entering an existing namespace

```bash
sudo nsenter -t <pid> -n -p -m --mount-proc /bin/bash
sudo nsenter -t <pid> -n ip addr
```

`nsenter` is how `docker exec` and `kubectl exec` work under the hood: they join the target's namespaces and run a process.

---

## 3. cgroups v2

cgroups account for and limit resources. cgroup v2 has a single unified hierarchy at `/sys/fs/cgroup`.

```text
/sys/fs/cgroup/
├── cgroup.controllers      # controllers available at this level
├── cgroup.subtree_control  # controllers enabled for children
├── cgroup.procs            # processes in this cgroup
├── memory.max              # hard memory limit
├── memory.high             # throttle threshold
├── memory.current
├── cpu.max                 # "$QUOTA $PERIOD" in microseconds
├── cpu.weight              # relative share, 1..10000
├── io.max                  # per-device "rbps wbps riops wiops"
└── pids.max
```

Check the host:

```bash
stat -fc %T /sys/fs/cgroup
# cgroup2fs means v2
cat /sys/fs/cgroup/cgroup.controllers
# cpuset cpu io memory hugetlb pids rdma misc
```

Create a cgroup manually (root, v2):

```bash
sudo mkdir /sys/fs/cgroup/lab
echo "+memory +cpu +pids" | sudo tee /sys/fs/cgroup/cgroup.subtree_control
cat /sys/fs/cgroup/lab/cgroup.controllers
```

```text
cpuset cpu io memory hugetlb pids rdma misc
```

Set limits and move a process in:

```bash
echo "100M" | sudo tee /sys/fs/cgroup/lab/memory.max
echo "50000 100000" | sudo tee /sys/fs/cgroup/lab/cpu.max   # 0.5 CPU
echo "64" | sudo tee /sys/fs/cgroup/lab/pids.max
echo $$ | sudo tee /sys/fs/cgroup/lab/cgroup.procs
```

Read accounting:

```bash
cat /sys/fs/cgroup/lab/memory.current
cat /sys/fs/cgroup/lab/memory.events
cat /sys/fs/cgroup/lab/cpu.stat
```

```text
low 0
high 0
max 3
oom 2
oom_kill 2
```

`memory.events` is the evidence for an OOM kill. `oom_kill 2` means two processes in this cgroup were killed because the cgroup hit `memory.max`.

### cgroup v1 vs v2

| v1 | v2 |
|---|---|
| Separate hierarchies per controller | One unified hierarchy |
| `memory.limit_in_bytes`, `cpu.cfs_quota_us` | `memory.max`, `cpu.max` |
| `tasks` | `cgroup.procs` / `cgroup.threads` |
| Harder to reason about combined limits | Controllers enabled per subtree |

Modern distributions (Ubuntu 22.04+, RHEL 9+, Debian 12+) default to v2. Kubernetes 1.25+ requires v2 for some features; kubelet's cgroup driver should be `systemd` on systemd hosts. Mixing `cgroupfs` and `systemd` causes duplicate cgroups and unreliable limits.

### "No internal processes" rule

In v2, a cgroup cannot both contain processes and enable domain controllers for children. Put processes in leaf cgroups; enable controllers in the parent's `cgroup.subtree_control` before creating children. This is why a naïve `mkdir` + `echo` setup can fail with `EBUSY`.

### Docker/containerd view

```bash
docker run -d --name lim --memory 128m --cpus 0.5 --pids-limit 64 nginx
docker inspect lim --format '{{.HostConfig.Memory}} {{.HostConfig.NanoCpus}} {{.HostConfig.PidsLimit}}'
systemd-cgls /sys/fs/cgroup/system.slice/docker-*.scope
docker stats --no-stream lim
```

```text
CONTAINER ID   NAME  CPU %   MEM USAGE / LIMIT   MEM %   NET I/O       BLOCK I/O
...            lim   0.00%   3.2MiB / 128MiB     2.50%   1.1kB / 0B    0B / 0B
```

Docker's `--memory` maps to `memory.max`; `--cpus 0.5` maps to `cpu.max = 50000 100000`; `--pids-limit` maps to `pids.max`.

---

## 4. Root filesystem and overlayfs

A container image is a stack of read-only layers plus metadata. At runtime the runtime adds a writable layer on top using overlayfs.

```text
        ┌──────────────────────────────┐
        │ writable layer (upperdir)     │  container writes
        ├──────────────────────────────┤
        │ image layer 3 (lowerdir)      │  read-only
        ├──────────────────────────────┤
        │ image layer 2 (lowerdir)      │  read-only
        ├──────────────────────────────┤
        │ base layer (lowerdir)         │  read-only
        └──────────────────────────────┘
                   merged
```

Mount one by hand:

```bash
mkdir -p /tmp/ovl/{lower,upper,work,merged}
echo "base" | sudo tee /tmp/ovl/lower/file.txt
sudo mount -t overlay overlay \
  -o lowerdir=/tmp/ovl/lower,upperdir=/tmp/ovl/upper,workdir=/tmp/ovl/work \
  /tmp/ovl/merged
cat /tmp/ovl/merged/file.txt          # base
echo "changed" | sudo tee /tmp/ovl/merged/file.txt
cat /tmp/ovl/lower/file.txt           # base (unchanged)
cat /tmp/ovl/upper/file.txt           # changed
```

Key details:

- `upperdir` and `workdir` must be on the same filesystem; `workdir` must be empty and separate from `upperdir`.
- A delete in the merged view creates a **whiteout** in `upperdir` (character device 0/0), hiding the lower file. It does not modify the lower layer.
- A directory replacement can use the `trusted.overlay.opaque` xattr.
- The writable layer is deleted with the container. Data that must survive belongs in a volume.

Inspect a container's mount:

```bash
docker inspect lim --format '{{json .GraphDriver.Data}}'
```

```json
{"LowerDir":"/var/lib/docker/overlay2/.../diff:...","MergedDir":"/var/lib/docker/overlay2/.../merged","UpperDir":"/var/lib/docker/overlay2/.../diff","WorkDir":"/var/lib/docker/overlay2/.../work"}
```

### Image layers and build cache

```bash
docker history --no-trunc nginx:1.27
docker image inspect nginx:1.27 --format '{{.Id}} {{.Architecture}} {{.Os}}'
docker manifest inspect nginx:1.27 | head -40
```

Each Dockerfile instruction that changes the filesystem creates a layer. `RUN apt-get update && apt-get install -y ...` in one layer avoids a stale cache. `COPY` invalidates the cache when file content changes. Multi-stage builds keep compilers out of the final image.

**Mistake:** `COPY . .` before `npm install`/`pip install` invalidates the dependency layer on every source change. Copy the lockfile first, install, then copy the source.

---

## 5. Capabilities, seccomp, LSMs, and user namespaces

Root in a container is still root to the kernel unless you use a user namespace. The default Docker profile reduces risk but is not a security boundary you should bet a multi-tenant platform on.

```bash
capsh --print
getpcaps $$
docker run --rm --cap-drop=ALL --cap-add=NET_BIND_SERVICE nginx:1.27
docker run --rm --security-opt no-new-privileges --read-only --tmpfs /tmp nginx:1.27
```

Default Docker capabilities include `CHOWN`, `DAC_OVERRIDE`, `FOWNER`, `FSETID`, `KILL`, `SETGID`, `SETUID`, `SETPCAP`, `NET_BIND_SERVICE`, `NET_RAW`, `SYS_CHROOT`, `MKNOD`, `AUDIT_WRITE`, `SETFCAP`. `NET_RAW` is why ping works inside many containers and also why raw sockets are a risk. `SYS_ADMIN` is effectively root and should be avoided.

| Control | What it stops | How to inspect |
|---|---|---|
| Capabilities | Privileged operations (mount, raw sockets, module load) | `capsh --print`, `getpcaps`, `/proc/<pid>/status` (`CapEff`) |
| Seccomp | Dangerous syscalls (`mount`, `ptrace`, `bpf`, `keyctl`) | `grep Seccomp /proc/<pid>/status`, runtime profile |
| AppArmor | File/network access by profile | `aa-status`, `/proc/<pid>/attr/current` |
| SELinux | Type enforcement, container labels | `getenforce`, `ls -Z`, `ausearch -m avc` |
| User namespace | UID 0 inside ≠ UID 0 outside | `/proc/<pid>/uid_map`, `unshare -Ur` |
| `no-new-privileges` | setuid binaries escalating | `docker inspect` security opts |

```bash
grep -E 'Cap(Eff|Bnd)|Seccomp|NoNewPrivs' /proc/<container-pid>/status
```

```text
CapEff: 00000000a80425fb
CapBnd: 00000000a80425fb
Seccomp: 2
NoNewPrivs: 0
```

`Seccomp: 2` means a seccomp filter is active. `CapEff` is a bitmask; decode it with `capsh --decode=00000000a80425fb`.

**Safety rule:** `--privileged` disables the useful security controls, mounts host devices, and gives all capabilities. If a workload only works with `--privileged`, find the specific capability/device it needs. Never run untrusted images privileged.

---

## 6. Container networking

A container gets its own network namespace. The runtime creates a veth pair: one end in the container (`eth0`), one end on a host bridge (`docker0`/`cni0`/`vethXXX`), then assigns IPs and adds NAT.

```text
container netns                 host netns
┌─────────────┐   veth pair   ┌──────────────────────┐
│ eth0        │◄─────────────►│ vethXXXX             │
│ 172.17.0.2  │               │ docker0 172.17.0.1   │
└─────────────┘               └──────────┬───────────┘
                                         │ MASQUERADE
                                      eth0 / Internet
```

Inspect:

```bash
docker run -d --name netlab nginx:1.27
docker inspect netlab --format '{{.State.Pid}}'
sudo nsenter -t $(docker inspect -f '{{.State.Pid}}' netlab) -n ip addr
sudo nsenter -t $(docker inspect -f '{{.State.Pid}}' netlab) -n ip route
ip link show master docker0
sudo iptables -t nat -S | grep -E 'DOCKER|MASQUERADE'
```

Port publishing:

```bash
docker run -d --name web -p 8080:80 nginx:1.27
curl -sS http://127.0.0.1:8080/ | head -5
sudo iptables -t nat -S DOCKER | grep 8080
sudo conntrack -L | grep 8080
```

`-p 8080:80` installs a DNAT rule from host:8080 to container:80 plus a masquerade rule for outbound traffic. The container sees the connection coming from the bridge gateway, not the original client, unless the runtime preserves the source.

**Common failure:** the app listens on `127.0.0.1` inside the container. Port publishing DNATs to the container IP, so the app never accepts. Bind to `0.0.0.0` inside the container.

---

## 7. Volumes and the writable layer

| Mount | Lifetime | Use |
|---|---|---|
| Container writable layer | Deleted with container | Temporary files, package installs at runtime |
| Named volume | Until deleted | Databases, persistent app data |
| Bind mount | Host path | Config, source code, host integration |
| `tmpfs` | Container lifetime, RAM | Secrets, scratch, `/tmp` |
| Image layer | Immutable | Application binaries and static assets |

```bash
docker volume create appdata
docker run -d --name db -v appdata:/var/lib/postgresql/data postgres:16
docker inspect db --format '{{json .Mounts}}'
docker volume inspect appdata
```

```json
[{"Type":"volume","Name":"appdata","Source":"/var/lib/docker/volumes/appdata/_data","Destination":"/var/lib/postgresql/data","Driver":"local","Mode":"z","RW":true,"Propagation":""}]
```

The `z`/`Z` mode is SELinux relabeling; on Ubuntu/AppArmor it is usually not needed. Rootless Docker maps UIDs, so a bind mount owned by root on the host may be unwritable inside the container.

---

## 8. Resource limits, OOM, and exit codes

```bash
docker run --rm --memory 64m --memory-swap 64m stress-ng --vm 1 --vm-bytes 128M
echo $?
docker inspect <container> --format '{{.State.OOMKilled}} {{.State.ExitCode}}'
```

```text
137
true 137
```

- Exit `137` = 128 + 9 = SIGKILL, often the OOM killer.
- Exit `143` = 128 + 15 = SIGTERM, often a clean stop.
- Exit `139` = SIGSEGV.
- Exit `126` = command found but not executable.
- Exit `127` = command not found.

`OOMKilled` in Docker means the container's cgroup hit `memory.max`. Host OOM can kill the container process too but may not set the flag. Check both `docker inspect` and the host `dmesg`/`journalctl -k`.

```bash
cat /sys/fs/cgroup/system.slice/docker-<id>.scope/memory.events
cat /sys/fs/cgroup/system.slice/docker-<id>.scope/memory.max
cat /sys/fs/cgroup/system.slice/docker-<id>.scope/memory.peak
```

CPU limits cause throttling, not kills. Check `cpu.stat`:

```text
nr_periods 100
nr_throttled 37
throttled_usec 18500000
```

High `nr_throttled` with latency spikes means the CPU quota is too low or the workload is bursty; raising `--cpus` or removing the limit may help, but it can also move contention to the host.

---

## 9. Failure scenarios

### A — Container exits immediately

```text
$ docker run --rm myapp:latest
standard_init_linux.go:228: exec user process caused: exec format error
```

Fault domain: image/architecture or entrypoint. Check `docker image inspect --format '{{.Architecture}}'`, `file` on the binary, and `docker run --entrypoint /bin/sh`. `exec format error` usually means an amd64 image on arm64 or a missing interpreter (e.g. a glibc binary on Alpine/musl).

### B — `permission denied` on a bind mount

Fault domain: UID/GID mapping, SELinux, or file mode. Check `id` inside the container, `ls -ln` on the host, `docker inspect` mounts, `ausearch -m avc`, and whether the runtime is rootless. Fix the ownership or use a named volume; do not `chmod 777`.

### C — Container has no network

Fault domain: namespace, bridge, NAT, DNS, or firewall. Inside: `ip addr`, `ip route`, `cat /etc/resolv.conf`, `ping 1.1.1.1`, `getent hosts example.com`. Host: `sysctl net.ipv4.ip_forward`, `iptables -t nat -L`, `nft list ruleset`, `docker network inspect bridge`. A common cause is a host firewall dropping forwarded traffic or `ip_forward=0`.

### D — Port publish does not work

Fault domain: app bind address, DNAT, host firewall, or port conflict. Check `ss -lntp` on the host, `docker port web`, `curl` from inside the container, `iptables -t nat -S DOCKER`, and `conntrack`. If `curl` works inside but not from the host, the app is bound to loopback or the DNAT/firewall is wrong.

### E — Works with `--privileged`, fails without

Fault domain: capabilities, devices, or LSM. Identify the exact syscall/device with `strace`, `dmesg`, or the app's error. Add only the needed capability (`--cap-add SYS_NICE`) or device (`--device /dev/fuse`), not `--privileged`.

### F — Disk fills up on the host

Fault domain: image/layer/volume/log growth. Check `docker system df`, `du -sh /var/lib/docker/*`, container logs (`/var/lib/docker/containers/*/*-json.log`), and dangling volumes. Clean with `docker system prune` only after confirming what will be deleted; never prune on a production host without a change window.

---

## 10. Command field manual

### `unshare` — create namespaces

**Purpose:** run a process with new namespaces for experimentation.

**Why:** proves what a container runtime does, without Docker hiding it.

**Options:** `--fork` (required for PID), `--pid`, `--net`, `--mount`, `--uts`, `--ipc`, `--user`, `--map-root-user`, `--mount-proc`.

```bash
sudo unshare --fork --pid --mount-proc --net --uts --ipc --mount /bin/bash
```

**Output:** a shell whose `ps` shows few processes and whose `ip link` shows only `lo`.

**Interpretation:** you are in new namespaces, but you have no rootfs, no veth, no cgroup limits, and no seccomp. That is the gap between "namespaces" and "a usable container".

**Scenario:** teaching, debugging a runtime, reproducing an isolation bug.

**Mistakes:** forgetting `--fork` (the shell stays in the old PID namespace); forgetting `--mount-proc` (`ps` shows host PIDs); expecting networking to work (loopback starts down, no veth).

### `nsenter` — enter namespaces

**Purpose:** run a command in an existing process's namespaces.

**Why:** inspect a container from the host, or debug a process that has no shell.

**Options:** `-t PID`, `-n` net, `-p` pid, `-m` mount, `-u` uts, `-i` ipc, `-U` user, `--mount-proc`.

```bash
sudo nsenter -t 4242 -n -p --mount-proc ip addr
```

**Interpretation:** you see the container's interfaces and routes, not the host's. If `nsenter` fails with `No such process`, the container exited.

**Mistakes:** entering only the net namespace and expecting the container's filesystem; using the host PID after the process restarted; running destructive commands with the container's root privileges.

### `lsns` — list namespaces

**Purpose:** map namespaces to processes.

**Why:** prove whether two processes share a network or mount namespace.

```bash
lsns -t net -o NS,TYPE,NPROCS,PID,COMMAND
```

**Interpretation:** same `NS` inode = shared namespace. `NPROCS` counts processes in it.

### `systemd-cgls` / `systemd-cgtop` — cgroup tree and usage

```bash
systemd-cgls /sys/fs/cgroup/system.slice
systemd-cgtop -m
```

**Interpretation:** shows the slice/scope hierarchy and per-cgroup CPU/memory. Use it to find which container is consuming host memory when `docker stats` is not enough.

### `docker inspect` — ground truth for a container

```bash
docker inspect web --format '{{.State.Status}} {{.State.Pid}} {{.State.OOMKilled}} {{.HostConfig.Memory}}'
```

**Why:** `docker ps` is a summary; `inspect` shows the actual config, mounts, network, exit code, and OOM flag.

**Mistakes:** reading only `State.Status` and missing `OOMKilled`; assuming the container's IP is stable.

### `docker stats` — live resource view

```bash
docker stats --no-stream
```

**Interpretation:** CPU% can exceed 100% on multi-core; `MEM LIMIT` is the cgroup limit; `BLOCK I/O` is cumulative. Use `--no-stream` for scripts.

### `docker history` — image layers

```bash
docker history --no-trunc myapp:latest
```

**Interpretation:** large layers reveal build mistakes (caches, toolchains, secrets). `--no-trunc` shows the full command; a `RUN` layer that deletes files in the same layer can still shrink the image, but deleting in a later layer does not reclaim space.

### `ctr` / `nerdctl` / `crictl` — runtime-level tools

```bash
sudo ctr -n k8s.io containers list
sudo crictl ps -a
sudo crictl inspect <id> | jq '.status'
```

**Why:** when Kubernetes says a container is running but `docker ps` shows nothing, the kubelet is using containerd/CRI-O, not Docker. `crictl` talks to the CRI endpoint; `ctr` talks to containerd directly. Use the right one.

**Mistakes:** `ctr` without `-n k8s.io` will not see Kubernetes containers; `crictl` needs the correct endpoint (`/run/containerd/containerd.sock`).

### `capsh` / `getpcaps` — capability inspection

```bash
capsh --decode=00000000a80425fb
getpcaps 1
```

**Interpretation:** decode the bitmask into names. A container with `CAP_SYS_ADMIN` is effectively privileged for many purposes.

### `strace` — syscall-level evidence

```bash
sudo strace -f -p <pid> -e trace=openat,connect,execve
```

**Why:** distinguishes "file not found" from "permission denied" from "seccomp killed it" when logs are useless. In containers, `strace` may itself be blocked by seccomp or need `SYS_PTRACE`.

---

## 11. Checkpoint

Answer without notes, then check.

1. Which kernel features define a container?
2. Why does `unshare --pid` need `--fork`?
3. What does `--mount-proc` do?
4. What is the difference between `memory.max` and `memory.high`?
5. What does `cpu.max = 50000 100000` mean?
6. What is the "no internal processes" rule in cgroup v2?
7. What are `lowerdir`, `upperdir`, `workdir`, and `merged` in overlayfs?
8. What happens to a file deleted from the merged overlay view?
9. Why is a container's writable layer not a persistence strategy?
10. What is the difference between a named volume and a bind mount?
11. Why is `--privileged` dangerous?
12. What does `Seccomp: 2` mean in `/proc/<pid>/status`?
13. What is the difference between containerd and runc?
14. What does exit code 137 usually mean?
15. Why can a container ping `1.1.1.1` but not resolve `example.com`?
16. Why does `-p 8080:80` fail if the app listens on `127.0.0.1:80` inside the container?
17. Which command shows a container's actual cgroup limits?
18. What does `nsenter -t <pid> -n` do, and what does it not do?
19. Why does `docker history` show a large layer after you deleted files in a later layer?
20. What is the difference between `docker stop` and `docker kill`?

### Answers

1. Namespaces (isolation), cgroups (resources), a root filesystem (overlayfs), and security controls (capabilities, seccomp, LSMs), plus a runtime that assembles them.
2. The new PID namespace applies to children created after `unshare`; without `--fork`, the shell remains in the old namespace. `--fork` makes the child PID 1.
3. It mounts a fresh `procfs` at `/proc`, so tools see the new PID namespace instead of the host's.
4. `memory.max` is the hard limit; exceeding it triggers the cgroup OOM killer. `memory.high` is a throttle threshold; the kernel reclaims and throttles, but does not necessarily kill.
5. A quota of 50,000 µs per 100,000 µs period = 0.5 CPU.
6. A cgroup cannot both contain processes and enable domain controllers for child cgroups; processes belong in leaf cgroups.
7. `lowerdir` = read-only image layers; `upperdir` = writable changes; `workdir` = overlayfs scratch (same filesystem as upper); `merged` = the combined view.
8. Overlayfs creates a whiteout in `upperdir`; the lower file is hidden but unchanged.
9. It is deleted with the container and is tied to that container's lifecycle; volumes are the persistence layer.
10. A named volume is managed by the runtime and has a lifecycle independent of the container; a bind mount maps a specific host path and depends on host ownership/permissions.
11. It grants all capabilities, disables seccomp/AppArmor restrictions in many setups, and exposes host devices; a compromised container can affect the host.
12. A seccomp filter is active (`0` = disabled, `1` = strict, `2` = filter).
13. containerd is a daemon that manages images, snapshots, and container lifecycle; runc is the low-level OCI runtime that creates namespaces/cgroups and execs the process, then exits.
14. SIGKILL (128+9), commonly an OOM kill or a forced stop.
15. DNS is broken: `/etc/resolv.conf`, the embedded DNS server, a firewall/NetworkPolicy blocking UDP/TCP 53, or the host resolver unreachable. IP connectivity works, name resolution does not.
16. DNAT sends traffic to the container IP, but the app is bound only to loopback, so it never accepts connections addressed to `eth0`.
17. `docker inspect` (`HostConfig.Memory`, `NanoCpus`, `PidsLimit`) and the cgroup files under `/sys/fs/cgroup/...`.
18. It enters the network namespace and runs a command there; it does not enter the mount namespace, so you see the host filesystem unless you also pass `-m`.
19. Deleting a file in a later layer creates a whiteout; the bytes still exist in the earlier layer, so the image size does not shrink. Rebuild the layer or use multi-stage builds.
20. `docker stop` sends SIGTERM, waits (default 10 s), then SIGKILL; `docker kill` sends SIGKILL immediately (or a specified signal).
