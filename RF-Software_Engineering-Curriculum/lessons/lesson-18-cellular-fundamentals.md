[← Table of Contents](../README.md) · [← Lesson 17](lesson-17-recorded-iq-adsb.md)

---

# Lesson 18 — Cellular Fundamentals: Cells, Duplexing, and Why OFDMA Won

*Opens Stage 6 — Cellular Fundamentals. Every lesson so far analyzed a single, isolated link. This lesson is about what changes when that link is one of millions sharing scarce spectrum across a whole network — and it turns out Lessons 3, 11, and 12 already contain the answer to one of this field's biggest historical questions.*

### 1. Intuition

Everything built through Lesson 17 analyzed one transmitter, one receiver, one link. A real cellular network is fundamentally different: potentially millions of users, spread across a wide geographic area, sharing a fixed, finite amount of licensed spectrum. The organizing idea that makes this tractable — splitting coverage into **cells** and deliberately **reusing** the same frequencies at a safe distance apart — is what turns a scarce resource into something that scales to a whole country. This lesson covers the system-level ideas that make everything you've built so far *deployable*, not just correct in isolation.

### 2. The engineering problem that forces this

Two distinct, hard problems. First: given limited spectrum, how do you serve geographically distributed users without their transmissions interfering with each other? Second: a real device usually needs to transmit *and* receive at the same time (a phone call, a data session) — how do you stop your own powerful transmitter from swamping your own much weaker receiver, sitting right next to it? There are exactly two mainstream engineering answers to the second problem, each with real, quantifiable costs, and the first problem's answer changed dramatically between cellular generations — for reasons that connect directly back to tools you already have.

### 3. Mathematical foundation

**Frequency reuse geometry** (standard hexagonal cellular model): for reuse factor $N$ (cells grouped into clusters of $N$, each cell in a cluster using a distinct frequency subset), the distance to the nearest co-channel cell (same frequency, next cluster) relative to cell radius is:
$$\frac{D}{R} = \sqrt{3N}$$
**Signal-to-Interference-plus-Noise Ratio (SINR)**, nearest-interferer approximation, with path-loss exponent $n$:
$$\text{SINR} \approx \left(\frac{D}{R}\right)^n$$
**FDD** (Frequency Division Duplex): separate frequency bands for uplink and downlink, used simultaneously. **TDD** (Time Division Duplex): one shared frequency band, uplink and downlink alternate in time.

### 4. Formal definition

**Co-channel interference** is interference from another cell transmitting on the *same* frequency as your own cell — in a real multi-cell deployment, this, not thermal noise, is usually the dominant impairment, which is why SINR (not plain SNR/$E_b/N_0$) is the relevant metric once you leave the isolated-link world of Lessons 7–14. **3GPP** organizes work into numbered **Releases** (e.g., Release 15 was the first 5G NR release) and technical **working groups** — RAN1 handles physical layer, RAN2 layer 2/3 protocols, RAN3 network architecture, RAN4 RF requirements — with features proposed by member companies as technical contributions, refined through study and work items, and eventually adopted into approved specification text.

### 5. Derivation

**(a) The reuse-factor/SINR trade-off.** Using a standard simplified path-loss model (received power $\propto 1/d^n$) and the hexagonal-geometry relationship $D/R=\sqrt{3N}$, SINR (from the single nearest co-channel interferer) improves as $N$ grows — cells using the same frequency get farther apart. But spectrum available *per cell* is $1/N$ of the total, since the band is split $N$ ways. **Larger $N$ trades capacity for link quality; smaller $N$ trades link quality for capacity** — a direct, quantifiable version of the general engineering theme running through this whole curriculum: nothing is free.

**(b) Why OFDMA replaced CDMA — reasoning from Lessons 3, 11, and 12, not from "it's newer."** 3G (UMTS/CDMA2000) used **CDMA**: every user shares the *same* frequency and time simultaneously, separated only by (nominally) orthogonal spreading codes. The problem: multipath (Lesson 10) destroys perfect code orthogonality, and once codes leak into each other even slightly, a nearby strong user's signal can swamp a distant weak user's — the classic **near-far problem** — requiring extremely tight, continuous power control just to keep the system functioning. **OFDMA** (LTE/NR) takes a completely different approach: assign *different users to different subcarriers* — literally extending single-user OFDM (Lesson 12) so that distinct users occupy distinct resource blocks instead of one user occupying the whole band. Subcarrier orthogonality is guaranteed **by construction** (Lesson 12 §4's DFT orthogonality, not by a code's statistical near-orthogonality), and each user's assigned subcarriers get cleanly equalized independently (Lesson 11) regardless of what any other user is doing. OFDMA sidesteps CDMA's near-far sensitivity entirely by simply never overlapping users in the same time-frequency resource — and gets **frequency-selective multi-user scheduling** (giving each user their best-conditioned subcarriers, a real, actively-used LTE/NR technique) as a natural bonus that CDMA's shared-everything structure can't easily offer. This is the real, technical answer to "why did the industry move from CDMA to OFDMA" — not fashion, a specific, derivable set of engineering advantages.

### 6. Visualization

```
Hexagonal frequency reuse (N=7):              FDD vs TDD:
      B   C                                    FDD:  [====UL band====]  [====DL band====]
    A       D                                        simultaneous, separate frequencies
      G   E
    (each letter = distinct freq subset;      TDD:  [UL][DL][UL][DL][UL][DL]...
     same letters reused at distance D)               same frequency, alternating in time
```

### 7. Worked example

$N=7$ (a classic textbook reuse factor): $D/R=\sqrt{21}\approx4.58$. With path-loss exponent $n=4$: SINR $\approx4.58^4\approx440\approx26.4$ dB — a healthy link. Compare $N=1$ (universal reuse, every cell uses the full band): $D/R=\sqrt3\approx1.73$, SINR $\approx1.73^4\approx9.0\approx9.5$ dB — noticeably worse, but not catastrophic. This is exactly why modern LTE/NR deployments can get away with reuse factor 1 (using the whole band in every cell) despite the lower per-cell-boundary SINR: OFDMA's built-in orthogonality, interference-aware scheduling, power control, and MIMO/beamforming (Lesson 14) manage interference far more effectively than older systems could, making the older frequency-partitioning strategy largely unnecessary.

### 8. Code

```python
import numpy as np

def sinr_db(N, n=4):
    D_over_R = np.sqrt(3*N)
    sinr_linear = D_over_R**n
    return 10*np.log10(sinr_linear)

for N in [1, 3, 4, 7, 9, 12]:
    for n in [3, 4]:
        print(f"N={N:2d}  path-loss n={n}: SINR = {sinr_db(N, n):.1f} dB   "
              f"(spectrum fraction per cell = 1/{N})")
```

### 9. Experiment

Run §8's sweep and tabulate SINR against $N$ and $n$. Notice SINR improves with both larger $N$ (farther-apart interferers) and larger $n$ (interference attenuates faster with distance, e.g. in dense urban environments) — and every dB of SINR gained via larger $N$ comes directly at the cost of less spectrum available per cell. There's no value of $N$ that improves both simultaneously; it's a genuine, quantifiable trade-off, not a bug in the model.

### 10. Expected output

A table showing SINR climbing steadily from $N=1$ (roughly single-digit-to-low-teens dB, depending on $n$) up through $N=12$ (several tens of dB) — concretely demonstrating why older, CDMA-era systems needed larger reuse factors to function acceptably, and why that historical design choice cost significant spectral efficiency compared to what reuse-1 OFDMA systems can achieve today.

### 11. Debugging exercise (system-design diagnosis, not code)

Symptom, no cause given: a TDD-based cellular deployment shows severe, otherwise-unexplained interference for users near the boundary between two adjacent cells, occurring specifically during certain time slots — but users well inside a single cell are unaffected, and an equivalent FDD deployment of the same underlying technology doesn't show this problem at all.

What does TDD specifically require, per §3/§4, that FDD doesn't — and what happens if that requirement is violated between two neighboring cells?

**Root cause (revealed):** **lack of network-wide TDD synchronization.** TDD requires every nearby cell to switch between uplink and downlink at the *same* instant; if two neighboring cells use unsynchronized (or differently patterned) uplink/downlink timing, one cell's strong downlink transmission can be active at exactly the moment a nearby cell is trying to receive a weak uplink signal from its own user — the strong downlink swamps the weak uplink at the cell boundary, a direct violation of the synchronization requirement flagged in §2. FDD never has this specific failure mode because uplink and downlink permanently occupy separate frequencies, with no shared time dimension to desynchronize.

### 12. Common mistakes

- Treating FDD as unconditionally "better" than TDD, or vice versa — each has real, different trade-offs (FDD: simpler synchronization, but needs paired spectrum and physical duplexer filtering hardware, less flexible for asymmetric traffic; TDD: single unpaired spectrum block, flexible/adjustable uplink-downlink ratio for asymmetric data demand, but requires the tight network-wide synchronization §11 shows the cost of violating). Real deployments choose based on available spectrum and traffic patterns, not universal superiority.
- Assuming reuse factor $N=1$ is a design error because it "ignores" co-channel interference — modern systems genuinely and successfully use $N=1$, precisely because OFDMA, scheduling, power control, and MIMO manage interference far better than older systems could, not because interference stopped mattering.
- Treating §5's SINR formula as an exact real-world predictor rather than a simplified textbook model — real deployments have irregular cell shapes, shadowing, multiple simultaneous interferers, and active interference-coordination techniques the simple hexagonal geometry doesn't capture. Useful for intuition and order-of-magnitude reasoning, not for precise link-budget engineering.

### 13. Real RF/cellular application

5G NR predominantly uses TDD in mid-band and FR2/mmWave deployments — well suited to the bursty, asymmetric data traffic of modern smartphone use, and TDD doesn't require paired spectrum, which is often simply unavailable at higher frequencies. FDD remains common in lower, legacy-compatible bands. 3GPP RAN1's frame-structure and numerology decisions directly reflect the FDD/TDD trade-offs derived here. And the shift from reuse-factor thinking to interference-aware scheduling (Lesson 8's link adaptation, reacting to *interference* not just noise) is a direct, practical consequence of §5(b)'s CDMA-to-OFDMA reasoning.

### 14. Interview question

*"Why did cellular standards move from CDMA in 3G to OFDMA in 4G and 5G?"* Answer from §5(b) — not "because it's newer," but specifically: CDMA's shared-everything structure is fundamentally sensitive to multipath-induced code-orthogonality loss and the near-far problem, demanding continuous tight power control; OFDMA sidesteps this by construction (subcarrier orthogonality guaranteed by the DFT, Lesson 3/12) and gains frequency-selective multi-user scheduling as a bonus CDMA structurally can't offer.

### 15. Active recall

1. Why does SINR, not plain SNR, become the relevant metric once you leave an isolated single-link analysis?
2. What's the fundamental trade-off in choosing a larger cellular frequency-reuse factor $N$?
3. Why is CDMA fundamentally more sensitive to the near-far problem than OFDMA?

### 16. Mini assignment

Using §8's code, find the smallest integer reuse factor $N$ that achieves at least 20 dB nearest-interferer SINR, separately for path-loss exponents $n=3$ and $n=4$. Compare the two required values of $N$ and explain, physically, why a larger path-loss exponent requires a smaller reuse factor to hit the same SINR target.

### 17. Exit test

1. Write the reuse-factor-to-distance relationship and the nearest-interferer SINR approximation from memory.
2. True or false, with a one-line reason: "TDD systems require tighter network-wide time synchronization than FDD systems."
3. In your own words: why did the industry move from CDMA to OFDMA?

---

**Next:** Lesson 19 begins **Stage 7 — LTE**, opening with the resource grid, PSS/SSS cell search, and PBCH — where Lessons 1–18 stop being separate tools and start assembling into one real, specified air interface.

---

[← Lesson 17](lesson-17-recorded-iq-adsb.md) · [Table of Contents](../README.md) · [Lesson 19 →](lesson-19-lte-cell-search.md)