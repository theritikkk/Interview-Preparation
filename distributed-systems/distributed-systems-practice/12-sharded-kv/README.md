# Sharded KV store with consistent hashing

Project 11's Raft-replicated KV store, scaled out: the keyspace is split
across several **independent Raft groups** (shards) by a **consistent-hash
ring**, a router library sends each key to the shard that owns it, and — the
main result — **adding a shard while clients are reading and writing** is
shown to be correct, with a stress test that checks every read and every CAS
against an exact model, and with deliberately broken variants of the protocol
that the same test catches.

12 OS processes (4 shards x 3 Raft nodes), plain TCP on loopback, C++17, no
dependencies beyond the standard library and POSIX sockets. See
[ANALYSIS.md](ANALYSIS.md) for the design, the measurements, exactly what is
real versus simulated, what is *not* covered, and the bugs found along the
way (including one in the test harness that produced a convincing but false
"the safe protocol is broken" result).

## What it demonstrates

| Question | Answer, and where to see it |
|---|---|
| Does the ring spread keys evenly? | Yes, with enough virtual nodes: 3 shards, 128 vnodes → heaviest shard 1.07x its fair share (`evidence/01`). The hash finalizer matters: plain FNV-1a is roughly 6x worse (CV 0.41 vs 0.07) (`evidence/07`). |
| How many keys move when a shard is added? | 3 → 4 shards: **25.02%** (ideal 25.00%), **0%** of them between existing shards. `hash % N` moves **74.97%** (`evidence/01`). |
| Is each key on the shard the ring says? | Asked each shard directly (`KEYS`) and compared with the ring: 1500 keys, 502 / 493 / 505, exact match (`evidence/02`). |
| Does a shard failure stay contained? | `kill -9` shard 2's leader: shards 1 and 3 served **0 failures** in ~138 requests each; shard 2 was out for 1.6-2.6 s (one re-election), then recovered with all 1500 keys intact. 3/3 runs (`evidence/03`). |
| Does an idle rebalance move exactly the right keys? | Added shard 4: the migrator moved **425** keys, the ring predicted **425**; all 1500 values intact (`evidence/04`). |
| Is a rebalance **under live traffic** correct? | Safe protocol: **0 violations in 10/10 stress runs** (~7,000 checked reads and ~2,750 checked CASes). Five variants with one protocol step removed: **5/5 runs each** show violations, each with the symptom that step prevents (`evidence/05`). |

## Layout

```
include/
  hash_ring.h        consistent-hash ring with virtual nodes (header-only)
  sharded_client.h   ShardedClient (router), RingState, the live-migration protocol
  kv_client.h        per-shard Raft client (from Project 11, + cad() and keys())
  raft_node.h  wire.h
src/
  raft_node.cpp      Raft + KV state machine (from Project 11, + KEYS and CAD commands)
  main.cpp           node process entry point
  kv_client.cpp  kvctl.cpp  direct_probe.cpp   (Project 11, small additions)
  sharded_client.cpp router + migration protocol
  shardctl.cpp       CLI: GET / SET / DEL / CAS / WHERE through the ring
  shardtool.cpp      load / verify / census / probe / rebalance / stress
  ring_stats.cpp     balance and key-movement measurements for the ring
  hash_ablation.cpp  FNV-1a with vs without the fmix64 finalizer
ring.conf            the shard map (see below)
shards/shardN.conf   membership of shard N's Raft group (3 nodes each)
scripts/
  run_shards.sh  stop_shards.sh      start / stop shards (default 1 2 3)
  isolation_test.sh                  kill a shard's leader, show the others are unaffected
  rebalance_demo.sh                  add shard 4 to a quiet cluster, verify placement
  rebalance_stress.sh                add shard 4 under live traffic; safe vs broken variants
evidence/            captured output from verified runs (see ANALYSIS.md)
```

## Build

```
cmake -B build -S .
cmake --build build -j2
```

Build and run in a normal local directory, not a FUSE-mounted delivery folder:
those often can't keep the executable bit, so fresh binaries fail with
"Permission denied". Copy the project somewhere local first if you hit that.

## The shard map

`ring.conf` is the only thing clients need besides each shard's `shardN.conf`:

```
vnodes 128
state stable            # stable | migrating
shards 1 2 3            # the ring clients converge on (the NEW ring while migrating)
old_shards 1 2 3        # only read while migrating
```

Every client builds the identical ring from it (the ring is a pure function of
shard set, vnode count and salt). Shard N's Raft group is `shards/shardN.conf`
(`<id> <host> <port>` per node; ids are local to the group).

## Run

### 0. The ring on its own (no cluster needed)

```
./build/ring_stats        # balance vs vnodes, key movement N -> N+1, ring vs hash % N
./build/hash_ablation     # why the finalizer in hash_ring.h is there
```

### 1. Start shards and use them

```
./scripts/run_shards.sh                       # shards 1 2 3 (9 nodes), waits for leaders
./build/shardctl ring.conf SET user:42 alice
./build/shardctl ring.conf GET user:42
./build/shardctl ring.conf WHERE user:42      # which shard owns it
./build/shardctl ring.conf CAS user:42 alice bob
./build/shardctl ring.conf DEL user:42
./build/shardtool load ring.conf 1500 48      # 1500 keys, 48 threads
./build/shardtool census ring.conf 1500       # ask every shard directly, compare with the ring
./scripts/stop_shards.sh
```

Values must be non-empty (empty is indistinguishable from "absent"), keys may
not contain spaces or newlines, and a CAS `expected` may not contain spaces —
the wire format is space-delimited. These are rejected with a clear message
rather than silently mangled.

### 2. Fault isolation

```
./scripts/isolation_test.sh        # ~40 s; prints ISOLATION TEST PASSED / FAILED
```

Loads 1500 keys, probes every shard with one GET per 100 ms for 14 s, `kill -9`s
shard 2's Raft leader at t=3 s, and asserts shards 1 and 3 had zero failures,
shard 2 failed and then recovered, and every key is still present and on the
right shard.

### 3. Rebalance a quiet cluster

```
./scripts/rebalance_demo.sh        # ~40 s; prints REBALANCE DEMO PASSED / FAILED
```

Loads 1500 keys on 3 shards, starts an empty shard 4, shows that judged against
the 4-shard ring the keys that must move are "misplaced" on 1-3 and "missing" on
4, runs the migrator, and asserts: the number of keys moved equals what the ring
predicts, every key is on its ring owner, every value is intact.

### 4. Rebalance under live traffic (the main experiment)

```
./scripts/rebalance_stress.sh      # ~10 min with the default 5 seeds
./scripts/rebalance_stress.sh 1 2  # quicker: just two seeds
```

Starts shards 1-4. Each run: 16 worker threads do a random mix of GET (40%),
SET (30%), DEL (15%) and CAS (15%, 40% of those deliberately given a wrong
expected value) on 100-200 keys; the shard map is flipped to *migrating*, four
migrator threads move every key whose owner changed, and the map is flipped to
the new stable ring while traffic continues. Every GET and every CAS result is
checked against what that key must hold. Variants:

| Variant | What it is | Expected |
|---|---|---|
| `safe-natural`, `safe-hostile` | the real protocol; "hostile" inserts 300-600 ms sleeps inside the race windows | 0 violations |
| `naive` | route by the new ring instantly, plain-overwrite copy | violations |
| `ablate-norecheck` | safe protocol minus the second read of the new owner | **vanished** reads |
| `ablate-notombstone` | safe protocol minus the tombstone on DEL | **resurrected** reads |
| `ablate-overwrite` | safe protocol, but the migrator copies with a plain SET | **stale** reads |

The script exits non-zero if the safe protocol shows any violation, if any
broken variant is *not* caught, or if a variant's signature symptom is absent.
Per-run output is in `logs/stress/`.

Captured result (seeds 1-5; the numbers are violations summed over the five runs):

```
safe-natural          0/5  runs with violations | vanished=0   stale=0   resurrected=0   cas-wrong=0
safe-hostile          0/5  runs with violations | vanished=0   stale=0   resurrected=0   cas-wrong=0
naive                 5/5  runs with violations | vanished=43  stale=31  resurrected=2   cas-wrong=25
ablate-norecheck      5/5  runs with violations | vanished=21  stale=0   resurrected=0   cas-wrong=0
ablate-notombstone    5/5  runs with violations | vanished=0   stale=0   resurrected=6   cas-wrong=5
ablate-overwrite      5/5  runs with violations | vanished=0   stale=25  resurrected=5   cas-wrong=11
```

(`resurrected` also appears under `overwrite` because a plain SET clobbers a
tombstone just as it clobbers a newer value.)

`shardtool stress` also takes `--keys`, `--workers`, `--migrators`, `--seed`,
`--read-gap-ms` and `--step-delay-ms` for your own experiments, and resets its
own keyspace on every shard before it starts, so you can run it repeatedly
against the same cluster.

## How this was verified

Everything above was run, not just written: the full 5-seed stress experiment
in the working tree, and then — from a fresh extract of this very zip, built
with the commands above — `isolation_test.sh` (PASSED), `rebalance_demo.sh`
(PASSED, 425 keys moved as the ring predicted), and `rebalance_stress.sh 1 2`
(safe: 0 violations in 4 runs; all four broken variants detected with their
signature symptom). No node processes were left running afterwards.

## What is and isn't real

Real: 12 separate processes, real TCP, real Raft (elections, log replication,
read-index reads), real `kill -9`, real concurrent clients. Simulated or absent:
there is no control plane (the shard map is a file and "every client adopts the
new map" is done in-process by the test), no epoch fencing, no persistence or
log compaction, no cluster membership changes inside a shard, and throughput
scaling is not measured (this sandbox has 2 CPUs for 12 processes). ANALYSIS.md
has the full list and why each matters.
