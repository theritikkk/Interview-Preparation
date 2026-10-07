#pragma once
// httpPropagation.hpp — a TextMapCarrier over httplib::Headers, so the
// real opentelemetry-cpp W3C propagator (HttpTraceContext) can read and
// write the "traceparent" header directly on Papy's actual HTTP requests
// (client side) and the demo target server's actual incoming requests
// (server side). httplib::Headers is the same type on both ends
// (std::unordered_multimap<std::string, std::string, case-insensitive>),
// so one carrier serves both directions.
//
// This is what makes the tracing genuinely *distributed* rather than
// just structured logging inside one process: the trace_id created for
// a request on the Papy (client) side is the same trace_id the target
// server sees when it extracts this header, so both processes' spans
// join into one trace.

#include "dependencies/httplib.h"
#include "opentelemetry/context/propagation/text_map_propagator.h"

class HttplibHeaderCarrier : public opentelemetry::context::propagation::TextMapCarrier {
public:
    explicit HttplibHeaderCarrier(httplib::Headers& headers) : headers_(headers) {}

    opentelemetry::nostd::string_view Get(
        opentelemetry::nostd::string_view key) const noexcept override {
        auto it = headers_.find(std::string(key));
        if (it == headers_.end()) {
            return "";
        }
        return it->second;
    }

    void Set(opentelemetry::nostd::string_view key,
             opentelemetry::nostd::string_view value) noexcept override {
        headers_.emplace(std::string(key), std::string(value));
    }

private:
    httplib::Headers& headers_;
};
