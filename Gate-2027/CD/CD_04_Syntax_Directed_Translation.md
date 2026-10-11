# Compiler Design → Syntax-Directed Translation

**GATE 2027 syllabus:** Compiler Design — syntax-directed translation.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** A **synthesized** attribute at a node is computed from the attributes of its children (and the node's own lexical value, for leaves). An **inherited** attribute at a node is computed from the attributes of its parent and/or siblings. An **S-attributed** grammar uses only synthesized attributes (evaluable bottom-up, e.g. during LR parsing). An **L-attributed** grammar allows inherited attributes too, but only "from the left" (a symbol's inherited attribute may depend only on the parent's inherited attributes and the attributes of symbols to its left in the same production) — L-attributed grammars can be evaluated during a single left-to-right (e.g. top-down, or LR with care) traversal.

---

## Questions

### Q1 · MCQ · Fundamental
A **synthesized** attribute at a parse-tree node is computed using

- (A) only the attributes of that node's parent
- (B) the attributes of that node's children (and its own lexical value, if a leaf)
- (C) the attributes of nodes anywhere in the tree, in any order
- (D) only global variables, never local tree attributes

---

### Q2 · NAT · Fundamental
Using the S-attributed rules E → E₁ + T {E.val = E₁.val + T.val}, E → T {E.val = T.val}, T → T₁ * F {T.val = T₁.val × F.val}, T → F {T.val = F.val}, F → digit {F.val = digit.lexval}, the value computed for the expression `3 + 4 * 2 + 5` is ______.

---

### Q3 · MSQ · Standard
Which of the following statements about S-attributed and L-attributed grammars is/are TRUE?

- (A) Every S-attributed grammar is also L-attributed.
- (B) An S-attributed grammar can be evaluated entirely during a bottom-up (e.g., LR) parse, since all information needed is synthesized from already-reduced children.
- (C) An L-attributed grammar with genuine inherited attributes generally requires top-down parsing information (or careful handling in LR parsing) because an inherited attribute may depend on information from the parent/left siblings not yet fully available bottom-up.
- (D) Every L-attributed grammar is also S-attributed.

---

### Q4 · NAT · Standard
Using the parse tree for `3 + 4 * 2 + 5` (same expression as Q2), the total number of nodes in the parse tree (counting every leaf and internal node, using a grammar where each `+` or `*` operation and each number is one node) is ______.

---

### Q5 · MCQ · Standard
A declaration grammar D → T L, T → int | float, L → L₁, id | id uses an **inherited** attribute L.in (the declared type) to propagate the type from T down to every identifier in L. This inherited attribute is set using the rule

- (A) D → T L {L.in = T.type}
- (B) L → L₁, id {L₁.in = L.in; addtype(id.entry, L.in)}
- (C) both (A) and (B) are needed together, since L's own inherited attribute must first be set by D, then passed down through the recursive L₁
- (D) T → int {T.type = L.in}

---

### Q6 · NAT · Moderate
For the declaration `float x, y, z`, using the inherited-attribute type-propagation scheme of Q5, every identifier receives the same type. If `float` is encoded as type value 2 (an arbitrary encoding) and `int` as type value 1, the sum of the type-codes assigned to the 3 identifiers x, y, z is ______.

---

### Q7 · MSQ · Moderate
Which of the following statements about SDT (syntax-directed translation) implementation is/are TRUE?

- (A) S-attributed definitions can be implemented directly during LR (bottom-up) parsing, attaching synthesized attribute values to grammar-symbol records on the parser's stack.
- (B) L-attributed definitions can be implemented during LL (top-down, predictive) parsing, by evaluating inherited attributes before recursively parsing a nonterminal.
- (C) Embedding semantic actions at arbitrary positions within a production (not just at the end) can require restructuring the grammar (e.g., introducing marker non-terminals that derive ε) for bottom-up parsing.
- (D) A circular attribute dependency (where an attribute's value depends, directly or indirectly, on itself) is always automatically detected and resolved during attribute evaluation, regardless of the specific evaluation order chosen.

---

### Q8 · NAT · Moderate
Using an SDT for array type/width computation: T → B C, B → int {B.type = integer; B.width = 4} | float {B.type = float; B.width = 8}, C → [num] C₁ {C.type = array(num.val, C₁.type); C.width = num.val × C₁.width} | ε {C.type = B.type; C.width = B.width} (C.type and C.width are inherited from B, synthesized back up through the chain of C's). For the declaration `int [3][5]` (array of 3 arrays of 5 integers), the total width (C.width at the outermost C) is ______.

---

### Q9 · MSQ · Hard
Which of the following statements is/are TRUE?

- (A) An attribute grammar's dependency graph (showing which attribute instances depend on which others, for a specific parse tree) must be acyclic for the attributes to be well-defined and evaluable.
- (B) Different valid topological orderings of an attribute dependency graph can, in general, all produce the same final attribute values, since the dependencies (not just an arbitrary order) determine the values.
- (C) A purely S-attributed grammar's dependency graph, for any parse tree, naturally respects a bottom-up evaluation order with no need for topological sort beyond "children before parent."
- (D) Two different grammars that generate the same language must have the same set of possible synthesized/inherited attribute computations, since the language is identical.

---

### Q10 · NAT · Killer
Using the array-width SDT of Q8, the declaration `float [2][4][3]` (array of 2 arrays of 4 arrays of 3 floats) has a total width W1. Separately, `int [3][5]` (from Q8) has total width W2. The value of W1 − W2 is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | B | Definition of a synthesized attribute |
| 2 | 16 | 3 + (4×2) + 5 = 3 + 8 + 5 (verified by evaluating the annotated parse tree) |
| 3 | A, B, C | (D) is false — L-attributed grammars strictly include S-attributed ones as a special case, not the reverse |
| 4 | 7 | Verified: 4 leaves (3,4,2,5) + 3 internal operator nodes (+,*,+) |
| 5 | C | Both the top-level inheritance from D and the recursive propagation through L are needed |
| 6 | 6 | All three identifiers get type 2 (float): 2+2+2 |
| 7 | A, B, C | (D) is false — circular dependencies are a real hazard that must be avoided by careful grammar/attribute design, not something automatically resolved |
| 8 | 60 | Verified by evaluating the SDT: 3 × 5 × 4 (int width 4) |
| 9 | A, B, C | (D) is false — different grammars for the same language can support entirely different sets of attributes/computations; the language alone doesn't determine this |
| 10 | 132 | W1 (float[2][4][3]) = 2×4×3×8 = 192 (verified); W2 = 60 (from Q8); 192 − 60 |
