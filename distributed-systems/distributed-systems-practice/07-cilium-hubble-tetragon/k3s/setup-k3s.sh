#!/bin/bash
# setup-k3s.sh — stands up a real, single-node Kubernetes v1.31 control
# plane via k3s, with CNI/kube-proxy intentionally disabled so Cilium
# (not flannel) becomes the CNI. This is the exact sequence that was
# run and verified in this project's sandbox — see ../ANALYSIS.md for
# what happened after this step.
#
# Every URL/version here is the exact one actually downloaded and run;
# nothing is a guess. k3s's airgap images tarball bundles containerd,
# CoreDNS, local-path-provisioner, etc. as a single file — no container
# registry is contacted for any of k3s's own default components, only
# for whatever CNI you install on top (Cilium, in this project).
set -euo pipefail

K3S_VERSION="v1.31.4+k3s1"
K3S_VERSION_URLENC="v1.31.4%2Bk3s1"

echo "== downloading k3s ${K3S_VERSION} binary =="
curl -sSL -o k3s "https://github.com/k3s-io/k3s/releases/download/${K3S_VERSION_URLENC}/k3s"
chmod +x k3s
./k3s --version

echo "== downloading k3s airgap images bundle (containerd/CoreDNS/etc — no registry needed) =="
curl -sSL -o k3s-airgap-images-amd64.tar.zst \
  "https://github.com/k3s-io/k3s/releases/download/${K3S_VERSION_URLENC}/k3s-airgap-images-amd64.tar.zst"

echo "== staging airgap images where k3s's embedded containerd auto-imports them on startup =="
sudo mkdir -p /var/lib/rancher/k3s/agent/images/
sudo cp k3s-airgap-images-amd64.tar.zst /var/lib/rancher/k3s/agent/images/

echo "== starting k3s server (CNI/kube-proxy disabled — Cilium will provide both) =="
sudo mkdir -p /etc/rancher/k3s
sudo ./k3s server \
  --flannel-backend=none \
  --disable-network-policy \
  --disable-kube-proxy \
  --disable=traefik,servicelb,local-storage,metrics-server \
  --write-kubeconfig-mode=644 &

echo "== waiting for the apiserver, then checking node status (expect NotReady — no CNI yet) =="
export KUBECONFIG=/etc/rancher/k3s/k3s.yaml
until ./k3s kubectl get nodes >/dev/null 2>&1; do sleep 2; done
./k3s kubectl get nodes -o wide

echo
echo "Node is up and NotReady, which is CORRECT at this point — a Kubernetes"
echo "node only goes Ready once a CNI has configured pod networking. Next:"
echo "  cd ../cilium && ./install-cilium.sh"
