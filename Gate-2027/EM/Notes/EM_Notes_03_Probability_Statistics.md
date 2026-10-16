# EM Notes 03 — Probability & Statistics (High-Yield Deep Dive)

> **Why this file exists:** The three probability/statistics banks (EM_06–EM_08) revolve around three computation engines — **conditional/Bayes arithmetic → standard distributions (binomial, Poisson, uniform, exponential, normal) → descriptive statistics & linear transformations**. Every value below was verified exactly (fractions) or numerically before writing; the practice questions are fresh (not in the banks) with every answer verified.
>
> **Companion question banks:** `../EM_06_Probability_Conditional_Bayes.md`, `../EM_07_Random_Variables_Distributions.md`, `../EM_08_Mean_Median_Mode_Standard_Deviation.md`. **PW source:** `../Reference/PW_Subject_Handbooks/PW_EngineeringMath.txt` §3.1–§3.8 (pages 1.29–1.40: axioms, conditional probability, total probability & Bayes, statistics, random variables, expectation/variance, distributions).

---

## 0. What GATE actually asks

```
(A) Foundations      axioms, addition theorem, independence vs mutual exclusivity
(B) Conditional      P(A|B), multiplication rule, without-replacement counts
(C) Bayes            total probability (machines/urns/tests), base-rate effect
(D) Random variables E and Var algebra, Cov/Var(X ± Y), linearity traps
(E) Distributions    binomial, Poisson, uniform, exponential, normal — means, variances, standard values
(F) Statistics       mean/median/mode, grouped data, population vs sample SD, skewness, y = ax + b
```

---

## 1. Foundations (PW §3.1–§3.3)

- Sample space S of a random experiment; an event is a subset. Axioms (PW §3.2.2): **0 ≤ P(A) ≤ 1** and **P(S) = 1**; consequences: P(∅) = 0 and **P(Aᶜ) = 1 − P(A)**.
- **Addition theorem (PW §3.3):** P(A ∪ B) = P(A) + P(B) − P(A ∩ B); for three events add the three singletons, subtract the three pairwise intersections, add back the triple intersection. Disjoint events ⇒ simply add. Monotone: A ⊆ B ⇒ P(A) ≤ P(B).
- **De Morgan (PW §3.3.1):** (A ∪ B)ᶜ = Aᶜ ∩ Bᶜ; (A ∩ B)ᶜ = Aᶜ ∪ Bᶜ.
- **Independence:** P(A ∩ B) = P(A)·P(B). **Mutually exclusive:** A ∩ B = ∅. Positive-probability exclusive events are **never** independent (0 ≠ P(A)P(B)), and independence never forces exclusivity — the two notions live in different chapters (bank MSQ staple).
- Worked micro-examples: independent union (fresh Q1): P(A ∪ B) = 0.5 + 0.4 − 0.2 = **0.7**; non-independent addition-theorem value (bank): 0.3 + 0.4 − 0.1 = 0.6.

---

## 2. Conditional probability and the multiplication rule (PW §3.3.3–§3.3.5)

- **Definition:** P(A|B) = P(A ∩ B)/P(B) for P(B) > 0.
- **Multiplication rule (PW §3.3.5):** P(A ∩ B) = P(A|B)·P(B) = P(B|A)·P(A).
- **Positive association equivalence:** P(A|B) > P(A) ⟺ P(A ∩ B) > P(A)P(B) ⟺ P(B|A) > P(B) — conditioning and reversed conditioning move together (verified on 10⁵ random event configurations; fresh Q3 (A)).
- **Within the condition:** P(A|B) + P(Aᶜ|B) = 1 (bank EM_06 Q3 (C)).
- **Splitting an event:** P(A) = P(A ∩ B) + P(A ∩ Bᶜ) — the total-probability idea in miniature (fresh Q3 (C)).
- **Without-replacement counting:** sequential denominators shrink — bank anchor: urn with 5 red, 3 blue; P(2nd red | 1st red) = 4/7; fresh Q6: P(both aces) = (4/52)(3/51) = 1/221.

---

## 3. Total probability and Bayes' theorem (PW §3.3.6–§3.3.7)

Let H₁, …, Hₙ be a **partition** of S (mutually exclusive, exhaustive):

$$P(E) = \sum_j P(H_j)\,P(E|H_j), \qquad P(H_i|E) = \frac{P(H_i)\,P(E|H_i)}{\sum_j P(H_j)\,P(E|H_j)}$$

- The denominator of Bayes **is** P(E) — the total (marginal) probability of the evidence (bank MSQ statement).
- **Tree framing:** multiply along branches (prior × likelihood), add across branches for the denominator, divide for the posterior.
- **Base-rate effect:** a rare prior can keep the posterior low even after a high-sensitivity positive test — the bank's 99%/98%/1% test gives ≈ 1/3, not ≈ 0.99 (EM_06 Q6). The posterior never simply equals sensitivity.
- **Machine template (bank EM_06 Q9):** production 30/40/30 with defect rates 2/4/3% ⇒ P(defective) = 0.6% + 1.6% + 0.9% = **0.031**, and P(M2 | defective) = 0.016/0.031 = **16/31** (demote the prior 0.4 — defectiveness pushes toward the worse machine).
- **Fresh instances:** urn Bayes = 1/3 (Q4); test Bayes = 18/67 ≈ 0.269 with P(+) = 67/1000 (Q5) — whose complement 49/67 = P(no disease | +) is the designed distractor.

---

## 4. Random variables; expectation & variance algebra (PW §3.5)

- Discrete: PMF p(x) = P(X = x), Σp = 1; continuous: PDF f(x) ≥ 0, ∫f = 1, point probabilities are 0. CDF F(x) = P(X ≤ x).
- **Expectation (PW §3.5.6):** E[X] = Σ x p(x) (or ∫ x f(x) dx). E[c] = c; E[aX] = aE[X]; **E[aX + b] = aE[X] + b**; **E[X + Y] = E[X] + E[Y] always** (linearity needs no independence). E[XY] = E[X]E[Y] **when independent** (converse false).
- **Variance (PW §3.5.5/§3.5.7):** Var(X) = E[(X − μ)²] = **E[X²] − (E[X])²** (the computational form). Var(c) = 0; **Var(aX + b) = a²·Var(X)** (shift b is irrelevant, sign is squared); Var(−X) = Var(X). General sum: Var(X + Y) = Var X + Var Y + 2Cov(X, Y); if independent, **Var(X ± Y) = Var X + Var Y** — variances *add for both signs* (PW §3.5.7 (iv)).
- **Covariance (PW §3.5.7 (vi)–(vii), §3.7.1):** Cov(X, Y) = E[XY] − E[X]E[Y]; Cov(X, X) = Var(X); Cov(aX + b, cY + d) = ac·Cov(X, Y); independence ⇒ Cov = 0; **Cov = 0 does NOT imply independence** (fresh Q7).
- **Counterexample for the converse (fresh Q7 (C)):** X uniform on {−1, 0, 1}, Y = X²: Cov = E[X³] − E[X]E[X²] = 0 − 0 = 0, yet X and Y are dependent (knowing Y = 0 forces X = 0).
- Bank joint-table anchor: P(0,0) = 0.1, P(0,1) = 0.2, P(1,0) = 0.3, P(1,1) = 0.4 ⇒ E[X] = 0.7, E[Y] = 0.6, E[XY] = 0.4 (only the (1,1) cell), Cov = −0.02 (EM_07 Q10).

---

## 5. Standard distributions (PW §3.6–§3.8)

| Distribution | pmf / pdf | Mean | Variance | Signature facts |
|---|---|---|---|---|
| Binomial(n, p) | C(n, r) pʳ q^(n−r) | np | npq | sum of n Bernoulli trials; q = 1 − p |
| Poisson(λ) | e^(−λ) λʳ / r! | λ | λ | limiting binomial (n→∞, p→0, np = λ); sums of independent Poissons are Poisson(Σλ) |
| Uniform(a, b) | 1/(b − a) on (a, b) | (a + b)/2 | (b − a)²/12 | rectangular density; probabilities = length ratios |
| Exponential(λ) | λe^(−λx), x ≥ 0 | 1/λ | 1/λ² | P(X > t) = e^(−λt); **memoryless**; sd = mean = 1/λ |
| Normal(μ, σ²) | Gaussian curve | μ | σ² | symmetric; mean = median = mode = μ; **68.26 / 95.44 / 99.73** within ±1σ / ±2σ / ±3σ |

**Normal work — standardize every time:** z = (x − μ)/σ. Symmetry identities: P(Z > z) = 1 − Φ(z); **P(−z < Z < z) = 2Φ(z) − 1**; Φ(0) = 1/2. The banks give you the Φ value to use — EM_07 uses Φ(1) = 0.8413. (PW quotes the ±1σ/±2σ mass as 0.6828/0.9544; tables often print 0.6827 — **use the number the question supplies**.)

- Binomial anchors: B(10, 0.2): P(X = 3) = C(10,3)(0.2)³(0.8)⁷ = **0.201** (EM_07 Q2).
- Poisson anchors: Poisson(3): P(X = 2) = e^(−3)·9/2 = **0.224** (EM_07 Q4); fresh: Poisson(2), P(X ≤ 1) = 3e^(−2) = **0.406** (Q8).
- Uniform: Uniform(2, 10): P(X > 6) = 4/8 = 0.5 (EM_07 Q5).
- Exponential: λ = 0.5: P(X > 4) = e^(−2) = **0.135** (EM_07 Q6); memoryless bank MSQ (only continuous memoryless law; models waiting times of a Poisson process; Var = mean²).
- Normal anchors: N(60, 100): P(X < 70) = Φ(1) = **0.8413** (EM_07 Q8); fresh: N(50, 25): P(40 < X < 60) = 2Φ(2) − 1 = **0.9544** (Q9).

---

## 6. Statistics (PW §3.4)

- **Mean** x̄ = Σxᵢ/n (grouped: Σfx/Σf); **median** = middle value (odd n) or average of the two middle values (even n; sorted first!); **mode** = most frequent value (bimodal data allowed) (PW §3.4.1–§3.4.3).
- Bank dataset {2, 4, 4, 4, 5, 5, 7, 9} (EM_08 anchor): mean 5, median 4.5, mode 4, population SD 2.0 (variance 4).
- **Grouped data:** values 10, 20, 30, 40, 50 with frequencies 2, 3, 5, 4, 1: mean = 440/15 = **29.33**, population variance = **126.22** (EM_08 Q6/Q8).
- **Population vs sample SD (PW §3.4.5):** divide squared deviations by **n** for the population, by **n − 1** for the (unbiased) sample version. Standard deviation carries the data's units; variance has squared units; range = max − min; the mean absolute deviation is not zero (Σ(xᵢ − x̄) = 0 always — PW §3.4.3).
- **Skewness (PW §3.4.5 note):** symmetric ⇒ mean = median = mode; right/positively skewed ⇒ mean > median > mode; left/negatively skewed ⇒ mean < median < mode. The median resists outliers; the mean does not (bank EM_08 Q3 (A)).
- **Linear transformations yᵢ = a·xᵢ + b (bank EM_08 Q9–Q10):** mean(y) = a·mean(x) + b; **median(y) = a·median(x) + b** (valid for a > 0, or with the order-reversal handled); **Var(y) = a²·Var(x)** and SD(y) = |a|·SD(x) — b never moves the spread. Bank killer: x-mean 5, SD 2, y = 3x − 4 ⇒ mean 11, SD 6.
- **Combined (weighted) mean:** mean = (n₁x̄₁ + n₂x̄₂)/(n₁ + n₂) — fresh Q10: (20·60 + 30·70)/50 = **66** (not 65!).

---

## 7. Trap list (read before the exam)

1. Independent vs mutually exclusive: exclusive + positive probabilities ⇒ dependent; independent + positive ⇒ non-exclusive.
2. P(A|B) ≠ P(B|A) — Bayes flips the conditioning; base-rate neglect is the classic wrong-answer generator.
3. Cov(X, Y) = 0 ⇏ independence; independence ⇒ Cov = 0 (one-way street).
4. Var(X − Y) = Var(X) + Var(Y) when independent — the **plus** surprises everyone; the minus shows up only inside 2Cov.
5. Var(aX + b) = a²·Var(X): shifts do nothing, signs get squared.
6. Population vs sample SD: divide by n vs n − 1; also SD keeps original units while variance squares them.
7. Binomial vs Poisson parameterization (np vs λ); Poisson's mean equals its variance — a property of the model, not of data.
8. Continuous variables: P(X = c) = 0; intervals can be open or closed interchangeably.
9. Normal tables: use the Φ value **given in the question** (0.8413 vs 0.6827/0.6828 convention clashes are deliberate distractors).
10. Without replacement changes denominators: P(both aces) = (4/52)(3/51) ≠ (4/52)².
11. Combined means weight by group sizes — averaging the averages is wrong (66, not 65).

---

## 8. Practice questions (fresh — not in the banks)

### Q1 · MCQ · Fundamental

A and B are independent events with P(A) = 0.5 and P(B) = 0.4. The value of P(A ∪ B) is

- (A) 0.9
- (B) 0.7
- (C) 0.6
- (D) 0.2

### Q2 · NAT · Fundamental

The **population** variance of the data set {3, 5, 7, 9, 11} is ______.

### Q3 · MSQ · Standard

Consider events with positive probabilities wherever required. Which of the following statements is/are TRUE?

- (A) P(A|B) > P(A) if and only if P(B|A) > P(B).
- (B) If A and B are mutually exclusive with P(A) > 0 and P(B) > 0, then P(A|B) = 0.
- (C) P(A ∩ B) + P(A ∩ Bᶜ) = P(A), where Bᶜ is the complement of B.
- (D) P(A|B) × P(B|A) = P(A ∩ B).

### Q4 · NAT · Standard

Urn I contains 2 red and 3 green balls; urn II contains 4 red and 1 green ball. A fair coin selects an urn, and then one ball is drawn from it: the ball is red. The probability that it came from urn I is ______ (give as a fraction).

### Q5 · MCQ · Standard

A disease has prevalence 2% in a population. A diagnostic test has sensitivity 90% (P(positive | disease) = 0.90) and specificity 95% (P(negative | no disease) = 0.95). Given that a person tests positive, the probability that the person has the disease is

- (A) 18/67
- (B) 49/67
- (C) 0.90
- (D) 0.02

### Q6 · NAT · Standard

Two cards are drawn at random, without replacement, from a standard 52-card deck. The probability that both cards are aces is ______ (give as a fraction in lowest terms).

### Q7 · MSQ · Moderate

Which of the following statements is/are TRUE (for random variables with finite variance)?

- (A) Cov(X, X) = Var(X).
- (B) If X and Y are independent, then Cov(X, Y) = 0.
- (C) If Cov(X, Y) = 0, then X and Y are independent.
- (D) Cov(aX + b, cY + d) = ac·Cov(X, Y) for constants a, b, c, d.

### Q8 · NAT · Moderate

X is a Poisson random variable with mean 2. The value of P(X ≤ 1), rounded to three decimal places, is ______.

### Q9 · NAT · Hard

X is normally distributed with mean 50 and variance 25. For the standard normal variable Z, P(Z < 2) = 0.9772. The value of P(40 < X < 60), rounded to four decimal places, is ______.

### Q10 · NAT · Killer

The average score of 20 students in section A is 60, and the average score of 30 students in section B is 70. The average score of all 50 students taken together is ______.

---

## 9. Answer key & full solutions

**Answer key:** 1-B · 2-8 · 3-A,B,C · 4-1/3 · 5-A · 6-1/221 · 7-A,B,D · 8-0.406 · 9-0.9544 · 10-66

**Q1 — B.** Independence: P(A ∩ B) = 0.5 × 0.4 = 0.2, so P(A ∪ B) = 0.5 + 0.4 − 0.2 = **0.7**. (A) 0.9 forgets to subtract the overlap; (D) 0.2 is the intersection, not the union.

**Q2 — 8.** Mean = (3 + 5 + 7 + 9 + 11)/5 = 7; squared deviations 16, 4, 0, 4, 16 sum to 40; population variance = 40/5 = **8** (the sample version would give 40/4 = 10 — read the word "population").

**Q3 — A, B, C.** (A) ✓ P(A|B) > P(A) ⟺ P(A ∩ B) > P(A)P(B) ⟺ P(B|A) > P(B), dividing by the positive probabilities (verified on 10⁵ random configurations). (B) ✓ exclusive ⇒ A ∩ B = ∅ ⇒ P(A|B) = 0/P(B) = 0. (C) ✓ splits A according to whether B occurs. (D) ✘ — dice, A = {2}, B = {even}: P(A ∩ B) = 1/6 but P(A|B)·P(B|A) = (1/3)(1) = 1/3.

**Q4 — 1/3.** P(red) = ½·(2/5) + ½·(4/5) = 1/5 + 2/5 = 3/5. Posterior P(urn I | red) = (1/5)/(3/5) = **1/3**. (Bayes with a fair prior: the likelihood ratio 2/5 : 4/5 = 1 : 2 splits probability 1/3 : 2/3.)

**Q5 — A.** P(+) = 0.02 × 0.90 + 0.98 × 0.05 = 0.018 + 0.049 = 0.067 = 67/1000 (exact fractions: 9/500 + 49/1000). So P(disease | +) = 0.018/0.067 = **18/67** ≈ 0.269. (B) 49/67 is P(no disease | +) — the complement, and the base-rate lesson: a 90%-sensitive test still leaves a 73% chance of health at 2% prevalence. (C) is the sensitivity; (D) is the prior.

**Q6 — 1/221.** (4/52)·(3/51) = 12/2652 = **1/221** ≈ 0.0045. (The naive (4/52)² overcounts by ignoring that the first ace is not replaced.)

**Q7 — A, B, D.** (A) ✓ Cov(X, X) = E[X²] − (E[X])² = Var(X). (B) ✓ independence gives E[XY] = E[X]E[Y]. (D) ✓ bilinearity of covariance (verified numerically). (C) ✘ — X uniform on {−1, 0, 1} and Y = X²: E[X] = 0, E[X³] = 0, so Cov = 0, yet Y is fully determined by X (verified).

**Q8 — 0.406.** P(X ≤ 1) = P(0) + P(1) = e^(−2)(1 + 2) = 3e^(−2) = 3 × 0.135335 = 0.406006 ≈ **0.406**.

**Q9 — 0.9544.** Standardize: (40 − 50)/5 = −2 and (60 − 50)/5 = 2, so the probability is P(−2 < Z < 2) = 2Φ(2) − 1 = 2(0.9772) − 1 = **0.9544** — exactly the handbook's 95.44% ±2σ mass.

**Q10 — 66.** Combined mean = (20 × 60 + 30 × 70)/50 = (1200 + 2100)/50 = 3300/50 = **66**. The classic trap is (60 + 70)/2 = 65, which ignores that section B is 50% larger.

---

## 10. Next steps

1. Rebuild the Bayes tree for one machine problem, one urn problem and one test problem from scratch — the same three-branch arithmetic covers every variant.
2. Memorize the distribution table's mean/variance column as a block, plus the standard facts: memoryless exponential, Poisson mean = variance, normal 68/95/99.7.
3. Practise the statistics transformations y = ax + b until mean/median/SD rules are reflex, and always check denominator n vs n − 1.
4. Run banks `EM_06`–`EM_08` cold. This completes the full high-yield notes set — the subject banks are the final checkpoint before revision.
