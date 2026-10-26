# Phase 3 Checkpoint — Virtualization and Storage

> **Rules:** closed book, 90 minutes for Parts A–D. A calculator is allowed for Part B. No searching, no notes, no `virsh help`. Then do the practical sign-off on the VM. Target: **80% overall**, and at least **30/40** on the diagnosis scenarios.
>
> **Environment assumption:** x86 Linux with VT-x/AMD-V for the KVM questions. On Apple Silicon, answer the KVM questions from the notes and run the storage labs in an x86 cloud VM or a Linux host. Ceph questions are answerable conceptually if you skipped L3.4.

---

## Part A — Rapid facts (25 × 2 = 50 points)

Answer in one or two sentences. Define every term you use.

1. What does KVM do, what does QEMU do, and what does libvirt do?
2. What is the difference between a type-1 and a type-2 hypervisor? Give one example of each.
3. What do VT-x/AMD-V enable, and what do EPT/NPT add?
4. A VM has 8 vCPUs. What are those vCPUs on the host, and what is a pCPU?
5. Why does NUMA placement matter for a VM, and what symptom appears when it is wrong?
6. What is memory ballooning, and what does the guest need for it to work?
7. What are hugepages, and why do databases and AI workloads use them?
8. When would you choose `raw` over `qcow2` for a VM disk?
9. What is a qcow2 backing chain? Which file must never be deleted while an overlay depends on it?
10. What does `cache=none` mean for a VM disk, and when is it a bad choice?
11. What is the difference between virtio-blk, virtio-scsi, and NVMe passthrough?
12. Draw the packet path from a guest NIC to the physical NIC in a bridged setup. Name the tap device and the bridge.
13. What is SR-IOV, and what does it cost you in operational flexibility?
14. Define PV, VG, LV, and physical extent. What is the default extent size in LVM2?
15. What is thin provisioning, and why is `Data%` more important than the advertised size?
16. ext4 vs XFS: which can shrink, which must be mounted to grow, and which is the safer default for a root filesystem?
17. A filesystem has free blocks but `touch` fails with `No space left on device`. What is the cause and the command?
18. A process deleted a 40 GB log file, but `df` still shows the space used. Why, and how do you confirm it?
19. What is the difference between NFS `root_squash` and `no_root_squash`, and why does the latter matter?
20. iSCSI: define target, initiator, LUN, and portal. Which TCP port is used?
21. Ceph: what are MON, OSD, MDS, and RGW responsible for?
22. What is a placement group, and what does CRUSH do with it?
23. Name the three common NVMe-oF transports and one reason to use NVMe-oF over iSCSI.
24. Define IOPS, throughput, and latency. State Little's law.
25. What is the RAID write penalty for RAID 1/10, RAID 5, and RAID 6?

---

## Part B — Calculations (6 × 5 = 30 points)

Show your work. Units must be correct.

### B1 — LVM extents

A volume group has one PV of `200 GiB`. The extent size is the LVM2 default.

1. How many physical extents does the PV provide (ignoring metadata)?
2. You create a `32 GiB` LV. How many extents does it consume, and how many remain?
3. You add a `100 GiB` PV. What is the VG's new free space in GiB and extents?
4. You then run `lvextend -l +100%FREE`. What size is the LV, and what must you run next for ext4 and for XFS?

### B2 — Thin pool

A thin pool is `4 TiB`. It backs 24 thin volumes of `500 GiB` each.

1. What is the advertised virtual capacity?
2. What is the overcommit ratio?
3. The pool reaches `Data% = 100`. What happens to writes, and what are the first three recovery actions?
4. Name two metrics you alert on, with thresholds, and explain why `Metadata%` can kill the pool before `Data%` does.

### B3 — IOPS and RAID

A workload is 70% random 8 KiB reads and 30% random 8 KiB writes, at a required 6,000 front-end IOPS.

1. What backend IOPS does RAID10 need? Use a write penalty of 2.
2. What backend IOPS does RAID6 need? Use a write penalty of 6.
3. What throughput does 6,000 × 8 KiB represent in MB/s?
4. A 15K SAS HDD does about 180 random IOPS. How many drives are needed for the RAID10 backend? What does that tell you about 8 KiB random I/O on HDDs?

### B4 — Little's law

1. A device serves 4,000 IOPS at an average latency of 0.8 ms. What is the average queue depth?
2. A device has a maximum queue depth of 32 and a latency of 8 ms. What is its maximum IOPS?
3. You need 10,000 IOPS from that device. What must change, and why can you not fix it by raising the queue depth to 80?

### B5 — qcow2 and snapshots

A VM disk is a `500 GiB` qcow2 file. `du` reports `120 GiB`; `qemu-img info` reports `virtual size: 500 GiB`.

1. Explain the difference between virtual size, disk usage, and allocated clusters.
2. You take an external snapshot and the guest writes `30 GiB`. What are the base and overlay sizes now, and what does the guest still see?
3. A colleague deletes the base image "to free space". What happens, and which command would have shown the dependency?
4. You want to merge the overlay back into the base without downtime. Which operation do you use, and what is the main risk?

### B6 — NFS throughput

An NFS server has one `10 GbE` link. A client reads a `10 GB` file.

1. What is the theoretical and realistic ceiling in MB/s for a single TCP flow?
2. How long does the transfer take at `1.1 GB/s`?
3. The server's backing array can only do 5,000 random 8 KiB IOPS. What throughput is that, and which is the bottleneck for this sequential read?
4. Eight clients all read at `100 MB/s`. Does the single 10 GbE link suffice? What would you check before adding a second link?

---

## Part C — Diagnosis scenarios (8 × 5 = 40 points)

For each scenario, state:

- the **fault domain** (guest, hypervisor, storage, network, filesystem, configuration),
- the **next two checks** with exact commands,
- the **fix**, and
- one **verification** that proves it is fixed.

Do not jump to a fix before naming the evidence you would collect.

### C1

```text
[  OK  ] Reached target Local File Systems.
[FAILED] Failed to mount /data.
See 'systemctl status data.mount' for details.
[DEPEND] Dependency failed for /data.
You are in emergency mode.
```

### C2

```text
$ df -h /var
Filesystem      Size  Used Avail Use% Mounted on
/dev/vg0/var     20G   20G     0 100% /var

$ du -sh /var/* 2>/dev/null | sort -h | tail -3
1.1G    /var/log
800M    /var/lib
120M    /var/cache
```

### C3

```text
$ sudo lvextend -L +10G /dev/vg_data/data
  Size of logical volume vg_data/data changed from 20.00 GiB to 30.00 GiB.
  Logical volume vg_data/data successfully resized.

$ df -h /data
Filesystem               Size  Used Avail Use% Mounted on
/dev/mapper/vg_data-data  20G   18G  2.0G  90% /data
```

### C4

```text
$ sudo lvs -a -o name,data_percent,metadata_percent,lv_size vg_data
  LV        Data%  Meta%  LSize
  thinpool  100.00 71.00  4.00t
  vm01      100.00        500.00g
  vm02      100.00        500.00g
```

### C5

```text
$ sudo mount -t nfs 10.0.0.10:/exports/data /mnt/data
<hangs for 60 seconds, then no prompt>

$ showmount -e 10.0.0.10
clnt_create: RPC: Port mapper failure - Timed out
```

### C6

```text
$ sudo iscsiadm -m discovery -t sendtargets -p 10.0.0.10
iscsiadm: No portals found
```

### C7

Inside a KVM guest:

```text
$ iostat -x 1
Device   r/s     w/s    rkB/s    wkB/s  await  %util
vda    1200.0   800.0  9600.0   6400.0  45.20  100.00
```

The host's NVMe shows `%util` around 35% and `await` around 0.4 ms.

### C8

```text
$ virsh start web01
error: Failed to start domain 'web01'
error: Cannot access storage file '/var/lib/libvirt/images/web01.qcow2'
(as uid:64055, gid:64055): No such file or directory
```

---

## Part D — Design and explain (2 × 20 = 40 points)

### D1 — Design a small virtualization platform (20 points)

Design storage and compute for 24 VMs:

| Property | Value |
|---|---|
| vCPUs per VM | 8 |
| RAM per VM | 32 GiB |
| Disk per VM | 500 GiB |
| Workloads | 6 databases (write-heavy, latency-sensitive), 18 build/test VMs (bursty, disposable) |
| Hosts | 3 |
| Failure requirement | One host may fail; no data loss; VMs restart on remaining hosts |
| Snapshot requirement | Daily snapshot, 7-day retention |
| Backup requirement | Databases must be restorable to a point in time |

Answer:

1. CPU and memory per host, including reserve. State your overcommit policy and why.
2. Storage technology and capacity. Show raw, usable, and thin-committed numbers.
3. Which workloads get block, file, or object storage, and why.
4. Snapshot vs backup. What does a qcow2 snapshot protect against, and what does it not?
5. The top three failure modes of your design and the monitoring signal for each.

### D2 — The write path (20 points)

Trace one `write()` from an application inside a KVM guest to the physical NVMe. Name every layer and the transition between guest and host.

Then answer:

1. Which command at each layer would you use to prove where a latency spike is?
2. The guest shows high `await`; the host NVMe shows low `await` but the QEMU process shows high CPU. What are the likely causes?
3. The guest uses `cache=writeback`. What is the data-loss window on host power loss, and when is that acceptable?
4. The VM has a 12-deep snapshot chain. Why does that hurt write performance, and what is the correct remediation?
5. The workload is an AI training job writing checkpoints. What would you change about the storage path?

---

## Answer key

### Part A

1. **KVM** is the in-kernel virtualization module that exposes VT-x/AMD-V and runs guest code in VMX non-root mode. **QEMU** is the userspace device model and machine emulator that creates the VM, emulates devices, and drives KVM. **libvirt** is the management API/daemon (`libvirtd`) that stores VM XML, manages lifecycle, networks, and storage pools; `virsh` is its CLI.
2. Type 1 runs directly on hardware (ESXi, Xen, Hyper-V); type 2 runs on a host OS (VirtualBox, VMware Workstation, QEMU without KVM acceleration). KVM is a kernel module, so it behaves like a type-1 hypervisor while QEMU remains a userspace component.
3. VT-x/AMD-V provide hardware-assisted CPU virtualization: privileged instructions and VM exits. EPT/NPT provide two-level address translation (guest virtual → guest physical → host physical) in hardware, avoiding shadow page tables and reducing exits.
4. Each vCPU is a host userspace thread (a QEMU vCPU thread) scheduled by the host kernel; a pCPU is a physical core/hardware thread. Eight vCPUs are eight schedulable threads, not eight dedicated cores unless you pin them.
5. A VM's vCPUs and memory should be placed on the same NUMA node. Cross-node placement adds memory latency and reduces bandwidth; symptoms are uneven performance, high remote-memory traffic, and a VM that is slower than its CPU utilization suggests. Check with `numastat`, `virsh numatune`, and `lscpu`.
6. The balloon driver in the guest returns unused pages to the host under memory pressure. The guest needs the `virtio_balloon` driver; the host uses `virsh setmem`/`dommemstat`. It is cooperative, not a hard limit.
7. Hugepages are pre-allocated page sizes (2 MiB or 1 GiB) that reduce TLB misses and page-table overhead. Databases and AI workloads use them for large contiguous memory regions and predictable latency; they must be reserved at boot (`vm.nr_hugepages`) and are not swappable.
8. Choose `raw` for maximum performance and simplicity, especially with `cache=none` and preallocated block devices; it supports direct I/O and is easy to back with LVM. Choose `qcow2` for snapshots, thin allocation, compression, and backing chains. Raw files can be sparse too, but qcow2 has more features.
9. A backing chain is an overlay qcow2 that reads unchanged clusters from its backing file and stores changed clusters itself. The base/backing file must never be deleted or modified while any overlay depends on it; `qemu-img info --backing-chain` shows the chain.
10. `cache=none` bypasses the host page cache using `O_DIRECT`; it is the usual choice for production and for live migration. It is a bad choice when the backing store does not support direct I/O correctly (some network filesystems), or when the guest does many small writes and you deliberately want host caching (at the cost of coherence and migration safety).
11. **virtio-blk** is a paravirtual block device with a simple queue, good for most disks. **virtio-scsi** exposes a SCSI-like path with more queues, device naming, and passthrough features; better for many disks and some SAN semantics. **NVMe passthrough** gives the guest a real or virtual NVMe controller with deep queues and low overhead, but complicates migration and snapshots.
12. Guest NIC → virtio-net driver → virtqueue → KVM/vhost → tap device (`vnetN`) → Linux bridge (`br0`) → physical NIC. With `vhost-net`, the host kernel thread handles the data path and QEMU is mostly out of the loop.
13. SR-IOV exposes virtual functions (VFs) from a physical NIC directly to VMs, giving near-native throughput and latency. It costs live migration (the VF is tied to the host), requires switch/VLAN configuration, limits VF counts, and removes some host visibility/control.
14. A **PV** is a disk or partition initialized for LVM. A **VG** is a pool of extents from one or more PVs. An **LV** is a logical block device carved from a VG. A **physical extent** is the allocation unit, default **4 MiB**.
15. Thin provisioning advertises more virtual capacity than the pool physically holds; blocks are allocated on first write. `Data%` shows actual pool consumption, which is what determines whether writes will fail; the advertised sizes are promises, not capacity.
16. **ext4** can grow online and can shrink offline (rarely worth it). **XFS** can grow online but cannot shrink. XFS is a good default for large filesystems and high concurrency; ext4 is a safe, simple default for root and small volumes. Neither replaces backups.
17. Inode exhaustion. Check `df -i`; find the directory with many small files (`find / -xdev -type f | wc -l`, `du --inodes` on newer coreutils). Fix by deleting files, archiving, or recreating the filesystem with more inodes (requires a new mkfs; ext4 inode count is fixed at creation).
18. The file is unlinked but still open; the inode and blocks remain until the last file descriptor closes. Confirm with `lsof +L1` or `lsof | grep deleted`. Recover space by restarting the writer or truncating the fd via `/proc/<pid>/fd/<fd>` (risky; prefer restart).
19. `root_squash` maps client UID 0 to `nobody`, preventing a client root from writing as root on the server. `no_root_squash` preserves root, so a compromised client can chown/delete files on the export; use it only for controlled use cases such as some backup or container workflows.
20. The **target** exports storage; the **initiator** is the client that logs in; a **LUN** is the exported block device; a **portal** is the target's IP:port. iSCSI uses TCP **3260**.
21. **MON** maintains the cluster map and quorum. **OSD** stores objects and serves data. **MDS** serves CephFS metadata. **RGW** provides the S3/Swift-compatible object gateway.
22. A placement group is a logical grouping of objects that CRUSH maps to a set of OSDs; it is the unit of replication and recovery. CRUSH is the deterministic hash-based placement algorithm that maps PGs to OSDs using the cluster map, failure domains, and weights.
23. NVMe-oF transports: **TCP**, **RDMA** (RoCE/InfiniBand), and **FC** (Fibre Channel). NVMe-oF gives lower latency, deeper queues, and NVMe semantics than iSCSI, especially for all-flash and AI/HPC workloads.
24. **IOPS** = I/O operations per second; **throughput** = bytes per second; **latency** = time per I/O. Little's law: `queue depth = IOPS × latency` (with consistent units), so `IOPS = QD / latency`.
25. RAID 1/10 = 2 (each write goes to two mirrors); RAID 5 = 4 (read-modify-write: read old data + old parity, write new data + new parity); RAID 6 = 6 (two parity blocks). Log-structured or full-stripe writes can avoid some of this, but the exam convention is the penalty above.

### Part B

**B1**

1. `200 GiB = 204,800 MiB`; `204,800 / 4 = 51,200 extents`.
2. `32 GiB = 32,768 MiB`; `32,768 / 4 = 8,192 extents`. Remaining: `51,200 − 8,192 = 43,008 extents = 168 GiB`.
3. New total `300 GiB = 307,200 MiB`; `307,200 / 4 = 76,800 extents`. Free: `76,800 − 8,192 = 68,608 extents = 268 GiB`.
4. `lvextend -l +100%FREE` consumes all free extents, producing a `300 GiB` LV (32 GiB + 268 GiB). Then `resize2fs /dev/vg_data/data` for ext4, or `xfs_growfs /data` for XFS (mounted).

**B2**

1. `24 × 500 GiB = 12,000 GiB = 11.72 TiB` advertised.
2. `11.72 / 4 = 2.93:1` overcommit.
3. Writes fail with `No space left on device` and may put the pool into a read-only/error state; guest filesystems can be corrupted. First actions: stop new writes and free space (`fstrim`, delete old snapshots, remove unneeded volumes), extend the pool with a new PV (`vgextend` + `lvextend` the pool), and verify `lvs`/`dmsetup status`. Do not run `fsck` on a mounted guest filesystem; recover the pool first.
4. Alert on `Data% > 75%` and `Metadata% > 70%`, with critical at 85%/80%. Metadata chunks track block allocation; if metadata is exhausted, the pool cannot allocate even when data space exists, and recovery may require extending metadata or mounting with recovery options. Monitor both.

**B3**

1. Backend = `0.7 × 6,000 + 0.3 × 6,000 × 2 = 4,200 + 3,600 = 7,800 IOPS`.
2. Backend = `4,200 + 1,800 × 6 = 4,200 + 10,800 = 15,000 IOPS`.
3. `6,000 × 8 KiB = 48,000 KiB/s = 46.9 MiB/s` (about 48 MB/s decimal).
4. `7,800 / 180 = 43.3`, so **44 drives** for the backend, before capacity and rebuild constraints. This shows that 8 KiB random I/O on HDDs is impractical; use SSDs/NVMe or change the I/O pattern (larger, sequential, cached).

**B4**

1. `QD = 4,000 × 0.0008 s = 3.2`.
2. `IOPS = 32 / 0.008 = 4,000 IOPS`.
3. To get 10,000 IOPS at 8 ms you would need `QD = 80`, beyond the device's 32. You must reduce latency (faster device, cache, more parallelism across devices) or add devices/queues; raising the queue depth beyond the hardware limit is not possible.

**B5**

1. Virtual size is what the guest sees (`500 GiB`). Disk usage is the host file's allocated bytes (`120 GiB`), because qcow2 is sparse. Allocated clusters are the 64 KiB (default) units actually written; metadata and internal snapshots also consume space.
2. The base remains `120 GiB` (unchanged clusters) and the overlay holds the `30 GiB` of new writes; the guest still sees `500 GiB`. The chain's read path checks the overlay first, then the base.
3. The overlay becomes unreadable; the VM fails to start or the disk is corrupt. `qemu-img info --backing-chain web01.qcow2` (or `virsh domblklist`) would show the dependency. Never delete a base that has overlays.
4. `blockcommit` (live, with libvirt/QEMU support) merges the overlay into the base; `blockpull` copies base data into the overlay. The main risk is I/O load and a long job; take a backup first, monitor free space, and do it in a maintenance window if the array is saturated.

**B6**

1. Theoretical `10 Gb/s ÷ 8 = 1.25 GB/s`; practical single-flow ceiling is about `1.1 GB/s` after protocol overhead.
2. `10 GB / 1.1 GB/s ≈ 9.1 s`.
3. `5,000 × 8 KiB = 40,000 KiB/s ≈ 39 MiB/s`. For a sequential 10 GB read, the link is not the bottleneck if the array can stream sequentially; if the file is fragmented or the array is IOPS-limited, 39 MB/s dominates and the transfer takes about 256 s. Measure both.
4. `8 × 100 MB/s = 800 MB/s`, which fits under 1.1 GB/s, but only if the server and storage can sustain it. Check server NIC utilization, NFS `nfsstat`, disk `iostat`, and whether the clients' aggregate is limited by the server's single flow or by the array. LACP does not make one TCP flow faster; it distributes flows.

### Part C

**C1 — fstab/mount unit**

- **Fault domain:** configuration (fstab) or the device/filesystem, not "the OS is broken".
- **Next checks:** `systemctl status data.mount`, `journalctl -xb | grep -i -A5 data`, `findmnt --verify`, `lsblk -f`, `blkid`.
- **Fix:** at the emergency prompt, `mount -o remount,rw /`; correct the UUID/device/options in `/etc/fstab` (or comment the line); `systemctl daemon-reload`; `systemctl reset-failed data.mount`; `mount -a`. If the filesystem is dirty and unmounted, run `fsck` on the device, then reboot. If the device is missing, fix the hypervisor/storage first.
- **Verify:** `findmnt /data`, `systemctl status data.mount`, reboot and confirm no emergency mode.

**C2 — deleted-open file or reserved blocks**

- **Fault domain:** filesystem/inode accounting, not necessarily a full disk.
- **Next checks:** `lsof +L1 /var`, `lsof /var | grep deleted`, `df -i /var`, `tune2fs -l /dev/vg0/var | grep -i reserved`.
- **Fix:** restart the process holding the deleted file, or truncate its fd (`: > /proc/<pid>/fd/<fd>`) after confirming what it is. If reserved blocks are the cause, adjust `tune2fs -m` only with intent; root-reserved space is a safety margin.
- **Verify:** `df -h /var` and `df -i /var` show free space; `lsof +L1` is empty.

**C3 — filesystem not grown**

- **Fault domain:** filesystem layer, below LVM.
- **Next checks:** `findmnt -T /data`, `df -T /data`, `lsblk /dev/vg_data/data`, `lvdisplay`.
- **Fix:** ext4: `resize2fs /dev/vg_data/data`; XFS: `xfs_growfs /data` (must be mounted). Then verify.
- **Verify:** `df -h /data` shows `30G`, `dumpe2fs -h` or `xfs_info /data` matches.

**C4 — thin pool full**

- **Fault domain:** LVM thin pool (data and/or metadata), not the guest filesystem.
- **Next checks:** `lvs -a -o name,data_percent,metadata_percent`, `dmsetup status`, `journalctl -k | grep -i thin`, `vgs`.
- **Fix:** stop writes; delete or archive old thin volumes/snapshots; `fstrim` the guests; add a PV and extend the pool (`vgextend vg_data /dev/sdX`, `lvextend -L +... vg_data/thinpool`); if metadata is full, extend metadata (`lvextend --poolmetadatasize +...`) and be prepared for recovery. A full pool can make the LV read-only; do not assume a simple resize fixes corruption.
- **Verify:** `Data%` and `Meta%` below thresholds; `dmsetup status` shows no `out_of_data_space`; guest writes succeed.

**C5 — NFS reachability/server**

- **Fault domain:** network/firewall or NFS server, not the client mount options yet.
- **Next checks:** `ping -c3 10.0.0.10`, `ss -lntp | grep 2049`, `rpcinfo -p 10.0.0.10`, `exportfs -v` on the server, firewall counters, `tcpdump -ni any port 2049 or port 111`.
- **Fix:** start `nfs-server`, ensure the export exists and is exported (`exportfs -ra`), open TCP 2049 (and 111 for NFSv3 ancillary services), correct the client mount. If the mount hangs despite reachability, check NFSv4 idmap/domain and server logs.
- **Verify:** `showmount -e` (v3) or `mount -t nfs4` succeeds; `findmnt /mnt/data`; read/write a test file; `nfsstat -m` shows the mount.

**C6 — iSCSI discovery/login**

- **Fault domain:** target configuration, network/firewall, or initiator authentication.
- **Next checks:** `ss -lntp | grep 3260` on the target, `targetcli ls`, `systemctl status iscsid`, `journalctl -u iscsid`, `iscsiadm -m session`, firewall, CHAP credentials.
- **Fix:** create the portal/target/LUN and ACL for the initiator IQN; start `iscsid`; open TCP 3260; run discovery, then `iscsiadm -m node -T <iqn> -p <portal> --login`; rescan (`iscsiadm -m session --rescan`), then `lsblk` and `multipath -ll`.
- **Verify:** `iscsiadm -m session` shows the session; the LUN appears in `lsblk`; `mkfs`/mount works; `multipath -ll` shows active paths.

**C7 — guest slow, host fast**

- **Fault domain:** guest/hypervisor data path, not the physical NVMe.
- **Next checks:** host `iostat -x 1`, `virsh domblkstat web01`, `qemu-img info --backing-chain`, `virsh dumpxml | grep -i cache`, `pidstat -p <qemu-pid> 1`, guest `blktrace`/`fio`.
- **Fix:** identify whether the bottleneck is queue depth in the guest, qcow2 metadata/backing chain, cache mode, CPU contention in QEMU, or a host-side limit. Try `cache=none`, a raw/preallocated disk, virtio-scsi with multiple queues, fewer snapshots, or move the VM to a less loaded host. Do not "fix" it by adding vCPUs blindly.
- **Verify:** guest `iostat` `await` and `%util` drop; `fio` shows expected IOPS/latency; host and guest measurements agree.

**C8 — VM storage path/permissions**

- **Fault domain:** hypervisor storage path, permissions, or MAC (SELinux/AppArmor), not the guest OS.
- **Next checks:** `virsh domblklist web01`, `ls -l /var/lib/libvirt/images/`, `namei -l <path>`, `getfacl`, `ausearch -m avc -ts recent` or `dmesg | grep -i apparmor`, `qemu-img info <path>`.
- **Fix:** restore the file from backup/snapshot; `chown qemu:qemu` (or the libvirt user), correct mode/ACL, `restorecon -Rv` on the path, reload the AppArmor profile, or fix the XML to point at the real path.
- **Verify:** `virsh start web01` succeeds; `virsh domblklist` shows the expected source; guest boots and can read/write.

### Part D

**D1 — Example design**

1. **CPU:** 24 × 8 = 192 vCPUs. With 3 hosts, 64 vCPUs per host; a 2:1 overcommit is acceptable for build VMs but not for databases. Use 1:1 for the 6 DBs (48 vCPU total) and 2:1 for build (144 vCPU virtual → 72 physical). Per host: about 40 physical vCPUs committed, with 64 cores available; leave headroom for host services and failure. **Memory:** 24 × 32 = 768 GiB; no overcommit for DBs; 3 hosts × 320 GiB = 960 GiB, leaving ~64 GiB per host for the hypervisor and ballooning. If one host fails, the remaining two must absorb 8 VMs: 16 VMs × 32 = 512 GiB per host, which fits in 320 GiB only with overcommit; therefore either reserve more RAM, reduce VM density, or accept degraded performance. State this trade-off explicitly.
2. **Storage:** 24 × 500 GiB = 12,000 GiB = 11.72 TiB virtual. At 3:1 thin, about 4 TiB actual. Use 3 hosts × 4 TiB NVMe = 12 TiB raw, RAID10 → 6 TiB usable, leaving room for snapshots and growth. If using Ceph with 3× replication, 12 TiB raw → 4 TiB usable, which is exactly the thin target with no headroom; increase raw capacity to 18–24 TiB or use erasure coding for cold data. Databases get thick or monitored thin volumes; build VMs get thin, disposable volumes.
3. **Block** for databases and VM disks (low latency, no POSIX locking surprises). **File** (NFS/CephFS) for shared config, ISOs, and build caches that many hosts read. **Object** (S3/Ceph RGW) for backups, artifacts, logs, and model/dataset artifacts. Do not put a database on NFS.
4. A qcow2 snapshot is a point-in-time disk view; it protects against a bad upgrade or accidental file deletion, but it lives on the same storage, depends on the base, and is not a backup. It does not protect against host/storage loss, ransomware with storage access, or corruption of the base chain. Backups must be copied to independent storage (object store or off-site), verified by restore, and databases need transaction-log/point-in-time backups.
5. Failure modes: (a) thin pool exhaustion → alert `Data% > 75%`, `Metadata% > 70%`; (b) host loss → alert on cluster quorum/HA restart events and capacity headroom; (c) snapshot chain growth / backup failure → alert on chain depth, snapshot age, and backup job success. Add storage latency and VM restart success as leading signals.

**D2 — Write path**

```text
app write()
  → guest VFS / page cache
  → guest filesystem (ext4/XFS)
  → guest block layer / I/O scheduler
  → virtio-blk or virtio-scsi driver
  → virtqueue (guest → host)
  → KVM exit / vhost worker
  → QEMU block layer (raw or qcow2)
  → host page cache (if cache=writeback) or O_DIRECT (cache=none)
  → host block layer / scheduler / device mapper
  → NVMe driver → PCIe → NVMe SSD → NAND
```

1. Guest: `iostat -x`, `blktrace`, `fio`, `strace -f -e trace=write`. Host: `iostat -x`, `pidstat`, `virsh domblkstat`, `qemu-img info`, `perf`/`bpftrace` on the QEMU process. Storage: `nvme smart-log`, `smartctl`, array counters. Network (for iSCSI/NFS): `ss`, `tcpdump`, `nfsstat`, `iscsiadm -m session`.
2. QEMU is spending CPU on I/O emulation, qcow2 metadata, or contention; likely causes are `cache=writeback` with fsync-heavy guest I/O, a deep backing chain, insufficient virtio queues, CPU overcommit, or a host-side lock. Check `pidstat -p <qemu-pid>`, `perf top`, `qemu-img info --backing-chain`, and the cache mode.
3. `cache=writeback` acknowledges writes from the host page cache before they reach stable storage; a host power loss can lose the last few seconds (or more) of writes. It is acceptable for disposable build/test VMs, not for databases or anything with a durability requirement. Use `cache=none`/`directsync` and application-level fsync for durability.
4. Each write must walk the chain to find the topmost cluster, adding metadata lookups and copy-on-write overhead; snapshots fragment the image and consume space. Remediate with `blockcommit`/`blockpull` to flatten, keep chains shallow (1–2), and use storage-level snapshots for long retention.
5. Put checkpoints on a parallel/high-throughput filesystem or object store, write them sequentially, use large I/O, preallocate space, avoid qcow2 snapshot chains on the hot path, and consider local NVMe with asynchronous upload to object storage. Separate the checkpoint path from the training data path so a checkpoint burst does not starve reads.

---

## Scoring

| Section | Points | Your score | Target |
|---|---:|---:|---:|
| A — facts | 50 | | ≥ 40 |
| B — calculations | 30 | | ≥ 24 |
| C — diagnosis | 40 | | ≥ 32 |
| D — design | 40 | | ≥ 32 |
| **Total** | **160** | | **≥ 128 (80%)** |

If you score below 80%, do not start Phase 4. Re-read the specific section, redo the matching lab (L3.1 for virtualization, L3.2 for LVM/filesystems, L3.3 for NFS/iSCSI), and retake after 48 hours. A calculation error is a knowledge gap, not a typo: redo the arithmetic with units.

---

## Practical sign-off

Do these on the VM without notes. Each is pass/fail.

- [ ] Create a VM from a cloud image with `virt-install`, log in on the console, SSH in, and show the qemu process and libvirt XML.
- [ ] Take a snapshot, write a file, revert, and prove the file is gone; explain what happened to the overlay.
- [ ] Inspect a disk's cache mode, NIC model, vCPU pinning, and NUMA placement from `virsh dumpxml`; change one safely.
- [ ] Build a VG/LV, create a filesystem, mount it, extend the LV online, grow the filesystem, and show `df` before/after.
- [ ] Fill a small filesystem's inodes with empty files, diagnose with `df -i`, clean up, and explain why deleting a directory tree is slow.
- [ ] Create a deleted-open file, show the `df`/`du` mismatch, recover the space without rebooting, and explain the risk.
- [ ] Break `/etc/fstab` in a snapshot VM, boot to emergency mode, repair it, and verify with `findmnt --verify` and a reboot.
- [ ] Export NFS to a second VM; test `root_squash`, remount after a server restart, and capture the NFS traffic.
- [ ] Create an iSCSI target and LUN, log in from the client, create a filesystem, mount it, and show `iscsiadm -m session` and `multipath -ll`.
- [ ] Run `fio` random read/write and sequential tests; interpret IOPS, latency, queue depth, and `%util`, and name the bottleneck.
- [ ] If L3.4 was completed: show a healthy `ceph -s`, stop an OSD, show `degraded`, and explain the recovery without adding a new OSD blindly.
- [ ] Given a VM that will not start, find the exact storage/permission error and fix it; prove it with `virsh start` and a guest login.

---

## Oral exam (12 minutes)

Answer out loud, without notes. Record yourself.

1. "A VM is slow. Walk me from the guest `write()` to the physical disk and tell me what you would measure at each layer."
2. "A filesystem is full but `du` says it is not. What are the possible causes?"
3. "You extended the LV and `df` did not change. Why, and what do you run next?"
4. "What is a qcow2 backing chain, and why did deleting the base image break the VM?"
5. "When would you use NFS instead of iSCSI, and when would you use object storage instead of either?"
6. "A thin pool is at 100%. Walk me through the first five minutes."
7. "A database runs on a VM with `cache=writeback`. What is your durability argument, and what would you change?"
8. "Explain RAID10 vs RAID6 for a write-heavy database. Where does the write penalty come from?"
9. "How would you prove that a slow VM is caused by the host and not the guest?"
10. "What is the most dangerous command you ran in these labs, and what did you do to make it safe?"

If you use a term you cannot define (`extent`, `ballooning`, `backing chain`, `placement group`, `write penalty`), stop and define it before continuing.

---

## Retest log

| Date | A | B | C | D | Total | Weak areas | Remediation | Retest date |
|---|---:|---:|---:|---:|---:|---|---|---|
| | | | | | | | | |
| | | | | | | | | |

When you pass, continue to [Phase 4 — Containers and Kubernetes](../04-containers-and-kubernetes/README.md).
