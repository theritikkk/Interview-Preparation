# Digital Logic → Floating-Point Representation (IEEE 754)

**GATE 2027 syllabus:** Digital Logic — number representation and arithmetic (floating point).
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** IEEE 754 single precision: 1 sign bit, 8 exponent bits (bias 127), 23 mantissa (fraction) bits. Value = (−1)^sign × 1.mantissa × 2^(exponent − 127) for normalized numbers (exponent field 1–254). Exponent field 0 denotes zero or denormals; exponent field 255 denotes infinity (mantissa 0) or NaN (mantissa ≠ 0). Double precision: 1 sign bit, 11 exponent bits (bias 1023), 52 mantissa bits.

---

## Questions

### Q1 · MCQ · Fundamental
In IEEE 754 single precision, the number of bits allocated to the exponent field is

- (A) 7
- (B) 8
- (C) 11
- (D) 23

---

### Q2 · NAT · Fundamental
In IEEE 754 single precision, the exponent field uses a bias of ______.

---

### Q3 · MSQ · Standard
Which of the following statements about IEEE 754 single precision is/are TRUE?

- (A) An exponent field of all 1s (255) with a zero mantissa represents ±infinity.
- (B) An exponent field of all 1s (255) with a nonzero mantissa represents NaN (Not a Number).
- (C) An exponent field of all 0s represents zero or a denormalized (subnormal) number.
- (D) The leading 1 of a normalized mantissa is stored explicitly as part of the 23 mantissa bits.

---

### Q4 · NAT · Standard
The decimal value 10.25 is represented in IEEE 754 single precision. The biased exponent field (as an 8-bit unsigned integer) is ______.

---

### Q5 · MCQ · Standard
The IEEE 754 single-precision hexadecimal bit pattern 0xC1200000 represents the decimal value

- (A) 10.0
- (B) −10.0
- (C) 5.0
- (D) −5.0

---

### Q6 · NAT · Moderate
The decimal value −7.5 is represented in IEEE 754 single precision. The unbiased exponent (true exponent, i.e., biased exponent − 127) is ______.

---

### Q7 · MSQ · Moderate
Which of the following statements about floating-point precision and rounding is/are TRUE?

- (A) 0.1 + 0.2 is not exactly equal to 0.3 in IEEE 754 double precision, due to representation error in the binary fractions.
- (B) IEEE 754 single precision can represent every integer from 0 up to 2²⁴ exactly, but not necessarily every integer beyond that (e.g., 2²⁴ + 1 cannot be represented exactly).
- (C) The "machine epsilon" for single precision (the smallest representable difference between 1.0 and the next larger representable value) is on the order of 10⁻⁷.
- (D) Floating-point addition is associative: (a + b) + c always exactly equals a + (b + c) for any three floating-point values a, b, c.

---

### Q8 · NAT · Moderate
The decimal value 100.0 is represented in IEEE 754 single precision. The biased exponent field (as an 8-bit unsigned integer, i.e., the decimal value of the 8-bit exponent field) is ______.

---

### Q9 · MSQ · Hard
Which of the following statements is/are TRUE?

- (A) IEEE 754 double precision has a larger exponent range and more mantissa bits than single precision, giving both greater range and greater precision.
- (B) Denormalized (subnormal) numbers allow representing values smaller in magnitude than the smallest normalized number, at the cost of reduced precision (fewer significant bits) as the magnitude decreases further.
- (C) Comparing two IEEE 754 floating-point bit patterns as if they were unsigned integers gives the same ordering as comparing the represented values, for all pairs of positive normalized floating-point numbers.
- (D) The IEEE 754 standard specifies exactly one representation each for +0 and −0, and these two bit patterns are numerically distinct when compared with the standard floating-point equality operator (0.0 == −0.0 evaluates to false under IEEE 754 semantics).

---

### Q10 · NAT · Killer
Four IEEE 754 single-precision hexadecimal bit patterns are given: 0x41200000, 0x3F000000, 0x00000000, and 0xC1200000. Let S be the sum of the four decimal values these patterns represent. The value of S is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | B | 8 exponent bits in single precision |
| 2 | 127 | Standard single-precision bias |
| 3 | A, B, C | (D) is false — the leading 1 is implicit (hidden) and not stored |
| 4 | 130 | 10.25 = 1.28125 × 2³; biased exponent = 3 + 127 |
| 5 | B | Verified by decoding: 0xC1200000 → −10.0 |
| 6 | 2 | −7.5 = −1.875 × 2²; unbiased exponent 2 |
| 7 | A, B, C | (D) is false — floating-point addition is not associative in general, due to rounding |
| 8 | 133 | 100.0 = 1.5625 × 2⁶; biased exponent = 6 + 127 |
| 9 | A, B, C, D | All four are standard, correct facts about IEEE 754 |
| 10 | 0.5 | 0x41200000 = 10.0, 0x3F000000 = 0.5, 0x00000000 = 0.0, 0xC1200000 = −10.0; sum = 10.0 + 0.5 + 0.0 − 10.0 |
