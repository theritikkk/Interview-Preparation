# 4. A Real TCP Client and Server
### *(Covers the guide's Chapter 6.1–6.2)*

Everything up to now has been individual pieces. This chapter puts them together into a program you can actually compile, run, and watch talk to itself — one process playing server, another playing client, both on `localhost`.

## 4.1 The client-server model, in one picture

Almost everything on a network is one program (the **client**) initiating contact, and another program (the **server**) that was already sitting there waiting for it. Your browser is a client; the web server it talks to is, well, a server. `ssh` is a client; `sshd` is the server. Once connected, both sides can `send()` and `recv()` — it's a two-way conversation, not a one-shot request:

```
   Client                        Server
     │                              │
     │──────── send() ────────────► │  request
     │                              │  recv()
     │                              │
     │          (network)           │
     │                              │
     │◄─────── recv() ──────────────│  response
     │                              │  send()
```

A server usually needs to serve **many** clients, often at once. The classic Unix approach — and the one this first example uses — is: the main process sits in a loop calling `accept()`, and every time a new client connects, it `fork()`s a child process to handle just that one client, while the parent immediately goes back to `accept()`-ing the next one. (We'll meet more scalable alternatives to `fork()`-per-client — `poll()` and `select()` — in Chapter 6.)

## 4.2 `server.c` — the whole thing

This server does exactly one thing: it accepts a connection, sends back the literal text `"Hello, world!"`, and closes the connection. Every function call here was introduced in the previous chapter — what's new is seeing them assembled into a real `main()`.

```c
/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT "3490" // the port users will be connecting to

#define BACKLOG 10 // how many pending connections queue will hold

void sigchld_handler(int s)
{
    (void)s; // quiet unused variable warning

    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;

    errno = saved_errno;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(void)
{
    // listen on sock_fd, new connection on new_fd
    int sockfd, new_fd;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address info
    socklen_t sin_size;
    struct sigaction sa;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                       sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while (1) { // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr,
                        &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
                  get_in_addr((struct sockaddr *)&their_addr),
                  s, sizeof s);
        printf("server: got connection from %s\n", s);

        if (!fork()) {     // this is the child process
            close(sockfd); // child doesn't need the listener
            if (send(new_fd, "Hello, world!", 13, 0) == -1)
                perror("send");
            close(new_fd);
            exit(0);
        }
        close(new_fd); // parent doesn't need this
    }

    return 0;
}
```

### Walking through the parts that aren't just "the five functions"

- **`sigchld_handler()` and the `sigaction()` setup.** When the server `fork()`s a child to handle a client, and that child eventually `exit()`s, the child becomes a **zombie process** until the parent "reaps" it with `waitpid()`. Left unhandled, a busy server would slowly accumulate zombies forever. The fix: install a signal handler for `SIGCHLD` (sent automatically whenever *any* child process terminates) that calls `waitpid(-1, NULL, WNOHANG)` in a loop to reap every child that's finished, without blocking if none have. It carefully saves/restores `errno` because the handler can interrupt the main program at literally any point, including mid-way through another system call that was about to check `errno` itself.
- **`get_in_addr()`.** A small helper that takes a generic `struct sockaddr *` and returns a pointer to the actual address bytes inside it — checking `sa_family` to know whether to treat it as a `sockaddr_in` (IPv4) or `sockaddr_in6` (IPv6) first. This is the "figure out which concrete struct this actually is" pattern from Chapter 2, put into a reusable function.
- **The `for` loop over `servinfo`.** `getaddrinfo()` can return several candidate addresses; this loop tries each one (`socket()` then `bind()`) until one succeeds, rather than blindly assuming the first result works.
- **`fork()`-per-client.** `!fork()` is `true` in exactly one of the two processes now running: the **child** (where `fork()` returns `0`). The child sends the greeting and exits; the parent immediately closes its own copy of `new_fd` (it doesn't need it — the child has its own copy) and loops back to `accept()` the next client.

## 4.3 `client.c` — the whole thing

```c
/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define PORT "3490" // the port client will be connecting to

#define MAXDATASIZE 100 // max number of bytes we can get at once

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
    int sockfd, numbytes;
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    if (argc != 2) {
        fprintf(stderr, "usage: client hostname\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        inet_ntop(p->ai_family,
                  get_in_addr((struct sockaddr *)p->ai_addr),
                  s, sizeof s);
        printf("client: attempting connection to %s\n", s);

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("client: connect");
            close(sockfd);
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family,
              get_in_addr((struct sockaddr *)p->ai_addr),
              s, sizeof s);
    printf("client: connected to %s\n", s);

    freeaddrinfo(servinfo); // all done with this structure

    if ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1) {
        perror("recv");
        exit(1);
    }

    buf[numbytes] = '\0';

    printf("client: received '%s'\n", buf);

    close(sockfd);

    return 0;
}
```

Notice this is almost the mirror image of the server: `getaddrinfo()` → `socket()` → `connect()` (no `bind()`, no `listen()`, no `accept()` — the client only cares about the *remote* address). Then a single `recv()` to grab the server's greeting, and `close()`.

One detail worth catching: `buf[numbytes] = '\0';` — `recv()` fills a raw byte buffer, it does **not** null-terminate it for you (why would it — it's not necessarily text!). Since the server sent exactly 13 bytes of ASCII text with no null terminator, the client manually adds one after receiving, so that `printf("%s", buf)` has a proper string to print instead of reading past the end of what was actually received.

## 4.4 Running it for real

Two terminal windows, one machine (`localhost`), real output — no guessing:

**Terminal 1 — start the server:**
```
$ gcc -Wall -Wextra -o server server.c
$ ./server
server: waiting for connections...
```
(It blocks here — this is `accept()` doing exactly what Chapter 3 said it would: sleeping until a client shows up.)

**Terminal 2 — run the client:**
```
$ gcc -Wall -Wextra -o client client.c
$ ./client localhost
client: attempting connection to 127.0.0.1
client: connected to 127.0.0.1
client: received 'Hello, world!'
```

**Back in Terminal 1**, the moment the client connected, this line appeared:
```
server: got connection from 127.0.0.1
```

The server then goes right back to `printf`-silently waiting for the next connection (try running the client a second time — you'll see a fresh `"got connection from 127.0.0.1"` and the client will get its `"Hello, world!"` again, since the server never actually stops running).

### What if there's no server running?

Worth deliberately trying once, because the failure mode is instructive:
```
$ ./client localhost
client: attempting connection to 127.0.0.1
client: connect: Connection refused
client: failed to connect
```
This is the OS actively telling you "I checked port 3490 on that machine, and nothing is listening" — a fast, definite failure. (Contrast this with connecting to a hostname/port that's silently firewalled somewhere in between, which instead just hangs until a timeout — a much less friendly failure mode you'll eventually run into on real networks, though not one you can easily demo on `localhost`.)

---

## Quick recap (for revision)

- Client-server = one side waits (`accept()`), one side initiates (`connect()`). Both can then `send()`/`recv()` freely — it's a two-way stream, not request-then-hang-up.
- This server handles multiple clients via **`fork()`-per-connection**: parent keeps `accept()`-ing, each child handles exactly one client then exits.
- A `SIGCHLD` handler calling `waitpid(..., WNOHANG)` is required to prevent forked children from piling up as zombies.
- `recv()` does not null-terminate what it reads — you must do that yourself if you intend to treat the bytes as a C string.
- `"Connection refused"` = nothing is listening on that port on that host — an immediate, definite error, not a timeout.

**Try this yourself:** modify `client.c`'s port to something nobody's listening on (e.g. `9999`) and re-run it against your running `server.c` — you should reproduce the `"Connection refused"` output above. Then try connecting the *unmodified* client to a hostname that doesn't resolve (e.g. `./client this-does-not-exist.invalid`) and see what `getaddrinfo()`'s own error path looks like.

Next: `05_UDP_Datagram_Sockets.md` — the same idea, but connectionless, plus a real bug I ran into building these notes.