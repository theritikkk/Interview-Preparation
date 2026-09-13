# OSTEP Notes 11 — Distributed Systems Fundamentals, NFS, and AFS

> Covers Ch47 (Distributed Systems), Ch48 (Sun's Network File System, NFS), Ch49 (The Andrew File System, AFS). See `00-index.md` for the full series map.

---

## Part A: Distributed Systems Fundamentals

⭐ **The defining new problem, distinct from everything earlier in the book:** distributed systems must tolerate **partial failure** — one machine can fail (or the network between machines can fail) while the rest of the system keeps running, unlike a single machine, where a failure tends to take the whole system down at once. Communication is now central, and it's fundamentally unreliable.

### Unreliable Communication Layers
⭐ **The simplest option: don't handle it — push the problem to the application.** This is a direct instance of the **end-to-end argument**: a low-level layer that tries to guarantee reliability can't fully succeed anyway (the application itself might still fail after "successfully" receiving a message), so sometimes it's better to keep the lower layer simple/fast and let the application decide what reliability it actually needs.
- **UDP/IP** is the classic unreliable layer: a sender can transmit a datagram, but if it's dropped, the sender is **never informed.** UDP does include a checksum (catching some corruption), but offers **no delivery guarantee** at all.

> **Tip: use checksums for integrity.** A checksum (XOR-based, CRC, Fletcher, etc.) is computed over a message before sending and re-verified on receipt — the classic effectiveness-vs-performance trade-off applies here exactly as it did for on-disk data integrity (previous notes file).

### Reliable Communication Layers — Building Reliability on Top
⭐ **Step 1 — acknowledgments (acks):** the receiver sends a short confirmation back after receiving a message.
⭐ **Step 2 — timeout/retry:** if no ack arrives within a set time, the sender assumes the message was lost and **resends** it (keeping a copy around in case a retry is needed).

⭐ **The remaining hazard: duplicate delivery.** If the *original* message was lost, a retry is exactly correct. But if instead the **acknowledgment** was lost (the message itself was received fine), the sender still times out and retries — and now the **receiver gets the same message twice.** For many operations this silently corrupts behavior (imagine a downloaded file with a repeated chunk).

⭐ **The fix: sequence counters**, not a naive "track every message ID ever seen" (which needs unbounded memory). Sender and receiver agree on a starting counter value; every message carries the current counter as its ID, and the sender increments it after each send. The receiver only accepts and passes up messages matching its **expected** next counter value; a message with an already-seen (lower) counter is simply **re-acked but not delivered again** to the application — cheaply detecting and discarding duplicates with only a single integer of state per sender.
- **TCP/IP** is the dominant real-world reliable layer, built on ideas like this plus much more sophistication (congestion control, multiple outstanding requests, etc.).

### Communication Abstractions
- **Sockets** (raw send/receive, as above) are the lowest-level abstraction.
- ⭐ **Distributed Shared Memory (DSM)** was an ambitious alternative — make remote memory accesses look like ordinary local memory reads/writes (analogous to how paging makes disk-backed memory look local, from the memory-virtualization notes files). ⭐ **The book is blunt about its fate:** DSM turned out to be a poor fit in practice — a load/store that's actually a network round-trip (triggered like a page fault) is far more expensive than programmers intuitively expect from "just touching memory," forcing careful, unnatural program restructuring to minimize communication — **"nobody builds reliable distributed systems using DSM today."**

### Remote Procedure Call (RPC)
⭐ **The abstraction that actually won:** make calling a function on a remote machine look, to the *programmer*, just like calling an ordinary local function. Built from two pieces:

**1. The stub generator** (protocol compiler) takes a simple interface definition:
```c
interface {
    int func1(int arg1);
    int func2(int arg1, int arg2);
};
```
and auto-generates:
- **Client stub:** packages ("**marshals**"/serializes) the function ID and arguments into a message buffer, sends it, **blocks waiting for a reply**, then unpacks ("unmarshals") the result and returns it to the caller — from the calling code's perspective, this looks exactly like an ordinary function call.
- **Server stub:** unmarshals an incoming message, calls the *actual* requested function with the extracted arguments, marshals the result, and sends the reply.

⭐ **This is precisely the point where the companion notes files' critique of RPC applies directly** — the stub generator's whole purpose is to *hide* the fact that this is a network call, but (as covered in the DDIA-based notes) that abstraction is fundamentally leaky: unlike a local call, this one can time out, get silently duplicated on retry, or fail in ways a local call never could — no amount of stub-generator cleverness removes that underlying difference, it only hides it from casual view.

**2. The run-time library** handles the genuinely hard parts:
- **Naming/locating the service** (typically via hostname + port, echoing standard internet naming).
- **Packaging complex arguments** — e.g., a `write()`-style call passing a buffer pointer and length needs the RPC system to understand how to serialize the pointed-to bytes, not just the pointer value itself (handled via well-known buffer types or annotated data structures).
- **Server concurrency** — a naive single-loop server wastes resources whenever one call blocks; most real servers use a **thread pool**, dispatching each incoming request to a worker thread — trading implementation simplicity for the usual concurrency concerns (locking, etc., from the earlier Concurrency notes files).

---

## Part B: Sun's Network File System (NFS)

⭐ **Historical/business note the book highlights:** Sun deliberately published NFS as an **open protocol** (just message formats) rather than a proprietary implementation — letting multiple vendors (NetApp, EMC, IBM, and others) build competing, interoperable NFS servers. This open-market approach is credited as a major factor in NFS's widespread, lasting success.

### Design Focus: Simple and Fast Server Crash Recovery
> **The Crux:** in a single-server, multi-client environment, *any* server downtime makes every client unproductive — so fast, simple recovery from a server crash is the design's central goal.

### Key to Fast Recovery: Statelessness
⭐ **The server tracks nothing about what clients are doing** — no open-file tables, no cached file pointer positions, nothing. Every request must be **fully self-describing**, carrying everything the server needs to complete it, so the server has no in-memory state to lose (or need to reconstruct) across a crash.

⭐ **Why a stateful design (like a naive port of local `open()`/`read()`) would be a real problem, illustrated directly:** if `open()` returned a server-tracked file descriptor, and the server crashed **between** two `read()` calls using that descriptor, the server would come back up with **no memory** of what that descriptor referred to — requiring an explicit, complex recovery protocol just to re-establish that lost mapping. Client crashes cause a mirror-image problem: if a client opens a file and then crashes, the server has no way to know it should ever free that file descriptor's resources, absent some way of detecting the client is gone. **NFS's stateless design sidesteps both problems entirely** — if the server crashes, it simply restarts; at worst, a client's in-flight request needs to be retried, exactly as if it had experienced ordinary packet loss.

### The File Handle — Making Statelessness Work
⭐ **The core mechanism replacing a stateful file descriptor.** A file handle has three parts:
| Field | Purpose |
|---|---|
| **Volume identifier** | Which file system (of possibly several) this request targets |
| **Inode number** | Which file/directory within that volume |
| **Generation number** | Incremented whenever an inode number is *reused* — so a client holding a stale handle to a deleted-then-recreated inode can't accidentally access the new file |

**Key protocol operations:** `LOOKUP` (resolve one pathname component to a file handle + attributes), `READ`/`WRITE` (given a handle, offset, count — fully self-contained, no server-tracked "current position"), `GETATTR` (fetch a file's attributes, including last-modified time — important for cache consistency, below).

### From Protocol to Distributed File System — a Worked Trace
⭐ **Crucially, all the "statefulness" a POSIX API needs (open file tables, current file offsets) is tracked entirely on the CLIENT, not the server:**
```
fd = open("/foo", ...)
  -> client sends LOOKUP(rootdir_FH, "foo")
  -> server returns foo's file handle + attributes
  -> client allocates a LOCAL file descriptor, stores foo's FH + position=0

read(fd, buffer, MAX)
  -> client looks up FH and current position from its OWN local table
  -> client sends READ(FH, offset=0, count=MAX)
  -> server uses FH to find volume/inode, reads the data, returns it
  -> client updates its OWN local position to MAX

close(fd)
  -> purely local cleanup -- no server communication needed at all
```
⭐ Notice a long pathname (`/home/remzi/foo.txt`) costs **one `LOOKUP` per path component** — each carrying its own self-contained information, exactly matching the stateless design principle.

### Handling Server Failure With Idempotent Operations
> **Tip: idempotency is powerful.** An operation is **idempotent** if performing it multiple times has the same effect as performing it once. Read operations are trivially idempotent; a `WRITE` (which specifies an exact offset and the data to place there) is *also* idempotent — writing the same bytes to the same offset twice leaves the same final result as once.

⭐ **This is precisely what lets NFS handle every kind of failure with one uniform mechanism: blind retry-on-timeout.** Whether the original request was lost, the server was briefly down, or the *reply* was lost after the server actually processed the request — in every case, the client's simple "timeout, then resend the exact same message" strategy produces the **correct final result**, because repeating an idempotent operation is harmless.

⭐ **The one notable exception, honestly acknowledged:** `MKDIR` is **not** naturally idempotent — if the original request actually succeeded but its reply was lost, the client's retry will fail with "directory already exists," surfacing a spurious error for an operation that, from the user's perspective, actually worked the first time.

> **Tip: perfect is the enemy of the good (Voltaire's Law).** The book explicitly declines to "fix" the `MKDIR` case with more elaborate machinery — NFS's simple, uniform idempotency-based design handles the overwhelming majority of cases cleanly, and chasing a fully perfect solution for one edge case isn't worth the added complexity. Good, simple engineering sometimes means accepting known, rare imperfections.

### Improving Performance: Client-Side Caching
⭐ Sending every read/write over the network is far slower than local memory/disk — NFS clients **cache** file data and metadata locally, and also **buffer writes** in the client cache before flushing to the server (decoupling the latency an application's `write()` call experiences from the actual, slower network write).

### The Cache Consistency Problem
⭐ **Two genuinely distinct sub-problems**, illustrated with three clients C1/C2/C3 sharing one file `F` via a server:
1. **Update visibility:** while C2's writes sit buffered in *its own* cache, any other client (C3) reading the file from the server still sees the stale, pre-update version — when do C2's updates actually become visible elsewhere?
2. **Cache staleness:** once C2 *has* flushed its update to the server, C1 might still be holding an old, now-stale cached copy in its own memory — how does C1 find out its cache is out of date?

**NFS's two-part fix:**
- **Update visibility → flush-on-close (close-to-open) semantics:** a client flushes all its dirty cached data back to the server the moment a written file is **closed** — guaranteeing any subsequent `open()` from *any* client sees at least that version.
- **Cache staleness → validate via `GETATTR` on open:** before trusting a cached copy, a client fetches the file's current last-modified time from the server; if it's newer than what the client's cached copy reflects, the cache entry is **invalidated**, forcing a fresh fetch.

⭐ **The performance problem this validation step then created, and its fix:** naive per-open `GETATTR` calls flooded the server with "has this changed?" checks, even though in the *overwhelming common case* nothing had. The fix: an **attribute cache** — cached attributes are trusted for a short window (with a timeout) before the client bothers re-checking with the server, trading a small, bounded staleness risk for a large reduction in server load. ⭐ This is a direct real-world illustration of the general engineering principle: **design for the common case, and make it fast** — even a theoretically "more correct" always-check approach can be the wrong engineering trade-off if it swamps the system with overhead for a case that almost never actually needs it.

---

## Part C: The Andrew File System (AFS)

⭐ **AFS's explicit primary design goal, distinguishing it sharply from NFS: SCALE** — how many clients can **one** server support? And, as a deliberate secondary goal from the start, **simple, well-defined user-visible cache consistency** — unlike NFS, whose actual consistency behavior depends on low-level, hard-to-reason-about details like cache-timeout intervals.

### AFS Version 1 — Whole-File Caching
⭐ **Core design choice, sharply contrasting with NFS:** cache the **entire file**, on the client's **local disk** (not just blocks, and not just in volatile client memory).
- `open()` → client (`Venus`) sends a `Fetch` message with the **full pathname**; the server (`Vice`) walks the path and ships the **whole file** back; it's cached to local disk.
- Subsequent `read()`/`write()` calls are **purely local** — no network traffic at all, redirected straight to the locally-cached copy (which may itself get further cached in client memory, just like an ordinary local file).
- `close()` → if modified, the whole file is flushed back via a `Store` message.
- On a **later** open of the same file, the client sends a cheap `TestAuth` message asking "has this changed?" before deciding whether to reuse its local cached copy.

### Problems With Version 1
⭐ **Measured, not guessed** (the book's tip: "measure then build," Patterson's Law) — each server could support only about **20 clients** before becoming overloaded, due to two measured bottlenecks: (1) **too many `TestAuth` messages** — every single file open, even for an already-fully-cached, unchanged file, still required a round trip; (2) **expensive full-pathname traversal** on every `Fetch`/`Store` — the server had to walk the entire path each time, even though most of that path (e.g., shared parent directories) rarely changes.

### AFS Version 2 — Callbacks
> **The Crux: how to design a scalable file protocol** — specifically, how to eliminate most of those redundant "has anything changed?" checks, and make the checks that remain cheap?

⭐ **The key innovation: the callback** — the server makes an explicit **promise** to proactively notify the client if a cached file is ever modified. This flips the interaction model entirely: instead of the client repeatedly *polling* "has this changed?" (as in NFS/AFSv1), the client simply **trusts its cache is valid until told otherwise** — the book itself draws the connection directly: *"insert analogy to polling versus interrupts here"* — precisely the same trade-off as polling vs. interrupt-driven I/O from the OS-internals notes file (Ch36).

⭐ **File Identifiers (FIDs)** replace raw pathnames as the unit clients reference — `(volume ID, file ID, "uniquifier")`, closely analogous to NFS's file handle. Crucially, the **client** (not the server) now walks a pathname **one component at a time**, caching each directory along the way and establishing a **separate callback on each** — so a later access to a deeply nested but already-cached path costs **nothing** server-side at all, directly attacking AFSv1's second bottleneck.

⭐ **The resulting common-case behavior:** the *first* access to a file generates the expected burst of `Fetch`+callback-setup messages; every **subsequent** access — as long as the callback hasn't been broken — is **entirely local**, "nearly identical to a local disk-based file system." This callback-based design is exactly what let AFS scale to vastly more clients per server than the polling-based NFS/AFSv1 approach.

### Cache Consistency in AFS — Clean and Well-Defined
⭐ **Between different machines:** an update becomes visible, and other clients' cached copies are invalidated, at the **exact same moment** — when the writing client **closes** the modified file. At that point, the server pushes the new version and **breaks the callback** on every other client currently caching the old version, forcing them to re-fetch on their next open.
⭐ **Within the same machine:** writes are visible to other **local** processes **immediately** (no need to wait for close) — matching ordinary UNIX single-machine semantics exactly; the distinctive cross-machine AFS behavior only becomes visible once a *different* machine is involved.

⭐ **The genuinely tricky case, worked through via a concrete timeline (the book's own detailed trace):** if processes on **two different machines** modify the *same* file concurrently, AFS's simple close-triggered-invalidation model reduces to a straightforward **last writer wins** — whichever client's `close()` (and resulting `Store`) happens last simply **overwrites** the other's changes entirely, with **no merging, no conflict detection, no warning.** A third process reading the file afterward just sees whatever the last writer left behind — silently losing the *other* writer's changes.

> **Aside: cache consistency is not a panacea.** The book is explicit that this baseline file-system-level consistency is meant for **casual usage** (a user reasonably expecting their own files to show up correctly when they log into a different machine) — it is emphatically **not** a substitute for real concurrency control in applications that actually care about concurrent conflicting updates (e.g., a code repository with simultaneous check-ins). Any application with genuine concurrent-write requirements must build its **own** explicit locking/merge logic on top — expecting the underlying distributed file system alone to handle that correctly is "setting yourself up for failure, disappointment, and tear-filled frustration."

---

## Summary Table

| System | Consistency model | Scalability mechanism | Key trade-off |
|---|---|---|---|
| **NFS** | Flush-on-close (visibility) + `GETATTR`-based staleness check (with an attribute cache to reduce load) | Stateless server + idempotent operations = trivial crash recovery | Consistency behavior depends on low-level timeout details; simple, self-contained requests |
| **AFS** | Callback-based: updates become visible and stale caches invalidated exactly at `close()`; last-writer-wins on true concurrent conflicts | Whole-file, local-disk caching + callbacks (push, not poll) | Much better server scalability; concurrent cross-machine writes can silently clobber each other |

## Interview/Exam-Ready Talking Points

- Explain **why UDP-style unreliable messaging plus timeout/retry alone isn't enough** for reliable delivery, and precisely how **sequence counters** solve the duplicate-delivery problem with minimal state.
- Explain **why RPC's "make remote calls look local" abstraction is fundamentally leaky**, connecting directly to the same critique from the DDIA-based notes file — a network call can time out, duplicate, or fail in ways a local call structurally cannot.
- Explain **why NFS chose a stateless design**, and walk through the concrete server-crash and client-crash scenarios that motivate it.
- Precisely describe an **NFS file handle** (volume ID + inode number + generation number) and explain why the generation number specifically exists.
- Explain **why most NFS operations are idempotent**, why that specifically enables NFS's uniform retry-on-timeout failure handling, and name the one common exception (`MKDIR`).
- Distinguish **NFS's two cache-consistency sub-problems** (update visibility vs. cache staleness) and state NFS's specific fix for each.
- Explain **AFS's callback mechanism** and why it's a direct structural analogue of polling vs. interrupts — and be ready to explain precisely why this shift is what let AFS scale far better than NFS/AFSv1.
- Walk through the **AFS "last writer wins" scenario** using a concrete two-client trace, and explain why this is a deliberate, accepted limitation rather than a bug — and why applications needing real conflict resolution must build it themselves on top.