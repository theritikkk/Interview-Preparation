#!/bin/bash
# query-examples.sh — the exact real gRPC calls used to pull real profile
# data back out of a running Parca server (../server/) via grpcurl
# (../verify/build-grpcurl.sh). Run this with the server and workload
# both up. Output matches ../evidence/09 through ../evidence/12.
set -euo pipefail

GRPCURL=${GRPCURL:-./grpcurl}
ADDR=${ADDR:-127.0.0.1:7070}

echo "== real scrape target health =="
"$GRPCURL" -plaintext "$ADDR" parca.scrape.v1alpha1.ScrapeService/Targets \
  | python3 -c "
import json,sys
d = json.load(sys.stdin)
for job, info in d['targets'].items():
    print('job:', job)
    for t in info['targets']:
        labels = {l['name']: l['value'] for l in t['discoveredLabels']['labels']}
        print(f\"  profile={labels.get('__name__'):15s} url={t['url']:55s} health={t['health']}\")
"

NOW=$(date -u +%Y-%m-%dT%H:%M:%SZ)
START=$(date -u -d '@'$(( $(date +%s) - 300 )) +%Y-%m-%dT%H:%M:%SZ)

echo
echo "== real CPU top query (job=profworkload) — expect crypto/sha256.blockSHANI on top =="
cat > /tmp/q_cpu.json << EOF
{
  "mode": "MODE_MERGE",
  "merge": {
    "query": "process_cpu:samples:count:cpu:nanoseconds:delta{job=\"profworkload\"}",
    "start": "$START",
    "end": "$NOW"
  },
  "report_type": "REPORT_TYPE_TOP"
}
EOF
"$GRPCURL" -plaintext -d @ "$ADDR" parca.query.v1alpha1.QueryService/Query < /tmp/q_cpu.json \
  | python3 -c "
import json,sys
d = json.load(sys.stdin)
for item in d.get('top',{}).get('list',[])[:10]:
    print(f\"{item.get('flat','0'):>15}  {item['meta']['function']['name']}\")
"

echo
echo "== real memory alloc_space top query (job=profworkload) — expect main.hashBurn / main.allocBurn =="
cat > /tmp/q_mem.json << EOF
{
  "mode": "MODE_MERGE",
  "merge": {
    "query": "memory:alloc_space:bytes:space:bytes{job=\"profworkload\"}",
    "start": "$START",
    "end": "$NOW"
  },
  "report_type": "REPORT_TYPE_TOP"
}
EOF
"$GRPCURL" -plaintext -d @ "$ADDR" parca.query.v1alpha1.QueryService/Query < /tmp/q_mem.json \
  | python3 -c "
import json,sys
d = json.load(sys.stdin)
for item in d.get('top',{}).get('list',[])[:8]:
    print(f\"{item.get('flat','0'):>15}  {item['meta']['function']['name']}\")
"
