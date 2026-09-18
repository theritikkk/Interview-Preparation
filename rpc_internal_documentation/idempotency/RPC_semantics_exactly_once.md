# RPC Failure Semantics: Why Exactly-Once Delivery Is Impossible

### With the actual duplicate-execution bug demonstrated live, and the real fix that production systems use

---

## The claim this document defends

**Exactly-once *delivery* is not a hard engineering problem that nobody has gotten around to solving well yet. It is impossible, in the strict sense that no amount of cleverness over an unreliable network can achieve it.** This is a foundational result in distributed systems, not an implementation shortcoming — and understanding *why* it's impossible is what makes the real-world workarounds (which absolutely do exist, and work well) make sense, rather than looking like hacks papering over a gap that "should" be closable.

To make this concrete rather than abstract, this document includes a small RPC server and client — compiled and run for real — that reproduces the actual bug this impossibility causes, side by side with the standard fix, so you can see both the failure and its mitigation happen live rather than taking either on faith.

## 1. The core ambiguity, stated precisely

A client sends an RPC request and, after some time, has received no reply. From the client's position, there is **no way to distinguish between these three situations**:

1. The request never arrived at the server at all (lost in transit, or the server was down).
2. The request arrived, the server is still working on it (it just hasn't finished yet).
3. The request arrived, the server finished processing it — **possibly with real, irreversible side effects already committed** — and the *reply* was what got lost on the way back.

All three produce the exact same observation at the client: silence. The client has to decide what to do — give up, or retry — with genuinely no information available to distinguish "nothing happened yet" from "something already happened, but I wasn't told."

This is closely related to the classical **Two Generals Problem**: two allied generals, on opposite sides of an enemy camp, need to coordinate a simultaneous attack, communicating only via messengers who might be captured (lost messages). It's a long-standing, formally proven result that **no protocol, using any finite number of messages over a channel that can lose messages, can let both generals reach guaranteed common knowledge that the attack will happen** — every acknowledgment itself needs its own acknowledgment, infinitely. RPC's "did my call actually run?" problem is the exact same structure: the reply is itself just another message that can be lost, and no amount of "reply to the reply" fixes it, because *that* message can be lost too.

## 2. The three real choices — there is no fourth

Given that ambiguity can never be fully eliminated, every real RPC system has to pick one of exactly three behaviors when a call's outcome is unknown:

| Choice | Behavior on ambiguous outcome | Failure mode this introduces |
|---|---|---|
| **At-most-once** | Give up. Never automatically retry. | A call that actually *did* succeed can be reported to the application as a failure — a false negative. |
| **At-least-once** | Retry the identical request until some acknowledgment is received, or a retry budget is exhausted. | A call can genuinely **execute more than once** — this document's live demo below is exactly this failure mode. |
| **"Exactly-once"** | *Not a fourth delivery option* — see §3. | — |

There is no way to get "definitely ran, and definitely ran only once" as a pure delivery guarantee, because that would require solving the Two Generals Problem — provably impossible over a channel that can lose messages (and every real network can lose messages, or at minimum delay them long enough to be indistinguishable from loss at the application layer).

## 3. What "exactly-once" *actually* means in real systems — and it isn't delivery

Every real system that advertises "exactly-once" — Kafka's transactional producers, Stripe's payment API, any well-designed RPC framework's own documentation — is **not** claiming to have solved §1's ambiguity. They're solving a **different, adjacent** problem: given that a request might genuinely be *delivered and processed* more than once (i.e., accepting at-least-once delivery as the honest baseline), make sure the **observable effect** still only happens once. This is sometimes more precisely called **effectively-once** semantics: the sender retries until it gets an answer (at-least-once delivery, honestly accepted as unavoidable), and the *receiver* is responsible for collapsing duplicates so that whatever changed in the world as a result only changed once.

The mechanism is always some version of the same idea: **a deduplication key, checked and stored atomically alongside whatever the call actually does.**

### Idempotency keys — the standard, industry-wide pattern

A client generates a unique key for each *logical* operation (not each network attempt — the same key is reused across retries of the same logical request) and sends it along with the call. The server, before doing anything else, checks whether it has already seen this exact key:

- **Not seen before** → process the request normally, and atomically record the key alongside the result.
- **Already seen** → don't re-run anything — return the *previously recorded* result directly.

This is precisely what Stripe's `Idempotency-Key` header does for payment API calls (deliberately chosen as the canonical real-world example, since "did this charge actually happen?" is exactly the kind of ambiguity where guessing wrong in either direction is genuinely costly — silently double-charging a customer, or wrongly believing a charge failed when it actually succeeded) — and it's the same underlying pattern behind Kafka's idempotent-producer feature (each producer tags its own messages with a sequence number; the broker rejects/discards a message it's already durably stored from that same producer) and Kafka's transactional exactly-once semantics (which, worth noting precisely, is explicitly scoped to the **consume-process-produce loop happening entirely within Kafka** — the moment a processing step calls out to something *external* — a third-party API, a different database, an email service — that external call is back to being an ordinary at-least-once problem, needing its *own* idempotency handling, exactly as this document's demo shows).

## 4. The bug, reproduced live

Here's a minimal RPC server implementing a single procedure, `INCR(key)`, which increments a global counter and returns its new value — and a client that deliberately reproduces §1's exact ambiguity: it sends a call, then **abandons the connection without reading the reply**, simulating a timeout or a lost reply. Not knowing whether the original attempt succeeded, it retries with the identical key.

**The unsafe server — no deduplication at all:**

```c
static int32_t handle_incr(const char *key) {
    g_counter++;
    printf("  [server] key=\"%s\" is new -- incrementing counter to %d\n", key, g_counter);
    return g_counter;
}
```

**The client, deliberately reproducing the ambiguity:**

```c
/* Attempt 1: send the call, then abandon the connection WITHOUT
   reading a reply -- indistinguishable, from the server's point of
   view, from a reply that was sent but lost in transit. */
send_framed(sock, out.buf, (uint32_t)out.len);
close(sock);   /* <-- no recv_framed() call at all */

/* Not knowing whether attempt 1 succeeded, retry with the SAME key */
int32_t result = call_incr(port, "txn-abc123", /* abandon = */ 0);
```

**Real output — the bug, happening live:**

```
$ ./idem_server 9600 &
[server] mode=UNSAFE (no dedup), listening on port 9600, counter starts at 0

$ ./idem_client 9600
[client] Attempt 1: call INCR("txn-abc123") -- but we abandon
         the connection before reading the reply.
  [client] (connection abandoned before reading a reply --
            simulates a lost reply / client-side timeout)

[client] Not knowing whether attempt 1 actually succeeded,
         we retry with the SAME idempotency key: "txn-abc123"
[client] retry of txn-abc123 returned: 2

[client] Now a genuinely NEW, distinct request: INCR("txn-xyz789")
[client] txn-xyz789 returned: 3

[client] Correct final state: exactly TWO distinct increments happened
         (txn-abc123 once, txn-xyz789 once) -- so the counter should be 2.
```

**And the server's own log, confirming exactly what happened:**
```
[server] mode=UNSAFE (no dedup), listening on port 9600, counter starts at 0

  [server] key="txn-abc123" is new -- incrementing counter to 1
  [server] key="txn-abc123" is new -- incrementing counter to 2
  [server] key="txn-xyz789" is new -- incrementing counter to 3
```

**The counter ended at 3. It should have ended at 2.** The server genuinely received and fully processed the "abandoned" attempt 1 — it wasn't lost at all, only its *reply* was (by the client's own design, deliberately, to simulate exactly that scenario) — and then processed the identical-key retry as if it were a completely new, unrelated request, incrementing a second time for what was logically one operation. This is §2's at-least-once failure mode, not a contrived edge case — it's the direct, inevitable consequence of "the client couldn't tell if it worked, so it retried," which is the *only* reasonable thing a client can do when it has no other information.

## 5. The fix, reproduced live — same client, same ambiguity, different server

The client above is completely unchanged. Only the server gains a dedup cache, checked *before* touching the counter at all:

```c
static int32_t handle_incr(const char *key) {
    int32_t cached = find_cached(key);
    if (cached >= 0) {
        printf("  [server] key=\"%s\" ALREADY SEEN -- returning cached result %d "
               "WITHOUT incrementing (counter stays at %d)\n", key, cached, g_counter);
        return cached;
    }
    g_counter++;
    store_cached(key, g_counter);
    return g_counter;
}
```

**Real output — identical client behavior, correct final result:**

```
$ ./idem_server 9601 --dedup &
[server] mode=SAFE (idempotency-key dedup), listening on port 9601, counter starts at 0

$ ./idem_client 9601
[client] Attempt 1: call INCR("txn-abc123") -- but we abandon
         the connection before reading the reply.
  [client] (connection abandoned before reading a reply --
            simulates a lost reply / client-side timeout)

[client] Not knowing whether attempt 1 actually succeeded,
         we retry with the SAME idempotency key: "txn-abc123"
[client] retry of txn-abc123 returned: 1

[client] Now a genuinely NEW, distinct request: INCR("txn-xyz789")
[client] txn-xyz789 returned: 2

[client] Correct final state: exactly TWO distinct increments happened
         (txn-abc123 once, txn-xyz789 once) -- so the counter should be 2.
```

```
[server] mode=SAFE (idempotency-key dedup), listening on port 9601, counter starts at 0

  [server] key="txn-abc123" is new -- incrementing counter to 1
  [server] key="txn-abc123" ALREADY SEEN -- returning cached result 1 WITHOUT incrementing (counter stays at 1)
  [server] key="txn-xyz789" is new -- incrementing counter to 2
```

**The counter correctly ends at 2.** The retry of `txn-abc123` was recognized as a duplicate of an already-completed operation and got back the *original* result (`1`) without touching the counter a second time. Notice what didn't change: the network is exactly as unreliable, the client's behavior is byte-for-byte identical, and the ambiguity from §1 is **still completely present and unsolved** — the server genuinely has no way to know, at the moment the retry arrives, whether this is "the same client asking again after a lost reply" versus "a different client that happens to have generated an identical key" (which is exactly why real systems generate keys with enough entropy — UUIDs, or a hash of a truly unique business fact — that a collision between two genuinely different logical operations is vanishingly unlikely). What changed is that the *effect* — the thing that actually matters to whoever's waiting on this call — is now protected against the ambiguity, rather than the ambiguity itself being eliminated (which, per §1–§2, cannot be done).

## 6. Designing for this in practice

- **Prefer naturally idempotent operations wherever the domain allows it.** "Set the account balance to $500" can be retried any number of times with an identical, correct outcome; "add $500 to the account balance" cannot. This document's `INCR` procedure was deliberately chosen to be the *non*-idempotent case specifically to make the bug reproducible — a real system would often simply redesign the operation itself (e.g., "set the value associated with transaction ID X to $500," which is naturally safe to repeat) rather than reach for a dedup cache at all, whenever the operation's own semantics permit it.
- **When the operation genuinely can't be made naturally idempotent** (incrementing a counter, appending to a log, charging a card once per distinct purchase, not per retry) — an explicit idempotency key, generated once by the client and reused across every retry of the *same logical* request, checked atomically against a dedup store before the underlying effect runs, is the standard, well-proven answer. The key generation matters: it must be stable across retries and derived from the actual business operation (an order ID, a hash of the canonical request), not freshly minted on every retry attempt — a client that generates a *new* random key on each retry defeats the entire mechanism, since the server would see what looks like a stream of distinct, never-before-seen requests.
- **The dedup check and the actual effect must be atomic with respect to each other** — checking "have I seen this key?" and then separately performing the effect, as two independent steps, reopens a narrower version of the exact same ambiguity (a crash between the check and the effect). Real production implementations typically use a single atomic database operation (an `INSERT ... ON CONFLICT`, a unique constraint, or an equivalent) covering both the "record that I've seen this key" and "commit the effect" in one indivisible step.
- **TCP's own reliability does not solve this problem**, and is worth explicitly ruling out as a fix: TCP guarantees that *if* a connection completes normally, the bytes that crossed it arrived exactly once, in order — but it says nothing at all about what happens when a *connection* is abandoned, times out, or the client-level retry happens over a brand-new connection (exactly what this document's demo does). TCP operates one layer below the "did my RPC call's logical effect happen" question — it can guarantee a stream of bytes wasn't corrupted or duplicated *within one connection*, but it has no concept of "this new connection is actually a retry of that old one," which is precisely the layer at which idempotency keys operate instead.

---

## Quick recap (for revision)

- The ambiguity — "did my call actually run, or not?" — is permanently unresolvable from the client's side alone, over any network that can lose or delay messages; this is the same structure as the classical, formally-proven-impossible Two Generals Problem.
- Every real system chooses exactly one of at-most-once (silent false negatives possible) or at-least-once (duplicate execution possible) as its actual delivery behavior — there is no third, pure "exactly-once delivery" option.
- **"Exactly-once" as advertised by real systems (Kafka, and every idempotency-key-based API) means effectively-once: at-least-once delivery, PLUS receiver-side deduplication that makes the observable effect happen only once** — not a solution to §1's ambiguity, a way of making that ambiguity harmless.
- **Demonstrated live, twice**: the identical client, retrying an identical call after an identical simulated lost reply, produces a genuinely wrong result (counter ends at 3, not 2) against an undefended server, and the exactly correct result (counter ends at 2) against a server with nothing changed except an idempotency-key dedup cache.
- Prefer naturally idempotent operation design first; reach for an explicit, stable, atomically-checked idempotency key only when the operation's own semantics genuinely can't be made safe to repeat.
- TCP's reliability operates one layer below this problem entirely — it protects bytes within one connection, and has no mechanism at all for recognizing a brand-new connection as a retry of an old one.