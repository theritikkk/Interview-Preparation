# Phase 6 Labs — Part 2 (L6.3–L6.4)

> **Safety:** L6.3 formats a disposable volume and snapshots it. L6.4 intentionally creates failed resources. Use only the sandbox project and only volumes/VMs you created. Never format a device you have not identified with `lsblk`; never detach a mounted filesystem without unmounting it.
>
> **Dependency:** L6.3/L6.4 assume the `opslab-vm`, `opslab-net`, `opslab-subnet`, `opslab-router`, `opslab-sg`, and `opslab-key` from L6.2 still exist. If you already cleaned them up, recreate them with L6.2 before starting. Keep them until the end of L6.4.

---

# Lab L6.3 — Cinder Volume: Attach, Persist, Snapshot, Restore

## Objective

Create a Cinder volume in the VM's AZ, attach it, format and mount it inside the guest, write data, snapshot it, detach it, restore the snapshot into a new volume, attach the restored volume, and prove the data survived. Then extend the volume and grow the filesystem.

## Prerequisites

- L6.2 completed; `opslab-vm` is `ACTIVE` and reachable.
- A Linux image with `e2fsprogs` (Ubuntu is ideal). If your cloud only has Cirros, use the console and check whether `mkfs.ext4` exists; otherwise do the attach/detach and skip the filesystem steps with a note.
- Volume quota for at least 3 GB total (1 GB original + 1 GB restored + snapshot).
- 45–60 minutes.

## Setup

```bash
source /path/to/openrc
openstack server show opslab-vm -c status -c OS-EXT-AZ:availability_zone -c addresses
openstack volume type list
openstack availability zone list --volume
openstack quota show
```

Record the server AZ:

```bash
export SERVER_AZ=$(openstack server show opslab-vm -f value -c OS-EXT-AZ:availability_zone)
echo "$SERVER_AZ"
```

If `volume type list` shows multiple types, use the default or the smallest capable type. Do not choose an encrypted/multi-attach type unless the lab explicitly covers it.

## Part A — Create and attach

```bash
openstack volume create --size 1 --availability-zone "$SERVER_AZ" opslab-vol
openstack volume show opslab-vol -f yaml
openstack server add volume opslab-vm opslab-vol
openstack volume show opslab-vol -c status -c attachments -c availability_zone
```

Expected:

```yaml
status: available          # before attach
availability_zone: nova    # or your AZ
attachments: []
```

After attach:

```yaml
status: in-use
attachments:
- server_id: <opslab-vm-id>
  device: /dev/vdb
```

The device path is a hint; confirm inside the guest.

## Part B — Format and mount inside the guest

```bash
ssh -i ~/.ssh/id_ed25519 ubuntu@"$FIP"
```

Inside the guest:

```bash
lsblk -o NAME,SIZE,TYPE,MOUNTPOINT,SERIAL
```

Identify the new 1 GB disk (often `vdb` or `sdb`). Confirm it is not the root disk before formatting.

```bash
sudo mkfs.ext4 /dev/vdb
sudo mkdir -p /data
sudo mount /dev/vdb /data
echo "snapshot-test-$(date -u +%Y%m%dT%H%M%SZ)" | sudo tee /data/proof.txt
sync
df -h /data
cat /data/proof.txt
```

Expected: `df -h /data` shows a 1 GB ext4 filesystem; the proof file contains the timestamp.

> If the device is `/dev/vdc` or `/dev/sdb`, use that name consistently. Do not use `/dev/vda` (usually the root disk).

## Part C — Snapshot and restore

From the client, while the volume is attached and the file is synced:

```bash
openstack volume snapshot create --volume opslab-vol --description "L6.3 proof" opslab-snap
openstack volume snapshot list
openstack volume snapshot show opslab-snap -c status -c size
```

Expected: `status: available`. A snapshot of an attached volume is crash-consistent; `sync` and a quiesced application are your responsibility.

Create a new volume from the snapshot in the same AZ:

```bash
openstack volume create \
  --snapshot opslab-snap \
  --size 1 \
  --availability-zone "$SERVER_AZ" \
  opslab-restored
openstack volume show opslab-restored -c status -c source_volid -c snapshot_id
```

Detach the original:

```bash
# In the guest:
sudo umount /data
```

```bash
# On the client:
openstack server remove volume opslab-vm opslab-vol
openstack volume show opslab-vol -c status -c attachments
```

Wait until `status: available` before attaching the restored volume:

```bash
openstack server add volume opslab-vm opslab-restored
openstack volume show opslab-restored -c status -c attachments
```

Inside the guest:

```bash
lsblk -o NAME,SIZE,TYPE,MOUNTPOINT,SERIAL
sudo mkdir -p /restored
sudo mount /dev/vdb /restored     # confirm the device from lsblk; may be vdc
cat /restored/proof.txt
```

Expected: the same timestamp. The restored volume is a new volume; the original snapshot is unchanged.

## Part D — Extend and grow (optional but recommended)

From the client:

```bash
openstack volume set --size 2 opslab-vol
# or: cinder extend opslab-vol 2
openstack volume show opslab-vol -c size -c status
```

Inside the guest (mount the original again first if it is detached):

```bash
lsblk
sudo resize2fs /dev/vdb
df -h /data
```

Expected: `lsblk` shows 2G; `df -h` shows ~2G after `resize2fs`. If you used a partition table, use `growpart` first, then `resize2fs`. If the filesystem is XFS, use `xfs_growfs /data`.

## Internal explanation

Cinder's API reserves the volume and the scheduler picks a backend matching the type/AZ. `cinder-volume` creates/exports the LUN or RBD image; `nova-compute` connects it (iSCSI login or librbd) and adds a libvirt disk. The guest sees a block device, not a Cinder object.

A snapshot is a backend point-in-time copy. Creating a volume from a snapshot clones that point in time; it does not modify the original. A backup would copy the volume to independent storage; a snapshot may die with the backend.

## Troubleshooting task

1. **Attach an already attached volume** to the same server:

```bash
openstack server add volume opslab-vm opslab-vol
```

Expected: an API error (already attached / invalid state). Record the exact message. Evidence: `openstack volume show opslab-vol -c status -c attachments`, `cinder-volume.log`, `nova-compute.log`. Explain why a second attach is not a valid way to get a second mount.

2. **Try to attach a volume that is still `creating`** (optional, if you can create one quickly):

```bash
openstack volume create --size 1 --availability-zone "$SERVER_AZ" opslab-pending
openstack server add volume opslab-vm opslab-pending
openstack volume show opslab-pending
```

If the API accepts it, it may queue the attach; if it rejects, record the state. Do not force anything.

3. **AZ mismatch (only if your cloud has multiple volume AZs):** create a volume in a different AZ, try to attach it to `opslab-vm`, and record the error. Do not try to force the attach. If you have only one AZ, write a short paragraph on what the error would prove.

4. **Explain** why `df -h` can show the old size after Cinder extends the volume, and the exact guest commands to fix it.

## Verification

- [ ] Volume is `in-use` and attached to `opslab-vm`.
- [ ] Guest sees a new block device and an ext4 filesystem.
- [ ] `proof.txt` exists before the snapshot.
- [ ] Snapshot is `available`.
- [ ] Restored volume contains the same proof.
- [ ] Original and restored volumes are both `available` after detach.
- [ ] Optional: the extended volume shows the new size in `lsblk` and `df`.

## Cleanup

Inside the guest, unmount everything you mounted:

```bash
sudo umount /restored 2>/dev/null || true
sudo umount /data 2>/dev/null || true
```

On the client:

```bash
openstack server remove volume opslab-vm opslab-restored
openstack volume delete opslab-restored
openstack volume snapshot delete opslab-snap
openstack server remove volume opslab-vm opslab-vol
openstack volume delete opslab-vol
openstack volume list
openstack volume snapshot list
```

If a volume is `in-use`, remove the attachment first. If a delete fails, inspect `openstack volume show` and wait; do not delete a snapshot that is the only copy of data you still need.

## Write-up template

```markdown
# L6.3 — Cinder volume

- Server/AZ:
- Volume/type/AZ:
- Device in guest:
- Snapshot/restored volume:

## Evidence
| Step | Command | Result |
|---|---|---|
| Attach | volume show | |
| Guest disk | lsblk | |
| Filesystem | df -h | |
| Snapshot | snapshot show | |
| Restore | cat /restored/proof.txt | |
| Extend | lsblk/df | |

## Failure tested
- Error:
- Fault domain:
- Evidence:
- Fix/prevention:

## Cleanup proof
(paste empty volume/snapshot lists)
```

---

# Lab L6.4 — Trace a Failed Launch and a Failed Attach

## Objective

Create three controlled failures — a launch that cannot schedule (or is rejected before scheduling), a VM that is `ACTIVE` but has no external path, and a volume attach that is invalid — and diagnose each from evidence. For every failure, name the stage, the evidence, the fault domain, and the smallest safe fix. Do not delete the resource until you have collected evidence.

## Prerequisites

- L6.2 (and preferably L6.3) completed; `opslab-vm` and its network still exist.
- Ability to create a flavor (admin/DevStack) for the scheduling variant; otherwise use the invalid-AZ variant.
- 45–60 minutes.
- A disposable environment. These failures are intentional.

## Setup

```bash
source /path/to/openrc
openstack server show opslab-vm -c status -c OS-EXT-AZ:availability_zone -c addresses
openstack network list
openstack availability zone list
openstack flavor list
openstack volume list
```

If `opslab-vm` is gone, recreate it with L6.2 before continuing. Record the server ID, network ID, image, flavor, and AZ.

```bash
export VM=opslab-vm
export NET_ID=$(openstack network show opslab-net -f value -c id)
export IMAGE=$(openstack image list -f value -c Name | grep -Ei 'ubuntu|cirros' | head -n1)
export FLAVOR=$(openstack flavor list -f value -c Name | grep -E 'm1.small|m1.tiny' | head -n1)
```

---

## Scenario A — A launch that never reaches a running domain

Choose **one** variant.

### A1 — Scheduling failure (admin/DevStack)

Create a deliberately oversized flavor:

```bash
openstack flavor create --vcpus 128 --ram 1048576 --disk 100 --id auto opslab.huge
openstack flavor show opslab.huge
```

Launch:

```bash
openstack server create \
  --flavor opslab.huge \
  --image "$IMAGE" \
  --nic net-id="$NET_ID" \
  --wait \
  opslab-bad-schedule
```

Expected: `ERROR` with `No valid host was found. There are not enough hosts available.` (or a similar scheduling failure).

### A2 — Pre-scheduling validation (non-admin)

Use a nonexistent AZ:

```bash
openstack server create \
  --flavor "$FLAVOR" \
  --image "$IMAGE" \
  --nic net-id="$NET_ID" \
  --availability-zone no-such-az \
  --wait \
  opslab-bad-az
```

Expected: an API error (400) or an `ERROR` server whose event shows the AZ/scheduler failure. The exact stage is the lesson: if no server ID is returned, the request failed at API validation; if a server record exists, it failed later.

### Evidence to collect

```bash
openstack server show opslab-bad-schedule -f yaml 2>/dev/null || \
openstack server show opslab-bad-az -f yaml
openstack server event list opslab-bad-schedule 2>/dev/null || \
openstack server event list opslab-bad-az
openstack compute service list
openstack hypervisor list
openstack aggregate list
openstack resource provider list
openstack resource provider usage show <provider-uuid>
openstack allocation candidate list --resource VCPU=2 --resource MEMORY_MB=2048
```

If the server record exists, `OS-EXT-SRV-ATTR:host` is empty/null for a scheduling failure. If it has a host, the failure happened after scheduling; follow the event/logs.

### Internal explanation

Nova API validation is synchronous: bad references/values can fail before a record exists. Scheduling is asynchronous: the API returns `202`, then Placement/filters decide. `No valid host` means no candidate satisfied the request after filters; it is not a libvirt or network failure. The hypervisor summary can look free while Placement's real allocations are full, or the flavor may request more than any single host.

### Troubleshooting task

Write down:

- Stage of failure: API validation / scheduling / spawn / network / guest.
- The exact evidence that proves it.
- The smallest fix: smaller flavor, valid AZ, add capacity, free capacity, or correct the request.
- Why deleting and recreating the same request would fail again.

### Verification

- [ ] You can point to the event/fault that identifies the stage.
- [ ] You can explain why `hypervisor show` alone is insufficient.
- [ ] You did not edit Placement/Nova state.

### Cleanup for A

```bash
openstack server delete --wait opslab-bad-schedule 2>/dev/null || true
openstack server delete --wait opslab-bad-az 2>/dev/null || true
openstack flavor delete opslab.huge 2>/dev/null || true
```

Only delete the flavor if you created it (A1). Do not delete a shared flavor.

---

## Scenario B — `ACTIVE` VM with no external path

### Create an isolated network

```bash
openstack network create opslab-isolated
openstack subnet create opslab-isolated-subnet \
  --network opslab-isolated \
  --subnet-range 10.0.200.0/24 \
  --gateway 10.0.200.1

export ISO_NET=$(openstack network show opslab-isolated -f value -c id)
```

Do **not** add a router or external gateway yet.

### Launch the VM

```bash
openstack server create \
  --flavor "$FLAVOR" \
  --image "$IMAGE" \
  --nic net-id="$ISO_NET" \
  --key-name opslab-key \
  --security-group opslab-sg \
  --wait \
  opslab-isolated-vm
```

Expected: `ACTIVE` with an address in `10.0.200.0/24`. There is no floating IP, so your laptop has no path to it.

### Evidence to collect

```bash
openstack server show opslab-isolated-vm -c status -c addresses -c OS-EXT-SRV-ATTR:host
openstack port list --server opslab-isolated-vm
openstack port show <port-id> -f yaml
openstack network agent list
openstack router list
openstack console log show opslab-isolated-vm
```

Then allocate a floating IP and try to associate it:

```bash
export FIP_ISO=$(openstack floating ip create "$EXTERNAL" -f value -c floating_ip_address)
openstack floating ip show "$FIP_ISO"
openstack server add floating ip opslab-isolated-vm "$FIP_ISO"
```

Expected: either the association is rejected because the subnet is not reachable from an external network, or it succeeds but traffic still has no path because there is no router. Both are valid evidence; record which happened.

### Internal explanation

A floating IP is NAT through a router with an external gateway. Without a router interface on the tenant subnet and an external gateway, there is no L3 path, even if the port is bound and the VM is `ACTIVE`. The guest's fixed IP is not routable from outside.

### Troubleshooting task

Before fixing, answer:

1. Is the port bound? (`port show`)
2. Is there a router with an interface on `opslab-isolated-subnet` and an external gateway?
3. Is the floating IP associated with the correct port/fixed IP?
4. Does the guest have a default route? (console)
5. What is the smallest change that creates a path: router + interface + external gateway, or direct attachment to an external/provider network?

Then fix it:

```bash
openstack router create opslab-iso-router
openstack router add subnet opslab-iso-router opslab-isolated-subnet
openstack router set opslab-iso-router --external-gateway "$EXTERNAL"
openstack server add floating ip opslab-isolated-vm "$FIP_ISO" 2>/dev/null || true
ping -c 2 "$FIP_ISO"
```

If the floating IP association was already created, it may start working after the router is added. If ping still fails, check the SG (ICMP/SSH) and the guest firewall.

### Verification

- [ ] The VM is `ACTIVE` before the fix.
- [ ] The port is bound.
- [ ] You can explain why there is no external path.
- [ ] After the router/gateway, the floating IP responds (or you can explain the remaining blocker).
- [ ] You did not disable security groups to make it work.

### Cleanup for B

```bash
openstack server delete --wait opslab-isolated-vm
openstack floating ip delete "$FIP_ISO"
openstack router unset --external-gateway opslab-iso-router
openstack router remove subnet opslab-iso-router opslab-isolated-subnet
openstack router delete opslab-iso-router
openstack subnet delete opslab-isolated-subnet
openstack network delete opslab-isolated
```

---

## Scenario C — An invalid volume attach

### Create and attach a volume

```bash
export SERVER_AZ=$(openstack server show "$VM" -f value -c OS-EXT-AZ:availability_zone)
openstack volume create --size 1 --availability-zone "$SERVER_AZ" opslab-attach-vol
openstack server add volume "$VM" opslab-attach-vol
openstack volume show opslab-attach-vol -c status -c attachments
```

### Try to attach it again

```bash
openstack server add volume "$VM" opslab-attach-vol
```

Expected: an API error such as `Volume ... is already attached to instance ...` or an invalid-state error. Record the exact message.

### Evidence to collect

```bash
openstack volume show opslab-attach-vol -f yaml
openstack server show "$VM" -c volumes_attached
openstack volume service list
# On the compute host, if you have access:
# sudo tail -n 50 /var/log/cinder/cinder-volume.log
# sudo tail -n 50 /var/log/nova/nova-compute.log
# iscsiadm -m session
```

### AZ mismatch variant (if multiple AZs)

```bash
openstack availability zone list --volume
# Choose an AZ different from $SERVER_AZ:
openstack volume create --size 1 --availability-zone <other-az> opslab-wrong-az
openstack server add volume "$VM" opslab-wrong-az
```

Expected: an AZ/attach error. Do not force it. If you only have one AZ, write what the error would prove and skip the command.

### Internal explanation

Attaching is a stateful transaction: Cinder exports the volume, Nova connects it, and the volume records the attachment. Attaching the same volume twice to the same server is not a second mount; it is a duplicate/invalid operation. Across AZs, the compute host cannot reach the backend's LUN/RBD in another AZ (unless the backend is specially replicated).

### Troubleshooting task

Write down:

- The exact error and whether it is API validation, Cinder state, or backend/compute.
- The evidence that proves it (`volume show` attachments, cinder/nova logs).
- The safe fix: detach before reattaching, use a separate volume for a second mount, or create the volume in the server's AZ.
- Why `--multi-attach` is not a license to mount ext4 on two servers.

### Verification

- [ ] You reproduced an invalid attach and captured the error.
- [ ] You can name the fault domain from the error + state.
- [ ] You did not corrupt a filesystem or force an AZ mismatch.
- [ ] The volume is still `in-use` or safely `available` after your diagnosis.

### Cleanup for C

```bash
openstack server remove volume "$VM" opslab-attach-vol
openstack volume delete opslab-attach-vol
openstack volume delete opslab-wrong-az 2>/dev/null || true
openstack volume list
```

---

## Final cleanup and proof

Delete the L6.2 resources if you are finished with the phase:

```bash
openstack server delete --wait opslab-vm
openstack floating ip delete "$FIP"
openstack router unset --external-gateway opslab-router
openstack router remove subnet opslab-router opslab-subnet
openstack router delete opslab-router
openstack subnet delete opslab-subnet
openstack network delete opslab-net
openstack security group delete opslab-sg
openstack keypair delete opslab-key
```

Check:

```bash
openstack server list
openstack volume list
openstack volume snapshot list
openstack floating ip list
openstack router list
openstack network list
openstack security group list
openstack keypair list
```

A clean lab has no VM, volume, snapshot, floating IP, router, tenant network, or lab keypair left. Shared images, flavors, external networks, and volume types must remain.

## Write-up template

```markdown
# L6.4 — Failure drills

## Scenario A — Launch failure
- Request:
- Stage (API/scheduler/spawn):
- Evidence:
- Fault domain:
- Fix:
- Why recreate would fail again:

## Scenario B — Active but isolated
- Port binding:
- Router/gateway:
- Floating IP:
- Guest route:
- Fix:
- Verification:

## Scenario C — Invalid attach
- Error:
- Volume state/attachments:
- Fault domain:
- Safe fix:
- Multi-attach/AZ lesson:

## Cleanup proof
(paste empty lists)
```
