#!/bin/bash
# The main experiment: concurrent money transfers between accounts spread over 3
# shards, checked exactly (see src/txntool.cpp). Runs the real protocol — with and
# without a shard leader being kill -9'd mid-run — and four deliberately broken
# variants, over several seeds, and asserts:
#   * the real protocol produces ZERO violations in every run, and
#   * every broken variant IS caught, with the symptom its sabotage should cause.
#
#   usage: scripts/bank_stress.sh [seed ...]       (default seeds: 1 2 3 4 5)
#   ONLY=<variant>    run just that variant (safe, safe-leaderkill, naive, plain-audit, ablate-nolock)
#   KILL_SHARD=<n>    in safe-leaderkill, always kill shard n's leader (default: rotate 2,3,1,...)
#   ~12 minutes with 5 seeds on 2 CPUs. Per-run output: logs/bank/
set -uo pipefail
cd "$(dirname "$0")/.."
source scripts/lib.sh
SEEDS=("$@"); [ ${#SEEDS[@]} -eq 0 ] && SEEDS=(1 2 3 4 5)
SECONDS_PER_RUN=20
OUT=logs/bank; rm -rf "$OUT"; mkdir -p "$OUT"
trap './scripts/stop_shards.sh 1 2 3 >/dev/null 2>&1' EXIT

field() { grep '^VIOLATIONS:' "$1" | grep -oP "$2=\K[0-9]+" | head -1; }
sumf() { local t=0 f; for f in "$OUT/$1"-seed*.txt; do t=$((t + $(field "$f" "$2"))); done; echo $t; }
grab() { grep -oP "$2" "$1" | head -1; }
ROWS=(); FAIL=0

# variant <name> <expect> <ablate-env or -> <kill: yes|no> <bank args...>
variant() {
    local name=$1 expect=$2 ablate=$3 kill=$4; shift 4
    [ -n "${ONLY:-}" ] && [ "$ONLY" != "$name" ] && return 0
    echo "--- $name: txntool bank $*   server-side sabotage: ${ablate/-/none}   kill a shard leader mid-run: $kill"
    local det=0 n=0 effect=0 committed=0 cross=0
    if [ "$kill" = no ]; then # one cluster for all seeds of this variant
        if [ "$ablate" = - ]; then start_cluster >/dev/null; else RAFT_TXN_ABLATE=$ablate start_cluster >/dev/null; fi
    fi
    for s in "${SEEDS[@]}"; do
        local f="$OUT/$name-seed$s.txt"
        if [ "$kill" = yes ]; then
            start_cluster >/dev/null
            local victim=${KILL_SHARD:-$(( s % 3 + 1 ))}
            ./build/txntool bank $R --seconds $SECONDS_PER_RUN --seed "$s" "$@" > "$f" 2>&1 &
            local pid=$!
            sleep 7
            local leader=$(grep -h "became LEADER" logs/shard$victim-node*.log | sort | tail -1 | grep -oP 'id=\K[0-9]+')
            local npid=$(cat logs/shard$victim-node$leader.pid)
            kill -9 "$npid"; echo "  seed $s: kill -9 leader of shard $victim (node $leader, pid $npid) at t=7s" 
            wait $pid; local rc=$?
        else
            ./build/txntool bank $R --seconds $SECONDS_PER_RUN --seed "$s" "$@" > "$f" 2>&1; local rc=$?
        fi
        n=$((n + 1))
        [ $rc -ge 2 ] && { echo "  seed $s: HARNESS ERROR rc=$rc"; tail -3 "$f"; FAIL=1; continue; }
        [ $rc -eq 1 ] && det=$((det + 1))
        local fx=$(( $(grab "$f" 'other=\K[0-9]+') + $(grab "$f" 'read-failures=\K[0-9]+') + $(grab "$f" 'in doubt after timeouts/failures: \K[0-9]+') ))
        effect=$((effect + fx))
        echo "  seed $s: $(grep '^VIOLATIONS:' "$f" | cut -d' ' -f2-)  committed=$(grab "$f" 'committed=\K[0-9]+') fault-felt=$fx"
    done
    local a=$(sumf "$name" audit-skew) fp=$(sumf "$name" final-per-account) ft=$(sumf "$name" final-total) st=$(sumf "$name" stuck-locks)
    local pr=$(sumf "$name" protocol) un=$(sumf "$name" unresolved-txns) ng=$(sumf "$name" negative-balance)
    local verdict=ok
    case $expect in
        clean)       [ $det -eq 0 ] || { verdict="FAIL: the real protocol violated correctness"; FAIL=1; }
                     if [ "$kill" = yes ] && [ $effect -eq 0 ]; then verdict="FAIL: the kill had no visible effect (test would prove nothing)"; FAIL=1; fi ;;
        naive)       [ $((fp + ft)) -gt 0 ] || { verdict="FAIL: lost updates not detected"; FAIL=1; } ;;
        plain-audit) { [ "$a" -gt 0 ] && [ $((fp + ft)) -eq 0 ]; } || { verdict="FAIL: expected read skew ONLY (audit-skew>0, exact final state)"; FAIL=1; } ;;
        nolock)      [ $((fp + ft)) -gt 0 ] || { verdict="FAIL: lost updates not detected"; FAIL=1; } ;;
    esac
    printf -v row "%-18s %d/%d runs w/ violations | audit-skew=%-4s final-total=%-2s final-per-account=%-3s stuck=%s unresolved=%s protocol=%s | %s" \
        "$name" "$det" "$n" "$a" "$ft" "$fp" "$st" "$un" "$pr" "$verdict"
    ROWS+=("$row"); echo
}

variant safe              clean       -      no
variant safe-leaderkill   clean       -      yes
variant naive             naive       -      no  --naive
variant plain-audit       plain-audit -      no  --plain-audit
variant ablate-nolock     nolock      nolock no

echo "================ SUMMARY (seeds: ${SEEDS[*]}, ${SECONDS_PER_RUN}s per run, 12 accounts in 4 groups, 8 workers) ================"
for r in "${ROWS[@]}"; do echo "$r"; done; echo
if [ $FAIL -eq 0 ]; then echo "BANK STRESS PASSED: real protocol clean in every run (including with a shard leader killed); every broken variant caught with its expected symptom"
else echo "BANK STRESS FAILED"; exit 1; fi
