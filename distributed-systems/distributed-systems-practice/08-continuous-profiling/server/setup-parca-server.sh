#!/bin/bash
# setup-parca-server.sh — download the real Parca server binary from its
# GitHub release (same "registries blocked, GitHub release assets allowed"
# pattern discovered in Project 7 for k3s/cilium-cli), then run it against
# the scrape_configs in parca.yaml (in this same directory).
#
# Parca itself needs no eBPF, no elevated capabilities, and no container
# registry — this part of the stack runs cleanly in any sandbox that can
# reach github.com.
set -euo pipefail

PARCA_VERSION="0.29.1"

echo "== downloading the real parca server binary =="
curl -sSL -o parca.tar.gz \
  "https://github.com/parca-dev/parca/releases/download/v${PARCA_VERSION}/parca_${PARCA_VERSION}_Linux_x86_64.tar.gz"
tar xzf parca.tar.gz parca
./parca --version

echo "== starting parca, scraping itself + workload/profworkload (see parca.yaml) =="
mkdir -p parca-data
./parca --config-path=./parca.yaml --storage-path=./parca-data --http-address=:7070
# UI: http://127.0.0.1:7070/
