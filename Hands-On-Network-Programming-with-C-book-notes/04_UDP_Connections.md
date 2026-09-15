# 4. Establishing UDP Connections
### *(Covers the book's Chapter 4 — "Establishing UDP Connections")*

Same underlying idea as Beej's Guide's UDP chapter — no `listen()`/`accept()`, every packet self-contained — but this book adds one genuinely useful technique that's easy to miss: **`connect()`-ing a UDP socket.**

## 4.1 The absolute minimum: one `sendto()`, one `recvfrom()`

Before building anything reusable, the book shows the smallest possible UDP exchange — a receiver that waits for exactly one packet, and a sender that fires exactly one packet at a hardcoded address:

**Receiver (`udp_recvfrom.c`):**
```c
#include "chap04.h"

int main() {

    printf("Configuring local address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
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

    struct sockaddr_storage client_address;
    socklen_t client_len = sizeof(client_address);
    char read[1024];
    int bytes_received = recvfrom(socket_listen,
            read, 1024, 0,
            (struct sockaddr*) &client_address, &client_len);

    printf("Received (%d bytes): %.*s\n",
            bytes_received, bytes_received, read);

    printf("Remote address is: ");
    char address_buffer[100];
    char service_buffer[100];
    getnameinfo(((struct sockaddr*)&client_address),
            client_len,
            address_buffer, sizeof(address_buffer),
            service_buffer, sizeof(service_buffer),
            NI_NUMERICHOST | NI_NUMERICSERV);
    printf("%s %s\n", address_buffer, service_buffer);

    CLOSESOCKET(socket_listen);
    printf("Finished.\n");
    return 0;
}
```

**Sender (`udp_sendto.c`):**
```c
#include "chap04.h"

int main() {

    printf("Configuring remote address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;
    struct addrinfo *peer_address;
    if (getaddrinfo("127.0.0.1", "8080", &hints, &peer_address)) {
        fprintf(stderr, "getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    printf("Remote address is: ");
    char address_buffer[100];
    char service_buffer[100];
    getnameinfo(peer_address->ai_addr, peer_address->ai_addrlen,
            address_buffer, sizeof(address_buffer),
            service_buffer, sizeof(service_buffer),
            NI_NUMERICHOST  | NI_NUMERICSERV);
    printf("%s %s\n", address_buffer, service_buffer);

    printf("Creating socket...\n");
    SOCKET socket_peer;
    socket_peer = socket(peer_address->ai_family,
            peer_address->ai_socktype, peer_address->ai_protocol);
    if (!ISVALIDSOCKET(socket_peer)) {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    const char *message = "Hello World";
    printf("Sending: %s\n", message);
    int bytes_sent = sendto(socket_peer,
            message, strlen(message), 0,
            peer_address->ai_addr, peer_address->ai_addrlen);
    printf("Sent %d bytes.\n", bytes_sent);

    freeaddrinfo(peer_address);
    CLOSESOCKET(socket_peer);
    printf("Finished.\n");
    return 0;
}
```

Notice the sender never calls `bind()` at all — exactly the same "clients don't care about their own local port" rule from every earlier TCP example, just as true for UDP.

**Real output:**
```
$ gcc -Wall -o udp_recvfrom udp_recvfrom.c
$ gcc -Wall -o udp_sendto udp_sendto.c
$ ./udp_recvfrom &
Configuring local address...
Creating socket...
Binding socket to local address...

$ ./udp_sendto
Configuring remote address...
Remote address is: 127.0.0.1 8080
Creating socket...
Sending: Hello World
Sent 11 bytes.
Finished.

# back in the receiver's output:
Received (11 bytes): Hello World
Remote address is: 127.0.0.1 43870
Finished.
```

Worth noticing: the receiver reports the sender's **ephemeral port** (`43870` here — yours will differ every run) — the OS picked that automatically when `udp_sendto` created its socket, exactly like it would for an unbound TCP client socket. There was never a `bind()` call on the sending side; the port still had to come from *somewhere*, so the kernel assigned one on the spot.

## 4.2 A reusable UDP "server": `connect()`-ing a datagram socket

Now for the technique worth specifically remembering. The book's UDP *client* — meant to be used repeatedly, interactively, like the TCP chat client from Chapter 3 — calls `connect()` on its `SOCK_DGRAM` socket:

```c
#include "chap04.h"

#if defined(_WIN32)
#include <conio.h>
#endif

int main(int argc, char *argv[]) {

    if (argc < 3) {
        fprintf(stderr, "usage: udp_client hostname port\n");
        return 1;
    }

    printf("Configuring remote address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;
    struct addrinfo *peer_address;
    if (getaddrinfo(argv[1], argv[2], &hints, &peer_address)) {
        fprintf(stderr, "getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    printf("Remote address is: ");
    char address_buffer[100];
    char service_buffer[100];
    getnameinfo(peer_address->ai_addr, peer_address->ai_addrlen,
            address_buffer, sizeof(address_buffer),
            service_buffer, sizeof(service_buffer),
            NI_NUMERICHOST);
    printf("%s %s\n", address_buffer, service_buffer);

    printf("Creating socket...\n");
    SOCKET socket_peer;
    socket_peer = socket(peer_address->ai_family,
            peer_address->ai_socktype, peer_address->ai_protocol);
    if (!ISVALIDSOCKET(socket_peer)) {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    printf("Connecting...\n");
    if (connect(socket_peer,
                peer_address->ai_addr, peer_address->ai_addrlen)) {
        fprintf(stderr, "connect() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }
    freeaddrinfo(peer_address);

    printf("Connected.\n");
    printf("To send data, enter text followed by enter.\n");

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

    printf("Closing socket...\n");
    CLOSESOCKET(socket_peer);
    printf("Finished.\n");
    return 0;
}
```

This is — deliberately — almost character-for-character identical to `tcp_client.c` from the last chapter, with two changes: `hints.ai_socktype = SOCK_DGRAM;` instead of `SOCK_STREAM`, and that's genuinely almost it. The client still calls `connect()`, and afterward uses plain `send()`/`recv()` — no address arguments needed on every call, because `connect()` told the kernel "always send to this one address, and only accept datagrams that actually came from it." Under the hood, `connect()` on a UDP socket does **not** perform any real handshake over the network — UDP has no handshake to perform — it's purely a *local* bookkeeping operation inside your own machine's kernel. This is the exact same "you're allowed to `connect()` a datagram socket" fact from Beej's Guide's Chapter 3, now shown as the backbone of an entire reusable client, rather than just a footnote.

**Why bother, when `sendto()`/`recvfrom()` would also work?** Convenience and safety: once connected, this socket will silently discard any datagram that didn't come from the connected peer (a small, free layer of protection against spoofed or misdirected packets from *other* senders), and the code gets to reuse the exact same `send()`/`recv()` calls — and, not coincidentally, the exact same `select()`-based "watch both the socket and my own keyboard" loop — you already saw in the TCP client.

## 4.3 The server side, with and without `select()`

The corresponding server echoes back whatever it receives, upper-cased — you've seen this exact "toy service" once already in Chapter 3's TCP version. There are two versions here: a bare-minimum one that just blocks on `recvfrom()` in a plain loop...

```c
while(1) {
    struct sockaddr_storage client_address;
    socklen_t client_len = sizeof(client_address);

    char read[1024];
    int bytes_received = recvfrom(socket_listen, read, 1024, 0,
            (struct sockaddr *)&client_address, &client_len);
    if (bytes_received < 1) {
        fprintf(stderr, "connection closed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    int j;
    for (j = 0; j < bytes_received; ++j)
        read[j] = toupper(read[j]);
    sendto(socket_listen, read, bytes_received, 0,
            (struct sockaddr*)&client_address, client_len);
}
```

...and one that wraps the identical logic in a `select()` call first, even though there's only ever one socket in the set:

```c
fd_set master;
FD_ZERO(&master);
FD_SET(socket_listen, &master);
SOCKET max_socket = socket_listen;

while(1) {
    fd_set reads;
    reads = master;
    if (select(max_socket+1, &reads, 0, 0, 0) < 0) {
        fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    if (FD_ISSET(socket_listen, &reads)) {
        /* ...identical recvfrom/toupper/sendto as above... */
    }
}
```

With only one socket to watch, `select()` here doesn't buy you anything a plain blocking `recvfrom()` wouldn't already give you — its real value only shows up once you're watching *several* sockets at once (multiple UDP sockets bound to different ports, or a mix of UDP and TCP sockets in the same process). The book shows it this way specifically so the *shape* of the code matches every other multiplexed example in the book — worth recognizing as "the same select() skeleton, applied even when it's not strictly required yet" rather than a new concept.

**Real interactive session, both pieces run live:**
```
$ gcc -Wall -o udp_serve_toupper udp_serve_toupper.c
$ gcc -Wall -o udp_client udp_client.c
$ ./udp_serve_toupper &
Configuring local address...
Creating socket...
Binding socket to local address...
Waiting for connections...

$ ./udp_client localhost 8080
Connected.
To send data, enter text followed by enter.
Sending: make me loud
Sent 13 bytes.
Received (13 bytes): MAKE ME LOUD
```

---

## Quick recap (for revision)

- The minimum viable UDP exchange is just one `socket()` + `bind()` + `recvfrom()` on one side, and one `socket()` + `sendto()` on the other — no `listen()`/`accept()`/`connect()` required at all.
- **`connect()` on a `SOCK_DGRAM` socket is legal and genuinely useful**: it fixes one peer address locally (no network handshake happens), letting you use plain `send()`/`recv()` afterward, and it makes the socket silently ignore datagrams from anyone else.
- Because of that, a UDP client built this way can reuse almost identical code to a TCP client — including the exact same "multiplex my socket and my own keyboard with `select()`" pattern from Chapter 3.
- `select()` on a single-socket set doesn't add capability over a plain blocking call — its payoff is purely about *scaling to more sockets*, and this chapter shows it early mostly for consistency with the rest of the book's style.

Next: `05_DNS_and_Hostname_Resolution.md` — what `getaddrinfo()` is actually doing under the hood, down to the raw DNS wire format.