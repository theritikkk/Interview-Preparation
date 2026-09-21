# OS → Disk & I/O Scheduling

**GATE 2027 syllabus:** Operating System — CPU and I/O scheduling (disk scheduling).
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** Cylinders are numbered 0–199 unless stated otherwise. Head movement is measured in cylinders. Average rotational latency = half a rotation. 1 GB = 2³⁰ bytes.

---

## Questions

### Q1 · NAT · Fundamental
A disk has 8 platters (both surfaces of every platter are used), 4096 cylinders, 256 sectors per track and 512-byte sectors. The capacity of the disk in GB is ______.

---

### Q2 · NAT · Fundamental
A disk rotates at 12,000 RPM and has 400 sectors per track. The time to transfer one sector (transfer only, no seek or latency) in microseconds is ______.

---

### Q3 · NAT · Standard
A disk rotates at 6000 RPM, has an average seek time of 6 ms and 300 sectors per track (1 KB per sector). A file occupies 50 sectors.

- **Contiguous:** the 50 sectors are consecutive on one track: one seek + average rotational latency + 50 sector-transfer times.
- **Scattered:** every sector needs its own average seek + average rotational latency + one sector-transfer time.

(Time for scattered) − (time for contiguous), in ms, is ______.

---

### Q4 · NAT · Standard
The head is at cylinder 100. The pending requests are for cylinders 45, 120, 35, 15, 190, 160 and 70. Using **SSTF** (no ties occur), the total head movement in cylinders is ______.

---

### Q5 · NAT · Standard
The head is at cylinder 90 and is moving toward higher cylinder numbers. The pending requests are for cylinders 30, 150, 60, 110, 20, 170 and 130. **C-LOOK** serves requests upward up to the highest request, then jumps directly to the lowest pending request and continues upward. The jump distance **is counted**. The total head movement in cylinders is ______.

---

### Q6 · MSQ · Standard
Which of the following disk-scheduling algorithms is/are free from starvation of requests?

- (A) SSTF
- (B) SCAN
- (C) C-LOOK
- (D) FCFS

---

### Q7 · NAT · Moderate
SSTF scheduling. The head is at cylinder 140 and is initially considered to be moving toward **higher** cylinders. The pending requests are for cylinders 90, 150, 160, 120, 10 and 130. If two pending requests are equally close, the one lying in the direction of the head's current movement is chosen. A *direction reversal* is a pair of consecutive seeks in opposite directions (the first seek has no predecessor). The number of direction reversals is ______.

---

### Q8 · NAT · Moderate
SSTF scheduling with requests arriving over time. At t = 0 the head is at cylinder 50. Moving the head by one cylinder takes 1 time unit and servicing a request (rotation + transfer) takes 5 time units. Requests: cylinders 20 and 85 at t = 0; cylinder 30 at t = 10; cylinder 60 at t = 20. When a service ends, the next request is chosen by SSTF among the requests that have already arrived (no ties occur). The time at which the request for cylinder 85 completes is ______.

---

### Q9 · NAT · Hard
The head is at cylinder 150 and the pending requests are for cylinders 180, 20, 90, 130, 70 and 100. Let A be the total head movement under **SSTF** (no ties occur) and B the minimum total head movement over **all possible service orders**. A − B = ______ cylinders.

---

### Q10 · MSQ · Killer
The head is at cylinder 100, initially moving toward higher cylinders. The pending requests are for cylinders 35, 88, 121, 160 and 40 (cylinders 0–199). The requests were served in the order **121, 160, 88, 40, 35**. Which of the following algorithms could have produced this order?

- (A) SSTF
- (B) SCAN
- (C) C-SCAN
- (D) LOOK

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | 8 | 16 surfaces × 4096 × 256 × 512 B = 8 GB |
| 2 | 12.5 | One rotation 5 ms; 5 ms / 400 |
| 3 | 539 | Rotation 10 ms; contiguous 6 + 5 + 50/30 = 12.67 ms; scattered 50 × (6 + 5 + 1/30) = 551.67 ms |
| 4 | 265 | Order 120, 160, 190, 70, 45, 35, 15: 20 + 40 + 30 + 120 + 25 + 10 + 20 |
| 5 | 270 | 90 → 170 = 80; jump 170 → 20 = 150; 20 → 60 = 40 |
| 6 | B, C, D | SSTF can starve far requests |
| 7 | 1 | Order 150, 160, 130, 120, 90, 10: up, up, down, down, down, down |
| 8 | 115 | Completions: 20 at 35, 30 at 50, 60 at 85, 85 at 115 |
| 9 | 100 | SSTF 290 (order 130, 100, 90, 70, 20, 180); optimum 190 (180 first, then down to 20) |
| 10 | B, D | SCAN and LOOK serve in the same order (121, 160, 88, 40, 35); C-SCAN gives 121, 160, 35, 40, 88; SSTF starts with 88 |
