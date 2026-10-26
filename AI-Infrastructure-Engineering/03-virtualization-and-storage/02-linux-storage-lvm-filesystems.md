# Linux Storage: Disks, LVM, and Filesystems

> **The one-sentence model:** a disk is a block device; a filesystem gives blocks names and structure; LVM inserts a flexible mapping layer between them. Every "disk full" or "slow disk" ticket is one of: blocks, inodes, deleted-open files, a full pool, or a saturated device.

---

## 1. The block stack

```text
application
  open/read/write
filesystem        ext4, XFS
  block I/O
LVM / dm          PV → VG → LV (optional)
  block I/O
partition table   GPT/MBR
  sectors
device            NVMe, SSD, HDD, virtio-blk, iSCSI LUN
```

Each layer can be the bottleneck. `df` sees the filesystem; `lsblk` sees the block devices; `iostat` sees the device; `fio` measures the device; `strace` sees the syscalls.

---

## 2. Identify the device before touching it

```bash
lsblk -o NAME,SIZE,TYPE,FSTYPE,MOUNTPOINT,MODEL,SERIAL
```

```text
NAME   SIZE TYPE FSTYPE MOUNTPOINT MODEL            SERIAL
vda     40G disk        ubuntu           QEMU HARDDISK     drive-virtio-disk0
├─vda1   39G part ext4   /
├─vda14   4M part
└─vda15 106M part vfat   /boot/efi
vdb     20G disk
└─vdb1   20G part ext4   /data
```

```bash
blkid
```

```text
/dev/vda1: UUID="..." TYPE="ext4" PARTUUID="..."
/dev/vdb1: UUID="..." TYPE="ext4" PARTUUID="..."
```

```bash
findmnt
```

```text
TARGET    SOURCE    FSTYPE OPTIONS
/         /dev/vda1 ext4   rw,relatime
/data     /dev/vdb1 ext4   rw,relatime
```

**Rule:** use `lsblk` + `blkid` + `findmnt` together. A device name can change; a UUID or labels is stable. `lsblk` shows the tree; `findmnt` shows what is actually mounted.

### Partitions

```bash
sudo parted /dev/vdb print
sudo sgdisk -p /dev/vdb
```

GPT is standard for disks > 2 TB and for UEFI. MBR is legacy, limited to 2 TB and four primary partitions (without extended partitions).

Create a partition (destructive; lab disk only):

```bash
sudo parted /dev/vdb --script mklabel gpt
sudo parted /dev/vdb --script mkpart primary ext4 1MiB 100%
sudo partprobe /dev/vdb
```

**Mistake:** running `parted` on `/dev/vda` instead of `/dev/vdb`. Always confirm the disk model/serial and that it is not mounted.

---

## 3. LVM

LVM inserts a mapping layer:

```text
physical disk/partition
        │ pvcreate
        ▼
   physical volume (PV)
        │ vgcreate
        ▼
  volume group (VG)  ← pool of extents
        │ lvcreate
        ▼
  logical volume (LV) → /dev/vg0/data
        │ mkfs
        ▼
     filesystem
```

Why use it:

- Resize volumes without repartitioning.
- Span multiple disks.
- Snapshots (with limitations).
- Thin provisioning.
- Move data between disks (`pvmove`).

### Create

```bash
sudo pvcreate /dev/vdb
sudo vgcreate vg_data /dev/vdb
sudo lvcreate -L 10G -n data vg_data
sudo mkfs.ext4 /dev/vg_data/data
sudo mkdir -p /data
sudo mount /dev/vg_data/data /data
```

Check:

```bash
sudo pvs
sudo vgs
sudo lvs
```

```text
  PV         VG      Fmt  Attr PSize   PFree
  /dev/vdb   vg_data lvm2 a--  <20.00g <10.00g

  VG      #PV #LV #SN Attr   VSize   VFree
  vg_data   1   1   0 wz--n- <20.00g <10.00g

  LV   VG      Attr       LSize  Pool Origin Data%  Meta%  Move Log Cpy%Sync Convert
  data vg_data -wi-ao---- 10.00g
```

`Attr` decoding: `w` writable, `i` inherited, `a` active, `o` open. `-wi-ao----` is a normal active LV.

### Extend online

```bash
sudo lvextend -L +5G /dev/vg_data/data
sudo resize2fs /dev/vg_data/data      # ext4
# or: sudo xfs_growfs /data            # XFS
df -h /data
```

For XFS, the filesystem must be mounted to grow; `xfs_growfs` takes the mountpoint, not the device. ext4 can grow online while mounted; shrinking ext4 is possible offline but rarely worth it. XFS cannot shrink.

### Extend the VG

```bash
sudo pvcreate /dev/vdc
sudo vgextend vg_data /dev/vdc
sudo lvextend -l +100%FREE /dev/vg_data/data
sudo resize2fs /dev/vg_data/data
```

`-l +100%FREE` uses extents; `-L +5G` uses absolute size. Check `vgs` for free extents before extending.

### Move data off a disk

```bash
sudo pvmove /dev/vdb /dev/vdc
sudo vgreduce vg_data /dev/vdb
sudo pvremove /dev/vdb
```

`pvmove` can take a long time and temporarily doubles I/O. Do it in a maintenance window or with throttling.

### Thin provisioning

```bash
sudo lvcreate -L 100G -T vg_data/thinpool
sudo lvcreate -V 20G -T vg_data/thinpool -n vm1
```

Thin LVs are virtual; the pool allocates blocks on write. Overprovisioning is possible, but if the pool fills, writes fail and filesystems can corrupt. Monitor `Data%` and `Meta%`:

```bash
sudo lvs -o name,vg_name,lv_size,pool_lv,data_percent,metadata_percent
```

**Rule:** never let a thin pool reach 100%. Alert at 80% data and 70% metadata.

### Snapshots

```bash
sudo lvcreate -L 2G -s -n data_snap /dev/vg_data/data
sudo lvremove /dev/vg_data/data_snap
```

An LVM snapshot is copy-on-write: the origin's changed blocks are copied to the snapshot. It is not a backup, and it slows writes to the origin. Size the snapshot for the expected change rate × snapshot lifetime. If it fills, it becomes invalid.

---

## 4. Filesystems

### ext4 vs XFS

| | ext4 | XFS |
|---|---|---|
| Grow online | Yes | Yes |
| Shrink | Offline, possible | No |
| Large files | Good | Excellent |
| Many small files | Good | Good, but inode allocation differs |
| Metadata performance | Good | Often better under parallel load |
| Default on | Debian/Ubuntu | RHEL/Rocky |
| Repair | `e2fsck` | `xfs_repair` |

Choose based on platform conventions and workload, not folklore. Both are journaling filesystems; both can lose recent writes on power loss unless the application fsyncs.

### Create and mount

```bash
sudo mkfs.ext4 -L data /dev/vg_data/data
sudo mkdir -p /data
sudo mount /dev/vg_data/data /data
```

Persist in `/etc/fstab`:

```fstab
UUID=xxxx-xxxx  /data  ext4  defaults,nofail  0  2
```

- Use `UUID=` or `LABEL=`, not `/dev/sdb1` (device names can change).
- `nofail` lets the boot continue if the disk is missing.
- `x-systemd.automount` mounts on first access instead of at boot.
- `0 2` = no dump, fsck order 2 (root is 1).
- For XFS, the last field is `0 0` (no fsck; use `xfs_repair`).

Apply without reboot:

```bash
sudo mount -a
findmnt /data
```

**Mistake:** a typo in `/etc/fstab` can drop the system to emergency mode. Always run `mount -a` and test before rebooting, and keep a console/snapshot.

### fstab recovery

If the system boots to emergency mode:

```bash
mount -o remount,rw /
cat /etc/fstab
mount -a
```

Fix the bad line, or comment it out, then `systemctl reboot`. If the root filesystem is read-only, remount it first. A `nofail` option prevents this class of outage for non-root mounts.

### Inodes vs blocks

A filesystem has two resources: data blocks and inodes (metadata for files). `df -h` shows blocks; `df -i` shows inodes.

```bash
df -h /data
df -i /data
```

```text
Filesystem      Size  Used Avail Use% Mounted on
/dev/vdb1        20G   19G  1.0G  95% /data

Filesystem     Inodes IUsed IFree IUse% Mounted on
/dev/vdb1      1.3M   1.3M     0  100% /data
```

A directory with millions of tiny files can exhaust inodes while blocks are free. `df -h` looks fine; new files fail with `No space left on device`. Solutions: delete files, use a filesystem with more inodes (XFS allocates dynamically), or change the workload (pack small files).

### Deleted-open files

A process holds a deleted file open; `df` shows space used, but `du` cannot find it.

```bash
sudo lsof +L1
```

```text
COMMAND   PID USER   FD   TYPE DEVICE SIZE/OFF NLINK    NODE NAME
java     4242 app    5w   REG  252,1  8.2G      0 1234567 /var/log/app.log (deleted)
```

The space is reclaimed only when the process closes the file or exits. Truncate it through `/proc`:

```bash
sudo truncate -s 0 /proc/4242/fd/5
```

Do not `kill -9` a production process just to free disk space unless that is the safest option. Restarting the service is often the correct fix; truncating through `/proc` is a temporary measure.

### Permissions and mount options

| Option | Effect |
|---|---|
| `ro` | Read-only |
| `noexec` | Cannot execute binaries |
| `nosuid` | Ignore SUID/SGID |
| `nodev` | Ignore device files |
| `noatime` | Do not update access time (performance) |
| `discard` | Issue TRIM on delete (or use `fstrim.timer`) |
| `defaults` | rw,suid,dev,exec,auto,nouser,async |

For data disks, `nosuid,nodev,noexec` reduce attack surface if the data does not need to run binaries. Do not apply `noexec` to `/home` if users compile/run tools there.

---

## 5. Swap

```bash
swapon --show
free -m
cat /proc/swaps
```

```text
NAME      TYPE      SIZE USED PRIO
/swap.img file        2G   0B   -2
```

Swap extends memory but is much slower than RAM. A little swap is normal; heavy swap on a database is a capacity problem. Check:

```bash
vmstat 1
```

```text
procs -----------memory---------- ---swap-- -----io---- -system-- ------cpu-----
 r  b   swpd   free   buff  cache   si   so    bi    bo   in   cs us sy id wa st
 2  1 524288  12000   2000 880000  120  340    40   220  800 1200 20  5 60 15  0
```

`si`/`so` are swap-in/swap-out KB/s. Sustained nonzero values mean memory pressure.

For a swap file:

```bash
sudo fallocate -l 2G /swapfile
sudo chmod 600 /swapfile
sudo mkswap /swapfile
sudo swapon /swapfile
echo '/swapfile none swap sw 0 0' | sudo tee -a /etc/fstab
```

---

## 6. Performance: IOPS, throughput, latency

| Metric | Meaning | Sensitive workload |
|---|---|---|
| IOPS | I/O operations per second | Databases, small random reads |
| Throughput | MB/s | Backups, large sequential scans |
| Latency | Time per I/O (ms/µs) | Transactional systems |
| Queue depth | Outstanding I/Os | NVMe, RAID |
| I/O size | Bytes per request | Sequential vs random |

A device can be fast at large sequential I/O and slow at small random I/O. "The disk is slow" is meaningless without the access pattern.

### `iostat`

```bash
iostat -xz 1
```

```text
Device            r/s     w/s     rkB/s     wkB/s  rrqm/s  wrqm/s  await  %util
nvme0n1         200.0   800.0   6400.0   25600.0    0.0    12.0   0.35   18.2
```

- `r/s`, `w/s` — IOPS.
- `rkB/s`, `wkB/s` — throughput.
- `await` — average latency including queue time.
- `%util` — busy time. On NVMe with multiple queues, 100% does not necessarily mean saturated; use `aqu-sz` (average queue depth) and latency together.
- `rrqm/s`, `wrqm/s` — merged requests; high values suggest sequential access.

### `fio`

```bash
fio --name=randread --filename=/dev/vdb --direct=1 --rw=randread \
    --bs=4k --iodepth=32 --numjobs=4 --size=1G --runtime=30 --group_reporting
```

Output includes IOPS, bandwidth, and latency percentiles (`clat`). Compare:

- 4K random read with `iodepth=1` → latency-sensitive.
- 128K sequential write with `iodepth=32` → throughput.
- Mixed 70/30 read/write → realistic database.

**Warning:** `fio` writes to the device and destroys data. Use a scratch LUN/file, never a production disk.

### Page cache

Linux caches file data in unused RAM. A read may be served from cache; `iostat` sees nothing. `free -m` shows `buff/cache`; `vmstat` shows `bi`/`bo`. Drop caches only for testing:

```bash
sync
echo 3 | sudo tee /proc/sys/vm/drop_caches
```

This hurts performance and is not a fix for memory pressure.

### I/O scheduler

```bash
cat /sys/block/nvme0n1/queue/scheduler
```

```text
[none] mq-deadline kyber bfq
```

NVMe usually uses `none` (no scheduler) because the device has deep queues. SATA/SAS may benefit from `mq-deadline`. Do not change without a benchmark.

---

## 7. Failure scenarios

### A — `No space left on device` but `df -h` shows free space

Check inodes:

```bash
df -i
```

If `IUse%` is 100%, delete files or move to a filesystem with more inodes. If inodes are fine, check deleted-open files with `lsof +L1`.

### B — LVM extend succeeds but `df` does not grow

You extended the LV but not the filesystem:

```bash
lsblk
df -h /data
sudo resize2fs /dev/vg_data/data     # ext4
sudo xfs_growfs /data                # XFS
```

`lvextend -r` can resize the filesystem automatically for supported filesystems. Use it carefully; it still needs the right filesystem tools.

### C — Thin pool full

```bash
sudo lvs -o name,data_percent,metadata_percent
```

```text
  data          98.20
```

Writes fail, and the filesystem may be corrupted. Extend the pool (`lvextend`) or delete snapshots, but first stop the workload. Monitor and alert well before 100%.

### D — Boot drops to emergency mode after fstab edit

At the console:

```bash
mount -o remount,rw /
cat /etc/fstab
mount -a
```

Fix the entry, then reboot. If the UUID changed (recreated filesystem), update the UUID with `blkid`. Use `nofail` for non-essential mounts.

### E — Disk is slow only inside a VM

Host `iostat` shows the physical device is busy; guest `iostat` shows high `await`. Causes: host contention, qcow2 cache mode, long backing chain, or network storage. Check the host first; the guest cannot see the host's queue.

### F — `device or resource busy` on unmount

```bash
sudo lsof +f -- /data
sudo fuser -vm /data
```

A process has a file open, or the mount is nested. Stop the process or unmount the nested mount first. `umount -l` (lazy) hides the problem; use it only when you understand the risk.

---

## 8. Command field manual

### `lsblk`

```bash
lsblk -o NAME,SIZE,TYPE,FSTYPE,MOUNTPOINT,MODEL,SERIAL
```

Shows the block-device tree. `-f` adds filesystem info; `-p` shows full paths. Use it before any destructive command.

### `df` / `du`

```bash
df -hT
df -i
du -xh --max-depth=1 /var | sort -h
```

- `df` reports filesystem usage; `du` sums file sizes.
- A large gap between them often means deleted-open files or reserved blocks.
- `du` follows hard links once; use `-l` to count them.

### `findmnt`

```bash
findmnt
findmnt /data
findmnt --verify
```

`--verify` checks `/etc/fstab` syntax and mountability. Run it after editing fstab.

### `pvs` / `vgs` / `lvs`

```bash
sudo pvs -o pv_name,vg_name,pv_size,pv_free
sudo vgs -o vg_name,vg_size,vg_free,vg_extent_size
sudo lvs -o lv_name,vg_name,lv_size,data_percent,metadata_percent
```

These are the fastest way to see capacity at each LVM layer.

### `iostat`

```bash
iostat -xz 1 5
```

`-x` extended stats, `-z` omit idle devices, `1 5` every second for five samples. Read `await`, `%util`, and `aqu-sz` together.

### `fio`

```bash
fio --name=test --filename=/tmp/fio.test --size=1G --rw=randwrite \
    --bs=4k --iodepth=16 --direct=1 --runtime=30 --time_based
```

Use a file for a safe test; use a raw device only when you intend to destroy it.

### `lsof` / `fuser`

```bash
sudo lsof +L1
sudo lsof +f -- /data
sudo fuser -vm /data
```

Find deleted-open files and processes holding a mount.

---

## 9. Common mistakes

| Mistake | Why it hurts | Fix |
|---|---|---|
| Using `/dev/sdX` in fstab | Names change on reboot | UUID/LABEL |
| Editing fstab without `mount -a` | Emergency mode | Test before reboot; `nofail` |
| Extending LV but not FS | `df` unchanged | `resize2fs`/`xfs_growfs` |
| Growing XFS with `resize2fs` | Wrong tool | `xfs_growfs /mountpoint` |
| Shrinking XFS | Unsupported | Back up and recreate |
| Filling a thin pool | Write failures/corruption | Monitor, alert, extend |
| Treating snapshots as backups | Same disk/failure domain | Off-host backups |
| Deleting files held open | Space not freed | `lsof +L1`, truncate/restart |
| `df -h` only | Inode exhaustion invisible | `df -i` |
| `fio` on a production disk | Data loss | Scratch device/file |
| Dropping caches as a "fix" | Performance regression | Measure, tune, add RAM |
| Ignoring `await` | IOPS look fine but latency is terrible | Read latency and queue depth |

---

## 10. Checkpoint

1. What are PV, VG, and LV?
2. How do you extend an ext4 filesystem online?
3. How do you extend XFS, and why can it not shrink?
4. Why use UUID instead of `/dev/sdb1` in fstab?
5. What does `nofail` do?
6. How do you check inode exhaustion?
7. A deleted file still consumes space. How do you find it?
8. What is the difference between `df` and `du`?
9. What does `iostat` `await` include?
10. Why can `%util` be misleading on NVMe?
11. What happens when a thin pool fills?
12. Why is an LVM snapshot not a backup?
13. What does `pvmove` do, and what is the risk?
14. How do you test `/etc/fstab` without rebooting?
15. A VM's disk is slow, but the guest shows low `%util`. What do you check on the host?

### Answers

1. Physical volume (disk/partition), volume group (pool), logical volume (usable block device).
2. `lvextend -L +5G /dev/vg/lv` then `resize2fs /dev/vg/lv` (or `lvextend -r`).
3. `xfs_growfs /mountpoint`; XFS does not support shrinking.
4. Device names are not stable across boots/controllers.
5. Boot continues if the mount is unavailable.
6. `df -i`.
7. `lsof +L1`; truncate through `/proc/<pid>/fd/<fd>` or restart the process.
8. `df` reports filesystem-level used/free; `du` sums file sizes and can miss deleted-open files.
9. Queue time plus device service time.
10. NVMe has multiple queues; 100% busy can still have headroom. Use latency and queue depth.
11. Writes fail; the filesystem may be corrupted. Stop writes and extend/delete snapshots.
12. It shares the origin's disk and failure domain; corruption/loss of the origin loses the snapshot.
13. Moves extents off a PV to other PVs in the VG; it is I/O-intensive and can take hours.
14. `findmnt --verify` and `mount -a`.
15. Host `iostat`, qcow2 cache/backing chain, host memory pressure, network storage, and noisy neighbours.

If you miss more than three, redo sections 3–6 and Lab L3.2.
