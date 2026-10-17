#!/bin/bash
# Starts the 3 Raft nodes of each given shard (default: shards 1 2 3) as
# background processes that survive this script, then waits until every
# started shard has elected a leader.
#   usage: scripts/run_shards.sh [shard-id ...]
# Logs: logs/shard<S>-node<ID>.log   PIDs: logs/shard<S>-node<ID>.pid
set -uo pipefail
cd "$(dirname "$0")/.."
BIN=./build/raft_node
[ -x "$BIN" ] || { echo "build first: cmake -B build -S . && cmake --build build -j2" >&2; exit 1; }
SHARDS=("$@"); [ ${#SHARDS[@]} -eq 0 ] && SHARDS=(1 2 3)
mkdir -p logs

for S in "${SHARDS[@]}"; do
    CONF=shards/shard$S.conf
    [ -f "$CONF" ] || { echo "no such shard config: $CONF" >&2; exit 1; }
    rm -f logs/shard$S-node*.log logs/shard$S-node*.pid
    while read -r id host port; do
        [[ "$id" =~ ^#.*$ || -z "$id" ]] && continue
        # The wrapper records ITS OWN pid, then exec()s the node, so the pid file
        # names the real node process (not a short-lived launcher) and the node
        # outlives this script.
        setsid nohup bash -c 'echo $$ > "$0"; exec "$@"' "logs/shard$S-node$id.pid" \
            "$BIN" "$CONF" "$id" > "logs/shard$S-node$id.log" 2>&1 < /dev/null &
    done < "$CONF"
done

leader_of() { grep -h "became LEADER" logs/shard$1-node*.log 2>/dev/null | sort | tail -1 | grep -oP 'id=\K[0-9]+' || true; }

for S in "${SHARDS[@]}"; do
    L=""
    for i in $(seq 1 60); do
        sleep 0.5
        L=$(leader_of "$S"); [ -n "$L" ] && break
    done
    if [ -z "$L" ]; then
        echo "shard $S: NO LEADER after 30s" >&2
        tail -n 3 logs/shard$S-node*.log >&2
        exit 1
    fi
    echo "shard $S: leader is node $L"
done
