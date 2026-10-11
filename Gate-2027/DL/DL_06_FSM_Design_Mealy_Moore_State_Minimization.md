# Digital Logic → Sequential Circuits II: FSM Design, Mealy vs. Moore & State Minimization

**GATE 2027 syllabus:** Digital Logic — design of sequential circuits.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** A **Moore machine**'s output depends only on the current state. A **Mealy machine**'s output depends on the current state **and** the current input. Two states are equivalent if, for every input, they produce the same output (Moore: same state-output; Mealy: same transition-output for that input) and transition to equivalent states. State minimization merges all equivalent states via partition refinement (table-filling), analogous to DFA minimization.

---

## Questions

### Q1 · MCQ · Fundamental
In a Mealy machine, the output at any instant depends on

- (A) only the current state
- (B) only the current input
- (C) both the current state and the current input
- (D) the entire input history, read all at once

---

### Q2 · NAT · Fundamental
A Moore machine has 6 states. State minimization (merging equivalent states) finds exactly 2 equivalent pairs (and no larger equivalence classes), with the remaining 2 states each in their own class. The number of states in the minimized machine is ______.

---

### Q3 · MSQ · Standard
Which of the following statements about Mealy vs. Moore machines is/are TRUE?

- (A) For the same input sequence, a Mealy machine's output sequence is typically "one step ahead" of (or differently timed from) an equivalent Moore machine's output sequence, because Moore outputs are associated with states (entered after processing a symbol) while Mealy outputs are associated with transitions.
- (B) Every Mealy machine has an equivalent Moore machine (recognizing the same input/output behavior, up to this timing difference).
- (C) Every Moore machine has an equivalent Mealy machine.
- (D) Converting a Mealy machine to an equivalent Moore machine can require **more** states than the original Mealy machine, if a single Mealy state is reached via transitions with different outputs.

---

### Q4 · NAT · Standard
A 6-state Moore machine over alphabet {0,1} has states A, B, C, D, E, F with transitions: A on 0→B, on 1→C; B on 0→D, on 1→E; C on 0→D, on 1→E; D on 0→F, on 1→F; E on 0→F, on 1→F; F on 0→F, on 1→F. Outputs: A=0, B=0, C=0, D=1, E=1, F=0. Using state minimization (table-filling), the number of states in the minimized machine is ______.

---

### Q5 · MCQ · Standard
Two states p and q of a Moore machine are found to have the same output and, for every input symbol, transition to states that are themselves already known to be equivalent. These two states p and q are

- (A) definitely equivalent and can be merged
- (B) definitely distinct and cannot be merged
- (C) equivalent only if p = q already
- (D) impossible to determine without simulating every possible input string

---

### Q6 · NAT · Moderate
A 5-state Mealy machine has states S0–S4 over alphabet {0,1}, with transitions: S0 on 0→S1, on 1→S2; S1 on 0→S3, on 1→S4; S2 on 0→S3, on 1→S4; S3 on 0→S0, on 1→S0; S4 on 0→S0, on 1→S0. Outputs: S0 on 0→0, on 1→0; S1 on 0→0, on 1→1; S2 on 0→0, on 1→1; S3 on 0→1, on 1→0; S4 on 0→1, on 1→0. Using state minimization (table-filling for Mealy machines), the number of states in the minimized machine is ______.

---

### Q7 · MSQ · Moderate
Which of the following statements about FSM design is/are TRUE?

- (A) The number of flip-flops needed for a state register is at least ⌈log₂(number of states)⌉.
- (B) Unused state codes (when the number of states is not a power of 2) can be treated as "don't cares" during next-state logic minimization.
- (C) A synchronous FSM's next-state and output logic are purely combinational, driven by the current state (and current input, for a Mealy machine) and the state register's flip-flops.
- (D) Every FSM design problem has a unique minimal state assignment (encoding) that minimizes the resulting combinational logic.

---

### Q8 · NAT · Moderate
Using the 5-state Mealy machine of Q6 (which minimizes to 3 states), the minimized machine is converted to an equivalent Moore machine, where state S0's incoming transitions have outputs {0, 1} (two distinct values) requiring it to be split into 2 Moore states, while each of the other two minimized Mealy states needs only 1 Moore state. The total number of states in the resulting Moore machine is ______.

---

### Q9 · MSQ · Hard
Which of the following statements is/are TRUE?

- (A) State minimization for a Moore or Mealy machine is analogous to DFA minimization, using an iterative partition-refinement (table-filling) procedure.
- (B) Two incompletely specified FSMs (with some transitions or outputs left unspecified, i.e., "don't care") can sometimes be merged into fewer states than either fully-specified version alone would allow, since don't-cares provide extra freedom.
- (C) FSM state minimization always reduces the number of states to the information-theoretic minimum needed to distinguish all behaviorally distinct input histories, which is unique up to renaming.
- (D) A Mealy machine can, in general, have fewer states than an equivalent Moore machine for the same input/output behavior.

---

### Q10 · NAT · Killer
A Mealy machine has 5 states, as in Q6, minimizing to 3 states (as verified). Separately, a Moore machine recognizing an **equivalent** behavior is constructed directly from the original 5-state description (before minimization), requiring state-splitting by incoming-transition output exactly as in Q8's logic, applied to all 5 original states individually (not the minimized 3). Given that this direct (non-minimized) Moore construction results in 7 total states, and the minimized-Mealy-then-converted-to-Moore approach (Q8) results in 4 states, the difference (7 − 4) is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | C | Defining property of a Mealy machine |
| 2 | 4 | 2 pairs merge into 2 states, plus the 2 remaining standalone states: 2 + 2 |
| 3 | A, B, C, D | All four are standard, correct facts about Mealy/Moore equivalence and conversion |
| 4 | 4 | Verified by table-filling: {B,C} merge, {D,E} merge, A and F remain separate — 4 classes |
| 5 | A | This is exactly the partition-refinement equivalence criterion |
| 6 | 3 | Verified by table-filling: {S1,S2} merge, {S3,S4} merge, S0 remains separate — 3 classes |
| 7 | A, B, C | (D) is false — multiple minimal-cost state assignments can exist, and finding the true optimal one is a hard combinatorial problem, not unique in general |
| 8 | 4 | S0's class splits into 2 (incoming outputs {0,1}); the other two minimized classes need only 1 Moore state each: 2 + 1 + 1 |
| 9 | A, B, D | (C) is false — minimization gives a unique minimal state **count** up to renaming, but "uniqueness" claims beyond that (like it matching some absolute information-theoretic bound across all encodings) overstate what minimization guarantees |
| 10 | 3 | 7 − 4 |
