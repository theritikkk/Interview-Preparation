[← Table of Contents](../README.md) · [← Lesson 14](lesson-14-mimo.md)

---

# Lesson 15 — Real IQ Data: DC Offset, IQ Imbalance, and the Jump from Simulation to Hardware

*Opens Stage 5 — Software-Defined Radio. Lessons 1–14 were entirely simulation, where you controlled every impairment. Real captured data doesn't extend that courtesy — and it comes with two extremely common artifacts that have nothing to do with your algorithms being wrong.*

### 1. Intuition

Every lesson so far has been simulation: you generated the signal, you chose the noise, you knew the ground truth. Real captured IQ data is a different animal — there's no ground truth to check against, and the *hardware itself* introduces artifacts no textbook channel model captures. Two of these are so common, in every direct-conversion SDR you'll ever touch, that recognizing them on sight is a genuine, specific skill: a spurious spike sitting exactly at 0 Hz, and a faint, mirrored ghost of your own signal on the wrong side of the spectrum. Neither is a bug in your code. Both have precise, well-understood physical causes — and, satisfyingly, the second one is explained by exactly the same math that opened this entire curriculum.

### 2. The engineering problem that forces this

A typical SDR receiver mixes RF down to baseband directly (zero-IF / direct-conversion architecture), splitting the signal into I and Q paths using a local oscillator (LO). Two things go wrong in real hardware that never show up in an idealized simulation. First, the LO signal itself often leaks slightly into the receive path and mixes with itself — self-mixing — producing a spurious component at exactly 0 Hz, unrelated to anything actually transmitted: **DC offset**. Second, the I and Q analog paths are never perfectly matched — small differences in gain and exactly-90°-phase-separation are inevitable in real analog hardware — and this mismatch causes a scaled, mirrored copy of your own signal to leak in on the opposite side of DC: **IQ imbalance**. If you don't know to expect these, they look exactly like mysterious bugs in your own receiver code.

### 3. Mathematical foundation

**DC offset:** $r[n] = x[n] + c$, a constant $c$ added in time domain.

**IQ imbalance**, first-order model with amplitude mismatch $\varepsilon$ and phase mismatch $\phi$ between I and Q paths:
$$r[n] = \alpha\,x[n] + \beta\,x^*[n], \qquad \alpha \approx 1+\frac{\varepsilon+j\phi}{2}, \quad \beta \approx \frac{j\phi-\varepsilon}{2}$$

**Image rejection ratio (IRR):** $|\alpha|^2/|\beta|^2$, typically expressed in dB — a standard hardware specification quantifying how well-matched a receiver's I/Q paths are.

### 4. Formal definition

DC offset in the frequency domain is a spectral impulse exactly at $f=0$ — the direct consequence of taking the Fourier transform of a nonzero constant (a fact you already have from Lesson 3's transform machinery: a constant in time is a delta in frequency). IQ imbalance's defining signature is the $\beta\,x^*[n]$ term: since conjugation in time corresponds to spectral mirroring ($x^*[n]\leftrightarrow X^*(-f)$, directly from Lesson 3's transform properties), this term is literally a scaled, frequency-flipped copy of your own signal — an **image**, appearing symmetric to your real signal around DC.

### 5. Derivation

**DC offset** needs no derivation beyond what §4 already states — it's a direct restatement of Lesson 3's transform-of-a-constant fact, now interpreted physically as LO self-mixing leakage.

**IQ imbalance — worth deriving properly, since it directly closes the loop on Lesson 1.** Model the Q path with gain $(1+\varepsilon)$ and phase error $\phi$ relative to ideal quadrature:
$$r[n] = I[n] + j\big[(1+\varepsilon)\big(Q[n]\cos\phi + I[n]\sin\phi\big)\big]$$
For small $\varepsilon,\phi$ (first-order approximation, $\cos\phi\approx1$, dropping the $\varepsilon\phi$ cross term):
$$r[n] \approx I[n] + jQ[n] + j\varepsilon Q[n] + j\phi I[n] = x[n] + j\varepsilon Q[n] + j\phi I[n]$$
Now express the correction terms in terms of $x[n]=I[n]+jQ[n]$ and $x^*[n]=I[n]-jQ[n]$, using $I[n]=\tfrac{x+x^*}{2}$ and $Q[n]=\tfrac{x-x^*}{2j}$:
$$j\varepsilon Q[n] = \frac{\varepsilon(x-x^*)}{2}, \qquad j\phi I[n] = \frac{j\phi(x+x^*)}{2}$$
Summing and collecting terms in $x[n]$ and $x^*[n]$ separately:
$$r[n] = x[n]\left(1+\frac{\varepsilon+j\phi}{2}\right) + x^*[n]\left(\frac{j\phi-\varepsilon}{2}\right) = \alpha\,x[n] + \beta\,x^*[n]$$
— exactly §3's model, derived rather than asserted. The physical meaning is now completely explicit: any I/Q gain or phase mismatch, no matter how small, inevitably produces a term proportional to your own signal's *conjugate* — and a conjugate is a mirror image in frequency (Lesson 1's entire premise, now showing you its dark side: the very property that let you separate $+f$ from $-f$ is exactly what gets *undermined* when the hardware generating your I and Q isn't perfectly matched).

### 6. Visualization

```
Spectrum of a real capture, before correction:

        │ ← DC spike (LO self-mixing)
        │
 ───────┼────╱╲──────────────────────  ← desired signal
        │   ╱  ╲
────────┼──╱────╲──╱╲ ───────────────  ← mirrored image (IQ imbalance),
        │        ╲╱  ╲                   scaled down, flipped around f=0
       f=0
```

### 7. Worked example

$\varepsilon=0.05$ (5% gain mismatch), $\phi=5° \approx 0.0873$ rad: $\alpha \approx 1.025+j0.0437$, $\beta\approx-0.025+j0.0437$. $|\alpha|^2\approx1.0525$, $|\beta|^2\approx0.00253$. IRR $=1.0525/0.00253\approx416$, or $10\log_{10}(416)\approx 26$ dB. That's a realistic, unremarkable image-rejection figure for inexpensive consumer SDR hardware — meaning even a fairly small, entirely typical hardware imperfection produces an image roughly 26 dB below your desired signal, easily strong enough to matter for a weak received signal near the noise floor.

### 8. Code

```python
import numpy as np

N = 2048
t = np.arange(N)
x = np.exp(1j*2*np.pi*0.1*t)                     # ideal complex baseband tone

eps, phi = 0.05, np.deg2rad(5)
alpha = 1 + (eps + 1j*phi)/2
beta  = (1j*phi - eps)/2
dc = 0.3 + 0.1j

r = alpha*x + beta*np.conj(x) + dc               # simulate real-hardware capture

# --- Correction 1: DC offset removal ---
r_dc_removed = r - np.mean(r)

# --- Correction 2: Gram-Schmidt IQ imbalance correction ---
I, Q = r_dc_removed.real, r_dc_removed.imag
Q_scaled = Q * (np.std(I) / np.std(Q))                       # fix amplitude mismatch
rho = np.mean(I * Q_scaled) / (np.std(I) * np.std(Q_scaled))  # correlation -> phase mismatch
Q_corrected = (Q_scaled - rho * I) / np.sqrt(1 - rho**2)
r_corrected = I + 1j*Q_corrected

for label, sig in [("raw", r), ("corrected", r_corrected)]:
    spec = np.abs(np.fft.fft(sig))
    print(label, " DC bin magnitude:", spec[0], " image-band peak:", np.max(spec[N//2-20:N//2]))
```

### 9. Experiment

Run §8's code and compare the FFT of `r` versus `r_corrected`: the raw spectrum should show a clear spike at bin 0 (DC) and a visible secondary peak mirrored around DC from the true tone's location; the corrected spectrum should show both substantially suppressed, approaching a single clean tone much closer to the ideal `x`. This won't be perfect — Gram-Schmidt correction is a simple, blind, first-order fix, not a full calibration — but the improvement should be large and clearly visible.

### 10. Expected output

Raw signal: strong DC bin, clear image peak. Corrected signal: DC bin near zero, image peak dramatically reduced (tens of dB improvement, consistent with §7's IRR reasoning) — direct, numeric confirmation that a real, common hardware impairment can be significantly (if not perfectly) undone with a simple, well-understood statistical correction.

### 11. Debugging exercise

Symptom, no cause given: you capture a real signal with an SDR, confident you're tuned correctly and that the transmitted signal itself is clean. Your spectrum shows a strong, narrow spike exactly at 0 Hz, and a fainter, mirrored copy of your actual signal appears on the opposite side of the spectrum from where your signal should be.

Before reading on: these are actually **two separate artifacts** with two separate causes. Identify each one and its fix, using only what's in this lesson.

**Root cause (revealed):** the spike at exactly 0 Hz is **DC offset**, from local-oscillator self-mixing/leakage — a hardware artifact essentially unrelated to the transmitted signal, fixed by subtracting the mean (or a slowly-adapting DC-blocking filter, per §12). The mirrored copy is **IQ imbalance**, from I/Q gain or phase mismatch — derived precisely in §5 as a $\beta\,x^*[n]$ term — fixed by a Gram-Schmidt-style correction like §8's. These are genuinely independent problems that happen to show up together constantly, because both stem from the same class of hardware (direct-conversion front ends), and every real SDR project has to handle both before any of Lessons 1–14's clean algorithms will work correctly on real captured data.

### 12. Common mistakes

- Mistaking the DC spike for actually-transmitted content — for the overwhelming majority of real cellular/broadcast signals, genuine content is essentially never carried at exactly DC; a spike there is almost always a receiver artifact.
- Guessing "swap I and Q" when seeing a mirrored spectrum — that fixes a *different* bug (a sign-negated axis, e.g. from an inverted spectrum convention), not IQ gain/phase imbalance; the two look superficially similar but have unrelated causes and unrelated fixes.
- Removing DC offset with a single global mean subtracted over an entire long capture, when real DC offset often drifts slowly over time (temperature, AGC changes) — a single static subtraction can under- or over-correct at different points in a long recording; real systems use a slowly-adapting or block-wise DC removal instead.

### 13. Real RF/cellular application

DC offset and IQ imbalance calibration are mandatory, standard steps in the RF front-end initialization of essentially every real cellular modem and SDR device, performed before any PHY-layer processing runs. 3GPP RF conformance testing explicitly specifies limits on image rejection ratio and DC offset as measurable receiver performance requirements — parameters that Keysight, Rohde & Schwarz, and Anritsu equipment are literally built to test, connecting this lesson directly and concretely to the RF/wireless test-engineering career track from Part 1 of this curriculum's career map.

### 14. Interview question

*"You capture an OFDM signal and see a spurious peak exactly at DC plus a faint mirrored copy of your signal elsewhere in the spectrum. What are the two standard explanations, and how would you tell them apart?"* Reason it out: a DC spike stays fixed at 0 Hz regardless of where your actual signal sits or what frequency you tune to — a hardware self-mixing artifact independent of the signal. An image specifically mirrors and moves *with* wherever your real signal's spectral content is — if you retune, the image retunes with it, because it's mathematically derived from your own signal (§5), not from the LO alone.

### 15. Active recall

1. Physically, what causes DC offset in a direct-conversion receiver?
2. Why does IQ imbalance specifically produce a term proportional to $x^*[n]$ rather than some other distortion?
3. What's the difference between the "swap I and Q" bug and true IQ imbalance?

### 16. Mini assignment

Using §8's code, compute the image rejection ratio numerically (ratio of power at the desired tone's bin to power at its mirrored image bin) both before and after correction, and report the improvement in dB. Then repeat with a larger imbalance ($\varepsilon=0.15$, $\phi=15°$) and see how much correction quality degrades — Gram-Schmidt's first-order approximation has real limits.

### 17. Exit test

1. Write the IQ imbalance model equation from memory, and identify which term causes the mirrored image.
2. True or false, with a one-line reason: "DC offset in a captured signal usually indicates the transmitter is sending real content at 0 Hz."
3. Your two-cause identification from §11, if you haven't sent it.

---

**Next:** Lesson 16 stays in Stage 5 and moves into **GNU Radio** properly — building an actual flowgraph, understanding its scheduler and buffer model, and decoding a real broadcast FM signal end to end using nothing but concepts you already have.

---

[← Lesson 14](lesson-14-mimo.md) · [Table of Contents](../README.md) · [Lesson 16 →](lesson-16-gnu-radio-fm-demod.md)