# CD Notes 02 — Bottom-Up Parsing & the LR Family (High-Yield Deep Dive)

> **Why this file exists:** the second half of the video topic "Parsers". Shift-reduce mechanics, ITEMS, the LR(0) automaton, and the four table-construction methods (LR(0) → SLR(1) → LALR(1) → CLR(1)) are the highest-yield "compute something" area of Compiler Design: state counts, table-cell counts, and conflict positions. Every automaton, state count and cell count below was **verified by running an LR/CLR/LALR simulator** before writing.
>
> **Companion question bank:** `../CD_03_BottomUp_Parsing_LR0_SLR1.md`. **PW source:** `../Reference/PW_Subject_Handbooks/PW_CompilerDesign.txt` (§2.2.11–2.4.15, pages 8.8–8.11: bottom-up parser, construction algorithm, entry types, conflicts, inadequate states).

---

## 0. The mental model: reduce your way home

A bottom-up parser starts with the input and repeatedly **reduces** a substring to a nonterminal until only the start symbol remains — i.e., it discovers a **rightmost derivation in reverse**.

```
Input:  a a b b $          Shift a|a|b|b onto the stack, reduce S → a S b, S → ε …
Stack:  states + grammar symbols (DPDA configuration)
```

**Left recursion and common prefixes are not problems** here (PW §2.2.11) — that is why practical parsers (yacc/bison style) are LR.

---

## 1. Shift-reduce mechanics (PW §2.2.11–2.2.12)

Four actions in the ACTION table, keyed by (state, lookahead):

| Action | Meaning |
|---|---|
| **shift s_k** | push the token and go to state k |
| **reduce A → α** | pop **|α| states** off the stack, then push GOTO(state, A) |
| **accept** | the item is S′ → S· — **this is not a reduction** (PW note) |
| **error** | empty cell |

- With a conflict-free table this is a deterministic pushdown automaton running in **O(n)** time.
- The **handle** is the substring replaced at each reduce; the stack always holds a **viable prefix** of a right-sentential form.
- Reduce in state I processing *item* `A → α·` means: you have just seen α on the stack, and (in the right table) the lookahead allows the reduction.

---

## 2. Items and the LR(0) automaton (PW §2.2.12)

**Construction recipe:** (1) augment the grammar with `S′ → S`; (2) build item sets (closure/goto); (3) fill table entries per §3.

- **LR(0) item:** a production with a dot marking how much you have consumed: `A → α·β`. Dot at the end (`A → α·`) = **complete item** = ready to reduce. Dot before a nonterminal triggers **closure**: add all items `B → ·γ` for that nonterminal.
- **goto(I, X):** advance the dot over X in every item of I that has X after the dot, then take closure.
- The start state is `closure({S′ → ·S})`; the **accept state** contains `S′ → S·`.
- Every LR variant (SLR/LALR/CLR) starts from these item sets — the difference is only *which lookahead information* is used for reduce entries (§3).

---

## 3. The one table that separates the four LR parsers (PW §2.2.13)

| Entry type | LR(0) | SLR(1) | LALR(1) | CLR(1) |
|---|---|---|---|---|
| **shift** (on terminal) | same | same | same | same (CLR automaton is bigger, but the same idea) |
| **goto** (on nonterminal) | same | same | same | same idea |
| **reduce** A → α entered under… | **EVERY column** of the state | columns **FOLLOW(A)** | the item's **lookaheads** (LALR) | the item's **lookaheads** (CLR) |

- SLR(1) and LALR(1) both use the **plain LR(0) automaton** — SLR reads lookaheads off FOLLOW sets, LALR attaches state-specific lookahead sets to the items. ⇒ **SLR and LALR tables always have the same number of states.**
- CLR(1) bakes the lookahead into each item ⇒ its automaton can be **larger**.

---

## 4. Conflicts and inadequate states (PW §2.2.14, §2.4.15)

A state with **any** conflict is called a **conflicting/inadequate** state.

| Conflict | Definition (per PW) |
|---|---|
| **Shift-Reduce (SR)** | a shift item `A → α·tβ` and a complete item coexist, and the reduction is *visible* on t: LR(0): always; SLR: **t ∈ FOLLOW(A)**; LALR/CLR: **t ∈ lookahead(A → α·)** |
| **Reduce-Reduce (RR)** | two complete items whose reduce sets overlap: LR(0): always; SLR: **FOLLOW(X) ∩ FOLLOW(Y) ≠ ∅**; LALR/CLR: **L₁ ∩ L₂ ≠ ∅** |

**Resolution conventions (parser generators):** prefer **shift** over reduce; for RR, reduce by the production written **first**. With such disambiguation an LALR parser can handle some grammars that are technically not LALR(1) (PW's "very important point": favouring shift over reduce parses the classic `E → E+E | E*E` ambiguity by precedence). Cascade: `LR(0) ⊂ SLR(1) ⊂ LALR(1) ⊂ CLR(1)` as grammar classes.

---

## 5. CLR vs LALR: the merging story (PW §2.4.15)

The **only** structural difference: CLR states with the **same core** (same LR(0) items, different lookaheads) are **merged** in LALR to save space.

**PW's three certification facts (PW §2.4.15):**
1. If CLR(1) has **no** conflict, conflicts **may or may not** appear after merging (LALR).
2. If LALR(1) has an **SR** conflict, then CLR(1) also has one.
3. LALR(1) has an SR conflict **if and only if** CLR(1) does.

**Why:** merging only *grows* lookahead sets and *unions* cores.
- A reduce item's LALR lookahead is the **union** of CLR lookaheads ⇒ **RR conflicts can be created** (never removed).
- SR conflicts **cannot be created or destroyed** by merging ⇒ the iff in fact 3.
- **RR conflicts present in CLR persist too** — union only grows. So merging can only *add* conflicts (RR), never remove any.

**Verified worked example (the classic RR-introducing grammar):**

```
S → a E c | a F d | b F c | b E d        E → e        F → e
```

- CLR: **14 states**, no conflicts. The states reached after `ae` and after `be` have the **same core** `{E → e·, F → e·}`.
- LALR merges them (14 → **13 states**): lookaheads union to `E → e· {c, d}`, `F → e· {c, d}` ⇒ **RR conflict on both c and d (two conflict cells)**.
- This is *the* example of "LALR(1) fails where CLR(1) succeeds".

**Second classic (different failure mode):** `S → L = R | R`, `L → * R | id`, `R → L` — LR(0)/SLR automaton has **10 states**, with an **SR conflict on `=`** (state `{S → L·=R, R → L·}`: FOLLOW(R) contains `=` globally, but in this state the reduction is never valid). CLR has 14 states and no conflict; **LALR (10 states) has no conflict either** ⇒ the grammar is **LALR(1) but not SLR(1)**.

---

## 6. Worked automaton: S → a S b | ε (verified, 5 states)

Augment: `P0: S′ → S`, `P1: S → a S b`, `P2: S → ε`. FOLLOW(S) = {b, $}.

```
I0 = {S′→·S, S→·aSb, S→·}        goto(I0,a) = I2   goto(I0,S) = I1
I1 = {S′→S·}  (accept)           goto(I2,a) = I2   goto(I2,S) = I3
I2 = {S→a·Sb, S→·aSb, S→·}       goto(I3,b) = I4
I3 = {S→aS·b}
I4 = {S→aSb·}
```

**SLR(1) ACTION table** (10 non-empty cells, no conflict):

| State | a | b | $ |
|---|---|---|---|
| I0 | s I2 | r2 | r2 |
| I1 | | | acc |
| I2 | s I2 | r2 | r2 |
| I3 | | s I4 | |
| I4 | | r1 | r1 |

Teaching points from this table: the **ε-item** `S → ·` (r2) appears in I0/I2 — LR(0) would put r2 in *every* column and clash with `shift a`, but SLR places it only on FOLLOW(S) = {b, $} ⇒ no conflict. **This grammar is SLR(1) but not LR(0).**

---

## 7. GATE facts & traps

- LR parser = DPDA; stack holds states (and matched symbols); **reduce A → α pops |α| states**, then GOTO.
- **Accept** is `S′ → S·` — not a reduce (PW).
- **Shift and goto entries are identical across SLR/LALR** (same LR(0) automaton); only **reduce lookaheads** differ. CLR uses its own bigger automaton.
- **State counts:** SLR = LALR (both LR(0)-based); CLR can be larger (merging shrinks it). For `S→L=R…`: 10/10/14. For the `aEc` grammar: 13 (LALR) vs 14 (CLR).
- **Merging can create RR conflicts; SR conflicts are created/destroyed never** (iff with CLR).
- No parser exists for an ambiguous grammar (except operator-precedence for some); unambiguous grammars can still fail every LR method (e.g. PW's `S → aSa | bSb | a | b` — unambiguous but no parser).
- A language must be a **DCFL** for any LR parser to exist; DCFL ⇔ has an LR(1) grammar.
- Reduce-reduce conflict in **SLR** needs `FOLLOW` sets to intersect — two complete items alone are **not** enough (their FOLLOWs may be disjoint; but in LR(0) tables any two completes clash).

---

## 8. Practice — 10 verified questions

> Every state count / cell count below was produced by the simulator, not by hand-waving.

### Q1 · MCQ · Fundamental

When an LR parser reduces `A → α` with `|α| = k`, it pops ___ from the state stack (state view) and then pushes `GOTO(top, A)`.

- (A) k + 1 states
- (B) exactly k states
- (C) exactly 1 state, regardless of k
- (D) 2k states

### Q2 · NAT · Fundamental

For G: `S → a S | b`, the number of states in the canonical **LR(0)** item-set automaton of the augmented grammar is ______.

### Q3 · MSQ · Standard

Which of the following statements about LR parsing is/are TRUE?

- (A) An LR parser traces a rightmost derivation in reverse.
- (B) A shift-reduce parser behaves as a deterministic pushdown automaton; with a conflict-free table, parsing n tokens takes O(n) time.
- (C) A state whose item set consists of a single complete item (and nothing else) can never be a conflict state.
- (D) LALR(1) tables are built on a different automaton than SLR(1), so their shift entries can differ for the same grammar.

### Q4 · NAT · Standard

For G: `S → C C`, `C → c C | d`, the number of states in its LR(0) item-set automaton (equivalently: the number of states in its SLR(1)/LALR(1) tables) is ______.

### Q5 · MCQ · Standard

Canonical LR(1) states with the same core (same LR(0) items, different lookaheads) are merged to form the LALR(1) automaton. Which one of the following is TRUE about this merging?

- (A) It can create new shift-reduce conflicts but never reduce-reduce conflicts.
- (B) It can create new reduce-reduce conflicts, but it can never create or destroy a shift-reduce conflict.
- (C) It can destroy shift-reduce conflicts that were present in the canonical LR(1) automaton.
- (D) It never creates any new conflict of any kind.

### Q6 · NAT · Moderate

For G of Q4 (`S → C C`, `C → c C | d`), count the **non-empty ACTION entries** of its SLR(1) table: rows = states, columns = terminals {c, d} ∪ {$}, each entry a shift, a reduce, or accept. Count = ______.

### Q7 · MSQ · Moderate

Which of the following statements is/are TRUE?

- (A) A shift-reduce conflict present in the plain LR(0) automaton state always persists into the SLR(1) table.
- (B) The canonical LR(1) automaton may contain two different states with exactly the same core but different lookahead sets.
- (C) For G: `S → a E c | a F d | b F c | b E d`, `E → e`, `F → e`, the canonical LR(1) automaton contains two distinct states with the same core — reached in the "ae" and "be" contexts — which the LALR construction merges into one.
- (D) For the same grammar, the SLR(1) and LALR(1) tables always have the same number of states.

### Q8 · NAT · Moderate

For G: `S → a E c | a F d | b F c | b E d`, `E → e`, `F → e` (same as Q7), the number of ACTION cells of the LALR(1) table that contain a **reduce-reduce conflict** is ______.

### Q9 · MSQ · Hard

Which of the following statements is/are TRUE?

- (A) A state containing a shift on terminal t and a complete item `A → α·` produces a shift-reduce conflict in the SLR(1) table if and only if t ∈ FOLLOW(A).
- (B) Two different complete items in the same state always produce a reduce-reduce conflict in the SLR(1) table.
- (C) A reduce-reduce conflict present in the SLR(1) table can sometimes disappear in the LALR(1) table, because LALR lookaheads are state-specific subsets of the FOLLOW sets.
- (D) Every language generated by an LR(1) grammar is a deterministic context-free language (DCFL).

### Q10 · NAT · Killer

For G: `S → a E c | a F d | b F c | b E d`, `E → e`, `F → e` (same grammar as Q7/Q8), let a = number of states of the canonical LR(1) automaton and b = number of states of the LALR(1) automaton. Then a − b = ______.

---

## 9. Answer key & full solutions

**Answer key:** 1-B · 2-5 · 3-A,B,C · 4-7 · 5-B · 6-14 · 7-B,C,D · 8-2 · 9-A,C,D · 10-1

**Q1 — B.** On reduce `A → α`, the parser pops |α| states (plus their symbol counterparts) and pushes GOTO. Reduce pops exactly k states.

**Q2 — 5.** States: I0 = {S′→·S, S→·aS, S→·b}, I1 = {S→a·S, S→·aS, S→·b}, I2 = {S→b·}, I3 = {S′→S·} (accept), I4 = {S→aS·}. That's 5 — the easy-to-forget one is I4 (reduce after matching `a…S`). *(Hand-traps: some texts count 4 by forgetting the reduce state.)*

**Q3 — A, B, C.** (A) ✔ bottom-up = RMD in reverse. (B) ✔ LR is a DPDA; linear time with a conflict-free table. (C) ✔ one complete item and no shift items ⇒ no partner for SR or RR. (D) ✘ SLR and LALR both use the LR(0) automaton — shift entries identical (only reduces differ).

**Q4 — 7.** States: I0 = {S′→·S, S→·CC, C→·cC, C→·d}; I1 = goto(I0,c) = {C→c·C, C→·cC, C→·d}; I2 = goto(I0,d) = {C→d·}; I3 = goto(I0,C) = {S→C·C, C→·cC, C→·d}; I4 = goto(I0,S) = {S′→S·}; I5 = goto(I3,C) = {S→CC·}; I6 = goto(I1,C) = {C→cC·}. Total **7**.

**Q5 — B.** Merging unions lookaheads (⇒ can create RR) — and by the persistence argument/PW fact ("LALR SR iff CLR SR") it can neither create nor destroy SR conflicts. (A) reverses it; (C) merging never removes conflicts (lookaheads only grow); (D) false — RR conflicts are routinely introduced.

**Q6 — 14.** Counting per state (terminals only): I0: c, d (shifts) → 2; I1: c, d → 2; I2: c, d, $ (reduce C→d on FOLLOW(C) = {c,d,$}) → 3; I3: c, d → 2; I4: $ (accept) → 1; I5: $ (reduce S→CC) → 1; I6: c, d, $ (reduce C→cC·) → 3. **2+2+3+2+1+1+3 = 14**, and there are **no conflicts** (each state's shifts and reduces never share a column) ⇒ SLR(1).

**Q7 — B, C, D.** (A) ✘ SLR places reduces only on FOLLOW(A) — if the shifted terminal is not in FOLLOW(A), the LR(0) conflict evaporates in SLR (this is exactly how the §6 grammar is SLR despite an LR(0) clash). (B) ✔ that is precisely why LALR merging exists. (C) ✔ verified: after `ae` the state is {E→e·{c}, F→e·{d}}; after `be` it is {E→e·{d}, F→e·{c}} — same core, different lookaheads, merged by LALR. (D) ✔ both SLR and LALR ride the same LR(0)-based automaton.

**Q8 — 2.** After merging, the lookaheads union to E→e· = {c, d} and F→e· = {c, d}; both reduce items claim both `c` and `d` ⇒ RR conflicts in 2 cells. (In CLR the two states keep disjoint lookaheads {c} vs {d} — no conflict there.)

**Q9 — A, C, D.** (A) ✔ SR in SLR happens iff the shifted terminal is in the reduce item's SLR lookahead set = FOLLOW. (B) ✘ SLR only conflicts if FOLLOW(X) ∩ FOLLOW(Y) ≠ ∅ — disjoint FOLLOWs mean no RR even with two completes (they always conflict in LR(0) tables though). (C) ✔ LALR lookaheads are state-specific subsets of FOLLOW; a reduce can drop out of the clashing column. (D) ✔ Knuth's characterization: languages with LR(1) grammars are exactly the DCFLs.

**Q10 — 1.** Simulator: CLR(1) = **14** states; the LALR construction merges exactly one same-core pair (states after `ae`/`be`) ⇒ LALR = **13** states. a − b = **1**. (Bonus fact: for this grammar the SLR table is *also* conflicting — the RR appears already via FOLLOW(E) ∩ FOLLOW(F) ∋ c, d — so the grammar is LALR-only, not SLR.)

---

## 10. Next steps

1. Rebuild the §6 automaton and its SLR table from a blank page — that 5-state exercise is the exam skeleton.
2. Redo Q4's automaton by hand (7 states) and Q6's count; these are typical GATE one-markers.
3. Continue to **File 03 — Syntax-Directed Translation** (`CD_Notes_03_Syntax_Directed_Translation.md`, bank `CD_04`). Compiler Design then jumps to Discrete Math in the overall plan.
