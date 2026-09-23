# Computer Networks → Mixed Advanced Numerical (Multi-Topic)

**GATE 2027 syllabus:** Computer Networks — spans multiple sub-topics per question (performance metrics, error detection, switching, MAC, routing, IPv4, TCP), the way many real GATE questions combine two or three ideas at once.
**Set:** 10 questions (MCQ / NAT). Every question deliberately mixes at least two CN sub-topics. Difficulty ramps from standard (Q1–Q2) to a killer conceptual-plus-numerical question (Q10).
**Conventions:** Unless a question states otherwise: ignore processing delay everywhere; a store-and-forward switch/router fully receives a unit before forwarding any bit of it; TCP's slow start doubles cwnd every RTT with no loss and no ssthresh cap unless stated; 1 KB = 1000 bytes and 1 MB = 10⁶ bytes for transfer-time arithmetic (not 1024-based) unless a question says otherwise.

---

## Questions

### Q1 · NAT · Standard *(TCP congestion control + file transfer sizing)*
A 1,000,000-byte file is sent over a freshly opened, persistent TCP connection with MSS = 1000 bytes and RTT = 50 ms. The connection starts in slow start with cwnd = 1 MSS, doubling every RTT, and the underlying link is never the bottleneck (only cwnd limits how much is sent per RTT). Counting only whole RTT rounds needed to get every segment sent at least once (ignore the final ACK's own transit time), the total time, in milliseconds, is ______.

---

### Q2 · NAT · Standard *(Error detection + stop-and-wait efficiency)*
A stop-and-wait data-link protocol sends frames consisting of 950 bytes of data, a 20-byte header, and a 2-byte CRC (16-bit generator). The link rate is 1 Mbps and the one-way propagation delay is 20 ms. The ACK is 40 bits and is transmitted at the same link rate; ACK propagation delay is also 20 ms one-way. The sender may not send the next frame until it receives the ACK for the current one. The link's efficiency (fraction of the total cycle time spent transmitting the CRC-protected frame itself), as a percentage rounded to two decimal places, is ______.

---

### Q3 · NAT · Moderate *(IPv4 fragmentation + multi-hop store-and-forward delay)*
A 3980-byte IP payload (before fragmentation) must cross two links in series through one intermediate router, with an MTU of 1500 bytes (including the 20-byte IP header) forcing fragmentation before the first link. Using the largest possible fragments (data length a multiple of 8 bytes, 20-byte header on every fragment), the datagram is split into 3 fragments, sent back-to-back with no other traffic. The first link runs at 2 Mbps and the second at 1 Mbps; both routers use store-and-forward with no propagation delay and no further fragmentation is needed at the second link. The time at which the **last** fragment is fully received at the final destination, in milliseconds (rounded to two decimal places), is ______.

---

### Q4 · NAT · Moderate *(Link-state routing + bandwidth-limited transfer time)*
The weighted graph below gives routing cost (first number) and link rate (second number, in Mbps) for each edge: A–B (2, 10), A–C (5, 2), B–C (1, 5), B–D (4, 8), C–D (2, 4), C–E (7, 1), D–E (1, 20). A router at A must send a 5,000,000-byte file to E. It first computes the least-cost path to E using Dijkstra's algorithm, then sends the file along that path. Ignoring propagation and processing delay, and treating the path's slowest link as the bottleneck for sustained transfer, the total transmission time, in seconds, is ______.

---

### Q5 · NAT · Moderate *(Reliability/ARQ + probability)*
A stop-and-wait protocol is used over a lossy link. Each transmission attempt (of a frame, followed by waiting for its ACK) independently succeeds — meaning both the frame and its ACK arrive intact — with probability 0.8. A **successful** attempt takes 15 ms from the start of transmission to receipt of the ACK. A **failed** attempt is only detected after a fixed timeout of 25 ms, after which the frame is retransmitted. The expected total time to eventually deliver one frame successfully, in milliseconds, is ______.

---

### Q6 · NAT · Moderate *(HTTP/application layer + TCP slow start)*
A client fetches a 64-KB object (65536 bytes) over an already-established persistent HTTP connection with MSS = 1024 bytes and RTT = 100 ms. The TCP connection is in slow start (cwnd = 1 MSS, doubling every RTT, no loss, no bandwidth bottleneck). Counting only the whole RTT rounds needed until every segment of the object has been sent at least once, the total time to fetch the object, in milliseconds, is ______.

---

### Q7 · NAT · Hard *(Medium access control + sustained file-transfer throughput)*
An Ethernet-style LAN runs CSMA/CD at 10 Mbps. Two stations are 2000 m apart (signal speed 2×10⁸ m/s) and every frame carries exactly 1200 bytes. One station transmits a 10,000,000-byte file as a back-to-back sequence of such frames with no competing traffic (so every frame transmission is successful, but each frame still incurs the standard CSMA/CD efficiency penalty 1/(1 + 5a), where a = propagation delay / frame transmission time). The total time to deliver the file, in seconds (rounded to two decimal places), is ______.

---

### Q8 · MCQ · Hard *(CIDR longest-prefix match + distance-vector cost)*
A router's forwarding table has the following entries, each giving a prefix, the next-hop router that advertised a route to that exact prefix, and the distance-vector cost of that route:

| Prefix | Next hop | Cost |
|---|---|---|
| 10.0.0.0/8 | R1 | 4 |
| 10.16.0.0/12 | R2 | 2 |
| 10.16.32.0/20 | R3 | 9 |
| 10.16.32.0/20 | R4 | 3 |
| 10.16.32.0/25 | R5 | 7 |
| 10.16.32.0/25 | R6 | 5 |

The router must forward a packet destined for 10.16.32.10. It first applies longest-prefix matching to narrow down the candidate entries, then — if more than one entry shares that longest prefix — picks the one with the lowest distance-vector cost. The chosen next hop is

- (A) R3
- (B) R4
- (C) R5
- (D) R6

---

### Q9 · NAT · Hard *(TCP congestion control throughput model + transfer time)*
A long-lived TCP connection has RTT = 60 ms, MSS = 1460 bytes, and experiences independent random packet loss with probability p = 0.0001. Using the Mathis throughput approximation, throughput ≈ 1.22 × MSS / (RTT × √p) (throughput and MSS×8 both in bits), the connection is used to transfer a 100,000,000-byte (100 MB) file at this steady-state throughput. The transfer time, in seconds (rounded to two decimal places), is ______.

---

### Q10 · NAT · Killer *(HTTP + TCP connection setup sequencing + slow start, combined)*
A browser needs to fetch two 100-KB objects (102,400... use 100 segments each, MSS = 1024 bytes, i.e. 100 segments per object) from the same server, over **two separate, non-pipelined persistent TCP connections** opened one after another (the browser's policy is to complete one connection's handshake before starting the next one's handshake — they do not overlap). RTT = 80 ms for every handshake and every data segment round. Each connection independently uses slow start (cwnd = 1 MSS, doubling every RTT, no loss, no bandwidth bottleneck) for its own 100-segment data transfer, and a connection's data transfer begins immediately (same RTT slot) once its own handshake RTT completes. Counting from the start of the first connection's handshake, the time at which **both** objects have been completely received, in milliseconds, is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | 500 | 1000 segments; cumulative sent after RTT k is 2^k − 1; needs k = 10 (2¹⁰ − 1 = 1023 ≥ 1000); 10 × 50 |
| 2 | 16.26 | Frame = 972 B = 7776 bits, Tf = 7.776 ms; ACK = 40 bits, Tack = 0.04 ms; cycle = Tf + 2Tp + Tack = 47.816 ms; 7.776/47.816 |
| 3 | 38.32 | Fragment sizes with header: 1500, 1500, 1040 B. Fragment 3 leaves link 1 at 16.16 ms but link 2 is busy with fragment 2 until 30 ms, so it starts link 2 at 30 ms and finishes at 30 + 8.32 |
| 4 | 10 | Shortest path A–B–C–D–E (cost 6); bottleneck rate on that path is C–D at 4 Mbps; 5,000,000 × 8 / 4,000,000 |
| 5 | 21.25 | E[extra failed attempts] = (1 − 0.8)/0.8 = 0.25; total = 0.25 × 25 + 15 |
| 6 | 700 | 64 segments; cumulative after RTT k is 2^k − 1; needs k = 7 (2⁷ − 1 = 127 ≥ 64); 7 × 100 |
| 7 | 8.42 | a = (2000/2×10⁸)/(1200×8/10⁷) = 0.01/0.0096 ≈ 0.0104; efficiency = 1/(1+5a) ≈ 0.9505; effective rate ≈ 9.505 Mbps; time = 80,000,000 bits / 9.505×10⁶ |
| 8 | D | Longest matching prefix is /25 (both R5 and R6 qualify); lower cost wins: R6 (cost 5) over R5 (cost 7) |
| 9 | 33.69 | Throughput = 1.22 × 11680 / (0.06 × 0.01) ≈ 23.75 Mbps; time = 800,000,000 / 23,749,333 |
| 10 | 720 | Connection 1: handshake (RTT 1) + data (100 segments needs 7 RTTs: 2⁷−1=127≥100) finishes at RTT 8. Connection 2's handshake starts only after connection 1's handshake completes (RTT 1), so it finishes at RTT 2, and its data (7 more RTTs) finishes at RTT 9. Total = 9 × 80 |
