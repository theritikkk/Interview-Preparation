# Phase 3 Labs — Part 1 (L3.1–L3.2)

> **Safety:** L3.2 is destructive to the disks/LVs you create. Confirm every device with `lsblk` before running `pvcreate`, `mkfs`, or `dd`. Work in a disposable VM with a snapshot.

---

# Lab L3.1 — Build, Inspect, and Snapshot a KVM VM

## Objective

Create an Ubuntu cloud-image VM with libvirt, inspect its XML, QEMU process, disks, vNIC, and memory balloon, then take and revert a snapshot. Break the VM's storage permission and network attachment and diagnose both from evidence.

**Skills:** `virt-install`, `virsh`, `qemu-img`, cloud-init, libvirt XML, tap/vnet, snapshots.

## Prerequisites

- x86 Linux host/VM with VT-x/AMD-V and `/dev/kvm`.
- 8 GB free RAM, 25 GB free disk.
- Root/sudo; `qemu-kvm`, `libvirt-daemon-system`, `virtinst`, `cloud-image-utils`, `qemu-utils`.

Check:

```bash
egrep -c '(vmx|svm)' /proc/cpuinfo
ls -l /dev/kvm
sudo virt-host-validate
```

If `/dev/kvm` is missing, stop. Do not continue with pure emulation; the lab will be too slow.

## Setup

```bash
sudo apt update
sudo apt install -y qemu-kvm libvirt-daemon-system libvirt-clients virtinst \
  cloud-image-utils qemu-utils
sudo systemctl enable --now libvirtd
sudo usermod -aG libvirt,kvm "$USER"
```

Log out and back in (or `newgrp libvirt`) so group membership applies. Verify:

```bash
virsh list --all
virsh net-list --all
```

```text
 Id   Name   State
--------------------

 Name      State    Autostart   Persistent
--------------------------------------------
 default   active   yes         yes
```

If the default network is not active:

```bash
sudo virsh net-start default
sudo virsh net-autostart default
```

Download a cloud image (about 600 MB):

```bash
cd /var/lib/libvirt/images
sudo curl -LO https://cloud-images.ubuntu.com/jammy/current/jammy-server-cloudimg-amd64.img
sudo qemu-img info jammy-server-cloudimg-amd64.img
```

Expected: `file format: qcow2`, `virtual size: 2.2G` (or similar), no backing file.

Create a 20 GB overlay so the base image stays pristine:

```bash
sudo qemu-img create -f qcow2 -b jammy-server-cloudimg-amd64.img \
  -F qcow2 lab-vm.qcow2 20G
sudo qemu-img info lab-vm.qcow2
```

Expected: `backing file: jammy-server-cloudimg-amd64.img`, `virtual size: 20 GiB`.

Create cloud-init seed data. Replace the SSH key with your public key (`~/.ssh/id_ed25519.pub`), or use a password for console login:

```bash
cat > user-data <<'EOF'
#cloud-config
users:
  - name: lab
    sudo: ALL=(ALL) NOPASSWD:ALL
    shell: /bin/bash
    ssh_authorized_keys:
      - ssh-ed25519 AAAA... your-key-here
ssh_pwauth: true
chpasswd:
  list: |
    lab:labpass
  expire: false
EOF

cat > meta-data <<'EOF'
instance-id: lab-vm
local-hostname: lab-vm
EOF

sudo cloud-localds seed.iso user-data meta-data
```

Create the VM:

```bash
sudo virt-install \
  --name lab-vm \
  --memory 2048 \
  --vcpus 2 \
  --disk /var/lib/libvirt/images/lab-vm.qcow2,format=qcow2,bus=virtio \
  --disk /var/lib/libvirt/images/seed.iso,device=cdrom \
  --import \
  --os-variant ubuntu22.04 \
  --network network=default,model=virtio \
  --graphics none \
  --console pty,target_type=serial
```

Expected: the installer imports the image, boots, and cloud-init configures the user. It may take 30–90 seconds. If it drops to a login prompt, log in as `lab` with the password or SSH key.

Find the address:

```bash
sudo virsh domifaddr lab-vm
```

```text
 Name       MAC address          Protocol     Address
-------------------------------------------------------------------------------
 vnet0      52:54:00:...         ipv4         192.168.122.45/24
```

SSH in:

```bash
ssh lab@192.168.122.45
```

## Steps and expected observations

### 1. Inspect the definition

```bash
virsh dominfo lab-vm
virsh dumpxml lab-vm
```

Look for:

```xml
<domain type='kvm'>
  <name>lab-vm</name>
  <memory unit='KiB'>2097152</memory>
  <vcpu placement='static'>2</vcpu>
  <devices>
    <disk type='file' device='disk'>
      <driver name='qemu' type='qcow2'/>
      <source file='/var/lib/libvirt/images/lab-vm.qcow2'/>
      <target dev='vda' bus='virtio'/>
    </disk>
    <interface type='network'>
      <source network='default'/>
      <model type='virtio'/>
      <target dev='vnet0'/>
    </interface>
  </devices>
</domain>
```

`type='kvm'` proves hardware acceleration. `bus='virtio'` proves the guest uses virtio, not emulated IDE/e1000.

### 2. Inspect disks and NICs

```bash
virsh domblklist lab-vm
virsh domiflist lab-vm
```

```text
 Target   Source
-----------------------------------------------
 vda      /var/lib/libvirt/images/lab-vm.qcow2
 sda      /var/lib/libvirt/images/seed.iso

 Interface   Type     Source    Model    MAC
------------------------------------------------------------
 vnet0       network  default   virtio   52:54:00:...
```

### 3. Inspect the QEMU process and vCPU threads

On the host:

```bash
ps -eLf | grep '[q]emu-system' | head -10
```

Expected: one main thread plus one thread per vCPU, all named `qemu-system-x86_64` (or `CPU 0/KVM` in `top -H`).

```bash
top -H -p "$(pgrep -f 'guest=lab-vm' | head -1)"
```

You should see `CPU 0/KVM` and `CPU 1/KVM` threads. This is the concrete proof that a vCPU is a host thread.

### 4. Inspect the tap/vnet device and bridge

```bash
ip -d link show vnet0
bridge link show
ip -d link show virbr0
```

Expected: `vnet0` is a tun/tap device with `master virbr0`; `virbr0` has an IP `192.168.122.1/24`. The guest's frames go guest → virtio-net → tap/vnet0 → virbr0 → NAT.

### 5. Memory balloon

```bash
virsh dommemstat lab-vm
```

Expected: `actual`, `unused`, `available`, and fault counters. If the guest lacks the balloon driver, `actual` may not change and `unused` may be absent.

Try a live balloon-down (if supported):

```bash
virsh setmem lab-vm 1536M --live
virsh dommemstat lab-vm
```

Expected: `actual` drops toward 1.5 GB if the guest returns pages. Restore:

```bash
virsh setmem lab-vm 2048M --live
```

### 6. Snapshot and revert

Inside the guest, create a marker:

```bash
ssh lab@192.168.122.45 'echo before > /tmp/marker; cat /tmp/marker'
```

On the host:

```bash
virsh snapshot-create-as lab-vm snap1 "before change"
virsh snapshot-list lab-vm
```

```text
 Name   Creation Time               State
---------------------------------------------------
 snap1  2026-10-26 22:50:00 +0000   running
```

Change the guest:

```bash
ssh lab@192.168.122.45 'echo after > /tmp/marker; cat /tmp/marker'
```

Revert:

```bash
virsh snapshot-revert lab-vm snap1
ssh lab@192.168.122.45 'cat /tmp/marker'
```

Expected: `before`. Reverting a running snapshot restarts/pauses the VM depending on the snapshot type; give it a few seconds.

### 7. Shutdown and restart

```bash
virsh shutdown lab-vm
virsh list --all
virsh start lab-vm
```

`shutdown` sends ACPI; if the guest ignores it, `virsh destroy` is a hard power-off. Do not use `destroy` as a synonym for "stop" on a production VM.

## Why it works

- `virt-install` writes a libvirt domain XML and starts QEMU with `-enable-kvm`.
- KVM runs vCPUs as host threads; QEMU emulates virtio devices.
- `vnet0` is the host side of the tap; `virbr0` is a Linux bridge with NAT and DHCP (dnsmasq).
- A qcow2 overlay stores only changed blocks; the base image remains read-only.
- A snapshot captures disk (and optionally memory) state; reverting restores it.

## Troubleshooting task

### Break A — storage permission

```bash
sudo chown root:root /var/lib/libvirt/images/lab-vm.qcow2
sudo chmod 600 /var/lib/libvirt/images/lab-vm.qcow2
virsh destroy lab-vm
virsh start lab-vm
```

Expected: `Cannot access storage file ... Permission denied`. Diagnose:

```bash
ls -l /var/lib/libvirt/images/lab-vm.qcow2
namei -l /var/lib/libvirt/images/lab-vm.qcow2
ps -o user,group,cmd -p "$(pgrep -f 'guest=lab-vm' | head -1)" 2>/dev/null
sudo ausearch -m avc -ts recent 2>/dev/null | tail
```

Fix ownership to the user QEMU runs as (often `libvirt-qemu:kvm` on Debian/Ubuntu, `qemu:qemu` on RHEL):

```bash
sudo chown libvirt-qemu:kvm /var/lib/libvirt/images/lab-vm.qcow2
sudo chmod 660 /var/lib/libvirt/images/lab-vm.qcow2
virsh start lab-vm
```

### Break B — network detached

```bash
virsh detach-interface lab-vm --type network --config
virsh destroy lab-vm
virsh start lab-vm
```

Expected: the guest boots but has no network. Diagnose:

```bash
virsh domiflist lab-vm
ip -br link show
bridge link show
```

Reattach:

```bash
virsh attach-interface lab-vm --type network --source default --model virtio --config
virsh destroy lab-vm
virsh start lab-vm
```

### Break C — qcow2 backing file missing

Do this only on a copy of the VM. Move the base image:

```bash
sudo mv /var/lib/libvirt/images/jammy-server-cloudimg-amd64.img /tmp/base.move
virsh destroy lab-vm
virsh start lab-vm
```

Expected: QEMU cannot open the overlay because the backing file is missing. `qemu-img info` still shows the backing path. Restore the file and start again. This is why backing images must be treated as production dependencies.

## Verification

```bash
virsh list --all
virsh domblklist lab-vm
virsh domiflist lab-vm
virsh snapshot-list lab-vm
ssh lab@192.168.122.45 'hostname; ip -br addr; df -h /'
```

Expected: VM running, `vda` + seed CD, `vnet0` on `default`, snapshot listed, guest reachable with a `192.168.122.x` address.

## Cleanup

```bash
virsh destroy lab-vm 2>/dev/null
virsh undefine lab-vm --remove-all-storage
sudo rm -f /var/lib/libvirt/images/{user-data,meta-data,seed.iso}
```

Do not delete the base image if you want to reuse it. If you moved it in Break C, restore it first.

## Write-up template

```markdown
## L3.1 — KVM VM lifecycle

**Host readiness:** /dev/kvm, virt-host-validate
**Image chain:** base + overlay (`qemu-img info`)
**XML evidence:** type=kvm, virtio disk/NIC, vnet0
**vCPU threads:** output from `ps -eLf`/`top -H`
**Network path:** guest → virtio → vnet0 → virbr0 → NAT
**Snapshot:** marker before/after revert
**Break A root cause:** ...
**Break B root cause:** ...
**Break C root cause:** ...
```

---

# Lab L3.2 — LVM, Filesystems, Inodes, and fstab Recovery

## Objective

Create an LVM volume, format and mount it by UUID, extend it online, fill it with tiny files to exhaust inodes, create a deleted-open file, and recover from a bad `/etc/fstab` entry. Measure the LV with `fio` and read `iostat`.

**Skills:** `lsblk`, `pvcreate`/`vgcreate`/`lvcreate`, ext4/XFS, fstab, `df -i`, `lsof +L1`, `fio`, `iostat`.

## Prerequisites

- L3.1 VM or any disposable Linux VM.
- Two extra virtual disks **or** two loopback files if you cannot add disks.
- `lvm2`, `e2fsprogs`, `xfsprogs`, `fio`, `lsof`, `sysstat`.

> **Destructive:** this lab formats disks. Confirm with `lsblk -o NAME,SIZE,TYPE,MODEL,SERIAL` that you are using the lab disks (e.g. `vdb`, `vdc`), not the root disk.

## Setup

### Option A — real virtual disks (preferred)

Add two 5 GB virtio disks to the VM (via `virsh attach-disk` or the hypervisor UI). Inside the guest:

```bash
lsblk -o NAME,SIZE,TYPE,FSTYPE,MOUNTPOINT,MODEL,SERIAL
```

Expected: `vdb` and `vdc`, 5G each, no filesystem/mountpoint. Do not proceed if either is mounted.

### Option B — loop devices (no extra disks)

```bash
mkdir -p ~/labs/l3.2
truncate -s 5G ~/labs/l3.2/disk1.img
truncate -s 5G ~/labs/l3.2/disk2.img
sudo losetup -fP ~/labs/l3.2/disk1.img
sudo losetup -fP ~/labs/l3.2/disk2.img
lsblk -o NAME,SIZE,TYPE,MOUNTPOINT
```

Expected: `/dev/loop0` and `/dev/loop1` (names may vary). Use those instead of `/dev/vdb`/`/dev/vdc` below.

### Install tools

```bash
sudo apt update && sudo apt install -y lvm2 e2fsprogs xfsprogs fio lsof sysstat
```

## Steps and expected observations

### 1. Create the PV/VG/LV

```bash
sudo pvcreate /dev/vdb
sudo vgcreate vg_lab /dev/vdb
sudo lvcreate -L 3G -n data vg_lab
sudo lvs
```

Expected:

```text
  LV   VG     Attr       LSize
  data vg_lab -wi-a----- 3.00g
```

### 2. Format and mount by UUID

```bash
sudo mkfs.ext4 -L labdata /dev/vg_lab/data
sudo mkdir -p /data
sudo blkid /dev/vg_lab/data
```

```text
/dev/vg_lab/data: LABEL="labdata" UUID="6f1c..." TYPE="ext4"
```

Add to `/etc/fstab` using the UUID:

```bash
UUID=$(sudo blkid -s UUID -o value /dev/vg_lab/data)
echo "UUID=$UUID  /data  ext4  defaults,nofail  0  2" | sudo tee -a /etc/fstab
sudo findmnt --verify
sudo mount -a
findmnt /data
df -hT /data
```

Expected: `/data` mounted ext4, about 3 GB. `findmnt --verify` should report no errors.

### 3. Extend online

```bash
sudo lvextend -L +1G /dev/vg_lab/data
sudo resize2fs /dev/vg_lab/data
df -h /data
```

Expected: size grows by ~1 GB; no unmount. Check `lvs` and `vgs`:

```bash
sudo lvs -o lv_name,lv_size,data_percent
sudo vgs -o vg_name,vg_size,vg_free
```

### 4. Inode exhaustion

Create a small filesystem with few inodes on the second disk:

```bash
sudo pvcreate /dev/vdc
sudo vgcreate vg_inode /dev/vdc
sudo lvcreate -L 500M -n tiny vg_inode
sudo mkfs.ext4 -N 512 /dev/vg_inode/tiny
sudo mkdir -p /tiny
sudo mount /dev/vg_inode/tiny /tiny
df -i /tiny
```

Expected: `Inodes 512`, `IFree ~500`. Create files until inodes run out:

```bash
sudo bash -c 'for i in $(seq 1 600); do touch /tiny/f$i 2>/dev/null || break; done'
df -i /tiny
df -h /tiny
touch /tiny/newfile
```

Expected: `IUse% 100%`, blocks still free, and `touch: cannot touch ... No space left on device`. This is the inode-full failure. Fix by deleting files:

```bash
sudo find /tiny -type f -delete
df -i /tiny
```

### 5. Deleted-open file

On `/data`:

```bash
sudo dd if=/dev/zero of=/data/bigfile bs=1M count=200 status=none
df -h /data
cat > /tmp/openfile.py <<'PY'
import time
f = open('/data/bigfile', 'rb')
time.sleep(300)
PY
python3 /tmp/openfile.py &
sleep 1
rm /data/bigfile
df -h /data
sudo lsof +L1 | grep bigfile
```

Expected: `df` still shows ~200 MB used; `lsof +L1` shows `(deleted)` and the PID/FD. Free it:

```bash
PID=$(pgrep -f openfile.py)
FD=$(sudo ls -l /proc/$PID/fd | awk '/bigfile/{print $9; exit}')
sudo truncate -s 0 /proc/$PID/fd/$FD
df -h /data
```

If the FD number is hard to find, `sudo lsof -p $PID | grep deleted` shows it. Kill the Python process afterward.

### 6. Bad fstab entry (safe recovery)

Back up fstab:

```bash
sudo cp /etc/fstab /etc/fstab.bak
```

Add a bogus entry:

```bash
echo 'UUID=00000000-0000-0000-0000-000000000000  /broken  ext4  defaults  0  2' | sudo tee -a /etc/fstab
sudo findmnt --verify
sudo mount -a
```

Expected: `mount: /broken: can't find UUID=...` and a non-zero exit. The system is still running; a reboot would drop to emergency mode (without `nofail`).

Recover without rebooting:

```bash
sudo cp /etc/fstab.bak /etc/fstab
sudo findmnt --verify
sudo mount -a
```

If you want to see emergency mode, do it in a snapshot and use the console. Do not reboot a remote production host to test this.

### 7. Measure with fio

```bash
sudo fio --name=randread --filename=/dev/vg_lab/data --direct=1 \
  --rw=randread --bs=4k --iodepth=16 --numjobs=1 --size=1G --runtime=20 \
  --time_based --group_reporting
```

Expected output includes `read: IOPS=...`, `clat` percentiles. Run a sequential write test too:

```bash
sudo fio --name=seqwrite --filename=/data/fio.test --direct=1 \
  --rw=write --bs=1M --iodepth=8 --size=1G --runtime=20 --time_based
```

Observe `iostat -xz 1` in another terminal. On a virtual disk, the guest sees the virtual device; the host may show a different bottleneck.

## Why it works

- LVM maps logical extents to physical extents; resizing the LV does not resize the filesystem.
- ext4 stores inodes at `mkfs` time (unless `-I`/features change); XFS allocates dynamically.
- A deleted file remains allocated while a process holds it open; `lsof +L1` links count 0.
- fstab is read at boot; an invalid entry can block the boot. `nofail` and `findmnt --verify` reduce risk.
- `fio` bypasses the page cache with `--direct=1`, measuring the device rather than RAM.

## Troubleshooting task

1. `lvextend` succeeded but `df` is unchanged. What did you forget?
2. `df -h` shows 40% used but `touch` fails. What do you check?
3. `df` and `du` disagree by 200 GB. Name two causes.
4. A thin pool is at 99%. What do you do first?
5. A reboot drops to emergency mode after an fstab change. What are the first three commands?
6. `fio` reports 50k IOPS but the application is slow. Name three possible reasons.

### Expected answers

1. Resize the filesystem (`resize2fs`/`xfs_growfs`).
2. Inodes (`df -i`), quota, read-only mount, or a full underlying pool.
3. Deleted-open files, reserved blocks, sparse files, or `du` missing mount boundaries.
4. Stop writes, identify the consumer, extend the pool or delete snapshots; do not ignore it.
5. `mount -o remount,rw /`, inspect `/etc/fstab`, fix/comment the bad line; then reboot.
6. Queue depth/latency, application I/O size/pattern, filesystem/journal overhead, network storage, or CPU bottleneck.

## Verification

```bash
lsblk
sudo pvs; sudo vgs; sudo lvs
findmnt /data /tiny
df -hT /data; df -i /tiny
sudo lsof +L1 | head
```

Expected: LVs active, mounts present, `/data` grown, `/tiny` inodes freed, no deleted-open file.

## Cleanup

Unmount and remove the test LVs/VGs:

```bash
sudo umount /data /tiny 2>/dev/null
sudo lvremove -y vg_lab/data vg_inode/tiny
sudo vgremove -y vg_lab vg_inode
sudo pvremove /dev/vdb /dev/vdc
sudo sed -i '\#/data#d; \#/tiny#d' /etc/fstab
```

For loop devices:

```bash
sudo losetup -d /dev/loop0 /dev/loop1
rm -f ~/labs/l3.2/disk*.img
```

Restore fstab from backup if needed:

```bash
sudo cp /etc/fstab.bak /etc/fstab
```

## Write-up template

```markdown
## L3.2 — LVM and filesystems

**PV/VG/LV:** ...
**ext4 online grow:** before/after `df`
**Inode exhaustion:** `df -i` output + error
**Deleted-open file:** `lsof +L1` output + truncate result
**fstab recovery:** bad line, `findmnt --verify`, fix
**fio:** IOPS/latency for 4K random and 1M sequential
**iostat:** await/%util during the test
**Root cause I missed at first:** ...
```

---

## Lab log

| Lab | Date | Time | Pass? | Weakest idea | Retest |
|---|---|---|---:|---|---|
| L3.1 | | | | | |
| L3.2 | | | | | |

Continue to [Labs part 2](05-labs-part2.md) for NFS, iSCSI, Ceph, and NVMe-oF.
