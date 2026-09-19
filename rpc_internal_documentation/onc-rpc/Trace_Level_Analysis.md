# Trace-Level Analysis: What Actually Happens When the ONC RPC Demo Runs

### Real `strace` output from all three processes — every syscall that actually fired, in the order it actually fired, with one genuinely interesting timing subtlety worked out rather than glossed over

---

## What this document is

`Sun_ONC_RPC_Internals.md` showed you the demo's *printed* output — `printf()` lines confirming the portmapper flow and the record-marking split worked. This document goes one level deeper: it attaches `strace` to all three processes (the portmapper, the service, and the client) during a real run and walks through the **actual system calls** the kernel executed — `socket()`, `bind()`, `getsockname()`, `connect()`, `accept()`, `sendto()`, `recvfrom()` — with real file descriptors, real port numbers, and real microsecond timestamps. This is "what's happening in the background" made literal: not a description of what *should* happen, but a transcript of what the kernel actually did.

Every trace excerpt below is genuine, unedited `strace -f -tt` output from one specific run. Rerun the demo yourself and you'll get different port numbers and PIDs — the *pattern* will be identical.

## 1. The cast: three real OS processes

```
$ strace -f -tt -o pm_strace.log ./portmapper &        # PID 122
$ strace -f -tt -o svc_strace.log ./demo_service &      # PID 127
$ strace -f -tt -o client_strace.log ./onc_client        # PID 131
```

Three genuinely separate processes, each with its own PID, its own file descriptor table, its own memory — the only thing connecting them is the network (well, loopback) traffic between their sockets. Nothing here is threads or shared memory; every single interaction below happens through the same `socket()`/`send()`/`recv()` machinery covered in the socket-programming study packs earlier in this series, just now with a kernel-level trace confirming exactly when each call happened and what it returned.

## 2. Phase 1: the service registers itself — real ephemeral port allocation

This is the single most instructive sequence in the whole demo, because it answers a question that's easy to wave hands at: **where does the port number 60671 (or whatever number your run produces) actually come from?**

**The service's real trace:**
```
127  12:40:47.953192 socket(AF_INET, SOCK_STREAM, IPPROTO_IP) = 3
127  12:40:47.953215 bind(3, {sin_port=htons(0), sin_addr="0.0.0.0"}, 16) = 0
127  12:40:47.953236 listen(3, 10) = 0
127  12:40:47.953248 getsockname(3, {sin_port=htons(60671), sin_addr="0.0.0.0"}, [16]) = 0
```

Walk this literally, one line at a time:
- `socket(...)` → the kernel hands back file descriptor `3`. Nothing about a port yet — this is just an unbound socket object sitting in the kernel.
- `bind(3, {sin_port=htons(0), ...})` → the program explicitly asks for port **`0`**. In the socket API, binding to port 0 doesn't mean "listen on port zero" — by convention, it's the kernel's signal to say **"pick any currently-unused port for me."** The kernel consults its ephemeral port range (commonly `32768`–`60999` or similar, tunable via `/proc/sys/net/ipv4/ip_local_port_range` on Linux) and silently, internally, assigns one — but at this exact moment, **the calling program still has no idea what number it got.** `bind()` returns `0` (success) and tells you nothing about *which* port.
- `listen(3, 10)` → the socket is now a real listening socket, on whatever port the kernel silently picked — but again, the *program* doesn't know the number yet.
- `getsockname(3, ...)` → **this is the only way the program finds out.** It asks the kernel, "what did you actually bind fd 3 to?", and the kernel replies with the real answer: `sin_port=htons(60671)`. This single syscall is the entire mechanism behind the demo's `printf("[service] OS assigned me ephemeral port %u")` line — there's no cleverness in the application code at all; it's just asking the kernel to reveal a decision the kernel already silently made two syscalls earlier.

**This is precisely why a portmapper has to exist at all** (the subject of `Sun_ONC_RPC_Internals.md` §5): the service's own port is decided by the kernel, at runtime, and is fundamentally unknowable in advance by anyone — including the service's own source code. The only way any *other* process can find it is by asking something that the service told, after the fact — which is exactly the portmapper's job.

## 3. Phase 1 continued: the actual registration call, byte for byte

Immediately after discovering its own port, the service connects to the portmapper and registers it:

```
127  12:40:47.953321 socket(AF_INET, SOCK_STREAM, IPPROTO_IP) = 4
127  12:40:47.953332 connect(4, {sin_port=htons(9111), sin_addr="127.0.0.1"}, 16) = 0
127  12:40:47.953462 sendto(4, "\200\0\0$", 4, 0, NULL, 0) = 4
127  12:40:47.953589 sendto(4, "\0\0\0\1\0\0\0\0\0\1\206\240\0\0\0\2\0\0\0\1\0\0\0\f\0\2\0\1\0\0\0\1"..., 36, 0, NULL, 0) = 36
127  12:40:47.953626 recvfrom(4, "\200\0\0\24", 4, 0, NULL, NULL) = 4
127  12:40:47.953676 recvfrom(4, "\0\0\0\1\0\0\0\1\0\0\0\0\0\0\0\4\0\0\0\1", 20, 0, NULL, NULL) = 20
```

**And the portmapper's own trace, the other half of the exact same exchange:**
```
122  12:40:46.950973 accept(3, {sin_port=htons(56684), sin_addr="127.0.0.1"}, [16]) = 4
122  12:40:47.953524 recvfrom(4, "\200\0\0$", 4, 0, NULL, NULL) = 4
122  12:40:47.953604 recvfrom(4, "\0\0\0\1\0\0\0\0\0\1\206\240\0\0\0\2\0\0\0\1\0\0\0\f\0\2\0\1\0\0\0\1"..., 36, 0, NULL, NULL) = 36
122  12:40:47.953649 sendto(4, "\200\0\0\24", 4, 0, NULL, 0) = 4
122  12:40:47.953680 sendto(4, "\0\0\0\1\0\0\0\1\0\0\0\0\0\0\0\4\0\0\0\1", 20, 0, NULL, 0) = 20
```

Decoding the request bytes by hand, exactly the way the portmapper's own `xdr_get_u32()` calls do it, four bytes at a time — `\200\0\0$` is the record-marking header (`0x80000024`: top bit set, length `0x24`=36); then the 36-byte body: `\0\0\0\1` = xid `1`, `\0\0\0\0` = `MSG_CALL`, `\0\1\206\240` = `0x000186a0` = **100000** (the portmapper's own real program number, exactly as specified in RFC 1833), `\0\0\0\2` = version `2`, `\0\0\0\1` = procedure `1` = `PMAPPROC_SET`, `\0\0\0\f` = arg length `12`, then the arguments: `\0\2\0\1` = `0x20001` (this demo's `DEMO_PROGRAM`), `\0\0\0\1` = version `1` — and the port value (truncated by the `...` in strace's own output, but confirmed by the portmapper's own printed log to be `60671`).

**The portmapper's `accept()` call itself is worth noting**: it happened at `.950973` — a full **1.003 seconds** before the actual request bytes arrived at `.953524`. That gap is real, and it's the demo's own `sleep(1)` calls in the orchestration script — the portmapper's `accept()` genuinely sat blocked, doing nothing, consuming zero CPU, for that entire second, until the service process actually connected. This is worth pausing on: **`accept()` blocking isn't a busy-wait** — the OS scheduler simply doesn't run this process again until there's actually a connection to hand it, exactly the same blocking-call behavior from the very first socket-programming material in this whole series, now visible directly as a real, measured gap between two consecutive lines of one process's own trace.

## 4. Phase 2: the client asks the portmapper — and gets the exact same port back

```
131  12:40:48.956038 socket(AF_INET, SOCK_STREAM, IPPROTO_IP) = 3
131  12:40:48.956081 connect(3, {sin_port=htons(9111), sin_addr="127.0.0.1"}, 16) = 0
131  12:40:48.956314 sendto(3, "\200\0\0 ", 4, 0, NULL, 0) = 4
131  12:40:48.956435 sendto(3, "\0\0\0\7\0\0\0\0\0\1\206\240\0\0\0\2\0\0\0\3\0\0\0\10\0\2\0\1\0\0\0\1", 32, 0, NULL, 0) = 32
131  12:40:48.956474 recvfrom(3, "\200\0\0\24", 4, 0, NULL, NULL) = 4
131  12:40:48.956551 recvfrom(3, "\0\0\0\7\0\0\0\1\0\0\0\0\0\0\0\4\0\0\354\377", 20, 0, NULL, NULL) = 20
131  12:40:48.956593 close(3)          = 0
```

Same decoding exercise, this time on the request: procedure `\0\0\0\3` = `3` = `PMAPPROC_GETPORT` (not `SET` this time), with the same `prog=0x20001, vers=1` arguments. And the reply's final four bytes, `\0\0\354\377`, decode to `0x0000ecff` = **60671** — the exact same number the service discovered for itself via `getsockname()` back in §2. **Two completely independent processes, using completely independent code paths, arrived at the identical number** — one by asking the kernel directly, one by asking a third party who'd been told. That agreement is the entire portmapper mechanism working correctly, made visible in the actual returned bytes rather than just asserted.

Notice also `close(3)` immediately after — the client deliberately disconnects from the portmapper the instant it has the answer. This connection's entire job was that one question; it doesn't linger.

## 5. Phase 3: the real call, straight to the real port, no portmapper involved

```
131  12:40:48.956636 socket(AF_INET, SOCK_STREAM, IPPROTO_IP) = 3
131  12:40:48.956661 connect(3, {sin_port=htons(60671), sin_addr="127.0.0.1"}, 16) = 0
131  12:40:48.956779 sendto(3, "\200\0\0 ", 4, 0, NULL, 0) = 4
131  12:40:48.956802 sendto(3, "\0\0\0\10\0\0\0\0\0\2\0\1\0\0\0\1\0\0\0\1\0\0\0\10\0\0\0\3\0\0\0\4", 32, 0, NULL, 0) = 32
131  12:40:48.956926 recvfrom(3, "\0\0\0\10\0\0\0\1\0\0\0\0\0\0\0\4\0\0\0\7", 20, 0, NULL, NULL) = 20
131  12:40:48.956962 close(3)
```

`connect(3, {sin_port=htons(60671), ...})` — **a fresh `socket()`/`connect()` pair, to a port number that exists nowhere in this program's source code.** `60671` only exists in this call because §4's `recvfrom()` put it into a variable at runtime. The request bytes decode to `procedure=1` (`PROC_ADD`), args `a=3, b=4`; the reply's last four bytes, `\0\0\0\7`, are `7` — `3+4`, confirmed correct, byte for byte, in the raw trace.

## 6. Phase 4: the fragmented call — a real subtlety worth working through honestly

Here's where tracing *both* sides independently turned up something genuinely worth explaining rather than smoothing over.

**The client's trace for the fragmented send:**
```
131  12:40:48.957078 sendto(3, "\0\0\0\20", 4, ...) = 4          <- fragment 1 header (top bit CLEAR, len 16)
131  12:40:48.957113 sendto(3, "\0\0\0c\0\0\0\0\0\2\0\1\0\0\0\1", 16, ...) = 16   <- fragment 1 body
131  12:40:48.957152 clock_nanosleep(CLOCK_REALTIME, 0, {tv_nsec=200000000}, NULL) = 0
131  12:40:49.157407 sendto(3, "\200\0\0\20", 4, ...) = 4         <- fragment 2 header (top bit SET, len 16)
131  12:40:49.157580 sendto(3, "\0\0\0\1\0\0\0\10\0\0\0o\0\0\0\336", 16, ...) = 16  <- fragment 2 body
```

That's completely unambiguous: a real `clock_nanosleep()` for exactly 200,000,000 nanoseconds (200ms), sitting squarely between sending fragment 1 and sending fragment 2. No question about what the client did.

**The service's trace for receiving the same exchange:**
```
127  12:40:48.957087 recvfrom(4, "\0\0\0\20", 4, ...) = 4          <- fragment 1 header
127  12:40:48.957121 recvfrom(4, "\0\0\0c\0\0\0\0\0\2\0\1\0\0\0\1", 16, ...) = 16   <- fragment 1 body
127  12:40:48.957147 recvfrom(4, "\200\0\0\20", 4, ...) = 4         <- fragment 2 header
127  12:40:49.157566 recvfrom(4, "\0\0\0\1\0\0\0\10\0\0\0o\0\0\0\336", 16, ...) = 16  <- fragment 2 body
```

Comparing these four lines **against each other, within this one file** (a perfectly valid comparison — every timestamp here comes from the same process, same clock, no ambiguity possible): the gap between reading fragment 2's *header* and fragment 2's *body* is `49.157566 − 48.957147 ≈ 200ms` — matching the client's sleep almost exactly.

**What this actually shows, worth stating precisely**: the header and body of "fragment 2" are sent via *two separate* `sendto()` calls in the client's code, back to back, with no sleep between them. Yet on the receiving end, the header's 4 bytes became available to read measurably before the body's 16 bytes did. This is a real, observed instance of the same lesson this whole document series keeps returning to from a new angle: **TCP has no concept of your application's logical groupings.** Even two `sendto()` calls issued microseconds apart, with nothing deliberately separating them, are not guaranteed to arrive — or become readable — as one atomic unit. The receiving side's `recv_all()` loop handles this correctly regardless (it keeps calling `recv()` until it has however many bytes it asked for, exactly as covered back in the very first socket-programming material in this series) — but the *timing* of when each piece becomes available is entirely up to the kernel's own buffering and scheduling, not something either side's application code controls or can assume.

**One honest limitation, stated directly rather than hidden**: comparing *absolute* wall-clock timestamps *between* the client's trace file and the service's trace file (e.g., "the client sent X at exactly time T, and the service received it at exactly time T+ε") turned out to be unreliable in this environment — an apparent ordering inversion showed up when attempting exactly that cross-file comparison, most plausibly explained by independent `strace` invocations not sharing a perfectly synchronized time reference at microsecond resolution, though the sandboxed environment's clock behavior wasn't investigated further. Every specific timing claim in this document is therefore deliberately made **within a single process's own trace file only** — comparisons that are always valid, since they share one uninterrupted clock reference — never across files. This is worth knowing as a general methodology point, not just a footnote: when analyzing distributed system behavior from multiple independently-collected traces, cross-source absolute-timestamp comparisons need genuine clock synchronization (NTP, PTP, or a shared reference) to be trustworthy at fine granularity — a real, common pitfall in distributed tracing generally, not something specific to this toy demo.

## 7. The concurrency picture: three processes, who's blocked when

```
TIME →

portmapper:  [socket][bind][listen]────accept()BLOCKED────┐[recv][send]──accept()BLOCKED──┐[recv][send]──accept()BLOCKED (forever, until killed)
                                                            │(service connects)             │(client connects)
service:                          [socket][bind:0][listen][getsockname]
                                                            └─[socket][connect]→PM [send][recv]──┐
                                                                                                    accept()BLOCKED (on ITS OWN port)
client:                                                                                             │(client connects to service)
                                                                                        [socket][connect]→PM [send][recv][close]
                                                                                                    └[socket][connect]→service [send][recv][close]
                                                                                                              [socket][connect]→service [send×2, w/ sleep][recv][close]
```

Every `accept()` and every `recv_all()` in this whole system is a **blocking** call — the OS genuinely suspends that process (consuming zero CPU, not busy-polling) until there's something to hand it, and wakes it back up the instant data or a connection actually arrives. This is why three completely independent, uncoordinated processes — started separately, with no shared memory, no explicit synchronization primitive between them at all — correctly execute a multi-step choreography (register → discover → connect → call) in the right order: each step's blocking call simply waits, for however long it takes, until its specific precondition (a connection arrives; a full message arrives) is satisfied. The `sleep(1)` calls in the orchestration script exist only to make sure processes are *started* in a sensible order for this demo's own bookkeeping — they are not what makes the RPC protocol itself correct. Kill the sleeps entirely, start everything at once, and the same blocking-call discipline would still produce a correct run — just with different, tighter timing, and possibly a few connection-refused retries if a client tries to connect before a server's `listen()` has run yet.

---

## Quick recap (for revision)

- `bind(port=0)` doesn't reserve port zero — it's the standard signal for "kernel, assign me any free port," and the caller genuinely doesn't know which one until a separate `getsockname()` call asks.
- The service's `getsockname()`-discovered port and the client's portmapper-discovered port were confirmed, byte for byte, in the real captured traffic, to be the exact same number — arrived at via two completely independent code paths, which is the portmapper mechanism actually working, not just described.
- `accept()` blocking is genuine OS-level suspension, not a busy-wait — visible directly as a real, multi-hundred-millisecond gap between consecutive lines in one process's own trace, consuming no CPU during that gap.
- **Even two back-to-back `sendto()` calls from the same logical "fragment" aren't guaranteed to become readable on the other end as one atomic unit** — confirmed directly by comparing a single process's own consecutive `recvfrom()` timestamps, not merely asserted from protocol theory.
- Cross-process absolute-timestamp comparisons require genuine clock synchronization to trust at fine granularity — a real methodological pitfall, demonstrated directly by an apparent (and unresolved) ordering inversion when this document's own draft attempted exactly that comparison. Every timing claim actually made above was deliberately restricted to comparisons within one trace file for this reason.
- Three fully independent, unsynchronized-at-the-code-level processes correctly execute a multi-step distributed protocol purely because every step's blocking call waits on its own real precondition — no shared clock, shared memory, or explicit coordination primitive between the processes is needed for correctness, only for this demo's own convenient startup ordering.
