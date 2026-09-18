[← Table of Contents](../README.md) · [← Lesson 5](lesson-05-multirate-dsp.md)

---

# Lesson 6 — Correlation and the Matched Filter

*Stage 2 — Digital Signal Processing (final DSP-tools lesson before Stage 3). This is the operation behind essentially every "detect a known thing in noise" problem you'll face from here on.*

### 1. Intuition

How do you find a known pattern buried in a noisy signal? The intuitive approach is exactly the right one: slide a copy of the pattern you're looking for across the received signal and see where it "lines up best" — where the overlap between the two is largest. That sliding-overlap operation is **correlation**. What's surprising, and worth actually proving rather than taking on faith, is that this intuitive approach isn't just *a* reasonable way to detect a known signal in noise — it's *provably the best possible way*, in a precise mathematical sense. That filter is called the **matched filter**, and §5 below proves why nothing beats it.

### 2. The engineering problem that forces this

Lesson 4 §12 flagged convolution and correlation as "similar formula, easy to confuse" and left it there. Now you need correlation specifically: cellular receivers must detect known reference sequences — LTE's PSS/SSS, NR's PRACH preambles — buried in noise, reliably, even at the worst-case SNR a cell-edge device experiences. "Eyeball it" doesn't scale to that. You need a principled way to answer "is this known pattern present, and exactly where does it start?" — which is the seed of every timing and frequency synchronization procedure in the rest of this curriculum.

### 3. Mathematical foundation

**Cross-correlation** of $x[n]$ with a known reference $s[n]$:
$$r[n] = \sum_k x[k]\,s^*[k-n]$$
measures how well $x$ matches a copy of $s$ shifted by $n$. The **matched filter** for detecting known signal $s[n]$ is defined as:
$$h[n] = s^*[-n]$$
— the time-reversed, complex-conjugated version of the signal you're looking for. Convolving the received signal with this specific $h$ is mathematically identical to correlating it with $s$ directly (convolution flips one of its two inputs; pre-flipping $s$ to make $h$ un-flips it back).

### 4. Formal definition

Given a received signal $r[n] = s[n] + w[n]$ (known signal $s$ plus white noise $w$ of variance $\sigma^2$), the matched filter $h[n]=s^*[-n]$ is, among **all possible linear filters**, the one that maximizes output SNR at the instant of full alignment. This is not "a good heuristic" — §5 proves it's the unique optimum.

### 5. Derivation — proving optimality (Cauchy–Schwarz)

Apply an arbitrary linear filter $h[n]$ to $r[n] = s[n]+w[n]$. At the alignment instant $n_0$ (where the filter fully overlaps the signal), the output splits into a signal part and a noise part:

**Signal part:** $y_s[n_0] = \sum_k s[k]\,h[n_0-k]$

**Noise part:** filtering white noise (variance $\sigma^2$, independent samples) through $h$ gives an output whose variance is $\sigma^2 \sum_k |h[k]|^2$ — this follows directly from the fact that the variance of a weighted sum of independent random variables is the sum of the squared weights times each variance.

So the output SNR at $n_0$ is:
$$\text{SNR} = \frac{\left|\sum_k s[k]\,h[n_0-k]\right|^2}{\sigma^2 \sum_k |h[n_0-k]|^2}$$

Now apply the **Cauchy–Schwarz inequality** — for any two sequences $a[k]$, $b[k]$:
$$\left|\sum_k a[k]\,b[k]\right|^2 \;\le\; \left(\sum_k |a[k]|^2\right)\left(\sum_k |b[k]|^2\right)$$
with **equality if and only if** $b[k] = c\cdot a^*[k]$ for some constant $c$. Setting $a[k]=s[k]$ and $b[k]=h[n_0-k]$:
$$\left|\sum_k s[k]\,h[n_0-k]\right|^2 \le \left(\sum_k |s[k]|^2\right)\left(\sum_k |h[n_0-k]|^2\right)$$

Substituting this bound into the SNR expression, the $\sum |h[n_0-k]|^2$ term **cancels top and bottom**, leaving:
$$\text{SNR} \le \frac{\sum_k |s[k]|^2}{\sigma^2} = \frac{E_s}{\sigma^2}$$
— the maximum possible SNR is just the signal's total energy divided by the noise power, completely independent of the filter's shape. And the equality condition tells you *which* filter achieves it: $h[n_0-k] = c\cdot s^*[k]$, i.e. $h[m] = c\cdot s^*[n_0-m]$ — which, up to the arbitrary scale $c$ and index origin $n_0$, is exactly $h[n]=s^*[-n]$: **the matched filter.** Nothing else reaches this bound; the matched filter isn't a good choice, it's the *only* choice that attains the theoretical ceiling.

### 6. Visualization

```
known sequence s:        [ 1 -1  1]
received (noisy) signal: [ .2  .1 -.9 .3 -1.1 .8  1.2  .0 -.1 ]
                                        ↑ slide s across, correlate at each position
correlation output:      [low  low  low  low  HIGH  low  low]
                                              ↑ peak marks where s actually sits
```

### 7. Worked example

Known sequence $s = [1,-1,1]$. Embed it (noiselessly, for clarity) inside a longer sequence of small random values, at position 4. Correlating $s^*[-n]$ against the full received sequence produces a sharp peak at exactly the offset where the sequence starts — everywhere else, $s$ doesn't line up with itself and the terms partially cancel, giving a low value. That peak-vs-everywhere-else contrast is the whole detection mechanism.

### 8. Code

```python
import numpy as np

s = np.array([1, -1, 1, 1, -1], dtype=float)   # known sequence to detect
N = 200
true_pos = 80
noise_level = 0.5

rx = noise_level * np.random.randn(N)
rx[true_pos:true_pos+len(s)] += s               # embed the known sequence

h = s[::-1].conj()                               # matched filter: time-reversed conjugate
corr = np.convolve(rx, h, mode='full')

detected_pos = np.argmax(np.abs(corr)) - (len(s)-1)
print("true position:", true_pos, " detected:", detected_pos)
```

### 9. Experiment

Run the code above at several `noise_level` values (try 0.1, 1.0, 3.0, 8.0) and watch `detected_pos` — it should stay correct even as the noise grows substantial, only failing once noise power becomes large relative to the sequence energy. Then, separately, try matching with a *deliberately wrong* filter (e.g. `h_wrong = s` without time-reversal/conjugation) and compare the peak-to-sidelobe ratio against the true matched filter at the same noise level — this is your direct, empirical confirmation of §5's optimality claim.

### 10. Expected output

The true matched filter should correctly locate the sequence at meaningfully higher noise levels than the mismatched filter before detection starts failing — a direct, visible demonstration that matched filtering isn't just "a filter that happens to work," it's measurably better than the alternative at the same task.

### 11. Debugging exercise

Symptom, no cause given: you build what you believe is a matched filter for your known sequence, run it against a noisy received signal, and instead of a sharp peak exactly where you embedded the sequence, you get a peak that's consistently offset by a fixed amount from where the sequence actually starts.

What would you check? (Hint: this is precisely the mistake Lesson 4 §12 warned you about, now come back to bite you for real.)

**Root cause (revealed):** using $h[n] = s[n]$ directly — i.e., *convolving* with the sequence itself instead of *correlating* with it — is not the same operation. Convolution implicitly flips one of its inputs; skipping the deliberate flip in $h[n]=s^*[-n]$ means you've silently built a **convolution** with $s$, not a **correlation** against $s$, and the result's peak lands at a position offset by the sequence length from the true alignment point. The fix is exactly the definition in §3: don't skip the time-reversal.

### 12. Common mistakes

- Forgetting the **conjugate** for complex (IQ) sequences — for real-valued sequences $s^*[n]=s[n]$, so this mistake is invisible until you move to complex baseband, where it silently degrades correlation quality without an obvious symptom.
- Using **raw correlation magnitude** as a detection threshold without normalizing for signal energy — a stronger received signal produces a taller peak for reasons that have nothing to do with a better match, causing false alarms or missed detections as received power varies. Real detectors use a normalized correlation (dividing by the energy of the received segment).
- Misreading the **peak index** — because of how `'full'` convolution mode pads the output, the true alignment point is offset from the raw `argmax` index by `len(s)-1`, exactly as handled in §8's code; skipping this adjustment silently shifts every detected position.

### 13. Real RF/cellular application

This lesson *is* the mechanism behind PSS/SSS cell-search in LTE (Stage 7), SSB detection in 5G NR (Stage 8), and PRACH random-access preamble detection — every one of these is fundamentally "correlate the received signal against a known reference sequence, find the peak." GPS receivers use the identical principle to acquire satellite PRN codes. From here through the rest of the curriculum, whenever you read "detect," "acquire," or "search for" a known signal, matched filtering is very likely the mechanism underneath.

### 14. Interview question

*"Why is the matched filter provably the optimal detector for a known signal in white noise, rather than just a reasonable design choice?"* Answer from the Cauchy–Schwarz equality condition in §5 — don't just state the conclusion, walk through why equality forces that specific filter shape.

### 15. Active recall

1. What's the precise relationship between the matched filter $h[n]$ and the signal $s[n]$ it's built to detect?
2. What quantity does the maximum achievable SNR from §5 actually depend on — and notably, what does it *not* depend on?
3. Why does using $s[n]$ directly as a filter (instead of $s^*[-n]$) produce a peak at the wrong position rather than just a weaker one?

### 16. Mini assignment

Using §8's code, sweep `noise_level` across a range (e.g. 0.1 to 10 in steps) and, for each level, record the ratio of the true peak's height to the second-highest peak elsewhere in the correlation output. Plot this ratio against noise level and find roughly where it drops below a level you'd trust for reliable detection (e.g. ratio < 2).

### 17. Exit test

1. Derive, in one line, why the noise term's variance after filtering is $\sigma^2\sum_k|h[k]|^2$ rather than just $\sigma^2$.
2. True or false, with a one-line reason: "A matched filter designed for signal $s_1$ will also perform optimally at detecting a different, unrelated signal $s_2$."
3. Your debugging hypothesis from §11, if you haven't sent it — this one is genuinely important to get right before Stage 3, where matched filtering becomes a core building block rather than an isolated concept.

---

**Next:** this closes out the DSP-tools stretch of Stage 2 (Lessons 3–6: transforms, filtering, multirate, correlation). Lesson 7 opens **Stage 3 — Digital Communications**: AWGN, SNR, and building your first BPSK modulator/demodulator with a real BER curve — the first project-scale payoff of everything so far.

---

[← Lesson 5](lesson-05-multirate-dsp.md) · [Table of Contents](../README.md)  ·  [Lesson 7 →](lesson-07-awgn-bpsk-ber.md)