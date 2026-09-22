[← Table of Contents](../README.md) · [← Lesson 33](lesson-33-nr-sdap-rrc-inactive.md)

---

# Lesson 34 — A Real Debugging Investigation: BER Spikes, and the Methodology of Systematic Root-Cause Elimination

*Opens Stage 12 — Testing and Debugging. Every debugging exercise in this book so far had exactly one, pre-identified cause, tied to that lesson's one new concept. A real bug doesn't arrive labeled "this is a Lesson 13 problem." This lesson is about the actual skill: efficiently narrowing a wide-open hypothesis space that spans your entire receiver, not learning one more specific failure mode.*

### 1. Intuition

A single observable symptom — "BER is unexpectedly high" — can stem from causes spanning nearly every lesson in this book: noise (Lesson 7), ISI (Lesson 9), multipath (Lesson 10), a bad equalizer (Lesson 11), OFDM structural issues (Lesson 12), timing/CFO (Lesson 13), fixed-point overflow (Lesson 25), a concurrency bug (Lesson 24), or an interface-contract mismatch (Lesson 31). A junior debugging instinct jumps to whichever cause is most familiar or most recently learned. A real methodology instead treats debugging as **systematic hypothesis elimination**: list what's plausible, then choose measurements that rule out the most candidates per unit of effort — not the ones that feel most likely.

### 2. The engineering problem that forces this

Given one symptom and a large space of plausible root causes spanning your whole system, how do you decide *what to check first* in a way that's actually efficient — rather than randomly poking at things, or defaulting to personal bias about what's "probably" wrong?

### 3. Mathematical foundation

Borrowed directly from information theory (the same idea behind binary search and good differential diagnosis): each possible test partitions your current hypothesis set into "consistent with this outcome" and "ruled out by it." The most valuable test to run next is the one that **discriminates best** among the hypotheses still alive — ideally splitting them roughly in half — not one that only distinguishes a single, already-unlikely candidate from the rest.

### 4. Formal definition

The methodology, as an explicit loop: **(1)** enumerate every plausible hypothesis given the symptom, without pruning by gut feeling yet. **(2)** Rank candidate tests by how well each discriminates among currently-live hypotheses, weighted by cost — cheap, already-available information beats an expensive new experiment that tells you the same thing. **(3)** Run the top-ranked test. **(4)** Eliminate every hypothesis inconsistent with the result. **(5)** Repeat until one hypothesis remains. **(6)** Confirm it by predicting a **further, independent, falsifiable** consequence of that specific hypothesis and checking it — correlation with the first test alone is not confirmation.

### 5. Working the actual investigation

**The symptom, given with no cause revealed:** your OFDM receiver decodes cleanly on a short signal recorded seconds after startup. Let the identical system run continuously for several minutes and decode a segment recorded later — BER has become significantly elevated, though nothing about the transmitted signal or the physical setup changed.

**Step 1 — list the plausible hypotheses, honestly, before touching anything:**
- **H1**: LO/CFO thermal drift — real oscillators drift in frequency as hardware physically warms up during continuous operation (Lesson 13).
- **H2**: A slow memory/resource leak in a long-running C++ process, eventually causing missed deadlines or dropped samples (Lessons 23–24).
- **H3**: Accumulating numerical error in some stateful, recursive fixed-point computation — a running estimator whose internal accumulator slowly saturates (Lesson 25).
- **H4**: Slowly-settling DC offset / AGC behavior that wasn't actually stable even at "startup" (Lesson 15).

**Step 2 — rank candidate tests by discrimination and cost, not by which hypothesis feels likeliest.** An expensive, deep audit of the fixed-point accumulator state (directly testing H3 only) is a poor first move — costly, and uninformative about H1, H2, or H4. A much better first move: **check whether your receiver already logs a CFO estimate as part of normal operation** (Lesson 13's tracking). If it does, this costs *nothing new* — it's existing instrumentation — and its outcome discriminates cleanly: a smooth, monotonic drift correlating with run duration is specific to H1; a memory leak (H2) wouldn't produce that specific *smooth frequency drift* signature; a fixed-point overflow (H3) would show a sudden discontinuity, not gradual drift; DC-offset settling (H4) wouldn't manifest as a CFO estimate change at all. One cheap check, and it discriminates among all four.

**Step 3 — run it.** The logged CFO estimate does show a slow, steady drift that grows with run duration.

**Step 4 — eliminate.** H2, H3, and H4 are now implausible as the primary cause; H1 remains standing.

**Step 5 — confirm with an independent, falsifiable prediction (§4 step 6), not just the correlation you already found.** If this really is thermal LO drift, the drift should track a *temperature* proxy (board/CPU temperature reading, or physical warm-up time since power-on rather than since process-start) — and it should be **correctable** simply by re-running the CFO estimator periodically throughout operation, rather than needing any structural fix. Checking this predicts something H2/H3/H4 never would have. This is also not new physics: it's precisely the residual-CFO tracking need already derived in Lesson 13 §11's common-phase-error discussion — the real fix here is exactly what that lesson already told you real receivers do, encountered now as a genuine multi-hypothesis diagnosis instead of a narrow, pre-labeled exercise.

### 6. Visualization

```
Hypothesis set: { H1: CFO drift, H2: memory leak, H3: fixed-pt overflow, H4: DC drift }

Test: check EXISTING CFO log (free, already instrumented)
        │
        ├── smooth, monotonic drift ──► consistent with H1 only ──► H2,H3,H4 eliminated
        │
        └── (had it shown a sudden jump, or no correlation at all,
             a completely different branch of tests would follow)

Confirm: does drift track a temperature proxy, and does periodic re-tracking fix it?
        │
        └── YES ──► H1 confirmed, not just correlated
```

### 7. What the wrong first move would have cost

Suppose instead you'd started with H3 — a deep audit of every fixed-point accumulator in the pipeline, checking each for overflow (Lesson 25's technique). That investigation is real, valid work — but it's expensive (touches many stages), and a clean result only rules out *one* of four hypotheses, leaving H1, H2, and H4 exactly as open as before. The CFO-log check cost nothing and discriminated among all four simultaneously. Test *order*, not just test *validity*, is a real, measurable efficiency difference.

### 8. Code

```python
# A small, reusable tool: given hypotheses and candidate tests (with their
# possible outcomes mapped to which hypotheses survive each outcome),
# rank tests by how well they discriminate -- Sec 3's principle, made concrete.

def discrimination_score(hypotheses, test_outcomes):
    """test_outcomes: dict of outcome -> set of surviving hypotheses.
       Returns how evenly this test would split the hypothesis set (higher = better)."""
    sizes = [len(surviving) for surviving in test_outcomes.values()]
    total = len(hypotheses)
    # A perfect binary split scores highest; a test that never changes
    # the surviving set (same size as total, every outcome) scores lowest.
    return total - min(sizes) if sizes else 0

hypotheses = {"H1_cfo_drift", "H2_mem_leak", "H3_fixedpt_overflow", "H4_dc_drift"}

candidate_tests = {
    "check_existing_cfo_log": {
        "drift_seen":    {"H1_cfo_drift"},
        "no_drift_seen": {"H2_mem_leak", "H3_fixedpt_overflow", "H4_dc_drift"},
    },
    "deep_audit_fixed_point_accumulators": {
        "overflow_found":  {"H3_fixedpt_overflow"},
        "no_overflow":     {"H1_cfo_drift", "H2_mem_leak", "H4_dc_drift"},
    },
}

for name, outcomes in candidate_tests.items():
    print(name, "-> discrimination score:", discrimination_score(hypotheses, outcomes))
```

### 9. Experiment

Run §8's tool on the two candidate tests above. `check_existing_cfo_log` should score at least as well as the accumulator audit on pure discrimination — and it's free, which the score alone doesn't capture but which any real prioritization must weigh alongside it. Extend the tool with a rough cost estimate per test and combine cost with discrimination score into one ranking, then confirm it recommends checking existing instrumentation before any new, expensive experiment.

### 10. Expected output

`check_existing_cfo_log` ranks as a strong first move on discrimination alone, and becomes an even clearer winner once cost is factored in — a concrete, tool-assisted version of the reasoning worked through by hand in §5, not just an assertion to trust.

### 11. Practice: a second scenario, for you to work through yourself

**Symptom, no cause given:** your matched-filter-based preamble detector (Lessons 6, 19, 30) works reliably in simulation and on captures from most of your SDR hardware, but fails intermittently specifically on captures from **one particular unit** — even though the transmitted signal and protocol are identical to the hardware that works fine.

Apply §4's loop yourself: list every plausible hypothesis you can think of (consider Lessons 15, 17, and others — hardware-specific impairments, file-format issues, clock accuracy, genuinely different physical conditions for that capture set), then propose what you'd check *first* and explain why it's the most discriminating, lowest-cost option among your hypotheses — not just the one you suspect most. Send me your hypothesis list and proposed first test; I'll tell you what it reveals and we'll work the elimination together, the same way §5 did.

### 12. Common mistakes

- Jumping to the first hypothesis that comes to mind — usually whatever you most recently learned or most recently touched in the code — instead of listing the full plausible set first, per §4 step 1.
- Running an expensive, deep test before checking cheap, already-available instrumentation, exactly §7's counterfactual.
- Treating a correlation found in step 4 as sufficient confirmation, skipping §4 step 6's independent falsifiable prediction — mistaking "consistent with" for "proven."
- Abandoning the systematic loop under time pressure and reverting to guessing once frustrated — a real, human tendency, worth naming and guarding against deliberately rather than pretending it won't happen.

### 13. Real RF/cellular application

This exact systematic hypothesis-elimination methodology — not additional technical knowledge alone — is what distinguishes efficient debugging from slow, frustrating guesswork in real RF/PHY/modem engineering. Lessons 1–33 gave you the technical knowledge of *what* could plausibly be wrong; this lesson is about finding the actual cause efficiently under real production time pressure, exactly the underlying skill "root-cause this scenario" interview questions are designed to probe, and genuine daily practice in field/lab RF debugging and protocol-analysis roles across this curriculum's career map.

### 14. Interview question

*"Describe your general methodology for debugging an unfamiliar system when a symptom could plausibly stem from many different subsystems."* Answer directly from §4: enumerate the plausible hypothesis space explicitly rather than jumping to one guess; prioritize cheap, already-available diagnostic information before designing new experiments; choose tests that discriminate well among currently-live hypotheses rather than ones addressing only a single unlikely candidate; and always confirm a leading hypothesis with an independent, falsifiable predicted consequence, not mere correlation.

### 15. Active recall

1. Why is "check existing instrumentation" often the correct first move, even over a test that seems more directly related to your prime suspect?
2. What's the difference between a test that's merely *valid* and one that's *well-discriminating*, and why does that distinction matter for efficiency?
3. Why is correlation with one test's outcome insufficient to confirm a hypothesis, per §4 step 6?

### 16. Mini assignment

Work through §11's practice scenario in full — your hypothesis list, your ranked test choices with reasoning, and your predicted discriminating outcome for your top test — and send it over.

### 17. Exit test

1. State the six-step methodology loop from §4, from memory.
2. True or false, with a one-line reason: "The best first diagnostic test is always the one that checks your most likely hypothesis."
3. Your hypothesis list and first proposed test from §11, if you haven't sent it yet.

---

**Next:** Lesson 35 continues Stage 12 with **hardware-in-the-loop testing and test vector generation** — turning this lesson's methodology into repeatable, automated regression tests instead of one-off manual investigations.

---

[← Lesson 33](lesson-33-nr-sdap-rrc-inactive.md) · [Table of Contents](../README.md) · [Lesson 35 →](lesson-35-hil-testing-test-vectors.md)
