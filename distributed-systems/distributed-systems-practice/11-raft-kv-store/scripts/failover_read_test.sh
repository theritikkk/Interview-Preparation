#!/bin/bash
# failover_read_test.sh — does a freshly elected leader ever answer a GET with
# data OLDER than a write the previous leader already acknowledged?
#
# Scenario, repeated N times against one binary:
#   1. start a 5-node cluster, wait for a leader
#   2. SET k <unique value> through it (kvctl returns only after commit)
#   3. kill -9 that leader IMMEDIATELY — before followers have necessarily
#      learned the new commitIndex (they only learn it on the next heartbeat)
#   4. hammer EVERY surviving node with raw GETs in a tight loop (direct_probe:
#      no retries, no redirects) and record the very FIRST answer with ok=true
#   5. that first answer must be the value we wrote; anything else is a
#      linearizability violation (a read that began after the write completed
#      saw older state)
#
# Usage: ./scripts/failover_read_test.sh <raft_node|raft_node_unsafe> [trials]
set -uo pipefail
cd "$(dirname "$0")/.."

BIN="./build/${1:-raft_node}"
TRIALS="${2:-10}"
CONF=./cluster.conf
KVCTL=./build/kvctl
PROBE=./build/direct_probe
[ -x "$BIN" ] && [ -x "$KVCTL" ] && [ -x "$PROBE" ] || { echo "build first" >&2; exit 2; }

stale=0; correct=0; inconclusive=0

for t in $(seq 1 "$TRIALS"); do
    bash ./scripts/stop_cluster.sh >/dev/null 2>&1
    mkdir -p logs; rm -f logs/*.log logs/*.pid
    while read -r id host port; do
        [[ "$id" =~ ^#.*$ || -z "$id" ]] && continue
        "$BIN" "$CONF" "$id" > "logs/node${id}.log" 2>&1 &
        echo $! > "logs/node${id}.pid"
    done < "$CONF"

    L1=""
    for i in $(seq 1 40); do
        sleep 0.5
        L1=$(grep -h "became LEADER" logs/node*.log 2>/dev/null | sort | tail -1 | grep -oP 'id=\K[0-9]+' || true)
        [ -n "$L1" ] && break
    done
    [ -n "$L1" ] || { echo "trial $t: no initial leader"; inconclusive=$((inconclusive+1)); continue; }
    sleep 1 # let the cluster settle so the SET below is the only recent write

    # Pre-seed an OLD value and let it fully propagate (every node's commitIndex
    # catches up), so a stale read shows up as a distinct, non-empty "old-..."
    # value rather than an ambiguous empty string. (An earlier version of this
    # script looped on "value is non-empty", which silently skipped exactly the
    # stale-empty answers it exists to catch — see ANALYSIS.md.)
    OLD="old-$t"
    "$KVCTL" "$CONF" SET k "$OLD" >/dev/null 2>&1 || { echo "trial $t: seed SET failed"; inconclusive=$((inconclusive+1)); continue; }
    sleep 1
    WANT="new-$t-$RANDOM"
    "$KVCTL" "$CONF" SET k "$WANT" >/dev/null 2>&1 || { echo "trial $t: SET failed"; inconclusive=$((inconclusive+1)); continue; }
    kill -9 "$(cat logs/node${L1}.pid)"   # immediately

    # tight loop over the survivors; first ok=true answer wins
    GOT=""; SERVED=""
    deadline=$(( $(date +%s) + 15 ))
    while [ -z "$SERVED" ] && [ "$(date +%s)" -lt "$deadline" ]; do
        while read -r id host port; do
            [[ "$id" =~ ^#.*$ || -z "$id" ]] && continue
            [ "$id" = "$L1" ] && continue
            out=$("$PROBE" "$host" "$port" "GET k" 2>/dev/null || true)
            if [[ "$out" == ok=true* ]]; then
                GOT=$(echo "$out" | sed -n 's/.*value="\(.*\)"$/\1/p')
                SERVED=$id
                break
            fi
        done < "$CONF"
    done

    if [ -z "$SERVED" ]; then
        echo "trial $t: inconclusive (no survivor answered ok=true in 15s)"
        inconclusive=$((inconclusive+1))
    elif [ "$GOT" = "$WANT" ]; then
        echo "trial $t: ok     first answer from node $SERVED = \"$GOT\""
        correct=$((correct+1))
    else
        echo "trial $t: STALE  first answer from node $SERVED = \"$GOT\"  (acknowledged write was \"$WANT\", previous value \"$OLD\")"
        stale=$((stale+1))
    fi
done

bash ./scripts/stop_cluster.sh >/dev/null 2>&1
echo
echo "$(basename "$BIN"): $TRIALS trials -> correct=$correct stale=$stale inconclusive=$inconclusive"
[ "$stale" -eq 0 ]
