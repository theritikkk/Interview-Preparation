[← Table of Contents](../README.md) · [← Lesson 2](lesson-02-sampling-theorem-and-aliasing.md)

---

# Lesson 3 — The DTFT, the DFT, and the FFT

*Stage 1 — Signals & Systems. This is where Lesson 2's "we'll verify this spectrally" promise finally gets paid off.*

### 1. Intuition

In Lesson 2 you *predicted* aliasing with a formula — you never actually looked at "how much of each frequency is present" in a signal. The Fourier transform family is the tool that answers exactly that question: "if I try to explain this signal as a sum of pure spinning phasors (Lesson 1) at different frequencies, how much of each one do I need?" The **DTFT** answers this for an infinite sequence of discrete samples. The **DFT** is the practical, finite version you can actually run on a computer. The **FFT** is not a different transform — it's just a fast *algorithm* for computing the exact same DFT numbers.

### 2. The engineering problem that forces this

You capture a finite block of, say, N = 1024 complex IQ samples from an SDR. You want to know what frequencies are in there. The true DTFT needs an infinite sum over discrete samples and produces a continuous function of frequency — neither infinite data nor a continuous output is something a digital machine can hold. So you need a transform that takes a **finite array in** and produces a **finite array out**: that's the DFT. But computed the naive way, an N-point DFT costs O(N²) multiply-adds — for real-time cellular processing at tens of millions of samples per second, that's not a rounding error, it's the difference between real-time and useless. The FFT exists purely to solve that second, very real engineering problem.

### 3. Mathematical foundation

**DTFT** (infinite discrete sequence → continuous frequency function, periodic with period $2\pi$):
$$X(e^{j\omega}) = \sum_{n=-\infty}^{\infty} x[n]\,e^{-j\omega n}$$

**DFT** (finite N-sample sequence → finite N-point frequency array):
$$X[k] = \sum_{n=0}^{N-1} x[n]\,e^{-j2\pi kn/N}, \quad k = 0, \dots, N-1$$

**FFT**: an $O(N\log N)$ algorithm that computes exactly the values in the DFT formula above — same numbers, dramatically less arithmetic.

### 4. Formal definition

Each DFT output $X[k]$ is a **frequency bin**, spaced $\Delta f = f_s/N$ apart. Bin $k$ corresponds to frequency $k \cdot f_s/N$ for $k = 0,\dots,N/2$ (positive frequencies), while bins $k = N/2+1,\dots,N-1$ represent **negative frequencies** — this is not a quirk, it's literally the same aliasing/periodicity fact from Lesson 2 showing up directly in your output array. $k=0$ is DC.

### 5. Derivation

Two things worth actually deriving, not memorizing:

**The DFT *is* the DTFT, sampled.** Plug $\omega = 2\pi k/N$ into the DTFT formula and restrict the (now infinite) sum to $n=0,\dots,N-1$: you get exactly the DFT formula. This means computing a DFT implicitly treats your finite block of samples as one period of an infinitely repeating (periodic) signal — the DFT doesn't know your block has edges. Keep that in your back pocket; it resurfaces in §11 and again when we get to OFDM's cyclic prefix later in this curriculum.

**Why FFT beats naive DFT — the core Cooley–Tukey idea.** A direct DFT computes N outputs, each a sum of N terms: $O(N^2)$. Cooley–Tukey's insight: split $x[n]$ into even-indexed and odd-indexed samples, and notice that an N-point DFT can be built from **two** $N/2$-point DFTs plus $O(N)$ combining work (a "butterfly"):
$$X[k] = E[k] + e^{-j2\pi k/N}\,O[k]$$
where $E[k]$ is the $N/2$-point DFT of the even samples and $O[k]$ of the odd samples (both reused, via periodicity, for $k$ and $k+N/2$). Apply this split recursively ($N/2 \to N/4 \to \dots$) and the total work collapses from $O(N^2)$ to $O(N\log N)$. This recursive halving is *why* power-of-2 lengths (256, 1024, 4096...) are the natural, fastest FFT sizes — the split only lands evenly when $N$ is a power of 2. That's the answer Lesson 2 §14 deferred.

### 6. Visualization

```
time domain (N samples)          frequency domain (N bins)
x[0] x[1] x[2] ... x[N-1]  --DFT-->  X[0] X[1] X[2] ... X[N-1]
                                      DC   +f   +2f  ...  -f  (wraps to negative)
```

A single butterfly (the atomic operation an FFT is built from):
```
E[k] ---->(+)----> X[k]
       \  /
        \/
        /\
       /  \
O[k]-->(x W)-->(-)----> X[k+N/2]
        ^
        W = e^{-j2πk/N}  (a "twiddle factor")
```

### 7. Worked example

Take the 4-sample sequence $x = [1, 0, -1, 0]$ — one full cycle of a cosine sampled at 4 points. By direct DFT formula: $X[0] = 1+0-1+0 = 0$ (no DC). $X[1] = 1\cdot e^{0} + 0\cdot e^{-j\pi/2} + (-1)\cdot e^{-j\pi} + 0\cdot e^{-j3\pi/2} = 1 - (-1) = 2$. $X[2] = 1 - 0 - 1 - 0 = 0$. $X[3] = 2$ as well by symmetry (real input → conjugate-symmetric output, since a real signal's negative-frequency content mirrors its positive-frequency content — the same Hermitian symmetry from Lesson 2). Result: all energy at bin 1 (and its mirror, bin 3) — exactly right, since $x[n]$ was a single-cycle cosine.

### 8. Code

```python
import numpy as np, time

N = 1024
x = np.random.randn(N) + 1j*np.random.randn(N)

# Naive O(N^2) DFT, straight from the formula
def naive_dft(x):
    N = len(x)
    X = np.zeros(N, dtype=complex)
    for k in range(N):
        for n in range(N):
            X[k] += x[n] * np.exp(-2j*np.pi*k*n/N)
    return X

t0 = time.time(); X_naive = naive_dft(x); t_naive = time.time()-t0
t0 = time.time(); X_fft   = np.fft.fft(x); t_fft   = time.time()-t0

print("max difference:", np.max(np.abs(X_naive - X_fft)))   # ~0
print(f"naive: {t_naive:.4f}s   fft: {t_fft:.6f}s")
```

### 9. Experiment

Run the code above, then finally close the loop on Lesson 2 §9: generate `f_true=30` and `f_true=130` at `fs=100` again, take `np.fft.fft` of each, and plot `np.abs(X)` for both. You predicted with the alias formula that they'd land on the same bin — now *see* it: both spectra should show their single peak at the identical frequency bin, confirming what §5 of Lesson 2 argued algebraically.

### 10. Expected output

`max difference` between naive and FFT should be effectively zero (floating-point roundoff). The FFT should be dramatically faster than the naive loop at N=1024 — often 100x or more — and the gap grows with N. Both aliased-tone spectra should show their peak at the same bin index.

### 11. Debugging exercise

Symptom, no cause given: you generate a pure single-tone sinusoid, take its FFT, and expect one sharp spike at one bin. Instead you see a tall spike at the expected bin — but with energy visibly smeared into several neighboring bins, not a clean single line.

What would you check? (Hint: go back to §5 — the DFT implicitly assumes your N-sample block repeats forever, seamlessly. What has to be true about your tone's frequency, relative to your block length, for that repetition to actually be seamless?)

**Root cause (revealed):** this is **spectral leakage**. It happens whenever your tone completes a non-integer number of cycles within your N-sample window — e.g. 10.3 cycles instead of exactly 10. The DFT's implicit periodic assumption then stitches together a signal with a discontinuity at the wrap-around point (where cycle 10.3 abruptly jumps back to the start of cycle 0), and that artificial discontinuity is not a pure tone anymore — it has energy at many frequencies, which shows up as smearing. Fix: either make the tone's frequency land exactly on a bin (integer number of cycles in the window), or apply a **window function** before the FFT to taper the edges and suppress the discontinuity — the subject of a later DSP lesson.

### 12. Common mistakes

- Forgetting the DFT assumes *periodic* extension — this causes real bugs later when linear convolution (what you want) and circular convolution (what a frequency-domain multiply actually gives you) disagree unless you zero-pad correctly.
- Reading bin $k > N/2$ as some strange artifact instead of recognizing it as the negative-frequency content — same idea as Lesson 2's aliasing, now sitting directly in your output array.
- Forgetting to normalize/scale (by $N$ or $\sqrt{N}$, depending on convention) when comparing FFT magnitude to a signal's actual physical amplitude or power.
- Assuming FFT *requires* a power-of-2 length — it doesn't (mixed-radix and Bluestein algorithms handle arbitrary N), but power-of-2 remains simplest and fastest, which is why embedded/DSP hardware overwhelmingly uses it.

### 13. Real RF/cellular application

This is the lesson where the payoff becomes unmistakable: **OFDM — the foundation of every LTE and NR downlink and uplink you'll ever work on — is built by taking an IFFT of frequency-domain data symbols to produce the time-domain waveform that actually gets transmitted, and the receiver takes an FFT to get back to frequency-domain symbols per subcarrier.** That's not a metaphor or an analogy — it's literally how OFDM works, using exactly the math in this lesson. PSS/SSS cell-search in LTE and SSB detection in NR are also fundamentally FFT/correlation operations. Project 4 (OFDM TX/RX) in the curriculum is where you'll build this yourself.

### 14. Interview question

*"5G NR often uses an FFT size like 4096 even though the number of actually-used subcarriers for a given channel might only be around 3300. Why not use an FFT sized to match exactly?"* Reason from two angles: what did §5 say about why power-of-2 sizes are fastest? And what does "unused" FFT bins at the edges buy you in terms of guard band and out-of-band emissions? (Full answer requires OFDM material still ahead — for now, reason from what you have.)

### 15. Active recall

1. In one sentence, what's the actual difference between the DTFT and the DFT?
2. Why is the FFT not "a different, better transform" than the DFT?
3. What causes spectral leakage, mechanically?

### 16. Mini assignment

By hand: compute the 4-point DFT of $x = [1, 1, 1, 1]$ (a constant/DC signal). Predict which bin should hold all the energy *before* you compute — then verify by hand, then check with `np.fft.fft`.

### 17. Exit test

1. If $f_s = 1000$ Hz and $N=500$, what is the frequency resolution $\Delta f$ (Hz per bin)?
2. A real-valued signal's FFT output — is it symmetric, and if so, in what sense?
3. Your hypothesis from §11 — this one sets up windowing, which comes early in the next DSP-focused lesson.

---

**Next:** Lesson 4 moves from "analyzing a signal that already exists" to "designing a filter" — FIR filters, convolution, and impulse response, which is also where windowing (the fix from §11) formally enters.

---

← [Lesson 2](lesson-02-sampling-theorem-and-aliasing.md) · [Table of Contents](../README.md)