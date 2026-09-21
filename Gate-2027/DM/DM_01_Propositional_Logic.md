# Discrete Mathematics → Propositional Logic

**GATE 2027 syllabus:** Engineering Mathematics — Discrete Mathematics: propositional and first-order logic.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** MCQ = exactly one correct option. MSQ = one or more correct options. NAT = numerical answer. ⊨ = logical consequence, ↔ = if and only if, ⊥ = the constant false.

---

## Questions

### Q1 · MCQ · Fundamental
Which one of the following is a tautology?

- (A) (p → q) → (q → p)
- (B) (p ∧ (p → q)) → q
- (C) (p ∨ q) → (p ∧ q)
- (D) ((p → q) ∧ (q → r)) → (r → p)

---

### Q2 · NAT · Fundamental
The number of truth assignments to (a, b, c, d) that satisfy (a → b) ∧ (b → c) ∧ (c → d) is ______.

---

### Q3 · MSQ · Standard
Which of the following pairs of formulas is/are logically equivalent?

- (A) ¬(p ↔ q) and p ↔ ¬q
- (B) p → (q → r) and (p → q) → r
- (C) p → (q ∧ r) and (p → q) ∧ (p → r)
- (D) (p ∨ q) → r and (p → r) ∨ (q → r)

---

### Q4 · NAT · Standard
Consider the six formulas

f₁ = p ∨ q, f₂ = p → r, f₃ = q → r, f₄ = ¬r, f₅ = p ↔ q, f₆ = p → ¬q.

The maximum number of these formulas that can be true simultaneously under a single truth assignment to p, q, r is ______.

---

### Q5 · MSQ · Standard
Which of the following sets of connectives is/are functionally complete? (⊥ is the constant false, ↑ is NAND.)

- (A) {∧, ∨, →}
- (B) {→, ⊥}
- (C) {¬, ↔}
- (D) {↑}

---

### Q6 · NAT · Moderate
The number of truth assignments to (a, b, c, d, e) that satisfy

(a ∨ b ∨ c) ∧ (¬a ∨ ¬b) ∧ (¬b ∨ ¬c ∨ d) ∧ (¬d ∨ e)

is ______.

---

### Q7 · NAT · Moderate
The minimum number of clauses in any CNF formula equivalent to (a ∧ b) ∨ (c ∧ d) ∨ (e ∧ f) is ______.

---

### Q8 · MSQ · Moderate
Which of the following statements is/are TRUE for arbitrary propositional formulas φ, ψ, χ?

- (A) If φ ⊨ ψ and ψ ⊨ χ, then φ ⊨ χ.
- (B) If φ ∨ ψ is valid, then φ is valid or ψ is valid.
- (C) If φ ∧ ψ is unsatisfiable, then φ is unsatisfiable or ψ is unsatisfiable.
- (D) If φ → ψ is valid and φ is satisfiable, then ψ is satisfiable.

---

### Q9 · NAT · Hard
Two Boolean functions f and g on two variables (so each is one of 2⁴ = 16 functions) satisfy f ⊨ g if g is true whenever f is true. The number of ordered pairs (f, g) with f ⊨ g (including f = g) is ______.

---

### Q10 · NAT · Killer
Formulas are built from the variables p, q, r using only ∧ and ∨ (no negation, no constants). The number of pairwise **non-equivalent** formulas that can be built is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | B | Modus ponens |
| 2 | 5 | 0000, 0001, 0011, 0111, 1111 |
| 3 | A, C | (B) and (D) fail: (D) should have ∧ |
| 4 | 5 | p = q = r = 0 makes every formula except f₁ true; f₁, f₂, f₃, f₄ cannot all hold |
| 5 | B, D | {∧, ∨, →} preserves truth; {¬, ↔} only produces parity-type (affine) functions |
| 6 | 13 | Count models clause by clause |
| 7 | 8 | 2 × 2 × 2 prime implicates (pick one variable from each conjunct), all essential |
| 8 | A, D | (B): p ∨ ¬p; (C): p and ¬p |
| 9 | 81 | Each of the 4 input rows takes (f, g) ∈ {(0,0), (0,1), (1,1)}: 3⁴ |
| 10 | 18 | Monotone functions on 3 variables (20) minus the two constants |
