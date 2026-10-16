# CN Notes 02 — TCP, Flow Control & Congestion Control (High-Yield Deep Dive)

> **Why this file exists:** "TCP" and "Flow Control" are both on the high-yield list. Together they cover: ARQ protocols (Stop-and-Wait, GBN, SR) with efficiency/throughput math, sliding windows, sequence-number constraints, TCP's header and sequence arithmetic, connection management, and congestion control (slow start / congestion avoidance). Built using the PW GATE handbook's formulas and tables, extended with derivations and a verified 10-question practice set.
>
> **Companion question banks:** `CN_08_TCP_Flow_Control_Reliable_Delivery.md`, `CN_09_TCP_Congestion_Control.md`. PW source: `../Reference/PW_Subject_Handbooks/PW_CN.txt` (§3, §5).

---

## 0. The problem being solved

```

    Data link: one hop          Transport: end to end
    ─────────────────           ─────────────────────────
    flaky wire, finite speed    want: no loss, no reorder,
                                no flooding the receiver
```

Flow control = don't overwhelm the **receiver**. Error control = recover from loss/reordering. Both are built from one primitive: the **sliding window**.

**Delays you must name and compute:**

| Delay | Formula | Notes |
|---|---|---|
| Transmission | `Td = L / B` (frame length / bandwidth) | time to push bits onto the wire |
| Propagation | `Pd = d / v` | time for the signal to travel |
| Queuing, Processing | `Qd`, `Prd` | often ignored in GATE unless stated |

Round-trip time: `RTT = 2·Pd`. A sender's useful time is `L/B` per frame; everything else is dead time.

---

## 1. Stop-and-Wait — the baseline

Sender transmits one frame; waits for the ACK; repeats.

```
 Sender:  [====frame====]...............[====frame 2====]....
                          ↕ RTT
 Receiver:            [==ACK==]
```

**Efficiency (link utilization):**

```
        Td(frame)                            Td
  η = ─────────────────────────  =  ──────────────────────
       Td + 2Pd + Qd + Prd + Td(ACK)      Td + RTT + Td(ACK)
```

(2Pd = full propagation round trip; ACK transmission time usually folded in or neglected.)

**Throughput** = η × B.

**Worked example:** `Td = 2 ms`, `RTT = 2Pd = 18 ms`, ACK negligible:

```
  η = 2 / (2 + 18) = 10 %
```

Straight-line derivation for exams: `η = 1/(1 + 2a)` where `a = Pd/Td`. Stop-and-Wait wastes the link whenever `2Pd > Td` — the timeout for the frame also serves as the ACK timeout (one timer is enough).

---

## 2. Sliding window — keep the pipe full

Allow up to **W** unacknowledged frames in flight. If `W·Td ≥ Td + RTT`, the link stays busy → η = 100%.

**Minimum window for full utilization:**

```
  W_min = ceil( (Td + 2Pd + Td(ACK)) / Td )
```

### 2.1 Go-Back-N (GBN)

- Sender window = N; **receiver window = 1**.
- Receiver **discards** out-of-order frames (no buffering).
- ACKs are **cumulative**: "ACK k" = everything before k received; k is the next expected frame.
- **One timer**, for the oldest unacknowledged frame. On timeout → retransmit the whole window.
- Efficiency: `η = N·Td / (Td + RTT + Td(ACK))`, capped at 1.
- **Sequence constraint:** window + receiver must be distinguishable:

```
  Ws + Wr ≤ ASN        →   N + 1 ≤ 2^k   →   N ≤ 2^k − 1   (k = sequence bits)
```

### 2.2 Selective Repeat (SR)

- `Ws = Wr = W`.
- Receiver **buffers** out-of-order frames; delivers to the upper layer **in sorted order**.
- ACKs are **independent** (per frame). A bad/new-out-of-order frame can trigger a **NACK** → immediate retransmission of that frame (even if its timer hasn't expired) — hence "searching at sender, sorting at receiver".
- **Timer per frame** in the window.
- Efficiency: `η = W·Td / (Td + RTT + Td(ACK))`.
- **Sequence constraint (the famous 2N rule):**

```
  Ws + Wr ≤ ASN   →   W + W ≤ 2^k   →   W ≤ 2^(k−1)
```

With k bits you can have at most **half** the sequence space per window; otherwise a new window's frames alias old ACKs.

### 2.3 Comparison table (memorize the shape)

| | Stop-and-Wait | GBN | SR |
|---|---|---|---|
| Sender window | 1 | N | W |
| Receiver window/buffer | 1 | 1 | W |
| Total buffering | 1 + 1 | N + 1 | N + N |
| Min distinct seq. numbers | 2 | N + 1 | 2N |
| ACK style | per frame | cumulative | independent (+NACK) |
| Timers | 1 | 1 (oldest frame) | per frame |
| Retransmission on loss | the frame | whole window from lost frame | just the lost frame |
| Out-of-order delivery to app | n/a | impossible by design | in order (after sort) |

**Sanity checks:** GBN needs at least N+1 sequence numbers because the receiver's current expectation must be distinguishable from a full window of old frames. SR needs 2N so sender and receiver windows never share ambiguity.

---

## 3. TCP — the connection-oriented workhorse

TCP gives reliable, ordered, byte-stream, full-duplex, point-to-point delivery. Connection uniquely identified by **(source IP, source port, dest IP, dest port)** — the 4-tuple.

### 3.1 Header (20 B base, up to 60 B with options)

```
 16-bit source port | 16-bit dest port
 32-bit sequence number
 32-bit acknowledgement number
 4-bit HLEN | 6 reserved | URG ACK PSH RST SYN FIN | 16-bit window
 16-bit checksum | 16-bit urgent pointer | options (0–40 B)
```

Port ranges: 0–1023 well-known, 1024–49151 registered, 49152–65535 dynamic.

### 3.2 Sequence-number arithmetic (PW's rules — GATE gold)

- Every **data byte consumes 1** sequence number.
- **SYN consumes 1**, **FIN consumes 1**, **ACK consumes 0**.
- ACK number = **next expected byte** (cumulative).
- Flags map: SYN=1,ACK=0 request; SYN=1,ACK=1 reply; SYN=0,ACK=1 ack; 0,0 = data.

**Wrap-around time (WAT)** — time until the 32-bit sequence space cycles:

```
  WAT = total sequence numbers / rate = 2^32 / (bandwidth in bytes/sec)
```

To stay safe, the sequence space must exceed `B × LT` (bandwidth × lifetime of a segment):

```
  min bits = ceil( log2( B_bytes/sec × LT ) )
```

### 3.3 Connection management

- **Three-way handshake:** `SYN(seq=x)` → `SYN+ACK(seq=y, ack=x+1)` → `ACK(ack=y+1)`. Note each SYN eats one sequence number.
- **Termination:** FIN consumes one sequence number too; each side closes independently (FIN, ACK, FIN, ACK).

### 3.4 Retransmission timeout (RTO)

- Basic: `RTO = 2 × RTT` (PW's simple model).
- **Jacobson's algorithm** (adaptive):

```
  NRTT (next RTT)  = α · IRTT + (1 − α) · ARTT      (α typically 0.9)
  AD (deviation)   = |IRTT − ARTT|
  ND (next dev.)   = α · ID + (1 − α) · AD
  RTO = 4 · ND + RTT
```

(IRTT = the round-trip time just measured; ARTT = the running average.)

---

## 4. TCP flow control vs congestion control

**Flow control — protect the receiver:**

- Receiver advertises its free buffer space in the **window** field of every ACK/segment → `rwnd`.
- Sender keeps `inflight ≤ rwnd`. Window = 0 → sender pauses and probes (**persist timer**) until a non-zero window arrives.
- (Silly Window Syndrome: tiny window advertisements / tiny segments → Nagle's algorithm + delayed ACKs mitigate.)

**Congestion control — protect the network** (AIMD family):

```
  Effective sender window  Ws = min(cwnd, rwnd)
```

| Phase | Trigger | Window growth |
|---|---|---|
| **Slow start** | connection begins / after timeout | `cwnd += 1 MSS` per ACK ⇒ **doubles every RTT** |
| **Congestion avoidance** | `cwnd ≥ ssthresh` | `cwnd += 1/cwnd` per ACK ⇒ **+1 MSS per RTT** |
| **Detection: timeout** | no ACK | `ssthresh = cwnd/2`, `cwnd = 1`, restart slow start |
| **Detection: 3 duplicate ACKs** | fast retransmit | `ssthresh = cwnd/2`, fast recovery (Reno: `cwnd = ssthresh`, continue in CA) |

**cwnd evolution example** (ssthresh = 8, starting cwnd = 1):

| After RTT | 1 | 2 | 3 | 4 | 5 | 6 |
|---|---|---|---|---|---|---|
| cwnd (MSS) | 2 | 4 | 8 | 9 | 10 | 11 |
| phase | SS | SS | SS→CA | CA | CA | CA |

(SS = doubling: 1→2→4→8; at 8 = ssthresh, switch to +1 per RTT.)

**Reading the TCP timeline:** initial surge (slow start), linear climb (avoidance), a cliff (timeout → back to 1) or a partial drop (3-dup-ACK → half, no cliff to 1 in Reno).

---

## 5. UDP — the minimal transport

| | TCP | UDP |
|---|---|---|
| Connection | oriented | connectionless |
| Reliability/ordering | yes | none |
| Header | 20–60 B | **8 B fixed** |
| Flow/congestion/error control | yes | none (checksum optional) |
| Use cases | HTTP, FTP, SMTP, POP, IMAP | DNS-ish, SNMP, TFTP, DHCP, RIP, real-time media (RTP) |

Why UDP exists (PW list): one-request-one-reply apps, constant dataflow, multimedia, broadcast/multicast, management protocols (SNMP), route updates (RIP), apps with their own flow/error control (TFTP).

**Quick recall table — application protocols:**

| Protocol | Port | Transport | Style |
|---|---|---|---|
| DNS | 53 | UDP | query/reply |
| HTTP | 80 | TCP | request/response (stateless) |
| SMTP | 25 | TCP | push (send mail) |
| POP3 / IMAP | 110 / 143 | TCP | pull (retrieve mail) |
| FTP | 20 data / 21 control | TCP | out-of-band (control,data separate) |
| Telnet | 23 | TCP | remote login |
| SNMP | 161/162 | UDP | management |
| TFTP | 69 | UDP | trivial transfer |
| DHCP | 67 server / 68 client | UDP | — |

---

## 6. Traps & exam patterns

1. **Cumulative vs independent ACK**: GBN's ACK number is *next expected*; a duplicate ACK doesn't move the window in GBN.
2. **Sequence-bit questions**: identify the protocol first; GBN → `2^k − 1 ≥ W`; SR → `2^(k−1) ≥ W`. Never mix formulas.
3. **Efficiency**: numerator counts only *useful transmission time* (N·Td or W·Td), denominator is one full cycle `Td + 2Pd + Td(ACK)`. Cap η at 1 (utilization can't exceed 100%).
4. **SYN/FIN eat sequence numbers** (1 each); ACK eats none. Handshake/termination sequence-number puzzles hinge on this.
5. **WAT units**: bandwidth in **bytes/sec** — convert from bps by dividing by 8 *before* dividing.
6. **cwnd phases**: draw the sawtooth; "3 duplicate ACKs" is NOT a timeout — cwnd doesn't fall to 1 in Reno.
7. TCP window = min(cwnd, rwnd): a big cwnd with rwnd=0 still means a stalled sender.

---

## 7. Practice set (verified by simulation)

Use these link parameters where referenced: **Td(frame) = 2 ms, RTT (2·Pd) = 18 ms, Td(ACK) and all other delays ≈ 0.**

### Q1 · NAT · Fundamental
For Stop-and-Wait on the link above, the efficiency (link utilization) in percent is ______.

---

### Q2 · NAT · Fundamental
Same link, Go-Back-N with sender window N = 7. The efficiency in percent is ______.

---

### Q3 · NAT · Standard
Same link. The minimum sender window (in whole frames) for **100% utilization** is ______.

---

### Q4 · MCQ · Standard
Which of the following is TRUE of **Go-Back-N**?

- (A) The receiver buffers out-of-order frames for later delivery.
- (B) ACKs are cumulative; the receiver window is 1; the sender keeps one timer for the oldest outstanding frame.
- (C) Every frame in the window has its own timer.
- (D) On timeout, only the lost frame is retransmitted.

---

### Q5 · NAT · Standard
In **Selective Repeat**, the sequence-number field is 3 bits. The maximum window size (in frames) is ______.

---

### Q6 · MSQ · Moderate
Which of the following statements about **Selective Repeat** is/are TRUE?

- (A) Sender and receiver windows may be equal.
- (B) The receiver buffers out-of-order frames and delivers to the application in sorted order.
- (C) A NACK can cause immediate retransmission of a specific frame before its timer expires.
- (D) The receiver window must always be 1.

---

### Q7 · NAT · Moderate
A TCP connection uses the full 32-bit sequence space over a 1 Gbps link. The **wrap-around time**, in seconds (nearest integer), is ______.

---

### Q8 · NAT · Moderate
The same 1 Gbps link. To prevent sequence-number wrap-around for a segment lifetime of 60 s, the minimum number of sequence-number bits required is ______.

---

### Q9 · NAT · Hard
GBN must keep a link of `Td = 1 ms` and `RTT = 24 ms` fully utilized. The minimum number of **sequence-number bits** needed for GBN is ______.

---

### Q10 · NAT · Killer
A TCP sender has `cwnd = 16 MSS` when its retransmission timer expires. After the timeout, the new `ssthresh` is ______ MSS.

---

## Answer Key

| Q | Answer | Why (condensed) |
|---|---|---|
| 1 | 10 | 2/(2+18) |
| 2 | 70 | 7×2/20 |
| 3 | 10 | (2+18)/2 |
| 4 | B | canonical GBN behavior |
| 5 | 4 | W ≤ 2^(k−1) = 2² |
| 6 | A, B, C | (D) false — Wr = Ws in SR |
| 7 | 34 | 2³² B ÷ 125×10⁶ B/s = 34.36 s |
| 8 | 33 | need ≥ 125e6 × 60 = 7.5e9; 2³² < 7.5e9 ≤ 2³³ |
| 9 | 5 | W ≥ 25; GBN needs 2^k − 1 ≥ 25 → k = 5 |
| 10 | 8 | ssthresh = cwnd/2 = 16/2 |

### Full solutions

**Q1.** `η = Td / (Td + RTT) = 2/20 = 10%`. (General shortcut: `η = 1/(1+2a)`, `a = Pd/Td = 9/2` here.)

**Q2.** `η = N·Td / (Td + RTT) = 14/20 = 70%`.

**Q3.** Full utilization when `W·Td ≥ Td + RTT` → `W ≥ 20/2 = 10`. So W = **10** (and efficiency saturates at 100% for any W ≥ 10). *Bonus sanity:* with W = 10, GBN needs `2^k − 1 ≥ 10` → k = 4 sequence bits.

**Q4.** B. (A) is SR. (C)(D) describe the per-frame-timer / retransmit-only-lost behavior of SR — GBN re-sends the whole window from the lost frame.

**Q5.** SR constraint: `Ws + Wr ≤ 2^k` with `Ws = Wr = W` → `2W ≤ 8` → **W = 4**. (Half the sequence space, always.)

**Q6.** A ✔ (by design Ws = Wr). B ✔ (buffering + sorting is SR's signature). C ✔ (NACK → immediate resend; PW's "searching at sender" line). D ✘ — that's GBN.

**Q7.** Rate = 1 Gbps = **125 × 10⁶ bytes/sec**. `WAT = 2³² / 125×10⁶ = 4,294,967,296 / 125,000,000 = 34.36 s` → nearest integer **34 s**.

**Q8.** Safety condition: sequence space ≥ B × LT = 125×10⁶ × 60 = **7.5 × 10⁹**. Powers: 2³² = 4.29e9 (short), 2³³ = 8.59e9 (enough). Minimum bits = **33**. (Yes — it's one more than 32, the classic gotcha.)

**Q9.** Utilisation: `W ≥ (1+24)/1 = 25`. GBN window limit: `W ≤ 2^k − 1`. Need `2^k − 1 ≥ 25` → `2^k ≥ 26` → **k = 5** (2⁵ − 1 = 31 ≥ 25; 2⁴ − 1 = 15 < 25).

**Q10.** Timeout rule: `ssthresh = cwnd/2 = 16/2 = 8 MSS`; `cwnd` resets to 1 MSS. (If it had been 3 duplicate ACKs instead, ssthresh would still be 8 but cwnd would not drop to 1 in Reno — fast recovery.)

---

## 8. Next steps

1. Re-derive Q3, Q5, Q9 formulas without notes — these three shapes cover most GATE ARQ questions.
2. Practice reps: `CN_08` (flow control/reliability) and `CN_09` (congestion control, cwnd timeline reading).
3. CN queue complete → move to **COA**: `../COA/Notes/COA_Notes_01_Instruction_Pipelining.md`.
