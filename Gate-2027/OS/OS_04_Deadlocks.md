# OS → Deadlocks

**GATE 2027 syllabus:** Operating System — deadlock.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** Resources are requested and released one unit at a time unless stated otherwise. A process holds what it has been allocated until it finishes. In Banker's algorithm, Need = Max − Allocation.

---

## Questions

### Q1 · MCQ · Fundamental
In a resource-allocation graph in which **every resource type has exactly one instance**, a cycle is

- (A) necessary but not sufficient for a deadlock
- (B) both necessary and sufficient for a deadlock
- (C) sufficient but not necessary for a deadlock
- (D) neither necessary nor sufficient for a deadlock

---

### Q2 · NAT · Fundamental
Four processes share identical units of a single resource type. Their maximum needs are 3, 4, 2 and 5 units. Each process requests one unit at a time and releases everything only when it finishes. The minimum number of units that guarantees deadlock can **never** occur is ______.

---

### Q3 · MCQ · Standard
A system has three resource types A, B, C and five processes. The current state is

| Process | Allocation (A B C) | Max (A B C) |
|---|---|---|
| P0 | 2 2 0 | 2 3 0 |
| P1 | 0 0 2 | 3 3 3 |
| P2 | 0 2 1 | 0 4 2 |
| P3 | 0 2 0 | 1 4 3 |
| P4 | 2 0 1 | 4 0 4 |

Available = (A, B, C) = (1, 2, 1). Which one of the following is a **safe sequence**?

- (A) P0, P4, P2, P3, P1
- (B) P2, P0, P3, P1, P4
- (C) P0, P2, P1, P3, P4
- (D) P0, P2, P4, P3, P1

---

### Q4 · MSQ · Standard
Which of the following can be used to **prevent** deadlock by making at least one of the four necessary conditions impossible?

- (A) Requiring every process to request all its resources at once, before it starts executing
- (B) Numbering the resource types and requiring every process to request resources in increasing order of number
- (C) Running the Banker's algorithm before every allocation
- (D) Allowing the OS to preempt the resources held by a process that is waiting for another resource

---

### Q5 · MCQ · Standard
Resource R1 has 2 instances; R2 and R3 have 1 instance each.

- Assigned: one instance of R1 to P1, the other instance of R1 to P4, R2 to P2, R3 to P3.
- Pending requests: P1 → R2, P2 → R3, P3 → R1. P4 has no pending request and will eventually finish.

Which one of the following is TRUE?

- (A) P1, P2 and P3 are deadlocked
- (B) All four processes are deadlocked
- (C) The graph contains a cycle, but there is no deadlock
- (D) The graph contains no cycle

---

### Q6 · MSQ · Moderate
Consider the state of **Q3**. Each of the following requests is made **independently** from that state. Which request(s) can be granted immediately without leaving the system in an unsafe state?

- (A) P1 requests (1, 1, 0)
- (B) P3 requests (1, 0, 0)
- (C) P4 requests (2, 0, 0)
- (D) P2 requests (0, 2, 0)

---

### Q7 · NAT · Moderate
Deadlock detection with multiple instances. Available = (A, B, C) = (0, 1, 1).

| Process | Allocation (A B C) | Request (A B C) |
|---|---|---|
| P0 | 0 2 0 | 0 0 2 |
| P1 | 0 0 1 | 0 1 0 |
| P2 | 0 0 1 | 0 0 2 |
| P3 | 2 0 2 | 1 1 0 |
| P4 | 0 2 1 | 1 0 2 |
| P5 | 1 0 2 | 1 1 2 |

The number of processes that are deadlocked is ______.

---

### Q8 · MSQ · Moderate
Three processes share three semaphores S, T, U (initial values given in the options as (S, T, U)):

```c
P:  wait(S); wait(T);  /* work */  signal(T); signal(S);
Q:  wait(T); wait(U);  /* work */  signal(U); signal(T);
R:  wait(U); wait(S);  /* work */  signal(S); signal(U);
```

For which initial values is a deadlock **possible**?

- (A) (1, 1, 1)
- (B) (2, 1, 1)
- (C) (1, 1, 2)
- (D) (1, 0, 1)

---

### Q9 · NAT · Hard
Four processes, three resource types A, B, C. Available = (2, 1, 2).

| Process | Allocation (A B C) | Max (A B C) |
|---|---|---|
| P0 | 3 0 0 | 5 1 0 |
| P1 | 2 2 1 | 2 6 2 |
| P2 | 3 1 0 | 5 5 1 |
| P3 | 2 1 3 | 4 3 3 |

The state is **not safe**. What is the minimum **total** number of additional resource instances (of any types, added to the Available vector) needed to make the state safe?

---

### Q10 · MSQ · Killer
Resource-allocation graph with **claim edges** (deadlock avoidance). R1, R2, R3 are single-instance resources. Claim (maximum) edges: P1 → {R1, R2}, P2 → {R2, R3}, P3 → {R3, R1}. Currently R1 is assigned to P1, R2 is assigned to P2 and R3 is free. There are no pending requests. The OS grants a request only if the resulting state is safe. Which of the following requests will be **granted**?

- (A) P3 requests R3
- (B) P2 requests R3
- (C) P1 requests R3
- (D) P3 requests R1

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | B | Single instance: cycle ⇔ deadlock |
| 2 | 11 | Σ(mᵢ − 1) + 1 = 2 + 3 + 1 + 4 + 1 |
| 3 | C | Need: P0 (0,1,0), P1 (3,3,1), P2 (0,2,1), P3 (1,2,3), P4 (2,0,3); P0 → P2 → P1 → P3 → P4 works; (A), (B), (D) each get stuck on C |
| 4 | A, B, D | (C) is avoidance, not prevention |
| 5 | C | Cycle P1→R2→P2→R3→P3→R1→P1, but P4 finishes and frees an R1 instance |
| 6 | A, D | (B) leads to an unsafe state; (C) exceeds Available |
| 7 | 3 | P1, P2, P0 can finish; P3, P4, P5 all wait for resource A |
| 8 | A, D | With any semaphore at 2 the circular wait cannot close; (1, 0, 1) blocks at T = 0 |
| 9 | 2 | B is the bottleneck; two extra instances of B suffice, no single instance of anything does |
| 10 | B | (A) makes the state unsafe; (C) is outside P1's claim; (D) R1 is not free |
