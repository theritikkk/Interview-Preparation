# System Design Case Studies: Comprehensive Interview Notes

**Author:** Ritik Raj (3rd-year B.Tech)
**Background:** Distributed Task Scheduler (RabbitMQ), WebChat (K8s, 6 microservices), Hermes (Event-Sourced Workflow Engine on AWS).
**Prerequisites:** CAP, Raft, 2PC/Saga, Kafka Internals, Microservice Patterns.

---

## Table of Contents
1. [URL Shortener (TinyURL)](#1-url-shortener-tinyurl)
2. [Rate Limiter](#2-rate-limiter)
3. [Distributed Task Scheduler](#3-distributed-task-scheduler)
4. [Real-Time Chat System](#4-real-time-chat-system)
5. [Notification System](#5-notification-system)
6. [News Feed / Timeline](#6-news-feed--timeline)
7. [Distributed Key-Value Store](#7-distributed-key-value-store)
8. [Video Streaming Platform](#8-video-streaming-platform)
9. [E-Commerce Order System](#9-e-commerce-order-system)
10. [Search Autocomplete / Typeahead](#10-search-autocomplete--typeahead)

---

## 1. URL Shortener (TinyURL)

### Requirements
**Functional:**
- Given a long URL, generate a shorter, unique alias.
- When users click the short link, redirect them to the original URL.
- Custom short links (optional but good to mention).
- Links expire after a standard timespan.

**Non-Functional:**
- Highly available (reads are critical).
- URL redirection should happen in real-time with minimal latency.
- Short links should not be predictable.
- Read-heavy system (100:1 read-to-write ratio).

### Back-of-the-Envelope Estimation
- **Traffic:** Assume 100M new URLs generated per month.
- **Reads:** 100M * 100 = 10 Billion redirections per month.
- **QPS:** Write QPS = 100M / (30 * 24 * 3600) ≈ 40 writes/sec. Read QPS = 4000 reads/sec.
- **Storage:** Assume URLs are kept for 10 years. 100M * 12 * 10 = 12 Billion records.
- **Storage Size:** Assume each record is 500 bytes. 12B * 500 bytes = 6 TB.
- **Cache:** Cache 20% of reads. 10B * 20% / 30 days = 66M requests/day to cache. 66M * 500B ≈ 33GB of memory.

### High-Level Architecture
```text
[Client] ---> [Load Balancer]
                    |
          +---------+---------+
          |                   |
   [Web Servers]       [Web Servers]
          |                   |
          +----+---------+----+
               |         |
           [Cache]   [Database]
          (Redis)     (RDBMS)
               |
      [ID Generator Service]
         (Snowflake/Zookeeper)
```

### API Design
- `POST /api/v1/data/shorten`
  - Body: `{ "longUrl": "https://...", "customAlias": "my-alias" (optional), "expireDate": "2024-01-01" }`
  - Returns: `{ "shortUrl": "http://tiny.url/xyz123" }`
- `GET /{shortUrl}`
  - Returns: `HTTP 301 Redirect` to longUrl.

### Data Model (RDBMS)
Using a relational database (PostgreSQL) is fine for 6TB, but NoSQL (Cassandra) is also great for massive horizontal scaling.
**Table: URL_MAP**
- `id` (bigint, PK) - Auto-increment or generated ID.
- `short_url` (varchar(10), Indexed) - Base62 encoded string.
- `long_url` (varchar(2048))
- `created_at` (timestamp)
- `expires_at` (timestamp)
- `user_id` (bigint)

### Deep Dive: ID Generation & Base62 Encoding
We need a unique ID for every long URL. Hashing the long URL (MD5/SHA) has collisions and is too long.
Instead, we generate a unique integer ID and convert it to Base62 (A-Z, a-z, 0-9).
- Base62 with 7 characters: 62^7 = ~3.5 Trillion URLs. (Plenty for our 12B estimation).

**How to get the unique integer?**
1. **Database Auto-Increment:** Hard to scale across multiple DB nodes.
2. **UUID:** 128-bit, too long, not sortable.
3. **Twitter Snowflake:** Generates 64-bit sortable IDs (timestamp + worker ID + sequence number).
4. **ZooKeeper / Etcd Range Allocation:** App servers request a range of IDs (e.g., 1-1000) from Zookeeper. They hand them out locally. When depleted, they fetch the next range. This is highly performant and avoids DB bottlenecks.

### Deep Dive: 301 vs 302 Redirects
- **301 (Moved Permanently):** The browser caches the response. Future requests go directly to the long URL. Reduces server load, but analytics (click tracking) are lost.
- **302 (Found / Moved Temporarily):** Browser does not cache. All clicks go through our servers. Best for analytics.

### Scaling & Bottlenecks
- **Caching:** Redis or Memcached. Store `<short_url, long_url>`. LRU eviction policy. Cache misses hit the DB.
- **Database Sharding:** Shard by the first character of the short URL or use consistent hashing on the short URL to distribute load across DB nodes.
- **Data Cleanup:** Run a background Cron job (distributed task scheduler) or use TTL in DB to purge expired links.

### Interview Follow-ups
**Q: How do you prevent malicious users from generating massive numbers of short links?**
A: Implement an API Gateway with rate limiting (e.g., token bucket per IP/API Key). Require authentication for high-volume users.

**Q: What if the custom alias is already taken?**
A: The DB should have a UNIQUE constraint on the `short_url` column. If a collision occurs, return an error to the user immediately.

**Q: How do you handle cache stampedes for a viral link?**
A: Use a probabilistic early expiration algorithm, or implement a lock (mutex) on the cache key so only one worker fetches from the DB while others wait.

---

## 2. Rate Limiter

### Requirements
**Functional:**
- Limit the number of requests a client can make within a specified time window.
- Exceeding limits returns HTTP 429 (Too Many Requests).
- Rules can be based on IP, User ID, or API Key.

**Non-Functional:**
- Extremely low latency (cannot slow down the main API).
- High throughput.
- Accurate in a distributed environment (across multiple gateway instances).

### Architecture Integration
```text
[Client] --> [API Gateway / Rate Limiter] --> [Microservices]
                         |
                 [Redis Cluster]
```
Rate limiter is typically implemented as middleware in the API Gateway or as a standalone service sidecar.

### Rate Limiting Algorithms
1. **Token Bucket:**
   - A bucket holds $N$ tokens. Tokens are added at a rate of $R$ per second.
   - Each request consumes a token. If empty, request is dropped.
   - *Pros:* Allows bursts of traffic. Memory efficient.
2. **Leaking Bucket:**
   - Requests are added to a queue. Processed at a fixed rate.
   - *Pros:* Smooths out traffic (constant processing rate).
3. **Fixed Window Counter:**
   - Divide time into fixed windows (e.g., 10:00:00 to 10:01:00). Increment counter.
   - *Cons:* Spikes at the edges of windows can allow 2x traffic.
4. **Sliding Window Log:**
   - Keep timestamps of all requests in a Redis Sorted Set.
   - *Cons:* High memory footprint.
5. **Sliding Window Counter:**
   - Hybrid of Fixed Window and Sliding Log. Estimates current window traffic based on overlap.

### Deep Dive: Distributed Rate Limiting with Redis
In a multi-node environment, an in-memory rate limiter per server is inaccurate if load balancers use round-robin. We need a centralized datastore like Redis.

**Token Bucket in Redis (Lua Script):**
Using a Lua script ensures the fetch, check, and update operations are **atomic**, preventing race conditions.
```lua
-- Redis Lua Script for Token Bucket
local tokens_key = KEYS[1]
local timestamp_key = KEYS[2]
local rate = tonumber(ARGV[1])
local capacity = tonumber(ARGV[2])
local now = tonumber(ARGV[3])

local last_tokens = tonumber(redis.call("get", tokens_key))
if last_tokens == nil then
  last_tokens = capacity
end

local last_refreshed = tonumber(redis.call("get", timestamp_key))
if last_refreshed == nil then
  last_refreshed = 0
end

local delta = math.max(0, now - last_refreshed)
local filled_tokens = math.min(capacity, last_tokens + (delta * rate))
local allowed = filled_tokens >= 1
local new_tokens = filled_tokens
if allowed then
  new_tokens = filled_tokens - 1
end

redis.call("setex", tokens_key, 3600, new_tokens)
redis.call("setex", timestamp_key, 3600, now)
return allowed
```

### Scaling Considerations
- **Latency:** Calling Redis for every request adds network overhead.
- **Optimization:** Use a local in-memory cache on the gateway for the rules (which rarely change). Use eventual consistency (batching) if strict accuracy isn't required, or use Redis pipeline.
- **Sharding Redis:** Shard Redis by User ID / IP to handle high throughput.

### Interview Follow-ups
**Q: How do you handle rate limiter failure?**
A: Fail open. If Redis goes down, we should not block all user traffic. Route traffic through, perhaps falling back to a lenient local in-memory rate limiter.

**Q: What happens if a user connects from multiple IPs?**
A: Rate limiting by IP is insufficient. We should rate limit by User ID (if authenticated) or API key. IP is only for unauthenticated DDoS protection.

---

## 3. Distributed Task Scheduler

*(Tied to your Distributed Task Scheduler with RabbitMQ project)*

### Requirements
**Functional:**
- Schedule tasks to execute at a specific time or on a recurring basis (Cron).
- Support for immediate asynchronous task execution.
- Ensure tasks execute exactly-once (or at-least-once with idempotency).

**Non-Functional:**
- High availability and fault tolerance (if a worker dies, task is reassigned).
- Scalable to millions of scheduled tasks.
- Low scheduling latency.

### High-Level Architecture
```text
[Clients] ---> [API Service] ---> [Database (Task Metadata)]
                                         |
                                  [Time-Wheel / Poller]
                                         |
                               [RabbitMQ / Message Broker]
                                         |
                            +------------+------------+
                            |                         |
                    [Worker Node 1]           [Worker Node 2]
```

### Data Model (Task Table)
- `task_id` (UUID, PK)
- `payload` (JSON)
- `execution_time` (Timestamp, Indexed)
- `status` (PENDING, QUEUED, RUNNING, COMPLETED, FAILED)
- `retry_count` (int)
- `lock_id` / `owner_id` (varchar - for worker claiming)

### Deep Dive: Scheduling Mechanisms
**Approach 1: Database Polling (The Naive Way)**
- A cron job runs every minute, queries `SELECT * FROM tasks WHERE execution_time <= NOW() AND status = 'PENDING'`.
- Updates status to QUEUED and pushes to RabbitMQ.
- *Problem:* Heavy DB load, high latency, contention if multiple pollers run.

**Approach 2: TTL and Dead Letter Exchanges (RabbitMQ)**
*(Your project's approach)*
- When a task is scheduled for 5 minutes in the future, publish it to a "Delay Queue" with a TTL of 5 minutes.
- The Delay Queue has NO consumers.
- When the TTL expires, RabbitMQ routes the message to a **Dead Letter Exchange (DLX)**, which routes it to the "Ready Queue".
- Workers consume from the Ready Queue.
- *Pros:* Highly scalable, offloads timing to the message broker.
- *Cons:* Difficult to cancel or modify tasks once they are in the queue (requires tombstoning in DB).

**Approach 3: Hashed Wheel Timer / Redis ZSET**
- Store scheduled tasks in a Redis Sorted Set: `ZADD scheduled_tasks <unix_timestamp> <task_id>`.
- A fast polling service runs `ZRANGEBYSCORE scheduled_tasks 0 <current_timestamp>`.
- Pushes fetched tasks to RabbitMQ and removes them from Redis.

### Deep Dive: Reliability and Exactly-Once Execution
- **At-Least-Once Delivery:** RabbitMQ requires explicit manual ACKs from workers. If a worker crashes mid-execution, the TCP connection drops, and RabbitMQ requeues the message.
- **Idempotency (Exactly-Once semantics):** Because messages can be redelivered, workers MUST be idempotent.
  - Store an `execution_id` or use the `task_id` in a distributed lock (Redis) or use a conditional update in the DB: `UPDATE tasks SET status='RUNNING' WHERE task_id=X AND status='QUEUED'`. If this returns 0 rows updated, another worker is processing it.

### Reconciliation
- What if a worker claims a task, updates DB to RUNNING, but then crashes and the message is lost or in a weird state?
- A background "Reaper" process scans the DB for tasks stuck in RUNNING state for > Timeout. It resets them to PENDING or QUEUED.

### Interview Follow-ups
**Q: How do you handle massive spikes in scheduled tasks (e.g., 1 million users scheduled a task for exactly midnight)?**
A: The "Thundering Herd" problem. If using Redis ZSET, we must chunk the `ZRANGEBYSCORE` reads and parallelize the push to RabbitMQ. Alternatively, jitter (add random seconds) to the execution times if strict accuracy isn't required, smoothing the load.

**Q: How do you cancel a scheduled task that is sitting in a RabbitMQ Delay Queue?**
A: Since we can't easily pluck it out of the queue, we update the task status in the Database to 'CANCELLED'. When the task finally reaches the worker, the worker's first step is to query the DB (or a Redis cache of cancelled IDs) and discard the task if it's cancelled.

**Q: How do you ensure high availability of the scheduler component itself?**
A: Run multiple instances of the scheduler service and use Leader Election (via Zookeeper or Redis distributed lock). Only the leader polls the DB/Redis to prevent duplicate queueing.

---

## 4. Real-Time Chat System

*(Tied to your WebChat K8s microservices project)*

### Requirements
**Functional:**
- 1-on-1 chatting with real-time delivery.
- Group chatting (up to 100 members).
- Online presence (status).
- Chat history (persistent storage).

**Non-Functional:**
- Low latency (under 100ms delivery).
- High concurrency (millions of concurrent connections).
- No message loss.

### High-Level Architecture
```text
      [Client A]                     [Client B]
           \                             /
        (WebSocket)                  (WebSocket)
             \                         /
    +-------------------+    +-------------------+
    | Chat Gateway Pod 1|    | Chat Gateway Pod 2|
    +-------------------+    +-------------------+
             |                         |
             +--------+       +--------+
                      |       |
                 [Redis Pub/Sub]
                      |
              [Message Sync Service]
                      |
           [Cassandra (Message Store)]
```

### Deep Dive: Connection Management & Message Routing
Clients connect to a **Chat Gateway** via WebSockets. The gateway holds long-lived stateful TCP connections.
Since Client A and Client B might be connected to different K8s pods, how does A's message reach B?

1. **Service Discovery / Routing Table:**
   - When a user connects, the Chat Gateway records the mapping in Redis: `User_B -> Pod_2`.
2. **Redis Pub/Sub:**
   - Pod 1 receives a message from Client A for Client B.
   - Pod 1 checks Redis, sees Client B is on Pod 2.
   - Pod 1 publishes the message to a Redis Pub/Sub channel specific to Pod 2 (e.g., `channel:pod2`).
   - Pod 2 is subscribed to its own channel. It receives the message and pushes it through the WebSocket to Client B.
   *(This avoids a massive global broadcast).*

### Data Model (Cassandra)
Chat systems are highly write-intensive. Cassandra is perfect.
**Table: messages**
- `channel_id` (Partition Key) - e.g., Hash of (UserA, UserB) or GroupID.
- `message_id` (Clustering Key) - Snowflake ID or TimeUUID for ordering.
- `sender_id`
- `content`
- `created_at`

### Deep Dive: Message Ordering and Read Receipts
- **Ordering:** Don't rely on server timestamps because clocks drift. Use Snowflake IDs generated at the gateway, or use a sequence number generator per channel.
- **Read Receipts:** When Client B reads a message, it sends a `READ_ACK` with the `message_id` to the server. Server broadcasts this to Client A. The DB updates a `last_read_message_id` for Client B in that channel.

### Presence Service (Online/Offline)
- Tracking presence for millions of users via WebSockets is heavy.
- **Heartbeats:** Clients send a ping every 5 seconds. Gateway updates a Redis key with a TTL of 10s: `SETEX presence:user_id 10 "online"`.
- **Fan-out:** When presence changes, how do we notify friends? For large friends lists, broadcasting every status change is too expensive.
- **Optimization:** Only fetch presence when opening a chat window, or batch presence updates via a separate pub/sub channel for active viewports.

### Interview Follow-ups
**Q: How do you handle offline users?**
A: If the Redis mapping shows User B is offline, Pod 1 skips Pub/Sub and directly sends the message to the Message Sync Service (or Kafka), which persists it. When User B comes online, they pull missed messages using a cursor (`last_received_message_id`).

**Q: How do you scale push notifications for offline users?**
A: The Message Sync Service detects the user is offline, pushes an event to a Notification Queue (RabbitMQ). A Notification Service consumes this and calls APNs (Apple) or FCM (Firebase) to wake up the user's phone.

---

## 5. Notification System

### Requirements
**Functional:**
- Send notifications across multiple channels: iOS Push, Android Push, SMS, Email.
- Support user preferences (opt-outs, channel preferences).
- High priority vs low priority alerts.

**Non-Functional:**
- High throughput (millions of messages per minute).
- No silent failures (retry mechanisms).
- Rate limiting (don't spam users).

### High-Level Architecture
```text
[Triggering Services] -> [Notification API]
                               |
                       [Kafka / RabbitMQ]
                         (Priority Queues)
                               |
                      [Dispatcher Service]  <-- [User Preferences DB]
                               |
          +--------------------+--------------------+
          |                    |                    |
   [Email Handler]      [SMS Handler]      [Push Handler]
          |                    |                    |
     [SendGrid]            [Twilio]         [APNs / FCM]
```

### Deep Dive: Fan-out and Queuing Strategy
- We separate the API from the dispatchers using message queues (Kafka or RabbitMQ).
- **Priority Queues:** Critical alerts (OTP, fraud alerts) go to a High-Priority Queue. Promotional messages go to a Low-Priority Queue.
- **Topic Exchange:** RabbitMQ routes messages based on routing keys (e.g., `notif.email.high`).

### Deep Dive: De-duplication and Rate Limiting
- **De-duplication:** Use Redis to store a hash of the notification payload or a unique `idempotency_key` with a TTL of 1 hour. Before sending, the handler checks if the key exists (`SETNX`).
- **Rate Limiting:** Users complain if they get 10 promos a day. Use a Redis token bucket per user per channel (e.g., max 3 promotional emails per day).

### Handling Failures
- Third-party APIs (Twilio, APNs) fail or rate-limit us.
- **Retry Queues:** If Twilio returns a 500 or 429, the SMS Handler publishes the message to a "Delayed Retry Queue" (using Dead Letter Exchanges in RabbitMQ) with exponential backoff.
- **Dead Letter Queue (DLQ):** After 5 failed retries, message goes to a DLQ for manual inspection or logging.

### Interview Follow-ups
**Q: How do you track if a notification was actually read/clicked?**
A: Append a tracking pixel to emails or wrap links in a click-tracking URL. For push notifications, the mobile app intercepts the tap event and sends an analytics event to our servers.

**Q: What if a triggering service sends 1 million notifications at once (e.g., breaking news)?**
A: This is a fan-out problem. The API service should write a single "Breaking News" event to Kafka. A dedicated Fan-out Service reads this, queries the DB for all subscribed users (paginated), and constructs individual notification messages to push onto the worker queues.

---

## 6. News Feed / Timeline

### Requirements
**Functional:**
- Generate a feed of posts from people a user follows.
- Users can publish posts (text, images).
- Chronological or ranked ordering.

**Non-Functional:**
- Real-time or near real-time feed generation.
- Feed reading is extremely fast (high read-to-write ratio).

### Deep Dive: Fan-out on Write vs Fan-out on Read

**1. Fan-out on Write (Push Model):**
- When User A posts, the system fetches all followers of User A.
- It injects the post ID into the pre-computed feed (in Redis) of every follower.
- *Pros:* Read is $O(1)$. Just fetch the list from Redis.
- *Cons:* The "Celebrity Problem". If Justin Bieber has 100M followers, one post triggers 100M cache updates. This causes immense lag (Hotkey problem).

**2. Fan-out on Read (Pull Model):**
- When User B opens their app, the system fetches all people User B follows.
- It queries the DB for the most recent posts of all those people and merges them.
- *Pros:* No celebrity problem.
- *Cons:* Heavy computation at read time. High latency for users who follow thousands of people.

**3. Hybrid Model (The Standard Solution):**
- Use Fan-out on Write for normal users.
- Use Fan-out on Read for celebrities.
- When User B loads their feed:
  1. Fetch User B's pre-computed feed (from Redis) - contains normal users' posts.
  2. Fetch recent posts from celebrities User B follows.
  3. Merge and sort the two lists in memory, then return to the user.

### High-Level Architecture
```text
[Client] --> [Feed API] ---> [Redis (Pre-computed Feeds)]
                 |
         [Fan-out Service]
                 |
[Graph DB (Followers)]  [Post DB (Cassandra)]
```

### Data Model
- **Post DB:** (Cassandra) Partition by `user_id`. Clustering key `created_at`.
- **Feed Cache:** (Redis) List or Sorted Set per user. Key: `feed:user_id`. Value: List of `post_ids`.

### Interview Follow-ups
**Q: How do you handle pagination in the feed?**
A: **Do not use OFFSET/LIMIT.** As new posts are added, offsets shift, causing duplicate posts to be shown. Use **Cursor-based Pagination**. Pass the `timestamp` or `post_id` of the last seen post as a cursor. The backend queries `WHERE created_at < cursor LIMIT 20`.

**Q: How does feed ranking work?**
A: Instead of pure chronological, you pass the candidate posts (from Redis + Celebs) through a Machine Learning ranking service. The service scores posts based on affinity (how often you interact with the author), edge weight (likes vs comments), and time decay.

---

## 7. Distributed Key-Value Store

*(Connections to Dynamo / Cassandra principles)*

### Requirements
**Functional:**
- `put(key, value)` and `get(key)` operations.
- Highly available and partition tolerant.

**Non-Functional:**
- Adjustable consistency levels (Eventual vs Strong).
- Horizontal scalability.

### High-Level Concepts

**1. Data Partitioning (Consistent Hashing):**
- We map keys and server nodes to a conceptual hash ring (0 to $2^{128}-1$).
- A key is hashed, and we walk clockwise to find the first server node on the ring to store the data.
- *Virtual Nodes:* To ensure even data distribution, each physical server represents multiple "virtual nodes" on the ring.

**2. Replication:**
- To ensure high availability, data is replicated to $N$ nodes.
- Walk clockwise on the hash ring and store the data on the first $N$ unique physical nodes.

**3. Quorum Reads & Writes:**
- $N$: Replication factor.
- $W$: Write quorum (number of nodes that must ACK a write).
- $R$: Read quorum (number of nodes that must respond to a read).
- **Strong Consistency:** Require $W + R > N$. (e.g., $N=3, W=2, R=2$).
- **Eventual Consistency:** $W=1, R=1$ (fastest, but might return stale data).

### Deep Dive: Conflict Resolution
If concurrent updates happen in a distributed system, clocks will drift.
- **Vector Clocks:** A list of `[server, version]` pairs associated with the data. E.g., `D1([S1, 1])`.
- If S1 updates it: `D2([S1, 2])`.
- If S2 updates D1 concurrently: `D3([S1, 1], [S2, 1])`.
- When a client reads, the system returns all conflicting versions. The client application must resolve the conflict (e.g., Amazon Shopping Cart merges the items).
- Alternatively, use **Last Write Wins (LWW)** using physical timestamps, though this risks data loss.

### Deep Dive: Handling Failures (Anti-Entropy)
- **Gossip Protocol:** Nodes randomly send state information to peers. They build a decentralized map of the cluster's health.
- **Hinted Handoff:** If Node A is down, Node B temporarily stores the writes intended for Node A. When A recovers, B pushes the data to A.
- **Merkle Trees:** For background synchronization, nodes compare Merkle Trees (hash trees) of their data. If root hashes differ, they traverse down to find exactly which blocks of data are out of sync and exchange only those, minimizing network IO.

### Interview Follow-ups
**Q: How does Cassandra write so fast?**
A: Log-Structured Merge (LSM) Trees. Writes are appended to an in-memory MemTable and sequentially written to a disk-based Commit Log for durability. When the MemTable is full, it flushes to disk as an immutable SSTable. Sequential IO is orders of magnitude faster than random IO (B-Trees). Reads have to merge data from MemTable and SSTables.

---

## 8. Video Streaming Platform

*(e.g., YouTube, Netflix)*

### Requirements
**Functional:**
- Users can upload videos.
- Users can stream videos smoothly.
- View count tracking.

**Non-Functional:**
- Global availability.
- No buffering (adaptive bitrate streaming).
- Handles massive file sizes (High storage).

### High-Level Architecture
```text
[Client] --> [API Gateway] ---> [Metadata DB (RDBMS)]
                 |
        [Upload Service] ---> [S3 (Original Video)]
                                     |
                       [Transcoding Task DAG / Kafka]
                                     |
                           [Transcoder Workers]
                                     |
                          [S3 (Processed Formats)]
                                     |
                                   [CDN] <--- [Client Streams Video]
```

### Deep Dive: Transcoding Pipeline (DAG)
When a raw video is uploaded, it must be converted into multiple formats (MP4, WebM) and resolutions (1080p, 720p, 480p) so clients can stream the right size based on their network.
- **Workflow / DAG Engine:** (Like Airflow or your Hermes project).
- Raw video is split into chunks (e.g., 5-second segments).
- A Message Queue (Kafka/RabbitMQ) distributes tasks.
- Workers parallelize the transcoding:
  - Worker A: Encodes chunk 1 to 1080p.
  - Worker B: Encodes chunk 1 to 480p.
- A final aggregator merges them and generates a manifest file (.m3u8 for HLS or .mpd for DASH).

### Deep Dive: Adaptive Bitrate Streaming
- The video is served in small chunks (2-10 seconds) via HTTP.
- The client player dynamically measures bandwidth. If bandwidth drops, the player requests the next chunk in a lower resolution (e.g., drops from 1080p to 480p). This prevents buffering.

### Content Delivery Network (CDN)
- Streaming directly from S3 is slow and expensive.
- CDN Edge servers cache the video chunks geographically close to users.
- 99% of reads are served from the CDN. The origin server is only hit on cache miss.

### Interview Follow-ups
**Q: How do you handle view counts for viral videos?**
A: Do not update the DB synchronously on every view. Use a high-throughput queue (Kafka) or Redis atomic counters to buffer views. A background process aggregates the counts every X seconds and flushes them to the DB.

**Q: How do you resume a failed video upload?**
A: Use multipart upload (supported by S3). The client splits the 5GB file into 10MB chunks. If chunk #45 fails, only chunk #45 is retried.

---

## 9. E-Commerce Order System

### Requirements
**Functional:**
- Users can add items to cart, checkout, and pay.
- System must reserve inventory.
- Track order status.

**Non-Functional:**
- Strict consistency (cannot oversell inventory).
- Highly available and partition tolerant.

### High-Level Architecture (Microservices)
```text
[Client] --> [API Gateway]
                 |
    +------------+------------+
    |            |            |
 [Order]    [Inventory]   [Payment]
 Service      Service      Service
    |            |            |
  (DB)         (DB)     (Stripe API)
```

### Deep Dive: Inventory Reservation (Distributed Locks)
To prevent overselling, we must reserve inventory before capturing payment.
- **SQL Transactions:** `UPDATE inventory SET stock = stock - 1 WHERE item_id = 123 AND stock > 0;`. This handles concurrency natively.
- **Distributed Locks (Redis):** If using NoSQL, use Redis Redlock around the inventory decrement operation.
- **Idempotency:** The checkout request must contain a unique `order_id`. If the client retries the request, the server detects the `order_id` and does not deduct inventory twice.

### Deep Dive: Distributed Transactions (SAGA Pattern)
Since Order, Inventory, and Payment are separate microservices with their own databases, a traditional ACID transaction doesn't work. We use the **Saga Pattern** (Orchestration or Choreography).

**Saga Orchestration Flow:**
1. **Order Service** creates Order (Status: PENDING).
2. Order Service calls **Inventory Service** to reserve items.
3. If Inventory OK, Order Service calls **Payment Service**.
4. If Payment Fails:
   - Order Service executes **Compensating Transactions**.
   - Calls Inventory Service to *release* the reservation.
   - Updates Order Status to CANCELLED.

*(Connect this to your Hermes Event-Sourced workflow engine!)*

### Interview Follow-ups
**Q: What if the Payment Service charges the user, but the Order Service crashes before updating the status to SUCCESS?**
A: **Reconciliation.** A cron job (scheduler) periodically checks PENDING orders older than 5 minutes. It queries the Payment gateway (Stripe) to check the actual status. If paid, it updates the Order DB and triggers shipping.

**Q: How do you handle massive flash sales (e.g., Black Friday)?**
A: The DB will become a bottleneck. We move inventory to Redis.
- Use a Redis Lua script to atomically check stock and decrement.
- Push the successful order event to Kafka.
- Async workers process Kafka and write to the actual SQL DB.
- Returns "Order Received" to the user immediately (Async Checkout).

---

## 10. Search Autocomplete / Typeahead

### Requirements
**Functional:**
- As the user types, suggest top $k$ (e.g., 5) search queries.
- Results should be ranked by popularity.

**Non-Functional:**
- Extremely low latency (< 50ms).
- Highly available.

### Deep Dive: Data Structure - The Trie
A Trie (prefix tree) is the optimal data structure.
- Each node represents a character.
- The path from root to node represents a prefix.
- A node stores the string and its frequency/weight.

**Optimization: Cached Top $K$ at Each Node**
If we traverse the trie to find all children and then sort them by frequency, it takes too long.
Instead, we precompute and store the top $5$ most frequent queries *inside every node*.
- E.g., The node for `a -> m -> a` stores `[amazon: 10k, amazing: 5k, amateur: 2k]`.
- Time complexity drops to $O(1)$ (plus the length of the prefix).

### High-Level Architecture
```text
[Client] --> [Load Balancer] --> [Trie Cache (Redis/In-Memory)]
                                         |
                               [Trie Build Service]
                                         |
                             [Analytics / Hadoop / Spark]
                                         |
                                 [Search Logs DB]
```

### Deep Dive: Distributed Trie (Sharding)
A massive trie won't fit in one machine's memory. We must shard it.
- **Range Based:** `a-m` on Server 1, `n-z` on Server 2. (Bad, `a` has way more traffic than `x`).
- **Consistent Hashing by Prefix:** Hash the prefix string (or just the first character if well balanced).
- **Zookeeper:** Maintains the mapping of prefixes to physical servers.

### Deep Dive: Data Gathering Service
We do not update the Trie in real-time for every search query (too slow).
- Search logs are streamed to Kafka.
- Spark Streaming or Hadoop aggregates the frequencies every 1 hour or 1 day.
- A new Trie is built offline.
- The new Trie replaces the old Trie atomically on the production servers (Blue-Green deployment).

### Interview Follow-ups
**Q: How do you filter out inappropriate or hateful words?**
A: Maintain a blocklist. During the offline Trie building phase, filter out any queries that match the blocklist before inserting them into the Trie.

**Q: How do you handle spelling mistakes?**
A: A pure Trie doesn't handle typos well. You could integrate an Edit Distance algorithm (Levenshtein), or use Elasticsearch's fuzzy matching, but for extreme scale, often the frontend caches common typos or we map typo prefixes to the correct node during the build phase.

---
*End of Document. Good luck on the interviews, Ritik!*

---

## Appendix: Code Examples (Java/Spring Boot, Node.js, SQL)

### 1. URL Shortener (Node.js & Redis)
```javascript
const express = require('express');
const redis = require('redis');
const { Snowflake } = require('snowflake-id');

const app = express();
const redisClient = redis.createClient({ url: 'redis://localhost:6379' });
const snowflake = new Snowflake({ mid: 1, offset: 2023 });

const BASE62 = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
function encodeBase62(num) {
    let encoded = '';
    while (num > 0) {
        encoded = BASE62[num % 62] + encoded;
        num = Math.floor(num / 62);
    }
    return encoded || '0';
}

app.post('/api/v1/shorten', async (req, res) => {
    const { longUrl } = req.body;
    const id = snowflake.generate();
    const shortUrl = encodeBase62(id);
    
    // Save to Redis and DB (Mocked)
    await redisClient.set(`url:${shortUrl}`, longUrl);
    res.json({ shortUrl: `http://tiny.url/${shortUrl}` });
});

app.get('/:shortUrl', async (req, res) => {
    const { shortUrl } = req.params;
    const longUrl = await redisClient.get(`url:${shortUrl}`);
    if (longUrl) {
        return res.redirect(301, longUrl);
    }
    res.status(404).send('Not Found');
});

app.listen(3000, () => console.log('URL Shortener started'));
```

### 2. Rate Limiter (Java Spring Boot + Redis Lua)
```java
@RestController
@RequestMapping("/api")
public class RateLimiterController {

    @Autowired
    private StringRedisTemplate redisTemplate;

    private static final String LUA_SCRIPT = 
        "local tokens_key = KEYS[1] " +
        "local timestamp_key = KEYS[2] " +
        "local rate = tonumber(ARGV[1]) " +
        "local capacity = tonumber(ARGV[2]) " +
        "local now = tonumber(ARGV[3]) " +
        "local last_tokens = tonumber(redis.call('get', tokens_key)) " +
        "if last_tokens == nil then last_tokens = capacity end " +
        "local last_refreshed = tonumber(redis.call('get', timestamp_key)) " +
        "if last_refreshed == nil then last_refreshed = 0 end " +
        "local delta = math.max(0, now - last_refreshed) " +
        "local filled_tokens = math.min(capacity, last_tokens + (delta * rate)) " +
        "local allowed = filled_tokens >= 1 " +
        "local new_tokens = filled_tokens " +
        "if allowed then new_tokens = filled_tokens - 1 end " +
        "redis.call('setex', tokens_key, 3600, new_tokens) " +
        "redis.call('setex', timestamp_key, 3600, now) " +
        "return allowed";

    @GetMapping("/resource")
    public ResponseEntity<String> getResource(@RequestHeader("User-Id") String userId) {
        List<String> keys = Arrays.asList("rate:tokens:" + userId, "rate:ts:" + userId);
        Long now = System.currentTimeMillis() / 1000;
        
        DefaultRedisScript<Boolean> script = new DefaultRedisScript<>(LUA_SCRIPT, Boolean.class);
        Boolean allowed = redisTemplate.execute(script, keys, "5", "10", now.toString()); // 5 req/sec, capacity 10
        
        if (allowed) {
            return ResponseEntity.ok("Success");
        } else {
            return ResponseEntity.status(HttpStatus.TOO_MANY_REQUESTS).body("Rate limit exceeded");
        }
    }
}
```

### 3. Distributed Task Scheduler (RabbitMQ DLX Configuration)
```bash
# Setting up Delayed Queue via RabbitMQ CLI / Management
# 1. Create Dead Letter Exchange
rabbitmqadmin declare exchange name=ready_exchange type=direct

# 2. Create Ready Queue
rabbitmqadmin declare queue name=ready_queue
rabbitmqadmin declare binding source=ready_exchange destination=ready_queue routing_key=ready

# 3. Create Delay Queue with TTL and DLX
rabbitmqadmin declare queue name=delay_queue arguments='{"x-message-ttl":300000, "x-dead-letter-exchange":"ready_exchange", "x-dead-letter-routing-key":"ready"}'
```
```java
// Java Spring AMQP Publisher
@Service
public class TaskPublisher {
    @Autowired
    private RabbitTemplate rabbitTemplate;

    public void scheduleTask(Task task, long delayMs) {
        rabbitTemplate.convertAndSend("delay_queue", task, message -> {
            message.getMessageProperties().setExpiration(String.valueOf(delayMs));
            return message;
        });
    }
}
```

### 4. Real-Time Chat (Node.js WebSocket + Redis Pub/Sub)
```javascript
const WebSocket = require('ws');
const redis = require('redis');

const wss = new WebSocket.Server({ port: 8080 });
const pubClient = redis.createClient();
const subClient = redis.createClient();
const POD_ID = process.env.POD_ID || 'pod_1';

subClient.subscribe(`chat:${POD_ID}`);
subClient.on('message', (channel, message) => {
    const { toUserId, content } = JSON.parse(message);
    const ws = connectedClients.get(toUserId);
    if (ws) ws.send(content);
});

const connectedClients = new Map();

wss.on('connection', (ws, req) => {
    const userId = req.url.split('?userId=')[1];
    connectedClients.set(userId, ws);
    pubClient.set(`presence:${userId}`, POD_ID, 'EX', 30); // Heartbeat

    ws.on('message', async (msg) => {
        const { toUserId, content } = JSON.parse(msg);
        const targetPod = await pubClient.get(`presence:${toUserId}`);
        
        if (targetPod) {
            pubClient.publish(`chat:${targetPod}`, JSON.stringify({ toUserId, content }));
        } else {
            // Push to offline queue (Cassandra/Kafka)
            await pubClient.lpush(`offline:${toUserId}`, content);
        }
    });
});
```

### 5. Notification System (Database Schema)
```sql
CREATE TABLE users (
    id UUID PRIMARY KEY,
    email VARCHAR(255),
    phone VARCHAR(20),
    device_token VARCHAR(255)
);

CREATE TABLE notification_preferences (
    user_id UUID REFERENCES users(id),
    channel VARCHAR(50), -- EMAIL, SMS, PUSH
    opt_in BOOLEAN DEFAULT TRUE,
    PRIMARY KEY (user_id, channel)
);

CREATE TABLE notification_logs (
    id UUID PRIMARY KEY,
    user_id UUID,
    channel VARCHAR(50),
    status VARCHAR(50), -- SENT, FAILED, DELIVERED
    sent_at TIMESTAMP,
    error_message TEXT
);
```

### 6. News Feed / Timeline (Redis Fan-out on Write)
```javascript
// Node.js Fan-out Service
async function publishPost(userId, postId) {
    // 1. Save to Cassandra
    await db.query('INSERT INTO posts (id, user_id, content) VALUES (?, ?, ?)', [postId, userId, content]);

    // 2. Fetch followers
    const followers = await getFollowers(userId); // E.g. [user2, user3, user4]
    
    // 3. Push to Redis Fan-out
    const pipeline = redisClient.pipeline();
    for (const followerId of followers) {
        pipeline.lpush(`feed:${followerId}`, postId);
        pipeline.ltrim(`feed:${followerId}`, 0, 999); // Keep only top 1000 posts
    }
    await pipeline.exec();
}
```

### 7. Distributed KV Store (Consistent Hashing Logic)
```java
import java.util.SortedMap;
import java.util.TreeMap;
import java.security.MessageDigest;

public class ConsistentHash<T> {
    private final int numberOfReplicas;
    private final SortedMap<Integer, T> circle = new TreeMap<>();

    public ConsistentHash(int numberOfReplicas) {
        this.numberOfReplicas = numberOfReplicas;
    }

    public void addNode(T node) {
        for (int i = 0; i < numberOfReplicas; i++) {
            circle.put(hash(node.toString() + i), node);
        }
    }

    public void removeNode(T node) {
        for (int i = 0; i < numberOfReplicas; i++) {
            circle.remove(hash(node.toString() + i));
        }
    }

    public T getNode(String key) {
        if (circle.isEmpty()) return null;
        int hash = hash(key);
        if (!circle.containsKey(hash)) {
            SortedMap<Integer, T> tailMap = circle.tailMap(hash);
            hash = tailMap.isEmpty() ? circle.firstKey() : tailMap.firstKey();
        }
        return circle.get(hash);
    }

    private int hash(String key) {
        // MD5 hash function implementation here
        return key.hashCode(); // Simplified for example
    }
}
```

### 8. Video Streaming (FFmpeg Transcoding Command)
```bash
# Convert source MP4 to HLS (HTTP Live Streaming) format for adaptive bitrate
ffmpeg -i input.mp4 \
  -preset slow -g 48 -sc_threshold 0 \
  -map 0:v:0 -map 0:a:0 -map 0:v:0 -map 0:a:0 -map 0:v:0 -map 0:a:0 \
  -c:v:0 libx264 -b:v:0 5000k \
  -c:v:1 libx264 -b:v:1 2500k \
  -c:v:2 libx264 -b:v:2 1000k \
  -c:a aac -b:a 128k \
  -var_stream_map "v:0,a:0 v:1,a:1 v:2,a:2" \
  -master_pl_name master.m3u8 \
  -f hls -hls_time 4 -hls_playlist_type vod \
  -hls_segment_filename "v%v/fileSequence%d.ts" \
  v%v/prog_index.m3u8
```

### 9. E-Commerce (Saga Pattern Coordinator in Node.js)
```javascript
// Saga Orchestrator
async function checkout(orderId, userId, items) {
    try {
        // Step 1: Create Order (PENDING)
        await orderService.create(orderId, userId, items);
        
        // Step 2: Reserve Inventory
        const inventoryRes = await inventoryService.reserve(orderId, items);
        if (!inventoryRes.success) throw new Error('INVENTORY_FAILED');

        // Step 3: Process Payment
        const paymentRes = await paymentService.charge(userId, amount);
        if (!paymentRes.success) throw new Error('PAYMENT_FAILED');

        // Step 4: Complete Order
        await orderService.updateStatus(orderId, 'SUCCESS');
        return { success: true };
    } catch (error) {
        // Compensating Transactions
        if (error.message === 'PAYMENT_FAILED') {
            await inventoryService.release(orderId, items); // Release Lock
        }
        await orderService.updateStatus(orderId, 'CANCELLED');
        return { success: false, reason: error.message };
    }
}
```

### 10. Search Autocomplete (Trie Data Structure Node)
```java
class TrieNode {
    Map<Character, TrieNode> children = new HashMap<>();
    boolean isEndOfWord = false;
    
    // Cached top K suggestions for O(1) retrieval
    List<String> topQueries = new ArrayList<>();
    
    public void addTopQuery(String query, int limit) {
        if (!topQueries.contains(query)) {
            topQueries.add(query);
            // In a real system, you'd sort by frequency score and trim to `limit`
            if (topQueries.size() > limit) {
                topQueries.remove(topQueries.size() - 1);
            }
        }
    }
}
```

---

## Appendix: Foundational Concepts & Comparisons

*(Integrating Ritik's notes on CAP, Raft, Kafka, etc.)*

### 1. CAP Theorem vs PACELC Theorem

**CAP Theorem:**
In the presence of a network **Partition (P)**, a distributed system must choose between:
- **Consistency (C):** Every read receives the most recent write or an error.
- **Availability (A):** Every request receives a non-error response, without the guarantee that it contains the most recent write.

**PACELC Theorem:**
An extension of CAP that addresses normal operation (when there is NO partition).
- If there is a **Partition (P)**, trade off between **Availability (A)** and **Consistency (C)**.
- **Else (E)** (normal operation), trade off between **Latency (L)** and **Consistency (C)**.
*Example:* DynamoDB is PA/EL (favors availability during partitions, favors latency during normal ops). MongoDB is PA/EC.

### 2. Databases: SQL vs NoSQL

| Feature | SQL (RDBMS) | NoSQL (Cassandra / MongoDB) |
|---------|------------|---------------------------|
| **Schema** | Rigid, predefined schema | Dynamic, schema-less |
| **Scaling** | Vertical scaling (hard to shard) | Horizontal scaling (sharding built-in) |
| **Transactions** | ACID (Atomicity, Consistency, Isolation, Durability) | BASE (Basically Available, Soft state, Eventual consistency) |
| **Joins** | Excellent, optimized for complex queries | Poor, requires denormalization |
| **Use Cases**| Financial systems, complex relationships | Big Data, real-time analytics, high write throughput |

### 3. Messaging: Kafka vs RabbitMQ

| Feature | RabbitMQ (Message Broker) | Apache Kafka (Event Streaming) |
|---------|-------------------------|------------------------------|
| **Architecture** | Smart broker / Dumb consumer | Dumb broker / Smart consumer |
| **Routing** | Direct, Topic, Fanout exchanges | Topics with Partitions |
| **Message State**| Ephemeral (deleted after ACK) | Persistent (kept for a retention period) |
| **Ordering** | FIFO within a single queue | Ordered within a specific partition |
| **Use Case** | Distributed Task Scheduler (your project) | Event Sourced Workflow Engine (your Hermes project) |

### 4. Consensus Algorithms: Raft vs Paxos

**Raft Overview:**
Raft is a consensus algorithm designed to be easy to understand. It achieves consensus via an elected leader. A node can be in 1 of 3 states: Leader, Follower, or Candidate.
- **Leader Election:** Followers expect heartbeats. If a timeout occurs, a follower becomes a Candidate and requests votes.
- **Log Replication:** The Leader receives client requests, appends to its log, and sends `AppendEntries` RPCs to followers.
- **Commit:** Once a majority of followers acknowledge, the leader commits the entry and applies it to its state machine.

*Compare to Paxos:* Paxos is mathematically proven but notoriously difficult to implement correctly in software. Raft forces a stricter structure (leader election before log replication), making implementation practical (e.g., used in etcd, Consul).

### 5. Microservices Patterns

- **API Gateway:** A single entry point for all clients. Handles routing, rate limiting, authentication, and SSL termination.
- **Circuit Breaker:** Prevents cascading failures. If Service A calls Service B and B is down, the circuit "trips" and A immediately returns a default response/error instead of waiting for a timeout.
- **Bulkhead Pattern:** Isolate elements of an application into pools so that if one fails, the others continue to function (e.g., separate thread pools for different downstream services).
- **Service Discovery:** (e.g., Eureka, Consul, K8s DNS) Allows microservices to find each other dynamically as instances spin up and down.
- **Strangler Fig:** A pattern for migrating a monolith to microservices by gradually replacing specific pieces of functionality.

### 6. Two-Phase Commit (2PC) vs Saga

**Two-Phase Commit (2PC):**
- **Phase 1 (Prepare):** The coordinator asks all participants if they can commit. Participants lock resources and reply YES/NO.
- **Phase 2 (Commit/Rollback):** If all reply YES, coordinator sends COMMIT. If any reply NO, coordinator sends ROLLBACK.
- *Problem:* Blocking and synchronous. If the coordinator crashes in Phase 2, participants are stuck with locked resources. Not suitable for microservices.

**Saga Pattern (As seen in the E-Commerce example):**
- A sequence of local transactions.
- Asynchronous and non-blocking.
- Uses Compensating Transactions to roll back.
- **Choreography:** Services publish events and other services react (decentralized).
- **Orchestration:** A central coordinator tells services what to do (centralized).

---
*Prepared specifically for Ritik Raj's System Design Interviews.*

---

## Appendix: Scaling Techniques & Infrastructure Deep Dives

### 1. Load Balancing Algorithms

Load balancers distribute incoming network traffic across a group of backend servers. Choosing the right algorithm is crucial for system performance.

- **Round Robin:** Requests are distributed sequentially. Best when all servers have equal specs and requests have similar processing times.
- **Weighted Round Robin:** Assigns weights to servers based on capacity. A server with a weight of 2 gets twice as many requests as a server with a weight of 1.
- **Least Connections:** Routes traffic to the server with the fewest active connections. Ideal for long-lived connections (e.g., WebSockets, Chat Systems).
- **Weighted Least Connections:** Combines weight and active connection counts.
- **IP Hash:** Hashes the client's IP address to determine the backend server. Ensures that a specific user consistently hits the same server (sticky sessions). Useful for in-memory session state, though stateless services are preferred.
- **Consistent Hashing:** Used in distributed caches (like Memcached/Redis) and load balancers. Minimizes the number of keys remapped when a node is added or removed.

### 2. Database Sharding Strategies

Sharding is the process of splitting a single logical database across multiple physical nodes (horizontal scaling).

- **Key-Based (Hash) Sharding:**
  - *Mechanism:* Apply a hash function to the partition key (e.g., `hash(user_id) % 4`).
  - *Pros:* Even distribution of data. Prevents hotspots.
  - *Cons:* Adding or removing a shard requires resharding and massive data migration (unless consistent hashing is used). Querying by ranges (e.g., `user_id BETWEEN 100 AND 200`) requires scattering the query to all shards.

- **Range-Based Sharding:**
  - *Mechanism:* Shard based on ranges of the partition key (e.g., User IDs 1-10000 on DB 1, 10001-20000 on DB 2).
  - *Pros:* Excellent for range queries. Easy to implement.
  - *Cons:* High risk of uneven data distribution and hotspots. For example, if partitioning by `created_at` timestamp, the shard for "today" will receive 100% of the write traffic.

- **Directory-Based Sharding:**
  - *Mechanism:* A lookup service (like Zookeeper or a routing database) maintains a mapping of exactly which shard holds which key.
  - *Pros:* Highly flexible. Keys can be moved easily without changing application logic.
  - *Cons:* The directory service becomes a single point of failure and a latency bottleneck.

### 3. Caching Strategies

Caching is the fastest way to improve read performance.

- **Cache-Aside (Lazy Loading):**
  - Application checks cache. If miss, fetches from DB, puts in cache, returns.
  - *Pros:* Cache only contains requested data. Resilient to cache failure.
  - *Cons:* Cache miss penalty (3 network hops). Stale data if DB is updated directly.

- **Write-Through:**
  - Application writes to Cache. Cache synchronously writes to DB. Returns success.
  - *Pros:* Data in cache is never stale. Read latency is extremely low.
  - *Cons:* Write latency increases (must write to both). Cache contains data that might never be read.

- **Write-Behind (Write-Back):**
  - Application writes to Cache. Cache returns success immediately. Cache asynchronously flushes to DB.
  - *Pros:* Extremely high write throughput (used in DynamoDB DAX).
  - *Cons:* Risk of data loss if the cache node crashes before flushing to the DB.

### 4. Observability and Monitoring

A distributed system is impossible to operate without observability.

- **Metrics (Prometheus + Grafana):**
  - Time-series data capturing the state of the system over time.
  - Important metrics: RED (Rate, Errors, Duration) for APIs, USE (Utilization, Saturation, Errors) for infrastructure.

- **Logging (ELK Stack - Elasticsearch, Logstash, Kibana):**
  - Structured, discrete events.
  - Every microservice should output logs in JSON format.
  - A log forwarder (Fluentd) ships logs to a central Elasticsearch cluster.

- **Distributed Tracing (Jaeger, OpenTelemetry):**
  - Solves the problem of figuring out which microservice caused an error in a 10-service chain.
  - A unique `Trace-ID` is injected at the API Gateway and passed in HTTP headers to every downstream service.
  - Each service generates a `Span` with start/end times.

### 5. Resiliency Techniques

- **Timeouts:** Never make a synchronous network call without a timeout.
- **Retries with Exponential Backoff:** When a call fails, retry. But wait progressively longer between retries (e.g., 1s, 2s, 4s, 8s) to avoid hammering a struggling service.
- **Jitter:** Add randomness to your retry intervals. If 1,000 clients fail at exactly the same time, you don't want them all retrying at exactly $T+1s$, $T+2s$, etc.
- **Dead Letter Queues (DLQ):** Messages that fail processing repeatedly should not clog up the main queue. Move them to a DLQ for manual inspection.

---
*Comprehensive Preparation Notes v1.0. Prepared for Ritik Raj.*
