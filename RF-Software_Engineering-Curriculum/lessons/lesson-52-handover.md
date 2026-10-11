[← Table of Contents](../README.md) · [← Lesson 51](lesson-51-drx.md)

---

# Lesson 52 — Handover: Turning a Coin Flip into a Reliable Decision

*Lessons 21 and 33 both mentioned handover in passing — PDCP surviving it, RRC_INACTIVE sometimes failing across it — without ever deriving the decision itself. This lesson derives when a UE should switch cells, and the Q-function from Lesson 7 returns to quantify exactly why hysteresis and time-to-trigger work.*

### 1. Intuition

A moving UE must eventually switch serving cells without dropping its connection. The naive rule — "switch when the neighbor's signal exceeds the serving cell's" — fails badly at the cell boundary, where the two signals are nearly equal and fast fading (Lesson 10) makes each measurement fluctuate randomly around zero difference. A decision made on raw measurements there is close to a coin flip, repeated every sample — the UE would bounce between cells constantly (**ping-pong**), burning signaling and risking dropped connections. Real systems fix this with three mechanisms layered together: measurement filtering, a **hysteresis** margin, and a **time-to-trigger (TTT)** requirement — and the combined effect is quantifiable, not folklore.

### 2. The engineering problem that forces this

Design a trigger rule that (a) almost never fires from noise alone when the UE sits near a boundary, yet (b) still fires promptly when the UE has genuinely crossed into the neighbor's territory, since waiting too long risks the serving link failing before handover completes (**too-late handover**).

### 3. Mathematical foundation

**Layer-3 filtering** smooths each measurement $M_n$: $F_n=(1-a)F_{n-1}+aM_n$, with $a=(1/2)^{k/4}$ set by a configured coefficient $k$. **Event A3** (the neighbor-better trigger): neighbor measurement $-$ hysteresis $>$ serving measurement $+$ offset, held continuously for the TTT duration.

### 4. Formal definition

**Hysteresis** $H$: a margin the neighbor must beat the serving cell by, not merely match. **TTT**: the condition must persist for a configured time (equivalently, $N$ consecutive measurement samples). **Ping-pong**: handover to a neighbor followed shortly by handover back. **Too-late handover**: the serving link degrades past usability before handover completes.

### 5. Derivation — quantifying how the three mechanisms suppress false triggers

Model the measured serving-vs-neighbor difference for a UE sitting *exactly* at the boundary (true difference zero) as Gaussian noise $d_n\sim N(0,\sigma^2)$, standing in for fast fading and shadowing.

**No protection.** The trigger condition is $d_n>0$: probability $0.5$ *per sample* — a literal coin flip every measurement.

**Hysteresis.** The condition becomes $d_n>H$, with per-sample probability $Q(H/\sigma)$ — Lesson 7's Q-function, returning for a new purpose.

**TTT.** If the condition must hold for $N$ consecutive *independent* samples: probability $Q(H/\sigma)^N$ — **exponentially** suppressed in $N$. With $\sigma=4$ dB, $H=2$ dB: $Q(0.5)\approx0.3085$; for $N=5$, $0.3085^5\approx0.0028$ — about **0.28%**, versus $50\%$ with no protection.

**Filtering.** An exponential filter with coefficient $a$ reduces noise variance. For i.i.d. input, $F_n=\sum_k a(1-a)^kM_{n-k}$ gives
$$\text{Var}(F)=\sigma^2a^2\sum_k(1-a)^{2k}=\sigma^2\frac{a^2}{1-(1-a)^2}=\sigma^2\frac{a}{2-a}$$
For $k=4$ ($a=0.5$): $\text{Var}=\sigma^2/3$, so effective $\sigma_f\approx2.31$ dB, and $Q(H/\sigma_f)=Q(0.866)\approx0.193$; for $N=5$, $0.193^5\approx0.00027$ — roughly another $10\times$ improvement.

**Two honest caveats.** First, the $Q^N$ result assumes consecutive samples are *independent*; filtered samples are positively correlated (the filter mixes neighbors), and real shadowing is correlated over tens of meters, so true false-trigger rates are *higher* than $Q^N$ predicts — the formula is optimistic, exact only for independent samples. Second, the cost side: the false-trigger probability falls *exponentially* in $N$ while detection delay grows only *linearly* ($\approx N\cdot T_{meas}$ plus filter lag), so returns diminish quickly, and past a point extra TTT buys almost nothing while pushing the UE toward too-late handover.

### 6. Visualization

```
RSRP
  │  serving ‾‾╲___            neighbor rises, noisy
  │             ╲  ╲___      ╱╲╱‾‾‾
  │   hysteresis  ╲____╲___╱╱  
  │   band ──────────── ╲╱ ────── (neighbor must clear serving + H)
  │                      |<── TTT: must STAY above for N samples ──>|
  └──────────────────────────────────────────────────────────→ distance
                                         handover triggers here, not at first crossing
```

### 7. Worked example

$\sigma=4$ dB, $H=2$ dB, $N=5$, no filtering: per-sample $Q(0.5)=0.3085$, five-in-a-row $\approx0.28\%$ — compared with a 50% coin flip each sample. With $k=4$ filtering added: $\approx0.027\%$. Meanwhile detection delay for a UE moving at 30 m/s with a 0.5 s effective decision window means roughly 15 m of travel before triggering — the real price of that protection.

### 8. Code

```python
import numpy as np
from scipy.special import erfc
from scipy.signal import lfilter

def Q(x): return 0.5 * erfc(x / np.sqrt(2))

def trigger_fraction(sigma, H, N, n_samples=2_000_000, k=None, seed=0):
    rng = np.random.default_rng(seed)
    d = rng.normal(0, sigma, n_samples)          # boundary UE: true difference = 0
    if k is not None:
        a = 0.5 ** (k / 4)
        d = lfilter([a], [1, -(1 - a)], d)        # Sec 3's L3 filter
    above = (d > H).astype(int)
    c = np.concatenate([[0], np.cumsum(above)])
    return np.mean((c[N:] - c[:-N]) == N)        # last N samples ALL above H

sigma = 4.0
for H, N in [(0, 1), (2, 1), (2, 5)]:
    print(f"H={H} N={N} unfiltered: sim={trigger_fraction(sigma,H,N):.5f}  "
          f"theory={Q(H/sigma)**N:.5f}")

a = 0.5; sigma_f = sigma * np.sqrt(a / (2 - a))
print(f"filtered k=4, H=2, N=5: sim={trigger_fraction(sigma,2,5,k=4):.5f}  "
      f"iid-optimistic theory={Q(2/sigma_f)**5:.5f}")
```

### 9. Experiment

Run §8. The unfiltered rows should match $Q(H/\sigma)^N$ closely — direct confirmation of the derivation. The filtered row should come out **higher** than the iid-optimistic prediction (but still far below the unfiltered $N=5$ figure): the filter's induced correlation reduces TTT's effective independence, which is §5's first caveat made visible in your own data.

### 10. Expected output

Unfiltered simulated fractions ≈ theory ($\approx0.5$, $\approx0.31$, $\approx0.0028$). Filtered simulation above its iid-optimistic estimate yet well below the unfiltered result — protection real, but weaker than the naive formula claims.

### 11. Debugging exercise

Symptom, no cause given: a network has configured a TTT long enough that, by the $Q^N$ formula, ping-pong should be vanishingly rare — yet traces show a UE at slow walking speed still ping-ponging near one boundary.

Using §5's caveats: what assumption behind $Q^N$ is almost certainly failing, and why especially for a *slow* UE?

**Root cause (revealed):** the independence assumption. Shadowing decorrelates over *distance* (tens of meters), so a slow-moving UE sees nearly the *same* shadowing value across the entire TTT window — the "$N$ samples" aren't $N$ independent draws but effectively one or two. The exponential suppression therefore never materializes, and a persistent shadowing excursion beyond $H$ triggers handover reliably. The fix isn't more TTT (which can't help correlated samples) — it's a larger hysteresis $H$ (or offset) for that cell pair, which shifts the *threshold* rather than leaning on repetition. Distinguishing the two failure modes — noise too fast for the margin versus noise too *slow* for the repetition — is exactly why real networks tune $H$ and TTT per cell pair.

### 12. Common mistakes

- Cranking $H$ and TTT up "to be safe" without accounting for UE speed — the price is too-late handover and radio link failure, a worse failure than occasional ping-pong.
- Treating TTT's exponential suppression as unconditional — it holds only for independent samples (§5, §11).
- Using one global parameter set for every cell pair and every speed class — real deployments tune per boundary and per mobility profile.

### 13. Real RF/cellular application

Handover performance is tracked by real KPIs — ping-pong rate, too-early/too-late handover counts, radio-link-failure rate — and 3GPP's self-organizing-network framework includes **Mobility Robustness Optimization**, which automatically adjusts exactly these parameters per cell pair from observed failure statistics. This lesson is the quantitative reasoning underneath that automation, and a direct fit for the multi-layer trace analysis of Lesson 38.

### 14. Interview question

*"Why do handover triggers use both hysteresis and a time-to-trigger instead of just comparing signal strengths?"* Answer from §5: at the boundary the measured difference is dominated by noise, so a bare comparison fires on roughly half of all samples. Hysteresis cuts the per-sample false-trigger probability to $Q(H/\sigma)$; requiring $N$ consecutive samples raises that to the $N$th power — exponential suppression — at only linear cost in detection delay. A strong answer also names the independence caveat.

### 15. Active recall

1. Derive the false-trigger probability with hysteresis and $N$-sample TTT, and state its independence assumption.
2. Why does exponential suppression versus linear delay cost imply diminishing returns for larger TTT?
3. Why can a slow UE ping-pong despite a long TTT?

### 16. Mini assignment

Extend §8 to add a deterministic ramp (true difference rising linearly through zero) and measure, for several $(H,N)$ settings, both the false-trigger rate at the boundary *and* the detection delay after the true crossing — tabulating the real trade-off curve between them.

### 17. Exit test

1. Compute $Q(H/\sigma)^N$ for $\sigma=3$ dB, $H=3$ dB, $N=4$.
2. True or false, with a one-line reason: "Doubling TTT always halves ping-pong probability."
3. Your reasoning from §11 about correlated shadowing, if you haven't worked through it yet.

---

**Next:** Lesson 53 turns to **carrier aggregation** — combining multiple frequency carriers for higher peak throughput, and the real RF-front-end and scheduling constraints that decide what combinations a device can actually use.

---

[← Lesson 51](lesson-51-drx.md) · [Table of Contents](../README.md) · [Lesson 53 →](lesson-53-carrier-aggregation.md)