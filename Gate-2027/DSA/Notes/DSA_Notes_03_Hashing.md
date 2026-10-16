# DSA Notes 03 — Hashing (High-Yield Deep Dive)

> **Why this file exists:** "Hashing" is on the high-yield list and is nearly free marks: GATE questions are almost always (1) simulate a probe sequence, (2) count probes/comparisons, or (3) state a property of a collision-resolution scheme. Simulation is mechanical once the rules are exact. All practice answers verified by simulation.
>
> **Companion question bank:** `DSA_05_Hashing.md`. PW source: `../Reference/PW_Subject_Handbooks/PW_Programming_DS.txt` (§10).

---

## 0. The mental model: direct addressing → arithmetic compression

Direct addressing (key = index) is O(1) but needs a table as large as the key universe. Hashing compresses keys into a small table with an arithmetic function:

```
  slot = h(key)  →  m slots
```

Pigeonhole says collisions are **unavoidable** when keys > slots — so every scheme = (hash function) + (collision policy). GATE always tests the collision policy.

**Load factor:**

```
  λ = n / m          (keys / slots)
```

---

## 1. Hash functions

| Function | Form | Note |
|---|---|---|
| Division | `h(k) = k mod m` | m prime (or least-divisible) spreads poorly-behaved keys best — the GATE default |
| Mid-square | square k, take middle digits | good distribution, extra cost |
| Folding | split key, add parts | numeric keys of fixed width |
| Universal | randomly chosen from a family | provable expected behavior |

**A good h:** deterministic, fast, uniform (any key equally likely to land anywhere). *Queries ask: "why is a good hash function better than a poor one for a fixed table and key set?" → shorter average probe chains (fewer collisions), not "no collisions".*

---

## 2. Collision resolution

### 2.1 Separate chaining

Each slot holds a linked list of all keys hashing to it.

- λ may exceed 1; extra memory for pointers; **deletion is easy** (unlink the node).
- Average successful search in a chain ≈ 1 + λ/2; unsuccessful ≈ λ (simple models).

### 2.2 Open addressing ("closed hashing")

One key per slot; on collision, **probe** other slots via a deterministic sequence. When probing starts at `h(k)`, the probe sequence examines:

| Scheme | i-th probe | Weakness |
|---|---|---|
| **Linear probing** | `(h(k) + i) mod m` | **primary clustering** — runs of occupied slots grow; the run's tail is hit by multiple keys |
| **Quadratic probing** | `(h(k) + i²) mod m` | **secondary clustering** (same-h keys share the sequence); may fail to reach all slots |
| **Double hashing** | `(h1(k) + i·h2(k)) mod m` | none of the above; needs h2(k) ≠ 0 (and gcd(h2, m) = 1) |

**Facts to bank:**

- Open addressing needs λ ≤ 1; performance collapses as λ → 1.
- Linear probing expected probes (standard approximations):

```
  successful:   ½ · (1 + 1/(1−λ))
  unsuccessful: ½ · (1 + 1/(1−λ)²)
```

- **Quadratic probing reachability:** with offsets i² the sequence can revisit a small subset. For `m = 8`, i² mod 8 ∈ {0, 1, 4} only → a key whose probe path is 0-1-4 can never see slots 2,3,5,6,7. (Theorem: for **prime m and λ ≤ 1/2**, i² probes hit every slot; for powers of two use offsets i(i+1)/2.)
- **Deletion in open addressing:** clearing a slot can break probe chains for other keys. Standard fixes: tombstones (deleted markers that probing skips over but insertion can reuse) or rehash the cluster. This is why chaining's deletion is "easier" (PW).

---

## 3. Worked examples (the exact moves GATE expects)

**Linear probing walk.** `m = 7`, `h(k) = k mod 7`, insert 5, 12, 19, 26, 33:

```
  5 → slot 5             (1 probe)
  12 → 5 occ → 6         (2 probes)
  19 → 5,6 occ → 0       (3 probes)
  26 → 5,6,0 occ → 1     (4 probes)
  33 → 5,6,0,1 occ → 2   (5 probes)
  table: [19,26,33,-,-,5,12]        total probes = 15
```

**Double hashing walk.** `m = 10`, `h1 = k mod 10`, `h2 = 7 − (k mod 7)`, insert 43, 53, 63:

```
  43 → 3                                (1 probe)
  53 → h1=3 occ → step h2(53)=7−4=3 → 6 (2 probes)
  63 → h1=3 occ → step h2(63)=7−0=7 → (3+7) mod 10 = 0
```

**Quadratic walk.** `m = 10`, `h = k mod 10`, insert 10, 20, 30, 40: 0 → 0; 20: 0 occ, +1² → 1; 30: 0,1 occ, +2² → 4; 40: 0,1,4 occ, +3² → 9.

**Chaining sanity:** `h = k mod 10`, keys 5, 15, 25, 6, 16, 26, 7 → chains at 5 (len 3), 6 (len 3), 7 (len 1) → longest chain 3.

---

## 4. Traps & exam patterns

1. **λ ≤ 1 in open addressing** — an insertion *fails* (needs rehash) only when the table is full, but performance degrades far earlier.
2. **Probe counts count every slot examined**, including the final (successful) one. "Number of probes to insert k" = position of the first empty slot in the sequence.
3. **Quadratic ≠ square the whole thing:** offset is i² for the i-th probe (i = 0,1,2,…).
4. **Double hashing h2(k) must never be 0**, else the probe sequence sticks at h1(k).
5. **Deletion in open addressing breaks searches** unless tombstones/rehashing are used.
6. **Chaining's λ > 1 is normal**; "max chain length" questions = group keys by residue and take the largest group.
7. **Distribution ≠ addresses:** even a perfect hash function can't avoid collisions once n > m.

---

## 5. Practice set (verified by simulation)

> All tables initially empty unless stated. Probe counts include the final examined slot.

### Q1 · NAT · Fundamental
`m = 7`, `h(k) = k mod 7`, keys `5, 12, 19, 26, 33` inserted in this order with **linear probing**. The number of probes used to insert key **33** is ______.

### Q2 · NAT · Fundamental
For the Q1 scenario, the **total** number of probes across all five insertions is ______.

### Q3 · MCQ · Standard
Which statement about probing schemes is TRUE?

- (A) Linear probing suffers primary clustering; quadratic probing suffers secondary but not primary clustering.
- (B) Quadratic probing reaches every slot of the table for any table size.
- (C) Linear probing requires that the table be at most half full.
- (D) Double hashing is simply quadratic probing with a fixed step.

### Q4 · NAT · Standard
Separate chaining with `h(k) = k mod 10`. Keys `5, 15, 25, 6, 16, 26, 7` are inserted. The length of the longest chain is ______.

### Q5 · NAT · Standard
`h(k) = k mod 13` for the keys 1, 2, 3, …, 100 (chaining). The length of the longest chain is ______.

### Q6 · MSQ · Standard
Which of the following is/are TRUE?

- (A) In open addressing, the load factor can never exceed 1.
- (B) Chaining uses extra memory beyond the keys themselves (pointers).
- (C) Deleting a key is simpler in chaining than in open addressing.
- (D) Open addressing never exhibits clustering if the hash function is good.

### Q7 · NAT · Moderate
`m = 10`, `h1(k) = k mod 10`, `h2(k) = 7 − (k mod 7)`, **double hashing**. Keys `43, 53, 63` are inserted in order. Key 63 finally occupies slot ______.

### Q8 · NAT · Moderate
Linear probing with **load factor λ = 0.5**. The expected number of probes for an **unsuccessful** search, using the standard approximation `½(1 + 1/(1−λ)²)`, is ______.

### Q9 · NAT · Moderate
`m = 10`, `h(k) = k mod 10`, **quadratic probing** (offsets 0, 1², 2², …). Keys `10, 20, 30, 40` are inserted in order. Key 40 lands in slot ______.

### Q10 · NAT · Killer
Quadratic probing with `m = 8`, `h(k) = k mod 8`, offsets i² (i = 0, 1, 2, …). Starting from slot 0, the number of **distinct** slots the probe sequence can ever visit is ______.

---

## Answer Key

| Q | Answer | Why (condensed) |
|---|---|---|
| 1 | 5 | probes 5,6,0,1,2 |
| 2 | 15 | 1+2+3+4+5 |
| 3 | A | clustering taxonomy |
| 4 | 3 | slots 5 and 6 hold three keys each |
| 5 | 8 | 100 = 7·13 + 9 → max residue class has 8 |
| 6 | A, B, C | (D) false — clustering is inherent to the schemes |
| 7 | 0 | (3 + 7) mod 10 |
| 8 | 2.5 | ½(1 + 1/0.5²) |
| 9 | 9 | 0→1→4→9 |
| 10 | 3 | 0, 1, 4 only |

### Full solutions

**Q1.** Probe sequence for 33: `33 mod 7 = 5` (occupied), 6 (occupied), 0 (19), 1 (26), 2 (empty → insert). That's **5 probes**. *(Full table: [19,26,33,·,·,5,12].)*

**Q2.** Probe counts: 5 → 1; 12 → 2; 19 → 3; 26 → 4; 33 → 5. Total = **15**. (Note the clustering effect: as the run 5,6,0,1 grew, later keys paid more.)

**Q3.** **A**. (B) false — e.g., m = 8 reaches only {0,1,4} starting from 0 (unless m is prime and λ ≤ ½). (C) false — any λ < 1 works, with degrading performance. (D) false — quadratic uses i²; double hashing uses a key-dependent step.

**Q4.** Group by `k mod 10`: slot 5 → {5,15,25} (3), slot 6 → {6,16,26} (3), slot 7 → {7}. Longest = **3**.

**Q5.** Keys 1..100 in residue classes mod 13: 100 = 7·13 + 9, so residues 1..9 appear 8 times each (e.g., 5, 18, 31, 44, 57, 70, 83, 96), the rest 7 times. Longest chain = **8**.

**Q6.** A ✔ — one key per slot. B ✔ — each node stores a next pointer (PW's disadvantage list). C ✔ — unlink vs tombstones/rehash. D ✘ — linear probing's primary clustering exists regardless of the function's uniformity (keys that hash near each other build runs).

**Q7.** 63 hashes to 3 (occupied by 43). Step = `7 − (63 mod 7) = 7 − 0 = 7`. Probe 3 → 3+7 = 10 ≡ **0** (empty → insert). *(Verified: 43→3, 53→6, 63→0.)*

**Q8.** `½(1 + 1/(1−0.5)²) = ½(1 + 1/0.25) = ½(1 + 4) = 2.5`.

**Q9.** 10 → 0. 20: 0 occupied → +1 → 1. 30: 0,1 occupied → +4 → 4. 40: 0,1,4 occupied → +9 → **9**. *(Verified.)*

**Q10.** i² mod 8 for i = 0..7: 0, 1, 4, 1, 0, 1, 4, 1 → distinct values **{0, 1, 4}** → **3** distinct slots. Consequence: with slots 0, 1, 4 occupied, inserting another key with h(k) = 0 loops forever (needs rehashing) even though the table is only 3/8 full — the reachability theorem matters. *(Verified.)*

---

## 6. Next steps

1. Re-run Q1/Q7/Q9 by hand on paper tables — mechanical wins.
2. Memorize the two expected-probe formulas (successful/unsuccessful) with their shapes.
3. Practice reps: `DSA_05_Hashing.md`.
4. Continue to `DSA_Notes_04_Stacks_Queues.md`.
