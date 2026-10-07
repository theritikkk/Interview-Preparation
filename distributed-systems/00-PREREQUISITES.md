# Prerequisites & Foundational Knowledge — Complete Reference

> **Purpose:** This file contains ALL the foundational knowledge you need before reading the deep-dive notes.  
> **How to use:** If you read something in the other files and don't understand a term or concept, come back here.  
> **Covers:** CS fundamentals, OS, Networking, Databases, Java, Spring Boot, Linux, Cloud, API design — everything the deep dives assume you already know.

---

## Table of Contents

1. [Data Structures & Complexity](#1-data-structures--complexity)
2. [Operating System Foundations](#2-operating-system-foundations)
3. [Networking Foundations](#3-networking-foundations)
4. [Database Foundations](#4-database-foundations)
5. [Java Language Essentials](#5-java-language-essentials)
6. [Spring Boot Essentials](#6-spring-boot-essentials)
7. [JavaScript / Node.js Essentials](#7-javascript--nodejs-essentials)
8. [Linux Essentials](#8-linux-essentials)
9. [API Design & HTTP](#9-api-design--http)
10. [Cloud & Infrastructure Vocabulary](#10-cloud--infrastructure-vocabulary)
11. [Distributed Systems Vocabulary](#11-distributed-systems-vocabulary)
12. [Serialization & Data Formats](#12-serialization--data-formats)
13. [Cryptography & Security Basics](#13-cryptography--security-basics)
14. [Git & Version Control](#14-git--version-control)
15. [Prerequisite Map — What to Read Before What](#15-prerequisite-map--what-to-read-before-what)

---

## 1. Data Structures & Complexity

### Big-O Complexity — The Language of Performance

```
Time Complexity Hierarchy (best to worst):

O(1)        Constant     Hash table lookup, array index access
O(log n)    Logarithmic  Binary search, balanced BST lookup
O(n)        Linear       Array scan, linked list traversal
O(n log n)  Linearithmic Merge sort, heap sort, TimSort
O(n²)       Quadratic    Nested loops, bubble sort
O(2ⁿ)       Exponential  Recursive backtracking, power set
O(n!)       Factorial    Permutation generation
```

**Space complexity** = how much extra memory your algorithm uses (not counting input).

**Amortized complexity** = average cost per operation over a sequence. Example: `ArrayList.add()` is O(1) amortized — occasionally it resizes (O(n) copy), but across n adds, the average is O(1).

### Core Data Structures You Must Know

```
┌──────────────────────────────────────────────────────────────────┐
│                    Data Structure Quick Reference                │
├──────────────┬──────────┬──────────┬──────────┬──────────────────┤
│ Structure    │ Access   │ Search   │ Insert   │ Delete           │
├──────────────┼──────────┼──────────┼──────────┼──────────────────┤
│ Array        │ O(1)     │ O(n)     │ O(n)     │ O(n)             │
│ Linked List  │ O(n)     │ O(n)     │ O(1)*    │ O(1)*            │
│ Stack        │ O(n)     │ O(n)     │ O(1)     │ O(1)             │
│ Queue        │ O(n)     │ O(n)     │ O(1)     │ O(1)             │
│ Hash Table   │ N/A      │ O(1) avg │ O(1) avg │ O(1) avg         │
│ BST          │ O(log n) │ O(log n) │ O(log n) │ O(log n)         │
│ Heap         │ O(1) max │ O(n)     │ O(log n) │ O(log n)         │
│ Trie         │ O(k)     │ O(k)     │ O(k)     │ O(k)  k=key len  │
├──────────────┴──────────┴──────────┴──────────┴──────────────────┤
│ * = if you have a pointer/reference to the position              │
└──────────────────────────────────────────────────────────────────┘
```

### Hash Table — The Most Important Structure

```
Key: "user:123"
         │
         ▼
    hash("user:123") = 7482910374
         │
         ▼
    7482910374 % 16 = 6   (16 = number of buckets)
         │
         ▼
    Bucket[6] → ("user:123", "Ritik") → ("order:456", "data") → null
                          ↑
                    Collision handled via chaining (linked list)
```

**Why this matters everywhere:**
- Java `HashMap`, Python `dict`, Redis, DynamoDB partition keys, Kafka partitioning, consistent hashing — ALL use hash functions.
- **Hash collision:** Two different keys map to the same bucket. Handled via chaining (linked list) or open addressing (probing).
- **Load factor:** `n_entries / n_buckets`. When it exceeds a threshold (Java: 0.75), the table resizes (doubles buckets, rehashes everything).

### Trees You Need to Know

**Binary Search Tree (BST):**
- Left child < parent < right child (for all nodes).
- Unbalanced BST degrades to O(n). That's why we need balanced variants.

**Balanced BSTs:**
- **Red-Black Tree** — Used by Java `TreeMap`, `TreeSet`, Linux kernel CFS scheduler. Guarantees O(log n).
- **AVL Tree** — Stricter balancing than Red-Black. Faster reads, slower writes.

**B-Tree / B+ Tree:**
- Used by **every relational database** (PostgreSQL, MySQL) for indexes.
- Wide, shallow tree optimized for disk I/O. Each node holds many keys (fanout of 100-500).
- B+ Tree: All data in leaf nodes, leaves are linked → efficient range scans.

**LSM-Tree (Log-Structured Merge-Tree):**
- Used by **Cassandra, RocksDB, LevelDB, Kafka (sort of)**.
- Write-optimized: writes go to in-memory structure (memtable), then flushed to sorted disk files (SSTables).
- Reads are slower (check memtable + multiple SSTables).
- Background compaction merges SSTables.

```
Write Path (LSM-Tree):
  Client Write
      │
      ▼
  WAL (append to disk log for durability)
      │
      ▼
  Memtable (in-memory sorted structure, e.g., skip list)
      │ (when memtable is full)
      ▼
  Flush to SSTable on disk (sorted, immutable file)
      │ (background)
      ▼
  Compaction (merge multiple SSTables, remove duplicates)
```

### Bloom Filter — Probabilistic Membership Test

```
"Is key X in this SSTable?"

Bloom Filter answers:
  "Definitely NOT here"  → 100% accurate, skip this SSTable
  "Maybe here"           → Could be a false positive, must check

False positive rate: ~1% with proper sizing
False negative rate: 0% (NEVER misses a present key)

Used by: Cassandra, HBase, LevelDB, Chrome (safe browsing), Kafka
```

### Skip List

```
Level 3:  HEAD ─────────────────────────────────► 50 ──────────► NIL
Level 2:  HEAD ──────────► 20 ──────────────────► 50 ──────────► NIL
Level 1:  HEAD ──► 10 ──► 20 ──► 30 ──► 40 ──► 50 ──► 60 ──► NIL
Level 0:  HEAD ──► 10 ──► 20 ──► 30 ──► 40 ──► 50 ──► 60 ──► NIL

O(log n) search by skipping ahead at higher levels.
Used by: Redis sorted sets, Java ConcurrentSkipListMap, Memtable in LSM-Trees
```

---

## 2. Operating System Foundations

### Process vs Thread

```
┌───────────────────────────────────────────────────────────────┐
│                         PROCESS                               │
│                                                               │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐                     │
│  │ Thread 1 │  │ Thread 2 │  │ Thread 3 │                     │
│  │          │  │          │  │          │                     │
│  │ Own Stack│  │ Own Stack│  │ Own Stack│  ◄── Each thread.   │
│  │ Own PC   │  │ Own PC   │  │ Own PC   │      has its own    │
│  │ Own Regs │  │ Own Regs │  │ Own Regs │      stack &        │
│  └──────────┘  └──────────┘  └──────────┘      registers      │
│                                                               │
│  ┌──────────────────────────────────────────────────────┐     │
│  │              SHARED (all threads see)                │     │
│  │  Heap memory, Global variables, File descriptors,    │     │
│  │  Code segment, Open sockets                          │     │
│  └──────────────────────────────────────────────────────┘     │
│                                                               │
│  Process has: Own virtual address space, own PID,             │
│               own file descriptor table, own memory map       │
└───────────────────────────────────────────────────────────────┘
```

**Key differences:**
| | Process | Thread |
|:--|:--------|:-------|
| Memory | Isolated address space | Shares process memory |
| Creation cost | Expensive (fork + copy-on-write) | Cheap (shared address space) |
| Communication | IPC (pipes, sockets, shared memory) | Direct memory access (need synchronization) |
| Crash impact | One process crash doesn't affect others | One thread crash can kill entire process |
| Context switch | Expensive (TLB flush, page table swap) | Cheaper (same address space) |

### Virtual Memory

```
Process sees:                     Physical Reality:
┌──────────────┐                 ┌───────────────┐
│   Stack      │ 0xFFFF...       │   RAM         │
│   ↓          │                 │ ┌──────────┐  │
│              │                 │ │ Page 42  │ ◄├─── Process A's page 7
│              │                 │ ├──────────┤  │
│   ↑          │                 │ │ Page 43  │ ◄├─── Process B's page 2
│   Heap       │                 │ ├──────────┤  │
│              │                 │ │ (free)   │  │
│   BSS        │                 │ ├──────────┤  │
│   Data       │                 │ │ Page 45  │ ◄├─── Process A's page 3
│   Text/Code  │ 0x0000...       │ └──────────┘  │
└──────────────┘                 │               │
                                 │   Disk (Swap) │
Each process thinks it           │ ┌──────────┐  │
has the ENTIRE address           │ │ Swapped  │  │
space to itself.                 │ │ pages    │  │
                                 │ └──────────┘  │
Page Table maps virtual          └───────────────┘
addresses → physical frames
```

**Page fault:** Process accesses a page not in RAM → OS loads it from disk → expensive (~10ms for HDD).  
**TLB (Translation Lookaside Buffer):** Cache for page table entries. TLB miss → walk page table (slow).

### Context Switch

```
Running Process A on CPU:
  1. Timer interrupt fires (or syscall/IO block)
  2. Save A's registers, PC, stack pointer → A's PCB
  3. Load B's registers, PC, stack pointer from B's PCB
  4. Flush TLB (if different address space)
  5. Resume Process B

Cost: ~1-10 microseconds (Linux)
Why it matters: Too many context switches = wasted CPU time = throughput drop
```

### Concurrency Primitives

**Mutex (Mutual Exclusion):**
- Only ONE thread can hold the lock at a time.
- `lock()` → critical section → `unlock()`
- If locked, other threads BLOCK (sleep) until released.

**Semaphore:**
- A counter. `acquire()` decrements, `release()` increments.
- Binary semaphore (0 or 1) ≈ mutex.
- Counting semaphore: allows N threads in simultaneously (e.g., connection pool of 10).

**Condition Variable:**
- `wait()`: release lock and sleep until signaled.
- `signal()`: wake up ONE waiting thread.
- `broadcast()`: wake up ALL waiting threads.
- Always used WITH a mutex to avoid race conditions.

**Deadlock conditions (all 4 must hold):**
1. Mutual exclusion
2. Hold and wait
3. No preemption
4. Circular wait

### I/O Models (Critical for Distributed Systems)

```
1. Blocking I/O:
   Thread calls read() → blocks until data arrives → wastes thread

2. Non-blocking I/O:
   Thread calls read() → returns immediately (EAGAIN if no data)
   Thread must poll repeatedly → wastes CPU

3. I/O Multiplexing (select/poll/epoll):
   Thread monitors MANY file descriptors at once
   Blocks until ANY of them is ready → handles that one
   Single thread can manage thousands of connections
   
   Used by: Node.js event loop, Nginx, Redis, Netty

4. Async I/O (io_uring, IOCP):
   Submit I/O request → kernel handles it → notifies when done
   Thread does other work in between
   Most efficient, least widely used

Why this matters:
  - Node.js = single-threaded + epoll event loop
  - Java Tomcat = thread-per-request (blocking I/O)
  - Java Netty = event loop (non-blocking I/O)
  - Spring WebFlux = Netty-based (non-blocking, reactive)
```

### File Descriptors

```
Every process has a file descriptor table:

fd 0 → stdin  (keyboard)
fd 1 → stdout (terminal)
fd 2 → stderr (terminal)
fd 3 → /var/log/app.log (opened file)
fd 4 → TCP socket to 10.0.0.5:5432 (PostgreSQL connection)
fd 5 → TCP socket to 10.0.0.6:6379 (Redis connection)
fd 6 → Unix socket /tmp/mysql.sock

Everything in Unix is a file:
  Regular files, directories, sockets, pipes, devices → all accessed via fd
```

---

## 3. Networking Foundations

### The TCP/IP Model (What Actually Matters)

```
┌─────────────────────────────────────────────────────────────┐
│ Layer 4: Application    │ HTTP, gRPC, DNS, SMTP, WebSocket  │
├─────────────────────────┼───────────────────────────────────┤
│ Layer 3: Transport      │ TCP (reliable) / UDP (fast)       │
├─────────────────────────┼───────────────────────────────────┤
│ Layer 2: Internet       │ IP (addressing, routing)          │
├─────────────────────────┼───────────────────────────────────┤
│ Layer 1: Link           │ Ethernet, Wi-Fi (MAC addresses)   │
└─────────────────────────┴───────────────────────────────────┘

Data flows down on send, up on receive:
  HTTP Request → TCP segment → IP packet → Ethernet frame → wire
```

### TCP — The Reliable Transport

**Three-way handshake (connection setup):**
```
Client                    Server
  │                         │
  │──── SYN (seq=100) ─────►│  1. Client wants to connect
  │                         │
  │◄── SYN-ACK (seq=300, ───│  2. Server acknowledges + its own SYN
  │     ack=101)            │
  │                         │
  │──── ACK (ack=301) ─────►│  3. Client acknowledges
  │                         │
  │   Connection ESTABLISHED│
```

**Why this matters:** Every new TCP connection costs 1 RTT (round-trip time). If RTT is 100ms, that's 100ms overhead per connection. This is why **connection pooling** exists (reuse connections instead of creating new ones).

**Four-way teardown:**
```
Client                    Server
  │──── FIN ───────────────►│
  │◄─── ACK ────────────────│
  │◄─── FIN ────────────────│
  │──── ACK ───────────────►│
  │                         │
  │  TIME_WAIT (2*MSL)      │  Client waits ~60s before releasing port
```

**TIME_WAIT:** The reason you see "Address already in use" errors. Fix: `SO_REUSEADDR` socket option.

**TCP guarantees:**
- Ordered delivery (sequence numbers)
- Reliable delivery (acknowledgments + retransmission)
- Flow control (receiver window — "don't send more than I can buffer")
- Congestion control (slow start → congestion avoidance → don't overwhelm the network)

### UDP — The Fast Transport
- No connection setup (no handshake)
- No ordering guarantee
- No delivery guarantee
- No congestion control
- **Used for:** DNS, video streaming, gaming, VoIP, health checks

### DNS — Domain Name Resolution

```
Browser types "api.example.com"
         │
         ▼
  1. Check browser cache
  2. Check OS cache (/etc/hosts)
  3. Query recursive resolver (ISP or 8.8.8.8)
         │
         ▼
  4. Root nameserver → "ask .com TLD"
  5. .com TLD server → "ask ns1.example.com"
  6. Authoritative nameserver → "api.example.com = 93.184.216.34"
         │
         ▼
  7. Cache the result (TTL = 300 seconds)
  8. Return IP address

Record types:
  A     → domain → IPv4 address
  AAAA  → domain → IPv6 address
  CNAME → domain → another domain (alias)
  MX    → domain → mail server
  TXT   → domain → arbitrary text (SPF, DKIM verification)
  NS    → domain → authoritative nameserver
  SRV   → service discovery (used by Consul, K8s)
```

### IP Addresses & Subnets

```
IPv4: 192.168.1.100 (32 bits, 4 octets)
IPv6: 2001:0db8:85a3::8a2e:0370:7334 (128 bits)

CIDR notation:
  10.0.0.0/24 = 10.0.0.0 to 10.0.0.255 (256 addresses)
  10.0.0.0/16 = 10.0.0.0 to 10.0.255.255 (65,536 addresses)
  10.0.0.0/8  = 10.0.0.0 to 10.255.255.255 (16.7M addresses)

Private ranges (not routable on internet):
  10.0.0.0/8      (Class A — used by most cloud VPCs)
  172.16.0.0/12   (Class B)
  192.168.0.0/16  (Class C — home routers)
  127.0.0.0/8     (Loopback — localhost)
```

### Ports

```
Well-known ports (0-1023):
  22   → SSH
  53   → DNS
  80   → HTTP
  443  → HTTPS
  5432 → PostgreSQL
  3306 → MySQL
  6379 → Redis
  9092 → Kafka
  27017 → MongoDB
  2181 → ZooKeeper
  8080 → Common app server port

Ephemeral ports (49152-65535):
  Client-side ports assigned by OS for outgoing connections
```

### HTTP — The Application Protocol

```
HTTP Request:
  GET /api/users/123 HTTP/1.1
  Host: api.example.com
  Authorization: Bearer eyJhbGci...
  Accept: application/json
  Content-Type: application/json

HTTP Response:
  HTTP/1.1 200 OK
  Content-Type: application/json
  Content-Length: 42
  
  {"id": 123, "name": "Ritik Raj"}
```

**HTTP Methods:**
| Method | Idempotent? | Safe? | Use |
|:-------|:------------|:------|:----|
| GET | ✅ Yes | ✅ Yes | Read data |
| POST | ❌ No | ❌ No | Create resource |
| PUT | ✅ Yes | ❌ No | Replace entire resource |
| PATCH | ❌ No | ❌ No | Partial update |
| DELETE | ✅ Yes | ❌ No | Remove resource |

**Idempotent** = calling it N times has the same effect as calling it once. Critical for distributed systems (retries are safe).

**Status Codes:**
```
2xx Success:  200 OK, 201 Created, 204 No Content
3xx Redirect: 301 Moved Permanently, 302 Found (temporary), 304 Not Modified
4xx Client Error: 400 Bad Request, 401 Unauthorized, 403 Forbidden, 404 Not Found, 
                  409 Conflict, 429 Too Many Requests
5xx Server Error: 500 Internal Server Error, 502 Bad Gateway, 503 Service Unavailable,
                  504 Gateway Timeout
```

### HTTP/2 & HTTP/3

```
HTTP/1.1:
  One request per TCP connection (or limited pipelining)
  Head-of-line blocking
  Text-based headers (repeated, verbose)

HTTP/2 (used by gRPC):
  Multiple streams over ONE TCP connection (multiplexing)
  Binary framing (not text)
  Header compression (HPACK)
  Server push
  Still uses TCP → TCP-level head-of-line blocking remains

HTTP/3:
  Uses QUIC (UDP-based) instead of TCP
  Eliminates TCP head-of-line blocking
  Built-in TLS 1.3
  Faster connection setup (0-RTT)
```

### WebSockets

```
HTTP Request (Upgrade):
  GET /chat HTTP/1.1
  Upgrade: websocket
  Connection: Upgrade
  Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==

HTTP Response (101 Switching Protocols):
  HTTP/1.1 101 Switching Protocols
  Upgrade: websocket
  Connection: Upgrade

After handshake:
  Full-duplex, persistent connection
  Both client and server can send messages at any time
  No request-response pattern — true bidirectional

Used by: Real-time chat, live dashboards, multiplayer games, stock tickers
Your WebChat project uses Socket.IO (WebSocket + fallbacks)
```

---

## 4. Database Foundations

### SQL — The Language

```sql
-- CREATE
CREATE TABLE users (
    id          SERIAL PRIMARY KEY,
    name        VARCHAR(255) NOT NULL,
    email       VARCHAR(255) UNIQUE NOT NULL,
    created_at  TIMESTAMP DEFAULT NOW()
);

-- INSERT
INSERT INTO users (name, email) VALUES ('Ritik', 'ritik@example.com');

-- SELECT (with JOIN)
SELECT o.id, o.total, u.name
FROM orders o
JOIN users u ON o.user_id = u.id
WHERE o.status = 'PENDING'
ORDER BY o.created_at DESC
LIMIT 10;

-- UPDATE
UPDATE orders SET status = 'SHIPPED' WHERE id = 123;

-- DELETE
DELETE FROM orders WHERE status = 'CANCELLED' AND created_at < NOW() - INTERVAL '30 days';

-- INDEX (makes queries fast)
CREATE INDEX idx_orders_status ON orders(status);
CREATE INDEX idx_orders_user_status ON orders(user_id, status);  -- composite index
```

### ACID Properties

```
A — Atomicity:    All or nothing. Either ALL operations in a transaction
                  succeed, or NONE do. No partial state.

C — Consistency:  Transaction moves DB from one valid state to another.
                  All constraints (foreign keys, unique, check) are satisfied.

I — Isolation:    Concurrent transactions don't interfere with each other.
                  (How strictly depends on isolation level — see below)

D — Durability:   Once committed, data survives crashes.
                  Written to WAL (Write-Ahead Log) on disk before commit.
```

### Isolation Levels

```
┌─────────────────────┬────────────────┬──────────────────────┬──────────────┐
│ Isolation Level     │ Dirty Read?    │ Non-Repeatable Read? │ Phantom      │
│                     │                │                      │ Read?        │
├─────────────────────┼────────────────┼──────────────────────┼──────────────┤
│ READ UNCOMMITTED    │ ✅ Possible    │     ✅ Possible       │ ✅ Possible  │
│ READ COMMITTED      │ ❌ Prevented   │     ✅ Possible       │ ✅ Possible  │
│ REPEATABLE READ     │ ❌ Prevented   │     ❌ Prevented      │ ✅ Possible  │
│ SERIALIZABLE        │ ❌ Prevented   │     ❌ Prevented      │ ❌ Prevented │
└─────────────────────┴────────────────┴──────────────────────┴──────────────┘

Dirty Read:          Reading uncommitted data from another transaction
Non-Repeatable Read: Same query returns different results within one transaction
Phantom Read:        New rows appear between two identical queries

PostgreSQL default: READ COMMITTED
MySQL InnoDB default: REPEATABLE READ
```

### Indexes — How Databases Search Fast

```
Without index (Full Table Scan):
  SELECT * FROM users WHERE email = 'ritik@example.com';
  → Scans ALL 10 million rows. O(n). Slow.

With B-Tree index on email:
  → Binary search through B-Tree. O(log n). ~20 disk reads for 10M rows.

Composite index: CREATE INDEX ON orders(user_id, status);
  → Fast for: WHERE user_id = 5 AND status = 'PENDING'
  → Fast for: WHERE user_id = 5  (leftmost prefix)
  → NOT fast for: WHERE status = 'PENDING' alone  (not leftmost)
```

### SQL vs NoSQL — When to Use What

| | SQL (PostgreSQL, MySQL) | NoSQL (MongoDB, DynamoDB, Cassandra) |
|:--|:---|:---|
| Schema | Fixed schema, migrations | Flexible/schema-less |
| Queries | Complex JOINs, aggregations | Simple key-based lookups |
| Transactions | Full ACID | Limited (some support it) |
| Scaling | Vertical (scale up) + read replicas | Horizontal (shard natively) |
| Consistency | Strong by default | Eventual by default |
| Best for | Complex relationships, financial data | High throughput, flexible schemas, caching |

### Connection Pooling

```
Without pool:                    With pool (e.g., HikariCP):
  Request 1 → open connection    Request 1 → borrow from pool
  Request 2 → open connection    Request 2 → borrow from pool
  Request 3 → open connection    Request 3 → borrow from pool
  ...                            ...
  Each open = TCP handshake      Connections are pre-created
  + TLS negotiation              and reused. ~0ms overhead.
  + auth. ~50-100ms each.

Pool sizing rule of thumb:
  connections = (core_count * 2) + effective_spindle_count
  For SSD: ~10-20 connections per service instance is usually optimal
```

---

## 5. Java Language Essentials

### Object-Oriented Programming

```java
// Class = blueprint
public class Order {
    // Fields (state)
    private String orderId;
    private BigDecimal amount;
    private OrderStatus status;
    
    // Constructor
    public Order(String orderId, BigDecimal amount) {
        this.orderId = orderId;
        this.amount = amount;
        this.status = OrderStatus.CREATED;
    }
    
    // Method (behavior)
    public void ship() {
        if (this.status != OrderStatus.PAID) {
            throw new IllegalStateException("Cannot ship unpaid order");
        }
        this.status = OrderStatus.SHIPPED;
    }
    
    // Getters (encapsulation)
    public String getOrderId() { return orderId; }
    public OrderStatus getStatus() { return status; }
}

// Inheritance
public class ExpressOrder extends Order {
    private LocalDate guaranteedDelivery;
    
    @Override
    public void ship() {
        super.ship();
        this.guaranteedDelivery = LocalDate.now().plusDays(1);
    }
}

// Interface (contract)
public interface PaymentProcessor {
    PaymentResult charge(BigDecimal amount, String customerId);
}

// Polymorphism
public class StripeProcessor implements PaymentProcessor {
    @Override
    public PaymentResult charge(BigDecimal amount, String customerId) {
        // Stripe-specific implementation
    }
}
```

**Four pillars:** Encapsulation (private fields + public methods), Inheritance (extends/implements), Polymorphism (same interface, different behavior), Abstraction (hiding complexity).

### Java Collections Framework

```
┌────────────────────────────────────────────────────────────────┐
│                    Collection Hierarchy                        │
│                                                                │
│  Iterable                                                      │
│  └── Collection                                                │
│      ├── List (ordered, duplicates allowed)                    │
│      │   ├── ArrayList   — backed by array, O(1) random        │
│      │   │                 access, O(n) insert in middle       │
│      │   └── LinkedList  — backed by doubly-linked list,       │
│      │                     O(1) insert/remove, O(n) access     │
│      ├── Set (no duplicates)                                   │
│      │   ├── HashSet     — O(1) add/contains, unordered        │
│      │   ├── LinkedHashSet — O(1) + insertion order            │
│      │   └── TreeSet     — O(log n), sorted (Red-Black Tree)   │
│      └── Queue                                                 │
│          ├── LinkedList   — also implements Queue              │
│          ├── PriorityQueue — min-heap, O(log n) poll           │
│          └── ArrayDeque   — double-ended, faster than Stack    │
│                                                                │
│  Map (key-value pairs, NOT a Collection)                       │
│  ├── HashMap        — O(1) get/put, unordered                  │
│  ├── LinkedHashMap  — O(1) + insertion order                   │
│  ├── TreeMap        — O(log n), sorted keys (Red-Black Tree)   │
│  └── ConcurrentHashMap — thread-safe HashMap                   │
└────────────────────────────────────────────────────────────────┘
```

### Generics

```java
// Type-safe container
public class Result<T> {
    private final T data;
    private final String error;
    
    public static <T> Result<T> success(T data) {
        return new Result<>(data, null);
    }
    
    public static <T> Result<T> failure(String error) {
        return new Result<>(null, error);
    }
}

// Usage:
Result<Order> orderResult = Result.success(new Order("123", new BigDecimal("99.99")));
Result<List<User>> usersResult = Result.success(userList);
```

### Lambda & Streams (Java 8+)

```java
// Lambda = anonymous function
List<Order> pendingOrders = orders.stream()
    .filter(o -> o.getStatus() == OrderStatus.PENDING)     // filter
    .sorted(Comparator.comparing(Order::getCreatedAt))      // sort
    .limit(10)                                               // take first 10
    .collect(Collectors.toList());                           // collect to list

// Map + reduce
BigDecimal totalRevenue = orders.stream()
    .filter(o -> o.getStatus() == OrderStatus.PAID)
    .map(Order::getAmount)
    .reduce(BigDecimal.ZERO, BigDecimal::add);

// Grouping
Map<OrderStatus, List<Order>> byStatus = orders.stream()
    .collect(Collectors.groupingBy(Order::getStatus));
```

### JVM Memory Model (Simplified)

```
┌─────────────────────────────────────────────────┐
│                  JVM Memory                     │
│                                                 │
│  ┌─────────────────────────────────┐            │
│  │         Heap (shared)           │            │
│  │  ┌───────────┬───────────────┐  │            │
│  │  │  Young Gen│   Old Gen     │  │            │
│  │  │  (Eden +  │   (long-lived │  │            │
│  │  │  Survivor)│    objects)   │  │            │
│  │  └───────────┴───────────────┘  │            │
│  └─────────────────────────────────┘            │
│                                                 │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐       │
│  │ Stack    │  │ Stack    │  │ Stack    │       │
│  │ Thread-1 │  │ Thread-2 │  │ Thread-3 │       │
│  │ (local   │  │ (local   │  │ (local   │       │
│  │  vars,   │  │  vars,   │  │  vars,   │       │
│  │  frames) │  │  frames) │  │  frames) │       │
│  └──────────┘  └──────────┘  └──────────┘       │
│                                                 │
│  ┌─────────────────────────────────┐            │
│  │      Metaspace (class metadata) │            │
│  └─────────────────────────────────┘            │
└─────────────────────────────────────────────────┘

Garbage Collection:
  Minor GC: Cleans Young Gen (fast, frequent, ~10ms)
  Major GC: Cleans Old Gen (slow, infrequent, ~100ms-1s+)
  
  GC Algorithms: G1 (default Java 9+), ZGC (low-latency), Shenandoah
  
  Why this matters for distributed systems:
    Long GC pauses → missed heartbeats → false failure detection
    → unnecessary leader elections / consumer group rebalances
```

### Exception Handling

```java
// Checked exceptions: MUST handle (IOException, SQLException)
// Unchecked exceptions: RuntimeException subclasses (NullPointerException, IllegalArgumentException)

try {
    Order order = orderRepository.findById(id)
        .orElseThrow(() -> new OrderNotFoundException("Order not found: " + id));
    paymentService.charge(order);
} catch (PaymentException e) {
    log.error("Payment failed for order {}: {}", id, e.getMessage());
    throw new OrderProcessingException("Payment failed", e);  // Wrap and rethrow
} finally {
    // Always runs — cleanup resources
}

// Try-with-resources (auto-close)
try (Connection conn = dataSource.getConnection();
     PreparedStatement stmt = conn.prepareStatement(sql)) {
    stmt.setString(1, userId);
    ResultSet rs = stmt.executeQuery();
    // ...
}  // Connection and statement automatically closed
```

---

## 6. Spring Boot Essentials

### The Core Idea: Inversion of Control (IoC)

```
Without Spring (you manage dependencies):
  PaymentService paymentService = new StripePaymentService(
      new HttpClient(),
      new ApiKeyProvider("sk_live_xxx")
  );
  OrderService orderService = new OrderService(
      new OrderRepository(dataSource),
      paymentService
  );

With Spring (framework manages dependencies):
  @Service
  public class OrderService {
      private final OrderRepository orderRepository;
      private final PaymentService paymentService;
      
      // Spring finds and injects the right beans automatically
      public OrderService(OrderRepository repo, PaymentService payment) {
          this.orderRepository = repo;
          this.paymentService = payment;
      }
  }
```

### Key Annotations

```java
@SpringBootApplication    // Entry point (= @Configuration + @EnableAutoConfiguration + @ComponentScan)
@RestController           // REST API controller (returns JSON, not views)
@Service                  // Business logic layer
@Repository              // Data access layer (adds DB exception translation)
@Component               // Generic Spring-managed bean
@Autowired               // Inject dependency (prefer constructor injection)
@Bean                    // Factory method that produces a Spring bean
@Configuration           // Class that contains @Bean definitions
@Value("${app.name}")    // Inject from application.properties
@Transactional           // Wrap method in a database transaction
@Async                   // Run method in a separate thread
@Scheduled               // Run method on a schedule (cron, fixed rate)
```

### Layered Architecture

```
┌─────────────────────────────────────────────────┐
│              @RestController                    │
│  Handles HTTP, validates input, returns DTOs    │
├─────────────────────────────────────────────────┤
│              @Service                           │
│  Business logic, orchestration, transactions    │
├─────────────────────────────────────────────────┤
│              @Repository                        │
│  Data access (JPA/Hibernate or JDBC)            │
├─────────────────────────────────────────────────┤
│              Database                           │
└─────────────────────────────────────────────────┘

DTO = Data Transfer Object (what the API returns)
Entity = Database-mapped object (JPA @Entity)
Never expose entities directly in API responses.
```

---

## 7. JavaScript / Node.js Essentials

### Event Loop — The Heart of Node.js

```
Node.js is SINGLE-THREADED (for JS code execution).
It handles concurrency via an EVENT LOOP, not threads.

   ┌────────────────────────────────┐
   │         Call Stack             │  ← Executes JS code (one thing at a time)
   └───────────┬────────────────────┘
               │
   ┌───────────▼─────────────────────┐
   │        Event Loop               │  ← Checks: "Any callbacks ready?"
   │                                 │
   │  Phases:                        │
   │  1. Timers (setTimeout)         │
   │  2. I/O callbacks               │
   │  3. Poll (new I/O events)       │
   │  4. Check (setImmediate)        │
   │  5. Close callbacks             │
   └───────────┬─────────────────────┘
               │
   ┌───────────▼───────────────────┐
   │    Thread Pool (libuv, 4 th)  │  ← Heavy I/O (file read, DNS, crypto)
   │    + OS async APIs (epoll)    │  ← Network I/O (non-blocking)
   └───────────────────────────────┘
```

### Async/Await & Promises

```javascript
// Promise = a value that will resolve in the future
const fetchUser = (id) => {
    return new Promise((resolve, reject) => {
        db.query('SELECT * FROM users WHERE id = $1', [id], (err, result) => {
            if (err) reject(err);
            else resolve(result.rows[0]);
        });
    });
};

// Async/await = cleaner syntax for Promises
async function getOrderWithUser(orderId) {
    try {
        const order = await orderRepository.findById(orderId);
        const user = await userRepository.findById(order.userId);
        return { order, user };
    } catch (error) {
        console.error('Failed:', error);
        throw error;
    }
}

// Promise.all = run in parallel
const [orders, users, products] = await Promise.all([
    fetchOrders(),
    fetchUsers(),
    fetchProducts()
]);
```

---

## 8. Linux Essentials

### Essential Commands

```bash
# Process management
ps aux                    # List all processes
top / htop                # Real-time process monitor
kill -9 <PID>             # Force kill process
kill -15 <PID>            # Graceful shutdown (SIGTERM)

# File system
ls -la                    # List files with permissions
find / -name "*.log"      # Find files by name
grep -r "error" /var/log  # Search file contents recursively
tail -f /var/log/app.log  # Follow log file in real-time
df -h                     # Disk space usage
du -sh /path              # Directory size

# Networking
curl -X GET https://api.example.com/users    # HTTP request
netstat -tlnp / ss -tlnp                     # List listening ports
dig example.com                               # DNS lookup
traceroute example.com                        # Network path
ip addr                                       # Network interfaces

# System info
uname -a                  # Kernel version
free -h                   # Memory usage
lscpu                     # CPU info
uptime                    # System uptime + load average
```

### File Permissions

```
-rwxr-xr-- 1 ritik developers 4096 Oct 7 app.jar

 │ │  │  │
 │ │  │  └── Others: r-- (read only)
 │ │  └───── Group:  r-x (read + execute)
 │ └──────── Owner:  rwx (read + write + execute)
 └────────── Type:   - (regular file), d (directory), l (symlink)

Numeric: rwx = 4+2+1 = 7, r-x = 4+0+1 = 5, r-- = 4+0+0 = 4
chmod 754 app.jar   # Owner=rwx, Group=r-x, Others=r--
chown ritik:devs app.jar  # Change owner and group
```

### Systemd & Services

```bash
# Managing services
systemctl start nginx        # Start service
systemctl stop nginx         # Stop service
systemctl restart nginx      # Restart
systemctl status nginx       # Check status
systemctl enable nginx       # Start on boot
journalctl -u nginx -f       # View service logs

# This is how services run in production Linux servers
# Docker containers replace this with container orchestration
```

---

## 9. API Design & HTTP

### REST API Design Conventions

```
Resources are nouns, not verbs:

✅ GET    /api/users              → List users
✅ GET    /api/users/123          → Get user 123
✅ POST   /api/users              → Create user
✅ PUT    /api/users/123          → Replace user 123
✅ PATCH  /api/users/123          → Update user 123 partially
✅ DELETE /api/users/123          → Delete user 123

❌ GET    /api/getUser?id=123     → Don't use verbs
❌ POST   /api/deleteUser         → Use HTTP methods, not URL verbs

Nested resources:
✅ GET    /api/users/123/orders   → Get orders for user 123
✅ POST   /api/users/123/orders   → Create order for user 123
```

### Request/Response Patterns

```json
// Successful response
{
    "data": {
        "id": 123,
        "name": "Ritik Raj",
        "email": "ritik@example.com"
    }
}

// Error response
{
    "error": {
        "code": "USER_NOT_FOUND",
        "message": "User with ID 999 does not exist",
        "timestamp": "2026-10-07T18:00:00Z"
    }
}

// Paginated list
{
    "data": [...],
    "pagination": {
        "page": 1,
        "pageSize": 20,
        "totalItems": 342,
        "totalPages": 18
    }
}
```

---

## 10. Cloud & Infrastructure Vocabulary

```
┌────────────────────────────────────────────────────────────────┐
│                    Key Concepts                                │
├──────────────┬─────────────────────────────────────────────────┤
│ Region       │ Geographic area (us-east-1, ap-south-1)         │
│ AZ           │ Isolated data center within a region            │
│ VPC          │ Your private network in the cloud               │
│ Subnet       │ Subdivision of VPC (public or private)          │
│ Security Grp │ Firewall rules (allow/deny by port + IP)        │
│ IAM          │ Identity & Access Management (who can do what)  │
│ Load Balancer│ Distributes traffic across instances            │
│ Auto Scaling │ Add/remove instances based on demand            │
│ CDN          │ Cache content at edge locations (CloudFront)    │
│ Object Store │ File storage (S3) — not a filesystem            │
│ Serverless   │ Code runs without managing servers (Lambda)     │
│ IaC          │ Infrastructure as Code (Terraform, CDK)         │
└──────────────┴─────────────────────────────────────────────────┘
```

### Container vs VM

```
Virtual Machine:                    Container:
┌─────────────────────┐            ┌─────────────────────────┐
│  App A    │  App B  │            │  App A    │  App B      │
├───────────┼─────────┤            ├───────────┼─────────────┤
│  Guest OS │Guest OS │            │  Bins/    │  Bins/      │
│  (full)   │(full)   │            │  Libs     │  Libs       │
├───────────┴─────────┤            ├───────────┴─────────────┤
│     Hypervisor      │            │   Container Runtime     │
├─────────────────────┤            │   (Docker Engine)       │
│     Host OS         │            ├─────────────────────────┤
├─────────────────────┤            │     Host OS (shared)    │
│     Hardware        │            ├─────────────────────────┤
└─────────────────────┘            │     Hardware            │
                                   └─────────────────────────┘
VM: Full OS copy per app           Container: Shares host kernel
    ~GB of overhead                    ~MB of overhead
    Minutes to start                   Seconds to start
    Strong isolation                   Process-level isolation
```

---

## 11. Distributed Systems Vocabulary

| Term | Definition |
|:-----|:-----------|
| **Latency** | Time for a single request (client → server → client). Measured in ms. |
| **Throughput** | Requests handled per second (RPS/QPS). |
| **Availability** | % of time system is operational. 99.9% = 8.7 hours downtime/year. |
| **Scalability** | Ability to handle increased load. Vertical (bigger machine) vs Horizontal (more machines). |
| **Partition** | Network split — nodes can't communicate but are individually healthy. |
| **Replication** | Keeping copies of data on multiple nodes for fault tolerance. |
| **Sharding** | Splitting data across multiple nodes for capacity. |
| **Consensus** | Agreement among distributed nodes on a single value (Raft, Paxos). |
| **Idempotency** | Doing something N times has the same effect as doing it once. |
| **Eventual Consistency** | All replicas converge to the same state, given enough time without new updates. |
| **Strong Consistency** | Every read returns the most recent write. |
| **Failover** | Automatic switching to a backup when the primary fails. |
| **Backpressure** | Consumer telling producer to slow down (I can't keep up). |
| **Fan-out** | One event triggers work to many recipients. |
| **SLA/SLO/SLI** | Agreement / Objective / Indicator for service reliability. |

### The Numbers Every Engineer Should Know

```
L1 cache reference:                    1 ns
L2 cache reference:                    4 ns
Main memory (RAM) reference:         100 ns
SSD random read:                  15,000 ns  (15 µs)
Read 1 MB sequentially from SSD: 49,000 ns  (49 µs)
HDD seek:                     2,000,000 ns  (2 ms)
Read 1 MB sequentially from HDD:1,000,000 ns (1 ms)
Send packet same datacenter:      500,000 ns (0.5 ms)
Round trip same datacenter:     1,000,000 ns  (1 ms)
Round trip US-East ↔ US-West:  40,000,000 ns  (40 ms)
Round trip US ↔ Europe:        80,000,000 ns  (80 ms)

Key insight: Network is 1000x slower than RAM. Disk is 100x slower than RAM.
Design to minimize network round trips and disk seeks.
```

---

## 12. Serialization & Data Formats

```
JSON (Human-readable, universal):
  {"name": "Ritik", "age": 22}
  Used by: REST APIs, config files, MongoDB

Protocol Buffers (Binary, schema-based, compact):
  message User { string name = 1; int32 age = 2; }
  Used by: gRPC, Kafka (optional), Google internal

Avro (Binary, schema evolution):
  Reader and writer schemas can differ
  Used by: Kafka (Confluent Schema Registry), Hadoop

MessagePack (Binary JSON):
  Compact binary version of JSON
  Used by: Redis (internal), some APIs

YAML (Human-readable, superset of JSON):
  name: Ritik
  age: 22
  Used by: Docker Compose, Kubernetes, GitHub Actions, Spring Boot config
```

---

## 13. Cryptography & Security Basics

### Hashing vs Encryption

```
Hashing (ONE-WAY):
  SHA-256("password123") = "ef92b778..."
  Cannot reverse. Same input = same output.
  Used for: Password storage, data integrity, checksums, Bloom filters

Encryption (TWO-WAY):
  Symmetric: AES-256 (same key encrypts and decrypts) — fast
  Asymmetric: RSA/ECDSA (public key encrypts, private key decrypts) — slow
  Used for: TLS/HTTPS, JWT signing, data at rest

TLS Handshake (simplified):
  Client → Server: "Hello, I support these ciphers"
  Server → Client: "Let's use AES-256. Here's my certificate."
  Client verifies certificate against trusted CAs
  Both derive a shared session key (via Diffie-Hellman)
  All subsequent data encrypted with AES-256
```

### JWT (JSON Web Token)

```
Header.Payload.Signature

Header:  {"alg": "HS256", "typ": "JWT"}
Payload: {"sub": "user123", "role": "admin", "exp": 1696694400}
Signature: HMAC-SHA256(base64(header) + "." + base64(payload), secret)

Key facts:
  - NOT encrypted (anyone can read the payload by base64-decoding)
  - IS signed (tampering is detectable)
  - Stateless: server doesn't need to store sessions
  - Expiration: set short-lived access tokens + refresh tokens
```

---

## 14. Git & Version Control

```bash
# Core workflow
git clone <repo>           # Clone repository
git checkout -b feature    # Create and switch to new branch
git add .                  # Stage all changes
git commit -m "message"    # Commit staged changes
git push origin feature    # Push branch to remote
git pull origin main       # Pull latest changes from main

# Branching strategy (GitFlow simplified)
main       ← production-ready code
  └── develop ← integration branch
        ├── feature/user-auth
        ├── feature/payment
        └── bugfix/login-crash

# Merge vs Rebase
git merge feature    # Creates merge commit (preserves history)
git rebase main      # Replays commits on top of main (linear history)

# Common fixes
git stash            # Save uncommitted changes temporarily
git stash pop        # Restore stashed changes
git reset --soft HEAD~1  # Undo last commit (keep changes staged)
git reset --hard HEAD~1  # Undo last commit (DISCARD changes)
```

---

## 15. Prerequisite Map — What to Read Before What

```
READ ORDER (follow the arrows):

 Sections 1-4 of THIS FILE (DS, OS, Networking, DB foundations)
       │
       ├──► distributed-systems-notes.md
       │         │
       │         └──► microservices-backend-interview-notes.md
       │                    │
       │                    ├──► kafka-event-driven-deep-dive.md
       │                    │         │
       │                    │         └──► (needs Java: Section 5 of this file)
       │                    │
       │                    └──► system-design-case-studies.md
       │
       ├──► Section 5 (Java) + Section 6 (Spring Boot) of THIS FILE
       │         │
       │         └──► java-concurrency-spring-boot-deep-dive.md
       │
       └──► Section 8 (Linux) + Section 10 (Cloud) of THIS FILE
                 │
                 └──► docker-kubernetes-aws-production.md


DEPENDENCY TABLE:
┌──────────────────────────────────────┬──────────────────────────────────┐
│ To Read This File                    │ You Need These Prerequisites     │
├──────────────────────────────────────┼──────────────────────────────────┤
│ distributed-systems-notes.md         │ §1 Data Structures, §3 Network,  │
│                                      │ §4 Database basics               │
├──────────────────────────────────────┼──────────────────────────────────┤
│ microservices-backend-interview-     │ §1-4 (all fundamentals),         │
│ notes.md                             │ §9 API Design, §11 DS Vocab      │
├──────────────────────────────────────┼──────────────────────────────────┤
│ kafka-event-driven-deep-dive.md      │ §5 Java, §6 Spring Boot,         │
│                                      │ §2 OS (threads), §11 DS Vocab    │
├──────────────────────────────────────┼──────────────────────────────────┤
│ java-concurrency-spring-boot-        │ §5 Java (full), §6 Spring Boot,  │
│ deep-dive.md                         │ §2 OS (threads, locks, memory)   │
├──────────────────────────────────────┼──────────────────────────────────┤
│ docker-kubernetes-aws-production.md  │ §8 Linux, §3 Networking,         │
│                                      │ §10 Cloud Vocab, §12 YAML        │
├──────────────────────────────────────┼──────────────────────────────────┤
│ system-design-case-studies.md        │ ALL of the above (capstone file) │
└──────────────────────────────────────┴──────────────────────────────────┘
```

---

> **How to use this file:**  
> 1. Skim through each section — if you already know 80%+, move on.  
> 2. If you find a concept you're shaky on, study that section before reading the deep-dive file that depends on it.  
> 3. Keep this file open as a reference while reading other notes — Ctrl+F is your friend.
