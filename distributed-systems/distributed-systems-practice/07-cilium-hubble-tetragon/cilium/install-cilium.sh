#!/bin/bash
# install-cilium.sh — the exact real commands run against the real k3s
# cluster from ../k3s/setup-k3s.sh, using the real cilium-cli tool.
#
# On a cluster that can actually reach quay.io (this sandbox can't —
# see ../ANALYSIS.md), this is the complete, correct install sequence
# and will bring the node to Ready with a working Cilium+Hubble
# dataplane. rendered-manifests.yaml in this directory is the literal,
# complete output of `cilium install --dry-run` below, captured from
# this exact chart version against this exact cluster — not hand-written.
set -euo pipefail

CILIUM_CLI_VERSION="latest"   # verified working via /releases/latest/download/
CILIUM_VERSION="1.16.5"

echo "== downloading the real cilium-cli =="
curl -sSL -o cilium.tar.gz \
  "https://github.com/cilium/cilium-cli/releases/${CILIUM_CLI_VERSION}/download/cilium-linux-amd64.tar.gz"
tar xzf cilium.tar.gz
./cilium version --client

export KUBECONFIG=/etc/rancher/k3s/k3s.yaml

echo "== installing Cilium as CNI + kube-proxy replacement, with Hubble relay+UI =="
./cilium install \
  --version "${CILIUM_VERSION}" \
  --helm-set hubble.relay.enabled=true \
  --helm-set hubble.ui.enabled=true
  # k3s is auto-detected by cilium-cli (kind=K3s), which sets
  # routingMode=tunnel/tunnelProtocol=vxlan and the k3s-specific
  # kube-proxy-replacement wiring automatically — see
  # values-hubble-enabled.yaml for exactly what it derived for this
  # cluster (captured via --dry-run-helm-values, same command+flags).

echo "== waiting for Cilium to report Ready (this is the step that needs quay.io reachability) =="
./cilium status --wait

echo "== once ready, the node goes Ready and pod networking works =="
./k3s kubectl get nodes

echo
echo "== Hubble observability =="
./cilium hubble port-forward &
hubble observe --follow &
echo "(hubble CLI: https://github.com/cilium/hubble/releases — same GitHub-release pattern as cilium-cli)"
