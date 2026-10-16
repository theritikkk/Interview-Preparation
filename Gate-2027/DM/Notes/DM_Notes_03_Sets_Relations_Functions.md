# DM Notes 03 — Sets, Relations & Functions (High-Yield Deep Dive)

> **Why this file exists:** "Sets & Relations" is the third video topic of Discrete Mathematics; with it come the two question banks' favourite factories — **counting relations** (anti/symmetric, transitive, equivalence, partial orders) and **poset/lattice classification**. Every count below was **verified by exhaustive enumeration** (all relations on small sets, all subsets, all triples) before writing.
>
> **Companion question banks:** `../DM_03_Sets_Relations_Functions.md`, `../DM_04_Partial_Orders_Lattices.md`. **PW source:** `../Reference/PW_Subject_Handbooks/PW_DiscreteMath.txt` §3.1–§3.6 (pages 2.18–2.34: sets, relations & their counts, closures, posets, lattices, functions).

---

## 0. What GATE actually asks

```
(A) Set algebra             identities, symmetric difference, region counting
(B) Inclusion–exclusion     |A∪B∪C| with overlaps; cardinalities
(C) Relation counting       reflexive/symmetric/antisymmetric/transitive
(D) Equivalence relations   ↔ partitions; Bell/Stirling numbers
(E) Closures                reflexive/symmetric/transitive closure of R
(F) Functions               counts (mⁿ, P(m,n), n!, onto formula); composition
(G) Posets                  Hasse diagrams, extremal elements, chains/antichains
(H) Lattices                distributive/complemented/Boolean; sublattice traps
(I) Countability            ℤ, ℚ, ℕ×ℕ countable; 2^ℕ, ℝ uncountable
```

---

## 1. Sets: power set, operations, laws (PW §3.1)

- |A| = n ⇒ **|P(A)| = 2ⁿ** (PW §3.1.4); A has C(n,k) subsets of size k; the number of **even-size subsets equals the number of odd-size ones = 2ⁿ⁻¹** (for n ≥ 1).
- Operations: ∪, ∩, complement, difference (A ∖ B = A ∩ B̄), **symmetric difference** A ⊕ B = (A∖B) ∪ (B∖A) = (A∪B)∖(A∩B) — "XOR of sets" (PW §3.1.5).
- **Symmetric difference behaves like XOR:** associative, commutative, A ⊕ A = ∅, A ⊕ ∅ = A, **cancellation A ⊕ B = A ⊕ C ⟹ B = C** (PW §3.1.6 note).
- Laws (PW §3.1.6): idempotent, domination (A∪U = U, A∩∅ = ∅), identity (A∩U = A, A∪∅ = A), associative, distributive A∩(B∪C) = (A∩B)∪(A∩C), absorption A∪(A∩B) = A, De Morgan.
- **Key difference identities:** A ∖ (B ∪ C) = (A∖B) ∩ (A∖C); A ∖ (B ∩ C) = (A∖B) ∪ (A∖C); (A∖B) ∖ C = A ∖ (B ∪ C) — always distribute the difference as an intersection/union of differences, and **never** drop parentheses: (A∖B)∖C ≠ A∖(B∖C) in general.
- **Cartesian product:** |A×B| = |A|·|B|; A×(B∪C) = (A×B) ∪ (A×C) (PW §3.1.8).
- **Inclusion–exclusion (3 sets):** |A∪B∪C| = |A|+|B|+|C| − |A∩B| − |A∩C| − |B∩C| + |A∩B∩C|. For the complement count, subtract from the universe.

---

## 2. Relations: the property dictionary (PW §3.2)

A relation R on A is a subset of A×A (think: 0/1 matrix, or directed graph).

| Property | Condition | Fast test on the graph/matrix |
|---|---|---|
| Reflexive | ∀a: aRa | all diagonal entries 1 |
| Irreflexive | ∀a: a R̄ a | all diagonal 0 |
| Symmetric | xRy ⇒ yRx | matrix symmetric; every edge has the reverse |
| Antisymmetric | xRy ∧ yRx ⇒ x = y | no 2-cycle between distinct elements |
| Asymmetric | xRy ⇒ y R̄ x | no cycle of length 1 or 2 (R ∩ R⁻¹ = ∅) |
| Transitive | xRy ∧ yRz ⇒ xRz | close under 2-step walks |

- **Asymmetric = irreflexive + antisymmetric**, so every asymmetric relation is antisymmetric (PW §3.2.4). Symmetric and antisymmetric simultaneously ⇒ only diagonal pairs (equality-like).
- **Equivalence relation** = reflexive + symmetric + transitive ⟺ **a partition of A** (classes = the partition blocks) (PW §3.2.9). Intersection of two equivalence relations is an equivalence relation; **union need not be** (PW §3.2.9). Closures stack in the order reflexive → symmetric → transitive (PW §3.2.6).

## 3. Counting relations — the master table (PW §3.2.4 + enumeration)

On a set with n elements (total relations: **2^(n²)**):

| Type | Count | n = 2 | n = 3 | n = 4 |
|---|---|---|---|---|
| reflexive / irreflexive | 2^(n²−n) | 4 | 64 | 4096 |
| symmetric | 2^(n(n+1)/2) | 8 | 64 | 1024 |
| antisymmetric | 2ⁿ · 3^(n(n−1)/2) | 12 | 216 | 11664 |
| asymmetric | 3^(n(n−1)/2) | 3 | 27 | 729 |
| transitive | no formula | 13 | 171 | 3994 |
| partial order | no formula | 3 | 19 | 219 |
| equivalence (Bell) | no formula | 2 | 5 | 15 |

Method for the diagonal/off-diagonal pairs: reflexivity fixes the n diagonal cells; each unordered pair {i,j} has 4 states, of which symmetric allows 2 (both edges or neither... i.e. treat each unordered pair as: no edges / both edges = 2 options), antisymmetric allows 3 (no edge / i→j / j→i). Multiply. Transitive *and* partial orders are **enumerated, not formula'd** — GATE only asks tiny n (≤ 4) where case-splitting works; use the standard values above as anchors.

**Equivalence relations = partitions (Bell numbers):** B₁…B₅ = 1, 2, 5, 15, 52. To reach B₅ = 52 by hand, add Stirling numbers of the second kind: S(5,1)=1, S(5,2)=15, S(5,3)=25, S(5,4)=10, S(5,5)=1. Two-block partitions of an n-set: **S(n,2) = 2ⁿ⁻¹ − 1** (which block holds element 1, minus taking everything).

## 4. Composition and closures (PW §3.2.5–§3.2.6)

- **Composition** S∘R = {(a,c) : ∃b, (a,b) ∈ R, (b,c) ∈ S}; for R on A, R² = R∘R, and R^k = k-step walks.
- **Transitive closure:** R⁺ = R ∪ R² ∪ … ∪ Rⁿ (n = |A| steps suffice); equivalently, in the digraph view, "add (a,b) for every reachable pair" — do it by tracing paths, not by expanding products forever.
- Reflexive closure = R ∪ Δ (Δ = diagonal); symmetric closure = R ∪ R⁻¹.
- R transitive ⟹ R^k ⊆ R for all k (PW).

## 5. Functions: counts and composition (PW §3.5–§3.6)

With |A| = m, |B| = n (the count is 0 when impossible):

| Type | Count |
|---|---|
| All functions f: A → B | **mⁿ** |
| Injective | **P(m,n) = m!/(m−n)!** (n ≥ m) |
| Bijective (m = n) | **n!** |
| Surjective (m ≥ n) | **Σᵢ₌₀ⁿ (−1)ⁱ C(n,i)(n−i)ᵐ** (PW §3.6.2) |

- For self-maps A → A with A finite: injective ⟺ surjective ⟺ bijective (PW §3.6.3).
- **Strictly increasing f: {1..m} → {1..n}** correspond to m-subsets of the codomain: count **C(n,m)**.
- **Composition rules** (gf = g∘f): gf injective ⟹ f injective (not g); gf surjective ⟹ g surjective (not f). Proofs are 2-line witness chases; counterexamples use tiny sets.
- Idempotent f (f∘f = f): f must fix every element of its image — useful when counting.

## 6. Posets (PW §3.3–§3.3.11)

A **partial order** = reflexive + antisymmetric + transitive. Poset = (S, ⪯).

- **Hasse diagram**: remove self-loops and transitively implied edges; draw covers upward.
- **Maximal** = nothing strictly above it; **Greatest** = above everything (≤ 1 exists); minimal/least dually. In a finite poset maximal/minimal elements exist; greatest/least may not, and if they exist they are **unique**.
- **Bounds**: upper bound of a subset; **lub (join, supremum)** = least upper bound; glb (meet, infimum) = greatest lower bound. In a lattice every **pair** has lub and glb.
- **Divisor posets Dₙ** (a ⪯ b ⟺ a | b): if n = p₁^e₁ ⋯ pₖ^eₖ, then Dₙ is the grid product [0,e₁]×⋯×[0,eₖ]:
  - number of elements = τ(n) = Π(eᵢ+1);
  - **maximal chains** from 1 to n = multinomial (Σeᵢ)! / Π(eᵢ!) — each chain = an order of the Σeᵢ prime factors;
  - Hasse-diagram edges = Σ over primes p | n of τ(n/p) (for each prime p, each divisor d of n/p gives the cover d ⟶ d·p);
  - n square-free ⟺ Dₙ is the Boolean lattice on k atoms (= hypercube Qₖ).
- **Chains and antichains**: a chain is totally ordered; an antichain has no comparable pairs. **Dilworth:** minimum number of chains covering a poset = size of its largest antichain. In the Boolean lattice, the largest antichain is the middle layer, of size C(n, ⌊n/2⌋) (Sperner).
- **Partial order counting anchors:** on {1,2,3}: 19; on {1,2,3,4}: 219 (enumerated). Decompose 19 by shape: antichain (1) + one comparable pair + isolated (6) + V (3) + Λ (3) + total orders (6).

## 7. Lattices (PW §3.4)

- Lattice = poset where every pair has a join (∨) and meet (∧); joins/meets are idempotent, commutative, associative, and satisfy absorption (PW §3.4.1). Join/meet semilattices have just one of the two operations.
- **Bounded**: has least (0) and greatest (1) element — every finite lattice is bounded (PW §3.4.3).
- **Complemented**: bounded and every element has a complement b (a∨b = 1, a∧b = 0). Complements need not be unique.
- **Distributive**: both distributive laws hold (e.g. every chain; every Dₙ; every P(S)). **In a bounded distributive lattice complements are unique** — so if you can exhibit **two different complements for one element, the lattice is not distributive** (PW §3.4.3). Equivalently: distributive ⟺ contains neither the pentagon N₅ nor the diamond M₃ as a sublattice.
- **Boolean algebra** = distributive + complemented; then every element has **exactly one** complement. Facts (PW §3.4.3): [Dₙ, |] is Boolean ⟺ n is square-free; [P(S), ⊆] is Boolean with 2^|S| elements, isomorphic to the hypercube; sublattice of a complemented lattice need not be complemented, but a sublattice of a bounded lattice is bounded.

## 8. Countability (banks: DM_03)

- **Countable** = finite, or can be listed: bijection with ℕ. ℤ, ℚ, ℕ×ℕ (diagonal enumeration) are countable; a countable union of countable sets is countable (e.g. all finite subsets of ℕ, all finite strings over a finite alphabet).
- **Uncountable:** P(ℕ) (Cantor), ℝ, and anything in bijection with ℝ (e.g. ℝ∖ℚ, infinite 0/1 sequences, functions ℕ → {0,1}, and relations on ℕ viewed as subsets of ℕ×ℕ).
- The countable-vs-uncountable split often decides MSQs: "all finite ___ over a countable structure" is countable; power sets keep jumping cardinality upward.

---

## 9. Trap list (read before the exam)

1. |P(A)| = 2ⁿ — powerset adds one dimension; P(P(A)) has 2^(2ⁿ) elements.
2. Even-size and odd-size subsets are equally many (2ⁿ⁻¹ each); ∅ counts as even.
3. (A∖B)∖C ≠ A∖(B∖C); A ∪ (B∩C) ≠ (A∪B) ∩ C.
4. Symmetric difference is XOR: A⊕B = A⊕C ⟹ B = C; A⊕A = ∅.
5. Antisymmetric ≠ "not symmetric" (both can fail: e.g. ∅ is both), and asymmetric = antisymmetric + irreflexive.
6. Symmetric counts treat each unordered pair as 2 options; antisymmetric as 3; total relations 2^(n²).
7. Reflexive + symmetric simultaneously: diagonal forced, free choices only on unordered pairs.
8. Equivalence relation ⟺ partition; Bell numbers 1, 2, 5, 15, 52; S(n,2) = 2ⁿ⁻¹ − 1.
9. R₁ ∪ R₂ of two equivalence relations is NOT necessarily an equivalence relation.
10. Transitive closure = reachability; n = |A| suffices (R ∪ … ∪ Rⁿ); don't forget multi-step chains.
11. Composition: gf injective gives injectivity only on the f-side; gf surjective only on the g-side.
12. Surjection counts need inclusion–exclusion; injective count is the falling factorial P(m,n).
13. Hasse diagram = covers only; maximal vs greatest: greatest needs to dominate ALL elements.
14. Lattice sublattices must preserve the ORIGINAL lub/glb — "it's a lattice by itself" is not enough.
15. Distributive lattices: unique complements; M₃ and N₅ are the two obstructions.
16. [Dₙ] Boolean ⟺ n square-free; chain D_{p^k} is a chain, not Boolean (unless k ≤ 1).
17. Countability: finite strings/subset-of-countable structure = countable; P(anything infinite) = uncountable.

---

## 10. Practice questions (fresh — not in the banks)

### Q1 · NAT · Fundamental

In a survey of 100 people: |A| = 41 like apples, |B| = 38 like bananas, |C| = 35 like cherries; |A∩B| = 12, |A∩C| = 15, |B∩C| = 9, and |A∩B∩C| = 5. The number of people who like at least one of the three fruits is ______.

### Q2 · MCQ · Fundamental

The number of subsets of the set {1, 2, 3, 4, 5} that have even cardinality is

- (A) 15
- (B) 16
- (C) 31
- (D) 32

### Q3 · MSQ · Standard

Which of the following identities hold for ALL sets A, B, C?

- (A) A ∖ (B ∪ C) = (A ∖ B) ∩ (A ∖ C)
- (B) (A ∖ B) ∖ C = A ∖ (B ∖ C)
- (C) A × (B ∪ C) = (A × B) ∪ (A × C)
- (D) A ∪ (B ∩ C) = (A ∪ B) ∩ C

### Q4 · NAT · Standard

The number of relations on the set {1, 2, 3} that are **antisymmetric** is ______.

### Q5 · NAT · Standard

A function f : {1, 2, 3, 4, 5} → {1, 2, 3, 4, 5, 6, 7, 8, 9} is called strictly increasing if x < y implies f(x) < f(y). The number of strictly increasing functions is ______.

### Q6 · NAT · Moderate

The number of equivalence relations on the set {1, 2, 3, 4} that have exactly two equivalence classes is ______.

### Q7 · NAT · Moderate

The number of transitive relations on the set {1, 2} is ______.

### Q8 · MSQ · Moderate

Which of the following sets is/are countable?

- (A) ℕ × ℕ (pairs of natural numbers)
- (B) The set of all subsets of ℕ
- (C) The set of rational numbers ℚ
- (D) The set of real numbers ℝ

### Q9 · NAT · Hard

The number of partial orders on the set {1, 2, 3} is ______.

### Q10 · NAT · Killer

The number of equivalence relations on the set {1, 2, 3, 4, 5} is ______.

---

## 11. Answer key & full solutions

**Answer key:** 1-83 · 2-B · 3-A,C · 4-216 · 5-126 · 6-7 · 7-13 · 8-A,C · 9-19 · 10-52

**Q1 — 83.** Inclusion–exclusion: 41 + 38 + 35 − 12 − 15 − 9 + 5 = **83**.

**Q2 — B.** Even sizes: C(5,0) + C(5,2) + C(5,4) = 1 + 10 + 5 = **16**. (In general half of 2⁵ = 32; (A) 15 = even subsets other than ∅; (C)/(D) are odd-total confusions.)

**Q3 — A, C.** (A) ✔ both sides = "in A, outside both B and C". (B) ✘ counterexample A = {0}, B = ∅, C = {0}: LHS = ∅, RHS = {0}. (C) ✔ a pair (a, x) has x ∈ B∪C ⟺ x ∈ B or x ∈ C. (D) ✘ counterexample A = {0}, B = ∅, C = ∅: LHS = {0}, RHS = ∅.

**Q4 — 216.** For each of the C(3,2) = 3 unordered pairs {i,j}: 3 options (no edge, i→j, j→i) — never both. Diagonal pairs are free: 2³. Total 3³ · 2³ = 27 · 8 = **216** = 2ⁿ·3^(n(n−1)/2) with n = 3 (PW §3.2.4). (Exhaustive check of all 512 relations on {1,2,3} agrees.)

**Q5 — 126.** A strictly increasing function is determined by its **image set**: choose any 5 of the 9 values, order them increasingly → C(9,5) = **126**.

**Q6 — 7.** Equivalence relations = partitions; two classes = choose the block containing 1: any non-empty proper subset of {2,3,4}: 2³ − 1 = **7**. (Same as S(4,2) = 7.)

**Q7 — 13.** A relation on {1,2} is a 0/1 choice for the four pairs (1,1), (1,2), (2,1), (2,2). Transitivity bites only when (1,2) and (2,1) are both present: it then forces (1,1) and (2,2). Count: 3 choices where not both cross-edges exist (none / 1→2 only / 2→1 only) with the 2 diagonals free: 3·4 = 12, plus the one fully-crossed relation: **13**.

**Q8 — A, C.** (A) ✔ enumerate pairs diagonally ((1,1), (1,2), (2,1), (1,3), …). (C) ✔ represent each rational as (numerator, denominator) ∈ ℤ×ℕ. (B) ✘ Cantor's theorem: |P(ℕ)| = 2^ℵ₀ > ℵ₀. (D) ✘ the diagonal argument: no listing of ℝ works.

**Q9 — 19.** Decompose by shape (PW §3.3):
- antichain: 1;
- exactly one comparable pair, third element incomparable: choose the pair C(3,2) = 3 × 2 directions = 6;
- "V" (one bottom below both others): choose bottom = 3;
- "Λ" (one top above both): 3;
- total orders (chains): 3! = 6.
Total 1 + 6 + 3 + 3 + 6 = **19**. (Brute force over all 2⁹ = 512 relations: reflexive ∧ antisymmetric ∧ transitive = 19.)

**Q10 — 52.** Equivalence relations on a 5-set = partitions of 5 = Bell number B₅ = Σₖ S(5,k): 1-block (1) + 2-block (2⁴ − 1 = 15) + 3-block (S(5,3) = (3⁵ − 3·2⁵ + 3)/6 = 25) + 4-block (C(5,2) = 10) + 5-block (1) = **52**. (Exhaustive enumeration of all 2²⁵ relations confirms.)

---

## 12. Next steps

1. Re-derive §3's master table row by row (diagonal/off-diagonal argument) and §6's divisor-lattice formulas — they are the seeds of most killer questions.
2. Memorize Bell anchors 1, 2, 5, 15, 52 and partial-order anchors 1, 3, 19, 219.
3. Run the banks `DM_03` and `DM_04` cold. This completes the Discrete Mathematics notes — move on to **Engineering Mathematics** (linear algebra → calculus → probability & statistics).
