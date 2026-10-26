# Question Bank — ~140 Recall Drills With Answers

> **How to use:** cover the answer, answer aloud in one or two sentences, then check. These are recall drills, not essay prompts — the goal is retrieval speed, which is what OAs and phone screens test. Grouped by phase; answer keys follow each group.

---

## Linux (P1) — 25 questions

1. What does the first field of `ls -l` output (`-rwxr-x---`) mean, and what are the three position groups?
2. What is the difference between 755 and 644, and when would you use each?
3. What does setuid do, and why is it a security surface?
4. What is the difference between a hard link and a symlink?
5. What does the `sticky bit` on a directory do, and where do you see it by default?
6. What does `df` measure that `du` does not, and vice versa?
7. What is an inode, and what two common failures relate to it?
8. What does `vmstat`'s `wa` column mean, and what does a high value imply?
9. What is the difference between a process's RSS and VSZ?
10. What is a page fault, and what is the difference between minor and major?
11. What does swap `si/so` activity in `vmstat` indicate, and is it always bad?
12. What does OOM killer consider when choosing a victim?
13. What is a cgroup, and what two things do containers use it for?
14. What is a namespace, and name three common types?
15. What is a zombie process, and what causes one?
16. What does `nice`/`renice` control, and what does it not control?
17. What is the difference between SIGTERM, SIGKILL, and SIGHUP in common service usage?
18. What is a D-state process, and why can't you kill it?
19. What does `systemctl status` show for a failed unit, and what are the first two things to read?
20. What is `RequiresMountsFor=` for?
21. What does `journalctl -u nginx -b` show?
22. What is the difference between journald and `/var/log` files, and what controls journald's disk use?
23. What does `ulimit -n` control, and what is the modern equivalent?
24. What does `lsof +L1` find?
25. What is PSI (pressure stall information), and why is it better than load average for "is the box suffering?"

### Answers — Linux

1. File type + 3×3 permission bits: owner, group, other (type char; then r/w/x for owner, group, other).
2. 755 = owner full, group/others read+execute (binaries, directories); 644 = owner write, others read (config files, static content). Directories need `x` to traverse.
3. Setuid on an executable runs it with the file owner's privileges (e.g., `passwd`); any setuid root binary is a privilege-escalation surface — audit with `find / -perm -4000`.
4. A hard link is another directory entry for the same inode (same filesystem); a symlink is a separate file whose content is a path, which can dangle and cross filesystems.
5. The sticky bit on a directory restricts deletion to the file's owner (and root); `/tmp` (`drwxrwxrwt`).
6. `df` reads the filesystem's free-space/usage accounting; `du` walks directory entries and sums file sizes. Deleted-but-open files, sparse files, and hidden mounts create divergence.
7. Inode = the metadata structure for a file (permissions, size, block pointers). Failures: inode exhaustion (`ENOSPC` with free bytes) and running out with millions of small files.
8. `%wa` is the share of CPU time waiting for I/O; high `wa` means the workload is I/O-bound or the storage is slow — check `iostat -x` for the device.
9. RSS = resident pages actually in RAM; VSZ = total virtual address space (includes mapped-but-untouched, shared libs, mmaps). VSZ is almost always larger and mostly meaningless for pressure.
10. A page fault is a reference to a page not currently mapped. Minor: resolved without disk (page cache, COW). Major: requires disk read (swap-in, mmap'd file). Major faults are the latency signal.
11. `si/so` are pages swapped in/out per second. It is not always bad: brief swapping during startup is normal. Sustained `si/so` with high `wa` means memory pressure with real cost.
12. Mostly oom_score (from RSS and other factors, adjustable via `oom_score_adj`), preferring to kill the process that frees the most memory; it picks per badness, not "the biggest process."
13. A kernel mechanism to group processes and account/limit resources (CPU, memory, I/O, pids). Containers use cgroups for resource limits and namespaces for isolation.
14. A namespace virtualizes a kernel resource so a process sees its own instance. Types: pid, net, mnt, uts, ipc, user, cgroup.
15. A process that exited but whose parent has not called `wait()`; it holds only a PID/exit status entry. Caused by a parent that never reaps; usually fixed by fixing or killing the parent.
16. Niceness adjusts scheduling weight (lower nice = higher priority) among same-user processes on CPU contention; it does not override cgroup CPU limits or I/O priority.
17. SIGTERM: polite request, handlers can clean up (default for `kill`). SIGKILL: immediate, uncatchable — may lose buffered data. SIGHUP: conventionally "reload config" for daemons (nginx, sshd).
18. Uninterruptible sleep — blocked in a syscall that cannot be aborted (usually NFS/disk). Signals are delivered but not acted on until the syscall returns; killing the process will not unstick the I/O.
19. Loaded unit file, Active state with result, the last exit code/status, and recent journal lines. Read the exit code/status and the journal lines first.
20. Declares that the unit requires the path to be a mounted filesystem, adding both a requirement and ordering dependency on the mount unit.
21. Journal entries for that unit, restricted to the current boot — the unit's startup and failure history.
22. Journald stores structured, indexed logs (binary) with `journalctl` query semantics; `/var/log` files are plain text written by syslog/rsyslog. `SystemMaxUse` in `journald.conf` and time-based vacuum control journald's disk use.
23. Open file descriptors per process. Modern equivalent: `LimitNOFILE=` in the systemd unit (or the cgroup/pids and fs.nr_open sysctls at the system level).
24. Files open by a process whose link count is 0 — deleted-but-open files holding allocated space.
25. PSI (`/proc/pressure/{cpu,memory,io}`) reports the fraction of time tasks are stalled waiting for a resource — a direct "are we suffering" measure, unlike load average which mixes runnable and blocked tasks.

---

## Networking (P2) — 25 questions

1. What does a /24 mean, and how many usable hosts in 10.0.0.0/24?
2. Compute the network and broadcast address of 10.10.5.77/26.
3. What is the difference between a switch (L2) and a router (L3) decision?
4. What does ARP do, and what does a partial ARP entry (incomplete) suggest?
5. What are the TCP handshake steps, and what does a SYN flood target?
6. What is the difference between TIME_WAIT and CLOSE_WAIT?
7. What is the TCP backlog, and what symptom does a full accept queue produce?
8. What is MTU, what is the default for Ethernet, and what happens with a mismatch?
9. What is PMTUD, and what breaks it?
10. What does `ss -s` tell you at a glance?
11. What is conntrack, and what happens when its table fills?
12. What does a NAT do, and where does conntrack live in the flow?
13. What is a DNS A record vs CNAME, and why do CNAME chains cost latency?
14. What is a DNS TTL, and what does a low TTL cost?
15. What does `dig +trace` show?
16. What is the difference between a public and a private DNS zone split view?
17. What are the TLS handshake steps (1.2 vs 1.3, roughly)?
18. What three things does a TLS client verify about a server certificate?
19. What is SNI, and why does it matter for one-IP-many-sites?
20. What is the difference between `iptables` and `nftables`?
21. What is the default policy vs an explicit DROP rule difference that causes "stealth" failures?
22. What does a subnet mask mismatch between two hosts cause?
23. What is a VLAN, and what does a trunk carry?
24. What is the difference between latency, bandwidth, and throughput?
25. What is a route table's default route, and how do you see the one your host uses?

### Answers — Networking

1. 24 network bits, 8 host bits → 256 addresses, 254 usable (network + broadcast reserved).
2. /26 = 64 addresses per block; 77 falls in 10.10.5.64–127 → network 10.10.5.64, broadcast 10.10.5.127.
3. A switch forwards frames within a broadcast domain by MAC; a router forwards packets between networks by IP, rewriting L2 at each hop.
4. ARP maps an IP to a MAC on the local segment. An `(incomplete)` entry means ARP requests got no reply — the host is down, on another segment, or filtered.
5. SYN → SYN-ACK → ACK. A SYN flood fills the server's half-open/accept queue with unacknowledged SYNs, denying service.
6. TIME_WAIT is on the side that **closed first** (client usually), lingering ~60 s to absorb retransmitted FINs. CLOSE_WAIT is on the side that received the close and hasn't closed its socket — an application bug (leaked sockets).
7. `somaxconn`/listen backlog is the queue of completed-but-not-yet-accepted connections. A full queue drops or resets new SYNs → periodic connection failures under burst load.
8. MTU is the max payload size of a link-layer frame; Ethernet default 1500. A mismatch causes fragmentation or, with DF set and no ICMP feedback, a PMTUD blackhole (size-dependent stalls).
9. PMTUD discovers the path MTU via ICMP Frag Needed. Broken by any device that filters ICMP or silently drops oversized packets (tunnels, firewalls).
10. Totals by socket state (estab, timewait, closewait...) — the fastest "is this box leaking sockets / which state is anomalous" check.
11. The kernel's connection-tracking table (used by NAT/stateful firewalls). When full, new connections are dropped — symptoms appear as random new-connection failures under load.
12. Rewrites addresses (and ports) to map many internal hosts to fewer public IPs. Conntrack lives in the NAT device's kernel: it must see both directions of every flow.
13. A maps a name to an IP; CNAME maps a name to another name (alias). Each CNAME hop is an extra lookup round trip before the A record is fetched.
14. TTL is how long a resolver may cache the record. Low TTL = fast propagation but more resolver load and lookup latency; high TTL = the opposite.
15. The full delegation chain from root → TLD → authoritative servers, showing where resolution actually happens — the tool for "my record is right but lookups fail."
16. A split view serves different answers per client network (internal IPs internally, public IPs externally). Misconfiguration here is a classic "works on the VPN, fails outside" cause.
17. 1.2: ClientHello → ServerHello+cert → key exchange → ChangeCipherSpec/Finished (2 RTT). 1.3: merges into ~1 RTT with key share in ClientHello, encrypts most of the handshake.
18. Chain to a trusted root, validity window (not expired), and hostname/SAN match. (Plus optional revocation/protocol checks.)
19. SNI sends the requested hostname in the ClientHello so the server can select the right certificate — required for multiple sites on one IP.
20. `nftables` is the modern replacement: unified syntax, sets/maps, better performance. `iptables` (legacy or via nft translation) still dominates older distros' docs.
21. A DROP default with no allow rule for a port causes silent timeouts; a REJECT causes immediate refused connections. Policy-vs-rule thinking prevents "the service is up but unreachable" mysteries.
22. Hosts on the same segment with different subnet masks disagree about which destinations are local → ARP vs gateway confusion, partial reachability.
23. A VLAN partitions one physical L2 into isolated broadcast domains (802.1Q tags); a trunk carries multiple VLANs on one link.
24. Latency is time per round trip; bandwidth is the pipe's capacity; throughput is what you actually achieve (limited by latency, loss, and protocol windows).
25. The route used when no more-specific route matches (`0.0.0.0/0`). See it with `ip route show default` or `ip route get <destination>`.

---

## Virtualization, storage, and cloud concepts (P3/P5-lite) — 20 questions

1. What does a hypervisor provide to a guest, and what is paravirtualization?
2. What is the difference between full virtualization and containers?
3. What is a qcow2 image's copy-on-write benefit?
4. What is an initrd/initramfs for?
5. What is the difference between a block device and a filesystem?
6. What does LVM give you over raw partitions?
7. What is RAID 1 vs RAID 5 vs RAID 10 in one line each?
8. What is a filesystem journal for?
9. What is NFS's root squash?
10. What is Ceph's CRUSH rule for?
11. What is an iSCSI target vs initiator?
12. What does NVMe-oF add over iSCSI?
13. What is a cloud availability zone, and why does cross-AZ traffic cost money?
14. What is the difference between a security group and a network ACL (conceptually)?
15. What is an instance metadata service, and why is it a security concern?
16. What is an AMI/image, and what is a snapshot vs an image?
17. What does an elastic IP cost when attached vs unattached?
18. What is a VPC endpoint, and which two kinds exist?
19. What is spot vs reserved vs on-demand in one line each?
20. What is a quota/limit in a cloud account, and why does it break launches?

### Answers — Virtualization/storage/cloud

1. A hypervisor presents virtual hardware (CPU with VT-x/AMD-V, memory with EPT/NPT, devices) to guests. Paravirtualization exposes a cooperative interface (virtio) so the guest knows it is virtual and avoids expensive emulated device traps.
2. Full virtualization runs a guest OS on virtual hardware with its own kernel; containers share the host kernel and isolate with namespaces/cgroups — faster and lighter, weaker isolation boundary.
3. qcow2 stores only the differences from a base image; clones start instantly and share the base until written, saving space and launch time.
4. An early-userspace RAM disk with the drivers needed to mount the real root filesystem (storage, network, crypto).
5. A block device is a raw sequence of sectors (`/dev/sdb`); a filesystem is the structure written on it that gives files, names, and permissions. The same block device can hold any filesystem — or be raw (databases sometimes prefer raw/their own journaling).
6. Volume grouping, online resize, snapshots, striping/mirroring across PVs, and thin provisioning without repartitioning.
7. RAID 1: mirror (2+ copies). RAID 5: striping with one parity block per stripe (survives 1 disk). RAID 10: mirror sets, striped (fast, survives multiple disks if not in the same mirror).
8. A journal records metadata (and optionally data) writes before committing them, so a crash can be replayed consistently instead of running a full fsck.
9. NFS maps client root (uid 0) to an unprivileged user on the server, preventing clients from having root power over the export.
10. CRUSH maps data to OSDs by rule (failure domain = host/rack/room), so placement is deterministic and scalable without a central placement table.
11. Target = the server exporting a block device over iSCSI; initiator = the client consuming it as a local disk.
12. NVMe protocol semantics over a fabric (RDMA/TCP): lower latency, more queues, closer-to-local performance than SCSI-based iSCSI.
13. An AZ is an isolated location with independent power/networking inside a region. Cross-AZ traffic is billed (data transfer) and adds latency, which is why chatty architectures care about placement.
14. A security group is a stateful firewall attached to instances (allow rules only); a network ACL is stateless and attached to subnets (allow and deny, ordered rules).
15. IMDS provides instance identity/credentials at a link-local address; exposed SSRF surface — hence IMDSv2 (token-based, hop-limited).
16. An AMI is a bootable template (root volume snapshot + launch config). A snapshot is a point-in-time copy of a volume; an image wraps a snapshot + metadata to be launchable.
17. An attached EIP is usually free while the instance runs; an **unattached** EIP is billed — a classic surprise line item.
18. A private path from a VPC to an AWS service without internet transit: interface endpoints (ENIs, per-subnet, support most services) and gateway endpoints (route-table entries, S3/DynamoDB, free).
19. Spot: unused capacity at deep discount, can be reclaimed with 2 min notice. Reserved/Savings Plan: commit for 1–3 years for a discount. On-demand: list price, no commitment.
20. A service limit (vCPUs, IPs, volumes, roles). Launches fail once hit with `LimitExceeded`/quota errors — check quotas **before** scaling events, not during them.

---

## Containers and Kubernetes (P4) — 25 questions

1. What does an image layer contain, and why do layers make builds fast?
2. What is the difference between `CMD` and `ENTRYPOINT`?
3. Why is `latest` a dangerous tag in production?
4. What does a container share with the host kernel, and what does it not share?
5. What does a cgroup limit do that a nice value cannot?
6. What is the Kubernetes control plane's data store, and what is stored in it?
7. What does the kubelet do?
8. What is the difference between a Deployment, a ReplicaSet, and a Pod?
9. What are resource requests vs limits, and what does the scheduler use?
10. What does OOMKilled exit code 137 mean?
11. What is a readiness probe vs a liveness probe?
12. What is a PodDisruptionBudget for?
13. What does `kubectl logs --previous` show?
14. What is a headless Service, and what uses it?
15. What is kube-proxy's job, and what are iptables vs IPVS modes?
16. What is a NetworkPolicy, and what must the CNI do for it to work?
17. What is a ConfigMap vs a Secret, and what is a Secret not?
18. What is a PersistentVolume vs a PersistentVolumeClaim vs a StorageClass?
19. What does `kubectl rollout undo` do, and what does it not do?
20. What is a taint vs a toleration?
21. What is a node affinity required vs preferred rule?
22. What does an init container guarantee?
23. What does the scheduler consider that the device plugin does not?
24. What is an extended resource, and can it be fractional?
25. What is `kubectl describe node`'s Allocated resources section telling you?

### Answers — Containers/Kubernetes

1. A layer is a filesystem diff (files added/changed) with metadata; layers are cached and shared, so rebuilds only re-run changed steps and pulls only fetch missing layers.
2. `ENTRYPOINT` is the executable; `CMD` is its default arguments (overridden by command in the run). Together they define what runs.
3. `latest` is mutable: what it points to changes without your knowledge, breaking reproducibility and rollback. Pin by digest.
4. Shares: the kernel, its modules, sysctls (partially), and hardware. Does not share: filesystem, PID/network/user namespaces (by default), mounts.
5. A cgroup enforces hard resource ceilings (CPU quota, memory limit → OOM kill, I/O limits) at the kernel level; nice only changes scheduling priority when contending.
6. etcd. All API objects: pods, deployments, services, secrets, RBAC, events (if not offloaded). Lose etcd and you lose the cluster state.
7. The node agent: watches for pods assigned to its node, creates/destroys containers via the runtime, runs probes, mounts volumes, reports node/pod status.
8. A Pod is the smallest unit (containers sharing net/ipc). A ReplicaSet maintains N pods matching a selector. A Deployment manages ReplicaSets for rollouts/rollbacks.
9. Requests: guaranteed minimum used by the scheduler for placement. Limits: max allowed; exceeding memory limits → OOM kill; CPU over limit → throttling. The scheduler uses **requests**.
10. The process was killed by the kernel's OOM killer for exceeding its memory limit (SIGKILL → 128+9=137).
11. Readiness: may this pod receive traffic now? (gates Service endpoints). Liveness: is this container alive, restart it if not? Confusing them restarts healthy-but-slow pods.
12. The minimum/maximum number of pods of a set that must stay available during **voluntary** disruptions (drains, rollouts) — it does not protect against involuntary failures.
13. The logs of the previous (crashed) container instance — the dying words for CrashLoopBackOff.
14. A Service with `clusterIP: None` that returns pod IPs directly via DNS; used by StatefulSets (stable identity), gRPC client-side balancing, and headless discovery.
15. Program the node's forwarding so ClusterIPs resolve to pod IPs: iptables mode uses random DNAT rules; IPVS uses kernel IPVS with better scalability and balancing algorithms.
16. An L3/L4 allowlist between selected pods. It only works if the CNI implements it — Calico/Cilium do; some default CNIs do not. **Test, don't assume.**
17. ConfigMap: non-sensitive config. Secret: same storage (etcd) with base64 encoding — readable by anyone with `get secrets` unless etcd encryption + RBAC are configured.
18. PV: a piece of storage in the cluster. PVC: a user's request for storage (size/access modes). StorageClass: how to dynamically provision a PV for a PVC.
19. Reverts the Deployment to the previous ReplicaSet's template. It does not restore data, undo migrations, or fix stateful side effects.
20. A taint repels pods from a node unless the pod tolerates it; a toleration is the pod-side permission to land on (or stay on) a tainted node.
21. Required: must match, or the pod is not scheduled. Preferred: scored/weighted; the pod schedules elsewhere if unmatched.
22. Runs to completion **before** app containers start; failures block pod start (unlike a sidecar). Used for waits, migrations, and setup.
23. Pod-level CPU/memory fit, taints, affinity, topology spread, PVC zone, priorities. Device plugin: device discovery, health, allocation.
24. A non-standard resource advertised by a node (e.g., `nvidia.com/gpu`). Integer only; requests must equal limits; no overcommit.
25. The sum of pod **requests** vs allocatable per resource — the scheduler's ledger. It is not actual usage; `kubectl top` is usage.

---

## Automation and CI/CD (P7) — 20 questions

1. What does idempotent mean for an Ansible task, and how do you prove it?
2. What is the difference between `--check` and `--diff`?
3. What is an Ansible handler, and when does it run?
4. What is `become` for, and what are the least-privilege options?
5. Why is `latest` a problem in `apt`/`package` tasks?
6. What is a vault-encrypted variable, and what is the alternative (better) pattern in CI?
7. What is a Python virtualenv for, and what do you pin in it?
8. What does `set -euo pipefail` do, piece by piece?
9. What is the difference between `"$VAR"` and `$VAR` unquoted in Bash?
10. What is a subshell, and what is the classic variable-modification-in-a-pipeline bug?
11. What is a trap in Bash used for?
12. What does `xargs -P` do, and what breaks with parallel xargs?
13. What is a GitHub Actions OIDC token, and why is it better than a stored cloud key?
14. What is a protected environment/gate in CI, and what does it enforce?
15. What is a dry-run in deploy tooling, and why is it not proof?
16. What is rollback-first design in a deploy pipeline?
17. What does `terraform plan` show, and what does `terraform apply -auto-approve` risk?
18. What is drift, and how do you detect it?
19. What is a runbook, and what makes one useful during an incident?
20. What is the difference between a playbook that converges state and a script that performs actions?

### Answers — Automation/CI

1. Running it again changes nothing further. Prove it: converge twice and show the second run reports `ok` (no `changed`) — capture both runs.
2. `--check` simulates (no changes); `--diff` shows the file/config changes that would occur. Use both for review; neither is proof the real run is safe.
3. A task linked to another task (usually a service restart) that runs once at the end of the play **only if** notified — deduplicating repeated restarts.
4. Privilege escalation to another user (usually root). Least privilege: `become_user` with limited sudo rights, not blanket root; separate admin vs app users.
5. `state: latest` hides the version in use; a later run may upgrade unpredictably and break reproducibility. Pin versions (or use `state: present` with a version variable).
6. Vault encrypts secrets at rest in the repo. Better in CI: the CI's secret store injects them as env at runtime; nothing secret lives in Git.
7. An isolated Python environment (site-packages + interpreter) so projects don't share dependency versions. Pin with a lock (`pip freeze > requirements.txt`, or better, a resolver lock file).
8. `-e` exit on error; `-u` error on unset variables; `-o pipefail` a pipeline fails if any stage fails (not just the last). Together: fail fast and loudly.
9. Quoted preserves spaces/emptiness as one argument; unquoted word-splits on IFS and glob-expands. `rm -rf "$dir"` vs `rm -rf $dir` are different programs.
10. A subshell is a child process; variables set inside it don't persist in the parent. Classic bug: `cat file | while read x; do COUNT=$((COUNT+1)); done` — `COUNT` is lost (the `while` runs in a subshell).
11. Runs a function on a signal/exit — used for cleanup (temp files, kill children, unlock).
12. Runs N processes in parallel. Breaks when order matters, when commands read stdin, or when you need results aggregated — capture output per task instead.
13. A short-lived OIDC token minted by GitHub, exchanged with the cloud for temporary credentials via a trust policy restricted to repo/branch/environment. No long-lived key in the repo.
14. A deployment environment with required reviewers (and optionally branch restrictions). Enforces human approval before production deploys.
15. Shows what would change. It is not proof: the world can change between plan and apply, and some effects are only visible at apply time. Still run it; then use apply with explicit review.
16. The pipeline knows how to return to the last good state (image/version/infra) before it learns to move forward; rollback is one action, not a rebuild.
17. `plan` shows the diff between desired config and real state. `-auto-approve` skips the human gate — fine for ephemeral lab environments, dangerous for shared infra.
18. Real infrastructure differing from declared configuration (manual changes, failed applies). Detect with `plan` (shows diffs) or drift-detection jobs.
19. A step-by-step operational guide for a specific failure: symptoms, diagnosis commands, mitigation, verification, escalation. Useful = testable by someone who didn't write it.
20. Convergent: describes desired state, safe to re-run, self-correcting. Action script: imperative, order-dependent, re-running may repeat side effects. Prefer convergent for platform state.

---

## Observability and reliability (P8) — 25 questions

1. What are the four signals and what question does each answer best?
2. Why does Prometheus pull, and what is the cost of pulling?
3. What does `up == 0` prove, and what does it not prove?
4. Why is a counter graphed with `rate()`?
5. What is the difference between `rate()` and `irate()`?
6. What are the four metric types?
7. Why can `histogram_quantile` mislead?
8. What is cardinality, and what makes it explode?
9. What is a recording rule, and its new failure mode?
10. What is a service-level indicator (SLI)?
11. What is an error budget, and what is it for?
12. Write the burn-rate formula, and why multiwindow alerts need two windows.
13. What is Little's law and one use?
14. Why does latency explode near 100% utilization?
15. What is the USE method?
16. What is a canary deploy, and what guardrail ends it?
17. What are the four incident roles, and what does the IC own?
18. What is a blameless postmortem, and what replaces "human error"?
19. What is a toil metric for alerts (precision/recall idea)?
20. What is the difference between a symptom alert and a cause alert?
21. What does `absent()` catch that a normal query does not?
22. Why is a Kubernetes Secret not secret by default?
23. What is the difference between Grafana proxy vs direct data source access?
24. What is the difference between a silence and an inhibition rule?
25. What is a runbook link's purpose on an alert?

### Answers — Observability

1. Metrics: aggregate health/trends/thresholds. Logs: detailed per-event context ("why"). Traces: request path and per-hop latency ("where"). Events/changes: what changed and when.
2. Pull centralizes discovery and health (a target that stops responding is visible), avoids exposing an ingest port on every service. Cost: the server must reach every target; short-lived jobs need pushgateway or another mechanism.
3. That the last scrape failed. Not that users are affected: other replicas may serve fine, and an app can return 500s while `up == 1`.
4. Counters only increase (plus resets); the raw value's slope depends on the window. `rate()` normalizes to per-second and handles resets.
5. `rate()` averages over the window (stable, alert-friendly); `irate()` uses the last two samples (spiky, "right now").
6. Counter (only up), gauge (up/down), histogram (bucketed observations + sum + count), summary (quantiles computed client-side).
7. Bucket boundaries and sample count: the quantile is interpolated inside a bucket; a threshold that isn't a bucket boundary or too-few samples make the estimate unreliable. Also invalid to average per-instance quantiles.
8. The number of unique label-value combinations (series). Raw paths, IDs, emails, error text explode it; memory/scrape cost/query time follow.
9. Precomputes an expression into a named series. New failure: the rule itself can fail/go stale while looking healthy — monitor rule health/lastEvaluation.
10. A measurable quantity of service behavior (e.g., fraction of requests < 300 ms, or non-5xx ratio) — the input to an SLO.
11. `1 − SLO` over the window: the amount of badness you may spend. It mediates reliability vs velocity: out of budget → slow down; plenty left → ship.
12. `burn = observed_error_ratio / (1 − SLO)`. A long window proves sustained burn; a short window proves it's still happening. Both required reduces flapping and stale pages.
13. `L = λW`: concurrency = arrival rate × time in system. Sets minimum workers/connections: 500 req/s × 0.12 s = 60 concurrent → pool of 60+.
14. Queueing: wait time ≈ `S/(1−ρ)`; as utilization ρ → 1, wait → ∞. At 90% utilization, wait is already 9× service time.
15. For every resource: check Utilization, Saturation, Errors. A systematic way to find the saturated resource instead of guessing.
16. A small percentage of traffic to the new version with the same real load; a guardrail metric (error ratio, p95, business KPI) breaching its threshold ends the canary (rollback).
17. IC (owns incident, severity, decisions), ops lead (technical investigation/mitigation), comms lead (updates/stakeholders), scribe (timeline/evidence). IC owns the incident, not the fix.
18. Analysis that examines system contributing factors without blaming individuals. "Human error" is replaced by "what made the action easy, likely, undetected, or hard to mitigate."
19. For alerts: how many pages were actionable (precision) and how many real incidents were caught (recall). Bad precision → alert fatigue; bad recall → missed incidents.
20. Symptom: the user-visible condition (latency/errors down). Cause: a mechanism (CPU, connections, queue). Page on symptoms; keep causes as context/diagnostics, not pages.
21. A missing series entirely (target gone, exporter dead, metric renamed). Normal comparisons return "no data" and quietly stop alerting; `absent()` turns "nothing" into an alert.
22. It is base64-encoded in etcd, readable by anyone with `get secrets` in the namespace. Encryption at rest and RBAC are separate controls; treat "Secret" as "config with a naming hint."
23. Proxy: queries flow Grafana-server → datasource (server-side creds, browser needs no access). Direct: browser → datasource (needs network reachability, exposes creds to the browser).
24. A silence is a manual time-boxed suppression of matching alerts. An inhibition is a causal rule: suppress target alerts while a source alert is active (e.g., node down suppresses its pods' alerts).
25. It puts the diagnosis/mitigation procedure one click away during the incident; an alert without a runbook is a pager with no map.

---

## AI infrastructure (P9) — 20 questions

1. What are the GPU memory hierarchy levels and their trade?
2. What is a warp, and why do GPUs hide latency with many warps?
3. What is arithmetic intensity, and what is the ridge point?
4. Why is "GPU 100% utilized" not proof of useful work?
5. What are the memory components of a training step besides activations?
6. What does mixed precision save, and what does the loss scaler protect?
7. What is gradient checkpointing trading for memory?
8. Name the four parallelism strategies and their communication pattern.
9. Why is tensor parallel usually within an NVLink domain?
10. Write the ring all-reduce bytes per rank and the busbw factor.
11. What is the difference between algbw and busbw?
12. What does GPUDirect RDMA bypass?
13. Why does RoCE need PFC/ECN?
14. What does the Kubernetes device plugin do, and not do?
15. Why must GPU requests equal limits?
16. What does MIG provide that time-slicing does not?
17. What is Slurm fair-share, and how does it differ from a quota?
18. What does Slurm `REASON=Resources` mean?
19. What is continuous batching, and what does PagedAttention optimize?
20. Define TTFT and TPOT, and what rising TTFT with flat TPOT indicates.

### Answers — AI infra

1. Registers → shared memory → L2 → HBM: capacity grows, speed falls; the working set you can keep near the SM decides speed.
2. 32 threads executing in lockstep on an SM; many resident warps let the SM switch to ready work while others wait on memory.
3. FLOPs per byte moved from memory; the ridge is `peak_FLOPs / peak_bandwidth` — above it you're compute-bound, below it memory-bound.
4. It measures whether any kernel was resident during the sample window, not whether tensor math was efficient or the SMs were occupied.
5. Gradients (`P × dtype`) and optimizer states (AdamW: m, v at fp32 = `P × 8` bytes; plus fp32 master weights under mixed precision = `P × 4`).
6. Halves weight/gradient bytes (2 bytes vs 4) and enables tensor cores; the loss scaler protects FP16 gradients from underflow (BF16's exponent range mostly avoids this).
7. Recomputes activations during backward instead of storing them: ~33% more forward compute for much less activation memory.
8. Data: all-reduce gradients per step. Tensor: per-layer all-gather/all-reduce. Pipeline: point-to-point activations. Expert: all-to-all routing.
9. It is chatty per layer and latency-sensitive; NVLink's bandwidth/latency absorb it, PCIe/Ethernet do not.
10. Each rank sends `2S(N−1)/N` bytes; `busbw = algbw × 2(N−1)/N`.
11. `algbw = S/time` (application view); `busbw` normalizes to the ring's actual link traffic — the number to compare against link bandwidth.
12. The CPU and host-memory staging copies: an RDMA NIC reads/writes GPU memory directly.
13. Lossless Ethernet needs priority flow control/ECN to avoid congestion collapse for RDMA; without them, drops force retransmits and throughput craters.
14. Advertises devices, reports health, allocates device IDs/env/mounts. Does not: schedule pods, enforce GPU memory limits, partition devices.
15. GPU allocation is an exclusive device reservation; devices cannot appear/disappear, so asymmetric requests are invalid.
16. Hardware-partitioned SMs/memory with fault and memory isolation; time-slicing shares one GPU with no memory isolation.
17. A scheduling weight that decays with historical usage; a quota is an enforced limit (MaxTRES) independent of history.
18. The job is pending because the requested resources are not free in that partition — capacity, not priority or policy.
19. Continuous batching admits/retires requests each decode step instead of batching by the longest sequence; PagedAttention stores KV in blocks to cut fragmentation and enable sharing.
20. TTFT: arrival→first token; TPOT: time per output token. TTFT up + TPOT flat = queueing/prefill (long prompts, cold cache); TPOT up = decode-side problem.

---

## Self-grading guide

| Band | Meaning |
|---|---|
| ≥90% in a domain | Ready; use spaced repetition monthly |
| 70–89% | Good; reread the specific notes for misses |
| <70% | The domain needs a lab pass, not more reading — book time on the VM |
