# Compiler Design → Local Optimization & Data Flow Analysis

**GATE 2027 syllabus:** Compiler Design — local optimisation, data flow analyses: constant propagation, liveness analysis, common subexpression elimination.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** A variable is **live** at a point if its current value may be used along some path before being overwritten; liveness is a **backward** data-flow analysis: IN[B] = use[B] ∪ (OUT[B] − def[B]), OUT[B] = ∪ IN[S] over successors S. **Constant propagation** is a **forward** data-flow analysis using the lattice {UNDEF, constants, NAC (not-a-constant)}, where UNDEF meets anything to give that thing, two different known constants meet to NAC, and NAC meets anything to give NAC. **Common subexpression elimination (CSE)** reuses a previously computed value instead of recomputing an identical expression, when no operand has been redefined in between.

---

## Questions

### Q1 · MCQ · Fundamental
Liveness analysis is an example of a

- (A) forward data-flow analysis
- (B) backward data-flow analysis
- (C) bidirectional data-flow analysis
- (D) analysis that doesn't use a control-flow graph at all

---

### Q2 · NAT · Fundamental
A basic block computes `t1 = a + b`, `t2 = c + d`, `t3 = a + b`, `t4 = t1 * t3`, `t5 = c + d`, `t6 = t4 + t5`, with none of a, b, c, d redefined within the block. Using common subexpression elimination, the number of redundant computations (instructions that can be replaced by reusing an earlier result) is ______.

---

### Q3 · MSQ · Standard
Which of the following statements about liveness analysis is/are TRUE?

- (A) A variable x is in use[B] if block B reads x before (re)defining it within B.
- (B) A variable x is in def[B] if block B assigns to x (possibly after first using its incoming value, which would also put x in use[B]).
- (C) The IN and OUT sets are computed using a backward pass over the control-flow graph, since liveness information flows from a block's successors back to it.
- (D) Liveness information is used in register allocation, to decide when a register holding a variable's value can be safely reused for another variable.

---

### Q4 · NAT · Standard
A control-flow graph has blocks B1 → B2 → B3, where B3 branches to B4 and B5, and both B4 and B5 flow into B6. Block definitions and uses are: B1 def={a,b}, use={}; B2 def={c,d}, use={a,b}; B3 def={}, use={c}; B4 def={e}, use={d}; B5 def={e}, use={d}; B6 def={}, use={e}. Using backward liveness analysis, the variable **d** is live on exit (OUT set) of which blocks? Count the number of blocks (out of B1–B6) for which d ∈ OUT.

---

### Q5 · MCQ · Standard
In the constant-propagation lattice, when two different control-flow paths reach the same program point with a variable x holding the constant value 5 along one path and the constant value 20 along the other, the meet of these two values for x is

- (A) 5
- (B) 20
- (C) UNDEF
- (D) NAC (not-a-constant)

---

### Q6 · NAT · Moderate
A CFG has: B1 (x=5; y=10), which flows to B2 (uses x,y, doesn't redefine them), which branches to B3a (x=20) or B3b (no change to x), both of which flow into B4. Using forward constant propagation (meet-over-all-paths), at the entry to B4, the variable **y** has a known constant value. That value is ______.

---

### Q7 · MSQ · Moderate
Which of the following statements about constant propagation is/are TRUE?

- (A) If a variable is assigned the same constant value along every path reaching a program point, constant propagation determines it holds that constant there.
- (B) If a variable is assigned different constant values along different paths reaching a program point, constant propagation conservatively marks it NAC at that point, even though at runtime only one path is actually taken for any given execution.
- (C) Constant propagation enables further optimizations, like constant folding (evaluating a computation entirely at compile time when all its operands are known constants) and dead-code elimination (e.g., removing a now-unreachable branch of a constant condition).
- (D) Constant propagation is a forward data-flow analysis, since a variable's constant-ness at a point depends on what happened along paths leading into that point.

---

### Q8 · NAT · Moderate
Using the liveness analysis of Q4, the variable **c** is live on exit (OUT set) of exactly how many of the 6 blocks (B1–B6)?

---

### Q9 · MSQ · Hard
Which of the following statements about local (within a single basic block) optimization is/are TRUE?

- (A) Common subexpression elimination within a basic block can be implemented efficiently using "value numbering," assigning the same value number to any two computations with provably identical operands and operator.
- (B) Dead-code elimination removes computations whose results are never subsequently used (not live), which can be identified using the liveness information computed for the block.
- (C) Algebraic simplifications, like replacing `x * 1` with `x` or `x + 0` with `x`, are a form of local optimization that doesn't require data-flow analysis across blocks.
- (D) Local optimizations (within one basic block) can never expose further opportunities for other local optimizations; a single pass always suffices.

---

### Q10 · NAT · Killer
Using the liveness analysis of Q4/Q8, let L = (number of blocks, out of 6, for which d ∈ OUT) and M = (number of blocks, out of 6, for which c ∈ OUT). Using the CSE result of Q2 (2 redundant computations eliminated, reducing 6 instructions to 4 "real" computations plus 2 copy instructions), let N = (number of real computation instructions remaining after CSE, i.e., instructions of the form `tX = A op B`, not simple copies). The value of L + M + N is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | B | Liveness flows backward from use points to definitions |
| 2 | 2 | `t3 = a+b` duplicates `t1`, and `t5 = c+d` duplicates `t2` (verified by value-numbering simulation) |
| 3 | A, B, C, D | All four are standard, correct facts about liveness analysis |
| 4 | 2 | Verified fixed point: OUT(B1)={a,b}, OUT(B2)={c,d}, OUT(B3)={d}, OUT(B4)=OUT(B5)={e}, OUT(B6)={}; d ∈ OUT only for B2 and B3 |
| 5 | D | Two distinct constants meet to NAC |
| 6 | 10 | y is never reassigned anywhere in the CFG, so it remains the constant 10 everywhere (verified by the data-flow fixed point) |
| 7 | A, B, C, D | All four are standard, correct facts about constant propagation |
| 8 | 1 | From the same verified fixed point, c ∈ OUT only for B2 |
| 9 | A, B, C | (D) is false — one local optimization (e.g. CSE) can create new opportunities for another (e.g. dead-code elimination on now-unused originals), so multiple passes can help |
| 10 | 7 | L = 2 (from Q4), M = 1 (from Q8), N = 4 (6 instructions − 2 copies, from Q2); L + M + N = 2 + 1 + 4 |
