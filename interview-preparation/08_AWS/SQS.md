# SQS (Simple Queue Service)

🔴 Transactional Outbox target in Hermes.

---

## Cluster 1: Standard vs. FIFO, Visibility Timeout, DLQ

### What is it?
**Standard queues**: at-least-once delivery, best-effort ordering (can deliver out of order or more than once), nearly unlimited throughput. **FIFO queues**: strict ordering and exactly-once *processing* (within a message group), at lower throughput. **Visibility timeout**: when a consumer receives a message, it becomes invisible to other consumers for a set period — if the consumer doesn't delete it (acknowledge) within that window, it reappears for redelivery, which is how SQS handles a consumer crashing mid-processing.

### How does it work?
```
Consumer receives message → message hidden for visibility_timeout seconds
  → consumer processes successfully → deletes message (ack)
  → OR consumer crashes/times out → message becomes visible again → redelivered
  → after maxReceiveCount redeliveries without ack → moved to a Dead Letter Queue (DLQ)
```

### Interview Questions
- **L1:** "Visibility timeout hides a received message from other consumers until it's deleted or the timeout expires, giving at-least-once delivery; a DLQ catches messages that fail processing repeatedly, after a configured max receive count."
- **L2:** explain precisely why "at-least-once" (SQS's real guarantee) requires your consumer logic to be **idempotent** — a message can be redelivered (e.g., the consumer processed it successfully but crashed *before* deleting it), so the same message might be handled twice; the system's correctness depends on handling that safely, not on SQS somehow preventing it.
- **L3 — 🔴 directly relevant to Hermes's Outbox chain (DynamoDB Streams → SQS → EventBridge):** explain what happens if the EventBridge publish step fails after SQS delivery but before acknowledgment — the message stays in SQS (visibility timeout expires, redelivered), so the retry naturally re-attempts the EventBridge publish; this is precisely why the chain's ordering (SQS in the middle) gives you built-in retry-on-failure semantics for that hop, as long as the EventBridge publish and the SQS delete are correctly sequenced (delete only after publish succeeds).

### DO NOT SAY THIS
"SQS guarantees exactly-once delivery." — Standard queues explicitly don't; FIFO queues guarantee exactly-once *processing within a 5-minute deduplication window*, using either content-based deduplication or an explicit deduplication ID — still worth stating the actual mechanism rather than the word "guarantee" alone.

### Quick Revision
- Standard: at-least-once, best-effort order, high throughput. FIFO: ordered, dedup-window exactly-once, lower throughput.
- Visibility timeout: the mechanism behind redelivery-on-consumer-failure.
- Consumer logic must be idempotent — at-least-once delivery is the real guarantee, redelivery is expected, not exceptional.
- DLQ: catches messages that exceed max receive attempts.
