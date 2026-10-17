#!/bin/bash
# The abort FENCE, with and without. A resolver aborts a transaction before its
# (slow) coordinator's PREPARE reaches a participant; the late PREPARE must be
# refused. With the fence sabotaged (RAFT_TXN_ABLATE=nofence) the participant
# instead takes locks for a transaction that has already been aborted, and the
# key stays locked until somebody waits out the ttl and cleans it up.
# Restarts the cluster twice. Prints FENCE ABLATION PASSED/FAILED.
set -uo pipefail
cd "$(dirname "$0")/.."
source scripts/lib.sh
trap './scripts/stop_shards.sh 1 2 3 >/dev/null 2>&1' EXIT
TTL=1500

run_case() { # <label> -> sets LATE, LOCK, BLOCKED_MS
    $T $R SET alice 100 >/dev/null
    local txid="2.$(date +%s%3N).fence.1"    # coordinator shard 2; well-formed, so a reader can resolve it
    local body; body=$($T $R ENCODE -c alice=100 -w alice=1)
    $T $R RAW 3 WRITE TXABORT "$txid" >/dev/null          # the resolver's abort arrives FIRST
    LATE=$($T $R RAW 3 WRITE TXPREP "$txid" "$body")       # ...then the slow coordinator's PREPARE
    LOCK=$($T $R RAW 3 READ GET alice | grep -c LOCKED)   # 1 iff a raw GET on the participant shows the lock marker
    local t0; t0=$(date +%s%N)
    GOT=$($T --lock-ttl-ms $TTL $R GET alice | sed -n 's/^OK value="\(.*\)" .*/\1/p')
    BLOCKED_MS=$(( ($(date +%s%N) - t0) / 1000000 ))
}

echo "== fence ON (the real protocol)"
start_cluster >/dev/null; echo
run_case
check "late PREPARE after an abort is refused" "$LATE" ABORTED
check "alice is not locked" "$LOCK" 0
check "a client read returns at once (<500 ms)" "$([ $BLOCKED_MS -lt 500 ] && echo yes || echo no)" yes
echo "        (read took ${BLOCKED_MS} ms)"
check "no leaked prepared txn" "$(pending_count)" 0

echo; echo "== fence OFF (RAFT_TXN_ABLATE=nofence)"
RAFT_TXN_ABLATE=nofence start_cluster >/dev/null; echo
run_case
check "ABLATION: the late PREPARE is accepted" "$LATE" PREPARED
check "ABLATION: alice is locked for a txn that was already aborted" "$LOCK" 1
check "ABLATION: a client read stalls for ~the ttl until it resolves the leak" "$([ $BLOCKED_MS -ge $((TTL - 400)) ] && echo yes || echo no)" yes
echo "        (read took ${BLOCKED_MS} ms, ttl ${TTL} ms)"
check "  ...and then reads the right value" "$GOT" 100
check "  ...and the leak is gone" "$(pending_count)" 0

echo
if [ $FAILS -eq 0 ]; then echo "FENCE ABLATION PASSED"; else echo "FENCE ABLATION FAILED ($FAILS check(s))"; exit 1; fi
