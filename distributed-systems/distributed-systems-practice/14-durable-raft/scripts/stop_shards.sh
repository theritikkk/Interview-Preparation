#!/bin/bash
# Stops every node recorded in logs/*.pid (or only the given shards') and
# waits for each to actually exit, so a following start never races it for
# the ports.   usage: scripts/stop_shards.sh [shard-id ...]
set -uo pipefail
cd "$(dirname "$0")/.."
if [ $# -eq 0 ]; then FILES=(logs/shard*-node*.pid); else FILES=(); for S in "$@"; do FILES+=(logs/shard$S-node*.pid); done; fi
for f in "${FILES[@]}"; do
    [ -f "$f" ] || continue
    pid=$(cat "$f")
    # (a zombie still answers kill -0, so look at the process state instead)
    alive() { local st; st=$(ps -o stat= -p "$1" 2>/dev/null); [ -n "$st" ] && [[ "$st" != Z* ]]; }
    if alive "$pid"; then
        kill "$pid" 2>/dev/null
        for i in $(seq 1 30); do alive "$pid" || break; sleep 0.2; done
    fi
    rm -f "$f"
done
