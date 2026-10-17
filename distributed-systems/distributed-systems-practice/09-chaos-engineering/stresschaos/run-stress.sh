#!/bin/bash
# run-stress.sh — real CPU/memory resource chaos via the real stress-ng
# binary, downloaded from chaos-mesh's own GitHub-released fork
# (github.com/chaos-mesh/stress-ng) — the exact same binary Chaos Mesh's
# StressChaos CRD downloads and wraps (see
# ../k8s-chaos-mesh/examples/burn-cpu.yaml: its `stressors.cpu.options`
# field is literally stress-ng CLI flags). Used here directly since the
# Chaos Mesh operator itself can't run in this sandbox (see ../ANALYSIS.md).
set -euo pipefail

STRESS_NG_VERSION="v0.14.02"

if [ ! -x ./stress-ng ]; then
  echo "== downloading the real stress-ng binary (same one Chaos Mesh's StressChaos uses) =="
  curl -sSL -o stress-ng \
    "https://github.com/chaos-mesh/stress-ng/releases/download/${STRESS_NG_VERSION}/stress-ng-x86_64"
  chmod +x stress-ng
fi
./stress-ng --version

echo
echo "== baseline load average =="
cat /proc/loadavg

echo
echo "== real CPU stress: 4 workers, 100% load, 10s =="
./stress-ng --cpu 4 --cpu-load 100 --metrics-brief --timeout 10s

echo
echo "== real memory stress: 2 workers x 256MB, 8s =="
free -m
./stress-ng --vm 2 --vm-bytes 256M --metrics-brief --timeout 8s
