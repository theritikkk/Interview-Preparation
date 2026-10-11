# Computer Architecture → Instruction Set & Addressing Modes

**GATE 2027 syllabus:** Computer Organization and Architecture — instruction set and addressing modes.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** In direct addressing, the instruction's address field holds the operand's memory address. In indirect addressing, it holds the address of a memory location that itself holds the operand's address (one extra memory reference). In indexed addressing, effective address (EA) = base + index × scale. In PC-relative addressing, EA = PC + offset (using the PC value at the time of the calculation, as stated in each question).

---

## Questions

### Q1 · MCQ · Fundamental
In **immediate** addressing mode, the operand is

- (A) a memory address given directly in the instruction
- (B) contained directly within the instruction itself
- (C) found in a register named by the instruction
- (D) found by adding an offset to the program counter

---

### Q2 · NAT · Fundamental
Memory contents are: address 100 holds the value 200; address 200 holds the value 300. An instruction uses **indirect** addressing with the address field containing 100. The value of the operand fetched is ______.

---

### Q3 · MSQ · Standard
Which of the following statements about addressing modes is/are TRUE?

- (A) Indirect addressing requires one more memory access than direct addressing to fetch the operand.
- (B) Register addressing is generally faster than any memory-based addressing mode, since no memory access is needed to fetch the operand.
- (C) PC-relative addressing is commonly used to implement position-independent code for branch instructions.
- (D) Immediate addressing mode can be used to specify the target address of a branch instruction that must vary at runtime based on data.

---

### Q4 · NAT · Standard
Using indexed addressing mode, EA = base + index × scale. For base = 1000, index = 5 and scale = 4, the effective address is ______.

---

### Q5 · MCQ · Standard
A computer has n-bit addresses and uses a fixed-length instruction format. If k bits are reserved for the opcode, the maximum number of distinct memory addresses directly addressable by a single address field (using the remaining bits for direct addressing) is

- (A) 2ⁿ
- (B) 2ⁿ⁻ᵏ
- (C) 2ᵏ
- (D) n − k

---

### Q6 · NAT · Moderate
The expression X = (A + B) × (C + D) is compiled for a **three-address** machine (each instruction can specify two source operands and one destination, all of which may be memory locations or temporaries). The minimum number of instructions needed is ______.

---

### Q7 · MSQ · Moderate
Which of the following statements about machine instruction formats is/are TRUE?

- (A) A zero-address (stack-based) machine implicitly uses the top of the stack as an operand source and destination.
- (B) A one-address (accumulator-based) machine implicitly uses the accumulator as one operand source and as the destination.
- (C) For a given high-level expression, a zero-address (stack) machine typically requires **more** instructions than a one-address (accumulator) machine to evaluate it.
- (D) Two-address instructions typically overwrite one of the source operands with the result.

---

### Q8 · NAT · Moderate
The expression X = (A + B) × (C + D) is compiled for a **one-address (accumulator-based)** machine, using the standard instruction sequence: LOAD A; ADD B; STORE T1; LOAD C; ADD D; STORE T2; LOAD T1; MUL T2; STORE X. The total number of instructions in this sequence is ______.

---

### Q9 · MSQ · Hard
Which of the following statements about CISC vs. RISC instruction sets is/are TRUE?

- (A) RISC architectures typically favor a large, uniform register set and simple, fixed-length instructions.
- (B) CISC architectures often include complex instructions that may take multiple cycles and directly operate on memory operands.
- (C) RISC's simpler instructions generally make pipelining easier to implement efficiently.
- (D) Every RISC instruction set is, by definition, a strict subset of some corresponding CISC instruction set for the same task.

---

### Q10 · NAT · Killer
The expression X = (A + B) × (C + D) is compiled separately for a three-address machine (Q6), a one-address/accumulator machine (Q8, 9 instructions), and a **zero-address (stack) machine** using the sequence: PUSH A; PUSH B; ADD; PUSH C; PUSH D; ADD; MUL; POP X. Let T = (instructions for stack machine) + (instructions for accumulator machine) + (instructions for three-address machine). The value of T is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | B | Defining property of immediate addressing |
| 2 | 300 | Address field 100 → memory[100] = 200 (the pointer) → memory[200] = 300 (the operand) |
| 3 | A, B, C | (D) is false — immediate mode encodes a fixed, compile-time-known constant, not a runtime-computed address |
| 4 | 1020 | 1000 + 5×4 |
| 5 | B | The address field has n − k bits (whatever bits remain after the opcode) |
| 6 | 3 | ADD T1,A,B / ADD T2,C,D / MUL X,T1,T2 |
| 7 | A, B, C, D | All four are standard, correct facts about instruction-format trade-offs |
| 8 | 9 | Count the listed instructions directly |
| 9 | A, B, C | (D) is false — RISC and CISC instruction sets are designed independently; one is not simply a subset of the other |
| 10 | 20 | Stack machine: 8 instructions; accumulator: 9; three-address: 3; total 8 + 9 + 3 |
