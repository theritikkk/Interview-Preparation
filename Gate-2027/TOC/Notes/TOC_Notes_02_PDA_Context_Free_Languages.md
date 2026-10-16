# TOC Notes 02 — Pushdown Automata & Context-Free Languages (High-Yield Deep Dive)

> **Why this file exists:** "PDA" is on the high-yield list. GATE's CFL questions circle a few fixed ideas: **derivations & ambiguity**, **PDA acceptance mechanics/classes (NPDA vs DPDA)**, **the closure-property tables**, **counting strings/derivation trees**, and **normal forms (CNF/CYK)**. All practice answers verified by simulation.
>
> **Companion question banks:** `TOC_03_Context_Free_Grammars_Pushdown_Automata.md`, `TOC_04_Context_Free_Languages_Pumping_Lemma_CYK.md`. PW source: `../Reference/PW_Subject_Handbooks/PW_TOC.txt` (§3).

---

## 0. The mental model: one unbounded LIFO memory

A PDA = finite control + **one stack**. That buys exactly *one* matched count:

```
  aⁿbⁿ     ✔  push a's, pop on b's (one match)
  aⁿbⁿcⁿ   ✘   after matching aⁿ with bⁿ the stack is empty — no memory left for cⁿ
  w wᴿ     ✔  push w, guess the middle, pop against wᴿ (needs guessing → nondeterminism)
  w w      ✘  comparing in the SAME order needs a queue (two stacks/TM)
```

One stack = CFL. Two stacks = everything a Turing machine can do (PW §4.5: **TM ≡ two-stack PDA ≡ multi-stack PDA**).

---

## 1. Context-free grammars (CFG)

```
  G = (V, T, P, S)      every production:  V → (V ∪ T)*     (single variable on the LEFT)
```

- **Derivations:** Leftmost (LMD) / Rightmost (RMD) — expand the leftmost / rightmost variable each step.
- **Parse tree (non-linear derivation):** leaves read left-to-right = the string.
- **Ambiguity:** some string has ≥ 2 distinct parse trees (equivalently, ≥ 2 LMDs). A grammar can be ambiguous; a *language* is "inherently ambiguous" if every grammar for it is.

**Grammar → language reading (drill these two shapes):**

```
  S → aS | Sb | ε        ⇒  a*b*      (right-recursion adds leading a's, left adds trailing b's)
  S → aSb | ε            ⇒  {aⁿbⁿ | n ≥ 0}   (nesting pairs one a with one b — NOT regular)
```

Rule of thumb: productions of the form `V → T*V | T*` stay regular; **nesting a terminal around the variable (`aSb`) is exactly what needs a stack**.

---

## 2. Pushdown automata (PDA)

```
  NPDA:  M = (Q, Σ, Γ, δ, q0, Z0, F),   δ : Q × Σε × Γ → 2^(Q × Γ*)
  DPDA:  M = (Q, Σ, Γ, δ, q0, Z0, F),   δ : Q × Σε × Γ → Q × Γ*   (single option)
  Γ = stack alphabet; Z0 = bottom marker; a move may push a string (possibly ε = pop).
```

**Configuration / instantaneous description:** `(state, remaining input, stack contents)`; a move consumes one input symbol (or ε) and rewrites the top of the stack.

**Acceptance mechanisms (PW §3.4):**

| Mechanism | NPDA | DPDA |
|---|---|---|
| Final state | ✔ | ✔ |
| Empty stack | ✔ | ✘ (not an equivalent DPDA mechanism) |
| Final state + empty stack | ✔ | ✔ |

For **NPDAs all three are equivalent** (a CFL can be accepted by any of them); for **DPDAs** only the last-state conventions are used. Every DPDA is (behaves as) an NPDA; not conversely.

**Worked PDA for aⁿbⁿ (accept by empty stack):** single state q; transitions `(q,a,Z0)→(q,AZ0)`, `(q,a,A)→(q,AA)`, `(q,b,A)→(q,ε)`; accept when input is done and the stack is empty. Stack after aᵏ = `Z0 Aᵏ` (k+1 symbols); each b pops one A.

---

## 3. The language ladder: Regular ⊂ DCFL ⊂ CFL

```
  Regular        ⊂   DCFL          ⊂   CFL            ⊂ CSL ⊂ ...
  (FA)               (DPDA)            (NPDA)
  every regular      deterministic     needs guessing (or ambiguity)
  language is DCFL   parsing; closed   e.g. w wᴿ (middle unknown)
```

**PW's DCFL list (aⁿbⁿ-style comparisons are deterministic):**

```
  {aⁿbⁿ}, {aⁿb²ⁿ}, {a²ⁿbⁿ}, {aⁿbⁿc*}, {aⁿb*cⁿ}, {a*bⁿcⁿ}      → DCFL (one matched count)
  {aᵐbⁿc* : m ≠ n}, {m < n}, {m > n}, {m ≥ n}                 → DCFL
  {aᵐbⁿc^(m+n)}                                                → DCFL
  {a^(m+n) b^(n+k) c^(k+m)}                                    → CFL, NOT DCFL
  w wᴿ (w ∈ {a,b}*)                                            → CFL, NOT DCFL  (guess the middle)
  w c wᴿ (fixed center c)                                      → DCFL ✔ (center marks the turn)
```

**Not context-free at all (the canonical witnesses):**

```
  aⁿbⁿcⁿ   — two matched counts
  w w      — needs same-order comparison
  w # w    — same, marker doesn't help
```

Their standard proof shape (for membership questions): **intersect with a regular language to isolate counting**, e.g.

```
  {w : #a = #b = #c} ∩ a*b*c* = {aⁿbⁿcⁿ}  → not CFL ⇒ the larger language isn't CFL.
```

---

## 4. Closure properties (the tables that GATE quotes)

### CFL (PW §3.6)

```
  Closed:    union, concatenation, reversal, L*, L+, prefix, suffix, substring,
             substitution, homomorphism, ε-free homomorphism, inverse homomorphism,
             finite union / finite concatenation / finite subset / finite substitution,
             L ∪ Regular,  L − Regular
  NOT closed: intersection, complement, set difference, subset, quotient L1/L2,
             symmetric difference, finite intersection, finite difference,
             infinite union/intersection/difference/concatenation/substitution,
             Regular − L
```

Key combinations (PW §3.6 notes):

```
  CFL ∩ CFL       → need not be CFL       (aⁿbⁿc* ∩ a*bⁿcⁿ = aⁿbⁿcⁿ)
  CFL ∩ Regular   → CFL (need not be DCFL)
  CFL ∩ Finite    → Finite
```

### DCFL (PW §3.7)

```
  Closed:    complement (!), prefix, inverse homomorphism, finite subset,
             L ∪ Regular, L ∩ Regular, L − Regular, Regular − L
  NOT closed: union, intersection, concatenation, reversal, L*, L+, suffix, substring,
              substitution, homomorphism, quotient, symmetric difference, infinite ops...
```

**The headline asymmetry:** **DCFLs are closed under complement; CFLs are not.** (Classic MCQ.) Also: `DCFL ∪ DCFL` need not be DCFL — e.g., {aⁱbʲcᵏ : i = j} ∪ {aⁱbʲcᵏ : j = k} = {i = j **or** j = k}, which is the standard CFL-not-DCFL language (both parts are DCFL, the union is not).

More PW combination cells: DCFL ∪ CFL → CFL (need not be DCFL); DCFL ∩ CFL → need not be CFL; DCFL ∪ Finite → DCFL; DCFL ∩ Finite → Finite; DCFL − Finite → DCFL; Finite − DCFL → Finite.

---

## 5. Normal forms & parsing algorithms (method notes)

**Chomsky Normal Form (CNF):** every production is `A → BC` or `A → a`, plus possibly `S → ε`.

- Eliminate ε-productions, unit productions, useless symbols, then binarize.
- **A CNF derivation of a string of length n always uses exactly 2n − 1 steps** (n−1 internal-node productions + n terminal productions), and every parse tree has exactly 2n − 1 nodes.

**Greibach Normal Form (GNF):** `A → a α` (one terminal first) — used for leftmost-derivation arguments.

**CYK algorithm:** table DP over substrings; works **only for CNF**; table has **n(n+1)/2 cells** (all non-empty substrings of an n-symbol input); time **O(n³)**.

---

## 6. Counting patterns (how these questions are assembled)

```
  Palindromes of length k over {a,b}:      2^⌈k/2⌉   (halves are free, middle fixed if odd)
  Equal-count strings of length 2k:        C(2k, k) (choose positions of a's)
  Parse trees, n same-precedence operands: Catalan(n−1)   [Catalan(m) = C(2m,m)/(m+1)]
  {aⁱbʲ : 0 ≤ i ≤ j ≤ m}:                  (m+1)(m+2)/2  (pairs (i,j) with i ≤ j)
```

*Worked:* i+i+i+i with E → E+E | i ⇒ full binary trees with 4 leaves = Catalan(3) = **5**. "aaa" with A → AA | a ⇒ trees with 3 leaves = **2** (it's ambiguous!). Length-5 palindromes ⇒ 2³ = **8**.

---

## 7. Pumping lemma for context-free languages

```
  If L is context-free, ∃ p: every z ∈ L with |z| ≥ p splits as z = u v w x y with
      |vwx| ≤ p,   |vx| ≥ 1,   and  uvⁱwxⁱy ∈ L for all i ≥ 0.
```

- TWO blocks are pumped at once (v and x) — unlike the regular lemma's single y.
- The window `|vwx| ≤ p` is the workhorse: it can only cover ≤ p symbols, so for a language with long blocks it must miss one of them.
- **Skeleton: aⁿbⁿcⁿ is not CFL.** Pick z = aᵖbᵖcᵖ. Any uvwx with |vwx| ≤ p lies within at most two of the three runs (touching all three needs > p symbols? it can touch at most two runs **plus maybe** — simplest correct statement: vwx contains at most two of the three letter types). Pumping cannot add to exactly the missing letter; one count breaks. Case analysis (vwx in a's / b's / c's / across a-b / across b-c) → contradiction.
- Direction again: **necessary, not sufficient** — violating ⇒ not CFL; satisfying ⇒ inconclusive.

---

## 8. Traps & exam patterns

1. **Acceptance modes:** NPDA: final-state ≡ empty-stack ≡ both. DPDA: final-state (or both); **empty-stack alone is not a DPDA convention**.
2. **CFL is not closed under intersection** — but CFL ∩ Regular = CFL. "Not closed" ≠ "always non-CFL".
3. **Complement:** DCFL closed, CFL not. Union: CFL closed, DCFL not. (Opposite asymmetries — favorite MCQ.)
4. **w wᴿ vs w c wᴿ:** adding a middle marker upgrades CFL → DCFL. Without it, the middle can't be found deterministically.
5. **2n − 1 steps**: only valid after CNF conversion; trees in CNF are full binary.
6. **CYK = CNF only**, n(n+1)/2 cells, O(n³).
7. **Two stacks = TM** (PW equivalence list) — classification of machines with k stacks is a known question shape.
8. **Don't trust "looks like" classifications** — aᵐbⁿcᵏ with one constraint is DCFL; with "or" of two constraints it's CFL; with "and" (three-way equality) it's not CFL.
9. **Ambiguity is about grammars**, not strings; inherent ambiguity is a language property (the {i=j or j=k} language is the standard inherently-ambiguous CFL).

---

## 9. Practice set (every answer verified by simulation)

### Q1 · MCQ · Fundamental

The language generated by the grammar `S → aS | Sb | ε` is:

- (A) a*b*
- (B) (ab)*
- (C) {aⁿbⁿ | n ≥ 0}
- (D) (a+b)*

### Q2 · NAT · Standard

Grammar: `E → E + E | i`. The number of distinct **parse trees** (i.e., distinct derivations up to parenthesization) for the string `i+i+i+i` is ______.

### Q3 · MSQ · Standard

Which of the following statements is/are TRUE?

- (A) Every DCFL is a CFL, but not every CFL is a DCFL.
- (B) The language {w wᴿ : w ∈ {a,b}*} is a DCFL.
- (C) Every regular language is a DCFL.
- (D) The class of DCFLs is closed under complementation.

### Q4 · NAT · Fundamental

A PDA for {aⁿbⁿ | n ≥ 1} (accept by empty stack) uses `(q,a,Z0)→(q,AZ0)`, `(q,a,A)→(q,AA)`, `(q,b,A)→(q,ε)`. After reading the input `aaab` (i.e., a³b¹), the number of symbols on the stack (including the bottom marker Z0) is ______.

### Q5 · MCQ · Moderate

Which of the following grammars is in **Chomsky Normal Form**?

- (A) S → AB; A → AA | a; B → b
- (B) S → aB | bA; A → aS; B → bS
- (C) S → ASA | aB; A → b
- (D) S → abS | ε

### Q6 · NAT · Fundamental

The grammar `S → aSa | bSb | a | b | ε` generates exactly the palindromes over {a,b}. The number of strings of length exactly **5** that it generates is ______.

### Q7 · MSQ · Moderate

Which of the following is/are TRUE about pushdown automata?

- (A) For NPDAs, acceptance by final state, by empty stack, and by both are equivalent.
- (B) Every DPDA can be viewed as (is a special case of) an NPDA.
- (C) A PDA with **two** stacks has the same power as a Turing machine.
- (D) Since a PDA has a stack added to a finite automaton, it still accepts exactly the regular languages.

### Q8 · MCQ · Moderate

Which of the following languages is **NOT** a DCFL?

- (A) {aⁿbⁿ | n ≥ 0}
- (B) {w wᴿ | w ∈ {a,b}*}
- (C) {aᵐbⁿcᵏ : m = n, k ≥ 0}
- (D) {aᵐbⁿcᵏ : m ≠ n, k ≥ 0}

### Q9 · NAT · Moderate

Grammar: `A → AA | a`. The string `aaa` has how many distinct parse trees under this grammar (i.e., how many ways to parenthesize its derivation)? ______

### Q10 · NAT · Killer

Consider L = {aⁱbʲ : 0 ≤ i ≤ j ≤ 3}. The number of strings in L is ______.

---

## Answer Key

| Q | Answer | Why (condensed) |
|---|---|---|
| 1 | A | aS adds leading a's; Sb adds trailing b's |
| 2 | 5 | binary trees with 4 leaves = Catalan(3) |
| 3 | A, C, D | (B) false — wwᴿ is CFL but NOT DCFL (PW) |
| 4 | 3 | Z0 A A A, one pop for b |
| 5 | A | A→BC or A→a only |
| 6 | 8 | positions 1–3 free: 2³ |
| 7 | A, B, C | (D) false — strictly more than FA |
| 8 | B | middle of wwᴿ can't be found deterministically |
| 9 | 2 | (a a)a vs a(a a) |
| 10 | 10 | count pairs (i,j) with i ≤ j |

### Full solutions

**Q1.** `S → aS` prefixes any number of a's; `S → Sb` appends any number of b's; then `ε`. Result: **a*b* (A)**. (Check: "aab" = S ⇒ aS ⇒ aaS ⇒ aaSb ⇒ aab ✔; "ba" can never be produced since b's only appear once a's stop.) *(Verified: the grammar's derivations terminate exactly in a*b*.)*

**Q2.** With no precedence (single operator '+'), each parse tree = a way to fully parenthesize 4 operands = a full binary tree with 4 leaves. Count = Catalan(3) = C(6,3)/4 = 20/4 = **5**. *(Verified: ways(4) = 5 via the Catalan-like recurrence.)*

**Q3.** (A) ✔ — Regular ⊂ DCFL ⊂ CFL, both strict. (B) ✘ — PW explicitly lists w wᴿ as **CFL but not DCFL**: accepting it requires nondeterministically guessing the midpoint, and no DPDA can. (C) ✔ — every regular language is accepted by an FA, and an FA is a degenerate DPDA that ignores its stack (PW: every regular is DCFL). (D) ✔ — PW's DCFL closure table has Complement marked closed (and the analogous CFL entry is not — remember the asymmetry). Answer **A, C, D**.

**Q4.** After reading a³ the stack (bottom → top) is `Z0 A A A` (4 symbols). The single b pops one A → `Z0 A A` = **3** symbols. *(Verified by simulating the transitions.)*

**Q5.** CNF allows only `A → BC` (two variables) or `A → a` (one terminal). **(A)** ✔: S → AB, A → AA, B → b, A → a — all legal (it generates a⁺b). (B) mixes a terminal and a variable (aB, not CNF form). (C) has 3 symbols on the right (ASA). (D) has two terminals (abS) and an ε-rule without CNF justification. Answer **A**.

**Q6.** Even-indexed positions mirror the front; for length 5, positions 1, 2, 3 are free (positions 4, 5 copy positions 2, 1) → 2·2·2 = **8** palindromes (e.g., aacaa, ababa, ...). *(Verified by counting 2³.)*

**Q7.** (A) ✔ PW §3.4: all three acceptance mechanisms are equivalent for NPDAs. (B) ✔ PW's DPDA/PDA table — "every DPDA is PDA". (C) ✔ PW §4.5 equivalence list: TM ≡ two-stack PDA ≡ multi-stack PDA. (D) ✘ — the stack adds power: PDA languages = CFLs ⊋ regular. Answer **A, B, C**.

**Q8.** (B) is not DCFL (PW's canonical example [16]: w wᴿ needs an undeterministic midpoint guess). (A) is the flagship DCFL; (C) and (D) are on PW's DCFL list ([6], [9]). Answer **B**.

**Q9.** Two distinct derivations: `A ⇒ AA ⇒ aA ⇒ aAA ⇒ aaa` counts as (a)(aa) — i.e., trees ((a a) a) and (a (a a)). No other shapes for 3 leaves. Answer **2**. *(Verified via tree-count recurrence: ways(3) = 2.)* — This grammar is thus ambiguous (`aaa` has two trees).

**Q10.** Count pairs (i,j) with 0 ≤ i ≤ j ≤ 3:

```
  i=0: j = 0,1,2,3   (4)
  i=1: j = 1,2,3     (3)
  i=2: j = 2,3       (2)
  i=3: j = 3         (1)
  total = 10
```

→ **10** (the formula (m+1)(m+2)/2 = 4·5/2 = 10). *(Verified by enumeration.)* Note the language is finite → regular (a reminder that classification is about the largest class you *need*).

---

## 10. Next steps

1. Rebuild Q2 and Q9 counting from the Catalan idea; draw the 5 trees for i+i+i+i.
2. Memorize the two closure tables' headline cells: CFL (not ∧, ¬) vs DCFL (¬ closed, but ∪, ∧, ∘, * not).
3. Practice reps: `TOC_03_Context_Free_Grammars_Pushdown_Automata.md`, `TOC_04_Context_Free_Languages_Pumping_Lemma_CYK.md`.
4. Continue to `TOC_Notes_03_Language_Detection_Classification.md`.
