#!/bin/bash
# Starts the 3 Raft nodes of each given shard (default: shards 1 2 3) as
# background processes that survive this script, then waits until every
# started shard has elected a leader.
#   usage: scripts/run_shards.sh [shard-id ...]
# Logs: logs/shard<S>-node<ID>.log   PIDs: logs/shard<S>-node<ID>.pid   Data: data/shard<S>-node<ID>/
#   KEEP_DATA=1  keep existing data directories and logs (restart rather than fresh start)
set -uo pipefail
cd "$(dirname "$0")/.."
BIN=./build/raft_node
[ -x "$BIN" ] || { echo "build first: cmake -B build -S . && cmake --build build -j2" >&2; exit 1; }
SHARDS=("$@"); [ ${#SHARDS[@]} -eq 0 ] && SHARDS=(1 2 3)
mkdir -p logs

for S in "${SHARDS[@]}"; do
    CONF=shards/shard$S.conf
    [ -f "$CONF" ] || { echo "no such shard config: $CONF" >&2; exit 1; }
    # Fresh start by default: wipe this shard's on-disk state and logs. KEEP_DATA=1
    # restarts the shard on whatever its data directories already hold.
    if [ -z "${KEEP_DATA:-}" ]; then
        rm -rf "${RAFT_DATA_DIR:-data}"/shard$S-node* logs/shard$S-node*.log
    fi
    rm -f logs/shard$S-node*.pid
    while read -r id host port; do
        [[ "$id" =~ ^#.*$ || -z "$id" ]] && continue
        # The wrapper records ITS OWN pid, then exec()s the node, so the pid file
        # names the real node process (not a short-lived launcher) and the node
        # outlives this script.
        setsid nohup bash -c 'echo $$ > "$0"; exec "$@"' "logs/shard$S-node$id.pid" \
            "$BIN" "$CONF" "$id" >> "logs/shard$S-node$id.log" 2>&1 < /dev/null &
    done < "$CONF"
done

leader_of() { # the node that answers INFO with role=leader (works across restarts, unlike scraping logs)
    local id host port
    while read -r id host port; do
        [[ "$id" =~ ^#.*$ || -z "$id" ]] && continue
        ./build/direct_probe "$host" "$port" INFO 2>/dev/null | grep -q 'role=leader' && { echo "$id"; return; }
    done < shards/shard$1.conf
    true
}

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
