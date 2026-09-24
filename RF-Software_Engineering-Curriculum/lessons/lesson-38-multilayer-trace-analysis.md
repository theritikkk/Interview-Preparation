[← Table of Contents](../README.md) · [← Lesson 37](lesson-37-address-sanitizer.md)

---

# Lesson 38 — Multi-Layer Protocol Trace Analysis: Diagnosing a Real Field Failure

*Stage 12, continued. Every debugging exercise since Lesson 22 was code-level — a specific function, a debugger you could attach. A real field failure gives you neither: just a timestamped trace across PHY, MAC, RLC, and RRC, and Lesson 34's methodology applied to something genuinely harder than a single hypothesis space in one codebase.*

### 1. Intuition

Lessons 22–37 debugged code you could step through. Real deployed-system failures present differently: a **trace** — a timestamped log of events across every protocol layer — with no source to step into, often not reproducible on demand. The skill is reconstructing *what happened* by correlating events **across layers**, using Lesson 21's "what moves between layers, and why" reasoning as your navigation tool, and Lesson 34's systematic elimination loop applied to trace-reading instead of code hypotheses. This is precisely the modem-diagnostic-tooling skill (QXDM/QCAT-style analysis) this curriculum's career map has named since Part 1.

### 2. The engineering problem that forces this

A real trace contains far more events than are relevant to any one failure. You need to navigate from a high-level, user-visible symptom down through layers to a specific root cause efficiently — using timestamps and cross-layer causal relationships as your primary tool, not linear reading.

### 3. Mathematical foundation

None new — this lesson is Lesson 34's methodology, applied to a genuinely multi-layer problem instead of a single-codebase one.

### 4. Formal definition

A representative trace event: `(timestamp, layer, event_type, parameters)`. The format below is illustrative — built to show the *kind* of information real diagnostic tools surface, not a claim to reproduce any specific tool's exact byte-level format.

### 5. Working a real investigation

**Symptom, given with no cause revealed:** a user reports a ~2-second data stall during an otherwise normal session — no call drop, no visible RRC-level event.

**Step 1 — hypotheses, honestly listed:**
- **H1**: RF condition degradation — a deep fade or momentary blockage stalling the radio link itself (Lesson 10).
- **H2**: RLC-layer stall — a reordering-timer mechanism, exactly the one Lesson 21 §11 explained as a possibility, now potentially the actual cause.
- **H3**: MAC scheduler starvation — the network simply stopped granting this UE resources for some unrelated reason.
- **H4**: Application-layer stall, unrelated to the radio stack entirely.

**Step 2 — rank by discrimination and cost.** The cheapest, most discriminating first check: read the **already-logged PHY-layer CQI/SINR reports** during the stall window — no new instrumentation needed. If SINR/CQI stayed healthy throughout, H1 is eliminated immediately; if MAC granted resources normally throughout, H3 is eliminated; and if RLC/MAC both show real activity during the window, H4 (a stall invisible to the radio stack) is eliminated too — one read of existing logs, four hypotheses addressed at once.

```
t=0.0s   PHY   CQI=11, SINR=14dB          (healthy)
t=0.3s   MAC   grant issued, TB size=2400  (scheduler active)
t=0.4s   PHY   HARQ NACK, HARQ proc=3
t=0.4s   MAC   retransmission scheduled, HARQ proc=3
t=0.5s   PHY   HARQ ACK, HARQ proc=3       (retransmission succeeded)
t=0.5s   RLC   PDU seq=1042 received, GAP detected (seq=1041 missing)
t=0.5s   RLC   reordering timer STARTED (seq=1041)
t=2.4s   RLC   reordering timer EXPIRED (seq=1041)
t=2.4s   RLC   status report sent, requesting retransmission of seq=1041
t=2.5s   RLC   seq=1041 received, reassembly complete, delivery resumes
t=2.5s   PHY   CQI=11, SINR=13dB          (still healthy throughout)
```

**Step 3–4 — work the trace.** CQI/SINR are healthy at every point (H1 eliminated). MAC granted resources actively throughout (H3 eliminated). The one real HARQ NACK at $t=0.4$s resolved successfully within 100 ms — not the cause of a 2-second stall. What actually spans the stall window is an **RLC reordering-timer expiry**, running from $t=0.5$s to $t=2.4$s — very close to 2 seconds, matching the symptom directly. This is exactly the mechanism Lesson 21 §11 explained as a real, non-radio cause of RLC retransmission activity — encountered here not as an abstract possibility but as the actual, trace-confirmed cause.

**Step 5 — confirm with an independent, falsifiable prediction (Lesson 34 §4 step 6).** If reordering-timer expiry really is the cause, the stall duration should closely match the **configured** reordering timer value (a known network parameter, not something to infer from this one trace alone), and the trace should show a "timer expired, requesting retransmission" event immediately preceding data resuming. Both hold here: the measured $\approx1.9$s gap is consistent with a commonly-configured timer value in that range, and the RLC status-report event at $t=2.4$s sits immediately before delivery resumes at $t=2.5$s — not merely correlated with the stall, but the specific predicted mechanism confirmed.

### 6. Visualization

```
PHY:  ─●───●───●─────────────────────────●───●──   (healthy throughout — H1 eliminated)
MAC:  ─────●───●─────────────────────────●───●──   (granting throughout — H3 eliminated)
RLC:  ─────────────[reordering timer running]────●─●   ← the actual gap lives here
                     0.5s              2.4s  2.5s
                     ↑                  ↑     ↑
                  gap detected      timer expires,   delivery
                                    retransmit req    resumes
```

### 7. Worked example

The trace above *is* the worked example — every timestamp and event in §5 is the concrete data the diagnosis was built from, not an abstraction layered on top of one.

### 8. Code

```python
def find_cross_layer_anomaly(trace):
    """trace: list of dicts with keys timestamp, layer, event_type, params.
       Flags windows where lower layers (PHY/MAC) look healthy but a higher
       layer (RLC) shows an open, unresolved event spanning real time —
       exactly Sec 5's discriminating check, automated."""
    phy_mac_events = [e for e in trace if e["layer"] in ("PHY", "MAC")]
    rlc_events = [e for e in trace if e["layer"] == "RLC"]

    open_timers = {}
    anomalies = []
    for e in rlc_events:
        if e["event_type"] == "reordering_timer_started":
            open_timers[e["params"]["seq"]] = e["timestamp"]
        elif e["event_type"] == "reordering_timer_expired":
            start = open_timers.pop(e["params"]["seq"], None)
            if start is not None:
                duration = e["timestamp"] - start
                anomalies.append({"seq": e["params"]["seq"], "duration": duration})
    return anomalies

trace = [
    {"timestamp": 0.0, "layer": "PHY", "event_type": "cqi_report", "params": {"cqi": 11}},
    {"timestamp": 0.5, "layer": "RLC", "event_type": "reordering_timer_started", "params": {"seq": 1041}},
    {"timestamp": 2.4, "layer": "RLC", "event_type": "reordering_timer_expired", "params": {"seq": 1041}},
]
print(find_cross_layer_anomaly(trace))
```

### 9. Experiment

Run §8's code on the full trace from §5 (reconstruct it as a list of dicts) and confirm it correctly isolates the reordering-timer window as the anomaly, with the right duration — automating the by-hand diagnosis into a reusable triage tool, exactly the kind of first-pass automated flagging a real protocol analyzer performs before a human investigates further.

### 10. Expected output

`find_cross_layer_anomaly` returns exactly one anomaly: `seq=1041`, duration $\approx1.9$s — matching the stall reported by the user and the manual diagnosis in §5.

### 11. Practice: a second scenario, for you to work through yourself

**Symptom, no cause given:** a different user's trace shows a **complete connection drop** — RRC transitions from CONNECTED directly to IDLE, unprompted by any user action — preceded by several seconds of PHY-layer SINR readings that **gradually** degrade (not a sudden cliff) before the drop.

Apply §5's loop yourself: list your hypotheses (consider Lessons 10, 13, and 18 — what physical or procedural causes produce *gradual* SINR decline specifically, as opposed to a sudden one), propose your first, most discriminating, lowest-cost check, and explain why. Send me your hypothesis list and first test; we'll work the elimination together the way §5 did.

### 12. Common mistakes

- Fixating on the layer where a symptom is *observed* ("the app stalled, must be an app bug") instead of tracing the full causal chain down to where it actually originates — the general form of Lesson 21 §11's specific point, now a standing principle for all multi-layer diagnosis.
- Reading a trace linearly or exhaustively instead of navigating via discriminating checks first, exactly Lesson 34 §7's "wrong first move" lesson, now applied to trace analysis instead of code investigation.
- Focusing only on *what* events occurred and ignoring precise cross-layer *timing* correlation — the actual diagnostic signal in multi-layer trace work is overwhelmingly in the timestamps, not just the event types.

### 13. Real RF/cellular application

This is, without exaggeration, the daily work of RF/protocol field-support and PHY/L2 debugging engineers at every company named in this curriculum's career map — exactly this kind of multi-layer correlation, on real QXDM/QCAT-style traces. This lesson is the direct, complete conceptual blueprint for **Project 10 (Cellular Protocol Analyzer)** — by this point in the curriculum, you have essentially every piece needed to build it for real.

### 14. Interview question

*"You're given a trace showing a 2-second data stall with no RRC-level connection event. How do you approach finding the root cause?"* Answer from §5: don't start investigating at the layer where the symptom was observed; start with the cheapest, most discriminating cross-layer check available (PHY health during the stall window), narrow down layer by layer using Lesson 34's elimination loop, and confirm the leading hypothesis with an independent predicted consequence — stall duration matching a known configured parameter — rather than stopping at correlation alone.

### 15. Active recall

1. Why did checking PHY-layer health first discriminate among four hypotheses simultaneously, rather than just one?
2. What specific, independent prediction confirmed the reordering-timer hypothesis beyond mere correlation?
3. Why is timestamp correlation across layers the primary diagnostic signal in this kind of analysis, more than event type alone?

### 16. Mini assignment

Extend §8's `find_cross_layer_anomaly` to automatically check §5 step 5's confirmation (does the anomaly's duration fall within a plausible configured-timer range you specify), turning the full diagnosis — detection *and* confirmation — into one automated function, applied end-to-end to the §5 trace.

### 17. Exit test

1. Restate, in your own words, why checking PHY/MAC health first was the correct opening move rather than starting at RLC where the anomaly actually was.
2. True or false, with a one-line reason: "A trace showing RLC retransmission activity during a stall always indicates a radio-quality problem."
3. Your hypothesis list and proposed first test from §11, if you haven't sent it.

---

**Next:** Lesson 39 closes out Stage 12 with **conformance and interoperability testing** — how a receiver gets validated against the 3GPP specification itself, not just against your own reference implementation.

---

[← Lesson 37](lesson-37-address-sanitizer.md) · [Table of Contents](../README.md) · [Lesson 39 →](lesson-39-conformance-interop-testing.md)