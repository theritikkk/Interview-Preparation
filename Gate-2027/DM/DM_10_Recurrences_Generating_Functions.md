# Discrete Mathematics → Recurrence Relations & Generating Functions

**GATE 2027 syllabus:** Engineering Mathematics — Discrete Mathematics: combinatorics (recurrence relations, generating functions).
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** Sequences start at index 0 unless stated otherwise. The generating function of (aₙ) is Σ aₙ xⁿ.

---

## Questions

### Q1 · NAT · Fundamental
A sequence satisfies aₙ = 3aₙ₋₁ − 2aₙ₋₂ for n ≥ 2, with a₀ = 1 and a₁ = 4. The value of a₅ is ______.

---

### Q2 · MCQ · Fundamental
The generating function of the sequence aₙ = (n + 1)·2ⁿ, n ≥ 0, is

- (A) 1 / (1 − 2x)²
- (B) 2 / (1 − x)²
- (C) 1 / (1 − 2x²)
- (D) 1 / (1 − 2x) + 1 / (1 − x)²

---

### Q3 · NAT · Standard
The number of solutions of e₁ + e₂ + e₃ + e₄ = 10 with 0 ≤ eᵢ ≤ 3 for every i (the coefficient of x¹⁰ in (1 + x + x² + x³)⁴) is ______.

---

### Q4 · NAT · Standard
A sequence satisfies aₙ = 6aₙ₋₁ − 9aₙ₋₂ for n ≥ 2, with a₀ = 2 and a₁ = 9. The value of a₆ is ______.

---

### Q5 · NAT · Standard
A sequence satisfies aₙ = 2aₙ₋₁ + 3ⁿ for n ≥ 1, with a₀ = 1. The value of a₆ is ______.

---

### Q6 · NAT · Moderate
The number of strings of length 6 over the alphabet {0, 1, 2} that do **not** contain two consecutive 2s is ______.

---

### Q7 · MSQ · Moderate
Which of the following statements is/are TRUE?

- (A) The number of partitions of n into distinct parts equals the number of partitions of n into odd parts.
- (B) The generating function of aₙ = C(n + 2, 2) is 1 / (1 − x)³.
- (C) The generating function for the number of ways to make n rupees with coins of ₹1, ₹5 and ₹10 (unlimited supply, order irrelevant) is 1 / ((1 − x)(1 − x⁵)(1 − x¹⁰)).
- (D) The generating function C(x) of the Catalan numbers satisfies C(x) = 1 + x·C(x).

---

### Q8 · NAT · Moderate
The number of ways to pay exactly ₹30 using coins of ₹1, ₹5, ₹10 and ₹20 (unlimited supply of each, order of coins irrelevant) is ______.

---

### Q9 · NAT · Hard
A sequence satisfies aₙ = 4aₙ₋₁ − 4aₙ₋₂ + 2ⁿ for n ≥ 2, with a₀ = 1 and a₁ = 4. The value of a₆ is ______.

---

### Q10 · NAT · Killer
A shop has four types of donuts A, B, C and D (unlimited supply of each). The number of ways to choose 12 donuts so that the number of A's is even and at least 2, the numbers of B's and of C's are each at least 1, and the number of D's is between 1 and 3 inclusive, is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | 94 | aₙ = 3·2ⁿ − 2 |
| 2 | A | Σ (n + 1)(2x)ⁿ = 1 / (1 − 2x)² |
| 3 | 10 | Coefficient extraction / inclusion–exclusion |
| 4 | 5832 | Repeated root 3: aₙ = (2 + n)·3ⁿ |
| 5 | 2059 | aₙ = 3ⁿ⁺¹ − 2ⁿ⁺¹ |
| 6 | 448 | aₙ = 2aₙ₋₁ + 2aₙ₋₂ with a₁ = 3, a₂ = 8: 22, 60, 164, 448 |
| 7 | A, B, C | (D) should be C(x) = 1 + x·C(x)² |
| 8 | 20 | Coin-change count for 30 with {1, 5, 10, 20} |
| 9 | 1408 | Resonant forcing 2ⁿ on the double root 2: aₙ = (n² + n + 2)·2ⁿ⁻¹ |
| 10 | 48 | Coefficient of x¹² in x²/(1 − x²) · (x/(1 − x))² · (x + x² + x³) |
