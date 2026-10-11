# Engineering Mathematics → Random Variables & Standard Distributions

**GATE 2027 syllabus:** Engineering Mathematics — Probability and Statistics: random variables, uniform, normal, exponential, Poisson and binomial distributions.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** Binomial(n,p): P(X=k) = C(n,k)pᵏ(1−p)ⁿ⁻ᵏ, mean np, variance np(1−p). Poisson(λ): P(X=k) = λᵏe⁻λ/k!, mean and variance both λ. Uniform(a,b): mean (a+b)/2, variance (b−a)²/12. Exponential(λ): P(X>t) = e⁻λt, mean 1/λ. Standard normal Z has P(Z<1.0) ≈ 0.8413.

---

## Questions

### Q1 · MCQ · Fundamental
For X ~ Binomial(n, p), the mean E[X] is

- (A) p
- (B) np
- (C) np(1−p)
- (D) n/p

---

### Q2 · NAT · Fundamental
For X ~ Binomial(10, 0.2), the value of P(X = 3), rounded to three decimal places, is ______.

---

### Q3 · MSQ · Standard
Which of the following statements is/are TRUE?

- (A) For X ~ Poisson(λ), the mean and variance are both equal to λ.
- (B) The Poisson distribution can be used as an approximation to the Binomial(n,p) distribution when n is large and p is small, with λ = np.
- (C) For X ~ Binomial(n,p), Var(X) = np(1−p), which is always ≤ E[X] = np.
- (D) The sum of two independent Poisson random variables with parameters λ₁ and λ₂ is itself Poisson with parameter λ₁ + λ₂.

---

### Q4 · NAT · Standard
For X ~ Poisson(3), the value of P(X = 2), rounded to three decimal places, is ______.

---

### Q5 · MCQ · Standard
For X ~ Uniform(2, 10), the value of P(X > 6) is

- (A) 0.2
- (B) 0.4
- (C) 0.5
- (D) 0.6

---

### Q6 · NAT · Moderate
For X ~ Exponential(λ = 0.5), the value of P(X > 4), rounded to three decimal places, is ______.

---

### Q7 · MSQ · Moderate
Let Z be a standard normal random variable, with P(Z < 1.0) ≈ 0.8413. Which of the following statements is/are TRUE (using this value and symmetry of the normal distribution)?

- (A) P(Z > 1.0) ≈ 0.1587.
- (B) P(−1.0 < Z < 1.0) ≈ 0.6827.
- (C) P(Z < −1.0) ≈ 0.1587.
- (D) The normal distribution is symmetric about its mean, so P(Z < 0) = 0.5 exactly.

---

### Q8 · NAT · Moderate
X ~ Normal(60, 100) (mean 60, variance 100, so standard deviation 10). Using P(Z < 1.0) ≈ 0.8413, the value of P(X < 70), rounded to four decimal places, is ______.

---

### Q9 · MSQ · Hard
Which of the following statements about the **exponential distribution** is/are TRUE?

- (A) It has the memoryless property: P(X > s+t | X > s) = P(X > t) for all s, t ≥ 0.
- (B) It is the only continuous distribution with the memoryless property.
- (C) It is commonly used to model the waiting time between events in a Poisson process.
- (D) Its variance equals the square of its mean, i.e., Var(X) = (1/λ)² = (E[X])².

---

### Q10 · NAT · Killer
A discrete joint distribution of (X, Y) is given by P(0,0) = 0.1, P(0,1) = 0.2, P(1,0) = 0.3, P(1,1) = 0.4 (each Xᵢ, Yⱼ ∈ {0,1}). The covariance Cov(X, Y) = E[XY] − E[X]E[Y], rounded to two decimal places, is ______  (give as a negative number if applicable, e.g. −0.05).

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | B | Standard binomial mean formula |
| 2 | 0.201 | C(10,3)(0.2)³(0.8)⁷ |
| 3 | A, B, C, D | All four are standard, correct facts (for C: since 1−p ≤ 1, np(1−p) ≤ np always) |
| 4 | 0.224 | 3²e⁻³/2! |
| 5 | C | (10−6)/(10−2) |
| 6 | 0.135 | e^(−0.5×4) = e⁻² |
| 7 | A, B, C, D | All four follow directly from symmetry and the given value |
| 8 | 0.8413 | (70−60)/10 = 1.0, so P(X<70) = P(Z<1.0) |
| 9 | A, B, C, D | All four are standard, correct facts about the exponential distribution |
| 10 | -0.02 | E[X]=0.7, E[Y]=0.6, E[XY]=0.4 (only the (1,1) term contributes); Cov = 0.4 − 0.42 |
