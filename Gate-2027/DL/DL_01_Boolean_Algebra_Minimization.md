# Digital Logic → Boolean Algebra & Minimization (K-Map, Tabular/Quine–McCluskey)

**GATE 2027 syllabus:** Digital Logic — Boolean algebra and minimization: algebraic technique, Karnaugh map, tabular method.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** Functions are given as a sum of minterms, Σm(...), optionally with don't-care terms Σd(...). A **prime implicant (PI)** is a product term that cannot be combined with another to form a larger (more general) implicant. An **essential** PI is one that is the only PI covering some minterm. The tabular (Quine–McCluskey) method systematically finds all PIs, then selects a minimum-cost subset covering all minterms (don't cares may be used to form PIs but need not be covered in the final selection).

---

## Questions

### Q1 · MCQ · Fundamental
A prime implicant of a Boolean function is a product term that

- (A) appears in every minimal SOP expression for the function
- (B) cannot be combined with another implicant to form a larger (fewer-literal) implicant that still implies the function
- (C) covers exactly one minterm
- (D) is always the same as an essential prime implicant

---

### Q2 · NAT · Fundamental
The Boolean expression F = AB + AB̄ simplifies to a single literal. That literal, evaluated as a function of the variables present, is ______  (answer 1 for "A", 2 for "B", to indicate which single variable it reduces to).

---

### Q3 · MSQ · Standard
Which of the following statements about Karnaugh maps is/are TRUE?

- (A) Adjacent cells in a K-map differ in exactly one variable (Gray-code adjacency).
- (B) A group of 2ᵏ adjacent 1-cells (a valid power-of-2 rectangle, including wraparound) corresponds to a product term with k fewer literals than the number of variables.
- (C) Every prime implicant corresponds to a K-map grouping that cannot be enlarged (by doubling to an adjacent valid power-of-2 group) while staying entirely within 1-cells (or don't-care cells).
- (D) The K-map method and the tabular (Quine–McCluskey) method, applied correctly, always identify exactly the same set of prime implicants for a given function, since prime implicants are a property of the function itself, not of the method.

---

### Q4 · NAT · Standard
For F(A,B,C,D) = Σm(0,2,5,7,8,10,13,15), the tabular method finds exactly 2 prime implicants, and both are essential. The total number of literals in the minimal SOP expression (summing the literals across both terms) is ______.

---

### Q5 · MCQ · Standard
A function F(A,B,C,D) = Σm(0,1,2,5,6,7,8,9,10,14) + Σd(3,11,12,13,15) is minimized using don't cares. Which of the following is the correct minimal SOP expression?

- (A) C + D + B̄
- (B) CD̄ + B̄C̄ + BDĀ
- (C) AB + CD
- (D) A + B + C + D

---

### Q6 · NAT · Moderate
For F(A,B,C) = Σm(0,1,2,5,6,7) (3 variables, no don't cares), the tabular method finds 6 prime implicants in total. The number of these that are **essential** is ______.

---

### Q7 · MSQ · Moderate
Which of the following statements about essential prime implicants (EPIs) is/are TRUE?

- (A) Every minimal SOP expression must include all essential prime implicants.
- (B) A function can have zero essential prime implicants, requiring a more careful (e.g., Petrick's method or exhaustive) selection among the remaining prime implicants to find a minimum cover.
- (C) If including all essential PIs already covers every minterm, no further prime implicants are needed.
- (D) The number of essential prime implicants can exceed the number of variables in the function.

---

### Q8 · NAT · Moderate
For F(A,B,C) = Σm(0,1,2,5,6,7) (same function as Q6, with 0 essential prime implicants), the **minimum** number of prime implicants needed to cover all 6 minterms is ______.

---

### Q9 · MSQ · Hard
Which of the following statements about minimization is/are TRUE?

- (A) A minimal SOP expression for a function is always unique.
- (B) Using don't-care terms in minimization can only ever reduce (or leave unchanged) the number of literals needed, never increase it, compared to not using them.
- (C) The tabular (Quine–McCluskey) method is guaranteed to find a minimum-literal SOP expression, unlike a hastily-drawn K-map grouping which can miss the true minimum if groups aren't chosen as large as possible.
- (D) POS (product of sums) minimization can be performed by minimizing the 0s of the function (treating them as the "minterms" of the complement) and then complementing the result.

---

### Q10 · NAT · Killer
For F(A,B,C,D) = Σm(0,2,5,7,8,10,13,15) (same function as Q4) and G(A,B,C,D) = Σm(0,1,2,5,6,7,8,9,10,14) + Σd(3,11,12,13,15) (same function as Q5), let P = (total literals in F's minimal SOP expression) and Q = (total literals in G's minimal SOP expression). The value of P + Q is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | B | Definition of a prime implicant |
| 2 | 1 | AB + AB̄ = A(B + B̄) = A |
| 3 | A, B, C, D | All four are standard, correct facts — prime implicants are intrinsic to the function, so both methods must find the same set |
| 4 | 4 | F minimizes to BD + B̄D̄ (verified by Quine–McCluskey: 2 essential PIs, each a 2-literal term); 2 + 2 |
| 5 | A | Verified via SOPform with don't cares: simplifies to C + D + B̄ |
| 6 | 0 | Verified by Quine–McCluskey: this is a classic "cyclic" prime-implicant-chart example with no essential PIs |
| 7 | A, B, C | (D) is false — the number of EPIs is bounded by practical considerations of the function, not guaranteed to exceed the variable count, and in general there's no such lower bound relationship implied |
| 8 | 3 | Verified by exhaustive set-cover search: the minimum cover uses exactly 3 of the 6 prime implicants |
| 9 | B, C, D | (A) is false — cyclic charts (like Q6/Q8) can have multiple, equally minimal SOP expressions |
| 10 | 7 | F = BD + B̄D̄ (4 literals, from Q4); G = C + D + B̄ (3 literals, from Q5); 4 + 3 |
