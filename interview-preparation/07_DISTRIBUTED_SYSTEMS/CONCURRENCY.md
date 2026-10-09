# Distributed Concurrency Control

🔴🔴 Directly defends Hermes's headline claim: 100 concurrent appends, exactly 1 succeeds, 99 rejected with `ConcurrencyException`.

---

## Cluster 1: Optimistic vs. Pessimistic Concurrency Control

### What is it?
**Pessimistic**: acquire a lock before touching a resource, preventing any conflicting concurrent access — safe by construction, but serializes access and can create contention/deadlock risk. **Optimistic**: don't lock; instead, proceed assuming no conflict, then check at write time whether the resource changed since you read it — if it did, reject and let the caller retry. Optimistic wins when conflicts are *rare* relative to the volume of operations (avoiding lock overhead on the common, non-conflicting case); pessimistic wins when conflicts are frequent (avoiding wasted work on repeated optimistic failures).

### How does it work? (Optimistic Concurrency — Version-Based)
```
1. Read the event stream for aggregate X, note its current version (e.g., version = 42)
2. Prepare a new event to append, tagged as "expected version 42"
3. Write conditionally: "append this event ONLY IF the current version is still 42"
   - DynamoDB: a conditional write (ConditionExpression) checking the version attribute
4. If another writer already appended (version is now 43), the conditional write FAILS
   → your code catches this and throws/returns a ConcurrencyException
```

### Interview Questions
- **L1:** "Optimistic concurrency control lets multiple writers proceed without locking, then checks at write time whether the underlying data changed since it was read — a version mismatch means someone else won the race, and the loser is rejected and can retry."
- **L2:** explain why this is a natural fit for an **event-sourced** system specifically: each aggregate's event stream has a natural version number (its current event count/sequence), so "is this still the version I read?" is a cheap, precise check — no separate locking infrastructure needed, just a conditional write against a version attribute already present in your data model.
- **L3 — 🔴 the exact mechanism behind your "100 concurrent, 1 succeeds, 99 rejected" test:** **Verified against your actual code** (`shared/event-store/src/index.ts`): this is implemented as **two sequential conditional `PutCommand` calls**, not a single atomic `TransactWriteItems`. First, a conditional `PutCommand` on a META item — `ConditionExpression: 'attribute_not_exists(version) OR version = :expectedVersion'` — this is what actually resolves the race; DynamoDB serializes writes to the same item, so of 100 concurrent attempts with the same expected version, exactly one succeeds here and the rest get a conditional-check failure, translated into your `ConcurrencyException`. Second, a separate conditional `PutCommand` on the event item itself (`ConditionExpression: 'attribute_not_exists(SK)'`) prevents overwriting an existing event at that sequence number. **Precise, honest framing if asked "is this atomic?":** the concurrency *resolution* is atomic (the META write is the sole arbiter of who wins); the two writes together are not wrapped in one transaction, so a crash between them is a narrow, real, recoverable edge case (a claimed version with no corresponding event) — a deliberate tradeoff avoiding the extra cost of a full transaction for what's usually a two-item write, not an oversight.

### Follow-up Chain
> **Q: How does your system guarantee exactly one of the 100 concurrent appends succeeds?**
> A: Each append is a conditional write against the aggregate's current version; only one of the 100 competing writes can see the version still match its expected value.
> **Follow-up: Is the whole append — claiming the version AND writing the event — one atomic operation?**
> A: No, and it's important to be precise here: it's two sequential conditional writes. The first (on a META item tracking version) is what resolves the concurrency race atomically. The second (writing the actual event) is a separate conditional write guarding against overwriting an existing sequence number. A crash between the two leaves a claimed-but-unwritten version — narrow, real, and an accepted tradeoff rather than an unknown gap.
> **Trap: "Optimistic concurrency control has no cost since it doesn't use locks."**
> Correct response: it trades lock overhead for wasted work on conflict — 99 callers in your test did real work (reading, preparing an event) that was then thrown away and must be retried; optimistic concurrency is a good trade specifically when conflicts are the *exception*, not the norm — under sustained heavy contention on the same aggregate, it can actually do more total work than a pessimistic lock would.

### Quick Revision
- Optimistic: no lock, check-and-reject on write, good for rare conflicts. Pessimistic: lock upfront, good for frequent conflicts.
- Version-based conditional writes are the standard optimistic-concurrency mechanism for event-sourced aggregates.
- The underlying store's atomic single-item conditional write is what actually guarantees exactly one winner — not application-level coordination.
- Under sustained heavy contention, optimistic concurrency can waste more work than pessimistic locking — know this tradeoff, don't oversell "no locks = free."
