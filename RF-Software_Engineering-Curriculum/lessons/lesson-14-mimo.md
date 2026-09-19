[← Table of Contents](../README.md) · [← Lesson 13](lesson-13-timing-cfo.md)

---

# Lesson 14 — MIMO: Spatial Multiplexing and Diversity

*Closes Stage 4 — Wireless Communications. Every single-antenna concept from Lessons 10–13 gets a matrix generalization here — and the most elegant part of this lesson is that Lesson 6's matched-filter proof turns out to already contain the answer for antenna diversity, just waiting to be re-read with a different index.*

### 1. Intuition

Every lesson so far assumed one transmit antenna, one receive antenna. Real modern systems use several of each, opening up a genuinely new resource: **space**. There are two very different ways to spend it. **Spatial multiplexing** sends *different* data streams simultaneously from different antennas, over the *same* time/frequency resource — if the receiver can tell the streams apart, you've multiplied your data rate for free. **Diversity** instead sends the *same* data via multiple independent paths, so that if one path happens to be in a deep fade (Lesson 10's occasional near-zero Rayleigh draws), the others probably aren't — trading rate for reliability instead of chasing more of it. These two goals genuinely compete for the same antennas, and this lesson is about both the machinery and the trade-off.

### 2. The engineering problem that forces this

With multiple TX and RX antennas, Lesson 10/11's single complex gain $H[k]$ becomes a full **matrix** — every receive antenna sees a mixture of every transmit antenna's signal. Two concrete engineering questions follow: how do you recover multiple simultaneously-transmitted streams from that mixture (Lesson 11's ZF/MMSE math has to generalize from scalar division to matrix inversion), and — critically — when is that even *possible*? Some antenna configurations and propagation environments make separation easy; others make it nearly impossible, for reasons directly analogous to Lesson 11's "deep fade" problem, now happening to a whole matrix instead of one number.

### 3. Mathematical foundation

**MIMO system model:** $\mathbf{y} = \mathbf{H}\mathbf{x} + \mathbf{n}$, where $\mathbf{H}$ is an $N_r \times N_t$ matrix of complex channel gains, $\mathbf{x}$ is the $N_t\times1$ transmit vector, $\mathbf{y}$ the $N_r\times1$ receive vector.

**ZF-MIMO receiver:** $\hat{\mathbf x} = \mathbf{H}^+\mathbf{y} = (\mathbf H^H\mathbf H)^{-1}\mathbf H^H\mathbf y$ — the pseudo-inverse.

**MMSE-MIMO receiver:** $\hat{\mathbf x} = \left(\mathbf H^H\mathbf H + \tfrac{\sigma_n^2}{\sigma_x^2}\mathbf I\right)^{-1}\mathbf H^H\mathbf y$.

**Maximal Ratio Combining (MRC, receive diversity):** with $N_r$ antennas all carrying the *same* symbol $s$ through independent gains $h_i$, combine as $\hat s = \sum_i w_i^* y_i$ with $w_i = h_i$ (derived in §5(b)) — combined SNR $=\sum_i|h_i|^2\,(E_s/\sigma^2)$.

### 4. Formal definition

**Spatial multiplexing gain** requires $N_r\ge N_t$ and, critically, that $\mathbf H$ is well-conditioned — its columns (each transmit antenna's channel to the whole receive array) must be sufficiently distinguishable. The **condition number** of $\mathbf H$ measures exactly this: well-conditioned means easy separation, near-singular means the streams are nearly indistinguishable — a direct matrix generalization of Lesson 11's "$|H[k]|$ near zero" idea. **Diversity gain** instead comes from having *multiple independent estimates* of the same symbol, requiring only that the branches fade somewhat independently — a much weaker, easier requirement than full matrix rank.

### 5. Derivation

**(a) MIMO ZF is exactly Lesson 11's ZF, generalized.** Minimize $\|\mathbf y - \mathbf H\mathbf x\|^2$ over $\mathbf x$ — the standard least-squares normal equations give $\hat{\mathbf x} = (\mathbf H^H\mathbf H)^{-1}\mathbf H^H\mathbf y$. Check the scalar special case, the same instinct from Lesson 8 §5(c): for $N_t=N_r=1$, $\mathbf H^H\mathbf H = |H|^2$ and $\mathbf H^H=H^*$, giving $\hat x = H^*y/|H|^2 = y/H$ — **exactly** Lesson 11's scalar ZF. The matrix machinery isn't new math; it's the same idea, extended to more dimensions.

**(b) MRC is Lesson 6's matched filter, with antennas playing the role time samples played before.** This is worth deriving in full because it's genuinely the *same proof*, just relabeled. Given $y_i = h_i\,s + n_i$ for $i=1,\dots,N_r$ (independent branches, independent noise variance $\sigma^2$ each), and a linear combiner $\hat s = \sum_i w_i^* y_i$, the output SNR is:
$$\text{SNR} = \frac{\left|\sum_i w_i^* h_i\right|^2 E_s}{\sigma^2\sum_i|w_i|^2}$$
This is **exactly** Lesson 6 §5's SNR expression, with the sum over antenna index $i$ standing in for the earlier sum over time index $k$. Cauchy–Schwarz applies identically:
$$\left|\sum_i w_i^* h_i\right|^2 \le \left(\sum_i|w_i|^2\right)\left(\sum_i|h_i|^2\right)$$
with equality iff $w_i = c\cdot h_i$. Substituting exactly as before, the $\sum|w_i|^2$ terms cancel, giving maximum SNR $= \tfrac{E_s}{\sigma^2}\sum_i|h_i|^2$ — the branch SNRs simply **add**. The optimal weights are $w_i=h_i$: conjugate-weight each branch by its own channel gain, precisely the matched-filter logic of Lesson 6, now operating across space instead of time. And because it's a sum of $N_r$ *independent* Rayleigh-derived (Lesson 10 §5(b)) magnitudes, the probability that *every* branch is simultaneously deep-faded is far smaller than the probability of any single branch being so — that shrinking joint probability, not a new mechanism, *is* diversity gain.

### 6. Visualization

```
Spatial multiplexing (2x2):              MRC receive diversity (1x2):
TX1 ──┬───h11───┬──► RX1                  TX ──┬───h1───► RX1 ─┐
      │╲        │                              │                (weighted
      │ ╲ h21   │                              └───h2───► RX2 ─┘  sum, w=h*)
TX2 ──┴──╲──h22─┴──► RX2                                         → ŝ
   (each RX sees a mixture of both TX)    (both RX see the SAME symbol s)
```

### 7. Worked example

**Multiplexing:** $\mathbf H = \begin{pmatrix}1 & 0.2\\0.3 & 1\end{pmatrix}$ (well-conditioned — off-diagonal terms small relative to diagonal), $\mathbf x=(1,-1)^T$: $\mathbf y = (0.8,\ -0.7)^T$ (noiseless). ZF recovers $\hat{\mathbf x}=\mathbf H^{-1}\mathbf y = (1,-1)^T$ exactly. **Diversity:** two branches with SNRs 3 dB and 10 dB (linear: $2.0$ and $10.0$). MRC combined SNR $= 2.0+10.0=12.0$ linear $\approx10.8$ dB — better than either branch *alone*, including the stronger one; the weaker branch still contributes useful energy rather than being wasted.

### 8. Code

```python
import numpy as np

# --- Spatial multiplexing: 2x2 ZF ---
H = np.array([[1.0, 0.2], [0.3, 1.0]], dtype=complex)
x = np.array([1, -1], dtype=complex)
noise = 0.01*(np.random.randn(2)+1j*np.random.randn(2))
y = H @ x + noise

x_zf = np.linalg.pinv(H) @ y                      # ZF: matrix generalization of Lesson 11
print("ZF recovery error:", np.abs(x_zf - x))

# --- Receive diversity: MRC across 2 branches ---
s = 1+0j
h = np.array([0.5, 1.5], dtype=complex)            # two independent branch gains
n = 0.3*(np.random.randn(2)+1j*np.random.randn(2))
y_branches = h*s + n

s_mrc = np.sum(np.conj(h)*y_branches) / np.sum(np.abs(h)**2)   # §5(b)'s optimal weights
print("MRC estimate:", s_mrc, " true:", s)
```

### 9. Experiment

Generate many random $2\times2$ channel matrices $\mathbf H$ (entries i.i.d. complex Gaussian — physically, independent Rayleigh-fading paths per antenna pair, Lesson 10 §5(b)) and, for each, compute the condition number and the ZF recovery error at fixed noise. Plot error against condition number — expect a sharp blow-up as conditioning worsens, mirroring Lesson 11 §9's per-subcarrier ZF-vs-notch experiment exactly, one dimension up. Separately, simulate BER for 1-antenna Rayleigh fading (Lesson 10's experiment) versus 1×2 MRC diversity at the same average SNR, and confirm MRC is dramatically better — direct empirical proof of §5(b)'s diversity-gain argument.

### 10. Expected output

ZF recovery error stays small for well-conditioned $\mathbf H$ and spikes sharply as condition number grows — the matrix-level version of Lesson 11's deep-fade blow-up. MRC's BER curve sits well below plain single-antenna Rayleigh fading at every SNR, converging toward AWGN-only performance as $N_r$ grows — fading's damage genuinely shrinks as independent branches accumulate.

### 11. Debugging exercise

Symptom, no cause given: your 2×2 spatial-multiplexing ZF receiver performs well across most random channel draws, but for certain specific channel realizations, BER spikes catastrophically — not a code bug, and it happens more often the more trials you run. This should feel familiar.

What's the matrix-level analog of Lesson 11's per-subcarrier ZF failure? What property of $\mathbf H$, specifically, would you check?

**Root cause (revealed):** an **ill-conditioned** (nearly singular) $\mathbf H$ — the two transmit antennas' channels to the receive array happen, for that particular random draw, to be nearly linearly dependent (nearly parallel columns), meaning the receiver has almost no independent information to separate the two streams. $(\mathbf H^H\mathbf H)^{-1}$ then blows up exactly the way $1/H[k]$ did in Lesson 11 — this is literally the same pathology, one dimension up. The fix is the same family of fixes: MMSE-MIMO (regularizing with $\tfrac{\sigma_n^2}{\sigma_x^2}\mathbf I$ before inverting, exactly Lesson 11 §5(b)'s trick) avoids the catastrophic blow-up; real systems also adaptively fall back to fewer simultaneous streams ("lower rank") or switch to transmit diversity when the channel is measured to be poorly conditioned — a genuine, dynamic decision real base stations make.

### 12. Common mistakes

- Assuming more antennas automatically means more multiplexing capacity — it only helps if $\mathbf H$ has good rank/conditioning, which needs genuinely independent propagation paths between antennas (rich multipath, Lesson 10). In near-line-of-sight conditions with closely spaced antennas, $\mathbf H$ can be nearly rank-1 regardless of antenna count — a real, practical limitation especially relevant to mmWave (FR2) deployments, where LOS is common.
- Treating diversity gain and multiplexing gain as both fully available at once from the same antennas — there's a well-known fundamental **diversity-multiplexing trade-off**: pushing harder for one generally costs some of the other at a fixed antenna count.
- Forgetting that MRC's optimality (§5(b)) assumed **independent** noise across branches — correlated noise or interference across closely-spaced antennas breaks the simple weighted-sum optimality and needs a noise-whitening step first.

### 13. Real RF/cellular application

MIMO is central to LTE (2×2, 4×4 downlink) and even more so to 5G NR (8×8 and beyond, massive MIMO with many base-station antennas, beamforming). The **transmission mode** a real system selects — spatial multiplexing for higher peak rate under rich-scattering/good-SNR conditions, versus transmit diversity for robustness at cell edge — is exactly this lesson's trade-off, and it's dynamically configured based on measured channel conditions: the **rank indicator (RI)** and **CQI** feedback in real 3GPP systems are literally reporting the channel-conditioning information this lesson derives.

### 14. Interview question

*"Why does spatial multiplexing gain depend on rich multipath scattering, while diversity or beamforming can still work in near-line-of-sight conditions?"* Reason from §5: multiplexing needs $\mathbf H$ to have good rank/conditioning — genuinely separable streams — which specifically requires multiple independent propagation paths (Lesson 10). Diversity (§5(b)) only needs branches to fade somewhat independently, a far weaker requirement that survives even in near-LOS conditions where a matrix-rank sense of "independence" would fail.

### 15. Active recall

1. In one sentence, why does MIMO ZF reduce exactly to Lesson 11's scalar ZF at $N_t=N_r=1$?
2. What determines whether spatial multiplexing gain is actually achievable, beyond simply having enough antennas?
3. Why does MRC's derivation in §5(b) produce the *identical* mathematical structure as Lesson 6's matched filter derivation?

### 16. Mini assignment

Using §9's setup, generate random $2\times2$ channels with a controllable correlation parameter between the two columns (e.g., blend a common component in with independent noise), sweep that correlation, and plot both condition number and ZF recovery error against it — directly connecting "how correlated are the antenna paths" to "how badly does ZF fail."

### 17. Exit test

1. Write out the MIMO ZF formula and confirm, symbolically, that it reduces to Lesson 11's scalar ZF at $N_t=N_r=1$.
2. True or false, with a one-line reason: "Adding more receive antennas for diversity always helps as much as adding them for multiplexing."
3. Your reasoning from §11 connecting matrix conditioning to Lesson 11's deep-fade problem, if you haven't sent it.

---

**Next:** This closes Stage 4. Lesson 15 opens **Stage 5 — Software-Defined Radio**, moving from pure simulation to real, captured IQ data for the first time — decoding an actual broadcast FM or ADS-B signal, and seeing every concept from Lessons 1–14 show up in data that didn't come from a random-number generator.

---

[← Lesson 13](lesson-13-timing-cfo.md) · [Table of Contents](../README.md) · [Lesson 15 →](lesson-15-real-iq-dc-offset-iq-imbalance.md)