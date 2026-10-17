// direct_probe.cpp — connects to exactly ONE node, sends exactly one raw
// client request, and prints exactly what that node said: no retry, no
// following a leaderHint elsewhere. kvctl/KVClient deliberately hide that
// kind of detail behind automatic retries, which is right for a real
// client but wrong for the partition demo, where the whole point is to
// see what one SPECIFIC node (a partitioned old leader) answers on its
// own, unfiltered.
#include "raft_node.h"

#include <cstdio>
#include <string>

int main(int argc, char** argv) {
    if (argc != 4) {
        std::fprintf(stderr, "usage: %s <host> <port> <GET key | SET key value>\n", argv[0]);
        return 2;
    }
    std::string host = argv[1];
    int port = std::stoi(argv[2]);
    std::string cmd = argv[3];

    int fd = raft::connectWithTimeout(host, static_cast<uint16_t>(port), 2000);
    if (fd < 0) {
        std::printf("UNREACHABLE\n");
        return 1;
    }
    bool ok = raft::sendFramed(fd, raft::encodeClientRequest(cmd));
    std::string resp;
    if (ok) ok = raft::recvFramed(fd, resp);
    ::close(fd);
    if (!ok || resp.empty()) {
        std::printf("NO_RESPONSE (connection accepted but no reply — consistent with a node that's\n");
        std::printf("busy waiting on something that will never happen, e.g. a majority that will\n");
        std::printf("never ack because this node is partitioned away from it)\n");
        return 1;
    }
    raft::ClientResult res = raft::decodeClientResponse(resp);
    std::printf("ok=%s isLeader=%s leaderHint=%u value=\"%s\"\n", res.ok ? "true" : "false",
               res.isLeader ? "true" : "false", res.leaderHint, res.value.c_str());
    return 0;
}
