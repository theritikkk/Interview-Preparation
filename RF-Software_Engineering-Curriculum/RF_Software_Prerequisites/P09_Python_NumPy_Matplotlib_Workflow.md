# P09 — Python / NumPy / Matplotlib Workflow for Signal Processing

*Prerequisite module 9 · ⏱ about 6–8 hours (most of it hands-on) · assumes basic Python (variables, loops, functions) and P01–P08 · every output below was produced by running the code with **Python 3.12, NumPy 2.4, SciPy 1.17, Matplotlib 3.10**.*

> **Where this unlocks the curriculum.** The lessons' code blocks are overwhelmingly NumPy: a scan counted `np.random.randn` 25×, `np.random.randint` 11×, `np.fft.fft` 13×, plus `scipy.signal.firwin/lfilter` and `scipy.special.erfc`; Matplotlib appears only in Lessons 1 and 7, so the emphasis is on numbers and checks rather than plots — and several lessons (for example 3 and 5) explicitly ask you to **predict a result before running the code**. Every lesson from Lesson 1 onward assumes this workflow, and some C++ lessons measure themselves against NumPy prototypes (Lesson 22's interview scenario, for example). This module makes the toolkit automatic.

---

## 9.1 Why Python + NumPy for radio work

* **Python** = fast to write, easy to inspect. **NumPy** = fixed-type arrays and loops written in C, so numeric code often runs 100× or more faster than pure Python loops (≈ 290× in the measured example in §9.5).
* **Typical workflow:** prototype an algorithm in NumPy → check it against theory (e.g. BER vs the Q-function, P05) → *then* port to C++/embedded, using the NumPy output as the reference to compare against.
* **Always reproducible:** fixed random seeds, recorded versions, saved test vectors.

**Environment (do once):**
```bash
python3 -m venv rf-env && source rf-env/bin/activate
pip install numpy scipy matplotlib jupyter
pip freeze > requirements.txt        # record exact versions
```
Use **scripts** (`python3 exp.py`) for anything you want to repeat or put under Git; use **Jupyter** for exploration. *Version caution (verified):* in NumPy 2.x `np.trapz` is gone — use `np.trapezoid` (P03); old tutorials may fail on that.

---

## 9.2 The `ndarray`: shape, dtype, indexing 🔥

An `ndarray` is a block of memory holding elements of **one dtype**, viewed with a **shape**.

```python
x = np.arange(12).reshape(3, 4)
print(x.shape, x.ndim, x.dtype, x.size, x.itemsize, x.nbytes)
# (3, 4) 2 int64 12 8 96
```
**Creating:** `np.zeros`, `np.ones`, `np.arange(start, stop, step)`, `np.linspace(a, b, n)` (includes the endpoint), `np.array([...])`, `np.eye(n)`.

**Indexing & slicing:** `x[i]`, `x[a:b]`, `x[::2]` (every 2nd), `x[0::2]` / `x[1::2]` (**de-interleave I and Q**: `[0,2,4,6]` and `[1,3,5,7]` for `arange(8)`, verified). **Boolean masks:** `v[v % 3 == 0]` → `[0 3 6 9]`.

### Views vs copies — the #1 NumPy surprise 🔥
* **Slices and reshapes are *views*** — they share memory with the original. *Verified:* `a = np.arange(10); v = a[2:5]; v[0] = 99` changes `a[2]` to **99**; `np.shares_memory(a[::2], a)` is **True**; `a[2:5].base is a` is True; `reshape` also shares.
* **Fancy (list/boolean) indexing and `.copy()` make copies.** *Verified:* `c = a[[2,3,4]]; c[0] = -1` leaves `a[2]` unchanged (**99**); `np.shares_memory(a[[0,2,4]], a)` is False.

*Meaning:* editing a slice in place edits the source array — great for speed, dangerous if unintended. When in doubt, `.copy()`.

---

## 9.3 dtypes and promotion 🔥 (ties to P08)

| Default | What you get |
|---|---|
| `np.array([1,2,3])` | `int64` |
| `np.array([1., 2])` | `float64` |
| `np.array([1j])` | `complex128` |

**Division:** `7/2 = 3.5` (true division), `7//2 = 3` (floor), `np.array([7])/2 = [3.5]`.

**Promotion — verified in NumPy 2.4:**
* `int8 + int16 → int16`; `uint8 + int8 → int16`; `float32 + float64 → float64`.
* **`complex64 × Python float → complex64`** (Python scalars are "weak" and keep the array's precision) but **`complex64 × np.float64` scalar or array → `complex128`** — a silent *doubling* of memory and time if you mix in float64 by accident.

**Memory (verified):** 10⁶ samples take **16 MB as `complex128`** but **8 MB as `complex64`** — a 20 Msps capture is 160 MB/s vs 80 MB/s (P08 §8.3).

**Overflow:** `uint8` 200 + 100 = **44** (wraps); cast first: `x.astype(np.int32) + 100 = 300` (P08 §8.2).

---

## 9.4 Complex numbers in NumPy 🔥

```python
z = np.array([3+4j, 1j, -1])
np.abs(z)               # [5. 1. 1.]
np.rad2deg(np.angle(z)) # [ 53.13  90.  180.]
np.conj(z)              # [3.-4.j  0.-1.j -1.-0.j]
z.real, z.imag          # [ 3. 0. -1.]  [4. 1. 0.]
np.exp(1j*np.arange(4)*np.pi/2)   # [1, j, -1, -j]  (rotating phasor, P02/Lesson 1)
```
* **Power** of a complex signal: `np.mean(np.abs(x)**2)` — *verified* 1.0 for a unit-amplitude phasor. (`np.vdot(x, x).real` gives the *sum* faster, P04.)
* **Square roots:** `np.sqrt(-1.0)` returns **`nan`** (with a warning); `np.sqrt(-1+0j)` returns **`1j`**. Make data complex *before* taking roots of possibly-negative values.
* **Phase unwrapping (verified):** `np.angle` wraps to $(-\pi,\pi]$: the phase of a steadily rotating phasor shows jumps (`0, 1.57, 3.14, −1.57, …`); `np.unwrap` restores the continuous phase — final value **6.0π** for 3 full turns (P03 §3.5).

---

## 9.5 Vectorisation and broadcasting 🔥

**Rule:** *avoid Python loops over samples; use whole-array operations.*

*Verified timing* (1,000,000 `complex64` samples, energy computation; your machine will differ):

| Method | Time |
|---|---|
| Python `for` loop (200 k samples, scaled to 10⁶) | **≈ 0.59 s** |
| `np.sum(np.abs(x)**2)` | **≈ 2.1 ms** (≈ 290× faster) |
| `np.vdot(x, x).real` | ≈ 1.7 ms |

(The two vectorised results, 1999310 and 1999298, differ in the 6th digit — float32 accumulation order, P08 §8.5.)

### Broadcasting
NumPy aligns shapes from the **right**; a dimension of size 1 is stretched. Result shape of `(N,1) op (1,N)` is `(N,N)`:

```python
N = 8; n = np.arange(N)
W = np.exp(-2j*np.pi*n[:, None]*n[None, :]/N)   # DFT matrix, shape (8, 8)
np.allclose(W @ x, np.fft.fft(x))               # True   (verified)
```
This also shows **the DFT is a matrix–vector product** (P04) whose rows are complex exponentials (P03). `np.outer([1,2],[10,20,30])` → `[[10 20 30],[20 40 60]]`.

**In-place operations** (`x += y`, `np.multiply(a, b, out=a)`) avoid temporary arrays on big captures.

---

## 9.6 The FFT workflow: get the frequency axis right 🔥

```python
fs = 1000; N = 1000; t = np.arange(N)/fs
x = np.exp(2j*np.pi*100*t)                       # complex tone at +100 Hz
X = np.fft.fftshift(np.fft.fft(x))               # put 0 Hz in the middle
f = np.fft.fftshift(np.fft.fftfreq(N, 1/fs))     # matching frequency axis in Hz
print(f[np.argmax(np.abs(X))])                   # 100.0
```
**Verified facts (all from running the code):**
* `np.fft.fftfreq(8, 1/8)` returns **`[0, 1, 2, 3, −4, −3, −2, −1]`** — *unshifted* order: non-negative frequencies first, then negative. `fftshift` reorders to −fs/2 … +fs/2.
* A complex tone at **−100 Hz** peaks at **−100 Hz**; a **real cosine** shows **two** peaks at **±100 Hz** (P02 §2.6 — the ±f ambiguity, solved by complex IQ).
* **Scaling:** `fft` is unnormalised; **Parseval** (P06 §6.6) holds as $\sum|x|^{2}=\tfrac1N\sum|X|^{2}$ — *verified* 610.485 on both sides.
* **Leakage (P03 §3.7, Lesson 3):** a tone at **100 Hz** (exactly 100 cycles in the window) is one clean bin; the far-bin level is at the numerical floor. A tone at **100.5 Hz** (non-integer cycles) loses **3.92 dB** at the peak (scalloping) and leaks to **−58.2 dB** in far bins with a rectangular window, but only **−163.9 dB** with a **Hann window** (`np.hanning(N)`), which tapers the edges.

---

## 9.7 Random numbers and reproducibility 🔥

Two APIs:
* **Legacy (used by the lessons):** `np.random.randn(n)`, `np.random.randint(0, 2, n)`; global state, seed with `np.random.seed(5)`.
* **Modern `Generator` (recommended for new code):** `rng = np.random.default_rng(5)`; `rng.standard_normal(n)`, `rng.integers(0, 2, n)`.

*Verified:* both give identical results when re-seeded with the same seed, and a `default_rng()` without a seed differs from the seeded one. **The lessons mostly don't seed** (only Lesson 33 does) — so *your* numbers will differ from the printed ones in noise-dependent examples; seed to compare reliably, and rely on statistics (P05) rather than exact digits.

**Complex Gaussian noise of unit power** (P05 §5.7):
```python
w = (rng.standard_normal(n) + 1j*rng.standard_normal(n)) / np.sqrt(2)   # E|w|^2 = 1  (verified 0.99997)
```

---

## 9.8 Reading and writing sample files 🔥 (extends P08 §8.3)

```python
sig.tofile("x.cf32")                                   # raw complex64, no header
chunk = np.fromfile("x.cf32", dtype=np.complex64, count=1000, offset=8*5000)  # chunk of 1000 samples at index 5000
mm = np.memmap("x.cf32", dtype=np.complex64, mode="r") # lazy view of a big file
np.save("s.npy", sig); sig2 = np.load("s.npy")         # self-describing (dtype+shape)
```
*Verified:* a 100,000-sample `complex64` file is exactly **800,000 bytes** (8 bytes/sample); the chunk read equals the in-memory slice; the `memmap` slice equals the original; `.npy` round-trips exactly. *Meaning:* use `count`/`offset`/`memmap` for captures larger than RAM; raw files carry **no header**, so record sample rate, dtype and centre frequency elsewhere (Lesson 17's SigMF metadata).

---

## 9.9 Matplotlib essentials (three plots you will make constantly)

```python
import matplotlib
matplotlib.use("Agg")            # no display needed; save to a file
import matplotlib.pyplot as plt

fs = 1e6; t = np.arange(2048)/fs
x = np.exp(2j*np.pi*100e3*t) + 0.1*(rng.standard_normal(2048) + 1j*rng.standard_normal(2048))

fig, ax = plt.subplots(1, 3, figsize=(12, 3.2))
ax[0].plot(t[:100]*1e6, x.real[:100], label="I"); ax[0].plot(t[:100]*1e6, x.imag[:100], label="Q")
ax[0].set_xlabel("time (µs)"); ax[0].legend()                                  # waveform
f, s_db = spectrum_db(x, fs)                                                    # helper from §9.11
ax[1].plot(f/1e3, s_db); ax[1].set_xlabel("frequency (kHz)"); ax[1].set_ylabel("dB rel. peak")  # spectrum
ax[2].scatter(x.real, x.imag, s=2); ax[2].set_aspect("equal"); ax[2].set_xlabel("I"); ax[2].set_ylabel("Q")  # constellation
fig.tight_layout(); fig.savefig("overview.png", dpi=80)
```
*Verified:* the script runs headless, writes a valid PNG (I inspected it), and the spectrum peak sits at **100.1 kHz** (bin width 488 Hz) about 50 dB above the noise floor. How to read it: I and Q are the same tone **90° apart** (P02); the spectrum has one line at **+100 kHz** (a complex tone has no mirror image); and the constellation of a pure tone is a **ring** — at 100 kHz sampled at 1 Msps there are 10 samples per cycle, so the ring shows up as **10 noisy clusters**. **Habits:** always label axes **with units**, plot spectra in **dB relative to the peak** (P01), use `set_aspect("equal")` for constellations (a squashed IQ plot hides imbalance — P07), and save figures with the script that made them.

---

## 9.10 Testing numerics: prove it, don't eyeball it 🔥

Several lessons (for example 3 and 5) ask you to **predict** a result before running the code. Build that habit:

1. **Compare to theory.** BPSK/QPSK BER vs the Q-function (P05): *verified* QPSK at $E_s/N_0=6$ dB, $10^6$ symbols: **simulated 0.02303 vs theory 0.02301**.
2. **Check invariants.** Parseval (energy preserved), `np.allclose(W@x, np.fft.fft(x))`, the DFT peak at the expected bin, power unchanged by a pure phase rotation.
3. **Use tolerances, never `==`.** `np.sin(np.pi) == 0` is **False** (it is 1.2×10⁻¹⁶); `np.allclose(np.sin(np.pi), 0, atol=1e-12)` is **True**.
4. **Seed and fix inputs** so a failure is repeatable; test tiny cases you can do by hand.
5. **Use `assert`** in scripts: `assert abs(measured_snr - target) < 0.2`.
6. **Time** with `time.perf_counter()` (or `%timeit` in Jupyter) before and after an optimisation.

---

## 9.11 A starter toolbox (`rf_utils.py`) — verified

```python
import numpy as np

def db10(x, eps=1e-30):
    """Power ratio -> dB (safe at 0)."""
    return 10*np.log10(np.maximum(np.asarray(x, dtype=float), eps))

def db20(x, eps=1e-15):
    """Amplitude ratio -> dB (safe at 0)."""
    return 20*np.log10(np.maximum(np.abs(x), eps))

def avg_power(x):
    """Mean |x|^2 of real or complex samples."""
    return np.mean(np.abs(x)**2)

def spectrum_db(x, fs, window="hann"):
    """Two-sided spectrum of complex samples. Returns (freq_Hz, dB relative to peak)."""
    n = len(x)
    w = np.hanning(n) if window == "hann" else np.ones(n)
    X = np.fft.fftshift(np.fft.fft(x * w))
    f = np.fft.fftshift(np.fft.fftfreq(n, 1/fs))
    mag = np.abs(X)
    return f, db20(mag / mag.max())

def awgn(x, snr_db, rng):
    """Add complex white Gaussian noise so the sample SNR is snr_db."""
    p_noise = avg_power(x) / 10**(snr_db/10)
    sigma = np.sqrt(p_noise/2)              # N0/2 per real dimension (P05)
    noise = sigma*(rng.standard_normal(len(x)) + 1j*rng.standard_normal(len(x)))
    return x + noise
```
*Verified:* `spectrum_db` places a 100 kHz tone's peak at 100.098 kHz (0.0 dB); `awgn` targets of 0, 10, 20 dB measured **−0.01, 10.07, 20.02 dB**; `db20([1, 0.1, 0])` → `[0, −20, −300]` (the floor prevents `-inf`); `db10(0) = −300`. Save this as your own `rf_utils.py` and import it in every exercise — the same functions will serve P05–P08 exercises and Lessons 1–7.

---

## 9.12 Project hygiene (small habits that pay off)

* **One experiment per script**, with a `main()` and an `if __name__ == "__main__":` guard; functions with docstrings; **units in names** (`fs_hz`, `fc_hz`, `snr_db`, `n_samples`).
* **No global mutable state** (pass `rng` in, as `awgn` does); no magic numbers.
* **Git:** commit after each working experiment; keep `requirements.txt`; don't commit big capture files.
* **Folder layout:** `src/` (functions), `experiments/` (scripts), `data/` (small test vectors), `figures/`.
* **Keep a golden-reference habit:** for any algorithm you will later port, save the NumPy input and output to files (`.npy` or raw) so a C++ implementation can be compared with a tolerance — a habit that will pay off when you reach the C++ lessons (22 onward).

---

## 9.13 Common mistakes ⚠️
1. **Mutating a slice** and changing the original (view vs copy).
2. **Mixing `float64` into a `complex64` pipeline** — silent doubling of memory/time.
3. **`np.sqrt` of negative reals → `nan`.**
4. **Python loops over millions of samples.**
5. **Forgetting `fftshift`/using the wrong frequency axis** — peaks appear at the wrong frequency, or negative frequencies look "wrapped".
6. **Reading the wrong dtype/endianness** from a raw file (P08) — garbage spectrum.
7. **Not seeding** when comparing runs; or seeding inside a loop so every trial is identical.
8. **Comparing floats with `==`.**
9. **Plotting without units or in linear power** — use dB relative to the peak.
10. **Unwrapped phase** differenced directly (P03).
11. **Normalisation errors:** unnormalised FFT vs window gain (`w.sum()`) when reading absolute levels.
12. **Using integer arithmetic accidentally:** `uint8` samples must be cast before subtracting the offset (P08).

---

## 9.14 Practice problems (answers at the end)

1. Predict the shape of `a[:, None] * b[None, :]` for `a` of length 5 and `b` of length 3. What is `np.outer(a, b).shape`?
2. `x = np.arange(10); y = x[2:6]; y[:] = 0`. What does `x` become? How would you avoid changing `x`?
3. What is the dtype of `np.ones(4, np.complex64) * np.float64(2.0)`? Of `... * 2.0`?
4. A complex tone has frequency $+250$ Hz at $f_s=2000$ Hz, $N=2000$. At which FFT bin index (unshifted) does the peak occur? And for $-250$ Hz?
5. Write one line that returns the average power of a complex array `x` in dBm assuming samples are in volts into 50 Ω *(hint: P07 §7.2)*.
6. Using `awgn`, add noise to a unit-power signal at 15 dB SNR. What noise variance per real dimension is used?
7. Read samples 1,000–1,009 from a 2 GB `cf32` file without loading the file. Which function(s)?
8. You compute `np.sin(2*np.pi*np.arange(8)/8) == 0`. Why is the result not all True for the zero crossings, and what should you use instead?
9. Why does `np.fft.fft` of a real cosine show two peaks, and what does `fftshift` change?
10. Design an `assert` that checks a BPSK simulation at $E_b/N_0=4$ dB is within 10 % of $Q(\sqrt{2E_b/N_0})$ using $10^{6}$ bits. Roughly how many errors do you expect (P05)?
11. *Think:* two engineers run "the same" noisy experiment and get slightly different BER curves. What are the three most likely explanations and fixes?

## 9.15 Active recall
1. State the difference between a view and a copy, with an example of each. 2. List the default dtypes and the NumPy 2 promotion rule for `complex64 × float64`. 3. How do you de-interleave IQ with slicing? 4. Why is vectorising 100–300× faster? 5. State the broadcasting rule. 6. Write the FFT + `fftshift` + frequency-axis recipe. 7. What does Parseval look like for an unnormalised FFT? 8. How do you seed legacy vs modern RNGs? 9. How do you read part of a huge file? 10. Name three habits for checking numerical code. 11. Why use dB-relative-to-peak spectra?

## 9.16 Self-check — ready for P10 if you can…
- [ ] create, slice and reshape arrays and predict view/copy behaviour;
- [ ] predict dtypes after mixed arithmetic;
- [ ] compute a correctly labelled spectrum of an IQ array from scratch;
- [ ] generate seeded noise at a target SNR and verify it;
- [ ] read a binary IQ file in chunks and save a PNG headlessly;
- [ ] write `assert`s comparing a simulation to theory.

---

## ✅ Answer key

1. Shape **(5, 3)**; `np.outer(a, b).shape` is also **(5, 3)** (for 1-D inputs). 2. `x` becomes `[0 1 0 0 0 0 6 7 8 9]` (slice is a view); avoid with `y = x[2:6].copy()`. 3. `* np.float64(2.0)` → **complex128**; `* 2.0` → **complex64**. 4. Bin = $f\,N/f_s=250\times2000/2000=\mathbf{250}$; $-250$ Hz appears at index $N-250=\mathbf{1750}$ (unshifted ordering). 5. `10*np.log10(np.mean(np.abs(x)**2)/50/1e-3)` (power = $|x|^2/R$ in watts, ÷ 1 mW for dBm). 6. $P_n=P_x/10^{1.5}=0.0316P_x$; for $P_x=1$: $\sigma^{2}_{\text{per dim}}=0.0316/2=\mathbf{0.0158}$ ($\sigma=0.126$). 7. `np.memmap(...)[1000:1010]` or `np.fromfile(..., count=10, offset=8*1000)`. 8. Floating-point rounding: `sin(2π)` ≈ −2.4×10⁻¹⁶, not exactly 0; use `np.isclose`/`np.allclose` with an absolute tolerance. 9. A real cosine is the sum of $+f$ and $-f$ complex exponentials (P02); `fftshift` only **reorders** bins so 0 Hz is centred — it changes no values. 10. `assert abs(ber_sim - Q)/Q < 0.10`; at 4 dB, theory ≈ $1.25\times10^{-2}$ ⇒ about **12,500 errors** in $10^{6}$ bits (relative uncertainty $\approx1/\sqrt{12500}\approx0.9\%$ — far inside 10 %). 11. (a) **Different random seeds/noise draws** → seed both; (b) **too few errors** (< 100) → simulate more bits (P05 §5.9); (c) **different code paths**, e.g. float32 vs float64, a missing `/2` in the noise variance, or different window/normalisation → compare against the analytic curve.

---

*Next: **P10 — C++ readiness** (compiling and running, types, pointers/references, stack vs heap, `std::vector`, `std::complex`, RAII, `const`, build tools, and the memory habits the performance lessons assume).*
