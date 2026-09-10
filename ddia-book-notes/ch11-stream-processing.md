# Chapter 11 — Stream Processing

> Part of a detailed, chapter-by-chapter note series on *Designing Data-Intensive Applications* (Kleppmann). See `00-index.md` for the full series map.

---

## 11.0 Framing: From Bounded to Unbounded Data

⭐ **The single defining difference from Ch10:** a batch job's input is a known, **bounded** dataset — a stream's input is **unbounded**, arriving gradually over time, without a defined "end." This single difference has deep implications for sorting (can't sort what hasn't all arrived yet — sort-merge joins are out), and especially for **fault tolerance** (a batch task that fails can simply restart from the beginning; a stream job that's been running for *years* cannot).

- A **record**, in streaming terms, is called an **event** — a small, self-contained, immutable object recording something that happened at a point in time (a user action, a sensor reading, a log line), usually carrying a timestamp.
- Just as a batch job's file is written once and potentially read by many jobs, an event is generated once by a **producer** and potentially processed by multiple **consumers**; related events are grouped into a **topic** or **stream.**
- ⭐ **Why not just poll a database/file for new events?** Technically sufficient, but **polling becomes expensive** as you push toward low-latency continual processing — the more frequently you poll, the higher the proportion of *empty* polls, and the more wasted overhead. Traditional databases also handle change-notification poorly (relational triggers are limited and something of an afterthought) — this gap is exactly what dedicated **messaging systems** exist to fill.

---

## 11.1 Transmitting Event Streams — Messaging Systems

Two design questions differentiate messaging systems fundamentally:
1. **What happens if producers outpace consumers?** Three broad options: **drop** messages, **buffer** them (in a queue — and if so, does the queue overflow to disk, and how does that affect performance?), or apply **backpressure/flow control** (block the producer — this is what Unix pipes and TCP do, via a small fixed-size buffer).
2. **What happens on a node crash — are messages lost?** As with databases, durability requires some combination of disk writes and replication, at a real performance cost. ⭐ Whether loss is tolerable is **application-specific**: an occasional dropped sensor reading is usually fine (a fresh value arrives shortly anyway — though silent large-scale drops can go unnoticed and quietly corrupt metrics); but **event *counting*** applications need reliable delivery, since every lost message directly corrupts a counter.

### Direct Messaging from Producers to Consumers
Some systems skip an intermediary broker entirely:
- **UDP multicast** — common in finance for low-latency market data feeds; unreliable at the transport layer, but application-level protocols can recover lost packets via retransmission (the producer must remember what it sent).
- **Brokerless libraries** (ZeroMQ, nanomsg) — implement pub/sub directly over TCP/multicast.
- **StatsD/Brubeck-style metrics collection** — deliberately uses unreliable UDP; a StatsD counter is only *exactly correct* if every packet arrives, so UDP-based metrics are inherently **approximate**, not exact.
- **Webhooks** — the consumer exposes an HTTP/RPC endpoint, and the producer directly pushes to it (a registered callback URL invoked whenever an event occurs).
- ⭐ **Shared weakness of all direct approaches:** application code must itself be aware of possible message loss, and these protocols generally **assume both producer and consumer stay online** — if a consumer is offline, it simply misses whatever was sent during that window (unless the producer separately buffers retries, which breaks down if the *producer* itself crashes and loses its own retry buffer).

### Message Brokers — Centralizing the Problem
A **message broker** (message queue) is essentially "a kind of database optimized for message streams" — producers and consumers connect to it as clients, and it centralizes durability concerns instead of pushing them onto every producer/consumer pair. Consumers become effectively **asynchronous**: a producer typically only waits for the *broker* to confirm buffering, not for actual downstream processing.

**Message brokers vs. databases — key practical differences (even though some brokers can join a 2PC/XA transaction, making them database-like on paper):**
| | Databases | Message brokers |
|---|---|---|
| Retention | Data kept until explicitly deleted | Messages auto-deleted once successfully delivered |
| Assumed working set | Can be arbitrarily large | Assumed small (short queues) — buffering a lot due to slow consumers degrades throughput |
| Access pattern | Secondary indexes, arbitrary queries | Subscribe to a topic/pattern |
| Result freshness | Point-in-time snapshot; caller must re-poll to notice changes | Actively **notifies** clients as new messages arrive |

Standardized in **JMS/AMQP**; implemented by RabbitMQ, ActiveMQ, HornetQ, TIBCO EMS, IBM MQ, Azure Service Bus, Google Cloud Pub/Sub.

**Two patterns for multiple consumers of the same topic:**
- **Load balancing** — each message goes to exactly *one* of several consumers, sharing the processing work (useful when messages are expensive to process).
- **Fan-out** — each message goes to *every* consumer independently — the streaming analog of several separate batch jobs reading the same input file.
- (These can be combined: multiple separate consumer *groups*, each receiving all messages, but load-balanced *within* each group.)

**Acknowledgments and redelivery:** consumers must explicitly **ack** a message once processed; if the connection drops or times out with no ack received, the broker assumes it wasn't processed and **redelivers** it (possibly to a different consumer). ⭐ **This interacts badly with ordering under load balancing:** if consumer 2 crashes mid-processing `m3` while consumer 1 is already onto `m4`, the un-acked `m3` gets **redelivered to consumer 1**, which now processes `m4, m3, m5` — **out of the original send order**, even though both JMS and AMQP nominally try to preserve ordering. Message reordering under load-balanced redelivery is essentially **unavoidable** unless you dedicate a separate queue per consumer (giving up load balancing) — it only actually matters if your messages have **causal dependencies** on each other.

---

## 11.2 Partitioned Logs — the Kafka-style Alternative

⭐ **A deep philosophical mismatch the book highlights:** traditional AMQP/JMS-style brokers are built around a **transient messaging mindset** — once delivered and acked, a message is gone forever, just like a network packet leaves no trace once sent. **Databases and files take the opposite stance** — everything written is expected to be *permanently* retained until explicitly deleted. This mindset difference has real consequences: a batch job can be **rerun safely** against its (immutable, read-only) input, but consuming-and-acking a message from a traditional broker is **destructive** — you can't just rerun the same consumer and expect the same result, and a *newly added* consumer only sees messages sent *after* it joined; everything earlier is simply gone.

**⭐ The core idea — why not combine durable storage with low-latency notification?** This is precisely what **log-based message brokers** do.

- A **log** is simply an **append-only sequence of records on disk** (the same structural idea as WALs, Ch3, and replication logs, Ch5). A producer appends; a consumer reads sequentially, and waits for a notification (conceptually like `tail -f`) once it hits the end.
- To scale beyond one disk's throughput, the log is **partitioned** (Ch6) — each partition is an independent log, hosted on its own machine; a **topic** = a group of same-type partitions.
- **Within a partition**, the broker assigns each message a **monotonically increasing offset** — since a partition is append-only, messages within it are **totally ordered.** ⚠️ **There is no ordering guarantee across different partitions.**
- **Apache Kafka, Amazon Kinesis Streams, Twitter's DistributedLog** all work this way; despite writing everything to disk, they achieve **millions of messages/sec** via partitioning across machines and fault tolerance via replication.

### Logs Compared to Traditional Messaging
- **Fan-out is trivially cheap** — many consumers can independently read the same log; reading doesn't delete anything.
- **Load balancing works differently — at the partition level, not the message level:** the broker assigns **entire partitions** to consumer nodes (rather than individual messages), and each consumer reads its assigned partition(s) sequentially, single-threaded.
- **Downsides of this coarser-grained approach:**
  - **Max parallelism is capped by partition count** — you can't have more actively-working consumers in a group than there are partitions.
  - **Head-of-line blocking:** one slow message holds up every subsequent message in that partition.
- ⭐ **Rule of thumb the book gives:** if messages are individually expensive to process and ordering doesn't matter much, prefer **JMS/AMQP-style** brokers; if throughput is high, each message is cheap to process, and **ordering matters**, the **log-based approach works very well.**

### Consumer Offsets
Because a partition is consumed strictly sequentially, tracking progress is trivial: **everything below the consumer's current offset has been processed; everything above hasn't.** The broker doesn't need to track per-message acks — just periodically record each consumer's offset. ⭐ **This offset is functionally identical to a log-sequence-number in single-leader database replication (Ch5)** — the message broker plays the role of the leader, the consumer plays the role of a follower, and exactly the same reconnect-and-resume mechanism applies. On a consumer crash, another group member picks up its partitions from the last **recorded** offset — any messages processed but not-yet-recorded get **reprocessed** (revisited under idempotence, below).

### Disk Space Usage
Since you can't append forever, logs are split into **segments**, with old segments periodically deleted/archived — meaning a very slow consumer that falls behind far enough will simply **miss** messages once its needed offset points to an already-deleted segment. ⭐ **This is effectively a large, disk-backed circular/ring buffer.** Back-of-envelope from the book: a 6TB drive at 150MB/s sequential write takes **~11 hours to fill** — so the disk buffers ~11 hours of messages at max write rate (though in practice, deployments rarely use full write bandwidth, so buffers often actually last **days or weeks**). Notably, **log throughput stays roughly constant regardless of retention length**, since every message is written to disk anyway — unlike memory-first brokers, which are fast with short queues but slow down sharply once forced to spill to disk.

- **Falling behind is isolated and non-disruptive:** unlike a traditional broker (where an abandoned queue keeps consuming memory until manually cleaned up), a stalled/crashed log consumer just... stops advancing its offset; it doesn't disrupt other consumers, and this makes it practically safe to spin up an experimental consumer against a **production** log for debugging/testing purposes.

### Replaying Old Messages
⭐ Because consuming a log is **read-only** (doesn't destroy anything), you can **freely rewind** a consumer's offset and reprocess old messages — extremely valuable operationally, e.g. to fix a bug in a downstream consumer and then reprocess historical data correctly, an option a traditional destructive-consume broker simply doesn't offer.

---

## 11.3 Databases and Streams

⭐ **The chapter's central unifying insight:** a database's **replication log** (Ch5) is *itself* an event stream — the leader's write stream, replayed by followers, is a specific case of the same idea. Recall **state machine replication** (Ch9): if every event is a deterministic write, and every replica applies the same events in the same order, replicas converge to the same state — "*it's just another case of event streams!*"

### Keeping Systems in Sync
Real applications typically combine several data technologies — an OLTP database, a cache, a search index, a data warehouse — each with its own copy of overlapping data, optimized differently. These copies must stay **in sync.**
- ETL (Ch3) handles this for data warehouses via periodic batch dumps.
- **A tempting but dangerous alternative: dual writes** — application code explicitly writes to each system on every change (DB, then search index, then cache invalidation).

⭐ **Why dual writes are fundamentally broken — a precise race-condition example:** two clients concurrently update item X — client 1 to "A," client 2 to "B." Both write to the database *then* the search index. Due to unlucky interleaving, the **database** ends up processing client 1's write first, then client 2's (so DB's final value = B) — but the **search index** happens to receive client 2's write first, then client 1's (so index's final value = A). **The two systems are now permanently, silently inconsistent — no error occurred, and without something like version vectors, you wouldn't even notice.** There's a deeper structural reason: with two genuinely **independent leaders** (the database and the search index, each ordering writes on their own), there's no single authority determining "the" true write order — exactly the multi-leader replication conflict problem from Ch5, now hiding inside ordinary application code. Dual writes also have a **separate fault-tolerance problem**: one write can succeed while the other fails, which is literally the atomic commit problem (Ch9), expensive to solve properly.
- ⭐ **The fix, stated directly:** *"the situation would be better if there really was only one leader... and if we could make the search index a follower of the database."*

### Change Data Capture (CDC)
**CDC** = observing all writes to a database and extracting them as a stream that can be replicated to other systems, ideally **immediately** as they happen.
- ⭐ **This is precisely "making one database the leader, and the others followers."** A log-based message broker is a natural fit for transporting CDC events, since it preserves ordering (avoiding exactly the redelivery-reordering issue from earlier in the chapter).
- **Implementation approaches:** database **triggers** can implement CDC (fragile, real performance overhead), or you can **parse the actual replication log/WAL** (more robust, but has its own challenges, like handling schema changes). Real tools: **Bottled Water** (decodes PostgreSQL's WAL), **Maxwell / Debezium** (parse MySQL's binlog), **Mongoriver** (MongoDB oplog), **GoldenGate** (Oracle); at large scale: **LinkedIn's Databus, Facebook's Wormhole, Yahoo's Sherpa.**
- **CDC is normally asynchronous** — same operational tradeoff as async DB replication: adding a slow CDC consumer doesn't threaten the system of record, but all the standard **replication-lag anomalies** (Ch5) still apply.

**Initial snapshot + log compaction — the two pieces needed to *bootstrap* a new derived system:**
- **Initial snapshot:** building something like a fresh full-text index needs a **complete** copy of the database, not just recent changes — so you need a consistent snapshot **tied to a known log offset**, exactly as with setting up a new replication follower (Ch5).
- ⭐ **Log compaction — an elegant alternative to repeated snapshotting:** the same principle as LSM-tree compaction (Ch3) — periodically discard all but the **most recent** update per key, keeping the compacted log's size bounded by *current database size*, not total historical write volume. **Applied to CDC:** if every change carries a primary key, and each update fully replaces the prior value for that key, a compacted log is guaranteed to contain **at least** the latest value for every key — meaning a brand-new consumer can bootstrap a **complete copy of the database** just by scanning the compacted log from offset 0, with **no separate snapshot process needed at all.** Kafka natively supports this — letting a log-based broker double as a form of **durable storage**, not just transient messaging.
- ⭐ **First-class API support (rather than retrofitted CDC)** is a growing trend: RethinkDB (subscribe to query result changes), Firebase/CouchDB (change feeds), Meteor (built on MongoDB's oplog), VoltDB (continuous export streams).

### Event Sourcing
A related but philosophically distinct idea from application architecture (not primarily a database-replication technique like CDC):
- **CDC** captures **low-level state changes** (a row insert/update/delete) — and the writing application doesn't even need to know CDC exists.
- ⭐ **Event sourcing**, by contrast, is a deliberate **application design philosophy**: the application explicitly records **immutable, higher-level events** representing user intent (e.g., "student cancelled their course enrollment"), never mutates or deletes the event log, and derives all current state by replaying it. This is a much more meaningful, semantically rich record than the corresponding low-level side effects ("row deleted from enrollments, row added to feedback") — and critically, it makes evolving the application easier: a brand-new feature (e.g., "offer the freed spot to the next person on the waitlist") can simply **chain a new consumer off the existing event**, no schema surgery required. Structurally related to a **star schema's fact table** (Ch3) and the **chronicle data model.**
- **Deriving current state:** requires deterministic replay logic (so re-running it always yields the same result) — but ⭐ **log compaction works differently for event sourcing than for CDC**: a CDC update event fully replaces the previous value, so compaction is straightforward; an event-sourced event typically expresses *intent*, not a full state snapshot, so **later events generally don't supersede earlier ones** — you usually need the **complete history** to reconstruct final state (snapshots of derived state are used purely as a read/recovery-speed optimization, not to discard the underlying log).

**Commands vs. Events — a precise, important distinction:** a user request first arrives as a **command** — it might still fail (e.g., a username is already taken) and must be **validated before** it's accepted. Only once validated does it become an immutable **event** — a durable fact. ⭐ **Once something is an event, downstream consumers are never allowed to reject it** — by the time they see it, it's already permanent and possibly already seen by others. This means all validation must happen **synchronously, before publication** — e.g. via a serializable transaction that atomically validates-and-publishes — or, alternatively, the request can be split into a **tentative** event followed by a separate **confirmation** event once async validation completes (directly echoing the total-order-broadcast technique from Ch9).

### State, Streams, and Immutability
⭐ **The recurring thesis, stated most directly here:** any *state* — a current account balance, currently-available theater seats, a response-time percentile graph — is **the result of some sequence of events that produced it over time.** *"No matter how the state changes, there was always a sequence of events that caused those changes."* This is exactly the same immutability principle that made batch processing's input files powerful (Ch10) — you can safely rerun/re-derive computation against an immutable log, without fear of corrupting the source of truth.

**Advantages of immutable events:**
- ⭐ **Centuries-old precedent: accounting ledgers.** Accountants never erase a mistaken transaction — they add a **compensating** entry (e.g., a refund) — the original mistake stays permanently in the ledger for audit purposes, and derived figures (profit/loss statements) simply get corrected in the *next* accounting period. This is described as "entirely normal in accounting," and the same append-only-with-compensation discipline pays off broadly (not just in regulated finance): if buggy code writes bad data, recovering from a **destructive overwrite** is much harder than recovering from an immutable append-only log where you can precisely diagnose what happened.
- ⭐ **Immutable events capture strictly more information than current state alone** — e.g., a user who added, then removed, an item from their cart: order fulfillment only cares about the net current state (empty cart), but *analytics* may care a lot that the item was briefly considered — information a mutable, delete-on-remove database would simply lose forever.

**Deriving several views from the same event log (a very consequential architectural pattern — CQRS):**
- Because the log is the durable source of truth, you can build **multiple independent, differently-optimized read views** from it simultaneously (search index, analytics DB, cache...), exactly like multiple consumers of a stream. Systems like **Druid** ingest directly from Kafka this way; **Kafka Connect** exports to many databases/indexes similarly.
- ⭐ **This dissolves the classic normalization-vs-denormalization debate (Ch2):** the argument for normalization is mainly about *avoiding write-side inconsistency* — but if a read-optimized view is **mechanically, continually derived** from an immutable write-optimized event log, it's entirely reasonable for that view to be *heavily* denormalized, since the translation process itself is what guarantees consistency with the source of truth. This general pattern — separating the form data is **written** in from the form(s) it's **read** in — is called **CQRS (Command Query Responsibility Segregation).** ⭐ Twitter's home-timeline cache (Ch1) is offered as a concrete real-world CQRS example: massively denormalized (your tweet is duplicated into every follower's timeline), kept consistent purely by the fan-out service continually applying the tweet/follow event stream.
- **Evolvability bonus:** introducing a genuinely new feature can mean building a **new, separate read view** off the existing event log and running it *alongside* existing systems — often much simpler than an in-place schema migration — and once obsolete, an old read view can simply be torn down.

**Concurrency control:** the biggest downside — async log consumers mean a user could write, then immediately read a derived view, and not yet see their own write (exactly the read-your-writes problem, Ch5). ⭐ **The upside, though, is real simplification of multi-object transactions:** much of the traditional *need* for multi-object atomic transactions comes from a single user action needing to touch several places at once — event sourcing collapses that into **one single atomic write** (appending the event), because the event itself is a self-contained description of the whole user action. If the event log and derived state share the **same partitioning**, a single-threaded log consumer processing one event at a time needs **zero extra concurrency control** — the log itself has already imposed a deterministic serial order (directly echoing "actual serial execution," Ch7).

---

## 11.4 Processing Streams

Once you have an event stream, three broad things can be done with it: (1) write derived data to a database/cache/search index for others to query later; (2) push events to users directly (email alerts, real-time dashboards); or (3) **process one or more input streams to produce one or more derived output streams** — this option is the chapter's main focus, and the code doing it is called a **stream processor** or **job**.

- Structurally very similar to MapReduce/dataflow-engine jobs (Ch10) — read-only input, append-only output, similar partitioning/parallelization patterns, and ordinary map/filter transformations work the same way.
- ⭐ **The one crucial difference, restated:** a stream never ends — so **sort-merge joins are off the table** (you can't sort what you haven't fully received), and **fault tolerance can't rely on "just restart from the beginning"** the way a short-lived batch task can.

### Uses of Stream Processing
- **Complex Event Processing (CEP)** — a 1990s technique for **searching for patterns of events** in a stream (analogous to a regex, but over events instead of characters), typically expressed in a high-level declarative language, matched via an internally maintained state machine; on a match, a "complex event" is emitted. ⭐ **A striking role-reversal versus normal databases:** ordinarily, data is stored persistently and a query is transient (arrives, searches, is forgotten); in CEP, **queries are stored long-term**, and it's the *events* that continuously flow past them. Uses: fraud detection, algorithmic trading, factory-machine monitoring, military/intelligence threat detection.
- **Stream analytics** — blurrier boundary with CEP, but generally more about **aggregation/statistics over many events** than specific pattern sequences — request rates, rolling averages, trend comparisons over fixed **windows** of time (windowing explored in depth below). Often uses **probabilistic algorithms** for memory efficiency — Bloom filters (Ch3) for set membership, **HyperLogLog** for cardinality estimation, percentile-estimation algorithms (Ch1). ⭐ **Explicit clarification the book makes:** using approximate algorithms is an *optional, memory-saving optimization*, not an inherent property of stream processing in general — "there is nothing inherently approximate about stream processing."
- **Maintaining materialized views** — a specific case of CDC/event-sourcing-style derived data (above), where the goal is simply keeping a queryable, up-to-date alternative representation of some dataset. ⭐ **Key structural difference from typical stream analytics:** materialized-view maintenance conceptually needs a window stretching **all the way back to the beginning of time** — you generally can't discard old events (apart from log-compacted redundant duplicates), unlike analytics workloads that only care about a fixed recent window.
- **Search on streams** — searching individual events (not patterns of several) against complex, standing search criteria (e.g., a media-monitoring service continually matching a news feed against saved search queries) — same query/data role-reversal as CEP; can be optimized by **indexing the queries themselves**, not just the documents (Elasticsearch's "percolator" feature is one implementation).

### Reasoning About Time
⭐ **"The average over the last five minutes" sounds unambiguous — it isn't**, and this section is largely about why.

- **In batch processing**, using the system clock of the machine *running* the job would be nonsensical — a batch job might crunch a year's worth of history in minutes, and what matters is the **timestamp embedded in each event**, not when the job happened to run. Using event timestamps also keeps batch processing **deterministic** — rerunning the same job on the same input always yields the same result.
- **Many stream frameworks instead default to the local processing-machine clock ("processing time")** for windowing — simple, and reasonable *only* if processing lag is negligible.

**⭐ Why conflating event time and processing time actively produces bad data:** if a stream processor is redeployed and briefly down, then comes back up and burns through its backlog, **windowing by processing time makes it look like there was a sudden traffic spike** during backlog-processing, when the *real* underlying request rate was perfectly steady the whole time — a pure artifact of when the job happened to be running, not of anything that actually occurred.

- **Message delays also scramble arrival order relative to true occurrence order** — the book's memorable analogy: **Star Wars episode release dates.** Episode IV (1977) → V (1980) → VI (1983) → then I, II, III (1999/2002/2005) → then VII (2015). If you watched them in release order, your *viewing* order ("processing time") is wildly inconsistent with the *narrative* order ("event time"/episode number) — humans cope with this instinctively; stream-processing algorithms have to be deliberately engineered to.

**Knowing when you're "ready" — the straggler problem:** you can never be fully certain you've received *every* event for a given time window — some might still be in flight, delayed by a network hiccup elsewhere. Two broad strategies:
1. **Time out and ignore stragglers**, tracking the drop rate as a monitored metric and alerting if it grows large.
2. **Publish a correction** once stragglers do arrive — potentially needing to **retract** a previously-emitted (now-stale) window result.
- A special "no more messages before timestamp t" marker can help trigger window completion — but this gets genuinely tricky with **multiple independent producers**, each needing to be tracked separately, and dynamic producer membership adds further complexity.

**⭐ "Whose clock are you using, anyway?" — timestamps under multi-hop buffering.** A mobile app can buffer events locally while offline, syncing (possibly *days* later) once reconnected — to any downstream consumer, these will look like extreme stragglers. The "true" event time should ideally be the device's **local clock at the moment of the interaction** — but a user-controlled device's clock **cannot be trusted** (may be innocently or deliberately wrong, Ch8). The **server's receipt clock** is more trustworthy (it's under your control) but describes when the event was *received*, not when the user actually did the thing.
- **A practical mitigation — log three timestamps:** (a) event time per the *device* clock, (b) send time per the *device* clock, (c) receive time per the *server* clock. Subtracting (b) from (c) estimates the **clock offset** between device and server (assuming negligible network delay relative to the needed accuracy), which can then be applied to correct (a) into a more trustworthy estimate of true event time — assuming the offset didn't itself drift between occurrence and send.
- ⭐ The book notes this is **not actually a stream-processing-specific problem** — batch processing has the exact same underlying issue — it's just far **more noticeable** in a streaming context, where you're continuously, actively reasoning about the passage of time.

### Types of Windows
| Window type | Definition | Key property |
|---|---|---|
| **Tumbling** | Fixed length, non-overlapping; every event belongs to exactly one window | E.g., 1-minute buckets: `[10:03:00–10:03:59]`, `[10:04:00–10:04:59]`, ... Implementable by rounding each timestamp down to the nearest boundary. |
| **Hopping** | Fixed length, but windows **overlap**, providing smoothing | E.g., a 5-minute window with a 1-minute hop: `[10:03–10:08)`, then `[10:04–10:09)`, ... Implementable as an aggregation over several adjacent tumbling windows. |
| **Sliding** | Contains all events within some interval of *each other* (not fixed calendar boundaries) | E.g., a 5-min sliding window groups events at 10:03:39 and 10:08:12 together (they're <5 min apart) — something a tumbling/hopping window with fixed boundaries would **not** do. Implementable via a time-sorted buffer, evicting events as they age out. |
| **Session** | **No fixed duration** — groups events for the *same user* that occur close together in time; ends after a period of inactivity (e.g., 30 min with no events) | Standard for website **sessionization** (revisiting Ch10's `GROUP BY` example, now continuous rather than batch). |

---

## 11.5 Stream Joins

⭐ Because new events on a stream can appear at any time, stream joins are structurally harder than batch joins — you can never be *sure* you've seen everything relevant yet. Three distinct types:

### 1. Stream-Stream Join (Window Join)
**Worked example — search click-through rate:** log a "search performed" event and, separately, a "result clicked" event (linked by session ID). A click might never come at all (user abandons the search), or might come anywhere from seconds to *weeks* later — and due to variable network delay, could theoretically even **arrive before** the corresponding search event. ⭐ **Why this genuinely requires a join, not just embedding the search context inside the click event:** doing that would only ever tell you about clicked results, entirely missing the (analytically crucial) case of searches that got **no** click at all.
- **Implementation:** the processor maintains state — e.g., an index of recent events by session ID, covering some chosen join window (e.g., "join a click with a search if they're within 1 hour of each other"). Every incoming event checks the *other* side's index for a match; a match emits a "clicked" event; a search that expires unmatched emits a "not clicked" event.

### 2. Stream-Table Join (Stream Enrichment)
Directly analogous to Ch10's batch activity-log/user-database join, but now **continuous.** Each incoming activity event is looked up against a (locally cached, hash-table- or on-disk-index–based) copy of the user database — the same technique as a **map-side hash join.**
- ⭐ **The key new wrinkle versus the batch case:** a batch job's "database" input is a fixed point-in-time snapshot; a **long-running stream processor's local copy must be kept continuously fresh**, since the underlying database keeps changing over the processor's (potentially very long) lifetime. Solution: the processor subscribes to **CDC** on the user database *as a second stream*, updating its local copy whenever a profile changes. The result is really a join between **two streams** — activity events and profile-update events — where the profile-update side effectively uses a window stretching back to "the beginning of time" (newer profile versions simply overwrite older ones in local state), while the activity-event side typically maintains no window at all.

### 3. Table-Table Join (Materialized View Maintenance)
**Worked example — maintaining Twitter's home-timeline cache (revisiting Ch1's load-description example)**, expressed as the continuous maintenance of this conceptual query's result:
```sql
SELECT follows.follower_id AS timeline_id,
  array_agg(tweets.* ORDER BY tweets.timestamp DESC)
FROM tweets
JOIN follows ON follows.followee_id = tweets.sender_id
GROUP BY follows.follower_id
```
To keep this "materialized" continuously, the stream processor needs event streams for **both** tweet actions (send/delete) **and** follow-relationship actions (follow/unfollow), and must maintain its own database of current followers per user, in order to know which timelines to touch on each new tweet. ⭐ **The direct correspondence:** the join of the two event streams **is** the join of the two underlying tables in the query above — the timeline cache is simply a continually-updated cached result of that join.

### Time-Dependence of Joins
⭐ **What all three join types share:** each requires maintaining some state derived from one input, queried by events from the other input — and **the order in which that state-building side is updated genuinely matters** (does a user follow-then-unfollow, or unfollow-then-follow?). Within a single log partition, order is preserved; **across different streams or partitions, there's generally no ordering guarantee at all.**
- **A related, subtler problem — nondeterministic joins from slowly-changing reference data:** if a batch job re-joins against a "current" table (e.g., tax rates) that has since changed, rerunning the exact same job later can silently produce a **different** join result than it did the first time — breaking the determinism batch processing normally guarantees. Data warehouses call this a **slowly changing dimension (SCD)**, commonly solved by giving every distinct *version* of a joined record its own unique identifier (e.g., a specific historical tax-rate version ID gets embedded directly in the invoice at time of sale) — making the join outcome permanently, deterministically pinned, at the cost of no longer being able to log-compact away old versions (all historical versions must be retained).

---

## 11.6 Fault Tolerance

⭐ **Recap of why batch fault tolerance is comparatively easy:** MapReduce's transparent task retry works because inputs are immutable, each task's output goes to a separate file, and output only becomes visible on **successful completion** — the net *visible* effect is as if every record were processed exactly once, even though internally a retried task might process some records more than once. This is called **exactly-once semantics** — the book prefers the more honest term "**effectively-once**," since records genuinely *can* be processed multiple times internally; what's actually guaranteed is that the **externally visible result** looks as if they weren't.

⭐ **Why this doesn't translate directly to streaming:** *"waiting until a task is finished before making its output visible is not an option, because a stream is infinite."*

### Microbatching and Checkpointing
- **Microbatching (Spark Streaming)** — break the unbounded stream into small chunks (typically ~1 second) and treat each like a tiny batch job, inheriting batch-style exactly-once semantics *for that chunk*. Batch size is a real tradeoff: smaller = more scheduling/coordination overhead; larger = longer delay before results become visible. This implicitly creates a **tumbling window** sized to the microbatch (by *processing* time, not event time) — larger logical windows need explicit state carried across microbatches.
- **Checkpointing (Apache Flink)** — periodically snapshot full operator state to durable storage; on crash, resume from the last checkpoint, discarding any output produced since. Checkpoints are triggered by **barriers** inserted into the message stream — conceptually similar to microbatch boundaries, but without forcing a fixed window granularity.
- ⭐ **A crucial limitation shared by both approaches:** they provide exactly-once semantics **only within the confines of the stream-processing framework itself.** The instant output leaves the framework — a database write, a message sent to an external broker, an email/push notification — the framework **can no longer discard** the effects of a failed, retried task. A restarted task causes that **external side effect to happen a second time**, and neither microbatching nor checkpointing alone prevents it.

### Atomic Commit Revisited
⭐ **The precise requirement for a true "exactly-once appearance" including external effects:** every effect of processing an event — downstream messages, database writes, operator state changes, **and** the consumer-offset advancement itself — must **all happen atomically together, or not at all.** This is directly the **atomic-commit problem** from Ch9. Traditional heterogeneous distributed transactions (XA) have the well-known operational problems covered there — but in a **more restricted environment**, an efficient atomic-commit mechanism *is* achievable, by keeping everything **internal to the stream-processing framework** (state changes and outgoing messages both managed by the same system) rather than trying to span arbitrary heterogeneous external technologies. Used by **Google Cloud Dataflow** and **VoltDB**; the book notes plans (at time of writing) to bring similar capability to **Apache Kafka.**

### Idempotence
⭐ **A frequently cheaper alternative to full atomic commit:** if an operation is naturally (or can be made) **idempotent** — repeating it has the same effect as doing it once — then safely retrying a failed/duplicated task doesn't require distributed transactions at all.
- Setting a key to a fixed value is naturally idempotent; **incrementing a counter is not** (repeating it doubles the effect).
- **Even a non-idempotent operation can often be *made* idempotent with a bit of extra metadata** — the book's example: since every Kafka message carries a persistent, monotonically increasing **offset**, you can attach that offset to a downstream database write, and on retry, check whether a write with that same offset was **already applied** — turning an otherwise non-idempotent update into an effectively idempotent one, without needing a full cross-system distributed transaction.
- ⭐ Two operational assumptions this relies on, both worth keeping in mind: (1) on task retry, the operation must be reapplied **in exactly the same way** (so the idempotence key is consistent), and (2) if a stream processor fails over to a different node, **the new node must pick up exactly the same idempotence bookkeeping** the old one had — not always trivial in practice.

---

## Chapter 11 Summary Table

| Concept | Core idea |
|---|---|
| **JMS/AMQP-style brokers** | Message-level load balancing, destructive consumption, small assumed working set |
| **Log-based brokers (Kafka-style)** | Partition-level load balancing, non-destructive/replayable consumption, offsets like a leader-replication log |
| **Change Data Capture (CDC)** | Turns a database into "the leader," other systems into "followers," fixing the dual-writes race condition |
| **Event sourcing** | Application-level immutable event log of *intent*, distinct from CDC's low-level state-change capture |
| **CQRS** | Separating write-optimized event logs from multiple derived, denormalized, read-optimized views |
| **Windowing** | Tumbling, hopping, sliding, session — each trading off fixed boundaries vs. adaptivity to actual event timing |
| **Event time vs. processing time** | Confusing the two produces spurious spikes/artifacts; true event time itself may need multi-clock correction |
| **Stream-stream / stream-table / table-table joins** | All require maintaining state from one side, queried by the other; ordering across streams isn't guaranteed |
| **Exactly-once ("effectively-once") semantics** | Achievable via microbatching/checkpointing *inside* the framework, but needs atomic commit or idempotence for effects that cross the framework boundary |

## Interview-Ready Talking Points

- Precisely explain **why dual writes are broken**, walking through the concrete two-client race-condition example — and state the fix ("make the search index a follower of the database" via CDC).
- Distinguish **CDC from event sourcing** crisply: low-level state-change capture (app doesn't need to know) vs. a deliberate, intent-centric application architecture — and explain why **log compaction works for CDC but not (in the same simple way) for event sourcing.**
- Explain **CQRS** and connect it directly back to Ch2's normalization/denormalization debate — denormalized read views become "safe" once they're mechanically derived from an immutable log.
- Walk through the **Star Wars analogy** for event time vs. processing time — it's memorable and precisely captures the concept.
- Explain the **three window types (tumbling/hopping/sliding/session)** with concrete boundary examples.
- Distinguish **stream-stream, stream-table, and table-table joins**, and be ready to explain why **all three ultimately reduce to "maintain state from one side, query it from the other."**
- Explain why **batch exactly-once ("effectively-once") is comparatively easy**, and precisely why streaming can't just borrow that trick wholesale — then explain **microbatching/checkpointing plus idempotence** as the two practical streaming answers.