#!/bin/bash
# Functional demo + protocol unit checks for cross-shard transactions.
# Starts a fresh 3-shard cluster, asserts every result, prints TXN DEMO PASSED/FAILED.
set -uo pipefail
cd "$(dirname "$0")/.."
source scripts/lib.sh
trap './scripts/stop_shards.sh 1 2 3 >/dev/null 2>&1' EXIT
start_cluster
echo

echo "== 1. placement (the test needs keys on three different shards)"
check "alice -> shard 3" "$(shard_of alice)" 3
check "bob   -> shard 3" "$(shard_of bob)" 3
check "carol -> shard 2" "$(shard_of carol)" 2
check "dave  -> shard 1" "$(shard_of dave)" 1
$T $R SET alice 100 >/dev/null; $T $R SET carol 50 >/dev/null; $T $R SET dave 0 >/dev/null

echo; echo "== 2. one transaction across THREE shards commits atomically (2PC)"
OUT=$($T $R TXN -c alice=100 -c carol=50 -c dave=0 -w alice=70 -w carol=80 -w dave=1); RC=$?
check "exit status (0 = committed)" "$RC" 0
check_contains "went through 2PC" "$OUT" "cross-shard (2PC)"
check "alice" "$(val alice)" 70; check "carol" "$(val carol)" 80; check "dave" "$(val dave)" 1

echo; echo "== 3. a stale compare aborts the WHOLE transaction and leaves no trace"
OUT=$($T $R TXN -c alice=100 -c carol=80 -w alice=0 -w carol=999); RC=$?
check "exit status (3 = aborted)" "$RC" 3
check_contains "reason" "$OUT" "ABORTED (mismatch)"
check "alice unchanged" "$(val alice)" 70; check "carol unchanged" "$(val carol)" 80
check "no locks left behind" "$(pending_count)" 0

echo; echo "== 4. single-shard transactions skip 2PC: one log entry"
OUT=$($T $R TXN -c bob= -c alice=70 -w bob=7 -w alice=63); RC=$?
check "exit status" "$RC" 0
check_contains "fast path" "$OUT" "single-shard (1 log entry)"
check "alice" "$(val alice)" 63; check "bob" "$(val bob)" 7

echo; echo "== 5. create-if-absent and erase, across shards, in one transaction"
OUT=$($T $R TXN -c erin= -w erin=new -w dave=); RC=$?
check "exit status" "$RC" 0
check "erin created" "$(val erin)" new; check "dave erased" "$(val dave)" ""
OUT=$($T $R TXN -c erin= -w erin=again -w carol=81); RC=$?
check "create-if-absent on an existing key aborts" "$RC" 3
check "carol untouched by the aborted txn" "$(val carol)" 80

echo; echo "== 6. bad input is refused, not mangled"
OUT=$($T $R TXN -w "a b=1" -w carol=1); RC=$?
check "key with a space" "$RC" 3; check_contains "reason" "$OUT" "ABORTED (invalid)"

echo; echo "== 7. protocol commands are idempotent (raw commands straight at one shard, shard 3)"
BODY=$($T $R ENCODE -c alice=63 -w alice=62)
P() { $T $R RAW 3 WRITE "$@"; }
check "PREPARE"                         "$(P TXPREP t7.1.x.1 "$BODY")" PREPARED
check "PREPARE again (retry)"           "$(P TXPREP t7.1.x.1 "$BODY")" PREPARED
check "key is locked while prepared"    "$($T $R RAW 3 READ GET alice | cut -c2-7)" LOCKED
check "plain SET on locked key refused" "$($T $R RAW 3 WRITE SET alice 5 | cut -c2-7)" LOCKED
check "a different txn cannot prepare it" "$(P TXPREP t7.2.x.1 "$BODY" | cut -d' ' -f1)" LOCKED
check "COMMIT"                          "$(P TXCOMMIT t7.1.x.1)" OK
check "  alice has the committed value" "$(val alice)" 62
check "plain SET now allowed (lock released)" "$($T $R SET alice 99 | cut -c1-2)" OK
check "COMMIT again (a retried message)" "$(P TXCOMMIT t7.1.x.1)" OK
check "  NOT re-applied (would have overwritten 99 with 62)" "$(val alice)" 99
$T $R SET alice 62 >/dev/null
check "ABORT after COMMIT is an error"  "$(P TXABORT t7.1.x.1)" ERR_COMMITTED
check "COMMIT of unknown txn"           "$(P TXCOMMIT t7.9.x.1)" ERR_UNKNOWN
BODY2=$($T $R ENCODE -c alice=62 -w alice=61)
check "PREPARE #2"                      "$(P TXPREP t7.3.x.1 "$BODY2")" PREPARED
check "ABORT"                           "$(P TXABORT t7.3.x.1)" OK
check "ABORT again"                     "$(P TXABORT t7.3.x.1)" OK
check "COMMIT after ABORT is an error"  "$(P TXCOMMIT t7.3.x.1)" ERR_ABORTED
check "abort released the lock"         "$(val alice)" 62
check "TXBEGIN creates the decision record" "$($T $R RAW 2 WRITE TXBEGIN t7.4.x.1 2,3)" "PENDING 2,3"
check "  TXBEGIN again (retry)"                "$($T $R RAW 2 WRITE TXBEGIN t7.4.x.1 2,3)" "PENDING 2,3"
check "  first decision wins: COMMIT"          "$($T $R RAW 2 WRITE TXDECIDE t7.4.x.1 COMMIT | cut -d' ' -f1)" COMMITTED
check "  a later ABORT only reads it back"    "$($T $R RAW 2 WRITE TXDECIDE t7.4.x.1 ABORT | cut -d' ' -f1)" COMMITTED
check "  COMMIT without a record"             "$($T $R RAW 2 WRITE TXDECIDE t7.5.x.1 COMMIT)" NOREC
check "  ABORT without a record fences the txn" "$($T $R RAW 2 WRITE TXDECIDE t7.5.x.1 ABORT | cut -d' ' -f1)" ABORTED
check "  ... so the coordinator's COMMIT now fails" "$($T $R RAW 2 WRITE TXDECIDE t7.5.x.1 COMMIT | cut -d' ' -f1)" ABORTED
BODY3=$($T $R ENCODE -c bob=7 -w bob=8)
check "TXONE"                           "$(P TXONE t7.6.x.1 "$BODY3")" COMMITTED
check "TXONE retried: not applied twice" "$(P TXONE t7.6.x.1 "$BODY3")" COMMITTED
check "  value"                         "$(val bob)" 8

echo; echo "== 8. the abort FENCE: an abort that arrives before its PREPARE stops the PREPARE"
BODY4=$($T $R ENCODE -c alice=62 -w alice=1)
check "TXABORT for a txn never seen"    "$(P TXABORT t8.1.x.1)" OK
check "the late PREPARE is refused"     "$(P TXPREP t8.1.x.1 "$BODY4")" ABORTED
check "so alice is not locked"          "$(val alice)" 62

echo
check "no prepared transactions left anywhere" "$(pending_count)" 0
echo
if [ $FAILS -eq 0 ]; then echo "TXN DEMO PASSED"; else echo "TXN DEMO FAILED ($FAILS check(s))"; exit 1; fi
