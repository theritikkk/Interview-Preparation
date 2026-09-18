[← Table of Contents](../README.md) · [← Lesson 7](lesson-07-awgn-bpsk-ber.md)

---

# Lesson 8 — QPSK and M-ary QAM: The Spectral Efficiency Trade-off

*Stage 3 — Digital Communications, continued. Where "more bits per symbol" splits into one free upgrade and one that costs you real SNR.*

### 1. Intuition

BPSK spends an entire symbol to send just 1 bit. QPSK notices something clever: a complex baseband signal genuinely has **two independent dimensions** — I and Q — so you can run two completely separate BPSK streams simultaneously, one on each axis, and get 2 bits per symbol essentially **for free**. Push further, though: instead of just $\pm1$ on each axis, use *more* amplitude levels per axis (M-QAM) to pack even more bits into one symbol. Now you pay a real price — for the same average transmit power, the constellation points crowd closer together, making them easier to confuse in noise. This lesson is about that one genuine free lunch, and then the real, quantifiable trade-off that begins right after it.

### 2. The engineering problem that forces this

Spectral efficiency — bits per second per Hz of bandwidth — is one of the central currencies of wireless engineering; every cellular generation fights hard for more of it within the same spectrum. Given a fixed channel bandwidth, the lever you have is bits per symbol. But every real system also has a reliability target (a maximum tolerable error rate) and a limited SNR budget. So the engineering question is precise: is there a way to get more bits/symbol with *zero* reliability cost (as QPSK does over BPSK)? And beyond that free lunch, exactly how much extra $E_b/N_0$ does each additional bit/symbol cost — the number that literally defines every row of an LTE/NR MCS table?

### 3. Mathematical foundation

**QPSK**: two independent BPSK streams on I and Q, symbol energy $E_s = 2E_b$ (2 bits/symbol).

**M-QAM** (square constellation, $M=2^{2k}$, $\sqrt{M}$ levels per axis at $\pm1,\pm3,\dots,\pm(\sqrt M-1)$): average symbol energy
$$E_{s,\text{avg}} = \frac{2(M-1)}{3}$$
**Approximate BER** (Gray-coded, high-SNR):
$$P_b \approx \frac{4}{\log_2 M}\left(1-\frac{1}{\sqrt M}\right) Q\!\left(\sqrt{\frac{3\log_2 M}{M-1}\cdot\frac{E_b}{N_0}}\right)$$

### 4. Formal definition

QPSK maps 2 bits to one of 4 points $(\pm1\pm j)/\sqrt2 \cdot \sqrt{E_s}$, with bits assigned so that **adjacent constellation points differ by exactly one bit** — this is **Gray coding**, and it matters because the most likely error (noise pushing you to your *nearest* neighbor) then costs only 1 bit, not 2. M-QAM extends this to a $\sqrt M \times \sqrt M$ grid, with each axis independently Gray-coded across its $\sqrt M$ amplitude levels — effectively two independent $\sqrt M$-level PAM streams on I and Q. **Spectral efficiency** is $\log_2 M$ bits/symbol.

### 5. Derivation

**(a) Why QPSK's BER exactly equals BPSK's.** From Lesson 7, complex-baseband AWGN has independent, zero-mean Gaussian noise on I and Q, each with variance $N_0/2$. Because the QPSK constellation is literally the Cartesian product of two independent BPSK constellations (I decides bit 0, Q decides bit 1, with no interaction between them), the receiver's decisions on I and Q are two completely independent BPSK detection problems. Each dimension carries $E_s/2$ of energy; since $E_s = 2E_b$ for QPSK, each dimension carries exactly $E_b$ — the identical per-bit energy as the Lesson 7 BPSK case, in the identical noise variance. So each independent sub-decision has **exactly** Lesson 7's BER:
$$P_{b,\text{QPSK}} = Q\!\left(\sqrt{\frac{2E_b}{N_0}}\right) = P_{b,\text{BPSK}}$$
Identical formula. You get double the data rate for the same reliability, because I and Q are genuinely orthogonal — this isn't an approximation, it's exact.

**(b) The M-QAM average-energy formula.** For one axis with levels $\pm1,\pm3,\dots,\pm(L-1)$ ($L=\sqrt M$, equally likely), average energy per axis is $\tfrac1L\sum_{i=1}^{L/2} 2\cdot(2i-1)^2$. Using the standard sum-of-odd-squares identity $\sum_{i=1}^{n}(2i-1)^2 = \tfrac{n(2n-1)(2n+1)}{3}$ with $n=L/2$ simplifies this to $(L^2-1)/3$. Two independent, identical axes double it: $E_{s,\text{avg}} = \tfrac23(L^2-1) = \tfrac23(M-1)$.

**(c) The general BER formula — sketched, then checked, not fully re-derived.** The full derivation follows a nearest-neighbor union-bound argument (each interior point has 4 nearest neighbors, edge points fewer — the $(1-1/\sqrt M)$ factor accounts for that boundary effect) combined with Gray coding (each symbol error costs, on average, close to 1 bit error out of $\log_2 M$, giving the $4/\log_2 M$ scaling). Rather than carry that derivation in full, check it the way you should always check an unfamiliar formula: **does it collapse to something you already proved at a known special case?** At $M=4$: prefactor $=\tfrac{4}{2}(1-\tfrac12)=1$, and the $Q(\cdot)$ argument becomes $\sqrt{\tfrac{3\cdot2}{3}\cdot\tfrac{E_b}{N_0}} = \sqrt{2E_b/N_0}$ — **exactly** the QPSK/BPSK formula from part (a). That the general formula reduces perfectly to an independently-derived exact result at $M=4$ is strong evidence it's correct, and it's the right instinct to apply any time you're handed a formula you haven't derived yourself.

### 6. Visualization

QPSK, Gray-coded (adjacent points differ by 1 bit):
```
        Q
        |
   01   |   11
        |
--------+--------  I
        |
   00   |   10
        |
```
16-QAM (4×4 grid, each axis independently Gray-coded across its 4 levels):
```
Q= 3  ●    ●    ●    ●
Q= 1  ●    ●    ●    ●
Q=-1  ●    ●    ●    ●
Q=-3  ●    ●    ●    ●
     I=-3  -1   1    3
```

### 7. Worked example

16-QAM average energy: $E_{s,\text{avg}} = \tfrac23(16-1) = 10$ (in units where adjacent levels are 2 apart). At $E_b/N_0 = 10$ dB, QPSK's BER is $\approx 3.9\times10^{-6}$ (same as BPSK, from Lesson 7 §7). Plugging into §3's M-QAM formula for 16-QAM at the same $E_b/N_0=10$ dB: prefactor $=\tfrac{4}{4}(1-\tfrac14)=0.75$, argument $=\sqrt{\tfrac{3\cdot4}{15}\cdot10} = \sqrt{8}=2.83$, giving $P_b \approx 0.75\times Q(2.83) \approx 0.75\times0.0023 \approx 1.7\times10^{-3}$ — **roughly 450× worse** than QPSK at the identical $E_b/N_0$. That gap, made concrete with real numbers, is the entire spectral-efficiency-vs-reliability trade-off in one comparison.

### 8. Code

```python
import numpy as np
from scipy.special import erfc

def q_function(x):
    return 0.5 * erfc(x / np.sqrt(2))

def qpsk_ber_sim(ebn0_db, num_symbols=10**6):
    bits_i = np.random.randint(0, 2, num_symbols)
    bits_q = np.random.randint(0, 2, num_symbols)
    sym_i, sym_q = 2*bits_i - 1, 2*bits_q - 1     # independent BPSK on each axis

    ebn0_linear = 10**(ebn0_db/10)
    noise_std = np.sqrt(1/(2*ebn0_linear))         # same per-dimension noise as Lesson 7
    rx_i = sym_i + noise_std*np.random.randn(num_symbols)
    rx_q = sym_q + noise_std*np.random.randn(num_symbols)

    dec_i, dec_q = (rx_i > 0).astype(int), (rx_q > 0).astype(int)
    errors = np.sum(dec_i != bits_i) + np.sum(dec_q != bits_q)
    return errors / (2*num_symbols)                # total bits = 2 per symbol

for e in range(0, 11, 2):
    print(f"Eb/N0={e} dB   QPSK sim={qpsk_ber_sim(e):.2e}   "
          f"BPSK theory={q_function(np.sqrt(2*10**(e/10))):.2e}")
```

### 9. Experiment

Run §8's code and confirm the QPSK simulated points land on the *same* curve as Lesson 7's BPSK theory line — direct empirical confirmation of the exact-equivalence result from §5(a). Then implement 16-QAM: map 4 bits to a Gray-coded $(I,Q)$ pair from $\{-3,-1,1,3\}$ on each axis, normalize so the *average* symbol energy matches your target $E_b/N_0$ (using §5(b)'s formula), add noise, do nearest-level decisions on each axis independently, and compare the resulting BER curve against both QPSK and the §3 approximate formula.

### 10. Expected output

QPSK's simulated curve overlaps BPSK's theoretical curve essentially exactly. 16-QAM's curve sits visibly to the *right* of QPSK's on a semilog plot — needing several more dB of $E_b/N_0$ to hit the same BER, quantitatively matching §7's numeric example.

### 11. Debugging exercise

Symptom, no cause given: you implement 16-QAM, reusing your working QPSK per-axis code. The resulting BER isn't just "somewhat worse than theory" — it's dramatically bad, close to what you'd expect from random guessing, even at high $E_b/N_0$ where errors should be rare.

What would you check? (Hint: "almost random guessing" even with high SNR is a very different kind of bug from "a few dB worse than expected.")

**Root cause (revealed):** a performance gap of a fraction of a dB is what you'd get from a missing Gray-coding optimization — annoying, but not catastrophic. "Almost random, regardless of SNR" is a different class of bug entirely: it means bits are wrong **even when the channel barely corrupted anything** — which points to a **mismatched mapping table between transmitter and receiver**. If your modulator maps bits→levels using one table (say, Gray-coded) and your demodulator maps levels→bits using a *different* table (say, natural binary, or a table with the levels in a different order), then even a perfect, noiseless channel produces wrong bits. Always verify your mapper and demapper are exact inverses of each other with a noiseless loopback test *before* adding any noise to the simulation — that isolates mapping bugs from channel/noise bugs completely.

### 12. Common mistakes

- Confusing two very different severity levels: **no Gray coding** (real but modest penalty, typically well under 1 dB at practical operating points) vs. a **mismatched TX/RX mapping table** (catastrophic, as in §11) — these can look superficially similar ("my QAM performs worse than expected") but have completely different root causes and fixes.
- Forgetting to **normalize average symbol energy** (§5(b)) when comparing different $M$ at the same $E_b/N_0$ — using raw, unnormalized levels silently changes the effective SNR and invalidates the comparison.
- Conflating **symbol error rate (SER)** with **bit error rate (BER)** — for $M>4$, one symbol error can correspond to more than one bit error depending on which neighbor was mistaken for the true symbol, so the two metrics diverge as $M$ grows.

### 13. Real RF/cellular application

This lesson is, quite literally, the physics behind every row of every LTE and NR MCS table: QPSK, 16-QAM, 64-QAM, 256-QAM (and 1024-QAM in later NR releases) form exactly this family, each requiring progressively higher SNR/CQI (channel quality indicator) to be usable reliably. **Link adaptation** (Stage 7/8, tied to HARQ) is the real-time algorithm that picks the modulation-and-code-rate combination matching the UE's currently reported channel quality, to maximize throughput while meeting a target BLER — every one of those decisions is grounded in curves exactly like the ones you just built.

### 14. Interview question

*"Why does QPSK give you double the data rate of BPSK with zero BER penalty, but 16-QAM costs real $E_b/N_0$ compared to QPSK?"* Answer from §5: QPSK exploits a genuinely new, orthogonal, non-interfering dimension (I and Q don't talk to each other) — that's a real free lunch. 16-QAM instead packs more levels into the *same* two dimensions, shrinking the distance between neighboring points for the same average power — that's a fundamentally different kind of "more bits," and it directly costs noise margin.

### 15. Active recall

1. Why is QPSK's BER *exactly* equal to BPSK's, not just approximately similar?
2. What does the $(1-1/\sqrt M)$ factor in the M-QAM BER formula account for physically?
3. How did checking the formula at $M=4$ serve as a sanity check in §5(c)?

### 16. Mini assignment

Extend §9's simulation to 64-QAM. At a target BER of $10^{-4}$, read off (from your plotted curves, or from the formula in §3) roughly how many extra dB of $E_b/N_0$ 64-QAM needs compared to 16-QAM, and how many extra dB 16-QAM needs compared to QPSK. These two numbers are worth having real intuition for — they show up constantly in link-budget reasoning.

### 17. Exit test

1. At the same $E_b/N_0$, which has lower BER: QPSK or 16-QAM? By roughly how much (order of magnitude), per §7?
2. True or false, with a one-line reason: "Gray coding changes the average symbol error rate of a constellation."
3. Your hypothesis from §11, if you haven't sent it.

---

**Next:** Lesson 9 introduces pulse shaping — root-raised-cosine filtering — which is where the "instantaneous symbol" model used in this and the last lesson becomes an actual continuous-time transmitted waveform, and where inter-symbol interference first enters the picture.

---

← [Lesson 7](lesson-07-awgn-bpsk-ber.md) · [Table of Contents](../README.md)  ·  [Lesson 9 →](lesson-09-pulse-shaping-rrc.md)