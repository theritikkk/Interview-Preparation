# ANALYSIS — making Raft durable: a WAL, snapshots, and the crash points in between

Projects 11-13 built Raft, a sharded store and cross-shard transactions — in memory.
Each of them ended with the same sentence: *no persistence, no log compaction*. That
sentence hides something important: Raft's safety argument is **conditional**. Its
Figure 2 lists three pieces of state — `currentTerm`, `votedFor`, `log[]` — that must be
on stable storage *before a node answers an RPC*, and everything the paper proves
(one leader per term, committed entries never lost) assumes that. An in-memory Raft
node that restarts is not "a slow node", it is a node that has violated the assumptions,
and the system can lose acknowledged writes the moment a majority restarts.

This project makes the assumption true, and then does what the earlier projects did:
for every mechanism, run an experiment that removes it and watch something specific
break. Two things are harder here than in the earlier projects, and the document is
honest about both: `kill -9` cannot test `fsync` at all (the page cache outlives the
process), so the power-loss experiments rest on a *simulated* power cut; and the
interesting bugs live at crash points *between* two durable steps, which I could only
reach by injecting crashes at named places.

## Headline results

| Claim | Result | Evidence |
|---|---|---|
| A killed node (follower, leader) restarts from its WAL, rejoins and converges | Byte-identical state digest with its peers; persist demo 38 checks, 0 failures, 3/3 runs | `evidence/01` |
| The whole cluster `kill -9`'d with a 2PC transaction half-done: locks and decision record survive | PREPARE-only ⇒ rolled **back**; decided COMMIT ⇒ rolled **forward**, money conserved | `evidence/01` §3 |
| A failed `fsync` stops the node instead of being retried | injected `EIO`: `FATAL: WAL fsync failed`, node stops, peers keep serving, node rejoins after restart | `evidence/01` §6 |
| Torn WAL tail is cut and re-sent; mid-log damage is refused | tail: cut + re-replicated; middle: node refuses to start with an explanation | `evidence/01` §4 |
| The persisted vote is load-bearing | Real: restarted node DENIES a second candidate in the same term. Ablation (`RAFT_NO_PERSIST_VOTE=1`): GRANTS it — a double vote | `evidence/01` §5 |
| Snapshots bound the log, the WAL and restart | ~43 entries retained and a ~2 KB WAL, versus 343-370 entries ever appended; restart replays only the tail | `evidence/02` §1-2 |
| InstallSnapshot catches up a follower that is behind the compacted prefix | leader log base 524-529 > follower last index 344; follower converged, `INSTALLED snapshot` logged | `evidence/02` §3 |
| A crash at every step inside compaction / installation recovers | 6 + 2 crash points, each injected, restarted, converged, no temp files, 120 values correct; 73 checks, 3/3 runs | `evidence/02` §4 |
| `fsync` matters only for power loss, and kill -9 cannot show that | fsync on: 60/60 acknowledged writes survive a simulated power loss of all nodes; `RAFT_FSYNC=0`: **0/60**; `RAFT_FSYNC=0` + `kill -9`: 60/60 | `evidence/03` |
| The bank workload stays exactly correct under a kill/restart nemesis, incl. all nine nodes dying at once | **0 violations in 10/10 runs** (5 seeds × {`kill -9`, simulated power loss}): 1,449 committed transfers, 682 consistent audits and 0 skewed, 13 transactions left in doubt by the outages (all resolved through the decision record: 5 rolled forward, 8 back, 0 unresolved), all replicas byte-identical afterwards, ~3,450 snapshots, 28 InstallSnapshots, 240 recoveries, 0 `FATAL` | `evidence/04` |
| ...and the stress test detects loss of durability | `RAFT_FSYNC=0` + power loss: **5/5** runs flagged; `RAFT_PERSIST=0`: **5/5** runs flagged — in every run all 12 accounts end unreadable (total 0 instead of 12,000) | `evidence/04` |
| What it costs | Restart ≈3.2 s regardless of log length (election timeout dominates); WAL replay 21→148 ms for 9.5k→52k entries without snapshots, ≈0-1 ms with them; WAL 7.4 MB vs ≤130 KB; fsync ≈0.5 ms (leader, one per command) / ≈0.8-1.0 ms (follower, batched); throughput 113 vs 120 writes/s with vs without fsync (~6%, within noise: the 150 ms tick dominates); but snapshots grow ~330 B per transaction forever (no GC) | `evidence/05` |
| Project 13's protocol is unaffected | On the durable build (persistence + fsync on, snapshots off): `txn_demo` 56 checks, `crash_recovery_demo` 31, `fence_ablation` 9 — all pass; `bank_stress 1 2`: real protocol 0 violations in 4 runs (2 with a leader killed), the three broken variants caught 2/2 | `evidence/06` |

What this does **not** show is under [Known gaps](#known-gaps-not-hidden-not-fixed).

## What Raft needs on disk — and what it doesn't

| State | Durable? | Why |
|---|---|---|
| `currentTerm`, `votedFor` | **yes**, before replying | If a node forgets it voted in term *t*, it can vote again for a different candidate in *t*: two leaders in one term. `evidence/01` §5 makes this happen. |
| the log | **yes**, before acknowledging | An `AppendEntries` success counts toward a commit quorum. If the follower forgets the entry after a restart, an entry the client was told was committed can sit on a "majority" that no longer holds it. |
| `commitIndex`, `lastApplied` | no | Re-derived: after restart `commitIndex = lastApplied = snapshot index`, and the rest is re-applied once a leader (a new one commits its no-op) says how much is committed. |
| `nextIndex`, `matchIndex` | no | Leader-only volatile state, rebuilt on election. |
| the state machine | no — it is a function of the log | …except that once the log is compacted, the snapshot *is* the durable copy of the prefix. |

The ordering rule that follows from the paper and that the code enforces everywhere:
**fsync before anything is said**.

| Event | Written | `fsync` happens before |
|---|---|---|
| term advances (higher term seen; starting an election) | HARDSTATE | any message carrying the new term |
| vote granted | HARDSTATE (term, votedFor) | the grant reply |
| leader accepts a client command | ENTRY | the leader counts its own copy toward a majority (it is also what the replicator sends later) |
| follower accepts `AppendEntries` | TRUNC (on conflict) + ENTRY… | **one** fsync, then the success reply |
| snapshot taken / installed | `snapshot.tmp` → rename → dir fsync, then WAL rewrite | the node continues (installation: before the reply) |

## Design

### The WAL

One file, `wal`, per node, of CRC-framed records:

```
[u32 len][u32 crc32(body)][body = u8 type, payload]        big-endian
  HARDSTATE  u64 term, u32 votedFor+1     latest wins
  ENTRY      u64 index, u64 term, str cmd must be exactly lastIndex+1
  TRUNC      u64 from                     drop entries >= from
  BASE       u64 index, u64 term          first record of a rewritten WAL
```

* **Truncation is a record**, not a rewrite: a follower that overwrites a conflicting
  suffix appends `TRUNC from` and then the new entries, and replay applies them in
  order. (Rewriting the file on every conflict would be both slow and a bigger crash window.)
* **Replay stops at the first bad record** — short header, implausible length, short
  body, CRC mismatch, or a record that is structurally inconsistent (an `ENTRY` that
  is not at `lastIndex+1`, a `TRUNC` outside the log, a `BASE` that is not first).
* **Torn tail vs damage in the middle.** A crash can only interrupt the write in
  flight, so a bad record with *nothing valid after it* is a torn tail: nobody was ever
  told about it (the fsync that precedes every acknowledgement never completed), so
  the file is `ftruncate`d at the last good record and the leader re-sends. A bad
  record with a **valid record after it** cannot be explained by a crash — it is media
  damage or a bug — and cutting there would silently drop entries *this node may have
  acknowledged*. `open()` scans every later byte offset for an intact record (a 32-bit
  CRC makes a false positive negligible, and this only runs when something is already
  wrong) and, if it finds one, refuses to start. `evidence/01` 4a/4b/4c exercise exactly these three
  cases.

### Errors are fatal

Every WAL or snapshot write or fsync failure ends the process (`_exit(2)` after
logging `FATAL: …`). The temptation is to retry: but after a failed `fsync` the kernel
may have *already dropped* the dirty pages and marked them clean, so a second `fsync`
can succeed without the data being on disk, and the node would acknowledge writes it
does not have ("fsyncgate"). A node that cannot make state durable must stop
participating; the other two nodes carry on. `persist_demo` §6 forces it with an injected `EIO` on a follower's third `fsync`
(`RAFT_FAIL_FSYNC_AFTER=2`): the node logs `FATAL: WAL fsync failed (Input/output error); exiting
rather than acknowledge data that may not be on disk`, stops, the other two nodes keep the shard
serving all 60 accounts, and restarted without the fault it rejoins and converges.

### Recovery

```
load snapshot  ──►  replay WAL  ──►  reconcile  ──►  commitIndex = lastApplied = snapshot index
 (CRC; corrupt         (torn tail cut;       │
  ⇒ refuse)             mid-log ⇒ refuse)    ├─ WAL base > snapshot index  ⇒ entries between are missing: refuse
                                              └─ snapshot index > last log index, or the log's term at the
                                                 snapshot index differs ⇒ the snapshot is committed state, so it
                                                 wins: reset the log to it and rewrite the WAL
```

The rest of the log is *not* applied at startup: it is unknown whether it is
committed. It is applied when a leader's `AppendEntries` carries a commit index, or when
this node becomes leader and commits its no-op. (Consequence: for a few seconds after a
whole-cluster restart nothing is served — election timeout 1.5-3 s dominates.)

### Snapshots and log compaction — two indices

Compaction needs two different numbers, and conflating them is the classic bug:

* `snapIndex_`/`snapTerm_` — the state-machine snapshot on disk: "this is the state after
  applying entry `snapIndex_`".
* `logBase_` — the index of the in-memory log's sentinel (`log_[i - logBase_]` is entry
  *i*). Always `logBase_ ≤ snapIndex_`.

They differ for two reasons. **`RAFT_SNAP_KEEP`**: keep a few entries behind the
snapshot so that a follower that is only a little behind can be served by
`AppendEntries` instead of a whole snapshot. And **crash windows**: the snapshot is
renamed into place *before* the WAL is rewritten, so a crash between them leaves
`logBase_ < snapIndex_` on disk — which is a perfectly valid state, and is what makes the
order safe (see the table below). (`lastApplied` and `commitIndex` are written nowhere:
the snapshot carries the state after `snapIndex_` and the WAL carries entries after
`logBase_`.)

The snapshot file is `RSNAP001 | u64 index | u64 term | str state | crc32`, written
to `snapshot.tmp`, `fsync`'d, `rename`d over `snapshot`, directory `fsync`'d. The
state machine is serialized (`kv_`, `locks_`, `txns_`, `recs_` — the whole Project 13
transaction state, so a snapshot taken mid-transaction carries its locks and
decisions) from `std::map`s, so **equal states serialize to equal bytes**. That is what
lets the `INFO` digest (FNV-1a over the serialized state) double as a byte-for-byte
replica comparison, and what the demos use for "converged".

### Every crash point inside compaction

`RAFT_CRASH_AT=<point>` makes a node `_exit(77)` exactly there. Each was injected
on a live follower, the node restarted on whatever was on disk, and `snapshot_demo.sh`
checks it converged with its peers and all 120 values are right.

| Crash point | On disk | What recovery does |
|---|---|---|
| `snap-tmp-synced` | complete `snapshot.tmp`; old `snapshot` (or none) and old WAL | `snapshot.tmp` is unlinked and ignored; old snapshot + old WAL are consistent |
| `snap-renamed` | new `snapshot` (dir entry maybe not durable); old WAL | `logBase ≤ snapIndex`: replays the old WAL over the new snapshot. Observed: `snapshot@657 log=(611,657]` |
| `snap-written` | new `snapshot`, dir synced; old WAL | same |
| `wal-tmp-synced` | new `snapshot`; complete `wal.tmp`; old WAL | `wal.tmp` is unlinked at open; old WAL is still valid |
| `wal-renamed` | new `snapshot`; new WAL (dir entry maybe not durable) | either old or new WAL is found; both valid |
| `compact-done` | everything | normal |
| `install-snap-written` (InstallSnapshot) | snapshot beyond the whole old log | reconcile: snapshot index > last log index ⇒ reset the log to the snapshot, rewrite the WAL. Observed: `snapshot@1026 log=(1026,1026]` |
| `wal-tmp-synced` during installation | as above, WAL rewrite interrupted | as above |

The design rule behind the order: **at every instant, the files on disk describe
a state some correct execution could have been in**. Snapshot first, WAL second works
because "snapshot ahead of the log base" is a legal state; WAL first, snapshot second
would not be (the log entries a snapshot replaces would already be gone).

### InstallSnapshot

When a leader's `nextIndex` for a follower is `≤ logBase_` the entries it needs no
longer exist, so the leader sends its snapshot (`MSG_INSTALL_SNAPSHOT`, whole state in
one message — 16 MiB frame cap). The follower, if the snapshot is newer than its
commit index: restores the state, keeps its log suffix **only if its term at the snapshot
index matches** (otherwise discards the log — it holds a different history), persists the
snapshot file, rewrites the WAL, and sets `commitIndex = lastApplied = snapIndex`.
After a failed `AppendEntries` the leader now uses the follower's `lastIndex` hint
(`nextIndex = max(1, min(nextIndex-1, hint+1))`), so a follower that is far behind is found in one
round trip rather than one entry at a time. A defensive check `_exit(3)`s if a leader
ever asks a node to overwrite an index it has already committed — a bug detector;
it never fired in the stress runs (`FATAL=0` in every seed).

## Why it is correct

**Acknowledged ⇒ durable.** Every reply and acknowledgement is preceded by the `fsync`
that covers what it promises (table above), under the node's mutex, so no other thread can
reply in between. A restart therefore finds at least everything it ever acknowledged;
what it may also find is a *partial* unacknowledged tail, which replay cuts.

**Vote safety.** The vote is `fsync`'d before the grant leaves the node, so a restart
sees it. The ablation shows the double vote; `evidence/01` §5 first proves, with a raw
`RequestVote` probe, that the real node's denial is caused by the remembered vote and not
by its term having moved on ("node's term now 1000").

**Snapshots lose nothing.** A snapshot at `lastApplied` covers only *applied* entries,
which are committed; compaction deletes only entries `≤ snapIndex` (minus the kept
tail); the snapshot is durable before the WAL stops containing those entries. A node
that receives an InstallSnapshot only adopts it if it is ahead of its commit index.
**Corrupt snapshot ⇒ refuse**, because the entries it replaced no longer exist
anywhere on this node; guessing would turn "a disk error" into "silently wrong state".

**Replay, not repetition.** After a restart a node's memory is rebuilt from the snapshot
(the state as of `snapIndex_`), and the log entries after it are applied once each, in
order, when a leader says they are committed. Nothing is applied twice onto the same
state. What this does need is determinism — the state machine must be a pure function of
the log — which the replica digests check continuously (equal applied index ⇒ equal bytes).

## Which mechanism is necessary — and the experiment that says so

| Remove… | Experiment | Observed |
|---|---|---|
| persisting `currentTerm`/`votedFor` | `persist_demo` §5: raw `VOTE` twice in one term with a `kill -9` between | real: second candidate **DENIED**; `RAFT_NO_PERSIST_VOTE=1`: **GRANTED** (double vote) |
| `fsync` | `power_loss_demo` B and `durable_stress` `nofsync-powerloss` | all 60 acknowledged writes gone; 12/12 accounts unreadable in 5/5 stress runs |
| the control: `kill -9` without fsync | `power_loss_demo` C | nothing lost — the page cache outlived the process. This is why a `kill -9` test of "durability" proves nothing about fsync |
| persistence altogether | `durable_stress` `nopersist` | 5/5 runs flagged: after the whole-shard and whole-cluster kills all 12 accounts are unreadable (total 0 of 12,000). Same symptom and same "all-or-nothing" caveat as no-fsync |
| WAL replay's tail handling | `persist_demo` §4 | shown to *happen*, not ablated (see Known gaps) |

## The stress test

`durable_stress.sh` reuses the Project 13 bank workload — 12 accounts in 4 groups over 3
shards, 8 workers doing optimistic transfers (2PC when the accounts live on different
shards), auditors taking validated snapshot reads — and adds the nemesis described in the
README: a random node, a random whole shard, a leader, **all nine nodes at once**, a
leader again, each restarted a few seconds later. Snapshots are on (every 25 entries,
5 kept), so restarts exercise snapshot + WAL tail and nodes that were down while the
leader compacted need InstallSnapshot.

| variant | how nodes die | server config | runs flagged | committed transfers (5 runs) | in doubt after outages (fwd / back / unresolved) | consistent audits / skewed | snapshots / installs / recoveries |
|---|---|---|---|---|---|---|---|
| `safe` | `kill -9` | durable | **0 / 5** | 731 | 9 (3 / 6 / 0) | 344 / 0 | 1747 / 13 / 120 |
| `safe-powerloss` | simulated power loss | durable | **0 / 5** | 718 | 4 (2 / 2 / 0) | 338 / 0 | 1710 / 15 / 120 |
| `nofsync-powerloss` | simulated power loss | `RAFT_FSYNC=0` | **5 / 5** (12 unreadable accounts each) | 364 | — | 128 / 0 | 1121 / 15 / 120 |
| `nopersist` | `kill -9` | `RAFT_PERSIST=0` | **5 / 5** (12 unreadable accounts each) | 362 | — | 127 / 0 | 0 / 0 / 0 |

(`evidence/04-durable-stress.txt`; per-run bank output, nemesis schedule and all nine node logs are in
`evidence/04-durable-stress-runs/`. A "recovery" is a node starting up and logging `RECOVERED from disk`:
24 per run = 9 initial starts + 15 restarts the nemesis caused — 1 + 3 + 1 + 9 + 1.)

What each seed asserts besides the bank's own exact checks (final balance of every
account = initial + the net of every transfer acknowledged as committed; total conserved; no
negative balance, stuck lock, unresolved in-doubt transaction, protocol violation, or
unreadable account): all nine nodes are up afterwards; each shard's replicas converge to the
same applied index and the same state digest; no node logged `FATAL`; and the clients
actually *felt* the faults (`fault-felt` > 0 counts failed reads, aborted-other and
in-doubt transactions — without it a run that the nemesis missed would pass vacuously).

### What the stress test does **not** check

* **No history checker.** "Sums and exact deltas" cannot see ordering anomalies that leave
  the totals unchanged. Same limit as Project 13.
* **The nemesis schedule is fixed per seed** (five events at set times; the *victims* are
  random). It does not kill during snapshot installation or inside a compaction — the
  crash-point injection in `snapshot_demo` covers those, separately.
* **Power loss is the truncation model**, not a real disk (below).
* **Throughput is low** (~150 committed transfers per 45 s run, with seconds of outage
  inside it) because the Raft tick is 150 ms. The runs are about correctness, not load.
* **The ablations are all-or-nothing.** With no fsync (or no persistence) and a
  cluster-wide outage, *everything* is lost, so the checker's symptom is "all 12 accounts
  unreadable" — it proves the harness can notice loss, not that it can notice a *subtle,
  partial* loss. The subtle failure (one node forgetting its vote or a tail of its log
  while the others remember) is only demonstrated by the vote experiment.

## Measurements

All on the same box: 2 CPUs, ext4 on a virtual disk, 9 node processes, 150 ms Raft tick.
Numbers are single runs unless a count says otherwise; `evidence/05-measurements.txt` has the raw output.

**M1 — restart cost vs log length** (`kill -9` all nine nodes, restart on the same data; shard 1 shown;
100-byte values, 1000 keys, 96 writer threads in 30 s bursts so the log grows between restarts):

| entries ever appended | no snapshots: WAL | replayed on restart | WAL replay (max of 3 nodes) | restart → replicas converged | with a snapshot every 1000: retained entries / WAL / snapshot | replay | restart → converged |
|---|---|---|---|---|---|---|---|
| ~9.5k | 1.35 MB | 9,542 | 21 ms | 3.20 s | 447 / 63 KB / 40 KB | 0 ms | 3.18 s |
| ~18k | 2.57 MB | 18,113 | 70 ms | 3.22 s | 870 / 123 KB / 40 KB | 1 ms | 3.18 s |
| ~26k | 3.79 MB | 26,723 | 70 ms | 3.21 s | 240 / 34 KB / 40 KB | 1 ms | 3.17 s |
| ~36k | 5.05 MB | 35,590 | 141 ms | 3.25 s | 638 / 91 KB / 40 KB | 0 ms | 2.68 s |
| ~44-46k | 6.24 MB | 43,985 | 138 ms | 3.20 s | 900 / 128 KB / 40 KB | 0 ms | 3.17 s |
| ~52-54k | 7.41 MB | 52,247 | 148 ms | 3.76 s | 134 / 19 KB / 40 KB | 0 ms | 3.18 s |

* **Without snapshots** the WAL grows linearly (~142 bytes per entry here) and so does WAL replay (the
  work is reading, CRC-checking and parsing — the entries are *not* re-applied during recovery).
* **With snapshots** the WAL stays under ~130 KB, the snapshot is a constant 40 KB (the data does not grow,
  only the history), and replay is ~0-1 ms.
* **What is *not* visible: the user-visible restart time.** "Restart → converged" is 2.7-3.8 s in all twelve
  rows and does not track the log length. It is the election timeout (1.5-3 s, randomized) plus my measurement's
  ~0.5 s polling granularity; re-applying 52k entries after the election finished within that granularity. So at
  *this* scale the benefit of snapshots is **disk space and recovery CPU**, not restart latency — I would be
  overselling it to say otherwise. A larger state machine (expensive applies, a 10 GB log) is where restart
  time starts to track the log; I did not test that.

**M2 — what fsync costs** (16 writer threads, 15 s, alternating on/off; six runs each across two rounds):

| | throughput (writes/s) | p50 / p95 latency |
|---|---|---|
| fsync on | 133.5, 110.1, 114.9, 108.9, 105.9, 105.7 — mean **113** | 130-150 / 177-187 ms |
| fsync off (`RAFT_FSYNC=0`) | 118.4, 112.6, 114.0, 133.7, 119.4, 121.9 — mean **120** | 146-150 / 158-195 ms |

fsync-on throughput is ~6% lower on average and the ranges overlap (106-134 vs 113-134); round B alone
(108.9, 105.9, 105.7 vs 133.7, 119.4, 121.9) looks like a real gap, round A does not. The honest reading: **a few
percent at most here, not separable from run-to-run noise with six runs, because the 150 ms tick — not the disk —
sets the commit latency** (p50 = 150 ms in both). The per-call cost is what the INFO counters show, and it
reveals the structure of the implementation:

| shard 1 node (round B) | entries | fsync calls | mean fsync |
|---|---|---|---|
| leader | 562 / 546 / 547 | 563 / 547 / 548 (≈ **one per command**) | 0.46-0.49 ms |
| followers | same | ~103 (**batched**: one per `AppendEntries` carrying new entries) | 0.79-1.02 ms |

The leader does one `fsync` per client command, under the mutex (no group commit): 563 × 0.48 ms ≈ 270 ms of
disk time in a 15 s run (~1.8%). Its ceiling would be about 1 / 0.48 ms ≈ 2,000 commands/s per shard; the
observed ~37/s per shard is tick-limited, so fsync is nowhere near the bottleneck on this box. On a slower
disk (a few ms per fsync, as on some network volumes) the same code would cap at a few hundred commands/s per
shard, which is when group commit becomes the first thing to build.

**M3 — snapshot size without garbage collection of transaction records** (bank workload, 12 accounts, four
40 s runs back to back on one cluster, snapshot every 25 entries; shard 1 is the coordinator shard that
holds the 2PC decision records):

| after run | committed transfers (cumulative) | shard 1: txns / recs / snapshot | shard 2: txns / snapshot | shard 3: txns / snapshot | WAL bytes (all shards) |
|---|---|---|---|---|---|
| 1 | 173 | 732 / 701 / 53.6 KB | 384 / 14.6 KB | 357 / 13.3 KB | 0.6-1.8 KB |
| 2 | 323 | 1,461 / 1,408 / 107 KB | 761 / 28.7 KB | 712 / 26.9 KB | 1.2-1.8 KB |
| 3 | 475 | 2,194 / 2,116 / 161 KB | 1,119 / 42.0 KB | 1,089 / 41.1 KB | 0.8-2.3 KB |
| 4 | 646 | 2,935 / 2,821 / 215 KB | 1,494 / 56.6 KB | 1,454 / 54.9 KB | 0.7-2.4 KB |

The WAL stays tiny (compaction works), but the **snapshot grows linearly with the number of transactions ever run**
(~330 bytes per committed transfer on the coordinator shard, ~85-90 on the others), because `txns_` and `recs_`
are never garbage-collected. The 12 accounts themselves are a few hundred bytes. That is the gap inherited from
Project 13 turning into a real cost: every snapshot, every WAL rewrite and every InstallSnapshot carries the
whole history. At this rate a snapshot would pass the 16 MiB InstallSnapshot frame cap after roughly 50,000
transfers on shard 1 (16 MiB / 330 B), at which point this implementation can no longer catch a follower up at all
— a failure that `snapshot_demo` cannot see, because it never runs that long.


## What's real and what's simulated

Real: 9 processes, TCP, Raft, an ext4 disk (`/dev/vda`), real `fsync`/`rename`/directory
`fsync`; real `kill -9` of nodes, shards and the whole cluster; real file corruption
(`dd`, `truncate`); real `_exit` at named points inside compaction.

Simulated or absent:

* **The power cut.** `SIGUSR2` truncates the WAL to its size at the last successful
  `fsync`, deletes a snapshot written without `fsync`, and exits. This models "bytes after
  the last sync are gone". It does **not** model: a disk reordering writes (a later record
  surviving without an earlier one), torn sectors inside a record that *was* synced, a
  `rename` that was not made durable by a directory `fsync` (the `wal-renamed` window is
  tested by killing the process, not by losing the rename), a disk that lies about
  flushing, or bit rot.
* **A real disk's flush latency.** The VM's `fsync` takes ~0.5-1.0 ms — an SSD-class
  number; on a spinning disk or a cloud volume with different semantics every
  conclusion about *cost* would change, the ones about *correctness* would not.
* **Everything about membership.** A node that loses its disk entirely cannot safely
  come back under the same id: it would forget votes and entries it acknowledged, which is
  exactly the failure the whole project exists to prevent. Real systems treat that as
  "remove the member, add a new one" (reconfiguration, not implemented here).

## Known gaps (not hidden, not fixed)

1. **Mid-log damage is detected only when an intact record follows it.** A flipped byte in
   the very last record is indistinguishable from a torn tail and is cut. That loses
   nothing *if the disk is honest* (the record was not yet synced) and loses an
   acknowledged entry if the disk corrupted a synced record; the follower can be re-sent it, but a
   *majority* with that damage could not. Real systems add per-segment checksums chained
   across records, or retain a footer, to tell the cases apart.
2. **`fsync` is called under the node mutex, one per mutation, with no group commit.**
   Heartbeats, votes and client requests on a node queue behind the disk. It is simple and
   obviously ordered, and it caps throughput at the fsync rate; batching several
   appends under one fsync (what the paper's pipelining and etcd's batching do) would
   be the first optimisation.
3. **Snapshots are taken on the apply path, under the lock, from a full serialization**
   of the state. Cost grows with state size and stalls the node. Real systems fork or
   copy-on-write (or snapshot incrementally) so the node keeps serving.
4. **Snapshots grow without bound** in this project: `txns_` and `recs_` (the 2PC
   transaction table and decision records) are never garbage-collected — a gap inherited from Project 13, but now it
   costs snapshot size and WAL rewrite time. Quantified in M3 above.
5. **InstallSnapshot is a single message** (whole state, 16 MiB frame cap). A real
   system chunks it. Past ~16 MiB of state this implementation cannot catch a follower
   up at all.
6. **No membership change** (see above), so disk loss is unrecoverable by design.
7. **The fatal-error path is exercised for one failure only.** It is forced for a failed WAL
   `fsync` (an injected `EIO`, `persist_demo` §6). Failures of `write`, of the snapshot's
   `fsync`/`rename`, of the directory `fsync`, and a full disk (`ENOSPC`) take the same
   `_exit` but were never forced. And the injection is a counter inside the node, not the
   kernel dropping dirty pages, so it shows that the node *stops*, not that a retry would
   have lost data.
8. **Directory-entry durability is argued, not tested.** The code `fsync`s the directory
   after creating the WAL and after each `rename`; no experiment can show a missing one
   matters, because the simulated power loss does not roll back directory operations.
9. **One filesystem, one kernel.** ext4 on a VM disk. Behaviour on XFS, btrfs, network
   filesystems or macOS (where `fsync` does not flush the drive's cache without `F_FULLFSYNC`)
   differs and is not covered.
10. **Clocks and the 150 ms tick** are as in Project 13; absolute latencies are not representative.

## Bugs and surprises found by verifying it

1. **SIGPIPE killed clients and nodes.** The first stress smoke test died with exit code
   141. `wire.h`'s `send()` had no `MSG_NOSIGNAL`, so writing to a connection a
   `kill -9`'d peer had reset raised SIGPIPE and terminated the process — a bug present
   since Project 1, hidden because earlier tests killed one node at a time and the
   timing rarely hit it. Cluster-wide outages hit it immediately. Fixed with
   `MSG_NOSIGNAL`.
2. **`kill -0` said dead nodes were alive.** In this sandbox PID 1 does not reap
   promptly, so a killed node whose parent script had exited lingered as a zombie, and the
   harness's liveness check (`kill -0`) kept succeeding on it. The first full run of the
   power-loss demo reported "node survived the power loss" for a node that was gone.
   `node_alive` now looks at the process state.
3. **The power-loss demo reported failure with every check passing.** `cut_all`'s last
   statement was the "is it still alive?" test whose *failure* is the good outcome, so the
   function returned non-zero when everything had worked. (Found by running the demo, reading
   the output, and noticing it said FAILED over a screen of `ok`.) Fixed with an explicit
   `return 0`.
4. **Four harness bugs in the first snapshot-demo run**, all of them tests that
   passed or failed for the wrong reason: `wait_converged` accepted "all nodes agree" before the
   restarted shard's new leader had committed anything (everyone agreed on the *snapshot's*
   state, which is not "caught up"); "same applied index after a restart" is wrong because a new
   leader appends a no-op, so the check is now `≥`; the leader-base check failed as a
   consequence of the first; and the crash counter matched an earlier crash line in a log
   that is appended across restarts (it now asserts a delta of exactly one).
5. **The fsync-failure test failed twice before it passed, for harness reasons.** The
   section ran after the vote experiment, which had left shard 1 stopped (so 17 of 60 accounts
   were "unreachable"); and with `RAFT_FAIL_FSYNC_AFTER=3` the follower simply did not perform
   four fsyncs in the one burst of writes, so it never failed — the "node stopped" check
   correctly said no. The test now writes until the node dies (and still fails if it never does).
6. **Acknowledged vs unacknowledged writes.** The first persist-demo run "lost" 23 of 60
   writes — which had never been acknowledged: the helper gave up on a key after 4 tries, which
   a leader election can outlast. A test of durability must only count what the system
   said yes to. `fill` now retries until every write is acknowledged.

Notice where these are: one in a networking helper that predates this project, one in the
environment, three in the test harness. None was in the new WAL/snapshot logic. That is a
statement about what the tests *found*, not a proof that the new logic has no bugs — the
gaps above are where I would look for them.

## How this relates to real systems

*(General knowledge, as far as I know — not checked against sources in this session,
and simplified.)*

* **etcd/raft and its WAL** — the same shape: a record-framed write-ahead log with CRCs, a
  separate snapshot file, the log is cut when a snapshot is taken, and the application
  `fsync`s ("Ready" → persist → send). etcd batches many entries per fsync and chains a
  CRC across records so a torn tail and mid-file damage can be told apart, and chunks
  snapshots; this project does none of that.
* **Raft paper §5.2/§7/§8** — the persistent-state list, snapshot, InstallSnapshot,
  and the rule that a leader may not count its own log toward a majority before it is durable
  are the paper's, implemented as stated.
* **"fsyncgate"** (the 2018 PostgreSQL discussion) — the reason a failed `fsync` is
  fatal here rather than retried.
* **LevelDB/RocksDB-style recovery** — "stop at the first bad record in the last log
  file" is the usual tolerance for a torn tail, and the same dilemma (is a bad record in the
  middle corruption or a crash?) is why those systems add a paranoid mode.
* **ARIES-style systems** are the other end of the design space (physical logging, LSNs
  on pages, group commit); the simplicity here comes from the state machine being replayed
  deterministically from a log.

## Things worth trying next

1. **Group commit:** one fsync per batch of appends and acknowledgements; measure
   throughput against the one-fsync-per-mutation baseline in M2.
2. **Chain the CRC** (each record's checksum covers the previous record's) and add a
   footer so a torn tail and a corrupted last record are distinguishable; then
   run the mid-log experiment with the damage in the *last* record.
3. **A real power-loss model**: replay a recorded write/fsync trace and drop or reorder
   unsynced writes (what dm-log-writes / CrashMonkey-style tools do) instead of
   truncating one file.
4. **Garbage-collect `txns_`/`recs_`** driven by participant acknowledgements, and watch M3's
   snapshot size go flat.
5. **Chunked, resumable InstallSnapshot**, and a test where the leader dies mid-transfer.
6. **Membership change** (joint consensus or single-server changes) so a node that lost its
   disk can be replaced instead of wedging the cluster.
7. **Background snapshots** (fork / copy-on-write) so the apply path does not stall.
8. **Fault injection at the syscall level** (`EIO` on `fsync`, `ENOSPC` on `write`) to
   exercise the fatal-error path properly rather than via a resource limit.
