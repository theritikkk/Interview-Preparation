# Multi-Hop RPC Chains: Trace Propagation, Deadline Enforcement, and the Interceptor Pattern

### A complete, working, compiled-and-run system — deep analysis of every file, real-world use cases, and interview questions

---

## 0. Why this exists, and what problem it actually solves

Every RPC document built so far in this series covered exactly **one call between two parties** — a client and a server. That's not how real systems work. A single request to a real backend commonly fans out through **five, ten, sometimes dozens of internal service calls** before a response comes back — a request hits an API gateway, which calls an auth service, which calls a user service, which calls a database proxy, and so on. Two problems appear the moment you have a *chain* instead of a single call, neither of which shows up at all with just one hop:

1. **"Which log lines, across every one of these services, belong to this one request?"** With ten services each writing their own logs independently, there's no way to reconstruct what happened to one specific request unless something ties all those log lines together. This is what a **trace ID** solves.
2. **"The client gave up waiting 2 seconds ago — why is hop #7 still doing work for an answer nobody's listening for anymore?"** Without a shared, propagated notion of "how much time is left," each hop can only guess at its own local timeout, and work far downstream keeps running long after it's already useless. This is what a **deadline** solves — and specifically an *absolute* deadline, not each hop inventing its own relative timeout.

This document is a from-scratch, minimal, but genuinely working implementation of both mechanisms, plus the **interceptor pattern** that makes them possible to enforce consistently without every single service remembering to reimplement the same logic.

## 1. The architecture, in one picture

```
                    trace_id + absolute deadline, generated ONCE,
                    propagated UNCHANGED through every arrow below

  client ──(chain_call)──► SVC-A ──(chain_call)──► SVC-B ──(chain_call)──► SVC-C
           :9303                    :9302                    :9301
                                                                  │
                                                          (does the real,
                                                           slow work)
```

Every arrow is a real, independent TCP connection, made by the exact same generic `chain_call()` function — a client calling A uses the identical code path that A itself uses to call B, and B uses to call C. There's nothing special about "being a client" versus "being a service that also happens to call another service" — this is the same realization that makes real microservice chains possible to reason about at all: **every hop is simultaneously a server (to whoever called it) and a client (to whoever it calls next)**, and the exact same middleware wraps both roles identically.

## 2. `chain_rpc.h` — the wire format and marshalling primitives

This is the lowest layer: turning real values (integers, 64-bit IDs, timestamps) into an unambiguous byte sequence, and back.

### Wire format

```
CALL:   xid(4) | msg_type(4) | trace_id(8) | deadline_ms(8) | program(4) | version(4) | procedure(4) | arg_len(4) | args...
REPLY:  xid(4) | msg_type(4) | status(4)   | result_len(4)  | result...
```

Compare this to the base `rpc.h` from the very first document in this series: the only *new* fields are `trace_id` (8 bytes, a `uint64_t`) and `deadline_ms` (8 bytes, an `int64_t`) — everything else is identical in spirit. This is deliberate: **propagating tracing/deadline context is not a fundamentally different kind of RPC feature; it's just two more fields riding along in the same header**, which is exactly how real systems do it too (gRPC carries deadlines as an HTTP/2 header, `grpc-timeout`; distributed tracing systems like OpenTelemetry carry trace context in headers like `traceparent`).

### Function-by-function

| Function | What it does | Why it exists |
|---|---|---|
| `xbuf_init()` | Sets up a cursor (`buf`, `cap`, `len`) over a raw byte buffer | The same "cursor into a buffer" abstraction used throughout every document in this series — tracks how much has been written/read so far |
| `put_u32()` / `get_u32()` | Encode/decode a 4-byte big-endian unsigned integer | The base primitive everything else is built from — same network-byte-order principle as `htonl()` in ordinary sockets |
| `put_u64()` / `get_u64()` | Encode/decode an 8-byte value as **two** `put_u32()`/`get_u32()` calls | There's no native 64-bit network-byte-order primitive in plain C the way there is for 32-bit (`htonl`) — so it's built by hand from two halves, high word first, exactly mirroring how real 64-bit serialization is often implemented when a language/library doesn't hand you one for free |
| `put_i64()` / `get_i64()` | Same as the `u64` versions, just reinterpreting the bits as signed | The deadline is stored as signed because "now minus deadline" needs to be able to go *negative* (a deadline in the past) — using an unsigned type here would make that arithmetic silently wrap around to a huge positive number instead, a real, easy-to-miss bug class |
| `now_ms()` | Returns the current absolute time as milliseconds since the Unix epoch, via `gettimeofday()` | This is the single function that makes "absolute deadline" meaningful at all — every hop calls this same function to check "am I past the deadline yet?", and it's what the client calls once, up front, to establish the deadline in the first place |

## 3. `chain_framing.h` — solving "where does one message end"

Identical in spirit to every earlier framing implementation in this series: a 4-byte big-endian length prefix, with `send_all()`/`recv_all()` looping to guarantee a full read/write regardless of how many actual `send()`/`recv()` syscalls it takes underneath. Nothing new conceptually here — it's included as its own file specifically so `chain_middleware.h` doesn't need to know anything about *how* bytes get from one process to another, only that `send_framed()`/`recv_framed()` reliably deliver one complete logical message. This separation (framing vs. content) is itself a small instance of the same "layer things so each piece only knows what it needs to" principle that shows up everywhere in networking.

## 4. `chain_middleware.h` — the actual subject of this document

This is where the interesting engineering lives. Two functions, both worth understanding completely.

### `chain_call()` — the generic outbound half

```c
static uint32_t chain_call(const char *host, int port,
                            uint64_t trace_id, int64_t deadline_ms,
                            uint32_t program, uint32_t version, uint32_t procedure,
                            xbuf_t *argx, uint8_t *resbuf, uint32_t resbuf_cap, xbuf_t *resx,
                            const char *from_svc) {

    int64_t remaining = deadline_ms - now_ms();
    if (remaining <= 0) {
        /* refuse to even open a connection */
        return STATUS_DEADLINE_EXCEEDED;
    }
    /* ...socket()/connect()... */
    /* ...marshal trace_id, deadline_ms, and the args onto the wire... */
    /* ...send_framed(), recv_framed()... */
    /* ...unmarshal the reply's status and result... */
    return status;
}
```

**Every single caller in this whole system — the client calling A, A calling B, B calling C — goes through this exact same function.** That's the point: the logic for "check the deadline before bothering to connect at all" and "put the trace_id and deadline on the wire correctly" is written **once**, and every hop gets it automatically just by using this function instead of hand-rolling its own socket code. This is precisely why real frameworks (gRPC, Java's gRPC-Java, ASP.NET's `HttpClient`) push developers hard toward using the framework's own call mechanism rather than raw sockets — it's not merely convenience, it's the only way to *guarantee* every call in a large system consistently propagates this kind of cross-cutting context.

### `serve_forever()` — the generic inbound half, and the actual interceptor

```c
while (1) {
    /* accept(), recv_framed() ... */
    /* unmarshal xid, trace_id, deadline_ms, ... */

    int64_t remaining = deadline_ms - now_ms();
    if (remaining <= 0) {
        status = STATUS_DEADLINE_EXCEEDED;   /* handler NEVER RUNS */
    } else {
        status = handler(trace_id, deadline_ms, &argx, &resx, svcname);

        if (status == STATUS_OK && now_ms() > deadline_ms) {
            status = STATUS_DEADLINE_EXCEEDED;   /* caught AFTER the fact too */
        }
    }
    /* marshal reply, send_framed() */
}
```

This is a textbook **interceptor** (also called middleware, in other ecosystems): a wrapper that runs identically for *every* incoming call, regardless of which specific service or procedure it's for, doing generic work **before and after** calling into the one thing that's actually specific to this service — the `handler` function pointer. The service-specific code (`handler_a`, `handler_b`, `handler_c`) never has to remember to check the deadline itself; it's structurally impossible to skip that check, because it happens in the one shared function every service is built from.

**The two-sided deadline check is the detail worth sitting with.** The first check (before calling `handler`) is cheap insurance against wasted work: if the deadline's already gone, don't even try. The second check (after calling `handler`, before replying) exists because **the first check alone is not enough** — a hop can pass the first check (deadline hasn't happened *yet*) and then spend so long doing its own work that the deadline expires *during* that work. Without the second check, that hop would happily report `STATUS_OK` for a result that's already too late to be useful to anyone. This exact bug appeared in this document's own first draft — the initial version only had the first check, and a deliberate test (a 200ms budget against a service that takes 300ms to do its work) came back reporting success, incorrectly. Real bug, really caught, by actually running the test rather than assuming the design was correct.

## 5. `svc_c.c`, `svc_b.c`, `svc_a.c` — the business logic, and how little of it there is

```c
static uint32_t handler_c(uint64_t trace_id, int64_t deadline_ms,
                           xbuf_t *argx, xbuf_t *resx, const char *svcname) {
    int32_t x = (int32_t)get_u32(argx);
    usleep(300000);              /* the ONLY "real work" in this whole demo */
    int32_t result = x * 2;
    put_u32(resx, (uint32_t)result);
    return STATUS_OK;
}
```

Notice `handler_c` never touches `trace_id` for logging purposes beyond what's shown, never checks the deadline itself, never deals with sockets, framing, or marshalling headers at all — every one of those concerns is already handled by `serve_forever()` before this function is ever called. `handler_b` and `handler_a` are nearly identical in shape, except their "work" is calling the *next* hop via `chain_call()`, propagating the exact same `trace_id` and `deadline_ms` parameters they themselves received — **not** generating new ones. This is the single most important line in both files, worth stating explicitly: `deadline_ms` here is a parameter received from the caller, passed straight through to `chain_call()` unchanged — never `now_ms() + something`. Regenerating a fresh relative timeout at each hop is exactly the bug that makes deadline propagation not work at all in real systems (see §7's interview questions).

## 6. `chain_client.c` — where the trace begins

```c
uint64_t trace_id = ((uint64_t)rand() << 32) | (uint64_t)rand();
int64_t deadline_ms = now_ms() + budget_ms;
```

Exactly two values, generated **exactly once**, at the true start of the whole chain. Everything downstream only ever reads and forwards these — nothing downstream ever creates a new trace ID or a new deadline. This is the root of the whole tree in §1's diagram.

## 7. Real execution — both outcomes, actually run

**Generous budget (2000ms) — the success path:**
```
[client] trace=0bd0705744dfba1e: starting chain call with input=10, total budget=2000ms
  [client] trace=0bd0705744dfba1e: -> calling downstream (deadline in 2000ms)
[client] trace=0bd0705744dfba1e: SUCCESS, result=22 (took 301ms)
```
```
[SVC-C] trace=0bd0705744dfba1e: incoming call, 1999ms left on the deadline
  [SVC-C] trace=0bd0705744dfba1e: doing real work on input 10 (this takes 300ms)...
  [SVC-C] trace=0bd0705744dfba1e: work done, result=20
[SVC-B] trace=0bd0705744dfba1e: got 20 from SVC-C, returning 21
[SVC-A] trace=0bd0705744dfba1e: got 21 from SVC-B, returning 22
```
The **identical** trace ID, `0bd0705744dfba1e`, appears in all four processes' independent log output — this is the entire value proposition of a trace ID, made directly visible.

**Tight budget (200ms, less than SVC-C's own 300ms of work) — the failure path:**
```
[client] trace=3a2556253a2ffd74: starting chain call with input=10, total budget=200ms
[client] trace=3a2556253a2ffd74: DEADLINE EXCEEDED somewhere in the chain (took 301ms, budget was 200ms)
```
```
[SVC-C] trace=3a2556253a2ffd74: incoming call, 200ms left on the deadline
  [SVC-C] trace=3a2556253a2ffd74: doing real work on input 10 (this takes 300ms)...
  [SVC-C] trace=3a2556253a2ffd74: work done, result=20
[SVC-C] trace=3a2556253a2ffd74: handler finished, but the deadline passed WHILE we
        were working -- downgrading reply to DEADLINE_EXCEEDED instead of OK

[SVC-B] trace=3a2556253a2ffd74: downstream call failed, status=1 --
        propagating failure upward, NOT doing any more work
[SVA-A] trace=3a2556253a2ffd74: downstream call failed, status=1 --
        propagating failure upward to the client
```
C still does its full 300ms of (now-wasted) work — this simple demo doesn't implement mid-flight cancellation (see §9) — but critically, **B and A do zero extra work once they see the failure**: no "B's own +1 contribution," no "A's own +1 contribution," just immediate propagation upward. In a chain of 10 services instead of 3, this is the difference between one wasted 300ms operation and ten.

## 8. Features, summarized

- **Absolute, not relative, deadline propagation** — every hop shares one fixed point in time, not a per-hop reinvented budget.
- **Trace ID correlation across process boundaries** — one opaque value, generated once, threading through independently-logging processes.
- **The interceptor pattern** — cross-cutting concerns (deadline checking, tracing) implemented exactly once, structurally impossible for a service author to forget.
- **Fail-fast without wasted downstream work** — a hop that already knows the deadline is gone refuses before connecting to anything further.
- **Post-hoc deadline re-validation** — catching the case where a hop's own work, not any downstream call, is what burned the remaining budget.
- **Uniform client/server code path** — `chain_call()` is used identically whether the caller is "the client" or "a service calling the next service" — there is no special-cased "top-level client" logic anywhere.

## 9. What's genuinely missing — real, honest gaps versus production systems

This is a minimal, teaching-sized implementation. Naming what it doesn't do is at least as informative as what it does:

- **No cancellation, only deadline-checking.** A real system (gRPC's `context.Context` in Go, for instance) can actively **interrupt** a hop that's already running once a deadline passes or the caller explicitly cancels — this demo's SVC-C always runs its full 300ms `usleep()` to completion even when the reply will be thrown away. Implementing real cancellation needs either cooperative checkpoints inside long-running work, or a separate out-of-band signal (a second connection, or a signal handler) telling an in-progress handler to abort.
- **No retries, no circuit breakers.** A transient failure (a dropped connection, one bad response) here just fails the whole chain immediately. Production systems layer retry policies (with backoff, and — tying directly back to this series' own semantics document — idempotency keys to make retries safe) and circuit breakers (stop even trying to call a downstream service that's been failing repeatedly, rather than adding load to something already struggling) on top of exactly this kind of call.
- **No connection pooling.** Every single `chain_call()` opens a brand-new TCP connection and tears it down immediately after. A real system reuses long-lived connections (or, in gRPC's case, multiplexed HTTP/2 streams on one connection) to avoid paying a full TCP handshake's latency on every call.
- **No transport security.** Plaintext TCP, no TLS, no authentication of any kind between hops — a real service mesh (Istio, Linkerd) typically wraps every one of these hops in mutual TLS automatically, without any application code needing to know.
- **No load balancing or service discovery.** Every hop hardcodes `"127.0.0.1"` and a fixed port. A real system looks up "where is SVC-C right now" dynamically (DNS, a service registry, or a sidecar proxy) and can spread calls across multiple instances of the same service.
- **A single `uint64_t` trace ID, not a full trace context.** Real distributed tracing (OpenTelemetry, Jaeger, Zipkin) propagates considerably more: a **span ID** for each individual hop (distinct from the overall trace ID), a parent-span pointer (so the full call tree, not just "these all belong together," can be reconstructed and visualized), and often additional baggage (arbitrary key-value context riding alongside the trace).
- **No structured logging or centralized aggregation.** These demo logs are plain `printf()` to each process's own stdout — a real system ships structured (JSON) logs to a central aggregator (the ELK stack, Splunk, Datadog) specifically so "show me every log line for trace `0bd0705744dfba1e`, across every service" is a single query instead of manually greeping four separate files, as this document's own analysis effectively had to do by hand.

## 10. Where this shows up in real, present-day systems

- **Service meshes** (Istio, Linkerd, Consul Connect) implement almost exactly this pattern — but transparently, via a **sidecar proxy** (typically Envoy) running alongside every service instance. Application code doesn't call `chain_call()` itself at all; the sidecar intercepts outbound and inbound traffic and injects/propagates trace headers and deadlines automatically, which is the production-grade version of "the interceptor pattern is what makes this consistent" — pushed one layer further down, into infrastructure the application doesn't even need to be aware of.
- **gRPC's own deadline propagation** (Go's `context.Context`, Java's `Deadline` class) is directly this same absolute-deadline idea, built into the framework rather than hand-rolled — a `context.WithTimeout()` call establishes an absolute deadline once, and every subsequent gRPC call made using that context automatically carries it forward.
- **OpenTelemetry** is the current, vendor-neutral standard for exactly the trace-ID-and-span propagation problem this demo simplifies down to one integer — its `traceparent` HTTP header (used by both HTTP and gRPC-based systems) is the direct real-world descendant of Google's 2010 Dapper paper, which first published this whole approach at scale.
- **Netflix's Hystrix** (and its modern successors, like resilience4j) is the canonical real implementation of the "circuit breaker" gap named in §9 — deliberately refusing to call a downstream dependency that's been failing, rather than piling more load onto it.
- **AWS X-Ray, Google Cloud Trace, Jaeger, Zipkin** are all production trace-visualization systems that consume exactly this kind of propagated trace/span data to draw the "waterfall diagram" of a request's full path through a distributed system — precisely reconstructing the A→B→C picture from §1, but for real systems with dozens of hops.

## 11. Interview-style questions

Genuinely likely to come up in a systems/backend/distributed-systems interview, given this exact material — questions first, then a concise, defensible answer for each.

**Q: Why propagate an *absolute* deadline instead of having each hop set its own relative timeout?**
A: A relative timeout resets the clock at every hop — if the client wants a 2-second overall budget, and hop 1 takes 1.9 seconds of that before calling hop 2, a *relative* 2-second timeout at hop 2 would let the total request run for nearly 4 seconds, wildly violating what the client actually wanted. An absolute deadline (a fixed point in time, computed once) means every hop, no matter how deep in the chain, always knows the true remaining budget relative to the *original* request, not to its own local start time.

**Q: What's a real risk with using wall-clock time (`gettimeofday()`) for this, especially across multiple machines?**
A: Clock skew. If two machines' clocks disagree by even a few hundred milliseconds, deadline checks become unreliable — a hop on a "slow" clock might think it still has time when the true, client-side deadline has already passed, or vice versa. Real distributed systems either rely on NTP-synchronized clocks and accept some bounded error, or use monotonic clocks plus a separately-communicated "time remaining" value instead of an absolute timestamp, specifically to avoid trusting that every machine's wall clock agrees.

**Q: Why check the deadline both before *and* after calling the handler, instead of just once?**
A: Checking only before guards against calling into work that was already pointless before it started, but says nothing about work that becomes pointless *while* it's running. A hop whose own processing (or one of its downstream calls) takes long enough to blow the budget needs a second check, after that work finishes, or it will report success for a result nobody needed anymore. (This exact gap was caught in this document's own first implementation by actually testing it, not by static reasoning alone — worth mentioning in an interview as a concrete example of testing catching a design gap.)

**Q: This demo lets a "cancelled" call keep running to completion anyway (SVC-C's full 300ms `usleep`, even when its result will be discarded). How would you fix that for real?**
A: True cancellation needs either (a) cooperative cancellation checkpoints — long-running work is broken into chunks, each checking a shared "should I stop?" flag/context before continuing, exactly how Go's `context.Context` cancellation works, or (b) preemptive cancellation via a separate signal — a second, out-of-band message telling an in-flight handler to abort, which is harder to implement correctly and not universally supported. Pure timeout-based "the caller just stops waiting" (what this demo effectively does upstream) never actually stops the downstream work; it only stops the caller from caring about the result.

**Q: What's the interceptor pattern, and why is it better than putting the same check inside every handler function?**
A: An interceptor is a piece of logic that wraps every call generically, so cross-cutting concerns (auth, logging, deadline enforcement, tracing) are implemented in exactly one place rather than duplicated in every individual handler. The practical benefit isn't just avoiding repetition — it's **correctness under change**: if the deadline-checking logic needs to change later, or a new cross-cutting concern (say, rate limiting) needs to be added, it's a single edit to the interceptor rather than needing to correctly update every handler in the codebase, some of which will inevitably be missed.

**Q: How would you make this system's logs actually queryable ("show me everything for trace X"), at scale, across many services?**
A: Structured logging (JSON, not free-text `printf`) with the trace ID as a dedicated, indexed field, shipped to a centralized log aggregation system (the ELK stack, Splunk, Datadog, Google Cloud Logging) rather than left in separate per-process log files. At that point, "everything for trace X" becomes one query against the aggregator's index, rather than the manual multi-file grep this document's own analysis effectively did by hand at small scale.

**Q: Why does `chain_call()` refuse to even open a connection if the deadline has already passed, rather than just letting the downstream service's own check catch it?**
A: Two reasons. First, efficiency — opening a TCP connection (a real three-way handshake) has real cost; refusing before paying that cost is strictly cheaper than paying it and having the other end reject the call a moment later. Second, and more importantly at scale: if a downstream service is already struggling (which is often *why* a deadline is close to expiring in the first place), sending it more doomed-to-fail requests anyway makes the struggling service's load worse, not better — refusing upstream is a small, local instance of the same instinct behind circuit breakers.

**Q: What would you change about this design to support the same request being served by *multiple instances* of SVC-C (for scaling), rather than one fixed address?**
A: Replace the hardcoded `"127.0.0.1", SVC_C_PORT` in `chain_call()`'s call sites with a service-discovery lookup (a DNS query, a call to a service registry, or delegating to a sidecar proxy that handles this transparently) that returns one of potentially several healthy instances — ideally combined with a load-balancing strategy (round-robin, least-connections, or latency-aware) rather than always picking the same one.

---

## 12. Quick recap (for revision)

- Multi-hop chains introduce two problems a single client/server pair never has: correlating logs across independent processes (trace ID), and preventing wasted work on a request nobody's waiting for anymore (absolute deadline propagation).
- Deadlines must be **absolute** (a fixed point in time) and passed through **unchanged** at every hop — regenerating a relative timeout per hop silently multiplies the true end-to-end budget.
- The interceptor pattern (`serve_forever()`'s wrapper around `handler`) is what makes deadline/trace enforcement *structural* rather than a matter of every developer remembering to add the right check.
- Checking a deadline only *before* work starts is insufficient — genuinely verified by this document's own first draft failing a real test — a second check *after* the work, before replying, is also required.
- This whole system is a simplified stand-in for what service meshes (Istio/Envoy), gRPC's built-in deadline propagation, and OpenTelemetry's trace-context standard all do for real, production-scale distributed systems.

---

## Appendix: full source

### `chain_rpc.h`
```c
#ifndef CHAIN_RPC_H
#define CHAIN_RPC_H

#include <stdint.h>
#include <string.h>
#include <sys/time.h>

#define MSG_CALL  0u
#define MSG_REPLY 1u

#define STATUS_OK               0u
#define STATUS_DEADLINE_EXCEEDED 1u
#define STATUS_ERROR            2u

typedef struct { uint8_t *buf; size_t cap; size_t len; } xbuf_t;

static void xbuf_init(xbuf_t *x, uint8_t *buf, size_t cap) {
    x->buf = buf; x->cap = cap; x->len = 0;
}

static void put_u32(xbuf_t *x, uint32_t v) {
    x->buf[x->len++] = (uint8_t)(v >> 24);
    x->buf[x->len++] = (uint8_t)(v >> 16);
    x->buf[x->len++] = (uint8_t)(v >> 8);
    x->buf[x->len++] = (uint8_t)(v);
}
static void put_u64(xbuf_t *x, uint64_t v) {
    put_u32(x, (uint32_t)(v >> 32));
    put_u32(x, (uint32_t)(v & 0xffffffffu));
}
static void put_i64(xbuf_t *x, int64_t v) { put_u64(x, (uint64_t)v); }

static uint32_t get_u32(xbuf_t *x) {
    uint32_t v = ((uint32_t)x->buf[x->len]   << 24) | ((uint32_t)x->buf[x->len+1] << 16) |
                 ((uint32_t)x->buf[x->len+2] << 8)  | ((uint32_t)x->buf[x->len+3]);
    x->len += 4;
    return v;
}
static uint64_t get_u64(xbuf_t *x) {
    uint64_t hi = get_u32(x), lo = get_u32(x);
    return (hi << 32) | lo;
}
static int64_t get_i64(xbuf_t *x) { return (int64_t)get_u64(x); }

static int64_t now_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

#endif
```

### `chain_framing.h`
```c
#ifndef CHAIN_FRAMING_H
#define CHAIN_FRAMING_H

#include <stdint.h>
#include <sys/socket.h>
#include <unistd.h>

static int send_all(int sock, const uint8_t *buf, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(sock, buf + sent, len - sent, 0);
        if (n <= 0) return -1;
        sent += (size_t)n;
    }
    return 0;
}
static int recv_all(int sock, uint8_t *buf, size_t len) {
    size_t got = 0;
    while (got < len) {
        ssize_t n = recv(sock, buf + got, len - got, 0);
        if (n <= 0) return -1;
        got += (size_t)n;
    }
    return 0;
}
static int send_framed(int sock, const uint8_t *msg, uint32_t len) {
    uint8_t hdr[4] = { (uint8_t)(len>>24), (uint8_t)(len>>16), (uint8_t)(len>>8), (uint8_t)len };
    if (send_all(sock, hdr, 4) != 0) return -1;
    return send_all(sock, msg, len);
}
static int32_t recv_framed(int sock, uint8_t *buf, uint32_t maxlen) {
    uint8_t hdr[4];
    if (recv_all(sock, hdr, 4) != 0) return -1;
    uint32_t len = ((uint32_t)hdr[0]<<24)|((uint32_t)hdr[1]<<16)|((uint32_t)hdr[2]<<8)|hdr[3];
    if (len > maxlen) return -1;
    if (recv_all(sock, buf, len) != 0) return -1;
    return (int32_t)len;
}

#endif
```

### `chain_middleware.h`
```c
#ifndef CHAIN_MIDDLEWARE_H
#define CHAIN_MIDDLEWARE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "chain_rpc.h"
#include "chain_framing.h"

#define MAXMSG 4096

typedef uint32_t (*handler_fn)(uint64_t trace_id, int64_t deadline_ms,
                                xbuf_t *argx, xbuf_t *resx, const char *svcname);

static uint32_t chain_call(const char *host, int port,
                            uint64_t trace_id, int64_t deadline_ms,
                            uint32_t program, uint32_t version, uint32_t procedure,
                            xbuf_t *argx, uint8_t *resbuf, uint32_t resbuf_cap, xbuf_t *resx,
                            const char *from_svc) {

    int64_t remaining = deadline_ms - now_ms();
    if (remaining <= 0) {
        printf("  [%s] trace=%016llx: deadline ALREADY passed (%lldms overdue) -- "
               "refusing to even connect downstream\n",
               from_svc, (unsigned long long)trace_id, (long long)(-remaining));
        return STATUS_DEADLINE_EXCEEDED;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    inet_pton(AF_INET, host, &addr.sin_addr);
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        return STATUS_ERROR;
    }

    uint8_t msg[MAXMSG];
    xbuf_t out; xbuf_init(&out, msg, sizeof(msg));
    put_u32(&out, 1); put_u32(&out, MSG_CALL);
    put_u64(&out, trace_id);
    put_i64(&out, deadline_ms);
    put_u32(&out, program); put_u32(&out, version); put_u32(&out, procedure);
    put_u32(&out, (uint32_t)argx->len);
    memcpy(out.buf + out.len, argx->buf, argx->len);
    out.len += argx->len;

    printf("  [%s] trace=%016llx: -> calling downstream (deadline in %lldms)\n",
           from_svc, (unsigned long long)trace_id, (long long)remaining);

    if (send_framed(sock, out.buf, (uint32_t)out.len) != 0) { close(sock); return STATUS_ERROR; }

    uint8_t reply[MAXMSG];
    int32_t n = recv_framed(sock, reply, sizeof(reply));
    close(sock);
    if (n < 0) return STATUS_ERROR;

    xbuf_t in; xbuf_init(&in, reply, (uint32_t)n);
    get_u32(&in); get_u32(&in);
    uint32_t status = get_u32(&in);
    uint32_t rlen = get_u32(&in);
    if (rlen > resbuf_cap) return STATUS_ERROR;
    memcpy(resbuf, reply + in.len, rlen);
    xbuf_init(resx, resbuf, rlen);
    return status;
}

static void serve_forever(int port, const char *svcname, handler_fn handler) {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    int yes = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons((uint16_t)port);
    bind(listener, (struct sockaddr*)&addr, sizeof(addr));
    listen(listener, 10);
    printf("[%s] listening on port %d\n", svcname, port);
    fflush(stdout);

    while (1) {
        struct sockaddr_in cli; socklen_t clilen = sizeof(cli);
        int client = accept(listener, (struct sockaddr*)&cli, &clilen);
        if (client < 0) continue;

        uint8_t msg[MAXMSG];
        int32_t n = recv_framed(client, msg, sizeof(msg));
        if (n >= 0) {
            xbuf_t in; xbuf_init(&in, msg, (uint32_t)n);
            uint32_t xid = get_u32(&in);
            get_u32(&in);
            uint64_t trace_id = get_u64(&in);
            int64_t deadline_ms = get_i64(&in);
            get_u32(&in); get_u32(&in); get_u32(&in);
            uint32_t arg_len = get_u32(&in);
            xbuf_t argx; xbuf_init(&argx, msg + in.len, arg_len);

            int64_t remaining = deadline_ms - now_ms();
            uint32_t status;
            uint8_t resbuf[MAXMSG];
            xbuf_t resx; xbuf_init(&resx, resbuf, sizeof(resbuf));

            printf("[%s] trace=%016llx: incoming call, %lldms left on the deadline\n",
                   svcname, (unsigned long long)trace_id, (long long)remaining);

            if (remaining <= 0) {
                printf("[%s] trace=%016llx: deadline already exceeded -- "
                       "REFUSING without running the handler\n",
                       svcname, (unsigned long long)trace_id);
                status = STATUS_DEADLINE_EXCEEDED;
            } else {
                status = handler(trace_id, deadline_ms, &argx, &resx, svcname);

                if (status == STATUS_OK && now_ms() > deadline_ms) {
                    printf("[%s] trace=%016llx: handler finished, but the deadline "
                           "passed WHILE we were working -- downgrading reply to "
                           "DEADLINE_EXCEEDED instead of OK\n",
                           svcname, (unsigned long long)trace_id);
                    status = STATUS_DEADLINE_EXCEEDED;
                }
            }

            uint8_t reply[MAXMSG];
            xbuf_t out; xbuf_init(&out, reply, sizeof(reply));
            put_u32(&out, xid); put_u32(&out, MSG_REPLY);
            put_u32(&out, status); put_u32(&out, (uint32_t)resx.len);
            memcpy(out.buf + out.len, resx.buf, resx.len);
            out.len += resx.len;
            fflush(stdout);
            send_framed(client, out.buf, (uint32_t)out.len);
        }
        close(client);
    }
}

#endif
```

### `svc_c.c`
```c
#include "chain_middleware.h"

static uint32_t handler_c(uint64_t trace_id, int64_t deadline_ms,
                           xbuf_t *argx, xbuf_t *resx, const char *svcname) {
    (void)deadline_ms;
    int32_t x = (int32_t)get_u32(argx);
    printf("  [%s] trace=%016llx: doing real work on input %d (this takes 300ms)...\n",
           svcname, (unsigned long long)trace_id, x);
    usleep(300000);
    int32_t result = x * 2;
    put_u32(resx, (uint32_t)result);
    printf("  [%s] trace=%016llx: work done, result=%d\n",
           svcname, (unsigned long long)trace_id, result);
    return STATUS_OK;
}

int main(int argc, char *argv[]) {
    int port = argc > 1 ? atoi(argv[1]) : 9301;
    serve_forever(port, "SVC-C", handler_c);
    return 0;
}
```

### `svc_b.c`
```c
#include "chain_middleware.h"

#define SVC_C_PORT 9301

static uint32_t handler_b(uint64_t trace_id, int64_t deadline_ms,
                           xbuf_t *argx, xbuf_t *resx, const char *svcname) {
    int32_t x = (int32_t)get_u32(argx);
    printf("  [%s] trace=%016llx: got %d, calling SVC-C...\n",
           svcname, (unsigned long long)trace_id, x);

    uint8_t argbuf[16];
    xbuf_t cargx; xbuf_init(&cargx, argbuf, sizeof(argbuf));
    put_u32(&cargx, (uint32_t)x);

    uint8_t resbuf[64];
    xbuf_t cresx;
    uint32_t status = chain_call("127.0.0.1", SVC_C_PORT, trace_id, deadline_ms,
                                  0, 0, 0, &cargx, resbuf, sizeof(resbuf), &cresx, svcname);
    if (status != STATUS_OK) {
        printf("  [%s] trace=%016llx: downstream call failed, status=%u -- "
               "propagating failure upward, NOT doing any more work\n",
               svcname, (unsigned long long)trace_id, status);
        return status;
    }

    int32_t c_result = (int32_t)get_u32(&cresx);
    int32_t final = c_result + 1;
    put_u32(resx, (uint32_t)final);
    printf("  [%s] trace=%016llx: got %d from SVC-C, returning %d\n",
           svcname, (unsigned long long)trace_id, c_result, final);
    return STATUS_OK;
}

int main(int argc, char *argv[]) {
    int port = argc > 1 ? atoi(argv[1]) : 9302;
    serve_forever(port, "SVC-B", handler_b);
    return 0;
}
```

### `svc_a.c`
```c
#include "chain_middleware.h"

#define SVC_B_PORT 9302

static uint32_t handler_a(uint64_t trace_id, int64_t deadline_ms,
                           xbuf_t *argx, xbuf_t *resx, const char *svcname) {
    int32_t x = (int32_t)get_u32(argx);
    printf("  [%s] trace=%016llx: got %d, calling SVC-B...\n",
           svcname, (unsigned long long)trace_id, x);

    uint8_t argbuf[16];
    xbuf_t cargx; xbuf_init(&cargx, argbuf, sizeof(argbuf));
    put_u32(&cargx, (uint32_t)x);

    uint8_t resbuf[64];
    xbuf_t cresx;
    uint32_t status = chain_call("127.0.0.1", SVC_B_PORT, trace_id, deadline_ms,
                                  0, 0, 0, &cargx, resbuf, sizeof(resbuf), &cresx, svcname);
    if (status != STATUS_OK) {
        printf("  [%s] trace=%016llx: downstream call failed, status=%u -- "
               "propagating failure upward to the client\n",
               svcname, (unsigned long long)trace_id, status);
        return status;
    }

    int32_t b_result = (int32_t)get_u32(&cresx);
    int32_t final = b_result + 1;
    put_u32(resx, (uint32_t)final);
    printf("  [%s] trace=%016llx: got %d from SVC-B, returning %d\n",
           svcname, (unsigned long long)trace_id, b_result, final);
    return STATUS_OK;
}

int main(int argc, char *argv[]) {
    int port = argc > 1 ? atoi(argv[1]) : 9303;
    serve_forever(port, "SVC-A", handler_a);
    return 0;
}
```

### `chain_client.c`
```c
#include "chain_middleware.h"
#include <time.h>

#define SVC_A_PORT 9303

int main(int argc, char *argv[]) {
    int budget_ms = argc > 1 ? atoi(argv[1]) : 2000;
    int32_t input = argc > 2 ? atoi(argv[2]) : 10;

    srand((unsigned)time(NULL) ^ (unsigned)getpid());
    uint64_t trace_id = ((uint64_t)rand() << 32) | (uint64_t)rand();
    int64_t deadline_ms = now_ms() + budget_ms;

    printf("[client] trace=%016llx: starting chain call with input=%d, "
           "total budget=%dms (absolute deadline=%lld)\n",
           (unsigned long long)trace_id, input, budget_ms, (long long)deadline_ms);

    uint8_t argbuf[16];
    xbuf_t argx; xbuf_init(&argx, argbuf, sizeof(argbuf));
    put_u32(&argx, (uint32_t)input);

    uint8_t resbuf[64];
    xbuf_t resx;
    int64_t t0 = now_ms();
    uint32_t status = chain_call("127.0.0.1", SVC_A_PORT, trace_id, deadline_ms,
                                  0, 0, 0, &argx, resbuf, sizeof(resbuf), &resx, "client");
    int64_t elapsed = now_ms() - t0;

    if (status == STATUS_OK) {
        int32_t result = (int32_t)get_u32(&resx);
        printf("[client] trace=%016llx: SUCCESS, result=%d (took %lldms)\n",
               (unsigned long long)trace_id, result, (long long)elapsed);
    } else if (status == STATUS_DEADLINE_EXCEEDED) {
        printf("[client] trace=%016llx: DEADLINE EXCEEDED somewhere in the chain "
               "(took %lldms, budget was %dms)\n",
               (unsigned long long)trace_id, (long long)elapsed, budget_ms);
    } else {
        printf("[client] trace=%016llx: ERROR, status=%u\n",
               (unsigned long long)trace_id, status);
    }
    return 0;
}
```
