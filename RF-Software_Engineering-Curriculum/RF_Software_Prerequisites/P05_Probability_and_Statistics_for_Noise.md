# P05 — Probability and Statistics for Noise: Random Variables, the Gaussian, the Q-Function, Monte Carlo, Confidence

*Prerequisite module 5 · ⏱ about 6–7 hours · assumes P01–P04 · every number and output below was computed and checked, including a million-bit BPSK simulation.*

> **Where this unlocks the curriculum.** Lesson 7 builds BER theory from the **Gaussian**, the **Q-function** and the rule that noise variance is **N₀/2**; Lesson 10 explains **Rayleigh fading via the Central Limit Theorem**; Lessons 35–36 use **binomial proportions, standard error, z-scores, percentiles**. This module gives you those tools so Lesson 7 is a derivation you follow rather than a formula you memorise.

---

## 5.1 Why probability is unavoidable in radio

* **Noise is random.** You can't predict the next noise sample, but you can describe *how it is distributed*.
* **BER is a probability.** "BER = 10⁻³" means each bit has a 0.1 % chance of being wrong.
* **Fading is random.** The channel gain itself is a random variable (Lesson 10).
* **Measurement is random.** Any error rate you *measure* has uncertainty; you must know how much to trust it.

---

## 5.2 Events, probability, independence

The **probability** of an event is a number in $[0,1]$: 0 = impossible, 1 = certain. For a repeatable experiment it is the long-run fraction of times the event occurs.

* **Complement:** $P(\text{not }A)=1-P(A)$. *At least one head in 3 fair tosses:* $1-0.5^{3}=\mathbf{0.875}$.
* **Independent events:** $P(A\text{ and }B)=P(A)P(B)$. Two fair tosses both heads: $0.5\times0.5=\mathbf{0.25}$ (this is diagnostic Q10 in `00_START_HERE`). Two independent bit errors, each with probability $10^{-3}$: $\mathbf{10^{-6}}$.
* **Conditional probability:** $P(A\mid B)=P(A\text{ and }B)/P(B)$ — "given that $B$ happened".

*Meaning:* if bit errors on separate bits are independent (an assumption — fading bursts break it!), probabilities of combinations multiply. This is why **interleaving** (spreading bursts out, college notes Ch. 9) helps coding.

---

## 5.3 Random variables, PMF, PDF, CDF

A **random variable** (RV) $X$ is a number produced by a random experiment.

| Kind | Example | Described by |
|---|---|---|
| **Discrete** | bit value, number of errors in a block | PMF $P(X=k)$ |
| **Continuous** | noise voltage, channel amplitude | PDF $f_X(x)$ (probabilities are *areas*) |

* **PDF $f_X(x)$:** the probability that $X$ lies in $[a,b]$ is $\int_a^b f_X(x)\,dx$ (P03). The PDF *value* at a point is **not** a probability (it can exceed 1); only areas are.
* **CDF $F_X(x)=P(X\le x)$:** running total of the PDF, rising from 0 to 1.
* **Histogram ≈ PDF.** Collect many samples, count how many fall in each bin; as samples → ∞ the normalised histogram approaches the PDF. This is how Lesson 10 checks its Rayleigh simulation.

**Bernoulli / binomial** — the model for bits. A bit error is a Bernoulli RV (error with probability $p$). The number of errors in $n$ independent bits is **Binomial($n,p$)**. *Verified:* $n=10$, $p=0.1$: $P(0\text{ errors})=\mathbf{0.349}$, $P(1)=\mathbf{0.387}$, $P(2)=\mathbf{0.194}$.

**Uniform** — equally likely over an interval; a random carrier phase is uniform on $[0,2\pi)$. For $U[0,1]$: mean $1/2$, variance $1/12=0.0833$ (simulated: 0.4998 and 0.0832 ✓).

---

## 5.4 Mean, variance, standard deviation 🔥

* **Mean (expected value)** $\mu=E[X]$: the long-run average.
* **Variance** $\sigma^{2}=E[(X-\mu)^{2}]$: average squared deviation — the "spread". **Standard deviation** $\sigma=\sqrt{\sigma^{2}}$.
* **For zero-mean noise, variance = average power:** $\sigma^{2}=E[X^{2}]$. This is why "noise power" and "variance" are used interchangeably.

**Rules (all verified by simulation with $X\sim N(0,2^{2})$, $Y\sim N(0,3^{2})$):**

| Rule | Check |
|---|---|
| $E[aX+b]=a\mu+b$ | – |
| $\operatorname{Var}(aX)=a^{2}\operatorname{Var}(X)$ | Var(3X) = 36.05 ≈ 36 ✓ |
| **Independent** $X,Y$: $\operatorname{Var}(X+Y)=\operatorname{Var}X+\operatorname{Var}Y$ | 12.98 ≈ 4 + 9 = 13 ✓ |

*Meaning:* **independent noise adds in power (variance), not in amplitude** — the same fact as "powers add" in P01 (two 0 dBm signals → +3 dBm).

---

## 5.5 The Gaussian (normal) distribution 🔥

$$f(x)=\frac1{\sqrt{2\pi}\,\sigma}\exp\!\Bigl(-\frac{(x-\mu)^{2}}{2\sigma^{2}}\Bigr),\qquad X\sim N(\mu,\sigma^{2})$$

A bell curve centred at $\mu$ with width $\sigma$. The **standard normal** is $Z\sim N(0,1)$; any Gaussian is $X=\mu+\sigma Z$.

**The 68–95–99.7 rule** (verified): within ±1σ → **68.27 %**, ±2σ → **95.45 %**, ±3σ → **99.73 %**.

**Why noise is Gaussian — the Central Limit Theorem (CLT).** The sum of many independent random contributions (each small and of any reasonable distribution) tends to a Gaussian. Thermal noise is the sum of countless random electron motions ⇒ Gaussian. **Sums of independent Gaussians are Gaussian, with means and variances adding.**

---

## 5.6 The Q-function — how BER gets its formula 🔥

$$Q(x)=P(Z>x)=\int_x^{\infty}\frac1{\sqrt{2\pi}}e^{-t^{2}/2}\,dt=\tfrac12\operatorname{erfc}\!\Bigl(\frac{x}{\sqrt2}\Bigr)$$

The **tail probability** of a standard normal: how likely noise pushes a value more than $x$ standard deviations above the mean. *Verified table:*

| $x$ | 0 | 1 | 2 | 3 | 4 | 5 |
|---|---|---|---|---|---|---|
| $Q(x)$ | 0.5 | 0.1587 | 0.02275 | 0.001350 | $3.17\times10^{-5}$ | $2.87\times10^{-7}$ |

### Where BPSK's BER comes from (Lesson 7, in four lines)
Send $s=\pm\sqrt{E_b}$. The receiver sees $r=s+n$ with $n\sim N(0,\sigma^{2}=N_0/2)$. If $+\sqrt{E_b}$ was sent, an error happens when noise drags $r$ below 0: $P(n<-\sqrt{E_b})=Q\bigl(\sqrt{E_b}/\sqrt{N_0/2}\bigr)$, so

$$\boxed{P_b=Q\!\Bigl(\sqrt{\tfrac{2E_b}{N_0}}\Bigr)}$$

*Verified against the curriculum's own numbers:* $E_b/N_0=0$ dB → $Q(1.414)=\mathbf{0.0786}$; 5 dB → $Q(2.515)=\mathbf{0.00595}$; 10 dB → $Q(4.472)=\mathbf{3.87\times10^{-6}}$; and 7 dB → **$7.7\times10^{-4}$** (closest to $10^{-3}$).

*Meaning:* BER falls **extremely fast** (exponentially in $E_b/N_0$) in AWGN — contrast with the Rayleigh-fading $1/(4\bar\gamma)$ of the college notes (Ch. 6), which is why fading is so expensive.

---

## 5.7 Noise in IQ samples: the N₀/2 rule 🔥

Baseband noise on a complex sample has an **I part and a Q part, independent**, each $N(0,N_0/2)$. So the **total complex noise power is $N_0$** and each real dimension has variance $N_0/2$.

*Verified* for $N_0=1$: $E|w|^{2}=\mathbf{0.9998}$, Var(I) = 0.4997, Var(Q) = 0.5001.

**The classic bug (Lesson 7's "root cause"):** using `std = sqrt(1/ebn0)` instead of `sqrt(1/(2*ebn0))`. *Verified:* at $E_b/N_0=4$ dB the correct simulation gave BER 0.0123 (theory 0.0125) while the buggy noise gave **0.0568** — noise power doubled, a 3 dB shift, ~4.5× more errors.

---

## 5.8 From sums of phasors to Rayleigh fading (Lesson 10 preview) 🔥

Many unresolved multipath components with random phases add as arrows (P02). By the CLT, the real and imaginary parts of the sum are Gaussian, so the **envelope $|h|$ is Rayleigh** and the **power $|h|^{2}$ is exponentially distributed**.

*Verified experiment (Lesson 10's recipe):* sum 20 unit phasors with uniform random phase, 100,000 trials. Mean power $E|h|^{2}=\mathbf{20.1}$ (theory 20); mean amplitude $\mathbf{3.99}$ vs Rayleigh theory $\sigma\sqrt{\pi/2}=3.96$ with $\sigma^{2}=10$; variance of the real part **10.1** (theory 10).
**Exponential-power facts (relative to the mean power):** $P(\text{power}>\text{mean})=e^{-1}=0.368$ (simulated 0.374); **$P(\text{power}<0.1\times\text{mean})=1-e^{-0.1}=0.0952$** (simulated 0.0916 — slightly off because 20 phasors only approximate the CLT).

> **Link to your college notes:** that 9.5 % is exactly the "deep-fade probability $p=1-e^{-S_i/S_r}$ with $S_i/S_r=0.1$" in the Diversity chapter (Ch. 7). Two independent branches both fade with $p^{2}=0.91\,\%$ — the same independence rule as §5.2.

---

## 5.9 Monte Carlo simulation and the law of large numbers 🔥

**Idea:** estimate a probability by simulating the experiment many times and counting. By the **law of large numbers** the observed fraction converges to the true probability.

**Verified BPSK Monte-Carlo** (bits ±1, noise std $\sqrt{1/(2E_b/N_0)}$):

| $E_b/N_0$ | bits | errors | simulated BER | theory $Q(\sqrt{2E_b/N_0})$ |
|---|---|---|---|---|
| 0 dB | 2,000,000 | 157,111 | 0.07856 | 0.07865 |
| 4 dB | 2,000,000 | 24,522 | 0.01226 | 0.01250 |
| 6 dB | 4,000,000 | 9,456 | 0.00236 | 0.00239 |

**How many errors is enough?** The number of errors is binomial; its relative uncertainty is about $1/\sqrt{\text{errors}}$. With **100 errors → ±10 %**; with 10 → ±32 %; with 0 → *no information*. This is the origin of Lesson 7's rule of thumb that you need **~100+ observed errors** before trusting a BER estimate, so very low BERs need huge simulations.

---

## 5.10 Measuring rates honestly: standard error, confidence intervals, z-scores 🔥

A measured error proportion $\hat p=\text{errors}/N$ has a **standard error**

$$\mathrm{SE}=\sqrt{\frac{\hat p(1-\hat p)}{N}}\qquad\text{(normal approximation, valid when you have enough errors)}$$

* **95 % confidence interval:** $\hat p\pm1.96\,\mathrm{SE}$. *Verified:* 100 errors in $10^{5}$ bits → $\hat p=10^{-3}$, SE $=1.0\times10^{-4}$ (relative **10 %**), interval **[8.04×10⁻⁴, 1.20×10⁻³]**.
* **z-score for a regression test (Lesson 35):** $z=(\hat p-p_0)/\mathrm{SE}$. *Verified with the lesson's numbers:* baseline $p_0=0.01$, $N=10^{5}$, new run $\hat p=0.0105$ ⇒ SE $=3.15\times10^{-4}$, $z=\mathbf{1.59}$ — **below 1.96, so not statistically significant at 95 %**: you cannot claim the change made BER worse.
* **Zero errors observed ≠ BER of zero.** A rough 95 % upper bound is the **"rule of three"**: $p\lesssim3/N$ — e.g. 0 errors in $10^{4}$ bits ⇒ BER $\lesssim3\times10^{-4}$. `[FOUNDATION ADDED — standard result, not stated in the lessons]`

**Latency and percentiles (Lesson 36).** A mean can hide the thing that matters. *Verified simulation:* 1,000,000 calls that take 100 µs, except 0.1 % that take 5,000 µs — mean **104.7 µs** (exact 104.9), median **100**, 99th percentile **100**, but 99.99th percentile **5,000 µs**. If your real-time deadline is 200 µs, the *mean* looks fine while a **tail event** breaks you — report **percentiles and miss rate**, not just the mean.

---

## 5.11 Python workout (verified)

```python
import numpy as np
from scipy.special import erfc

def Q(x):                       # tail of the standard normal
    return 0.5 * erfc(np.asarray(x) / np.sqrt(2))

print(Q([0, 1, 2, 3]))          # [0.5  0.1587 0.02275 0.00135]

rng = np.random.default_rng(0)  # always seed for repeatable results

# --- BPSK Monte-Carlo
def ber_bpsk(ebn0_db, n_bits):
    e = 10**(ebn0_db/10)
    bits = rng.integers(0, 2, n_bits)
    s = 2*bits - 1.0                          # 0->-1, 1->+1  (Eb = 1)
    sigma = np.sqrt(1/(2*e))                  # NOTE the /2  (N0/2 per real dim)
    r = s + sigma*rng.standard_normal(n_bits)
    return np.mean((r > 0) != (bits == 1))

print(ber_bpsk(4, 2_000_000), Q(np.sqrt(2*10**0.4)))   # ~0.0125  0.0125  (simulated value varies slightly with the RNG state)

# --- complex noise with N0 = 1
n = 10**6
w = np.sqrt(1/2) * (rng.standard_normal(n) + 1j*rng.standard_normal(n))
print(np.mean(abs(w)**2))                              # ~1.0

# --- Rayleigh from 20 random unit phasors (Lesson 10 recipe)
h = np.exp(1j*rng.uniform(0, 2*np.pi, (100000, 20))).sum(axis=1)
P = abs(h)**2 / np.mean(abs(h)**2)
print(np.mean(P < 0.1))                                # ~0.09  (theory 0.0952)

# --- confidence interval and z-score
p0, N, ph = 0.01, 100_000, 0.0105
se = np.sqrt(p0*(1-p0)/N); print(se, (ph-p0)/se)       # 3.15e-4  1.59
```

---

## 5.12 Common mistakes ⚠️
1. **$\sigma^{2}=N_0$ instead of $N_0/2$** in simulations (3 dB error, 4.5× BER at 4 dB).
2. **Mixing $\sigma$ and $\sigma^{2}$** — noise *amplitude* scales with σ, noise *power* with σ².
3. **Treating a PDF value as a probability.** Probabilities are areas.
4. **Believing "0 errors" means "BER = 0"** (use ≥ 100 errors, or quote a bound).
5. **Averaging dB** (P01) and **averaging BERs across very different SNRs** without care.
6. **Forgetting independence assumptions** — burst (fading) errors are *not* independent.
7. **Using the normal approximation of the binomial with very few errors.**
8. **Not seeding the RNG** — irreproducible plots and "flaky" tests.
9. **Reporting only the mean** of a heavy-tailed measurement (latency).
10. **`erfc` scaling:** $Q(x)=\tfrac12\operatorname{erfc}(x/\sqrt2)$, not $\operatorname{erfc}(x)$.

---

## 5.13 Practice problems (answers at the end)

1. Three fair coins are tossed. Find P(exactly two heads).
2. A bit error occurs with probability 0.02, independently. What is P(no error in 5 bits)?
3. $X\sim N(0,4)$ and $Y\sim N(0,9)$ are independent. What is the distribution of $X+Y$? Of $2X$?
4. What fraction of a Gaussian lies within ±2σ of the mean?
5. Use the table to estimate $Q(2.5)$ and $Q(3.5)$ qualitatively (is it nearer $10^{-2}$, $10^{-3}$, $10^{-4}$?). Then state BPSK BER at $E_b/N_0$ with $\sqrt{2E_b/N_0}=3$.
6. Convert $E_b/N_0=6$ dB to linear and compute $\sqrt{2E_b/N_0}$. Estimate BER with the table.
7. A complex noise sample has $N_0=2$. What is the variance of its real part and its total power?
8. A simulation of $10^{6}$ bits shows 40 errors. Give $\hat p$, SE, and the relative uncertainty. Is this enough errors?
9. Baseline BER 0.02 over $N=10^{5}$ bits; a new build shows 0.0206. Compute $z$. Significant at 95 %?
10. Zero errors in $3\times10^{5}$ bits. Give a rough 95 % upper bound on BER.
11. *Think:* explain why the mean of a latency distribution can look fine while the system misses deadlines.
12. *Think:* two diversity branches each have a deep-fade probability of 0.0952. What is the probability both fade (independent)? What assumption is hidden?

## 5.14 Active recall
1. Define PDF, CDF and PMF in one line each. 2. State variance rules for scaling and for sums of independent RVs. 3. Why is thermal noise Gaussian? 4. Define $Q(x)$ and relate it to erfc. 5. Derive $P_b=Q(\sqrt{2E_b/N_0})$ in four lines. 6. Why is each real noise dimension $N_0/2$? 7. How does Rayleigh fading arise from the CLT? 8. How many errors do you need before trusting a BER estimate, and why? 9. Write SE and z-score for a proportion. 10. Why prefer percentiles to the mean for latency?

## 5.15 Self-check — ready for P06 if you can…
- [ ] state mean/variance/independence rules and use them for noise;
- [ ] compute Gaussian tail probabilities with `Q(x)` and sanity-check against the table;
- [ ] simulate BPSK correctly with variance $N_0/2$ and match theory;
- [ ] explain why Rayleigh fading follows from many random phasors;
- [ ] attach an honest error bar to a measured BER and run the z-test.

---

## ✅ Answer key

1. Outcomes with exactly 2 heads: 3 of 8 ⇒ **0.375**. 2. $0.98^{5}=\mathbf{0.904}$. 3. $X+Y\sim N(0,\,4+9=13)$; $2X\sim N(0,\,4\cdot4=16)$. 4. **95.45 %**. 5. $Q(2.5)\approx6.2\times10^{-3}$ (between $Q(2)=0.0228$ and $Q(3)=0.00135$) — nearer $10^{-2}$/$10^{-3}$; $Q(3.5)\approx2.3\times10^{-4}$ — nearer $10^{-4}$; $\sqrt{2E_b/N_0}=3\Rightarrow P_b=Q(3)=\mathbf{1.35\times10^{-3}}$. 6. 6 dB ⇒ 3.981; $\sqrt{7.96}=2.82$ ⇒ $Q(2.82)\approx\mathbf{2.4\times10^{-3}}$ (the simulated 2.36×10⁻³ above). 7. Real part variance $N_0/2=\mathbf1$; total power $N_0=\mathbf2$. 8. $\hat p=4\times10^{-5}$; SE $=\sqrt{4\times10^{-5}(1-4\times10^{-5})/10^{6}}=6.3\times10^{-6}$ (**≈ 16 %**, = $1/\sqrt{40}$); **not enough** — aim for ≥ 100 errors. 9. $\mathrm{SE}=\sqrt{0.02\cdot0.98/10^{5}}=4.43\times10^{-4}$; $z=0.0006/4.43\times10^{-4}=\mathbf{1.35}<1.96$ ⇒ **not significant**. 10. $3/(3\times10^{5})=\mathbf{10^{-5}}$. 11. Rare long events (tail) barely move the mean but violate hard deadlines; percentiles/miss-rate expose them (§5.10). 12. $0.0952^{2}=\mathbf{0.00906}$ (0.91 %); it assumes the two branches fade **independently** — real branches that are correlated (antennas too close) give a much larger joint probability.

---

*Next: **P06 — Signals primer and Fourier series** (what a signal is, periodic signals, energy and power, Fourier series, spectrum and bandwidth) — the gateway to Lesson 2–5.*
