#!/bin/bash
# rebalance_demo.sh — add shard 4 to a QUIET 3-shard cluster and move exactly
# the keys the ring says must move. (Rebalancing under live traffic is the
# separate, harder test: scripts/rebalance_stress.sh.)
#
#   usage: scripts/rebalance_demo.sh [numKeys]
set -uo pipefail
cd "$(dirname "$0")/.."
N=${1:-1500}
trap './scripts/stop_shards.sh; rm -f ring.migrating.conf ring.stable4.conf' EXIT

# Generated ring configs sit beside ring.conf so shard config paths resolve.
cat > ring.migrating.conf <<CONF
vnodes 128
state migrating
shards 1 2 3 4
old_shards 1 2 3
CONF
cat > ring.stable4.conf <<CONF
vnodes 128
state stable
shards 1 2 3 4
CONF

./scripts/stop_shards.sh
./scripts/run_shards.sh 1 2 3 || exit 1
./build/shardtool load ring.conf "$N" 48 || exit 1
echo; echo "== 1. before: 3 shards, every key on its ring owner =="
./build/shardtool census ring.conf "$N" || exit 1

echo; echo "== 2. start shard 4 (empty). Judged against the 4-shard ring, the keys that must move show up as 'misplaced' on 1-3 and 'missing' on 4 =="
./scripts/run_shards.sh 4 || exit 1
./build/shardtool census ring.stable4.conf "$N" || true   # expected to FAIL: nothing has moved yet

echo; echo "== 3. rebalance (safe protocol) =="
./build/shardtool rebalance ring.migrating.conf --expect "$N" || { echo "REBALANCE DEMO FAILED"; exit 1; }

echo; echo "== 4. after: 4 shards, every key on its ring owner, every value intact =="
fail=0
./build/shardtool census ring.stable4.conf "$N" || fail=1
./build/shardtool verify ring.stable4.conf "$N" 48 || fail=1
echo
if [ $fail -eq 0 ]; then echo "REBALANCE DEMO PASSED"; else echo "REBALANCE DEMO FAILED"; exit 1; fi
