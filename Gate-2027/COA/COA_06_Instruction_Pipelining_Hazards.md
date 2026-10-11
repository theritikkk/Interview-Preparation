# Computer Architecture → Instruction Pipelining & Pipeline Hazards

**GATE 2027 syllabus:** Computer Organization and Architecture — instruction pipelining, pipeline hazards.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** For a k-stage pipeline processing n instructions with no hazards or stalls, total cycles = k + (n − 1), and speedup over non-pipelined execution = kn / (k + n − 1). A stall (bubble) adds one or more extra cycles for the affected instruction(s). The pipeline's clock period is determined by the slowest (longest-delay) stage. Three hazard types: structural (resource conflict), data (dependency between instructions), control (branches).

---

## Questions

### Q1 · MCQ · Fundamental
In an ideal k-stage pipeline with no hazards, executing n instructions takes

- (A) kn cycles
- (B) k + n cycles
- (C) k + (n − 1) cycles
- (D) n + (k − 1)² cycles

---

### Q2 · NAT · Fundamental
A 5-stage pipeline (no hazards, no stalls) executes 200 instructions. The total number of cycles required is ______.

---

### Q3 · MSQ · Standard
Which of the following statements about pipeline hazards is/are TRUE?

- (A) A structural hazard occurs when two instructions need the same hardware resource in the same cycle.
- (B) A data hazard occurs when an instruction depends on the result of a previous instruction still in the pipeline.
- (C) A control hazard arises from branch instructions, since the next instruction to fetch may not be known until the branch is resolved.
- (D) Forwarding (bypassing) can eliminate some data hazards without requiring a pipeline stall.

---

### Q4 · NAT · Standard
A 5-stage pipeline executes 200 instructions, of which 20% are load-use hazards, each costing exactly 1 stall cycle (no other hazards occur). The total number of cycles required is ______.

---

### Q5 · MCQ · Standard
The clock period of a pipelined processor is determined by

- (A) the average of all stage delays
- (B) the sum of all stage delays
- (C) the delay of the slowest (longest) pipeline stage
- (D) the delay of the fastest pipeline stage

---

### Q6 · NAT · Moderate
A pipeline has 5 stages with delays 2 ns, 1.5 ns, 2.5 ns, 2 ns and 1 ns respectively (plus negligible inter-stage latch delay). Using this pipeline (clock period = slowest stage delay) to execute 100 instructions with no hazards, the total execution time, in ns, is ______.

---

### Q7 · MSQ · Moderate
Which of the following statements about handling **control hazards** (branches) is/are TRUE?

- (A) "Predict not-taken" is a simple static branch-prediction strategy: fetch the next sequential instruction, and flush the pipeline (incurring a penalty) only if the branch turns out to be taken.
- (B) Delayed branching places one or more instructions (that execute regardless of the branch outcome) immediately after the branch, in the "delay slot(s)," to keep the pipeline usefully busy.
- (C) Dynamic branch prediction (e.g., using a branch history table) can achieve higher accuracy than static prediction by adapting to a program's actual runtime behavior.
- (D) A control hazard can only be resolved by stalling the pipeline; no other technique exists.

---

### Q8 · NAT · Moderate
A 5-stage pipeline executes 200 instructions with these hazard-related stalls: 15% of instructions are branches, of which 30% are mispredicted, each misprediction costing a 2-cycle penalty (no other stalls occur). The total number of cycles required is ______.

---

### Q9 · MSQ · Hard
Which of the following statements about pipelining is/are TRUE?

- (A) Deeper pipelines (more stages) can increase clock frequency but also increase the penalty (in cycles) for hazards like branch mispredictions.
- (B) The theoretical maximum speedup of a k-stage pipeline over non-pipelined execution, as the number of instructions n → ∞, approaches k.
- (C) Out-of-order execution is a technique that can help hide the latency of data hazards by executing independent, ready instructions ahead of a stalled one.
- (D) Increasing the number of pipeline stages always increases the total throughput of the processor for every program, with no exceptions.

---

### Q10 · NAT · Killer
A 5-stage pipeline executes 200 instructions. The base CPI (with no hazards at all) is 1. Additionally: 30% of instructions incur a 1-cycle stall (one hazard type), 20% of instructions incur a 2-cycle stall (a different, non-overlapping hazard type), and 10% incur a 3-cycle stall (a third, non-overlapping hazard type); no instruction suffers more than one type of stall. The effective CPI, and hence total cycles for 200 instructions (using total cycles ≈ CPI × n, a standard approximation for large n), rounded to the nearest whole number, is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | C | Standard pipeline-fill formula |
| 2 | 204 | 5 + 200 − 1 |
| 3 | A, B, C, D | All four are standard, correct definitions and facts |
| 4 | 244 | 204 (base) + 200×0.2×1 (stalls) |
| 5 | C | The pipeline can't go faster than its slowest stage |
| 6 | 260 | Clock period = 2.5 ns (slowest stage); total cycles = 5 + 100 − 1 = 104; 104 × 2.5 |
| 7 | A, B, C | (D) is false — prediction, delayed branching and other techniques avoid always stalling |
| 8 | 222 | 204 (base) + 200×0.15×0.3×2 (branch misprediction penalty) |
| 9 | A, B, C | (D) is false — very deep pipelines can suffer diminishing or negative returns due to hazard penalties and overhead, so it does not "always" help |
| 10 | 400 | Effective CPI = 1 + 0.3×1 + 0.2×2 + 0.1×3 = 1 + 0.3 + 0.4 + 0.3 = 2.0; total cycles = 2.0 × 200 |
