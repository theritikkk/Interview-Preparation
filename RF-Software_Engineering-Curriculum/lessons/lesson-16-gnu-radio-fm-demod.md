[← Table of Contents](../README.md) · [← Lesson 15](lesson-15-real-iq-dc-offset-iq-imbalance.md)

---

# Lesson 16 — GNU Radio: Flowgraphs, the Scheduler, and Decoding Real FM

*Stage 5 — Software-Defined Radio, continued. This is where "a Python script that runs top to bottom" becomes "a graph of reusable, connected blocks" — the same architectural pattern you'll meet again inside GNU Radio itself, srsRAN, and OpenAirInterface.*

### 1. Intuition

Every lesson so far processed IQ samples inside one monolithic script you controlled start to finish. Real DSP software — GNU Radio, and every serious SDR/RAN codebase you'll read later — is instead built as a **graph of small, reusable blocks** connected by continuous data streams, each block knowing nothing except "read some samples in, write some samples out." This isn't just a GNU Radio convention; it's the standard shape of real-time streaming DSP software generally, and understanding its scheduler and buffer model here pays off directly once you're reading srsRAN's source or building your own real-time pipeline in Stage 10. Broadcast FM is the perfect first target to build this way: real, immediately verifiable (you either hear clean audio or you don't), and its demodulation math is a short, genuinely satisfying derivation.

### 2. The engineering problem that forces this

You need a receiver chain — source, filter, demodulator, audio output — that can be assembled and rearranged without rewriting a script each time, that correctly handles data arriving continuously at *different* rates at different points in the chain (after a decimating block, Lesson 5, downstream blocks see a lower rate), and that doesn't let one slow stage stall the whole pipeline unnecessarily. FM demodulation itself needs its own answer to a very specific question: given a stream of complex baseband samples whose *instantaneous frequency* carries the message, how do you extract that frequency, sample by sample, without the numerical fragility of tracking absolute phase over a long, noisy signal?

### 3. Mathematical foundation

An FM signal's complex baseband representation is $x(t) = A\,e^{j\theta(t)}$, where the **instantaneous frequency** is $\frac{1}{2\pi}\theta'(t) = k_f\,m(t)$ — the message $m(t)$ is encoded directly as how fast the phase is spinning, not in the amplitude. The standard, numerically robust demodulator computes:
$$y[n] = \angle\big(x[n]\cdot x^*[n-1]\big)$$

### 4. Formal definition

$\theta'(t)$, the **instantaneous frequency**, is the quantity FM demodulation must recover. A **GNU Radio flowgraph** is a directed graph of **blocks**, each implementing a `work()` function that consumes available input-buffer samples and produces output-buffer samples; a runtime **scheduler** calls each block's `work()` whenever enough input is available and enough output space is free, without any block needing to know about the others directly — streaming connections carry data of *unbounded, unknown-in-advance* length, unlike a one-shot function call.

### 5. Derivation — the quadrature FM demodulator, from first principles

Start from $x(t) = A(t)e^{j\theta(t)}$ and differentiate using the product rule:
$$x'(t) = \big[A'(t) + j\theta'(t)A(t)\big]e^{j\theta(t)}$$
Multiply by $x^*(t) = A(t)e^{-j\theta(t)}$:
$$x'(t)\,x^*(t) = A(t)A'(t) + j\,\theta'(t)\,A(t)^2$$
The real part is an amplitude-derivative term (irrelevant for ideal constant-envelope FM); the **imaginary part** isolates exactly what we want:
$$\theta'(t) = \frac{\text{Im}\{x'(t)\,x^*(t)\}}{A(t)^2}$$
In discrete time, approximate $x'(t)\approx x[n]-x[n-1]$ and substitute:
$$\theta'[n] \approx \frac{\text{Im}\{(x[n]-x[n-1])\,x^*[n-1]\}}{|x[n-1]|^2} = \frac{\text{Im}\{x[n]\,x^*[n-1]\} - \text{Im}\{|x[n-1]|^2\}}{|x[n-1]|^2}$$
The second term in the numerator is purely real, so its imaginary part is zero, leaving $\theta'[n]\propto \text{Im}\{x[n]x^*[n-1]\}$. Now notice: $x[n]x^*[n-1] = |x[n]||x[n-1]|\,e^{j(\theta[n]-\theta[n-1])}$ — a complex number whose **angle is exactly the phase difference** between consecutive samples. Provided that phase difference doesn't exceed $\pm\pi$ (a condition worth remembering for §11), taking the angle directly gives:
$$y[n] = \angle\big(x[n]\,x^*[n-1]\big) \approx \theta[n]-\theta[n-1] \propto \theta'[n]$$
— the instantaneous frequency, recovered with **one conjugate multiply and one arctangent per sample**, no absolute phase tracking, no unwrapping. This is precisely GNU Radio's built-in "Quadrature Demod" block, and it reuses the exact same primitive operation — conjugate multiplication to extract a phase relationship — that Lesson 6's correlation and Lesson 13's Schmidl–Cox timing/CFO estimator both used, just pointed at a different question.

### 6. Visualization

```
Minimal FM-receiver flowgraph:

[RTL-SDR Source] → [Low-Pass Filter + Decimate] → [Quadrature Demod] → [Audio Filter + Decimate] → [Audio Sink]
     (Lesson 15)         (Lesson 4 & 5)                (this lesson, §5)         (Lesson 4 & 5)

Phase difference between consecutive samples = instantaneous frequency:
   x[n-1]  ●
            ╲ Δθ
             ╲
   x[n]       ●───►  angle(x[n]·conj(x[n-1])) = Δθ ∝ instantaneous frequency
```

### 7. Worked example

Suppose $x[n-1] = 1\angle0°$ and $x[n]=1\angle10°$ (a small phase advance between samples). Then $x[n]\cdot x^*[n-1] = 1\angle10° \cdot 1\angle0° = 1\angle10°$ (conjugating a unit-magnitude sample at angle 0 doesn't change its angle). $y[n] = \angle(1\angle10°) = 10°$, directly recovering the phase advance — and since instantaneous frequency is phase advance per unit time, this is exactly proportional to the message value at that instant.

### 8. Code

```python
import numpy as np

fs = 200_000
fc_dev = 20_000                 # peak frequency deviation
t = np.arange(0, 0.02, 1/fs)
message = np.sin(2*np.pi*500*t)                       # a 500 Hz test tone as the "audio"

# --- FM modulate ---
phase = 2*np.pi*fc_dev*np.cumsum(message)/fs           # integral of message, Lesson 3-style
x = np.exp(1j*phase)                                    # ideal constant-envelope FM baseband

# --- Quadrature demod, exactly §5's derivation ---
y = np.angle(x[1:] * np.conj(x[:-1]))
y_scaled = y * fs / (2*np.pi)                           # convert phase-diff/sample to Hz

# y_scaled should closely track fc_dev * message[1:] (up to scaling)
recovered = y_scaled / fc_dev
print("max recovery error:", np.max(np.abs(recovered - message[1:])))
```

### 9. Experiment

Run §8's code and confirm `recovered` closely tracks the original `message` — direct, working proof of §5's derivation. Then push `fc_dev` much higher (e.g. $5\times$) relative to `fs` without changing the sample rate, and rerun: watch the recovered waveform develop sharp, obviously wrong spikes exactly where the message's instantaneous slope is steepest. This is deliberately reproducing §5's "phase difference must not exceed $\pm\pi$" condition being violated — a direct, hands-on encounter with the caveat flagged during the derivation.

### 10. Expected output

At moderate `fc_dev`, recovery error should be small (limited mainly by the discrete-derivative approximation, not a fundamental flaw). At excessive `fc_dev` relative to `fs`, sharp discontinuities and spikes appear precisely where phase changes fastest — the demodulator's angle computation silently wrapping.

### 11. Debugging exercise

Symptom, no cause given: your FM demodulator produces audio that sounds mostly correct, but with periodic crackling and popping artifacts, worse specifically during louder (higher-deviation) passages of the source audio.

What condition from §5 might occasionally be violated only during the "loud" parts, and why would that specifically explain crackling tied to loudness?

**Root cause (revealed):** this is exactly §9's phenomenon, occurring naturally rather than by deliberate exaggeration. Louder audio means larger instantaneous frequency deviation, which means a larger phase change between consecutive samples. If that per-sample phase change occasionally exceeds $\pm\pi$ radians — precisely the condition §5 flagged as necessary for `angle()` to return the true phase difference rather than a wrapped, aliased value — the demodulator momentarily produces a corrupted, wrong value exactly at those loud moments, heard as crackling. This is a **sampling-rate problem in disguise**: it's Lesson 2's Nyquist criterion, applied not to the signal's bandwidth directly but to how fast *phase itself* is allowed to change between samples. The fix is the same family of fix as always: increase the sample rate relative to the maximum expected frequency deviation, giving the phase less room to advance between any two consecutive samples.

### 12. Common mistakes

- Attempting FM demod by literally tracking and unwrapping absolute phase over a long signal, instead of the bounded, robust per-sample conjugate-multiply trick from §5 — unwrapping accumulates numerical error and is far more fragile in the presence of noise.
- Assuming a GNU Radio block always receives a fixed-size buffer — the scheduler hands each block whatever amount of data is currently available, which varies call to call; correctly written blocks must never hard-code an expected buffer length.
- Forgetting that after a decimating block (Lesson 5), every block downstream in the flowgraph sees a **lower** effective sample rate — filter and demod parameters downstream must be designed relative to the post-decimation rate, not the original capture rate. This is a very common real flowgraph-construction bug.

### 13. Real RF/cellular application

The exact primitive derived in §5 — extracting a phase relationship via conjugate multiplication — is reused constantly in cellular receivers for purposes well beyond FM broadcast: simple one-shot frequency-offset estimation is a direct relative of this trick, and it's structurally the same operation Lesson 13's Schmidl–Cox algorithm used for joint timing/CFO estimation, just applied to a delayed copy of a known reference structure instead of consecutive raw samples. More broadly, the block/flowgraph/scheduler architecture you've just used is the same conceptual pattern found in srsRAN and OpenAirInterface's real, production code — understanding this one teaches you to read those.

### 14. Interview question

*"Why does practical FM demodulation use `angle(x[n] * conj(x[n-1]))` instead of directly differentiating an unwrapped phase signal?"* Answer from §5 and §12: unwrapping requires tracking cumulative phase across an entire signal and is numerically fragile, especially with noise; the conjugate-multiply trick computes a bounded, self-contained phase *difference* directly, needs only the immediately preceding sample, and never needs to unwrap anything at all.

### 15. Active recall

1. Derive, in one line, why $x[n]x^*[n-1]$'s angle equals the phase difference between consecutive samples.
2. Why does excessive frequency deviation relative to sample rate cause demodulation artifacts, in terms of §5's derivation?
3. Why must a correctly written GNU Radio block avoid assuming a fixed input buffer size?

### 16. Mini assignment

Extend §8's experiment: sweep `fc_dev` across a range and, for each value, compute the maximum recovery error. Find, empirically, roughly where error starts growing sharply, and check that this threshold roughly corresponds to the point where the maximum per-sample phase change approaches $\pi$ radians — connecting the empirical breakdown point directly back to §5's derivation.

### 17. Exit test

1. Write the quadrature FM demodulator formula from memory, then check it against §3.
2. True or false, with a one-line reason: "A GNU Radio block can assume it will always receive the same number of samples on every call to its work function."
3. Your reasoning from §11 connecting crackling artifacts to Lesson 2's Nyquist criterion, if you haven't sent it.

---

**Next:** Lesson 17 moves to working with **recorded IQ datasets and real signal formats** (SigMF metadata, common file conventions) — decoding something genuinely more complex than broadcast FM, using a real capture you didn't generate yourself.

---

[← Lesson 15](lesson-15-real-iq-dc-offset-iq-imbalance.md) · [Table of Contents](../README.md) · [Lesson 17 →](lesson-17-recorded-iq-adsb.md)