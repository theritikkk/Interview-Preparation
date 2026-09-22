# Computer Networks → TCP: Congestion Control

**GATE 2027 syllabus:** Computer Networks — TCP: congestion control.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** cwnd is measured in MSS-sized units (so cwnd = 1 means one MSS may be outstanding). Slow start: cwnd doubles every RTT until cwnd ≥ ssthresh. Congestion avoidance: cwnd increases by 1 (additive increase) every RTT. On a **timeout**, ssthresh is set to max(cwnd/2, 1) [integer division rounds down] and cwnd resets to 1, then slow start resumes. Unless stated otherwise, cwnd starts at 1 and ssthresh starts at 16.

---

## Questions

### Q1 · MCQ · Fundamental
During TCP's slow-start phase (with no losses), cwnd grows

- (A) linearly, by 1 MSS per RTT
- (B) exponentially, roughly doubling every RTT
- (C) exponentially, roughly doubling every round trip only if ssthresh is infinite
- (D) it does not grow; ssthresh grows instead

---

### Q2 · NAT · Fundamental
Starting at cwnd = 1 with ssthresh = 16 and no losses, the value of cwnd at the **end of the 4th RTT** (after 4 doublings) is ______.

---

### Q3 · NAT · Standard
Starting at cwnd = 1 with ssthresh = 16, a **timeout** occurs at the end of the RTT during which cwnd was 16 (i.e., cwnd = 16 was used for one full RTT, then the timeout is detected). Immediately after the timeout is processed, the new value of ssthresh is ______.

---

### Q4 · MSQ · Standard
Which of the following statements about TCP congestion control is/are TRUE?

- (A) Slow start is used both at connection start and after a timeout.
- (B) In congestion avoidance (AIMD), cwnd increases by roughly 1 MSS per RTT and is cut by roughly half on detecting loss via triple duplicate ACKs (fast retransmit/fast recovery), rather than being reset to 1.
- (C) A larger ssthresh always causes TCP to reach a given cwnd value faster.
- (D) TCP Reno's fast recovery skips slow start after 3 duplicate ACKs, going directly into congestion avoidance from around cwnd/2.

---

### Q5 · NAT · Standard
Starting from cwnd = 1 and ssthresh = 16 (no losses), the number of RTTs needed for cwnd to first reach **at least 100** is ______.

---

### Q6 · MCQ · Moderate
A TCP sender's cwnd follows this sequence of RTTs (starting cwnd = 1, ssthresh = 8): 1, 2, 4, 8, 9, 10, 11, then a **timeout** occurs. Immediately after the timeout is handled, (ssthresh, cwnd) equals

- (A) (4, 1)
- (B) (5, 1)
- (C) (8, 1)
- (D) (11, 1)

---

### Q7 · MSQ · Moderate
Which of the following statements about the AIMD (additive-increase, multiplicative-decrease) sawtooth is/are TRUE, assuming cwnd oscillates between W_max (just before a loss) and W_max/2 (just after), increasing by 1 each RTT?

- (A) The long-run average throughput is proportional to W_max.
- (B) The average cwnd over one sawtooth cycle is (3/4)·W_max.
- (C) A connection with a larger RTT achieves a higher throughput than one with a smaller RTT, all else being equal, because it takes longer to reach W_max.
- (D) Halving cwnd on loss (rather than resetting to 1) allows TCP to recover throughput faster than slow start would after a timeout.

---

### Q8 · NAT · Moderate
Starting from cwnd = 1, ssthresh = 32 (no losses), the number of RTTs needed for cwnd to first reach exactly 100 is ______.

---

### Q9 · MSQ · Hard
Which of the following statements about TCP timeout (RTO) computation and behaviour is/are TRUE?

- (A) RTO is typically computed from a smoothed estimate of RTT plus a term proportional to its variation (deviation).
- (B) After each consecutive retransmission timeout for the same segment (Karn's algorithm scenario), the RTO is typically doubled (exponential backoff), until the segment is finally acknowledged.
- (C) A retransmitted segment's RTT sample is used directly to update the smoothed RTT estimate, without any special handling.
- (D) A very low, fixed RTO (much smaller than the actual RTT) would cause unnecessary retransmissions.

---

### Q10 · NAT · Killer
Starting from cwnd = 1 and ssthresh = 32 (no losses), a timeout occurs at the end of the RTT in which cwnd first reaches (or would have reached) exactly 32. The connection recovers (slow start resumes with the new ssthresh, then switches to congestion avoidance as usual) and, with no further losses, continues. The value of cwnd at the end of the **12th RTT overall** (counting from the very first RTT, RTT 1, through the timeout and recovery) is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | B | Slow start doubles cwnd each RTT |
| 2 | 16 | 1, 2, 4, 8, 16 |
| 3 | 8 | max(16/2, 1) |
| 4 | A, B, D | (C) is false — ssthresh only determines when slow start switches to linear growth, not the speed of growth within slow start |
| 5 | 88 | Slow start reaches cwnd = 16 after 4 RTTs (1→2→4→8→16), switching to congestion avoidance; +1 per RTT needs 84 more RTTs to reach 100: 4 + 84 |
| 6 | B | ssthresh = max(11/2,1) = 5, cwnd resets to 1 |
| 7 | A, B | (C) and (D) are false; a larger RTT gives lower throughput, and a timeout resets to 1, which is slower than halving |
| 8 | 73 | Slow start reaches cwnd = 32 after 5 RTTs (1→2→4→8→16→32), switching to congestion avoidance; +1 per RTT needs 68 more RTTs to reach 100: 5 + 68 |
| 9 | A, B, D | (C) is false — Karn's algorithm excludes retransmitted segments' RTT samples from the estimate |
| 10 | 19 | cwnd after each RTT: 1, 2, 4, 8, 16, then RTT 5 would double to 32 — the timeout fires there instead, resetting to ssthresh = 16, cwnd = 1; RTTs 6–9 then double back to 16 (switching to CA); RTTs 10, 11, 12 add 1 each, giving 17, 18, 19 |
