# 8. Common Questions
### *(Covers the guide's Chapter 8 — condensed FAQ)*

This chapter of the original guide is a grab-bag of "wait, but what about..." questions that come up once you start writing real code. Here are the ones most worth knowing, explained rather than just answered.

## "bind() says 'Address already in use' — what do I do?"

Covered in Chapter 3, but worth repeating since it's the single most common error beginners hit: set `SO_REUSEADDR` on the listening socket *before* calling `bind()`.
```c
int yes = 1;
setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
```
This happens because the kernel keeps a just-closed socket in a brief cooldown state; `SO_REUSEADDR` tells it "let me rebind anyway."

## "Do I need an actual network to test client/server code?"

No. Every machine has a **loopback device** (`lo` in your routing table) — a fake network interface that exists purely in the kernel and just hands packets straight back to your own machine. This is exactly what every demo in these notes has been doing: running both the client and server as two processes on one machine, connecting to `localhost` or `127.0.0.1`. **No code changes are needed** to make a program work this way — it's the same `getaddrinfo()`/`socket()`/`connect()` calls either way; the loopback device is invisible to your program's logic.

## "How do I tell if the other side hung up?"

`recv()` returns `0`. Not `-1` (that's an error), not blocking (that's "no data *yet*") — specifically `0` means "the peer has cleanly closed their end; there will be no more data." You've seen this exact check in every server in Chapters 4 and 6 (`if (nbytes == 0) { /* connection closed */ }`).

## "How do I shorten the timeout on connect()?"

By default, `connect()` can take a genuinely long time to fail if the remote host is unreachable (as opposed to actively refusing, which — as you saw in Chapter 4 — is fast). The technique: make the socket non-blocking *before* calling `connect()`. It'll return `-1` immediately with `errno == EINPROGRESS`, meaning "started, not done yet." Then call `select()` with your own timeout, watching that socket in both the read and write sets. If `select()` returns before your timeout, the connection attempt has resolved one way or the other; check which with `getsockopt(sockfd, SOL_SOCKET, SO_ERROR, ...)`. Remember to set the socket back to blocking afterward if you want normal `send()`/`recv()` behavior going forward.

## "Why does select()/poll() randomly return -1 with errno == EINTR?"

A signal arrived and interrupted the blocked system call — this is normal, expected Unix behavior, not a bug in your program. `sigaction()`'s `SA_RESTART` flag is *supposed* to make the kernel auto-retry the interrupted call for you, but it doesn't cover every case reliably across every system. The book's own pragmatic (and self-aware, "yes I'm using goto") fix:
```c
select_restart:
if ((err = select(fdmax + 1, &readfds, NULL, NULL, NULL)) == -1) {
    if (errno == EINTR) {
        goto select_restart; // interrupted by a signal, just try again
    }
    perror("select"); // a genuine error
}
```
The takeaway even if you never use `goto` yourself: **always check for `EINTR` specifically** around blocking calls in any program that also installs signal handlers (like the `SIGCHLD` reaper from Chapter 4), and treat it as "try again," not "something broke."

## "How do I put a timeout on recv()?"

Wrap it with `select()`, using a distinct return value for "timed out" that can't be confused with `recv()`'s own return values (`0` = peer closed, `-1` = error):
```c
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

int recvtimeout(int s, char *buf, int len, int timeout)
{
    fd_set fds;
    int n;
    struct timeval tv;

    FD_ZERO(&fds);
    FD_SET(s, &fds);

    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    n = select(s + 1, &fds, NULL, NULL, &tv);
    if (n == 0) return -2; // timeout!
    if (n == -1) return -1; // error

    return recv(s, buf, len, 0); // data must be here
}
```
Called like: `n = recvtimeout(s, buf, sizeof buf, 10);` then check `n == -1` (error), `n == -2` (timeout), or otherwise treat `n` as a normal `recv()` byte count.

## "How do I encrypt/compress data before sending it?"

Think of your data pipeline as a sequence of transformation steps, and slot your own step in between "read the data" and "`send()` the data":
```
 server: read data → [encrypt/compress — you add this] → send()
 client: recv() → [decrypt/decompress — you add this] → use/write the data
```
One rule of thumb worth remembering: **compress before you encrypt**, not after (encrypted data looks like random noise, which compresses poorly). For real-world encryption, reach for an established library like OpenSSL/TLS rather than rolling your own — this guide's scope stops at "where in the pipeline this would go."

## "How do I write a server that runs shell commands from a client?"

You can — `accept()`, `recv()` the command string, then `system(str)` — but the book is emphatic that **this is a serious security hole** unless you're extremely careful. A client that can send arbitrary strings to be executed as shell commands can do arbitrary damage (`"rm -rf ~"` being the canonical example). Even an "allow-list" of specific safe-looking commands isn't safe on its own — shell metacharacters (`;`, `|`, `&&`, backticks) let an attacker chain a permitted command with an arbitrary one (`"foobar; rm -rf ~"`). If you ever do build something like this for real, you need to properly escape or reject non-alphanumeric input, and ideally avoid `system()` entirely in favor of directly `exec`-ing a fixed, known program with validated arguments.

## "I'm only receiving 536 or 1460 bytes per recv() call over a real network, but it all arrives at once on localhost — what's going on?"

You've hit the **MTU** (Maximum Transmission Unit) — the largest chunk of data the underlying physical medium can carry in one frame. Ethernet's MTU is around 1500 bytes (so ~1460 bytes of your actual data once headers are subtracted); older dial-up-style links used to cap around 576. The **loopback device** used for localhost testing has no such real hardware limit and typically allows much larger chunks through in one go (8KB or more) — which is exactly why code that "worked fine" in every demo in these notes can still need a `recv()`-in-a-loop when it meets a real network. This is precisely why `sendall()` (Chapter 7) exists on the sending side, and why length-prefixed framing ("Son of Data Encapsulation," also Chapter 7) exists on the receiving side — both are there specifically because a single send/receive pair is not guaranteed to move all your data in one system call once real network hardware (not loopback) is involved.

## "How do I know which port to use?"

If you're implementing a well-known protocol, use its assigned port (80 for HTTP, 22 for SSH, etc. — see the IANA port list mentioned in Chapter 2). For your own custom protocol, pick anything above 1023 and just try it — if it's taken, `bind()` will tell you plainly (`"Address already in use"`), and you pick another. Being in IANA's registry doesn't grant exclusive rights to a port number on your own machine; it's just a convention to reduce collisions between well-known services.

---

## Quick recap (for revision)

- Loopback (`localhost`/`127.0.0.1`) needs zero code changes to work — it's not a "test mode," it's a real (if local-only) network interface.
- `recv() == 0` → peer closed. `recv() == -1` → real error. Blocking → no data *yet*. Never conflate these three.
- `EINTR` after a signal interrupts a blocking call is normal — retry, don't treat it as a hard failure.
- A `recv()`/`select()` timeout needs its own distinct sentinel value (the book uses `-2`) since `0` and `-1` are already taken by `recv()`'s own meanings.
- Real networks have a real MTU (~1460 bytes of payload on Ethernet); loopback doesn't — code that "just works" in local testing can still need `sendall()`-style looping and length-prefixed framing once it hits a real network.
- Never `system()` a client-supplied string without serious escaping/allow-listing — this is a genuine, classic security hole, not a theoretical one.

Next: `09_ManPages_Quick_Reference.md` — every function from this whole guide, its signature, and what its return value means, in one scannable place.