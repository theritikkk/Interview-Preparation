#!/bin/bash
# Kills every node process started by run_cluster.sh, and waits for each
# to actually exit before returning — a node takes up to ~1s to shut down
# (acceptorLoop's accept() only wakes up to recheck running_ once a
# second), and starting a new cluster before the old one's ports are
# truly free races for them. Found by running this back-to-back with
# partition_demo.sh; see that script and ANALYSIS.md.
set -uo pipefail
cd "$(dirname "$0")/.."

for pidfile in logs/*.pid; do
    [ -f "$pidfile" ] || continue
    pid=$(cat "$pidfile")
    if kill -0 "$pid" 2>/dev/null; then
        kill "$pid" 2>/dev/null
        echo "stopped pid $pid ($pidfile)"
        for i in $(seq 1 25); do
            kill -0 "$pid" 2>/dev/null || break
            sleep 0.2
        done
    fi
    rm -f "$pidfile"
done
