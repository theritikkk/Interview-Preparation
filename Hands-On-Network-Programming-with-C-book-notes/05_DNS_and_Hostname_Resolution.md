# 5. DNS and Hostname Resolution
### *(Covers the book's Chapter 5 — "Hostname Resolution and DNS")*

Every program in the last three chapters called `getaddrinfo()` and trusted it to "just work." This chapter opens the hood: first by using the friendlier, higher-level functions properly, then by hand-building a real DNS query packet and firing it at a real DNS server, byte by byte.

## 5.1 `getaddrinfo()`, used properly — and a genuine gotcha

You've been calling `getaddrinfo()` for four chapters, always with `hints.ai_socktype` set to either `SOCK_STREAM` or `SOCK_DGRAM`. What happens if you *don't* set it?

```c
#include "chap05.h"

#ifndef AI_ALL
#define AI_ALL 0x0100
#endif

int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("Usage:\n\tlookup hostname\n");
        printf("Example:\n\tlookup example.com\n");
        exit(0);
    }

    printf("Resolving hostname '%s'\n", argv[1]);
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_ALL;
    struct addrinfo *peer_address;
    if (getaddrinfo(argv[1], 0, &hints, &peer_address)) {
        fprintf(stderr, "getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    printf("Remote address is:\n");
    struct addrinfo *address = peer_address;
    do {
        char address_buffer[100];
        getnameinfo(address->ai_addr, address->ai_addrlen,
                address_buffer, sizeof(address_buffer),
                0, 0,
                NI_NUMERICHOST);
        printf("\t%s\n", address_buffer);
    } while ((address = address->ai_next));

    freeaddrinfo(peer_address);
    return 0;
}
```

**Real output — and the gotcha, live:**
```
$ gcc -Wall -o lookup lookup.c
$ ./lookup localhost
Resolving hostname 'localhost'
Remote address is:
	127.0.0.1
	127.0.0.1
	127.0.0.1

$ ./lookup github.com
Resolving hostname 'github.com'
Remote address is:
	140.82.114.4
	140.82.114.4
	140.82.114.4
```

The *same* address printed three times isn't a bug in this program — it's `getaddrinfo()` doing exactly what it was asked. Leaving `hints.ai_socktype` at `0` means "any socket type is fine," and DNS resolution genuinely doesn't distinguish TCP from UDP from raw at all (that distinction is a Transport-layer concept — DNS only resolves names to addresses, an Internet-layer concern). So `getaddrinfo()` faithfully returns one result *per combination* of address and usable socket type — here, one address, times three plausible socket types (stream, datagram, raw) it's willing to hand back, giving you three structurally distinct `addrinfo` entries that happen to describe the same address. **The fix is exactly what every earlier chapter already did without explaining why**: set `hints.ai_socktype` to the one type you actually intend to use, and this triples-up-everything behavior disappears. Worth remembering the next time `getaddrinfo()` seems to be returning "duplicate" results — check whether you actually constrained the socket type.

## 5.2 The DNS protocol, by hand

Here's the ambitious part of this chapter: rather than trusting `getaddrinfo()` as a black box, build the actual DNS query **wire format** yourself, byte by byte, send it over a plain UDP socket to a real DNS server, and manually parse whatever comes back.

### The message format, in brief

Every DNS message — query or response — starts with a fixed 12-byte header:

```
 Byte:   0    1    2    3    4    5    6    7    8    9   10   11
        ┌────────┬─────────┬──────────┬──────────┬──────────┬──────────┐
        │   ID   │ flags   │ QDCOUNT  │ ANCOUNT  │ NSCOUNT  │ ARCOUNT  │
        └────────┴─────────┴──────────┴──────────┴──────────┴──────────┘
```
- **ID**: any 16-bit value you pick — echoed back in the response, so you can match responses to the query that caused them (useful if you have several outstanding at once).
- **Flags** (2 bytes, individual bits): whether this is a query or response (`QR`), the operation type (`OPCODE`), whether the answer is authoritative (`AA`), whether the message got truncated (`TC`), whether you're asking for recursive resolution (`RD`), and (in responses) a result code (`RCODE` — `0` = success, others indicate various failures).
- **QDCOUNT / ANCOUNT / NSCOUNT / ARCOUNT**: how many entries follow in each of the four sections — Questions, Answers, Name-server records, and Additional records.

After the header comes the **question section** (the hostname you're asking about, plus the record type you want — `A` for an IPv4 address, `AAAA` for IPv6, `MX` for mail servers, `TXT` for arbitrary text, and so on), and then, in a response, the **answer section** with the actual results.

### Building the query, one byte at a time

```c
char query[1024] = {0xAB, 0xCD, /* ID */
                    0x01, 0x00, /* Set recursion */
                    0x00, 0x01, /* QDCOUNT */
                    0x00, 0x00, /* ANCOUNT */
                    0x00, 0x00, /* NSCOUNT */
                    0x00, 0x00 /* ARCOUNT */};

char *p = query + 12;
char *h = argv[1];

while(*h) {
    char *len = p;
    p++;
    if (h != argv[1]) ++h;

    while(*h && *h != '.') *p++ = *h++;
    *len = p - len - 1;
}

*p++ = 0;
*p++ = 0x00; *p++ = type; /* QTYPE */
*p++ = 0x00; *p++ = 0x01; /* QCLASS */

const int query_size = p - query;
```

The header is just a literal byte array — `0xAB, 0xCD` for the ID (an arbitrary but memorable choice), `0x01, 0x00` sets the "recursion desired" flag bit and nothing else, and `QDCOUNT = 1` (we're asking exactly one question) with everything else zero. Then comes the genuinely clever bit: DNS encodes a hostname not as a plain string with dots, but as a sequence of **length-prefixed labels** — `example.com` becomes `\x07example\x03com\x00` (7 bytes of "example", 3 bytes of "com", then a zero byte marking the end). The `while(*h)` loop is walking the hostname one label at a time, writing a placeholder length byte, then backfilling it once it knows how many characters that label turned out to be (`*len = p - len - 1;`).

### Parsing the response — including the compression trick

Real DNS responses use a space-saving trick: instead of repeating a hostname that already appeared earlier in the message, they insert a 2-byte **pointer** back to where it first appeared. This function has to understand that to print anything correctly:

```c
const unsigned char *print_name(const unsigned char *msg,
        const unsigned char *p, const unsigned char *end) {

    if (p + 2 > end) {
        fprintf(stderr, "End of message.\n"); exit(1);}

    if ((*p & 0xC0) == 0xC0) {
        const int k = ((*p & 0x3F) << 8) + p[1];
        p += 2;
        printf(" (pointer %d) ", k);
        print_name(msg, msg+k, end);
        return p;

    } else {
        const int len = *p++;
        if (p + len + 1 > end) {
            fprintf(stderr, "End of message.\n"); exit(1);}

        printf("%.*s", len, p);
        p += len;
        if (*p) {
            printf(".");
            return print_name(msg, p, end);
        } else {
            return p+1;
        }
    }
}
```

The top two bits of a length byte being `11` (checked with `(*p & 0xC0) == 0xC0`) is the signal "this isn't a length, it's a pointer" — the remaining 14 bits give an offset from the *start* of the whole message (`msg`, not the current position) to jump to and recursively print from there. This function calls itself recursively both for "there's another label after this one" and for "this is actually a pointer, follow it" — a nice small example of recursion doing real, practical work rather than being a textbook toy.

The rest of `print_dns_message()` (not reproduced in full here — it's mostly repetitive bit-shifting to pull 16- and 32-bit fields like TTL, record type, and record length out of the byte stream, then a `switch`-like chain of `if`s to format each record type — `A`, `AAAA`, `MX`, `TXT`, `CNAME` — appropriately) walks the header counts, then the question section, then every answer/authority/additional record, printing each field as it goes.

## 5.3 Running it against a real DNS server, live

This program sends its hand-built query straight to `8.8.8.8` (Google's public DNS resolver) on port 53, over plain UDP:

```c
if (getaddrinfo("8.8.8.8", "53", &hints, &peer_address)) { ... }
/* ...build query as above... */
sendto(socket_peer, query, query_size, 0, peer_address->ai_addr, peer_address->ai_addrlen);
print_dns_message(query, query_size);

char read[1024];
int bytes_received = recvfrom(socket_peer, read, 1024, 0, 0, 0);
print_dns_message(read, bytes_received);
```

**Real, live query for an `A` record:**
```
$ gcc -Wall -o dns_query dns_query.c
$ ./dns_query example.com a
Configuring remote address...
Creating socket...
Sent 29 bytes.
ID = AB CD
QR = 0 query
OPCODE = 0 standard
AA = 0
TC = 0
RD = 1 recursion desired
QDCOUNT = 1
ANCOUNT = 0
NSCOUNT = 0
ARCOUNT = 0
Query  1
  name: example.com
  type: 1
 class: 1

Received 61 bytes.
ID = AB CD
QR = 1 response
OPCODE = 0 standard
AA = 0
TC = 0
RD = 1 recursion desired
RCODE = 0 success
QDCOUNT = 1
ANCOUNT = 2
NSCOUNT = 0
ARCOUNT = 0
Query  1
  name: example.com
  type: 1
 class: 1
Answer  1
  name:  (pointer 12) example.com
  type: 1
 class: 1
   ttl: 300
 rdlen: 4
Address 104.20.23.154
Answer  2
  name:  (pointer 12) example.com
  type: 1
 class: 1
   ttl: 300
 rdlen: 4
Address 172.66.147.243
```

A few things worth pointing out in this genuinely real response: the **ID field comes back exactly as sent** (`AB CD`) — confirming this response actually answers *this* query, not some other one; the answer's name is `(pointer 12)` — the server, rather than repeating `example.com` again, points back to byte offset 12, right where the question section's copy of the name lives, exactly the compression trick from §5.2; and `example.com` resolves to **two** separate `A` records — this domain is genuinely served from more than one IP address (nothing to do with the earlier `getaddrinfo()` triple-counting gotcha — this really is two distinct addresses, both type `A`, both class `1`).

**And a real `MX` (mail server) query, for variety:**
```
$ ./dns_query google.com mx
...
Answer  1
  name:  (pointer 12) google.com
  type: 15
 class: 1
   ttl: 179
 rdlen: 9
  pref: 10
MX: smtp. (pointer 12) google.com
```
`smtp.` followed by a pointer back to `google.com` — the mail server's full name, `smtp.google.com`, reconstructed from a literal label plus a compression pointer, exactly the mixed case `print_name()`'s recursive design handles.

---

## Quick recap (for revision)

- Leaving `hints.ai_socktype` at `0` in `getaddrinfo()` returns one result per usable socket type for each address — looks like duplicates, isn't a bug, fixed by setting the socket type you actually want.
- A DNS message is a fixed 12-byte header (ID, flags, four section counts) followed by however many question/answer/authority/additional entries those counts promise.
- Hostnames on the wire are length-prefixed label sequences (`\x07example\x03com\x00`), **not** plain dotted strings.
- DNS responses compress repeated names into 2-byte back-pointers (top two bits of the length byte set to `11`) rather than repeating them — parsing code has to check for this on every name it reads.
- The response's ID field should match what you sent — your one guarantee that a UDP response actually corresponds to your query, given UDP itself has no built-in request/response linkage.

Next: `06_HTTP_Client.md` — using everything so far to build a real HTTP client from raw sockets.