# Raft Consensus: A Real 5-Node Cluster, Election, Partition, and Crash Recovery

### Leader election, log replication, and the safety property that makes it all work — demonstrated with 5 real OS processes, a real simulated network partition, and a real `kill -9` + disk recovery

---

## What this is

Every RPC document in this series covered *calling* a service reliably. This is the layer underneath that: how do multiple *replicas* of a service agree on one consistent view of the world, even when machines crash and the network partitions? **Raft** (Ongaro & Ousterhout, 2014 — explicitly designed to be more understandable than Paxos while providing the same guarantees) is the answer nearly every modern distributed data store uses. This isn't a description — it's 5 real Python processes, talking real TCP/JSON RPCs, actually elected, actually partitioned, actually crashed and recovered.

## The real run, in six phases

**1. Election.** 5 nodes start as followers with randomized election timeouts (500–1000ms). N1's timer fires first:
```
04:45:49.246 [N1] ELECTION TIMEOUT -> CANDIDATE, term 1
04:45:49.251 [N1] vote GRANTED by N4 (2/5 votes, need 3)
04:45:49.252 [N1] vote GRANTED by N3 (3/5 votes, need 3)
04:45:49.252 [N1] *** became LEADER for term 1 *** (appended no-op at idx 1)
```

**2. Client writes.** Every write goes through the leader, gets appended to its log, then replicated:
```
client SET x=1 appended at idx 2 (term 1), waiting for majority
COMMIT idx=2 (3/5 replicas)
APPLY idx=2 x=1
```

**3. A real partition.** N1+N2 isolated from N3+N4+N5. The **old leader N1 still believes it's leader** and accepts a write — which blocks forever, because it can't reach a majority:
```
!!! PARTITION: dropping all traffic with [3, 4, 5]
client SET z=stuck appended at idx 5 (term 1), waiting for majority
  -> [times out after 3s: "timeout_not_committed"]
```
Meanwhile the majority side elects its **own** leader, N3, in **term 2** — strictly higher than N1's term 1. This is the whole safety property in one sentence: **two nodes can simultaneously believe they're leader, but only the one whose term the majority actually recognizes can ever commit anything.**

**4. Healing.** The instant N1 can reach the real cluster again:
```
-> FOLLOWER (term 2): N4 replied with higher term 2 (I was a stale leader)
ELECTION TIMEOUT -> CANDIDATE, term 3        <- N1 tries again, doesn't know term 4 exists yet
-> FOLLOWER (term 4): saw higher term 4 in RequestVote from N3
LOG CONFLICT at idx 5: truncating my entries [t1:z=stuck] to match leader N3
APPLY idx=6 z=committed_by_new_leader
APPLY idx=7 x=99
```
**N1's own uncommitted write (`z=stuck`) is silently discarded** — real, direct proof that data a "leader" accepted but never got a majority to confirm is not safe, and Raft correctly throws it away on rejoin rather than corrupting the log.

**5. Crash + recovery.** The current leader gets `kill -9`'d — no clean shutdown, no chance to flush anything new:
```
N3 BEFORE kill -9: state=leader term=4 log=[..., 't4:x=99']
N3 killed with SIGKILL
N3 AFTER restart:  state=follower term=5 log=[..., 't4:x=99', 't5:noop']
```
Term and log survived the crash — recovered from the fsync'd JSON file on disk, exactly as Raft's persistence rule requires (§4 of the paper: `currentTerm`, `votedFor`, and the log must be durable *before* replying to any RPC).

**6. Convergence.** Every reachable node ends up agreeing on the identical committed state: `{'x': '99', 'y': '2', 'z': 'committed_by_new_leader'}`.

## Why this matters: the property being demonstrated

**Consensus isn't about preventing bad things from happening — machines *will* crash, networks *will* partition. It's about guaranteeing that once a majority agrees something happened, it's permanent, and anything that *didn't* reach a majority is safely discardable.** That's the entire content of phase 3–4 above, made concrete instead of asserted.

## What's implemented vs. honestly not

**Implemented**: leader election with randomized timeouts, the log-replication consistency check + conflict truncation, the majority-commit rule (with the "never commit older-term entries by counting alone" fix from the paper's Figure 8), the leader no-op entry, and full disk persistence.

**Not implemented, named honestly**: cluster membership changes, log compaction/snapshots, PreVote/CheckQuorum (production hardening against needless elections), and linearizable reads. Real systems — **etcd** (literally Raft, and the coordination backbone of every Kubernetes cluster), **CockroachDB** and **TiKV** (Raft per data shard), **Consul** — implement all of this plus these hardening features.

## Where this shows up

- **Every Kubernetes cluster is running Raft right now**, via etcd, to agree on cluster state.
- **CockroachDB/TiKV**: one Raft group per data shard — directly relevant to distributed-database interview questions.
- **Interview angle**: "why does the old leader's write hang instead of failing immediately?" (answer: it *doesn't know yet* that it's stale — this is exactly what phase 3 shows), "why elect a new term instead of just any new leader?" (monotonic terms are the entire mechanism that lets everyone unambiguously agree which leader is more recent).

---

## Quick recap

- A term is a logical clock; higher always wins, unconditionally — this is what let N1 (term 3, still clueless) immediately defer to N3 (term 4) the moment they could talk again.
- Majority-commit + "leader may hold, but discard, unreplicated entries" together mean data loss requires *losing a majority permanently*, not just one crash or one partition.
- Persistence must happen (fsync) **before** replying to any RPC — verified directly by killing the term-4 leader mid-flight and watching its exact term and log reappear after restart.
