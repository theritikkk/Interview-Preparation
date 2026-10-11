#!/bin/bash
# setup-workload.sh — build and run profworkload, a small real Go service
# written for this project (main.go, same directory). It does real,
# continuous CPU work (SHA-256 hashing, in hashBurn) and real allocation
# work (map/string churn, in allocBurn), and serves them over the standard
# library's net/http/pprof on :9999 — exactly the kind of target Parca's
# scrape_configs are designed to pull from, with zero eBPF and zero
# special privileges required on either side.
set -euo pipefail

echo "== building profworkload =="
GOTOOLCHAIN=local go build -o profworkload .

echo "== starting it (pprof endpoints under http://127.0.0.1:9999/debug/pprof/) =="
./profworkload
