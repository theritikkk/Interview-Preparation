# 15. Final Revision Cheat Sheet
### *(The whole book, compressed)*

Use this as your last pass before you consider yourself done. If every section here makes sense at a glance, you've genuinely absorbed the material — if something makes you pause, that's your pointer to which earlier file to reread.

## The cross-platform foundation (Chapter 2), underneath everything

```c
#if defined(_WIN32)
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #define ISVALIDSOCKET(s) ((s) != INVALID_SOCKET)
  #define CLOSESOCKET(s) closesocket(s)
  #define GETSOCKETERRNO() (WSAGetLastError())
#else
  #include <sys/socket.h> /* + netinet/in.h, arpa/inet.h, netdb.h, unistd.h, errno.h */
  #define ISVALIDSOCKET(s) ((s) >= 0)
  #define CLOSESOCKET(s) close(s)
  #define SOCKET int
  #define GETSOCKETERRNO() (errno)
#endif
```
Windows needs `WSAStartup()`/`WSACleanup()` around all socket use; Unix needs neither. Every program in this book (and these notes) is built on this one abstraction layer.

## The two core call sequences

```
 TCP SERVER                             TCP CLIENT
 ──────────                             ──────────
 getaddrinfo(NULL, port, AI_PASSIVE)    getaddrinfo(hostname, port)
 socket()                               socket()
 setsockopt(SO_REUSEADDR)   <- always!
 bind()
 listen()
 accept() -> new socket                 connect()
 recv()/send() on new socket            recv()/send() on socket
 close()                                close()
```
UDP drops `listen`/`accept`/`connect` — server: `getaddrinfo → socket → bind → recvfrom`; client: `getaddrinfo → socket → sendto` (or `connect()` once, then plain `send`/`recv`, per Chapter 4).

## Layer stack (Chapter 1)

```
Application     <- HTTP, DNS, SMTP, SSH, MQTT/CoAP    (you live here)
Transport       <- TCP or UDP                         (you choose this)
Internet        <- IP, ICMP                           (kernel handles this)
Link            <- Ethernet, Wi-Fi, 802.15.4          (hardware/driver handles this)
```

## Handling many clients at once — three techniques (Chapter 3)

| Technique | Cost model | Cross-platform? |
|---|---|---|
| `fork()`-per-client | One OS process per client | Unix only |
| `select()` | One process, `fd_set` bitmask, must track `fdmax` yourself | Yes, but `FD_SETSIZE` (typically 1024) is a hard ceiling |
| `poll()` | One process, growable `struct pollfd[]` array | Yes (not covered by name in this book — this is the Beej's-Guide-covered alternative) |

**Always put the listening socket in the same set as every client** — "ready to read" on it means a new connection is waiting to be `accept()`-ed, not data waiting to be `recv()`-ed.

## Function return-value conventions

| Pattern | Functions |
|---|---|
| `0`/error-code + a `*_strerror()` decoder | `getaddrinfo()`, `getnameinfo()` |
| `-1` + global `errno`/`GETSOCKETERRNO()` | `socket`, `bind`, `connect`, `listen`, `accept`, `send`, `recv`, `sendto`, `recvfrom`, `select` |
| Bytes actually transferred (may be less than requested) | `send()`, `sendto()`, `SSL_write()` |
| Bytes read, or `0` specifically for "peer closed" | `recv()`, `recvfrom()`, `SSL_read()` |
| A brand-new descriptor, distinct from the one called on | `accept()` |

## Protocols covered, one line each

| Protocol | Transport | Chapter | One-line summary |
|---|---|---|---|
| Raw TCP/UDP | — | 3–4 | Byte stream (TCP) vs. self-contained datagrams (UDP) — the foundation everything else sits on |
| DNS | UDP (usually) | 5 | 12-byte header + questions/answers; names are length-prefixed labels; responses compress repeated names via back-pointers |
| HTTP | TCP | 6–7 | Plain-text request/response; body length via `Content-Length`, chunked encoding, or connection-close |
| SMTP | TCP | 8 | Strict command→3-digit-code turn-taking; envelope (`MAIL FROM`) and message headers (`From:`) are separate and can disagree |
| TLS/HTTPS | TCP (wrapped) | 9–10 | Encryption + integrity + certificate-based authentication; `SSL_read`/`SSL_write` replace `recv`/`send` after the handshake |
| SSH | TCP (wrapped) | 11 | Trust-on-first-connect host verification (not CA-based); channels carry commands/file-transfer within one connection |
| ICMP (raw sockets) | — | 12 | No ports; powers `ping`/`traceroute`; needs root/`CAP_NET_RAW` — a deliberately bigger trust boundary than normal sockets |

## The TLS/SSH setup skeleton (Chapters 9–11), side by side

```c
// TLS (OpenSSL) — client side
SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
SSL *ssl = SSL_new(ctx);
SSL_set_tlsext_host_name(ssl, hostname);   // SNI
SSL_set_fd(ssl, socket_peer);
SSL_connect(ssl);                           // the handshake
// ... SSL_read()/SSL_write() ...

// TLS (OpenSSL) — server side
SSL_CTX *ctx = SSL_CTX_new(TLS_server_method());
SSL_CTX_use_certificate_file(ctx, "cert.pem", SSL_FILETYPE_PEM);
SSL_CTX_use_PrivateKey_file(ctx, "key.pem", SSL_FILETYPE_PEM);
// per client: SSL_new(ctx) -> SSL_set_fd -> SSL_accept()

// SSH (libssh) — client side
ssh_session ssh = ssh_new();
ssh_options_set(ssh, SSH_OPTIONS_HOST, hostname);
ssh_connect(ssh);
ssh_userauth_password(ssh, 0, password);
ssh_channel channel = ssh_channel_new(ssh);
ssh_channel_open_session(channel);
ssh_channel_request_exec(channel, command);
```
Link flags to never forget: `-lssl -lcrypto` for OpenSSL, `-lssh` for libssh.

## Real bugs and gaps this project actually found (not hypothetical — verified live)

1. **`sprintf(buffer, "Content-Length: %u\r\n", cl)`** where `cl` is `size_t` — a real format-specifier/type mismatch, present in both `web_server.c` and `https_server.c`. Fix: `%zu`.
2. **This book's servers don't call `SO_REUSEADDR`** until Chapter 13 explicitly revisits it — I hit genuine `"bind() failed. (98)"` errors testing Chapters 7 and 10, exactly as Chapter 13 predicts.
3. **`ieee754.c`-equivalent pattern** (from the *other* book covered in these notes, Beej's Guide) and **this book's `ssh_download.c`** both show the same lesson from different angles: **published example code can rely on something that's since changed** — a macro the printed source never defines, or an entire API (`ssh_scp_*`) the library maintainers have since deprecated in favor of SFTP. Reading and running the actual code, rather than trusting a summary, is what surfaces this.
4. **`SIGPIPE` didn't reproduce on the first testing attempt** — a graceful client close let both `send()`s in `server_crash.c` succeed; only an abortive `SO_LINGER`-forced RST close reliably triggered the real crash (signal 13). The lesson generalizes: a pitfall being "well known" doesn't mean it's trivially reproducible on demand — the exact trigger conditions matter.

## Self-test

1. Why does `accept()` return a *different* file descriptor than the one passed to `listen()`?
2. What are the three ways an HTTP client can know a response body is complete, and which one requires hex, not decimal, parsing?
3. Why do `MAIL FROM:` and a message's `From:` header not have to match — and why does that matter for security?
4. What's the actual difference between how TLS and SSH each answer "am I talking to who I think I am"?
5. Why did forcing an abortive close (`SO_LINGER` with a zero timeout) succeed at triggering `SIGPIPE` when a graceful close didn't?
6. Name one thing a microcontroller-based IoT device's networking code has to worry about that a full Linux server never does.

*(Answers: 1 — so the original listening socket can keep accepting further connections while this new socket handles one specific, already-established client. 2 — `Content-Length` (decimal), `Transfer-Encoding: chunked` (each chunk's length prefix is hexadecimal), or connection-close (read until EOF). 3 — the envelope address is what routing/delivery actually uses, while the header address is just displayed text with no cryptographic tie to the envelope — which is exactly the gap classic sender-spoofing exploits. 4 — TLS relies on a chain of trust rooted in pre-installed Certificate Authorities; SSH relies on trust-on-first-connect plus a remembered per-host fingerprint, with no third-party authority involved at all. 5 — an RST makes the kernel aware the connection is dead immediately, whereas a graceful FIN can leave the first `send()` still succeeding until the kernel has fully processed the close — timing, not the underlying rule, was the variable. 6 — memory/flash constraints tight enough that even a single connection's buffers are a meaningful fraction of total RAM, and often no general-purpose OS underneath the networking code at all.)*

---

You've now been through every code example this book's own official repository provides — pulled from its real, MIT-licensed source, compiled with zero suppressed warnings, and actually run: a real multi-client chat room, a real DNS query against 8.8.8.8, a real HTTP round trip between the book's own client and server, a real SMTP session against a live test mail server, a real TLS handshake with certificate inspection, a real local SSH server handling genuine password auth, remote command execution, and file transfer, and a real, live-reproduced `SIGPIPE` crash. That's a solid, verified foundation — from here, the natural next step is picking one of these programs and extending it yourself: add SFTP support to replace `ssh_download.c`'s deprecated SCP calls, add `STARTTLS` to the SMTP client, or convert one of the `select()`-based servers to `poll()` and compare the two directly, the same way Beej's Guide's own notes did.