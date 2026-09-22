# Computer Networks → IPv4 Fragmentation & NAT

**GATE 2027 syllabus:** Computer Networks — IPv4: Fragmentation, NAT.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** The IPv4 header is 20 bytes (no options) unless stated otherwise. The fragment offset field is measured in units of 8 bytes. Each fragment's data length (except possibly the last) must be a multiple of 8 bytes. MTU includes the IP header.

---

## Questions

### Q1 · NAT · Fundamental
An IP datagram carries 4000 bytes of data plus a 20-byte header (4020 bytes total) and must be sent over a link with an MTU of 1500 bytes. Using the largest possible fragments, the number of fragments produced is ______.

---

### Q2 · MCQ · Fundamental
In IPv4, the "more fragments" (MF) flag is set to 0 in

- (A) every fragment of a fragmented datagram
- (B) only the first fragment
- (C) only the last fragment (or an unfragmented datagram)
- (D) only fragments carrying an odd offset

---

### Q3 · NAT · Standard
Using the scenario of Q1 (4000 bytes of data, 1500-byte MTU, 20-byte header on every fragment), the fragment-offset value (in 8-byte units) stored in the **last** fragment is ______.

---

### Q4 · MSQ · Standard
Which of the following statements about IPv4 fragmentation is/are TRUE?

- (A) Fragments of the same original datagram can be reassembled only at the final destination, not at an intermediate router.
- (B) All fragments of a datagram carry the same value in the Identification field.
- (C) Fragment reassembly requires the offset field, the more-fragments flag and the identification field.
- (D) A fragment can itself be fragmented further if it meets a link with an even smaller MTU.

---

### Q5 · NAT · Moderate
A 4020-byte datagram (20-byte header + 4000 bytes of data) is fragmented for a 1500-byte-MTU link, producing 3 fragments as in Q1. One of these fragments then reaches a second link with an MTU of only 620 bytes and must be fragmented again. The total number of fragments finally delivered to the destination (across all three original fragments) is ______.

---

### Q6 · MSQ · Moderate
A NAT router uses **port address translation**: for each outgoing (private IP, private port, destination IP, destination port) tuple, it allocates a translation table entry with a unique external port (if one doesn't already exist for that exact tuple) and rewrites the source address/port. Which of the following statements about this scheme is/are TRUE?

- (A) Two different internal hosts can be distinguished by external servers using only the external port number, even though they share one public IP.
- (B) The same internal host and port, talking to two different external (server IP, server port) destinations, will generally get two different external port numbers.
- (C) NAT eliminates the need for any translation table state in the router.
- (D) An external host cannot normally initiate a new connection directly to an internal host without additional configuration (e.g., port forwarding).

---

### Q7 · NAT · Moderate
A NAT router observes these outgoing connection attempts, each shown as (internal IP, internal port, destination IP, destination port), and assigns a new external port only when the exact 4-tuple has not been seen before: (10.0.0.5, 5000, 93.1.1.1, 80), (10.0.0.6, 5000, 93.1.1.1, 80), (10.0.0.5, 5001, 93.1.1.1, 80), (10.0.0.5, 5000, 93.1.1.1, 80), (10.0.0.5, 5000, 93.1.1.1, 443). The number of distinct NAT table entries created is ______.

---

### Q8 · MCQ · Hard
A datagram with Identification field 0x1234 is fragmented into 3 fragments by router R1, and then the middle fragment is further fragmented into 2 sub-fragments by router R2 (which has a smaller MTU). At the destination, reassembly is based on matching Identification, source and destination addresses. Which one of the following is TRUE about the Identification field of R2's two sub-fragments?

- (A) Each sub-fragment gets a new, distinct Identification field, different from 0x1234 and from each other.
- (B) Both sub-fragments keep the original Identification field 0x1234, exactly as the fragment they came from.
- (C) The sub-fragments' Identification fields are set to 0x1234 XOR (their offset).
- (D) Only one of the two sub-fragments keeps 0x1234; the other gets a new value.

---

### Q9 · MSQ · Hard
Which of the following statements about NAT is/are TRUE?

- (A) NAT breaks the end-to-end connectivity principle of the original Internet architecture.
- (B) NAT can complicate protocols that embed IP addresses or port numbers inside the application-layer payload (e.g., older FTP), because the payload itself is not rewritten by basic NAT.
- (C) A NAT router must always be also a router that changes the TTL field.
- (D) NAT alone can substitute for a stateful firewall, since unsolicited inbound packets to un-mapped external ports are dropped by default.

---

### Q10 · NAT · Killer
An 8960-byte datagram (20-byte header + 8940 bytes of data) is sent over a link with an MTU of 1500 bytes, producing several first-level fragments (each, except possibly the last, carrying the maximum multiple-of-8 data payload). Each of these first-level fragments is then further fragmented for a second link with an MTU of 620 bytes (each second-level fragment, except possibly the last one from each first-level fragment, carrying the maximum multiple-of-8 data payload). The total number of fragments that finally arrive at the destination is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | 3 | Max data/fragment = ⌊1480/8⌋×8 = 1480; ⌈4000/1480⌉ |
| 2 | C | MF = 0 marks the end of the fragment series |
| 3 | 370 | Fragments carry 1480, 1480, 1040 bytes of data; offsets 0, 185, 370 |
| 4 | A, B, C, D | All are standard IPv4 fragmentation facts |
| 5 | 8 | First fragment (1480 B data) → 3 sub-fragments (600,600,280); second (1480 B) → 3 (600,600,280); third (1040 B) → 2 (600,440); total 3+3+2 |
| 6 | A, B, D | (C) is false — NAT is inherently stateful |
| 7 | 4 | (10.0.0.5,5000,...,80) once, (10.0.0.6,5000,...,80), (10.0.0.5,5001,...,80), (10.0.0.5,5000,...,443); the repeated 4th tuple reuses the first entry |
| 8 | B | Fragments of the same original datagram keep the original Identification field so the destination can group them |
| 9 | A, B | (C) is false — NAT is a distinct function from TTL decrementing, though a NAT box often also routes; (D) is not guaranteed by NAT itself, though it is a common side effect |
| 10 | 19 | 8940 data → 6 fragments of 1480 bytes + 1 of 60 bytes (7 first-level fragments); each 1480-byte fragment splits into 3 second-level pieces (600, 600, 280), the 60-byte one stays whole; total 6×3 + 1 |
