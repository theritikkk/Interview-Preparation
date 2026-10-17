# Raft-backed Distributed KV Store

A replicated key/value store built on the from-scratch Raft implementation
from Project 1, extended into something you can actually use as a
database: `SET`, `GET`, `DEL` and compare-and-swap (`CAS`), a reusable
client library, and — the main new result — **read-index linearizable
reads**, with an experiment that shows, by running it, why they matter.

One OS process per node, plain TCP, C++17, no dependencies beyond the
standard library and POSIX sockets. See [ANALYSIS.md](ANALYSIS.md) for the
design decisions, exactly what is real versus simulated, and the bugs
found along the way.

## What's new over Project 1

| | Project 1 (toy Raft) | This project |
|---|---|---|
| Commands | `SET`, `GET` | `SET`, `GET`, `DEL`, `CAS` |
| `GET` safety | local check once per term; a partitioned old leader can serve stale data (flagged as a known gap in Project 1's own ANALYSIS.md) | **read-index**: a fresh majority round-trip on *every* read, **plus** the precondition that the leader has committed in its own term |
| Client | inline retry loop in `main()` | `KVClient` library with sticky leader caching; `kvctl` CLI is a thin wrapper |
| Evidence the gap is real | prose only | `partition_demo.sh` and `failover_read_test.sh` run the same scenarios against both read implementations and report what each actually returns |

## Layout

```
include/
  wire.h            binary framing + serialization
  raft_node.h       RaftNode class, message types, ClientResult helpers
  kv_client.h       KVClient — the reusable client library
src/
  raft_node.cpp     the Raft algorithm + KV state machine + read-index
  main.cpp          node process entry point (SIGUSR1 toggles the simulated partition)
  kv_client.cpp     leader discovery, redirect-following, sticky leader hint
  kvctl.cpp         CLI: GET / SET / DEL / CAS, built on KVClient
  direct_probe.cpp  one-shot raw probe of a single node (no redirects, no retries)
cluster.conf        5-node membership: <id> <host> <port>
scripts/
  run_cluster.sh    start all 5 nodes (background), logs to logs/
  stop_cluster.sh   stop them and wait until they have actually exited
  demo.sh           election -> writes -> kill -9 leader -> failover -> DEL -> CAS
  partition_demo.sh the partitioned-old-leader experiment (takes a binary name)
  failover_read_test.sh  the freshly-elected-leader experiment (binary name, trials)
evidence/           captured output from verified runs (see ANALYSIS.md)
CMakeLists.txt      builds raft_node AND raft_node_unsafe from the same source
```

## Build

```
cmake -B build -S .
cmake --build build -j4
```

Produces `build/raft_node`, `build/raft_node_unsafe`, `build/kvctl` and
`build/direct_probe`.

`raft_node_unsafe` is **not** a second implementation. It is the same
`raft_node.cpp` compiled with `-DRAFT_UNSAFE_READS`, which swaps in
Project 1's original local-only `GET` check. It exists purely so the
partition experiment can be a controlled A/B comparison.

## Run

### 1. Functional demo (failover, DEL, CAS)

```
bash scripts/demo.sh
```

Builds, starts a 5-node cluster, waits for an election, writes three keys,
reads one back, **`kill -9`s the leader**, waits for a new election, proves
the data survived, writes through the new leader, then exercises `DEL` and
both outcomes of `CAS` (match → `OK` and the value changes; mismatch →
`MISMATCH` and the value does *not* change). Prints `DEMO PASSED` or
`DEMO FAILED` and exits accordingly; every check is made against the
client's actual output.

The cluster is left running afterwards so you can poke at it:

```
./build/kvctl cluster.conf SET foo bar
./build/kvctl cluster.conf GET foo
./build/kvctl cluster.conf CAS foo bar baz     # prints OK or MISMATCH
./build/kvctl cluster.conf DEL foo
bash scripts/stop_cluster.sh
```

`CAS key expected new` treats a missing key as the empty string, so
`CAS k "" v` is "set `k` only if it doesn't exist yet".

### 2. The read-index experiment (the main result)

```
./scripts/partition_demo.sh raft_node_unsafe    # Project 1's check: serves a STALE read
./scripts/partition_demo.sh raft_node           # read-index: refuses
```

Each run starts a fresh 5-node cluster and:

1. writes `k = before-partition` through the leader,
2. partitions that leader (`kill -USR1`) — it stays alive, keeps serving
   clients, but is cut off from every peer,
3. waits for the other four nodes to elect a new leader,
4. writes `k = after-partition-new-leader` through the new leader,
5. asks the **old** leader directly for `k`, using `direct_probe` so
   nothing retries or redirects and you see its raw answer.

Captured results (also in `evidence/`):

| Binary | Old leader's answer to `GET k` | Verdict |
|---|---|---|
| `raft_node_unsafe` | `ok=true value="before-partition"` | stale read — linearizability violated |
| `raft_node` | `ok=false value=""` | refuses, because it can't confirm a majority still follows it |

Verified 3 runs of each binary, alternating, with identical outcomes and
no leaked processes afterwards.

**Important caveat:** the partition is simulated inside the node (a flag
that drops all peer RPCs), not done at the kernel network layer. It is
equivalent *for this experiment* because the thing being tested is the
node's decision logic, but it is a test hook, not a real network fault.
ANALYSIS.md covers what that does and doesn't prove.

### 3. The other half of read safety: freshly elected leaders

```
./scripts/failover_read_test.sh raft_node 20          # read-index
./scripts/failover_read_test.sh raft_node_unsafe 10   # Project 1's check
```

Per trial: write a value, **`kill -9` the leader immediately**, then hammer every
surviving node with raw reads and check that the *first* successful answer is
the value that was acknowledged. This catches the opposite failure from the
partition test: a leader that is too *new* (its `commitIndex_` hasn't caught up
to what the previous leader acknowledged) rather than too *old*.

| Build | Result |
|---|---|
| final `raft_node` | 20/20 correct |
| `raft_node_unsafe` | 10/10 correct (it keeps the own-term-commit wait) |
| first read-index version (missing that precondition) | **20/20 stale** — see ANALYSIS.md bug #7 |

The two tests are complementary: `raft_node_unsafe` passes this one and fails
the partition test; the first read-index version did the reverse. A correct
read path needs both mechanisms.

### Cost

Read-index makes every `GET` do a round trip to the peers. Measured on loopback
(in-process client, p50): ~370 µs vs ~72 µs for the local-only check, and it
stays flat when a follower is dead or frozen because a read returns as soon as a
majority has answered. Details and the discarded first attempt in
`evidence/04-read-index-latency.txt` and ANALYSIS.md bug #8.

## Logs

`logs/nodeN.log`, millisecond timestamps. Sort by timestamp rather than
file name when reading them together — see Project 1's ANALYSIS.md for the
bug that taught that lesson.

## Build-location note

Build and run in a normal local directory, not a FUSE-mounted delivery
folder; those commonly can't keep the executable bit, so freshly built
binaries fail with "Permission denied". Copy the project somewhere local
first if you hit that.
