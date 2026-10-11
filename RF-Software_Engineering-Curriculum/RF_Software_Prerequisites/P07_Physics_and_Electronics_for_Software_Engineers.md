# P07 — Physics and Electronics for Software Engineers: From Antenna to IQ Samples

*Prerequisite module 7 · ⏱ about 6–7 hours · assumes P01–P06 (dB, sinusoids, mixing, noise) · every number and output below was computed and checked, including a simulated quadrature receiver.*

> **Where this unlocks the curriculum.** A scan of the lessons shows antenna/array ideas dominating Lessons 14, 46 and 47; propagation/path loss in 10, 14, 18, 30; **zero-IF mixers, LO leakage, DC offset and IQ imbalance** as the whole subject of Lesson 15; **oscillator error and carrier-frequency offset** in 13; **ADC bits** in 25; **PA nonlinearity, PAPR and regulation** in 42. The curriculum is about *software*, but it assumes you know what the hardware in front of your code does and why it misbehaves. This module is the "hardware, explained for programmers" layer — concepts and numbers, **not** circuit design.

---

## 7.1 What you are *not* going to learn here
No transistor biasing, no matching networks, no PCB layout. The aim is a correct **mental model** and the handful of equations that let you read a datasheet and a log file.

---

## 7.2 Electricity in one page

* **Voltage $V$** (volts): electrical "pressure". **Current $I$** (amps): flow of charge. **Power $P$** (watts): rate of energy use.
* $P=VI$; **Ohm's law** $V=IR$; so $P=V^{2}/R=I^{2}R$.
* RF systems are conventionally built around a **50 Ω** impedance, so voltage and power convert simply: $V_{\text{rms}}=\sqrt{PR}$.

**Verified signal levels in 50 Ω** (rms voltage):

| Level | Power | $V_{\text{rms}}$ |
|---|---|---|
| 0 dBm | 1 mW | 0.224 V |
| −30 dBm | 1 µW | 7.07 mV |
| **−90 dBm** | 1 pW | **7.07 µV** |

*Meaning:* a radio signal that is "strong enough to decode" can be **a few microvolts**. An ADC cannot digitise microvolts directly — the signal must be **amplified** first (§7.8). Peak voltage of a sinusoid is $\sqrt2\times V_{\text{rms}}$ (P02: average power $A^{2}/2$).

---

## 7.3 Resistors, capacitors, inductors and the RC filter

| Part | Opposes… | Frequency behaviour | One-line intuition |
|---|---|---|---|
| **Resistor** $R$ | current, equally at all frequencies | flat | friction |
| **Capacitor** $C$ | *change in voltage* | reactance $X_C=\dfrac1{2\pi fC}$ — **blocks low frequencies, passes high** | a tiny reservoir |
| **Inductor** $L$ | *change in current* | reactance $X_L=2\pi fL$ — **passes low, blocks high** | a flywheel |

**Impedance** $Z$ is the generalisation of resistance to AC (a complex quantity, P02/Lesson 1); reactances are its imaginary parts.

### The RC low-pass filter 🔥 (the prototype analog filter)
Resistor in series, capacitor to ground: a divider whose capacitor shorts out high frequencies.

$$|H(f)|=\frac1{\sqrt{1+(f/f_c)^{2}}},\qquad f_c=\frac1{2\pi RC},\qquad\tau=RC$$

*Verified* for $R=1\,\text{k}\Omega,\ C=159.15\,\text{nF}$ ⇒ $f_c=1.000$ kHz, $\tau=159\,\mu$s:

| $f$ | $\lvert H\rvert$ | dB |
|---|---|---|
| 100 Hz | 0.995 | −0.04 |
| **1 kHz ($f_c$)** | 0.707 | **−3.01** |
| 10 kHz | 0.0995 | −20.04 |
| 100 kHz | 0.0100 | −40.00 |

*Meaning:* the **cutoff is the −3 dB point** (P01: half power); beyond it the gain falls **20 dB per decade**. In time, a step input charges the capacitor to **63.2 % after $\tau$** and **95 % after $3\tau$** (P03: $1-e^{-t/\tau}$). This single circuit is what an **anti-aliasing filter** (Lesson 2) and a mixer's baseband low-pass (§7.8) are built from; digital filters (Lessons 4–5) imitate it in software.

---

## 7.4 Electromagnetic waves and antennas 🔥

* A radio wave is coupled oscillating **electric and magnetic fields** travelling at $c$ (P01: $\lambda=c/f$). In free space they are perpendicular to each other and to the direction of travel.
* An **antenna** converts between a guided current and a radiating wave. Antennas are efficient when their size is a sizeable fraction of the wavelength; the classic **half-wave dipole** has length $\lambda/2$, a **quarter-wave monopole** $\lambda/4$.

*Verified sizes:*

| Frequency | $\lambda$ | $\lambda/2$ | $\lambda/4$ |
|---|---|---|---|
| 900 MHz | 33.3 cm | 16.7 cm | 8.3 cm |
| 2.4 GHz | 12.5 cm | 6.25 cm | 3.1 cm |
| 3.5 GHz | 8.57 cm | 4.28 cm | 2.1 cm |
| **28 GHz** | **1.07 cm** | **0.535 cm** | 0.27 cm |

* **Isotropic antenna** = a hypothetical antenna radiating equally in all directions; **gain in dBi** compares a real antenna's strongest direction to it (P01). Gain is *redistribution*, not amplification: a high-gain antenna concentrates power into a narrower **beam**.
* **Polarisation** = the orientation of the electric field (vertical/horizontal/circular). Transmit and receive antennas should match.
* **Antenna arrays** (Lessons 14, 46, 47): several elements spaced about **$\lambda/2$** apart. Combining their signals with chosen **complex weights** (P04 dot products!) forms beams and nulls — the physical basis of MIMO and beamforming. Spacing at 3.5 GHz ≈ **4.3 cm**; at 28 GHz ≈ **5.4 mm** — which is why mmWave phones can fit many elements.

---

## 7.5 Spectrum: bands, licences and why transmitting is regulated

Radio spectrum is a **shared, finite, regulated resource** (the college notes, Ch. 1/3, built cellular reuse on exactly this). Typical regions you will see:

| Band | Typical use |
|---|---|
| 700–900 MHz | long-range cellular (good building penetration) |
| 1.8–2.6 GHz | the workhorse cellular bands |
| **2.4 GHz and 5 GHz (ISM / unlicensed)** | Wi-Fi, Bluetooth — *anyone may transmit within power and mask limits* |
| 3.3–3.8 GHz | 5G mid-band |
| 24–52 GHz (e.g. 28 GHz) | 5G mmWave |

* **Licensed** bands are held by operators; **unlicensed (ISM)** bands have rules about power and out-of-band emissions.
* **Practical rule for learners:** *receiving* with an RTL-SDR is fine; **transmitting** requires a legal frequency, power and spectral mask. Lesson 42's own title says transmission "is legally regulated" — treat that as a hard constraint, not a footnote.

---

## 7.6 Propagation and path loss 🔥

**Free-space path loss (FSPL)** — spreading of power over a growing sphere between two isotropic antennas:

$$\text{FSPL}=\Bigl(\frac{4\pi d}{\lambda}\Bigr)^{2}\ \Longrightarrow\ \text{FSPL}_{\text{dB}}=20\log_{10}d_{\text{km}}+20\log_{10}f_{\text{MHz}}+32.44$$

*Verified:*

| Case | FSPL |
|---|---|
| 2 GHz, 1 km | **98.46 dB** (exact formula 98.47) |
| 900 MHz, 1 km | 91.52 dB |
| 3.5 GHz, 1 km | 103.32 dB |
| 28 GHz, 1 km | **121.38 dB** |
| doubling distance | **+6.02 dB** |
| doubling frequency | **+6.02 dB** |

*Meaning:* every **doubling of distance costs 6 dB** (an *inverse-square* law), and higher frequencies lose more at the same distance — a main reason mmWave has short range. The 98.5 dB used in P01's link budget came from this formula: $23+2-98.46+0-1=\mathbf{-74.46}$ dBm received.

**Real environments are worse than free space.** Obstacles add shadowing; multipath adds fading (college notes Ch. 6; Lesson 10). A common empirical model replaces the 20 with $10n$: $\text{PL}\propto10n\log_{10}d$, with **path-loss exponent $n$ ≈ 2 in free space and 3–5 in urban areas**. *Verified:* with $n=3.5$, going from 100 m to 1 km adds **35 dB** (free space would add only 20 dB).

**Doppler shift** $f_d=\dfrac{v}{c}f_c$ (college notes Ch. 6): *verified* 120 km/h at 2 GHz ⇒ **222 Hz**; 350 km/h at 3.5 GHz ⇒ **1.14 kHz**. Fast motion at high frequency makes the channel change quickly (Lessons 10, 13).

---

## 7.7 Noise, noise figure and sensitivity 🔥

**Thermal noise** (P01 §1.5, P05 §5.7): $N=kTB$ ⇒ **−174 dBm/Hz** at 290 K (computed −173.98).

**Noise figure (NF)** measures how much *worse* the SNR gets passing through a device: $\text{NF}=\text{SNR}_{\text{in}}/\text{SNR}_{\text{out}}$ in dB. A perfect device has 0 dB. NF is added to the noise floor.

**Receiver sensitivity** (minimum usable signal):

$$P_{\min}=-174+10\log_{10}B+\text{NF}+\text{SNR}_{\min}\quad[\text{dBm}]$$

*Verified:* $B=20$ MHz, NF = 5 dB, required SNR = 10 dB ⇒ $-174+73.01+5+10=\mathbf{-85.99}\approx\mathbf{-86\ dBm}$.

### Why the **first** amplifier matters most (Friis cascade) 🔥
For stages with noise factors $F_i$ (linear) and gains $G_i$ (linear):

$$F_{\text{total}}=F_1+\frac{F_2-1}{G_1}+\frac{F_3-1}{G_1G_2}+\cdots$$

*Verified:* an **LNA** (low-noise amplifier) with NF 1 dB and gain 20 dB, followed by a stage with NF 10 dB: $F=1.259+9/100=1.349$ ⇒ **1.30 dB** total. Without the LNA the system NF is **10 dB**. *Meaning:* **gain early in the chain makes the noise of later stages negligible** — so receivers put a low-noise amplifier right behind the antenna.

---

## 7.8 The receiver chain: from antenna to IQ samples 🔥📐

```
                                        ┌──────── LO (local oscillator, f_LO) ────────┐
                                        │  cos(2π f_LO t)               −sin(2π f_LO t)│
 antenna → band-select → LNA → ──────▶ [ mixer I ] ─▶ LPF ─▶┐                          │
           filter      (low-noise  │                          ├▶ VGA/AGC ▶ ADC ▶ I[n]   │
                        amplifier) └──▶ [ mixer Q ] ─▶ LPF ─▶┘            ADC ▶ Q[n]    │
                                                      (analog baseband low-pass)         ▼
                                                                                  your software: z[n] = I[n] + j Q[n]
```
This is the **direct-conversion (zero-IF) receiver** assumed by Lesson 15 and used by most SDRs.

### Quadrature down-conversion — derived with P02's product identity 🔥
Let the received RF be $r(t)=A\cos\bigl(2\pi(f_c+\delta)t+\varphi\bigr)$, i.e. a signal $\delta$ above the LO at $f_c$.

* **I branch:** multiply by $2\cos(2\pi f_ct)$. By $2\cos a\cos b=\cos(a-b)+\cos(a+b)$:
 $A\cos(2\pi\delta t+\varphi)+A\cos\bigl(2\pi(2f_c+\delta)t+\varphi\bigr)$ → the low-pass filter removes the $2f_c$ term ⇒ $I(t)=A\cos(2\pi\delta t+\varphi)$.
* **Q branch:** multiply by $-2\sin(2\pi f_ct)$; the same steps give $Q(t)=A\sin(2\pi\delta t+\varphi)$.

$$z(t)=I+jQ=A\,e^{j(2\pi\delta t+\varphi)}\qquad\textbf{— the complex baseband signal of Lesson 1}$$

*Verified by simulation* ($f_c=10$ kHz, $\delta=500$ Hz, $A=1.3$, $\varphi=0.7$ rad, FIR low-pass): recovered amplitude **1.306**, frequency **500.0 Hz**, phase **0.700 rad**. Because the output is complex, **negative $\delta$ (below the LO) is distinguishable from positive** — the ±f ambiguity of P02 §2.6 is solved by using two mixers 90° apart.

### Two impairments you will meet in real IQ data (Lesson 15) ⚠️
1. **DC offset.** The LO leaks into the receive path and mixes with itself, adding a constant: $r[n]=x[n]+c$ — a spike at exactly **0 Hz** unrelated to any signal.
2. **IQ imbalance.** The I and Q paths never match exactly in gain ($\varepsilon$) and phase ($\phi$). First-order model (given in Lesson 15): $r[n]=\alpha x[n]+\beta x^{*}[n]$ with $\alpha\approx1+\tfrac{\varepsilon+j\phi}2$, $\beta\approx\tfrac{j\phi-\varepsilon}2$. The $x^{*}$ term is a **mirror-image copy at $-f$**. Image rejection ratio $\text{IRR}=|\alpha|^{2}/|\beta|^{2}$.
 *Verified:* $\varepsilon=0.05$ (5 % amplitude error), $\phi=2^\circ$ ⇒ $|\alpha|^{2}=1.0509$, $|\beta|^{2}=0.00093$, **IRR = 30.5 dB**; an FFT of a tone at +400 bins showed the signal at 1.025 and the ghost at −400 bins at 0.0305, i.e. **30.53 dB** apart ✓.

### The ADC: sampling and quantising 🔥
* **Sample rate $f_s$:** for a complex (IQ) stream the rate needed equals the bandwidth being captured (Lesson 2 explains why complex baseband is cheaper).
* **Resolution $B$ bits:** the signal is rounded to one of $2^{B}$ levels. Quantisation noise limits SNR:

$$\text{SNR}_{\text{quant}}\approx6.02\,B+1.76\ \text{dB}\qquad(\text{full-scale sine})$$

*Verified by simulation:* $B=4\to26.33$ dB (rule 25.84); $B=8\to49.77$ (49.92); $B=12\to73.82$ (74.00); $B=16\to98.09$ (98.08). **Each extra bit buys ≈ 6 dB.**
* **dBFS** (P01): 0 dBFS is full scale; a half-scale sine is **−6.02 dBFS**; one LSB of a 16-bit converter is **−90.3 dBFS**.
* **AGC / gain setting:** signal too **large** ⇒ **clipping** (severe distortion); too **small** ⇒ quantisation noise dominates. Receivers amplify (VGA/AGC) so the signal uses most of the ADC range without clipping — this is why you see "gain" settings on every SDR (Lesson 25 builds on the same bit-depth idea).

---

## 7.9 Oscillators and frequency error (ppm) 🔥

Every mixer's LO and the ADC clock derive from a **crystal oscillator**, accurate only to some **parts per million (ppm)**: error $\Delta f=\text{ppm}\times10^{-6}\times f$.

*Verified:* **1 ppm at 2 GHz = 2 kHz**; **20 ppm at 2 GHz = 40 kHz**; 2.5 ppm at 3.5 GHz = 8.75 kHz.

*Meaning:* a transmitter and receiver each with their own crystals disagree about the carrier frequency by thousands of hertz, compared with the **15 kHz subcarrier spacing** used in LTE (a number that appears in Lessons 13, 19 and 28) — so receivers must **estimate and correct the carrier-frequency offset (CFO)**; that is Lesson 13. Typical crystals are tens of ppm; temperature-compensated oscillators (TCXO) are about 1–3 ppm. `[FOUNDATION ADDED — general hardware knowledge, not from the lessons]`

---

## 7.10 The transmit chain and the power amplifier

```
 your software → DAC → analog low-pass → upconversion mixer (with LO) → band filter → PA (power amplifier) → antenna
```
* **Upconversion** is the receive mixing in reverse (P02 product identity again): baseband × LO → RF.
* **PA nonlinearity.** Real amplifiers are linear only up to a point; push them and the output compresses and distorts. A nonlinearity creates **new frequencies** (P06: squaring a cosine adds DC and $2f$), which appear as **out-of-band emissions** that violate the spectral mask — hence the regulation theme in Lesson 42.
* **Peak-to-average power ratio (PAPR):** $10\log_{10}(P_{\text{peak}}/P_{\text{avg}})$. A **pure sine** has PAPR $=10\log_{10}2=\mathbf{3.01}$ dB; a constant-envelope signal has 0 dB. *Verified simulation of a 256-subcarrier OFDM-like symbol (4× oversampled):* **median PAPR 8.3 dB, 99th percentile 10.5 dB**. Many independent subcarriers add up occasionally to a big peak, so the PA must be **backed off** (run well below saturation), wasting efficiency — the central OFDM trade-off detailed in Lesson 42.

---

## 7.11 Python workout (verified)

```python
import numpy as np
from scipy import signal

# FSPL in dB (d in km, f in MHz)
fspl = lambda d_km, f_mhz: 20*np.log10(d_km) + 20*np.log10(f_mhz) + 32.44
print(fspl(1, 2000))                         # 98.46

# sensitivity: -174 + 10log10(B) + NF + SNR_min
print(-174 + 10*np.log10(20e6) + 5 + 10)      # -85.99

# Friis cascade for LNA (NF 1 dB, G 20 dB) then stage NF 10 dB
F1, G1, F2 = 10**0.1, 10**2.0, 10**1.0
print(10*np.log10(F1 + (F2-1)/G1))            # 1.30 dB

# quadrature down-conversion
fs, fc, d, A, phi = 100e3, 10e3, 500, 1.3, 0.7
t = np.arange(20000)/fs
r = A*np.cos(2*np.pi*(fc+d)*t + phi)
lp = signal.firwin(201, 3e3, fs=fs)
I = signal.lfilter(lp, 1,  2*r*np.cos(2*np.pi*fc*t))
Q = signal.lfilter(lp, 1, -2*r*np.sin(2*np.pi*fc*t))
z = (I + 1j*Q)[1000:]
print(np.mean(abs(z)))                                        # ~1.31
print(np.mean(np.angle(z[1:]*np.conj(z[:-1])))*fs/(2*np.pi))  # ~500.0 Hz

# IQ imbalance image rejection (Lesson 15 first-order model)
eps, ph = 0.05, np.deg2rad(2)
al, be = 1 + (eps+1j*ph)/2, (1j*ph - eps)/2
print(10*np.log10(abs(al)**2/abs(be)**2))     # ~30.5 dB

# ADC SNR vs 6.02B + 1.76
B = 12; x = np.sin(2*np.pi*0.123456789*np.arange(2**18))
q = 2/2**B; e = np.round(x/q)*q - x
print(10*np.log10(np.mean(x**2)/np.mean(e**2)), 6.02*B+1.76)   # 73.8  74.0
```

---

## 7.12 Common mistakes ⚠️
1. **Mixing up dB and dBm** (P01) — a "−90" with no unit is meaningless.
2. **Treating dBi as amplification.** An antenna adds directional gain, not power.
3. **Applying FSPL to real urban links** without a path-loss exponent or margins.
4. **Forgetting NF when computing a noise floor** (−174 dBm/Hz is the *ideal* floor).
5. **Assuming the LNA doesn't matter** — the first stage dominates the cascade.
6. **Setting ADC gain so the signal clips** (flat-topped waveform, spectral splatter) — or so low that only a few LSBs toggle.
7. **Mistaking the 0 Hz spike for a signal** (LO leakage) or a mirrored ghost for a second transmitter (IQ imbalance).
8. **Ignoring ppm.** Two devices on "the same frequency" are never exactly on it.
9. **Forgetting that an ADC's bits are a *ceiling*:** adding bits doesn't help if analog noise already exceeds the quantisation noise.
10. **Transmitting casually.** Always check legal frequency, power and spectral-mask limits first.

---

## 7.13 Practice problems (answers at the end)

1. What is the rms voltage of a −60 dBm signal in 50 Ω?
2. Design an RC low-pass with $f_c=10$ kHz using $C=10$ nF. What $R$? What is the gain at 100 kHz, in dB?
3. Length of a half-wave dipole at 868 MHz? At 5.8 GHz?
4. Compute FSPL for 5 km at 900 MHz and 5 km at 3.5 GHz. By how many dB do they differ?
5. With path-loss exponent $n=3$, how much extra loss does going from 200 m to 2 km add?
6. A receiver has $B=5$ MHz, NF = 7 dB and needs 8 dB SNR. Find the sensitivity.
7. A system has a 3 dB-NF LNA with 15 dB gain followed by a mixer with 12 dB NF. Total NF? (Friis; use linear factors.)
8. A 14-bit ADC samples a full-scale sine. Expected SNR? How many bits for ≈ 90 dB?
9. A 2.1 GHz oscillator is off by 5 ppm. Frequency error in kHz? Compare with the 15 kHz LTE subcarrier spacing.
10. IQ imbalance: $\varepsilon=0.02$, $\phi=1^\circ$ (use radians). Approximate $|\alpha|^{2}$, $|\beta|^{2}$ and IRR in dB.
11. A baseband signal is $z(t)=e^{-j2\pi\,300\,t}$. Is the RF signal above or below the LO, and by how much?
12. *Think:* a measured spectrum shows a spike exactly at 0 Hz and a weak mirror copy of your tone. What two hardware effects are responsible and which one is which?
13. *Think:* why do 256-subcarrier OFDM transmitters need power-amplifier backoff while a constant-envelope scheme does not?

## 7.14 Active recall
1. Write Ohm's law and the power formulas. 2. What is the −3 dB point of an RC filter and how fast does it roll off? 3. Why are antennas about λ/2 long, and what is λ at 2.4 GHz? 4. State the FSPL formula in dB and the effect of doubling d or f. 5. What is NF and why does the first stage dominate? 6. Write the sensitivity formula. 7. Draw the direct-conversion receiver and explain what I and Q are. 8. Derive $I$ and $Q$ from the product identity. 9. What produces DC offset and IQ image? 10. State 6.02B + 1.76 and what dBFS means. 11. How is ppm turned into hertz and why does it matter? 12. What is PAPR and why does OFDM suffer from it?

## 7.15 Self-check — ready for P08 if you can…
- [ ] convert dBm to volts in 50 Ω and back;
- [ ] compute FSPL, a noise floor and a sensitivity figure;
- [ ] explain why a first-stage LNA improves system NF;
- [ ] draw and explain the zero-IF receiver and derive I/Q from a mixer;
- [ ] recognise DC offset and IQ imbalance symptoms and state IRR;
- [ ] apply 6.02B+1.76 and the ppm → Hz conversion.

---

## ✅ Answer key

1. $P=10^{-9}$ W; $V=\sqrt{10^{-9}\times50}=\mathbf{2.24\times10^{-4}\ V}$ (**224 µV**). 2. $R=1/(2\pi f_cC)=1/(2\pi\cdot10^{4}\cdot10^{-8})=\mathbf{1.59\ k\Omega}$; at 100 kHz, $|H|=1/\sqrt{1+100}=0.0995$ ⇒ **−20.0 dB**. 3. $\lambda=c/f$: 868 MHz → 0.3454 m ⇒ half-wave **17.3 cm**; 5.8 GHz → 0.0517 m ⇒ **2.59 cm**. 4. FSPL = 20 log 5 + 20 log 900 + 32.44 = 13.98 + 59.08 + 32.44 = **105.5 dB**; at 3.5 GHz: 13.98 + 70.88 + 32.44 = **117.3 dB**; difference **11.8 dB** (= 20 log(3500/900)). 5. 10 n log₁₀(10) = **30 dB**. 6. −174 + 10 log₁₀(5×10⁶) + 7 + 8 = −174 + 66.99 + 15 = **−92.0 dBm**. 7. F₁ = 2.0, G₁ = 31.6, F₂ = 15.85 ⇒ F = 2.0 + 14.85/31.6 = 2.47 ⇒ **3.9 dB**. 8. 6.02×14 + 1.76 = **86.0 dB**; ≈ **15 bits** (6.02×15+1.76 = 92.1). 9. 5×10⁻⁶ × 2.1×10⁹ = **10.5 kHz** — comparable to (about 70 % of) one 15 kHz subcarrier spacing. 10. φ = 0.01745 rad; |α|² ≈ 1.0201 + 0.0001 ≈ 1.0204, |β|² = (0.02² + 0.01745²)/4 = 1.76×10⁻⁴ ⇒ IRR ≈ 5800 ⇒ **37.6 dB**. 11. The baseband tone is at **−300 Hz** ⇒ the RF signal is **300 Hz below** the LO. 12. The **0 Hz spike = DC offset (LO self-mixing)**; the **mirror ghost = IQ imbalance** (the $x^{*}$ term). 13. OFDM's many random subcarriers occasionally add into large peaks (PAPR ≈ 8–10 dB), so the PA must back off to stay linear; a constant-envelope signal has 0 dB PAPR and can run the PA near saturation.

---

*Next: **P08 — Digital foundations** (binary, two's complement, fixed-width integers, IEEE-754 floats, endianness, XOR/modulo-2/GF(2), LFSRs) — the groundwork for fixed-point (Lesson 25), file formats (Lesson 17) and error-correcting codes (Lessons 41–45).*
