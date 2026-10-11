# Engineering Mathematics → Maxima, Minima & Mean Value Theorem

**GATE 2027 syllabus:** Engineering Mathematics — Calculus: Maxima and minima, Mean value theorem.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** A critical point is where f'(x) = 0 or f' does not exist. The second-derivative test: f''(c) > 0 ⇒ local minimum at c; f''(c) < 0 ⇒ local maximum at c; f''(c) = 0 is inconclusive. Rolle's theorem: if f is continuous on [a,b], differentiable on (a,b), and f(a) = f(b), then f'(c) = 0 for some c ∈ (a,b). The Mean Value Theorem (MVT): under the same continuity/differentiability conditions, there exists c ∈ (a,b) with f'(c) = (f(b) − f(a))/(b − a).

---

## Questions

### Q1 · MCQ · Fundamental
At a local maximum of a twice-differentiable function f, the second derivative f''(c) is

- (A) always positive
- (B) always negative or zero
- (C) always zero
- (D) undefined

---

### Q2 · NAT · Fundamental
The function f(x) = x³ − 3x² − 9x + 5 has a local maximum at x = −1. The value of f(−1) is ______.

---

### Q3 · MSQ · Standard
Which of the following statements about Rolle's theorem and the Mean Value Theorem is/are TRUE?

- (A) Rolle's theorem is a special case of the MVT, with f(a) = f(b).
- (B) The MVT requires f to be continuous on the closed interval [a,b] and differentiable on the open interval (a,b).
- (C) The MVT guarantees the point c is unique.
- (D) If f'(x) = 0 for all x in (a,b), then f is constant on [a,b] (assuming the continuity/differentiability hypotheses hold).

---

### Q4 · NAT · Standard
Using the Mean Value Theorem for f(x) = x² on the interval [1, 4], the value of c ∈ (1,4) guaranteed by the theorem is ______  (give as a decimal).

---

### Q5 · MCQ · Standard
The function f(x) = x³ − 6x² + 9x + 1 is evaluated on the closed interval [0, 5]. Its critical points inside this interval are at x = 1 and x = 3. The **global maximum** value of f on [0, 5] is

- (A) 5
- (B) 1
- (C) 21
- (D) 22

---

### Q6 · NAT · Moderate
The function g(x) = x⁴ − 4x³ + 4x² has critical points at x = 0, 1, 2. The number of these critical points that are **local minima** (using the second-derivative test) is ______.

---

### Q7 · MSQ · Moderate
Which of the following statements is/are TRUE about f(x) = x³ − 4x?

- (A) f(−2) = f(0) = f(2) = 0, so Rolle's theorem applies on both [−2, 0] and [0, 2].
- (B) The value(s) of c guaranteed by Rolle's theorem on [−2, 0] and on [0, 2] are, respectively, $-\dfrac{2}{\sqrt 3}$ and $\dfrac{2}{\sqrt 3}$.
- (C) f has exactly one critical point in (−2, 2).
- (D) f is an odd function (f(−x) = −f(x)).

---

### Q8 · NAT · Moderate
The global minimum value of h(x) = x³ − 6x² + 9x + 1 on the closed interval [0, 5] (same function as Q5) is ______.

---

### Q9 · MSQ · Hard
Which of the following statements is/are TRUE?

- (A) A function can have a local minimum at a point where it is not differentiable (e.g., f(x) = |x| at x = 0).
- (B) A continuous function on a closed, bounded interval [a,b] always attains both a global maximum and a global minimum on that interval (extreme value theorem).
- (C) If f'(c) = 0, then f has a local extremum at c (this is always true, with no exceptions).
- (D) An increasing function on an interval can have f'(x) = 0 at isolated points within that interval (e.g., f(x) = x³ at x = 0) while still being (weakly or strictly) increasing overall.

---

### Q10 · NAT · Killer
The function f(x) = x³ − 3x² − 9x + 5 (same as Q2) has a local maximum at x = −1 (value 10) and a local minimum at x = 3 (value −22). On the closed interval [−2, 5], the global maximum value of f is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | B | Concave down at a local max |
| 2 | 10 | f(−1) = −1 − 3 + 9 + 5 |
| 3 | A, B, D | (C) is false — the MVT only guarantees existence, not uniqueness, of c |
| 4 | 2.5 | Average slope = (16−1)/3 = 5; f'(c) = 2c = 5 gives c = 2.5 |
| 5 | C | Candidates: f(0)=1, f(1)=5, f(3)=1, f(5)=21; the maximum is 21 |
| 6 | 2 | g''(0) = 8 > 0 (min), g''(1) = −4 < 0 (max), g''(2) = 8 > 0 (min): 2 local minima |
| 7 | A, B, C, D | All four are correct, verified facts about this specific function |
| 8 | 1 | From Q5's candidate list: f(0) = 1 and f(3) = 1 tie for the global minimum |
| 9 | A, B, D | (C) is false — f(x) = x³ has f'(0) = 0 but no local extremum there (an inflection point) |
| 10 | 10 | Candidates: f(−2)=3, f(−1)=10, f(3)=−22, f(5)=10; the maximum, 10, is attained at both x=−1 and x=5 |
