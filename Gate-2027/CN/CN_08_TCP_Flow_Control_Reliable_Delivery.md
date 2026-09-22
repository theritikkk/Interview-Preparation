# Computer Networks → TCP: Flow Control & Reliable Delivery

**GATE 2027 syllabus:** Computer Networks — TCP: flow control; socket API.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** TCP sequence numbers are 32 bits, counting bytes. RTT is round-trip time. The receive window (RWND) limits how many unacknowledged bytes the sender may have in flight due to flow control (ignore congestion control in this file — that is covered separately). Go-Back-N (GBN) and Selective Repeat (SR) are evaluated with a fixed window of N segments, cumulative ACKs for GBN, and per-segment ACKs for SR; a lost segment is detected by timeout and triggers exactly the retransmissions described by the protocol.

---

## Questions

### Q1 · NAT · Fundamental
A TCP connection runs over a link of rate 100 Mbps. Assuming the sequence number space must not wrap around while old segments could still be in the network, the time (in seconds) for the 32-bit sequence number to wrap around completely, at full link rate, is ______  (round to the nearest integer).

---

### Q2 · MCQ · Fundamental
In TCP, an ACK with acknowledgement number X means

- (A) byte number X has just been received
- (B) all bytes up to and including byte X − 1 have been received; byte X is next expected
- (C) exactly X bytes have been sent so far
- (D) the receiver's window size is X bytes

---

### Q3 · NAT · Standard
A TCP receiver advertises a fixed window of 64 KB (65536 bytes) and the RTT is 40 ms. Assuming the sender can always fill the window and is limited only by this window (not by congestion control or the underlying link rate), the maximum achievable throughput, in Mbps, is ______  (round to two decimal places).

---

### Q4 · NAT · Standard
A link has bandwidth 10 Mbps and RTT 30 ms. The minimum TCP window size (in bytes) needed to keep the link continuously busy (i.e., at least the bandwidth-delay product) is ______.

---

### Q5 · MSQ · Standard
Which of the following statements about TCP flow control is/are TRUE?

- (A) The receiver advertises its available buffer space in every ACK segment via the window field.
- (B) Flow control protects the receiver from being overwhelmed; it is unrelated to network congestion.
- (C) If the receiver advertises a window of 0, the sender must stop sending data until a subsequent segment announces a non-zero window (persist timer probing aside).
- (D) TCP's flow-control window can never be smaller than the maximum segment size (MSS).

---

### Q6 · NAT · Moderate
A sender uses a sliding window of size N = 16 segments to communicate with a receiver whose acknowledgement numbers must always be unambiguous. To avoid any ambiguity between a new set of segments and an old, already-acknowledged set circulating late on the network, the sequence-number space must contain strictly more than ______ distinct values usable within one window's worth of outstanding segments (i.e., > 2N).

---

### Q7 · MSQ · Moderate
Which of the following statements about the Go-Back-N and Selective-Repeat ARQ protocols is/are TRUE?

- (A) In Go-Back-N, a single lost segment forces retransmission of that segment and all segments sent after it that were already in flight.
- (B) In Selective Repeat, the receiver may buffer out-of-order segments that arrive after a gap, instead of discarding them.
- (C) Go-Back-N requires the receiver to buffer out-of-order segments.
- (D) Selective Repeat generally requires a larger receiver window/buffer management than Go-Back-N for the same throughput.

---

### Q8 · NAT · Moderate
A sender uses Go-Back-N with window size N = 4 to send 10 segments (numbered 1 to 10) to a receiver. Segment 5 is lost on its first transmission attempt (every other segment, and every retransmission, is delivered successfully). The total number of segment transmissions (original sends plus retransmissions) needed to deliver all 10 segments is ______.

---

### Q9 · NAT · Hard
The same scenario as Q8, but now using **Selective Repeat** (per-segment ACKs; only the lost segment is retransmitted once the sender notices the gap) with window size N = 4. The total number of segment transmissions needed to deliver all 10 segments is ______.

---

### Q10 · NAT · Killer
A sender uses Go-Back-N with window size N = 3 to send 9 segments (numbered 1 to 9). Segments 4 and 7 are each lost on their first transmission attempt (every other transmission succeeds). The total number of segment transmissions needed to deliver all 9 segments is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | 344 | 2³² × 8 bits / 10⁸ bps |
| 2 | B | Cumulative acknowledgement semantics |
| 3 | 13.11 | 65536 × 8 / 0.04 / 10⁶ |
| 4 | 37500 | 10⁷ × 0.03 / 8 |
| 5 | A, B, C | (D) is false; the window can shrink below MSS (silly window syndrome territory) |
| 6 | 32 | Sequence space must exceed 2N |
| 7 | A, B, D | (C) is false — that is exactly what distinguishes SR from GBN |
| 8 | 14 | Segments 1–4 sent (4); window slides; 5–8 sent (4) but 5 lost, so on timeout resend 5,6,7,8 (4) — later 9,10 need 2 more sends: 4+4+4+2 |
| 9 | 11 | 1–4 (4), 5–8 (4, only 5 lost and not acked), 9–10 sent once window frees (2), then 5 alone retransmitted (1): 4+4+2+1 |
| 10 | 15 | 1–3 (3), 4–6 (3, 4 lost) → resend 4,5,6 (3), 7–9 sent (3, 7 lost) → resend 7,8,9 (3): 3+3+3+3+3 |
