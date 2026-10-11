# P13 — Readiness Exam: Are You Ready to Start Lesson 1?

*Prerequisite module 13 (final) · ⏱ about 3 hours · **closed-book** · covers P01–P12 · every written answer was computed and every lab was run, with the expected results shown.*

> **Purpose.** Reading is not knowing. This exam tells you — with numbers, not feelings — whether to **start Lesson 1 now**, **start but fix specific gaps in parallel**, or **go back first**. It also gives **lesson-by-lesson gates** (e.g. "don't start Lesson 7 until probability scores ≥ 3/5") so you can begin early without hitting a wall.
>
> **Important honesty note.** The *pass marks and gates below are my suggestions*, derived from the dependency map in `00_START_HERE.md` (which came from keyword scans of the curriculum); they are **not** requirements stated by the curriculum.

---

## How to take it

1. **Part 1 — Written (60 questions, 1 mark each, ~90 minutes).** Closed book. A calculator is allowed; **no code**, no notes, no internet. Write each answer before looking at the key.
2. **Part 2 — Labs (7 hands-on tasks, pass/fail, ~90 minutes).** You may use Python/NumPy/g++ and the tools you built (e.g. your `rf_utils.py`), but **not** the reference solutions. Each lab states its expected result.
3. **Mark yourself honestly** with the key at the end, fill in the score table (§P13.4), then read the decision rules.
4. If you score below the thresholds, follow the remediation map and **retake only the failed modules** after a few days.

---

# PART 1 — WRITTEN EXAM (60 marks)

### Section A — P01 Units, logs, decibels (5)
1. Convert **20 dBm** to milliwatts.
2. Two *incoherent* signals of **−20 dBm** each are added. What is the total, in dBm?
3. What is the thermal noise floor in **10 MHz** at 290 K (no noise figure)?
4. What is a **voltage** ratio of 0.1 in dB?
5. A signal arrives at −82 dBm and the noise floor is −96 dBm. What is the SNR?

### Section B — P02 Trigonometry and sinusoids (5)
6. Express **270°** in radians.
7. A 500 Hz tone is delayed by **0.5 ms**. What phase shift (magnitude, degrees) results?
8. A 900 MHz cosine is multiplied by an 880 MHz cosine. Which two frequencies appear?
9. Combine $3\cos\omega t+4\cos(\omega t+90^\circ)$ into $R\cos(\omega t+\varphi)$. Give $R$ and $\varphi$.
10. A 3 kHz tone is sampled at 4 kHz. At what frequency does it appear?

### Section C — P03 Series and calculus (5)
11. Evaluate $\sum_{n=0}^{4}2^{n}$.
12. Find the amplitude of $\frac{d}{dt}\bigl[5\sin(2\pi\cdot20\,t)\bigr]$.
13. A phase is $\varphi(t)=2\pi(50t+25t^{2})$. What is the instantaneous frequency at $t=2$ s?
14. Evaluate $\int_0^1\cos^{2}(2\pi t)\,dt$.
15. Evaluate $\sum_{n=0}^{7}e^{j2\pi\cdot3n/8}$.

### Section D — P04 Linear algebra (5)
16. Find $\|[2,3,6]\|$.
17. For $\mathbf a=[1,\ j]$ and $\mathbf b=[1,\ 1]$ compute the complex inner product $\mathbf a^{H}\mathbf b$.
18. What is the rank of $\begin{bmatrix}1&2\\2&4\end{bmatrix}$?
19. $\mathbf H$ is $4\times2$ and $\mathbf x$ is $2\times1$. What is the shape of $\mathbf H\mathbf x$, and how many receive/transmit antennas does that suggest?
20. A channel has singular values 8 and 0.5. What is its condition number?

### Section E — P05 Probability and noise (5)
21. Each bit is wrong independently with probability 0.01. What is $P(\text{no errors in 8 bits})$?
22. $X\sim N(0,1)$ and $Y\sim N(0,4)$ are independent. What is the variance of $X+Y$?
23. What is the BPSK bit-error probability when $\sqrt{2E_b/N_0}=3$? (Use the Q-function.)
24. A complex noise sample has $N_0=4$. What is the variance of its *real part*?
25. A simulation sees **25 errors** in $10^{5}$ bits. Give $\hat p$, the approximate relative uncertainty, and whether this is enough errors.

### Section F — P06 Signals and Fourier series (5)
26. Is $y[n]=n\,x[n]$ linear? Time-invariant?
27. What fraction of a ±1 square wave's power is in its fundamental?
28. Where is the first spectral null of a **2 µs** rectangular pulse?
29. What is the energy of a rectangular pulse of height 3 and width 2 s?
30. You add 1000 harmonics to the square-wave Fourier series. Does the edge overshoot vanish?

### Section G — P07 Physics and electronics (5)
31. What is the rms voltage of a **−80 dBm** signal in 50 Ω?
32. Compute free-space path loss for **1 km at 1 GHz**.
33. Sensitivity for $B=10$ MHz, NF = 6 dB, required SNR = 5 dB?
34. Expected SNR from a full-scale sine through a **16-bit** ADC?
35. How many kHz is **5 ppm** at 2.4 GHz?

### Section H — P08 Digital foundations (5)
36. What is the 8-bit two's-complement value of `11010000`?
37. In `int16`, what does 30000 + 10000 wrap to?
38. Convert the `cu8` byte **191** to a float sample.
39. Compute `0xA5 ^ 0xFF`.
40. What is the period of a maximal-length **5-bit** LFSR?

### Section I — P09 Python / NumPy (5)
41. Is `np.arange(10)[::2]` a view or a copy?
42. What dtype results from multiplying a `complex64` array by a `float64` array?
43. What does `np.fft.fftfreq(8, 1/8)[5]` return?
44. Write the single call that reads 1000 `complex64` samples starting at sample index 5000 of a raw file.
45. What is the standard deviation of *each real dimension* of unit-power complex Gaussian noise?

### Section J — P10 C++ (5)
46. What is `sizeof(std::complex<double>)`?
47. After `uint8_t x = 250; x += 10;` what is `x`?
48. Which compiler flag detects a heap-buffer-overflow at run time?
49. What does `-1 < 1u` evaluate to in C++, and why?
50. What does `std::norm(std::complex<float>(3, 4))` return?

### Section K — P11 Systems and protocol layers (5)
51. A system call costs 110 ns. What happens if you make one per sample at **10 Msps**?
52. What is the per-sample time budget for 30,720 samples in 1 ms?
53. Name a remedy for **priority inversion**.
54. Stop-and-wait: RTT 40 ms, 1000-byte PDUs, 10 Mbit/s. What is the link utilisation?
55. The bytes `0x12 0x34` are read as a **big-endian** 16-bit integer. What is the value?

### Section L — P12 Bridge to the college notes (5)
56. By how many dB do the six-interferer SIR and the one-interferer SINR models differ?
57. What curriculum terms correspond to "handoff" and "forward channel"?
58. Lesson 18's formula gives what SINR for $N=7$, $n=4$?
59. Which NR RRC state is the analogue of GPRS's STANDBY idea?
60. Which of AMPS, EDGE and Mobile IP does the curriculum cover?

---

# PART 2 — HANDS-ON LABS (7 pass/fail tasks)

Run each in a clean folder. A lab *passes* only if your output meets the stated criterion.

**Lab 1 — dB toolbox and noise at a target SNR (P01, P05, P09).** Write `db10`, `avg_power` and `awgn(x, snr_db, rng)` (complex noise with variance $N_0/2$ per dimension). Add noise to a unit-power tone at **0, 10 and 20 dB** and measure the SNR you actually achieved. *Pass:* every measured value within **±0.3 dB** of its target.

**Lab 2 — BPSK Monte Carlo (P05).** Simulate **2,000,000** BPSK bits at $E_b/N_0=4$ dB with a seeded generator. *Pass:* BER within **5 %** of $Q(\sqrt{2E_b/N_0})\approx0.0125$ and **more than 100 errors**.

**Lab 3 — FFT recipe (P06, P09).** Make a complex tone at **−150 kHz** with $f_s=1$ MHz, $N=4096$; compute a Hann-windowed spectrum with the correct frequency axis. *Pass:* peak within **one bin (244 Hz)** of −150 kHz.

**Lab 4 — Read a `cu8` capture (P07, P08, P09).** The generator below writes `capture.cu8` (100,000 samples; a tone of unknown-to-you frequency 123.4 kHz and amplitude 0.8 at 1 Msps). Read it back, convert correctly, and report the **tone frequency** and **average power in dBFS**. *Pass:* frequency within **100 Hz** and power within **0.1 dB** of 20·log₁₀(0.8) = −1.94 dBFS.

**Lab 5 — Find the bug (P10).** Compile the P10 program `asan.cpp` with `-fsanitize=address -g -O1`, run it, and report the **bug class** and the **source line**. *Pass:* you name *heap-buffer-overflow* and *line 4*, and you can explain why the same program prints `p[4] = 4` and exits with code 0 **without** the sanitizer.

**Lab 6 — Reproduce and fix a race (P11).** Compile the P11 program `race.cpp` with `-pthread`, run it **three times**, and tabulate the three counters. *Pass:* the plain counter is **less than 10,000,000 in each run** (I observed 5.0–6.7 million on a single-core machine), while the atomic and mutex counters are **exactly 10,000,000**; and you can explain why `volatile` did not help.

**Lab 7 — Header bits and byte order (P08, P11).** Implement `pack_hdr(poll, sn)` / `unpack_hdr` for the toy 16-bit header `[Poll:1][Reserved:3][SN:12]` in network byte order. *Pass:* `pack_hdr(1, 2500)` gives bytes `89 c4`, round-trips to `(1, 2500)`, and a little-endian reader sees SN **1161**.

---

# P13.4 — SCORING AND DECISION

### Score table

| Section | Module | Score (0–5) |
|---|---|---|
| A | P01 | |
| B | P02 | |
| C | P03 | |
| D | P04 | |
| E | P05 | |
| F | P06 | |
| G | P07 | |
| H | P08 | |
| I | P09 | |
| J | P10 | |
| K | P11 | |
| L | P12 | |
| | **Total /60** | |
| Labs passed | | /7 |

### Overall decision *(my suggested rules)*

| Total | Labs | Decision |
|---|---|---|
| **≥ 54** | ≥ 6 | **Start Lesson 1 now.** Skim the modules you scored 4/5 or less on. |
| **42–53** | ≥ 5 | **Start Lesson 1; fix gaps in parallel** using the gates below and the remediation map. |
| **30–41** | any | **Do not start yet.** Re-read the modules below 4/5, redo their practice problems, retake those sections. |
| **< 30** | any | **Go back to P01** and work through the modules in order, running every code block yourself. |

### Lesson gates *(do not start the lesson until you meet its gate)*

| Before… | Gate |
|---|---|
| **Lesson 1** | P01, P02, P03 and P09 each ≥ **3/5**; Labs 1 and 3 passed |
| Lessons 2–5 | P06 ≥ 3/5 |
| **Lesson 7** | P05 ≥ 3/5; Lab 2 passed |
| Lessons 11–14, 46 | P04 ≥ 3/5 |
| Lessons 15–17, 42 | P07 ≥ 3/5 and P08 ≥ 3/5; Lab 4 passed |
| Lesson 18 | skim P12 (§12.4 especially) |
| **Lesson 22** (start of C++) | P10 ≥ 3/5; Lab 5 passed |
| Lessons 24, 27 | P11 ≥ 3/5; Lab 6 passed |
| Lessons 20, 21, 33, 38, 39 | P11 §11.10 re-read; Lab 7 passed |
| Lessons 41, 44, 45 | P08 ≥ 3/5 (XOR/GF(2)) |

### Remediation map — if a section scores below 3/5, re-read:

| Section | Re-read | Then redo |
|---|---|---|
| A (P01) | §1.4 (dB), §1.5 (noise) | P01 problems 1–12 |
| B (P02) | §2.4 (sinusoids), §2.5 (identities), §2.9 (sampling) | P02 problems 1–11 |
| C (P03) | §3.2–3.3 (Σ, geometric series), §3.5–3.7 (derivative, integral, orthogonality) | P03 problems 1–11 |
| D (P04) | §4.3 (dot product), §4.6–4.7 (inverse, rank), §4.9 (SVD) | P04 problems 1–13 |
| E (P05) | §5.4–5.7 (variance, Gaussian, Q, N₀/2), §5.9–5.10 (Monte Carlo, error bars) | P05 problems 1–12 |
| F (P06) | §6.4 (systems), §6.5–6.6 (Fourier series, Parseval), §6.8 (duality) | P06 problems 1–12 |
| G (P07) | §7.6 (path loss), §7.7 (noise), §7.8 (receiver chain, ADC) | P07 problems 1–13 |
| H (P08) | §8.2 (two's complement), §8.3 (cu8), §8.7 (XOR/GF(2)) | P08 problems 1–13 |
| I (P09) | §9.2–9.3 (views, dtypes), §9.6 (FFT), §9.8 (files) | P09 problems 1–11 |
| J (P10) | §10.3 (types, UB), §10.5 (memory, ASan), §10.6 (`vector`) | P10 problems 1–12 |
| K (P11) | §11.2–11.8 (timing, memory, races), §11.10 (layers) | P11 problems 1–13 |
| L (P12) | §12.3 (terminology), §12.4 (SINR gap), §12.5 (bridges) | P12 exercises 1–9 |

---

# ANSWER KEY (check only after finishing)

### Part 1
**A.** 1. **100 mW** (10^(20/10)). 2. 2 × 10 µW = 20 µW ⇒ **−16.99 ≈ −17 dBm**. 3. −174 + 70 = **−104 dBm**. 4. 20 log₁₀(0.1) = **−20 dB**. 5. **14 dB**.
**B.** 6. **3π/2 ≈ 4.712 rad**. 7. 2π·500·0.0005 = π/2 ⇒ **90°**. 8. **20 MHz** (difference) and **1.78 GHz** (sum). 9. Arrows 3∠0° + 4∠90° ⇒ **R = 5, φ = 53.13°**. 10. 3 kHz sampled at 4 kHz aliases to **1 kHz** (|3 − 4|).
**C.** 11. 1+2+4+8+16 = **31**. 12. 5 · 2π · 20 = 200π ≈ **628.3**. 13. (1/2π)dφ/dt = 50 + 50t ⇒ **150 Hz**. 14. **0.5**. 15. Equally spaced unit arrows (k = 3 is not a multiple of 8) sum to **0**.
**D.** 16. √(4+9+36) = **7**. 17. conj(1)·1 + conj(j)·1 = 1 − j ⇒ **1 − j** (`np.dot` would give 1 + j — no conjugate). 18. **1** (row 2 = 2 × row 1). 19. **4 × 1**; 4 receive and 2 transmit antennas. 20. 8 / 0.5 = **16**.
**E.** 21. 0.99⁸ = **0.923**. 22. 1 + 4 = **5**. 23. Q(3) = **1.35 × 10⁻³**. 24. N₀/2 = **2**. 25. p̂ = 2.5 × 10⁻⁴; relative uncertainty ≈ 1/√25 = **20 %**; **not enough** — aim for ≥ 100 errors.
**F.** 26. **Linear: yes; time-invariant: no** (the multiplier n depends on time). 27. (4/π)²/2 = **81.06 %**. 28. 1/τ = **500 kHz**. 29. 3² × 2 = **18**. 30. **No** — the peak stays ≈ 1.179 (≈ 9 % of the jump); only its width shrinks (Gibbs).
**G.** 31. P = 10⁻¹¹ W; V = √(10⁻¹¹ × 50) = **22.4 µV**. 32. 20 log₁₀ 1 + 20 log₁₀ 1000 + 32.44 = **92.44 dB**. 33. −174 + 70 + 6 + 5 = **−93 dBm**. 34. 6.02 × 16 + 1.76 = **98.1 dB**. 35. 5 × 10⁻⁶ × 2.4 × 10⁹ = **12 kHz**.
**H.** 36. 208 − 256 = **−48**. 37. 40000 − 65536 = **−25536**. 38. (191 − 127.5)/127.5 = **+0.498**. 39. **0x5A**. 40. 2⁵ − 1 = **31**.
**I.** 41. **View** (it shares memory). 42. **complex128**. 43. **−3.0** (the unshifted order is 0, 1, 2, 3, −4, −3, −2, −1). 44. `np.fromfile(path, dtype=np.complex64, count=1000, offset=8*5000)`. 45. 1/√2 ≈ **0.707**.
**J.** 46. **16**. 47. **4** (unsigned wrap-around). 48. **`-fsanitize=address`**. 49. **false** — the −1 is converted to the unsigned value 4,294,967,295 before comparing. 50. **25** (`norm` = re² + im², no square root).
**K.** 51. 10 × 10⁶ × 110 ns = **1.1 s of CPU per second ⇒ impossible** (110 % of a core). 52. 1 ms / 30,720 = **32.6 ns**. 53. **Priority inheritance** (or avoid the lock with a lock-free queue). 54. T_tx = 0.8 ms ⇒ 0.8 / (0.8 + 40) = **1.96 %**. 55. **0x1234 = 4660**.
**L.** 56. **7.78 dB** (10 log₁₀ 6; the six-interferer model is lower). 57. **handover** and **downlink**. 58. **26.4 dB** (one nearest interferer). 59. **RRC_INACTIVE**. 60. **None** — all three have 0 mentions in the curriculum.

### Part 2 — reference solutions

**Lab 1**
```python
import numpy as np

def db10(x, eps=1e-30):
    return 10*np.log10(np.maximum(np.asarray(x, dtype=float), eps))

def avg_power(x):
    return np.mean(np.abs(x)**2)

def awgn(x, snr_db, rng):
    p_noise = avg_power(x) / 10**(snr_db/10)
    sigma = np.sqrt(p_noise/2)
    return x + sigma*(rng.standard_normal(len(x)) + 1j*rng.standard_normal(len(x)))

rng = np.random.default_rng(1)
x = np.exp(2j*np.pi*0.1*np.arange(200_000))          # unit-power tone
for target in (0, 10, 20):
    y = awgn(x, target, rng)
    measured = db10(avg_power(x) / avg_power(y - x))
    print(f"target {target:2d} dB -> measured {measured:6.2f} dB")
    assert abs(measured - target) < 0.3
print("LAB 1 PASS")
```
Real output: `target 0 dB -> measured 0.02 dB`, `10 dB -> 10.01 dB`, `20 dB -> 20.01 dB`, then `LAB 1 PASS`.

**Lab 2**
```python
import numpy as np
from scipy.special import erfc

Q = lambda x: 0.5*erfc(x/np.sqrt(2))
rng = np.random.default_rng(2)
ebn0_db, n_bits = 4.0, 2_000_000
e = 10**(ebn0_db/10)
bits = rng.integers(0, 2, n_bits)
s = 2*bits - 1.0                                       # Eb = 1
r = s + np.sqrt(1/(2*e))*rng.standard_normal(n_bits)   # noise variance N0/2 = 1/(2e)
errors = np.count_nonzero((r > 0) != (bits == 1))
ber = errors/n_bits; theory = Q(np.sqrt(2*e))
print(f"errors={errors}  BER sim={ber:.5f}  theory={theory:.5f}  ratio={ber/theory:.3f}")
assert abs(ber/theory - 1) < 0.05 and errors > 100
print("LAB 2 PASS")
```
Real output: `errors=25140  BER sim=0.01257  theory=0.01250  ratio=1.006` → pass.

**Lab 3**
```python
import numpy as np

fs, N, f0 = 1e6, 4096, -150e3
t = np.arange(N)/fs
x = np.exp(2j*np.pi*f0*t)
X = np.fft.fftshift(np.fft.fft(x*np.hanning(N)))
f = np.fft.fftshift(np.fft.fftfreq(N, 1/fs))
peak = f[np.argmax(np.abs(X))]
print(f"peak at {peak/1e3:.3f} kHz (bin width {fs/N:.1f} Hz)")
assert abs(peak - f0) <= fs/N
print("LAB 3 PASS")
```
Real output: `peak at -149.902 kHz (bin width 244.1 Hz)` → pass.

**Lab 4** *(the first half generates the file; the second half is the task's solution)*
```python
import numpy as np

# --- generate a test capture (cu8): tone at +123.4 kHz, amplitude 0.8, fs = 1 MHz
fs, f0, A, N = 1e6, 123.4e3, 0.8, 100_000
t = np.arange(N)/fs
z = A*np.exp(2j*np.pi*f0*t)
iq = np.empty(2*N, dtype=np.uint8)
iq[0::2] = np.round(z.real*127.5 + 127.5).astype(np.uint8)
iq[1::2] = np.round(z.imag*127.5 + 127.5).astype(np.uint8)
iq.tofile("capture.cu8")

# --- YOUR TASK: read it back, estimate the frequency and the average power in dBFS
raw = np.fromfile("capture.cu8", dtype=np.uint8)
f = (raw.astype(np.float32) - 127.5)/127.5             # cast BEFORE subtracting
x = f[0::2] + 1j*f[1::2]
X = np.fft.fftshift(np.fft.fft(x[:16384]*np.hanning(16384)))
freqs = np.fft.fftshift(np.fft.fftfreq(16384, 1/fs))
f_est = freqs[np.argmax(np.abs(X))]
p_dbfs = 10*np.log10(np.mean(np.abs(x)**2))
print(f"samples={len(x)}  f_est={f_est/1e3:.3f} kHz  power={p_dbfs:.2f} dBFS  (expected 123.4 kHz, {20*np.log10(A):.2f} dBFS)")
assert abs(f_est - f0) < 100 and abs(p_dbfs - 20*np.log10(A)) < 0.1
print("LAB 4 PASS")
```
Real output: `samples=100000  f_est=123.413 kHz  power=-1.94 dBFS  (expected 123.4 kHz, -1.94 dBFS)` → pass. Note the **cast to `float32` before subtracting 127.5** (P08 §8.2).

**Lab 5** — expected diagnosis: `ERROR: AddressSanitizer: heap-buffer-overflow ... WRITE of size 4 ... asan.cpp:4`. The loop `for (int i = 0; i <= 4; ++i) p[i] = i;` writes `p[4]` past the end of a 4-element array. Without the sanitizer the write lands in unchecked memory and the program prints `p[4] = 4` and exits with status 0 — **undefined behaviour is silent unless something checks for it**.

**Lab 6** — expected table (your plain-counter numbers will differ run to run; mine were 5,000,000 / 6,476,316 / 5,381,016 / 6,739,981 over four runs): plain `volatile int` **< 10,000,000**; `std::atomic<int>` **10,000,000**; `std::mutex` **10,000,000**. Explanation: `counter = counter + 1` is load–add–store; a pre-empted thread's store overwrites the other's update; `volatile` only prevents the compiler from optimising the variable away — it gives no atomicity or ordering.

**Lab 7**
```python
import struct

def pack_hdr(poll, sn):
    return struct.pack('>H', (poll & 1) << 15 | (sn & 0x0FFF))

def unpack_hdr(b):
    (w,) = struct.unpack('>H', b)
    return (w >> 15) & 1, w & 0x0FFF

h = pack_hdr(1, 2500)
print(h.hex(), unpack_hdr(h))
(w_le,) = struct.unpack('<H', h)                        # a wrong (little-endian) reader
print("little-endian misreading:", (w_le >> 15) & 1, w_le & 0x0FFF)
assert h.hex() == "89c4" and unpack_hdr(h) == (1, 2500) and (w_le & 0x0FFF) == 1161
print("LAB 7 PASS")
```
Real output: `89c4 (1, 2500)`, `little-endian misreading: 1 1161`, `LAB 7 PASS`.

---

## If you passed

You have the mathematical, numerical and tooling base the curriculum assumes. Open `RF-Software_Engineering-Curriculum/README.md`, begin with **Lesson 1 — Complex Numbers and Phasors**, and keep these notes next to you as a reference: the **dB table (P01)**, the **I/Q derivation (P07 §7.8)**, the **toolbox `rf_utils.py` (P09 §9.11)**, and the **terminology table (P12 §12.3)**.

## If you didn't

That is useful information, not a verdict. Use the remediation map, rerun the labs you failed, and retake only the failing sections after a few days. Most gaps close in an evening once you've *typed and run* the code rather than read it.

---

*End of the prerequisite notes. Everything in this folder is independent of your college Wireless Communication notes except where P12 and the cross-references say otherwise.*
