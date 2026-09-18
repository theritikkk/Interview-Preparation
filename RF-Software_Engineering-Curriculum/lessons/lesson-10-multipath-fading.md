[← Table of Contents](../README.md) · [← Lesson 9](lesson-09-pulse-shaping-rrc.md)

---

# Lesson 10 — Multipath and Fading: The Real Wireless Channel

*Opens Stage 4 — Wireless Communications. Every lesson so far assumed the channel was "just add noise." This is where that assumption gets replaced with something physically real — and it turns out to be built entirely from tools you already have.*

### 1. Intuition

Every lesson so far modeled the channel as AWGN — add some noise, done. Real wireless channels are messier: a transmitted signal doesn't travel to the receiver along one path. It reflects off buildings, the ground, vehicles, arriving via many distinct paths, each with its own delay, attenuation, and phase rotation. At the receiver, all these delayed, scaled copies **sum together**. Here's the reassuring part: this is not a new mathematical object. It's literally **convolution** — the channel itself behaves as an LTI system (Lesson 4), except this time nature designed the impulse response instead of you.

### 2. The engineering problem that forces this

Lessons 7–9 built matched receivers and BER curves assuming the *only* channel effect was additive noise. A real channel also **convolves** the transmitted signal with its own multi-tap impulse response before any noise is added — and this convolution isn't a minor correction. It causes a second, physically distinct kind of ISI (not the timing-related ISI from Lesson 9 — this one comes from genuinely different propagation paths), and it causes the received signal's amplitude to randomly rise and fall over time and frequency as those paths interfere constructively or destructively — **fading**. You need a model precise enough to predict this, the same way AWGN let you predict BER.

### 3. Mathematical foundation

**Multipath channel (tapped-delay-line model):**
$$h_{ch}[n] = \sum_i \alpha_i\,\delta[n-\tau_i], \qquad r[n] = (x * h_{ch})[n] + w[n]$$
where $\alpha_i$ are complex path gains and $\tau_i$ are path delays.

**Delay spread** $\tau_{spread}$: the time extent of the channel's taps. **Coherence bandwidth** $B_c \approx 1/\tau_{spread}$: the frequency range over which the channel's response stays roughly constant. **Doppler spread**: spread of frequency shifts across paths due to relative motion. **Coherence time** $T_c \approx 1/\text{Doppler spread}$: duration over which the channel stays roughly constant.

### 4. Formal definition

If the signal bandwidth is much smaller than $B_c$, the channel looks like a single, roughly constant complex gain across the whole signal — **flat fading**. If signal bandwidth exceeds $B_c$, different frequencies within the signal see genuinely different gains — **frequency-selective fading**, which is exactly what makes equalization necessary. **Rayleigh fading**: when no single path dominates (no line-of-sight), the channel gain's magnitude follows a Rayleigh distribution — derived properly in §5(b). **Rician fading**: Rayleigh plus one dominant line-of-sight path.

### 5. Derivation

**(a) Multipath is literally convolution.** A signal $x(t)$ traveling via path $i$ arrives delayed by $\tau_i$ and scaled/rotated by complex gain $\alpha_i$ (accounting for path loss and the phase shift from that path's length). The received signal is the sum over all paths:
$$r(t) = \sum_i \alpha_i\,x(t-\tau_i) + w(t) = (x * h_{ch})(t) + w(t), \qquad h_{ch}(t) = \sum_i \alpha_i\,\delta(t-\tau_i)$$
This is exactly Lesson 4's LTI/convolution framework — the physical channel simply *is* an impulse response, and every tool you already have (frequency response, filtering intuition) applies directly to it.

**(b) Why fading is Rayleigh — a direct callback to Lesson 1.** When many unresolved multipath components (too close in delay to separate) arrive with independent, effectively random phases, the total channel gain is a sum of many independent phasors — literally Lesson 1's $z=|z|e^{j\theta}$ objects, now random:
$$h = \sum_{i=1}^{N} \alpha_i e^{j\theta_i}, \quad \theta_i \text{ i.i.d. uniform}, \quad N \text{ large}$$
Split into real and imaginary parts: $h = \left(\sum_i \alpha_i\cos\theta_i\right) + j\left(\sum_i \alpha_i\sin\theta_i\right)$. Each sum is a sum of many independent, identically-distributed random terms — by the **Central Limit Theorem**, each converges to a zero-mean Gaussian, and by the symmetry of uniform phase, the two Gaussians end up independent with equal variance. A complex number with independent, equal-variance Gaussian real and imaginary parts is called **circularly symmetric complex Gaussian**, and the *magnitude* of such a number follows precisely the **Rayleigh distribution**. This isn't assumed — it falls directly out of applying the Central Limit Theorem to a sum of Lesson-1-style phasors, which is exactly why real, physically messy multipath so reliably produces this one clean, well-studied statistical shape.

**(c) Why coherence bandwidth $\approx 1/\tau_{spread}$ — Lesson 3's duality, again.** The channel's *frequency response* $H(f)$ is the Fourier transform of $h_{ch}(t)$ — literally the same transform machinery from Lesson 3, now applied to a channel instead of a signal. A signal spread over duration $\tau_{spread}$ in time has a Fourier transform that varies significantly over a frequency span of roughly $1/\tau_{spread}$ — the same time/frequency reciprocal relationship that gave you Nyquist's bandwidth (Lesson 2) and the pulse-bandwidth trade-off (Lesson 9). Applied here: a channel with more delay spread has a *narrower* coherence bandwidth — its frequency response wiggles faster (in frequency) the more spread out it is in time. Same duality, third appearance, three different physical settings.

### 6. Visualization

```
TX ──────┬───────────────────────────► direct path (short delay, strong)
          ╲
           ╲──────────────────────────► reflected path (longer delay, weaker, phase-shifted)
                                              ↓
                                            RX sums all arriving copies

Channel taps h[n]:        Frequency response |H(f)|:
  █                        ─╲    ╱╲    ╱─
  █     ▂                    ╲__╱  ╲__╱     ← frequency-selective "notches"
  0  τ1 τ2  → n (delay)          f →
```

### 7. Worked example

Two-tap channel $h = [1,\ 0.5e^{j\pi/3}]$. Frequency response $H(f) = 1 + 0.5e^{j\pi/3}e^{-j2\pi f}$ (a 1-sample delay contributes phase $e^{-j2\pi f}$, normalized units). At $f=0$: $H(0) = 1 + 0.5e^{j\pi/3} \approx 1.25 + 0.43j$, $|H(0)|\approx1.32$. At $f=0.5$ (half the normalized sample rate): $e^{-j\pi}=-1$, so $H(0.5) = 1 - 0.5e^{j\pi/3} \approx 0.75 - 0.43j$, $|H(0.5)|\approx0.87$. Same channel, two frequencies, visibly different gain — frequency-selective fading, made concrete with two numbers instead of a hand-wave.

### 8. Code

```python
import numpy as np

# A multipath channel: strong direct path + weaker delayed reflection
h_ch = np.array([1.0, 0.5*np.exp(1j*np.pi/3)])

# Reuse Lesson 9's pulse-shaped signal (conceptually) -- here a simple BPSK pulse train
bits = np.random.randint(0, 2, 500)
symbols = (2*bits - 1).astype(complex)

rx_multipath = np.convolve(symbols, h_ch, mode='full')   # the channel IS convolution

# Frequency response of the channel (zero-padded FFT, Lesson 3's tool)
H = np.fft.fft(h_ch, 256)
freqs = np.fft.fftfreq(256)
# plot np.abs(H) vs freqs -- look for frequency-selective dips
```

### 9. Experiment

Generate many (e.g. 10,000) independent Rayleigh-fading realizations using §5(b)'s recipe directly: sum $N=20$ random unit phasors with uniform random phase, take the magnitude of each sum, and histogram the results. Overlay the theoretical Rayleigh PDF and confirm they match — direct empirical proof of the Central Limit Theorem argument in §5(b). Then simulate BER over a **flat** Rayleigh-fading channel (multiply each symbol by one random complex Rayleigh gain, then add AWGN as in Lesson 7) and compare against the AWGN-only curve at the same *average* $E_b/N_0$.

### 10. Expected output

The simulated magnitude histogram should closely match the Rayleigh PDF shape. The fading BER curve should be **dramatically worse** than AWGN-only at the same average SNR — not a small gap, often orders of magnitude at moderate SNR — because even when *average* SNR looks fine, fading occasionally drives the *instantaneous* SNR very low, and those deep fades dominate the error rate. This is the single most important qualitative fact about fading channels: average SNR alone does not tell the whole story.

### 11. Debugging exercise

Symptom, no cause given: your Lesson 9 receiver worked perfectly in AWGN. You add a simple 2-tap multipath channel before the noise. Now BER is much worse than expected — even at **high** average SNR — and the eye diagram technique from Lesson 9 shows a permanently distorted, partially-closed eye that **doesn't open up** no matter how much you increase SNR.

What's the key diagnostic clue here — specifically, the fact that more SNR doesn't fix it? What kind of problem does that rule out, and what kind does it point to?

**Root cause (revealed):** this is the diagnostic signature that separates **noise** from **channel distortion**. Random noise closes an eye diagram with *blur/jitter that shrinks as SNR improves* — more signal power directly helps. Multipath-induced ISI closes the eye with a *fixed, deterministic distortion* baked in by the channel itself, before noise is ever added — throwing more power at the transmitter doesn't un-convolve the channel's impulse response. If more SNR doesn't fix an eye-diagram problem, the cause is systematic (channel or hardware), not random — and the fix here specifically is **equalization** (removing or compensating for the channel's own impulse response), not "more power."

### 12. Common mistakes

- Assuming "just improve SNR" fixes any BER problem — it fixes noise-induced errors, but does **nothing** for channel-induced ISI, which needs equalization, not power.
- Confusing **channel-induced ISI** (this lesson — from real, distinct propagation paths, doesn't improve with correct timing) with **pulse-shaping ISI** (Lesson 9 — from sampling at the wrong instant, fixed entirely by correct timing) — similar symptom (closed eye), completely different cause and fix.
- Assuming "flat fading" applies without checking it — whether a channel is flat or frequency-selective depends on **both** the channel's coherence bandwidth **and** your signal's bandwidth; the same physical channel is flat for a narrowband signal and frequency-selective for a wideband one.

### 13. Real RF/cellular application

This lesson is the direct physical justification for **why LTE and NR use OFDM** — and closes the loop opened in the very first message of this whole curriculum. A wideband signal experiences frequency-selective fading when its bandwidth exceeds the channel's coherence bandwidth; OFDM's response is to split that wideband transmission into many narrow subcarriers (Lesson 3's FFT/IFFT machinery), each individually much narrower than $B_c$ — so each subcarrier sees approximately **flat** fading, correctable with a single complex multiply instead of a complicated wideband time-domain equalizer. Coherence time and coherence bandwidth are also, quite literally, the physical parameters that determine how densely 3GPP reference signals (DMRS, CSI-RS) must be placed in time and frequency to track the channel.

### 14. Interview question

*"Why does 5G NR use OFDM, and specifically how does that help with multipath?"* Reason from this lesson: a wideband signal spans multiple coherence-bandwidth "chunks" of a frequency-selective channel; splitting it into many subcarriers each narrower than $B_c$ turns one hard wideband-equalization problem into many easy per-subcarrier flat-fading corrections. (The full mechanics of *how* OFDM implements this arrive once we build it directly, but the physical justification is entirely this lesson's.)

### 15. Active recall

1. Why is a multipath channel mathematically the same kind of object as an FIR filter?
2. What does the Central Limit Theorem have to do with why fading follows a Rayleigh distribution?
3. Why doesn't increasing transmit power fix channel-induced ISI?

### 16. Mini assignment

For a channel with taps at delays $[0, 0.5, 1.2]\,\mu s$, estimate the delay spread and coherence bandwidth. Then, for a signal with 200 kHz bandwidth and separately one with 5 MHz bandwidth, determine which one experiences this channel as roughly flat and which experiences it as frequency-selective.

### 17. Exit test

1. Write the received-signal equation for a multipath channel with $L$ taps, fully expanded (no summation notation) for $L=3$.
2. True or false, with a one-line reason: "A channel with more delay spread has a wider coherence bandwidth."
3. Your diagnostic reasoning from §11, if you haven't sent it — the noise-vs-systematic-distortion distinction is one of the most transferable debugging skills in this entire field.

---

**Next:** Lesson 11 stays in Stage 4 and tackles **channel estimation and equalization** — how a receiver actually measures $H(f)$ using known reference symbols (a direct application of Lesson 6's correlation machinery) and undoes the channel's distortion.

---

[← Lesson 9](lesson-09-pulse-shaping-rrc.md) · [Table of Contents](../README.md) · [Lesson 11 →](lesson-11-channel-estimation-equalization.md)