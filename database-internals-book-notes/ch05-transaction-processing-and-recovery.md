<Chapter 5: Transaction Processing and Recovery>
> A deep dive into how database systems guarantee ACID properties using buffer management, logging mechanisms (WAL/ARIES), and concurrency control techniques (Locks, MVCC, OCC).

## Table of Contents
- [Chapter Overview](#chapter-overview)
- [ACID Properties](#acid-properties)
- [Buffer Management (Page Cache)](#buffer-management-page-cache)
  - [Caching Semantics & Eviction](#caching-semantics--eviction)
  - [Page Replacement Policies (LRU, CLOCK, TinyLFU)](#page-replacement-policies-lru-clock-tinylfu)
- [Recovery & Write-Ahead Logging (WAL)](#recovery--write-ahead-logging-wal)
  - [Log Semantics & Checkpoints](#log-semantics--checkpoints)
  - [Physical vs. Logical Logging](#physical-vs-logical-logging)
  - [Steal and Force Policies](#steal-and-force-policies)
  - [The ARIES Algorithm](#the-aries-algorithm)
- [Concurrency Control](#concurrency-control)
  - [Serializability & Isolation Levels](#serializability--isolation-levels)
  - [Read and Write Anomalies](#read-and-write-anomalies)
  - [Optimistic (OCC) vs. Pessimistic (PCC) vs. MVCC](#optimistic-occ-vs-pessimistic-pcc-vs-mvcc)
- [Lock-Based Concurrency Control](#lock-based-concurrency-control)
  - [Two-Phase Locking (2PL) & Deadlocks](#two-phase-locking-2pl--deadlocks)
  - [Locks vs. Latches](#locks-vs-latches)
  - [Latch Crabbing & B-Link Trees](#latch-crabbing--b-link-trees)
- [Worked Examples & Code Implementations](#worked-examples--code-implementations)
- [Design Patterns & Best Practices](#design-patterns--best-practices)
- [Trade-offs & Decision Framework](#trade-offs--decision-framework)
- [Interview & System Design Angles](#interview--system-design-angles)
- [AI/ML Engineering Relevance](#aiml-engineering-relevance)
- [Key Takeaways](#key-takeaways)
- [Glossary](#glossary)

## Chapter Overview
After exploring low-level storage structures (like B-Trees and LSM Trees), this chapter elevates the focus to the components responsible for the reliability and concurrency of modern databases: **buffer management, lock management, and recovery protocols**. These mechanisms are the absolute prerequisites for implementing database transactions. A modern DBMS must safely interleave operations from thousands of concurrent clients without sacrificing performance, while guaranteeing that a sudden power loss won’t corrupt data. We achieve this by splitting responsibilities across the Transaction Manager, Lock Manager, Page Cache, and Log Manager.

## ACID Properties
A transaction is an indivisible logical unit of work. It must preserve the four ACID properties:
*   **Atomicity:** "All or nothing." Transactions execute completely (commit) or completely revert (abort/rollback).
*   **Consistency:** The database moves from one valid state to another valid state, maintaining invariants (constraints, foreign keys). It is the only property largely controlled by the application logic rather than just the DB.
*   **Isolation:** Concurrently executing transactions should not interfere with each other. The illusion of serial execution.
*   **Durability:** Once committed, state modifications survive power outages, crashes, and system failures.

---

## Buffer Management (Page Cache)
### Core Concept
Disks are slow; RAM is fast. The **page cache** (or buffer pool) is a memory region bridging the storage engine and the disk. It stages state changes in main memory. To avoid kernel-level overhead and OS page cache eviction algorithms that lack database context, many systems open files with `O_DIRECT` to bypass the kernel cache.

### How It Works
1.  **Read/Write Semantics:** When a page is requested, the cache translates the logical page ID to a physical disk address. If the page is in RAM, a cached version is returned and marked as *referenced*. Modifications flag the page as *dirty*.
2.  **Pinning:** Frequently accessed pages (like B-Tree roots and upper levels) are *pinned* in memory to prevent eviction. This significantly drops the $O(\log N)$ I/O cost to a near-constant disk access for leaf nodes.
3.  **Eviction & Flushes:** To load new pages, old ones must be evicted. Dirty pages must be synced to disk first. A background checkpoint process coordinates flushing to keep the cache size within bounds without blocking queries.

### Page Replacement Policies (LRU, CLOCK, TinyLFU)
Evicting the wrong page leads to **Bélády’s anomaly** (where adding more cache increases cache misses due to poor eviction choices).

*   **FIFO & LRU:** FIFO evicts oldest pages, but this kills performance (e.g., evicting the B-Tree root). Least Recently Used (LRU) fixes this by re-promoting pages, but pointer manipulation for every read is costly in concurrent environments.
*   **CLOCK-sweep:** A concurrent alternative to LRU. It uses a circular buffer of pages and "access bits." The clock hand sweeps: if a bit is 1, it sets it to 0 and moves on. If 0, the page is evicted.
*   **TinyLFU (Frequency over Recency):** Some DBs track Least Frequently Used (LFU). TinyLFU maintains a compact frequency histogram. It uses three queues: Admission (LRU), Probation, and Protected. An item only enters Probation if its historical frequency beats the item currently scheduled for eviction.

---

## Recovery & Write-Ahead Logging (WAL)
### Core Concept
The **Write-Ahead Log (WAL)** or commit log is an append-only, sequential, immutable file on disk. The golden rule: **Every operation that modifies the database must be logged to the WAL before the associated dirty pages in the page cache can be flushed to disk.**

### Log Semantics & Checkpoints
*   **Log Sequence Number (LSN):** Every log record has a unique, monotonically increasing LSN.
*   **Checkpoints:** The process of flushing dirty pages to disk. Doing a *sync checkpoint* (pausing everything to flush) is too slow. Databases use **fuzzy checkpoints**: appending a `begin_checkpoint` log, flushing pages asynchronously, appending `end_checkpoint`, and updating the log header's `last_checkpoint` pointer.

### Physical vs. Logical Logging
*   **Physical Logging:** Records exact byte-wise changes (before-image / after-image) of an entire page.
*   **Logical Logging:** Records the abstract operation (e.g., "insert record X into table Y").
*   **Physiological Logging:** A hybrid (used by ARIES)—using logical logging for undo (better concurrency) and physical logging for redo (faster recovery).

### Steal and Force Policies
These dictate when dirty pages can be written to disk relative to a transaction's commit:
| Policy | Definition | Implication on Recovery |
| :--- | :--- | :--- |
| **Steal** | Uncommitted pages *can* be flushed to disk. | Requires **UNDO** logging (to roll back if transaction aborts). |
| **No-Steal** | Uncommitted pages *cannot* be flushed. | No UNDO needed, but requires massive RAM to hold dirty pages. |
| **Force** | All dirty pages *must* be flushed at commit. | No REDO needed, but causes terrible I/O latency on commit. |
| **No-Force** | Commits can succeed without flushing pages. | Requires **REDO** logging (to reapply changes on crash). |

*Most modern databases use a **Steal / No-Force** policy.*

### The ARIES Algorithm
ARIES (Algorithm for Recovery and Isolation Exploiting Semantics) is a Steal/No-Force algorithm. It relies on LSNs, physiological logging, and repeating history.
1.  **Analysis Phase:** Scans the WAL forward from the last checkpoint to identify dirty pages and active transactions at the time of the crash.
2.  **Redo Phase:** Repeats history forward, applying physical redo to restore the DB to its exact crash state (including uncommitted transactions).
3.  **Undo Phase:** Rolls back all uncompleted transactions in reverse chronological order using logical undo, writing Compensation Log Records (CLRs) so undone actions aren't repeated if the system crashes again during recovery.

---

## Concurrency Control
Concurrency control ensures isolation. Transactions must interleave safely.

### Read and Write Anomalies
*   **Dirty Read:** Reading uncommitted changes from another transaction.
*   **Nonrepeatable (Fuzzy) Read:** Reading the same row twice and getting different results due to another transaction's commit.
*   **Phantom Read:** Executing a range query twice and finding new "phantom" rows.
*   **Lost Update:** T1 and T2 read value V. Both update it. T2 commits after T1, completely wiping out T1's update.
*   **Dirty Write:** Modifying an uncommitted value written by another transaction.
*   **Write Skew:** T1 and T2 read overlapping data and update independent parts, satisfying local constraints but violating global invariants (e.g., withdrawing from two accounts without violating the combined minimum balance).

### Serializability & Isolation Levels
A schedule is **serializable** if its outcome is equivalent to executing transactions strictly one after another. Since purely serial execution is slow, SQL defines Isolation Levels:

| Isolation Level | Dirty Read | Nonrepeatable Read | Phantom Read |
| :--- | :--- | :--- | :--- |
| **Read Uncommitted** | Allowed | Allowed | Allowed |
| **Read Committed** | Prevented | Allowed | Allowed |
| **Repeatable Read** | Prevented | Prevented | Allowed |
| **Serializable** | Prevented | Prevented | Prevented |

> [!NOTE]
> **Snapshot Isolation (SI):** Reads from a static snapshot of the DB at start time. Avoids lost updates via "first-committer-wins" but is vulnerable to **Write Skew**.

### Optimistic (OCC) vs. Pessimistic (PCC) vs. MVCC
*   **Optimistic (OCC):** Assumes conflicts are rare. Three phases: Read (private context), Validate (check for conflicts with concurrent transactions), and Write. Aborts if validation fails.
*   **Multiversion (MVCC):** Reads don't block writes; writes don't block reads. Keeps multiple timestamped versions of records. Used widely (Postgres, MySQL InnoDB) to implement Snapshot Isolation.
*   **Pessimistic (PCC):** Assumes conflicts are common. Locks resources or aborts proactively. Lock-free variant: Timestamp Ordering (uses `max_read_timestamp` and aborts if someone read a newer value).

---

## Lock-Based Concurrency Control

### Two-Phase Locking (2PL) & Deadlocks
2PL is a pessimistic protocol to guarantee serializability.
*   **Growing Phase:** Acquire all locks needed; release none.
*   **Shrinking Phase:** Release locks; acquire no new ones.

**Deadlocks** occur when T1 waits for a lock held by T2, and T2 waits for a lock held by T1.
*   **Detection:** Transaction manager maintains a "waits-for" graph. A cycle means a deadlock; abort the youngest transaction.
*   **Avoidance:** Uses timestamps.
    *   *Wait-die:* Older transactions wait for younger ones; younger ones abort instead of waiting.
    *   *Wound-wait:* Older transactions abort ("wound") younger ones; younger ones wait for older ones.

### Locks vs. Latches
*   **Locks:** Guard *logical* data (keys/rows). Held for the duration of the transaction. Managed by Lock Manager.
*   **Latches:** Guard *physical* data (B-Tree pages, memory structures). Held only for the split second the page is accessed. Not prone to traditional deadlock detection (relies on careful programming).

### Latch Crabbing & B-Link Trees
When traversing a B-Tree, acquiring latches naively kills concurrency.
*   **Latch Crabbing (Coupling):** Optimistic descent. Acquire root latch, get child latch, check if child is "safe" (won't split/merge). If safe, release the parent latch immediately. Crabs down the tree.
*   **B-Link ($B^{link}$) Trees:** Adds "high keys" and "sibling link pointers" to nodes. If a thread reads a node while it splits, the search key will exceed the node's high key. The thread gracefully follows the sibling link pointer rightward. Completely removes the need to hold parent latches during splits, vastly increasing concurrency.

---

## Worked Examples & Code Implementations

### Example 1: CLOCK Page Replacement Algorithm (Buffer Eviction)

Simulates a circular buffer pool with a sweeping hand and reference/usage bits to evaluate page hits, misses, and victim frame evictions without requiring the continuous pointer manipulation of an LRU linked list.

```python
class ClockBufferPool:
    def __init__(self, capacity: int):
        self.capacity = capacity
        self.frames = [None] * capacity  # Cached page IDs
        self.usage_bits = [0] * capacity
        self.hand = 0
        self.hits = 0
        self.misses = 0

    def access_page(self, page_id: int) -> str:
        # Check for cache hit
        if page_id in self.frames:
            idx = self.frames.index(page_id)
            self.usage_bits[idx] = 1
            self.hits += 1
            return f"HIT  (Page {page_id} in Frame {idx})"

        # Cache miss: find victim frame using CLOCK sweep
        self.misses += 1
        while True:
            if self.frames[self.hand] is None:
                victim_idx = self.hand
                evicted = None
                break
            elif self.usage_bits[self.hand] == 1:
                # Clear usage bit and advance hand
                self.usage_bits[self.hand] = 0
                self.hand = (self.hand + 1) % self.capacity
            else:
                # Victim found: usage bit is 0
                victim_idx = self.hand
                evicted = self.frames[victim_idx]
                break

        self.frames[victim_idx] = page_id
        self.usage_bits[victim_idx] = 1
        self.hand = (victim_idx + 1) % self.capacity
        evict_str = f"Evicted Page {evicted}" if evicted is not None else "Empty Slot"
        return f"MISS (Page {page_id} -> Frame {victim_idx}, {evict_str})"

print("=== CLOCK Buffer Pool Eviction ===")
pool = ClockBufferPool(capacity=3)
page_stream = [1, 2, 3, 1, 4, 5, 2, 1]
for p in page_stream:
    status = pool.access_page(p)
    print(f"Access {p:2d} -> {status} | Frames: {pool.frames} | Bits: {pool.usage_bits}")
print(f"Summary: {pool.hits} Hits, {pool.misses} Misses (Hit Ratio: {pool.hits/len(page_stream)*100:.1f}%)")
```

#### Execution Output
```text
=== CLOCK Buffer Pool Eviction ===
Access  1 -> MISS (Page 1 -> Frame 0, Empty Slot) | Frames: [1, None, None] | Bits: [1, 0, 0]
Access  2 -> MISS (Page 2 -> Frame 1, Empty Slot) | Frames: [1, 2, None] | Bits: [1, 1, 0]
Access  3 -> MISS (Page 3 -> Frame 2, Empty Slot) | Frames: [1, 2, 3] | Bits: [1, 1, 1]
Access  1 -> HIT  (Page 1 in Frame 0) | Frames: [1, 2, 3] | Bits: [1, 1, 1]
Access  4 -> MISS (Page 4 -> Frame 0, Evicted Page 1) | Frames: [4, 2, 3] | Bits: [1, 0, 0]
Access  5 -> MISS (Page 5 -> Frame 1, Evicted Page 2) | Frames: [4, 5, 3] | Bits: [1, 1, 0]
Access  2 -> MISS (Page 2 -> Frame 2, Evicted Page 3) | Frames: [4, 5, 2] | Bits: [1, 1, 1]
Access  1 -> MISS (Page 1 -> Frame 0, Evicted Page 4) | Frames: [1, 5, 2] | Bits: [1, 0, 0]
Summary: 1 Hits, 7 Misses (Hit Ratio: 12.5%)
```

---

### Example 2: Mini-ARIES Crash Recovery Engine (Analysis, Redo, Undo)

Demonstrates the 3-phase ARIES recovery algorithm after an unexpected crash. In Phase 1 (Analysis), the engine reconstructs the Dirty Page Table (DPT) and Transaction Table. In Phase 2 (Redo), it repeats history starting from the oldest un-flushed `recLSN`. In Phase 3 (Undo), it rolls back uncommitted loser transactions in reverse LSN order with Compensation Log Records (CLRs).

```python
class ARIESRecovery:
    def __init__(self, wal_records: list, flushed_pages: dict):
        self.wal = wal_records
        self.pages = dict(flushed_pages)  # Physical disk state
        self.dpt = {}                     # Dirty Page Table: page_id -> recLSN
        self.tx_table = {}                # Transaction Table: tx_id -> status

    def run_recovery(self) -> dict:
        print("--- Phase 1: Analysis ---")
        for rec in self.wal:
            lsn, tx_id, rec_type, page_id, redo_val, undo_val = rec
            self.tx_table[tx_id] = {"lastLSN": lsn, "status": "ACTIVE"}
            
            if rec_type == "COMMIT":
                self.tx_table[tx_id]["status"] = "COMMITTED"
            elif rec_type == "UPDATE":
                if page_id not in self.dpt:
                    self.dpt[page_id] = lsn

        active_losers = [t for t, v in self.tx_table.items() if v["status"] == "ACTIVE"]
        print(f"Analysis complete: Active Losers: {active_losers} | DPT: {self.dpt}")

        print("--- Phase 2: Redo (Repeating History) ---")
        smallest_rec_lsn = min(self.dpt.values()) if self.dpt else 0
        for rec in self.wal:
            lsn, tx_id, rec_type, page_id, redo_val, undo_val = rec
            if lsn >= smallest_rec_lsn and rec_type == "UPDATE":
                self.pages[page_id] = redo_val
                print(f"  Redo LSN {lsn}: Set Page {page_id} = '{redo_val}'")

        print("--- Phase 3: Undo (Rolling Back Losers) ---")
        for rec in reversed(self.wal):
            lsn, tx_id, rec_type, page_id, redo_val, undo_val = rec
            if tx_id in active_losers and rec_type == "UPDATE":
                self.pages[page_id] = undo_val
                print(f"  Undo LSN {lsn} (Tx {tx_id}): Restored Page {page_id} = '{undo_val}' [CLR Logged]")

        return self.pages

wal_log = [
    # (LSN, tx_id, type, page_id, redo_val, undo_val)
    (1, "T1", "UPDATE", 101, "Alice $200", "Alice $100"),
    (2, "T2", "UPDATE", 102, "Bob $500", "Bob $300"),
    (3, "T1", "COMMIT", None, None, None),
    (4, "T3", "UPDATE", 101, "Alice $999", "Alice $200"),  # T3 crashes uncommitted!
]
flushed_disk = {101: "Alice $100", 102: "Bob $300"}

print("=== Mini-ARIES Crash Recovery Engine ===")
engine = ARIESRecovery(wal_log, flushed_disk)
recovered_state = engine.run_recovery()
print(f"Recovered Disk State: {recovered_state}")
```

#### Execution Output
```text
=== Mini-ARIES Crash Recovery Engine ===
--- Phase 1: Analysis ---
Analysis complete: Active Losers: ['T2', 'T3'] | DPT: {101: 1, 102: 2}
--- Phase 2: Redo (Repeating History) ---
  Redo LSN 1: Set Page 101 = 'Alice $200'
  Redo LSN 2: Set Page 102 = 'Bob $500'
  Redo LSN 4: Set Page 101 = 'Alice $999'
--- Phase 3: Undo (Rolling Back Losers) ---
  Undo LSN 4 (Tx T3): Restored Page 101 = 'Alice $200' [CLR Logged]
  Undo LSN 2 (Tx T2): Restored Page 102 = 'Bob $300' [CLR Logged]
Recovered Disk State: {101: 'Alice $200', 102: 'Bob $300'}
```

---

### Example 3: Deadlock Detection & Wound-Wait Avoidance

Demonstrates cycle detection in a transaction waits-for dependency graph using Depth-First Search (DFS), along with timestamp-based Wound-Wait deadlock prevention.

```python
def detect_deadlock(waits_for: dict) -> bool:
    """DFS Cycle detection on waits-for graph."""
    visited = set()
    rec_stack = set()
    
    def dfs(node):
        visited.add(node)
        rec_stack.add(node)
        for neighbor in waits_for.get(node, []):
            if neighbor not in visited:
                if dfs(neighbor):
                    return True
            elif neighbor in rec_stack:
                return True
        rec_stack.remove(node)
        return False

    for node in waits_for:
        if node not in visited:
            if dfs(node):
                return True
    return False

graph_with_cycle = {"T1": ["T2"], "T2": ["T3"], "T3": ["T1"]}
graph_no_cycle = {"T1": ["T2"], "T2": ["T3"], "T3": []}

print("=== Deadlock Detection & Wound-Wait ===")
print(f"Graph T1->T2->T3->T1 Deadlock Cycle Detected: {detect_deadlock(graph_with_cycle)}")
print(f"Graph T1->T2->T3    Deadlock Cycle Detected: {detect_deadlock(graph_no_cycle)}")

# Wound-Wait Avoidance (Older transaction has smaller timestamp)
def wound_wait(requester_ts: int, holder_ts: int) -> str:
    if requester_ts < holder_ts:
        return f"Older T(ts={requester_ts}) WOUNDS (aborts) Younger T(ts={holder_ts})"
    else:
        return f"Younger T(ts={requester_ts}) WAITS for Older T(ts={holder_ts})"

print(f"Conflict 1: {wound_wait(requester_ts=10, holder_ts=25)}")
print(f"Conflict 2: {wound_wait(requester_ts=30, holder_ts=15)}")
```

#### Execution Output
```text
=== Deadlock Detection & Wound-Wait ===
Graph T1->T2->T3->T1 Deadlock Cycle Detected: True
Graph T1->T2->T3    Deadlock Cycle Detected: False
Conflict 1: Older T(ts=10) WOUNDS (aborts) Younger T(ts=25)
Conflict 2: Younger T(ts=30) WAITS for Older T(ts=15)
```

---

### Example 4: Write Skew Anomaly under Snapshot Isolation

Demonstrates how two concurrent transactions modifying disjoint rows can satisfy their local constraints under Snapshot Isolation yet collectively violate a global business invariant ($Account_1 + Account_2 \ge 0$).

```python
# Invariant: Account1 + Account2 >= 0
acc1 = 100
acc2 = 100

# Transaction 1: Reads snapshot (Acc1=100, Acc2=100), withdraws 150 from Acc1
snap_sum_T1 = acc1 + acc2
t1_withdrawal = 150
if snap_sum_T1 >= t1_withdrawal:
    new_acc1 = acc1 - t1_withdrawal  # -$50

# Transaction 2 (Concurrent): Reads snapshot (Acc1=100, Acc2=100), withdraws 150 from Acc2
snap_sum_T2 = acc1 + acc2
t2_withdrawal = 150
if snap_sum_T2 >= t2_withdrawal:
    new_acc2 = acc2 - t2_withdrawal  # -$50

# Under Snapshot Isolation: Both write disjoint rows, so both commit without conflict!
final_acc1 = new_acc1
final_acc2 = new_acc2
final_total = final_acc1 + final_acc2

print("=== Write Skew Anomaly under Snapshot Isolation ===")
print(f"Initial: Acc1=${acc1}, Acc2=${acc2} (Total: ${acc1+acc2})")
print(f"T1 commits Acc1=${final_acc1} | T2 commits Acc2=${final_acc2}")
print(f"Final Total Balance: ${final_total} -> Invariant Violation (Write Skew): {final_total < 0}")
```

#### Execution Output
```text
=== Write Skew Anomaly under Snapshot Isolation ===
Initial: Acc1=$100, Acc2=$100 (Total: $200)
T1 commits Acc1=$-50 | T2 commits Acc2=$-50
Final Total Balance: $-100 -> Invariant Violation (Write Skew): True
```

---

## Design Patterns & Best Practices
1.  **Bypass Kernel Caching:** Databases need granular control over eviction and prefetching. Pattern: Direct I/O (`O_DIRECT`) + custom buffer pool.
2.  **Separate Logical vs Physical Logging:** Use logical for undo (smaller footprints, avoids page-level locking) and physical for redo (idempotent, fast recovery).
3.  **Latch Crabbing + B-Link Trees:** Design pattern for concurrent tree structures. Eliminate root-to-leaf lock contention by using sibling pointers and optimistic traversal.
4.  **Readers-Writer Locks:** Granular latching pattern. Allow parallel reads but exclusive writes, upgrading locks lazily.

---

## Trade-offs & Decision Framework
*   **Steal vs. No-Steal:** Steal allows massive transactions that exceed RAM capacity but requires complex UNDO logging. No-Steal simplifies recovery but limits transaction size to available RAM.
*   **Force vs. No-Force:** Force guarantees durability instantly but blocks the application on slow disk seeks. No-Force requires REDO logging but enables high-throughput batching.
*   **OCC vs. Pessimistic Locking:** Use OCC when reads vastly outnumber writes and conflicts are extremely rare. Use Pessimistic (2PL) for high-contention, write-heavy environments to avoid thrashing CPU on transaction retries.

---

## Interview & System Design Angles
*   **"How does a database recover from a crash?"** -> Explain ARIES: Steal/No-Force, the 3 phases (Analysis, Redo history, Undo losers).
*   **"How do we prevent lost updates?"** -> Discuss Snapshot Isolation and "first committer wins" rule, or standard Repeatable Read with row locks.
*   **"What is the difference between a Lock and a Latch?"** -> Locks protect logical transactions (ACID); Latches protect physical data structures (multithreading memory consistency).
*   **"Explain Write Skew"** -> Highly relevant. Explain the bank account sum example. Mention that Snapshot Isolation allows it, requiring true Serializable isolation or explicit `SELECT FOR UPDATE` to prevent.

---

## AI/ML Engineering Relevance
*   **Feature Stores & ML Databases:** Systems like Feast or Hopsworks heavily rely on MVCC. During model training, you need point-in-time correctness (Snapshot Isolation). MVCC allows massive read-throughput for model inference/training without blocking concurrent feature ingestion.
*   **Vector Databases & HNSW:** Concurrent writes to Hierarchical Navigable Small World (HNSW) graphs in vector DBs (Milvus, Qdrant) use highly specialized lock-free algorithms and latch crabbing to prevent index corruption during massive vector embeddings ingestion.
*   **Distributed Training & Checkpoints:** The concepts of "fuzzy checkpoints" and Write-Ahead Logging translate perfectly to distributed ML training faults. Model state is persisted asynchronously (fuzzy checkpoints), and operations are logged so nodes can recover without pausing global training.

---

## Key Takeaways
*   **ACID requires coordination:** Transactions are an illusion achieved via rigorous logging and locking.
*   **Page Caches bridge speed gaps:** Custom buffer management using LFU/CLOCK outperforms standard OS page caching.
*   **WAL is the ultimate source of truth:** Unflushed data isn't safe until it hits the WAL.
*   **ARIES is the gold standard:** Utilizing Steal/No-Force and physiological logging enables blazing fast operations and safe recovery.
*   **Isolation is a spectrum:** Performance trades off against anomalies. Understand Dirty Reads, Phantom Reads, and Write Skew.
*   **Concurrency relies on MVCC and Latches:** B-Link Trees and MVCC are what allow modern databases to handle thousands of concurrent queries without locking the whole system.

## Glossary
*   **Page Cache:** In-memory buffer for disk blocks.
*   **WAL:** Write-Ahead Log, an append-only file for operations.
*   **LSN:** Log Sequence Number, unique ID for log entries.
*   **Dirty Page:** A cached page that has been modified but not synced to disk.
*   **Deadlock:** Two transactions endlessly waiting for each other’s locks.
*   **Phantom Read:** Range query anomaly where rows appear/disappear.
*   **Write Skew:** Interleaving anomaly where combined updates violate invariant despite isolated correctness.
*   **Latch:** A lightweight thread synchronization mechanism for physical structures.
*   **Lock:** A database construct for logical data isolation.
