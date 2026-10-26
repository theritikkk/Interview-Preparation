# Ethernet, ARP, Switching, and VLANs

> **The one-sentence model:** IP decides *where* a packet should go; Ethernet decides *which physical port* it leaves through, using MAC addresses learned from traffic. ARP is the glue that turns the next-hop IP into a MAC address.

This layer is where most "the IP is right but it still doesn't work" incidents live: wrong VLAN, stale ARP, duplicate IP, MAC flap, or an MTU mismatch.

---

## 1. What layer 2 actually does

A host with an IP packet needs to hand it to a NIC. The NIC needs a destination MAC address for the next hop — the router if the destination is off-subnet, or the destination host if it is on-subnet.

```text
IP packet:  src 192.168.10.10 -> dst 192.168.10.20
Ethernet:   src 52:54:00:aa:bb:cc -> dst 52:54:00:dd:ee:ff
```

The IP packet is the payload inside the Ethernet frame. Layer 2 is local: it delivers frames within a **broadcast domain**. Routers break broadcast domains; switches extend them.

### Broadcast domain vs collision domain

| Concept | Meaning | Modern reality |
|---|---|---|
| Collision domain | Set of devices sharing one collision path | Point-to-point full-duplex links make this mostly historical |
| Broadcast domain | Set of devices that receive each other's broadcasts | One VLAN = one broadcast domain |
| LAN | A layer-2 network, usually one VLAN/subnet | Not the same as "the building" |

Two hosts in different VLANs are in different broadcast domains even if they share a cable/switch. They can only communicate through a router (or a layer-3 switch).

---

## 2. The Ethernet frame

```text
+---------+---------+-----------+------------------+---------+-----+
| Preamble|  DST MAC|  SRC MAC  | EtherType / VLAN | Payload | FCS |
|  7+1 B  |  6 B    |   6 B     |      2 B         | 46–1500 | 4 B |
+---------+---------+-----------+------------------+---------+-----+
```

| Field | Size | Purpose |
|---|---:|---|
| Preamble/SFD | 8 B | Clock synchronisation; not counted in frame size |
| Destination MAC | 6 B | Who should receive this frame |
| Source MAC | 6 B | Who sent it |
| EtherType | 2 B | What is inside: `0x0800` IPv4, `0x86DD` IPv6, `0x0806` ARP, `0x8100` VLAN tag |
| Payload | 46–1500 B | The IP packet, padded if too small |
| FCS | 4 B | CRC for corruption detection |

- Standard maximum payload (MTU) is **1500 bytes**.
- The frame on the wire is 1518 bytes without a VLAN tag, 1522 with one.
- Jumbo frames are typically 9000 bytes MTU and must be configured end-to-end.
- The FCS detects corruption; it does not correct it. A bad frame is dropped.

### MAC addresses

```text
52:54:00:aa:bb:cc
└──┬──┘ └──┬──┘
 OUI      NIC-specific
```

- 48 bits, usually written as six hex octets.
- First 24 bits are the OUI (vendor); locally administered addresses can ignore this.
- **Unicast:** least significant bit of the first octet is 0.
- **Multicast:** least significant bit of the first octet is 1 (e.g. `01:00:5e:...` for IPv4 multicast).
- **Broadcast:** `ff:ff:ff:ff:ff:ff`, delivered to every port in the broadcast domain.

Check a MAC on Linux:

```bash
ip link show eth0
```

```text
2: eth0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc fq_codel state UP mode DEFAULT group default qlen 1000
    link/ether 52:54:00:aa:bb:cc brd ff:ff:ff:ff:ff:ff
```

`link/ether` is the MAC; `brd` is the broadcast address. The `<...>` flags tell you the interface is broadcast-capable and multicast-capable; `LOWER_UP` means the link is physically up.

---

## 3. ARP: IP to MAC

A host knows the destination IP but not the destination MAC. ARP resolves it.

### The exchange

```text
Host A (192.168.10.10) wants to send to 192.168.10.20

1. A checks its ARP cache: no entry.
2. A broadcasts: "Who has 192.168.10.20? Tell 192.168.10.10."
   dst MAC ff:ff:ff:ff:ff:ff, EtherType 0x0806
3. Every host in the broadcast domain receives it.
4. Only 192.168.10.20 replies, unicast:
   "192.168.10.20 is at 52:54:00:dd:ee:ff."
5. A caches the mapping and sends the IP packet inside a unicast frame.
```

If the destination is off-subnet, A ARPs for the **default gateway**, not the final destination. The destination MAC is always the next hop's MAC; the destination IP stays end-to-end.

### The ARP cache

```bash
ip neigh show
```

```text
192.168.10.1 dev eth0 lladdr 52:54:00:12:34:56 REACHABLE
192.168.10.20 dev eth0 lladdr 52:54:00:dd:ee:ff STALE
192.168.10.99 dev eth0 FAILED
```

| State | Meaning |
|---|---|
| `REACHABLE` | Confirmed recently; usable |
| `STALE` | Entry exists but not recently confirmed; traffic triggers revalidation |
| `DELAY` | Waiting briefly before probing |
| `PROBE` | Actively sending ARP requests |
| `FAILED` | No reply; resolution failed |
| `INCOMPLETE` | Request sent, no reply yet |
| `PERMANENT` | Static entry; never ages out |
| `NOARP` | Interface does not use ARP (e.g. point-to-point) |

`STALE` is normal, not an error. A stale entry is still used to send traffic while the kernel revalidates it.

Force a re-check:

```bash
sudo arping -I eth0 -c 3 192.168.10.20
```

```text
ARPING 192.168.10.20 from 192.168.10.10 eth0
Unicast reply from 192.168.10.20 [52:54:00:DD:EE:FF]  0.712ms
Unicast reply from 192.168.10.20 [52:54:00:DD:EE:FF]  0.698ms
Sent 3 probes (1 broadcast(s))
Received 2 response(s)
```

`arping` works at layer 2. It can succeed when ICMP is blocked, which makes it a good test of "is the host alive on this segment?" It bypasses routing, so it only works for hosts on the same subnet.

### Gratuitous ARP

A host sends an ARP announcement for its own IP, usually at boot or when an interface comes up, or when an IP moves (failover, VRRP, Kubernetes LoadBalancer). It updates neighbours' caches without a request. It is also how duplicate-IP detection is often implemented.

Watch for it:

```bash
sudo tcpdump -eni eth0 arp
```

```text
15:04:21.123456 52:54:00:aa:bb:cc > ff:ff:ff:ff:ff:ff, ethertype ARP (0x0806), length 42: Request who-has 192.168.10.10 tell 192.168.10.10, length 28
15:04:21.124001 52:54:00:dd:ee:ff > 52:54:00:aa:bb:cc, ethertype ARP (0x0806), length 42: Reply 192.168.10.10 is-at 52:54:00:dd:ee:ff, length 28
```

`-e` prints MAC addresses. Without it, `tcpdump` hides the layer-2 header you are trying to inspect.

### Proxy ARP

A router answers ARP on behalf of a host on another subnet, making itself the destination MAC. It is used in some overlay networks and misconfigured subnets, but it hides topology problems. Check with:

```bash
cat /proc/sys/net/ipv4/conf/eth0/proxy_arp
```

`0` = off. If it is `1` and you did not set it, find out why.

---

## 4. Switching: how a frame finds a port

A switch maintains a **MAC address table** (FDB, forwarding database):

| MAC | Port | VLAN | Age |
|---|---|---|---|
| 52:54:00:aa:bb:cc | 1 | 10 | 12 s |
| 52:54:00:dd:ee:ff | 7 | 10 | 4 s |

### Learning, flooding, filtering, aging

1. **Learn:** on ingress, record source MAC + port + VLAN.
2. **Known unicast:** if the destination MAC is in the table for that VLAN, forward only to that port.
3. **Unknown unicast / broadcast / multicast:** flood to all ports in the VLAN except the ingress port.
4. **Age:** entries expire (typically 300 s) so moves are relearned.

This is why the first packet to a new host may be flooded and later packets are not. It is also why a MAC move (same MAC on two ports) is a warning sign: a loop, a VM migration, or a spoof.

### Broadcast storms and loops

Ethernet has no TTL. A frame looping between two switches is flooded forever, multiplying until links saturate and CPUs spike. **STP/RSTP** prevents this by electing a root bridge and blocking redundant ports.

| STP concept | Meaning |
|---|---|
| Root bridge | Switch with the lowest bridge ID; the topology centre |
| Root port | Each non-root switch's best port toward the root |
| Designated port | Forwarding port on a segment toward the root |
| Blocking/alternate | Redundant port held down to break the loop |
| BPDU | Bridge Protocol Data Unit; hello/config messages |
| Edge/PortFast | Port to a host; skips listening/learning delay |

In virtual environments, the same problem exists with Linux bridges and hypervisor virtual switches. A loop between a bridge and a VM with two NICs on the same bridge can melt a host. If you see broadcast traffic climbing and `softirq` CPU high, suspect a loop before blaming the app.

Check interface errors and drops:

```bash
ip -s link show eth0
```

```text
2: eth0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc fq_codel state UP mode DEFAULT group default qlen 1000
    link/ether 52:54:00:aa:bb:cc brd ff:ff:ff:ff:ff:ff
    RX:  bytes packets errors dropped  missed   mcast
     18432012  143221      0       2       0    1204
    TX:  bytes packets errors dropped carrier collsns
      9231004   98211      0       0       0       0
```

- `errors` → physical/cable/duplex problems.
- `dropped` → buffer exhaustion or policy.
- `missed` → ring buffer overrun; often CPU starvation.
- `carrier` → link transitions; cable or SFP issue.
- `collsns` → collisions; should be zero on full-duplex.

`ethtool` gives link negotiation details:

```bash
sudo ethtool eth0
```

```text
Settings for eth0:
	Supported ports: [ TP ]
	Speed: 1000Mb/s
	Duplex: Full
	Auto-negotiation: on
	Link detected: yes
```

A speed/duplex mismatch (one side 100/full, other 100/half) causes late collisions, retransmits, and terrible throughput that looks like an application problem.

---

## 5. VLANs

A VLAN partitions one physical switch into multiple logical broadcast domains.

Why:

- Contain broadcast traffic.
- Separate tenants, environments, or security zones.
- Reduce failure blast radius.
- Make L2 topology match L3 subnets.

### 802.1Q tag

A 4-byte tag inserted after the source MAC:

```text
+--------+--------+------+-----+-----+---------+---------+
| TPID   | PCP    | DEI  | VID (12 bits)             | ... |
| 0x8100 | 3 bits | 1 bit| 1–4094                    |     |
+--------+--------+------+-----+-----+---------+---------+
```

| Field | Purpose |
|---|---|
| TPID `0x8100` | Marks the frame as tagged |
| PCP | Priority (QoS), 0–7 |
| DEI | Drop eligible indicator |
| VID | VLAN ID, 1–4094 (0 and 4095 reserved) |

### Access vs trunk ports

| Port type | Frames to/from host | Frames on the wire | Typical use |
|---|---|---|---|
| Access | Untagged | Untagged (switch internally tags) | Server/host NIC |
| Trunk | Tagged for allowed VLANs | Tagged, except native VLAN | Switch-to-switch, hypervisor uplink |

A host NIC normally sends untagged frames and is unaware of the VLAN. The switch's access port assigns the PVID (port VLAN ID). If you need the host to see tags — a hypervisor, a router, a network appliance — configure a trunk and a VLAN sub-interface.

**Native VLAN:** the one VLAN whose frames are sent untagged on a trunk. If two ends disagree on the native VLAN, traffic leaks between VLANs or breaks. Best practice: use an unused VLAN as native, never VLAN 1.

### VLAN hopping

Two classic attacks:

1. **Switch spoofing:** an attacker negotiates a trunk (DTP) and sees all VLANs. Fix: disable DTP/auto-trunking on access ports.
2. **Double tagging:** an attacker sends a frame with two tags. The first switch strips the outer (native) tag and forwards the inner tag to another VLAN. Fix: do not use VLAN 1 as native; do not put access ports on the native VLAN; prune unused VLANs from trunks.

VLANs are segmentation, not encryption. A misconfigured trunk defeats them.

---

## 6. VLANs on Linux

Linux can be a switch (bridge), a router (VLAN interfaces), or both.

### VLAN sub-interface (host is VLAN-aware)

```bash
sudo ip link add link eth0 name eth0.10 type vlan id 10
sudo ip addr add 192.168.10.10/24 dev eth0.10
sudo ip link set eth0.10 up
ip -d link show eth0.10
```

```text
3: eth0.10@eth0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue state UP mode DEFAULT group default qlen 1000
    link/ether 52:54:00:aa:bb:cc brd ff:ff:ff:ff:ff:ff promiscuity 0
    vlan protocol 802.1Q id 10 <REORDER_HDR> addrgenmode eui64 numtxqueues 1 numrxqueues 1
```

The `vlan protocol 802.1Q id 10` line is your proof that tagging is configured. This works when the upstream port is a trunk that allows VLAN 10.

### VLAN-aware bridge (Linux acts as a switch)

```bash
sudo ip link add br0 type bridge vlan_filtering 1
sudo ip link set br0 up

# Access port for a VM/host on VLAN 10
sudo ip link set eth1 master br0
sudo bridge vlan add dev eth1 vid 10 pvid untagged

# Trunk port toward another switch, allowing VLANs 10 and 20
sudo ip link set eth2 master br0
sudo bridge vlan add dev eth2 vid 10
sudo bridge vlan add dev eth2 vid 20

# The bridge itself needs the VLAN for local routing/SVI
sudo bridge vlan add dev br0 vid 10 self
```

`pvid` = port VLAN ID for untagged ingress. `untagged` = egress without a tag. `self` = the bridge's own VLAN membership (needed if you put an IP on `br0.10`, for example).

Inspect:

```bash
bridge vlan show
```

```text
port    vlan-id
eth1    10 PVID Egress Untagged
eth2    10
eth2    20
br0     10
```

And the forwarding table:

```bash
bridge fdb show
```

```text
52:54:00:dd:ee:ff dev eth1 vlan 10 master br0
33:33:00:00:00:01 dev br0 self permanent
```

The `vlan 10` in the FDB matters: the same MAC can exist in two VLANs and must be forwarded independently.

> A Linux bridge with `vlan_filtering 1` behaves like a managed switch. Without it, the bridge is VLAN-unaware and passes tagged frames as opaque payload.

### Namespace lab topology

If you have one Linux VM, you can build the whole thing with namespaces:

```text
ns1 (192.168.10.10/24) --- veth --- br0 (VLAN 10) --- veth --- ns2 (192.168.10.20/24)
                                      |
                              router ns (br0.10, br0.20)
                                      |
ns3 (192.168.20.30/24) --- veth --- br0 (VLAN 20)
```

Lab L2.2 builds this. The point is to prove that same-VLAN hosts talk directly, different-VLAN hosts need the router, and a trunk carries both tags.

---

## 7. Inter-VLAN routing

Three ways:

| Method | How | Trade-off |
|---|---|---|
| Router-on-a-stick | One trunk to a router; sub-interfaces per VLAN | Cheap; one link is a bottleneck and single point of failure |
| Layer-3 switch (SVI) | Switch virtual interface per VLAN; routing in hardware | Fast; more complex config and control plane |
| Firewall on a stick | Trunk to firewall; policy between zones | Security inspection; firewall becomes bottleneck |

On Linux, router-on-a-stick is just VLAN sub-interfaces plus `net.ipv4.ip_forward=1`:

```bash
sudo sysctl -w net.ipv4.ip_forward=1
sudo ip addr add 192.168.10.1/24 dev eth0.10
sudo ip addr add 192.168.20.1/24 dev eth0.20
```

Hosts use `.1` in their VLAN as the default gateway. The router needs routes to both subnets (connected routes appear automatically).

Check:

```bash
ip route
```

```text
192.168.10.0/24 dev eth0.10 proto kernel scope link src 192.168.10.1
192.168.20.0/24 dev eth0.20 proto kernel scope link src 192.168.20.1
```

Test from a host in VLAN 10 to VLAN 20:

```bash
ping -c 2 192.168.20.30
traceroute -n 192.168.20.30
```

If it fails, the order to check is: host gateway → VLAN tag on trunk → router sub-interface → return route → firewall. Do not start with `tcpdump` on the wrong interface.

---

## 8. Command field manual

### `ip link` — interfaces and MAC

**Purpose:** list or change link-layer state.

```bash
ip -br link
```

```text
lo               UNKNOWN        00:00:00:00:00:00 <LOOPBACK,UP,LOWER_UP>
eth0             UP             52:54:00:aa:bb:cc <BROADCAST,MULTICAST,UP,LOWER_UP>
eth1             UP             52:54:00:bb:cc:dd <BROADCAST,MULTICAST,UP,LOWER_UP>
br0              UP             52:54:00:cc:dd:ee <BROADCAST,MULTICAST,UP,LOWER_UP>
```

- `-br` = brief, one line per interface. Fast for scripts.
- `UP` is admin state; `LOWER_UP` is carrier.
- An interface can be `UP` with no `LOWER_UP` if the cable is unplugged.

**Common options:** `ip link set eth0 up|down`, `ip link set eth0 mtu 9000`, `ip link set eth0 promisc on`, `ip -s link`.

**Mistake:** setting an interface down over SSH on the management NIC. Use a console, or schedule a rollback:

```bash
sudo sh -c 'ip link set eth0 down; sleep 30; ip link set eth0 up' &
```

Even this is risky. Prefer a second management path.

### `ip neigh` — ARP/NDP cache

**Purpose:** inspect and manage neighbour mappings.

```bash
ip neigh show dev eth0
ip neigh flush dev eth0
sudo ip neigh replace 192.168.10.20 lladdr 52:54:00:dd:ee:ff dev eth0 nud permanent
```

- `replace` is idempotent; `add` fails if the entry exists.
- `nud permanent` stops revalidation — only for controlled cases; a wrong static entry is a silent outage.
- `flush` forces re-resolution; use after a gateway MAC change or failover.

**Scenario:** after a firewall/VRRP failover, hosts keep sending to the old gateway MAC. `ip neigh show` still says `REACHABLE` because the entry has not expired. Flushing fixes it; gratuitous ARP from the new gateway should prevent it.

### `bridge` — MAC table and VLANs

```bash
bridge link show
bridge fdb show br br0
bridge vlan show dev eth1
bridge -s fdb show
```

- `bridge link` lists ports and their bridge membership.
- `bridge fdb` is the MAC table; `-s` adds packet counters.
- `self` entries belong to the bridge itself, not a port.

**Scenario:** a VM cannot reach anything after migration. `bridge fdb show` shows its MAC still on the old port. The switch has not aged it out. Clear it or wait for aging; the real fix is to ensure the hypervisor sends a gratuitous ARP after migration.

### `arping` — layer-2 reachability

```bash
sudo arping -I eth0 -c 3 -D 192.168.10.20
```

- `-I` selects the interface. ARP is not routed; you must be on the segment.
- `-c 3` stops after three.
- `-D` = duplicate address detection; exits non-zero if any reply is seen. Useful before assigning a static IP.

**Mistake:** using `arping` across subnets. It cannot work; use `ping` and a route.

### `tcpdump -e` — see MACs and tags

```bash
sudo tcpdump -eni eth0 arp or vlan
```

```text
15:04:21.123456 52:54:00:aa:bb:cc > ff:ff:ff:ff:ff:ff, ethertype 802.1Q (0x8100), length 46: vlan 10, p 0, ethertype ARP, Request who-has 192.168.10.20 tell 192.168.10.10, length 28
```

- `-e` = print link-layer header.
- `-n` = no DNS/service resolution; avoids delays and misleading names.
- `-i any` captures all interfaces but loses some link-layer detail; use the specific interface when debugging L2.
- Filter `vlan` shows tagged frames; `arp` shows ARP.

**Scenario:** you suspect the trunk is not passing VLAN 20. Capture on the trunk: if you see `vlan 20` frames leaving but none arriving on the other side, the problem is the trunk allowed list, not the host.

### `ethtool` — physical link

```bash
sudo ethtool eth0
sudo ethtool -S eth0 | head -40
sudo ethtool -k eth0
```

- `-S` = NIC statistics (driver-specific).
- `-k` = offload features. `tcp-segmentation-offload` can make captures look like giant frames; disable it when capturing (`ethtool -K eth0 tso off gso off`) if you need exact wire behaviour.
- `-i` = driver/firmware; useful when a NIC is flapping.

**Mistake:** reading `Speed: Unknown!` as a broken link. Virtual NICs often report unknown; check `Link detected` and actual traffic instead.

---

## 9. Failure scenarios and how to reason

### Scenario A — two hosts on the same switch cannot ping

Facts: `192.168.10.10/24` and `192.168.10.20/24`, same switch, different access ports. `ip neigh` shows `FAILED`.

Order of evidence:

1. `ip -br addr` on both — are they actually in the same subnet?
2. `bridge vlan show` / switch config — same VLAN on both ports?
3. `tcpdump -eni <if> arp` on the sender — is the request leaving?
4. Capture on the receiver's port — does the request arrive?
5. `ip neigh` on the receiver — did it reply?
6. Check port security / MAC limits — a port that learned its maximum may drop new MACs.

A common cause: ports are in VLAN 10 and VLAN 20 but the IPs are in the same /24. The hosts think they are local, ARP never crosses, and both report failure.

### Scenario B — duplicate IP

Facts: intermittent connectivity, ARP cache flips between two MACs, monitoring shows packet loss.

```bash
sudo arping -I eth0 -c 5 -D 192.168.10.20
```

```text
ARPING 192.168.10.20
Unicast reply from 192.168.10.20 [52:54:00:dd:ee:ff]  0.701ms
Unicast reply from 192.168.10.20 [52:54:00:11:22:33]  0.688ms
Sent 5 probes (5 broadcast(s))
Received 2 response(s)
```

Two MACs answering the same IP is definitive. Find the switch port for each MAC (`bridge fdb`, switch `show mac address-table`), then find the host. Do not "fix" it by adding a static ARP entry; that just picks a winner and leaves the conflict.

### Scenario C — MAC flap

Facts: logs show `MAC address flapping`, latency spikes, occasional drops.

Causes: a loop, two NICs bonded incorrectly, a VM with two interfaces on the same bridge, or a spoofing/attack.

Evidence:

```bash
bridge -s fdb show | grep -i <mac>
```

If the port changes rapidly, find the two paths. STP should block one; if STP is disabled or the loop is inside a VM, it will not.

### Scenario D — MTU mismatch

Facts: SSH works, small pings work, large transfers hang; TLS handshake stalls.

Layer 2 itself is fine; the problem is that a 1500-byte packet plus headers exceeds a path that only supports 1400. Test:

```bash
ping -M do -s 1472 -c 1 192.168.10.20
```

`1472 + 8 ICMP + 20 IP = 1500`. If this fails but `-s 1400` works, the path MTU is smaller. For TCP, clamping MSS or fixing the tunnel MTU is the real fix. See [TCP and sockets](04-tcp-udp-sockets.md).

### Scenario E — VLAN mismatch on a trunk

Facts: both switches have VLAN 10 and 20, but hosts in VLAN 20 cannot reach each other.

Check the allowed VLAN list on both ends:

```text
switchport trunk allowed vlan 10,20
```

A trunk that allows only VLAN 10 drops VLAN 20 silently. `tcpdump -e` on the sender shows tagged frames leaving; on the receiver, nothing. That asymmetry is the clue.

---

## 10. Security notes for infrastructure engineers

- **Disable unused ports** and put them in an unused VLAN.
- **Do not use VLAN 1** for production traffic or as the native VLAN.
- **Prune trunks** to only the VLANs needed.
- **Enable BPDU guard** on host ports to prevent rogue switches.
- **Enable DHCP snooping** and dynamic ARP inspection where available; ARP has no authentication.
- **Port security / MAC limits** stop MAC flooding that overflows the FDB and turns a switch into a hub.
- **Private VLANs** isolate hosts within one subnet (useful for multi-tenant).
- **VLANs are not a security boundary by themselves.** Use firewalls and encryption between trust zones.

In cloud and Kubernetes, the equivalent controls are security groups, NetworkPolicies, and VPC/subnet design. The mental model — broadcast domain, default gateway, policy at the boundary — transfers directly.

---

## 11. Common mistakes

| Mistake | Why it hurts | Fix |
|---|---|---|
| Treating VLAN as a subnet | They often map 1:1, but are independent | VLAN = L2 domain; subnet = L3 range |
| Using the same subnet on two VLANs | ARP cannot cross; routing is ambiguous | One subnet per VLAN, or proper L3 design |
| Forgetting the trunk allowed list | Tagged frames dropped silently | Allow only needed VLANs on both ends |
| Native VLAN mismatch | Untagged traffic lands in the wrong VLAN | Match native VLAN or avoid it |
| Assuming `STALE` ARP is broken | It is normal | Check `FAILED`/`INCOMPLETE`, not `STALE` |
| Using `arping` across routers | ARP is link-local | Use `ping`/`traceroute` |
| Ignoring `ip -s link` errors | Physical problems masquerade as app issues | Check errors/drops/carrier first |
| Leaving `proxy_arp` on | Hides routing problems, security risk | Disable unless required |
| Static ARP as a quick fix | Masks duplicate IP/MAC move | Find the real owner |
| Testing failover without gratuitous ARP | Clients keep old MAC | Ensure GARP/unsolicited NA on failover |

---

## 12. Checkpoint

1. What is the destination MAC when a host sends to `8.8.8.8`? Why?
2. Why does ARP use broadcast for requests and unicast for replies?
3. A switch receives a frame for an unknown unicast MAC. What does it do?
4. What breaks if two ends of a trunk disagree on the native VLAN?
5. How many usable VLAN IDs are there?
6. What does `pvid untagged` mean on a Linux bridge port?
7. Why can `arping` succeed while `ping` fails?
8. What is the difference between `STALE`, `FAILED`, and `INCOMPLETE` in `ip neigh`?
9. Why is a broadcast storm catastrophic even on a fast network?
10. A host moved to a new port and traffic still goes to the old one. What are two possible causes?

### Answers

1. The default gateway's MAC. The IP destination stays `8.8.8.8`, but Ethernet only delivers to the next hop.
2. The sender does not know the target MAC, so it must ask everyone; the reply can be directed because the request carried the sender's MAC.
3. Floods it to all ports in the VLAN except the ingress port.
4. Untagged frames are classified into different VLANs at each end; traffic leaks or is dropped.
5. 4094 (1–4094).
6. Untagged ingress is assigned to that VLAN, and egress is sent untagged.
7. ARP is layer 2 and bypasses IP routing/firewalls; ICMP may be blocked or routed differently.
8. `STALE` = valid but unconfirmed; `FAILED` = resolution failed permanently; `INCOMPLETE` = request sent, no reply yet.
9. Ethernet has no TTL; frames multiply every loop until links/CPUs saturate.
10. Stale FDB/ARP entries; missing gratuitous ARP after migration; or a loop causing MAC flapping.

If you cannot answer 7 of 10, re-read sections 3–5 and redo Lab L2.2 before continuing.
