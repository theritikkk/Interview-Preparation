#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>
#include <csignal>

#include "cliHelper.hpp"
#include "threadWorks.hpp"
#include "otelTracing.hpp"

int main(int argc, char* argv[]) {

    // Base variables that are altered based off CLI arguments
    int numThreads = 1, payloadCount = 0, rateLimit = 0, ramp = 0, spike = 0;
    bool verbose = false;
    std::string target, endpoint, payload, parameter, bearer, authorization;

    cliHelper::parseArguments(argc, argv, numThreads, payloadCount, rateLimit, ramp, spike, target, endpoint, verbose, payload, parameter, bearer, authorization);

    // PAPY_TRACE_LOG, if set, writes spans to that file instead of stdout
    // — handy for the demo, where stdout is busy with Papy's own live
    // stats line. See ANALYSIS.md for the full tracing design.
    const char* traceLog = std::getenv("PAPY_TRACE_LOG");
    otelTracing::init("papy-client", traceLog ? traceLog : "");

    cliHelper::printBanner(target, endpoint, numThreads, rateLimit);

    // Signal handler controlling ctl + c program halting
    signal(SIGINT, threadWorks::signalHandler);
    std::cout << "Program is running. Press Ctrl+C to stop.\n";

    // Launch threads
    std::vector<std::thread> threads;
    threads.reserve(numThreads);
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(&threadWorks::runWorkerThread, target, endpoint, verbose, payloadCount, rateLimit, ramp, spike, payload, parameter, bearer, authorization);
    }

    // Wait for threads to finish
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    std::cout << "\nProgram has stopped.\n";
    otelTracing::shutdown();
    return 0;
}
