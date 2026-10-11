# P01 — The Engineer's Quantitative Toolkit: Units, Powers of Ten, Logarithms and Decibels

*Prerequisite module 1 of the RF-curriculum prerequisite notes · ⏱ about 3–4 hours · no prior knowledge assumed beyond school algebra · every number and code output below was run and checked.*

> **Where this unlocks the curriculum.** A keyword scan of the 48 lessons finds decibel/dB terms in 13 of them (11 with three or more mentions), heaviest in Lesson 7 (Eb/N0 and BER) and Lesson 25 (the 6.02B + 1.76 dB rule), and also in Lessons 8, 14, 15, 18, 20, 32, 41, 42 and 48. The curriculum assumes you are comfortable with dB, so this is the first thing to be fluent in.

---

## 1.1 Why this comes first

Radio numbers are absurd by everyday standards:

* A phone transmits about **0.2 W**; the signal that *arrives* at the tower can be **0.00000000004 W** (4 × 10⁻¹¹ W).
* Carrier frequencies are **billions** of cycles per second; the information on them changes **thousands** of times per second.
* Gains and losses multiply along the chain (amplifier × cable × antenna × distance × …).

Humans are bad at reading 0.000000000035 and bad at multiplying five numbers in their head. Engineers therefore use two tricks: **powers of ten (SI prefixes)** to write sizes, and **logarithms (decibels)** to turn *multiplication into addition*. If you get comfortable with both, link budgets, noise floors, SNR and BER plots stop being intimidating.

---

## 1.2 Units, SI prefixes and scientific notation

### The prefixes you must know by heart

| Prefix | Symbol | Power of 10 | Typical radio use |
|---|---|---|---|
| tera | T | 10¹² | – |
| giga | G | 10⁹ | 2.4 GHz Wi-Fi, 3.5 GHz 5G |
| mega | M | 10⁶ | 20 MHz channel, 30.72 Msps |
| kilo | k | 10³ | 15 kHz subcarrier spacing |
| (none) | – | 10⁰ | Hz, W, V |
| milli | m | 10⁻³ | mW, ms (a 1 ms LTE subframe) |
| micro | µ | 10⁻⁶ | µs (cyclic prefix durations), µV |
| nano | n | 10⁻⁹ | ns (sampling instants), nW |
| pico | p | 10⁻¹² | pW (received powers) |

**Habit that prevents most bugs:** *always carry the unit, convert to base units (Hz, s, W, V, m) before calculating, and convert back at the end.* 30.72 Msps is 30.72 × 10⁶ samples per second; a 15 kHz spacing is 15 000 Hz; 1 ms is 10⁻³ s.

### Frequency, period, wavelength — three views of the same sine wave

$$T=\frac1f,\qquad \lambda=\frac{c}{f},\qquad c\approx3\times10^{8}\ \text{m/s}$$

* **Frequency $f$** (hertz, Hz) = cycles per second. **Period $T$** (seconds) = time for one cycle.
* **Wavelength $\lambda$** (metres) = distance the wave travels in one cycle.

Verified values:

| Frequency | Wavelength | Period |
|---|---|---|
| 88 MHz (FM radio) | 3.41 m | 11.4 ns |
| 900 MHz (GSM) | 33.3 cm | 1.11 ns |
| 1.8 GHz | 16.7 cm | 0.556 ns |
| 2.4 GHz (Wi-Fi/Bluetooth) | 12.5 cm | 0.417 ns |
| 3.5 GHz (5G mid-band) | 8.57 cm | 0.286 ns |
| 28 GHz (5G mmWave) | 1.07 cm | 0.0357 ns |

*Meaning:* antennas are typically a fraction of a wavelength long, so higher frequency ⇒ smaller antennas (why phones can fit many 5G antennas). *(You may recognise c = fλ from the college Wireless Communication notes, Chapter 1.)*

### Scientific notation and estimation
Write numbers as $a\times10^{n}$ with $1\le a<10$. Multiply by adding exponents, divide by subtracting.
**Fermi estimate example:** how many samples does a 20 MHz LTE channel produce in one 1 ms subframe? The curriculum's baseband rate is 30.72 Msps `[Curriculum Lesson 2]`: $30.72\times10^{6}\times10^{-3}=30{,}720$ complex samples. A quick order-of-magnitude check ("tens of thousands") catches most copy-paste errors in code.

---

## 1.3 Exponents and logarithms from first principles

**Exponent:** $10^{n}$ means "multiply 1 by 10, $n$ times"; $10^{0}=1$; $10^{-n}=1/10^{n}$. Rules: $10^{a}\cdot10^{b}=10^{a+b}$, $(10^{a})^{b}=10^{ab}$.

**Logarithm (base 10)** answers the reverse question: *"10 to the power of what gives this number?"*

$$\log_{10}(x)=y\iff10^{y}=x$$

So $\log_{10}1000=3$, $\log_{10}0.01=-2$, $\log_{10}2\approx0.301$, $\log_{10}3\approx0.477$.

**The property that makes everything work** (it follows directly from the exponent rule above):

$$\log(ab)=\log a+\log b,\qquad \log\frac ab=\log a-\log b,\qquad \log(a^{k})=k\log a$$

*Multiplication becomes addition, division becomes subtraction, powers become multiplication.* That is the *entire* reason for decibels.

**Natural log** $\ln$ uses base $e\approx2.718$; you will meet $e^{x}$ constantly (Euler's formula in Lesson 1, Gaussian noise in Lesson 7). **Change of base:** $\log_{10}x=\ln x/\ln10$. In Python, `np.log10` and `np.log` are the two you need.

**Danger:** $\log$ of 0 is $-\infty$ (and of a negative number is undefined for real logs). Code that converts power to dB must guard against zero (see §1.6).

---

## 1.4 The decibel

### The definition (memorise exactly)

$$\boxed{\;G_{\text{dB}}=10\log_{10}\!\frac{P_{2}}{P_{1}}\;}\qquad\text{for a ratio of powers}$$

$$\boxed{\;G_{\text{dB}}=20\log_{10}\!\frac{V_{2}}{V_{1}}\;}\qquad\text{for a ratio of amplitudes (voltage, current, field)}$$

**Why 20 for amplitudes?** Power is proportional to amplitude *squared*, and $\log(V^{2})=2\log V$. So the same physical ratio gives the same dB either way: doubling the **voltage** is a factor 4 in **power**, and both give +6.02 dB.

A **bel** is $\log_{10}$ of a power ratio; the **deci**bel is a tenth of a bel — hence the "10". A decibel is a *ratio*, not a unit of power: it is **dimensionless**.

### The numbers worth memorising (all verified)

| Power ratio | dB | Mental shortcut |
|---|---|---|
| ×1 | 0 | no change |
| ×1.26 | +1 | 1 dB ≈ +26 % power |
| ×2 | **+3.01** | "3 dB = double" |
| ×4 | **+6.02** | two doublings |
| ×10 | **+10** | |
| ×100 | **+20** | |
| ×1000 | **+30** | |
| ×0.5 | **−3.01** | "−3 dB = half" |
| ×0.1 | −10 | |
| ×0.01 | −20 | |

**The 3-6-10 rule:** $\;+3\text{ dB}\approx\times2,\;+6\approx\times4,\;+10=\times10$. Everything else is built by adding: $+13\text{ dB}=10+3\to\times10\times2=\times20$ (exact: 19.95). $+7\text{ dB}=10-3\to\times10/2=\times5$ (exact 5.01). $-20\text{ dB}=\times0.01$.

**dB → ratio:** $\;\text{ratio}=10^{G_{\text{dB}}/10}$ (power) or $10^{G_{\text{dB}}/20}$ (amplitude).

### Absolute levels: dBm, dBW and friends 🔥

A pure ratio is *relative*. To describe an **absolute** power, compare to a fixed reference and put the reference in the unit's name:

| Unit | Reference | Meaning / example |
|---|---|---|
| **dBW** | 1 W | 1 W = 0 dBW |
| **dBm** | 1 mW | **0 dBm = 1 mW**, 30 dBm = 1 W, −30 dBm = 1 µW, −90 dBm = 1 pW |
| **dBc** | the carrier's own power | "spurious emission −60 dBc" = 60 dB below the main signal |
| **dBi** | an ideal isotropic antenna | antenna gain: +2 dBi, +17 dBi |
| **dBFS** | ADC full scale | 0 dBFS = maximum representable level; every lower level is negative |

Verified conversions: 1 mW → 0 dBm; 10 mW → 10 dBm; 100 mW → 20 dBm; **200 mW → 23.01 dBm** (a typical maximum phone transmit power is quoted as **23 dBm**, ≈ 199.5 mW); 1 W → 30 dBm; 5 W → 37 dBm; 10⁻⁹ mW → −90 dBm.

### The arithmetic rules (this is where people lose marks and hours)

| Operation | Allowed? | Rule |
|---|---|---|
| dBm + dB → dBm | ✅ | apply a gain/loss to a level: **−90 dBm + 20 dB = −70 dBm** |
| dBm − dBm → dB | ✅ | difference of two levels is a *ratio*: SNR = signal(dBm) − noise(dBm) |
| dB + dB → dB | ✅ | cascaded gains add |
| **dBm + dBm** | ❌ **not meaningful as written** | to add two *powers*, convert to mW, add, convert back |
| averaging dB values | ⚠️ usually wrong | average in the **linear** domain, then convert |

**Verified examples**
* Two equal 0 dBm (1 mW) signals that do not interfere coherently add to **2 mW = +3.01 dBm**, *not* 0 + 0 = 0.
* −10, −10 and −13 dBm summed: 0.1 + 0.1 + 0.05 = 0.25 mW ⇒ **−6.02 dBm**.
* Average of 0 dBm (1 mW) and 20 dBm (100 mW): linear mean 50.5 mW ⇒ **17.03 dBm**, whereas the naïve dB average gives 10 dBm — wrong by 7 dB.

### The superpower: cascades become additions (a link-budget taste)

A transmitter sends **23 dBm**; its antenna adds **+2 dBi**; free-space loss over the path is **98.5 dB**; the receive antenna adds **0 dBi**; a cable loses **1 dB**:

$$P_{rx}=23+2-98.5+0-1=-74.5\ \text{dBm}=3.5\times10^{-11}\ \text{W}$$

Five multiplications of awkward numbers became one row of additions. Where 98.5 dB comes from is in P07 (path loss).

---

## 1.5 Noise power in dBm — one number you will use for years

Thermal noise power in a bandwidth $B$ at temperature $T$ is $N=kTB$ (derived and explained in **P07**), with Boltzmann's constant $k=1.380649\times10^{-23}$ J/K. At the conventional $T=290$ K:

$$kT=4.0\times10^{-21}\ \text{W/Hz}=\boxed{-174\ \text{dBm/Hz}}\qquad N_{\text{dBm}}=-174+10\log_{10}B_{\text{Hz}}$$

| Bandwidth | Noise floor | with a 5 dB receiver noise figure |
|---|---|---|
| 1 kHz | −144.0 dBm | −139.0 dBm |
| 200 kHz (GSM channel) | −121.0 dBm | −116.0 dBm |
| 1.4 MHz (smallest LTE) | −112.5 dBm | −107.5 dBm |
| **20 MHz** (large LTE) | **−101.0 dBm** | **−96.0 dBm** |
| 100 MHz (5G NR) | −94.0 dBm | −89.0 dBm |

**Worked SNR example.** A signal arrives at **−85 dBm** in a 20 MHz channel; the receiver has a 5 dB noise figure ⇒ noise floor −96 dBm ⇒ **SNR = −85 − (−96) = +11 dB.** Notice we only *subtracted two dBm numbers* — the answer is a plain dB ratio.

*Meaning:* doubling the bandwidth raises the noise floor by 3 dB, so wideband systems pay for their speed in noise.

---

## 1.6 Doing it in Python (verified code)

```python
import numpy as np

def db10(x):            # power ratio  -> dB
    return 10*np.log10(x)

def db20(x):            # amplitude ratio -> dB
    return 20*np.log10(x)

def from_db10(d):       # dB -> power ratio
    return 10**(np.asarray(d)/10)

def from_db20(d):       # dB -> amplitude ratio
    return 10**(np.asarray(d)/20)

def dbm_to_mw(d): return 10**(np.asarray(d)/10)
def mw_to_dbm(p): return 10*np.log10(p)

print(db10(2), db20(2))                 # 3.0103  6.0206
print(from_db10(23))                    # 199.526  (23 dBm ~ 199.5 mW)
print(mw_to_dbm(1+1))                   # 3.0103   two 1 mW signals add to +3 dBm
levels = np.array([-10, -10, -13])      # dBm
print(mw_to_dbm(dbm_to_mw(levels).sum()))   # -6.0206

# averaging in the linear domain (correct) vs dB domain (wrong)
x = np.array([0.0, 20.0])               # dBm
print(mw_to_dbm(dbm_to_mw(x).mean()), x.mean())   # 17.03  vs 10.0

# guard against log of zero
v = np.array([1.0, 0.1, 0.0])
print(20*np.log10(np.maximum(np.abs(v), 1e-12)))  # [   0.  -20. -240.]
```
Without the `np.maximum(...)` guard, `np.log10(0.0)` returns `-inf` (and NumPy prints a warning) — a classic way spectrum plots break.

---

## 1.7 Common mistakes ⚠️
1. **Using 10 where 20 belongs (or vice versa).** Power/energy/PSD → 10; voltage/current/field/amplitude/magnitude of a complex sample → 20. (If you compute dB from $|x|^2$ use 10; from $|x|$ use 20.)
2. **Adding two dBm values.** Convert to linear first.
3. **Averaging dB numbers** (e.g. averaging SNR in dB over many trials) — convert to linear, average, convert back.
4. **Treating "−3 dB" as "−3 units".** It means "multiply power by 0.5".
5. **Forgetting the unit.** "20 dB" alone is a ratio; "20 dBm" is an absolute level. A receiver sensitivity of "−100 dBm" is a power, not a gain.
6. **Rounding 3.01 to 3 and 6.02 to 6 inside long chains** — fine for mental estimates, but use exact values in code.
7. **log of zero/negative** in plotting code.

---

## 1.8 Practice problems (answers at the end)

1. Convert **27 dBm** to milliwatts.
2. A gain of **+13 dB** corresponds to roughly what power factor? Use the 3-6-10 rule, then check with `from_db10`.
3. A chain: amplifier **+20 dB**, cable **−3 dB**, filter **−1.5 dB**. What is the total gain? If the input is **−90 dBm**, what is the output?
4. Add two signals of **−30 dBm** and **−40 dBm** (powers add). Result in dBm?
5. The voltage amplitude of a signal is doubled. By how many dB does its power change?
6. A signal level is **−95 dBm** and the noise floor is **−101 dBm**. What is the SNR in dB?
7. What is the thermal noise floor (T = 290 K, no noise figure) in **1.4 MHz**?
8. How many dB is a **voltage** ratio of 1/1000?
9. Find the wavelength at **2.1 GHz** (use c = 3 × 10⁸ m/s).
10. A transmitter outputs **5 W**. Express this in dBm.
11. A 16-bit ADC sample with amplitude **1 LSB** (= 1/32768 of full scale) is how many **dBFS**?
12. *Think:* a colleague reports "average SNR = 10 dB" by averaging dB values from 1000 trials with wildly varying SNR. Why is this suspicious and how would you fix it?

## 1.9 Active recall (answer without looking)
1. Write the two dB definitions and say when each applies. 2. Why is the amplitude version 20 and not 10? 3. State the 3-6-10 rule. 4. What is 0 dBm in watts? 5. What does dBFS refer to? 6. Why can't you add dBm values directly? 7. What is −174 dBm/Hz and where does it come from (one sentence)? 8. What happens to the noise floor when bandwidth doubles? 9. Why do logs turn cascades into sums?

## 1.10 Self-check — you are ready for the next module if you can…
- [ ] convert between dB and linear ratios *in your head* for 3, 6, 10, 20 dB;
- [ ] compute an SNR from two dBm numbers and a bandwidth;
- [ ] explain why 0 dBm + 0 dBm = +3 dBm (not 0);
- [ ] write `db10` / `db20` and a safe-log helper without looking.

---

## ✅ Answer key

1. $10^{2.7}=501.2$ mW ≈ **0.5 W**. 2. 3-6-10 rule: 13 = 10 + 3 ⇒ ×10 × 2 = **×20** (exact 19.95). 3. Total = 20 − 3 − 1.5 = **+15.5 dB**; output = −90 + 15.5 = **−74.5 dBm**. 4. 0.001 mW + 0.0001 mW = 0.0011 mW ⇒ **−29.59 dBm**. 5. Power ×4 ⇒ **+6.02 dB**. 6. −95 − (−101) = **+6 dB**. 7. −174 + 10 log₁₀(1.4×10⁶) = **−112.5 dBm**. 8. 20 log₁₀(0.001) = **−60 dB**. 9. λ = 3×10⁸ / 2.1×10⁹ = **0.143 m (14.3 cm)**. 10. 5 W = 5000 mW ⇒ **36.99 ≈ 37 dBm**. 11. 20 log₁₀(1/32768) = **−90.3 dBFS** (this is the ~"90 dB dynamic range" of 16-bit audio/IQ; compare with the 6.02B+1.76 rule in Lesson 25). 12. Averaging dB gives the *geometric* mean, which hides the high-SNR trials; convert each SNR to linear, average, then convert (or report the median/CDF).

---

*Next: **P02 — Trigonometry and sinusoids** (radians, amplitude/frequency/phase, the sine and cosine as one rotating arrow).*
