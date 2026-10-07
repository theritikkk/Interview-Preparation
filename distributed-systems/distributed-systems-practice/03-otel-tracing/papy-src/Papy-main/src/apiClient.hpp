#pragma once

#include <string>

// project dependencies
#include "dependencies/json.hpp"

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "dependencies/httplib.h"

class apiClient {
public:
    explicit apiClient(const std::string& serverAddress);

    void setEndpoint(const std::string& endpoint);
    void setParameter(const std::string& parameter);
    void setPayload(const nlohmann::json& payload);

    // Full target URL, for tracing span attributes (http.url) — not used
    // for the actual request (the real server address lives inside the
    // httplib client objects), just for describing the request in spans.
    std::string describeTarget() const { return serverAddress + (endpoint.empty() ? "/" : endpoint); }

    // extraHeaders lets a caller attach additional headers — in practice,
    // this is how the "traceparent" header (W3C distributed-tracing
    // context) gets onto the wire; see otelTracing.hpp / ANALYSIS.md.
    std::string sendGETRequest(std::string bearer, std::string authorization,
                               const httplib::Headers& extraHeaders = {});
    std::string sendPOSTRequest(std::string bearer, std::string authorization,
                                const httplib::Headers& extraHeaders = {});

private:
    std::unique_ptr<httplib::Client> client;
    std::unique_ptr<httplib::SSLClient> sslClient;

    std::string endpoint;
    std::string parameter;
    nlohmann::json payload;
    std::string serverAddress;

    std::string errorToString(httplib::Error err);
};
