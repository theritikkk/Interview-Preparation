# TOC Notes 03 — Language Detection & Language Classification (High-Yield Deep Dive)

> **Why this file exists:** the video's third TOC item — "Language Detection" — is the skill of answering *"what is the smallest class of this language?"* GATE asks it constantly (regular? DCFL? CFL? beyond?), plus the **decision-property table** (membership/emptiness/equivalence per machine type). No new machinery: it's the closure properties from Notes 01–02 plus the heuristics below. Practice answers verified by computation.
>
> **Companion question banks:** `TOC_02_Regular_Languages_Minimal_DFA_Myhill_Nerode.md`, `TOC_04_Context_Free_Languages_Pumping_Lemma_CYK.md` (optional extras: `TOC_05`/`TOC_06` cover TM/undecidability — outside the video's high-yield list). PW source: `../Reference/PW_Subject_Handbooks/PW_TOC.txt` (§1.5, §2.16, §3.9, §4.15).

---

## 0. The ladder (Chomsky hierarchy) — always answer with the *smallest* class

```
  Regular  ⊂  DCFL  ⊂  CFL  ⊂  CSL  ⊂  Recursive  ⊂  REL  ⊂  ALL
   (FA)        (DPDA)   (NPDA)   (LBA)     (HTM)        (TM)
```

| Class | Machine | Grammar | Strictness witness (classic) |
|---|---|---|---|
| Regular | FA (DFA/NFA) | regular (right/left-linear) | {aⁿbⁿ} needs a stack |
| DCFL | DPDA | LR(k)-parseable | {w wᴿ} needs a guess |
| CFL | NPDA | CFG | {aⁿbⁿcⁿ} needs two matches |
| CSL | LBA | non-contracting (CSG) | {w w}, {w # w} need same-order compare |
| Recursive | TM that always halts | — (type 0 generates REL) | "M halts on all inputs"-type problems are RE but not recursive |
| REL | TM | unrestricted (UG) | halting problem: RE but not recursive; its complement is not RE |
| ALL | — | — | counting argument (below) |

**The counting fact (cheap mark):** over a finite alphabet there are countably many strings but **uncountably many languages**, while there are only countably many machines ⇒ **most languages are not even RE**.

---

## 1. Detection heuristics — the cheat sheet

> Quantifier discipline first: **bounded numbers → finite → regular**; **unbounded comparison → climb the ladder**.

| # | Pattern | Smallest class | Why |
|---|---|---|---|
| 1 | {aᵐbⁿ : m < n < 10}, {aᵖ : p prime < 100}, {aᵏbᵐ : k = m ≤ 5} | **Regular** | finite sets are regular |
| 2 | \|w\| = k, starts/ends/contains fixed string, only-last-k lookback | **Regular** | bounded memory |
| 3 | n-th symbol from the **end** is fixed | **Regular** (2ⁿ states) | remember last n−1 symbols |
| 4 | #a ≡ 0 (mod m), #b ≡ 0 (mod n), parity of length | **Regular** (m, m·n states) | finite counters mod m |
| 5 | unary {a^f(n)}: f = linear/multiples | **Regular** | ultimately periodic (a²ⁿ = (aa)*) |
| 6 | unary {a^f(n)}: f = n², 2ⁿ, n!, prime | **Not regular** | pumping fails |
| 7 | {aⁿbⁿ}, {aⁿb²ⁿ}, {a²ⁿbⁿ}, {aᵐbⁿ : m < n}, {m > n}, {m ≠ n}, {m ≥ n} | **DCFL, not regular** | one stacked match (PW's list) |
| 8 | aⁿb*cⁿ, aⁿbⁿc*, aᵐbⁿc^(m+n) | **DCFL** | match with free middle |
| 9 | w wᴿ (even palindrome) | **CFL, not DCFL** | the middle must be guessed |
| 10 | w c wᴿ (fixed center symbol c) | **DCFL** | the marker exposes the middle |
| 11 | {w x w : x ∈ (a+b)⁺}, {w wᴿ x}, {x w wᴿ} | **Regular** | the wildcard middle absorbs everything (choose it freely) |
| 12 | {aⁱbʲcᵏ : i = j **or** j = k} | **CFL, not DCFL** | union of two DCFLs; inherently ambiguous |
| 13 | aⁿbⁿcⁿ (= i = j **and** j = k) | **Not CFL** | two independent matches, one stack |
| 14 | w w, w # w | **Not CFL** | same-order comparison needs a queue |
| 15 | {aᵐbⁿ : gcd(m,n) = 1} vs {… : lcm(m,n) = 1} | gcd → **not regular**; lcm → **Regular** | lcm = 1 forces m = n = 1: finite! (PW) |
| 16 | {w : #a(w) = #b(w)} | **DCFL** | stack holds the running difference |

**Two proof tools built into detection:**

```
  Isolate counting by intersecting with a REGULAR set:
     {#a = #b}   ∩ a*b*     = aⁿbⁿ        (not regular)
     {#a=#b=#c}  ∩ a*b*c*   = aⁿbⁿcⁿ      (not CFL)
  Then finish with the matching pumping lemma.
```

---

## 2. Closure cells worth memorizing (details in Notes 01–02)

| Class | Closed under | NOT closed under | Key mixed cells |
|---|---|---|---|
| Regular | ∪, ∩, ¬, −, ∘, ᴿ, *, ⁺, prefix/suffix/substring, h, h⁻¹, quotient | subset; infinite operations | — |
| CFL | ∪, ∘, ᴿ, *, ⁺, prefix/suffix/substring, h, h⁻¹, substitution | ∩, ¬, −, subset, quotient | **CFL ∩ Regular = CFL**; CFL ∪ R = CFL; CFL − R = CFL |
| DCFL | **¬ (complement — the headline)**, prefix, ∩/∪ with regular | ∪, ∩, ∘, ᴿ, *, ⁺ | DCFL ∪ Finite = DCFL |

Detection uses of these cells:

- L is CFL? Try to build it by closed operations from known CFLs, or intersect with a regular set to isolate a known non-CFL.
- **Complement asymmetry:** DCFL closed under ¬, CFL not (and CFL *is* closed under union, DPDA is not) — a favorite one-liner MCQ.
- Don't overshoot: CFL ∩ CFL may be non-CFL — so an intersection being non-CFL proves nothing about the factors; you must *construct* a witness like {aⁿbⁿc*} ∩ {a*bⁿcⁿ}.

---

## 3. Decision properties (PW §4.15): the 8-property table

```
  D = decidable, UD = undecidable        FA   DPDA   PDA   LBA/HTM   TM
  H  halting                             D     D      D      D         UD
  M  membership                          D     D      D      D         UD
  Em emptiness                           D     D      D      UD        UD
  F  finiteness                          D     D      D      UD        UD
  T  totality (accepts Σ*)               D     D      UD     UD        UD
  Eq equivalence                         D     D      UD     UD        UD
  D  disjointness                        D     UD     UD     UD        UD
  S  set containment                     D     UD     UD     UD        UD
```

**Reading guide (how this table is quizzed):**

- FA row: everything decidable (product constructions; emptiness via reachability).
- DPDA: everything except Disjoint & Containment.
- **PDA (CFLs):** membership decidable (CYK), emptiness/finiteness decidable (grammar algorithms); totality, equivalence, disjointness, containment undecidable.
- **LBA:** membership decidable (exponentially many configurations, each check finite); emptiness/finiteness and the rest undecidable.
- TM: halting & membership undecidable — membership is **semi-decidable** (accepting runs halt; rejecting runs may loop); everything else undecidable.
- Complement facts (PW §4.13): decidable ⇒ complement decidable; RE-but-not-recursive ⇒ complement **not RE**; not-RE ⇒ complement is either not-RE or RE-but-not-recursive.

---

## 4. Traps & exam patterns

1. **Bounded vs unbounded** parameters — "m < n < 10" is finite/regular; "m < n" is not.
2. Answer the **smallest class**, and use the exact option ladder: Regular / DCFL-not-regular / CFL-not-DCFL / not CFL.
3. **OR vs AND** of constraints: i = j OR j = k → CFL; i = j AND j = k → not CFL.
4. **Middle wildcard collapse:** {w x w^R : x ∈ (a+b)⁺} = (a+b)⁺ — regular, not CFL-flavored.
5. **lcm vs gcd** (PW): lcm = 1 is finite; gcd = 1 is the non-regular one.
6. Even palindrome vs marker palindrome (CFL vs DCFL).
7. **w#w is not CFL**, but **w#wᴿ is DCFL** — the # alone doesn't fix same-order comparison; the reversal + marker does.
8. **Class overshoot:** "not regular" ≠ "not CFL"; "not CFL" ≠ "not recursive". aⁿbⁿcⁿ is context-sensitive, not unreachable.
9. Decision table: don't carry FA's "all decidable" to PDAs/LBAs; match the property to the machine's row.
10. Countability: uncountably many languages vs countably many TMs ⇒ non-RE languages exist. (A statement-level fact.)

---

## 5. Practice set (every answer verified by computation)

### Q1 · MSQ · Standard

Which of the following languages is/are **context-free**?

- (A) {w wᴿ : w ∈ {0,1}*}
- (B) {w w : w ∈ {0,1}*}
- (C) {0ⁿ1ⁿ : n ≥ 0}
- (D) {0ⁿ1ⁿ0ⁿ : n ≥ 0}

### Q2 · MCQ · Standard

The smallest class containing L = {w wᴿ : w ∈ {a,b}*} is:

- (A) regular
- (B) DCFL but not regular
- (C) CFL but not DCFL
- (D) not context-free

### Q3 · MCQ · Standard

For a fixed symbol c, let L = {w c wᴿ : w ∈ {a,b}*}. The smallest class containing L is:

- (A) regular
- (B) DCFL but not regular
- (C) CFL but not DCFL
- (D) not context-free

### Q4 · MSQ · Moderate

Which of the following is/are **regular**?

- (A) {aᵐbⁿ : m < n < 10}
- (B) {aᵐbⁿ : m < n}
- (C) {aᵐbⁿ : m ≡ n (mod 2)}
- (D) {aᵐbⁿ : m = 2n}

### Q5 · NAT · Moderate

The number of strings in the language L = {aᵐbⁿ : m < n < 10, m ≥ 0} is ______.

### Q6 · MSQ · Moderate

Using the standard decision-properties table for machine classes, which of the following is/are TRUE?

- (A) Membership of a string in a fixed context-sensitive language is decidable.
- (B) Equivalence of two LBAs is decidable.
- (C) Emptiness of an LBA's language is undecidable.
- (D) Finiteness of a context-free language is decidable.

### Q7 · MCQ · Moderate

The smallest class containing L = {w # w : w ∈ {a,b}*} (the two copies are separated by #) is:

- (A) regular
- (B) DCFL but not regular
- (C) CFL but not DCFL
- (D) not context-free

### Q8 · NAT · Moderate

The number of strings in L = {aᵏbᵐ : k = m} ∩ {aᵏbᵐ : k ≤ 3} is ______.

### Q9 · MSQ · Moderate

Which of the following languages is/are **NOT regular**?

- (A) {aᵐbⁿ : m = 2n}
- (B) {aᵐbⁿ : m · n = 0}
- (C) {aᵐbⁿ : m + n is even}
- (D) {aᵐbⁿ : m = n²}

### Q10 · NAT · Killer

From the standard decision-properties table (properties: Halting, Membership, Emptiness, Finiteness, Totality, Equivalence, Disjointness, Containment), the number of properties that are **decidable for PDAs but undecidable for LBAs** is ______.

---

## Answer Key

| Q | Answer | Why (condensed) |
|---|---|---|
| 1 | A, C | wwᴿ and 0ⁿ1ⁿ; ww and 0ⁿ1ⁿ0ⁿ fail |
| 2 | C | guess the midpoint → nondeterminism essential |
| 3 | B | center marker reveals the turn |
| 4 | A, C | (A) finite; (C) ⇔ even length |
| 5 | 45 | Σ for n = 1..9 of n |
| 6 | A, C, D | (B) LBA equivalence is UD |
| 7 | D | same-order match — beyond CFL |
| 8 | 4 | k = 0,1,2,3 |
| 9 | A, D | both need counting comparisons |
| 10 | 2 | Emptiness and Finiteness |

### Full solutions

**Q1.** (A) ✔ — push w, guess the middle, pop against wᴿ (NPDA). (C) ✔ — the flagship DCFL. (B) ✘ — {w w} requires comparing two copies in the *same* order; a stack reverses order, so pumping fails (z = aᵖbᵖaᵖbᵖ). (D) ✘ — {0ⁿ1ⁿ0ⁿ} needs two matching counts (intersect with 0*1*0* and use the CFL pumping lemma). Answer **A, C**.

**Q2.** For w wᴿ the midpoint is not marked; an acceptor must guess where w ends. A DPDA cannot (no nondeterminism), so it's **CFL but not DCFL (C)** — PW's canonical [16]. (Not (A): unbounded matching. Not (B): (B) would require a DPDA — impossible.)

**Q3.** The literal c marks the end of w. Push w, then after reading c, deterministically pop while reading wᴿ. That's a DPDA → **DCFL but not regular (B)**. Contrast: without c (Q2), the same language family drops out of DCFL — one symbol changes the class.

**Q4.** (A) ✔ — both m, n bounded → finite → regular. (B) ✘ — unbounded comparison m < n is the classic non-regular DCFL. (C) ✔ — m ≡ n (mod 2) ⇔ m + n even ⇔ |w| even: a 2-state parity DFA. (D) ✘ — m = 2n is unbounded counting (pump a^p b^p ∉ L). Answer **A, C**.

**Q5.** Count pairs: for n = 1..9, m ranges over 0..n−1 → n choices each → 1+2+…+9 = **45**. (The language is finite → regular; bounded parameters keep you on the bottom rung.) *(Verified by enumeration.)*

**Q6.** Per the table: (A) ✔ — M for LBA is decidable. (B) ✘ — Eq for LBAs is undecidable. (C) ✔ — Em for LBAs is undecidable. (D) ✔ — F for PDA (CFL) is decidable. Answer **A, C, D**.

**Q7.** w#w demands comparing the two copies **in the same order** — a stack can only reverse. No CFL accepts it (CFL pumping fails; equivalently, intersecting with a#a*b* isolates aⁿ#aⁿ, a counting comparison a stack can't do in both directions). → **D** (it is context-sensitive/decidable, but answering "smallest class" stops at D).

**Q8.** Intersect the equality condition with k ≤ 3: strings are aᵏbᵏ for k = 0,1,2,3 → ε, ab, aabb, aaabbb → **4** strings. (CFL ∩ Regular stays CFL — here it even collapses to finite.) *(Verified.)*

**Q9.** (A) ✘-option ✔ non-regular: m = 2n needs unbounded counting (PL: s = a²ᵖbᵖ). (B) is regular: m·n = 0 ⇔ no a's or no b's → a* ∪ b* (DFA with a couple of states). (C) is regular: parity of total length. (D): m = n² — pumping aᵖ²... pick s = a^{p²}b^{p²}? simplest: s = a^{4p²}? Standard: m = n² forces quadratic growth — not regular. Answer **A, D**. *(Sanity: (B) and (C) accept by finite automata; (A), (D) need counters.)*

**Q10.** Scan the PDA and LBA/HTM columns of the table row by row:

```
  H : D  vs D   → no
  M : D  vs D   → no
  Em: D  vs UD  → YES  (1)
  F : D  vs UD  → YES  (2)
  T : UD vs UD  → no
  Eq: UD vs UD  → no
  D : UD vs UD  → no
  S : UD vs UD  → no
  → 2
```

Answer **2** (Emptiness, Finiteness). *(Verified against PW §4.15's table.)*

---

## 6. Next steps

1. Make a one-page flashcard of §1's heuristics table — it *is* the topic.
2. Re-derive why each witness in §0 sits where it does (aⁿbⁿ stack; aⁿbⁿcⁿ two matches; ww order).
3. Practice reps: `TOC_02_Regular_Languages_Minimal_DFA_Myhill_Nerode.md`, `TOC_04_Context_Free_Languages_Pumping_Lemma_CYK.md`.
4. **TOC notes complete.** Subject order next: **DBMS** — normalization → relational algebra/SQL → indexing/transactions.
