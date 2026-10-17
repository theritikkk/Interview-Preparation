[← Table of Contents](../README.md) · [← Lesson 54](lesson-54-outer-loop-link-adaptation.md)

---

# Lesson 55 — PRACH Contention: Sizing the Preamble Pool with Slotted-ALOHA Math

*Lesson 30 made a single random-access attempt detectable and ranged. It left one question open: what happens when several UEs attempt at once and pick the same preamble? The answer is a counting argument, and it yields the classic 1/e limit.*

### 1. Intuition

In contention-based random access, each UE that wants to connect picks a preamble at random from a shared pool. The gNB hears a preamble and replies, but if two UEs picked the *same* one it cannot tell them apart; they receive the same response and collide later at the next step. A pool of preambles therefore behaves like a set of slots in slotted ALOHA. How many preambles does a cell need so that most attempts succeed, and how efficiently can a pool ever be used?

### 2. The engineering problem that forces this

Given $K$ UEs attempting in the same RACH occasion and a pool of $N$ contention-based preambles, quantify the collision probability and the expected number of successful attempts, and derive the best operating load and the pool size needed for a target reliability.

### 3. Mathematical foundation

Each UE picks one of $N$ preambles independently and uniformly at random. A UE succeeds (at this step) if no other UE picked its preamble.

### 4. Formal definition

**Offered load** $\lambda=K/N$ (attempts per preamble). **Per-UE success probability** $s$. **Pool throughput** $T$: expected number of successful attempts per preamble per occasion.

### 5. Derivation

**(a) Exact result for $K$ attempts.** The other $K-1$ UEs each avoid your preamble with probability $1-1/N$, independently, so
$$s=\Big(1-\tfrac1N\Big)^{K-1},\qquad P(\text{collision})=1-\Big(1-\tfrac1N\Big)^{K-1}.$$
The expected number of preambles chosen by exactly one UE is $N\cdot K\cdot\frac1N\big(1-\frac1N\big)^{K-1}=K\,s$, consistent with $K$ UEs each succeeding with probability $s$.

**(b) The exponential form.** For large $N$, $\big(1-\frac1N\big)^{K-1}\approx e^{-K/N}=e^{-\lambda}$. If instead the number of attempts per occasion is Poisson with mean $\mu$, thinning gives the number choosing any one preamble as Poisson$(\mu/N)$, and a given attempt succeeds exactly when no *other* attempt lands on its preamble:
$$s=e^{-\mu/N}\quad\text{(exact for Poisson arrivals).}$$

**(c) Best operating load: the 1/e limit.** Throughput per preamble is $T(\lambda)=\lambda e^{-\lambda}$. Setting $T'(\lambda)=e^{-\lambda}(1-\lambda)=0$ gives $\lambda=1$, with $T_{max}=1/e\approx0.368$ — the classical slotted-ALOHA ceiling. At that load, each UE succeeds with probability only $e^{-1}\approx37\%$. Efficiency and per-UE reliability pull in opposite directions: running near $\lambda=1$ uses the pool best and serves each UE worst.

**(d) Pool size for a reliability target.** Requiring $s\ge1-\varepsilon$ means $e^{-K/N}\ge1-\varepsilon$, so
$$N\ge\frac{K}{\ln\!\big(1/(1-\varepsilon)\big)}\approx\frac{K}{\varepsilon}\quad(\varepsilon\text{ small}).$$
For $\varepsilon=1\%$: $N\gtrsim99.5\,K$ — about a hundred preambles per simultaneous attempt. A cell cannot get that from one occasion's pool of 64, so the real levers are *fewer attempts per occasion* (more RACH occasions in time and frequency, which in NR are also associated with beams) and backoff that spreads retries.

### 6. Visualization

```
N = 8 preambles, K = 5 UEs pick at random:

preamble:  0   1   2   3   4   5   6   7
UEs:       .   A   B,E .   C   .   D   .
                    ^collision        ^ singletons: A, C, D succeed (3 of 5)

Throughput T(λ)=λe^(-λ):
 0.37 |            ****
      |        ****    ****
      |     ***            ***
      |  ***                  ****
      +--------------------------------> λ
         0       1 (max)        3
```

### 7. Worked example

$N=64$. With $K=4$: $s=(63/64)^3\approx0.954$, collision $\approx4.6\%$. With $K=16$: $s=(63/64)^{15}\approx0.790$, collision $\approx21\%$. With $K=64$ ($\lambda=1$): $s\approx0.371$, matching $e^{-1}\approx0.368$. Expected attempts until success, treating each attempt as independent, is $1/s=e^{\lambda}$: about 1.05 at $\lambda=0.0625$, but 2.7 at $\lambda=1$ — and in a real system retries add to the load, which pushes $\lambda$ up further.

### 8. Code

```python
import numpy as np

def exact_success(N, K):
    return (1 - 1 / N) ** (K - 1)

def simulate_success(N, K, trials=20000, seed=0):
    rng = np.random.default_rng(seed)
    succ = 0
    for _ in range(trials):
        counts = np.bincount(rng.integers(0, N, K), minlength=N)
        succ += np.sum(counts == 1)           # preambles chosen by exactly one UE
    return succ / (trials * K)                # fraction of attempts that succeed

N = 64
for K in [4, 16, 32, 64]:
    print(f"N={N} K={K:3d}  exact={exact_success(N,K):.3f}  "
          f"sim={simulate_success(N,K):.3f}  e^-lambda={np.exp(-K/N):.3f}")

# Throughput peak at lambda = 1
lams = np.linspace(0.05, 3, 300)
T = lams * np.exp(-lams)
print("max throughput at lambda =", lams[np.argmax(T)], " T =", T.max(), " (1/e =", 1/np.e, ")")

# Sec 11: bursty vs smooth load with the SAME mean load
def success_fraction(lambdas):
    lambdas = np.asarray(lambdas)
    return np.mean(lambdas * np.exp(-lambdas)) / np.mean(lambdas)
print("smooth load  lambda=1.0:       ", success_fraction([1.0, 1.0]))
print("bursty 0.1 / 1.9 (mean 1.0):   ", success_fraction([0.1, 1.9]))
```

### 9. Experiment

Run §8. Confirm the exact formula, simulation, and $e^{-\lambda}$ agree closely, and that throughput peaks at $\lambda=1$ with value $\approx0.368$. Then compare the last two lines: the same *mean* load produces very different success fractions depending on how that load is distributed over occasions.

### 10. Expected output

Approximately: $K=4$: 0.954; $K=16$: 0.790 (exact) versus 0.791 ($e^{-\lambda}$); $K=32$: 0.624; $K=64$: 0.371 versus 0.368. Throughput peak at $\lambda\approx1.0$, $T\approx0.368$. Smooth load gives 0.368; the bursty 0.1/1.9 mix gives about **0.187** — roughly half.

### 11. Debugging exercise

Symptom, no cause given: a network's measured random-access success rate is consistently about half of what the $e^{-\lambda}$ formula predicts from the *average* attempt rate per occasion, with no sign of any bug in preamble generation or detection.

Using §5 and the last line of §8: what assumption about the arrival process is the formula making, and how could real traffic violate it?

**Root cause (revealed):** the formula assumes attempts spread evenly (Poisson with a steady mean) across occasions. Real arrivals are **bursty and synchronized**: a train pulling in, a stadium emptying, recovery after an outage, or a paging storm drives many UEs to attempt in the same occasions while neighboring occasions sit nearly empty. Because the success probability $e^{-\lambda}$ falls *exponentially* with load, the occasions with high $\lambda$ dominate the failures, and most UEs experience those crowded occasions. Using the numbers from §10, a 0.1/1.9 mix with mean 1.0 gives 0.187 rather than 0.368. The formula is exact for its assumption; it is the average load that was the wrong input. Diagnose by looking at the *distribution* of attempts per occasion, not just its mean.

### 12. Common mistakes

- Sizing the pool from the average attempt rate. The right input is a high percentile of simultaneous attempts, for the reason in §11.
- Ignoring retries: each failed UE comes back, raising the offered load, which raises collisions, which creates more retries — a feedback that can push the system past its stable operating point. Backoff exists to break it.
- Treating "preamble collision" as always fatal. Contention resolution (the later steps of random access) and capture effects can rescue some collisions, so the formula is a pessimistic bound on first-step failure rather than the final access failure rate.

### 13. Real RF/cellular application

Random-access dimensioning — how many RACH occasions per frame, how many contention-based preambles versus reserved contention-free ones, what backoff to broadcast — is a real planning decision, and RACH success rate is a routinely monitored KPI. Burst scenarios (events, transport hubs, post-outage recovery) are exactly where cells see failure spikes, which makes this a natural application of the multi-layer trace methodology from Lesson 38.

### 14. Interview question

*"A cell has 64 contention-based preambles. What is the best simultaneous load for the pool, and why would you not run it there?"* Answer from §5: pool throughput $\lambda e^{-\lambda}$ peaks at $\lambda=1$ with value $1/e\approx37\%$, but at that load each attempt succeeds only 37% of the time, so access latency (mean $e^{\lambda}$ attempts) and retry-driven load growth make it a poor operating point. Real systems run at low $\lambda$ and add occasions to keep the per-occasion load small.

### 15. Active recall

1. Derive the per-UE collision probability and the large-$N$ exponential approximation.
2. Prove that $\lambda e^{-\lambda}$ is maximized at $\lambda=1$ with value $1/e$.
3. Why does a bursty load with the same mean produce a lower success rate?

### 16. Mini assignment

Extend §8 into a retry simulation: UEs that collide retry after a uniformly random backoff of $b$ occasions. Sweep the base arrival rate and find the load at which the backlog of retrying UEs stops draining (the instability point), then show how increasing the backoff window or the number of occasions moves it.

### 17. Exit test

1. Compute the per-UE success probability for $N=54$ preambles and $K=10$ attempts, exactly and with $e^{-\lambda}$.
2. True or false, with a one-line reason: "Doubling the number of preambles halves the collision probability."
3. Your reasoning from §11 about bursty arrivals, if you haven't worked through it yet.

---

**Next:** Lesson 56 crosses into territory you already know from the backend side: **TCP over a cellular link** — how HARQ and RLC retransmission delay inflates and destabilizes RTT, and why TCP's retransmission-timeout estimator can misfire on a radio link.

---

← [Lesson 54](lesson-54-outer-loop-link-adaptation.md) · [Table of Contents](../README.md)
