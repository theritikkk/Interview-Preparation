// ring_stats — measures the two properties that justify a consistent-hash
// ring, instead of just asserting them:
//
//   1. BALANCE: how evenly keys spread across shards, as a function of the
//      number of virtual nodes per shard.
//   2. MINIMAL MOVEMENT: what fraction of keys change owner when a shard is
//      added, versus the naive `hash(key) % N`.
//
// Every figure is averaged over many independent rings (different vnode
// salts) so a single lucky or unlucky ring doesn't masquerade as the trend.

#include "hash_ring.h"

#include <cmath>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

using shard::HashRing;
using shard::hashString;

namespace {

constexpr int kKeys = 100000;
constexpr int kRings = 20; // independent rings averaged per data point

std::vector<std::string> makeKeys() {
    std::vector<std::string> keys;
    keys.reserve(kKeys);
    for (int i = 0; i < kKeys; i++) keys.push_back("key-" + std::to_string(i));
    return keys;
}

struct Balance {
    double maxOverMean = 0; // heaviest shard / ideal share
    double minOverMean = 0; // lightest shard / ideal share
    double cv = 0;          // stddev / mean of per-shard load
};

Balance balanceFor(int nShards, int vnodes, const std::vector<std::string>& keys) {
    Balance acc;
    for (int r = 0; r < kRings; r++) {
        HashRing ring(vnodes, "salt" + std::to_string(r) + ":");
        for (int s = 1; s <= nShards; s++) ring.addShard(static_cast<uint32_t>(s));
        std::map<uint32_t, int> counts;
        for (int s = 1; s <= nShards; s++) counts[static_cast<uint32_t>(s)] = 0;
        for (auto& k : keys) counts[ring.owner(k)]++;
        double mean = static_cast<double>(keys.size()) / nShards;
        double mx = 0, mn = 1e18, var = 0;
        for (auto& kv : counts) {
            double c = kv.second;
            mx = std::max(mx, c);
            mn = std::min(mn, c);
            var += (c - mean) * (c - mean);
        }
        var /= nShards;
        acc.maxOverMean += mx / mean;
        acc.minOverMean += mn / mean;
        acc.cv += std::sqrt(var) / mean;
    }
    acc.maxOverMean /= kRings;
    acc.minOverMean /= kRings;
    acc.cv /= kRings;
    return acc;
}

struct Movement {
    double ringMoved = 0;        // fraction of keys changing owner, consistent hashing
    double ringMovedElsewhere = 0; // of those, fraction NOT moving to the new shard (should be 0)
    double modMoved = 0;         // fraction changing owner under hash % N
    bool removalIsExactReverse = true;
};

Movement movementFor(int nBefore, int vnodes, const std::vector<std::string>& keys) {
    Movement m;
    for (int r = 0; r < kRings; r++) {
        std::string salt = "salt" + std::to_string(r) + ":";
        HashRing before(vnodes, salt), after(vnodes, salt);
        for (int s = 1; s <= nBefore; s++) before.addShard(static_cast<uint32_t>(s));
        for (int s = 1; s <= nBefore + 1; s++) after.addShard(static_cast<uint32_t>(s));
        const uint32_t newShard = static_cast<uint32_t>(nBefore + 1);

        // Removing the new shard from `after` must reproduce `before` exactly
        // (the ring is a pure function of the shard set).
        HashRing removed = after;
        removed.removeShard(newShard);

        long ringMoved = 0, movedElsewhere = 0, modMoved = 0;
        for (auto& k : keys) {
            uint32_t a = before.owner(k), b = after.owner(k);
            if (a != b) {
                ringMoved++;
                if (b != newShard) movedElsewhere++;
            }
            if (removed.owner(k) != a) m.removalIsExactReverse = false;

            uint64_t h = hashString(k);
            if (h % static_cast<uint64_t>(nBefore) != h % static_cast<uint64_t>(nBefore + 1)) modMoved++;
        }
        m.ringMoved += static_cast<double>(ringMoved) / keys.size();
        m.ringMovedElsewhere += ringMoved ? static_cast<double>(movedElsewhere) / ringMoved : 0;
        m.modMoved += static_cast<double>(modMoved) / keys.size();
    }
    m.ringMoved /= kRings;
    m.ringMovedElsewhere /= kRings;
    m.modMoved /= kRings;
    return m;
}

} // namespace

int main() {
    auto keys = makeKeys();
    std::printf("keys per measurement: %d    independent rings averaged per row: %d\n\n", kKeys, kRings);

    std::printf("=== 1. BALANCE vs virtual nodes per shard ===\n");
    std::printf("max/mean = heaviest shard's load / ideal share (1.00 = perfect)\n");
    std::printf("min/mean = lightest shard's load / ideal share\n");
    std::printf("cv       = stddev/mean of per-shard load;  1/sqrt(V) is shown only as a rough reference\n\n");
    const int vnodeList[] = {1, 4, 16, 64, 128, 256, 1024};
    for (int n : {3, 10}) {
        std::printf("-- %d shards --\n", n);
        std::printf("%8s %10s %10s %10s %12s\n", "vnodes", "max/mean", "min/mean", "cv", "1/sqrt(V)");
        for (int v : vnodeList) {
            Balance b = balanceFor(n, v, keys);
            std::printf("%8d %10.3f %10.3f %10.4f %12.4f\n", v, b.maxOverMean, b.minOverMean, b.cv,
                        1.0 / std::sqrt(static_cast<double>(v)));
        }
        std::printf("\n");
    }

    std::printf("=== 2. KEY MOVEMENT when adding one shard (vnodes=128) ===\n");
    std::printf("moved%%      = share of keys whose owner changed\n");
    std::printf("elsewhere%%  = of the moved keys, share that moved to a shard OTHER than the new one\n");
    std::printf("               (consistent hashing should never shuffle keys between existing shards)\n");
    std::printf("ideal%%      = 1/(N+1), the minimum that must move to give the new shard its fair share\n\n");
    std::printf("%10s %10s %12s %12s %10s %14s\n", "N -> N+1", "ring moved%", "elsewhere%", "mod-N moved%", "ideal%",
                "remove=undo");
    for (int n : {2, 3, 5, 9, 19}) {
        Movement m = movementFor(n, 128, keys);
        char label[32];
        std::snprintf(label, sizeof label, "%d -> %d", n, n + 1);
        std::printf("%10s %10.2f %12.2f %12.2f %10.2f %14s\n", label, 100 * m.ringMoved,
                    100 * m.ringMovedElsewhere, 100 * m.modMoved, 100.0 / (n + 1),
                    m.removalIsExactReverse ? "exact" : "MISMATCH");
    }
    return 0;
}
