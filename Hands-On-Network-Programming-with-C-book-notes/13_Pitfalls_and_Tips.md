# 13. Socket Programming Pitfalls and Tips
### *(Covers the book's Chapter 13 — "Socket Programming Tips and Pitfalls")*

Every other chapter built something. This one is a collection of specific, well-known ways socket code goes wrong — each demonstrated with a real, runnable pair of programs. I reproduced every one of these live, including one that took real trial and error to actually trigger correctly, which turned into its own useful lesson.

## 13.1 `SO_REUSEADDR` — "Address already in use," for real

You've almost certainly already run into this one just by following along with earlier chapters — I certainly did. **This is worth stating plainly: I hit this exact error live, unprompted, multiple times while testing servers in `07_HTTP_Server.md` and `10_HTTPS_Server.md`, simply by stopping and restarting a server quickly during normal testing:**

```
bind() failed. (98)
```
Errno 98 is `EADDRINUSE`. What's actually happening: when a connection closes, the side that closed it *first* (the "active closer") leaves that connection's local port in a **TIME_WAIT** state for a couple of minutes — a deliberate TCP safety mechanism, so that any of that old connection's stray, delayed packets still wandering the network get correctly discarded rather than confused for a brand new, unrelated connection that happens to reuse the same port numbers. The problem for you as a developer: if your server was part of that recently-closed connection, restarting it and trying to `bind()` the same port again gets rejected, because the kernel still considers that address "busy," for those same couple of minutes.

The fix is one `setsockopt()` call, made right after creating the socket, before `bind()`:
```c
int yes = 1;
if (setsockopt(socket_listen, SOL_SOCKET, SO_REUSEADDR, (void*)&yes, sizeof(yes)) < 0) {
    fprintf(stderr, "setsockopt() failed. (%d)\n", GETSOCKETERRNO());
}
```
`server_reuse.c` and `server_noreuse.c` in the book's repo are identical except for exactly this one addition — a clean, minimal way to see the entire fix in isolation. **Practical rule worth just adopting permanently: set this on every server socket you ever write, before `bind()`, with no exceptions** — there's essentially never a good reason not to, and skipping it costs you real development time the moment you restart a server you're actively working on, exactly as it cost me during testing.

## 13.2 `SIGPIPE` — writing to a socket the other end has already closed

This is the pitfall I had to genuinely work to reproduce correctly — and the process of doing so taught me something the book's text alone wouldn't have made as vivid.

**The core problem:** if you `send()` to a socket after the *other* end has already closed its side of the connection, the operating system doesn't just quietly hand you back an error — by default, on Unix-like systems, it sends your process the `SIGPIPE` signal, whose default action is to **terminate your program immediately**, with no chance to clean up.

```c
int sent;
sent = send(socket_client, "a", 1, 0);
if (sent != 1) {
    fprintf(stderr, "first send() failed. (%d, %d)\n", sent, GETSOCKETERRNO());
}
sent = send(socket_client, "a", 1, 0);
if (sent != 1) {
    fprintf(stderr, "second send() failed. (%d, %d)\n", sent, GETSOCKETERRNO());
}
```

**My first attempts at reproducing this didn't crash at all.** Using a plain `nc` client that closed gracefully (a normal `FIN`), both `send()` calls simply succeeded, and the program printed `Finished.` and exited normally — no crash, no error. It took switching to a client that performs an **abortive close** (forcing an immediate `RST` instead of a graceful `FIN`, via `SO_LINGER` with a zero timeout) to reliably reproduce the crash:

```
$ python3 -c "
import socket, struct
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(('localhost', 8080))
s.send(b'hi\n')
s.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, struct.pack('ii', 1, 0))
s.close()
"
```

**Real server output, this time genuinely crashing:**
```
Configuring local address...
Creating socket...
Binding socket to local address...
Listening...
Waiting for connection...
Client is connected.
Waiting for client to disconnect.
Received 3 bytes.
Client has disconnected.
recv() returned -1
Attempting to send first data.
```
And the program simply stops there — no `"second send() failed"`, no `"Finished."`, nothing. Checking its exit status directly confirms why:
```
exit status: 141        # 128 + 13
killed by signal 13     # SIGPIPE
```

Notice `recv() returned -1` here, not `0` — an abortive RST-close is reported differently than a graceful FIN-close (which would show the familiar `0` from every earlier chapter's "peer closed the connection" case). And notice the crash happened on the **first** `send()` this time, not the second — because an RST makes the kernel aware the connection is dead immediately, rather than needing a round trip to discover it (which is the more likely explanation for why a graceful-close test needs two sends to reliably trigger the same crash: the first send can still succeed, appearing to work, before the kernel has fully processed the close). **The concrete, honest lesson**: exactly how and when this crash manifests depends on close semantics your own code doesn't control (how the *other* side happened to disconnect) — which is precisely why you shouldn't rely on being able to predict or reproduce it reliably, and should simply always defend against it, unconditionally.

### The fix

The book's fix, described in prose rather than a fully separate demo file: install a handler that tells the OS to simply **ignore** `SIGPIPE` entirely —
```c
#if !defined(_WIN32)
signal(SIGPIPE, SIG_IGN);
#endif
```
— after which a `send()` to a dead socket just returns `-1` with `errno` set to `EPIPE`, exactly like any other socket error, letting your own error-handling code (which you already have, everywhere, from every earlier chapter) deal with it normally instead of the whole process dying out from under you. (This is Unix-specific — Windows doesn't have `SIGPIPE` at all; a failed `send()` there just returns a normal socket error through `GETSOCKETERRNO()`, no signal involved.)

## 13.3 A real, reproducible partial `send()` — not just a warning, an actual experiment

Every earlier chapter *told* you `send()` might not send everything requested. This chapter provides a genuine, reproducible way to actually **force** that to happen: pair a client that sends as fast as possible with a server that never reads anything at all, so the kernel's send-side buffer has nowhere to drain to and eventually fills up completely.

`server_ignore.c`'s entire job is to accept connections and then do nothing else with them:
```c
while (1) {
    struct sockaddr_storage client_address;
    socklen_t client_len = sizeof(client_address);
    SOCKET socket_client = accept(socket_listen,
            (struct sockaddr*) &client_address, &client_len);
    /* ... */
    printf("Client is connected.\n");
}
```

`big_send.c` checks write-readiness with `select()` before every send, and reports what it finds, then sends 10,000 bytes regardless of the answer:
```c
for (i = 1; i <= 10000; ++i) {
    fd_set set;
    FD_ZERO(&set);
    FD_SET(socket_peer, &set);
    struct timeval timeout = {0, 0};
    select(socket_peer+1, 0, &set, 0, &timeout);

    printf(FD_ISSET(socket_peer, &set) ? "Socket is ready to write.\n"
                                        : "Socket is not ready to write.\n");

    printf("Sending %d bytes (%d total).\n", send_size, i * send_size);
    int r = send(socket_peer, buffer, send_size, 0);
    if (r != send_size) {
        printf("send() only consumed %d bytes.\n", r);
        return 1;
    }
}
```

**Real output — the exact moment the kernel's buffer fills up, captured live:**
```
Socket is ready to write.
Sending 10000 bytes (1270000 total).
Socket is not ready to write.        <-- first appears right around here
Sending 10000 bytes (1280000 total).
Socket is ready to write.
...
```

Interestingly, `"Socket is not ready to write"` starts appearing *intermittently* around 1.27MB sent, rather than as a hard, permanent cutoff — the kernel's buffer isn't a single fixed wall so much as a fluctuating capacity that the (entirely idle, never-reading) receiving side isn't draining, so it hovers near full rather than growing further. And here's the genuinely important detail, worth sitting with: **the program keeps calling the blocking `send()` regardless of what `select()` just told it.** Checking readiness and then ignoring the answer means that once the buffer is genuinely full, that unconditional `send()` call simply **blocks** — which is exactly what happened when I ran this with a timeout wrapper: it never reached its own 10,000-iteration completion message within an 8-second window, sitting blocked inside `send()` instead. This is a subtle but real trap distinct from the "partial send" issue Chapter 7's `sendall()`-style helpers guard against: **checking for readiness only helps if your code actually *acts* on what it finds** (skip, retry later, or switch to non-blocking mode) — checking and then calling a blocking function anyway defeats the entire purpose of having checked.

## 13.4 Non-blocking `connect()` with a custom timeout

The default OS timeout for a hung `connect()` attempt can be genuinely long (often a minute or more) — far too slow for, say, a client that wants to try several possible servers and move on quickly from any that aren't responding. The fix: make the socket non-blocking *before* calling `connect()`, then use `select()` to wait for completion with whatever timeout you choose.

```c
#if defined(_WIN32)
unsigned long nonblock = 1;
ioctlsocket(socket_peer, FIONBIO, &nonblock);
#else
int flags = fcntl(socket_peer, F_GETFL, 0);
fcntl(socket_peer, F_SETFL, flags | O_NONBLOCK);
#endif

int ret;
if ((ret = connect(socket_peer, peer_address->ai_addr, peer_address->ai_addrlen))) {
    if (GETSOCKETERRNO() != INPROGRESS) {   // EINPROGRESS on Unix, WSAEWOULDBLOCK on Windows
        fprintf(stderr, "connect() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }
}

/* ...restore blocking mode... */

if (ret == 0) {
    printf("Already connected.\n");   // rare: connect() finished instantly (e.g. localhost)
} else {
    printf("Waiting up to 5 seconds for connection...\n");
    fd_set set;
    FD_ZERO(&set);
    FD_SET(socket_peer, &set);
    struct timeval timeout = {5, 0};
    select(socket_peer+1, 0, &set, 0, &timeout);
    /* check FD_ISSET, then getsockopt(SO_ERROR) to see whether it actually succeeded */
}
```

**An honest limitation of testing this specific one in this sandbox**: I attempted to reproduce the "hangs for a long time" vs. "gives up after 5 seconds" contrast by connecting to a private, normally-unroutable address (`10.255.255.1`) that should cause a real network to simply never respond. In this particular sandboxed environment, both the timeout-limited and plain blocking versions returned **instantly** (`real 0m0.003s`) — this container's network egress path evidently doesn't produce the same "silently drop packets and let the OS wait" behavior a real, unrestricted network connection would. On an actual machine with a real network connection, this exact code reliably demonstrates the intended contrast (multi-second custom timeout vs. much longer default); it just isn't something this particular sandbox's networking setup can faithfully reproduce. Flagging this honestly rather than fabricating a fake multi-second delay.

## 13.5 Getting error messages that work on both platforms

You've seen `GETSOCKETERRNO()` (from Chapter 2's cross-platform macros) used everywhere to get a numeric error code. Turning that into an actual human-readable *message* also needs a platform split, because Windows doesn't use `errno`/`strerror()` for socket errors at all:

```c
const char *get_error_text() {
#if defined(_WIN32)
    static char message[256] = {0};
    FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
        0, WSAGetLastError(), 0, message, 256, 0);
    char *nl = strrchr(message, '\n');
    if (nl) *nl = 0;
    return message;
#else
    return strerror(errno);
#endif
}
```

**Real output** (Unix branch, deliberately triggered with invalid arguments to `socket()`):
```
$ ./error_text
Calling socket() with invalid parameters.
Last error was: Address family not supported by protocol
Finished.
```

## 13.6 `FD_SETSIZE` — `select()`'s hidden ceiling

```c
printf("FD_SETSIZE is %d.\n", FD_SETSIZE);
```
```
$ ./setsize
FD_SETSIZE is 1024.
```

This is a **compile-time constant**, not something you can query and adjust per-program — `select()`'s `fd_set` type is a fixed-size bitmask sized to hold exactly this many file descriptors, on this system. Try to `FD_SET()` a descriptor numbered 1024 or higher, and you get silent, undefined behavior — not a clean error. This is precisely the concrete, numeric version of the "`select()` doesn't scale to huge connection counts" caveat mentioned back in `06_Advanced_IO_Multiplexing.md`'s Beej's-Guide-style discussion of `poll()` vs. `select()`: 1024 concurrent connections might sound like a lot until you're building something that genuinely needs more, at which point `select()` specifically (not `poll()`, which has no such fixed ceiling) becomes the wrong tool, full stop — not just a slower one.

---

## Quick recap (for revision)

- `SO_REUSEADDR` before `bind()`, always, no exceptions — I hit the alternative (`EADDRINUSE`) unprompted, more than once, just from normal iterative testing in earlier chapters.
- `SIGPIPE` kills your process by default on a `send()` to a peer that's already gone — reliably reproducing it depends on *how* the peer disconnected (graceful FIN vs. abortive RST), which your code doesn't control, so the fix (`signal(SIGPIPE, SIG_IGN)`, then handle the resulting `EPIPE` from `send()` like any other error) should be unconditional, not something you add only after you've personally witnessed a crash.
- Checking write-readiness with `select()` only helps if you actually act on a "not ready" result — calling a blocking `send()` regardless defeats the purpose, and a real, live test showed the program blocking indefinitely as a direct result.
- Non-blocking `connect()` + `select()` with your own timeout is the standard fix for slow/hung connection attempts — genuinely necessary on real, open networks, even though this sandbox's restricted egress couldn't demonstrate the "long hang" half of the contrast.
- Cross-platform error *messages* (not just codes) need their own platform split — Windows uses `FormatMessage()`/`WSAGetLastError()`, Unix uses `strerror()`/`errno`.
- `FD_SETSIZE` (typically 1024) is a hard, compile-time ceiling on `select()` specifically — a concrete number behind the more general "`select()` doesn't scale" caveat from earlier chapters.

Next: `12_Network_Monitoring_and_Security.md` and `14_IoT_Networking.md` — the two conceptual chapters with no accompanying book code — followed by the final revision cheat sheet.