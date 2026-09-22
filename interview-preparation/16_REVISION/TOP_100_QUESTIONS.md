# Top Questions — Curated Index

A note on this file: rather than padding out exactly 100/50 near-duplicate questions per category (which would dilute signal), this is a curated set of the highest-probability, highest-value questions per category, each pointing to where the full answer lives. Treat this as your final-pass question list, not your first-read material.

---

## Must-Know Questions (Cross-Cutting)
1. Walk me through your resume → `14_BEHAVIORAL/RESUME_WALKTHROUGH.md`
2. Why Event Sourcing / CQRS for Hermes? → `13_PROJECT_DEEP_DIVES/HERMES.md`
3. Is your Task Scheduler's delivery really exactly-once? → `13_PROJECT_DEEP_DIVES/DISTRIBUTED_TASK_SCHEDULER.md`
4. What does zero-copy mean in NetSight? → `13_PROJECT_DEEP_DIVES/NETSIGHT.md`
5. How does tenant isolation work in Hermes? → `08_AWS/COGNITO.md`
6. Why RabbitMQ, not Kafka? → `07_DISTRIBUTED_SYSTEMS/MESSAGING.md`
7. What are the tradeoffs of serverless? → `13_PROJECT_DEEP_DIVES/HERMES.md`
8. Tell me about a project you're proud of → `14_BEHAVIORAL/STAR_STORIES.md`
9. Prove your two-pointer/sliding-window solution is O(n), not O(n²) → `02_DSA/PATTERNS.md`
10. What breaks first if this system 10x'd its load? (any of your 3 projects) → each project's deep-dive file

## Top Coding Patterns
11. Two Sum (hashing) · 12. Longest Substring Without Repeating Characters (sliding window) · 13. Merge Intervals · 14. Valid Parentheses · 15. Daily Temperatures (monotonic stack) · 16. Binary Search on rotated array · 17. Level order traversal · 18. Kth largest in a stream (heap) · 19. Course Schedule (topological sort) · 20. Number of Islands (BFS/DFS) · 21. Coin Change (DP) · 22. Climbing Stairs (DP) · 23. Subsets (backtracking) · 24. Network Delay Time (Dijkstra) · 25. Redundant Connection (union-find) · 26. Single Number (bit manipulation) · 27. Implement Trie · 28. LRU Cache (design — hash map + doubly linked list) · 29. Merge k Sorted Lists · 30. Word Ladder (BFS on implicit graph)
→ full pattern breakdown for all of these: `02_DSA/PATTERNS.md`

## Top CS Fundamentals Questions
31. Process vs thread? · 32. What are the 4 deadlock conditions? · 33. What is virtual memory / paging / TLB? · 34. B-tree vs B+ tree, and why do databases use B+ trees? · 35. ACID, and what does each letter actually guarantee? · 36. MVCC — how does it let readers avoid blocking writers? · 37. TCP vs UDP, and when would you choose each? · 38. What is epoll and why does it matter for Node's concurrency model? · 39. Normalization vs denormalization — when is denormalizing the right call? · 40. The 4 OOP pillars, one sentence each.
→ `03_CS_FUNDAMENTALS/`

## Top Full-Stack Questions
41. Idempotent vs non-idempotent HTTP methods? · 42. What's an idempotency key and why does it matter? · 43. Cursor vs offset pagination — when does offset actually break? · 44. `useEffect` dependency array — what does `[]` vs omitted vs `[x]` actually mean? · 45. What's a stale closure bug? · 46. CORS — what is it actually protecting, and what does it NOT protect against? · 47. XSS vs CSRF — different attacks, different defenses, name both. · 48. Cache-aside pattern — walk through it. · 49. What is a cache stampede and how do you mitigate it? · 50. Explain the dependency-injection pattern and why it aids testability.
→ `04_BACKEND/`, `05_FRONTEND/`, `06_DATABASES/`, `12_SECURITY/`

## Top Project Questions (Yours Specifically)
51. Why did you choose DynamoDB for Hermes's event store? · 52. Walk through exactly what happens when 100 concurrent writers hit the same aggregate. · 53. Name your six read models and what each serves. · 54. How does the Transactional Outbox prevent the dual-write problem? · 55. Why Step Functions AND EventBridge — aren't they redundant? · 56. Walk through your Cognito JWT verification path. · 57. Why TTL + dead-letter-exchange instead of polling? · 58. What's the actual bottleneck your load test revealed? · 59. Is the coordinator a single point of failure? · 60. What does your row-count convergence check actually prove? · 61. Why AF_PACKET/TPACKET_V3 specifically? · 62. Explain your Treiber stack's ABA-problem handling. · 63. What is BTF CO-RE and why does it matter for your eBPF code? · 64. What was the hardest bug in your internship project? · 65. Why did you ship the ticketing module after the internship ended?
→ `13_PROJECT_DEEP_DIVES/` (all four files)

## Top System Design Questions
66. Design a URL shortener · 67. Design a rate limiter · 68. Design a distributed job scheduler (this is your own project — derive it from scratch) · 69. Design an event-driven workflow engine (also your own project) · 70. Design a notification system · 71. Design a chat application · 72. How do you estimate scale for a system handling 10M DAU? · 73. Walk through your framework for a 45-minute system design interview.
→ `11_SYSTEM_DESIGN/`

## Top Debugging Questions
74. API latency jumped from 70ms to 500ms — what do you check, in order? · 75. RabbitMQ queue depth keeps growing — diagnose it. · 76. CPU is normal but latency increased — what does that rule in/out? · 77. Database connections are exhausted — what's your first move? · 78. One Kubernetes pod keeps restarting — liveness or readiness probe first? · 79. Memory grows continuously — what's your process for finding the leak? · 80. Users report duplicate results on a single request — what's the likely cause given at-least-once delivery?
→ `15_INTERVIEW_PREP/DEBUGGING.md`, `10_OBSERVABILITY/`

## Top Behavioral Questions
81. Tell me about yourself · 82. Tell me about a difficult technical problem · 83. Tell me about a failure · 84. Tell me about ownership · 85. Tell me about learning something quickly · 86. Why should we hire you? · 87. Tell me about a disagreement · 88. What's your biggest weakness?
→ `14_BEHAVIORAL/HR.md`, `STAR_STORIES.md`

### Quick Revision
- This index is a final-pass tool — if a question here feels unfamiliar, go read its source file in full rather than trying to absorb it from the one-line prompt alone.
- Questions 1-10 are the highest-probability, highest-stakes set in this entire knowledge base — know them cold before anything else.
