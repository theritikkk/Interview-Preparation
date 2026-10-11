# Engineering Mathematics → Probability Basics, Conditional Probability & Bayes' Theorem

**GATE 2027 syllabus:** Engineering Mathematics — Probability and Statistics: conditional probability and Bayes' theorem.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** P(A|B) = P(A∩B)/P(B), defined when P(B) > 0. Events A and B are independent iff P(A∩B) = P(A)P(B). Bayes' theorem: P(Hᵢ|E) = P(Hᵢ)P(E|Hᵢ) / Σⱼ P(Hⱼ)P(E|Hⱼ), for a partition of hypotheses H₁, …, Hₙ.

---

## Questions

### Q1 · MCQ · Fundamental
For any two events A and B (with P(B) > 0), P(A|B) is defined as

- (A) P(A) × P(B)
- (B) P(A∩B) / P(B)
- (C) P(A∪B) / P(B)
- (D) P(A) + P(B) − P(A∩B)

---

### Q2 · NAT · Fundamental
P(A) = 0.3, P(B) = 0.4, P(A∩B) = 0.1. The value of P(A∪B) is ______  (give as a decimal).

---

### Q3 · MSQ · Standard
Which of the following statements is/are TRUE?

- (A) If A and B are independent, then P(A|B) = P(A).
- (B) If A and B are mutually exclusive (and both have positive probability), they cannot be independent.
- (C) P(A|B) + P(Aᶜ|B) = 1, where Aᶜ is the complement of A.
- (D) Independence of A and B implies A and B are mutually exclusive.

---

### Q4 · NAT · Standard
Two dice are rolled. Given that the sum of the two numbers shown is 8, the probability that **both** dice show even numbers is ______  (give as a fraction, e.g. 3/5).

---

### Q5 · MCQ · Standard
A bag A has 3 red and 2 blue balls; bag B has 4 red and 1 blue ball. A bag is chosen at random (probability 1/2 each), then one ball is drawn from it. Given that the ball drawn is red, the probability it came from bag A is

- (A) 1/2
- (B) 3/5
- (C) 3/7
- (D) 4/7

---

### Q6 · NAT · Moderate
A diagnostic test for a disease has sensitivity 99% (P(positive | disease) = 0.99) and specificity 98% (P(negative | no disease) = 0.98). The disease has a prevalence of 1% in the population (P(disease) = 0.01). Given that a randomly selected person tests **positive**, the probability (rounded to two decimal places) that they actually have the disease is ______.

---

### Q7 · MSQ · Moderate
Which of the following statements about Bayes' theorem is/are TRUE?

- (A) It requires the hypotheses H₁, …, Hₙ to form a partition of the sample space (mutually exclusive and exhaustive).
- (B) The denominator Σⱼ P(Hⱼ)P(E|Hⱼ) equals P(E), the total (marginal) probability of the evidence.
- (C) A very rare condition (low prior probability) can still have a low posterior probability even after a positive test result with high sensitivity and specificity, if the false-positive rate affects a much larger healthy population.
- (D) Bayes' theorem can only be applied when there are exactly two hypotheses.

---

### Q8 · NAT · Moderate
An urn contains 5 red and 3 blue balls. Two balls are drawn **without replacement**. Given that the first ball drawn is red, the probability that the second ball drawn is also red is ______  (give as a fraction, e.g. 4/7).

---

### Q9 · MSQ · Hard
Three machines M1, M2, M3 produce respectively 30%, 40% and 30% of a factory's output. Their defect rates are 2%, 4% and 3% respectively. Which of the following statements is/are TRUE?

- (A) The overall probability that a randomly selected item is defective is 0.031.
- (B) Given that an item is defective, the probability it came from M2 is greater than the probability it came from M1.
- (C) Given that an item is defective, the probability it came from M2 is 16/31.
- (D) Given that an item is **not** defective, the probability it came from M1 is exactly 0.30 (unchanged from the prior), since non-defectiveness gives no information at all about the source machine.

---

### Q10 · NAT · Killer
Three hypotheses H1, H2, H3 have prior probabilities 0.3, 0.3 and 0.4 respectively, forming a partition of the sample space. An evidence event E has P(E|H1) = 0.1, P(E|H2) = 0.2, P(E|H3) = 0.3. Given that E has occurred, the posterior probability P(H2|E), expressed as a fraction in lowest terms, has denominator ______  (i.e., if P(H2|E) = a/b in lowest terms, give the value of b).

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | B | Definition of conditional probability |
| 2 | 0.6 | 0.3 + 0.4 − 0.1 |
| 3 | A, B, C | (D) is false — independence and mutual exclusivity are generally incompatible (unless one event has probability 0) |
| 4 | 3/5 | Sum=8 outcomes: (2,6),(3,5),(4,4),(5,3),(6,2); both-even: (2,6),(4,4),(6,2) → 3 of 5 |
| 5 | C | P(A|red) = (1/2 × 3/5) / (1/2 × 3/5 + 1/2 × 4/5) = (3/10)/(7/10) |
| 6 | 0.33 | P(disease|+) = (0.01×0.99)/(0.01×0.99 + 0.99×0.02) = 1/3 |
| 7 | A, B, C | (D) is false — Bayes' theorem works for any finite (or countable) partition, not just two hypotheses |
| 8 | 4/7 | 4 red remain out of 7 total remaining balls |
| 9 | A, B, C | (D) is false — non-defectiveness DOES shift the posterior (away from the prior), since machines have different defect rates |
| 10 | 7 | P(E) = 0.3×0.1+0.3×0.2+0.4×0.3 = 0.21; P(H2\|E) = 0.06/0.21 = 2/7, so the denominator in lowest terms is 7 |
