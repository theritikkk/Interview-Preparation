[← Table of Contents](../README.md) · [← Lesson 20](lesson-20-pdcch-pdsch-harq.md)

---

# Lesson 21 — RLC, PDCP, and RRC: What Moves Between Layers, and Why Each One Exists

*Stage 7 — LTE, continued. This lesson finally answers the question this whole curriculum's roadmap raised at the very start: who talks to whom, what data moves between layers, and why does each layer exist — not as a diagram to memorize, but as a series of specific problems only that layer can solve.*

### 1. Intuition

Lessons 19–20 got a device synchronized and receiving scheduled transmissions. But a raw MAC transport block isn't useful "data" yet — it needs reassembly into the original packet, delivery in the right order, a second line of defense when HARQ ultimately fails, encryption, and connection management that survives moving between cells. RLC, PDCP, and RRC are the layers that turn "the radio successfully moved some bits" into "a reliable, secure, manageable connection" — and the genuinely interesting question isn't "what does each layer do" but "why does this specific problem live at exactly this layer, and not one above or below it."

### 2. The engineering problem that forces this

Four questions that a layer diagram alone doesn't answer: why retransmit at RLC when HARQ (Lesson 20) already retransmits at MAC/PHY? Why segment packets at RLC when MAC already multiplexes data into transport blocks every TTI? Why does ciphering live at PDCP specifically, rather than RLC below it or NAS above it? And why is RRC a separate, radio-specific signaling layer rather than folded into NAS? Each answer turns out to be forced by a genuinely distinct constraint — not architectural taste.

### 3. Mathematical foundation

This lesson's "math" is a set of trade-offs already familiar in shape: **RLC modes** — TM (Transparent, no header, signaling only), UM (Unacknowledged, no ARQ, used for delay-sensitive traffic), AM (Acknowledged, full ARQ) — are reliability-vs-latency operating points, structurally the same trade-off as Lesson 8's modulation choice and Lesson 20's HARQ-attempt-count choice. **SDU/PDU chain**: IP packet → PDCP SDU → (PDCP adds header: ciphering, optional ROHC compression, sequence number) → PDCP PDU = RLC SDU → (RLC segments/concatenates to fit available grant) → RLC PDU = MAC SDU → (MAC multiplexes logical channels into one transport block) → MAC PDU → PHY (Lessons 1–20).

### 4. Formal definition

An **SDU** (Service Data Unit) is what a layer receives from the layer above; a **PDU** (Protocol Data Unit) is what it produces for the layer below, typically its own SDU plus a header. Each layer's PDU becomes the next layer down's SDU — a clean, recursive interface, letting each layer be reasoned about (and implemented, and tested) independently of the ones above and below it.

### 5. Derivation — four "why," answered as forced consequences, not preferences

**(a) Why RLC ARQ, when HARQ (Lesson 20) already retransmits?** HARQ operates per-TTI at the MAC/PHY boundary, on a small fixed number of parallel processes, with a limited retry count, using soft-combining (Lesson 20 §5(a)) — optimized for *fast* recovery of common bit errors, with zero visibility into anything above the raw transport block. But HARQ can still exhaust its retries and fail outright. RLC's Acknowledged Mode is a *slower*, PHY-agnostic safety net, operating on logical RLC-PDU sequence numbers with explicit status reports, positioned specifically to catch what HARQ ultimately can't fix — a second line of defense at a different timescale and abstraction level, not a redundant duplicate.

**(b) Why RLC segmentation, separate from MAC's per-TTI multiplexing?** An IP packet's size is arbitrary and has no relationship to how much grant space MAC scheduling (Lesson 20) happens to allocate *this specific TTI* — which varies dynamically. RLC is precisely the layer that knows both "how large is my SDU" and interfaces directly with the layer that knows "how much room exists this TTI," so it's the only layer positioned to correctly chop a variable-size SDU into whatever pieces fit, and reassemble them later — including when a piece needed RLC-level retransmission. PHY/HARQ below only ever sees one fixed-size transport block per TTI, with no visibility into SDU boundaries at all.

**(c) Why does PDCP hold ciphering, specifically?** Security needs to survive **handover** — when a UE moves to a new cell, RLC state is typically re-established from scratch (sequence numbers reset), while PDCP specifically preserves continuity across that handover with its own independent sequence numbering, forwarding any data lost in the RLC-layer transition from old cell to new. PDCP sits at exactly the point in the stack close enough to the radio to protect the actual over-the-air transmission, yet high enough to survive a handover between physically different cells. Put ciphering below PDCP (at RLC), and every handover would force a full security-context re-establishment; put it above PDCP (at NAS), and the radio link itself would carry unprotected data over the air. Neither alternative works; PDCP's position is forced, not chosen for convenience.

**(d) Why is RRC a distinct layer, not folded into NAS?** RRC manages radio-specific state: cell selection, connection establishment, measurement reporting for handover, security-key derivation triggers — all inherently tied to *which cell and which radio technology* the UE currently uses. NAS, sitting above RRC, is deliberately built to know *nothing* about that — it manages mobility and session context at the core-network level, specifically so a single NAS session can survive a UE moving between completely different radio technologies (e.g., an LTE-to-NR handover). That clean ignorance is only possible because RRC absorbs every radio-specific detail beneath it.

### 6. Visualization

```
IP packet
   ↓
[PDCP] ← ciphering, header compression, seq num — survives handover (§5c)
   ↓ (PDCP PDU = RLC SDU)
[RLC]  ← segmentation to fit TTI grant, AM/UM/TM ARQ — safety net beyond HARQ (§5a,b)
   ↓ (RLC PDU = MAC SDU)
[MAC]  ← multiplexes logical channels into one transport block per TTI, HARQ (Lesson 20)
   ↓ (MAC PDU = PHY transport block)
[PHY]  ← Lessons 1-20: coding, modulation, resource mapping, the actual radio waveform
```

### 7. Worked example

A 1500-byte IP packet arrives, but this TTI's MAC grant only has room for 400 bytes of RLC data. PDCP adds its header (ciphering + sequence number) to the packet. RLC segments the resulting PDCP PDU into three pieces to fit three successive TTIs' grants (400+400+~700, say), each becoming its own RLC PDU with a segment header. MAC multiplexes each RLC PDU (possibly alongside other logical channels' data) into that TTI's transport block, which PHY then encodes and transmits (everything from Lessons 1–20). The receiver reverses this exactly: PHY decodes each transport block, MAC demultiplexes, RLC reassembles the three segments back into one PDCP PDU (requesting retransmission via ARQ if one segment's HARQ ultimately failed), and PDCP deciphers and hands the original 1500-byte packet upward.

### 8. Code

```python
# A minimal simulation of RLC-style segmentation and reassembly across
# TTIs with varying, unpredictable grant sizes -- exactly section 5(b)'s reasoning.

def segment(sdu: bytes, grant_sizes: list[int]):
    segments, offset = [], 0
    for i, size in enumerate(grant_sizes):
        if offset >= len(sdu):
            break
        chunk = sdu[offset:offset+size]
        segments.append({"seq": i, "data": chunk})
        offset += size
    return segments

def reassemble(received_segments: list[dict]) -> bytes:
    ordered = sorted(received_segments, key=lambda s: s["seq"])
    return b"".join(s["data"] for s in ordered)

sdu = b"X" * 1500
grant_sizes = [400, 400, 700]         # varies TTI to TTI, per real scheduling (Lesson 20)
segs = segment(sdu, grant_sizes)

# Simulate one segment arriving late/out of order, then reassemble anyway
received_out_of_order = [segs[2], segs[0], segs[1]]
reassembled = reassemble(received_out_of_order)
print("correctly reassembled:", reassembled == sdu)
```

### 9. Experiment

Run §8's code and confirm reassembly succeeds regardless of arrival order, provided every segment eventually arrives — the whole point of sequence-numbered segmentation. Then simulate a genuinely *missing* segment (drop one from `received_out_of_order`) and observe reassembly failing or producing corrupted output, motivating exactly why an RLC-AM-style retransmission request exists: without it, one lost segment corrupts the entire reassembled SDU, not just its own portion.

### 10. Expected output

With all segments present, regardless of arrival order: perfect reassembly. With one segment missing: reassembly fails or is corrupted — concretely showing why sequence numbering *alone* (just knowing the order) isn't sufficient; something has to also detect and request retransmission of genuinely missing pieces, which is precisely RLC AM's job.

### 11. Debugging exercise

Symptom, no cause given: in a protocol trace (the kind Project 10 will build), you observe RLC-layer retransmissions occurring even though the PHY/MAC HARQ statistics for the same period look completely healthy — low retransmission rate, no obvious radio link problems.

Before concluding the radio link must actually be bad somewhere HARQ isn't catching: what *else*, besides raw bit errors, could trigger an RLC-layer retransmission?

**Root cause (revealed):** RLC retransmissions are not only triggered by radio-link bit errors HARQ missed. A **lost or delayed RLC status report** (a control-plane message, not user data) can trigger a retransmission the underlying data delivery never actually needed. **Reordering-timer expiry** can also fire even with healthy HARQ, since MAC-layer scheduling across multiple HARQ processes can legitimately deliver RLC PDUs slightly out of strict order without any error occurring. The transferable lesson: a layer's retransmission activity doesn't necessarily reflect *that layer's own* link quality — it can be triggered by adjacent-layer or control-plane issues, which is exactly why real diagnostic work correlates *across* layers (the QXDM/QCAT-style tracing flagged repeatedly in this curriculum's career map) before concluding where a problem actually lives.

### 12. Common mistakes

- Assuming RLC AM (full ARQ) is always the "best" mode — UM deliberately skips ARQ for delay-sensitive traffic like VoIP, where a late retransmission is worse than an occasional dropped packet; TM skips headers entirely for specific signaling channels. Choosing the mode is itself a real, deliberate reliability-vs-latency design decision.
- Conflating PDCP's header compression (ROHC) with its ciphering — they're separate, independently configurable functions serving entirely different purposes (overhead reduction vs. security), not two names for the same mechanism.
- Assuming RRC_IDLE means "disconnected" — a UE in RRC_IDLE still holds a NAS-level registration with the core network and can be paged; it's specifically a radio-connection power-saving state, not a logout, a very common naming-driven misconception.

### 13. Real RF/cellular application

This entire layer-boundary discussion is exactly what real protocol conformance and interoperability test tools analyze (this curriculum's career map's test-engineering track, and the Keysight/R&S/Anritsu equipment behind it), and cross-layer trace correlation — matching a PDCP-layer event to the corresponding RLC/MAC/PHY events around it — is precisely the daily diagnostic-tooling skill flagged as a real differentiator in Part 1's research findings. This lesson is the direct conceptual foundation for Project 10 (Cellular Protocol Analyzer).

### 14. Interview question

*"Why does LTE have both HARQ (MAC/PHY) and ARQ (RLC) — isn't that redundant?"* Answer from §5(a): different timescales, different abstraction levels, different failure modes each is specifically positioned to catch — HARQ is fast, PHY-aware, soft-combining, limited-retry; RLC ARQ is a slower, PHY-agnostic safety net catching what HARQ ultimately fails to resolve, operating on logical PDU sequence numbers rather than raw transport blocks.

### 15. Active recall

1. Why is PDCP, specifically, the layer that holds ciphering — not RLC, not NAS?
2. Why does NAS deliberately know nothing about which cell or radio technology a UE is using?
3. Name the three RLC modes and the trade-off each represents.

### 16. Mini assignment

Extend §8's simulation to handle a genuinely missing segment with an RLC-AM-style retransmission request: detect the gap via sequence numbers, "request" (re-insert) the missing segment, and confirm correct reassembly once it arrives — even if it arrives after later segments already did.

### 17. Exit test

1. Trace an IP packet's full name-change down the stack: what is it called as it crosses each layer boundary, from PDCP down to PHY?
2. True or false, with a one-line reason: "RLC segmentation exists because MAC cannot fit large packets into a transport block."
3. Your reasoning from §11 about non-radio causes of RLC retransmission, if you haven't sent it.

---

**Next:** Lesson 22 moves to **Stage 9/10 territory**: taking everything built as Python simulation so far and beginning the C++ implementation of a scoped-down LTE PHY chain — where "I understand this" starts becoming "I can ship this as real-time embedded code."

---

[← Lesson 20](lesson-20-pdcch-pdsch-harq.md) · [Table of Contents](../README.md) · [Lesson 22 →](lesson-22-cpp-memory-layout.md)