# Engineering Mathematics → Integration

**GATE 2027 syllabus:** Engineering Mathematics — Calculus: Integration.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** All integrals are definite unless stated otherwise. An improper integral converges if its limiting value exists and is finite. The Fundamental Theorem of Calculus (FTC) part 1: if $g(x) = \int_a^x f(t)\,dt$, then $g'(x) = f(x)$.

---

## Questions

### Q1 · MCQ · Fundamental
The value of $\int_0^3 x^2 \, dx$ is

- (A) 6
- (B) 9
- (C) 18
- (D) 27

---

### Q2 · NAT · Fundamental
The value of $\int_0^{\pi} \sin x \, dx$ is ______.

---

### Q3 · MSQ · Standard
Which of the following statements is/are TRUE?

- (A) $\int_1^{\infty} \dfrac{1}{x^2}\,dx$ converges (to a finite value).
- (B) $\int_1^{\infty} \dfrac{1}{x}\,dx$ diverges.
- (C) $\int_1^{\infty} \dfrac{1}{x^p}\,dx$ converges if and only if p > 1.
- (D) Every improper integral of an unbounded function over an unbounded interval diverges.

---

### Q4 · NAT · Standard
The value of $\int_0^{\infty} e^{-x} \, dx$ is ______.

---

### Q5 · MCQ · Standard
The value of $\int_0^1 \dfrac{1}{1+x^2}\,dx$ is

- (A) π/6
- (B) π/4
- (C) π/2
- (D) π

---

### Q6 · NAT · Moderate
The value of $\int_0^1 x e^x \, dx$ (using integration by parts) is ______.

---

### Q7 · MSQ · Moderate
Which of the following statements is/are TRUE?

- (A) If $g(x) = \int_0^x \cos t \, dt$, then $g(x) = \sin x$ and $g'(x) = \cos x$, consistent with the Fundamental Theorem of Calculus.
- (B) $\int_{-1}^{1} \sqrt{1-x^2}\, dx$ equals the area of a semicircle of radius 1, i.e., π/2.
- (C) The area enclosed between the curves y = 2x and y = x² (for x between their two intersection points) is 4/3.
- (D) $\int_a^b f(x)\,dx = -\int_b^a f(x)\,dx$ for any integrable f and any a, b.

---

### Q8 · NAT · Moderate
The value of $\int_0^1 \dfrac{x}{x^2+1}\,dx$ is ______  (give in the form $\ln(k)/2$, answering with just the value of k).

---

### Q9 · MSQ · Hard
Which of the following statements about double integrals is/are TRUE?

- (A) $\int_0^2 \int_0^1 xy \, dy \, dx = \int_0^1 \int_0^2 xy \, dx \, dy$ (Fubini's theorem allows swapping the order of integration here, since the integrand is continuous and the region is a rectangle).
- (B) The double integral $\int_0^2\int_0^1 xy\,dy\,dx$ evaluates to 1.
- (C) A double integral over a rectangular region with a separable integrand f(x)g(y) always factors as (single integral of f) × (single integral of g).
- (D) Double integrals can be used to compute the volume under a surface z = f(x,y) over a region in the xy-plane.

---

### Q10 · NAT · Killer
The value of $\int_0^1 x^2 e^x \, dx$, computed using repeated integration by parts, is ______  (give as a decimal rounded to two decimal places, using e ≈ 2.71828).

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | B | x³/3 evaluated from 0 to 3 |
| 2 | 2 | −cos(π) − (−cos(0)) = 1 + 1 |
| 3 | A, B, C | (D) is false — many such integrals do converge, e.g. (A) |
| 4 | 1 | −e⁻ˣ evaluated from 0 to ∞ |
| 5 | B | arctan(1) − arctan(0) = π/4 |
| 6 | 1 | [x·eˣ − eˣ] from 0 to 1 = (e − e) − (0 − 1) |
| 7 | A, B, C, D | All four are standard, correct facts |
| 8 | 2 | ln(x²+1)/2 evaluated from 0 to 1 gives ln(2)/2, so k = 2 |
| 9 | A, B, C, D | All four are standard, correct facts — (C) holds precisely because the region is rectangular and the integrand separates |
| 10 | 0.72 | e − 2 ≈ 2.71828 − 2 |
