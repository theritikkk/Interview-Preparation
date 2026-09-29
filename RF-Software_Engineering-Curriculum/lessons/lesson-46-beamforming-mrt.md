[← Table of Contents](../README.md) · [← Lesson 45](lesson-45-ldpc-codes.md)

---

# Lesson 46 — Beamforming: Transmit Precoding, and the Fourth Appearance of the Same Proof

*Lesson 14 covered MIMO diversity and spatial multiplexing but deliberately left out a third real use of multiple antennas. This lesson closes that gap — and the optimal solution turns out to be the exact same Cauchy-Schwarz proof that's now appeared four times in this curriculum, in four completely different physical settings.*

### 1. Intuition

Lesson 14 covered combining multiple *receive* antennas (diversity) and sending different streams from multiple *transmit* antennas (multiplexing). A third real use of multiple transmit antennas: given known channel conditions, weight each antenna's copy of the *same* symbol so the signals combine **constructively** at the receiver — beamforming. Given known channel gains, what's the optimal weight per antenna? It's the identical optimization problem you've now solved three times before — Lesson 6's matched filter, Lesson 14's MRC diversity, Lesson 20's HARQ chase combining — applied a fourth time, at the transmitter instead of the receiver.

### 2. The engineering problem that forces this

Given known channel gains $h_1,\dots,h_{N_t}$ from $N_t$ transmit antennas to one receive antenna, and a fixed total transmit power budget, what complex weight $w_i$ should each antenna apply to the same symbol $s$ to maximize received SNR?

### 3. Mathematical foundation

Transmit model: antenna $i$ sends $w_i s$. Received signal: $y = \left(\sum_i h_i w_i\right)s + n$. Power constraint: $\sum_i |w_i|^2 = 1$.

### 4. Formal definition

The **beamforming (precoding) vector** $\mathbf{w}$ determines the *effective* channel gain $\sum_i h_i w_i$ seen by the receiver. **Maximum Ratio Transmission (MRT)** is the specific choice of $\mathbf{w}$ that maximizes received SNR under the power constraint — derived, not assumed, in §5.

### 5. Derivation — the fourth appearance of the same proof

Maximize $\left|\sum_i h_i w_i\right|^2$ subject to $\sum_i|w_i|^2=1$. This is **exactly** the Cauchy–Schwarz setup from Lesson 6 §5 and Lesson 14 §5(b), with $h_i$ (channel) and $w_i$ (weights we choose) playing the identical roles as before:
$$\left|\sum_i h_i w_i\right|^2 \le \left(\sum_i|h_i|^2\right)\left(\sum_i|w_i|^2\right) = \sum_i|h_i|^2$$
with equality iff $w_i = c\cdot h_i^*$ for some constant $c$ — the identical equality condition as Lesson 6 and Lesson 14, relabeled. Normalizing for the power constraint:
$$w_i = \frac{h_i^*}{\sqrt{\sum_j|h_j|^2}}$$
This is **MRT**: weight each transmit antenna by the conjugate of its own channel, normalized. The achieved SNR is $\left(\sum_i|h_i|^2\right)\cdot\frac{E_s}{\sigma^2}$ — branch gains simply **add**, exactly Lesson 14 §5(b)'s result, now on the transmit side of the link. Four settings — correlating in time (Lesson 6), combining across receive antennas (Lesson 14), combining across retransmission attempts (Lesson 20), and now weighting transmit antennas (this lesson) — are all, underneath, the identical optimization with the identical proof. Recognizing that pattern, rather than re-deriving it as if new each time, is the actual transferable skill.

### 6. Visualization

```
Multiple TX antennas, weighted to combine constructively at one RX point:

TX1 ──[w1=h1*]──╲
TX2 ──[w2=h2*]───╲___
TX3 ──[w3=h3*]───╱    ╲──► RX: signals ADD constructively
TX4 ──[w4=h4*]──╱         (each weight cancels its own channel's phase)
```

### 7. Worked example

Two TX antennas, channel $h=[0.6+0.8j,\ 0.3-0.4j]$ ($|h_1|=1$, $|h_2|=0.5$). 
$\sum|h_i|^2 = 1+0.25=1.25$. MRT weights: $w_1 = (0.6-0.8j)/\sqrt{1.25}$, $w_2=(0.3+0.4j)/\sqrt{1.25}$. Achieved effective channel gain: $\sqrt{1.25}\approx1.118$, versus a naive equal-weight scheme that generally achieves less, since it doesn't correct for each antenna's individual phase.

### 8. Code

```python
import numpy as np

def mrt_weights(h):
    return np.conj(h) / np.sqrt(np.sum(np.abs(h)**2))

def effective_gain(h, w):
    return np.abs(np.sum(h * w))

h = np.array([0.6+0.8j, 0.3-0.4j])
w_mrt = mrt_weights(h)
w_equal = np.ones(len(h)) / np.sqrt(len(h))     # naive equal-weight, no channel matching

print("MRT effective gain:  ", effective_gain(h, w_mrt))
print("Equal-weight gain:   ", effective_gain(h, w_equal))
print("sqrt(sum|h_i|^2), theoretical MRT max:", np.sqrt(np.sum(np.abs(h)**2)))
```

### 9. Experiment

Run §8's code across many random channel realizations and compare MRT's achieved gain against the equal-weight scheme's. Confirm MRT is **never** worse and generally meaningfully better — directly the Cauchy-Schwarz inequality's guaranteed direction, not a coincidence of the specific example in §7.

### 10. Expected output

MRT's effective gain matches $\sqrt{\sum|h_i|^2}$ exactly on every trial; the equal-weight scheme's gain is always less than or equal to this, confirming the inequality's direction empirically across random channels, not just the one worked example.

### 11. Debugging exercise

Symptom, no cause given: your beamforming system performs exactly as predicted in simulation with perfectly known channel state, but in a more realistic test using a channel estimate measured a few milliseconds before the actual transmission, the achieved gain is much smaller than expected — occasionally even *worse* than no beamforming at all.

Using §5's derivation and Lesson 29 §5's coherence-time reasoning: what would make MRT's guarantee stop holding?

**Root cause (revealed):** MRT's optimality proof in §5 relies **entirely** on $\mathbf{w}$ being matched to the *actual current* channel $\mathbf{h}$. If the true channel has changed — due to motion, exactly Lesson 10/29's Doppler-driven coherence-time argument — since the CSI was measured, the weights are matched to a *stale* channel, and applying them to the *new* channel can align poorly, or even destructively, with no guarantee of the Cauchy-Schwarz bound anymore. This is a direct, transmit-side reapplication of Lesson 29 §5's exact coherence-time requirement: beamforming CSI must be refreshed faster than the channel's coherence time, or the theoretical gain simply doesn't hold in practice, regardless of how correct the MRT derivation is in principle.

### 12. Common mistakes

- Forgetting the conjugate in MRT weights ($w_i=h_i$ instead of $h_i^*$) — the exact same mistake flagged for matched filtering back in Lesson 6 §12, now in a new context, producing a wrong (not merely suboptimal) result.
- Assuming beamforming and spatial multiplexing (Lesson 14) can both run at full strength on the same antenna resources simultaneously — a real trade-off, another instance of Lesson 14 §12's diversity-multiplexing tension, now with beamforming as a third competing use of the same antennas.
- Ignoring CSI staleness relative to coherence time — §11's exact failure mode, easy to miss if you only ever test with perfect, instantaneous channel knowledge.

### 13. Real RF/cellular application

Beamforming — especially massive MIMO with many base-station antennas — is a defining feature of real 5G NR performance gains, used for both coverage extension (particularly critical at FR2/mmWave, directly connecting to Lesson 28's SSB beam-sweeping) and capacity. Real NR CSI feedback and reporting mechanisms exist specifically to keep the base station's channel knowledge fresh enough for beamforming to actually deliver its theoretical gain — the deployed, real-world version of exactly this lesson's debugging exercise.

### 14. Interview question

*"Derive the optimal transmit beamforming weights for a single-receive-antenna system with known channel state, and explain what determines the achieved gain."* A strong answer explicitly recognizes and states the connection: this is the identical Cauchy-Schwarz-optimal-combining structure as the matched filter (Lesson 6) and MRC diversity combining (Lesson 14), not a new problem requiring a fresh derivation — naming that connection is itself part of a strong answer.

### 15. Active recall

1. Write the MRT weight formula from memory, and state the equality condition it comes from.
2. Why does MRT's guarantee break down under stale CSI, specifically?
3. Name the three other places in this curriculum where the identical Cauchy-Schwarz optimization has appeared.

### 16. Mini assignment

Extend §8's code to a multi-receive-antenna case ($N_t\times N_r$, not just $N_t\times1$) — note, without deriving it fully, that joint optimal transmit/receive beamforming in that general case connects to the SVD of the full channel matrix, a natural pointer for further self-study beyond this lesson's single-receive-antenna derivation.

### 17. Exit test

1. Derive the MRT weight vector from the Cauchy-Schwarz inequality, from memory.
2. True or false, with a one-line reason: "Beamforming gain is guaranteed regardless of how old the channel state information used to compute the weights is."
3. Your reasoning from §11 connecting CSI staleness to coherence time, if you haven't worked through it yet.

---

**Next:** Lesson 47 continues directly: **NR beam management** — the P1/P2/P3 procedures that progressively narrow from Lesson 28's coarse SSB-based beam discovery to the fine, high-gain beam pairs this lesson's MRT derivation assumes, and how a connection tracks that pairing as conditions change.

---

[← Lesson 45](lesson-45-ldpc-codes.md) · [Table of Contents](../README.md) · [Lesson 47 →](lesson-47-beam-management.md)