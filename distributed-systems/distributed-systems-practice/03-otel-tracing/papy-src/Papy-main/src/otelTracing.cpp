#include "otelTracing.hpp"

#include <fstream>
#include <iostream>

#include "opentelemetry/sdk/trace/tracer_provider_factory.h"
#include "opentelemetry/sdk/trace/simple_processor_factory.h"
#include "opentelemetry/sdk/trace/batch_span_processor_factory.h"
#include "opentelemetry/sdk/resource/resource.h"
#include "opentelemetry/exporters/ostream/span_exporter_factory.h"

namespace trace_sdk = opentelemetry::sdk::trace;
namespace trace_api = opentelemetry::trace;

namespace otelTracing {
namespace {
// Kept alive for the whole process: the exporter holds a reference to
// this stream for as long as the TracerProvider (and therefore the
// exporter) is alive.
std::ofstream g_logFile;
}  // namespace

void init(const std::string& serviceName, const std::string& logFilePath) {
    std::unique_ptr<trace_sdk::SpanExporter> exporter;
    if (!logFilePath.empty()) {
        g_logFile.open(logFilePath, std::ios::out | std::ios::trunc);
        exporter = opentelemetry::exporter::trace::OStreamSpanExporterFactory::Create(g_logFile);
    } else {
        exporter = opentelemetry::exporter::trace::OStreamSpanExporterFactory::Create();
    }

    // SimpleSpanProcessor exports each span synchronously the moment it
    // ends — no batching, no background export thread. That's the right
    // choice for a short-lived CLI tool like Papy: a BatchSpanProcessor
    // (the production default, which buffers spans and flushes them
    // periodically on a background thread for far less per-span export
    // overhead) risks losing the tail of spans if the process exits
    // before the next batch flush fires.
    auto processor = trace_sdk::SimpleSpanProcessorFactory::Create(std::move(exporter));
    std::vector<std::unique_ptr<trace_sdk::SpanProcessor>> processors;
    processors.push_back(std::move(processor));

    auto resource = opentelemetry::sdk::resource::Resource::Create(
        {{"service.name", serviceName}});

    auto providerImpl = trace_sdk::TracerProviderFactory::Create(std::move(processors), resource);
    opentelemetry::nostd::shared_ptr<trace_api::TracerProvider> provider(
        std::shared_ptr<trace_api::TracerProvider>(std::move(providerImpl)));
    trace_api::Provider::SetTracerProvider(provider);
}

void shutdown() {
    auto provider = trace_api::Provider::GetTracerProvider();
    auto* sdkProvider = static_cast<trace_sdk::TracerProvider*>(provider.get());
    if (sdkProvider != nullptr) {
        sdkProvider->Shutdown();
    }
    if (g_logFile.is_open()) {
        g_logFile.flush();
    }
}

opentelemetry::nostd::shared_ptr<trace_api::Tracer> getTracer() {
    return trace_api::Provider::GetTracerProvider()->GetTracer("papy");
}

}  // namespace otelTracing
