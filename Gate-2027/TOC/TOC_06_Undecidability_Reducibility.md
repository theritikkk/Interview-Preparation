# Theory of Computation → Undecidability & Reducibility

**GATE 2027 syllabus:** Theory of Computation — Turing machines and undecidability.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** A language is decidable (recursive) if some Turing machine always halts and correctly accepts/rejects it. A₁ ≤ₘ A₂ (many-one reducible) means there is a computable function f such that x ∈ A₁ ⇔ f(x) ∈ A₂; if A₁ ≤ₘ A₂ and A₂ is decidable, then A₁ is decidable (equivalently, if A₁ is undecidable, so is A₂). A "trivial" property of the language recognized by a TM is one that either all TMs have or no TM has.

---

## Questions

### Q1 · MCQ · Fundamental
The Halting Problem — deciding whether a given Turing machine halts on a given input — is

- (A) decidable, by simulating the machine for a bounded number of steps
- (B) undecidable, but its complement is decidable
- (C) undecidable, and it is recursively enumerable (semi-decidable): a TM can halt and accept whenever the simulated machine halts, but may run forever otherwise
- (D) not even recursively enumerable

---

### Q2 · NAT · Fundamental
A Post Correspondence Problem (PCP) instance has three tile pairs: (top, bottom) = (a, baa), (ab, a), (bba, bb), indexed 1, 2 and 3. A solution is a sequence of indices (repetition allowed) such that concatenating the tops in that order equals concatenating the bottoms in that order. One valid solution is the sequence 2, 3, 1. The length of the matched string (top = bottom) produced by this sequence is ______.

---

### Q3 · MSQ · Standard
Which of the following statements is/are TRUE?

- (A) The Post Correspondence Problem is undecidable in general.
- (B) Some specific PCP instances are solvable, and a solution (if the instance is solvable) can be found by exhaustive search — it is only the general decision problem (over all instances) that is undecidable.
- (C) Every PCP instance where at least one tile has top = bottom trivially has a solution.
- (D) Determining whether a *specific, given* candidate sequence of indices is a valid solution to a PCP instance is itself decidable (just concatenate and compare).

---

### Q4 · NAT · Standard
Consider the following four problems about an arbitrary Turing machine M: (1) Does M halt on the empty input? (2) Does M halt on every input? (3) Is L(M) empty? (4) Does M have at least 5 states? The number of these four problems that are **undecidable** is ______.

---

### Q5 · MCQ · Standard
**Rice's theorem** states that

- (A) every non-trivial property of the language recognized by a Turing machine is undecidable
- (B) every property of a Turing machine's internal structure (like its number of states) is undecidable
- (C) only the halting problem and problems reducible from it are undecidable
- (D) all properties of recursively enumerable languages are decidable except emptiness

---

### Q6 · NAT · Moderate
Consider the following six problems about Turing machines: (1) L(M) = ∅? (2) L(M) = Σ*? (3) L(M) is regular? (4) L(M) is finite? (5) M has an even number of states? (6) M halts within 100 steps on a specific given input w? Using Rice's theorem (for language properties) and direct reasoning (for the others), the number of these six problems that are **decidable** is ______.

---

### Q7 · MSQ · Moderate
Which of the following statements about reductions is/are TRUE?

- (A) If A ≤ₘ B and B is decidable, then A is decidable.
- (B) If A ≤ₘ B and A is undecidable, then B is undecidable.
- (C) If A ≤ₘ B and A is decidable, then B is decidable.
- (D) The Halting Problem is many-one reducible to the problem "does TM M halt on the empty input," via an appropriate computable transformation of the instance.

---

### Q8 · NAT · Moderate
Consider the following five languages, where M ranges over Turing-machine descriptions: L1 = {⟨M⟩ : L(M) = ∅}, L2 = {⟨M⟩ : L(M) is finite}, L3 = {⟨M⟩ : L(M) = Σ*}, L4 = {⟨M, w⟩ : M halts on w}, L5 = {⟨M⟩ : M has a useless (unreachable) state}. The number of these five languages that are undecidable is ______.

---

### Q9 · MSQ · Hard
Which of the following statements is/are TRUE?

- (A) The complement of the Halting Problem's language (the set of ⟨M,w⟩ where M does **not** halt on w) is not recursively enumerable.
- (B) A language L is decidable if and only if both L and its complement are recursively enumerable.
- (C) The set of all Turing machine descriptions is countable, while the set of all languages over a fixed alphabet is uncountable — hence there exist languages with no Turing machine (undecidable in the strongest sense: not even recognizable).
- (D) Diagonalization is one classical technique used to prove the Halting Problem is undecidable, by assuming a decider exists and constructing a machine that contradicts it on its own description.

---

### Q10 · NAT · Killer
The following problems about a Turing machine M are considered: (1) L(M) = ∅ (undecidable); (2) M has at most 10 states (decidable); (3) L(M) is context-free (undecidable, by Rice's theorem — it's a non-trivial language property); (4) M halts on input w within exactly 50 steps (decidable — simulate 50 steps); (5) L(M) = L(M'), for a second fixed TM M' (undecidable, by Rice's theorem, unless the target property is trivial, and equivalence to a fixed non-trivial language is non-trivial); (6) M's transition table has more than 3 duplicate rows (decidable — direct inspection). Among these 6 problems, let U be the count of undecidable ones and D the count of decidable ones. The value of U × D is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | C | The Halting Problem is the canonical undecidable-but-RE language |
| 2 | 6 | top: "ab"+"bba"+"a" = "abbbaa"; length 6 (verified by direct concatenation) |
| 3 | A, B, C, D | All four are correct — a tile with top = bottom is itself a valid one-tile solution, so (C) holds too |
| 4 | 3 | (1), (2), (3) are all undecidable (Rice's theorem / halting-related); (4) is decidable — it's a direct, mechanical property of M's description |
| 5 | A | The precise statement of Rice's theorem |
| 6 | 2 | (1)–(4) are non-trivial language properties, undecidable by Rice's theorem; (5) and (6) are decidable — both are direct, mechanical checks on M's description, not language properties |
| 7 | A, B, D | (C) is false — knowing A is decidable tells us nothing about B in this direction |
| 8 | 4 | L1, L2, L3 are undecidable (Rice's theorem, non-trivial language properties); L4 is undecidable (it's the Halting Problem itself); L5 is decidable (a direct, mechanical structural check on M's description) |
| 9 | A, B, C, D | All four are standard, correct facts about undecidability and diagonalization |
| 10 | 9 | Undecidable: (1), (3), (5) → U=3; Decidable: (2), (4), (6) → D=3; U×D = 3×3 |
