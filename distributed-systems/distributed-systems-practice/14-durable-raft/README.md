# Durable Raft: write-ahead log, crash recovery, snapshots and InstallSnapshot

Projects 11-13 built a sharded, transactional key-value store on Raft — and all of
it lived in memory. Every one of those projects listed the same gap: *a node that
restarts has forgotten everything*, so "the data survives" only ever meant "survives
a minority of nodes failing". This project closes that gap in the Raft nodes
themselves:

* **A write-ahead log (WAL)** for Raft's persistent state — `currentTerm`,
  `votedFor` and the log — CRC-framed, `fsync`'d before any node replies to or
  acknowledges anything, with a recovery path that distinguishes a *torn tail* (a
  write the crash interrupted: cut it) from *damage in the middle* (refuse to start).
* **Snapshots and log compaction**, so the WAL and the restart time stay bounded,
  with crash-safe file replacement and a recovery path for every point a crash can
  land between "snapshot written" and "log rewritten".
* **InstallSnapshot**, so a follower that was down while the leader compacted can
  still be brought up to date.

It is the same 9-process, 3-shard transactional store as Project 13 (the 2PC
protocol is untouched), now surviving `kill -9` of any node, of a whole shard, and of
**the entire cluster at once** — with a transaction half-done.

The evidence is a set of crash experiments run on real processes and a real disk:
`kill -9` at random and at chosen points, whole-cluster outages under the bank-transfer
workload from Project 13 (checked exactly), crashes injected at each step *inside* log
compaction and snapshot installation, hand-corrupted WAL and snapshot files, and a
**simulated power loss** (the only part that is simulated — see below) that shows what
`fsync` is actually for. Each safety mechanism has an ablation that removes it and
shows what breaks. See [ANALYSIS.md](ANALYSIS.md) for the design, the crash-consistency
argument, the measurements, what is real versus simulated, what is *not* covered, and
the bugs found along the way.

C++17, no dependencies beyond the standard library and POSIX (`fsync`, `rename`).

## What it demonstrates

| Question | Answer, and where to see it |
|---|---|
| Does a node that is `kill -9`'d come back with its data, and rejoin? | Yes: follower and leader, both converge to the byte-identical state digest of their peers, having rebuilt their log from the WAL (`evidence/01`, section 1). |
| Does a whole shard dying at once lose anything? | No: all 60 accounts read back as the latest acknowledged value (`evidence/01`, section 2). |
| What happens to a half-done 2PC transaction when **every node** is killed? | Its locks and (if recorded) its decision record survive: PREPARE-only ⇒ rolled **back**; COMMIT decided ⇒ rolled **forward** after the restart (`evidence/01`, section 3). |
| A crash tears the last WAL record. | Replay stops at the bad record, cuts the tail, the leader re-sends the lost entry (`evidence/01`, 4a/4b). |
| A byte flips in the *middle* of the WAL. | Not a torn tail — the node refuses to start and says why, instead of silently dropping entries it may have acknowledged (4c). |
| A write-path I/O error (`fsync` returns `EIO`). | The node stops (`FATAL: WAL fsync failed`) instead of retrying and acknowledging data that may not be on disk; its peers keep serving; it rejoins after a restart (`evidence/01`, section 6). |
| Is persisting the vote necessary? | Ablation: with `RAFT_NO_PERSIST_VOTE=1` a restarted node grants a second vote in the same term (a double vote); the real node denies it (`evidence/01`, section 5). |
| Is the log bounded? | Yes: a snapshot every 40 applied entries keeps ~43 entries and a ~2 KB WAL versus 343-370 ever appended (`evidence/02`, section 1). |
| Does restart use the snapshot? | Yes: nodes recover from `snapshot@N` and replay only the WAL tail; digests identical (`evidence/02`, section 2). |
| A follower down while the leader compacted past its log? | Brought up by **InstallSnapshot** (leader's log base 524-529 > follower's last index 344); converged (section 3). |
| Does a crash at *every step inside compaction / installation* recover? | 6 compaction crash points and 2 install crash points, each: crash exactly there, restart on the half-written files, converge, no temp files left, data correct (section 4). |
| A snapshot file is damaged. | Refused ("snapshot is corrupt") — the log it replaced is gone, so guessing would lose committed data (section 5). |
| What does `fsync` buy, given `kill -9` never loses page-cache data? | Power loss does. With fsync, all 60 acknowledged writes survive a simulated power loss of all nodes; with `RAFT_FSYNC=0`, **0 of 60** do; and `kill -9` with `RAFT_FSYNC=0` still loses nothing, which is why `kill -9` cannot test fsync (`evidence/03`). |
| Does the transactional bank workload stay exactly correct while the cluster is killed under it, including an all-nodes outage? | Yes: **0 violations in 10/10 runs** (5 seeds × `kill -9` and simulated power loss, each with all nine nodes dying at once at t=26 s); 13 transactions left in doubt by the outages were all resolved from their decision records; with `RAFT_FSYNC=0` or `RAFT_PERSIST=0` the same test flags **5/5** runs each (`evidence/04`). |
| What do the durability mechanisms cost? | Restart takes ~3.2 s however long the log is (election timeout dominates); without snapshots the WAL reaches 7.4 MB at 52k entries (replay ≈150 ms), with them ≤130 KB; fsync costs ≈0.5 ms per call and ~6% of write throughput at most (within noise, the 150 ms tick dominates); but snapshots grow ~330 B per transaction because transaction records are never garbage-collected (`evidence/05`). |

## Layout

```
include/
  wal.h              the WAL (record format, replay, rewrite) + snapshot file + helpers
  raft_node.h        Raft + KV/transaction state machine (Project 13's) + persistence,
                     snapshots, InstallSnapshot, INFO
  txn_client.h  sharded_client.h  kv_client.h  hash_ring.h  wire.h   (earlier projects)
src/
  wal.cpp            WAL append/replay/rewrite, torn-tail vs mid-log corruption, snapshot
                     file write/load, simulated power loss, crc32/fnv1a
  raft_node.cpp      Raft; recovery; persistence points; compaction; InstallSnapshot
  main.cpp           node process (SIGUSR2 = TEST-ONLY simulated power loss)
  direct_probe.cpp   raw-RPC probe: INFO, VOTE <term> <cand> <lastIdx> <lastTerm>, ...
  txntool.cpp        bank workload (+ `load`, a plain-SET load generator)
  txn_client.cpp  txnctl.cpp  kv_client.cpp  sharded_client.cpp  kvctl.cpp   (earlier projects)
ring.conf            the shard map (3 shards)
shards/shardN.conf   membership of shard N's Raft group (3 nodes each)
scripts/
  lib.sh  node_ctl.sh      check helpers; per-node start/kill/INFO/wait_converged helpers
  run_shards.sh  stop_shards.sh
  persist_demo.sh          kill -9 recovery, whole-cluster kill with half-done 2PC, WAL damage, vote persistence
  snapshot_demo.sh         compaction, restart from snapshot, InstallSnapshot, crash at every compaction step
  power_loss_demo.sh       what fsync is for: fsync on/off x simulated power loss / kill -9
  durable_stress.sh        the main experiment: bank workload under a kill/restart nemesis, 4 variants
  measure.sh               recovery time vs log length, fsync cost, snapshot size
  txn_demo.sh  crash_recovery_demo.sh  fence_ablation.sh  bank_stress.sh     (Project 13's; unchanged)
evidence/            captured output from verified runs (see "How this was verified"):
  01 persist demo   02 snapshot demo   03 power-loss demo   04 durable stress (20 runs + per-run logs)
  05 measurements   06 Project 13's scripts on the durable build   07 fresh-extract verification
```

## Build

```
cmake -B build -S .
cmake --build build -j2
```

Build and run in a normal local directory, not a FUSE-mounted delivery folder: those
often can't keep the executable bit, so fresh binaries and the scripts fail with
"Permission denied". Copy the project somewhere local first if you hit that. The
scripts need `bash`, `bc`, `ps` and `tar`; the nodes write to `./data/` (relative to the
project root, or `$RAFT_DATA_DIR`).

## Configuration (environment variables read by `raft_node`)

Read once at startup; there is no `#ifdef`, so the shipped binary is the ablated binary.

| Variable | Default | Effect |
|---|---|---|
| `RAFT_DATA_DIR` | `data` | parent of the per-node directories `<config-stem>-node<id>/` (`wal`, `snapshot`) |
| `RAFT_PERSIST` | `1` | `0` = in-memory only, as in Projects 11-13 (the "no persistence" ablation) |
| `RAFT_FSYNC` | `1` | `0` = write the WAL but never `fsync` (the "no fsync" ablation) |
| `RAFT_NO_PERSIST_VOTE` | off | `1` = never record `currentTerm`/`votedFor` (the "forgets its vote" ablation) |
| `RAFT_SNAP_EVERY` | `0` (off) | take a snapshot after this many applied entries |
| `RAFT_SNAP_KEEP` | `0` | entries to keep behind the snapshot so slightly-slow followers need no InstallSnapshot |
| `RAFT_FAIL_FSYNC_AFTER` | off | test only: after N successful fsyncs every WAL `fsync` fails with `EIO` |
| `RAFT_CRASH_AT` | none | `_exit(77)` at a named point: `snap-tmp-synced`, `snap-renamed`, `snap-written`, `wal-tmp-synced`, `wal-renamed`, `compact-done`, `install-snap-written` |
| `RAFT_TXN_ABLATE` | none | Project 13's transaction ablations |

`SIGUSR2` to a node = **simulated power loss** (test only): cut the WAL back to what was
`fsync`'d, drop an unsynced snapshot, `_exit(137)`.

## Run

### 1. Start a cluster and look at it

```
scripts/run_shards.sh 1 2 3                      # wipes ./data and ./logs unless KEEP_DATA=1
./build/txnctl ring.conf SET alice 100
./build/direct_probe 127.0.0.1 9111 INFO         # any node answers INFO about itself
kill -9 $(cat logs/shard1-node1.pid); KEEP_DATA=1 scripts/run_shards.sh 1   # restart on the same data
```

`INFO` shows role, term, votedFor, log base / last index / commit / applied, snapshot
index and size, WAL bytes, fsync count and time, what was recovered at startup and how
long it took, and a 64-bit digest of the state machine — equal digests on equal applied
indices mean byte-identical replicas. A node's log (`logs/shardS-nodeN.log`) records
`RECOVERED from disk: ...`, `SNAPSHOT at index ...`, `INSTALLED snapshot from leader ...`.

### 2. WAL recovery demo (`scripts/persist_demo.sh`, ~35 s)

Starts its own cluster. Prints `PERSIST DEMO PASSED` (38 checks) or `FAILED`.

### 3. Snapshot / InstallSnapshot / crash-inside-compaction demo (`scripts/snapshot_demo.sh`, ~30 s)

Prints `SNAPSHOT DEMO PASSED` (73 checks) or `FAILED`.

### 4. Power loss: what fsync is for (`scripts/power_loss_demo.sh`, ~30 s)

Three fresh clusters (fsync on + power loss; fsync off + power loss; fsync off + `kill -9`).
Prints `POWER LOSS DEMO PASSED` (9 checks) or `FAILED`.

### 5. The stress test (`scripts/durable_stress.sh`, the main experiment, ~18 min)

```
scripts/durable_stress.sh            # seeds 1 2 3 4 5, four variants
scripts/durable_stress.sh 1 2        # fewer seeds
ONLY=safe scripts/durable_stress.sh 1
```

The Project 13 bank workload (`txntool bank`, 8 workers, 12 accounts, 45 s) runs while a
nemesis kills a random node (t=4 s), a random whole shard (10 s), a shard leader (18 s),
**all nine nodes at once** (26 s) and another leader (35 s), restarting each. Snapshots
are on (every 25 entries). Four variants: `safe` (kill -9), `safe-powerloss` (SIGUSR2),
`nofsync-powerloss`, `nopersist`; the first two must be clean, the last two must be caught.

### 6. Measurements (`scripts/measure.sh`, ~12 min)

Recovery time vs log length (no snapshots vs snapshots), the cost of fsync, snapshot
size growth. `scripts/measure.sh m2` runs one experiment.

### 7. Project 13's scripts still pass on the persistent build

`txn_demo.sh`, `crash_recovery_demo.sh`, `fence_ablation.sh`, `bank_stress.sh` — unchanged;
they run with persistence and fsync on, snapshots off.

## How this was verified

Everything above was run, not just written, and each piece more than once:

* In the working tree, on the final build: `persist_demo.sh` 3/3 (38 checks), `snapshot_demo.sh` 3/3
  (73 checks), `power_loss_demo.sh` 3/3 (9 checks) — `evidence/01`-`03`. `measure.sh` once, `m2` twice
  (`evidence/05`).
* `durable_stress.sh`, full: 4 variants x 5 seeds = 20 runs of 45 s (`evidence/04`, with every run's bank
  output, nemesis schedule and node logs). The real protocol: 0 violations in 10/10 runs; `RAFT_FSYNC=0` and
  `RAFT_PERSIST=0`: flagged 5/5 each. (That run was captured one small change before the final tree: a
  test-only fault-injection hook, `RAFT_FAIL_FSYNC_AFTER`, was added to `Wal::sync()` afterwards, inert unless
  the variable is set. The reduced re-run below is on the final code.)
* Project 13's `txn_demo.sh` (56 checks), `crash_recovery_demo.sh` (31), `fence_ablation.sh` (9) and
  `bank_stress.sh 1 2` re-run unchanged on the durable build (`evidence/06`).
* Then, from a **fresh unzip of this very zip**, built with the commands above in an empty directory
  (`evidence/07`): `persist_demo.sh`, `snapshot_demo.sh`, `power_loss_demo.sh`, `durable_stress.sh 1 2`
  (all four variants, 7 min: clean 4/4 for the real protocol, flagged 4/4 for the ablations),
  `txn_demo.sh`, `crash_recovery_demo.sh`, `fence_ablation.sh` and `bank_stress.sh 1 2` — all PASSED. No
  node processes were left running.

The ablations exist because a check that has never been seen to fail has not been shown able to detect
failure; some of the checks were also seen to fail first (the fsync-failure test failing while the node did not
stop, the power-loss demo's survivor check reporting a zombie as alive). ANALYSIS.md's "Bugs and surprises"
lists what testing the tests turned up, including a SIGPIPE bug in `wire.h` that predates this project.

## What is and isn't real

Real: 9 separate processes, real TCP, real Raft, a real ext4 disk with real `fsync`,
`rename` and directory-`fsync` calls, real `kill -9` of nodes / shards / the whole cluster,
files corrupted with `dd`/`truncate`, crashes injected by `_exit` at named points inside
compaction, real concurrent clients.

**Simulated: the power cut.** This box cannot lose power on demand, and `kill -9` cannot
test `fsync` (the kernel's page cache outlives the process), so `SIGUSR2` truncates the WAL
to the length it had at the last successful `fsync` and exits. The `fsync` calls are real;
the lost-power effect is a model of "everything after the last sync is gone", not of a
real disk, which may also reorder writes, tear sectors, or lose a `rename` whose directory
was not synced. Absent: membership changes (a node that lost its disk cannot safely rejoin
under the same id), group commit, transaction-record garbage collection (snapshots grow),
a multi-chunk InstallSnapshot, and every real-hardware failure mode (silent corruption,
lying disk caches). ANALYSIS.md has the full list and why each matters.
