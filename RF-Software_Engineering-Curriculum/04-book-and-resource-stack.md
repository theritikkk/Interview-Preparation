[← Table of Contents](README.md)

---

# 4. Book & Resource Stack

Small and curated, as requested — not fifty books.

| Resource | Classification | Use it for | What to actually study | What to skip |
|---|---|---|---|---|
| **Understanding Digital Signal Processing**, Richard G. Lyons | **MUST READ** | Stages 1-2 | Ch. 1-9 deeply, especially Ch. 8 (quadrature/IQ signals) — the clearest plain-English treatment of IQ available | Advanced multirate/Hilbert-transform chapters until you actually need them |
| **PySDR: A Guide to SDR and DSP using Python**, Marc Lichtman (free, online) | **MUST READ** | Stages 1-5, parallel track | Read alongside Lyons; its Python code turns Lyons's math into something you can run immediately | Nothing — it's short and free |
| **Software-Defined Radio for Engineers**, Collins/Getz/Pu/Wyglinski (free from Analog Devices) | **MUST READ** | Stage 5 | The hands-on chapters tying IQ theory to real hardware (PlutoSDR); the OFDM chapter is a good preview of Stage 4/7 | Chapters specific to Analog Devices hardware you don't own, if any |
| **Digital Communications**, John Proakis (& Salehi) | **REFERENCE** | Stage 3 onward | Matched filtering, ML detection, error-probability derivations — use as a reference for "why," not a linear read | Don't try to read cover-to-cover; it's a graduate reference text |
| **Digital Signal Processing**, Proakis & Manolakis | **REFERENCE** | Stages 1-2, as backup | Multirate/polyphase filter design detail when Lyons doesn't go deep enough | Redundant chapters already covered by Lyons |
| **LTE – The UMTS Long Term Evolution: From Theory to Practice**, Sesia, Toufik, Baker | **MUST READ** | Stage 6-7 | PHY and MAC-scheduling/HARQ chapters closely; this is the industry-standard reference, written to map 3GPP structure onto explainable engineering | Deep RRM/mobility-optimization chapters until you need them |
| **5G NR: The Next Generation Wireless Access Technology**, Dahlman, Parkvall, Sköld | **MUST READ** | Stage 8 | PHY chapters, read as "the delta from LTE" — same authorial lineage, same approach | Advanced/optional-feature chapters (e.g. niche IoT profiles) until relevant |
| **Communication Systems Engineering with GNU Radio**, Friedt & Boeglen (2024) | **OPTIONAL** | Stage 5, supplementary | A recent, hands-on second opinion on GNU Radio if SDR4Engineers doesn't fully click | Skip if SDR4Engineers + PySDR already cover what you need |
| **Digital Communications with Emphasis on Data Modems**, Richard W. Middlestead | **ADVANCED** | Stage 9, optional deep-dive | Only once you're implementing your own acquisition/timing-recovery code and want more implementation depth on synchronization than Proakis gives | Don't start here — 824 dense pages, not a first read |
| **The Scientist and Engineer's Guide to DSP**, Steven Smith (free, online) | **OPTIONAL/HISTORICAL** | Anytime | Alternate explanation source if a specific Lyons chapter doesn't click | Superseded by Lyons/PySDR for most purposes; some examples are dated |
| **Linux Kernel Development**, 3rd ed., Robert Love | **REFERENCE** (you already have it) | Stage 10, ongoing | See §2 above for exactly which chapters | Filesystem/networking-stack internals for this track |

---

[← 3. The Roadmap](03-roadmap-overview.md) · [5. 3GPP Specification Reading Curriculum →](05-3gpp-specification-reading-guide.md) · [Table of Contents](README.md)
