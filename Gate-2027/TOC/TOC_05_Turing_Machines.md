# Theory of Computation → Turing Machines

**GATE 2027 syllabus:** Theory of Computation — Turing machines.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** A (single-tape, deterministic) Turing machine's transition function maps (state, tape symbol) to (new state, symbol to write, head move ∈ {L, R}); an undefined transition means implicit rejection. "Accept" means the machine halts in an accepting state. One "step" is one application of the transition function.

---

## Questions

### Q1 · MCQ · Fundamental
A Turing machine, compared to a pushdown automaton, has

- (A) a read-only input tape and no auxiliary memory at all
- (B) a tape that can be both read and written, with the head free to move in either direction
- (C) exactly the same computational power, since both use a stack-like structure
- (D) two tapes instead of one, by definition

---

### Q2 · NAT · Fundamental
A Turing machine that increments a binary number (rightmost bit is the least significant, head starts at the leftmost symbol) is run on the input `111`. Using the standard algorithm (scan right to the end, then scan left flipping 1s to 0s until a 0 is found and flipped to 1, or the left end is reached and a 1 is prepended), the number of steps taken until the machine halts is ______.

---

### Q3 · MSQ · Standard
Which of the following statements is/are TRUE?

- (A) Every regular language is decidable (recursive).
- (B) Every context-free language is decidable (recursive).
- (C) Every decidable language is recognizable (recursively enumerable).
- (D) Every recognizable (recursively enumerable) language is decidable.

---

### Q4 · NAT · Standard
A single-tape Turing machine decides L = {aⁿbⁿcⁿ : n ≥ 0} using the standard "cross off one of each symbol per pass" algorithm (mark one unmarked a, then the next unmarked b, then the next unmarked c, then return to the left end and repeat; when no unmarked a remains, verify the rest of the tape has no leftover unmarked b or c). Run on the input `aabbcc` (n = 2), the total number of steps (transition applications) until the machine halts and accepts is ______.

---

### Q5 · MCQ · Standard
A k-tape Turing machine (k ≥ 2) and a single-tape Turing machine

- (A) are not equivalent — k-tape machines can decide strictly more languages
- (B) are equivalent in the languages they can decide, but a k-tape machine can be asymptotically faster (e.g., polynomial vs. possibly-quadratic slowdown for simulation)
- (C) are equivalent in both the languages they decide and their exact running time
- (D) differ only in that k-tape machines can accept, but never reject

---

### Q6 · NAT · Moderate
Using the same Turing machine and algorithm as Q4, run on the input `aaabbbccc` (n = 3), the total number of steps until the machine halts and accepts is ______.

---

### Q7 · MSQ · Moderate
Which of the following statements about **recursively enumerable (RE)** languages is/are TRUE?

- (A) A language L is RE if and only if some Turing machine halts and accepts on every string in L (and either rejects or loops forever on strings not in L).
- (B) If both L and its complement are RE, then L is decidable (recursive).
- (C) The class of RE languages is closed under union and intersection.
- (D) The class of RE languages is closed under complementation.

---

### Q8 · NAT · Moderate
The same binary-increment Turing machine from Q2 is run on the input `1111`. The number of steps taken until it halts is ______.

---

### Q9 · MSQ · Hard
Which of the following statements about the Church–Turing thesis and computability is/are TRUE?

- (A) The Church–Turing thesis asserts that every function that is "effectively computable" (by any reasonable intuitive notion of algorithm) can be computed by some Turing machine.
- (B) The thesis is a mathematically proven theorem, not a hypothesis.
- (C) Multiple independently-developed formal models of computation (Turing machines, lambda calculus, general recursive functions) have all been shown to be equivalent in power, which is considered strong evidence for the thesis.
- (D) A universal Turing machine can simulate any other Turing machine, given a description of that machine and its input.

---

### Q10 · NAT · Killer
Using the same Turing machine and algorithm as Q4 and Q6, the number of steps taken to accept `aⁿbⁿcⁿ` follows the pattern: n=1 → 10 steps, n=2 → 29 steps, n=3 → 58 steps, n=4 → 97 steps. This sequence fits a quadratic formula steps(n) = An² + Bn + C. Using this formula (fitted from the four given data points), the predicted number of steps for n = 6 is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | B | The read/write, bidirectional tape is the key extra power over a PDA's stack |
| 2 | 8 | Verified by simulation: 111 → 1000 in 8 steps |
| 3 | A, B, C | (D) is false — RE does not imply decidable (e.g. the halting problem's language) |
| 4 | 29 | Verified by direct simulation of the described TM |
| 5 | B | Standard simulation result: single-tape can simulate k-tape with at most quadratic slowdown |
| 6 | 58 | Verified by direct simulation |
| 7 | A, B, C | (D) is false — RE languages are not closed under complement in general (that's exactly the recursive/RE distinction) |
| 8 | 10 | Verified by simulation: 1111 → 10000 in 10 steps |
| 9 | A, C, D | (B) is false — it is a thesis (an informal claim linking an intuitive notion to a formal one), not a provable theorem |
| 10 | 205 | Fitting An²+Bn+C to (1,10),(2,29),(3,58) gives A=5, B=4, C=1 (and correctly predicts 97 at n=4, matching the given data); steps(6) = 5(36) + 4(6) + 1 = 180 + 24 + 1, matching direct simulation |
