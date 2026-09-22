# X-Ray

🔴 Explicitly named in Hermes's Step Functions orchestration.

---

## Cluster 1: Distributed Tracing — Segments, Subsegments, Trace ID Propagation

### What is it?
X-Ray traces a single request's path across multiple services/Lambdas/Step Functions states, stitching them into one **trace** made of **segments** (one per service/component the request touched) and **subsegments** (finer-grained timing within a segment, e.g., a specific DynamoDB call). This solves the problem plain per-service logs can't: seeing one logical request's *end-to-end* path and where time was actually spent across service boundaries.

### How does it work?
A **trace ID** is generated at the entry point and propagated through every downstream call (via headers, or automatically by the Step Functions/Lambda X-Ray SDK integration) — every segment tags itself with the same trace ID, which is how X-Ray reassembles the full picture afterward.

### Interview Questions
- **L1:** "X-Ray traces a request across multiple services by propagating a shared trace ID, stitching per-service segments into one end-to-end view — showing exactly where time was spent across a distributed call path, not just within one service."
- **L2 — 🔴 directly relevant to Hermes's multi-step Step Functions pipelines:** explain why tracing matters specifically for orchestrated, multi-step workflows: without it, diagnosing "why did this workflow take 4 seconds" means correlating separate logs from each Lambda step manually by timestamp; with X-Ray, the trace directly shows each state's duration within the single stitched view, immediately revealing which specific step was the bottleneck.
- **L3:** connect to the general concept of a **correlation ID** (the broader pattern X-Ray's trace ID is one specific implementation of) — relevant even outside AWS-native tracing, e.g., if asked how you'd debug a similar cross-service latency issue in the Task Scheduler (which doesn't use X-Ray): the same idea (a shared ID propagated through logs/messages across the API → coordinator → worker path) would let you correlate that system's Loki logs the same way X-Ray correlates Hermes's segments.

### Quick Revision
- Trace = one logical request, made of segments (per-service) and subsegments (finer detail within a segment).
- Trace ID propagation across service boundaries is what makes end-to-end (not just per-service) timing visible.
- The general pattern (correlation ID) applies even without X-Ray specifically — relevant if asked to debug a non-AWS distributed system the same way.
