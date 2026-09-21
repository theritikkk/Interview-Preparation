# OS → CPU Scheduling

**GATE 2027 syllabus:** Operating System — CPU and I/O scheduling.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** Single CPU unless stated otherwise. Context-switch overhead is negligible. Waiting time = time spent in the ready queue. Turnaround time = completion − arrival. Response time = first CPU allocation − arrival. All times in ms.

---

## Questions

### Q1 · NAT · Fundamental
Five processes are scheduled by **non-preemptive Shortest Job First (SJF)**. The CPU stays idle whenever the ready queue is empty.

| Process | P1 | P2 | P3 | P4 | P5 |
|---|---|---|---|---|---|
| Arrival time | 1 | 2 | 3 | 4 | 15 |
| CPU burst | 5 | 3 | 1 | 2 | 2 |

The average waiting time (rounded to one decimal place) is ______.

---

### Q2 · MCQ · Fundamental
Processes P1, P2, …, Pn all arrive at time 0 (in this order) and are scheduled by Round Robin with time quantum q. There is no I/O and no later arrival. The response time of Pn is at most

- (A) q
- (B) (n − 1)·q
- (C) n·q
- (D) (n − 1)·(q + 1)

---

### Q3 · NAT · Standard
**Preemptive SRTF** scheduling. If an arriving process has burst time *equal* to the remaining time of the running process, the running process is **not** preempted; any other tie is broken in favour of the earlier arrival.

| Process | P1 | P2 | P3 | P4 |
|---|---|---|---|---|
| Arrival time | 0 | 2 | 4 | 5 |
| CPU burst | 7 | 4 | 2 | 3 |

The average **response time** (rounded to two decimal places) is ______.

---

### Q4 · NAT · Standard
**Round Robin**, time quantum q = 2. Convention: a process that arrives at the same instant at which a running process's quantum expires is placed in the ready queue **ahead of** the preempted process.

| Process | P1 | P2 | P3 |
|---|---|---|---|
| Arrival time | 0 | 2 | 4 |
| CPU burst | 5 | 4 | 2 |

The average waiting time (rounded to one decimal place) is ______.

---

### Q5 · MSQ · Standard
A set of processes all arrive at time 0 on a single CPU. Which of the following statements is/are TRUE?

- (A) Non-preemptive SJF and SRTF produce identical schedules.
- (B) If the arrival order of the processes is in non-decreasing order of burst time, FCFS and SJF produce identical schedules.
- (C) Round Robin with a time quantum at least as large as the largest burst time produces the same schedule as FCFS.
- (D) Round Robin always gives a lower average waiting time than FCFS.

---

### Q6 · NAT · Moderate
Two identical CPUs share one ready queue and use **non-preemptive SJF**: whenever a CPU becomes free it takes the ready process with the smallest burst time. Six processes arrive at time 0.

| Process | P1 | P2 | P3 | P4 | P5 | P6 |
|---|---|---|---|---|---|---|
| CPU burst | 8 | 1 | 10 | 4 | 9 | 6 |

The average waiting time (rounded to one decimal place) is ______.

---

### Q7 · MSQ · Moderate
Three processes with arrival times and CPU bursts

| Process | P1 | P2 | P3 |
|---|---|---|---|
| Arrival time | 0 | 1 | 2 |
| CPU burst | 4 | 2 | 1 |

produced the following Gantt chart: **P1 (0–4), P2 (4–6), P3 (6–7)**. Which of the following algorithms could have produced it? (Ties, if any, may be broken arbitrarily.)

- (A) FCFS
- (B) Non-preemptive SJF
- (C) SRTF
- (D) Round Robin with quantum 4

---

### Q8 · NAT · Moderate
**Preemptive priority** scheduling (smaller number = higher priority). All values distinct, no ties.

| Process | P1 | P2 | P3 | P4 | P5 |
|---|---|---|---|---|---|
| Arrival time | 0 | 1 | 2 | 3 | 12 |
| CPU burst | 6 | 4 | 5 | 2 | 3 |
| Priority | 3 | 2 | 4 | 1 | 5 |

(Average waiting time) − (average response time) = ______ (rounded to one decimal place).

---

### Q9 · NAT · Hard
Round Robin, quantum 2. Each process alternates between CPU bursts (C) and I/O bursts (I). All three arrive at time 0 and are initially queued in the order P1, P2, P3.

| Process | Burst sequence |
|---|---|
| P1 | C4, I3, C2 |
| P2 | C3, I2, C3 |
| P3 | C5 |

Every I/O burst starts the moment the process leaves the CPU and runs on its own device (no queueing among I/O requests). A process that finishes its I/O joins the tail of the ready queue. If an I/O completion and a quantum expiry happen at the same instant, the process returning from I/O is queued **ahead of** the preempted process (several returning processes are queued in the order P1, P2, P3). A process whose CPU burst ends exactly when its quantum ends leaves the CPU (it is not requeued). The completion time of P3 is ______ ms.

---

### Q10 · MSQ · Killer
Three processes A, B, C with CPU bursts 3, 5 and 7 ms all arrive at time 0. A single CPU is used; it never idles while a process is ready; preemption is allowed at millisecond boundaries and any scheduling policy may be used. Which of the following completion-time vectors (C_A, C_B, C_C) is/are **IMPOSSIBLE**?

- (A) (7, 15, 10)
- (B) (6, 15, 9)
- (C) (13, 15, 14)
- (D) (3, 12, 9)

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | 2.6 | Idle 0–1, P1 1–6, P3 6–7, P4 7–9, P2 9–12, idle 12–15, P5 15–17; waits 0, 7, 3, 3, 0 |
| 2 | B | The other n−1 processes each run at most q first |
| 3 | 1.25 | P1 0–2, P2 2–6, P3 6–8, P4 8–11, P1 11–16; response times 0, 0, 2, 3 |
| 4 | 4.0 | P1 0–2, P2 2–4, P1 4–6, P3 6–8, P2 8–10, P1 10–11; waits 6, 4, 2 |
| 5 | A, B, C | (D) fails, e.g. two equal bursts |
| 6 | 4.0 | Bursts 1, 4, 6, 8, 9, 10; waits 0, 0, 1, 4, 7, 12 |
| 7 | A, D | SJF would run P3 at t = 4; SRTF would preempt at t = 1; RR q = 2 would preempt at t = 2 |
| 8 | 1.6 | Gantt P1 0–1, P2 1–3, P4 3–5, P2 5–7, P1 7–12, P3 12–17, P5 17–20; avg wait 4.6, avg response 3.0 |
| 9 | 16 | P1 0–2, P2 2–4, P3 4–6, P1 6–8, P2 8–9, P3 9–11, P1 11–13, P2 13–15, P3 15–16, P2 16–17 |
| 10 | B, D | (B) both A and C must be done by t = 9, needing 10 units; (D) last completion must equal total work 15 |
