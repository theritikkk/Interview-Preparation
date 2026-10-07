#include <atomic>
#include <chrono>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

// project dependencies
#include "dependencies/json.hpp"
// https://github.com/madler/zlib
#include "dependencies/gzip/zlib.h"

#include "apiClient.hpp"
#include "matchBuilder.hpp"
#include "millisecondClock.hpp"
#include "myRandom.hpp"
#include "oceanBuilder.hpp"
#include "configPayload.hpp"
#include "threadWorks.hpp"

// distributed tracing
#include "otelTracing.hpp"
#include "httpPropagation.hpp"
#include "opentelemetry/trace/propagation/http_trace_context.h"
#include "opentelemetry/trace/scope.h"

using json = nlohmann::json;
namespace trace_api = opentelemetry::trace;

namespace {
std::string currentThreadIdString() {
    std::ostringstream oss;
    oss << std::this_thread::get_id();
    return oss.str();
}

// Marks the span Error with `detail` unless the response was a plain
// "200", and always records the response as the http.status_code
// attribute when a numeric code can be found in it.
void annotateResponse(const opentelemetry::nostd::shared_ptr<trace_api::Span>& span,
                      const std::string& response) {
    if (response == "200") {
        span->SetAttribute("http.status_code", 200);
        span->SetStatus(trace_api::StatusCode::kOk);
        return;
    }
    auto digitsStart = response.find_last_of(' ');
    if (digitsStart != std::string::npos) {
        try {
            int code = std::stoi(response.substr(digitsStart + 1));
            span->SetAttribute("http.status_code", code);
        } catch (const std::exception&) {
            // response didn't end in a parseable status code (e.g. a
            // connection-level error) — nothing more to attach.
        }
    }
    span->SetStatus(trace_api::StatusCode::kError, response);
}
}  // namespace


// Static member initialization
std::atomic<bool> threadWorks::isProgramActive(true);
std::mutex threadWorks::consoleMutex;
std::atomic<int> threadWorks::totalPayloadsSent(0);
std::atomic<int> threadWorks::totalPayloadsSuccessful(0);
std::atomic<int> threadWorks::packetsPerSecond(0);
std::mutex errorMutex;

// Static signal handler function
void threadWorks::signalHandler(int signal) {
    std::lock_guard<std::mutex> guard(consoleMutex);
    std::cout << "\nStopping program..." << std::endl;
    isProgramActive = false;
}
// Function to compress data using zlib
std::string threadWorks::gzip_compress(const std::string &data) {
    z_stream zs;
    memset(&zs, 0, sizeof(zs));

    if (deflateInit2(&zs, Z_BEST_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        throw std::runtime_error("Failed to initialize zlib for compression");
    }

    zs.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(data.data()));
    zs.avail_in = data.size();

    char buffer[4096];
    std::string compressed;

    do {
        zs.next_out = reinterpret_cast<Bytef *>(buffer);
        zs.avail_out = sizeof(buffer);

        if (deflate(&zs, Z_FINISH) == Z_STREAM_ERROR) {
            deflateEnd(&zs);
            throw std::runtime_error("Compression failed");
        }

        compressed.append(buffer, sizeof(buffer) - zs.avail_out);
    } while (zs.avail_out == 0);

    deflateEnd(&zs);
    return compressed;
}

void threadWorks::sendRequest(apiClient& client, bool verbose, std::string payload, millisecondClock& clock, std::string bearer, std::string authorization) {
    // A third pre-existing Papy defect, found while verifying the file
    // payload path end-to-end (see ANALYSIS.md): this used to lowercase
    // `payload` itself, which is reused below as a literal filesystem
    // path when it's none of the three known presets. That silently
    // broke any custom payload file whose real path has uppercase
    // characters, on any case-sensitive filesystem (every Linux
    // filesystem; not macOS's default APFS, which is why this likely
    // went unnoticed). Comparing against a lowercased COPY keeps preset
    // matching case-insensitive without mangling the path.
    std::string payloadLower = payload;
    std::transform(payloadLower.begin(), payloadLower.end(), payloadLower.begin(), ::tolower);
    std::string response;

    // --- tracing: one root span per request, covering payload generation
    // and the HTTP call as child spans. This is the unit of work a real
    // distributed trace follows across process boundaries (see
    // ANALYSIS.md) — everything from here to rootSpan->End() belongs to
    // one trace_id.
    auto tracer = otelTracing::getTracer();
    auto rootSpan = tracer->StartSpan("LoadTestRequest");
    rootSpan->SetAttribute("thread.id", currentThreadIdString());
    rootSpan->SetAttribute("payload.kind", payload.empty() ? "get" : payload);
    auto rootScope = tracer->WithActiveSpan(rootSpan);

    // Performs the actual HTTP call as its own child span, injecting the
    // real W3C "traceparent" header so the target server (if it's also
    // instrumented — see server/target_server.cpp) continues this exact
    // trace rather than starting a new, disconnected one.
    auto performHttpCall = [&](const char* method, auto&& doCall) -> std::string {
        auto httpSpan = tracer->StartSpan("HTTPRequest");
        httpSpan->SetAttribute("http.method", method);
        httpSpan->SetAttribute("http.url", client.describeTarget());
        auto httpScope = tracer->WithActiveSpan(httpSpan);

        httplib::Headers traceHeaders;
        HttplibHeaderCarrier carrier(traceHeaders);
        trace_api::propagation::HttpTraceContext().Inject(
            carrier, opentelemetry::context::RuntimeContext::GetCurrent());

        std::string resp = doCall(traceHeaders);
        annotateResponse(httpSpan, resp);
        httpSpan->End();
        return resp;
    };

    // TODO: Test out what it takes to implement reading the custom payload as a file path from anywhere rather than from just in the same directory as the binary executable file.

    // This is where you add additional options for handling different preset payloads
    // You add another option to the conditional and however you want to handle it
    if (payload.empty()) {
        response = performHttpCall("GET", [&](const httplib::Headers& h) {
            return client.sendGETRequest(bearer, authorization, h);
        });
    } else if (payloadLower == "lol") {
        auto genSpan = tracer->StartSpan("GeneratePayload");
        genSpan->SetAttribute("payload.kind", "lol");
        auto genScope = tracer->WithActiveSpan(genSpan);
        nlohmann::json lolPayload = matchBuilder::randomMatch();
        std::string compressedLolPayload = gzip_compress(lolPayload.dump());
        client.setPayload(compressedLolPayload);
        genSpan->End();

        response = performHttpCall("POST", [&](const httplib::Headers& h) {
            return client.sendPOSTRequest(bearer, authorization, h);
        });
    } else if (payloadLower == "ocean") {
        auto genSpan = tracer->StartSpan("GeneratePayload");
        genSpan->SetAttribute("payload.kind", "ocean");
        auto genScope = tracer->WithActiveSpan(genSpan);
        // apiClient::sendPOSTRequest always declares "Content-Encoding:
        // gzip" (a pre-existing Papy behavior — see ANALYSIS.md, "A
        // second pre-existing Papy defect"), so the body must actually
        // be gzip-compressed or any server that honors that header will
        // reject it outright (cpp-httplib itself does, with a 415).
        client.setPayload(gzip_compress(oceanBuilder::randomOcean().dump()));
        genSpan->End();

        response = performHttpCall("POST", [&](const httplib::Headers& h) {
            return client.sendPOSTRequest(bearer, authorization, h);
        });
    } else if (payloadLower == "payload.json") {
        auto genSpan = tracer->StartSpan("GeneratePayload");
        genSpan->SetAttribute("payload.kind", "payload.json");
        auto genScope = tracer->WithActiveSpan(genSpan);
        client.setPayload(gzip_compress(configPayload::readConfig().dump()));
        genSpan->End();

        response = performHttpCall("POST", [&](const httplib::Headers& h) {
            return client.sendPOSTRequest(bearer, authorization, h);
        });
    } else {
        auto genSpan = tracer->StartSpan("GeneratePayload");
        genSpan->SetAttribute("payload.kind", "file");
        genSpan->SetAttribute("payload.file", payload);
        auto genScope = tracer->WithActiveSpan(genSpan);

        std::ifstream f(payload);
        json jsonPayload;

        // Catches payload not found errors essentially. If it can not parse it
        // then it was mostly not found or malformed.
        //
        // A fourth pre-existing Papy defect, found alongside the one
        // above (see ANALYSIS.md): this used to declare `json
        // jsonPayload = json::parse(f);` right here, which *shadows*
        // the outer `jsonPayload` instead of assigning to it. Parsing
        // could succeed yet the outer (always-null) variable is what
        // gets sent below, so this path silently POSTed the literal
        // text "null" no matter what the file actually contained.
        try {
            jsonPayload = json::parse(f);
        } catch (const nlohmann::json_abi_v3_11_3::detail::parse_error& e) {
            genSpan->SetStatus(trace_api::StatusCode::kError, "payload file not found or invalid JSON");
            genSpan->End();
            rootSpan->SetStatus(trace_api::StatusCode::kError);
            rootSpan->End();
            std::lock_guard<std::mutex> lock(errorMutex);
            std::cerr << "Parsing Error: Payload file not found or incorrect JSON" << std::endl;
            std::exit(EXIT_FAILURE);
        }

        client.setPayload(gzip_compress(jsonPayload.dump()));
        genSpan->End();

        response = performHttpCall("POST", [&](const httplib::Headers& h) {
            return client.sendPOSTRequest(bearer, authorization, h);
        });
    }

    rootSpan->End();

    if (verbose) {
        std::cout << "Response: " << response << std::endl;
    } else {
        

        // Mutex guarded packets per second calculations
        std::lock_guard<std::mutex> guard(consoleMutex);

        // You can run it for two days before you get an integer overflow LOL
        int elapsedSec = clock.elapsedMilliseconds() / 1000;
        if (elapsedSec > 0) {
            packetsPerSecond = totalPayloadsSent / elapsedSec;
        } else {
            packetsPerSecond = 0; // Default value if elapsed time is too short
        }

        if (response == "200") {
            totalPayloadsSuccessful++;
        }

        // Having totalPayloadsSent++ at the top of the namespace caused the 
        // failed stat to blip
        totalPayloadsSent++;

        // Limit how frequently we display the update message
        static auto last_update_time = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update_time);

        // 20 Hz refresh rate = less than screen, almost max human eye
        constexpr unsigned int DELAY_MS = 75;
        if (elapsed.count() < DELAY_MS) {
            // It's been less than the delay time, so skip displaying an update.
            return;
        }

        last_update_time = now;

        // Print updated info on the same line
        std::cout << "\rTotal Sent: " << totalPayloadsSent
                  << " | Successful: " << totalPayloadsSuccessful
                  << " | Failed: " << (totalPayloadsSent - totalPayloadsSuccessful)
                  << " | Packets/s: " << packetsPerSecond
                  << " | Elapsed Time: " << clock.elapsedMilliseconds()
		  << "     "  // clear a few characters past the end
                  << std::flush;
    }
}

// Orchestrates the sending of requests and main loop for program
void threadWorks::runWorkerThread(const std::string& targetURL, const std::string& endpoint, bool verbose, int payloadCount, int rateLimit, int ramp, int spike, std::string payload, std::string parameter, std::string bearer, std::string authorization) {
    millisecondClock clock;
    apiClient client(targetURL);
    client.setEndpoint(endpoint);
    client.setParameter(parameter);

    clock.start();
    while (isProgramActive) {
        sendRequest(client, verbose, payload, clock, bearer, authorization);
        
        if (payloadCount > 0 && --payloadCount == 0) break;

        if (rateLimit) {
            std::this_thread::sleep_for(std::chrono::milliseconds(rateLimit));
        }
        if (ramp > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(ramp));
            ramp = std::max(ramp / 2, 1);
        } else if (spike > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(myRandom::generateRandomInt(0, spike)));
        }
    }
}
