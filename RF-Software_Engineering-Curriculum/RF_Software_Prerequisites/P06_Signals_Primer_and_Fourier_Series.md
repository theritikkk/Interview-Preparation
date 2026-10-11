# P06 — Signals Primer and Fourier Series: Signals, Systems, Spectrum and Bandwidth

*Prerequisite module 6 · ⏱ about 6–7 hours · assumes P01–P05 (especially P02 sinusoids and P03 orthogonality) · every number and output below was computed and checked.*

> **Where this unlocks the curriculum.** Stage 1 (Lessons 2–5) is *Signals & Systems*. Lesson 2 talks about a spectrum $X(f)$ and sampling copies of it; Lesson 3 builds the DTFT/DFT/FFT ("how much of each frequency is present?"); Lesson 4 defines LTI systems and convolution; Lesson 9 shapes pulses to control bandwidth. This module gives you the **vocabulary and the picture** behind all four: what a signal and a system are, how a periodic signal is a sum of sinusoids (Fourier series), what "spectrum" and "bandwidth" mean, and why sharp edges need wide bandwidth. It deliberately stops *before* the Fourier transform, sampling theorem, DFT and convolution — those are the lessons.

---

## 6.1 What is a signal?

A **signal** is information carried by a quantity that varies — usually with time. Examples: the voltage from a microphone, the electric field at an antenna, a stream of IQ samples from an SDR.

| Classification | Types | Notes |
|---|---|---|
| **Time** | **continuous-time** $x(t)$ / **discrete-time** $x[n]$ | $x[n]$ is a list of samples taken every $T_s=1/f_s$ (P02 §2.9) |
| **Value** | **analog** (any value) / **digital** (quantised to a few levels, e.g. 16-bit) | ADCs do both: sample *and* quantise |
| **Predictability** | **deterministic** (a formula) / **random** (noise; described statistically, P05) | real signals = deterministic wanted part + random noise |
| **Number type** | **real** / **complex** | IQ samples are complex — Lesson 1 shows why packing $I+jQ$ is useful |
| **Repetition** | **periodic** ($x(t+T)=x(t)$) / **aperiodic** | periodic signals get the Fourier *series* (this module); aperiodic ones the Fourier *transform* (Lesson 2–3) |

**Even and odd signals.** $x$ is **even** if $x(-t)=x(t)$ (like cosine), **odd** if $x(-t)=-x(t)$ (like sine). Any signal splits uniquely as $x=x_e+x_o$ with $x_e=\tfrac12[x(t)+x(-t)]$ and $x_o=\tfrac12[x(t)-x(-t)]$ — *verified* on $e^{-t}u(t)$. (This is the root of the ±f story in P02.)

---

## 6.2 Building blocks every signal is made of

| Signal | Definition | Why it matters |
|---|---|---|
| **Sinusoid** | $A\cos(2\pi ft+\varphi)$ | the "atom" of frequency analysis (P02) |
| **Exponential** | $e^{at}$ (decaying if $a<0$) | filter responses, $e^{j\theta}$ (Lesson 1) |
| **Rectangular pulse** | height $A$ for $0\le t<\tau$, else 0 | a bit/symbol; defines bandwidth ≈ $1/\tau$ (§6.8) |
| **Unit step** $u[n]$ | 0 for $n<0$, 1 for $n\ge0$ | "switch on" |
| **Unit impulse** $\delta[n]$ | 1 at $n=0$, 0 elsewhere | the test signal for systems (Lesson 4) |

**Any sequence is a sum of scaled, shifted impulses:** $x[n]=\sum_k x[k]\,\delta[n-k]$. *Verified:* $[2,-1,3]=2\,\delta[n]-1\,\delta[n-1]+3\,\delta[n-2]$. *Meaning:* if you know what a system does to a single impulse, you know what it does to every signal — **the idea behind convolution (Lesson 4)**.

**Signal operations**
* **Delay:** $x(t-\tau)$ shifts the signal right (adds phase $-2\pi f\tau$ to a tone, P02).
* **Scaling:** $A\,x(t)$ changes level (a gain of $20\log_{10}A$ dB, P01).
* **Time compression:** $x(2t)$ plays the signal twice as fast ⇒ every frequency doubles. *(Faster in time = wider in frequency; see §6.8.)*

---

## 6.3 Energy and power of a signal 🔥

$$E=\int_{-\infty}^{\infty}|x(t)|^{2}dt\quad\Bigl(\text{discrete: }\sum_n|x[n]|^{2}\Bigr),\qquad P=\lim_{T\to\infty}\frac1{2T}\int_{-T}^{T}|x(t)|^{2}dt$$

* **Energy signals** have finite energy (a single pulse). **Power signals** go on forever and have finite average power (a sinusoid, noise).
* *Verified:* a rectangular pulse of height 2 and width 3 s has $E=2^{2}\times3=\mathbf{12}$; a tone $3\cos(\cdot)$ has $P=A^{2}/2=\mathbf{4.5}$ (same $A^{2}/2$ as P02/P03). Noise's average power is its variance (P05). Convert power ratios to dB with P01.

---

## 6.4 Systems: what a filter, a channel and an amplifier have in common 🔥

A **system** turns an input signal into an output signal: $y=\mathcal S\{x\}$. Radios are chains of systems: antenna → amplifier → filter → mixer → ADC → your code.

Two properties make a system *analysable*:
* **Linear:** scaling and adding inputs scales and adds outputs — $\mathcal S\{ax_1+bx_2\}=a\mathcal S\{x_1\}+b\mathcal S\{x_2\}$.
* **Time-invariant:** a delayed input just produces a delayed output.

**Verified by numerical tests** (random inputs, superposition and shift tests):

| System | Linear? | Time-invariant? |
|---|---|---|
| $y=3x$ | ✅ | ✅ |
| $y=x^{2}$ | ❌ | ✅ |
| $y[n]=x[n]+x[n-1]$ (2-point sum) | ✅ | ✅ |
| $y[n]=n\,x[n]$ | ✅ | ❌ |
| $y[n]=x[n]+1$ | ❌ (adds a constant: $x=0\Rightarrow y=1$) | ✅ |

Systems that are both are called **LTI** — filters and ideal channels. Lesson 4 shows an LTI system is *completely* described by its response to an impulse.

### The central fact about LTI systems 🔥
**Feed a sinusoid into an LTI system and the output is a sinusoid of the *same frequency*, only scaled and delayed.** The scale factor depends on frequency — that function is the system's **frequency response**.

*Verified with a 3-point moving average* $y[n]=\tfrac13\bigl(x[n]+x[n-1]+x[n-2]\bigr)$:

| Input frequency (cycles/sample) | Output amplitude (measured) | Theory $\lvert(1+2\cos2\pi f)/3\rvert$ |
|---|---|---|
| 0.05 (slow) | **0.9674** | 0.9674 |
| 0.40 (fast) | **0.2060** | 0.2060 |

Slow tones pass almost unchanged; fast tones are strongly attenuated — a **low-pass filter**. *Meaning:* once you split a signal into sinusoids (Fourier!), a filter's effect is just *multiplying each by a number* — no wonder frequency-domain thinking dominates radio.

**Nonlinear systems create new frequencies.** Squaring $\cos(2\pi ft)$ gives $\tfrac12+\tfrac12\cos(4\pi ft)$ (P02's identity): a **DC term and a tone at $2f$** that were not in the input. This is how amplifier distortion and mixers behave (Lesson 42 on PA nonlinearity); a *linear* system can never add a new frequency.

---

## 6.5 The Fourier series: a periodic signal is a sum of harmonics 🔥

**Claim.** A periodic signal with period $T$ (fundamental $f_0=1/T$) can be written as a DC term plus sinusoids at the **harmonics** $f_0,2f_0,3f_0,\dots$

$$x(t)=a_0+\sum_{k=1}^{\infty}\Bigl[a_k\cos(2\pi kf_0t)+b_k\sin(2\pi kf_0t)\Bigr]$$

**How to find the coefficients — straight from P03's orthogonality** (multiply by the harmonic you want, integrate over one period, and every other harmonic drops out):

$$a_0=\frac1T\!\int_0^Tx\,dt,\qquad a_k=\frac2T\!\int_0^Tx\cos(2\pi kf_0t)\,dt,\qquad b_k=\frac2T\!\int_0^Tx\sin(2\pi kf_0t)\,dt$$

*Meaning:* $a_0$ is the average (DC). The amplitude of the $k$-th harmonic is $\sqrt{a_k^{2}+b_k^{2}}$. This is exactly the projection onto orthogonal vectors from P04 §4.4.

### Worked example: the ±1 square wave
*Verified by numerically evaluating the integrals:*

| $k$ | $a_k$ | $b_k$ | theory $b_k=\dfrac4{\pi k}$ (odd $k$) |
|---|---|---|---|
| 0 | 0 (zero mean) | – | – |
| 1 | 0 | **1.2732** | 1.2732 |
| 2 | 0 | 0 | 0 |
| 3 | 0 | **0.4244** | 0.4244 |
| 4 | 0 | 0 | 0 |
| 5 | 0 | **0.2546** | 0.2546 |
| 7 | 0 | **0.1819** | 0.1819 |

$$\boxed{x_{\text{square}}(t)=\frac4\pi\Bigl[\sin\omega_0t+\tfrac13\sin3\omega_0t+\tfrac15\sin5\omega_0t+\cdots\Bigr]}$$
**Only odd harmonics, falling as $1/k$.** (Symmetry explains it: the wave is odd ⇒ only sines; half-wave symmetric ⇒ only odd harmonics.)

Line spectrum (amplitude of each harmonic):
```
 amplitude
 1.27 |█
      |█
 0.42 |█     █
 0.25 |█     █     █
 0.18 |█     █     █     █        (even harmonics are exactly 0)
      +-----------------------------------▶ frequency
       f0   3f0   5f0   7f0
```

### Building the square wave from sinusoids — and the Gibbs phenomenon
*Verified:* partial sums using harmonics up to $K$:

| Harmonics up to | RMS error vs ideal square | Peak value |
|---|---|---|
| 1 | 0.4352 | 1.2732 |
| 3 | 0.3152 | 1.2004 |
| 9 | 0.2010 | 1.1823 |
| 19 | 0.1423 | 1.1798 |
| 99 | **0.0637** | **1.1790** |

The **RMS error keeps falling**, but the **overshoot at the edges does not disappear** — it settles near **1.179** (≈ 9 % of the 2-unit jump). This is the **Gibbs phenomenon**: a harmless "ringing" next to every discontinuity that gets narrower, not smaller. *(It reappears as ringing in sharp filters and pulses, Lessons 4/9.)*

---

## 6.6 Spectrum and power in the frequency domain 🔥

The **spectrum** is the plot of amplitude (or power) against frequency. A periodic signal has a **line spectrum** — power only at multiples of $f_0$.

**Power of harmonic $k$:** $(a_k^{2}+b_k^{2})/2$, and the powers **add up to the total power** — *Parseval's relation*:

$$P_{\text{total}}=a_0^{2}+\sum_{k\ge1}\frac{a_k^{2}+b_k^{2}}2$$

*Verified for the square wave* (total power $\mathbf{1.0}$ since $|x|=1$):

| Harmonics included | Fraction of power |
|---|---|
| fundamental only | **81.06 %** |
| up to 3rd | 90.06 % |
| up to 5th | 93.31 % |
| up to 9th | 95.96 % |
| up to 13th | 97.11 % |
| **99 % reached at** | **harmonic 41** |

*Meaning:* the fundamental already carries 81 %, but capturing **99 %** of the power of a *sharp-edged* wave needs harmonics out to **41 $f_0$** — a 41× wider band. **Sharp transitions demand wide bandwidth.** That is precisely why real systems **shape their pulses** (smooth the edges) before transmitting — Lesson 9.

---

## 6.7 Bandwidth: how wide is a signal in frequency? 🔥

**Bandwidth** is the width, in Hz, of the frequency range that contains the signal's significant energy. "Significant" needs a definition — different ones are used:

| Definition | Meaning |
|---|---|
| **Absolute** | the band outside of which the spectrum is exactly zero (only ideal signals) |
| **3 dB (half-power)** | the band where power is within 3 dB of the peak (P01: 3 dB = ½) |
| **Null-to-null** | distance between the first zeros of the spectrum around the main lobe |
| **Occupied (x %)** | the band containing, say, 99 % of the power (the square wave needs 41 $f_0$) |

Always state **which definition** you use. *(Careful with "single-sided vs double-sided": a real signal's spectrum has mirror-image halves at ±f, P02 §2.6; a complex IQ signal's bandwidth is measured over its whole range.)*

---

## 6.8 Time ↔ frequency duality: short in time = wide in frequency 🔥

**Periodic pulse train (duty cycle $d=\tau/T$).** *Verified Fourier amplitudes for $d=0.25$* (matching the formula $|c_k|=\dfrac{2}{\pi k}|\sin(\pi kd)|$):

| $k$ | 0 (DC) | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
|---|---|---|---|---|---|---|---|---|---|
| amplitude | 0.2500 | **0.4502** | 0.3183 | 0.1501 | **0.0000** | 0.0900 | 0.1061 | 0.0643 | **0.0000** |

The envelope is a **sinc** ($\sin x/x$) shape, with zeros at $k=1/d=4,8,\dots$, i.e. at frequency $1/\tau$.

**Single rectangular pulse of width $\tau$** (*verified by FFT:* $\tau=1\,\mu$s gives the first spectral null at **1.00 MHz** $=1/\tau$, spectrum value 1.0 at DC):

$$\text{first null at }f=\frac1\tau,\qquad\text{null-to-null main-lobe bandwidth}=\frac2\tau,\qquad B\sim\frac1\tau$$

| Pulse width | Main-lobe null-to-null |
|---|---|
| 1 µs | **2 MHz** |
| 0.1 µs | **20 MHz** |

*Meaning:* **making a pulse 10× shorter makes its spectrum 10× wider.** Faster data ⇒ shorter symbols ⇒ more bandwidth — the basic reason higher data rates need wider channels. Likewise, to separate tones $\Delta f$ apart you must observe for at least $\sim1/\Delta f$: for $\Delta f=15$ kHz that is $1/15\,\text{kHz}=66.7\,\mu$s of signal — a hint of how OFDM subcarriers are spaced (Lesson 12) and of the DFT's resolution $f_s/N$ (Lesson 3).

---

## 6.9 From Fourier series to what comes next (the roadmap)

| If the signal is… | The tool is… | Taught in |
|---|---|---|
| periodic, continuous time | **Fourier series** (this module) | P06 |
| non-periodic, continuous time | Fourier transform $X(f)$ (the line spectrum becomes continuous as $T\to\infty$) | Lesson 2 (uses it) |
| **sampled** | sampling theorem, aliasing; spectrum **copies** every $f_s$ | Lesson 2 |
| finite block of samples | **DFT / FFT** — the series coefficients, computed fast | Lesson 3 |
| passing through a filter/channel | multiply spectrum by frequency response; convolution in time | Lesson 4 |

One more connection for later: since $\cos x=\tfrac12(e^{jx}+e^{-jx})$ (Euler, Lesson 1), every real sinusoid in the spectrum appears as **two lines, at $+f$ and $-f$**. The DFT bins you will plot are these complex-exponential coefficients.

---

## 6.10 Python workout (verified)

```python
import numpy as np

T = 1.0; N = 400000
t = np.linspace(0, T, N, endpoint=False); dt = T/N; f0 = 1/T
sq = np.where(t < 0.5, 1.0, -1.0)                  # +-1 square wave

# Fourier coefficients by projection (orthogonality, P03)
def coeffs(x, k):
    a = 2/T*np.sum(x*np.cos(2*np.pi*k*f0*t))*dt
    b = 2/T*np.sum(x*np.sin(2*np.pi*k*f0*t))*dt
    return a, b
for k in (1, 2, 3, 5):
    print(k, np.round(coeffs(sq, k), 4))           # b1=1.2732, b3=0.4244, b5=0.2546

# partial sum, Gibbs overshoot
def partial(K):
    return sum(4/(np.pi*k)*np.sin(2*np.pi*k*f0*t) for k in range(1, K+1, 2))
print(partial(99).max())                           # ~1.179  (does not go to 1)

# Parseval: power in the first few harmonics
p = np.cumsum([(4/(np.pi*k))**2/2 for k in (1, 3, 5)])
print(p)                                            # [0.8106 0.9006 0.9331]

# LTI: sinusoid in -> sinusoid out (3-point moving average)
n = np.arange(2000)
for cyc in (0.05, 0.4):
    y = np.convolve(np.cos(2*np.pi*cyc*n), np.ones(3)/3)[:len(n)]
    print(cyc, np.abs(y[100:]).max(), abs((1 + 2*np.cos(2*np.pi*cyc))/3))

# pulse width <-> bandwidth: first null at 1/tau
fs = 100e6; tau = 1e-6; Nn = 100000
x = np.zeros(Nn); x[:int(tau*fs)] = 1
X = np.abs(np.fft.rfft(x))/(tau*fs); f = np.fft.rfftfreq(Nn, 1/fs)
print(f[np.where(X[:2000] < 1e-3)[0][0]]/1e6, "MHz")   # 1.0 MHz
```

---

## 6.11 Common mistakes ⚠️
1. **Using $1/T$ instead of $2/T$** in $a_k,b_k$ (only $a_0$ gets $1/T$).
2. **Forgetting the DC term** $a_0$ (a signal that is not zero-mean has one).
3. **Treating Gibbs ringing as a bug.** More harmonics shrink the *width* of the overshoot, not its height.
4. **Mixing up period $T$, fundamental $f_0=1/T$ and harmonic number $k$.**
5. **Quoting "bandwidth" without a definition** (3 dB? null-to-null? 99 %?).
6. **Assuming any system with a gain is linear.** $y=x^{2}$ and $y=x+1$ are not; they create new frequencies / offsets.
7. **Confusing time-invariant with constant:** time-invariant means the *rule* doesn't change over time, not that the output is constant.
8. **Integrating over a non-integer number of periods** when computing coefficients (leakage — P03, Lesson 3).
9. **Single-sided vs double-sided spectra** — remember real signals have mirrored $\pm f$ halves.

---

## 6.12 Practice problems (answers at the end)

1. Classify each as an energy or power signal and give its value: (a) $\cos(2\pi50t)$; (b) $e^{-t}u(t)$; (c) a rectangular pulse of height 2 and width 3 s.
2. Write $x=[2,-1,3]$ as a sum of shifted, scaled unit impulses.
3. Is $y[n]=x[n]+1$ linear? time-invariant? Give a one-line test for each.
4. Is $y[n]=x[-n]$ (time reversal) time-invariant? (Shift the input and compare.)
5. A 3-point moving average receives a tone at 0.25 cycles/sample. What is its gain?
6. A ±1 square wave has period 1 ms. List the frequencies and amplitudes of its first three non-zero harmonics.
7. What fraction of that square wave's power is in its fundamental? How many harmonics are needed for about 99 %?
8. A rectangular pulse has width 0.5 µs. Find the first spectral null and the null-to-null bandwidth.
9. A pulse train with 25 % duty cycle and period 1 ms: which harmonics are exactly zero, and at what frequencies?
10. A signal $x(t)=\cos(2\pi ft)$ passes through $y=x^{2}$. Which frequencies appear in $y$?
11. *Think:* you add 1000 harmonics to a square-wave series. Does the overshoot at the edge vanish? What does shrink?
12. *Think:* why does sending bits twice as fast (symbols half as long) need roughly twice the bandwidth?

## 6.13 Active recall
1. List five classifications of signals. 2. What is a unit impulse and why is it special for systems? 3. Define linear and time-invariant. 4. What happens to a sinusoid passing through an LTI system? 5. Why can a nonlinear system create new frequencies but a linear one cannot? 6. Write the Fourier series and the formulas for $a_0,a_k,b_k$. 7. Why do square waves have only odd harmonics? 8. State Parseval's relation in words. 9. What is Gibbs' phenomenon? 10. State the pulse-width ↔ bandwidth relation. 11. Name three definitions of bandwidth.

## 6.14 Self-check — ready for Lesson 2 (after P07–P13) if you can…
- [ ] tell whether a system is linear/time-invariant with a quick test;
- [ ] explain why sinusoids are the "natural" signals for LTI systems;
- [ ] compute Fourier-series coefficients by orthogonality and read a line spectrum;
- [ ] explain why sharp edges need bandwidth and why short pulses are wide in frequency;
- [ ] state three bandwidth definitions and use $B\sim1/\tau$.

---

## ✅ Answer key

1. (a) **power**, $P=0.5$; (b) **energy**, $E=\int_0^\infty e^{-2t}dt=\mathbf{0.5}$; (c) **energy**, $E=2^{2}\times3=\mathbf{12}$. 2. $2\delta[n]-\delta[n-1]+3\delta[n-2]$. 3. **Not linear:** input 0 gives output 1 (a linear system maps 0 to 0). **Time-invariant:** yes (shifting the input shifts the output; the "+1" doesn't depend on $n$). 4. **No — time-varying.** Shifting the input by $k$ gives $x[-n-k]$, but the shifted output is $x[-n+k]$; they differ. 5. $|(1+2\cos(\pi/2))/3|=\mathbf{1/3}$. 6. **1 kHz: 1.273; 3 kHz: 0.424; 5 kHz: 0.255** (even harmonics zero). 7. **81.06 %**; ≈ **41 harmonics** for 99 %. 8. First null $=1/0.5\,\mu\text{s}=\mathbf{2\ MHz}$; null-to-null $=\mathbf{4\ MHz}$. 9. Harmonics with $k=4,8,12,\dots$ ⇒ **4 kHz, 8 kHz, …** (where $\sin(\pi kd)=0$ for $d=0.25$). 10. $\cos^{2}=\tfrac12+\tfrac12\cos(4\pi ft)$ ⇒ **DC and $2f$** (no $f$ component). 11. **No, the overshoot stays ≈ 1.179**; the *width* of the ringing and the RMS error shrink. 12. Pulse width halves ⇒ first null $1/\tau$ doubles ⇒ spectrum ≈ twice as wide.

---

*Next: **P07 — Physics and electronics for software engineers** (voltage, power, impedance in one page; the RC filter; EM waves and antennas; path loss; thermal noise; the block diagram of a radio from antenna to ADC).*
