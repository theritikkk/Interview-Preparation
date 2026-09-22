# Computer Networks → Delays & Performance Metrics

**GATE 2027 syllabus:** Computer Networks — principles of layering, performance metrics.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** Transmission delay = packet size / link rate. Propagation delay = distance / signal speed. Processing and queueing delays are ignored unless stated. Bandwidth-delay product (BDP) = bandwidth × RTT.

---

## Questions

### Q1 · MCQ · Fundamental
A host sends a very large file over a link. As the link rate R increases (distance and packet size fixed), the transmission delay

- (A) increases
- (B) decreases
- (C) stays the same
- (D) first decreases, then increases

---

### Q2 · NAT · Fundamental
A 1500-byte packet is sent over a 100 Mbps link. The transmission delay in microseconds is ______.

---

### Q3 · NAT · Fundamental
A message of 1,000,000 bits is sent over a path of two links in series (a store-and-forward switch in between), each of rate 10 Mbps. There is no propagation or processing delay. The total time to deliver the message, in milliseconds, is ______.

---

### Q4 · NAT · Standard
A link drops each packet independently with probability 0.2. Ten packets are sent. The probability (rounded to two decimal places) that **more than 3** packets are dropped is ______.

---

### Q5 · MSQ · Standard
Which of the following statements about delay is/are TRUE?

- (A) Propagation delay depends on the physical length of the link, not on the link's transmission rate.
- (B) Transmission delay depends on the packet size and the link rate, not on the link's physical length.
- (C) In store-and-forward switching, an intermediate switch must receive an entire packet before forwarding any bit of it.
- (D) Queueing delay is zero whenever the link's transmission rate is very high.

---

### Q6 · NAT · Standard
A link has a rate of 100 Mbps and a one-way propagation delay of 10 ms. The bandwidth-delay product, expressed as a number of 1250-byte packets that can be "in flight" (rounded down), is ______.

---

### Q7 · NAT · Moderate
A protocol sends 1000-byte data packets, each with a 20-byte header. Every data packet is followed by a separate 20-byte acknowledgement packet with its own 18-byte header, and the sender must wait for the ack before sending the next packet (headers count toward transmission time; ignore propagation and processing delay). The efficiency (fraction of transmitted bits, header and data of the data packet included, that are useful data) rounded to one decimal place, as a percentage, is ______.

---

### Q8 · MCQ · Moderate
Host A sends packets to host B over a path of 3 links with propagation delays d₁ < d₂ < d₃ and transmission delays t₁, t₂, t₃ at the respective links. For a single packet sent with no other traffic, the total end-to-end delay is

- (A) max(t₁, t₂, t₃) + d₁ + d₂ + d₃
- (B) t₁ + t₂ + t₃ + max(d₁, d₂, d₃)
- (C) t₁ + t₂ + t₃ + d₁ + d₂ + d₃
- (D) max(t₁ + d₁, t₂ + d₂, t₃ + d₃)

---

### Q9 · NAT · Hard
Five 12,000-bit packets are sent back-to-back (with no gap) from a source through two intermediate store-and-forward switches to a destination. The three links have rates 10 Mbps, 5 Mbps and 20 Mbps and one-way propagation delays 2 ms, 4 ms and 1 ms respectively. The time (in ms) at which the **last** packet is fully received at the destination is ______.

---

### Q10 · NAT · Killer
A file of 24,000 bytes is to be sent as N equal-sized packets, each with an 180-byte header (so a packet of payload size x bytes takes x + 180 bytes to transmit). The link can carry k = 4 packets "in flight" before an acknowledgement is required, and each batch of up to k packets must be fully transmitted, then one round trip elapses, before the next batch starts (ignore the ack's own transmission time; the link rate is such that one byte takes exactly 1 time unit to transmit, and one round trip is negligible compared to transmission, i.e. contributes 0 extra time units, so total time = total bytes transmitted across all packets). The payload size x (dividing 24,000 evenly) that minimizes the total number of bytes transmitted (payload + headers) is ______ bytes.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | B | Delay = size / rate |
| 2 | 120 | 1500·8 / 100·10⁶ = 1.2·10⁻⁴ s |
| 3 | 200 | Two links in series, no propagation delay: 2 × (10⁶ bits / 10⁷ bps) = 2 × 100 ms |
| 4 | 0.12 | 1 − P(X ≤ 3) for Binomial(10, 0.2) |
| 5 | A, B, C | (D) is false; queueing depends on arrival pattern, not just rate |
| 6 | 100 | BDP = 10⁸ × 0.01 = 10⁶ bits = 125,000 bytes; 125,000 / 1250 |
| 7 | 94.5 | 8000 / (8000 + 160 + 160 + 144) |
| 8 | C | Delays add along a single path with no cross traffic |
| 9 | 20.8 | Track each link's queue: last packet departs link 1 at 6 ms, link 2 at 15.8 ms, link 3 at 19.8 ms + propagation |
| 10 | 1200 | Minimize N·180 + 24000 over divisors N of 24000/x; smallest N ≥ 1 with x dividing 24000 evenly balances header overhead — N = 20 packets of 1200 bytes |
