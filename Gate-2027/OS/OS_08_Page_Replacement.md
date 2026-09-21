# OS → Page Replacement

**GATE 2027 syllabus:** Operating System — memory management and virtual memory.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** Frames are initially empty. Every first reference to a page is a page fault. Unless stated otherwise, a fault happening while a free frame exists just loads the page.

---

## Questions

### Q1 · NAT · Fundamental
Reference string (14 references): `2 1 3 3 4 4 5 5 4 4 4 2 3 4`. The system has **3 frames** and uses **FIFO** replacement. The number of page faults is ______.

---

### Q2 · NAT · Fundamental
Reference string (16 references): `4 3 6 5 4 5 4 2 2 4 3 4 1 4 5 2`. The system has **4 frames** and uses **LRU** replacement. The number of page **hits** is ______.

---

### Q3 · MCQ · Standard
The number of frames is increased from 3 to 4 under **FIFO**. Which one of the following reference strings exhibits **Belady's anomaly**?

- (A) `6 5 1 3 1 4 2 2 5 4 2 3 3 1`
- (B) `1 5 3 5 4 2 2 2 1 1 2 6 5 4`
- (C) `3 6 2 5 3 6 1 3 6 2 4 4 1 4`
- (D) `3 2 6 3 1 6 5 6 5 5 4 1 6 3`

---

### Q4 · MSQ · Standard
Which of the following statements is/are TRUE?

- (A) LRU is a stack algorithm.
- (B) For the optimal (OPT) policy, the number of page faults never increases when the number of frames increases.
- (C) For every reference string, the number of page faults under FIFO never increases when the number of frames increases.
- (D) Under LRU, for any reference string and any prefix of it, the set of pages resident with m frames is always a subset of the set resident with m + 1 frames.

---

### Q5 · NAT · Standard
There are **3 frames**. The reference string is `4 6 2 2 2 5 1 4 4 6 4 1 3 1 3`. The OS approximates the optimal policy with a **limited lookahead**: on a page fault with all frames full at position i, it may look only at the next three references (positions i+1, i+2, i+3; fewer if the string ends). It evicts the resident page whose **first occurrence in this window is latest**; a resident page that does not occur in the window counts as later than any page that does occur. If several resident pages do not occur in the window, the one loaded earliest is evicted. The number of page faults is ______.

---

### Q6 · MSQ · Moderate
There are 3 frames and the reference string is `1 4 3 2 3 4 5 1 5 2 3 4`. The sequence of pages **evicted**, in order, was `1, 4, 3, 2, 5`. Which of the following policies could have produced it? (Ties in OPT may be broken arbitrarily.)

- (A) FIFO
- (B) LRU
- (C) OPT (optimal)
- (D) MRU (evict the most recently used page)

---

### Q7 · NAT · Moderate
**4 frames**, reference string `6 5 2 1 1 3 5 4 6 1 6 5 2 1 4 4`. (Number of page faults under FIFO) − (number of page faults under OPT) = ______.

---

### Q8 · NAT · Moderate
**4 frames** managed by the **clock (second-chance)** algorithm. The frames form a circle with a hand that initially points to frame 0; each frame has a reference bit.

- On a hit: the page's reference bit is set to 1; the hand does not move.
- On a fault: examine the frame under the hand. If it is empty or its reference bit is 0, load the new page there, set its reference bit to 1, and advance the hand by one frame. Otherwise clear its reference bit to 0, advance the hand, and repeat.

Reference string: `6 1 5 5 3 2 5 4 6 2 5 1 2 1 2 3`. The number of page faults is ______.

---

### Q9 · NAT · Hard
Reference string: `3 6 1 3 4 3 6 3 2 2 4 2 5 2 3 4`. Let m_L be the smallest number of frames for which **LRU** incurs only compulsory faults (faults equal to the number of distinct pages), and m_F the same for **FIFO**. Then m_F − m_L = ______.

---

### Q10 · NAT · Killer
There are 4 frames. The reference string is the sequence `1 2 3 4 5` repeated 6 times (30 references). (Page faults under LRU) − (page faults under OPT) = ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | 8 | Faults at 2, 1, 3, 4, 5, 2, 3, 4 |
| 2 | 7 | 9 faults (LRU) out of 16 |
| 3 | C | FIFO faults with 3 → 4 frames: (A) 9 → 8, (B) 9 → 9, (C) 9 → 10, (D) 8 → 7 |
| 4 | A, B, D | (C) is exactly Belady's anomaly |
| 5 | 7 | Faults at positions 1, 2, 3, 6, 7, 10, 13 (1-indexed); evictions 6, 2, 5, 4 |
| 6 | A, C | FIFO evicts 1, 4, 3, 2, 5; LRU evicts 1, 2, 3, 4, 1, 5; OPT works with a tie at the last two evictions |
| 7 | 4 | FIFO 11, OPT 7 |
| 8 | 10 | Clock 10 (FIFO 11, LRU 9 for the same string) |
| 9 | 2 | m_L = 4 (maximum LRU stack distance), m_F = 6 |
| 10 | 19 | LRU faults on all 30 references; OPT faults 11 |
