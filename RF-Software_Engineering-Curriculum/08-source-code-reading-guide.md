[← Table of Contents](README.md)

---

# 8. Source-Code Reading Targets

- **srsRAN** — full open-source 4G/5G RAN + UE stack in C++; the single most relevant codebase for this track
- **OpenAirInterface (OAI)** — similar scope, heavily used in O-RAN/research contexts
- **GNU Radio** — the DSP framework itself; understand its scheduler/buffer model, not just its blocks
- **Osmocom** — GSM/UMTS-focused; useful for 2G/3G internals and SDR-based GSM experiments specifically

**How to navigate any of these (the method, not a "read the whole repo" instruction):**
1. Find the top-level architecture doc/README first.
2. Find the "main loop" — the real-time thread that ties DSP ↔ scheduler ↔ protocol stack together.
3. Pick **one** procedure (e.g., PRACH detection, or a single uplink grant) and trace it start to finish across layers using `grep`/`ctags`, not linear reading.
4. Identify the threading model and where the locks are.
5. Identify one hot path and how it's been optimized.

We'll do this hands-on once you're in Stage 7-9, using srsRAN as the primary reference.

---

[← 7. Major Projects](07-major-projects-and-portfolio.md) · [9. Debugging Training — the method →](09-debugging-training-framework.md) · [Table of Contents](README.md)
