# Discrete Mathematics → Combinatorics: Counting

**GATE 2027 syllabus:** Engineering Mathematics — Discrete Mathematics: combinatorics (counting).
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** C(n, k) is the binomial coefficient. D_n is the number of derangements of n objects. In circular arrangements, rotations are the same arrangement and reflections are different.

---

## Questions

### Q1 · NAT · Fundamental
The number of distinct arrangements of the letters of the word MISSISSIPPI is ______.

---

### Q2 · NAT · Fundamental
The number of solutions of x₁ + x₂ + x₃ + x₄ = 15 in integers with x₁ ≥ 2, x₂ ≥ 3, x₃ ≥ 0, x₄ ≥ 0 is ______.

---

### Q3 · NAT · Standard
The number of integers in {1, 2, …, 1000} that are divisible by none of 2, 3 and 5 is ______.

---

### Q4 · NAT · Standard
Six distinct letters are placed in six addressed envelopes, one letter per envelope. The number of ways in which **exactly two** letters go into their own envelopes is ______.

---

### Q5 · NAT · Standard
The smallest number n such that every n-element subset of {1, 2, …, 20} contains two distinct elements whose sum is 21 is ______.

---

### Q6 · NAT · Moderate
Eight people, including two particular people A and B, are seated around a circular table. The number of seatings in which A and B are **not** adjacent is ______.

---

### Q7 · NAT · Moderate
The number of ways to place 7 distinguishable balls into 3 distinguishable boxes so that no box is empty is ______.

---

### Q8 · NAT · Moderate
A lattice path from (0, 0) to (6, 6) uses unit steps right (+1, 0) and up (0, +1). The number of such paths that never go above the line y = x is ______.

---

### Q9 · NAT · Hard
The number of integer solutions of x₁ + x₂ + x₃ + x₄ = 20 with 0 ≤ xᵢ ≤ 7 for every i is ______.

---

### Q10 · NAT · Killer
Twenty people sit around a circular table. The number of ways to choose 5 of them so that no two chosen people sit next to each other is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | 34650 | 11! / (4!·4!·2!) |
| 2 | 286 | Substitute: sum becomes 10 over 4 variables, C(13, 3) |
| 3 | 266 | 1000 − (500 + 333 + 200) + (166 + 100 + 66) − 33 |
| 4 | 135 | C(6, 2) · D₄ = 15 · 9 |
| 5 | 11 | 10 pairs {i, 21 − i}; 10 elements can avoid a sum of 21, 11 cannot |
| 6 | 3600 | 7! − 2·6! |
| 7 | 1806 | 3⁷ − 3·2⁷ + 3 |
| 8 | 132 | Catalan number C₆ |
| 9 | 161 | C(23, 3) − 4·C(15, 3) + 6·C(7, 3) |
| 10 | 4004 | (20 / 15) · C(15, 5) |
