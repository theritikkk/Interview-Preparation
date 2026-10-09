[← Table of Contents](../README.md) · [← Lesson 48](lesson-48-csi-feedback-compression.md)

---

# Lesson 49 — Proportional Fair Scheduling: Balancing Throughput and Fairness

*Lesson 20 explained what PDCCH signals; Lesson 29 explained how CQI is measured. Neither explained the actual decision: with many UEs competing for the same TTI's resources, who wins? This lesson closes that gap with a derivation connecting a simple, practical scheduling rule to a real fairness-optimization criterion.*

### 1. Intuition

Every TTI, a real scheduler must pick who gets the limited time-frequency resources. The naive rule — always schedule whoever has the best instantaneous channel — maximizes total system throughput but can completely starve users with persistently weaker channels (cell-edge users, say), a real fairness failure. **Proportional Fair (PF)** scheduling, the algorithm that actually dominates real commercial LTE/NR deployments, balances these competing goals elegantly — and it isn't an arbitrary compromise; it falls directly out of optimizing a specific, principled fairness objective.

### 2. The engineering problem that forces this

Design a scheduling metric that still exploits multi-user diversity (favor good instantaneous conditions, to keep system throughput high) while guaranteeing every user gets a fair, nonzero long-run share of resources, rather than being starved indefinitely just because their average channel is worse than others'.

### 3. Mathematical foundation

The **PF metric** for user $i$ at time $t$: $\text{PF}_i(t) = R_i(t)/T_i(t)$, where $R_i(t)$ is user $i$'s instantaneous achievable rate right now (from current CQI, Lesson 29), and $T_i(t)$ is their **exponentially-averaged** recent throughput. Schedule whoever has the highest $\text{PF}_i(t)$ each TTI.

### 4. Formal definition

The throughput average updates every TTI for **every** user: $T_i(t{+}1) = (1-1/t_c)T_i(t) + (1/t_c)R_i(t)$ if scheduled this TTI, else $T_i(t{+}1) = (1-1/t_c)T_i(t)$ — the decay branch applies even to users who *don't* win, which §11 will show is not optional bookkeeping.

### 5. Derivation — PF as log-utility fairness, not an arbitrary heuristic

Consider the fairness objective $\sum_i \log T_i$ — a standard embodiment of diminishing marginal value (a user already getting a lot gains much less marginal benefit from one more scheduled TTI than a user getting very little, exactly what produces fairness rather than throughput-only greed). A myopic, greedy scheduler picks, each TTI, whichever user's scheduling produces the largest instantaneous increase in this objective. Using §4's update rule, the per-TTI change for a scheduled user is approximately $\Delta T_i \approx (1/t_c)(R_i - T_i)$; the resulting change in $\log T_i$ is, by the chain rule ($\frac{d}{dT_i}\log T_i = 1/T_i$):
$$\Delta(\log T_i) \approx \frac{1}{T_i}\cdot\frac{R_i-T_i}{t_c}$$
For the dominant, near-term comparison across users (treating the $-T_i/t_c$ term as a common, roughly-equal offset across candidates), maximizing this is maximizing $R_i/T_i$ — **exactly the PF rule**, derived from a genuine fairness-utility optimization, not asserted as a sensible-sounding heuristic. This connection (proportional fairness ⟺ log-utility maximization) is classical, well-established scheduling theory, not specific to this curriculum's presentation of it.

### 6. Visualization

```
Two users' instantaneous rate R_i(t) over time:
User A (consistently good):  ~~~~~~~~~~~~~~~~~~~  (high, low variance)
User B (cell-edge, bursty):  _.-'‾'-._.-'‾'-._    (low average, occasional peaks)

"Always best": A wins almost every TTI — B starved.
PF: picks whoever is relatively good vs THEIR OWN average —
    B's occasional peaks win THEIR TTIs, A still wins most others.
```

### 7. Worked example

User A: $R_A=10$ Mbps now, $T_A=8$ Mbps average → $\text{PF}_A=1.25$. User B: $R_B=3$ Mbps now, $T_B=1$ Mbps average (but this TTI is unusually good *for them*) → $\text{PF}_B=3.0$. The scheduler picks **B**, despite B's far lower absolute rate — relative to their own typical conditions, B is having a standout moment, and the PF criterion says to exploit it specifically for them.

### 8. Code

```python
import numpy as np

def simulate(n_ttis, mean_rates, variances, policy, t_c=100):
    n_users = len(mean_rates)
    T = np.ones(n_users) * 0.1
    scheduled_count = np.zeros(n_users)
    total_delivered = np.zeros(n_users)

    for t in range(n_ttis):
        R = np.maximum(0, np.random.normal(mean_rates, variances))
        if policy == "pf":
            winner = np.argmax(R / T)
        elif policy == "best":
            winner = np.argmax(R)
        elif policy == "round_robin":
            winner = t % n_users

        for i in range(n_users):
            if i == winner:
                T[i] = (1-1/t_c)*T[i] + (1/t_c)*R[i]
                scheduled_count[i] += 1
                total_delivered[i] += R[i]
            else:
                T[i] = (1-1/t_c)*T[i]                 # decay for NON-winners too, Sec 4
    return total_delivered, scheduled_count

mean_rates = [8, 2]       # User A: consistently good; User B: cell-edge
variances  = [1, 1.5]

for policy in ["best", "round_robin", "pf"]:
    delivered, counts = simulate(5000, mean_rates, variances, policy)
    jains = (sum(delivered))**2 / (len(delivered) * sum(d**2 for d in delivered))
    print(f"{policy:12s} total={sum(delivered):8.0f}  per-user={delivered}  "
          f"Jain's fairness={jains:.3f}")
```

### 9. Experiment

Run §8's comparison across `"best"`, `"round_robin"`, and `"pf"`. Confirm PF achieves meaningfully higher total throughput than round-robin (it still exploits multi-user diversity) while achieving meaningfully better fairness (Jain's index closer to 1) than always-best (which should show User B badly starved) — the actual, measured "best of both worlds" result that explains PF's real industry dominance, not just an assertion of it.

### 10. Expected output

`"best"`: highest raw total throughput, very poor fairness (User B's delivered total near zero). `"round_robin"`: best fairness, lower total throughput (ignores channel quality entirely). `"pf"`: total throughput meaningfully above round-robin, fairness meaningfully above "best" — sitting genuinely between the two extremes, not merely an average of them.

### 11. Debugging exercise

Symptom, no cause given: in your PF simulation, one user's average throughput $T_i$ converges to a value that seems far too *low* given how often you'd intuitively expect them to occasionally win the PF competition.

Using §4's exact update rule: what happens to $T_i$ on TTIs where user $i$ is **not** scheduled, and what would break if that case were implemented incorrectly?

**Root cause (revealed):** almost certainly **forgetting to decay $T_i$ for non-winning users** — updating $T_i$ only for the TTI's winner and leaving everyone else's average frozen. This corrupts the entire mechanism: $T_i$ is supposed to represent average throughput *over all time*, including the (typically many) TTIs a user isn't scheduled, not just "my average rate on the TTIs I happen to win." A frozen, stale $T_i$ for non-winners either keeps them artificially appearing "overdue" (if frozen too high) or never properly reflects their declining share (if frozen too low) — either way, it breaks the fairness guarantee §5 derived, since that derivation assumed $T_i$ genuinely tracks the time-average the log-utility objective is defined over.

### 12. Common mistakes

- §11's decay bug — the single most common, most damaging PF implementation error.
- Treating $R_i(t)$ as a fixed per-user constant rather than a genuinely time-varying, CQI-derived quantity (Lesson 8/29) that must be re-evaluated fresh every TTI.
- Assuming PF is the *only* criterion real schedulers use — production schedulers typically layer QoS-class-specific weighting (prioritizing URLLC or voice traffic, say) on top of a PF-style base metric, a real refinement beyond this lesson's base derivation.

### 13. Real RF/cellular application

Proportional fair scheduling (or close variants) is the dominant real scheduling algorithm in commercial LTE and NR base stations — first popularized in CDMA2000 1xEV-DO/HDR systems and carried forward as a standard LTE/NR MAC-layer approach. Deriving and explaining it precisely is a genuine, substantive topic in MAC-layer and scheduler algorithm-design interviews at infrastructure/RAN companies, directly tied to the "RAN Software Engineer" role family in this curriculum's career map.

### 14. Interview question

*"Why do real cellular schedulers use proportional fair scheduling instead of always scheduling whichever user currently has the best channel?"* Answer from §5: always-best maximizes raw throughput but can starve persistently weaker-channel users entirely — a real fairness failure. PF instead maximizes a log-utility fairness objective, which is derivably equivalent to the simple $R_i(t)/T_i(t)$ greedy rule — still exploiting multi-user diversity while guaranteeing every user a fair, nonzero long-run share.

### 15. Active recall

1. Derive the PF rule from the log-utility objective, from memory.
2. Why must $T_i$ decay for non-scheduled users every TTI, not just update for the winner?
3. What does $t_c$ physically control, and what would a very short versus very long $t_c$ do to scheduling behavior?

### 16. Mini assignment

Extend §8's simulation to 4+ users with varied channel statistics, compute Jain's fairness index for all three policies, and report concrete numbers — turning "PF balances throughput and fairness" from an assertion into something you measured yourself.

### 17. Exit test

1. Write the PF scheduling rule from memory, and state the fairness objective it derives from.
2. True or false, with a one-line reason: "A scheduler that always picks the user with the best instantaneous channel maximizes long-run fairness."
3. Your reasoning from §11 about the non-winner decay bug, if you haven't worked through it yet.

---

**Next:** Lesson 50 turns to **uplink power control** — open-loop and closed-loop, and why real networks deliberately use *fractional*, not full, path-loss compensation, a direct power-domain reprise of Lesson 18's frequency-reuse trade-off.

---

[← Lesson 48](lesson-48-csi-feedback-compression.md) · [Table of Contents](../README.md) · [Lesson 50 →](lesson-50-uplink-power-control.md)