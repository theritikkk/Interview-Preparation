[← Table of Contents](../README.md) · [← Lesson 43](lesson-43-portfolio-packaging.md)

---

# Lesson 44 — Polar Codes: Channel Polarization and Successive Cancellation Decoding

*Closes a real gap left open since Lessons 19, 20, and 29 discussed PBCH and PDCCH without ever specifying how those bits are actually error-protected. Polar codes are the answer for NR's control channels — and the recursive structure that makes them work is a genuine cousin of the FFT butterfly you derived back in Lesson 3.*

### 1. Intuition

Lesson 41's convolutional/Viterbi coding is a real, workhorse scheme, but it isn't provably optimal — it approaches good performance without a constructive guarantee of hitting channel capacity. Polar codes, invented by Arikan in 2008, are the first explicit, constructive code family **proven** to achieve the Shannon capacity of certain channels — and 3GPP selected them for 5G NR's control-channel encoding (PDCCH and PBCH) within about a decade of their invention, one of the most direct paths from cutting-edge information theory into a deployed, billion-device standard in this entire curriculum. The core idea: combine two uses of the same noisy channel in a specific, simple way, and you get one synthetic channel that's *worse* than before and one that's *better* — apply this recursively, and channels literally **polarize** toward perfectly reliable or perfectly useless. Transmit real information only on the reliable ones.

### 2. The engineering problem that forces this

Given $N$ uses of a noisy channel, how do you decide exactly which bit positions carry real information and which are "frozen" (fixed to a known value, carrying zero information, protecting against the worst synthetic channels) so the scheme provably approaches capacity — and how do you decode efficiently, exploiting the recursive structure, rather than paying an exponential cost?

### 3. Mathematical foundation

The **polar transform**, base case: given two independent uses of channel $W$, combine as $x_1 = u_1\oplus u_2$, $x_2=u_2$ — a simple XOR combine. This produces two synthetic channels: $W^-$ (worse) and $W^+$ (better). **Mutual information is conserved**: $I(W^-)+I(W^+) = 2I(W)$, but strictly redistributed: $I(W^-)\le I(W) \le I(W^+)$ (strict inequality unless $W$ is already perfect or useless).

### 4. Formal definition

Recursively applying the 2×2 kernel $F=\begin{pmatrix}1&0\\1&1\end{pmatrix}$ via its $n$-th Kronecker power $G_N=F^{\otimes n}$ ($N=2^n$) to $N$ copies of the channel produces $N$ synthetic bit-channels. As $n\to\infty$, the **fraction** of synthesized channels with reliability near-perfect converges to exactly the channel capacity $I(W)$, and the fraction near-useless converges to $1-I(W)$ — a proven theorem (Arikan, via a martingale convergence argument beyond this lesson's derivation scope, stated here as an established result). What we *can* fully derive is the exact base-case splitting, which carries the entire intuition.

### 5. Derivation — exact polarization for the Binary Erasure Channel (BEC)

The BEC, with erasure probability $\varepsilon$, is the standard channel for deriving this exactly, since erasures compose with simple, exact algebra (this is the canonical pedagogical simplification used throughout the polar-coding literature). Decoding $u_1$ (the $W^-$ branch) requires **both** underlying channel uses to succeed (since $x_1=u_1\oplus u_2$ mixes both), so it erases if *either* use erases:
$$\varepsilon^- = 1-(1-\varepsilon)^2 = 2\varepsilon-\varepsilon^2$$
Decoding $u_2$ (the $W^+$ branch) happens in a successive-cancellation structure where $u_1$ is already known — giving a "second chance" via the other branch — so it erases only if **both** uses erase:
$$\varepsilon^+ = \varepsilon^2$$
**Check the conservation claim directly:** $(1-\varepsilon^-)+(1-\varepsilon^+) = (1-\varepsilon)^2 + (1-\varepsilon^2) = (1-\varepsilon)^2+(1-\varepsilon)(1+\varepsilon) = (1-\varepsilon)[(1-\varepsilon)+(1+\varepsilon)] = 2(1-\varepsilon)$ — exactly $2I(W)$, confirming conservation holds exactly, not approximately. **Check the ordering claim:** $I(W^-)=(1-\varepsilon)^2 \le 1-\varepsilon = I(W)$ always (squaring a value in $[0,1]$ shrinks it), and $I(W^+)=1-\varepsilon^2=(1-\varepsilon)(1+\varepsilon)\ge 1-\varepsilon=I(W)$ always — both inequalities verified directly from the algebra, not asserted.

### 6. Visualization

```
Polar transform (recursive Kronecker-power structure — a genuine cousin
of Lesson 3's FFT butterfly, built the same way from a small kernel):

u1 ──┬──[XOR]── x1        Recursion structure, N=4:
     │   │                [2-point combine]─┐
u2 ──┴───┴────── x2                          ├─[2-point combine]─→ 4 synthetic channels
                                [2-point combine]─┘

Polarization over levels (BEC, starting ε=0.5):
level 0:  {0.5}
level 1:  {0.75, 0.25}                        ← splitting apart
level 2:  {0.9375, 0.5625, 0.4375, 0.0625}    ← spreading toward extremes
```

### 7. Worked example

BEC with $\varepsilon=0.5$: $\varepsilon^-=2(0.5)-0.25=0.75$, $\varepsilon^+=0.25$. Apply again to each: from $\varepsilon^-=0.75$: $\varepsilon^{--}=2(0.75)-0.5625=0.9375$, $\varepsilon^{-+}=0.5625$. From $\varepsilon^+=0.25$: $\varepsilon^{+-}=2(0.25)-0.0625=0.4375$, $\varepsilon^{++}=0.0625$. After just two levels ($N=4$): $\{0.9375, 0.5625, 0.4375, 0.0625\}$ — already visibly spreading apart from the original uniform $0.5$, with real, checkable arithmetic, not merely an assertion of "polarization happens."

### 8. Code

```python
def polarize(eps_list):
    """One level of the recursive BEC split, Sec 5's exact formulas."""
    new_list = []
    for eps in eps_list:
        eps_minus = 2*eps - eps**2
        eps_plus = eps**2
        new_list.extend([eps_minus, eps_plus])
    return new_list

eps = [0.5]
for level in range(8):                 # N = 2^8 = 256
    eps = polarize(eps)

reliable = sum(1 for e in eps if e < 0.01)
unreliable = sum(1 for e in eps if e > 0.99)
print(f"N={len(eps)}  near-perfect: {reliable}  near-useless: {unreliable}")
print(f"fraction near-perfect: {reliable/len(eps):.3f}  (capacity = {1-0.5})")
```

### 9. Experiment

Run §8's code for $N=256$ (8 levels) and inspect the sorted distribution of final erasure probabilities — most should cluster very close to 0 or very close to 1, with comparatively few in between, a direct, computed demonstration of channel polarization rather than a claim to take on faith. Compare the fraction landing near 0 against the channel capacity $1-\varepsilon$.

### 10. Expected output

A visibly bimodal distribution of final erasure probabilities, clustering toward 0 and 1. The fraction of near-perfect channels should approach $1-\varepsilon$ (here, 0.5) as $N$ grows — the simulated fraction converging toward the theoretical capacity claim from §4.

### 11. Debugging exercise

Symptom, no cause given: your polar encoder/decoder works correctly for small $N$ (e.g. $N=4$), matching hand-calculated results exactly. For larger $N$, decoded BER is much worse than polar codes' known theoretical performance, even at high SNR.

Given this lesson's core derivation: what specific step, if computed incorrectly, would silently corrupt performance without causing any crash?

**Root cause (revealed):** almost certainly **incorrect frozen-bit selection** — choosing which bit positions to freeze based on a wrong reliability ordering (computed for the wrong $\varepsilon$, or a bug in the recursive §5/§8 computation itself). Get this wrong, and you silently place real information on some of the *worst* synthetic channels while freezing some of the *best* ones — a purely correctness-level bug tied directly to this lesson's central reliability computation, distinguishable from a generic decoder-implementation bug precisely because it degrades BER smoothly rather than crashing or producing garbage.

### 12. Common mistakes

- Confusing **frozen bits** (fixed, zero-information positions protecting against bad channels) with **CRC/parity bits** — real NR polar coding actually combines both (CRC-aided list decoding) for meaningfully better finite-length performance, a genuine refinement beyond this lesson's base scheme, worth naming without over-claiming full coverage of it here.
- Assuming $N$ must always be a power of 2 in practice — real NR polar codes use rate-matching/puncturing to support other block lengths, a refinement beyond this lesson's scope.
- Treating basic **successive cancellation** decoding (derived here, $O(N\log N)$ — directly the same complexity class as the FFT, for a structurally related reason) as the best practical polar decoder — real systems typically use **successive cancellation list** decoding for meaningfully better finite-length performance.

### 13. Real RF/cellular application

Polar codes are literally the channel code 3GPP selected for 5G NR's control-channel encoding (PDCCH and PBCH) — directly closing the loop with Lesson 20/29's PDCCH/DCI discussion and Lesson 19's PBCH discussion, neither of which specified how those bits are actually error-protected. This is one of the most direct "cutting-edge information theory into a deployed, billion-device standard within a decade" stories in modern engineering, and understanding channel polarization genuinely differentiates candidates in DSP/PHY algorithm-design interviews at exactly the systems-engineering and signal-processing roles flagged in this curriculum's career map.

### 14. Interview question

*"Why are polar codes described as 'provably capacity-achieving,' unlike convolutional codes decoded with Viterbi?"* Answer from §5: the polar transform provably conserves total mutual information while strictly redistributing it toward reliable and unreliable synthetic channels every time it's applied — shown via exact BEC algebra, not simulation — and this separation provably sharpens with recursion depth, with the fraction of essentially-perfect channels provably converging to exactly the channel capacity. It's a specific, provable construction, not an empirically-good heuristic.

### 15. Active recall

1. Derive $\varepsilon^-$ and $\varepsilon^+$ for a BEC from the definitions of the two synthetic channels.
2. Why is successive-cancellation decoding's $O(N\log N)$ complexity structurally related to the FFT's, rather than a coincidence?
3. What real refinement does NR add on top of the base polar scheme this lesson derives?

### 16. Mini assignment

Extend §8/§9's experiment across several base erasure probabilities $\varepsilon$, computing what fraction of $N=256$ synthetic channels end up "essentially perfect" (erasure probability below some small threshold) for each, and confirm this fraction tracks $1-\varepsilon$ reasonably closely across all of them — connecting the simulation directly to the general capacity claim, not just the one $\varepsilon=0.5$ case worked through by hand.

### 17. Exit test

1. State the two BEC polarization formulas from memory, and verify the conservation identity from them.
2. True or false, with a one-line reason: "Polar codes' successive cancellation decoder finds the exact maximum-likelihood sequence, the same guarantee Viterbi decoding provides."
3. Your diagnosis from §11 about silent frozen-bit-selection bugs, if you haven't worked through it yet.

---

**Next:** With Lessons 41 and 44 together, PBCH and PDCCH's channel coding is now fully closed. From here, the remaining open thread is turbo codes and LDPC — NR's actual data-channel code — or, if you'd rather, a return to hands-on project work now that the theory backlog is genuinely thin. Your call.

---

[← Lesson 43](lesson-43-portfolio-packaging.md) · [Table of Contents](../README.md) · [Lesson 45 →](lesson-45-ldpc-codes.md)