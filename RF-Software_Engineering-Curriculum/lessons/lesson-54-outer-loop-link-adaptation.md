[← Table of Contents](../README.md) · [← Lesson 53](lesson-53-carrier-aggregation.md)

---

# Lesson 54 — Outer-Loop Link Adaptation: Hitting a BLER Target Without Knowing the Bias

*Lessons 8, 20, and 29 built the inner loop of link adaptation: measure the channel, report CQI, pick an MCS. In practice the reported CQI is systematically off. The outer loop fixes this with two numbers, and the equilibrium they produce can be derived in three lines.*

### 1. Intuition

The CQI table is defined around a target: roughly, the highest MCS whose first-transmission block error rate would stay near 10% at the reported channel quality. But real CQI is biased. UE implementations differ in how they estimate SINR, feedback arrives late (the channel has moved on, Lesson 10), and interference fluctuates. If reports are optimistic, the scheduler picks MCSs that fail far more than 10% of the time and burns capacity on retransmissions (Lesson 20). If reports are pessimistic, it leaves capacity on the table. The scheduler cannot see the bias directly, but it sees every ACK and NACK. The **outer loop** uses those to learn the bias.

### 2. The engineering problem that forces this

Drive the long-run first-transmission BLER to a chosen target $p^*$ using only ACK/NACK feedback, without knowing the size or sign of the CQI bias, while also tracking a bias that drifts over time.

### 3. Mathematical foundation

Keep an offset $\delta$ (in dB) added to the reported SINR before MCS selection. After each first transmission: on ACK, $\delta\leftarrow\delta+\Delta_{up}$ (be more aggressive); on NACK, $\delta\leftarrow\delta-\Delta_{down}$ (back off).

### 4. Formal definition

The **aggressiveness** of the MCS choice is $a=(\text{reported SINR}+\delta)-\text{true SINR}$. A positive $a$ means the scheduler is assuming a better channel than exists. The block error probability is an increasing function $p(a)$ of aggressiveness, with $p(0)\approx p^*$ by the CQI definition.

### 5. Derivation — the equilibrium ratio, and the speed-versus-jitter trade-off

**(a) Equilibrium.** At steady state the offset stops drifting on average. The expected change per update is
$$E[\Delta\delta]=(1-p)\,\Delta_{up}-p\,\Delta_{down}=0\;\Longrightarrow\;p=\frac{\Delta_{up}}{\Delta_{up}+\Delta_{down}}.$$
The loop converges to *whatever BLER the step ratio encodes*, independent of the channel or its bias. To hit $p^*$:
$$\frac{\Delta_{up}}{\Delta_{down}}=\frac{p^*}{1-p^*}\quad\Rightarrow\quad p^*=0.1:\;\Delta_{down}=9\,\Delta_{up}.$$

**(b) Step size: tracking speed against jitter.** Near equilibrium write $p(\delta)\approx p^*+s(\delta-\delta^*)$, where $s=dp/d\delta>0$ is the BLER slope. The mean drift becomes $-(\Delta_{up}+\Delta_{down})\,s\,(\delta-\delta^*)$, a restoring force with rate $\lambda=(\Delta_{up}+\Delta_{down})s$ per update, so a bias is corrected with time constant $1/\lambda$ updates. Each update also injects noise with variance $p\Delta_{down}^2+(1-p)\Delta_{up}^2$; for $p=0.1$, $\Delta_{down}=9\Delta_{up}$ this is $0.1\cdot81\Delta_{up}^2+0.9\Delta_{up}^2=9\Delta_{up}^2$. For this first-order mean-reverting process (an AR(1) recursion) the stationary variance is approximately noise variance $/\,2\lambda$:
$$\text{Var}(\delta)\approx\frac{9\Delta_{up}^2}{2\cdot10\Delta_{up}s}=\frac{0.45\,\Delta_{up}}{s}.$$
So jitter grows as $\sqrt{\Delta_{up}}$ while convergence time shrinks as $1/\Delta_{up}$: halving the steps cuts jitter by $\sqrt2$ but doubles the time to correct a bias. Neither extreme is free.

### 6. Visualization

```
reported SINR ──►(+ δ)──► MCS table ──► transmit ──► ACK / NACK
                  ▲                                     │
                  └──── δ += Δup (ACK)  /  δ -= Δdown (NACK) ◄──┘

BLER target 10%:  Δdown = 9 × Δup   (rare NACKs get large corrections)
```

### 7. Worked example

Target $p^*=10\%$ with $\Delta_{up}=0.1$ dB and $\Delta_{down}=0.9$ dB. Suppose the UE over-reports by 2 dB. Initially $a=+2$ dB, BLER is high, NACKs drive $\delta$ down in 0.9 dB jumps until $\delta\approx-2$ dB, where $a\approx0$ and BLER settles near 10%. As an illustration, if the BLER slope near the operating point is about $s\approx0.15$ per dB, the formula in (b) gives a jitter of roughly $\sqrt{0.45\cdot0.1/0.15}\approx0.55$ dB and a time constant near $1/(10\cdot0.1\cdot0.15)\approx6.7$ updates. (The slope $s$ here is an assumed illustrative value, not a measured one.)

### 8. Code

```python
import numpy as np

def p_nack(a, k=1.7):
    """BLER vs aggressiveness a (dB), built so p(0) = 0.10."""
    return 1.0 / (1.0 + 9.0 * np.exp(-k * a))

def run(bias_db, d_up, d_down, n=60000, seed=0):
    rng = np.random.default_rng(seed)
    delta, nacks, deltas = 0.0, [], []
    for _ in range(n):
        a = bias_db + delta                     # UE over-reports by bias_db
        nack = rng.random() < p_nack(a)
        delta += -d_down if nack else d_up
        nacks.append(nack); deltas.append(delta)
    h = n // 2                                   # discard the transient
    return np.mean(nacks[h:]), np.mean(deltas[h:]), np.std(deltas[h:])

for d_up, d_down in [(0.1, 0.9), (0.05, 0.45), (0.5, 0.5)]:
    bler, mean_d, std_d = run(2.0, d_up, d_down)
    theory = d_up / (d_up + d_down)
    print(f"up={d_up:4.2f} down={d_down:4.2f}  BLER={bler:.3f} (theory {theory:.3f})  "
          f"mean delta={mean_d:+.2f} dB  std={std_d:.2f} dB")
```

### 9. Experiment

Run §8. Confirm the long-run BLER matches $\Delta_{up}/(\Delta_{up}+\Delta_{down})$ in all three cases, including the deliberately wrong $0.5/0.5$ ratio. Compare the jitter between the first two rows, which share the same 1:9 ratio but differ in step size.

### 10. Expected output

Approximately: rows one and two converge to BLER $\approx0.10$ with mean $\delta\approx-2.0$ dB; row two (smaller steps) shows roughly $\sqrt2$ times less jitter (about 0.4 dB versus 0.55 dB). Row three converges to BLER $\approx0.50$ with mean $\delta\approx-0.7$ dB — the loop faithfully hits the wrong target because the ratio encodes 50%. The logistic nonlinearity makes these approximate rather than exact.

### 11. Debugging exercise

Symptom, no cause given: after enabling the outer loop, long-run first-transmission BLER settles at about 25%, not the intended 10%. The loop is stable and the offset moves in response to feedback.

What are the two most plausible causes, and what one cheap check separates them?

**Root cause (revealed):** two candidates. **(1) A step ratio that doesn't encode the target.** By §5(a), the loop converges to $\Delta_{up}/(\Delta_{up}+\Delta_{down})$; a ratio of $1{:}3$ gives exactly 25%, regardless of channel. **(2) Saturation:** if $\delta$ is clamped to a configured minimum or maximum, the loop cannot reach equilibrium and BLER reflects the bound, not the ratio. The cheap check is to log $\delta$: pinned at a limit means saturation (fix the bound or the underlying bias source); wandering freely while BLER sits at 25% means the ratio is wrong. A trace showing both a free-floating $\delta$ and BLER matching $\Delta_{up}/(\Delta_{up}+\Delta_{down})$ is the loop working exactly as written.

### 12. Common mistakes

- Choosing the two step sizes independently, without checking that their ratio encodes the target BLER.
- Updating on the final HARQ outcome rather than the first transmission. The 10% target is conventionally an initial-transmission BLER; the residual error after HARQ combining (Lesson 20) is a different, much smaller number.
- Treating the step size as free. §5(b) shows it is a genuine trade between how fast a drifting bias is tracked and how much the MCS choice jitters in steady state.

### 13. Real RF/cellular application

The standard defines the CQI and MCS tables, not the outer loop; OLLA is an implementation algorithm that each vendor's scheduler builds, which makes it direct territory for RAN software engineers. Its behavior shows up in field traces as a slowly varying MCS offset, and mismatches between BLER target and observed BLER are a routine first check when diagnosing throughput complaints (the methodology of Lesson 38).

### 14. Interview question

*"How do you choose the up and down step sizes in an outer-loop link adaptation scheme?"* Answer from §5: the ratio is fixed by the target, $\Delta_{up}/\Delta_{down}=p^*/(1-p^*)$ (so 1:9 for a 10% target), because the equilibrium BLER is exactly $\Delta_{up}/(\Delta_{up}+\Delta_{down})$. The absolute size then trades tracking speed ($\propto 1/\Delta$) against steady-state jitter ($\propto\sqrt\Delta$).

### 15. Active recall

1. Derive the equilibrium BLER from the zero-drift condition.
2. Why does halving both step sizes reduce jitter but slow convergence?
3. Why must the update use first-transmission outcomes?

### 16. Mini assignment

Extend §8 to make the bias drift slowly (for example a sinusoid with a period of several thousand updates) and sweep $\Delta_{up}$ with the 1:9 ratio held fixed. Plot the BLER tracking error against $\Delta_{up}$ and find the step size that minimizes it — the point where jitter and lag balance.

### 17. Exit test

1. State the step-size ratio for a 5% BLER target and show the derivation.
2. True or false, with a one-line reason: "A well-tuned outer loop makes the reported CQI bias irrelevant to long-run BLER."
3. Your two-hypothesis diagnosis from §11, if you haven't worked through it yet.

---

**Next:** Lesson 55 returns to random access with **PRACH contention**: how many preambles a cell needs so that simultaneous attempts collide rarely, derived from a simple counting argument that builds directly on Lesson 30.

---

← [Lesson 53](lesson-53-carrier-aggregation.md) · [Table of Contents](../README.md) · [Lesson 55 →](lesson-55-prach-contention.md)