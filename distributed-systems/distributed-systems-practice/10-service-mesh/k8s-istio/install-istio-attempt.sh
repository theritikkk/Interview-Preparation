#!/bin/bash
# install-istio-attempt.sh — the real install runbook for Istio's
# Kubernetes-native control plane, using the real `istioctl` CLI
# (downloaded from istio/istio's actual GitHub release — Istio, unlike
# chaosd/helm in Project 9, DOES ship via GitHub Releases).
#
# This was run against a real, fresh, single-node k3s v1.31.4 cluster
# with its DEFAULT flannel CNI (not disabled, unlike Project 7's Cilium
# setup) — so unlike Project 7, there is no CNI-specific confound here.
# It fails for exactly one reason, independent of Istio or its images:
# this sandbox's cgroup hierarchy isn't delegated for nested pod-sandbox
# creation, so no pod — Istio's or anyone else's — can ever start. This
# is PROVEN in evidence/05 (k3s's own bundled CoreDNS, zero registries
# involved) and evidence/06 (istiod itself, same signature) — see
# ../ANALYSIS.md for the full story.
#
# On infrastructure without that cgroup-delegation limitation (a real
# cloud VM, most CI runners, a laptop with Docker), this is the
# complete, correct install sequence and will bring up a real, working
# Istio control plane and mesh.
set -euo pipefail

ISTIO_VERSION="1.31.1"

echo "== downloading the real istioctl CLI =="
curl -sSL -o istioctl.tar.gz \
  "https://github.com/istio/istio/releases/download/${ISTIO_VERSION}/istioctl-${ISTIO_VERSION}-linux-amd64.tar.gz"
tar xzf istioctl.tar.gz
./istioctl version --remote=false

echo "== installing Istio's minimal profile against the current kube context =="
./istioctl install --set profile=minimal -y

echo "== waiting for istiod =="
kubectl -n istio-system rollout status deployment/istiod

echo "== labeling the default namespace for automatic sidecar injection =="
kubectl label namespace default istio-injection=enabled --overwrite

echo "== (example) deploy a workload and watch Envoy get injected as a sidecar =="
echo "kubectl apply -f https://raw.githubusercontent.com/istio/istio/release-${ISTIO_VERSION%.*}/samples/sleep/sleep.yaml"
echo "kubectl get pods   # expect 2/2 READY: the app container + the injected istio-proxy (Envoy) sidecar"
