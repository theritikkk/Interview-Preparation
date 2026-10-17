#!/bin/bash
# run-mesh-demo.sh — stands up the whole data-plane demo (5 backend
# services, 3 real Envoy instances) and runs all four feature
# demonstrations end to end, printing live evidence for each. This
# reproduces exactly what's captured in evidence/01 through evidence/04.
#
# Requires: a Linux x86_64 host, python3, openssl, curl. Downloads the
# real Envoy v1.39.4 binary from its GitHub release if not already
# present (see ENVOY_URL below — the asset naming was confirmed by
# probing the release, not guessed; see ANALYSIS.md).
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

ENVOY_URL="https://github.com/envoyproxy/envoy/releases/download/v1.39.4/envoy-1.39.4-linux-x86_64"
PIDS=()

cleanup() {
  echo
  echo "== shutting down demo processes =="
  for pid in "${PIDS[@]:-}"; do
    kill "$pid" 2>/dev/null || true
  done
}
trap cleanup EXIT

echo "###############################################"
echo "# Setup"
echo "###############################################"
if [ ! -x ./envoy ]; then
  echo "== downloading real Envoy v1.39.4 =="
  curl -sSL -o envoy "$ENVOY_URL"
  chmod +x envoy
fi
./envoy --version

if [ ! -f envoy-config/mtls/certs/ca-cert.pem ]; then
  ./envoy-config/mtls/gen-certs.sh
fi

echo
echo "== starting backend services =="
SERVICE_NAME=service-v1     PORT=9001 FLAKY_PCT=0  DELAY_MS=0   python3 backends/app.py >/tmp/mesh-v1.log 2>&1 &     PIDS+=($!)
SERVICE_NAME=service-v2     PORT=9002 FLAKY_PCT=0  DELAY_MS=0   python3 backends/app.py >/tmp/mesh-v2.log 2>&1 &     PIDS+=($!)
SERVICE_NAME=flaky-service  PORT=9003 FLAKY_PCT=40 DELAY_MS=0   python3 backends/app.py >/tmp/mesh-flaky.log 2>&1 & PIDS+=($!)
SERVICE_NAME=cb-service     PORT=9004 FLAKY_PCT=0  DELAY_MS=500 python3 backends/app.py >/tmp/mesh-cb.log 2>&1 &    PIDS+=($!)
SERVICE_NAME=secure-backend PORT=9005 FLAKY_PCT=0  DELAY_MS=0   python3 backends/app.py >/tmp/mesh-secure.log 2>&1 & PIDS+=($!)
sleep 1

echo "== starting Envoy front-proxy (split/retry/circuit routes) =="
"$SCRIPT_DIR/envoy" -c envoy-config/front-proxy.yaml --log-level warn >/tmp/mesh-front.log 2>&1 &
PIDS+=($!)

echo "== starting Envoy mTLS sidecar pair =="
# `exec` inside the subshell replaces the subshell process with envoy
# itself, so the PID captured by $! right after is envoy's real PID —
# without it, $! would name the short-lived "cd && envoy" subshell
# instead, and `kill` in cleanup() would miss the actual envoy process.
( cd envoy-config/mtls && exec "$SCRIPT_DIR/envoy" -c server-sidecar.yaml --log-level warn --disable-hot-restart >/tmp/mesh-server-sidecar.log 2>&1 ) &
PIDS+=($!)
( cd envoy-config/mtls && exec "$SCRIPT_DIR/envoy" -c client-sidecar.yaml --log-level warn --disable-hot-restart >/tmp/mesh-client-sidecar.log 2>&1 ) &
PIDS+=($!)
sleep 2

echo
echo "###############################################"
echo "# 1. Traffic split (80/20 weighted_clusters)"
echo "###############################################"
v1=0; v2=0
for i in $(seq 1 100); do
  r=$(curl -sS http://127.0.0.1:10000/split)
  [ "$r" = "service-v1" ] && v1=$((v1+1))
  [ "$r" = "service-v2" ] && v2=$((v2+1))
done
echo "100 requests: v1=$v1 v2=$v2 (configured: 80/20)"

echo
echo "###############################################"
echo "# 2. Retry policy (backend fails ~40% of the time)"
echo "###############################################"
echo "-- direct to the flaky backend, no retry, 50 requests --"
s=0; f=0
for i in $(seq 1 50); do
  c=$(curl -sS -o /dev/null -w "%{http_code}" http://127.0.0.1:9003/)
  [ "$c" = "200" ] && s=$((s+1)) || f=$((f+1))
done
echo "direct: success=$s fail=$f"
echo "-- through Envoy's retry_policy (retry_on 5xx, num_retries 3), 50 requests --"
s=0; f=0
for i in $(seq 1 50); do
  c=$(curl -sS -o /dev/null -w "%{http_code}" http://127.0.0.1:10000/retry)
  [ "$c" = "200" ] && s=$((s+1)) || f=$((f+1))
done
echo "via envoy: success=$s fail=$f"

echo
echo "###############################################"
echo "# 3. Circuit breaker (max_connections=2, max_pending_requests=2)"
echo "###############################################"
echo "-- 10 concurrent requests against a backend with a 500ms artificial delay --"
CB_PIDS=()
for i in $(seq 1 10); do
  curl -sS -o /dev/null -w "%{http_code}\n" http://127.0.0.1:10000/circuit &
  CB_PIDS+=($!)
done
for pid in "${CB_PIDS[@]}"; do wait "$pid"; done
echo "(expect ~4x 200, ~6x 503 overflow — see evidence/03 for Envoy's own counters)"

echo
echo "###############################################"
echo "# 4. mTLS (real CA, real certs, genuinely enforced)"
echo "###############################################"
echo "-- through the real mTLS path (client-sidecar -> server-sidecar) --"
curl -sS -w " [HTTP %{http_code}]\n" http://127.0.0.1:20000/
echo "-- plaintext straight at the mTLS-only port (bypassing client-sidecar) --"
curl -sS -m 3 -w " [HTTP %{http_code}]\n" http://127.0.0.1:20001/ 2>&1 || true
echo "-- real TLS handshake, no client cert presented --"
timeout 5 openssl s_client -connect 127.0.0.1:20001 -CAfile envoy-config/mtls/certs/ca-cert.pem < /dev/null 2>&1 | grep -E "alert|CONNECTED|error" || true

echo
echo "Demo complete. See evidence/ for the original captured runs (larger samples)."
