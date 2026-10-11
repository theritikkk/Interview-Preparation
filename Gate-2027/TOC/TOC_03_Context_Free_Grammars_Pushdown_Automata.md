# Theory of Computation → Context-Free Grammars & Pushdown Automata

**GATE 2027 syllabus:** Theory of Computation — context-free grammars and push-down automata.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** Σ = {a, b} unless stated otherwise; ε is the empty string; uppercase letters are non-terminals and S is the start symbol. A leftmost derivation always rewrites the leftmost non-terminal; each application of one production is one step. A PDA transition (q, x, T) → (p, γ) means: in state q, reading input symbol x (or ε, without consuming input) with T on top of the stack, move to state p and replace T by the string γ (written top-first). "Accepts by empty stack" means the input is fully consumed and the stack is empty.

---

## Questions

### Q1 · MCQ · Fundamental
A pushdown automaton (PDA) differs from a finite automaton mainly because it has

- (A) more than one start state
- (B) an unbounded stack that it can push to and pop from
- (C) the ability to move its input head backwards
- (D) a read/write tape

---

### Q2 · NAT · Fundamental
Using the grammar S → aSb | ε, the number of steps in a leftmost derivation of the string a⁵b⁵ is ______.

---

### Q3 · MSQ · Standard
Which of the following statements about context-free languages (CFLs) is/are TRUE?

- (A) Every regular language is a CFL.
- (B) CFLs are closed under union.
- (C) CFLs are closed under intersection.
- (D) CFLs are closed under complementation.

---

### Q4 · NAT · Standard
The grammar E → E + E | E * E | i is ambiguous. The number of distinct parse trees for the string `i+i*i+i` is ______.

---

### Q5 · NAT · Standard
A PDA has a single state q and start stack symbol Z, and accepts by empty stack. Its transitions are:

- (q, a, Z) → (q, AZ)
- (q, a, A) → (q, AA)
- (q, b, A) → (q, ε)
- (q, ε, Z) → (q, ε)

The number of strings of length exactly 8 over {a, b} that it accepts is ______.

---

### Q6 · NAT · Moderate
The grammar S → aSa | bSb | a | b | ε generates palindromes over {a, b}. The number of strings of length exactly 7 that it generates is ______.

---

### Q7 · MSQ · Moderate
Which of the following statements is/are TRUE?

- (A) A language can be context-free even though a particular grammar generating it is ambiguous.
- (B) There exist context-free languages that are inherently ambiguous (every grammar for them is ambiguous), for example {aⁱbʲcᵏ : i = j or j = k}.
- (C) It is decidable whether an arbitrary given context-free grammar is ambiguous.
- (D) Every context-free language not containing ε has a grammar in Chomsky Normal Form.

---

### Q8 · NAT · Moderate
The language L = {aⁱbʲcᵏ : i, j, k ≥ 0, and (i = j or j = k)} is context-free. The number of strings of length exactly 9 in L is ______.

---

### Q9 · MSQ · Hard
Which of the following statements about pushdown automata is/are TRUE?

- (A) PDAs that accept by final state and PDAs that accept by empty stack recognise exactly the same class of languages.
- (B) Deterministic PDAs (DPDAs) are as powerful as nondeterministic PDAs.
- (C) The language {wwᴿ : w ∈ {a,b}*} is accepted by some nondeterministic PDA but by no deterministic PDA.
- (D) A PDA equipped with two independent stacks can recognise only context-free languages.

---

### Q10 · NAT · Killer
The grammar S → aSb | bSa | SS | ε generates exactly the strings over {a, b} that contain equally many a's and b's. The number of strings of length exactly 8 that it generates is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | B | The stack is what lets a PDA recognise languages like aⁿbⁿ |
| 2 | 6 | Five applications of S → aSb, then one application of S → ε |
| 3 | A, B | CFLs are not closed under intersection or complement (e.g. {aⁿbⁿcⁿ} is an intersection of two CFLs) |
| 4 | 5 | Three operators give the Catalan number C₃ = 5 (verified by counting derivations) |
| 5 | 14 | The PDA accepts exactly the balanced-bracket words with a as "open" and b as "close"; there are C₄ = 14 of length 8 (verified by simulation) |
| 6 | 16 | A palindrome of length 7 is fixed by its first 4 symbols: 2⁴ (verified by enumeration) |
| 7 | A, B, D | (C) is false — ambiguity of CFGs is undecidable |
| 8 | 9 | i = j: 5 strings; j = k: 5 strings; overlap i = j = k = 3: 1 string; 5 + 5 − 1 |
| 9 | A, C | (B) false: DPDAs are strictly weaker; (D) false: two stacks give Turing-machine power |
| 10 | 70 | All strings of length 8 with four a's and four b's: C(8,4) (verified by enumeration up to length 8) |
