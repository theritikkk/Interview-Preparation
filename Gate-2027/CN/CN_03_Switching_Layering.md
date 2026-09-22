# Computer Networks → Layering & Switching

**GATE 2027 syllabus:** Computer Networks — principles of layering; basics of switching (circuit, packet, virtual circuit) and performance metrics.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** OSI layers, bottom to top: Physical, Data Link, Network, Transport, Session, Presentation, Application. Circuit switching reserves a fixed rate for the whole call; packet switching (statistical multiplexing) shares the link on demand.

---

## Questions

### Q1 · MCQ · Fundamental
In the layered (OSI-style) network architecture, encapsulation means that

- (A) each layer compresses the data it receives from the layer above
- (B) each layer adds its own header (and sometimes trailer) around the data unit passed down from the layer above
- (C) each layer encrypts the payload before passing it down
- (D) each layer strips the header added by the layer above

---

### Q2 · MSQ · Fundamental
Which of the following functions is/are typically performed at the **network** layer (not the data link or transport layer)?

- (A) Logical (IP) addressing
- (B) Routing between networks
- (C) Framing and MAC addressing
- (D) End-to-end reliable delivery using sequence numbers and acknowledgements

---

### Q3 · NAT · Standard
A call requires a dedicated circuit for 500 ms of setup followed by transmission of a 1 Mbit message over a 2 Mbps circuit-switched path. A packet-switched alternative sends the same message as 125 packets of 1000 bytes each, store-and-forward, over 4 hops of rate 20 Mbps each, sent back-to-back with no other traffic (ignore propagation and processing delay). (Circuit-switched total time) − (packet-switched total time), in seconds, is ______  (round to two decimal places).

---

### Q4 · NAT · Standard
A link of capacity 10 Mbps is shared using **circuit switching** among users who each need a dedicated rate of 1 Mbps while active. The maximum number of users the link can support simultaneously (whether active or not) is ______.

---

### Q5 · MSQ · Standard
Which of the following statements about virtual-circuit (VC) packet switching is/are TRUE?

- (A) A VC requires a call setup phase before data transfer.
- (B) Every packet in a VC carries a full destination address, just as in datagram switching.
- (C) Switches maintain per-connection state (a virtual-circuit table) in VC switching.
- (D) All packets of a connection follow the same fixed path through the network.

---

### Q6 · NAT · Moderate
Twenty users share a statistically multiplexed (packet-switched) link of capacity 10 Mbps. Each user is independently active with probability 0.3 and, when active, needs 1 Mbps. Using the exact binomial probability, the probability (rounded to four decimal places) that the instantaneous demand **exceeds** the link capacity is ______.

---

### Q7 · MSQ · Moderate
Which of the following is/are advantages of packet switching over circuit switching for **bursty** traffic?

- (A) Higher link utilization, since idle users do not reserve bandwidth
- (B) Guaranteed constant delay for every packet
- (C) No call-setup delay is required before the first packet can be sent
- (D) Better support for a larger number of users with low average rate demands

---

### Q8 · MCQ · Moderate
Which one of the following best describes the relationship between the **service** a layer provides and the **protocol** it uses?

- (A) They are the same thing; every layer has exactly one protocol and one service.
- (B) The service defines what operations a layer offers to the layer above; the protocol defines the rules (message formats, timing) used by peer entities of that layer on different machines to implement the service.
- (C) The protocol is always visible to the layer above, while the service is hidden.
- (D) A single protocol can never support more than one possible service.

---

### Q9 · NAT · Hard
A network path has 6 intermediate **circuit** switches, each requiring 50 ms of setup signalling delay per switch (setup at successive switches happens sequentially, not in parallel), followed by transmission of a 10 Mbit file over the resulting circuit at 5 Mbps. The alternative is **packet switching** (datagram, store-and-forward) over the same 6 hops, each of rate 5 Mbps, sending the file as 625 packets of 2000 bytes each, back-to-back, with no setup delay (ignore propagation and processing delay). (Circuit-switched total time) − (packet-switched total time), in seconds, is ______  (round to two decimal places).

---

### Q10 · MSQ · Killer
Twenty-five users independently need 1 Mbps of bandwidth with probability 0.3 each, on a link of capacity C Mbps using statistical multiplexing. Which of the following values of C keep the probability that demand exceeds capacity **below 0.05**? (Use the exact binomial distribution.)

- (A) C = 10
- (B) C = 11
- (C) C = 12
- (D) C = 13

(Here demand is Binomial(25, 0.3) Mbps.)

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | B | Encapsulation wraps, not strips or compresses |
| 2 | A, B | (C) is data link; (D) is transport |
| 3 | 0.95 | Circuit: 0.5 + 0.5 = 1.00 s; packet: (hops − 1 + N)·(pkt bits/rate) = 128 × 0.0004 = 0.0512 s |
| 4 | 10 | 10 Mbps / 1 Mbps |
| 5 | A, C, D | (B): VC packets carry a short connection identifier, not a full address |
| 6 | 0.0171 | P(X ≥ 11) for Binomial(20, 0.3) |
| 7 | A, C, D | Packet switching gives no delay guarantee, so (B) is false |
| 8 | B | Classic OSI service/protocol distinction |
| 9 | 0.28 | Circuit: 6 × 0.05 + 2 = 2.30 s; packet: (6 − 1 + 625) × (16000/5×10⁶) = 630 × 0.0032 = 2.016 s |
| 10 | C, D | Binomial(25, 0.3): P(X > 10) ≈ 0.098, P(X > 11) ≈ 0.044, P(X > 12) ≈ 0.018 (< 0.05), P(X > 13) ≈ 0.006 (< 0.05) |
