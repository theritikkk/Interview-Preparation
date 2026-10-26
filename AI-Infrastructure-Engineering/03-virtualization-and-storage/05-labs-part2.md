# Phase 3 Labs — Part 2 (L3.3–L3.4)

> **Topology for L3.3:** two VMs on an isolated virtual network. `storage-server` = `10.0.0.10`, `storage-client` = `10.0.0.11`. If you only have one VM, both roles can run on it using `127.0.0.1`; the commands are the same, but the network failure modes are less realistic.

---

# Lab L3.3 — NFS and iSCSI End to End

## Objective

Export a directory over NFS and a block LUN over iSCSI from one VM, consume both from another, and prove the difference in what the client sees. Break permissions, ACLs, and the export path; diagnose each from the client and server.

**Skills:** `/etc/exports`, `exportfs`, `mount -t nfs`, UID mapping/`root_squash`, `targetcli`, `iscsiadm`, LUN formatting, `nfsiostat`, session state.

## Prerequisites

- Two Ubuntu VMs (or one with loopback), root/sudo.
- Packages: `nfs-kernel-server`, `nfs-common`, `targetcli-fb`, `open-iscsi`, `lvm2`, `e2fsprogs`.
- An isolated network; do not expose iSCSI or NFS to the public internet.

## Setup

On **both**:

```bash
sudo apt update
sudo apt install -y nfs-kernel-server nfs-common targetcli-fb open-iscsi lvm2 e2fsprogs
```

On the **server** (`10.0.0.10`):

```bash
sudo mkdir -p /srv/nfs/data /srv/iscsi
sudo chown nobody:nogroup /srv/nfs/data
sudo truncate -s 5G /srv/iscsi/disk1.img
```

## Part A — NFS

### 1. Export

```bash
echo '/srv/nfs/data 10.0.0.0/24(rw,sync,no_subtree_check)' | sudo tee /etc/exports
sudo exportfs -rav
sudo exportfs -v
```

Expected:

```text
exporting 10.0.0.0/24:/srv/nfs/data
/srv/nfs/data  	10.0.0.0/24(sync,wdelay,hide,no_subtree_check,sec=sys,rw,secure,root_squash,no_all_squash)
```

Check the service:

```bash
sudo systemctl status nfs-server --no-pager
sudo ss -lntp | grep 2049
```

### 2. Mount from the client

```bash
sudo mkdir -p /mnt/nfs
sudo mount -t nfs 10.0.0.10:/srv/nfs/data /mnt/nfs
findmnt /mnt/nfs
```

Expected: `nfs4`, `rw`, `vers=4.2`, `rsize/wsize` around 1 MB.

Write as the client user:

```bash
echo hello > /mnt/nfs/from-client.txt
ls -l /mnt/nfs/from-client.txt
```

### 3. Prove `root_squash`

On the client:

```bash
sudo chown root:root /mnt/nfs/from-client.txt
ls -l /mnt/nfs/from-client.txt
```

Expected: the owner may remain your user or become `nobody`, depending on the mapping. Root on the client cannot create a root-owned file because the server maps UID 0 to `nobody`.

On the server:

```bash
ls -ln /srv/nfs/data
```

`-n` shows numeric UIDs/GIDs, which is the truth NFS uses. If the client and server have different UIDs for the same username, files appear owned by the wrong person.

### 4. Measure

```bash
sudo apt install -y nfs-common sysstat
nfsiostat 1 3
nfsstat -c
```

Look for retransmits and high RTT. A retransmit rate above ~1% suggests loss or server overload.

### 5. Persist

On the client, add to `/etc/fstab`:

```fstab
10.0.0.10:/srv/nfs/data  /mnt/nfs  nfs4  defaults,_netdev,nofail  0  0
```

```bash
sudo findmnt --verify
sudo mount -a
```

**Mistake:** omitting `_netdev` and `nofail`. If the server is unavailable at boot, the client can hang or drop to emergency mode.

## Part B — iSCSI

### 1. Create the target

On the server:

```bash
sudo targetcli
```

At the `/>` prompt:

```text
/> backstores/fileio create name=disk1 file_or_dev=/srv/iscsi/disk1.img size=5G
/> iscsi/ create iqn.2026-10.lab:storage-server
/> iscsi/iqn.2026-10.lab:storage-server/tpg1/luns create /backstores/fileio/disk1
/> iscsi/iqn.2026-10.lab:storage-server/tpg1/acls create iqn.2026-10.lab:storage-client
/> iscsi/iqn.2026-10.lab:storage-server/tpg1/portals create 10.0.0.10
/> saveconfig
/> exit
```

Verify:

```bash
sudo targetcli ls
sudo ss -lntp | grep 3260
```

Expected: a portal on `10.0.0.10:3260`, one LUN, one ACL.

### 2. Discover and log in from the client

```bash
sudo systemctl enable --now iscsid
sudo iscsiadm -m discovery -t sendtargets -p 10.0.0.10
```

Expected: `10.0.0.10:3260,1 iqn.2026-10.lab:storage-server`.

```bash
sudo iscsiadm -m node -T iqn.2026-10.lab:storage-server -p 10.0.0.10 --login
sudo iscsiadm -m session
```

Expected: a TCP session. Check the new block device:

```bash
lsblk -o NAME,SIZE,TYPE,FSTYPE,MOUNTPOINT
```

Expected: a new `sdb` (or `sdc`) of 5G, no filesystem.

### 3. Format and mount

```bash
sudo mkfs.ext4 -L iscsidata /dev/sdb
sudo mkdir -p /mnt/iscsi
sudo mount /dev/sdb /mnt/iscsi
findmnt /mnt/iscsi
echo 'block device' | sudo tee /mnt/iscsi/hello.txt
```

The client sees a **block device**, not a directory. It owns the filesystem; the server only stores blocks. Do not mount the same LUN on two ordinary clients.

### 4. Persist the session

```bash
sudo iscsiadm -m node -T iqn.2026-10.lab:storage-server -p 10.0.0.10 \
  -o update -n node.startup -v automatic
```

Add the filesystem to fstab by UUID:

```bash
UUID=$(sudo blkid -s UUID -o value /dev/sdb)
echo "UUID=$UUID  /mnt/iscsi  ext4  defaults,_netdev,nofail  0  2" | sudo tee -a /etc/fstab
sudo findmnt --verify
```

Ensure `iscsid` and `open-iscsi` are enabled so the device exists before mount:

```bash
sudo systemctl enable iscsid open-iscsi
```

## Why it works

- NFS exports a directory; the server owns the filesystem and enforces UID/GID mapping.
- iSCSI exports a block LUN; the client formats and mounts it, so the server has no idea what filesystem is on it.
- `root_squash` maps client root to `nobody` to prevent a compromised client from owning server files.
- `iscsid` maintains the session; `node.startup=automatic` reconnects at boot.
- NFS is usually simpler to share; iSCSI gives lower-level control and is common for VM disks.

## Troubleshooting task

### Break A — NFS permission denied

On the server:

```bash
sudo chmod 700 /srv/nfs/data
```

On the client:

```bash
ls /mnt/nfs
```

Expected: `Permission denied` for a non-root user, because the export directory itself is not traversable. Check on the server:

```bash
namei -l /srv/nfs/data
sudo -u nobody ls -ld /srv/nfs/data
```

Fix the directory mode (e.g. `755`) and retest. Do not `chmod 777`; fix the ownership model.

### Break B — stale NFS handle

On the server:

```bash
sudo mv /srv/nfs/data /srv/nfs/data-old
sudo mkdir /srv/nfs/data
sudo chown nobody:nogroup /srv/nfs/data
sudo exportfs -ra
```

On the client:

```bash
ls /mnt/nfs
```

Expected: `Stale file handle`. The export path now points to a new inode; the client's cached file handles are invalid. Fix by unmounting/remounting:

```bash
sudo umount /mnt/nfs
sudo mount -t nfs 10.0.0.10:/srv/nfs/data /mnt/nfs
```

On the server, move the old data back or copy it into the new directory. Stale handles are not a client bug.

### Break C — iSCSI ACL mismatch

On the server, remove the client ACL:

```bash
sudo targetcli iscsi/iqn.2026-10.lab:storage-server/tpg1/acls delete iqn.2026-10.lab:storage-client
```

On the client:

```bash
sudo iscsiadm -m node -T iqn.2026-10.lab:storage-server -p 10.0.0.10 --logout
sudo iscsiadm -m node -T iqn.2026-10.lab:storage-server -p 10.0.0.10 --login
```

Expected: login fails with `authorization failure` or the LUN is not visible. Check the target's ACL list and the client's initiator name:

```bash
cat /etc/iscsi/initiatorname.iscsi
sudo targetcli ls /iscsi/iqn.2026-10.lab:storage-server/tpg1/acls
```

Recreate the ACL with the exact client IQN.

### Break D — NFS server down

On the server:

```bash
sudo systemctl stop nfs-server
```

On the client, access the mount. With `hard` (default), the process may hang in `D` state. Check:

```bash
ps -eo stat,pid,comm | grep ' D '
mount | grep nfs
```

Restart the server; the client should recover. Do not run `rm -rf` on a hung mount. If you must detach, `umount -f` or `umount -l` after understanding the risk.

## Verification

```bash
# NFS
findmnt /mnt/nfs
cat /mnt/nfs/from-client.txt

# iSCSI
sudo iscsiadm -m session
lsblk /dev/sdb
findmnt /mnt/iscsi
cat /mnt/iscsi/hello.txt

# No duplicate LUNs
sudo multipath -ll 2>/dev/null || true
```

Expected: NFS file visible; iSCSI session present; `/dev/sdb` mounted; one path (multipath is not configured in this lab).

## Cleanup

On the client:

```bash
sudo umount /mnt/nfs /mnt/iscsi 2>/dev/null
sudo sed -i '\#/mnt/nfs#d; \#/mnt/iscsi#d' /etc/fstab
sudo iscsiadm -m node -T iqn.2026-10.lab:storage-server -p 10.0.0.10 --logout
sudo iscsiadm -m node -T iqn.2026-10.lab:storage-server -p 10.0.0.10 -o delete
```

On the server:

```bash
sudo targetcli clearconfig confirm=True
sudo rm -f /srv/iscsi/disk1.img
sudo sed -i '\#/srv/nfs/data#d' /etc/exports
sudo exportfs -ra
sudo rm -rf /srv/nfs/data /srv/nfs/data-old
```

## Write-up template

```markdown
## L3.3 — NFS and iSCSI

**NFS export:** path, options, client mount options
**Client view:** directory vs block device
**root_squash evidence:** numeric UID on server
**iSCSI:** IQN, portal, LUN, session
**Break A:** permission denied -> root cause
**Break B:** stale handle -> fix
**Break C:** ACL mismatch -> evidence
**NFS vs iSCSI:** one sentence on when to use each
```

---

# Lab L3.4 — Ceph and NVMe-oF (Scoped)

## Objective

Either run a single-node MicroCeph cluster and create an RBD image, or analyse a provided Ceph cluster state and design a pool/CRUSH layout. Then observe an NVMe-oF target/initiator connection if your kernel supports it; otherwise analyse `nvme` output and answer the queue/fabric questions.

This lab is **optional and resource-heavy**. It is deliberately scoped: you are learning the data path and the failure vocabulary, not becoming a Ceph operator in one afternoon.

## Prerequisites

- Ubuntu 22.04/24.04 VM with 4 vCPU, 8 GB RAM, and a spare 10 GB disk (`/dev/vdb`) that can be wiped.
- Root/sudo.
- For NVMe-oF: kernel modules `nvmet`, `nvmet-tcp`, `nvme-tcp`; `nvme-cli`.

> **Destructive:** MicroCeph will wipe the disk you give it. Confirm with `lsblk` and use only the spare disk. Do not run this on a machine with data.

## Option A — MicroCeph single node

```bash
sudo snap install microceph
sudo microceph cluster bootstrap
sudo microceph status
```

Expected: one MON and one MGR, `HEALTH_OK` or `HEALTH_WARN` with a single node. Add the spare disk:

```bash
sudo microceph disk add /dev/vdb --wipe
sudo microceph.ceph status
```

Expected: one OSD `up` and `in`. With a single OSD, replication must be 1 for data to be available:

```bash
sudo microceph.ceph osd pool create rbd 32 32
sudo microceph.ceph osd pool set rbd size 1
sudo microceph.ceph osd pool set rbd min_size 1
rbd pool init rbd
```

Create and map an image:

```bash
sudo rbd create --size 1G rbd/demo
sudo rbd info rbd/demo
sudo rbd map rbd/demo
lsblk
```

Expected: `/dev/rbd0` of 1 GB. Format and mount:

```bash
sudo mkfs.ext4 /dev/rbd0
sudo mkdir -p /mnt/rbd
sudo mount /dev/rbd0 /mnt/rbd
echo 'hello ceph' | sudo tee /mnt/rbd/hello.txt
```

Read the cluster:

```bash
sudo microceph.ceph status
sudo microceph.ceph osd tree
sudo microceph.ceph df
sudo microceph.ceph pg stat
```

Interpret:

- `health: HEALTH_OK` vs `HEALTH_WARN`.
- `osd: 1 osds: 1 up, 1 in` — one OSD, no redundancy.
- `pools: 1 pools, 32 pgs` — small lab PG count.
- `pgs: 32 active+clean` — normal.

Break it: stop the OSD or remove the disk (lab only), then read the health:

```bash
sudo microceph.ceph osd tree
sudo microceph.ceph status
```

With one OSD and `size=1`, losing the OSD means data is unavailable. This is the concrete lesson: replication is not optional for durability.

### Cleanup

```bash
sudo umount /mnt/rbd 2>/dev/null
sudo rbd unmap /dev/rbd0 2>/dev/null
sudo snap remove microceph --purge
```

`--purge` destroys the cluster and its data. Confirm you are on the lab VM.

## Option B — Ceph state analysis (no install)

If you cannot run MicroCeph, use this output and answer the questions:

```text
  cluster:
    id:     7f3c...
    health: HEALTH_WARN
           1 osds down
           1 host (1 osds) down
           Degraded data redundancy: 128/384 objects degraded (33.333%), 32 pgs degraded

  services:
    mon: 3 daemons, quorum a,b,c (age 2h)
    mgr: x(active, since 2h), standbys: y
    osd: 6 osds: 5 up, 6 in

  data:
    pools:   2 pools, 128 pgs
    objects: 384 objects, 1.5 GiB
    usage:   4.5 GiB used, 55 GiB / 60 GiB avail
    pgs:     128 active+clean
             32  active+degraded
```

Questions:

1. How many replicas are configured if one OSD down causes 33% degradation?
2. Is the cluster serving reads/writes?
3. What is the first thing you check on the down host?
4. What is the risk if another OSD fails before recovery?
5. Why is `1 host down` more serious than `1 osd down`?

### Answers

1. Three replicas: losing one of three copies degrades 1/3 of objects.
2. Yes, `active+degraded` serves I/O; `active+undersized` may still serve; `incomplete`/`stale` may not.
3. Host power/network, OSD process, disk health, and `ceph osd tree`/logs.
4. If the cluster drops below `min_size` (2), I/O may block for affected PGs.
5. All OSDs on that host are down, so the failure domain lost multiple replicas at once; CRUSH placement matters.

## Part C — NVMe-oF loopback (optional)

Check modules:

```bash
sudo modprobe nvmet
sudo modprobe nvmet-tcp
sudo modprobe nvme-tcp
lsmod | grep -E 'nvmet|nvme_tcp'
```

If modules are unavailable, skip to the analysis questions.

Create a backing file or use a spare LUN:

```bash
sudo truncate -s 2G /srv/nvme.img
sudo mkdir -p /sys/kernel/config/nvmet/subsystems/nqn.2026-10.lab:nvme1
echo 1 | sudo tee /sys/kernel/config/nvmet/subsystems/nqn.2026-10.lab:nvme1/attr_allow_any_host
sudo mkdir -p /sys/kernel/config/nvmet/subsystems/nqn.2026-10.lab:nvme1/namespaces/1
echo -n /srv/nvme.img | sudo tee /sys/kernel/config/nvmet/subsystems/nqn.2026-10.lab:nvme1/namespaces/1/device_path
echo 1 | sudo tee /sys/kernel/config/nvmet/subsystems/nqn.2026-10.lab:nvme1/namespaces/1/enable

sudo mkdir -p /sys/kernel/config/nvmet/ports/1
echo tcp | sudo tee /sys/kernel/config/nvmet/ports/1/addr_trtype
echo 127.0.0.1 | sudo tee /sys/kernel/config/nvmet/ports/1/addr_traddr
echo 4420 | sudo tee /sys/kernel/config/nvmet/ports/1/addr_trsvcid
echo ipv4 | sudo tee /sys/kernel/config/nvmet/ports/1/addr_adrfam
sudo ln -s /sys/kernel/config/nvmet/subsystems/nqn.2026-10.lab:nvme1 \
  /sys/kernel/config/nvmet/ports/1/subsystems/nqn.2026-10.lab:nvme1
```

Connect:

```bash
sudo nvme discover -t tcp -a 127.0.0.1 -s 4420
sudo nvme connect -t tcp -n nqn.2026-10.lab:nvme1 -a 127.0.0.1 -s 4420
nvme list
nvme list-subsys
```

Expected: a new `/dev/nvme0n1` (or `nvme1n1`). Format/mount it like a block device.

Analyse the queues:

```bash
sudo nvme show-regs /dev/nvme0 | head -30
cat /sys/block/nvme0n1/queue/nr_requests
```

NVMe uses multiple queue pairs; the exact queue count depends on CPU count and driver. Compare with iSCSI, which uses a TCP session and SCSI queue.

### Cleanup

```bash
sudo umount /mnt/nvme 2>/dev/null
sudo nvme disconnect -n nqn.2026-10.lab:nvme1
sudo rm /sys/kernel/config/nvmet/ports/1/subsystems/nqn.2026-10.lab:nvme1
sudo rmdir /sys/kernel/config/nvmet/ports/1
sudo rmdir /sys/kernel/config/nvmet/subsystems/nqn.2026-10.lab:nvme1/namespaces/1
sudo rmdir /sys/kernel/config/nvmet/subsystems/nqn.2026-10.lab:nvme1
rm -f /srv/nvme.img
```

## Why it works

- Ceph distributes objects across OSDs using CRUSH; pools define replication and PG counts.
- A single-node cluster with `size=1` has no redundancy; losing the OSD loses the data.
- RBD presents a block device backed by RADOS objects.
- NVMe-oF carries NVMe commands over TCP/RDMA; the initiator sees a native NVMe namespace with multiple queues.
- iSCSI uses SCSI over TCP; NVMe-oF has lower overhead and deeper parallelism.

## Troubleshooting task

1. `ceph status` says `HEALTH_WARN`, `1 osds down`. What do you check first?
2. `ceph df` shows 90% used. Why is that dangerous even if `MAX AVAIL` looks nonzero?
3. `rbd map` fails with `permission denied`. What do you check?
4. `nvme connect` hangs or returns `Connection refused`. Name three causes.
5. An NVMe-oF workload has high latency but the target disk is idle. What do you check?
6. A pool has `size=3` but all replicas are in one rack. Why is that a design bug?

### Expected answers

1. Host power/network, OSD process, disk health, `ceph osd tree`, logs.
2. Recovery/metadata writes need headroom; a full cluster can stop writes and become unavailable.
3. Ceph keyring/client auth, pool permissions, and the `rbd` feature/profile.
4. Target not listening, wrong portal/port, firewall, wrong NQN, no route.
5. Network loss/congestion, RDMA PFC/ECN, MTU, queue depth, initiator CPU.
6. A rack failure can take all replicas; CRUSH failure domain should be `rack` (or `host` at minimum).

## Verification

```bash
sudo microceph.ceph status 2>/dev/null || ceph status
sudo rbd ls 2>/dev/null
nvme list 2>/dev/null
```

Expected: `HEALTH_OK`/known warning, `demo` image, NVMe namespace if the optional part ran.

## Write-up template

```markdown
## L3.4 — Ceph / NVMe-oF

**Cluster:** MON/MGR/OSD counts, health
**Pool:** size/min_size, PG count
**RBD:** image, map device, filesystem
**Degraded state:** what changed, what still served
**NVMe-oF:** transport, NQN, namespace, queues
**Failure domain:** why `size=3` is not enough without racks/hosts
**What I would monitor:** ...
```

---

## Lab log

| Lab | Date | Time | Pass? | Weakest idea | Retest |
|---|---|---|---:|---|---|
| L3.3 | | | | | |
| L3.4 | | | | | |

Then take [the Phase 3 checkpoint](06-checkpoint.md).
