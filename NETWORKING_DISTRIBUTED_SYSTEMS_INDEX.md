# Networking & Distributed Systems — Master Index

### A map across every folder built in this series, a cross-reference by concept, a real-bugs-found ledger, and a suggested study order — meant to live at the root of `Interview-Preparation/`

---

## 0. The big picture: how everything connects

```
SOCKETS (raw bytes on a wire)
   │   bgnet-book-notes/, Hands-On-Network-Programming-with-C-book-notes/
   ▼
KERNEL INTERNALS (what the OS does underneath those syscalls)
   │   tcp-ip-architecture-design-and-implementation-in-linux-book-notes/
   ▼
RPC (making a remote call look like a local one)
   │   rpc_internal_documentation/  (base demo, ONC RPC, gRPC, idempotency, tracing/deadlines)
   ▼
DISTRIBUTED SYSTEMS (many RPC-connected nodes agreeing on shared state)
       distributed-systems/  (pooling, Raft consensus, 2PC vs. Saga)
```

Each layer's documents assume the one above it. If a distributed-systems concept doesn't make sense, the gap is very often actually an RPC-layer gap; if an RPC document doesn't make sense, the gap is very often a plain-sockets gap.

---

## 1. Folder-by-folder guide

### `bgnet-book-notes/` — Beej's Guide to Network Programming
**11 files**, `00_START_HERE.md` → `10_FINAL_REVISION_CHEATSHEET.md`.
The absolute foundation: what a socket is, TCP vs. UDP, the struct family (`sockaddr`/`sockaddr_in`/`sockaddr_storage`), `select()`/`poll()`, serialization basics. Every program compiled and run for real on localhost.
**Start here if**: sockets themselves are shaky.

### `Hands-On-Network-Programming-with-C-book-notes/` — the advanced sequel
**16 files**, `00_START_HERE.md` → `15_FINAL_REVISION_CHEATSHEET.md`.
Everything Beej's Guide doesn't cover: DNS wire format, HTTP client/server from raw sockets, SMTP, TLS/OpenSSL (client *and* server, real handshake), SSH via `libssh` against a real local `sshd`, and a pitfalls chapter with a genuinely reproduced `SIGPIPE` crash.
**Start here if**: comfortable with plain sockets, need HTTP/TLS/SSH mechanics.

### `tcp-ip-architecture-design-and-implementation-in-linux-book-notes/` — kernel internals
**21 files**, `00_START_HERE.md` → `20_FINAL_REVISION_CHEATSHEET.md`.
What actually happens inside the Linux kernel: `sk_buff`, the socket layer's VFS hookup, the SYN-queue-vs-accept-queue split, TCP's state machine/congestion control/timers, netfilter, softIRQs, device drivers. Every chapter pairs the book's 2002-era (kernel 2.4.20) design with a verified "what's changed since" section (nftables replacing iptables, the route cache's removal, NAPI, etc.).
**Start here if**: asked "what happens when you call `accept()`?" and want the real answer, three layers down.

### `rpc_internal_documentation/` — RPC, end to end
The base files (`README.md`, `rpc.h`, `framing.h`, `rpc_server.c`, `rpc_client.c`, `rpc_client_err.c`) are a from-scratch RPC system — real client/server, real marshalling, real wire bytes decoded by hand. Four subfolders go deeper:

- **`onc-rpc/`** — classic Sun/ONC RPC (the protocol under NFS). `Sun_ONC_RPC_Internals.md` covers XDR, record marking, the portmapper — verified against the actual RFCs, *and* against a real portmapper + service + client this project built (`portmapper.c`, `demo_service.c`, `onc_client.c`). `Trace_Level_Analysis.md` and `Real_Capture_Analysis.md` go one level deeper still: real `strace` syscall traces and a real `tcpdump` packet capture of that exact demo, decoded byte-for-byte.
- **`grpc/`** — modern gRPC. `gRPC_Internals.md` is built around a *real* gRPC client/server (genuine generated Protobuf stubs) with a logging proxy capturing the actual HTTP/2 frames — you can see the literal protobuf bytes for a request and reply, and the trailing `HEADERS` frame carrying `grpc-status` that most explanations only describe.
- **`idempotency/`** — `RPC_semantics_exactly_once.md`: why exactly-once delivery is provably impossible (the Two Generals Problem), with a **live-reproduced bug**: a server without deduplication double-counts a retried call; the fixed version (an idempotency-key cache) gets it right — both runs captured for real.
- **`distributed-tracing-deadline-propagation/`** — `Multi_Hop_RPC_Chain_Analysis.md`: a real 3-hop chain (client→A→B→C) propagating a trace ID and an *absolute* deadline through every hop via the interceptor pattern. Documents a real bug this project found in its own first draft (checking the deadline only *before* work, not after) and the fix.

**Start here if**: need to explain how RPC actually works, or the classic-vs-modern (ONC RPC vs. gRPC) contrast, or distributed tracing.

### `distributed-systems/` — multi-node agreement and coordination
- **`connection-pooling-and-distributed-systems-concepts/`** — a real, correct, thread-safe connection pool (mutex + condition variable, blocking not spinning) with a **measured** result: pooling cut 50 calls from 1014ms to 284ms (~3.6x). Also the conceptual survey: load balancing algorithms, service discovery, rate limiting, sync-RPC-vs-async-queue, CAP theorem, bulkheading/backpressure — plus a dedicated careers/interview-mapping section.
- **`raft-consensus/`** — a real 5-process Raft cluster: leader election, log replication, a genuinely simulated network partition (two "leaders" exist simultaneously — only one can actually commit), a real `kill -9` + disk-recovery test. `Raft_Consensus_Internals.md` has the real log excerpts.
- **`distributed-transactions-2pc-vs-saga/`** — the same failure (a shipping step fails), handled two ways by two real orchestrators: Two-Phase Commit (nothing changes anywhere when the vote fails) vs. Saga (real debit + real reservation happen, then are explicitly compensated). `TwoPC_vs_Saga.md` has both real runs side by side.

**Start here if**: asked about consensus, leader election, or "how do you keep multiple services consistent."

---

## 2. Cross-reference: find a concept fast

| Concept | Where |
|---|---|
| Socket basics, TCP/UDP, `select()`/`poll()` | `bgnet-book-notes/` |
| DNS wire format (raw, by hand) | `Hands-On-Network-Programming-with-C-book-notes/05_DNS_and_Hostname_Resolution.md`; also `rpc_internal_documentation/onc-rpc/` (a different angle — resolving *services*, not hostnames) |
| TLS/HTTPS handshake, certificates | `Hands-On-Network-Programming-with-C-book-notes/09`–`10`; `rpc_internal_documentation/grpc/` (TLS at the transport layer under gRPC) |
| `sk_buff`, kernel packet representation | `tcp-ip-.../05_sk_buff.md` |
| TCP state machine, congestion control | `tcp-ip-.../11`, `12` |
| netfilter / iptables / nftables | `tcp-ip-.../16_Netfilter_Framework.md` |
| Marshalling / serialization (XDR, Protobuf) | `rpc_internal_documentation/README.md`; `onc-rpc/` (XDR in depth); `grpc/` (Protobuf wire format) |
| Portmapper / service discovery (classic) | `rpc_internal_documentation/onc-rpc/` |
| Service discovery (modern: DNS/registry/health checks) | `distributed-systems/connection-pooling.../Connection_Pooling_and_Remaining_Concepts.md` |
| HTTP/2 framing | `rpc_internal_documentation/grpc/gRPC_Internals.md` |
| Exactly-once / idempotency | `rpc_internal_documentation/idempotency/` |
| Distributed tracing, trace IDs | `rpc_internal_documentation/distributed-tracing-deadline-propagation/` |
| Deadline/timeout propagation | same folder — and conceptually, `tcp-ip-.../10_TCP_Timers.md` for the lowest-level version |
| Interceptor / middleware pattern | `distributed-tracing-deadline-propagation/Multi_Hop_RPC_Chain_Analysis.md` |
| Connection pooling | `distributed-systems/connection-pooling-and-distributed-systems-concepts/` |
| Load balancing algorithms | same folder |
| Rate limiting (token/leaky bucket) | same folder |
| CAP theorem | same folder |
| Circuit breakers / bulkheading / backpressure | same folder (concept); `distributed-transactions-2pc-vs-saga/` (backpressure's cousin — blocking participants) |
| Leader election, Raft, log replication | `distributed-systems/raft-consensus/` |
| Network partitions (simulated, real) | `raft-consensus/` |
| Two-Phase Commit, Saga pattern, compensation | `distributed-systems/distributed-transactions-2pc-vs-saga/` |
| `strace` / syscall-level tracing | `rpc_internal_documentation/onc-rpc/Trace_Level_Analysis.md` |
| `tcpdump` / packet-level analysis | `rpc_internal_documentation/onc-rpc/Real_Capture_Analysis.md` |

---

## 3. The real-bugs-and-measurements ledger

This is genuinely strong interview material — specific, verifiable things found or measured, not vague claims. Worth having memorized as ready answers to "tell me about a bug you found" or "tell me about a performance improvement you made":

1. **A missing macro in a published book's own code.** Beej's Guide's `pack2.c` calls `pack754_16()`/`unpack754_16()`, but the book's printed `ieee754.c` only ever defines the 32- and 64-bit versions — confirmed by searching the entire extracted text. Fixed by following the established pattern (16-bit IEEE-754: 5 exponent bits). *(`bgnet-book-notes/`)*
2. **A deprecated API, found by compiling, not reading.** `Hands-On Network Programming with C`'s SCP-based file-transfer chapter compiles with 11 deprecation warnings — `libssh`'s entire SCP API has been deprecated in favor of SFTP since the book was written. *(`Hands-On-Network-Programming-with-C-book-notes/11`)*
3. **`SIGPIPE` didn't reproduce on the first attempt.** A graceful client disconnect let a server's two `send()` calls both succeed; only an abortive (`SO_LINGER`-forced RST) disconnect reliably triggered the crash — a real, measured illustration that "well-known bug" doesn't mean "trivially reproducible on demand." *(`Hands-On-Network-Programming-with-C-book-notes/13`)*
4. **The Linux route cache's removal, verified against real history.** Not just "it changed" — confirmed kernel 3.6 removed it entirely, and *why* (both a real DoS vector and no longer worth its complexity once `fib_trie` made direct lookups fast). *(`tcp-ip-.../14`)*
5. **A protocol mismatch, found by cross-referencing two independent captures.** The service's `getsockname()`-discovered ephemeral port and the client's portmapper-discovered port were confirmed *byte-for-byte identical* in real captured traffic — two independent code paths agreeing, proven, not assumed. *(`onc-rpc/Real_Capture_Analysis.md`)*
6. **A cross-process clock-skew artifact, found and explicitly worked around.** Comparing absolute timestamps across two independently-launched `strace` processes produced a physically-impossible ordering; the fix was restricting every timing claim to within-one-trace-file comparisons, and naming the methodology explicitly rather than hiding it. *(`onc-rpc/Trace_Level_Analysis.md`)*
7. **A real duplicate-execution bug, reproduced live, then fixed live.** An undefended RPC server double-counted a retried call (counter ends at 3, should be 2); an idempotency-key-cached version gets exactly 2 — same client, same simulated lost reply, different server. *(`idempotency/`)*
8. **A deadline-enforcement gap, caught by actually testing, not by code review.** The multi-hop chain's first draft only checked the deadline *before* a hop's work — a 200ms budget against a 300ms operation incorrectly reported success. Fixed by adding a second check *after* the handler runs. *(`distributed-tracing-deadline-propagation/`)*
9. **A ~3.6x measured performance improvement from connection pooling.** 50 calls: 1014ms without pooling vs. 284ms with a 4-connection pool shared by 8 threads — a real number, not an estimate. *(`connection-pooling-.../`)*
10. **A real double-leader window, proven safe by construction.** During a simulated partition, two Raft nodes simultaneously believed they were leader (different terms) — only the higher-term one could ever commit; the stale leader's accepted write was silently and correctly discarded on rejoin. *(`raft-consensus/`)*
11. **The exact same failure, two different real consequences.** A shipping-step failure left *zero* state change under Two-Phase Commit, versus a real debit and real inventory reservation (both genuinely committed, then genuinely reversed) under the Saga pattern — same failure, same services, deliberately contrasted. *(`distributed-transactions-2pc-vs-saga/`)*

---

## 4. Suggested study order

1. `bgnet-book-notes/` (if sockets aren't already solid)
2. `Hands-On-Network-Programming-with-C-book-notes/` chapters 1–8 (skip TLS/SSH on a first pass if time-constrained)
3. `rpc_internal_documentation/README.md` (the base RPC demo) → `onc-rpc/` → `grpc/`
4. `rpc_internal_documentation/idempotency/` → `distributed-tracing-deadline-propagation/`
5. `distributed-systems/connection-pooling-.../` (ties RPC back to practical infra concerns)
6. `distributed-systems/raft-consensus/` → `distributed-transactions-2pc-vs-saga/`
7. `tcp-ip-architecture-....../` as a standing deep-reference — not linear reading, but the place to go when a question needs a kernel-level answer

---

## Quick recap

- Four layers, each building on the last: sockets → kernel internals → RPC → distributed systems.
- Section 2 is the fast lookup table — find a concept, jump straight to its file.
- Section 3 is interview ammunition — eleven specific, verifiable things actually found or measured across this whole series, not generic claims.
