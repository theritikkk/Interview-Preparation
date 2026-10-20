#!/bin/bash
# Does fsync matter? (Project 14.) kill -9 cannot answer that: the kernel's page cache
# outlives the process, so even a node that never calls fsync keeps every write across a
# kill -9. What fsync buys is surviving the machine losing power. This box cannot lose
# power on demand, so the nodes have a TEST-ONLY SIGUSR2 that does what power loss does:
# cuts the WAL back to the last size that was fsync'd (nothing else is lost) and exits.
# The fsync calls themselves are real; only the "power cut" is simulated.
# Three fresh clusters:  A fsync ON + power loss   B fsync OFF + power loss   C fsync OFF + kill -9
# Prints POWER LOSS DEMO PASSED / FAILED.
set -uo pipefail
cd "$(dirname "$0")/.."
source scripts/lib.sh
trap './scripts/stop_shards.sh 1 2 3 >/dev/null 2>&1' EXIT
unset RAFT_SNAP_EVERY RAFT_SNAP_KEEP RAFT_PERSIST RAFT_NO_PERSIST_VOTE RAFT_CRASH_AT RAFT_FSYNC
N=60
yes_no() { if "$@"; then echo yes; else echo no; fi; }
fill() { local t; for t in 1 2 3 4 5; do ./build/txntool init $R $N "$1" >/dev/null && return 0; sleep 2; done; return 1; }
count_value() { local n=0 i; for i in $(seq 0 $((N - 1))); do [ "$(val acct-$i)" = "$1" ] && n=$((n + 1)); done; echo $n; }
cut_all() { # <signal> : hit all nine nodes at once, wait until they are gone
    local s id; for s in 1 2 3; do for id in $(node_ids $s); do kill -"$1" "$(node_pid $s $id)" 2>/dev/null; done; done
    sleep 1.5; for s in 1 2 3; do for id in $(node_ids $s); do node_alive $s $id && { echo "node $s.$id survived"; return 1; }; done; done
    return 0   # without this the status of the LAST `node_alive` (dead => 1) leaks out as a failure
}
restart_all() { KEEP_DATA=1 ./scripts/run_shards.sh 1 2 3 >/dev/null; }

scenario() { # <label> <fsync env value> <signal> <expected surviving count>
    echo; echo "== $1"
    if [ "$2" = off ]; then export RAFT_FSYNC=0; else unset RAFT_FSYNC; fi
    start_cluster >/dev/null
    check "writes acknowledged by the cluster ($N accounts set to 7777)" "$(yes_no fill 7777)" yes
    check "they read back before the cut" "$(count_value 7777)" $N
    local fs; fs=$(info_field 3 1 fsyncs)
    echo "   (shard 3 node 1 has made $fs fsync calls so far)"
    cut_all "$3" || FAILS=$((FAILS + 1))
    restart_all
    check "$4" "$(count_value 7777)" "$5"
}
scenario "A. fsync ON, then every node loses power"            on  USR2 "every acknowledged write survived the power loss" $N
scenario "B. fsync OFF, then every node loses power"           off USR2 "ABLATION: acknowledged writes are GONE after the power loss (0 survive)" 0
scenario "C. fsync OFF, but the nodes are only kill -9'd"      off KILL "(control) kill -9 alone loses nothing, which is why kill -9 cannot test fsync" $N

echo
if [ $FAILS -eq 0 ]; then echo "POWER LOSS DEMO PASSED"; else echo "POWER LOSS DEMO FAILED ($FAILS check(s))"; exit 1; fi
