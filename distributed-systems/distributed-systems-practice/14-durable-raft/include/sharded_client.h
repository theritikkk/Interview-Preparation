// sharded_client.h — routes each key to the Raft group (shard) that owns it,
// and keeps reads/writes correct WHILE the shard map is changing.
//
// Steady state is trivial: owner = ring.owner(key); talk to that shard's
// KVClient. The interesting case is a rebalance: when a shard is added, about
// 1/(N+1) of keys change owner, and clients and the migrator are all acting
// at once. A key that is "moving" has an OLD owner A (before the change) and
// a NEW owner B (after). The safe protocol, per operation on a moving key:
//
//   GET   read B; if B has something (value or tombstone) that is the answer.
//         Otherwise read A, then read B AGAIN: if B now has something use it,
//         else use A. (The second B read closes a race with the migrator,
//         which writes B *before* deleting A — see ANALYSIS.md.)
//   SET   write B only. B is authoritative the moment it is non-empty, so A's
//         stale copy is hidden and the migrator can never overwrite it.
//   DEL   write a TOMBSTONE to B, then delete A's copy. The tombstone stops
//         the migrator from "resurrecting" the key by copying A's old value
//         into B afterwards.
//   CAS   first move the key (A -> B), then compare-and-swap on B.
//
// The migrator moves a key with create-if-absent on B (CAS expected="") and a
// guarded delete on A (CAD), so it can never clobber a newer client write.
//
// Mode::Naive is the deliberately-broken baseline used to show what goes wrong
// without all this: it routes every key by the NEW ring immediately and has no
// fallback to A, no tombstones.

#pragma once

#include "hash_ring.h"
#include "kv_client.h"

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace shard {

// Reserved value written by a DEL on a moving key. Clients treat it as
// "absent"; SET refuses to store it so users can't forge one.
inline const std::string kTombstone = "__TOMBSTONE__";

struct RingConfig {
    int vnodes = 128;
    bool migrating = false;
    std::vector<uint32_t> shards;    // the ring clients are converging on (the NEW ring when migrating)
    std::vector<uint32_t> oldShards; // the ring before the change; only meaningful while migrating
    std::string dir;                 // directory of ring.conf; shard N's Raft group is <dir>/shards/shardN.conf
};

bool loadRingConfig(const std::string& path, RingConfig& out, std::string& err);
std::vector<raft::PeerConfig> loadPeerConfig(const std::string& path);

// A shared, swappable view of the ring, so a state change (migrating ->
// stable) becomes visible to every thread's client at once.
class RingState {
public:
    struct Snapshot {
        RingConfig cfg;
        HashRing newRing;
        HashRing oldRing; // empty unless cfg.migrating
    };
    explicit RingState(RingConfig cfg);
    std::shared_ptr<const Snapshot> snapshot() const;
    void set(RingConfig cfg);
    // Bumped by every set(). Lets a coordinator tell when every in-flight
    // operation that might have used the previous ring has finished.
    uint64_t version() const { return version_.load(); }

private:
    static std::shared_ptr<const Snapshot> build(RingConfig cfg);
    mutable std::mutex mu_;
    std::shared_ptr<const Snapshot> cur_;
    std::atomic<uint64_t> version_{0};
};

enum class Mode { Safe, Naive };

// Protocol steps that can be deliberately REMOVED, to show by experiment that
// each one is load-bearing (see scripts/rebalance_stress.sh). Never set these
// outside tests.
enum Ablation : unsigned {
    kNoRecheck = 1,      // GET on a moving key: skip the second read of B
    kNoTombstone = 2,    // DEL on a moving key: plain delete, no tombstone
    kOverwriteCopy = 4,  // move: copy with a plain SET instead of create-if-absent
};

enum class MoveOutcome { NothingToMove, Copied, DestinationHadNewer };

struct ClientOptions {
    Mode mode = Mode::Safe;
    int rpcTimeoutMs = 1000;
    unsigned ablate = 0;
    // Test hooks that WIDEN a race window that already exists naturally (a
    // millisecond or so), so it can be hit reliably instead of once in a
    // million runs. They never change what the protocol does, only when.
    int readGapMs = 0; // GET on a moving key: sleep between reading B and reading A
    int moveGapMs = 0; // moveKey: sleep between reading the source and writing the destination
};

// NOT thread-safe: it caches a KVClient (with sticky leader state) per shard.
// Give each thread its own ShardedClient; they can all share one RingState.
class ShardedClient {
public:
    explicit ShardedClient(std::shared_ptr<RingState> state, ClientOptions opts = {});

    // Same contract as raft::KVClient. On a rejected request (bad key, empty
    // or reserved value) ok is false and `value` holds the reason.
    raft::KVResult get(const std::string& key);
    raft::KVResult set(const std::string& key, const std::string& value);
    raft::KVResult del(const std::string& key);
    raft::KVResult cas(const std::string& key, const std::string& expected, const std::string& newValue);

    // Idempotent, race-safe move of one key from shard A to shard B (the
    // building block of both migration and CAS-on-a-moving-key).
    raft::KVResult moveKey(uint32_t from, uint32_t to, const std::string& key,
                           MoveOutcome* outcome = nullptr);

    uint32_t ownerOf(const std::string& key) const;
    raft::KVClient& shard(uint32_t id); // direct access to one shard's Raft group

private:
    std::shared_ptr<RingState> state_;
    ClientOptions opts_;
    std::map<uint32_t, std::unique_ptr<raft::KVClient>> clients_;

    raft::KVResult casOnOwner(uint32_t owner, const std::string& key, const std::string& expected,
                              const std::string& newValue);
};

} // namespace shard
