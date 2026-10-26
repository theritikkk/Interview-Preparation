# Routing and Linux Networking

> **The one-sentence model:** for every packet, the kernel asks one question — "which route matches this destination best?" — and the answer names an output interface and a next-hop IP. Everything else (ARP, firewall, NAT) follows from that choice.

Routing is not a list of static facts. It is a deterministic lookup with tie-breakers, and every hop repeats it independently.

---

## 1. The routing decision

For an outbound packet to `D`:

```text
1. Look up D in the main routing table.
2. Choose the most specific matching route (longest prefix).
3. If several match equally, choose the lowest metric.
4. If the route has a gateway, ARP/resolve the gateway and send to its MAC.
   If the route is directly connected, ARP/resolve D itself.
5. If no route matches, use the default route (0.0.0.0/0).
6. If there is still no route, return "Network is unreachable".
```

The kernel does this per packet, not per connection (with exceptions: cached routes, policy rules, conntrack). The source address is chosen from the route or the interface.

### Longest prefix match

```text
Destination: 10.20.30.40

10.0.0.0/8        via 192.168.1.1     matches (8 bits)
10.20.0.0/16      via 192.168.1.2     matches (16 bits)
10.20.30.0/24     dev eth1            matches (24 bits)  ← wins
0.0.0.0/0         via 192.168.1.254   matches (0 bits)
```

Specificity beats metric. A `/24` with metric 1000 beats a `/8` with metric 0. Metrics only break ties between routes of the same prefix length (and same table).

### Why this matters in incidents

- A leftover `/32` host route silently overrides your new default gateway.
- A VPN pushes a `0.0.0.0/1` and `128.0.0.0/1`, which beat the real default without replacing it.
- Kubernetes adds routes and iptables rules that can override what you expect.
- A connected route always exists for an interface's subnet, even if you "removed" the default route.

---

## 2. Reading the route table

```bash
ip route
```

```text
default via 192.168.1.1 dev eth0 proto dhcp src 192.168.1.50 metric 100
192.168.1.0/24 dev eth0 proto kernel scope link src 192.168.1.50 metric 100
10.10.0.0/16 via 192.168.1.254 dev eth0 proto static metric 50
172.17.0.0/16 dev docker0 proto kernel scope link src 172.17.0.1 linkdown
```

| Field | Meaning |
|---|---|
| `default` | `0.0.0.0/0` — catch-all |
| `via` | Next-hop IP (gateway) |
| `dev` | Output interface |
| `proto` | Who installed it: `kernel` (connected), `dhcp`, `static`, `boot`, `ra` |
| `scope link` | Destination is directly reachable on this link; no gateway needed |
| `src` | Preferred source address for this route |
| `metric` | Preference among same-prefix routes; lower wins |
| `linkdown` | Interface is down; route is inactive |

`proto kernel` routes are automatic connected routes. You cannot delete them while the interface has the address; remove the address instead.

### Route types

| Type | Example | Behaviour |
|---|---|---|
| Unicast | `ip route add 10.0.0.0/8 via 192.168.1.1` | Normal forwarding |
| Default | `ip route add default via 192.168.1.1` | Fallback |
| Blackhole | `ip route add blackhole 10.99.0.0/16` | Silently drops; no ICMP |
| Unreachable | `ip route add unreachable 10.98.0.0/16` | Drops and returns ICMP unreachable |
| Prohibit | `ip route add prohibit 10.97.0.0/16` | Drops and returns ICMP prohibited |
| Local | automatic for local IPs | Delivers to the host itself |
| Broadcast | automatic | Delivers to the subnet broadcast |
| Throw | `throw` | Falls through to the next rule/table |

Blackhole routes are useful for remote-triggered blackholing (RTBH) during DDoS: drop traffic to a victim IP before it reaches the host, without installing a firewall rule on every router.

### Multiple default routes

```bash
ip route add default via 192.168.1.1 dev eth0 metric 100
ip route add default via 10.0.0.1 dev eth1 metric 200
```

Lower metric wins. If the primary fails, the route is removed only if the kernel/daemon notices. A static secondary default with a higher metric is not automatic failover unless link state or a routing daemon withdraws the primary.

Check the chosen one:

```bash
ip route get 8.8.8.8
```

```text
8.8.8.8 via 192.168.1.1 dev eth0 src 192.168.1.50 uid 1000
    cache
```

The `via` and `dev` are the decision. `src` is the source address that will be used. `uid 1000` shows that policy routing can depend on the user. Drop `cache` from your mental model; modern kernels do not cache routes the old way, but `ip route get` still prints it.

### `ip route get` is the single most useful routing command

```bash
ip route get 10.20.30.40
ip route get 10.20.30.40 from 192.168.1.50
ip route get 10.20.30.40 iif eth1
ip route get 10.20.30.40 mark 0x1
```

- `from` tests source-based policy routing.
- `iif` tests the input interface (useful for asymmetric routing / VRF).
- `mark` tests firewall-mark-based routing.

**Scenario:** "The app can't reach the database." Run `ip route get <db-ip>` on the app host. If it says `via 192.168.1.1 dev eth0` but the database is on `10.10.0.0/16` reachable via a second NIC, you have found the problem without a single packet capture.

---

## 3. Interface configuration

### Temporary (lost on reboot)

```bash
sudo ip addr add 192.168.10.10/24 dev eth0
sudo ip link set eth0 up
sudo ip route add 10.0.0.0/8 via 192.168.10.1
```

Check:

```bash
ip -br addr
```

```text
lo               UNKNOWN        127.0.0.1/8 ::1/128
eth0             UP             192.168.10.10/24 fe80::5054:ff:feaa:bbcc/64
eth1             UP             10.0.0.10/24
```

`ip addr add` does not persist. For labs, that is fine; for servers, use the platform's configuration.

### Persistent options

| Platform | Tool | Where |
|---|---|---|
| Ubuntu 20.04+/22.04 | netplan (YAML) | `/etc/netplan/*.yaml` |
| Ubuntu 18.04 / Debian | ifupdown | `/etc/network/interfaces` |
| RHEL/CentOS/Rocky 8+ | NetworkManager keyfiles | `/etc/NetworkManager/system-connections/` |
| RHEL 7 | network-scripts | `/etc/sysconfig/network-scripts/ifcfg-*` |
| Cloud images | cloud-init | `/etc/cloud/cloud.cfg.d/` |
| Containers | none (namespace config) | runtime/CNI |

### netplan example (Ubuntu)

```yaml
# /etc/netplan/50-cloud-init.yaml
network:
  version: 2
  ethernets:
    eth0:
      dhcp4: false
      addresses: [192.168.10.10/24]
      routes:
        - to: default
          via: 192.168.10.1
      nameservers:
        addresses: [192.168.10.53, 1.1.1.1]
        search: [lab.local]
```

```bash
sudo netplan try      # apply with automatic rollback if you lose connectivity
sudo netplan apply
```

`netplan try` is the safe way to change networking over SSH. It reverts after 120 seconds unless you confirm.

### NetworkManager example (RHEL/Rocky)

```bash
sudo nmcli con mod "System eth0" \
  ipv4.method manual \
  ipv4.addresses 192.168.10.10/24 \
  ipv4.gateway 192.168.10.1 \
  ipv4.dns "192.168.10.53 1.1.1.1"
sudo nmcli con up "System eth0"
```

Check:

```bash
nmcli -p con show "System eth0"
nmcli device status
```

**Mistake:** mixing netplan and NetworkManager on the same interface. Pick one renderer per interface; otherwise both write routes and you get duplicates or flapping.

---

## 4. IP forwarding

A Linux host forwards packets only if told to:

```bash
sysctl net.ipv4.ip_forward
```

```text
net.ipv4.ip_forward = 0
```

Enable temporarily:

```bash
sudo sysctl -w net.ipv4.ip_forward=1
```

Persist:

```bash
echo 'net.ipv4.ip_forward = 1' | sudo tee /etc/sysctl.d/99-forwarding.conf
sudo sysctl --system
```

For IPv6:

```bash
sudo sysctl -w net.ipv6.conf.all.forwarding=1
```

Enable forwarding when the host is:

- a router between subnets,
- a VPN gateway,
- a Kubernetes node running pod networking (many CNIs require it),
- a NAT gateway.

Do **not** enable it on a normal workstation "to make things work". It turns the host into a router and can expose networks.

### Reverse path filtering

`rp_filter` drops packets whose source address would not be routed back out the interface they arrived on. It is a spoofing defence and a common cause of asymmetric-routing breakage.

```bash
sysctl net.ipv4.conf.all.rp_filter net.ipv4.conf.eth0.rp_filter
```

```text
net.ipv4.conf.all.rp_filter = 1
net.ipv4.conf.eth0.rp_filter = 2
```

| Value | Meaning |
|---:|---|
| 0 | Off |
| 1 | Strict: reverse route must use the same interface |
| 2 | Loose: any route back is enough |

Strict mode breaks multihomed hosts and some overlay networks. Loose mode is the usual compromise. `all` and the interface value interact: the effective value is the maximum of the two in many kernels, so check both.

**Scenario:** a VPN client can connect but not pass traffic. `rp_filter=1` drops return packets that arrive on the tunnel but would route back via the physical NIC. The fix is a route/policy fix, not blindly setting `rp_filter=0`.

---

## 5. Policy routing

The main table is not the only table. Rules select a table based on source, destination, mark, interface, or UID.

```bash
ip rule show
```

```text
0:	from all lookup local
32766:	from all lookup main
32767:	from all lookup default
```

Default order: `local` (highest priority, lowest number), then `main`, then `default`. Lower priority number = evaluated earlier.

### Custom table for a second uplink

Goal: traffic from `10.0.0.0/24` uses ISP B, everything else uses ISP A.

```bash
echo '200 isp_b' | sudo tee -a /etc/iproute2/rt_tables

sudo ip route add 10.0.0.0/24 dev eth1 src 10.0.0.10 table isp_b
sudo ip route add default via 10.0.0.1 dev eth1 table isp_b

sudo ip rule add from 10.0.0.0/24 lookup isp_b priority 100
```

Verify:

```bash
ip rule show
ip route show table isp_b
ip route get 8.8.8.8 from 10.0.0.10
```

```text
8.8.8.8 from 10.0.0.10 via 10.0.0.1 dev eth1 table isp_b src 10.0.0.10 uid 0
```

Without the rule, the main table's default would win. This is source-based routing, common for multi-homed firewalls and VPN gateways.

### Mark-based routing

```bash
sudo ip rule add fwmark 0x1 lookup 100 priority 100
sudo ip route add default via 10.0.0.1 table 100
sudo iptables -t mangle -A OUTPUT -p tcp --dport 443 -j MARK --set-mark 0x1
```

Used for policy-based forwarding, transparent proxies, and some CNIs. Debug with `ip route get ... mark 0x1`.

### VRF (virtual routing and forwarding)

VRFs give a host multiple independent routing tables, like a router. Common in telecom, multi-tenant hosts, and Kubernetes with VRF-based CNIs.

```bash
sudo ip link add vrf-blue type vrf table 100
sudo ip link set vrf-blue up
sudo ip link set eth1 master vrf-blue
ip route show table 100
ip vrf exec vrf-blue curl -s ifconfig.me
```

`ip vrf exec` runs a process bound to the VRF. Without it, the process uses the main table.

**Mistake:** putting an interface in a VRF and expecting the main table to still see its connected route. It won't. Add explicit routes or use `ip vrf exec`.

---

## 6. Asymmetric routing

Asymmetric routing means the forward and return paths differ. It is normal in some designs and a bug in others.

```text
Client -> FW1 -> Server
Server -> FW2 -> Client
```

Consequences:

- Stateful firewalls drop the return packet (no state on FW2).
- Strict `rp_filter` drops it.
- NAT is applied on one path only, so the return address is wrong.
- TCP works sometimes, usually during the first packets, then stalls.

Diagnose:

```bash
ip route get <client-ip>
tcpdump -ni any host <client-ip>
```

On the server, check which interface the return packet leaves. If it is not the interface the request arrived on, you have asymmetry.

Fixes:

- Make routing symmetric (preferred).
- Use loose `rp_filter` on the server.
- Use a stateful firewall pair with session sync.
- Use policy routing to force the return path.

Never "fix" asymmetry by disabling firewalls globally.

---

## 7. ARP flux

On a multihomed Linux host, ARP replies may be sent from any interface for any local IP. This is **ARP flux**: a client on `eth1` asks for the host's `eth0` IP, and the host answers on `eth1`, causing traffic to arrive on the wrong NIC.

Controls:

```bash
sysctl net.ipv4.conf.all.arp_ignore net.ipv4.conf.all.arp_announce
```

| Setting | Value | Effect |
|---|---:|---|
| `arp_ignore` | 0 | Reply for any local address on any interface |
| | 1 | Reply only if the target IP is configured on the receiving interface |
| `arp_announce` | 0 | Use any local source address |
| | 2 | Prefer the best local address for the target |

For routers and multihomed hosts, `arp_ignore=1` and `arp_announce=2` are common. They are also required for some VRRP/keepalived setups.

**Scenario:** a load balancer VIP is configured on `eth0`, but clients on `eth1` ARP for it and get a reply from `eth1`. Traffic arrives on the wrong interface and the service breaks. `arp_ignore=1` forces correct behaviour.

---

## 8. Traceroute, ping, and path evidence

### `ping`

```bash
ping -c 4 10.20.30.40
ping -I eth0 -c 4 10.20.30.40
ping -M do -s 1472 -c 1 10.20.30.40
```

- `-I` chooses source interface/address.
- `-M do` sets DF (don't fragment) to test path MTU.
- `-s` payload size.
- `-W` timeout, `-i` interval (root only below 0.2 s).

```text
PING 10.20.30.40 (10.20.30.40) from 192.168.10.10 eth0: 56(84) bytes of data.
64 bytes from 10.20.30.40: icmp_seq=1 ttl=62 time=1.24 ms
64 bytes from 10.20.30.40: icmp_seq=2 ttl=62 time=1.31 ms
```

`ttl=62` suggests two routed hops (initial 64). A sudden TTL change can indicate a path change.

ICMP may be blocked while TCP works. A failed ping is not proof the host is down.

### `traceroute`

```bash
traceroute -n 10.20.30.40
traceroute -T -p 443 -n example.com
```

```text
traceroute to 10.20.30.40 (10.20.30.40), 30 hops max, 60 byte packets
 1  192.168.10.1    0.512 ms  0.498 ms  0.501 ms
 2  10.0.0.1        1.204 ms  1.198 ms  1.211 ms
 3  10.20.30.40     1.890 ms  1.876 ms  1.901 ms
```

- Default probes are UDP to high ports; many networks filter them.
- `-T` uses TCP SYN (often allowed through firewalls).
- `-I` uses ICMP echo.
- `* * *` means no response — could be filtering, rate limiting, or a real black hole.

`mtr` combines ping and traceroute continuously; useful for intermittent loss.

```bash
mtr -n -c 20 10.20.30.40
```

Look for the first hop where loss appears **and persists** to later hops. Loss at one hop that disappears later is often ICMP rate limiting on that router, not a real problem.

### `ss` is routing-adjacent

A listening socket bound to `127.0.0.1:5432` is unreachable from another host no matter what the route table says.

```bash
ss -tulpn
```

```text
Netid State  Local Address:Port  Process
tcp   LISTEN 127.0.0.1:5432       users:(("postgres",pid=812,fd=5))
tcp   LISTEN 0.0.0.0:22           users:(("sshd",pid=701,fd=3))
```

A local bind is a common "routing problem" that is actually a bind-address problem. See [TCP and sockets](04-tcp-udp-sockets.md).

---

## 9. Command field manual

### `ip route` — view and change routes

**Purpose:** inspect or modify the forwarding table.

```bash
ip route show
ip route show table all
ip route get 1.1.1.1
sudo ip route add 10.50.0.0/16 via 192.168.1.254 dev eth0
sudo ip route del 10.50.0.0/16
sudo ip route replace default via 192.168.1.1
```

- `show table all` reveals policy tables you forgot about. This is the first command when routing "makes no sense".
- `replace` is safer than `add` in automation; it is idempotent.
- `del` needs enough specificity; deleting a prefix removes the matching route.

**Mistake:** adding a route via a gateway that is not on-link. The kernel returns `Nexthop has invalid gateway`. Either the gateway is wrong or you need `onlink` (rare, and usually a design smell).

### `ip rule` — policy routing rules

```bash
ip rule show
sudo ip rule add from 10.0.0.0/24 lookup 100 priority 100
sudo ip rule del priority 100
```

- Rules are ordered by priority (lower first).
- `lookup` names a table (number or `/etc/iproute2/rt_tables` name).
- `from all` is implicit if omitted.

**Mistake:** adding a rule without a route in the target table. The lookup returns nothing and the packet is dropped or falls through, depending on the rule.

### `ip addr` — addresses

```bash
ip -br addr
ip addr show dev eth0
sudo ip addr add 10.0.0.10/24 dev eth1
sudo ip addr del 10.0.0.10/24 dev eth1
```

- Adding an address also adds a connected route and a local route.
- Removing the last address removes the connected route.
- `ip addr flush dev eth0` removes all addresses on the interface — dangerous over SSH.

### `sysctl` — kernel network knobs

```bash
sysctl -a | grep -E 'ip_forward|rp_filter|arp_ignore'
sudo sysctl -w net.ipv4.ip_forward=1
sudo sysctl --system
```

- `-w` writes at runtime.
- `--system` reloads all `/etc/sysctl.d/*.conf` files.
- Persistent settings belong in `/etc/sysctl.d/`, not `/etc/sysctl.conf` (both work; drop-ins are cleaner).

**Mistake:** changing `net.ipv4.conf.all.rp_filter` but not the per-interface value. The effective value can still be strict.

### `nmcli` — NetworkManager

```bash
nmcli device status
nmcli con show
nmcli con show "System eth0" | grep -E 'ipv4|IP4'
sudo nmcli con mod "System eth0" ipv4.dns "1.1.1.1 8.8.8.8"
sudo nmcli con up "System eth0"
```

- `device status` shows whether NetworkManager manages each NIC.
- `con show` lists connection profiles, not active state.
- Changes to a profile apply on the next `up`.

### `traceroute` / `mtr`

```bash
traceroute -T -p 443 -n 1.1.1.1
mtr -n -c 20 1.1.1.1
```

Use TCP mode when UDP is filtered. Use `mtr` for loss over time.

---

## 10. Worked scenarios

### Scenario A — new NIC, no connectivity

Facts: you added `eth1` with `10.0.0.10/24`, but the host cannot reach `10.0.0.1`.

Evidence order:

```bash
ip -br addr show eth1          # address present? state UP?
ip route get 10.0.0.1          # which interface?
ping -I eth1 -c 2 10.0.0.1     # L3 reachability on that interface
ip neigh show dev eth1         # ARP resolved?
sudo tcpdump -eni eth1 arp     # requests leaving? replies arriving?
```

Likely causes:

- Interface `DOWN` (no `LOWER_UP`) → virtual NIC not connected.
- Wrong subnet mask → `10.0.0.1` is considered remote and goes via the default route.
- Upstream switch port in the wrong VLAN.
- `rp_filter` dropping replies.

### Scenario B — default route exists, nothing works

```bash
ip route get 8.8.8.8
```

```text
8.8.8.8 via 192.168.1.1 dev eth0 src 192.168.1.50
```

So the route is right. Next:

```bash
ping -c 2 192.168.1.1
ip neigh show 192.168.1.1
```

If the gateway does not answer ARP, the problem is L2/VLAN, not routing. If it answers but does not forward, the gateway's routing/NAT is the problem. If `traceroute` stops after the gateway, the issue is upstream.

### Scenario C — two default routes, traffic leaves the wrong one

```bash
ip route show default
```

```text
default via 192.168.1.1 dev eth0 metric 100
default via 10.0.0.1 dev eth1 metric 100
```

Equal metrics → the kernel picks one, often the first installed, and it can change. Fix by setting distinct metrics or using policy routing. Do not rely on "it worked yesterday".

### Scenario D — Docker/Kubernetes broke host routing

Container runtimes add bridges (`docker0`, `cni0`), veth pairs, and iptables/nftables rules. Symptoms: host cannot reach a subnet that overlaps with a container network, or forwarded traffic is masqueraded unexpectedly.

```bash
ip route show table all | grep -E 'docker|cni|flannel|calico'
sudo iptables -t nat -S | grep -E 'DOCKER|CNI'
```

Overlapping CIDRs are the usual cause. Choose pod/service CIDRs that do not overlap with the corporate network or VPN. See [Kubernetes](../04-containers-and-kubernetes/README.md).

### Scenario E — VPN connects but no traffic

Check in order:

```bash
ip rule show
ip route show table all | grep -E 'vpn|tun|wg'
ip route get 8.8.8.8
sysctl net.ipv4.ip_forward net.ipv4.conf.all.rp_filter
```

Common causes: VPN pushes routes into a separate table without a rule; `rp_filter` strict; missing NAT on the tunnel; DNS still pointing at a local resolver that is now unreachable.

---

## 11. Common mistakes

| Mistake | Why it hurts | Fix |
|---|---|---|
| Reading only `ip route`, not `table all` | Policy routes invisible | Always check `table all` when confused |
| Assuming metric beats prefix | Longest prefix wins first | Compare prefix length first |
| Equal-metric defaults | Nondeterministic path | Set metrics or policy |
| Route via non-onlink gateway | Kernel rejects it | Correct gateway or `onlink` with reason |
| Forgetting `ip_forward` | Router silently drops | Enable only where needed |
| Strict `rp_filter` on multihomed | Drops valid asymmetric traffic | Loose mode or fix routing |
| Mixing netplan and NetworkManager | Duplicate/flapping config | One renderer per interface |
| `ip addr flush` over SSH | Instant lockout | Console access; targeted `del` |
| Blaming routing for a bind address | `127.0.0.1` is not routable | Check `ss -tulpn` |
| Ignoring ARP/neigh | Route is fine, L2 is broken | Check `ip neigh`, `tcpdump` |

---

## 12. Checkpoint

1. Which route wins: `10.0.0.0/8 via A` or `10.1.0.0/16 via B` for `10.1.2.3`?
2. What does `ip route get 8.8.8.8` tell you that `ip route` does not?
3. What is the difference between `blackhole` and `unreachable`?
4. Why does a connected route disappear when you remove the address?
5. What does `scope link` mean?
6. When would you use policy routing instead of a second default route?
7. What does `rp_filter=1` do, and when does it break things?
8. Why is `arp_ignore=1` useful on a multihomed host?
9. A host can ping its gateway but not `8.8.8.8`. Name three possible causes.
10. You added a route via `10.0.0.1`, but the kernel says "Nexthop has invalid gateway". Why?

### Answers

1. `10.1.0.0/16` — longer prefix wins regardless of metric.
2. The actual chosen route, output interface, and source address for a specific destination, including policy rules.
3. `blackhole` drops silently; `unreachable` sends ICMP unreachable.
4. The connected route is derived from the address; no address, no connected subnet.
5. The destination is directly reachable on that link; no gateway is needed.
6. When the path depends on source address, mark, interface, or user — not just destination.
7. Strict reverse-path filter: drops packets whose source would not route back out the ingress interface. Breaks multihoming and asymmetric routing.
8. It makes the host reply to ARP only for IPs on the receiving interface, preventing ARP flux.
9. Missing/default route on the gateway; gateway has no route/NAT; upstream firewall; DNS vs IP confusion; `rp_filter` on return.
10. `10.0.0.1` is not in a directly connected subnet, so the kernel cannot ARP for it. Add the correct gateway or an on-link route.

If you miss more than two, redo sections 2, 5, and 7 before Lab L2.2 (Part B).
