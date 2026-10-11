# P12 — Bridge to Your College Notes: What You Already Know, What Differs, What Is New

*Prerequisite module 12 · ⏱ about 3–4 hours · connects the **separate** college folder (`Unit_1_…`, `Unit_2_…`, `Revision_Pack`) to this RF-curriculum folder · the numbers below were computed and the college formulas re-checked by simulation.*

> **Purpose.** You have two bodies of material that look similar but are not the same: your **university Wireless Communication notes** (system-level: cells, handoff, multiple access, fading models, GSM/GPRS/IS-95, Mobile IP — built for an exam) and the **RF curriculum** (implementation-level: DSP, OFDM, LTE/NR physical layer, C++, testing — built to make you hireable). This module tells you where they overlap, where they *disagree*, what each leaves out, and how to use the first to go faster through the second. It does **not** replace either.

> **How the map was built.** I compared the two bodies of text by (a) the curriculum's own stage tags in its README, (b) a keyword scan of the 48 lessons for the college notes' terms, and (c) reading the specific lessons involved (18, 19, 30, 33). The overlap ratings (● strong, ◐ partial/conceptual, ○ none) are **my judgement** from that evidence.

---

## 12.1 The honest big picture

| | **Your college notes** | **The RF curriculum** |
|---|---|---|
| Level | system architecture, analytical models | implementation: algorithms, code, test |
| Era | AMPS → GSM → GPRS/EDGE → IS-95 (2G/2.5G); a bit of 3G | **LTE and 5G NR** (plus DSP/comms foundations) |
| Form | definitions, diagrams, formulas, exam answers | derivations, Python/C++ code, exercises, interview prep |
| Math | standard textbook formulas | derived and then simulated |
| Goal | score in a theory exam | build/debug a modem pipeline; pass interviews |

**What carries over is mostly *concepts*, not details.** A keyword scan shows the curriculum uses *UE* 112 times, *gNB* 20, *downlink/uplink* 27/27, *handover* 13, *SINR* 26, *OFDMA* 21, *Zadoff–Chu* 41 — but mentions **paging, HLR/VLR, TMSI, AMPS, EDGE, Mobile IP, Walsh codes, RAKE and spread-spectrum processing gain 0 times**, GPRS twice, GSM in four files and CDMA in four. So a big slice of your Unit II (AMPS/ETACS, GSM core network, Mobile IP, IS-95 channel structure) is **exam material that the curriculum never needs** — keep it for your university paper, but you do not need it to start Lesson 1.

---

## 12.2 Stage-by-stage bridge 🔥

(Stages and lesson titles as given in the curriculum README. "College §" refers to chapters/sections of your notes: **U1** = Unit I file, **U2** = Unit II file.)

| Stage · Lessons | Topic | Overlap | What your college notes already give you | What the lessons add |
|---|---|---|---|---|
| **0** · L1 | complex numbers, phasors | ○ | sinusoids and c = fλ (U1 Ch.1, Ch.6 Doppler) | the complex-baseband idea; prepared by P02/P03 |
| **1** · L2–L4 | sampling, DFT/FFT, convolution, FIR | ○/◐ | only GSM's 8 kHz speech sampling (U2 Ch.9 §9.7.1) | all of it — P06 is your preparation |
| **2** · L5–L6 | multirate, correlation / matched filter | ◐ | **CDMA despreading *is* correlation** (U1 Ch.5 §5.4, U2 Ch.12) — see P04 §4.3 | decimation/interpolation; matched-filter derivation |
| **3** · L7–L9 | AWGN, BER (BPSK), QPSK/QAM, pulse shaping | ● L7, ◐ L8, ○ L9 | BER = Q(√(2Eb/N0)); AWGN vs Rayleigh; modulation table (U1 Ch.6 §6.6–6.7) | Monte-Carlo BER curves, constellations, RRC filtering / ISI |
| **3** · L41, L44, L45 | convolutional/Viterbi, polar, LDPC | ◐ / ○ | **code parameters** (GSM rate ½ K = 5, IS-95 K = 9 rate ½/⅓, interleaving, CRC — U2 Ch.9 §9.7, Ch.12 §12.5–12.6) | the **decoders** (Viterbi, successive cancellation, belief propagation) |
| **4** · L10 | multipath and fading | ● | Rayleigh/Rician, Doppler, delay spread, coherence time/bandwidth, flat vs selective (U1 Ch.6) | a simulation recipe (P05 §5.8) and the real channel in code |
| **4** · L11, L13 | channel estimation/equalisation, timing & CFO | ◐ | TDMA training sequence/equaliser (U1 Ch.5, U2 Ch.9 §9.4); GSM timing advance | estimators, equalisers, offset correction |
| **4** · L12 | OFDM transceiver | ○ | only the problem it solves (frequency-selective fading, U1 Ch.6) | the complete OFDM TX/RX |
| **4** · L14, L46 | MIMO, beamforming | ◐ | **space diversity; MRC/EGC/SC** (U1 Ch.7); sectoring (U1 Ch.3) | spatial multiplexing, precoding, rank/condition number (P04) |
| **5** · L15–L17, L42 | real IQ, GNU Radio FM, recorded IQ, PA/PAPR | ○ (◐ FM, ◐ GMSK/linear amps) | analog FM voice (U2 Ch.8); constant-envelope GMSK vs linear QPSK (U1 Ch.6 §6.7) | hardware reality (P07), file formats (P08), PA nonlinearity |
| **6** · **L18** | cellular fundamentals, why OFDMA won | **●** | **almost all of it**: cells, hexagons, D/R = √(3N), SINR, duplexing, FDMA/TDMA/CDMA, near–far, power control (U1 Ch.2–5, U2 Ch.12 §12.7–12.8) | the *argument* "why OFDMA beat CDMA" and reuse-1 reasoning |
| **7** · L19 | LTE resource grid, **PSS/SSS cell search**, PBCH | ◐ | the same *problems* solved by GSM's FCCH/SCH/BCCH and IS-95's pilot/sync (U2 Ch.9, Ch.12) | LTE's actual signals and blind detection (§12.5) |
| **7** · L20 | PDCCH/PDSCH/**HARQ** | ◐ | diversity vs ARQ (U1 Ch.7 §7.5); control vs traffic channels (U2 Ch.9 §9.4); GPRS scheduling | HARQ processes and soft combining |
| **7** · L21 | RLC/PDCP/RRC | ◐ | GSM L2/L3 (LAPDm, RR/MM/CM — U2 Ch.9 §9.5); GPRS layers; layered protocols (P11 §11.10) | the LTE layer split and why each layer exists |
| **8** · L28–L30, L47–L48 | NR numerology/SSB, DMRS/CORESET, PRACH/timing advance, beams, CSI | ◐ L29, ◐ L30, ○ rest | IS-95's **pilot** channel as reference signal (U2 Ch.12 §12.5); GSM **RACH** and **timing advance** (U2 Ch.9) | NR specifics; Zadoff–Chu |
| **9** · L31–L32 | PSS/SSS/PBCH pipeline, NR SSB | ◐ | PN / M-sequences (U2 Ch.12 §12.4, P08 §8.9) | building a pipeline in C++ |
| **9/10** · L22–L27 | C++, ring buffers, threads, fixed-point, SIMD, profiling | ○ | — | prepared by P08, P10, P11 |
| **11** · **L33** | NR protocol stack, **RRC_INACTIVE** | **●** (concept) | GPRS states **IDLE / STANDBY / READY** (U2 Ch.10 §10.4.4) and registration/paging (U1 Ch.4) | the third RRC state and QoS flows (SDAP) |
| **12** · L34–L39 | debugging, HIL, latency, ASan, trace analysis, conformance | ○ (◐ L38) | message flows such as the GSM call set-up table (U2 Ch.9 §9.6.4) — good practice for reading traces | testing methodology |
| **14–15** · L40, L43 | interview synthesis, portfolio | ○ | — | — |

---

## 12.3 Terminology translation table 🔥

Same concept, different words. The curriculum's usage is counted from its own text.

| Your college notes | Curriculum / modern 3GPP | Note |
|---|---|---|
| forward / reverse channel | **downlink / uplink** | the curriculum never says "forward/reverse" (0 mentions) |
| mobile station (MS) | **UE** (user equipment) | 112 mentions |
| base station (BS/BTS) | **gNB** (NR), **eNodeB** (LTE) | 20 and 2 mentions |
| handoff | **handover** | 13 vs 1 |
| C/I, S/I, SIR | **SINR** (includes noise) | different *models* — see §12.4 |
| cluster size N | **reuse factor N** | same symbol, same $D/R=\sqrt{3N}$ |
| Eb/N0 | Eb/N0 | same; do not mix it up with SNR (§12.6) |
| FDMA/TDMA/CDMA | **OFDMA** (LTE/NR downlink) | CDMA appears in the curriculum mainly in Lesson 18's "why OFDMA won" |
| PN / Walsh codes | **Zadoff–Chu**, M-sequences | Walsh: 0 mentions; Zadoff–Chu: 41 |
| MSC, HLR, VLR, AuC, EIR | **core network** nodes (e.g. MME, HSS) | the curriculum is baseband-focused: MME, HSS, HLR and VLR are **never mentioned** `[FOUNDATION ADDED for the MME/HSS naming]` |
| SIM, IMSI, TMSI | USIM, IMSI, S-TMSI/GUTI | not covered in the curriculum |
| location area | tracking area | not covered in the curriculum `[FOUNDATION ADDED]` |
| control channels (FCCH, SCH, BCCH, RACH, AGCH, PCH) | physical/logical channels (PSS/SSS, PBCH, PRACH, PDCCH…) | different generation, same jobs (§12.5) |
| GPRS IDLE / STANDBY / READY | **RRC_IDLE / RRC_CONNECTED / RRC_INACTIVE** | §12.5 |

---

## 12.4 Bridge 1 — Frequency reuse and SINR: **two models, a 7.8 dB gap** 🔥⚠️

Your notes (following Singal/Rappaport) use the **six first-tier co-channel interferers**:
$$\frac SI\approx\frac{(D/R)^{n}}{6}$$
**Lesson 18's code** uses **one nearest interferer**:
$$\text{SINR}\approx(D/R)^{n}\qquad(\text{`D_over_R = sqrt(3*N)`; `sinr = D_over_R**n`})$$
Both use $D/R=\sqrt{3N}$; they differ by a constant factor 6 ⇒ **$10\log_{10}6=\mathbf{7.78\ dB}$ everywhere**.

*Verified table* (dB):

| $N$ | $D/R$ | $n=4$: Lesson 18 (1 interferer) | $n=4$: college (6 interferers) | $n=3$: Lesson 18 | $n=3$: college |
|---|---|---|---|---|---|
| 1 | 1.73 | 9.5 | 1.8 | 7.2 | −0.6 |
| 3 | 3.00 | 19.1 | 11.3 | 14.3 | 6.5 |
| 4 | 3.46 | 21.6 | 13.8 | 16.2 | 8.4 |
| **7** | **4.58** | **26.4** | **18.7** | 19.8 | 12.1 |
| 9 | 5.20 | 28.6 | 20.8 | 21.5 | 13.7 |
| 12 | 6.00 | 31.1 | 23.3 | 23.3 | 15.6 |

*Reading it:* for $N=7$, $n=4$ the curriculum says **26.4 dB**, your notes say **18.7 dB** (the classic "18 dB needed for AMPS" target). Neither is "wrong": the curriculum itself calls its formula "a simplified textbook model" (Lesson 18, common mistakes) and warns that real deployments have irregular cells, shadowing and many simultaneous interferers; the six-interferer form is the more conservative textbook model. **Rule:** *in your college exam, use the six-interferer formula and say so; in the curriculum, use Lesson 18's formula; in both, state the assumption.*

*(Lesson 18's own exercise — "smallest N for 20 dB at n = 3 and n = 4" — is left for you; the bridge exercise below uses a different threshold so I don't spoil it.)*

```python
import numpy as np
def sinr_db(N, n=4, interferers=1):
    return 10*np.log10(np.sqrt(3*N)**n / interferers)
print(sinr_db(7, 4, 1), sinr_db(7, 4, 6))     # 26.44  18.66   (difference 7.78 dB)
```

---

## 12.5 Bridge 2 — Same problems, two generations 🔥

**(a) From CDMA to OFDMA (your Unit II Ch.12 ↔ Lesson 18).** The roadmap's Stage-6 exercise is to explain *why the industry moved from CDMA to OFDMA* — and your IS-95 chapter already holds the ingredients. Lesson 18's argument (paraphrased): CDMA's shared-everything structure is sensitive to **multipath-induced loss of orthogonality** and the **near–far problem**, so it needs continuous tight **power control**; OFDMA avoids these by construction (subcarrier orthogonality is guaranteed by the DFT) and also gains frequency-selective multi-user scheduling as a bonus that CDMA structurally cannot offer. Map it to your notes: near–far and power control = U2 Ch.12 §12.7–12.8; multipath and RAKE = §12.10; reuse-1 and soft capacity = §12.3.

**(b) Getting onto the network: sync and random access.** A phone that powers up must solve the same five problems in any generation:

| Problem | **GSM (your notes, U2 Ch.9)** | **LTE/NR (curriculum)** |
|---|---|---|
| Find the carrier frequency | **FCCH** frequency-correction burst | at power-up the device knows *nothing* — not the exact carrier frequency, symbol timing or which of 504 cell identities (Lesson 19); residual CFO is corrected in Lesson 13 |
| Find frame/symbol timing | **SCH** frame-sync burst | **PSS** (symbol 6 of slot 0, central 62 subcarriers) gives symbol timing, but leaves the **10 ms frame boundary ambiguous** until **SSS** resolves it (Lesson 19) |
| Identify the cell | **BSIC** (on the SCH) | **PSS** conveys 1 of 3 sequences ($N_{ID}^{(2)}$), **SSS** 1 of 168 ($N_{ID}^{(1)}$); together $N_{ID}^{cell}=3N_{ID}^{(1)}+N_{ID}^{(2)}$ → **504 physical cell IDs**; PSS alone does *not* reveal the cell (Lesson 19) |
| Read system information | **BCCH** | **PBCH** (decoded only after PSS+SSS) |
| Ask for a channel | **RACH** (slotted ALOHA) → **AGCH** grant | **PRACH** random access with Zadoff–Chu preambles (Lesson 30) |
| Fix propagation delay | **timing advance** | **timing advance**: the UE only has *downlink* timing, so its uplink delay is unknown at first; the preamble's cyclic-shift spacing must exceed the maximum round-trip delay (Lesson 30) |

The function-by-function pairing is conceptual (my mapping); the curriculum describes LTE's side in its own lessons.

**(c) Connection states and mobility (U2 Ch.10 ↔ Lesson 33).** GPRS has **IDLE / STANDBY / READY**: how precisely the network knows where you are (routing area vs cell) trades signalling against paging. NR's **RRC_IDLE / CONNECTED / INACTIVE** is the same trade-off; Lesson 33 argues INACTIVE is a *real third state* — staying CONNECTED between bursts wastes resources and battery, dropping to IDLE forces expensive re-establishment on the next burst. If you understood *why* GPRS needed STANDBY, you already understand *why* NR needs INACTIVE.

**(d) Error control (U2 Ch.9/12 ↔ Lessons 41, 44, 45).** Your notes tell you *what* the codes are (e.g. GSM: 50 + 3 CRC bits, rate-½ K = 5 convolutional code, 456 coded bits/20 ms; IS-95: K = 9). The curriculum teaches *how to decode them* (Viterbi for convolutional codes; polar and LDPC for NR). P08 §8.7 already has the XOR/parity groundwork.

**(e) Diversity and ARQ (U1 Ch.7 ↔ Lessons 14, 20, 46).** MRC, selection and equal-gain combining reappear as MIMO receive combining and **beamforming** (Lesson 46); "diversity vs ARQ" reappears as **HARQ** — ARQ with soft combining of failed attempts.

---

## 12.6 Traps when moving between the two ⚠️

1. **SNR ≠ Eb/N0 ≠ SINR.** Your notes' modulation table (U1 Ch.6 §6.7: BPSK 11.1 dB, QPSK 14.0 dB, 16-PSK 26.0 dB "required S/N for BER 10⁻⁶") is a *textbook S/N figure*; the curriculum works in **Eb/N0** with its own derived BER curves. Don't copy one table into the other.
2. **Rayleigh BPSK formulas.** Your notes give $P_b=\tfrac12[1-\sqrt{\bar\gamma/(1+\bar\gamma)}]\approx1/(4\bar\gamma)$ — fine, but the approximation is loose at low SNR (verified: at 10 dB the exact value is 0.0233 vs 0.025 from $1/4\bar\gamma$).
3. **"Handoff" in an exam vs "handover" in the curriculum** — write the one your examiner/interviewer uses.
4. **Reuse factor:** your notes take N = 7 as classic; Lesson 18 stresses modern systems use **N = 1** — different generations, not contradictory facts.
5. **Walsh codes and RAKE** are exam topics, not curriculum topics — don't expect them in Lessons 1–48.
6. **Hexagons are a model.** Both sources say so; neither is the real coverage shape.
7. **Don't assume notation matches** — your Singal-based notes use $K$ for cluster size and $N$ for total channels in places (U1 Ch.3 warns about this).

---

## 12.7 Re-checking your college formulas by simulation (verified)

The following results were produced by Monte-Carlo runs (2–6 million bits, seeded) to confirm the "foundation" formulas used in your college notes:

| Quantity | Formula in your notes | Simulation |
|---|---|---|
| BPSK in flat Rayleigh, avg SNR 5 / 10 / 15 dB | $\tfrac12[1-\sqrt{\bar\gamma/(1+\bar\gamma)}]$ = 0.06418 / 0.02327 / 0.00772 | **0.06415 / 0.02332 / 0.00772** ✓ |
| SNR needed for BER 10⁻⁴: Rayleigh vs AWGN | 34 dB vs 8.4 dB (≈ 26 dB penalty) | exact: **33.98 vs 8.40 dB ⇒ 25.6 dB** ✓ |
| BER 10⁻³: Rayleigh vs AWGN | 24 dB vs 6.8 dB | **23.97 vs 6.79 dB ⇒ 17.2 dB** ✓ |
| Two-branch MRC, 10 dB per branch | exact 0.001599 (approximation $3/(4\bar\gamma)^2$ gives 0.001875) | **0.001615** ✓ |
| Two-branch MRC needed for BER 10⁻⁴ | 16.4 dB (approximation) | exact **16.28 dB** (gain over one branch ≈ **17.7 dB**) ✓ |
| Selection-diversity outage, threshold 0.1·mean, M = 1–4 | 0.0952, 0.00906, 0.00086, 0.00008 | **0.0953, 0.00905, 0.00088, 0.00008** ✓ |
| Average SNR gain: SC / MRC, M = 2 and 4 | +1.76 / +3.01 dB and +3.19 / +6.02 dB | **1.501×, 2.002× and 2.082×, 3.998×** ✓ |

So the formulas in your college notes hold; the approximations ($1/4\bar\gamma$, $\binom{2M-1}{M}/(4\bar\gamma)^M$) are accurate at high SNR and slightly pessimistic at low SNR.

---

## 12.8 How to use your college notes while doing the curriculum

| When you reach… | First, 10-minute refresh from your notes | Then expect the lesson to add |
|---|---|---|
| Lesson 6 (matched filter) | U1 Ch.5 §5.4 (CDMA despreading), P04 §4.3 | optimal detection theory |
| Lesson 7 (BER) | U1 Ch.6 §6.6, P05 §5.6 | Monte-Carlo curves and the N₀/2 trap |
| Lesson 10 (fading) | U1 Ch.6 §6.2–6.5 | simulation of the channel in code |
| Lesson 14 (MIMO) | U1 Ch.7 (diversity) | spatial multiplexing; P04 rank/SVD |
| **Lesson 18 (cellular)** | U1 Ch.2–5 and U2 Ch.12 §12.7–12.10 | the OFDMA-vs-CDMA argument (§12.5a) |
| Lesson 19 (cell search) | U2 Ch.9 §9.4 (FCCH/SCH/BCCH) | PSS/SSS/PBCH in detail |
| Lessons 20–21 (HARQ, layers) | U1 Ch.7 §7.5; U2 Ch.9 §9.5 and Ch.10; P11 §11.10 | HARQ combining; LTE layer roles |
| Lesson 30 (PRACH) | U2 Ch.9 (RACH, timing advance) | Zadoff–Chu preambles |
| Lesson 33 (RRC states) | U2 Ch.10 §10.4.4 | RRC_INACTIVE |
| Lesson 41 (Viterbi) | U2 Ch.9 §9.7.3, Ch.12 §12.5 | trellis decoding |

---

## 12.9 Bridge exercises (answers at the end)

1. Compute the nearest-interferer SINR (Lesson 18 style) and the six-interferer value for $N=9$, $n=3.5$. By how many dB do they differ?
2. For $n=3.5$, find the smallest *valid* hexagonal cluster size ($N\in\{1,3,4,7,9,12,13,\dots\}$) giving **≥ 15 dB** under (a) the one-interferer model, (b) the six-interferer model.
3. Using the exact Rayleigh formula, how much more average SNR does BPSK need than in AWGN to reach BER $10^{-3}$? Where did you see the AWGN curve in the curriculum and which lesson models the Rayleigh channel?
4. A two-branch MRC receiver has 10 dB average SNR per branch. Estimate the BER improvement over a single branch (exact values from the table above).
5. Match each GSM item to the LTE/NR function it parallels, and say what the LTE side does *differently*: FCCH, SCH, BCCH, RACH, timing advance.
6. Translate into curriculum language: "A mobile station moving along the reverse channel is handed off by the MSC when the forward-channel C/I falls below 18 dB."
7. GPRS keeps a user in STANDBY between packet bursts. Which NR RRC state is the analogue of this idea, and what trade-off does it manage?
8. *Think:* your notes pick $N=7$; Lesson 18 says modern LTE/NR use $N=1$. List the techniques the lesson credits that make $N=1$ workable (it names four).
9. *Think:* which parts of your Unit II would you **not** need to revisit before starting Lesson 1, and why is that still worth keeping for your exam?

## 12.10 Active recall
1. State the two SINR models and the constant that separates them. 2. Name three lessons where your college notes give a head start. 3. List five terms that differ between your notes and the curriculum. 4. What five problems does a phone solve at power-up, in both GSM and LTE? 5. Why did CDMA lose to OFDMA (Lesson 18's argument)? 6. What do your notes teach about channel codes that the curriculum then builds on? 7. Why is the curriculum's coverage of Unit II topics small, and which topics are absent? 8. What is the difference between SNR, Eb/N0 and SINR?

## 12.11 Self-check — ready for P13 if you can…
- [ ] explain the 7.8 dB difference between the two SINR models and use the right one in each context;
- [ ] map GSM's sync/access channels onto LTE's cell-search and PRACH functions;
- [ ] translate college terminology into curriculum terminology;
- [ ] say which of your Unit II topics the curriculum skips;
- [ ] use §12.8 to plan a 10-minute refresh before each relevant lesson.

---

## ✅ Answer key

1. $N=9$: $D/R=\sqrt{27}=5.196$; nearest-interferer SINR $=35\log_{10}5.196=\mathbf{25.05\ dB}$; six-interferer $=25.05-7.78=\mathbf{17.27\ dB}$; difference **7.78 dB** ($10\log_{10}6$). 2. (a) One-interferer: $N=3$ gives 16.7 dB (≥ 15) ⇒ **N = 3** (N = 1 gives only ~8.4 dB). (b) Six-interferer: $N=3\to8.9$, $N=4\to11.1$ dB, $N=7\to\mathbf{15.4}$ dB ⇒ **N = 7**. 3. $\approx\mathbf{17.2\ dB}$ more (23.97 vs 6.79 dB). AWGN BPSK BER is **Lesson 7**; the fading channel is **Lesson 10**. 4. Single-branch Rayleigh at 10 dB: 0.0233; two-branch MRC: **≈ 0.0016** — about **14.6×** lower (0.02327/0.001599). 5. FCCH → frequency/carrier synchronisation (LTE: handled across cell search and CFO correction, Lessons 13/19); SCH → timing synchronisation (LTE: **PSS** then **SSS** — PSS alone leaves the 10 ms frame boundary ambiguous); BCCH → system information (LTE: **PBCH**); RACH → random access (LTE/NR: **PRACH** with Zadoff–Chu preambles); timing advance → same purpose in both: the UE's uplink delay is unknown at first and must be corrected. 6. "A **UE** on the **uplink** is **handed over** by the network when the **downlink SINR** falls below the threshold." (Note: "SINR", not C/I, and "handover", not "handoff".) 7. **RRC_INACTIVE** (Lesson 33): it keeps the UE registered with cheap resumption so it doesn't pay full re-establishment from IDLE nor waste resources staying CONNECTED — the same signalling-versus-responsiveness trade-off as STANDBY. 8. Per the lesson: **OFDMA, scheduling, power control and MIMO** manage interference far better than older systems could — not because interference stopped mattering. 9. AMPS/ETACS, the GSM core-network nodes (HLR/VLR/AuC/EIR), Mobile IP, EDGE and IS-95 channel details don't appear in the curriculum (0 mentions in the scan), so Lesson 1 doesn't depend on them; keep them because your **university paper** asks for them, and because the *concepts* (handoff, registration, power control, ARQ) recur.

---

*Next: **P13 — Readiness exam** (a graded test across P01–P12, with a scoring guide and a "start Lesson 1" decision).*
