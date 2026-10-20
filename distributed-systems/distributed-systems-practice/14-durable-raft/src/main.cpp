// main.cpp — process entry point for one Raft cluster node.
// Usage: raft_node <cluster_config_file> <my_id>
//
// Persistent state (Project 14) lives in $RAFT_DATA_DIR/<config-stem>-node<id>/
// (RAFT_DATA_DIR defaults to ./data): a write-ahead log and a snapshot file.
// SIGUSR2 is a TEST-ONLY simulated power failure (see RaftNode::simulatePowerLoss).
//
// SIGUSR1 toggles a simulated network partition for this node (see
// RaftNode::toggleDeafened / deafened_ in raft_node.h) — used by
// scripts/partition_demo.sh to demonstrate why read-index linearizable
// reads matter. `kill -USR1 <pid>` once to partition, again to heal.

#include "raft_node.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <thread>

namespace {
std::atomic<bool> g_stop{false};
void onSignal(int) { g_stop = true; }

raft::RaftNode* g_node = nullptr;
void onUsr1(int) {
    if (g_node) g_node->toggleDeafened();
}
std::atomic<bool> g_powerLoss{false};
void onUsr2(int) { g_powerLoss = true; } // acted on from the main loop, not in the handler

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
    std::signal(SIGUSR1, onUsr1);
    std::signal(SIGUSR2, onUsr2);

    // data dir: $RAFT_DATA_DIR/<config file stem>-node<id>
    std::string stem = argv[1];
    if (auto slash = stem.find_last_of('/'); slash != std::string::npos) stem = stem.substr(slash + 1);
    if (auto dot = stem.find_last_of('.'); dot != std::string::npos) stem = stem.substr(0, dot);
    const char* base = std::getenv("RAFT_DATA_DIR");
    std::string dataDir = std::string(base ? base : "data") + "/" + stem + "-node" + std::to_string(myId);

    raft::RaftNode node(myId, myPort, peers, "node" + std::to_string(myId), dataDir);
    g_node = &node;
    node.start();
    // Found by actually running the partition demo back-to-back (see
    // ANALYSIS.md): a node whose acceptorLoop fails to bind() used to set
    // its internal running_ flag false and just leak a dead thread forever
    // — main() here was only watching g_stop (set by SIGINT/SIGTERM), so
    // the process never actually exited, left its PID file pointing at a
    // zombie-ish process, and the next cluster start raced it for the
    // port. Now main() exits as soon as EITHER signal fires.
    while (!g_stop.load() && node.isRunning()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        if (g_powerLoss.load()) node.simulatePowerLoss(); // does not return
    }
    bool selfStopped = !node.isRunning();
    node.stop();
    return selfStopped ? 1 : 0;
}
