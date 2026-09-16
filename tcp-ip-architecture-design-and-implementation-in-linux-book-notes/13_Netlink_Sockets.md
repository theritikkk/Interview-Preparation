# 13. Netlink Sockets
### *(Covers the book's Chapter 13 — "Netlink Sockets")*

Every chapter so far has been about the **data plane** — actual packets flowing through actual connections. This chapter is about the **control plane** instead: how userspace tools and daemons (`ip route`, `netstat`, routing daemons, network managers) actually talk *to* the kernel to inspect or change its networking state — routes, interfaces, addresses — rather than to send or receive application data at all.

## 13.1 Why not just use `ioctl()`?

Older Unix networking control interfaces (and Linux's own earlier mechanisms) leaned heavily on `ioctl()` — a general-purpose "do something specific to this file descriptor" escape hatch. It works, but has real, specific limitations that matter a great deal for networking control specifically:

- **Strictly request/response, no proactive notification.** If a userspace routing daemon needs to know the instant a network interface goes down, `ioctl()` alone gives it no way to be *told* — only to repeatedly *ask* ("polling"), wasting cycles and introducing real detection latency.
- **Awkward extensibility.** Adding a genuinely new kind of information to report or configure typically means a new `ioctl()` number and a new, fairly rigid fixed-structure argument — no natural, general way to send a variable-length, richly-structured message.

**Netlink** is Linux's purpose-built answer: a **socket-based** IPC mechanism (you `socket(AF_NETLINK, ...)` it, exactly like any other socket) specifically designed for kernel-to-userspace *and* userspace-to-userspace control-plane communication, supporting variable-length structured messages *and*, critically, **multicast notification groups** — letting the kernel proactively push "something changed" events to every interested listener, with zero polling required.

## 13.2 The socket, the address, and the message format

```c
int sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
```

The third argument picks a specific **netlink family** — `NETLINK_ROUTE` is the one this chapter cares most about (routing table, interface, and address management — see §13.3), but others exist for entirely different subsystems (firewall configuration, ARP daemon communication, and more).

```c
struct sockaddr_nl {
    sa_family_t   nl_family;   // AF_NETLINK
    unsigned short nl_pad;
    __u32         nl_pid;      // this endpoint's own "port ID" (often the PID)
    __u32         nl_groups;   // bitmask: which multicast groups to receive
};
```

Unlike an ordinary IP-based socket's address (an IP + port identifying a specific machine and service), a netlink address identifies **a specific endpoint on the local machine** — `nl_pid` conventionally matches the requesting process's own PID (though it doesn't strictly have to), and `nl_groups` is where the multicast-subscription model lives: setting a bit here at `bind()` time means "also deliver me any message the kernel multicasts to this group," entirely independent of anything this process explicitly requested.

```c
struct nlmsghdr {
    __u32   nlmsg_len;     // total length of THIS message (header + payload)
    __u16   nlmsg_type;    // what kind of message (RTM_NEWROUTE, etc. — §13.3)
    __u16   nlmsg_flags;   // NLM_F_REQUEST, NLM_F_DUMP, NLM_F_ACK, ...
    __u32   nlmsg_seq;     // sequence number — matches requests to responses
    __u32   nlmsg_pid;     // sender's port ID
};
/* followed by nlmsg_len - sizeof(nlmsghdr) bytes of type-specific payload */
```

Every netlink message starts with this fixed header, and — since a single `recv()` (same lesson as every earlier chapter's byte-stream/datagram-boundary discussions) might return several concatenated messages at once — genuine netlink-parsing code always loops, reading `nlmsg_len` to find where one message ends and the next begins, using the same "walk a length-prefixed sequence" discipline you've now seen applied to DNS labels, HTTP chunked encoding, and SMTP multi-line responses across the earlier study packs in this series.

## 13.3 `rtnetlink` — routing, links, and addresses specifically

The `NETLINK_ROUTE` family's message types (`nlmsg_type`) cover exactly the operations `ip route`/`ip link`/`ip addr` expose to a human:

| Message type | Meaning |
|---|---|
| `RTM_NEWROUTE` / `RTM_DELROUTE` / `RTM_GETROUTE` | Add / remove / query a routing table entry |
| `RTM_NEWLINK` / `RTM_DELLINK` / `RTM_GETLINK` | Bring up/configure, remove, or query a network interface |
| `RTM_NEWADDR` / `RTM_DELADDR` / `RTM_GETADDR` | Add, remove, or query an IP address assigned to an interface |
| `RTM_NEWNEIGH` / `RTM_DELNEIGH` | Add or remove an ARP/neighbor-table entry |

When you type `ip route add 10.0.0.0/8 via 192.168.1.1`, the `ip` command is, under the hood, constructing exactly an `RTM_NEWROUTE` netlink message (with the destination, gateway, and other route attributes packed into the payload using a generic, extensible **attribute (`rtattr`) encoding** — a length-type-value scheme letting new fields be added over time without breaking older tools that don't understand them) and sending it to the kernel over a netlink socket. `14_IP_Routing_Table_FIB.md` picks up on the *receiving* end of exactly this message — what the kernel actually does once an `RTM_NEWROUTE` request like this arrives.

## 13.4 Multicast notification groups — the real payoff

Beyond simple request/response, `rtnetlink` defines multicast groups a process can subscribe to (via `nl_groups`, §13.2) to be told about changes **without asking**:

```c
struct sockaddr_nl addr;
memset(&addr, 0, sizeof(addr));
addr.nl_family = AF_NETLINK;
addr.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR | RTMGRP_IPV4_ROUTE;
bind(sock, (struct sockaddr*)&addr, sizeof(addr));

/* from here on, recv() on this socket will unblock automatically
   whenever a link state changes, an IPv4 address is added/removed,
   or a route changes — with NO polling required at all */
```

This is precisely how a routing daemon (historically Zebra, and its modern successors like FRRouting) or a system's network manager notices "the cable was just unplugged" or "DHCP just handed me a new address" essentially instantly, and reacts — subscribed once, at startup, to exactly the groups it cares about, then simply blocking in `recv()` until the kernel has something to say, the same fundamentally efficient blocking-until-ready pattern that's been a running theme since the very first socket-programming study pack in this series' `select()`/`poll()` chapters — just applied to control-plane events instead of application data.

## 13.5 What's changed since kernel 2.4.20

- **Generic Netlink (`NETLINK_GENERIC`)**, added during the 2.6 series, solves a real scaling problem with the original design: every *specific* netlink family originally needed its own reserved protocol number (a limited resource, similar in spirit to the scarce-syscall-numbers concern from `01_Kernel_Basics_and_TCPIP_System_Calls.md`). Generic Netlink instead provides **one** registered family that itself supports dynamic, run-time sub-family registration by name — letting new kernel subsystems add their own netlink-based control interface without needing a dedicated protocol number allocated at all. Much of the networking-adjacent (and non-networking) control-plane surface added since this book's era uses Generic Netlink rather than a dedicated numbered family.
- **Many more `rtnetlink`-adjacent and Generic-Netlink-based families exist today** for subsystems with no 2.4.20 counterpart at all — IPsec policy/state management (`NETLINK_XFRM`), audit logging (`NETLINK_AUDIT`), and considerably more, alongside `rtnetlink` itself having grown many new message types (network-namespace-aware operations, VRF — Virtual Routing and Forwarding — configuration, and more) supporting capabilities this book's single-routing-table model doesn't anticipate.
- **`ss`** (from `iproute2`, the same tool suite netlink-based `ip` belongs to) has increasingly displaced the older `netstat` for socket inspection specifically because it talks netlink directly, rather than `netstat`'s traditional approach of parsing `/proc` files — a smaller, but real, illustration of netlink's control-plane role expanding at the expense of older mechanisms even for read-only inspection tasks.

---

## Quick recap (for revision)

- Netlink exists because `ioctl()`-based control interfaces can't do proactive, multicast notification and are awkward to extend — netlink is socket-based, supports variable-length structured messages, and lets the kernel push change notifications to subscribers with zero polling.
- A netlink address (`sockaddr_nl`) identifies a local endpoint (`nl_pid`) and a multicast-group subscription bitmask (`nl_groups`) — nothing about a remote machine at all.
- Every netlink message starts with a fixed `nlmsghdr` (length, type, flags, sequence, pid) — parsing code must loop over `nlmsg_len` exactly like every other length-prefixed protocol covered across this whole notes series.
- `rtnetlink` (`NETLINK_ROUTE`)'s message types (`RTM_NEWROUTE`, `RTM_NEWLINK`, `RTM_NEWADDR`, `RTM_NEWNEIGH`, and their `DEL`/`GET` counterparts) are exactly what `ip route`/`ip link`/`ip addr` translate your commands into.
- Multicast groups (`RTMGRP_*`) are the mechanism behind routing daemons and network managers reacting to interface/address/route changes essentially instantly, without ever polling.
- Generic Netlink's dynamic family registration is the single biggest structural addition since this book's era — solving the "every family needs its own scarce protocol number" limitation the original, fixed-family design has.

Next: `14_IP_Routing_Table_FIB.md` — the actual routing table this chapter's `RTM_*ROUTE` messages are manipulating.