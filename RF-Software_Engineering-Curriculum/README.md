# RF / Cellular Modem Software Engineering — Master Curriculum

A living roadmap from backend/systems engineering to job-readiness for **PHY, baseband, modem, and protocol-stack software** roles (Qualcomm, MediaTek, Samsung, Apple, Nokia, Ericsson, Broadcom, Keysight, Rohde & Schwarz, and similar). Built from a real software background and from current job postings researched in September 2026. This is meant to be revisited and edited as you progress — not read once and forgotten.

**Explicitly out of scope:** analog RF IC design. **In scope:** the software/DSP/embedded layer that sits between "IQ samples" and "a working cellular link."

**How to use this book:** the numbered chapters (1–16) are reference material — read them once for orientation, then return to them as needed. The `lessons/` folder is where the actual teaching happens, one topic at a time, each following a fixed 17-part format (intuition → math → derivation → code → debugging exercise → interview question → exit test). New lesson files get added here as you progress — this folder is meant to grow.

---

## Table of Contents

### Reference chapters
1. [Career Map](01-career-map.md) — role families, skill clusters by seniority, company-specific notes, the AI/automation angle
2. [Linux for RF Software](02-linux-for-rf-software.md) — exactly which kernel concepts matter, mapped to *Linux Kernel Development* (Robert Love)
3. [Roadmap Overview](03-roadmap-overview.md) — the 14-stage curriculum, with a diagram and a stage-by-stage table
4. [Book & Resource Stack](04-book-and-resource-stack.md) — a small, curated, classified book list
5. [3GPP Specification Reading Guide](05-3gpp-specification-reading-guide.md) — how engineers actually use the standards
6. [SDR Laboratory Progression](06-sdr-laboratory-progression.md) — from pure simulation to hardware-in-the-loop, with hardware/legal guidance
7. [Major Projects & Portfolio](07-major-projects-and-portfolio.md) — 12 projects, 5 flagged as the flagship portfolio
8. [Source-Code Reading Guide](08-source-code-reading-guide.md) — how to navigate srsRAN, OpenAirInterface, GNU Radio, Osmocom
9. [Debugging Training Framework](09-debugging-training-framework.md) — the symptom-first method used throughout the lessons
10. [Interview Preparation Framework](10-interview-preparation-framework.md) — how to reason through questions, not memorize answers
11. [AI & Automation](11-ai-and-automation.md) — durable reference version of the automation-resistance analysis
12. [Job-Description Decoder](12-job-description-decoder.md) — how to tell what a posting actually is, plus search keywords
13. [12–18 Month Plan](13-twelve-to-eighteen-month-plan.md) — month-by-month execution plan
14. [Weekly Study System](14-weekly-study-system.md) — the day-by-day rotation
15. [Progress Checklist](15-progress-checklist.md) — stage-by-stage checkboxes
16. [Glossary](16-glossary.md) — core terms, grows as lessons introduce new ones

### Lessons (taught one at a time, in order)
- [Lesson 1 — Complex Numbers, Phasors, and Why Radio Needs Two Dimensions](lessons/lesson-01-complex-numbers-and-phasors.md) *(Stage 0)*
- [Lesson 2 — The Sampling Theorem: Nyquist, Aliasing, and Why Complex Baseband Is Cheaper](lessons/lesson-02-sampling-theorem-and-aliasing.md) *(Stage 1)*
- [Lesson 3 — The DTFT, the DFT, and the FFT](lessons/lesson-03-dtft-dft-fft.md) *(Stage 1)*
- [Lesson 4 — Convolution, Impulse Response, and Your First FIR Filter](lessons/lesson-04-convolution-fir-filters.md) *(Stage 1→2)*
- [Lesson 5 — Multirate DSP: Decimation, Interpolation, and Why You Always Filter First](lessons/lesson-05-multirate-dsp.md) *(Stage 2)*
- [Lesson 6 — Correlation and the Matched Filter](lessons/lesson-06-correlation-matched-filter.md) *(Stage 2)*
- [Lesson 7 — AWGN, Eb/N0, and Your First BER Curve (BPSK)](lessons/lesson-07-awgn-bpsk-ber.md) *(Stage 3)*
- [Lesson 8 — QPSK and M-ary QAM: The Spectral Efficiency Trade-off](lessons/lesson-08-qpsk-mqam.md) *(Stage 3)*
- [Lesson 9 — Pulse Shaping: Root-Raised-Cosine Filtering and the Nyquist ISI Criterion](lessons/lesson-09-pulse-shaping-rrc.md) *(Stage 3)*
- [Lesson 10 — Multipath and Fading: The Real Wireless Channel](lessons/lesson-10-multipath-fading.md) *(Stage 4)*
- [Lesson 11 — Channel Estimation and Equalization](lessons/lesson-11-channel-estimation-equalization.md) *(Stage 4)*
- [Lesson 12 — OFDM: The Complete Transmitter and Receiver](lessons/lesson-12-ofdm-transceiver.md) *(Stage 4)*
- [Lesson 13 — Timing Offset and Carrier Frequency Offset (CFO)](lessons/lesson-13-timing-cfo.md) *(Stage 4)*
- [Lesson 14 — MIMO: Spatial Multiplexing and Diversity](lessons/lesson-14-mimo.md) *(Stage 4)*
- [Lesson 15 — Real IQ Data: DC Offset, IQ Imbalance, and the Jump from Simulation to Hardware](lessons/lesson-15-real-iq-dc-offset-iq-imbalance.md) *(Stage 5)*
- [Lesson 16 — GNU Radio: Flowgraphs, the Scheduler, and Decoding Real FM](lessons/lesson-16-gnu-radio-fm-demod.md) *(Stage 5)*
- [Lesson 17 — Recorded IQ Datasets: File Formats, SigMF, and Decoding ADS-B](lessons/lesson-17-recorded-iq-adsb.md) *(Stage 5)*
- [Lesson 18 — Cellular Fundamentals: Cells, Duplexing, and Why OFDMA Won](lessons/lesson-18-cellular-fundamentals.md) *(Stage 6)*
- [Lesson 19 — LTE: The Resource Grid, PSS/SSS Cell Search, and PBCH](lessons/lesson-19-lte-cell-search.md) *(Stage 7)*
- [Lesson 20 — PDCCH, PDSCH, and HARQ](lessons/lesson-20-pdcch-pdsch-harq.md) *(Stage 7)*
- [Lesson 21 — RLC, PDCP, and RRC: What Moves Between Layers, and Why Each One Exists](lessons/lesson-21-rlc-pdcp-rrc.md) *(Stage 7)*
- [Lesson 22 — From Python to C++: Memory Layout, std::complex, and Why "Compiled" Doesn't Automatically Mean "Fast"](lessons/lesson-22-cpp-memory-layout.md) *(Stage 9/10)*
- [Lesson 23 — A Real-Time FIR Filter in C++: Ring Buffers and Why the Hot Path Must Never Allocate](lessons/lesson-23-cpp-fir-ring-buffer.md) *(Stage 9/10)*
- [Lesson 24 — Multithreading and Lock-Free Structures: Passing Samples Between Threads Safely](lessons/lesson-24-multithreading-lockfree.md) *(Stage 9/10)*
- [Lesson 25 — Fixed-Point Arithmetic: Q-Format, Overflow, and the 6.02B+1.76 dB Rule](lessons/lesson-25-fixed-point-arithmetic.md) *(Stage 9/10)*
- [Lesson 26 — SIMD Intrinsics: Hand-Vectorizing a DSP Kernel with AVX2](lessons/lesson-26-simd-avx2.md) *(Stage 9/10)*
- [Lesson 27 — Profiling: Measuring Where Time Actually Goes, Instead of Guessing](lessons/lesson-27-profiling-amdahl.md) *(Stage 9/10)*
- [Lesson 28 — 5G NR: Numerologies, the Flexible Resource Grid, and SSB](lessons/lesson-28-nr-numerology-ssb.md) *(Stage 8)*
- [Lesson 29 — 5G NR: DMRS Channel Estimation and PDCCH's CORESET](lessons/lesson-29-nr-dmrs-coreset.md) *(Stage 8)*
- [Lesson 30 — PRACH: Random Access, and Zadoff-Chu Reused for Timing Advance](lessons/lesson-30-prach-timing-advance.md) *(Stage 8)*
- [Lesson 31 — Building the Pipeline: A Scoped LTE PSS/SSS/PBCH Receiver in C++](lessons/lesson-31-lte-pss-sss-pbch-pipeline.md) *(Stage 9, Project 7)*
- [Lesson 32 — Generalizing to NR SSB: Where the Pipeline's Contracts Hold, and Where a Hidden Assumption Breaks](lessons/lesson-32-nr-ssb-msequence.md) *(Stage 9, Project 8)*
- [Lesson 33 — NR's Protocol Stack: SDAP and RRC_INACTIVE](lessons/lesson-33-nr-sdap-rrc-inactive.md) *(Stage 11)*
- [Lesson 34 — A Real Debugging Investigation: Systematic Root-Cause Elimination](lessons/lesson-34-debugging-methodology.md) *(Stage 12)*
- [Lesson 35 — Hardware-in-the-Loop Testing and Test Vectors](lessons/lesson-35-hil-testing-test-vectors.md) *(Stage 12)*
- [Lesson 36 — Latency and Throughput: Measuring Real-Time Correctness](lessons/lesson-36-latency-throughput.md) *(Stage 12)*
- [Lesson 37 — Memory-Safety Tooling: AddressSanitizer](lessons/lesson-37-address-sanitizer.md) *(Stage 12)*
- [Lesson 38 — Multi-Layer Protocol Trace Analysis: Diagnosing a Real Field Failure](lessons/lesson-38-multilayer-trace-analysis.md) *(Stage 12, Project 10)*
- [Lesson 39 — Conformance and Interoperability Testing](lessons/lesson-39-conformance-interop-testing.md) *(Stage 12)*
- [Lesson 40 — Interview Synthesis: Building a Real Question Bank from Lessons 1–39](lessons/lesson-40-interview-synthesis.md) *(Stage 15)*
- [Lesson 41 — Convolutional Codes and Viterbi Decoding](lessons/lesson-41-convolutional-viterbi.md) *(Stage 3)*

---

*Start at [Chapter 1](01-career-map.md) for orientation, or jump straight to [Lesson 1](lessons/lesson-01-complex-numbers-and-phasors.md) if you already know why you're here.*