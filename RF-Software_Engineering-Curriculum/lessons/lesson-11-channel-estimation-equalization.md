[← Table of Contents](../README.md) · [← Lesson 10](lesson-10-multipath-fading.md)

---

# Lesson 11 — Channel Estimation and Equalization

*Stage 4 — Wireless Communications, continued. Where Lesson 10's "the channel is just convolution" becomes "and here's how you undo it."*

### 1. Intuition

Lesson 10 showed the channel behaves as an unknown filter that distorts the transmitted signal. If you knew that filter exactly, undoing its effect would just be inverse filtering — divide it back out. The catch is you don't know the channel in advance; it depends on the environment, and it changes over time. So real receivers do something clever: embed a few **known** symbols (pilots/reference signals) in the transmission, measure how the channel distorted *those* (since you know what was sent), and use that measurement to correct everything else. Measure, then undo — that's the whole idea, and it turns out the "undo" part has a genuinely important subtlety that naive intuition gets wrong.

### 2. The engineering problem that forces this

Given known pilot symbols at known positions, corrupted by both the multipath channel from Lesson 10 *and* noise, how do you best estimate the channel, and then best invert it for the *unknown* data symbols sitting around those pilots? The naive approach — "just divide out the channel" — has a serious flaw: wherever the channel happens to be weak (a deep fade, Lesson 10 §7), dividing by a near-zero number **amplifies noise catastrophically**. A good equalizer has to know when *not* to fully trust its own channel estimate.

### 3. Mathematical foundation

**Least-squares (LS) channel estimate**, per subcarrier/frequency bin, using known pilot $P[k]$ and received $Y[k]$:
$$\hat H[k] = \frac{Y[k]}{P[k]}$$

**Zero-forcing (ZF) equalizer:** $\hat X[k] = Y[k]/\hat H[k]$ — fully inverts the estimated channel.

**MMSE equalizer:**
$$W_{MMSE}[k] = \frac{H^*[k]}{|H[k]|^2 + \sigma_n^2/\sigma_x^2}$$

### 4. Formal definition

The LS estimate treats the single pilot observation as a one-equation, one-unknown system and solves it exactly (noise rides along uncorrected). ZF then applies the *inverse* of that estimate directly to data. MMSE instead is defined as the linear filter $W[k]$ that minimizes mean-squared estimation error $E\big[|X[k]-W[k]Y[k]|^2\big]$ — not "invert the channel," but "balance channel-inversion against noise," derived properly in §5.

### 5. Derivation

**(a) The LS estimate is exact for a single pilot.** With $Y[k] = H[k]P[k] + N[k]$ and $P[k]$ known, there's one equation and one unknown ($H[k]$); solving for it directly gives $\hat H[k]=Y[k]/P[k]$ exactly — the noise term $N[k]/P[k]$ just comes along for the ride, meaning $\hat H[k]$ is itself a *noisy* estimate of the true channel, not the true channel — worth remembering for §12.

**(b) MMSE, derived properly via the orthogonality principle.** We want the linear estimator $W[k]$ minimizing $E\big[|X[k]-W[k]Y[k]|^2\big]$. The standard result for linear MMSE estimation is the **orthogonality principle**: at the optimum, the estimation error is uncorrelated with the observation:
$$E\big[(X[k]-W[k]Y[k])\,Y^*[k]\big] = 0$$
Expand, using $Y[k]=H[k]X[k]+N[k]$ with $X[k]$, $N[k]$ independent, zero-mean, and variances $\sigma_x^2,\sigma_n^2$:
$$E[X[k]Y^*[k]] = E\big[X[k](H^*[k]X^*[k]+N^*[k])\big] = H^*[k]\,\sigma_x^2$$
$$E[|Y[k]|^2] = |H[k]|^2\sigma_x^2 + \sigma_n^2$$
Substituting into the orthogonality equation and solving for $W[k]$:
$$W[k] = \frac{H^*[k]\,\sigma_x^2}{|H[k]|^2\sigma_x^2 + \sigma_n^2} = \frac{H^*[k]}{|H[k]|^2 + \sigma_n^2/\sigma_x^2}$$
— exactly the formula in §3. Now check it at the extremes, the same "known special case" instinct from Lesson 8 §5(c): as $\sigma_n^2\to0$ (noiseless), $W[k]\to H^*[k]/|H[k]|^2 = 1/H[k]$ — **MMSE reduces exactly to ZF** when there's no noise to protect against, which is exactly what you'd want. But as $|H[k]|\to0$ (a deep fade, from Lesson 10), ZF's $1/H[k]$ **blows up** — catastrophic noise amplification — while MMSE's numerator $H^*[k]\to0$ *faster* than the now-noise-dominated denominator shrinks, so $W[k]\to0$ gracefully instead. MMSE doesn't try to desperately recover data from a subcarrier that's essentially pure noise; ZF does, and pays for it.

### 6. Visualization

```
Equalizer gain |W[k]| near a deep channel fade:

ZF:    ────╲          ╱────     ← spikes toward infinity at the null
            ╲___null__╱

MMSE:  ────╲___null___╱────     ← rolls off gracefully instead
```

### 7. Worked example

Subcarrier with $H[k]=0.1$ (a deep fade) and noise-to-signal ratio $\sigma_n^2/\sigma_x^2 = 0.01$. ZF gain: $1/0.1 = 10$ — amplifies noise power by $100\times$ at this subcarrier alone. MMSE gain: $0.1/(0.01+0.01) = 5$ — noticeably more conservative, trading some signal recovery for avoiding a noise catastrophe.

### 8. Code

```python
import numpy as np

N = 64
h_ch = np.array([1.0, 0.5*np.exp(1j*np.pi/3), 0.2*np.exp(-1j*np.pi/4)])
H = np.fft.fft(h_ch, N)                          # true per-subcarrier channel (Lesson 10's tool)

pilots = np.ones(N, dtype=complex)                # known pilot on every subcarrier, for simplicity
snr_linear = 20
noise_var = 1/snr_linear
noise = np.sqrt(noise_var/2)*(np.random.randn(N)+1j*np.random.randn(N))

Y_pilot = H*pilots + noise
H_est = Y_pilot/pilots                            # LS estimate, §5(a)

# Data symbols on the same subcarriers
data = (2*np.random.randint(0,2,N)-1).astype(complex)
Y_data = H*data + np.sqrt(noise_var/2)*(np.random.randn(N)+1j*np.random.randn(N))

X_zf   = Y_data / H_est
X_mmse = (np.conj(H_est) / (np.abs(H_est)**2 + noise_var)) * Y_data

print("ZF error:  ", np.mean(np.abs(X_zf - data)**2))
print("MMSE error:", np.mean(np.abs(X_mmse - data)**2))
```

### 9. Experiment

Run §8's code and compare per-subcarrier error at the frequencies where `np.abs(H)` is smallest (the notches from Lesson 10's frequency-selective fading) versus where it's largest. ZF's error should spike dramatically at the notches; MMSE's should rise more mildly. Then sweep SNR and plot overall BER for both — at high SNR they should converge (matching §5(b)'s noiseless-limit result); at low-to-moderate SNR, MMSE should clearly outperform ZF.

### 10. Expected output

ZF shows large, spiky errors concentrated exactly at the channel's frequency nulls. MMSE shows smaller, more evenly distributed errors. At high SNR the two converge; at realistic SNR levels, MMSE meaningfully outperforms ZF — directly visible in the numbers, not just asserted.

### 11. Debugging exercise

Symptom, no cause given: your zero-forcing equalizer works well most of the time across many random channel trials, but occasionally — not due to any code bug — BER spikes catastrophically for a particular channel realization, far worse than the average SNR would suggest, and this happens more often the more trials you run.

What's happening, statistically? (Hint: you don't need new code to answer this — you need to think about what a *random* multipath channel occasionally does, per Lesson 10.)

**Root cause (revealed):** this is exactly §5(b)'s pathology, showing up empirically rather than by construction. A random multipath channel occasionally produces a realization with a very deep null at (or near) one of your active subcarriers — Lesson 10 §5(b) established that fading magnitude follows a Rayleigh distribution, and a Rayleigh random variable *does* occasionally land very close to zero. When that happens, ZF's $1/H[k]$ explodes for that trial, dominating the error statistics even though every other subcarrier was fine. This isn't a bug to fix in your code — it's the *actual, correct behavior of zero-forcing*, and it's precisely the argument for using MMSE (or techniques like erasure/puncturing of severely faded subcarriers) in any real system.

### 12. Common mistakes

- Applying ZF/MMSE equalization by inverting the full **time-domain** convolution (a large matrix inversion) instead of exploiting the clean **frequency-domain**, per-subcarrier division — the frequency-domain shortcut only works this simply when the channel's convolution is effectively *circular*, which is precisely why OFDM's cyclic prefix (coming next lesson) matters so much.
- Forgetting that $\hat H[k]$ from §5(a) is itself **noisy**, not the true channel — every equalizer's performance is fundamentally capped by channel-estimation quality, not just by which equalization formula you use.
- Ignoring pilot density relative to the channel's coherence bandwidth and coherence time (Lesson 10 §3) — pilots spaced too sparsely in frequency or time can't track a channel that varies faster than the pilot spacing allows, no matter how good the equalizer formula is.

### 13. Real RF/cellular application

This lesson *is* what LTE's CRS and NR's DMRS/CSI-RS reference signals exist for: known pilot patterns placed in the time-frequency resource grid specifically so the receiver can perform exactly this LS-style estimation per subcarrier, then equalize — real systems use MMSE-family equalization, not naive ZF, for precisely the robustness reason derived in §5(b). And the pilot *density* 3GPP specifies for different scenarios is directly governed by the expected coherence bandwidth and coherence time from Lesson 10 — closing that loop concretely.

### 14. Interview question

*"Why do practical receivers use MMSE equalization instead of zero-forcing, even though ZF perfectly removes channel distortion when the estimate is exact?"* Answer from §5(b): ZF's "perfect" inversion is only perfect in a noiseless world; with real noise present, it catastrophically amplifies noise wherever the channel is weak, while MMSE explicitly trades off signal recovery against noise amplification and degrades gracefully instead of catastrophically.

### 15. Active recall

1. Why does the LS channel estimate in §5(a) come out "exact" despite the channel being noisy?
2. In the noiseless limit, what does the MMSE equalizer formula reduce to, and why does that make sense?
3. What specific statistical property of Rayleigh fading (Lesson 10) explains why ZF occasionally fails catastrophically?

### 16. Mini assignment

Using §8's code, deliberately construct a channel with one very deep null (e.g. two nearly-canceling taps), and quantify — with actual numbers — how much worse ZF's error is than MMSE's specifically at the subcarrier nearest that null.

### 17. Exit test

1. Write the ZF and MMSE equalizer formulas from memory, then check against §3.
2. True or false, with a one-line reason: "A better channel estimate always fully solves ZF's noise-amplification problem."
3. Your statistical reasoning from §11, if you haven't sent it.

---

**Next:** Lesson 12 brings the FFT (Lesson 3), the cyclic-prefix trick (foreshadowed in §12 above), and this lesson's per-subcarrier equalization together into a complete **OFDM transmitter and receiver** — the single biggest payoff lesson in the curriculum so far, and the direct foundation of Project 4.

---

[← Lesson 10](lesson-10-multipath-fading.md) · [Table of Contents](../README.md) · [Lesson 12 →](lesson-12-ofdm-transceiver.md)