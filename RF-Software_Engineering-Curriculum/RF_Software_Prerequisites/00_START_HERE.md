# RF / CELLULAR MODEM SOFTWARE ENGINEERING — PREREQUISITE NOTES
**"What you need to know *before* Lesson 1."** 

*A standalone set of notes, deliberately kept separate from the college Wireless Communication unit-wise notes. It supports the uploaded curriculum* `RF-Software_Engineering-Curriculum` *(48 lessons, 16 reference chapters, 14 stages).*

---

## 0.1 What this folder is — and is not

* **Is:** the layer *underneath* the curriculum. The curriculum's own Stage 0 is described as a **"refresher only — no dedicated book; we'll derive what's needed as we go"** (roadmap, Stage 0), and its lessons use decibels, linear algebra, probability, NumPy and C++ *without re-teaching them*. These notes teach those from first principles so that Lesson 1 onward feels like learning radio, not like catching up on school maths and tooling.
* **Is not:** a replacement for the curriculum. **Complex numbers and phasors are taught *inside* Lesson 1**, and the Fourier transform, sampling, OFDM, LTE/NR etc. are all taught in the lessons. I do not duplicate them; I only prepare you for them. Where a topic is taught in a lesson, the module says so.
* **Not tied to your college exam notes**, though P12 will show which parts of those notes already count as "done" for this curriculum.

**Source discipline.** There is no uploaded textbook for these basics, so the modules are my own explanations. Each module states *which lessons depend on it*, based on a keyword scan of all 48 lesson files plus the lesson titles (so treat "used in Lesson N" as "that lesson relies on it", verified by scan, not an exhaustive claim). Every numerical example and code snippet was executed and checked.

---

## 0.2 The module list and status

| # | Module | What it gives you | Status |
|---|---|---|---|
| **P01** | [Units, Logs and Decibels](P01_Units_Logs_and_Decibels.md) | SI prefixes, logarithms, **dB / dBm**, noise-floor numbers | ✅ written |
| **P02** | [Trigonometry and Sinusoids](P02_Trigonometry_and_Sinusoids.md) | radians, unit circle, amplitude/frequency/phase, mixing, I/Q idea | ✅ written |
| **P03** | [Algebra, Series and Calculus Essentials](P03_Algebra_Series_and_Calculus.md) | Σ notation, geometric series, derivatives, integrals, Taylor series, orthogonality | ✅ written |
| **P04** | [Linear Algebra](P04_Linear_Algebra.md) | vectors, dot products, matrices, rank, condition number, SVD intro, least squares | ✅ written |
| **P05** | [Probability and Statistics for Noise](P05_Probability_and_Statistics_for_Noise.md) | random variables, Gaussian, Q-function, N₀/2 rule, Monte-Carlo, confidence intervals | ✅ written |
| **P06** | [Signals Primer and Fourier Series](P06_Signals_Primer_and_Fourier_Series.md) | signals & systems vocabulary, LTI idea, Fourier series, spectrum, bandwidth, time–frequency duality | ✅ written |
| **P07** | [Physics and Electronics for Software Engineers](P07_Physics_and_Electronics_for_Software_Engineers.md) | voltage/power, RC filter, antennas, path loss, noise figure, zero-IF receiver, ADC, ppm, PAPR | ✅ written |
| **P08** | [Digital Foundations](P08_Digital_Foundations.md) | binary, two's complement, cu8/Q15, IEEE-754, endianness, XOR/GF(2), parity, CRC, LFSR | ✅ written |
| **P09** | [Python / NumPy / Matplotlib Workflow](P09_Python_NumPy_Matplotlib_Workflow.md) | arrays, views vs copies, dtypes, complex data, vectorising, FFT recipe, seeds, binary IQ files, plotting, test habits, `rf_utils.py` | ✅ written |
| **P10** | [C++ Readiness](P10_Cpp_Readiness.md) | compile/link, types & UB, references, stack/heap/RAII, ASan/UBSan, `std::vector`/`std::complex`, benchmarking, Python→C++ cheat-sheet | ✅ written |
| **P11** | [Systems Baseline: OS, Linux, Concurrency, Protocol Layers](P11_Systems_OS_Linux_Concurrency_Protocols.md) | syscalls, scheduling/affinity, interrupts & DMA, page faults, cache locality, timer jitter, data races, CLI, SDU/PDU/ARQ/windows | ✅ written |
| **P12** | [Bridge to Your College Notes](P12_Bridge_to_Your_College_Notes.md) | stage-by-stage overlap map, terminology translation, SINR-model gap (7.8 dB), GSM↔LTE sync/access, GPRS states↔RRC states, formulas re-verified | ✅ written |
| **P13** | [Readiness Exam](P13_Readiness_Exam.md) | 60 written questions + 7 hands-on labs, scoring bands, per-lesson gates, remediation map | ✅ written |

Each module ends with **Practice → Active Recall → Self-check → Answer Key** (answers after the questions).

---

## 0.3 Where each prerequisite is used in the curriculum

*(Strict keyword scan of the 48 lesson files: decibel/dB terms appear in 13 lessons; series/Taylor/geometric-series terms in 16; probability terms (Gaussian, variance, Q-function, Monte Carlo…) in 15; NumPy code in about 29; C++ in 13; linear-algebra terms are heaviest in Lesson 14 (MIMO). Counts are approximate — they measure keywords, not importance.)*

| Module | Lessons that rely on it (from the keyword scan; heaviest first where noted) |
|---|---|
| **P01 dB / logs** | 7 (AWGN, Eb/N0, BER) · 25 (fixed-point, 6.02B+1.76 dB) · 8 (QPSK/QAM) · 18 (cellular) · 42 (PA/PAPR) · 15 (real IQ) · 20 · 48 · 14 · 32 · 41 |
| **P02 trig / sinusoids** | 1 (identities, ±f problem) · 12, 15, 21, 48 (cos/sin forms) · radians and phase in 3, 10, 13, 16, 19 |
| **P03 series / calculus** | 1 (Taylor series → Euler's formula) · 6 (correlation sums) · 14 · 46 · 3 (DFT sums, orthogonality) · 4 (convolution sums) · 9, 10, 13, 16 (phase derivative = frequency), 19, 42 |
| **P04 linear algebra** | **14 (MIMO — heaviest)** · 18 · 12 · 11 · 13 · 26 · 45 (parity matrices) · light use in 46 and 48 |
| **P05 probability / statistics** | **7 (BER, Q-function — heaviest)** · 6 · 8 · 10 (Rayleigh) · 31 · 14 · 25 · 39 · statistics for measurement in 11 and 36 |
| **P06 signals / Fourier series** | Stage 1 as a whole: 2 (sampling), 3 (DFT/FFT), 4 (convolution), 5 (multirate) |
| **P07 physics / radio chain** | **14, 46 (antennas, arrays — heaviest)** · 20 · 30 · 47 · ADC / oscillator / mixer concepts in 2, 13, 15 |
| **P08 binary / GF(2)** | **25 (fixed-point, Q15 — heaviest)** · 32 (m-sequences) · 17 (file formats) · 41, 44, 45 (convolutional, polar, LDPC codes) |
| **P09 Python / NumPy** | virtually every lesson with a "Code" section (about 29 of the 48 lessons contain NumPy code) |
| **P10 C++** | **22 (memory layout — heaviest)** · 24 · 31 · 23 · 36 · 32 · 26 · 37 · 43 |
| **P11 OS / Linux / networking** | **24 (threads, lock-free — heaviest)** · 23 · 16 · 27 (profiling, `perf`) · 20, 21 (HARQ, RLC/PDCP headers) · 33 (protocol stack) · 38, 39 (trace analysis, conformance) |

> The curriculum's own Linux chapter (Ch. 2) maps to Robert Love's *Linux Kernel Development* and focuses on real-time scheduling, CPU affinity, interrupts, memory management, kernel synchronisation and timers. P11 prepares the vocabulary needed to read that chapter comfortably.

---

## 0.4 Suggested pace

The curriculum's own plan puts **Stage 0 (math refresh) + Stage 1 (signals & systems) in months 1–2**. A sensible plan for these prerequisites *before* that clock starts:

| Week | Modules | Hours (suggested) |
|---|---|---|
| 1 | P01, P02 | 8 |
| 2 | P03, P06 | 8–10 |
| 3 | P04, P05 | 8–10 |
| 4 | P07, P08 | 8 |
| 5 | P09, P10 | 8–10 |
| 6 | P11, P12, P13 (readiness exam) | 6–8 |

*These hour counts are my own estimate for a motivated third/fourth-year B.Tech student, not figures from the curriculum.* Compress if you already know a module — use its **Self-check** list: if you can tick every box, skip it.

**How to study each module:** read → run every code block yourself (don't copy outputs) → do the practice problems on paper first → compare with the answer key → close the file and answer the Active Recall questions from memory.

---

## 0.5 Tool setup checklist (do this once)

The lessons use **Python 3 with NumPy, SciPy, Matplotlib** (NumPy is imported across the lessons, with `scipy.signal`, `scipy.special` and `matplotlib` in a few), **C++ with `g++`** (headers like `<vector>`, `<complex>`, `<cstdint>`, `<atomic>`, `<chrono>`, and `<immintrin.h>` for AVX2), and later **GNU Radio, SigMF, RTL-SDR, srsRAN** and memory tools (AddressSanitizer, Valgrind, `perf`).

- [ ] Python 3.10+ installed; create a virtual environment (`python3 -m venv rf-env && source rf-env/bin/activate`)
- [ ] `pip install numpy scipy matplotlib jupyter`
- [ ] `g++ --version` works (GCC 11+ is fine); `git --version` works
- [ ] A Linux shell (native Linux, WSL2 on Windows, or macOS Terminal)
- [ ] A scratch folder with Git initialised (`git init`) — you will keep all exercises
- [ ] *(later, not now)* GNU Radio, an RTL-SDR dongle — introduced in Stage 5; not needed for prerequisites

---

## 0.6 Quick diagnostic — 10 questions (≈ 10 minutes, no calculator)

1. A gain of **3 dB** multiplies *power* by about… ?
2. **10 mW** equals how many dBm?
3. What is **cos(π)**?
4. What is the angular frequency ω (rad/s) of a **1 kHz** tone?
5. Compute **(3 + 4j)(3 − 4j)**.
6. Compute the matrix–vector product **[[1,2],[3,4]] · [1,1]ᵀ**.
7. Evaluate **Σₙ₌₀³ 2ⁿ**.
8. What is the **8-bit two's-complement** value of the bit pattern **11111111**?
9. **1011 XOR 0110** = ?
10. A fair coin is tossed twice. **P(two heads)** = ?

**Scoring:** 9–10 → you can skim P01–P05 via their self-checks; 6–8 → read all modules in order; ≤ 5 → start from P01 and don't skip anything.

### ✅ Diagnostic answers (all executed)
1. **×2** (3.01 dB). 2. **10 dBm**. 3. **−1**. 4. **2π × 1000 ≈ 6283 rad/s**. 5. **25** (|z|² = 3² + 4²). 6. **[3, 7]ᵀ**. 7. 1 + 2 + 4 + 8 = **15**. 8. **−1**. 9. **1101**. 10. **0.25**.

---

## 0.7 Rules of the road
* **Don't skip the "Common mistakes" section** of each module — those are the exact bugs that waste days in later lessons.
* **Units and dB discipline from P01** applies in every later module.
* If a concept feels shaky, *run the code, change a number, predict the output first.* Several lessons (e.g. 3 and 5) explicitly ask you to predict a result before running the code.
* All modules are now written (P01–P13). Code blocks were executed during writing; C++ blocks were compiled with `g++ -std=c++17`.

---

*Start with **P01 — Units, Logs and Decibels**, then **P02** through **P13**. **All thirteen modules are complete — finish with the P13 readiness exam before opening Lesson 1.***
