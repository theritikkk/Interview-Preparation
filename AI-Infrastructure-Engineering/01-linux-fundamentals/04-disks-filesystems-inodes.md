# Disks, Filesystems, Inodes, Mounts, and LVM

> **The one-sentence model:** a block device is a pile of sectors; a partition/LVM volume carves it; a filesystem turns it into files and directories; a mount attaches that filesystem to the directory tree. Disk incidents happen at one of those four layers.

---

## 1. The Linux storage stack

```text
application
  open() / read() / write()
        ↓
VFS (virtual filesystem switch)
        ↓
filesystem: ext4, XFS, overlayfs, NFS, tmpfs
        ↓
page cache + writeback
        ↓
block layer: I/O scheduler, queues, merges
        ↓
device driver: NVMe, virtio-blk, SCSI, RAID
        ↓
physical device / network volume
```

When someone says "the disk is slow", you must ask **which layer**:

- filesystem fragmentation or metadata contention,
- page cache/writeback,
- block queue saturation,
- device latency/errors,
- network storage (NFS/iSCSI/Ceph),
- virtualization (virtio, cache mode).

---

## 2. Block devices and names

```bash
$ lsblk -o NAME,SIZE,TYPE,FSTYPE,MOUNTPOINTS
NAME                      SIZE TYPE FSTYPE      MOUNTPOINTS
nvme0n1                   40G disk
├─nvme0n1p1                1G part vfat        /boot/efi
├─nvme0n1p2                2G part ext4        /boot
└─nvme0n1p3               37G part LVM2_member
  └─ubuntu--vg-ubuntu--lv 37G lvm  ext4        /
sdb                      100G disk
└─sdb1                   100G part ext4        /data
```

| Name pattern | Meaning |
|---|---|
| `/dev/sda`, `/dev/sdb` | SCSI/SATA/USB/virtio disks |
| `/dev/nvme0n1` | NVMe namespace 1 |
| `/dev/nvme0n1p1` | Partition 1 on that NVMe |
| `/dev/vda` | virtio disk in a VM |
| `/dev/dm-0`, `/dev/mapper/vg-lv` | Device-mapper (LVM, LUKS, multipath) |
| `/dev/md0` | Linux software RAID |
| `/dev/loop0` | Loopback device (image file) |

Always confirm with `lsblk` before `mkfs`, `dd`, or `parted`. Device names are not stable across reboots; use UUIDs in `/etc/fstab`.

```bash
$ blkid
/dev/nvme0n1p2: UUID="0f0e..." TYPE="ext4" PARTUUID="..."
/dev/mapper/ubuntu--vg-ubuntu--lv: UUID="7a1c..." TYPE="ext4"
```

---

## 3. Partitions, LVM, and filesystems

### Partitions

- **MBR**: 4 primary partitions, 2 TiB limit, legacy.
- **GPT**: 128 partitions typical, larger disks, checksums, used by UEFI.

```bash
sudo parted /dev/sdb print
sudo parted /dev/sdb mklabel gpt
sudo parted -a optimal /dev/sdb mkpart primary ext4 0% 100%
```

### LVM

LVM adds a mapping layer:

```text
physical disk/partition → PV (physical volume)
                        → VG (volume group, a pool)
                        → LV (logical volume, a block device)
                        → filesystem
```

```bash
$ sudo pvs
  PV         VG        Fmt  Attr PSize   PFree
  /dev/sdb   data      lvm2 a--  100.00g 100.00g

$ sudo vgs
  VG   #PV #LV #SN Attr   VSize   VFree
  data   1   0   0 wz--n- 100.00g 100.00g

$ sudo lvcreate -L 20G -n app data
  Logical volume "app" created.

$ sudo mkfs.ext4 /dev/data/app
$ sudo mkdir -p /srv/app
$ echo '/dev/data/app /srv/app ext4 defaults 0 2' | sudo tee -a /etc/fstab
$ sudo mount -a
$ findmnt /srv/app
TARGET   SOURCE         FSTYPE OPTIONS
/srv/app /dev/mapper/data-app ext4 rw,relatime
```

Extend:

```bash
sudo lvextend -L +10G /dev/data/app
sudo resize2fs /dev/data/app     # ext4, online
# XFS: sudo xfs_growfs /srv/app
```

LVM is not RAID. It gives flexibility, snapshots, and pooling; redundancy requires RAID/multipath/replication underneath.

### Filesystem choice

| | ext4 | XFS |
|---|---|---|
| Growth | Online grow | Online grow |
| Shrink | Offline shrink possible | **Not supported** |
| Large files | Good | Excellent |
| Many small files | Good | Good, but inode allocation differs |
| Default on | Ubuntu/Debian | RHEL/Rocky |
| Repair | `fsck` | `xfs_repair` |
| Snapshots | LVM, not native | LVM/reflink on some setups |

For a general-purpose Linux server, either is fine. Know the one your distribution uses and how to grow/repair it.

---

## 4. Mounts and `/etc/fstab`

```bash
$ findmnt -T /srv/app
TARGET   SOURCE         FSTYPE OPTIONS
/srv/app /dev/mapper/data-app ext4 rw,relatime

$ mount | grep ' /srv/app '
/dev/mapper/data-app on /srv/app type ext4 (rw,relatime)
```

`/etc/fstab` fields:

```text
<device>        <mountpoint>  <type>  <options>       <dump> <pass>
UUID=7a1c...    /srv/app      ext4    defaults,noatime 0     2
```

| Field | Meaning |
|---|---|
| device | Prefer `UUID=` or `LABEL=` |
| mountpoint | Directory; must exist |
| type | `ext4`, `xfs`, `nfs`, `tmpfs`, ... |
| options | `defaults`, `noatime`, `ro`, `noexec`, `nosuid`, `nodev` |
| dump | Legacy backup flag; usually `0` |
| pass | `fsck` order: root `1`, others `2`, none `0` |

```bash
sudo mount -a          # test fstab without rebooting
sudo systemctl daemon-reload
```

A bad fstab entry can prevent boot. Always test with `mount -a` and keep a root shell/snapshot.

Mount options that matter:

| Option | Effect |
|---|---|
| `ro` | Read-only; often set automatically after I/O errors |
| `noexec` | Cannot execute binaries from the mount |
| `nosuid` | SUID/SGID bits ignored |
| `nodev` | Device nodes ignored |
| `noatime` | Do not update access time; less write amplification |
| `sync` | Synchronous writes; slow, rarely appropriate |
| `nfs` options | `hard`/`soft`, `intr`, `vers=4.2`, `timeo` |

---

## 5. `df` vs `du` vs inodes

```bash
$ df -hT
Filesystem                        Type  Size  Used Avail Use% Mounted on
/dev/mapper/ubuntu--vg-ubuntu--lv ext4   37G   36G  0.5G  99% /
/dev/sdb1                         ext4  100G   20G   80G  20% /data

$ df -i
Filesystem                        Inodes  IUsed   IFree IUse% Mounted on
/dev/mapper/ubuntu--vg-ubuntu--lv 2.4M   2.4M       0  100% /
/dev/sdb1                         6.5M   120K    6.4M    2% /data
```

| Command | Answers |
|---|---|
| `df -h` | How much **block space** is used per filesystem? |
| `df -i` | How many **inodes** are used per filesystem? |
| `du -sh /path` | How much space do files under this path consume? |
| `du -x` | Stay on one filesystem |
| `du --inodes` | Count inodes per directory (GNU coreutils) |

Why they disagree:

1. **Deleted-but-open file** — `df` counts the blocks; `du` cannot see a name.
2. **Reserved blocks** — ext4 reserves 5% for root by default; `Avail` is smaller than `Size - Used`.
3. **Sparse files** — `du` counts allocated blocks; `ls -l` shows apparent size.
4. **Different mount** — `du` without `-x` crosses into other filesystems.
5. **Permissions** — `du` cannot see directories it cannot read, so it undercounts.
6. **Bind mounts/overlay** — the same blocks may appear at multiple paths.
7. **tmpfs** — consumes RAM, not disk; `df` shows it as a filesystem.

```bash
# find the biggest directories, staying on one filesystem
sudo du -xhd1 / | sort -h

# find the biggest files
sudo find / -xdev -type f -size +500M -exec ls -lh {} \; 2>/dev/null
```

---

## 6. Inode exhaustion

An inode is consumed per file, directory, symlink, socket, etc. A filesystem can have free blocks but zero inodes.

```bash
$ df -i /
Filesystem      Inodes  IUsed  IFree IUse% Mounted on
/dev/sda1       2.4M    2.4M   0     100% /

$ sudo du --inodes -xhd1 /var | sort -h | tail
1200000 /var/spool/postfix
...
```

Common causes: mail queues, session files, cache directories, container layers, millions of tiny logs, runaway temp files.

```bash
# count entries in a directory without sorting (fast)
ls -f /var/spool/postfix | wc -l

# find directories with the most entries (GNU find)
sudo find /var -xdev -type d -printf '%p\n' | while read -r d; do
  printf '%8d %s\n' "$(ls -f "$d" | wc -l)" "$d"
done | sort -nr | head
```

Fixing inode exhaustion means deleting files, not adding space. Add inodes only by recreating the filesystem (or using a filesystem that allocates inodes dynamically, like XFS).

---

## 7. Deleted-but-open files

```bash
$ df -h /
Filesystem      Size  Used Avail Use% Mounted on
/dev/sda1        40G   40G     0 100% /

$ sudo du -xsh /var/* | sort -h | tail -3
120M    /var/lib
80M     /var/log
20M     /var/tmp

$ sudo lsof +L1
COMMAND   PID USER   FD   TYPE DEVICE SIZE/OFF NLINK   NODE NAME
app       912 app    3w   REG  253,0 38G      0 131500 /var/log/app.log (deleted)
```

Fixes:

```bash
# Option 1: ask the process to reopen its log (SIGHUP/USR1, depends on app)
sudo kill -HUP 912

# Option 2: truncate through the open fd (safe, no restart)
sudo truncate -s 0 /proc/912/fd/3

# Option 3: restart the service during a maintenance window
sudo systemctl restart app
```

Do not `rm` the `/proc/912/fd/3` path unless you understand it; truncation is usually what you want.

---

## 8. I/O performance: IOPS, throughput, latency, queue

| Metric | Meaning | Unit |
|---|---|---|
| IOPS | I/O operations per second | ops/s |
| Throughput | Bytes per second | MB/s |
| Latency | Time per I/O | ms/µs |
| Queue depth | Outstanding I/Os | count |
| Utilization | Fraction of time the device is busy | % |

A disk can be 100% utilized and still deliver low throughput because each I/O is slow (latency-bound), or deliver high IOPS with small requests (throughput low). Do not equate `%util` with "the disk is full".

```bash
$ iostat -xz 1
Device            r/s     w/s     rkB/s   wkB/s  await  aqu-sz  %util
nvme0n1         120.0    80.0   4800.0  3200.0   0.42    0.08   2.1
sdb               0.0   420.0      0.0  33600.0  18.40    7.60  99.8
```

- `await` is average latency (queue + service).
- `aqu-sz` is average queue depth.
- `%util` near 100 on `sdb` with high `await` and queue depth indicates saturation.
- NVMe can show high `%util` but still low latency because of parallelism; look at `await` and `aqu-sz`.

```bash
$ sudo iotop -oPa
$ sudo fio --name=randread --ioengine=libaio --direct=1 --bs=4k --iodepth=32 \
  --rw=randread --size=1G --runtime=30 --filename=/tmp/fio-test
```

`fio` is for lab benchmarking, not for hammering a production volume.

### Device errors

```bash
$ sudo dmesg -T | grep -i -E 'I/O error|medium error|nvme|ata'
[Sun Mar 15 03:12:01 2026] blk_update_request: I/O error, dev sdb, sector 123456 op 0x0:(READ)
[Sun Mar 15 03:12:01 2026] EXT4-fs (sdb1): Remounting filesystem read-only
```

A filesystem often remounts read-only after an I/O error to prevent corruption. Fix the device/network first; `mount -o remount,rw` may appear to work but the underlying problem remains.

```bash
$ sudo smartctl -a /dev/sdb | grep -E 'SMART overall|Reallocated|Pending|Uncorrectable|Temperature'
```

---

## 9. Worked scenarios

### Scenario A — disk full, but `du` says there is space

```bash
$ df -h /
Filesystem      Size  Used Avail Use% Mounted on
/dev/sda1        40G   40G     0 100% /
$ sudo du -xsh /var/* | sort -h | tail
120M /var/lib
80M  /var/log
```

Run `lsof +L1`. If a deleted file is held open, truncate it through `/proc/<pid>/fd/<fd>` or restart the writer. If not, check reserved blocks (`tune2fs -l`), `tmpfs`, and other mounts with `df -hT`.

### Scenario B — inode full

```bash
$ df -i /
Filesystem      Inodes  IUsed  IFree IUse% Mounted on
/dev/sda1       2.4M    2.4M   0     100% /
$ sudo du --inodes -xhd1 /var | sort -h | tail
1200000 /var/spool/postfix
```

Delete the stale queue/session/cache files, fix the producer, and add an alert on `df -i`. Adding disk space will not help.

### Scenario C — filesystem remounted read-only

```bash
$ touch /data/test
touch: cannot touch '/data/test': Read-only file system

$ findmnt /data
TARGET SOURCE    FSTYPE OPTIONS
/data  /dev/sdb1 ext4   ro,relatime

$ sudo dmesg -T | tail
[...] EXT4-fs error (device sdb1): ...
[...] EXT4-fs (sdb1): Remounting filesystem read-only
```

Do not immediately `remount,rw`. Capture `dmesg`, check SMART, check the SAN/NFS/iSCSI path. If the device is healthy, unmount and `fsck` (ext4) or `xfs_repair` (XFS) during a maintenance window. Remounting read-write on a corrupt filesystem can make it worse.

### Scenario D — slow writes on a database host

```bash
$ iostat -xz 1
Device  r/s   w/s   rkB/s  wkB/s  await  aqu-sz  %util
sdb     2.0  800.0    64   6400  42.0   33.6   100.0
```

Evidence: write latency 42 ms, queue depth 33, device saturated. Hypotheses: undersized volume, noisy neighbour, write cache disabled, fsync-heavy workload, RAID rebuild, network storage. Tests: `fio` in a lab, check `%util`/`await`, check SAN/NFS latency, check `dmesg`, check the workload's fsync rate. Do not "fix" it by adding `noatime` alone; it will not reduce fsync.

### Scenario E — LVM extend did not grow the filesystem

```bash
$ df -h /srv/app
Filesystem              Size  Used Avail Use% Mounted on
/dev/mapper/data-app    20G   18G  1.0G  95% /srv/app

$ sudo lvextend -L +10G /dev/data/app
$ df -h /srv/app
# still 20G
```

`lvextend` grows the block device, not the filesystem. Run `resize2fs` for ext4 or `xfs_growfs` for XFS:

```bash
sudo resize2fs /dev/data/app
# or
sudo xfs_growfs /srv/app
```

---

## 10. Common mistakes

| Mistake | Why it is wrong | Better |
|---|---|---|
| `mkfs` on the wrong device | Destroys data instantly | `lsblk`, `blkid`, confirm twice |
| Using `/dev/sdX` in fstab | Names can change | Use `UUID=` |
| `fsck` on a mounted read-write fs | Can corrupt data | Unmount or boot rescue |
| `mount -o remount,rw` after I/O errors | Hides corruption | Investigate device first |
| Deleting a deleted-open file's path | Fails; file has no name | Truncate via `/proc/<pid>/fd` |
| Adding disk for inode exhaustion | Blocks are not the problem | Delete files / recreate fs |
| Ignoring `df -i` | Inode full looks like disk full | Check both |
| `du` across mounts | Counts other filesystems | `du -x` |
| `%util` = "disk full" | It is busy time, not capacity | `await`, `aqu-sz`, IOPS |
| No `mount -a` test | Bad fstab can block boot | Test before reboot |

---

## 11. Command reference (quick)

```bash
# topology
lsblk -o NAME,SIZE,TYPE,FSTYPE,MOUNTPOINTS
blkid
findmnt
findmnt -T /path
df -hT
df -i

# usage
du -xhd1 /var | sort -h
du -xsh /path
lsof +L1
sudo lsof | grep deleted

# mount
mount /dev/sdb1 /mnt
mount -a
umount /mnt
mount -o remount,ro /mnt

# LVM
pvs; vgs; lvs
pvcreate /dev/sdb
vgcreate data /dev/sdb
lvcreate -L 20G -n app data
lvextend -L +10G /dev/data/app
resize2fs /dev/data/app
xfs_growfs /mount

# filesystem
mkfs.ext4 /dev/sdb1
mkfs.xfs /dev/sdb1
fsck -n /dev/sdb1
xfs_repair -n /dev/sdb1

# performance / health
iostat -xz 1
iotop -oPa
fio --name=test --rw=randread --bs=4k --iodepth=32 --size=1G --runtime=30 --filename=/tmp/fio-test
smartctl -a /dev/sdb
dmesg -T | grep -i -E 'I/O error|read-only|nvme|ata'
```

---

## 12. Checkpoint

1. What is the difference between a partition, an LVM LV, and a filesystem?
2. Why can `df -h` show 100% while `du` shows only 20 GB used?
3. How do you find a deleted-but-open file, and how do you free the space safely?
4. What is inode exhaustion, and why does adding disk space not fix it?
5. What does `df -i` show that `df -h` does not?
6. Why should `/etc/fstab` use UUIDs?
7. What does `%util=100` tell you, and what does it not tell you?
8. What is the safe order for extending an LVM volume and its filesystem?
9. What would you check before running `fsck` on a production filesystem?
10. A filesystem remounted read-only after an I/O error. What is the first thing you do?

If you miss more than two, redo Lab L1.5.
