[← Table of Contents](../README.md) · [← Lesson 8](lesson-08-qpsk-mqam.md)

---

# Lesson 9 — Pulse Shaping: Root-Raised-Cosine Filtering and the Nyquist ISI Criterion

*Stage 3 — Digital Communications, continued. Where the "instantaneous symbol" abstraction from Lessons 7–8 becomes an actual transmitted waveform.*

### 1. Intuition

Lessons 7 and 8 treated each symbol as if it appeared instantaneously — a single number, corrupted by noise, decided on the spot. Real transmission can't work that way: an infinitely narrow pulse needs infinite bandwidth, which Lesson 2 already ruled out. So each symbol actually becomes a smoothly-shaped pulse spread out in time, and consecutive symbols' pulses genuinely overlap. The remarkable engineering trick — **pulse shaping** — is designing that pulse's shape so that even though neighboring pulses overlap everywhere else, at the *exact instant* you sample the current symbol, every neighboring pulse crosses precisely zero. Overlap everywhere, interference nowhere it's measured. That sounds almost too convenient to be real, and yet it's exactly what a raised-cosine pulse delivers.

### 2. The engineering problem that forces this

Two hard constraints collide: you must fit your transmission inside a limited channel bandwidth (a regulatory and physical constraint, directly tied to Lesson 2's bandwidth/sample-rate relationship), and you must send symbols back-to-back at a high rate without them corrupting each other's decisions. A naive rectangular pulse — implicitly what Lessons 7–8 assumed — has terrible spectral properties: through any real, band-limited system it smears out (gets convolved with a filter's impulse response, exactly Lesson 4's convolution), and neighboring symbols leak into each other's sampling instants. That leakage is **inter-symbol interference (ISI)**, and it directly corrupts the simple "decide based on which side of zero" rule from Lesson 7.

### 3. Mathematical foundation

**Raised-cosine spectrum**, roll-off factor $\beta \in [0,1]$:
$$P(f) = \begin{cases} T, & |f|\le \tfrac{1-\beta}{2T} \\ \tfrac{T}{2}\Big[1+\cos\Big(\tfrac{\pi T}{\beta}\big(|f|-\tfrac{1-\beta}{2T}\big)\Big)\Big], & \tfrac{1-\beta}{2T} < |f| \le \tfrac{1+\beta}{2T} \\ 0, & |f| > \tfrac{1+\beta}{2T}\end{cases}$$

**Root-raised-cosine (RRC):** $P_{RRC}(f) = \sqrt{P(f)}$, used at *both* transmitter and receiver, so their cascade reconstructs the full raised-cosine response.

**Nyquist zero-ISI criterion:** a pulse $p(t)$ causes zero ISI at symbol-spaced samples if and only if $p(nT) = 0$ for every nonzero integer $n$ (and $p(0)\ne0$, conventionally normalized to 1).

### 4. Formal definition

**ISI** occurs when the pulse used to transmit symbol $k$ has nonzero value at the sampling instants of neighboring symbols $k\pm1, k\pm2,\dots$ — the receiver's decision statistic at symbol $k$'s instant then becomes a corrupted mixture of symbol $k$ plus weighted contributions from its neighbors, not just symbol $k$ plus noise. The **roll-off factor $\beta$** trades bandwidth for time-domain decay: occupied bandwidth is $\tfrac{1+\beta}{2T}$ (versus the theoretical minimum $\tfrac{1}{2T}$ at $\beta=0$), while larger $\beta$ makes the pulse decay faster in time, reducing sensitivity to small timing errors.

### 5. Derivation — why raised-cosine achieves zero ISI (and the direct callback to Lessons 2 & 3)

Sampling $p(t)$ at the symbol rate and asking "does it equal $\delta[n]$" is, mechanically, identical to the sampling problem from Lesson 2 — and it has the identical spectral-copies solution from Lesson 3. Specifically, sampling $p(t)$ at rate $1/T$ produces a DTFT that is a sum of shifted copies of $P(f)$, spaced $1/T$ apart (exactly Lesson 2 §5's derivation, applied here instead of to an ADC): $$\sum_n p(nT)\,e^{-j2\pi fnT} = \frac{1}{T}\sum_k P\!\left(f - \frac{k}{T}\right)$$ The zero-ISI condition $p(nT)=\delta[n]$ makes the left side simply $1$ (the DTFT of a Kronecker delta sequence is the constant function 1). So **the zero-ISI condition is exactly equivalent to**:
$$\sum_k P\!\left(f - \frac{k}{T}\right) = T \quad \text{(a constant, for all } f\text{)}$$
This is the frequency-domain form of the Nyquist criterion — the shifted copies of $P(f)$, spaced $1/T$ apart, must sum to a flat constant everywhere. Raised-cosine's roll-off region is deliberately built with **odd symmetry around the band edge** $\pm\tfrac{1}{2T}$: the rising edge of one shifted copy exactly complements the falling edge of its neighbor, so wherever they overlap, they sum to precisely the flat plateau value. That's the whole mechanism — not a coincidence, but a shape *engineered* so its Lesson-2-style shifted copies interlock into a perfect constant.

### 6. Visualization

Time domain — raised-cosine pulse, crossing exactly zero at every integer multiple of $T$ except $t=0$:
```
      ╭─╮
     ╱   ╲
────╱─────╲──0───0───0───0──── t (zero crossings at ±T, ±2T, ±3T...)
        ╲_╱  ╲_╱
```
Frequency domain — flat plateau, then symmetric roll-off to zero:
```
P(f): ▔▔▔▔▔▔╲
             ╲___  (symmetric roll-off region, width β/T)
                  ╲___
```

### 7. Worked example

At symbol rate $R_s = 1$ MHz ($T=1\mu s$): $\beta=0$ gives the theoretical minimum bandwidth, $500$ kHz (one-sided) — but requires an infinite, non-causal filter to implement exactly. $\beta=0.22$ (a value real standards commonly use) gives $610$ kHz — a practical, implementable filter with fast enough time-domain decay. $\beta=1$ gives $1$ MHz — double the minimum bandwidth, but very short, timing-robust pulses. This is a real, concrete engineering dial: more bandwidth spent buys more robustness to imperfect timing.

### 8. Code

```python
import numpy as np

def rrc_filter(beta, span_symbols, sps):
    N = span_symbols * sps
    t = (np.arange(-N//2, N//2+1)) / sps            # time in units of symbol periods
    h = np.zeros_like(t)
    for i, ti in enumerate(t):
        if ti == 0:
            h[i] = 1 - beta + 4*beta/np.pi
        elif abs(abs(4*beta*ti) - 1) < 1e-8:
            h[i] = (beta/np.sqrt(2)) * (((1+2/np.pi)*np.sin(np.pi/(4*beta))) +
                                          ((1-2/np.pi)*np.cos(np.pi/(4*beta))))
        else:
            num = np.sin(np.pi*ti*(1-beta)) + 4*beta*ti*np.cos(np.pi*ti*(1+beta))
            den = np.pi*ti*(1-(4*beta*ti)**2)
            h[i] = num/den
    return h / np.sqrt(np.sum(h**2))                 # normalize energy

sps = 8                       # samples per symbol (oversampling — Lesson 5's interpolation)
h_rrc = rrc_filter(beta=0.35, span_symbols=10, sps=sps)

bits = np.random.randint(0, 2, 2000)
symbols = 2*bits - 1
upsampled = np.zeros(len(symbols)*sps)
upsampled[::sps] = symbols                            # zero-stuff, exactly Lesson 5 §3

tx = np.convolve(upsampled, h_rrc, mode='same')       # pulse-shape via convolution (Lesson 4)
rx_mf = np.convolve(tx, h_rrc, mode='same')            # matched RRC at receiver (Lesson 6)

delay = 0                                              # correct sampling offset — see §11
decided = (rx_mf[delay::sps][:len(bits)] > 0).astype(int)
print("bit errors (noiseless):", np.sum(decided != bits))
```

### 9. Experiment

Run §8's code noiselessly first — you should get **zero** bit errors, confirming the zero-ISI property directly (this is your first checkpoint). Then add AWGN before the receive filter, sweep $E_b/N_0$, and confirm the resulting BER curve matches Lesson 7/8's theoretical curves — this validates that the "instantaneous symbol" abstraction used in those lessons was a legitimate simplification, not a cheat. Finally, deliberately shift the sampling index by a few samples (not a multiple of `sps`) and watch bit errors appear even with **zero** added noise — pure ISI from bad timing alone. Plotting many overlapping symbol periods of `rx_mf` (an **eye diagram**) makes this vivid: correct timing shows a wide-open "eye" at the sampling instant; bad timing shows it closing.

### 10. Expected output

Noiseless, correctly-timed: zero errors. With AWGN: BER curve overlapping Lesson 7/8's theory. Mistimed, even noiseless: nonzero errors, growing as the offset moves further from the correct instant — direct, hands-on proof that the zero-ISI guarantee is conditional on sampling at exactly the right instant, not automatic.

### 11. Debugging exercise

Symptom, no cause given: your simulated BER with pulse shaping is noticeably worse than Lesson 7/8's theoretical curve, even though you're using RRC filters correctly at both ends, AWGN is added correctly, and you've already verified your bit-mapping with a noiseless loopback test per Lesson 8 §11's advice.

What's left to check? (Hint: §9's mistiming experiment is not a coincidence — it's exactly this bug, made deliberately.)

**Root cause (revealed):** **symbol timing offset.** An RRC/matched-filter pair convolved with real filters has a **group delay** — roughly half the filter's total length in samples — so the "true" symbol-aligned samples in `rx_mf` are *not* at indices `0, sps, 2·sps, ...` but shifted by that filter delay. Sampling even one or two samples away from the true zero-ISI instant reintroduces real ISI, because the Nyquist guarantee only holds at *exactly* the right instants (§5's derivation required sampling precisely at $t=nT$). The fix here is bookkeeping (compute the known filter delay and offset your sampling index accordingly); in a real receiver with an unknown channel delay, this same problem requires active **timing recovery** — a genuine open problem this lesson deliberately leaves for Stage 4.

### 12. Common mistakes

- Using a full **raised-cosine** filter (not root-raised-cosine) at *both* ends — the cascade would then be $RC * RC \ne RC$, breaking both the zero-ISI property and Lesson 6's matched-filter optimality simultaneously. RRC at each end, cascading to RC, is not a stylistic choice — it's required for both properties to hold together.
- Forgetting the filter's **group delay** when picking the sampling index — the single most common practical bug in this area, as shown in §11.
- Choosing $\beta=0$ "because it's most bandwidth-efficient" without accounting for the cost: it requires an extremely long filter to decently approximate the ideal (technically non-causal, infinite) response, and is highly sensitive to timing error. Practical systems almost always use $\beta$ in the 0.2–0.35 range as a deliberate robustness trade-off, not a compromise born of ignorance.

### 13. Real RF/cellular application

Root-raised-cosine pulse shaping is standard in single-carrier digital communication systems generally, and directly relevant to LTE's **uplink** (SC-FDMA, which is fundamentally single-carrier-like at its core, unlike the downlink's pure OFDM — more on that distinction once OFDM arrives in Stage 4). This lesson is squarely **Project 3 (Software Modem)** territory — the pulse-shaping stage is a required piece of that project's TX/RX chain.

### 14. Interview question

*"Why do practical systems split pulse shaping into root-raised-cosine filters at both the transmitter and receiver, instead of putting one full raised-cosine filter at just one end?"* Reason from two lessons at once: Lesson 6 says the receiver filter must be matched to the transmitted pulse shape for SNR-optimal detection; this lesson's §5 says the *cascade* must equal the full raised-cosine response for zero ISI. Splitting the response as a square root at each end is the only way to satisfy both requirements simultaneously — putting the whole filter at one end gets you at most one of the two properties.

### 15. Active recall

1. What's the precise mathematical condition (in the time domain) for a pulse to produce zero ISI at symbol-spaced sampling instants?
2. Why must RRC — not full RC — be used at both transmitter and receiver?
3. What does the roll-off factor $\beta$ physically trade off against what?

### 16. Mini assignment

Using §8's code, generate an eye diagram: reshape `rx_mf` (noiseless, correctly timed) into overlapping segments of length `2*sps` starting at every symbol boundary, and overlay-plot them all on the same axes. Do this again with AWGN added at a moderate $E_b/N_0$ (e.g. 5 dB) and observe how the "eye" partially closes — this is the standard way real receiver hardware visually diagnoses signal quality.

### 17. Exit test

1. Why does $\beta=0$ theoretically achieve the minimum possible bandwidth, and what practical price does that minimum cost?
2. True or false, with a one-line reason: "As long as your RRC filter is correct, timing offset can never cause bit errors."
3. Your debugging hypothesis from §11, if you haven't already sent it — timing recovery is about to become a running theme starting next lesson.

---

**Next:** Lesson 10 opens **Stage 4 — Wireless Communications** with multipath and fading — the physical channel that makes timing and frequency synchronization a genuinely hard problem, not just a bookkeeping detail.

---

[← Lesson 8](lesson-08-qpsk-mqam.md) · [Table of Contents](../README.md) · [Lesson 10 →](lesson-10-multipath-fading.md)