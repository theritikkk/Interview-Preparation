# Cross-shard transactions (two-phase commit over Raft groups)

Project 12's sharded KV store, extended so that **one transaction can read-check
and write keys that live on different shards, atomically**. Each shard is an
independent 3-node Raft group; a client-side library runs **two-phase commit**
across the shards, with the one thing 2PC usually gets wrong — *the decision
must survive the coordinator* — handled by storing the commit/abort decision in
a **Raft-replicated record**, so any client can finish a transaction whose
coordinator died.

The main result is a **bank-transfer stress test** that checks every audit and
the final balances *exactly* — with the real protocol (including a `kill -9` of
a shard's Raft leader mid-run) and with four deliberately broken variants that
the same test catches. A set of demos exercises each coordinator-crash case
deterministically.

9 OS processes (3 shards x 3 Raft nodes), plain TCP on loopback, C++17, no
dependencies beyond the standard library and POSIX sockets. See
[ANALYSIS.md](ANALYSIS.md) for the protocol, the serializability argument, the
measurements, what is real versus simulated, what is *not* covered, and the
bugs found along the way.

## What it demonstrates

| Question | Answer, and where to see it |
|---|---|
| Does a transaction spanning 3 shards commit atomically? | Yes — one `TXN` over keys on shards 1, 2 and 3 applies all writes or none (`evidence/01`). |
| Does a failed compare leave any trace? | No: a stale `-c` aborts the whole transaction; every key is unchanged and no lock is left behind (`evidence/01`). |
| Is the single-shard case cheaper? | Yes: it skips 2PC and is one log entry — ~148 ms vs ~590 ms for a cross-shard commit at this project's 150 ms Raft tick (`evidence/03`). |
| What if the coordinator dies after PREPARE (no decision yet)? | Locks stay held, plain `SET`/`GET` on those keys are refused, a reader waits out the lock ttl and rolls it **back** (`evidence/02`, scenario A). |
| …after the decision is recorded but before any participant is told? | A reader finds the COMMITTED record and rolls it **forward** (scenario B). |
| …midway through delivering COMMIT (one shard committed, one not)? | A reader rolls the other shard **forward** to match; money is conserved (scenario C). |
| Is the decision record load-bearing? | Ablation: a resolver that has no record and guesses ABORT produces alice=100, carol=80 — **atomicity broken, 180 instead of 150** (scenario D). |
| Is aborting a *live* slow coordinator safe? | Yes: the late coordinator learns it was fenced and reports "aborted"; nothing is changed (scenario E). The fence is also checked directly (`evidence/05`). |
| Does the whole thing hold up under concurrent load? | Bank stress, 5 seeds x 20 s, 8 workers, 12 accounts: **0 violations in 5/5 runs**; with a shard leader `kill -9`'d mid-run, **0 violations in 15/15 runs** (5 seeds rotating the victim shard, plus 10 seeds always killing shard 2) (`evidence/03`, `evidence/04`). |
| Does the test actually detect broken transactions? | Yes: no transactions at all (`naive`), unvalidated audit reads (`plain-audit`), and server-side locks disabled (`ablate-nolock`) are each caught **5/5**, each with its own signature symptom (`evidence/03`). |

## Layout

```
include/
  txn_client.h       TxnClient: commit (single-shard fast path or 2PC), get/set that help
                     with locks, snapshotRead, resolve, sweep; Txn builder; Options
  raft_node.h        Raft + KV state machine (Project 12's, + lock table, staged writes,
                     decision records — all replicated through the log)
  sharded_client.h   ShardedClient (ring router; Project 12's migration code is unused here)
  hash_ring.h  kv_client.h  wire.h
src/
  raft_node.cpp      Raft + the TX* state machine commands (TXONE, TXPREP, TXCOMMIT,
                     TXABORT, TXBEGIN, TXDECIDE, TXPENDING) and lock checks on plain ops
  txn_client.cpp     coordinator logic, recovery, snapshot reads
  txnctl.cpp         CLI: GET / SET / TXN / WHERE / PENDING / SWEEP / RESOLVE / ENCODE / RAW
  txntool.cpp        bank workload: init / balances / transfer / bank (the stress test)
  main.cpp  kv_client.cpp  sharded_client.cpp  kvctl.cpp  direct_probe.cpp   (earlier projects)
ring.conf            the shard map (3 shards, 128 vnodes)
shards/shardN.conf   membership of shard N's Raft group (3 nodes each)
scripts/
  lib.sh                    shared check helpers
  run_shards.sh  stop_shards.sh   start / stop shards (default 1 2 3)
  txn_demo.sh               functional demo + protocol-level checks (idempotency, fences, ...)
  crash_recovery_demo.sh    five coordinator-failure scenarios incl. a no-decision-record ablation
  fence_ablation.sh         the abort fence on vs off
  bank_stress.sh            the main experiment: real protocol vs broken variants, with leader kill
evidence/            captured output from verified runs:
  01 functional demo (3 runs)        02 crash-recovery demo (scenarios A-E)
  03 bank stress, 5 seeds x 5 variants + per-seed logs   03-...-first-run-summary  an earlier complete run
  04 leader-kill on shard 2, 10 seeds   05 abort fence on vs off   06 all-12-account audit starvation
```

## Build

```
cmake -B build -S .
cmake --build build -j2
```

Build and run in a normal local directory, not a FUSE-mounted delivery folder:
those often can't keep the executable bit, so fresh binaries fail with
"Permission denied". Copy the project somewhere local first if you hit that.

## The transaction model

A transaction is a set of **compares** and a set of **writes** over any keys:

* *compare*: `key must currently equal V` (`V` empty means "must be absent")
* *write*: set `key` to a value, or erase it (empty value)

It commits only if **all** compares hold at the commit point, and then applies
**all** writes; otherwise it applies nothing. A read-modify-write such as a bank
transfer is "compare both balances against what I read, write both new
balances" — optimistic concurrency, validated at commit. (This is the
mini-transaction shape from Sinfonia; see ANALYSIS.md for why it was chosen.)

Values must be non-empty and must not begin with byte `0x01` (that marker
means "locked" on the wire); keys may not contain spaces or newlines. These are
rejected with a message rather than silently mangled.

## Run

### 1. Start shards and use them

```
./scripts/run_shards.sh                         # shards 1 2 3 (9 nodes), waits for leaders
./build/txnctl ring.conf SET alice 100
./build/txnctl ring.conf SET carol 50
./build/txnctl ring.conf WHERE alice            # which shard owns it
./build/txnctl ring.conf GET alice
# move 30 from alice to carol, but only if both still hold what we think:
./build/txnctl ring.conf TXN -c alice=100 -c carol=50 -w alice=70 -w carol=80
echo $?                                         # 0 committed, 3 aborted, 4 indeterminate
./build/txnctl ring.conf PENDING                # prepared-but-unfinished txns (should be 0)
./scripts/stop_shards.sh
```

Other verbs: `SWEEP [minAgeMs]` (resolve stuck transactions older than that),
`RESOLVE <txid> [shard]`, `ENCODE` (hex-encode a transaction body — the box has
no `xxd`), and `RAW <shard> <READ|WRITE> <command...>` to talk to one shard's
state machine directly (used by the protocol-level checks in `txn_demo.sh`).
Options: `--lock-ttl-ms`, `--crash-at after-begin|after-prepare|after-decide|mid-commit`
(kill *this* process at that point), `--recovery guess-abort` (the ablation),
`--pause-after-prepare-ms`.

### 2. Functional demo (~30 s)

```
./scripts/txn_demo.sh              # prints TXN DEMO PASSED / FAILED
```

Placement of four keys on three shards; an atomic 3-shard commit; a mismatch
that aborts everything; the single-shard fast path; create-if-absent and erase;
a refused bad key; then protocol-level checks straight at a shard's state
machine: PREPARE and COMMIT retried (a plain `SET` between a COMMIT and its
retry proves the retry is not re-applied), `ERR_COMMITTED` / `ERR_UNKNOWN` /
`ERR_ABORTED` answers, first-decision-wins on the decision record, and the
abort fence.

### 3. Coordinator crash recovery (~35 s)

```
./scripts/crash_recovery_demo.sh   # prints CRASH RECOVERY DEMO PASSED / FAILED
```

A transaction across shards 2 and 3 is run by a client process that `_exit(86)`s
at a chosen point (after PREPARE, after the decision, mid-commit), and a
*different* client then finds the wreckage and finishes it. Scenario D repeats
the mid-commit crash with a resolver that has no decision record and must
guess — and shows atomicity breaking. Scenario E aborts a live, merely slow
coordinator and shows that it is fenced out rather than corrupting anything.

### 4. The abort fence on vs off (~45 s)

```
./scripts/fence_ablation.sh        # prints FENCE ABLATION PASSED / FAILED
```

Restarts the cluster twice. A resolver aborts a transaction before the
coordinator's PREPARE reaches a participant. With the fence, the late PREPARE
is refused and a reader is not delayed (5 ms). With `RAFT_TXN_ABLATE=nofence`
the participant accepts it, locks the key for an already-aborted transaction,
and the next reader stalls for the full lock ttl (1.5 s) until it cleans up.

### 5. The bank stress test (the main experiment)

```
./scripts/bank_stress.sh           # ~12 min with the default 5 seeds
./scripts/bank_stress.sh 1 2       # quicker: two seeds, ~5.5 min
ONLY=safe-leaderkill KILL_SHARD=2 ./scripts/bank_stress.sh 11 12 13   # one variant, one victim
```

Each run is 20 s. 12 accounts (`acct-0`…`acct-11`) start at 1000 each, in 4
*groups*; 8 worker threads pick random transfers *within a group* (so some are
single-shard and some are cross-shard), one auditor thread per group
repeatedly reads its group's balances. Afterwards the tool resolves anything
left in doubt, then computes each account's **exact** expected balance from the
transfers it saw commit and compares. Checked, and counted as violations:

| Violation | Meaning |
|---|---|
| `audit-skew` | an audit's balances did not sum to the group's invariant total |
| `negative-balance` | a balance went below zero |
| `final-total` / `final-per-account` | the end state differs from the exact expected state |
| `stuck-locks` / `unresolved-txns` | a lock or prepared txn survived the end-of-run cleanup |
| `protocol` | the server answered a client in a way the protocol forbids (e.g. told to COMMIT something already aborted) |

Variants:

| Variant | What it is | Expected |
|---|---|---|
| `safe` | the real protocol | 0 violations |
| `safe-leaderkill` | the real protocol; `kill -9` a shard's Raft leader at t=7 s (asserts the client actually saw the fault) | 0 violations |
| `naive` | no transactions: two plain `SET`s | audit skew **and** wrong final state |
| `plain-audit` | real transfers, but auditors read without validation | audit skew only; final state exact |
| `ablate-nolock` | servers started with `RAFT_TXN_ABLATE=nolock` (no key locks) | audit skew and wrong final state |

The script exits non-zero if the real protocol shows any violation, if any
broken variant is *not* caught, or if a variant's signature symptom is absent.
Per-run output is in `logs/bank/`.

Captured result (seeds 1-5; violations summed over the five runs):

```
safe               0/5 runs w/ violations | audit-skew=0    final-total=0  final-per-account=0
safe-leaderkill    0/5 runs w/ violations | audit-skew=0    final-total=0  final-per-account=0
naive              5/5 runs w/ violations | audit-skew=89   final-total=5  final-per-account=60
plain-audit        5/5 runs w/ violations | audit-skew=37   final-total=0  final-per-account=0
ablate-nolock      5/5 runs w/ violations | audit-skew=371  final-total=5  final-per-account=60
```

Plus 10 more leader-kill runs, always killing shard 2's leader (seeds 11-20):
0/10 with violations (`evidence/04`). `evidence/03` also has per-variant
aggregates (commits, aborts, audits, latencies); runs are not deterministic, so
a re-run gives different counts and the same pattern (`evidence/03-bank-stress-first-run-summary.txt`
is an earlier complete run). `evidence/06` measures why the audits are per
group: auditing all 12 accounts in one snapshot completed 1-2 audits per 20 s.

Custom runs: `./build/txntool bank ring.conf --seconds 30 --workers 16 --accounts 12 --groups 4 --seed 7`
(also `--initial`, `--max-amount`, `--audit-ms`, `--attempts`, `--lock-ttl-ms`,
`--naive`, `--plain-audit`). Start a cluster first (`scripts/run_shards.sh`);
`txntool init` resets the accounts and resolves leftovers from earlier runs.

## How this was verified

Everything above was run, not just written: each demo several times in the
working tree (functional and crash-recovery demos 3/3, fence ablation 3/3), the
full 5-seed bank experiment, and the extra 10-seed leader-kill run — and then,
from a fresh extract of this very zip built with the commands above,
`txn_demo.sh` (PASSED, 28 s), `crash_recovery_demo.sh` (PASSED, 35 s),
`fence_ablation.sh` (PASSED, 44 s) and `bank_stress.sh 1 2` (PASSED, 337 s:
the real protocol 0 violations in 4 runs, one with a leader killed; all three
broken variants caught in 2/2 runs). No node processes were left running. The ablations exist because a check that has never been
seen to fail has not been shown able to detect failure.

## What is and isn't real

Real: 9 separate processes, real TCP, real Raft, real `kill -9` of leaders and
of a coordinator process, real concurrent clients, real lock conflicts and
aborts. Simulated or absent: no persistence or log compaction (a restarted node
loses its state), decision records and abort fences are never garbage
collected, the client library is the coordinator (there is no coordinator
service), every Raft write takes ~125 ms because of the 150 ms tick (so
absolute latencies are not representative), and no-wait locking starves large
read sets. ANALYSIS.md has the full list and why each matters.
