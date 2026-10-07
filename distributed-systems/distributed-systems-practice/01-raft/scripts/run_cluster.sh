#!/bin/bash
# Launches all 5 nodes from cluster.conf as background processes.
# Logs go to logs/nodeN.log. PIDs go to logs/nodeN.pid.
set -euo pipefail
cd "$(dirname "$0")/.."

BIN=./build/raft_node
CONF=./cluster.conf
mkdir -p logs
rm -f logs/*.log logs/*.pid

if [ ! -x "$BIN" ]; then
    echo "raft_node binary not found — build first: cmake -B build -S . && cmake --build build" >&2
    exit 1
fi

while read -r id host port; do
    [[ "$id" =~ ^#.*$ || -z "$id" ]] && continue
    echo "starting node $id on port $port"
    "$BIN" "$CONF" "$id" > "logs/node${id}.log" 2>&1 &
    echo $! > "logs/node${id}.pid"
done < "$CONF"

echo "all nodes started. tail -f logs/node1.log to watch, or ./scripts/demo.sh for a full run."
