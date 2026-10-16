# OS Notes 02 — Page Replacement, Virtual Memory & Thrashing (High-Yield Deep Dive)

> **Why this file exists:** the video's "Memory Management" bullet includes the replacement side of virtual memory — and GATE asks it almost every year (usually 1–2 marks, sometimes a 2-mark NAT trace). This file teaches you to *simulate and reason*, not memorize results: FIFO → Belady's anomaly → OPT → LRU → stack property → clock → thrashing & working set. Ends with a verified 10-question practice set.
>
> **Companion question banks:** `OS_08_Page_Replacement.md`, `OS_05_Memory_Management_Paging.md`.

---

## 0. Where we are in the story

You now have demand paging: pages live on disk and move into RAM frames on demand (previous notes file). The missing piece: **what happens when a fault occurs and all frames are full?**

```
  CPU references page P
        │
   valid bit = 1? ──yes──▶ access (hit)
        │no
   free frame available? ──yes──▶ load P (page fault, but easy)
        │no
   choose a victim by the REPLACEMENT POLICY  ◀── this file
        │
   victim dirty? → write back to disk
        │
   load P, update table, restart instruction
```

Every reference is either a **hit** or a **fault**. The exam gives you a reference string (sequence of page numbers) and a frame count; you report faults/hits. The whole file is about doing that faster and knowing which policies can do what.

---

## 1. The policies, from simplest to smartest

### 1.1 FIFO — evict the oldest-loaded page

Mental model: a queue. New page at the tail; victim at the head. No use of history.

**Worked trace:** 3 frames, string `2 3 2 1 5 2 4 5 3 2 5 2`

| ref | frames after | fault? |
|---|---|---|
| 2 | [2] | F |
| 3 | [2,3] | F |
| 2 | [2,3] | H |
| 1 | [2,3,1] | F |
| 5 | [3,1,5] (evict 2) | F |
| 2 | [1,5,2] (evict 3) | F |
| 4 | [5,2,4] (evict 1) | F |
| 5 | [5,2,4] | H |
| 3 | [2,4,3] (evict 5) | F |
| 2 | [2,4,3] | H |
| 5 | [4,3,5] (evict 2) | F |
| 2 | [3,5,2] (evict 4) | F |

**9 faults, 3 hits.**

### 1.2 Belady's anomaly — FIFO's fatal flaw

With FIFO, adding frames can *increase* faults. Classic string `1 2 3 4 1 2 5 1 2 3 4 5`:

- **3 frames → 9 faults**
- **4 frames → 10 faults** 😱

Intuition: FIFO ignores recency. A bigger cache keeps an old, about-to-be-evicted page around longer, which can force out a page that's about to be reused. (Stack algorithms never do this — §1.5.)

### 1.3 OPT (Belady's optimal) — evict the page used farthest in the future

Look *forward* in the reference string; evict the page whose next use is latest (or never). Gives the theoretical minimum faults. Not implementable offline, but the benchmark every other policy is graded against.

**In-exam technique:** scan forward only until you've found the next occurrence of each resident page. You don't need the whole future — just the minimum lookahead to disambiguate.

### 1.4 LRU — evict the least recently used page

Look *backward*; evict the page whose last use is oldest. Approximates OPT using the past. Implementable with a stack/order list.

**In-exam technique:** keep a recency list, most recent on top; on every access (hit or fault) move the page to the top; victim = bottom.

### 1.5 The stack property (why LRU/OPT never misbehave)

A policy is a **stack algorithm** if, for every reference string, the set of pages resident with `m` frames is always a **subset** of the set resident with `m+1` frames at the same point in time.

- LRU ✔ — the top `m` most-recently-used pages are the same regardless of `m`.
- OPT ✔ — same nested structure.
- **FIFO ✘** — hence Belady's anomaly.

Consequence: for stack algorithms, faults with `m+1` frames ≤ faults with `m` frames, always.

### 1.6 Second-chance / Clock — LRU on a budget

Perfect LRU needs ordering work per access. Clock approximates it with one **reference bit** per frame:

- Frames in a circle, a "hand" points at the oldest candidate.
- On access → hardware sets the page's reference bit to 1.
- On a fault: scan from the hand — if the bit is 1, clear it and advance (the page gets a "second chance", acting like a free pass); if 0, evict that page, load the new one, set its bit = 1, advance the hand.

**Enhanced second chance** uses (reference bit, dirty bit): avoid evicting dirty pages when possible because writing back is expensive. Priority order: `(0,0)` clean & unused > `(0,1)` dirty & unused > `(1,0)` > `(1,1)`.

### 1.7 The rest (know the idea, not a trace)

| Policy | Rule | Why not used |
|---|---|---|
| LFU | evict fewest past uses | chokes on pages that were once hot |
| MFU | evict most past uses | counter-intuitive but sound given locality |
| Aging | periodic right-shift of a counter built from reference bits | a practical LFU-ish LRU approximation |

---

## 2. Counting faults fast — exam heuristics

1. **First touch = fault**: every page appearing for the first time inside a prefix without free frames cannot be a hit (except with OPT, oddly — no: under OPT new pages also fault at most once per load). Count distinct pages as a lower bound: no policy can beat `#distinct pages`.
2. **If frames ≥ distinct pages** in the entire string: after the warm-up, zero faults; each page faults once → `faults = #distinct pages` under every policy.
3. **Cyclic patterns are LRU poison.** With 5 cyclically-referenced pages and only 4 frames, LRU faults on *every* reference — each page is evicted right before it's needed. (You'll hit this in practice Q9.) Same for FIFO. OPT with 4 frames handles it in far fewer faults.
4. OPT fault count = "new page" faults + (evictions forced where the *next* reference forces reloading a previous page), find via lookahead.
5. Do traces with a table: one column per frame slot; note evictions with arrows; tally. Neatness prevents off-by-one answers — GATE answers hinge on the exact string and initial emptiness.

---

## 3. Thrashing and the working-set model

**Thrashing:** the system spends more time paging than executing. Cause: **degree of multiprogramming too high** — each process's resident set shrinks below its true working set, so nearly every instruction faults.

```
  CPU util
    ^
100%│        ┌─────────── thrashing begins
    │      ┌─┘
    │    ┌─┘
    │  ┌─┘
    └──┴────────────────────────▶ degree of multiprogramming
```

**Cure:** reduce multiprogramming (swap a process out), or detect with a **page-fault frequency (PFF)** threshold per process and give the hungrier process more frames / swap out when its PFF is too high.

**Working set** `W(t, Δ)` = the set of distinct pages referenced in the window of the last Δ references (or last Δ time units). Its size is the frames a process currently *needs*.

- If Σ working sets of all processes > available frames → thrashing is imminent.
- OS policy: admit a process only if it fits, else suspend it.

**Worked example:** string `2 6 1 5 7 7 7 7 5 1 6 2 3 4 1 2 3 4 4 4`, Δ = 5.

| t | refs t−4 … t | W(t,5) |
|---|---|---|
| 5 | 2 6 1 5 7 | {2,6,1,5,7} → 5 |
| 15 | 6 2 3 4 1 | {1,2,3,4,6} → 5 |
| 16 | 2 3 4 1 2 | {1,2,3,4} → 4 |
| 20 | 2 3 4 4 4 | {2,3,4} → 3 |

Max window size = **5** → run this process only if ≥ 5 frames can be spared.

**Local vs global replacement:** local = each process evicts only from its own frames (isolation — a thrashing process can't hurt others, but can't borrow either); global = evict from anyone's frames (better throughput typically, but a runaway process can drag the system into thrashing).

---

## 4. Practice set (verified by simulation)

### Q1 · NAT · Fundamental
Reference string `2 3 2 1 5 2 4 5 3 2 5 2`, **3 frames**, **FIFO** (frames initially empty). The number of page faults is ______.

---

### Q2 · NAT · Fundamental
Same string, same 3 frames, **LRU**. The number of page faults is ______.

---

### Q3 · NAT · Standard
Same string, same 3 frames, **OPT**. The number of page faults is ______.

---

### Q4 · MCQ · Standard
Which statement about Belady's anomaly is TRUE?

- (A) It can occur under LRU when the reference string is cyclic.
- (B) It is the phenomenon where increasing the number of frames increases page faults, and it can occur under FIFO.
- (C) It can occur under OPT if the string is long enough.
- (D) It is caused by counting a fault twice when the victim page is dirty.

---

### Q5 · MSQ · Standard
Which of the following statements is/are TRUE?

- (A) LRU and OPT are stack algorithms; FIFO is not.
- (B) For a stack algorithm, the pages resident with m frames are always a subset of the pages resident with m+1 frames, at every point in the string.
- (C) The optimal (OPT) policy minimizes the number of page faults for every reference string among all policies.
- (D) Any policy that is implementable online must be a stack algorithm.

---

### Q6 · NAT · Standard
Reference string `1 2 3 4 1 2 5 1 2 3 4 5`. With FIFO, faults with 3 frames = 9. The number of faults with **4 frames** under FIFO is ______.

---

### Q7 · NAT · Moderate
Reference string `2 6 1 5 7 7 7 7 5 1 6 2 3 4 1 2 3 4 4 4`. For the working-set model with window Δ = 5, the **maximum** working-set size over all t is ______ (also: the minimum frames that guarantee no thrashing for this string).

---

### Q8 · MSQ · Moderate
Which of the following statements about thrashing is/are TRUE?

- (A) It is caused by raising the degree of multiprogramming beyond what the available frames can support.
- (B) Under global replacement, a page-faulting process can steal frames from other processes, potentially spreading thrashing.
- (C) The working-set model prevents thrashing by admitting processes only when their working sets fit.
- (D) Increasing the page size always eliminates thrashing.

---

### Q9 · NAT · Hard
The reference string is `1 2 3 4 5 1 2 3 4 5 1 2 3 4 5 6` (16 references), 4 frames, LRU. The number of page **hits** is ______.

---

### Q10 · NAT · Killer
A 100 × 100 integer array `A` is stored **row-major**, and one page holds exactly one full row (100 integers). The program accesses **all elements in column-major order** (column 0 top-to-bottom, then column 1, …): `for j = 0..99: for i = 0..99: A[i][j]`. The system has **50 frames** and uses LRU. The total number of page faults is ______.

---

## Answer Key

| Q | Answer | Why (condensed) |
|---|---|---|
| 1 | 9 | FIFO trace above |
| 2 | 7 | LRU: evict oldest last use |
| 3 | 6 | OPT: lookahead evictions are nearly free |
| 4 | B | Definition; FIFO is the classic victim |
| 5 | A, B, C | (D) false — clock/2nd-chance is online, not a stack algorithm |
| 6 | 10 | Belady's anomaly: more frames, more faults |
| 7 | 5 | Max distinct pages in any 5-reference window |
| 8 | A, B, C | (D) false — page size doesn't fix over-multiprogramming |
| 9 | 0 | Cyclic 1..5 with 4 frames: every access faults under LRU |
| 10 | 10000 | Column-major touches 100 distinct pages per column; 50 < 100 → fault every access |

### Full solutions

**Q1.** Trace from §1.1: fault at every ref except the two hits at refs 3 and 8 → **9 faults**.

**Q2.** LRU recency list (top = most recent):
- 2F [2] · 3F [3,2] · 2H [2,3] · 1F [1,2,3] · 5F evict bottom 3 → [5,1,2] · 2H [2,5,1] · 4F evict 1 → [4,2,5] · 5H [5,4,2] · 3F evict 2 → [3,5,4] · 2F evict 4 → [2,3,5] · 5H [5,2,3] · 2H.
- Faults = 7. ✔ (LRU beats FIFO here: 7 vs 9.)

**Q3.** OPT: on fault, evict resident page whose next use is latest:
- 2F, 3F, 2H, 1F [2,3,1]
- 5F: next uses — 2@6, 3@9, 1@never → evict 1 → [2,3,5]
- 2H, 4F: next — 2@10, 3@9, 5@8 → evict 2 → [4,3,5]... (continue similarly) → **6 faults**.

**Q4.** B. (A) LRU is a stack algorithm — immune. (C) OPT is also immune. (D) Nonsense.

**Q5.** A ✔, B ✔ (definition), C ✔ (optimal by construction), D ✘ — many online approximations (clock, LFU, aging) are not stack algorithms, and that's an accepted trade-off.

**Q6.** With 4 frames FIFO on `1 2 3 4 1 2 5 1 2 3 4 5`: faults at 1,2,3,4,5,1,2,3,4,5 → **10**. Fewer frames (9) is *better* here — the anomaly.

**Q7.** Max window = the window `2 6 1 5 7` (t=5) → {2,6,1,5,7} = **5**. Later windows shrink (the `7 7 7 7` run collapses to one page). Answer 5.

**Q8.** A ✔ — that's the definition of the cause. B ✔ — global replacement has no process isolation. C ✔ — that's the working-set admission policy. D ✘ — bigger pages reduce page-table size, not the frame shortage; faults can still stampede.

**Q9.** Walk it: 1F,2F,3F,4F [1,2,3,4] · 5F evict 1 → [2,3,4,5] · 1F evict 2 → [3,4,5,1] · 2F evict 3 · 3F evict 4 · 4F evict 5 · 5F evict 1 · 1F evict 2 · 2F evict 3 · 3F evict 4 · 4F evict 5 · 5F evict 1 · 6F evict 2.
Every single reference misses: pattern cycles through 5 pages with only 4 frames, and LRU evicts exactly the page needed next in the cycle. **Hits = 0.** (This is the classic worst case; note OPT with 4 frames needs only 8 faults — stack property still holds, LRU is just pessimal *for this string*.)

**Q10.** Row-major layout → **each page = one row**. Column-major traversal visits rows `0,1,…,99` for column 0 — **100 distinct pages** in quick succession. With only 50 frames, by the time row 51 is touched, row 0's page has been evicted; the running page is always the LRU victim candidate. Net effect: **every access to a new row's page faults**, and every column re-touches all 100 rows → `100 columns × 100 row-faults = 10,000` faults. (Contrast: row-major traversal with 50 frames → 1 fault per row, ~100 total. Same data, same frames — **access order decides everything**. This is the deep lesson of locality.)

---

## 5. Next steps

1. Reproduce Q1–Q3 traces on paper; target < 3 minutes for a 12-reference trace.
2. Do `OS_08_Page_Replacement.md`'s 10 questions — they add second-chance, MRU and mixed-policy variants.
3. Continue to `OS_Notes_03_Process_Synchronization.md`.
