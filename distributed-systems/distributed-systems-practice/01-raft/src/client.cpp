// client.cpp — tiny CLI client for the toy Raft key/value store.
// Usage: raft_client <cluster_config_file> <SET key value | GET key>
//
// Does not know who the leader is in advance: it tries a node, and if that
// node isn't the leader, follows the "leaderHint" in the response (or moves
// to the next node in the config if there's no hint yet, e.g. right after
// a crash before anyone has heard from the new leader).

#include "raft_node.h"

#include <cstdio>
#include <fstream>
#include <sstream>

namespace {
std::vector<raft::PeerConfig> loadConfig(const std::string& path) {
    std::vector<raft::PeerConfig> out;
    std::ifstream f(path);
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

uint32_t nextId(const std::vector<raft::PeerConfig>& peers, uint32_t afterId) {
    for (size_t i = 0; i < peers.size(); i++) {
        if (peers[i].id == afterId) return peers[(i + 1) % peers.size()].id;
    }
    return peers.front().id;
}

const raft::PeerConfig* findPeer(const std::vector<raft::PeerConfig>& peers, uint32_t id) {
    for (auto& p : peers)
        if (p.id == id) return &p;
    return nullptr;
}
} // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr, "usage: %s <cluster_config_file> <SET key value | GET key>\n", argv[0]);
        return 1;
    }
    auto peers = loadConfig(argv[1]);
    if (peers.empty()) {
        std::fprintf(stderr, "config file empty or unreadable\n");
        return 1;
    }
    std::string cmd;
    for (int i = 2; i < argc; i++) {
        if (i > 2) cmd += " ";
        cmd += argv[i];
    }
    bool isGet = cmd.rfind("GET ", 0) == 0;

    uint32_t tryId = peers.front().id;
    int maxAttempts = static_cast<int>(peers.size()) * 3 + 3;

    for (int attempt = 0; attempt < maxAttempts; attempt++) {
        const raft::PeerConfig* target = findPeer(peers, tryId);
        if (!target) target = &peers.front();

        int fd = raft::connectWithTimeout(target->host, target->port, 1000);
        if (fd < 0) {
            std::fprintf(stderr, "  (node %u unreachable, trying another node...)\n", target->id);
            tryId = nextId(peers, target->id);
            continue;
        }

        bool ok = raft::sendFramed(fd, raft::encodeClientRequest(cmd));
        std::string resp;
        if (ok) ok = raft::recvFramed(fd, resp);
        ::close(fd);
        if (!ok || resp.empty()) {
            std::fprintf(stderr, "  (node %u dropped the connection, trying another node...)\n",
                         target->id);
            tryId = nextId(peers, target->id);
            continue;
        }

        raft::ClientResult res = raft::decodeClientResponse(resp);
        if (res.isLeader && res.ok) {
            if (isGet)
                std::printf("OK value=\"%s\" (from leader: node %u)\n", res.value.c_str(), target->id);
            else
                std::printf("OK (committed via leader: node %u)\n", target->id);
            return 0;
        }
        if (res.isLeader && !res.ok) {
            std::fprintf(stderr, "  (leader node %u could not commit in time, retrying it...)\n",
                         target->id);
            continue; // same leader is still probably right; try again
        }
        // not the leader
        tryId = (res.leaderHint != 0) ? res.leaderHint : nextId(peers, target->id);
        std::fprintf(stderr, "  (node %u says it isn't the leader, trying node %u...)\n",
                     target->id, tryId);
    }

    std::fprintf(stderr, "FAILED: could not reach a leader after %d attempts\n", maxAttempts);
    return 1;
}
