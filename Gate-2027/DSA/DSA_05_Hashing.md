# Data Structures & Algorithms → Hashing

**GATE 2027 syllabus:** Algorithms — searching, sorting, hashing.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** Table size m, load factor α = n/m. Linear probing: probe sequence h(k), h(k)+1, h(k)+2, … (mod m). Quadratic probing: h(k), h(k)+1², h(k)+2², … (mod m). Double hashing: h₁(k), h₁(k)+h₂(k), h₁(k)+2h₂(k), … (mod m). "Number of probes" for a successful insertion counts the slot finally used as 1 probe (an immediate empty slot = 1 probe).

---

## Questions

### Q1 · MCQ · Fundamental
The primary advantage of a good hash function over a poor one, for a fixed table size and key set, is that it

- (A) always uses less memory
- (B) spreads keys more evenly across slots, reducing collisions
- (C) guarantees O(1) worst-case lookup regardless of collisions
- (D) eliminates the need for collision resolution entirely

---

### Q2 · NAT · Fundamental
A hash table has 11 slots (indices 0–10) and uses separate chaining. The load factor when 33 keys have been inserted is ______.

---

### Q3 · MSQ · Standard
Keys 23, 44, 13, 88, 17, 58 are inserted **in this order** into an initially empty table of size m = 11 using h(k) = k mod 11 with **linear probing**. Which of the following (key, final slot) pairs is/are CORRECT?

- (A) (23, 1)
- (B) (44, 0)
- (C) (88, 4)
- (D) (17, 6)

---

### Q4 · NAT · Standard
Using the scenario of Q3 (keys 23, 44, 13, 88, 17, 58, table size 11, h(k) = k mod 11, linear probing), the total number of probes used across all 6 insertions (each insertion's probe count is the number of slots examined, including the one finally used) is ______.

---

### Q5 · MCQ · Standard
Under **open addressing** (any probing scheme), when a key is deleted from a slot, that slot

- (A) can simply be marked empty, since future searches will always still work correctly
- (B) must be marked with a special "deleted" marker (not plain "empty"), so that searches for other keys that probed past this slot still continue correctly
- (C) causes no issue, because open addressing never needs deletion support
- (D) must trigger a full table rehash immediately

---

### Q6 · NAT · Moderate
Keys 23, 44, 13, 88, 17, 58 are inserted, **in this order**, into an initially empty table of size m = 13 using h(k) = k mod 13 with **quadratic probing** (offsets 0, 1², 2², 3², …, mod 13). The final slot occupied by key 58 is ______.

---

### Q7 · MSQ · Moderate
Which of the following statements about hashing is/are TRUE?

- (A) In separate chaining, the expected number of probes for a successful search is approximately 1 + α/2, where α is the load factor.
- (B) In separate chaining, the expected number of probes for an unsuccessful search is approximately α.
- (C) Open addressing requires the load factor α to always stay strictly less than 1.
- (D) A perfect hash function guarantees no collisions for a specific, fixed, known set of keys, but such a function generally cannot be found efficiently for an arbitrary, unknown key set in advance.

---

### Q8 · NAT · Moderate
Keys 23, 44, 13, 88, 17, 58 are inserted, in this order, into an initially empty table of size m = 13 using h₁(k) = k mod 13 and h₂(k) = 1 + (k mod 7) with **double hashing**. The total number of probes used across all 6 insertions is ______.

---

### Q9 · MSQ · Hard
Which of the following statements about open-addressing probe sequences is/are TRUE?

- (A) Linear probing suffers from primary clustering (long runs of occupied slots tend to grow longer).
- (B) Quadratic probing eliminates primary clustering but can still suffer from secondary clustering (keys with the same initial probe position follow the same probe sequence).
- (C) Double hashing, with a well-chosen second hash function, comes closest among the three schemes to the ideal of uniform hashing.
- (D) Quadratic probing with any step function is guaranteed to probe every slot in the table before repeating, for any table size m.

---

### Q10 · NAT · Killer
Using **linear probing** with h(k) = k mod 11 on a table of size m = 11, the expected number of probes for an **unsuccessful** search, using the standard approximation ½(1 + 1/(1−α)²), is computed for a table currently holding 8 keys (load factor α = 8/11). The value, rounded to two decimal places, is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | B | Even spread reduces collision chains |
| 2 | 3 | 33 / 11 |
| 3 | A, B, D | Verified by simulating the insertions: table becomes [44,23,13,88,58,_,17,_,_,_,_], so (23→1), (44→0), (17→6) are correct; 88 lands at slot 3, not 4 |
| 4 | 10 | Individual probe counts (verified by simulation): 1,1,1,4,1,2 |
| 5 | B | Plain "empty" would wrongly terminate a probe sequence early for other keys |
| 6 | 6 | Verified by simulating quadratic probing for this exact sequence |
| 7 | A, B, C, D | All four are standard, correct facts about hashing |
| 8 | 7 | Individual probe counts (verified by simulation): 1,1,1,2,1,1 |
| 9 | A, B, C | (D) is false — quadratic probing can fail to probe all slots for many table sizes; a prime table size with a specific quadratic form is needed for that guarantee |
| 10 | 7.22 | ½(1 + 1/(1 − 8/11)²) = ½(1 + 1/(3/11)²) = ½(1 + 121/9) ≈ ½ × 14.44 |
