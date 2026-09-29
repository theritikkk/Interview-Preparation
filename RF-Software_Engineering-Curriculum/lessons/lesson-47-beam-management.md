[← Table of Contents](../README.md) · [← Lesson 46](lesson-46-beamforming-mrt.md)

---

# Lesson 47 — NR Beam Management: P1/P2/P3, and Why Hierarchical Search Beats Exhaustive Search

*Lesson 28 gave coarse SSB-based beam discovery. Lesson 46 derived the optimal weights for a fine, high-gain beam. This lesson closes the gap between them — and the efficiency argument for why NR does this in stages is the same reasoning that makes binary search beat linear search.*

### 1. Intuition

Lesson 28's SSB sweep gives a coarse, wide-beam starting point for both the network and UE. Lesson 46 showed how to compute optimal weights for a *known* channel — but at mmWave, the highest-gain beams are extremely narrow, and there are many of them, and a UE moves. Sweeping every narrow beam exhaustively, everywhere, all the time, would be enormously expensive in signaling. NR's answer is a genuinely hierarchical procedure — coarse first, then progressively refined — and the efficiency gain from doing it this way, rather than an exhaustive fine search from the start, is directly derivable, not just "how the spec happens to define it."

### 2. The engineering problem that forces this

You need a procedure that starts from Lesson 28's coarse, cheap-to-sweep wide beams, progressively refines to much narrower, higher-gain beams without exhaustively sweeping the *entire* angular range at fine resolution, and then continuously *tracks* the resulting narrow beam pair as the UE moves — without constant, expensive full re-sweeps.

### 3. Mathematical foundation

For a uniform array of $N_{ant}$ elements with spacing $d$, beamwidth scales roughly as $\theta \approx \lambda/(N_{ant}d)$ — more elements produce a narrower beam and higher directional gain. Number of beams needed to exhaustively cover a total angular range $\Theta$ at beamwidth $\theta$: approximately $\Theta/\theta$.

### 4. Formal definition

**P1**: initial, coarse beam sweep — the network and UE establish a usable beam pair using wide beams, exactly Lesson 28's SSB procedure. **P2**: network-side refinement — the gNB sweeps a small set of *narrower* candidate beams, but only within the angular window P1 already identified, using CSI-RS (configurable far more densely than the fixed, periodic SSB) rather than SSB itself. **P3**: UE-side refinement — the analogous sweep of the UE's own receive beam/panel, with the gNB's beam held fixed. **Beam failure recovery**: a scoped, fast procedure — structurally a lightweight cousin of Lesson 30's PRACH machinery — for reacquiring a beam pair within an otherwise-still-valid connection, not tearing down and rebuilding the whole connection.

### 5. Derivation — the efficiency argument for hierarchical beam search

Direct exhaustive fine search cost: $\Theta/\theta_{narrow}$ narrow-beam measurements, covering the full range at final resolution. Two-stage cost: $\Theta/\theta_{wide}$ (P1, coarse, cheap) $+ \theta_{wide}/\theta_{narrow}$ (P2, fine, but only searching the single wide beam's worth of angular uncertainty P1 already narrowed down to — since that's the entire remaining unknown after P1 commits to one wide-beam direction). Total: $\Theta/\theta_{wide} + \theta_{wide}/\theta_{narrow}$, versus $\Theta/\theta_{narrow}$ for direct search. For any realistic case where $\theta_{narrow}\ll\theta_{wide}\ll\Theta$, the two-stage total is dramatically smaller — this is structurally the identical efficiency argument behind binary search beating linear search, and behind Lesson 34's "discriminating test first" debugging methodology: pay a small, fixed cost to eliminate most of the search space, then only search exhaustively within what remains.

### 6. Visualization

```
P1 (wide beams, full sector):        P2 (narrow beams, ONLY within the
[■■■■][■■■■][■■■■]                    winning P1 beam's angular window):
        ↑ winner                              [▪▪▪▪▪▪▪▪▪▪]
                                        10 narrow beams, not 40
```

### 7. Worked example

Wide beams: 30° each, 4 beams cover a 120° sector (P1 cost: 4). Narrow beams: 3° each — a direct exhaustive fine sweep of the full 120° would need $120/3=40$ measurements. Two-stage: P1's 4 + P2's search of just the winning 30° window at 3° resolution ($30/3=10$) = **14 total measurements**, versus 40 for direct search — a real, computed ~3× efficiency gain from the hierarchy alone, not an assumption.

### 8. Code

```python
def sweep_cost(total_range, wide_bw, narrow_bw, two_stage=True):
    if not two_stage:
        return total_range / narrow_bw
    p1_cost = total_range / wide_bw
    p2_cost = wide_bw / narrow_bw
    return p1_cost + p2_cost

total_range = 120
for narrow_bw in [10, 5, 3, 1]:
    direct = sweep_cost(total_range, 30, narrow_bw, two_stage=False)
    hier = sweep_cost(total_range, 30, narrow_bw, two_stage=True)
    print(f"narrow_bw={narrow_bw}°: direct={direct:.0f}  hierarchical={hier:.0f}  "
          f"savings={direct/hier:.1f}x")
```

### 9. Experiment

Run §8's sweep across shrinking `narrow_bw` (representing more antenna elements, per §3's beamwidth formula) and observe the efficiency gain of the hierarchical approach *growing* as beams get narrower — the more antennas a real massive-MIMO system has, the more valuable this hierarchical structure becomes, not less.

### 10. Expected output

Efficiency gain increasing monotonically as `narrow_bw` shrinks — directly demonstrating that hierarchical beam search isn't a fixed-percentage optimization, but one that matters *more* precisely as real systems push toward the narrower, higher-gain beams massive MIMO is built to provide.

### 11. Debugging exercise

Symptom, no cause given: a UE using a well-established, previously-working narrow beam pair experiences a sudden, complete link failure — not gradual degradation — despite no UE movement and apparently stable RF conditions moments before.

Given this lesson's focus on narrow, high-gain beams specifically: what real physical cause produces a *sharp* failure rather than a gradual one, and why would it be specifically more likely with narrow beams than wide ones?

**Root cause (revealed):** most likely a sudden **blockage** — a person, vehicle, or other obstacle moving directly into the narrow beam's path. This is a genuine, physically-derivable downside of high directional gain: a narrow beam concentrates energy along one specific path with comparatively little inherent path diversity, so a small obstacle can completely interrupt it, unlike a wide or omnidirectional pattern which has many alternate paths available. (Rapid UE rotation — instantly invalidating the P3-selected UE-side beam even with zero translation — is the other realistic candidate; distinguish the two by whether beam-failure recovery using a *different*, previously-reported candidate beam pair immediately restores the link. If it does, this confirms a beam-specific failure rather than a general RF/system problem — precisely why NR beam management maintains multiple candidate reports rather than tracking only the single current best beam.)

### 12. Common mistakes

- Assuming narrow, high-gain beams are strictly better than wider coverage in every respect — they trade blockage robustness and tracking overhead for peak gain, a genuine engineering trade-off, not an unconditional improvement.
- Confusing P2 (network-side refinement) with P3 (UE-side refinement) — they refine *different* ends of the same link, and a real system generally needs both.
- Treating beam failure recovery as equivalent to full RACH-based initial access (Lesson 30) — it's a scoped, faster procedure for recovering a beam pair within an otherwise-still-valid connection, not a full connection rebuild.

### 13. Real RF/cellular application

Beam management is the defining operational characteristic of real FR2/mmWave 5G deployments, and beam-tracking success/failure rates are a genuine, actively monitored KPI in commercial operations — directly analyzable with Lesson 38's multi-layer trace methodology. Understanding the P1/P2/P3 hierarchy's *efficiency rationale*, not just its existence, is a real differentiator in 5G NR PHY/RRM (radio resource management) interviews specifically.

### 14. Interview question

*"Why does 5G NR use a hierarchical, coarse-then-fine beam sweeping procedure instead of directly sweeping the finest, highest-gain beams from the start?"* Answer from §5: directly analogous to why binary search beats linear search — exhaustive fine sweeping over the full angular range costs proportionally to (total range / narrow beamwidth), which grows large for genuinely narrow, high-gain beams. A two-stage approach pays a small, fixed coarse-sweep cost once, then fine-sweeps only the small window the coarse stage already narrowed to, reaching the same final resolution at dramatically lower total cost — a quantifiable efficiency argument, not an arbitrary spec choice.

### 15. Active recall

1. Derive the hierarchical sweep-cost formula and explain why it beats direct exhaustive search.
2. Why does narrow-beam blockage tend to produce a sharp failure rather than gradual degradation?
3. What's the precise difference between P2 and P3?

### 16. Mini assignment

Using §3's beamwidth formula and §8's cost calculator, compute the resulting narrow beamwidth and hierarchical-search savings for a realistic massive-MIMO array (e.g. 64 or 256 elements) — connecting the abstract efficiency argument to a concrete, realistic antenna count.

### 17. Exit test

1. Write the hierarchical sweep-cost formula from memory and state why it grows more favorable as beams narrow.
2. True or false, with a one-line reason: "Beam failure recovery and initial random access (PRACH) are the same procedure with different names."
3. Your reasoning from §11 distinguishing blockage from rotation as causes of sharp beam failure, if you haven't worked through it.

---

**Next:** Lesson 48 turns to **massive MIMO CSI feedback compression** — with dozens or hundreds of antenna elements, raw channel-state feedback becomes its own real bandwidth problem, and NR's codebook-based compression scheme is a genuinely elegant solution worth deriving.

---

[← Lesson 46](lesson-46-beamforming-mrt.md) · [Table of Contents](../README.md) · [Lesson 48 →](lesson-48-csi-feedback-compression.md)