# Toy Raft — a teaching implementation of leader election, log replication, and failover

A from-scratch Raft cluster in C++17: one OS process per node, talking over
plain TCP, with a tiny key/value store as the replicated state machine.
No external dependencies — no protobuf, no gRPC, no libraries beyond the
C++ standard library and POSIX sockets. The point is to see every message
on the wire and every line of consensus logic, not to hide it behind a
framework.

Based on Diego Ongaro and John Ousterhout's ["In Search of an
Understandable Consensus Algorithm"](https://raft.github.io/raft.pdf)
(the Raft paper). See [ANALYSIS.md](ANALYSIS.md) for exactly which parts
of the paper this does and doesn't implement, and for a detailed account
of three real concurrency/correctness bugs this implementation had — and
how each one was actually found and fixed, not just asserted to work.

## What's here

```
include/
  wire.h          binary framing + serialization, shared by nodes and client
  raft_node.h     RaftNode class declaration, message types, wire helpers
src/
  raft_node.cpp   the Raft algorithm itself — election, replication, commit
  main.cpp        node process entry point (reads cluster.conf, runs one node)
  client.cpp      CLI client with leader-redirect-following
cluster.conf      5-node membership list: <id> <host> <port>
scripts/
  run_cluster.sh  starts all 5 nodes as background processes, logs to logs/
  stop_cluster.sh kills every node started by run_cluster.sh
  demo.sh         full scripted demo: election -> writes -> kill leader ->
                   failover -> prove data survived -> new writes
CMakeLists.txt
```

## Build

```
cmake -B build -S .
cmake --build build -j4
```

Produces `build/raft_node` (the server) and `build/raft_client` (the CLI).

## Run the full demo

```
bash scripts/demo.sh
```

This builds, starts a 5-node cluster, waits for an election, writes three
keys through whichever node wins, reads one back, **kills that leader
process outright** (`kill -9`, simulating a real crash), waits for the
survivors to elect a new leader, proves the previously-written data is
still there, and writes one more key through the new leader to show the
cluster is fully healthy again. It prints `DEMO PASSED` or `DEMO FAILED`
and exits accordingly — nothing about the result is asserted without the
script actually checking it against the client's output.

Leave the cluster running afterward and poke at it directly:

```
./build/raft_client ./cluster.conf SET foo bar
./build/raft_client ./cluster.conf GET foo
bash scripts/stop_cluster.sh      # when done
```

The client doesn't know who the leader is in advance — it tries a node,
and if that node isn't the leader, follows the `leaderHint` in the
response (or just moves to the next node in the config if there's no
usable hint yet) until it finds the real leader or exhausts its retry
budget.

Logs for each node land in `logs/nodeN.log`, timestamped to the
millisecond — read them side by side (sorted by timestamp, not by
filename — see ANALYSIS.md for why that distinction matters) to watch an
election unfold across all five nodes at once.

## A note on the sandbox this was built in

Build-and-run has to happen in a real local directory, not under a
FUSE-mounted delivery/outputs folder — such mounts commonly can't persist
the executable bit, so a binary built there will fail with "Permission
denied" even right after a successful `chmod +x`. If you hit that, copy
the project to a normal directory first.
