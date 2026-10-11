# P03 — Algebra, Series and Calculus Essentials (Σ, Geometric Series, Derivatives, Integrals, Taylor, Orthogonality)

*Prerequisite module 3 · ⏱ about 5–6 hours · assumes P01–P02 · every number below was computed and checked.*

> **Where this unlocks the curriculum.** Lesson 1 derives Euler's formula from the **Taylor series**. Lesson 3 (DFT) rests on **summation notation, the geometric series and orthogonality**. Lessons 4 and 6 are built from **sums** (convolution, correlation). Lesson 16 (FM demodulation) uses the fact that **frequency is the derivative of phase**. You do *not* need to be a calculus expert — you need to read these four ideas fluently: **Σ, geometric series, derivative-as-rate, integral-as-accumulation**.

---

## 3.1 Functions and the shape of growth

A **function** $f(x)$ turns an input into an output. In signal processing the input is usually **time** $t$ (continuous) or **sample index** $n$ (discrete).

Three shapes appear constantly:

| Shape | Formula | Where you will meet it |
|---|---|---|
| Linear | $y=ax+b$ | phase growing steadily with time ($\varphi=2\pi ft$) |
| Oscillating | $y=A\cos(\omega t+\varphi)$ | the carrier (P02) |
| Exponential | $y=e^{ax}$ | decay of a filter response, $e^{j\theta}$ in Lesson 1 |

**Exponential facts.** $e\approx2.71828$ is the base for which $\dfrac{d}{dx}e^{x}=e^{x}$. $e^{a+b}=e^{a}e^{b}$. $\ln$ is its inverse. A decaying exponential $e^{-t/\tau}$ falls to $e^{-1}\approx37\%$ after one *time constant* $\tau$, and to $e^{-3}\approx\mathbf{5\%}$ after $3\tau$ (verified: 0.0498).

---

## 3.2 Summation notation — a for-loop in one symbol 🔥

$$\sum_{n=0}^{N-1}x[n]\;=\;x[0]+x[1]+\cdots+x[N-1]$$

Read it as: *"for $n$ from 0 to $N-1$, add up $x[n]$."* In code it is literally

```python
total = 0
for n in range(N):      # n = 0 ... N-1
    total += x[n]
```
(or `x.sum()` in NumPy). Almost every DSP formula you will see — **DFT, convolution, correlation, energy, FIR filtering** — is a $\sum$.

**Properties**
* **Linearity:** $\sum(a\,x[n]+b\,y[n])=a\sum x[n]+b\sum y[n]$ (constants come out, sums split).
* **Index shift:** $\sum_{n=1}^{N}x[n-1]=\sum_{n=0}^{N-1}x[n]$ (renaming the loop variable).
* **Double sums** are nested loops.

**Two sums to know.**
* Sum of the first $N$ integers: $\displaystyle\sum_{n=1}^{N}n=\frac{N(N+1)}2$ — e.g. $N=100$ gives **5050** (verified).
* Sum of squares for small cases: $1+4+9+16=\mathbf{30}$ (verified).

**Energy of a discrete signal** is $E=\sum_n|x[n]|^2$ — the sum-of-squares you will compute from IQ samples (Lesson 1 shows $|z|^2$ for complex samples).

---

## 3.3 The geometric series 🔥 (the key to the DFT)

A sequence where each term is the previous one times a constant ratio $r$: $1,\ r,\ r^2,\ r^3,\ \dots$

$$\boxed{\;\sum_{n=0}^{N-1}r^{n}=\frac{1-r^{N}}{1-r}\quad(r\ne1)\;}\qquad\text{and for }|r|<1:\ \sum_{n=0}^{\infty}r^{n}=\frac1{1-r}$$

**Why it's true (two lines).** Call the sum $S=1+r+\cdots+r^{N-1}$. Then $rS=r+r^{2}+\cdots+r^{N}$. Subtract: $S-rS=1-r^{N}$, so $S=(1-r^{N})/(1-r)$.

*Verified:* $r=0.5,\ N=10$: sum = **1.99805** = formula; infinite sum → **2**. $\sum_{n=0}^{7}(1/2)^n=\mathbf{1.99219}$.

### The payoff: sums of spinning arrows vanish (orthogonality of the DFT basis)
Take $r=e^{j\theta}$ (a point on the unit circle — Lesson 1 explains why; for now just treat $e^{j\theta}$ as "an arrow of length 1 at angle $\theta$"). Choose $\theta=2\pi k/N$ so that after $N$ steps the arrow makes exactly $k$ full turns, i.e. $r^{N}=1$. If $k\not\equiv0\pmod N$ then $r\ne1$ and the formula gives

$$\sum_{n=0}^{N-1}e^{j2\pi kn/N}=\frac{1-1}{1-e^{j2\pi k/N}}=0$$

while for $k=0$ every term is 1 and the sum is $N$. **Verified for $N=8$:** magnitudes for $k=0,1,\dots,7$ are **[8, 0, 0, 0, 0, 0, 0, 0]**. And $N=4$: $1+j-1-j=\mathbf0$.

*Meaning:* equally spaced arrows around a circle cancel to zero. This is *exactly* why the DFT can pick out one frequency and ignore the others (Lesson 3), and why OFDM subcarriers do not interfere (Lesson 12). You will see this identity again and again.

---

## 3.4 Limits (just the intuition) and the definition of $e$

A **limit** asks "what does this approach as $n$ grows (or a step shrinks) without ever necessarily reaching it?" You never need to prove one here; you need to recognise one.

* $e=\lim_{n\to\infty}(1+1/n)^{n}$. *Verified:* $n=1\to2.0$, $n=10\to2.594$, $n=1000\to2.717$, $n=10^{6}\to2.71828$.
* The idea that matters: **a derivative and an integral are limits of ordinary arithmetic on smaller and smaller steps.** That is also why a computer can approximate them with differences and sums (see §3.5–3.6).

---

## 3.5 Derivatives — "how fast is it changing?" 🔥

The **derivative** $\dfrac{dy}{dt}$ is the instantaneous rate of change = the **slope** of the graph at a point.

**Rules you actually need**

| Function | Derivative |
|---|---|
| constant $c$ | 0 |
| $t^{n}$ | $n\,t^{n-1}$ |
| $\sin(\omega t)$ | $\omega\cos(\omega t)$ |
| $\cos(\omega t)$ | $-\omega\sin(\omega t)$ |
| $e^{at}$ | $a\,e^{at}$ |
| $u(t)v(t)$ (product rule) | $u'v+uv'$ |
| $g(h(t))$ (chain rule) | $g'(h(t))\,h'(t)$ |

*Check by simulation:* the slope of $\sin(2\pi\cdot5t)$ at $t=0$ should be $2\pi\cdot5=31.416$; a numerical gradient gave **31.41593** ✓.

### Frequency is the derivative of phase 🔥 (used in Lesson 16 FM demodulation)
A tone $\cos(\varphi(t))$ has **instantaneous frequency**

$$\boxed{\;f_{\text{inst}}(t)=\frac1{2\pi}\frac{d\varphi}{dt}\;}$$

* If $\varphi(t)=2\pi f t$ (steady tone), then $\dfrac{d\varphi}{dt}=2\pi f$ ⇒ $f_{\text{inst}}=f$ — the familiar case.
* If the phase *accelerates*, the frequency changes: a **chirp** $\varphi(t)=2\pi(100t+50t^{2})$ has $f_{\text{inst}}=100+100t$. *Verified numerically:* at $t=0.5$ s the measured value was **150.0000 Hz** ✓.
* **FM** works precisely this way: the message changes the frequency, so recovering it means measuring $d\varphi/dt$ — i.e. differencing the phase of IQ samples. Lesson 16 does this with a neat trick, $\angle\bigl(x[n]\,x^{*}[n-1]\bigr)$, which gives the phase *difference* directly and avoids phase unwrapping.

**Discrete version:** the derivative becomes a **difference**, $\Delta\varphi[n]=\varphi[n]-\varphi[n-1]$, and $f_{\text{inst}}\approx\Delta\varphi\cdot f_s/2\pi$. In NumPy: `np.diff(phase)` or `np.gradient`.

---

## 3.6 Integrals — "how much has accumulated?" 🔥

The **definite integral** $\displaystyle\int_a^b x(t)\,dt$ is the **area under the curve** between $a$ and $b$ — equivalently, the continuous-time version of $\sum$. Integrals and sums are the same idea: *continuous* signals use $\int dt$, *sampled* signals use $\sum$.

**Rules you need**

| Integral | Result |
|---|---|
| $\int\cos(\omega t)\,dt$ | $\dfrac1\omega\sin(\omega t)$ |
| $\int\sin(\omega t)\,dt$ | $-\dfrac1\omega\cos(\omega t)$ |
| $\int e^{at}\,dt$ | $\dfrac1a e^{at}$ |
| $\int_a^bc\,dt$ | $c(b-a)$ |

*Verified:* $\int_0^{\pi}\sin t\,dt=\mathbf2$ (numerical trapezoid: 1.99999999984; SciPy `quad`: 2.0).

**Two quantities you will use constantly**
* **Energy** of a signal: $E=\int|x(t)|^2dt$ (discrete: $\sum|x[n]|^2$). *Verified:* $x=3\cos(2\pi\cdot5t)$ over 1 s gives **4.5 = $A^{2}T/2$** ✓ (this is the $A^{2}/2$ average power from P02).
* **Average value:** $\dfrac1T\int_0^Tx(t)\,dt$ — a pure sinusoid over whole cycles averages to **0**.

---

## 3.7 Orthogonality of sinusoids 🔥 (the idea behind Fourier series, OFDM and correlation)

Two signals are **orthogonal** over an interval if the integral of their *product* is zero — they have "nothing in common", like perpendicular vectors with zero dot product (P04 will make this literal).

Over one period $T=1$ s with integer frequencies $m,n\ge1$:

$$\int_0^{1}\cos(2\pi mt)\cos(2\pi nt)\,dt=\begin{cases}\tfrac12&m=n\\[2pt]0&m\ne n\end{cases},\qquad\int_0^{1}\sin(2\pi mt)\cos(2\pi nt)\,dt=0$$

**Verified numerically:** $(m,n)=(1,1)\to\mathbf{0.5}$; $(1,2)\to\mathbf0$; $(2,3)\to\mathbf0$; $(3,3)\to\mathbf{0.5}$; $\int\sin\cos=\mathbf0$; $\int\sin(2\pi t)\sin(4\pi t)=\mathbf0$.

**Why (one line, via P02's product identity):** $\cos a\cos b=\tfrac12[\cos(a-b)+\cos(a+b)]$. For $m\ne n$ both terms are whole-cycle cosines, which integrate to zero; for $m=n$ the first term is the constant $\tfrac12$.

*Meaning:* you can **extract one frequency's strength** from a mixture simply by multiplying by that frequency's sinusoid and integrating (or summing). That single idea *is* the Fourier series (P06), the DFT (Lesson 3), matched filtering (Lesson 6) and OFDM (Lesson 12).

---

## 3.8 Taylor series — building functions out of powers 🔥 (used to derive Euler's formula in Lesson 1)

Many smooth functions can be written as an infinite polynomial:

$$e^{x}=\sum_{k=0}^{\infty}\frac{x^{k}}{k!}=1+x+\frac{x^{2}}2+\frac{x^{3}}6+\cdots$$
$$\sin x=x-\frac{x^{3}}{3!}+\frac{x^{5}}{5!}-\cdots,\qquad\cos x=1-\frac{x^{2}}{2!}+\frac{x^{4}}{4!}-\cdots$$

($k!$ is the factorial: $4!=24$.) Adding more terms improves the approximation:

| Terms of $e^{1}$ | Partial sum | True $e=2.718282$ |
|---|---|---|
| 2 | 2.0 | |
| 3 | 2.5 | |
| 5 | 2.7083 | |
| 10 | **2.7182815** | ✓ |

*Verified:* $\sin 1\approx1-\tfrac16+\tfrac1{120}-\tfrac1{5040}=0.841468$ vs true **0.841471**; $\cos1\approx0.540278$ vs **0.540302**; $e^{0.1}\approx1+0.1+0.005=1.105$ vs **1.10517**.

**Why you care.** Lesson 1 substitutes $x=j\theta$ into the $e^x$ series (where $j^2=-1$, which Lesson 1 teaches) and **splits it into the series for cosine and for sine**, producing Euler's formula $e^{j\theta}=\cos\theta+j\sin\theta$. Having seen all three series here, that derivation will read as a recognition, not a leap. Taylor series also justify **small-angle approximations** such as $\sin x\approx x$ and $\cos x\approx1-x^{2}/2$ (check against the table above), which engineers use to simplify small-error analysis.

---

## 3.9 Doing it in Python (verified code)

```python
import numpy as np, math

# --- sums
print(sum(range(1, 101)))                          # 5050
r, N = 0.5, 10
print(sum(r**n for n in range(N)), (1-r**N)/(1-r)) # 1.998046875 both

# --- geometric series of spinning arrows -> DFT orthogonality
n = np.arange(8)
mag = [abs(np.exp(2j*np.pi*k*n/8).sum()) for k in range(8)]
print(np.round(mag, 12))                           # [8. 0. 0. 0. 0. 0. 0. 0.]

# --- derivative as a rate: instantaneous frequency of a chirp
t = np.linspace(0, 1, 100001)
phi = 2*np.pi*(100*t + 50*t**2)
f_inst = np.gradient(phi, t) / (2*np.pi)
print(f_inst[np.argmin(abs(t-0.5))])               # ~150.0

# --- integral as accumulation (NumPy >= 2.0 uses trapezoid, not trapz)
tt = np.linspace(0, 1, 100001)
print(np.trapezoid(np.cos(2*np.pi*tt)**2, tt))     # 0.5  (energy-like)
print(np.trapezoid(np.cos(2*np.pi*tt)*np.cos(4*np.pi*tt), tt))  # ~0 (orthogonal)

# --- Taylor partial sums
print(sum(1.0**k/math.factorial(k) for k in range(10)))  # 2.7182815
```

> **Version note (verified):** in the NumPy 2.x installed here, `np.trapz` no longer exists — use `np.trapezoid` (or `scipy.integrate.trapezoid`). If you copy older tutorials, replace `trapz` accordingly.

---

## 3.10 Common mistakes ⚠️
1. **Off-by-one in sums:** $\sum_{n=0}^{N-1}$ has $N$ terms, not $N-1$. Python's `range(N)` is $0\ldots N-1$.
2. **Forgetting the $1/(2\pi)$** when converting $d\varphi/dt$ (rad/s) to Hz.
3. **Differentiating a wrapped phase.** `np.angle` returns values in $(-\pi,\pi]$; differencing it across a jump gives huge fake spikes — either unwrap first (`np.unwrap`) or use the conjugate-multiply form $\angle(x[n]x^{*}[n-1])$ that Lesson 16 uses.
4. **Using the geometric-series formula with $r=1$** (division by zero; the sum is just $N$).
5. **Orthogonality only holds over whole periods.** Integrating over a non-integer number of cycles leaves leftovers — this is the root of *spectral leakage* in Lesson 3.
6. **Using degrees** inside derivatives of sin/cos — the rule $\tfrac d{dt}\sin\omega t=\omega\cos\omega t$ assumes radians.
7. **`np.trapz` on NumPy 2.x** — use `np.trapezoid`.

---

## 3.11 Practice problems (answers at the end)

1. Evaluate $\sum_{k=1}^{4}k^{2}$ and $\sum_{n=0}^{7}(1/2)^{n}$.
2. Use the geometric-series formula to sum $1+3+9+27+81$.
3. Find $\dfrac{d}{dt}\bigl[3\sin(2\pi\cdot10t)\bigr]$ and state its amplitude.
4. A phase is $\varphi(t)=2\pi(1000t+200t^{2})$. Find the instantaneous frequency at $t=0.1$ s.
5. Compute $\displaystyle\int_0^1\cos^{2}(2\pi t)\,dt$.
6. A filter output decays as $e^{-t/\tau}$ with $\tau=1$ ms. What fraction of the initial value remains at 3 ms?
7. Approximate $e^{0.1}$ with the first three Taylor terms and compare with the true value.
8. Evaluate $\sum_{n=0}^{3}e^{j2\pi n/4}$ without a calculator.
9. Differentiate $x\,e^{x}$.
10. Explain, using a product identity, why $\int_0^{T}\sin\!\bigl(\tfrac{2\pi t}{T}\bigr)\cos\!\bigl(\tfrac{2\pi t}{T}\bigr)dt=0$.
11. *Think:* you sample a tone for exactly 3.5 cycles and multiply by the same-frequency cosine and sum. Will the result be zero for a *different* frequency? Why does this matter for the DFT?

## 3.12 Active recall
1. Translate $\sum_{n=0}^{N-1}x[n]$ into a for-loop. 2. State the geometric-series sum and give its two-line proof. 3. Why do $N$ equally spaced unit arrows sum to zero? 4. State "frequency is the derivative of phase" with the $2\pi$. 5. What does the integral of a product of two sinusoids tell you? 6. List the Taylor series of $e^{x}$, $\sin x$, $\cos x$. 7. Why must you unwrap phase before differencing (or what trick avoids it)? 8. What replaces the integral for sampled signals?

## 3.13 Self-check — ready for P04 if you can…
- [ ] read any $\sum$ as a loop and write it in NumPy;
- [ ] derive the geometric-series formula and use it for $e^{j\theta}$;
- [ ] differentiate sin/cos/exponentials and state frequency = $\frac1{2\pi}\,d\varphi/dt$;
- [ ] explain orthogonality of different-frequency sinusoids over whole periods;
- [ ] write the three Taylor series from memory.

---

## ✅ Answer key

1. $1+4+9+16=\mathbf{30}$; $\sum_{n=0}^{7}2^{-n}=(1-2^{-8})/(1-\tfrac12)=\mathbf{1.99219}$. 2. $r=3,\ N=5$: $(1-3^{5})/(1-3)=(1-243)/(-2)=\mathbf{121}$ (check: 1+3+9+27+81 = 121). 3. $60\pi\cos(2\pi\cdot10t)$; amplitude **188.5**. 4. $\tfrac1{2\pi}\tfrac{d\varphi}{dt}=1000+400t$ ⇒ at 0.1 s: **1040 Hz**. 5. **0.5**. 6. $e^{-3}=\mathbf{0.0498}$ (≈ 5 %). 7. $1+0.1+0.005=\mathbf{1.105}$ vs true **1.10517** (error ≈ 0.0002). 8. $1+j+(-1)+(-j)=\mathbf0$. 9. $e^{x}+xe^{x}=(1+x)e^{x}$. 10. $\sin u\cos u=\tfrac12\sin2u$; over a whole number of cycles of $\sin 2u$ (here two cycles) the integral is **0**. 11. Not guaranteed — over a *non-integer* number of cycles the cancellation is incomplete, so a "wrong" frequency leaks a non-zero result. That leftover is exactly **spectral leakage** in the DFT (Lesson 3).

---

*Next: **P04 — Linear algebra** (vectors, dot products, projections, matrices, eigenvalues, least squares, SVD intro) — the language of MIMO and channel estimation.*
