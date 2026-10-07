# ANALYSIS — design decisions, what's simplified, and three real bugs

This document does three things: maps this implementation onto the Raft
paper section by section, states plainly what's simplified and why, and
walks through three genuine concurrency/correctness bugs this code had —
each one found by actually running the cluster and watching it fail, not
by inspection alone. If you're learning distributed systems, the bugs are
the more useful part of this document: each is a case where the code
*looked* right, compiled cleanly, and still had a real, reproducible
correctness problem.

## Mapping onto the Raft paper

| Paper section | What it covers | Status here |
|---|---|---|
| §5.1 | Server states (follower/candidate/leader), terms | Implemented — `Role` enum, `currentTerm_` |
| §5.2 | Leader election, randomized timeouts | Implemented — `electionLoop()`, 1500–3000ms randomized timeout, 150ms heartbeat |
| §5.3 | Log replication, AppendEntries, log matching | Implemented — `replicateToPeer()`, conflict detection + truncation in `handleAppendEntriesMsg` |
| §5.4.1 | Election restriction (candidate must be at least as up-to-date) | Implemented — the `upToDate` check in `handleRequestVoteMsg` |
| §5.4.2 | Committing entries from previous terms (leader completeness) | Implemented via the no-op-on-election fix — see Bug #2 below. This is the part that's easiest to get wrong and the paper spends real effort explaining why |
| §5.5 | Follower and candidate crashes | Handled implicitly — RPCs just time out and retry on the next heartbeat tick; see "what's NOT here" below for what this doesn't cover |
| §5.6 | Timing and availability | Implemented — the 150ms heartbeat / 1500–3000ms election timeout spread follows the paper's broad-strokes recommendation (timeout ≫ broadcast time) |
| §6 | Cluster membership changes | **Not implemented.** `cluster.conf` is static; see below |
| §7 | Log compaction / snapshotting | **Not implemented.** The log grows unboundedly; see below |
| §8 | Client interaction, linearizable semantics | Partially implemented — session semantics (retry against the right leader) are handled by the client's redirect-following; the no-op-on-election read-safety fix (Bug #3) directly implements the paper's §8 recommendation. Full linearizable reads (read-index with a fresh round-trip confirming leadership) are **not** implemented — see below |

## What's deliberately simplified, and why

**No persistent storage.** `currentTerm_`, `votedFor_`, and `log_` all live
in memory only. The real Raft paper requires these survive a crash and
reboot (§5.1: "these ... have been updated on stable storage before
responding to RPCs"). A node that crashes and restarts here comes back
with a blank slate and rejoins as if it were brand new — which is fine
for a crash-and-stay-dead demo (exactly what `demo.sh` does with
`kill -9`), but would violate safety in a crash-and-*restart* scenario.
Adding this would mean an fsync'd write-ahead log before every RPC
response — a real cost worth demonstrating, but a different exercise
from the one this project is about (the election/replication/commit
logic itself).

**No log compaction or snapshotting (§7).** The log vector grows forever.
For a teaching cluster that runs for minutes and handles a handful of
keys, this doesn't matter; for a real system it would need bounding.

**No cluster membership changes (§6).** `cluster.conf` is read once at
startup and never changes. Adding or removing a node means restarting
the whole cluster with a new config, not the live joint-consensus
procedure the paper describes.

**Reads are not fully linearizable.** `GET` is served from the leader's
local `kv_` once it's confirmed caught-up-enough (see Bug #3), which is
sufficient for *this* leader to not serve stale data relative to its own
election. It is not sufficient for the strongest linearizability
guarantee the paper describes in §8, which requires the leader to
confirm — on every single read, not just once per term — that it's
still the leader by getting a fresh heartbeat ack from a majority
(the "read-index" approach) or using a time-bounded lease. A leader that
has been silently partitioned away from the rest of the cluster (but
doesn't know it yet) could serve a stale read here. Implementing true
read-index was out of scope; the no-op fix closes the specific,
demonstrable gap this project actually hit.

**One fresh TCP connection per RPC**, not a persistent connection per
peer. Simpler to reason about and debug (every RPC is a clean
connect → send → recv → close), at a throughput cost that doesn't matter
for a 5-node teaching cluster making a few RPCs every 150ms.

## Three real bugs — found by running it, not by reading it

All three were found the same way: run `demo.sh` repeatedly (not once —
Raft's randomized timeouts mean a single pass can get lucky) and take
every failure seriously, including ones that looked like they might be
"just the demo script" rather than the actual Raft code.

### Bug #1 (tooling, not Raft): chronological order vs. file order

`demo.sh` detects the current leader by grepping every node's log file
for `"became LEADER"` and taking the last match:

```bash
grep -h "became LEADER" logs/node*.log | tail -1
```

`grep -h logs/node*.log` expands the glob alphabetically — `node1.log`,
`node2.log`, `node3.log`, ... — and prints matches in that **file**
order, not the order the events actually happened in. If node3 was the
very first leader (term 1) and node1 later won a real election (term 2)
after node3 crashed, `node3.log`'s line still sorts *after* `node1.log`'s
line alphabetically, so `tail -1` returned node3 — the dead former
leader — as the "current" one. The demo then polled forever for a "new"
leader that wasn't node3, except node3 (correctly reported, just by the
wrong method) was being excluded from being counted again, so it hung
until its 20-second timeout and reported `DEMO FAILED: no new leader
elected after the crash` — even though the C++ code had already elected
one correctly.

**Fix:** every log line starts with a fixed-width `[HH:MM:SS.mmm]`
timestamp, so a plain lexicographic `sort` before `tail -1` is exactly a
chronological sort:

```bash
grep -h "became LEADER" logs/node*.log | sort | tail -1
```

This is a one-line fix, but it's worth dwelling on: it's a reminder that
*the test harness itself* is code, and a bug in it can look exactly like
a bug in the system under test. The first instinct — "the Raft
implementation must have a concurrency bug" — would have sent debugging
effort in the wrong direction entirely.

### Bug #2 (real Raft bug): a new leader can't read its own log

After fixing Bug #1, `demo.sh` got further, then failed differently:
after failover, `GET last_book` came back empty — data that had
genuinely been committed by the old leader before it crashed.

The relevant rule, correctly implemented, is in `leaderLoop()`'s
commit-index advancement:

```cpp
for (uint64_t N = lastLogIndex(); N > commitIndex_; --N) {
    if (log_[N].term != currentTerm_) continue;   // <-- this line
    ...
}
```

This is §5.4.2 of the paper: a leader must never advance `commitIndex_`
over an entry from a **previous** term purely by counting replicas,
because that specific shortcut is exactly how the paper's Figure 8
scenario can un-commit a previously-"committed" entry. It's a required
safety rule, not a bug.

But its consequence is easy to miss: a *freshly elected* leader can
already hold older, majority-replicated entries in its own log (they got
there via normal replication before the old leader crashed) — yet it
cannot advance its own `commitIndex_` to include them, because none of
them carry its own (brand new) term. It needs to commit *something* in
its own term before the rule above will let it move `commitIndex_`
forward at all. Until then, `lastApplied_` — and therefore the `kv_`
state machine `GET` reads from — is stuck below entries that are
genuinely durable and committed by any reasonable external definition.

Concretely: node3 wins a new term, already has `last_book` in its log
(replicated before the old leader died), but its `commitIndex_` hasn't
moved, so `lastApplied_` hasn't either, so `kv_["last_book"]` was never
populated on node3 — a correct reflection of the rule above, and a
correctness trap if nothing accounts for it.

**Fix** (the paper's own recommended solution, §8): every new leader
immediately appends a no-op log entry stamped with its own current term:

```cpp
log_.push_back(LogEntry{currentTerm_, ""});
```

Once that entry replicates to a majority, the Log Matching Property
means committing it also commits everything before it, in one step —
unblocking `commitIndex_` without waiting for a real client write to
happen to land in the new term.

### Bug #3 (real Raft bug): the no-op fixes writes, but reads can still race it

Bug #2's fix made the demo pass most of the time — but not all of the
time. Re-running it in a loop surfaced a case where `GET last_book` was
*still* empty after failover, even with the no-op in place.

The no-op needs one full heartbeat round-trip to actually commit: the
new leader has to replicate it to a majority and count the acks before
`commitIndex_` moves. That's typically well under 150ms locally, but it
is not zero — and `demo.sh` detects the new leader from its own
`"became LEADER"` log line, which prints the *instant* the election
succeeds, before that first heartbeat round has had any chance to run.
A `GET` landing in that narrow window was being answered straight from
`kv_` with whatever was already applied, which could be behind what was
actually committed moments before the crash.

**Fix:** track the no-op's own log index (`termStartIndex_`, set in
`becomeLeader()` right after pushing the no-op), and make `GET` wait —
the same way `SET` already does, via `cv_.wait_for` on the shared
condition variable — until `lastApplied_` reaches it:

```cpp
cv_.wait_for(lk, std::chrono::seconds(3), [&] {
    return lastApplied_ >= termStartIndex_ || role_ != Role::Leader ||
           currentTerm_ != term;
});
```

This is the direct, minimal version of what the paper's §8 describes:
a leader must not answer a client until it has confirmed (by committing
something in its own term) that it actually has an up-to-date view of
what's committed.

### Bug #4 (real Raft bug): a stale leader hint outlives the leader it points to

Even with Bugs #2 and #3 fixed, a different failure mode showed up under
closer, higher-frequency testing (firing several client requests
concurrently right after a forced failover): clients would bounce
between two nodes forever and exhaust their retry budget, logging things
like:

```
(node 2 says it isn't the leader, trying node 1...)
(node 1 unreachable, trying another node...)
(node 2 says it isn't the leader, trying node 1...)
(node 1 unreachable, trying another node...)
...
FAILED: could not reach a leader after 18 attempts
```

— even though a real, legitimately elected leader (node 4, confirmed by
every other node's own vote log) was alive and reachable the entire
time. Raw TCP-level probing during the failure window confirmed the
"unreachable" node's port genuinely wasn't the issue — the node being
pointed to (node 1) really was dead; the problem was that node 2 kept
*recommending* it.

The cause: `currentLeaderHint_` is only ever refreshed inside
`handleAppendEntriesMsg`, i.e. when a node actually hears from the
current leader. But a node that just **voted** for a new leader in a
higher term has, by definition, not yet heard from that leader — it
granted a vote, which is a promise, not a confirmation the candidate
won. Nothing cleared the *old* hint in the meantime, so a client asking
"who's the leader?" between the vote and the new leader's first
heartbeat got sent straight back to the node that just lost power
(or died).

**Fix:** a leader hint from a previous term is never trustworthy once
the term has advanced — something (a new candidate's vote request, or a
new leader's heartbeat) has already told this node its old picture of
the world is stale. So the hint is invalidated the moment the term
advances, in `becomeFollower()`:

```cpp
void RaftNode::becomeFollower(uint64_t newTerm) {
    if (newTerm > currentTerm_) {
        currentTerm_ = newTerm;
        votedFor_ = -1;
        currentLeaderHint_ = -1;   // the fix
    }
    ...
}
```

It gets set correctly again the moment this node actually hears from
the new term's real leader. The client already treats an unknown hint
(`leaderHint == 0`) correctly — falling back to just trying the next
node in the config — so clearing the stale value was the entire fix.

### The common thread

None of these four bugs were visible from reading the code once. Each
one needed the cluster actually running, a failure actually
reproduced, and — critically — *repetition*: because election timeouts
are randomized, any given run can get lucky and paper over a real race.
The demo was run in a loop (5, then 8 consecutive passes) specifically
to build confidence that each fix addressed the actual race rather than
one instance of it.
