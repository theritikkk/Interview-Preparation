# 4. Kernel Implementation of Connection Setup
### *(Covers the book's Chapter 4 — "Kernel Implementation of Connection Setup")*

You've called `bind()`, `listen()`, `connect()`, and `accept()` throughout the previous two study packs in this series. This chapter is what actually happens inside each one — and it resolves a question every socket programmer eventually asks but rarely gets a satisfying answer to: **what does the `backlog` argument to `listen()` actually limit?** The honest answer needs two separate queues, not one, and understanding why is the heart of this chapter.

## 4.1 `bind()` — claiming a local port

```c
int inet_bind(struct socket *sock, struct sockaddr *uaddr, int addr_len)
```

For a server explicitly requesting a specific port, `inet_bind()`'s job is mostly bookkeeping: verify the port isn't already in use in a conflicting way, then record the chosen local address/port into `sk->rcv_saddr` / `sk->sport`. The "isn't already in use" check consults the **bind hash table** (`tcp_bhash` in this book's kernel era) — a hash table specifically for tracking which local ports are currently bound, indexed by port number, letting this check run in roughly constant time regardless of how many sockets currently exist system-wide, rather than needing a linear scan.

If the application didn't request a specific port (the common client-side case — you've never once called `bind()` explicitly in either previous study pack's client programs), the kernel still needs to hand out a genuinely free **ephemeral port** the moment one is needed — deferred, in fact, until `connect()` actually runs, which is where §4.3 below picks this up.

## 4.2 `listen()` — and the two-queue design

```c
int inet_listen(struct socket *sock, int backlog)
```

This is where the chapter's central idea lives. A server calling `listen(sockfd, backlog)` needs the kernel to track **incoming connection attempts in two genuinely different states**, and conflating them (treating `backlog` as describing just one queue) would be a real correctness bug, not just an implementation detail:

- **The SYN queue** (this book's kernel calls this structure the `syn_table`, hanging off a `tcp_listen_opt`): connections that have received an initial `SYN` and replied with `SYN-ACK`, but **haven't yet completed the three-way handshake** — the client's final `ACK` hasn't arrived (or hasn't been processed) yet. Each entry here is a lightweight `struct open_request`, deliberately *not* a full `struct sock` — allocating a complete socket for every half-open attempt would be an easy way for an attacker to exhaust kernel memory with nothing more than a flood of `SYN` packets they never intend to complete (the classic **SYN flood** attack, and exactly why this queue's entries are kept minimal).
- **The accept queue**: connections whose three-way handshake has **fully completed** — a real, fully-populated `struct sock` already exists, TCP considers the connection `ESTABLISHED`, and it's simply waiting for the application to call `accept()` and claim it.

```
       SYN arrives                    final ACK arrives,
       │                               handshake completes
       ▼                                  │
  ┌───────────────┐                       ▼
  │  SYN queue    │  ────────────►  ┌────────────────┐   accept() ┌───────────────┐
  │ (open_request,│                 │  Accept queue  │ ─────────► │ your process  │
  │  lightweight) │                 │  (full struct  │            │  gets a new   │
  └───────────────┘                 │  sock, ESTAB)  │            │  fd back      │
                                    └────────────────┘            └───────────────┘
```

**`backlog` limits the accept queue specifically — not the SYN queue.** This matters in practice: a slow-to-`accept()`-loop application (imagine a server doing meaningful work between `accept()` calls, or one that's simply fallen behind) can fill up its accept queue with fully-handshaken connections nobody's claimed yet, at which point *further completed handshakes get rejected*, even though the SYN queue itself might have plenty of room. The SYN queue has its own, separately-tuned size limit (historically influenced by `/proc/sys/net/ipv4/tcp_max_syn_backlog`) precisely because it needs to be sized for a completely different concern — surviving a flood of never-completed handshake attempts — rather than "how fast is my application calling `accept()`."

## 4.3 `connect()` — the active open

```c
int tcp_v4_connect(struct sock *sk, struct sockaddr *uaddr, int addr_len)
```

On the client side, `connect()` does considerably more work than `bind()` did:

1. **Route lookup**: consult the routing table (`14_IP_Routing_Table_FIB.md`) to determine which local interface/source address will actually be used to reach the destination — you can't finish filling in the TCP/IP headers without knowing your own source address, and on a multi-homed machine, that's not knowable in advance without doing this lookup.
2. **Ephemeral port allocation**, if `bind()` was never explicitly called (§4.1) — walks a range of local port numbers, checking the same bind hash table, until it finds one both unused and not already used for an identical 4-tuple to some other connection.
3. **Initial Sequence Number (ISN) generation** — historically (and still, conceptually) derived from a combination of a clock-based component and a secret/random component, specifically so an attacker can't easily predict what sequence number a new connection will start with (predictable ISNs historically enabled several classes of connection-hijacking/spoofing attacks).
4. **Build and send the initial `SYN` segment**, and transition the socket's state to `TCP_SYN_SENT`.
5. **Return to the caller** — if the socket is in blocking mode (the default), the calling process now sleeps, waiting to be woken once the handshake either completes (state becomes `TCP_ESTABLISHED`) or definitively fails (a `RST` arrives, or a timeout — `10_TCP_Timers.md` — expires).

## 4.4 `accept()` — claiming a completed connection

```c
struct sock *tcp_accept(struct sock *sk, int flags, int *err)
```

Conceptually simple, given §4.2's two-queue picture already in place: check the accept queue; if it's non-empty, unlink the first `struct sock` off it and hand it back (wrapped in a brand-new `struct socket` + file descriptor, exactly the allocation dance from `03_BSD_Socket_Layer_Implementation.md` §3.2, run again for this specific already-established connection); if it's empty and the call is blocking, sleep on the listening socket's wait queue until either a new connection arrives or a signal interrupts the call.

**This is the exact kernel-level explanation behind a fact the previous study packs' socket-programming chapters stated without fully justifying: `accept()` returns a *new*, separate file descriptor, distinct from the listening socket.** The listening socket's own `struct sock` never represents any individual client — it only ever owns the SYN queue and the accept queue. Each accepted connection genuinely is its own independent `struct sock`, created fresh the moment its handshake completed, sitting in the accept queue purely as cargo waiting to be picked up — `accept()` doesn't "convert" the listening socket into a connected one; it simply hands you a reference to an object that already existed, fully formed, before you ever called it.

## 4.5 Watching this happen, conceptually, via `tcpdump`

The book illustrates this whole flow against real captured traffic. Without needing to reproduce a specific capture here, the shape of what you'd see on the wire for a normal connection maps directly onto the states above:

```
Client                                          Server
  │                                                │
  │  SYN, seq=X                    ─────────────►  │   (server: entry created in SYN queue)
  │                                                │
  │  ◄─────────────  SYN-ACK, seq=Y, ack=X+1       │
  │                                                │
  │  ACK, ack=Y+1                  ─────────────►  │   (server: entry MOVES from SYN queue
  │                                                │    to accept queue — handshake complete)
  │                                                │
  │            [ data exchange, both directions ]  │
```

The specific moment a `struct open_request` in the SYN queue gets promoted into a full `struct sock` on the accept queue is exactly the third packet of this exchange landing — nothing else in this whole flow is that single, precise trigger point.

## 4.6 What's changed since kernel 2.4.20

- **SYN cookies**: a stateless alternative to the SYN-queue approach above, where the server, instead of storing *any* per-connection state for a half-open request, encodes enough information (a cryptographically-derived value) directly into the `SYN-ACK`'s own sequence number to reconstruct what it needs *if and only if* a valid final `ACK` ever comes back — meaning an attacker flooding `SYN`s with no intention of finishing the handshake costs the server essentially zero per-attempt memory, a direct, much stronger answer to the SYN-flood concern §4.2 raised. SYN cookies existed in limited form even in some 2.4-era kernels as an optional defense, but are far more refined, and far more commonly enabled by default, in modern kernels.
- **`accept4()`**: a newer syscall variant letting you set flags like `SOCK_NONBLOCK`/`SOCK_CLOEXEC` on the newly accepted socket atomically, at `accept()` time, instead of needing a separate, racy follow-up `fcntl()` call — a small but genuinely important correctness fix for concurrent server code.
- **`SO_REUSEPORT`** (added in kernel 3.9): lets *multiple independent listening sockets* bind the exact same address/port simultaneously, with the kernel load-balancing incoming connections across them — enabling a very different, now-common scaling pattern (many worker processes, each with their own listening socket and therefore their own accept queue, rather than one listener whose accept queue is shared/contended across worker threads) that has no counterpart in this book's single-listener-socket model.
- **Network namespaces** apply here too, exactly as noted in the previous chapter — the bind hash table, SYN queue, and accept queue are all conceptually per-namespace in a modern kernel, not global.

---

## Quick recap (for revision)

- **Two queues, two purposes**: the SYN queue holds lightweight, incomplete handshake attempts (deliberately cheap, to resist SYN floods); the accept queue holds fully-established connections waiting for `accept()`. `listen()`'s `backlog` argument sizes the accept queue specifically — not the SYN queue, which is sized separately.
- `connect()` does real work beyond "send a SYN": a routing-table lookup for source address selection, ephemeral port allocation if needed, and ISN generation with an unpredictability requirement baked in for security reasons.
- `accept()` returning a new, separate fd isn't a convenience abstraction — it reflects a real, separate `struct sock` that was already fully created the instant the three-way handshake's final ACK was processed, sitting in the accept queue as-is until claimed.
- The precise moment a connection moves from the SYN queue to the accept queue is the arrival of the handshake's third packet — a single, identifiable trigger, not a gradual transition.

Next: `05_sk_buff.md` — the data structure every one of these packets is actually represented by, once you're inside the kernel.