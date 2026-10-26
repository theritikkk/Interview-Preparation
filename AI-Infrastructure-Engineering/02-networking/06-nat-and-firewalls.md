# NAT and Firewalls

> **The one-sentence model:** a firewall decides whether a packet may pass; NAT rewrites addresses/ports and remembers the rewrite in a state table so replies can be un-rewritten. Both happen at specific hooks in the kernel's packet path, and the order explains most "it works locally but not remotely" bugs.

---

## 1. Why NAT exists

IPv4 has ~4.3 billion addresses; the internet has more devices. NAT lets many private hosts share one public address.

| Type | What changes | Typical use |
|---|---|---|
| SNAT | Source IP (and optionally port) | Private hosts reaching the internet |
| MASQUERADE | SNAT to the outgoing interface's address | Dynamic public IP (DHCP, cloud) |
| DNAT | Destination IP/port | Port forwarding, load balancing |
| PAT/NAPT | Port translation as well as address | Almost all consumer NAT |
| Hairpin NAT | Internal client → public IP → internal server | Same service reachable from inside and outside |
| CGNAT | Carrier-grade NAT, `100.64.0.0/10` | ISPs without enough public IPv4 |

NAT is not security. It hides topology as a side effect, but a DNAT rule can expose anything, and outbound NAT does not stop outbound attacks.

### The state table is the point

```text
Inside:  10.0.0.5:51422  →  203.0.113.10:443
NAT:     203.0.113.20:41001 → 203.0.113.10:443
Reply:   203.0.113.10:443 → 203.0.113.20:41001
Un-NAT:  203.0.113.10:443 → 10.0.0.5:51422
```

Without the state entry, the reply has nowhere to go. This is why:

- NAT breaks protocols that embed IPs in payloads (SIP, FTP active mode, some games).
- A NAT device restart drops all connections.
- Conntrack table exhaustion breaks new connections.
- Asymmetric routing breaks NAT (the reply bypasses the device that holds state).

IPv6 was designed without NAT, but NAT66/NPTv6 exists. Do not assume IPv6 means "no translation" in every network.

---

## 2. The Linux packet path

```text
             ┌──────────────┐
 incoming →  │ PREROUTING   │  raw → mangle → nat (DNAT)
             └──────┬───────┘
                    │ routing decision
          ┌─────────┴─────────┐
          ▼                   ▼
     ┌─────────┐         ┌─────────┐
     │ INPUT   │         │ FORWARD │  mangle → filter
     └────┬────┘         └────┬────┘
          ▼                   ▼
      local process      ┌──────────────┐
                         │ POSTROUTING  │  mangle → nat (SNAT)
                         └──────┬───────┘
                                ▼
                            outgoing
```

For locally generated traffic, `OUTPUT` runs before `POSTROUTING`.

| Hook | Sees | NAT? |
|---|---|---|
| PREROUTING | All incoming packets, before routing | DNAT |
| INPUT | Packets destined for this host | No |
| FORWARD | Packets routed through this host | No (filter) |
| OUTPUT | Locally generated packets | DNAT |
| POSTROUTING | All outgoing packets, after routing | SNAT/MASQUERADE |

Key consequence: **DNAT happens before the routing decision** (so the new destination is routed), and **SNAT happens after** (so the source is rewritten last).

### Netfilter vs iptables vs nftables

- **Netfilter** is the kernel framework (hooks, conntrack, NAT).
- **iptables** is the legacy userspace tool; on modern distros it is often `iptables-nft`, a compatibility layer over nftables.
- **nftables** is the modern tool and kernel backend, with better performance and atomic rulesets.
- **firewalld** and **ufw** are frontends that manage nftables/iptables for you.

Do not mix tools on the same system. If `iptables` is `iptables-nft`, rules show up in `nft list ruleset`; if firewalld is active, it owns the ruleset. Editing iptables directly while firewalld runs leads to rules that disappear on reload.

Check:

```bash
iptables --version
```

```text
iptables v1.8.9 (nf_tables)
```

The `(nf_tables)` suffix means the nft backend.

---

## 3. iptables essentials

### Tables

| Table | Purpose | Chains |
|---|---|---|
| `filter` | Allow/deny | INPUT, FORWARD, OUTPUT |
| `nat` | Address translation | PREROUTING, OUTPUT, POSTROUTING |
| `mangle` | Header changes, marks | All five |
| `raw` | Bypass conntrack | PREROUTING, OUTPUT |
| `security` | SELinux labels | INPUT, FORWARD, OUTPUT |

### Targets

| Target | Effect |
|---|---|
| `ACCEPT` | Stop this table's traversal; packet continues |
| `DROP` | Silently discard |
| `REJECT` | Discard and send ICMP/TCP reset |
| `LOG` | Log and continue (not a verdict) |
| `RETURN` | Return to the calling chain |
| `SNAT`/`MASQUERADE` | Source translation (nat table) |
| `DNAT` | Destination translation (nat table) |
| `REDIRECT` | DNAT to the local host |
| `MARK` | Set fwmark for routing/policy |

### A default-deny host firewall

```bash
sudo iptables -F
sudo iptables -P INPUT DROP
sudo iptables -P FORWARD DROP
sudo iptables -P OUTPUT ACCEPT

# Allow loopback
sudo iptables -A INPUT -i lo -j ACCEPT

# Allow established/related return traffic
sudo iptables -A INPUT -m conntrack --ctstate ESTABLISHED,RELATED -j ACCEPT

# Allow SSH, HTTP, HTTPS
sudo iptables -A INPUT -p tcp --dport 22 -j ACCEPT
sudo iptables -A INPUT -p tcp --dport 80 -j ACCEPT
sudo iptables -A INPUT -p tcp --dport 443 -j ACCEPT

# Drop invalid
sudo iptables -A INPUT -m conntrack --ctstate INVALID -j DROP

# Allow ICMP echo for diagnostics (optional)
sudo iptables -A INPUT -p icmp --icmp-type echo-request -j ACCEPT
```

**Do not run this over SSH without console access.** `-P INPUT DROP` takes effect immediately. Add the SSH rule before the policy, or use `iptables-apply`:

```bash
sudo iptables-apply -t 60 /etc/iptables/rules.v4
```

`iptables-apply` asks you to confirm within 60 seconds; otherwise it restores the previous rules.

### Persistence

| Distro | Package/tool |
|---|---|
| Debian/Ubuntu | `iptables-persistent`, `/etc/iptables/rules.v4` |
| RHEL/Rocky | `iptables-services`, `/etc/sysconfig/iptables` |
| Any | `iptables-save > file`, `iptables-restore < file` |

On nftables-native systems, use `nft list ruleset > /etc/nftables.conf` and enable `nftables.service`.

### Counters

```bash
sudo iptables -L INPUT -n -v --line-numbers
```

```text
Chain INPUT (policy DROP 0 packets, 0 bytes)
num   pkts bytes target     prot opt in     out     source               destination
1     1204  100K ACCEPT     all  --  lo     *       0.0.0.0/0            0.0.0.0/0
2    88213   12M ACCEPT     all  --  *      *       0.0.0.0/0            0.0.0.0/0  ctstate RELATED,ESTABLISHED
3      402 24120 ACCEPT     tcp  --  *      *       0.0.0.0/0            0.0.0.0/0  tcp dpt:22
4        0     0 DROP       all  --  *      *       0.0.0.0/0            0.0.0.0/0
```

Counters are the fastest way to prove whether a rule is matching. `-x` shows exact numbers. Reset with `iptables -Z`.

**Mistake:** adding a DROP rule above an ACCEPT rule and wondering why the service is down. iptables evaluates top to bottom; first verdict wins.

---

## 4. nftables essentials

One tool, one ruleset, atomic reloads.

```bash
sudo nft list ruleset
```

```text
table inet filter {
	chain input {
		type filter hook input priority filter; policy drop;
		iif "lo" accept
		ct state established,related accept
		tcp dport 22 accept
		tcp dport { 80, 443 } accept
		ip protocol icmp icmp type echo-request accept
	}
	chain forward {
		type filter hook forward priority filter; policy drop;
	}
	chain output {
		type filter hook output priority filter; policy accept;
	}
}
```

Create it from a file:

```bash
sudo nft -c -f /etc/nftables.conf    # check syntax
sudo nft -f /etc/nftables.conf       # apply
```

`-c` checks without applying. This is the equivalent of `nginx -t` for your firewall; use it every time.

### NAT with nftables

```bash
# Enable forwarding first
sudo sysctl -w net.ipv4.ip_forward=1

sudo nft add table ip nat
sudo nft 'add chain ip nat postrouting { type nat hook postrouting priority srcnat; policy accept; }'
sudo nft add rule ip nat postrouting oifname "eth0" masquerade

sudo nft add table ip filter
sudo nft 'add chain ip filter forward { type filter hook forward priority filter; policy drop; }'
sudo nft add rule ip filter forward iifname "eth1" oifname "eth0" accept
sudo nft add rule ip filter forward iifname "eth0" oifname "eth1" ct state established,related accept
```

Note: nftables NAT is configured in the `nat` table with `type nat` chains, and the kernel handles conntrack automatically.

### Sets and maps

```bash
sudo nft add set inet filter blocked { type ipv4_addr\; }
sudo nft add element inet filter blocked { 203.0.113.66, 203.0.113.67 }
sudo nft add rule inet filter input ip saddr @blocked drop
```

Sets are efficient for large blocklists; maps translate values (e.g. port → backend).

---

## 5. firewalld

firewalld manages zones and services, and is standard on RHEL/Rocky/Fedora.

```bash
sudo firewall-cmd --state
sudo firewall-cmd --get-default-zone
sudo firewall-cmd --get-active-zones
```

```text
public
  interfaces: eth0
```

### Zones

| Zone | Default behaviour |
|---|---|
| `drop` | Drop all incoming, no reply |
| `block` | Reject all incoming |
| `public` | Allow selected services; untrusted networks |
| `external` | NAT/masquerade for external-facing |
| `internal` | Trusted internal network |
| `dmz` | Publicly accessible services, limited internal access |
| `trusted` | Allow everything |

A packet's zone is determined by its ingress interface (or source address). The default zone applies to interfaces not explicitly assigned.

### Runtime vs permanent

```bash
sudo firewall-cmd --add-service=https              # runtime only
sudo firewall-cmd --add-service=https --permanent  # written to config
sudo firewall-cmd --reload                          # apply permanent config
```

**Trap:** `--permanent` changes do not take effect until `--reload`. `--reload` also discards runtime-only changes. Make changes with `--permanent`, then reload, and verify.

### Common operations

```bash
sudo firewall-cmd --list-all
sudo firewall-cmd --zone=public --add-service=http --permanent
sudo firewall-cmd --zone=public --add-port=8080/tcp --permanent
sudo firewall-cmd --zone=public --remove-service=http --permanent
sudo firewall-cmd --reload
```

### NAT and port forwarding

```bash
# Masquerade outbound traffic
sudo firewall-cmd --zone=external --add-masquerade --permanent

# Forward external 8080 to internal 10.0.0.5:80
sudo firewall-cmd --zone=external \
  --add-forward-port=port=8080:proto=tcp:toport=80:toaddr=10.0.0.5 --permanent

# Enable IP forwarding (firewalld can do this)
sudo firewall-cmd --zone=external --add-forward --permanent
sudo firewall-cmd --reload
```

Firewalld writes the necessary nftables/iptables rules. Do not add duplicate rules manually.

### Rich rules

```bash
sudo firewall-cmd --zone=public --add-rich-rule='
  rule family="ipv4" source address="203.0.113.0/24" service name="ssh" accept' --permanent
```

Rich rules allow source-based policy, logging, and rate limits without dropping to raw nftables.

### Panic mode

```bash
sudo firewall-cmd --panic-on
sudo firewall-cmd --panic-off
```

Drops all traffic. Useful during an active compromise, but you lose remote access. Console required.

---

## 6. ufw (Ubuntu)

A simpler frontend, good for single hosts.

```bash
sudo ufw status verbose
sudo ufw default deny incoming
sudo ufw default allow outgoing
sudo ufw allow 22/tcp
sudo ufw allow from 10.0.0.0/24 to any port 5432 proto tcp
sudo ufw enable
```

```text
Status: active
Logging: on (low)
Default: deny (incoming), allow (outgoing), disabled (routed)
New profiles: skip

To                         Action      From
--                         ------      ----
22/tcp                     ALLOW IN    Anywhere
5432/tcp                   ALLOW IN    10.0.0.0/24
```

- Rules are evaluated in order; `ufw status numbered` shows them.
- `ufw delete <number>` removes one.
- `ufw allow` inserts at the top by default for some rule types; check order.
- Docker bypasses ufw for published ports because Docker inserts its own iptables rules. This is a classic surprise: `ufw deny 5432` does not stop `docker run -p 5432:5432`. Fix with `DOCKER_OPTS="--iptables=false"` (and manage rules yourself) or bind to `127.0.0.1`.

---

## 7. DROP vs REJECT

| | DROP | REJECT |
|---|---|---|
| Sender experience | Timeout | Immediate error |
| ICMP | None | ICMP admin prohibited / port unreachable |
| TCP | SYN retries, then timeout | RST (with `tcp-reset`) |
| Security | Reveals less | Reveals a firewall exists |
| Debugging | Harder | Easier |
| Best for | Internet edge, untrusted | Internal networks, client-facing |

A default DROP on the internet edge is normal. Inside a datacenter, REJECT makes failures fast and obvious. Either way, be consistent: a mix of DROP and REJECT makes timeouts ambiguous.

```bash
# Reject with TCP reset for a specific port
sudo iptables -A INPUT -p tcp --dport 8080 -j REJECT --reject-with tcp-reset
```

`REJECT --reject-with tcp-reset` produces "connection refused" instead of a timeout, which is often what clients expect.

---

## 8. Conntrack states and rules

| State | Meaning |
|---|---|
| `NEW` | First packet of a connection |
| `ESTABLISHED` | Part of an existing connection |
| `RELATED` | Related to an existing connection (FTP data, ICMP errors) |
| `INVALID` | Does not fit any state (malformed, out of window) |
| `UNTRACKED` | Bypassed conntrack via raw table |
| `SNAT`/`DNAT` | Translated connection |

The standard stateful firewall pattern:

```bash
sudo iptables -A INPUT -m conntrack --ctstate ESTABLISHED,RELATED -j ACCEPT
sudo iptables -A INPUT -m conntrack --ctstate INVALID -j DROP
sudo iptables -A INPUT -p tcp --dport 22 -m conntrack --ctstate NEW -j ACCEPT
```

Why allow ESTABLISHED first: return traffic for outbound connections must come back in. Without it, a default-deny host cannot browse, apt update, or resolve DNS.

**Mistake:** allowing only `NEW` on the service port and forgetting `ESTABLISHED,RELATED`, so requests arrive but replies are dropped.

### Conntrack tuning

```bash
sysctl net.netfilter.nf_conntrack_max
sysctl net.netfilter.nf_conntrack_tcp_timeout_established
conntrack -S
```

Long-lived idle connections can fill the table if the established timeout is too long (default 5 days). Lower it only with care; it can break long-lived sessions.

---

## 9. NAT in practice

### NAT gateway (masquerade)

Topology: `eth1` internal `10.0.0.1/24`, `eth0` external DHCP.

```bash
sudo sysctl -w net.ipv4.ip_forward=1

# nftables
sudo nft add table ip nat
sudo nft 'add chain ip nat postrouting { type nat hook postrouting priority srcnat; }'
sudo nft add rule ip nat postrouting oifname "eth0" masquerade

sudo nft add table ip filter
sudo nft 'add chain ip filter forward { type filter hook forward priority filter; policy drop; }'
sudo nft add rule ip filter forward iifname "eth1" oifname "eth0" accept
sudo nft add rule ip filter forward iifname "eth0" oifname "eth1" ct state established,related accept
```

Test from an internal host:

```bash
ping -c 2 1.1.1.1
curl -s https://ifconfig.me
```

The public address you see is the NAT gateway's `eth0` address.

### Port forwarding (DNAT)

Expose internal `10.0.0.5:80` as `203.0.113.20:8080`:

```bash
sudo nft add table ip nat
sudo nft 'add chain ip nat prerouting { type nat hook prerouting priority dstnat; }'
sudo nft add rule ip nat prerouting iifname "eth0" tcp dport 8080 dnat to 10.0.0.5:80

sudo nft add rule ip filter forward iifname "eth0" oifname "eth1" ip daddr 10.0.0.5 tcp dport 80 accept
sudo nft add rule ip filter forward iifname "eth1" oifname "eth0" ct state established,related accept
```

The return path needs either a route back through the gateway or SNAT. If the internal server's default gateway is not this host, replies bypass it and the connection fails.

### Hairpin NAT

An internal client resolving the public name gets the public IP, sends to the gateway, which DNATs to the internal server. The server replies directly to the client (same subnet), so the client sees a reply from a different IP than it sent to and drops it.

Fix: SNAT hairpin traffic so the server replies to the gateway:

```bash
sudo nft add rule ip nat postrouting ip saddr 10.0.0.0/24 ip daddr 10.0.0.5 tcp dport 80 masquerade
```

Or use split-horizon DNS so internal clients resolve the internal IP directly. Split DNS is cleaner; hairpin NAT is a workaround.

### Double NAT

```text
Client → home router NAT → ISP CGNAT → internet
```

Breaks inbound connections, some VPNs, and protocols that need a stable public port. Diagnose by comparing the address on the host with the address seen by a public service:

```bash
curl -s https://ifconfig.me
ip -br addr
```

If they differ, you are behind NAT. If the public address is in `100.64.0.0/10`, you are behind CGNAT and cannot port-forward without ISP help or a tunnel.

---

## 10. Cloud and container firewalls

### Security groups vs NACLs (AWS)

| | Security group | NACL |
|---|---|---|
| Applied to | ENI/instance | Subnet |
| Stateful | Yes | No |
| Rules | Allow only | Allow and deny |
| Evaluation | All rules | Numbered order |
| Return traffic | Automatically allowed | Must allow ephemeral ports |

A common failure: a NACL blocks return traffic on ephemeral ports (1024–65535). Security groups do not have this problem because they are stateful.

### Kubernetes

- **Service ClusterIP** is virtual; kube-proxy programs DNAT rules (iptables/nftables/IPVS) on every node.
- **NetworkPolicy** is enforced by the CNI, not by kube-proxy. A policy can block traffic even when the Service and endpoints are correct.
- **Conntrack** on nodes tracks Service connections; table pressure affects pod networking.
- **Hairpin** and **externalTrafficPolicy** affect source IP preservation and routing.

Debug order:

```bash
kubectl get svc,endpoints
kubectl get networkpolicy
kubectl exec -it pod -- curl -v http://service:port
sudo iptables -t nat -L KUBE-SERVICES -n -v
sudo conntrack -L | grep <service-ip>
```

If the Service has no endpoints, DNS may resolve but connections time out or reset. If a NetworkPolicy selects the pod, traffic may be dropped before it reaches the app.

### Docker

Docker inserts `DOCKER` chains and DNAT rules for `-p`. Published ports bypass ufw/firewalld host rules in many setups. Bind to `127.0.0.1` for local-only services, or manage Docker's iptables integration explicitly.

```bash
sudo iptables -t nat -L DOCKER -n -v
```

---

## 11. Debugging methodology

```text
1. Is forwarding enabled?          sysctl net.ipv4.ip_forward
2. Is there a route?               ip route get <dst>
3. Is the packet arriving?         tcpdump -ni <if> host <src>
4. Is it being translated?         conntrack -L, nft list ruleset
5. Is it being dropped?            rule counters, nft monitor trace
6. Is the return path valid?       tcpdump on both sides, ip route get
7. Is the app listening?           ss -tulpn
```

### nftables tracing

```bash
sudo nft add table ip trace
sudo nft 'add chain ip trace prerouting { type filter hook prerouting priority -300; }'
sudo nft add rule ip trace prerouting meta nftrace set 1
sudo nft monitor trace
```

This prints every rule a packet traverses. Remove the trace rule when done; it is expensive.

### Logging drops

```bash
sudo nft add rule inet filter input tcp dport 8080 log prefix "DROP-8080 " drop
sudo journalctl -kf | grep DROP-8080
```

Log with a rate limit:

```bash
sudo nft add rule inet filter input tcp dport 8080 limit rate 5/minute log prefix "DROP-8080 " drop
```

Unlimited logging on a dropped flood can take the host down.

### iptables counters

```bash
sudo iptables -t nat -L PREROUTING -n -v
sudo iptables -t filter -L FORWARD -n -v
```

If the DNAT rule counter is zero, the packet never reached it (wrong interface, earlier drop, or wrong hook).

---

## 12. Worked scenarios

### Scenario A — port forward works locally, not from outside

Facts: `curl http://10.0.0.5` works from the gateway; external clients time out.

Evidence:

```bash
sudo nft list ruleset
sudo nft list chain ip filter forward -a
sudo conntrack -L | grep 8080
sudo tcpdump -ni eth0 'tcp port 8080'
sudo tcpdump -ni eth1 'tcp port 80'
```

Likely causes:

- FORWARD policy DROP with no accept rule.
- DNAT rule on the wrong interface.
- Internal server's default gateway is not the NAT host (asymmetric return).
- Cloud security group/NACL blocking 8080.
- ISP blocks inbound ports.

### Scenario B — outbound NAT works for ICMP, not TCP

Facts: `ping 8.8.8.8` works from an internal host; `curl` hangs.

Hypotheses: MTU/MSS (ICMP small, TCP large), DNS, or a firewall rule allowing ICMP but not TCP.

Evidence:

```bash
curl -v --max-time 5 http://example.com
ping -M do -s 1472 -c 1 8.8.8.8
sudo tcpdump -ni eth0 'tcp port 80'
```

If SYNs leave but no SYN-ACK returns, the upstream is dropping. If the handshake completes but data stalls, suspect MSS/MTU; clamp MSS on the gateway.

### Scenario C — firewall change locks out SSH

Recovery options, in order:

1. Console/hypervisor access (the only guaranteed path).
2. Cloud serial console or Session Manager.
3. `iptables-apply` rollback if you used it.
4. A scheduled `at` job that restores rules:

```bash
echo 'iptables-restore < /root/rules.backup' | sudo at now + 5 minutes
```

5. Out-of-band management network.

Prevention: always keep an SSH accept rule above the default deny, test in a console, and use a rollback timer.

### Scenario D — Kubernetes Service unreachable

```bash
kubectl get svc myapp -o wide
kubectl get endpointslices -l kubernetes.io/service-name=myapp
kubectl exec -it test -- curl -v http://myapp.default.svc.cluster.local:8080
```

- No endpoints → selector mismatch or pods not ready.
- Endpoints present but timeout → NetworkPolicy, kube-proxy, or conntrack.
- DNS NXDOMAIN → CoreDNS/Service name.
- Connection refused → pod listening on the wrong port/address.

On the node:

```bash
sudo iptables -t nat -L KUBE-SERVICES -n -v | grep myapp
sudo conntrack -L | grep <cluster-ip>
```

### Scenario E — NAT gateway stops after conntrack fills

```bash
dmesg | tail
```

```text
nf_conntrack: table full, dropping packet
```

```bash
sysctl net.netfilter.nf_conntrack_count net.netfilter.nf_conntrack_max
conntrack -S | head
```

Find the source of the connection flood (often a scanner, a broken client retrying, or a leak). Raise `nf_conntrack_max` only after understanding the cause; otherwise you just delay the outage.

---

## 13. Common mistakes

| Mistake | Why it hurts | Fix |
|---|---|---|
| `-P INPUT DROP` before adding SSH | Instant lockout | Add allow rules first, use `iptables-apply` |
| Mixing firewalld and raw iptables | Rules disappear on reload | Pick one manager |
| Forgetting `ESTABLISHED,RELATED` | Replies dropped, one-way traffic | Add stateful rule first |
| DNAT without FORWARD accept | Packet translated then dropped | Allow forwarded flow |
| DNAT without return route | Asymmetric replies | SNAT/hairpin or fix gateway |
| Testing NAT only from the gateway | Local traffic bypasses PREROUTING | Test from a real client |
| `--permanent` without `--reload` | Changes not active | Reload and verify |
| Docker + ufw assumption | Published ports bypass ufw | Bind loopback or manage Docker rules |
| Logging every dropped packet | Disk/CPU exhaustion | Rate-limit logs |
| REJECT everywhere on the edge | Reveals firewall, extra ICMP | DROP at edge, REJECT internally |
| Assuming NAT = security | DNAT can expose anything | Explicit policy + patching |
| Ignoring IPv6 firewall | v6 is not filtered by v4 rules | Separate `ip6`/`inet` rules |

---

## 14. Checkpoint

1. What is the difference between SNAT and DNAT, and which hook does each use?
2. Why must DNAT happen before routing and SNAT after?
3. Why does a stateful firewall need `ESTABLISHED,RELATED`?
4. What is the difference between DROP and REJECT from the client's perspective?
5. Why does hairpin NAT need an extra SNAT rule?
6. What happens when the conntrack table is full?
7. Why is a NACL harder to manage than a security group?
8. What does `iptables -L -v` show that `iptables -S` does not?
9. Why can Docker published ports bypass ufw?
10. What is the first rule you add before setting a default DROP policy?
11. How do you test an nftables file without applying it?
12. A DNAT counter is zero. What are three possible reasons?

### Answers

1. SNAT rewrites source (POSTROUTING); DNAT rewrites destination (PREROUTING/OUTPUT).
2. DNAT changes the destination, so the route must be chosen after; SNAT changes the source, which only matters on egress.
3. Return traffic for outbound connections must be allowed without opening all inbound ports.
4. DROP = timeout; REJECT = immediate error (ICMP/RST).
5. The internal server replies directly to the internal client, bypassing the gateway; SNAT forces the reply through the gateway.
6. New connections are dropped (`nf_conntrack: table full`); existing may survive.
7. NACLs are stateless and ordered, so you must allow return ephemeral ports explicitly; SGs are stateful and allow-only.
8. Packet/byte counters per rule, which prove whether a rule matches.
9. Docker inserts its own DNAT rules before/around ufw's, so published ports are reachable regardless of ufw.
10. `-A INPUT -i lo -j ACCEPT` and the SSH allow rule (plus ESTABLISHED,RELATED).
11. `nft -c -f file`.
12. Wrong interface, packet dropped earlier, wrong hook/table, or traffic never arrives.

If you miss more than three, redo sections 2, 3, and 9 before Lab L2.5.
