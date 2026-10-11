# Engineering Mathematics → Limits, Continuity & Differentiability

**GATE 2027 syllabus:** Engineering Mathematics — Calculus: limits, continuity and differentiability.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** All functions are real-valued. A function is continuous at a point if the left limit, right limit and function value all agree there. Differentiability at a point requires the left-hand and right-hand derivatives to be equal (and finite). Differentiability at a point implies continuity there, but not conversely.

---

## Questions

### Q1 · MCQ · Fundamental
Which one of the following is the value of $\lim_{x \to 0} \dfrac{\sin x}{x}$?

- (A) 0
- (B) 1
- (C) ∞
- (D) The limit does not exist

---

### Q2 · NAT · Fundamental
The value of $\lim_{x \to 2} \dfrac{x^2 - 4}{x - 2}$ is ______.

---

### Q3 · MSQ · Standard
Which of the following statements is/are TRUE?

- (A) If f is differentiable at a point c, then f is continuous at c.
- (B) If f is continuous at a point c, then f is differentiable at c.
- (C) f(x) = |x| is continuous everywhere but not differentiable at x = 0.
- (D) A function can be continuous at a point without the limit existing there.

---

### Q4 · NAT · Standard
The value of $\lim_{x \to 0} \dfrac{1 - \cos x}{x^2}$ is ______  (give as a fraction or decimal, e.g. 0.5).

---

### Q5 · MCQ · Standard
The value of $\lim_{x \to \infty} \left(1 + \dfrac{1}{x}\right)^x$ is

- (A) 1
- (B) e
- (C) ∞
- (D) 0

---

### Q6 · NAT · Moderate
The value of $\lim_{x \to 0} \dfrac{\tan x - \sin x}{x^3}$ is ______  (give as a fraction or decimal).

---

### Q7 · MSQ · Moderate
Consider $f(x) = |x^2 - 4|$. Which of the following statements is/are TRUE?

- (A) f is continuous everywhere on ℝ.
- (B) f is differentiable everywhere on ℝ.
- (C) f fails to be differentiable at exactly two points, x = 2 and x = −2.
- (D) At x = 2, the left-hand derivative and right-hand derivative of f have opposite signs.

---

### Q8 · NAT · Moderate
The function
$$f(x) = \begin{cases} \dfrac{\sin(3x)}{x}, & x \neq 0 \\ k, & x = 0 \end{cases}$$
is continuous at x = 0 for exactly one value of k. That value is ______.

---

### Q9 · MSQ · Hard
Which of the following statements about limits and continuity is/are TRUE?

- (A) If $\lim_{x\to c} f(x)$ exists but does not equal f(c), then f has a **removable** discontinuity at c.
- (B) If the left-hand limit and right-hand limit at c both exist but are unequal, f has a **jump** discontinuity at c.
- (C) Every function that is bounded on a closed interval [a,b] is necessarily continuous on [a,b].
- (D) The composition of two continuous functions is continuous (where the composition is defined).

---

### Q10 · NAT · Killer
The value of $\lim_{x \to \infty} x\left(\sqrt{x^2+1} - x\right)$ is ______  (give as a fraction or decimal).

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | B | Standard fundamental limit |
| 2 | 4 | Factor and cancel: (x+2)(x−2)/(x−2) → x+2 → 4 |
| 3 | A, C, D | (B) is false — |x| is the classic counterexample: continuous but not differentiable at 0 |
| 4 | 0.5 | Standard limit, verified by L'Hôpital or Taylor expansion |
| 5 | B | Definition of e as a limit |
| 6 | 0.5 | Verified via Taylor expansion / L'Hôpital: tan x − sin x ≈ x³/2 for small x |
| 7 | A, C, D | (B) is false — f has corners at x = ±2 where the absolute value changes the sign of the inner derivative |
| 8 | 3 | limₓ→₀ sin(3x)/x = 3 (by the standard sin(u)/u → 1 substitution with u = 3x) |
| 9 | A, B, D | (C) is false — a bounded function need not be continuous, e.g. a step function |
| 10 | 0.5 | Multiply by the conjugate: x(√(x²+1) − x) = x / (√(x²+1) + x) → 1/2 as x → ∞ |
