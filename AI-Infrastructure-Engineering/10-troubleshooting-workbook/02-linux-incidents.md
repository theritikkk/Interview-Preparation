# Linux Incidents (INC-01 – INC-04)

> **Evidence first.** Each incident shows the page and the raw observations. Write your timeline, hypotheses, and the discriminating test **before** you read the analysis section. The analysis is separated by a marked line for a reason.

---

## INC-01 — Load average 40, CPU mostly idle

### The page

> **02:14 SEV2** — Batch host `prod-batch-07` (64 vCPU, 256 GB): Load average **40.2**. Users report jobs queued behind it are slow. The CPU graph looks almost flat. No paging threshold on CPU was crossed, so nobody looked for six hours.

### The evidence (raw, as collected)

You SSH in (read-only first). Collect:

```bash
uptime
vmstat 2 5
iostat -x 2 3
ps -eo pid,stat,wchan:32,pcpu,pmem,comm --sort=-pcpu | head -15
ps -eo pid,stat,wchan:32,comm | awk '$2 ~ /D/ {print}' | head
cat /proc/pressure/io
cat /proc/pressure/cpu
mount | grep -E 'nfs|overlay|xfs|ext4'
```

Output (abridged to the informative lines):

```text
$ uptime
 02:14:32 up 41 days,  3:12,  2 users,  load average: 40.18, 39.94, 39.71

$ vmstat 2 5
procs -----------memory---------- ---swap-- -----io---- -system-- ------cpu-----
 r  b   swpd   free   buff  cache   si   so    bi    bo   in   cs us sy id wa st
 3 38      0 12.1G  1.2G  180G    0    0 512004     8 2400 4100  2  3  2 93  0
 2 41      0 11.9G  1.2G  180G    0    0 523776     4 2300 3900  1  2  1 95  0
 1 39      0 12.3G  1.2G  180G    0    0 498112    12 2200 3800  2  2  2 94  0
 4 37      0 11.8G  1.2G  180G    0    0 530044     6 2500 4300  2  3  1 94  0
 2 40      0 12.0G  1.2G  180G    0    0 509880    10 2350 4000  1  2  2 95  0

$ iostat -x 2 3
Device      r/s     rkB/s   avgqu-sz  await  r_await  w_await  %util
nvme0n1    2.10   8.50      0.01      0.35     0.30     1.10    1.2
dm-0       1.80   7.20      0.01      0.40     0.35     1.20    1.0
nfs0      3980.00 512004.00  38.20   9.60     —        —      96.4
nfs0      4050.00 523776.00  41.10  10.10     —        —      97.1

$ ps -eo pid,stat,wchan:32,pcpu,pmem,comm --sort=-pcpu | head -8
  PID STAT WCHAN                          %CPU %MEM COMMAND
 8812 D   nfs_getattr                     0.4  2.1 python3
 8813 D   nfs_getattr                     0.3  2.1 python3
 8901 D   nfs_getattr                     0.2  2.0 python3
 9104 D   nfs_getattr                     0.1  2.0 python3
 7712 S   -                               0.1  0.8 java

$ cat /proc/pressure/io
some avg10=94.20 avg60=93.90 avg300=91.10
full avg10=88.10 avg60=87.90 avg300=85.20
```

Additional context from the ticket: the host mounts a large NFS share for training data; the monitoring agent shows the NFS server itself healthy; there is no swap in use; memory is fine.

### Your turn (do this before the line)

- [ ] Timeline: what happened, when, who noticed, what changed.
- [ ] Three hypotheses ranked, each with the supporting evidence.
- [ ] The one test that discriminates between your top two.
- [ ] What you do in the first 10 minutes if this is production.

---
---

# ANALYSIS — INC-01 (do not read until you have written the above)

### Timeline

Sometime in the previous evening, the data pipeline on `prod-batch-07` switched its read path from local disk to the NFS share (a change deployed the day before). Since ~01:00, worker processes are spending nearly all their time in uninterruptible waits on NFS metadata operations (`nfs_getattr`). Users noticed slowness at 02:00; nobody paged because CPU was idle and the load threshold was not configured.

### Hypotheses that fit, ranked

1. **NFS is the bottleneck (metadata-bound).** `vmstat` shows `b` (blocked) ≈ 38–41 with `wa` 93–95%, `iostat` shows the NFS device at ~96% utilization with `avgqu-sz` ≈ 38–41 and `await` ~10 ms, `ps` shows the workers in `D` state inside `nfs_getattr`, and PSI says I/O pressure is 94%. This is I/O waiting, not CPU work.
2. **A D-state deadlock / stuck mount.** Possible if the NFS server or network degraded; but the monitoring says the NFS server is healthy and the queue depth is high (meaning requests are flowing, slowly) rather than hanging forever.
3. **CPU contention.** Rejected: `us`+`sy` ≈ 3–5%, `r` run queue is small, PSI cpu would show pressure (not shown, but implied by the numbers above).

### The discriminating test

`iostat -x` on the NFS device vs the local disks: NFS at ~96% utilization with a deep queue while local NVMe is at 1% is the discriminator. A second, cheaper test: `cat /proc/pressure/io` — `full avg10=88` means **all** non-idle tasks are stalled on I/O simultaneously, which rules out "one unlucky process."

### Root cause

The workload became **metadata-bound on NFS**. Each worker calls `stat()` on thousands of small files before reading them (`nfs_getattr` in the wait channel). NFS attribute lookups are round-trips to the server; with a deep queue, every lookup costs ~10 ms, and the workers cannot pipeline them. The load average reflects tasks **waiting** for I/O, not CPU contention. This is the classic "high load, idle CPU" signature: load counts runnable **and** uninterruptible tasks.

### Fix (smallest safe first)

1. Confirm the change: `git log` / deploy history for the pipeline's read path (the next morning; for now, mitigate).
2. Mitigate now: restart the pipeline with the previous local-disk read path if that is a config flag; if not, batch the `stat()` calls (`os.scandir` once per directory instead of per-file `stat`) and reduce worker concurrency so the queue drains instead of growing.
3. If the dataset is read-heavy and small, stage it to local NVMe once (`rsync` or `tar`) and read from there; NFS is for sharing, not for per-file random access at this pattern.

### Prevention

- Alert on **PSI** (`/proc/pressure/io full avg60 > 30`) and on D-state count (`ps -eo stat | grep -c '^D'`), not on load average alone.
- Make the read-path change a reviewed config flag with a rollback.
- Add an iostat-based NFS dashboard (utilization, queue depth, await) to the host dashboard.
- Run a 5-minute load test after any data-path change, comparing tokens/s or rows/s against the baseline.

### Lessons

- **Load average includes uninterruptible waits.** High load + idle CPU = waiting, not working. Read `vmstat`'s `b` column and PSI before blaming CPU.
- **NFS metadata is a round trip.** A "fast" shared filesystem becomes slow the moment your access pattern is thousands of small `stat()` calls. Measure the access pattern, not just the bandwidth.
- **PSI is the modern signal.** `full avg10=88` is a clearer statement of "everything is stalled" than any load number.

### 5-minute walkthrough script

> "A batch host had load average 40 with an almost-flat CPU graph, so nobody paged for six hours. When I got in, the first thing I noticed was the mismatch: high load, idle CPU. `vmstat` showed the blocked queue around 40 with 94% iowait, and PSI said I/O pressure at 94%. `ps` showed all the worker processes in D state inside `nfs_getattr` — they were waiting on NFS attribute lookups, not doing compute. `iostat -x` confirmed the NFS device at 96% utilization with a deep queue while local NVMe sat at 1%. The root cause was a read-path change the day before: the pipeline went from local disk to NFS and became metadata-bound, doing a `stat()` per small file, each one a network round trip. Mitigation was to batch the metadata lookups and reduce concurrency so the queue drained; the real fix was staging the dataset to local NVMe for the read-heavy phase. The lesson I took away: load average counts tasks that are waiting, and the modern signal for that is PSI — I now alert on PSI and D-state count, not load."

---

## INC-02 — "No space left on device" with 30% free

### The page

> **11:05 SEV1** — CI runner `runner-3` fails every job with `OSError: [Errno 28] No space left on device`. `df -h` shows the root filesystem at 68% used. Jobs are blocked; the queue is growing.

### The evidence (raw, as collected)

```bash
df -h /
df -i /
df -h /tmp
du -sh /var/lib/docker 2>/dev/null
mount | grep -E 'overlay|/tmp'
journalctl -u docker --since "2 hours ago" | grep -iE 'space|error' | tail -5
find / -xdev -type f -size +1G -exec ls -lh {} \; 2>/dev/null | head
```

Output (abridged):

```text
$ df -h /
Filesystem      Size  Used Avail Use% Mounted on
/dev/nvme0n1p2  200G  132G   60G  68% /

$ df -i /
Filesystem       Inodes   IUsed   IFree IUse% Mounted on
/dev/nvme0n1p2  12800000 12798012    1988  100% /

$ df -h /tmp
Filesystem      Size  Used Avail Use% Mounted on
tmpfs            16G   11G  4.6G  70% /tmp

$ mount | grep overlay
overlay on /var/lib/docker/overlay2/... type overlay (...)

$ find / -xdev -type f -size +1G ... (nothing over 1G)
```

The runner creates and deletes tens of thousands of small files per job (test artifacts, node_modules, build intermediates).

### Your turn (before the line)

- [ ] Timeline.
- [ ] Three hypotheses ranked with evidence.
- [ ] The one discriminating test.
- [ ] First 10 minutes.

---
---

# ANALYSIS — INC-02

### Timeline

Over the previous three days, the CI runner processed unusually many small-file-heavy jobs (a new test suite). Each job creates and deletes ~400k small files. At some point, the inode count crossed 100%, and every subsequent `open()` or `creat()` failed with `ENOSPC` even though byte usage was only 68%.

### Hypotheses ranked

1. **Inode exhaustion.** `df -i` shows 100% with 1,988 free. `df -h` shows bytes available. `ENOSPC` is returned by the kernel when either budget is exhausted. Supported strongly.
2. **A different filesystem is actually full.** `df -h /tmp` is a tmpfs at 70%; if the job wrote to `/tmp` on a small tmpfs, that could be the limit. Rejected as primary: `df -h /` is the filesystem in the error path (the runner writes to `$PWD` on `/`), and the inode evidence is conclusive anyway.
3. **Quota or read-only remount.** Possible but unsupported by evidence; a read-only remount would give `EROFS`, not `ENOSPC`.

### The discriminating test

`df -i /` vs `df -h /`: two budgets, one exhausted. If both were fine, the next test would be `df -h` on the specific path's filesystem and checking quotas (`repquota`/`xfs_quota`), but the inode number is the discriminator here.

### Root cause

The filesystem ran out of **inodes**, not bytes. Each file (and directory, and symlink) consumes one inode regardless of size. Millions of tiny files from CI runs exhausted the allocation. `df` bytes and `df` inodes are separate budgets on ext4/XFS.

### Fix (smallest safe first)

1. Identify and delete the small-file hoards: `find /var/lib/docker -type f -size -1k | wc -l` to size the problem; clean Docker build cache (`docker builder prune`), old job workspaces, and package-manager caches.
2. If a specific directory is the hoard (typical: `node_modules` copies, `.git` objects, test artifacts), remove the oldest job workspaces with `find /var/lib/runner/work -maxdepth 2 -mtime +7 -exec rm -rf {} +`.
3. Long-term: either (a) grow the filesystem/inode count (ext4: `resize2fs`; XFS auto-grows inodes with space, so on XFS this symptom is rarer), or (b) move small-file-heavy workloads to a filesystem with better small-file behavior, or (c) reduce file count (zip artifacts before storage, use content-addressed caches).

### Prevention

- Alert on inode usage (`df -i` at 80%), not just byte usage.
- Cap CI workspace retention; clean up after every job (`rm -rf $WORKSPACE` in the runner's after_script).
- Prefer artifact archives over exploded trees when a job produces >100k files.
- Document the inode budget as part of capacity planning for CI hosts.

### Lessons

- **Two budgets, one error.** `ENOSPC` can mean bytes or inodes; check both before concluding anything.
- **Small files are a real cost.** A directory of a million 100-byte files costs a million inodes and megabytes of metadata; "it's tiny" is not a capacity argument.
- **Capacity alerts must match the constraint.** Alert on the budget the workload can exhaust, not the one that is easiest to graph.

### 5-minute walkthrough script

> "A CI runner failed everything with 'No space left on device' while `df -h` showed 68% used — the mismatch is the clue. `df -i` showed the inode table at 100% with under 2,000 free, while bytes had 60 GB available. Inodes and bytes are separate budgets; each file consumes one inode regardless of size, and the runner's new test suite creates around 400,000 small files per job. The fix was cleaning Docker build cache and old workspaces, and the prevention was alerting on inode usage and capping workspace retention. The lesson: when a filesystem error does not match the byte numbers, check the second budget — inodes — before anything else."

---

## INC-03 — Disk at 100% but `du` finds nothing

### The page

> **16:40 SEV2** — Log host `logs-2`: `/var` at 100%. `du -sh /var/*` totals ~20 GB on a 100 GB volume. Services on the host are erroring on writes.

### The evidence (raw, as collected)

```bash
df -h /var
du -xh --max-depth=1 /var | sort -h | tail -10
lsof +L1 /var 2>/dev/null | head -10
ls -l /proc/$(pgrep -f 'log_shipper' | head -1)/fd 2>/dev/null | head
find /var -xdev -type f -size +500M -exec ls -lh {} \; 2>/dev/null | head
```

Output (abridged):

```text
$ df -h /var
Filesystem      Size  Used Avail Use% Mounted on
/dev/nvme0n1p3  100G   99G   20M  100% /var

$ du -xh --max-depth=1 /var | sort -h | tail -5
20G     /var/lib
1.4G    /var/log
  ...
Total visible: ~22G of 99G

$ lsof +L1 /var 2>/dev/null | head -8
COMMAND     PID   USER   FD   TYPE DEVICE  SIZE/OFF NODE NAME
log_shipp  3120   root   4w   REG   8,3  74283104256  132 /var/log/app/combined.log (deleted)
log_shipp  3120   root   5w   REG   8,3  12884901888  134 /var/log/app/debug.log (deleted)
```

### Your turn (before the line)

- [ ] Timeline.
- [ ] Three hypotheses ranked with evidence.
- [ ] The one discriminating test.
- [ ] First 10 minutes.

---
---

# ANALYSIS — INC-03

### Timeline

The log shipper had been rotating and deleting files (`logrotate`), but it **held the old file open** after deletion. The kernel keeps the space allocated while any process holds a file descriptor, so `du` (which walks directory entries) cannot see the space, but `df` (which reads the filesystem's free-space bitmap) reports it as used. Over weeks, two deleted logs grew to ~87 GB.

### Hypotheses ranked

1. **Deleted-but-open files.** `lsof +L1` lists files with link count 0 that are still open, showing ~74 GB and ~13 GB held by `log_shipper`. Direct match for "df says used, du says small."
2. **Filesystem metadata/reserved blocks.** ext4 reserves ~5% for root; that could hide ~5 GB, not 77 GB. Rejected as primary.
3. **Sparse files or hidden mounts.** A mount over a directory would hide it from `du` (`du -x` already excludes cross-filesystem), and `find -size +500M` would still see it. Rejected.

### The discriminating test

`lsof +L1 /var` — it directly lists deleted-but-open files with their sizes. (`df` vs `du` mismatch is the trigger; `lsof +L1` is the proof.)

### Root cause

The log shipper opens `combined.log` once at startup and keeps writing to the same file descriptor. When logrotate renames and deletes the file, the shipper keeps appending to the now-unlinked inode, which grows without bound and is invisible to `du`.

### Fix (smallest safe first)

1. Truncate the held file descriptors instead of deleting (this frees space immediately and is safe):

```bash
# empty the file contents without closing the descriptor
> /proc/3120/fd/4
> /proc/3120/fd/5
```

2. Restart the shipper so it reopens the current log path properly.
3. Fix the rotation: either the shipper must support `SIGHUP`-based reopen (or `copytruncate` in logrotate), or use a shipper that follows renames. Verify with `lsof -p <pid> | grep deleted` returning nothing after the next rotation.

### Prevention

- Add `lsof +L1` (or a `df`–`du` divergence check) to the disk-full runbook.
- Alert on the divergence: `df_used_GB − du_visible_GB > 5 GB`.
- Prefer `copytruncate` or a rotate-signal-reopen design; never combine "delete" with "hold the fd."
- Test log rotation in staging with a large log file and assert the space is actually reclaimed.

### Lessons

- **`df` and `du` answer different questions.** `df` reads the free-space bitmap; `du` walks directory entries. Deleted-but-open files appear in one and not the other.
- **File descriptors outlive the filename.** Deleting a file does not free its space while a process holds it open; truncating the descriptor does.
- **Rotation is a contract with the writer.** If the writer does not reopen on rotation, you get an invisible file that grows forever.

### 5-minute walkthrough script

> "A log host showed /var at 100% but `du` could only account for about 22 GB of 99 GB. The gap — df says used, du says small — points at deleted-but-open files. `lsof +L1` found two deleted logs held open by the log shipper, 74 and 13 GB. The shipper opened the file once and kept writing to the descriptor after logrotate renamed and deleted it; the kernel keeps the space allocated until the descriptor closes. Immediate fix was truncating `/proc/<pid>/fd/N` to reclaim space without a restart, then restarting the shipper and fixing rotation to use copytruncate. Prevention: alert on the df–du divergence and test rotation with a large file. The lesson: deleting a file does not free space while a process holds it open — truncate the descriptor instead."

---

## INC-04 — Service fails after every reboot, works when started manually

### The page

> **07:12 SEV2** — `app-4` reboots for kernel patching at 07:00. At 07:12, the app is down. When you start it manually, it works. This is the third host this month with the same pattern.

### The evidence (raw, as collected)

```bash
systemctl status myapp.service
journalctl -u myapp.service -b --no-pager | tail -20
systemctl list-dependencies myapp.service
systemctl show myapp.service -p After,Requires,Wants,ExecStart,WorkingDirectory
systemd-analyze blame | head
ls -l /etc/systemd/system/myapp.service /etc/fstab
df -h /data
```

Output (abridged):

```text
$ systemctl status myapp.service
● myapp.service - Application server
     Loaded: loaded (/etc/systemd/system/myapp.service; enabled; preset: enabled)
     Active: failed (Result: exit-code) since Tue 07:01:44
    Process: 812 ExecStart=/usr/local/bin/myapp (code=exited, status=1/FAILURE)
   Main PID: 812 (code=exited, status=1/FAILURE)

$ journalctl -u myapp.service -b --no-pager | tail -6
Jul 07:01:41 app-4 systemd[1]: Starting myapp.service...
Jul 07:01:42 app-4 myapp[812]: config: opening /data/app/config.yaml
Jul 07:01:42 app-4 myapp[812]: error: open /data/app/config.yaml: no such file or directory
Jul 07:01:44 app-4 systemd[1]: myapp.service: Failed with result 'exit-code'.

$ systemctl show myapp.service -p After,Requires,Wants
After=sysinit.target network.target
Requires=sysinit.target
Wants=network.target

$ df -h /data
Filesystem      Size  Used Avail Use% Mounted on
/dev/sdb1        2T  340G  1.6T  18% /data

$ systemd-analyze blame | head -3
  6.2s data.mount
  ...
```

`/data` is an additional volume (`/etc/fstab` entry, `nofail` not set... actually check: `mount | grep /data` shows it mounted **now**, because you started it manually after boot and the mount raced in).

### Your turn (before the line)

- [ ] Timeline.
- [ ] Three hypotheses ranked with evidence.
- [ ] The one discriminating test.
- [ ] First 10 minutes.

---
---

# ANALYSIS — INC-04

### Timeline

The app is `enabled` and starts at boot via `sysinit.target`/`network.target` ordering. `/data` is an additional volume mounted by `data.mount` (generated from `/etc/fstab`), which takes ~6 s to appear (network-attached volume attaching, or a slow device). The app starts **before** the mount, cannot find its config, and exits 1. Manually starting it later works because by then `/data` is mounted.

### Hypotheses ranked

1. **Boot-order race: app starts before `/data` is mounted.** The journal shows `open /data/app/config.yaml: no such file or directory` at 07:01:42; `systemd-analyze blame` shows `data.mount` at 6.2 s; the unit has no `After=data.mount` or `RequiresMountsFor=/data`. Matches.
2. **A missing config file.** Possible, but the file exists after boot (the app works when started manually), so it is a timing issue, not an absence.
3. **A bad ExecStart or environment.** Rejected: the same command works manually.

### The discriminating test

Add `RequiresMountsFor=/data` (or `After=data.mount`) and reboot a test host. If it survives, the ordering hypothesis is proven. Alternatively `systemctl list-jobs` during boot shows the race directly.

### Root cause

systemd starts units in dependency order; **without an explicit ordering or mount dependency, parallel start is allowed.** The app unit only ordered after `sysinit`/`network`, not after the mount that holds its config. On a fast local disk the race is invisible; on a slower volume attach, it loses.

### Fix (smallest safe first)

1. Add to `[Unit]`:

```ini
RequiresMountsFor=/data
```

   (implies both a requirement and ordering; `Requires=data.mount` + `After=data.mount` is the more explicit form).
2. Add a startup retry/probe in the app for resilience anyway (do not rely on ordering alone for correctness).
3. Reboot a test host to verify; then roll out to the other hosts with the same pattern.

### Prevention

- Every service that reads from a non-root volume declares `RequiresMountsFor`.
- Boot-test after kernel/volume changes: a reboot is part of the change, not a separate event.
- A startup probe (or `systemd` `ExecStartPre=/usr/bin/test -f /data/app/config.yaml`) makes the dependency explicit and fails fast with a clear message.
- Consider `nofail` carefully: a `nofail` mount that silently fails leaves the service broken with a confusing error; if the volume is required, do not use `nofail`.

### Lessons

- **systemd parallelizes by default.** No ordering dependency means units start concurrently; correctness must be declared, not assumed.
- **`RequiresMountsFor` is the right tool.** It expresses "this unit needs this path to be a mounted filesystem."
- **Race conditions in boot are environment-dependent.** They appear when a volume, network, or dependency is slower than during development.

### 5-minute walkthrough script

> "Three hosts this month had a service that worked when started manually but failed at every boot. The journal showed the app couldn't open its config under /data, and `systemd-analyze blame` showed the /data mount taking 6 seconds. The unit only ordered after sysinit and network — it had no mount dependency, so systemd started it in parallel with the mount, and on a slow volume attach the app lost the race. The fix was `RequiresMountsFor=/data` plus a config-existence check as `ExecStartPre`, verified with a reboot. The lesson: systemd starts units in parallel unless you declare ordering; a service that depends on a mounted path must say so explicitly."
