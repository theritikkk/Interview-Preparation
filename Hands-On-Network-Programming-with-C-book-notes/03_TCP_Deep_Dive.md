# 3. TCP In-Depth: Multiplexing and a Real Chat Room
### *(Covers the book's Chapter 3 — "An In-Depth Overview of TCP Connections")*

Chapter 2 built a server that handles exactly one client, once, then quits. This chapter fixes that — with **three different techniques**, applied to the *same* simple "uppercase echo" service, so you can see them side by side before the chapter's centerpiece: a real multi-client chat room.

## 3.1 Three ways to handle more than one client

| Technique | How it works | Trade-off |
|---|---|---|
| Non-blocking polling | Set every socket non-blocking; loop, trying each one, doing nothing if it's not ready | Simple to picture, but wastes CPU busy-looping — same problem you'd hit implementing this by hand in any language |
| `fork()`-per-client | Accept a connection, `fork()` a child process to own it entirely, parent loops back to `accept()` | Simple *logic* per client (each child just blocks freely, no juggling) — Unix-only, and each client costs a whole OS process |
| `select()` | One process, one loop, ask the kernel which sockets (out of *all* of them, listener included) are ready | Scales much better than forking, works identically on Windows and Unix, but the code juggles all clients' state together in one place |

This book demonstrates the same toy service — echo back whatever text a client sends, but upper-cased — built both the `select()` way and the `fork()` way, so you can compare them directly rather than taking either one on faith.

### The `select()` version

```c
#include "chap03.h"
#include <ctype.h>

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

    fd_set master;
    FD_ZERO(&master);
    FD_SET(socket_listen, &master);
    SOCKET max_socket = socket_listen;

    printf("Waiting for connections...\n");

    while(1) {
        fd_set reads;
        reads = master;
        if (select(max_socket+1, &reads, 0, 0, 0) < 0) {
            fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
            return 1;
        }

        SOCKET i;
        for(i = 1; i <= max_socket; ++i) {
            if (FD_ISSET(i, &reads)) {

                if (i == socket_listen) {
                    struct sockaddr_storage client_address;
                    socklen_t client_len = sizeof(client_address);
                    SOCKET socket_client = accept(socket_listen,
                            (struct sockaddr*) &client_address,
                            &client_len);
                    if (!ISVALIDSOCKET(socket_client)) {
                        fprintf(stderr, "accept() failed. (%d)\n",
                                GETSOCKETERRNO());
                        return 1;
                    }

                    FD_SET(socket_client, &master);
                    if (socket_client > max_socket)
                        max_socket = socket_client;

                    char address_buffer[100];
                    getnameinfo((struct sockaddr*)&client_address,
                            client_len,
                            address_buffer, sizeof(address_buffer), 0, 0,
                            NI_NUMERICHOST);
                    printf("New connection from %s\n", address_buffer);

                } else {
                    char read[1024];
                    int bytes_received = recv(i, read, 1024, 0);
                    if (bytes_received < 1) {
                        FD_CLR(i, &master);
                        CLOSESOCKET(i);
                        continue;
                    }

                    int j;
                    for (j = 0; j < bytes_received; ++j)
                        read[j] = toupper(read[j]);
                    send(i, read, bytes_received, 0);
                }
            }
        }
    }
}
```

The core trick, worth stating explicitly: **the listening socket lives inside the same `fd_set` as every client socket.** `select()` doesn't distinguish "new connection waiting" from "existing client has data" — both just look like "this file descriptor is readable." The code tells them apart itself, by checking `if (i == socket_listen)`. This is the exact same trick you'd have seen in `pollserver.c` from Beej's Guide, just expressed with `select()`'s `fd_set`/`FD_ISSET` instead of `poll()`'s array of structs — a good sign that this is a fundamental pattern of the underlying OS API, not a stylistic choice of any one book.

**Real, live output:**
```
$ gcc -Wall -o tcp_serve_toupper tcp_serve_toupper.c
$ ./tcp_serve_toupper &
Configuring local address...
Creating socket...
Binding socket to local address...
Listening...
Waiting for connections...

$ printf "hello world\n" | nc -q1 localhost 8080
HELLO WORLD
```
And in the server's own log:
```
New connection from 127.0.0.1
```

### The `fork()` version — same service, opposite architecture

```c
#if defined(_WIN32)
#error This program does not support Windows.
#endif

#include "chap03.h"
#include <ctype.h>
#include <stdlib.h>

int main() {

    /* ...identical getaddrinfo/socket/bind/listen setup as above... */

    while(1) {
        struct sockaddr_storage client_address;
        socklen_t client_len = sizeof(client_address);
        SOCKET socket_client = accept(socket_listen,
                (struct sockaddr*) &client_address, &client_len);
        if (!ISVALIDSOCKET(socket_client)) {
            fprintf(stderr, "accept() failed. (%d)\n", GETSOCKETERRNO());
            return 1;
        }

        char address_buffer[100];
        getnameinfo((struct sockaddr*)&client_address,
                client_len, address_buffer, sizeof(address_buffer), 0, 0,
                NI_NUMERICHOST);
        printf("New connection from %s\n", address_buffer);

        int pid = fork();

        if (pid == 0) { //child process
            CLOSESOCKET(socket_listen);
            while(1) {
                char read[1024];
                int bytes_received = recv(socket_client, read, 1024, 0);
                if (bytes_received < 1) {
                    CLOSESOCKET(socket_client);
                    exit(0);
                }

                int j;
                for (j = 0; j < bytes_received; ++j)
                    read[j] = toupper(read[j]);
                send(socket_client, read, bytes_received, 0);
            }
        }

        CLOSESOCKET(socket_client);
    }
}
```

Notice the **`#error` directive right at the top** — the author deliberately makes this fail to *compile at all* on Windows, rather than let you discover at runtime that `fork()` doesn't exist there. That's a genuinely good defensive habit for any code with a hard platform dependency: fail loud, at compile time, with an explanatory message, rather than producing a confusing linker error or runtime crash later.

The child process here can afford to just call `recv()` in a plain, blocking loop — no `select()`, no juggling other clients — because the OS gives it its own private copy of everything, including its own instruction pointer running independently of the parent. That simplicity is exactly what you're trading the cost of a whole new process for.

**Real output, functionally identical from the outside:**
```
$ gcc -Wall -o tcp_serve_toupper_fork tcp_serve_toupper_fork.c
$ ./tcp_serve_toupper_fork &
$ printf "fork test\n" | nc -q1 localhost 8080
FORK TEST
```
```
New connection from 127.0.0.1
```

Same input, same output, same "New connection" log line — this is the point. A client talking to either server can't tell which one it's connected to. The difference is entirely about how the *server* scales and what resources it uses, invisible from the client's side.

## 3.2 The TCP client — and multiplexing your *own* input

Here's a genuinely useful trick this book introduces early: a client that needs to watch **two** file descriptors at once — the socket (for incoming data) *and* standard input (so the user can type a new message at any time) — using `select()` with a short timeout, checked repeatedly in a loop:

```c
while(1) {

    fd_set reads;
    FD_ZERO(&reads);
    FD_SET(socket_peer, &reads);
#if !defined(_WIN32)
    FD_SET(0, &reads);
#endif

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;

    if (select(socket_peer+1, &reads, 0, 0, &timeout) < 0) {
        fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    if (FD_ISSET(socket_peer, &reads)) {
        char read[4096];
        int bytes_received = recv(socket_peer, read, 4096, 0);
        if (bytes_received < 1) {
            printf("Connection closed by peer.\n");
            break;
        }
        printf("Received (%d bytes): %.*s",
                bytes_received, bytes_received, read);
    }

#if defined(_WIN32)
    if(_kbhit()) {
#else
    if(FD_ISSET(0, &reads)) {
#endif
        char read[4096];
        if (!fgets(read, 4096, stdin)) break;
        printf("Sending: %s", read);
        int bytes_sent = send(socket_peer, read, strlen(read), 0);
        printf("Sent %d bytes.\n", bytes_sent);
    }
}
```

A 100-millisecond timeout on `select()` means the loop wakes up ten times a second no matter what, checks both the socket and (on Unix — Windows uses `_kbhit()` instead, since standard input doesn't play nicely with `select()` on Windows) standard input, and reacts to whichever one actually has something. This is the same core idea as `poll()`/`select()` server-side multiplexing from before, just applied to make a *client* feel interactive rather than to serve many connections.

## 3.3 The chat room — everything combined

```c
#include "chap03.h"
#include <ctype.h>

int main() {

    /* ...identical getaddrinfo/socket/bind/listen setup... */

    fd_set master;
    FD_ZERO(&master);
    FD_SET(socket_listen, &master);
    SOCKET max_socket = socket_listen;

    printf("Waiting for connections...\n");

    while(1) {
        fd_set reads;
        reads = master;
        if (select(max_socket+1, &reads, 0, 0, 0) < 0) {
            fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
            return 1;
        }

        SOCKET i;
        for(i = 1; i <= max_socket; ++i) {
            if (FD_ISSET(i, &reads)) {

                if (i == socket_listen) {
                    struct sockaddr_storage client_address;
                    socklen_t client_len = sizeof(client_address);
                    SOCKET socket_client = accept(socket_listen,
                            (struct sockaddr*) &client_address,
                            &client_len);
                    if (!ISVALIDSOCKET(socket_client)) {
                        fprintf(stderr, "accept() failed. (%d)\n",
                                GETSOCKETERRNO());
                        return 1;
                    }

                    FD_SET(socket_client, &master);
                    if (socket_client > max_socket)
                        max_socket = socket_client;

                    char address_buffer[100];
                    getnameinfo((struct sockaddr*)&client_address,
                            client_len,
                            address_buffer, sizeof(address_buffer), 0, 0,
                            NI_NUMERICHOST);
                    printf("New connection from %s\n", address_buffer);

                } else {
                    char read[1024];
                    int bytes_received = recv(i, read, 1024, 0);
                    if (bytes_received < 1) {
                        FD_CLR(i, &master);
                        CLOSESOCKET(i);
                        continue;
                    }

                    SOCKET j;
                    for (j = 1; j <= max_socket; ++j) {
                        if (FD_ISSET(j, &master)) {
                            if (j == socket_listen || j == i)
                                continue;
                            else
                                send(j, read, bytes_received, 0);
                        }
                    }
                }
            }
        }
    }
}
```

The only real change from the `toupper` server: instead of `toupper()`-ing the received bytes and sending them straight back to the *same* client, it loops over **every other connected socket** (`j == socket_listen || j == i` is skipped — don't echo to the listener, and don't echo a message back to whoever sent it) and relays the message to all of them. This is precisely a chat room's core behavior, and it's a strikingly small amount of code once `select()`'s bookkeeping is already in place.

**Real two-client session, driven by the interactive client above:**

```
$ gcc -Wall -o tcp_serve_chat tcp_serve_chat.c
$ gcc -Wall -o tcp_client tcp_client.c
$ ./tcp_serve_chat &

# Client A:
$ ./tcp_client localhost 8080
Connected.
To send data, enter text followed by enter.
Sending: Hello from A!
Sent 14 bytes.
Received (17 bytes): Hey A, this is B

# Client B (a separate terminal):
$ ./tcp_client localhost 8080
Connected.
To send data, enter text followed by enter.
Received (14 bytes): Hello from A!
Sending: Hey A, this is B
Sent 17 bytes.
```

And the server's own log across the whole session:
```
Configuring local address...
Creating socket...
Binding socket to local address...
Listening...
Waiting for connections...
New connection from 127.0.0.1
New connection from 127.0.0.1
```

Exactly as expected: **A never sees its own message echoed back**, and B receives it in real time. This is functionally the same result as Beej's Guide's `pollserver.c`/`selectserver.c` chat rooms — a good sign that "broadcast-to-everyone-except-sender via a multiplexed socket set" really is *the* canonical pattern for this kind of small chat server, arrived at independently by different authors because it's simply the natural shape the problem takes.

## 3.4 "TCP is a stream protocol" — the one sentence to never forget

The book makes a point of stressing this, and it's worth repeating here too: **TCP has no concept of "messages."** It's a continuous stream of bytes. If you `send()` twice in a row, the receiving `recv()` might get both sends' data in one call, or split across several calls, or (rarely) even coalesced with parts of a third `send()` — there's no guarantee that your `send()`/`recv()` calls line up one-to-one. Every example in this chapter gets away without worrying about this because the messages are short enough to essentially always arrive whole in one `recv()` on `localhost` — but production code that cares about exact message boundaries needs to build that structure itself (length-prefixing a message, or using a delimiter like a newline and buffering until one appears), exactly the same lesson Beej's Guide's serialization chapter drove home with its own "Son of Data Encapsulation" section.

---

## Quick recap (for revision)

- Three ways to serve multiple clients: non-blocking polling (simple, wasteful), `fork()`-per-client (simple per-client logic, Unix-only, one process per client), `select()`/multiplexing (scales better, cross-platform, more bookkeeping in one place).
- The listening socket goes into the *same* `fd_set`/`pollfd` array as every client — "ready to read" on it means a new connection is waiting, not data.
- `select()` isn't just for servers: a client can multiplex its own socket *and* stdin, using a short timeout, to feel interactive without busy-waiting.
- A chat-room relay is "the toupper-echo server, but send to everyone except the listener and the sender" — remarkably little extra code once multiplexing is already working.
- **TCP delivers a byte stream, not discrete messages** — `send()`/`recv()` call boundaries are not preserved. Never assume one `send()` = one `recv()` in code you actually rely on.

Next: `04_UDP_Connections.md` — the same client/server pairing built on connectionless UDP instead.