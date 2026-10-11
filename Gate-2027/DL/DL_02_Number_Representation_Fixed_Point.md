# Digital Logic → Number Representation & Fixed-Point Arithmetic

**GATE 2027 syllabus:** Digital Logic — number representation and arithmetic (fixed point).
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** n-bit two's complement range: −2ⁿ⁻¹ to 2ⁿ⁻¹−1. Overflow in addition occurs when the mathematically correct sum falls outside this range. BCD represents each decimal digit in 4 bits (8421 code); BCD addition adds digit-pairs as 4-bit binary values and adds 6 (correction) whenever a digit-pair sum exceeds 9, generating a carry into the next digit.

---

## Questions

### Q1 · MCQ · Fundamental
In 8-bit two's complement representation, the decimal number −45 is

- (A) 11010011
- (B) 11010010
- (C) 10101101
- (D) 01010011

---

### Q2 · NAT · Fundamental
The range of values representable in 16-bit two's complement is −32768 to ______.

---

### Q3 · MSQ · Standard
Which of the following statements about two's complement representation is/are TRUE?

- (A) There is exactly one representation of zero.
- (B) The most negative representable n-bit value has no corresponding positive representable value of the same magnitude.
- (C) Overflow in addition can be detected by comparing the carry into the sign bit with the carry out of the sign bit — they differ exactly when overflow occurs.
- (D) Subtraction A − B can be performed as addition of A and the two's complement of B.

---

### Q4 · NAT · Standard
Two 8-bit two's complement numbers, 100 and −50, are added. Interpreting the 8-bit result as a signed two's complement number, the result is ______  (give as a signed decimal number; note this addition **does not** overflow, since both inputs together with the true sum 50 fit in the 8-bit range).

---

### Q5 · MCQ · Standard
Two 8-bit two's complement numbers, −100 and −50, are added. Which one of the following correctly describes the result?

- (A) No overflow; result is −150
- (B) Overflow occurs, since the true sum −150 is outside the 8-bit range [−128, 127]
- (C) No overflow; result is 106 (the wrapped-around value)
- (D) Overflow occurs, but the wrapped 8-bit result still correctly represents −150 if reinterpreted as unsigned

---

### Q6 · NAT · Moderate
Two 4-digit BCD numbers, 2785 and 1948, are added using standard BCD addition (digit-by-digit, with a +6 correction and carry whenever a digit-pair sum exceeds 9). The number of digit positions (out of 4) that require the +6 correction is ______.

---

### Q7 · MSQ · Moderate
Which of the following statements about BCD (Binary-Coded Decimal) arithmetic is/are TRUE?

- (A) Each BCD digit uses 4 bits but only 10 of the 16 possible bit patterns (0000–1001) are valid digits.
- (B) BCD addition can require a correction step that a pure binary adder does not need, specifically because 4-bit binary addition naturally produces results in the full 0–15 range, not the 0–9 range needed by BCD.
- (C) BCD is more memory-efficient (fewer bits per stored number) than pure binary for representing large decimal integers.
- (D) BCD is commonly used in applications (like financial calculations) where exact decimal rounding behavior matters.

---

### Q8 · NAT · Moderate
Two 4-digit BCD numbers, 5 8 9 (treated as 589, i.e. with a leading 0 to make 0589) and 4 6 7 (0467), are added using standard BCD addition. The number of digit positions (out of 4, including the leading-zero positions) that require the +6 correction is ______.

---

### Q9 · MSQ · Hard
Which of the following statements about alternative number representations is/are TRUE?

- (A) In sign-magnitude representation, there are two distinct representations of zero (+0 and −0).
- (B) In one's complement representation, there are also two distinct representations of zero.
- (C) An n-bit excess-K (biased) representation stores the value v as the unsigned binary encoding of (v + K); it is commonly used for floating-point exponents because it allows magnitude comparison using unsigned integer comparison.
- (D) Gray code is a binary encoding where consecutive values differ in exactly one bit position, making it useful for minimizing glitches in applications like rotary encoders.

---

### Q10 · NAT · Killer
Three 8-bit two's complement additions are performed: (100) + (50), (−100) + (−50), and (−128) + (−1). Let V be the number of these three additions (out of 3) that produce an overflow. Let W be the sum of the correctly-wrapped 8-bit results of the additions that **do** overflow, each reinterpreted as an 8-bit signed two's complement number (i.e., sum only the "wrong," overflowed results, in their wrapped signed form). The value of V × 10 + W is ______  (W may be negative; compute V×10 + W algebraically).

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | A | 45 = 00101101; two's complement (invert + 1) = 11010011 |
| 2 | 32767 | 2¹⁵ − 1 |
| 3 | A, B, C, D | All four are standard, correct facts about two's complement |
| 4 | 50 | 100 + (−50) = 50, well within range, no overflow |
| 5 | B | True sum −150 < −128, outside the 8-bit range: overflow |
| 6 | 3 | Digit pairs (from the right): 5+8=13>9 (correct), 8+4+1=13>9 (correct), 7+9+1=17>9 (correct), 2+1+1=4 (no correction) — 3 corrections |
| 7 | A, B, D | (C) is false — BCD uses more bits than pure binary for the same decimal value (since 4 bits per digit wastes 6 of 16 codes) |
| 8 | 3 | Verified by simulation: 9+7=16 (correct), 8+6+1=15 (correct), 5+4+1=10 (correct), 0+0+1=1 (no correction) |
| 9 | A, B, C, D | All four are standard, correct facts about these representations |
| 10 | 157 | All three additions overflow: (100,50)→−106, (−100,−50)→106, (−128,−1)→127; V=3, W = −106+106+127 = 127; V×10+W = 30+127 |
