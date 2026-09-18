[← Table of Contents](../README.md) · [← Lesson 12](lesson-12-ofdm-transceiver.md)

---

# Lesson 13 — Timing Offset and Carrier Frequency Offset (CFO)

*Stage 4 — Wireless Communications, continued. Lesson 12 proved OFDM's orthogonality under perfect assumptions. This lesson is about what breaks when a real receiver can't quite meet them — and the elegant fix hiding inside the cyclic prefix itself.*

### 1. Intuition

Lesson 12's proof that $Y[k]=H[k]X[k]$ exactly relied on two silent assumptions: you sample the FFT window at exactly the right instant, and your receiver's oscillator matches the transmitter's carrier frequency exactly. Neither is ever quite true. The receiver doesn't know in advance exactly when a symbol starts, and every real oscillator has some tolerance — a few parts-per-million, which at gigahertz carrier frequencies is real, Hz-scale frequency error, compounded by Doppler shift from motion. Timing error and frequency error turn out to behave completely differently: one is surprisingly forgiving, the other is not, and understanding exactly why is the point of this lesson.

### 2. The engineering problem that forces this

You need to answer two separate questions with two separate tools: *when* does a symbol start (timing), and *what's the frequency error* between transmitter and receiver oscillators (CFO)? Get timing slightly wrong and — depending on exactly how wrong — you might get away with it entirely, or you might reintroduce Lesson 12's ISI catastrophe. Get frequency slightly wrong and something more insidious happens regardless of timing: subcarriers that Lesson 12 proved were perfectly orthogonal start leaking into each other — **inter-carrier interference (ICI)** — for reasons that have nothing to do with the channel at all.

### 3. Mathematical foundation

**Timing offset $\delta$ (samples):** safe, self-correcting if $0 \le \delta \le L_{cp}-(L-1)$; reintroduces ISI outside that window.

**CFO**, normalized to subcarrier spacing: $\varepsilon = \Delta f / \Delta f_{sc}$. **Integer CFO** ($\varepsilon \in \mathbb{Z}$): a clean, lossless shift of DFT bin index — correctable *after* the FFT. **Fractional CFO** ($\varepsilon \notin \mathbb{Z}$): smears energy across *all* subcarriers — must be corrected *before* the FFT, in the time domain.

### 4. Formal definition

A **timing offset** shifts which $N$ samples the receiver feeds into its FFT. A **CFO** is an uncompensated residual phasor rotation (Lesson 1) — the receiver's demodulating local oscillator doesn't exactly cancel the transmitter's carrier, leaving the baseband signal multiplied by $e^{j2\pi\Delta f\, t}$. **ICI** is energy from subcarrier $k'$ leaking into the receiver's estimate of subcarrier $k\ne k'$, caused by fractional CFO breaking the exact orthogonality Lesson 12 §4 established.

### 5. Derivation

**(a) Timing offset within the safe window is self-correcting.** If the FFT window starts $\delta$ samples late but still within $[0,\,L_{cp}-(L-1)]$, the $N$ captured samples are a *circularly shifted* version of the ideal window: $y_{shift}[n] = y_{ideal}[(n+\delta)\bmod N]$. The DFT's time-shift property (a direct consequence of the DFT's definition — shifting the input circularly multiplies every output bin by a linear phase ramp) gives:
$$Y_{shift}[k] = Y_{ideal}[k]\cdot e^{j2\pi k\delta/N} = H[k]X[k]\cdot e^{j2\pi k\delta/N}$$
Here's the payoff: the **pilots go through the identical shift**. Lesson 11's LS estimate becomes $\hat H[k] = H[k]\,e^{j2\pi k\delta/N}$ — it absorbs the timing-induced phase ramp as if it were just part of the channel. Equalizing with $Y[k]/\hat H[k]$ then cancels it out completely, automatically, with zero extra effort: $X[k]$ comes back exactly. **OFDM is remarkably timing-tolerant specifically because channel estimation quietly absorbs any timing error within the CP margin** — a sharp contrast with Lesson 9's single-carrier case, where timing had to be *exactly* right.

**(b) Fractional CFO causes ICI — and it's Lesson 3's spectral leakage again.** With CFO, the received time-domain signal is $r[n] = x[n]\,e^{j2\pi\varepsilon n/N}$. Its DFT:
$$R[k] = \sum_n x[n]\,e^{j2\pi\varepsilon n/N}\,e^{-j2\pi kn/N} = \sum_n x[n]\,e^{-j2\pi(k-\varepsilon)n/N}$$
If $\varepsilon$ is an **integer**, this collapses exactly to $X[k-\varepsilon]$ — subcarriers land on the wrong (but still individually clean) bins, fixed trivially by relabeling. If $\varepsilon$ is **fractional**, the sum does *not* collapse to one term — it becomes:
$$R[k] = \sum_{k'} X[k']\cdot S(k-k'-\varepsilon), \qquad S(x) = \frac{1}{N}\sum_{n=0}^{N-1} e^{j2\pi xn/N}$$
$S(x)$ is a **Dirichlet kernel** — exactly the same sinc-shaped leakage kernel behind Lesson 3 §11's spectral leakage, now appearing because the received samples complete a non-integer number of extra rotations within the FFT window. $S(0)=1$ but $S(x)\ne0$ for other integers when $\varepsilon$ isn't itself an integer — every subcarrier picks up nonzero contributions from every other. This is not a new phenomenon; it's Lesson 3's leakage bug, wearing a different hat.

### 6. Visualization

```
Timing offset safe window (self-correcting via Lesson 11's equalizer):
[ Lcp ][safe: 0 ≤ δ ≤ Lcp-(L-1)][  danger: ISI returns  ]

Fractional CFO — leakage across subcarriers (Dirichlet kernel, same shape as Lesson 3 §11):
subcarrier k:  ...  k-2   k-1    k    k+1   k+2  ...
leakage:            small small BIG  small small   ← every neighbor picks up a little
```

### 7. Worked example

15 kHz subcarrier spacing (LTE-like), $N=64$. A 3 kHz CFO: $\varepsilon = 3/15 = 0.2$ — fractional, causes ICI. A 15 kHz CFO: $\varepsilon = 1$ — integer, causes zero ICI, just a clean one-bin index shift, correctable after the FFT by relabeling. Same *size* of frequency error can be either harmless or damaging depending entirely on where it falls relative to the subcarrier grid.

### 8. Code

```python
import numpy as np

N, L_cp = 64, 8
X = (2*np.random.randint(0,2,N)-1).astype(complex)
x = np.fft.ifft(X)
x_cp = np.concatenate([x[-L_cp:], x])

def apply_cfo(sig, eps, N):
    n = np.arange(len(sig))
    return sig * np.exp(1j*2*np.pi*eps*n/N)

for eps in [0.0, 0.2, 0.5, 1.0]:
    rx = apply_cfo(x_cp, eps, N)[L_cp:L_cp+N]     # CFO applied, then CP removed as usual
    Y = np.fft.fft(rx)
    if eps == round(eps):                          # integer CFO: undo by relabeling
        Y = np.roll(Y, -int(round(eps)))
    err = np.mean(np.abs(Y - X)**2)
    print(f"eps={eps}: mean squared error = {err:.3f}")
```

### 9. Experiment

Run §8's sweep — error should be near zero at $\varepsilon=0$ and $\varepsilon=1$ (both cleanly correctable), and largest around $\varepsilon=0.5$ (maximal fractional leakage, halfway between bins). Separately, test timing offset by shifting the FFT window start both within and beyond $L_{cp}-(L-1)$ for a known channel — confirm §5(a)'s prediction: near-zero error throughout the safe window, then a sharp jump once you cross it, mirroring Lesson 12 §9's threshold behavior exactly.

### 10. Expected output

CFO error curve: low at integer $\varepsilon$, peaking near $\varepsilon=0.5$. Timing error curve: flat near zero inside the safe window, sharp step increase right at the boundary — direct visual confirmation that these two impairments behave completely differently, one forgiving, one not.

### 11. Debugging exercise

Symptom, no cause given: after applying a coarse CFO estimate and correcting for it, your system still isn't quite right. Each *individual* OFDM symbol's equalized constellation looks reasonably tight — but watching the average constellation phase across many *consecutive* symbols, it visibly drifts, slowly rotating over time rather than staying fixed.

What would cause a phase error that accumulates symbol-to-symbol, specifically, rather than just sitting there as a constant, or showing up as noise-like scatter within one symbol?

**Root cause (revealed):** **residual CFO** — a small leftover frequency error your coarse estimate didn't fully remove. Even a tiny residual $\Delta f_{res}$ keeps rotating the signal's phasor (Lesson 1) continuously; each OFDM symbol occupies a fixed time duration, so the *same* residual rotation accumulates by a *constant additional amount* every symbol period — a steadily growing phase, common to all subcarriers within a given symbol (distinct from the within-symbol ICI derived in §5(b), which is noise-like scatter, not a coherent drift). This effect is well known in real systems as **common phase error (CPE)**, and it's exactly why real receivers don't stop at one coarse, one-shot CFO correction — they track and correct a small residual phase, symbol by symbol, using pilots, for the whole duration of a connection.

### 12. Common mistakes

- Trying to correct **fractional** CFO *after* the FFT — once the Dirichlet-kernel leakage from §5(b) has smeared energy across bins, no post-FFT operation can cleanly un-smear it. Fractional CFO correction must happen as a **time-domain** de-rotation, applied to the raw samples *before* the FFT, using the estimated $\hat\varepsilon$: multiply by $e^{-j2\pi\hat\varepsilon n/N}$ first.
- Treating **integer** CFO the same way — it genuinely doesn't need time-domain correction; a post-FFT bin relabel is sufficient and cheaper, since §5(b) showed the integer case never smears in the first place.
- Assuming one CFO estimate at the start of a connection is sufficient — oscillator drift and Doppler change CFO slowly over time, which is exactly what §11's debugging exercise demonstrates; real systems need ongoing tracking, not a single correction.
- Confusing timing offset's self-correcting property (§5(a)) with "timing doesn't matter" in general — it only self-corrects *inside* the CP margin; step outside it and Lesson 12's ISI catastrophe returns exactly as before.

### 13. Real RF/cellular application

This lesson explains why LTE/NR cell search is a genuinely staged procedure. A particularly elegant technique — directly buildable from what you already know — exploits the cyclic prefix's own structure for **joint timing and CFO estimation**: because the CP is literally a copy of the symbol's tail, correlating the received signal against a copy of itself delayed by $N$ samples (Lesson 6's correlation, applied to the signal against *itself*) produces a peak exactly where a symbol boundary sits (timing), and the **phase** of that peak directly reveals the CFO — if a CFO of $\Delta f$ is present, the delayed self-correlation accumulates exactly $2\pi\Delta f\, N T_s$ radians of extra phase, so $\Delta f = \angle(\text{peak})/(2\pi N T_s)$, a phase-to-frequency conversion straight out of Lesson 1. This is essentially the well-known **Schmidl–Cox** approach to OFDM synchronization, and it's why PSS/SSS-based cell search in LTE/NR (Lesson 6's machinery again) is followed by exactly this kind of CP-based fine tracking, then ongoing pilot-based CPE tracking (§11) for the life of the connection.

### 14. Interview question

*"Why must fractional CFO be corrected before the FFT, in the time domain, while integer CFO can be corrected after the FFT?"* Answer from §5(b): integer CFO causes a lossless re-indexing of DFT output bins — nothing is mixed together, so relabeling after the fact recovers everything perfectly. Fractional CFO causes genuine energy leakage *across* bins, baked into the FFT computation the moment you compute it on CFO-rotated samples — once that mixing has happened, it can't be cleanly reversed; only removing the rotation from the raw samples first prevents the leakage from occurring at all.

### 15. Active recall

1. Why does a timing offset within the CP's safe margin require no separate correction step at all?
2. What's the mathematical difference between integer and fractional CFO that makes one harmless and the other not?
3. Why does residual CFO cause a phase that *accumulates* across symbols rather than staying constant?

### 16. Mini assignment

Implement the Schmidl–Cox-style estimator from §13: correlate the received signal against itself delayed by $N$ samples over a sliding window; find the peak (timing) and its phase (CFO), via $\Delta f = \angle(\text{peak})/(2\pi N T_s)$. Inject a known timing offset and CFO into a simulated signal and verify your estimator recovers both correctly.

### 17. Exit test

1. State the exact condition on timing offset $\delta$ under which it requires no correction at all.
2. True or false, with a one-line reason: "A larger CFO always causes more ICI than a smaller one."
3. Your reasoning from §11 about accumulating phase error, if you haven't sent it.

---

**Next:** Lesson 14 closes Stage 4 with **MIMO** — spatial multiplexing and diversity — where the single-antenna model used throughout Lessons 10–13 becomes a matrix equation, and Lesson 11's equalizer math generalizes directly rather than starting over.

---

[← Lesson 12](lesson-12-ofdm-transceiver.md) · [Table of Contents](../README.md) · [Lesson 14 →](lesson-14-mimo.md)