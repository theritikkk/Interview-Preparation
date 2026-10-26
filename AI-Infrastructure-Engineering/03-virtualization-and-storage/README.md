# Phase 3 — Virtualization and Storage

> **Why this phase matters:** Kubernetes and OpenStack are built on the same primitives: a kernel isolating processes, a hypervisor isolating machines, and a storage layer pretending to be a disk. If you cannot explain a vCPU, a qcow2 overlay, or an NFS mount, you will memorise cloud symptoms without understanding them.

![OpenStack VM flow](../images/openstack-vm-flow.svg)

## Outcomes

By the end of this phase you can:

1. Explain type-1 vs type-2 hypervisors, and what KVM, QEMU, and libvirt each do.
2. Create, inspect, snapshot, and resize a VM with `virsh`; read its XML and qemu process.
3. Explain virtio, tap devices, bridges, and SR-IOV in the VM data path.
4. Explain CPU virtualization (VT-x/AMD-V, EPT/NPT), NUMA, memory overcommit, ballooning, and hugepages.
5. Read `lsblk`, `blkid`, `findmnt`, `df`, `iostat`, and `fio` output and name the bottleneck.
6. Build and extend LVM; recover a broken `/etc/fstab`; explain inode vs block exhaustion.
7. Distinguish block, file, and object storage; mount NFS; attach an iSCSI LUN.
8. Explain Ceph's MON/OSD/CRUSH/pool model and NVMe-oF queues at a working level.
9. Size storage for IOPS, throughput, and latency instead of "GB".

## Prerequisites

- Phase 1 Linux (processes, memory, disks, systemd).
- Phase 2 networking (bridges, routing, NAT).
- A VM with nested virtualization enabled, or a bare-metal Linux host. On Apple Silicon, KVM is not available; use an x86 cloud VM or a Linux host for L3.1–L3.2.

## Environment

| Lab | Needs | Cost/safety |
|---|---|---|
| L3.1 KVM | x86 Linux with VT-x/AMD-V, 8 GB RAM, 40 GB disk | Free; snapshots |
| L3.2 storage | Same VM, 2 extra virtual disks | Destructive to test disks only |
| L3.3 NFS/iSCSI | Two VMs (server + client) | Free; isolated network |
| L3.4 Ceph/NVMe-oF | 3 small VMs or containers, 8–16 GB RAM | Optional; can be conceptual if resources are tight |

> **Do not run `mkfs`, `pvremove`, `vgremove`, or `dd` against a device you have not identified.** Use `lsblk -o NAME,SIZE,TYPE,MOUNTPOINT,MODEL,SERIAL` and confirm the device is a lab disk every time.

## Study order

| Order | Note | Key question |
|---|---|---|
| 1 | [Virtualization: KVM, QEMU, libvirt](01-virtualization-kvm-qemu-libvirt.md) | What is actually running my VM? |
| 2 | [Linux storage: LVM and filesystems](02-linux-storage-lvm-filesystems.md) | Where did my space/inodes/IOPS go? |
| 3 | [Shared storage: NFS, iSCSI, Ceph, NVMe-oF](03-shared-storage-nfs-iscsi-ceph-nvmeof.md) | Is this block, file, or object, and what does the client see? |
| 4 | [Labs part 1](04-labs-part1.md) | Can I build and inspect a VM? |
| 5 | [Labs part 2](05-labs-part2.md) | Can I attach, mount, and measure storage? |
| 6 | [Checkpoint](06-checkpoint.md) | Can I explain the data path without notes? |

## The layer model

```text
application        database / model training
filesystem         ext4, xfs, NFS client, overlayfs
block layer        LVM, dm, multipath, iSCSI initiator, NVMe
transport          PCIe, NVMe-oF/TCP, iSCSI/TCP, NFS/TCP, Ceph network
device             SSD/NVMe, HDD, RAID controller, virtio-blk
hypervisor         KVM + QEMU + virtio
physical           CPU, RAM, NIC, disk
```

Every performance problem lives at one of these layers. `iostat` tells you the block layer is slow; it does not tell you whether the cause is the SSD, the RAID controller, the hypervisor, the network, or the filesystem. You need the layer-specific tool.

## The three storage types

| Type | Unit | Access | Examples | Best for |
|---|---|---|---|---|
| Block | LUN/volume | Read/write fixed-size blocks | EBS, iSCSI, Ceph RBD, NVMe | Databases, VM disks |
| File | File/directory | POSIX open/read/write | NFS, SMB, CephFS | Shared config, home dirs, many readers |
| Object | Object + metadata | HTTP PUT/GET | S3, Swift, Ceph RGW | Backups, datasets, model artifacts |

A database on NFS is usually a bad idea (locking, latency, semantics). A model checkpoint on block storage is possible but object storage is usually cheaper and easier to share. Match the access pattern to the type.

## Completion criterion

You can:

- Draw the path from a guest `write()` to a qcow2 file on the host and name each layer.
- Explain why a VM has slow disk I/O when the host `iostat` shows 100% util.
- Extend an LV online, grow the filesystem, and verify with `df`.
- Recover a VM that drops to emergency mode after an `/etc/fstab` typo.
- Mount an NFS export and attach an iSCSI LUN; explain the difference in what the client sees.
- Read `ceph status` and say whether the cluster is healthy, degraded, or recovering.
- Answer 30 mixed virtualization/storage questions at ≥ 80%.

Then take [the checkpoint](06-checkpoint.md).
