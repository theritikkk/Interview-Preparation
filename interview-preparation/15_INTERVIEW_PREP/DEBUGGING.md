# Debugging Training — Reading Unfamiliar Code

A systematic process, not a talent — this is exactly what "tell me about debugging something unfamiliar" (`14_BEHAVIORAL/HR.md`) is testing whether you have.

---

## The Process

1. **Understand folder structure** — where's the entry point, where's config, where's business logic vs. infrastructure code.
2. **Find the entry point** — where does execution actually start (`main`, a server's listen call, a Lambda handler).
3. **Trace one request end-to-end** — pick the simplest realistic request and follow it through every layer by hand before touching anything.
4. **Identify dependencies** — what external systems (DB, cache, queue, other services) does this code touch.
5. **Understand data flow** — what shape does data take at each layer, where does it transform.
6. **Find state changes** — what's mutated, where, and by what.
7. **Trace database access** — what queries run, when, and are they in a transaction.
8. **Trace external service calls** — what happens on failure/timeout.
9. **Identify concurrency** — is this code ever called from multiple threads/processes at once; what's shared.
10. **Locate likely bugs** — given the above, where does the actual symptom most plausibly originate.
11. **Read logs** — what does the system actually say happened, not just what you'd guess.
12. **Form a hypothesis** — state it explicitly, in one sentence, before testing it.
13. **Verify the hypothesis** — the smallest possible test that would prove or disprove it (a log line, a breakpoint, a targeted query) — resist the urge to "fix" before confirming the actual cause.
14. **Fix, and verify the fix addresses the root cause**, not just the symptom.

The single most common failure mode under interview pressure: skipping step 12 (explicit hypothesis) and jumping straight to trying random changes — narrate your hypothesis out loud before testing it; it's a strong, visible signal of systematic thinking.

---

## Example Bug Scenarios by Stack

**React:** a list item's checkbox state appears to "jump" to the wrong item after the list reorders — root cause is almost always an unstable `key` prop (array index instead of a stable ID) — see `05_FRONTEND/REACT.md` Cluster 2.

**Node:** a service's response times degrade under load, but CPU looks fine — check for a synchronous, blocking call (`fs.readFileSync`, a CPU-heavy synchronous loop) stalling the single event-loop thread — see `04_BACKEND/NODE.md` Cluster 1.

**Python:** a function that appends to a list parameter behaves unexpectedly differently across repeated calls — the mutable-default-argument trap — see `01_PROGRAMMING/PYTHON.md` Cluster 1.

**Java:** objects that should be found in a `HashSet` aren't — check whether `equals()` was overridden without `hashCode()` — see `01_PROGRAMMING/JAVA.md` Cluster 2.

**SQL:** a query that should use an index is doing a full table scan — check `EXPLAIN ANALYZE` output, verify the index actually matches the filtered/sorted columns (leftmost-prefix rule for composite indexes) — see `01_PROGRAMMING/SQL.md` Cluster 3.

**Docker:** a container works locally but can't reach another service in production — check whether both containers are actually on the same Docker network, and whether the code is using a hardcoded IP instead of the service name — see `09_DEVOPS/DOCKER.md` Cluster 2.

**Kubernetes:** a Pod keeps restarting — check the liveness probe configuration first (an overly aggressive timeout can restart a Pod that's just slow, not actually dead) — see `09_DEVOPS/KUBERNETES.md` Cluster 2.

**Distributed systems:** two consumers appear to have processed the same message — this is expected under at-least-once delivery (`07_DISTRIBUTED_SYSTEMS/FUNDAMENTALS.md` Cluster 2); the actual bug is almost always that the processing logic wasn't made idempotent, not that "delivery happened twice" (which is normal and expected).

**C/Linux:** a program's memory usage grows slowly over a long run — run Valgrind on a representative workload first to localize the leak to a specific allocation site, rather than guessing from the symptom alone — see `01_PROGRAMMING/C.md` Cluster 3.

### Quick Revision
- The process is 14 steps, but the two most commonly skipped under pressure are: trace one request end-to-end *before* touching code, and state an explicit hypothesis *before* testing a fix.
- Each stack has a "usual suspect" bug class — recognizing the pattern (stale closures, mutable defaults, missing hashCode, unstable keys) is faster than re-deriving from first principles every time.
- At-least-once-delivery duplicate processing is expected system behavior, not a bug — the bug (if any) is in the idempotency handling, not the delivery.
