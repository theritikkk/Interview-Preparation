# 5. UDP: Datagram Sockets
### *(Covers the guide's Chapter 6.3)*

Same client-server idea as the last chapter, but connectionless: no `listen()`, no `accept()`, and every packet carries its own destination address. This is `SOCK_DGRAM` from Chapter 1, made real.

## 5.1 The two programs

`listener.c` plays server: it sits on port 4950 and waits for exactly one incoming packet, prints it, and exits. `talker.c` plays client: it sends one packet containing whatever message you give it on the command line, to whatever host you name, then exits.

**A deliberate design choice worth understanding before you even read the code:** both programs hardcode `hints.ai_family = AF_INET6` instead of the `AF_UNSPEC` you saw everywhere in the TCP chapter. Why? Because UDP has no handshake. If `listener.c` happened to bind an IPv6 socket while `talker.c` happened to send from an IPv4 socket, the packet would simply **vanish — no error on either side.** With TCP, a family mismatch causes `connect()` to fail outright, and the client's retry loop (walking the `getaddrinfo()` results) would naturally try the other family next. UDP has no such safety net, so the book sidesteps the whole problem by pinning both programs to the same family explicitly. (The comment `// set to AF_INET to use IPv4` in the source is the book's own note for how to flip this if you need to — which, as you're about to see, is exactly what I needed to do.)

### `listener.c`

```c
/*
** listener.c -- a datagram sockets "server" demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define MYPORT "4950" // the port users will be connecting to

#define MAXBUFLEN 100

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
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct sockaddr_storage their_addr;
    char buf[MAXBUFLEN];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET6; // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("listener: socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("listener: bind");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "listener: failed to bind socket\n");
        return 2;
    }

    freeaddrinfo(servinfo);

    printf("listener: waiting to recvfrom...\n");

    addr_len = sizeof their_addr;
    if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN - 1, 0,
                             (struct sockaddr *)&their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
    }

    printf("listener: got packet from %s\n",
           inet_ntop(their_addr.ss_family,
                     get_in_addr((struct sockaddr *)&their_addr),
                     s, sizeof s));
    printf("listener: packet is %d bytes long\n", numbytes);
    buf[numbytes] = '\0';
    printf("listener: packet contains \"%s\"\n", buf);

    close(sockfd);

    return 0;
}
```

Compare this to `server.c` from the last chapter: `getaddrinfo()`, the try-each-result `for` loop, `socket()`, `bind()` — all identical in shape. What's missing is `listen()` and `accept()` — there's no notion of a "connection" to accept, so `listener.c` goes straight from `bind()` to blocking on `recvfrom()`, which (like `recv()`) sleeps until a packet actually shows up, and additionally fills in `their_addr` with whoever sent it.

### `talker.c`

```c
/*
** talker.c -- a datagram "client" demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define SERVERPORT "4950" // the port users will be connecting to

int main(int argc, char *argv[])
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;

    if (argc != 3) {
        fprintf(stderr, "usage: talker hostname message\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET6; // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_DGRAM;

    rv = getaddrinfo(argv[1], SERVERPORT, &hints, &servinfo);
    if (rv != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and make a socket
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("talker: socket");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "talker: failed to create socket\n");
        return 2;
    }

    if ((numbytes = sendto(sockfd, argv[2], strlen(argv[2]), 0,
                           p->ai_addr, p->ai_addrlen)) == -1) {
        perror("talker: sendto");
        exit(1);
    }

    freeaddrinfo(servinfo);

    printf("talker: sent %d bytes to %s\n", numbytes, argv[1]);
    close(sockfd);

    return 0;
}
```

Notice what's **absent** compared to `client.c`: no `bind()` (fine, same as before — clients don't care about their local port), but also no `connect()`! `talker.c` calls `socket()` and then goes **directly** to `sendto()`. This is the essence of connectionless sockets: there was never a "connection" to establish in the first place — `sendto()` just stamps the destination address (`p->ai_addr`) onto this one packet and fires it.

## 5.2 Running it — and a real bug I hit doing it

I compiled both exactly as printed above and ran them on `localhost`. Here's what actually happened:

```
$ gcc -Wall -Wextra -o listener listener.c
$ gcc -Wall -Wextra -o talker talker.c
$ ./listener &
$ ./talker localhost "Hello, world from UDP!"
```

**Real output:**
```
listener: socket: Address family not supported by protocol
listener: failed to bind socket

getaddrinfo: Name or service not known
```

Both programs failed! Was the book's code wrong? No — this is a genuine **environment** issue, and it's exactly the scenario the book's own comment (`// set to AF_INET to use IPv4`) anticipates. The sandbox this was tested in has **no IPv6 support in its loopback interface**: there's no `::1` entry set up, and the kernel refuses to even create an `AF_INET6` `SOCK_DGRAM` socket. `listener.c` failed at `socket()` for exactly that reason; `talker.c` failed one step earlier, because `getaddrinfo()` couldn't find an IPv6 address for `"localhost"` at all (there's no IPv6 entry for it in this machine's `/etc/hosts`).

**The fix is the one-line change the book's own comment invites:**
```c
hints.ai_family = AF_INET; // was AF_INET6
```
Apply that to both files, recompile, and re-run:

```
$ ./listener_ipv4 &
$ ./talker_ipv4 localhost "Hello, world from UDP!"
```

**Real output, this time successful:**
```
listener: waiting to recvfrom...
listener: got packet from 127.0.0.1
listener: packet is 22 bytes long
listener: packet contains "Hello, world from UDP!"
talker: sent 22 bytes to localhost
```

**Why this is worth remembering, beyond "it worked eventually":** this is a textbook demonstration of exactly the risk the book warned about one paragraph earlier — an address-family mismatch between two UDP endpoints causes a **silent, confusing failure** rather than a clean error most of the time. In this particular case we were lucky: the mismatch was severe enough (no IPv6 stack at all) that both sides errored out loudly instead of one side silently discarding packets. On a machine *with* working (but firewalled, or differently-configured) IPv6, you could easily end up in the worse situation — `talker.c` reports `"sent N bytes"` successfully, and `listener.c` just... never receives anything, with no error on either side to tell you why. If your own UDP code on your own machine ever "sends fine but nothing arrives," checking for exactly this kind of IPv4/IPv6 mismatch should be your first move.

---

## Quick recap (for revision)

- UDP client/server flow has no `listen()`/`accept()` — server: `getaddrinfo → socket → bind → recvfrom`; client: `getaddrinfo → socket → sendto` (no `connect()` at all).
- `recvfrom()` fills in a `sockaddr_storage` telling you who sent the packet — this is how a connectionless server can still know "who just talked to me."
- Address-family mismatches between UDP peers **fail silently** (packet just vanishes) rather than with a clean error — unlike TCP, where `connect()` would simply fail and you'd know immediately. This is exactly why this pair of programs deliberately hardcodes one family instead of using `AF_UNSPEC`.
- If your environment lacks IPv6 support (common in containers/sandboxes), you'll see `"Address family not supported by protocol"` from `socket()`, or `"Name or service not known"` from `getaddrinfo()` — both are environment issues, not code bugs, and both are fixed the same way: switch `AF_INET6` to `AF_INET`.

**Try this yourself:** run `listener.c` unmodified (IPv6) alongside the IPv4-patched `talker_ipv4.c` on a machine that *does* have working IPv6 loopback. You should be able to reproduce the "silent failure" case directly — `talker` reports success, `listener` never wakes up — which is a more realistic (and more dangerous) version of the bug than the loud one shown above.

Next: `06_Advanced_IO_Multiplexing.md` — handling many clients at once without `fork()`-ing a process per connection.