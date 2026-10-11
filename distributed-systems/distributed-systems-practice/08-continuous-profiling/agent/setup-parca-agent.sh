#!/bin/bash
# setup-parca-agent.sh — download the real parca-agent (eBPF-based
# whole-system profiler) binary from its GitHub release, and run it
# exactly as documented.
#
# On most hosts (a real cloud VM, a dev laptop, a CI runner with
# CAP_SYS_RESOURCE available to root) this is the complete, correct
# command and parca-agent will start sampling every process on the
# machine via eBPF, no source-code changes needed anywhere.
#
# In THIS specific sandbox it fails at eBPF-tracer load time with a
# real, reproducible error — not a crash, not a missing binary, a
# precise permission failure. See ../ANALYSIS.md for the full
# diagnosis (traced to one specific missing Linux capability,
# CAP_SYS_RESOURCE, and the exact library call that needs it) and
# ../evidence/ for the captured proof.
set -euo pipefail

AGENT_VERSION="0.50.0"

echo "== downloading the real parca-agent binary =="
curl -sSL -o parca-agent \
  "https://github.com/parca-dev/parca-agent/releases/download/v${AGENT_VERSION}/parca-agent_${AGENT_VERSION}_Linux_x86_64"
chmod +x parca-agent
./parca-agent --version

echo "== attempting to start the real eBPF tracer, pointed at the real Parca server =="
./parca-agent \
  --remote-store-address=127.0.0.1:7070 \
  --remote-store-insecure
# In this sandbox: fails with
#   "Failed to load eBPF tracer: ... failed to adjust rlimit: failed to
#    set temporary rlimit: operation not permitted"
# See ../ANALYSIS.md.
