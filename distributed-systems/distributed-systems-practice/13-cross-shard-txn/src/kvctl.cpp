// kvctl.cpp — CLI for the Raft-backed KV store. Thin wrapper over the real
// client library in kv_client.h/.cpp — all the leader-finding/retry logic
// lives there now, not here, so anything else that wants to talk to the
// cluster (the partition-demo harness, say) can link kv_client directly
// instead of shelling out to this binary.
//
// Usage: kvctl <cluster_config_file> <GET key | SET key value | DEL key |
//                                     CAS key expected new | CAD key expected | KEYS>

#include "kv_client.h"

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
} // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr,
                     "usage: %s <cluster_config_file> <GET key | SET key value | "
                     "DEL key | CAS key expected new | CAD key expected | KEYS>\n",
                     argv[0]);
        return 1;
    }
    auto peers = loadConfig(argv[1]);
    if (peers.empty()) {
        std::fprintf(stderr, "config file empty or unreadable\n");
        return 1;
    }

    std::string verb = argv[2];
    raft::KVClient client(peers);
    raft::KVResult res;

    if (verb == "GET" && argc == 4) {
        res = client.get(argv[3]);
        if (res.ok) std::printf("OK value=\"%s\" (from leader: node %u)\n", res.value.c_str(), res.servedBy);
    } else if (verb == "SET" && argc == 5) {
        res = client.set(argv[3], argv[4]);
        if (res.ok) std::printf("OK (committed via leader: node %u)\n", res.servedBy);
    } else if (verb == "DEL" && argc == 4) {
        res = client.del(argv[3]);
        if (res.ok) std::printf("OK (committed via leader: node %u)\n", res.servedBy);
    } else if (verb == "CAS" && argc == 6) {
        res = client.cas(argv[3], argv[4], argv[5]);
        if (res.ok) std::printf("%s (via leader: node %u)\n", res.value.c_str(), res.servedBy);
    } else if (verb == "CAD" && argc == 5) {
        res = client.cad(argv[3], argv[4]);
        if (res.ok) std::printf("%s (via leader: node %u)\n", res.value.c_str(), res.servedBy);
    } else if (verb == "KEYS" && argc == 3) {
        res = client.keys();
        if (res.ok) {
            size_t n = 0;
            for (char c : res.value) n += (c == '\n');
            std::printf("%zu keys (from leader: node %u)\n%s", n, res.servedBy, res.value.c_str());
        }
    } else {
        std::fprintf(stderr,
                     "usage: %s <cluster_config_file> <GET key | SET key value | "
                     "DEL key | CAS key expected new | CAD key expected | KEYS>\n",
                     argv[0]);
        return 1;
    }

    if (!res.ok) {
        if (!res.reachedCluster)
            std::fprintf(stderr, "FAILED: no node in the cluster was reachable\n");
        else
            std::fprintf(stderr, "FAILED: could not reach a leader / get a committed answer in time\n");
        return 1;
    }
    return 0;
}
