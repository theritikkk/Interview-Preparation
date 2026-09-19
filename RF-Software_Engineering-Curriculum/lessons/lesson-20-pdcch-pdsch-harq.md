[← Table of Contents](../README.md) · [← Lesson 19](lesson-19-lte-cell-search.md)

---

# Lesson 20 — PDCCH, PDSCH, and HARQ

*Stage 7 — LTE, continued. Where "here's your data" (scheduling) and "let's not waste a failed attempt" (retransmission) turn out to reuse Lesson 6 and Lesson 14's Cauchy-Schwarz result for a third time.*

### 1. Intuition

Lesson 19 got a device onto a cell — it still doesn't know when or where its own data will actually appear. LTE schedules resources dynamically, subframe by subframe, among however many UEs are active. **PDCCH** is the "here's your ticket" channel: a small control message telling a specific UE exactly where in the resource grid to find its data (**PDSCH**) this subframe. And because wireless links are never perfectly reliable, LTE doesn't just detect a failed transmission and throw it away — it **combines** the failed attempt's soft information with the retransmission, using precisely the same statistical machinery you've now derived twice already, for a third context.

### 2. The engineering problem that forces this

Two distinct problems. First: the network must tell each active UE, every 1 ms, exactly where its data lives — but explicitly signaling *where the control message itself begins* for every UE would create exactly the overhead problem dynamic scheduling exists to avoid. The solution is **blind decoding** across a UE-specific candidate set. Second: some transmissions will fail a CRC check despite channel coding. Simply retransmitting from scratch wastes real information already received — a failed attempt is a noisy, but not useless, observation of the same bits, and Lessons 6 and 14 already proved exactly how to extract maximum value from multiple noisy observations of the same thing.

### 3. Mathematical foundation

**PDCCH** is built from **CCEs** (Control Channel Elements), assembled into **aggregation levels** $\{1,2,4,8\}$ — higher aggregation means more redundancy/lower effective code rate, used for UEs in poorer SNR conditions, a direct instance of Lesson 8's robustness-vs-rate trade-off. A UE blindly decodes a **search space**: a pseudo-random, UE-specific subset of candidate CCE locations (derived from its RNTI and the subframe number), not the entire resource grid.

**HARQ chase combining**: retransmissions are *identical* copies of the original; the receiver combines them as
$$\hat s = \sum_i w_i^* y_i, \qquad w_i \propto \text{(reliability of attempt } i\text{)}$$
**HARQ incremental redundancy (IR)**: retransmissions carry *different* coded bits (different redundancy versions from the same mother code) — genuinely new parity information, not repeated copies.

### 4. Formal definition

**DCI** (Downlink Control Information) is the payload PDCCH carries: resource block assignment, modulation and coding scheme, HARQ process number, redundancy version, and more. A **HARQ process** is one of several (e.g., 8 for FDD) parallel stop-and-wait channels, existing specifically to hide the round-trip ACK/NACK latency — while waiting for feedback on process 1, the sender keeps transmitting on processes 2 through 8, keeping the link continuously busy instead of idling.

### 5. Derivation

**(a) Chase combining is MRC across time — the third appearance of Lesson 6's proof.** Model $N$ chase-combined attempts as $y_i = s + n_i$, $i=1,\dots,N$ — literally identical in structure to Lesson 14 §5(b)'s $N_r$ receive-diversity branches, with "HARQ attempt index" standing in for "antenna index," which itself stood in for "time-sample index" back in Lesson 6 §5. The Cauchy–Schwarz argument doesn't care what the index physically represents — it applies verbatim: the SNR-maximizing weights are $w_i \propto$ (attempt $i$'s own effective channel/reliability), and the maximum combined SNR is the **sum of the per-attempt SNRs**. Three completely different physical settings — correlation in time (Lesson 6), combining across antennas (Lesson 14), and now combining across retransmission attempts — are all, underneath, the identical optimization problem with the identical proof.

**(b) IR is a genuinely different mechanism, not the same proof again.** Chase combining repeats the *same* coded bits, so combining is a weighted sum of noisy copies of one thing. IR instead transmits *different* coded bits (different puncturing/redundancy versions of a mother code) on each attempt — there's no single "same symbol" to weight-and-sum; the decoder must combine genuinely different parity information at the channel-decoder level, extracting a real *coding* gain beyond what simple diversity combining provides. This is worth being precise about specifically because the surface behavior (both "improve reliability using multiple attempts") invites conflating two mechanistically different techniques.

### 6. Visualization

```
Resource grid, PDCCH pointing to PDSCH:
[PDCCH: "UE 42, use RBs 10-15, QPSK, HARQ proc 3"] → [ PDSCH: UE 42's actual data, RBs 10-15 ]

HARQ round trip (parallel processes hide the latency):
Process 1: TX ──────► ...waiting for ACK/NACK...──► NACK → retransmit → combine → ACK
Process 2:      TX ──────► ...waiting...──► ACK
Process 3:           TX ──────► ...waiting...──► ACK
   (processes 2,3 keep the link busy while process 1 awaits feedback)
```

### 7. Worked example

Two chase-combined attempts: first at 0 dB (linear SNR $=1$), second (retransmission) at 3 dB (linear $\approx2$). Combined SNR $=1+2=3$ linear $\approx4.77$ dB — better than either individual attempt, including the stronger second one, exactly Lesson 14 §7's worked example, now interpreted as two transmissions in time rather than two antennas in space.

### 8. Code

```python
import numpy as np
from scipy.special import erfc

def q_function(x):
    return 0.5 * erfc(x / np.sqrt(2))

def chase_combine_ber(ebn0_db_per_attempt, n_attempts, num_bits=10**6):
    bits = np.random.randint(0, 2, num_bits)
    s = 2*bits - 1
    ebn0_linear = 10**(ebn0_db_per_attempt/10)
    noise_std = np.sqrt(1/(2*ebn0_linear))

    combined = np.zeros(num_bits)
    for _ in range(n_attempts):
        attempt = s + noise_std*np.random.randn(num_bits)
        combined += attempt                      # equal SNR per attempt -> equal weights, §5(a)

    decided = (combined > 0).astype(int)
    return np.mean(decided != bits)

for n in [1, 2, 3, 4]:
    ber = chase_combine_ber(ebn0_db_per_attempt=0, n_attempts=n)
    combined_snr_db = 10*np.log10(n * 10**(0/10))
    theory = q_function(np.sqrt(2*10**(combined_snr_db/10)))
    print(f"{n} attempt(s): simulated BER={ber:.2e}   theory (summed-SNR)={theory:.2e}")
```

### 9. Experiment

Run §8's sweep and confirm BER improves with each additional combined attempt, tracking the summed-SNR theoretical prediction (Lesson 7's BER formula, evaluated at the combined SNR rather than the single-attempt SNR). Note how much *more* benefit each additional attempt provides — diminishing but real. Incremental redundancy would show an even steeper improvement per attempt than chase combining, since it adds genuinely new coding information rather than just another noisy copy — a real, important distinction, though implementing IR fully requires channel-coding depth this curriculum builds later.

### 10. Expected output

BER should drop substantially between 1 and 2 attempts, continuing to improve (with diminishing returns) through 3 and 4, each simulated point landing close to the summed-SNR theoretical prediction — direct, numeric confirmation that chase combining behaves exactly like the MRC derivation predicts.

### 11. Debugging exercise

Symptom, no cause given: your chase-combining implementation does improve BER with each retransmission, but nowhere near as much as the summed-SNR theoretical prediction from §9 suggests it should.

What specific step in the combining process, if done incorrectly, would still show *some* improvement (more data is still more data) but fall well short of the provably optimal gain?

**Root cause (revealed):** an **unweighted** (or incorrectly weighted) combination — e.g., a plain average of the attempts instead of the reliability-weighted sum $w_i\propto$ per-attempt SNR derived in §5(a). Averaging isn't *wrong*, exactly — it's the Cauchy–Schwarz proof's inequality holding *strictly*, not at equality, because the weights don't match the optimal $w_i\propto h_i^*$ condition. This is the identical lesson from Lesson 6 §5 and Lesson 14 §5(b), returning a third time: there's a large difference between "a way to combine multiple observations" and "*the* SNR-optimal way to combine them," and only the derived weighting achieves the full predicted gain.

### 12. Common mistakes

- Treating chase combining and incremental redundancy as interchangeable — combining *different* redundancy-version bits (IR) the way you'd combine identical repeated symbols (chase) is a genuine conceptual error, not just a minor implementation detail, per §5(b).
- Forgetting HARQ operates across multiple **parallel processes** specifically to hide ACK/NACK round-trip latency — without them, a sender would idle waiting for feedback after every single transmission, severely limiting throughput.
- Assuming PDCCH blind decoding is unreliable because it's "blind" — in practice each candidate decode attempt is validated by a CRC scrambled with the UE's own RNTI, so a successful decode is self-validating (a wrong UE's control message essentially never produces a passing CRC check for your RNTI), making blind decoding robust despite involving no explicit addressing.

### 13. Real RF/cellular application

PDCCH search-space blind decoding and HARQ combining are exactly how real commercial LTE (and, with updated parameters, NR) scheduling and retransmission work — unchanged in concept from what you've just derived. Diagnosing HARQ retransmission rates and failure patterns is a genuine, common task for RF/protocol engineers, using precisely the modem-diagnostic-tooling skills (QXDM/QCAT-style trace analysis) flagged in this curriculum's career map, and directly practiced in Project 10 (Cellular Protocol Analyzer).

### 14. Interview question

*"Why does HARQ combine failed transmission attempts instead of discarding them and retransmitting fresh?"* Answer from §5(a): a failed attempt still carries real, nonzero information about the transmitted bits — it's a noisy, not useless, observation. Discarding it throws away signal energy already received; combining it with the Cauchy–Schwarz-optimal weighting (identical to Lesson 6's and Lesson 14's proofs) extracts strictly more information than either attempt could provide alone.

### 15. Active recall

1. In one sentence, why is chase combining mathematically identical to MRC, despite combining across time instead of space?
2. What's the fundamental difference between chase combining and incremental redundancy?
3. Why does HARQ need multiple parallel processes rather than one?

### 16. Mini assignment

Using §8's code, run the sweep at a lower per-attempt SNR (e.g., $-3$ dB) and a higher one (e.g., $5$ dB), and compare how many combined attempts each needs to reach a target BER of $10^{-4}$ — connecting HARQ's practical retransmission count directly to link quality, the same way MCS selection (Lesson 8) connects to it.

### 17. Exit test

1. Write the combined-SNR formula for $N$ equal-SNR chase-combined attempts.
2. True or false, with a one-line reason: "Incremental redundancy combining can be implemented by simply summing the raw received symbols from each attempt, the same way chase combining is."
3. Your weighting-error reasoning from §11, if you haven't sent it — this same mistake will resurface in any future combining scenario, so it's worth having solid.

---

**Next:** Lesson 21 covers **RLC, PDCP, and RRC** — tracing what happens to data above the PHY/MAC layers, and answering the "what data moves between layers, and why does each layer exist" question this whole curriculum's roadmap flagged back at the very beginning.

---

[← Lesson 19](lesson-19-lte-cell-search.md) · [Table of Contents](../README.md) · [Lesson 21 →](lesson-21-rlc-pdcp-rrc.md)