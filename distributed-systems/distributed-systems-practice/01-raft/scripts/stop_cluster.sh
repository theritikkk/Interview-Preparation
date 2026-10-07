#!/bin/bash
# Kills every node process started by run_cluster.sh.
set -uo pipefail
cd "$(dirname "$0")/.."

for pidfile in logs/*.pid; do
    [ -f "$pidfile" ] || continue
    pid=$(cat "$pidfile")
    if kill -0 "$pid" 2>/dev/null; then
        kill "$pid" 2>/dev/null
        echo "stopped pid $pid ($pidfile)"
    fi
    rm -f "$pidfile"
done
