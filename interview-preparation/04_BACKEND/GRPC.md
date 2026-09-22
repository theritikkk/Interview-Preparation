# gRPC (Backend Specifics)

Positioning versus REST/WebSockets is in `03_CS_FUNDAMENTALS/NETWORKING.md` (Cluster 4). ⚠️ Skills-list item, no project bullet — prepare conceptually.

---

## Cluster 1: Protocol Buffers & Code Generation

### What is it?
A `.proto` file defines a service's methods and message schemas in a language-neutral way; a compiler (`protoc`) generates strongly-typed client and server code in whatever language you target — the schema is the single source of truth, not hand-written serialization code.

### How does it work?
```protobuf
service TaskService {
  rpc GetTask (TaskRequest) returns (TaskResponse);
}
message TaskRequest { string id = 1; }
message TaskResponse { string id = 1; string status = 2; }
```

### Interview Questions
- **L1:** "gRPC uses Protocol Buffers — a schema-first, binary serialization format — with generated client/server code, over HTTP/2, for efficient typed service-to-service communication."
- **L2:** explain why binary + schema-defined (Protobuf) beats JSON on both size and (de)serialization speed, at the cost of not being human-readable off the wire and requiring the generated code/schema to inspect a message at all.

---

## Cluster 2: Streaming Modes

### What is it?
gRPC supports four call shapes: unary (one request, one response — like a normal REST call), server-streaming (one request, a stream of responses), client-streaming (a stream of requests, one response), and bidirectional streaming (both sides stream independently).

### Interview Questions
- **L1:** name the four modes and one use case each (unary: simple RPC; server-streaming: live updates/feed; client-streaming: uploading a large dataset in chunks; bidi: a chat-like continuous exchange).
- **L2:** contrast bidirectional gRPC streaming with WebSockets for a "continuous two-way data" use case: gRPC streaming is typed and schema-enforced by Protobuf, generally used service-to-service; WebSockets are more common for browser-facing use cases (native browser support, no gRPC-web proxy layer needed).

### Quick Revision
- `.proto` schema → generated typed client/server code, no hand-written serialization.
- Binary + schema = smaller/faster than JSON, less human-debuggable.
- Four streaming modes: unary, server-streaming, client-streaming, bidirectional.
