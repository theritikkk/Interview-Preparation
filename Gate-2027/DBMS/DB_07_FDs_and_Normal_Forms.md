# DBMS → Functional Dependencies & Normal Forms

**GATE 2027 syllabus:** Databases — integrity constraints, normal forms.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** X⁺ is the attribute closure of X. Normal forms considered: 1NF, 2NF, 3NF, BCNF. A prime attribute belongs to some candidate key. Decompositions are judged with respect to the given set F of functional dependencies.

---

## Questions

### Q1 · NAT · Fundamental
R(A, B, C, D, E) with F = {A → B, B → C, C → D}. The number of attributes in {A}⁺ is ______.

---

### Q2 · NAT · Fundamental
R(A, B, C, D) with F = {A → B, B → C, C → D, D → A}. The number of candidate keys of R is ______.

---

### Q3 · MCQ · Standard
R(A, B, C, D) with F = {AB → CD, C → B}. The highest normal form R satisfies is

- (A) 1NF
- (B) 2NF
- (C) 3NF
- (D) BCNF

---

### Q4 · NAT · Standard
R(A, B, C, D, E) with F = {A → B, B → C, C → A, D → E}. The number of candidate keys of R is ______.

---

### Q5 · MSQ · Standard
F = {A → B, B → C, CD → E} holds on R(A, B, C, D, E). Which of the following functional dependencies is/are implied by F?

- (A) A → C
- (B) AD → E
- (C) B → A
- (D) BD → E

---

### Q6 · NAT · Moderate
F = {A → BC, B → C, A → B, AB → C}. The number of functional dependencies in a minimal (canonical) cover of F, with a single attribute on every right-hand side, is ______.

---

### Q7 · NAT · Moderate
R(A, B, C, D, E) with F = {AB → C, C → A, D → E}. The number of superkeys of R is ______.

---

### Q8 · MSQ · Moderate
R(A, B, C, D) with F = {A → B, B → C, C → D}. Which of the following decompositions is/are both lossless-join and dependency-preserving?

- (A) {AB, BC, CD}
- (B) {ABC, CD}
- (C) {AB, CD}
- (D) {ABD, BC}

---

### Q9 · NAT · Hard
R(A, B, C, D, E, G) with F = {A → BC, B → C, AB → D, D → E, E → D}. The 3NF synthesis algorithm is applied: (1) compute a canonical cover; (2) create one relation per left-hand side (with all the attributes of its dependencies), discarding duplicate relations; (3) if no relation contains a candidate key of R, add a relation consisting of a candidate key; (4) discard any relation that is a subset of another. The number of relations produced is ______.

---

### Q10 · MSQ · Killer
Which of the following statements is/are TRUE?

- (A) Every relation with exactly two attributes is in BCNF.
- (B) A relation that is in 3NF and has exactly one candidate key is in BCNF.
- (C) Every relation has a lossless-join, dependency-preserving decomposition into 3NF relations.
- (D) Every relation has a lossless-join, dependency-preserving decomposition into BCNF relations.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | 4 | {A, B, C, D} |
| 2 | 4 | Each single attribute is a key |
| 3 | C | Keys AB, AC; C → B has B prime but C is not a superkey |
| 4 | 3 | AD, BD, CD |
| 5 | A, B, D | (C) B⁺ = {B, C} |
| 6 | 2 | {A → B, B → C} |
| 7 | 6 | Keys ABD and BCD; 4 + 4 − 2 supersets |
| 8 | A, B | (C) is lossy; (D) loses C → D |
| 9 | 4 | Cover {A→B, A→D, B→C, D→E, E→D}: relations ABD, BC, DE; G is in every key, so add AG |
| 10 | A, B, C | (D): e.g. R(A, B, C) with AB → C and C → B has no dependency-preserving BCNF decomposition |
