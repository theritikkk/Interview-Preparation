#!/bin/bash
# Kill the coordinator at every dangerous point of two-phase commit and show
# that another client finishes the job correctly from the replicated decision
# record — and that the same crash WITHOUT the record breaks atomicity.
# Starts a fresh 3-shard cluster. Prints CRASH RECOVERY DEMO PASSED/FAILED.
set -uo pipefail
cd "$(dirname "$0")/.."
source scripts/lib.sh
trap './scripts/stop_shards.sh 1 2 3 >/dev/null 2>&1' EXIT
start_cluster
echo
TTL=1500   # a lock must be this old before a reader treats its coordinator as dead
# (val() in lib.sh reads through the helping client with the DEFAULT 3000 ms ttl, so
#  any val of a key that is still locked will itself wait and resolve the lock.)
reset() { $T $R SET alice 100 >/dev/null; $T $R SET carol 50 >/dev/null; }
# alice lives on shard 3, carol on shard 2 -> a transfer between them is a 2PC with
# coordinator/record on shard 2 (lowest participant id).
check "placement: alice on shard 3" "$(shard_of alice)" 3
check "placement: carol on shard 2" "$(shard_of carol)" 2
XFER="-c alice=100 -c carol=50 -w alice=70 -w carol=80"

echo; echo "== A. coordinator dies AFTER PREPARE: locks are held, no decision exists"
reset
$T --crash-at after-prepare $R TXN $XFER >/dev/null 2>&1; check "coordinator exit status (86 = injected crash)" "$?" 86
check "both shards hold a prepared txn" "$(pending_count)" 2
check "plain SET at the locked key is refused" "$($T $R RAW 3 WRITE SET alice 5 | cut -c2-7)" LOCKED
check "plain GET at the locked key does not return a value" "$($T $R RAW 3 READ GET alice | cut -c2-7)" LOCKED
$T $R TXN -c alice=100 -w alice=99 >/dev/null; check "a competing transaction is aborted (blocked)" "$?" 3
# NB: nothing may READ alice through the helping client before this point, or it would do the resolving itself.
T0=$(date +%s%N)
GOT=$($T --lock-ttl-ms $TTL $R GET alice | sed -n 's/^OK value="\(.*\)" .*/\1/p')
echo "        (that read blocked for $(( ($(date +%s%N) - T0) / 1000000 )) ms: it waited out the remaining ttl, then ran the resolution)"
check "a reader past the ttl resolves it: alice rolled BACK" "$GOT" 100
check "carol rolled back too" "$(val carol)" 50
check "no locks left" "$(pending_count)" 0

echo; echo "== B. coordinator dies AFTER the decision (COMMITTED) is recorded, before telling any participant"
reset
$T --crash-at after-decide $R TXN $XFER >/dev/null 2>&1; check "coordinator exit status" "$?" 86
check "both shards still hold the txn prepared" "$(pending_count)" 2
check "a reader resolves it: alice rolled FORWARD" "$($T --lock-ttl-ms $TTL $R GET alice | sed -n 's/^OK value="\(.*\)" .*/\1/p')" 70
check "carol rolled forward too" "$(val carol)" 80
check "no locks left" "$(pending_count)" 0

echo; echo "== C. coordinator dies MID-COMMIT: told shard 2 to commit, died before telling shard 3"
reset
$T --crash-at mid-commit $R TXN $XFER >/dev/null 2>&1; check "coordinator exit status" "$?" 86
check "carol (shard 2) already shows the new value" "$(val carol)" 80
check "alice (shard 3) is still locked" "$($T $R RAW 3 READ GET alice | cut -c2-7)" LOCKED
check "one prepared txn left" "$(pending_count)" 1
check "a reader resolves it: alice rolled FORWARD to match carol" "$($T --lock-ttl-ms $TTL $R GET alice | sed -n 's/^OK value="\(.*\)" .*/\1/p')" 70
check "money conserved (alice + carol)" "$(( $(val alice) + $(val carol) ))" 150
check "no locks left" "$(pending_count)" 0

echo; echo "== D. ABLATION: the same mid-commit crash, but the resolver has no decision record and guesses ABORT"
reset
$T --crash-at mid-commit $R TXN $XFER >/dev/null 2>&1; check "coordinator exit status" "$?" 86
$T --recovery guess-abort --lock-ttl-ms $TTL $R GET alice >/dev/null
A=$(val alice); C=$(val carol)
echo "        alice=$A carol=$C  (a correct run ends with 70 and 80)"
check "ATOMICITY BROKEN as expected: shard 3 aborted, shard 2 committed" "$A/$C" "100/80"
check "money is NOT conserved (150 expected)" "$(( A + C ))" 180

echo; echo "== E. a LIVE coordinator is slow; an impatient resolver aborts it first (premature abort must be safe)"
reset
( $T --pause-after-prepare-ms 4000 $R TXN $XFER > /tmp/coord_e.$$ 2>&1; echo "rc=$?" >> /tmp/coord_e.$$ ) &
sleep 1.8
check "the txn is prepared and waiting" "$(pending_count)" 2
$T $R SWEEP 500 >/dev/null
wait
check_contains "the slow coordinator learns it was aborted (fenced out)" "$(cat /tmp/coord_e.$$)" "ABORTED (fenced)"
check_contains "...and exits with 'aborted', not 'committed'" "$(cat /tmp/coord_e.$$)" "rc=3"
rm -f /tmp/coord_e.$$
check "alice untouched" "$(val alice)" 100; check "carol untouched" "$(val carol)" 50
check "no locks left" "$(pending_count)" 0

echo
if [ $FAILS -eq 0 ]; then echo "CRASH RECOVERY DEMO PASSED"; else echo "CRASH RECOVERY DEMO FAILED ($FAILS check(s))"; exit 1; fi
