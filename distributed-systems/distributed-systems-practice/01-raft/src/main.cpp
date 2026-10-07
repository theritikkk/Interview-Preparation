// main.cpp — process entry point for one Raft cluster node.
// Usage: raft_node <cluster_config_file> <my_id>

#include "raft_node.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <thread>

namespace {
std::atomic<bool> g_stop{false};
void onSignal(int) { g_stop = true; }

std::vector<raft::PeerConfig> loadConfig(const std::string& path) {
    std::vector<raft::PeerConfig> out;
    std::ifstream f(path);
    if (!f) {
        std::fprintf(stderr, "cannot open config file: %s\n", path.c_str());
        return out;
    }
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream iss(line);
        raft::PeerConfig p;
        int port = 0;
        if (iss >> p.id >> p.host >> port) {
            p.port = static_cast<uint16_t>(port);
            out.push_back(p);
        }
    }
    return out;
}
} // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr, "usage: %s <cluster_config_file> <my_id>\n", argv[0]);
        return 1;
    }
    auto all = loadConfig(argv[1]);
    if (all.empty()) {
        std::fprintf(stderr, "config file empty or unreadable\n");
        return 1;
    }
    uint32_t myId = static_cast<uint32_t>(std::stoul(argv[2]));

    uint16_t myPort = 0;
    std::vector<raft::PeerConfig> peers;
    for (auto& p : all) {
        if (p.id == myId)
            myPort = p.port;
        else
            peers.push_back(p);
    }
    if (myPort == 0) {
        std::fprintf(stderr, "node id %u not found in %s\n", myId, argv[1]);
        return 1;
    }

    std::signal(SIGINT, onSignal);
    std::signal(SIGTERM, onSignal);

    raft::RaftNode node(myId, myPort, peers, "node" + std::to_string(myId));
    node.start();
    while (!g_stop.load()) std::this_thread::sleep_for(std::chrono::milliseconds(200));
    node.stop();
    return 0;
}
