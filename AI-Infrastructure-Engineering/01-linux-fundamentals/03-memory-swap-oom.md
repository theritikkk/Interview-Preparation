# Memory, Swap, Page Cache, and the OOM Killer

> **The one-sentence model:** Linux uses free RAM as a cache, gives each process a virtual address space, and only kills processes when it cannot reclaim memory fast enough. "Free memory is low" is usually the system doing its job.

---

## 1. Virtual memory in one page

Each process sees a contiguous virtual address space. The kernel maps virtual pages to physical frames through page tables.

```text
process virtual address space
┌─────────────────────────────┐  high
│ stack                       │  grows down
├─────────────────────────────┤
│ mmap regions / shared libs  │
├─────────────────────────────┤
│ heap                        │  grows up
├─────────────────────────────┤
│ BSS (zero-initialised)      │
├─────────────────────────────┤
│ data (initialised)          │
├─────────────────────────────┤
│ text (code)                 │
└─────────────────────────────┘  low
```

A virtual page may be:

- **resident** in RAM,
- **swapped out** to disk,
- **file-backed** and clean (can be dropped and re-read),
- **not yet allocated** (demand paging),
- **shared** with other processes (libraries, page cache).

This is why `VSZ` can be enormous and harmless, while `RSS` is the number that matters for pressure.

---

## 2. The metrics that matter

| Metric | What it is | Where |
|---|---|---|
| **VSZ** | Virtual size: all mapped pages | `ps`, `top` |
| **RSS** | Resident set size: pages currently in RAM | `ps`, `top`, `/proc/<pid>/status` |
| **PSS** | Proportional set size: shared pages divided by sharers | `smem`, `/proc/<pid>/smaps_rollup` |
| **USS** | Unique set size: private pages only | `smem`, `smaps_rollup` |
| **Page cache** | File contents cached in RAM | `free` `buff/cache`, `/proc/meminfo` `Cached` |
| **Buffers** | Block-device metadata | `free` `buff/cache`, `Buffers` |
| **Available** | Estimate of memory reclaimable for new workloads | `free` `available` |
| **Swap used** | Pages moved to swap | `free`, `/proc/meminfo` |
| **`si`/`so`** | Swap-in/swap-out rate (KB/s) | `vmstat` |

Summing RSS across processes overcounts shared pages. Use PSS when you need "who is really using memory".

```bash
$ smem -tk
  PID User     Command                         Swap      USS      PSS      RSS
 1042 app      node /opt/app/server.js            0   310.2M   342.1M   492.0M
  812 root     /usr/bin/containerd                0    28.4M    31.2M    34.0M
...
```

---

## 3. Reading `free` correctly

```bash
$ free -h
               total        used        free      shared  buff/cache   available
Mem:           7.7Gi       2.1Gi       1.2Gi        28Mi       4.4Gi       5.2Gi
Swap:          2.0Gi          0B       2.0Gi
```

| Column | Meaning |
|---|---|
| `total` | Physical RAM |
| `used` | Memory in use by processes (excluding reclaimable cache) |
| `free` | Completely unused RAM |
| `shared` | `tmpfs`/shared memory |
| `buff/cache` | Page cache + buffers, reclaimable under pressure |
| `available` | Best estimate of memory available to start new workloads without swapping |

The healthy reading is **`available`**, not `free`. A server with `free` = 1.2 GiB and `available` = 5.2 GiB is fine; the kernel is caching files and will reclaim them on demand.

```bash
$ cat /proc/meminfo | head -12
MemTotal:        8063224 kB
MemFree:         1260124 kB
MemAvailable:    5452100 kB
Buffers:          201124 kB
Cached:          4310220 kB
SwapCached:            0 kB
Active:          3210440 kB
Inactive:        2810220 kB
...
```

`MemAvailable` is the kernel's estimate, not a guarantee. It accounts for reclaimable page cache and most swap.

---

## 4. Page cache, buffers, and writeback

When you read a file, the kernel caches its pages. When you write, it marks pages dirty and writes them back asynchronously (`pdflush`/writeback threads).

```bash
$ dd if=/dev/zero of=/tmp/testfile bs=1M count=2048 status=progress
$ free -h
               total        used        free      shared  buff/cache   available
Mem:           7.7Gi       1.9Gi       220Mi        28Mi       5.6Gi       5.4Gi
```

The 2 GiB file is in `buff/cache`, not "used" by a process. Dropping caches is a diagnostic, not a production fix:

```bash
sync
echo 3 | sudo tee /proc/sys/vm/drop_caches
```

This can hurt performance by forcing re-reads. Never run it on a busy production host without a reason.

Dirty page limits:

```bash
$ sysctl vm.dirty_ratio vm.dirty_background_ratio vm.dirty_expire_centisecs
vm.dirty_ratio = 20
vm.dirty_background_ratio = 10
vm.dirty_expire_centisecs = 3000
```

A large dirty ratio can cause latency spikes when writeback kicks in.

---

## 5. Swap

Swap extends memory to disk. It is not a substitute for RAM; it is a safety valve and a way to move cold pages out.

```bash
$ swapon --show
NAME      TYPE      SIZE USED PRIO
/dev/sda2 partition   2G   0B   -2

$ vmstat 1
procs -----------memory---------- ---swap-- -----io---- ...
 r  b   swpd   free   buff  cache   si   so    bi    bo ...
 2  0      0  12000  20000 800000    0    0     4    20 ...
```

- Occasional `si`/`so` is normal.
- Sustained `si`/`so` with high `wa` and rising latency is **swap thrash**.
- `swappiness` controls how aggressively the kernel reclaims anonymous pages. Default 60; servers often use 10. `0` does not disable swap entirely (OOM can still swap).

```bash
sysctl vm.swappiness
sudo sysctl -w vm.swappiness=10       # temporary
# persistent: /etc/sysctl.d/99-swap.conf
```

On Kubernetes nodes, swap is usually disabled because memory limits and QoS assume no swap. Know the platform's policy before enabling it.

---

## 6. Overcommit and the OOM killer

Linux overcommits virtual memory: a process can reserve more address space than physical RAM, because most reservations are not touched.

```bash
$ cat /proc/sys/vm/overcommit_memory
0
$ cat /proc/sys/vm/overcommit_ratio
50
$ grep -E 'CommitLimit|Committed_AS' /proc/meminfo
CommitLimit:     4031612 kB
Committed_AS:    6120440 kB
```

| Value | Policy |
|---:|---|
| `0` | Heuristic overcommit (default); obvious overcommits refused |
| `1` | Always overcommit; never refuse |
| `2` | Strict: total commit ≤ `CommitLimit` |

When reclaim fails, the OOM killer selects a process by `oom_score` (roughly proportional to memory use, adjusted by `oom_score_adj`) and sends `SIGKILL`.

```bash
$ sudo dmesg -T | grep -i -E 'oom|killed process'
[Sun Mar 15 03:14:22 2026] java invoked oom-killer: gfp_mask=0x..., order=0, oom_score_adj=0
[Sun Mar 15 03:14:22 2026] Out of memory: Killed process 4242 (java) total-vm:8245120kB, anon-rss:6120440kB, file-rss:0kB, shmem-rss:0kB
[Sun Mar 15 03:14:22 2026] oom_reaper: reaped process 4242 (java), now anon-rss:0kB, file-rss:0kB
```

Read the `anon-rss` to see how much anonymous memory the victim held. `total-vm` can be huge and irrelevant.

```bash
cat /proc/<pid>/oom_score
cat /proc/<pid>/oom_score_adj
```

`oom_score_adj` ranges from `-1000` (never kill) to `1000` (kill first). Do not set `-1000` on a process that can leak; you will make the kernel kill something else.

---

## 7. cgroups: memory limits that actually apply

On a container or systemd service, the host may have plenty of memory while the cgroup is at its limit.

```bash
$ systemctl show app.service -p MemoryMax -p MemoryCurrent -p MemoryHigh
MemoryMax=1073741824
MemoryCurrent=1048576000
MemoryHigh=infinity

$ cat /sys/fs/cgroup/system.slice/app.service/memory.events
low 0
high 0
max 12
oom 3
oom_kill 3
```

| File | Meaning |
|---|---|
| `memory.max` | Hard limit; allocations beyond it trigger reclaim/OOM |
| `memory.high` | Throttle threshold; slows allocation before the hard limit |
| `memory.current` | Current usage |
| `memory.events` | Counts of `low`, `high`, `max`, `oom`, `oom_kill` |
| `memory.stat` | Detailed breakdown: anon, file, slab, etc. |

A container killed for exceeding `memory.max` gets `SIGKILL`; Kubernetes reports `OOMKilled` with exit code `137`.

```bash
$ kubectl describe pod api-7d9f | tail -20
    Last State:     Terminated
      Reason:       OOMKilled
      Exit Code:    137
    Limits:
      memory:      512Mi
    Requests:
      memory:      256Mi
```

`137 = 128 + 9`, i.e. killed by `SIGKILL`. It can also be a manual `kill -9`; the `Reason: OOMKilled` and node/cgroup evidence distinguish them.

---

## 8. Finding a memory leak

A leak is sustained growth of **anonymous** memory that does not plateau, not a large RSS.

```bash
# watch the trend, not one sample
while true; do
  date +%T
  ps -o pid,rss,vsz,cmd -p 1042
  sleep 10
done
```

Then break it down:

```bash
$ grep -E 'RssAnon|RssFile|RssShmem|Pss' /proc/1042/smaps_rollup
Rss:              492000 kB
Pss:              342100 kB
Pss_Anon:         310200 kB
Pss_File:          31900 kB
Pss_Shmem:             0 kB

$ sudo pmap -x 1042 | tail -5
```

- `RssAnon` growing → heap/native allocations, leak or workload growth.
- `RssFile` growing → mapped files/page cache, often reclaimable.
- `RssShmem` growing → `tmpfs`/shared memory.

Language-specific tools:

| Runtime | Tool |
|---|---|
| Java | `jcmd GC.heap_info`, `jmap -histo`, heap dump, `-Xmx`, Native Memory Tracking |
| Node | `--inspect`, heap snapshot, `process.memoryUsage()` |
| Python | `tracemalloc`, `objgraph`, `memray` |
| C/C++ | `valgrind massif`, `heaptrack`, ASan |
| Any | `perf record -g`, `bpftrace` (advanced) |

**Do not confuse a heap limit with RSS.** A Java process can have `-Xmx2g` and an RSS of 4 GB because of metaspace, thread stacks, direct buffers, JIT code, and native libraries. Check all of them.

---

## 9. Worked scenarios

### Scenario A — "free memory is almost zero, is the server dying?"

```bash
$ free -h
               total        used        free      shared  buff/cache   available
Mem:           7.7Gi       2.0Gi       180Mi        28Mi       5.5Gi       5.4Gi
$ vmstat 1
 r  b   swpd   free   buff  cache   si   so    bi    bo ...
 1  0      0 180000  20000 5600000    0    0     0    12 ...
```

Evidence: `available` is 5.4 GiB, no swap I/O, no blocked tasks. The kernel is caching files. No action. Adding RAM would not improve this.

### Scenario B — memory grows until the process is killed

```bash
$ dmesg -T | grep -i 'killed process'
[...] Out of memory: Killed process 4242 (java) ... anon-rss:6120440kB ...
$ ps -o pid,rss,vsz,etime,cmd -p 4242
# process is gone; check the service's restart history
$ systemctl status app
   Active: activating (auto-restart) ... (code=killed, signal=KILL)
```

Hypotheses:

1. Heap leak (`-Xmx` too high or unbounded cache).
2. Native/off-heap leak (direct buffers, JNI, thread stacks).
3. Load growth faster than capacity.
4. Container limit lower than the JVM heap + overhead.

Tests: `jcmd GC.heap_info`, heap dump, `smaps_rollup` trend, cgroup `memory.events`, compare `-Xmx` with `memory.max`. Fix the cause; adding swap usually makes it slower, not better.

### Scenario C — container OOMKilled but the node has free memory

```bash
$ kubectl get pod api-7d9f -o jsonpath='{.status.containerStatuses[0].lastState.terminated.reason}'
OOMKilled
$ kubectl exec ... -- cat /sys/fs/cgroup/memory.max
536870912
$ kubectl exec ... -- cat /sys/fs/cgroup/memory.events
max 42
oom 7
oom_kill 7
```

The cgroup limit is 512 MiB; the process tried to exceed it. Raise the limit only after checking whether the process is genuinely using more memory or leaking. A `memory.high` throttle can reduce latency spikes before the hard kill.

### Scenario D — swap thrash

```bash
$ vmstat 1
 r  b   swpd   free   buff  cache   si   so    bi    bo ...
 4 12 1900000  40000  10000 200000 18000 22000  ...
$ uptime
load average: 18.20, 16.40, 12.10
```

Sustained `si`/`so` in the tens of MB/s, high `wa`, high load. The working set does not fit in RAM. Options: reduce concurrency, add RAM, fix the leak, or move the workload. `swappiness=0` will not save a working set larger than RAM.

### Scenario E — page cache or a real leak?

```bash
$ free -h
               total        used        free      shared  buff/cache   available
Mem:           15Gi        14Gi       200Mi       1.0Gi       800Mi       400Mi
$ sudo smem -tk | head
```

If `used` is high and `buff/cache` is low, the memory is in processes (or `tmpfs`). Check `smem`, `ps` sorted by RSS/PSS, and `/proc/meminfo` `Shmem`. A large `tmpfs` (`/dev/shm`, `/run`) counts as shared memory and is not reclaimable like file cache.

---

## 10. Common mistakes

| Mistake | Why it is wrong | Better |
|---|---|---|
| Panicking at low `free` | Page cache is reclaimable | Read `available`, `vmstat`, `si/so` |
| Adding swap to fix a leak | Hides the leak and destroys latency | Find the leak; add RAM only for capacity |
| Dropping caches in production | Forces re-reads; can spike latency | Diagnose first; drop only in a lab |
| Using RSS as "memory used" | Shared pages are double-counted | PSS/USS, `smaps_rollup` |
| Setting `overcommit_memory=1` blindly | Lets allocations succeed until OOM | Understand the workload; keep heuristic |
| `oom_score_adj=-1000` on a leaky app | Kernel kills something else | Fix the leak or set a realistic limit |
| Raising a container limit without a request | Scheduler may overcommit the node | Set requests and limits; measure |
| Ignoring `tmpfs` | `/dev/shm` and `/run` consume RAM | Check `Shmem`, size mounts |
| Confusing exit 137 with OOM | 137 = SIGKILL, not necessarily OOM | Check `Reason`, cgroup events, dmesg |

---

## 11. Command reference (quick)

```bash
# overview
free -h
vmstat 1
cat /proc/meminfo
smem -tk
ps -eo pid,user,rss,vsz,pmem,cmd --sort=-rss | head

# per-process
cat /proc/PID/status
grep -E 'Rss|Pss' /proc/PID/smaps_rollup
pmap -x PID
cat /proc/PID/oom_score /proc/PID/oom_score_adj

# swap
swapon --show
sysctl vm.swappiness
cat /proc/swaps

# OOM
sudo dmesg -T | grep -i -E 'oom|killed process'
sudo journalctl -k --since '1 hour ago' | grep -i oom

# cgroup
systemctl show app.service -p MemoryMax -p MemoryCurrent -p MemoryHigh
cat /sys/fs/cgroup/.../memory.current
cat /sys/fs/cgroup/.../memory.max
cat /sys/fs/cgroup/.../memory.events
cat /sys/fs/cgroup/.../memory.stat
```

---

## 12. Checkpoint

1. Why is `free` memory low on a healthy Linux host?
2. What is the difference between RSS, PSS, and USS?
3. What does `available` estimate that `free` does not?
4. What is the difference between page cache and anonymous memory?
5. When does the OOM killer run, and how does it choose a victim?
6. What does `oom_score_adj=-1000` do, and why is it dangerous?
7. How do you tell a container OOMKill from a manual `kill -9`?
8. Why can a Java process with `-Xmx2g` have an RSS above 4 GB?
9. What evidence distinguishes swap thrash from a CPU bottleneck?
10. What is the first thing you check when a process grows until it is killed?

If you miss more than two, redo Lab L1.4.
