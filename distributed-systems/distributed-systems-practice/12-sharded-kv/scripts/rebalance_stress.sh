#!/bin/bash
# Live-traffic rebalance experiment: add shard 4 to a 3-shard cluster WHILE
# 16 client threads hammer 100-200 keys with GET/SET/DEL/CAS, and check every
# single read and CAS against an exact model (see `shardtool stress`).
#
# It runs the SAFE protocol and five deliberately broken variants over several
# seeds and asserts:
#   * the safe protocol produces ZERO violations in every run, and
#   * every broken variant produces violations (so the checker demonstrably can
#     fail), and the symptom matches the protocol step that was removed.
#
#   usage: scripts/rebalance_stress.sh [seed ...]      (default seeds: 1 2 3 4 5)
#   takes ~10 minutes with 5 seeds on 2 CPUs.  Per-run output: logs/stress/
set -uo pipefail
cd "$(dirname "$0")/.."
SEEDS=("$@"); [ ${#SEEDS[@]} -eq 0 ] && SEEDS=(1 2 3 4 5)
TOOL=./build/shardtool
[ -x "$TOOL" ] || { echo "build first: cmake -B build -S . && cmake --build build -j2" >&2; exit 1; }
OUT=logs/stress
rm -rf "$OUT"; mkdir -p "$OUT"

cleanup() { ./scripts/stop_shards.sh 1 2 3 4 >/dev/null 2>&1; }
trap cleanup EXIT
./scripts/stop_shards.sh 1 2 3 4 >/dev/null 2>&1
./scripts/run_shards.sh 1 2 3 4 || { echo "cluster failed to start" >&2; exit 1; }
echo

FAIL=0
ROWS=()

cat_sum() { # file-glob category -> total across runs
    local total=0 v
    for f in $1; do
        v=$(grep -oP "^VIOLATIONS:.*?\b$2=\K[0-9]+" "$f" | head -1)
        total=$((total + ${v:-0}))
    done
    echo "$total"
}

# variant <name> <expect: clean|violations> <signature-category|any> <stress args...>
variant() {
    local name=$1 expect=$2 sig=$3; shift 3
    local det=0 n=0 rc
    echo "--- $name:  stress $*"
    for s in "${SEEDS[@]}"; do
        local f="$OUT/$name-seed$s.txt"
        $TOOL stress ring.conf --seed "$s" "$@" > "$f" 2>&1; rc=$?
        n=$((n + 1))
        if [ $rc -ge 2 ]; then
            echo "  seed $s: HARNESS ERROR (exit $rc) — see $f"; tail -n 3 "$f"; FAIL=1; continue
        fi
        [ $rc -eq 1 ] && det=$((det + 1))
        echo "  seed $s: $(grep -oP '^VIOLATIONS: \K.*?(?=   \()' "$f")"
    done
    local g="$OUT/$name-seed*.txt"
    local vv=$(cat_sum "$g" vanished-reads) ss=$(cat_sum "$g" stale-reads) rr=$(cat_sum "$g" resurrected-reads)
    local cc=$(cat_sum "$g" cas-wrong) fm=$(cat_sum "$g" final-state-mismatch) st=$(cat_sum "$g" stray-copies-on-old-shards)
    local verdict="ok"
    if [ "$expect" = clean ]; then
        [ $det -eq 0 ] || { verdict="FAIL: safe protocol violated correctness"; FAIL=1; }
    else
        [ $det -ge 1 ] || { verdict="FAIL: checker never detected the broken variant"; FAIL=1; }
        case $sig in
            vanished-reads)    [ "$vv" -gt 0 ] || { verdict="FAIL: expected vanished reads"; FAIL=1; } ;;
            resurrected-reads) [ "$rr" -gt 0 ] || { verdict="FAIL: expected resurrected reads"; FAIL=1; } ;;
            stale-reads)       [ "$ss" -gt 0 ] || { verdict="FAIL: expected stale reads"; FAIL=1; } ;;
        esac
    fi
    printf -v row "%-20s %2d/%-2d runs with violations | vanished=%-3s stale=%-3s resurrected=%-3s cas-wrong=%-3s final-mismatch=%s stray=%s | %s" \
        "$name" "$det" "$n" "$vv" "$ss" "$rr" "$cc" "$fm" "$st" "$verdict"
    ROWS+=("$row")
    echo
}

variant safe-natural        clean      any
variant safe-hostile        clean      any               --keys 200 --read-gap-ms 600 --step-delay-ms 300
variant naive               violations any               --naive
variant ablate-norecheck    violations vanished-reads    --ablate norecheck --read-gap-ms 600
variant ablate-notombstone  violations resurrected-reads --keys 200 --ablate notombstone --step-delay-ms 300
variant ablate-overwrite    violations stale-reads       --ablate overwrite --step-delay-ms 200

echo "================ SUMMARY (seeds: ${SEEDS[*]}) ================"
for r in "${ROWS[@]}"; do echo "$r"; done
echo
if [ $FAIL -eq 0 ]; then
    echo "STRESS PASSED: safe protocol clean in every run; every broken variant detected with its expected symptom"
else
    echo "STRESS FAILED"
    exit 1
fi
