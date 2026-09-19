# Sun/ONC RPC Internals: The Protocol Behind NFS

### Portmapper, XDR, credentials, and TCP record marking — verified directly against RFC 1831/5531/1833

---

## What this is, and why it still matters

**ONC RPC** ("Open Network Computing RPC," also called "Sun RPC" after its origin at Sun Microsystems in 1984) is the RPC mechanism underneath **NFS** (Network File System) and **NIS** (Network Information Service). It's the oldest RPC system still in genuine, widespread production use — every time a Unix/Linux machine mounts an NFS export, every `read()`/`write()` call against that mount is, underneath, an ONC RPC call. If you've read the companion document on RPC internals (the from-scratch client/server demo), this document is the "here's exactly how the real thing differs from that simplified version" companion — every claim below is checked directly against the actual specifications: RFC 1831 (1995, the original), RFC 5531 (2009, the current version, functionally identical for everything covered here), and RFC 1833 (the binding/portmapper protocol). Two of this document's own claims — the portmapper discovery flow, and record marking's fragment-reassembly rule — are additionally backed by a real, working portmapper, service, and client, compiled and run (§6), not just quoted from the specification.

## 1. The message format, exactly as specified

The top-level message envelope, straight from the RFC's own XDR language definition:

```c
struct rpc_msg {
    unsigned int xid;
    union switch (msg_type mtype) {
        case CALL:  call_body cbody;
        case REPLY: reply_body rbody;
    } body;
};
```

Note the RFC's own explicit caveat about `xid`, worth internalizing since it corrects a natural assumption: *"the xid field is only used for clients matching reply messages with call messages or for servers detecting retransmissions; the service side cannot treat this id as any type of sequence number."* In other words: **don't read any ordering guarantee into `xid` at all** — it's purely an opaque matching token, not a counter the server can rely on incrementing predictably.

The call itself:

```c
struct call_body {
    unsigned int rpcvers;    /* MUST be equal to 2 */
    unsigned int prog;
    unsigned int vers;
    unsigned int proc;
    opaque_auth  cred;
    opaque_auth  verf;
    /* procedure-specific parameters start here */
};
```

Compare this directly to the simplified demo's header (`xid`, `msg_type`, `program`, `version`, `procedure`, `arg_len`) — it's the same four identifying fields (`prog`/`vers`/`proc` map directly onto `program`/`version`/`procedure`), plus one thing the simplified demo deliberately left out: **`cred` and `verf`, a pair of authentication fields present on every single call, unconditionally.** There is no "optional auth" mode in the protocol structurally — every call carries *some* credential, even if that credential is the deliberately empty `AUTH_NONE` (§4 below).

The reply side is a union on whether the server even attempted to run the procedure:

```c
union reply_body switch (reply_stat stat) {
    case MSG_ACCEPTED: accepted_reply areply;
    case MSG_DENIED:   rejected_reply rreply;
} reply;
```

`MSG_DENIED` covers the case where the server refuses to even consider running the procedure at all — typically an authentication failure, or an RPC protocol version the server doesn't speak — as distinct from `MSG_ACCEPTED`, which means the server engaged with the request, and *then* reports one of several possible outcomes:

| Accept status | Meaning |
|---|---|
| `SUCCESS` (0) | The procedure ran and returned a result |
| `PROG_UNAVAIL` (1) | This server doesn't implement that `prog` number at all |
| `PROG_MISMATCH` (2) | This server implements that program, but not that `vers` |
| `PROC_UNAVAIL` (3) | Right program and version, but no such `proc` number |
| `GARBAGE_ARGS` (4) | The arguments didn't decode cleanly — malformed/corrupt request |
| `SYSTEM_ERR` (5) | Some other server-side failure unrelated to the call itself |

Notice how much finer-grained this is than the simplified demo's two-code error scheme (`RPC_ERR_NO_PROC`/`RPC_ERR_BAD_VERSION`) — real ONC RPC distinguishes "wrong program entirely" from "right program, wrong version" from "right program and version, but no such procedure," each independently diagnosable by a client without needing to guess which layer of the identification actually failed.

## 2. XDR — the encoding rules in full

XDR (External Data Representation, RFC 4506/1832) is ONC RPC's marshalling layer, and it's built from a small set of primitive rules that compose into everything else:

- **Every basic unit is 4 bytes, big-endian.** A plain `int`/`unsigned int` is exactly 4 bytes. There is no native XDR encoding smaller than 4 bytes at all — even a boolean is encoded as a full 4-byte `0` or `1`.
- **Variable-length data is length-prefixed**, with the length itself being a standard 4-byte unsigned int, immediately followed by the data, padded with zero bytes up to the next 4-byte boundary. This is exactly the encoding this whole document series' simplified demo's `xdr_put_string()` mirrors directly.
- **Fixed-length opaque data** (a byte blob whose length is already known from context, not encoded inline) is just the raw bytes, still padded to a 4-byte boundary — no length prefix needed since both sides already agree on the size from the type definition itself.
- **Arrays**: a *variable-length* array is encoded exactly like a string — a 4-byte count, then that many encoded elements back to back. A *fixed-length* array (size baked into the type definition) skips the count entirely, since both sides already know how many elements to expect.
- **Structs** are simply their fields, encoded in declaration order, one after another — no padding *between* fields beyond what each individual field's own encoding already requires, and no field-name or field-count information on the wire at all (unlike Protobuf's tag-based scheme — see the companion gRPC document). This is exactly why both sides of an XDR exchange must agree on the *exact* structure layout in advance, from a shared `.x` IDL file — there's nothing self-describing about the format itself.
- **Unions** (a value tagged with a discriminant telling the receiver which variant follows) are encoded as the discriminant (a 4-byte int or enum), followed immediately by whichever variant's own encoding that discriminant selects — this is precisely how `reply_body` above works: the `stat` field is a plain 4-byte enum, and what follows it is *either* an `accepted_reply` *or* a `rejected_reply`, decided purely by that one value.

**The 4-byte-alignment rule specifically exists for real hardware reasons**, not as an arbitrary stylistic choice: many CPU architectures either cannot read a multi-byte value from a memory address that isn't a multiple of that value's own size, or can only do so with a real performance penalty — padding every field to a 4-byte boundary guarantees the *next* field always starts aligned, regardless of what odd-length string or opaque blob happened to precede it.

## 3. Record marking — RPC's own framing for TCP, verified against the RFC's exact wording

TCP is a byte stream with no message boundaries (the same lesson underlying every framing scheme in this whole document series) — RFC 1831/5531 §10/§11 specifies **Record Marking (RM)** to solve this, and the exact bit layout is worth quoting precisely, since it differs in one genuinely important way from the simplified demo's plain 4-byte length prefix:

> *"A record fragment is a four-byte header followed by 0 to (2^31 - 1) bytes of fragment data... The number encodes two values — a boolean which indicates whether the fragment is the last fragment of the record (bit value 1 implies the fragment is the last fragment) and a 31-bit unsigned binary value which is the length in bytes of the fragment's data. The boolean value is the highest-order bit of the header; the length is the 31 low-order bits."*

```
 Record fragment header (4 bytes, big-endian):

  bit:   31                                              0
        ┌──┬──────────────────────────────────────────────┐
        │L │              fragment length (31 bits)         │
        └──┴──────────────────────────────────────────────┘
         ▲
         └── 1 = this is the LAST fragment of the record (message)
             0 = more fragments follow
```

**One RPC message = one "record," and a record can be split across multiple fragments** — a genuine capability the simplified demo's single flat 4-byte length prefix doesn't have at all. In practice, the overwhelming majority of real ONC RPC implementations simply send every message as exactly one fragment (setting the top bit immediately, with the full message length in the remaining 31 bits) — but the protocol explicitly *allows* a sender to break one large message into several smaller fragments, each with its own 4-byte header, with only the *final* fragment's header having its top bit set. A receiver has to keep reading fragments, concatenating their payloads, until it sees a header with that top bit finally set — only then does it have a complete record to actually hand up to the RPC layer for XDR decoding.

This is a genuinely more flexible framing scheme than the simplified demo's, for a specific reason: it lets a sender start transmitting a very large message's *first* fragment before the *entire* message is even fully assembled/ready — useful for streaming a large response without needing to buffer the whole thing in memory first, something a single global length-prefix scheme (which requires knowing the total length before you send even the first byte of the header) can't do as naturally.

## 4. Authentication — a first-class part of every single call

Every `call_body` carries a `cred` and a `verf` (§1) — and the RFC defines several concrete authentication "flavors" a real implementation chooses between:

| Flavor | Numeric value | What it actually provides |
|---|---|---|
| `AUTH_NONE` | 0 | No identity claim at all — "the client doesn't care about its identity, or the server doesn't care who's calling." The RFC explicitly recommends the opaque data length be zero in this case. |
| `AUTH_SYS` (historically `AUTH_UNIX`) | 1 | A **claimed** Unix-style identity — no cryptographic proof at all, just a plain, unencrypted assertion |
| `AUTH_SHORT` | 2 | A compact, previously-negotiated shorthand credential (avoids re-sending a full credential on every call) |
| `AUTH_DES` | 3 | An older, DES-based credential scheme with genuine cryptographic backing |
| `RPCSEC_GSS` | 6 | A GSS-API-based framework supporting genuinely strong, modern authentication and encryption (notably including Kerberos) |

**`AUTH_SYS`'s exact wire structure is worth seeing in full**, because its complete lack of cryptographic verification is a real, well-known, and still-relevant security limitation of classic NFS deployments:

```c
struct authsys_parms {
    unsigned int stamp;
    string       machinename<255>;
    unsigned int uid;
    unsigned int gid;
    unsigned int gids<16>;
};
```

`uid` and `gid` here are exactly what they sound like: the calling process's Unix user and group IDs, as the *client* claims them to be — **sent in plain text, with nothing whatsoever cryptographically binding this claim to the actual identity of whoever sent the packet.** Any machine that can put a packet on the wire with the right IP address can claim to be UID 0 (root) with `AUTH_SYS`, and a server trusting `AUTH_SYS` alone has no way to tell the difference. This is precisely *why* classic NFS-over-`AUTH_SYS` is traditionally deployed only inside networks already trusted at the perimeter (and why `RPCSEC_GSS`-backed NFSv4 with real Kerberos authentication exists as the actual, security-conscious modern answer) — the protocol's own default authentication flavor is, by design, an unauthenticated *claim*, not a proof.

## 5. The portmapper (`rpcbind`) — solving "which port is this service actually on?"

Here's a problem the simplified demo's client sidesteps entirely by hardcoding a port number: **a real ONC RPC service doesn't have a fixed, universally-agreed-upon port.** Two different machines might run NFS on two completely different ports, chosen dynamically at boot. So how does a client find it?

The answer is a second, dedicated RPC service — the **portmapper** — that *does* have a fixed, universally-agreed-upon location: **program number 100000, listening on the single well-known port 111**, on both TCP and UDP, on every machine that offers any ONC RPC services at all.

```c
const PMAP_PORT = 111;

struct mapping {
    unsigned int prog;
    unsigned int vers;
    unsigned int prot;   /* IPPROTO_TCP (6) or IPPROTO_UDP (17) */
    unsigned int port;
};

program PMAP_PROG {
    version PMAP_VERS {
        void         PMAPPROC_NULL(void)      = 0;
        bool         PMAPPROC_SET(mapping)    = 1;
        bool         PMAPPROC_UNSET(mapping)  = 2;
        unsigned int PMAPPROC_GETPORT(mapping) = 3;
        pmaplist     PMAPPROC_DUMP(void)      = 4;
        call_result  PMAPPROC_CALLIT(call_args) = 5;
    } = 2;
} = 100000;
```

**The actual sequence, end to end, for a client that wants to talk to NFS's mount daemon:**

```
1. Client → port 111 on the target machine:
   "PMAPPROC_GETPORT { prog=100005 (mountd), vers=1, prot=TCP }"

2. Portmapper → Client:
   "Port 32767"

3. Client → port 32767 directly:
   the ACTUAL mountd RPC call (mount request, etc.)
```

And on the *server* side, at boot: every RPC-based daemon, right after binding its own (often dynamically-chosen) port, makes exactly one `PMAPPROC_SET` call to the local portmapper — "I'm program 100005, version 1, TCP, and I'm listening on port 32767" — registering itself so the lookup above has something to find. `rpcinfo -p` against a real machine dumps the portmapper's entire live registry, and reading its output is a direct, literal view of every `PMAPPROC_SET` call currently in effect:

```
   program vers proto   port  service
    100000    4   tcp    111  portmapper
    100000    3   tcp    111  portmapper
    100000    2   tcp    111  portmapper
    100003    3   tcp   2049  nfs
    100005    3   tcp  32767  mountd
    100021    4   tcp  32768  nlockmgr
```

Every line here is a live `(prog, vers, prot) → port` mapping, sitting in the portmapper's registry exactly as `PMAPPROC_SET` put it there — and this is the entire, complete mechanism by which "I want to talk to NFS version 3" turns into "connect to port 2049," without either side needing to hardcode that port number anywhere.

**`rpcbind`, on modern systems, is the same idea, generalized**: rather than being tied to plain "TCP or UDP on this IP," it supports a more general, transport-independent addressing scheme (`r_netid`, `r_addr` — a "universal address" that can describe TCP/IP, UDP/IP, or entirely different transports uniformly), registered via `RPCBPROC_SET` instead of `PMAPPROC_SET`, but solving the exact same "where is program X, version Y actually listening right now" problem for exactly the same reason.

## 6. Live demonstration: a real portmapper, and a real multi-fragment record

Everything above — §3's record marking, §5's portmapper flow — was, in the original version of this document, explained but not run. Here it's run for real: a genuine portmapper (on a non-privileged stand-in port, 9111, instead of the real, root-only port 111), a real service that lets the OS assign it a random ephemeral port and only *then* registers itself, and a client that does the full two-step discovery dance before ever talking to the actual service — plus a second test that deliberately sends one logical RPC message as two separate TCP-level fragments, to prove §3's "only the last fragment has the top bit set" rule against genuine reassembly code, not just the RFC's own wording.

**The service starts up not knowing its own port, and registers whatever the OS actually gave it:**

```c
addr.sin_port = 0;   /* "OS, pick any free port for me" */
bind(listener, (struct sockaddr*)&addr, sizeof(addr));
listen(listener, 10);

struct sockaddr_in actual;
socklen_t alen = sizeof(actual);
getsockname(listener, (struct sockaddr*)&actual, &alen);
uint32_t my_port = ntohs(actual.sin_port);   /* genuinely unknown until NOW */

register_with_portmapper(DEMO_PROGRAM, DEMO_VERSION, my_port);  /* a real PMAPPROC_SET call */
```

**Real output — the port really is different every run, assigned by the kernel, not chosen:**
```
[service] OS assigned me ephemeral port 35297 (I did not choose this)
[service] registered myself with the portmapper: prog=0x20001 vers=1 port=35297
[service] now serving real requests on port 35297
```

**The portmapper's own log, confirming a real `PMAPPROC_SET` followed later by a real `PMAPPROC_GETPORT`:**
```
[portmapper] listening on the well-known port 9111

  [portmapper] PMAPPROC_SET: registered prog=0x20001 vers=1 -> port 35297
  [portmapper] PMAPPROC_GETPORT: prog=0x20001 vers=1 -> port 35297
```

**And the client, doing exactly the two-step dance §5 described — ask the portmapper first, then connect directly, with no further portmapper involvement at all:**
```
[client] Step 1: asking the portmapper (port 9111) where program 0x20001, version 1 lives...
[client]         portmapper says: port 35297

[client] Step 2: connecting DIRECTLY to port 35297 (no portmapper involved this time)
[client]         ADD(3, 4) = 7
```

Notice the client never once hardcodes port 35297 anywhere in its source — it genuinely doesn't know that number until the portmapper's real reply tells it, live, at runtime. That's the entire mechanism, proven rather than just described.

### The record-marking fragment split, proven byte-for-byte

The client builds one complete, ordinary 32-byte CALL message (`ADD(111, 222)`), then — instead of sending it as the single fragment `send_framed()` normally would — deliberately splits it in half and sends each half with its own real RFC 5531 fragment header:

```c
uint32_t frag1_header = split_at;                    /* top bit CLEAR */
/* ...send hdr1, then the first 16 bytes... */

uint32_t frag2_header = remaining | 0x80000000u;      /* top bit SET */
/* ...send hdr2, then the remaining 16 bytes... */
```

**Real output — the actual header bytes, and the actual bit pattern, for both fragments:**
```
[client] Fragment 1 header bytes: 00 00 00 10  (top bit = 0 -> more fragments follow; length = 16)
[client] Fragment 2 header bytes: 80 00 00 10  (top bit = 1 -> LAST fragment; length = 16)
[client] Server correctly reassembled both fragments and replied: 111 + 222 = 333
```

`00 00 00 10` is exactly `16` with the top bit clear (`0x00000010`); `80 00 00 10` is exactly `16` again, but with the top bit set (`0x80000010`) — the *only* difference between "more is coming" and "this is the end" is that single highest-order bit, precisely as RFC 5531's own text (quoted in §3) specifies. The server's `recv_framed()` — which knows nothing special about this particular call, it's the exact same reassembly loop used for every ordinary single-fragment call throughout this whole demo — read the first fragment, saw the bit was clear, went back for another header, read the second fragment, saw the bit was *set*, and only then handed the fully-concatenated 32 bytes up to the RPC layer for decoding. The correct result (`333`) is only possible if that reassembly genuinely happened — a server that mishandled the split would have tried to decode 16 incomplete bytes as a full RPC message and failed outright.

## 7. What this means for reading real NFS traffic

If you've ever run `rpcinfo -p` against a file server, or watched NFS traffic in a packet capture, everything above is directly what you're looking at: the initial port-111 conversation is `PMAPPROC_GETPORT`; the subsequent traffic to whatever port that returned is real `call_body`/`reply_body` messages, each with a real `xid`, a real `prog`/`vers`/`proc` (NFS's own program number is 100003; `proc` values map onto specific operations like `NFSPROC_READ`, `NFSPROC_WRITE`, `NFSPROC_LOOKUP`), and a real `cred` — almost universally `AUTH_SYS` in classic, perimeter-trusted deployments, carrying the exact `authsys_parms` structure from §4, in plain text, on every single call.

---

## Quick recap (for revision)

- `call_body` = `rpcvers` (always 2) + `prog`/`vers`/`proc` + **mandatory** `cred`/`verf` — every call carries some authentication, even if it's the deliberately empty `AUTH_NONE`.
- `MSG_ACCEPTED` vs. `MSG_DENIED` splits "the server engaged with this call" from "the server refused it outright" — and accepted calls get one of five specific status codes, distinguishing wrong-program from wrong-version from wrong-procedure from garbled arguments.
- XDR: everything is 4-byte-aligned, big-endian; variable-length data is length-prefixed; structs have no field names or self-description on the wire at all — both sides must already agree on the exact layout from a shared IDL.
- Record marking's 4-byte fragment header dedicates its **top bit** specifically to "is this the last fragment" — letting one RPC message legitimately span several TCP-level fragments, a real flexibility the simplified demo's flat length-prefix doesn't offer.
- **`AUTH_SYS` is a plain-text, unverified claim of Unix UID/GID — not cryptographic proof of anything** — which is exactly why real security-conscious NFS deployments use `RPCSEC_GSS` (Kerberos-backed) instead, and why `AUTH_SYS`-based NFS has always assumed a trusted network perimeter.
- The portmapper (fixed at program 100000, port 111, on every ONC RPC-capable machine) solves "which port is service X actually on right now" via `PMAPPROC_SET` (server registers) / `PMAPPROC_GETPORT` (client looks up) — decoupling a service's *identity* (program/version number) from wherever it happens to be listening on this particular boot.
- **Demonstrated live, both halves**: a real service let the OS assign it a genuinely unpredictable ephemeral port (35297 in the captured run), registered it with a real portmapper, and a client that never once hardcodes that port discovered it via a real `PMAPPROC_GETPORT` call before connecting directly — and a message deliberately split into two real RFC 5531 fragments (headers `00 00 00 10` then `80 00 00 10` — the top bit is the *entire* difference) was correctly reassembled by ordinary reassembly code into one 32-byte record before decoding, producing the correct result.