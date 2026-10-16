# COA Notes 01 — Instruction Pipelining & Hazards (High-Yield Deep Dive)

> **Why this file exists:** "Instruction Pipelining" is on the high-yield list, and it's one of COA's most formula-friendly topics: cycles, time, speedup, efficiency, throughput and hazard penalties are all arithmetic once the model is clear. Built using the PW GATE handbook's framing (parallel processing levels, performance formulas, stalls, branch handling) plus derivation-first explanations and a verified 10-question practice set.
>
> **Companion question bank:** `COA_06_Instruction_Pipelining_Hazards.md`. PW source: `../Reference/PW_Subject_Handbooks/PW_COA.txt` (§7).

---

## 0. The mental model: an assembly line

Before pipelining, one instruction owns the whole datapath for its entire execution:

```
  Instruction i:  [ F | D | E | M | W ]   ← non-overlapped, 5 units of time
  Instruction i+1:                         (starts only after the previous finishes)
```

Pipelining lets **different instructions occupy different stages at the same time** — exactly like a car wash: each car moves one station per cycle, and one car exits per cycle once the line is full.

```
  Time →     1    2    3    4    5    6    7    8    9
  I1:        F    D    E    M    W
  I2:             F    D    E    M    W
  I3:                  F    D    E    M    W
  I4:                       F    D    E    M    W
  I5:                            F    D    E    M    W
             ─────────────────────────────────────────
  exits:                                    I1   I2   I3   ...
```

Two facts fall straight out of the diagram:

1. **Ideal cycles for n instructions in a k-stage pipeline = k + (n − 1)** (the first instruction takes k cycles; every additional one adds exactly 1).
2. **Ideal throughput = 1 instruction per cycle** in steady state → ideal CPI = 1.

> PW framing: "parallel processing" = simultaneous data-processing for speed; pipelining is the instruction-level version. Throughput = work done per unit time; pipelining increases throughput, not the latency of a single instruction (each instruction still takes k cycles end-to-end).

---

## 1. The clock and the stage delay (where most marks hide)

A pipeline's clock must be slow enough for the **slowest stage**, plus the **latch/register delay** between stages:

```
  tp (one cycle) = max(stage delays) + latch delay × (1)
```

- Uneven stages waste time in fast stages — the clock is set by the slowest.
- Deeper pipelines have more latch overhead: `k × latch_delay` accumulates. (This is why "more stages = always better" is false.)

**Worked example.** Stages take 3, 4, 2, 5 ns; latch delay 1 ns.

```
  tp = max(3,4,2,5) + 1 = 5 + 1 = 6 ns
```

For n = 100 instructions, k = 4:

```
  cycles = k + (n−1) = 4 + 99  = 103
  time   = 103 × 6 ns          = 618 ns
```

(A hypothetical non-pipelined machine would take `(3+4+2+5) = 14 ns` per instruction → `100 × 14 = 1400 ns`.)

---

## 2. Performance formulas (derive, don't memorize)

| Quantity | Formula | Ideal limit |
|---|---|---|
| Cycles (n instrs, k stages, no stalls) | `k + (n − 1)` | — |
| Time | `(k + n − 1) × tp` | — |
| Speedup | `S = n·k / (k + n − 1)` | `S → k` as n → ∞ |
| Efficiency | `E = S/k = n / (k + n − 1)` | `E → 1` (100%) |
| Throughput | `f = n / ((k + n − 1) × tp)` | `f → 1/tp` (one instr/cycle) |

**Why the speedup formula:** non-pipelined time for n instructions = `n × k × tp` (one instruction occupies all k stages); pipelined time = `(k + n − 1) × tp`. Divide.

```  S = n·k·tp / ((k+n−1)·tp) = nk/(k+n−1)
```

- **k = 5, n = 100:** cycles = 104, `S = 500/104 = 4.81`, `E = 100/104 = 96.15%`.
- **k = 6, n = 100 (PW example):** cycles = 105 — as n grows, `k + n − 1 ≈ n` and both S and E approach the ideal.

> **Sanity check that saves marks:** S < k always (for finite n); E < 1 always. If your answer violates this, you've added a cycle somewhere.

---

## 3. Hazards — when the ideal assumptions break

A **hazard** is any situation where the next instruction cannot execute in its intended clock cycle. It forces a **stall** (bubble): the pipeline inserts idle cycles.

### 3.1 Structural hazards — resource conflict

Two instructions want the same hardware in the same cycle. Classic example: a single memory used for both instruction fetch (F) and operand access (M).

Fixes: separate I-cache and D-cache (split caches), multiple register-file ports, adder for PC increment, register renaming.

### 3.2 Data hazards — depend on data

```
  I1:  ADD R1, R2, R3
  I2:  SUB R4, R1, R5     ← needs R1, but I1 hasn't written it yet (RAW)
```

| Type | Pattern | Note |
|---|---|---|
| RAW (read-after-write) | I2 reads what I1 writes | the classic true dependency |
| WAR (write-after-read) | I2 writes what I1 reads | only in out-of-order execution |
| WAW (write-after-write) | both write same register | only in out-of-order execution |

**Fixes (in order of power):**

1. **Forwarding / bypassing** — route the ALU result directly to the next instruction's ALU input without waiting for the register write. Handles ALU-ALU chains *and* ALU-to-store, usually with 0 stalls.
2. **Load-use forwarding fails by 1 cycle:** a load produces data only after M, so the dependent instruction needs **exactly 1 stall** even with forwarding. This is the single most-tested data-hazard fact.
3. **Compiler scheduling / software NOPs (PW):** the compiler reorders independent instructions into delay slots or inserts NOPs; simpler hardware, larger code.
4. **Hardware interlock:** stall the dependent instruction until data is ready.

### 3.3 Control hazards — branches

The pipeline doesn't know the branch outcome/target until some stage decides it (ID, E, ...). Everything fetched meanwhile may be wrong.

**Branch penalty** = number of cycles wasted per branch = (stage where target/outcome is known) − (fetch stage) [− any delay slots filled].

```
  Target known in ID (stage 2)  → penalty ≈ 1
  Target known in EX (stage 3)  → penalty ≈ 2
  (deeper pipelines → larger penalty)
```

**Fixes:**

1. **Stalling** until the decision is known — simple, expensive.
2. **Early branch resolution:** compute the target in ID, add a separate adder — cuts the penalty.
3. **Delayed branch (PW):** the compiler fills the "branch delay slots" with instructions that **execute regardless of whether the branch is taken** (they come from before the branch or from the target). Hardware executes them unconditionally — so they must be *safe*.
4. **Branch prediction:**
   - *Static:* predict not-taken / taken / backward-taken-forward-not-taken.
   - *Dynamic:* 1-bit or 2-bit saturating counters, BHT/BTB; misprediction → flush.
5. **Instruction queue / prefetch:** keeps fetching past the branch; combined with prediction.

---

## 4. Adding stalls to the cycle count (the exam arithmetic)

```  Actual cycles = (k + n − 1) + total stall cycles
                  = (k + n − 1) + Σ (frequency × penalty) per instruction class
```

**Worked example.** k = 5, n = 300, 25% of instructions are branches with a 2-cycle penalty each:

```
  base         = 5 + 299        = 304
  stall cycles = 0.25 × 300 × 2 = 150
  total        = 454 cycles
```

Notice: stalls don't stretch the *first* instruction's k cycles; they only add bubbles, and in a **stall-per-branch** model every branch contributes its penalty (a simpler but GATE-standard assumption — always check whether the question says "taken" or "mispredicted").

**Effective CPI of a pipelined machine:**

```
  CPI = 1 + (stall cycles per instruction)      [ideal CPI = 1]
```

---

## 5. Traps & exam patterns

1. **"Time for the FIRST instruction" vs "time for n instructions":** time = cycles × tp with cycles = k + n − 1. Don't use n·k·tp unless it's the *non-pipelined* machine.
2. **Speedup CEILING is k, not "n times better"** — and efficiency can never exceed 1.
3. **Latch delay is added to every cycle** — the formula is max(stage) + latch, applied for all k stages (not k×latch per instruction, but latch is inside tp).
4. **Load-use = 1 stall (with forwarding); ALU-ALU = 0** with forwarding.
5. **Branch penalty counts from fetch of the branch**, and only if the branch path isn't already correctly supplied.
6. **Uneven stages:** total *latency* of one instruction = Σ stages (+ latches), but clock = max stage + latch — don't mix them.
7. When a question gives "stalls per instruction" directly, multiply by n and add to `k + n − 1` — no other correction is needed.

---

## 6. Practice set (verified by computation)

### Q1 · NAT · Fundamental
A processor has a 5-stage pipeline. For 100 instructions with no stalls, the total number of clock cycles is ______.

### Q2 · NAT · Fundamental
Same setup, cycle time tp = 2 ns. The total execution time of the 100 instructions is ______ ns.

### Q3 · NAT · Standard
Same setup (k = 5, n = 100). The speedup over a non-pipelined implementation is ______ (2 decimal places).

### Q4 · NAT · Standard
Same setup. The pipeline efficiency, in percent, is ______ (2 decimal places).

### Q5 · MSQ · Standard
Which of the following is/are TRUE?

- (A) A pipeline stall inserts idle (bubble) cycles and reduces the achieved speedup.
- (B) Computing a branch target earlier in the pipeline reduces the branch penalty.
- (C) In a delayed-branch scheme, the instructions in the delay slots execute regardless of whether the branch is taken.
- (D) Increasing the number of pipeline stages invariably improves performance.

### Q6 · NAT · Moderate
A 5-stage pipeline executes 300 instructions. 25% of the instructions are branches, each causing a 2-cycle stall. The total execution time in clock cycles is ______.

### Q7 · NAT · Moderate
A 4-stage pipeline has stage delays 3, 4, 2 and 5 ns, with a latch delay of 1 ns between stages. The pipeline clock period is ______ ns.

### Q8 · NAT · Moderate
With the Q7 machine, the time to execute 100 instructions (no stalls) is ______ ns.

### Q9 · NAT · Hard
With the Q7 machine, the steady-state instruction completion rate is approximately ______ million instructions per second (1 MIPS = 10⁶ instructions/s; 2 decimal places).

### Q10 · NAT · Killer
A 5-stage pipeline has stage delays 2, 3, 2, 4 and 2 ns, and a latch delay of 0.5 ns. For 200 instructions with no stalls, the total execution time is ______ ns.

---

## Answer Key

| Q | Answer | Why (condensed) |
|---|---|---|
| 1 | 104 | 5 + 99 |
| 2 | 208 | 104 × 2 ns |
| 3 | 4.81 | 5×100/104 |
| 4 | 96.15 | 100/104 × 100 |
| 5 | A, B, C | (D) false — latches/hazards grow with k |
| 6 | 454 | 304 + 0.25×300×2 |
| 7 | 6 | max(3,4,2,5) + 1 |
| 8 | 618 | (4 + 99) × 6 |
| 9 | 166.67 | 1/6 ns = 166.67 × 10⁶ instrs/s |
| 10 | 918 | tp = 4.5; (5 + 199) × 4.5 |

### Full solutions

**Q1.** Ideal cycles = `k + (n−1) = 5 + 99 = 104`. (Space-time diagram: rows fill at cycle 5, then one instruction finishes every cycle: 5 + 95 exits = 104.)

**Q2.** `104 cycles × 2 ns = 208 ns`.

**Q3.** Non-pipelined: each instruction takes 5 cycles → `5 × 100 = 500` cycles. Pipelined: 104. `S = 500/104 = 4.8077 → 4.81`. (Formula check: `nk/(k+n−1) = 500/104` ✔.)

**Q4.** `E = S/k = 4.81/5 = 96.15%` — or directly `n/(k+n−1) = 100/104`.

**Q5.** A ✔ — that's the definition of a stall. B ✔ — smaller distance between fetch and resolution = smaller penalty. C ✔ — the defining rule of delayed branching (which is why the compiler must only put safe instructions there). D ✘ — latch overhead scales with k, and hazards/penalties worsen; deep pipelines also cost power and area.

**Q6.** No-stall baseline: `5 + 299 = 304` cycles. Branches: `0.25 × 300 = 75` branches × 2 cycles = 150 stall cycles. Total `304 + 150 = 454`.

**Q7.** The clock accommodates the slowest stage plus latch: `max(3,4,2,5) + 1 = 6 ns`. (Other stages idle 1–4 ns worth of slack each cycle — this is why balanced stages matter.)

**Q8.** Cycles = `4 + 99 = 103`. Time = `103 × 6 = 618 ns`. (Compare non-pipelined: `100 × 14 = 1400 ns` → speedup ≈ 2.27.)

**Q9.** In steady state the pipeline finishes 1 instruction per cycle: rate = `1/tp = 1/6ns = 166.67 × 10⁶/s = 166.67 MIPS`. (Cross-check: `n / time = 100/618 ns = 161.8 MIPS` — the small gap is the fill/drain overhead; steady state ≈ 166.67.)

**Q10.** `tp = max(2,3,2,4,2) + 0.5 = 4.5 ns`. Cycles = `5 + 199 = 204`. Time = `204 × 4.5 = 918 ns`. (Trap: using Σstages = 13 ns or forgetting the latch on the 2 ns stages; the clock is set only by the slowest stage + latch.)

---

## 7. Next steps

1. Redraw Q6's timeline by hand: 300 instructions, mark where each branch's 2 bubbles land.
2. Practice reps: `COA_06_Instruction_Pipelining_Hazards.md`.
3. Continue to `COA_Notes_02_Cache_Memory.md`.
