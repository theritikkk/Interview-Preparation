# Computer Networks → Medium Access Control & Ethernet

**GATE 2027 syllabus:** Computer Networks — data link layer: Medium Access Control, Ethernet.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** Signal propagation speed 2 × 10⁸ m/s unless stated otherwise. Slotted ALOHA / pure ALOHA are evaluated at their throughput-maximizing offered load unless stated otherwise. CSMA/CD efficiency ≈ 1 / (1 + 5a), where a = propagation delay / transmission delay of the minimum frame.

---

## Questions

### Q1 · NAT · Fundamental
Two stations are 1 km apart on a 10 Mbps Ethernet-style link; the signal speed is 2 × 10⁸ m/s. To guarantee that a collision is detected by the sender before it finishes sending, the minimum frame size, in bytes, is ______.

---

### Q2 · MCQ · Fundamental
Which one of the following statements about CSMA/CD is TRUE?

- (A) It completely eliminates the possibility of collisions.
- (B) A station listens to the channel while transmitting and aborts immediately on detecting a collision.
- (C) It requires a central arbiter to grant channel access.
- (D) It guarantees a bounded maximum access delay for every station.

---

### Q3 · MCQ · Standard
The maximum possible channel utilization (throughput as a fraction of capacity) of **slotted ALOHA**, at the offered load that maximizes it, is approximately

- (A) 1/e ≈ 0.368
- (B) 1/(2e) ≈ 0.184
- (C) 0.5
- (D) 1.0

---

### Q4 · NAT · Standard
For a CSMA/CD LAN with a = (propagation delay)/(transmission delay of the minimum frame) = 0.1, the efficiency 1/(1 + 5a), expressed as a percentage and rounded to the nearest integer, is ______.

---

### Q5 · MSQ · Standard
Which of the following statements about Ethernet switches (as opposed to hubs) is/are TRUE?

- (A) A switch forwards a frame only out of the port leading to the destination MAC address, when that address is known.
- (B) Every port of a switch is a separate collision domain.
- (C) A switch is a Layer-3 (network layer) device.
- (D) A switch can interconnect segments running at different speeds (e.g., 100 Mbps and 1 Gbps).

---

### Q6 · NAT · Moderate
Under the binary exponential backoff algorithm, after the k-th collision (k ≥ 1) a station picks a random number of slots to wait, uniformly from {0, 1, …, 2^k − 1}. The expected number of slots waited after exactly the 4th collision is ______  (as a decimal, e.g. 3.5).

---

### Q7 · MCQ · Moderate
A switch learns MAC addresses by examining the **source** address of every incoming frame and recording the port it arrived on. When a frame's destination MAC address is not yet in the switch's forwarding table, the switch

- (A) discards the frame
- (B) floods the frame out of every port except the one it arrived on
- (C) broadcasts an ARP request to discover the port
- (D) buffers the frame until the address is learned

---

### Q8 · NAT · Moderate
A switch forwarding table currently has no entries. Frames arrive in this order (source MAC, destination MAC, arrival port): (A,B,1), (B,A,2), (C,A,3), (A,C,1), (D,B,4), (B,D,2). The number of these six frames that the switch **floods** (forwards out of more than one port because the destination is unknown at that point) is ______.

---

### Q9 · MSQ · Hard
Which of the following statements about pure (unslotted) ALOHA versus slotted ALOHA is/are TRUE?

- (A) In pure ALOHA, a transmitted frame is safe from collision only if no other station transmits during a window of length 2× the frame time centered on it.
- (B) Slotted ALOHA doubles the maximum throughput of pure ALOHA.
- (C) Slotted ALOHA requires all stations to be synchronized to slot boundaries.
- (D) In slotted ALOHA, a collision can still occur even if only one station transmits in a slot.

---

### Q10 · NAT · Killer
On a CSMA/CD LAN, N stations each independently attempt to transmit in a given slot with probability p. The probability that the slot is used successfully by exactly one station is p(1 − p)^(N−1) × N, maximized over p at p = 1/N, giving a maximum success probability of (1 − 1/N)^(N−1). For N = 20, this maximum probability, rounded to three decimal places, is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | 13 | 2 × (1000 m / 2×10⁸) × 10×10⁶ = 100 bits = 12.5 bytes, round up |
| 2 | B | Listen-while-transmit is the "CD" in CSMA/CD |
| 3 | A | 1/e |
| 4 | 67 | 1/(1 + 0.5) |
| 5 | A, B, D | (C) a plain switch is Layer 2 |
| 6 | 7.5 | Uniform over {0,…,15}: mean 7.5 |
| 7 | B | Unknown destination → flood |
| 8 | 4 | Floods: (A,B,1), (B,A,2), (D,B,4), (B,D,2). Unicasts: (C,A,3) — A already known; (A,C,1) — C became known from frame 3 |
| 9 | A, B, C | (D) is false: with exactly one transmitter in a slot there is nothing to collide with |
| 10 | 0.377 | (1 − 1/20)¹⁹ = 0.95¹⁹ |
