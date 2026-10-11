# Theory of Computation → Regular Expressions & Finite Automata

**GATE 2027 syllabus:** Theory of Computation — regular expressions and finite automata.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** Σ = {a, b} unless stated otherwise. A DFA is deterministic and total (a transition defined for every state-symbol pair) unless stated otherwise. Minimization means the unique minimal-state DFA (up to renaming) accepting the same language, found by removing unreachable states and merging equivalent (indistinguishable) states.

---

## Questions

### Q1 · MCQ · Fundamental
A DFA and an NFA accepting the same language always have

- (A) exactly the same number of states
- (B) the DFA with strictly fewer states than the NFA
- (C) the same expressive power (every NFA has an equivalent DFA), though the DFA may need more states
- (D) different expressive power — NFAs can accept languages no DFA can

---

### Q2 · NAT · Fundamental
A DFA over Σ = {0,1} accepts all strings containing the substring "01". Built directly (3 states: "no progress," "seen 0," "seen 01/trap-accept"), the minimum number of states in a DFA accepting this language is ______.

---

### Q3 · MSQ · Standard
Which of the following statements about regular languages is/are TRUE?

- (A) The union of two regular languages is regular.
- (B) The intersection of two regular languages is regular.
- (C) The complement of a regular language (with respect to Σ*) is regular.
- (D) Every finite language (a finite set of strings) is regular.

---

### Q4 · NAT · Standard
A DFA over Σ = {a,b} has 5 states {q0,...,q4}. Transitions: q0→q1 (a), q0→q0 (b), q1→q0 (a), q1→q1 (b), q2→q3 (a), q2→q2 (b), q3→q2 (a), q3→q3 (b), q4→q4 (a), q4→q4 (b). Accept states = {q0, q2}, start state q0. (This DFA accepts strings with an even number of a's — states q0/q1 and q2/q3 are two redundant copies of the same 2-state logic, and q4 is unreachable.) The number of states in the **minimized** DFA is ______.

---

### Q5 · MCQ · Standard
Using **subset construction** to convert an NFA to an equivalent DFA, in the worst case the resulting DFA has, relative to an n-state NFA,

- (A) at most n states
- (B) exactly n states
- (C) at most 2ⁿ states (one per subset of NFA states)
- (D) at most n² states

---

### Q6 · NAT · Moderate
An NFA over Σ = {a,b} accepts strings ending in "ab": q0 (start) has a-transitions to both q0 and q1 (b stays at q0); q1 has a b-transition to q2 (accepting). Applying subset construction, the number of **reachable** DFA states (including the empty/dead state, if reachable) is ______.

---

### Q7 · MSQ · Moderate
Which of the following statements is/are TRUE, where L₁ and L₂ are regular languages over the same alphabet?

- (A) L₁ · L₂ (concatenation) is regular.
- (B) L₁* (Kleene star) is regular.
- (C) L₁ − L₂ (set difference) is regular.
- (D) The reversal of L₁ (every string in L₁ written backwards) is regular.

---

### Q8 · NAT · Moderate
An NFA over Σ = {a,b} accepts strings **containing** "aba" as a substring, using states q0 (start), q1, q2, q3 (accepting, with a self-loop on both a and b once reached): q0 has a-transitions to both q0 and q1 (b stays at q0); q1 has a b-transition to q2; q2 has an a-transition to q3; q3 loops to itself on both a and b. Applying subset construction, the number of **reachable** DFA states is ______.

---

### Q9 · MSQ · Hard
Which of the following statements about the **pumping lemma for regular languages** is/are TRUE?

- (A) It states that every sufficiently long string in a regular language L can be split into xyz with |xy| ≤ p, |y| ≥ 1, and xyⁱz ∈ L for all i ≥ 0, for some pumping length p depending only on L.
- (B) It can be used to prove a language is regular, by exhibiting a valid pumping decomposition.
- (C) It is typically used to prove a language is **not** regular, by showing that **no** valid decomposition allows pumping to stay in the language.
- (D) Failing to find a pumping decomposition for one specific string in L does not necessarily prove L is non-regular, unless every possible split of that string is shown to fail.

---

### Q10 · NAT · Killer
Consider the language L = {aⁿbⁿ | n ≥ 0} over Σ = {a,b}. Using the pumping lemma with pumping length p = 5, the string s = a⁵b⁵ is chosen (as is standard). For **every** valid split s = xyz with |xy| ≤ 5 and |y| ≥ 1, the substring y consists entirely of a's (since |xy| ≤ 5 means xy lies within the first 5 characters, all a's) — say y has length k, where 1 ≤ k ≤ 5. Pumping down to xy⁰z = xz then produces a string with exactly (5 − k) a's and 5 b's, which is never of the form aᵐbᵐ (since 5 − k ≠ 5 whenever k ≥ 1). The number of distinct valid choices of k (from 1 to 5) for which this happens — i.e., for which pumping down breaks the string out of L — is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | C | NFA = DFA in expressive power (regular languages); DFA may need exponentially more states in the worst case |
| 2 | 3 | States: no-progress, seen-0, seen-01 (absorbing accept) |
| 3 | A, B, C, D | All four are standard closure properties of regular languages |
| 4 | 2 | Verified by table-filling minimization: q0≡q2, q1≡q3, and q4 is unreachable — 2 equivalence classes |
| 5 | C | Each DFA state corresponds to a subset of NFA states |
| 6 | 3 | Reachable DFA states: {q0}, {q0,q1}, {q0,q2} — verified by subset construction |
| 7 | A, B, C, D | All four are standard closure properties of regular languages |
| 8 | 6 | Verified by subset construction; reachable subsets: {q0}, {q0,q1}, {q0,q2}, {q0,q3}, {q0,q1,q3}, {q0,q2,q3} (the minimal DFA for this language has only 4 states) |
| 9 | A, C, D | (B) is false — the pumping lemma is a necessary condition for regularity, not a sufficient one; it can only be used to disprove regularity, never to prove it |
| 10 | 5 | Every k from 1 to 5 gives (5−k) a's and 5 b's with 5−k ≠ 5 (since k ≥ 1), so all 5 values of k break the string out of L |
