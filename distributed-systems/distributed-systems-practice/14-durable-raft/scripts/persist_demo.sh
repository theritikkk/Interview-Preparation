#!/bin/bash
# Crash-restart demo for the write-ahead log (Project 14). Everything here is
# kill -9 + restart on the SAME data directories; fsync-vs-power-loss is in
# power_loss_demo.sh. Starts a fresh 3-shard cluster (9 nodes, snapshots off, so
# these are pure WAL replays). Prints PERSIST DEMO PASSED / FAILED.
set -uo pipefail
cd "$(dirname "$0")/.."
source scripts/lib.sh
trap './scripts/stop_shards.sh 1 2 3 >/dev/null 2>&1' EXIT
unset RAFT_SNAP_EVERY RAFT_SNAP_KEEP RAFT_FSYNC RAFT_PERSIST RAFT_NO_PERSIST_VOTE RAFT_CRASH_AT RAFT_FAIL_FSYNC_AFTER
start_cluster
echo

yes_no() { if "$@"; then echo yes; else echo no; fi; }
gt0()    { [ "${1:-0}" -gt 0 ]; }
leader_up() { wait_leader "$1" "$2" >/dev/null; }
follower_of() { local l; l=$(leader_node "$1"); for i in $(node_ids "$1"); do [ "$i" != "$l" ] && { echo "$i"; return; }; done; }
# 60 accounts spread over the 3 shards. init gives up on a key after 4 tries, which a leader
# election can outlast, so repeat until every write was acknowledged: the checks below are
# about what ACKNOWLEDGED writes do, so an unacknowledged one must not count as "lost".
fill()   { local t; for t in 1 2 3 4 5; do ./build/txntool init $R 60 "$1" >/dev/null && return 0; sleep 2; done; echo "fill $1: writes kept failing" >&2; return 1; }
count_value() { local n=0 i; for i in $(seq 0 59); do [ "$(val acct-$i)" = "$1" ] && n=$((n + 1)); done; echo $n; }

fill 1000 >/dev/null
echo "== 1. one node is killed (kill -9) while the cluster keeps writing, then restarted"
F=$(follower_of 3); L=$(leader_node 3)
echo "   shard 3: leader node $L, killing follower node $F"
kill_node 3 "$F"
fill 2000 >/dev/null
start_node 3 "$F"
read -r APPLIED DIGEST < <(wait_converged 3 40)
check "the restarted follower converged with its peers (same applied index and state digest)" "$(yes_no [ -n "${DIGEST:-}" ])" yes
check "it rebuilt its log from the WAL (entries recovered > 0)" "$(yes_no gt0 "$(info_field 3 "$F" recoveredEntries)")" yes
check "no torn tail (it was killed between writes)" "$(info_field 3 "$F" tornTail)" 0
echo "   ... now the LEADER of shard 3 is killed"
kill_node 3 "$L"
fill 3000 >/dev/null
start_node 3 "$L"
read -r APPLIED DIGEST < <(wait_converged 3 40)
check "the restarted old leader rejoined and converged" "$(yes_no [ -n "${DIGEST:-}" ])" yes
check "all 60 accounts read back as the latest value" "$(count_value 3000)" 60

echo; echo "== 2. a whole shard dies at once (all 3 nodes kill -9), then restarts: nothing is lost"
fill 4000 >/dev/null
kill_shard 2
start_shard 2
check "shard 2 elected a leader again" "$(yes_no leader_up 2 40)" yes
check "all 60 accounts (every shard) read back as the latest value" "$(count_value 4000)" 60

echo; echo "== 3. the WHOLE cluster is killed with a transaction half-done; its locks and decision survive"
XFER="-c alice=100 -c carol=50 -w alice=70 -w carol=80"   # alice: shard 3, carol: shard 2 -> 2PC, record on shard 2
check "placement: alice on shard 3, carol on shard 2" "$(shard_of alice)/$(shard_of carol)" 3/2
reset() { $T $R SET alice 100 >/dev/null; $T $R SET carol 50 >/dev/null; }
restart_all() { for s in 1 2 3; do kill_shard $s; done; KEEP_DATA=1 ./scripts/run_shards.sh 1 2 3 >/dev/null; }
echo "   3a. coordinator dies after PREPARE (no decision yet); then every node is killed and restarted"
reset
$T --crash-at after-prepare $R TXN $XFER >/dev/null 2>&1; check "coordinator exit status (86 = injected crash)" "$?" 86
restart_all
check "both shards still hold the prepared txn after the restart" "$(pending_count)" 2
check "the lock survived: a plain SET on alice is still refused" "$($T $R RAW 3 WRITE SET alice 5 | cut -c2-7)" LOCKED
check "a reader past the ttl resolves it: rolled BACK (alice)" "$($T --lock-ttl-ms 1500 $R GET alice | sed -n 's/^OK value="\(.*\)" .*/\1/p')" 100
check "carol rolled back too" "$(val carol)" 50
check "no locks left" "$(pending_count)" 0
echo "   3b. coordinator dies AFTER the COMMIT decision is recorded; then every node is killed and restarted"
reset
$T --crash-at after-decide $R TXN $XFER >/dev/null 2>&1; check "coordinator exit status" "$?" 86
restart_all
check "both shards still hold the prepared txn" "$(pending_count)" 2
check "a reader resolves it from the SURVIVING decision record: rolled FORWARD (alice)" "$($T --lock-ttl-ms 1500 $R GET alice | sed -n 's/^OK value="\(.*\)" .*/\1/p')" 70
check "carol rolled forward too" "$(val carol)" 80
check "no locks left" "$(pending_count)" 0

echo; echo "== 4. damage to a WAL on disk (shard 2, a follower)"
F=$(follower_of 2); W=data/shard2-node$F/wal
fill 5000 >/dev/null
echo "   4a. a crash tears the last record: cut 5 bytes off the end of node $F's WAL"
kill_node 2 "$F"; SZ=$(stat -c %s "$W"); truncate -s $((SZ - 5)) "$W"
start_node 2 "$F"
read -r APPLIED DIGEST < <(wait_converged 2 40)
check "node recovered, cut the torn tail (tornTail=1)" "$(info_field 2 "$F" tornTail)" 1
check_contains "the node log says exactly what it cut" "$(grep RECOVERED logs/shard2-node$F.log | tail -1)" "TORN-TAIL-CUT"
check "and the leader re-sent the lost entry: converged" "$(yes_no [ -n "${DIGEST:-}" ])" yes
echo "   4b. garbage after the last record (a half-written append): 13 random bytes"
kill_node 2 "$F"; head -c 13 /dev/urandom >> "$W"
start_node 2 "$F"
read -r APPLIED DIGEST < <(wait_converged 2 40)
check "node recovered, cut the garbage (tornTail=1)" "$(info_field 2 "$F" tornTail)" 1
check "converged" "$(yes_no [ -n "${DIGEST:-}" ])" yes
echo "   4c. a byte flipped in the MIDDLE of the log is not a torn tail: the node must refuse to start"
kill_node 2 "$F"; SZ=$(stat -c %s "$W")
printf '\377' | dd of="$W" bs=1 seek=$((SZ / 3)) conv=notrunc 2>/dev/null
start_node 2 "$F"; sleep 2
check "the node did not come up" "$(yes_no node_alive 2 "$F")" no
check_contains "and says why" "$(grep -h FATAL logs/shard2-node$F.log | tail -1)" "damaged in the middle"
check "the other two nodes of shard 2 still serve (quorum intact)" "$(val acct-1 >/dev/null; yes_no [ "$(count_value 5000)" -eq 60 ])" yes

echo; echo "== 5. a vote must survive a restart (Raft: at most one vote per term)"
# Isolate node 1 of shard 1 (kill its peers) so no real election can interfere, then
# ask it for its vote by hand twice in the same term, with a restart in between.
P=$(node_port 1 1)
vote() { $PROBE 127.0.0.1 "$P" VOTE 1000 "$1" 1000000 1000 2>/dev/null; }
wait_up() { for i in $(seq 1 100); do [ -n "$(node_info 1 1)" ] && return 0; sleep 0.05; done; return 1; }
vote_experiment() { # <label>  -> sets V1 V2
    kill_node 1 1; kill_node 1 2; kill_node 1 3
    start_node 1 1; wait_up
    V1=$(vote 2)
    kill_node 1 1
    start_node 1 1; wait_up
    V2=$(vote 3)
    echo "   $1: first request  (candidate 2): $V1"
    echo "   $1: after kill -9 + restart, same term (candidate 3): $V2"
}
vote_experiment "real"
check_contains "first request granted" "$V1" GRANTED
check_contains "after the restart the node still remembers its vote and DENIES candidate 3" "$V2" DENIED
check_contains "  (denied because of the remembered vote, not because its term moved on)" "$V2" "now 1000"
check "its stored state: term 1000, voted for node 2" "$(info_field 1 1 term)/$(info_field 1 1 votedFor)" 1000/2
kill_node 1 1; rm -rf data/shard1-node1
echo "   ABLATION: same experiment with RAFT_NO_PERSIST_VOTE=1 (term/vote not written to the WAL)"
export RAFT_NO_PERSIST_VOTE=1
vote_experiment "ablation"
unset RAFT_NO_PERSIST_VOTE
check_contains "ablation: first request granted" "$V1" GRANTED
check_contains "ABLATION: the restarted node forgot and GRANTS candidate 3 in the same term (a double vote)" "$V2" GRANTED

echo; echo "== 6. a failed fsync is fatal, never retried (shard 3, one follower, injected EIO)"
# After a failed fsync the kernel may already have dropped the dirty pages, so a retry could "succeed" without the
# data being on disk. The node must stop instead of acknowledging what it may not have.
kill_shard 1; start_shard 1   # section 5 left shard 1 stopped (and wiped its node 1): bring it back first
check "shard 1 is back (a leader was elected)" "$(yes_no leader_up 1 40)" yes
F=$(follower_of 3)
kill_node 3 "$F"
RAFT_FAIL_FSYNC_AFTER=2 start_node 3 "$F"; sleep 1
echo "   node $F restarted with RAFT_FAIL_FSYNC_AFTER=2: its 3rd fsync fails with EIO; now writing until it does"
V=6000
for t in 1 2 3 4 5 6; do V=$((6000 + t)); fill $V >/dev/null; sleep 1; node_alive 3 "$F" || break; done
check "the node stopped instead of carrying on" "$(yes_no node_alive 3 "$F")" no
check_contains "and logged why" "$(grep -h FATAL logs/shard3-node$F.log | tail -1)" "failed (Input/output error)"
check "the other two nodes kept shard 3 available: all 60 accounts read back as the latest value" "$(count_value $V)" 60
start_node 3 "$F"
read -r APPLIED DIGEST < <(wait_converged 3 40)
check "restarted without the fault it rejoined and converged" "$(yes_no [ -n "${DIGEST:-}" ])" yes

echo
if [ $FAILS -eq 0 ]; then echo "PERSIST DEMO PASSED"; else echo "PERSIST DEMO FAILED ($FAILS check(s))"; exit 1; fi
