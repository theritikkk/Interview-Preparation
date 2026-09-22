# Computer Networking

🔴 This phase carries more weight for you than for a typical candidate — you built a packet-capture engine (NetSight) that parses the protocol stack directly. Interviewers who see that on your resume will go deeper here than they would otherwise.

---

## Cluster 1: The Protocol Stack — Ethernet, ARP, IP

### What is it?
Networking is layered: **Ethernet** (data link — MAC addresses, frames, VLANs) carries **IP** packets (network layer — routing, IP addresses) which carry **TCP/UDP** segments (transport layer — ports, reliability). **ARP** resolves an IP address to a MAC address on the local network segment.

### How does it work?
```
Ethernet Frame [ dst MAC | src MAC | (VLAN tag) | EtherType | IP Packet | FCS ]
IP Packet      [ version | header len | ... | src IP | dst IP | TCP/UDP Segment ]
TCP Segment    [ src port | dst port | seq# | ack# | flags | ... | payload ]
```
NetSight's stated protocol stack — Ethernet, VLAN, IPv4, TCP/UDP/ICMP — is exactly this layering, parsed directly from raw captured frames rather than through the kernel's normal socket abstraction.

### Interview Questions
- **L1:** "Ethernet handles local delivery via MAC addresses; IP handles routing across networks via IP addresses; TCP/UDP add ports and (for TCP) reliability on top."
- **L2:** explain what a VLAN tag does (logically segments a physical network into isolated broadcast domains) and why NetSight needs to explicitly parse it (an EtherType of `0x8100` signals a VLAN tag is present, shifting where the real payload starts — a parser that doesn't handle this misreads every tagged frame).
- **L3:** explain ARP's role precisely: before a host can send an Ethernet frame to an IP address on its local network, it needs the destination's MAC address — ARP broadcasts "who has this IP?" and caches the reply. This matters for a packet-capture tool because ARP traffic is itself something you might need to recognize and classify, separately from IP traffic.

### Resume Connection
🔴 This is literally what NetSight's parser does — "sustaining line-rate capture across a full protocol stack (Ethernet, VLAN, IPv4, TCP/UDP/ICMP)" is this cluster, implemented in C, at speed.

---

## Cluster 2: TCP vs UDP

### What is it?
TCP is connection-oriented, reliable, ordered, and flow/congestion-controlled (via the three-way handshake, sequence numbers, acknowledgments, retransmission). UDP is connectionless, unordered, and has no reliability guarantees — you get lower overhead and lower latency in exchange for handling loss/ordering yourself if you need it.

### How does it work?
Three-way handshake: `SYN` → `SYN-ACK` → `ACK` establishes a TCP connection, exchanging initial sequence numbers so both sides can detect loss/reordering afterward. TCP flow control (receiver-advertised window) prevents overwhelming a slow receiver; congestion control (e.g., slow start, congestion avoidance) prevents overwhelming the *network*.

### Interview Questions
- **L1:** "TCP is reliable and ordered via handshaking, sequence numbers, and retransmission; UDP is fire-and-forget with lower overhead."
- **L2:** explain why a video call typically uses UDP (a late/out-of-order retransmitted frame is worse than a dropped one) while a file transfer uses TCP (correctness matters more than latency).
- **L3 — 🔴 NetSight-specific:** explain how you'd track TCP connection *state* from raw captured packets without OS help — i.e., inferring a connection's lifecycle (SYN → SYN-ACK → ACK → data → FIN/RST) purely by observing the flag bits and sequence numbers on the wire, which is exactly what your stated "TCP connection lifecycle" eBPF tracing and your 1M-entry flow table are doing.

### Common Mistakes
- Saying "UDP is unreliable" as if that's purely a downside — it's a deliberate design tradeoff (no retransmission delay, no handshake latency) appropriate for specific workloads (DNS queries, real-time media, and — relevant to your NetSight project — often used for high-throughput telemetry precisely because it avoids TCP's overhead).

---

## Cluster 3: HTTP Evolution & TLS

### What is it?
HTTP/1.1: one request per TCP connection at a time by default (head-of-line blocking without pipelining tricks). HTTP/2: multiplexes many requests over one TCP connection (solves HTTP-level head-of-line blocking, but a single lost TCP packet still stalls all multiplexed streams — TCP-level head-of-line blocking remains). HTTP/3: runs over QUIC (UDP-based) instead of TCP, so one lost packet only stalls its own stream, not all of them.

### How does it work?
TLS handshake (simplified, TLS 1.3): client and server negotiate a cipher suite and exchange keys (often in a single round trip in TLS 1.3, down from two in TLS 1.2), then all subsequent traffic is encrypted with the negotiated session key.

### Interview Questions
- **L1:** "HTTP/1.1 is one request per connection; HTTP/2 multiplexes over one TCP connection; HTTP/3 moves to QUIC/UDP to eliminate TCP-level head-of-line blocking entirely."
- **L2:** explain what head-of-line blocking is at both the HTTP layer (HTTP/1.1) and the transport layer (HTTP/2 over TCP) — and why HTTP/3 specifically targets the second one.
- **L3:** explain why your API Gateway → Lambda (Hermes) or API Gateway → coordinator (Task Scheduler) traffic almost certainly runs over HTTPS/TLS by default in a managed cloud environment, and what TLS termination means (where encryption ends — often at a load balancer/gateway, with plaintext traffic internally in a trusted network).

### Resume Connection
Every REST API across your three projects sits on this stack; NetSight sits one layer *below* it (parsing the raw transport-layer bytes HTTP itself rides on) — a genuinely interesting contrast to bring up if asked "how does your systems project relate to your web projects?"

---

## Cluster 4: WebSockets & gRPC

⚠️ Both listed as skills, not tied to a specific project bullet — prepare conceptually.

### What is it?
WebSockets: a single persistent, full-duplex TCP connection after an HTTP upgrade handshake — good for continuous bidirectional traffic (chat, live updates) instead of repeated request/response polling. gRPC: RPC framework over HTTP/2, using Protocol Buffers for compact, strongly-typed, binary serialization instead of JSON.

### Interview Questions
- **L1:** "WebSockets give a persistent full-duplex connection for real-time bidirectional data; gRPC gives strongly-typed, efficient binary RPC over HTTP/2, generally for service-to-service communication rather than browser clients."
- **L2:** explain why gRPC's use of Protocol Buffers (binary, schema-defined) is typically faster to serialize/deserialize and smaller on the wire than JSON, at the cost of not being human-readable and needing generated client/server code from a `.proto` schema.

---

## Cluster 5: Load Balancers, Reverse Proxies, Connection Pooling, NAT

### What is it?
A load balancer distributes incoming traffic across multiple backend instances (by round-robin, least-connections, or other strategies). A reverse proxy sits in front of backend servers, forwarding client requests (often also handling TLS termination, caching, or routing). Connection pooling reuses established connections (to a database, or between services) instead of paying handshake cost per request. NAT translates private IPs to a public one at a network boundary.

### Interview Questions
- **L1:** "A load balancer spreads traffic across multiple instances for scalability and fault tolerance; a reverse proxy sits in front of your services doing routing/TLS/caching; connection pooling avoids re-establishing expensive connections repeatedly."
- **L2:** explain, specifically for your Task Scheduler's API gateway/coordinator/worker-fleet design, why a load balancer in front of the API layer is a natural fit for horizontal scaling, and why the coordinator itself needs separate thought about whether it's a single point of failure (see `13_PROJECT_DEEP_DIVES/DISTRIBUTED_TASK_SCHEDULER.md` once generated).
- **L3:** explain why database connection pooling matters specifically under the kind of concurrent load your Task Scheduler was tested with (10,000 simultaneous tasks) — without pooling, each worker opening a fresh PostgreSQL connection per task would itself become a bottleneck and could exhaust the database's max-connections limit well before the application logic becomes the constraint.

### Quick Revision
- Ethernet (MAC) → IP (routing) → TCP/UDP (ports + optional reliability) — exactly NetSight's parse stack.
- TCP: handshake, reliable, ordered. UDP: no handshake, no guarantees, lower overhead.
- HTTP/2 multiplexes over TCP (still has TCP-level head-of-line blocking); HTTP/3 (QUIC/UDP) removes it.
- Zero-copy capture and TCP state tracking (NetSight) happen *below* the HTTP layer your backend APIs live at.
- Connection pooling prevents connection-count exhaustion under high concurrent load — directly relevant to your Task Scheduler's 10,000-task test.
