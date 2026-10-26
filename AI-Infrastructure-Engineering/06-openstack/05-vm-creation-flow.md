# The VM Creation Flow

> **The one-sentence model:** `openstack server create` is a long-running distributed transaction: Keystone authenticates, Nova reserves a database record and a Placement allocation, Neutron binds a port, Glance supplies the image, Cinder may attach a volume, and `nova-compute` asks libvirt to start the domain. Each stage has a different owner, log, and failure signature.

---

## 1. Preconditions

Before the API call, confirm the request is even valid:

```bash
openstack token issue                     # identity + project + catalog
openstack image list                      # image exists and is active
openstack flavor list                     # flavor exists and fits
openstack network list                    # network exists and is reachable
openstack security group list             # SG exists
openstack keypair list                    # key exists (if using SSH)
openstack quota show                      # instances/cores/RAM/ports/volumes
openstack availability zone list          # AZ exists
```

A missing image, flavor, network, keypair, or quota produces an error before scheduling. Do not debug Placement for a typo in the network name.

---

## 2. The sequence

```text
openstack CLI
  │ 1. Keystone: authenticate, get token + catalog
  ▼
nova-api
  │ 2. Validate request, quota, image/flavor/network references
  │ 3. Create DB record: status=BUILD, task_state=scheduling
  │ 4. RPC to nova-scheduler
  ▼
nova-scheduler
  │ 5. Build resource request from flavor + image + volume
  │ 6. Placement: allocation candidates (VCPU/RAM/DISK/traits)
  │ 7. Filters/weighers: AZ, compute enabled, affinity, NUMA, PCI
  │ 8. Claim allocation; choose host
  │ 9. RPC to nova-conductor → nova-compute
  ▼
nova-compute
  │ 10. task_state=spawning
  │ 11. Download/convert image (Glance → local/cache)
  │ 12. Create instance directory, disk, libvirt XML
  │ 13. Neutron: bind port, plug VIF (OVS/OVN/SR-IOV)
  │ 14. Cinder: connect/attach volumes (if any)
  │ 15. libvirt: define/start domain, power on
  ▼
guest
  │ 16. Bootloader/kernel, cloud-init/metadata, DHCP
  │ 17. SSH/application ready
  ▼
status=ACTIVE
```

### Stage 1 — Keystone

The CLI obtains a project-scoped token and the service catalog. A 401 here means the request never reached Nova; a wrong project means you may create the VM in the wrong tenant.

**Evidence:** `openstack token issue`, `openstack catalog list`.

### Stage 2 — Nova API validation

`nova-api` checks:

- the image exists and is visible/active;
- the flavor exists and is visible;
- the network/port exists and belongs to the project;
- the keypair/security group exists;
- the project has quota;
- the AZ is valid;
- the request is semantically valid (e.g., not both `--image` and `--volume` in a conflicting way).

**Evidence:** API response, `nova-api.log`, `openstack server show` (if a record was created).

### Stage 3 — Database record and scheduling

Nova creates the instance row with `status=BUILD`, `task_state=scheduling`, and sends an RPC message. The API returns `202 Accepted` with the server ID. At this point the VM does not exist on a host.

**Evidence:** `openstack server show` shows `BUILD`/`scheduling`; `server event list` shows the create action.

### Stage 4 — Scheduler and Placement

The scheduler turns the flavor/image/volume request into a resource request:

- `VCPU`, `MEMORY_MB`, `DISK_GB` from the flavor;
- traits from image/flavor extra specs (CPU policy, hugepages, GPU, NUMA);
- PCI devices from aliases;
- affinity/anti-affinity from server groups;
- AZ/aggregate constraints.

Placement returns allocation candidates. Filters remove hosts that fail AZ, compute state, image properties, affinity, NUMA, PCI, or aggregate constraints. Weighers choose among the rest. Nova claims the allocation.

**Evidence:** `openstack allocation candidate list`, `openstack resource provider usage show`, `nova-scheduler.log` (filter names and host choices).

### Stage 5 — Conductor and compute

`nova-conductor` persists the selected host and calls `nova-compute`:

```text
compute.build_and_run_instance()
  → task_state=spawning
  → _allocate_network / port binding
  → _create_image / disk
  → _create_domain / libvirt XML
  → _create_domain_and_network / plug VIF
  → power_on
  → status=ACTIVE
```

The order can vary slightly by driver/version, but the task states give you the stage.

**Evidence:** `openstack server show` (`OS-EXT-SRV-ATTR:host`, `task_state`), `nova-compute.log`, libvirt logs, `virsh list --all`.

### Stage 6 — Neutron port binding

Nova asks Neutron to bind the port to the chosen host. The agent on that host creates the tap/veth, OVS/OVN flows, security-group rules, and DHCP/metadata path.

**Evidence:** `openstack port show` (`binding:host_id`, `binding:vif_type`, `status`), `openstack network agent list`, `ovs-vsctl show`, `ip netns`.

### Stage 7 — Cinder attach (if any)

For a boot-from-volume or additional volume, Cinder exports the LUN/RBD and Nova connects it to the host. The volume must be in the same AZ as the instance.

**Evidence:** `openstack volume show` (`attachments`, `status`), `cinder-volume.log`, `nova-compute.log`, `iscsiadm -m session`, `rbd info`.

### Stage 8 — libvirt and guest boot

`nova-compute` defines the libvirt domain, starts it, and the guest boots. `ACTIVE` means libvirt started the domain; it does not mean the guest OS booted successfully or the network works.

**Evidence:** `openstack console log show`, `openstack console url show`, `virsh dumpxml`, guest cloud-init logs.

---

## 3. State and task-state map

| Status | Task state | Meaning | Where to look |
|---|---|---|---|
| `BUILD` | `scheduling` | Waiting for scheduler/Placement | Scheduler logs, Placement inventory |
| `BUILD` | `block_device_mapping` | Preparing volumes | Cinder/Nova compute logs |
| `BUILD` | `networking` | Binding ports/network | Neutron agents, port show |
| `BUILD` | `spawning` | Creating domain/disks | nova-compute, libvirt |
| `BUILD` | `powering-on` | Starting the domain | libvirt, console log |
| `ACTIVE` | `null` | Domain running | Guest boot/network is separate |
| `ERROR` | `null` | Creation or action failed | `fault`, `server event list`, logs |
| `SHUTOFF` | `null` | Powered off | Guest/ACPI, not a failed build |
| `RESIZED` | `null` | Waiting for confirm/revert | Resize workflow |
| `SHELVED_OFFLOADED` | `null` | Compute released | Shelve/unshelve workflow |

A VM can be `ACTIVE` with a `task_state` such as `rebooting` or `attaching_volume`; the task state tracks the in-flight operation.

---

## 4. A worked trace

```bash
openstack server create \
  --flavor m1.small \
  --image cirros \
  --nic net-id=<net-id> \
  --key-name opslab-key \
  --security-group opslab-sg \
  --wait \
  opslab-vm
```

If `--wait` returns:

```text
+---------------------+--------------------------------------+
| Field               | Value                                |
+---------------------+--------------------------------------+
| id                  | 8f7...                               |
| status              | ERROR                                |
| fault               | {'code': 500, 'message': 'No valid host was found...'} |
+---------------------+--------------------------------------+
```

Do not immediately delete it. Collect:

```bash
openstack server show opslab-vm -f yaml
openstack server event list opslab-vm
openstack server show opslab-vm -c OS-EXT-SRV-ATTR:host -c OS-EXT-AZ:availability_zone
openstack compute service list
openstack resource provider list
openstack allocation candidate list --resource VCPU=2 --resource MEMORY_MB=2048
```

If the event shows `compute.create_instance` with `NoValidHost`, the failure is scheduling. If it shows a later action, follow that stage.

### Example: successful but unreachable

```text
$ openstack server show opslab-vm -c status -c addresses
+-----------+--------------------------------------+
| Field     | Value                                |
+-----------+--------------------------------------+
| status    | ACTIVE                               |
| addresses | opslab-net=10.0.100.12               |
+-----------+--------------------------------------+
```

`ACTIVE` + an IP only proves the domain started and the port has an address. To prove reachability:

```bash
openstack port list --server opslab-vm
openstack port show <port> -c binding:vif_type -c binding:host_id -c status
openstack console log show opslab-vm
# inside the guest: ip route, ping gateway, curl metadata
```

If there is no floating IP, there is no path from your laptop; use the console or add a floating IP/router.

---

## 5. Failure-to-stage map

| Symptom | Stage | First evidence | Likely fix |
|---|---|---|---|
| 401 before a server ID | Keystone | `token issue`, auth URL/domain | Fix credentials/scope, re-source |
| 403 before a server ID | Policy/quota | Token roles, quota show | Role/scope/quota |
| 400 invalid image/flavor/network | API validation | Error message, `image/flavor/network show` | Correct the reference |
| `ERROR` `No valid host` | Scheduler/Placement | `server event list`, allocation candidates, scheduler log | Capacity/AZ/trait/flavor |
| `ERROR` image larger than disk | Scheduler/spawn | `image show`, `flavor show` | Larger flavor/smaller image |
| `BUILD` stuck `networking` | Neutron binding | `port show`, agents, neutron logs | Agent/port/VIF |
| `BUILD`/`ERROR` volume attach | Cinder/Nova | `volume show`, cinder/nova logs | AZ/backend/iSCSI/RBD |
| `ERROR` libvirt/spawn | Compute/libvirt | `nova-compute.log`, `virsh`, `dmesg` | Disk/image/XML/driver |
| `ACTIVE` but no SSH | Guest/network | Console, port, SG, floating IP, route | Network/SG/guest |
| `ACTIVE` but cloud-init failed | Metadata | Console, `cloud-init status`, metadata agent | Metadata/config drive |

### Do not do these first

- Delete and recreate the VM: you lose the evidence and may repeat the failure.
- Restart every OpenStack service: it can turn a local issue into a cloud-wide outage.
- Edit the Nova/Cinder/Neutron database to "fix" a state: unsupported and dangerous.
- Force-attach a volume across AZs or to a mounted filesystem: data corruption.
- Disable security groups to test connectivity: it hides the real fault and widens exposure.

---

## 6. Timing expectations

- API validation: milliseconds to seconds.
- Scheduling: usually sub-second to a few seconds; can be longer under contention.
- Image download/convert: seconds to minutes depending on size, cache, and backend.
- Port binding: sub-second to tens of seconds, depending on agent health.
- Volume attach: seconds to minutes, depending on backend/iSCSI.
- Guest boot: seconds to minutes; cloud-init can add more.
- `--wait` timeout: client-side; a timeout does not prove the server failed.

A VM that is `BUILD` for 10 minutes is not automatically broken, but it is worth checking the task state and logs. A VM that is `ERROR` is not "still building."

---

## 7. Checkpoint

1. What does `202 Accepted` from `server create` prove?
2. Which component creates the initial database record?
3. Which component chooses the host?
4. What does Placement contribute to scheduling?
5. What does `task_state=scheduling` mean?
6. What does `task_state=networking` mean?
7. What does `task_state=spawning` mean?
8. Why can a VM be `ACTIVE` but unreachable?
9. What does `server event list` show?
10. What is the difference between `status=ERROR` and `status=SHUTOFF`?
11. What does `OS-EXT-SRV-ATTR:host` tell you?
12. Where do you look for a port binding failure?
13. Where do you look for a volume attach failure?
14. Where do you look for a libvirt/spawn failure?
15. Why is deleting and recreating a failed VM a bad first step?
16. What is the first check for `No valid host was found`?
17. What is the first check for a VM stuck at `networking`?
18. What is the first check for a VM that boots but has no SSH key?
19. Why is `--wait` timeout not proof of failure?
20. Draw the path from `server create` to a running domain in five stages.

### Answers

1. The API accepted the request and created/queued an instance record; it does not prove scheduling or data-plane success.
2. `nova-api`.
3. `nova-scheduler`, using Placement candidates plus filters/weighers.
4. Inventory, traits, aggregates, and allocations; it answers which providers can satisfy the resource request.
5. The scheduler is choosing a host; the VM is not yet being spawned.
6. Nova/Neutron are binding the port and wiring the network.
7. `nova-compute` is creating disks/domain and starting the VM.
8. The domain is running, but the guest may be stuck at boot, have no route/floating IP, be blocked by a security group, or have failed cloud-init.
9. The lifecycle actions and results (create, schedule, spawn, network, volume, power-on), which identify the failing stage.
10. `ERROR` means an operation failed; `SHUTOFF` means the VM is powered off, which can be intentional.
11. The compute host that owns the instance; it tells you where to inspect libvirt/logs.
12. `openstack port show`, `openstack network agent list`, `neutron-server.log`, the host's OVS/OVN agent logs.
13. `openstack volume show`, `cinder-volume.log`, `nova-compute.log`, iSCSI/RBD tools.
14. `nova-compute.log`, libvirt logs, `virsh dumpxml`, `dmesg`, console log.
15. It destroys evidence and may repeat the same failure; inspect the event/fault/logs first.
16. Flavor vs Placement inventory/usage/allocation candidates, then compute services/AZ/aggregates and scheduler logs.
17. Port binding/agent state, Neutron logs, and the compute agent.
18. Metadata path/config drive, console log, `cloud-init status`; the key may never have been injected.
19. `--wait` is a client-side timeout; the server may still be building or may have succeeded after the client gave up.
20. Keystone auth → Nova API record → scheduler/Placement → conductor/compute → Neutron/Cinder/libvirt → guest boot.
