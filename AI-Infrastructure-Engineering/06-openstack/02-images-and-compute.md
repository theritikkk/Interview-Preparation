# Images and Compute: Glance, Nova, Placement

> **The one-sentence model:** Glance stores the boot image, Nova turns an API request into a libvirt domain, and Placement is the inventory ledger that tells the scheduler whether a host has the CPU, RAM, disk, and traits the flavor needs. A VM launch fails when any one of those three disagrees with the request.

---

## 1. Glance: the image registry

### What an image is

An OpenStack image is a disk image plus metadata. The metadata tells Nova and the hypervisor how to boot it:

| Field | Meaning | Why it matters |
|---|---|---|
| `disk_format` | `qcow2`, `raw`, `vmdk`, `vdi`, `iso`, `aki/ari/ami` | Nova/libvirt must understand it; conversion may be needed |
| `container_format` | `bare`, `ovf`, `ova`, `aki/ari/ami` | `bare` is the normal VM disk; OVF is a packaging format |
| `status` | `queued`, `saving`, `active`, `deactivated`, `killed`, `deleted`, `pending_delete` | Only `active` is normally usable |
| `visibility` | `public`, `private`, `shared`, `community` | Who can see/use it |
| `size` | Bytes of the image | Download time, storage, cache pressure |
| `min_disk` | Minimum root disk GB | A flavor smaller than this cannot boot it |
| `min_ram` | Minimum RAM MB | The flavor must meet it |
| `protected` | Prevent deletion | Protects a base image |
| `hw_*` properties | Disk bus, VIF model, machine type, CD-ROM, etc. | Hypervisor compatibility |
| `owner` | Project that owns it | Visibility and quota |

```bash
openstack image list --long
openstack image show cirros
openstack image show ubuntu-24.04 -f yaml
```

Example:

```text
+----------------------------------+--------------+--------+
| ID                               | Name         | Status |
+----------------------------------+--------------+--------+
| 1b2c3d4e-...                     | cirros       | active |
| 5f6a7b8c-...                     | ubuntu-24.04 | active |
+----------------------------------+--------------+--------+

+------------------+--------------------------------------------------+
| Field            | Value                                            |
+------------------+--------------------------------------------------+
| status           | active                                           |
| disk_format      | qcow2                                            |
| container_format | bare                                             |
| size             | 123456789                                        |
| min_disk         | 5                                                |
| min_ram          | 512                                              |
| visibility       | public                                           |
| protected        | False                                            |
+------------------+--------------------------------------------------+
```

### Image lifecycle

```text
create metadata → queued → upload bytes → saving → active
                                             │
                                             ├── deactivated (admin)
                                             ├── killed (unusable)
                                             └── deleted / pending_delete
```

- **queued:** metadata exists, no bytes yet (or upload not started).
- **saving:** upload in progress.
- **active:** complete and usable.
- **deactivated:** hidden from normal use; can be reactivated by an operator.
- **killed:** unusable; often a failed upload or corrupt backend.
- **pending_delete:** marked for deletion; bytes may still exist.

```bash
# Create and upload in one command
openstack image create opslab-ubuntu \
  --disk-format qcow2 --container-format bare \
  --file ubuntu-24.04-minimal-cloudimg-amd64.img \
  --property hw_disk_bus=virtio \
  --property hw_vif_model=virtio

# Download for inspection
openstack image save --file /tmp/opslab-ubuntu.qcow2 opslab-ubuntu
qemu-img info /tmp/opslab-ubuntu.qcow2

# Share with another project (image must be shared first)
openstack image set --shared opslab-ubuntu
openstack image add project opslab-ubuntu <project-id>
openstack image member list opslab-ubuntu
```

### Backends and caching

Glance can store bytes in:

- **file:** local filesystem (simple, not shared across API nodes unless on shared storage).
- **Swift:** object storage.
- **Ceph RBD:** direct to a Ceph pool (common in production).
- **NFS:** shared filesystem.

Nova may cache images on compute nodes (`image_cache_manager`). A cached image can be stale or corrupt; the cache is an optimization, not the source of truth. If one host boots an image and another fails, compare the image checksum and the host's cache, not just the Glance status.

```bash
openstack image show <image> -c checksum -c size -c status
# On a compute host (operator):
# ls /var/lib/nova/instances/_base/
# du -sh /var/lib/nova/instances/_base/*
```

### Image failure modes

| Symptom | Likely cause | Evidence |
|---|---|---|
| `status: queued` forever | Upload never started/finished | `openstack image show`; Glance API/backend logs |
| `status: saving` stuck | Backend slow/unreachable, large image | Glance logs, backend capacity |
| `status: killed` | Corrupt upload, backend error | Glance logs, checksum mismatch |
| Image active but VM won't boot | Wrong `disk_format`, missing `hw_*`, image not bootable | Console log, libvirt error, `qemu-img info` |
| `min_disk` > flavor disk | Flavor too small | `openstack flavor show`, `image show` |
| Image visible but forbidden | Visibility/member not accepted | `image member list`, project ID |
| Different behavior per host | Image cache/backend inconsistency | Checksums, compute cache |

---

## 2. Nova: compute lifecycle

### The components

| Component | Job | Failure symptom |
|---|---|---|
| `nova-api` | Accepts REST requests, validates, writes DB | API errors, 500s, no instance record |
| `nova-scheduler` | Chooses a host from Placement candidates and filters | `No valid host was found` |
| `nova-conductor` | Mediates DB/RPC between API/compute; runs long tasks | Timeouts, stuck builds, DB errors |
| `nova-compute` | Talks to libvirt, creates/disks/network domain | `ERROR`, libvirt/spawn failures |
| `nova-novncproxy` | Console proxy | Console URL fails, VM is fine |
| `nova-api-metadata` | Metadata endpoint for guests | Cloud-init can't get metadata |
| Cells | Scale/partition the compute database | Instance mapping/cell errors |

```bash
openstack compute service list
openstack hypervisor list
openstack hypervisor stats show
```

Example:

```text
+----+----------------+------------+----------+---------+-------+----------------------------+
| ID | Binary         | Host       | Zone     | Status  | State | Updated At                 |
+----+----------------+------------+----------+---------+-------+----------------------------+
| 1  | nova-conductor | controller | internal | enabled | up    | 2026-10-27T10:00:00.000000 |
| 2  | nova-scheduler | controller | internal | enabled | up    | 2026-10-27T10:00:00.000000 |
| 3  | nova-compute   | compute1   | nova     | enabled | up    | 2026-10-27T10:00:00.000000 |
+----+----------------+------------+----------+---------+-------+----------------------------+
```

`State: up` is heartbeat-based. A compute can be `up` but unable to spawn because its libvirt/storage/network is broken. A compute can be `down` and still have running VMs; the control plane has lost contact, not necessarily the data plane.

### Flavors

A flavor is a resource template:

| Field | Meaning |
|---|---|
| `vcpus` | Virtual CPUs |
| `ram` | Memory MB |
| `disk` | Root disk GB (0 means "use image size" in some configurations; be explicit) |
| `ephemeral` | Ephemeral disk GB |
| `swap` | Swap MB |
| `rxtx_factor` | Network throughput factor (driver-dependent) |
| `is_public` | Visible to all projects |
| `extra_specs` | Scheduling/feature requirements (CPU policy, hugepages, PCI, NUMA) |

```bash
openstack flavor list
openstack flavor show m1.small
openstack flavor create --vcpus 2 --ram 2048 --disk 20 --ephemeral 0 --swap 0 opslab.small
openstack flavor set --property hw:cpu_policy=dedicated opslab.small
openstack flavor set --property hw:mem_page_size=large opslab.small
```

Extra specs are **requests**, not guarantees. A flavor that asks for `hw:cpu_policy=dedicated` needs a host with the right trait/capability; otherwise the scheduler finds no valid host. GPU passthrough uses aliases and PCI whitelists; a flavor with a GPU alias cannot land on a host without the configured PCI device.

### Instance states and task states

| State | Meaning |
|---|---|
| `BUILD` | Being created; not yet usable |
| `ACTIVE` | Running (or at least powered on) |
| `ERROR` | Creation or an operation failed; inspect `fault` |
| `SHUTOFF` | Powered off |
| `PAUSED` | CPU paused, memory retained |
| `SUSPENDED` | Suspended to disk/memory; different from shelved |
| `RESCUED` | Booted into a rescue image |
| `RESIZED` / `VERIFY_RESIZE` | Waiting for confirm/revert |
| `SHELVED` / `SHELVED_OFFLOADED` | Released compute resources |
| `DELETED` / `SOFT_DELETED` | Removed (soft delete may be recoverable) |

Task states include `scheduling`, `block_device_mapping`, `networking`, `spawning`, `powering-on`, `deleting`, `rebooting`, `resizing`, `migrating`. The pair `(status, task_state)` tells you where it stopped.

```bash
openstack server show opslab-vm -f yaml
openstack server event list opslab-vm
```

Example error:

```yaml
status: ERROR
task_state: null
fault:
  code: 500
  message: 'No valid host was found. There are not enough hosts available.'
  details: 'Traceback ...'
```

`server event list` is often more useful than the fault message because it shows each stage and the exception:

```text
+---------------------+---------------------+---------------------+----------+
| Time                | Action              | Start Time          | Result   |
+---------------------+---------------------+---------------------+----------+
| 2026-10-27T10:00:00 | compute.create_instance | ...             | Error    |
+---------------------+---------------------+---------------------+----------+
```

### Actions

| Need | Command | Notes |
|---|---|---|
| Start/stop | `openstack server start/stop` | Stop is a graceful ACPI shutdown |
| Hard reboot | `openstack server reboot --hard` | Use when the guest is hung |
| Soft reboot | `openstack server reboot --soft` | ACPI reboot |
| Console log | `openstack console log show <server>` | Boot/kernel messages; not interactive |
| Console URL | `openstack console url show <server>` | noVNC/SPICE URL |
| Rescue | `openstack server rescue` | Boots a rescue image; useful for broken fstab |
| Shelve | `openstack server shelve` | Frees compute; data on root disk may be offloaded |
| Resize | `openstack server resize --flavor <flavor>` | Then `resize confirm` or `resize revert` |
| Live migrate | `openstack server live-migrate --host <host>` | Shared storage/CPU compatibility required |
| Evacuate | `openstack server evacuate` | After host failure; needs shared storage or rebuild |
| Rebuild | `openstack server rebuild --image <image>` | Replaces root disk; data loss if not backed up |

A `SHUTOFF` VM still consumes quota and storage. `SHELVED_OFFLOADED` releases compute but keeps the volume/image state.

### Quotas

```bash
openstack quota show demo
openstack quota set --instances 20 --cores 40 --ram 81920 demo
```

Quota errors are explicit: `Quota exceeded for instances`, `cores`, `ram`, `volumes`, `floating_ips`, `security_groups`, `ports`. Check the project quota, not the host capacity, when the message names a quota.

---

## 3. Placement: the inventory ledger

Placement is a separate API/database. It does not schedule; it answers "which resource providers can satisfy this request?" Nova's scheduler consumes allocation candidates and then applies filters/weighers.

### The model

```text
Resource provider (compute1)
├── inventory: VCPU=64, MEMORY_MB=262144, DISK_GB=2000
├── traits: HW_CPU_X86_AVX2, CUSTOM_GPU_A100, COMPUTE_STATUS_DISABLED?
├── aggregates: az-nova, gpu-pool
└── allocations: instance-uuid → VCPU=4, MEMORY_MB=8192, DISK_GB=20
```

| Concept | Meaning |
|---|---|
| **Resource provider** | A thing that supplies resources, usually a compute host |
| **Inventory** | Total/reserved/min/max/step and allocation ratio per resource class |
| **Resource class** | `VCPU`, `MEMORY_MB`, `DISK_GB`, `PCI_DEVICE`, `SRIOV_NET_VF`, `CUSTOM_*` |
| **Trait** | A capability/flag (`HW_CPU_X86_AVX2`, `CUSTOM_GPU_A100`, `COMPUTE_STATUS_DISABLED`) |
| **Aggregate** | A group of providers (AZ, GPU pool); can carry traits |
| **Allocation** | Resources consumed by a consumer (instance UUID) |
| **Allocation candidate** | A provider/group that can satisfy a request |

```bash
openstack resource provider list
openstack resource provider inventory list <provider-uuid>
openstack resource provider usage show <provider-uuid>
openstack resource provider trait list <provider-uuid>
openstack allocation candidate list --resource VCPU=1 --resource MEMORY_MB=512
```

Example inventory:

```text
+----------------+---------------+----------+-----------+----------+-----------+---------------+
| resource_class | total         | reserved | min_unit  | max_unit | step_size | allocation_ratio |
+----------------+---------------+----------+-----------+----------+-----------+---------------+
| VCPU           | 64            | 0        | 1         | 64       | 1         | 1.0           |
| MEMORY_MB      | 262144        | 512      | 1         | 262144   | 1         | 1.0           |
| DISK_GB        | 2000          | 0        | 1         | 2000     | 1         | 1.0           |
+----------------+---------------+----------+-----------+----------+-----------+---------------+
```

`total` is not the same as "free". Free capacity is `total × allocation_ratio − reserved − sum(allocations)`. A host with 64 VCPU and 60 allocated has 4 free; if the flavor asks for 8, it is not a candidate even if the host is "up".

### How the scheduler uses Placement

1. Nova builds a request from the flavor (VCPU/RAM/disk, extra specs, traits, PCI).
2. Placement returns allocation candidates.
3. Nova scheduler applies filters (AZ, compute enabled, image properties, affinity, NUMA, PCI, aggregate).
4. Weighers choose among remaining hosts.
5. Nova claims the allocation in Placement.
6. `nova-compute` spawns the domain.

```bash
# Scheduler logs show filters and candidates
# /var/log/nova/nova-scheduler.log
```

If `openstack allocation candidate list` returns nothing for the flavor's resources, the problem is inventory/capacity/traits, not the Nova API. If candidates exist but the scheduler rejects them, read the filter logs.

### Aggregates and availability zones

```bash
openstack aggregate list
openstack aggregate show <aggregate>
openstack aggregate add host <aggregate> compute1
openstack aggregate set --property availability_zone=az-a <aggregate>
openstack availability zone list
```

An AZ is an aggregate with `availability_zone` metadata. A host can be in one AZ (for scheduling) and multiple aggregates (for traits/tenancy). A volume AZ must match the instance AZ for attach; a boot-from-volume request must be scheduled in the volume's AZ.

### GPU and accelerator scheduling

For AI infrastructure, Placement is where GPU scheduling becomes concrete:

- A GPU is exposed via PCI passthrough or SR-IOV; the compute host needs the PCI whitelist and driver.
- Placement inventory uses `PCI_DEVICE` or a `CUSTOM_*` resource class (deployment-specific).
- A flavor extra spec or alias requests the GPU; the scheduler filters hosts by trait/inventory.
- MIG partitions, vGPU, and whole-GPU passthrough have different inventory models; do not assume a `CUSTOM_GPU` class means the same thing across clouds.
- NUMA/CPU pinning/hugepages are requested via extra specs and validated by the `NUMATopologyFilter`; a GPU workload often needs all three aligned.

If a GPU flavor fails to schedule, check `resource provider inventory list`, `trait list`, the PCI whitelist on the host, and `nova-scheduler.log`; do not just resize the flavor.

---

## 4. Command field manual

### `openstack image create`

```bash
openstack image create opslab-ubuntu \
  --disk-format qcow2 \
  --container-format bare \
  --file ubuntu.img \
  --min-disk 10 --min-ram 1024 \
  --property hw_disk_bus=virtio
```

**Why:** registers metadata and uploads bytes. `--min-disk`/`--min-ram` prevent scheduling onto a flavor that cannot boot the image.

**Mistake:** using the wrong `disk_format` (e.g., qcow2 labeled raw) produces a boot failure or a corrupt disk; `qemu-img info` the file before upload.

### `openstack image show`

```bash
openstack image show <image> -f yaml
```

**Interpretation:** `status`, `disk_format`, `size`, `checksum`, `min_disk`, `min_ram`, `visibility`, `owner`, `properties`. An `active` image can still be non-bootable.

### `openstack flavor show`

```bash
openstack flavor show <flavor> -f yaml
```

**Interpretation:** `vcpus`, `ram`, `disk`, `OS-FLV-EXT-DATA:ephemeral`, `swap`, `extra_specs`. Extra specs are scheduling constraints; read them before blaming Placement.

### `openstack server show`

```bash
openstack server show <server> -f yaml
```

**Interpretation:** `status`, `task_state`, `fault`, `addresses`, `flavor`, `image`, `key_name`, `security_groups`, `volumes_attached`, `OS-EXT-SRV-ATTR:host`, `OS-EXT-AZ:availability_zone`. The host field tells you where to look next.

### `openstack server event list`

```bash
openstack server event list <server>
```

**Why:** shows the lifecycle actions and result. A failed `compute.create_instance` is the scheduler/compute stage; a failed `compute.build_and_run_instance` may be network/libvirt.

### `openstack console log show`

```bash
openstack console log show <server>
openstack console url show <server>
```

**Why:** boot/kernel/cloud-init messages when SSH is unavailable. `console log` is text; `console url` is the interactive console. A VM can be `ACTIVE` and still be stuck at a boot prompt.

### `openstack compute service list`

```bash
openstack compute service list
openstack compute service set --disable compute1 nova-compute
openstack compute service set --enable compute1 nova-compute
```

**Interpretation:** `State` is heartbeat; `Status` is enabled/disabled. Disabling a service stops new scheduling but does not stop running VMs. A `down` compute may still run its VMs.

### `openstack hypervisor list` / `show`

```bash
openstack hypervisor list
openstack hypervisor show compute1 -f yaml
openstack hypervisor stats show
```

**Interpretation:** `vcpus`, `vcpus_used`, `memory_mb`, `memory_mb_used`, `local_gb`, `local_gb_used`, `running_vms`, `state`, `status`. These are a quick sanity check; Placement is the scheduling source of truth.

### `openstack resource provider inventory list`

```bash
openstack resource provider list
openstack resource provider inventory list <uuid>
openstack resource provider usage show <uuid>
```

**Interpretation:** total/reserved/ratio vs used. If `usage` is at `total`, new flavors that need more cannot schedule, even if the hypervisor summary looks stale.

### `openstack allocation candidate list`

```bash
openstack allocation candidate list \
  --resource VCPU=2 \
  --resource MEMORY_MB=2048 \
  --resource DISK_GB=20
```

**Why:** tests Placement directly, without Nova's filters. Empty output means no provider can satisfy the resource request; non-empty output means the issue is likely a Nova filter or a later stage.

---

## 5. Failure scenarios

### A — `No valid host was found`

```text
$ openstack server show big-vm
status: ERROR
fault: No valid host was found. There are not enough hosts available.
```

Fault domain: scheduling/capacity/traits. Check:

1. `openstack flavor show <flavor>` — vCPU/RAM/disk and extra specs.
2. `openstack resource provider list` + `inventory list` + `usage show` — is there free inventory?
3. `openstack allocation candidate list --resource VCPU=... --resource MEMORY_MB=...` — does Placement see any candidate?
4. `openstack compute service list` — are computes enabled/up?
5. `openstack aggregate list` / `availability zone list` — is the requested AZ valid?
6. `nova-scheduler.log` — which filter removed the host?
7. Quota — an explicit quota error, not `No valid host`.

Fix: smaller flavor, add capacity, correct the AZ/aggregate/trait, enable a compute, or adjust the request. Do not edit Placement rows by hand in a lab you care about.

### B — Stuck in `BUILD` with a port binding failure

```text
$ openstack server show vm -f yaml
status: BUILD
task_state: networking

# nova-compute.log
Failed to allocate the network(s), not rescheduling
```

Fault domain: Neutron port binding/agent. Check `openstack port list --server vm`, `openstack port show <port>`, `openstack network agent list`, and the Neutron logs. The VM may be `ACTIVE` only after the port is bound; if binding fails, it stays in `BUILD` or goes `ERROR`.

### C — `ERROR` after spawn

```text
$ openstack server show vm
status: ERROR
fault: Unexpected error while running command: ... libvirtError ...
```

Fault domain: compute/libvirt/storage. Check `nova-compute.log` on the host from `server show`, the libvirt domain XML, `virsh list --all`, and the volume/image state. A full disk or a missing image can fail at spawn even though scheduling succeeded.

### D — Image too large for the flavor

```text
$ openstack server create --image ubuntu-24.04 --flavor m1.tiny ...
status: ERROR
fault: Image ... is larger than the flavor's disk
```

The image's virtual size / `min_disk` exceeds the flavor root disk. Fix: use a larger flavor, set `--min-disk` correctly, or create a smaller image. Do not resize the flavor to a disk smaller than the image.

### E — Quota exceeded

```text
Quota exceeded for instances: Requested 1, but already used 10 of 10 instances
```

Fault domain: project quota, not host capacity. `openstack quota show <project>`; request an increase or delete unused resources. A quota error can appear at API time or at scheduling depending on the resource.

### F — Compute service down

```text
$ openstack compute service list
| nova-compute | compute2 | nova | disabled | down |
```

New instances avoid `compute2`; existing instances may keep running. Check the host's `nova-compute` process, RabbitMQ connectivity, and logs. Enabling the service does not automatically fix the underlying reason it went down.

### G — Image cache inconsistency

One host boots an image; another fails with a corrupt disk or wrong size. Compare the image checksum in Glance, the cached file on each host, and the libvirt error. Clear the specific cache entry (operator action) and re-download; do not delete the Glance image to "reset" it.

### H — Console works, SSH does not

The VM is running; the fault is network/security, not compute. Check the port IP, security group, router, floating IP, and guest firewall. This is a Phase 2/Neutron problem wearing a Nova costume.

---

## 6. Checkpoint

1. What does Glance store besides the image bytes?
2. What is the difference between `disk_format` and `container_format`?
3. What does `min_disk` protect against?
4. What do the Glance states `queued`, `saving`, `active`, and `killed` mean?
5. Why can an `active` image still fail to boot?
6. What does `nova-scheduler` use Placement for?
7. What is the difference between a Nova service being `down` and `disabled`?
8. What does `(status=ERROR, task_state=null, fault=...)` tell you?
9. What does `openstack server event list` add beyond `server show`?
10. What is a flavor extra spec, and why can it make a flavor unschedulable?
11. What is an availability zone in Nova, and how is it implemented?
12. What does a Placement inventory `total` mean vs free capacity?
13. What is a resource provider trait? Give a GPU example.
14. Why can aggregate CPU/RAM fit but a VM still not schedule?
15. What is the first check for `No valid host was found`?
16. What is the first check for a VM stuck in `BUILD` at `networking`?
17. What is the first check for an image larger than the flavor disk?
18. Why does a `down` compute not necessarily stop its running VMs?
19. Why is `openstack hypervisor show` not the scheduling source of truth?
20. How would you prove a quota problem rather than a capacity problem?

### Answers

1. Metadata: format, status, visibility, size, checksum, min disk/RAM, owner, and hardware properties.
2. `disk_format` is the virtual disk format (`qcow2`, `raw`, etc.); `container_format` is the packaging (`bare`, `ovf`, etc.). A VM disk is usually `bare` + `qcow2`/`raw`.
3. It prevents scheduling onto a flavor whose root disk is smaller than the image's virtual size.
4. `queued`: metadata only; `saving`: upload in progress; `active`: complete/usable; `killed`: unusable/corrupt.
5. Wrong format/properties, non-bootable image, missing bootloader, or a flavor/hypervisor mismatch; `active` only means Glance has the bytes.
6. To get allocation candidates based on inventory, traits, aggregates, and allocations; Nova then filters/weighs.
7. `down` means no heartbeat (control plane lost contact); `disabled` means scheduler won't place new instances there. A disabled service can still be up; a down service may still run VMs.
8. The instance creation/operation failed; the fault message and logs identify the stage. `task_state=null` means no task is currently in progress.
9. It shows the lifecycle actions and their results over time, which often reveals the failing stage more clearly than the final fault.
10. A scheduling/feature requirement (CPU policy, hugepages, NUMA, PCI/GPU). If no host has the capability/trait, the flavor has no valid host.
11. A logical failure domain implemented as a host aggregate with `availability_zone` metadata; it groups compute hosts and can be used for placement and volume affinity.
12. `total` is the raw inventory; free is `total × allocation_ratio − reserved − allocations`. A high total with high usage has no free capacity.
13. A capability flag on a provider, e.g. `CUSTOM_GPU_A100` or `HW_CPU_X86_AVX2`; flavors/images can require traits.
14. Fragmentation, AZ/aggregate constraints, traits, PCI/NUMA, affinity, disabled/down computes, quota, or a flavor larger than any single host.
15. Read the flavor, Placement inventory/usage, allocation candidates, compute services, AZ/aggregates, and scheduler logs — in that order.
16. Check the port, Neutron agents, and `nova-compute`/Neutron logs; the port binding failed before the VM could finish networking.
17. Compare `image show` (`min_disk`, virtual size) with `flavor show` (`disk`); use a larger flavor or a smaller image.
18. The VMs are already running on the host; `down` means the control plane lost heartbeat, not that libvirt stopped the guests.
19. It reports the hypervisor's view and can lag; Placement is the inventory/allocation ledger the scheduler trusts.
20. The error names a quota and `openstack quota show` shows the limit/usage; capacity errors name scheduling/`No valid host` instead.
