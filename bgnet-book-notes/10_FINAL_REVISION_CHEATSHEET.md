# 10. Final Revision Cheat Sheet
### *(The whole guide, compressed)*

Use this as your last pass before you consider yourself "done" — if every section below makes sense at a glance, you've genuinely absorbed the guide. If any section makes you pause, that's a pointer to which earlier file to reread.

## The two call sequences (memorize these)

```
 SERVER                                    CLIENT
 ──────                                    ──────
 getaddrinfo(NULL, port, AI_PASSIVE)       getaddrinfo(hostname, port)
 socket()                                  socket()
 setsockopt(SO_REUSEADDR)
 bind()
 listen()
 accept()  ──►  new_fd                     connect()
 send()/recv() on new_fd                   send()/recv() on sockfd
 close(new_fd)                             close(sockfd)
```

UDP drops `listen()`/`accept()`/`connect()` entirely — server: `getaddrinfo → socket → bind → recvfrom`; client: `getaddrinfo → socket → sendto`.

## The struct family tree

```
              struct sockaddr            (generic, 16 bytes: sa_family + opaque data)
                     ▲   ▲
        "cast to"    │   │    "cast to"
                     │   │
  struct sockaddr_in │   │ struct sockaddr_in6
   (IPv4-specific)   │   │  (IPv6-specific)
                     │   │
      struct sockaddr_storage   (big enough for either — used when the type isn't known yet,
                                  e.g. what accept()/recvfrom() fill in)
```

`struct addrinfo` is the separate, higher-level struct you fill in as `hints` and get back (as a linked list, via `ai_next`) from `getaddrinfo()` — it *contains* a `struct sockaddr *` (`ai_addr`) but isn't part of the cast-between-each-other family above.

## Data's journey out of your program and onto the wire

```
 ┌──────────────────────────────────────────────────────────────────┐
 │ Ethernet header │ IP header │ TCP/UDP header │      YOUR DATA    │
 └──────────────────────────────────────────────────────────────────┘
    hardware/driver    kernel        kernel        send()/sendto()
```
Stripped back off in the same layers, in reverse, on the receiving machine. You only ever touch the top.

## Function return-value conventions — the single most important table in these notes

| Pattern | Functions |
|---|---|
| `0` success / nonzero error code, decode with a `*_strerror()` | `getaddrinfo()`, `getnameinfo()` |
| `-1` on error, check global `errno` (`perror()`/`strerror()`) | `socket()`, `bind()`, `connect()`, `listen()`, `accept()`, `send()`, `recv()`, `sendto()`, `recvfrom()`, `close()`, `shutdown()`, `poll()`, `select()`, `setsockopt()`, `getsockopt()`, `getpeername()`, `gethostname()` |
| Bytes actually transferred (may be less than requested!) | `send()`, `sendto()` |
| Bytes read, or specifically `0` for "peer closed" | `recv()`, `recvfrom()` |
| A **brand new** descriptor, distinct from the one you called it on | `accept()` |

## Byte order — the rule you'll forget at least once

**Convert right before sending, convert right after receiving.** `htons()`/`htonl()` outbound, `ntohs()`/`ntohl()` inbound. Every multi-byte number in a header (most importantly port numbers) needs this. There's no built-in equivalent for floats — see the packing chapter for why, and what to do instead.

## TCP vs UDP, in one line each

- **TCP (`SOCK_STREAM`)**: reliable, ordered, connection-based. `connect()`/`accept()` required. Use when correctness matters more than raw speed.
- **UDP (`SOCK_DGRAM`)**: unreliable, unordered, connectionless. Every send needs an explicit destination address (`sendto()`) unless you `connect()` the UDP socket first. Use when speed matters more than occasional loss — or build your own reliability layer on top if you need both.

## `poll()`/`select()` — one process, many clients, no `fork()`

Both: hand the kernel a list of sockets, sleep until *any* has something ready, avoid busy-waiting. Put the **listening socket in the same set as every client** — when *it* shows "ready to read," that means a new connection is waiting to be `accept()`-ed, not that there's data.

| | `poll()` | `select()` |
|---|---|---|
| Structure | Array of `struct pollfd` | Bitmask `fd_set` |
| Extra bookkeeping | None | Must track highest fd (`fdmax`) yourself |
| Prefer for | New code | Reading/maintaining old code |

Neither scales to huge connection counts — that's `epoll`/`kqueue`/`libevent` territory.

## The five gotchas most likely to bite you specifically (each one verified for real while building these notes)

1. **`memset()` your `hints`/`sockaddr_in` structs to zero before filling them in.** Uninitialized padding/flags cause mysterious `bind()`/`connect()` failures.
2. **`recv()` doesn't null-terminate.** If you're treating the bytes as a C string, add `buf[n] = '\0';` yourself.
3. **UDP family mismatches fail *silently*** (packet just vanishes) — unlike TCP, where `connect()` fails loudly and your retry loop just tries the next address family. This is exactly why `listener.c`/`talker.c` hardcode one family instead of `AF_UNSPEC`.
4. **`"Address already in use"` on server restart** → add `SO_REUSEADDR` before `bind()`. **`"Connection refused"`** → nothing is listening on that port on that host (an immediate, definite failure — not a timeout).
5. **`send()`'s return value can be less than you asked for.** Loop (`sendall()`-style) for anything beyond a small, guaranteed-to-fit message. Real networks also *deliver* in pieces (MTU limits) — a `recv()`-once-and-assume-it's-complete pattern that "worked" on `localhost` in testing is not safe on a real network.

## Self-test

Try answering these from memory before checking the earlier chapters:

1. What does `recv()` returning `0` mean, and how is that different from it blocking?
2. Why does `accept()` return a *different* file descriptor than the one you called `listen()` on?
3. Name one thing that's true of `SOCK_STREAM` but false of `SOCK_DGRAM`, and one thing true of `SOCK_DGRAM` but false of `SOCK_STREAM`.
4. What's wrong with `send(s, &some_double, sizeof some_double, 0)` sent directly to another machine?
5. In `select()`, what should the first argument (`numfds`) actually be set to?
6. Why do `listener.c`/`talker.c` hardcode `AF_INET6` instead of using `AF_UNSPEC` like every TCP example does?
7. What's the difference in what `poll()` reports when the *listening* socket is ready to read, versus when a *client* socket is ready to read — and how does `pollserver.c` tell them apart?

*(Answers: 1 — peer performed an orderly close, a permanent state, vs. blocking which is temporary "nothing yet." 2 — so the original listening socket can keep accepting further new connections while this one handles an established client. 3 — Stream: reliable & ordered, connection-based / Datagram: connectionless, can lose or reorder packets. 4 — non-portable: byte order and in-memory float representation aren't guaranteed to match between machines. 5 — the highest-numbered fd across all your sets, plus one. 6 — because a UDP family mismatch fails silently with no error on either side, unlike TCP's loud `connect()` failure — pinning both sides to the same family avoids the silent-failure case entirely. 7 — both report "ready to read," but on the listener it means a new connection is waiting to be `accept()`-ed rather than data waiting to be `recv()`-ed; `pollserver.c`'s `process_connections()` checks `fd == listener` to branch between the two.)*

---

You've now been through every code example in this 126-page guide — extracted from your own PDF, compiled with zero warnings, and actually run, including real multi-client chat sessions, a genuine UDP broadcast confirmed end-to-end, and two real bugs (a missing macro in the book's own printed serialization code, and an IPv6-vs-sandbox mismatch) found and fixed rather than glossed over. That's a solid foundation — the natural next step from here is picking one of these programs and deliberately breaking/extending it yourself (add a new `pack()` format character, make `pollserver.c` support private messages between two specific clients, add a simple length-prefixed framing layer to the chat relay) until modifying socket code stops feeling unfamiliar.