# Chapter 11: Replication and Consistency
> A deep dive into how distributed databases maintain data consistency across multiple replicas while managing the trade-offs between availability, partition tolerance, and synchronization overhead.

## Table of Contents
- [Chapter Overview](#chapter-overview)
- [Achieving Availability & The CAP Theorem](#achieving-availability--the-cap-theorem)
  - [CAP and PACELC](#cap-and-pacelc)
  - [Harvest and Yield](#harvest-and-yield)
- [Shared Memory & Register Semantics](#shared-memory--register-semantics)
- [Consistency Models (Single-Operation)](#consistency-models-single-operation)
  - [Strict Consistency](#strict-consistency)
  - [Linearizability](#linearizability)
  - [Sequential Consistency](#sequential-consistency)
  - [Causal Consistency](#causal-consistency)
- [Session Models (Client-Centric Consistency)](#session-models-client-centric-consistency)
- [Eventual & Tunable Consistency](#eventual--tunable-consistency)
  - [Quorums (N, W, R)](#quorums-n-w-r)
  - [Witness Replicas](#witness-replicas)
- [Strong Eventual Consistency & CRDTs](#strong-eventual-consistency--crdts)
- [Worked Examples & Code Implementations](#worked-examples--code-implementations)
- [Design Patterns & Best Practices](#design-patterns--best-practices)
- [Trade-offs & Decision Framework](#trade-offs--decision-framework)
- [Interview & System Design Angles](#interview--system-design-angles)
- [AI/ML Engineering Relevance](#aiml-engineering-relevance)
- [Key Takeaways](#key-takeaways)
- [Glossary](#glossary)

## Chapter Overview
Data replication is the fundamental mechanism for introducing redundancy in distributed databases. By maintaining multiple copies of data across different nodes or geographical regions, a system removes single points of failure, improves fault tolerance, and reduces read latencies for geographically distributed clients. However, atomically updating multiple copies of data is equivalent to solving the distributed consensus problem, which introduces significant latency and overhead.

This chapter explores the semantics, guarantees, and trade-offs required to make replicated data look consistent to the user. We dive deep into theoretical constraints like the CAP theorem, explore the spectrum of consistency models (from linearizability to eventual consistency), examine client-centric session models, and analyze techniques like quorums, vector clocks, and CRDTs to achieve tunable and strong eventual consistency.

## Achieving Availability & The CAP Theorem

To survive intermittent node crashes or network partitions without impacting user-facing availability, databases replicate data. But as soon as we replicate, we face the challenge of keeping the copies in sync.

### CAP and PACELC
The CAP theorem (formulated by Eric Brewer) dictates the fundamental trade-offs in distributed systems when network partitions occur:
- **Consistency (C):** Linearizable reads. Every read receives the most recent write or an error.
- **Availability (A):** Every request receives a (non-error) response, without the guarantee that it contains the most recent write.
- **Partition Tolerance (P):** The system continues to operate despite an arbitrary number of messages being dropped or delayed by the network between nodes.

In the presence of a network partition, a system must choose between **Consistency (CP)** or **Availability (AP)**.
*   **CP Systems:** Prefer failing requests to serving potentially inconsistent or stale data (e.g., Paxos/Raft based systems).
*   **AP Systems:** Prefer serving available (but potentially stale) data rather than failing the request (e.g., Dynamo-style leaderless replication).

> [!NOTE] 
> **CAP Consistency vs. ACID Consistency**
> In CAP, consistency means operations are atomic and linearizable across replicas. In ACID (Chapter 5), consistency refers to maintaining database invariants (constraints, foreign keys) when transitioning from one valid state to another.

**PACELC Extension**
Formulated by Daniel Abadi, PACELC extends CAP to address system behavior when running normally (without partitions):
*   **PAC:** If there is a Partition, trade-off between Availability and Consistency.
*   **ELC:** Else (when running normally), trade-off between Latency and Consistency.

### Harvest and Yield
Instead of strict all-or-nothing availability, systems can optimize for degraded modes using two tunable metrics (introduced by Fox and Brewer):
*   **Harvest:** Completeness of the query result. (e.g., returning 99 out of 100 rows if one partition is down is better than an outright error).
*   **Yield:** The ratio of successfully completed requests to total attempted requests. 
We can explicitly trade harvest for yield by returning partial data from available nodes, thereby maintaining high uptime and graceful degradation.

## Shared Memory & Register Semantics

Distributed databases simulate a "shared memory" architecture where internode communication is abstracted. We view shared memory as an array of registers. Depending on how a register behaves when read and write operations overlap concurrently, we distinguish three types:

1.  **Safe Register:** If a read overlaps with a concurrent write, it can return an *arbitrary* value within the register's range. (Binary safe registers may appear to "flicker" between states).
2.  **Regular Register:** Provides stronger guarantees. A read overlapping with a write returns either the value from the most recent completed write, or the value currently being written.
3.  **Atomic Register:** Guarantees linearizability. There is a single, instantaneous moment before which reads return the old value, and after which reads return the new value.

## Consistency Models (Single-Operation)

Consistency models are a contract between the database and the developer, limiting the "possible histories" of interleaving concurrent operations. 

### Strict Consistency
The theoretical ideal: Complete replication transparency. A write by any process is instantly visible to all subsequent reads by any process, anywhere in the cluster.
*   Requires a perfect global clock.
*   Impossible to implement due to the speed of light and physical limits of network transmission.

### Linearizability
The strongest practical single-object consistency model.
*   **Core Concept:** Operations appear to execute atomically and instantaneously exactly once between their invocation and completion times. 
*   **Linearization Point:** The exact moment an operation takes effect. Before this point, old values are seen. After this point, all clients universally see the new value.
*   **Cost:** Synchronization is incredibly expensive. It requires consensus algorithms (like Paxos/Raft) or atomic compare-and-swap (CAS) CPU primitives, triggering cross-node CPU traffic and cache invalidations.

**RIFL (Reusable Infrastructure for Linearizability)**
Implements linearizable RPCs by uniquely identifying messages using client IDs and monotonic sequence numbers. If a client crashes and retries a write, completion objects on durable storage prevent duplicate execution, enforcing exact-once linearizable semantics independently of underlying storage.

### Sequential Consistency
Relaxes the requirement of global real-time ordering.
*   **Core Concept:** Operations of each *individual process* are executed in the program order specified by that process. Overall, operations appear in some valid sequential order, and *all nodes must observe operations in that exact same order*.
*   **Trade-off:** Nodes might be arbitrarily stale compared to the real-time wall clock, but the *sequence* of state changes is identical everywhere.
*   *Note:* Sequential consistency is not composable across multiple objects, whereas linearizability is.

### Causal Consistency
Relaxes global total order to a partial order based on cause and effect.
*   **Core Concept:** Concurrent writes with no causal relationship can be observed in different orders by different nodes. However, causally related writes (e.g., a write generated as a reaction to reading a previous value) must be seen in the same order by all nodes.
*   **Vector Clocks:** Used heavily in Dynamo and Riak to track causal histories. Each node maintains a vector (array) of logical clocks (`[Node1_clock, Node2_clock, ...]`). On a write, the node increments its clock and attaches the vector. When merging, vectors help detect if events are causally ordered or concurrent/divergent.

> [!WARNING]
> Implementing causal consistency requires storing causal history (vector clocks), garbage collection for these clocks, and mechanisms for users to resolve conflicts when divergent histories (concurrent writes) are detected. Systems like Cassandra often skip this in favor of Last-Write-Wins (LWW) to avoid the overhead.

## Session Models (Client-Centric Consistency)

Instead of looking globally at all clients, session models look at consistency from the perspective of a single client session.

*   **Read-Own-Writes (Read-Your-Writes):** A client will always see the effects of its own recent writes. 
*   **Monotonic Reads:** If a client reads value $V$, any subsequent read by that client will return $V$ or a newer value. It will never read a value older than $V$.
*   **Monotonic Writes:** Writes by the same client are processed by the system in the exact order the client submitted them.
*   **Writes-Follow-Reads (Session Causality):** If a client reads $V_1$ and then writes $V_2$, the system guarantees $V_2$ is ordered logically after $V_1$.
*   **PRAM (Pipelined RAM) / FIFO Consistency:** The combination of Monotonic Reads, Monotonic Writes, and Read-Own-Writes. Writes from a single process propagate in order, but writes from different processes can be observed in different orders.

## Eventual & Tunable Consistency

In eventually consistent systems (like DynamoDB, Cassandra, Riak), updates propagate asynchronously. If no new updates are made, all replicas will *eventually* converge to the latest value.

### Quorums (N, W, R)
To provide flexibility, Dynamo-style databases offer tunable consistency using three parameters:
*   **N (Replication Factor):** Total replicas storing the data.
*   **W (Write Consistency):** Replicas that must acknowledge a write for it to be deemed successful.
*   **R (Read Consistency):** Replicas that must respond to a read.

**Strict Quorum (Strong Consistency):** 
If **$R + W > N$**, the read and write sets overlap. At least one replica in the read quorum is guaranteed to have the latest write.
*   *Example:* $N=3, W=2, R=2$. Tolerates 1 node failure.
*   *Write-heavy:* $W=1, R=N$. Fast writes, slow reads (requires all nodes up).
*   *Read-heavy:* $W=N, R=1$. Slow writes, fast reads.

> [!TIP]
> **Read Repair:** Quorums don't automatically guarantee monotonicity if a write fails midway. To ensure consistency, systems use blocking Read Repair during the read path to detect stale replicas in the quorum and asynchronously (or synchronously) update them.

### Witness Replicas
Quorums guarantee consistency but require high storage costs (e.g., $N=5$ means 5 full copies).
*   **Witness Replicas:** Nodes that participate in quorums but do not store the actual data. They merely store metadata/logs indicating a write occurred.
*   If a data replica goes down, a witness can be temporarily promoted to store data.
*   Any majority quorum containing at least one *data* replica can safely serve reads.
*   Used in systems like Google Spanner to reduce storage costs while maintaining high availability and quorum intersections.

## Strong Eventual Consistency & CRDTs

**Strong Eventual Consistency (SEC)** goes a step further than eventual consistency. Replicas can process updates out of order, but mathematical properties guarantee that once all updates arrive, they will automatically merge into the exact same valid state without user intervention.

This is implemented via **Conflict-Free Replicated Data Types (CRDTs)**:
*   **CmRDTs (Commutative Replicated Data Types):** Operation-based. Operations must be side-effect free, commutative ($X \cdot Y = Y \cdot X$), and causally ordered.
*   **Types of CRDTs:**
    1.  **G-Counter (Grow-only Counter):** Each node has a slot in a vector. It only increments its own slot. Merge function takes the `max()` of each slot. Total sum is the sum of the vector.
    2.  **PN-Counter (Positive-Negative Counter):** Combines two G-Counters (one for increments, one for decrements). 
    3.  **LWW Register (Last-Write-Wins):** Uses unique, globally ordered timestamps. Merge trivially picks the higher timestamp.
    4.  **G-Set (Grow-only Set):** Appending elements produces valid sets; union is commutative.
    5.  **Replicated JSON:** Complex CRDTs handling insertions, deletions, and nested maps/lists without specific ordering requirements.

---

## Worked Examples & Code Implementations

### Example 1: State-Based CRDTs (G-Counter & PN-Counter)

Demonstrates state-based Conflict-Free Replicated Data Types (CRDTs). Multiple nodes perform concurrent increments and decrements in arbitrary orders, gossiping their state vectors and converging to the exact same total without central coordination.

```python
class GCounter:
    def __init__(self, node_id: str, cluster: list[str]):
        self.node_id = node_id
        self.slots = {n: 0 for n in cluster}

    def inc(self, value: int = 1):
        self.slots[self.node_id] += value

    def value(self) -> int:
        return sum(self.slots.values())

    def merge(self, other: "GCounter"):
        for n, count in other.slots.items():
            self.slots[n] = max(self.slots.get(n, 0), count)

class PNCounter:
    def __init__(self, node_id: str, cluster: list[str]):
        self.node_id = node_id
        self.p_counter = GCounter(node_id, cluster)  # Increments
        self.n_counter = GCounter(node_id, cluster)  # Decrements

    def inc(self, value: int = 1):
        self.p_counter.inc(value)

    def dec(self, value: int = 1):
        self.n_counter.inc(value)

    def value(self) -> int:
        return self.p_counter.value() - self.n_counter.value()

    def merge(self, other: "PNCounter"):
        self.p_counter.merge(other.p_counter)
        self.n_counter.merge(other.n_counter)

cluster = ["Node_A", "Node_B", "Node_C"]
c_A = PNCounter("Node_A", cluster)
c_B = PNCounter("Node_B", cluster)
c_C = PNCounter("Node_C", cluster)

# Independent concurrent operations
c_A.inc(10)
c_B.inc(5)
c_B.dec(2)
c_C.dec(1)

print("=== State-Based CRDT (PN-Counter) ===")
print(f"Before Merge -> Node A: {c_A.value()}, Node B: {c_B.value()}, Node C: {c_C.value()}")

# State propagation / Pairwise merges
c_A.merge(c_B)
c_A.merge(c_C)
c_B.merge(c_A)
c_C.merge(c_A)

print(f"After Convergence -> Node A: {c_A.value()}, Node B: {c_B.value()}, Node C: {c_C.value()} (All Replicas Converged to 12!)")
```

#### Execution Output
```text
=== State-Based CRDT (PN-Counter) ===
Before Merge -> Node A: 10, Node B: 3, Node C: -1
After Convergence -> Node A: 12, Node B: 12, Node C: 12 (All Replicas Converged to 12!)
```

---

### Example 2: Tunable Quorum Simulator ($N, W, R$) & Background Read Repair

Demonstrates the difference between non-strict quorums ($W=1, R=1, W+R \le N$, leading to stale reads) and strict quorums ($W=2, R=2, W+R > N$, ensuring read-write overlap), accompanied by automatic background read repair of lagging replicas.

```python
class Replica:
    def __init__(self, node_id: str):
        self.node_id = node_id
        self.data = {}  # key -> (value, timestamp)

class DynamoCluster:
    def __init__(self):
        self.nodes = [Replica("R1"), Replica("R2"), Replica("R3")]

    def write(self, key: str, val: str, ts: int, w_quorum: int) -> list[str]:
        written_nodes = []
        for r in self.nodes[:w_quorum]:
            r.data[key] = (val, ts)
            written_nodes.append(r.node_id)
        return written_nodes

    def read(self, key: str, r_nodes_indices: list[int]) -> tuple[str, list[str], list[str]]:
        responses = []
        sampled_replicas = [self.nodes[i] for i in r_nodes_indices]
        for r in sampled_replicas:
            entry = r.data.get(key, (None, 0))
            responses.append((r, entry[0], entry[1]))

        # Pick newest value by timestamp
        highest = max(responses, key=lambda x: x[2])
        best_val, best_ts = highest[1], highest[2]

        # Read Repair: identify and update stale replicas
        repaired = []
        for r, val, ts in responses:
            if ts < best_ts:
                r.data[key] = (best_val, best_ts)
                repaired.append(r.node_id)

        return best_val, [r.node_id for r in sampled_replicas], repaired

db = DynamoCluster()

print("=== Tunable Quorum (N=3) & Read Repair ===")

# Scenario 1: Weak Quorum (W=1, R=1, W+R=2 <= N=3)
db.write("user:profile", "Alice v1 (2026)", ts=100, w_quorum=1)
val_stale, sampled, _ = db.read("user:profile", r_nodes_indices=[2])
print(f"Weak Quorum (W=1, R=1): Read from {sampled} -> Result: '{val_stale}' (STALE / NOT FOUND anomaly!)")

# Scenario 2: Strict Quorum (W=2, R=2, W+R=4 > N=3)
db.write("user:profile", "Alice v2 (Updated)", ts=200, w_quorum=2)  # Writes to R1, R2
val_fresh, sampled, repaired = db.read("user:profile", r_nodes_indices=[1, 2])
print(f"Strict Quorum (W=2, R=2): Read from {sampled} -> Result: '{val_fresh}' (Guaranteed Freshness!)")
print(f"  => Background Read Repair asynchronously updated lagging replicas: {repaired}")
print(f"  => R3 State After Read Repair: {db.nodes[2].data['user:profile']}")
```

#### Execution Output
```text
=== Tunable Quorum (N=3) & Read Repair ===
Weak Quorum (W=1, R=1): Read from ['R3'] -> Result: 'None' (STALE / NOT FOUND anomaly!)
Strict Quorum (W=2, R=2): Read from ['R2', 'R3'] -> Result: 'Alice v2 (Updated)' (Guaranteed Freshness!)
  => Background Read Repair asynchronously updated lagging replicas: ['R3']
  => R3 State After Read Repair: ('Alice v2 (Updated)', 200)
```

---

## Design Patterns & Best Practices

1. **Explicit Degradation (Harvest vs. Yield):** Design systems to gracefully degrade. Return partial results rather than failing a request completely during partitions.
2. **Read Repair for Quorums:** In Dynamo-style stores, implement read-repair to continuously fix replica divergence on the read path.
3. **Use Vector Clocks for Causal Dependencies:** When the application semantics strictly prohibit losing concurrent updates (e.g., a shopping cart), rely on vector clocks and expose the conflict to the client for domain-specific resolution.
4. **Leverage CRDTs for Leaderless Topologies:** If operations are inherently commutative (like page views counters or "likes"), use CRDTs to achieve partition tolerance and zero-coordination writes with mathematically guaranteed convergence.

## Trade-offs & Decision Framework

| Consistency Model | Latency | Availability | Trade-offs & Best Use Case |
| :--- | :--- | :--- | :--- |
| **Linearizable** | Highest | Lowest (CP) | Best for financial transactions, coordination systems (Zookeeper). High synchronization overhead. |
| **Sequential** | High | Low | Good when strict real-time isn't needed but global order is critical. |
| **Causal** | Medium | Medium | Good for social media feeds, forums. Requires vector clock overhead and garbage collection. |
| **Eventual (Tunable)** | Lowest | Highest (AP) | Best for highly scalable, write-heavy systems (metrics, logging, user preferences). Allows $R+W>N$ tuning. |
| **Strong Eventual (CRDT)** | Lowest | Highest | Perfect for distributed counters, sets, collaborative editing where operations commute. |

## Interview & System Design Angles

**1. "Design a Highly Available System (e.g., Shopping Cart, metrics tracker)"**
*   *Angle:* Discuss the CAP theorem. Mention that you'll build an AP system using Dynamo-style replication. Use $W=1, R=2$ for fast cart additions, but mention vector clocks to handle concurrent edits, or CRDT sets (OR-Set) to ensure no cart items are lost during network partitions.

**2. "How do you handle read consistency in a leaderless database?"**
*   *Angle:* Explain Quorum Math ($R+W>N$). Mention that a quorum read fetches from multiple nodes, compares timestamps or vector clocks, returns the newest value to the client, and issues an asynchronous *read-repair* to the stale nodes.

**3. "When would you choose CP over AP?"**
*   *Angle:* Mention pacing invariants (e.g., preventing double-spending). Explain that you need consensus protocols (Raft) yielding Linearizability, which inherently sacrifices availability during network partitions.

## AI/ML Engineering Relevance

*   **Distributed Training & Parameter Servers:** Parameter servers often relax strict consistency (similar to PRAM or Eventual Consistency) to maximize throughput. Asynchronous SGD (Stochastic Gradient Descent) operates on stale weights deliberately trading consistency (exact gradients) for yield and latency, which mathematically still converges in many non-convex optimizations.
*   **Feature Stores:** Feature stores typically prioritize high-availability reads (AP). Stale features (by a few seconds) in an eventually consistent cluster are completely acceptable for most inference workloads, whereas write latency must be low.
*   **Vector Databases & ANN Indexes:** Distributed ANN index building is extremely expensive. Most vector DBs replicate data asynchronously and offer tunable consistency. A leaderless approach ensures high ingest rates for embeddings, with eventual consistency for the search index graph updates. 

## Key Takeaways
- **Availability is not absolute:** CAP proves you cannot have linearizability and availability during partitions. PACELC expands this to normal operations (latency vs consistency).
- **Linearizability requires consensus:** It provides a single real-time sequence of atomic operations but introduces heavy synchronization costs.
- **Client-Centric consistency solves UX problems:** Session models like Read-Your-Writes provide intuitive behavior for users without paying the global synchronization cost of linearizability.
- **Quorums offer tunable consistency:** By tweaking $R$ and $W$, developers can slide the scale between performance and consistency on a per-query basis.
- **CRDTs enable conflict-free distribution:** By designing operations to be mathematically commutative and using specialized data types, systems can achieve Strong Eventual Consistency without coordination.

## Glossary
- **Linearization Point:** The exact instantaneous moment when an operation takes effect and becomes visible to all subsequent operations.
- **Vector Clock:** An array of logical clocks used to determine partial ordering and detect causality/conflicts among events in distributed systems.
- **Quorum:** A majority (or configured threshold) of nodes that must participate in a read/write to guarantee overlap.
- **Harvest:** The completeness of a query response in degraded conditions.
- **Yield:** The ratio of successfully served requests compared to total attempts.
- **Witness Replica:** A replica that participates in quorums and stores operation logs/metadata but does not hold a full copy of the data, saving storage costs.
- **CRDT:** Conflict-Free Replicated Data Type. Data structures that automatically merge diverged states without conflicts.
