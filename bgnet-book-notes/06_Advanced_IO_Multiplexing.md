# 6. Handling Many Clients: `poll()` and `select()`
### *(Covers the guide's Chapter 7.1–7.3)*

The `fork()`-per-client server in Chapter 4 works, but spawning a whole new OS process for every single connection is heavy. This chapter covers the classic alternative: **one single process that watches many sockets at once**, and only does work on the ones that actually have something to say.

## 6.1 Blocking, and why it's a problem here

Recall from Chapter 3: `accept()` and the `recv()` family **block** — they sleep the calling process until something happens. That's completely fine when you have exactly one thing to wait for. It becomes a real problem the moment you want to wait on *several* sockets simultaneously: if you call `recv()` on client A and no data ever comes, you're frozen, even if client B has been trying to talk to you for the last ten seconds.

The "obvious" fix — making a socket **non-blocking** with `fcntl(sockfd, F_SETFL, O_NONBLOCK)`, so a `recv()` with nothing to read returns `-1`/`EWOULDBLOCK` immediately instead of sleeping — technically works, but leads you to write a loop that calls `recv()` over and over as fast as possible on every socket, "just checking." That's called **busy-waiting**, and it will peg a CPU core at 100% for no reason. There's a better way.

## 6.2 `poll()` — ask the kernel to do the waiting for you

The right approach: hand the kernel a *list* of sockets and say "wake me up when **any** of these has something ready — and until then, let me sleep, using zero CPU." That's exactly what `poll()` does.

```c
#include <poll.h>

int poll(struct pollfd fds[], nfds_t nfds, int timeout);
```

You build an array of this struct, one entry per socket you care about:

```c
struct pollfd {
    int   fd;      // the socket descriptor to watch
    short events;  // bitmask: what you want to be told about
    short revents; // bitmask: what actually happened (filled in by poll())
};
```

The `events` field you set yourself, usually just `POLLIN` ("tell me when there's data to `recv()`"). `timeout` is in milliseconds — a negative value means "wait forever," `0` means "check right now and return immediately" (effectively a manual poll, hence the function's name). When `poll()` returns, it tells you *how many* file descriptors had something happen; you then loop through your array checking each entry's `revents` field.

### Minimal demo: waiting on stdin with a timeout

```c
#include <stdio.h>
#include <poll.h>

int main(void)
{
    struct pollfd pfds[1]; // More if you want to monitor more

    pfds[0].fd = 0; // Standard input
    pfds[0].events = POLLIN; // Tell me when ready to read

    printf("Hit RETURN or wait 2.5 seconds for timeout\n");

    int num_events = poll(pfds, 1, 2500); // 2.5 second timeout

    if (num_events == 0) {
        printf("Poll timed out!\n");
    } else {
        int pollin_happened = pfds[0].revents & POLLIN;

        if (pollin_happened) {
            printf("File descriptor %d is ready to read\n", pfds[0].fd);
        } else {
            printf("Unexpected event occurred: %d\n", pfds[0].revents);
        }
    }

    return 0;
}
```

**Real output, both code paths, actually run:**

```
$ echo "hi" | ./poll_demo
Hit RETURN or wait 2.5 seconds for timeout
File descriptor 0 is ready to read

$ ./poll_demo < some_still-open_but_idle_input
Hit RETURN or wait 2.5 seconds for timeout
Poll timed out!
```

(One subtlety I ran into building this: redirecting from `/dev/null` makes stdin immediately "ready to read" too — because reaching end-of-file counts as readable, same as `recv()` returning `0` meant "peer hung up" back in Chapter 3. To see the *actual* 2.5-second timeout path, stdin needs to be a still-open pipe with nobody writing to it yet, not an already-closed one.)

### The real payoff: `pollserver.c`, a chat server for many clients

This program listens on port 9034 and relays whatever any client sends to every *other* connected client — a minimal chat room. All of it runs in **one process, no `fork()` anywhere** — this is the entire point of using `poll()`.

```c
/*
** pollserver.c -- a cheezy multiperson chat server
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>

#define PORT "9034" // Port we're listening on

/*
 * Convert socket to IP address string.
 * addr: struct sockaddr_in or struct sockaddr_in6
 */
const char *inet_ntop2(void *addr, char *buf, size_t size)
{
    struct sockaddr_storage *sas = addr;
    struct sockaddr_in *sa4;
    struct sockaddr_in6 *sa6;
    void *src;

    switch (sas->ss_family) {
    case AF_INET:
        sa4 = addr;
        src = &(sa4->sin_addr);
        break;
    case AF_INET6:
        sa6 = addr;
        src = &(sa6->sin6_addr);
        break;
    default:
        return NULL;
    }

    return inet_ntop(sas->ss_family, src, buf, size);
}

/*
 * Return a listening socket.
 */
int get_listener_socket(void)
{
    int listener; // Listening socket descriptor
    int yes = 1;  // For setsockopt() SO_REUSEADDR, below
    int rv;

    struct addrinfo hints, *ai, *p;

    // Get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
        fprintf(stderr, "pollserver: %s\n", gai_strerror(rv));
        exit(1);
    }

    for (p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype,
                          p->ai_protocol);
        if (listener < 0) {
            continue;
        }

        // Lose the pesky "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes,
                   sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }

        break;
    }

    // If we got here, it means we didn't get bound
    if (p == NULL) {
        return -1;
    }

    freeaddrinfo(ai); // All done with this

    // Listen
    if (listen(listener, 10) == -1) {
        return -1;
    }

    return listener;
}

/*
 * Add a new file descriptor to the set.
 */
void add_to_pfds(struct pollfd **pfds, int newfd, int *fd_count,
                 int *fd_size)
{
    // If we don't have room, add more space in the pfds array
    if (*fd_count == *fd_size) {
        *fd_size *= 2; // Double it
        *pfds = realloc(*pfds, sizeof(**pfds) * (*fd_size));
    }

    (*pfds)[*fd_count].fd = newfd;
    (*pfds)[*fd_count].events = POLLIN; // Check ready-to-read
    (*pfds)[*fd_count].revents = 0;

    (*fd_count)++;
}

/*
 * Remove a file descriptor at a given index from the set.
 */
void del_from_pfds(struct pollfd pfds[], int i, int *fd_count)
{
    // Copy the one from the end over this one
    pfds[i] = pfds[*fd_count - 1];

    (*fd_count)--;
}

/*
 * Handle incoming connections.
 */
void handle_new_connection(int listener, int *fd_count,
                           int *fd_size, struct pollfd **pfds)
{
    struct sockaddr_storage remoteaddr; // Client address
    socklen_t addrlen;
    int newfd; // Newly accept()ed socket descriptor
    char remoteIP[INET6_ADDRSTRLEN];

    addrlen = sizeof remoteaddr;
    newfd = accept(listener, (struct sockaddr *)&remoteaddr,
                   &addrlen);

    if (newfd == -1) {
        perror("accept");
    } else {
        add_to_pfds(pfds, newfd, fd_count, fd_size);

        printf("pollserver: new connection from %s on socket %d\n",
               inet_ntop2(&remoteaddr, remoteIP, sizeof remoteIP),
               newfd);
    }
}

/*
 * Handle regular client data or client hangups.
 */
void handle_client_data(int listener, int *fd_count,
                        struct pollfd *pfds, int *pfd_i)
{
    char buf[256]; // Buffer for client data

    int nbytes = recv(pfds[*pfd_i].fd, buf, sizeof buf, 0);

    int sender_fd = pfds[*pfd_i].fd;

    if (nbytes <= 0) { // Got error or connection closed by client
        if (nbytes == 0) {
            // Connection closed
            printf("pollserver: socket %d hung up\n", sender_fd);
        } else {
            perror("recv");
        }

        close(pfds[*pfd_i].fd); // Bye!

        del_from_pfds(pfds, *pfd_i, fd_count);

        // reexamine the slot we just deleted
        (*pfd_i)--;

    } else { // We got some good data from a client
        printf("pollserver: recv from fd %d: %.*s", sender_fd,
               nbytes, buf);
        // Send to everyone!
        for (int j = 0; j < *fd_count; j++) {
            int dest_fd = pfds[j].fd;

            // Except the listener and ourselves
            if (dest_fd != listener && dest_fd != sender_fd) {
                if (send(dest_fd, buf, nbytes, 0) == -1) {
                    perror("send");
                }
            }
        }
    }
}

/*
 * Process all existing connections.
 */
void process_connections(int listener, int *fd_count, int *fd_size,
                         struct pollfd **pfds)
{
    for (int i = 0; i < *fd_count; i++) {

        // Check if someone's ready to read
        if ((*pfds)[i].revents & (POLLIN | POLLHUP)) {
            // We got one!!

            if ((*pfds)[i].fd == listener) {
                // If we're the listener, it's a new connection
                handle_new_connection(listener, fd_count, fd_size,
                                      pfds);
            } else {
                // Otherwise we're just a regular client
                handle_client_data(listener, fd_count, *pfds, &i);
            }
        }
    }
}

/*
 * Main: create a listener and connection set, loop forever
 * processing connections.
 */
int main(void)
{
    int listener; // Listening socket descriptor

    // Start off with room for 5 connections
    // (We'll realloc as necessary)
    int fd_size = 5;
    int fd_count = 0;
    struct pollfd *pfds = malloc(sizeof *pfds * fd_size);

    // Set up and get a listening socket
    listener = get_listener_socket();

    if (listener == -1) {
        fprintf(stderr, "error getting listening socket\n");
        exit(1);
    }

    // Add the listener to set;
    // Report ready to read on incoming connection
    pfds[0].fd = listener;
    pfds[0].events = POLLIN;

    fd_count = 1; // For the listener

    puts("pollserver: waiting for connections...");

    // Main loop
    for (;;) {
        int poll_count = poll(pfds, fd_count, -1);

        if (poll_count == -1) {
            perror("poll");
            exit(1);
        }

        // Run through connections looking for data to read
        process_connections(listener, &fd_count, &fd_size, &pfds);
    }

    free(pfds);
}
```

**The key trick, worth calling out explicitly:** the *listening* socket itself is put into the same `pfds` array as every client. When a brand new connection is waiting, `poll()` reports the **listener** as "ready to read" — reading being a stand-in here for "there's an `accept()`-able connection queued." `process_connections()` checks `(*pfds)[i].fd == listener` to tell "new client trying to connect" apart from "existing client sent data," and dispatches to `handle_new_connection()` or `handle_client_data()` accordingly. One `poll()` call, one array, one loop, handling both cases.

### Real multi-client run — three simultaneous connections

I compiled this and connected three separate clients to it with `nc` (netcat), simulating three people in a chat room, sent messages from two of them, and forcibly disconnected the third mid-session:

```
$ gcc -Wall -Wextra -o pollserver pollserver.c
$ ./pollserver &
```

**Server log (captured live, in order):**
```
pollserver: waiting for connections...
pollserver: new connection from 127.0.0.1 on socket 4
pollserver: new connection from 127.0.0.1 on socket 5
pollserver: new connection from 127.0.0.1 on socket 6
pollserver: recv from fd 4: Hello from client A!
pollserver: recv from fd 5: Hi everyone, this is B
pollserver: socket 6 hung up
pollserver: recv from fd 4: Is C still here?
```

**What each client actually saw:**

| Client | Sent | Received |
|---|---|---|
| A (fd 4) | `Hello from client A!`, later `Is C still here?` | `Hi everyone, this is B` |
| B (fd 5) | `Hi everyone, this is B` | `Hello from client A!`, `Is C still here?` |
| C (fd 6) | *(nothing — killed after connecting)* | `Hello from client A!`, `Hi everyone, this is B` |

This is exactly the "send to everyone except the listener and the sender" logic in `handle_client_data()`, confirmed working live: **A never sees its own message echoed back**, and once **C is killed, the server detects the hangup on its very next `poll()` cycle** (`"socket 6 hung up"`) and stops trying to send it anything further — no error, no crash, no zombie entry lingering in the `pfds` array.

## 6.3 `select()` — the older, more portable cousin of `poll()`

`select()` solves the exact same problem as `poll()` and predates it; you'll see it constantly in older C and in code that needs to run on more exotic/older Unix systems, since it's slightly more universally available. It works with a different data structure — a **bitmask-like set** (`fd_set`) rather than an array of structs — manipulated with four macros:

```c
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

int select(int numfds, fd_set *readfds, fd_set *writefds,
           fd_set *exceptfds, struct timeval *timeout);
```

| Macro | Meaning |
|---|---|
| `FD_SET(fd, &set)` | Add `fd` to the set |
| `FD_CLR(fd, &set)` | Remove `fd` from the set |
| `FD_ISSET(fd, &set)` | Is `fd` currently in the set? |
| `FD_ZERO(&set)` | Empty the set entirely |

`numfds` must be set to **the highest file descriptor number in any of your sets, plus one** — an easy detail to forget and a classic source of "why isn't this working" bugs. `timeout` works like `poll()`'s, but as a `struct timeval` (seconds + **micro**seconds, not milliseconds) instead of a plain integer of milliseconds; pass `NULL` to wait forever.

### Minimal demo

```c
/*
** select.c -- a select() demo
*/

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define STDIN 0 // file descriptor for standard input

int main(void)
{
    struct timeval tv;
    fd_set readfds;

    tv.tv_sec = 2;
    tv.tv_usec = 500000;

    FD_ZERO(&readfds);
    FD_SET(STDIN, &readfds);

    // don't care about writefds and exceptfds:
    select(STDIN + 1, &readfds, NULL, NULL, &tv);

    if (FD_ISSET(STDIN, &readfds))
        printf("A key was pressed!\n");
    else
        printf("Timed out.\n");

    return 0;
}
```

**Real output, both paths:**
```
$ echo x | ./select_demo
A key was pressed!

$ ./select_demo < some_still-open_but_idle_input
Timed out.
```

### `selectserver.c` — the same chat server, `select()`-flavored

Functionally identical to `pollserver.c` — same relay logic (broadcast to everyone except the listener and the sender), same hangup handling — just built on `fd_set`/`FD_ISSET` instead of an array of `struct pollfd`. Notice it tracks `fdmax` by hand (the "highest fd" `select()` needs) since there's no array to just measure the length of:

```c
/*
** selectserver.c -- a cheezy multiperson chat server
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT "9034" // port we're listening on

/*
 * Convert socket to IP address string.
 * addr: struct sockaddr_in or struct sockaddr_in6
 */
const char *inet_ntop2(void *addr, char *buf, size_t size)
{
    struct sockaddr_storage *sas = addr;
    struct sockaddr_in *sa4;
    struct sockaddr_in6 *sa6;
    void *src;

    switch (sas->ss_family) {
    case AF_INET:
        sa4 = addr;
        src = &(sa4->sin_addr);
        break;
    case AF_INET6:
        sa6 = addr;
        src = &(sa6->sin6_addr);
        break;
    default:
        return NULL;
    }

    return inet_ntop(sas->ss_family, src, buf, size);
}

/*
 * Return a listening socket
 */
int get_listener_socket(void)
{
    struct addrinfo hints, *ai, *p;
    int yes = 1; // for setsockopt() SO_REUSEADDR, below
    int rv;
    int listener;

    // get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }

    for (p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype,
                          p->ai_protocol);
        if (listener < 0) {
            continue;
        }

        // lose the pesky "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes,
                   sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }

        break;
    }

    // if we got here, it means we didn't get bound
    if (p == NULL) {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }

    freeaddrinfo(ai); // all done with this

    // listen
    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(3);
    }

    return listener;
}

/*
 * Add new incoming connections to the proper sets
 */
void handle_new_connection(int listener, fd_set *master, int *fdmax)
{
    socklen_t addrlen;
    int newfd;                          // newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr; // client address
    char remoteIP[INET6_ADDRSTRLEN];

    addrlen = sizeof remoteaddr;
    newfd = accept(listener,
                   (struct sockaddr *)&remoteaddr,
                   &addrlen);

    if (newfd == -1) {
        perror("accept");
    } else {
        FD_SET(newfd, master); // add to master set
        if (newfd > *fdmax) {  // keep track of the max
            *fdmax = newfd;
        }
        printf("selectserver: new connection from %s on "
               "socket %d\n",
               inet_ntop2(&remoteaddr, remoteIP, sizeof remoteIP),
               newfd);
    }
}

/*
 * Broadcast a message to all clients
 */
void broadcast(char *buf, int nbytes, int listener, int s,
               fd_set *master, int fdmax)
{
    for (int j = 0; j <= fdmax; j++) {
        // send to everyone!
        if (FD_ISSET(j, master)) {
            // except the listener and ourselves
            if (j != listener && j != s) {
                if (send(j, buf, nbytes, 0) == -1) {
                    perror("send");
                }
            }
        }
    }
}

/*
 * Handle client data and hangups
 */
void handle_client_data(int s, int listener, fd_set *master,
                        int fdmax)
{
    char buf[256]; // buffer for client data
    int nbytes;

    // handle data from a client
    if ((nbytes = recv(s, buf, sizeof buf, 0)) <= 0) {
        // got error or connection closed by client
        if (nbytes == 0) {
            // connection closed
            printf("selectserver: socket %d hung up\n", s);
        } else {
            perror("recv");
        }
        close(s);          // bye!
        FD_CLR(s, master); // remove from master set
    } else {
        // we got some data from a client
        broadcast(buf, nbytes, listener, s, master, fdmax);
    }
}

/*
 * Main
 */
int main(void)
{
    fd_set master;   // master file descriptor list
    fd_set read_fds; // temp file descriptor list for select()
    int fdmax;       // maximum file descriptor number

    int listener; // listening socket descriptor

    FD_ZERO(&master); // clear the master and temp sets
    FD_ZERO(&read_fds);

    listener = get_listener_socket();

    // add the listener to the master set
    FD_SET(listener, &master);

    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one

    // main loop
    for (;;) {
        read_fds = master; // copy it
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }

        // run through the existing connections looking for data
        // to read
        for (int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { // we got one!!
                if (i == listener)
                    handle_new_connection(i, &master, &fdmax);
                else
                    handle_client_data(i, listener, &master, fdmax);
            }
        }
    }

    return 0;
}
```

**Real two-client run**, confirming identical behavior to `pollserver.c`:

```
selectserver: new connection from 127.0.0.1 on socket 4
selectserver: new connection from 127.0.0.1 on socket 5
selectserver: socket 4 hung up
selectserver: socket 5 hung up
```
Client A (socket 4) sent `"Hello from A on selectserver!"` and received back `"Reply from B"`; client B (socket 5) sent `"Reply from B"` and received `"Hello from A on selectserver!"` — same broadcast-except-sender relay, same clean hangup handling, different underlying mechanism.

### `poll()` vs. `select()` — when would you pick one over the other?

| | `poll()` | `select()` |
|---|---|---|
| Data structure | Array of `struct pollfd` (grows cleanly with `realloc`) | Fixed-size bitmask `fd_set` (has a compile-time max FD count on most systems) |
| Tracking "the highest fd" | Not needed | Required, and easy to forget to update |
| Portability | POSIX, ubiquitous on Unix; historically less common on some very old/embedded systems | Slightly more universally available, including some older/non-Unix systems |
| Practical takeaway | Prefer this for new code | Know it because it's everywhere in existing code and some interview/coursework contexts |

Both share the same real limitation, which the book is upfront about: **neither scales well to huge numbers of connections** (think tens of thousands) because both require the kernel to walk the *entire* list/set on every single call, even to check just one ready descriptor. For that scale, you'd reach for an OS-specific mechanism (`epoll` on Linux, `kqueue` on BSD/macOS) or a library like `libevent` that picks the fastest one available automatically — worth knowing the names exist, not something you need to implement as a beginner.

---

## Quick recap (for revision)

- Blocking calls (`accept()`, `recv()`) sleep until ready — great for one socket, useless for watching many at once.
- Non-blocking sockets (`fcntl(..., O_NONBLOCK)`) avoid sleeping but tempt you into a CPU-burning busy-wait loop — avoid this pattern.
- `poll()`: array of `struct pollfd { fd, events, revents }`; put the *listening* socket in the same array as every client — when it shows "ready to read," that means a new connection is waiting to be `accept()`-ed, not that there's data to `recv()`.
- `select()`: same job, older API, uses `fd_set` + `FD_SET`/`FD_CLR`/`FD_ISSET`/`FD_ZERO`, and requires you to manually track the highest fd number (`fdmax`) yourself.
- Both let one single process/thread juggle many clients with zero `fork()`-ing and zero CPU waste while idle.
- Neither is the "final answer" at very large scale — that's `epoll`/`kqueue`/`libevent` territory, out of scope here but good to know by name.

**Common beginner mix-up:** forgetting that `recv()` returning `0` (peer hung up) is a completely different thing from `recv()` returning `-1` (an actual error) — both `pollserver.c` and `selectserver.c` handle these as two separate branches (`nbytes == 0` vs. the general `nbytes <= 0` catch-all with `perror()`), and mixing them up will make your server either crash on a normal disconnect or silently ignore a real error.

Next: `07_Serialization_and_Broadcast.md` — what to do when `send()` doesn't send everything at once, how to pack structured (non-string) data for the wire, and UDP broadcast.