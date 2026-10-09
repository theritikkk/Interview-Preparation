[← Table of Contents](../README.md) · [← Lesson 49](lesson-49-proportional-fair-scheduling.md)

---

# Lesson 50 — Uplink Power Control: Open-Loop, Closed-Loop, and Why Fractional Compensation Wins

*A direct power-domain reprise of Lesson 18's frequency-reuse trade-off. Full path-loss compensation seems like the obviously "fair" choice for each individual UE — and it's provably the wrong choice for the system as a whole, for a reason that's genuinely derivable.*

### 1. Intuition

Lesson 18 showed why CDMA's shared-everything structure made tight power control essential, and why OFDMA's orthogonal resource allocation sidesteps much of that specific problem. But OFDMA/SC-FDMA systems still need power control — for a related but distinct reason: UEs at wildly different distances from their own cell create wildly different levels of interference to *neighboring* cells if left uncontrolled. Real systems combine **open-loop** power control (a UE estimates its own path loss from downlink measurements and sets initial power without any network round-trip) with **closed-loop** correction (explicit network commands fine-tune it) — and the single most interesting design parameter in the open-loop formula, the fractional compensation factor, turns out not to be a compromise but a provably better choice than the "obviously fair" alternative.

### 2. The engineering problem that forces this

Design a power-control rule a UE can compute entirely from its own downlink measurements (for fast initial access, no network signaling delay), while giving the network a real, tunable parameter controlling how much interference distant UEs are allowed to create at neighboring cells — and understand precisely why real deployments choose that parameter to be *less* than the value that would make each individual UE's own link "ideal."

### 3. Mathematical foundation

The standard open-loop uplink power control formula (LTE PUSCH, carried into NR with adaptations):
$$P_{TX} = \min\big(P_{MAX},\; P_0 + \alpha\cdot PL + 10\log_{10}(M) + \Delta_{MCS} + f(\text{closed-loop})\big)$$
$P_0$: network-configured target received power. $PL$: the UE's estimated downlink path loss (from reference-signal received power vs. known transmit power). $\alpha\in[0,1]$: the **fractional path-loss compensation factor**. $M$: assigned resource blocks. $f(\cdot)$: accumulated closed-loop TPC (Transmit Power Control) adjustments.

### 4. Formal definition

$\alpha=1$: **full compensation** — every UE arrives at its own cell at exactly the same target power $P_0$, regardless of distance. $\alpha<1$: **fractional (partial) compensation** — farther UEs deliberately transmit at less than the power full compensation would dictate, accepting a somewhat worse own-cell SINR.

### 5. Derivation — why full compensation is provably worse for the system, not just a design preference

With full compensation, a UE's transmit power scales as $\alpha\cdot PL_{own}$ with $\alpha=1$ — a cell-edge UE (high $PL_{own}$) must transmit at *high* power specifically to overcome its own large path loss and reach $P_0$ at its serving cell. But that same high-power signal, having a path loss to the *nearest neighboring cell* that is typically only somewhat lower than $PL_{own}$ (cell-edge means close to the neighbor too), creates severe interference there. This is a direct negative externality — optimizing each UE's own link SINR at the expense of every neighboring cell's SINR — structurally the identical trade-off as Lesson 18 §5(a)'s frequency-reuse argument, now expressed in the power domain instead of the frequency domain: a locally-optimal choice can be made at real expense to collective system capacity.

Make this precise: transmit power $\propto 10^{(\alpha\cdot PL_{own})/10}$ (converting the dB-scale formula to linear power) — **exponential** in $\alpha\cdot PL_{own}$. Interference caused at a neighbor scales with this same quantity, divided by the (comparatively small) path loss to that neighbor. Because the relationship is exponential in $\alpha \cdot PL_{own}$, reducing $\alpha$ has a **disproportionately large** effect specifically on the highest-$PL_{own}$ (cell-edge) UEs — exactly the UEs that would otherwise cause the *most* interference. A modest reduction from $\alpha=1$ to $\alpha=0.8$ therefore yields an outsized reduction in exactly the worst-offender interference, not a uniform, modest improvement spread evenly across all UEs — which is precisely why real deployments commonly use $\alpha$ around 0.7–0.8 rather than 1.0: system-level simulation and field experience consistently show this improves *overall and cell-edge* system throughput, despite nominally "sacrificing" each individual cell-edge UE's own best-case received power.

### 6. Visualization

```
Full compensation (α=1):                                         Fractional compensation (α=0.8):
cell-edge UE TX power: HIGH                                      cell-edge UE TX power: notably lower
         ╲                                                          ╲
          ╲                                                          ╲
           ╲___interference____► neighbor                             ╲___smaller footprint___► neighbor
                              cell (large footprint)                    cell (disproportionately reduced,
                                                                       per the exponential leverage in Sec 5)
```

### 7. Worked example

$PL_{own}=120$ dB (cell edge). At $\alpha=1$: required TX power term $\propto P_0+120$ dB-equivalent. At $\alpha=0.8$: $P_0+0.8\times120=P_0+96$ — a full **24 dB** reduction in required transmit power versus full compensation, for this one UE alone. That's a massive, concrete reduction in both battery drain and neighbor-cell interference footprint, at the cost of the UE's own received SINR sitting intentionally below the "ideal" $P_0$ target.

### 8. Code

```python
import numpy as np

def tx_power_db(P0, alpha, PL, M=1, delta_mcs=0, closed_loop=0, P_max=23):
    return min(P_max, P0 + alpha*PL + 10*np.log10(M) + delta_mcs + closed_loop)

def neighbor_interference_db(tx_power_db, PL_to_neighbor):
    return tx_power_db - PL_to_neighbor      # received interference power at neighbor, dB

P0 = -90
PL_own = 120        # cell-edge UE
PL_neighbor = 100    # closer to the neighbor than to its own serving cell

for alpha in [1.0, 0.8, 0.6, 0.4]:
    tx = tx_power_db(P0, alpha, PL_own)
    interference = neighbor_interference_db(tx, PL_neighbor)
    print(f"alpha={alpha}: TX power={tx:.1f} dBm   "
          f"interference at neighbor={interference:.1f} dBm")
```

### 9. Experiment

Run §8's sweep across $\alpha$ and separately compute the UE's own achieved SINR at its serving cell (received power minus noise floor, roughly $P_0$ scaled by $\alpha$'s shortfall from full compensation) versus interference caused at the neighbor. Plot one against the other — a genuine Pareto-frontier trade-off, with the exponential leverage from §5 visible as a disproportionately steep improvement in neighbor interference for only a modest own-SINR cost, specifically for high-path-loss UEs.

### 10. Expected output

TX power and neighbor interference both drop substantially as $\alpha$ decreases, with the drop being *steeper* than linear in $\alpha$ for the cell-edge case — directly reflecting the exponential-in-$PL_{own}$ relationship derived in §5, not a coincidence of the specific numbers chosen.

### 11. Debugging exercise

Symptom, no cause given: in a real network trace, a cell-edge UE's actual transmitted power (inferred from logs) is consistently *higher* than what the open-loop formula alone, with the configured $\alpha$ and $P_0$, would predict — by a roughly stable offset.

Is this necessarily a misconfiguration? Using §3's full formula: what term, besides the open-loop part, could legitimately account for a stable, consistent offset?

**Root cause (usually not a bug at all):** almost certainly the **closed-loop TPC term**, $f(\text{closed-loop})$, actively correcting for open-loop estimation error — for instance, real UL/DL path-loss asymmetry (a direct echo of Lesson 48 §12's FDD-reciprocity-failure point: the downlink measurement the UE used for its open-loop estimate isn't guaranteed to exactly match the real uplink path loss) or ordinary estimation noise. The network's closed-loop TPC commands exist *specifically* to correct for exactly this kind of open-loop imperfection — a power offset that's **consistent and stable** across time is the expected signature of closed-loop correction working as designed, not evidence of a fault. A signature worth distinguishing this from: wildly varying, unstable offsets would point toward a genuine problem instead.

### 12. Common mistakes

- Assuming $\alpha=1$ is simply "the correct choice" because full compensation sounds most intuitively fair to each individual UE — missing the system-level interference externality derived in §5 entirely.
- Confusing open-loop (self-computed, no network round-trip, fast) with closed-loop (explicit TPC commands, slower, corrects real errors) — or assuming either alone is sufficient in a real deployment, when real systems need both.
- Forgetting the $M$ (resource-block count) term — a UE assigned more bandwidth needs proportionally more *total* power to maintain the same power spectral density, an easy scaling factor to overlook.

### 13. Real RF/cellular application

Power control parameter tuning — $\alpha$ selection specifically — is a genuine, real RF network-planning and optimization task at real operators and infrastructure vendors, directly balancing cell-edge user experience against overall system capacity: exactly the RRM (radio resource management) algorithm design and tuning work within the "RAN Software Engineer" and network-optimization career tracks in this curriculum's map. UE battery life is also directly affected — lower required TX power is a real, user-visible consequence of these parameter choices.

### 14. Interview question

*"Why do real cellular networks use fractional, not full, path-loss compensation for uplink power control?"* Answer from §5: full compensation optimizes each UE's own received SINR at its own cell but creates a severe, exponentially-leveraged interference externality at neighboring cells specifically from the highest-path-loss (cell-edge) UEs. Fractional compensation deliberately sacrifices some of each individual UE's best-case own-cell performance in exchange for a disproportionately large reduction in the interference that same UE causes elsewhere — a genuine system-level optimization, directly parallel to Lesson 18's frequency-reuse trade-off but expressed in the power domain.

### 15. Active recall

1. Write the open-loop power control formula from memory and identify each term's purpose.
2. Why does reducing $\alpha$ have a disproportionately large effect on interference from cell-edge UEs specifically?
3. What real, non-fault cause explains a stable, consistent power offset above the open-loop-only prediction?

### 16. Mini assignment

Using §8's code, compute TX power and neighbor interference for three UEs (cell-edge, mid-cell, cell-center) across $\alpha\in\{1.0, 0.8, 0.6\}$, and identify which $\alpha$ value seems to offer the best overall system trade-off for your chosen scenario — turning the derivation into a concrete design decision.

### 17. Exit test

1. State the exponential-leverage argument from §5 in one sentence, and explain why it specifically favors cell-edge UEs' interference reduction.
2. True or false, with a one-line reason: "Full path-loss compensation (α=1) maximizes overall system capacity, even though it may seem unfair to individual cell-edge users."
3. Your reasoning from §11 about the stable power offset, if you haven't worked through it yet.

---

**Next:** Lesson 51 turns to **DRX (Discontinuous Reception)** — the sleep/wake cycling that saves real smartphone battery life while remaining fully connected, and the clean, derivable linear-cost-vs-diminishing-returns trade-off that determines how long a sleep cycle should actually be.

---

[← Lesson 49](lesson-49-proportional-fair-scheduling.md) · [Table of Contents](../README.md) · [Lesson 51 →](lesson-51-drx.md)