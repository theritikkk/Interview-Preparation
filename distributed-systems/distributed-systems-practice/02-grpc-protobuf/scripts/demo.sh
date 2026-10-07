#!/bin/bash
# Starts the server, runs the client through all four RPC shapes, checks
# the client actually exited 0, then stops the server. Prints DEMO PASSED
# or DEMO FAILED and exits accordingly.
set -uo pipefail
cd "$(dirname "$0")/.."

fail() {
    echo "DEMO FAILED: $*" >&2
    [ -n "${SERVER_PID:-}" ] && kill "$SERVER_PID" 2>/dev/null
    exit 1
}

if [ ! -f gen/telemetry_pb2.py ]; then
    bash scripts/gen.sh || fail "could not generate protobuf stubs"
fi

echo "=== starting server ==="
python3 server.py > /tmp/telemetry_server.log 2>&1 &
SERVER_PID=$!
sleep 0.5
if ! kill -0 "$SERVER_PID" 2>/dev/null; then
    cat /tmp/telemetry_server.log >&2
    fail "server exited immediately, see output above"
fi
echo "server running (pid $SERVER_PID)"

echo "=== running client (all four RPC shapes) ==="
python3 client.py
CLIENT_EXIT=$?

echo
echo "=== server log ==="
cat /tmp/telemetry_server.log

kill "$SERVER_PID" 2>/dev/null
wait "$SERVER_PID" 2>/dev/null

if [ "$CLIENT_EXIT" -ne 0 ]; then
    fail "client exited with status $CLIENT_EXIT"
fi

echo
echo "=== DEMO PASSED ==="
