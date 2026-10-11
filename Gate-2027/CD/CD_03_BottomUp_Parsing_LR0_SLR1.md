# Compiler Design → Bottom-Up Parsing (LR(0), SLR(1))

**GATE 2027 syllabus:** Compiler Design — parsing.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** An LR(0) item is a production with a dot marking how much of the right-hand side has been matched. The canonical collection of LR(0) items is built by closure and goto operations on an augmented grammar (S' → S added). A grammar is SLR(1) if the SLR(1) parsing table (built using FOLLOW sets to decide reduce actions) has no shift-reduce or reduce-reduce conflicts.

---

## Questions

### Q1 · MCQ · Fundamental
An LR(0) item A → α.β indicates that, in the current parse state, the parser has

- (A) already reduced by A → αβ
- (B) recognized α on top of the stack and expects to see a string derivable from β next
- (C) rejected the input
- (D) completed parsing the entire input

---

### Q2 · NAT · Fundamental
For the augmented grammar E' → E, E → E + T | T, T → T * F | F, F → ( E ) | id, the closure of the initial item set {E' → .E} includes the item T → .T * F. The total number of items in this initial closure (I₀) is ______.

---

### Q3 · MSQ · Standard
Which of the following statements about LR parsing is/are TRUE?

- (A) LR parsers can handle a strictly larger class of grammars than LL parsers.
- (B) An LR(0) parser makes shift/reduce decisions using only the current state (no lookahead at all).
- (C) An SLR(1) parser uses one symbol of lookahead together with FOLLOW sets to resolve reduce decisions.
- (D) Every SLR(1) grammar is also LR(0).

---

### Q4 · NAT · Standard
For the grammar of Q2 (the classic expression grammar), the total number of states in the canonical LR(0) automaton is ______.

---

### Q5 · MCQ · Standard
A **shift-reduce conflict** in an SLR(1) parsing table occurs when, for some state and lookahead symbol, the table would need to specify

- (A) two different reduce actions
- (B) both a shift action and a reduce action
- (C) two different goto actions
- (D) neither a shift nor a reduce action

---

### Q6 · NAT · Moderate
For the classic expression grammar of Q2/Q4, the full SLR(1) parsing table (ACTION and GOTO combined) has exactly ______ non-empty entries in total.

---

### Q7 · MSQ · Moderate
Which of the following statements is/are TRUE about the classic grammar S → L = R | R, L → * R | id, R → L (used to illustrate pointer dereference and assignment)?

- (A) This grammar is ambiguous.
- (B) This grammar has a shift-reduce conflict in its SLR(1) parsing table, caused by the FOLLOW set of R including '=' even though, in the specific state where the conflict arises, that reduction is never actually valid.
- (C) This grammar is not SLR(1), but it is LALR(1) (and also full canonical LR(1)), since those stronger methods use more precise lookahead information tied to the specific parser state.
- (D) The canonical LR(0) automaton for this grammar has exactly 10 states.

---

### Q8 · NAT · Moderate
For the grammar S → L = R | R, L → * R | id, R → L (same as Q7), the SLR(1) parsing table construction finds exactly ______ shift-reduce or reduce-reduce conflict(s) in total.

---

### Q9 · MSQ · Hard
Which of the following statements about the relationship between SLR(1), LALR(1) and canonical LR(1) parsing is/are TRUE?

- (A) All three build the same underlying LR(0)-like automaton structure (SLR(1) and LALR(1) use the plain LR(0) item sets; canonical LR(1) uses a typically larger automaton with lookahead baked into each item).
- (B) LALR(1) tables are obtained by merging canonical LR(1) states that have the same "core" (set of LR(0) items, ignoring lookaheads), which can occasionally introduce a reduce-reduce conflict not present in the full canonical LR(1) table.
- (C) Every grammar that is SLR(1) is also LALR(1).
- (D) Every grammar that is LALR(1) is also SLR(1).

---

### Q10 · NAT · Killer
Two grammars are considered: Grammar A is the classic expression grammar (Q2/Q4/Q6: 12 LR(0) states, 0 SLR(1) conflicts, 36 ACTION + 9 GOTO = 45 total SLR(1) table entries). Grammar B is S → L = R | R, L → * R | id, R → L (Q7/Q8: 10 LR(0) states, 1 SLR(1) conflict). Let D = (number of LR(0) states in Grammar A) − (number of LR(0) states in Grammar B), and let C = (number of SLR(1) conflicts in Grammar A) + (number of SLR(1) conflicts in Grammar B). The value of D + C is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | B | Standard interpretation of an LR item |
| 2 | 7 | Verified by direct closure computation: E'→.E, E→.E+T, E→.T, T→.T*F, T→.F, F→.(E), F→.id |
| 3 | A, B, C | (D) is false — SLR(1) is a strictly larger class than LR(0); not every SLR(1) grammar is LR(0) |
| 4 | 12 | Verified by building the full canonical LR(0) automaton |
| 5 | B | Definition of a shift-reduce conflict |
| 6 | 45 | Verified: 36 ACTION entries (shifts + reduces + accept) + 9 GOTO entries |
| 7 | A, B, C, D | All four are standard, correct facts about this classic illustrative grammar |
| 8 | 1 | Verified: exactly one shift-reduce conflict, at the state reached after L, on lookahead '=' |
| 9 | A, B, C | (D) is false — LALR(1) can resolve some conflicts (via more precise, state-specific lookaheads) that SLR(1) cannot, so LALR(1) is strictly more powerful, not equal to or weaker |
| 10 | 3 | D = 12 − 10 = 2; C = 0 + 1 = 1; D + C = 2 + 1 |
