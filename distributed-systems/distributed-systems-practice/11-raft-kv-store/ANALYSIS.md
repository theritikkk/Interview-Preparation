# ANALYSIS — read-index, what's simulated, and the bugs found while verifying it

This project extends Project 1's Raft cluster into a usable KV store (`SET`,
`GET`, `DEL`, `CAS`, a client library) and closes the gap Project 1's own
ANALYSIS.md flagged as "not implemented": **linearizable reads**. The
interesting part is not that read-index works — it's that my first read-index
implementation did *not*, in two different ways, and each time the only thing
that caught it was running a test designed to catch exactly that failure.
Three of the bugs below were in my *test harnesses*, one of which made a
broken implementation look correct. Those are documented too, because a
verification result is only worth what the check behind it could actually
detect.

Everything claimed here is backed by a captured run in `evidence/`.

## Headline results

| Question | Answer | Evidence |
|---|---|---|
| Does Project 1's local-only `GET` serve stale data from a partitioned leader? | **Yes**, 3/3 runs: `ok=true value="before-partition"` after a newer value was committed elsewhere | `01-…unsafe-stale-read.txt` |
| Does read-index refuse in the same scenario? | **Yes**, 3/3 runs: `ok=false` | `02-…safe-correctly-refuses.txt` |
| Does a newly elected leader ever answer a read older than an acknowledged write? | First read-index version: **yes, 20/20 trials**. Final version: **no, 0/20** | `05-failover-read-test.txt` |
| What does read-index cost? | ~5× a local read when healthy (≈370 µs vs ≈72 µs p50 on loopback); flat when a follower is dead or frozen | `04-read-index-latency.txt` |

The two read-safety mechanisms are **orthogonal**, and a correct implementation
needs both:

- **Own-term-commit precondition** (`termStartIndex_`) guards against a leader
  that is *too new*: it was just elected and its `commitIndex_` hasn't caught up
  to writes the previous leader already acknowledged.
- **Majority confirmation** (`confirmLeadershipMajority`) guards against a leader
  that is *too old*: it was deposed but doesn't know it.

`raft_node_unsafe` has only the first; that is exactly why it passes the
failover test and fails the partition test. The first read-index version had
only the second; that is exactly why it passed the partition test and failed
the failover test. Neither test alone would have shown the whole picture.

## What this adds, and how it works

### DEL and CAS

Both go through the replicated log exactly like `SET`; only the post-commit
reporting differs. `CAS key expected new` applies `new` iff the current value
equals `expected` (a missing key counts as `""`, so `CAS k "" v` means "create
if absent"). The state machine therefore has to report a per-command outcome
back to the one client waiting on that log index: `applierLoop()` stores
`"OK"`/`"MISMATCH"` in `applyResults_[index]`, and the waiting client handler
fetches and erases it. A `CAS` that *mismatches* is still a committed log
entry — the client gets `ok=true, value="MISMATCH"`, because the cluster did
answer; "the compare failed" is data, not an error.

### Read-index (§8), four steps, all required

1. **Precondition:** wait until `lastApplied_ >= termStartIndex_`, i.e. this
   leader has committed its own-term no-op. Until then `commitIndex_` can lag
   writes the old leader acknowledged (see bug #7).
2. **Snapshot** `readIndex = commitIndex_`.
3. **Confirm** leadership with a *fresh* AppendEntries round to the peers, right
   now — not a belief left over from the last 150 ms heartbeat tick. Return as
   soon as a majority (counting self) has answered with a term ≤ ours.
4. **Wait** for `lastApplied_ >= readIndex`, then answer from `kv_`.

`confirmLeadershipMajority` deliberately ignores each response's `success`
field. A follower may reject the empty AppendEntries on a log-matching
mismatch and still be perfectly good evidence that it hasn't moved to a newer
term; read-index asks "is anyone newer than me?", not "do your logs match?".
If a peer reports a *higher* term, the node demotes itself and refuses.

A late straggler that reveals a higher term *after* a majority already acked
is ignored for that read. That is safe: a majority acknowledging term T at
that moment means no newer leader could have been elected in the meantime
(electing one needs a majority, which would intersect this one). The leader
finds out through the normal heartbeat loop.

**Why not lease-based reads?** A lease avoids the per-read round trip but
depends on bounded clock drift between nodes; get that assumption wrong and you
get stale reads with no network event to blame. Read-index depends only on
message exchange, which is the safer thing to demonstrate and to reason about.
The price is the latency below.

### `KVClient`

Replaces Project 1's retry loop inlined in `main()`. It follows `leaderHint`
redirects, falls back to the next node when there's no hint, and **remembers the
last known leader across calls** so a long-lived client doesn't re-probe from
`peers.front()` every time. `kvctl` is now a thin CLI over it, and
`direct_probe` is the deliberate opposite: one connection, one request, raw
answer, no retries or redirects, so the partition test can see exactly what a
specific node says.

### The A/B technique

`raft_node` and `raft_node_unsafe` are the **same `raft_node.cpp`** compiled
twice; the only difference is `-DRAFT_UNSAFE_READS`, which swaps the `GET`
branch for Project 1's original local-only check. Any difference in behavior
under the identical scenario is therefore attributable to that one branch — a
much stronger claim than "I wrote two programs and one behaved differently."

## What's real and what's simulated

**The partition is simulated inside the node.** `kill -USR1` toggles a
`deafened_` flag: while set, the node silently drops every inbound peer RPC
(closes the connection without answering) and every outbound peer RPC fails
immediately, while client requests are served normally. That is not a kernel
network fault — there is no packet loss, delay, reordering or asymmetry — and
it is not equivalent in general.

What it *does* prove: the node's **decision logic**. A leader that cannot reach
anyone should not answer a read, and the experiment shows the local-only check
does and read-index doesn't. That is what the experiment is about.

What it does *not* prove or measure:

- **Latency of refusal in a real partition.** Here outbound RPCs short-circuit,
  so the deafened leader refuses instantly. In a real partition each peer RPC
  would run to the 600 ms timeout before the leader could conclude it has no
  majority. By the code, that's the expected behavior; I did not measure it.
- Anything involving real packet loss or asymmetric partitions (A can reach B,
  B can't reach A).

The same applies to `kill -9` and `SIGSTOP` in the other tests: they are real
process-level faults, but not network faults.

## Mapping onto the Raft paper

| Section | Status here |
|---|---|
| §5.1–5.4 election, replication, restriction, no-op-on-election | Implemented in Project 1; unchanged. Four inherited bugs are summarised at the end |
| §5.5 follower/candidate crashes | Handled implicitly (RPCs time out and retry) |
| §6 membership changes | **Not implemented** — `cluster.conf` is static |
| §7 log compaction | **Not implemented** — the log (and `applyResults_`) grow without bound |
| §8 linearizable reads | **Implemented**: own-term-commit precondition + per-read majority confirmation. Lease reads not implemented |
| §8 client sessions / duplicate suppression | **Not implemented** — see "Known gaps" |
| Persistence (§5.1 "stable storage") | **Not implemented** — state is in memory only; a restarted node comes back blank. The `kill -9` demos are crash-and-stay-dead, which this tolerates; crash-and-restart would not be safe |

## Known gaps (not hidden, not fixed)

- **Retried writes can apply twice.** `KVClient` retries a write when the
  leader answers `isLeader && !ok` (timed out waiting). If the first attempt
  did commit, the retry commits again. `SET` and `DEL` are idempotent so this
  is harmless; `CAS` is not — a retried *successful* CAS would report
  `MISMATCH`. The paper's fix is a client session ID + sequence number with
  dedup in the state machine. This is from reading the code; I did not build a
  test that reproduces it.
- **Each read costs one fresh TCP connection plus one thread per peer.**
  The standard optimization is to let one heartbeat round confirm *many*
  concurrent reads; not done.
- **Detached straggler threads capture `this`.** After a read returns on majority
  ack, slow peers' threads finish on their own for up to the 600 ms RPC timeout.
  If the node were destroyed in that window they would touch freed memory. In
  practice the process is exiting anyway, and the inherited per-connection
  handler threads have the same property, but it is a real lifetime hazard.
- **A leader that can't commit its own-term no-op** (no quorum) makes every read
  wait up to 3 s and then refuse. Correct, not graceful.

## Bugs, found by running it

Project 1's four bugs (log-order grep, the no-op on election, the no-op/read
race, stale leader hint) are documented in that project and still fixed here.
This project's, continuing the numbering:

### Bug #5 — a node whose `bind()` failed never exited

Running `partition_demo.sh` back-to-back, a node that failed to `bind()` set its
internal `running_` flag to false and returned from its acceptor thread — but
`main()` only waited on SIGINT/SIGTERM, so the process stayed alive with a dead
listener and a PID file pointing at it. The next cluster start then raced it
for the port. **Fix:** `RaftNode::isRunning()`; `main()` now exits (code 1) when
either a signal arrives or the node stops itself. Paired with wait-for-actual-exit
loops in `stop_cluster.sh` and the demo scripts, since a node takes up to ~1 s to
shut down (its `accept()` only wakes once a second).

### Bug #6 — an ODR / struct-layout mismatch made one binary exit ~1 ms after starting

After the fixes above, `raft_node_unsafe` failed 3/3 with "no leader elected in
10 s" while `raft_node` passed 3/3. The node logs showed each unsafe node print
"starting" and "stopped" within milliseconds — no election, no errors, no
`FATAL: bind()`.

`raft_node.h` added a member, `termStartIndex_`, only `#ifdef RAFT_UNSAFE_READS`,
and CMake defined that macro `PRIVATE` on the `raft_core_unsafe` library. So
`raft_node.cpp` (compiled into that library) saw a larger `RaftNode` than
`main.cpp` (compiled separately into the executable, where the macro was *not*
defined). Same header, two different class layouts in one binary. The inline
`isRunning()` compiled into `main.cpp` read `running_` at the wrong offset,
saw garbage, and `main()`'s loop exited at once. Measured, not inferred:

```
sizeof(RaftNode) without the macro: 480
sizeof(RaftNode) with the macro:    488      (one uint64_t)
broken build, single node:  "starting" … "stopped" 1 ms later, exit 1
```

(`evidence/03-odr-bug-reproduction.txt`, reproduced in a scratch copy with the
original `PRIVATE` line.) The compiler warned about nothing and the code is
"correct" per translation unit; the defect only exists in the *combination*.

**First fix:** `PUBLIC` instead of `PRIVATE`. **Root-cause fix (later, as part
of bug #7):** `termStartIndex_` is now unconditional, so no header-visible layout
depends on the macro at all; `PUBLIC` is kept as defense in depth.

The lesson that generalizes: a member (or inline function body) that varies
under a preprocessor macro is only safe if *every* translation unit that
includes the header sees the same macro. Making the layout independent of the
macro is sturdier than getting the build flags right.

### Bug #7 — read-index served stale reads after every failover (20/20)

My first read-index implementation took `readIndex = commitIndex_` and then did
the majority confirmation. I had removed Project 1's Bug #3 fix
(`termStartIndex_`) from the safe path, reasoning that read-index subsumed it.
It does not.

Why: followers only learn a commit on the **next** heartbeat (up to 150 ms
later), and a newly elected leader cannot advance `commitIndex_` over old-term
entries by counting replicas (the §5.4.2 rule). So right after an election its
`commitIndex_` can sit behind a write the old leader already acknowledged. A new
leader *legitimately passes* the majority check — it really is the leader — so
the partition test could never catch this.

`scripts/failover_read_test.sh` does: write, `kill -9` the leader immediately,
then hammer every survivor with raw GETs and record the **first** successful
answer.

| Build | Result |
|---|---|
| First read-index version | **20/20 stale**, first answer = the *old* value (a second run with only the precondition removed: 19/20) |
| Final read-index | **20/20 correct** |
| `raft_node_unsafe` (control) | 10/10 correct — it kept the `termStartIndex_` wait |

**Fix:** step 1 above — wait for `lastApplied_ >= termStartIndex_` before
snapshotting `readIndex`. This matches the paper's §8 requirement and what
production implementations enforce.

Note the irony the data shows: in the failover scenario the build labeled
"unsafe" was the *safer* one. "Unsafe" only means "missing majority
confirmation", not "wrong everywhere".

### Bug #8 — one hung follower made every read take 600 ms

Measured, not guessed. The first `confirmLeadershipMajority` joined every peer
thread, so it waited for the *slowest* peer even after a majority had answered.
With one follower `SIGSTOP`'d (the kernel still accepts the TCP connection; the
process never answers) every read took the full RPC timeout:

| Scenario (p50) | local-only | read-index v1 | read-index final |
|---|---|---|---|
| healthy, 5/5 | 72 µs | 453 µs | 370 µs |
| 2 followers dead, 3/5 up | 71 µs | **11.1 ms** | 347 µs |
| 1 follower frozen, 4/5 up | 76 µs | **601 ms** | 323 µs |

A majority protocol must not stall on a minority failure. **Fix:** return as soon
as `acks >= majority`; stragglers finish detached, writing into heap state held
by `shared_ptr` (see the lifetime caveat above). One thing I did **not**
root-cause: why the 2-dead case cost ~11 ms in v1 — refused loopback connects
should be near-instant. It disappeared once reads stopped waiting on those
threads, and I'm not going to invent a reason for it.

## Bugs in the test harnesses

These are listed separately because they're about *verification*, not the
system — and one of them produced a confident wrong answer.

**H1 — `partition_demo.sh`: unguarded `grep` under `set -euo pipefail`.** When
`grep -oP` matched nothing it returned 1 and aborted the whole script at the
assignment, before the explicit "no leader elected" error handling ever ran.
Fixed with a polling loop and `|| true`.

**H2 — `failover_read_test.sh` was blind to the failure it exists to detect.**
Its probe loop ran `while [ -z "$GOT" ]`. A stale read of a never-written key
returns `value=""` — empty — so the loop treated it as "no answer yet" and kept
polling until the *correct* value appeared. Result: it reported **8/8 and 20/20
correct** against an implementation that was wrong 20/20 times. I only caught it
by tracing what a stale read would look like in that loop, not by trusting the
green result. Fixed by looping on "a node answered `ok=true`" and by
pre-seeding a distinct `old-N` value so a stale answer is a non-empty,
unmistakable string. The fixed script then showed 20/20 stale on the broken
build — which is what makes the later 20/20 *correct* mean something.

**H3 — the latency benchmark silently measured the wrong thing.** Starting nodes
with `setsid … &` makes `$!` the PID of the short-lived `setsid` wrapper, not
the node. The `kill -9` / `SIGSTOP` calls then hit nothing, `stop_cluster.sh`
couldn't stop the real nodes, and the "read-index" runs were actually measured
against leftover *unsafe* processes. The first set of numbers was discarded
entirely. (The same PID-capture mistake appeared in Project 10.) The rewritten
harness asserts that each PID's `/proc/<pid>/exe` is the intended binary and
that kills/stops actually took effect, and fails closed otherwise. A smaller
self-inflicted one on the way: `pkill -f "build/raft_node"` matched the shell
running it and killed that too — the pattern must not match its own command line.

**The common thread** is the same as Project 1's, with one addition. Every real
bug here was found by running something repeatedly against a scenario designed
to expose it, not by reading code. The addition: **a check that has never been
seen to fail hasn't been shown to be able to.** The failover test only became
trustworthy once it had demonstrably failed against a known-bad build.
