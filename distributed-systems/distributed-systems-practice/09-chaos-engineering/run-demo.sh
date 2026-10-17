#!/bin/bash
# run-demo.sh — runs all three real chaos experiments end to end against
# a fresh target server, printing before/after evidence for each. This
# reproduces exactly what's captured in evidence/07, evidence/08,
# evidence/09, and evidence/10.
set -euo pipefail
cd "$(dirname "$0")"

echo "###############################################"
echo "# Building tools"
echo "###############################################"
(cd tcpchaos && GOTOOLCHAIN=local go build -o tcpchaos .)
(cd stresschaos && [ -x stress-ng ] || curl -sSL -o stresschaos/stress-ng \
  "https://github.com/chaos-mesh/stress-ng/releases/download/v0.14.02/stress-ng-x86_64" && chmod +x stresschaos/stress-ng)

echo
echo "###############################################"
echo "# Starting target server"
echo "###############################################"
(cd target-server && nohup python3 -m http.server 8765 --bind 127.0.0.1 > server.log 2>&1 &)
sleep 1
curl -sS http://127.0.0.1:8765/
TARGET_PID=$(pgrep -f "http.server 8765" | head -1)
echo "target server pid=$TARGET_PID"

echo
echo "###############################################"
echo "# 1. Network chaos (tcpchaos: real delay+jitter+loss TCP proxy)"
echo "###############################################"
echo "-- baseline (direct) --"
for i in $(seq 1 5); do curl -sS -o /dev/null -w "%{http_code} %{time_total}s\n" http://127.0.0.1:8765/; done

./tcpchaos/tcpchaos -listen 127.0.0.1:18765 -target 127.0.0.1:8765 -delay-ms 150 -jitter-ms 30 -loss-pct 10 &
TCPCHAOS_PID=$!
sleep 1
echo "-- through chaos proxy (150ms delay, 30ms jitter, 10% loss) --"
for i in $(seq 1 5); do curl -sS -m 3 -o /dev/null -w "%{http_code} %{time_total}s\n" http://127.0.0.1:18765/; done
kill "$TCPCHAOS_PID" 2>/dev/null || true

echo
echo "###############################################"
echo "# 2. Process chaos (procchaos: real SIGSTOP/SIGCONT, SIGKILL)"
echo "###############################################"
./procchaos/procchaos.sh pause "$TARGET_PID" 3

echo
echo "###############################################"
echo "# 3. Resource chaos (stress-ng: real CPU + memory load)"
echo "###############################################"
(cd stresschaos && ./stress-ng --cpu 2 --cpu-load 100 --metrics-brief --timeout 5s)

echo
echo "Demo complete. See evidence/ for the original captured runs."
