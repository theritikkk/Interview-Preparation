# DL Notes 03 — Combinational Circuits (High-Yield Deep Dive)

> **Why this file exists:** "Combinational Circuits" is the third video topic for Digital Logic, and it is **block-question heaven**: gate-counts, propagation delays, MUX-based function implementation, decoder-based implementation, adders/subtractors, comparators, encoders/priority encoders, code conversion, parity. GATE reuses ~10 block patterns every year. Every delay/function/count answer below was **verified by simulation** before writing.
>
> **Companion question bank:** `../DL_04_Combinational_Circuit_Design.md`. PW source: `../Reference/PW_Subject_Handbooks/PW_DigitalLogic.txt` (§3.2–3.16, pages 3.23–3.42: adders, subtractors, parallel adder, CLA, comparators, MUX/DEMUX, decoder, encoders, priority encoder, code converters, parity generator).

---

## 0. The mental model: truth table → minimized expression → block

A combinational circuit's output depends **only on present inputs** (no memory). Design = (define truth table) → (K-map minimize, File 02) → (wire gates/blocks). GATE doesn't ask you to *build* much — it asks you to **count** (gates, delays, lines) and **read** (what data goes where). The toolbox:

```
Adder / Subtractor → arithmetic         MUX → select data      Decoder → minterms → OR
Comparator → A ? B                     Encoder → index         Parity → XOR chain
```

---

## 1. Half adder & full adder (PW §3.2)

| Block | Inputs | Sum / Difference | Carry / Borrow |
|---|---|---|---|
| **Half adder (HA)** | A, B | S = A ⊕ B | C = A·B |
| **Full adder (FA)** | A, B, Cin | S = A ⊕ B ⊕ Cin | Cout = AB + Cin(A ⊕ B) |

- **4 possible input patterns for HA; 8 for FA.** S = 1 ⇔ **odd** number of 1s (that's the ⊕ identity from File 02).
- **FA from two HAs + one OR (PW's build):** HA1(A,B) gives S1 = A⊕B and C1 = AB; HA2(S1,Cin) gives S = S1⊕Cin and C2 = S1·Cin; Cout = C1 + C2 = AB + Cin(A⊕B). The OR gate is **required** — an answer choice "two half adders only" is a classic trap.
- Cout formula: first part AB is "both inputs 1"; second part is "exactly one input 1, and the carry-in is 1".

**Delay analysis through a gate-level FA (PW §3.2.2 pattern).** Given XOR = 2 ns, AND = 1.5 ns, OR = 1 ns in the two-HA construction:

```
Sum path :  A → XOR1 (2) → S1 → XOR2 (2) → S        = 4 ns
Cout path:  {AB via AND1 (1.5)}  and  {A → XOR1 (2) → AND2 (1.5)} → OR (1)
         =  max(1.5, 3.5) + 1                     = 4.5 ns
FA delay = max(4, 4.5) = 4.5 ns   (the carry path is critical)
```

---

## 2. n-bit addition, ripple delay & carry look-ahead (PW §3.4–3.5)

**Binary parallel adder (ripple-carry adder, RCA):** n FAs chained — FA0 handles the LSB, its carry-out feeds FA1, and so on. Add/subtract unit: XOR each B bit with a control line `M` and set Cin = M → M=0: A+B; M=1: A + B′ + 1 = A − B (two's complement, File 01).

**Worst-case (ripple) delay:** the carry must ripple through every stage before the top sum settles:

```
t_total = (n − 1)·t_carry + t_sum(carry-in → sum)

Example: n = 4, t_carry = 2 ns (carry-in → carry-out), t_sum = 3 ns (carry-in → sum),
Cin stable at t = 0  →  C1 @2, C2 @4, C3 @6, S3 = 6 + 3 = 9 ns.
```

(If every FA is characterized by one uniform delay t, this collapses to **n·t** — that's the other common exam phrasing.)

**Carry Look-Ahead Adder (CLA)** attacks the ripple itself with generate/propagate:

```
G_i = A_i·B_i   (generate)            P_i = A_i ⊕ B_i   (propagate)
S_i = P_i ⊕ C_i                       C_{i+1} = G_i + P_i·C_i

C1 = G0 + P0C0                                (all 2-level, parallel!)
C2 = G1 + P1G0 + P1P0C0
```

Every carry is computed **directly** from inputs (2 gate levels) instead of waiting for the previous carry — the price is wider AND/OR gates. GATE favorite: "which signal does S3 wait on in an RCA but not in a CLA?" → **the carry chain**.

---

## 3. Subtractors (PW §3.3)

| Block | Inputs | Difference | Borrow |
|---|---|---|---|
| **Half subtractor** | A, B | D = A ⊕ B | Bout = A′B |
| **Full subtractor** | A, B, Bin | D = A ⊕ B ⊕ Bin | Bout = A′B + A′Bin + B·Bin |

- Half-subtractor borrow: you borrow ⇔ A = 0, B = 1 → A′B.
- Full-subtractor borrow logic: borrow ⇔ (A = 0 and B = 1) or (A = 0 and Bin = 1) or (B = 1 and Bin = 1).
- **Subtraction via the adder** is the GATE-favored route: A − B = A + B′ + 1 (invert B, carry-in 1). In the n-bit unit, **carry-out = 1 ⇔ A ≥ B** (no borrow).

**Worked (verified):** 4-bit subtractor computing 3 − 9: 0011 + 0110 (= B′) + 1 = 1010 → as signed 4-bit = **−6**, carry-out = 0 (borrow). Reverse case 9 − 3 = 0110 with carry-out = 1.

---

## 4. Magnitude comparator (PW §3.6)

1-bit comparator (inputs A, B):

```
A > B : Z = A·B′        A < B : X = A′·B        A = B : Y = A⊙B  (XNOR)
```

Why XNOR for equality: both 0 or both 1 → A′B′ + AB. For n bits, compare from the **MSB**: the first (most significant) position where bits differ decides; equality requires all XNORs to be 1 (cascade X and Z similarly).

---

## 5. Multiplexer — the universal function block (PW §3.7–3.9)

2ⁿ data inputs, n select lines, 1 output. Expression (4:1 with selects S1S0):

```
Y = I0·S1′S0′ + I1·S1′S0 + I2·S1S0′ + I3·S1S0      (Y = Σ Ii · mi)
```

**Direct implementation of any n-variable function with a 2ⁿ:1 MUX:** wire the select lines to the n variables and each data input to the function's truth-table value (0/1). A 4:1 MUX can also do a **3-variable** function: select = 2 variables, data inputs = functions of the leftover variable **{0, 1, C, C′}**.

**Worked (verified): F(A,B,C) = Σm(2,3,5), A on S1, B on S0.**

| Row (A B) | C=0 cell | C=1 cell | Data input |
|---|---|---|---|
| 00 | m0 = 0 | m1 = 0 | **I0 = 0** |
| 01 | m2 = 1 | m3 = 1 | **I1 = 1** |
| 10 | m4 = 0 | m5 = 1 | **I2 = C** |
| 11 | m6 = 0 | m7 = 0 | **I3 = 0** |

**Sizing up: building an M:1 MUX from m:1 MUXes.** A tree of m:1 MUXes: total count = **(M − 1)/(m − 1)**. Special case m = 2: **2ⁿ:1 from 2:1 needs 2ⁿ − 1** (32:1 → 16 + 8 + 4 + 2 + 1 = **31**; 16:1 from 4:1 → 4 + 1 = **5**). Trap: "2^(N−n)" is a wrong form of this counting and shows up in fake solutions.

**MUX applications (PW §3.9):** data selection, function realization, parallel-to-serial conversion, and as a lookup/LUT block.

---

## 6. Demux, decoder, encoder, priority encoder (PW §3.10–3.14)

- **Demultiplexer:** 1 data input, n selects, **2ⁿ outputs** (opposite of MUX). Same hardware, swapped role.
- **Decoder (2→4 example):** outputs are exactly the minterms: Y0 = A′B′, Y1 = A′B, Y2 = AB′, Y3 = AB.
- **Function realization with a decoder:** wire the function's minterm outputs into an OR gate (active-high outputs); with **active-low** outputs use a **NAND** instead (a NAND is an OR on inverted inputs — De Morgan).
  - Worked: F(A,B,C,D) = Σm(0,1,7,8,15) on a **4→16** decoder → one OR/NAND gate with **5 connections**; the other **11** outputs are unconnected.
- **Encoder = inverse of decoder;** cautions: only **one** input may be active at a time, and the all-zero input is ambiguous (that's why priority encoders add a valid bit).
- **Octal (8→3) encoder expressions (PW §3.13):** A2 = D4+D5+D6+D7, A1 = D2+D3+D6+D7, A0 = D1+D3+D5+D7.
- **Decimal → BCD encoder:** A = D8+D9, B = D4+D5+D6+D7, C = D2+D3+D6+D7, D = D1+D3+D5+D7+D9.
- **Priority encoder (4-input, PW §3.14):** if several inputs are active, the **highest-index** input wins; outputs for lower-priority active inputs are don't-care (X). Worked (verified): D3 = 0, D2 = 1, D1 = 1, D0 = 1 → the winner is **D2 → output 10**.

| D3 | D2 | D1 | D0 | A | B | V (valid) |
|---|---|---|---|---|---|---|
| 0 | 0 | 0 | 0 | X | X | 0 |
| 1 | X | X | X | 1 | 1 | 1 |
| 0 | 1 | X | X | 1 | 0 | 1 |
| 0 | 0 | 1 | X | 0 | 1 | 1 |
| 0 | 0 | 0 | 1 | 0 | 0 | 1 |

---

## 7. Code converters & parity (PW §3.15–3.16)

**Binary → BCD converter (4-bit binary 0–15 → 5-bit BCD).** Verified output expressions (B4 = tens bit; B3..B0 = units nibble):

```
B4 = AB + AC = A(B + C)          (10…15)
B3 = A·B′·C′                     (exactly 8, 9)
B2 = A′B + BC = B(A′ + C)        (4…7 and 14, 15)
B1 = A′C + AB·C′                 (2,3,6,7 and 12,13)
B0 = D                           (odd inputs ↔ odd units digit)
```

(Vanity check in your head: 9 → 0 1 0 0 1 ✓; 15 → 1 0 1 0 1 ✓.)

**BCD ↔ excess-3:** add 3 (0011) digit-by-digit BCD → XS-3; subtract 3 back. 9 → 1100. **XS-3 is self-complementing** (File 01 §3).

**Parity generator (PW §3.16):**
- Even parity bit Ps = **XOR of the data bits** (chain: P = A⊕B⊕C⊕D). Total 1s (data + parity) become even.
- Odd parity bit = complement of the even one — a chain of XORs plus a final inversion (or XNOR combination, as PW shows).
- **Detection:** recompute at the receiver; a mismatch flags **any odd number of bit errors**. **Limitation:** an even number of errors (e.g. 2 bits) is invisible. Parity detects, never corrects. (Verified examples: data 1011 → Ps = 1; 1101 → Ps = 1; 1111 → Ps = 0.)

---

## 8. Traps checklist

1. **FA needs two HAs *plus OR*;** half subtractor ≠ half adder with swapped inputs.
2. **Ripple delay counts (n−1) carries + final sum** — not n carries + sum, not (n−1)·t if one uniform delay is specified (then n·t).
3. **Carry path is usually the critical path** (Cout 4.5 ns > Sum 4 ns in the §1 example).
4. **MUX data inputs can be variables**, not just 0/1 (4:1 with 3-var function). Read the select assignment order (S1S0 vs S0S1!) — Σm-table rows depend on it.
5. **Decoder + active-low outputs → NAND, not OR.** Unconnected-lines questions: 2ⁿ − (#minterms).
6. **Priority encoder output = index of the highest set bit** (verified: 0,1,1,1 → 10).
7. **Encoder vs priority encoder:** plain encoder requires one-hot input.
8. **Subtractor carry-out meaning is inverted vs adder:** in the add/sub unit Cout = 1 ⇔ A ≥ B.
9. **Parity is XOR-chain for even, inverted for odd; catches odd # of errors only.**
10. **CLA removes the carry chain, not the gate fan-in cost.**

---

## 9. Practice set (10 questions, all values verified by simulation)

**Q1 · MCQ · Fundamental** — Which of the following is NOT a combinational circuit?
(A) Half adder  (B) 4:1 multiplexer  (C) JK flip-flop  (D) 2-to-4 decoder

**Q2 · NAT · Fundamental** — The minimum number of 2-input NAND gates required to realize a half adder is ______.

**Q3 · NAT · Standard** — In a 4-bit ripple-carry adder, each full adder presents: carry-out available **2 ns** after its carry-in, and sum available **3 ns** after its carry-in. All A/B inputs and the LSB carry-in (0) are stable at t = 0. The arrival time (ns) of the final sum S3 is ______.

**Q4 · MSQ · Standard** — Which statements about multiplexers are TRUE?
(A) A 4:1 MUX with data inputs I0 = I3 = 0 and I1 = I2 = 1 implements the XOR of its two select lines.
(B) A 4:1 MUX can realize any 4-variable Boolean function using two variables on the select lines and functions of the remaining two variables on the data inputs.
(C) A 2:1 MUX cannot implement the XOR function.
(D) Implementing a function with the canonical (unminimized) expression on a MUX requires no algebraic simplification.

**Q5 · MCQ · Moderate** — A 4:1 MUX realizes F(A,B,C) = Σm(2,3,5) with A on S1 and B on S0. The data input I2 must be:
(A) 0  (B) 1  (C) C  (D) C′

**Q6 · NAT · Moderate** — In a 4-input priority encoder (D3 highest priority), the inputs are D3 = 0, D2 = 1, D1 = 1, D0 = 1. The 2-bit output (as a binary string, e.g. write 10 for two) is ______.

**Q7 · MSQ · Moderate** — Which statements are TRUE?
(A) For even parity, the parity bit for n data bits is the XOR of the data bits.
(B) The equality output of a 1-bit magnitude comparator is A⊙B (XNOR).
(C) A demultiplexer with n select lines has one data input and 2ⁿ outputs.
(D) A 2-input XOR gate can be implemented with exactly 3 two-input NAND gates.

**Q8 · NAT · Moderate** — The 4-bit binary number 1001 (decimal 9) is converted to excess-3 code. The 4-bit excess-3 output, read as an unsigned binary value, equals ______.

**Q9 · MSQ · Hard** — Which statements are TRUE?
(A) The difference output of a half subtractor has the same form as the sum output of a half adder (both A ⊕ B).
(B) Full adder carry: Cout = AB + Cin(A ⊕ B).
(C) Half subtractor borrow: Bout = A′B.
(D) Half adder carry: C = A + B.

**Q10 · NAT · Killer** — A 4-bit two's complement subtractor computes 0011 − 1001 as A + B′ + 1 (LSB carry-in = 1). The resulting 4-bit difference 1010, interpreted as a signed value, equals ______.

---

### Answer key

| Q | Ans | Q | Ans |
|---|---|---|---|
| 1 | C | 6 | 10 |
| 2 | 5 | 7 | A, B, C |
| 3 | 9 | 8 | 12 |
| 4 | A, B, D | 9 | A, B, C |
| 5 | C | 10 | −6 |

### Full solutions

**Q1 — C.** A JK flip-flop's next output depends on its inputs *and* on its stored state — it has memory, so it is sequential, not combinational. Half adder, MUX and decoder are all memoryless: output = pure function of the present inputs.

**Q2 — 5.** XOR needs exactly 4 NANDs: N1 = (AB)′, N2 = (A·N1)′, N3 = (B·N1)′, N4 = (N2·N3)′ = A⊕B (verified on all 4 input pairs). Carry = AB = invert N1 → 1 more NAND (N1 into both inputs) ⇒ 4 + 1 = **5**. Three gates is provably impossible for XOR.

**Q3 — 9.** Carry chain: C1 arrives after 2 ns (FA0), C2 after 4, C3 after 6; S3 needs 3 ns after C3 ⇒ 6 + 3 = **9 ns** = (n−1)·t_carry + t_sum = 3·2 + 3.

**Q4 — A, B, D.** (A) ✔ Y = I0·S1′S0′ + I1·S1′S0 + I2·S1S0′ + I3·S1S0 = S1′S0 + S1S0′ = S1⊕S0 (verified on all four rows). (B) ✔ select = 2 variables; each data input is a 2-variable function (0/1/W/W′/W⊕V…). (C) ✘ S = A, I0 = B, I1 = B′ implements Y = A′B + AB′ = A⊕B. (D) ✔ canonical form wires directly (that's the appeal of MUX implementation).

**Q5 — C.** With A = S1, B = S0, row 10 covers m4 (C = 0) and m5 (C = 1). m4 ∉ F → 0; m5 ∈ F → 1. So I2 = 0·C′ + 1·C = **C**. (Check siblings: I0 = 0, I1 = 1, I3 = 0.)

**Q6 — 10.** Highest set input is D2 → output is the binary index 2 = **10**. (D0, D1 being 1 too is irrelevant — priority.)

**Q7 — A, B, C.** (A) ✔ XOR chain. (B) ✔ equal ⇔ A′B′ + AB = XNOR. (C) ✔ definition. (D) ✘ the minimum is 4 NANDs (as verified in File 02/Q9 and Q2 above) — 3 cannot produce XOR.

**Q8 — 12.** XS-3(9) = 9 + 3 = 12 = 1100₂; as unsigned binary that is **12**.

**Q9 — A, B, C.** (A) ✔ in a half subtractor, 0−1 leaves difference 1 and borrows — exactly the XOR pattern. (C) ✔ standard expressions from §1/§3. (B) ✔ §1. (D) ✘ carry = AB (AND), not OR — the + sign confusion is the intended catch.

**Q10 — −6.** Invert B: 1001 → 0110; add with carry-in 1: 0011 + 0110 + 1 = 1010. Signed 4-bit: MSB = 1 → negative; magnitude = 2's complement of 1010 = 0110 = 6 ⇒ **−6** (= 3 − 9 ✓). Carry-out = 0 correctly signals a borrow.

---

## 10. Next steps

1. Redraw the §1 delay diagram and §5 MUX table from memory — those two mechanics cover most "design" questions.
2. Memorize the §8 trap list, then run the `DL_04` bank cold.
3. **Digital Logic is now complete (Files 01–03).** Next in the video order: **Compiler Design** (lexical analysis → parsing → SDT/code generation; banks `CD_02`–`CD_04`; source `PW_CompilerDesign.txt`).
