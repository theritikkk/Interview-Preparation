# 2. Socket API Basics
### *(Covers the book's Chapter 2 — "Getting to Grips with Socket APIs")*

This chapter builds your very first real network program — and introduces a set of small macros you'll see reused in **every single example for the rest of the book**, because this book takes cross-platform support (Windows + Linux/macOS) seriously from page one.

## 2.1 Why the code looks the way it does: Winsock vs. Berkeley sockets

Windows and Unix-family systems (Linux, macOS, BSD) both implement "sockets," but with small, annoying differences:

| | Berkeley sockets (Unix/Linux/macOS) | Winsock (Windows) |
|---|---|---|
| Needs explicit startup? | No | Yes — `WSAStartup()` before anything else, `WSACleanup()` at the end |
| Socket type | Plain `int` | Opaque `SOCKET` type |
| Invalid socket value | `-1` | `INVALID_SOCKET` (a distinct constant, not `-1`) |
| Closing a socket | `close()` (same as any file descriptor) | `closesocket()` (a different function — sockets aren't file descriptors on Windows) |
| Getting the last error | Global `errno` | `WSAGetLastError()` |

Rather than littering every program with `#ifdef _WIN32` checks at each call site, this book defines a small set of macros **once**, at the top of every networked program, that paper over these differences:

```c
#if defined(_WIN32)
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#endif


#if defined(_WIN32)
#define ISVALIDSOCKET(s) ((s) != INVALID_SOCKET)
#define CLOSESOCKET(s) closesocket(s)
#define GETSOCKETERRNO() (WSAGetLastError())

#else
#define ISVALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s) close(s)
#define SOCKET int
#define GETSOCKETERRNO() (errno)
#endif
```

From this point on, the *rest* of the program just calls `ISVALIDSOCKET(x)`, `CLOSESOCKET(x)`, and `GETSOCKETERRNO()` and never has to think about which platform it's on again. This is a genuinely good pattern worth internalizing even outside networking: **when two platforms differ in lots of small, mechanical ways, isolate the differences into a thin macro layer once, rather than scattering `#ifdef`s through your actual logic.**

On Windows specifically, you also need one extra step before *any* socket call will work — initializing the Winsock library:

```c
#if defined(_WIN32)
    WSADATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d)) {
        fprintf(stderr, "Failed to initialize.\n");
        return 1;
    }
#endif
```

And the matching cleanup at the very end:
```c
#if defined(_WIN32)
    WSACleanup();
#endif
```

On Unix-like systems, both of these are simply no-ops (the socket API is just "always available," no setup step needed) — which is why they're wrapped in `#if defined(_WIN32)` rather than being unconditional.

**A minimal program using nothing but this setup/teardown**, to prove the pattern works before building anything real on top of it:

```c
#include <stdio.h>

int main() {
#if defined(_WIN32)
    WSADATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d)) {
        fprintf(stderr, "Failed to initialize.\n");
        return 1;
    }
#endif

    printf("Ready to use socket API.\n");

#if defined(_WIN32)
    WSACleanup();
#endif

    return 0;
}
```

**Real output** (Linux, so the Winsock branches are simply compiled out and never run):
```
$ gcc -Wall -o sock_init sock_init.c
$ ./sock_init
Ready to use socket API.
```

## 2.2 The motivating example: a "what time is it" server

The book's running example for this chapter, and a genuinely nice teaching device: start with a program that just prints the local time...

```c
#include <stdio.h>
#include <time.h>

int main() {
    time_t timer;
    time(&timer);
    printf("Local time is: %s", ctime(&timer));
    return 0;
}
```
```
$ ./time_console
Local time is: Mon Sep 14 21:14:28 2026
```

...then turn it into a **server** that any client on the network can ask for the time, over HTTP-shaped (but drastically simplified) plain text. This is the single most useful mental exercise for learning sockets: **almost any local program can become a network service by wrapping its input/output in `accept()`/`recv()`/`send()`.** The actual "business logic" (getting the time) doesn't change at all.

## 2.3 The full TCP server, function-by-function

```c
#include <stdio.h>
#include <string.h>
#include <time.h>

int main() {

    printf("Configuring local address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *bind_address;
    getaddrinfo(0, "8080", &hints, &bind_address);


    printf("Creating socket...\n");
    SOCKET socket_listen;
    socket_listen = socket(bind_address->ai_family,
            bind_address->ai_socktype, bind_address->ai_protocol);
    if (!ISVALIDSOCKET(socket_listen)) {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }


    printf("Binding socket to local address...\n");
    if (bind(socket_listen,
                bind_address->ai_addr, bind_address->ai_addrlen)) {
        fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }
    freeaddrinfo(bind_address);


    printf("Listening...\n");
    if (listen(socket_listen, 10) < 0) {
        fprintf(stderr, "listen() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }


    printf("Waiting for connection...\n");
    struct sockaddr_storage client_address;
    socklen_t client_len = sizeof(client_address);
    SOCKET socket_client = accept(socket_listen,
            (struct sockaddr*) &client_address, &client_len);
    if (!ISVALIDSOCKET(socket_client)) {
        fprintf(stderr, "accept() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }


    printf("Client is connected... ");
    char address_buffer[100];
    getnameinfo((struct sockaddr*)&client_address,
            client_len, address_buffer, sizeof(address_buffer), 0, 0,
            NI_NUMERICHOST);
    printf("%s\n", address_buffer);


    printf("Reading request...\n");
    char request[1024];
    int bytes_received = recv(socket_client, request, 1024, 0);
    printf("Received %d bytes.\n", bytes_received);


    printf("Sending response...\n");
    const char *response =
        "HTTP/1.1 200 OK\r\n"
        "Connection: close\r\n"
        "Content-Type: text/plain\r\n\r\n"
        "Local time is: ";
    int bytes_sent = send(socket_client, response, strlen(response), 0);
    printf("Sent %d of %d bytes.\n", bytes_sent, (int)strlen(response));

    time_t timer;
    time(&timer);
    char *time_msg = ctime(&timer);
    bytes_sent = send(socket_client, time_msg, strlen(time_msg), 0);
    printf("Sent %d of %d bytes.\n", bytes_sent, (int)strlen(time_msg));


    printf("Closing connection...\n");
    CLOSESOCKET(socket_client);

    printf("Closing listening socket...\n");
    CLOSESOCKET(socket_listen);

    printf("Finished.\n");

    return 0;
}
```

*(Windows setup/teardown and the cross-platform macros from §2.1 sit at the top of this file, exactly as shown above — omitted here for readability since you've just seen them in full.)*

Walking through the shape of this: `getaddrinfo()` → `socket()` → `bind()` → `listen()` → `accept()` → `recv()`/`send()` → `CLOSESOCKET()` — if this sequence looks familiar, that's because it's the *exact same* universal TCP server pattern every C sockets tutorial teaches (it's dictated by the underlying OS API, not by any particular book). What's specific to *this* program is deliberately minimal: it handles **exactly one client, once, then exits** — no loop, no `fork()`, no multiplexing yet. That's intentional; those come in Chapter 3. This version exists purely to get the full connect→respond→close cycle working end to end, once, before adding any complexity.

Notice the response is a hand-typed, minimal HTTP response (`"HTTP/1.1 200 OK\r\n..."`) — just enough for a real web browser or `curl` to accept it as valid, without any actual HTTP-parsing logic on the receiving end of the request. This is a preview of Chapters 6–7, where a *real* (if still simple) HTTP implementation gets built.

## 2.4 Running it for real

```
$ gcc -Wall -o time_server time_server.c
$ ./time_server &
Configuring local address...
Creating socket...
Binding socket to local address...
Listening...
Waiting for connection...
```
(It blocks on `accept()` here — exactly as expected.)

```
$ curl -v http://localhost:8080/
*   Trying 127.0.0.1:8080...
* Connected to localhost (127.0.0.1) port 8080
> GET / HTTP/1.1
> Host: localhost:8080
> User-Agent: curl/8.5.0
> Accept: */*
>
< HTTP/1.1 200 OK
< Connection: close
< Content-Type: text/plain
<
Local time is: Mon Sep 14 21:12:59 2026
* Closing connection
```

**And back in the server's terminal**, the full sequence printed in real time as `curl` connected:
```
Client is connected... 127.0.0.1
Reading request...
Received 77 bytes.
Sending response...
Sent 79 of 79 bytes.
Sent 25 of 25 bytes.
Closing connection...
Closing listening socket...
Finished.
```

A real web browser pointed at `http://localhost:8080/` would show exactly the same plain-text response — this genuinely is a (extremely minimal, single-use) working HTTP server, built from nothing but the raw socket calls you've just read line-by-line.

## 2.5 IPv6 and dual-stack — and a real environment limitation

The book includes two variants worth knowing about even though I can't fully demo them live here:

- **`time_server_ipv6.c`** is identical to the server above with exactly one line changed: `hints.ai_family = AF_INET;` becomes `hints.ai_family = AF_INET6;`. That's the entire adaptation needed to make this specific server IPv6-only instead of IPv4-only — a nice, concrete illustration of just how symmetric the IPv4/IPv6 socket APIs are once you're going through `getaddrinfo()`.
- **`time_server_dual.c`** goes further: bind a single IPv6 socket, but explicitly clear the `IPV6_V6ONLY` socket option, which makes that one socket accept **both** IPv4 and IPv6 connections simultaneously (IPv4 connections show up as IPv4-mapped IPv6 addresses, like `::ffff:127.0.0.1`):
  ```c
  int option = 0;
  if (setsockopt(socket_listen, IPPROTO_IPV6, IPV6_V6ONLY, (void*)&option, sizeof(option))) {
      fprintf(stderr, "setsockopt() failed. (%d)\n", GETSOCKETERRNO());
      return 1;
  }
  ```

**Real attempt, real result, in this sandbox:**
```
$ gcc -Wall -o time_server_dual time_server_dual.c
$ ./time_server_dual
Configuring local address...
Creating socket...
socket() failed. (97)
```
Errno `97` is `EAFNOSUPPORT` — "address family not supported." This sandbox container has no IPv6 stack at all (confirmed the same way back when testing Beej's Guide's UDP examples), so any `socket(AF_INET6, ...)` call fails immediately, regardless of what the program does afterward. **On a normal machine with working IPv6** (which is to say, almost any real computer today), both of these programs work exactly like the IPv4 version — `time_server_ipv6.c` would simply be reachable via `curl -6 http://[::1]:8080/` instead of `http://127.0.0.1:8080/`, and `time_server_dual.c` would successfully answer *either* form.

---

## Quick recap (for revision)

- This book's cross-platform macros (`SOCKET`, `ISVALIDSOCKET`, `CLOSESOCKET`, `GETSOCKETERRNO`) exist to isolate Windows/Winsock-vs-Unix/Berkeley differences into one place — you'll see them at the top of nearly every remaining example.
- Windows needs an explicit `WSAStartup()`/`WSACleanup()` pair around all socket use; Unix needs neither.
- The core TCP server sequence — `getaddrinfo → socket → bind → listen → accept → recv/send → close` — is identical to what you'd write with any sockets API in any book; this chapter's contribution is showing it end-to-end, once, cleanly, with real output at every step.
- One line (`AF_INET` → `AF_INET6`) is the entire difference between an IPv4-only and IPv6-only version of this exact server; dual-stack support needs exactly one additional `setsockopt(IPV6_V6ONLY)` call.
- `EAFNOSUPPORT` (errno 97) at `socket()` means "this environment doesn't support the address family you asked for" — in practice today, almost always a missing-IPv6 sandbox/container issue, not a code bug.

Next: `03_TCP_Deep_Dive.md` — turning this one-shot server into a real multi-client TCP chat room using `select()`.