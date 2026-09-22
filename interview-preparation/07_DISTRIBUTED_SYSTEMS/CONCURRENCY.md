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
- **L3 — 🔴 the exact mechanism behind your "100 concurrent, 1 succeeds, 99 rejected" test:** walk through precisely what happens: 100 threads/processes all read the same starting version (say, version 5), all prepare an append expecting version 5, and all issue a conditional write simultaneously. DynamoDB (or whichever store) processes writes to the same item serially at the storage layer — so exactly one conditional write's "expected version = 5" check succeeds (and the version atomically becomes 6), and by the time each of the other 99 writes is evaluated, the actual version is already 6 (or higher, depending on internal ordering), so their "expected version = 5" condition fails, and all 99 receive a conditional-check-failure, which your application code translates into a `ConcurrencyException`. **`NEEDS VERIFICATION`**: confirm this is genuinely how your event store's write path works (DynamoDB `ConditionExpression` on a version attribute is the standard, expected implementation) before stating it as fact — but this is the textbook-correct explanation of what your test result implies.

### Follow-up Chain
> **Q: How does your system guarantee exactly one of the 100 concurrent appends succeeds?**
> A: Each append is a conditional write against the aggregate's current version; only one of the 100 competing writes can see the version still match its expected value.
> **Follow-up: What guarantees the underlying store actually serializes these conditional writes correctly — couldn't two "wins" happen simultaneously on truly parallel hardware?**
> A: This relies on the underlying store's own consistency guarantee for single-item conditional writes — DynamoDB (and similarly, a relational database's row-level compare-and-set) guarantees that writes to the *same item/row* are linearized (processed one at a time in some order), even under massive external parallelism; the conditional-write primitive itself is what the store guarantees atomically, not something your application code has to build from scratch.
> **Trap: "Optimistic concurrency control has no cost since it doesn't use locks."**
> Correct response: it trades lock overhead for wasted work on conflict — 99 callers in your test did real work (reading, preparing an event) that was then thrown away and must be retried; optimistic concurrency is a good trade specifically when conflicts are the *exception*, not the norm — under sustained heavy contention on the same aggregate, it can actually do more total work than a pessimistic lock would.

### Quick Revision
- Optimistic: no lock, check-and-reject on write, good for rare conflicts. Pessimistic: lock upfront, good for frequent conflicts.
- Version-based conditional writes are the standard optimistic-concurrency mechanism for event-sourced aggregates.
- The underlying store's atomic single-item conditional write is what actually guarantees exactly one winner — not application-level coordination.
- Under sustained heavy contention, optimistic concurrency can waste more work than pessimistic locking — know this tradeoff, don't oversell "no locks = free."
