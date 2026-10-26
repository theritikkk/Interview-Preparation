# Packet Flows and Debugging

> **The one-sentence model:** every packet is built by a process, wrapped in TCP/UDP, wrapped in IP, wrapped in Ethernet, and rewritten at every L3 hop. If you can name the layer where the evidence stops, you have found the fault domain.

This file ties Phase 2 together. Read it after the protocol notes; use it as the runbook during labs.

---

## 1. The full journey

A process on `10.0.0.5` sends HTTPS to `203.0.113.10`.

```text
1. App        write(fd, buf, n)
2. Socket     TCP segments, adds ports/seq/ack, checksum
3. IP         adds src 10.0.0.5, dst 203.0.113.10, TTL 64
4. Route      ʻip route get 203.0.113.10ʼ → via 10.0.0.1 dev eth0
5. Neighbour  ARP for 10.0.0.1 → MAC of router
6. Ethernet   src MAC = host NIC, dst MAC = router, EtherType 0x0800
7. Qdisc      queue, maybe shape/delay
8. NIC        DMA, checksum offload, transmit
9. Switch     learns src MAC, forwards to router port (same VLAN)
10. Router    strips Ethernet, decrements TTL, routes, rewrites L2
11. NAT       SNAT source to public IP (conntrack entry)
12. Internet  repeated L3 hops; MACs change every hop
13. Server    receives, replies; NAT reverses; host receives
```

At every L3 hop:

- **IP source/destination stay the same** (unless NAT).
- **TTL decreases by 1.**
- **MAC source/destination are rewritten** to the current link.
- **VLAN tags may be added/removed.**
- **Checksums are updated** (IP header; TCP/UDP checksum covers a pseudo-header, so NAT must update it).

### Worked trace

Topology:

```text
host A 10.0.0.5/24 ── gw 10.0.0.1 ── NAT 203.0.113.20 ── internet ── 203.0.113.10
```

| Point | Src IP | Dst IP | Src MAC | Dst MAC | TTL |
|---|---|---|---|---|---|
| A egress | 10.0.0.5 | 203.0.113.10 | A | R1 (10.0.0.1) | 64 |
| R1 ingress | 10.0.0.5 | 203.0.113.10 | A | R1 | 64 |
| R1 egress (post-NAT) | 203.0.113.20 | 203.0.113.10 | R1-WAN | next-hop | 63 |
| Server ingress | 203.0.113.20 | 203.0.113.10 | last-hop | server | 63−n |
| Server reply | 203.0.113.10 | 203.0.113.20 | server | last-hop | 64 |
| R1 ingress (reply) | 203.0.113.10 | 203.0.113.20 | last-hop | R1-WAN | 64−n |
| R1 egress (un-NAT) | 203.0.113.10 | 10.0.0.5 | R1-LAN | A | 63−n |
| A ingress | 203.0.113.10 | 10.0.0.5 | R1-LAN | A | 63−n |

If you capture at A and the server simultaneously, you can align the two views and see exactly where the packet is lost or rewritten.

---

## 2. Where to capture

| Point | What it proves | Caveat |
|---|---|---|
| Sender host | Packet left the process/NIC | Offloads may make frames look larger |
| Switch SPAN/mirror | Traffic actually crossed the wire | Mirror port can drop under load |
| Router ingress/egress | Forwarding, NAT, ACL | Router may rate-limit capture |
| Receiver host | Packet arrived | Firewall may drop before capture point |
| Load balancer | Backend selection, TLS termination | Often only metadata/logs |

Capture as close to the failing endpoint as possible, then move outward. Capturing only in the middle tells you the packet existed somewhere, not where it died.

### A capture plan

```text
1. Capture on the client for 30 s: SYN? SYN-ACK? RST?
2. Capture on the server for the same window (NTP-synced clocks).
3. Compare: request seen on both? response seen on both?
4. If request missing at server → path/firewall.
5. If request present, response missing at client → return path/firewall.
6. If both present but app fails → application/protocol.
```

Synchronise clocks (`chrony`/`systemd-timesyncd`). Without it, comparing captures is guesswork.

---

## 3. tcpdump

### The first commands

```bash
sudo tcpdump -ni any -c 50
sudo tcpdump -ni eth0 host 10.0.0.9
sudo tcpdump -ni eth0 tcp port 443
sudo tcpdump -eni eth0 arp
```

- `-i any` captures all interfaces; the link-layer header may be `LINUX_SLL`, not Ethernet.
- `-n` disables DNS/port resolution (faster, less misleading).
- `-e` prints MAC addresses.
- `-c` stops after N packets.
- `-tttt` absolute timestamps; `-tt` relative.
- `-q` less protocol detail; `-A` ASCII payload; `-X` hex+ASCII.
- `-s 0` full packet (default on modern tcpdump).
- `-w file.pcap` write; `-r file.pcap` read.

### Filter cheat sheet

| Goal | Filter |
|---|---|
| Host | `host 10.0.0.9` |
| Subnet | `net 10.0.0.0/24` |
| Port | `port 443` |
| Port range | `portrange 8000-8100` |
| TCP flags SYN | `tcp[tcpflags] & tcp-syn != 0` |
| SYN only (not SYN-ACK) | `tcp[tcpflags] & (tcp-syn|tcp-ack) == tcp-syn` |
| RST | `tcp[tcpflags] & tcp-rst != 0` |
| ICMP echo | `icmp[icmptype] == icmp-echo` |
| ARP | `arp` |
| VLAN | `vlan 10` |
| MAC | `ether host 52:54:00:aa:bb:cc` |
| DNS | `port 53` |
| DHCP | `port 67 or port 68` |
| HTTP | `tcp port 80` |
| Exclude SSH | `not port 22` |
| Payload contains string | `tcp contains "GET /health"` |

### Reading a capture

```bash
sudo tcpdump -ni eth0 -tttt 'host 10.0.0.9 and tcp port 443'
```

```text
2026-10-26 22:31:04.123456 IP 10.0.0.5.51422 > 10.0.0.9.443: Flags [S], seq 3123456789, win 64240, options [mss 1460,sackOK,TS val 1 ecr 0,nop,wscale 7], length 0
2026-10-26 22:31:04.124102 IP 10.0.0.9.443 > 10.0.0.5.51422: Flags [S.], seq 987654321, ack 3123456790, win 65160, options [mss 1460,sackOK,TS val 2 ecr 1,nop,wscale 7], length 0
2026-10-26 22:31:04.124301 IP 10.0.0.5.51422 > 10.0.0.9.443: Flags [.], ack 1, win 502, length 0
2026-10-26 22:31:04.125001 IP 10.0.0.5.51422 > 10.0.0.9.443: Flags [P.], seq 1:518, ack 1, win 502, length 517
2026-10-26 22:31:04.180001 IP 10.0.0.9.443 > 10.0.0.5.51422: Flags [P.], seq 1:1401, ack 518, win 501, length 1400
```

What to read:

- `Flags [S]` → SYN, `[S.]` → SYN-ACK, `[.]` → ACK, `[P.]` → PSH+ACK, `[F.]` → FIN+ACK, `[R]` → RST.
- `seq`/`ack` show ordering; gaps imply loss.
- `win` is the receive window; zero means the receiver is full.
- `length` is TCP payload bytes.
- `mss` and `wscale` are negotiated once, in the handshake.

### Common capture mistakes

| Mistake | Effect |
|---|---|
| Capturing on the wrong interface | No packets, false conclusion |
| `-i any` for L2 debugging | No Ethernet header (SLL) |
| Not disabling offloads | Capture shows segments larger than MTU |
| No `-n` | Slow reverse DNS, confusing names |
| Capturing too much | Disk fills, packets dropped by tcpdump |
| Forgetting `-c` or a timeout | Runaway capture |
| Capturing plaintext credentials | Compliance/security incident |

Use `timeout 30 tcpdump ...` for a bounded capture:

```bash
sudo timeout 30 tcpdump -ni eth0 -w /tmp/cap.pcap 'tcp port 443'
```

Disable offloads when exact wire behaviour matters:

```bash
sudo ethtool -K eth0 tso off gso off gro off
```

Remember to re-enable them; disabling offloads costs CPU.

---

## 4. Wireshark and tshark

Graphical analysis is better for TCP state, retransmissions, and TLS. `tshark` is the CLI equivalent.

```bash
tshark -r cap.pcap -Y 'tcp.analysis.retransmission'
tshark -r cap.pcap -Y 'http.response.code >= 500'
tshark -r cap.pcap -Y 'tls.handshake.extensions_server_name'
tshark -r cap.pcap -T fields -e ip.src -e ip.dst -e tcp.srcport -e tcp.dstport -e frame.time
```

Useful display filters:

| Filter | Finds |
|---|---|
| `tcp.flags.syn == 1 && tcp.flags.ack == 0` | SYNs |
| `tcp.flags.reset == 1` | Resets |
| `tcp.analysis.retransmission` | Retransmits |
| `tcp.analysis.zero_window` | Receiver window full |
| `tcp.analysis.window_update` | Window changes |
| `icmp.type == 3` | Destination unreachable (incl. frag needed) |
| `tls.alert_message` | TLS alerts |
| `http.request` / `http.response` | HTTP messages |
| `dns.flags.rcode != 0` | DNS errors |

Follow a stream: right-click → Follow → TCP Stream. This reconstructs the application conversation, which is often faster than reading packets.

**Security:** pcaps may contain credentials, tokens, and personal data. Store them with restricted permissions, delete after the incident, and follow your data-handling policy.

---

## 5. The five-step debug, with evidence

```text
1. NAME     getent hosts app.example.com       → IP?
2. ROUTE    ip route get <ip>                  → dev/gateway?
3. LINK     ping -c 2 <next-hop>; ip neigh     → reachable?
4. PORT     nc -vz <ip> <port>; ss -tulpn      → open?
5. APP      curl -v; openssl s_client          → protocol works?
```

### Evidence table

| Step | Command | Pass looks like | Fail means |
|---|---|---|---|
| Name | `getent hosts` | One or more IPs | DNS/NSS |
| Resolver | `dig @<resolver>` | `NOERROR` + answer | DNS server/cache |
| Route | `ip route get` | Expected dev/via | Routing |
| Neighbour | `ip neigh` | `REACHABLE`/`STALE` | L2/ARP/VLAN |
| ICMP | `ping -c 2` | Replies | Filtering/host down (not conclusive) |
| Port | `nc -vz` | `succeeded` | Firewall/app bind |
| Socket | `ss -tulpn` | LISTEN on expected addr | Bind/address |
| TLS | `openssl s_client` | `verify return code: 0` | Cert/chain/SNI |
| HTTP | `curl -v` | Expected status/body | App/proxy |
| Packets | `tcpdump` | Expected direction/flags | Where it stops |

Each step should produce a fact, not an opinion. "It's probably DNS" is not evidence; `getent hosts` returning NXDOMAIN is.

---

## 6. Network namespaces: a lab in one VM

Namespaces let you build isolated hosts, switches, and routers without extra VMs.

```bash
sudo ip netns add h1
sudo ip netns add h2
sudo ip netns add r1

sudo ip link add veth-h1 type veth peer name veth-r1a
sudo ip link set veth-h1 netns h1
sudo ip link set veth-r1a netns r1

sudo ip link add veth-h2 type veth peer name veth-r1b
sudo ip link set veth-h2 netns h2
sudo ip link set veth-r1b netns r1

sudo ip -n h1 addr add 10.0.1.10/24 dev veth-h1
sudo ip -n h1 link set veth-h1 up
sudo ip -n h1 link set lo up
sudo ip -n h1 route add default via 10.0.1.1

sudo ip -n h2 addr add 10.0.2.10/24 dev veth-h2
sudo ip -n h2 link set veth-h2 up
sudo ip -n h2 link set lo up
sudo ip -n h2 route add default via 10.0.2.1

sudo ip -n r1 addr add 10.0.1.1/24 dev veth-r1a
sudo ip -n r1 addr add 10.0.2.1/24 dev veth-r1b
sudo ip -n r1 link set veth-r1a up
sudo ip -n r1 link set veth-r1b up
sudo ip -n r1 link set lo up
sudo ip netns exec r1 sysctl -w net.ipv4.ip_forward=1
```

Test:

```bash
sudo ip netns exec h1 ping -c 2 10.0.2.10
sudo ip netns exec h1 traceroute -n 10.0.2.10
sudo ip netns exec r1 tcpdump -ni any icmp
```

Add a bridge and VLANs to reproduce L2 scenarios. Clean up:

```bash
sudo ip netns del h1
sudo ip netns del h2
sudo ip netns del r1
```

Deleting a namespace deletes its interfaces and processes. It is the fastest way to reset a lab.

---

## 7. Latency, loss, and throughput

These are different problems with different tools.

### Latency

```bash
ping -c 10 10.0.0.9
mtr -n -c 50 10.0.0.9
ss -tin dst 10.0.0.9
```

- ICMP RTT includes kernel processing, not application time.
- `ss -i` shows TCP RTT and retransmits for real connections.
- `mtr` shows per-hop loss/latency over time.
- Application latency = queue + connect + TLS + server processing + transfer.

### Loss

```bash
nstat -az | grep -E 'TcpRetrans|TcpExt'
ss -tin | grep -o 'retrans:[0-9/]*'
sudo tcpdump -ni eth0 'tcp[tcpflags] & tcp-rst != 0'
```

Retransmissions imply loss or corruption. A few are normal; a rising rate with falling throughput is not. Distinguish:

- Loss on the local link → `ip -s link` errors, `ethtool -S`.
- Loss in the path → `mtr` at the first hop where loss persists.
- Loss at the receiver → small receive buffer, `ss -tin` zero window.

### Throughput

Bandwidth-delay product:

```text
BDP = bandwidth × RTT
100 Mbps × 50 ms = 5 Mbit ≈ 625 KB in flight
```

A single TCP stream needs a window of at least BDP to fill the link. High-RTT paths need window scaling and larger buffers.

```bash
ss -tin
```

```text
cubic wscale:7,7 rtt:48.2/1.2 mss:1460 cwnd:42 bytes_retrans:0
```

`cwnd:42` × 1460 ≈ 61 KB in flight; on a 100 ms path that caps throughput around 5 Mbps. That is a tuning/latency issue, not packet loss.

### Queueing

```bash
tc -s qdisc show dev eth0
```

```text
qdisc fq_codel 0: root refcnt 2 limit 10240p flows 1024 quantum 1514 target 5ms interval 100ms
 Sent 18432012 bytes 143221 pkt (dropped 0, overlimits 0 requeues 0)
 backlog 0b 0p requeues 0
```

Drops/overlimits indicate congestion. `fq_codel` is a good default; `pfifo_fast` is not.

---

## 8. Worked scenarios

### Scenario A — "slow network" that is actually DNS

Facts: `curl` takes 5 s, then succeeds; `ping` by IP is fast.

```bash
curl -sS -o /dev/null -w 'dns=%{time_namelookup} connect=%{time_connect} tls=%{time_appconnect} ttfb=%{time_starttransfer}\n' https://app.example.com
```

```text
dns=5.002 connect=5.010 tls=5.060 ttfb=5.070
```

DNS is the 5 s. Check `resolvectl status`, `dig`, `/etc/resolv.conf`, and whether the first resolver is unreachable (the stub waits for timeout then tries the next).

### Scenario B — packet loss only for large packets

Facts: ping small works, SSH connects then hangs, `curl` stalls.

```bash
ping -M do -s 1472 -c 1 10.0.0.9
ping -M do -s 1372 -c 1 10.0.0.9
```

If 1472 fails and 1372 works, path MTU is ~1400. Confirm with `tracepath` and check for ICMP type 3 code 4 filtering. Fix the tunnel MTU or clamp MSS; do not lower the whole host MTU.

### Scenario C — one-way audio / one-way traffic

Facts: request arrives at the server; response never reaches the client.

Capture both ends:

- Server egress shows replies leaving.
- Client ingress shows nothing.

Possible causes: return route missing, `rp_filter`, NAT not reversing, cloud NACL blocking ephemeral ports, firewall state missing.

Check on the server:

```bash
ip route get <client-ip>
sudo tcpdump -ni any host <client-ip>
```

### Scenario D — intermittent RST every 60 s

Capture with timestamps and compute the delta from the last data packet. If it is consistently 60 s, look for:

- LB idle timeout = 60 s.
- NAT/conntrack timeout = 60 s.
- Application keepalive > 60 s.

Fix by keeping the connection active below the timeout or raising the timeout. See [TCP and sockets](04-tcp-udp-sockets.md).

### Scenario E — duplicate IP causes flapping

```bash
sudo arping -I eth0 -c 3 -D 10.0.0.20
sudo tcpdump -eni eth0 arp
```

Two MACs replying to one IP. Find both switch ports (`bridge fdb`, switch MAC table), identify the hosts, and remove the static/duplicate address. Do not paper over it with a static ARP entry.

### Scenario F — Kubernetes pod can reach IPs but not Services

Facts: `curl 10.0.0.9` works; `curl myapp` times out.

This is DNS or Service DNAT, not general networking.

```bash
kubectl exec -it pod -- cat /etc/resolv.conf
kubectl exec -it pod -- getent hosts myapp
kubectl get svc,endpointslices myapp
kubectl get networkpolicy
```

If DNS resolves but the connection times out, check kube-proxy rules, endpoints, and NetworkPolicy. If DNS fails, check CoreDNS and the pod's `dnsPolicy`.

---

## 9. The debugging runbook template

Use this for every lab and incident. Write it down; do not keep it in your head.

```markdown
## Incident: <one line>

**Impact:** who/what is broken, since when.
**Change:** what changed recently (deploy, config, cert, scale, network).
**Hypothesis:** one sentence, falsifiable.

### Evidence
| # | Check | Command | Result |
|---|---|---|---|
| 1 | Name | getent hosts ... | ... |
| 2 | Route | ip route get ... | ... |
| 3 | L2 | ip neigh / arping | ... |
| 4 | Port | ss -tulpn / nc | ... |
| 5 | TLS | openssl s_client | ... |
| 6 | HTTP | curl -v | ... |
| 7 | Packets | tcpdump | ... |

**Fault domain:** DNS / route / L2 / firewall / TCP / TLS / app.
**Root cause:** ...
**Fix:** ...
**Verification:** ...
**Follow-up:** monitoring, alert, runbook, test.
```

A good incident write-up answers "how did we prove it?" not just "what did we change?".

---

## 10. Common mistakes

| Mistake | Why it hurts | Fix |
|---|---|---|
| Starting with `tcpdump` | Too much data, no hypothesis | Run the five steps first |
| Capturing on the wrong host/interface | False "no packets" | Confirm interface and direction |
| Comparing unsynced clocks | Wrong ordering | Use NTP/chrony |
| Ignoring return path | Half the problem invisible | Capture both ends |
| Assuming ICMP failure = host down | ICMP often filtered | Test TCP port |
| Forgetting NAT in captures | Addresses look "wrong" | Capture before/after NAT |
| Trusting one `ping` | ICMP rate limits/loss | Use `mtr`, TCP tests |
| Changing several things at once | Cannot attribute the fix | One change, verify, record |
| No baseline | Cannot tell "slow" from "normal" | Measure before/after |
| Not saving evidence | Cannot explain or prevent | Save pcap, commands, timings |

---

## 11. Checkpoint

1. What changes at every L3 hop, and what stays the same?
2. Why is capturing at the sender not enough?
3. Which tcpdump flag shows MAC addresses, and why does `-i any` hide them?
4. What does `Flags [R]` mean, and what are three causes?
5. How do you prove a packet was dropped by a firewall rather than the application?
6. What is the first hop where `mtr` loss is meaningful?
7. Why can a TCP stream be slow on a high-RTT path with no loss?
8. What does `ss -tin` show that `ping` cannot?
9. Why is `ip netns` useful for network labs?
10. What should every incident write-up contain besides the fix?

### Answers

1. TTL decrements and MACs are rewritten; IPs stay the same unless NAT.
2. The packet may leave the sender and be dropped or rewritten later; you need the receiver's view.
3. `-e`. `-i any` uses the Linux cooked header (SLL), which has no Ethernet MACs.
4. RST: abort. Causes: app crash/close with unread data, firewall/LB reset, conntrack expiry, peer reboot.
5. Capture on both sides and check firewall counters/logs; if the packet arrives at the host but no application response, it is not the path.
6. The first hop where loss appears and persists to later hops.
7. Bandwidth-delay product exceeds the congestion/receive window; throughput is window/RTT.
8. Real connection RTT, cwnd, retransmits, MSS/PMTU, delivery rate.
9. It creates isolated hosts/routers/switches in one VM, with instant cleanup.
10. Impact, evidence, fault domain, root cause, verification, and follow-up.

If you miss more than three, redo sections 1, 3, and 5 before the Phase 2 labs.
