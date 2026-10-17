[← Table of Contents](../README.md) · [← Lesson 52](lesson-52-handover.md)

---

# Lesson 53 — Carrier Aggregation: Why More Bandwidth Doesn't Always Mean More Throughput

*Operators hold spectrum in scattered chunks, and peak-rate targets keep climbing. Carrier aggregation lets one UE use several carriers at once. The interesting question is when that actually helps, and Shannon's formula answers it exactly.*

### 1. Intuition

Spectrum is fragmented: an operator might hold a 20 MHz block at 800 MHz, another at 1800 MHz, another at 2600 MHz. **Carrier aggregation (CA)** lets a single UE transmit and receive on several of these **component carriers (CCs)** simultaneously. One carrier acts as the **PCell** (it carries the RRC connection and the security anchor); the others are **SCells**, each with its own MAC/HARQ processing but sharing one RRC connection and one PDCP/SDAP stack above (the layer split from Lessons 21 and 33). The naive expectation is that five 20 MHz carriers give five times the throughput of one. Sometimes they do. Sometimes they give almost nothing, and the reason falls straight out of Shannon's capacity formula.

### 2. The engineering problem that forces this

Predict how aggregate throughput scales with the number of carriers, and identify the conditions under which aggregation stops paying off, so that scarce RF-front-end complexity and UE power aren't spent on carriers that can't help.

### 3. Mathematical foundation

Shannon capacity of one carrier of bandwidth $B$ with total received power $P$ and noise density $N_0$:
$$C(B)=B\log_2\!\Big(1+\frac{P}{N_0B}\Big)$$
Define the single-carrier SNR $x=P/(N_0B)$. Aggregating $M$ carriers means bandwidth $MB$. In the **uplink**, the UE's total transmit power $P$ is fixed by its power class, so it is *shared* across carriers. In the **downlink**, the base station typically has a separate power budget per carrier, so per-carrier SNR is unchanged as carriers are added.

### 4. Formal definition

**Aggregation gain** $G(M,x)=C(MB)/C(B)$ at fixed total power. **PCell/SCell**: the primary cell handles the RRC connection and mobility; secondary cells add capacity and can be activated or deactivated by MAC signaling. **Intra-band contiguous / intra-band non-contiguous / inter-band**: the three spectral arrangements, each demanding progressively harder RF front-end design.

### 5. Derivation — when aggregation helps, and when it can't

**(a) Capacity gain at fixed total power.** Spreading $P$ over $MB$ of bandwidth gives per-hertz SNR $x/M$:
$$G(M,x)=\frac{MB\log_2(1+x/M)}{B\log_2(1+x)}=\frac{M\log_2(1+x/M)}{\log_2(1+x)}$$
(Splitting power equally across $M$ separate carriers of width $B$ gives the identical result, since each sees SNR $\frac{P/M}{N_0B}=x/M$, so the equal split costs nothing in the ideal case.)

*Low-SNR limit.* For small $y$, $\log_2(1+y)\approx y/\ln 2$. Then the numerator is $M\cdot\frac{x/M}{\ln2}=\frac{x}{\ln 2}$ and the denominator is $\frac{x}{\ln2}$, so
$$G\to1.$$
When a link is **power-limited**, extra bandwidth adds essentially nothing: capacity saturates at $P/(N_0\ln2)$, independent of $B$.

*High-SNR limit.* For large $x$, $G\approx M\Big(1-\frac{\log_2 M}{\log_2 x}\Big)$, approaching $M$ — near-linear scaling when the link is **bandwidth-limited**.

**(b) A second, physical effect: carriers are not equal.** Free-space path loss grows as $20\log_{10}f$. Between an 800 MHz and a 2600 MHz carrier serving the same location, the extra loss is $20\log_{10}(2600/800)=20\log_{10}(3.25)\approx10.2$ dB. The higher carrier therefore delivers roughly 10 dB less SNR at the same distance, and by (a), an SCell at low SNR contributes little. This is why SCells on higher bands are often usable only in part of a cell's coverage. (Real urban propagation differs from free space in detail, but the direction of the effect holds.)

### 6. Visualization

```
Gain G(M=5, x) at fixed total power:

G
5 |                           ......  (bandwidth-limited: approaches M)
  |                     ......
3 |               ......
  |         ......
1 |_______.......__________________________  (power-limited: G -> 1)
   -10 dB     0 dB     10 dB     20 dB     30 dB   (single-carrier SNR x)
```

### 7. Worked example

Five carriers ($M=5$). At $x=20$ dB (100): $G=5\log_2(21)/\log_2(101)=5(4.392)/6.658\approx3.30$. At $x=0$ dB (1): $G=5\log_2(1.2)/\log_2(2)\approx1.32$. At $x=-10$ dB (0.1): $G=5\log_2(1.02)/\log_2(1.1)\approx1.04$. The same five carriers deliver 3.3× at a cell-center SNR and essentially 1× at a cell-edge SNR.

### 8. Code

```python
import numpy as np

def aggregation_gain(M, snr_db):
    x = 10 ** (snr_db / 10)
    return M * np.log2(1 + x / M) / np.log2(1 + x)

print("SNR(dB)   M=2    M=3    M=5")
for snr_db in [-10, 0, 10, 20, 30]:
    row = [aggregation_gain(M, snr_db) for M in (2, 3, 5)]
    print(f"{snr_db:6d}  " + "  ".join(f"{g:5.2f}" for g in row))

# Sec 5(b): free-space path loss difference between two carrier frequencies
print("extra path loss 800 MHz -> 2600 MHz:", 20 * np.log10(2600 / 800), "dB")
```

### 9. Experiment

Run §8 and confirm the two limits from §5(a): $G\approx1$ at the low-SNR end and $G$ approaching $M$ (never reaching it) at the high-SNR end. Then find, for $M=5$, the SNR at which $G$ first reaches half of its ceiling (2.5) — a number that tells you which UEs a five-carrier configuration actually serves well.

### 10. Expected output

Approximately (values from my own calculation, so verify against your run):

```
SNR(dB)   M=2    M=3    M=5
   -10    1.02   1.03   1.04
     0    1.17   1.25   1.32
    10    1.49   1.83   2.29
    20    1.70   2.30   3.30
    30    1.80   2.52   3.84
```
The path-loss line should print about 10.2 dB.

### 11. Debugging exercise

Symptom, no cause given: a UE at the cell edge shows near-linear throughput gain from aggregation in the **downlink**, but essentially no gain from aggregation in the **uplink**, even though the same carriers are configured both ways.

Using §3 and §5(a): what is different about the two directions, and what measurement would confirm your hypothesis?

**Root cause (revealed):** the uplink is **power-limited** at the cell edge. The UE's total transmit power is fixed, so adding carriers divides the same power over more bandwidth and the per-hertz SNR falls — the $G\to1$ regime of §5(a). In the downlink the base station adds power with each carrier, so per-carrier SNR is preserved and gain stays near-linear. The confirming measurement is the UE's **power headroom report** (the MAC-layer report of how far the UE is below its maximum transmit power): at the cell edge it is already near zero on the PCell alone, so there is no spare power for a second carrier to use. This ties directly to Lesson 50's power-control formula, where the $P_{MAX}$ cap is exactly the constraint now binding.

### 12. Common mistakes

- Assuming throughput scales linearly with aggregated bandwidth unconditionally — §5(a) shows it does so only in the bandwidth-limited regime.
- Confusing carrier aggregation (more *bandwidth*) with MIMO (more *spatial streams*, Lesson 14); they are independent resources and combine multiplicatively when both are available.
- Treating all component carriers as equivalent — §5(b)'s path-loss difference means an SCell can be unusable across much of the coverage area.

### 13. Real RF/cellular application

LTE-Advanced introduced aggregation of up to five 20 MHz carriers (100 MHz total), and NR extends the framework to considerably more carriers across much wider bandwidths. A device does not support arbitrary combinations: each supported band combination is a specific RF front-end design commitment, which is why UE capability lists enumerate permitted combinations explicitly. The uplink-versus-downlink asymmetry derived here is one reason many networks deploy uplink aggregation more selectively than downlink aggregation.

### 14. Interview question

*"Why does uplink carrier aggregation give much smaller gains than downlink aggregation for cell-edge users?"* Answer from §5(a): uplink power is fixed by the UE's power class, so spreading it over more bandwidth lowers per-hertz SNR, and in the low-SNR regime capacity saturates at $P/(N_0\ln2)$ regardless of bandwidth. The base station's per-carrier power in the downlink keeps SNR per carrier constant, so downlink gain stays near-linear.

### 15. Active recall

1. Derive $G(M,x)$ and its two limits.
2. Why is a power-limited link insensitive to additional bandwidth?
3. What does a power headroom report tell you in this diagnosis?

### 16. Mini assignment

Extend §8 to a table over $M\in\{2,3,4,5\}$ and SNR from $-10$ to $30$ dB in 5 dB steps. For each $M$, report the SNR at which $G$ first exceeds $0.5\,M$, and state in one sentence what that implies for which users should be configured with how many carriers.

### 17. Exit test

1. Compute $G(5, 10\text{ dB})$ by hand.
2. True or false, with a one-line reason: "Adding component carriers always increases uplink throughput proportionally."
3. Your reasoning from §11 about why the uplink and downlink behave differently, if you haven't worked through it yet.

---

**Next:** Lesson 54 closes the link-adaptation loop: the **outer-loop MCS adjustment** that keeps block error rate at its target when the reported CQI is systematically optimistic or pessimistic — with the equilibrium step-size ratio derived directly.

---

[← Lesson 52](lesson-52-handover.md) · [Table of Contents](../README.md) · [Lesson 54 →](lesson-54-outer-loop-link-adaptation.md)
