# ANALYSIS — consistent hashing, live rebalancing, and a test harness that lied

This project has two halves. The first is routine and worked as expected: a
consistent-hash ring spreads keys across independent Raft groups, and a shard
failure stays inside its shard. The second is the interesting one: **adding a
shard while clients keep reading and writing**. That needs a migration
protocol, and a protocol like that is only worth anything if a test can show it
fails when a step is removed. Most of this document is about that, and about
the one time the test said something false.

## Headline results

| Claim | Result | Evidence |
|---|---|---|
| Ring balance, 3 shards, 128 vnodes | heaviest shard 1.07x its fair share, CV 5.8% | `evidence/01` |
| Ring balance, 10 shards, 128 vnodes | heaviest shard 1.15x, CV 8.6% | `evidence/01` |
| Keys moved when adding a 4th shard | **25.02%** (ideal 25.00%); 0% moved between existing shards; removing the shard is the exact inverse | `evidence/01` |
| Same change with `hash(key) % N` | **74.97%** moved | `evidence/01` |
| Value of the hash finalizer | plain FNV-1a: CV 0.41, heaviest shard 1.50x; with fmix64: CV 0.07, 1.08x (3 shards, 128 vnodes) | `evidence/07` |
| Placement | 1500 keys, 502 / 493 / 505 per shard, every key on its ring owner, none duplicated | `evidence/02` |
| Fault isolation | leader of shard 2 `kill -9`'d: shards 1 and 3 had 0 failures in ~138 requests each; shard 2 out 1.62-2.62 s, recovered, 1500/1500 keys intact; 3/3 runs | `evidence/03` |
| Quiet rebalance 3 → 4 shards | migrator moved 425 keys; ring predicted 425; census and verify pass | `evidence/04` |
| Live rebalance, safe protocol | **0 violations in 10/10 runs** (7,104 checked reads, 2,751 checked CASes, 313 key moves) | `evidence/05` |
| Live rebalance, each step removed | **5/5 runs** show violations, each with its signature symptom | `evidence/05` |

## Design

### Shards are independent Raft groups

Each shard is a 3-node Raft group running the unchanged Project 11 `raft_node`.
Shards never talk to each other. That is the whole point of sharding for
availability: a shard's leader election, log, and read-index machinery involve
only its own three nodes, so a failure in one cannot slow another (measured:
`evidence/03`). It is also the limit of the design — there are **no
cross-shard transactions**, so an operation touching two keys on different
shards is not atomic.

Two state-machine commands were added to the Raft KV for this project:

- `KEYS` — a linearizable (read-index) listing of every key in the shard. The
  migrator needs it to know what to move, and the census needs it to check
  placement without going through the router.
- `CAD key expected` — compare-and-**delete**. The migrator deletes the old
  copy only if it is still exactly the value it read, so it can never destroy
  something it did not copy. (`expected` is the rest of the line and may
  contain spaces; empty means "absent".)

### The ring

`HashRing` places each shard at `vnodes` pseudo-random points on a 64-bit
circle; a key belongs to the first point at or after its hash, wrapping. The
ring is a **pure function** of (shard set, vnodes, salt) and is rebuilt from
scratch on every change, never mutated incrementally. That matters because
every client computes the ring independently from `ring.conf`; if insertion
order could change an answer, two clients would disagree about where a key
lives.

Hash: FNV-1a 64-bit followed by MurmurHash3's `fmix64` finalizer. The
finalizer is not decoration. Vnode labels like `shard-1#17` and `shard-1#18`
differ in one trailing character, and FNV-1a alone mixes that poorly, so
vnode positions clump. Measured (`evidence/07`, 20 rings x 100,000 keys):

| 128 vnodes | CV, FNV-1a only | CV, with fmix64 | heaviest shard, FNV-1a only | with fmix64 |
|---|---|---|---|---|
| 3 shards | 0.41 | 0.07 | 1.50x | 1.08x |
| 10 shards | 0.55 | 0.09 | 2.19x | 1.13x |

(The 3-shard fmix64 figure here, 0.072, differs from `evidence/01`'s 0.058: both
are averages over 20 random rings of a statistic computed from only 3 numbers,
so they are noisy at that level. The 6x gap is not.)

Balance against theory: the usual rule of thumb is that the load CV falls like
`1/sqrt(vnodes)`. Measured, it tracks that rule closely at 128 vnodes for 10
shards (0.086 vs 0.088) and sits below it for 3 shards (0.058 vs 0.088). I did
not try to derive why; the table in `evidence/01` shows the whole range.
128 vnodes is a compromise: more vnodes means better balance but a bigger ring
to build and search; 1024 vnodes cuts the 3-shard CV from 5.8% to 2.3% at 8x
the ring size.

**Movement.** Adding shard N+1 should move the minimum possible, 1/(N+1) of
the keys, all of them *to* the new shard. Measured for N = 2, 3, 5, 9, 19:
33.11%, 25.02%, 16.49%, 9.88%, 4.93% moved versus ideal 33.33, 25.00, 16.67,
10.00, 5.00 — and in every case **0.00%** of the moved keys went to a shard
other than the new one. Removing a shard restores the exact previous mapping.
With `hash % N` the same change moves 66% to 95% of the keys, which at
database scale means a rebalance that touches nearly all the data.

## Live migration

### The problem

When shard 4 joins, about a quarter of the keys change owner. Each has an old
owner **A** and a new owner **B**. During the move three kinds of actor are
active at once: clients (reading and writing), the migrator (copying A → B and
deleting from A), and — in a real system — clients that have not yet heard
about the change. Any naive scheme loses to some interleaving of these. The
simplest, "switch every client to the new ring and then copy", fails
immediately: a client reads B before the key has arrived and sees the key as
absent. (That is the `naive` variant; it fails in every run.)

### The protocol

Precondition (see "No epoch fencing" below): every client has adopted the
*migrating* map before the migrator starts, and none still routes by the old
ring. In this project the test enforces that with an explicit drain step.

Per-operation rules for a **moving** key (A ≠ B). Non-moving keys always go to
their single owner.

| Op | Rule | Why |
|---|---|---|
| GET | read B; if B holds anything (a value **or a tombstone**), that is the answer. Else read A, **then read B again**: if B now holds something use it, else use A's value. | B is authoritative whenever non-empty. The second read of B closes a race with the migrator (below). |
| SET | write B only. | B is now authoritative and hides A's stale copy; because the migrator creates on B only if B is empty, it can never overwrite this. |
| DEL | write a **tombstone** (`__TOMBSTONE__`) to B, **then** delete A's copy. | The tombstone stops the migrator from resurrecting the key by copying A's old value into B afterwards. |
| CAS | first `moveKey(A → B)`, then compare-and-swap on B. | The compare must be against the authoritative value, which lives on B only once the key has been moved. |
| migrator `moveKey` | read A; create-if-absent on B (`CAS key "" value`); then `CAD A value`. | Create-if-absent never clobbers a newer client write or a tombstone. CAD never deletes a value it did not copy. Whether the create succeeded or B already had something newer, A's copy is now redundant. |

After the migrator finishes, a cleanup pass removes leftover tombstones
(`CAD(B, key, tombstone)`), and only then is the map flipped to *stable-new*.
Clients also treat a leftover tombstone as "absent" in stable mode, so a
straggler cannot leak one to a reader.

### Why each step exists — and the experiment that says so

Each protocol step was removed in turn, and the stress test run against the
result. This is the only reason to believe the step is needed, as opposed to
merely plausible.

| Step removed | Failure it should allow | Variant | Observed (5 runs, `evidence/05`) |
|---|---|---|---|
| second read of B in GET | migrator copies A → B and deletes A between the reader's B read and A read → reader sees **nothing** for a key that exists | `ablate-norecheck` | 21 **vanished** reads; 0 stale, 0 resurrected, 0 cas-wrong |
| tombstone on DEL | migrator read A, then client deletes both copies, then migrator creates B from its stale read → the deleted key **comes back** | `ablate-notombstone` | 6 **resurrected** reads (+5 cas-wrong); 0 vanished, 0 stale |
| create-if-absent copy | migrator overwrites B with an old A value after a client has written a newer one → reader sees an **older** value | `ablate-overwrite` | 25 **stale** reads (+11 cas-wrong, 5 resurrected) |
| everything (route by new ring, plain copy) | all of the above plus unmoved keys looking absent | `naive` | 43 vanished, 31 stale, 2 resurrected, 25 cas-wrong |
| none (the real protocol) | — | `safe-natural`, `safe-hostile` | **0 in all 10 runs** |

The three single-step signatures are clean: each removed step produces its
predicted read symptom, and (for `norecheck`, `notombstone`) none of the other
read symptoms; `notombstone` also produces some wrong CAS results. The
exception worth understanding is `overwrite`: it also produces 5 resurrected
reads, because a plain `SET` copy clobbers a *tombstone* exactly as it clobbers
a newer value — the same mistake, caught at a second place.

Evidence that the safe runs were not trivially easy: in the safe runs, 102 of
313 key moves (32 of 101 and 70 of 212) found that the client had already
written something newer to the destination, and the migrator correctly left it
alone. Those are exactly the interleavings the protocol exists to resolve.

### The B-A-B read, in detail

The least obvious step. The migrator writes B *before* it deletes A (that
order means a crash mid-move leaves two copies rather than none — an argument
from the design; I did not test a migrator crash). A reader that looks at B
(empty), then A, can be interleaved with the migrator like this:

```
reader:   GET B  -> empty
migrator:                 CAS-create B = x ; CAD A
reader:                                      GET A -> empty      (already deleted)
reader returns "absent"   <- wrong, the key exists on B
```

Reading B once more after A cannot miss: if A was already deleted when the
reader looked, B was written before that delete, so B is already populated by
the time of the second read.

## The stress test

`shardtool stress` (driven by `scripts/rebalance_stress.sh`):

- **Model.** Key `i` is owned by worker `i % W`; only that worker ever writes
  it. So the worker always knows the exact value its last *acknowledged*
  operation left, and every GET must return exactly that. A CAS is issued
  half the time with the correct expected value (must return `OK` and change
  the value) and 40% of the remainder with a deliberately wrong one (must
  return `MISMATCH` and change nothing).
- **Uncertainty.** If an operation fails (timeout, no leader), the key becomes
  "uncertain" — it may or may not have applied — and is not checked until the
  next acknowledged write. In the 30 runs in `evidence/05` every run reported
  `failed-ops=0`, so nothing was excused this way.
- **Phases.** Warm-up on the 3-shard ring; flip to *migrating*; **drain**
  (wait until no worker is inside an operation that started under the old
  map); migrate with 4 threads; flip to *stable-new*; settle with traffic
  still running; final quiescent audit.
- **Violation categories.** `vanished` (read says absent, key exists),
  `resurrected` (read returns data for a deleted key), `stale` (read returns a
  different value), `cas-wrong`, `final-state-mismatch`, and
  `stray-copies-on-old-shards` (raw check that nothing is left on a shard that
  no longer owns the key).
- **Reset.** Every run first deletes all `sk-*` keys on every shard and
  refuses to run if any survive (see bug H4).

### Why the race-widening hooks exist

At natural timing the narrow races are hit rarely, and an experiment that
rarely fails shows nothing. Measured with the default workload (100 keys, 5
seeds each, after the harness fix): `naive` was caught 5/5, `overwrite` 4/5,
but `norecheck` and `notombstone` **0/5** — the windows exist but this workload
doesn't land in them. So the stress test has two test-only options,
`--read-gap-ms` and `--step-delay-ms`, which insert a sleep at exactly the two
named points (between a GET's read of B and its read of A; between the
migrator's read of A and its write to B). They never change *what* the protocol
does, only *when*, and the **safe protocol is also run under hostile timing** so the comparison is
fair: `safe-hostile` uses *both* hooks at their largest settings (200 keys, 600
ms read gap, 300 ms step delay), so it is at least as hostile as any single
ablation run (each ablation uses only the hook that targets its own window).
Results: safe 0/5, `norecheck` 5/5, `notombstone` 5/5. The settings took a few
tries: `norecheck` was caught in 3 of 5 runs with a 200 ms gap, 2 of 5 with a
300 ms gap and 200 keys, and 5 of 5 with a 600 ms gap; `notombstone` in 3 of 5
with a 200 ms step delay and 100 keys, and 5 of 5 with 300 ms and 200 keys. I
kept widening until the failure was reliable rather than stopping at the first
setting that produced one — and the control was run at the widest setting.

### What the stress test does *not* check

- **Concurrent writers to the same key.** Single-writer-per-key is what makes
  the model exact; it also means linearizability *between* clients is not
  tested here. (Within a shard, Raft plus read-index gives it — Project 11.)
- **Faults during migration.** No node is killed while a rebalance runs. A
  migrator that crashes mid-move, a shard that loses its leader mid-copy —
  not exercised. `moveKey` is written to be idempotent and safe to re-run, and
  the migrator retries failed moves up to 4 times, but I did not test
  crash-and-resume.
- **Clients holding the old map.** The drain step *assumes away* the hardest
  real-world problem; see below.
- **Scale.** 100-200 keys, 16 workers, 3 → 4 shards, 5 seeds per variant. Zero
  violations in 10 safe runs is evidence, not proof; the ablations show the
  test can see three specific classes of bug, not every class.

## What's real and what's simulated

| | |
|---|---|
| Real | 12 OS processes (4 shards x 3 Raft nodes); real TCP on loopback; real Raft elections, replication and read-index reads; real `kill -9`; real concurrent clients and migrator threads; real data placement, checked by asking each shard directly |
| In-process stand-in | "every client has adopted the new shard map" — the stress test flips an in-memory `RingState` shared by all worker threads, then waits for in-flight operations (the drain). A real deployment needs a distribution mechanism and a fence (next section) |
| Test-only hooks | `--read-gap-ms`, `--step-delay-ms` (sleeps at two points), `Mode::Naive` and the `Ablation` bits (deliberately broken protocol variants) |
| Absent | control plane, epoch fencing, persistence, log compaction, in-shard membership change, client sessions / request dedup, throughput measurement |

## Known gaps (not hidden, not fixed)

1. **No epoch fencing.** Safety requires every client to adopt the migrating
   map *before* the migrator starts and to switch to stable-new only *after* it
   finishes. Here that is true by construction (one process, an explicit
   drain). With independent clients and a file for a shard map it would not be:
   a client still on the old map keeps writing to A, and a key written to A
   after the migrator has moved it is silently orphaned. The fix is a versioned
   shard map held in a strongly-consistent place (typically its own Raft
   group) and shards that reject requests carrying a stale epoch. That is a
   project in itself.
2. **No persistence or log compaction.** Restarting a node loses its state;
   the cluster survives only while a majority of each shard is up. (This is
   also why restarting shard 4 gives a clean slate — see H4.)
3. **No client session / request deduplication.** A request that times out and
   is retried can apply twice. For SET this is harmless; for CAS it can make a
   retried CAS return `MISMATCH` after it actually succeeded.
4. **Tombstones can straggle.** A DEL on a moving key that lands after the
   tombstone-cleanup pass leaves a tombstone behind. Readers treat it as
   absent and the next write replaces it, so it is cosmetic, but it is
   storage not reclaimed. The stress runs reported `tombstones-left-behind=0`.
5. **Value/key restrictions** forced by the space-delimited wire format: no
   empty values (indistinguishable from absent), no spaces or newlines in keys,
   no spaces in a CAS `expected`, and the string `__TOMBSTONE__` is reserved.
   All are rejected with an explanatory error rather than mangled.
6. **Detached straggler threads.** Project 11's read-index confirmation
   returns on majority and leaves slower RPCs on detached threads that capture
   `this`; safe only because nodes live for the process lifetime.
7. **Throughput scaling is not measured.** The sandbox has 2 CPUs for 12
   processes, and Raft writes here commit on 150 ms leader ticks, so the
   cluster is latency-bound. A throughput-vs-shards curve measured here would
   show scheduler contention, not sharding. The rebalance took 8.0 s for 425
   keys in `evidence/04`; that is a property of the 150 ms commit tick, not a
   claim about migration speed.
8. **The partition simulation from Project 11** (a flag that drops peer RPCs)
   is not used here; faults in this project are real process kills.

## Bugs found by running it

### H4 — harness bug: the "empty" new shard was not empty, and the safe protocol looked broken

The first live-traffic run of the safe protocol (seed 1) was **clean**. I
then ran seeds 2-9 as a quick sanity pass and **all eight reported
violations** — 6 to 22 stale reads each, plus resurrected reads, wrong CAS
results, and in two runs a final-state mismatch. The tempting reading was
"the protocol has a race". The Naive and ablation runs I had done just before
(also contaminated, as it turned out) had all shown stale reads too, which
looked like corroboration.

What distinguished the two readings was the forensic log I added to the
harness (key, operation, phase, expected vs. got, raw per-shard values). Every
bad read had the same shape:

```
STALE key=sk-36 expected="v4-9" got="v4-38"  raw[ s2="v4-9" s4="v4-38" ]
STALE key=sk-42 expected="v10-13" got="v10-31"
```

The value returned by the **new** shard always carried a *later* operation
counter than the model's last acknowledged write — data the model could not
have written. Not a race: leftovers. Shard 4 never resets, so it still held the
previous run's final values; the protocol treats a non-empty B as authoritative
(correctly) and returned them. Seed 1 was clean only because shard 4 had just
been started. `kvctl ... KEYS` on shard 4 listed 16 leftover keys; restarting
shard 4 and re-running seed 2 went from 22 violations to **0**, which confirmed
the diagnosis rather than just supporting it.

Fix: `stress` now deletes every `sk-*` key on every shard before preloading and
exits 2 if any survive. Re-run after the fix, back to back with **no**
restarts (each run cleaning up ~70-80 leftovers): safe 8/8 clean.

Two lessons, both already taught by earlier projects. (1) A single passing
run proved nothing — the first run was lucky, and running eight more is what
exposed the problem. (2) The same defect would have been invisible in a
harness whose checks could not fail; here it surfaced because they could. All
Naive and ablation numbers taken before the fix were discarded; `evidence/05`
is from the fixed harness (`evidence/06` records the contaminated run).

### My first predicted signature table was wrong where it mattered

Before measuring, I had planned for each ablation to produce a specific
symptom (`norecheck` → vanished, `notombstone` → resurrected, `overwrite` →
stale). The contaminated runs showed *stale* reads for all of them, which
looked like a refutation of that mapping. It was the leftover data again:
every variant inherited the same contamination, so they all reported the same
symptom. After the fix, with the windows widened, the original mapping held —
including the extra detail that `overwrite` also resurrects (it clobbers
tombstones too), which I had not predicted.

### Unmeasured claim in a code comment

`hash_ring.h` said plain FNV-1a "produces visibly clumped vnode positions". I
had not measured that when I wrote it. Measured now (`evidence/07`): true, by a
wide margin (CV 0.41 vs 0.07). Kept, now with evidence behind it, and the
measurement is its own build target (`hash_ablation`).

### Minor

- `scripts/stop_cluster.sh` was copied over from Project 11 and referenced a
  script that does not exist here; removed.
- Natural-timing ablations that never fail (0/5 for two of them) were treated
  as a finding about the test, not about the protocol: see "race-widening
  hooks" above.

### What the stress test did *not* find

No bug in the safe protocol itself. I cannot tell from that whether there is
none or whether the test cannot see it; the ablations establish that it can see
vanished, resurrected and stale reads and wrong CAS results under the injected
timing, and the "does not check" list above says what it cannot see.

## How this relates to real systems

- **Redis Cluster** shards by 16,384 fixed hash *slots* rather than a ring, and
  migrates slot by slot; its `ASK` redirection sends a client to the importing
  node for a key the source no longer holds. That is the same two-owner lookup
  as here, but with the source consulted first and the server, not the client,
  doing the redirecting.
- **Dynamo / Cassandra** use a ring with virtual nodes as here, but replicate
  each key to the next N ring positions and handle membership through gossip;
  this project instead replicates *within* a shard with Raft and puts one owner
  per key on the ring.
- **Range-sharded systems** (Spanner, CockroachDB) split by key range rather
  than hash, which makes range scans efficient at the cost of hot-spotting on
  sequential keys; hashing gives balance and makes a range scan a
  scatter-gather over every shard.

These are comparisons of approach, from general knowledge of those systems,
not something I measured or checked against their current documentation.

## Things worth trying next

- Fence the shard map with an epoch stored in its own Raft group, and have
  shards reject stale-epoch requests (gap 1).
- Kill a shard leader and the migrator *during* a rebalance, restart the
  migrator, and check that the same stress model still holds.
- Add concurrent writers per key and check linearizability with a history
  checker rather than a per-key model.
- Persist the Raft log so a node restart is not data loss.
