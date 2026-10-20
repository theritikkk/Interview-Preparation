#!/bin/bash
# The durability stress test (Project 14): the Project 13 bank workload — concurrent
# money transfers over three Raft shards, checked EXACTLY — while a nemesis kills and
# restarts the servers underneath it, on-disk state and all:
#
#     t= 4s  one random node dies, comes back 3 s later
#     t=10s  one random whole shard (all 3 nodes) dies, comes back 3 s later
#     t=18s  the leader of a random shard dies
#     t=26s  ALL NINE nodes die at the same instant, then all restart (cluster-wide outage)
#     t=35s  the leader of another random shard dies
#
# Snapshots are on (every 25 applied entries, 5 kept), so the restarts exercise
# snapshot + WAL-tail recovery and InstallSnapshot for nodes that were down while the
# leader compacted. "Dies" is kill -9, or, in the power-loss variants, the TEST-ONLY
# SIGUSR2 that cuts the WAL back to what was fsync'd before exiting (see
# power_loss_demo.sh for why kill -9 alone cannot test fsync).
#
# Variants (what each must show):
#   safe               kill -9 + real durability        -> CLEAN every seed
#   safe-powerloss     power loss  + real durability    -> CLEAN every seed
#   nofsync-powerloss  power loss, fsync ablated        -> data loss DETECTED every seed
#   nopersist          kill -9, persistence switched off-> data loss DETECTED every seed
#
# Checked after every run: the bank's own exact checks (final balance of every account =
# the sum of the transfers that were acknowledged as committed, total conserved, no
# stuck locks, no unresolved transactions, no protocol violations, no unreadable
# account), PLUS: every node of every shard is up again, each shard's replicas converge
# to byte-identical state (digest), no node logged FATAL, and the faults were actually
# felt by the clients (otherwise the run would prove nothing).
#
#   usage: scripts/durable_stress.sh [seed ...]          (default seeds: 1 2 3 4 5)
#   ONLY=<variant>   run just that variant        SECONDS_PER_RUN=45
#   ~18 minutes with 5 seeds on 2 CPUs. Per-run output: logs/durable/
set -uo pipefail
cd "$(dirname "$0")/.."
source scripts/lib.sh
SEEDS=("$@"); [ ${#SEEDS[@]} -eq 0 ] && SEEDS=(1 2 3 4 5)
DUR=${SECONDS_PER_RUN:-45}
OUT=logs/durable; rm -rf "$OUT"; mkdir -p "$OUT"
trap './scripts/stop_shards.sh 1 2 3 >/dev/null 2>&1' EXIT
unset RAFT_FSYNC RAFT_PERSIST RAFT_NO_PERSIST_VOTE RAFT_CRASH_AT RAFT_TXN_ABLATE
export RAFT_SNAP_EVERY=25 RAFT_SNAP_KEEP=5

grab() { grep -oP "$2" "$1" | head -1; }
nodes_of() { local id; for id in $(node_ids "$1"); do echo "$1.$id"; done; }
# hit <signal> s.id ...  : signal them, then wait until every one is really gone
hit() {
    local sg=$1 n i; shift
    for n in "$@"; do kill -"$sg" "$(node_pid "${n%.*}" "${n#*.}")" 2>/dev/null; done
    for n in "$@"; do for i in $(seq 1 80); do node_alive "${n%.*}" "${n#*.}" || break; sleep 0.1; done; done
}
up() { local n; for n in "$@"; do node_alive "${n%.*}" "${n#*.}" || start_node "${n%.*}" "${n#*.}"; done; }

# nemesis <seed> <signal for single/shard/leader events> <signal for the cluster-wide event> <t0>
nemesis() {
    local seed=$1 sig=$2 tsig=$3 t0=$4 s id L all=() nodes=()
    RANDOM=$seed
    say() { printf 't=%02ds  %s\n' $((SECONDS - t0)) "$*"; }
    at()  { local now=$((SECONDS - t0)); [ "$1" -gt "$now" ] && sleep $(($1 - now)); return 0; }
    leader_event() {
        s=$((RANDOM % 3 + 1)); L=$(leader_node $s)
        if [ -z "$L" ]; then L=$(node_ids $s | head -1); say "shard $s has no leader right now; killing node $L instead"
        else say "kill the LEADER of shard $s (node $L): signal $sig"; fi
        hit "$sig" $s.$L; sleep 3; up $s.$L; say "node $s.$L restarted"
    }
    at 4
    s=$((RANDOM % 3 + 1)); id=$(node_ids $s | sed -n "$((RANDOM % 3 + 1))p")
    say "kill one node $s.$id: signal $sig"; hit "$sig" $s.$id; sleep 3; up $s.$id; say "node $s.$id restarted"
    at 10
    s=$((RANDOM % 3 + 1)); mapfile -t nodes < <(nodes_of $s)
    say "kill the WHOLE of shard $s: signal $sig"; hit "$sig" "${nodes[@]}"; sleep 3; up "${nodes[@]}"; say "shard $s restarted"
    at 18; leader_event
    at 26
    mapfile -t all < <(for s in 1 2 3; do nodes_of $s; done)
    say "kill ALL NINE nodes at once: signal $tsig"; hit "$tsig" "${all[@]}"; sleep 2; up "${all[@]}"; say "all nine restarted"
    at 35; leader_event
    say "nemesis done"
}

ROWS=(); FAIL=0
# variant <name> <expect: clean|loss> <signal> <cluster-wide signal> [ENV=VALUE ...]
variant() {
    local name=$1 expect=$2 sig=$3 tsig=$4; shift 4
    [ -n "${ONLY:-}" ] && [ "$ONLY" != "$name" ] && return 0
    local e; for e in "$@"; do export "$e"; done
    echo "--- $name: nemesis signal=$sig cluster-wide=$tsig  server env: RAFT_SNAP_EVERY=25 RAFT_SNAP_KEEP=5 $*"
    local det=0 n=0 clean_all=1 felt_all=1 sumsnaps=0 suminst=0 sumrec=0 sumtorn=0 sumviol=0 s
    for s in "${SEEDS[@]}"; do
        local f="$OUT/$name-seed$s.txt" nf="$OUT/$name-seed$s.nemesis.txt"
        start_cluster >/dev/null
        local t0=$SECONDS
        ./build/txntool bank $R --seconds "$DUR" --seed "$s" > "$f" 2>&1 & local bp=$!
        nemesis "$s" "$sig" "$tsig" "$t0" > "$nf" 2>&1 & local np=$!
        wait $bp; local rc=$?; wait $np
        n=$((n + 1))
        if [ $rc -ge 2 ]; then echo "  seed $s: HARNESS ERROR rc=$rc"; tail -3 "$f"; FAIL=1; continue; fi
        [ $rc -eq 1 ] && det=$((det + 1))
        # state of the servers afterwards
        local alive=0 conv=0 sh id a d digests=""
        for sh in 1 2 3; do
            for id in $(node_ids $sh); do node_alive $sh $id && alive=$((alive + 1)); done
            read -r a d < <(wait_converged $sh 40)
            if [ -n "${d:-}" ]; then conv=$((conv + 1)); digests="$digests s$sh=${d:0:8}"; fi
            d=""
        done
        local fatal snaps inst rec torn
        fatal=$(cat logs/shard*-node*.log | grep -c 'FATAL')
        snaps=$(cat logs/shard*-node*.log | grep -c 'SNAPSHOT at index')
        inst=$(cat logs/shard*-node*.log | grep -c 'INSTALLED snapshot')
        rec=$(cat logs/shard*-node*.log | grep -c 'RECOVERED from disk')
        torn=$(cat logs/shard*-node*.log | grep -c 'TORN-TAIL-CUT')
        sumsnaps=$((sumsnaps + snaps)); suminst=$((suminst + inst)); sumrec=$((sumrec + rec)); sumtorn=$((sumtorn + torn))
        local fx=$(( $(grab "$f" 'other=\K[0-9]+') + $(grab "$f" 'read-failures=\K[0-9]+') + $(grab "$f" 'in doubt after timeouts/failures: \K[0-9]+') ))
        local viol; viol=$(grab "$f" '\(\K[0-9]+(?= violations\))'); sumviol=$((sumviol + ${viol:-0}))
        local verdict_seed=""
        [ "$expect" = clean ] && {
            [ $rc -eq 0 ]    || { verdict_seed="$verdict_seed bank-violations"; clean_all=0; }
            [ $alive -eq 9 ] || { verdict_seed="$verdict_seed nodes-down($alive/9)"; clean_all=0; }
            [ $conv -eq 3 ]  || { verdict_seed="$verdict_seed not-converged($conv/3)"; clean_all=0; }
            [ "$fatal" -eq 0 ] || { verdict_seed="$verdict_seed FATAL-logged($fatal)"; clean_all=0; }
            [ $fx -gt 0 ]    || { verdict_seed="$verdict_seed fault-not-felt"; felt_all=0; }
        }
        echo "  seed $s: $(grep '^VIOLATIONS:' "$f" | cut -d' ' -f2-)"
        printf '          committed=%s fault-felt=%s | nodes-up=%s/9 shards-converged=%s/3 |%s | snapshots=%s installs=%s recoveries=%s torn-tail-cuts=%s FATAL=%s%s\n' \
            "$(grab "$f" 'committed=\K[0-9]+')" "$fx" "$alive" "$conv" "$digests" "$snaps" "$inst" "$rec" "$torn" "$fatal" "${verdict_seed:+   <== $verdict_seed}"
        if [ $rc -eq 1 ] && [ "$expect" = loss ]; then
            grep '^  e.g.' "$f" | head -2 | sed 's/^/          /'
        fi
        tar czf "$OUT/$name-seed$s-nodelogs.tgz" logs/shard*-node*.log 2>/dev/null
    done
    local verdict=ok
    case $expect in
        clean) [ $clean_all = 1 ] || { verdict="FAIL: the durable cluster lost or corrupted something"; FAIL=1; }
               [ $felt_all = 1 ]  || { verdict="FAIL: a run where clients felt no fault proves nothing"; FAIL=1; }
               [ $sumsnaps -gt 0 ] || { verdict="FAIL: no snapshot was ever taken (test did not cover compaction)"; FAIL=1; } ;;
        loss)  [ $det -eq $n ] || { verdict="FAIL: data loss went undetected in $((n - det)) of $n runs"; FAIL=1; } ;;
    esac
    printf -v row "%-18s %d/%d runs w/ violations | violations=%-4s snapshots=%-3s installs=%-3s recoveries=%-3s torn-tail-cuts=%-2s | %s" \
        "$name" "$det" "$n" "$sumviol" "$sumsnaps" "$suminst" "$sumrec" "$sumtorn" "$verdict"
    ROWS+=("$row"); echo
    for e in "$@"; do unset "${e%%=*}"; done
}

variant safe              clean KILL KILL
variant safe-powerloss    clean USR2 USR2
variant nofsync-powerloss loss  USR2 USR2 RAFT_FSYNC=0
variant nopersist         loss  KILL KILL RAFT_PERSIST=0

echo "================ SUMMARY (seeds: ${SEEDS[*]}, ${DUR}s per run, 12 accounts in 4 groups, 8 workers, 3 shards x 3 nodes) ================"
for r in "${ROWS[@]}"; do echo "$r"; done; echo
if [ $FAIL -eq 0 ]; then
    if [ -n "${ONLY:-}" ]; then echo "DURABLE STRESS PASSED (partial run: ONLY=$ONLY)"
    else echo "DURABLE STRESS PASSED: the real protocol is clean in every run (kill -9 and power loss, incl. a cluster-wide outage); both ablations are caught"; fi
else echo "DURABLE STRESS FAILED"; exit 1; fi
