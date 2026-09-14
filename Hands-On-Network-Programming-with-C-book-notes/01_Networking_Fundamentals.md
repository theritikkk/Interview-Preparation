# 1. Networking Fundamentals
### *(Covers the book's Chapter 1 — "Introducing Networks and Protocols")*

This opening chapter is all background — no sockets yet. The goal is to have a solid mental model of *what's actually happening* underneath every socket call you'll write later, so none of it feels like magic.

## 1.1 Layers: OSI vs. TCP/IP

You'll see networking described with two competing layer models. The **OSI model** is the textbook/academic one, with seven layers:

```
7. Application
6. Presentation
5. Session
4. Transport
3. Network
2. Data Link
1. Physical
```

In practice, almost nobody implements all seven distinctly — real systems (and this book) use the simpler, four-layer **TCP/IP model**, which maps roughly onto OSI like this:

```
 TCP/IP model      ≈     OSI layers                                  Example
 ─────────────         ──────────────                                ───────
 Application           Application + Presentation + Session          HTTP, DNS, SSH, SMTP
 Transport              Transport                                    TCP, UDP
 Internet               Network                                      IP
 Link                   Data Link + Physical                         Ethernet, Wi-Fi
```

As a socket programmer, you'll spend virtually all your time at the **Application** layer, occasionally touching **Transport** (choosing TCP vs. UDP), and essentially never touching **Internet** or **Link** directly — the OS handles those for you. This is the entire point of having layers at all: each one only needs to know how to talk to the layer directly above and below it, so your HTTP client code works identically whether the machine is on Wi-Fi, Ethernet, or a cellular modem.

## 1.2 Data encapsulation, once more, concretely

Every layer wraps the data from the layer above it in its own header before passing it down, and unwraps it in reverse on the way up:

```
┌──────────────┬────────────┬─────────────┬────────────────────┐
│ Ethernet hdr │  IP header │ TCP/UDP hdr │    your app data   │
└──────────────┴────────────┴─────────────┴────────────────────┘
```

Nothing new conceptually here if you've seen this before — the useful habit to build is: whenever something in a network capture or a bug report looks confusing, ask *"which layer is this happening at?"* first. A "connection refused" is a Transport-layer event; a "no route to host" is Internet-layer; a physically unplugged cable is Link-layer. Knowing which layer a symptom belongs to massively narrows down where to look.

## 1.3 IP addresses, subnets, and CIDR — the practical version

An IPv4 address like `192.0.2.12` is split conceptually into a **network portion** and a **host portion**. The **subnet mask** tells you where that split happens. The modern, compact way to write this is **CIDR notation**: `192.0.2.0/24` means "the first 24 bits are the network, the remaining 8 bits identify a specific host on it" — giving you 256 possible addresses (192.0.2.0 through 192.0.2.255) on that network, two of which (the all-zeros and all-ones host portions) are reserved as the network's own address and its broadcast address respectively.

A few CIDR sizes worth having a feel for, since you'll see them constantly:

| CIDR | Host addresses | Common use |
|---|---|---|
| `/32` | 1 | A single specific host |
| `/24` | 256 | A typical small home/office LAN |
| `/16` | 65,536 | A large private network |
| `/8` | 16,777,216 | Historically, an entire "Class A" allocation |

**Private address ranges** (`10.0.0.0/8`, `172.16.0.0/12`, `192.168.0.0/16`) are set aside by convention for use inside local networks and are never routed on the public internet — this is why your home router can hand out `192.168.1.x` addresses without coordinating with anyone, and why **Network Address Translation (NAT)** exists: it's the mechanism your router uses to let many private-address devices share one public IP address when talking to the outside world.

## 1.4 Multicast, broadcast, and anycast — three ways to address "more than one"

Normal ("unicast") traffic goes to exactly one specific address. Three variations exist for other cases:

- **Broadcast**: one packet, delivered to *every* host on the local network segment (e.g., `192.168.1.255` for the `192.168.1.0/24` network). Simple, but wasteful — every machine has to at least look at the packet, whether it cares or not.
- **Multicast**: one packet, delivered only to hosts that have specifically "subscribed" to a multicast group address. This is the more targeted, scalable alternative to broadcast — used for things like streaming video to multiple simultaneous viewers without sending a separate copy of each packet to each one.
- **Anycast**: the *same* address is announced by multiple different physical servers in different locations, and network routing delivers each client's packets to whichever one is "closest" (by routing distance). This is how large-scale services (major DNS resolvers like `8.8.8.8`, content delivery networks) serve a global audience from one seemingly-single address while actually load-balancing across many real machines.

## 1.5 Port numbers, briefly revisited

A 16-bit number (0–65535) that, combined with an IP address, identifies a specific running program on a specific machine. Ports below 1024 are "well-known" and conventionally reserved for standard services (80 = HTTP, 443 = HTTPS, 22 = SSH) — and on most systems, actually require elevated privileges to bind to.

## 1.6 Finding your own machine's addresses, in C

Here's where this chapter finally becomes code. Rather than hardcoding an assumption about your own IP address, you can ask the operating system directly with `getifaddrs()` — a function that walks every network interface (adapter) on the machine and hands you back a linked list describing each one.

```c
/*
 * MIT License
 *
 * Copyright (c) 2018 Lewis Van Winkle
 * (Full MIT license text — see 00_START_HERE.md for the complete wording,
 * preserved unmodified from the book's official repository in every file.)
 */

#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>


int main() {

    struct ifaddrs *addresses;

    if (getifaddrs(&addresses) == -1) {
        printf("getifaddrs call failed\n");
        return -1;
    }

    struct ifaddrs *address = addresses;
    while(address) {
        if (address->ifa_addr == NULL) {
            address = address->ifa_next;
            continue;
        }
        int family = address->ifa_addr->sa_family;
        if (family == AF_INET || family == AF_INET6) {

            printf("%s\t", address->ifa_name);
            printf("%s\t", family == AF_INET ? "IPv4" : "IPv6");

            char ap[100];
            const int family_size = family == AF_INET ?
                sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
            getnameinfo(address->ifa_addr,
                    family_size, ap, sizeof(ap), 0, 0, NI_NUMERICHOST);
            printf("\t%s\n", ap);

        }
        address = address->ifa_next;
    }


    freeifaddrs(addresses);
    return 0;
}
```

Notice this reuses a function you'll meet properly in the DNS chapter — `getnameinfo()` — here just to convert a raw address struct into a printable string (the `NI_NUMERICHOST` flag says "give me the numeric address, don't try to resolve a hostname"). This is a nice preview of how the socket API's pieces get reused across totally different tasks: the same "convert this address struct to text" function works whether the address came from a live connection, a DNS query, or — as here — just enumerating your own hardware.

**This is Unix/Linux/macOS-only code** — `getifaddrs()` doesn't exist on Windows. (The book's repo includes a separate `win_list.c` using the Windows-specific `GetAdaptersAddresses()` API for that platform — the underlying goal is identical, just a completely different function call, which is a good early example of how much platform-specific branching real cross-platform network code ends up needing.)

**Real, compiled-and-run output**, from inside this sandbox:

```
$ gcc -Wall -o unix_list unix_list.c
$ ./unix_list
lo	IPv4		127.0.0.1
eth0	IPv4		192.0.2.2
```

Two interfaces: `lo` is the loopback device you'll be using for every "client and server on one machine" demo in these notes (Chapter 2 onward); `eth0` is this sandbox's actual (virtual) network interface, with its own private-range address. On your own machine, you'll likely see more entries — Wi-Fi, Ethernet, maybe a VPN's virtual adapter — and, if your machine has working IPv6 (this sandbox doesn't — see the note in `00_START_HERE.md`), IPv6 addresses listed alongside the IPv4 ones for interfaces that have both.

---

## Quick recap (for revision)

- TCP/IP's 4 practical layers (Application / Transport / Internet / Link) matter more day-to-day than OSI's academic 7.
- CIDR notation (`/24`, etc.) states network-vs-host split directly as a bit count — get comfortable converting a CIDR size to "how many addresses" quickly.
- Private address ranges (`10.x`, `172.16-31.x`, `192.168.x`) + NAT is why your home network doesn't need public IPs for every device.
- Broadcast = everyone on the segment; multicast = only subscribed hosts; anycast = "closest" of several servers sharing one address.
- `getifaddrs()` (Unix) / `GetAdaptersAddresses()` (Windows) enumerate your own machine's network interfaces — different APIs, same underlying goal, a preview of the cross-platform theme this whole book leans into.

Next: `02_Socket_API_Basics.md` — where the cross-platform socket macros this book uses everywhere get introduced, and you compile and run your first real client-server exchange.