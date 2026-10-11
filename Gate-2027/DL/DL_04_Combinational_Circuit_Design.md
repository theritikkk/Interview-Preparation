# Digital Logic → Combinational Circuit Design (MUX, Decoders, Adders, Comparators)

**GATE 2027 syllabus:** Digital Logic — design of combinational circuits.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** A 2ⁿ:1 multiplexer (MUX) has n select lines and 2ⁿ data inputs; data input at address (binary value of the select lines) is routed to the output. A full adder takes inputs A, B, Cin and produces Sum = A⊕B⊕Cin and Cout = AB + BCin + ACin. An n:2ⁿ decoder activates exactly one of 2ⁿ outputs based on its n inputs.

---

## Questions

### Q1 · MCQ · Fundamental
An 8:1 multiplexer has select lines S2 S1 S0 (S2 is most significant). To implement a 3-variable Boolean function F(A,B,C) directly (one data input per minterm), the natural assignment is

- (A) S2=C, S1=B, S0=A
- (B) S2=A, S1=B, S0=C
- (C) select lines are irrelevant to the assignment
- (D) only 2 of the 3 variables can be connected to select lines

---

### Q2 · NAT · Fundamental
A full adder computes Sum and Cout for inputs A=1, B=1, Cin=1. The value of Sum is ______  (0 or 1).

---

### Q3 · MSQ · Standard
Which of the following statements about multiplexer-based function implementation is/are TRUE?

- (A) An 8:1 MUX can directly implement any 3-variable Boolean function by connecting each data input to 0 or 1 according to the function's truth table.
- (B) A 4:1 MUX can implement any 3-variable Boolean function by using 2 of the 3 variables as select lines and expressing each data input as a function (0, 1, the third variable, or its complement) of the remaining variable.
- (C) An n:1 MUX, using the "variable reduction" technique, can implement any function of up to (log₂n + 1) variables.
- (D) A MUX-based implementation always uses fewer total gates than a direct SOP (sum-of-products) implementation of the same function.

---

### Q4 · NAT · Standard
A 4:1 MUX implements F(A,B,C) = Σm(0,1,3,4,5,7), using A as select line S1 and B as select line S0 (data inputs depend on C). The number of the 4 data inputs that must be wired to the constant 1 is ______.

---

### Q5 · MCQ · Standard
Using a 3-to-8 decoder (active-high outputs, one output per minterm) together with external OR gates, to implement two functions F1 = Σm(1,3,5) and F2 = Σm(2,3,6,7) simultaneously, the two OR gates need, respectively,

- (A) 3 inputs and 4 inputs
- (B) 4 inputs and 3 inputs
- (C) 3 inputs and 3 inputs
- (D) 8 inputs and 8 inputs

---

### Q6 · NAT · Moderate
A 4-bit ripple-carry adder adds 1011₂ (11) and 0110₂ (6), with an initial carry-in of 0. The number of full adders (out of the 4) that produce a carry-out of 1 is ______.

---

### Q7 · MSQ · Moderate
Which of the following statements about 2-bit magnitude comparators is/are TRUE?

- (A) Of the 16 possible input combinations (A, B each 2 bits), exactly 4 produce "A = B."
- (B) Of the 16 possible input combinations, "A > B" and "A < B" each occur for exactly 6 combinations.
- (C) A magnitude comparator's "equal" output can be built from XNOR gates (one per bit pair) combined with AND.
- (D) A larger n-bit magnitude comparator can be built by cascading smaller comparator stages, using the equality output of the more-significant stage to enable comparison of the less-significant bits.

---

### Q8 · NAT · Moderate
A BCD-to-Excess-3 code converter outputs, for BCD input 0110 (decimal 6), the Excess-3 code. Interpreting that 4-bit Excess-3 output as an unsigned binary number, its decimal value is ______.

---

### Q9 · MSQ · Hard
Which of the following statements about combinational circuit design is/are TRUE?

- (A) A half adder can be built from a single XOR gate (for sum) and a single AND gate (for carry), using only 2 gates total.
- (B) A full adder can be built from two half adders and one OR gate.
- (C) A decoder can be used, together with external OR gates, to implement any combinational function of its input variables, since each output line corresponds to exactly one minterm.
- (D) An encoder is the functional inverse of a decoder, converting one active input line (out of 2ⁿ) back into an n-bit code, though a basic (non-priority) encoder malfunctions if more than one input is active simultaneously.

---

### Q10 · NAT · Killer
A 4-bit ripple-carry **subtractor** computes A − B using two's complement (B is bitwise complemented and fed into the adder chain with an initial carry-in of 1, equivalent to adding A + B̄ + 1). It is used to compute 1011₂ (11) − 0110₂ (6). The 4-bit result (as an unsigned binary interpretation of the sum output bits, ignoring the final carry-out) is ______  (give as a decimal number).

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | B | Most significant variable to most significant select line is the standard convention |
| 2 | 1 | 1⊕1⊕1 = 1 |
| 3 | A, B, C | (D) is false — gate count comparison depends on the specific function; no universal guarantee |
| 4 | 2 | At (A,B)=(0,0) and (1,0), both C=0 and C=1 give F=1, so those data inputs are the constant 1 (verified: inputs are 1, C, 1, C) |
| 5 | A | F1 has 3 minterms (3-input OR), F2 has 4 minterms (4-input OR) |
| 6 | 2 | 11 + 6 = 17 = 10001₂; verified by simulating the full-adder chain: carries out of bit 0 and bit 3 are 1 |
| 7 | A, B, C, D | All four are standard, correct facts about magnitude comparators |
| 8 | 9 | 6 + 3 = 9 (Excess-3 code for 6 is 1001₂ = 9) |
| 9 | A, B, C, D | All four are standard, correct facts about these standard combinational building blocks |
| 10 | 5 | 11 − 6 = 5; verified by the two's-complement-subtraction identity (A + B̄ + 1 mod 16 = 5 with a carry-out of 1, correctly indicating no borrow) |
