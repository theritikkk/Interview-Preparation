# CD Notes 01 — Top-Down Parsing & LL(1) (High-Yield Deep Dive)

> **Why this file exists:** "Parsers" is the first video topic for Compiler Design, and top-down parsing is its foundation: derivations, left recursion, left factoring, FIRST/FOLLOW, and the LL(1) table — the single most asked computation from this subject. Every set, table and count below was **verified by running an LL(1) table builder / parser simulator** before writing.
>
> **Companion question bank:** `../CD_02_TopDown_Parsing_LL1.md`. **PW source:** `../Reference/PW_Subject_Handbooks/PW_CompilerDesign.txt` (§2.1–2.2.10, pages 8.3–8.8: grammar, parse/syntax tree, top-down parser, table-construction algorithm, left factoring, FIRST/FOLLOW, table entries, hierarchy, operator-precedence format, LL(1) check without table).

---

## 0. The mental model: one string, two directions

A parser answers two questions: **is w ∈ L(G)?** and **how was w derived?**

```
Top-down :  S  ⟹ … ⟹ w        (expand nonterminals until the input appears)
            traces a LEFTMOST derivation, uses prediction (FIRST/FOLLOW)

Bottom-up:  w  ⟹ … ⟹ S        (reduce substrings back to S)
            traces a RIGHTMOST derivation IN REVERSE, uses handles (File 02)
```

**LL(1)** = scan **L**eft-to-right, build **L**eftmost derivation, **1** lookahead token.
**LR(1)** = scan **L**eft-to-right, build **R**ightmost derivation in reverse, 1 lookahead.

---

## 1. Grammars, derivations and trees (PW §2.1–2.1.1)

- A grammar is G = (V, T, P, S). A derivation α ⇒ β replaces **one** nonterminal by the RHS of one of its productions. **Leftmost (LMD)** always expands the leftmost nonterminal; **rightmost (RMD)** the rightmost.
- **Parse tree** vs **syntax tree**: a parse tree records every derivation step (single-child chains included); a syntax tree collapses chains and keeps only operators/operands.
- **Ambiguity** = some string has ≥ 2 different parse trees (equivalently, two different LMDs). Classic ambiguous shapes: `E → E + E | E * E | id`, `S → S a S | a`, the dangling-else grammar.
- **Reading priority from a tree (PW §2.1.1):** derive `E → E + T`, `E → 2 * 3 + 4`-style until operators appear, then check which operator was applied **first** — the deepest operator binds tightest. For `2 * 3 + 4`: `E ⇒ E+T ⇒ T+T ⇒ T*F+T ⇒ F*F+T ⇒ 2*F+T ⇒ 2*3+T ⇒ 2*3+F ⇒ 2*3+4` — `*` sits below `+`, so `*` has higher priority.
- Associativity: a same-operator chain that leans left (`A → A + a`) is left-associative; if a grammar can make both leanings, it is ambiguous for that reason.
- Compilers are built on CFGs (Type-2) for syntax and regular grammars (Type-3) for tokens.

---

## 2. The parser landscape (PW §2.1.2, §2.2.2, §2.2.11)

```
                 ┌─ recursive descent (may backtrack)
   Top-down  ────┤
                 └─ LL(1) predictive: table + explicit stack, NO backtracking

                 ┌─ operator precedence (also parses some AMBIGUOUS grammars)
   Bottom-up ────┤
   (shift-reduce)└─ LR family: LR(0) → SLR(1) → LALR(1) → CLR(1)
```

- **LL(1) requirements:** no left recursion, no common prefixes (transform first, §3), and FIRST/FOLLOW disjointness (§6). One stack symbol + one lookahead ⇒ **no backtracking**.
- **Bottom-up advantages (PW §2.2.11):** left recursion and common prefixes are *not* problems — it builds the RMD in reverse. LR(1) = LR(0) + 1 lookahead.

---

## 3. Grammar surgery: left recursion & left factoring (PW §2.2.3–2.2.4)

**Why left recursion kills a top-down parser:** it loops forever — `A ⇒ Aα ⇒ Aαα ⇒ …` — before consuming any input.

**Removal of immediate left recursion:**

```
A → A α | β        ⟹        A  → β A′
   (β not starting with A)   A′ → α A′ | ε

General: A → Aα₁|…|Aαₘ|β₁|…|βₙ  ⟹  A → β₁A′|…|βₙA′ ;  A′ → α₁A′|…|αₘA′|ε
```

Example: `E → E + T | T` becomes `E → T E′`, `E′ → + T E′ | ε`.

**Left factoring (removing common prefixes):**

```
A → αβ₁ | αβ₂      ⟹        A  → α A′
                             A′ → β₁ | β₂
```

**Key facts:** both transformations **preserve L(G)** (language unchanged, derivation/tree shapes change); neither *guarantees* the result is LL(1) — you still run the checks in §6. PW's pipeline (top-down): **(1) remove left recursion → (2) left-factor → (3) compute FIRST/FOLLOW → (4) build the table**.

---

## 4. FIRST and FOLLOW (PW §2.2.5–2.2.6)

**FIRST(X)** = terminals that can begin a string derived from X (plus ε if X ⇒* ε).

```
FIRST(a) = {a}  for terminal a;   FIRST(ε) = {ε}
For A → X₁X₂…Xₖ :  add FIRST(X₁)\{ε};
                   if X₁ is nullable, add FIRST(X₂)\{ε}; … continue down the body;
                   if EVERY Xi is nullable, add ε.
```

**FOLLOW(A)** = terminals that can appear immediately after A in some sentential form (plus `$` at the end of input). **ε is never a FOLLOW element.**

```
1) $ ∈ FOLLOW(S)                    (start symbol)
2) A → αB β :  add FIRST(β)\{ε} to FOLLOW(B)
3) A → αB β :  if β ⇒* ε (β nullable), add FOLLOW(A) to FOLLOW(B)
   (special case: if B is the LAST symbol, β = ε, so FOLLOW(B) ⊇ FOLLOW(A))
```

**Worked example (do this slowly once):**  G: `S → A a`, `A → b | ε`

```
FIRST(A) = {b, ε}
FIRST(S) = FIRST(A)\{ε} ∪ FIRST(a)   (A is nullable → move on to 'a')
         = {b} ∪ {a} = {a, b}        ← note: 'a' enters FIRST(S) although
                                        the body STARTS with A — this catches
                                        people who forget the nullable rule.
FOLLOW(S) = {$}
FOLLOW(A): A → body A a  — B = A, β = a  (NOT nullable)  → add FIRST(a) = {a}
         = {a}
```

---

## 5. Building the LL(1) table (PW §2.2.7)

```
Rows    = nonterminals
Columns = terminals + $
Production A → α :
   • enter it in the columns FIRST(α) \ {ε}
   • if α ⇒* ε (nullable), ALSO enter it under every terminal of FOLLOW(A)
Accept/stack discipline: start with [$, S]; expand the top nonterminal using
the cell (top-nonterminal, lookahead); match terminals against the input.
```

**Table for the §4 grammar** (S → Aa; A → b | ε):

| | a | b | $ |
|---|---|---|---|
| **S** | S → Aa | S → Aa | — |
| **A** | A → ε (via FOLLOW) | A → b | — |

4 non-empty cells, 2 empty, **zero conflicts** ⇒ this grammar is LL(1). A grammar is LL(1) **iff its table has no cell with two productions**.

---

## 6. Checking LL(1) without building the table (PW §2.2.10)

For every nonterminal A with alternatives α₁, …, αₙ:

```
(1) FIRST(αi) ∩ FIRST(αj) = ∅          for all i ≠ j      (no common-prefix clash)
(2) if αi ⇒* ε, then FOLLOW(A) ∩ FIRST(αj) = ∅  for all j ≠ i   (no ε-clash)
```

Both conditions for all nonterminals ⇔ LL(1). This is just the table rule re-read per row — no automaton needed.

---

## 7. Where LL(1) conflicts come from

- **Common prefix, not factored:** `A → aB | aC` — both productions want column `a` (FIRST∩FIRST). Left factoring fixes it.
- **Nullable alternative:** if A is nullable, its ε-production occupies **FOLLOW(A)** columns; if a sibling production's FIRST intrudes on FOLLOW(A), the cell clashes (condition (2) above). The **dangling-else** grammar is the canonical example (ambiguous, not LL(1); its table has exactly one conflicting cell — Q4 asks you to locate it).
- **Left recursion:** never LL(1) as written — it must be eliminated, not "resolved".

---

## 8. GATE facts & traps (PW §2.2.8, §2.4.9, §2.4.15)

- **Operator-precedence grammar format (PW §2.4.9):** (1) no production has two nonterminals side-by-side in its right-hand side; (2) **no ε-productions**.
- **Parser existence:** a parser exists only when the language is a **DCFL** — and even then, *some grammars for a DCFL admit no parser* (PW: "There are some grammars whose language is DCFL but no parser is possible for it").
- **Ambiguity:** no LL or LR parser can be constructed for an ambiguous grammar — **except** operator-precedence parsing, which can handle *some* ambiguous grammars via precedence relations (PW note).
- **Every regular grammar is NOT LL(1)** — it may be ambiguous, recursive, or have common prefixes (PW).
- **Class relations:** every LL(1) grammar is unambiguous and is also LR(1)/CLR(1); LL(1) vs **LALR(1) are incomparable** — for ε-producing grammars an LL(1) grammar may fail to be LALR(1) (PW §2.2.8). Left-recursive ⇒ not LL(1) as written.
- **Error detection:** a conflict-free LL(1)/predictive parser reports a syntax error at the **first offending token** (empty table cell) — no backtracking, no "shift first, discover later".
- **Undecidability:** cannot decide whether an arbitrary CFG is ambiguous.

---

## 9. Practice — 10 verified questions

> Format: `Qn · type · difficulty`. Answer key + full solutions in §10. Every answer below was checked by an LL(1) table builder and a stack simulator.

### Q1 · MCQ · Fundamental

Which one of the following correctly matches a parser with the derivation it traces?

- (A) A top-down parser traces a **leftmost** derivation; a bottom-up (LR) parser traces a **rightmost** derivation in reverse.
- (B) A top-down parser traces a rightmost derivation; a bottom-up (LR) parser traces a leftmost derivation in reverse.
- (C) Both trace leftmost derivations; they differ only in whether the stack is explicit.
- (D) A bottom-up (LR) parser traces leftmost and rightmost derivations simultaneously.

### Q2 · NAT · Fundamental

Consider the grammar

```
S → A B        A → ε | a A        B → b | ε
```

with terminals {a, b}. The number of elements of FIRST(S) — counting ε as an element if present — is ______.

### Q3 · MSQ · Standard

Which of the following statements about FIRST/FOLLOW sets is/are TRUE?

- (A) ε can never be an element of a FOLLOW set.
- (B) If a production has B as its last right-hand-side symbol (A → αB), then FOLLOW(B) ⊇ FOLLOW(A).
- (C) If A → αBβ where β can derive ε, then FOLLOW(B) ⊇ FOLLOW(A).
- (D) If two alternative productions of a nonterminal have overlapping FIRST sets, the grammar must be ambiguous.

### Q4 · MCQ · Standard

Consider the dangling-else grammar (i = `if`, t = `then`, e = `else`, o = any other statement, b = a boolean expression):

```
S  → i E t S S′ | o
S′ → e S | ε
E  → b
```

Building its LL(1) table, the construction hits a conflict in exactly one cell. That cell (row, column) is:

- (A) (S, e)
- (B) (S′, e)
- (C) (S′, $)
- (D) (S′, t)

### Q5 · MCQ · Standard

For the grammar

```
S → A B        A → a | ε        B → b | ε
```

the production **A → ε** must be entered in the LL(1) table under the columns

- (A) {a}
- (B) {b, $}
- (C) {a, b}
- (D) {$}

### Q6 · NAT · Moderate

Consider the balanced-parenthesis grammar G: `S → ( S ) S | ε` with terminals {(, )}. In its LL(1) parsing table (rows = nonterminals, columns = terminals + $), the number of **non-empty entries** is ______.

### Q7 · MSQ · Moderate

Which of the following statements is/are TRUE?

- (A) In an operator-precedence grammar, no production has two nonterminals adjacent in its right-hand side.
- (B) An operator-precedence grammar contains no ε-productions.
- (C) Every LL(1) grammar is also LALR(1).
- (D) It is possible for a particular grammar to generate a DCFL while admitting no parser (LL or LR) at all.

### Q8 · NAT · Moderate

For the grammar `S → A B`, `A → a | ε`, `B → b | ε` (same as Q5), the number of **non-empty entries** in its LL(1) parsing table (rows = {S, A, B}; columns = {a, b, $}) is ______.

### Q9 · MSQ · Hard

Which of the following statements is/are TRUE?

- (A) Although no LL or LR parser can be built for an ambiguous grammar, an operator-precedence parser can sometimes parse an ambiguous grammar by exploiting precedence relations.
- (B) Every regular grammar is LL(1).
- (C) A conflict-free LL(1) parser that reads a token which cannot legally extend the parse reports the syntax error at that token, without backtracking.
- (D) Whether an arbitrary context-free grammar is ambiguous is an undecidable problem.

### Q10 · NAT · Killer

The LL(1) parser for G: `S → ( S ) S | ε` (same grammar as Q6) parses the input `(()())`. It starts in configuration: stack `[$ S]` (top of stack on the right), input `(()())$`.

Convention: an expansion replaces the top `S` by the production's right-hand side pushed so that its **first** symbol ends on top — i.e., for S → ( S ) S the top `S` becomes the stack segment `S ) S (` with `(` on top. Matching a terminal pops it and consumes the token.

The maximum number of symbols on the stack at any point during this parse (count `$` and every symbol entry) is ______.

---

## 10. Answer key & full solutions

**Answer key:** 1-A · 2-3 · 3-A,B,C · 4-B · 5-B · 6-3 · 7-A,B,D · 8-8 · 9-A,C,D · 10-7

**Q1 — A.** Top-down = leftmost derivation; bottom-up (LR) = rightmost derivation in reverse (PW §2.2.11: "uses RMD in reverse").

**Q2 — 3.** FIRST(A) = {a, ε}, FIRST(B) = {b, ε}. Since A is nullable, FIRST(S) = FIRST(A)\{ε} ∪ FIRST(B) = {a} ∪ {b, ε} = {a, b, ε} ⇒ **3 elements**.

**Q3 — A, B, C.** (A) ✔ FOLLOW holds terminals + `$` only. (B) ✔ the special case of rule 3 with β = ε. (C) ✔ rule 3 exactly. (D) ✘ overlapping FIRSTs only mean *not LL(1) as written* — e.g. `S → aB | aC` is unambiguous yet overlaps (left factoring restores LL(1)).

**Q4 — B.** `FOLLOW(S′) = {e, $}` because S′ is followed by the `else` branch possibility (and can end the statement). Meanwhile `FIRST(S′ → e S) = {e}`. When the lookahead is `e`, the cell (S′, e) wants **both** `S′ → e S` and `S′ → ε` ⇒ the unique conflict cell is **(S′, e)** (verified by table construction: only `S2,e` clashes).

**Q5 — B.** An ε-production goes under FOLLOW(A). `FOLLOW(A)`: from `S → A B`, add FIRST(B)\{ε} = {b}; B is nullable so also add FOLLOW(S) = {$}. FOLLOW(A) = **{b, $}**.

**Q6 — 3.** `FOLLOW(S) = {), $}` (S is followed by `)` inside `( S ) S`, or by end-of-input at top level). Entries: **(S, `(`) → S → ( S ) S** via FIRST; **(S, `)`) → S → ε** and **(S, `$`) → S → ε** via FOLLOW. Total 3, no conflicts ⇒ LL(1).

**Q7 — A, B, D.** (A) ✔ and (B) ✔ are exactly the operator-precedence format (PW §2.4.9). (C) ✘ false — the classes are incomparable: with ε-productions an LL(1) grammar may not be LALR(1) (PW §2.2.8). (D) ✔ PW: parsers exist only for DCFL languages, and there are grammars generating a DCFL for which no parser is possible.

**Q8 — 8.** Row S: columns a and b (from FIRST(AB) = {a, b}) **and** `$` (S is nullable since A, B are, so S → AB also lands under FOLLOW(S) = {$}) ⇒ 3 cells. Row A: `a` (A → a) and `b, $` (A → ε via FOLLOW(A) = {b, $}) ⇒ 3. Row B: `b` (B → b) and `$` (B → ε via FOLLOW(B) = {$}) ⇒ 2. Total **3 + 3 + 2 = 8** non-empty (the only empty cell is (B, a)).

**Q9 — A, C, D.** (A) ✔ PW: no parser for ambiguous grammars, *except* operator precedence can handle some ambiguous grammars. (B) ✘ PW: every regular grammar is not LL(1) — it may be ambiguous, recursive or have common prefixes. (C) ✔ predictive parser fails exactly at the offending token (empty cell), no backtracking. (D) ✔ standard undecidability result for CFGs.

**Q10 — 7.** Traced simulation (stack, top on the right):

```
[$ S]                    start, lookahead (
[$ S ) S (]              expand S → ( S ) S           → size 5
[$ S ) S]                match (
[$ S ) S ) S (]          expand inner S              → size 7  ← maximum
[$ S ) S ) S]            match (
[$ S ) S )]              expand S → ε (lookahead ))
[$ S ) S]                match )
[$ S ) S ) S (]          expand S (lookahead ()      → size 7 again
[$ S ) S ) S] → [$ S ) S )] → [$ S ) S] → [$ S )] → [$ S] → [$]
```

Maximum stack size = **7** (achieved twice; input fully consumed, accepted).

---

## 11. Next steps

1. Redo §4's FIRST/FOLLOW example and §5's table from a blank page — they are the template for every LL(1) question.
2. Memorize §8's trap list, then run the `CD_02` bank cold.
3. Continue to **File 02 — Bottom-Up Parsing & the LR Family** (`CD_Notes_02_Parsing_BottomUp_LR.md`, bank `CD_03`), then **File 03 — Syntax-Directed Translation** (bank `CD_04`).
