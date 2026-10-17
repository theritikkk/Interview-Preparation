#!/bin/bash
# isolation_test.sh — does losing one shard's leader disturb ONLY that shard?
#
#   1. start shards 1 2 3, load N keys through the router, census (each key on
#      exactly its ring owner)
#   2. start a per-shard availability probe (one GET every 100ms per shard)
#   3. 3s in, kill -9 the LEADER of shard 2
#   4. check: shards 1 and 3 saw zero failures; shard 2 failed for a while and
#      then recovered on its own (re-election); no key was lost
#
#   usage: scripts/isolation_test.sh [numKeys]
set -uo pipefail
cd "$(dirname "$0")/.."
N=${1:-1500}
VICTIM=2
trap './scripts/stop_shards.sh' EXIT

./scripts/stop_shards.sh
./scripts/run_shards.sh 1 2 3 || { echo "ISOLATION TEST FAILED: cluster did not start"; exit 1; }
./build/shardtool load   ring.conf "$N" 48 || { echo "ISOLATION TEST FAILED: load"; exit 1; }
./build/shardtool census ring.conf "$N"    || { echo "ISOLATION TEST FAILED: census before fault"; exit 1; }

LEADER=$(grep -h "became LEADER" logs/shard$VICTIM-node*.log | sort | tail -1 | grep -oP 'id=\K[0-9]+')
PID=$(cat logs/shard$VICTIM-node$LEADER.pid)
echo
echo "== probing all shards for 14s; at t=3s kill -9 shard $VICTIM's leader (node $LEADER, pid $PID) =="
./build/shardtool probe ring.conf 14 100 > logs/probe.out &
PROBE=$!
sleep 3
kill -9 "$PID"
wait "$PROBE" || true   # probe exits 1 when anything failed; we interpret the table ourselves
cat logs/probe.out

fail=0
row() { awk -v s="$1" '$1==s {print}' logs/probe.out; }
for S in 1 3; do
    f=$(row $S | awk '{print $3}')
    if [ "$f" != "0" ]; then echo "FAIL: shard $S saw $f failed requests (should be 0)"; fail=1; fi
done
f2=$(row $VICTIM | awk '{print $3}')
if [ "${f2:-0}" -eq 0 ]; then echo "FAIL: shard $VICTIM saw no failures (the kill had no visible effect?)"; fail=1; fi
if row $VICTIM | grep -q "STILL DOWN"; then echo "FAIL: shard $VICTIM never recovered"; fail=1; fi

echo
echo "== after recovery: is all the data still there? =="
./build/shardtool verify ring.conf "$N" 48 || fail=1
./build/shardtool census ring.conf "$N" || fail=1

echo
if [ $fail -eq 0 ]; then echo "ISOLATION TEST PASSED"; else echo "ISOLATION TEST FAILED"; exit 1; fi
