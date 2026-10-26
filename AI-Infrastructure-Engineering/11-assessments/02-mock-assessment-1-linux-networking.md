# Mock Assessment 1 — Linux, Networking, and Bash

> **Format:** 60 minutes, closed book. 100 points: rapid questions (40), calculations (20), scenarios (40). Target: **≥75**. The answer key follows; score with its reasoning, not just the letter.
>
> **Rules:** no notes, no CLI, no search. Where an exact output depends on your environment, answer with the evidence you would collect and the decision rule.

---

## Part A — Rapid questions (20 × 2 = 40 points)

1. What is the difference between 644 and 600 permissions, and when is each right?
2. What command shows which processes have a file open, and what option finds deleted-but-open files?
3. What does load average measure, and what does a high load with idle CPU suggest?
4. What does the `wa` column in `vmstat` mean?
5. What is the difference between RSS and VSZ?
6. What happens to a process's children when its parent dies?
7. What does `kill -9` not do that `kill -15` does?
8. What does `journalctl -u myapp --since "-10min"` show?
9. What does `systemctl enable` do, and what does it not do?
10. What does `df -i` measure, and what error does its exhaustion produce?
11. What is a hard link, and when does it fail?
12. What does `ip route get 10.0.0.5` do?
13. What is the difference between a `connection refused` and a `connection timed out` at TCP level?
14. What does `ss -tan state established | wc -l` count?
15. What does TTL mean in a DNS record, and what is the trade of a low TTL?
16. What does a CNAME chain cost at resolution time?
17. Name the three checks a TLS client performs on a server certificate.
18. What does `iptables -P INPUT DROP` change about unmatched traffic?
19. What is a /27 subnet's size in addresses and usable hosts?
20. What does `set -euo pipefail` do in one sentence?

## Part B — Calculations (5 × 4 = 20 points)

### B1 — Subnet math

Given `10.20.30.45/27`: what are the network address, broadcast address, first and last usable host, and total usable count?

### B2 — Disk budget

A 500 GB volume holds logs at 2.4 GB/hour. Retention policy keeps 7 days. Log compression reduces 2.4 GB/hour to 0.7 GB/hour effective storage.

1. Uncompressed retention size?
2. Compressed retention size?
3. If the volume also holds 120 GB of other data and needs 15% free headroom, does compressed retention fit?

### B3 — Bandwidth and transfer time

A 40 GB dataset must move over a link that delivers a sustained 180 MB/s. Protocol overhead wastes 8%.

1. Effective transfer rate?
2. Transfer time?
3. If latency is 12 ms RTT and the transfer uses a single TCP stream with 256 KB window, what is the theoretical max throughput (bandwidth-delay product view)? Does the window limit apply here?

### B4 — Connection math

A service handles 400 req/s with an average request duration of 250 ms.

1. What is the average concurrency (Little's law)?
2. If each request holds one database connection, what is the minimum pool size to avoid queueing?
3. If the pool has 300 connections, what happens at peak?

### B5 — Inode budget

A filesystem has 4,000,000 inodes. A CI job creates 120,000 files per run, deleting them at the end. The cleanup script has a bug and leaves 5% of files behind.

1. How many runs before the filesystem exhausts inodes?
2. What symptom would users see?
3. Name two detections that catch this before exhaustion.

## Part C — Scenarios (4 × 10 = 40 points)

For each: evidence you collect first (3), two likely causes (2), the smallest safe next action (3), prevention (2).

### C1 — The web server is unreachable after a failover

A web server failed over to a standby node. The VIP moved. Internal clients get connection timeouts; the health check from the load balancer passes.

### C2 — The service restarts every 90 seconds

A containerized service is Running but restarts every ~90 seconds with exit code 137. Memory limit is 512 Mi. `kubectl top` shows ~480 Mi steady, with spikes to 640 Mi during GC.

### C3 — The nightly backup is slow some nights

A backup job usually finishes in 40 minutes. On some nights it takes 3+ hours, with no error. The job reads from an NFS mount and writes to local disk. The slow nights are unpredictable, and local disk I/O looks normal.

### C4 — The new internal hostname does not resolve

A new internal hostname `metrics.internal.example.com` fails to resolve from some hosts but resolves from others. The DNS record exists in the internal zone. TTL is 300.

---

# ANSWER KEY — Mock 1

## Part A

1. 644 = owner rw, group/other r (shared-readable configs/content); 600 = owner rw only (keys, credentials, personal files). Anything with a secret or a per-user file should be 600.
2. `lsof <path>`; `lsof +L1` lists link-count-0 (deleted) open files.
3. Runnable + uninterruptible tasks averaged over 1/5/15 min. High load + idle CPU = tasks waiting (I/O, D-state, lock contention), not CPU work.
4. Percent of CPU time waiting for I/O — the I/O-bound signature.
5. RSS = resident pages in RAM; VSZ = virtual address space including mapped-but-untouched and shared mappings. RSS is the pressure signal.
6. They are reparented to init/systemd (PID 1), which reaps them.
7. SIGKILL cannot be caught/handled — no cleanup handlers run, buffers may be lost. SIGTERM lets the process close files, flush, release locks.
8. Journal entries for that unit in the last 10 minutes — startup/error lines with timestamps.
9. Creates the symlink into the enablement directory so the unit starts at boot (per its WantedBy). It does not start the unit now (`start` does that).
10. Inode count/usage. Exhaustion produces `ENOSPC` ("No space left on device") even with free bytes.
11. A second directory entry for the same inode. Cannot cross filesystems, cannot link directories (without privileges).
12. Prints the route (next hop, interface, source IP) the kernel would use to reach that address — the fastest "why can't I reach it" question.
13. Refused: an active RST was received (listener absent or a reject rule). Timed out: no answer at all (drop rule, wrong route, host down, filtered).
14. The number of TCP sockets in ESTABLISHED state (including listening-adjacent filters depending on state filter) — a load/concurrency and leak check.
15. How long a resolver may cache. Low TTL = fast changes but more lookup traffic and latency; high TTL = cheaper but slower propagation.
16. Each CNAME hop is an additional resolver round trip before the final A/AAAA lookup — added latency per hop and more failure points.
17. Chain to a trusted root, validity window, hostname/SAN match.
18. Sets the default policy for unmatched packets to DROP — silent drops (timeouts) rather than explicit rejects.
19. 32 addresses, 30 usable (network + broadcast reserved).
20. Exit on error, error on unset variables, and fail a pipeline if any stage fails — fail fast, loudly, and unambiguously.

## Part B

### B1

Block size = 32 addresses. 45 mod 32 = 13 → network = 10.20.30.32, broadcast = 10.20.30.63, usable 10.20.30.33–62, usable count = 30.

### B2

1. `2.4 × 24 × 7 = 403.2 GB`.
2. `0.7 × 24 × 7 = 117.6 GB`.
3. `120 + 117.6 = 237.6 GB` used; need 15% free = 75 GB → total required ≈ 312.6 GB < 500 GB. Fits with ~187 GB spare (compressed). Uncompressed would need 523 GB — over the volume once other data is added.

### B3

1. `180 × 0.92 = 165.6 MB/s`.
2. `40,000 MB / 165.6 MB/s ≈ 241 s ≈ 4 min`.
3. BDP = `180 MB/s × 0.012 s = 2.16 MB`. A 256 KB window limits a single stream to roughly `0.25 MB / 0.012 s ≈ 21 MB/s` — far below the link. So yes, the window dominates for a single stream; parallel streams or a larger window (BBR/window scaling) would be needed to reach 165 MB/s.

### B4

1. `L = λW = 400 × 0.25 = 100` concurrent requests.
2. 100 connections minimum to serve without queueing; in practice N+ headroom (say 120+) for bursts.
3. 100 needed vs 300 available — 200 spare; the pool is not the constraint at this load. The risk is the opposite (over-provisioned pool → DB connection/memory pressure), so size from Little's law, not from fear.

### B5

1. Residue per run = `6,000` files. Inodes available = 4,000,000. Runs ≈ `4,000,000 / 6,000 ≈ 666` runs. At, say, 40 runs/day, ~17 days.
2. `ENOSPC`/`No space left on device` on new files even though `df -h` shows bytes free; CI failures with a misleading error.
3. Alert on inode usage (`df -i` > 80%); count residue per run (assert cleanup left < N files); a weekly `find /workspace -type f | wc -l` trend.

## Part C

### C1

Evidence: ARP/neighbor table for the VIP on both nodes, `ip addr` for the VIP's presence, a packet capture on both nodes for client → VIP, and the health-check source. Likely causes: the VIP moved but gratuitous ARP did not propagate (clients/switch hold stale ARP), or an asymmetric return path/firewall on the new node. Next action: send gratuitous ARP from the new master and capture packets to confirm direction; verify the client's ARP entry. Prevention: monitor VIP ownership and ARP propagation; add a cross-subnet reachability probe to the failover test.

### C2

Evidence: `kubectl describe pod` (OOMKilled in last state), `kubectl top`/cgroup memory over time, GC pauses in app logs, and the memory limit. Likely causes: a real leak or an undersized limit with GC spikes. Next action: raise the limit to clear the immediate symptom (safe, reversible) **and** capture a memory profile/heap dump; if the leak is confirmed, fix the code, not the limit. Prevention: alert on container memory near limit; profile before raising limits routinely; set requests from P95.

### C3

Evidence: NFS server utilization/queue during slow windows, per-phase timing in the job's logs (read vs compute vs write), `iostat`/`nfsstat`, and whether the slow nights correlate with another consumer (a competing backup). Likely causes: NFS server-side contention (another client saturating it) or metadata-heavy read patterns vs local disk. Next action: timestamp each phase and diff a slow night vs a fast one; if reads dominate, stage the dataset locally first or batch metadata calls. Prevention: alert on NFS latency/queue depth; schedule competing jobs apart; measure the access pattern, not just the bandwidth.

### C4

Evidence: `dig @<authoritative-internal-server> metrics.internal...` from a failing host and a working one (bypassing local resolvers), `/etc/resolv.conf` on both, and the zone's delegation. Likely causes: different resolvers with different views/cache (one has stale/negative cache), or the failing hosts use a different resolver that doesn't have the internal zone. Next action: query the authoritative server directly from a failing host; if it answers, the resolver path is the problem, not the record. Prevention: synthetic DNS checks from each network segment; consistent resolver configuration via DHCP/automation; document split-view topology.

---

## Scoring

| Part | Your score | Max |
|---|---:|---:|
| A | | 40 |
| B | | 20 |
| C | | 40 |
| **Total** | | **100** |

**Pass ≥75.** Below 75: re-read the specific phase notes for every missed question, then re-attempt in 3 days with the variation trick (change one fact and re-derive).

**Weak-area mapping:** Part A misses → the corresponding phase notes (Linux → `01-linux-fundamentals`; Network → `02-networking`). Part B misses → rework the arithmetic by hand (subnet math, Little's law, BDP). Part C misses → the matching incident in `10-troubleshooting-workbook/02-linux-incidents.md` or `03-network-incidents.md`.
