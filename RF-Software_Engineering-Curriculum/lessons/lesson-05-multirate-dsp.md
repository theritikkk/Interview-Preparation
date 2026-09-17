[← Table of Contents](../README.md) · [← Lesson 4](lesson-04-convolution-fir-filters.md)

---

# Lesson 5 — Multirate DSP: Decimation, Interpolation, and Why You Always Filter First

*Stage 2 — Digital Signal Processing. Where Lesson 4's "controlled approximation" line pays off.*

### 1. Intuition

Sometimes the sample rate you captured at isn't the sample rate you want to *work* at. Maybe you captured a wide 30.72 Msps block to catch an entire wideband channel, but the signal you actually care about only occupies a narrow slice of it — processing all 30.72 million samples/sec for a 1.4 MHz sliver wastes enormous compute. You want to throw samples away (**decimation**) to save work. Or the reverse: you have samples at one rate but need a finer time grid — for precise timing alignment, or to match a DAC's output rate (**interpolation**). Both sound like they should be simple bookkeeping — "just keep every Mth sample" or "just add some samples in between." Neither is simple, and Lesson 2's aliasing theorem is exactly why.

### 2. The engineering problem that forces this

Say you decimate by $M=8$ by literally keeping every 8th sample and throwing the rest away. You've just changed your effective sample rate from $f_s$ to $f_s/8$ — which means your *new* Nyquist frequency is $f_s/16$, eight times lower than before. Any content in your original signal between the new and old Nyquist frequencies doesn't disappear — it **aliases**, exactly per Lesson 2's derivation, folding down on top of your signal of interest and corrupting it. So naive decimation is not "free downsampling" — it's a silent aliasing bug waiting to happen unless you filter first. Interpolation has the mirror-image problem: if you insert zeros between samples to raise the rate, you don't get a smoother signal — you get spectral copies ("images") that need to be filtered away before the result is usable.

### 3. Mathematical foundation

**Decimation by $M$:** low-pass filter $x[n]$ to cutoff $f_s/(2M)$, *then* keep every $M$th sample: $y[n] = x_{filtered}[Mn]$.

**Interpolation by $L$:** insert $L-1$ zeros between each original sample, *then* low-pass filter (cutoff at the original Nyquist, now expressed on the new higher rate's frequency axis) to remove the spurious images the zero-insertion creates, then scale amplitude by $L$ to restore the correct signal level.

### 4. Formal definition

For decimation, $y[n] = x[Mn]$ relates to the original spectrum by:
$$Y(e^{j\omega}) = \frac{1}{M}\sum_{k=0}^{M-1} X\!\left(e^{j(\omega - 2\pi k)/M}\right)$$
— the new spectrum is a sum of $M$ frequency-stretched, shifted copies of the original. If $X$ occupied more than $1/M$ of the band before decimating, those copies **overlap**: aliasing, exactly per Lesson 2. For interpolation, zero-stuffing by $L$ gives $W(e^{j\omega}) = X(e^{j\omega L})$ — a frequency-*compressed* copy of $X$, repeated $L$ times across one period. Those extra repeats are the "images" that must be filtered out.

### 5. Derivation (the core relationship, sketched)

The decimation formula in §4 comes from writing $y[n]=x[Mn]$ as $x[n]$ multiplied by an impulse train that keeps every $M$th sample and zeros the rest, then taking the DTFT — multiplication in time again becomes a sum of shifted copies in frequency, the same mechanism as the impulse-train sampling derivation from Lesson 2 §5, just applied a second time at a coarser grid. The practical consequence is the whole point: **decimation only preserves information correctly if the signal was already band-limited to $f_s/(2M)$ before you threw samples away** — which is exactly why you filter first, not after. Once the copies in §4's sum overlap, that overlap is summed and irreversible, for the identical reason overlapping spectra were irreversible in Lesson 2. There is no filter you can apply *after* decimating that undoes it — the information is already gone.

### 6. Visualization

Naive decimation without pre-filtering — wideband content folds onto your signal of interest:
```
before:  ___/\____/\/\____/\___     (signal of interest + wideband content)
              ↑ keep every Mth sample, no filtering first
after:   ___/\/\/\/\/\/\/\/\___     (content that was "out there" aliases in on top)
```
Correct order — filter to the new Nyquist first, *then* decimate:
```
before:  ___/\____/\/\____/\___
filter:  ___/\______________     (out-of-band content removed first)
decim.:  ___/\____                (now safe to keep every Mth sample)
```

### 7. Worked example

A signal contains a wanted tone at 1 kHz and interference at 9 kHz, sampled at $f_s = 20$ kHz. Decimate by $M=2$ naively (new $f_s = 10$ kHz, new Nyquist = 5 kHz): the 9 kHz interferer aliases to $|9 - 10| = 1$ kHz — landing **exactly on top of** your 1 kHz signal of interest, indistinguishable and unrecoverable. Filter to cutoff 5 kHz *before* decimating, and the 9 kHz interferer is removed before it ever gets the chance to alias.

### 8. Code

```python
import numpy as np
from scipy.signal import firwin, lfilter

fs = 20000
t = np.arange(0, 0.05, 1/fs)
x = np.cos(2*np.pi*1000*t) + np.cos(2*np.pi*9000*t)   # wanted 1kHz + interferer 9kHz

M = 2
# WRONG: naive decimation, no filtering
y_naive = x[::M]

# RIGHT: filter to new Nyquist (fs/(2M) = 5kHz), then decimate
h = firwin(65, cutoff=5000, fs=fs)          # low-pass FIR, cutoff at new Nyquist
x_filtered = lfilter(h, 1.0, x)
y_correct = x_filtered[::M]

# Compare spectra (using the FFT tools from Lesson 3) — y_naive shows a spurious
# low-frequency artifact from the aliased 9kHz tone; y_correct does not.
```

### 9. Experiment

Run the code above, take `np.fft.fft` of `y_naive` and `y_correct` (new sample rate 10 kHz), and compare. `y_naive`'s spectrum should show energy at both 1 kHz *and* an aliased artifact near 1 kHz from the folded 9 kHz tone — they'll interfere and distort the peak. `y_correct`'s spectrum should show a clean single peak at 1 kHz only. Then try interpolation: zero-stuff a short tone by $L=4$ (`y_up = np.zeros(4*len(x)); y_up[::4] = x`), take its FFT, and observe the repeated spectral images — then apply a low-pass filter at the *original* Nyquist and watch them disappear.

### 10. Expected output

`y_naive`'s spectrum is visibly corrupted near 1 kHz; `y_correct`'s is clean. The zero-stuffed signal's raw spectrum shows $L=4$ repeated copies of the original spectrum across the full band; after filtering, only the original baseband copy remains.

### 11. Debugging exercise

Symptom, no cause given: you're reducing an SDR capture from 30.72 Msps down to 3.84 Msps (decimate by 8) to save CPU for narrowband processing — you did it the fast way, just keeping every 8th sample. Your narrowband signal of interest, which looked clean in the original wideband capture, now looks noisy with extra tones that weren't there before.

What's your hypothesis, based on everything in this lesson? What would you check first?

**Root cause (revealed):** classic missing pre-filter. Some wideband content elsewhere in the original 30.72 Msps capture — outside your narrowband signal of interest, but within the *original* Nyquist — is above the *new*, eight-times-lower Nyquist frequency after decimation, and aliases straight down onto your signal. The fix is exactly §3's recipe: low-pass filter to $f_s/(2\times8)$ before keeping every 8th sample.

### 12. Common mistakes

- Skipping the pre-filter before decimating — by far the most common real bug in this area, and often silent (nothing crashes; the data is just quietly corrupted).
- Forgetting the amplitude-scaling factor $L$ after interpolation — zero-stuffing followed by filtering leaves the signal $L\times$ too small in amplitude unless you explicitly rescale.
- Designing the filter's cutoff relative to the wrong rate — for decimation, the cutoff is relative to the **new, lower** rate's Nyquist; for interpolation's imaging filter, relative to the **original, lower** rate's Nyquist, expressed on the new higher sample-rate axis. Mixing these up is an easy, very common index error.
- For a combined rational resampling ratio ($L/M$), doing decimation before interpolation — the correct order is always **interpolate (up) first, then decimate (down)**, so you never throw away information before you've had the chance to add the resolution interpolation provides.

### 13. Real RF/cellular application

This is precisely how SDR and cellular receivers handle bandwidth efficiently: capture wideband, then decimate down to the channel of interest to cut compute load — the exact scenario in this lesson's worked example. Interpolation shows up constantly in synchronization: recovering a matched filter's output at a fractional (non-integer) timing offset — something you'll need for the Software Modem project (Project 3) — is done by interpolating to a finer time grid. And LTE/NR's family of channel bandwidths are all related to a common base sample rate (30.72 Msps for LTE) by simple rational resampling ratios — this lesson is literally the mechanism behind that.

*(One efficiency detail worth flagging for later, not deriving now: doing the filter and the decimation as two separate steps wastes computation — you'd be computing filter outputs for samples you're about to throw away. **Polyphase filtering** restructures the filter to only compute the samples you keep, giving the same result far more cheaply. That's a Stage 10 / performance-optimization topic, not needed to understand the concept here.)*

### 14. Interview question

*"Why must you filter before decimating, rather than decimating first and filtering the result afterward?"* Reason from §5: once the spectral copies from decimation overlap, that overlap is a sum — irreversible, for the same reason Lesson 2's aliased spectra can't be un-added. Filtering after the fact can't recover information that's already been folded together.

### 15. Active recall

1. What determines the correct filter cutoff when decimating by $M$?
2. Why does zero-stuffing by $L$ create spectral images, and how do you remove them?
3. For a combined resample by $L/M$, why interpolate before decimating rather than the other way around?

### 16. Mini assignment

Extend §8's code: decimate the same two-tone signal by $M=4$ instead of $M=2$, both naively and with proper filtering. Predict, before running it, whether the 9 kHz interferer will still alias onto the 1 kHz signal of interest at this new decimation factor — then check.

### 17. Exit test

1. A signal is band-limited to 6 MHz, sampled at 20 Msps. What's the maximum decimation factor $M$ you could use without needing to filter first?
2. True or false, with a one-line reason: "Interpolating a signal by zero-stuffing adds new information to it."
3. Your hypothesis from §11, if you haven't already sent it.

---

**Next:** Lesson 6 wraps up the DSP-tools stretch of the curriculum with correlation and the matched filter — the operation that will directly power synchronization and detection once we reach digital communications.

---

← [Lesson 4](lesson-04-convolution-fir-filters.md) · [Table of Contents](../README.md)  ·  [Lesson 6 →](lesson-06-correlation-matched-filter.md)