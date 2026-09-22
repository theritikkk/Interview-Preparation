[← Table of Contents](../README.md) · [← Lesson 32](lesson-32-nr-ssb-msequence.md)

---

# Lesson 33 — NR's Protocol Stack: SDAP and RRC_INACTIVE — The Deltas from LTE's Layers

*Opens Stage 11 for NR. Lesson 21 explained why RLC, PDCP, and RRC each exist as forced consequences of specific constraints. NR keeps that entire layered architecture — and adds exactly one new layer and one new state, each solving a problem that genuinely didn't press as hard in LTE's original design context.*

### 1. Intuition

Lessons 20–21 built LTE's protocol stack by asking, for each layer, "what specific problem forces this to exist exactly here, and not one layer up or down?" NR's protocol stack is overwhelmingly the *same* layered architecture, answering the *same* questions the same way. Two genuine additions exist, and both are forced by real shifts in what the network needs to support: a new layer, **SDAP**, and a new intermediate connection state, **RRC_INACTIVE**. Neither is arbitrary — each solves a problem that LTE's original design either didn't have or didn't have as urgently.

### 2. The engineering problem that forces this

NR was designed from the outset to support far more diverse, differentiated QoS requirements on one network simultaneously — network slicing across eMBB, URLLC, and mMTC traffic — than LTE's original bearer-based QoS model was built around. Something has to map IP-layer QoS flows onto the specific radio bearers RLC/PDCP actually schedule and prioritize, and neither Lesson 21's PDCP nor RLC has a clean place to do that mapping without conflating it with their existing jobs. Separately: real modern smartphone traffic is extremely bursty — short data exchanges followed by idle gaps, repeated constantly. LTE's two-state model (fully torn-down IDLE vs. fully set-up CONNECTED) makes every single burst pay the full cost of connection re-establishment, which is expensive in both latency and battery for traffic that behaves this way as its *normal* pattern, not an edge case.

### 3. Mathematical foundation

This lesson, like Lesson 21, is reasoning-heavy rather than math-heavy — the content is architectural forcing functions, not derivations. **QFI** (QoS Flow Identifier): a per-flow tag distinguishing differentiated QoS treatment within a single PDU session. **AS context**: the UE's security keys and bearer configuration state, whose retention (or release) is the key variable distinguishing NR's three RRC states.

### 4. Formal definition

**SDAP** (Service Data Adaptation Protocol) sits directly **above** PDCP, mapping IP-layer QoS flows (identified by QFI) onto specific NR radio bearers — each configured with its own PDCP/RLC treatment (e.g., duplication for URLLC reliability, relaxed settings for mMTC). **RRC_INACTIVE** is a third RRC state, distinct from both IDLE and CONNECTED: the UE's AS context is **retained** at both the UE and a designated serving ("anchor") gNB — unlike IDLE, where it's fully released — enabling a lightweight **resume** procedure instead of full re-establishment.

### 5. Derivation — both additions as forced consequences, not preferences

**(a) Why SDAP, and why specifically above PDCP, not folded into it.** LTE's one-bearer-roughly-one-QoS-treatment model was a reasonable fit for its original target traffic. NR's target explicitly includes many differentiated QoS flows — potentially with very different latency/reliability needs — coexisting within a single connection. PDCP (Lesson 21's ciphering-and-handover-continuity layer) has no natural home for this mapping without conflating two genuinely separate concerns: security/handover continuity, and QoS-flow-to-bearer assignment. Rather than overload PDCP with a second, unrelated job, NR adds SDAP as a clean, single-purpose layer immediately above it — exactly Lesson 21's "why does this problem live at exactly this layer" reasoning, applied to a genuinely new problem NR's traffic mix created.

**(b) Why RRC_INACTIVE is a real third state, not "IDLE with extra steps."** Staying fully CONNECTED between bursts wastes radio resources sitting idle and costs UE battery continuously. Dropping fully to IDLE between every burst forces the *next* burst to pay full re-establishment cost — expensive signaling, multiple round trips, latency the user directly feels. For genuinely bursty traffic — the dominant real smartphone pattern — oscillating between these two extremes is wasteful on *both* axes simultaneously, for every single burst. RRC_INACTIVE resolves this by splitting what gets retained from what gets released: retain the **expensive-to-rebuild** part (security context, bearer configuration) at the UE and its anchor gNB, while releasing the **comparatively cheap-to-reacquire** part (the actual radio resource allocation) between bursts. That specific split — not merely "a state between the other two" — is what makes it a genuinely distinct, non-redundant addition rather than a relabeling of an existing state.

### 6. Visualization

```
NR protocol stack (delta from Lesson 21's LTE diagram — one new layer, above PDCP):
IP packet
   ↓
[SDAP] ← NEW: maps QoS flows (QFI) to radio bearers — this lesson's §5(a)
   ↓
[PDCP] ← unchanged from Lesson 21: ciphering, handover continuity
   ↓
[RLC] [MAC] [PHY] ← unchanged from Lessons 20-21

Three RRC states (NR):
IDLE ←──── (full setup / full release, expensive both ways) ────→ CONNECTED
  ╲                                                              ╱
   ╲                                                            ╱
    ╲                                                          ╱
     ╲                                                        ╱
      ╲                                                      ╱
       ╲                                                    ╱
        ╲───────────────────── INACTIVE  ──────────────────╱
                (context retained, resources released)
        ↑ lightweight "resume" — the forced middle ground, §5(b)
```

### 7. Worked example

A UE with bursty app traffic (a messaging app checking for updates every so often): under an IDLE-only policy, every burst pays full connection setup — multiple signaling round trips, each one costing both latency the user notices and battery. Under an INACTIVE-capable policy, the same burst pattern pays a much lighter resume cost for gaps short enough that context is still retained, only falling back to full IDLE-style setup for genuinely long gaps — meaningfully less total signaling overhead across a realistic session, without the ongoing resource cost of staying fully CONNECTED the whole time.

### 8. Code

```python
import random

# Toy, illustrative cost model -- NOT real spec-cited latency numbers,
# just relative costs to make the tradeoff from Sec 5(b) concrete and measurable.
COST_FULL_SETUP = 10     # IDLE -> CONNECTED: many signaling round trips
COST_RESUME     = 3      # INACTIVE -> CONNECTED: lightweight resume
COST_TO_INACTIVE = 1     # CONNECTED -> INACTIVE: cheap, network-initiated
COST_TO_IDLE     = 1     # CONNECTED -> IDLE: also cheap to release

def simulate(gap_lengths, policy):
    total_cost = 0
    for gap in gap_lengths:
        if policy == "idle_only":
            total_cost += COST_TO_IDLE + COST_FULL_SETUP
        elif policy == "inactive_aware":
            if gap < 5:                      # short gap: use INACTIVE
                total_cost += COST_TO_INACTIVE + COST_RESUME
            else:                             # long gap: fall back to IDLE
                total_cost += COST_TO_IDLE + COST_FULL_SETUP
    return total_cost

random.seed(0)
bursty_gaps = [random.choice([1,2,3,4,8,12]) for _ in range(200)]   # realistic bursty mix

print("IDLE-only total cost:     ", simulate(bursty_gaps, "idle_only"))
print("INACTIVE-aware total cost:", simulate(bursty_gaps, "inactive_aware"))
```

### 9. Experiment

Run §8's simulation and compare total signaling cost between the two policies across the same realistic, bursty gap pattern. The INACTIVE-aware policy should show meaningfully lower total cost — a concrete, quantifiable (if illustrative rather than spec-exact) demonstration of exactly why this third state exists, rather than an assertion to take on faith.

### 10. Expected output

`inactive_aware` total cost noticeably lower than `idle_only`, with the gap widening further if you skew `bursty_gaps` toward more short gaps — matching the intuition that INACTIVE's advantage is specifically largest for genuinely bursty traffic, and shrinks (or could even reverse, depending on the illustrative cost constants) for traffic with mostly long idle periods.

### 11. Debugging exercise

Symptom, no cause given: in a protocol trace, you observe a UE transition from RRC_CONNECTED to RRC_INACTIVE, then shortly after — well within what should be a safe resume window — a **full** RRC re-establishment back to CONNECTED instead of a lightweight resume.

Given §4's definition of what RRC_INACTIVE actually retains, and where: what could make a resume fail even though the timing gap itself was short?

**Root cause (revealed):** the UE likely moved to a **different cell/gNB** than the one holding its retained AS context (the "anchor" gNB) during the inactive period, and the network-side context transfer between gNBs either wasn't attempted or failed (e.g., a backhaul issue) — directly echoing Lesson 21 §5(c)'s handover-continuity reasoning, but showing a case where that continuity does **not** automatically hold. RRC_INACTIVE's fast-resume benefit is a common-case optimization contingent on the new cell successfully obtaining the UE's retained context; when that fails, the network correctly falls back to full re-establishment rather than proceeding with an inconsistent or missing context — a real, expected failure mode, not a bug.

### 12. Common mistakes

- Treating RRC_INACTIVE as "basically IDLE" or "basically CONNECTED" rather than a genuinely distinct state defined by its specific retain/release split (§5(b)) — the split itself, not just "being in between," is what matters.
- Assuming SDAP **replaces** PDCP's function rather than sitting cleanly above it, doing a different job entirely — a common layering confusion given both are "new-ish" concepts relative to LTE.
- Assuming a UE in INACTIVE can *always* resume via the lightweight path regardless of circumstances — §11 shows a real condition (mobility to an unprepared cell) under which it legitimately cannot, making INACTIVE an optimization for the common case, not an unconditional guarantee.

### 13. Real RF/cellular application

SDAP's QoS-flow mapping is directly how real, deployed 5G networks deliver differentiated network-slicing SLAs — a URLLC industrial-IoT slice and an eMBB video-streaming slice genuinely coexisting on one physical network with distinct treatment. RRC_INACTIVE is a direct, real contributor to measurably better smartphone battery life during typical bursty app usage compared to an LTE-style IDLE/CONNECTED-only design, and INACTIVE-to-CONNECTED resume success/failure rates are exactly the kind of real KPI tracked in commercial network operations — tying directly back to the protocol-diagnostic-tooling theme running throughout this curriculum's career map.

### 14. Interview question

*"Why does 5G NR introduce a third RRC state, RRC_INACTIVE, instead of using LTE's IDLE and CONNECTED alone?"* Answer from §5(b): bursty modern traffic makes both LTE-style extremes costly — CONNECTED wastes resources sitting idle, IDLE forces expensive full re-setup for every burst. INACTIVE is the forced middle ground: retain the expensive-to-rebuild context, release the comparatively cheap-to-reacquire radio resource, giving a fast resume without paying CONNECTED's ongoing cost.

### 15. Active recall

1. Why does SDAP sit above PDCP specifically, rather than being folded into PDCP's existing responsibilities?
2. What specific split of "what's retained vs. released" makes RRC_INACTIVE a genuinely distinct state?
3. Under what real condition can a UE in RRC_INACTIVE fail to use the lightweight resume path?

### 16. Mini assignment

Extend §8's simulation with a third, adaptive policy that only routes a UE into INACTIVE when its *observed* recent gap-length distribution is short, falling back to an IDLE-only strategy otherwise — and compare its total cost against both simpler policies across a gap sequence that shifts partway through from bursty to sparse.

### 17. Exit test

1. In one sentence, what problem does SDAP solve that didn't need solving as urgently in LTE's original design?
2. True or false, with a one-line reason: "A UE in RRC_INACTIVE always retains its full radio resource allocation, the same as in RRC_CONNECTED."
3. Your reasoning from §11 about the resume-vs-fallback failure mode, if you haven't sent it.

---

**Next:** Lesson 34 turns to **Stage 12 — Testing and Debugging**: building the first of this curriculum's promised symptom-first debugging scenarios as a real, hands-on exercise — now drawing on 33 lessons of genuine implementation experience rather than theory alone.

---

[← Lesson 32](lesson-32-nr-ssb-msequence.md) · [Table of Contents](../README.md) · [Lesson 34 →](lesson-34-debugging-methodology.md)