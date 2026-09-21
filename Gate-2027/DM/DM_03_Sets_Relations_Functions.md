# Discrete Mathematics → Sets, Relations & Functions

**GATE 2027 syllabus:** Engineering Mathematics — Discrete Mathematics: sets, relations, functions.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** A relation on a set A is any subset of A × A. ℕ = {0, 1, 2, …}.

---

## Questions

### Q1 · NAT · Fundamental
The number of relations on a 4-element set that are both reflexive and symmetric is ______.

---

### Q2 · MSQ · Fundamental
Which of the following relations on the set ℤ of integers is/are equivalence relations?

- (A) x R y if and only if x² = y²
- (B) x R y if and only if |x − y| ≤ 2
- (C) x R y if and only if 4 divides (x − y) or 6 divides (x − y)
- (D) x R y if and only if ⌊x / 3⌋ = ⌊y / 3⌋

---

### Q3 · NAT · Standard
The number of equivalence relations on {1, 2, 3, 4, 5, 6} in which 1 and 2 are related but 3 is **not** related to 1 is ______.

---

### Q4 · MSQ · Standard
Let f : A → B and g : B → C be functions. Which of the following statements is/are TRUE?

- (A) If g ∘ f is injective, then f is injective.
- (B) If g ∘ f is injective, then g is injective.
- (C) If g ∘ f is surjective, then g is surjective.
- (D) If g ∘ f is surjective, then f is surjective.

---

### Q5 · NAT · Standard
The number of onto (surjective) functions from {1, 2, 3, 4, 5, 6} to {a, b, c} is ______.

---

### Q6 · NAT · Moderate
The number of relations on a 4-element set that are reflexive and symmetric but **not** transitive is ______.

---

### Q7 · NAT · Moderate
Let R = {(1, 2), (2, 3), (3, 1), (4, 5)} on the set {1, 2, 3, 4, 5}. The number of ordered pairs in the transitive closure of R is ______.

---

### Q8 · MSQ · Moderate
Which of the following sets is/are countable?

- (A) The set of all finite subsets of ℕ
- (B) The set of all functions from ℕ to {0, 1}
- (C) The set of all binary relations on ℕ
- (D) The set of all finite-length strings over a countably infinite alphabet

---

### Q9 · NAT · Hard
The number of functions f : {1, 2, 3, 4, 5} → {1, 2, 3, 4, 5} satisfying f(f(x)) = f(x) for every x is ______.

---

### Q10 · NAT · Killer
The number of relations on a 4-element set that are symmetric **and** transitive (not necessarily reflexive) is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | 64 | 2^(C(4,2)) |
| 2 | A, D | (B) not transitive; (C) 0 R 4, 4 R 10, but not 0 R 10 |
| 3 | 37 | Merge 1, 2 into one element: Bell(5) − Bell(4) = 52 − 15 |
| 4 | A, C | Standard cancellation facts |
| 5 | 540 | 3⁶ − 3·2⁶ + 3 |
| 6 | 49 | 64 reflexive-symmetric relations − 15 equivalence relations |
| 7 | 10 | Cycle 1→2→3→1 gives all 9 pairs on {1,2,3}; plus (4, 5) |
| 8 | A, B, D | (B) is uncountable by diagonalisation; (C) contains 2^ℕ subsets of ℕ × ℕ |
| 9 | 196 | Choose the image set of size k (fixed points), other elements map into it: Σ C(5,k)·k^(5−k) |
| 10 | 52 | Pick the set of elements that appear in some pair and an equivalence relation on it: Σ C(4,k)·Bell(k) = Bell(5) |
