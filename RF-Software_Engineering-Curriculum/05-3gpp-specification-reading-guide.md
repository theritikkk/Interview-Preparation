[← Table of Contents](README.md)

---

# 5. 3GPP Specification Reading Curriculum

You will never read 3GPP specs linearly — nobody does. Engineers use them as a lookup reference while implementing or debugging a specific feature.

**How 3GPP is organized:** Specs are grouped into Release "trains" (e.g., Release 15 = first 5G NR release) and numbered by series. The ones that matter for this track:
- **36-series** = LTE (36.211 physical channels, 36.212 coding, 36.213 procedures, 36.321 MAC, 36.322 RLC, 36.323 PDCP, 36.331 RRC)
- **38-series** = 5G NR (38.211 physical channels/modulation, 38.212 coding, 38.213 control procedures, 38.214 data procedures, 38.321 MAC, 38.331 RRC)
- **24-series** = NAS (24.301 for LTE, 24.501 for 5G)
- **45/44-series** = GSM/GPRS (only skim for historical context)
- **25-series** = UMTS/WCDMA (only skim for historical context)

**How to actually use them, as a habit you'll build during Stages 7-11:**
1. Start from a *procedure*, not a document. "How does initial cell search work?" points you to 38.211 (signal structure of SSB) + 38.213 (procedures) — not "read 38.211 front to back."
2. Use the table of contents as an index, not a reading order — the spec structure itself tells you where a field/procedure lives.
3. When debugging, specs answer "what should this field/value be," logs/traces answer "what did it actually do" — you triangulate between the two, which is exactly what the modem-diagnostic-tooling skill (DIAG/QMI/QXDM-style, see §1) is for.
4. Cross-reference specs with a real open-source implementation (srsRAN, OAI) — reading how a spec paragraph became 200 lines of C++ is often faster than parsing the prose alone.
5. Never try to "finish" a spec. Track *features* you've traced end-to-end, not pages read.

This becomes concrete once we reach Stage 7; for now, just know the map exists.

---

[← 4. Book & Resource Stack](04-book-and-resource-stack.md) · [6. SDR Laboratory Progression →](06-sdr-laboratory-progression.md) · [Table of Contents](README.md)
