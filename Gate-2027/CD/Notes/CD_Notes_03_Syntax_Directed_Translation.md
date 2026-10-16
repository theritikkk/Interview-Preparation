# CD Notes 03 — Syntax-Directed Translation (High-Yield Deep Dive)

> **Why this file exists:** "Syntax-Directed Translation" is the second video topic for Compiler Design. Most questions are **electric**: classify the attribute (inherited vs synthesized), walk an annotated parse tree, and compute a value (expression, count, array width, type). The taxonomy (SDD, SDT, S-attributed, L-attributed, dependency graphs) is the theory layer. Every numeric answer below was **verified by running the SDD evaluator** before writing.
>
> **Companion question bank:** `../CD_04_Syntax_Directed_Translation.md`. **PW source:** `../Reference/PW_Subject_Handbooks/PW_CompilerDesign.txt` (§4.1, pages 8.14–8.16: applications, attributes, identifying attribute type, SDDs, identifying SDDs).

---

## 0. The mental model: a parse tree with values flowing

Parsing answers "is the structure legal?". **Syntax-directed translation** answers "what does it *mean*?" — by attaching **attributes** to grammar symbols and **semantic rules** to productions, then computing the attribute values on the parse tree:

```
Parse tree  +  attributes  =  annotated parse tree
              ↑ values flow UP (synthesized) or DOWN/rightward (inherited)
```

The two notations:
- **SDD (syntax-directed definition)** = CFG + semantic rules attached to each production — the *design*.
- **SDT (syntax-directed translation scheme)** = CFG + **semantic actions written inside the right-hand sides** — the *implementation hint* (order of evaluation is interleaved with parsing).

**Applications (PW §4.1.1):** semantic analysis · building parse trees · producing intermediate representations · evaluating expressions · converting infix → prefix/postfix.

---

## 1. Attributes (PW §4.1.2)

| | **Synthesized** | **Inherited** |
|---|---|---|
| where it is assigned | attribute of the **LHS** symbol | attribute of a symbol **on the RHS** |
| value computed from | **children** (and own inherited values) | **parent and/or left siblings** (context) |
| flows | bottom-up in the tree | down / left-to-right |

- Quick classifier (PW §4.1.3: "check every translation"): a rule `X.a = f(...)` where X is the production's LHS ⇒ **a is synthesized**; where X is a symbol on the RHS ⇒ **a is inherited**.
- **Terminals:** customarily restricted to **synthesized** attributes — the lexeme value handed over by the lexer (e.g. `id.lex`).
- **Annotated parse tree:** each node shows all its attribute instances and values. This is the object you draw in exam answers.

---

## 2. S-attributed vs L-attributed (PW §4.1.4–4.1.5)

| Disciplines | **S-attributed** | **L-attributed** |
|---|---|---|
| attributes allowed | synthesized **only** | synthesized + inherited with a **restriction** |
| the restriction | — | an inherited attribute may depend only on **(1) the parent** and **(2) LEFT siblings** — never right siblings or own descendants |
| where rules/actions sit | at the **end** of the production | anywhere in the RHS |
| natural evaluation | **bottom-up postorder** = reverse of RMD (LR-friendly) | **one left-to-right depth-first pass** (in-order of the annotated tree; LL-friendly) |

- **Every S-attributed SDD is also L-attributed** (with no inherited attributes used). Converse is false.
- **Dependency graph:** nodes = attribute instances of one parse tree; edges = "depends on". It **must be acyclic** for the attributes to be evaluable; any **topological order** works and all valid orders give the **same final values** (dependencies determine values, not the order).
- **Circular SDDs** exist and are undesirable; a cycle in the dependency graph ⇒ no evaluation order.

---

## 3. Worked examples (all verified)

**(a) Declaration typing — inherited `.in`, synthesized `.type`** (the classic from PW §4.1.4):

```
D → T L             { L.in = T.type }                 (T.type flows INTO L → inherited)
T → int             { T.type = int }                  (synthesized)
T → float           { T.type = float }
L → L₁ , id         { L₁.in = L.in ; addtype(id.entry, L.in) }   (L.in inherited)
L → id              { addtype(id.entry, L.in) }
```

For `int a, b`: T computes `int` **up**; D pushes `int` **down** as `L₁.in`; each `id` receives the type through the chain. One right-to-… well, purely left-to-right pass suffices ⇒ this SDD is **L-attributed** (has inherited attributes, so **not** S-attributed).

**(b) Array width — width computed from the inside out:**

```
T → B C             { T.w = B.w × C.w }
B → int             { B.w = 4 }
C → [ num ] C₁      { C.w = num.val × C₁.w }
C → ε               { C.w = 1 }
```

For `float [2][5]` : C₂(ε) = 1 → C₁ = 5×1 = 5 → C = 2×5 = 10 → T.w = **8×10 = 80**. Note the innermost dimension multiplies first.

**(c) Infix → postfix SDT (action form):**

```
E → E₁ + T   { print('+') }      E → T
T → T₁ * F   { print('*') }      T → F
F → ( E )                        F → id
```

The prints fire as productions are completed; for `a + b` the output is `a b +`; for `a + b * c` it is `a b c * +` (the `*` fires before the `+`). This SDT needs **no attributes at all** — pure actions.

**(d) Classify-at-a-glance gallery (PW §4.1.4):** `E → E₁ + T {E.val = E₁.val + T.val}` — `val` synthesized ⇒ S-attributed. `S → A B {A.x = B.x; S.y = A.x}` — `x` inherited-style flow, `y` synthesized. `D → T L {L.in = T.type}` — `in` inherited, `type` synthesized. When you see any inherited attribute, the SDD is L-attributed (and to be L-attributed, no rule may read a **right** sibling).

---

## 4. GATE facts & traps

- **SDD vs SDT:** SDD = rules (declarative); SDT = actions embedded in RHS (operational). An SDD does **not** prescribe a unique evaluation order — dependencies restrict it; multiple topological orders usually exist.
- **S-attributed** ⇒ evaluable during LR parsing by keeping attribute values on the parser stack (children reduce first). **L-attributed** ⇒ evaluable during LL parsing by computing inherited attributes before recursing into a child.
- Mid-production actions in bottom-up parsing may require introducing **marker non-terminals** (that derive ε) so that the action has a reduction point.
- **Inherited attributes cannot always be eliminated** by rewriting the grammar into synthesized-only form — some translations genuinely need context passed downward.
- Dependency graph facts: nodes are **attribute instances of one specific parse tree**; acyclic required; cycles ⇒ not evaluable; all valid orders give identical results.
- Attribute values may be any data: numbers, strings, types, pointers to symbol-table entries, code fragments.

---

## 5. Practice — 10 verified questions

### Q1 · MCQ · Fundamental

In an **annotated parse tree**, the value(s) attached to each node are:

- (A) the names of the grammar symbols on the derivation
- (B) the values of the attributes computed for that node
- (C) the token codes produced by the lexical analyzer
- (D) the production numbers used at that node during parsing

### Q2 · NAT · Fundamental

A desk calculator SDD uses the synthesized attribute `val` with the usual rules (E → E + T, E → T, T → T * F, T → F, F → ( E ), F → num). The value of the expression `4 + 2 * (3 + 1)` computed by this SDD is ______.

### Q3 · MSQ · Standard

Which of the following statements is/are TRUE?

- (A) An SDD (attribute grammar) is a context-free grammar together with semantic rules attached to its productions.
- (B) Attribute values may be numbers, strings, types, symbol-table entries, or code fragments.
- (C) An SDD must explicitly prescribe the order in which its semantic rules are evaluated.
- (D) An SDT augments a grammar with semantic actions embedded within the right-hand sides of productions, indicating the intended interleaving with parsing.

### Q4 · NAT · Standard

Consider the **count** SDD (PW §4.1.1):

```
S → S₁ S₂     { S.count = S₁.count + S₂.count }
S → ( S₁ )    { S.count = S₁.count + 1 }
S → ε         { S.count = 0 }
```

For the parenthesized string `((()))`, `S.count` at the root is ______.

### Q5 · MCQ · Standard

Consider the action SDT:

```
E → E₁ + T { print('+') } | T
T → T₁ * F { print('*') } | F
F → ( E ) | id
```

with actions executed when their production is completed during the parse. The string printed for the input `a + b * c` is:

- (A) `a b c * +`
- (B) `a b + c *`
- (C) `a b c + *`
- (D) `+ a * b c`

### Q6 · NAT · Moderate

Using the array-width SDD

```
T → B C        { T.w = B.w × C.w }
B → int        { B.w = 4 }
C → [ num ] C₁ { C.w = num.val × C₁.w }
C → ε          { C.w = 1 }
```

the total width of the declaration `int [6][2] x;` (in bytes) is ______.

### Q7 · MSQ · Moderate

Which of the following statements is/are TRUE?

- (A) In the dependency graph of an S-attributed SDD, edges go only from child attribute instances to parent attribute instances (upward).
- (B) The dependency graph is built for one particular parse tree, and its nodes are that tree's attribute instances.
- (C) In an L-attributed SDD, an inherited attribute of a child may depend on attributes of any of its siblings, left or right.
- (D) It is customary to allow terminals only synthesized attributes (their lexeme value comes from the lexer).

### Q8 · NAT · Moderate

With the desk calculator SDD of Q2 (standard precedence and associativity), the value of `6 * (2 + 3) - 4` is ______.

### Q9 · MSQ · Hard

Consider the SDD:

```
S → A B    { A.i = f(0); S.s = g(A.s, B.s) }
A → a      { A.s = h(a.lex, A.i) }
B → b      { B.s = k(b.lex) }
```

Which of the following statements is/are TRUE?

- (A) `A.i` is an inherited attribute.
- (B) `A.s` is a synthesized attribute.
- (C) This SDD is L-attributed.
- (D) This SDD is S-attributed.

### Q10 · NAT · Killer

Using the count SDD of Q4 (`S → S₁S₂` sum; `S → (S₁)` +1; `S → ε` 0), the value of `S.count` at the root for the string `(((())))()` is ______.

---

## 6. Answer key & full solutions

**Answer key:** 1-B · 2-12 · 3-A,B,D · 4-3 · 5-A · 6-48 · 7-A,B,D · 8-26 · 9-A,B,C · 10-5

**Q1 — B.** Annotated tree = parse tree + attribute values at nodes (that's the whole point of annotation).

**Q2 — 12.** `4 + 2*(3+1)` = 4 + 2·4 = **12** (parentheses first, then `*`, then `+` — exactly what the tree shape enforces). Verified by evaluation.

**Q3 — A, B, D.** (C) ✘ an SDD fixes **dependencies**, not an order: any topological order of the (acyclic) dependency graph works and yields identical values.

**Q4 — 3.** The count is "nesting depth + sum of parts": innermost `()` has count 0 + 1 = 1; `(())` = 1 + 1 = 2; `((()))` = 2 + 1 = **3**. (Verified by recursive evaluation.)

**Q5 — A.** Bottom-up completion fires `*` before `+` (T's get completed inside E's right operand first): `a b c * +`. Option (B) `a b + c *` is the output for `(a + b) * c` — the classic trap; (C) corresponds to `a * (b + c)`; (D) is a prefix rendering.

**Q6 — 48.** Innermost: C₂(ε).w = 1 → C₁ = 2 × 1 = 2 → C = 6 × 2 = 12 → T.w = 4 × 12 = **48**. (Element count 6×2 = 12 ints, 4 bytes each.)

**Q7 — A, B, D.** (C) ✘ L-attributed explicitly restricts inherited attributes to the **parent and LEFT siblings only** (PW §4.1.4) — a right-sibling dependency destroys left-to-right evaluability. (A) ✔ synthesized-only ⇒ values flow child → parent. (D) ✔ terminals carry just their lexeme value.

**Q8 — 26.** `6 * (2 + 3) - 4` = 6·5 − 4 = 30 − 4 = **26**. Verified by evaluation.

**Q9 — A, B, C.** Rule span check (PW classifier): (A) ✔ `A.i` is assigned in the S-production to a **RHS** symbol ⇒ inherited. (B) ✔ `A.s` is assigned as the **LHS** attribute of A ⇒ synthesized. (C) ✔ the only inherited attribute (`A.i`) depends on the parent's rule context (a constant here); no rule reads a right sibling or a descendant ⇒ L-attributed. (D) ✘ not S-attributed, since A has an inherited attribute.

**Q10 — 5.** Split at top level: `(((())))` has count 1 + count(`((()))`) = 1 + 3 = 4; `()` has count 1; the S → S₁S₂ rule sums them: 4 + 1 = **5**. (Verified by recursive evaluation of the count SDD.)

---

## 7. Next steps

1. Redraw the `int a, b` annotated tree from §3(a) and label each attribute instance S/I (synthesized/inherited) — that single drill covers most classification questions.
2. Re-derive §3(b)'s width (80) and Q6's (48) from the SDD without peeking; write the innermost-first multiplication order explicitly.
3. **Compiler Design is now complete (Files 01–03).** Next in the video order: **Discrete Mathematics + Engineering Mathematics** (last subjects), then a full revision pass over `00_HIGH_YIELD_NOTES_INDEX.md`.
