#!/bin/bash
# gen-certs.sh — generates a real local CA plus a server-sidecar and
# client-sidecar identity cert, signed by that CA. This is the same
# trust-bootstrapping step a service mesh's own CA (Istio's istiod, or
# SPIFFE/SPIRE) does automatically at runtime; here it's a one-time local
# script since there's no running control plane to do it for us (see
# ../../ANALYSIS.md). Private keys aren't shipped in the deliverable —
# run this first to generate everything in certs/.
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p certs
cd certs

echo "== generating mesh CA =="
openssl genrsa -out ca-key.pem 2048 2>/dev/null
openssl req -x509 -new -nodes -key ca-key.pem -sha256 -days 365 -out ca-cert.pem \
  -subj "/O=mesh-demo/CN=mesh-demo-ca" 2>/dev/null

echo "== generating server-sidecar identity, signed by the mesh CA =="
openssl genrsa -out server-key.pem 2048 2>/dev/null
openssl req -new -key server-key.pem -out server.csr -subj "/O=mesh-demo/CN=server-sidecar" 2>/dev/null
openssl x509 -req -in server.csr -CA ca-cert.pem -CAkey ca-key.pem -CAcreateserial \
  -out server-cert.pem -days 365 -sha256 2>/dev/null

echo "== generating client-sidecar identity, signed by the mesh CA =="
openssl genrsa -out client-key.pem 2048 2>/dev/null
openssl req -new -key client-key.pem -out client.csr -subj "/O=mesh-demo/CN=client-sidecar" 2>/dev/null
openssl x509 -req -in client.csr -CA ca-cert.pem -CAkey ca-key.pem -CAcreateserial \
  -out client-cert.pem -days 365 -sha256 2>/dev/null

rm -f server.csr client.csr
echo "== verifying the chain =="
openssl verify -CAfile ca-cert.pem server-cert.pem client-cert.pem
echo "done — certs/ now has ca-cert.pem, server-{cert,key}.pem, client-{cert,key}.pem"
