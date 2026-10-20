#!/bin/bash
# Measurements for Project 14 (the numbers quoted in ANALYSIS.md). Three experiments, each
# on a fresh 9-node cluster, on a 2-CPU box with a real ext4 disk:
#
#   M1  recovery time vs log length: with NO snapshots, the WAL (and so the restart) grows
#       with history; with a snapshot every 1000 entries it stays flat.
#   M2  what fsync costs: write throughput and latency with fsync on vs off, alternating.
#   M3  snapshot size when the state machine never garbage-collects transaction
#       records (txns_/recs_ only ever grow): bank runs back to back on one cluster.
#
#   usage: scripts/measure.sh [m1|m2|m3 ...]      (default: all; ~12 minutes)
# Output goes to stdout; evidence/04-measurements.txt is a saved run.
set -uo pipefail
cd "$(dirname "$0")/.."
source scripts/lib.sh
trap './scripts/stop_shards.sh 1 2 3 >/dev/null 2>&1' EXIT
unset RAFT_SNAP_EVERY RAFT_SNAP_KEEP RAFT_FSYNC RAFT_PERSIST RAFT_NO_PERSIST_VOTE RAFT_CRASH_AT RAFT_TXN_ABLATE
WHICH=("$@"); [ ${#WHICH[@]} -eq 0 ] && WHICH=(m1 m2 m3)
want() { local w; for w in "${WHICH[@]}"; do [ "$w" = "$1" ] && return 0; done; return 1; }
now() { date +%s.%N; }
F_() { info_field "$1" "$2" "$3"; }
maxof() { local m=0 v; for v in "$@"; do [ "${v:-0}" -gt "$m" ] && m=$v; done; echo $m; }

if want m1; then
    echo "== M1. restart cost vs log length (kill -9 the whole cluster, restart on its data, 100-byte values, 1000 keys)"
    for mode in "no-snapshots:0" "snapshot-every-1000:1000"; do
        label=${mode%%:*}; every=${mode##*:}
        if [ "$every" = 0 ]; then unset RAFT_SNAP_EVERY RAFT_SNAP_KEEP; else export RAFT_SNAP_EVERY=$every RAFT_SNAP_KEEP=100; fi
        start_cluster >/dev/null
        echo "-- $label"
        printf '   %-12s %-9s %-10s %-10s %-9s %-14s %-12s %s\n' "ever-appended" "retained" "WAL-bytes" "snap-bytes" "replayed" "WAL-replay-ms" "leader-ms" "converged-ms"
        for phase in 1 2 3 4 5 6; do
            ./build/txntool load $R --threads 96 --seconds 30 --keys 1000 --value-bytes 100 > /dev/null
            read -r _ _ < <(wait_converged 1 30)
            li=$(F_ 1 1 lastIndex); lb=$(F_ 1 1 logBase); wb=$(F_ 1 1 walBytes); sb=$(F_ 1 1 snapBytes)
            for s in 1 2 3; do kill_shard $s; done
            t0=$(now)
            KEEP_DATA=1 ./scripts/run_shards.sh 1 2 3 > /dev/null
            t1=$(now)
            read -r _ _ < <(wait_converged 1 60)
            t2=$(now)
            rec=$(maxof "$(F_ 1 1 recoveredEntries)" "$(F_ 1 2 recoveredEntries)" "$(F_ 1 3 recoveredEntries)")
            ms=$(maxof "$(F_ 1 1 recoveryMs)" "$(F_ 1 2 recoveryMs)" "$(F_ 1 3 recoveryMs)")
            printf '   %-12s %-9s %-10s %-10s %-9s %-14s %-12.0f %.0f\n' "$li" "$((li - lb))" "$wb" "$sb" "$rec" "$ms" \
                "$(echo "($t1 - $t0) * 1000" | bc -l)" "$(echo "($t2 - $t0) * 1000" | bc -l)"
        done
    done
    unset RAFT_SNAP_EVERY RAFT_SNAP_KEEP
    echo "   (columns are shard 1, node 1 before the kill; replayed / WAL-replay-ms are the max over shard 1's 3 nodes AFTER the restart."
    echo "    leader-ms = until all 3 shards had a leader again; converged-ms = until shard 1's replicas had applied their whole log.)"
    echo
fi

if want m2; then
    echo "== M2. cost of fsync: 16 writer threads, 15 s per run, alternating on/off x3"
    for rep in 1 2 3; do
        for mode in on off; do
            if [ "$mode" = off ]; then export RAFT_FSYNC=0; else unset RAFT_FSYNC; fi
            start_cluster >/dev/null
            out=$(./build/txntool load $R --threads 16 --seconds 15 --keys 1000 --value-bytes 100)
            printf '   fsync %-3s run %d: %s\n' "$mode" "$rep" "${out#load: }"
            if [ "$mode" = on ]; then
                for n in 1 2 3; do
                    li=$(F_ 1 $n lastIndex); fs=$(F_ 1 $n fsyncs); fu=$(F_ 1 $n fsyncUs); ro=$(F_ 1 $n role)
                    [ "${fs:-0}" -gt 0 ] && printf '                    shard 1 node %s (%-8s): %s entries, %4s fsync calls, mean fsync %.2f ms\n' "$n" "$ro" "$li" "$fs" "$(echo "$fu / $fs / 1000" | bc -l)"
                done
            fi
        done
    done
    unset RAFT_FSYNC
    echo
fi

if want m3; then
    echo "== M3. snapshot size when transaction records are never garbage-collected (bank, 12 accounts, snapshot every 25)"
    export RAFT_SNAP_EVERY=25 RAFT_SNAP_KEEP=5
    start_cluster >/dev/null
    printf '   %-22s %-9s %-7s %-6s %-6s %-6s %-11s %s\n' "after" "committed" "keys" "locks" "txns" "recs" "snap-bytes" "WAL-bytes"
    total=0
    for run in 1 2 3 4; do
        out=$(./build/txntool bank $R --seconds 40 --seed $run)
        c=$(echo "$out" | grep -oP 'committed=\K[0-9]+' | head -1); total=$((total + c))
        read -r _ _ < <(wait_converged 1 30); read -r _ _ < <(wait_converged 2 30); read -r _ _ < <(wait_converged 3 30)
        for s in 1 2 3; do
            L=$(leader_node $s)
            printf '   %-22s %-9s %-7s %-6s %-6s %-6s %-11s %s\n' "bank run $run, shard $s" "$total" "$(F_ $s $L keys)" "$(F_ $s $L locks)" "$(F_ $s $L txns)" "$(F_ $s $L recs)" "$(F_ $s $L snapBytes)" "$(F_ $s $L walBytes)"
        done
    done
    unset RAFT_SNAP_EVERY RAFT_SNAP_KEEP
fi
