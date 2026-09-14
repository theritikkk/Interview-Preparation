# 3. Core System Calls
### *(Covers the guide's Chapter 5 — the functions everything else is built on)*

This chapter is the payoff for the last two. You now know what a socket is and what the address structs look like — here's every function call you'll actually type, roughly in the order you'll call them.

## 3.1 `getaddrinfo()` — Prepare to launch

Before you can create a socket, you need to know *what kind* of socket to create — and if you're connecting somewhere, *where*. `getaddrinfo()` does both jobs: it looks up a hostname (doing the DNS lookup if needed) and fills in all the address-family/socket-type details for you, replacing the old, IPv4-only `gethostbyname()`.

```c
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

int getaddrinfo(const char *node,     // hostname or IP address ("www.example.com", "10.0.0.1")
                 const char *service,  // port number or service name ("3490", "http")
                 const struct addrinfo *hints, // your requirements
                 struct addrinfo **res);       // OUTPUT: linked list of results
```

You fill in a `struct addrinfo hints` with what you want (family, socket type, flags), and the function hands back a **linked list** (`res`) of matching results — because a hostname might resolve to several addresses, and you get to pick one (in practice, the first one that works is usually fine, and that's the pattern every example program below uses).

**Server-side pattern** — "give me an address I can listen on, for whichever IP version is available, on port 3490":
```c
struct addrinfo hints, *servinfo;
memset(&hints, 0, sizeof hints);
hints.ai_family   = AF_UNSPEC;    // IPv4 or IPv6, don't care
hints.ai_socktype = SOCK_STREAM;
hints.ai_flags    = AI_PASSIVE;   // fill in my own IP for me — this is what makes node = NULL work
getaddrinfo(NULL, "3490", &hints, &servinfo);
```

**Client-side pattern** — "give me the address of this specific remote host":
```c
struct addrinfo hints, *servinfo;
memset(&hints, 0, sizeof hints);
hints.ai_family   = AF_UNSPEC;
hints.ai_socktype = SOCK_STREAM;
getaddrinfo("www.example.com", "3490", &hints, &servinfo);
```

The only structural difference: servers pass `node = NULL` with the `AI_PASSIVE` flag (meaning "bind to *my* address"); clients pass an actual hostname/IP with no `AI_PASSIVE`. **Always call `freeaddrinfo(servinfo)` once you're done with the results** — it's a linked list the library allocated for you, and it needs to be released like any other heap memory.

`getaddrinfo()` returns `0` on success or a nonzero error code on failure — note this is *not* the usual "`-1` and check `errno`" pattern; instead, pass the return value to `gai_strerror()` to get a human-readable message:
```c
int rv;
if ((rv = getaddrinfo(NULL, "3490", &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
}
```

### Live demo: `showip.c`

Here's a small, complete, self-contained program straight from the book that does nothing but call `getaddrinfo()` and print every address it gets back — a perfect way to see the function in isolation before it gets buried inside a bigger server or client.

```c
/*
** showip.c
**
** show IP addresses for a host given on the command line
*/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

int main(int argc, char *argv[])
{
    struct addrinfo hints, *res, *p;
    int status;
    char ipstr[INET6_ADDRSTRLEN];

    if (argc != 2) {
        fprintf(stderr, "usage: showip hostname\n");
        return 1;
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // Either IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(argv[1], NULL, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 2;
    }

    printf("IP addresses for %s:\n\n", argv[1]);

    for (p = res; p != NULL; p = p->ai_next) {
        void *addr;
        char *ipver;
        struct sockaddr_in *ipv4;
        struct sockaddr_in6 *ipv6;

        // get the pointer to the address itself,
        // different fields in IPv4 and IPv6:
        if (p->ai_family == AF_INET) { // IPv4
            ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
            ipver = "IPv4";
        } else { // IPv6
            ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6->sin6_addr);
            ipver = "IPv6";
        }

        // convert the IP to a string and print it:
        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        printf(" %s: %s\n", ipver, ipstr);
    }

    freeaddrinfo(res); // free the linked list

    return 0;
}
```

**Compiling and running it — real output, captured live:**

```
$ gcc -Wall -Wextra -o showip showip.c
$ ./showip localhost
IP addresses for localhost:

 IPv4: 127.0.0.1

$ ./showip 127.0.0.1
IP addresses for 127.0.0.1:

 IPv4: 127.0.0.1

$ ./showip google.com
IP addresses for google.com:

 IPv4: 173.194.210.139
 IPv4: 173.194.210.113
 IPv4: 173.194.210.102
 IPv4: 173.194.210.101
 IPv4: 173.194.210.100
 IPv4: 173.194.210.138
 IPv6: 2607:f8b0:400c:c0f::66
 IPv6: 2607:f8b0:400c:c0f::8a
 IPv6: 2607:f8b0:400c:c0f::8b
 IPv6: 2607:f8b0:400c:c0f::71

$ ./showip
usage: showip hostname
```

Two things worth noticing in that real run: first, `google.com` resolves to **multiple** addresses of **both** families — this is exactly why `res` is a linked list and why `AF_UNSPEC` matters; a program hard-coded to IPv4-only would still work here (since IPv4 addresses exist), but would be silently leaving IPv6 connectivity on the table. Second, this is the exact loop-and-cast pattern (`ai_family == AF_INET ? cast-to-sockaddr_in : cast-to-sockaddr_in6`) you'll reuse in almost every program in this guide — it's worth having memorized.

## 3.2 `socket()` — Get the file descriptor

```c
#include <sys/types.h>
#include <sys/socket.h>

int socket(int domain, int type, int protocol);
```

This allocates a socket and hands you back a file descriptor (or `-1` on error). In principle you could hardcode the three arguments yourself (`domain = PF_INET`, `type = SOCK_STREAM`, `protocol = 0` for "pick the right one automatically") — but in practice, you almost always feed it straight from a `getaddrinfo()` result, which is both less error-prone and automatically IPv4/IPv6-correct:

```c
sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
```

A small historical note that trips people up when reading old code: `PF_INET` (used in `socket()`'s domain argument) and `AF_INET` (used in `sockaddr_in.sin_family`) are, on every real system, literally the same numeric value — a "protocol family" vs. "address family" distinction that never actually needed two separate constants in practice. You'll see both used interchangeably; it doesn't matter which you use in `socket()`.

By itself, a freshly-created socket doesn't do anything yet — it needs to be bound, connected, or listened on before it's useful, which is exactly what the rest of this chapter covers.

## 3.3 `bind()` — What port am I on?

```c
int bind(int sockfd, struct sockaddr *my_addr, int addrlen);
```

`bind()` associates a socket with a specific local port (and, optionally, a specific local IP, if the machine has more than one). **Servers need this** — clients connecting to you need to know which port to knock on. **Clients almost never call this explicitly** — you only care about the *remote* port you're connecting to, and the kernel will silently pick you an available local port when you `connect()`.

```c
struct addrinfo hints, *res;
memset(&hints, 0, sizeof hints);
hints.ai_family = AF_UNSPEC;
hints.ai_socktype = SOCK_STREAM;
hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

getaddrinfo(NULL, "3490", &hints, &res);
sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
bind(sockfd, res->ai_addr, res->ai_addrlen);
```

**The gotcha you *will* hit eventually:** stop a server, then immediately try to restart it, and `bind()` fails with `"Address already in use"`. This happens because the kernel keeps a recently-used socket in a brief cooldown state even after your program exits. The fix, which you'll see at the top of literally every server example in this guide, is to set the `SO_REUSEADDR` socket option right after creating the socket and before binding it:

```c
int yes = 1;
setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
```

Also remember: **ports below 1024 require root/administrator privileges** to bind to. If your server mysteriously fails to bind with a permissions error, check you didn't accidentally pick a low port number.

## 3.4 `connect()` — Hey, you!

```c
int connect(int sockfd, struct sockaddr *serv_addr, int addrlen);
```

This is how a **client** actually initiates a connection to a server. Feed it a socket and the server's address (again, straight from `getaddrinfo()`):

```c
struct addrinfo hints, *res;
memset(&hints, 0, sizeof hints);
hints.ai_family = AF_UNSPEC;
hints.ai_socktype = SOCK_STREAM;

getaddrinfo("www.example.com", "3490", &hints, &res);
sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
connect(sockfd, res->ai_addr, res->ai_addrlen);
```

Returns `-1` and sets `errno` on failure (e.g. nothing listening on the other end — you'll see exactly this error message, `"Connection refused"`, in the next chapter when a client tries to reach a server that isn't running). Note there's no `bind()` in this snippet — the client doesn't care what its own local port is, so it skips straight from `socket()` to `connect()`.

## 3.5 `listen()` — Will somebody please call me?

```c
int listen(int sockfd, int backlog);
```

Once a **server** has `bind()`'d a socket to a port, `listen()` tells the kernel "start queuing up incoming connection attempts on this socket." `backlog` is how many pending (not-yet-`accept()`-ed) connections are allowed to wait in that queue at once — most systems cap this around 20 regardless of what you ask for; 5–10 is a totally normal value for a small demo server.

The full server-side sequence up to this point:
```
getaddrinfo()  →  socket()  →  bind()  →  listen()  →  [accept() goes here]
```

## 3.6 `accept()` — "Thank you for calling port 3490"

This is the one beginners find genuinely surprising the first time: **`accept()` hands you back a brand-new socket descriptor**, separate from the one you were listening on.

```c
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
```

Here's the mental model: `sockfd` (the listening socket) is like a receptionist's phone that never stops ringing — it just keeps taking new calls. Each time you call `accept()`, it plucks the next waiting connection off the queue and gives you a **separate, private line** (`new_fd`) dedicated to that one caller. The original listening socket keeps doing its job (accepting still more connections); the new socket is what you actually `send()`/`recv()` on for that specific client. This is exactly how a server can juggle many simultaneous clients on one well-known port.

`addr` is typically a `struct sockaddr_storage` (from Chapter 2) — since at the point you call `accept()`, you don't yet know whether the incoming connection is IPv4 or IPv6, and `sockaddr_storage` is guaranteed big enough for either. `addrlen` must be initialized to `sizeof(struct sockaddr_storage)` beforehand; `accept()` will adjust it down if it used fewer bytes.

```c
struct sockaddr_storage their_addr;
socklen_t addr_size = sizeof their_addr;
int new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
// now new_fd is ready for send()/recv() with this one client,
// while sockfd is still listening for the next one
```

You'll see this exact flow inside a real, running server in `04_TCP_Client_Server.md` — reading it in isolation is much less intuitive than watching it accept a real connection.

## 3.7 `send()` and `recv()` — for stream sockets (and connected datagram sockets)

```c
int send(int sockfd, const void *msg, int len, int flags);
int recv(int sockfd, void *buf, int len, int flags);
```

Set `flags` to `0` for normal use in both. Three things to internalize now, because they'll matter a lot later:

1. **These are blocking calls.** `recv()` will sit there — "sleep," in kernel terminology — until data actually arrives. Your program doesn't spin or poll; it's simply suspended by the OS until there's something to hand back. `send()` can technically block too (if internal buffers are full), though this is rarer for small messages.
2. **`send()` might not send everything you asked it to.** Its return value is the number of bytes it *actually* managed to queue up — which can be less than `len` if the amount of data is large. For small messages (under ~1KB) this basically never happens in practice, but robust code checks the return value and sends the rest in a loop. You'll see the book's own helper function for this, `sendall()`, in `07_Serialization_and_Broadcast.md`.
3. **`recv()` returning `0` is a specific, meaningful signal: the other end has closed the connection.** It's not an error (that's `-1`), and it's not "no data yet" (that's just... blocking). It specifically means "there will never be more data on this socket — the peer hung up."

## 3.8 `sendto()` and `recvfrom()` — for unconnected datagram sockets

Datagram sockets that haven't been `connect()`-ed (the normal case for UDP) don't have a fixed "other end" to `send()`/`recv()` with — every packet needs its own destination address attached, since who you're sending *to* can change from packet to packet:

```c
int sendto(int sockfd, const void *msg, int len, unsigned int flags,
           const struct sockaddr *to, socklen_t tolen);

int recvfrom(int sockfd, void *buf, int len, unsigned int flags,
             struct sockaddr *from, int *fromlen);
```

Same idea as `send()`/`recv()`, with an address struct bolted on — `to` for where you're sending, `from` for who a received packet came from (typically a `struct sockaddr_storage`, for the same "could be IPv4 or IPv6" reason as `accept()`). You'll see both of these used for real, with actual UDP packets going over `localhost`, in `05_UDP_Datagram_Sockets.md`.

> **Worth remembering:** if you *do* `connect()` a datagram socket (yes, this is legal — it just tells the kernel "always send to/receive from this one address, don't make me repeat it"), you can then use the plain `send()`/`recv()` instead of `sendto()`/`recvfrom()`. The socket is still UDP underneath; you've just fixed who the "other end" is.

## 3.9 `close()` and `shutdown()` — get outta my face

`close(sockfd)` works exactly like closing any other file descriptor — it's the same function you'd use to close a file. After this, neither end can read or write that socket anymore.

`shutdown()` gives you finer-grained control if you want to close only one direction of a connection while leaving the other open:

```c
int shutdown(int sockfd, int how);
```

| `how` | Effect |
|---|---|
| `0` | Further **receives** disallowed |
| `1` | Further **sends** disallowed |
| `2` | Both — equivalent to `close()`, except... |

**Important distinction: `shutdown()` does *not* actually free the file descriptor.** It only changes what you're allowed to do with it — you still need to call `close()` afterward to actually release it. (On Windows/Winsock, the equivalent of `close()` is `closesocket()` — a naming quirk worth knowing if you ever port code.)

## 3.10 `getpeername()` — Who are you?

```c
#include <sys/socket.h>
int getpeername(int sockfd, struct sockaddr *addr, int *addrlen);
```

Once you have a connected stream socket, this tells you who's on the other end (fills in an address struct, same pattern as everything above). Genuinely simple — there's not much more to say about it.

## 3.11 `gethostname()` — Who am I?

```c
#include <unistd.h>
int gethostname(char *hostname, size_t size);
```

Returns the hostname of the machine your own program is running on — handy if you then want to feed that name into `getaddrinfo()` to find your own machine's address.

---

## Quick recap (for revision)

**The two call sequences, side by side:**

```
 SERVER                              CLIENT
 ──────                              ──────
 getaddrinfo(NULL, port, AI_PASSIVE) getaddrinfo(hostname, port)
 socket()                            socket()
 setsockopt(SO_REUSEADDR)
 bind()
 listen()
 accept()  ──► new_fd                connect()
 send()/recv() on new_fd             send()/recv() on sockfd
 close(new_fd)                       close(sockfd)
```

- `getaddrinfo()` returns `0`/nonzero + `gai_strerror()` — **not** the usual `-1`/`errno` pattern every other function here uses.
- `accept()` returns a **new** socket descriptor per client; the original listening socket keeps listening.
- `send()`/`recv()` are for connected sockets (TCP, or UDP after `connect()`); `sendto()`/`recvfrom()` are for unconnected UDP, and carry an address with every call.
- `recv()` returning `0` means "the peer closed the connection" — not an error, not "nothing yet."
- `send()`'s return value can be less than what you asked it to send — always check it for anything beyond small demo messages.
- `"Address already in use"` on a restarted server → add `SO_REUSEADDR` before `bind()`.

Next: `04_TCP_Client_Server.md` — every function above, wired together into a real, running server and client.