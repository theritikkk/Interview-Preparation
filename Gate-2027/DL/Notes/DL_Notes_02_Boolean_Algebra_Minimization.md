# DL Notes 02 — Boolean Algebra, K-Maps & Minimization (High-Yield Deep Dive)

> **Why this file exists:** "Boolean Algebra" is the second video topic for Digital Logic. GATE's questions are crisply checkable: simplify an expression by laws (consensus/absorption/De Morgan), minimize a Σm/πM function on a K-map, identify **prime implicants (PIs)** and **essential prime implicants (EPIs)**, use **don't cares**, count literals/terms, count functions, and count gates (universality, XOR from NANDs). This file gives the complete workflow. Every minimization answer below was **verified with a Quine–McCluskey (tabular-method) engine** run on the exact function.
>
> **Companion question bank:** `../DL_01_Boolean_Algebra_Minimization.md`. PW source: `../Reference/PW_Subject_Handbooks/PW_DigitalLogic.txt` (§2.1–2.10, pages 3.13–3.22: laws, De Morgan, duality, K-maps 2–5 variables, simplification rules, redundant groups, don't care, implicants/PI/EPI).

---

## 0. The mental model: three layers of the same object

```
   truth table  ──►  canonical SOP / POS   ──►  minimal expression (fewest literals/gates)
        │                    │                            │
   2^(2^n) possible     Σm(...) / πM(...)            K-map / tabular method
   functions            1s / 0s rows                 grouping 2^k cells
```

**Why minimize at all (PW §2.4):** fewer gates → smaller circuit, faster (fewer levels), less power, lower fan-in. GATE measures minimality by **literal count** (each literal = one input wire, typically) and **term count**.

---

## 1. The laws you must have at instant recall (PW §2.2 + §2.3)

| Law | Form |
|---|---|
| Commutative | A+B = B+A; AB = BA |
| Associative | (A+B)+C = A+(B+C); (AB)C = A(BC) |
| Distributive | A(B+C) = AB+AC; A+BC = (A+B)(A+C) |
| Identity / Null | A+0 = A; A·1 = A; A+1 = 1; A·0 = 0 |
| Idempotent | A+A = A; A·A = A |
| Absorption | A + AB = A; A(A+B) = A |
| Involution | (A′)′ = A |
| Combining | AB + AB′ = A; (A+B)(A+B′) = A |
| De Morgan | (AB)′ = A′ + B′; (A+B)′ = A′B′ — *break the bar, change the sign* |
| **Consensus** | **AB + A′C + BC = AB + A′C** (BC is the redundant consensus term) |
| Duality | swap (·↔+ , 0↔1) in a *valid identity* → still valid (NOT the same as complementing) |

**XOR facts (used everywhere in adders/parity):** A⊕0 = A; A⊕1 = A′; A⊕A = 0; A⊕A′ = 1; XOR is associative & commutative; a⊕b⊕c = 1 ⇔ an **odd** number of the inputs are 1.

**Worked algebraic simplifications:**
- `A + A′B = A + B` (distribute: A + A′B = (A+A′)(A+B) = 1·(A+B)).
- `AB + A′C + BC = AB + A′C` — consensus (§above).

---

## 2. Canonical forms: minterms and maxterms (PW §2.5)

For n variables, each row of the truth table contributes exactly one minterm (product, 1s-row) and one maxterm (sum, 0s-row). Convention: **uncomplemented variable = 1** in minterms; **0 = uncomplemented** in maxterms.

2-variable demonstration table:

| x y | minterm | maxterm |
|---|---|---|
| 0 0 | m0 = x′y′ | M0 = x+y |
| 0 1 | m1 = x′y | M1 = x+y′ |
| 1 0 | m2 = xy′ | M2 = x′+y |
| 1 1 | m3 = xy | M3 = x′+y′ |

Key conversions (free marks):
- Row i: **Mi = mi′** and **mi = Mi′**. (Complement = flip every literal's bar and AND↔OR.)
- F = Σm(list of 1-rows) ⇔ F = πM(list of 0-rows) — the SAME function. E.g. with 3 variables: F = Σm(0,2,5) = πM(1,3,4,6,7).
- F′ (the complement function) = Σm(the other list). So F = (Σm(missing))′, useful: **F = Σm(S) ⇔ F = πM(S̄)** where S̄ is the complement set.
- Expand any SOP term to minterms: AB = ABC + ABC′ (multiply by (X+X′) for each missing variable).

---

## 3. K-map mechanics (PW §2.6–2.8)

Cell order is **Gray order** (any two adjacent cells differ in exactly one variable) — that is why from File 01 you already know the Gray sequence. 4-variable map (rows AB, columns CD):

```
        CD=00  01  11  10
   AB=00  m0  m1  m3  m2
   AB=01  m4  m5  m7  m6
   AB=11  m12 m13 m15 m14
   AB=10  m8  m9  m11 m10
```

Rules of the game:
1. Group only **1s** (SOP) in **powers of 2** (1, 2, 4, 8, 16): a group of 2ᵏ cells kills k variables — the variables that **change inside the group**. Bigger group ⇒ fewer literals.
2. Wrapping is legal: left↔right column bands, top↔bottom row bands. **The four corner cells form a valid quad** (e.g. m0, m2, m8, m10 = B′D′).
3. Overlap freely; each 1 must be in ≥ 1 group; groups of 3, 5, 6, 7 cells are **invalid**.
4. **Redundant group** (PW §2.8): all its 1s are already covered elsewhere — delete it (it only adds a gate).
5. 5-variable maps (PW §2.6.4): two 4-variable maps side by side (A = 0 / A = 1), adjacency across the mirror plane links them. 32 cells.

**Watch the 4-var example — group order matters:** F(A,B,C,D) = Σm(0,1,3,7,11,15). Map reading: m0,m1 → pair A′B′C′; the quad m3,m7,m11,m15 → **CD** (B and A change, C and D stay 1). m1,m3 → A′B′D exists but is **non-essential** (both its cells are covered by the other two groups), and the minimal cover is simply:

```
   F = A′B′C′ + CD      (2 terms, 6 literals)
        └─ EPI ─┘  └ EPI ┘        A′B′D is a PI but NOT needed
```

---

## 4. PI / EPI workflow (PW §2.10)

- **Implicant:** any product term whose 1-cells are all inside F.
- **Prime implicant (PI):** cannot be enlarged further (maximal group).
- **Essential PI (EPI):** contains at least one minterm that **no other PI** covers.

**The 4-step method that never fails:**

```
   1. List ALL PIs (all maximal groups).
   2. Mark each minterm owned by exactly ONE PI  → those PIs are EPIs. Take them all.
   3. Cover the remaining minterms with the fewest extra PIs
      (tie-break: prefer groups covering more still-uncovered minterms).
   4. Write the OR of chosen terms; count literals/terms.
```

**Don't-care conditions (PW §2.9):** for output patterns that never occur, the cell may be treated as **0 or 1 at will** — use them to *enlarge* groups but you are **never required to cover** them. Each d-cell may be shared among several groups.

**Worked don't-care example:** F(A,B,C,D) = Σm(1,5,9,13) + d(3,7,11,15).
- Without the d's: F = C′D (one pair — m1,m5 differ in B; m1,m9 differ in A; so only pairs, e.g. {1,5},{5,13},{1,9},{9,13}).
- With d's as 1s: the octet {1,3,5,7,9,11,13,15} = **D** becomes available → **F = D** (1 literal!). Verified by tabular method: single PI `---1`.

---

## 5. Counting facts GATE plants as MCQs

- Number of Boolean functions of n variables = **2^(2ⁿ)**: 2 vars → 16; 3 → 256; 4 → **65536**; 5 → 4.29×10⁹.
- Number of functions expressible as *one literal* of n variables = 2n (each variable, complemented or not).
- Number of minterms = number of maxterms = 2n.
- "All 4-variable functions whose minimal SOP has t terms / l literals" — count by mapping, verify by listing (the tabular engine is your friend).
- A function with all cells 1 ⇒ F = 1 (constant); K-map has a 16-group (0 literals).

**Universal gates (§1.2):** NAND alone and NOR alone can realize {NOT, AND, OR} ⇒ **universal**. XOR/XNOR are **not** universal (they can't produce AND alone — no monotone growth).
- NOT from NAND: tie both inputs. NOT from NOR: same trick.
- **2-input XOR needs exactly 4 two-input NAND gates** (or 5 NORs); building it with 3 is impossible.
- **Half adder = 5 two-input NANDs** (share one NAND for A·B): XOR(4) + inverter for carry(1).

---

## 6. Traps checklist

1. **EPIs are never optional** — every minimal SOP contains all of them. Skipping one always ruins minimality.
2. **A PI need not be used** (the A′B′D lesson in §3); a redundant group must be deleted.
3. **Groups of 3 / 5 / 6 don't exist.** Only powers of two (including 1).
4. **Corner quads are legal**; edge wrap is legal; copy cells from the 5-var mirror plane are adjacent.
5. **Consensus kills the *third* term** (BC in AB + A′C + BC) — the answer keeps the two "boundary" terms.
6. **Literal count ≠ term count.** "Minimal" in GATE = fewest literals, then fewest terms.
7. **Minimal SOP ≠ minimal POS of the same function** — sometimes one side is cheaper; group the 0s for POS (maxterm groupings), and the POS literal count can differ.
8. **Don't-care misuse:** you may *use* d-cells in a group only if every 1-cell of that group is real; you may not create groups of all-d cells.
9. **2^(2ⁿ), not 2ⁿ** for function counts (256 vs 8 for n = 3 — classic).
10. **De Morgan + duality confusion:** complementing flips literals; duality doesn't.

---

## 7. Practice set (10 questions, all minimizations verified by tabular method)

**Q1 · MCQ · Fundamental** — The consensus theorem is:
(A) AB + A′C + BC = AB + A′C
(B) A + AB = A
(C) AB + AB′ = A
(D) (A+B)(A+C) = A + BC

**Q2 · NAT · Fundamental** — The expression F = A′BC + A′BC′ + ABC + ABC′ simplifies to a form with the fewest possible number of literals. That number of literals is ______.

**Q3 · NAT · Standard** — For F(A,B,C) = Σm(0,1,4,5,6), the minimal SOP has 2 terms. The total number of literals in it is ______.

**Q4 · MSQ · Standard** — Which statements about K-maps are TRUE?
(A) In a 4-variable map, the four corner cells form a valid quad.
(B) A valid group of 8 adjacent cells corresponds to a product term with exactly 1 literal.
(C) A group of 3 ones is a valid prime implicant.
(D) Larger valid groups always produce terms with fewer literals.

**Q5 · MCQ · Moderate** — The minimal SOP of F(A,B,C,D) = Σm(2,3,4,5,6,7,12,13,14,15) is:
(A) B + A′C  (B) B + C  (C) AB + A′C  (D) B + AC′

**Q6 · NAT · Moderate** — For F(A,B,C,D) = Σm(1,4,6,7,8,10,12,14): let p = number of prime implicants and e = number of essential prime implicants. Then p + e = ______.

**Q7 · MSQ · Moderate** — Which statements about minimization are TRUE?
(A) The minimal SOP and the minimal POS of the same function can have different literal counts.
(B) Every don't-care minterm must be covered by at least one group in every valid minimal SOP.
(C) Minterms not covered by any essential prime implicant must be covered by suitably chosen non-essential PIs.
(D) The consensus theorem can remove a product term that is logically redundant.

**Q8 · NAT · Moderate** — The number of distinct Boolean functions of **4 variables** is ______.

**Q9 · MSQ · Hard** — Which statements are TRUE?
(A) NAND is a universal gate.
(B) XOR is a universal gate.
(C) A 2-input XOR gate requires 4 two-input NAND gates.
(D) NOR is a universal gate.

**Q10 · NAT · Killer** — F1(A,B,C,D) = Σm(4,6,12,14) with no don't cares has a minimal SOP with L1 literals. F2 = the same minterms **plus** d(0,2,8,10) has a minimal SOP with L2 literals. Then L1 − L2 = ______.

---

### Answer key

| Q | Ans | Q | Ans |
|---|---|---|---|
| 1 | A | 6 | 8 |
| 2 | 1 | 7 | A, C, D |
| 3 | 3 | 8 | 65536 |
| 4 | A, B, D | 9 | A, C, D |
| 5 | A | 10 | 1 |

### Full solutions

**Q1 — A.** Consensus theorem: the term combining the two "opposite" literals (BC from AB and A′C) is redundant. (B) absorption, (C) combining, (D) distributive.

**Q2 — 1.** A′BC + A′BC′ = A′B; ABC + ABC′ = AB; A′B + AB = B(A′+A) = **B** — a single literal, so the minimal literal count is 1.

**Q3 — 3.** Map: the quad m0,m1,m4,m5 = **B′**; the pair m4,m6 = **AC′**. F = B′ + AC′ → 1 + 2 = **3 literals** (2 terms). Verified: tabular method finds exactly these 2 PIs, both essential.

**Q4 — A, B, D.** (A) ✔ corners m0,m2,m8,m10 form a legal quad = B′D′ (wraps count as adjacency). (B) ✔ 4 variables − 3 = 1 literal. (C) ✘ group sizes are powers of two only. (D) ✔ a 2ᵏ group drops k variables.

**Q5 — A.** PIs: m2,m3,m6,m7 → **A′C**; the octet m4–m7, m12–m15 → **B** (B must be 1; A,C,D vary). Both are essential (m2 only via A′C; m4 only via B) → F = B + A′C (3 literals). Traps: (B) B + C **covers m10** (1010: C = 1), which is NOT in F — fewer literals but wrong function! (C) AB misses m2,m3. (D) AC′ misses m2,m3.

**Q6 — 8.** Tabular method PIs: **A′B′C′D** (m1 alone), **A′BC** (m6,m7), **BD′** (m4,m6,m12,m14), **AD′** (m8,m10,m12,m14). Each PI owns at least one private minterm (m1, m7, m4, m8 respectively) → all 4 are essential. p + e = 4 + 4 = **8**.

**Q7 — A, C, D.** (A) ✔ POS groups the 0s; counts commonly differ. (B) ✘ don't-care cells may be left as 0s — covering them is never mandatory. (C) ✔ the standard "cover the remainder" step. (D) ✔ e.g. AB + A′C + BC → AB + A′C.

**Q8 — 65536.** A function is defined by its truth table: 2⁴ = 16 independent bits → 2^16 = **65536** functions. (n variables → 2^(2ⁿ).)

**Q9 — A, C, D.** (A) ✔ (D) ✔ both sets realize every gate. (B) ✘ XOR cannot make AND. (C) ✔ classic gate-count: N1 = NAND(A,B); N2 = NAND(A,N1); N3 = NAND(N1,B); N4 = NAND(N2,N3) = A⊕B (verified all 4 input combos).

**Q10 — 1.** F1 (no d's): all four minterms share B = 1, D = 0 → single PI **BD′** → L1 = 2. F2 with d(0,2,8,10) (all have D = 0): the full octet of D = 0 cells → single PI **D′** → L2 = 1. L1 − L2 = **1**. (Lesson: don't cares collapse a 2-literal answer to 1.)

---

## 8. Next steps

1. Re-derive the §3 example and the §4 don't-care example on paper — the EPI-first workflow is the exam skill.
2. Memorize §6's trap list; then attack the `DL_01` bank cold.
3. Then `DL_Notes_03_Combinational_Circuits.md` — where these minimized expressions become real gates (adders, MUX, decoders, parity).
