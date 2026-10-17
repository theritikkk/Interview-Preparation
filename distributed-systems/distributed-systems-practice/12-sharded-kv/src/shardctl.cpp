// shardctl — CLI for the sharded KV store. Routes each key to its shard via
// the consistent-hash ring in <ring.conf>, then behaves like kvctl.
//
// Usage: shardctl <ring.conf> <GET key | SET key value | DEL key |
//                              CAS key expected new | WHERE key>

#include "sharded_client.h"

#include <cstdio>

int main(int argc, char** argv) {
    if (argc < 4) {
        std::fprintf(stderr,
                     "usage: %s <ring.conf> <GET key | SET key value | DEL key | "
                     "CAS key expected new | WHERE key>\n",
                     argv[0]);
        return 1;
    }
    shard::RingConfig cfg;
    std::string err;
    if (!shard::loadRingConfig(argv[1], cfg, err)) {
        std::fprintf(stderr, "ring config error: %s\n", err.c_str());
        return 1;
    }
    auto state = std::make_shared<shard::RingState>(cfg);
    shard::ShardedClient client(state);

    std::string verb = argv[2];
    raft::KVResult res;

    if (verb == "WHERE" && argc == 4) {
        auto snap = state->snapshot();
        uint32_t b = snap->newRing.owner(argv[3]);
        if (cfg.migrating) {
            uint32_t a = snap->oldRing.owner(argv[3]);
            std::printf("key \"%s\" -> shard %u (old owner: shard %u%s)\n", argv[3], b, a,
                        a == b ? "" : ", MOVING");
        } else {
            std::printf("key \"%s\" -> shard %u\n", argv[3], b);
        }
        return 0;
    }

    if (verb == "GET" && argc == 4) {
        res = client.get(argv[3]);
        if (res.ok) std::printf("OK value=\"%s\" (shard %u)\n", res.value.c_str(), client.ownerOf(argv[3]));
    } else if (verb == "SET" && argc == 5) {
        res = client.set(argv[3], argv[4]);
        if (res.ok) std::printf("OK (shard %u)\n", client.ownerOf(argv[3]));
    } else if (verb == "DEL" && argc == 4) {
        res = client.del(argv[3]);
        if (res.ok) std::printf("OK (shard %u)\n", client.ownerOf(argv[3]));
    } else if (verb == "CAS" && argc == 6) {
        res = client.cas(argv[3], argv[4], argv[5]);
        if (res.ok) std::printf("%s (shard %u)\n", res.value.c_str(), client.ownerOf(argv[3]));
    } else {
        std::fprintf(stderr, "bad command or wrong number of arguments\n");
        return 1;
    }

    if (!res.ok) {
        if (res.value.rfind("REJECTED", 0) == 0)
            std::fprintf(stderr, "%s\n", res.value.c_str());
        else if (!res.reachedCluster)
            std::fprintf(stderr, "FAILED: no node of the owning shard was reachable\n");
        else
            std::fprintf(stderr, "FAILED: the owning shard has no leader / did not answer in time\n");
        return 1;
    }
    return 0;
}
