# 2. IP Addresses, Structs, and Byte Order
### *(Covers the guide's Chapters 3–4)*

This is the chapter where things stop being abstract theory and start being actual C. Read it slowly — every struct here is one you'll be casting, filling out, or reading from in nearly every program you write.

## 2.1 IP addresses, in a nutshell

**IPv4** addresses are four bytes, written as four decimal numbers separated by dots: `192.0.2.111`. There are about 4 billion possible IPv4 addresses (2³²), and — despite that sounding huge — the internet has run out of new ones to hand out.

**IPv6** was invented to fix that. It uses 128-bit addresses (2¹²⁸ possible addresses — an almost incomprehensibly large number), written as eight groups of hex digits separated by colons:

```
2001:0db8:c9d2:aee5:73e3:934a:a5ae:9551
```

Two shorthand rules make these less painful to read/write:
- Leading zeros in each group can be dropped: `0db8` → `db8`
- One run of consecutive all-zero groups can be collapsed to `::` (but only once per address, since otherwise it'd be ambiguous how many zero-groups were collapsed)

```
2001:0db8:c9d2:0012:0000:0000:0000:0051   →   2001:db8:c9d2:12::51
0000:0000:0000:0000:0000:0000:0000:0001   →   ::1
```

`::1` is the **IPv6 loopback address** — "this machine," equivalent to IPv4's `127.0.0.1`. You'll see both throughout these notes since almost every demo runs client and server on the same machine.

**Subnets, briefly:** an IP address is really split into a "network" part and a "host" part, e.g. `192.0.2.12` might mean "host 12 on network 192.0.2.0." The **netmask** (like `255.255.255.0`) marks where that split happens; the modern shorthand writes it as a suffix, e.g. `192.0.2.12/24`. You mostly don't need to manipulate this yourself as an application programmer — routers handle it — but you'll see the notation often enough to recognize it.

**Port numbers:** an IP address gets you to a *machine*; a **port number** (a 16-bit number, 0–65535) picks which *application* on that machine you're talking to. If the IP address is a hotel's street address, the port number is the room number. Well-known ports: HTTP=80, SSH=22, Telnet=23, SMTP=25. Ports below 1024 generally need special OS privileges to bind to — this is why demo servers in this guide use a port like `3490`, comfortably above that line.

## 2.2 Byte order (endianness) — the bug that bites everyone once

Multi-byte numbers can be stored in memory in two different orders:

- **Big-Endian** ("Network Byte Order"): the most significant byte comes first. This is the order every machine on the internet agrees to use when numbers travel over the wire.
- **Little-Endian** ("Host Byte Order" on x86/x86-64, which is what almost every desktop, laptop, and cloud server uses today): the least significant byte comes first.

The problem: your program's native (**host**) byte order might not match **network** byte order. If you write a 16-bit port number into a struct without converting it, and your machine is little-endian (nearly all are), the bytes will be backwards by the time they hit the wire — and the other end will read a completely different number.

The fix is four small conversion functions. You don't need to know or check what your host's byte order actually is — just always run values through the right one of these before putting them on the wire, and after taking them off:

| Function | Meaning |
|---|---|
| `htons()` | **H**ost **to** **N**etwork, **S**hort (16-bit) |
| `htonl()` | **H**ost **to** **N**etwork, **L**ong (32-bit) |
| `ntohs()` | **N**etwork **to** **H**ost, **S**hort |
| `ntohl()` | **N**etwork **to** **H**ost, **L**ong |

Rule of thumb: **convert to network order right before it goes out, convert to host order right after it comes in.** You'll use `htons()` constantly for port numbers (`sin_port`). There's no built-in 64-bit or floating-point variant — for those, see the serialization chapter (`07_Serialization_and_Broadcast.md`), where the book's own example has to build this from scratch.

## 2.3 The structs — this is the part people find scary

None of these structs are individually complicated. What's confusing the first time is *why there are so many of them*. Here's the short version: **`struct sockaddr` is a generic, one-size-fits-all address container. `struct sockaddr_in` and `struct sockaddr_in6` are IPv4- and IPv6-specific versions that are easier to actually fill in by hand. They're all designed to be the same size (or safely castable) so functions that want a "generic address" (`struct sockaddr *`) will happily accept a pointer to any of the specific ones, cast to that generic type.**

### `struct addrinfo` — your starting point

```c
struct addrinfo {
    int              ai_flags;     // AI_PASSIVE, AI_CANONNAME, etc.
    int              ai_family;    // AF_INET, AF_INET6, AF_UNSPEC
    int              ai_socktype;  // SOCK_STREAM, SOCK_DGRAM
    int              ai_protocol;  // use 0 for "any"
    size_t           ai_addrlen;   // size of ai_addr in bytes
    struct sockaddr *ai_addr;      // struct sockaddr_in or _in6
    char            *ai_canonname; // full canonical hostname
    struct addrinfo *ai_next;      // linked list, next node
};
```

This is what you'll fill out (partially — just the `hints` fields) and hand to `getaddrinfo()` in Chapter 5. It comes back as a **linked list** (`ai_next`), because a hostname can resolve to multiple addresses (e.g. one IPv4 and several IPv6 addresses) — you get to pick which one to actually use. Setting `ai_family = AF_UNSPEC` tells it "I don't care, give me whatever's available," which is how you write code that works for both IPv4 and IPv6 without an `#ifdef` in sight.

### `struct sockaddr` — the generic container

```c
struct sockaddr {
    unsigned short sa_family;   // AF_INET or AF_INET6
    char           sa_data[14]; // 14 bytes of protocol address
};
```

You'll almost never fill this one in by hand — that `sa_data` blob is deliberately opaque. Instead you fill in one of the two specific structs below, then **cast its pointer to `struct sockaddr *`** whenever a function (like `bind()` or `connect()`) asks for one. This works because all three structs are laid out to be the same size, and the first field of each (`sa_family` / `sin_family` / `sin6_family`) lines up — that's what lets a function that only understands `struct sockaddr` peek at `sa_family` and know which concrete type it's actually looking at.

### `struct sockaddr_in` — the IPv4-specific version

```c
struct sockaddr_in {
    short int          sin_family;  // AF_INET
    unsigned short int sin_port;    // port number, must be in Network Byte Order
    struct in_addr     sin_addr;    // the actual IPv4 address
    unsigned char      sin_zero[8]; // padding — always zero it with memset()
};

struct in_addr {
    uint32_t s_addr; // the 4-byte IP address, in Network Byte Order
};
```

Two things to remember every time you touch this struct: (1) `sin_port` needs `htons()` applied, and (2) `sin_zero` needs to be zeroed with `memset()` — it exists purely to pad this struct to the same size as `struct sockaddr`.

### `struct sockaddr_in6` — the IPv6 version

```c
struct sockaddr_in6 {
    u_int16_t       sin6_family;   // AF_INET6
    u_int16_t       sin6_port;     // port, Network Byte Order
    u_int32_t       sin6_flowinfo; // IPv6 flow info (ignore as a beginner)
    struct in6_addr sin6_addr;     // the actual IPv6 address
    u_int32_t       sin6_scope_id; // scope ID (ignore as a beginner)
};

struct in6_addr {
    unsigned char s6_addr[16]; // 16-byte IPv6 address
};
```

Structurally the same idea as the IPv4 version, just bigger (16 bytes of address instead of 4), and with two extra fields you can safely ignore until you need genuinely advanced IPv6 features.

### `struct sockaddr_storage` — "I don't know which one yet"

```c
struct sockaddr_storage {
    sa_family_t ss_family; // check this: AF_INET or AF_INET6?
    // ... padding, guaranteed big enough for either IPv4 or IPv6 ...
};
```

Some functions (most importantly `accept()`, in Chapter 5) fill in an address for you but don't know in advance whether it'll be IPv4 or IPv6. `sockaddr_storage` is guaranteed large enough to hold *either*. You pass a pointer to one of these, and afterward check `ss_family` to find out which kind you actually got, then cast accordingly. You'll see this exact pattern in `server.c` in the next chapter.

### The whole family tree, visually

```
                struct sockaddr           (generic — 16 bytes, sa_family + opaque data)
                       ▲  ▲
          "cast to"    │  │    "cast to"
                       │  │
    struct sockaddr_in │  │ struct sockaddr_in6
    (IPv4-specific)     │  │ (IPv6-specific)
                       │  │
    struct sockaddr_storage  (big enough to hold either — used when you don't know yet)
```

## 2.4 Converting addresses between text and binary

You'll type IP addresses as strings (`"192.0.2.1"`) but the structs above want them as packed binary numbers. Two functions handle the conversion (their names — "pton"/"ntop" — stand for "presentation to network" and "network to presentation," i.e. text ⇄ binary):

```c
// text -> binary  ("presentation to network")
struct sockaddr_in sa;
inet_pton(AF_INET, "10.12.110.57", &(sa.sin_addr));

// binary -> text  ("network to presentation")
char ip4[INET_ADDRSTRLEN];
inet_ntop(AF_INET, &(sa.sin_addr), ip4, INET_ADDRSTRLEN);
printf("The IPv4 address is: %s\n", ip4);
```

Both work for IPv4 (`AF_INET`) and IPv6 (`AF_INET6`) — just swap the family constant and use `sockaddr_in6`/`INET6_ADDRSTRLEN` instead. `inet_pton()` returns `-1` on a system error or `0` if the string just isn't a valid address, so always check the return value is `> 0` before trusting the result.

You'll meet two older, IPv4-only functions in existing code you might come across — `inet_addr()`/`inet_aton()` and `inet_ntoa()`. They're obsolete; don't use them in new code, since they don't understand IPv6. Also note: none of these four functions do a DNS lookup — they only convert already-numeric addresses. Turning a hostname like `"www.example.com"` into an address is `getaddrinfo()`'s job, coming up next chapter.

## 2.5 Jumping from IPv4-only code to IPv4/IPv6-agnostic code

The book's Chapter 4 is essentially a checklist for converting old IPv4-only code to work with both protocols. You won't need to do this conversion yourself much, since every program later in this guide is already written the "modern" way (via `getaddrinfo()` with `AF_UNSPEC`), but it's worth knowing the mapping, because you'll see the "old style" constantly in code you find online:

| Old (IPv4-only) | New (works for both, via `getaddrinfo()`) |
|---|---|
| `AF_INET` | `AF_INET6`, or better: `AF_UNSPEC` and let `getaddrinfo()` decide |
| `struct sockaddr_in` | `struct sockaddr_in6` |
| `struct in_addr` | `struct in6_addr` |
| `INADDR_ANY` | `in6addr_any` |
| `inet_aton()` / `inet_addr()` | `inet_pton()` |
| `inet_ntoa()` | `inet_ntop()` |
| `gethostbyname()` | `getaddrinfo()` |

---

## Quick recap (for revision)

- IPv4 = 4 bytes, dotted-decimal. IPv6 = 16 bytes, colon-hex, with `::` shorthand for runs of zeros. `127.0.0.1` and `::1` both mean "this machine."
- **Always** convert multi-byte numbers with `htons()`/`htonl()` before sending, `ntohs()`/`ntohl()` after receiving. This is the single most common silent bug in beginner socket code.
- Struct hierarchy: `sockaddr` (generic) ⟷ `sockaddr_in` (IPv4) / `sockaddr_in6` (IPv6) (specific, freely cast-able to the generic one) ⟷ `sockaddr_storage` (big enough for either, used when the type isn't known yet).
- `inet_pton()` / `inet_ntop()` convert address **text ⇄ binary** — they do *not* do DNS lookups.
- Write new code with `AF_UNSPEC` + `getaddrinfo()` so it transparently supports both IPv4 and IPv6, instead of hard-coding one.

**Common beginner mix-up:** forgetting to `memset()` a `sockaddr_in`/`hints` struct to zero before filling in a few fields. Uninitialized stack memory in the fields you *didn't* set (like `sin_zero`, or unset `addrinfo` flags) can cause `bind()`/`connect()` to mysteriously fail or misbehave — every example in this guide starts with `memset(&hints, 0, sizeof hints);` for exactly this reason.

Next: `03_Core_System_Calls.md` — where these structs finally get used for real.