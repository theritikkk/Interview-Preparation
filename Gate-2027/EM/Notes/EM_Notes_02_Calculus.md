# EM Notes 02 — Calculus: Limits, MVT, Extrema & Integration (High-Yield Deep Dive)

> **Why this file exists:** The three calculus banks (EM_03–EM_05) test a small set of reflexes — **standard limits → L'Hôpital/Taylor → |·| kinks → MVT & extrema → integration by parts/symmetry → improper & double integrals**. Every value below was verified symbolically or by numerical quadrature before writing; the practice questions are fresh (not in the banks) with every answer verified.
>
> **Companion question banks:** `../EM_03_Limits_Continuity_Differentiability.md`, `../EM_04_Maxima_Minima_Mean_Value_Theorem.md`, `../EM_05_Integration.md`. **PW source:** `../Reference/PW_Subject_Handbooks/PW_EngineeringMath.txt` §1.1–§1.9 and §1.13–§1.16 (pages 1.1–1.16: limits, MVT, extrema, Taylor, integrals, definite-integral properties, gamma, areas, multivariable calculus).

---

## 0. What GATE actually asks

```
(A) Limits           standard limits ; L'Hôpital (only 0/0, ∞/∞) ; Taylor/Maclaurin expansions
(B) Continuity/diff  |x| and |x²−c| kinks ; piecewise "choose k" ; one-sided derivatives
(C) MVT family       Rolle / Lagrange / Cauchy existence ; solving for c
(D) Extrema          first/second-derivative tests ; global = candidates + endpoints
(E) Integration      standard forms ; by parts (ILATE) ; even/odd ; King's property
(F) Improper         ∫₁^∞ x^(−p): converges iff p > 1 ; exponential tails
(G) Areas/volumes    area between curves ; iterated (double) integrals
```

---

## 1. Limits (PW §1.1, §1.5) — the starter pack

Standard limits the exam reuses with rescaled arguments:

| Limit | Value | Limit | Value |
|---|---|---|---|
| limₓ→₀ (sin x)/x | 1 | limₓ→₀ (1 − cos x)/x² | 1/2 |
| limₓ→₀ (eˣ − 1)/x | 1 | limₓ→₀ (tan x − sin x)/x³ | 1/2 |
| limₓ→∞ (1 + 1/x)^x | e | limₓ→∞ (1 + a/x)^x | eᵃ |
| limₓ→₀ (1 + x)^{1/x} | e | limₓ→∞ x(√(x² + 1) − x) | 1/2 |

**Scaling — the engine behind "new" limit questions:**
- sin(5x)/(3x) = (5/3)·sin(5x)/(5x) → **5/3**;
- (e^{2x} − 1)/x = 2·(e^{2x} − 1)/(2x) → **2**;
- (1 − cos 4x)/x² = 16·(1 − cos 4x)/(4x)² → 16·(1/2) = **8**.

(All three are fresh practice items below, verified numerically.)

**Conjugate trick:** x(√(x² + 1) − x) = x/(√(x² + 1) + x) → 1/2 (bank EM_03 Q10).

**Taylor/Maclaurin (PW §1.5):** sin x = x − x³/6 + …, cos x = 1 − x²/2 + …, eˣ = 1 + x + x²/2 + …, ln(1 + x) = x − x²/2 + …. For messy 0/0 forms, expand numerator and denominator to the first surviving power — e.g. (tan x − sin x)/x³ = [(x + x³/3) − (x − x³/6)]/x³ → **1/2** (bank anchor).

**L'Hôpital (PW §1.1.1):** valid **only** for 0/0 or ∞/∞ — differentiate numerator and denominator separately, repeat if needed. The other indeterminate forms (0·∞, ∞ − ∞, 1^∞, 0⁰, ∞⁰) must be reshaped into a quotient or handled with logs first.

**Rational functions at ∞:** compare degrees — denominator degree wins ⇒ 0; tie ⇒ ratio of leading coefficients; numerator degree wins ⇒ ±∞ (sign of leading coefficient).

---

## 2. Continuity and differentiability (PW §1.1.1)

- **Differentiable ⇒ continuous** at a point. **Continuous ⇏ differentiable** — |x| at 0 is the canonical counterexample (bank staple).
- **Continuity at c:** limₓ→c f(x) = f(c); equivalently, both one-sided limits exist and equal f(c). A function can be continuous at a point where the two-sided limit of the *formula* varies — that's exactly the |x|-style corner situation, and the piecewise "choose k" family.
- **One-sided derivatives:** f'(c⁺) = lim_{h→0⁺} [f(c + h) − f(c)]/h (and similarly f'(c⁻)); differentiable at c ⟺ both exist and are equal.
- **Kink landmarks in the syllabus:** |x| at 0; |x − a| at a; **|x² − c| at ±√c**; |x| + |x − a| at 0 and a. Between kinks, absolute-value functions are perfectly differentiable (e.g. f(x) = |x² − 4| is differentiable at x = 0 and x = 3, only kinked at ±2 — bank EM_03 Q7).
- **x·|x| is differentiable everywhere:** for x ≥ 0, x|x| = x²; for x ≤ 0 it is −x²; at 0 the difference quotient is h|h|/h = |h| → 0. So f'(x) = 2|x| for x ≠ 0 and f'(0) = 0 — a corner-free cousin of |x| (fresh practice Q3).
- **"Choose k" continuity:** compute the limit of the formula at the junction; that limit is the required k. f(x) = sin(3x)/x (x ≠ 0) needs k = 3 (bank); f(x) = (1 − cos 4x)/x² (x ≠ 0) needs k = 8 (fresh Q4).
- **Discontinuity taxonomy (bank EM_03 Q9):** removable (limit exists but ≠ f(c) or f(c) undefined), jump (both one-sided limits exist, unequal), otherwise essential/infinite. Boundedness on [a, b] does **not** imply continuity; the composition of continuous functions is continuous wherever defined.

---

## 3. The mean value theorems (PW §1.2)

**Rolle (PW §1.2.2).** f continuous on [a, b], differentiable on (a, b), **f(a) = f(b)** ⇒ ∃c ∈ (a, b) with f'(c) = 0.

**Lagrange MVT (PW §1.2.1).** f continuous on [a, b], differentiable on (a, b) ⇒ ∃c ∈ (a, b):

$$f'(c) = \frac{f(b) - f(a)}{b - a}$$

(the tangent at c is parallel to the chord; note PW §1.2.1).

**Cauchy MVT (PW §1.2.3).** ∂ for f, g: (f(b) − f(a))·g'(c) = (g(b) − g(a))·f'(c) for some c — the ratio form behind L'Hôpital.

**Solving for c (the usual numeric ask):**
- Quadratic f ⇒ c = (a + b)/2 always; e.g. f = x² on [1, 4]: average slope (16 − 1)/3 = 5, f'(c) = 2c = 5 ⇒ **c = 2.5** (bank EM_04 Q4).
- Cubic ⇒ solve f'(x) = slope. Rolle example: f = x(x − 1)(x − 2) = x³ − 3x² + 2x on [0, 2] (f(0) = f(2) = 0): f' = 3x² − 6x + 2 = 0 ⇒ x = 1 ± 1/√3 ≈ 0.42 and **1.58** — Rolle guarantees *at least one* c; this f has two (fresh Q6).
- Rolle ⇔ MVT with f(a) = f(b) (bank MSQ statement: Rolle is the special case).

**Consequences (bank MSQ material):** f' > 0 on an interval ⇒ f strictly increasing there; f' < 0 ⇒ strictly decreasing; f' ≡ 0 on (a, b) ⇒ f constant on [a, b] (PW §1.3). The MVT gives **existence**, never uniqueness, of c.

---

## 4. Monotonicity and extrema (PW §1.3–§1.4)

**First-derivative test.** Between critical points, sign(f') sets ↑/↓; a sign change + → − at c is a local max, − → + a local min, no sign change ⇒ neither (f = x³ at 0: f' = 3x² ≥ 0, an inflection — bank EM_04 Q9).

**Second-derivative test (PW §1.4).** At f'(c) = 0: f''(c) < 0 ⇒ local max; f''(c) > 0 ⇒ local min; **f''(c) = 0 ⇒ inconclusive** — escalate to f''', f⁗, … (PW's four-step procedure: solve f' = 0, then screen each root with the first nonzero higher derivative).

**Key facts engine:**
- **Fermat:** at an interior point of a local extremum where f is differentiable, f'(c) = 0. The converse is FALSE (x³ at 0); and differentiability may fail entirely (|x| has its minimum at a corner — bank EM_04 Q9).
- **Extreme value theorem:** continuous on closed bounded [a, b] ⇒ attains global max and min on [a, b].
- **Global extrema recipe:** candidates = critical points inside (a, b) **plus both endpoints**; evaluate and rank. Never skip endpoints (that is where the bank's Q10 traps live).
- **Strictly increasing with infinitely many critical points:** f(x) = x + sin x has f'(x) = 1 + cos x ≥ 0, vanishing exactly at the isolated points x = (2k + 1)π, so f is strictly increasing on all of ℝ (fresh Q7 (A); verified numerically on 20,000 random pairs). Monotonicity needs f' ≥ 0 with isolated zeros — not f' > 0 everywhere, and not a *single* sign check: f'(c) < 0 at one point says nothing global (fresh Q7 (C) trap).

**Worked bank anchors.** f = x³ − 3x² − 9x + 5: local max x = −1 (value 10), local min x = 3 (value −22); on [−2, 5] the global max is 10 at both x = −1 and x = 5 (EM_04 Q10). f = x³ − 6x² + 9x + 1 on [0, 5]: candidates {0, 1, 3, 5} → max 21, min 1 (EM_04 Q5/Q8). Fresh twin below: f = x³ − 12x on [−3, 3] → global max **16** at x = −2.

---

## 5. Integration (PW §1.6–§1.9, §1.13–§1.16)

**Core antiderivatives (PW §1.8):** xⁿ (n ≠ −1), 1/x, sin, cos, sec², eˣ, aˣ, 1/(1 + x²) → arctan x, and the log-forms ∫f'/f = ln|f|.

**By parts (PW §1.8.15) with the ILATE order:** ∫ f·g = f·∫g − ∫ f'·(∫g); choose as the differentiated part the function earlier in **I**nverse, **L**og, **A**lgebraic, **T**rig, **E**xponential. Bank anchors: ∫₀¹ x eˣ dx = 1; ∫₀¹ x² eˣ dx = e − 2 ≈ 0.72.

**Definite-integral properties (PW §1.9):**
- Reversal: ∫ₐᵇ f = −∫ᵦᵃ f.
- **Even/odd on symmetric intervals:** ∫₋ₐᵃ f = 2∫₀ᵃ f for even f (∫₋₁¹ |x| dx = 1); ∫₋ₐᵃ f = 0 for odd f (∫₋π^π sin x dx = 0) — fresh Q9.
- **King's property:** ∫ₐᵇ f(x) dx = ∫ₐᵇ f(a + b − x) dx; the engine behind ∫₀^{π/2} sin x/(sin x + cos x) dx = π/4 (PW §1.9 example, verified numerically).
- Wallis (PW §1.9.7): ∫₀^{π/2} sinᵐ x dx = ∫₀^{π/2} cosᵐ x dx via the descending-fraction rule.
- **Newton–Leibniz rule** (PW §1.7): (d/dx)∫_{φ(x)}^{ψ(x)} f(t) dt = f(ψ(x))ψ'(x) − f(φ(x))φ'(x).

**Improper integrals (p-test).** ∫₁^∞ x^{−p} dx converges ⟺ **p > 1** (value 1/(p − 1)); ∫₀¹ x^{−p} dx converges ⟺ p < 1; exponential tails ∫₁^∞ e^{−ax} dx converge for every a > 0. Bank anchors: ∫₁^∞ dx/x² = 1 converges; ∫₁^∞ dx/x diverges (EM_05 Q3).

**Areas between curves (PW §1.15).** Area = ∫ (upper − lower) over the overlap interval. Between y = √x and y = x² on [0, 1]: ∫₀¹(√x − x²) dx = 2/3 − 1/3 = **1/3** (verified numerically). Bank anchor: y = 2x vs y = x² over the overlap gives 4/3 (EM_05 Q7).

**Double integrals (PW §1.16).** Iterate (inner variable first, outer fixed); Fubini allows swapping the order on rectangles; on rectangular regions with separable integrands f(x)g(y), the integral factors as (∫f)(∫g); on a triangle the inner limit depends on the outer variable. Worked example (fresh Q10):

$$\int_0^1\!\int_0^x (x + y)\,dy\,dx = \int_0^1 \Big[x y + \tfrac{y^2}{2}\Big]_{y=0}^{x} dx = \int_0^1 \tfrac{3x^2}{2}\,dx = \frac{1}{2}$$

(verified by fine numerical quadrature).

**Gamma shortcut (PW §1.13):** ∫₀^∞ xⁿ e^{−x} dx = n! — e.g. ∫₀^∞ x e^{−λx} dx = 1/λ².

---

## 6. Trap list (read before the exam)

1. L'Hôpital on a non-indeterminate form is invalid; 0·∞, ∞ − ∞, 1^∞, 0⁰, ∞⁰ must be reshaped (quotient/log) first.
2. Differentiable ⇒ continuous, never the reverse; |x| at 0 settles every such True/False.
3. |x² − c| has **two** kinks (±√c); |x| + |x − a| kinks exactly at 0 and a; x·|x| has no kink at all.
4. MVT/Rolle give existence, not uniqueness, of c — and c lies strictly inside (a, b), never at an endpoint.
5. f'(c) = 0 is necessary for interior differentiable extrema but **not sufficient** (x³).
6. Global extrema require the endpoints — the best interior critical point can lose to f(a) or f(b).
7. Strictly increasing ⇏ f' > 0 everywhere (x + sin x is strictly increasing with f' = 0 infinitely often); "f'(c) < 0 somewhere ⇒ decreasing everywhere" is flatly false.
8. ∫ₐᵇ = −∫ᵦᵃ — dropped minus signs are the #1 loss; odd-function vanishing needs the **symmetric** interval.
9. p-test direction flips between the two ends: at ∞ convergence needs p > 1; at 0 (x^{−p}) it needs p < 1.
10. By parts: the differentiated factor should be the earlier one in ILATE; flipping the order can loop forever.
11. Non-rectangular double integrals: inner bounds may depend on the outer variable — sketch the region before swapping the order.

---

## 7. Practice questions (fresh — not in the banks)

### Q1 · MCQ · Fundamental

$$\lim_{x \to 0} \frac{\sin(5x)}{3x} = $$

- (A) 5/3
- (B) 3/5
- (C) 5
- (D) 1

### Q2 · NAT · Fundamental

$$\lim_{x \to 0} \frac{e^{2x} - 1}{x}$$
is ______.

### Q3 · MSQ · Standard

Let f(x) = x·|x|. Which of the following statements is/are TRUE?

- (A) f is differentiable at x = 0, with f'(0) = 0.
- (B) f is continuous everywhere but not differentiable at x = 0.
- (C) f'(x) = 2|x| for every x ≠ 0.
- (D) f is not differentiable at any point of ℝ.

### Q4 · NAT · Standard

The function

$$f(x) = \begin{cases} \dfrac{1 - \cos(4x)}{x^2}, & x \neq 0 \\[4pt] k, & x = 0 \end{cases}$$

is continuous at x = 0 for exactly one value of k. That value is ______.

### Q5 · MCQ · Standard

$$\int_0^2 (3x^2 + 2x)\, dx = $$

- (A) 8
- (B) 10
- (C) 12
- (D) 16

### Q6 · NAT · Moderate

Let f(x) = x(x − 1)(x − 2). Rolle's theorem applies on [0, 2]; in fact f'(x) = 0 has exactly two solutions in (0, 2). The **larger** of the two, rounded to two decimal places, is ______.

### Q7 · MSQ · Moderate

Which of the following statements is/are TRUE?

- (A) f(x) = x + sin x is strictly increasing on all of ℝ, even though f'(x) = 0 at infinitely many points.
- (B) If f'(x) > 0 for every x in an interval, then f is strictly increasing on that interval.
- (C) If f'(c) < 0 at some point c, then f is decreasing on every interval containing c.
- (D) If f has a local maximum at an interior point c where f is differentiable, then f'(c) = 0.

### Q8 · NAT · Moderate

The global maximum value of f(x) = x³ − 12x on the closed interval [−3, 3] is ______.

### Q9 · MSQ · Hard

Which of the following statements is/are TRUE?

- (A) $\displaystyle\int_{-1}^{1} |x|\, dx = 1$.
- (B) $\displaystyle\int_{-\pi}^{\pi} \sin x\, dx = 0$ (odd-function symmetry).
- (C) If f is even and continuous on [−a, a], then $\displaystyle\int_{-a}^{a} f(x)\,dx = 2\int_0^{a} f(x)\,dx$.
- (D) Every continuous function on a closed interval [a, b] has an antiderivative expressible in elementary functions (an explicit closed form).

### Q10 · NAT · Killer

$$\int_0^1 \int_0^{x} (x + y)\, dy\, dx$$
is ______ (give as a fraction or decimal, e.g. 0.5).

---

## 8. Answer key & full solutions

**Answer key:** 1-A · 2-2 · 3-A,C · 4-8 · 5-C · 6-1.58 · 7-A,B,D · 8-16 · 9-A,B,C · 10-0.5

**Q1 — A.** sin(5x)/(3x) = (5/3)·sin(5x)/(5x) → (5/3)·1 = **5/3**. (B) 3/5 inverts the scaling; (D) drops the factor entirely.

**Q2 — 2.** With u = 2x: (e^{2x} − 1)/x = 2·(e^u − 1)/u → 2·1 = **2** (verified numerically: 2.0000002 at x = 10⁻⁷).

**Q3 — A, C.** At 0: [f(h) − f(0)]/h = h|h|/h = |h| → 0, so f'(0) = 0 exists — (A) ✓, (B) ✘. For x > 0, f = x² ⇒ f' = 2x; for x < 0, f = −x² ⇒ f' = −2x = 2|x| — (C) ✓. (D) ✘ since (A) holds. (Numerically: f'(−2) ≈ 4 = 2|−2|, f'(3) ≈ 6 = 2·3.)

**Q4 — 8.** (1 − cos 4x)/x² = 16·(1 − cos 4x)/(4x)² → 16·(1/2) = **8** (numerically 7.9999996 at x = 10⁻⁵).

**Q5 — C.** ∫ = [x³ + x²]₀² = 8 + 4 = **12**.

**Q6 — 1.58.** f(0) = 0 = f(2) and f is a polynomial ⇒ Rolle applies. f'(x) = 3x² − 6x + 2 = 0 ⇒ x = (6 ± √12)/6 = 1 ± 1/√3 ≈ 0.4226, 1.5774. The larger, rounded, is **1.58**. (Rolle guarantees at least one c; the count here is two.)

**Q7 — A, B, D.** (A) ✓ f' = 1 + cos x ≥ 0 with zeros only at isolated points x = (2k + 1)π, so the integral of f' over any nonzero interval is positive — strictly increasing (verified on 20,000 random pairs: zero violations). (B) ✓ MVT: f(b) − f(a) = f'(c)(b − a) > 0. (C) ✘ counterexample f(x) = x³ − 3x at c = 0: f'(0) = −3 < 0 yet f'(1.5) = 3.75 > 0, so f is not decreasing on the whole interval. (D) ✓ Fermat's theorem for interior differentiable local maxima.

**Q8 — 16.** f' = 3x² − 12 = 0 ⇒ x = ±2. Compare f(−3) = 9, f(−2) = 16, f(2) = −16, f(3) = −9: the global maximum is **16** (at x = −2). Skipping endpoints still gives 16 here, but the full comparison is the exam-safe routine.

**Q9 — A, B, C.** (A) ✓ 2∫₀¹ x dx = 1 (verified numerically: 1.000000). (B) ✓ sin is odd and the interval is symmetric (numerically ≈ 0). (C) ✓ the standard even-function halving property (PW §1.9 property 3). (D) ✘ false — e.g. e^{−x²} is continuous on every [a, b], but its antiderivative is not an elementary function.

**Q10 — 0.5 (1/2).** Inner integral: ∫₀^x (x + y) dy = xy + y²/2 evaluated from 0 to x = x² + x²/2 = 3x²/2. Outer: ∫₀¹ (3x²/2) dx = [x³/2]₀¹ = **1/2**. (Verified by numerical quadrature on a 4000×4000 grid.)

---

## 9. Next steps

1. Rebuild the standard-limits table from the Taylor expansions — every entry is a one-line verification and the scaled versions become free marks.
2. Keep a running list of kink landmarks (|x|, |x − a|, |x² − c|, sums of |·|) and test differentiability at each separately.
3. Practise the MVT "solve for c" procedure until quadratics and cubics are seconds-fast, and always include endpoints in global-extrema tables.
4. Run banks `EM_03`–`EM_05` cold, then continue with `EM_Notes_03_Probability_Statistics.md`.
