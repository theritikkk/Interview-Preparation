// kv_client.h — a real client library for the Raft-backed KV store, used
// by both the kvctl CLI and the partition-demo test harness. Replaces
// Project 1's inline-in-main() retry loop with a reusable class: connect,
// follow leader redirects, retry against the right node, and remember the
// last known leader across calls (a real client optimization — once you
// know who's leading, don't re-probe from peers.front() every time).
#pragma once

#include "raft_node.h"

#include <string>
#include <vector>

namespace raft {

struct KVResult {
    bool ok = false;         // true iff the cluster actually committed/answered this
    bool reachedCluster = false; // false only if every node was unreachable
    std::string value;       // GET's value, or CAS's "OK"/"MISMATCH"; empty for SET/DEL
    uint32_t servedBy = 0;   // which node id actually answered
};

class KVClient {
public:
    explicit KVClient(std::vector<PeerConfig> peers, int rpcTimeoutMs = 1000);

    KVResult get(const std::string& key);
    KVResult set(const std::string& key, const std::string& value);
    KVResult del(const std::string& key);
    KVResult cas(const std::string& key, const std::string& expected, const std::string& newValue);
    // Compare-and-delete: erases `key` iff its value equals `expected`
    // ("" == absent). value is "OK" or "MISMATCH", as with cas().
    KVResult cad(const std::string& key, const std::string& expected);
    // Every key the shard holds, as one string: keys separated by '\n',
    // each followed by one. Linearizable (read-index), like get().
    KVResult keys();

private:
    std::vector<PeerConfig> peers_;
    int rpcTimeoutMs_;
    uint32_t lastKnownLeader_; // sticky leader hint across calls; 0 == "no idea yet"

    KVResult sendCommand(const std::string& cmd, bool isGet);
    uint32_t nextId(uint32_t afterId) const;
    const PeerConfig* findPeer(uint32_t id) const;
};

} // namespace raft
