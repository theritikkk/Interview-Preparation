# DBMS Notes 02 — Functional Dependencies & Normalization (High-Yield Deep Dive)

> **Why this file exists:** "Normalization & FD" is on the high-yield list and is the **cheapest DBMS marks** — the question types are so fixed you can almost pattern-match them: **closure size, candidate-key counting, superkey counting, minimal cover, highest normal form, lossless-join test, dependency preservation**. This file gives you the algorithms, the reason they work, and the traps. Every practice answer was brute-force verified by enumerating all attribute subsets.
>
> **Companion question bank:** `DB_07_FDs_and_Normal_Forms.md`. PW source: `../Reference/PW_Subject_Handbooks/PW_DBMS.txt` (§1.6–§1.9, pages 10.4–10.9).

---

## 0. The mental model: constraints that repeat facts

A **functional dependency** `X → Y` is a promise about every *legal* relation instance: whenever two tuples agree on the attributes of `X`, they **must** agree on `Y`.

```
   X → Y   ⇔   for all tuples t1, t2:  t1.X = t2.X  ⇒  t1.Y = t2.Y
```

- A **key** is the ultimate FD: it functionally determines *everything*.
- **Redundancy** arises when a non-key FD forces the same fact to be stored many times → update/insert/delete **anomalies**.
- Normalization = decompose the schema until only "safe" FDs remain — while not losing information (**lossless join**) and (ideally) not losing constraints (**dependency preservation**).

```
   REGULARITY LADDER:   1NF  →  2NF  →  3NF  →  BCNF
                        atoms   no partial   no transitive,   every determinant
                        only    dependencies   primes allowed   is a superkey
```

---

## 1. FD toolkit (PW §1.6)

**Types:**
- **Trivial:** `X → Y` with `Y ⊆ X` (e.g., `AB → A`). Always holds; adds no constraint.
- **Non-trivial:** `X ∩ Y = ∅` (e.g., `A → B`).
- **Semi-trivial:** partially overlapping (`A → AB` ≡ `A → A` + `A → B`); the non-overlapping part is what matters.

**Armstrong's axioms (sound and complete — the only primitive rules needed):**

```
   Reflexivity   :  Y ⊆ X             ⇒  X → Y
   Augmentation  :  X → Y             ⇒  XZ → YZ       (Z any set of attrs)
   Transitivity  :  X → Y,  Y → Z     ⇒  X → Z
```

**Derived rules** (PW §1.6.4 — useful shortcuts, all provable from the three):

```
   Union (additivity)      :  X → Y, X → Z        ⇒  X → YZ
   Decomposition (splitting): X → YZ              ⇒  X → Y, X → Z
   Pseudo-transitivity     :  X → Y, YW → Z       ⇒  XW → Z   ← GATE favourite
   Composition             :  X → Y, Z → W        ⇒  XZ → YW
```

> **Splitting is only valid on the RIGHT.** `AB → C` does **not** give `A → C` or `B → C`; `A → BC` **does** give `A → B`, `A → C`.

---

## 2. Attribute closure X⁺ — one algorithm to rule them all (PW §1.6.3)

```
   X⁺ := X
   repeat:
      for each FD  U → V  in F:
          if U ⊆ X⁺  then  X⁺ := X⁺ ∪ V
   until no change
```

**Everything reduces to closure:**

| Task | Method |
|---|---|
| Is `X → Y` implied by F? (membership) | compute `X⁺`; check `Y ⊆ X⁺` |
| Is `X` a superkey? | check `X⁺ = R` (all attributes) |
| Is `X` a candidate key? | `X⁺ = R` **and** no proper subset of X is a superkey |
| Do F and G force the same constraints? | every FD of F ∈ closure of G **and** vice versa |

**Worked closure.** `R(A,B,C,D,E)`, `F = {A→B, B→DE, C→A}`, compute `{C}⁺`:

```
   {C}  →(C→A)  {C,A}  →(A→B)  {C,A,B}  →(B→DE)  {C,A,B,D,E}     ⇒  |{C}⁺| = 5
```

---

## 3. Candidate keys: search systematically, don't guess (PW §1.6.5)

**Definitions.** Superkey: any set that determines all attributes. Candidate key (CK): a *minimal* superkey. Prime attribute: appears in **some** CK; non-prime: appears in none.

**Pre-analysis that cuts the search (use this in every problem):**

```
   • attributes that never appear on ANY right-hand side (of the FDs)
        ⇒ MUST be in every candidate key
   • attributes that appear ONLY on right-hand sides
        ⇒ can be in NO candidate key (unless they're also "never-RHS"; they can tag along only if key already complete)
   • start closing the "must" set; while X⁺ ≠ R, add candidate attributes one at a time,
     and keep only minimal completions
```

**Worked key hunt.** `R(A,B,C,D)`, `F = {AB→C, C→D, D→B}`:

```
   RHS of F = {C, D, B}        LHS-only attr: A        ⇒ A is in every key
   A⁺  = {A}                ✗  (A can't create anything alone)
   AB⁺ = {A,B,C,D}          ✔  AB is a key
   AC⁺ = {A,C,D,B}          ✔  AC is a key
   AD⁺ = {A,D,B,C}          ✔  AD is a key   (AD→B, then AB→C, then C→D)
   ⇒ CKs = {AB, AC, AD};  every attribute is prime
```

**Counting superkeys (when asked "how many superkeys?"):** every superset of a CK is a superkey — but a superset of *two* CKs gets double-counted. Use inclusion–exclusion:

```
   #superkeys  =  Σ over non-empty subsets T of CKs  (−1)^(|T|+1) · 2^(n − |∪_{k∈T} k|)
```

**Example.** `R(A,B,C)`, `F = {A→B, B→A}` → CKs `AC`, `BC`:

```
   supersets of AC: 2^(3−2) = 2      (AC, ABC)
   supersets of BC: 2                (BC, ABC)
   intersection:    ABC counted twice ⇒ #superkeys = 2 + 2 − 1 = 3
```

(When there is a single CK of size k: `#superkeys = 2^(n−k)`.)

---

## 4. Minimal (canonical) cover (PW §1.7.2)

A **minimal cover** `F_m` of F is a simplest FD set logically equal to F:

```
   Step 1 — SPLIT right sides        A → BC   ⇒   A → B,  A → C
   Step 2 — REMOVE extraneous LHS attributes:
            for X → Y and attribute a ∈ X:
               if Y ⊆ (X − {a})⁺  (computed with the CURRENT set, X→Y included)
               then drop a from the LHS
   Step 3 — REMOVE redundant FDs:
            for each X → Y: if Y ⊆ X⁺ computed over (current set − {X→Y}), delete X→Y
```

**Worked example.** `F = {A→BC, B→AC, C→AB}`:

```
   split        : A→B, A→C, B→A, B→C, C→A, C→B
   drop A→C     : {A→B, B→A, B→C, C→A, C→B}  — A⁺ = {A,B,C} ✔ delete
   drop C→B     : …C→A→B ✔ delete
   drop B→A     : B→C→A ✔ delete       ⇒  F_m = {A→B, B→C, C→A}   (3 FDs)
```

Two facts GATE tests:
- **Extraneous LHS example:** `{WXY→Z, W→X} ≡ {WY→Z, W→X}` — X is extraneous in `WXY` because `W → X`.
- Minimal covers **need not be unique** (and even the FD count can differ between covers); all minimal covers are logically equivalent. The *size* of a smallest minimal cover is a well-defined problem → the practice questions ask this.

---

## 5. Where redundancy comes from (PW §1.8.2, IP 1)

> **X → Y causes redundancy ⇔ the FD is non-trivial AND X is not a superkey.**

Why: if X is a superkey, at most one tuple carries any X-value → nothing repeats. If Y ⊆ X (trivial), no constraint. Every redundancy/anomaly in a schema is traceable to a non-trivial FD with a non-superkey determinant.

**Anomaly types (PW §1.8):** insertion anomaly (can't insert a fact without an unrelated fact), deletion anomaly (lose unrelated facts when deleting a row), update anomaly (must update a repeated fact in many rows), join anomaly (lossy decomposition).

---

## 6. Decomposition quality I: lossless join (PW §1.8.1)

Decomposing R into `R1, R2, …, Rn` is **lossless** if `R1 ⋈ R2 ⋈ … ⋈ Rn = R`. (In general `⋈` of projections can only *add* spurious tuples — the danger is producing extra rows, i.e. `⊇ R` holds trivially.)

**Binary shortcut (only for R1, R2):** the decomposition is lossless **iff** at least one holds:

```
   R1 ∩ R2 → R1        or        R1 ∩ R2 → R2
```

i.e. the common attributes contain a superkey of at least one fragment.

**General (n-way): the CHASE / tableau test.**

```
   1. Build a matrix: row i = fragment Ri, column A ∈ R.
      cell(i,A) = a_A if A ∈ Ri, else b_(i,A)   (distinct marks)
   2. For every FD X → Y in F:
        if two rows agree on ALL X-columns, make their Y-values agree
        (if either is an 'a', copy it; else force equal b's)
   3. Repeat till fixed point. LOSSLESS ⇔ some row becomes all-a.
```

**Worked chase (lossy case).** `R(A,B,C,D)`, `F = {AB→CD, C→B, D→A}`, fragments `R1(AC)`, `R2(BCD)`:

```
          A   B   C   D
   R1 :   a   b1  a   b2
   R2 :   b3  a   a   a        ← R1∩R2 = {C}: unify C-column (already 'a' in both)
   apply C→B : rows agree on C ⇒ unify B:  b1 := a
   apply D→A : D values a vs b2 differ ⇒ no-op
   no row is all-a  ⇒  LOSSY
```

(The binary shortcut confirms it: `{C}⁺ = {C,B}` neither ⊇ AC nor ⊇ BCD.)

---

## 7. Decomposition quality II: dependency preservation (PW §1.8.3)

For fragments `R1..Rn` let `Fi = π_{Ri}(F⁺)` = all FDs of F⁺ whose attributes all live inside Ri. Then

```
   decomposition is DEPENDENCY-PRESERVING  ⇔  (F1 ∪ F2 ∪ … ∪ Fn) ≡ F
```

If even one FD of F cannot be re-derived from the fragments' FDs, **every update checking that FD requires a join** — that's the practical cost.

**Practical check for an exam solution:** prove *non*-preservation by closure: find an FD `X → Y ∈ F` such that `X⁺` computed from `F1 ∪ … ∪ Fn` misses `Y`. Prove preservation by deriving each `X → Y ∈ F` using only the union.

**Worked example.** Same schema as §6: `R(A,B,C,D)`, `F = {AB→CD, C→B, D→A}`, fragments `R1(A,C)`, `R2(B,C,D)`:

```
   F1 = π_AC(F⁺)  = { }                       (no non-trivial FD inside AC)
   F2 = π_BCD(F⁺) = { C→B,  CD→B }
   union = { C→B, CD→B }
   {A,B}⁺ over the union = {A, B}   — cannot reach C or D  ⇒  AB→CD NOT preserved  ⇒  NOT DP
```

(So this decomposition is *lossy and* not dependency preserving — two independent defects; GATE often asks you to check them separately.)

---

## 8. The normal forms, one table (PW §1.9)

| NF | Condition on every **non-trivial** FD `X → A` | Intuition | Removes |
|---|---|---|---|
| **1NF** | every attribute value is atomic | no lists/relations inside a cell | multivalued attributes |
| **2NF** | 1NF + no **partial dependency**: no *proper subset of a candidate key* determines a **non-prime** attribute | no leftover of "half a key" | partial dependencies |
| **3NF** | `X` is a superkey **OR** `A` is prime | determinant is a key, or the dependent is a key-part | transitive dependencies of non-primes |
| **BCNF** | `X` is a superkey (always) | every determinant is a full key | all FD-caused redundancy |

**The PW important points (memorize these five):**

1. If `X → A` is non-trivial and X is **not** a superkey ⇒ redundancy exists (regardless of NF label of other FDs).
2. **3NF but not BCNF** ⇔ there is an FD `(proper subset of a CK) → (some other CK's attribute)` — with prime attributes on both sides. *Example:* `R(A,B,C)`, `{AB→C, C→B}`: CKs `AB, AC`; `C→B` has non-superkey determinant but prime dependent → 3NF ✔, BCNF ✘.
3. **All candidate keys simple (single attribute)** ⇒ relation is automatically **2NF** (no proper non-empty subset of a 1-attribute key exists).
4. **Every attribute prime** ⇒ relation is automatically **3NF** (any FD's RHS is prime). BCNF may still fail.
5. **No non-trivial FDs at all** ⇒ BCNF automatically.

**Decomposability guarantees (PW §1.9.5):**

```
   ⇒ 1NF..BCNF:  lossless-join decomposition ALWAYS possible
   ⇒ 1NF..3NF :  lossless + DEPENDENCY-PRESERVING always possible   (3NF synthesis)
   ⇒ BCNF     :  lossless always; dependency preservation MAY be impossible
```

**Algorithms worth quoting:**
- **3NF synthesis:** compute a minimal cover; one relation per distinct LHS (carrying its LHS + all RHS attributes); if no relation contains a CK, add a relation consisting of a CK. Always lossless + DP.
- **BCNF decomposition:** while some relation Ri has a violating FD `X → A` (X not a superkey of Ri): split Ri into `(X ∪ {A})` and `(Ri − {A})`. Lossless at each step.

---

## 9. Derived mini-facts worth internalizing

```
   • X → Y is non-trivial ⇒ X⁺ grows by at least |Y − X| attributes
   • AB⁺ ⊇ A⁺ and AB⁺ ⊇ B⁺;  closure is monotone in the attribute set
   • if A is a CK then every superset of A is a superkey — massively reduces counting
   • deciding "X is a CK" is polynomial; deciding "there exists a BCNF DP decomposition" is NP-hard
     (not asked in GATE — only "is THIS decomposition DP" is asked, which the projection test settles)
   • an FD with a single-attribute LHS whose RHS is a key ⇒ that LHS is a key too
     (if B → A and A is a superkey, then B⁺ ⊇ A⁺ = R)
```

---

## 10. Trap sheet

| # | Trap | Fact |
|---|---|---|
| 1 | Splitting LHS | `AB→C` never implies `A→C` |
| 2 | Closure input | closure is computed of an attribute *set*; `{A,B}⁺ ⊇ {A}⁺ ∪ extra` |
| 3 | CK vs superkey | CK must be minimal — the full attribute set is always a (non-minimal) superkey |
| 4 | Prime attribute definition | appears in **some** CK, not "in the chosen CK" |
| 5 | 2NF test | only **non-prime** RHS matters — `part-of-key → prime` is fine |
| 6 | 3NF test | determinant superkey OR RHS prime — two independent escape hatches |
| 7 | Lossless binary test | need `R1∩R2 → R1` **or** `→ R2` (one side suffices); use chase for n ≥ 3 |
| 8 | Lossless ≠ DP | they are independent properties; check separately |
| 9 | Minimal cover | LHS-attribute removal (step 2) must use the current set — order can matter for *which* cover you get |
| 10 | Superkey counting | inclusion–exclusion over CK supersets; don't double count ABC-type overlaps |

---

# Practice set — DBMS Notes 02

*All key counts and covers below were brute-force verified (every attribute subset enumerated).*

### Q1 · NAT · Fundamental

`R(A, B, C, D, E)`, `F = {A → B, B → DE, C → A}`. The number of attributes in `{C}⁺` is ______.

### Q2 · NAT · Standard

`R(A, B, C, D)`, `F = {AB → C, C → D, D → B}`. The number of candidate keys of R is ______.

### Q3 · MCQ · Standard

`R(A, B, C, D)`, `F = {AB → C, C → D, D → A}`. The highest normal form that R satisfies is

- (A) 1NF
- (B) 2NF
- (C) 3NF
- (D) BCNF

### Q4 · NAT · Standard

`R(A, B, C)`, `F = {A → B, B → A}`. The number of superkeys of R is ______.

### Q5 · MSQ · Moderate

`R(A, B, C, D)` with `F = {AB → CD, C → B, D → A}` is decomposed into `R1(A, C)` and `R2(B, C, D)`. Which of the following are TRUE?

- (A) The decomposition is lossless-join
- (B) The decomposition is lossy (producing spurious tuples is possible)
- (C) The decomposition is dependency-preserving
- (D) The FD `AB → CD` is not preserved by this decomposition

### Q6 · NAT · Moderate

`F = {A → BC, B → AC, C → AB}`. The minimum possible number of FDs in a minimal (canonical) cover of F is ______.

### Q7 · MSQ · Moderate

`F = {AB → C, C → D, BD → E}` holds on `R(A, B, C, D, E)`. Which of the following FDs are implied by F?

- (A) `AB → D`
- (B) `AB → E`
- (C) `A → C`
- (D) `AD → E`

### Q8 · MCQ · Standard

`R(A, B, C, D)`, `F = {AB → C, A → D}`. The highest normal form that R satisfies is

- (A) 1NF
- (B) 2NF
- (C) 3NF
- (D) BCNF

### Q9 · MSQ · Moderate

Which of the following statements are TRUE?

- (A) Every relation in BCNF is also in 3NF
- (B) 2NF eliminates transitive dependencies
- (C) Every relation in 3NF is also in BCNF
- (D) If every attribute of a relation is prime, the relation is in 3NF

### Q10 · NAT · Killer

`R(A, B, C, D, E)` with `F = {A → BC, CD → E, B → D, E → A}`. The number of candidate keys of R is ______.

---

## Answer key (quick)

| Q | Ans | Q | Ans |
|---|---|---|---|
| 1 | **5** | 6 | **3** |
| 2 | **3** | 7 | **A, B** |
| 3 | **C (3NF)** | 8 | **A (1NF)** |
| 4 | **3** | 9 | **A, D** |
| 5 | **B, D** | 10 | **4** |

---

## Detailed solutions

**Q1.** `{C}⁺`: C → A (C→A), A → B, B → D and B → E ⇒ `{C, A, B, D, E}` → **5 attributes**.

**Q2.** RHS set = {C, D, B}; attribute `A` never appears on a RHS ⇒ in every key. `A⁺ = {A}`; add B → `AB⁺ = ABCD` ✔; add C → `AC⁺ = ACDB` ✔ (C→D, D→B); add D → `AD⁺ = ADBC` ✔ (D→B, AB→C). Minimal in each case ⇒ CKs `{AB, AC, AD}` → **3**.

**Q3.** Brute force gives CKs `{AB, BC, BD}` — so **every** attribute is prime. Check the FDs:
- `AB → C`: AB is a superkey ✔ (satisfies even BCNF).
- `C → D`: `C⁺ = {C, D, A}` ≠ ABCD ⇒ C is **not** a superkey; but D is prime ⇒ 3NF condition holds, BCNF fails.
- `D → A`: `D⁺ = {D, A}` not a superkey; A prime ⇒ 3NF holds.

So R is in 3NF but not BCNF → highest is **3NF (C)**.

**Q4.** CKs: `AC` and `BC` (`AC⁺ = ABC` via A→B; `BC⁺ = ABC` via B→A; `C` alone determines nothing). Superkeys = supersets of AC ∪ supersets of BC = `{AC, ABC} ∪ {BC, ABC}` → **3** (equivalently `2+2−1` by inclusion–exclusion).

**Q5.** 
- Common attributes: `R1 ∩ R2 = {C}`. `C⁺ = {C, B}` — not ⊇ AC (misses A), not ⊇ BCD (misses D) ⇒ binary test fails ⇒ **lossy** (B true, A false). The chase row also never becomes all-`a`.
- Projected FDs: `F1 = ∅` on AC; `F2 = {C→B, CD→B}` on BCD. Over `F1 ∪ F2`, `{A,B}⁺ = {A,B}` ⇒ C and D unreachable ⇒ `AB → CD` is **not** preserved (D true, C false).

**Q6.** Split: `A→B, A→C, B→A, B→C, C→A, C→B`. Dropping redundant FDs greedily (e.g., keep `A→B, B→C, C→A`) leaves 3. Brute force over all subsets confirms **no 2-FD cover exists**: implying the "cycle" A→B→C→A needs all three edges (any 2 FDs leave one edge underivable). Answer **3**. (Minimal covers are not unique: `{A→B, B→C, C→A}`, `{A→B, B→A, B→C}`, … all size 3.)

**Q7.** `{A,B}⁺ = {A, B, C, D} ∪ (BD→E ⇒ E) = {A,B,C,D,E}`:
- (A) `AB → D` ✔ (C→D applied after AB→C).
- (B) `AB → E` ✔ (once D is derivable with B, `BD → E` fires).
- (C) `{A}⁺ = {A}` ⇒ `A → C` ✘.
- (D) `{A,D}⁺ = {A, D}` — no FD applies (BD→E needs B) ⇒ `AD → E` ✘.

**Q8.** Attributes on RHS: `{C, D}` ⇒ B is in every key; `B⁺ = {B}`. `AB⁺ = ABCD` ⇒ CK = `{AB}` (single key). `A → D` is a **partial dependency** (A is a proper subset of the key AB; D is non-prime) ⇒ 2NF violated. R is atomic (1NF) with no other structure left ⇒ highest = **1NF (A)**.

**Q9.** 
- (A) ✔ — BCNF's condition (every determinant a superkey) implies 3NF's disjunction.
- (B) ✘ — 2NF removes *partial* dependencies; transitive dependencies are 3NF's job.
- (C) ✘ — `R(A,B,C)` with `{AB→C, C→B}` is 3NF (C's RHS is prime) but not BCNF.
- (D) ✔ — PW Important Point 6: all-prime ⇒ 3NF (any FD has a prime RHS).

**Q10.** Systematic search: keys must include only attributes needed to power the FDs; brute force over all 2⁵ subsets gives CKs `{A, E, BC, CD}`:
```
   A⁺ = A,BC,(B→D)D,(CD→E)E  = all ✔
   E⁺ = E,A,BC,D             = all ✔    (E→A → A→BC → B→D)
   BC⁺ = B,C,D,(CD→E)E,(E→A)A = all ✔
   CD⁺ = C,D,E,(E→A)A,(A→BC)B = all ✔
   single B, C, D fail; BCD, ACD, … are supersets of smaller keys ⇒ not minimal
```
⇒ **4 candidate keys**. (Every attribute is prime here too — a good self-check for "3NF but maybe not BCNF" style questions.)

---

*Cross-practice: `DB_07` (FD closure, key counting, normal forms, 3NF synthesis). Next file: file organization, indexing & B/B+ trees.*
