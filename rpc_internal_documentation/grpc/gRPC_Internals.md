# gRPC Internals: HTTP/2 Framing, Verified Against Real Captured Traffic

### Not described from memory — captured live from a real gRPC client and server, parsed frame-by-frame

---

## What makes this document different

Rather than describe gRPC's wire format from documentation alone, this document is built around a **real gRPC service** — a Python client and server, using genuinely generated Protobuf/gRPC stub code — with a small logging proxy inserted between them purely to capture every byte that actually crossed the wire. Every frame shown below is real, parsed with a from-scratch HTTP/2 frame parser (not a debugger or Wireshark) against the raw captured bytes. If you've read the companion "how RPC works internally" document, this is the promised deep dive into gRPC's specific answer to the same six architectural pieces every RPC system needs.

## 1. The setup that produced this capture

A trivial one-method service, defined in Protobuf:

```protobuf
syntax = "proto3";
package demo;

service Greeter {
  rpc SayHello (HelloRequest) returns (HelloReply) {}
}
message HelloRequest { string name = 1; }
message HelloReply   { string message = 1; }
```

```
$ python3 -m grpc_tools.protoc -I. --python_out=. --grpc_python_out=. greeter.proto
```

That one command generated the client stub and server-side dispatch code automatically — exactly the "IDL compiler generates the plumbing" step from the companion document's §3, here using `protoc` (the Protobuf compiler) with gRPC's own plugin, instead of ONC RPC's `rpcgen`. A real server (`grpc.server(...)`, listening with `add_insecure_port`) and a real client (`grpc.insecure_channel(...)`, calling `stub.SayHello(HelloRequest(name='World'))`) were run against each other, with a plain byte-logging TCP proxy sitting in between to capture every byte in both directions, unmodified. "Insecure" here specifically means **no TLS** — deliberately, so the captured bytes are the real HTTP/2 wire format directly, with nothing encrypted to strip away first (a production gRPC deployment runs this same traffic inside TLS, but the framing underneath is identical either way).

## 2. HTTP/2 fundamentals — just enough to read what follows

gRPC doesn't invent its own transport framing at all — it runs entirely on top of **HTTP/2**, and everything below is really "how gRPC uses HTTP/2's own existing mechanisms," not a separate protocol layered alongside it.

- **Streams**: HTTP/2 multiplexes many independent request/response exchanges over **one single TCP connection**, each with its own **stream ID**. This is HTTP/2's own answer to "which reply matches which request" — the companion document's simplified demo needed an explicit `xid` field for exactly this job; gRPC needs no such field at all, because the transport it rides on already solved that problem one layer down.
- **Frames**: every unit of HTTP/2 communication — headers, data, control messages — is a **frame**, each with an identical 9-byte header:

```
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                 Length (24 bits)                             |
  +---------------+---------------+-------------------------------+
  |   Type (8)    |   Flags (8)   |
  +-+-------------+---------------+-------------------------------+
  |R|                 Stream Identifier (31)                      |
  +-+-------------------------------------------------------------+
  |                   Frame Payload (variable)                    |
  +---------------------------------------------------------------+
```

The frame types that actually appear in this capture: `SETTINGS` (0x04, negotiating connection parameters), `WINDOW_UPDATE` (0x08, flow control), `HEADERS` (0x01, request/response metadata), `DATA` (0x00, the actual message payload — this is where gRPC's own framing, §4, lives), and `PING` (0x06, keepalive/liveness).

## 3. The real capture, decoded frame by frame

**Every byte below is genuine — captured from a real running gRPC client and server, parsed programmatically from the raw bytes, not transcribed by hand or simulated.**

### The connection opens: preface, then SETTINGS

```
[HTTP/2 CLIENT CONNECTION PREFACE]: b'PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n'
FRAME  len=36    type=SETTINGS      flags=0x00  stream_id=0
       setting ENABLE_PUSH = 0
       setting MAX_CONCURRENT_STREAMS = 0
       setting INITIAL_WINDOW_SIZE = 4194304
       setting MAX_FRAME_SIZE = 4194304
       setting MAX_HEADER_LIST_SIZE = 16384
FRAME  len=4     type=WINDOW_UPDATE flags=0x00  stream_id=0
```

That literal string `"PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n"` is HTTP/2's mandatory **connection preface** — every single HTTP/2 connection, gRPC or otherwise, begins with exactly this sequence of bytes, unconditionally, before anything else. It exists specifically so that a server expecting plain HTTP/1.1 (which this string is deliberately *not* valid as) fails fast and obviously, rather than misinterpreting HTTP/2 binary framing as garbled HTTP/1.1 text.

The `SETTINGS` frame that follows (`stream_id=0` — settings apply to the whole connection, not any one stream) is real, live parameter negotiation: `MAX_FRAME_SIZE = 4194304` here means this client is willing to accept individual frames up to 4MB; `INITIAL_WINDOW_SIZE` is the starting flow-control allowance (§3, `WINDOW_UPDATE`, is how that allowance gets topped back up as data is consumed).

### The request itself: a real HEADERS frame

```
FRAME  len=211   type=HEADERS       flags=0x04  stream_id=1
       HPACK-compressed header block (hex) = 40053a70617468162f64656d6f2e477265657465722f53617948656c6c6f...
```

Flag `0x04` is `END_HEADERS` — telling the receiver "the full header block is in this one frame, don't wait for a `CONTINUATION` frame." Decoding the **HPACK**-compressed block (HTTP/2's own header-compression scheme — not shown byte-by-byte here since HPACK's Huffman/dynamic-table mechanics are a separate topic from gRPC itself, but the *decoded* result is fully legible) yields exactly this set of headers:

```
:method: POST
:scheme: http
:path: /demo.Greeter/SayHello
:authority: 127.0.0.1:50051
content-type: application/grpc
te: trailers
grpc-accept-encoding: identity, deflate, gzip
user-agent: grpc-python/1.84.0 grpc-c/56.0.0 (linux; chttp2)
```

**This is the entirety of gRPC's answer to "which remote procedure is this call for."** There's no separate `program`/`procedure` numeric field the way the companion document's simplified demo (or Sun RPC's `prog`/`proc`) has — gRPC encodes the exact same information as an ordinary HTTP path: `/demo.Greeter/SayHello` is literally `/<package>.<Service>/<Method>`, taken directly from the `.proto` file's own service and method names. `content-type: application/grpc` is what tells the server "interpret the following `DATA` frames using gRPC's message framing, not as a plain HTTP body" — and `te: trailers` is a real, required signal that this client supports HTTP trailers, since gRPC's status code (§5) rides in exactly those.

### The call's actual payload: a real DATA frame, containing real Protobuf bytes

```
FRAME  len=12    type=DATA          flags=0x01  stream_id=1
       gRPC frame: compressed_flag=0 message_length=7
       protobuf payload (hex) = 0a05576f726c64
       decoded protobuf: field=1 wire_type=2 len=5 value=b'World'
```

Flag `0x01` here is `END_STREAM` — this is the client's *last* frame on this stream; it has no more to send. Inside this one `DATA` frame's 12-byte payload sits **gRPC's own message framing**, exactly as documented (and now directly confirmed against real, live traffic):

```
[compression flag: 1 byte][message length: 4 bytes, big-endian][protobuf payload]
       00                    00 00 00 07                0a 05 57 6f 72 6c 64
```

`compressed_flag=0` means this specific message isn't compressed (gRPC supports optional per-message compression, negotiated via the `grpc-encoding` header — not used in this simple local call). The remaining 7 bytes are genuine **Protobuf wire format**: `0a` is a tag byte, decomposed as `field_number=1, wire_type=2` (`2` = length-delimited — the encoding Protobuf uses for strings, bytes, and embedded messages alike); `05` is that field's length (5 bytes); and `576f726c64` is, byte for byte, the ASCII string `"World"`. **This is the literal, on-the-wire encoding of `HelloRequest(name="World")`** — field 1 (`name`, per the `.proto` definition) as a length-delimited string, nothing more.

### The response: HEADERS (status), DATA (result), then trailing HEADERS (the actual gRPC status)

```
FRAME  len=78    type=HEADERS       flags=0x04  stream_id=1
       HPACK-compressed header block (hex) = 88400c636f6e74656e742d74797065...

FRAME  len=20    type=DATA          flags=0x00  stream_id=1
       gRPC frame: compressed_flag=0 message_length=15
       protobuf payload (hex) = 0a0d48656c6c6f2c20576f726c6421
       decoded protobuf: field=1 wire_type=2 len=13 value=b'Hello, World!'

FRAME  len=30    type=HEADERS       flags=0x05  stream_id=1
       HPACK-compressed header block (hex) = 400b677270632d7374617475730130000c677270632d6d65737361676500
```

Three real, distinct frames worth examining individually:

1. **The first `HEADERS` frame** (decoded: `:status: 200`, `content-type: application/grpc`, `grpc-accept-encoding: identity, deflate, gzip`) is the *ordinary HTTP-level* response headers — note `:status: 200`, meaning "the HTTP transport itself succeeded." **This says nothing about whether the RPC call itself succeeded** — that's a separate question, answered below.
2. **The `DATA` frame** carries the actual result, using the exact same gRPC 5-byte-prefix framing as the request: `compressed_flag=0`, `message_length=15`, and the Protobuf bytes decode to field 1, length 13, value `b'Hello, World!'` — the literal wire encoding of `HelloReply(message="Hello, World!")`. Notice its flags are `0x00` — specifically **not** `END_STREAM` — because the gRPC status still needs to follow separately.
3. **The second `HEADERS` frame**, flags `0x05` (`END_HEADERS | END_STREAM` together) is genuinely a **second, separate HEADERS frame on the same stream** — this is an HTTP/2 **trailer**, and decoding it yields exactly: `grpc-status: 0`, `grpc-message: ` (empty). **This is gRPC's actual success/failure signal — not the earlier `:status: 200`, and not anything inside the `DATA` frame's payload.** A failed RPC call (say, the server threw an exception) would still very often show `:status: 200` at the HTTP level, with a *nonzero* `grpc-status` in this trailing frame instead — the two status concepts are genuinely independent, and gRPC-aware client libraries are the ones responsible for checking the *right* one.

### Keepalive, and the connection's own liveness check

```
FRAME  len=8     type=PING          flags=0x00  stream_id=0
       opaque payload = 01b6091e70b5018b
FRAME  len=8     type=PING          flags=0x01  stream_id=0
       opaque payload = 01b6091e70b5018b
```

A `PING` frame with an arbitrary 8-byte opaque payload, immediately answered by the peer echoing the *exact same* payload back with the `ACK` flag (`0x01`) set — HTTP/2's built-in connection-liveness check, entirely independent of any particular RPC call, running underneath gRPC without gRPC needing to implement anything itself for it.

## 4. Why gRPC needs its own 5-byte framing at all, given HTTP/2 already frames things

A natural question: HTTP/2 already splits everything into discrete frames (§2) — so why does gRPC add *another* length-prefix scheme (§3's `DATA` frame internals) on top? Because **one logical gRPC message and one HTTP/2 `DATA` frame are not guaranteed to be the same size** — a single large gRPC message can legitimately be split across several `DATA` frames (bounded by `MAX_FRAME_SIZE` from the `SETTINGS` negotiation in §3), or, conversely, several consecutive small messages (in a streaming call — §6) might in principle be packed efficiently. HTTP/2's own framing solves "where does one HTTP/2 frame end," not "where does one *gRPC message* end" — those are genuinely different boundaries, and gRPC's 5-byte prefix is exactly the mechanism resolving the second question, one layer above the first. This is, structurally, the *exact same problem* the companion document's simplified demo solved with its own 4-byte length prefix over raw TCP — gRPC just has to solve it one layer higher, since the transport underneath already has its own, coarser-grained framing.

## 5. Where the six architectural pieces live in gRPC specifically

Mapped directly onto the companion document's universal six-piece breakdown:

| Piece | gRPC's specific answer |
|---|---|
| IDL | Protobuf `.proto` files |
| Stub generation | `protoc` + the gRPC plugin |
| Marshalling | Protobuf binary wire format (tag-length-value, as seen directly in §3) |
| Wire protocol / framing | 5-byte prefix (compression flag + length) inside HTTP/2 `DATA` frames |
| Transport | HTTP/2 (itself running over TCP, optionally inside TLS) |
| Call correlation | HTTP/2 stream ID — no separate field needed, confirmed directly: every frame above shares `stream_id=1` |
| Dispatch | The `:path` pseudo-header (`/Service/Method`), routed by the generated server-side skeleton |
| Status/error reporting | HTTP/2 trailers (`grpc-status`/`grpc-message`) — genuinely separate from the transport-level `:status` |

## 6. Streaming — the one capability this simple example doesn't show

This capture demonstrates a **unary** call (one request, one response) — but HTTP/2's stream multiplexing is what makes gRPC's other three call shapes possible without any additional transport-level machinery:

- **Server streaming**: one request, *multiple* `DATA` frames in reply, each independently gRPC-framed exactly as §3 showed, before the final trailing `HEADERS` frame closes the stream.
- **Client streaming**: the mirror image — multiple `DATA` frames from the client, one final response.
- **Bidirectional streaming**: both sides send `DATA` frames on the *same* stream, interleaved, in either order, until either side signals `END_STREAM`.

None of these need a new transport mechanism — they're all just "more `DATA` frames, on the same HTTP/2 stream, before the stream closes," which is exactly why gRPC could support all four call shapes from day one without inventing anything beyond what this document's single unary-call capture already demonstrates in full.

---

## Quick recap (for revision)

- gRPC adds no transport machinery of its own — it's Protobuf marshalling plus a thin 5-byte message-framing convention, running entirely on top of ordinary HTTP/2.
- The HTTP/2 connection preface (`PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n`) is mandatory, identical on every HTTP/2 connection, and exists specifically to fail fast against HTTP/1.1-only peers.
- `:path: /Service/Method` (an ordinary HTTP path, derived directly from the `.proto` service/method names) is gRPC's entire dispatch mechanism — no separate numeric program/procedure fields exist at all.
- **Verified directly, byte for byte**: the gRPC `DATA` frame payload is `[1-byte compression flag][4-byte big-endian length][Protobuf bytes]` — confirmed against real captured traffic showing `HelloRequest(name="World")` and `HelloReply(message="Hello, World!")` encoded exactly this way.
- **The real gRPC status is a trailing HTTP/2 `HEADERS` frame (`grpc-status`/`grpc-message`), not the earlier `:status` pseudo-header** — confirmed directly in the capture as a second, separate `HEADERS` frame closing the stream. A `:status: 200` at the transport level and a nonzero `grpc-status` in the trailer can both be true simultaneously; a correct client checks the latter.
- HTTP/2's own stream ID replaces the explicit transaction-ID field every simpler RPC scheme (including this document series' own from-scratch demo, and classic ONC RPC's `xid`) needs to add for itself — confirmed directly: every frame in this entire exchange carries `stream_id=1`.
- Streaming (server/client/bidirectional) needs no new transport concept at all — it's the same `DATA`-frames-on-a-stream mechanism this unary call already demonstrates, just with more than one message frame before the stream closes.