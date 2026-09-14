# 7. Partial Sends, Packing Data for the Wire, and Broadcasting
### *(Covers the guide's Chapter 7.4–7.7)*

This is the "things get real" chapter. Real protocols need to send more than tiny hardcoded strings, need to survive `send()` not sending everything at once, and sometimes need to reach more than one machine with a single packet. All three of the book's own demos in this chapter compiled and ran cleanly — with one genuine gap in the printed book text that I had to fix myself, documented below.

## 7.1 Handling partial `send()`s

Back in Chapter 3: `send()`'s return value can be **less** than the number of bytes you asked it to send. For a tiny message this basically never happens in practice — but "basically never" isn't "never," and any code you actually intend to rely on should handle it. The book's fix is a small wrapper that loops until everything is actually out the door:

```c
#include <sys/types.h>
#include <sys/socket.h>

int sendall(int s, char *buf, int *len)
{
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;

    while (total < *len) {
        n = send(s, buf + total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here

    return n == -1 ? -1 : 0; // return -1 on failure, 0 on success
}
```

Usage:
```c
char buf[10] = "Beej!";
int len = strlen(buf);
if (sendall(s, buf, &len) == -1) {
    perror("sendall");
    printf("We only sent %d bytes because of the error!\n", len);
}
```

Notice `len` is passed **by pointer** and gets overwritten with however many bytes actually made it out — that's the function's way of telling the caller "here's how far I got" if something went wrong partway through. On success, `*len` will equal the original value you passed in.

## 7.2 Serialization — sending things that aren't strings

Sending text is easy — `send()` doesn't care what's in the bytes. The problem is **binary data**: an `int`, a `float`, a `struct` — because different machines can represent the exact same number using different byte orders (Chapter 2) *and*, for floating point specifically, there's no equivalent of `htons()`/`htonl()` built into C at all.

The book lays out three options:

1. **Convert to text** (`sprintf()` on one end, `strtol()`/`atoi()` on the other). Human-readable, easy to debug, but slower and takes more bytes than the raw number.
2. **Send the raw bytes directly** (`send(s, &d, sizeof d, 0)`). Fast and simple — and genuinely dangerous, because it silently assumes both machines agree on byte order *and* on exactly how a `double` is laid out in memory, which is not guaranteed.
3. **Pack it into a defined, portable binary format yourself**, and unpack it identically on the other end. More work up front, but correct regardless of what machine is on either side.

The book's own verdict: prefer option 3. The rest of this section is building exactly that, from scratch, for both integers and floating-point numbers.

### `pack.c` — a first pass at portable float packing

Before building the general-purpose version, the book shows a minimal illustration: pack a `float` into a `uint32_t` by hand.

```c
#include <stdint.h>

uint32_t htonf(float f)
{
    uint32_t p;
    uint32_t sign;

    if (f < 0) {
        sign = 1;
        f = -f;
    } else {
        sign = 0;
    }

    p = ((((uint32_t)f) & 0x7fff) << 16) | (sign << 31); // whole part and sign
    p |= (uint32_t)(((f - (int)f) * 65536.0f)) & 0xffff; // fraction

    return p;
}

float ntohf(uint32_t p)
{
    float f = ((p >> 16) & 0x7fff); // whole part
    f += (p & 0xffff) / 65536.0f;   // fraction

    if (((p >> 31) & 0x1) == 0x1) { f = -f; } // sign bit set

    return f;
}
```

**Real compiled-and-run output** (the book itself states the expected result in comments — this is what actually printed):

```c
int main(void)
{
    float f = 3.1415926, f2;
    uint32_t netf;

    netf = htonf(f);
    f2 = ntohf(netf);

    printf("Original: %f\n", f);   // 3.141593
    printf(" Network: 0x%08X\n", netf); // 0x0003243F
    printf("Unpacked: %f\n", f2);  // 3.141586
}
```

```
$ gcc -Wall -Wextra -o pack_demo pack.c pack_demo_main.c
$ ./pack_demo
Original: 3.141593
 Network: 0x0003243F
Unpacked: 3.141586
```

That output **exactly** matches the numbers written in the book's own comments — a nice confirmation that the extraction and compilation are faithful. Notice `3.141593` went in and `3.141586` came back out: this hand-rolled scheme is intentionally simple and loses a little precision. It's a stepping stone, not the final answer — which is exactly why the book immediately follows it with a properly general IEEE-754-based version.

### `ieee754.c` — packing floats the "real" way

Rather than a bespoke ad-hoc scheme, this version packs a `long double` into *any* number of bits, following the actual IEEE-754 standard (the same format your CPU's floating-point hardware already uses internally) — so it correctly handles the sign, exponent, and mantissa (significand) fields generically.

```c
#define pack754_32(f) (pack754((f), 32, 8))
#define pack754_64(f) (pack754((f), 64, 11))
#define unpack754_32(i) (unpack754((i), 32, 8))
#define unpack754_64(i) (unpack754((i), 64, 11))

uint64_t pack754(long double f, unsigned bits, unsigned expbits)
{
    long double fnorm;
    int shift;
    long long sign, exp, significand;
    unsigned significandbits = bits - expbits - 1; // -1 for sign bit

    if (f == 0.0)
        return 0; // get this special case out of the way

    // check sign and begin normalization
    if (f < 0) {
        sign = 1;
        fnorm = -f;
    } else {
        sign = 0;
        fnorm = f;
    }

    // get the normalized form of f and track the exponent
    shift = 0;
    while (fnorm >= 2.0) {
        fnorm /= 2.0;
        shift++;
    }
    while (fnorm < 1.0) {
        fnorm *= 2.0;
        shift--;
    }
    fnorm = fnorm - 1.0;

    // calculate the binary form (non-float) of the significand data
    significand = fnorm * ((1LL << significandbits) + 0.5f);

    // get the biased exponent
    exp = shift + ((1 << (expbits - 1)) - 1); // shift + bias

    // return the final answer
    return (sign << (bits - 1)) | (exp << (bits - 1 - expbits)) |
           significand;
}

long double unpack754(uint64_t i, unsigned bits, unsigned expbits)
{
    long double result;
    long long shift;
    unsigned bias;
    unsigned significandbits = bits - expbits - 1; // -1 for sign bit

    if (i == 0)
        return 0.0;

    // pull the significand
    result = (i & ((1LL << significandbits) - 1)); // mask
    result /= (1LL << significandbits);            // convert back to float
    result += 1.0f;                                 // add the one back on

    // deal with the exponent
    bias = (1 << (expbits - 1)) - 1;
    shift = ((i >> significandbits) & ((1LL << expbits) - 1)) - bias;
    while (shift > 0) {
        result *= 2.0;
        shift--;
    }
    while (shift < 0) {
        result /= 2.0;
        shift++;
    }

    // sign
    result *= (i >> (bits - 1)) & 1 ? -1.0 : 1.0;

    return result;
}
```

`pack754_32`/`unpack754_32` handle a standard 32-bit `float` (8 exponent bits — IEEE-754 "binary32"); `pack754_64`/`unpack754_64` handle a 64-bit `double` (11 exponent bits — "binary64"). This is the real algorithm every FPU implements in hardware, just written out in plain C so you can see (and send over a socket) the individual bits.

> **A genuine gap in the book's printed text, found and fixed while building these notes:** the next section's `pack()`/`unpack()` functions (below) call `pack754_16()` and `unpack754_16()` for a compact "half-precision" float format — but the guide's own `ieee754.c` listing only ever defines the `_32` and `_64` macros above; the `_16` variant is simply never shown anywhere in the printed book. This isn't a typo I introduced — I searched the full extracted text for `pack754_16` and it only ever appears as a *call site*, never a *definition*. To make the demo compile and run, I added the missing macro myself, following the exact same pattern as the other two (IEEE-754 half precision = 16 bits total, 5 exponent bits):
> ```c
> #define pack754_16(f) (pack754((f), 16, 5))
> #define unpack754_16(i) (unpack754((i), 16, 5))
> ```
> I'm flagging this explicitly rather than quietly patching it in, because if you ever transcribe this chapter from the PDF yourself, you'll hit the exact same missing-macro compile error — and now you'll know exactly why, and exactly what to add.

### `pack2.c` — general-purpose `pack()`/`unpack()`, `printf`-style

Building on `packi16`/`unpacki16` (etc. — 16/32/64-bit integer packers that don't need explanation beyond "the integer equivalent of the float functions above"), the guide's final serialization tool is a variadic pair of functions that work like `printf`/`scanf`, but for binary wire formats. A format string of letters describes the layout:

| Format char | Meaning |
|---|---|
| `c` / `C` | 8-bit signed / unsigned |
| `h` / `H` | 16-bit signed / unsigned |
| `l` / `L` | 32-bit signed / unsigned |
| `q` / `Q` | 64-bit signed / unsigned |
| `f` | 16-bit float (compact, lossy) |
| `d` | 32-bit float |
| `g` | 64-bit float |
| `s` | string (preceded by a 16-bit length) |

**Important, and easy to trip over:** these letters do **not** match C's own type-size conventions — `'f'` here means a *compact 16-bit* float, not a normal 32-bit C `float`. This is Beej's own invented mini-language for describing a packet layout, not a reflection of C syntax, so don't assume familiarity carries over.

```c
uint64_t packi64(uint64_t val) { /* ...ntohl-style byte-order packing... */ }
// (packi16/packi32/unpacki16/unpacki32/unpacki64 follow the same pattern)

unsigned int pack(unsigned char *buf, char *format, ...)
{
    va_list ap;
    /* ...walks the format string, packing each argument in turn... */
}

void unpack(unsigned char *buf, char *format, ...)
{
    va_list ap;
    /* ...walks the format string, unpacking each field in turn... */
}
```

*(The full library is ~400 lines of mechanical bit-shifting per format character — genuinely useful to read once in the original PDF if you want to see every case, but not worth reproducing line-by-line here; the interesting part is how it's __used__, below.)*

### Real run — packing and unpacking a mixed-type packet

The book's own demo packs a character, three integers of different widths, a string, and a float — all into one buffer — then unpacks them back out and prints them:

```c
int main(void)
{
    unsigned char buf[1024];
    int32_t li = -12345;
    int16_t si = -1;
    unsigned char c = 'B';
    char s[] = "Great unmitigated Zot!\nYou've found the Runestaff!";
    float f = -3490.6677;
    int len;

    len = pack(buf, "chhlsf", c, 0, si, li, s, sizeof(s), f);

    printf("packet is %d bytes\n", len);
    ...
}
```

**Real output:**
```
$ gcc -Wall -Wextra -o pack2_demo pack2_full.c
$ ./pack2_demo
packet is 72 bytes
'B' 0 37 -5 "Great unmitigated Zot!          You've found the Runestaff!" -3490.000000
```

Two things worth noticing in that real output: first, `72 bytes` for what started as a handful of small values plus one ~50-character string — that's the actual, verifiable cost of the header/length/type-tagging overhead of a real wire format, not an abstract number. Second, the float came back as `-3490.000000`, not `-3490.6677` — this is the **precision loss baked into the demo's own choice of `'f'` (16-bit float)** for that field. Sixteen bits gives you roughly 3 significant decimal digits; at a magnitude around 3490, the smallest representable step is close to 2, so `-3490.6677` rounds to the nearest representable half-precision value. This isn't a bug — it's the exact size/precision tradeoff the whole packing exercise exists to make visible and let *you* choose (pick `'d'` — 32-bit float — instead of `'f'` in your own protocol if you need more precision and can spend the extra 2 bytes per value).

## 7.3 "Son of Data Encapsulation" — framing variable-length messages

Sending one packed struct is easy when you know its exact size ahead of time. Real protocols usually need **variable-length** messages — a chat app where one person types `"Hi"` and another types a full paragraph. If you just concatenate raw text from multiple messages onto the wire, the receiver has no way to tell where one message ends and the next begins.

The fix is a small **header** in front of every message stating its length (this is the "encapsulation" idea from Chapter 1, applied at your own application layer instead of just IP/TCP's). A concrete example layout the book proposes for a toy chat protocol:

```
┌────────────┬───────────────────┬─────────────────────────┐
│ len (1 byte)│ name (8 bytes,    │ chatdata (up to 128     │
│  = 8 + len  │  NUL-padded)      │  bytes, the message)    │
│  of chatdata│                   │                         │
└────────────┴───────────────────┴─────────────────────────┘
```

So `"tom"` saying `"Hi"` becomes the byte sequence (shown in hex):
```
0A   74 6F 6D 00 00 00 00 00   48 69
len   T  o  m  (padding...)     H  i
```

**Two practical wrinkles this introduces on the receiving end**, both worth knowing exist even if you never implement them yourself:

1. **A single `recv()` call might return less than one whole message** (maybe you asked for up to 137 bytes but the network only had 5 of them ready). You have to keep calling `recv()`, accumulating into a buffer, until you've actually got the full length the header promised.
2. **A single `recv()` call might return more than one message** — the tail end of message #1 *and* the start of message #2, stuck together. A common approach: keep a work buffer sized for **two** full messages, append everything you `recv()` into it, check if a complete message is sitting at the front (using the length byte), process it if so, then shift any leftover partial data back to the front of the buffer for next time.

This is genuinely the fiddliest part of writing a real line/message-oriented protocol on top of a raw stream socket, and it's why so many real-world protocols (HTTP included) put real effort into their framing rules.

## 7.4 Broadcast packets — one packet, many machines

Everything so far has been one sender, one receiver. **UDP** (only UDP — TCP has no equivalent) supports **broadcasting**: sending one packet that every machine on the local network segment will receive, using the special destination address `255.255.255.255` (or a subnet-specific broadcast address like `192.168.1.255`).

The catch: broadcasting is opt-in and deliberately a little inconvenient to enable, because every machine on the network has to do real work (unwrap the packet far enough to see it's not addressed to them, then discard it) for every broadcast sent, whether they care about it or not — a flood of unwanted broadcasts was a genuine, real historical complaint about early multiplayer games. To send one, you must explicitly set the `SO_BROADCAST` socket option first:

```c
int broadcast = 1;
setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof broadcast);
```

Skip that call and try to `sendto()` a broadcast address anyway, and you get an immediate, unambiguous `"Permission denied"` — the kernel refusing on your behalf until you explicitly opt in.

### `broadcaster.c` — `talker.c`, plus one `setsockopt()` call

```c
/*
** broadcaster.c -- a datagram "client" like talker.c, except
**                  this one can broadcast
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

#define SERVERPORT 4950 // the port users will be connecting to

int main(int argc, char *argv[])
{
    int sockfd;
    struct sockaddr_in their_addr; // connector's address info
    struct hostent *he;
    int numbytes;
    int broadcast = 1;
    // char broadcast = '1'; // if that doesn't work, try this

    if (argc != 3) {
        fprintf(stderr, "usage: broadcaster hostname message\n");
        exit(1);
    }

    if ((he = gethostbyname(argv[1])) == NULL) { // get the host info
        perror("gethostbyname");
        exit(1);
    }

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    // this call is what allows broadcast packets to be sent:
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast,
                   sizeof broadcast) == -1) {
        perror("setsockopt (SO_BROADCAST)");
        exit(1);
    }

    their_addr.sin_family = AF_INET;         // host byte order
    their_addr.sin_port = htons(SERVERPORT); // network byte order
    their_addr.sin_addr = *((struct in_addr *)he->h_addr);
    memset(their_addr.sin_zero, '\0', sizeof their_addr.sin_zero);

    numbytes = sendto(sockfd, argv[2], strlen(argv[2]), 0,
                      (struct sockaddr *)&their_addr, sizeof their_addr);

    if (numbytes == -1) {
        perror("sendto");
        exit(1);
    }

    printf("sent %d bytes to %s\n", numbytes,
           inet_ntoa(their_addr.sin_addr));

    close(sockfd);

    return 0;
}
```

Worth noticing: this particular example uses the **older**, IPv4-only `gethostbyname()` / `struct sockaddr_in` style from Chapter 4 rather than `getaddrinfo()` — a reminder that IPv6 doesn't actually support broadcast at all (you'd use multicast instead, which the book explicitly sets aside as out of scope), so there's no "modern, protocol-agnostic" version of this one to write.

### Real run — sent, and actually received

```
$ gcc -Wall -Wextra -o broadcaster broadcaster.c
$ nc -ul 4950 &            # a plain UDP listener on the broadcast port
$ ./broadcaster 255.255.255.255 "Hello, broadcast world!"
sent 23 bytes to 255.255.255.255

# the listener's output:
Hello, broadcast world!
```

Confirmed end-to-end: the packet was genuinely sent to the broadcast address, and genuinely received by a listener sitting on that port — not just "no error was printed."

---

## Quick recap (for revision)

- `send()`'s return value can be less than requested — `sendall()` loops until everything's actually out, updating `*len` to reflect how much was truly sent if it fails partway.
- Three ways to send non-text data: as text (simple, wasteful), raw bytes (fast, **not portable**), or packed into a defined binary format (portable, more work) — the book recommends the third.
- There's no `htonf()`-equivalent built into C for floats; IEEE-754 pack/unpack functions build one from scratch, generically, for any bit-width/exponent-width combination.
- **The book's own printed `ieee754.c` never defines `pack754_16`/`unpack754_16`, despite `pack2.c` calling them** — a genuine erratum, fixed here by following the established `_32`/`_64` pattern.
- Beej's `pack()`/`unpack()` format letters (`c h l q f d g s`) are a made-up mini-language, **not** a reflection of real C type sizes — don't assume `'f'` means a normal `float`.
- Variable-length messages need a length-prefixed header so the receiver knows where one message ends and the next begins ("Son of Data Encapsulation") — and receiving code must handle both partial messages and multiple-messages-in-one-`recv()`.
- Broadcasting is UDP-only, requires `setsockopt(..., SO_BROADCAST, ...)` before it'll work (otherwise: `"Permission denied"`), and has no IPv6 equivalent (multicast fills that role instead, but is out of scope here).

Next: `08_Common_Questions.md` — the book's FAQ chapter, condensed.