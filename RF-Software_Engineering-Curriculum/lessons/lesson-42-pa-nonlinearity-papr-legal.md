[← Table of Contents](../README.md) · [← Lesson 41](lesson-41-convolutional-viterbi.md)

---

# Lesson 42 — Transmit-Capable SDR: Power Amplifier Nonlinearity, PAPR, and Why Transmission Is Legally Regulated

*SDR Laboratory, Stage 5-6. Every lesson so far treated the transmitter as an ideal, linear black box. Real transmit hardware has one component receivers never need to worry about the same way — and understanding exactly how it distorts your signal explains both a real LTE/NR design decision and why spectrum regulation exists at all, from the same derivation.*

### 1. Intuition

Every waveform this curriculum has generated assumed a perfectly linear path from digital samples to the air. Real transmit hardware adds one thing that breaks this: a **power amplifier (PA)**, which boosts a signal to real transmit power and is, to some degree, always **nonlinear** — especially near its maximum output. This nonlinearity distorts your carefully-designed waveform in a very specific way: it spreads energy *outside* your intended channel, directly interfering with neighboring users. This is precisely why every country's spectrum regulator imposes strict limits on out-of-band emissions, and precisely why transmitting is regulated far more strictly than receiving — the two facts share one root cause, not two separate ones.

### 2. The engineering problem that forces this

You need to understand, quantitatively, how PA nonlinearity distorts a signal, why some waveforms (OFDM specifically) are more vulnerable to it than others, and what "spectral regrowth" actually means mathematically — not as a vague warning, but as something directly derivable from Lesson 4's convolution theorem.

### 3. Mathematical foundation

A memoryless PA nonlinearity is commonly modeled as an odd-order polynomial: $y(t) = a_1x(t) + a_3x(t)|x(t)|^2 + a_5x(t)|x(t)|^4+\cdots$ (even-order terms fall far out-of-band for a bandpass PA and are conventionally omitted). **PAPR** (peak-to-average power ratio): $\text{PAPR} = P_{peak}/P_{avg}$.

### 4. Formal definition

**AM/AM distortion**: output amplitude deviates from linear as a function of input amplitude, typically compressing near saturation. **AM/PM distortion**: output *phase* shifts as a function of input amplitude — a real, often underappreciated effect. **Spectral regrowth**: out-of-band spectral energy created specifically by the odd-order nonlinear terms.

### 5. Derivation — spectral regrowth bandwidth, and why OFDM is specifically vulnerable

**(a) Why a 3rd-order nonlinearity triples bandwidth.** Take $x(t)$ band-limited to width $B$. The 3rd-order term is $x(t)|x(t)|^2 = x(t)x(t)x^*(t)$. Multiplication in time is convolution in frequency (Lesson 4's convolution theorem, again) — so this term's spectrum is $X(f)*X(f)*X^*(-f)$, a **three-fold convolution** of the original spectrum with itself. Convolving two signals each of width $B$ produces a result of width $2B$ (direct interval arithmetic — the supports add); convolving three together (two successive convolutions) produces width $3B$. This is exactly why 3rd-order intermodulation lands at three times the original bandwidth — a direct, derivable consequence of tools you already have, not an isolated fact to memorize.

**(b) Why OFDM's PAPR makes this specifically worse — the same derivation as Lesson 10 §5(b), pointed at a new question.** An OFDM time-domain sample is (via the IFFT, Lesson 12) a sum of many independently-modulated subcarrier phasors. By the identical Central Limit Theorem argument that gave Rayleigh-fading magnitude its distribution in Lesson 10 §5(b) — a sum of many independent phasors converges toward a complex Gaussian — OFDM's *instantaneous amplitude* occasionally produces large peaks relative to its average power, purely from many subcarriers happening to align constructively at once. High PAPR means the signal periodically drives the PA into its nonlinear region during those peaks, even when *average* power sits comfortably within the PA's linear range — producing exactly the $3B$-bandwidth spectral regrowth from (a), concentrated at those peak moments.

This forces a real trade-off: either **back off** the PA (operate at reduced average power relative to its maximum, preserving headroom for peaks — a genuine efficiency cost, since PAs are most power-efficient near saturation), or reduce PAPR at the waveform level. This is precisely why LTE's **uplink** uses SC-FDMA rather than pure OFDMA: a single-carrier-like signal doesn't sum many independent subcarrier phasors the same way OFDM does, giving it inherently lower PAPR — directly relevant because uplink transmission happens from a battery-constrained handset, where PA efficiency is a much harder constraint than at a mains-powered base station, which is why the **downlink** can afford pure OFDMA's higher PAPR. One derivation connects a battery-life engineering decision to a mathematical fact about summed phasors.

### 6. Visualization

```
AM/AM compression curve:                   Spectral regrowth:
output                                         power
  │   ╱‾‾‾‾ (saturation, compresses)            │    ▄▄▄▄▄
  │  ╱                                          │  ▄▄█████▄▄
  │ ╱   (linear region)                         │▄▄███████████▄▄  ← "shoulders":
  │╱                                            └──┴───┴───┴──→ f   regrowth at 3B
  └────────────→ input                              in-band  ↑ out-of-band (3x width)
```

### 7. Worked example

A 20 MHz LTE-like signal: 3rd-order intermodulation spreads regrowth across roughly 60 MHz total width — directly relevant to how far away an adjacent channel or a different operator's band needs to sit to avoid interference, and directly why ACPR (adjacent-channel power ratio) is the real regulatory metric, not just "in-band signal quality."

### 8. Code

```python
import numpy as np

def pa_nonlinearity(x, a1=1.0, a3=-0.3):
    return a1*x + a3*x*np.abs(x)**2          # Sec 3's odd-order model

def papr_db(x):
    return 10*np.log10(np.max(np.abs(x)**2) / np.mean(np.abs(x)**2))

N = 256
ofdm_symbols = (2*np.random.randint(0,2,(N,2)).astype(float)-1)
ofdm_x = np.fft.ifft(ofdm_symbols[:,0] + 1j*ofdm_symbols[:,1])
ofdm_x /= np.sqrt(np.mean(np.abs(ofdm_x)**2))     # normalize average power

sc_x = (2*np.random.randint(0,2,N)-1).astype(complex)  # single-carrier, low-PAPR reference
sc_x /= np.sqrt(np.mean(np.abs(sc_x)**2))

print("OFDM PAPR:", papr_db(ofdm_x), "dB")
print("Single-carrier PAPR:", papr_db(sc_x), "dB")

for backoff_db in [0, 3, 6, 9]:
    scale = 10**(-backoff_db/20)
    y_ofdm = pa_nonlinearity(ofdm_x * scale)
    spec = np.abs(np.fft.fft(y_ofdm, 1024))**2
    inband = np.sum(spec[:len(spec)//4])
    adjacent = np.sum(spec[len(spec)//4:len(spec)//2])
    print(f"backoff={backoff_db}dB  ACPR≈{10*np.log10(adjacent/inband):.1f} dB")
```

### 9. Experiment

Run §8's code, confirming OFDM's PAPR is meaningfully higher than the single-carrier reference — direct, numeric confirmation of §5(b)'s claim. Sweep back-off and observe ACPR improve (become more negative, i.e. less adjacent-channel leakage) as back-off increases — the real efficiency-vs-compliance trade-off, quantified rather than asserted.

### 10. Expected output

OFDM PAPR noticeably higher than the single-carrier signal's. ACPR improving steadily with more back-off for both, but the OFDM signal needing *more* back-off than the single-carrier signal to hit the same ACPR target — the concrete cost §5(b) predicted.

### 11. A practical scenario, not a code bug

**Symptom, worth reasoning through carefully:** you build a low-power personal transmit experiment with a HackRF, picking a frequency that looked empty on a quick spectrum scan, at what seems like a very low, surely-harmless power level. Using only this lesson: what's actually wrong here, if anything?

**Reasoning through it:** "looked empty during a quick scan" is not evidence a band is unlicensed — many licensed users transmit intermittently, or your receiver simply isn't picking up a legitimate, distant, encrypted, or low-power user who is still there and still protected by license. Separately, and specifically because of this lesson: low **intended** signal power does not bound what your hardware chain actually radiates — §5's spectral regrowth means real out-of-band emissions depend on PA/DSP nonlinearity in your specific chain, not just your target signal's designed bandwidth, and a naive setup can produce meaningful out-of-band emissions even at modest total power. The safe path for early experimentation is to transmit only on genuinely license-exempt ISM allocations while respecting their *specific* technical limits (power, bandwidth, duty cycle), or on bands you hold a valid license for (e.g., amateur radio, WPC-issued in India per Lesson 6's earlier note) — and for pure learning experiments, into a shielded enclosure or dummy load rather than an actual antenna, removing the question of real-world emissions entirely while you're still learning the hardware.

### 12. Common mistakes

- Assuming low transmit power alone makes a transmission legal — power is only one of several independent requirements (frequency allocation, emission mask, licensing); satisfying one doesn't satisfy the others.
- Treating "I didn't observe anything on a scan" as equivalent to "this band is free to use" — a common, incorrect inference, per §11.
- Reasoning about compliance only in terms of your intended signal's bandwidth, ignoring what your actual hardware chain radiates once real PA/DSP nonlinearity (§5) is accounted for.

### 13. Real RF/cellular application

PA linearity-versus-efficiency trade-offs are a genuine, major engineering specialization at every RF/wireless semiconductor company in this curriculum's career map — PA design and DSP-side mitigation techniques (digital predistortion, crest-factor reduction) are real, named, valuable skills directly built on this lesson's derivation. Regulatory emissions compliance testing is the direct business of certification labs and a mandatory step before any commercial wireless product ships, tying to the FCC/regulatory-testing side of this curriculum's test-engineering career track.

### 14. Interview question

*"Why does LTE/NR use OFDMA for the downlink but a different waveform, SC-FDMA, for the uplink specifically?"* Answer, now derivable rather than recalled: uplink transmission comes from a battery-constrained handset where PA efficiency is a hard constraint; OFDM's higher PAPR (§5(b)'s CLT-driven peak argument) would demand more back-off there, directly hurting battery life. Downlink transmission comes from a mains-powered base station with a more capable PA, where this constraint is far less severe — an asymmetric physical constraint producing a deliberate, asymmetric waveform choice.

### 15. Active recall

1. Derive, in one line, why 3rd-order PA nonlinearity triples a signal's occupied bandwidth.
2. Why does OFDM specifically have higher PAPR than a single-carrier signal, in terms of the same math behind Rayleigh fading?
3. Why doesn't low transmit power alone guarantee regulatory compliance?

### 16. Mini assignment

Using §8's code, find the minimum back-off (in dB) each waveform (OFDM and single-carrier) needs to reach a common illustrative ACPR target (e.g. −40 dB), and report the gap between them — a concrete, quantified version of this lesson's central efficiency trade-off.

### 17. Exit test

1. State the spectral-regrowth bandwidth result for 3rd-order nonlinearity, and derive it from the convolution theorem.
2. True or false, with a one-line reason: "A signal with low average transmit power cannot produce significant out-of-band emissions."
3. Your reasoning from §11 about the HackRF scenario, if you haven't worked through it yet.

---

**Next:** Lesson 43 turns to something this curriculum's original project brief asked for directly and hasn't been addressed yet: **portfolio packaging** — GitHub repository structure, README requirements, and how to turn each project into resume bullet points a hiring manager can actually evaluate.

---

[← Lesson 41](lesson-41-convolutional-viterbi.md) · [Table of Contents](../README.md) · [Lesson 43 →](lesson-43-portfolio-packaging.md)