// txntool — bank-transfer workloads that check the transactional KV store.
//
//   txntool init     <ring.conf> <accounts> <initialBalance>
//   txntool balances <ring.conf> <accounts>
//   txntool transfer <ring.conf> <fromIdx> <toIdx> <amount> [client options]
//   txntool bank     <ring.conf> [--accounts A=12] [--groups G=4] [--workers W=8] [--seconds S=20] [--initial I=1000]
//                    [--max-amount M=50] [--audit-ms 100] [--seed N=1] [--attempts K=8] [--lock-ttl-ms T=3000]
//                    [--naive] [--plain-audit]
//
// `bank` is the main experiment. A fixed pool of accounts is spread over the
// shards by the hash ring. Worker threads repeatedly move random amounts
// between two accounts of the same GROUP (account i is in group i % G); one
// auditor thread per group repeatedly reads that group's accounts and checks
// that the group's total is unchanged. Money can only move, never appear or
// disappear, so any deviation is a transaction-isolation or atomicity bug.
// (Groups keep each audit's read set small: validating ALL accounts at once
// starves under no-wait locking, because some worker is nearly always
// mid-commit — measured and discussed in ANALYSIS.md.)
//
// Safe mode: each transfer is a transaction whose compares are the balances it
// read (so it aborts if either account changed in the meantime) and whose
// writes are the two new balances; it is a 2PC across shards whenever the two
// accounts live on different shards. The auditor takes a validated snapshot.
//
// --naive        no transactions: read both balances, then two plain SETs.
// --plain-audit  the auditor reads the accounts one by one with no validation.
// (Server-side sabotage is chosen when the cluster starts: RAFT_TXN_ABLATE=nolock.)
//
// Every check is exact. Each worker records the transfers that committed; after
// the run any transaction left in doubt (timeout, killed shard) is settled
// through the decision record, and the final balance of every account must equal
// its initial balance plus the net of the committed transfers. Exit status 0
// iff there were no violations.

#include "txn_client.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <random>
#include <thread>

using namespace txn;
using Clock = std::chrono::steady_clock;

namespace {

std::string acct(int i) { return "acct-" + std::to_string(i); }

void sleepMs(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }

bool parseLong(const std::string& s, long& out) {
    if (s.empty()) return false;
    char* end = nullptr;
    out = std::strtol(s.c_str(), &end, 10);
    return end && *end == '\0';
}

std::shared_ptr<shard::RingState> loadState(const std::string& path) {
    shard::RingConfig cfg;
    std::string err;
    if (!shard::loadRingConfig(path, cfg, err)) {
        std::fprintf(stderr, "ring config error: %s\n", err.c_str());
        std::exit(1);
    }
    return std::make_shared<shard::RingState>(cfg);
}

long pct(std::vector<int> v, int p) {
    if (v.empty()) return -1;
    std::sort(v.begin(), v.end());
    return v[std::min(v.size() - 1, v.size() * p / 100)];
}

struct Transfer {
    int from, to;
    long amount;
};
struct InDoubt {
    std::string txid;
    Txn txn;
    bool crossShard;
    Transfer tr;
};
struct Stats {
    long attempted = 0, gaveUp = 0, committed = 0, abortedMismatch = 0, abortedConflict = 0, abortedOther = 0, insufficient = 0,
         readFail = 0, crossCommitted = 0, singleCommitted = 0;
    std::vector<Transfer> done;
    std::vector<InDoubt> doubt;
    std::vector<int> latCross, latSingle;
};

// ------------------------------------------------------------------- init
int cmdInit(int argc, char** argv) {
    if (argc != 5) return 1;
    auto state = loadState(argv[2]);
    int n = std::atoi(argv[3]);
    long initial = std::atol(argv[4]);
    {
        TxnClient c(state);
        int resolved = c.sweep(0); // start from a clean slate: finish anything left prepared by an earlier run
        if (resolved) std::printf("init: resolved %d leftover prepared transaction(s)\n", resolved);
    }
    std::atomic<int> failed{0};
    std::vector<std::thread> ths;
    for (int i = 0; i < n; i++)
        ths.emplace_back([&, i] {
            TxnClient c(state);
            for (int a = 0; a < 4; a++)
                if (c.set(acct(i), std::to_string(initial)).ok) return;
            failed++;
        });
    for (auto& t : ths) t.join();
    std::printf("init: %d accounts x %ld = %ld total, %d failed\n", n, initial, n * initial, failed.load());
    return failed ? 1 : 0;
}

int cmdBalances(int argc, char** argv) {
    if (argc != 4) return 1;
    auto state = loadState(argv[2]);
    int n = std::atoi(argv[3]);
    TxnClient c(state);
    long sum = 0;
    for (int i = 0; i < n; i++) {
        auto r = c.get(acct(i));
        long v = 0;
        bool ok = r.ok && parseLong(r.value, v);
        std::printf("%-8s shard %u  %s\n", acct(i).c_str(), c.ownerOf(acct(i)), ok ? r.value.c_str() : "(unreadable)");
        if (ok) sum += v;
    }
    std::printf("total %ld\n", sum);
    return 0;
}

// --------------------------------------------------------------- transfer
int cmdTransfer(int argc, char** argv) {
    if (argc < 6) return 1;
    auto state = loadState(argv[2]);
    int from = std::atoi(argv[3]), to = std::atoi(argv[4]);
    long amt = std::atol(argv[5]);
    Options opts;
    for (int i = 6; i < argc; i++) {
        std::string a = argv[i];
        auto next = [&]() -> std::string { return (i + 1 < argc) ? argv[++i] : ""; };
        if (a == "--crash-at") {
            std::string v = next();
            opts.crashAt = v == "after-begin" ? CrashPoint::AfterBegin : v == "after-prepare" ? CrashPoint::AfterPrepare
                         : v == "after-decide" ? CrashPoint::AfterDecide : v == "mid-commit" ? CrashPoint::MidCommit
                         : CrashPoint::None;
        } else if (a == "--pause-after-prepare-ms") {
            opts.pauseAfterPrepareMs = std::atoi(next().c_str());
        } else if (a == "--lock-ttl-ms") {
            opts.lockTtlMs = std::atoi(next().c_str());
        }
    }
    TxnClient c(state, opts);
    auto rf = c.get(acct(from)), rt = c.get(acct(to));
    long vf = 0, vt = 0;
    if (!rf.ok || !rt.ok || !parseLong(rf.value, vf) || !parseLong(rt.value, vt)) {
        std::fprintf(stderr, "cannot read both accounts\n");
        return 1;
    }
    Txn t;
    t.expect(acct(from), rf.value).expect(acct(to), rt.value).put(acct(from), std::to_string(vf - amt)).put(acct(to), std::to_string(vt + amt));
    Result r = c.commit(t);
    const char* name = r.outcome == Outcome::Committed ? "COMMITTED" : r.outcome == Outcome::Aborted ? "ABORTED" : "INDETERMINATE";
    std::printf("transfer %ld from %s (shard %u, was %ld) to %s (shard %u, was %ld): %s  txid=%s  %s  %dms\n", amt,
                acct(from).c_str(), c.ownerOf(acct(from)), vf, acct(to).c_str(), c.ownerOf(acct(to)), vt, name,
                r.txid.c_str(), r.crossShard ? "2PC" : "single-shard", r.latencyMs);
    return r.outcome == Outcome::Committed ? 0 : r.outcome == Outcome::Aborted ? 3 : 4;
}

// ------------------------------------------------------------------- bank
int cmdBank(int argc, char** argv) {
    if (argc < 3) return 1;
    auto state = loadState(argv[2]);
    int A = 12, G = 4, W = 8, S = 20, auditMs = 100, seed = 1, attempts = 8, ttl = 3000;
    long initial = 1000, maxAmount = 50;
    bool naive = false, plainAudit = false;
    for (int i = 3; i < argc; i++) {
        std::string a = argv[i];
        auto next = [&]() -> std::string { return (i + 1 < argc) ? argv[++i] : ""; };
        if (a == "--accounts") A = std::atoi(next().c_str());
        else if (a == "--groups") G = std::atoi(next().c_str());
        else if (a == "--workers") W = std::atoi(next().c_str());
        else if (a == "--seconds") S = std::atoi(next().c_str());
        else if (a == "--initial") initial = std::atol(next().c_str());
        else if (a == "--max-amount") maxAmount = std::atol(next().c_str());
        else if (a == "--audit-ms") auditMs = std::atoi(next().c_str());
        else if (a == "--seed") seed = std::atoi(next().c_str());
        else if (a == "--attempts") attempts = std::atoi(next().c_str());
        else if (a == "--lock-ttl-ms") ttl = std::atoi(next().c_str());
        else if (a == "--naive") naive = true;
        else if (a == "--plain-audit") plainAudit = true;
        else { std::fprintf(stderr, "unknown option %s\n", a.c_str()); return 2; }
    }
    if (G < 1 || G > A) G = 1;
    const long total = A * initial;
    std::vector<std::vector<int>> members(G);
    for (int i = 0; i < A; i++) members[i % G].push_back(i);
    for (auto& m : members) if (m.size() < 2) { std::fprintf(stderr, "bank: every group needs at least 2 accounts\n"); return 2; }
    Options opts;
    opts.lockTtlMs = ttl;

    // ---- reset: clean slate on every shard, then fund the accounts
    {
        TxnClient c(state, opts);
        c.sweep(0);
        std::vector<std::thread> ths;
        std::atomic<int> failed{0};
        for (int i = 0; i < A; i++)
            ths.emplace_back([&, i] {
                TxnClient cc(state, opts);
                for (int a = 0; a < 5; a++)
                    if (cc.set(acct(i), std::to_string(initial)).ok) return;
                failed++;
            });
        for (auto& t : ths) t.join();
        if (failed) { std::fprintf(stderr, "bank: could not fund %d account(s)\n", failed.load()); return 2; }
    }
    int onShard[8] = {0};
    {
        TxnClient c(state, opts);
        for (int i = 0; i < A; i++) onShard[c.ownerOf(acct(i)) % 8]++;
    }

    std::printf("bank: mode=%s audit=%s accounts=%d groups=%d workers=%d seconds=%d seed=%d lock-ttl=%dms  (accounts per shard:", naive ? "NAIVE(no transactions)" : "safe",
                plainAudit ? "plain" : "snapshot", A, G, W, S, seed, ttl);
    for (int s = 1; s < 8; s++) if (onShard[s]) std::printf(" s%d=%d", s, onShard[s]);
    std::printf(")\n");

    std::atomic<bool> stop{false};
    std::vector<Stats> stats(W);
    std::vector<std::thread> workers;
    for (int w = 0; w < W; w++) {
        workers.emplace_back([&, w] {
            TxnClient c(state, opts);
            std::mt19937 rng(static_cast<unsigned>(seed * 7919 + w));
            Stats& st = stats[w];
            while (!stop.load()) {
                const auto& grp = members[rng() % G];
                int from = grp[rng() % grp.size()], to = grp[rng() % grp.size()];
                if (from == to) continue;
                long amt = 1 + static_cast<long>(rng() % maxAmount);
                st.attempted++;
                bool settled = false; // committed, insufficient funds, or left in doubt (all counted elsewhere)
                for (int attempt = 0; attempt < attempts && !stop.load(); attempt++) {
                    auto rf = c.get(acct(from)), rt = c.get(acct(to));
                    long vf = 0, vt = 0;
                    if (!rf.ok || !rt.ok || !parseLong(rf.value, vf) || !parseLong(rt.value, vt)) {
                        st.readFail++;
                        sleepMs(100);
                        continue;
                    }
                    if (vf < amt) { st.insufficient++; settled = true; break; }
                    if (naive) {
                        if (c.set(acct(from), std::to_string(vf - amt)).ok && c.set(acct(to), std::to_string(vt + amt)).ok) {
                            st.committed++;
                            st.done.push_back({from, to, amt});
                            settled = true;
                        }
                        break;
                    }
                    Txn t;
                    t.expect(acct(from), rf.value).expect(acct(to), rt.value)
                     .put(acct(from), std::to_string(vf - amt)).put(acct(to), std::to_string(vt + amt));
                    Result r = c.commit(t);
                    if (r.outcome == Outcome::Committed) {
                        st.committed++;
                        (r.crossShard ? st.crossCommitted : st.singleCommitted)++;
                        (r.crossShard ? st.latCross : st.latSingle).push_back(r.latencyMs);
                        st.done.push_back({from, to, amt});
                        settled = true;
                        break;
                    }
                    if (r.outcome == Outcome::Indeterminate) {
                        st.doubt.push_back({r.txid, t, r.crossShard, {from, to, amt}});
                        settled = true;
                        break;
                    }
                    if (r.reason == Reason::Mismatch) st.abortedMismatch++;
                    else if (r.reason == Reason::Conflict) st.abortedConflict++;
                    else st.abortedOther++;
                    sleepMs(10 + static_cast<int>(rng() % (40 * (attempt + 1))));
                }
                if (!settled && !stop.load()) st.gaveUp++;
            }
        });
    }

    // ---- auditor
    std::atomic<long> auditsOk{0}, auditSkew{0}, auditNeg{0}, auditUnavail{0};
    std::mutex logMu;
    std::vector<std::string> samples;
    std::vector<std::thread> auditors;
    for (int g = 0; g < G; g++) {
        auditors.emplace_back([&, g] {
            TxnClient c(state, opts);
            std::vector<std::string> keys;
            for (int i : members[g]) keys.push_back(acct(i));
            const long groupTotal = static_cast<long>(keys.size()) * initial;
            while (!stop.load()) {
                sleepMs(auditMs);
                std::map<std::string, std::string> vals;
                bool ok = true;
                if (plainAudit) {
                    for (auto& k : keys) {
                        auto r = c.get(k);
                        if (!r.ok) { ok = false; break; }
                        vals[k] = r.value;
                    }
                } else {
                    ok = c.snapshotRead(keys, vals, 10);
                }
                if (!ok) { auditUnavail++; continue; }
                long sum = 0;
                bool parsed = true, neg = false;
                for (auto& kv : vals) {
                    long v;
                    if (!parseLong(kv.second, v)) { parsed = false; break; }
                    sum += v;
                    if (v < 0) neg = true;
                }
                if (!parsed) { auditUnavail++; continue; }
                if (neg) auditNeg++;
                if (sum != groupTotal) {
                    auditSkew++;
                    std::lock_guard<std::mutex> gd(logMu);
                    if (samples.size() < 5) samples.push_back("audit of group " + std::to_string(g) + " saw total " + std::to_string(sum) + " (should be " + std::to_string(groupTotal) + ")");
                } else {
                    auditsOk++;
                }
            }
        });
    }

    sleepMs(S * 1000);
    stop = true;
    for (auto& t : workers) t.join();
    for (auto& t : auditors) t.join();

    // ---- settle: finish every transaction still prepared, then every one left in doubt
    TxnClient c(state, opts);
    long stuck = 0;
    for (int round = 0; round < 20; round++) {
        c.sweep(0);
        auto p = c.pending();
        stuck = static_cast<long>(p.size());
        if (p.empty()) break;
        sleepMs(300);
    }
    long doubtCommitted = 0, doubtAborted = 0, doubtUnresolved = 0;
    std::vector<long> expected(A, initial);
    Stats agg;
    for (auto& st : stats) {
        agg.attempted += st.attempted; agg.gaveUp += st.gaveUp; agg.committed += st.committed; agg.abortedMismatch += st.abortedMismatch;
        agg.abortedConflict += st.abortedConflict; agg.abortedOther += st.abortedOther; agg.insufficient += st.insufficient;
        agg.readFail += st.readFail; agg.crossCommitted += st.crossCommitted; agg.singleCommitted += st.singleCommitted;
        agg.latCross.insert(agg.latCross.end(), st.latCross.begin(), st.latCross.end());
        agg.latSingle.insert(agg.latSingle.end(), st.latSingle.begin(), st.latSingle.end());
        for (auto& d : st.done) { expected[d.from] -= d.amount; expected[d.to] += d.amount; }
        for (auto& d : st.doubt) {
            bool committed = false, known = false;
            for (int tryNo = 0; tryNo < 20 && !known; tryNo++) {
                if (d.crossShard) {
                    auto dec = c.resolve(d.txid, 0);
                    if (dec != TxnClient::Decision::Unknown) { known = true; committed = (dec == TxnClient::Decision::Committed); }
                } else {
                    Result r = c.retrySingle(d.txn, d.txid);
                    if (r.outcome != Outcome::Indeterminate) { known = true; committed = (r.outcome == Outcome::Committed); }
                }
                if (!known) sleepMs(500);
            }
            if (!known) { doubtUnresolved++; continue; }
            if (committed) { doubtCommitted++; expected[d.tr.from] -= d.tr.amount; expected[d.tr.to] += d.tr.amount; }
            else doubtAborted++;
        }
    }
    for (int round = 0; round < 10; round++) { // resolving in-doubt txns may have left more prepared state
        c.sweep(0);
        if (c.pending().empty()) break;
        sleepMs(300);
    }
    stuck = static_cast<long>(c.pending().size());

    long finalSum = 0, finalMismatch = 0, finalNeg = 0, finalUnreadable = 0;
    for (int i = 0; i < A; i++) {
        auto r = c.get(acct(i));
        long v;
        if (!r.ok || !parseLong(r.value, v)) { finalUnreadable++; continue; }
        finalSum += v;
        if (v < 0) finalNeg++;
        if (v != expected[i]) {
            finalMismatch++;
            if (samples.size() < 8) samples.push_back(acct(i) + " ended at " + std::to_string(v) + ", committed transfers say " + std::to_string(expected[i]));
        }
    }

    const long sumViolation = (finalSum != total && finalUnreadable == 0) ? 1 : 0;
    std::printf("transfers: attempted=%ld committed=%ld (cross-shard 2PC=%ld, single-shard=%ld) gave-up-after-%d-tries=%ld  aborted attempts: mismatch=%ld conflict=%ld other=%ld  insufficient-funds=%ld  read-failures=%ld\n",
                agg.attempted, agg.committed + doubtCommitted, agg.crossCommitted, agg.singleCommitted, attempts, agg.gaveUp,
                agg.abortedMismatch, agg.abortedConflict, agg.abortedOther, agg.insufficient, agg.readFail);
    std::printf("in doubt after timeouts/failures: %ld resolved via decision record -> committed=%ld aborted=%ld unresolved=%ld\n",
                doubtCommitted + doubtAborted + doubtUnresolved, doubtCommitted, doubtAborted, doubtUnresolved);
    std::printf("commit latency ms (p50/p95): cross-shard %ld/%ld (n=%zu)   single-shard %ld/%ld (n=%zu)\n", pct(agg.latCross, 50),
                pct(agg.latCross, 95), agg.latCross.size(), pct(agg.latSingle, 50), pct(agg.latSingle, 95), agg.latSingle.size());
    std::printf("audits: consistent=%ld skewed=%ld unavailable=%ld   final: total=%ld (expected %ld) unreadable-accounts=%ld\n",
                auditsOk.load(), auditSkew.load(), auditUnavail.load(), finalSum, total, finalUnreadable);
    for (auto& s : samples) std::printf("  e.g. %s\n", s.c_str());
    const long proto = TxnClient::protocolViolations();
    std::printf("VIOLATIONS: audit-skew=%ld negative-balance=%ld final-total=%ld final-per-account=%ld stuck-locks=%ld unresolved-txns=%ld protocol=%ld\n",
                auditSkew.load(), auditNeg.load() + finalNeg, sumViolation, finalMismatch, stuck, doubtUnresolved, proto);
    long total_v = auditSkew + auditNeg + finalNeg + sumViolation + finalMismatch + stuck + doubtUnresolved + proto + finalUnreadable;
    std::printf("RESULT: %s (%ld violations)\n", total_v == 0 ? "CLEAN" : "VIOLATIONS DETECTED", total_v);
    return total_v == 0 ? 0 : 1;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr, "usage: %s <init|balances|transfer|bank> <ring.conf> ...\n", argv[0]);
        return 2;
    }
    std::string cmd = argv[1];
    int rc = 1;
    if (cmd == "init") rc = cmdInit(argc, argv);
    else if (cmd == "balances") rc = cmdBalances(argc, argv);
    else if (cmd == "transfer") rc = cmdTransfer(argc, argv);
    else if (cmd == "bank") rc = cmdBank(argc, argv);
    if (rc == 1 && (cmd != "bank")) { /* fallthrough: genuine failure or bad usage */ }
    return rc;
}
