[← Table of Contents](../README.md) · [← Lesson 1](lesson-01-complex-numbers-and-phasors.md)

---

# Lesson 2 — The Sampling Theorem: Nyquist, Aliasing, and Why Complex Baseband Is Cheaper

*Stage 1 — Signals & Systems. Builds directly on Lesson 1's negative-frequency result.*

### 1. Intuition

Picture a strobe light on a spinning fan. Flash too slowly relative to the fan's spin, and the fan can appear to stand still, spin slowly, or even spin *backwards*. Your eye isn't malfunctioning — it's literally being given too few snapshots per revolution to know which way the fan actually turned. Sampling a continuous signal is the exact same problem: a sequence of discrete snapshots is fundamentally ambiguous about what happened *between* the snapshots unless you sample fast enough. The sampling theorem tells you exactly how fast "fast enough" is — and it's not the same number for a real signal as for a complex one, which is where Lesson 1 comes back in.

### 2. The engineering problem that forces this

Say you want to receive a 20 MHz-wide LTE channel sitting on a 2 GHz carrier. Naively, "sample fast enough to capture everything" sounds like it needs multi-GHz ADCs — expensive, power-hungry, and pointless, since almost all your actual *information* lives in a 20 MHz sliver, not the 2 GHz carrier itself. The fix is exactly what Lesson 1 set up: mix the signal down to complex baseband first (multiply by $e^{-j2\pi f_c t}$, a phasor spinning at the carrier rate), *then* sample. Once you're in baseband, the question becomes "how fast do I need to sample a 20 MHz-wide complex signal?" — and here's the payoff for using IQ instead of a real signal: because a complex baseband signal only occupies positive frequencies (no mirrored negative-frequency image — that's precisely what Lesson 1 showed complex representation buys you), you need a sample rate proportional to the *bandwidth*, not double it. A real signal covering the same width is forced to also represent a mirror image at negative frequencies (a real signal's spectrum is always symmetric — $X(-f) = X^*(f)$), so it effectively needs to leave room for content on both sides. That's a factor-of-2 savings in your ADC/DAC sample rate, directly traceable to the same "IQ separates +f from −f" idea from Lesson 1.

### 3. Mathematical foundation

**Nyquist–Shannon sampling theorem:** a signal band-limited to a maximum frequency $f_{max}$ must be sampled at a rate $f_s > 2f_{max}$ to be perfectly reconstructible from its samples. $f_s/2$ is called the **Nyquist frequency**. For a *complex baseband* signal occupying total bandwidth $B$ (say, from $-B/2$ to $+B/2$), the requirement relaxes to $f_s > B$ — because there's no mirror image eating into the available room.

### 4. Formal definition

**Aliasing:** when $f_s$ is too low, frequency content above the Nyquist frequency doesn't disappear — it gets *misrepresented* as a lower frequency in the sampled signal. Specifically, a tone at frequency $f$ sampled at rate $f_s$ appears, after sampling, at the **aliased frequency**:

$$f_{alias} = \left| f - f_s \cdot \text{round}\!\left(\frac{f}{f_s}\right) \right|$$

This isn't noise or distortion in the usual sense — it's a perfectly deterministic, silent substitution of one frequency for another. That's what makes it dangerous: nothing in the samples themselves tells you it happened.

### 5. Derivation

Model ideal sampling as multiplying a continuous signal $x(t)$ by a train of unit impulses spaced $T = 1/f_s$ apart: $x_s(t) = x(t)\sum_n \delta(t-nT)$. Multiplication in time is **convolution in frequency** (this is the same convolution-theorem machinery you'll formalize fully in the next DSP lesson). An impulse train in time transforms to *another impulse train in frequency*, spaced $f_s$ apart. So:

$$X_s(f) = X(f) * \sum_n \delta(f - n f_s) = \sum_n X(f - n f_s)$$

In words: sampling doesn't just capture $X(f)$ — it creates infinitely many *copies* of $X(f)$, repeated every $f_s$ Hz, all stacked on top of each other. If $X(f)$ is narrower than $f_s$, the copies sit side by side with gaps between them and you can filter out one clean copy to recover the original. If $X(f)$ is *wider* than $f_s$ (i.e., you undersampled), the copies **overlap** — and once they overlap, the overlapping energy is irreversibly summed together. You cannot un-add two overlapping spectra. That irreversibility is aliasing, and it's why the theorem is a hard inequality, not a suggestion.

Now the real-vs-complex payoff, made precise: a real signal's spectrum is Hermitian-symmetric, so if it has content up to $f_{max}$, it also has mirrored content down to $-f_{max}$ — its *occupied width* is $2f_{max}$, and the copies-spaced-by-$f_s$ argument above forces $f_s > 2f_{max}$. A complex baseband signal with no mirror image only occupies width $B$, so the same copies-must-not-overlap argument only forces $f_s > B$. Same theorem, same derivation — the factor of 2 was never a separate rule, it was just what happens when your spectrum happens to be symmetric.

### 6. Visualization

Undersampled (aliasing) case — copies of the spectrum spaced by $f_s$, overlapping because $X(f)$ is too wide relative to $f_s$:

```
        overlap region (aliasing!)
              ↓↓↓
...___/\___/\/\___/\___...    ← copies of X(f), spaced fs apart
      |<-- fs -->|
```

Properly sampled case — same copies, but $f_s$ wide enough that they sit side by side untouched:

```
...___/\_______/\_______/\___...
      |<--   fs   -->|
```

### 7. Worked example

A real signal band-limited to 10 kHz needs $f_s > 20$ kHz. A complex baseband signal occupying $-5$ kHz to $+5$ kHz — 10 kHz of total bandwidth, same as the real case above — only needs $f_s > 10$ kHz. Same information width, half the sample rate, purely because one is real and one is complex.

### 8. Code

```python
import numpy as np

fs = 100        # sampling rate, Hz — deliberately too low for what follows
t  = np.arange(0, 0.1, 1/fs)

def aliased_freq(f, fs):
    return abs(f - fs * round(f / fs))

for f_true in [30, 60, 90, 120, 150]:
    x = np.cos(2*np.pi*f_true*t)          # sample a real tone at f_true
    predicted = aliased_freq(f_true, fs)
    print(f"true={f_true} Hz  ->  predicted alias={predicted} Hz")
```

### 9. Experiment

Run the code above and, separately, plot `x` for `f_true = 30` and `f_true = 130` on the same axes using the *same* `t` array. They should look like the exact same discrete sequence of points, even though they're wildly different continuous frequencies — that's aliasing made visible: your samples genuinely cannot tell these two signals apart. (We'll verify this spectrally with the DFT once that's introduced in Lesson 3 — for now, confirm it by eye and against the `aliased_freq` prediction.)

### 10. Expected output

For `fs = 100`: `f_true=30` predicts `alias=30` (correctly under Nyquist, no aliasing). `f_true=60` predicts `alias=40`. `f_true=90` predicts `alias=10`. `f_true=120` predicts `alias=20`. `f_true=150` predicts `alias=50` (exactly at Nyquist — the boundary case). The plotted sample points for `f_true=30` and `f_true=130` should be visually identical.

### 11. Debugging exercise

Symptom, no cause given: You're recording audio at an 8 kHz sample rate, expecting to safely capture everything up to 4 kHz. There's high-pitched electrical interference in the room at roughly 6 kHz. In your recording, you hear an unexplained low tone around 2 kHz that has no business being there — nothing in your actual audio source is anywhere near 2 kHz.

Form a hypothesis before reading on: what's the relationship between 6 kHz, 8 kHz, and 2 kHz?

**Root cause (revealed):** $f_{alias} = |6000 - 8000\cdot\text{round}(6000/8000)| = |6000 - 8000| = 2000$ Hz. The 6 kHz interference is above the 4 kHz Nyquist frequency for an 8 kHz sample rate, so it aliases down to exactly 2 kHz. The fix is an anti-aliasing low-pass filter before the ADC, cutting off content above 4 kHz — sampling alone cannot fix this after the fact, per §12.

### 12. Common mistakes

- Treating $f_s = 2f_{max}$ as "safe" — the theorem needs a *strict* inequality, and real-world signals are never perfectly band-limited, so practical systems always add a guard margin above the bare minimum.
- Forgetting the **anti-aliasing filter** — the sampling theorem describes what happens mathematically, but it does nothing to *prevent* aliasing on its own; you need an analog low-pass filter before the ADC to physically remove content above Nyquist, or it aliases regardless of how carefully you chose $f_s$.
- Applying the real-signal "times 2" rule out of habit to a complex baseband signal — a very common mistake once people start working with IQ, since most people learn Nyquist on real signals first.

### 13. Real RF/cellular application

This is exactly why LTE's standard baseband sample rate for a 20 MHz channel is 30.72 Msps (complex), not some multi-GHz RF rate — the channel is downconverted to complex baseband first, and 30.72 Msps comfortably covers the 20 MHz occupied bandwidth with margin. 5G NR generalizes this: different subcarrier-spacing numerologies scale the same underlying sample-rate logic. Every SDR and every cellular modem you'll touch in this curriculum is built entirely on this one theorem.

### 14. Interview question

*"LTE's sample rate for a 20 MHz channel is 30.72 MHz, not 20 MHz. Why the extra margin?"* Reason it out rather than recalling a number: what did §12 say about real-world signals never being perfectly band-limited? What would happen at the edges of your spectrum if you sampled at exactly the theoretical minimum? (There's also a practical FFT-size convenience reason we'll get to once FFT sizing comes up in Lesson 3 — for now, reason from the aliasing-margin angle alone.)

### 15. Active recall

1. What causes aliasing, mechanically — in terms of the spectral copies from §5?
2. Why does a complex baseband signal need a lower sample rate than a real signal covering the same bandwidth?
3. Does the sampling theorem by itself *prevent* aliasing, or only tell you when it will happen?

### 16. Mini assignment

Using the `aliased_freq` function from §8: at $f_s = 44{,}100$ Hz (standard audio sample rate), what does a true tone at 30 kHz alias down to? Compute it by hand first, then check with code.

### 17. Exit test

1. A signal is band-limited to 8 MHz. What's the minimum real-signal sample rate? What's the minimum complex-baseband sample rate for the same 8 MHz of content?
2. True or false, with a one-line reason: "As long as I filter my signal after sampling, I can remove any aliasing that occurred."
3. In one sentence: why does the sampling-as-impulse-train derivation in §5 produce *copies* of the spectrum rather than just attenuating the original?

---

← [Lesson 1](lesson-01-complex-numbers-and-phasors.md) · [Table of Contents](../README.md) · [Lesson 3 →](lesson-03-dtft-dft-fft.md)