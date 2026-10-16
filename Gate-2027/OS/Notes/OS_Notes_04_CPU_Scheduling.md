# OS Notes 04 — CPU Scheduling (High-Yield Deep Dive)

> **Why this file exists:** "Scheduling" is on the high-yield list. Scheduling questions are the most *mechanical* in OS — if you can draw a Gantt chart correctly and apply two formulas, the marks are guaranteed. This file: metrics → every algorithm with worked traces → tie-break conventions → overhead math → a verified 10-question practice set.
>
> **Companion question bank:** `OS_02_CPU_Scheduling.md`.

---

## 0. Vocabulary and the two formulas that give you everything

For each process:

| Term | Meaning |
|---|---|
| Arrival time `A` | when it enters the ready queue |
| Burst time `B` | CPU time it needs |
| Completion time `C` | when it finishes |
| **Turnaround time (TAT)** | `TAT = C − A` (total time from arrival to finish) |
| **Waiting time (WT)** | `WT = TAT − B` (time in ready queue) |
| Response time (RT) | first time it gets the CPU, minus A (matters for interactive systems) |

**Two-step method for any scheduling question:**

1. Draw the **Gantt chart** (who runs when), left to right; record every `C`.
2. Fill a table: `TAT = C − A`, `WT = TAT − B`; then take averages *only if asked*.

Whole-system metrics: **CPU utilization** (fraction busy), **throughput** (processes per unit time), **fairness**. No single algorithm maximizes everything — that tension *is* the topic.

---

## 1. The algorithms — rule, behavior, personality

### 1.1 FCFS (First-Come First-Served)
- Ready **queue**: runs to completion (non-preemptive).
- **Convoy effect:** one long process delays everyone behind it → poor average WT.
- Simplest; no starvation (arrival order).

### 1.2 SJF (Shortest Job First) — non-preemptive
- When the CPU frees, run the arrived process with the **smallest B**.
- **Provably minimizes average waiting time** among non-preemptive algorithms *for a fixed set of processes that all arrive together* (proof idea: swapping any adjacent pair where the longer runs first strictly reduces the sum of waits).
- Needs burst prediction (in practice: exponential averaging of past bursts).
- **Starvation** for long jobs when short jobs keep arriving.

### 1.3 SRTF (Shortest Remaining Time First) — preemptive SJF
- A new arrival with remaining time **strictly smaller** than the running process's remaining time preempts it.
- Optimal for average waiting time overall. Same starvation issue.
- Exam detail: preempt on *strictly less* remaining (equal remaining ⇒ keep running, common convention; state your assumption).

### 1.4 Round Robin (RR)
- FCFS + **time quantum q**: a process runs at most q; if unfinished, it goes to the **back** of the ready queue.
- Good **response time** and fairness; every process waits at most `(n − 1) × q` before its next slice.
- `q → ∞` ⇒ RR degenerates to FCFS. Very small q ⇒ context-switch overhead dominates.
- Typical GATE numbers: no process waits more than `(n−1)q`; completion math from traces.

### 1.5 Priority scheduling
- Highest priority always runs; preemptive or non-preemptive (read the question!).
- **Starvation** of low-priority processes → fixed by **aging** (raise priority the longer you wait — guarantees bounded waiting in the limit).

### 1.6 HRRN (Highest Response Ratio Next)
- Non-preemptive; on CPU free, pick max of `(B + WT_so_far) / B` — a starvation cure built into the formula (older waiting processes see their ratio climb).

### 1.7 Multilevel queues / feedback (MLQ / MLFQ)
- Several queues by class (system, interactive, batch) — fixed priorities between queues (MLQ), or **feedback**: processes that hog CPU sink to lower-priority queues, I/O-bound rise. Adaptive; know the *idea* for MSQs.

---

## 2. Full worked example — one process set, four algorithms

**Processes:** P1(A=0, B=8), P2(A=1, B=4), P3(A=2, B=9), P4(A=3, B=5).

### 2.1 FCFS

```
P1 0────────8 | P2 8───12 | P3 12─────────21 | P4 21────26
```

| | P1 | P2 | P3 | P4 | avg |
|---|---|---|---|---|---|
| C | 8 | 12 | 21 | 26 | |
| TAT = C−A | 8 | 11 | 19 | 23 | **15.25** |
| WT = TAT−B | 0 | 7 | 10 | 18 | **8.75** |

### 2.2 SJF (non-preemptive)

At t=8, arrived = {P2(4), P3(9), P4(5)} → P2; at t=12, {P3(9), P4(5)} → P4; then P3.

```
P1 0────────8 | P2 8───12 | P4 12────17 | P3 17─────────26
```

| | P1 | P2 | P3 | P4 | avg |
|---|---|---|---|---|---|
| C | 8 | 12 | 26 | 17 | |
| TAT | 8 | 11 | 24 | 14 | **14.25** |
| WT | 0 | 7 | 15 | 9 | **7.75** |

### 2.3 SRTF (preemptive)

- t=0: P1 (only). t=1: P2 arrives with 4 < P1's remaining 7 → **preempt**.
- t=5: P2 done; remaining {P1:7, P4:5, P3:9} → P4. t=10: {P1:7, P3:9} → P1. t=17: P3 → done 26.

```
P1 0─1 | P2 1───5 | P4 5────10 | P1 10───────17 | P3 17─────────26
```

| | P1 | P2 | P3 | P4 | avg |
|---|---|---|---|---|---|
| C | 17 | 5 | 26 | 10 | |
| TAT | 17 | 4 | 24 | 7 | **13** |
| WT | 9 | 0 | 15 | 2 | **6.5** |

SRTF (6.5) beats SJF (7.75) beats FCFS (8.75) → the expected order. ✔

### 2.4 RR with q = 2

**Convention used (state it in exams):** a process arriving at the same instant a quantum expires joins the ready queue **before** the running process is requeued.

```
P1 0─2 | P2 2─4 | P3 4─6 | P1 6─8 | P4 8──10 | P2 10──12 | P3 12──14
| P1 14──16 | P4 16──18 | P3 18──20 | P1 20──22 | P4 22─23 | P3 23─25,25─26
```

| | P1 | P2 | P3 | P4 | avg |
|---|---|---|---|---|---|
| C | 22 | 12 | 26 | 23 | |
| TAT | 22 | 11 | 24 | 20 | **19.25** |
| WT | 14 | 7 | 15 | 15 | **12.75** |

Notice: RR's average WT is *worse* than SJF here (long jobs stretch out), but its **first response** is far better — every process starts within one round. That's the RR trade-off in one example.

### 2.5 RR with context-switch overhead

3 processes, each `B = 2`, quantum 1, **context switch cost = 0.5** per switch:

```
A 0─1 |sw| B 1.5─2.5 |sw| C 3─4 |sw| A 4.5─5.5 |sw| B 6─7 |sw| C 7.5─8.5
```

Total time = useful work (6) + 5 switches × 0.5 = **8.5**. Formula-wise: `T = ΣB + (#switches) × cost`, and #switches = (number of dispatches) − 1.

---

## 3. Gantt-chart craft (avoid the classic slips)

1. **Idle gaps:** a process can only run when arrived: at each completion, jump to the next arrival if the queue is empty. FCFS example below shows a gap.
2. **Tie-breaks:** equal bursts → FCFS order (by arrival). Equal arrival+bust → by process name. *State the rule* if ambiguous.
3. **RR bookkeeping:** keep the ready queue as a written list; after each slice: (a) add arrivals *up to and including time t*, (b) requeue the preempted process **at the back**. Most wrong answers come from flipping these two steps.
4. **Recompute remaining times** every slice — RR + preemption errors compound fast.
5. Averages: divide by n only after all rows; round only at the end.

**Idle-gap example (FCFS):** P1(0,5), P2(3,4), P3(10,2).

```
P1 0────5 | P2 5───9 | idle 9─10 | P3 10──12
```

WT: P1 0, P2 `9−3−4 = 2`, P3 0 → avg `2/3 ≈ 0.67`.

---

## 4. Facts table (what MSQs test)

| Statement | Truth |
|---|---|
| SJF minimizes average WT (non-preemptive, fixed process set) | ✔ |
| SRTF minimizes average WT overall | ✔ |
| FCFS can suffer the convoy effect | ✔ |
| RR guarantees response within `(n−1)q` | ✔ |
| `q → ∞` makes RR = FCFS | ✔ |
| Priority scheduling can starve; aging fixes it | ✔ |
| FCFS is non-preemptive | ✔ |
| SJF is always unimprovable even when processes arrive over time | ✘ (only optimal for the arrival-known fixed set; SJF *with* arrivals is not globally optimal — SRTF beats it) |
| RR necessarily minimizes average TAT | ✘ (often worse — see trace) |

---

## 5. Practice set (all numeric answers verified by simulation)

The process set below is used in Q1–Q3, Q5, Q10: **P1(A=0,B=8), P2(A=1,B=4), P3(A=2,B=9), P4(A=3,B=5)**.

### Q1 · NAT · Fundamental
Average waiting time under **FCFS** (two decimal places) is ______.

---

### Q2 · NAT · Fundamental
Average turnaround time under **SJF (non-preemptive)** (two decimal places) is ______.

---

### Q3 · NAT · Standard
Average waiting time under **SRTF** (two decimal places) is ______.

---

### Q4 · MCQ · Standard
Which of the following is TRUE?

- (A) SJF gives the minimum average turnaround time even when processes arrive at different times.
- (B) SRTF is the preemptive version of SJF and can preempt the running process when a new process arrives with a strictly smaller remaining time.
- (C) RR with a very large quantum tends to approximate SRTF.
- (D) FCFS is preemptive.

---

### Q5 · NAT · Standard
For the process set above, under **RR with quantum 2** (arrivals at an instant join the queue before the expiring process is requeued), the average turnaround time (two decimal places) is ______.

---

### Q6 · MSQ · Standard
Which of the following statements is/are TRUE?

- (A) SJF minimizes average waiting time among non-preemptive algorithms (fixed, simultaneously-known process set).
- (B) SRTF can starve long processes.
- (C) As the RR quantum tends to infinity, RR tends to FCFS.
- (D) SJF minimizes average turnaround time in all scenarios, preemptive or not.

---

### Q7 · NAT · Moderate
Processes **P1(A=0,B=5), P2(A=3,B=4), P3(A=10,B=2)** under **FCFS**. The average waiting time (two decimal places) is ______.

---

### Q8 · NAT · Moderate
Processes arrive as: **P1(A=0,B=4)**, **P2(A=1,B=3)**, **P3(A=2,B=2)**. **Non-preemptive priority** scheduling, smaller priority number = higher priority: P1 has priority 3, P2 has priority 1, P3 has priority 2. The average waiting time (two decimal places) is ______.

---

### Q9 · NAT · Moderate
Three processes, each with burst time **2**, arrive together. RR with quantum **1** and a **context-switch cost of 0.5** per switch. The total time until the last process completes is ______.

---

### Q10 · NAT · Killer
For the Q1–Q3 process set: (average WT under SJF) − (average WT under SRTF) = ______.

---

## Answer Key

| Q | Answer | Why (condensed) |
|---|---|---|
| 1 | 8.75 | Gantt: 0-8-12-21-26 → WTs 0,7,10,18 |
| 2 | 14.25 | Order P1,P2,P4,P3 → TATs 8,11,14,24 |
| 3 | 6.5 | Order P1,P2,P4,P1,P3 → WTs 9,0,2,15 |
| 4 | B | SRTF = preemptive SJF; (A)/(C)/(D) false |
| 5 | 19.25 | RR trace in §2.4 → TATs 22,11,24,20 |
| 6 | A, B, C | (D) false — SRTF beats SJF with arrivals |
| 7 | 0.67 | WTs 0,2,0 → 2/3 |
| 8 | 2.67 | Order P1(0-4), P2(4-7), P3(7-9); WTs 0,3,5 → 8/3 |
| 9 | 8.5 | 6 work + 5 switches × 0.5 |
| 10 | 1.25 | 7.75 − 6.5 |

### Full solutions

**Q1.** FCFS order = arrival order: `0-8 | 8-12 | 12-21 | 21-26`. WT(P1)=0, WT(P2)=12−1−4=7, WT(P3)=21−2−9=10, WT(P4)=26−3−5=18. Avg = 35/4 = **8.75**.

**Q2.** SJF: P1 runs 0–8 (only arrival). Then shortest among {P2:4, P4:5, P3:9} → P2 (8–12), then {P4:5, P3:9} → P4 (12–17), then P3 (17–26). TAT = 8, 11, 24, 14; avg = 57/4 = **14.25**. (Same table as §2.2.)

**Q3.** SRTF trace §2.3: completions 17, 5, 26, 10 → TAT 17, 4, 24, 7; WT 9, 0, 15, 2; avg = 26/4 = **6.5**.

**Q4.** B. (A) ✘ — SJF's optimality is for the *fixed set*; with arrivals, SRTF can beat it (here 6.5 < 7.75). (C) ✘ — large quantum ≈ FCFS, not SRTF. (D) ✘ — FCFS never preempts. (B) ✔ by definition, "strictly smaller" included.

**Q5.** Trace (§2.4): P1 0-2, P2 2-4, P3 4-6, P1 6-8, P4 8-10, P2 10-12 (done), P3 12-14, P1 14-16, P4 16-18, P3 18-20, P1 20-22 (done), P4 22-23 (done), P3 23-26 (done). Completions: 22, 12, 26, 23 → TAT 22, 11, 24, 20 → avg = 77/4 = **19.25**.

**Q6.** A ✔ (classic theorem, fixed set). B ✔ (short jobs monopolize). C ✔ (no preemption ever occurs if quantum exceeds every burst). D ✘ (see Q4-A).

**Q7.** FCFS: P1 0–5; P2 5–9; gap 9–10; P3 10–12. WT(P1) = 0; WT(P2) = C−A−B = 9−3−4 = 2; WT(P3) = 12−10−2 = 0. Avg = (0+2+0)/3 = **0.67**.

**Q8.** At t=0 only P1 (priority 3) is present; non-preemptive → it runs 0–4. At t=4, waiting: P2 (pri 1, higher) and P3 (pri 2) → P2 (4–7), then P3 (7–9). WT: P1 0; P2 = 7−1−3 = 3; P3 = 9−2−2 = 5. Avg = 8/3 ≈ **2.67**. (Note: priority only mattered *after* P1 — non-preemptive cannot preempt P1 at t=1.)

**Q9.** Order: A(0–1), switch(1–1.5), B(1.5–2.5), switch(2.5–3), C(3–4), switch(4–4.5), A(4.5–5.5 done), switch(5.5–6), B(6–7 done), switch(7–7.5), C(7.5–8.5 done). Work 6 + 5×0.5 = **8.5**.

**Q10.** SJF avg WT = 7.75 (§2.2); SRTF avg WT = 6.5 (§2.3). Difference = **1.25**. (This is the price of not being able to preempt — the reason real schedulers are preemptive with respect to priority/SJF-like orders.)

---

## 6. Next steps

1. Redraw the §2.4 RR trace from scratch — if you get all completion times, this topic is done.
2. Do `OS_02_CPU_Scheduling.md` (10 questions including MLFQ-ish variants).
3. OS notes complete! → Continue with **CN**: `../CN/Notes/CN_Notes_01_IP_and_IP_Addressing.md` (next to be written).
