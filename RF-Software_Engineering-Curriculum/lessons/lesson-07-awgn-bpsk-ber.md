[← Table of Contents](../README.md) · [← Lesson 6](lesson-06-correlation-matched-filter.md)

---

# Lesson 7 — AWGN, Eb/N0, and Your First BER Curve (BPSK)

*Opens Stage 3 — Digital Communications. This is where matched filtering (Lesson 6) becomes a real receiver, and where "communication theory" turns into a curve you can actually simulate and watch fail gracefully.*

### 1. Intuition

Every real channel corrupts a transmitted signal with random noise. The simplest, most foundational noise model in all of communications theory is **AWGN** — additive (it adds to the signal), white (equal power across all frequencies), Gaussian (its amplitude follows a normal distribution). **BPSK** is the simplest possible digital modulation: map each bit to one of two symbol values (say, $+1$ or $-1$), transmit it, and let the receiver decide which was more likely sent given what actually arrived. Put these together and you get something genuinely powerful: the ability to *predict*, mathematically, exactly how often the receiver will guess wrong at a given noise level — and then *verify that prediction by simulation*. That predicted-vs-simulated curve is the standard yardstick — the **BER curve** — used to evaluate literally every digital communication system you'll touch in this field.

### 2. The engineering problem that forces this

How do you characterize a link's quality in a way that lets you fairly compare BPSK against QPSK, or one channel condition against another? Raw SNR isn't good enough — it depends on bandwidth and symbol rate, which differ across schemes and configurations, so two systems can have identical raw SNR and wildly different actual performance. You need a rate-independent measure: **energy per information bit, divided by noise power spectral density** — $E_b/N_0$. Combine that with an optimal receiver decision rule (which, not coincidentally, is exactly what Lesson 6's matched filter provides) and you can predict bit-error rate analytically, then confirm it in code. That prediction-then-verification loop is precisely what Project 1 (Digital Modulation Laboratory) in your curriculum needs.

### 3. Mathematical foundation

**AWGN channel:** $r[n] = s[n] + w[n]$, where $w[n]$ is Gaussian noise, zero mean, with power spectral density $N_0$ (for complex baseband, split as variance $N_0/2$ on each of I and Q).

**$E_b/N_0$:** energy per transmitted bit, divided by the noise power spectral density. For BPSK (1 bit per symbol), symbol energy $E_s$ equals bit energy $E_b$.

**Theoretical BER for BPSK:**
$$P_b = Q\!\left(\sqrt{\frac{2E_b}{N_0}}\right), \qquad Q(x) = \frac{1}{2}\,\text{erfc}\!\left(\frac{x}{\sqrt{2}}\right)$$

### 4. Formal definition

The **$Q$-function** is the tail probability of a standard normal distribution: $Q(x) = P(Z > x)$ for $Z \sim N(0,1)$. The **optimal (maximum-likelihood) BPSK receiver**, for equally-likely, equal-energy antipodal symbols $\pm\sqrt{E_s}$ in Gaussian noise, is the simplest possible decision rule: decide bit $=1$ if the received (matched-filtered) sample $r>0$, else decide bit $=0$. This decision boundary — literally just "which side of zero" — is what a matched filter (Lesson 6) reduces the entire detection problem to, once you've correlated against the known pulse shape.

### 5. Derivation

Transmit $s = +\sqrt{E_s}$ (bit 1) or $s=-\sqrt{E_s}$ (bit 0). After the optimal receiver (matched filter, Lesson 6), the decision statistic is $r = s + n$, with $n \sim N(0,\sigma^2)$, $\sigma^2 = N_0/2$. Suppose $+\sqrt{E_s}$ was sent. An error happens if the noise is negative enough to push $r$ below zero: $P(\text{error}\mid +) = P(n < -\sqrt{E_s})$. By the symmetry of a zero-mean Gaussian, $P(n<-\sqrt{E_s}) = P(n>\sqrt{E_s}) = Q(\sqrt{E_s}/\sigma)$. Substitute $\sigma = \sqrt{N_0/2}$:
$$Q\!\left(\frac{\sqrt{E_s}}{\sqrt{N_0/2}}\right) = Q\!\left(\sqrt{\frac{2E_s}{N_0}}\right) = Q\!\left(\sqrt{\frac{2E_b}{N_0}}\right)$$
(using $E_s=E_b$ for BPSK). By symmetry, sending $-\sqrt{E_s}$ gives the identical error probability, so this is the overall bit error rate — not an approximation, the exact answer for this channel and receiver, following directly from Lesson 6's optimal-detector result applied to a binary decision.

### 6. Visualization

```
BPSK constellation, with noise cloud around each point:
        (noise cloud)          (noise cloud)
    ●●●●●●●●●              ●●●●●●●●●
  ●●●● -√Es ●●●    |    ●●●● +√Es ●●●
    ●●●●●●●●●       ↑      ●●●●●●●●●
                 decision
                 boundary (r=0)
```
An error occurs whenever noise pushes a point across the boundary at $r=0$ — the $Q$-function is exactly the area of the Gaussian noise cloud that spills across that line.

### 7. Worked example

At $E_b/N_0 = 0$ dB (linear $=1$): $Q(\sqrt{2})=Q(1.414)\approx 0.0786$. At $5$ dB (linear $\approx3.162$): $Q(\sqrt{6.324})=Q(2.515)\approx 0.0060$. At $10$ dB (linear $=10$): $Q(\sqrt{20})=Q(4.472)\approx 3.9\times10^{-6}$. Notice the shape: roughly a 5 dB improvement takes you from $\sim$8% errors to $\sim$0.0004% errors — BER drops *extremely* fast (nearly exponentially) with SNR, which is exactly why a "few extra dB" matters so much in real link budgets.

### 8. Code

```python
import numpy as np
from scipy.special import erfc

def q_function(x):
    return 0.5 * erfc(x / np.sqrt(2))

def bpsk_ber_sim(ebn0_db, num_bits=10**6):
    bits = np.random.randint(0, 2, num_bits)
    symbols = 2*bits - 1                          # 0->-1, 1->+1  (unit energy, Es=1)

    ebn0_linear = 10**(ebn0_db/10)
    noise_std = np.sqrt(1/(2*ebn0_linear))         # sigma = sqrt(N0/2), with Eb=1
    noise = noise_std * np.random.randn(num_bits)

    rx = symbols + noise
    decided_bits = (rx > 0).astype(int)            # the ML decision rule from §4

    errors = np.sum(decided_bits != bits)
    return errors / num_bits

ebn0_range = np.arange(0, 11, 1)
sim_ber = [bpsk_ber_sim(e) for e in ebn0_range]
theory_ber = [q_function(np.sqrt(2 * 10**(e/10))) for e in ebn0_range]

for e, s, t in zip(ebn0_range, sim_ber, theory_ber):
    print(f"Eb/N0={e:2d} dB   simulated={s:.2e}   theory={t:.2e}")
```

### 9. Experiment

Run the sweep above and plot both curves with the y-axis on a **log scale** (`plt.semilogy`) — this is the standard way BER curves are always presented, precisely because BER spans many orders of magnitude. At low $E_b/N_0$ you'll get plenty of errors and a tight match to theory; at high $E_b/N_0$ (say, 10+ dB) you may see zero errors out of $10^6$ bits — that's not a bug, it's a sample-size problem addressed in §12.

### 10. Expected output

Simulated points should closely track the theoretical $Q$-function curve at every $E_b/N_0$ where enough errors occurred to estimate BER reliably; deviation grows at very low BER simply from having too few error events to average over.

### 11. Debugging exercise

Symptom, no cause given: your simulated BER curve is consistently *worse* than theory at every single $E_b/N_0$ point — not wildly, but by a fairly consistent factor, as if every point were shifted a couple dB in the wrong direction.

What would you check first? (Hint: there's exactly one line in §8's code where getting a factor of 2 wrong would produce precisely this symptom.)

**Root cause (revealed):** this is almost always the noise variance formula — using `noise_std = np.sqrt(1/ebn0_linear)` (missing the `/2`) instead of the correct `np.sqrt(1/(2*ebn0_linear))` injects noise with twice the intended power, which directly worsens every measured BER by a roughly constant amount on the log scale (matching "shifted a couple dB"). This single `/2` — tracing back to $\sigma^2=N_0/2$ in §5's derivation — is the most common bug when first implementing an AWGN simulation, precisely because it's easy to write down $N_0$ where you meant $N_0/2$.

### 12. Common mistakes

- Confusing $E_b/N_0$ with raw SNR — they coincide only under specific bandwidth/rate assumptions; $E_b/N_0$ is the standard rate-independent way to compare schemes, and treating them as interchangeable produces confidently wrong comparisons.
- The $\sigma^2 = N_0/2$ factor from §11 — the single most common implementation bug in this entire area.
- **Not running enough trials at high SNR.** Measuring zero errors out of $10^4$ bits at high $E_b/N_0$ does not mean BER is truly zero — it means you haven't generated enough trials to see one. A standard rule of thumb: you need on the order of 100+ observed bit errors before an estimated BER is statistically trustworthy, which means simulating very low-BER points requires very large bit counts.
- Forgetting to convert dB to linear (`10**(db/10)` for power ratios) before using a value in a formula — an easy, very common off-by-a-log slip.

### 13. Real RF/cellular application

This is not an academic exercise — BER (and the closely related BLER, block error rate) curves like this are literally how link budgets and **modulation and coding scheme (MCS) tables** in LTE and NR are built: the spec defines what SNR/$E_b/N_0$ a given modulation+code-rate combination needs to hit a target error rate, and **link adaptation** (which you'll meet properly in Stage 7/8, tied to HARQ) is entirely the real-time process of picking the best MCS for current channel conditions using exactly this kind of relationship. You've also just built the BPSK case of **Project 1 (Digital Modulation Laboratory)** — QPSK and QAM in the next couple of lessons complete it.

### 14. Interview question

*"Why do communications engineers use $E_b/N_0$ instead of raw SNR when comparing different modulation schemes?"* Answer from §2 — SNR is bandwidth/rate-dependent and can make two genuinely different-performing systems look identical (or vice versa); $E_b/N_0$ normalizes to "cost in energy per bit of information," which is the fair, scheme-independent comparison.

### 15. Active recall

1. Write the theoretical BER formula for BPSK from memory, then check it against §3.
2. Why does $\sigma^2 = N_0/2$ rather than $\sigma^2 = N_0$ in the derivation?
3. Why is measuring "zero errors" at high SNR not the same as "BER is zero"?

### 16. Mini assignment

Extend §8's sweep down to $E_b/N_0 = -2$ dB and up to $12$ dB. At the highest SNR points, increase `num_bits` as needed until you consistently observe at least ~50 errors, and note how large `num_bits` has to get — this is a direct, hands-on feel for §12's "not enough trials" trap.

### 17. Exit test

1. At $E_b/N_0 = 7$ dB, is BER closer to $10^{-2}$, $10^{-3}$, or $10^{-4}$? (Estimate using the trend from §7, don't just compute exactly.)
2. True or false, with a one-line reason: "Doubling $E_b/N_0$ (in linear terms) roughly doubles how good the BER is."
3. Your debugging hypothesis from §11, if you haven't sent it.

---

**Next:** Lesson 8 extends this same machinery to QPSK and general M-ary QAM — same channel, same optimal-detector logic, more bits per symbol, and a real trade-off between spectral efficiency and required $E_b/N_0$.

---

[← Lesson 6](lesson-06-correlation-matched-filter.md) · [Table of Contents](../README.md)  ·  [Lesson 8 →](lesson-08-qpsk-mqam.md)