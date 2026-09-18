[← Table of Contents](../README.md) · [← Lesson 11](lesson-11-channel-estimation-equalization.md)

---

# Lesson 12 — OFDM: The Complete Transmitter and Receiver

*Stage 4 — Wireless Communications, synthesis lesson. Every prior lesson converges here: the FFT (Lesson 3), convolution and its circular/linear distinction (Lessons 4 & 5), the channel as convolution (Lesson 10), and per-subcarrier equalization (Lesson 11). This is also Project 4, essentially complete by the end of this lesson.*

### 1. Intuition

Lesson 10 showed a wideband signal suffers frequency-selective fading. Lesson 11 showed that splitting into many narrow subcarriers turns that into many trivial flat-fading corrections. **OFDM** makes this concrete: instead of one high-rate stream, split your data across many low-rate parallel QAM streams (Lesson 8), each on its own narrow subcarrier, all transmitted *simultaneously* via a single IFFT. The receiver takes an FFT to separate them and equalizes each one with Lesson 11's simple per-subcarrier multiply. The genius is that these subcarriers pack together as tightly as physically possible — overlapping in spectrum, in fact — without interfering with each other. That's the "orthogonal" in OFDM, and it falls directly out of the DFT math you already derived in Lesson 3.

### 2. The engineering problem that forces this

You need a scheme that: packs subcarriers with zero wasted bandwidth between them, turns hard wideband equalization into trivial narrowband equalization (Lesson 11), is efficiently implementable via the FFT's $O(N\log N)$ (Lesson 3 — without this, a bank of $N$ separate modulators would be far too expensive to be practical at cellular scale), and handles multipath's delay spread (Lesson 10) *without* introducing a brand-new kind of interference *between* OFDM symbols. That last requirement is where the **cyclic prefix** enters — and it directly resolves a loose thread flagged all the way back in Lesson 3 §12: the DFT assumes your block is periodic, but a real channel does *linear*, not circular, convolution. This lesson is the payoff of that discrepancy.

### 3. Mathematical foundation

**OFDM transmitter:** take $N$ frequency-domain data symbols $X[0..N-1]$ (one QAM symbol per subcarrier), compute $x[n] = \text{IFFT}\{X[k]\}$, then prepend the **last $L_{cp}$ samples** of $x[n]$ to its own front, forming a transmitted block of length $N+L_{cp}$.

**OFDM receiver:** receive, **discard the first $L_{cp}$ samples**, take the FFT of the remaining $N$ samples: $Y[k] = \text{FFT}\{y[n]\}$.

**Subcarrier spacing:** $\Delta f = 1/(NT_s)$, exactly matching the DFT's native frequency resolution from Lesson 3 §4.

### 4. Formal definition

An OFDM symbol has $N$ data-bearing subcarriers, cyclic prefix length $L_{cp}$, subcarrier spacing $\Delta f = 1/(NT_s)$, and total duration $(N+L_{cp})T_s$. The cyclic prefix must satisfy $L_{cp} \ge L-1$, where $L$ is the number of channel taps — §5 proves exactly why. **Subcarrier orthogonality**: the IFFT basis functions $e^{j2\pi kn/N}$ for different $k$ are exactly orthogonal over an $N$-sample window — this is literally the same orthogonality that makes the DFT itself well-defined (Lesson 3), which is why each subcarrier's spectral leakage has zero crossings exactly at every *other* subcarrier's center frequency, letting them overlap with no crosstalk.

### 5. Derivation — why the cyclic prefix makes Lesson 11's model exactly true

This is the lesson's centerpiece, so follow it carefully. The channel performs **linear** convolution (Lesson 10): $y[n] = \sum_m x[m]\,h[n-m]$. If you transmitted the raw IFFT output $x[0..N-1]$ with no prefix, the channel's linear convolution would smear the *previous* symbol's tail into the *start* of the current one — an edge effect, exactly the linear-vs-circular convolution mismatch flagged in Lesson 3 §12 and Lesson 5 §12.

The fix: prepend the **last** $L_{cp}$ samples of $x[n]$ to its own front — not zeros, the signal's own tail, copied. Send this extended block through the channel (real, linear convolution) and then **discard the first $L_{cp}$ output samples**. Claim: what remains, exactly, is the **circular convolution** of $x[0..N-1]$ with $h$ (zero-padded to length $N$):
$$y_{kept}[n] = (x \circledast h)[n] \pmod N$$

Why: the discarded portion of the output is precisely where the channel's memory is still "filling up" with the copied prefix (garbage you throw away anyway); by the time you reach the kept samples, the channel has been convolving with a signal that — thanks to the copied prefix — looks, from the channel's point of view, exactly as if $x[n]$ repeated periodically. That's precisely the periodic-extension assumption the DFT makes about its input (Lesson 3 §5) — except here it's not an assumption anymore, it's something you *engineered to be true* by literally copying the tail to the front.

Now apply the **circular convolution theorem** (the exact, finite-length sibling of Lesson 4 §4's convolution theorem — that one was stated for infinite/linear signals; for finite-length DFTs, the precise match is specifically with *circular* convolution):
$$\text{DFT}\{x \circledast h\}[k] = X[k]\cdot H[k]$$
So:
$$Y[k] = \text{FFT}\{y_{kept}[n]\} = X[k]\cdot H[k]$$
**Exactly** — no approximation. This is precisely Lesson 11's starting assumption $Y[k]=H[k]X[k]+N[k]$, which that lesson simply asserted. Here, it's *proven*: the cyclic prefix is the specific mechanism that makes per-subcarrier equalization valid at all. The condition $L_{cp}\ge L-1$ is exactly what guarantees the discarded prefix fully absorbs the transient — too short, and some of the linear-convolution "tail" leaks into the kept samples, breaking the circular-convolution identity and reintroducing interference, now *between* OFDM symbols rather than within one.

### 6. Visualization

```
OFDM symbol with cyclic prefix:
[ copy of last Lcp samples ][           N IFFT samples            ]
 ←────── Lcp ──────→         ←──────────── N ────────────→
              ↑ discarded at RX      ↑ FFT'd at RX

Subcarrier spectra (orthogonality — each null lands on neighbors' peaks):
      ╱╲    ╱╲    ╱╲
     ╱  ╲╳╱  ╲╳╱  ╲╳╱╲     ← overlapping sinc lobes, zero crosstalk at sample points
```

### 7. Worked example

$N=8$ subcarriers, 2-tap channel (Lesson 10's example: $h=[1, 0.5e^{j\pi/3}]$, so $L=2$, requiring $L_{cp}\ge1$). Without a prefix, the *first* output sample after each symbol boundary is corrupted — it's a mix of the current symbol's start and the previous symbol's tail. With $L_{cp}=1$ (copy just the last sample to the front) and discarding exactly 1 sample at the receiver, the remaining 8 samples are *exactly* the circular convolution of $x$ with $h$ — confirmed by §5's identity, not by luck.

### 8. Code

```python
import numpy as np

N = 64                    # subcarriers
L_cp = 8                  # cyclic prefix length
h_ch = np.array([1.0, 0.5*np.exp(1j*np.pi/3), 0.2*np.exp(-1j*np.pi/4)])  # L=3 taps

# --- Transmitter ---
data = (2*np.random.randint(0,2,(N,2)).astype(float)-1)   # QPSK-ish per subcarrier
X = data[:,0] + 1j*data[:,1]
x = np.fft.ifft(X)
x_cp = np.concatenate([x[-L_cp:], x])                      # cyclic prefix, §5's copy-the-tail

# --- Channel (linear convolution, Lesson 10) + noise ---
rx = np.convolve(x_cp, h_ch, mode='full')
noise = 0.01*(np.random.randn(len(rx))+1j*np.random.randn(len(rx)))
rx = rx[:len(x_cp)] + noise                                 # truncate to symbol length for this demo

# --- Receiver ---
rx_no_cp = rx[L_cp:L_cp+N]                                  # discard CP, §5's key step
Y = np.fft.fft(rx_no_cp)
H = np.fft.fft(h_ch, N)                                     # true channel spectrum (Lesson 10's tool)

# --- Equalize (Lesson 11) ---
X_hat = Y / H                                                # ZF, since H is known exactly here
print("max recovery error:", np.max(np.abs(X_hat - X)))     # should be ~ noise floor only
```

### 9. Experiment

Run §8's code with `L_cp=8` (safely $\ge L-1=2$) and confirm `max recovery error` is at the noise floor — the circular-convolution identity holding exactly. Then set `L_cp=1` (deliberately less than $L-1=2$) and rerun: expect visibly larger, structured errors — not just noise, but genuine model-breakdown, since §5's identity no longer holds. Sweep `L_cp` from 0 up to well past `L-1` and plot error versus `L_cp` — you should see a sharp transition right around `L_cp = L-1`.

### 10. Expected output

With sufficient CP: recovery error at the noise floor, matching Lesson 11's clean per-subcarrier model. With insufficient CP: a clear, sharp jump in error once `L_cp < L-1`, confirming this is a hard threshold effect (the identity either holds exactly or it doesn't), not a gradual degradation.

### 11. Debugging exercise

Symptom, no cause given: your OFDM receiver works perfectly with a short test channel. You then test with a channel one tap longer than your fixed CP length. BER degrades — but *unlike* Lesson 11's zero-forcing failure mode (concentrated sharply at specific frequency nulls), this new degradation is spread fairly uniformly across *all* subcarriers.

Why would these two failure modes look so different? What does "spread across all subcarriers" suggest is broken, versus "concentrated at specific nulls"?

**Root cause (revealed):** these are genuinely different failure mechanisms, and the *shape* of the error is the diagnostic clue. Lesson 11's ZF failure is a **per-subcarrier** noise-amplification problem — it only shows up where $H[k]$ happens to be small, so it's concentrated. This new failure is a **structural** breakdown: an insufficient CP means §5's circular-convolution identity no longer holds *at all* — $Y[k]$ is no longer cleanly $H[k]X[k]$ for *any* subcarrier, because inter-OFDM-symbol interference has leaked into every one of them roughly equally. A concentrated failure points you toward equalizer robustness (Lesson 11's territory); a broad, uniform failure points you toward a structural/timing assumption being violated (this lesson's territory) — learning to tell these apart from the *shape* of the error, before you've even found the specific line of buggy code, is a real, transferable debugging skill.

### 12. Common mistakes

- Picking CP length without reference to the actual channel's delay spread (Lesson 10) — CP is pure overhead ($L_{cp}/(N+L_{cp})$ fraction of transmission time/bandwidth wasted), so real systems size it just long enough for the expected worst-case delay spread, not longer.
- Applying Lesson 4's convolution theorem in its *linear* form to OFDM's per-subcarrier multiply — the exact identity here is specifically with **circular** convolution, which only holds because of the cyclic prefix; conflating the two is a conceptual error, not just an implementation slip.
- Off-by-one in exactly how many samples to prepend at TX or discard at RX — an extremely common real bug; get this wrong and the FFT window is misaligned, corrupting every subcarrier even when the CP is otherwise long enough.

### 13. Real RF/cellular application

This derivation is not a simplified analogy — it is, essentially exactly, how LTE and NR downlink work: LTE uses a 2048-point (or scaled) FFT with a CP length (roughly 144 samples for "normal" CP, longer for "extended" CP in high-delay-spread environments) sized to cover typical urban/suburban multipath. NR generalizes this across multiple numerologies (different subcarrier spacings), with CP length scaling correspondingly — same underlying mathematics as this lesson, at cellular scale. You have now also substantially completed **Project 4 (OFDM Transmitter/Receiver)**.

### 14. Interview question

*"Why is the cyclic prefix specifically a copy of the end of the OFDM symbol, rather than, say, a block of zeros?"* Reason from §5: a block of zeros would prevent inter-OFDM-symbol interference too, but it would **not** reproduce circular convolution — only copying the symbol's own tail creates the periodic extension the DFT implicitly assumes (Lesson 3 §5), turned from an assumption into an engineered fact. Zeros solve a different, easier problem; only the specific "copy the tail" choice makes $Y[k]=H[k]X[k]$ exact.

### 15. Active recall

1. What specific condition must $L_{cp}$ satisfy relative to the channel, and what breaks if it's violated?
2. Why does discarding the first $L_{cp}$ received samples turn linear convolution into circular convolution?
3. Why is OFDM's per-subcarrier equalization essentially free computationally, once you already have the FFT?

### 16. Mini assignment

Using §8's code, sweep `L_cp` and plot recovery error against it for a fixed 3-tap channel, confirming the sharp threshold at `L_cp = L-1` predicted in §9. Then compute the CP overhead fraction for LTE's real parameters ($N=2048$, $L_{cp}\approx144$) and for a hypothetical NR numerology with double the subcarrier spacing (roughly half the useful symbol duration) — how does overhead scale?

### 17. Exit test

1. State, precisely, the condition on $L_{cp}$ relative to the channel that makes $Y[k]=H[k]X[k]$ exact.
2. True or false, with a one-line reason: "A longer cyclic prefix always improves system performance."
3. Your reasoning from §11 distinguishing the two failure-mode shapes, if you haven't sent it.

---

**Next:** Lesson 13 tackles what a clean OFDM derivation quietly assumed away: **timing offset and carrier frequency offset (CFO)**. Get either wrong and orthogonality — the very thing this lesson proved — breaks down for reasons that have nothing to do with the channel at all.

---

[← Lesson 11](lesson-11-channel-estimation-equalization.md) · [Table of Contents](../README.md) · [Lesson 13 →](lesson-13-timing-cfo.md)