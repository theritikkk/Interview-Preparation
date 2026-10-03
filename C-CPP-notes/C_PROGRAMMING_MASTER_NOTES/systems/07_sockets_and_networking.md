# Sockets & Networking (Bonus)

> **Phase 9 · UNIX/POSIX System Interface — bonus file.** Depends on:
> `systems/03_file_descriptors_and_unix_io.md` (a socket is a file descriptor),
> `05_processes_fork_exec_wait.md` (`fork` for concurrent servers),
> `06_pipes_and_signals.md` (`SIGPIPE`, `SIGCHLD`, byte-stream semantics),
> `fundamentals/02_types.md` §5 (endianness — this file *fulfills* that file's forward
> reference to `ntohl`/`htonl`).
> Sources: **Book A** ch.29 (primary — the only one of the three books that touches
> networking at all; its client/server walkthrough is the backbone below), plus `[External]`
> for TCP semantics and modern address resolution. **This is a bonus topic**: it isn't on
> the brief's core list, but it's the natural next step from pipes, and it's where Phase 1's
> endianness discussion finally pays off.

---

## 1. The big idea: a socket is a file descriptor that can span machines

`[Book-derived, A]` Book A's own one-line summary of the `socket()` call is the key idea:
it "tells our OS that we want a file descriptor for a socket which we can use for a
network stream connection." `[Synthesis]` Everything from Phase 8 carries over: the socket
is a small integer, `close` releases it, and — for a connected stream socket — even plain
`read`/`write` work on it (the socket-specific `send`/`recv` add a flags argument). A
**pipe** (`06_pipes_and_signals.md`) connects two related processes on one machine; a
**socket** generalizes the same "byte stream through a descriptor" idea to unrelated
processes, possibly on different machines.

```mermaid
sequenceDiagram
    participant S as Server
    participant C as Client
    S->>S: socket()  — get a descriptor
    S->>S: bind()    — claim a port
    S->>S: listen()  — start queuing connections
    C->>C: socket()
    C->>S: connect() — request a connection
    S->>S: accept()  — take it off the queue; returns a NEW "session" descriptor
    C->>S: send() / recv() — bytes flow, both directions
    S->>C: send() / recv()
    C->>C: close()
    S->>S: close() (the session descriptor)
```

---

## 2. The client, step by step

`[Book-derived, A, condensed and adapted — Book A's own listing omits error checking "for
clarity"; the version below adds it, because the omissions hide real bugs (§5)]`

```c
int sock = socket(AF_INET, SOCK_STREAM, 0);           /* TCP stream socket */
if (sock < 0) { perror("socket"); exit(EXIT_FAILURE); }

struct sockaddr_in dest;
memset(&dest, 0, sizeof dest);                          /* zero the whole struct first */
dest.sin_family      = AF_INET;                          /* must match socket()'s first arg */
dest.sin_addr.s_addr = inet_addr("127.0.0.1");            /* text -> big-endian integer */
dest.sin_port        = htons(PORTNUM);                     /* host order -> network order */

if (connect(sock, (struct sockaddr *)&dest, sizeof dest) < 0) {
    perror("connect"); exit(EXIT_FAILURE);
}
char buf[MAXRCVLEN + 1];
ssize_t len = recv(sock, buf, MAXRCVLEN, 0);
if (len < 0) { perror("recv"); exit(EXIT_FAILURE); }
buf[len] = '\0';                                            /* recv does NOT null-terminate */
printf("Received %s (%zd bytes).\n", buf, len);
close(sock);
```

**Line by line, the parts that teach something:**
- `memset(&dest, 0, sizeof dest)` — `sockaddr_in` has padding and fields you don't set; zeroing
  first avoids sending/reading garbage (`memory/07_alignment_padding_aliasing.md` §2 —
  padding bytes again).
- `(struct sockaddr *)&dest` — the socket API takes a *generic* `struct sockaddr *` so one
  set of functions serves IPv4, IPv6, and Unix-domain addresses. The cast is the
  pre-`void *`-era C idiom for "pretend this specific struct is the generic one"
  (`memory/02_pointers.md` §7).
- `recv` **does not null-terminate** — `[Book-derived, A]` Book A says this explicitly, and
  it's the same lesson as `memory/04_arrays_and_strings.md` §5: data from the outside world
  is bytes, not a C string, until *you* terminate it.

---

## 3. Network byte order — Phase 1's endianness question, answered

`[Book-derived, A + Synthesis, closing the loop with `fundamentals/02_types.md` §5 and its
systems-level interview question]` Book A notes that `sin_addr.s_addr` "is just an integer
stored in Big Endian format" and that `htons()` "converts the port number into a Big Endian
short integer" — adding that `dest.sin_port = 21` would work only on machines that already
happen to be big-endian, so "for portability reasons `htons()` should always be used."

This is exactly the situation Phase 1 warned about: a multi-byte integer has no single
in-memory byte order (little-endian x86 vs. big-endian others), so two machines exchanging
raw `int` bytes would disagree about the value. **The Internet protocols resolved this by
fixing one order on the wire — big-endian, called *network byte order* — and the C library
provides four conversion functions:**

| Function | Converts |
|---|---|
| `htons(x)` | **h**ost → **n**etwork, **s**hort (16-bit, e.g. a port) |
| `htonl(x)` | host → network, **l**ong (32-bit, e.g. an IPv4 address) |
| `ntohs(x)` | network → host, short |
| `ntohl(x)` | network → host, long |

On a big-endian host these compile to no-ops; on a little-endian host they swap bytes. The
**program is portable either way** — you write the conversion once, and the platform decides
whether it does anything. `[Synthesis]` This is also why `memcpy`-ing a received 4-byte
value straight into an `int` is a bug (the exact scenario in `fundamentals/02_types.md`
§10's systems-level question): the received bytes are in network order, the `int` expects
host order, and the conversion function is the bridge between them.

---

## 4. The server, step by step

`[Book-derived, A, condensed]` The server differs from the client in four calls:

- **`bind(sock, &serv, sizeof serv)`** — associates the socket with a local address/port
  (Book A: this "allows the machine to know the data received on the port specified …
  should be handled by our specified socket"). A server fills in **its own** address
  (`INADDR_ANY`, via `htonl`, means "any network interface"), where the client filled in the
  *destination's*.
- **`listen(sock, backlog)`** — marks the socket as passive; `backlog` is the queue length
  for connections that have arrived but haven't been `accept`ed yet.
- **`accept(sock, &client_addr, &len)`** — blocks until a connection is waiting, removes it
  from the queue, and returns a **new descriptor**. `[Book-derived, A]` Book A calls this a
  "session" socket: it's used *solely* for that one client, while the original listening
  socket keeps accepting others.
- **`send`/`recv` on the session descriptor** — same as the client.

### 💡 Why `accept` returns a *new* descriptor
`[Synthesis]` A listening socket represents "the door"; a session socket represents "a
specific conversation." If `accept` handed back the listening socket itself, the server
couldn't keep listening while talking. Two descriptors — one for the door, one per
conversation — is what lets a server hold many clients at once.

### Handling more than one client
`[Book-derived, A]` Book A states the limit plainly: the example server handles only one
connection at a time; to serve several simultaneously "you'll need to `fork()` off separate
processes, or use threads." `[Synthesis]` The `fork`-per-connection version is a direct
application of Phase 9 so far: after `accept`, `fork`; the **child** closes the listening
descriptor and serves the session; the **parent** closes the *session* descriptor and loops
back to `accept`; a `SIGCHLD` handler reaping with `waitpid(-1, …, WNOHANG)`
(`06_pipes_and_signals.md` §4) prevents zombies. (Modern high-performance servers use
`select`/`poll`/`epoll` event loops or thread pools instead — `[External]`, beyond this
vault's core scope.)

---

## 5. ⚠️ Real bugs hiding in the "bare bones" example

`[Synthesis — a close read of Book A's own listings, whose stated simplification is that
"error checking has been left out for clarity"]` Each of these is instructive precisely
because it looks fine:

1. **`buffer[len] = '\0'` with an unchecked `recv`.** `recv` returns `-1` on error, so
   `buffer[-1]` is an **out-of-bounds write** — undefined behavior
   (`memory/02_pointers.md` §4), reachable by any network failure. Always check `len < 0`
   first, as in §2.
2. **`while (consocket)` as the server's loop condition.** `accept` returns `-1` on failure —
   which is *nonzero, hence true* in C (`fundamentals/04_control_flow.md` §8). A failing
   `accept` would keep the loop spinning instead of ending it. The correct test is
   `>= 0`.
3. **A leaked descriptor per connection.** In Book A's server loop, each new `accept`
   overwrites `consocket` without closing the previous session descriptor — every served
   client leaks one file descriptor until the process exhausts its limit
   (`systems/03_file_descriptors_and_unix_io.md` §2: descriptors are a finite resource).
   Close each session descriptor when done with it.
4. **`sizeof(struct sockaddr)` in `connect`/`bind`.** Book A passes the size of the
   *generic* struct; the correct convention is the size of the *actual* address struct you
   filled in (`sizeof dest`). On common platforms the two happen to be the same size, so it
   works — by coincidence, not by contract `[Needs verification: exact sizes per platform]`.
5. **`recv` and `send` may transfer fewer bytes than requested.** `[Book-derived, A]` Book A
   warns for `send`: it "may not be able to send all of the bytes, so it is important to
   check that its return value is equal to the number of bytes you tried to send." The same
   is true of `recv` — see §6.

---

## 6. TCP is a byte stream, not a message stream

`[External]` A `SOCK_STREAM` (TCP) socket has **no message boundaries** — the same
property `06_pipes_and_signals.md` §1 established for pipes. Two `send`s of 10 bytes may
arrive as one `recv` of 20 bytes, or as three `recv`s of 7, 7, and 6. A protocol built on
TCP therefore must define its own framing: a fixed-size header, a length prefix (send a
count — in network byte order! — then that many bytes), or a delimiter such as `\n`. The
robust idiom for sending everything:

```c
/* send exactly n bytes, or fail */
static int send_all(int fd, const char *buf, size_t n) {
    size_t sent = 0;
    while (sent < n) {
        ssize_t r = send(fd, buf + sent, n - sent, 0);
        if (r < 0) return -1;              /* real code also handles EINTR (06_… §4) */
        sent += (size_t)r;
    }
    return 0;
}
```

A `recv` that returns **`0`** means the peer closed its end cleanly (the network analog of
a pipe's EOF); `-1` is an error.

---

## 7. Useful network functions — and one that's obsolete

`[Book-derived, A]` Book A lists `gethostname` and `gethostbyname`. `[External, verified
against the Linux `gethostbyname(3)` manual page]` **`gethostbyname` and its relatives are
obsolete**: the manual states they were marked obsolescent in POSIX.1-2001 and removed in
POSIX.1-2008, recommending **`getaddrinfo`/`getnameinfo`** instead. A second, subtler reason
to avoid it: it "may return pointers to static data, which may be overwritten by later
calls" — the same non-reentrancy hazard as `strtok` (`libraries/03_string.md` §5), for the
same underlying cause (hidden static state). `getaddrinfo` also handles IPv6 and service
names uniformly, so code written against it doesn't hard-code `AF_INET`. Likewise
`inet_addr` (Book A's text-to-address conversion) has a wart — its error return,
`INADDR_NONE`, is a valid address (`255.255.255.255`) — which is why `inet_pton` is the
usual modern replacement `[External]`.

---

## 8. ⚠️ Misconceptions to unlearn now

- **"One `send` on one side equals one `recv` on the other."** TCP preserves byte order,
  never message boundaries (§6).
- **"Ports and addresses can be assigned directly, like `dest.sin_port = 80`."** They must go
  through `htons`/`htonl` (§3) — direct assignment works only on big-endian hosts.
- **"The listening socket is also the socket you talk to the client on."** `accept` returns a
  separate session descriptor (§4).
- **"Leaving out error checks in example code is harmless."** In §5, four of five real bugs
  exist *only* because error returns were ignored.

---

## 9. 🎯 Interview questions

**Easy**
- What does `accept` return? → A new file descriptor for the accepted connection (§4).
- What does `recv` return when the peer has closed the connection? → `0` (§6).

**Medium**
- Why must you call `htons` on a port number? → The wire format is big-endian (network byte
  order); on little-endian hosts the bytes must be swapped, and `htons` does that portably
  (§3).
- Why does a server need two kinds of sockets (listening vs. session)? → The listening
  socket keeps accepting new clients; each session socket carries one conversation (§4).
- Why might `send(fd, buf, 1000, 0)` return `600`? → It can transfer fewer bytes than
  requested; the caller must loop for the rest (§6).

**Hard**
- Explain why `buffer[len] = '\0'` after an unchecked `recv` is undefined behavior. →
  On error `recv` returns `-1`, so this writes `buffer[-1]`, an out-of-bounds store (§5).
- How would you design a message protocol over TCP, given it has no message boundaries? →
  Add framing: a length prefix (in network byte order) followed by the payload, or a
  delimiter; receivers loop on `recv` until a whole frame has arrived (§6).
- Why is `gethostbyname` discouraged beyond being obsolete? → It returns pointers into
  static storage that later calls overwrite (non-reentrant, not thread-safe), and it
  predates IPv6 — `getaddrinfo` fixes both (§7).

**Systems-level**
- Your `fork`-per-connection server slowly stops accepting clients after days of uptime,
  with no crash. List the resource leaks worth checking. → Un-reaped children (zombies —
  missing `SIGCHLD`/`waitpid`), un-closed session descriptors in the parent after `fork`
  (each leaks a descriptor toward the per-process limit), and un-closed listening
  descriptors in children (§4, §5, `05_processes_fork_exec_wait.md` §6).
- A network client on a little-endian machine reads a 4-byte length field from the socket
  and gets absurdly large values. What went wrong? → The field arrived in network (big-
  endian) byte order but was used as a host `int` without `ntohl` (§3).

---

## 10. External references

- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/) `[External]` — the
  widely used, beginner-friendly companion for exactly this material, including
  `getaddrinfo`-based client/server templates (verify the specific page at point of use).
- [man7.org — `socket(2)`, `bind(2)`, `listen(2)`, `accept(2)`, `connect(2)`, `recv(2)`, `send(2)`](https://man7.org/linux/man-pages/)
  `[External]` — the authoritative per-call documentation.
- [man7.org — `gethostbyname(3)`](https://www.man7.org/linux/man-pages/man3/gethostbyname.3.html)
  `[External]` — the source for §7's obsolescence facts (also lists `getaddrinfo(3)`).
- [man7.org — `getaddrinfo(3)`](https://man7.org/linux/man-pages/) `[External]` — the modern
  name/address resolution interface.

## 11. 🛠 Practice

1. Build Book A's client/server pair, adding all the error checks from §2 and fixing all five
   issues in §5; confirm they interoperate over `127.0.0.1`.
2. Use `strace` (previewed; `debugging/`, Phase 10) on both to watch the exact system-call
   sequence from the §1 diagram — `socket`, `bind`, `listen`, `accept`, `connect`, `sendto`/
   `recvfrom`-family calls.
3. Extend the server to `fork` per connection with a `SIGCHLD` reaping handler
   (`06_pipes_and_signals.md` §4); connect many clients quickly and confirm with `ps` that no
   zombies accumulate and descriptor counts stay flat.
4. Write a length-prefixed echo protocol (`htonl`-encoded 4-byte length, then payload) and
   test it with messages far larger than one `recv` returns, to make §6 concrete.
