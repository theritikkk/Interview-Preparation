[← Table of Contents](../README.md) · [← Lesson 47](lesson-47-beam-management.md)

---

# Lesson 48 — Massive MIMO CSI Feedback Compression: Codebooks and the Cost of Finite Feedback

*Lesson 46 assumed the transmitter simply knows the channel. In FDD systems, it doesn't — and can't derive it from uplink measurements the way TDD can. This lesson derives exactly how much beamforming gain is lost when the UE has to compress and feed back the channel instead of the base station measuring it directly.*

### 1. Intuition

Lesson 46's MRT derivation assumed the transmitter *knows* $\mathbf{h}$ exactly. In FDD systems — uplink and downlink on different frequencies, Lesson 18 — the base station cannot measure the downlink channel from uplink signals, because different frequencies experience genuinely different fading (Lesson 10's frequency-selectivity, now working against you instead of just being a phenomenon to equalize). The UE must measure the downlink channel itself and report it back — but with massive MIMO's dozens or hundreds of antennas, feeding back the full channel vector would consume enormous uplink overhead. NR's solution: don't feed back the channel — feed back an *index* into a shared, pre-agreed codebook of candidate beam directions that best approximates it. This is vector quantization, and it has a real, derivable cost.

### 2. The engineering problem that forces this

Given a channel vector $\mathbf{h}$ and a shared codebook $\{\mathbf{c}_1,\dots,\mathbf{c}_M\}$ known to both UE and base station, how does the UE select the entry that best approximates Lesson 46's true MRT-optimal direction, using only $\log_2 M$ bits of feedback — and precisely how much beamforming gain is lost compared to perfect, full-precision feedback?

### 3. Mathematical foundation

The UE selects $\mathbf{c}_i$ maximizing $|\mathbf{h}^H\mathbf{c}_i|$ — exactly Lesson 46 §5's gain expression, now evaluated over a *constrained*, finite set of candidate directions instead of the continuous, freely-optimal one.

### 4. Formal definition

NR's **Type I codebook** structure is, at a high level, a discretized, oversampled grid of DFT-like steering directions — literally built from the same complex-exponential basis vectors that make up the DFT matrix (Lesson 3), sampled more finely than the bare $N$-point DFT to give a denser angular grid. **PMI** (Precoding Matrix Indicator) is the reported codebook index. **RI** (Rank Indicator) is the UE's separate report of how many spatial streams the channel currently supports — directly Lesson 14's matrix-conditioning reasoning, reported back to the network rather than computed at the receiver alone.

### 5. Derivation — quantifying the quantization loss, directly extending Lesson 46

Let $\mathbf{w}_{codebook}$ be the selected codebook entry, at angle $\theta_{err}$ from the true optimal direction $\mathbf{h}^*/|\mathbf{h}|$ (both unit vectors). The achieved gain is the projection of one onto the other, scaled by the angle between them — the standard geometric meaning of a complex inner product, the same geometry underlying Lesson 46 §5's Cauchy-Schwarz equality condition:
$$|\mathbf{h}^H \mathbf{w}_{codebook}| = |\mathbf{h}|\cos\theta_{err}$$
So achieved gain relative to the perfect-CSI optimum is $\cos\theta_{err}$, and since SNR scales with gain *squared*:
$$\frac{\text{achieved SNR}}{\text{optimal SNR}} = \cos^2\theta_{err}$$
A clean, derived relationship — not cited — directly extending Lesson 46's framework to the finite-feedback case. As codebook size $M$ grows, the codebook covers the space of directions more densely, shrinking the typical $\theta_{err}$ for any given channel — with diminishing returns, since covering a high-dimensional space of directions ever more finely requires disproportionately more entries as the target angular resolution shrinks (the general shape of any high-dimensional quantization problem, not a number to take as an exact universal formula).

### 6. Visualization

```
Codebook: a discrete set of candidate directions covering the full space
        ×     ×
    ×      ●←true optimal direction        ×  = codebook entries
        ×  ↑θ_err  ×                        ● = actual channel's optimal MRT direction
              ×                             nearest × is what gets reported (PMI)

SNR loss vs quantization angle:
loss(dB) = -10·log10(cos²θ_err)  — grows slowly at first, then more steeply
```

### 7. Worked example

$\theta_{err}=10°$: gain loss $\cos(10°)\approx0.985$, SNR loss $\approx0.970\approx-0.13$ dB — negligible. $\theta_{err}=30°$: gain loss $\cos(30°)\approx0.866$, SNR loss $\approx0.75\approx-1.25$ dB — real and noticeable. Small angle errors cost very little; the loss grows meaningfully once quantization error becomes large, giving a concrete sense of how fine a codebook actually needs to be.

### 8. Code

```python
import numpy as np

def dft_codebook(n_ant, oversample=4):
    n_entries = n_ant * oversample
    codebook = []
    for k in range(n_entries):
        angle = 2*np.pi*k/n_entries
        c = np.exp(1j*angle*np.arange(n_ant)) / np.sqrt(n_ant)
        codebook.append(c)
    return np.array(codebook)

def select_pmi(h, codebook):
    gains = np.abs(codebook @ np.conj(h))
    best = np.argmax(gains)
    return best, gains[best]

n_ant = 8
h = (np.random.randn(n_ant) + 1j*np.random.randn(n_ant))
h /= np.linalg.norm(h)
perfect_gain = np.linalg.norm(h)                 # Lesson 46's unquantized MRT gain

for oversample in [1, 2, 4, 8]:
    cb = dft_codebook(n_ant, oversample)
    pmi, achieved_gain = select_pmi(h, cb)
    snr_loss_db = 10*np.log10((achieved_gain/perfect_gain)**2)
    print(f"codebook size={len(cb):3d}  SNR loss: {snr_loss_db:.2f} dB")
```

### 9. Experiment

Run §8's sweep across growing codebook sizes (more oversampling = more feedback bits) and confirm SNR loss shrinks with diminishing returns — each additional feedback bit buying progressively less improvement, a real, quantifiable consideration for choosing how many PMI bits a real system should actually spend.

### 10. Expected output

SNR loss shrinking from a fraction of a dB at small codebooks toward negligible at large ones, with the *rate* of improvement clearly slowing — directly demonstrating the diminishing-returns shape rather than a straight-line relationship between feedback bits and gain.

### 11. Debugging exercise

Symptom, no cause given: your simulated massive-MIMO system's measured throughput sits noticeably below the perfect-CSI (Lesson 46, unquantized MRT) theoretical prediction — even though your PMI-selection logic correctly picks the best available codebook entry every single time, verified independently.

Using §5: is this necessarily a bug?

**Root cause (usually not a bug at all):** this is exactly the **expected quantization loss** this lesson derives — $\cos^2\theta_{err} < 1$ whenever the codebook is finite, which it always is in a real system. The fix isn't code; it's recognizing some gap between codebook-based performance and perfect-CSI performance is fundamentally unavoidable given finite feedback. The real engineering question is whether the *observed* gap matches what §5's formula predicts for your specific codebook size — not whether the gap is zero, which no finite-feedback scheme can achieve.

### 12. Common mistakes

- Expecting codebook-based beamforming to match perfect Lesson 46 MRT exactly — a basic conceptual error; some loss is fundamentally unavoidable with finite feedback, not an implementation imperfection to be debugged away.
- Confusing **PMI** (which direction to beam) with **RI** (how many streams/rank to use) — two genuinely separate reported quantities serving different purposes.
- Assuming TDD systems need this feedback machinery too — TDD can exploit channel **reciprocity** (an uplink pilot measurement directly gives the downlink channel, since both use the same frequency), avoiding much of this overhead entirely — a genuine, real FDD-versus-TDD architectural difference, not a minor detail.

### 13. Real RF/cellular application

CSI feedback design — codebook structure, PMI/RI/CQI reporting overhead versus accuracy — is a genuine, active area of real 5G NR system design, and understanding precisely why a correctly-selected PMI still doesn't guarantee perfect beamforming gain is directly relevant to the "Modem Systems/Algorithms Engineer" and "Signal Processing Systems Engineer" role families named early in this curriculum's career map.

### 14. Interview question

*"A UE reports a PMI selecting the best available codebook entry. Why does achieved SNR still typically fall short of the theoretical perfect-CSI MRT bound from Lesson 46?"* Answer from §5: the codebook is finite, so the selected entry sits at some nonzero angle $\theta_{err}$ from the true continuous-optimal direction; achieved gain scales by $\cos\theta_{err}<1$, SNR by $\cos^2\theta_{err}$ — a fundamental, derivable quantization loss inherent to any finite-feedback scheme, not a flaw in correctly-functioning selection logic.

### 15. Active recall

1. Derive the SNR-loss-versus-quantization-angle relationship from the geometry of the complex inner product.
2. Why can TDD systems largely avoid this feedback overhead while FDD systems cannot?
3. What's the precise difference between PMI and RI?

### 16. Mini assignment

Run §8/§9's sweep for a couple of different antenna counts and find, for each, roughly how many feedback bits are needed to bring SNR loss under a target such as 0.5 dB — connecting codebook design directly to a concrete, defensible number of required feedback bits.

### 17. Exit test

1. Write the SNR-loss formula in terms of quantization angle, from memory.
2. True or false, with a one-line reason: "A well-designed codebook-based beamforming system can, in principle, achieve the exact same SNR as perfect-CSI MRT."
3. Your reasoning from §11 about whether the observed throughput gap is necessarily a bug, if you haven't worked through it.

---

[← Lesson 47](lesson-47-beam-management.md) · [Table of Contents](../README.md) · [Lesson 49 →](lesson-49-proportional-fair-scheduling.md)