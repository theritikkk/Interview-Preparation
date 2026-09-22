# Computer Networks → Application Layer: DNS, HTTP & Sockets

**GATE 2027 syllabus:** Computer Networks — DNS and HTTP; TCP socket API.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** RTT is the round-trip time between client and server, taken as 100 ms unless stated otherwise. "Base page" means the HTML document; it references 5 embedded objects, all on the same server, unless stated otherwise. Ignore transmission delay (assume it is negligible compared to RTT) and processing time at client/server.

---

## Questions

### Q1 · MCQ · Fundamental
Which one of the following is the correct order of socket API calls made by a **TCP server**?

- (A) socket → connect → bind → listen → accept
- (B) socket → bind → listen → accept
- (C) socket → listen → bind → accept
- (D) bind → socket → accept → listen

---

### Q2 · MCQ · Fundamental
In DNS, an **iterative** query from a resolver to a root server typically results in

- (A) the root server itself contacting the TLD server and returning the final answer
- (B) the root server returning a referral (the address of a TLD server) rather than the final answer
- (C) the root server returning the IP address of the requested hostname directly
- (D) an error, since root servers do not accept iterative queries

---

### Q3 · NAT · Standard
A client fetches a base HTML page and 5 embedded objects, all from the same server, using **non-persistent** HTTP with no parallel connections (each object requires its own new TCP connection: one RTT for the handshake, then one RTT for the request/response). Ignore DNS lookup time. The total time to fetch the entire page, in units of RTT, is ______.

---

### Q4 · MSQ · Standard
Which of the following statements about HTTP is/are TRUE?

- (A) HTTP is a stateless protocol; the server does not retain information about prior requests by default.
- (B) Persistent HTTP with pipelining can send multiple requests over one TCP connection without waiting for each response before sending the next request.
- (C) HTTP response status codes in the 4xx range indicate a server-side error, while 5xx codes indicate a client-side error.
- (D) HTTP response status code 304 means "Not Modified", typically used with conditional GET and caching.

---

### Q5 · NAT · Standard
Using the same scenario as Q3 (base page + 5 same-server objects, RTT = 100 ms, ignore DNS and transmission time) but now with **persistent HTTP without pipelining** (one TCP connection reused, but each request still waits for its response before the next request is sent), the total time to fetch everything, in milliseconds, is ______.

---

### Q6 · MCQ · Moderate
A resolver performs a fully **recursive** DNS lookup for www.example.com by querying its configured local DNS server, which itself contacts the root, then the .com TLD server, then example.com's authoritative server, each step being a separate query-response pair from the local server's perspective. Assuming no caching anywhere and one RTT per query-response pair (local resolver ↔ local DNS server counts as one additional RTT), the total number of RTTs experienced by the **original resolver** (the client, waiting for its single request to the local DNS server) is

- (A) 1
- (B) 3
- (C) 4
- (D) 5

---

### Q7 · MSQ · Moderate
Which of the following statements about DNS is/are TRUE?

- (A) A DNS resource record has an associated TTL that governs how long it may be cached.
- (B) MX records specify the mail server(s) responsible for a domain.
- (C) DNS exclusively uses TCP, never UDP, because reliability is essential for name resolution.
- (D) A CNAME record maps an alias hostname to another (canonical) hostname.

---

### Q8 · NAT · Moderate
Using the scenario of Q3 (base page + 5 same-server objects, RTT = 100 ms) but now with **non-persistent HTTP and up to 3 parallel TCP connections** (so at any time, up to 3 objects can be fetched simultaneously, each needing its own connection setup + request/response = 2 RTTs; the base page must be fully fetched first, before any embedded-object fetch begins; once a batch of parallel fetches finishes, the next batch of remaining objects starts immediately), the total time, in milliseconds, is ______.

---

### Q9 · MSQ · Hard
Which of the following statements about the TCP socket API is/are TRUE, for a typical connection-oriented (SOCK_STREAM) server?

- (A) `bind()` associates the socket with a specific local IP address and port number.
- (B) `listen()` puts the socket into a passive mode and specifies a queue length for pending connections.
- (C) `accept()` blocks until a client connection arrives, and returns a **new** socket descriptor for that connection, leaving the original listening socket free to accept further connections.
- (D) A single call to `accept()` handles all subsequent data exchange with every connected client.

---

### Q10 · NAT · Killer
A client fetches a base HTML page (from server S) that references 5 embedded objects: 3 are hosted on server S itself, and 2 are hosted on a different server T (requiring a separate DNS lookup and a separate TCP connection to T). Assume: DNS lookup for a new hostname takes 1 RTT and can be done in parallel with other DNS lookups but must complete before any connection to that host begins; RTT to both S and T is 100 ms; HTTP is **persistent with pipelining** — one TCP connection to S (after DNS + handshake, both already done for S since the base page was just fetched over it, so no new DNS or handshake delay for S) can fetch all 3 of S's objects together in 1 more RTT; one new TCP connection to T (needing 1 DNS RTT, done in parallel with the base page fetch, plus 1 RTT for the T-handshake, plus 1 RTT to pipeline both T objects) is opened separately. The two connections (to S for the objects, and to T) proceed in parallel once both are ready. The total time from the start of the DNS lookup for S to receiving all objects, in milliseconds (given the very first step is a 1-RTT DNS lookup for S, followed by a 1-RTT TCP handshake to S, then a 1-RTT fetch of the base page from S), is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | B | socket, bind, listen, accept |
| 2 | B | Iterative queries get referrals, not full resolution |
| 3 | 12 | 2 RTT for base page + 5 × 2 RTT for objects |
| 4 | A, B, D | (C) has it backwards: 4xx is client error, 5xx is server error |
| 5 | 700 | 2 RTT for connection + base page, then 1 RTT per object × 5 = 2 + 5 |
| 6 | C | 1 RTT for the client's own request to the local server, plus 3 sequential RTTs the local server spends querying root, TLD and authoritative servers before it can reply: 1 + 3 |
| 7 | A, B, D | (C) is false — DNS primarily uses UDP, falling back to TCP for large responses or zone transfers |
| 8 | 600 | Base page: 2 RTT; then ⌈5/3⌉ = 2 batches, each taking 2 RTT (3 objects in parallel, then the remaining 2): 2 + 2 + 2 |
| 9 | A, B, C | (D) is false — a server typically uses a new socket/thread/process per connection or an event loop, not the same blocking accept() call |
| 10 | 500 | DNS(S) + handshake(S) + base page(S) = 3 RTT; at that point DNS(T) has already finished in parallel. From RTT 3: S's 3 objects finish 1 RTT later (RTT 4, connection already open); T needs a handshake (RTT 4) then a pipelined fetch of both objects (RTT 5). The slower path (T) sets the total: 5 RTT |
