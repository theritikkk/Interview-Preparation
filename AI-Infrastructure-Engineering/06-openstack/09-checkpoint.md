# Phase 6 Checkpoint — OpenStack

> **Rules:** closed book, 90 minutes for Parts A–D. No CLI, no notes, no search. Then do the practical sign-off in a sandbox project. Target: **80% overall**, and at least **32/40** on the diagnosis scenarios.
>
> **Assumptions:** a modern OpenStack (Wallaby or newer) with Keystone v3, Placement, Neutron ML2/OVS or OVN, Cinder, and Glance. Commands use the `openstack` CLI. Your cloud may use different AZ names, external network names, or image/flavor names; answer conceptually where the name is environment-specific.
>
> **Safety:** use a disposable project/DevStack. Do not run the practical in production. Delete every VM, volume, snapshot, floating IP, router, network, and keypair you create, and prove it with CLI output.

---

## Part A — Rapid facts (25 × 2 = 50 points)

1. What is the difference between a Keystone domain and a project?
2. What is a project-scoped token vs a system-scoped token?
3. What does the Keystone service catalog contain?
4. What is the difference between a role and a policy?
5. What is a role assignment? Name its three parts.
6. Why can a valid token still get a 403?
7. What is a Glance image's `disk_format` vs `container_format`?
8. What does `min_disk` protect against?
9. What do the image states `queued`, `saving`, `active`, and `killed` mean?
10. What does `nova-scheduler` use Placement for?
11. What is the difference between a Nova service being `down` and `disabled`?
12. What does `task_state=networking` mean during a launch?
13. What is a flavor extra spec? Give one GPU/NUMA example.
14. What is a Nova availability zone, and how is it implemented?
15. What is a Placement resource provider, inventory, and allocation?
16. Why can aggregate free CPU/RAM still produce `No valid host`?
17. What is the difference between a tenant network and a provider network?
18. What does a Neutron port represent, and what does `binding:vif_type: unbound` mean?
19. What does the DHCP agent do, and where does it run?
20. What is the difference between a router's external gateway and a floating IP?
21. What is SNAT vs DNAT in a Neutron router?
22. What is the difference between a Cinder snapshot and a backup?
23. Why must a Cinder volume and a Nova instance be in the same AZ to attach?
24. What does `openstack server event list` show?
25. What is the first thing you check when a VM is `ACTIVE` but SSH times out?

---

## Part B — Calculations (6 × 5 = 30 points)

### B1 — Placement capacity and fragmentation

A cluster has 4 compute hosts. Each has:

- inventory `VCPU=64`, `MEMORY_MB=262144`, `DISK_GB=2000`;
- allocation ratio `1.0`;
- reserved `VCPU=2`, `MEMORY_MB=4096`, `DISK_GB=20`.

Current allocations:

- Hosts A, B, C: `VCPU=40`, `MEMORY_MB=122880`, `DISK_GB=500` each.
- Host D: `VCPU=55`, `MEMORY_MB=204800`, `DISK_GB=1000`.

A flavor requests `VCPU=8`, `MEMORY_MB=32768`, `DISK_GB=100`.

Answer:

1. What is the free VCPU, memory, and disk on each host?
2. How many instances of this flavor fit on each host, and in total?
3. The aggregate free VCPU is 73. Why can the cluster not run 9 instances (9 × 8 = 72 vCPU)?
4. What is the smallest capacity change that lets Host D fit one more instance?
5. If the VCPU allocation ratio were 2.0, what would change, and what risk would you document?

### B2 — Neutron subnet capacity

A tenant subnet is `10.0.100.0/24`. Assume:

- the default allocation pool is `.2`–`.254` (253 addresses);
- the gateway is `.1`;
- one DHCP port and one router interface consume addresses from the pool;
- each VM consumes one port/IP;
- no additional HA/metadata ports.

Answer:

1. How many usable addresses are in the subnet, and how many are in the allocation pool?
2. After the DHCP port and router interface, how many addresses remain for VMs?
3. If 10 VMs are running, how many free addresses remain?
4. `openstack ip availability show` reports fewer free addresses than your calculation. Name two legitimate reasons.
5. What happens when the pool is exhausted, and what is the first command you run?

### B3 — Image and flavor fit

An image has:

- virtual size 8 GiB;
- `min_disk=10` GiB;
- `min_ram=1024` MB.

Flavors:

| Flavor | vCPU | RAM | Root disk |
|---|---:|---:|---:|
| A | 2 | 2048 MB | 10 GB |
| B | 4 | 512 MB | 20 GB |
| C | 2 | 4096 MB | 8 GB |
| D | 4 | 4096 MB | 20 GB |

Answer:

1. Which flavors can boot the image? Explain each.
2. Why does flavor B fail even though its disk is large enough?
3. Why does flavor C fail even though its RAM is large enough?
4. What is the minimum root disk you would specify for this image, and why?
5. If the image were `raw` and 8 GiB, would the answer to question 1 change? Why or why not?

### B4 — Project quota

Project quotas and usage:

| Resource | Quota | Used |
|---|---:|---:|
| Instances | 10 | 8 |
| Cores | 20 | 18 |
| RAM | 51200 MB | 49152 MB |
| Volumes | 10 | 6 |
| Gigabytes | 1000 | 300 |
| Floating IPs | 5 | 3 |
| Ports | 20 | 15 |
| Security groups | 10 | 4 |
| Security group rules | 50 | 20 |

A request asks for:

- 3 instances, each 4 vCPU, 8192 MB RAM, one 50 GB volume;
- 1 floating IP;
- 1 new security group with 5 rules.

Answer:

1. Which quotas block the request? Show the arithmetic.
2. Which quotas do **not** block it?
3. How many of the requested instances can fit under the current core quota? Under the RAM quota?
4. What is the smallest quota change that allows all 3 instances, assuming the flavor is fixed?
5. Why is a quota error not a scheduler/capacity error?

### B5 — Cinder backend capacity

A Cinder backend reports:

- 20 TB raw capacity;
- 10% reserved for metadata/rebuild;
- 30 volumes × 200 GB allocated;
- 1.5 TB of snapshots on the same backend;
- thin provisioning enabled, but the team policy is not to exceed 80% physical usage.

Answer:

1. What is the reserved capacity in TB?
2. What is the allocated volume + snapshot capacity?
3. How much physical capacity remains before the 80% policy limit? Show the arithmetic.
4. Can the team create 8 new 1 TB volumes? Explain.
5. Why is "the backend has 20 TB" not proof that a 1 TB volume will be placed successfully?

### B6 — Image download and `--wait`

An 8 GiB image is downloaded at an effective 500 Mbps link with 70% efficiency. Conversion adds 50% of the download time. Volume attach takes 30 s and guest boot takes 60 s. The client uses `--wait` with a 300 s timeout.

Answer:

1. What is the effective transfer rate in MB/s?
2. How long does the download take (using 1 GiB = 1024 MiB)?
3. How long does conversion add?
4. What is the approximate total time including attach and boot?
5. Will `--wait` time out? Does a timeout prove the launch failed? What should you do?

---

## Part C — Diagnosis scenarios (8 × 5 = 40 points)

For each scenario, name the **fault domain**, the **next two evidence sources/commands**, and the **fix**. Do not guess before naming the evidence.

### C1 — All commands return 401

```text
$ openstack server list
The request you have made requires authentication. (HTTP 401)

$ env | grep ^OS_
OS_AUTH_URL=https://cloud.example.com/identity/v3
OS_USERNAME=alice
OS_PROJECT_NAME=demo
OS_USER_DOMAIN_NAME=Default
OS_PROJECT_DOMAIN_NAME=Default
OS_REGION_NAME=RegionOne
```

The password was just changed, and the `openrc` file was copied from a teammate.

### C2 — 403 when creating a VM

```text
$ openstack server create --flavor m1.small --image cirros \
    --nic net-id=... myvm
You are not authorized to perform the requested action. (HTTP 403)

$ openstack token issue
roles:
- name: reader
project_id: 3f2b...
```

### C3 — `No valid host` with free-looking hypervisors

```text
$ openstack server show big-vm -f yaml
status: ERROR
fault:
  message: 'No valid host was found. There are not enough hosts available.'

$ openstack hypervisor show compute1 -f yaml
vcpus: 64
vcpus_used: 8
memory_mb: 262144
memory_mb_used: 32768

$ openstack resource provider usage show <compute1-uuid>
VCPU: 60
MEMORY_MB: 245760
DISK_GB: 1800
```

### C4 — VM stuck in `BUILD`

```text
$ openstack server show vm -f yaml
status: BUILD
task_state: networking

$ openstack port list --server vm
| ID | Name | Status | Binding: vif_type | Binding: host_id |
| p-1 | | DOWN | unbound | |

$ openstack network agent list
| Host | Agent Type | Alive | State |
| compute1 | Open vSwitch agent | False | DOWN |
```

### C5 — `ACTIVE` VM, SSH timeout

```text
$ openstack server show vm -c status -c addresses
status: ACTIVE
addresses: opslab-net=10.0.100.12

$ openstack floating ip list
| Floating IP | Fixed IP | Port | Status |
| 203.0.113.45 | 10.0.100.12 | p-1 | ACTIVE |

$ nc -vz 203.0.113.45 22
nc: connect to 203.0.113.45 port 22 (tcp) failed: Connection timed out
```

The guest console shows `sshd` running, and the guest's default route is `10.0.100.1`.

### C6 — Volume `available`, attach fails

```text
$ openstack volume show vol -c status -c availability_zone
status: available
availability_zone: az-a

$ openstack server show vm -c OS-EXT-AZ:availability_zone
OS-EXT-AZ:availability_zone: az-b

$ openstack server add volume vm vol
Error: ...
```

### C7 — Image `killed`, VM `ERROR`

```text
$ openstack image show ubuntu-custom -c status -c disk_format -c size
status: killed
disk_format: qcow2
size: 0

$ openstack server show vm -f yaml
status: ERROR
fault: ...
```

### C8 — Volume `in-use`, guest has no disk

```text
$ openstack volume show vol -c status -c attachments
status: in-use
attachments:
- server_id: vm-id
  device: /dev/vdb

$ ssh ubuntu@203.0.113.45 'lsblk'
NAME   SIZE TYPE MOUNTPOINT
vda     10G disk /
```

The compute host's `iscsiadm -m session` shows no session for the volume.

---

## Part D — Design and explain (2 × 20 = 40 points)

### D1 — Multi-tenant OpenStack landing zone (20 points)

Design a landing zone for three teams (`research`, `platform`, `students`) on one OpenStack cloud.

Requirements:

- Each team has its own project, quota, and non-overlapping tenant network.
- `research` needs larger VMs and GPUs; `students` needs small, capped VMs; `platform` runs shared services.
- Tenants must not reach each other's fixed IPs by default, but all need outbound internet.
- SSH is allowed only from a bastion or the team's VPN CIDR, not the whole internet.
- Images are curated by the platform team; tenants can use but not modify the base images.
- Volumes are encrypted for `research`; snapshots are retained for 7 days.
- The cloud has 3 AZs; a tenant VM should survive one AZ failure where the workload supports it.
- Chargeback/showback is by project and tag.

Answer:

1. Domains/projects/users/groups/roles and the role assignments.
2. Network design: CIDRs, routers, external network, security groups, bastion/VPN, east-west isolation.
3. Quotas and flavor strategy for the three teams, including GPU flavors.
4. Image lifecycle: who owns what, visibility, members, protected images, and updates.
5. Volume types, encryption, snapshots/backups, and AZ placement.
6. Monitoring/showback: which metrics, quotas, logs, and tags.
7. The top three operational failure modes and the evidence for each.

### D2 — GPU flavor and scheduling design (20 points)

A research team wants 8-GPU nodes for training on OpenStack.

Requirements:

- Each node has 8 GPUs, 2 NUMA nodes, 1 TB RAM, and 8 TB local NVMe.
- Training jobs need GPUs, CPU pinning, hugepages, and high-speed east-west networking (SR-IOV/RDMA).
- Datasets are shared read-only; checkpoints are written to durable storage.
- Jobs are submitted by researchers who should not be OpenStack admins.
- The cloud must prevent a single project from consuming every GPU.

Answer:

1. The host aggregate/trait/inventory model for GPU nodes.
2. The flavor extra specs and image properties for GPU, CPU pinning, hugepages, and NUMA.
3. The Neutron port/VIF model for SR-IOV/RDMA, and the MTU considerations.
4. The storage model for datasets and checkpoints (Cinder vs Manila/Swift/CephFS), including performance and sharing.
5. The Keystone project/role/quota model for researchers and operators.
6. The scheduler/filter settings and the evidence you would use when a GPU job stays `PENDING`/`ERROR`.
7. Three failure modes specific to GPU workloads and their first checks.

---

## Answer key

### Part A

1. A domain is a namespace for identity objects (users, groups, projects); a project is a resource-owning tenant inside a domain.
2. A project-scoped token is limited to one project and carries roles there; a system-scoped token is for cloud-wide operations and is not the same as project `admin`.
3. A mapping of service types/names to endpoints per region/interface, built into the token at issuance.
4. A role is a named assignment; a policy is the service-side rule that decides what that role/scope can do.
5. A binding of an actor (user/group), a role, and a scope (project/domain/system).
6. The token is valid but the policy denies: missing role, wrong project/scope, or a policy requiring a different role.
7. `disk_format` is the virtual disk format (`qcow2`, `raw`, etc.); `container_format` is the packaging (`bare`, `ovf`, etc.).
8. It prevents scheduling onto a flavor whose root disk is smaller than the image's virtual/minimum size.
9. `queued`: metadata only; `saving`: upload in progress; `active`: complete/usable; `killed`: unusable/corrupt.
10. Allocation candidates based on inventory, traits, aggregates, and allocations; Nova then filters/weighs.
11. `down` = no heartbeat/control-plane contact; `disabled` = scheduler will not place new instances there. A disabled service can be up; a down service may still run VMs.
12. Nova/Neutron are binding the port/wiring the network; the VM is not yet spawned.
13. A scheduling/feature requirement in a flavor, e.g. `hw:cpu_policy=dedicated`, `hw:mem_page_size=large`, `pci_passthrough:alias=gpu:1`.
14. A logical failure domain implemented as a host aggregate with `availability_zone` metadata; it groups compute hosts for placement and volume affinity.
15. A provider is a resource supplier; inventory is its total/reserved/ratio; an allocation is the resources consumed by a consumer such as an instance.
16. Fragmentation, AZ/aggregate/trait/PCI/NUMA constraints, affinity, disabled/down computes, quota, or a flavor larger than any single host.
17. A tenant network is project-owned, usually overlay/VLAN, and needs a router for external access; a provider network maps to a physical network (often external/public) and can be attached directly.
18. A port is the virtual NIC binding point (MAC/IP/SG/binding state); `unbound` means no host has claimed/plugged it yet.
19. It runs dnsmasq in a `qdhcp-<network>` namespace and hands out IP/gateway/DNS/MTU to guests.
20. The external gateway connects a router to an external network and provides SNAT/default route; a floating IP is a 1:1 DNAT/SNAT mapping to a fixed IP through that router.
21. SNAT rewrites the source to the router's external address for outbound; DNAT rewrites the destination from the floating IP to the guest's fixed IP for inbound.
22. A snapshot is a backend-local point-in-time copy for fast clone/restore; a backup is copied to independent storage for recovery/retention.
23. Block volumes are AZ-scoped; the compute host can only connect a volume in its AZ (unless a special replicated/multi-AZ backend is used).
24. The lifecycle actions and results (create, schedule, network, spawn, volume, power-on), which identify the failing stage.
25. Check the guest/console, security group, floating IP/router path, and port binding; `ACTIVE` only means the domain started.

### Part B

**B1**

1. Hosts A–C: VCPU `64 − 2 − 40 = 22`; memory `262144 − 4096 − 122880 = 135168` MB; disk `2000 − 20 − 500 = 1480` GB. Host D: VCPU `64 − 2 − 55 = 7`; memory `262144 − 4096 − 204800 = 53248` MB; disk `2000 − 20 − 1000 = 980` GB.
2. A–C: VCPU allows `floor(22/8)=2`; memory allows 4; disk allows 14 → **2 each**. D: VCPU 7 < 8 → **0**. Total **6 instances**.
3. The 72 vCPU are fragmented across four hosts; a flavor cannot span hosts. Host D has only 7 free, and A–C have 22 each (two 8-vCPU instances each, 6 vCPU stranded per host). Aggregate free capacity is not a single schedulable pool.
4. Add at least 1 vCPU to Host D's free capacity (e.g., migrate a small instance off D, or add a host). To run 7 more instances you would need more than that; the question asks for one more on D: free 8 vCPU.
5. With `allocation_ratio=2.0`, Placement advertises `64 × 2 = 128` vCPU per host, so more instances can be allocated; the risk is CPU overcommit/contention, noisy neighbors, and latency-sensitive workloads missing their performance SLO. Do not use overcommit for dedicated/pinned GPU workloads without measurement.

**B2**

1. `/24` has 256 addresses; the allocation pool `.2–.254` has 253. The network/broadcast and gateway are outside the pool (gateway `.1`).
2. `253 − 1 DHCP − 1 router = 251` addresses for VM ports (before other ports).
3. `251 − 10 = 241` free.
4. Additional ports (HA DHCP/router, metadata, load balancer, allowed address pairs), a different allocation pool, ports in `DOWN` state still holding IPs, or other projects sharing the network.
5. New ports fail with an IP-allocation/`No more IP addresses available` error; VMs may stay `BUILD`/`ERROR`. First command: `openstack ip availability show <network-id>` (then `openstack port list` for stale ports).

**B3**

1. **A** and **D** can boot: A has 10 GB disk ≥ `min_disk=10` and 2048 MB ≥ 1024; D has 20 GB and 4096 MB. B fails RAM (512 < 1024); C fails disk (8 < 10; also below the 8 GiB virtual size plus overhead).
2. `min_ram=1024` MB is a hard minimum; a 512 MB flavor cannot satisfy it even with a large disk.
3. `min_disk=10` GB is a hard minimum; an 8 GB root disk is too small, even with plenty of RAM.
4. At least 10 GB, and practically the image virtual size plus filesystem/overhead (8 GiB virtual size needs ≥ 8 GiB, but `min_disk=10` is the stated floor). Use 10–12 GB for safety.
5. The format alone does not change the `min_disk`/`min_ram` requirements; A and D still fit. `raw` may be larger on disk than a compressed qcow2, so verify the actual virtual size and backend capacity; it can make the disk requirement stricter, not looser.

**B4**

1. Instances: `8 + 3 = 11 > 10` → block. Cores: `18 + 12 = 30 > 20` → block. RAM: `49152 + 24576 = 73728 > 51200` → block.
2. Volumes: `6 + 3 = 9 ≤ 10`; gigabytes `300 + 150 = 450 ≤ 1000`; floating IPs `3 + 1 = 4 ≤ 5`; ports `15 + 3 = 18 ≤ 20`; security groups `4 + 1 = 5 ≤ 10`; rules `20 + 5 = 25 ≤ 50`.
3. Cores: 2 free, each instance needs 4 → **0** of the requested instances. RAM: 2048 MB free, each needs 8192 → **0**. (If smaller flavors were allowed, 0 at 4 vCPU/8 GB; you need a quota increase or a smaller flavor.)
4. To fit 3 × 4 vCPU, cores must be at least 30; to fit 3 × 8192 MB, RAM at least 73728 MB; instances at least 11. Raise all three to those values (with headroom).
5. A quota is a project-level API limit; the host may have plenty of capacity. The error names the quota resource; capacity failures appear as scheduling/`No valid host`.

**B5**

1. `20 × 10% = 2 TB` reserved.
2. Volumes `30 × 200 GB = 6000 GB = 6 TB`; plus 1.5 TB snapshots = **7.5 TB**.
3. Usable before policy: `20 − 2 = 18 TB`; 80% policy → `18 × 0.8 = 14.4 TB`; minus 7.5 TB = **6.9 TB** available under policy. (If you apply 80% to raw 20 TB = 16 TB, minus 2 reserved minus 7.5 = 6.5 TB; state your assumption. The stricter 6.5 TB is safer.)
4. 8 × 1 TB = 8 TB > 6.5–6.9 TB available → **no** under the 80% policy, unless you free space, expand the backend, or change policy. Thin provisioning may accept the request and fail later; do not rely on overcommit.
5. "20 TB" is raw; reserved space, existing volumes/snapshots, thin overcommit, pool/backend selection, volume type/AZ, and the 80% policy all reduce what a new volume can actually use.

**B6**

1. `500 Mbps / 8 = 62.5 MB/s` theoretical; at 70% = **43.75 MB/s**.
2. `8 GiB = 8192 MiB`; `8192 / 43.75 ≈ 187.2 s`.
3. `187.2 × 0.5 ≈ 93.6 s`.
4. `187.2 + 93.6 + 30 + 60 ≈ 370.8 s` (~6.2 minutes).
5. Yes, a 300 s `--wait` will likely time out. A timeout does **not** prove failure; check `server show`, `server event list`, and the console before deleting. The server may become `ACTIVE` after the client gives up.

### Part C

**C1 — Authentication/scope**

- **Fault domain:** Keystone authentication, likely the copied `openrc` has the teammate's password/project/domain, or the password changed.
- **Next evidence:** `openstack token issue -f yaml`; verify `OS_AUTH_URL`, `OS_USER_DOMAIN_NAME`, `OS_PROJECT_DOMAIN_NAME`, `OS_PROJECT_NAME`, user/project enabled state; check clock skew. Use a fresh `openrc`/`clouds.yaml`.
- **Fix:** use your own credentials/application credential and correct domain/project; re-source. Do not reset the teammate's password or disable MFA.
- **Why:** a 401 is authentication; a wrong domain/project can look like a bad password.

**C2 — Authorization/role**

- **Fault domain:** Keystone/RBAC policy, not quota or capacity.
- **Next evidence:** `openstack role assignment list --user alice --project demo --names`; `openstack token issue`; the service policy for `os_compute_api:servers:create`; `openstack quota show`.
- **Fix:** grant the appropriate `member`/`creator` role on the project (or the policy-required role) via the operator; do not share an admin account. If quota is also involved, the message will name it.
- **Why:** a `reader` role is valid for GET but not create; the 403 is authorization, not authentication.

**C3 — Placement/capacity or filter**

- **Fault domain:** scheduling/Placement, not the hypervisor summary.
- **Next evidence:** `openstack resource provider inventory list <uuid>` and `usage show` (shown full); `openstack allocation candidate list --resource VCPU=8 --resource MEMORY_MB=32768`; `openstack flavor show`, `openstack aggregate list`/AZ, `nova-scheduler.log` filters.
- **Fix:** free capacity/migrate instances, add a host, choose a smaller flavor, or correct the AZ/aggregate/trait. Do not edit Placement rows.
- **Why:** `hypervisor show` can lag or show host totals; Placement's allocations say VCPU 60/64, so an 8-vCPU flavor cannot fit on that host. Fragmentation across hosts can also block.

**C4 — Neutron port binding/agent**

- **Fault domain:** Neutron data plane on `compute1`, not Nova scheduling.
- **Next evidence:** `openstack network agent list`/`show <agent>`; `openstack port show p-1`; `neutron-openvswitch-agent.log`/OVN logs on `compute1`; `ovs-vsctl show`; `systemctl status neutron-openvswitch-agent`.
- **Fix:** restore the agent (process, RabbitMQ/OVS/OVN connectivity), then rebind/retry the port. If the host is down, evacuate/reschedule the VM; do not just restart Nova.
- **Why:** the port is `unbound` and the OVS agent is down, so the VIF cannot be plugged; `task_state=networking` is the stage.

**C5 — Security group/router/floating path**

- **Fault domain:** north-south network/security, not the guest (sshd is running).
- **Next evidence:** `openstack security group rule list <sg>`; `openstack router show` (external gateway); `openstack floating ip show`; on the host `ip netns exec qrouter-... iptables -t nat -L -n -v`, `conntrack`, `tcpdump` on `qg-*`; guest `ss -lntp`.
- **Fix:** allow TCP/22 from the client CIDR, ensure the router has the external gateway and the floating IP is associated with the correct port, and check the external provider mapping. Do not disable the security group.
- **Why:** `ACTIVE` + an associated floating IP only proves NAT state; a missing SG rule or broken router path produces a timeout.

**C6 — Cinder AZ mismatch**

- **Fault domain:** storage topology/AZ, not Cinder API auth.
- **Next evidence:** `openstack volume show vol` (AZ), `openstack server show vm` (AZ), `openstack availability zone list --volume`, `cinder-volume.log`, `openstack volume service list`.
- **Fix:** create a volume in `az-b` (or migrate the VM to `az-a`), then attach. Do not force across AZs; the compute cannot reach the backend LUN in another AZ.
- **Why:** EBS-like block volumes are AZ-scoped; the attach path requires the compute and volume in the same AZ (unless a special replicated backend is configured).

**C7 — Glance image corrupt/unusable**

- **Fault domain:** image backend/upload, not Nova scheduling.
- **Next evidence:** `openstack image show ubuntu-custom -f yaml` (status/size/checksum), Glance API/backend logs, `qemu-img info` if you have the file, `openstack image list --long`, and the server's `fault`/event.
- **Fix:** re-upload a valid image with the correct `disk_format`, verify checksum/size, then recreate the server. Do not retry the same corrupt image; `killed`/size 0 is not bootable.
- **Why:** Nova can schedule and then fail at image download/spawn; the image state is the evidence.

**C8 — Cinder/iSCSI attach path**

- **Fault domain:** compute-to-backend data path (iSCSI/RBD), not the guest filesystem.
- **Next evidence:** `openstack volume show vol` (attachments/status), `cinder-volume.log`, `nova-compute.log`, on the compute `iscsiadm -m session`, `multipath -ll`, `dmesg`, `lsblk`; for RBD `ceph health`/`rbd info`.
- **Fix:** re-establish the iSCSI session/initiator or RBD mapping, repair the backend/network, then rescan/rebind; if the volume state is stale, clean up the attachment via the API before retrying. Do not format a guessed device.
- **Why:** the volume is `in-use` but the guest has only `vda`; the attachment record exists while the data path does not.

### Part D

**D1 — Example landing zone**

1. **Identity:** one domain (or one per security boundary), projects `research`, `platform`, `students`; groups per team; roles `member` (create/use own resources), `reader` (view), `admin` only for platform operators. `platform` gets a project-scoped admin for shared services, not system admin unless required. Role assignments by group, not individual users; SSO/application credentials for automation. Quotas per project.
2. **Network:** non-overlapping CIDRs, e.g. `10.10.0.0/16` research, `10.20.0.0/16` platform, `10.30.0.0/16` students; each project has a tenant network/subnet and a router with an external gateway. Default-deny east-west between projects; routers/security groups enforce policy. SSH only from a bastion/VPN CIDR via SG; no `0.0.0.0/0:22`. Use a shared external network for floating IPs; optionally a shared services network for the platform team. For isolation, separate routers (not a shared router with extra routes) and no shared tenant networks.
3. **Quotas/flavors:** students capped (e.g. 10 instances, 20 vCPU, 40 GB RAM, small flavors only); research higher (e.g. 40 instances, 160 vCPU, 512 GB RAM, GPU flavors); platform sized for shared services. GPU flavors use host aggregates/traits and project-specific quotas to prevent one project consuming all GPUs. Use flavor extra specs for CPU policy/hugepages/GPU; publish only approved flavors.
4. **Images:** platform owns a curated project and public/shared images; `protected=True`; tenants get `shared` visibility or image members, not ownership. Base images are updated on a schedule, checksummed, and tested; tenants can create private snapshots/images but cannot modify the base. Use `min_disk`/`min_ram` and `hw_*` properties.
5. **Storage:** volume types for `standard`, `fast`, `encrypted-research`; encryption via Barbican; `research` uses the encrypted type; snapshots retained 7 days via a policy/automation; backups to independent storage. Use `WaitForFirstConsumer`-equivalent AZ alignment: create volumes in the server AZ; for HA, replicate/backup across AZs or use a backend with multi-AZ replication. State clearly that Cinder volumes are AZ-scoped.
6. **Monitoring/showback:** per-project quotas/usage, Cinder pool capacity, Nova/Placement allocations, Neutron IP usage, API error rates/latency, agent health, volume attach failures. Tag resources (`project`, `owner`, `environment`, `expires`); export usage by project; alert on quota thresholds, GPU exhaustion, backend capacity, and floating IP exhaustion.
7. **Failure modes:** (a) tenant CIDR overlap → routing/floating IP ambiguity; evidence: subnet CIDRs/router. (b) quota exhaustion → API 403 naming the quota; evidence: `quota show`. (c) GPU capacity fragmentation/reservation → jobs pending/No valid host; evidence: Placement inventory/usage, aggregates/traits, scheduler logs. Add: image corruption, Cinder backend full, Neutron IP exhaustion.

**D2 — Example GPU design**

1. **Aggregate/trait/inventory:** a `gpu-a100` host aggregate (or `gpu-h100`) containing only GPU computes; traits such as `CUSTOM_GPU_H100`, `HW_CPU_X86_AVX512`, `HW_NUMA_ROOT`; Placement inventory includes `VCPU`, `MEMORY_MB`, `DISK_GB`, plus `PCI_DEVICE`/`CUSTOM_GPU_H100` (deployment-specific). Each host inventories 8 GPUs, 2 NUMA nodes, 1 TB RAM, 8 TB local NVMe (as `DISK_GB` or a custom resource). Use host aggregates for AZ/tenant isolation and traits for capabilities.
2. **Flavor/image:** flavor extra specs `pci_passthrough:alias=gpu_h100:8`, `hw:cpu_policy=dedicated`, `hw:mem_page_size=large`/`1GB`, `hw:numa_nodes=2`, `hw:cpu_thread_policy=isolate`; image properties `hw_firmware_type=uefi`, `hw_machine_type=q35`, `hw_disk_bus=virtio` or `scsi`, `hw_vif_model=virtio`/`sriov`; ensure the guest image has the GPU driver/CUDA stack or a bootstrap that installs it. The flavor requests, the host must satisfy with traits/inventory/pci whitelist.
3. **Network:** SR-IOV VF ports for RDMA/high-speed east-west; `binding:vnic_type=direct` or `direct-physical`; `port_security_enabled` may need to be false for some RDMA setups (document the risk); MTU must match the physical fabric (e.g. 9000 for RoCE) and the tenant network; use a dedicated provider/tenant network for GPU traffic, separate from management. Validate VF availability and the NIC's `sriov_numvfs`.
4. **Storage:** datasets on shared read-only storage (Manila/CephFS/NFS or Swift/S3), checkpoints on durable Cinder volumes or a parallel filesystem; local NVMe for scratch only (ephemeral, lost on move). Cinder for per-job scratch/checkpoints if the job is node-local; Manila/CephFS for shared multi-node training. Size IOPS/throughput, not just GB; separate the dataset network from the management network where possible.
5. **Identity/quota:** `research-gpu` project with `member` for researchers, `reader` for observers, `admin` only for operators; GPU quota via project quota/aggregate reservations; flavor access restricted to the project; no admin credentials for researchers; application credentials for job submission. Use a reservation/lease model if the cloud supports it, or separate projects per lab to cap consumption.
6. **Scheduler/evidence:** enable `PciPassthroughFilter`, `NUMATopologyFilter`, `AggregateInstanceExtraSpecsFilter`/`AggregateMultiTenancyIsolation`, `AvailabilityZoneFilter`; use host aggregates/traits. If a job is pending/error, check `openstack flavor show` extra specs, Placement inventory/traits/usage, PCI whitelist on the host, `nova-scheduler.log` filter messages, and `server event list`. Do not just resize the flavor.
7. **GPU failure modes:** (a) GPU not visible in the guest → PCI passthrough/IOMMU/whitelist/driver; check `lspci`, `nvidia-smi`, host `dmesg`, libvirt XML. (b) NUMA/CPU mismatch → job slower or fails to schedule; check `numactl -H`, flavor extra specs, `NUMATopologyFilter`. (c) RDMA/SR-IOV VF unavailable or MTU wrong → NCCL hangs/timeouts; check `ibv_devinfo`, `ip link`, VF count, MTU, and fabric counters. Add: GPU exhaustion/fragmentation, image driver mismatch, local NVMe data loss on migration.

---

## Scoring

| Section | Points | Your score | Target |
|---|---:|---:|---:|
| A — facts | 50 | | ≥ 40 |
| B — calculations | 30 | | ≥ 24 |
| C — diagnosis | 40 | | ≥ 32 |
| D — design | 40 | | ≥ 32 |
| **Total** | **160** | | **≥ 128 (80%)** |

If you score below 80%, re-read the specific note, redo L6.1–L6.4, and retake after 48 hours. A conceptual miss in Keystone scope, Placement, or Neutron routing will compound in the AI infrastructure phase; fix it before adding GPU scheduling.

---

## Practical sign-off

Do these in a sandbox project/DevStack, without notes. Each is pass/fail. If you have no cloud, do the read-only L6.1 and a paper walkthrough for the rest; mark those **not executed**.

- [ ] Prove your Keystone scope, roles, catalog, and one endpoint with `openstack token issue`/`catalog list`/`endpoint list`.
- [ ] Explain a 401 vs a 403 you reproduced, and the exact evidence for each.
- [ ] Map one compute host, one network agent, one volume backend, and one Placement provider.
- [ ] Create a tenant network/subnet/router with an external gateway; prove the external gateway in `router show`.
- [ ] Launch a VM, prove the port binding, associate a floating IP, and SSH/console into it.
- [ ] Prove metadata works from the guest (or explain config drive if it does not).
- [ ] Break SSH with a security-group rule, collect evidence, and restore it without rebooting the VM.
- [ ] Create/attach/format/mount a Cinder volume; write data; detach safely.
- [ ] Snapshot the volume, restore to a new volume, attach it, and verify the data.
- [ ] Extend a volume and grow the filesystem; explain `lsblk` vs `df`.
- [ ] Diagnose a launch failure from `server show`/`server event list`/Placement evidence; name the stage.
- [ ] Diagnose an `ACTIVE`-but-unreachable VM from port/router/SG/floating-IP evidence.
- [ ] Diagnose an invalid volume attach (in-use or AZ mismatch) from volume state/logs.
- [ ] Clean up every VM, volume, snapshot, floating IP, router, network, and keypair; prove it.

**Cleanup proof:** paste the empty `server list`, `volume list`, `volume snapshot list`, `floating ip list`, `router list`, `network list`, and lab keypair output. A screenshot is weaker than command output.

---

## Oral exam (12 minutes)

Answer out loud, without notes. Record yourself. If you use a term you cannot define, stop and define it.

1. "Walk me through `openstack server create` from the CLI to a running VM."
2. "A user gets a 401; another gets a 403. What is the difference, and what do you check?"
3. "A VM is `ACTIVE` but SSH times out. Walk me through the packet path."
4. "Why can a flavor not schedule when the hypervisor shows free CPU and RAM?"
5. "Explain a Neutron port, a router, and a floating IP in one story."
6. "What does Placement know that the hypervisor summary does not?"
7. "A volume is `available` but attach fails. What are the first three checks?"
8. "What is the difference between a Cinder snapshot and a backup, and when is each appropriate?"
9. "How would you design a GPU flavor and host aggregate on OpenStack?"
10. "Tell me about a failure you caused in these labs, the evidence you used, and what you changed in your process."

---

## Retest log

| Date | A | B | C | D | Total | Weak areas | Remediation | Retest date |
|---|---:|---:|---:|---:|---:|---|---|---|
| | | | | | | | | |
| | | | | | | | | |

When you pass, continue to [Phase 7 — Automation](../07-automation/README.md). The next phase turns the manual commands into repeatable, idempotent operations; if a command only works because you remembered the flags, automate it and document the failure modes.
