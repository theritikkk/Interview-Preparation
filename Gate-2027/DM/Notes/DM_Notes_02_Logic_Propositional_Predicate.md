# DM Notes 02 — Propositional & Predicate Logic (High-Yield Deep Dive)

> **Why this file exists:** "Logic" is the second video topic of Discrete Mathematics. GATE asks four things here: classify formulas (tautology/contradiction/contingency), check equivalences and entailments, *count* models/interpretations, and reason about quantifiers (translation, negation, small-domain counting). Every count below was **verified by running truth-table and finite-model enumerators** before writing.
>
> **Companion question banks:** `../DM_01_Propositional_Logic.md`, `../DM_02_First_Order_Logic.md`. **PW source:** `../Reference/PW_Subject_Handbooks/PW_DiscreteMath.txt` §2 (pages 2.11–2.17: connectives, laws, inference rules, quantifier rules).

---

## 0. What GATE actually asks

```
(A) Classification        tautology / contradiction / contingency / satisfiable
(B) Equivalences          rewrite formulas; which pairs are ≡
(C) Model counting        how many assignments satisfy φ; max simultaneous trues
(D) Normal forms          CNF/DNF/PDNF/PCNF; minimum clauses; resolution
(E) Completeness          functionally complete connective sets; monotone formulas
(F) FOL translation       "all/some/no/only if/unless" to ∀∃ patterns
(G) Quantifier algebra    ¬∀ ≡ ∃¬; valid distribution rules; W side-conditions
(H) Interpretations       count models of φ on a fixed finite domain
```

---

## 1. Propositional basics (PW §2.1, pages 2.11–2.12)

Connectives: ¬, ∧, ∨, →, ↔. The two you must be able to tabulate instantly:

| p | q | p → q | p ↔ q |
|---|---|-------|-------|
| 1 | 1 | 1 | 1 |
| 1 | 0 | **0** | 0 |
| 0 | 1 | 1 | 0 |
| 0 | 0 | 1 | 1 |

**Implication is false only in the T→F row.** Every English phrasing GATE uses:

- "if p then q", "p implies q", "p is sufficient for q", "q if p", "q whenever p", "q when p"
- "q is necessary for p", "a necessary condition for p is q", "q follows from p"
- **"p only if q"** = p → q (the direction *towards* q), "q unless ¬p" = p → q.

**Operator precedence (PW §2.1):** ¬ > ∧ > ∨ > → > ↔. So `p ∨ q → r ∧ s` means `(p ∨ q) → (r ∧ s)`.

**Classification (PW §2.1):** tautology = true in every row; contradiction = false in every row; contingency = both kinds of rows. Every tautology/contradiction is (un)satisfiable in the obvious way, and **every contingency is satisfiable but not valid**.

## 2. The law sheet and inference rules (PW §2.1)

**Equivalence laws (PW):** double negation; De Morgan ¬(p∧q) ≡ ¬p∨¬q, ¬(p∨q) ≡ ¬p∧¬q; commutative; associative; distributive (both ways); idempotent; identity (p∧⊤ ≡ p, p∨⊥ ≡ p); inverse (p∨¬p ≡ ⊤, p∧¬p ≡ ⊥); domination; absorption p∨(p∧q) ≡ p.

**Implication conversion power-list (PW pages 2.13–2.14) — high yield:**

```
p → q  ≡ ¬p ∨ q ≡ ¬q → ¬p ≡ ¬(p ∧ ¬q)      (contrapositive version!)
¬(p → q) ≡ p ∧ ¬q                          (NOT ¬p → ¬q !)
p ∨ q ≡ ¬p → q ;  p ∧ q ≡ ¬(p → ¬q)
(p → q) ∧ (p → r) ≡ p → (q ∧ r)            (right-∧ distribution)
(p → r) ∧ (q → r) ≡ (p ∨ q) → r            (left-∨ on the condition)
(p → q) ∨ (p → r) ≡ p → (q ∨ r)
(p → r) ∨ (q → r) ≡ (p ∧ q) → r            (note: ∧ here — the ∨-version is the trap)
p ↔ q ≡ (p → q) ∧ (q → p) ≡ (p ∧ q) ∨ (¬p ∧ ¬q) ≡ ¬p ↔ ¬q ;  ¬(p ↔ q) ≡ p ↔ ¬q
```

**Inference rules (PW §2.1, pages 2.13–2.14):** modus ponens [(p, p→q) ⇒ q]; modus tollens [(p→q, ¬q) ⇒ ¬p]; hypothetical syllogism [(p→q, q→r) ⇒ p→r]; disjunctive syllogism [(p∨q, ¬p) ⇒ q]; simplification [(p∧q) ⇒ p]; amplification [p ⇒ p∨q]; conjunction [(p, q) ⇒ p∧q]; proof by cases; constructive dilemma [(p→q, r→s, p∨r) ⇒ q∨s]; destructive dilemma.

## 3. Normal forms, minterms and resolution (standard GATE kit)

- **Literal** = variable or its negation; **clause** = OR of literals; **term** = AND of literals.
- **CNF** = AND of clauses (e.g. (¬p∨q) ∧ (p∨¬q) is p ↔ q). **DNF** = OR of terms.
- **PDNF/PCNF (full forms):** minterm uses *every* variable once; **the PDNF has exactly one minterm per satisfying row** — so "count minterms" = "count models". Maxterm = the negation of a minterm, false in exactly one row.
- **Minimum CNF intuition:** a clause is falsified on exactly one *subcube* of rows (fix the literals false); making φ equivalent to a CNF = **covering all falsifying rows by such subcubes**, each lying entirely inside the falsifying set. Minimum clauses is usually a small covering problem (watch: one clause can kill many rows, e.g. (p∨q) kills both (0,0,0) and (0,0,1)).

**Resolution:** from (A ∨ x) and (B ∨ ¬x) derive the resolvent (A ∨ B). Special cases: x is called a unit clause when A = B = ∅; derive the **empty clause □** ⟺ the clause set is unsatisfiable (refutation completeness). A resolvent containing x and ¬x is a tautology — discard it.

Worked trace (this clause set is *not* one of the practice questions):

```
{(p), (¬p ∨ q), (¬q ∨ r), (¬r)}
(p) ⊗ (¬p ∨ q)          ⟹ (q)        step 1   [unit resolution]
(q) ⊗ (¬q ∨ r)          ⟹ (r)        step 2
(r) ⊗ (¬r)              ⟹ □          step 3   ⟹ UNSAT
```

**Duality that powers everything:** φ ⊨ ψ ⟺ φ ∧ ¬ψ is unsatisfiable ⟺ φ → ψ is valid. To *test* entailment, always look for a row/model with φ true and ψ false.

## 4. Boolean functions and functional completeness

- There are **2^(2ⁿ) Boolean functions** of n variables (PW §2.2): 4 for n=1, 16 for n=2, 256 for n=3. (Formulas up to equivalence count the same.)
- **Functionally complete** sets can build every Boolean function: {¬, ∧}, {¬, ∨}, {¬, →}, {NAND}, {NOR}, {→, ⊥} all qualify. To *prove* a set complete, build ¬ and one of ∧/∨.
- **Incomplete sets and why:** {∧, ∨, →} — all three are *truth-preserving* (T,T ↦ T), so ¬ can never be produced; {∧, ∨} — only **monotone** functions (0→1 flips never break truth); {¬, ↔} — only affine/parity-type functions.
- Monotone counting in the small: Boolean functions of 2 variables that are monotone: 0, 1, p, q, p∧q, p∨q = 6; **formulas built from p, q using only ∧ and ∨** express exactly the *non-constant* ones = {p, q, p∧q, p∨q} = 4 (constants are unwritable without ¬ or a constant symbol).

## 5. Counting models (propositional) — the workhorse technique

1. **Split on one variable** and count each branch; exploit equivalences first. Example: models of p → q = 3 rows (all but (1,0)).
2. **Chains of implications constrain order:** the models of (x₁→x₂) ∧ (x₂→x₃) ∧ … ∧ (x_{k−1}→x_k) are exactly the **non-decreasing 0/1 words** x₁x₂…x_k (k+1 of them).
3. **Independence:** if φ = φ₁ ∧ φ₂ on disjoint variable sets, multiply counts (subtract the overlap carefully otherwise).
4. **Pairs with entailment:** count (f, g) with f ⊨ g **row by row** — on each row the allowed (f, g) values are (0,0), (0,1), (1,1) — 3 options. For 1-variable functions: 3² = 9 of the 16 pairs.

---

## 6. First-order logic: semantics and quantifier algebra (PW §2.2)

A structure fixes a **domain D**, meanings for predicates/functions/constants. ∀xφ = "φ holds for every element"; ∃xφ = "φ holds for at least one element". **Truth of a closed formula is checked one element at a time.**

**Negation duality (PW §2.2):** ¬∀xφ ≡ ∃x¬φ and ¬∃xφ ≡ ∀x¬φ. "Not all graphs are connected" = ∃x(G(x) ∧ ¬C(x)) — the ∧ appears because you negate the →.

**Valid distribution rules (≡, both directions):**

```
∀x (P(x) ∧ Q(x)) ≡ ∀xP(x) ∧ ∀xQ(x)          ∃x (P(x) ∨ Q(x)) ≡ ∃xP(x) ∨ ∃xQ(x)
W ∨ ∀xP(x) ≡ ∀x (W ∨ P(x))                   W ∧ ∃xP(x) ≡ ∃x (W ∧ P(x))       [W without x]
W → ∀xP(x) ≡ ∀x (W → P(x))                   ∃xP(x) → W ≡ ∀x (P(x) → W)       [quantifier FLIPS]
                                             ∀xP(x) → W ≡ ∃x (P(x) → W)       [quantifier FLIPS]
```

**One-way rules — valid ⟹ but NOT ≡:**

```
∃x (P(x) ∧ Q(x))  ⟹  ∃xP(x) ∧ ∃xQ(x)          (can't combine witnesses)
∀xP(x) ∨ ∀xQ(x)   ⟹  ∀x (P(x) ∨ Q(x))          (the ∨ version does NOT pull out)
∀x (P(x) → Q(x))  ⟹  (∀xP(x) → ∀xQ(x))
∃x∀y R(x,y)       ⟹  ∀y∃x R(x,y)               (swap x∃∀→∀∃ is safe; ∀∃→∃∀ is NOT)
```

Mnemonic for the flips: pulling a quantifier *into* an implication with the quantified atom on the LEFT of → flips the quantifier (same as negation does). For ∧ and ∨ it never flips and both directions are equivalences.

## 7. Translating English, and standard structures

| English | FOL |
|---|---|
| All S are P | ∀x (S(x) → P(x)) |
| Some S is P | ∃x (S(x) ∧ P(x)) |
| No S is P | ∀x (S(x) → ¬P(x)) ≡ ¬∃x (S(x) ∧ P(x)) |
| Not all S are P | ∃x (S(x) ∧ ¬P(x)) |
| Only S are P | ∀x (P(x) → S(x)) — "only" reverses the arrow |
| Every S has some T it likes | ∀x (S(x) → ∃y (T(y) ∧ L(x,y))) — ∃ goes INSIDE the → |

**∃∀ vs ∀∃:** ∃x∀y R = "one x works for every y"; ∀x∃y R = "each x has its own y". The first is stronger.

**Standard structure — (ℕ⁺, divisibility | ):** "1 | y" for all y (1 is a minimum); every x divides x and 2x ≠ x, so ∀x∃y (x|y ∧ x ≠ y) is TRUE; there is no maximum element, so ∃x∀y (y|x) is FALSE; x|y ∧ y|x ⟹ x = y (antisymmetry); x|y ∧ y|z ⟹ x|z (transitivity). Always instantiate a structure question with concrete numbers before choosing an option.

## 8. Counting interpretations on a finite domain (killer-question factory)

Totals for a domain of size k: unary predicate P → 2ᵏ choices; binary relation R → 2^(k²) choices; unary function f → kᵏ choices; pair (f, P) → kᵏ · 2ᵏ.

Methods, in order of usefulness:

1. **Fix the predicate(s), count compatible functions/relations independently per element** (product rule).
2. **Inclusion–exclusion** when the condition is "some element has property X" (∃-conditions) plus a global constraint (like seriality).
3. **Standard relation counts:** symmetric: 2^(k(k+1)/2); reflexive: 2^(k(k−1)); antisymmetric: 3^(k(k−1)/2).

Mini-example (fix-P method): domain {1,2}; count (f, P) with ∀x (P(x) ⟨condition between x and f(x)⟩) — enumerate the kᵏ functions, and for each f count the predicates P compatible with the condition element-wise. This is exactly the shape of every GATE interpretation-counting question.

---

## 9. Trap list (read before the exam)

1. ¬(p → q) = p ∧ ¬q, **not** ¬p → ¬q; contrapositive of p → q is ¬q → ¬p, not q → p.
2. "p only if q" = p → q; "p if q" = q → p. Direction matters.
3. p → q ≡ ¬p ∨ q: distribute ¬ correctly over the whole implication.
4. ⊨ is not →: entailment is "every model of the left is a model of the right".
5. Check (p → r) ∨ (q → r) ≠ (p ∨ q) → r — the ∨-version needs (p ∧ q).
6. ∀x(P ∨ Q) is NOT ∀xP ∨ ∀xQ; always test with P = {a}, Q = {b} on a 2-element domain.
7. ∃x(P ∧ Q) needs a SINGLE witness doing both — not two different witnesses.
8. Quantifier negation flips ∀↔∃ and changes → into ∧¬.
9. ∃x∀y ⇒ ∀y∃x only; the converse fails ("no largest number").
10. When pulling a quantifier past → with a W that lacks x, remember the flip (∃xP → W ≡ ∀x(P → W)); with ∧/∨ there is no flip.
11. In counting (f, P): functions are counted over the whole domain; predicates as subsets. Multiply independent choices; use inclusion–exclusion for ∃+global conditions.
12. Constants: {→} alone is not complete; {→, ⊥} is. Whether constants (⊤/⊥) are allowed changes the answer.
13. Resolution discards tautological resolvents; deriving □ proves unsatisfiability, not satisfiability.
14. "Unsatisfiable" vs "not valid": a formula can be satisfiable in some rows and false in others.

---

## 10. Practice questions (fresh — not in the banks)

### Q1 · MCQ · Fundamental

Which one of the following is a tautology?

- (A) (p → q) → p
- (B) p → (p → q)
- (C) (p → q) ∨ (q → r)
- (D) (p → q) ∧ (q → p)

### Q2 · NAT · Fundamental

The number of truth assignments to (p, q, r) that satisfy (p → q) ∧ (q ↔ r) is ______.

### Q3 · MSQ · Standard

Which of the following pairs of formulas is/are logically equivalent?

- (A) p → q and ¬q → ¬p
- (B) p ∧ (q ∨ r) and (p ∧ q) ∨ r
- (C) p → (q ∨ r) and (p → q) ∨ (p → r)
- (D) ¬(p → q) and p ∧ ¬q

### Q4 · NAT · Standard

Consider the clause set S = {(p ∨ q), (¬p ∨ q), (p ∨ ¬q), (¬p ∨ ¬q)}.

Convention: a resolution step picks two clauses containing a complementary pair of literals and adds their resolvent (keep all literals of both clauses except the complementary pair, remove duplicates, and discard the resolvent if it contains both a literal and its negation). The clause with no literals is written □.

The minimum number of resolution steps needed to derive □ from S is ______.

### Q5 · MSQ · Standard

Which of the following entailments hold?

- (A) {p → q, q → r} ⊨ p → r
- (B) {p ∨ q, ¬p} ⊨ q
- (C) {p → q} ⊨ q → p
- (D) {p ↔ q, ¬q} ⊨ ¬p

### Q6 · MCQ · Moderate

Which one of the following formulas is unsatisfiable? (⊕ denotes exclusive OR.)

- (A) (p → q) → (q → p)
- (B) (p ↔ q) ∧ (p ⊕ q)
- (C) (p ⊕ q) ∨ (p ↔ q)
- (D) (p ∧ ¬q) → (¬p ∨ q)

### Q7 · MSQ · Moderate

Which of the following equivalences involving unary predicates P and Q is/are logically valid?

- (A) ∀x (P(x) ∧ Q(x)) ≡ ∀x P(x) ∧ ∀x Q(x)
- (B) ∀x (P(x) ∨ Q(x)) ≡ ∀x P(x) ∨ ∀x Q(x)
- (C) ∃x (P(x) ∨ Q(x)) ≡ ∃x P(x) ∨ ∃x Q(x)
- (D) ∃x (P(x) → Q(x)) ≡ ∃x P(x) → ∃x Q(x)

### Q8 · NAT · Moderate

The domain is {1, 2}. f is a unary function symbol and P is a unary predicate. The number of interpretations (f, P) that make

∀x (P(x) ↔ P(f(x)))

true is ______.

### Q9 · MCQ · Hard

Which one of the following formulas is VALID (true in every structure with a non-empty domain)?

- (A) ∀x (P(x) ∨ Q(x)) → (∀x P(x) ∨ ∀x Q(x))
- (B) (∃x P(x) ∧ ∃x Q(x)) → ∃x (P(x) ∧ Q(x))
- (C) (∀x P(x) ∧ ∃x Q(x)) → ∃x (P(x) ∧ Q(x))
- (D) (∀x P(x) ↔ ∀x Q(x)) → ∀x (P(x) ↔ Q(x))

### Q10 · NAT · Killer

The domain is {1, 2, 3} and R is a binary predicate. The number of interpretations of R that make

∀x ∃y R(x, y) ∧ ∃x ∀y ¬R(y, x)

true is ______.

---

## 11. Answer key & full solutions

**Answer key:** 1-C · 2-3 · 3-A,C,D · 4-3 · 5-A,B,D · 6-B · 7-A,C · 8-10 · 9-C · 10-78

**Q1 — C.** (C): if q = 1 then (p → q) = 1; if q = 0 then (q → r) = 1 — so one disjunct is always true (8/8 rows, brute-force checked). (A) ✘ fails at p=0, q=0: (p→q) → p = 1 → 0 = 0. (B) ✘ fails at p=1, q=0: 1 → 0 = 0. (D) ✘ is just p ↔ q, fails at p=1, q=0.

**Q2 — 3.** (q ↔ r) forces q = r. Case q = r = 0: need p → 0, so p = 0 → (0,0,0). Case q = r = 1: p → 1 holds for both p → (0,1,1), (1,1,1). Total **3** (verified by enumeration).

**Q3 — A, C, D.** (A) ✔ contrapositive (PW). (B) ✘ p ∧ (q ∨ r) ≠ (p ∧ q) ∨ r: at p=0, q=0, r=1: LHS = 0, RHS = 1. (C) ✔ PW's (p→q) ∨ (p→r) ≡ p → (q ∨ r). (D) ✔ PW's ¬(p→q) ≡ p ∧ ¬q.

**Q4 — 3.** 3-step derivation: (p∨q) with (¬p∨q) gives (q); (p∨¬q) with (¬p∨¬q) gives (¬q); (q) with (¬q) gives □. It cannot be done in 2: every input clause has 2 literals, so any first resolvent still has ≥ 1 literal, and □ only arises by resolving two complementary **unit** clauses — deriving both units already takes 2 steps, then 1 more for □. Minimum = **3** (BFS over all derivations confirms).

**Q5 — A, B, D.** (A) ✔ hypothetical syllogism. (B) ✔ disjunctive syllogism. (C) ✘ countermodel p=0, q=1: premise 0→1 = 1, conclusion 1→0 = 0. (D) ✔ p ↔ q with ¬q forces p = 0, so ¬p.

**Q6 — B.** (p ↔ q) and (p ⊕ q) are each other's negation — no row can make both true. (A) satisfiable (p = q = 0). (C) is a tautology — one of ↔, ⊕ always true. (D) simplifies to ¬p ∨ q — satisfiable.

**Q7 — A, C.** (A) ✔ universal distributes over ∧. (C) ✔ existential distributes over ∨. (B) ✘ countermodel D = {1,2}, P = {1}, Q = {2}: LHS true, RHS (∀xP or ∀xQ) false. (D) ✘ countermodel D = {1,2}, P = {1}, Q = ∅: LHS true (x = 2 has ¬P), RHS "∃xP → ∃xQ" = T → F = false.

**Q8 — 10.** The condition says x and f(x) always receive the same P-value. Enumerate f (1→1, 1→2, 2→1, 2→2 as strings):
- f = identity (12): every P works — 4 choices.
- f = swap (21): need P(1) = P(2) — 2 choices (∅ or {1,2}).
- f = constant 1 (11): need P(1) = P(2) — 2. f = constant 2 (22): 2.
Total 4 + 2 + 2 + 2 = **10** (brute-force enumeration matches).

**Q9 — C.** (C) ✔ proof: pick a with Q(a) (exists); ∀xP gives P(a); hence P(a) ∧ Q(a) — one witness doing both. (A) ✘ D = {1,2}, P = {1}, Q = {2}: LHS true, RHS false. (B) ✘ same P, Q: both existentials true but no element is in both. (D) ✘ D = {1,2}, P = ∅, Q = {1}: ∀xP and ∀xQ are both false (LHS true) but P(1) ↔ Q(1) is false.

**Q10 — 78.** Count over the 2⁹ = 512 relations. First, seriality: each of the 3 rows must be a non-empty subset of the 3 columns — 2³ − 1 = 7 options per row → 7³ = 343 serial relations. Add "some column has no in-edge" (∃x∀y¬R(y,x)); let Aⱼ be the serial relations with column j empty. Row choices avoiding column j: 3 per row → |Aⱼ| = 3³ = 27; each Aᵢ ∩ Aⱼ: every row is forced to the single remaining column → 1 relation; triples: impossible (a row would be empty). Inclusion–exclusion: 3·27 − 3·1 = **78**. (Exhaustive check of all 512 relations agrees.)

---

## 12. Next steps

1. Memorize the boxed conversion list in §2 and the quantifier flip rules in §6 — they generate most equivalence/validity marks.
2. Practice the two counting moves: split-on-a-variable (propositional) and fix-predicates-then-count (FOL).
3. Run the banks `DM_01` and `DM_02` cold, then continue to **File 03 — Sets, Relations & Functions** (`DM_Notes_03_Sets_Relations_Functions.md`, banks `DM_03`–`DM_04`).
