# System Design — Fundamentals

Most of the individual building blocks (caching, replication, sharding, messaging, consistency) are covered in depth elsewhere in this knowledge base — this file is the estimation toolkit and the checklist of what a complete design needs to address.

---

## Cluster 1: Back-of-Envelope Scale Estimation

### What is it?
Quick, order-of-magnitude math to figure out whether a design needs to handle thousands or billions of requests — this determines almost every subsequent decision (single server vs. distributed, SQL vs. NoSQL, caching strategy).

### Numbers Worth Having Memorized
| Quantity | Rough value |
|---|---|
| Seconds in a day | ~86,400 (~100K for quick mental math) |
| 1 million requests/day | ~12 requests/sec average |
| L1 cache reference | ~1 ns |
| Main memory reference | ~100 ns |
| SSD random read | ~100 μs |
| Network round trip, same datacenter | ~0.5 ms |
| Network round trip, cross-continent | ~100-150 ms |
| Disk seek (spinning) | ~10 ms |

### How to Use It
```
"Design a system handling 10M daily active users, each making ~5 requests/day"
→ 50M requests/day ÷ 86,400s ≈ 580 req/sec average
→ Peak is typically 2-5x average → design for ~2,000-3,000 req/sec peak
→ If each request is 1KB → ~50GB/day of traffic → informs bandwidth/storage estimates
```

### Interview Questions
- **L1:** "Back-of-envelope estimation converts a stated user/request volume into requests-per-second, storage, and bandwidth figures — rounding aggressively is fine and expected; the goal is the right order of magnitude, not precision."
- **L2:** explain why you estimate *peak*, not just average, load — a system designed only for average load falls over during real traffic patterns (most systems have meaningful peak-to-average ratios, e.g., 2-5x, from daily/weekly usage cycles), and interviewers specifically watch for whether you remember to account for this.

---

## Cluster 2: What a Complete Design Must Address

A checklist to run through explicitly in any system-design interview, in roughly this order:

1. **Functional requirements** — what must the system do (be explicit, don't assume)
2. **Non-functional requirements** — scale, latency targets, consistency needs, availability targets
3. **Scale estimation** — back-of-envelope numbers (Cluster 1)
4. **API design** — the contract between client and system
5. **Data model** — what's stored, where, and why that shape
6. **High-level architecture** — the boxes-and-arrows diagram
7. **Deep dive** — the 1-2 hardest/most interesting components, in real depth
8. **Bottlenecks & failure modes** — what breaks first, and what happens when it does
9. **Scaling strategy** — how each bottleneck gets addressed as load grows
10. **Consistency & tradeoffs** — explicit CAP-style tradeoffs made and why
11. **Security** — auth, tenant isolation if relevant, rate limiting
12. **Observability** — how you'd know if it's working/broken in production

### Interview Questions
- **L2:** explain why skipping step 1 (clarifying functional requirements) is the single most common system-design interview failure mode: designing an excellent solution to a problem the interviewer didn't actually ask about wastes the entire session — always paraphrase the requirements back and ask 2-3 clarifying questions before drawing anything.

### Quick Revision
- Peak load, not average, is what you design for — remember the 2-5x multiplier.
- Requirements clarification is step 1, not an afterthought — most interview failures start here.
- A complete answer touches all 12 checklist items, even briefly — depth on one item while skipping others (especially failure modes/tradeoffs) reads as incomplete.
