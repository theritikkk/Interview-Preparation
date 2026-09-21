# Discrete Mathematics → Partial Orders & Lattices

**GATE 2027 syllabus:** Engineering Mathematics — Discrete Mathematics: partial orders and lattices.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** D_n denotes the poset of positive divisors of n ordered by divisibility. A Hasse diagram has an edge for every covering pair. Lattices are compared up to isomorphism.

---

## Questions

### Q1 · MCQ · Fundamental
Which one of the following relations on the set of positive integers is a partial order?

- (A) a R b if and only if a divides b
- (B) a R b if and only if a − b is even
- (C) a R b if and only if a < b
- (D) a R b if and only if gcd(a, b) = 1

---

### Q2 · NAT · Fundamental
The number of edges in the Hasse diagram of D₃₆ (divisors of 36 under divisibility) is ______.

---

### Q3 · NAT · Standard
The number of maximal chains from 1 to 120 in D₁₂₀ is ______.

---

### Q4 · MSQ · Standard
A poset P has elements 0, a, b, c, d, 1 with covering relations 0 < a, 0 < b, a < c, a < d, b < c, b < d, c < 1, d < 1. Which of the following statements is/are TRUE?

- (A) P is a lattice.
- (B) P has both a least element and a greatest element.
- (C) The elements a and b have no least upper bound.
- (D) The elements c and d have a greatest lower bound.

---

### Q5 · NAT · Standard
The power set of {1, 2, 3, 4, 5} is ordered by inclusion. The minimum number of chains whose union is the whole poset is ______.

---

### Q6 · NAT · Moderate
The number of linear extensions (total orders that respect divisibility) of D₃₀ is ______.

---

### Q7 · MSQ · Moderate
Which of the following lattices is/are distributive?

- (A) The pentagon N₅: elements 0, a, b, c, 1 with 0 < a < b < 1 and 0 < c < 1, c incomparable with a and b
- (B) D₃₀
- (C) The diamond M₃: elements 0, three pairwise incomparable elements a, b, c, and 1
- (D) The 5-element chain

---

### Q8 · NAT · Moderate
In the lattice D₇₂ (divisors of 72), the number of elements that have a complement is ______.

---

### Q9 · NAT · Hard
The number of antichains of the poset D₆₀, **including** the empty antichain, is ______.

---

### Q10 · NAT · Killer
The number of pairwise non-isomorphic lattices with exactly 5 elements is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | A | (B) equivalence; (C) irreflexive; (D) neither reflexive nor transitive |
| 2 | 12 | 3 × 3 grid (36 = 2²·3²): 2·3·2 edges |
| 3 | 20 | 5!/(3!·1!·1!) orderings of prime factors |
| 4 | B, C | c and d have lower bounds a, b, 0 with a, b incomparable, so no glb; likewise a, b have no lub |
| 5 | 10 | Dilworth: width of the Boolean lattice B₅ is C(5,2) |
| 6 | 48 | D₃₀ ≅ B₃; linear extensions of the Boolean lattice on 3 atoms |
| 7 | B, D | N₅ and M₃ are exactly the non-distributive lattices |
| 8 | 4 | Complemented divisors are the unitary ones: 1, 8, 9, 72 |
| 9 | 50 | Antichains ↔ order ideals of the 3 × 2 × 2 grid |
| 10 | 5 | Chain, N₅, M₃, B₂ with a new top, B₂ with a new bottom |
