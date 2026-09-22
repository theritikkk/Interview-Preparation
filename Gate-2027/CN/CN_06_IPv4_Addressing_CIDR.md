# Computer Networks → IPv4 Addressing, Subnetting & CIDR

**GATE 2027 syllabus:** Computer Networks — IPv4: CIDR notation.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** A /n network has 32 − n host bits. Usable hosts on an ordinary subnet = 2^(host bits) − 2 (network and broadcast addresses excluded), except where a question says otherwise. Two addresses are in the same subnet under mask /n if they share the same top n bits.

---

## Questions

### Q1 · MCQ · Fundamental
The IPv4 address 192.168.10.130 with subnet mask 255.255.255.192 (/26) belongs to which subnet?

- (A) 192.168.10.0/26
- (B) 192.168.10.64/26
- (C) 192.168.10.128/26
- (D) 192.168.10.192/26

---

### Q2 · NAT · Fundamental
An organization needs to allocate a subnet that can hold at least 500 usable host addresses. The minimum number of host bits required is ______.

---

### Q3 · NAT · Standard
A /20 block is available (4096 addresses). It must be split by VLSM into 5 subnets needing 1000, 500, 200, 100 and 50 usable hosts respectively, each subnet sized to the smallest power of two that provides enough usable addresses, allocated largest-request-first starting from the beginning of the block with no gaps. The total number of addresses **left over** (not assigned to any subnet) is ______.

---

### Q4 · MCQ · Standard
Which one of the following is the correct CIDR aggregate (supernet) for the four contiguous /24 blocks 202.61.64.0/24, 202.61.65.0/24, 202.61.66.0/24 and 202.61.67.0/24?

- (A) 202.61.64.0/21
- (B) 202.61.64.0/22
- (C) 202.61.64.0/23
- (D) 202.61.0.0/16

---

### Q5 · NAT · Standard
The number of distinct /24 subnets contained within a single /20 block is ______.

---

### Q6 · MSQ · Moderate
Host X = 10.1.1.60 and host Y = 10.1.1.70. Under which of the following subnet masks do X and Y lie in the **same** subnet?

- (A) /24
- (B) /25
- (C) /26
- (D) /27

---

### Q7 · MCQ · Moderate
An ISP owns the address block 198.24.0.0/16 and has already assigned 198.24.16.0/21 to a customer. Which one of the following blocks is guaranteed to be **disjoint** from that customer's block and also lies inside the ISP's /16?

- (A) 198.24.8.0/21
- (B) 198.24.16.0/22
- (C) 198.24.20.0/22
- (D) 198.24.18.0/23

---

### Q8 · NAT · Moderate
A router's forwarding table has these entries (destination prefix → next hop): 10.0.0.0/8 → R1, 10.16.0.0/12 → R2, 10.16.32.0/20 → R3, 10.16.32.0/24 → R4. Using **longest-prefix matching**, the packet destined for 10.16.32.5 is forwarded to ______  (answer with just the router label's number, e.g. for R1 answer 1).

---

### Q9 · MSQ · Hard
Which of the following statements about CIDR and classful addressing is/are TRUE?

- (A) CIDR allows a network prefix length that does not fall on an 8-bit boundary.
- (B) Under classful addressing, a single organization needing 300,000 host addresses would need a Class A block, which wastes fewer addresses than a single, appropriately sized CIDR block would.
- (C) Route aggregation (supernetting) under CIDR can reduce the number of entries in a backbone router's forwarding table.
- (D) Two different organizations can legitimately be assigned overlapping public CIDR blocks, provided their networks never connect to each other.

---

### Q10 · NAT · Killer
An organization is given the block 172.16.0.0/16. It must create the maximum possible number of equal-sized subnets such that each subnet has enough usable host addresses for 100 hosts, and the resulting subnet mask must be identical across all subnets (fixed-length subnetting, not VLSM). The number of such subnets that can be created is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | C | 130 in binary falls in the third /26 block (128–191) |
| 2 | 9 | 2⁹ − 2 = 510 ≥ 500; 2⁸ − 2 = 254 is too few |
| 3 | 2112 | Sizes 1024+512+256+128+64 = 1984 used of 4096 |
| 4 | B | 4 contiguous /24s combine into one /22 |
| 5 | 16 | 2^(24−20) |
| 6 | A, B | /26 splits at .0–.63/.64–.127, so 60 and 70 fall in different /26 and /27 blocks |
| 7 | A | (C) and (D) overlap the customer's 198.24.16.0–198.24.23.255 range |
| 8 | 4 | /24 is the longest matching prefix |
| 9 | A, C | (B) is backwards: a Class A block gives about 16.7M addresses (wasting about 16.4M), while a CIDR /13 already gives about 524,000 addresses (wasting only about 224,000) — CIDR wastes far less; (D) is false, public CIDR blocks must be globally unique |
| 10 | 512 | Need 7 host bits (2⁷−2=126≥100); subnet bits = 16−7=9 from the /16; 2⁹ |
