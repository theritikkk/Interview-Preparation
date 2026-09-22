# WebSockets (Backend Specifics)

Protocol-level mechanics (upgrade handshake, full-duplex persistent connection) are in `03_CS_FUNDAMENTALS/NETWORKING.md` (Cluster 4). ⚠️ Skills-list item, no project bullet — prepare conceptually.

---

## Cluster 1: Connection Lifecycle & Broadcast Patterns

### What is it?
A WebSocket connection persists for as long as both ends keep it open — the server must track connected clients explicitly (unlike stateless REST, where nothing is remembered between requests) to support broadcasting or targeted messaging.

### How does it work?
```js
const clients = new Set();
wss.on('connection', (ws) => {
    clients.add(ws);
    ws.on('close', () => clients.delete(ws));   // MUST clean up, or you leak references
    ws.on('message', (msg) => {
        for (const client of clients) client.send(msg);   // naive broadcast to everyone
    });
});
```

### Interview Questions
- **L1:** "Unlike REST, a WebSocket server must track open connections explicitly to support broadcasting; cleanup on disconnect is essential to avoid leaking references to dead connections."
- **L2:** explain "rooms"/channels (grouping a subset of connections, e.g. by chat room or tenant) instead of naive broadcast-to-everyone, and why that grouping matters for both correctness (don't leak tenant A's messages to tenant B — directly relevant given Hermes's multi-tenancy theme) and efficiency.

---

## Cluster 2: Scaling WebSockets Across Multiple Server Instances

### What is it?
A WebSocket connection is pinned to the specific server instance it was opened on — a naive multi-instance deployment breaks broadcasting, because instance A has no way to reach a client connected to instance B.

### How does it work?
The standard fix: a shared pub/sub backplane (commonly Redis pub/sub) — every instance subscribes to relevant channels, and a message published by any instance reaches all instances, each of which forwards it only to its own locally-connected clients that need it.

### Interview Questions
- **L1:** "WebSocket connections are pinned to one server instance; scaling to multiple instances needs a shared pub/sub layer (like Redis) so a message from any instance reaches the right clients wherever they're connected."
- **L2:** explain why this is architecturally a *harder* scaling problem than REST's, precisely because REST's statelessness (any instance can handle any request) doesn't hold for a stateful, long-lived WebSocket connection.

### Quick Revision
- Server must explicitly track connections and clean up on disconnect.
- Rooms/channels scope broadcasts — critical in any multi-tenant context.
- Multi-instance WebSocket scaling needs a shared pub/sub backplane (e.g., Redis) because connections are pinned per-instance.
