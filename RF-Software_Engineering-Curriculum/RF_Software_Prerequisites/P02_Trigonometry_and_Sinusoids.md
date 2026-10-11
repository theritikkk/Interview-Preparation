# P02 — Trigonometry and Sinusoids: Angles, the Unit Circle, Amplitude / Frequency / Phase

*Prerequisite module 2 · ⏱ about 4–5 hours · needs only P01's habit of carrying units · every number below was computed and checked.*

> **Where this unlocks the curriculum.** Lesson 1 (complex numbers and phasors) opens by using the product identity $\cos a\cos b=\tfrac12[\cos(a-b)+\cos(a+b)]$ and the fact that $\cos(-\theta)=\cos\theta$. Lessons 3, 10, 12, 13 and 16 lean on radians, phase and frequency throughout. If this module is solid, Lesson 1 reads as *"oh, that's a neat packaging of what I already know"* instead of a wall.

---

## 2.1 Why trigonometry is the language of radio

A radio wave is, at its core, a **sinusoid**: a quantity that goes up and down smoothly and repeats. Everything a radio does to information — shift it to another frequency, filter it, decode it — is arithmetic on sinusoids. Trigonometry is the vocabulary for talking about them: *how big* (amplitude), *how fast* (frequency), *where in its cycle* (phase).

---

## 2.2 Angles: degrees vs radians

* **Degrees:** a full turn is 360°. Convenient for people, awkward for maths.
* **Radians:** the angle whose arc length equals the radius. A full turn is $2\pi$ rad (≈ 6.283); half a turn is $\pi$.

$$\text{rad}=\text{deg}\times\frac{\pi}{180},\qquad \text{deg}=\text{rad}\times\frac{180}{\pi}$$

| Degrees | Radians | As a fraction of a turn |
|---|---|---|
| 30° | π/6 ≈ 0.524 | 1/12 |
| 45° | π/4 ≈ 0.785 | 1/8 |
| 60° | π/3 ≈ 1.047 | 1/6 |
| 90° | π/2 ≈ 1.571 | 1/4 |
| 180° | π ≈ 3.142 | 1/2 |
| 270° | 3π/2 ≈ 4.712 | 3/4 |
| 360° | 2π ≈ 6.283 | 1 |

**Why radians?** Calculus and Euler's formula ($e^{j\theta}$, Lesson 1) are *clean* only in radians, and DSP measures frequency in **radians per sample**. **Programming rule: every `np.sin`/`np.cos`/`np.exp(1j*…)` call takes radians.** Passing degrees is the most common silent bug in beginner DSP code.

---

## 2.3 The unit circle: sine and cosine as coordinates

Draw a circle of radius 1 centred at the origin. A point at angle $\theta$ (measured counter-clockwise from the +x axis) has coordinates

$$(x,y)=(\cos\theta,\ \sin\theta)$$

```
            y (sin)
             |
      (cosθ, sinθ) •
             |   ╱
             |  ╱ θ
 ------------+--------- x (cos)
             |
```
So **cosine is the horizontal "shadow"** of the arrow and **sine is the vertical shadow**. As θ increases the arrow rotates counter-clockwise; the shadows oscillate. *This single picture is the seed of the phasor in Lesson 1.*

| θ | cos θ | sin θ |
|---|---|---|
| 0° | 1 | 0 |
| 30° | 0.866 | 0.5 |
| 45° | 0.707 | 0.707 |
| 60° | 0.5 | 0.866 |
| 90° | 0 | 1 |
| 180° | −1 | 0 |
| 270° | 0 | −1 |

**Facts to carry:**
1. $\sin^{2}\theta+\cos^{2}\theta=1$ (a point on the unit circle satisfies $x^{2}+y^{2}=1$).
2. Both repeat every $2\pi$: $\cos(\theta+2\pi)=\cos\theta$.
3. **Cosine is even, sine is odd:** $\cos(-\theta)=\cos\theta$, $\sin(-\theta)=-\sin\theta$. *(Hold on to this: it is exactly why a real signal cannot tell positive from negative frequency — §2.6.)*
4. **Sine is cosine delayed by 90°:** $\sin\theta=\cos(\theta-90^\circ)$. The pair is called a **quadrature** pair ("a quarter turn apart") — the two components of every IQ signal.

---

## 2.4 Anatomy of a sinusoid

$$x(t)=A\cos\!\bigl(2\pi f\,t+\varphi\bigr)=A\cos(\omega t+\varphi)$$

| Symbol | Name | Unit | Meaning |
|---|---|---|---|
| $A$ | amplitude | volts (or any signal unit) | peak height; the maximum of $x(t)$ |
| $f$ | frequency | Hz = cycles/s | how many full cycles per second |
| $\omega=2\pi f$ | angular frequency | rad/s | how fast the *angle* advances (a 15 kHz tone: $\omega=94{,}248$ rad/s) |
| $T=1/f$ | period | s | time of one cycle (50 Hz → 20 ms) |
| $\varphi$ | phase | rad (or °) | where in its cycle the wave is at $t=0$ |

**Average power** of $A\cos(\omega t+\varphi)$ (into a 1 Ω reference) is $A^{2}/2$ — numerically confirmed: $A=2$ gives 2.0. This "$|x|^2/2$ for a real sinusoid" factor reappears when you compute power from IQ samples (Lesson 1 shows complex samples give $|z|^2$).

### Phase is a *time shift* in disguise 🔥
Delay a sinusoid by $\tau$ seconds: $x(t-\tau)=A\cos(2\pi f t-2\pi f\tau)$. So a delay $\tau$ appears as a phase change

$$\boxed{\Delta\varphi=-2\pi f\,\tau}$$

*Example:* a 250 Hz tone delayed by 2 ms shifts by $2\pi\cdot250\cdot0.002=\pi$ rad = **180°** (peak becomes trough). A 1 kHz tone delayed by 0.25 ms (a quarter of its 1 ms period) shifts by **90°**.
*Meaning:* the same time delay is a **bigger phase shift at higher frequency** — a fact that returns in multipath (Lesson 10) and timing offset (Lesson 13).

### Phase 90° apart = "quadrature"
$\cos(\omega t)$ and $\sin(\omega t)=\cos(\omega t-90^\circ)$ are orthogonal in a precise sense you will meet in P03 (their product averages to zero). That makes them two independent "channels" riding on one frequency.

---

## 2.5 The trig identities you actually use

| Identity | Verified numerically |
|---|---|
| $\sin^{2}a+\cos^{2}a=1$ | ✓ |
| $\cos(a+b)=\cos a\cos b-\sin a\sin b$ | ✓ |
| $\cos a\cos b=\tfrac12[\cos(a-b)+\cos(a+b)]$ | ✓ (0.74960 both sides) |
| $\sin a\sin b=\tfrac12[\cos(a-b)-\cos(a+b)]$ | ✓ |
| $\sin a\cos b=\tfrac12[\sin(a+b)+\sin(a-b)]$ | ✓ |
| $\sin\theta=\cos(\theta-\pi/2)$ | ✓ |

### The product identity = frequency mixing 🔥
Multiply two cosines of frequencies $f_1$ and $f_2$:

$$\cos(2\pi f_1t)\cos(2\pi f_2t)=\tfrac12\cos\bigl(2\pi(f_1-f_2)t\bigr)+\tfrac12\cos\bigl(2\pi(f_1+f_2)t\bigr)$$

**Verified in code:** 1000 Hz × 900 Hz produced spectral peaks at exactly **100 Hz and 1900 Hz**, each with amplitude **0.5**.
*Meaning:* multiplying by a cosine of frequency $f_{LO}$ moves a signal to the **difference** and **sum** frequencies. Receivers use this to shift a 2.4 GHz signal down to baseband; the unwanted sum term is removed by a low-pass filter. *Practice 5 below applies it to 2.4 GHz.* (This is the very step Lesson 1 uses to motivate complex numbers.)

---

## 2.6 The problem real signals cannot solve: ±f ambiguity

Because cosine is even, $\cos(2\pi(-f)t)=\cos(2\pi ft)$ — a tone at **−100 Hz** is *identical* to one at **+100 Hz**. Verified in code: the sampled values match exactly (`allclose → True`). Sine only flips sign, so a sine alone is also ambiguous about the sign of the frequency.

Consequence: a receiver that works with real numbers cannot separate "a signal 100 Hz above the carrier" from "a signal 100 Hz below it" — they fold on top of each other. **Lesson 1's whole point** is that using *two* real numbers together (a complex number) removes this ambiguity. This module's job is to make sure you can recognise the problem when you see it.

---

## 2.7 A sinusoid is a rotating arrow (informal phasors)

Think of an arrow of length $A$ rotating counter-clockwise at $f$ turns per second, starting at angle $\varphi$. Its horizontal shadow is $A\cos(2\pi ft+\varphi)$.

**Adding sinusoids of the *same* frequency = adding arrows** (vector addition), and the result is again a sinusoid of that frequency.

*Verified example:* $3\cos(\omega t)+4\cos(\omega t+90^\circ)$ — arrows of length 3 at 0° and 4 at 90° form a 3-4-5 triangle, so the sum is $5\cos(\omega t+53.13^\circ)$. A least-squares fit of the actual samples returned **R = 5.000, φ = 53.13°** ✓.
*Another:* two arrows of length 2 at 0° and 120° sum to length **2 at 60°** (practice 7).

**Why this matters:** two multipath copies of the same carrier add as arrows; if they point in opposite directions they cancel — destructive interference/fading (Lesson 10).

---

## 2.8 Splitting a sinusoid into two components: the I/Q idea

Use the angle-sum identity on $A\cos(\omega t+\varphi)$:

$$A\cos(\omega t+\varphi)=\underbrace{A\cos\varphi}_{I}\ \cos\omega t\;-\;\underbrace{A\sin\varphi}_{Q}\ \sin\omega t$$

So *any* sinusoid of a given frequency is fully described by **two real numbers**, $I$ (**in-phase**) and $Q$ (**quadrature**), multiplying a cosine and a sine. *Example:* $A=2,\ \varphi=60^\circ\Rightarrow I=1,\ Q=1.732$ (checked). Going the other way, $A=\sqrt{I^{2}+Q^{2}}$ and $\varphi=\operatorname{atan2}(Q,I)$.

*Meaning:* amplitude + phase ⟷ I + Q. Lesson 1 will pack the pair $(I,Q)$ into the single complex number $I+jQ$ — and then every operation (rotate, scale, add) becomes ordinary complex arithmetic.

---

## 2.9 Sampling a sinusoid (first look at discrete time)

Computers hold *samples* taken every $1/f_s$ seconds. Sample $x(t)=\cos(2\pi ft)$ at $t=n/f_s$:

$$x[n]=\cos\!\Bigl(2\pi\frac{f}{f_s}\,n\Bigr)=\cos(\hat\omega n),\qquad \hat\omega=2\pi\frac{f}{f_s}\ \text{(rad/sample)}$$

* $f/f_s$ is **cycles per sample**; $f_s/f$ is **samples per cycle**.
* *Example:* $f=1$ kHz, $f_s=8$ kHz ⇒ 8 samples per cycle, **45° per sample**, sample values $[1,\ 0.707,\ 0,\ -0.707,\ -1,\ -0.707,\ 0,\ 0.707]$ (checked).
* *Example:* 3 kHz at $f_s=12$ kHz ⇒ 4 samples/cycle, 90° (π/2) per sample.

### A warning flag for Lesson 2 (aliasing)
At $f_s=1000$ Hz, a **900 Hz** cosine produces *exactly the same samples* as a **100 Hz** cosine (verified). Likewise a **7 kHz** tone at $f_s=10$ kHz looks like **3 kHz** (verified). Once you sample, different frequencies can become indistinguishable — **Lesson 2 (sampling theorem and aliasing) explains when this happens and how to prevent it.** For now just notice that *frequency in samples is only meaningful modulo one full turn per sample*.

---

## 2.10 Python workout (verified)

```python
import numpy as np

fs = 8000                     # samples per second
f  = 1000                     # tone frequency (Hz)
n  = np.arange(8)
x  = np.cos(2*np.pi*f*n/fs)   # radians! (2*pi*f/fs rad/sample)
print(np.round(x, 4))         # [ 1.  0.7071 0. -0.7071 -1. -0.7071 -0. 0.7071]

# degrees -> radians
print(np.deg2rad(135))        # 2.3562  (= 3*pi/4)

# I/Q components of A*cos(wt+phi)
A, phi = 2.0, np.deg2rad(60)
I, Q = A*np.cos(phi), A*np.sin(phi)
print(I, Q)                   # 1.0  1.732
print(np.hypot(I, Q), np.rad2deg(np.arctan2(Q, I)))  # 2.0  60.0

# mixing creates sum and difference frequencies
fs = 20000; N = 20000; t = np.arange(N)/fs
y = np.cos(2*np.pi*1000*t) * np.cos(2*np.pi*900*t)
Y = np.abs(np.fft.rfft(y))/N*2
f_axis = np.fft.rfftfreq(N, 1/fs)
print(sorted(f_axis[np.argsort(Y)[-2:]]))   # [100.0, 1900.0]

# +f and -f look identical for a real cosine
t2 = np.arange(8)/8
print(np.allclose(np.cos(2*np.pi*t2), np.cos(-2*np.pi*t2)))   # True
```

---

## 2.11 Common mistakes ⚠️
1. **Degrees into `np.cos`.** Always radians.
2. **Forgetting the $2\pi$** when going from $f$ (Hz) to $\omega$ (rad/s) or to rad/sample.
3. **Confusing frequency $f$ with angular frequency $\omega$** — a formula with $\omega t$ vs $2\pi f t$ must use matching quantities.
4. **Sign of phase vs delay:** a delay *subtracts* phase ($-2\pi f\tau$); a time advance adds it.
5. **Treating cos and sin as interchangeable.** They are 90° apart; swapping them rotates your constellation (Lessons 8, 13).
6. **Using `arctan(Q/I)` instead of `arctan2(Q, I)`** — plain arctan loses the quadrant (and divides by zero at I = 0).
7. **Mixing "peak" and "RMS" amplitude.** Average power is $A^{2}/2$ for peak amplitude $A$.

---

## 2.12 Practice problems (answers at the end)

1. Convert 135° to radians and 5π/6 to degrees.
2. Evaluate cos 60°, sin 150°, cos 120° (no calculator).
3. For $x(t)=3\cos(2\pi\cdot50\,t+\pi/3)$ find amplitude, frequency, period, angular frequency, phase in degrees, and average power.
4. A 250 Hz tone is delayed by 2 ms. What phase shift (magnitude, in degrees) does that produce?
5. A 2.4 GHz carrier is multiplied by a 2.38 GHz oscillator. Which two frequencies appear? Which would a low-pass filter keep?
6. Write $5\cos(\omega t+36.87^\circ)$ as $I\cos\omega t-Q\sin\omega t$.
7. Add $2\cos\omega t+2\cos(\omega t+120^\circ)$ using arrows. Give amplitude and phase of the result.
8. A 3 kHz tone is sampled at 12 kHz. How many samples per cycle and how many radians per sample?
9. In one sentence: why can't a real cosine tell +f from −f?
10. A 7 kHz tone is sampled at 10 kHz. Which lower frequency does it look like?
11. *Think:* two multipath copies of a 1 GHz signal arrive 0.5 ns apart. How large is the phase difference between them? What does that suggest about the sum? (Use $\Delta\varphi=2\pi f\tau$.)

## 2.13 Active recall
1. State $\sin^2+\cos^2=1$ and give its unit-circle meaning. 2. What does it mean that cosine is even? 3. Write $x(t)$ for a sinusoid and name every symbol with its unit. 4. How does a delay change phase? 5. Write the product-to-sum identity for $\cos a\cos b$. 6. What is "quadrature"? 7. Write $A\cos(\omega t+\varphi)$ in terms of $I$ and $Q$. 8. What is $\hat\omega$ and how do you compute it from $f$ and $f_s$? 9. Why must you use `arctan2`?

## 2.14 Self-check — ready for P03 if you can…
- [ ] sketch the unit circle with cos and sin and read off 0°, 30°, 45°, 60°, 90°;
- [ ] convert degrees ↔ radians without hesitation;
- [ ] explain "phase = time shift" with a number;
- [ ] derive that mixing produces sum and difference frequencies;
- [ ] decompose a sinusoid into $I$ and $Q$.

---

## ✅ Answer key

1. 135° = **3π/4 ≈ 2.356 rad**; 5π/6 = **150°**. 2. **0.5**, **0.5**, **−0.5**. 3. A = **3**, f = **50 Hz**, T = **20 ms**, ω = **314.16 rad/s**, φ = **60°**, power = A²/2 = **4.5**. 4. Δφ = 2π·250·0.002 = π ⇒ **180°** (a delay, so −180°, equivalent). 5. **20 MHz (difference)** and **4.78 GHz (sum)**; the low-pass keeps **20 MHz**. 6. I = 5 cos 36.87° = **4.0**, Q = 5 sin 36.87° = **3.0** (a 3-4-5 triangle again). 7. Arrows 2∠0° + 2∠120° = 1 + j1.732 ⇒ amplitude **2**, phase **60°**, i.e. $2\cos(\omega t+60^\circ)$. 8. **4 samples/cycle, π/2 rad/sample (90°)**. 9. Because cos(−θ) = cos θ, a negative-frequency tone produces the same real signal as the positive one. 10. **3 kHz** (aliasing: 10 − 7). 11. Δφ = 2π·10⁹·0.5×10⁻⁹ = π rad = **180°** ⇒ the two arrows point in opposite directions and (if equal in size) **cancel** — a deep fade.

---

*Next: **P03 — Algebra, series and calculus essentials** (summation notation, geometric series, derivatives as rates, integrals as accumulation, Taylor series, orthogonality of sinusoids).*
