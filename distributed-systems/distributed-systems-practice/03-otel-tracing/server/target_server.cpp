// target_server.cpp — a tiny HTTP "target" for Papy to load-test, whose
// only real purpose is to prove that Papy's tracing is *distributed*:
// this is a separate OS process, built from a separate binary, that
// extracts the W3C "traceparent" header Papy injects and continues the
// exact same trace_id in its own spans. See ANALYSIS.md, "How to verify
// the tracing is really distributed, not just nested."
//
// Uses the same vendored cpp-httplib Papy itself uses (so this needs no
// dependencies beyond what Papy already vendors), with neither OpenSSL
// nor zlib support compiled in — this server only needs plain HTTP.

#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

// Papy always marks its POST bodies "Content-Encoding: gzip" (see
// ANALYSIS.md) — cpp-httplib enforces that header strictly server-side
// and has no way to ignore it, so without zlib support compiled in it
// replies 415 to every POST before our handler ever runs. This matches
// what a real target would need anyway.
#define CPPHTTPLIB_ZLIB_SUPPORT
#include "dependencies/httplib.h"
#include "dependencies/json.hpp"

#include "otelTracing.hpp"
#include "httpPropagation.hpp"
#include "opentelemetry/trace/propagation/http_trace_context.h"
#include "opentelemetry/trace/scope.h"

namespace trace_api = opentelemetry::trace;
using json = nlohmann::json;

namespace {

std::string currentThreadIdString() {
    std::ostringstream oss;
    oss << std::this_thread::get_id();
    return oss.str();
}

httplib::Server* g_server = nullptr;

void handleSigint(int) {
    // svr.stop() just flips an atomic flag the accept loop checks; it's
    // safe enough to call directly from a signal handler for this demo.
    // Calling it (rather than std::exit) lets main() fall through to
    // otelTracing::shutdown() normally.
    if (g_server != nullptr) {
        g_server->stop();
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    int port = 8080;
    std::string logPath;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if (arg == "--log" && i + 1 < argc) {
            logPath = argv[++i];
        } else if (arg == "-h" || arg == "--help") {
            std::cout << "Usage: target_server [--port N] [--log path]\n"
                      << "  --port N   Port to listen on (default: 8080)\n"
                      << "  --log path Write spans here instead of stdout\n"
                      << "             (PAPY_TRACE_LOG does the same for Papy itself)\n";
            return 0;
        }
    }

    otelTracing::init("papy-target-server", logPath);
    auto tracer = otelTracing::getTracer();

    httplib::Server svr;
    g_server = &svr;

    // One handler for any method/path Papy might hit (GET "/", GET
    // "/printJson", POST "/addMatch", ...) — this is a demo target, not
    // a real service, so it doesn't care what the endpoint is.
    std::atomic<int64_t> requestsServed{0};
    auto handle = [&](const httplib::Request& req, httplib::Response& res) {
        // req.headers is `const Headers` here (req is a const&); Get()
        // is the only thing the carrier does with it on this path, so
        // stripping const to satisfy HttplibHeaderCarrier's constructor
        // (shared with the inject side, which does need to mutate) is
        // safe.
        HttplibHeaderCarrier carrier(const_cast<httplib::Headers&>(req.headers));
        opentelemetry::context::Context currentCtx =
            opentelemetry::context::RuntimeContext::GetCurrent();
        auto extractedCtx =
            trace_api::propagation::HttpTraceContext().Extract(carrier, currentCtx);

        // If Papy's "traceparent" header was present and valid, this is
        // a *real* parent SpanContext from another process; if absent,
        // IsValid() is false and StartSpan falls back to a root span —
        // i.e. this server works fine standalone, outside the demo too.
        trace_api::SpanContext parentSpanContext = trace_api::GetSpan(extractedCtx)->GetContext();
        bool continuedTrace = parentSpanContext.IsValid();

        trace_api::StartSpanOptions opts;
        opts.parent = parentSpanContext;

        auto span = tracer->StartSpan("HandleRequest", opts);
        span->SetAttribute("http.method", req.method);
        span->SetAttribute("http.target", req.path);
        span->SetAttribute("thread.id", currentThreadIdString());
        span->SetAttribute("trace.continued_from_client", continuedTrace);
        auto scope = tracer->WithActiveSpan(span);

        // A bit of fake work as its own child span, so the trace has
        // more than one span on the server side too.
        {
            auto workSpan = tracer->StartSpan("ProcessPayload");
            auto workScope = tracer->WithActiveSpan(workSpan);
            if (!req.body.empty()) {
                workSpan->SetAttribute("payload.bytes", static_cast<int64_t>(req.body.size()));
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            workSpan->End();
        }

        int64_t seq = ++requestsServed;

        json body;
        body["status"] = "ok";
        body["service"] = "papy-target-server";
        body["path"] = req.path;
        body["trace_continued"] = continuedTrace;
        body["seq"] = seq;
        res.set_content(body.dump(), "application/json");
        res.status = 200;

        span->SetAttribute("http.status_code", 200);
        span->SetAttribute("request.sequence", seq);
        span->SetStatus(trace_api::StatusCode::kOk);
        span->End();
    };

    svr.Get(R"(/.*)", handle);
    svr.Post(R"(/.*)", handle);

    std::cout << "papy-target-server listening on http://0.0.0.0:" << port << "\n";
    std::cout << "Press Ctrl+C to stop.\n";
    signal(SIGINT, handleSigint);

    svr.listen("0.0.0.0", port);

    std::cout << "\npapy-target-server stopped. Served " << requestsServed.load()
              << " request(s).\n";
    otelTracing::shutdown();
    return 0;
}
