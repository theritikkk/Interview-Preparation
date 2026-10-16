# COA Notes 03 — IEEE 754 Floating Point (High-Yield Deep Dive)

> **Why this file exists:** "IEEE 754 Floating Point" is on the high-yield list. GATE rarely asks "define the format" — it asks you to **encode a decimal into hex**, **decode a hex into decimal**, or reason about range/precision (denormals, bias, spacing). All of that is drillable arithmetic once the layout is internalized. Built using the PW handbook's floating-point basics plus derivations and a verified 10-question practice set.
>
> **Companion question bank:** `DL_03_Floating_Point_IEEE754.md` (floating point is indexed under Digital Logic in the syllabus). PW sources: `../Reference/PW_Subject_Handbooks/PW_DigitalLogic.txt` (§5.2.2).

---

## 0. The mental model: scientific notation, but binary

Decimal scientific notation: `−1.25 × 10³` — sign, significand, exponent. Floating point stores the same three things in binary:

```
  value = (−1)^s × 1.f × 2^(E)        (normalized numbers)
```

- **s** — sign bit (0 = positive, 1 = negative). Note: **not** two's complement — the rest of the number is magnitude.
- **f** — the fraction bits (everything after the binary point).
- **E** — the *true* exponent; what's stored is `E + bias` (biased/excess form).

Fixed-point stores a point at one fixed place (limited range); floating point *moves* the point (`m · r^e`, PW's general form) so the same bits cover a huge range — at the cost of precision that shrinks as the magnitude grows.

**Layouts to memorize:**

| Format | Bits | Sign | Exponent field | Fraction | Bias | True exponent range |
|---|---|---|---|---|---|---|
| Single | 32 | 1 | 8 | 23 | 127 | −126 … +127 |
| Double | 64 | 1 | 11 | 52 | 1023 | −1022 … +1023 |

**Why the bias?** Storing `E + 127` keeps the exponent field non-negative, so the whole float can be compared like an unsigned/sign-magnitude integer (bigger bits = bigger number). It also frees the all-zero exponent field to mean zero/denormals, and all-ones to mean infinity/NaN.

**Why normalization + implicit 1?** A normalized number always has a 1 before the binary point (`1.f`), so the 1 is **not stored** — every stored fraction gains one free bit of precision ("more significant digits", PW).

---

## 1. Encoding a decimal → IEEE single (the 5-step recipe)

1. **Sign:** strip the sign, remember it.
2. **Convert to binary** (integer part + fraction part).
3. **Normalize** to the form `1.f × 2^E` (move the point so exactly one 1 remains to its left).
4. **Bias the exponent:** field = `E + 127`, written in 8 bits.
5. **Assemble:** `s | exponent field (8) | fraction f (23, zero-padded)`.

### Worked example 1 — encode 10.25

```
  10.25  = 1010.01                (integer 10 = 1010; 0.25 = 01)
  1010.01 = 1.01001 × 2^3         → E = 3
  field   = 3 + 127 = 130 = 10000010
  fraction f = 01001 → 0100 1000 0000 0000 0000 000
  bits: 0 10000010 01001000000000000000000  →  0x41240000
```

### Worked example 2 — encode 25.625

```
  25.625 = 11001.101 = 1.1001101 × 2^4
  field  = 131 = 10000011;  f = 1001101 → 10011010000000000000000
  bits: 0 10000011 10011010000000000000000  →  0x41CD0000
```

Sanity check the fraction as a decimal: `0.1001101₂ = 0.5 + 0.0625 + 0.03125 + 0.0078125 = 0.6015625`; significand `1.6015625 × 16 = 25.625` ✔.

### Worked example 3 — encode −0.5

```
  0.5 = 0.1 = 1.0 × 2^(−1)   → field = −1 + 127 = 126 = 01111110; f = 0
  bits: 1 01111110 00000000000000000000000  →  0xBF000000
```

**Binary fraction drill (the only real skill):** repeatedly multiply the decimal fraction by 2; the integer part of each product is the next bit. `0.625 → 1.25 →0.5 →1.0` gives `101`; stop when it hits 0 (or after 23+ bits — see rounding).

---

## 2. Decoding IEEE single → decimal (reverse recipe)

1. Split bits: `s | field(8) | f(23)`.
2. True exponent `E = field − 127`.
3. Significand: `1 + value(f)/2^23` (i.e., `1.f`).
4. Value = `(−1)^s × (1.f) × 2^E`.

### Worked example 4 — decode 0x41C80000

```
  bits:   0 10000011 10010000000000000000000
  s = 0;  field = 131 → E = 4;  f = 0.1001₂ = 0.5625
  value = 1.5625 × 2^4 = 25.0
```

### Worked example 5 — decode 0xC1A00000

```
  s = 1;  field = 131 → E = 4;  f = 0.25
  value = −1.25 × 16 = −20.0
```

---

## 3. Special values: what the reserved patterns mean

| Exponent field | Fraction | Meaning |
|---|---|---|
| 0 | 0 | ±0 (two bit patterns; they compare equal) |
| 0 | ≠ 0 | **Denormalized**: value = ±(0.f) × 2^−126 → fills the gap near zero *with reduced precision* |
| 1 … 254 | any | normal number: ±(1.f) × 2^(E−127) |
| 255 | 0 | ±∞ (overflow, division by zero) |
| 255 | ≠ 0 | **NaN** (0/0, ∞−∞, invalid) |

Denormals exist so that the distance from zero to the smallest representable number grows smoothly instead of a sudden hole; their implicit leading bit is **0**, not 1 — that's what "denormalized" buys, at lower precision.

---

## 4. Range and precision (derive on the spot)

**Single precision:**
- Smallest positive **normal**: `1.0 × 2^−126 ≈ 1.18 × 10^−38`.
- Smallest positive **denormal**: `2^−149 ≈ 1.4 × 10^−45`.
- Largest finite: `(2 − 2^−23) × 2^127 ≈ 3.4 × 10^38` (exponent field 254, all fraction bits 1).
- **Spacing (ulp)** in the range `[2^E, 2^(E+1))`: `2^(E−23)`. At 1.0 → `2^−23 ≈ 1.19 × 10^−7` (≈ 7 decimal digits). At 2^24 → exactly **2** — which is why `2^24 + 1` is not representable and rounds (ties-to-even: down to 2^24).
- **Double:** same structure; ulp at 1.0 = `2^−52` (≈ 16 decimal digits).

**Rounding (GATE occasionally):** IEEE default = **round to nearest, ties to even**. Implementation detail: keep **guard, round, sticky** bits beyond the fraction; ties decided so the last kept bit is even.

---

## 5. Traps & exam patterns

1. **The implicit 1** — the most-forgotten bit. `field=127` means E=0 and significand starts at 1.0, not 0.
2. **Exponent field ≠ true exponent** — always subtract the bias when decoding, add when encoding.
3. **Not two's complement:** negative numbers are sign-magnitude; `0xBF000000` is −0.5, not "two's complement of something".
4. **"Smallest positive value"** — check whether the question means *normal* (2^−126) or *denormal* (2^−149).
5. **Precision is relative, not absolute:** floats near 10^30 cannot represent unit differences — spacing = `2^(E−23)`.
6. **f = 0 with field = 255 is ∞; add any nonzero fraction bit → NaN.**
7. **Hex assembly is bit-exact:** write all 32 bits (pad fraction on the right with zeros), then convert 4 hex digits at a time. Most lost marks are right-padding errors.

---

## 6. Practice set (verified by computation)

### Q1 · NAT · Fundamental
The biased (stored) exponent of `10.25` in single precision is ______.

### Q2 · MCQ · Fundamental
The single-precision pattern `0x41C80000` represents

- (A) 24.5
- (B) 25.0
- (C) 28.5
- (D) 12.5

### Q3 · NAT · Standard
In IEEE single precision, the smallest positive **denormalized** number is `2^x`. The value of `x` is ______.

### Q4 · NAT · Standard
The IEEE single-precision representation of `25.625` in hexadecimal (uppercase, 8 digits, no `0x` prefix) is ______.

### Q5 · MSQ · Standard
Which of the following is/are TRUE?

- (A) Exponent field 0 with nonzero fraction represents a denormalized number.
- (B) Exponent field 255 with zero fraction represents ±∞.
- (C) Exponent field 255 with nonzero fraction represents NaN.
- (D) Denormalized numbers use an implicit leading 1.

### Q6 · NAT · Moderate
For `12.375`, the stored 23-bit fraction field, interpreted as a decimal fraction value (e.g., 0.f as a real number), equals ______.

### Q7 · NAT · Moderate
In IEEE **double** precision, the next representable value after 1.0 is `1 + 2^x`. The value of `x` is ______.

### Q8 · NAT · Moderate
The largest positive finite single-precision number has biased exponent field ______.

### Q9 · NAT · Hard
In single precision, the gap (spacing) between consecutive representable numbers in the binade `[2^24, 2^25)` is ______.

### Q10 · NAT · Killer
The IEEE single-precision hexadecimal representation of `−0.5` is 0x\\_\\_\\_\\_\\_\\_\\_\\_ (write 8 uppercase hex digits, e.g. as `BF000000`).

---

## Answer Key

| Q | Answer | Why (condensed) |
|---|---|---|
| 1 | 130 | E = 3; 3 + 127 |
| 2 | B | field 131 → E = 4; 1.5625 × 16 |
| 3 | −149 | 0.0…01 × 2^−126 = 2^−23 × 2^−126 |
| 4 | 41CD0000 | E = 4, field 131, f = .1001101 |
| 5 | A, B, C | (D) false — implicit 0 for denormals |
| 6 | 0.546875 | f = .100011₂ |
| 7 | −52 | ulp at 1.0 = 2^−52 |
| 8 | 254 | max exponent before ∞ (255) |
| 9 | 2 | 2^24 × 2^−23 |
| 10 | BF000000 | sign 1, field 126, f = 0 |

### Full solutions

**Q1.** `10.25 = 1010.01 = 1.01001 × 2^3` → E = 3 → field = `3 + 127 = 130` (= `0x82`). (Full encoding: `0x41240000`.)

**Q2.** Split `0x41C80000` = `0100 0001 1100 1000 …`: sign 0; field = `1000 0011₂ = 131` → E = 4; fraction `f = 0x480000/2^23 = 0.5625` → `1.5625 × 2^4 = 25.0`. **B**. (Distractors: `0x41C40000` would give 24.5 — a one-bit change in f.)

**Q3.** Smallest denormal: fraction bit pattern `00…01` with field 0 → fraction value `2^−23`, times `2^−126` → `2^−149`. Answer **−149**. (Contrast: smallest *normal* is field 1, f = 0 → `2^−126`.)

**Q4.** `25.625 = 11001.101 = 1.1001101 × 2^4` → field = 131 = `10000011`; fraction `f = .1001101` → `1001101` + 16 zeros. Bits: `0 10000011 10011010000000000000000` → `0x41CD0000`. Answer **41CD0000**. (Check: fraction decimal `0.6015625`; `1.6015625 × 16 = 25.625` ✔.)

**Q5.** A ✔, B ✔, C ✔ — the three reserved-pattern rules. D ✘ — denormals drop the implicit 1 (that's exactly why they lose precision, and why `2^−149` is reachable).

**Q6.** `12.375 = 1100.011 = 1.100011 × 2^3`. Fraction bits `f = 100011…0` → `0.100011₂ = 0.5 + 0.03125 + 0.015625 = 0.546875`. Answer **0.546875**. (Full encoding: field 130, `0x41460000`. Note the *significand* is `1.546875` — the question asks for the stored fraction.)

**Q7.** In double precision the fraction has 52 bits; the spacing at 1.0 is `2^−52` → `x = −52`. (Cross-check single: `2^−23`; double adds 29 more fraction bits.)

**Q8.** Largest finite = largest normal: exponent field `1111 1110₂ = 254` (255 is reserved for ∞/NaN), fraction all 1s → value `(2 − 2^−23) × 2^127 ≈ 3.4 × 10^38`. Answer **254**.

**Q9.** In binade `[2^E, 2^(E+1))`, ulp = `2^(E−23)`. Here E = 24 → `2^1 = ` **2**. (This is the famous "2^24 + 1 is not representable; it rounds to 2^24" fact. Contrast: the gap at 1.0 is `2^−23`.)

**Q10.** `−0.5 = −(0.1₂) = −1.0 × 2^−1` → sign bit 1; field = `−1 + 127 = 126 = 0111 1110`; fraction 0. Bits: `1 01111110 00000000000000000000000` = `1011 1111 0000 …` → **BF000000**.

---

## 7. Next steps

1. Encode/decode drill: pick `6.25`, `0.15625`, `−12.375` and do all five steps out loud.
2. Practice reps: `DL_03_Floating_Point_IEEE754.md` (also revises number systems from `DL_01`).
3. **COA queue done** → the video-order remaining subjects: C + Data Structures next (highest scoring-per-hour), then TOC, DBMS, Algorithms, Digital Logic, Compiler Design; Discrete + Engineering Math last.
