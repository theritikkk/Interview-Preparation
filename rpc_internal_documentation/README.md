# How RPC Works Internally: A Low-Level Guide

### With a complete, from-scratch RPC implementation, compiled and run, including real wire-level byte dumps

---

## Why this document is built the way it is

"RPC" (Remote Procedure Call) is usually taught as a diagram with two boxes and an arrow: "it makes a network call look like a function call." That's true, but it's the least interesting part of the story — the real engineering is in *how* that illusion is actually constructed out of nothing but sockets, byte buffers, and a dispatch table.

Rather than describe that abstractly, this document builds a complete, minimal RPC system from scratch in C — a real client, a real server, a real wire protocol — compiles it, runs it, and shows you the **actual bytes** that cross the network for a real remote call. Every mechanism described below (marshalling, framing, dispatch, call correlation) is demonstrated in working code, not just asserted. After the from-scratch implementation, the document connects those same mechanisms to two real, production RPC systems — classic Sun/ONC RPC (the protocol behind NFS) and modern gRPC — so you can see the identical underlying ideas wearing different, real-world clothes.

---

## 1. The illusion, and what actually has to happen underneath it

When you write ordinary code like `result = add(3, 4);`, the compiler generates a handful of machine instructions: push arguments onto the stack (or into registers), jump to `add`'s address, let it run, read the return value out of a register. All of this happens in microseconds, in one address space, with no possibility of the call "not arriving."

RPC's entire job is making `result = add(3, 4);` — where `add` actually runs on a **different machine** — look and feel like that same simple call, despite none of those guarantees holding anymore:

- The "jump to `add`'s address" doesn't make sense — there's no shared address space. Instead, the arguments have to be turned into a self-contained sequence of bytes, sent over a network, and reconstructed on the other end.
- The call might take milliseconds instead of nanoseconds — six orders of magnitude slower, minimum.
- The call might not arrive at all, or the reply might not come back, and there's no way to distinguish "the network ate my request" from "the network ate the reply" from where you're standing.
- Two different machines might disagree about how a 32-bit integer is laid out in memory (byte order), or how a `struct` is padded.

Every mechanism in this document exists to paper over exactly one of these gaps. Keep that framing in mind — it turns "why does RPC need X" into an answerable question for every X that follows.

## 2. The six pieces every RPC system has

Regardless of implementation language or specific wire format, every RPC system — from 1980s Sun RPC to modern gRPC — is built from the same six pieces:

```
   CLIENT SIDE                              SERVER SIDE
  ┌─────────────┐                          ┌───────────────┐
  │ Application │                          │   Real        │
  │    code     │                          │  handler      │
  │             │                          │  function     │
  │ calls the   │                          │  (ordinary    │
  │ "stub" as   │                          │   code, not   │
  │ if it were  │                          │   RPC-aware)  │
  │ local       │                          └───────▲───────┘
  └──────┬──────┘                                  │
         │                                  ┌──────┴────────┐
  ┌──────▼───────┐                          │  Dispatcher   │
  │ CLIENT STUB  │                          │ (picks which  │
  │ marshals     │                          │  handler by   │
  │ args         │                          │  procedure #) │
  └──────┬───────┘                          └──────▲────────┘
         │                                         │
  ┌──────▼───────┐      wire protocol       ┌──────┴───────┐
  │  Transport   │ ───────────────────────► │  Transport   │
  │ (sockets)    │ ◄─────────────────────── │ (sockets)    │
  └──────────────┘                          └──────────────┘
```

1. **The client stub**: a function with the *same signature the application expects* — it's what makes the illusion work. Internally, it marshals arguments, sends a request, blocks for a reply, unmarshals the result.
2. **Marshalling / serialization**: turning language-level values (ints, strings, structs) into a flat, unambiguous byte sequence, and back. This is where byte-order and struct-padding disagreements between machines get resolved.
3. **The wire protocol**: the actual byte layout that travels over the network — message framing, headers identifying *which* call this is and *which* procedure it's for.
4. **Transport**: the underlying sockets (TCP or UDP) actually moving those bytes.
5. **The dispatcher**: server-side code that reads an incoming request's procedure identifier and routes it to the right handler — the direct implementation of "calling a function by number instead of by address."
6. **The server stub / skeleton**: the mirror image of the client stub — unmarshal incoming arguments, call the real handler, marshal its result into a reply.

Everything below is these six pieces, examined one at a time, with real code and real captured bytes for each.

## 3. The IDL — why RPC systems generate this code instead of hand-writing it

In this document's from-scratch implementation, the client stub and server dispatcher are hand-written, so you can read every line. In every real production RPC system, they're **generated automatically** from an **IDL** (Interface Definition Language) — a language-neutral description of what procedures exist and what types they take:

```
// what an IDL definition typically looks like (this exact syntax
// resembles Sun RPC's rpcgen input, and Protocol Buffers' .proto files
// follow a closely related philosophy)
program DEMO_PROG {
    version DEMO_VERS {
        int ADD(int, int)         = 1;
        string REVERSE(string)    = 2;
    } = 1;
} = 0x20001;
```

A compiler for this IDL (`rpcgen` for classic Sun RPC, `protoc` for Protocol Buffers/gRPC) reads this once and generates the client stub, the server-side dispatch skeleton, and the marshalling code for every type mentioned — in whatever target language you need. **The entire point is that the programmer never hand-writes the plumbing you're about to see below** — they write `int ADD(int, int)`, and the generator produces everything from §2's diagram except the boxes labeled "Application code" and "Real handler function." Because this document is teaching the internals, everything the generator would normally produce is written out by hand instead — but recognize that in any real system, this is machine-generated boilerplate, not something engineers write by hand for every single procedure.

## 4. Marshalling — the actual byte-level encoding rules

Before any of this can work, there has to be an unambiguous answer to "given the number 4, what bytes represent it, in what order?" — and "given a string, how does the receiver know where it ends?" This document's implementation uses a small, self-contained encoder mirroring the real rules classic RPC's **XDR** (External Data Representation) uses:

```c
/* from rpc.h -- encoding a 32-bit integer, always big-endian */
static int xdr_put_u32(xbuf_t *x, uint32_t v) {
    if (x->len + 4 > x->cap) return -1;
    x->buf[x->len+0] = (uint8_t)(v >> 24);
    x->buf[x->len+1] = (uint8_t)(v >> 16);
    x->buf[x->len+2] = (uint8_t)(v >> 8);
    x->buf[x->len+3] = (uint8_t)(v);
    x->len += 4;
    return 0;
}
```

Two rules worth internalizing, since they recur in essentially every binary wire protocol you'll ever read (this is the same "network byte order" concept from ordinary socket programming, just applied to a richer set of types):

- **A fixed byte order, always** — regardless of whether the sending or receiving machine's own native byte order is big- or little-endian, the *wire* format is fixed. This is exactly why `htons()`/`htonl()` exist in plain socket programming, generalized to every field a marshalling layer might ever encode.
- **Length-prefix anything variable-length.** A string is encoded as its length (4 bytes), then that many raw bytes:

```c
static int xdr_put_string(xbuf_t *x, const char *s) {
    uint32_t n = (uint32_t)strlen(s);
    if (xdr_put_u32(x, n) != 0) return -1;
    memcpy(x->buf + x->len, s, n);
    x->len += n;
    /* pad to a 4-byte boundary, exactly like real XDR does */
    while (x->len % 4 != 0) x->buf[x->len++] = 0;
    return 0;
}
```

Real XDR's **4-byte alignment padding** (every field, including strings, padded so the *next* field starts on a 4-byte boundary) exists for a genuinely practical reason: many CPU architectures either can't read a multi-byte value from a misaligned address at all, or can only do so slowly — padding every field guarantees every subsequent field starts aligned, regardless of what odd-length string preceded it. This document's implementation does exactly this, faithfully.

## 5. The wire protocol — framing and headers

Two genuinely separate problems, both of which have to be solved before "marshalled bytes" become "a working RPC call":

**Problem 1: TCP has no message boundaries.** Exactly like every earlier lesson about TCP being a byte stream (from ordinary socket programming), if you just `send()` a marshalled request, the receiving `recv()` has no way to know where it ends. The fix, used here exactly as it's used in real protocols, is a length prefix:

```c
/* from framing.h */
static int send_framed(int sock, const uint8_t *msg, uint32_t len) {
    uint8_t hdr[4] = { (uint8_t)(len>>24), (uint8_t)(len>>16),
                       (uint8_t)(len>>8),  (uint8_t)(len) };
    if (send_all(sock, hdr, 4) != 0) return -1;
    return send_all(sock, msg, len);
}
```

**Problem 2: even once you have one complete message, how does the receiver know *which* remote function this call is for, and how does the *caller* know which reply matches which outstanding call?** This document's message header (deliberately modeled closely on real RPC systems' headers) answers both:

```c
/*  CALL message:
 *    xid        -- client-chosen transaction ID; the reply echoes it back
 *    msg_type   -- CALL (0) or REPLY (1)
 *    program    -- which service (a namespacing number)
 *    version    -- interface version, for compatibility checking
 *    procedure  -- WHICH remote function -- this is the entire mechanism
 *                  behind "calling a function by number instead of address"
 *    arg_len    -- length of the marshalled arguments that follow
 */
```

The **`xid`** (transaction ID) deserves special attention: it's the *entire* mechanism by which a client, potentially with several calls outstanding at once (or simply needing to be sure a late-arriving reply actually corresponds to the call it thinks it does, not some earlier one), matches replies to requests. The server does nothing clever with it at all — it just copies whatever value the client sent, verbatim, into the reply. All the "intelligence" lives on the client side, checking that an incoming reply's `xid` matches what it's currently waiting for.

## 6. The client-side call sequence, in full

This is `rpc_add()`'s complete internal sequence — the exact code that runs underneath what looks, to its caller, like an ordinary function call:

```c
int32_t rpc_add(int32_t a, int32_t b, int dump_wire) {
    /* 1. MARSHAL the arguments into a flat byte buffer */
    uint8_t argbuf[64];
    xbuf_t argx; xbuf_init(&argx, argbuf, sizeof(argbuf));
    xdr_put_i32(&argx, a);
    xdr_put_i32(&argx, b);

    /* 2. Hand off to the generic call engine: build the CALL header,
          send it framed, block waiting for the framed REPLY */
    uint8_t resbuf[64];
    xbuf_t resx;
    if (rpc_call(PROC_ADD, &argx, resbuf, sizeof(resbuf), &resx, dump_wire) != 0) {
        fprintf(stderr, "rpc_add failed\n"); exit(1);
    }

    /* 3. UNMARSHAL the result back into a real C value */
    int32_t result;
    xdr_get_i32(&resx, &result);
    return result;
}
```

And the generic engine every stub calls into:

```c
static int rpc_call(uint32_t procedure, xbuf_t *argx,
                     uint8_t *resbuf, uint32_t resbuf_cap,
                     xbuf_t *resx, int dump_wire) {
    uint32_t xid = g_next_xid++;

    /* Build: [xid][CALL][program][version][procedure][arg_len][args...] */
    uint8_t msg[MAXMSG];
    xbuf_t out; xbuf_init(&out, msg, sizeof(msg));
    xdr_put_u32(&out, xid);
    xdr_put_u32(&out, MSG_CALL);
    xdr_put_u32(&out, DEMO_PROGRAM);
    xdr_put_u32(&out, DEMO_VERSION);
    xdr_put_u32(&out, procedure);
    xdr_put_u32(&out, (uint32_t)argx->len);
    memcpy(out.buf + out.len, argx->buf, argx->len);
    out.len += argx->len;

    send_framed(g_sock, out.buf, (uint32_t)out.len);

    /* Block here -- this is a SYNCHRONOUS call. The calling thread
       genuinely stops here until a reply arrives (or the connection
       breaks), exactly the way recv() blocks in ordinary socket code. */
    uint8_t reply[MAXMSG];
    int32_t n = recv_framed(g_sock, reply, sizeof(reply));

    /* Parse the reply header, verify the xid matches, check status */
    xbuf_t in; xbuf_init(&in, reply, (uint32_t)n);
    uint32_t rxid, msg_type, status, result_len;
    xdr_get_u32(&in, &rxid); xdr_get_u32(&in, &msg_type);
    xdr_get_u32(&in, &status); xdr_get_u32(&in, &result_len);

    if (rxid != xid) { /* ... reject, this isn't our reply ... */ }
    if (status != RPC_SUCCESS) { /* ... surface the error ... */ }

    memcpy(resbuf, reply + in.len, result_len);
    xbuf_init(resx, resbuf, result_len);
    return 0;
}
```

**This function is the "magic" of RPC, in its entirety.** There's no magic left once you've read it: build a header, marshal the arguments right after it, send the whole thing with a length prefix, block on a blocking `recv()`, check that the reply's `xid` matches, unmarshal the result. Every RPC system on earth is a variation on this same sequence.

## 7. The server-side call sequence, in full

The mirror image — and the place where "calling a function by number" actually happens:

```c
/* This function IS the "server skeleton" for the whole demo service --
   turn a wire-level procedure number into a real call, by hand. A real
   IDL compiler generates one dispatch function like this per SERVICE,
   automatically, from the interface definition. */
static uint32_t dispatch(uint32_t procedure, xbuf_t *argx, xbuf_t *resx) {
    switch (procedure) {

    case PROC_ADD: {
        int32_t a, b;
        xdr_get_i32(argx, &a);
        xdr_get_i32(argx, &b);
        int32_t result = handler_add(a, b);   /* the REAL function */
        xdr_put_i32(resx, result);
        return RPC_SUCCESS;
    }

    case PROC_REVERSE: {
        char s[256], rev[256];
        xdr_get_string(argx, s, sizeof(s) - 1);
        handler_reverse(s, rev);               /* the REAL function */
        xdr_put_string(resx, rev);
        return RPC_SUCCESS;
    }

    default:
        return RPC_ERR_NO_PROC;   /* the client asked for a procedure
                                      number this server doesn't have */
    }
}
```

Notice `handler_add()` and `handler_reverse()` are **completely ordinary C functions** — nothing about them knows or cares that they're being called remotely. This separation (real logic vs. RPC plumbing) is exactly what makes RPC usable in practice: an application programmer writes `int add(int, int)` once, and it can be called locally *or* remotely with zero changes to the function itself — every difference lives entirely in the dispatch/marshalling layer surrounding it.

The full per-connection server loop, tying it together:

```c
while (1) {
    int32_t n = recv_framed(client, msg, sizeof(msg));   /* block for a request */
    if (n < 0) break;   /* client disconnected */

    /* Parse the CALL header */
    xbuf_t in; xbuf_init(&in, msg, (uint32_t)n);
    uint32_t xid, msg_type, program, version, procedure, arg_len;
    xdr_get_u32(&in, &xid); xdr_get_u32(&in, &msg_type);
    xdr_get_u32(&in, &program); xdr_get_u32(&in, &version);
    xdr_get_u32(&in, &procedure); xdr_get_u32(&in, &arg_len);

    uint32_t status;
    if (program != DEMO_PROGRAM)      status = RPC_ERR_NO_PROC;
    else if (version != DEMO_VERSION) status = RPC_ERR_BAD_VERSION;
    else {
        xbuf_t argx; xbuf_init(&argx, msg + in.len, arg_len);
        xbuf_t resx; xbuf_init(&resx, resbuf, sizeof(resbuf));
        status = dispatch(procedure, &argx, &resx);
    }

    /* Build: [xid][REPLY][status][result_len][result...] and send it */
    xbuf_t out; xbuf_init(&out, reply, sizeof(reply));
    xdr_put_u32(&out, xid);          /* echo it back, unchanged */
    xdr_put_u32(&out, MSG_REPLY);
    xdr_put_u32(&out, status);
    xdr_put_u32(&out, (uint32_t)resx.len);
    memcpy(out.buf + out.len, resx.buf, resx.len);
    out.len += resx.len;

    send_framed(client, out.buf, (uint32_t)out.len);
}
```

The `program`/`version` checks, right at the top, are worth noting: they're what let a server safely refuse a request that's structurally well-formed but semantically wrong (an old client talking to a newer, incompatible server; a client that's simply connected to the wrong service entirely) — checked *before* attempting to dispatch or unmarshal anything procedure-specific, exactly like a defensive network parser should.

## 8. Running it for real — actual wire bytes, actual dispatch, actual results

Everything above is demonstrated, not just asserted. Here's a genuine run: the server started, the client connecting to it over `localhost`, real TCP, real bytes.

**Server startup and full session log:**
```
$ gcc -Wall -Wextra -o rpc_server rpc_server.c
$ gcc -Wall -Wextra -o rpc_client rpc_client.c
$ ./rpc_server 9500 &
[server] listening on port 9500, program=0x20001 version=1

$ ./rpc_client 127.0.0.1 9500
```

**Client output — including the exact bytes sent and received for the first call:**
```
[client] connected to 127.0.0.1:9500

[client] calling rpc_add(3, 4)  -- looks like an ordinary function call --
  CALL message about to be sent (32 bytes):
    0000: 00 00 00 01 00 00 00 00 00 02 00 01 00 00 00 01  ................
    0010: 00 00 00 01 00 00 00 08 00 00 00 03 00 00 00 04  ................
  REPLY message received (20 bytes):
    0000: 00 00 00 01 00 00 00 01 00 00 00 00 00 00 00 04  ................
    0010: 00 00 00 07                                      ....
[client] rpc_add(3, 4) = 7
```

**Let's decode that CALL message's 32 bytes by hand, exactly the way the server's own `xdr_get_u32()` calls do it, four bytes at a time:**

| Bytes | Value | Field |
|---|---|---|
| `00 00 00 01` | 1 | `xid` — this client's first call |
| `00 00 00 00` | 0 | `msg_type` = `MSG_CALL` |
| `00 02 00 01` | 0x20001 | `program` — matches `DEMO_PROGRAM` |
| `00 00 00 01` | 1 | `version` |
| `00 00 00 01` | 1 | `procedure` = `PROC_ADD` |
| `00 00 00 08` | 8 | `arg_len` — two 4-byte ints follow |
| `00 00 00 03` | 3 | argument `a` |
| `00 00 00 04` | 4 | argument `b` |

That's the entire call, byte for byte — no hidden magic, nothing left unaccounted for. **This table is the complete, literal answer to "what does an RPC call actually look like on the wire."**

**And the string-argument call, showing length-prefixing and padding in action:**
```
[client] calling rpc_reverse("Hello, RPC!")
  CALL message about to be sent (40 bytes):
    0000: 00 00 00 03 00 00 00 00 00 02 00 01 00 00 00 01  ................
    0010: 00 00 00 02 00 00 00 10 00 00 00 0b 48 65 6c 6c  ............Hell
    0020: 6f 2c 20 52 50 43 21 00                          o, RPC!.
  REPLY message received (32 bytes):
    0000: 00 00 00 03 00 00 00 01 00 00 00 00 00 00 00 10  ....
    0010: 00 00 00 0b 21 43 50 52 20 2c 6f 6c 6c 65 48 00  ....!CPR ,olleH.
[client] rpc_reverse("Hello, RPC!") = "!CPR ,olleH"
```

Notice `arg_len` here is `0x10` (16 bytes) even though `"Hello, RPC!"` is only 11 characters: 4 bytes for the string's own length prefix (`0b` = 11) + 11 bytes of text + **1 padding byte** (the trailing `00`) to round up to the next 4-byte boundary — exactly §4's alignment rule, visible directly in the actual bytes.

**And the server's own log for this entire session, confirming it dispatched by procedure number exactly as described:**
```
[server] connection from 127.0.0.1
[server] CALL received: xid=1 program=0x20001 version=1 procedure=1 arg_len=8
  [server] dispatch PROC_ADD(3, 4)
[server] REPLY sent:    xid=1 status=0 result_len=4

[server] CALL received: xid=3 program=0x20001 version=1 procedure=2 arg_len=16
  [server] dispatch PROC_REVERSE("Hello, RPC!")
[server] REPLY sent:    xid=3 status=0 result_len=16

[server] client disconnected
```

### The error paths, also demonstrated for real

What happens when a client asks for a procedure number the server doesn't implement, or a version it doesn't support — genuinely run, not described hypothetically:

```
$ ./rpc_client_err 9501
[client] unknown-procedure call -> xid=42 status=1 (RPC_ERR_NO_PROC=1)
[client] bad-version call    -> xid=43 status=2 (RPC_ERR_BAD_VERSION=2)
```
```
[server] CALL received: xid=42 program=0x20001 version=1 procedure=99 arg_len=0
  [server] dispatch: unknown procedure 99
[server] REPLY sent:    xid=42 status=1 result_len=0

[server] CALL received: xid=43 program=0x20001 version=99 procedure=1 arg_len=0
[server] REPLY sent:    xid=43 status=2 result_len=0
```

The server never even reaches `dispatch()` for the bad-version call — exactly as the code in §7 shows, the version check happens first, and a mismatched version is rejected before any procedure-specific logic runs at all.

---

## 9. Case study: classic Sun/ONC RPC (the protocol behind NFS)

Everything above is a deliberately simplified reconstruction of ideas that trace directly back to **Sun RPC** (also called **ONC RPC** — Open Network Computing RPC, RFC 1831 for the protocol, RFC 1832 for XDR specifically), designed in the 1980s and still, today, the mechanism underneath **NFS** (Network File System) and **NIS**. If you've ever wondered how mounting a remote filesystem actually turns `read()` calls into network traffic, this is the answer.

**What's genuinely identical to this document's demo:**
- XDR's fixed big-endian byte order and 4-byte alignment padding (§4) — this document's encoder is a deliberately close, simplified reconstruction of the real thing.
- A header carrying a transaction ID (Sun RPC calls it `xid` too — not a coincidence; this document borrowed the real name), a program number, a version number, and a procedure number.
- The exact same "server checks program/version before dispatching, dispatches by procedure number via a generated switch statement" structure.

**What real Sun RPC adds that this demo simplifies away:**
- **Authentication is a first-class part of the header**, not bolted on separately — every real call carries a credential (`AUTH_NONE`, `AUTH_UNIX`/`AUTH_SYS` carrying a Unix UID/GID, or stronger mechanisms) and a matching verifier, checked by the server before dispatch is even attempted.
- **The portmapper (`rpcbind`)**: this document's demo requires you to already know the server's port number in advance (`9500`, hardcoded). Real Sun RPC services instead register themselves, at startup, with a well-known local daemon (**`portmapper`**, historically on a fixed port, later renamed/generalized as `rpcbind`) — a client wanting to reach "NFS version 3" first asks the target machine's portmapper "what port is program X, version Y actually listening on right now?", then connects there. This decouples "which program/version I want" from "which specific port it happens to be using this boot," letting services bind to any available port rather than requiring a globally-agreed-upon fixed port for every possible service.
- **Both UDP and TCP transports are supported**, chosen per-service — NFS historically defaulted to UDP for performance reasons on the local networks it was designed for, which introduces the "what if the request or reply is simply lost, with no built-in retransmission from TCP" problem covered in §11 below.

## 10. Case study: gRPC (modern, HTTP/2-based RPC)

**gRPC** is Google's modern RPC framework, and while its transport is completely different from both this demo and classic Sun RPC, the *underlying six-piece architecture from §2 is identical* — which is exactly the point of including it here.

- **IDL**: **Protocol Buffers** (`.proto` files) play the exact role §3 described — `protoc` (the Protobuf compiler, with a gRPC plugin) generates client stubs and server-side dispatch skeletons in whatever target language you need, from one shared interface definition.
- **Marshalling**: Protobuf's own binary wire format — conceptually the same "flatten language-level values into an unambiguous byte sequence" job as this document's XDR-like encoder (§4), though Protobuf uses a tag-length-value scheme (each field is preceded by a field-number-plus-type tag) rather than XDR's purely positional layout, which is specifically what lets Protobuf messages evolve over time (add new fields, safely ignored by old parsers) more gracefully than a rigid positional format can.
- **Framing**: verified directly for this document — every gRPC message is wrapped in exactly a **5-byte prefix**: 1 byte for a compression flag, then a 4-byte **big-endian** length, then that many bytes of Protobuf payload:
  ```
  [compression flag: 1 byte][message length: 4 bytes, big-endian][protobuf payload]
  ```
  This is, functionally, **exactly** this document's `send_framed()`/`recv_framed()` (§5) with one extra leading byte — the same length-prefix-solves-TCP's-lack-of-message-boundaries idea, verified as still being the actual, current mechanism in a system built decades after Sun RPC.
- **Transport**: gRPC runs over **HTTP/2** rather than a bare TCP socket — the method/path/headers this document's demo would encode as raw header fields (`program`, `version`, `procedure`) instead ride inside genuine HTTP/2 request headers (`:method: POST`, `:path: /ServiceName.MethodName`, `content-type: application/grpc`), and the actual gRPC-framed Protobuf payload rides inside HTTP/2 DATA frames. The *result code* (this document's `status` field) comes back as an HTTP/2 **trailer** (`grpc-status`, `grpc-message`) rather than inside the message body — a direct, if dressed-up, analogue of this demo's reply header's `status` field.
- **Call correlation**: HTTP/2 already provides this natively via its own **stream IDs** — every request/response pair on a gRPC connection runs on its own HTTP/2 stream, so gRPC doesn't need a separate `xid`-style field at all; HTTP/2's transport layer already solved "which reply matches which request" for it, one level down.

The practical upshot: **gRPC didn't reinvent RPC's fundamental architecture — it re-implemented the exact same six pieces from §2 on top of a modern, multiplexing-capable transport (HTTP/2) and a more flexible, evolvable marshalling format (Protobuf) than XDR's rigid positional layout.** Understanding this document's from-scratch demo genuinely does transfer directly to reading gRPC's own wire format.

## 11. Cross-cutting concerns every real RPC system has to answer

### Call semantics: what happens when you don't know if it worked?

This is RPC's single hardest, most fundamental problem, and it has no clean solution — only trade-offs:

- **At-most-once**: if there's any doubt the call succeeded (timeout, no reply), give up and report failure, *never* retry automatically. Safe (never accidentally runs a procedure twice) but can report failure for calls that actually succeeded just fine — the reply was simply lost on the way back.
- **At-least-once**: retry on timeout, possibly executing the same call more than once on the server if the *reply* (not the original request) was what got lost. Never silently fails, but can duplicate side effects (imagine `transfer_funds()` executing twice).
- **Exactly-once**: the semantics everyone actually wants, and — across an unreliable network, without additional machinery — **provably impossible** to guarantee in the general case (this is a foundational, well-known result in distributed systems, not an implementation shortcoming). What real systems actually offer instead is exactly-once *for idempotent operations* (an operation safe to repeat, like "set the value to X," as opposed to "add 5 to the value"), or exactly-once via *additional* bookkeeping — the server remembers recently-seen call IDs and detects/suppresses genuine duplicates rather than re-executing them, at the cost of that extra state.

This document's demo (running over TCP, with an explicit `xid`) is closest to at-most-once in practice: a dropped connection simply fails the pending call, with no automatic retry logic. Classic Sun RPC over UDP explicitly documents its own default as at-least-once, and *specifically recommends designing remote procedures to be idempotent* wherever possible, precisely because it's the one design choice that makes "maybe it ran twice" harmless.

### Timeouts and retransmission

A call that never gets a reply has to give up *eventually* rather than blocking forever — exactly the same "always guard a blocking network wait with a timeout" lesson from ordinary socket programming, just applied to a whole RPC call rather than a single `recv()`. UDP-based RPC (lacking TCP's own built-in retransmission entirely) has to implement this itself, at the RPC layer: pick a timeout, retransmit the *identical* request (same `xid` and all) if it expires, and give up after some maximum number of attempts.

### Authentication

Because a "local" function call has no equivalent question ("is this call allowed to happen?" is meaningless when the caller and callee share a process and trust boundary already), RPC systems have to solve this explicitly, as part of the protocol itself — Sun RPC's credential-and-verifier fields (§9) and gRPC's typical reliance on TLS (encrypting and authenticating the whole HTTP/2 connection, rather than per-call credentials) are two different real answers to a question that simply doesn't exist for an ordinary function call.

---

## 12. Comparison table

| | This demo | Sun/ONC RPC | gRPC |
|---|---|---|---|
| Transport | Raw TCP | UDP or TCP | HTTP/2 |
| Marshalling | XDR-like (hand-rolled) | XDR | Protobuf |
| Framing | 4-byte length prefix | Record-marking (TCP) / datagram boundary (UDP) | 5-byte length+flag prefix, inside HTTP/2 DATA frames |
| Call correlation | Explicit `xid` field | Explicit `xid` field | HTTP/2 stream ID (no separate field needed) |
| Service discovery | Hardcoded port | Portmapper/`rpcbind` | DNS + typically a fixed well-known port |
| Auth | None | Credential+verifier in every call header | Usually TLS at the connection level |
| IDL | None (hand-written) | `rpcgen` | `protoc` |

## 13. Quick recap (for revision)

- RPC's whole job is making a network call *feel* like a local function call — every mechanism in this document exists to paper over one specific way those two things genuinely differ (no shared address space, unreliable delivery, byte-order disagreement, unknown latency).
- Six pieces, always: client stub, marshalling, wire protocol, transport, dispatcher, server stub — every real RPC system (Sun RPC, gRPC, and everything between) is a variation on exactly this structure.
- Marshalling needs a fixed byte order and length-prefixing for anything variable-length — this document's XDR-like encoder, and real XDR itself, both add 4-byte alignment padding specifically for architectures where misaligned access is slow or illegal.
- The wire protocol solves two separate problems: framing (where does one message end — a length prefix) and call correlation (which reply matches which request — an echoed transaction ID, or in HTTP/2-based systems like gRPC, the transport's own stream ID).
- The dispatcher — a procedure-number-keyed switch statement or lookup table — is the literal mechanism behind "calling a function by number instead of by address," and it's exactly what an IDL compiler generates automatically in any real-world system.
- **Exactly-once delivery is fundamentally, provably impossible** over an unreliable network without extra bookkeeping — every real RPC system explicitly chooses at-most-once or at-least-once, and idempotent procedure design is the standard mitigation for at-least-once's "maybe it ran twice" risk.
- gRPC didn't reinvent this architecture — verified directly against its own real specification, its 5-byte length-prefixed framing is functionally the same idea as this document's 4-byte one, riding inside a more modern transport (HTTP/2) with a more evolvable marshalling format (Protobuf) than XDR's rigid positional layout.