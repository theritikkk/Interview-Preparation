[← Table of Contents](../README.md) · [← Lesson 29](lesson-29-nr-dmrs-coreset.md)

---

# Lesson 30 — PRACH: Random Access, and Zadoff-Chu Reused for Timing Advance

*Stage 8 — 5G NR, closing the essential arc. Lesson 19's Zadoff-Chu proof gets reused for a second, genuinely different purpose — not just "is it there," but "exactly how far away is it."*

### 1. Intuition

Before a UE can be scheduled (Lessons 20 and 29), it must first announce itself to a network that has never heard from it — and critically, the network must learn something cell search alone can never reveal: how *far* the UE actually is, so its future uplink transmissions arrive time-aligned at the gNB despite an unknown propagation delay. **PRACH** solves detection and ranging with a single transmission, reusing the exact ideal-autocorrelation property of Zadoff-Chu sequences proven in Lesson 19 — except this time, the correlation peak's *position* carries genuinely new information, not just "found it."

### 2. The engineering problem that forces this

Multiple UEs at different distances may attempt random access at overlapping times, using preambles from a shared pool, with completely unknown initial uplink timing (a UE has only *downlink* timing from SSB — its own uplink propagation delay is exactly what's unknown). The network must detect which preamble(s) arrived, distinguish simultaneous attempts from different UEs, and derive a precise **timing advance (TA)** value instructing each UE how much to advance its future transmissions — the individual-UE-level version of the synchronization theme Lesson 18 raised for TDD generally.

### 3. Mathematical foundation

Same Zadoff-Chu definition as Lesson 19: $x_u[n]=e^{-j\pi un(n+1)/N_{ZC}}$, now with **long format** ($N_{ZC}=839$, larger cells) or **short format** ($N_{ZC}=139$, smaller cells/higher numerologies). Instead of using different root indices per identity (as PSS does), PRACH primarily assigns different UEs **different cyclic shifts of the same root sequence**: $x_{u,S}[n] = x_u[(n+S)\bmod N_{ZC}]$.

### 4. Formal definition

The **Zero Correlation Zone (ZCZ)**, of size $N_{cs}$ samples, is the spacing between adjacent assigned cyclic shifts — sized specifically to exceed the cell's maximum expected round-trip propagation delay, proven necessary in §5. **Timing advance** is the correction a UE applies to its transmit timing so that, accounting for its own propagation delay, its uplink signal arrives at the gNB aligned with the network's timing reference.

### 5. Derivation — extending Lesson 19's proof to extract ranging information, and why ZCZ size is forced, not arbitrary

Let the UE transmit using assigned shift $S_1$: $x_{u,S_1}[n]=x_u[(n+S_1)\bmod N_{ZC}]$. Real propagation adds an additional, unknown delay $\tau$ (in samples) before the gNB receives it — the received signal is effectively $x_u$ shifted by $(S_1+\tau)$. The gNB correlates the received signal against its **local, unshifted** reference $x_u[n]$ (root, zero shift). Correlating a shifted copy of a ZC sequence against its own unshifted root is exactly evaluating Lesson 19 §5's ideal autocorrelation function $R(k)$ at the shifted lag — still **zero everywhere except a single sharp peak**, now located precisely at $k=S_1+\tau$, by the identical "sum of roots of unity vanishes" proof, no new mathematics required.

This single correlation therefore reveals **both** pieces of information at once: since the network knows the legitimate assigned-shift positions $\{0, N_{cs}, 2N_{cs},\dots\}$, it decomposes the observed peak position $P$ as $P = (\text{nearest legitimate shift } S_1) + (\text{residual } \tau)$ — the nearest shift bin identifies *which* UE/preamble attempt this was, and the residual $\tau$ *is* the round-trip delay, directly convertible into a timing advance instruction. This decomposition is only unambiguous if $N_{cs}$ **safely exceeds** the cell's maximum possible round-trip delay in samples — otherwise a distant UE's own $\tau$ could push its peak into or past the *next* UE's assigned shift zone, corrupting both UEs' identification simultaneously. This is precisely why $N_{cs}$ must be sized from the cell's configured maximum radius (round-trip delay $=2\times\text{radius}/c$), and precisely why larger cells need the long preamble format ($N_{ZC}=839$, room for a larger $N_{cs}$) while smaller cells can use the short format and pack shift-based opportunities more tightly.

### 6. Visualization

```
Correlation peak position decomposes into: assigned shift + round-trip delay

Assigned shift zones, each width N_cs (the guard/ZCZ):
[ UE-A zone: 0..Ncs-1 ][ UE-B zone: Ncs..2Ncs-1 ][ UE-C zone: 2Ncs..3Ncs-1 ]
        ↑ peak lands here = S1 + τ, safely inside UE-A's zone if τ < Ncs

If τ (round-trip delay) exceeds Ncs:
        ↑ peak lands HERE instead — inside UE-B's zone — ambiguous/wrong identification
```

### 7. Worked example

A cell with radius 1 km: round-trip delay $=2\times1000\text{m}/(3\times10^8\text{m/s})\approx6.67\,\mu s$. Whatever $N_{cs}$ is configured, it must correspond to **at least** this many microseconds of guard spacing at PRACH's sampling rate — a cell planned for a larger radius needs proportionally more guard-zone margin, directly motivating the long-format ($N_{ZC}=839$) choice for wide-area macro cells versus the short format for dense small cells with inherently much smaller maximum round-trip delay.

### 8. Code

```python
import numpy as np

def zc_sequence(u, N):
    n = np.arange(N)
    return np.exp(-1j*np.pi*u*n*(n+1)/N)

N_ZC = 139          # short format, for this toy example
u = 1
N_cs = 20            # zero correlation zone size (samples)

root = zc_sequence(u, N_ZC)

# Simulate one UE: assigned shift S1, plus real round-trip delay tau (samples)
S1, tau = 40, 7      # tau < N_cs: should be unambiguous
shifted = np.roll(root, S1 + tau)

corr = np.abs(np.correlate(np.concatenate([shifted, shifted]), root, mode='valid')[:N_ZC])
peak_pos = np.argmax(corr)

nearest_shift = round(peak_pos / N_cs) * N_cs
residual_delay = peak_pos - nearest_shift

print(f"true shift={S1}, true delay={tau}")
print(f"detected peak={peak_pos}, decomposed shift={nearest_shift}, decomposed delay={residual_delay}")
```

### 9. Experiment

Run §8's code, then sweep `tau` from 0 up to and past `N_cs`. For `tau < N_cs`, the decomposed shift and delay should exactly match the true values. Once `tau` approaches or exceeds `N_cs`, watch the decomposed shift jump to the *wrong* bin — a direct, reproducible demonstration of §5's ZCZ-violation ambiguity, not merely a claim.

### 10. Expected output

Correct shift and delay recovery throughout the safe region ($\tau<N_{cs}$), with a sharp, unambiguous failure — a wrong shift bin, not gradual degradation — exactly at the boundary predicted by §5's derivation.

### 11. Debugging exercise

Symptom, no cause given: at a real cell edge, two *different* UEs' PRACH attempts — using different assigned cyclic shifts of the same root sequence — occasionally appear to the gNB as a single UE's attempt with a strange, inconsistent timing-advance estimate.

Using only §5's derivation: what specifically has gone wrong, and why would it happen *specifically* at the cell edge rather than for UEs closer in?

**Root cause (revealed):** exactly the **ZCZ violation** from §5. A UE at the cell edge has the largest possible round-trip delay $\tau$ for that cell; if the configured $N_{cs}$ was sized too small for the cell's actual radius (or the UE is genuinely beyond the cell's planned coverage), that UE's peak lands inside or beyond an adjacent UE's assigned shift zone — conflating two distinct random-access attempts into what looks like one, with a timing-advance estimate that reflects neither UE's true delay correctly. This is precisely why cell planning must choose the preamble format and $N_{cs}$ to match the cell's actual maximum supported radius, per §4/§5's sizing rule — this failure mode is a direct, real-world consequence of that configuration being mismatched to deployment.

### 12. Common mistakes

- Treating PRACH detection as "just PSS again" — the genuinely new requirement is extracting an actionable, precise timing-advance *value* from the peak's position, not merely a binary detection decision, per §5.
- Sizing $N_{cs}$ for the *current* UE's actual distance rather than the cell's configured *maximum* expected radius — a logical inversion, since the whole point of random access is that the UE's distance is exactly what's unknown beforehand.
- Confusing preamble **format** choice (long vs. short — a cell-level radio configuration, tied to planned coverage radius) with preamble **index/shift** choice (a per-attempt UE selection from the pool the cell has configured) — two entirely different configuration levels.

### 13. Real RF/cellular application

Timing advance is a live, continuously-updated MAC-layer parameter for connected UEs, refreshed by ongoing procedures as a UE moves — not a one-time RACH value. Correct initial TA estimation is exactly what allows a synchronized cellular uplink to function with multiple simultaneous UEs at different ranges without their transmissions colliding at the gNB. RACH success/failure rates are a standard, real, continuously-monitored KPI in commercial deployments — directly tied to the modem-diagnostic-tooling emphasis running throughout this curriculum's career map.

### 14. Interview question

*"PRACH detection uses the same Zadoff-Chu correlation technique as PSS cell search. What's the genuinely new capability PRACH needs, and how does the zero-correlation-zone design address it?"* Answer from §5: PRACH must extract an actionable round-trip-delay (timing advance) value from the correlation peak's position, not merely detect presence — requiring the guard spacing between different UEs' assigned cyclic shifts to be deliberately sized larger than the cell's maximum expected round-trip delay, so a real UE's propagation delay can never be confused with a different UE's identity.

### 15. Active recall

1. In one sentence, why does correlating a shifted ZC sequence against its own unshifted root reveal the shift amount exactly?
2. What determines the minimum safe size of the zero correlation zone, precisely?
3. Why does a larger planned cell radius require the long preamble format rather than the short one?

### 16. Mini assignment

Extend §9's sweep into a full report: for `N_cs` values of 10, 20, 40, and 80, find the maximum `tau` each can safely support before the decomposition fails, and convert that maximum `tau` into an equivalent maximum cell radius using the round-trip-delay relationship from §7 — producing your own, derived version of the long-vs-short format sizing table.

### 17. Exit test

1. Write the round-trip-delay-to-distance relationship and compute it for a 5 km cell radius.
2. True or false, with a one-line reason: "PRACH long format exists purely for historical/legacy compatibility reasons, unrelated to cell size."
3. Your reasoning from §11 about the cell-edge conflation symptom, if you haven't sent it.

---

**Next:** This closes the essential NR arc (Lessons 28–30: numerology/SSB, DMRS/CORESET, PRACH) alongside LTE's equivalent arc (Lessons 19–21) — between them, you now have enough PHY-layer grounding for Project 7 and Project 8 (the scoped LTE and NR PHY learning implementations) to stop being theoretical and become genuinely buildable. Lesson 31 turns to actually assembling one: a scoped LTE PSS/SSS/PBCH decode chain in C++, using Lessons 19 and 22–27's implementation techniques together for the first time on a real, specified signal.

---

[← Lesson 29](lesson-29-nr-dmrs-coreset.md) · [Table of Contents](../README.md) · [Lesson 31 →](lesson-31-lte-pss-sss-pbch-pipeline.md)