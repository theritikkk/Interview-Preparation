[← Table of Contents](../README.md)

---

# Lesson 1 — Complex Numbers, Phasors, and Why Radio Needs Two Dimensions

*Stage 0 — Mathematical Foundations. Prerequisite for everything downstream: IQ signals, DSP, OFDM, and every cellular PHY chapter in this book.*

### 1. Intuition

Real hardware can only push a voltage up and down over time — one real number per instant. But a radio wave actually carries *two* independent pieces of information at every instant: how big it is (amplitude) and where it is in its cycle (phase). A single real number can't hold both at once without ambiguity. A **complex number** is just an accounting trick that lets you carry "amplitude and phase" — or equivalently "two real numbers that rotate together" — as one mathematical object. Once you see a sinusoid as the *shadow* of something spinning in a 2D plane, IQ samples, OFDM subcarriers, and channel estimation all stop looking like separate topics and start looking like the same picture from different angles.

### 2. The engineering problem that forces this

Here's the concrete problem. Say you want to shift a signal from RF down to baseband (this is literally what every receiver's front end does). With real signals, you multiply by a real cosine — but $\cos(a)\cos(b) = \tfrac{1}{2}[\cos(a-b) + \cos(a+b)]$. You get a sum term *and* a difference term, and worse: $\cos(-\theta) = \cos(\theta)$, so a real cosine cannot tell the difference between $+f$ and $-f$. Two genuinely different signals — one just above your carrier, one just below — fold on top of each other and become indistinguishable. That's a real engineering failure, not an abstraction. Complex exponentials fix this because $e^{j\theta} \neq e^{-j\theta}$ — sign of frequency is preserved. This one property is the entire reason IQ representation exists.

### 3. Mathematical foundation

A complex number is $z = a + jb$, where $a$ (real part) and $b$ (imaginary part) are ordinary real numbers, and $j$ satisfies $j^2 = -1$. (Electrical engineering uses $j$, not $i$, specifically to avoid clashing with current, which is also called $i$.) The same number has a polar form: $z = |z|\,e^{j\theta}$, where $|z| = \sqrt{a^2+b^2}$ is the magnitude and $\theta = \operatorname{atan2}(b,a)$ is the phase. The bridge between these two forms is **Euler's formula**:

$$e^{j\theta} = \cos\theta + j\sin\theta$$

### 4. Formal definition

$\mathbb{C}$ is the field of numbers $a+jb$ with $a,b \in \mathbb{R}$, equipped with addition $(a+jb)+(c+jd) = (a+c)+j(b+d)$ and multiplication $(a+jb)(c+jd) = (ac-bd) + j(ad+bc)$. The **conjugate** $z^* = a - jb$ flips the sign of the imaginary part; note $z z^* = a^2+b^2 = |z|^2$ — this is exactly how you'll compute signal power from IQ samples later. Multiplication in polar form is the part that matters most for you: $z_1 z_2 = |z_1||z_2|\,e^{j(\theta_1+\theta_2)}$ — **magnitudes multiply, phases add.** Hold onto that; it's the whole mechanism behind frequency mixing.

### 5. Derivation (why Euler's formula is true, not just a rule to memorize)

Take the Taylor series of $e^x$ and substitute $x = j\theta$:

$$e^{j\theta} = \sum_{n=0}^{\infty}\frac{(j\theta)^n}{n!} = 1 + j\theta - \frac{\theta^2}{2!} - j\frac{\theta^3}{3!} + \frac{\theta^4}{4!} + \cdots$$

Because $j^2=-1, j^3=-j, j^4=1$, this cycles. Collecting purely real terms gives $1-\tfrac{\theta^2}{2!}+\tfrac{\theta^4}{4!}-\cdots = \cos\theta$, and collecting the $j$-multiplied terms gives $j(\theta - \tfrac{\theta^3}{3!}+\cdots) = j\sin\theta$. Add them: $e^{j\theta}=\cos\theta+j\sin\theta$. Now the payoff — solve for $\cos\theta$ alone by also writing $e^{-j\theta}=\cos\theta-j\sin\theta$ and adding:

$$\cos\theta = \frac{e^{j\theta}+e^{-j\theta}}{2}$$

**This is the single most important equation in this lesson.** It says a real cosine is nothing but two phasors spinning in opposite directions, added together and averaged. One spins counter-clockwise at $+\theta$ (positive frequency), one clockwise at $-\theta$ (negative frequency). A real-valued receiver can never separate them — it only ever sees their sum. An IQ (complex) receiver keeps them apart. That's the derivation behind why negative frequency is a real, physically meaningful thing in this field and not a mathematical curiosity.

### 6. Visualization

```
        Im
         |
         |    z = e^(jθ)
         |   ↗
         | ↗  θ
---------+-------- Re
         |
         |
```

As $\theta$ increases linearly with time ($\theta = 2\pi f t$), the point spins counter-clockwise around the unit circle at constant speed. Its **shadow on the real axis** — its horizontal projection — is exactly $\cos(2\pi f t)$, oscillating up and down. The spinning point carries strictly more information (it also tells you the sign of the frequency) than its shadow does.

### 7. Worked example

Let $z = 3 + 4j$. Magnitude: $|z| = \sqrt{3^2+4^2} = 5$. Phase: $\theta = \operatorname{atan2}(4,3) \approx 53.13°$. So $z = 5e^{j53.13°}$. Now multiply by $e^{j90°}$ (i.e., by $j$, since $j = e^{j90°}$): magnitudes multiply ($5 \times 1 = 5$), phases add ($53.13° + 90° = 143.13°$) — a pure 90° rotation, no change in size. Check directly: $j(3+4j) = 3j + 4j^2 = -4+3j$, and indeed $\sqrt{16+9}=5$. Multiplying by $j$ *is* a 90° rotation — this is the algebraic fact behind every quadrature (I/Q) mixer in existence.

### 8. Code

```python
import numpy as np
import matplotlib.pyplot as plt

fs = 1000          # sample rate, Hz
f  = 5              # signal frequency, Hz
t  = np.arange(0, 1, 1/fs)

theta = 2*np.pi*f*t
z = np.exp(1j*theta)          # the rotating phasor: e^(j*theta)

# Confirm: real part of the phasor should equal cos(2*pi*f*t)
real_part = z.real
cosine    = np.cos(theta)
print("max difference:", np.max(np.abs(real_part - cosine)))  # should be ~0

# Confirm the derivation: (e^{j theta} + e^{-j theta}) / 2 == cos(theta)
reconstructed = (np.exp(1j*theta) + np.exp(-1j*theta)) / 2
print("max diff from derivation:", np.max(np.abs(reconstructed.real - cosine)))
```

### 9. Experiment

Run the code above, then modify it: plot `z.real` vs `t` (should look like an ordinary cosine) and separately plot `z.real` vs `z.imag` (should trace a circle, not a line). Then compute `np.exp(1j*theta) * np.exp(1j*np.pi/2)` for a few values of `theta` and confirm numerically that it matches `np.exp(1j*(theta + np.pi/2))` — verifying the "phases add" rule from §4 empirically, not just algebraically.

### 10. Expected output

The `t` vs `real_part` plot is a smooth cosine wave. The `real` vs `imag` plot is a perfect circle traced counter-clockwise. Both "max difference" print statements should output a number effectively zero (on the order of $10^{-15}$ or $10^{-16}$ — floating-point roundoff, not a real discrepancy).

### 11. Debugging exercise

Here's the symptom, no cause given yet: You write code to generate what should be a 5 Hz cosine using the complex-exponential method from §8. When you plot `z.real`, the wave looks correct in shape, but it appears to complete its cycles in *half* the time you expect — it looks like a 10 Hz signal, not 5 Hz.

What would you check first? Form a hypothesis before reading on. (Hint: what quantities feed into `theta`, and which of them, if doubled or mis-scaled, would double the apparent frequency without changing the waveform's shape?) Work out your hypothesis and what you'd print or plot to test it before moving on.

**Root cause (revealed):** the most common culprit is `theta = 2*np.pi*f*t` being computed with `f` accidentally doubled somewhere upstream (e.g., a copy-pasted sample-rate variable reused as the frequency), or `t` built with the wrong step size so twice as many cycles fit in the same array. The fix is always the same: print `theta[1] - theta[0]` and compare it by hand to `2*np.pi*f/fs` — if they don't match, the bug is upstream of `theta`, not in the complex-exponential math itself.

### 12. Common mistakes

- Mixing degrees and radians — `np.cos` and `np.exp` expect radians; a stray `theta_deg` fed in unconverted silently produces garbage-looking phase, not an error.
- Writing $i$ out of habit instead of $j$ when reading EE/DSP material — harmless mathematically, but it'll make you stumble reading datasheets and papers that assume $j$.
- Confusing $|z|$ (magnitude, linear amplitude) with $|z|^2$ (power) — this shows up constantly later when computing SNR and will cost you a factor of 2 in dB if you mix them up.
- Assuming $e^{-j\theta}$ is "the same frequency, just delayed" — it is not; it's the **negative-frequency** component, a physically distinct thing once you're doing complex/IQ processing.

### 13. Real RF/cellular application

The complex exponential $e^{j2\pi f t}$ you just generated in code *is*, almost literally, what comes out of an SDR's ADC after quadrature downconversion — that stream of complex numbers is exactly your IQ samples. Every single OFDM subcarrier in LTE and NR is defined mathematically as one of these complex exponentials at a specific frequency offset; the entire OFDM transmitter is just a sum of many of these, computed efficiently via FFT (Stage 1). When you eventually estimate a wireless channel using reference/pilot signals, what you're estimating at each subcarrier is a single complex number — a magnitude (fading) and a phase (delay) — for exactly the reason derived in §5.

### 14. Interview question

*"Why do receivers use complex (I/Q) baseband representation instead of just sampling the RF signal directly with a fast ADC?"* Don't answer from memory — reason it out: what does a real-valued signal fail to distinguish (§2)? What does the Nyquist sampling rate requirement look like for a real bandpass signal centered at a high carrier frequency, versus a complex baseband signal after downconversion? (We'll formalize the sampling-rate side of this fully once we hit the sampling theorem in Stage 1 — for now, reason only from what you know today.)

### 15. Active recall (answer without scrolling back up)

1. What does multiplying two phasors do to their magnitudes? To their phases?
2. Write $\cos\theta$ purely in terms of $e^{j\theta}$ and $e^{-j\theta}$.
3. Why can't a real-valued cosine distinguish $+f$ from $-f$?

### 16. Mini assignment

By hand (paper, no code): compute $z_1 z_2$ for $z_1 = 2e^{j30°}$ and $z_2 = 3e^{j60°}$, in polar form. Then convert both $z_1$ and $z_2$ to rectangular ($a+jb$) form, multiply them out using the FOIL/distribution rule from §4, and confirm you get the same answer.

### 17. Exit test

Before moving to Lesson 2 (sampling, DTFT, and the FFT), answer these:
1. $j \cdot j \cdot j = ?$ (simplify to $\pm 1$ or $\pm j$)
2. If $z = 5e^{j45°}$, what is $z \cdot z^*$?
3. In your own words (not a memorized definition): what physical ambiguity does complex/IQ representation resolve that a real signal cannot?

---

← [16. Glossary](../16-glossary.md) · [Table of Contents](../README.md)
