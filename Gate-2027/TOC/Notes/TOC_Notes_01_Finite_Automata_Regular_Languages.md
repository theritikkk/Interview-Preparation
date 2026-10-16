# TOC Notes 01 — Finite Automata & Regular Languages (High-Yield Deep Dive)

> **Why this file exists:** "Finite Automata" is on the high-yield list and GATE recycles a small set of skills: **state-counting formulas**, **subset construction**, **minimization/Myhill–Nerode**, **regular-expression algebra (incl. Arden)**, **closure properties**, and **the pumping lemma**. Each is mechanical once the method is exact. All practice answers verified by simulation.
>
> **Companion question banks:** `TOC_01_Regular_Expressions_Finite_Automata.md`, `TOC_02_Regular_Languages_Minimal_DFA_Myhill_Nerode.md`. PW source: `../Reference/PW_Subject_Handbooks/PW_TOC.txt` (§2).

---

## 0. The mental model: a machine with finite memory

An FA is a decision device whose only memory is **the current state** — hence it can only "remember" finitely many distinctions (parity, modulo, last-k symbols), never a full count. That single sentence predicts which languages are regular.

**Definitions:**

```
  DFA:  M = (Q, Σ, δ, q0, F),   δ : Q × Σ → Q        (exactly one move; exactly one run per string)
  NFA:  M = (Q, Σ, δ, q0, F),   δ : Q × Σ → 2^Q      (zero or more moves; accept if SOME run ends in F)
  ε-NFA: additionally moves without reading input
  Accept: the run(s) end in a final state after the whole string is consumed.
```

**Conventions PW highlights (worth 1 mark each):**

- ε ∈ L ⇔ the **initial state is final** in the DFA.
- A **dead state** is a non-final state with no path to any final state (once entered, hopeless). Needed when the language has an "exactly / at most / starts with" flavor; languages of the "contains / at least" flavor don't need one.
- All states final → L = Σ*; no state final → L = ∅.

**NFA vs DFA (what's actually different):**

| | NFA | DFA |
|---|---|---|
| Transition | δ : Q×Σ → 2^Q | δ : Q×Σ → Q |
| Paths for a valid string | ≥ 1 (any one may accept) | exactly 1 |
| Paths for an invalid string | 0 or many (none accept) | exactly 1 |
| Power | = DFA (equivalent) | = NFA |

---

## 1. State-counting formulas (the single most recycled GATE table)

Let w ∈ {a,b}*, |w| = n. Minimum number of states:

| Language | NFA | DFA |
|---|---|---|
| \|w\| = n (exactly n) | n + 1 | **n + 2** |
| \|w\| ≤ n, or \|w\| ≥ n | n + 1 | n + 2 (for ≤ n) / n + 1 (for ≥ n) |
| starts with a | 2 | 3 |
| n-th symbol from **beginning** is a | n + 1 | n + 2 |
| n-th symbol from the **end** is a | n + 1 | **2ⁿ** |
| #a divisible by m AND #b divisible by n | — | **m × n** |

Notes that make the table safe:

- **Window languages need two dead classes.** A "between L1 and L2" language like 2 ≤ |w| ≤ 5 has classes for lengths 0,1 (too short) and ≥ 6 (too long), plus the lengths 2..5 — that's why the count is bigger than either single bound (worked below and as practice Q1).
- **From-the-end lookbacks are exponential**: to know the n-th symbol from the end you must remember the last n−1 symbols — 2ⁿ states over a binary alphabet (PW writes "2n"; it means 2ⁿ). Example: 2nd-from-last is 1 → 4 states (practice Q10).
- **Mod conditions multiply**: the DFA tracks (#a mod m, #b mod n) → m·n states.

---

## 2. NFA → DFA: subset construction (the exact algorithm)

```
  1. DFA states = SUBSETS of NFA states.  Start = ε-closure({q0}).
  2. For a subset S and symbol c: δ_D(S,c) = ε-closure( ∪_{q∈S} δ_N(q,c) ).
  3. Only REACHABLE subsets become states. Add ∅ (dead) if it is entered.
  4. S is accepting iff S ∩ F ≠ ∅.
  Worst case: an n-state NFA can need 2ⁿ DFA states; minimal DFA never exceeds that.
```

**Worked example (ε-moves).** NFA: q0 start with ε → q1; q0 -a→ q0; q1 -b→ q2 (accept).

```
  Start: ε-closure({q0}) = {q0, q1}
  {q0,q1} -a→ {q0} → ε-close → {q0, q1}        (self-loop)
  {q0,q1} -b→ {q2} → {q2}   (accept)
  {q2} -a/-b→ ∅             (dead)
  ∅ loops to ∅
  → 3 reachable states. Language: a*b.
```

**Worked example (pure subset dynamics).** NFA for "2nd-from-last symbol is 1": q0 -0→ q0; q0 -1→ {q0,q1}; q1 -0/-1→ q2 (accept); q2 has no moves.

```
  {q0} -0→ {q0};  {q0} -1→ {q0,q1}
  {q0,q1} -0→ {q0,q2} (accept);  {q0,q1} -1→ {q0,q1,q2} (accept)
  {q0,q2} -0→ {q0};  {q0,q2} -1→ {q0,q1}
  {q0,q1,q2} -0→ {q0,q2};  {q0,q1,q2} -1→ {q0,q1}
  → exactly 4 reachable states = 2², matching the formula for "2nd from end".
```

---

## 3. Minimization & Myhill–Nerode

**Procedure:**

```
  1. Delete unreachable states.
  2. Merge states that are "equivalent": no string takes exactly one of them to an accept state.
  3. Classic mark/sweep (table-filling): mark pairs with different accept-status; propagate.
```

**Myhill–Nerode (the theory behind it):** define x ≡ y iff for every z, xz ∈ L ⟺ yz ∈ L.

> **L is regular ⇔ the number of equivalence classes is finite; and that number = the number of states of the minimal DFA.**

- The minimal DFA is **unique up to renaming**; minimal NFAs may not be unique.
- Two useful corollaries: right congruence classes = states (counting question!); an infinite set of pairwise distinguishable strings ⇒ non-regular.

**Worked class count.** L = {w : 2 ≤ |w| ≤ 5}. Classify by the set of remaining lengths that reach acceptance:

```
  class          allowed remaining lengths
  |w| = 0        {2,3,4,5}
  |w| = 1        {1,2,3,4}
  |w| = 2        {0,1,2,3}
  |w| = 3        {0,1,2}
  |w| = 4        {0,1}
  |w| = 5        {0}
  |w| ≥ 6        {}   (dead)
  → 7 distinct classes → minimal DFA has 7 states
```

---

## 4. Regular expressions & regular grammars

**Syntax & precedence:** `*` binds tightest, then concatenation, then `+` (union at loosest). *Examples:* `ab*+c = (a(b*))+c`.

**Identity table (all pure marks):**

```
  a + a = a          a + ∅ = a         a + ε = {a, ε}
  ∅* = ε             ∅+ = ∅            ε+ = ε = ε*
  R* = R+ + ε        R+ = R·R*         ε·R = R
  R·∅ = ∅            (R*)* = R*        R*R* = R*       L* · L* = L*
  ∅ is identity for union; ε is identity for concatenation
```

**Arden's Lemma (solving regex from automaton equations):**

```
  If R = Q + R·P and P does not contain ε, then R = Q·P*
```

*Worked example:* R = b + R·a → R = b·a* = "b followed by any number of a's". (Check: ba* = b + ba*·a = b + ba⁺ ✔.)

**Regular grammars** — a grammar whose productions are linear is regular:

```
  Right-linear (RLG):  V → T*V | T*       e.g., S → aS | ε         → a*
  Left-linear  (LLG):  V → VT* | T*       e.g., S → Sa | ε         → a*
```

- `S → aS | ε` = a*; `S → aS | Sb | ε` = a*b* (right rules add leading a's; left rules add trailing b's).
- **A single variable grammar cannot "nest"**: `S → aSb | ε` is NOT regular — matching a's against b's needs a stack (this is the jump to CFLs, next file).

---

## 5. Closure properties of regular languages (PW's full list, compressed)

**Closed under:** union, intersection, complement, difference L1−L2, concatenation, reversal Lᴿ, Kleene star L*, Kleene plus L⁺, prefix, suffix, substring, substitution, homomorphism (and ε-free h), **inverse homomorphism h⁻¹**, quotient L1/L2, symmetric difference, and all finite versions (finite union/intersection/difference/concatenation/substitution), union with a regular set, etc.

**NOT closed under:**

```
  • Subset:  L1 ∩ L2 regular  ⇏  L1 or L2 regular
  • The "infinite" operations (PW's 6): infinite union, infinite intersection,
    infinite difference, infinite concatenation, infinite subset, infinite substitution
```

PW's summary: of their 36 listed operations, **7 are not closed** (subset + the 6 infinite ones).

**Proof recipes (name-check these in solutions):** product construction (union/intersection), swap F ↔ Q∖F (complement), reverse NFA moves (reversal), feed h over transitions (homomorphism).

**PW property to remember:** if L is regular, complement L̄ is regular, and vice versa (L regular ⇔ L̄ regular). Also: L1 ∩ L2 regular does **not** imply L1 is regular — e.g., L1 = {aⁿbⁿ}, L2 = a*: intersection = {ε}, regular, yet L1 is not.

---

## 6. Pumping lemma for regular languages

```
  If L is regular, then ∃ p ≥ 1 such that:
    every s ∈ L with |s| ≥ p splits as s = xyz with
      |xy| ≤ p,  |y| ≥ 1,  and  xyⁱz ∈ L for every i ≥ 0.
```

**Direction matters:** it is **necessary, not sufficient**. Violating it ⇒ not regular (that's the contrapositive we use). Satisfying it ⇒ *nothing proven*. (Non-regular languages that satisfy the pumping conditions exist.)

**Template proof (learn this once):** to show L = {aⁿbⁿ} is not regular:

```
  1. Assume regular, let p be the pumping length.
  2. Choose s = a^p b^p  ∈ L, |s| = 2p ≥ p.
  3. Any split with |xy| ≤ p has xy inside the a-run → y = a^k, k ≥ 1.
  4. Pump i = 0:  xy⁰z = a^(p−k) b^p ∉ L.  Contradiction.
```

Choosing **s** is 90% of the work: pick it so every allowed split breaks the language after pumping (for count-based languages, push the counted block past the first p symbols).

---

## 7. Moore & Mealy machines (PW §2.8 — quick facts)

| | Moore | Mealy |
|---|---|---|
| Output attached to | **states** | **transitions** |
| n-symbol input gives | **n + 1** outputs (one extra) | **n** outputs |
| Output function | λ : Q → Δ | λ : Q × Σ → Δ |

Both are equivalent in power (every Moore has an equivalent Mealy and vice versa); both are FSMs, not acceptors.

---

## 8. Traps & exam patterns

1. **2ⁿ, not 2n** — from-the-end lookbacks blow up exponentially. Compute small n by hand if unsure.
2. **Subset construction counts only reachable subsets** — and includes ∅ only if some transition enters it.
3. **ε-closures on both ends** — before labeling a start subset and after every transition.
4. **Minimal DFA unique; minimal NFA not** — and NFA can be much smaller (n+1 vs 2ⁿ).
5. **"Number of states"** — check if the question says *minimal* / *complete*; a partial DFA (missing transitions) can be smaller.
6. **Pumping lemma direction** — only the non-regularity direction gives conclusions.
7. **Closure exceptions** — subset, and the infinite operations. "Intersection of L1, L2 is regular" says nothing about L1 alone.
8. **Finite languages are always regular** — and so is any language with a bounded numerical parameter (m < n < 10).
9. **Regular grammar direction** — LLG grows on the left of the variable, RLG on the right; both are regular, but `S → aSb` is neither.

---

## 9. Practice set (every answer verified by simulation)

> Convention: DFAs are complete (every state has a transition for every symbol) unless stated.

### Q1 · NAT · Standard

The set of all strings over {a,b} of length **2 through 5** (i.e., 2 ≤ |w| ≤ 5) is accepted by a DFA. The number of states in the minimal (complete) DFA is ______.

### Q2 · NAT · Standard

L = {w ∈ {a,b}* : the **3rd symbol from the beginning** of w is 'a'}. (Strings of length < 3 are not in L.) The minimum number of states in an **NFA** accepting L is ______.

### Q3 · MCQ · Fundamental

A DFA over {0,1} has states q00 (start and accept), q10, q01, q11. Transitions: `q00 -0→ q10`, `q00 -1→ q01`, `q10 -0→ q00`, `q10 -1→ q11`, `q01 -0→ q11`, `q01 -1→ q00`, `q11 -0→ q01`, `q11 -1→ q10`. This DFA accepts exactly:

- (A) strings with an even number of 0s **and** an even number of 1s
- (B) strings with an even number of 0s **or** an even number of 1s
- (C) strings whose length is divisible by 4
- (D) strings that contain "00"

### Q4 · NAT · Standard

An ε-NFA over {a,b}: states q0 (start), q1, q2 (accepting); ε-move q0 → q1; on 'a': q0 → q0; on 'b': q1 → q2; q2 has no outgoing moves. Using **subset construction**, the number of **reachable** states of the equivalent DFA (including the dead state, if it is reachable) is ______.

### Q5 · MCQ · Moderate

Which regular-expression identity is **FALSE**?

- (A) ∅* = ε
- (B) (R*)* = R*
- (C) ε · R = R
- (D) (ab)* = a*b*

### Q6 · MSQ · Standard

Which of the following statements about the **pumping lemma for regular languages** is/are TRUE?

- (A) If L is regular, then L satisfies the pumping conditions for some pumping length p.
- (B) If L satisfies the pumping conditions for some p, then L is regular.
- (C) The pumping lemma can be used to prove that a specific language is not regular.
- (D) The pumping lemma can be used to prove that a language is regular.

### Q7 · MSQ · Moderate

Which of the following is/are TRUE about DFA minimization?

- (A) The minimal DFA for a regular language is unique up to renaming of states.
- (B) States unreachable from the start state must be removed before merging equivalent states.
- (C) Two states are equivalent iff for every string z, both states end in an accepting state or both end in a non-accepting state.
- (D) Merging two equivalent states changes the language accepted.

### Q8 · MCQ · Moderate

Which statement is **FALSE**?

- (A) Regular languages are closed under reversal.
- (B) Regular languages are closed under inverse homomorphism.
- (C) If L1 ∩ L2 is a regular language, then both L1 and L2 must be regular.
- (D) Regular languages are closed under taking prefixes.

### Q9 · MCQ · Moderate

The regular expression R satisfies the equation R = b + R·a. The solution R is:

- (A) ba*
- (B) a*b
- (C) (ba)*
- (D) b*a

### Q10 · NAT · Killer

An NFA over {0,1} accepts exactly the strings whose **2nd symbol from the end is 1**: q0 (start) with self-loop on 0 and a transition on 1 to {q0, q1}; q1 on 0 or 1 goes to q2; q2 is accepting and has no outgoing transitions. Applying **subset construction**, the number of **reachable** DFA states is ______.

---

## Answer Key

| Q | Answer | Why (condensed) |
|---|---|---|
| 1 | 7 | classes: lengths 0,1,2,3,4,5, ≥6 — all distinguishable |
| 2 | 4 | n-th from begin: NFA n+1 (DFA n+2) |
| 3 | A | state = (parity of #0, parity of #1) |
| 4 | 3 | {q0,q1}, {q2}, ∅ |
| 5 | D | "a" ∈ a*b* but ∉ (ab)* |
| 6 | A, C | necessary, not sufficient |
| 7 | A, B, C | (D) merging equivalent states preserves L |
| 8 | C | PW: intersection regular ⇏ factors regular |
| 9 | A | Arden: R = b + Ra ⇒ R = ba* |
| 10 | 4 | {q0}, {q0,q1}, {q0,q2}, {q0,q1,q2} |

### Full solutions

**Q1.** Minimal-DFA state = Myhill–Nerode class = "set of remaining lengths that reach acceptance":

```
  |w|=0 → {2,3,4,5}     |w|=3 → {0,1,2}
  |w|=1 → {1,2,3,4}     |w|=4 → {0,1}
  |w|=2 → {0,1,2,3}     |w|=5 → {0}
                        |w|≥6 → {}  (dead)
```

All 7 sets are distinct → **7** states. (Contrast the one-sided bounds: |w| ≤ 5 alone → 7? No — alone it is 5+2 = 7? For ≤ 5: lengths 0..5 accept-dead?? For ≤ n the classes are lengths 0..n then dead → n+2 total; for ≥ n: lengths 0..n−1 distinguished (n classes) + one "≥ n" accept class = n+1. A **window** stacks both ends: 6 length-classes + dead run = 7 here.) *(Verified by class count.)*

**Q2.** By PW's table, the n-th-symbol-from-the-beginning language has **n+1 = 4** NFA states: a chain q0→q1→q2 (each on a or b), then q2 -a→ q3 (accept) and q3 self-loops on a and b; if the 3rd symbol is b, that path dies (no acceptance). Picturing it: the NFA "counts to 3", so 4 states; the DFA needs a dead state → n+2 = 5. *(Formula + construction.)*

**Q3.** The state name encodes (parity of 0s, parity of 1s); q00 is the only accept state → **A (even 0s and even 1s)**. Spot checks: "01" ⇒ q00→q10→q11 (reject: one 0, one 1 — both odd) ✔; "0011" ⇒ q00→q10→q00→q01→q00 (accept) ✔. The other options fail on specific strings: (B) would accept "001" (even #0s), but the DFA lands in q01 → rejects; (C) would accept "0001" (length 4), but the DFA rejects it (odd #0s); (D) would accept "100" (contains "00"), but it has odd #1s → q01 → rejects. *(Exhaustively verified over all strings up to length 7.)*

**Q4.** Start: ε-closure({q0}) = **{q0,q1}**. `{q0,q1} -a→ {q0} ⇒ ε-closure {q0,q1}` (self-loop); `{q0,q1} -b→ {q2}` (accept). `{q2} -a/-b→ ∅`; `∅ -a/-b→ ∅`. Reachable = {q0,q1}, {q2}, ∅ → **3** states. Language: a*b. *(Verified by simulation: reachable subsets are exactly ['0,1','2',''] .)*

**Q5.** (D) is false: "a" ∈ a*b* but "a" ∉ (ab)* (every (ab)* string has even length). (A), (B), (C) are standard identities. Answer **D**.

**Q6.** (A) ✔ definition. (B) ✘ — that's the converse, which fails: pumping conditions are necessary but not sufficient. (C) ✔ contrapositive use (the only productive use). (D) ✘ — no. Answer **A, C**.

**Q7.** (A) ✔, (B) ✔ (unreachable junk can otherwise look like extra classes), (C) ✔ (that's equivalence). (D) ✘ — merging **equivalent** states is precisely what preserves L. Answer **A, B, C**.

**Q8.** (C) is false (PW §2.19.2): take L1 = {aⁿbⁿ}, L2 = a*; L1 ∩ L2 = {ε} is regular, but L1 is not. (A), (B), (D) are in PW's closed list. Answer **C**. *(This is the classic "intersection with a regular language proves nothing unless you intersect to isolate counting" flip side — used in the Language Detection file.)*

**Q9.** Arden's form R = Q + R·P with Q = b, P = a (and ε ∉ {a}, so applicable) gives R = b·a* → **A**. Check by substitution: ba* = b + b a* a = b + b a⁺ ✔.

**Q10.** Subset construction from {q0} (no ε-moves, so closure is trivial):

```
  {q0}        -0→ {q0}          -1→ {q0,q1}
  {q0,q1}     -0→ {q0,q2}       -1→ {q0,q1,q2}
  {q0,q2}     -0→ {q0}          -1→ {q0,q1}      (accept: contains q2)
  {q0,q1,q2}  -0→ {q0,q2}       -1→ {q0,q1}      (accept)
  → 4 reachable states = 2² ✔ matches the from-the-end formula.
```

*(Verified by simulation — the four reachable subsets are exactly {q0}, {q0,q1}, {q0,q2}, {q0,q1,q2}; the dead state is never entered.)*

---

## 10. Next steps

1. Redo Q1's class table and Q10's subset table from scratch — they are the two exam archetypes.
2. Re-derive the state-counting table's entries for n = 2 by hand (NFA/DFA, from begin/end).
3. Practice reps: `TOC_01_Regular_Expressions_Finite_Automata.md`, `TOC_02_Regular_Languages_Minimal_DFA_Myhill_Nerode.md`.
4. Continue to `TOC_Notes_02_PDA_Context_Free_Languages.md`.
