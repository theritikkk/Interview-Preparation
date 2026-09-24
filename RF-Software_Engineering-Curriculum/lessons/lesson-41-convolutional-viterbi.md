[← Table of Contents](../README.md) · [← Lesson 40](lesson-40-interview-synthesis.md)

---

# Lesson 41 — Convolutional Codes and Viterbi Decoding: Closing the LLR Thread

*Stage 3/9 return. Lesson 31 produced soft bits and said "ready for a channel decoder" — and left it there. This lesson builds that decoder, and its correctness proof is one of the cleanest applications of dynamic programming's optimal-substructure principle you'll ever see, directly in the style of the Cauchy-Schwarz and Cooley-Tukey proofs earlier in this book.*

### 1. Intuition

Lesson 31 produced LLRs — soft confidence values for each received bit — and explicitly deferred to "a channel decoder." Real systems don't transmit raw information bits directly; they first add structured redundancy via **channel coding**, so that even when some received bits are corrupted, the redundancy lets a receiver recover the original message reliably. A **convolutional code** is one of the most elegant ways to do this: each output bit depends not just on the current input bit but on a short, sliding window of recent input bits, via a small shift register — creating sequential, structured redundancy a receiver can exploit far more cleverly than if each bit were coded independently.

### 2. The engineering problem that forces this

Given a stream of LLRs corrupted by a noisy channel, and knowing the exact structure the encoder imposed, how do you find the **most likely** original transmitted sequence? Checking every possible sequence is exponential in message length — utterly infeasible for any real message length. You need an algorithm that finds the *exact* maximum-likelihood sequence, not an approximation, in time that scales tractably.

### 3. Mathematical foundation

A rate-1/2, constraint-length-$K$ convolutional encoder produces two output bits per input bit, each a linear (XOR) function of the current input and the encoder's $K-1$ bits of memory (its **state**). The **trellis** is a graph where nodes are (state, time) pairs and edges are valid state transitions, each labeled with the two output bits that transition produces.

### 4. Formal definition

The **branch metric** at a trellis edge measures how well that edge's expected output bits match the actually-received LLRs at that time step. The **path metric** at a (state, time) node is the cumulative sum of branch metrics along the best path reaching it. The **survivor path**: at each state and time, only the single highest-path-metric path reaching that node is retained — every other path reaching the same node is permanently discarded. This single design choice is what makes the algorithm tractable, and it's worth proving correct rather than accepting on faith.

### 5. Derivation — proving the Viterbi algorithm's optimality (the survivor-path principle)

**Claim:** for finding the maximum-likelihood *complete* path through the trellis, it suffices to keep, at each (state, time) pair, only the single best path reaching it.

**Proof.** Suppose two paths $P_1$ and $P_2$ both reach the same state $s$ at time $t$, with $P_1$'s cumulative path metric strictly greater than $P_2$'s. Consider any future extension of the trellis from $(s,t)$ to the end of the message. The branch metrics of that extension depend **only** on the current state $s$ and the future received LLRs — not on how the path arrived at $s$. This is the trellis's Markov property, and it's not an assumption; it's forced directly by the encoder itself having exactly $K-1$ bits of memory, no more — the state $s$ is, by construction, a complete summary of everything about the past that's relevant to any future transition.

So for *any* given future extension $E$, the total metric of $P_1$-then-$E$ equals ($P_1$'s metric) + ($E$'s metric), and $P_2$-then-$E$ equals ($P_2$'s metric) + ($E$'s metric) — the same $E$-contribution added to each, since $E$'s branch metrics don't depend on which path preceded it. Since $P_1$'s metric is strictly greater than $P_2$'s, $P_1$-then-$E$ is *always* strictly better than $P_2$-then-$E$, for **every possible** $E$. Therefore $P_2$ can never be part of the globally optimal complete path — it's safe to discard $P_2$ permanently, right now, without ever examining any future extension. This argument applies independently at every single (state, time) pair in the trellis — it's exactly Bellman's principle of optimal substructure, applied here to trellis decoding.

The payoff: there are only $2^{K-1}$ possible states, so total work is (number of states) × (message length) × $O(1)$ per step — **linear** in message length, not exponential. This is the entire reason Viterbi decoding is tractable at all, and it's a provable fact, not a heuristic that happens to work well in practice.

### 6. Visualization

```
Trellis (K=3, so 2^(K-1)=4 states), a few time steps:

state 00 ●───●───●───●
          ╲ ╱ ╲ ╱ ╲ ╱
state 01   ●───●───●
          ╱ ╲ ╱ ╲ ╱ ╲
state 10 ●───●───●───●
          ╲ ╱ ╲ ╱ ╲ ╱
state 11   ●───●───●
           t0  t1  t2  t3

At each node, multiple incoming paths converge — keep only the
highest-path-metric survivor, discard the rest (Sec 5's proof).
```

### 7. Worked example

A standard, simple $K=3$, rate-1/2 encoder: with state bits $(s_1,s_2)$ holding the two most recent input bits, output 1 $= x \oplus s_1 \oplus s_2$, output 2 $= x \oplus s_2$ (generator polynomials $[1,1,1]$ and $[1,0,1]$). Encoding input $1,0,1$ from the all-zero state: input 1 (state 00→10): outputs $1\oplus0\oplus0=1$, $1\oplus0=1$ → "11". Input 0 (state 10→01): outputs $0\oplus1\oplus0=1$, $0\oplus0=0$ → "10". Input 1 (state 01→10): outputs $1\oplus0\oplus1=0$, $1\oplus1=0$ → "00". Encoded stream: `11 10 00`.

### 8. Code

```python
import numpy as np
from scipy.special import erfc

G1, G2 = [1,1,1], [1,0,1]   # Sec 7's generator polynomials

def encode(bits):
    state = [0, 0]
    out = []
    for x in bits:
        s1, s2 = state
        o1 = x ^ s1 ^ s2
        o2 = x ^ s2
        out.extend([o1, o2])
        state = [x, s1]
    return out

def viterbi_decode(llrs, n_states=4):
    n_steps = len(llrs) // 2
    path_metric = {s: -np.inf for s in range(n_states)}
    path_metric[0] = 0.0
    survivors = {s: [] for s in range(n_states)}

    for t in range(n_steps):
        llr1, llr2 = llrs[2*t], llrs[2*t+1]
        new_metric = {s: -np.inf for s in range(n_states)}
        new_survivors = {s: [] for s in range(n_states)}
        for state in range(n_states):
            if path_metric[state] == -np.inf: continue
            s1, s2 = state >> 1, state & 1
            for x in (0, 1):
                o1 = x ^ s1 ^ s2
                o2 = x ^ s2
                branch = (llr1 if o1 else -llr1) + (llr2 if o2 else -llr2)  # Sec 4
                next_state = ((x << 1) | s1)
                metric = path_metric[state] + branch
                if metric > new_metric[next_state]:
                    new_metric[next_state] = metric
                    new_survivors[next_state] = survivors[state] + [x]
        path_metric, survivors = new_metric, new_survivors

    best_state = max(path_metric, key=path_metric.get)
    return survivors[best_state]
```

### 9. Experiment

Encode a random bit sequence, BPSK-modulate the coded bits, add AWGN at a chosen $E_b/N_0$ **per information bit** (accounting for the rate-1/2 code spending twice the coded-bit energy per information bit — the standard, fair way to compare coded against uncoded), compute LLRs via Lesson 31 §5's exact formula, and Viterbi-decode. Compare decoded BER against an **uncoded** BPSK system at the identical per-information-bit $E_b/N_0$ (Lesson 7's system, unmodified).

### 10. Expected output

At the same per-information-bit $E_b/N_0$, the coded system's BER should be meaningfully lower than the uncoded system's — a real, measured **coding gain**, typically several dB at moderate BER targets for even this simple code, directly reproducing why channel coding is used universally in real systems rather than being a theoretical nicety.

### 11. Debugging exercise

Symptom, no cause given: your Viterbi decoder's output matches the true transmitted sequence perfectly for most of the message, but diverges from a specific point onward — and that divergence point is consistently near the very end of the message, not scattered randomly throughout.

Using §5's proof and §7's encoder structure: what's special about the *end* of a message that could cause errors concentrated specifically there?

**Root cause (revealed):** almost certainly missing **trellis termination handling**. Real convolutional encoders are typically flushed with known "tail bits" at the end of a message, forcing the encoder back to a known (usually all-zero) state — giving the decoder a known endpoint to trace back from, rather than having to guess which final state is correct. A decoder that doesn't account for this (either the encoder didn't actually flush, or the decoder doesn't know to expect it and picks the wrong final state via `max(path_metric, ...)` when the true final state was fixed and known) will show exactly this symptom — correct decoding everywhere except near the boundary where the termination assumption breaks down. (If the divergence instead occurred at a random, non-boundary location, the more likely explanation would be a genuine, severe noise burst overwhelming the code's correction capability at that specific point — a real, expected failure mode, not a bug — distinguishable by checking whether the LLR magnitudes were unusually low specifically in that region.)

### 12. Common mistakes

- Confusing constraint length $K$ with the number of trellis states — states $=2^{K-1}$, not $2^K$, an easy, common off-by-one-in-the-exponent error.
- Using **hard** bit decisions (0/1) as decoder input instead of the **soft** LLR values directly — soft-decision Viterbi decoding provides a real, well-known coding-gain improvement (commonly cited around 2 dB) over hard-decision decoding, because converting to hard bits before decoding discards real, useful confidence information for free.
- Forgetting trellis termination handling — §11's exact failure mode.

### 13. Real RF/cellular application

Convolutional coding with Viterbi decoding was the workhorse FEC scheme for control channels and lower-rate data in 3G and early 4G systems, and remains used for certain control-channel encoding in LTE/NR today, even though turbo codes, LDPC, and polar codes (Stage 3's remaining roadmap topics) handle the main data channels in modern standards. Understanding Viterbi thoroughly is also the essential conceptual stepping stone to turbo codes' iterative BCJR/MAP decoding (a close mathematical relative) and to understanding why LDPC/polar decoding, used in NR's actual data channels, took the directions they did.

### 14. Interview question

*"Why can the Viterbi algorithm find the exact maximum-likelihood sequence in time linear in message length, when the total number of possible sequences grows exponentially?"* Answer from §5: because the trellis has the Markov property — a state fully summarizes everything about the past relevant to the future — so at each state and time, only the single best path reaching it can ever be part of the globally optimal complete path; every other path reaching that same node is provably, permanently safe to discard, collapsing an exponential search into one linear in message length times a fixed number of states.

### 15. Active recall

1. State the survivor-path principle and the one-sentence reason it's provably correct, not just empirically effective.
2. Why does soft-decision (LLR-based) decoding outperform hard-decision decoding, and by roughly how much?
3. Why does missing trellis termination specifically produce errors concentrated near the end of a message?

### 16. Mini assignment

Run §9's experiment across a sweep of $E_b/N_0$ values, plot coded and uncoded BER curves together (Lesson 7's exact plotting style), and report the coding gain in dB at a target BER of $10^{-4}$ — how many fewer dB of $E_b/N_0$ the coded system needs to reach the same BER as uncoded transmission.

### 17. Exit test

1. Write the branch-metric formula used in §8's decoder, and explain why it uses the LLR directly rather than a hard bit decision.
2. True or false, with a one-line reason: "The Viterbi algorithm is an approximation that usually finds a good, but not necessarily optimal, decoded sequence."
3. Your diagnosis from §11 distinguishing a termination bug from a genuine noise burst, if you haven't sent it.

---

**Next:** This closes a genuinely open thread from Lesson 31 and fills in the "FEC" gap flagged all the way back in Stage 3 of this curriculum's original roadmap. Lesson 42 turns to the SDR laboratory work this book's roadmap always intended alongside the theory: moving from RTL-SDR's receive-only experiments toward transmit-capable hardware, and the legal/regulatory realities that come with actually putting a signal on the air.

---

[← Lesson 40](lesson-40-interview-synthesis.md) · [Table of Contents](../README.md) · [Lesson 42 →](lesson-41-convolutional-viterbi.md)