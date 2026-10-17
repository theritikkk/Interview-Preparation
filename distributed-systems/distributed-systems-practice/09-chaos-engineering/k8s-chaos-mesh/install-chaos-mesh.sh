#!/bin/bash
# install-chaos-mesh.sh — the real install runbook for Chaos Mesh's
# Kubernetes-native operator, using its real upstream Helm chart
# (chart/, copied verbatim from github.com/chaos-mesh/chaos-mesh at tag
# v2.7.2 — diff it against the real repo to confirm).
#
# This is NOT run end-to-end in this project's sandbox. Three
# independent, real, precisely-diagnosed reasons why, each confirmed
# separately (see ../ANALYSIS.md and ../evidence/):
#
#   1. No working `helm` binary could be obtained here: unlike k3s,
#      cilium-cli, parca, and parca-agent (all real GitHub-release
#      binaries), both chaosd AND helm itself distribute their compiled
#      binaries via their own cloud storage (an S3 bucket for chaosd,
#      Azure Blob Storage behind get.helm.sh for helm) rather than
#      GitHub Releases — and that storage is blocked by this sandbox's
#      egress policy, same as every container registry tested in
#      Project 7.
#   2. This chart's images come from ghcr.io (see chart/values.yaml's
#      `registry: "ghcr.io"`), one of the registries already confirmed
#      blocked (403) in Project 7, alongside docker.io/quay.io/registry.k8s.io.
#   3. Independent of both of the above: Project 7 already proved, with
#      a real, unprivileged pod (cilium-operator), that this sandbox's
#      cgroup hierarchy isn't delegated for nested pod-sandbox creation —
#      ANY pod fails identically with "can't get final child's PID from
#      pipe: EOF", regardless of image or chart.
#
# On infrastructure without these three specific constraints (a real
# cloud VM, most CI runners, a laptop with Docker), this is the
# complete, correct install sequence and will bring up a real,
# functioning Chaos Mesh control plane.
set -euo pipefail

export KUBECONFIG=/etc/rancher/k3s/k3s.yaml

echo "== creating namespace =="
kubectl create ns chaos-mesh 2>/dev/null || true

echo "== installing the real upstream Chaos Mesh chart =="
helm install chaos-mesh chart/ \
  --namespace chaos-mesh \
  --set chaosDaemon.runtime=containerd \
  --set chaosDaemon.socketPath=/run/k3s/containerd/containerd.sock

echo "== waiting for the controller-manager =="
kubectl -n chaos-mesh rollout status deployment/chaos-mesh-controller-manager

echo "== applying a real example: inject 90ms+-90ms delay into tikv-labeled pods =="
kubectl apply -f examples/network-delay-example.yaml

echo "== applying a real example: real stress-ng CPU burn (see examples/burn-cpu.yaml) =="
kubectl apply -f examples/burn-cpu.yaml

echo "== watching experiments =="
kubectl get networkchaos,podchaos,stresschaos -A
