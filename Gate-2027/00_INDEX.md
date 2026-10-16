# GATE CS 2027 — Question Bank Index

> **Companion notes:** high-yield concept notes (with their own verified practice sets) live in `00_HIGH_YIELD_NOTES_INDEX.md` and `<Subject>/Notes/`.

**880 questions across 88 files**, covering the full GATE 2027 Computer Science syllabus. Every file has 10 questions (MCQ/MSQ/NAT mix, difficulty ramping from fundamental to a killer conceptual question) followed by an answer key. All numeric answers were verified by running code (simulations, exact computation, or symbolic math), not worked by hand.

---

## Operating Systems (10 files)
| File | Topic |
|---|---|
| OS_01 | Processes, Threads & System Calls |
| OS_02 | CPU Scheduling |
| OS_03 | Semaphores & Synchronization |
| OS_04 | Deadlocks |
| OS_05 | Memory Management & Paging (Address Translation) |
| OS_06 | Page Tables (single-level, multi-level, inverted) |
| OS_07 | TLB & Effective Memory Access Time |
| OS_08 | Page Replacement |
| OS_09 | File Systems (allocation, inodes, free space, links) |
| OS_10 | Disk & I/O Scheduling |

## Discrete Mathematics (10 files)
| File | Topic |
|---|---|
| DM_01 | Propositional Logic |
| DM_02 | First-Order Logic |
| DM_03 | Sets, Relations & Functions |
| DM_04 | Partial Orders & Lattices |
| DM_05 | Monoids & Groups |
| DM_06 | Graphs: Connectivity, Trees & Degree Sequences |
| DM_07 | Graphs: Matching |
| DM_08 | Graphs: Colouring |
| DM_09 | Combinatorics: Counting |
| DM_10 | Recurrence Relations & Generating Functions |

## DBMS (11 files)
| File | Topic |
|---|---|
| DB_01 | ER Model |
| DB_02 | Relational Model, Keys & Integrity Constraints |
| DB_03 | Relational Algebra |
| DB_04 | Tuple Relational Calculus |
| DB_05 | SQL I: Joins, Aggregation, GROUP BY / HAVING, NULLs |
| DB_06 | SQL II: Subqueries, Set Operations, Views |
| DB_07 | Functional Dependencies & Normal Forms |
| DB_08 | File Organization & Indexing |
| DB_09 | B and B+ Trees |
| DB_10 | Transactions: Schedules, Serializability & Recoverability |
| DB_11 | Concurrency Control (2PL, Timestamp Ordering, Deadlocks, Isolation) |

## Computer Networks (11 files)
| File | Topic |
|---|---|
| CN_01 | Delays & Performance Metrics |
| CN_02 | Error Detection & Framing (CRC, Checksum, Bit Stuffing) |
| CN_03 | Layering & Switching |
| CN_04 | Medium Access Control & Ethernet |
| CN_05 | Routing: Distance Vector & Link State |
| CN_06 | IPv4 Addressing, Subnetting & CIDR |
| CN_07 | IPv4 Fragmentation & NAT |
| CN_08 | TCP: Flow Control & Reliable Delivery |
| CN_09 | TCP: Congestion Control |
| CN_10 | Application Layer: DNS, HTTP & Sockets |
| CN_11 | **Mixed Advanced Numerical** — multi-topic, GATE-style problems combining 2-3 CN sub-areas per question |

## Data Structures & Algorithms (13 files)
| File | Topic |
|---|---|
| DSA_01 | Arrays, Stacks & Queues |
| DSA_02 | Linked Lists |
| DSA_03 | Trees & Binary Search Trees |
| DSA_04 | Binary Heaps & Priority Queues |
| DSA_05 | Hashing |
| DSA_06 | Sorting |
| DSA_07 | Searching & Asymptotic Complexity |
| DSA_08 | Recursion |
| DSA_09 | Greedy Algorithms |
| DSA_10 | Dynamic Programming |
| DSA_11 | Divide and Conquer |
| DSA_12 | Graph Traversal, Topological Sort & Strongly Connected Components |
| DSA_13 | Minimum Spanning Trees & Shortest Paths |

## Engineering Mathematics (8 files)
*(Linear Algebra, Calculus, Probability & Statistics — Discrete Mathematics is covered separately above)*

| File | Topic |
|---|---|
| EM_01 | Matrices, Determinants & Systems of Linear Equations |
| EM_02 | Eigenvalues, Eigenvectors & LU Decomposition |
| EM_03 | Limits, Continuity & Differentiability |
| EM_04 | Maxima, Minima & Mean Value Theorem |
| EM_05 | Integration |
| EM_06 | Probability Basics, Conditional Probability & Bayes' Theorem |
| EM_07 | Random Variables & Standard Distributions |
| EM_08 | Mean, Median, Mode & Standard Deviation |

## Computer Architecture (6 files)
| File | Topic |
|---|---|
| COA_01 | Instruction Set & Addressing Modes |
| COA_02 | ALU Design (Adders, Multiplication, Booth's Algorithm) |
| COA_03 | Control Unit Design (Hardwired & Microprogrammed) |
| COA_04 | Memory Hierarchy & Cache Mapping |
| COA_05 | I/O Interface (Interrupts & DMA) |
| COA_06 | Instruction Pipelining & Pipeline Hazards |

## Theory of Computation (6 files)
| File | Topic |
|---|---|
| TOC_01 | Regular Expressions & Finite Automata |
| TOC_02 | Regular Languages: Minimal DFAs & Myhill–Nerode |
| TOC_03 | Context-Free Grammars & Pushdown Automata |
| TOC_04 | Context-Free Languages: Closure, Pumping Lemma & CYK |
| TOC_05 | Turing Machines |
| TOC_06 | Undecidability & Reducibility |

## Digital Logic (6 files)
| File | Topic |
|---|---|
| DL_01 | Boolean Algebra & Minimization (K-Map, Tabular/Quine–McCluskey) |
| DL_02 | Number Representation & Fixed-Point Arithmetic |
| DL_03 | Floating-Point Representation (IEEE 754) |
| DL_04 | Combinational Circuit Design (MUX, Decoders, Adders, Comparators) |
| DL_05 | Sequential Circuits I: Flip-Flops, Latches & Counters |
| DL_06 | Sequential Circuits II: FSM Design, Mealy vs. Moore & State Minimization |

## Compiler Design (7 files)
| File | Topic |
|---|---|
| CD_01 | Lexical Analysis |
| CD_02 | Top-Down Parsing (FIRST/FOLLOW, LL(1)) |
| CD_03 | Bottom-Up Parsing (LR(0), SLR(1)) |
| CD_04 | Syntax-Directed Translation |
| CD_05 | Runtime Environments |
| CD_06 | Intermediate Code Generation |
| CD_07 | Local Optimization & Data Flow Analysis |

---

## Known caveats
- The OS and Discrete Mathematics files (the first two subjects built) got lighter computational verification than everything from DBMS onward — flag anything that looks off in those and it'll get rechecked.
- CN_11 is the only "mixed/advanced" multi-topic set built so far. The same treatment can be built for any other subject on request.
