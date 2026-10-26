# Shared Storage: NFS, iSCSI, Ceph, and NVMe-oF

> **The one-sentence model:** block storage gives the client a disk, file storage gives it a directory, object storage gives it an HTTP namespace. The client-side commands look similar (mount, format, read), but the failure modes and performance limits are completely different.

---

## 1. Block vs file vs object

| | Block | File | Object |
|---|---|---|---|
| Client sees | `/dev/sdb`, a LUN | `/mnt/share`, a directory | Bucket/key over HTTP |
| Protocol | iSCSI, NVMe-oF, FC, RBD | NFS, SMB | S3, Swift |
| Filesystem | Client formats it | Server owns it | None (object metadata) |
| Sharing | Usually one client | Many clients | Many clients |
| Latency | Lowest | Higher (protocol + server FS) | Highest |
| Use | VM disks, databases | Home dirs, shared config | Backups, datasets, artifacts |

A block LUN must be formatted by the client. Two clients writing the same non-cluster filesystem on one LUN will corrupt it. A file share is designed for concurrent clients, but locking and cache coherence still matter. Object storage is eventually consistent in some systems and has no POSIX semantics.

---

## 2. NFS

NFS exports a directory from a server; clients mount it over the network.

### Versions

| Version | Notes |
|---|---|
| NFSv3 | Stateless, separate `mountd`/`rpcbind`, widely supported |
| NFSv4 | Stateful, integrated locking, Kerberos, single port 2049 |
| NFSv4.1/4.2 | Sessions, pNFS, server-side copy, ACLs |

NFSv4 is the default on modern Linux. It uses TCP port 2049; NFSv3 needs `rpcbind` (111) and `mountd` (dynamic).

### Server

```bash
sudo apt install -y nfs-kernel-server
sudo mkdir -p /srv/nfs/data
sudo chown nobody:nogroup /srv/nfs/data
```

`/etc/exports`:

```exports
/srv/nfs/data  10.0.0.0/24(rw,sync,no_subtree_check)
```

```bash
sudo exportfs -rav
sudo exportfs -v
```

```text
/srv/nfs/data  	10.0.0.0/24(sync,wdelay,hide,no_subtree_check,sec=sys,rw,secure,root_squash,no_all_squash)
```

| Option | Meaning |
|---|---|
| `rw` | Read/write |
| `ro` | Read-only |
| `sync` | Reply after data is on stable storage (safe, slower) |
| `async` | Reply before commit (faster, risk on crash) |
| `root_squash` | Map client root to `nobody` (default, safer) |
| `no_root_squash` | Trust client root (dangerous) |
| `no_subtree_check` | Disable subtree checks (faster, slightly less strict) |
| `sec=sys` | AUTH_SYS (UID/GID trust, not encryption) |

`root_squash` is why root on the client cannot chown files on the server. If an application needs root-owned files, fix ownership on the server or use `all_squash` with explicit UID/GID mapping; do not disable root_squash casually.

### Client

```bash
sudo apt install -y nfs-common
sudo mkdir -p /mnt/nfs
sudo mount -t nfs 10.0.0.10:/srv/nfs/data /mnt/nfs
findmnt /mnt/nfs
```

```text
TARGET    SOURCE                     FSTYPE OPTIONS
/mnt/nfs  10.0.0.10:/srv/nfs/data    nfs4   rw,relatime,vers=4.2,rsize=1048576,wsize=1048576,...
```

Persist in fstab:

```fstab
10.0.0.10:/srv/nfs/data  /mnt/nfs  nfs4  defaults,_netdev,nofail  0  0
```

- `_netdev` waits for the network.
- `nofail` prevents boot failure if the server is down.
- `hard` (default) retries forever; `soft` returns errors after `timeo`/`retrans`. Use `soft` only for non-critical mounts; a `soft` mount can silently return I/O errors.

### Permissions

NFSv4 with `sec=sys` trusts the client's UID/GID. If UIDs differ between client and server, files appear owned by the wrong user. Check:

```bash
id alice          # client
getent passwd alice  # server
```

Options: centralise UIDs (LDAP/SSSD), use `all_squash` with `anonuid`/`anongid`, or use Kerberos (`sec=krb5p`) for authenticated, encrypted access.

### Performance and failure modes

- `rsize`/`wsize` (often 1 MB on modern NFS) affect throughput.
- NFS is sensitive to latency; a 1 ms RTT storage network is very different from a 10 ms one.
- `nfsstat -c` shows RPC calls, retransmits, and errors.
- `mountstats` / `nfsiostat` show per-operation latency.

```bash
nfsiostat 1
```

```text
   ops/s       rpc bklog
   120.0           0.0
read:  ops/s   kB/s   kB/op   retrans   avg RTT (ms)
      80.0   6400.0   80.0       0.0          0.8
```

Stale file handle:

```text
Stale file handle
```

The server no longer recognises the file (deleted/replaced, export changed, server rebooted with a different FS). Unmount/remount; if it persists, find what changed on the server. Do not `rm -rf` blindly.

Hanging mount:

```bash
mount | grep nfs
ps -eo stat,pid,comm | grep ' D '
```

A `D` state process is blocked in the kernel, often on NFS. `umount -f` may help; `umount -l` detaches but leaves the mount until references disappear. Fix the server/network first.

---

## 3. iSCSI

iSCSI carries SCSI commands over TCP. The client (initiator) sees a block device; the server (target) exports a LUN.

```text
initiator (client)  ── TCP 3260 ──  target (server)
  /dev/sdb                              backing file/LVM/ZFS zvol
```

### Vocabulary

| Term | Meaning |
|---|---|
| IQN | iSCSI Qualified Name, e.g. `iqn.2026-10.lab:client1` |
| Target | The exported storage endpoint |
| Portal | IP:port of the target (default 3260) |
| LUN | Logical unit number inside a target |
| ACL | Which initiator IQN may connect |
| CHAP | Challenge-Handshake Authentication Protocol |

### Server with `targetcli`

```bash
sudo apt install -y targetcli-fb
sudo targetcli
```

```text
/> backstores/fileio create name=disk1 file_or_dev=/srv/iscsi/disk1.img size=10G
/> iscsi/ create iqn.2026-10.lab:server
/> iscsi/iqn.2026-10.lab:server/tpg1/luns create /backstores/fileio/disk1
/> iscsi/iqn.2026-10.lab:server/tpg1/acls create iqn.2026-10.lab:client1
/> iscsi/iqn.2026-10.lab:server/tpg1/portals create 10.0.0.10
/> saveconfig
/> exit
```

For production, use a real block device or LVM LV, not a file-backed image. File-backed LUNs add a filesystem layer and are slower.

### Client with `iscsiadm`

```bash
sudo apt install -y open-iscsi
sudo systemctl enable --now iscsid

sudo iscsiadm -m discovery -t sendtargets -p 10.0.0.10
```

```text
10.0.0.10:3260,1 iqn.2026-10.lab:server
```

```bash
sudo iscsiadm -m node -T iqn.2026-10.lab:server -p 10.0.0.10 --login
sudo iscsiadm -m session
```

```text
tcp: [1] 10.0.0.10:3260,1 iqn.2026-10.lab:server (non-flash)
```

```bash
lsblk
```

```text
NAME   MAJ:MIN RM SIZE RO TYPE MOUNTPOINT
sdb      8:16   0  10G  0 disk
```

The LUN appears as `/dev/sdb` (or `/dev/sdX`). Format and mount it like a local disk. The iSCSI session must be up before the filesystem mounts; use `_netdev` and `iscsiadm` service ordering.

### CHAP

```bash
sudo iscsiadm -m node -T iqn.2026-10.lab:server -p 10.0.0.10 \
  --op update -n node.session.auth.authmethod -v CHAP
sudo iscsiadm -m node -T iqn.2026-10.lab:server -p 10.0.0.10 \
  --op update -n node.session.auth.username -v client1
sudo iscsiadm -m node -T iqn.2026-10.lab:server -p 10.0.0.10 \
  --op update -n node.session.auth.password -v 'secret'
```

CHAP protects the session, not the data in transit. Use IPsec or a private storage network for confidentiality.

### Multipath

Two paths to the same LUN should appear as one device, not two.

```bash
sudo apt install -y multipath-tools
sudo multipath -ll
```

```text
mpatha (36001405...) dm-0 QEMU,QEMU HARDDISK
size=10G features='1 queue_if_no_path' hwhandler='1 alua' wp=rw
|-+- policy='service-time 0' prio=50 status=active
| `- 1:0:0:0 sdb 8:16 active ready running
`-+- policy='service-time 0' prio=10 status=enabled
  `- 2:0:0:0 sdc 8:32 active ready running
```

Without multipath, the same LUN mounted twice causes corruption. With it, use `/dev/mapper/mpatha`, not `/dev/sdb`.

### Failure modes

- Session drops: `iscsiadm -m session` empty; check network, target ACL, CHAP.
- LUN disappears after reboot: `iscsid` not enabled, or `node.startup` is `manual`. Set `automatic`.
- Duplicate devices: multipath not configured or two sessions to the same LUN.
- Slow I/O: network congestion, TCP retransmits, target disk saturation. Check `ip -s link`, `ss -ti`, `iostat` on the target.

---

## 4. Ceph

Ceph is a distributed storage system that provides block (RBD), file (CephFS), and object (RGW) interfaces on one cluster.

```text
          ┌──────────────┐
          │    MON(s)    │  cluster map, quorum
          └──────┬───────┘
                 │
   ┌─────────────┼─────────────┐
   ▼             ▼             ▼
 MGR           OSDs          MDS (CephFS)
 metrics       data + CRUSH   metadata
   │             │
   └────── RGW / clients ─────┘
```

| Component | Role |
|---|---|
| MON | Maintains the cluster map; needs quorum (odd number, usually 3) |
| MGR | Metrics, dashboard, orchestration modules |
| OSD | Stores objects on a disk; handles replication/recovery |
| MDS | CephFS metadata server |
| RGW | S3/Swift gateway |
| CRUSH | Deterministic placement algorithm (no central lookup) |

### Pools and placement groups

A **pool** is a logical namespace with a replication or erasure-code policy. A **PG** (placement group) is a shard of a pool mapped to OSDs by CRUSH.

```bash
ceph osd pool create rbd 128 128
rbd pool init rbd
```

`128` PGs is a small lab value. Production PG counts are sized by `(OSDs × 100) / replicas`, rounded to a power of two, with limits.

### Replication

```bash
ceph osd pool set rbd size 3
ceph osd pool set rbd min_size 2
```

`size=3` means three copies; `min_size=2` means I/O continues with two. With `size=3, min_size=2`, the cluster can lose one OSD and stay available. With `min_size=1`, it can lose two but risks data loss.

### CRUSH

CRUSH maps an object to a set of OSDs using the cluster map, failure domain, and weights. It avoids a central index and survives OSD changes. The failure domain matters: if all replicas land in one rack, a rack failure loses data despite `size=3`.

```bash
ceph osd crush tree
ceph osd tree
```

### Health

```bash
ceph status
```

```text
  cluster:
    id:     ...
    health: HEALTH_OK

  services:
    mon: 3 daemons, quorum a,b,c
    mgr: x(active), y(standby)
    osd: 6 osds: 6 up, 6 in

  data:
    pools:   2 pools, 128 pgs
    objects: 1.2k objects, 4.8 GiB
    usage:   15 GiB used, 105 GiB / 120 GiB avail
    pgs:     128 active+clean
```

Read it in order: health → services up/in → PG states → capacity.

| State | Meaning |
|---|---|
| `HEALTH_OK` | No action |
| `HEALTH_WARN` | Degraded but serving; investigate |
| `HEALTH_ERR` | Data availability/durability at risk |

Common PG states:

| State | Meaning |
|---|---|
| `active+clean` | Normal |
| `active+degraded` | Missing a replica but serving |
| `undersized` | Fewer replicas than configured |
| `peering` | OSDs negotiating state |
| `stale` | Primary OSD not reporting |
| `inconsistent` | Scrub found mismatched replicas |

### RBD

```bash
rbd create --size 10G rbd/vm1
rbd info rbd/vm1
rbd map rbd/vm1
```

```text
rbd image 'vm1':
	size 10 GiB in 2560 objects
	order 22 (4 MiB objects)
	block_name_prefix: rbd_data....
	format: 2
	features: layering, exclusive-lock, object-map, fast-diff, deep-flatten
```

`rbd map` creates `/dev/rbd0` on the client. It requires the Ceph client and a keyring. RBD is often used as a Kubernetes CSI volume or an OpenStack Cinder backend.

### CephFS

CephFS needs at least one MDS. Clients mount with the kernel driver or `ceph-fuse`:

```bash
sudo mount -t ceph 10.0.0.10:/ /mnt/cephfs -o name=admin,secret=<key>
```

CephFS is POSIX-compliant and good for shared files, but metadata-heavy workloads stress the MDS. It is not a drop-in replacement for a local filesystem at high IOPS.

### Failure modes

- `HEALTH_WARN` with `1 osds down`: replication is degraded; find the OSD/host.
- `near full` / `full`: stop writes, add capacity, or delete data. A full Ceph cluster can become unavailable.
- `mon quorum lost`: the cluster cannot accept map changes; restore MON connectivity, do not delete mon data.
- Slow recovery: recovery I/O competes with client I/O; throttle with `osd_recovery_max_active`/`osd_recovery_sleep`.
- Clock skew: MONs and OSDs need accurate time; use chrony.

---

## 5. NVMe-oF

NVMe over Fabrics extends the NVMe command set over a network:

| Transport | Typical use |
|---|---|
| NVMe/TCP | Standard Ethernet, easiest to deploy |
| NVMe/RDMA | Low latency, lossless RoCE/InfiniBand |
| NVMe/FC | Fibre Channel SAN |

The client sees a real NVMe namespace (`/dev/nvme0n1`) with multiple queues, not a SCSI LUN. That means lower latency and higher IOPS, but the network must be lossless for RDMA and well-tuned for TCP.

### Target (example with `nvmetcli` or configfs)

```bash
sudo modprobe nvmet
sudo mkdir /sys/kernel/config/nvmet/subsystems/nqn.2026-10.lab:nvme1
echo 1 | sudo tee /sys/kernel/config/nvmet/subsystems/nqn.2026-10.lab:nvme1/attr_allow_any_host
sudo mkdir /sys/kernel/config/nvmet/subsystems/nqn.2026-10.lab:nvme1/namespaces/1
echo -n /dev/vdb | sudo tee /sys/kernel/config/nvmet/subsystems/nqn.2026-10.lab:nvme1/namespaces/1/device_path
echo 1 | sudo tee /sys/kernel/config/nvmet/subsystems/nqn.2026-10.lab:nvme1/namespaces/1/enable
```

Ports and subsystems are configured via configfs; `nvmetcli` saves/restores the config. In production, use a supported target (SPDK, Ceph NVMe-oF gateway, vendor array).

### Client

```bash
sudo nvme discover -t tcp -a 10.0.0.10 -s 4420
sudo nvme connect -t tcp -n nqn.2026-10.lab:nvme1 -a 10.0.0.10 -s 4420
nvme list
```

```text
Node                  Generic               SN                   Model
--------------------- --------------------- -------------------- -----
/dev/nvme0n1          /dev/ng0n1            ...                  Linux
```

```bash
nvme list-subsys
```

### Performance notes

- One NVMe queue pair per CPU is common; check `nvme show-regs`.
- TCP adds latency; RDMA requires PFC/ECN and a lossless fabric.
- Multipath (`nvme connect` to multiple portals) needs `nvme-cli` native multipath or dm-multipath.
- NVMe namespace is block storage; format/mount like a disk.

---

## 6. Comparison

| | NFS | iSCSI | Ceph RBD | NVMe-oF |
|---|---|---|---|---|
| Type | File | Block | Block | Block |
| Client sees | Directory | `/dev/sdX` | `/dev/rbdX` | `/dev/nvmeXnY` |
| Protocol | NFS/TCP | iSCSI/TCP | RADOS | NVMe/TCP/RDMA |
| Latency | Medium | Medium | Medium | Lowest |
| Scale | Server-limited | Target-limited | Distributed | Fabric-limited |
| Snapshot | Server-side | Target/LVM | RBD snapshot | Array/target |
| Best for | Shared files | VM disks | Cloud volumes | AI/HPC, databases |

---

## 7. Command field manual

### `showmount` / `exportfs`

```bash
showmount -e 10.0.0.10
sudo exportfs -v
```

`showmount -e` lists exports; `exportfs -v` shows the server's active options. If `showmount` hangs, `rpcbind` or the firewall is blocking.

### `mount` / `findmnt`

```bash
sudo mount -t nfs 10.0.0.10:/srv/nfs/data /mnt/nfs
findmnt -T /mnt/nfs
```

`findmnt -T` finds the mount containing a path, not just the exact mountpoint.

### `nfsstat` / `nfsiostat`

```bash
nfsstat -c
nfsiostat 1
```

Look for retransmits, auth errors, and high RTT. A retransmit rate above ~1% usually means packet loss or server overload.

### `iscsiadm`

```bash
sudo iscsiadm -m discovery -t sendtargets -p 10.0.0.10
sudo iscsiadm -m node --login
sudo iscsiadm -m session
sudo iscsiadm -m node -T <target> -p <portal> --logout
```

`--logout` disconnects; the LUN disappears. Unmount the filesystem first.

### `multipath`

```bash
sudo multipath -ll
sudo multipath -f mpatha
```

`-f` flushes a map; only after stopping I/O. `-ll` shows path state (`active ready`, `failed faulty`).

### `ceph`

```bash
ceph status
ceph osd tree
ceph df
ceph pg stat
rbd ls
rbd info rbd/vm1
```

`ceph df` shows raw vs usable capacity; replication reduces usable capacity (3 replicas = 1/3 usable, roughly).

### `nvme`

```bash
nvme list
nvme list-subsys
nvme discover -t tcp -a 10.0.0.10 -s 4420
nvme connect -t tcp -n <nqn> -a 10.0.0.10 -s 4420
nvme disconnect -n <nqn>
```

`disconnect` removes the namespace; unmount first.

---

## 8. Failure scenarios

### A — NFS mount hangs on boot

`/etc/fstab` has an NFS mount without `_netdev`/`nofail`; the server is unreachable. Boot waits, then drops to emergency mode. Fix from console: comment the mount, boot, add `_netdev,nofail`, and use `x-systemd.automount` for non-critical shares.

### B — iSCSI LUN is not visible after reboot

```bash
systemctl status iscsid
sudo iscsiadm -m session
sudo iscsiadm -m node -o show | grep startup
```

If `node.startup = manual`, set it to `automatic`:

```bash
sudo iscsiadm -m node -T <target> -p <portal> -o update -n node.startup -v automatic
```

### C — Ceph is near full

```bash
ceph df
ceph osd df
```

`ceph df` shows `%USED` and `MAX AVAIL`. If a pool is near full, stop writes, delete/expire data, or add OSDs. Do not let a Ceph cluster reach 100%; recovery and metadata writes need headroom.

### D — NVMe-oF connection drops

```bash
dmesg -T | grep -i nvme
nvme list-subsys
```

Check the network (lossless config for RDMA, MTU, PFC), the target logs, and whether the initiator can reach the portal. Reconnect after fixing; do not script a blind reconnect loop.

### E — NFS permissions look wrong

```bash
id
getent passwd <user>
showmount -e <server>
sudo exportfs -v
```

UID/GID mismatch or `root_squash` is the usual cause. Fix identity mapping; do not `chmod 777`.

---

## 9. Common mistakes

| Mistake | Why it hurts | Fix |
|---|---|---|
| Two clients format/write one LUN | Filesystem corruption | Cluster FS or one client |
| NFS without `_netdev,nofail` | Boot hang | Add options/automount |
| `soft` mount for a database | Silent I/O errors | `hard` + monitoring |
| Disabling `root_squash` casually | Client root owns server files | UID mapping/Kerberos |
| iSCSI without multipath | Duplicate devices/corruption | `multipath-tools` |
| Forgetting `iscsid` at boot | LUN missing after reboot | Enable service, `node.startup=automatic` |
| Ceph `min_size=1` | Data loss on a second failure | `min_size=2` for 3 replicas |
| Filling Ceph to 100% | Cluster unavailable | Capacity alerts/headroom |
| NVMe-oF over a lossy network | Retries/latency | Lossless fabric for RDMA |
| Treating object storage as POSIX | Apps break on rename/lock | Use file/block for POSIX |

---

## 10. Checkpoint

1. What does the client see for NFS, iSCSI, and object storage?
2. What is the difference between `root_squash` and `no_root_squash`?
3. Why is `hard` the default NFS mount option?
4. What is an IQN, and what is it used for?
5. What does `multipath -ll` show, and why is it needed?
6. What are MON, OSD, MDS, and RGW in Ceph?
7. What is a PG, and what does `active+degraded` mean?
8. Why does `size=3` not guarantee durability if all replicas are in one rack?
9. What does NVMe-oF change compared with iSCSI?
10. Why must a block LUN not be mounted by two ordinary clients?
11. What is the first command you run when an NFS mount hangs?
12. Why is a Ceph cluster at 95% capacity dangerous?

### Answers

1. NFS: a directory; iSCSI: a block device `/dev/sdX`; object: HTTP bucket/key.
2. `root_squash` maps client root to `nobody`; `no_root_squash` trusts client root.
3. It retries indefinitely instead of returning I/O errors, preserving POSIX semantics.
4. iSCSI Qualified Name; identifies initiators/targets for discovery and ACLs.
5. Paths to a LUN and their state; it presents one device and prevents duplicate paths.
6. MON: cluster map/quorum; OSD: data; MDS: CephFS metadata; RGW: S3/Swift gateway.
7. Placement group, a shard of a pool; `active+degraded` means serving with missing replicas.
8. CRUSH failure domain; if all replicas share a rack, a rack failure loses all copies.
9. NVMe command set over a fabric, multiple queues, lower latency, `/dev/nvmeXnY`.
10. A non-cluster filesystem has no coordination; concurrent writes corrupt metadata/data.
11. Check the server/network (`ping`, `showmount`, `exportfs`), not `rm -rf`; look for `D`-state processes.
12. Recovery/metadata writes need space; a full cluster can stop accepting writes and become unavailable.

If you miss more than three, re-read sections 2–5 and redo Lab L3.3.
