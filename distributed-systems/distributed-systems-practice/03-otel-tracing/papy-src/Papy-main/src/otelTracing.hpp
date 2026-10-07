#pragma once
// otelTracing.hpp — wires Papy up to the real OpenTelemetry C++ SDK.
//
// This is genuinely the opentelemetry-cpp SDK (TracerProvider, Tracer,
// Span, SpanContext, the real W3C HttpTraceContext propagator) built from
// source — see ANALYSIS.md for exactly how, and why the exporter is
// "ostream" rather than a real OTLP collector (the sandbox this was
// developed in can't install grpc/curl, which OTLP export needs; a real
// deployment would swap in OtlpHttpExporter or OtlpGrpcExporter here and
// nothing else in this file or in the instrumented call sites would
// change — that's the whole point of coding against the OTel API).

#include <memory>
#include <string>

#include "opentelemetry/trace/tracer.h"
#include "opentelemetry/trace/provider.h"
#include "opentelemetry/nostd/shared_ptr.h"

namespace otelTracing {

// Call once at process startup (both the papy binary and the demo target
// server call this, each with their own service name, so spans from
// either process are labeled with which service emitted them).
void init(const std::string& serviceName, const std::string& logFilePath = "");

// Call once at process shutdown so buffered spans are flushed before the
// process exits (SimpleSpanProcessor exports synchronously here, but
// calling Shutdown() explicitly is still the correct, documented pattern
// and matters the moment this is swapped for a batching processor).
void shutdown();

opentelemetry::nostd::shared_ptr<opentelemetry::trace::Tracer> getTracer();

}  // namespace otelTracing
