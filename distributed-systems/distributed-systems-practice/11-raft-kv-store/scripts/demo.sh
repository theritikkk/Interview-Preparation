#!/bin/bash
# Full demonstration: build, elect a leader, write data through it, kill the
# leader mid-flight, show a new leader gets elected, and prove the
# previously-committed data survived the crash. That last part is the
# entire point of Raft — this script exists to make it visible, not just
# claimed.
set -uo pipefail
cd "$(dirname "$0")/.."

CLIENT=./build/kvctl
CONF=./cluster.conf

say()  { echo; echo "=== $* ==="; }
fail() { echo "DEMO FAILED: $*" >&2; bash ./scripts/stop_cluster.sh >/dev/null 2>&1; exit 1; }

current_leader() {
    # Looks across all node logs for the most recent "became LEADER" line
    # and returns that node's id, or empty if no one has won yet.
    #
    # IMPORTANT: each node writes its own log file independently, so
    # concatenating them with `grep -h logs/node*.log` interleaves lines in
    # *filename* order (node1, node2, node3, ...), not chronological order.
    # Naively taking the last match (`tail -1` with no sort) can return a
    # stale leader from an earlier term just because its file sorts later
    # alphabetically than the file holding the true latest leader. Every log
    # line starts with a fixed-width "[HH:MM:SS.mmm]" timestamp, so a plain
    # lexicographic `sort` is exactly a chronological sort here, and fixes
    # that ordering bug.
    grep -h "became LEADER" logs/node*.log 2>/dev/null | sort | tail -1 | sed -n 's/.*node\([0-9]\+\).*/\1/p'
}

wait_for_leader() {
    local excluded="${1:-}"
    for _ in $(seq 1 100); do # up to ~20s
        local leader
        leader=$(current_leader)
        if [ -n "$leader" ] && [ "$leader" != "$excluded" ]; then
            echo "$leader"
            return 0
        fi
        sleep 0.2
    done
    echo ""
}

say "1. Building"
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release >/tmp/raft_cmake.log 2>&1 || fail "cmake failed, see /tmp/raft_cmake.log"
cmake --build build -j4 >/tmp/raft_build.log 2>&1 || fail "build failed, see /tmp/raft_build.log"
echo "build OK"

say "2. Starting 5-node cluster"
bash ./scripts/stop_cluster.sh >/dev/null 2>&1
bash ./scripts/run_cluster.sh

say "3. Waiting for initial leader election"
LEADER1=$(wait_for_leader)
[ -n "$LEADER1" ] || fail "no leader elected within timeout"
echo "leader elected: node $LEADER1"

say "4. Writing data through the leader"
$CLIENT "$CONF" SET username ayush || fail "SET username failed"
$CLIENT "$CONF" SET favorite_topic distributed-systems || fail "SET favorite_topic failed"
$CLIENT "$CONF" SET last_book bpf-performance-tools || fail "SET last_book failed"

say "5. Reading it back (sanity check before we break anything)"
OUT=$($CLIENT "$CONF" GET username) || fail "GET username failed"
echo "$OUT"
echo "$OUT" | grep -q 'value="ayush"' || fail "unexpected value for 'username': $OUT"

say "6. Killing the leader (node $LEADER1) — simulating a crash"
kill -9 "$(cat logs/node${LEADER1}.pid)"
echo "node $LEADER1 killed"

say "7. Waiting for the remaining nodes to elect a NEW leader"
LEADER2=$(wait_for_leader "$LEADER1")
[ -n "$LEADER2" ] || fail "no new leader elected after the crash"
[ "$LEADER2" != "$LEADER1" ] || fail "'new' leader is the same node we killed — election did not actually re-run"
echo "new leader elected: node $LEADER2 (old leader was node $LEADER1)"

say "8. Proving the previously-committed data survived the crash"
OUT=$($CLIENT "$CONF" GET username) || fail "GET username failed after failover"
echo "$OUT"
echo "$OUT" | grep -q 'value="ayush"' || fail "DATA LOST after failover: $OUT"

OUT=$($CLIENT "$CONF" GET last_book) || fail "GET last_book failed after failover"
echo "$OUT"
echo "$OUT" | grep -q 'value="bpf-performance-tools"' || fail "DATA LOST after failover: $OUT"

say "9. Writing new data through the NEW leader, to show the cluster is fully functional again"
$CLIENT "$CONF" SET post_failover_write still-works || fail "SET after failover failed"
OUT=$($CLIENT "$CONF" GET post_failover_write) || fail "GET after failover failed"
echo "$OUT"
echo "$OUT" | grep -q 'value="still-works"' || fail "post-failover write didn't stick: $OUT"

say "10. DEL — deleting a key and confirming it's gone"
$CLIENT "$CONF" DEL favorite_topic || fail "DEL favorite_topic failed"
OUT=$($CLIENT "$CONF" GET favorite_topic) || fail "GET after DEL failed"
echo "$OUT"
echo "$OUT" | grep -q 'value=""' || fail "key still present after DEL: $OUT"

say "11. CAS — compare-and-swap, success case"
OUT=$($CLIENT "$CONF" CAS username ayush ayush-renamed) || fail "CAS (expected to succeed) failed"
echo "$OUT"
echo "$OUT" | grep -q '^OK ' || fail "CAS should have matched and reported OK: $OUT"
OUT=$($CLIENT "$CONF" GET username) || fail "GET after successful CAS failed"
echo "$OUT"
echo "$OUT" | grep -q 'value="ayush-renamed"' || fail "CAS reported OK but value didn't change: $OUT"

say "12. CAS — compare-and-swap, mismatch case (value must NOT change)"
OUT=$($CLIENT "$CONF" CAS username wrong-expected-value should-not-be-set) || fail "CAS (expected to mismatch) failed"
echo "$OUT"
echo "$OUT" | grep -q '^MISMATCH ' || fail "CAS against the wrong expected value should have reported MISMATCH: $OUT"
OUT=$($CLIENT "$CONF" GET username) || fail "GET after mismatched CAS failed"
echo "$OUT"
echo "$OUT" | grep -q 'value="ayush-renamed"' || fail "a MISMATCHed CAS changed the value anyway: $OUT"

say "DEMO PASSED"
echo "Summary:"
echo "  - Initial leader: node $LEADER1"
echo "  - Data written and read back correctly"
echo "  - Leader killed to simulate a crash"
echo "  - New leader elected: node $LEADER2"
echo "  - All previously-committed data survived the crash intact"
echo "  - Cluster accepted new writes after failover"
echo "  - DEL removes a key"
echo "  - CAS commits and reports OK on a match, and correctly refuses (MISMATCH,"
echo "    no change) when the expected value is wrong"
echo
echo "Cluster is still running. ./scripts/stop_cluster.sh to stop it, or inspect logs/node*.log."
echo "For the read-index partition demo (the project's main new result), see:"
echo "  ./scripts/partition_demo.sh raft_node          # read-index: correctly refuses a stale read"
echo "  ./scripts/partition_demo.sh raft_node_unsafe   # Project 1's local-only check: serves one"
