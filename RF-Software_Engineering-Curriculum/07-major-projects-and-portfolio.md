[← Table of Contents](README.md)

---

# 7. Major Projects

Twelve projects, progressively harder. We'll build these as we reach the relevant stage — this is the map, not the assignment for today.

1. **Digital Modulation Laboratory** (Python) — BPSK/QPSK/QAM modulator + demodulator + AWGN channel, BER curves matching theory
2. **DSP Signal Analyzer** (C++) — real-time-ish spectrum analyzer/spectrogram from an IQ file or live SDR input
3. **Software Modem** (C++) — full TX/RX chain: bits → pulse shaping → modulation → channel → matched filter → timing/carrier recovery → demod → bits
4. **OFDM Transmitter/Receiver** (C++) — cyclic prefix, FFT/IFFT, pilot-based channel estimation, simple equalizer — the single most cellular-relevant project
5. **MIMO Simulator** (Python/C++) — 2×2 spatial multiplexing, zero-forcing/MMSE receiver, capacity-vs-SNR curves
6. **SDR Spectrum/Protocol Analyzer** (C++/GNU Radio) — live spectrum + basic protocol decode from an RTL-SDR
7. **LTE PHY Learning Implementation** (C++) — scoped subset: PSS/SSS cell search + PBCH decode — deliberately not the whole standard
8. **5G NR PHY Learning Implementation** (C++) — scoped subset: SSB detection + basic PDSCH decode chain
9. **Real-Time SDR Processing Pipeline** (C++, multithreaded) — lock-free ring buffers, deadline-driven processing, CPU-affinity-pinned threads
10. **Cellular Protocol Analyzer** (C++/Python) — parse/visualize a public LTE/5G log or trace format across PHY→MAC→RLC→PDCP→RRC, in the spirit of QXDM/QCAT
11. **High-Performance DSP Engine** (C++/SIMD) — hand-optimize FIR filtering or FFT with AVX2/NEON intrinsics; profile and document the speedup
12. **Hardware-in-the-Loop Wireless Test System** (Python + SDR/RF gear) — automated test harness producing pass/fail reports against spec-like thresholds — mirrors what test/validation engineers at Keysight/R&S/Viavi actually build

### Flagship portfolio (pick these 5)

| Project | What an engineer would learn about you |
|---|---|
| **#4 OFDM TX/RX** | You understand the exact mechanism (multipath → ISI → cyclic prefix → FFT) that every cellular PHY is built on, not just the buzzword |
| **#7 or #8 (LTE or NR PHY subset — pick one, do it well)** | You can turn a 3GPP procedure into working, correct code — the single highest-signal project on this list |
| **#9 Real-time pipeline** | You understand deterministic latency and concurrency under load, not just "the algorithm is correct in a script" |
| **#11 SIMD DSP engine** | You can profile, optimize, and *prove* a speedup — the performance-engineering skill that separates strong candidates from average ones |
| **#10 Protocol analyzer** | You understand the system as a whole, across layers — and you've built the exact kind of diagnostic tooling literacy that shows up repeatedly in real job postings |

No project here guarantees an interview or an offer — think of this set as evidence a hiring manager can actually evaluate, not a checklist that unlocks a job.

---

[← 6. SDR Laboratory Progression](06-sdr-laboratory-progression.md) · [8. Source-Code Reading Targets →](08-source-code-reading-guide.md) · [Table of Contents](README.md)
