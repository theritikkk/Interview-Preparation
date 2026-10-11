[← Table of Contents](../README.md) · [← Lesson 50](lesson-50-uplink-power-control.md)

---

# Lesson 51 — DRX: The Latency-vs-Battery Trade-off, Derived

*Lesson 33 covered RRC_INACTIVE — a connection-state-level power saver. DRX is the complementary mechanism that saves power even while a UE stays fully RRC_CONNECTED. The trade-off it navigates has a clean, exact derivation: linear latency cost against a saturating power benefit.*

### 1. Intuition

Lesson 33's RRC_INACTIVE saves power by leaving the CONNECTED state entirely between bursts. **DRX (Discontinuous Reception)** does something different and complementary: even while remaining RRC_CONNECTED — able to receive scheduled data — a UE can turn its receiver *off* between periodic "wake windows" where it checks PDCCH for grants. The rest of the time it sleeps, saving real, significant power. But the network can only reach the UE during those wake windows, so sleeping longer saves more battery while adding more latency before new data can be delivered. Unlike a vague "there's a trade-off," this one has an exact, derivable shape.

### 2. The engineering problem that forces this

Design a sleep/wake schedule that balances battery savings against acceptable latency, given that new data arrivals are genuinely unpredictable. Is there a principled way to reason about how long the cycle should be, rather than picking a number that merely sounds reasonable?

### 3. Mathematical foundation

A DRX cycle consists of an **On Duration** $T_{on}$ (UE monitors PDCCH) followed by sleep, for a total cycle length $T_{cycle}$. Real NR/LTE also uses a two-tier scheme: a **short DRX cycle** immediately after recent activity (data likely clustered — real traffic is bursty, not memoryless), transitioning via an **inactivity timer** to a **long DRX cycle** if no activity occurs.

### 4. Formal definition

**Duty cycle** $= T_{on}/T_{cycle}$. **Average power** $\approx \text{duty}\cdot P_{on} + (1-\text{duty})\cdot P_{sleep}$. **Added latency** is the extra wait, beyond normal processing, before a newly-arrived packet can actually be scheduled — because the UE may be asleep when the packet arrives.

### 5. Derivation — exact latency cost, and why power savings saturate

**(a) Expected added latency.** Assume a packet arrives at a time uniformly distributed within a DRX cycle (a standard, tractable modeling assumption, reasonable absent knowledge of arrival timing being correlated with the DRX schedule). The wait until the *next* wake window at time $T_{cycle}$, for an arrival at time $t\in[0,T_{cycle}]$, is $T_{cycle}-t$. Averaging uniformly:
$$E[\text{wait}] = \frac{1}{T_{cycle}}\int_0^{T_{cycle}}(T_{cycle}-t)\,dt = \frac{1}{T_{cycle}}\cdot\frac{T_{cycle}^2}{2} = \frac{T_{cycle}}{2}$$
Exact, elementary calculus — not an approximation. Expected added latency grows **linearly** with cycle length.

**(b) Why the power benefit saturates, while the latency cost doesn't.** Average power $=\frac{T_{on}}{T_{cycle}}P_{on}+\left(1-\frac{T_{on}}{T_{cycle}}\right)P_{sleep}$. As $T_{cycle}\to\infty$ with fixed $T_{on}$, duty cycle $\to0$ and power approaches the **floor** $P_{sleep}$ — it cannot go lower than always-sleeping. The marginal power saved per additional unit of $T_{cycle}$ shrinks as $1/T_{cycle}^2$ (the derivative of $T_{on}/T_{cycle}$), while the marginal latency cost per additional unit of $T_{cycle}$ is a constant $1/2$. So a real, derivable asymmetry exists: **linear, unbounded latency cost versus a benefit that saturates against a hard floor** — meaning there's a genuine region where each additional millisecond of cycle length buys almost nothing in power while still costing the same fixed amount in latency. A principled design should sit *before* that region, not in it.

### 6. Visualization

```
DRX timeline:
[ON][..........sleep..........][ON][..........sleep..........][ON]
 ↑ UE listens here only          ↑ a packet arriving here waits
                                    until the NEXT ON window

Latency vs power as T_cycle grows:
latency:    ╱  (linear, unbounded)
           ╱
          ╱
power saved:  ╭───────────  (saturates toward the floor)
             ╱
            ╱
```

### 7. Worked example

$T_{on}=10$ ms, $T_{cycle}=320$ ms (a realistic long-DRX-scale value): expected added latency $=160$ ms; duty cycle $=10/320\approx3.1\%$, so the receiver is on only about 3% of the time — roughly a 97% reduction in receiver-on time compared to staying continuously awake, at a real, quantifiable average latency cost of 160 ms.

### 8. Code

```python
import numpy as np

def simulate_drx(n_packets, T_on, T_cycle, seed=0):
    rng = np.random.default_rng(seed)
    arrival_phase = rng.uniform(0, T_cycle, n_packets)       # arrival within the cycle
    waits = np.where(arrival_phase < T_on, 0.0, T_cycle - arrival_phase)
    return waits.mean()

def avg_power(T_on, T_cycle, P_on=1.0, P_sleep=0.02):
    duty = T_on / T_cycle
    return duty * P_on + (1 - duty) * P_sleep

T_on = 10
for T_cycle in [20, 40, 80, 160, 320, 640, 1280]:
    sim_wait = simulate_drx(200_000, T_on, T_cycle)
    print(f"T_cycle={T_cycle:5d}ms  simulated wait={sim_wait:7.2f}ms  "
          f"theory (T_cycle/2)={T_cycle/2:7.2f}ms  avg power={avg_power(T_on, T_cycle):.3f}")
```

### 9. Experiment

Run §8's sweep. The simulated average wait should track $T_{cycle}/2$ closely — direct confirmation of §5(a)'s derivation (small deviations arise because arrivals landing *inside* the on-window incur zero wait, slightly pulling the average below the exact $T_{cycle}/2$ figure). Meanwhile average power should flatten visibly at large $T_{cycle}$, confirming §5(b)'s saturation claim rather than asserting it.

### 10. Expected output

Wait growing proportionally to $T_{cycle}$; power savings improving steeply at small $T_{cycle}$ and then flattening toward $P_{sleep}$ — a real, plotted illustration of the linear-versus-saturating asymmetry from §5.

### 11. Debugging exercise

Symptom, no cause given: your DRX simulation's power numbers improve with $T_{cycle}$ exactly as expected, but a separate latency measurement under a realistic traffic trace comes out roughly 2× *worse* than the $T_{cycle}/2$ formula predicts.

Using §5(a)'s stated assumption: what must be true about real traffic for this derivation to hold, and what plausible reality violates it?

**Root cause (revealed):** §5(a)'s $T_{cycle}/2$ result assumes arrival times are **uniformly random relative to the DRX schedule**. Real traffic is **bursty and correlated**: packets cluster, and critically, if arrivals happen to correlate with *when the UE just went back to sleep* (e.g., a response packet arriving right after the UE finishes an exchange and the inactivity timer expires), the average wait can systematically exceed the uniform-arrival average — the formula remains exactly correct *for its stated assumption*, but the assumption itself doesn't match this traffic. The transferable lesson is Lesson 35/36's "verify against reality, don't trust a clean formula's applicability unexamined" — worth checking a derivation's assumptions against measured traffic before concluding the formula "failed."

### 12. Common mistakes

- Using the same DRX cycle for every application regardless of latency sensitivity — real systems configure different parameters per QoS requirement (gaming/voice versus background sync), directly echoing Lesson 33's SDAP QoS-flow differentiation.
- Confusing DRX (periodic sleep *within* CONNECTED) with RRC_INACTIVE/IDLE (Lesson 33's connection-state-level saving) — genuinely complementary, different mechanisms operating at different levels, typically used together in a real device's overall power strategy.
- Forgetting the On-Duration window itself consumes real power even when nothing is actually scheduled during it — an unavoidable baseline cost of any periodic-polling power-saving scheme, easy to leave out of a naive power estimate.

### 13. Real RF/cellular application

DRX parameters (cycle length, on-duration, inactivity timer) are real, RRC-configured, per-UE values in live networks, and poorly tuned DRX is a documented real source of field battery-life complaints — squarely within this curriculum's protocol-diagnostic and field-failure-diagnosis thread (Lesson 38's methodology applies directly to diagnosing it).

### 14. Interview question

*"Why does lengthening the DRX cycle show diminishing returns for battery savings while latency cost keeps growing?"* Answer from §5: expected added latency is exactly $T_{cycle}/2$ — linear and unbounded — while power savings approach a hard floor ($P_{sleep}$) as duty cycle shrinks toward zero, with marginal benefit shrinking as $1/T_{cycle}^2$. A linear cost against a saturating benefit defines a real sweet spot, not an arbitrary choice.

### 15. Active recall

1. Derive $E[\text{wait}]=T_{cycle}/2$ from the uniform-arrival assumption.
2. Why does the marginal power benefit of a longer DRX cycle shrink while the marginal latency cost stays constant?
3. What assumption underlies §5(a), and what real traffic property can violate it?

### 16. Mini assignment

Extend §8's sweep to find, numerically, the $T_{cycle}$ beyond which the marginal power saved per additional 10 ms of cycle length drops below a threshold you choose (e.g. 0.1% of total power) — identifying a defensible, derived sweet-spot recommendation instead of an arbitrary pick.

### 17. Exit test

1. State the expected-latency result and the average-power formula from memory.
2. True or false, with a one-line reason: "DRX and RRC_INACTIVE are two names for the same power-saving mechanism."
3. Your reasoning from §11 about why a derivation can be correct yet fail to match a real trace, if you haven't worked through it yet.

---

**Next:** Lesson 52 turns to **handover** — when a moving UE should switch cells, and how hysteresis plus time-to-trigger turn a coin-flip decision into a reliable one, with the Q-function from Lesson 7 returning to quantify it.

---

[← Lesson 50](lesson-50-uplink-power-control.md) · [Table of Contents](../README.md) · [Lesson 52 →](lesson-52-handover.md)