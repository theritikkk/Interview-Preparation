# TCP, UDP, and Sockets

> **The one-sentence model:** a socket is a kernel object identified by `(protocol, local IP:port, remote IP:port)`. TCP adds ordered, reliable, flow-controlled delivery on top of IP; UDP does not. Most "connection" bugs are visible in `ss` or `tcpdump` before you ever read application logs.

---

## 1. Ports and sockets

An IP address gets a packet to a host. A **port** gets it to a process.

```text
192.168.10.10:5432  →  postgres listening
192.168.10.10:22    →  sshd listening
```

A connection is a 4-tuple:

```text
(protocol, src IP, src port, dst IP, dst port)
 TCP      10.0.0.5  51422    10.0.0.9  443
```

Two connections can share a local port if the remote tuple differs. A listening socket is special: it has no remote address yet.

### Port ranges

| Range | Use |
|---|---|
| 0–1023 | Well-known; binding usually requires root or `CAP_NET_BIND_SERVICE` |
| 1024–49151 | Registered/user ports |
| 32768–60999 | Linux ephemeral range (`net.ipv4.ip_local_port_range`) |
| 49152–65535 | IANA ephemeral range; overlaps Linux's tail |

Check:

```bash
sysctl net.ipv4.ip_local_port_range
```

```text
net.ipv4.ip_local_port_range = 32768	60999
```

That is ~28,000 ephemeral ports. A client making many short-lived connections to one server can exhaust them; the symptom is `Cannot assign requested address` or connect timeouts. The fix is connection reuse, not a bigger range (though widening the range helps temporarily).

### Socket lifecycle (server)

```c
fd = socket(AF_INET, SOCK_STREAM, 0);
bind(fd, addr, len);       // claim IP:port
listen(fd, backlog);       // mark as accepting
conn = accept(fd, ...);    // return a new fd per connection
read(conn, buf, n);
write(conn, buf, n);
close(conn);
```

`listen()` does not accept connections; it creates two queues:

- **SYN queue** (incomplete): handshake in progress.
- **Accept queue** (complete): handshake done, waiting for `accept()`.

If the application stops calling `accept()`, the accept queue fills and new connections are dropped or reset. `ss -lnt` shows the queue:

```bash
ss -lnt
```

```text
State  Recv-Q Send-Q Local Address:Port  Peer Address:Port
LISTEN 0      511          0.0.0.0:80         0.0.0.0:*
LISTEN 12     511        127.0.0.1:5432       0.0.0.0:*
```

For a listening socket:

- **Recv-Q** = completed connections waiting in the accept queue.
- **Send-Q** = backlog limit (`min(backlog, somaxconn)`).

`Recv-Q` persistently near `Send-Q` means the application is not accepting fast enough. That is an application/threading problem, not a network problem.

Tune carefully:

```bash
sysctl net.core.somaxconn
sysctl net.ipv4.tcp_max_syn_backlog
```

```text
net.core.somaxconn = 4096
net.ipv4.tcp_max_syn_backlog = 4096
```

Nginx, HAProxy, and Go/Rust servers often set their own backlog; the kernel cap is `somaxconn`. Raising the cap without fixing the accept loop just delays the failure.

---

## 2. The TCP header

```text
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|          Source Port          |       Destination Port        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                        Sequence Number                        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                    Acknowledgment Number                      |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  Data |     |C|E|U|A|P|R|S|F|                               |
| Offset| Res |W|C|R|C|S|H|Y|I|            Window             |
|       |     |R|E|G|K|H|T|N|N|                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|           Checksum            |         Urgent Pointer        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                    Options (variable)                         |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

| Field | Why you care |
|---|---|
| Sequence/Ack | Ordering and reliability; gaps trigger retransmission |
| Flags | SYN/FIN/RST/ACK/PSH/URG; RST is an immediate abort |
| Window | Receiver's buffer space; zero window = sender must stop |
| Checksum | Corruption detection |
| Options | MSS, window scale, SACK, timestamps |

The header is 20 bytes without options, up to 60 with. Options are negotiated in the handshake; you cannot add window scaling later.

### Flags in practice

| Flag | Meaning |
|---|---|
| SYN | Open a connection |
| ACK | Acknowledgment field is valid |
| FIN | Graceful close; no more data from sender |
| RST | Abort; connection is dead now |
| PSH | Push buffered data to the application (hint) |
| URG | Urgent pointer valid (rarely used) |
| ECE/CWR | Explicit congestion notification |

---

## 3. The three-way handshake

```text
Client                                Server
  | -------- SYN seq=x ---------------> |   SYN_SENT / SYN_RECV
  | <--- SYN+ACK seq=y ack=x+1 -------- |
  | -------- ACK ack=y+1 -------------> |   ESTABLISHED
```

Why three packets: each side must prove it can send and receive, and both sequence numbers must be acknowledged. Two packets would leave the server unsure the client received its SYN.

Capture:

```bash
sudo tcpdump -ni eth0 'tcp port 443 and tcp[tcpflags] & (tcp-syn|tcp-ack) != 0'
```

```text
10.0.0.5.51422 > 10.0.0.9.443: Flags [S], seq 3123456789, win 64240, options [mss 1460,sackOK,TS val 1 ecr 0,nop,wscale 7], length 0
10.0.0.9.443 > 10.0.0.5.51422: Flags [S.], seq 987654321, ack 3123456790, win 65160, options [mss 1460,sackOK,TS val 2 ecr 1,nop,wscale 7], length 0
10.0.0.5.51422 > 10.0.0.9.443: Flags [.], ack 1, win 502, length 0
```

- `S` = SYN, `S.` = SYN+ACK, `.` = ACK.
- `win 64240` before scaling; `wscale 7` multiplies by 128 after the handshake.
- `mss 1460` = 1500 MTU − 20 IP − 20 TCP.

If you see SYN sent repeatedly with no reply, the problem is path/firewall/drop, not the application. If you see SYN → RST, something is actively refusing.

---

## 4. Teardown and TCP states

### Graceful close

```text
Client                                Server
  | -------- FIN ---------------------> |   FIN_WAIT_1 / CLOSE_WAIT
  | <------- ACK ---------------------- |
  | <------- FIN ---------------------- |   FIN_WAIT_2 / LAST_ACK
  | -------- ACK ---------------------> |   TIME_WAIT / CLOSED
```

Either side can close first. A full close needs four segments because each direction is independent (TCP is full-duplex).

### State diagram (abridged)

```text
CLOSED
  | SYN
SYN_SENT ── SYN+ACK ──> ESTABLISHED
  |                        |
  |                    close()
  |                        v
  |                   FIN_WAIT_1
  |                        |
  |                 ACK ---+---> FIN_WAIT_2 --- FIN ---> TIME_WAIT --2MSL--> CLOSED
  |
  +── (server) SYN_RECV ── ACK ──> ESTABLISHED ── FIN ──> CLOSE_WAIT ── close() ──> LAST_ACK ── ACK ──> CLOSED
```

| State | Meaning | Typical owner |
|---|---|---|
| `LISTEN` | Waiting for connections | Server |
| `SYN_SENT` | SYN sent, no reply | Client |
| `SYN_RECV` | SYN received, handshake incomplete | Server |
| `ESTABLISHED` | Data can flow | Both |
| `FIN_WAIT_1` | Sent FIN, waiting for ACK | Closer |
| `FIN_WAIT_2` | FIN acknowledged, waiting for peer FIN | Closer |
| `TIME_WAIT` | Waiting 2×MSL after final ACK | Closer |
| `CLOSE_WAIT` | Peer closed; local app has not closed yet | Receiver of FIN |
| `LAST_ACK` | Sent FIN after CLOSE_WAIT, waiting for ACK | Receiver of FIN |
| `CLOSING` | Simultaneous close | Both |

### Why TIME_WAIT exists

Two reasons:

1. **Retransmitted FINs:** if the final ACK is lost, the peer retransmits FIN. TIME_WAIT lets the host re-ACK instead of treating the FIN as a new connection.
2. **Old duplicate segments:** a delayed segment from the old connection must not be accepted by a new connection reusing the same tuple.

Linux fixes the duration at 60 seconds regardless of MSL. High TIME_WAIT on a **client** making many short connections is normal but consumes ephemeral ports. On a **server**, TIME_WAIT means the server closed first; ideally the client closes first, or the server uses keep-alive.

Check:

```bash
ss -tan state time-wait | wc -l
ss -tan state time-wait | head
```

```text
State      Recv-Q Send-Q Local Address:Port   Peer Address:Port
TIME-WAIT  0      0      10.0.0.5:51422       10.0.0.9:443
TIME-WAIT  0      0      10.0.0.5:51423       10.0.0.9:443
```

`tcp_tw_reuse` allows reuse of TIME_WAIT sockets for **outbound** connections under certain conditions:

```bash
sysctl net.ipv4.tcp_tw_reuse
```

```text
net.ipv4.tcp_tw_reuse = 2
```

Values vary by kernel; `2` means loopback-only on newer kernels, `1` enables it more broadly. Do not enable `tcp_tw_recycle`; it was removed because it breaks NAT.

### CLOSE_WAIT is an application bug

`CLOSE_WAIT` means the peer sent FIN and the local application has not called `close()`. The kernel cannot clean it up. Many sockets stuck in `CLOSE_WAIT` = file descriptor leak in the app.

```bash
ss -tan state close-wait
ss -tanp state close-wait | head
```

```text
State      Recv-Q Send-Q Local Address:Port   Peer Address:Port   Process
CLOSE-WAIT 0      0      10.0.0.5:8080        10.0.0.9:51234    users:(("java",pid=4242,fd=812))
```

The fix is in the application: close the socket in a `finally`/defer, or fix the connection pool. Restarting hides it temporarily. Raising `tcp_fin_timeout` only affects `FIN_WAIT_2`, not `CLOSE_WAIT`.

### FIN_WAIT_2 stuck

The local side closed, peer ACKed the FIN, but never sent its own FIN. The socket waits. Linux `tcp_fin_timeout` (default 60 s) eventually reaps it; some peers (broken clients, load balancers) hold it longer.

---

## 5. RST vs timeout vs refused

These are different failures with different causes.

| Symptom | On the wire | Meaning |
|---|---|---|
| `Connection refused` | SYN → RST | Host reachable, nothing listening (or firewall REJECT) |
| `Connection timed out` | SYN retransmitted, no reply | Packet dropped: firewall DROP, route black hole, host down |
| `Connection reset by peer` | RST during ESTABLISHED | Process died/closed with unread data, firewall, LB timeout, conntrack |
| `No route to host` | ICMP unreachable | Routing failure or ICMP host-unreachable |
| `Cannot assign requested address` | No SYN | Ephemeral port exhaustion or bind to a missing IP |

### Why RST happens mid-connection

1. The application crashed or called `close()` with unread data in the receive buffer. TCP sends RST instead of FIN.
2. A firewall or NAT device expired the flow and sent RST on a later packet.
3. A load balancer idle timeout (often 60 s) closed the backend connection while the client kept it open.
4. `SO_LINGER` with timeout 0 forces RST on close.
5. Conntrack table full: new packets are dropped, and some middleboxes reset.
6. The peer rebooted and received a packet for an old connection.

**Distinguish:** if the RST arrives exactly N seconds after the last data, suspect an idle timeout. If it arrives immediately after a request, suspect the application.

### Why timeouts happen

- Firewall `DROP` (no ICMP, no RST).
- Security group/NACL blocks the port.
- Route points to a black hole.
- SYN queue overflow with `tcp_abort_on_overflow=0` (default) silently drops.
- Server is down but ARP is proxied by a router.

`tcpdump` shows retransmitted SYNs at 1, 2, 4, 8... seconds (exponential backoff). `ss -tan` shows `SYN-SENT` for the client.

```bash
ss -tan state syn-sent
```

```text
State      Recv-Q Send-Q Local Address:Port   Peer Address:Port
SYN-SENT   0      1      10.0.0.5:51424       10.0.0.9:443
```

---

## 6. `ss`: the socket inspector

`ss` reads kernel socket tables directly; it is faster and more accurate than `netstat`.

```bash
ss -tulpn
```

```text
Netid State  Recv-Q Send-Q Local Address:Port  Peer Address:Port Process
udp   UNCONN 0      0          127.0.0.53:53         0.0.0.0:*     users:(("systemd-resolve",pid=701,fd=13))
tcp   LISTEN 0      4096         0.0.0.0:22           0.0.0.0:*     users:(("sshd",pid=812,fd=3))
tcp   LISTEN 0      511        127.0.0.1:5432         0.0.0.0:*     users:(("postgres",pid=900,fd=5))
tcp   LISTEN 0      511          0.0.0.0:80           0.0.0.0:*     users:(("nginx",pid=1001,fd=6))
```

Flags:

| Flag | Meaning |
|---|---|
| `-t` | TCP |
| `-u` | UDP |
| `-l` | Listening only |
| `-n` | Numeric ports/addresses |
| `-p` | Process (needs root for other users) |
| `-a` | All sockets, not just connected |
| `-o` | Timers (keepalive, retransmit) |
| `-s` | Summary statistics |
| `-i` | Internal TCP info (cwnd, rtt, retrans) |
| `-e` | Extended socket info (uid, inode) |
| `-m` | Memory usage |

### Filtering by state

```bash
ss -tan state established
ss -tan state time-wait
ss -tan state close-wait
ss -tan state syn-sent
ss -tan state established '( sport = :443 )'
ss -tan dst 10.0.0.9
```

### TCP internals

```bash
ss -tin
```

```text
State  Recv-Q Send-Q Local Address:Port  Peer Address:Port
ESTAB  0      0      10.0.0.5:51422     10.0.0.9:443
	 cubic wscale:7,7 rto:204 rtt:0.842/0.219 ato:40 mss:1460 pmtu:1500 rcvmss:1460
	 advmss:1460 cwnd:10 bytes_sent:18432 bytes_acked:18432 segs_out:142 segs_in:98
	 data_segs_out:71 send 1.2Gbps lastsnd:12 lastrcv:12 lastack:12
	 pacing_rate 2.4Gbps delivery_rate 1.1Gbps
	 retrans:0/0 reordering:3
```

- `rtt` — smoothed round-trip / variance.
- `cwnd` — congestion window in MSS units.
- `retrans:0/0` — retransmits / total.
- `pmtu` — path MTU in use.
- `wscale` — window scaling.
- `send`/`delivery_rate` — throughput estimates.

A high retransmit count with low throughput points to loss; a low `cwnd` with no loss points to the receiver window or application backpressure.

### Socket memory and queues

`Recv-Q`/`Send-Q` on established sockets are **bytes**, not packets:

```bash
ss -tmn
```

```text
State  Recv-Q Send-Q Local Address:Port Peer Address:Port
ESTAB  0      131072 10.0.0.5:51422    10.0.0.9:443
	 skmem:(r0,rb131072,t0,tb2626560,f4096,w0,o0,bl0,d0)
```

`Send-Q` growing means the sender is producing faster than the network/peer can drain. `Recv-Q` growing means the application is not reading fast enough.

### `netstat` equivalents

If `ss` is unavailable:

```bash
netstat -tulpn
netstat -s | grep -i -E 'retrans|overflow|drop'
```

`netstat` is deprecated but still useful for the `-s` counters on older systems.

---

## 7. UDP

UDP is a thin wrapper over IP: source port, destination port, length, checksum. No handshake, no ordering, no retransmission, no congestion control (unless the application implements it).

| Property | TCP | UDP |
|---|---|---|
| Connection | Yes | No |
| Reliability | Yes | No |
| Ordering | Yes | No |
| Flow/congestion control | Yes | No |
| Header | 20–60 B | 8 B |
| Latency | Higher (handshake, ACKs) | Lower |
| Use cases | HTTP/1.1, SSH, databases | DNS, DHCP, QUIC/HTTP3, syslog, metrics, VoIP, games |

UDP is connectionless, but the kernel still tracks sockets and (for connected UDP) a 4-tuple. `ss -uan` shows bound sockets; `ss -uan state established` shows "connected" UDP sockets that called `connect()`.

```bash
ss -uanp
```

```text
State  Recv-Q Send-Q Local Address:Port Peer Address:Port Process
UNCONN 0      0      0.0.0.0:68        0.0.0.0:*     users:(("dhclient",pid=701,fd=6))
UNCONN 0      0      127.0.0.53:53     0.0.0.0:*     users:(("systemd-resolve",pid=702,fd=12))
```

### UDP failure modes

- **Packet loss is silent.** The application must detect it (DNS retries, QUIC recovery).
- **ICMP port unreachable** may be returned for a closed UDP port, but firewalls often drop it. `connect()` on UDP can succeed even if nothing is listening; the error arrives on a later `send`/`recv` as `ECONNREFUSED`.
- **Amplification:** UDP services that reply to spoofed sources are DDoS reflectors. Never expose open DNS/NTP/SSDP resolvers.
- **Fragmentation:** large UDP datagrams fragment; a lost fragment loses the whole datagram. QUIC avoids this with path MTU discovery.

Test UDP:

```bash
nc -u -l 9999
nc -u 127.0.0.1 9999
```

There is no handshake, so "connection refused" is not immediate. Type a line; if it arrives, the port is open.

---

## 8. MTU, MSS, and PMTUD

| Term | Meaning |
|---|---|
| MTU | Maximum payload a link can carry (1500 typical) |
| MSS | Maximum TCP payload = MTU − IP header − TCP header (1460 for IPv4/1500) |
| PMTUD | Path MTU Discovery: use DF, learn from ICMP "fragmentation needed" |
| PLPMTUD | Packetization-layer PMTUD; probe sizes without relying on ICMP (QUIC) |

### The black hole

```text
Client --1500--> Router --1400--> Server
```

The router cannot forward a 1500-byte packet on a 1400-byte link. With DF set (normal for TCP), it sends ICMP "fragmentation needed, MTU 1400" back. If a firewall blocks that ICMP, the client never learns and retransmits the same too-big packet forever. Small packets (ping, TLS ClientHello sometimes) work; large transfers hang.

Symptoms:

- SSH connects, then hangs on large output.
- TLS handshake stalls after ClientHello.
- `ping` works, `curl` hangs.
- Works from some networks, fails from others (different path MTU).

Test:

```bash
ping -M do -s 1472 -c 2 10.0.0.9      # 1472 + 28 = 1500
ping -M do -s 1372 -c 2 10.0.0.9      # 1400
```

If 1500 fails and 1400 works, the path MTU is ~1400. Confirm with:

```bash
tracepath 10.0.0.9
```

```text
 1?: [LOCALHOST]                      pmtu 1500
 1:  192.168.10.1                     0.512ms
 2:  10.0.0.1                         1.204ms pmtu 1400
 3:  10.0.0.9                         1.890ms reached
```

Fixes:

1. Fix the path: raise the tunnel/link MTU, or allow ICMP type 3 code 4.
2. Clamp MSS on the router/firewall: `iptables -t mangle -A FORWARD -p tcp --tcp-flags SYN,RST SYN -j TCPMSS --clamp-mss-to-pmtu`.
3. Reduce the interface MTU on the endpoint (last resort; affects all traffic).
4. Enable `tcp_mtu_probing=1` on Linux (slow, but recovers).

Do not "fix" it by lowering MTU everywhere. Find the constraining hop.

### Jumbo frames

MTU 9000 on storage/backend networks. Every device in the path — NICs, switches, routers, VLANs, overlays — must support it. A single 1500-byte hop breaks jumbo traffic in confusing ways. If you are not sure, stay at 1500.

---

## 9. TCP tuning that actually matters

| Knob | Default | When to change |
|---|---|---|
| `net.core.somaxconn` | 4096 (modern) | High connection rate; pair with app backlog |
| `net.ipv4.tcp_max_syn_backlog` | 4096 | SYN floods / slow clients |
| `net.ipv4.tcp_syncookies` | 1 | Keep on; protects SYN queue |
| `net.ipv4.tcp_tw_reuse` | 0/2 | Many short outbound connections |
| `net.ipv4.tcp_fin_timeout` | 60 | Rarely; affects FIN_WAIT_2 |
| `net.ipv4.tcp_keepalive_time` | 7200 | Lower for long-lived idle connections behind NAT/LB |
| `net.ipv4.tcp_congestion_control` | cubic | `bbr` for high-latency/lossy paths |
| `net.core.rmem_max` / `wmem_max` | 212992 | High-bandwidth long-fat networks |
| `net.ipv4.tcp_rmem` / `tcp_wmem` | auto-tuned | Only with evidence |

### Keepalive

```bash
sysctl net.ipv4.tcp_keepalive_time net.ipv4.tcp_keepalive_intvl net.ipv4.tcp_keepalive_probes
```

```text
net.ipv4.tcp_keepalive_time = 7200
net.ipv4.tcp_keepalive_intvl = 75
net.ipv4.tcp_keepalive_probes = 9
```

Default: first probe after 2 hours, then every 75 s, 9 probes → ~2h11m to detect a dead peer. Applications and load balancers usually implement their own keepalive at 30–60 s. If a NAT/LB idle timeout is 60 s and the app is silent for 65 s, the flow is dropped; a keepalive or application ping keeps it alive.

`ss -o` shows timers:

```bash
ss -ton state established
```

```text
ESTAB 0 0 10.0.0.5:51422 10.0.0.9:443 timer:(keepalive,1h59min,0)
```

### Congestion control

```bash
sysctl net.ipv4.tcp_congestion_control
sysctl net.ipv4.tcp_available_congestion_control
```

```text
net.ipv4.tcp_congestion_control = cubic
net.ipv4.tcp_available_congestion_control = reno cubic bbr
```

CUBIC is the default and works well in most cases. BBR can improve throughput on lossy/high-latency links but can be unfair to loss-based flows. Change it with evidence, not fashion.

### Nagle and delayed ACK

Nagle's algorithm batches small writes until an ACK arrives. Delayed ACK waits up to ~40 ms before ACKing. Together they can add 40 ms latency to request/response protocols that write headers and body separately.

- Fix in the application: `TCP_NODELAY`.
- Do not disable delayed ACK globally; it hurts bulk throughput.

---

## 10. Conntrack and NAT state

Netfilter tracks connections in `nf_conntrack`. NAT depends on it.

```bash
conntrack -L
```

```text
tcp      6 431997 ESTABLISHED src=10.0.0.5 dst=10.0.0.9 sport=51422 dport=443 src=10.0.0.9 dst=10.0.0.5 sport=443 dport=51422 [ASSURED] mark=0 use=1
```

```bash
sysctl net.netfilter.nf_conntrack_count net.netfilter.nf_conntrack_max
```

```text
net.netfilter.nf_conntrack_count = 18234
net.netfilter.nf_conntrack_max = 262144
```

When the table is full:

```text
nf_conntrack: table full, dropping packet
```

New connections fail while existing ones may survive. Causes: connection flood, too-low max, long timeouts, or a leak of half-open flows. Raise `nf_conntrack_max` and reduce timeouts, but find the source.

Check drop counters:

```bash
nstat -az | grep -i -E 'conntrack|listen|syn'
```

```text
TcpExtListenOverflows            12
TcpExtListenDrops                12
TcpExtTCPReqQFullDrop            0
TcpExtSyncookiesSent             0
```

- `ListenOverflows` > 0: accept queue full.
- `SyncookiesSent` > 0: SYN queue under pressure.
- `TCPAbortOnMemory`, `TCPReqQFullDrop`: resource exhaustion.

These counters are more useful than any single `ss` snapshot because they are cumulative.

---

## 11. Command field manual

### `ss` — sockets

**Purpose:** inspect listening sockets, connections, queues, and TCP internals.

```bash
ss -tulpn                 # listeners
ss -tan state established # active connections
ss -tin                   # TCP details
ss -s                     # summary
```

**Why:** it is the fastest way to answer "is the port open, who owns it, and is the queue full?"

**Mistake:** running `ss -p` as a non-root user and concluding a process is missing. Without root, other users' process info is hidden.

**Scenario:** a service is "up" but unreachable. `ss -tulpn` shows it bound to `127.0.0.1:8080`, not `0.0.0.0:8080`. The fix is the bind address, not the firewall.

### `nc` — connect and listen

```bash
nc -vz 10.0.0.9 443
nc -l 9999
nc 10.0.0.9 9999
```

```text
Connection to 10.0.0.9 443 port [tcp/https] succeeded!
```

- `-z` = scan without sending data.
- `-v` = verbose.
- `-l` = listen.
- `-u` = UDP.
- `-w 3` = timeout.

**Mistake:** using `nc -z` as a service health check. It proves the TCP handshake, not that the application works. A process can accept and immediately close.

### `tcpdump` — packets

```bash
sudo tcpdump -ni eth0 host 10.0.0.9 and port 443
sudo tcpdump -ni eth0 'tcp[tcpflags] & tcp-rst != 0'
sudo tcpdump -ni any -c 100 -w /tmp/cap.pcap
```

- `-c` stops after N packets.
- `-w` writes pcap; open in Wireshark for state analysis.
- `-A`/`-X` prints payload; be careful with credentials.
- `-s 0` captures full packets (default is already 262144 on modern tcpdump).

**Scenario:** SYN leaves but no SYN-ACK returns → upstream drop. SYN-ACK returns but the client never ACKs → client-side firewall or routing. Capture on both ends to localise.

### `conntrack` — NAT/state table

```bash
conntrack -L
conntrack -L -p tcp --dport 443
conntrack -S
```

`-S` shows per-CPU statistics, including `insert_failed`, `drop`, `early_drop`.

### `sysctl` — TCP knobs

```bash
sysctl net.ipv4.tcp_rmem net.ipv4.tcp_wmem
sysctl net.core.somaxconn
sysctl net.ipv4.tcp_syncookies
```

Read before writing. Record the old value before changing anything.

### `nstat` — kernel counters

```bash
nstat -az | grep -E 'TcpExt|Tcp'
```

`nstat` prints deltas since the last run by default; `-a` shows absolute values, `-z` includes zeros. Use it to confirm retransmits, listen overflows, and SYN cookie use.

---

## 12. Worked scenarios

### Scenario A — "connection refused" after deploy

```bash
curl -v http://10.0.0.9:8080/
```

```text
*   Trying 10.0.0.9:8080...
* connect to 10.0.0.9 port 8080 failed: Connection refused
```

Meaning: the host is reachable, but nothing is listening on 8080 (or a firewall rejected it).

Evidence:

```bash
ss -tulpn | grep 8080
journalctl -u myapp --since '10 min ago'
```

If `ss` shows nothing, the app failed to bind or crashed. If it shows `127.0.0.1:8080`, the app is bound to loopback. If it shows `0.0.0.0:8080`, check firewall REJECT rules and whether the request reached the right host.

### Scenario B — request hangs, then times out

```bash
curl -v --max-time 10 https://10.0.0.9/
```

```text
*   Trying 10.0.0.9:443...
* Operation timed out after 10001 milliseconds with 0 bytes received
```

Meaning: SYN got no response. Capture:

```bash
sudo tcpdump -ni eth0 host 10.0.0.9 and port 443
```

```text
10.0.0.5.51430 > 10.0.0.9.443: Flags [S], seq ..., length 0
10.0.0.5.51430 > 10.0.0.9.443: Flags [S], seq ..., length 0
```

Repeated SYNs with no reply → DROP rule, security group, or route black hole. Not the application.

### Scenario C — connections reset every 60 seconds

Facts: long-poll or WebSocket clients disconnect at ~60 s.

Hypothesis: idle timeout on a load balancer/NAT/firewall.

Evidence:

- Capture timestamps: RST or FIN exactly 60 s after last data.
- Check LB idle timeout config.
- Check `conntrack -L` timeout for the flow.
- Application keepalive interval.

Fix: keepalive/ping below the idle timeout, or raise the timeout. Do not just retry on the client.

### Scenario D — server has thousands of CLOSE_WAIT

```bash
ss -tan state close-wait | wc -l
```

```text
4821
```

This is an application leak. The peer closed; the server never did. Find the process:

```bash
ss -tanp state close-wait | awk 'NR>1 {print $NF}' | sort | uniq -c | sort -rn | head
```

Fix the code path that misses `close()` (often an exception path or a connection pool that does not evict dead connections). Restarting buys time.

### Scenario E — intermittent TLS handshake failures

Facts: some connections succeed, some hang; more failures from one region.

Check:

```bash
curl -v --max-time 5 https://example.com/
openssl s_client -connect example.com:443 -servername example.com
ping -M do -s 1472 -c 1 example.com
```

If small requests work and large handshakes fail, suspect MTU/PMTUD. If all requests from one region fail, suspect routing or firewall. If the handshake completes but the certificate is wrong, that is TLS, not TCP — see [HTTP, TLS, load balancing](07-http-tls-load-balancing.md).

### Scenario F — ephemeral port exhaustion

Facts: a service making outbound HTTP calls starts failing with `Cannot assign requested address`.

```bash
ss -tan state time-wait | wc -l
ss -tan | awk '{print $4}' | cut -d: -f2 | sort | uniq -c | sort -rn | head
sysctl net.ipv4.ip_local_port_range
```

If tens of thousands of TIME_WAIT sockets to the same destination exist, the client is opening a new connection per request. Fix: connection pooling/keep-alive. Widening the port range and enabling `tcp_tw_reuse` are mitigations, not fixes.

---

## 13. Common mistakes

| Mistake | Why it hurts | Fix |
|---|---|---|
| Checking only `ss -tulpn` without bind address | `127.0.0.1` looks "listening" | Read Local Address column |
| Treating `CLOSE_WAIT` as a network issue | It is an app leak | Fix close paths |
| Blaming TIME_WAIT for outages | Usually harmless | Count ephemeral usage; use pooling |
| Setting `tcp_tw_recycle=1` | Breaks NAT; removed in modern kernels | Don't |
| Lowering MTU everywhere for one tunnel | Hides the real path issue | Fix MTU/ICMP at the constraining hop |
| Using `nc -z` as an app health check | Only proves TCP handshake | Use an HTTP health endpoint |
| Ignoring `Recv-Q` on LISTEN | Accept queue overflow looks like random loss | Fix accept loop/backlog |
| Assuming RST = firewall | App crash/LB timeout also reset | Correlate timing and logs |
| Disabling syncookies | SYN flood takes the service down | Keep enabled |
| Reading `ss` queue values as packets | They are bytes for established sockets | Interpret per state |

---

## 14. Checkpoint

1. What are the three packets of the TCP handshake, and why three?
2. What does `SYN_SENT` on the client tell you?
3. What is the difference between `CLOSE_WAIT` and `TIME_WAIT`?
4. Why does TIME_WAIT last 2×MSL, and what breaks if you remove it?
5. What is the difference between connection refused and timeout on the wire?
6. What does `Recv-Q` mean for a LISTEN socket vs an ESTABLISHED socket?
7. What is MSS for a 1500-byte MTU over IPv4?
8. A large transfer hangs but ping works. What do you test?
9. Why can UDP `connect()` succeed when nothing is listening?
10. What counter proves the accept queue overflowed?
11. Why is `tcp_tw_recycle` dangerous?
12. What does `ss -tin` show that `ss -tan` does not?

### Answers

1. SYN, SYN+ACK, ACK. Three prove bidirectional reachability and synchronise both sequence numbers.
2. The SYN left but no SYN+ACK arrived: drop, route, or server down.
3. `CLOSE_WAIT` = peer closed, local app has not; `TIME_WAIT` = local side closed and is waiting 2×MSL.
4. To re-ACK lost FINs and prevent old duplicate segments from being accepted by a new connection.
5. Refused = RST; timeout = no response (DROP/black hole).
6. LISTEN: completed connections waiting to be accepted. ESTABLISHED: bytes received but not read by the app.
7. 1460 bytes (1500 − 20 IP − 20 TCP).
8. Path MTU: `ping -M do -s 1472`, `tracepath`; check for ICMP filtering.
9. UDP has no handshake; the kernel just records the peer. Errors arrive on later sends/receives.
10. `TcpExtListenOverflows` (and `ListenDrops`) from `nstat`/`netstat -s`.
11. It uses per-host timestamps and drops packets from NATed clients with different clocks; removed from Linux.
12. Congestion window, RTT, retransmits, MSS/PMTU, pacing, and delivery rate.

If you miss more than three, redo sections 3–5 and Lab L2.3.
