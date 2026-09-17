[← Table of Contents](README.md)

---

# 1. Career Map (reference)

### Role families and how they actually differ

| Role family | What they really do | Typical code | Math depth |
|---|---|---|---|
| **RF/PHY Firmware Engineer (L1)** | Implement PHY algorithms (sync, channel estimation, equalization, FEC) as embedded C/C++ on DSP + ARM cores, against real-time deadlines | C, some assembly, DSP intrinsics | Medium-high, but implementation-focused |
| **Baseband/Modem Software Developer** | Build and maintain the L1/L2 baseband software stack — the "systems engineering" of the modem | C/C++, real-time embedded | Medium |
| **Modem Systems / Algorithms Engineer** | Design and simulate the algorithms before they're implemented (channel coding, MIMO detection, synchronization) | C/C++/MATLAB simulation, less production firmware | High |
| **DSP R&D / ASIC-adjacent Engineer** | Bit-exact modeling of DSP algorithms for silicon, RTL vector matching, ASIC lab debug | C/C++, MATLAB/Simulink | Very high; often PhD-heavy |
| **Protocol Stack Engineer (L2/L3)** | RRC/NAS/RLC/PDCP/MAC state machines, call flows, signaling | C/C++, sometimes Java; less DSP | Low-medium; more state-machine/systems thinking |
| **Wireless/RF Test & Validation Engineer** | Automate conformance/performance testing against real hardware and lab equipment (Keysight, R&S, Anritsu) | Python-heavy automation | Low-medium; needs to *understand* the measurements, not derive them |
| **Embedded Wireless / RF Software Engineer (general)** | Broad embedded role touching modem, drivers, platform software | C/C++, Linux | Medium |
| **RAN Software Engineer (network side)** | Same PHY/MAC/RLC work but on the gNB/eNB (base station / DU) side instead of the UE side | C/C++, often ORAN-adjacent (DPDK, SmartNICs) | Medium-high |

These overlap heavily — a single posting often blends two or three of these. Qualcomm's senior systems-engineering postings, for example, want strong 5G NR PHY/MAC/RLC/PDCP protocol knowledge combined with RF/MIMO understanding, communication theory, and the ability to write simulation code across modem subsystems and layers — i.e., the "systems engineer" role sits deliberately between algorithms and implementation. By contrast, Ericsson's baseband developer postings are framed much more narrowly around developing and maintaining high-performance 5G L1/L2 baseband software in C/C++ — a more purely-implementation role. Apple's modem test/validation roles are explicitly about defining and automating test scenarios for 4G/5G modems in Python, with AI/ML increasingly used to improve testing efficiency — a genuinely different day-to-day from firmware development, even though the job title says "cellular."

### Skill clusters by seniority (grounded in current postings)

| Cluster | Entry level (0-2 yrs) | Mid (2-5 yrs) | Senior/Staff (5-10+ yrs) |
|---|---|---|---|
| **Programming** | Strong C/C++, real-time embedded software experience in Linux environments | Same + emphasis on real-time, multi-threaded software; multi-core, low-latency embedded Linux | Architecture-level C/C++ across ARM + DSP + accelerators; sometimes assembly |
| **DSP/Comms theory** | Knowledge of 4G or 5G physical layer expected but not deep mastery; Python/MATLAB "an advantage" | Wireless communication theory — quadrature modulation, channel coding, OFDMA — and DSP programming: FFTs, filters, complex-number arithmetic | Deep cellular signal-processing experience: manipulating IQ samples, mod/demod, channel coding/error correction, synchronization, filtering, channel estimation, waveform design, with strong communication-theory math |
| **Standards/3GPP** | Awareness expected, not mastery | Strong understanding of 3GPP radio access standardization for LTE and NR | Strong knowledge of cellular PHY/MAC/RLC/PDCP and upper layers, familiarity with 3GPP standards across generations |
| **Linux/embedded/RTOS** | Basic real-time embedded Linux exposure | Solid understanding of Linux, including performance/data-path optimization (e.g. DPDK, SmartNICs) | Deep understanding of modem subsystem architecture — ARM cores, DSP, hardware accelerators |
| **Testing/tools** | Exposure to lab/integration environments | Test-plan development, node-level testing | Owns test methodology, works across hardware/software/RF boundary |
| **Education** | BS typically sufficient | BS + experience, MS common | MS/PhD increasingly preferred at senior/staff levels, especially for algorithm-heavy roles |

**Company-specific notes (not a ranking, just texture):**
- Broadcom's DSP roles skew ASIC/RTL-adjacent and are frequently PhD-gated — that's a different, more research-flavored track than "software engineer."
- Apple splits fairly cleanly into implementation roles (PHY/firmware, 10+ years common at senior levels) and test/validation roles (much more approachable entry point, Python-heavy).
- Defense/SIGINT-adjacent RF software roles (the "RF Software Engineer, DSP/C++/Linux" postings tied to security clearances) pay notably well and want the same core skills — DSP algorithm development, C++, Linux, Python — but require a US security clearance, which is a separate, non-technical gate.
- A growing number of postings (Qualcomm, and modem-adjacent tooling roles) explicitly expect fluency with modem-specific diagnostic tooling — DIAG, QMI, QXDM/QCAT — and the ability to map failures across NAS, RRC, and L1/L2. This diagnostic-tooling literacy is rarely taught in courses and is a genuine differentiator; the "cellular protocol analyzer" project later in this document targets exactly this skill.

### The AI/automation angle (grounded)

This is not a hype question — it shows up directly in current postings. One current Qualcomm modem-software posting is explicitly framed around an "AI-native" development paradigm: engineers are expected to use code-generation agents, LLM-based code review, and automated log/crash-triage tooling to accelerate delivery — while still being responsible for designing and optimizing real-time embedded C/C++ across DSP and ARM cores, and for root-causing complex multi-layer issues on target hardware. Read carefully, that posting is a good preview of where the field is heading: AI tools are becoming normal *inputs* to the workflow (boilerplate, log triage, test generation, first-draft code review), but the thing being hired for — the ability to reason about real-time deadlines, hardware constraints, and multi-layer root causes on physical silicon — is unchanged.

The pattern that emerges across every posting: work becomes **less** automatable as it gets closer to hardware, real-time deadlines, standards compliance, and physical measurement, and **more** assistable the further it is from those (boilerplate code, test-case generation, log summarization, documentation, first-pass simulation scripting). Skills that get *more* valuable as AI-assisted coding spreads: the ability to validate that generated code actually meets real-time/deterministic constraints, to read a spec and know an implementation is wrong even when it compiles and passes a shallow test, and to debug from physical symptoms (constellation plots, spectrum, timing) rather than from stack traces alone. This document treats AI as a tool you'll use constantly (documentation search, boilerplate, test generation, debugging hypotheses) while the actual curriculum is designed so you can derive, implement, and debug all of this independently — see §11 in the roadmap sections below for how that boundary is drawn stage by stage.

---

[2. Linux, specifically for this track (mapped to *Linux Kernel Development*, 3rd ed.) →](02-linux-for-rf-software.md) · [Table of Contents](README.md)
