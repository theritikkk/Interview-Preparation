# Command Field Manual

> Sixteen commands. For each: what it does, why you use it, the options that matter, a representative output, how to read it, one scenario, and the mistakes that get people in trouble.
>
> Outputs are representative. Your kernel, distribution, and load will differ. Learn the **columns and units**, not the exact numbers.

---

## 1. `ps` — process snapshot

**What:** lists processes and selected attributes at one moment.

**Why:** the first question in most incidents is "what is running, as whom, in what state?"

```bash
ps aux
ps -eo pid,ppid,user,stat,ni,psr,wchan:30,etime,cmd --sort=-%cpu | head
ps -p 1042 -o pid,ppid,stat,wchan:25,cmd
ps -eLf                    # threads
ps -eo pid,ppid,stat,cmd --forest
```

| Option | Meaning |
|---|---|
| `a` | All users' processes with a tty |
| `u` | User-oriented format |
| `x` | Include processes without a tty |
| `-e` | Every process |
| `-o` | Custom columns |
| `--sort` | Sort by a column |
| `-L` | Threads (LWPs) |
| `--forest` | Process tree |

```text
USER       PID %CPU %MEM    VSZ   RSS TTY      STAT START   TIME COMMAND
root         1  0.0  0.1 167432 12800 ?        Ss   10:01   0:02 /sbin/init
app       1042  2.3  6.1 2845120 492000 ?     Ssl  10:05   0:14 node /opt/app/server.js
app       1180  0.0  0.0      0     0 ?       Z    10:07   0:00 [worker] <defunct>
```

**Read:** `STAT` first (`S` sleeping, `D` uninterruptible, `Z` zombie), then `TIME` vs `START` (low CPU time over a long life = waiting), then `RSS` and `COMMAND`.

**Scenario:** a service is "up" but unresponsive. `ps -o stat,wchan` shows `D` in `rpc_wait_bit_killable`; the process is blocked on NFS, not CPU. Check the mount/server, not the app.

**Mistakes:** treating `%CPU` from `ps` as current (it is a lifetime average); summing RSS across processes (shared pages double-count); assuming a zombie can be killed (fix the parent).

---

## 2. `top` — live process and system view

**What:** a periodically refreshed view of load, CPU, memory, and processes.

**Why:** quick triage of CPU vs memory vs I/O pressure, and per-thread hotspots.

```bash
top
top -H -p 1042
top -b -n1 | head -20
```

Keys: `P` sort by CPU, `M` by memory, `H` threads, `1` per-CPU, `c` command line, `k` kill, `r` renice.

```text
top - 10:40:01 up 1 day,  2 users,  load average: 12.40, 11.80, 9.20
Tasks: 210 total,   2 running, 206 sleeping,   2 stopped,   0 zombie
%Cpu(s):  3.1 us,  1.2 sy,  0.0 ni,  8.4 id, 87.1 wa,  0.0 hi,  0.2 si,  0.0 st
MiB Mem :   7936.0 total,    210.4 free,   2048.2 used,   5677.4 buff/cache
MiB Swap:   2048.0 total,      0.0 used,   2048.0 free.   5210.1 avail Mem

    PID USER      PR  NI    VIRT    RES    SHR S  %CPU  %MEM     TIME+ COMMAND
   2041 app       20   0  812340  92100  12000 D   0.3   1.1   0:02.11 rsync
```

**Read:** `load average` against CPU count; `wa` (I/O wait); `us/sy/id/st`; `RES` vs `VIRT`; process state.

**Scenario:** load 12 on 4 CPUs, 87% `wa`, processes in `D`. The bottleneck is storage/NFS. `top` alone does not tell you which device; go to `iostat`/`dmesg`.

**Mistakes:** reading `load` as CPU-only; ignoring `wa` and `st` (steal, in VMs); assuming 100% CPU is bad (it may be a busy batch job).

---

## 3. `free` — memory overview

**What:** total/used/free memory, buffers/cache, swap.

**Why:** the fastest way to distinguish "memory pressure" from "the kernel is caching files".

```bash
free -h
free -m -s 2
```

```text
               total        used        free      shared  buff/cache   available
Mem:           7.7Gi       2.1Gi       1.2Gi        28Mi       4.4Gi       5.2Gi
Swap:          2.0Gi          0B       2.0Gi
```

**Read:** `available` is the number that matters. `buff/cache` is reclaimable. `shared` includes `tmpfs`. Swap `used` alone is not a problem; `si/so` rates are.

**Scenario:** "free is 1.2 GiB, add RAM." `available` is 5.2 GiB and `vmstat` shows no swap I/O. No action.

**Mistakes:** panicking at low `free`; dropping caches; adding swap to hide a leak.

---

## 4. `vmstat` — virtual memory and scheduler stats

**What:** periodic summary of processes, memory, swap, I/O, CPU.

**Why:** one view that separates runnable CPU pressure (`r`), blocked I/O (`b`), swap activity (`si/so`), and CPU breakdown.

```bash
vmstat 1 5
vmstat -s
```

```text
procs -----------memory---------- ---swap-- -----io---- -system-- ------cpu-----
 r  b   swpd   free   buff  cache   si   so    bi    bo   in   cs us sy id wa st
 1  0      0 812340  20112 920144    0    0     4    18  220  410  3  1 95  1  0
 0  2      0 802110  20112 918220    0    0     0  2048  180  390  1  1 20 78  0
```

**Read:** `r` > CPU count = run queue; `b` > 0 = blocked (often I/O); `si/so` = swap in/out KB/s; `wa` = I/O wait; `st` = stolen by hypervisor.

**Scenario:** `b=11`, `wa=89`, `r=1`. The system is not CPU-bound; it is waiting on I/O. Check `iostat`, `dmesg`, NFS.

**Mistakes:** reading one sample; ignoring `st` on a VM; treating `free` as "available".

---

## 5. `iostat` — block device performance

**What:** per-device I/O rates, latency, queue depth, utilization.

**Why:** proves whether a disk/volume is saturated and whether the problem is IOPS, throughput, or latency.

```bash
iostat -xz 1
iostat -x 1 5
```

```text
Device            r/s     w/s     rkB/s   wkB/s  await  aqu-sz  %util
nvme0n1         120.0    80.0   4800.0  3200.0   0.42    0.08   2.1
sdb               0.0   420.0      0.0  33600.0  18.40    7.60  99.8
```

**Read:** `await` = average latency (queue + service); `aqu-sz` = average queue depth; `%util` = busy time. High `await` + queue + `%util` = saturation. NVMe can have high `%util` with low latency because of parallelism.

**Scenario:** database writes slow. `sdb` at 100% util, `await` 42 ms, queue 33. The volume is saturated. Check volume IOPS limit, noisy neighbour, RAID rebuild, fsync rate.

**Mistakes:** assuming `%util=100` means "disk full"; ignoring the queue; benchmarking production with `fio`.

---

## 6. `df` — filesystem space and inodes

**What:** used/available blocks and inodes per mounted filesystem.

**Why:** the first command in any "disk full" incident, and the one that distinguishes block exhaustion from inode exhaustion.

```bash
df -hT
df -i
df -h /var
```

```text
Filesystem                        Type  Size  Used Avail Use% Mounted on
/dev/mapper/ubuntu--vg-ubuntu--lv ext4   37G   36G  0.5G  99% /
/dev/sdb1                         ext4  100G   20G   80G  20% /data
```

**Read:** `Use%` on blocks; `IUse%` on inodes; `Avail` may be less than `Size-Used` due to reserved blocks; `-T` shows filesystem type.

**Scenario:** `df -h` 100%, `du` says 20 GB. Check `df -i` (inodes), `lsof +L1` (deleted-open), `findmnt` (other mounts), reserved blocks.

**Mistakes:** checking only `-h` and not `-i`; running `du` across mounts; assuming `df` sees all namespaces/containers.

---

## 7. `du` — directory usage

**What:** disk space consumed by files under a path.

**Why:** finds the directory/file causing the usage.

```bash
du -xhd1 /var | sort -h
du -xsh /var/log
du --inodes -xhd1 /var | sort -h
```

| Option | Meaning |
|---|---|
| `-s` | Summary only |
| `-h` | Human units |
| `-x` | Stay on one filesystem |
| `-d1` | Depth 1 |
| `--inodes` | Count inodes (GNU) |

**Read:** `du` walks names; it cannot see deleted-open files. `-x` prevents crossing mounts. `sort -h` understands human units.

**Scenario:** `/var` is full. `du -xhd1 /var` shows `/var/log` is small, `/var/lib` is large. Drill down; if nothing matches `df`, check `lsof +L1`.

**Mistakes:** forgetting `-x`; running as a user without read permission (undercounts); deleting the wrong path based on a partial result.

---

## 8. `lsblk` — block device topology

**What:** tree of disks, partitions, LVM, RAID, filesystems, mount points.

**Why:** the safest way to see what a device is before touching it.

```bash
lsblk
lsblk -o NAME,SIZE,TYPE,FSTYPE,MOUNTPOINTS
lsblk -f
```

```text
NAME                      SIZE TYPE FSTYPE      MOUNTPOINTS
nvme0n1                   40G disk
├─nvme0n1p1                1G part vfat        /boot/efi
├─nvme0n1p2                2G part ext4        /boot
└─nvme0n1p3               37G part LVM2_member
  └─ubuntu--vg-ubuntu--lv 37G lvm  ext4        /
sdb                      100G disk
└─sdb1                   100G part ext4        /data
```

**Read:** disk → partition → LVM → filesystem → mount. `TYPE` tells you whether it is a disk, part, lvm, raid, crypt.

**Scenario:** before `mkfs /dev/sdb`, confirm `/dev/sdb` is the new empty disk and not the root device. `lsblk` + `blkid` + `findmnt`.

**Mistakes:** using `fdisk -l` output alone; confusing `/dev/sda` with a partition; running `mkfs` on a mounted device.

---

## 9. `mount` and `findmnt` — what is mounted where

**What:** `mount` lists or changes mounts; `findmnt` queries the mount table in a tree/structured form.

**Why:** a path can be on a different filesystem, read-only, `noexec`, or a network mount. This changes the diagnosis.

```bash
findmnt
findmnt -T /var/log
mount | grep ' /data '
sudo mount /dev/sdb1 /mnt
sudo umount /mnt
sudo mount -o remount,ro /mnt
```

**Read:** target, source, fstype, options (`rw/ro`, `noexec`, `nosuid`, `relatime`, NFS `hard/soft`).

**Scenario:** a script fails with "Permission denied" despite `755`. `findmnt -T /opt/app` shows `noexec`; the binary cannot execute from that mount.

**Mistakes:** `umount` while a process uses the mount (`lsof +f -- /mnt`); remounting `rw` after I/O errors; bad fstab without `mount -a` test.

---

## 10. `ss` — sockets and listening ports

**What:** socket statistics: listening, established, states, process owner.

**Why:** answers "is anything listening, on which address, and who owns it?" — before firewalls and packet captures.

```bash
ss -tulpn
ss -tan
ss -tn state established
ss -tulpn | grep :8080
ss -s
```

| Option | Meaning |
|---|---|
| `-t` | TCP |
| `-u` | UDP |
| `-l` | Listening |
| `-a` | All |
| `-n` | Numeric ports/addresses |
| `-p` | Process (needs root for other users) |
| `-s` | Summary |

```text
Netid State  Local Address:Port  Peer Address:Port Process
tcp   LISTEN 0.0.0.0:22          0.0.0.0:*         users:(("sshd",pid=812,fd=3))
tcp   LISTEN 127.0.0.1:8080      0.0.0.0:*         users:(("node",pid=1042,fd=7))
tcp   ESTAB 10.0.0.10:22         10.0.0.5:51234    users:(("sshd",pid=900,fd=4))
```

**Read:** `Local Address` — `0.0.0.0` is all IPv4 interfaces, `127.0.0.1` is loopback only, `[::]` is all IPv6. `State` — `LISTEN`, `ESTAB`, `TIME-WAIT`, `CLOSE-WAIT`. `Peer Address` for connections.

**Scenario:** remote clients cannot connect, but the process is healthy. `ss` shows `127.0.0.1:8080`. The app binds loopback; fix the bind address, not the firewall.

**Mistakes:** using `netstat` (deprecated); forgetting `-p` needs privilege; assuming `LISTEN` means the app can serve (it may be a stuck accept loop).

---

## 11. `lsof` — open files and sockets

**What:** lists open files, including sockets, deleted files, and the process holding them.

**Why:** "disk full but no file" and "who is using this port/mount" are `lsof` questions.

```bash
lsof -p 1042
lsof -i :8080
lsof +L1                 # link count 0 (deleted but open)
lsof /var/log/app.log
lsof +f -- /mnt/data
lsof -u app
```

**Read:** `COMMAND PID USER FD TYPE DEVICE SIZE/OFF NLINK NODE NAME`. `FD` `3w` = fd 3, write; `DEL`/`(deleted)` = unlinked.

**Scenario:** `df` 100%, `du` small. `lsof +L1` shows a 38 GB deleted log held by PID 912. Truncate `/proc/912/fd/3` or restart the writer.

**Mistakes:** running without root (misses other users); deleting the `/proc/PID/fd/N` symlink; ignoring `NLINK 0`.

---

## 12. `dmesg` — kernel ring buffer

**What:** kernel messages: boot, drivers, disks, network, OOM, segfaults, firewall drops.

**Why:** the kernel often records the cause before userspace does.

```bash
dmesg -T | tail -50
dmesg -T | grep -i -E 'error|fail|oom|I/O|nvme|eth'
dmesg -w
journalctl -k -b
```

**Read:** timestamps, device names, error codes. `I/O error` + `remounting read-only` is a storage incident. `Out of memory: Killed process` is an OOM kill. `TCP: request_sock_TCP: Possible SYN flooding` can indicate a backlog/attack.

**Scenario:** a filesystem suddenly read-only. `dmesg -T` shows an I/O error on the device before the remount. The filesystem is a victim; investigate the device/SAN.

**Mistakes:** `dmesg` without `-T` (relative timestamps); clearing the ring buffer (`dmesg -c`) before capturing evidence; ignoring rate-limited messages.

---

## 13. `journalctl` — systemd journal

**What:** query the structured journal from journald.

**Why:** service lifecycle, kernel messages, auth, and boot history in one index.

```bash
journalctl -u app -f
journalctl -u app --since "10 min ago" -o short-iso
journalctl -p err -b
journalctl -k
journalctl -b -1
journalctl _PID=1042
journalctl --disk-usage
```

**Read:** timestamp, host, unit/PID, message. `-b` selects a boot; `-p` priority; `-o` format; `--since/--until` window.

**Scenario:** a service failed 20 minutes ago and restarted. `journalctl -u app --since "30 min ago"` shows the crash stack before the restart. Capture it before vacuuming.

**Mistakes:** volatile journal lost on reboot; rate limiting hides bursts; wrong unit name; not using `-b -1` for the previous boot.

---

## 14. `systemctl` — service and unit control

**What:** control systemd units and inspect their state.

**Why:** the standard way to start, stop, enable, and debug services on modern Linux.

```bash
systemctl status app
systemctl start|stop|restart|reload app
systemctl enable --now app
systemctl cat app
systemctl show app -p MainPID -p MemoryCurrent -p Restart
systemctl list-units --failed
systemctl reset-failed app
```

**Read:** `Loaded` (unit path, enabled), `Active` (state, timestamp), `Main PID`, `CGroup`, recent log lines. `failed` means the start limit or exit code; `activating (auto-restart)` means a restart loop.

**Scenario:** service restarts every 30 seconds. `status` shows `code=exited, status=1`; `journalctl -u app` shows `EADDRINUSE`. Find the port owner, fix the conflict.

**Mistakes:** editing vendor units; forgetting `daemon-reload`; `restart` when `reload` is enough; `enable` without `--now` (or vice versa).

---

## 15. `ip` — addresses, links, routes, neighbors

**What:** modern Linux network configuration and inspection.

**Why:** the replacement for `ifconfig`, `route`, and `arp`; the first command in any network incident.

```bash
ip addr show
ip -br addr
ip link show
ip route show
ip route get 8.8.8.8
ip neigh show
ip -s link show eth0
ip link set eth0 up
ip addr add 10.0.0.10/24 dev eth0
```

**Read:** `state UP/DOWN`, `mtu`, `qdisc`, addresses with prefix, `default via`, `dev`, `src`, neighbor `REACHABLE/STALE/FAILED`.

**Scenario:** a host can reach its subnet but not the internet. `ip route` has no default route, or the default points to the wrong gateway. `ip route get 8.8.8.8` shows the chosen path.

**Mistakes:** using `ifconfig` (deprecated); forgetting the prefix length; adding an address without a route; changing the default route remotely and locking yourself out.

---

## 16. `ethtool` — NIC and link diagnostics

**What:** query/modify Ethernet device settings: speed, duplex, link, driver, rings, offloads, statistics.

**Why:** distinguishes a physical/link problem from an IP/routing problem.

```bash
ethtool eth0
ethtool -S eth0
ethtool -i eth0
ethtool -k eth0
ethtool -g eth0
ethtool --show-eee eth0
```

```text
Settings for eth0:
    Supported ports: [ TP ]
    Speed: 10000Mb/s
    Duplex: Full
    Auto-negotiation: on
    Link detected: yes
```

**Read:** `Link detected`, `Speed`, `Duplex`; `-S` for drops/errors (`rx_dropped`, `tx_errors`, `rx_missed_errors`); `-i` for driver/firmware; `-k` for offloads.

**Scenario:** throughput is low and `ip -s link` shows RX drops. `ethtool -S eth0` shows `rx_missed_errors` rising; the NIC/ring/CPU cannot keep up. Check ring sizes, IRQ affinity, and whether the VM's virtual NIC is the bottleneck.

**Mistakes:** forcing speed/duplex on one side only (duplex mismatch); changing offloads to "fix" a bug without evidence; ignoring `Link detected: no` and looking at IP config.

---

## Quick selection guide

| Question | First command |
|---|---|
| What is running? | `ps aux` |
| What is hot right now? | `top` |
| Is memory pressure real? | `free -h`, `vmstat 1` |
| Is the disk saturated? | `iostat -xz 1` |
| Is the filesystem full? | `df -hT`, `df -i` |
| What is using the space? | `du -xhd1 /path` |
| What is this block device? | `lsblk -f` |
| Is it mounted read-only/noexec? | `findmnt -T /path` |
| Is anything listening? | `ss -tulpn` |
| Who holds this file/port? | `lsof -p`, `lsof -i :port` |
| What did the kernel say? | `dmesg -T` |
| Why did the unit fail? | `systemctl status`, `journalctl -u` |
| Where does this packet go? | `ip route get` |
| Is the link healthy? | `ethtool eth0` |

---

## Checkpoint

1. Which command tells you whether a filesystem is out of blocks or inodes?
2. How do you prove a process is blocked on I/O rather than CPU?
3. What is the difference between `free` and `available`?
4. What does `%util` in `iostat` not tell you?
5. How do you find a deleted-but-open file?
6. What does `ss -tulpn` show, and why does `127.0.0.1` matter?
7. What is the difference between `ps` and `top` CPU percentages?
8. How do you find the process holding a mount busy?
9. What does `ip route get 8.8.8.8` prove?
10. What does `ethtool -S` add to `ip -s link`?
