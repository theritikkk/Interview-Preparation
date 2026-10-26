# Roadmap A — First 30 Days

> **Outcome:** a working Linux VM, the P0 Linux/networking reflexes, one safe automation script, five incident stories, and a Mock 1 score of at least 75%.
>
> **Not the outcome:** finishing the entire syllabus. Thirty days at 12–15 h/week is a **P0 sprint**, not all of P0. You will finish P0 around day 50–60 while applying.

## Time budget

| Activity | Weekday | Weekend | Weekly |
|---|---:|---:|---:|
| Infrastructure study + lab | 1.5–2 h | 4–5 h | 12–15 h |
| DSA | 1 h | 1 h | 7 h |
| Core CS revision (OS/DBMS/CN) | — | 1–2 h | 1–2 h |
| Applications/admin | 15 min | 30 min | ~2 h |

If you have exams or a deadline, drop to the **8-hour track**: one 90-minute infra block on weekdays, one 3-hour lab on Sunday, DSA 45 min/day. Stretch the 30 days to 45; do not skip the labs.

---

## What you must be able to do by day 30

### Explain without notes

1. The Linux layers: hardware → kernel → syscall boundary → userspace → your app.
2. What `ps` state `D`, `S`, `R`, `Z` means and why `D` is dangerous.
3. Why `free -h` showing little "free" memory is usually healthy.
4. `df` vs `du`, inode exhaustion, and a deleted-but-open file.
5. The systemd boot chain and how to debug a failed unit.
6. How a packet leaves a process: socket → TCP/IP → route → ARP → NIC → switch/router.
7. The difference between `connection refused`, `timeout`, and `reset`.
8. DNS resolution and why "works by IP, fails by name" happens.
9. What NAT does to a packet and why conntrack matters.
10. How to read a subnet and answer "is this host in this network?" in under 30 seconds.

### Troubleshoot independently

- A service failed to start after a config change.
- A disk is full but `du` says there is space.
- A process is unresponsive but still in `ps`.
- A host is up but a port is unreachable.
- A name resolves but the connection times out.
- A service listens on `127.0.0.1` and is unreachable remotely.
- A permission error persists after `chmod`.

---

## Week 1 — Linux: files, permissions, processes, memory

**Goal:** stop using Linux like a guest. Own the filesystem, the process table, and the memory picture.

| Day | Focus | Read | Lab / drill | Deliverable |
|---|---|---|---|---|
| 1 | Set up the disposable VM; FHS; navigation; file operations | [Linux README](../01-linux-fundamentals/README.md), [filesystem notes](../01-linux-fundamentals/01-filesystem-and-permissions.md) | Create the VM; snapshot; tour `/etc`, `/var`, `/proc`, `/sys`, `/usr`; `findmnt`, `lsblk` | `notes/day-01-vm.md` with `uname -a`, `lsblk`, `findmnt`, `df -hT` |
| 2 | Permissions, umask, ACLs, special bits, sudo | Same note, permissions sections | **L1.1** and **L1.2**: shared directory, ACL, SUID demo, `namei -l` | Permission matrix + one "denied despite rwx" case |
| 3 | Users, groups, sudo, SSH keys | [packages/SSH notes](../01-linux-fundamentals/06-packages-and-ssh.md) | Create a user, group, sudoers drop-in, SSH key login; disable password auth in the VM | `notes/day-03-access.md` |
| 4 | Processes, states, signals, jobs | [process notes](../01-linux-fundamentals/02-processes-signals-jobs.md) | **L1.3**: fork a child, stop it, kill it, create a zombie, inspect `/proc` | Process-state table + signal cheat sheet |
| 5 | CPU, load average, scheduling, `nice` | Process notes | `yes > /dev/null`, `stress-ng`, `pidstat`, `top -H`, `nice`/`renice` | One screenshot/transcript of load vs CPU count |
| 6 | Memory, page cache, swap, OOM | [memory notes](../01-linux-fundamentals/03-memory-swap-oom.md) | **L1.4**: allocate memory, force swap, trigger a cgroup OOM (not host OOM) | `free`, `vmstat`, `smem`/`ps` comparison + OOM log |
| 7 | Review + quiz | — | 20 mixed Linux questions; redo the weakest lab | Week-1 note with 5 "what I got wrong" items |

**Week 1 checkpoint:** explain the difference between VSZ, RSS, PSS, page cache, and swap. If you cannot, redo L1.4 before moving on.

---

## Week 2 — Linux operations: disks, systemd, logs, packages, shell

**Goal:** operate a host, not just inspect it. Learn the failure modes that cause real incidents.

| Day | Focus | Read | Lab / drill | Deliverable |
|---|---|---|---|---|
| 8 | Disks, partitions, filesystems, inodes, mounts | [disk notes](../01-linux-fundamentals/04-disks-filesystems-inodes.md) | **L1.5**: add a disk, partition, `mkfs`, mount, `fstab`; fill inodes; create a deleted-open file | Disk decision tree |
| 9 | LVM, `df` vs `du`, read-only fs, `iostat` | Same | Extend an LV; simulate ENOSPC; use `lsof +L1`; inspect `dmesg` for I/O errors | Incident note: "disk full but du says 2 GB" |
| 10 | systemd units, targets, boot | [systemd notes](../01-linux-fundamentals/05-systemd-boot-logs.md) | **L1.6**: write a service + timer; break it; use `systemctl status`, `journalctl -u`, `systemd-analyze` | Unit file + failure timeline |
| 11 | journald, `/var/log`, logrotate | Same | **L1.7**: configure rotation; force rotation; find a boot's kernel errors; use `journalctl --since` | Log-triage checklist |
| 12 | Packages: apt/dnf/yum, repos, pins, history | [packages/SSH notes](../01-linux-fundamentals/06-packages-and-ssh.md) | Install, pin, downgrade, remove; inspect repo/GPG; use `rpm -qf`/`dpkg -S` | Package troubleshooting note |
| 13 | Shell toolkit: grep/sed/awk/find/jq + Bash safety | [shell toolkit](../01-linux-fundamentals/07-shell-text-toolkit.md), [Bash](../07-automation/01-bash-for-operations.md) | Parse `ss`, `ps`, and a JSON file; write a 20-line safe script | Script v0 + ShellCheck output |
| 14 | Review + incident 1 | [INC-01](../10-troubleshooting-workbook/02-linux-incidents.md) | Work INC-01 evidence-first; write the timeline before revealing the analysis | 5-minute incident walkthrough |

**Week 2 checkpoint:** you can diagnose a failed unit, a full disk, and a permission error without guessing. You can write a script that stops on error and cleans up.

---

## Week 3 — Networking from first principles

**Goal:** stop saying "the network is down". Trace the path and name the failing layer.

| Day | Focus | Read | Lab / drill | Deliverable |
|---|---|---|---|---|
| 15 | Binary, IPv4, masks, CIDR | [IP/subnetting](../02-networking/01-ip-addressing-and-subnetting.md) | **L2.1**: 20 subnet problems; design a /24 split | Subnetting worksheet (no calculator) |
| 16 | VLSM, overlap, host/network/broadcast, IPv6 basics | Same | 10 VLSM problems; find overlapping subnets in a list | Error log of every mistake |
| 17 | Ethernet, ARP, switching, VLANs | [L2 notes](../02-networking/02-ethernet-arp-switching-vlans.md) | **L2.2**: two VMs on a bridge; VLAN interfaces; `tcpdump -e` ARP; MAC table | Packet-flow diagram for on-link traffic |
| 18 | Routing, gateways, `ip route get`, forwarding | [routing notes](../02-networking/03-routing-and-linux-networking.md) | **L2.2 Part B**: add a second NIC; wrong gateway; static route; `traceroute`; enable forwarding | "Why is this route chosen?" note |
| 19 | TCP/UDP, sockets, `ss`, states, MTU | [TCP notes](../02-networking/04-tcp-udp-sockets.md) | **L2.3**: run a listener; connect; inspect `ss`; fill backlog; test `TIME_WAIT`; break MTU | `ss` state table + one timeout case |
| 20 | DNS resolution, records, caching, `dig`/`getent` | [DNS notes](../02-networking/05-dns-and-dhcp.md) | **L2.4**: local DNS server or `/etc/hosts`; wrong search domain; stale cache; split-horizon concept | DNS troubleshooting ladder |
| 21 | Review + **Mock 1** | [Mock 1](../11-assessments/02-mock-assessment-1-linux-networking.md) | 60 minutes, closed book; review every wrong answer | Score + top 3 weak areas |

**Week 3 checkpoint:** you can answer "the server is running but unreachable" with a layered test plan, not a restart.

---

## Week 4 — Firewalls, TLS, packet paths, incidents, applications

**Goal:** convert knowledge into assessment performance and interview stories.

| Day | Focus | Read | Lab / drill | Deliverable |
|---|---|---|---|---|
| 22 | NAT, conntrack, iptables/nftables, firewalld | [NAT/firewall notes](../02-networking/06-nat-and-firewalls.md) | **L2.5**: DNAT a port; default-deny; allow only 443; observe drop vs reject; conntrack table | Firewall rule set + packet evidence |
| 23 | HTTP, TLS, certificates, load balancing | [HTTP/TLS notes](../02-networking/07-http-tls-load-balancing.md) | **L2.6**: `curl -v`, `openssl s_client`, missing intermediate, wrong SNI, 502/504 simulation | TLS/HTTP failure table |
| 24 | Packet-path drill + `tcpdump` | [packet debugging](../02-networking/08-packet-flows-and-debugging.md) | **L2.7**: capture a TCP handshake, DNS query, ARP, TLS ClientHello, and ICMP; read with `tshark` | Annotated capture |
| 25 | Bash health-check script | [Bash](../07-automation/01-bash-for-operations.md) | **L7.1**: disk, service, HTTP checks; `set -euo pipefail`; trap; exit codes; ShellCheck | Script + test output |
| 26 | Incidents 2–4 | [INC-02…04](../10-troubleshooting-workbook/02-linux-incidents.md) | Evidence-first; write timeline, hypotheses, tests | Three 5-minute stories |
| 27 | Mock 1 retake + review | — | Timed; target ≥ 75%; redo every missed topic | Score delta + remediation list |
| 28 | Resume, GitHub, applications | [interview notes](../12-interviews/README.md) | Rewrite 3 bullets with evidence; add repo; apply to 5 infra/backend internships | 5 applications submitted |
| 29 | Incident 5 + weak-area repair | [INC-05](../10-troubleshooting-workbook/03-network-incidents.md) | One weak topic from Mock 1; teach it out loud in 3 minutes | Recording or written explanation |
| 30 | 30-day assessment + next plan | [checklist](../16-checklists/01-master-progress-checklist.md) | Mock 1 one more time or a 30-question oral quiz; plan days 31–60 | `notes/30-day-review.md` |

---

## Daily template

```text
07:30–08:30   DSA (1 medium + review 1 old)
09:00–11:00   College / classes
11:00–12:30   Infrastructure read (25 min) + lab (65 min)
14:00–17:00   College / project work
20:00–20:45   DSA (1 problem) or core CS
21:00–21:30   Write lab note + tomorrow's one question
```

Adjust to your timetable. The non-negotiable parts are: **one lab block**, **one DSA block**, **one written note**.

---

## Weekly deliverables (the artifact trail)

| Week | Artifact | Objective check |
|---|---|---|
| 1 | VM + Linux inspection note | `uname`, `lsblk`, `findmnt`, `df -hT`, `free -m`, `ps aux` captured and explained |
| 1 | Permission lab | A shared directory with SGID + ACL; a "denied despite rwx" case explained with `namei -l` |
| 2 | Disk incident note | Deleted-open file found with `lsof +L1`; inode exhaustion reproduced |
| 2 | systemd + log lab | Unit, timer, failed start, `journalctl` timeline |
| 2 | Bash script v0 | ShellCheck clean; exits non-zero on failure; cleans up temp files |
| 3 | Subnetting worksheet | 30 problems, ≥ 90% correct, no calculator |
| 3 | Packet-path diagram | On-link and off-link path with commands at each hop |
| 3 | Mock 1 | ≥ 75%, every wrong answer explained |
| 4 | Firewall/TLS lab | DNAT + default-deny + `curl`/`openssl` evidence |
| 4 | Incident stories | 5 stories, each ≤ 5 minutes, with evidence and prevention |
| 4 | Applications | 5 roles applied; resume updated with infra evidence |

---

## The 8-hour/week track

If college is heavy:

- **Week 1–2:** Linux files, permissions, processes, memory (one lab per week).
- **Week 3–4:** disks, systemd, logs, networking basics (one lab per week).
- **Week 5–6:** subnetting, TCP/DNS, firewalls, packet path.
- **Week 7:** Bash + incidents.
- **Week 8:** Mock 1 + applications.

Do not compress the labs. Compress the reading.

---

## What to do when you get stuck

1. **Write the symptom in one sentence.** "The service is active but curl times out."
2. **Draw the path.** Client → DNS → route → firewall → socket → process.
3. **Name one command per hop.** `getent`, `ip route get`, `nft list ruleset`, `ss -tulpn`, `journalctl -u`.
4. **Run the cheapest test first.** `ss` is cheaper than `tcpdump`; `getent` is cheaper than reading DNS config.
5. **Write what the result establishes and what it does not.**
6. **Change one thing.** Verify. If it did not help, revert.
7. **Ask: what would have prevented this?** That becomes the runbook/monitor.

---

## 30-day review questions

1. Which command would you run first for: high load, full disk, unreachable port, failed service, DNS failure?
2. What is the difference between `df -h` and `df -i`? Why can one be full while the other is not?
3. Why is `free` memory low on a healthy Linux host?
4. What does `ss -tulpn` show that `ps` does not?
5. Why does a firewall `DROP` produce a timeout while `REJECT` produces "connection refused"?
6. What is the difference between a router and a switch? Where does ARP stop?
7. What happens, step by step, when you type `curl https://example.com`?
8. How would you prove that a packet is leaving the host? That it is arriving? That it is being dropped?
9. What is the smallest safe change for a failed service: reload, restart, rollback, or reboot? Why?
10. Which of your five incident stories is weakest, and what evidence would make it stronger?

If you can answer 8/10 without notes, you are ready to apply. Continue with [Roadmap B](02-first-90-days.md).
