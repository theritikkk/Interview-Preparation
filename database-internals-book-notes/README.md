# 📚 Database Internals — Comprehensive Study Notes

> Expert-level chapter-by-chapter notes from **"Database Internals" by Alex Petrov (O'Reilly, 2019)**
>
> Designed for engineers preparing for system design interviews, building database-backed systems, or working on AI/ML infrastructure.

---

## 🏗️ Book Overview

This book covers the fundamental concepts, algorithms, and data structures used in modern database systems. It is split into two parts:

- **Part I — Storage Engines**: How databases store and retrieve data on a single node
- **Part II — Distributed Systems**: How databases coordinate across multiple nodes

```mermaid
graph TB
    subgraph "Part I: Storage Engines"
        CH1[Ch 1: Introduction & Overview]
        CH2[Ch 2: B-Tree Basics]
        CH3[Ch 3: File Formats]
        CH4[Ch 4: Implementing B-Trees]
        CH5[Ch 5: Transaction Processing & Recovery]
        CH6[Ch 6: B-Tree Variants]
        CH7[Ch 7: Log-Structured Storage]
    end
    
    subgraph "Part II: Distributed Systems"
        CH8[Ch 8: Distributed Systems Overview]
        CH9[Ch 9: Failure Detection]
        CH10[Ch 10: Leader Election]
        CH11[Ch 11: Replication & Consistency]
        CH12[Ch 12: Anti-Entropy & Dissemination]
        CH13[Ch 13: Distributed Transactions]
        CH14[Ch 14: Consensus]
    end

    CH1 --> CH2 --> CH3 --> CH4
    CH4 --> CH5 --> CH6 --> CH7
    CH8 --> CH9 --> CH10
    CH10 --> CH11 --> CH12
    CH12 --> CH13 --> CH14
```

---

## 📖 Chapter Notes

### Part I — Storage Engines

| Chapter | Title | Key Concepts | Notes |
|---------|-------|-------------|-------|
| 1 | **Introduction & Overview** | DBMS architecture, memory vs disk, column vs row stores, data/index files | [📄 Notes](ch01-introduction-and-overview.md) |
| 2 | **B-Tree Basics** | BSTs → B-Trees, disk-based trees, splits, merges, fanout | [📄 Notes](ch02-btree-basics.md) |
| 3 | **File Formats** | Page structure, slotted pages, cell layout, checksums, encoding | [📄 Notes](ch03-file-formats.md) |
| 4 | **Implementing B-Trees** | Page headers, overflow pages, rebalancing, bulk loading, compression | [📄 Notes](ch04-implementing-btrees.md) |
| 5 | **Transaction Processing & Recovery** | Buffer mgmt, WAL, ARIES, MVCC, isolation levels, concurrency | [📄 Notes](ch05-transaction-processing-and-recovery.md) |
| 6 | **B-Tree Variants** | Copy-on-write, Lazy B-Trees, FD-Trees, Bw-Trees, cache-oblivious | [📄 Notes](ch06-btree-variants.md) |
| 7 | **Log-Structured Storage** | LSM Trees, SSTables, compaction, Bloom filters, RUM conjecture | [📄 Notes](ch07-log-structured-storage.md) |

### Part II — Distributed Systems

| Chapter | Title | Key Concepts | Notes |
|---------|-------|-------------|-------|
| 8 | **Distributed Systems Overview** | Fallacies, failure models, clocks, CAP/PACELC | [📄 Notes](ch08-distributed-systems-overview.md) |
| 9 | **Failure Detection** | Heartbeats, Phi-Accrual, SWIM, gossip detection | [📄 Notes](ch09-failure-detection.md) |
| 10 | **Leader Election** | Bully, Ring, ZAB, Raft election, fencing | [📄 Notes](ch10-leader-election.md) |
| 11 | **Replication & Consistency** | Consistency models, quorums, CRDTs, tunable consistency | [📄 Notes](ch11-replication-and-consistency.md) |
| 12 | **Anti-Entropy & Dissemination** | Read repair, Merkle trees, gossip protocols, Plumtree | [📄 Notes](ch12-anti-entropy-and-dissemination.md) |
| 13 | **Distributed Transactions** | 2PC, 3PC, Calvin, Spanner, Percolator | [📄 Notes](ch13-distributed-transactions.md) |
| 14 | **Consensus** | Paxos, Multi-Paxos, Raft, ZAB, EPaxos, Flexible Paxos | [📄 Notes](ch14-consensus.md) |

---

## 🎯 How to Use These Notes

### For Interview Preparation
1. **System Design Interviews**: Focus on chapters 1, 5, 7, 11, 13, 14
2. **Storage/Infrastructure Roles**: Focus on Part I (chapters 1-7)
3. **Distributed Systems Roles**: Focus on Part II (chapters 8-14)

### For AI/ML Engineers
Each chapter includes an **AI/ML Engineering Relevance** section covering:
- Vector databases and ANN indexing
- Feature stores and model serving
- Distributed training coordination
- Embedding storage and retrieval
- ML pipeline data management

### Suggested Reading Order

```
Beginner Path:     Ch1 → Ch2 → Ch7 → Ch8 → Ch11 → Ch14
Intermediate Path: Ch1 → Ch2 → Ch5 → Ch7 → Ch8 → Ch11 → Ch13 → Ch14
Deep Dive Path:    Ch1 → Ch2 → Ch3 → Ch4 → Ch5 → Ch6 → Ch7 →
                   Ch8 → Ch9 → Ch10 → Ch11 → Ch12 → Ch13 → Ch14
```

---

## 🗺️ Concept Map Across Chapters

| Concept | Chapters |
|---------|----------|
| **B-Trees** | 2, 3, 4, 6 |
| **LSM Trees** | 7 |
| **Transactions** | 5, 13 |
| **Concurrency** | 5, 6 |
| **Replication** | 11, 12 |
| **Consensus** | 10, 14 |
| **Failure Handling** | 8, 9 |
| **Consistency Models** | 8, 11 |
| **Recovery** | 5, 13 |
| **Gossip Protocols** | 9, 12 |

---

## 📚 What Each Notes File Contains

Every chapter note includes:
1. **Chapter Overview** — goals and context
2. **Core Concepts** — deep explanations with real-world parallels
3. **Data Structures & Algorithms** — pseudocode, complexity analysis
4. **Architecture Diagrams** — Mermaid diagrams for systems, data flows, state machines
5. **Design Patterns** — patterns used in production databases
6. **Worked Examples** — step-by-step walkthroughs
7. **Trade-off Analysis** — when to use what and why
8. **Interview & System Design Angles** — FAANG interview applications
9. **AI/ML Relevance** — vector DBs, feature stores, distributed training
10. **Key Takeaways & Glossary**

---

> *"Database Internals" by Alex Petrov, O'Reilly Media, 2019. ISBN: 978-1-492-04034-7*
