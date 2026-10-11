# Computer Architecture → ALU Design (Adders, Multiplication, Booth's Algorithm)

**GATE 2027 syllabus:** Computer Organization and Architecture — design of arithmetic and logic unit (ALU).
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** A ripple-carry adder (RCA) of n bits has each full-adder's carry-out feed the next stage; its worst-case delay is n × (delay per full adder). Booth's algorithm recodes a signed multiplier bit-by-bit: examining the current bit Qᵢ and the previously examined bit (initially an implicit 0), a 0→1 transition triggers a subtraction of the multiplicand, a 1→0 transition triggers an addition, and no transition (00 or 11) triggers neither — only a shift.

---

## Questions

### Q1 · MCQ · Fundamental
The worst-case propagation delay of an n-bit ripple-carry adder, where each full adder has a carry propagation delay of d, is

- (A) O(1), independent of n
- (B) O(log n) × d
- (C) O(n) × d
- (D) O(n²) × d

---

### Q2 · NAT · Fundamental
A 32-bit ripple-carry adder has a per-bit carry delay of 2 ns. Its worst-case total delay, in ns, is ______.

---

### Q3 · MSQ · Standard
Which of the following statements about **carry-lookahead adders (CLA)** is/are TRUE?

- (A) A CLA computes carry signals using generate (G) and propagate (P) terms for each bit position, in parallel, rather than waiting for the ripple.
- (B) A CLA is generally faster than a ripple-carry adder of the same width, at the cost of more hardware (gates).
- (C) A CLA completely eliminates the dependency of any output bit on any input bit — all output bits are computed simultaneously with the same, fixed delay regardless of adder width.
- (D) Large CLAs are often built hierarchically (e.g., 4-bit CLA blocks combined with a second level of lookahead across blocks) to balance speed and hardware complexity.

---

### Q4 · NAT · Standard
Using Booth's algorithm on a 5-bit signed multiplier Q = −3 (in 5-bit two's complement, i.e. bit pattern 11101), examining bits from the LSB (with an implicit initial previous bit of 0), the total number of **arithmetic operations** (additions plus subtractions of the multiplicand; steps with no transition don't count) performed is ______.

---

### Q5 · MCQ · Standard
Booth's algorithm is primarily used to

- (A) detect overflow in addition
- (B) perform signed multiplication with a variable, potentially reduced, number of add/subtract operations depending on the multiplier's bit pattern
- (C) perform division of two signed numbers
- (D) convert between sign-magnitude and two's complement representations

---

### Q6 · NAT · Moderate
Using Booth's algorithm on a 6-bit signed multiplier Q = 23 (bit pattern 010111), examining bits from the LSB (implicit initial previous bit 0), the total number of arithmetic operations (additions plus subtractions) performed is ______.

---

### Q7 · MSQ · Moderate
Which of the following statements about combinational ALU design is/are TRUE?

- (A) A single ALU circuit can be designed to compute several different operations (e.g., ADD, SUB, AND, OR), selected by control input lines.
- (B) Subtraction (A − B) can be implemented as addition of A and the two's complement of B, reusing the same adder hardware used for addition.
- (C) Overflow in signed addition can be detected by comparing the carry into the most-significant bit position with the carry out of that position — they differ exactly when overflow occurs.
- (D) An n-bit ALU's AND/OR/XOR logic operations, performed bitwise, have the same O(n) worst-case gate-delay dependency on n as the adder's carry chain.

---

### Q8 · NAT · Moderate
Using Booth's algorithm on a 5-bit signed multiplier Q = 7 (bit pattern 00111), examining bits from the LSB (implicit initial previous bit 0), the total number of arithmetic operations (additions plus subtractions) performed is ______.

---

### Q9 · MSQ · Hard
Which of the following statements about Booth's algorithm is/are TRUE?

- (A) A run of consecutive 1s of length k in the multiplier's bit pattern is handled with exactly 2 arithmetic operations (one subtraction at the start of the run, one addition at the end), regardless of k (for k ≥ 1).
- (B) A multiplier of all 1s (e.g., representing −1) can be handled with just 1 arithmetic operation if the run extends to the implicit leading sign-extension bits.
- (C) Booth's algorithm can result in **more** arithmetic operations than naive shift-and-add multiplication for some bit patterns (e.g., alternating 1010...10), making it not universally faster.
- (D) Booth's algorithm works only for positive multipliers; negative multipliers must first be converted to their absolute value.

---

### Q10 · NAT · Killer
Using Booth's algorithm on a 6-bit signed multiplier Q = −13 (bit pattern 110011), examining bits from the LSB (implicit initial previous bit 0), let A be the number of subtraction operations and S the number of addition operations performed (no-transition steps don't count). The value of A × 10 + S is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | C | Each stage waits for the previous stage's carry: O(n) delay |
| 2 | 64 | 32 × 2 |
| 3 | A, B, D | (C) is false — even CLAs have some dependency on width (via block/hierarchy delay), just far less than ripple carry |
| 4 | 3 | Bit pattern (LSB→MSB) 1,0,1,1,1 with prev=0: transitions SUB, ADD, SUB, NOP, NOP → 3 operations |
| 5 | B | Definition and purpose of Booth's algorithm |
| 6 | 4 | Bit pattern (LSB→MSB) 1,1,1,0,1,0 with prev=0: SUB, NOP, NOP, ADD, SUB, ADD → 4 operations |
| 7 | A, B, C, D | All four are standard, correct facts about ALU design |
| 8 | 2 | Bit pattern (LSB→MSB) 1,1,1,0,0 with prev=0: SUB, NOP, NOP, ADD, NOP → 2 operations |
| 9 | A, B, C | (D) is false — Booth's algorithm is specifically designed to handle negative multipliers directly, in two's complement, without any separate conversion |
| 10 | 21 | Bit pattern (LSB→MSB) 1,1,0,0,1,1 with prev=0: SUB, NOP, ADD, NOP, SUB, NOP → S=1 addition, A=2 subtractions; A×10+S = 20+1 |
