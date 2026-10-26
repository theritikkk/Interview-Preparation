# OpenStack Troubleshooting Playbook

> **The one-sentence model:** debug OpenStack in layers — client/auth, API/DB, scheduler/Placement, agent/backend, guest — and use the request ID to correlate one request across services. Restarting services is a change, not a diagnosis.

---

## 1. Triage card

Answer these before running a command:

1. **Who/what:** Which project, user, and server/volume/network ID? Is the client CLI, SDK, Horizon, or an automated job?
2. **When:** When did it last work? What changed (deploy, quota, network, image, host maintenance, certificate, password)?
3. **Where:** Which region, AZ, host, and service? Is it one VM, one project, one host, or everyone?
4. **Which plane:** Is the API failing (4xx/5xx before a resource ID), scheduling failing (resource stuck `BUILD`), or the data plane failing (`ACTIVE` but unreachable)?
5. **What is the evidence:** Exact error text, request ID, resource ID, timestamp, and the command that produced it.

A useful opening:

```bash
openstack token issue
openstack server show <server> -f yaml
openstack server event list <server>
openstack volume show <volume> -f yaml
openstack port list --server <server>
```

If you do not have an ID, get one from the user. "It doesn't work" is not a fault domain.

---

## 2. Evidence hierarchy

Work from the client inward; do not start at the deepest layer.

```text
1. Client command + exact error
2. Keystone token/scope/catalog
3. API service log + DB state (resource status/fault)
4. Scheduler/Placement decision
5. Agent on the host (compute/network/volume)
6. Data plane (libvirt, OVS/OVN, iSCSI/RBD, namespace)
7. Guest OS (console, cloud-init, routes, firewall)
```

Each layer can lie about the next:

- The API can return `202` while the scheduler later fails.
- The scheduler can choose a host while the agent cannot bind a port.
- The port can be `ACTIVE` while the guest has no route.
- The volume can be `in-use` while the guest does not see the device.

### Request ID correlation

```bash
openstack --debug server show <server> 2>&1 | grep -i x-openstack-request-id
```

Then search the service logs for that ID:

```bash
sudo grep -R "req-<id>" /var/log/nova /var/log/neutron /var/log/cinder
```

The request ID is more reliable than a timestamp, especially across time zones and load-balanced API nodes.

---

## 3. First ten commands

```bash
openstack token issue
openstack server show <server> -f yaml
openstack server event list <server>
openstack console log show <server>
openstack port list --server <server>
openstack port show <port> -f yaml
openstack volume show <volume> -f yaml
openstack compute service list
openstack network agent list
openstack volume service list
```

Then add service-specific evidence:

```bash
openstack image show <image> -f yaml
openstack flavor show <flavor> -f yaml
openstack resource provider list
openstack resource provider inventory list <uuid>
openstack resource provider usage show <uuid>
openstack allocation candidate list --resource VCPU=2 --resource MEMORY_MB=2048
openstack router show <router> -f yaml
openstack floating ip show <ip>
openstack security group rule list <sg>
openstack ip availability show <network-id>
```

---

## 4. Service-by-service triage

### Keystone / authentication

| Symptom | Check | Likely cause |
|---|---|---|
| 401 on every call | Auth URL, domain, project, password, user/project enabled, clock | Wrong scope/credential; expired token |
| 403 after auth | Token roles, role assignments, policy/scope | Missing role; system vs project scope |
| 404 on a service | `endpoint list`, `catalog show`, region/interface | Wrong endpoint/region; service not registered |
| Works in Horizon, not CLI | Project/region/auth method | Different scope or cloud |
| 401 in service logs | Service user role/assignment/endpoint | Service-to-service auth broken |

```bash
openstack token issue -f yaml
openstack role assignment list --names --project <project>
openstack endpoint list --service <service>
openstack --debug <command>
```

### Glance / images

```bash
openstack image show <image> -f yaml
openstack image list --long
```

Check `status`, `disk_format`, `container_format`, `size`, `checksum`, `min_disk`, `min_ram`, `visibility`, `owner`, `protected`. A `queued`/`saving` image has no usable bytes; a `killed` image is corrupt/unusable; a private image in another project is invisible. Glance API/backend logs are the next step.

### Nova / compute

```bash
openstack server show <server> -f yaml
openstack server event list <server>
openstack compute service list
openstack hypervisor list
openstack hypervisor show <host> -f yaml
```

Check `status`, `task_state`, `fault`, host, AZ, image, flavor, addresses, volumes. `server event list` identifies the failing action. `compute service list` distinguishes a down/disabled compute from a scheduler issue. A `down` compute may still run existing VMs.

### Placement / scheduling

```bash
openstack resource provider list
openstack resource provider inventory list <uuid>
openstack resource provider usage show <uuid>
openstack allocation candidate list --resource VCPU=2 --resource MEMORY_MB=2048
```

Check total/reserved/ratio vs used, traits, aggregates, and `COMPUTE_STATUS_DISABLED`. Empty candidates mean Placement cannot satisfy the request; non-empty candidates plus a scheduler failure means a Nova filter or a later stage.

### Neutron / networking

```bash
openstack network agent list
openstack port show <port> -f yaml
openstack router show <router> -f yaml
openstack floating ip show <ip>
openstack ip availability show <network-id>
```

Check port binding (`binding:vif_type`, `binding:host_id`, `status`), agent alive/state, router external gateway, floating IP association, subnet free IPs, security groups. On the host: `ip netns`, `ovs-vsctl show`, `ovs-ofctl dump-flows`, `tcpdump`, `conntrack`.

### Cinder / storage

```bash
openstack volume show <volume> -f yaml
openstack volume service list
openstack volume snapshot list
openstack volume backup list
cinder get-pools --detail
```

Check `status`, AZ, type, attachments, `multiattach`, backend service state, pool capacity. On the compute: `iscsiadm -m session`, `multipath -ll`, `lsblk`, `dmesg`; for Ceph: `ceph health`, `rbd info`.

### RabbitMQ / database / shared services

If multiple unrelated services are slow or return 500s, check the message bus and DB before chasing each service:

```bash
sudo systemctl status rabbitmq-server mariadb
sudo rabbitmqctl status
sudo rabbitmqctl list_queues name messages consumers
sudo journalctl -u rabbitmq-server -n 100
```

A full queue, a down DB, or a disk-full controller can look like "Nova is broken" and "Neutron is broken" at the same time.

---

## 5. Log map

Paths vary by distribution and DevStack. Common locations:

| Service | Logs |
|---|---|
| Keystone | `/var/log/keystone/keystone.log` |
| Glance | `/var/log/glance/glance-api.log` |
| Nova API | `/var/log/nova/nova-api.log` |
| Nova scheduler | `/var/log/nova/nova-scheduler.log` |
| Nova conductor | `/var/log/nova/nova-conductor.log` |
| Nova compute | `/var/log/nova/nova-compute.log` |
| Neutron server | `/var/log/neutron/neutron-server.log` |
| OVS agent | `/var/log/neutron/openvswitch-agent.log` |
| L3 agent | `/var/log/neutron/l3-agent.log` |
| DHCP agent | `/var/log/neutron/dhcp-agent.log` |
| Metadata agent | `/var/log/neutron/metadata-agent.log` |
| Cinder API | `/var/log/cinder/cinder-api.log` |
| Cinder scheduler | `/var/log/cinder/cinder-scheduler.log` |
| Cinder volume | `/var/log/cinder/cinder-volume.log` |
| Cinder backup | `/var/log/cinder/cinder-backup.log` |
| RabbitMQ | `/var/log/rabbitmq/rabbit@<host>.log` |
| DevStack (often) | `/opt/stack/logs/*.log` |

```bash
sudo journalctl -u nova-compute -n 200 --no-pager
sudo journalctl -u neutron-openvswitch-agent -n 200 --no-pager
sudo tail -f /var/log/nova/nova-compute.log
```

Read the **last error before the traceback**, not just the top of the stack. Correlate timestamps and request IDs.

---

## 6. Case studies

### Case 1 — `ACTIVE` VM, SSH times out

**Evidence:**

```text
openstack server show vm: status ACTIVE, addresses opslab-net=10.0.100.12
openstack floating ip list: 203.0.113.45 associated with the VM port
nc -vz 203.0.113.45 22: timeout
openstack console log show vm: guest booted, sshd listening
```

**Fault domain:** north-south path/security, not the VM process. **Next:** `openstack router show` (external gateway), `openstack security group rule list`, router namespace DNAT/conntrack, security group, `tcpdump` on `qg-*`. **Fix:** add the missing SG rule or router gateway/floating IP association; do not reboot the VM first.

### Case 2 — `No valid host` while the hypervisor shows free CPU

**Evidence:**

```text
openstack server show big: ERROR, No valid host was found
openstack hypervisor show compute1: 64 vCPU, 8 used
openstack resource provider usage show <uuid>: VCPU 60/64
openstack allocation candidate list --resource VCPU=8: empty
```

**Fault domain:** Placement capacity/fragmentation or a filter. The hypervisor summary is stale/aggregate; Placement knows the real allocations. **Next:** check inventory/usage, flavor extra specs, AZ/aggregate, scheduler log filter names. **Fix:** free capacity, add a host, or choose a flavor that fits; do not edit Placement.

### Case 3 — Volume `available`, attach fails

**Evidence:**

```text
openstack volume show vol: available, availability_zone az-a
openstack server show vm: OS-EXT-AZ:availability_zone az-b
openstack server add volume vm vol: error
```

**Fault domain:** AZ mismatch. **Next:** `openstack availability zone list --volume`, `openstack volume show`, `cinder-volume.log`. **Fix:** create a volume in `az-b` or migrate the VM; do not force-attach across AZs.

### Case 4 — Everything 401 after a Keystone restart

**Evidence:** all clients fail with 401; Keystone logs show token validation errors; the restart changed the Fernet key set or a load-balanced Keystone node has a different key. **Fault domain:** Keystone token keys. **Next:** check Keystone key repository/permissions and all API nodes; reissue a token. **Fix:** restore consistent keys/sync them; do not reset every user's password.

### Case 5 — Floating IP stops after an L3 failover

**Evidence:** the router is HA; one L3 agent restarted; floating IP pings fail; `openstack network agent list` shows one agent down; the router namespace moved. **Fault domain:** L3 HA/namespace. **Next:** check agent state, VRRP/keepalived logs, the router's `ha` flag, and the external network on the standby host. **Fix:** restore the agent/network; failover should recover automatically. Do not delete the router.

### Case 6 — Intermittent 500s from one API

**Evidence:** some requests succeed, some 500; the load balancer has multiple API nodes; one node has a full disk, bad config, or cannot reach RabbitMQ. **Fault domain:** one API instance, not the whole service. **Next:** correlate the request ID with the API node logs, check disk/queue/DB from each node. **Fix:** remove the bad node from the LB, repair it, return it. Do not restart all API nodes at once.

---

## 7. Escalation packet

When you hand off to an operator, include:

- Project name/ID and user (redact secrets).
- Resource type/ID (server, volume, port, router, image).
- Exact command and full error text.
- Timestamp/time zone and request ID.
- Region/AZ/host if known.
- What changed and what you already tried.
- Evidence: `server show`, `server event list`, `port show`, `volume show`, relevant log lines.

Do not paste a raw `--debug` dump with tokens. Redact `X-Auth-Token`, passwords, and secret keys.

---

## 8. Change safety

- **One change at a time.** If you restart three services and it recovers, you learned nothing.
- **Prefer disable over delete.** `compute service set --disable` stops new scheduling without destroying state.
- **Do not force-detach a mounted volume.** Unmount/quiesce first.
- **Do not delete a router/network with active ports.** It can strand VMs and floating IPs.
- **Do not edit the database.** Use the API; if the API cannot express the recovery, escalate.
- **Snapshot before risky changes** (image, volume, VM rebuild), and verify the snapshot is usable.
- **Record the before/after.** A screenshot is not a substitute for command output.

---

## 9. Checkpoint

1. What are the five layers you check in order?
2. What does a request ID correlate?
3. What is the difference between a 401 and a 403?
4. Why is `ACTIVE` not proof of reachability?
5. What does `server event list` show that `server show` does not?
6. What does `openstack port show` tell you about the data plane?
7. What does `resource provider usage show` tell you that `hypervisor show` does not?
8. What is the first check for a volume attach failure?
9. What is the first check for a floating IP timeout?
10. Why can one API node cause intermittent 500s?
11. What should you include in an escalation packet?
12. Why is restarting every service a bad first step?
13. What is the safest way to stop new instances on a compute?
14. Why should you not edit the database?
15. What would you snapshot before a risky volume change?

### Answers

1. Client/auth → API/DB → scheduler/Placement → agent/backend → data plane/guest.
2. One client request across services/logs, independent of timestamp/time zone.
3. 401 = not authenticated (credentials/token/scope); 403 = authenticated but not authorized by policy/role/scope.
4. The domain is running; the guest may be stuck at boot, have no route/floating IP, be blocked by an SG, or have failed cloud-init.
5. The lifecycle actions/results, which identify the failing stage (schedule, network, spawn, volume, power-on).
6. Whether the port is bound, to which host, with which VIF type; `unbound` means the data plane has not claimed it.
7. Real allocations vs total inventory; `hypervisor show` can lag or show aggregate host data, while Placement is the scheduler's ledger.
8. AZ match, volume status/attachments, backend service, cinder/nova logs, iSCSI/RBD.
9. Router external gateway, floating IP association, security group, DNAT/conntrack, external network mapping.
10. The load balancer may route some requests to a bad node (disk/config/RabbitMQ/DB), while other nodes serve successfully.
11. Project/resource IDs, exact command/error, timestamp/request ID, region/AZ/host, changes, evidence/logs (redacted).
12. It can widen the outage and destroy evidence; change one thing at a time.
13. `openstack compute service set --disable <host> nova-compute`.
14. It bypasses validation/notifications and can corrupt state; use the API or escalate.
15. A volume snapshot/backup (and verify it), plus application consistency/quiesce if needed.
