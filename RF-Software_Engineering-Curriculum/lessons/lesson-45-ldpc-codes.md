[← Table of Contents](../README.md) · [← Lesson 44](lesson-44-polar-codes.md)

---

# Lesson 45 — LDPC Codes: Belief Propagation and NR's Actual Data-Channel Code

*Closes the final major coding-theory gap. Lesson 44 closed NR's control-channel code (polar). This lesson closes NR's actual data-channel code (PDSCH/PUSCH) — and it works via a completely different paradigm: not a clean recursive transform, but an iterative, graphical message-passing algorithm running on real modem hardware in your phone right now.*

### 1. Intuition

Lesson 44 closed the loop on NR's *control*-channel coding. This lesson closes the loop on NR's actual *data*-channel coding — LDPC (Low-Density Parity-Check) codes. Unlike polar codes' clean algebraic recursion, LDPC codes are defined by a **sparse** parity-check matrix — a graph connecting bits to parity constraints — and decoded via **belief propagation**: bit-nodes and constraint-nodes repeatedly exchange probabilistic messages until they converge on a consistent, likely codeword. This is a genuinely different algorithmic paradigm from anything else in this curriculum, and it's the exact mechanism protecting your phone's actual 5G data throughput at this moment.

### 2. The engineering problem that forces this

Given a sparse parity-check matrix $H$ (valid codewords $c$ satisfy $Hc=0 \pmod 2$) and a stream of noisy LLRs, how do you efficiently find a likely codeword satisfying every parity constraint, without exhaustively checking an exponential number of candidates? The bipartite graph structure — variable nodes for bits, check nodes for parity constraints, edges only where $H$ has a 1 (hence *sparse*) — enables an efficient, local, iterative message-passing algorithm.

### 3. Mathematical foundation

The **Tanner graph**: a bipartite graph representation of $H$. **Belief propagation (sum-product algorithm)**: each variable node sends its current LLR belief to connected check nodes; each check node computes an updated belief for each connected variable, using messages from every *other* connected variable (never itself), and sends it back; repeat for several iterations.

### 4. Formal definition

A **check node** of degree $d$ enforces that the XOR of its $d$ connected bits equals 0. The **check-to-variable message** for a specific connected variable is computed from the incoming LLRs of the *other* $d-1$ connected variables — the mathematical heart of the whole algorithm, derived properly in §5 rather than quoted.

### 5. Derivation — the exact LLR combination rule under XOR, proven from first principles

For a bit $b$ with $p=P(b{=}1)$ and $L=\ln\frac{P(b=0)}{P(b=1)}$, define $x = P(b{=}0)-P(b{=}1) = 1-2p$. Using $\tanh(z/2)=\frac{e^z-1}{e^z+1}$ (the standard half-angle identity) with $z=L$, and $e^L=(1-p)/p$: $\tanh(L/2) = \frac{(1-p)/p - 1}{(1-p)/p+1} = \frac{1-2p}{1} = 1-2p = x$. So $x=\tanh(L/2)$ exactly — a clean, verified identity, not assumed.

Now compute $x$ for the XOR of two independent bits $b_1,b_2$: $P(b_1{\oplus}b_2{=}0) = (1-p_1)(1-p_2)+p_1p_2$ (both 0 or both 1), $P(b_1{\oplus}b_2{=}1)=p_1(1-p_2)+p_2(1-p_1)$. Subtracting:
$$x_{1\oplus2} = \big[(1-p_1)(1-p_2)+p_1p_2\big] - \big[p_1(1-p_2)+p_2(1-p_1)\big] = (1-2p_1)(1-2p_2) = x_1 x_2$$
(direct algebraic expansion, fully verifiable by hand). So $x_{1\oplus2}=x_1x_2$, and since $x_i=\tanh(L_i/2)$:
$$\tanh(L_{1\oplus2}/2) = \tanh(L_1/2)\cdot\tanh(L_2/2)$$
— the exact **tanh rule**, proven, not quoted. It generalizes by induction to any degree-$d$ check: $\tanh(L_{out}/2) = \prod_j \tanh(L_j/2)$ over the other $d-1$ connected variables.

**The practical problem, and why min-sum exists.** The exact tanh rule requires a transcendental function evaluation per message, per edge, per iteration — a real, significant computational cost at the throughputs and iteration counts LDPC decoding runs at, exactly the kind of real-time cost concern Lessons 22–27 built a whole toolkit around. Practical decoders use the **min-sum approximation** instead: $\text{sign}(L_{out}) = \prod_j \text{sign}(L_j)$, $|L_{out}| \approx \min_j |L_j|$. This tracks the exact rule closely because $\tanh$ saturates toward $\pm1$ for large $|L|$ — the product in the exact rule is dominated by the smallest-magnitude term, which is exactly what min-sum extracts directly, at a fraction of the computational cost.

### 6. Visualization

```
Tanner graph (bipartite: variable nodes ↔ check nodes, edges = nonzero H entries):

variable:  v1   v2   v3   v4   v5   v6
            \  / \  /  \  |  /
             \/    \/    \ | /
check:       c1    c2     c3

Messages flow both directions each iteration:
v→c: "here's my current belief, given everything EXCEPT this one check"
c→v: "here's what I infer about you, given everything the OTHER connected variables say"
```

### 7. Worked example

Three variables connected to one check, LLRs $L_1=2.0, L_2=1.0, L_3=0.5$. Exact tanh rule for the message to variable 1 (combining $L_2,L_3$): $\tanh(L_{out}/2)=\tanh(0.5)\tanh(0.25)\approx0.4621\times0.2449\approx0.1132$, so $L_{out}\approx2\tanh^{-1}(0.1132)\approx0.2274$. Min-sum approximation: sign is positive (both positive), magnitude $=\min(1.0,0.5)=0.5$. The approximation ($0.5$) is in the same ballpark as the exact value ($0.227$) but not identical — a real, small, well-characterized gap, exactly what §16 asks you to quantify properly.

### 8. Code

```python
import numpy as np

# Small example H (3 checks, 6 variables), each row a parity constraint
H = np.array([
    [1,1,0,1,0,0],
    [0,1,1,0,1,0],
    [1,0,0,1,1,1],
])

def min_sum_decode(llr, H, max_iters=20):
    n_checks, n_vars = H.shape
    msg_v2c = np.tile(llr, (n_checks, 1)) * H
    for it in range(max_iters):
        msg_c2v = np.zeros_like(msg_v2c)
        for c in range(n_checks):
            vars_in_check = np.nonzero(H[c])[0]
            for v in vars_in_check:
                others = [msg_v2c[c, u] for u in vars_in_check if u != v]
                sign = np.prod(np.sign(others))
                mag = np.min(np.abs(others))
                msg_c2v[c, v] = sign * mag                    # Sec 5's min-sum rule
        total = llr + msg_c2v.sum(axis=0)
        decoded = (total < 0).astype(int)
        if np.all((H @ decoded) % 2 == 0):                     # all checks satisfied
            return decoded, it+1
        for v in range(n_vars):
            checks_of_v = np.nonzero(H[:, v])[0]
            for c in checks_of_v:
                msg_v2c[c, v] = llr[v] + sum(msg_c2v[cc, v] for cc in checks_of_v if cc != c)
    return decoded, max_iters

llr = np.array([2.0, -1.5, 1.0, -0.5, 2.5, -1.0])   # noisy LLRs for a 6-bit codeword
decoded, iters = min_sum_decode(llr, H)
print("decoded:", decoded, " converged in", iters, "iterations")
```

### 9. Experiment

Run §8's decoder on a codeword you construct to actually satisfy $H$ (encode first, then add simulated LLR noise), across a range of noise levels. At moderate noise, confirm it converges to the correct codeword within a handful of iterations. Then sweep SNR and plot decoded BER — LDPC's signature behavior is a sharp **waterfall**: reliable convergence below a threshold SNR, a comparatively abrupt failure above it, distinct from the smoother BER curves of uncoded BPSK back in Lesson 7.

### 10. Expected output

Correct convergence at moderate-to-good SNR, typically within under 10–20 iterations for a well-conditioned code. The BER-vs-SNR curve should show a visibly sharper knee than Lesson 7's smooth $Q$-function curve — the real, well-documented LDPC waterfall effect, predictable in principle via density evolution (a more advanced analysis beyond this lesson's derivation scope, but the qualitative shape is directly observable in your own simulation).

### 11. Debugging exercise

Symptom, no cause given: your decoder sometimes converges — every parity check exactly satisfied, $Hc=0$ precisely — to a codeword that is **not** the actual transmitted one.

Is this necessarily a bug? Using §5's derivation as your guide: what does the exact tanh rule (and its min-sum approximation) actually assume about the graph structure, and what happens when that assumption doesn't strictly hold?

**Root cause (usually not a bug at all):** belief propagation's message-passing independence assumptions are exactly valid only on a **cycle-free** (tree-structured) graph. Real sparse LDPC Tanner graphs generally contain **short cycles**, where messages can reinforce their own earlier beliefs rather than genuinely independent information — a well-documented phenomenon called a **trapping set** in the LDPC literature, which can cause convergence to a valid-but-wrong codeword. Distinguish this from an actual implementation bug by checking whether the wrong convergence varies with noise realization and initialization (consistent with a real trapping-set event) versus showing a deterministic, reproducible pattern regardless of input (which would point to an actual code defect instead).

### 12. Common mistakes

- Implementing the exact tanh rule naively in a real-time-constrained decoder without considering min-sum's necessity — directly Lessons 22–27's real-time computational-cost theme, now in a new context.
- Assuming belief propagation always converges to the true maximum-likelihood codeword — it's an *approximate* inference algorithm, exact only on cycle-free graphs, which real LDPC Tanner graphs are not; §11's trapping-set phenomenon is the real, expected consequence.
- Confusing "sparse" ($H$ mostly zeros — the property that makes per-iteration message passing computationally tractable) with "small" — real NR LDPC codes have large block lengths, just sparse connectivity.

### 13. Real RF/cellular application

LDPC codes are, right now, protecting the actual downlink (PDSCH) and uplink (PUSCH) data in every 5G NR connection — chosen specifically for excellent performance at the very high throughputs 5G targets, and for a decoding structure that parallelizes well (each check/variable update is largely independent), suiting both the SIMD hardware from Lesson 26 and dedicated LDPC decoder accelerator hardware in real modem chips. Understanding the exact-versus-min-sum trade-off is genuine, valuable, ASIC-adjacent PHY engineering knowledge.

### 14. Interview question

*"Why do practical LDPC decoders almost universally use the min-sum approximation instead of the mathematically exact tanh rule?"* Answer from §5: the exact rule needs a transcendental function evaluation per message, per edge, per iteration — a real, significant cost at LDPC's operating throughputs and iteration counts. Min-sum approximates it with a sign product and a min operation, tracking the exact rule's behavior closely (since tanh saturates for large $|L|$, the smallest-magnitude term dominates the exact product anyway) while trading a small, well-characterized performance loss for a large, necessary computational saving.

### 15. Active recall

1. Derive the tanh-rule identity for two bits' XOR from the definitions of $L$ and $x$, from memory.
2. Why does min-sum track the exact tanh rule's behavior reasonably well despite being a much cruder computation?
3. What structural property of a Tanner graph causes trapping sets, and why does it break BP's exactness?

### 16. Mini assignment

Implement both the exact tanh-rule check-node update and the min-sum approximation in your decoder, run both across a range of SNRs on the same noisy codewords, and quantify the BER gap between them — turning "min-sum is a good, cheap approximation" from an assertion into a number you measured yourself.

### 17. Exit test

1. Write the exact tanh-rule LLR combination formula and the min-sum approximation, and state which is used in practice and why.
2. True or false, with a one-line reason: "Belief propagation is guaranteed to converge to the maximum-likelihood codeword for any parity-check matrix."
3. Your reasoning from §11 distinguishing a trapping set from a genuine decoder bug, if you haven't worked through it yet.

---

**Next:** With Viterbi (Lesson 41), polar (Lesson 44), and LDPC (Lesson 45) all covered, the coding-theory arc is now genuinely complete — you have the actual codes protecting every major LTE/NR channel, each with a proven or derived correctness argument behind it. The theory backlog from this curriculum's original roadmap is thin. From here it's genuinely your call: deeper into any stage, a project built out in full, more interview drilling, or a fresh direction entirely.

---

[← Lesson 44](lesson-44-polar-codes.md) · [Table of Contents](../README.md) · [Lesson 46 →](lesson-46-beamforming-mrt.md)