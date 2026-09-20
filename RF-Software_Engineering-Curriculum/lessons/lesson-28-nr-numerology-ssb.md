[← Table of Contents](../README.md) · [← Lesson 27](lesson-27-profiling-amdahl.md)

---

# Lesson 28 — 5G NR: Numerologies, the Flexible Resource Grid, and SSB — The Deltas from LTE

*Opens Stage 8 — 5G NR. As promised back in the book's resource stack: read this as the delta from LTE, not a new technology. Lessons 1–14's OFDM math is completely unchanged — NR's real innovation is making nearly every parameter LTE fixed into something configurable.*

### 1. Intuition

LTE fixed almost everything: one subcarrier spacing (15 kHz), one CP-length family, one implicit numerology for the whole system. NR's core move isn't reinventing OFDM — Lesson 12's cyclic-prefix derivation, Lesson 3's DFT math, all of it carries over completely unchanged — it's taking nearly every OFDM parameter LTE hard-coded and making it **scalable**, so one underlying framework can serve wildly different targets: eMBB (high-throughput mobile broadband), URLLC (ultra-low-latency), and mMTC (massive machine-type IoT) without needing three separate air interfaces. Understanding NR is overwhelmingly about understanding *what* became configurable and *why* — layered directly onto everything you already have from Lessons 19–21.

### 2. The engineering problem that forces this

LTE's single fixed 15 kHz subcarrier spacing is a poor fit for radically different deployment targets simultaneously: mmWave (FR2) needs wider channels and shorter symbols to combat worse phase noise and enable lower latency; massive IoT devices benefit from long, robust symbols; URLLC needs very short slot durations to hit tight latency budgets. NR's answer is **numerology**: scale subcarrier spacing — and everything that follows from it — by powers of 2 from LTE's 15 kHz baseline, letting one specification span dramatically different requirements through one configurable parameter rather than through separate standards.

### 3. Mathematical foundation

**NR numerology $\mu$:** subcarrier spacing $\Delta f_\mu = 15\cdot2^\mu$ kHz, for $\mu\in\{0,1,2,3,(4)\}$ → $\{15,30,60,120,(240)\}$ kHz. **BWP (Bandwidth Part):** a contiguous, independently-configurable subset of the full carrier bandwidth, with its **own** numerology, that a given UE actually operates within — unlike LTE, where every UE effectively sees the same fixed bandwidth and numerology.

### 4. Formal definition

**SSB** (Synchronization Signal Block) is NR's evolution of LTE's PSS/SSS/PBCH bundle (Lesson 19): the same three logical components, packaged as one compact block, transmitted periodically — and, genuinely new relative to LTE, **swept across multiple beam directions** in FR2 mmWave deployments, since a UE there must discover not just *when* a cell transmits but *which beam* to listen on.

### 5. Derivation — the numerology scaling relationship is forced by Lesson 3, not a separate design choice

Lesson 3 §4 already established that OFDM subcarrier spacing and useful symbol duration are reciprocals: $T_{symbol} = 1/\Delta f$ — this is not an NR-specific rule, it falls directly out of the DFT math already fully derived. So NR's numerology family $\Delta f_\mu = 15\cdot2^\mu$ kHz **automatically** forces:
$$T_\mu = \frac{1}{\Delta f_\mu} = \frac{1}{15\text{ kHz}} \cdot \frac{1}{2^\mu} = \frac{T_{LTE,useful}}{2^\mu}$$
Doubling subcarrier spacing isn't two independent decisions (wider subcarriers *and separately* shorter symbols) — it's **one** decision (choose $\mu$) with the symbol-duration consequence mathematically forced by Lesson 3's own relationship. NR additionally keeps LTE's 1 ms subframe / 10 ms frame structure as a deliberate backward-compatible outer synchronization grid; since each subframe must still span exactly 1 ms, and symbols got $2^\mu$ times shorter, a subframe now contains $2^\mu$ slots instead of 1. Everything in the following table is a *consequence* of choosing $\mu$, not a list of independent spec parameters someone picked separately.

### 6. Visualization

```
Numerology family (μ=0 matches LTE's baseline exactly):

μ=0: Δf=15kHz   T_symbol≈66.7µs   1 slot/subframe   (LTE-compatible)
μ=1: Δf=30kHz   T_symbol≈33.3µs   2 slots/subframe
μ=2: Δf=60kHz   T_symbol≈16.7µs   4 slots/subframe
μ=3: Δf=120kHz  T_symbol≈8.33µs   8 slots/subframe   (typical FR2/mmWave)

SSB beam sweeping (FR2):
beam1: [SSB] . . . . . .
beam2: . . . [SSB] . . .   ← same block, different direction, different time slot
beam3: . . . . . . [SSB]      (UE must find which beam AND which timing)
```

### 7. Worked example

$\mu=3$ (120 kHz, typical FR2 numerology): $T_{symbol} = 1/120\text{kHz} \approx 8.33\,\mu s$. Check against $\mu=0$: $66.7\,\mu s / 8 \approx 8.33\,\mu s$ — exact match, confirming $2^\mu$ scaling numerically, not just symbolically. Slots per subframe: 8, versus $\mu=0$'s 1 — eight times the scheduling granularity within the same 1 ms subframe, directly enabling URLLC's tighter latency budgets.

### 8. Code

```python
def numerology_params(mu):
    delta_f_khz = 15 * 2**mu
    t_symbol_us = 1e3 / delta_f_khz          # 1/Δf, in microseconds
    slots_per_subframe = 2**mu
    return delta_f_khz, t_symbol_us, slots_per_subframe

for mu in range(5):
    df, ts, slots = numerology_params(mu)
    print(f"mu={mu}: Δf={df:6.1f} kHz   T_symbol={ts:6.2f} µs   "
          f"slots/subframe={slots}   min sched. latency ≈ {1000/slots:.3f} ms")
```

### 9. Experiment

Run §8's code and confirm the exact $2^\mu$ scaling in every column. Then reason about the latency payoff directly: minimum achievable scheduling granularity is roughly one slot duration, so $\mu=3$'s slots (0.125 ms each) allow scheduling decisions roughly 8× more often than $\mu=0$'s (1 ms) — a concrete, numeric link between a numerology choice and URLLC's actual latency requirement, not just an abstract claim.

### 10. Expected output

A clean table confirming $T_\mu = T_0/2^\mu$ and slots-per-subframe $=2^\mu$ exactly, plus the derived minimum scheduling latency shrinking proportionally — numeric confirmation that higher numerology is chosen specifically *for* the latency payoff, not as an arbitrary spec variation.

### 11. Debugging exercise

Symptom, no cause given: your NR receiver correctly detects SSB and computes seemingly reasonable timing, but when you align PDSCH decoding using LTE-style assumptions (14 symbols per 1 ms, one fixed numerology), everything is systematically misaligned — and the misalignment is worse on some cells than others, not uniform.

Given that different NR cells/BWPs can genuinely use different $\mu$, and per §5, $T_\mu$ scales by exactly $2^\mu$: what would explain misalignment that's *cell-dependent* specifically?

**Root cause (revealed):** hard-coding **LTE's fixed numerology assumption** ($\mu=0$ implicitly) instead of reading the cell's *actual* configured numerology from its broadcast configuration before assuming any timing structure. Since NR has no single fixed numerology the way LTE does, this bug is invisible on any cell that happens to use $\mu=0$ (matching the hard-coded assumption by coincidence) and produces misalignment that scales predictably with $2^\mu$ on any cell using a higher numerology — exactly explaining "worse for some cells, not others." This is a direct generalization of Lesson 19's warning against assuming fixed parameters, now applied to numerology specifically instead of channel bandwidth.

### 12. Common mistakes

- Treating NR as "a completely different technology" from LTE rather than the same OFDM/PHY mathematics (Lessons 1–14, entirely unchanged) with more parameters made configurable — a genuine, common overstatement that undersells how much you already know once you've done Lessons 1–14 properly.
- Forgetting that **CP length must scale with numerology too** — shorter symbols need proportionally shorter CPs to preserve the same fractional overhead, directly reconnecting to Lesson 12 §12's CP-length-vs-delay-spread trade-off: a high-numerology cell deployed somewhere with significant delay spread has strictly less absolute CP margin to work with than the shorter symbol duration alone might suggest.
- Assuming a BWP is just "a smaller slice of the same grid" rather than potentially carrying its **own independent numerology** — this is the genuinely novel part relative to anything LTE has, not merely a bandwidth restriction.

### 13. Real RF/cellular application

**BWP switching for power savings** is a real, actively deployed NR feature — a UE moves to a narrower BWP during low-activity periods specifically to reduce RF front-end power draw, a real specialization within modem power-management engineering. **Numerology selection** is a genuine, live deployment trade-off balanced directly against delay spread and coverage (higher numerology, shorter CP, less multipath tolerance — Lesson 12's trade-off, now an actual per-deployment decision rather than a fixed spec constant). **SSB beam-sweeping** is precisely why FR2/mmWave initial access measurably takes longer than sub-6GHz — a well-known, practical characteristic directly explained by §4's SSB definition.

### 14. Interview question

*"Why does 5G NR support multiple numerologies instead of one fixed subcarrier spacing like LTE?"* Answer from §2 and §5: fundamentally different deployment targets — mmWave, URLLC, mMTC — have genuinely different requirements for symbol duration, latency, and robustness that no single fixed $\Delta f$ can simultaneously satisfy; NR's $\Delta f=15\cdot2^\mu$ family lets one underlying OFDM framework (mathematically unchanged from Lessons 1–14) serve all of them by scaling a single parameter, rather than requiring genuinely separate air interfaces.

### 15. Active recall

1. Derive, in one line, why doubling subcarrier spacing necessarily halves symbol duration.
2. Why does a subframe contain $2^\mu$ slots rather than a fixed number, and what design choice forces that?
3. What's genuinely new about a BWP compared to anything in LTE?

### 16. Mini assignment

Extend §8's table to include an approximate CP duration (using LTE's normal-CP ratio of roughly 7% of symbol duration as a rough guide) for each $\mu$, and identify which numerology would suit a large rural macro-cell (long delay spread, needs generous CP margin) versus a small indoor mmWave hotspot (short delay spread, wants minimum latency) — connecting the numeric table to a real deployment decision.

### 17. Exit test

1. Write the numerology formula for $\Delta f_\mu$ and derive $T_\mu$ from it, from memory.
2. True or false, with a one-line reason: "5G NR's PHY layer uses fundamentally different mathematics from LTE's OFDM."
3. Your reasoning from §11 about cell-dependent misalignment, if you haven't sent it.

---

**Next:** Lesson 29 continues Stage 8 with **PDCCH/PDSCH in NR** and **DMRS-based channel estimation** — the same Lesson 20/11 machinery, generalized for NR's flexible numerology and BWP structure.

---

[← Lesson 27](lesson-27-profiling-amdahl.md) · [Table of Contents](../README.md)