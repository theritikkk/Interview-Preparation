#!/usr/bin/env bash
# setup_otel.sh — builds a minimal, real OpenTelemetry C++ SDK from source
# and drops it into third_party/otel-cpp/{include,lib} as flat, Makefile-
# friendly directories. This repo ships a pre-built third_party/otel-cpp
# already (so you can build Papy immediately without running this), but
# this script is the actual, reproducible recipe that produced it — run
# it if you want to rebuild from scratch, update the OTel version, or
# verify the provenance yourself.
#
# What this builds: the trace API/SDK core and the OStreamSpanExporter
# only. OTLP (grpc/http), Zipkin, Prometheus, and the YAML-based
# declarative configuration loader are all disabled — they need grpc,
# curl, or a libstdc++ feature this toolchain doesn't have (see
# ANALYSIS.md, "What's real vs. simplified" and "A build that doesn't
# work"). None of that is needed for genuine distributed tracing with
# W3C trace-context propagation; it only affects where spans are
# exported TO.
#
# Usage: ./scripts/setup_otel.sh
# Requires: git, cmake, a C++17-capable compiler, network access to
# github.com (clones opentelemetry-cpp and two small header-only
# submodules: ms-gsl and nlohmann-json).

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="$ROOT_DIR/otel-cpp-src"
OUT_DIR="$ROOT_DIR/third_party/otel-cpp"
OTEL_COMMIT="main"   # pin a tag/commit here if you need reproducibility

echo "==> Cloning opentelemetry-cpp into $SRC_DIR"
if [ -d "$SRC_DIR" ]; then
    echo "    $SRC_DIR already exists, skipping clone (delete it to re-clone)"
else
    git clone --depth 1 --branch "$OTEL_COMMIT" \
        https://github.com/open-telemetry/opentelemetry-cpp.git "$SRC_DIR" \
        || git clone --depth 1 https://github.com/open-telemetry/opentelemetry-cpp.git "$SRC_DIR"
fi

echo "==> Fetching required submodules (ms-gsl, nlohmann-json — both header-only)"
cd "$SRC_DIR"
git submodule update --init --depth 1 -- third_party/ms-gsl third_party/nlohmann-json

echo "==> Configuring CMake (SDK + OStream exporter only; everything else OFF)"
cmake -S "$SRC_DIR" -B "$SRC_DIR/build" \
    -DCMAKE_BUILD_TYPE=Release \
    -DOTELCPP_BUILD_TESTING=OFF \
    -DOTELCPP_WITH_EXAMPLES=OFF \
    -DOTELCPP_WITH_BENCHMARK=OFF \
    -DOTELCPP_WITH_ABI_VERSION_1=ON \
    -DOTELCPP_WITH_ABI_VERSION_2=OFF \
    -DOTELCPP_WITH_STL=CXX23 \
    -DOTELCPP_WITH_OTLP_GRPC=OFF \
    -DOTELCPP_WITH_OTLP_HTTP=OFF \
    -DOTELCPP_WITH_OTLP_FILE=OFF \
    -DOTELCPP_WITH_ZIPKIN=OFF \
    -DOTELCPP_WITH_PROMETHEUS=OFF \
    -DOTELCPP_WITH_ELASTICSEARCH=OFF \
    -DOTELCPP_WITH_OPENTRACING=OFF

echo "==> Building the 6 targets this project actually needs"
# opentelemetry_configuration_core (declarative YAML config) is pulled in
# by the ostream builder helper but fails to compile on GCC13+libstdc++
# with -std=c++23 (nostd::string_view isn't accepted by
# unordered_map::find() without heterogeneous lookup support — a real
# upstream/toolchain incompatibility, not something this project uses).
# Building these 6 targets directly skips it entirely.
TARGETS=(
    opentelemetry_trace
    opentelemetry_exporter_ostream_span
    opentelemetry_common
    opentelemetry_resources
    opentelemetry_instrumentation_scope
    opentelemetry_version
)
cmake --build "$SRC_DIR/build" --target "${TARGETS[@]}" -j"$(nproc)"

echo "==> Flattening headers + libs into $OUT_DIR"
mkdir -p "$OUT_DIR/include" "$OUT_DIR/lib"
cp -r "$SRC_DIR/api/include/." "$OUT_DIR/include/"
cp -r "$SRC_DIR/sdk/include/." "$OUT_DIR/include/"
cp -r "$SRC_DIR/exporters/ostream/include/." "$OUT_DIR/include/"

cp "$SRC_DIR/build/sdk/src/trace/libopentelemetry_trace.a" "$OUT_DIR/lib/"
cp "$SRC_DIR/build/exporters/ostream/libopentelemetry_exporter_ostream_span.a" "$OUT_DIR/lib/"
cp "$SRC_DIR/build/sdk/src/resource/libopentelemetry_resources.a" "$OUT_DIR/lib/"
cp "$SRC_DIR/build/sdk/src/instrumentationscope/libopentelemetry_instrumentation_scope.a" "$OUT_DIR/lib/"
cp "$SRC_DIR/build/sdk/src/common/libopentelemetry_common.a" "$OUT_DIR/lib/"
cp "$SRC_DIR/build/sdk/src/version/libopentelemetry_version.a" "$OUT_DIR/lib/"

echo "==> Done. $OUT_DIR now has:"
du -sh "$OUT_DIR"/include "$OUT_DIR"/lib
echo "You can now 'make' Papy (see papy-src/Papy-main) and the demo server (see server/)."
