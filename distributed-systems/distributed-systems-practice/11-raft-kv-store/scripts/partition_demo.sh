#!/bin/bash
# partition_demo.sh — demonstrates, by actually running it, why read-index
# linearizable reads matter. Runs the SAME partition scenario against two
# binaries built from the identical Raft core: raft_node (the real,
# read-index-protected implementation) and raft_node_unsafe (Project 1's
# original, local-only check, compiled in via RAFT_UNSAFE_READS — see
# the #ifdef in src/raft_node.cpp). See ANALYSIS.md for the full writeup.
#
# Usage: ./scripts/partition_demo.sh <raft_node|raft_node_unsafe>
set -euo pipefail
cd "$(dirname "$0")/.."

BIN="./build/${1:-raft_node}"
CONF=./cluster.conf
KVCTL=./build/kvctl
PROBE=./build/direct_probe

if [ ! -x "$BIN" ]; then
    echo "binary not found: $BIN — build first (cmake -B build -S . && cmake --build build)" >&2
    exit 1
fi
if [ ! -x "$KVCTL" ] || [ ! -x "$PROBE" ]; then
    echo "kvctl/direct_probe not found — build first" >&2
    exit 1
fi

mkdir -p logs
rm -f logs/*.log logs/*.pid logs/new_leader.conf

echo "###############################################"
echo "# Starting 5-node cluster with $(basename "$BIN")"
echo "###############################################"
while read -r id host port; do
    [[ "$id" =~ ^#.*$ || -z "$id" ]] && continue
    "$BIN" "$CONF" "$id" > "logs/node${id}.log" 2>&1 &
    echo $! > "logs/node${id}.pid"
done < "$CONF"

echo "== waiting for initial leader election =="
LEADER_ID=""
for i in $(seq 1 20); do
    sleep 0.5
    # `|| true`: under `set -eo pipefail`, grep -oP matching nothing would
    # otherwise kill the script right here — before the explicit
    # empty-check below ever runs. Found the same way as the main.cpp
    # lifecycle bug: by running this script enough times to hit it.
    LEADER_ID=$(grep -h "became LEADER" logs/node*.log 2>/dev/null | sort | tail -1 | grep -oP 'id=\K[0-9]+' || true)
    [ -n "$LEADER_ID" ] && break
done
if [ -z "$LEADER_ID" ]; then
    echo "FAILED: no leader elected within 10s" >&2
    exit 1
fi
LEADER_PID=$(cat "logs/node${LEADER_ID}.pid")
LEADER_PORT=$(grep "^${LEADER_ID} " "$CONF" | awk '{print $3}')
echo "leader is node $LEADER_ID (pid $LEADER_PID, port $LEADER_PORT)"

echo
echo "###############################################"
echo "# Writing a value via the real leader (before partition)"
echo "###############################################"
"$KVCTL" "$CONF" SET k before-partition
"$KVCTL" "$CONF" GET k

echo
echo "###############################################"
echo "# Partitioning the leader (SIGUSR1)"
echo "# deafened from every peer RPC; client connections untouched"
echo "###############################################"
kill -USR1 "$LEADER_PID"
sleep 1

echo
echo "== waiting for the remaining 4 nodes to elect a NEW leader among themselves =="
NEW_LEADER_ID=""
for i in $(seq 1 40); do
    sleep 0.5
    NL=$(grep -h "became LEADER" logs/node*.log | sort | tail -1 | grep -oP 'id=\K[0-9]+' || true)
    if [ -n "$NL" ] && [ "$NL" != "$LEADER_ID" ]; then
        NEW_LEADER_ID="$NL"
        break
    fi
done
if [ -z "$NEW_LEADER_ID" ]; then
    echo "FAILED: no new leader elected after partitioning node $LEADER_ID" >&2
    exit 1
fi
echo "new leader elected: node $NEW_LEADER_ID (node $LEADER_ID has no way to find out — it's deafened)"

echo
echo "###############################################"
echo "# Writing a NEW value through the new leader"
echo "###############################################"
NEW_LEADER_HOST=$(grep "^${NEW_LEADER_ID} " "$CONF" | awk '{print $2}')
NEW_LEADER_PORT=$(grep "^${NEW_LEADER_ID} " "$CONF" | awk '{print $3}')
# Point a scratch 1-line config straight at the new leader, so this
# write doesn't risk kvctl's retry loop getting stuck re-probing the
# still-alive-but-deafened old leader first (it would see isLeader=true
# and keep retrying it — a real, separate, honestly-noted limitation of
# the "same leader, try again" heuristic; see ANALYSIS.md).
echo "$NEW_LEADER_ID $NEW_LEADER_HOST $NEW_LEADER_PORT" > logs/new_leader.conf
"$KVCTL" logs/new_leader.conf SET k after-partition-new-leader
echo "confirmed via the new leader:"
"$KVCTL" logs/new_leader.conf GET k

echo
echo "###############################################"
echo "# The real test: ask the OLD (partitioned) leader directly"
echo "# node $LEADER_ID still believes it's the leader — it has no way to"
echo "# know otherwise. What does IT say when asked for k, unfiltered?"
echo "###############################################"
"$PROBE" 127.0.0.1 "$LEADER_PORT" "GET k"

echo
echo "== healing the partition and shutting the cluster down =="
kill -USR1 "$LEADER_PID" 2>/dev/null || true
sleep 1
for pidfile in logs/*.pid; do
    pid=$(cat "$pidfile")
    kill "$pid" 2>/dev/null || true
done
# Wait for every node to actually exit (up to ~5s) before returning, so a
# second demo run back-to-back never races these for the listening ports
# — found the hard way by running this script twice in a row; see
# ANALYSIS.md and the isRunning()/main.cpp fix it's paired with.
for pidfile in logs/*.pid; do
    pid=$(cat "$pidfile")
    for i in $(seq 1 25); do
        kill -0 "$pid" 2>/dev/null || break
        sleep 0.2
    done
done
rm -f logs/new_leader.conf
