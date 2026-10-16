# DL Notes 01 — Number Systems, Complements & Codes (High-Yield Deep Dive)

> **Why this file exists:** "Number System" is the first video topic for Digital Logic — and the cheapest marks in the subject. GATE asks four shapes: (1) **conversions** (any base ↔ decimal, binary ↔ octal/hex grouping); (2) **signed representations** (sign-magnitude / 1's / 2's, ranges, the −2ⁿ⁻¹ edge case); (3) **arithmetic** (2's-complement add/subtract, **overflow detection**, BCD addition with +6 correction); (4) **codes** (BCD, excess-3, Gray, weighted/reflective codes, unused codes). Every numeric answer below was **verified by computation** (conversions, complement arithmetic, overflow vectors, BCD digit simulation).
>
> **Companion question bank:** `../DL_02_Number_Representation_Fixed_Point.md`. PW source: `../Reference/PW_Subject_Handbooks/PW_DigitalLogic.txt` (§5.1–5.2, pages 3.68–3.75). *Floating point (IEEE 754) is intentionally **not** repeated here — it lives in `../../COA/Notes/COA_Notes_03_IEEE754_Floating_Point.md`.*

---

## 0. The mental model: everything is "digit × weight"

```
   value = Σ (digit_i × base^i)        i extends left (+) and right (−) of the point

   base 2 {0,1}   base 8 {0..7}   base 10 {0..9}   base 16 {0..9, A..F}
   24 23 22 21 20 . 2−1 2−2 …      each position has "weight" base^i
```

Two conversion directions, two recipes — never mix them up:

| Direction | Recipe |
|---|---|
| any base → decimal | expand by weights and add: (673)₈ = 6·64+7·8+3 = **443** |
| decimal → any base | integer part: divide by base, collect remainders **bottom-up**; fraction: multiply by base, collect carries **top-down** |
| binary ↔ octal | group **3** bits from the point (pad with zeros) |
| binary ↔ hex | group **4** bits from the point |

**Worked fraction conversion:** (0.6875)₁₀ → binary: 0.6875×2 = **1**.375 → 0.375×2 = **0**.75 → 0.75×2 = **1**.5 → 0.5×2 = **1**.0 ⇒ **0.1011₂** (multipliers: 1, 0, 1, 1 — stop when fraction = 0).

**Quick decimal anchors to keep in RAM:** (2A7)₁₆ = 512+160+7 = **679**; (1101011)₂ = 64+32+8+2+1 = **107**; (432)₅ = 100+15+2 = **117**.

---

## 1. Complements — the machinery behind subtraction

For base r with n digits:

```
   (r−1)'s complement:  (rⁿ − 1) − N      →  per-digit complement to r−1
   r's complement:      rⁿ − N            →  (r−1)'s complement + 1
   binary:              1's  = flip every bit;   2's = flip every bit, then add 1
```

**Fast 2's-complement trick (PW §5.2):** copy bits from the **right up to and including the first 1**, then flip everything to its left. Example: 10011000 → 01101000 ✓.

**Subtraction via addition:** A − B = A + (2's complement of B), then **discard the final carry-out** (for unsigned arithmetic in n bits).

---

## 2. Signed representations — the comparison table GATE loves

For an n-bit register (example with 8 bits, value −87):

| Representation | How | −87 in 8 bits | Range (n bits) | Zero |
|---|---|---|---|---|
| Sign-magnitude | sign bit + magnitude | 1 1010111 | −(2ⁿ⁻¹−1) … +(2ⁿ⁻¹−1) | two zeros (±0) |
| 1's complement | flip all bits of +87 | 1 0101000 | −(2ⁿ⁻¹−1) … +(2ⁿ⁻¹−1) | two zeros (±0) |
| 2's complement | 1's complement + 1 | **1 0101001 = 0xA9** | **−2ⁿ⁻¹ … +2ⁿ⁻¹−1** | **unique zero** |

**The three facts that decide most MCQs:**
1. Only **2's complement** has one zero and one extra negative value: for 8 bits the range is **−128 … +127** (the −128 = 10000000 pattern has *no* positive counterpart). For 12 bits: **−2048 … +2047**.
2. Negation = flip + 1 — and it works "in place" (2's complement of the 2's complement returns the original).
3. Sign test: MSB = 1 ⇔ negative (in all three signed forms). But `1000 0000` is −128 in 2's complement — *not* "negative zero".

**Worked: 61 + (−87) in 8-bit two's complement**

```
    61 = 0011 1101
   −87 = 1010 1001      (2's complement: 0101 0111 → 1010 1000 → +1)
   sum = 1110 0110      = 230 unsigned;   as signed: −(0001 1010) = −26   ✓ 61−87 = −26
```

**Overflow — the #1 trap.** Overflow is a property of *signed* addition:
- **Rule (works always):** overflow ⇔ the two addends have the **same sign** but the result has the **opposite sign**.
- Carry-out of the MSB ≠ overflow! (e.g. −43 + −73 = −116 leaves a carry-out in some notations yet is perfectly fine; 110 + 91 = 201 flips the sign bit → overflow though the bit pattern's carry tells you nothing by itself).
- For subtraction A − B in 2's complement, the same rule applies to A + (2's comp B).

---

## 3. BCD and special codes (PW §5.1.2)

**BCD (8421, weighted):** each decimal *digit* gets its own 4-bit code. 25₁₀ → 0010 0101 (NOT 11001₂!). Codes 1010–1111 are **invalid** (6 unused patterns) — classic "which are not valid BCD" question.

**BCD addition** — add nibble-wise like binary, then correct each nibble:
```
   if a nibble sum > 9  (or produced a carry):  add +6 to that nibble
```

**Worked, verified: 4729 + 3586**

| position | raw sum | correction | digit (carry) |
|---|---|---|---|
| units | 9+6 = 15 | +6 → 21 | 1, carry 1 |
| tens | 2+8+1 = 11 | +6 → 17 | 7, carry 1 |
| hundreds | 7+5+1 = 13 | +6 → 19 | 9, carry 1 |
| thousands | 4+3+1 = 8 | none | 8 |

Result: **8315**, with the +6 correction applied **3 times**.

**Excess-3:** code(d) = d + 3 (BCD form). 9 → 1100. It is unweighted yet **self-complementing**: the 9's complement of a digit ↔ 1's complement of its code (e.g. 4 = 0111, 5 = 1000 — bitwise flips).

**BCO / BCH:** Binary-Coded-Octal / -Hex: 3 bits per octal digit, 4 per hex digit — and unlike BCD, these are *exactly* the plain binary value (grouping shortcut). **Unused states:** BCD leaves 6; excess-3 leaves 6 (0000, 0001, 0010, 1101, 1110, 1111).

**Gray code (standard GATE topic — supplementary to PW, tied to K-map ordering in File 02):**

```
   binary → Gray:  g1 = b1 (MSB copy);  gi = b(i−1) ⊕ bi      (adjacent XOR)
   Gray → binary:  b1 = g1;             bi = b(i−1) ⊕ gi      (running XOR)

   worked: 1011 → 1110     (1, 1⊕0=1, 0⊕1=1, 1⊕1=0)
   worked: 1101 → 1001     (Gray back to binary)
```

**Code taxonomy (PW §5.2):** *weighted* (8421, 2421, 5211), *reflective/self-complementing* (2421, excess-3), *sequential* (BCD), plus parity codes (details in File 03 §8). ASCII and Hollerith are mentioned only in passing.

---

## 4. Traps checklist

1. **BCD ≠ binary:** 15 = 1111₂ but BCD(15) = 0001 0101 (two nibbles).
2. **Carry-out ≠ overflow** (see §2 rule).
3. **−2ⁿ⁻¹ edge:** in 8-bit 2's complement, 128 has *no* representation; negating −128 overflows (result = −128, not +128).
4. **2's complement of 0 is 0** (flip 000…0 → 111…1 → +1 → 000…0 + carry discarded).
5. **Excess-3 complement:** the 9's complement of digit d corresponds to the 1's complement of code(d) — works because d + (9−d) = 9 and 0011 + 1100 = 1111.
6. **Grouping direction:** binary→hex groups from the radix point outward (pad the *left* of the integer and *right* of the fraction).
7. **Fraction conversions don't always terminate** (e.g. 0.1₁₀) — GATE sticks to terminating ones like 0.6875, 0.375.
8. **Gray is unweighted** — "Gray code value = numeric value of bits" is always false.

---

## 5. Practice set (10 questions, all values verified by computation)

**Q1 · MCQ · Fundamental** — Which of the following conversions is CORRECT?
(A) (673)₈ = 489₁₀  (B) (2A7)₁₆ = 679₁₀  (C) (1101011)₂ = 105₁₀  (D) (432)₅ = 127₁₀

**Q2 · NAT · Fundamental** — The largest value representable in **12-bit two's complement** is ______.

**Q3 · NAT · Standard** — Using 8-bit two's complement arithmetic, compute 61 − 87. The 8-bit result, interpreted as a signed value, is ______ (give the decimal value).

**Q4 · MSQ · Standard** — Which statements about two's complement are TRUE?
(A) The successor of the largest positive n-bit number is the smallest negative n-bit number (the arithmetic wraps around).
(B) Negating a number = flip all bits and add 1.
(C) Adding a number to its two's complement negation yields 0, with any final carry-out discarded.
(D) Adding two 8-bit two's complement numbers always produces a correct result in 8 bits.

**Q5 · MCQ · Standard** — Which of these 8-bit two's complement additions causes **signed overflow**?
(A) 11010101 + 10110111  (B) 01101110 + 01011011  (C) 10101010 + 00110011  (D) 00001010 + 11110110

**Q6 · NAT · Standard** — 4-digit BCD numbers 4729 and 3586 are added using standard BCD addition (digit-by-digit, +6 correction when a digit sum exceeds 9). The decimal value of the final result is ______.

**Q7 · MSQ · Moderate** — Which statements about codes are TRUE?
(A) Excess-3 is a self-complementing code.
(B) The 2421 code is a weighted code that is also self-complementing.
(C) Gray code is a weighted code.
(D) Unweighted codes cannot be self-complementing.

**Q8 · NAT · Moderate** — The 4-bit binary number 1011 (decimal 11) is converted to Gray code. The Gray code output is ______.

**Q9 · MSQ · Hard** — Which statements about signed representations are TRUE?
(A) Sign-magnitude has a symmetric range (−(2ⁿ⁻¹−1) … +(2ⁿ⁻¹−1)), while two's complement extends one step further negative.
(B) In 1's complement addition, a final carry-out must be added back to the LSB (end-around carry).
(C) The 2's complement of a positive number equals the bit pattern of the same magnitude negative number.
(D) Adding a number to its 1's complement always yields all-zero bits.

**Q10 · NAT · Killer** — The hexadecimal byte 0xB6 is interpreted twice: (i) as an unsigned 8-bit integer, and (ii) as a signed 8-bit two's complement integer. The sum of the two interpreted values is ______.

---

### Answer key

| Q | Ans | Q | Ans |
|---|---|---|---|
| 1 | B | 6 | 8315 |
| 2 | 2047 | 7 | A, B |
| 3 | −26 | 8 | 1110 |
| 4 | A, B, C | 9 | A, B, C |
| 5 | B | 10 | 108 |

### Full solutions

**Q1 — B.** (2A7)₁₆ = 2·256 + 10·16 + 7 = **679** ✔. (A) (673)₈ = 6·64+7·8+3 = 443 (not 489). (C) (1101011)₂ = 64+32+8+2+1 = 107 (not 105). (D) (432)₅ = 4·25+3·5+2 = 117 (not 127).

**Q2 — 2047.** 12-bit two's complement range: −2¹¹ … 2¹¹−1 = −2048 … **2047** (n-bit: −2ⁿ⁻¹ … 2ⁿ⁻¹−1).

**Q3 — −26.** 61 = 00111101; −87 = 10101001. Sum = 11100110. MSB = 1 ⇒ negative; magnitude = 2's complement of 11100110 = 00011010 = 26 ⇒ **−26** ✔.

**Q4 — A, B, C.** (A) ✔ e.g. 0111…1 + 1 = 1000…0 (hardware wraps silently). (B) ✔ definition. (C) ✔ x + (−x) = 0 (mod 2ⁿ); the carry out of the MSB is simply discarded. (D) ✘ overflow is exactly the failure case (e.g. 110 + 91).

**Q5 — B.** (B) 110 + 91 = 201; bits 11001001 — sign flipped from 0 to 1 with both addends positive ⇒ **overflow**. (A) −43 + −73 = −116 ∈ [−128,127] ✔ (C) −86 + 51 = −35 ✔ (D) 10 + (−10) = 0 ✔.

**Q6 — 8315.** Per-digit with correction (see §3 table): 15(+6)→1 c1; 2+8+1 = 11(+6)→7 c1; 7+5+1 = 13(+6)→9 c1; 4+3+1 = 8. Result **8315** (3 corrections applied).

**Q7 — A, B.** (A) ✔ digit and its 9's complement have bit-flipped codes. (B) ✔ 2421 is weighted (weights 2,4,2,1) and self-complementing: 4 = 0100, 5 = 1011 (bitwise flip). (C) ✘ Gray is unweighted (positions have no fixed weights). (D) ✘ excess-3 is unweighted AND self-complementing — the statement is false.

**Q8 — 1110.** g3 = b3 = 1; g2 = b3⊕b2 = 1⊕0 = 1; g1 = b2⊕b1 = 0⊕1 = 1; g0 = b1⊕b0 = 1⊕1 = 0 → **1110**.

**Q9 — A, B, C.** (A) ✔ SM/1's ranges are symmetric; two's complement adds the extra value −2ⁿ⁻¹. (B) ✔ 1's complement uses end-around carry (PW's example: −6 + 9 = 3 via 1111001 + 0001001 = (1)0000010, wrapped carry +1 → 0000011). (C) ✔ exactly the encoding definition. (D) ✘ a number plus its 1's complement gives all **ones** (the "negative zero" pattern 111…1), not all zeros. *(Same-family fact worth keeping: a positive plus a negative 2's complement number can never overflow — the result lies between the two operands.)*

**Q10 — 108.** (i) unsigned: 0xB6 = 182. (ii) signed: 10110110 ⇒ negative; magnitude = 2's comp = 01001010 = 74 ⇒ −74. Sum = 182 + (−74) = **108**.

---

## 6. Next steps

1. Drill §1–§2 tables until conversions are reflex.
2. Re-derive the overflow rule with the Q5 vectors — that's the module GATE reuses.
3. Then `DL_Notes_02_Boolean_Algebra_Minimization.md` — where Gray ordering from §3 shows up directly in K-maps.
