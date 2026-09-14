# 9. Quick-Reference: Every Function, One Page
### *(Covers the guide's Chapter 9 — condensed for fast lookup, not first-time learning)*

This chapter is not meant to teach you anything new — if you've read Chapters 3, 6, and 7, you've already met every function here in context. This is the page you come back to later, when you just need to check an argument order or a return-value convention without re-reading a full explanation. Grouped by what you use them for, not alphabetically — connection setup, data transfer, monitoring, then conversion/utility functions.

## Connection setup (client & server)

| Function | Signature | Returns |
|---|---|---|
| `getaddrinfo()` | `int getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res);` | `0` on success, nonzero error code otherwise — use `gai_strerror(rv)` to print it. **Not** the usual `-1`/`errno` pattern. |
| `freeaddrinfo()` | `void freeaddrinfo(struct addrinfo *res);` | Nothing — always call this once you're done with a `getaddrinfo()` result. |
| `gai_strerror()` | `const char *gai_strerror(int errcode);` | Human-readable string for a `getaddrinfo()` error code. |
| `socket()` | `int socket(int domain, int type, int protocol);` | Socket descriptor, or `-1` (+ `errno`). |
| `bind()` | `int bind(int sockfd, struct sockaddr *my_addr, int addrlen);` | `0` on success, `-1` (+ `errno`) on error. Servers only. |
| `connect()` | `int connect(int sockfd, struct sockaddr *serv_addr, int addrlen);` | `0` on success, `-1` (+ `errno`) on error. Clients only — no `bind()` needed first. |
| `listen()` | `int listen(int sockfd, int backlog);` | `0` on success, `-1` (+ `errno`) on error. `backlog` = max queued unaccepted connections (systems often cap ~20 regardless). |
| `accept()` | `int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);` | A **new** socket descriptor for the one client, or `-1` (+ `errno`). `addr` is typically `struct sockaddr_storage`; init `*addrlen = sizeof(*addr)` before calling. |
| `close()` | `int close(int fd);` | `0` on success, `-1` (+ `errno`). Frees the descriptor entirely; use for any socket you're fully done with. |
| `shutdown()` | `int shutdown(int sockfd, int how);` | `0`/`-1`. `how`: `0`=no more receives, `1`=no more sends, `2`=both. **Doesn't** free the fd — still call `close()` after. |

## Sending & receiving data

| Function | Signature | Returns |
|---|---|---|
| `send()` | `int send(int sockfd, const void *msg, int len, int flags);` | **Bytes actually sent** (may be `< len`!) or `-1` (+ `errno`). Connected sockets (TCP, or `connect()`-ed UDP) only. Set `flags = 0` normally. |
| `recv()` | `int recv(int sockfd, void *buf, int len, int flags);` | Bytes actually read, `0` if peer closed the connection, or `-1` (+ `errno`) on error. Does **not** null-terminate the buffer for you. |
| `sendto()` | `int sendto(int sockfd, const void *msg, int len, unsigned int flags, const struct sockaddr *to, socklen_t tolen);` | Same as `send()`, plus you supply the destination address every call. Unconnected UDP. |
| `recvfrom()` | `int recvfrom(int sockfd, void *buf, int len, unsigned int flags, struct sockaddr *from, int *fromlen);` | Same as `recv()`, plus fills in who sent the packet. Unconnected UDP. |

## Monitoring multiple sockets

| Function | Signature | Returns |
|---|---|---|
| `poll()` | `int poll(struct pollfd fds[], nfds_t nfds, int timeout);` | Count of descriptors with an event, `0` on timeout, `-1` (+ `errno`) on error. `timeout` in **milliseconds**; negative = forever. Check `revents & POLLIN` per entry. |
| `select()` | `int select(int numfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);` | Same semantics as `poll()`. `numfds` = **highest fd across all sets, plus one** — a very easy detail to forget. `timeout` is `struct timeval` (seconds + **micro**seconds); `NULL` = forever. |
| `FD_ZERO/SET/CLR/ISSET` | `void FD_ZERO(fd_set *set); void FD_SET(int fd, fd_set *set); void FD_CLR(int fd, fd_set *set); int FD_ISSET(int fd, fd_set *set);` | Macros for manipulating an `fd_set` used with `select()`. |
| `fcntl()` | `int fcntl(int s, int cmd, long arg);` | General-purpose fd control; the socket-relevant use is `fcntl(sockfd, F_SETFL, O_NONBLOCK)` to make a socket non-blocking (failed reads then return `-1`/`EWOULDBLOCK` or `EAGAIN` instead of sleeping). |

## Address lookup & info

| Function | Signature | Returns |
|---|---|---|
| `gethostname()` | `int gethostname(char *name, size_t len);` | `0`/`-1` (+ `errno`). Gets *this* machine's own hostname. |
| `getpeername()` | `int getpeername(int sockfd, struct sockaddr *addr, int *addrlen);` | `0`/`-1` (+ `errno`). Who's on the other end of a connected stream socket. |
| `getnameinfo()` | `int getnameinfo(const struct sockaddr *sa, socklen_t salen, char *host, size_t hostlen, char *serv, size_t servlen, int flags);` | `0`/nonzero (like `getaddrinfo()`). The *reverse* of `getaddrinfo()`: given an address, get back a hostname/service name. |
| `gethostbyname()` | `struct hostent *gethostbyname(const char *name);` | **Deprecated** — IPv4-only, no reentrancy guarantees. Use `getaddrinfo()` instead in new code; shown here only because you'll see it in older code (e.g. `broadcaster.c` in Chapter 7). |
| `gethostbyaddr()` | `struct hostent *gethostbyaddr(const char *addr, int len, int type);` | Also legacy — prefer `getnameinfo()`. |

## Options, errors, and conversions

| Function | Signature | Returns |
|---|---|---|
| `setsockopt()` | `int setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen);` | `0`/`-1`. The Swiss-army-knife for socket tuning — you've already used it for `SO_REUSEADDR` and `SO_BROADCAST`. `level` is almost always `SOL_SOCKET` for the options in this guide. |
| `getsockopt()` | `int getsockopt(int s, int level, int optname, void *optval, socklen_t *optlen);` | `0`/`-1`. The read-back counterpart — e.g. reading `SO_ERROR` to check the outcome of a non-blocking `connect()` (Chapter 8). |
| `perror()` | `void perror(const char *s);` | Nothing — prints `s`, a colon, and the current `errno`'s message, to `stderr`. What every example in these notes calls after a failed syscall. |
| `strerror()` | `char *strerror(int errnum);` | The message string for a given `errno` value, without printing it — use when you want the text for your own formatting instead of `perror()`'s fixed layout. |
| `htons()`/`htonl()` | `uint16_t htons(uint16_t hostshort); uint32_t htonl(uint32_t hostlong);` | The value, byte-swapped to network order if needed (no-op on big-endian machines). |
| `ntohs()`/`ntohl()` | `uint16_t ntohs(uint16_t netshort); uint32_t ntohl(uint32_t netlong);` | The reverse — network order back to host order. |
| `inet_pton()` | `int inet_pton(int af, const char *src, void *dst);` | `1` on success, `0` if the string isn't a valid address, `-1` on a real error. Text → binary. Works for both `AF_INET`/`AF_INET6`. |
| `inet_ntop()` | `const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);` | `dst` on success, `NULL` on error. Binary → text. |
| `inet_addr()` / `inet_aton()` / `inet_ntoa()` | `in_addr_t inet_addr(const char *cp); int inet_aton(const char *cp, struct in_addr *inp); char *inet_ntoa(struct in_addr in);` | **All three legacy, IPv4-only** — use `inet_pton()`/`inet_ntop()` in new code instead. Shown here because `broadcaster.c` (Chapter 7) uses `inet_ntoa()`. |
| `errno` | (global variable, `#include <errno.h>`) | Set by most functions above on failure (the `-1`-returning ones — remember `getaddrinfo()` is the odd one out and uses its own return code instead). Read it immediately after a failed call, before calling anything else that might overwrite it. |

## The address structs, side by side

You met all of these in full in Chapter 2 — here they are once more, together, for a fast side-by-side scan:

```c
struct addrinfo {
    int ai_flags, ai_family, ai_socktype, ai_protocol;
    size_t ai_addrlen;
    struct sockaddr *ai_addr;
    char *ai_canonname;
    struct addrinfo *ai_next;
};

struct sockaddr {              // generic — cast to/from the specific types below
    unsigned short sa_family;
    char sa_data[14];
};

struct sockaddr_in {           // IPv4
    short sin_family;          // AF_INET
    unsigned short sin_port;   // htons()'d
    struct in_addr sin_addr;   // { uint32_t s_addr; }
    unsigned char sin_zero[8]; // memset() to 0
};

struct sockaddr_in6 {          // IPv6
    u_int16_t sin6_family;     // AF_INET6
    u_int16_t sin6_port;
    u_int32_t sin6_flowinfo;
    struct in6_addr sin6_addr; // { unsigned char s6_addr[16]; }
    u_int32_t sin6_scope_id;
};

struct sockaddr_storage {      // big enough for either — check ss_family, then cast
    sa_family_t ss_family;
    /* ...padding... */
};
```

---

## Quick recap (for revision)

- **`getaddrinfo()`/`getnameinfo()` return `0`/error-code + a `*_strerror()` function** — every other function on this page returns `-1` + sets the global `errno`, checked with `perror()` or `strerror()`.
- **`send()`/`sendto()` return bytes actually sent** (possibly less than asked) — never assume it equals your input length without checking.
- **`recv()`/`recvfrom()` return `0` specifically for "peer closed the connection"** — distinct from `-1` (real error).
- **`accept()` returns a brand-new fd**, separate from the listening socket, which keeps listening.
- Anything with `gethostbyname`/`gethostbyaddr`/`inet_addr`/`inet_aton`/`inet_ntoa` in the name is **legacy, IPv4-only** — prefer the `getaddrinfo()`/`inet_pton()`/`inet_ntop()`/`getnameinfo()` family in new code.
- `select()`'s first argument is the **highest fd + 1**, not a count of descriptors you're watching — a very common off-by-something bug.

Next: `10_FINAL_REVISION_CHEATSHEET.md` — the entire guide compressed onto a couple of pages, plus a self-test.