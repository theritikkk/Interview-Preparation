# Computer Architecture → Control Unit Design (Hardwired & Microprogrammed)

**GATE 2027 syllabus:** Computer Organization and Architecture — design of control unit: hardwired and microprogrammed.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** In microprogrammed control, each microinstruction (control word) is stored in the control store (control memory), one word per micro-step. A **horizontal** microinstruction has one bit per control signal (signals can be asserted in parallel, but words are wide). A **vertical** microinstruction encodes control signals and requires a decoder (words are narrow, but signals are more restricted per step, and decoding adds delay).

---

## Questions

### Q1 · MCQ · Fundamental
The main advantage of **microprogrammed** control over **hardwired** control is

- (A) it is always faster, since there is no propagation delay through combinational logic
- (B) it is easier to modify or extend the instruction set — just change the microprogram, not the hardware
- (C) it eliminates the need for a control store entirely
- (D) it requires fewer control signals overall

---

### Q2 · NAT · Fundamental
A microprogrammed control unit has a control store holding 512 microinstructions. The minimum number of bits needed for the micro-program counter (µPC) to address every microinstruction is ______.

---

### Q3 · MSQ · Standard
Which of the following statements about hardwired vs. microprogrammed control is/are TRUE?

- (A) Hardwired control is generally implemented using combinational logic (or an FSM) directly wired to produce control signals, without a stored program.
- (B) Microprogrammed control naturally supports a "subroutine" mechanism within the microprogram, since a microinstruction can specify a jump to another micro-address.
- (C) Hardwired control is typically faster than microprogrammed control for a given technology, since it avoids the extra memory access per micro-step.
- (D) Hardwired control is generally easier to modify after the chip has been fabricated than microprogrammed control.

---

### Q4 · NAT · Standard
A control store holds 1024 microinstructions, each 24 bits wide. The total size of the control store, in bits, is ______.

---

### Q5 · MCQ · Standard
A **horizontal** microinstruction format, compared to a **vertical** format for the same set of control signals, typically has

- (A) shorter microinstruction words and faster decoding
- (B) longer microinstruction words but no decoding delay (direct control)
- (C) the same word length in both cases
- (D) no relationship to the number of control signals

---

### Q6 · NAT · Moderate
A microprocessor has 32 distinct control signals. Using a **vertical** (fully encoded) microinstruction format, the minimum number of bits needed to encode which single signal (or combination requiring a decoder) is active is ______  (assume one signal is selected per microinstruction, requiring log₂(32) bits).

---

### Q7 · MSQ · Moderate
Which of the following statements about microprogrammed control is/are TRUE?

- (A) The **next-address field** in a microinstruction can specify the address of the next microinstruction, supporting branching within the microprogram (e.g., based on the opcode being decoded).
- (B) A **nanoprogramming** (two-level microprogramming) scheme can reduce control-store size by sharing common vertical-format "nanoinstructions" across multiple horizontal microinstructions.
- (C) Every microinstruction must take exactly the same number of clock cycles to execute, regardless of format.
- (D) A mapping (or "dispatch") ROM is often used to translate the opcode of a machine instruction into the starting micro-address of its microroutine.

---

### Q8 · NAT · Moderate
A control store holds 2048 microinstructions, each 30 bits wide. The total size of the control store, in **bytes**, is ______.

---

### Q9 · MSQ · Hard
Which of the following statements about control unit design is/are TRUE?

- (A) A hardwired control unit for a CPU with n distinct instruction-cycle states can, in principle, be implemented as a finite-state machine requiring at least ⌈log₂ n⌉ flip-flops for its state register.
- (B) Microprogrammed control was historically popular partly because it let CISC instruction sets implement complex instructions (like string operations) as sequences of simpler internal micro-operations.
- (C) A single control store cannot be shared across multiple identical microinstruction sequences for common sub-tasks (e.g., a memory-read sequence); such sequences must always be duplicated in full for every instruction that needs them.
- (D) Hardwired control units are generally more suitable when the instruction set is fixed, simple and speed is paramount, as in many RISC designs.

---

### Q10 · NAT · Killer
A microprogrammed control unit is redesigned from a horizontal format (48 bits/microinstruction, no decoding delay) to a fully vertical format for the same 40 control signals, adding a decoder with propagation delay 3 ns; the microinstruction fetch time (from control store) is 5 ns in both designs, and a machine instruction requires exactly 12 micro-steps to execute (assume neither design changes the number of micro-steps needed). Using the vertical microinstruction word length of ⌈log₂ 40⌉ bits, the value of (total instruction-execution time under vertical control, in ns) − (total instruction-execution time under horizontal control, in ns) is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | B | Flexibility — modify the microprogram, not the hardware, to change/extend behaviour |
| 2 | 9 | ⌈log₂ 512⌉ |
| 3 | A, B, C | (D) is false — hardwired control (fixed logic) is generally harder to modify post-fabrication than a stored microprogram |
| 4 | 24576 | 1024 × 24 |
| 5 | B | Horizontal: wide words, no decoder needed (direct control) |
| 6 | 5 | ⌈log₂ 32⌉ |
| 7 | A, B, D | (C) is false — different microinstructions (and even the same one across designs) can take different numbers of cycles, e.g. when waiting on memory |
| 8 | 7680 | (2048 × 30) / 8 |
| 9 | A, B, D | (C) is false — sharing common microroutines (with jumps/subroutine-like calls) is exactly one of the classic advantages of microprogramming |
| 10 | 36 | Vertical adds a 3 ns decode delay per micro-step: 12 × 3 = 36 ns more than horizontal (word-length and fetch-time differences don't add extra per-step time beyond the stated decode delay) |
