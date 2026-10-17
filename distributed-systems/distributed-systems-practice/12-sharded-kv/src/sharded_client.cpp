#include "sharded_client.h"

#include <chrono>
#include <fstream>
#include <sstream>
#include <thread>

namespace shard {

using raft::KVResult;

// ------------------------------------------------------------------ config
std::vector<raft::PeerConfig> loadPeerConfig(const std::string& path) {
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

bool loadRingConfig(const std::string& path, RingConfig& out, std::string& err) {
    std::ifstream f(path);
    if (!f) {
        err = "cannot open ring config: " + path;
        return false;
    }
    out = RingConfig{};
    auto slash = path.find_last_of('/');
    out.dir = (slash == std::string::npos) ? "." : path.substr(0, slash);

    std::string line;
    while (std::getline(f, line)) {
        auto hash = line.find('#');
        if (hash != std::string::npos) line.erase(hash);
        std::istringstream iss(line);
        std::string key;
        if (!(iss >> key)) continue;
        if (key == "vnodes") {
            iss >> out.vnodes;
        } else if (key == "state") {
            std::string v;
            iss >> v;
            if (v == "migrating") out.migrating = true;
            else if (v == "stable") out.migrating = false;
            else {
                err = "state must be 'stable' or 'migrating', got: " + v;
                return false;
            }
        } else if (key == "shards" || key == "old_shards") {
            auto& dst = (key == "shards") ? out.shards : out.oldShards;
            uint32_t id;
            while (iss >> id) dst.push_back(id);
        } else {
            err = "unknown ring config key: " + key;
            return false;
        }
    }
    if (out.shards.empty()) {
        err = "ring config lists no shards";
        return false;
    }
    if (out.migrating && out.oldShards.empty()) {
        err = "state is 'migrating' but old_shards is empty";
        return false;
    }
    if (out.vnodes < 1) {
        err = "vnodes must be >= 1";
        return false;
    }
    return true;
}

// -------------------------------------------------------------- RingState
std::shared_ptr<const RingState::Snapshot> RingState::build(RingConfig cfg) {
    auto snap = std::make_shared<Snapshot>();
    snap->newRing = HashRing(cfg.vnodes);
    for (uint32_t s : cfg.shards) snap->newRing.addShard(s);
    snap->oldRing = HashRing(cfg.vnodes);
    if (cfg.migrating)
        for (uint32_t s : cfg.oldShards) snap->oldRing.addShard(s);
    snap->cfg = std::move(cfg);
    return snap;
}

RingState::RingState(RingConfig cfg) : cur_(build(std::move(cfg))) {}

std::shared_ptr<const RingState::Snapshot> RingState::snapshot() const {
    std::lock_guard<std::mutex> g(mu_);
    return cur_;
}

void RingState::set(RingConfig cfg) {
    auto next = build(std::move(cfg));
    std::lock_guard<std::mutex> g(mu_);
    cur_ = std::move(next);
    version_++;
}

// ---------------------------------------------------------- ShardedClient
namespace {

KVResult rejected(const std::string& why) {
    KVResult r;
    r.ok = false;
    r.reachedCluster = true;
    r.value = "REJECTED: " + why;
    return r;
}

bool validKey(const std::string& k) {
    // The wire protocol is "VERB key value..." split on spaces, and KEYS
    // returns newline-separated keys, so a key may contain neither.
    return !k.empty() && k.find_first_of(" \n") == std::string::npos;
}

KVResult okResult(const std::string& value = "") {
    KVResult r;
    r.ok = true;
    r.reachedCluster = true;
    r.value = value;
    return r;
}

} // namespace

ShardedClient::ShardedClient(std::shared_ptr<RingState> state, ClientOptions opts)
    : state_(std::move(state)), opts_(opts) {}

namespace {
void sleepMs(int ms) {
    if (ms > 0) std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
} // namespace

uint32_t ShardedClient::ownerOf(const std::string& key) const {
    return state_->snapshot()->newRing.owner(key);
}

raft::KVClient& ShardedClient::shard(uint32_t id) {
    auto it = clients_.find(id);
    if (it == clients_.end()) {
        auto snap = state_->snapshot();
        auto peers = loadPeerConfig(snap->cfg.dir + "/shards/shard" + std::to_string(id) + ".conf");
        it = clients_.emplace(id, std::make_unique<raft::KVClient>(std::move(peers), opts_.rpcTimeoutMs)).first;
    }
    return *it->second;
}

KVResult ShardedClient::get(const std::string& key) {
    if (!validKey(key)) return rejected("bad key");
    auto snap = state_->snapshot();
    const uint32_t B = snap->newRing.owner(key);
    uint32_t A = B;
    bool moving = false;
    if (opts_.mode == Mode::Safe && snap->cfg.migrating) {
        A = snap->oldRing.owner(key);
        moving = (A != B);
    }

    auto clean = [](KVResult r) {
        if (r.ok && r.value == kTombstone) r.value.clear(); // tombstone == absent
        return r;
    };

    if (!moving) return clean(shard(B).get(key));

    // Moving key: B, then A, then B again.
    KVResult b1 = shard(B).get(key);
    if (!b1.ok) return b1;
    if (!b1.value.empty()) return clean(b1);

    sleepMs(opts_.readGapMs); // test hook: widen the b1 -> a window
    KVResult a = shard(A).get(key);
    if (!a.ok) return a;

    if (opts_.ablate & kNoRecheck) return clean(a); // ABLATION: the step under test is removed

    KVResult b2 = shard(B).get(key);
    if (!b2.ok) return b2;
    if (!b2.value.empty()) return clean(b2);

    return clean(a);
}

KVResult ShardedClient::set(const std::string& key, const std::string& value) {
    if (!validKey(key)) return rejected("bad key");
    if (value.empty()) return rejected("empty value (indistinguishable from absent)");
    if (value == kTombstone) return rejected("value is the reserved tombstone marker");
    // Writes ALWAYS go to the new owner — in steady state it is the owner;
    // while migrating it is the shard that will be authoritative from here on.
    return shard(state_->snapshot()->newRing.owner(key)).set(key, value);
}

KVResult ShardedClient::del(const std::string& key) {
    if (!validKey(key)) return rejected("bad key");
    auto snap = state_->snapshot();
    const uint32_t B = snap->newRing.owner(key);
    uint32_t A = B;
    bool moving = false;
    if (opts_.mode == Mode::Safe && snap->cfg.migrating) {
        A = snap->oldRing.owner(key);
        moving = (A != B);
    }
    if (!moving) return shard(B).del(key);

    if (opts_.ablate & kNoTombstone) { // ABLATION: plain deletes on both owners
        KVResult d1 = shard(B).del(key);
        if (!d1.ok) return d1;
        return shard(A).del(key);
    }

    // Tombstone first: the moment it commits, the key is logically deleted
    // (GET sees it before it ever looks at A). Only then remove A's copy.
    KVResult t = shard(B).set(key, kTombstone);
    if (!t.ok) return t;
    KVResult d = shard(A).del(key);
    if (!d.ok) return d;
    return t;
}

KVResult ShardedClient::casOnOwner(uint32_t owner, const std::string& key, const std::string& expected,
                                   const std::string& newValue) {
    // Read-compare-CAS loop on the raw stored value, so a leftover tombstone
    // ("absent" to clients) can still be compared against expected="" and
    // then replaced atomically (the CAS's own expected is the raw value).
    for (int attempt = 0; attempt < 8; attempt++) {
        KVResult cur = shard(owner).get(key);
        if (!cur.ok) return cur;
        const std::string effective = (cur.value == kTombstone) ? "" : cur.value;
        if (effective != expected) return okResult("MISMATCH");
        KVResult r = shard(owner).cas(key, cur.value, newValue);
        if (!r.ok) return r;
        if (r.value == "OK") return r;
        // Someone changed the key between our read and our CAS: re-read, retry.
    }
    return rejected("CAS lost the race 8 times in a row");
}

KVResult ShardedClient::cas(const std::string& key, const std::string& expected,
                            const std::string& newValue) {
    if (!validKey(key)) return rejected("bad key");
    if (expected.find(' ') != std::string::npos)
        return rejected("CAS expected value may not contain spaces (wire-format limitation)");
    if (newValue.empty()) return rejected("empty value (indistinguishable from absent)");
    if (newValue == kTombstone) return rejected("value is the reserved tombstone marker");

    auto snap = state_->snapshot();
    const uint32_t B = snap->newRing.owner(key);
    if (opts_.mode == Mode::Safe && snap->cfg.migrating) {
        uint32_t A = snap->oldRing.owner(key);
        if (A != B) {
            KVResult mv = moveKey(A, B, key); // B must hold the authoritative value before we CAS it
            if (!mv.ok) return mv;
        }
    }
    return casOnOwner(B, key, expected, newValue);
}

KVResult ShardedClient::moveKey(uint32_t from, uint32_t to, const std::string& key,
                                MoveOutcome* outcome) {
    if (outcome) *outcome = MoveOutcome::NothingToMove;
    KVResult a = shard(from).get(key);
    if (!a.ok) return a;
    if (a.value.empty() || a.value == kTombstone) return okResult(); // nothing (left) to move

    sleepMs(opts_.moveGapMs); // test hook: widen the read-source -> write-destination window

    if (opts_.mode == Mode::Naive || (opts_.ablate & kOverwriteCopy)) {
        // NAIVE / ABLATION: blind overwrite — clobbers any newer value the
        // destination received in the meantime.
        KVResult w = shard(to).set(key, a.value);
        if (!w.ok) return w;
        if (outcome) *outcome = MoveOutcome::Copied;
        return (opts_.mode == Mode::Naive) ? shard(from).del(key) : shard(from).cad(key, a.value);
    }

    // Create-if-absent on the destination: succeeds only if the key is
    // entirely unset there. If a client already wrote a newer value — or a
    // tombstone — the CAS reports MISMATCH and we correctly leave it alone.
    KVResult c = shard(to).cas(key, "", a.value);
    if (!c.ok) return c;
    if (outcome) *outcome = (c.value == "OK") ? MoveOutcome::Copied : MoveOutcome::DestinationHadNewer;

    // Either way (copied, or destination already had something newer) the
    // source copy is now redundant. Delete it only if it is still exactly
    // what we read, so we never destroy something we did not copy.
    return shard(from).cad(key, a.value);
}

} // namespace shard
