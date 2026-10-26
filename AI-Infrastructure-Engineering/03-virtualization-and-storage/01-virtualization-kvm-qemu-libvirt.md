# Virtualization: KVM, QEMU, and libvirt

> **The one-sentence model:** KVM turns the Linux kernel into a hypervisor, QEMU emulates the hardware and devices, and libvirt is the management layer that writes XML and launches the QEMU process. A "VM" is a normal Linux process with special CPU and memory mappings.

---

## 1. Why virtualize

| Benefit | Mechanism | Trade-off |
|---|---|---|
| Consolidation | Many guests on one host | Noisy neighbours, overcommit |
| Isolation | Hardware-assisted CPU/memory boundaries | Shared kernel? No — each VM has its own kernel |
| Snapshots | Copy-on-write disk images | Chain complexity, I/O overhead |
| Live migration | Copy memory/state to another host | Shared storage/network, CPU compatibility |
| Templates | Clone a golden image | Image drift, sysprep/cloud-init |

Containers isolate processes inside one kernel; VMs isolate entire kernels. That is why a container escape is a host kernel problem, while a VM escape requires a hypervisor bug.

### Type 1 vs type 2

| | Type 1 (bare metal) | Type 2 (hosted) |
|---|---|---|
| Runs on | Hardware directly | An existing OS |
| Examples | KVM (with Linux), Xen, ESXi, Hyper-V | VirtualBox, VMware Fusion, QEMU without KVM |
| Performance | Near-native with virtio | Higher overhead |
| Typical use | Cloud, datacenter | Developer laptop |

KVM is unusual: it is a Linux kernel module, so the host OS *is* the hypervisor. It is often called type 1 even though Linux is running, because the kernel provides the hypervisor functionality directly.

---

## 2. The three pieces

```text
libvirtd / virt-install / virsh
        │  writes XML, starts process
        ▼
QEMU (device model, I/O, migration)
        │  ioctl /dev/kvm
        ▼
KVM (kernel module: VT-x/AMD-V, EPT/NPT, vCPU exits)
        │
        ▼
Host Linux kernel (scheduler, memory, network, block)
```

| Component | Responsibility | What you see |
|---|---|---|
| KVM | CPU/memory virtualization, VM exits | `/dev/kvm`, `kvm_intel`/`kvm_amd` modules |
| QEMU | Emulated chipset, devices, disk/network backends | `qemu-system-x86_64` process |
| libvirt | API, XML, lifecycle, snapshots, networks, pools | `virsh`, `virt-install`, `libvirtd` |
| guest kernel | The OS inside the VM | Console/SSH into the guest |

### Check the host

```bash
egrep -c '(vmx|svm)' /proc/cpuinfo
lsmod | grep kvm
ls -l /dev/kvm
```

```text
16
kvm_intel             393216  0
kvm                  1146880  1 kvm_intel
crw-rw---- 1 root kvm 10, 232 Oct 26 22:40 /dev/kvm
```

- `16` means 16 logical CPUs report virtualization extensions.
- Both `kvm` and `kvm_intel`/`kvm_amd` must be loaded.
- `/dev/kvm` must exist and be accessible to the user running QEMU (usually via the `kvm` group).

If `/dev/kvm` is missing, the host BIOS/UEFI may have virtualization disabled, or the VM you are inside does not expose nested virtualization.

```bash
sudo virt-host-validate
```

```text
 QEMU: Checking for hardware virtualization                  : PASS
 QEMU: Checking if device /dev/kvm exists                    : PASS
 QEMU: Checking if device /dev/kvm is accessible             : PASS
 QEMU: Checking for cgroup 'cpu' controller support          : PASS
```

### The QEMU process

```bash
ps -eLf | grep qemu | head
```

```text
root 1421 1 1421 0 24 Oct26 ? 00:00:12 qemu-system-x86_64 -name guest=vm1 ...
root 1421 1 1426 0 24 Oct26 ? 00:00:08 qemu-system-x86_64 -name guest=vm1 ...   # vCPU 0
root 1421 1 1427 0 24 Oct26 ? 00:00:07 qemu-system-x86_64 -name guest=vm1 ...   # vCPU 1
```

Each vCPU is a host thread. The QEMU main thread handles I/O and the monitor. This is why `top -H` on the host shows vCPU threads, and why CPU pinning is possible.

---

## 3. CPU virtualization

Without hardware support, the hypervisor would trap and emulate privileged instructions (slow). VT-x (Intel) and AMD-V add a guest mode:

- Guest code runs directly on the CPU until it executes a privileged instruction or triggers an intercept.
- The CPU saves guest state in a **VMCS** (Intel) or **VMCB** (AMD) and exits to KVM.
- KVM handles the exit and resumes the guest.

### EPT/NPT

Shadow page tables were the old way to virtualize memory (expensive). **EPT** (Intel) / **NPT** (AMD) let the CPU translate guest-virtual → guest-physical → host-physical in hardware. This is why modern VMs are fast.

### CPU models and features

```bash
virsh capabilities | grep -A3 '<model>'
virsh dumpxml vm1 | grep -A5 '<cpu'
```

```xml
<cpu mode='host-model' check='partial'>
  <model fallback='allow'>Skylake-Client-IBRS</model>
  <feature policy='require' name='vmx'/>
</cpu>
```

| Mode | Behaviour | Use |
|---|---|---|
| `host-passthrough` | Guest sees the exact host CPU | Best performance; migration to a different CPU may fail |
| `host-model` | libvirt picks a model close to the host | Good default; migration-safe within a pool |
| `custom` | Explicit model/features | Compatibility with older guests |

**Failure mode:** a VM starts on host A with `host-passthrough`, then live-migrates to host B with a different CPU. The guest may crash with an illegal instruction. Use a named CPU model for migration pools.

### NUMA

A server has multiple CPU sockets, each with local memory. Accessing remote memory is slower.

```bash
lscpu | grep -i numa
numactl --hardware
```

```text
available: 2 nodes (0-1)
node 0 cpus: 0 1 2 3 4 5 6 7
node 0 size: 32768 MB
node 1 cpus: 8 9 10 11 12 13 14 15
node 1 size: 32768 MB
```

A VM with 16 vCPUs and 64 GB RAM should ideally fit within one NUMA node, or be explicitly split. Otherwise the guest's memory accesses cross the interconnect and throughput drops.

```bash
virsh numatune vm1
virsh vcpupin vm1
virsh emulatorpin vm1
```

`vcpupin` pins vCPU threads to host CPUs; `numatune` binds guest memory to NUMA nodes. Use only when you have measured a NUMA penalty, not by default.

### Overcommit

`vCPU overcommit ratio = total vCPU / total physical CPU threads`. A 4:1 ratio is common for bursty workloads; a database with steady CPU load should be closer to 1:1. Overcommit is safe only if the guests do not all peak at once.

---

## 4. Memory virtualization

The guest sees physical addresses starting at 0. The hypervisor maps them to host memory:

```text
guest virtual → guest physical (guest page tables)
guest physical → host physical (EPT/NPT)
```

### Overcommit and ballooning

`virsh dominfo` shows memory; `virsh dommemstat` shows usage. The **virtio-balloon** driver lets the host ask the guest to return unused pages. It is cooperative: a guest under memory pressure may not return pages, and a guest without the driver ignores the request.

```bash
virsh dommemstat vm1
```

```text
actual 4194304
swap_in 0
swap_out 0
major_fault 120
minor_fault 88421
unused 1048576
available 3145728
```

- `actual` = host memory currently allocated (KB).
- `unused` = guest-reported free memory.
- `available` = ballooned-down available memory.

**KSM** (Kernel Same-page Merging) deduplicates identical pages across VMs. It saves memory but costs CPU and can create side-channel risk. Check:

```bash
cat /sys/kernel/mm/ksm/run
```

`1` = enabled. Many production hosts disable KSM for security/performance reasons.

### Hugepages

Normal pages are 4 KB; hugepages are 2 MB or 1 GB. They reduce TLB misses and are often required for databases and DPDK.

```bash
grep -i huge /proc/meminfo
```

```text
AnonHugePages:         0 kB
ShmemHugePages:        0 kB
HugePages_Total:    1024
HugePages_Free:      512
Hugepagesize:       2048 kB
```

To give a VM hugepages, the host must reserve them and the guest XML must request them:

```xml
<memoryBacking>
  <hugepages/>
</memoryBacking>
```

Hugepages are not swappable and are reserved up front. If the host has no free hugepages, the VM fails to start — a common "why won't this VM boot" cause.

### OOM

If the host runs out of memory, the kernel OOM killer chooses a process. A QEMU process may be killed, taking the VM down. Check:

```bash
dmesg -T | grep -i -E 'oom|killed process'
journalctl -k --since '1 hour ago' | grep -i oom
```

Protect critical VMs with `oom_score_adj` or, better, do not overcommit beyond what you can guarantee.

---

## 5. Storage for VMs

### raw vs qcow2

| | raw | qcow2 |
|---|---|---|
| Allocation | Full size unless sparse | Grows on demand |
| Snapshots | External only (LVM/Ceph) | Internal/external, backing files |
| Performance | Slightly faster | CoW overhead |
| Features | Simple | Compression, encryption, backing chains |
| Use | High-performance DB, Ceph RBD | Templates, lab VMs, snapshots |

```bash
qemu-img info /var/lib/libvirt/images/vm1.qcow2
```

```text
image: /var/lib/libvirt/images/vm1.qcow2
file format: qcow2
virtual size: 20 GiB (21474836480 bytes)
disk size: 4.2 GiB
cluster_size: 65536
backing file: /var/lib/libvirt/images/base.qcow2 (actual path: ...)
Snapshot list:
ID        TAG               VM SIZE                DATE       VM CLOCK
1         before-upgrade    1.2 GiB 2026-10-26 22:00:00 00:01:23.456
```

- **virtual size** = what the guest sees.
- **disk size** = actual host usage.
- **backing file** = the parent image; changes are stored in the overlay.
- **cluster_size** = allocation granularity; smaller clusters waste less but have more metadata.

### Backing chains

```text
base.qcow2 (read-only)
  └── vm1-overlay.qcow2 (writes)
        └── vm1-snapshot.qcow2 (more writes)
```

A chain that is too long slows every read (QEMU walks the chain) and complicates deletion. Periodically **blockcommit** (merge overlay into base) or **blockpull** (copy backing data into the overlay).

```bash
virsh blockcommit vm1 vda --active --pivot
virsh blockpull vm1 vda --wait
```

### Cache modes

| Mode | Host page cache | Guest flush | Risk |
|---|---|---|---|
| `none` | No | Direct I/O | Safe for most; good performance |
| `writethrough` | Yes | Writes go through | Slow, safe |
| `writeback` | Yes | Guest flushes eventually | Host crash can lose data |
| `directsync` | No | Writes + flushes synced | Slow, very safe |
| `unsafe` | Yes | Ignored | Fast, data loss on host crash |

For a lab, `none` or `writeback` is typical. For a database, measure and understand the durability guarantee; do not assume the guest `fsync` reached the platter if the host lies about flushes.

### Discard/TRIM

Thin-provisioned storage needs to know when blocks are freed. Pass `discard='unmap'` on the disk and enable `discard` in the guest mount options:

```xml
<driver name='qemu' type='qcow2' discard='unmap'/>
```

Without discard, deleted guest data remains allocated on the host, and thin pools grow until full.

---

## 6. VM networking

### The default NAT network

```bash
virsh net-list --all
virsh net-dumpxml default
```

```xml
<network>
  <name>default</name>
  <forward mode='nat'/>
  <bridge name='virbr0' stp='on' delay='0'/>
  <ip address='192.168.122.1' netmask='255.255.255.0'>
    <dhcp>
      <range start='192.168.122.2' end='192.168.122.254'/>
    </dhcp>
  </ip>
</network>
```

The host runs dnsmasq on `virbr0`, hands out `192.168.122.x`, and NATs outbound traffic. Guests cannot be reached from outside without port forwarding. This is fine for labs, not for production.

### Bridge (the normal production path)

```text
guest eth0 ← virtio-net → tap0 → br0 → physical NIC
```

```bash
ip -d link show virbr0
bridge link show
ip -d link show tap0
```

A **tap** device is a virtual Ethernet endpoint. QEMU writes frames to the tap; the host bridge forwards them. The guest sees a virtio NIC.

```xml
<interface type='bridge'>
  <source bridge='br0'/>
  <model type='virtio'/>
</interface>
```

### macvtap and SR-IOV

| Mode | How | Trade-off |
|---|---|---|
| Bridge + tap | Host bridge forwards | Flexible, host can route/filter |
| macvtap | Guest shares host NIC via macvlan | Fast, but host↔guest communication is limited |
| SR-IOV VF | Guest gets a virtual function | Near-native, needs NIC support, less live migration |

### virtio and vhost

Virtio is a paravirtualized device: the guest knows it is a VM and uses an efficient ring buffer instead of emulating a real NIC. **vhost-net** moves the data plane into the host kernel, reducing QEMU context switches. **vhost-user** moves it to a separate userspace process (DPDK/OVS).

Without virtio, the guest emulates an e1000 or IDE device, which is much slower. Always use virtio for disk and network unless the guest OS lacks drivers.

---

## 7. The VM lifecycle

```bash
virt-install \
  --name lab-vm \
  --memory 2048 \
  --vcpus 2 \
  --disk path=/var/lib/libvirt/images/lab-vm.qcow2,size=20,format=qcow2,bus=virtio \
  --network network=default,model=virtio \
  --os-variant ubuntu24.04 \
  --cdrom /var/lib/libvirt/images/ubuntu-24.04.iso \
  --graphics none \
  --console pty,target_type=serial
```

Inspect:

```bash
virsh list --all
virsh dominfo lab-vm
virsh domblklist lab-vm
virsh domiflist lab-vm
virsh dumpxml lab-vm
```

```text
 Id   Name      State
------------------------
 3    lab-vm    running

Name:           lab-vm
UUID:           ...
State:          running
CPU(s):         2
Max memory:     2097152 KiB
Used memory:    2097152 KiB

Target   Source
-----------------------------------------
vda      /var/lib/libvirt/images/lab-vm.qcow2

Interface   Type     Source    Model    MAC
------------------------------------------------------------
vnet0       network  default   virtio   52:54:00:...

vnet0  bridge  virbr0  ... 52:54:00:...
```

Lifecycle:

```bash
virsh shutdown lab-vm      # ACPI graceful
virsh destroy lab-vm       # pull the plug
virsh start lab-vm
virsh suspend lab-vm
virsh resume lab-vm
virsh undefine lab-vm --remove-all-storage
```

`destroy` is not "delete"; it is a hard power-off. `undefine` removes the definition; `--remove-all-storage` deletes disks. Confusing the two is a classic way to lose a VM.

### Snapshots

```bash
virsh snapshot-create-as lab-vm before-upgrade "before kernel upgrade"
virsh snapshot-list lab-vm
virsh snapshot-revert lab-vm before-upgrade
virsh snapshot-delete lab-vm before-upgrade
```

An internal snapshot stores disk state inside the qcow2 and (optionally) VM memory. It is not a backup: if the qcow2 is corrupted, the snapshot is gone. External snapshots use a new overlay file and are more flexible.

---

## 8. Failure scenarios

### A — VM will not start: `Cannot access storage file`

```bash
virsh start lab-vm
```

```text
error: Failed to start domain 'lab-vm'
error: Cannot access storage file '/var/lib/libvirt/images/lab-vm.qcow2': Permission denied
```

Check:

```bash
ls -l /var/lib/libvirt/images/lab-vm.qcow2
namei -l /var/lib/libvirt/images/lab-vm.qcow2
getfacl /var/lib/libvirt/images/lab-vm.qcow2
```

The QEMU process runs as `qemu` (or `libvirt-qemu`); it needs execute permission on every parent directory and read/write on the disk. SELinux may also deny it:

```bash
sudo ausearch -m avc -ts recent
sudo restorecon -Rv /var/lib/libvirt/images
```

### B — VM boots but has no network

```bash
virsh domiflist lab-vm
ip -d link show vnet0
bridge link show
virsh net-list
```

If `vnet0` exists but is not attached to `virbr0`, the network definition changed or the bridge is down. If the guest has no DHCP address, check dnsmasq on the host and the guest's interface state. If the guest can reach the host but not the internet, check `net.ipv4.ip_forward` and the NAT rules.

### C — VM disk is slow

```bash
iostat -xz 1
virsh domblkstat lab-vm vda
qemu-img info /var/lib/libvirt/images/lab-vm.qcow2
```

Causes: qcow2 on a network filesystem without proper locking, cache mode `writethrough`, a long backing chain, host disk saturation, or memory pressure forcing I/O. `%util` near 100 with high `await` means the host block device is the bottleneck, not the guest.

### D — VM memory is exhausted

```bash
virsh dommemstat lab-vm
free -m
dmesg -T | grep -i oom
```

If the guest is swapping, it needs more memory or a smaller workload. If the host is swapping, overcommit is too aggressive. Ballooning can reclaim only unused guest pages; it cannot fix a guest that genuinely needs more.

### E — live migration fails

```text
error: unsupported configuration: host doesn't support invariant TSC
```

or a CPU feature mismatch. Check both hosts:

```bash
virsh capabilities | grep -E 'model|feature'
virsh domcapabilities | grep -A10 '<cpu>'
```

Use a common CPU model (`host-model` with a named baseline), ensure shared storage is visible on both hosts, and check that the migration network has enough bandwidth. Live migration copies dirty memory; a busy guest may never converge.

---

## 9. Command field manual

### `virsh` — libvirt lifecycle

```bash
virsh list --all
virsh dominfo <vm>
virsh dumpxml <vm>
virsh domblklist <vm>
virsh domiflist <vm>
virsh dommemstat <vm>
virsh vcpuinfo <vm>
```

- `dumpxml` is the source of truth for the VM definition; `virsh edit` edits it.
- `domblklist` shows disks and their backing files.
- `domiflist` shows vNICs, type, source bridge/network, model, MAC.
- `vcpuinfo` shows host CPU affinity and state.

**Mistake:** editing `/etc/libvirt/qemu/<vm>.xml` directly. libvirt may overwrite it. Use `virsh edit`.

### `qemu-img` — image inspection

```bash
qemu-img info disk.qcow2
qemu-img check disk.qcow2
qemu-img create -f qcow2 disk.qcow2 20G
qemu-img create -f qcow2 -b base.qcow2 -F qcow2 overlay.qcow2
qemu-img convert -O raw disk.qcow2 disk.raw
```

- `info` shows virtual size, actual size, backing file, snapshots.
- `check` finds qcow2 metadata corruption; it is not a filesystem check.
- `-b`/`-F` create an overlay on a base image.
- `convert` is how you move between formats; do it with the VM stopped.

**Mistake:** deleting a base image while an overlay depends on it. The overlay becomes unreadable. `qemu-img info` shows the backing file; merge or flatten first.

### `virt-host-validate` — host readiness

```bash
virt-host-validate
```

Checks virtualization extensions, `/dev/kvm`, cgroups, and optional features. Run it before blaming libvirt.

### `lscpu`, `numactl` — topology

```bash
lscpu
numactl --hardware
```

Use these to size vCPUs/memory to NUMA nodes and to explain why a 64-vCPU VM is not automatically faster.

### `iostat` — block I/O

```bash
iostat -xz 1
```

```text
Device            r/s     w/s     rkB/s     wkB/s   await  %util
nvme0n1         120.0   340.0   4800.0   13600.0    0.42   12.3
```

- `await` = average latency (ms) for requests, including queue time.
- `%util` = fraction of time the device had I/O in flight; 100% on a single queue can be misleading for NVMe.
- `r_await`/`w_await` separate read/write latency.

See [Linux storage](02-linux-storage-lvm-filesystems.md) for the full interpretation.

---

## 10. Common mistakes

| Mistake | Why it hurts | Fix |
|---|---|---|
| Treating Docker as virtualization | Shared kernel, different isolation | Explain namespaces/cgroups vs VMs |
| Using `qemu-img` on a running VM | Corruption | Stop or snapshot first |
| Editing libvirt XML files directly | Overwritten | `virsh edit` |
| `destroy` when you meant `shutdown` | Data loss | ACPI shutdown first |
| `undefine --remove-all-storage` casually | Deletes disks | Confirm the VM and backups |
| `host-passthrough` in a migration pool | CPU mismatch crash | Named CPU model |
| Ignoring NUMA on large VMs | Cross-node memory penalty | `numatune`/`vcpupin` with evidence |
| Overcommitting memory without ballooning | Host OOM kills VMs | Monitor, reserve, set limits |
| Long qcow2 backing chains | Slow reads, fragile | Commit/pull periodically |
| Forgetting discard | Thin storage grows forever | `discard=unmap` + guest TRIM |
| Assuming `fsync` is durable with cache=writeback | Host crash loses data | Match cache mode to durability needs |

---

## 11. Checkpoint

1. What does KVM do that QEMU does not?
2. What is `/dev/kvm`, and why does QEMU need it?
3. Why is each vCPU a host thread?
4. What is EPT/NPT, and which problem does it solve?
5. When would `host-passthrough` break live migration?
6. What is the difference between `virsh destroy` and `virsh undefine`?
7. What does a qcow2 backing file do, and why can a long chain be slow?
8. What is the difference between `raw` and `qcow2`?
9. What is a tap device, and how does a guest frame reach the physical NIC?
10. What is virtio, and why is it faster than emulated e1000?
11. What does memory ballooning actually reclaim?
12. Why can a VM fail to start when hugepages are enabled?
13. What does `iostat` `%util` near 100 tell you, and what does it not tell you?
14. Why is an internal qcow2 snapshot not a backup?
15. A VM has 16 vCPUs but is slower than a 4-vCPU VM. Name three possible reasons.

### Answers

1. KVM provides CPU/memory virtualization in the kernel; QEMU emulates devices and manages the VM process.
2. The KVM device node; QEMU uses `ioctl` on it to create/run vCPUs.
3. KVM runs each vCPU as a thread scheduled by the host kernel.
4. Extended/Nested Page Tables; hardware-assisted guest-physical → host-physical translation, avoiding shadow page tables.
5. When the destination host lacks a CPU feature the guest saw at boot.
6. `destroy` powers off; `undefine` removes the definition (and optionally storage).
7. It stores only changed blocks; reads walk the chain, so long chains add latency and fragility.
8. raw is a full-size block image; qcow2 is copy-on-write with snapshots/backing files.
9. A virtual Ethernet endpoint; QEMU writes frames to the tap, the host bridge forwards them to the NIC.
10. A paravirtualized device with efficient ring buffers and fewer traps.
11. Only guest pages the guest agrees are unused; it is cooperative.
12. The host has no free reserved hugepages, or the guest requests more than available.
13. The device is busy; it does not tell you whether the bottleneck is the device, queue depth, or upstream network/storage.
14. It lives inside the same qcow2; corruption or deletion of the file destroys it.
15. NUMA crossing, CPU overcommit/steal, cache contention, or a single-threaded workload that cannot use 16 vCPUs.

If you miss more than three, re-read sections 2–6 and redo Lab L3.1.
