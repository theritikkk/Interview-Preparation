#!/usr/bin/env bash
# demo.sh — builds (if needed) and runs the full Papy <-> target_server
# tracing demo for one request, then prints both processes' spans and
# highlights the trace_id they share, so you can see the propagation
# working without reading raw log files yourself.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PAPY_DIR="$ROOT/papy-src/Papy-main"
SERVER_DIR="$ROOT/server"
PORT="${1:-8099}"

if [ ! -d "$ROOT/third_party/otel-cpp/lib" ]; then
    echo "third_party/otel-cpp is missing. Run scripts/setup_otel.sh first" >&2
    exit 1
fi

if [ ! -x "$PAPY_DIR/bin/papy" ]; then
    echo "==> Building Papy..."
    make -C "$PAPY_DIR"
fi

if [ ! -x "$SERVER_DIR/bin/target_server" ]; then
    echo "==> Building the demo target server..."
    make -C "$SERVER_DIR"
fi

SERVER_LOG=$(mktemp /tmp/otel-demo-server-XXXX.log)
CLIENT_LOG=$(mktemp /tmp/otel-demo-client-XXXX.log)
SERVER_STDOUT=$(mktemp /tmp/otel-demo-server-stdout-XXXX.log)

echo "==> Starting target_server on port $PORT (spans -> $SERVER_LOG)"
"$SERVER_DIR/bin/target_server" --port "$PORT" --log "$SERVER_LOG" > "$SERVER_STDOUT" 2>&1 &
SERVER_PID=$!

cleanup() {
    if kill -0 "$SERVER_PID" 2>/dev/null; then
        kill -INT "$SERVER_PID" 2>/dev/null || true
        wait "$SERVER_PID" 2>/dev/null || true
    fi
}
trap cleanup EXIT

# Give it a moment to bind before hitting it.
for _ in $(seq 1 20); do
    if timeout 1 bash -c "echo > /dev/tcp/127.0.0.1/$PORT" 2>/dev/null; then
        break
    fi
    sleep 0.1
done

echo "==> Running Papy against it (spans -> $CLIENT_LOG)"
PAPY_TRACE_LOG="$CLIENT_LOG" "$PAPY_DIR/bin/papy" \
    --target "http://127.0.0.1:$PORT" --endpoint "/printJson" --count 1 --verbose

sleep 0.3
cleanup
trap - EXIT

echo
echo "================= Papy (client) spans ================="
cat "$CLIENT_LOG"
echo
echo "================= target_server spans =================="
cat "$SERVER_LOG"
echo

CLIENT_TRACE_ID=$(grep "trace_id      :" "$CLIENT_LOG" | head -1 | awk '{print $3}')
SERVER_TRACE_ID=$(grep "trace_id      :" "$SERVER_LOG" | head -1 | awk '{print $3}')

echo "================= Verdict ==============================="
echo "Client trace_id: $CLIENT_TRACE_ID"
echo "Server trace_id: $SERVER_TRACE_ID"
if [ -n "$CLIENT_TRACE_ID" ] && [ "$CLIENT_TRACE_ID" = "$SERVER_TRACE_ID" ]; then
    echo "MATCH — one trace, two processes. This is what makes it distributed"
    echo "tracing and not just structured logging inside Papy."
else
    echo "NO MATCH — something is wrong; the trace was not propagated."
    exit 1
fi
