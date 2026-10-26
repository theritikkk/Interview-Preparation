# Cinder Storage

> **The one-sentence model:** Cinder is a control plane that asks a scheduler to place a block volume on a backend, then asks an agent/driver to create a LUN or RBD image; Nova connects that volume to the compute host and libvirt presents it to the guest. The volume can be `available` while the attach path is broken, and it can be `in-use` while the guest cannot see it.

---

## 1. The components

| Component | Job | Failure symptom |
|---|---|---|
| `cinder-api` | Accepts REST requests, validates, writes DB | API errors, volume never appears |
| `cinder-scheduler` | Chooses a backend/volume service for the type/AZ/capacity | Volume stuck `creating`, `No valid host` |
| `cinder-volume` | Driver process per backend; creates/deletes/attaches LUNs | Backend errors, `error` state, slow operations |
| `cinder-backup` | Backs up volumes to Swift/S3/Ceph/NFS | Backup stuck/failed, restore unavailable |
| Volume driver | LVM/iSCSI, Ceph RBD, NFS, vendor array | Backend-specific errors, path/initiator issues |
| `nova-compute` | Connects the volume to the host and libvirt | Attach fails at the compute stage |

```bash
openstack volume service list
openstack volume type list
openstack volume list
openstack volume snapshot list
openstack volume backup list
```

Example:

```text
+------------------+----------------+------+---------+-------+----------------------------+
| Binary           | Host           | Zone | Status  | State | Updated At                 |
+------------------+----------------+------+---------+-------+----------------------------+
| cinder-scheduler | controller     | nova | enabled | up    | 2026-10-27T10:00:00.000000 |
| cinder-volume    | compute1@lvm   | nova | enabled | up    | 2026-10-27T10:00:00.000000 |
+------------------+----------------+------+---------+-------+----------------------------+
```

`Host` often includes the backend name (`compute1@lvm`, `controller@ceph`). A volume type maps to a backend through extra specs such as `volume_backend_name`.

---

## 2. Volume types and scheduling

A volume type is a label plus extra specs/capabilities:

```bash
openstack volume type create opslab-fast
openstack volume type set --property volume_backend_name=ceph \
  --property capabilities:storage_protocol=RBD opslab-fast
openstack volume type show opslab-fast -f yaml
```

When you create a volume with `--type opslab-fast`, Cinder's scheduler finds a backend whose capabilities match. If no backend matches, the volume stays `creating` and then goes `error`; the scheduler/volume logs name the filter failure.

```bash
openstack volume create --size 10 --type opslab-fast opslab-vol
openstack volume show opslab-vol -f yaml
```

Example:

```yaml
status: available
size: 10
availability_zone: nova
volume_type: opslab-fast
multiattach: false
encrypted: false
attachments: []
bootable: false
```

### Availability zones

Cinder AZs are independent of Nova AZs in configuration, though they are often aligned by name. A volume can only attach to an instance in the same AZ. Check both:

```bash
openstack availability zone list
openstack availability zone list --volume
openstack volume show opslab-vol -c availability_zone
openstack server show opslab-vm -c OS-EXT-AZ:availability_zone
```

If the volume is in `az-a` and the VM in `az-b`, the attach fails with an AZ mismatch. Create the volume in the VM's AZ (or migrate/rebuild), not with a force flag.

### QoS and encryption

- **QoS:** `openstack volume qos create`, `qos set --property`, `qos associate <qos> <type>`. Limits are enforced by the backend; not every driver honors every key.
- **Encryption:** volume types can specify an encryption provider/cipher/key size; Barbican stores keys. Encryption is not a substitute for backups or access control.
- **Multi-attach:** only on backends that support it; the guest must use a cluster-aware filesystem or coordinated writes. `multiattach: true` is a capability, not a recommendation.

```bash
openstack volume qos create opslab-iops --consumer front-end \
  --property read_iops_sec=5000 --property write_iops_sec=5000
openstack volume qos associate opslab-iops opslab-fast
openstack volume create --size 20 --type opslab-fast --multi-attach opslab-shared
```

---

## 3. The attach path

```text
client: openstack server add volume vm vol
  │
  ▼
cinder-api: validate, reserve volume, write attachment
  │
  ▼
cinder-scheduler / cinder-volume: create/export LUN (iSCSI target, RBD image)
  │
  ▼
nova-compute: log in to iSCSI target / map RBD, build libvirt disk XML
  │
  ▼
libvirt/QEMU: attach virtio-blk/SCSI device
  │
  ▼
guest: /dev/vdb (or /dev/sdb) appears
```

### Commands

```bash
openstack server add volume opslab-vm opslab-vol
openstack volume show opslab-vol -f yaml
openstack server show opslab-vm -c volumes_attached -c addresses
```

The volume transitions `available → attaching → in-use`; the guest sees a new block device. If it stays `attaching`, the compute/backend step is stuck.

Inside the guest:

```bash
lsblk -o NAME,SIZE,TYPE,MOUNTPOINT,SERIAL
sudo blkid /dev/vdb
sudo mkfs.ext4 /dev/vdb
sudo mkdir -p /data
sudo mount /dev/vdb /data
echo "hello" | sudo tee /data/hello.txt
df -h /data
```

Use the device name from `lsblk`, not a guess. On some images the volume appears as `/dev/vdb`, `/dev/sdb`, or a by-id path. Add `/etc/fstab` by UUID only after the filesystem is stable:

```bash
sudo blkid -s UUID -o value /dev/vdb
# /etc/fstab: UUID=<uuid> /data ext4 defaults,nofail 0 2
```

### Detach

```bash
# In the guest:
sudo umount /data

# From the client:
openstack server remove volume opslab-vm opslab-vol
openstack volume show opslab-vol -c status -c attachments
```

Unmount before detaching. A forced detach of a mounted filesystem can corrupt data or leave the guest with a stale device. If the guest still shows the device after detach, rescan the SCSI bus or reboot; do not reuse the volume elsewhere until it is `available`.

### Extend

```bash
openstack volume set --size 20 opslab-vol
# or: cinder extend opslab-vol 20
```

Then in the guest:

```bash
lsblk
sudo growpart /dev/vdb 1     # if the volume has a partition table
sudo resize2fs /dev/vdb1     # ext4
# or: sudo xfs_growfs /data  # XFS
df -h /data
```

Cinder grows the block device; it does not grow the partition or filesystem. `lsblk` showing 20G while `df -h` shows 10G means you skipped the guest step.

---

## 4. Snapshots and backups

| | Snapshot | Backup |
|---|---|---|
| Location | Same backend (usually) | Object store/Swift/S3/NFS/Ceph |
| Purpose | Fast restore/clone | Disaster recovery/retention |
| Consistency | Crash-consistent unless quiesced | Crash-consistent unless quiesced |
| Restore | Create a new volume from snapshot | Restore to a volume |
| Cost | Backend-dependent | Storage + transfer |

```bash
openstack volume snapshot create --volume opslab-vol opslab-snap
openstack volume snapshot list
openstack volume create --snapshot opslab-snap --size 10 opslab-restored
openstack volume backup create --name opslab-backup opslab-vol
openstack volume backup list
openstack volume backup restore <backup-id> <target-volume-id>
```

A snapshot is not a backup if it lives on the same failed backend. For production, combine snapshots with backups to independent storage and test restores.

**Consistency:** quiesce the application or freeze the filesystem before a snapshot if you need an application-consistent image. A crash-consistent snapshot is fine for many stateless/dev workloads, not for a busy database without recovery checks.

**Restore:** `openstack volume create --snapshot` creates a new volume; the original is unchanged. You can attach the restored volume to a VM and verify data before deleting the old one.

---

## 5. Command field manual

### `openstack volume create`

```bash
openstack volume create \
  --size 20 \
  --type opslab-fast \
  --availability-zone nova \
  --description "opslab data" \
  opslab-vol
```

**Why:** explicit size/type/AZ avoids scheduler surprises. `--image` creates a bootable volume from an image; `--snapshot` clones a snapshot; `--source` clones another volume.

**Mistake:** creating in the default AZ when the VM is in another; creating a volume smaller than the filesystem you intend to restore.

### `openstack volume show`

```bash
openstack volume show opslab-vol -f yaml
```

**Interpretation:** `status`, `size`, `availability_zone`, `volume_type`, `attachments`, `multiattach`, `encrypted`, `bootable`, `source_volid`, `snapshot_id`, `image_id`. `attachments` tells you which server/device has it.

### `openstack volume list --long`

```bash
openstack volume list --long
```

**Interpretation:** status, size, type, AZ, attachments, bootable. `available` means not attached; `in-use` means attached; `error` needs logs.

### `openstack volume service list`

```bash
openstack volume service list
```

**Interpretation:** `State` heartbeat and `Status` enabled/disabled. A backend `down` means new volumes for its type may fail; existing volumes may still be accessible.

### `openstack server add volume`

```bash
openstack server add volume opslab-vm opslab-vol
```

**Why:** asks Nova/Cinder to attach the volume to the specified server. If the volume is in the wrong AZ or already attached, the API rejects it.

**Mistake:** using the volume ID as the server ID, or attaching a volume that is still `creating`/`attaching`.

### `openstack volume snapshot create`

```bash
openstack volume snapshot create --volume opslab-vol --description "before upgrade" opslab-snap
```

**Interpretation:** `status: available` means the snapshot is complete; `creating`/`error` need backend logs. A snapshot of an attached volume is crash-consistent unless quiesced.

### `openstack volume backup create`

```bash
openstack volume backup create --name opslab-backup --container v2 opslab-vol
```

**Why:** copies the volume to the backup backend for recovery. Check `openstack volume backup show` for `status` and `restore` tests.

### `cinder get-pools --detail` (operator)

```bash
cinder get-pools --detail
```

**Interpretation:** backend capacity, free space, capabilities, and QoS support. Use it to confirm a backend is full or lacks a capability; `openstack volume service list` only shows the service, not capacity.

---

## 6. Failure scenarios

### A — Volume stuck `creating`

Fault domain: scheduler/backend. Check:

1. `openstack volume show <vol>` — status and `volume_type`.
2. `openstack volume service list` — backend up/enabled?
3. `cinder-scheduler.log` and `cinder-volume.log` — no valid backend, capability mismatch, backend unreachable.
4. `cinder get-pools --detail` — free capacity.
5. Quota — explicit quota error.

Fix: correct the type/AZ, enable the backend, free capacity, or choose a type the backend supports. Do not delete and recreate blindly; the first volume may still be consuming a LUN.

### B — Attach fails or stays `attaching`

Fault domain: AZ, backend export, compute initiator, or Nova. Check:

- volume AZ vs server AZ;
- `openstack volume show` attachments/status;
- `cinder-volume.log` for export/attach errors;
- `nova-compute.log` for iSCSI/RBD/libvirt errors;
- on the compute: `iscsiadm -m session`, `multipath -ll`, `lsblk`, `dmesg`;
- for RBD: `rbd ls <pool>`, `rbd info <pool>/<image>`, Ceph health.

Fix: create the volume in the right AZ, repair the iSCSI session/initiator, or fix the backend. A force detach/attach may leave stale state; clean up the attachment before retrying.

### C — Multi-attach corruption

Two VMs mount the same non-cluster-aware filesystem (ext4/XFS) from one multi-attached volume. Both write, the filesystem corrupts. Multi-attach is only for cluster-aware filesystems (GFS2, OCFS2) or applications that coordinate writes. Use a shared file service (Manila/NFS) instead for ordinary shared data.

### D — Guest does not see the new device

Check `lsblk`, `dmesg`, `iscsiadm -m session`, `multipath -ll`, and the libvirt domain XML (`virsh dumpxml`). The volume can be `in-use` while the guest has not scanned the SCSI bus. Do not format a guessed device; confirm the serial/size.

### E — `df` does not grow after extend

Cinder grew the block device; the partition/filesystem did not. `lsblk` vs `df -h`; run `growpart` and `resize2fs`/`xfs_growfs`. For XFS, grow the mounted filesystem; for ext4, online growth is supported.

### F — Snapshot/backup `error`

Check the backend's snapshot capability, free space, the backup target (Swift/S3/NFS), and `cinder-backup` logs. A snapshot of an attached volume can fail if the backend requires a consistent state; quiesce or detach.

### G — Detach stuck

The guest may still be using the device, the compute may have lost the iSCSI session, or Nova/Cinder state disagrees. Check `openstack volume show`, `nova-compute.log`, `iscsiadm -m session`, and the guest's `lsof`/`mount`. Do not delete the volume while it is attached; clean up the attachment first.

### H — Backend full

Volumes fail to create or go read-only; `cinder get-pools --detail` shows free capacity near zero. Free space, expand the backend, or migrate volumes. Thin-provisioned backends can overcommit and fail later; monitor both allocated and physical usage.

---

## 7. Checkpoint

1. What does Cinder's scheduler choose, and what does the volume driver do?
2. What is a volume type, and how does it influence scheduling?
3. Why must a volume and an instance be in the same AZ to attach?
4. What does `available` vs `in-use` vs `attaching` mean?
5. What is the difference between a snapshot and a backup?
6. Why is a snapshot not a backup by itself?
7. What does `openstack server add volume` do behind the scenes?
8. Why does `df` not grow after `openstack volume set --size`?
9. When is Multi-Attach safe, and when is it dangerous?
10. What is the difference between iSCSI and RBD from the compute's perspective?
11. What does `cinder-volume` do that `cinder-api` does not?
12. What does a volume service `down` mean for existing volumes?
13. What is the first check for a volume stuck `creating`?
14. What is the first check for an attach failure?
15. What is the first check when the guest does not see the disk?
16. Why should you unmount before detaching?
17. What does an encrypted volume type require besides Cinder?
18. How would you prove a backend is full rather than a scheduler bug?
19. What is a bootable volume, and why is it useful?
20. How would you restore a snapshot and verify it before replacing production?

### Answers

1. The scheduler chooses a backend/volume service that matches type/AZ/capacity; the driver creates/exports/deletes the actual LUN/RBD/image.
2. A label with extra specs/capabilities (backend name, protocol, QoS, encryption); the scheduler matches it to a backend.
3. Block volumes are AZ-scoped; the compute host can only connect a volume in its AZ (or a replicated/multi-AZ backend, which is a special design).
4. `available`: created, not attached; `in-use`: attached; `attaching`: attachment in progress. `error` means it failed.
5. A snapshot is a backend-local point-in-time copy for fast clone/restore; a backup is copied to independent storage for recovery/retention.
6. It may live on the same failed backend and does not protect against backend loss or accidental deletion; test restores and keep off-backend backups.
7. Cinder reserves/exports the volume, Nova connects it to the compute (iSCSI/RBD), and libvirt attaches it to the guest as a block device.
8. Cinder grew the block device, but the partition/filesystem was not grown; run `growpart` and `resize2fs`/`xfs_growfs`.
9. Safe only on a capable backend with a cluster-aware filesystem/coordinated application; dangerous with ext4/XFS on multiple writers because it corrupts data.
10. iSCSI exports a LUN over TCP and the compute logs in via `iscsiadm`; RBD maps a Ceph image over the Ceph network using librbd, with no iSCSI session.
11. `cinder-api` accepts/validates requests; `cinder-volume` runs the backend driver and performs the actual create/export/attach/delete operations.
12. New volumes for that backend may fail; existing volumes may remain accessible if the data path is independent, but management operations may not work.
13. Volume status, volume type, backend service state, scheduler/volume logs, pool capacity, quota.
14. AZ match, volume status/attachments, backend export, compute iSCSI/RBD/libvirt logs.
15. `lsblk`, `dmesg`, iSCSI session/multipath, and the libvirt domain XML; confirm the device by serial/size.
16. Detaching a mounted filesystem can corrupt data or leave stale state; unmount and flush first.
17. A key manager (Barbican) and a configured encryption provider/key; the backend must support encryption.
18. `cinder get-pools --detail` shows free/allocated capacity; the scheduler/volume logs may show capacity filter failures.
19. A volume created from an image that can boot a VM; useful for persistent root disks, faster boots, and volume-level snapshots/backups.
20. Create a new volume from the snapshot, attach it to a test VM, verify data/checksums and application consistency, then replace production after a backup.
