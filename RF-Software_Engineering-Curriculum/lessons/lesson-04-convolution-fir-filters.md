[← Table of Contents](../README.md)

[← Lesson 3](lesson-03-dtft-dft-fft.md)

---

# Lesson 4 — Convolution, Impulse Response, and Your First FIR Filter

*Stage 1→2 bridge. This is where filtering stops being a black box, and where Lesson 3's spectral-leakage debugging exercise gets its full resolution.*

### 1. Intuition

Every real system — a filter, a wireless channel, an antenna, a microphone — takes a signal in and produces a transformed signal out. That sounds like it could be arbitrarily complicated to characterize. Here's the remarkable fact: if the system is **linear** (scaling and adding inputs scales and adds outputs the same way) and **time-invariant** (it behaves the same today as tomorrow — no clock-dependent weirdness), then its *entire* behavior, no matter how complex internally, is fully captured by a single signal: how it responds to one sharp "tap" — a unit impulse. Once you know that one response, you know the system's response to literally any input, via an operation called convolution. This is why FIR filters are just "a list of numbers" — that list *is* the impulse response, and it's a complete description.

### 2. The engineering problem that forces this

Lesson 2 needed an anti-aliasing filter before the ADC and never built one. Lesson 3's debugging exercise needed a "window" to fix spectral leakage and never built one either. Both of those are *specified* in the frequency domain ("keep frequencies below X, attenuate everything above") but need to be *implemented* as a finite sequence of numbers you multiply-and-accumulate against incoming samples. Convolution is exactly the bridge between those two worlds — and by the end of this lesson you'll build an actual filter, not just describe one.

### 3. Mathematical foundation

For an LTI system with impulse response $h[n]$, the output for any input $x[n]$ is the **convolution**:
$$y[n] = x[n] * h[n] = \sum_{k=-\infty}^{\infty} x[k]\,h[n-k]$$
An **FIR** (finite impulse response) filter has $h[n]$ nonzero for only $M$ taps, so the sum becomes finite and directly implementable:
$$y[n] = \sum_{k=0}^{M-1} h[k]\,x[n-k]$$

### 4. Formal definition

The **unit impulse** $\delta[n]$ is 1 at $n=0$ and 0 elsewhere. The **impulse response** $h[n]$ is defined as the system's output when the input is $\delta[n]$. **Linearity**: scaling/summing inputs scales/sums outputs identically. **Time-invariance**: a shifted input produces an identically-shaped, equally-shifted output. The **convolution theorem** (this is where Lesson 3 pays off directly): convolution in time equals multiplication in frequency —
$$Y(e^{j\omega}) = X(e^{j\omega})\,H(e^{j\omega})$$
— meaning you can filter a signal two mathematically equivalent ways: convolve in time, or FFT → multiply → IFFT. For long filters, the second approach (fast convolution) is dramatically cheaper, for exactly the $O(N\log N)$ reasons derived last lesson.

### 5. Derivation (why convolution is forced, not chosen)

Any discrete signal can be written as a sum of scaled, shifted impulses — the **sifting property**:
$$x[n] = \sum_{k=-\infty}^{\infty} x[k]\,\delta[n-k]$$
(This is just saying "a signal is the sum of its own samples, each one a spike at the right place, scaled to the right height" — true by construction, not an assumption.) Now apply the system, call it $T\{\cdot\}$, to both sides:
$$T\{x[n]\} = T\Big\{\sum_k x[k]\,\delta[n-k]\Big\}$$
By **linearity**, $T$ passes through the sum and the constants $x[k]$:
$$= \sum_k x[k]\,T\{\delta[n-k]\}$$
By **time-invariance**, the response to a *shifted* impulse $\delta[n-k]$ is just the impulse response, shifted the same way: $T\{\delta[n-k]\} = h[n-k]$. Substituting:
$$y[n] = \sum_k x[k]\,h[n-k]$$
That's the convolution sum — it isn't a clever trick someone invented, it's the *only possible consequence* of a system being linear and time-invariant. Every FIR/IIR filter, every wireless multipath channel, every matched filter you'll build later in this curriculum is convolution for exactly this reason.

### 6. Visualization

Convolution as "flip, slide, multiply, accumulate":

```
x:      [x0  x1  x2  x3  x4]
h (flipped, sliding across x):
step n:      [h2 h1 h0]
                  ↓
y[n] = h0*x[n] + h1*x[n-1] + h2*x[n-2]   (for a 3-tap filter)
```

At each position, flip $h$, overlap it with $x$, multiply pointwise, and sum — that single number is one output sample. Slide by one, repeat.

### 7. Worked example

A 3-tap moving-average filter: $h = [\tfrac13, \tfrac13, \tfrac13]$. Convolve with $x = [1, 5, 1, 5, 1]$:
$y[0] = \tfrac13(1) = 0.33$, $y[1] = \tfrac13(5+1)=2.0$, $y[2]=\tfrac13(1+5+1)=2.33$, $y[3]=\tfrac13(5+1+5)=3.67$, ... — the sharp oscillation in $x$ gets smoothed toward the local average, exactly what "low-pass filter" means intuitively: it can't follow rapid (high-frequency) swings.

### 8. Code

```python
import numpy as np

x = np.array([1, 5, 1, 5, 1, 5, 1], dtype=float)
h = np.array([1/3, 1/3, 1/3])

y_manual = np.array([np.dot(h, x[max(0,n-2):n+1][::-1])
                      for n in range(len(x))])   # crude direct version, edge-truncated
y_np = np.convolve(x, h, mode='same')

print("manual (approx):", np.round(y_manual, 2))
print("np.convolve:    ", np.round(y_np, 2))
```

### 9. Experiment — building a real low-pass FIR, and closing the loop on windowing

The *ideal* brick-wall low-pass filter (passes everything below cutoff $f_c$, blocks everything above, perfectly) has an impulse response that is a **sinc function** — infinite in length, since a perfectly sharp cutoff in frequency requires infinite extent in time (this is the same time/frequency trade-off underlying the whole sampling-and-transform story so far). You can't implement infinite. So you **truncate** it to $M$ taps — and truncating a signal is *exactly* multiplying it by a rectangular window. Run this:

```python
import numpy as np

M = 51                      # number of taps (odd, for symmetry)
fc = 0.1                    # cutoff as a fraction of sample rate
n = np.arange(M) - (M-1)/2
h_ideal = np.sinc(2*fc*n)                    # infinite ideal filter, truncated to M taps

h_rect = h_ideal * np.ones(M)                # rectangular window = no taper (the naive version)
h_hamming = h_ideal * np.hamming(M)          # tapered window

import numpy.fft as fft
H_rect = np.abs(fft.fft(h_rect, 2048))
H_hamming = np.abs(fft.fft(h_hamming, 2048))
# plot 20*np.log10(H_rect) and 20*np.log10(H_hamming) — compare ripple and rolloff
```

### 10. Expected output

Both frequency responses show a low-pass shape, but `H_rect` (rectangular-windowed, i.e. abrupt truncation) has *visible ripple* in both passband and stopband — oscillation that never fully dies out, called the **Gibbs phenomenon**. `H_hamming` has a much cleaner, smoother rolloff with far less ripple, at the cost of a slightly wider transition band. This is the exact same mechanism as Lesson 3's spectral leakage: an abrupt edge (there, the end of your FFT window; here, the truncation of an infinite sinc) creates a discontinuity, and a discontinuity spreads energy across frequencies. A taper fixes both problems for the same underlying reason.

### 11. Debugging exercise

Symptom, no cause given: you design a low-pass filter meant to cleanly pass everything below 1 kHz and block everything above. When you plot its actual frequency response, instead of a clean "brick wall," you see noticeable ripple in the passband, ripple in the stopband, and a gradual (not instant) transition rather than a sharp cutoff.

Before reading on — what would you check, and what design change would you try? (You now have everything you need to answer this from §9 alone.)

**Root cause (revealed):** this is the rectangular-window truncation problem from §9 — an ideal brick-wall filter needs infinite taps, and cutting it off abruptly at $M$ taps is a rectangular window, which causes Gibbs-phenomenon ripple. The fix is the same fix as Lesson 3's leakage problem: apply a smoother taper (Hamming, Hann, Blackman) to the truncated impulse response instead of cutting it off sharply. The trade-off is real and unavoidable: a smoother taper reduces ripple but widens the transition band — there is no window that gives you both a perfectly sharp cutoff and zero ripple with a finite number of taps.

### 12. Common mistakes

- Confusing **convolution** with **correlation** — nearly the same formula, but correlation doesn't time-reverse $h$. This distinction becomes critical later when you build a matched filter (Stage 3) and get the sign/direction wrong.
- Not knowing `np.convolve`'s output-length modes: `'full'` (length $N+M-1$), `'same'` (length $N$, centered), `'valid'` (only fully-overlapping samples) — picking the wrong one silently shifts or truncates your output and produces confusing off-by-a-few-samples bugs.
- Assuming any FIR filter has zero phase distortion — only true for **linear-phase** designs, which requires symmetric (or antisymmetric) taps. Non-symmetric FIR filters distort phase, which matters enormously once you're doing synchronization and channel estimation.

### 13. Real RF/cellular application

Every receiver chain you'll ever work with is full of FIR filters built exactly this way: channel-selection filtering, anti-aliasing before decimation, pulse-shaping filters (root-raised-cosine, arriving in Stage 3, is a windowed-sinc-family design), and matched filters for synchronization. The fast-convolution idea from §4 (FFT → multiply → IFFT instead of direct time-domain convolution) is a real, standard technique in real-time DSP pipelines once filter lengths get long — directly relevant when you build the real-time embedded pipeline in Stage 10.

### 14. Interview question

*"Why is convolution the mathematically correct operation for filtering, rather than, say, just multiplying two signals sample-by-sample?"* Answer from the derivation in §5 — linearity and time-invariance alone *force* the convolution sum; it was never a design choice.

### 15. Active recall

1. What two properties of a system, together, guarantee that convolution describes its behavior completely?
2. Why does the ideal brick-wall low-pass filter require infinite taps?
3. What's the practical trade-off when you switch from a rectangular window to a Hamming window in filter design?

### 16. Mini assignment

Using the code in §9, build a 21-tap low-pass filter at $f_c = 0.15$ with a rectangular window and again with a Hamming window. Compare the two: measure (by eye, from the plot) roughly how much wider the Hamming version's transition band is, and roughly how much lower its stopband ripple is.

### 17. Exit test

1. Write the convolution sum for an FIR filter with $M=4$ taps, fully expanded (no summation notation).
2. True or false, with a one-line reason: "A longer FIR filter (more taps) always gives a sharper frequency-domain cutoff."
3. In one sentence: why is the Gibbs-phenomenon ripple in §10 the *same underlying phenomenon* as the spectral leakage from Lesson 3, not a coincidence?

---

**Next:** Lesson 5 moves into Stage 2 properly: multirate DSP — interpolation, decimation, and why you basically never actually build that infinite ideal filter in practice, but always work with a controlled approximation of it.

---

[← Lesson 3](lesson-03-dtft-dft-fft.md)  ·   [Table of Contents](../README.md)  ·  [Lesson 5 →](lesson-05-multirate-dsp.md)