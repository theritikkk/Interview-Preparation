# Discrete Mathematics → First-Order Logic

**GATE 2027 syllabus:** Engineering Mathematics — Discrete Mathematics: propositional and first-order logic.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** Domains are non-empty. Variables may take equal values (x and y may denote the same element) unless a formula says x ≠ y. "Model" means an interpretation of the predicate/function symbols over the stated domain.

---

## Questions

### Q1 · MCQ · Fundamental
With S(x): "x is a student", C(x): "x is a course", L(x, y): "x likes y", which one of the following expresses "Every student likes some course"?

- (A) ∀x ∃y (S(x) ∧ C(y) ∧ L(x, y))
- (B) ∀x (S(x) → ∃y (C(y) ∧ L(x, y)))
- (C) ∀x ∃y ((S(x) ∧ C(y)) → L(x, y))
- (D) ∃y ∀x (S(x) → (C(y) ∧ L(x, y)))

---

### Q2 · MCQ · Fundamental
Which one of the following is equivalent to ¬∀x ∃y (P(x, y) → Q(x, y))?

- (A) ∃x ∀y (¬P(x, y) ∨ Q(x, y))
- (B) ∃x ∀y (P(x, y) ∧ ¬Q(x, y))
- (C) ∀x ∃y (P(x, y) ∧ ¬Q(x, y))
- (D) ∃x ∃y (P(x, y) ∧ ¬Q(x, y))

---

### Q3 · MSQ · Standard
Which of the following formulas is/are valid?

- (A) ∀x (P(x) → Q(x)) → (∀x P(x) → ∀x Q(x))
- (B) (∀x P(x) → ∀x Q(x)) → ∀x (P(x) → Q(x))
- (C) ∃x ∀y R(x, y) → ∀y ∃x R(x, y)
- (D) ∀x ∃y R(x, y) → ∃y ∀x R(x, y)

---

### Q4 · NAT · Standard
The domain is {1, 2, 3} and R is a binary predicate. The number of interpretations of R that make ∀x ∃y R(x, y) ∧ ∀x ¬R(x, x) true is ______.

---

### Q5 · MCQ · Moderate
Let φ = (∀x ¬R(x, x)) ∧ (∀x ∀y ∀z (R(x, y) ∧ R(y, z) → R(x, z))) ∧ (∀x ∃y R(x, y)). Which one of the following is TRUE?

- (A) φ is unsatisfiable
- (B) φ has a model with exactly 3 elements
- (C) φ is satisfiable, but every model of φ is infinite
- (D) φ is valid

---

### Q6 · NAT · Moderate
The domain is {1, 2, 3}. The number of interpretations of the binary predicate R that make ∀x ∀y (R(x, y) → R(y, x)) ∧ ∀x ∃y R(x, y) true is ______.

---

### Q7 · MSQ · Moderate
In the structure whose domain is the set of positive integers and whose only predicate is divisibility "x | y", which of the following sentences is/are TRUE?

- (A) ∃x ∀y (x | y)
- (B) ∃x ∀y (y | x)
- (C) ∀x ∃y (x | y ∧ x ≠ y)
- (D) ∀x ∀y ∃z (x | z ∧ y | z)

---

### Q8 · NAT · Moderate
The domain is {1, 2, 3}, f is a unary function symbol and P is a unary predicate. The number of interpretations (f, P) that make ∀x (P(x) → P(f(x))) true is ______.

---

### Q9 · MCQ · Hard
In the following, W is a formula with no free occurrence of x. Which one of the equivalences is **NOT** logically valid?

- (A) ∀x (W → P(x)) ≡ (W → ∀x P(x))
- (B) ∃x (W ∧ P(x)) ≡ (W ∧ ∃x P(x))
- (C) ∃x (P(x) → W) ≡ (∃x P(x) → W)
- (D) ∀x (P(x) ∨ W) ≡ (∀x P(x) ∨ W)

---

### Q10 · NAT · Killer
The domain is {1, 2, 3, 4} and R is a binary predicate. The number of interpretations of R that make all of the following true is ______.

- ∀x ¬R(x, x)
- ∀x ∀y (x ≠ y → (R(x, y) ∨ R(y, x)))
- ∀x ∀y (R(x, y) → ¬R(y, x))
- ∀x ∀y ∀z (R(x, y) ∧ R(y, z) → R(x, z))

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | B | (A) forces everything to be a student; (C) is satisfied by a non-course y; (D) needs one course liked by all students |
| 2 | B | ¬∀∃ → ∃∀¬, and ¬(P → Q) ≡ P ∧ ¬Q |
| 3 | A, C | Countermodels exist for (B) and (D) on small domains |
| 4 | 27 | Each x picks a non-empty subset of the other two elements: 3³ |
| 5 | C | (ℕ, <) is a model; a finite serial strict order would need a cycle |
| 6 | 45 | 64 symmetric relations − 19 with an element having no R-neighbour (inclusion–exclusion 24 − 6 + 1) |
| 7 | A, C, D | (A) x = 1; (C) y = 2x; (D) z = xy; (B) there is no largest element |
| 8 | 117 | Sum over the 27 functions f of the number of f-closed subsets P |
| 9 | C | The correct form is ∃x P(x) → W ≡ ∀x (P(x) → W) |
| 10 | 24 | Strict total orders on 4 elements: 4! |
