# Neutron Networking

> **The one-sentence model:** Neutron turns a declarative network/subnet/port request into a set of Linux namespaces, OVS/OVN flows, and iptables/nftables NAT rules; a VM can have an IP and still have no path because the port is unbound, the router has no external gateway, a security group drops the packet, or the MTU is wrong.

---

## 1. The objects

| Object | What it is | Key fields |
|---|---|---|
| **Network** | An L2 broadcast domain | `provider:network_type`, `provider:physical_network`, `provider:segmentation_id`, `mtu`, `shared` |
| **Subnet** | An IPv4/IPv6 CIDR on a network | `cidr`, `gateway_ip`, `allocation_pools`, `dns_nameservers`, `ip_version` |
| **Port** | A virtual NIC attachment point | MAC, fixed IPs, security groups, `binding:vnic_type`, `binding:host_id`, `status` |
| **Router** | An L3 forwarding/NAT device | `external_gateway_info`, interfaces, `ha`, `distributed`, extra routes |
| **Floating IP** | A 1:1 NAT from an external IP to a fixed IP | external network, port/fixed IP, router |
| **Security group** | Stateful packet filter on ports | ingress/egress rules, remote group/IP, protocol/port |
| **Network agent** | Host process that implements the data plane | DHCP, L3, OVS/OVN, metadata, L2 |

```bash
openstack network list
openstack subnet list
openstack port list
openstack router list
openstack floating ip list
openstack security group list
```

### Tenant vs provider networks

- **Tenant network:** created by a project, usually overlay (VXLAN/Geneve) or VLAN; the project controls subnets and ports. It needs a router for external connectivity.
- **Provider network:** mapped to a physical network and often external/public; typically created by an operator with `--provider:*` attributes. A VM attached directly to a provider network gets an address from that physical network and does not need a floating IP if the address is routable.

```bash
# Tenant network (overlay)
openstack network create opslab-net
openstack subnet create opslab-subnet \
  --network opslab-net \
  --subnet-range 10.0.100.0/24 \
  --gateway 10.0.100.1 \
  --dns-nameserver 8.8.8.8

# Inspect provider attributes
openstack network show public -f yaml
openstack network show opslab-net -c provider:network_type -c mtu
```

A network's `shared` flag lets other projects attach to it; it does not make it routable. A `router:external` network is an external network, not automatically a tenant network.

---

## 2. Ports, DHCP, and metadata

### The port is the contract

A Nova VM does not attach to a network directly; it attaches to a **port**. The port holds the MAC, fixed IP, security groups, and binding state.

```bash
openstack port list --server opslab-vm
openstack port show <port> -f yaml
```

Example:

```yaml
status: ACTIVE
binding:vif_type: ovs
binding:vif_details:
  port_filter: true
binding:host_id: compute1
fixed_ips:
- ip_address: 10.0.100.12
  subnet_id: ...
mac_address: fa:16:3e:...
security_groups:
- ...
```

`binding:vif_type: unbound` means the port is not bound to a host; the VM is not wiring yet. `status: DOWN` can be normal before binding; `ACTIVE` means the plug succeeded. `binding:host_id` tells you which compute/agent should have the tap/OVS port.

### DHCP

The DHCP agent runs `dnsmasq` in a namespace named `qdhcp-<network-id>`:

```bash
ip netns list | grep qdhcp
sudo ip netns exec qdhcp-<network-id> ip addr
sudo ip netns exec qdhcp-<network-id> ps aux | grep dnsmasq
```

A VM gets its fixed IP from the port; DHCP supplies the address, gateway, DNS, and MTU. If the port has a fixed IP but the guest has no lease, check the DHCP agent, the namespace, and the subnet's allocation pool.

```bash
openstack network agent list
openstack network agent show <agent-id>
openstack ip availability list
openstack ip availability show <network-id>
```

### Metadata

Cloud-init and cloudbase-init fetch instance metadata from `169.254.169.254`. Neutron's metadata agent proxies the request to Nova's metadata API:

```text
guest → 169.254.169.254 → qrouter/qdhcp namespace → neutron-metadata-agent
      → nova-api-metadata → instance metadata
```

If the guest has no route to `169.254.169.254` (isolated network with no router), use **config drive** (`--config-drive true`) or provide a metadata route. A metadata failure looks like a VM that boots but never sets its SSH key, hostname, or user-data.

```bash
openstack network agent list | grep -i metadata
# Inside the guest:
# curl -s http://169.254.169.254/latest/meta-data/instance-id
# cloud-init status --long
```

---

## 3. Routers and floating IPs

### The north-south path

```text
VM (10.0.100.12)
  │ tap/veth
  ▼
br-int ── security group/conntrack ── qrouter namespace
  │                                      │
  │                                      ├── SNAT: 10.0.100.0/24 → router gateway IP
  │                                      └── DNAT: floating IP → 10.0.100.12
  ▼
br-ex / provider bridge ── external network ── internet
```

A tenant router:

```bash
openstack router create opslab-router
openstack router add subnet opslab-router opslab-subnet
openstack router set opslab-router --external-gateway public
openstack router show opslab-router -f yaml
```

`external_gateway_info` must contain the external network and an external fixed IP (or `enable_snat: true`). Without it, the router has no default route to the outside; floating IPs cannot be associated.

### Floating IP association

```bash
openstack floating ip create public
openstack floating ip list
openstack server add floating ip opslab-vm <floating-ip>
# or bind directly to a port:
openstack floating ip set --port <port-id> <floating-ip>
```

A floating IP is DNAT inbound and SNAT outbound for that fixed IP. It is not a second interface in the guest. If the guest has multiple ports, specify which fixed IP/port the floating IP maps to.

### Router namespaces

```bash
ip netns list | grep qrouter
sudo ip netns exec qrouter-<router-id> ip addr
sudo ip netns exec qrouter-<router-id> ip route
sudo ip netns exec qrouter-<router-id> iptables -t nat -L -n -v
sudo ip netns exec qrouter-<router-id> conntrack -L
```

DVR deployments may have `snat-*` and `fip-*` namespaces per compute. The exact namespace names depend on the driver; the principle is the same: the router is a Linux namespace with interfaces, routes, and NAT rules.

### HA and DVR

- **HA router:** multiple L3 agents host the router with VRRP; one active, one standby. A failover can briefly drop floating IP connectivity.
- **DVR:** the DNAT/floating-IP path is distributed to the compute node; SNAT may remain centralized. It reduces tromboning but adds complexity.
- **Centralized router:** all north-south traffic traverses the router's L3 agent host; that host is a bandwidth/latency bottleneck and a failure point.

For AI workloads, east-west traffic (GPU-to-GPU, storage) often dominates; do not assume the north-south router design is the right topology for training traffic.

---

## 4. Security groups and port security

A security group is stateful: allowing inbound TCP 22 automatically allows the response. Default egress is usually allow-all; default ingress is deny.

```bash
openstack security group create opslab-sg
openstack security group rule create --proto tcp --dst-port 22 \
  --remote-ip 203.0.113.10/32 opslab-sg
openstack security group rule create --proto icmp opslab-sg
openstack security group rule list opslab-sg
```

Rules can reference another security group (`--remote-group`) instead of a CIDR. That is how you allow `web` → `db` without hard-coding addresses.

`port_security_enabled` controls anti-spoofing and security-group enforcement on a port. Disabling it allows arbitrary MAC/IPs and is usually a bad idea outside a router/appliance port. `allowed_address_pairs` lets a port legitimately use additional IPs (e.g., a VIP).

```bash
openstack port show <port> -c port_security_enabled -c allowed_address_pairs
```

**Security groups are not a substitute for network segmentation.** They are enforced at the port/OVS layer; a compromised host or a misconfigured port-security setting can bypass assumptions.

---

## 5. ML2, OVS, and OVN

### ML2

Modular Layer 2 (ML2) separates:

- **Type drivers:** how the network is encapsulated on the wire (`flat`, `vlan`, `vxlan`, `gre`, `geneve`).
- **Mechanism drivers:** how the host implements it (`openvswitch`, `linuxbridge`, `ovn`, `sriovnicswitch`).

A typical OVS deployment has:

```text
br-ex  ── physical NIC / external network
  │
br-int ── integration bridge; all tenant ports and tunnels
  │
br-tun ── VXLAN/GRE tunnel bridge (OVS)
```

```bash
sudo ovs-vsctl show
sudo ovs-ofctl dump-flows br-int
sudo ovs-ofctl dump-flows br-tun
ip -br link
```

### OVN

OVN replaces the OVS agent/L3/DHCP agents with a central northbound/southbound database and per-host `ovn-controller`. Diagnostic tools differ:

```bash
ovn-nbctl show
ovn-sbctl show
ovn-trace --detailed <datapath> <flow>
```

You do not need to master OVN flow syntax for this role, but you must know whether your cloud uses OVS or OVN before you run `ovs-vsctl` commands. Asking the wrong data plane for evidence wastes time.

### East-west path

```text
VM A tap → br-int → (same network) → br-int → VM B tap
                 └─ (different network) → router namespace → br-int → VM B
```

For VXLAN, the frame is encapsulated on `br-tun`, sent to the remote VTEP, decapsulated on the peer, and delivered to `br-int`. A missing tunnel, wrong VTEP IP, or MTU problem breaks east-west traffic even when both VMs are `ACTIVE`.

### MTU

Overlay encapsulation adds overhead:

- VXLAN: 50 bytes (outer IPv4 + UDP + VXLAN) typical.
- Geneve: 50+ bytes depending on options.
- If the underlay MTU is 1500, the tenant MTU should be 1450; if the underlay is 9000, tenant 8950.

```bash
openstack network show opslab-net -c mtu
ip link show <tap>
ping -M do -s 1472 <peer>   # 1500-byte IP packet; adjust for overhead
```

A classic symptom: SSH connects, small pings work, but large transfers hang. Check MTU/MSS, not the security group.

---

## 6. Command field manual

### `openstack network agent list`

```bash
openstack network agent list
```

**Interpretation:** `Alive` (heartbeat), `State` (`UP`/`DOWN`), agent type, host, binary. A DHCP agent down means no new leases on its networks; an L3 agent down means routers on that host lose north-south connectivity; an OVS agent down means ports on that host cannot bind.

### `openstack port show`

```bash
openstack port show <port> -f yaml
```

**Interpretation:** `status`, `binding:vif_type`, `binding:host_id`, `fixed_ips`, `mac_address`, `security_groups`, `port_security_enabled`, `allowed_address_pairs`. `unbound` means no host has claimed the port.

### `openstack router show`

```bash
openstack router show <router> -f yaml
```

**Interpretation:** interfaces, `external_gateway_info` (network, external IP, SNAT), `routes`, `ha`, `distributed`. A router with no external gateway cannot provide floating IPs or default egress.

### `openstack floating ip list`

```bash
openstack floating ip list
openstack floating ip show <ip>
```

**Interpretation:** `floating_ip_address`, `fixed_ip_address`, `port_id`, `router_id`, `status`. An unassociated floating IP is allocated but not NATed to anything.

### `openstack security group rule list`

```bash
openstack security group rule list <sg>
```

**Interpretation:** direction, ethertype, protocol, port range, remote IP/group. Remember that egress may be allow-all by default; a "blocked" connection may be the return path or the guest firewall.

### `ip netns` / `ovs-vsctl`

```bash
ip netns list
sudo ip netns exec qrouter-<id> ip route
sudo ovs-vsctl show
```

**Why:** these are the data-plane truth on the host. The Neutron DB can say a router exists while the namespace is missing or has no route.

### `openstack ip availability`

```bash
openstack ip availability list
openstack ip availability show <network-id>
```

**Interpretation:** total/free IPs per subnet. A subnet can be exhausted even if the network looks healthy; new ports fail with `IP allocation` errors.

---

## 7. Failure scenarios

### A — VM has an IP but cannot reach the gateway

Fault domain: port binding, security group, route, or OVS/OVN. Check:

1. `openstack port show <port>` — `binding:vif_type` and `binding:host_id`.
2. `openstack network agent list` — is the host's OVS/OVN agent alive?
3. Security group egress/ingress and guest firewall.
4. Subnet gateway and `ip route` inside the guest.
5. On the host, `ovs-vsctl show`, `ovs-ofctl dump-flows br-int`, and the tap interface.
6. `tcpdump -ni <tap>` on the host to see whether ARP/ICMP leaves the guest.

A port can be `ACTIVE` while the guest has no default route; `ACTIVE` is the plug state, not end-to-end reachability.

### B — No DHCP lease / no IP in the guest

Fault domain: DHCP agent/namespace or subnet allocation. Check `openstack network agent list` for the DHCP agent, `ip netns list | grep qdhcp`, the dnsmasq process/log, `openstack ip availability show`, and the port's fixed IP. If the subnet has no free addresses, the port may fail to create or the VM may get no lease.

### C — Cloud-init never runs / no SSH key

Fault domain: metadata path. Check `openstack network agent list` for the metadata agent, the guest's route to `169.254.169.254`, the router/DHCP namespace, `nova-api-metadata`, and whether config drive was used. In an isolated network with no router, use `--config-drive true` or add a metadata route.

### D — Floating IP is allocated but SSH times out

Fault domain: router/external gateway, DNAT, security group, or external mapping. Check:

- `openstack router show` — external gateway present, SNAT enabled;
- `openstack floating ip show` — associated with the right port/fixed IP;
- security group allows 22 from your source;
- router namespace has the floating IP and DNAT rule;
- `conntrack`/`tcpdump` on `qg-*`/`fg-*` interfaces;
- external network provider mapping and ARP/proxy-ARP;
- the guest is listening (`ss -lntp`).

A floating IP is not a public IP inside the guest; the guest still sees its fixed IP. Do not configure the floating IP on `eth0`.

### E — East-west fails between two VMs on the same network

Fault domain: security group, OVS/OVN tunnel, or MTU. Check the ports' security groups, the agents on both hosts, `br-tun`/VXLAN endpoints, `ovs-ofctl dump-flows`, and `ping -M do` with large packets. If the VMs are on different networks, check the router and both subnets.

### F — Port binding fails

```text
binding:vif_type: unbound
```

Check the agent on the target host, the VIF type (OVS vs SR-IOV vs direct), the physical network mapping, and whether the host is in the right availability zone/aggregate. For SR-IOV, check VF availability and the NIC's `sriov_numvfs`.

### G — Overlapping CIDRs

Two tenant networks can use the same CIDR in different projects, but a router cannot route between overlapping subnets, and floating IPs/peering may be ambiguous. Plan non-overlapping address space per project/region; re-IP or use a NAT/PrivateLink-like design if you inherit an overlap.

### H — MTU black hole

Small packets work, large transfers stall. Check the network MTU, the guest MTU, the underlay MTU, and TCP MSS. A VXLAN overlay on a 1500-byte underlay must use a smaller tenant MTU (commonly 1450) or clamp MSS. Do not "fix" it by disabling security groups.

### I — Router HA failover

Floating IP connectivity drops briefly during a failover; the router namespace moves to another L3 agent. Check `openstack network agent list`, the router's `ha` flag, VRRP/keepalived logs, and whether the external network is reachable from the standby host.

---

## 8. Checkpoint

1. What is the difference between a tenant network and a provider network?
2. What does a port represent, and why does Nova attach a VM to a port rather than a network?
3. What does `binding:vif_type: unbound` mean?
4. What is the difference between `status: DOWN` and `status: ACTIVE` on a port?
5. What does the DHCP agent do, and where does it run?
6. How does a guest reach the metadata service, and what breaks it?
7. What is the difference between a router's external gateway and a floating IP?
8. What is SNAT vs DNAT in a Neutron router?
9. Why can a floating IP be allocated but not reachable?
10. What is the difference between ML2 type and mechanism drivers?
11. What is the east-west path for two VMs on the same VXLAN network?
12. Why does VXLAN require MTU planning?
13. What does a security group do that a router does not?
14. What is `port_security_enabled`, and why is disabling it risky?
15. What does an L3 agent do, and what happens when it dies?
16. What does an OVS agent do, and what happens when it dies?
17. What is the first check for a VM with an IP but no gateway connectivity?
18. What is the first check for a floating IP timeout?
19. What is the first check for cloud-init metadata failure?
20. Why can two VMs be `ACTIVE` and still not communicate?

### Answers

1. A tenant network is project-owned, usually overlay/VLAN, and needs a router for external access; a provider network is mapped to a physical network (often external/public) and can be attached directly.
2. A port is the virtual NIC binding point holding MAC/IP/security groups/binding state; Nova needs a stable attachment object to plug into a host and to apply policy.
3. The port has not been bound to a host/data-plane VIF; the VM's networking is not wired yet.
4. `DOWN` is often the initial/unbound state; `ACTIVE` means the port is bound/plugged. Neither proves end-to-end reachability.
5. It runs dnsmasq in a `qdhcp-<network>` namespace and hands out IP/gateway/DNS/MTU to guests.
6. The guest requests `169.254.169.254`; the router/DHCP namespace forwards to `neutron-metadata-agent`, which proxies to Nova metadata. A missing route, dead agent, isolated network without config drive, or wrong secret breaks it.
7. The external gateway connects the router to an external network and provides SNAT/default route; a floating IP is a 1:1 DNAT/SNAT mapping from an external address to a fixed IP through that router.
8. SNAT rewrites the source to the router's external address for outbound traffic; DNAT rewrites the destination from the floating IP to the guest's fixed IP for inbound traffic.
9. No external gateway, no route to the external network, no association to a port/fixed IP, security group, external provider mapping/ARP, or a dead L3 agent.
10. Type drivers define the encapsulation (flat/VLAN/VXLAN/Geneve); mechanism drivers implement it on the host (OVS/Linux bridge/OVN/SR-IOV).
11. VM A tap → br-int → br-tun → VXLAN tunnel → br-tun → br-int → VM B tap, with security-group/conntrack processing along the path.
12. Encapsulation adds overhead; if the underlay is 1500 and the tenant MTU is 1500, large packets are dropped/fragmented. Plan tenant MTU and MSS.
13. Security groups filter traffic at the port (stateful, per-VM); routers forward/NAT between subnets. A router does not enforce per-workload security policy by itself.
14. It enables anti-spoofing and security-group enforcement on the port. Disabling it lets the port use arbitrary MAC/IPs, which can bypass isolation or impersonate another host.
15. It hosts router namespaces and NAT/floating-IP forwarding on a host; if it dies, routers on that host lose north-south connectivity (HA may fail over).
16. It programs the OVS bridges/flows for ports and tunnels on a host; if it dies, new ports cannot bind and existing flows may be stale.
17. Check the port binding/agent, security groups, guest route, and OVS/OVN data plane; `ACTIVE` does not prove reachability.
18. Check the router's external gateway, floating IP association, security group, DNAT/conntrack, and external network mapping.
19. Check the metadata agent, route to `169.254.169.254`, router/DHCP namespace, and whether config drive is needed.
20. They can be on different networks without a router, blocked by security groups, have a broken tunnel/MTU, or be bound to a dead agent; `ACTIVE` is only the VM/port state.
