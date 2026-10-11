# Digital Logic → Sequential Circuits I: Flip-Flops, Latches & Counters

**GATE 2027 syllabus:** Digital Logic — design of sequential circuits.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** JK flip-flop: Q(next) = Q (hold) if J=K=0; 0 if J=0,K=1; 1 if J=1,K=0; toggles (complements Q) if J=K=1. T flip-flop: holds if T=0, toggles if T=1. D flip-flop: Q(next) = D. A ripple (asynchronous) counter chains flip-flop outputs as the clock of the next stage, so each stage divides frequency by 2 and delays accumulate stage-by-stage. A MOD-N counter needs ⌈log₂N⌉ flip-flops at minimum.

---

## Questions

### Q1 · MCQ · Fundamental
A JK flip-flop has J=1, K=1. On the next active clock edge, Q

- (A) stays the same (holds)
- (B) becomes 0 regardless of its current value
- (C) becomes 1 regardless of its current value
- (D) toggles (complements its current value)

---

### Q2 · NAT · Fundamental
The minimum number of flip-flops needed to build a MOD-12 counter is ______.

---

### Q3 · MSQ · Standard
Which of the following statements about the **JK flip-flop excitation table** (the required J, K inputs for a given Q → Q(next) transition) is/are TRUE?

- (A) For the transition Q=0 → Q(next)=0, the required inputs are J=0, K=X (don't care).
- (B) For the transition Q=0 → Q(next)=1, the required inputs are J=1, K=X (don't care).
- (C) For the transition Q=1 → Q(next)=0, the required inputs are J=X (don't care), K=1.
- (D) For the transition Q=1 → Q(next)=1, the required inputs are J=1, K=1 only (no don't cares).

---

### Q4 · NAT · Standard
A 16 MHz clock signal is fed through a 3-stage ripple counter built from T flip-flops (each stage toggling on the falling/rising edge of the previous stage's output). The output frequency after all 3 stages, in MHz, is ______.

---

### Q5 · MCQ · Standard
The fundamental difference between a **latch** and a (edge-triggered) **flip-flop** is that

- (A) a latch is level-sensitive (transparent while the enable/clock is active), while a flip-flop changes state only at a clock edge
- (B) a latch can only store 0, while a flip-flop can store both 0 and 1
- (C) a flip-flop requires no clock signal at all
- (D) there is no real difference; the terms are fully interchangeable in all contexts

---

### Q6 · NAT · Moderate
A MOD-5 counter (states 0, 1, 2, 3, 4, then wrapping to 0) starts at state 0. After 12 clock pulses, the counter is in state ______.

---

### Q7 · MSQ · Moderate
Which of the following statements about **ripple (asynchronous) counters** vs. **synchronous counters** is/are TRUE?

- (A) In a ripple counter, each flip-flop is clocked by the output of the previous stage, not by a common clock.
- (B) Ripple counters can suffer from glitches ("counting spikes") during transitions, since not all bits change simultaneously.
- (C) Synchronous counters clock all flip-flops simultaneously from a common clock, avoiding ripple-through delay, at the cost of more complex next-state logic.
- (D) The worst-case propagation delay of an n-stage ripple counter grows linearly with n, while a synchronous counter's delay is roughly independent of n (just one flip-flop's delay plus combinational logic delay).

---

### Q8 · NAT · Moderate
A master-slave JK flip-flop has J=1, K=0 held constant, with the flip-flop initially at Q=0. After 3 complete clock cycles (each cycle being one full clock pulse, master then slave), the value of Q is ______  (0 or 1).

---

### Q9 · MSQ · Hard
Which of the following statements is/are TRUE?

- (A) A D flip-flop can be constructed from a JK flip-flop by tying J = D and K = D̄ (the complement of D).
- (B) A T flip-flop can be constructed from a JK flip-flop by tying J = K = T.
- (C) An SR latch built from two cross-coupled NOR gates has an invalid (forbidden) state when both S and R are asserted simultaneously (both 1, for active-high inputs).
- (D) A T flip-flop cannot be constructed from a D flip-flop; only the reverse construction (D from T) is possible.

---

### Q10 · NAT · Killer
A Moore-machine sequence detector for the (possibly overlapping) pattern "101" has 4 states: S0 (initial/no progress), S1 (seen "1"), S2 (seen "10"), S3 (seen "101", output=1, the only accepting/output state). Transitions: S0 on 0→S0, on 1→S1; S1 on 0→S2, on 1→S1; S2 on 0→S0, on 1→S3; S3 on 0→S2, on 1→S1. Starting at S0, the input sequence `101101101` is applied one bit at a time. The total number of times the output is 1 (i.e., the detector is in state S3) over the 9 bit-steps is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | D | J=K=1 is the toggle condition for a JK flip-flop |
| 2 | 4 | ⌈log₂ 12⌉ = 4 (a 3-bit counter only reaches 8 states, too few) |
| 3 | A, B, C | (D) is false — Q=1→1 is achieved by J=X, K=0 (don't care on J) |
| 4 | 2 | 16 MHz / 2³ = 2 MHz |
| 5 | A | The defining distinction between level-sensitive latches and edge-triggered flip-flops |
| 6 | 2 | 12 mod 5 = 2 (verified by direct simulation: 0,1,2,3,4,0,1,2,3,4,0,1,2 after 12 pulses) |
| 7 | A, B, C, D | All four are standard, correct facts about ripple vs. synchronous counters |
| 8 | 1 | J=1,K=0 forces Q→1 on the first clock edge; it then holds at 1 for subsequent cycles |
| 9 | A, B, C | (D) is false — a T flip-flop can be built from a D flip-flop by feeding back D = T⊕Q |
| 10 | 3 | Verified by simulating the FSM on `101101101`: outputs are 0,0,1,0,0,1,0,0,1 — three 1s |
