# Compiler Design → Intermediate Code Generation

**GATE 2027 syllabus:** Compiler Design — intermediate code generation.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** Three-address code (TAC) restricts each instruction to at most one operator on the right-hand side, e.g. `t1 = a + b`. Each binary operation in an expression tree generates exactly one TAC instruction and (if its result is not immediately consumed into another named variable) one new temporary. For control flow, `if cond == 0 goto L` branches on falsity; labels mark jump targets.

---

## Questions

### Q1 · MCQ · Fundamental
Three-address code restricts each instruction to have

- (A) at most one operator and at most three addresses (operands/result) total
- (B) exactly three operands and no operators
- (C) at most three instructions per statement
- (D) no temporaries, only named variables

---

### Q2 · NAT · Fundamental
Generating three-address code for the expression `(a + b) * (c - d) / e` (using the standard one-temporary-per-operator scheme), the total number of TAC instructions generated is ______.

---

### Q3 · MSQ · Standard
Which of the following statements about three-address code is/are TRUE?

- (A) It is a linearized representation of a syntax tree (or DAG), with each node becoming one instruction.
- (B) Common intermediate forms include quadruples (op, arg1, arg2, result), triples (op, arg1, arg2, referenced by instruction number), and indirect triples.
- (C) A DAG (rather than a tree) representation of an expression can let the code generator reuse a temporary for a repeated common subexpression, generating fewer instructions than a tree-based generator would.
- (D) Three-address code generation always produces the fewest possible temporaries for any expression, regardless of tree shape.

---

### Q4 · NAT · Standard
Generating three-address code for `a + b * c - d / e * f` (standard operator precedence: `*` and `/` bind tighter than `+` and `-`; evaluated left to right within the same precedence level), the total number of temporaries created is ______.

---

### Q5 · MCQ · Standard
For the statement `if (a < b) { x = 1; } else { x = 2; }`, standard three-address code generation (with explicit labels for the false-branch and the end) produces how many label definitions?

- (A) 0
- (B) 1
- (C) 2
- (D) 3

---

### Q6 · NAT · Moderate
Generating three-address code for `while (i < n) { s = s + i; i = i + 1; }` (with a label at the loop start, a conditional branch to a label at loop end, the two body statements, and an unconditional jump back to the loop start), the total number of lines of generated code (including label-definition lines and the comparison instruction) is ______.

---

### Q7 · MSQ · Moderate
Which of the following statements about **backpatching** is/are TRUE?

- (A) Backpatching is a technique for generating code for boolean expressions and control statements in a single pass, without needing to know target labels in advance.
- (B) It works by maintaining lists of instructions whose jump targets are not yet known, and filling in (patching) the actual label addresses once they become known.
- (C) It is primarily useful for one-pass compilers, since a two-pass compiler could instead compute all labels in a first pass and simply emit correct jump targets directly in a second pass.
- (D) Backpatching is incompatible with short-circuit evaluation of boolean expressions (e.g., `a && b`).

---

### Q8 · NAT · Moderate
For `(a + b) * (c - d) / e` (same expression as Q2), the number of **temporaries** created equals the number of internal (operator) nodes in its expression tree. This expression tree has ______ internal (operator) nodes.

---

### Q9 · MSQ · Hard
Which of the following statements about intermediate representations is/are TRUE?

- (A) An abstract syntax tree (AST) is generally more compact than a parse tree, omitting nodes for non-terminals that exist only to enforce grammar structure (like single-production chains).
- (B) Three-address code is a linear (list-based) intermediate representation, while an AST or DAG is a tree/graph-based representation; a compiler can translate between these forms.
- (C) Static Single Assignment (SSA) form requires every variable to be assigned a value at most once in the program text, introducing new versioned variable names (and "phi" functions at control-flow merge points) as needed.
- (D) Intermediate code generation is unnecessary for any compiler; source code can always be translated directly to target machine code in a single step without loss of modularity or portability.

---

### Q10 · NAT · Killer
Three-address code is generated for the expression `(a + b) * (c - d) / e` (Q2, 4 instructions) and separately for `a + b * c - d / e * f` (Q4). Let P = (number of TAC instructions for the first expression) and Q = (number of TAC instructions for the second expression). Using the fact that each expression's instruction count equals its temporary count (one instruction per temporary created), the value of P + Q is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | A | Definition of three-address code |
| 2 | 4 | Verified: t1=a+b, t2=c-d, t3=t1*t2, t4=t3/e |
| 3 | A, B, C | (D) is false — a naive tree-based generator can create more temporaries than necessary; DAG-based generation (C) is precisely the counterexample showing fewer is possible |
| 4 | 5 | Verified: t1=b*c, t2=a+t1, t3=d/e, t4=t3*f, t5=t2-t4 |
| 5 | C | Labels for the else-branch and the end-of-if (verified: L1 for else, L2 for end) |
| 6 | 7 | Verified: loop-start label, comparison, conditional branch, 2 body statements, jump back, loop-end label |
| 7 | A, B, C | (D) is false — backpatching is in fact commonly used specifically to implement short-circuit evaluation efficiently |
| 8 | 4 | The expression has 4 binary operators (+, -, *, /), matching the 4 temporaries from Q2 |
| 9 | A, B, C | (D) is false — intermediate code generation is a standard, valuable technique (for portability and reusable optimization), not something "unnecessary" |
| 10 | 9 | P = 4 (from Q2); Q = 5 (from Q4); 4 + 5 |
