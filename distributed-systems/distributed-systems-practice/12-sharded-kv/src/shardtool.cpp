// shardtool — bulk load, inspection and measurement for the sharded KV store.
//
//   shardtool load    <ring.conf> <numKeys> <threads>   write key-0..key-(N-1) = val-<i>
//   shardtool verify  <ring.conf> <numKeys> <threads>   read every key back through the router
//   shardtool census  <ring.conf> <numKeys>             ask every shard directly which keys it holds
//   shardtool probe   <ring.conf> <seconds> <intervalMs> per-shard availability over time
//   shardtool rebalance <ring.conf> [--naive] [--threads T] [--step-delay-ms D] [--expect N]
//                                                        move every key whose owner changed (ring.conf must say "migrating")
//   shardtool stress  <ring.conf> [--naive] [--ablate a,b] [--keys K] [--workers W] ...
//                                                        concurrent traffic DURING a live rebalance, checked per key
//
// Exit status is non-zero whenever a check fails, so scripts can rely on it.

#include "sharded_client.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <mutex>
#include <memory>
#include <random>
#include <set>
#include <sstream>
#include <thread>
#include <vector>

using namespace shard;
using Clock = std::chrono::steady_clock;

namespace {

std::string keyName(int i) { return "key-" + std::to_string(i); }
std::string valName(int i) { return "val-" + std::to_string(i); }

std::shared_ptr<RingState> loadState(const char* path) {
    RingConfig cfg;
    std::string err;
    if (!loadRingConfig(path, cfg, err)) {
        std::fprintf(stderr, "ring config error: %s\n", err.c_str());
        std::exit(2);
    }
    return std::make_shared<RingState>(cfg);
}

double secondsSince(Clock::time_point t0) {
    return std::chrono::duration<double>(Clock::now() - t0).count();
}

// Run fn(i) for i in [0, n) across `threads` workers, each with its own client.
template <typename Fn>
void parallelFor(std::shared_ptr<RingState> state, int n, int threads, Fn fn) {
    std::atomic<int> next{0};
    std::vector<std::thread> ws;
    for (int t = 0; t < threads; t++) {
        ws.emplace_back([&] {
            ShardedClient client(state);
            for (;;) {
                int i = next.fetch_add(1);
                if (i >= n) break;
                fn(client, i);
            }
        });
    }
    for (auto& w : ws) w.join();
}

int cmdLoad(int argc, char** argv) {
    if (argc != 5) return 2;
    auto state = loadState(argv[2]);
    int n = std::atoi(argv[3]), threads = std::atoi(argv[4]);
    std::atomic<int> failed{0};
    auto t0 = Clock::now();
    parallelFor(state, n, threads, [&](ShardedClient& c, int i) {
        for (int attempt = 0; attempt < 4; attempt++)
            if (c.set(keyName(i), valName(i)).ok) return;
        failed++;
    });
    std::printf("load: wrote %d keys with %d threads in %.1fs, %d failed\n", n, threads,
                secondsSince(t0), failed.load());
    return failed.load() == 0 ? 0 : 1;
}

int cmdVerify(int argc, char** argv) {
    if (argc != 5) return 2;
    auto state = loadState(argv[2]);
    int n = std::atoi(argv[3]), threads = std::atoi(argv[4]);
    std::atomic<int> good{0}, missing{0}, wrong{0}, errors{0};
    auto t0 = Clock::now();
    parallelFor(state, n, threads, [&](ShardedClient& c, int i) {
        raft::KVResult r;
        for (int attempt = 0; attempt < 4; attempt++) {
            r = c.get(keyName(i));
            if (r.ok) break;
        }
        if (!r.ok) errors++;
        else if (r.value.empty()) missing++;
        else if (r.value != valName(i)) wrong++;
        else good++;
    });
    std::printf("verify: %d keys in %.1fs -> correct=%d missing=%d wrong-value=%d errors=%d\n", n,
                secondsSince(t0), good.load(), missing.load(), wrong.load(), errors.load());
    return (missing + wrong + errors) == 0 ? 0 : 1;
}

int cmdCensus(int argc, char** argv) {
    if (argc != 4) return 2;
    auto state = loadState(argv[2]);
    int n = std::atoi(argv[3]);
    auto snap = state->snapshot();

    std::set<uint32_t> ids(snap->cfg.shards.begin(), snap->cfg.shards.end());
    if (snap->cfg.migrating) ids.insert(snap->cfg.oldShards.begin(), snap->cfg.oldShards.end());

    ShardedClient client(state);
    std::map<uint32_t, std::set<std::string>> held;
    for (uint32_t id : ids) {
        raft::KVResult r;
        for (int attempt = 0; attempt < 4 && !r.ok; attempt++) r = client.shard(id).keys();
        if (!r.ok) {
            std::fprintf(stderr, "census: could not list keys of shard %u\n", id);
            return 1;
        }
        std::istringstream iss(r.value);
        std::string k;
        while (std::getline(iss, k)) held[id].insert(k);
    }

    // What the ring says each shard SHOULD hold, for the synthetic key set.
    std::map<uint32_t, int> expected;
    for (uint32_t id : ids) expected[id] = 0;
    for (int i = 0; i < n; i++) expected[snap->newRing.owner(keyName(i))]++;

    std::map<std::string, int> copies;
    for (auto& kv : held)
        for (auto& k : kv.second) copies[k]++;
    int onMultiple = 0;
    for (auto& kv : copies)
        if (kv.second > 1) onMultiple++;

    std::printf("%-7s %8s %10s %11s %9s\n", "shard", "holds", "ring says", "misplaced", "missing");
    int totalMisplaced = 0, totalMissing = 0;
    for (uint32_t id : ids) {
        int misplaced = 0;
        for (auto& k : held[id])
            if (snap->newRing.owner(k) != id) misplaced++;
        int missing = 0;
        for (int i = 0; i < n; i++) {
            std::string k = keyName(i);
            if (snap->newRing.owner(k) == id && !held[id].count(k)) missing++;
        }
        totalMisplaced += misplaced;
        totalMissing += missing;
        std::printf("%-7u %8zu %10d %11d %9d\n", id, held[id].size(), expected[id], misplaced, missing);
    }
    std::printf("distinct keys across all shards: %zu   keys held by more than one shard: %d\n",
                copies.size(), onMultiple);
    bool ok = totalMisplaced == 0 && totalMissing == 0 && onMultiple == 0;
    std::printf("census: %s\n", ok ? "PASS (every key on exactly its ring owner, none missing)" : "FAIL");
    return ok ? 0 : 1;
}

int cmdProbe(int argc, char** argv) {
    if (argc != 5) return 2;
    auto state = loadState(argv[2]);
    int seconds = std::atoi(argv[3]), intervalMs = std::atoi(argv[4]);
    auto snap = state->snapshot();

    struct Sample {
        double t;
        bool ok;
    };
    std::map<uint32_t, std::vector<Sample>> samples;
    std::map<uint32_t, std::string> probeKey;
    for (uint32_t id : snap->cfg.shards) {
        for (int i = 0; i < 100000; i++) {
            if (snap->newRing.owner(keyName(i)) == id) {
                probeKey[id] = keyName(i);
                break;
            }
        }
        samples[id];
    }

    auto t0 = Clock::now();
    std::vector<std::thread> ws;
    std::mutex mu;
    for (uint32_t id : snap->cfg.shards) {
        ws.emplace_back([&, id] {
            // Short timeout so a dead shard fails FAST and the outage window
            // measures the shard, not the client's patience.
            ClientOptions probeOpts;
            probeOpts.rpcTimeoutMs = 300;
            ShardedClient c(state, probeOpts);
            std::vector<Sample> local;
            while (secondsSince(t0) < seconds) {
                double t = secondsSince(t0);
                bool ok = c.get(probeKey[id]).ok;
                local.push_back({t, ok});
                std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));
            }
            std::lock_guard<std::mutex> g(mu);
            samples[id] = std::move(local);
        });
    }
    for (auto& w : ws) w.join();

    std::printf("probe: one GET every %dms per shard for %ds (rpc timeout 300ms)\n", intervalMs, seconds);
    std::printf("%-7s %9s %9s   %s\n", "shard", "attempts", "failures", "outage windows (first failure -> next success)");
    bool anyFailure = false;
    for (auto& kv : samples) {
        int fails = 0;
        std::string windows;
        bool inOutage = false;
        double start = 0;
        for (auto& s : kv.second) {
            if (!s.ok) {
                fails++;
                if (!inOutage) {
                    inOutage = true;
                    start = s.t;
                }
            } else if (inOutage) {
                char buf[96];
                std::snprintf(buf, sizeof buf, "[t=%.2fs -> t=%.2fs, %.2fs] ", start, s.t, s.t - start);
                windows += buf;
                inOutage = false;
            }
        }
        if (inOutage) {
            char buf[96];
            std::snprintf(buf, sizeof buf, "[t=%.2fs -> STILL DOWN at end] ", start);
            windows += buf;
        }
        if (fails) anyFailure = true;
        std::printf("%-7u %9zu %9d   %s\n", kv.first, kv.second.size(), fails,
                    windows.empty() ? "none" : windows.c_str());
    }
    return anyFailure ? 1 : 0; // informational: the calling script interprets it
}


// ------------------------------------------------------------ migration
struct MigrationStats {
    std::atomic<int> copied{0}, destHadNewer{0}, nothing{0}, failed{0}, tombstonesCleaned{0};
    int candidates = 0;
    double seconds = 0;
};

std::vector<std::string> splitKeys(const std::string& blob) {
    std::vector<std::string> out;
    std::istringstream iss(blob);
    std::string k;
    while (std::getline(iss, k))
        if (!k.empty()) out.push_back(k);
    return out;
}

// Moves every key whose owner differs between the old and new ring, then
// (unless `cleanup` is false) removes the tombstones clients left behind.
// The state's ring must be in the "migrating" configuration.
void runMigration(std::shared_ptr<RingState> state, ClientOptions opts, int threads, bool cleanup,
                  MigrationStats& st) {
    auto t0 = Clock::now();
    auto snap = state->snapshot();
    ShardedClient lister(state, opts);

    struct Item {
        uint32_t from, to;
        std::string key;
    };
    std::vector<Item> items;
    for (uint32_t from : snap->cfg.oldShards) {
        raft::KVResult r;
        for (int attempt = 0; attempt < 4 && !r.ok; attempt++) r = lister.shard(from).keys();
        if (!r.ok) {
            st.failed++;
            continue;
        }
        for (auto& k : splitKeys(r.value)) {
            uint32_t to = snap->newRing.owner(k);
            if (to != from) items.push_back({from, to, k});
        }
    }
    st.candidates = static_cast<int>(items.size());

    std::atomic<size_t> next{0};
    std::vector<std::thread> ws;
    for (int t = 0; t < threads; t++) {
        ws.emplace_back([&] {
            ShardedClient c(state, opts);
            for (;;) {
                size_t i = next.fetch_add(1);
                if (i >= items.size()) break;
                bool done = false;
                for (int attempt = 0; attempt < 4 && !done; attempt++) {
                    MoveOutcome oc = MoveOutcome::NothingToMove;
                    if (c.moveKey(items[i].from, items[i].to, items[i].key, &oc).ok) {
                        done = true;
                        if (oc == MoveOutcome::Copied) st.copied++;
                        else if (oc == MoveOutcome::DestinationHadNewer) st.destHadNewer++;
                        else st.nothing++;
                    }
                }
                if (!done) st.failed++;
            }
        });
    }
    for (auto& w : ws) w.join();

    if (cleanup) {
        struct Tomb {
            uint32_t shard;
            std::string key;
        };
        std::vector<Tomb> tombs;
        for (uint32_t b : snap->cfg.shards) {
            raft::KVResult r;
            for (int attempt = 0; attempt < 4 && !r.ok; attempt++) r = lister.shard(b).keys();
            if (!r.ok) {
                st.failed++;
                continue;
            }
            // Only keys that moved INTO this shard can carry a tombstone.
            for (auto& k : splitKeys(r.value))
                if (snap->oldRing.owner(k) != b) tombs.push_back({b, k});
        }
        std::atomic<size_t> tn{0};
        std::vector<std::thread> cs;
        for (int t = 0; t < threads; t++) {
            cs.emplace_back([&] {
                ShardedClient c(state, opts);
                for (;;) {
                    size_t i = tn.fetch_add(1);
                    if (i >= tombs.size()) break;
                    // CAD deletes only if the value is STILL the tombstone, so a
                    // client that has meanwhile written a real value is untouched.
                    raft::KVResult r = c.shard(tombs[i].shard).cad(tombs[i].key, kTombstone);
                    if (r.ok && r.value == "OK") st.tombstonesCleaned++;
                }
            });
        }
        for (auto& c : cs) c.join();
    }
    st.seconds = secondsSince(t0);
}

bool hasFlag(int argc, char** argv, const char* flag) {
    for (int i = 0; i < argc; i++)
        if (std::string(argv[i]) == flag) return true;
    return false;
}
int intFlag(int argc, char** argv, const char* flag, int dflt) {
    for (int i = 0; i + 1 < argc; i++)
        if (std::string(argv[i]) == flag) return std::atoi(argv[i + 1]);
    return dflt;
}
std::string strFlag(int argc, char** argv, const char* flag, const std::string& dflt) {
    for (int i = 0; i + 1 < argc; i++)
        if (std::string(argv[i]) == flag) return argv[i + 1];
    return dflt;
}

void printMigration(const MigrationStats& st) {
    std::printf("migration: candidates=%d copied=%d destination-had-newer=%d nothing-to-move=%d failed=%d "
                "tombstones-cleaned=%d  (%.1fs)\n",
                st.candidates, st.copied.load(), st.destHadNewer.load(), st.nothing.load(), st.failed.load(),
                st.tombstonesCleaned.load(), st.seconds);
}

int cmdRebalance(int argc, char** argv) {
    if (argc < 3) return 2;
    auto state = loadState(argv[2]);
    auto snap = state->snapshot();
    if (!snap->cfg.migrating) {
        std::fprintf(stderr, "rebalance: ring config must say 'state migrating' (with old_shards)\n");
        return 1;
    }
    ClientOptions opts;
    opts.mode = hasFlag(argc, argv, "--naive") ? Mode::Naive : Mode::Safe;
    opts.moveGapMs = intFlag(argc, argv, "--step-delay-ms", 0);
    int threads = intFlag(argc, argv, "--threads", 16);
    int expectN = intFlag(argc, argv, "--expect", 0);

    MigrationStats st;
    runMigration(state, opts, threads, opts.mode == Mode::Safe, st);
    printMigration(st);

    bool ok = st.failed.load() == 0;
    if (expectN > 0) {
        int predicted = 0;
        for (int i = 0; i < expectN; i++)
            if (snap->oldRing.owner(keyName(i)) != snap->newRing.owner(keyName(i))) predicted++;
        bool match = (predicted == st.copied.load());
        std::printf("ring prediction for key-0..key-%d: %d keys change owner (%.2f%%); migrator copied %d -> %s\n",
                    expectN - 1, predicted, 100.0 * predicted / expectN, st.copied.load(),
                    match ? "MATCH" : "MISMATCH");
        // For contrast: the same key set under hash(key) % N, old N -> new N.
        int modMoved = 0;
        const uint64_t oldN = snap->cfg.oldShards.size(), newN = snap->cfg.shards.size();
        for (int i = 0; i < expectN; i++) {
            uint64_t h = hashString(keyName(i));
            if (h % oldN != h % newN) modMoved++;
        }
        std::printf("for contrast, hash(key) %% N (%llu -> %llu shards) would have moved %d keys (%.2f%%)\n",
                    (unsigned long long)oldN, (unsigned long long)newN, modMoved, 100.0 * modMoved / expectN);
        ok = ok && match;
    }
    return ok ? 0 : 1;
}

// ------------------------------------------------------------- stress
int cmdStress(int argc, char** argv) {
    if (argc < 3) return 2;
    const int K = intFlag(argc, argv, "--keys", 100);
    const int W = intFlag(argc, argv, "--workers", 16);
    const int M = intFlag(argc, argv, "--migrators", 4);
    const int seed = intFlag(argc, argv, "--seed", 1);
    const int warmupMs = intFlag(argc, argv, "--warmup-ms", 1500);
    const int settleMs = intFlag(argc, argv, "--settle-ms", 3000);

    ClientOptions copts;
    copts.mode = hasFlag(argc, argv, "--naive") ? Mode::Naive : Mode::Safe;
    copts.readGapMs = intFlag(argc, argv, "--read-gap-ms", 0);
    copts.moveGapMs = intFlag(argc, argv, "--step-delay-ms", 0);
    std::string ablateStr = strFlag(argc, argv, "--ablate", "");
    if (ablateStr.find("norecheck") != std::string::npos) copts.ablate |= kNoRecheck;
    if (ablateStr.find("notombstone") != std::string::npos) copts.ablate |= kNoTombstone;
    if (ablateStr.find("overwrite") != std::string::npos) copts.ablate |= kOverwriteCopy;

    RingConfig base;
    std::string err;
    if (!loadRingConfig(argv[2], base, err) || base.migrating) {
        std::fprintf(stderr, "stress: ring.conf must be a valid STABLE config (%s)\n", err.c_str());
        return 2;
    }
    const uint32_t newId = *std::max_element(base.shards.begin(), base.shards.end()) + 1;
    RingConfig stableOld = base;
    RingConfig migrating = base;
    migrating.migrating = true;
    migrating.oldShards = base.shards;
    migrating.shards.push_back(newId);
    RingConfig stableNew = base;
    stableNew.shards.push_back(newId);

    auto state = std::make_shared<RingState>(stableOld);

    std::vector<std::string> names(K);
    for (int i = 0; i < K; i++) names[i] = "sk-" + std::to_string(i);

    // ---- reset: a stress run must start from an empty keyspace on EVERY shard.
    // (Harness bug H4: shard 4 kept the previous run's final values, the
    // "new, empty" shard was not empty, and every run after the first reported
    // bogus violations — leftovers from run N-1 look exactly like stale data.)
    {
        ShardedClient admin(state, copts);
        std::vector<uint32_t> every = base.shards;
        every.push_back(newId);
        long removed = 0;
        for (uint32_t sid : every) {
            raft::KVResult ks = admin.shard(sid).keys();
            if (!ks.ok) {
                std::fprintf(stderr, "stress: cannot list keys of shard %u to reset it\n", sid);
                return 2;
            }
            std::istringstream is(ks.value);
            std::string k;
            while (std::getline(is, k))
                if (k.rfind("sk-", 0) == 0) {
                    if (!admin.shard(sid).del(k).ok) {
                        std::fprintf(stderr, "stress: cannot delete leftover %s on shard %u\n", k.c_str(), sid);
                        return 2;
                    }
                    removed++;
                }
        }
        for (uint32_t sid : every) {
            raft::KVResult ks = admin.shard(sid).keys();
            std::istringstream is(ks.value);
            std::string k;
            while (std::getline(is, k))
                if (k.rfind("sk-", 0) == 0) {
                    std::fprintf(stderr, "stress: shard %u still holds %s after reset — refusing to run\n", sid, k.c_str());
                    return 2;
                }
        }
        if (removed) std::printf("reset: removed %ld leftover sk-* keys from earlier runs\n", removed);
    }

    // ---- preload (unchecked bulk write through the old ring)
    std::atomic<int> preloadFail{0};
    parallelFor(state, K, 24, [&](ShardedClient& c, int i) {
        for (int attempt = 0; attempt < 4; attempt++)
            if (c.set(names[i], "init-" + std::to_string(i)).ok) return;
        preloadFail++;
    });
    if (preloadFail) {
        std::fprintf(stderr, "stress: preload failed for %d keys\n", preloadFail.load());
        return 2;
    }

    // ---- per-key truth, owned by exactly one worker (so the model is exact)
    struct KeyTruth {
        std::string expected;
        bool certain = true;
    };
    std::vector<KeyTruth> truth(K);
    for (int i = 0; i < K; i++) truth[i].expected = "init-" + std::to_string(i);

    struct WorkerCtl {
        std::atomic<bool> busy{false};
        std::atomic<uint64_t> opVersion{0};
    };
    std::vector<std::unique_ptr<WorkerCtl>> ctl;
    for (int w = 0; w < W; w++) ctl.push_back(std::make_unique<WorkerCtl>());

    std::atomic<bool> stop{false};
    std::atomic<long> ops{0}, reads{0}, writes{0}, failedOps{0}, checkedReads{0}, casChecked{0};
    std::atomic<long> vanished{0}, stale{0}, resurrected{0}, casWrong{0};
    // Forensics: which phase each violation happened in, plus a bounded log of
    // the first few with the raw per-shard values at the time we noticed.
    // 0 = warmup (old ring), 1 = migrating, 2 = settle (new ring)
    std::atomic<int> phase{0};
    std::atomic<long> violationsInPhase[3] = {{0}, {0}, {0}};
    std::mutex vlogMu;
    std::vector<std::string> vlog;
    auto logViolation = [&](const char* kind, const std::string& key, const std::string& op,
                            const std::string& expected, const std::string& got, int ph,
                            ShardedClient& c) {
        violationsInPhase[ph]++;
        std::lock_guard<std::mutex> g(vlogMu);
        if (vlog.size() >= 12) return;
        // Raw per-shard values read AFTER the violation (so racy, but they show
        // where the data physically was a moment later).
        auto sn = state->snapshot();
        std::ostringstream raw;
        std::set<uint32_t> all(sn->cfg.shards.begin(), sn->cfg.shards.end());
        for (uint32_t os : sn->cfg.oldShards) all.insert(os);
        for (uint32_t sid : base.shards) all.insert(sid);
        all.insert(newId);
        raw << "raw[";
        for (uint32_t sid : all) {
            raft::KVResult rr = c.shard(sid).get(key);
            if (rr.ok && !rr.value.empty()) raw << " s" << sid << "=\"" << rr.value << "\"";
        }
        raw << " ] newOwner=s" << sn->newRing.owner(key);
        if (sn->cfg.migrating) raw << " oldOwner=s" << sn->oldRing.owner(key);
        std::string snap = raw.str();
        std::ostringstream o;
        o << kind << " key=" << key << " during=" << op << " phase=" << (ph == 0 ? "warmup" : ph == 1 ? "migrating" : "settle")
          << " expected=\"" << expected << "\" got=\"" << got << "\" " << snap;
        vlog.push_back(o.str());
    };

    std::vector<std::thread> workers;
    for (int w = 0; w < W; w++) {
        workers.emplace_back([&, w] {
            ShardedClient c(state, copts);
            std::mt19937 rng(static_cast<unsigned>(seed * 1000 + w));
            std::vector<int> mine;
            for (int i = w; i < K; i += W) mine.push_back(i);
            if (mine.empty()) return;
            long counter = 0;
            while (!stop.load()) {
                int i = mine[rng() % mine.size()];
                const std::string& key = names[i];
                KeyTruth& t = truth[i];
                int dice = static_cast<int>(rng() % 100);

                ctl[w]->busy = true;
                ctl[w]->opVersion = state->version();
                const int phaseAtStart = phase.load();
                ops++;
                if (dice < 40) { // GET
                    reads++;
                    raft::KVResult r = c.get(key);
                    if (!r.ok) {
                        failedOps++;
                    } else if (t.certain) {
                        checkedReads++;
                        if (r.value != t.expected) {
                            const char* kind;
                            if (r.value.empty()) { vanished++; kind = "VANISHED"; }                // key exists, read says absent
                            else if (t.expected.empty()) { resurrected++; kind = "RESURRECTED"; }  // key was deleted, read returns data
                            else { stale++; kind = "STALE"; }                                      // some OTHER (older) value
                            logViolation(kind, key, "GET", t.expected, r.value, phaseAtStart, c);
                        }
                    }
                } else if (dice < 70 || !t.certain) { // SET
                    writes++;
                    std::string v = "v" + std::to_string(w) + "-" + std::to_string(++counter);
                    raft::KVResult r = c.set(key, v);
                    if (r.ok) {
                        t.expected = v;
                        t.certain = true;
                    } else {
                        failedOps++;
                        t.certain = false; // outcome unknown: may or may not have applied
                    }
                } else if (dice < 85) { // DEL
                    writes++;
                    raft::KVResult r = c.del(key);
                    if (r.ok) {
                        t.expected.clear();
                        t.certain = true;
                    } else {
                        failedOps++;
                        t.certain = false;
                    }
                } else { // CAS — half the time with a deliberately wrong expected value
                    writes++;
                    bool wrong = (rng() % 100) < 40;
                    std::string e = wrong ? "wrong-" + std::to_string(counter) : t.expected;
                    std::string v = "c" + std::to_string(w) + "-" + std::to_string(++counter);
                    bool predictedMatch = (e == t.expected);
                    raft::KVResult r = c.cas(key, e, v);
                    if (!r.ok) {
                        failedOps++;
                        t.certain = false;
                    } else {
                        casChecked++;
                        const std::string want = predictedMatch ? "OK" : "MISMATCH";
                        if (r.value != want) {
                            casWrong++;
                            logViolation("CAS-WRONG", key, "CAS(expected=\"" + e + "\")", t.expected,
                                         "returned " + r.value + " (wanted " + want + ")", phaseAtStart, c);
                            t.certain = false; // can't trust our model of this key any more
                        } else if (predictedMatch) {
                            t.expected = v;
                        }
                    }
                }
                ctl[w]->busy = false;
            }
        });
    }

    auto sleepFor = [](int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); };

    std::printf("stress: mode=%s ablate=%s keys=%d workers=%d migrators=%d read-gap=%dms step-delay=%dms seed=%d\n",
                copts.mode == Mode::Naive ? "NAIVE" : "safe", ablateStr.empty() ? "none" : ablateStr.c_str(), K, W, M,
                copts.readGapMs, copts.moveGapMs, seed);

    sleepFor(warmupMs); // checked traffic against the plain 3-shard ring first

    phase = 1;
    state->set(migrating);
    const uint64_t flipped = state->version();
    // Drain: wait until no worker is still inside an operation that began under
    // the OLD ring. (In a real deployment this is what "every client has
    // acknowledged the new shard map" means — see ANALYSIS.md on fencing.)
    for (int w = 0; w < W; w++) {
        auto deadline = Clock::now() + std::chrono::seconds(30);
        while (ctl[w]->busy.load() && ctl[w]->opVersion.load() < flipped && Clock::now() < deadline) sleepFor(2);
    }

    MigrationStats st;
    runMigration(state, copts, M, copts.mode == Mode::Safe, st);
    state->set(stableNew);
    phase = 2;
    sleepFor(settleMs); // traffic continues against the final 4-shard ring

    stop = true;
    for (auto& t : workers) t.join();

    // ---- final audit, quiescent. Logical state through the final ring:
    ShardedClient audit(state);
    long finalMismatch = 0, uncertain = 0, strayOld = 0, tombLeft = 0, auditErr = 0;
    auto finalSnap = state->snapshot();
    for (int i = 0; i < K; i++) {
        if (!truth[i].certain) {
            uncertain++;
        } else {
            raft::KVResult r;
            for (int attempt = 0; attempt < 4 && !r.ok; attempt++) r = audit.get(names[i]);
            if (!r.ok) auditErr++;
            else if (r.value != truth[i].expected) finalMismatch++;
        }
        // Raw placement: nothing should be left on a shard that no longer owns the key.
        uint32_t owner = finalSnap->newRing.owner(names[i]);
        for (uint32_t s : base.shards) {
            if (s == owner) continue;
            raft::KVResult r = audit.shard(s).get(names[i]);
            if (r.ok && !r.value.empty()) strayOld++;
        }
        raft::KVResult r = audit.shard(owner).get(names[i]);
        if (r.ok && r.value == kTombstone) tombLeft++;
    }

    printMigration(st);
    for (const auto& l : vlog) std::printf("  violation: %s\n", l.c_str());
    std::printf("violations by phase: warmup=%ld migrating=%ld settle=%ld\n", violationsInPhase[0].load(),
                violationsInPhase[1].load(), violationsInPhase[2].load());
    std::printf("traffic: ops=%ld (reads=%ld writes=%ld) failed-ops=%ld uncertain-keys=%ld  checked-reads=%ld checked-cas=%ld\n",
                ops.load(), reads.load(), writes.load(), failedOps.load(), uncertain, checkedReads.load(),
                casChecked.load());
    std::printf("VIOLATIONS: vanished-reads=%ld stale-reads=%ld resurrected-reads=%ld cas-wrong=%ld "
                "final-state-mismatch=%ld stray-copies-on-old-shards=%ld   (audit-errors=%ld)\n",
                vanished.load(), stale.load(), resurrected.load(), casWrong.load(), finalMismatch, strayOld, auditErr);
    std::printf("informational: tombstones-left-behind=%ld\n", tombLeft);
    long total = vanished + stale + resurrected + casWrong + finalMismatch + strayOld + auditErr;
    std::printf("RESULT: %s (%ld violations)\n", total == 0 ? "CLEAN" : "VIOLATIONS DETECTED", total);
    return total == 0 ? 0 : 1;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr, "usage: %s <load|verify|census|probe> <ring.conf> ...\n", argv[0]);
        return 2;
    }
    std::string cmd = argv[1];
    int rc = 2;
    if (cmd == "load") rc = cmdLoad(argc, argv);
    else if (cmd == "verify") rc = cmdVerify(argc, argv);
    else if (cmd == "census") rc = cmdCensus(argc, argv);
    else if (cmd == "probe") rc = cmdProbe(argc, argv);
    else if (cmd == "rebalance") rc = cmdRebalance(argc, argv);
    else if (cmd == "stress") rc = cmdStress(argc, argv);
    if (rc == 2) std::fprintf(stderr, "bad usage; see the header of src/shardtool.cpp\n");
    return rc;
}
