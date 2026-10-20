#!/bin/bash
# Snapshots and log compaction (Project 14): the log is cut down, a restart uses
# snapshot + WAL tail, a follower that fell behind the compacted prefix is brought up
# with InstallSnapshot, and a crash at EVERY step inside compaction / installation
# recovers to the same state as its peers. Fresh 3-shard cluster, snapshot every 40
# applied entries keeping 5 behind. Prints SNAPSHOT DEMO PASSED / FAILED.
set -uo pipefail
cd "$(dirname "$0")/.."
source scripts/lib.sh
trap './scripts/stop_shards.sh 1 2 3 >/dev/null 2>&1' EXIT
unset RAFT_FSYNC RAFT_PERSIST RAFT_NO_PERSIST_VOTE RAFT_CRASH_AT
export RAFT_SNAP_EVERY=40 RAFT_SNAP_KEEP=5
N=120
start_cluster
echo

yes_no() { if "$@"; then echo yes; else echo no; fi; }
fill() { local t; for t in 1 2 3 4 5 6; do ./build/txntool init $R $N "$1" >/dev/null && return 0; sleep 2; done; echo "fill $1: writes kept failing" >&2; return 1; }
count_value() { local n=0 i; for i in $(seq 0 $((N - 1))); do [ "$(val acct-$i)" = "$1" ] && n=$((n + 1)); done; echo $n; }
follower_of() { local l; l=$(leader_node "$1"); for i in $(node_ids "$1"); do [ "$i" != "$l" ] && { echo "$i"; return; }; done; }
F_() { info_field "$1" "$2" "$3"; }

VAL=1000
fill $VAL

echo "== 1. compaction keeps the log and the WAL bounded"
for r in 1 2 3 4 5 6 7 8; do VAL=$((VAL + 1)); fill $VAL; done
read -r _ _ < <(wait_converged 2 30)
for s in 1 2 3; do
    for n in $(node_ids $s); do
        li=$(F_ $s $n lastIndex); lb=$(F_ $s $n logBase); st=$(F_ $s $n snapsTaken); wb=$(F_ $s $n walBytes); sb=$(F_ $s $n snapBytes)
        printf '   shard %s node %s: %4s entries ever appended, %2s retained, snapshots taken %s (%s bytes), WAL %s bytes\n' $s $n "$li" $((li - lb)) "$st" "$sb" "$wb"
        check "shard $s node $n took snapshots" "$(yes_no [ "$st" -ge 1 ])" yes
        check "shard $s node $n retains at most ~50 log entries (40 + 5 kept + in flight)" "$(yes_no [ $((li - lb)) -le 60 ])" yes
    done
done

echo; echo "== 2. restart from snapshot + WAL tail: kill -9 all of shard 1"
read -r A0 D0 < <(wait_converged 1 30)
kill_shard 1; start_shard 1
read -r A1 D1 < <(wait_converged 1 40)
check "the log is at least as long after the restart (the new leader adds a no-op)" "$(yes_no [ "$A1" -ge "$A0" ])" yes
check "same state digest after the restart (byte-for-byte)" "$D1" "$D0"
for n in $(node_ids 1); do
    check_contains "node $n recovered from a snapshot, not from index 0" "$(grep RECOVERED logs/shard1-node$n.log | tail -1)" "snapshot@"
    check "node $n replayed only the WAL tail, not the whole history" "$(yes_no [ "$(F_ 1 $n recoveredEntries)" -lt "$(( $(F_ 1 $n lastIndex) / 2 ))" ])" yes
done

echo; echo "== 3. a follower that fell behind the compacted prefix is caught up with InstallSnapshot"
F=$(follower_of 1); L=$(leader_node 1)
FL=$(F_ 1 "$F" lastIndex)
kill_node 1 "$F"
for r in 1 2 3 4 5; do VAL=$((VAL + 1)); fill $VAL; done
echo "   follower (node $F) died with log up to index $FL; the leader (node $L) has since compacted up to index $(F_ 1 $L logBase)"
check "the leader's log base is beyond the follower's last index (AppendEntries cannot catch it up)" "$(yes_no [ "$(F_ 1 $L logBase)" -gt "$FL" ])" yes
start_node 1 "$F"
read -r A D < <(wait_converged 1 40)
check "follower converged" "$(yes_no [ -n "${D:-}" ])" yes
check "it was brought up by InstallSnapshot" "$(yes_no [ "$(F_ 1 $F snapsInstalled)" -ge 1 ])" yes
check_contains "its log says so" "$(grep -h INSTALLED logs/shard1-node$F.log | tail -1)" "INSTALLED snapshot from leader"
check "all $N accounts read back as the latest value" "$(count_value $VAL)" $N

echo; echo "== 4. a crash at every step INSIDE compaction (shard 2, one follower, restarted without the fault)"
crash_test() { # <point> <how>   how = compact | install
    local P=$1 how=$2 X; X=$(follower_of 2)
    kill_node 2 "$X"
    if [ "$how" = install ]; then for r in 1 2 3 4 5; do VAL=$((VAL + 1)); fill $VAL; done; fi
    local before; before=$(grep -c "CRASH INJECTED at $P" logs/shard2-node$X.log)
    RAFT_CRASH_AT=$P start_node 2 "$X"
    local i; for i in 1 2 3 4 5 6 7 8; do
        node_alive 2 "$X" || break
        VAL=$((VAL + 1)); fill $VAL; sleep 1
    done
    check "[$P/$how] the node crashed exactly there" "$(( $(grep -c "CRASH INJECTED at $P" logs/shard2-node$X.log) - before ))" 1
    check "[$P/$how] ... and is really dead" "$(yes_no node_alive 2 "$X")" no
    start_node 2 "$X"
    read -r A D < <(wait_converged 2 40)
    check "[$P/$how] restarted on the half-written files, converged with its peers" "$(yes_no [ -n "${D:-}" ])" yes
    check "[$P/$how] no stale temp files left in its data dir" "$(ls data/shard2-node$X | grep -c '\.tmp$')" 0
    check "[$P/$how] all $N accounts read back as the latest value" "$(count_value $VAL)" $N
}
for P in snap-tmp-synced snap-renamed snap-written wal-tmp-synced wal-renamed compact-done; do crash_test $P compact; done
echo "   ... and inside InstallSnapshot on a follower that is far behind"
for P in install-snap-written wal-tmp-synced; do crash_test $P install; done

echo; echo "== 5. a damaged snapshot is refused, not guessed at (the log it replaced is gone)"
F=$(follower_of 3); S=data/shard3-node$F/snapshot
kill_node 3 "$F"
printf '\377\377' | dd of="$S" bs=1 seek=40 conv=notrunc 2>/dev/null
start_node 3 "$F"; sleep 2
check "the node did not come up" "$(yes_no node_alive 3 "$F")" no
check_contains "and says why" "$(grep -h FATAL logs/shard3-node$F.log | tail -1)" "snapshot is corrupt"

echo
if [ $FAILS -eq 0 ]; then echo "SNAPSHOT DEMO PASSED"; else echo "SNAPSHOT DEMO FAILED ($FAILS check(s))"; exit 1; fi
