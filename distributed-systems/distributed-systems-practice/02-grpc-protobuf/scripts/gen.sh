#!/bin/bash
# Regenerates gen/telemetry_pb2.py from proto/telemetry.proto using the
# real protoc compiler. Run this after editing the .proto file.
set -euo pipefail
cd "$(dirname "$0")/.."

if ! command -v protoc >/dev/null 2>&1; then
    echo "protoc not found. Install it, e.g.:" >&2
    echo "  Debian/Ubuntu : apt-get install -y protobuf-compiler" >&2
    echo "  macOS         : brew install protobuf" >&2
    echo "  Any platform  : download a prebuilt binary from" >&2
    echo "    https://github.com/protocolbuffers/protobuf/releases" >&2
    echo "    (that's exactly what this project's own dev environment did —" >&2
    echo "    apt and the Python grpcio-tools wheel were both unavailable" >&2
    echo "    there; see ANALYSIS.md)." >&2
    exit 1
fi

mkdir -p gen
protoc --proto_path=proto --python_out=gen proto/telemetry.proto
echo "generated gen/telemetry_pb2.py from proto/telemetry.proto ($(protoc --version))"
