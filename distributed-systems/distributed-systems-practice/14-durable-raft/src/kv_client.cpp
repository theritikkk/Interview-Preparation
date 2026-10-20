#include "kv_client.h"

#include <cstdio>

namespace raft {

KVClient::KVClient(std::vector<PeerConfig> peers, int rpcTimeoutMs)
    : peers_(std::move(peers)), rpcTimeoutMs_(rpcTimeoutMs), lastKnownLeader_(0) {}

uint32_t KVClient::nextId(uint32_t afterId) const {
    for (size_t i = 0; i < peers_.size(); i++) {
        if (peers_[i].id == afterId) return peers_[(i + 1) % peers_.size()].id;
    }
    return peers_.front().id;
}

const PeerConfig* KVClient::findPeer(uint32_t id) const {
    for (auto& p : peers_)
        if (p.id == id) return &p;
    return nullptr;
}

KVResult KVClient::sendCommand(const std::string& cmd, bool /*isGet*/) {
    uint32_t tryId = (lastKnownLeader_ != 0) ? lastKnownLeader_ : peers_.front().id;
    int maxAttempts = static_cast<int>(peers_.size()) * 3 + 3;

    KVResult result;
    for (int attempt = 0; attempt < maxAttempts; attempt++) {
        const PeerConfig* target = findPeer(tryId);
        if (!target) target = &peers_.front();

        int fd = connectWithTimeout(target->host, target->port, rpcTimeoutMs_);
        if (fd < 0) {
            tryId = nextId(target->id);
            continue;
        }

        bool ok = sendFramed(fd, encodeClientRequest(cmd));
        std::string resp;
        if (ok) ok = recvFramed(fd, resp);
        ::close(fd);
        if (!ok || resp.empty()) {
            tryId = nextId(target->id);
            continue;
        }
        result.reachedCluster = true;

        ClientResult res = decodeClientResponse(resp);
        if (res.isLeader && res.ok) {
            lastKnownLeader_ = target->id; // remember for next time
            result.ok = true;
            result.value = res.value;
            result.servedBy = target->id;
            return result;
        }
        if (res.isLeader && !res.ok) {
            continue; // same leader, couldn't commit/confirm in time — retry it
        }
        // not the leader: follow the hint, or fall back to the next node
        lastKnownLeader_ = 0;
        tryId = (res.leaderHint != 0) ? res.leaderHint : nextId(target->id);
    }
    return result; // ok == false; reachedCluster tells the caller why
}

KVResult KVClient::get(const std::string& key) { return sendCommand("GET " + key, true); }
KVResult KVClient::set(const std::string& key, const std::string& value) {
    return sendCommand("SET " + key + " " + value, false);
}
KVResult KVClient::del(const std::string& key) { return sendCommand("DEL " + key, false); }
KVResult KVClient::cas(const std::string& key, const std::string& expected,
                       const std::string& newValue) {
    return sendCommand("CAS " + key + " " + expected + " " + newValue, false);
}

KVResult KVClient::cad(const std::string& key, const std::string& expected) {
    return sendCommand("CAD " + key + " " + expected, false);
}
KVResult KVClient::keys() { return sendCommand("KEYS", true); }

} // namespace raft
