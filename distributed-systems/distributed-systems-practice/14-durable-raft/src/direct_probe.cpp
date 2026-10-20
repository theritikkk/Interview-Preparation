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
    if (argc != 4 && argc != 8) {
        std::fprintf(stderr, "usage: %s <host> <port> <command (GET key | SET key value | INFO ...)>\n"
                             "       %s <host> <port> VOTE <term> <candidateId> <lastLogIndex> <lastLogTerm>\n",
                     argv[0], argv[0]);
        return 2;
    }
    std::string host = argv[1];
    int port = std::stoi(argv[2]);
    std::string cmd = argv[3];

    if (argc == 8 && cmd == "VOTE") {
        // A raw RequestVote RPC, as a candidate would send it (Project 14: shows
        // whether a node remembers its vote across a restart).
        int vfd = raft::connectWithTimeout(host, static_cast<uint16_t>(port), 2000);
        if (vfd < 0) { std::printf("UNREACHABLE\n"); return 1; }
        raft::ByteWriter w;
        w.putU8(raft::MSG_REQUEST_VOTE);
        w.putU64(std::stoull(argv[4]));
        w.putU32(static_cast<uint32_t>(std::stoul(argv[5])));
        w.putU64(std::stoull(argv[6]));
        w.putU64(std::stoull(argv[7]));
        std::string vresp;
        bool vok = raft::sendFramed(vfd, w.bytes()) && raft::recvFramed(vfd, vresp);
        ::close(vfd);
        if (!vok || vresp.empty()) { std::printf("NO_RESPONSE\n"); return 1; }
        raft::ByteReader r(vresp);
        r.getU8();
        uint64_t rterm = r.getU64();
        bool granted = r.getU8() != 0;
        std::printf("vote %s (node's term now %llu)\n", granted ? "GRANTED" : "DENIED", (unsigned long long)rterm);
        return 0;
    }

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
