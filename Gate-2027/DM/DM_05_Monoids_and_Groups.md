# Discrete Mathematics → Monoids & Groups

**GATE 2027 syllabus:** Engineering Mathematics — Discrete Mathematics: monoids, groups.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** ℤₙ denotes the integers mod n under addition unless a different operation is stated. Sₙ is the symmetric group on n symbols. φ is Euler's totient function.

---

## Questions

### Q1 · MCQ · Fundamental
Which one of the following is a group under the stated operation?

- (A) (ℤ, ×)
- (B) (ℚ ∖ {0}, ×)
- (C) (ℕ, +)
- (D) ({1, 2, 3, 4, 5}, multiplication mod 6)

---

### Q2 · NAT · Fundamental
The order of the element 9 in the group (ℤ₃₀, +) is ______.

---

### Q3 · NAT · Standard
The number of elements of order 12 in (ℤ₆₀, +) is ______.

---

### Q4 · MSQ · Standard
Which of the following is/are monoids that are **not** groups?

- (A) (ℕ ∪ {0}, +)
- (B) (ℤ₇ ∖ {0}, multiplication mod 7)
- (C) The set of all 2 × 2 real matrices under matrix multiplication
- (D) The set of all finite strings over {a, b} under concatenation

---

### Q5 · MSQ · Standard
Which of the following can **NOT** be the order of a subgroup of a group of order 20?

- (A) 3
- (B) 5
- (C) 8
- (D) 10

---

### Q6 · NAT · Moderate
The number of subgroups of (ℤ₇₂, +) is ______.

---

### Q7 · NAT · Moderate
The number of elements of order 2 in the symmetric group S₄ is ______.

---

### Q8 · MSQ · Moderate
Which of the following statements is/are TRUE?

- (A) Every group of order 5 is isomorphic to ℤ₅.
- (B) Every group of order 4 is cyclic.
- (C) If x² = e for every element x of a group G, then G is abelian.
- (D) Every group of order 6 is abelian.

---

### Q9 · NAT · Hard
The maximum possible order of an element of the symmetric group S₁₀ is ______.

---

### Q10 · NAT · Killer
The number of group homomorphisms from (ℤ₂₀, +) to (ℤ₃₀, +) is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | B | (A) no inverses; (C) no inverses; (D) 2·3 ≡ 0 mod 6, so not closed |
| 2 | 10 | 30 / gcd(9, 30) |
| 3 | 4 | φ(12) |
| 4 | A, C, D | (B) is a group |
| 5 | A, C | Lagrange: order must divide 20; 3 and 8 do not |
| 6 | 12 | One subgroup per divisor of 72 |
| 7 | 9 | 6 transpositions + 3 double transpositions |
| 8 | A, C | Counterexamples: Klein four-group for (B), S₃ for (D) |
| 9 | 30 | Cycle type 5 + 3 + 2, lcm 30 |
| 10 | 10 | Image of 1 must be an element x of ℤ₃₀ with 20x ≡ 0 (mod 30): gcd(20, 30) choices |
