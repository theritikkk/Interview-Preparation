# Kafka, Event-Driven Architecture & Thread Handling — Deep Dive Notes

> **Purpose:** End-to-end study material for a Distributed Systems Software Engineer.  
> **Prerequisites:** You've completed `distributed-systems-notes.md` (CAP, consensus, replication, partitioning) and are finishing `microservices-backend-interview-notes.md`.  
> **Scope:** Kafka internals → Spring Boot integration → threading & concurrency → production patterns → observability → advanced distributed topics.

---

## Table of Contents

1. [Apache Kafka — Core Internals](#1-apache-kafka--core-internals)
2. [Kafka Cluster Architecture](#2-kafka-cluster-architecture)
3. [Kafka Storage Engine — The Commit Log](#3-kafka-storage-engine--the-commit-log)
4. [Replication, ISR & Leader Election](#4-replication-isr--leader-election)
5. [Producer Deep Dive](#5-producer-deep-dive)
6. [Consumer Deep Dive](#6-consumer-deep-dive)
7. [Consumer Groups & Rebalancing](#7-consumer-groups--rebalancing)
8. [Kafka Streams & KTables (Quick Reference)](#8-kafka-streams--ktables-quick-reference)
9. [Spring Boot + Kafka Integration](#9-spring-boot--kafka-integration)
10. [Threading Model — Kafka + Spring Boot](#10-threading-model--kafka--spring-boot)
11. [Exactly-Once Semantics (EOS)](#11-exactly-once-semantics-eos)
12. [Transactional Outbox Pattern](#12-transactional-outbox-pattern)
13. [Dead Letter Queue (DLT)](#13-dead-letter-queue-dlt)
14. [Idempotent Consumer Design](#14-idempotent-consumer-design)
15. [Kafka Performance Tuning](#15-kafka-performance-tuning)
16. [Event-Driven Architecture Patterns](#16-event-driven-architecture-patterns)
17. [Distributed Concurrency & Locking](#17-distributed-concurrency--locking)
18. [Observability in Distributed Systems](#18-observability-in-distributed-systems)
19. [Resilience Patterns for Production](#19-resilience-patterns-for-production)
20. [Kafka vs Other Message Brokers](#20-kafka-vs-other-message-brokers)
21. [Interview Questions — Scenario-Based](#21-interview-questions--scenario-based)
22. [What To Study Next — Roadmap](#22-what-to-study-next--roadmap)

---

## 1. Apache Kafka — Core Internals

### What Is Kafka?
- A **distributed, append-only commit log** designed for high-throughput, fault-tolerant, real-time event streaming.
- NOT a traditional message queue — messages are **not deleted** after consumption. They are retained based on retention policies.
- Think of it as a **durable, distributed, replayable log** — consumers track their own position (offset).

### Key Abstractions

| Concept     | Description |
|:------------|:------------|
| **Topic**   | A named channel/category to which records are published. Logical grouping only. |
| **Partition** | A topic is split into N partitions. Each partition is an **ordered, immutable** sequence of records. **The unit of parallelism.** |
| **Offset**  | A unique, monotonically increasing ID assigned to each record within a partition. Think of it as a position pointer. |
| **Record**  | Key + Value + Timestamp + Headers. The fundamental unit of data. |
| **Broker**  | A Kafka server that stores partitions and serves client requests. |
| **Cluster** | A group of brokers working together. |

### Why Kafka Is Fast
1. **Sequential Disk I/O** — Append-only writes avoid random disk seeks (sequential writes on modern disks are faster than random memory access!).
2. **Zero-Copy Transfer** — Uses OS-level `sendfile()` syscall to transfer data directly from disk page cache to network socket, bypassing user-space entirely.
3. **Batching** — Producers batch records; brokers write batches; consumers fetch batches. Amortizes network overhead.
4. **Page Cache** — Kafka relies on the OS page cache instead of in-process JVM heap cache, avoiding GC overhead.
5. **Compression** — Supports gzip, snappy, lz4, zstd compression at the batch level.

---

## 2. Kafka Cluster Architecture

```
┌──────────────────────────────────────────────────────┐
│                   Kafka Cluster                      │
│                                                      │
│    ┌──────────┐     ┌──────────┐     ┌──────────┐    │
│    │ Broker 1 │     │ Broker 2 │     │ Broker 3 │    │
│    │          │     │          │     │          │    │
│    │ P0(L)    │     │ P0(F)    │     │ P0(F)    │    │
│    │ P1(F)    │     │ P1(L)    │     │ P1(F)    │    │
│    │ P2(F)    │     │ P2(F)    │     │ P2(L)    │    │
│    └──────────┘     └──────────┘     └──────────┘    │
│                                                      │
│   L = Leader Replica      F = Follower Replica       │
│                                                      │
│    ┌──────────────────────────────────────────┐      │
│    │ KRaft Controller Quorum (replaces ZK)    │      │
│    │ Manages metadata, leader election, etc.  │      │
│    └──────────────────────────────────────────┘      │
└──────────────────────────────────────────────────────┘

Producers ───► Leader Replicas ───► Follower Replicas
Consumers ◄─── Leader Replicas (by default)
```

### KRaft Mode (Kafka Raft)
- **Kafka 3.3+**: ZooKeeper dependency eliminated.
- Internal Raft-based quorum controller manages cluster metadata: topic configs, partition assignments, leader elections.
- Simplifies operations — no separate ZK ensemble to manage.

---

## 3. Kafka Storage Engine — The Commit Log

### Partition Directory Structure
```
/kafka-data/topic-orders-0/
├── 00000000000000000000.log        # Segment file (message data)
├── 00000000000000000000.index      # Offset → byte position mapping
├── 00000000000000000000.timeindex  # Timestamp → offset mapping
├── 00000000000000367823.log        # Next segment (starts at offset 367823)
├── 00000000000000367823.index
├── 00000000000000367823.timeindex
└── leader-epoch-checkpoint
```

### Segment Mechanics
- Each partition is divided into **segments** (default 1 GB or 7 days, whichever comes first).
- Only the **active segment** (latest) accepts writes. Older segments are immutable.
- Segment file naming = base offset of the first message in that segment.

### Index Files
- **`.index`** — Sparse mapping from logical offset → physical byte position in `.log` file. Enables O(1) lookup.
- **`.timeindex`** — Maps timestamp → offset. Enables time-based seeking (e.g., "replay from 2 hours ago").

### Retention Policies

| Policy | Config | Behavior |
|:-------|:-------|:---------|
| **Time-based** | `log.retention.hours=168` (7 days) | Delete segments older than configured time |
| **Size-based** | `log.retention.bytes=1073741824` (1 GB) | Delete oldest segments when total exceeds limit |
| **Compaction** | `log.cleanup.policy=compact` | Keep only the **latest value for each key**. Used for changelogs, KTables. |

### Log Compaction — Deep Dive
- Kafka keeps a background thread ("log cleaner") that reads old segments, deduplicates by key, and writes compacted segments.
- A record with `value = null` (tombstone) signals deletion of that key.
- **Use case:** KTable changelogs, CDC (Change Data Capture), config topics.

---

## 4. Replication, ISR & Leader Election

### Replication Model
- Each partition has a configurable **replication factor** (e.g., `replication.factor=3`).
- **One leader**, rest are **followers**.
- **All writes** go to the leader. Followers continuously **pull** data from the leader (NOT push).
- By default, **all reads** also go to the leader (followers can serve reads since Kafka 2.4 with `replica.selector.class`).

### In-Sync Replicas (ISR)

```
ISR = { Leader, Follower_A, Follower_B }  ← All caught up

If Follower_B falls behind beyond replica.lag.time.max.ms (default 30s):
ISR = { Leader, Follower_A }  ← Follower_B removed

When Follower_B catches up again:
ISR = { Leader, Follower_A, Follower_B }  ← Re-added
```

### When Is a Message "Committed"?
- A message is **committed** = written to ALL replicas in the current ISR.
- Only committed messages are visible to consumers.
- Controlled by producer `acks` setting (see Producer section).

### Leader Election
- If the leader dies, a new leader is elected **only from the ISR** (by default).
- `unclean.leader.election.enable=false` (default) — Prevents data loss by refusing to elect an out-of-sync replica as leader. The partition goes **offline** instead.
- `unclean.leader.election.enable=true` — Allows an out-of-sync replica to become leader. **Enables availability at the cost of potential data loss.**

### The `min.insync.replicas` Setting
- `min.insync.replicas=2` with `acks=all` means: the producer write will **fail** if fewer than 2 replicas are in the ISR.
- This prevents writes to a cluster where only 1 replica survives (and could lose data if that one dies too).

**Production setup:** `replication.factor=3`, `min.insync.replicas=2`, producer `acks=all` → Tolerates 1 broker failure with zero data loss.

---

## 5. Producer Deep Dive

### Producer Architecture

```
Application Thread
    │
    ▼
┌─────────────────────────────────────────────────────────┐
│              KafkaProducer                              │
│                                                         │
│  ┌───────────┐   ┌───────────┐   ┌────────────┐         │ 
│  │ Serializer│──►│Partitioner│──►│ Record     │         │
│  │ (Key+Val) │   │           │   │ Accumltor  │         │
│  └───────────┘   └───────────┘   └────────────┘         │
│                                      │                  │
│                              ┌───────▼────────────┐     │
│                              │  Sender Thread     │     │
│                              │  (I/O Thread)      │     │
│                              │  Batches → Broker  │     │
│                              └────────────────────┘     │
└─────────────────────────────────────────────────────────┘
```

### Key Producer Configs

| Config | Default | What It Does |
|:-------|:--------|:-------------|
| `acks` | `all` (Kafka 3.0+) | `0` = fire-and-forget, `1` = leader ack, `all` = all ISR ack |
| `batch.size` | 16384 (16 KB) | Max bytes per batch per partition |
| `linger.ms` | 0 | How long to wait for more records before sending a batch |
| `buffer.memory` | 33554432 (32 MB) | Total memory for unsent records. If full, `send()` blocks |
| `max.in.flight.requests.per.connection` | 5 | Max unacknowledged requests per connection |
| `enable.idempotence` | `true` (Kafka 3.0+) | Deduplicates retried messages using Producer ID + Sequence Number |
| `compression.type` | `none` | `gzip`, `snappy`, `lz4`, `zstd` |
| `retries` | `Integer.MAX_VALUE` (with idempotence) | Number of retries on transient failure |
| `delivery.timeout.ms` | 120000 (2 min) | Total time allowed for a record to be sent (includes retries) |

### Partitioning Strategy
1. **Key is specified** → `hash(key) % numPartitions` — Guarantees all records with the same key go to the same partition (message ordering per key).
2. **Key is null** → Sticky partitioner (Kafka 2.4+). Batches fill up a single partition, then "sticks" to another. Better than round-robin for batching efficiency.
3. **Custom partitioner** → Implement `Partitioner` interface.

### Idempotent Producer (How It Works)
```
Producer sends record with:
  - Producer ID (PID): assigned by broker on init
  - Sequence Number: per-partition monotonic counter

Broker checks:
  - If seq == expected → Accept
  - If seq < expected  → Duplicate! Silently ignore, return success
  - If seq > expected  → Out of order! Return error

Result: Even if the producer retries, the broker deduplicates.
Constraint: max.in.flight.requests.per.connection ≤ 5 (with idempotence)
```

---

## 6. Consumer Deep Dive

### The `poll()` Loop — Heart of the Consumer

```java
while (true) {
    ConsumerRecords<String, String> records = consumer.poll(Duration.ofMillis(100));
    for (ConsumerRecord<String, String> record : records) {
        process(record);  // Your business logic
    }
    consumer.commitSync();  // or commitAsync()
}
```

### Key Consumer Configs

| Config | Default | What It Does |
|:-------|:--------|:-------------|
| `group.id` | — | Consumer group identifier |
| `auto.offset.reset` | `latest` | `earliest` (read from beginning) or `latest` (read only new) |
| `enable.auto.commit` | `true` | Auto-commit offsets every `auto.commit.interval.ms` |
| `max.poll.records` | 500 | Max records returned per `poll()` call |
| `max.poll.interval.ms` | 300000 (5 min) | Max time between `poll()` calls before consumer is considered dead |
| `session.timeout.ms` | 45000 (45s) | Heartbeat timeout. If no heartbeat for this long, consumer is removed from group |
| `heartbeat.interval.ms` | 3000 (3s) | Frequency of heartbeats to the group coordinator |
| `fetch.min.bytes` | 1 | Min data broker should accumulate before responding to fetch |
| `fetch.max.wait.ms` | 500 | Max time broker waits to accumulate `fetch.min.bytes` |
| `isolation.level` | `read_uncommitted` | `read_committed` → only read transactionally committed messages |

### Offset Management

```
                  ┌───────────────────────────────────────┐
                  │        Partition (ordered log)        │
                  │                                       │
  Committed ──►   │ [0][1][2][3][4][5][6][7][8][9]        │
  Offset = 5      │              ▲           ▲            │
                  │              │           │            │
                  │     Last Committed   Current          │
                  │        Offset       Position          │
                  └───────────────────────────────────────┘
                  
  If consumer crashes and restarts:
    - Resumes from "Last Committed Offset"
    - Records 5-7 will be reprocessed (at-least-once)
```

**Three delivery guarantees:**

| Guarantee | How | Trade-off |
|:----------|:----|:----------|
| **At-most-once** | Commit offset BEFORE processing | May lose messages |
| **At-least-once** | Commit offset AFTER processing (default) | May process duplicates |
| **Exactly-once** | Use Kafka transactions (EOS) | Higher latency, more complexity |

---

## 7. Consumer Groups & Rebalancing

### How Partitions Are Assigned

```
Topic "orders" has 6 partitions: P0, P1, P2, P3, P4, P5

Consumer Group "order-service" with 3 consumers:
  Consumer-A ── P0, P1
  Consumer-B ── P2, P3
  Consumer-C ── P4, P5

If Consumer-C dies:
  Consumer-A ── P0, P1, P4    ← Rebalance!
  Consumer-B ── P2, P3, P5

If a 7th consumer joins → It stays idle (more consumers than partitions)
```

**Key rules:**
- Each partition is consumed by **exactly one** consumer within a group.
- If `consumers > partitions`, some consumers are idle.
- If `consumers < partitions`, some consumers handle multiple partitions.
- Different consumer groups consume **independently** (each gets all messages).

### Rebalancing — The Expensive Operation

**Triggers:**
1. Consumer joins the group
2. Consumer leaves (or is kicked due to heartbeat/poll timeout)
3. Topic metadata changes (new partitions added)
4. Consumer subscribes/unsubscribes

**Problem:** During a rebalance, ALL consumers in the group STOP consuming. This causes a processing pause.

**Mitigation strategies:**
1. **Static Group Membership** — Set `group.instance.id` on each consumer. The broker won't immediately remove the consumer on heartbeat miss; it waits for `session.timeout.ms` before rebalancing.
2. **CooperativeStickyAssignor** — Instead of revoking ALL partitions and reassigning, only move the minimal set. Reduces stop-the-world pauses.
3. **Tune timeouts** — Set `session.timeout.ms` high enough to survive GC pauses but low enough to detect real failures.

### Partition Assignment Strategies

| Strategy | Behavior |
|:---------|:---------|
| `RangeAssignor` (default) | Assigns contiguous partition ranges per topic. Can be uneven across consumers. |
| `RoundRobinAssignor` | Round-robin across all partitions of all topics. More even. |
| `StickyAssignor` | Like RoundRobin but tries to keep existing assignments during rebalance. |
| `CooperativeStickyAssignor` | Incremental cooperative rebalancing. **Recommended for production.** |

---

## 8. Kafka Streams & KTables (Quick Reference)

### Kafka Streams
- A **library** (not a separate cluster) for building stream processing applications.
- Runs inside your Java/Spring Boot application.
- Provides exactly-once processing out of the box.

### Key Concepts

| Concept | Description |
|:--------|:------------|
| **KStream** | An unbounded, continuously updating stream of records. Each record is an independent event. |
| **KTable** | A changelog stream where each record is an **upsert** (update for the given key). Represents a "table" view of the stream. |
| **GlobalKTable** | Like KTable but replicated to ALL instances (for small lookup tables like "country codes"). |
| **State Store** | Local, embedded RocksDB instance for stateful operations (aggregations, joins). Backed by a changelog topic for fault tolerance. |
| **Windowing** | Tumbling, Hopping, Sliding, Session windows for time-based aggregations. |

### When to Use What
- **Kafka Streams** → Processing within the same Kafka ecosystem, low-latency, embedded in microservice.
- **Apache Flink / Spark Streaming** → Cross-system processing, complex event processing, large-scale batch + stream.

---

## 9. Spring Boot + Kafka Integration

### Dependencies (Maven)

```xml
<dependency>
    <groupId>org.springframework.kafka</groupId>
    <artifactId>spring-kafka</artifactId>
</dependency>
```

### Application Properties — Comprehensive

```yaml
spring:
  kafka:
    bootstrap-servers: localhost:9092
    
    producer:
      key-serializer: org.apache.kafka.common.serialization.StringSerializer
      value-serializer: org.springframework.kafka.support.serializer.JsonSerializer
      acks: all
      retries: 3
      properties:
        enable.idempotence: true
        max.in.flight.requests.per.connection: 5
        linger.ms: 10
        batch.size: 32768
        compression.type: snappy
      # For transactional producers (EOS):
      # transaction-id-prefix: tx-order-service-
    
    consumer:
      group-id: order-service-group
      key-deserializer: org.apache.kafka.common.serialization.StringDeserializer
      value-deserializer: org.springframework.kafka.support.serializer.JsonDeserializer
      auto-offset-reset: earliest
      enable-auto-commit: false   # Always manual commit in production
      max-poll-records: 100
      properties:
        spring.json.trusted.packages: "com.example.dto"
        # For EOS:
        # isolation.level: read_committed
    
    listener:
      ack-mode: MANUAL_IMMEDIATE  # Fine-grained commit control
      concurrency: 3              # Number of consumer threads
      type: SINGLE                # or BATCH for batch processing
```

### Producer — Sending Messages

```java
@Service
@RequiredArgsConstructor
public class OrderEventProducer {

    private final KafkaTemplate<String, OrderEvent> kafkaTemplate;

    // --- Fire and forget ---
    public void sendFireAndForget(OrderEvent event) {
        kafkaTemplate.send("orders", event.getOrderId(), event);
    }

    // --- Synchronous (blocks until broker ack) ---
    public void sendSync(OrderEvent event) throws Exception {
        SendResult<String, OrderEvent> result = kafkaTemplate
            .send("orders", event.getOrderId(), event)
            .get(10, TimeUnit.SECONDS);  // blocks
        
        RecordMetadata metadata = result.getRecordMetadata();
        log.info("Sent to partition={}, offset={}", 
                 metadata.partition(), metadata.offset());
    }

    // --- Async with callback (recommended) ---
    public void sendAsync(OrderEvent event) {
        CompletableFuture<SendResult<String, OrderEvent>> future = 
            kafkaTemplate.send("orders", event.getOrderId(), event);
        
        future.whenComplete((result, ex) -> {
            if (ex != null) {
                log.error("Failed to send: {}", ex.getMessage());
                // Handle failure: retry, send to fallback, alert
            } else {
                log.info("Sent to partition={}, offset={}", 
                    result.getRecordMetadata().partition(),
                    result.getRecordMetadata().offset());
            }
        });
    }
}
```

### Consumer — Receiving Messages

```java
@Service
@Slf4j
public class OrderEventConsumer {

    // --- Basic consumer ---
    @KafkaListener(topics = "orders", groupId = "order-service-group")
    public void consume(OrderEvent event) {
        log.info("Received order: {}", event.getOrderId());
        processOrder(event);
    }

    // --- With acknowledgment (manual commit) ---
    @KafkaListener(topics = "orders", groupId = "order-service-group")
    public void consumeWithAck(OrderEvent event, Acknowledgment ack) {
        try {
            processOrder(event);
            ack.acknowledge();  // Commit offset only after successful processing
        } catch (Exception e) {
            log.error("Processing failed, NOT committing offset", e);
            // Message will be redelivered on next poll
        }
    }

    // --- Batch consumer ---
    @KafkaListener(topics = "orders", groupId = "order-batch-group",
                   containerFactory = "batchFactory")
    public void consumeBatch(List<OrderEvent> events, Acknowledgment ack) {
        events.forEach(this::processOrder);
        ack.acknowledge();  // Commit after entire batch
    }

    // --- With full metadata ---
    @KafkaListener(topics = "orders", groupId = "order-service-group")
    public void consumeWithHeaders(
            @Payload OrderEvent event,
            @Header(KafkaHeaders.RECEIVED_PARTITION) int partition,
            @Header(KafkaHeaders.OFFSET) long offset,
            @Header(KafkaHeaders.RECEIVED_TIMESTAMP) long timestamp,
            Acknowledgment ack) {
        log.info("Partition={}, Offset={}, Timestamp={}", 
                 partition, offset, timestamp);
        processOrder(event);
        ack.acknowledge();
    }
}
```

---

## 10. Threading Model — Kafka + Spring Boot

> **This is a critical interview topic.** Most candidates know how to use `@KafkaListener` but can't explain the threading model underneath.

### The Core Rule

```
⚠️  KafkaConsumer is NOT thread-safe.
    One consumer instance = one thread.
    Spring Kafka manages this for you via the listener container.
```

### How `spring.kafka.listener.concurrency` Works

```
concurrency = 3  (Topic has 6 partitions)

┌───────────────────────────────────────────────────────────────┐
│             KafkaListenerContainer                            │
│                                                               │
│  ┌──────────────────┐  ┌──────────────────┐  ┌────────────┐   │
│  │ Thread-1         │  │ Thread-2         │  │ Thread-3   │   │
│  │ KafkaConsumer-1  │  │ KafkaConsumer-2  │  │ Consumer-3 │   │
│  │ Assigned: P0, P1 │  │ Assigned: P2, P3 │  │ P4, P5     │   │
│  │                  │  │                  │  │            │   │
│  │ poll() loop      │  │ poll() loop      │  │ poll()     │   │
│  │   → deserialize  │  │   → deserialize  │  │ loop       │   │
│  │   → call handler │  │   → call handler │  │            │   │
│  │   → commit       │  │   → commit       │  │            │   │
│  └──────────────────┘  └──────────────────┘  └────────────┘   │
└───────────────────────────────────────────────────────────────┘

Each thread:
  1. Runs its own poll() loop
  2. Has its own KafkaConsumer instance
  3. Gets assigned a subset of partitions
  4. Processes records SEQUENTIALLY within its thread
```

**If `concurrency > number of partitions` → extra threads sit idle.**

### The Problem: Slow Processing Blocks the Poll Loop

```
Thread-1:
  poll() → gets 100 records
    → process record 1 (takes 2 seconds — calling external API)
    → process record 2 (takes 3 seconds — heavy DB write)
    → ...
    → Total: 200 seconds for 100 records
    → Next poll() call: 200 seconds later!

If max.poll.interval.ms = 300 seconds (5 min):
  → Still OK, but barely.

If processing takes > max.poll.interval.ms:
  → Consumer kicked from group → REBALANCE → duplicates!
```

### Solution 1: Async Processing with ExecutorService

```java
@Service
@Slf4j
public class AsyncOrderConsumer {

    private final ExecutorService executor = Executors.newFixedThreadPool(10);

    @KafkaListener(topics = "orders", groupId = "order-async-group")
    public void consume(OrderEvent event, Acknowledgment ack) {
        // Immediately hand off to thread pool
        CompletableFuture.runAsync(() -> {
            try {
                processOrder(event);  // Slow operation
            } catch (Exception e) {
                log.error("Async processing failed", e);
            }
        }, executor).whenComplete((result, ex) -> {
            ack.acknowledge();  // Commit after async processing completes
        });
    }

    // ⚠️ CAUTION: This decouples offset commit from processing.
    // If the app crashes between processing and ack, data could be lost.
    // Solution: Use at-least-once + idempotent consumer pattern.
}
```

### Solution 2: Custom ConcurrentKafkaListenerContainerFactory

```java
@Configuration
public class KafkaConsumerConfig {

    @Bean
    public ConcurrentKafkaListenerContainerFactory<String, OrderEvent> 
            kafkaListenerContainerFactory(ConsumerFactory<String, OrderEvent> cf) {
        
        ConcurrentKafkaListenerContainerFactory<String, OrderEvent> factory = 
            new ConcurrentKafkaListenerContainerFactory<>();
        
        factory.setConsumerFactory(cf);
        factory.setConcurrency(6);  // Match your partition count
        factory.getContainerProperties().setAckMode(AckMode.MANUAL_IMMEDIATE);
        
        // Batch processing
        factory.setBatchListener(true);
        
        // Common error handler with retry
        factory.setCommonErrorHandler(new DefaultErrorHandler(
            new FixedBackOff(1000L, 3L)  // 1s delay, 3 retries
        ));
        
        return factory;
    }
}
```

### Solution 3: Virtual Threads (Java 21+)

```java
@Configuration
public class VirtualThreadConfig {

    @Bean
    public ConcurrentKafkaListenerContainerFactory<String, OrderEvent>
            virtualThreadFactory(ConsumerFactory<String, OrderEvent> cf) {
        
        ConcurrentKafkaListenerContainerFactory<String, OrderEvent> factory =
            new ConcurrentKafkaListenerContainerFactory<>();
        factory.setConsumerFactory(cf);
        
        // Use virtual threads for listener execution
        factory.getContainerProperties()
            .setListenerTaskExecutor(
                new SimpleAsyncTaskExecutor("kafka-vt-"));  
        // SimpleAsyncTaskExecutor with Spring Boot 3.2+ uses virtual threads
        // when spring.threads.virtual.enabled=true
        
        return factory;
    }
}
```

### Thread Safety Rules — Summary

| Component | Thread-Safe? | Notes |
|:----------|:-------------|:------|
| `KafkaConsumer` | ❌ NO | One instance per thread. Never share. |
| `KafkaProducer` | ✅ YES | Thread-safe. Share one instance across threads. |
| `KafkaTemplate` | ✅ YES | Wraps the producer. Safe to inject everywhere. |
| `@KafkaListener` | ✅ Managed | Spring creates separate consumers per `concurrency` thread. |

### Heartbeat vs. Poll — Two Separate Mechanisms

```
┌────────────────────────────────────────────────┐
│              Consumer Thread                   │
│                                                │
│  while (true) {                                │
│      records = poll(timeout);  ◄── Controlled  │
│      process(records);             by          │
│  }                                 max.poll.   │ 
│                                    interval.ms │
│                                                │
│  ┌─────────────────────┐                       │
│  │ Heartbeat Thread    │ ◄── Separate thread!  │
│  │ Sends heartbeats    │     Controlled by     │
│  │ every 3s (default)  │     heartbeat.        │ 
│  └─────────────────────┘     interval.ms       │
│                                                │
│  session.timeout.ms = 45s                      │
│  → If no heartbeat for 45s → consumer dead     │ 
│                                                │
│  max.poll.interval.ms = 300s                   │
│  → If no poll() for 300s → consumer dead       │
└────────────────────────────────────────────────┘

Key insight: Heartbeat thread runs independently.
So even if your processing is slow (but < max.poll.interval.ms),
heartbeats still keep the consumer alive.
```

---

## 11. Exactly-Once Semantics (EOS)

### The Three Pillars

```
  Idempotent Producer (PID + Sequence Number)
+ Transactional Producer (atomically write to multiple partitions)
+ Consumer read_committed isolation
= Exactly-Once Semantics
```

### How Kafka Transactions Work

```
BEGIN TRANSACTION
  1. Producer writes to topic "orders"   (partition 0)
  2. Producer writes to topic "audit"    (partition 2)
  3. Consumer offsets committed to __consumer_offsets topic
COMMIT TRANSACTION

All 3 writes are atomic:
  - Either ALL are visible to read_committed consumers
  - Or NONE are visible (if abort)
```

### Spring Boot EOS Configuration

```yaml
spring:
  kafka:
    producer:
      transaction-id-prefix: tx-order-svc-    # Enables transactions
      properties:
        enable.idempotence: true
    consumer:
      isolation-level: read_committed          # Only read committed messages
      enable-auto-commit: false
```

```java
@Service
@RequiredArgsConstructor
public class TransactionalOrderService {

    private final KafkaTemplate<String, OrderEvent> kafkaTemplate;

    // Option 1: Declarative with @Transactional
    @Transactional
    public void processOrder(OrderEvent order) {
        kafkaTemplate.send("orders", order.getOrderId(), order);
        kafkaTemplate.send("audit-log", order.getOrderId(), createAuditEvent(order));
        // Both sends are part of the same Kafka transaction
    }

    // Option 2: Programmatic
    public void processOrderProgrammatic(OrderEvent order) {
        kafkaTemplate.executeInTransaction(ops -> {
            ops.send("orders", order.getOrderId(), order);
            ops.send("audit-log", order.getOrderId(), createAuditEvent(order));
            return null;
        });
    }
}
```

### ⚠️ EOS Limitations
- **Kafka → Kafka only.** Transactions do NOT span external databases.
- If you need DB + Kafka atomicity → Use the **Transactional Outbox Pattern**.
- Performance cost: ~3-5% throughput reduction.
- Requires `max.in.flight.requests.per.connection ≤ 5`.

---

## 12. Transactional Outbox Pattern

### The Dual-Write Problem

```
// ❌ WRONG: This is a dual-write anti-pattern
@Transactional
public void createOrder(Order order) {
    orderRepository.save(order);                // Write 1: DB
    kafkaTemplate.send("orders", orderEvent);   // Write 2: Kafka
}

// What if the app crashes between save() and send()?
//   → DB has the order, Kafka doesn't → INCONSISTENCY
//
// What if Kafka send succeeds but DB transaction rolls back?
//   → Kafka has a phantom event → INCONSISTENCY
```

### The Solution: Outbox Pattern

```
┌───────────────────────────────────────────────────────────────┐
│                    Service (Spring Boot)                      │
│                                                               │
│  @Transactional                                               │
│  public void createOrder(Order order) {                       │
│      orderRepository.save(order);          ← Same DB          │
│      outboxRepository.save(new OutboxEvent(                   │
│          "orders",                           transaction      │
│          order.getId(),                                       │
│          toJson(orderEvent)                                   │
│      ));                                                      │
│  }                                                            │
│  // Both writes are in the SAME local ACID transaction        │
└───────────────────────────────────────────────────────────────┘
                           │
                           ▼
┌───────────────────────────────────────────────────────────────┐
│               Outbox Relay / CDC Connector                    │
│                                                               │
│  Option A: Polling Publisher                                  │
│    @Scheduled(fixedDelay = 1000)                              │
│    SELECT * FROM outbox WHERE published = false               │
│    FOR UPDATE SKIP LOCKED                                     │
│    → Send to Kafka                                            │
│    → Mark as published                                        │
│                                                               │
│  Option B: Debezium CDC                                       │
│    Reads DB transaction log (WAL/binlog)                      │
│    → Automatically publishes changes to Kafka                 │
│    → No polling, near-real-time                               │
└───────────────────────────────────────────────────────────────┘
```

### Outbox Table Schema

```sql
CREATE TABLE outbox_events (
    id              UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    aggregate_type  VARCHAR(255) NOT NULL,    -- e.g., "Order"
    aggregate_id    VARCHAR(255) NOT NULL,    -- e.g., order ID
    event_type      VARCHAR(255) NOT NULL,    -- e.g., "OrderCreated"
    topic           VARCHAR(255) NOT NULL,    -- Kafka topic
    payload         JSONB NOT NULL,           -- Serialized event
    created_at      TIMESTAMP DEFAULT NOW(),
    published       BOOLEAN DEFAULT FALSE,
    published_at    TIMESTAMP
);

CREATE INDEX idx_outbox_unpublished ON outbox_events(published) WHERE published = false;
```

### Polling Publisher Implementation

```java
@Service
@RequiredArgsConstructor
@Slf4j
public class OutboxPublisher {

    private final OutboxRepository outboxRepository;
    private final KafkaTemplate<String, String> kafkaTemplate;

    @Scheduled(fixedDelay = 1000)  // Every 1 second
    @Transactional
    public void publishOutboxEvents() {
        List<OutboxEvent> events = outboxRepository
            .findUnpublishedForUpdate();  // SELECT ... FOR UPDATE SKIP LOCKED

        for (OutboxEvent event : events) {
            try {
                kafkaTemplate.send(event.getTopic(), event.getAggregateId(), 
                                   event.getPayload())
                    .get(5, TimeUnit.SECONDS);  // Wait for ack
                
                event.setPublished(true);
                event.setPublishedAt(Instant.now());
                outboxRepository.save(event);
            } catch (Exception e) {
                log.error("Failed to publish outbox event: {}", event.getId(), e);
                // Will be retried on next scheduled run
            }
        }
    }
}
```

---

## 13. Dead Letter Queue (DLT)

### What Is It?
A "parking lot" for messages that fail processing after all retries are exhausted. Prevents "poison pill" messages from blocking the partition.

### Spring Kafka DLT Configuration

```java
@Configuration
public class KafkaErrorConfig {

    @Bean
    public DefaultErrorHandler errorHandler(
            KafkaOperations<Object, Object> kafkaTemplate) {
        
        // Publish failed messages to a DLT (Dead Letter Topic)
        DeadLetterPublishingRecoverer recoverer = 
            new DeadLetterPublishingRecoverer(kafkaTemplate,
                // Custom DLT topic naming: original-topic.DLT
                (record, ex) -> new TopicPartition(
                    record.topic() + ".DLT", record.partition()));
        
        // Retry 3 times with 1-second backoff, then send to DLT
        FixedBackOff backOff = new FixedBackOff(1000L, 3L);
        
        DefaultErrorHandler errorHandler = new DefaultErrorHandler(recoverer, backOff);
        
        // Don't retry on these exceptions (send directly to DLT)
        errorHandler.addNotRetryableExceptions(
            DeserializationException.class,
            ClassCastException.class,
            NullPointerException.class
        );
        
        return errorHandler;
    }
}
```

### DLT Consumer — Processing Failed Messages

```java
@KafkaListener(topics = "orders.DLT", groupId = "dlt-processor-group")
public void processDlt(
        ConsumerRecord<String, OrderEvent> record,
        @Header(KafkaHeaders.DLT_EXCEPTION_MESSAGE) String exceptionMessage,
        @Header(KafkaHeaders.DLT_ORIGINAL_TOPIC) String originalTopic,
        @Header(KafkaHeaders.DLT_ORIGINAL_OFFSET) String originalOffset) {
    
    log.error("DLT received from topic={}, offset={}, error={}", 
              originalTopic, originalOffset, exceptionMessage);
    
    // Options:
    // 1. Alert ops team
    // 2. Store in a "failed events" DB table for manual review
    // 3. Attempt different processing logic
    // 4. Forward to a human-in-the-loop system
}
```

### Retry Strategies

| Strategy | Class | Use Case |
|:---------|:------|:---------|
| **Fixed Backoff** | `FixedBackOff(interval, maxAttempts)` | Simple, predictable |
| **Exponential Backoff** | `ExponentialBackOff` | Gradual increase: 1s, 2s, 4s, 8s... |
| **Non-blocking Retry** | `@RetryableTopic` | Retry via separate topics, doesn't block partition |

### Non-Blocking Retry with `@RetryableTopic`

```java
@RetryableTopic(
    attempts = "4",
    backoff = @Backoff(delay = 1000, multiplier = 2.0),
    dltStrategy = DltStrategy.ALWAYS_RETRY_ON_ERROR,
    topicSuffixingStrategy = TopicSuffixingStrategy.SUFFIX_WITH_INDEX_VALUE
)
@KafkaListener(topics = "orders")
public void consume(OrderEvent event) {
    processOrder(event);
    // If this throws, message goes to:
    // orders-retry-0 (1s delay)
    // orders-retry-1 (2s delay)
    // orders-retry-2 (4s delay)
    // orders.DLT (final failure)
}
```

---

## 14. Idempotent Consumer Design

> **Rule: Always design consumers to be idempotent.** Even with EOS, edge cases exist (app restarts, rebalances).

### Strategy 1: Idempotency Key in Database

```java
@Service
@RequiredArgsConstructor
public class IdempotentOrderConsumer {

    private final ProcessedEventRepository processedEventRepo;
    private final OrderService orderService;

    @KafkaListener(topics = "orders")
    @Transactional
    public void consume(OrderEvent event, Acknowledgment ack) {
        String idempotencyKey = event.getEventId();  // Unique event ID
        
        // Check if already processed
        if (processedEventRepo.existsById(idempotencyKey)) {
            log.info("Duplicate event, skipping: {}", idempotencyKey);
            ack.acknowledge();
            return;
        }
        
        // Process
        orderService.createOrder(event);
        
        // Record as processed (same DB transaction)
        processedEventRepo.save(new ProcessedEvent(idempotencyKey, Instant.now()));
        
        ack.acknowledge();
    }
}
```

### Strategy 2: Database Upsert (Natural Idempotency)

```sql
-- Instead of INSERT, use upsert
INSERT INTO orders (order_id, status, amount, updated_at)
VALUES ('ORD-123', 'CREATED', 99.99, NOW())
ON CONFLICT (order_id) DO UPDATE 
SET status = EXCLUDED.status, 
    amount = EXCLUDED.amount,
    updated_at = EXCLUDED.updated_at
WHERE orders.updated_at < EXCLUDED.updated_at;  -- Only update if newer
```

### Strategy 3: Version-Based Deduplication

```java
// Use Kafka record metadata as dedup key
String dedupKey = String.format("%s-%d-%d", 
    record.topic(), record.partition(), record.offset());
// Topic + partition + offset is globally unique
```

---

## 15. Kafka Performance Tuning

### Producer Tuning

```
                              Throughput
                                 ▲
                                 │
        batch.size=64KB          │      ●  (batch.size=64KB + linger.ms=50)
        linger.ms=0              │     /
            ●                    │    /
           /                     │   /
          /                      │  ●  (batch.size=16KB + linger.ms=50)
         /                       │
        /                        │
       ● (defaults)              │
       └─────────────────────────┼──────────────► Latency
                                 │
```

| Tuning Goal | Configuration |
|:------------|:--------------|
| **Max throughput** | `batch.size=65536`, `linger.ms=50`, `compression.type=lz4` |
| **Low latency** | `batch.size=0`, `linger.ms=0` (send immediately) |
| **Balanced** | `batch.size=32768`, `linger.ms=10`, `compression.type=snappy` |

### Consumer Tuning

| Tuning Goal | Configuration |
|:------------|:--------------|
| **Higher throughput** | Increase `max.poll.records`, increase `fetch.min.bytes`, use batch listener |
| **Lower latency** | Decrease `fetch.max.wait.ms`, decrease `max.poll.records` |
| **Avoid rebalances** | Increase `max.poll.interval.ms`, optimize processing time, use async processing |

### Monitoring — Key Metrics

| Metric | What It Tells You | Alert Threshold |
|:-------|:------------------|:----------------|
| **Consumer Lag** | How far behind the consumer is from the latest offset | > 10,000 records |
| **Under-replicated Partitions** | Partitions where ISR < replication factor | > 0 |
| **Request Rate** | Produce/fetch requests per second | Sudden drops |
| **ISR Shrinks** | Rate at which replicas fall out of sync | > 0 per minute |
| **Disk Usage** | Per-broker storage | > 75% |
| **GC Pauses** | Broker JVM garbage collection | > 500ms |

---

## 16. Event-Driven Architecture Patterns

### 1. Event Notification
```
Order Service ──► "OrderCreated" event ──► Notification Service
                                       ──► Inventory Service
                                       ──► Analytics Service

Events are thin: just ID + type. Consumers call back for details.
```

### 2. Event-Carried State Transfer (ECST)
```
Order Service ──► "OrderCreated" event (contains full order data)
                  ──► Notification Service (no callback needed)
                  ──► Inventory Service (has all data it needs)

Events are fat: contain the full state. Consumers are decoupled.
Downside: Events are larger, potentially stale.
```

### 3. Event Sourcing
```
Instead of storing current state:
  Order { id: 123, status: "SHIPPED", amount: 99.99 }

Store the sequence of events:
  1. OrderCreated { id: 123, amount: 99.99 }
  2. PaymentReceived { id: 123, paymentId: "PAY-456" }
  3. OrderShipped { id: 123, trackingNo: "TRACK-789" }

Current state = replay(all events for aggregate)

Advantages:
  ✅ Full audit trail
  ✅ Time-travel debugging
  ✅ Can rebuild read models from events

Challenges:
  ⚠️ Event schema evolution (versioning)
  ⚠️ Snapshot optimization needed for long event streams
  ⚠️ Eventual consistency is the norm
```

### 4. CQRS (Command Query Responsibility Segregation)
```
┌──────────┐     Commands     ┌───────────────┐
│  Client  │ ─────────────►   │  Write Model  │ ──► Event Store / Kafka
│          │                  │  (Domain)     │
│          │     Queries      └───────────────┘
│          │ ─────────────► ┌────────────────┐         │
└──────────┘                │  Read Model    │  ◄──────┘
                            │  (Projections) │    Events update
                            │  (Denormalized │    the read model
                            │   DB views)    │
                            └────────────────┘

Write model: Optimized for business rules, validation, consistency
Read model: Optimized for queries, denormalized, eventually consistent
```

### 5. Saga Pattern (Distributed Transactions)
```
Order Saga (Choreography):

  Order Service ──► "OrderCreated" ──► Payment Service
                                            │
                                    "PaymentCompleted"
                                            │
                                    Inventory Service
                                            │
                                    "InventoryReserved"
                                            │
                                    Shipping Service

  If PaymentFailed:
    ──► "PaymentFailed" ──► Order Service ──► Compensate (cancel order)

  Choreography: Each service listens and reacts. No central coordinator.
  Orchestration: A Saga Orchestrator tells each service what to do.
```

---

## 17. Distributed Concurrency & Locking

### Why Local Locks Don't Work

```
Pod-1 (synchronized block)     Pod-2 (synchronized block)
        │                               │
        ▼                               ▼
   Lock acquired                   Lock acquired
   (local JVM lock)               (local JVM lock)
        │                               │
        ▼                               ▼
   Access shared DB resource       Access shared DB resource
        │                               │
        ▼                               ▼
     💥 BOTH pods access simultaneously!
        Java Semaphore, ReentrantLock, synchronized — 
        all useless across JVM boundaries.
```

### Distributed Locking Solutions

| Solution | Tool | Trade-offs |
|:---------|:-----|:-----------|
| **Redis-based** | Redisson, Spring Integration | Fast, but requires careful TTL management. Not perfectly safe (Redlock debate). |
| **ZooKeeper** | Curator `InterProcessMutex` | Strong guarantees, but adds infra dependency. |
| **Database** | `SELECT ... FOR UPDATE` | Simple, no extra infra. But limited throughput. |
| **Kafka** | Topic-based coordination | Use compacted topics as a coordination channel. |

### Redisson Distributed Lock Example

```java
@Service
@RequiredArgsConstructor
public class DistributedPaymentService {

    private final RedissonClient redissonClient;

    public void processPayment(String orderId) {
        RLock lock = redissonClient.getLock("payment-lock:" + orderId);
        
        try {
            // Wait up to 10s to acquire, auto-release after 30s
            boolean acquired = lock.tryLock(10, 30, TimeUnit.SECONDS);
            if (!acquired) {
                throw new RuntimeException("Could not acquire lock for " + orderId);
            }
            
            // Critical section — only one pod executes this per orderId
            doPaymentProcessing(orderId);
            
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        } finally {
            if (lock.isHeldByCurrentThread()) {
                lock.unlock();
            }
        }
    }
}
```

### Optimistic Concurrency Control (Alternative)

```java
@Entity
public class Order {
    @Id
    private String orderId;
    
    @Version
    private Long version;  // JPA auto-increments on update
    
    private String status;
    private BigDecimal amount;
}

// When two concurrent updates happen:
// Update 1: version=1 → version=2  ✅ Success
// Update 2: version=1 → OptimisticLockException!  ❌ Retry
```

---

## 18. Observability in Distributed Systems

### The Three Pillars

```
┌────────────────────────────────────────────────────┐
│                 Observability                      │
│                                                    │
│  ┌───────────┐  ┌───────────┐  ┌───────────────┐   │
│  │   Logs    │  │  Metrics  │  │   Traces      │   │ 
│  │           │  │           │  │               │   │
│  │ What      │  │ How much  │  │ Where (flow   │   │
│  │ happened  │  │ & how fast│  │ across svcs)  │   │
│  │           │  │           │  │               │   │
│  │ ELK/Loki  │  │Prometheus │  │ Jaeger/Zipkin │   │
│  │           │  │+ Grafana  │  │ + OTel        │   │
│  └───────────┘  └───────────┘  └───────────────┘   │
└────────────────────────────────────────────────────┘
```

### Structured Logging Best Practices

```java
// ✅ Good: Structured, contextual
log.info("Order processed", 
    kv("orderId", order.getId()),
    kv("customerId", order.getCustomerId()),
    kv("amount", order.getAmount()),
    kv("durationMs", stopwatch.elapsed()));

// ❌ Bad: Unstructured, hard to parse
log.info("Processed order " + order.getId() + " for customer " + 
         order.getCustomerId());
```

### Distributed Tracing with OpenTelemetry

```
Request flow through services:

Client → API Gateway → Order Service → Payment Service → DB
   │         │              │               │            │
   └─────────┴──────────────┴───────────────┴────────────┘
                    Single Trace (Trace ID: abc-123)
                    
   Span 1: API Gateway (parent)
     └── Span 2: Order Service 
           └── Span 3: Kafka Produce ("orders")
           └── Span 4: Payment Service (HTTP call)
                 └── Span 5: Database query

Each span has: trace_id, span_id, parent_span_id, start_time, duration, tags
```

### Spring Boot + Micrometer + Kafka Metrics

```yaml
# application.yml
management:
  endpoints:
    web:
      exposure:
        include: prometheus, health, info
  metrics:
    tags:
      application: order-service
    distribution:
      percentiles-histogram:
        http.server.requests: true

# Key Kafka metrics exposed automatically:
# kafka.consumer.records.consumed.total
# kafka.consumer.records.lag.max
# kafka.producer.record.send.total
# kafka.producer.record.error.total
```

---

## 19. Resilience Patterns for Production

### Circuit Breaker (Resilience4j)

```java
@Service
public class PaymentGateway {

    @CircuitBreaker(name = "paymentService", fallbackMethod = "fallback")
    @Retry(name = "paymentService")
    @RateLimiter(name = "paymentService")
    public PaymentResult processPayment(PaymentRequest request) {
        return paymentClient.charge(request);  // External call
    }

    public PaymentResult fallback(PaymentRequest request, Exception ex) {
        log.warn("Circuit open, using fallback for order: {}", request.getOrderId());
        return PaymentResult.pending(request.getOrderId());
        // Queue for later retry
    }
}
```

```yaml
# application.yml
resilience4j:
  circuitbreaker:
    instances:
      paymentService:
        sliding-window-size: 10
        failure-rate-threshold: 50         # Open circuit at 50% failure
        wait-duration-in-open-state: 30s   # Stay open for 30s
        permitted-number-of-calls-in-half-open-state: 3
  retry:
    instances:
      paymentService:
        max-attempts: 3
        wait-duration: 1s
        exponential-backoff-multiplier: 2
```

### Bulkhead Pattern

```java
// Thread pool bulkhead — isolates failures
@Bulkhead(name = "paymentService", type = Bulkhead.Type.THREADPOOL)
public CompletableFuture<PaymentResult> processPayment(PaymentRequest req) {
    return CompletableFuture.supplyAsync(() -> paymentClient.charge(req));
}
```

```
Without Bulkhead:                    With Bulkhead:
┌──────────────────────┐            ┌──────────────────────┐
│ Shared Thread Pool   │            │ Service A Pool (10)  │
│ (200 threads)        │            │ ┌─────┐ ┌─────┐      │
│                      │            │ │ t1  │ │ t2  │ ...  │
│ Service A calls: 190 │ ← Starves  │ └─────┘ └─────┘      │
│ Service B calls: 10  │            │                      │
│                      │            │ Service B Pool (10)  │
│ If A is slow, B      │            │ ┌─────┐ ┌─────┐      │
│ can't get threads!   │            │ │ t1  │ │ t2  │ ...  │
└──────────────────────┘            │ └─────┘ └─────┘      │
                                    │ Isolated!            │
                                    └──────────────────────┘
```

### Rate Limiting & Load Shedding

```java
// Rate limiter — prevent overwhelming downstream
@RateLimiter(name = "externalApi")
public Result callExternalApi(Request req) {
    return externalClient.call(req);
}

// Load shedding — reject excess requests
@TimeLimiter(name = "criticalService")
public CompletableFuture<Result> criticalOperation() {
    return CompletableFuture.supplyAsync(() -> {
        // If this takes too long, CompletableFuture is cancelled
        return doWork();
    });
}
```

### Backpressure in Kafka

```
When consumer can't keep up:

Option 1: Increase partitions + consumers (horizontal scaling)
Option 2: Buffer in-memory, process in batches
Option 3: Pause consumer, drain buffer, resume

// Programmatic pause/resume in Spring Kafka
@Autowired
KafkaListenerEndpointRegistry registry;

public void pauseConsumer() {
    MessageListenerContainer container = registry.getListenerContainer("myListener");
    container.pause();  // Stops fetching from broker
}

public void resumeConsumer() {
    MessageListenerContainer container = registry.getListenerContainer("myListener");
    container.resume();
}
```

---

## 20. Kafka vs Other Message Brokers

| Feature | **Kafka** | **RabbitMQ** | **AWS SQS** | **Pulsar** |
|:--------|:----------|:-------------|:------------|:-----------|
| **Model** | Distributed log | Traditional message queue | Managed queue | Distributed log |
| **Ordering** | Per-partition | Per-queue | Best-effort (FIFO queues available) | Per-partition |
| **Retention** | Configurable (days/weeks) | Until consumed | 14 days max | Configurable + tiered storage |
| **Throughput** | Millions/sec | ~50K/sec | Auto-scales | Millions/sec |
| **Replay** | ✅ Yes (offset reset) | ❌ No (consumed = deleted) | ❌ No | ✅ Yes |
| **Consumer Groups** | ✅ Native | ❌ (use exchanges/bindings) | ❌ (manual) | ✅ Native |
| **Exactly-Once** | ✅ (Kafka transactions) | ❌ (at-least-once) | ❌ (at-least-once) | ✅ |
| **Best For** | Event streaming, logs, CDC, high-throughput | Task queues, RPC, complex routing | Simple cloud-native queuing | Multi-tenant, geo-replication |

### When to Use Kafka
✅ Event streaming / real-time data pipelines  
✅ Event sourcing / audit trails  
✅ Log aggregation  
✅ CDC (Change Data Capture)  
✅ High-throughput messaging (millions/sec)  
✅ Message replay required  

### When NOT to Use Kafka
❌ Simple task queue (use RabbitMQ/SQS)  
❌ Request-reply pattern (use gRPC/REST)  
❌ Very low message volume (overkill)  
❌ Complex routing logic (use RabbitMQ exchanges)  

---

## 21. Interview Questions — Scenario-Based

### Q1: "Our consumer takes 10 seconds per message. `max.poll.interval.ms` is 5 seconds. What happens?"
**A:** Constant rebalancing. The consumer can't call `poll()` within 5 seconds, so the broker considers it dead and triggers a rebalance. The partitions get reassigned, the consumer rejoins, the same thing happens again — infinite rebalance loop. **Fix:** Increase `max.poll.interval.ms`, reduce `max.poll.records`, or use async processing to hand off to a thread pool.

### Q2: "How would you guarantee exactly-once processing when writing to both a database and Kafka?"
**A:** You can't use Kafka transactions for this (they only span Kafka topics). Use the **Transactional Outbox Pattern**: write the event to an outbox table in the same DB transaction as your business data. A separate relay process (polling or CDC with Debezium) publishes from the outbox table to Kafka.

### Q3: "We have 12 partitions and 6 consumer instances. What happens if we add 8 more consumers?"
**A:** Only 12 consumers can be active (one per partition). The remaining 2 will be idle, sitting in the consumer group but not assigned any partitions. This wastes resources. **Better:** Scale partitions first, then scale consumers.

### Q4: "A single 'poison pill' message keeps crashing our consumer. How do you handle it?"
**A:** Configure a `DefaultErrorHandler` with retry + `DeadLetterPublishingRecoverer`. After N retries, the message is sent to a Dead Letter Topic (DLT). The consumer moves on to the next message. A separate DLT consumer handles failed messages (alert, manual review, alternate processing).

### Q5: "How does Kafka achieve high throughput despite using disk?"
**A:** Four mechanisms: (1) Sequential I/O (append-only writes avoid random seeks), (2) Zero-copy transfer (`sendfile()` syscall), (3) Batching (amortizes network overhead), (4) Page cache (OS-level caching, avoids JVM GC).

### Q6: "What's the difference between `session.timeout.ms` and `max.poll.interval.ms`?"
**A:** `session.timeout.ms` controls heartbeat-based liveness (a background thread sends heartbeats). `max.poll.interval.ms` controls processing-based liveness (how long between `poll()` calls). A consumer can have an active heartbeat but still be kicked if it doesn't call `poll()` within `max.poll.interval.ms`.

### Q7: "Explain the ISR mechanism and what happens during a broker failure."
**A:** ISR = set of replicas fully caught up with the leader. When a broker hosting the leader fails: (1) The controller detects the failure, (2) A new leader is elected from the ISR, (3) Consumers/producers discover the new leader via metadata refresh, (4) If `unclean.leader.election.enable=false` and no ISR members are alive, the partition goes offline. With `min.insync.replicas=2` and `acks=all`, producers are guaranteed that at least 2 replicas have the data before acknowledging.

### Q8: "Design a notification system that sends emails when an order is placed."
**A:** 
```
Order Service → [Kafka: "order-events"] → Notification Consumer
                                               │
                                    ┌──────────┤
                                    │          │
                              Idempotency   Email Service
                              Check (DB)    (SMTP/SES)
                                    │
                              Mark as sent
```
Key considerations: idempotent consumer (avoid duplicate emails), DLT for failed sends, circuit breaker on email service, rate limiting to avoid SMTP throttling.

---

## 22. What To Study Next — Roadmap

### You've Completed ✅
- [x] Distributed Systems Fundamentals (CAP, consensus, replication)
- [x] Raft Consensus Internals
- [x] Distributed Transactions (2PC vs Saga)
- [x] Microservices Architecture
- [x] Kafka + Event-Driven + Threading (this file)

### Recommended Next Topics 🎯

#### Priority 1 — High-Impact for Interviews
| Topic | Why | Suggested Resources |
|:------|:----|:--------------------|
| **System Design Case Studies** | Every senior interview has 1-2 system design rounds | *Designing Data-Intensive Applications* (Kleppmann), Alex Xu's *System Design Interview* |
| **Database Internals** | B-Trees vs LSM-Trees, WAL, MVCC, connection pooling | PostgreSQL internals blog, *Database Internals* (Petrov) |
| **gRPC & Protocol Buffers** | Standard for inter-service communication in distributed systems | You have practice code already in `02-grpc-protobuf/` |

#### Priority 2 — Production Depth
| Topic | Why |
|:------|:----|
| **Kubernetes & Container Orchestration** | How your services actually run in production |
| **Observability Deep Dive (OpenTelemetry)** | Distributed tracing, metrics, correlation IDs |
| **Chaos Engineering** | Jepsen, Chaos Monkey — proving systems work under failure |
| **CI/CD Pipelines** | Canary deployments, blue-green, feature flags |

#### Priority 3 — Advanced Distributed Systems
| Topic | Why |
|:------|:----|
| **CRDTs (Conflict-free Replicated Data Types)** | Eventual consistency without coordination |
| **Gossip Protocols** | Membership, failure detection (used by Cassandra, Consul) |
| **Consistent Hashing** | Data partitioning in distributed caches/DBs |
| **Vector Clocks & Lamport Timestamps** | Causality tracking in distributed systems |
| **LSM-Tree Compaction Strategies** | How Cassandra, RocksDB, LevelDB manage storage |

#### Recommended Books (Ordered)
1. 📘 **Designing Data-Intensive Applications** — Martin Kleppmann (THE bible)
2. 📗 **System Design Interview Vol 1 & 2** — Alex Xu
3. 📙 **Database Internals** — Alex Petrov
4. 📕 **Building Microservices** — Sam Newman
5. 📒 **Understanding Distributed Systems** — Roberto Vitillo

---

> **Last updated:** October 2026  
> **Author's note:** Focus on understanding *why* things work, not just *how*. In interviews, the "what happens when X fails?" questions separate senior engineers from juniors.
