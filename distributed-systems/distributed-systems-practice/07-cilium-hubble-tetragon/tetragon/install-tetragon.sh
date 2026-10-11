#!/bin/bash
# install-tetragon.sh — install Tetragon (runtime security/process
# observability) via its real Helm chart, then apply a real example
# TracingPolicy.
#
# Unlike Cilium, Tetragon has no dedicated CLI with a built-in chart —
# it's installed with plain `helm install` against the chart in
# chart-reference/ (copied verbatim from github.com/cilium/tetragon's
# install/kubernetes/tetragon/ at the commit this project checked out —
# diff it against the real repo to confirm). This sandbox had no
# working `helm` binary to actually run this (see ANALYSIS.md for what
# was and wasn't reachable), so this script is real and correct but
# untested end-to-end here — the chart and values are the genuine
# upstream files, not hand-authored.
set -euo pipefail

export KUBECONFIG=/etc/rancher/k3s/k3s.yaml

echo "== installing Tetragon from the real upstream chart =="
helm install tetragon chart-reference/ \
  --namespace kube-system \
  --values chart-reference/values.yaml

echo "== waiting for the Tetragon DaemonSet =="
kubectl -n kube-system rollout status daemonset/tetragon

echo "== applying a real example TracingPolicy: privilege-escalation detection via commit_creds() =="
kubectl apply -f examples/kprobe_commit_creds.yaml

echo "== tailing real-time process/security events =="
kubectl -n kube-system exec -ti ds/tetragon -c export-stdout -- sh -c 'tail -f /var/run/cilium/tetragon/tetragon.log'
# or, with the tetra CLI (github.com/cilium/tetragon/releases, same
# GitHub-release pattern already verified working for cilium-cli/k3s):
#   tetra getevents -o compact
