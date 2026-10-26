# Phase 2 Checkpoint — Networking

> **Rules:** closed book, 75 minutes for Parts A–D. Paper for subnetting. No `dig`, no search engine. Then do the practical sign-off with the VM. Target: 80% overall, and at least 8/10 on the diagnosis scenarios.

---

## Part A — Rapid facts (15 × 2 = 30 points)

1. What is the network address of `10.20.30.40/22`?
2. How many `/26` subnets fit in a `/24`?
3. What is the broadcast address of `192.168.1.130/26`?
4. Summarise `10.1.4.0/24` through `10.1.7.0/24`.
5. Is `10.0.0.200/24` inside `10.0.0.0/25`?
6. What destination MAC does a host use when sending to `8.8.8.8`?
7. What destination MAC does a host use when sending to another host in the same subnet?
8. How large is an 802.1Q tag, and what is the usable VLAN range?
9. What is the difference between an access port and a trunk port?
10. Why does a native VLAN mismatch break a trunk?
11. Which route wins for `10.1.2.3`: `10.0.0.0/8 via A` or `10.1.0.0/16 via B`?
12. What does `scope link` mean in a route?
13. What does `net.ipv4.ip_forward=1` enable?
14. What does strict `rp_filter=1` do?
15. What is the difference between `blackhole` and `unreachable` routes?

---

## Part B — TCP, DNS, and NAT (15 × 2 = 30 points)

16. List the three TCP handshake packets and their flags.
17. What does `SYN-SENT` on the client mean?
18. What does `CLOSE_WAIT` mean, and who must fix it?
19. Why does `TIME_WAIT` exist?
20. What is the on-wire difference between `Connection refused` and a timeout?
21. For a LISTEN socket, what do `Recv-Q` and `Send-Q` mean?
22. What is the MSS for a 1500-byte MTU over IPv4?
23. How do you test for a path-MTU black hole with `ping`?
24. What is the difference between recursive and iterative DNS?
25. How long is an NXDOMAIN answer cached?
26. Why can `dig` fail while `getent hosts` succeeds?
27. List the four DHCP messages and say why Request is broadcast.
28. Why does a routed network need a DHCP relay?
29. Which NAT hook does DNAT use, and which does SNAT use?
30. What are the four main conntrack states?

---

## Part C — HTTP, TLS, and load balancing (10 × 2 = 20 points)

31. What is the difference between 502, 503, and 504?
32. What does SNI do, and why does testing TLS by IP often fail?
33. A server sends only the leaf certificate. Which clients fail, and why?
34. Which curl timing metric isolates the TLS handshake?
35. What is the difference between L4 and L7 load balancing?
36. Why can HTTP/2 still have head-of-line blocking?
37. What does `X-Forwarded-Proto` tell an application?
38. Why should client keep-alive be shorter than the load balancer idle timeout?
39. What is the risk of TLS 1.3 0-RTT?
40. What does a 429 status mean, and which header should you read?

---

## Part D — Diagnosis scenarios (10 × 3 = 30 points)

For each, name the **fault domain** and the **next command/check**.

### D1

```text
$ curl -v https://api.example.com/
*   Trying 203.0.113.10:443...
* connect to 203.0.113.10 port 443 failed: Connection refused
```

### D2

```text
$ curl -v --max-time 8 https://api.example.com/
*   Trying 203.0.113.10:443...
* Operation timed out after 8001 milliseconds with 0 bytes received
```

### D3

```text
$ ss -lnt
State  Recv-Q Send-Q Local Address:Port
LISTEN 512    512    0.0.0.0:8080
```

### D4

```text
$ ss -tan state close-wait | wc -l
4821
```

### D5

```text
$ ip route get 10.20.30.40
10.20.30.40 via 192.168.1.1 dev eth0 src 192.168.1.50
```

The database is supposed to be reachable via `eth1` on `10.20.0.0/16`.

### D6

```text
$ dig app.lab.local
;; ->>HEADER<<- opcode: QUERY, status: NXDOMAIN
$ getent hosts app.lab.local
10.10.10.50  app.lab.local
```

### D7

```text
$ ping -M do -s 1472 -c 1 10.0.0.9
ping: local error: message too long, mtu=1400
$ ping -M do -s 1372 -c 1 10.0.0.9
64 bytes from 10.0.0.9: icmp_seq=1 ttl=63 time=1.2 ms
```

### D8

```text
10.0.0.5.51422 > 10.0.0.9.443: Flags [S], seq 1, length 0
10.0.0.5.51422 > 10.0.0.9.443: Flags [S], seq 1, length 0
10.0.0.5.51422 > 10.0.0.9.443: Flags [S], seq 1, length 0
```

### D9

Facts: request arrives at the server; the server's response leaves the server; the client never receives it. `ip route get <client>` on the server points to a different interface than the request arrived on.

### D10

```text
$ curl -v https://api.example.com/
* SSL certificate problem: unable to get local issuer certificate
$ openssl s_client -connect api.example.com:443 -servername api.example.com -showcerts
...
Certificate chain
 0 s:CN = api.example.com
   i:CN = R3
```

---

## Part E — Design and explain (20 points)

### E1 — Address plan (10 points)

Given `172.16.8.0/21`, allocate:

| Segment | Hosts |
|---|---:|
| compute | 500 |
| storage | 200 |
| management | 50 |
| gpu | 25 |
| p2p | 2 |

Largest-first, aligned, no overlap. State the remaining free range.

### E2 — Packet walk (10 points)

Host A `10.0.1.10/24` (gateway `10.0.1.1`) sends to `10.0.2.20` behind a router. Describe:

1. Whether A ARPs for `10.0.2.20` or `10.0.1.1`.
2. The source/destination IP and MAC on the first frame.
3. What the router changes before forwarding.
4. What the server sees as the source IP if the router uses masquerade.
5. Where the return path can fail.

---

## Answer key

### Part A

1. `10.20.28.0` (block 4 in third octet; 30 → 28).
2. Four.
3. `192.168.1.191` (block 64; `.128` network, `.191` broadcast).
4. `10.1.4.0/22`.
5. No; the `/25` covers `.0–.127`, `.200` is outside.
6. The default gateway's MAC.
7. The destination host's MAC (after ARP).
8. 4 bytes; usable VLAN IDs 1–4094.
9. Access carries one untagged VLAN to a host; trunk carries multiple tagged VLANs (plus a native VLAN) between switches/hypervisors.
10. The two ends classify untagged frames into different VLANs, so traffic leaks or is dropped.
11. `10.1.0.0/16` — longest prefix wins.
12. The destination is directly reachable on that link; no gateway.
13. IPv4 forwarding between interfaces (the host acts as a router).
14. Drops packets whose source would not route back out the ingress interface; breaks asymmetric routing.
15. `blackhole` silently drops; `unreachable` returns ICMP unreachable.

### Part B

16. SYN, SYN+ACK, ACK.
17. The SYN was sent but no SYN+ACK arrived (drop, route, or server down).
18. The peer sent FIN and the local application has not closed; the application must fix it.
19. To re-ACK a lost FIN and prevent old duplicate segments from entering a new connection with the same tuple.
20. Refused = RST; timeout = no response (DROP/black hole).
21. Recv-Q = completed connections waiting to be accepted; Send-Q = backlog limit.
22. 1460 bytes.
23. `ping -M do -s 1472` (1500 total); lower the size until it works, then `tracepath`.
24. Recursive asks one resolver for the final answer; iterative queries each delegation level.
25. For the SOA minimum/negative TTL, commonly 300 s.
26. `getent` uses NSS and can read `/etc/hosts` or other sources; `dig` queries DNS directly.
27. Discover, Offer, Request, ACK. Request is broadcast so all offering servers know which offer was accepted.
28. Broadcasts do not cross routers; a relay forwards with `giaddr` so the server chooses the right pool.
29. DNAT in PREROUTING (or OUTPUT for local); SNAT in POSTROUTING.
30. NEW, ESTABLISHED, RELATED, INVALID (plus UNTRACKED/SNAT/DNAT).

### Part C

31. 502 = bad upstream response/connection; 503 = no healthy upstream/overload; 504 = upstream timeout.
32. SNI carries the hostname in the TLS handshake so the server selects the certificate/vhost; without it, an IP connection gets the default cert.
33. Clients without the intermediate cached, such as curl/Java; the chain cannot be built to a trusted root.
34. `time_appconnect - time_connect`.
35. L4 routes by IP/port; L7 parses HTTP and can route by host/path/header, terminate TLS, and add proxy headers.
36. All streams share one TCP connection; one lost TCP segment blocks delivery to all streams.
37. The original request scheme (http/https) so the app generates correct redirects/URLs.
38. Otherwise the client reuses a connection the LB already closed, causing resets.
39. Replay of early data; only safe for idempotent requests.
40. Too many requests; read `Retry-After`.

### Part D

| # | Fault domain | Next check |
|---|---|---|
| D1 | TCP/port or firewall REJECT | `ss -tulpn` on the server; app bind address |
| D2 | Packet drop/route/black hole | `tcpdump` on client/server; security group/NACL |
| D3 | Accept queue full | Application accept loop/threads; `nstat ListenOverflows` |
| D4 | Application socket leak | Process/socket owner; fix close paths |
| D5 | Routing | `ip route show table all`; expected `eth1` route missing/overridden |
| D6 | NSS/hosts vs DNS | `/etc/hosts`/`nsswitch.conf`; app uses NSS |
| D7 | Path MTU/PMTUD | `tracepath`; ICMP frag-needed filtering; clamp MSS |
| D8 | DROP/black hole | Firewall counters, route, capture at server |
| D9 | Asymmetric routing/NAT/rp_filter | Return route; `rp_filter`; SNAT/hairpin |
| D10 | TLS chain incomplete | Serve fullchain; `openssl verify`; intermediate missing |

### Part E

**E1** — `172.16.8.0/21` = `172.16.8.0–172.16.15.255`.

| Segment | Prefix | Network | Broadcast | Usable |
|---|---|---|---|---|
| compute | /23 | 172.16.8.0 | 172.16.9.255 | .8.1–.9.254 |
| storage | /24 | 172.16.10.0 | 172.16.10.255 | .10.1–.10.254 |
| management | /26 | 172.16.11.0 | 172.16.11.63 | .11.1–.11.62 |
| gpu | /27 | 172.16.11.64 | 172.16.11.95 | .11.65–.11.94 |
| p2p | /30 | 172.16.11.96 | 172.16.11.99 | .11.97–.11.98 |
| free | — | 172.16.11.100–172.16.15.255 | — | reserved |

**E2** —

1. A ARPs for `10.0.1.1` (the destination is off-subnet).
2. Frame: src IP `10.0.1.10`, dst IP `10.0.2.20`, src MAC A, dst MAC router.
3. The router decrements TTL, rewrites the Ethernet header (src router, dst next hop/server), and forwards; with NAT it also rewrites the source IP.
4. With masquerade, the server sees the router's address on that interface (e.g. `10.0.2.1`), not `10.0.1.10`.
5. The server's return route, the router's NAT/conntrack state, `rp_filter`, and the client's inbound firewall.

---

## Scoring

| Section | Points | Your score | Target |
|---|---:|---:|---:|
| A — facts | 30 | | ≥ 24 |
| B — TCP/DNS/NAT | 30 | | ≥ 24 |
| C — HTTP/TLS | 20 | | ≥ 16 |
| D — diagnosis | 30 | | ≥ 24 |
| E — design | 20 | | ≥ 16 |
| **Total** | **130** | | **≥ 104 (80%)** |

If you score below 80%, do not move on. Re-read the specific notes, redo the matching lab, and retake the checkpoint after 48 hours.

---

## Practical sign-off

Do these on the VM without notes. Each is pass/fail.

- [ ] Subnet `10.20.0.0/22` into five segments (largest-first) and verify with a script.
- [ ] Build two VLANs and a trunk with namespaces; prove same-VLAN and cross-VLAN with ping and `tcpdump -e`.
- [ ] Create a listening socket, fill its accept queue, and show `Recv-Q`/`ListenOverflows`.
- [ ] Create `CLOSE_WAIT` and explain why a restart only hides it.
- [ ] Run a local DNS server; show NXDOMAIN, caching, and split-horizon.
- [ ] Add masquerade NAT and DNAT; show conntrack entries and the server-side source IP.
- [ ] Write a default-deny nftables ruleset allowing only 443; show the counter and a DROP vs REJECT difference.
- [ ] Build a root → intermediate → leaf chain; show leaf-only failure and full-chain success.
- [ ] Capture a TCP handshake, DNS query, ARP exchange, and TLS ClientHello with SNI.
- [ ] From a pcap with repeated SYNs and no reply, name the fault domain and the next check.

---

## Oral exam (10 minutes)

Answer out loud, without notes:

1. "Walk me through what happens when I type `curl https://api.example.com` on a fresh VM."
2. "The server is up and the port is listening, but I get a timeout. What do you check?"
3. "Why can two hosts ping each other but not reach a third host in another VLAN?"
4. "Explain TIME_WAIT to a developer who wants to disable it."
5. "A certificate works in Chrome but not in curl. Why?"
6. "What is the difference between a 502 and a 504, and what would you look at first?"
7. "How would you prove a packet is being dropped by a firewall rather than the application?"
8. "What is the most common mistake you made in these labs, and how did you find it?"

Record yourself. If you use a word you cannot define (`conntrack`, `SNI`, `backlog`, `asymmetric`), stop and define it.

---

## Retest log

| Date | Score | Weak areas | Remediation | Retest date |
|---|---:|---|---|---|
| | | | | |
| | | | | |

When you pass, continue to [Phase 3 — Virtualization and Storage](../03-virtualization-and-storage/README.md).
