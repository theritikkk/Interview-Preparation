[← Table of Contents](../README.md) · [← Lesson 18](lesson-18-cellular-fundamentals.md)

---

# Lesson 19 — LTE: The Resource Grid, PSS/SSS Cell Search, and PBCH

*Opens Stage 7 — LTE. This is where the last eighteen lessons of general theory become one real, fully specified air interface — starting with the very first thing any LTE device does on power-up, and it turns out the ideal signal for that job is provable from the same "sum of roots of unity" fact buried inside Lesson 3's DFT.*

### 1. Intuition

Everything through Lesson 18 was generic wireless/OFDM theory — true of any system built this way. LTE pins down every parameter to a specific number: exact subcarrier spacing, exact frame structure, exact sequences for synchronization. A device powering on knows *nothing*: not exact carrier frequency, not symbol timing, not which of 504 possible cell identities it's listening to. Within a fraction of a second, it must find a cell entirely blind. This procedure — **cell search** — is where Lesson 6 (matched filtering), Lesson 13 (timing/CFO), and Lesson 3 (the FFT) stop being separate tools and assemble into one real, working system.

### 2. The engineering problem that forces this

You need a signal engineered specifically for blind detection: unambiguous timing recovery even before you know the channel, robust identification of one of many possible identities, and enough structure to bootstrap frame timing — all before the receiver has any other information to work with. This isn't solved with an arbitrary sequence; it demands one with very specific, provable mathematical properties, chosen deliberately for exactly this problem.

### 3. Mathematical foundation

**LTE frame structure:** 15 kHz subcarrier spacing; one slot = 0.5 ms (7 OFDM symbols, normal CP); one subframe = 1 ms (2 slots); one radio frame = 10 ms (10 subframes). **Resource element (RE)** = one subcarrier × one OFDM symbol; **resource block (RB)** = 12 subcarriers × one slot.

**PSS** (Primary Synchronization Signal): one of 3 Zadoff-Chu sequences, conveying $N_{ID}^{(2)} \in \{0,1,2\}$. **SSS** (Secondary Synchronization Signal): conveys $N_{ID}^{(1)} \in \{0,\dots,167\}$. **Physical Cell ID:** $N_{ID}^{cell} = 3N_{ID}^{(1)} + N_{ID}^{(2)}$ — 504 total possible identities.

**Zadoff-Chu (ZC) sequence**, root index $u$, length $N$ (odd, $\gcd(u,N)=1$):
$$x_u[n] = e^{-j\pi u n(n+1)/N}$$

### 4. Formal definition

A ZC sequence is **CAZAC**: Constant Amplitude ($|x_u[n]|=1$ for all $n$) and Zero (ideal) Autocorrelation — proven in §5. PSS uses $N=63$ (62 of the 63 values mapped onto subcarriers, with the center element excluded), with three defined LTE root indices $u\in\{25,29,34\}$ mapped one-to-one onto $N_{ID}^{(2)}\in\{0,1,2\}$.

### 5. Derivation — proving Zadoff-Chu's ideal autocorrelation, from the same fact behind the DFT

Compute the periodic autocorrelation $R(k) = \sum_{n=0}^{N-1} x_u[n]\,x_u^*[(n+k)\bmod N]$. For $k=0$: every term has $|x_u[n]|^2=1$, so $R(0)=N$ trivially. For $k\ne0$, expand the product:
$$x_u[n]\,x_u^*[n+k] = \exp\!\left(\frac{j\pi u}{N}\big[(n+k)(n+k+1) - n(n+1)\big]\right)$$
Expand the bracket: $(n+k)(n+k+1)-n(n+1) = 2nk+k^2+k = k(2n+k+1)$. So the term becomes:
$$\exp\!\left(\frac{j\pi u k(k+1)}{N}\right)\cdot\exp\!\left(\frac{j2\pi uk\,n}{N}\right)$$
The first factor doesn't depend on $n$ — pull it out of the sum:
$$R(k) = e^{j\pi uk(k+1)/N}\sum_{n=0}^{N-1} e^{j2\pi (uk) n/N}$$
That remaining sum is exactly the **sum of the $N$-th roots of unity** raised to successive powers — the identical fact underlying why the DFT basis functions are orthogonal (Lesson 3): this geometric series equals $N$ if $uk\equiv0\pmod N$, and **exactly zero otherwise**. Since $\gcd(u,N)=1$ and $0<k<N$, $uk\not\equiv0\pmod N$, so the sum is **exactly zero**. Therefore:
$$R(k) = 0 \text{ for all } k\ne0, \qquad R(0)=N$$
A perfectly sharp single spike, exactly zero everywhere else — not approximately good, *ideal*. This is precisely why ZC sequences are the right tool for blind timing detection: correlate against one (Lesson 6's matched filter, now applied to a sequence engineered to make that correlation unambiguous by construction), and there is no possible confusable sidelobe from the sequence's own structure.

### 6. Visualization

```
LTE resource grid (first subframe, center 6 RBs):

freq ↑  [ PSS ]                    ← symbol 6 (last symbol of slot 0), center 62 subcarriers
        [ SSS ]                    ← symbol 5, center 62 subcarriers
        [ PBCH ][ PBCH ][PBCH][PBCH] ← symbols 7-10, center 72 subcarriers
        ─────────────────────→ time

Zadoff-Chu autocorrelation (§5's result):
        │
        │           ▲  ← single spike at k=0, height N
────────┴───────────┴───────────  ← exactly zero everywhere else
```

### 7. Worked example

Real LTE PSS uses $N_{ZC}=63$ with root indices $u\in\{25,29,34\}$ — three specific, fixed sequences, one per $N_{ID}^{(2)}$. A receiver correlates the incoming signal against all three simultaneously; whichever gives the strongest peak identifies both the exact symbol timing (from the peak's position) and $N_{ID}^{(2)}$ (from *which* candidate won) in a single operation — a genuine three-way matched-filter bank, not three separate sequential searches.

### 8. Code

```python
import numpy as np

def zc_sequence(u, N):
    n = np.arange(N)
    return np.exp(-1j*np.pi*u*n*(n+1)/N)

N = 63
roots = {0: 25, 1: 29, 2: 34}   # LTE's actual PSS root-index mapping

# Verify ideal autocorrelation, §5's proof, numerically
x = zc_sequence(25, N)
autocorr = np.correlate(np.concatenate([x,x]), x, mode='valid')[:N]
print("autocorrelation (should be N at k=0, ~0 elsewhere):", np.round(np.abs(autocorr), 2))

# --- Simulate blind PSS detection: unknown timing, unknown which of 3 sequences ---
true_nid2 = 1
true_pos = 500
rx = 0.3*(np.random.randn(2000)+1j*np.random.randn(2000))
rx[true_pos:true_pos+N] += zc_sequence(roots[true_nid2], N)

best_score, best_nid2, best_pos = -1, None, None
for nid2, u in roots.items():
    template = np.conj(zc_sequence(u, N)[::-1])          # matched filter, Lesson 6
    corr = np.abs(np.convolve(rx, template, mode='valid'))
    peak = np.max(corr)
    if peak > best_score:
        best_score, best_nid2, best_pos = peak, nid2, np.argmax(corr)

print(f"detected N_ID(2)={best_nid2} (true={true_nid2}), position={best_pos} (true={true_pos})")
```

### 9. Experiment

Run §8's code and confirm both the correct $N_{ID}^{(2)}$ and correct timing position are recovered. Then reduce SNR (increase the noise scale) progressively and find roughly where detection starts failing — connecting back to Lesson 6 §9's peak-to-sidelobe robustness experiment and Lesson 7's general SNR-vs-reliability theme, now applied to a real, specified signal instead of a generic example.

### 10. Expected output

The autocorrelation check should show essentially $N$ at lag 0 and values at or near the floating-point noise floor everywhere else — direct numeric confirmation of §5's proof. The detection test should correctly recover both the true root index and the true timing position at reasonable SNR, degrading only as noise grows large.

### 11. Debugging exercise

Symptom, no cause given: your three-way PSS correlator produces a strong, sharp-looking peak — but which of the three candidate sequences wins is inconsistent across otherwise-identical runs, occasionally picking the wrong root index even at fairly high SNR.

What element of the received signal, not yet accounted for in §8's code, could distort correlation enough to occasionally favor the wrong candidate? (This should connect directly to a specific earlier lesson.)

**Root cause (revealed):** **uncompensated carrier frequency offset (CFO)**, Lesson 13's territory returning at exactly the moment it matters most. A real received signal almost never arrives with zero residual CFO before initial synchronization — that's precisely the chicken-and-egg problem real cell search has to solve. A frequency offset smears and reduces the sharpness of *every* candidate's correlation peak (Lesson 13 §5(b)'s fractional-CFO leakage, applied here), and if that smearing is severe enough, noise can occasionally push a wrong candidate's degraded peak above the true candidate's own degraded peak. This is precisely why real LTE cell-search implementations perform a **coarse frequency-offset search or joint timing/frequency estimation** — not a single blind correlation assuming perfect frequency alignment — during this exact stage, before trusting any single PSS correlation result.

### 12. Common mistakes

- Believing PSS alone reveals the full cell identity — it only conveys $N_{ID}^{(2)}$ (1 of 3); the other factor, $N_{ID}^{(1)}$ (1 of 168), requires decoding SSS as well, a very common beginner misconception.
- Forgetting that PSS/SSS occupy only the **center 62 subcarriers**, regardless of the cell's actual overall channel bandwidth — a deliberate design choice meaning cell search doesn't require knowing the full channel bandwidth in advance, letting a device search for cells before it has any configuration information at all.
- Assuming PBCH can be decoded immediately after PSS — SSS is also needed to resolve the 10 ms frame-boundary ambiguity that PSS alone leaves unresolved (SSS's sequence differs between LTE's two possible half-frames), and only after that does PBCH decoding proceed. This is a genuine ordering dependency in the spec, not an arbitrary implementation choice.

### 13. Real RF/cellular application

This is literally the first procedure every LTE-capable chipset executes on power-up, before anything else in the modem stack can function. Cell-search speed and reliability at low SNR (cell-edge conditions) is a genuine, measurable performance differentiator between real modem implementations, and a common interview/design discussion topic at exactly the companies in this curriculum's career map. Once cell search completes, **PBCH** carries the Master Information Block (MIB) — just enough bits (system bandwidth, PHICH configuration, system frame number) to bootstrap reading the rest of the cell's broadcast system information.

### 14. Interview question

*"Why does LTE specifically use a length-63 Zadoff-Chu sequence for PSS, rather than some other sequence?"* Answer from §5's proof: ZC sequences have provably ideal (exactly zero, not just low) periodic autocorrelation and constant amplitude — exactly what's needed for an unambiguous, sharp single timing peak with zero self-generated sidelobes, letting a matched-filter detector (Lesson 6) operate at genuinely optimal reliability during blind, time-critical cell search.

### 15. Active recall

1. State, precisely, what PSS alone tells a receiver, and what it does *not* tell it.
2. In one line, why does the sum in §5's derivation vanish for $k\ne0$?
3. Why must SSS be decoded before PBCH, not just after PSS?

### 16. Mini assignment

Extend §8's code to also search over a small set of candidate integer sample-timing offsets around the true position, confirming the detector correctly locates timing without prior knowledge of exactly where the signal starts — turning the toy example into a genuinely blind search, closer to what a real receiver must do.

### 17. Exit test

1. Write the formula for LTE's physical cell ID in terms of $N_{ID}^{(1)}$ and $N_{ID}^{(2)}$, and state how many total cell identities this allows.
2. True or false, with a one-line reason: "A Zadoff-Chu sequence's autocorrelation sidelobes are small but nonzero."
3. Your CFO-based reasoning from §11, if you haven't sent it.

---

**Next:** Lesson 20 continues Stage 7 with **PDCCH, PDSCH, and HARQ** — how a scheduled downlink transmission is actually signaled, decoded, and, when it fails, efficiently retried using something you've already derived in Lesson 11.

---

[← Lesson 18](lesson-18-cellular-fundamentals.md) · [Table of Contents](../README.md) · [Lesson 20 →](lesson-20-pdcch-pdsch-harq.md)