# Theory of Computation → Context-Free Languages: Closure, Pumping Lemma & CYK

**GATE 2027 syllabus:** Theory of Computation — regular and context-free languages, pumping lemma.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** Σ = {a, b, c} where the letter c appears. Pumping lemma for CFLs: if L is context-free, there is a pumping length p such that every z ∈ L with |z| ≥ p can be written z = uvwxy with |vwx| ≤ p, |vx| ≥ 1, and uvⁱwxⁱy ∈ L for every i ≥ 0. A grammar is in Chomsky Normal Form (CNF) if every production is A → BC or A → a (plus possibly S → ε). The CYK algorithm fills a triangular table with one cell for every non-empty substring of the input.

---

## Questions

### Q1 · MCQ · Fundamental
Which one of the following correctly states the pumping lemma for context-free languages?

- (A) z = xyz with |xy| ≤ p, |y| ≥ 1 and xyⁱz ∈ L for all i ≥ 0
- (B) z = uvwxy with |vwx| ≤ p, |vx| ≥ 1 and uvⁱwxⁱy ∈ L for all i ≥ 0
- (C) z = uvwxy with |uvw| ≤ p, |v| ≥ 1 and uvⁱwxy ∈ L for all i ≥ 0
- (D) z = uvw with |v| ≥ 1 and uvⁱw ∈ L for all i ≥ 1

---

### Q2 · NAT · Fundamental
The CYK algorithm is run on an input string of length 8. The number of cells in its triangular table (one for each non-empty substring position) is ______.

---

### Q3 · MSQ · Standard
Which of the following languages is/are context-free?

- (A) {aⁿbⁿcᵐ : n, m ≥ 0}
- (B) {aⁿbⁿcⁿ : n ≥ 0}
- (C) {ww : w ∈ {a,b}*}
- (D) {wwᴿ : w ∈ {a,b}*}

---

### Q4 · NAT · Standard
A context-free grammar in Chomsky Normal Form generates a string of length 9. The number of derivation steps (production applications) in any derivation of that string is ______.

---

### Q5 · MCQ · Standard
If L₁ and L₂ are context-free languages, which one of the following is **not** necessarily context-free?

- (A) L₁ ∪ L₂
- (B) L₁ · L₂
- (C) L₁ ∩ L₂
- (D) L₁*

---

### Q6 · NAT · Moderate
A CNF grammar has the productions S → AB | AC, C → SB, A → a, B → b. The CYK algorithm is run on the input `aaabbb`. The number of table cells (out of the 21 cells) that contain the non-terminal S is ______.

---

### Q7 · MSQ · Moderate
Which of the following statements is/are TRUE?

- (A) If L is context-free and R is regular, then L ∩ R is context-free.
- (B) If L is context-free and R is regular, then L − R is context-free.
- (C) If L is context-free, then its complement is context-free.
- (D) Deterministic context-free languages are closed under complementation.

---

### Q8 · NAT · Moderate
The language L = {aⁱbʲcᵏ : i ≠ j or j ≠ k} (i, j, k ≥ 0) is context-free, being the union of two context-free languages. The number of strings of length exactly 12 in L is ______.

---

### Q9 · MSQ · Hard
Which of the following statements is/are TRUE?

- (A) To show that {aⁿbⁿcⁿ} is not context-free, one may take z = aᵖbᵖcᵖ, because any substring vwx of length at most p can contain at most two of the three distinct symbols.
- (B) The pumping lemma for context-free languages can be used to prove that a language **is** context-free.
- (C) {a^(n²) : n ≥ 0} is not context-free.
- (D) The complement of {ww : w ∈ {a,b}*} is context-free, even though {ww} itself is not.

---

### Q10 · NAT · Killer
Let L = {aⁿbⁿcⁿ : n ≥ 0} and take z = a³b³c³ (length 9) with pumping length p = 3. Consider **all** decompositions z = uvwxy with |vwx| ≤ 3 and |vx| ≥ 1 (two decompositions are different if the lengths of some part differ). Let D be the total number of such decompositions and P the number of them for which uv²wx²y ∈ L. The value of D + P is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | B | (A) is the pumping lemma for regular languages |
| 2 | 36 | 8·9/2 |
| 3 | A, D | (B) and (C) are the classic non-context-free languages |
| 4 | 17 | In CNF, a string of length n needs (n − 1) binary rules and n terminal rules: 2n − 1 |
| 5 | C | CFLs are closed under union, concatenation and Kleene star, but not intersection |
| 6 | 3 | S appears for the substrings ab, aabb and aaabbb (verified by running CYK; 11 cells are non-empty in total) |
| 7 | A, B, D | (C) is false; complement of a CFL need not be a CFL |
| 8 | 90 | All 91 triples (i, j, k) with i + j + k = 12 give a string of the form aⁱbʲcᵏ; only (4, 4, 4) is excluded |
| 9 | A, C, D | (B) is false — the pumping lemma only ever disproves context-freeness |
| 10 | 121 | For |vwx| = 1, 2, 3 there are 9, 8, 7 window positions and 2, 5, 9 ways to split the window into v, w, x with |vx| ≥ 1: D = 9·2 + 8·5 + 7·9 = 121; P = 0, since no decomposition survives pumping (both verified by brute force) |
