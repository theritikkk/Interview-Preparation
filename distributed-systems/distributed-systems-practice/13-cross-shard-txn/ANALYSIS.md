# ANALYSIS — cross-shard transactions, a decision record that survives its coordinator, and which mechanism each experiment proves necessary

Project 12 split a keyspace across independent Raft groups. Its limit was
stated there: an operation touches **one key on one shard**. This project adds
the next thing a real sharded store needs — a transaction that checks and
writes keys on **several shards atomically** — and then spends most of its
effort on the question that matters for a protocol like this: *which of its
parts is actually necessary, and how do I know?* Every mechanism below has an
experiment that removes it and watches something specific break.

## Headline results

| Claim | Result | Evidence |
|---|---|---|
| A transaction over keys on 3 shards commits atomically; a stale compare aborts all of it with no trace | Demo: 56 checks, 0 failures, 3/3 runs | `evidence/01` |
| Single-shard transactions take a one-log-entry fast path | ~148 ms vs ~590 ms for 2PC (p50) | `evidence/03` |
| Coordinator dies after PREPARE / after the decision / mid-commit: another client finishes the job | Rolled back / forward / forward, money conserved; 31 checks, 0 failures, 3/3 runs | `evidence/02` |
| The decision record is load-bearing for **atomicity** | Resolver without it guesses ABORT after a mid-commit crash: alice=100, carol=80, total **180 instead of 150** | `evidence/02`, scenario D |
| The abort fence is load-bearing for **liveness**, not safety | Fence on: late PREPARE refused, read returns in ~5 ms. Fence off: key locked for an already-aborted txn, read stalls the full 1.5 s ttl. 3/3 runs | `evidence/05` |
| Under concurrent load the real protocol is exactly correct | Bank stress 0 violations in 5/5 runs (440 committed transfers, 267 of them 2PC; 206 consistent audits, 0 skewed) | `evidence/03` |
| ...including with a shard's Raft leader `kill -9`'d mid-run | 0 violations in 15/15 runs (5 rotating victims + 10 always shard 2) | `evidence/03`, `evidence/04` |
| The test can detect broken transactions | `naive` (no txns), `plain-audit` (unvalidated reads), `ablate-nolock` (locks off): each caught 5/5 with its own signature | `evidence/03` |
| Correctness costs throughput | The no-transaction variant "commits" about 6x as many transfers (2,724 vs 440 over five 20 s runs) — and ends with wrong balances in every account in every run | `evidence/03` |

What this does **not** show is listed under [Known gaps](#known-gaps-not-hidden-not-fixed). The two that
matter most: the leader-kill runs almost never left a transaction in doubt
(coordinator-crash *recovery* is exercised by the deterministic demos, not by the
stress test), and the stress test's final-state check cannot see ordering
anomalies that leave the sums unchanged.

## Design

### The transaction model: mini-transactions

A transaction is two lists:

* **compare items** — `key must currently equal V` (`V = ""` means absent)
* **write items** — set `key` to a value, or erase it (`""`)

It commits iff every compare holds *at the commit point*, and then applies all
writes. A read-modify-write is expressed as optimistic concurrency: read the
values, then commit "compare everything I read, write the new values". This is
the *mini-transaction* shape from Sinfonia, chosen because it needs no
interactive sessions or server-side transaction state beyond what must be
durable anyway: the whole transaction is described up front, so a recovering
party never needs to ask the original client what it meant to do. The price is
that every read that matters must be repeated as a compare, and a conflict
costs a full retry rather than a lock wait.

### Where state lives

Everything is replicated through each shard's Raft log, and changed **only** by
`applyToStateMachine()`, so all three replicas of a shard hold identical copies
and a newly elected leader inherits it. Per shard:

| State | Meaning |
|---|---|
| `kv_` | the data |
| `locks_` | key → txid of the prepared transaction holding it |
| `txns_` | participant side, by txid: `Prepared` (with staged writes and locked keys) / `Committed` / `Aborted` |
| `recs_` | decision records, by txid: `Pending` / `Committed` / `Aborted`, plus the participant list |

A shard can be a participant in one transaction and hold the decision record of
another at the same time.

### The protocol

**Single-shard transaction** — one log entry, `TXONE txid body`. The state
machine applies it atomically: refuse if any key is locked (`LOCKED <txid>`),
check the compares, apply the writes. No locks, no second phase. Outcomes are
remembered by txid, so a retry after a lost reply returns what happened rather
than applying it twice.

**Cross-shard transaction** — the *coordinator shard* is the lowest-numbered
participant, so anyone holding just the txid can find it
(`<coordShard>.<epochMs>.<clientTag>.<seq>`).

1. `TXBEGIN txid participants` → coordinator shard. Creates the decision record
   `Pending`, listing every participant. Done **before** any participant
   prepares, so whoever ever finds a lock can find the record.
2. `TXPREP txid body` → every participant, in parallel. Each one, in a single
   log entry, takes exclusive **no-wait** locks on all of its keys (refusing
   with `LOCKED <holder>` if any is taken), checks its compares (`MISMATCH`),
   and stages its writes. Reply `PREPARED`.
3. `TXDECIDE txid COMMIT|ABORT` → coordinator shard. **This is the commit
   point.** The state machine applies the first decision for a txid and answers
   every later one with the decision that already stands. The client sends
   `COMMIT` only if every participant answered `PREPARED`.
4. `TXCOMMIT` / `TXABORT txid` → every participant: apply or discard the staged
   writes, release the locks.

Every command is **idempotent by txid** because the client library retries on
timeouts and leader changes and a retried command can be appended to the log
twice. `TXCOMMIT` on a committed txn is `OK`; `TXCOMMIT` on an aborted one is
`ERR_ABORTED` (which the client counts as a protocol violation — it must be
impossible); `TXPREP` on an existing txid just reports its state.

### Recovery is the same code as normal operation

Anyone can finish a stuck transaction with `resolve(txid)`:

1. Send `TXDECIDE txid ABORT` to the coordinator shard (found from the txid).
   If a decision exists, this simply **reads it back**; if the record is still
   `Pending`, it makes the abort final — which also means the original
   coordinator's later `TXDECIDE COMMIT` will be answered `ABORTED`.
2. Deliver `TXCOMMIT` or `TXABORT` to every participant in the record.

That one idea — *ask the record, and only ever propose ABORT* — means a
resolver can never create a disagreement: it either learns the existing
decision or establishes the only decision that can exist.

`TXABORT` for a txid a participant has never seen records an **abort fence**:
a `TXPREP` that arrives later (a slow coordinator racing a resolver) is
answered `ABORTED` instead of taking locks nobody will release.

### Reads, locks, and who helps

* A plain `SET`/`DEL`/`CAS`/`CAD` on a locked key is refused with
  `"\x01LOCKED <txid>"`. A `GET` on a locked key returns the same marker rather
  than a value — the pre-commit value may already be wrong, the post-commit
  value is not yet applied. (`0x01` is reserved: `TxnClient` rejects user values
  beginning with it, because a reply that *begins* with it must always be the
  lock marker.)
* The client library **helps**. On seeing a marker, if the blocker's age (from
  its txid) is at least `lockTtl`, it runs `resolve()` on it; otherwise it waits
  a randomised 15-55 ms and retries. Nothing runs in the background: recovery
  happens when somebody wants the key. `SWEEP` does it eagerly.
* `snapshotRead(keys)` = `GET` every key, then validate by committing a
  *compare-only* transaction on all of them. If it commits, all those keys held
  the values read **at one instant** (the commit point, inside every lock
  interval). This is how an auditor gets a consistent multi-shard view without
  MVCC.

## Why it is correct

*(An argument, backed by experiments below — not a proof, and not model-checked.)*

**Atomicity.** The only event that decides a cross-shard transaction is the
first `TXDECIDE` applied to the record. Participants act only on that decision
(`TXCOMMIT` is sent only after a `COMMITTED` reply; resolvers read the record
before delivering anything). The record is Raft-replicated, so it survives
coordinator death, leader changes and client crashes. There are exactly two
places where a participant is aborted *without* consulting the record, both
safe: (a) the coordinator itself, after a prepare failed and the decision
record was unreachable — only the coordinator ever proposes `COMMIT`, and it
has just decided not to; (b) the `GuessAbort` **ablation**, which is unsafe on
purpose and is demonstrated to be (scenario D).

**Isolation (strict two-phase locking plus validation).** For a committed
transaction T and any key *k* it touches, T holds *k*'s lock continuously from
its `TXPREP` on *k*'s shard until its `TXCOMMIT` is applied there — an interval
that contains T's commit point (the `TXDECIDE COMMIT`). While the lock is held,
no other `TXPREP` or `TXONE` succeeds on *k* and no plain write is applied.
Hence (i) *k* still has the value T compared against when T commits, and
(ii) two transactions that conflict on a key have disjoint lock intervals on
it, so their commit points are ordered the same way the intervals are, on every
shared key. Ordering transactions by commit point is therefore a serial order.
Single-shard `TXONE` is one atomic log entry that refuses locked keys, so it
slots into the same order.

**Deadlock freedom.** Locks are no-wait: a conflict aborts the requester,
never blocks it, so no wait-for cycle can form. The cost is livelock risk
(two transactions repeatedly aborting each other), mitigated here only by
randomised backoff and a retry limit — and visible in the numbers
(35 of 504 attempted transfers never committed in the real-protocol runs;
see the aggregates).

**Premature abort is safe.** A resolver that loses patience and aborts a live,
merely slow coordinator cannot corrupt anything: the coordinator's `COMMIT`
decision is answered `ABORTED`, it delivers aborts, and reports
`ABORTED (fenced)`. This is scenario E and it is why `lockTtl` is a *liveness*
knob that depends on loosely synchronised client clocks (it is compared against
the epoch embedded in the txid) but never a safety one.

**What does *not* come from the protocol.** A sequence of plain reads is not a
snapshot: read X before a commit and Y after it and you see a state that never
existed. That is what `snapshotRead`'s validation fixes, and what the
`plain-audit` experiment shows when it is removed.

## Which mechanism is necessary — and the experiment that says so

| Mechanism | Removed by | What breaks | Observed |
|---|---|---|---|
| Transactions at all | `bank --naive` (two plain `SET`s) | lost updates, torn transfers | 5/5 runs; total wrong in every run; **every** account wrong in every run (60 of 60 account-runs) |
| Key locks across PREPARE→COMMIT | server env `RAFT_TXN_ABLATE=nolock` | concurrent transactions interleave their reads and writes | 5/5 runs; 59-60 of 60 account-runs wrong (two complete runs); the most audit skew of any variant (371) |
| Validation of snapshot reads | `bank --plain-audit` | auditors see half a transfer | 5/5 runs show skew (37 of 2,242 audits); **final state is exact** — the transfers were right, only the *reads* were skewed |
| Decision record | `Recovery::GuessAbort` (crash demo D) | a resolver aborts one participant that the coordinator already committed on another | alice=100, carol=80: **180 instead of 150** |
| Abort fence | server env `RAFT_TXN_ABLATE=nofence` (`fence_ablation.sh`) | a late `TXPREP` takes locks for an already-aborted txn | key locked until a reader waits out the ttl: **5 ms → ~1500 ms** |
| `TXDECIDE` returning an existing decision | scenario E (live slow coordinator) | — (shown *not* to break) | the late coordinator learns "aborted", changes nothing |

Two things in that table are easy to misread.

**The fence is not a safety mechanism.** Once the record says `Aborted`, no
later `TXPREP` can lead to a commit — a commit needs `TXDECIDE COMMIT`, which
would be answered `ABORTED`. A participant that prepares late therefore only
*leaks locks* until somebody cleans up. The bank stress test cannot catch the
missing fence for that reason: its end-of-run cleanup resolves every leak, so
`stuck-locks` stays 0. I originally listed the fence as "unproven"; writing
`fence_ablation.sh` showed what it is actually for, and the answer is
"liveness". (Like the outcomes in `txns_`, fences are never garbage-collected —
see gaps.)

**`plain-audit` having an exact final state is the point.** It isolates the
*read* path. The transfers are fine; only the observer is wrong. A test that
only checked end balances would have called that variant correct.

## The stress test

`txntool bank` (20 s per run, 12 accounts `acct-0..11` at 1000 each, 4 groups,
8 workers, one auditor per group):

* **Workers** pick a random group, two accounts in it, an amount in 1..50, read
  both with `get` (helping any locks), and commit "compare both balances,
  write both new balances". Because groups are interleaved over a 3-shard ring,
  some transfers land on one shard (fast path) and some on two (2PC).
* **Auditors** repeatedly `snapshotRead` their group and check the sum equals
  the group's invariant total (`--plain-audit`: plain reads, no validation).
* **End of run**: sweep, resolve every transaction a worker left in doubt
  (cross-shard via the decision record; single-shard by retrying `TXONE` with
  the same txid, which returns the recorded outcome), then compute each
  account's **exact** expected balance from the transfers known to have
  committed and compare with what the shards hold.

Counted as violations: `audit-skew`, `negative-balance`, `final-total`,
`final-per-account`, `stuck-locks`, `unresolved-txns`, `protocol` (a participant
answered `TXCOMMIT` with `ERR_*`, or `TXABORT` with `ERR_COMMITTED`). The tool
exits 0 iff there are none. `bank_stress.sh` then asserts the real protocol is
clean **and** that each broken variant is caught with its signature symptom —
so a test that had silently stopped checking would fail the *broken* variants
and be noticed.

`safe-leaderkill` `kill -9`s a shard's Raft leader 7 s into the run and
asserts the client *felt* it (`fault-felt` counts failed operations), so a kill
that happened to be absorbed invisibly does not count as coverage.

### Aggregates

| Variant | runs | transfers attempted | committed (2PC / single-shard) | gave up after 8 tries | aborted attempts (mismatch / conflict / other) | in doubt | audits consistent / skewed / unavailable | p50 ms cross / single (range over runs) |
|---|---|---|---|---|---|---|---|---|
| `safe` | 5 | 504 | 440 (267 / 173) | 35 | 175 / 721 / 0 | 0 | 206 / 0 / 5 | 580-599 / 147-148 |
| `safe-leaderkill` | 5 | 562 | 456 (259 / 197) | 77 | 174 / 640 / 8 | 0 | 193 / 0 / 34 | 552-581 / 148-148 |
| `naive` | 5 | 2724 | 2724 (n/a) | 0 | 0 / 0 / 0 | 0 | 2 / 89 / 21 | n/a / n/a |
| `plain-audit` | 5 | 657 | 612 (394 / 218) | 17 | 190 / 624 / 0 | 0 | 2205 / 37 / 0 | 573-599 / 144-147 |
| `ablate-nolock` | 5 | 1160 | 1138 (748 / 390) | 1 | 628 / 0 / 0 | 0 | 44 / 371 / 0 | 573-599 / 118-127 |

Sums over the five 20 s runs of each variant, from the per-seed logs saved in
`evidence/03` (latencies: the range of the per-run p50 across the five runs).
Things the table shows that the pass/fail summary hides:

* **Contention is the dominant cost.** In the real protocol, `LOCKED` conflicts
  (721) outnumber stale-compare mismatches (175) four to one, and 35 of 504
  attempted transfers (7%) never committed within 8 tries — 77 of 562 (14%) in
  the leader-kill runs.
* **The real protocol is slower, and that is the price of being right.**
  `naive` "commits" 2,724 transfers to the real protocol's 440 (about 6x) and
  ends with a wrong balance in every account of every run. It is not a fair
  throughput comparison — it does half the consensus rounds and no validation —
  which is exactly the point: the missing work was the correctness.
* **`naive`'s audit skew is persistent, not transient.** Its auditors validate
  their snapshots like everyone's, yet only 2 of its 91 completed audits were
  consistent: once lost updates have changed a group's total, *every* later
  audit of that group sees the wrong sum. Compare `plain-audit`, whose transfers
  are correct and whose skew is transient.
* **`plain-audit` skews rarely but reliably**: 37 of 2,242 audits (1.7%) saw a
  half-applied transfer, in all five runs (4-14 per run), against 0 of 206 for
  validated audits. (The count is noisy — 144 skewed audits in the first complete run, 37 in
  this one, 50 in the 2-seed clean-extract run — but none of those 12
  plain-audit runs had zero.) Plain audits are ~10x more numerous only because they skip
  the validation round — they are cheap *and* wrong.
* **Leader kills are felt, not fatal.** `fault-felt` (operations that failed
  while the leader was down) was 38-175 per run, with 413 failed reads in total
  across the five runs; no violation, no stuck lock, no unresolved transaction.

### What the stress test does *not* check

* **Ordering anomalies that preserve sums.** Transfers are commutative deltas,
  so the exact final state is the same for any serial order and for many
  non-serializable ones. The final-state check catches lost updates and torn
  writes; `audit-skew` catches read skew *inside an audit's view*. Neither is a
  history-based serializability check (no Elle/Knossos-style analysis).
* **`negative-balance` can barely fire.** 1000 initial, transfers of at most 50,
  ~75-105 commits per run: `insufficient-funds` is 0 in every log I looked at. The real
  invariants are conservation and exact state; I kept the check but it should
  not be read as evidence.
* **Real-time ordering across transactions** (does a transaction that started
  after another committed see it?). `get` is linearizable per key (read-index)
  and the lock/validate argument above implies it, but nothing here measures it.
* **Faults between PREPARE and COMMIT under load.** See below.

## Measurements

* **Cross-shard commit p50 ≈ 575-600 ms, single-shard ≈ 140-150 ms.** The
  Raft core from Projects 1/11 replicates on a **150 ms tick**, so any write
  takes roughly one tick (~125-148 ms measured). A 2PC commit is four
  *sequential* Raft commits — `TXBEGIN`, parallel `TXPREP`, `TXDECIDE`, parallel
  `TXCOMMIT` — and 4 x ~148 ms ≈ 590 ms, which is what is measured. The fast
  path is one commit. These are properties of the tick, not of 2PC: the
  *ratio* (about 4:1, i.e. the number of sequential consensus rounds) is the
  transferable result; the absolute numbers are not. I deliberately did not
  change the Raft core.
* **A read-only validation costs the same as a write.** `snapshotRead` commits
  a compare-only transaction, which records a decision just like any other.
* **Aborts are common under contention.** Conflict (`LOCKED`) aborts outnumber
  mismatch aborts four to one (721 vs 175 over the five safe runs), and 7% of
  attempted transfers never committed within 8 tries. No-wait locking with
  8 workers over 3 accounts per group is a harsh workload; that the protocol is
  *correct* under it is the point, not that it is fast. Committed throughput is
  ~4.4 transfers/s for the whole cluster.
* **Large read sets starve.** Auditing *all 12*
  accounts in one validation (`--groups 1`) completed **2 and 1 consistent
  audits in two 20 s runs** (`evidence/06`), against ~41 per run when each
  auditor covers a group of 3. With no-wait locks, some key of the twelve is
  nearly always held by someone. This is why the bank test audits per *group*.

## What's real and what's simulated

Real: 9 separate processes, real TCP, real Raft (elections, replication,
read-index reads), real `kill -9` of Raft leaders, a real client process that
`_exit(86)`s at chosen protocol points, real concurrent workers, real lock
conflicts.

Simulated or absent:

* **No persistence, no log compaction** (inherited). A restarted node loses
  its state; "the decision record survives" means *survives a minority of its
  Raft group failing*, not a full-cluster power loss.
* **The coordinator is the client library.** There is no coordinator service.
  This is intentional (it makes "coordinator dies" a cheap, exact thing to
  test) and it is also how Percolator-style systems work, but it is not how a
  server-side SQL layer would do it.
* **Crash points are injected, not natural.** Scenarios A-C kill the client at
  four named points, which is more precise than random kills and says nothing
  about points I did not name.
* **Clocks** are the machine's one clock. `lockTtl` is compared against the
  epoch in the txid, so skewed client clocks would cause wasted aborts (never
  incorrect ones — see above), but this project never exercises skew.
* **Throughput** is not measured meaningfully: 9 processes on 2 CPUs and a
  150 ms tick.

## Known gaps (not hidden, not fixed)

1. **Recovery is exercised by deterministic demos, not by the stress test.**
   Across 20 leader-kill runs, **3** transactions were left in doubt, all
   resolved *aborted*, none committed: 0 in the five runs saved in `evidence/03`,
   1 in the ten extra seeds that always kill shard 2's leader (`evidence/04`),
   and 2 in an earlier complete 5-seed run whose per-seed logs were overwritten
   (only my notes of that run remain, so don't count it as evidence). The
   client's idempotent retries ride through a leader election almost every
   time, so killing a Raft leader barely strands a transaction. Rolling
   *forward* a stuck commit under load has therefore never been observed in the
   stress test — only in scenarios B and C. The experiment that would exercise
   it is killing the **client** mid-transaction under load.
2. **2PC still blocks.** If the shard holding a decision record, or a
   participant, loses its Raft quorum, prepared transactions stay locked until
   it returns. Replicating the record fixed "the coordinator process is a
   single point of failure"; it moved the dependency to the coordinator
   *shard*.
3. **No garbage collection.** `txns_` (outcomes and fences) and `recs_` (decision
   records) grow forever. `TXPENDING` walks every entry of `txns_` to find the
   prepared ones, so it gets slower with history. A real system acknowledges completion to the record
   holder and truncates.
4. **No-wait locking starves large read sets** and gives a high abort rate
   under contention (measured above). Wait-die/wound-wait, or MVCC reads, are the
   standard fixes.
5. **No read-only fast path.** Consistent multi-shard reads cost a full 2PC
   with a durable decision.
6. **Premature aborts are wasteful.** An impatient resolver aborts transactions
   that would have finished; nothing rate-limits that.
7. **Client retry may append a command twice.** Safe only because every `TX*`
   command is idempotent by txid; plain `SET`/`DEL` inherit the old behaviour.
8. **Reserved byte and wire format.** Values may not begin with `0x01`, keys
   and values may not contain spaces or newlines. Rejected, not mangled.
9. **Plain operations are not transactional.** A plain `SET` against a locked
   key is refused (`LOCKED`), but plain reads across keys are not snapshots.
10. **The test checks sums and exact deltas**, not serializability histories
    (see above).

## Bugs and surprises found by verifying it

**Starvation of the all-accounts audit** (design finding). The first bank test
audited all 12 accounts in one snapshot; it succeeded once in 20 s. That is not
a bug in the protocol — it is what no-wait locking does to a large read set —
but it would have made `audit-skew=0` meaningless for the safe protocol (no
audits would have run). Switched to group audits and made the stress summary
report *consistent audits completed* next to *skewed audits*, so "0 skew" is
always accompanied by "out of how many".

**Crash scenario A measured the wrong thing.** In my first version an
earlier `val alice` read implicitly triggered the resolution (with the default
3 s ttl), so the timed line was not measuring the 1.5 s ttl wait it was printed
to illustrate. Reordered so the resolving read is the first one, and the printed
duration is the time that read actually took (1254 ms in the captured run).

**Weak checks in the functional demo.** A combined `$(cmd1; cmd2)` check could
pass if only one half behaved, and the "applied exactly once" check was too
weak to tell a re-apply from a single apply. Replaced by a plain `SET` between a
`TXCOMMIT` and its retry: a re-apply would clobber the plain write, so the
check can fail.

**A fence check that failed — in my script, not the protocol.** The first
`fence_ablation.sh` run failed one check ("alice is not locked: got `00`"):
the script cut a fixed column out of the raw reply, which for an unlocked key
holding `100` is `00`. The ablation half of the script had been *detecting* the
lock correctly; the negative half asked the wrong question. Changed to a
yes/no `grep -c LOCKED`; 3/3 runs pass. Worth recording because the checker
that works on the broken variant and the checker that works on the correct one
are different checks, and I had only really exercised one.

**An unproven claim of my own.** An earlier draft said the fence was "not shown
load-bearing". Instead of writing that down as a limitation, I wrote the
ablation — which found that its effect is liveness (5 ms vs 1.5 s), not
atomicity, and corrected my mental model above.

**Thin leader-kill coverage.** See gap 1. The extended run was added *because*
the first one produced 2 in-doubt transactions, and the honest conclusion of
the extension is "leader kills don't exercise recovery", not "recovery is
proven under faults".

**Carried-over lesson: ablation switches are runtime flags.** Project 11 lost
time to an `#ifdef` that changed `sizeof(RaftNode)` between translation units.
`nolock`/`nofence` are read once from the environment in the constructor into
unconditional `bool` members, so the layout never differs between binaries.

**Minor.** `sweep` double-counted a cross-shard transaction (it shows up once per
participant) until it counted distinct txids; the box has no `xxd`, so
`txnctl ENCODE` hex-encodes a body; `bank_stress.sh` wipes `logs/bank/` at the
start of every invocation, so a partial `ONLY=` run discards earlier per-seed
logs (the evidence files copy what matters).

## How this relates to real systems

*(General knowledge, as far as I know — not checked against sources in this
session, and simplified.)*

* **Sinfonia** — the mini-transaction model (compare items, write items) is
  Sinfonia's. Sinfonia runs 2PC with the memory nodes' logs as the durable
  record and a recovery coordinator that gathers votes; this project stores an
  explicit decision record instead, which makes "what was decided?" a single
  linearizable read.
* **Percolator** — the closest relative in spirit: client-driven 2PC, locks
  that other clients clean up after a timeout, and a *primary* whose state is
  the commit point. Here the decision record plays the primary's role and
  `TXDECIDE` first-wins plays the role of the primary's atomic commit/rollback.
  Percolator adds timestamps and MVCC, so readers need not block; this project
  has neither, hence lock markers and validated snapshot reads.
* **Spanner / CockroachDB** — 2PC across Paxos/Raft groups with the coordinator
  state itself replicated (a transaction record in CockroachDB; the coordinator
  leader's group in Spanner), the same answer to "2PC's coordinator is a single
  point of failure". They also add timestamp ordering, parallel commits and
  intent resolution on read — most of what is listed under *things to try*.
* **Calvin** takes the opposite route: sequence transactions first, execute
  deterministically everywhere, and avoid 2PC. Not attempted here.

## Things worth trying next

1. **Kill the client mid-transaction under load** (random `_exit` in the
   bank workers). This is the experiment that would exercise roll-forward
   under contention, which leader kills did not.
2. **Cut the sequential rounds**: let the coordinator shard also be a
   participant and fold `TXBEGIN` into its `TXPREP`; return to the caller after
   the decision and deliver commits asynchronously. That is 4 → 2-3 consensus
   rounds, measurable against the baseline here.
3. **Wait-die instead of no-wait**, and compare the abort rate and the
   all-accounts-audit starvation from `evidence/06`.
4. **MVCC + a timestamp source** so read-only transactions never block and need
   no validation.
5. **A history checker** (record every transaction's reads and writes, check
   serializability) to replace "sum and exact deltas".
6. **Garbage collection** of records and fences driven by participant
   acknowledgements, and **persistence** so a full restart keeps decisions.
7. **Clock skew injection** on the txid epoch to measure the cost of premature
   aborts.
