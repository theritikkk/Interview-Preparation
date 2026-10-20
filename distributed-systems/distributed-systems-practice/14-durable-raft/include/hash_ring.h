// hash_ring.h — a consistent-hash ring with virtual nodes (header-only).
//
// Maps string keys to shard ids so that adding or removing a shard moves only
// the keys that must move (about 1/(N+1) of them for N -> N+1 shards), unlike
// "hash(key) % N", which reshuffles almost everything. Each shard is placed
// on the ring at `vnodes` pseudo-random points; a key belongs to the shard
// owning the first point at or after hash(key), wrapping around.
//
// The ring is a PURE FUNCTION of (shard-id set, vnodes, salt): it is rebuilt
// from scratch on every add/remove, never mutated incrementally. That matters
// because every client must compute the identical ring independently from the
// same config — insertion order must never change an answer.

#pragma once

#include <algorithm>
#include <cstdint>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace shard {

// MurmurHash3's 64-bit finalizer: full avalanche, so near-identical inputs
// ("shard-1#17" vs "shard-1#18") land at unrelated ring positions. Plain
// FNV-1a alone has poor avalanche on short, similar strings and produces
// visibly clumped vnode positions; mixing its output fixes that.
inline uint64_t fmix64(uint64_t k) {
    k ^= k >> 33;
    k *= 0xff51afd7ed558ccdULL;
    k ^= k >> 33;
    k *= 0xc4ceb9fe1a85ec53ULL;
    k ^= k >> 33;
    return k;
}

inline uint64_t hashString(const std::string& s) {
    uint64_t h = 1469598103934665603ULL; // FNV-1a 64-bit offset basis
    for (unsigned char c : s) {
        h ^= c;
        h *= 1099511628211ULL; // FNV-1a 64-bit prime
    }
    return fmix64(h);
}

class HashRing {
public:
    // `salt` changes every vnode position; it exists so ring_stats can average
    // over many independent rings. Production use leaves it empty.
    explicit HashRing(int vnodes = 128, std::string salt = "")
        : vnodes_(vnodes), salt_(std::move(salt)) {}

    void addShard(uint32_t id) {
        ids_.insert(id);
        rebuild();
    }
    void removeShard(uint32_t id) {
        ids_.erase(id);
        rebuild();
    }

    bool empty() const { return points_.empty(); }
    std::vector<uint32_t> shards() const { return std::vector<uint32_t>(ids_.begin(), ids_.end()); }
    int vnodes() const { return vnodes_; }

    // Owner shard of `key`. Precondition: the ring is not empty.
    uint32_t owner(const std::string& key) const {
        uint64_t h = hashString(key);
        auto it = std::lower_bound(points_.begin(), points_.end(),
                                   std::make_pair(h, static_cast<uint32_t>(0)));
        if (it == points_.end()) it = points_.begin(); // wrap around
        return it->second;
    }

private:
    int vnodes_;
    std::string salt_;
    std::set<uint32_t> ids_;
    std::vector<std::pair<uint64_t, uint32_t>> points_; // sorted (position, shard id)

    void rebuild() {
        points_.clear();
        for (uint32_t id : ids_) {
            for (int i = 0; i < vnodes_; i++) {
                points_.emplace_back(
                    hashString(salt_ + "shard-" + std::to_string(id) + "#" + std::to_string(i)), id);
            }
        }
        std::sort(points_.begin(), points_.end());
        // Two vnodes landing on the exact same 64-bit position is astronomically
        // unlikely, but if it ever happens resolve it deterministically (lowest
        // shard id wins) so the ring stays a pure function of its inputs.
        points_.erase(std::unique(points_.begin(), points_.end(),
                                  [](const std::pair<uint64_t, uint32_t>& a,
                                     const std::pair<uint64_t, uint32_t>& b) {
                                      return a.first == b.first;
                                  }),
                      points_.end());
    }
};

} // namespace shard
