// txnctl — command-line access to the transactional sharded KV store.
//
//   txnctl [options] <ring.conf> GET key
//   txnctl [options] <ring.conf> SET key value
//   txnctl [options] <ring.conf> TXN [-c key=expected]... [-w key=value]...
//        -c key=       key must be ABSENT        -w key=      ERASE the key
//   txnctl [options] <ring.conf> WHERE key
//   txnctl [options] <ring.conf> PENDING                    prepared-but-undecided transactions
//   txnctl [options] <ring.conf> SWEEP [minAgeMs]           resolve pending transactions at least that old (default 0)
//   txnctl [options] <ring.conf> RESOLVE <txid> [shard]     finish one transaction via its decision record
//   txnctl [options] <ring.conf> ENCODE (-c|-w key=value)...      print a transaction body in wire form
//   txnctl [options] <ring.conf> RAW <shard> <READ|WRITE> <command...>   send one raw command to one shard
//
// options: --crash-at after-begin|after-prepare|after-decide|mid-commit   (kill THIS process there)
//          --recovery guess-abort                                          (ablation: resolve without the record)
//          --pause-after-prepare-ms N    --lock-ttl-ms N
//
// Exit status for TXN: 0 committed, 3 aborted, 4 indeterminate.

#include "txn_client.h"

#include <cstdio>
#include <cstring>

using namespace txn;

static std::string show(const std::string& v) {
    std::string out;
    for (unsigned char c : v) {
        if (c == 0x01) out += "<LOCKED>";
        else out += static_cast<char>(c);
    }
    return out;
}

int main(int argc, char** argv) {
    Options opts;
    std::vector<std::string> args;
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        auto next = [&]() -> std::string { return (i + 1 < argc) ? argv[++i] : ""; };
        if (a == "--crash-at") {
            std::string v = next();
            if (v == "after-begin") opts.crashAt = CrashPoint::AfterBegin;
            else if (v == "after-prepare") opts.crashAt = CrashPoint::AfterPrepare;
            else if (v == "after-decide") opts.crashAt = CrashPoint::AfterDecide;
            else if (v == "mid-commit") opts.crashAt = CrashPoint::MidCommit;
            else { std::fprintf(stderr, "unknown crash point '%s'\n", v.c_str()); return 1; }
        } else if (a == "--recovery") {
            std::string v = next();
            if (v == "guess-abort") opts.recovery = Recovery::GuessAbort;
            else if (v != "record") { std::fprintf(stderr, "unknown recovery '%s'\n", v.c_str()); return 1; }
        } else if (a == "--pause-after-prepare-ms") {
            opts.pauseAfterPrepareMs = std::atoi(next().c_str());
        } else if (a == "--lock-ttl-ms") {
            opts.lockTtlMs = std::atoi(next().c_str());
        } else {
            args.push_back(a);
        }
    }
    if (args.size() < 2) {
        std::fprintf(stderr, "usage: %s [options] <ring.conf> <GET|SET|TXN|WHERE|PENDING|SWEEP|RESOLVE|RAW> ...\n", argv[0]);
        return 1;
    }
    shard::RingConfig cfg;
    std::string err;
    if (!shard::loadRingConfig(args[0], cfg, err)) {
        std::fprintf(stderr, "ring config error: %s\n", err.c_str());
        return 1;
    }
    auto state = std::make_shared<shard::RingState>(cfg);
    TxnClient c(state, opts);
    const std::string verb = args[1];

    if (verb == "WHERE" && args.size() == 3) {
        std::printf("key \"%s\" -> shard %u\n", args[2].c_str(), c.ownerOf(args[2]));
        return 0;
    }
    if (verb == "GET" && args.size() == 3) {
        auto r = c.get(args[2]);
        if (!r.ok) { std::fprintf(stderr, "FAILED: %s\n", r.value.empty() ? "no answer" : show(r.value).c_str()); return 1; }
        std::printf("OK value=\"%s\" (shard %u)\n", show(r.value).c_str(), c.ownerOf(args[2]));
        return 0;
    }
    if (verb == "SET" && args.size() == 4) {
        auto r = c.set(args[2], args[3]);
        if (!r.ok) { std::fprintf(stderr, "FAILED: %s\n", r.value.empty() ? "no answer" : show(r.value).c_str()); return 1; }
        std::printf("OK (shard %u)\n", c.ownerOf(args[2]));
        return 0;
    }
    if (verb == "PENDING" && args.size() == 2) {
        auto p = c.pending();
        for (auto& pr : p) std::printf("shard %u  %s  age=%ldms\n", pr.first, pr.second.c_str(), TxnClient::ageMs(pr.second));
        std::printf("%zu prepared-but-undecided transaction(s)\n", p.size());
        return 0;
    }
    if (verb == "SWEEP" && args.size() <= 3) {
        int minAge = args.size() == 3 ? std::atoi(args[2].c_str()) : 0;
        int n = c.sweep(minAge);
        std::printf("resolved %d transaction(s)\n", n);
        return 0;
    }
    if (verb == "RESOLVE" && (args.size() == 3 || args.size() == 4)) {
        uint32_t hint = args.size() == 4 ? static_cast<uint32_t>(std::atoi(args[3].c_str())) : 0;
        bool cleanup = false;
        auto d = c.resolve(args[2], hint, &cleanup);
        std::printf("%s%s\n", d == TxnClient::Decision::Committed ? "COMMITTED" : d == TxnClient::Decision::Aborted ? "ABORTED" : "UNKNOWN",
                    cleanup ? " (cleanup pending)" : "");
        return d == TxnClient::Decision::Unknown ? 4 : 0;
    }
    if (verb == "RAW" && args.size() >= 5) {
        uint32_t s = static_cast<uint32_t>(std::atoi(args[2].c_str()));
        bool isRead = (args[3] == "READ");
        std::string cmd;
        for (size_t i = 4; i < args.size(); i++) cmd += (i > 4 ? " " : "") + args[i];
        auto r = c.shardClient(s).exec(cmd, isRead);
        if (!r.ok) { std::fprintf(stderr, "FAILED: no answer from shard %u\n", s); return 1; }
        std::printf("%s\n", show(r.value).c_str());
        return 0;
    }
    if (verb == "ENCODE") { // print the wire form of a transaction body, for building RAW commands
        Txn t;
        for (size_t i = 2; i + 1 < args.size(); i += 2) {
            std::string kv = args[i + 1];
            auto eq = kv.find('=');
            if (eq == std::string::npos || (args[i] != "-c" && args[i] != "-w")) { std::fprintf(stderr, "usage: ENCODE (-c|-w key=value)...\n"); return 1; }
            std::string k = kv.substr(0, eq), v = kv.substr(eq + 1);
            if (args[i] == "-c") t.expect(k, v);
            else if (v.empty()) t.erase(k);
            else t.put(k, v);
        }
        std::printf("%s\n", encodeItems(t.items()).c_str());
        return 0;
    }
    if (verb == "TXN") {
        Txn t;
        for (size_t i = 2; i < args.size(); i++) {
            const std::string& flag = args[i];
            if ((flag == "-c" || flag == "-w") && i + 1 < args.size()) {
                std::string kv = args[++i];
                auto eq = kv.find('=');
                if (eq == std::string::npos) { std::fprintf(stderr, "expected key=value, got '%s'\n", kv.c_str()); return 1; }
                std::string k = kv.substr(0, eq), v = kv.substr(eq + 1);
                if (flag == "-c") t.expect(k, v);
                else if (v.empty()) t.erase(k);
                else t.put(k, v);
            } else {
                std::fprintf(stderr, "bad TXN argument '%s'\n", flag.c_str());
                return 1;
            }
        }
        Result r = c.commit(t);
        static const char* reason[] = {"", "mismatch", "conflict", "fenced", "unavailable", "invalid"};
        const char* name = r.outcome == Outcome::Committed ? "COMMITTED" : r.outcome == Outcome::Aborted ? "ABORTED" : "INDETERMINATE";
        std::printf("%s%s%s%s  txid=%s  %s  %dms%s%s\n", name, r.reason == Reason::None ? "" : " (",
                    reason[static_cast<int>(r.reason)], r.reason == Reason::None ? "" : ")", r.txid.c_str(),
                    r.crossShard ? "cross-shard (2PC)" : "single-shard (1 log entry)", r.latencyMs,
                    r.cleanupPending ? "  [cleanup pending]" : "", r.detail.empty() ? "" : ("  " + r.detail).c_str());
        return r.outcome == Outcome::Committed ? 0 : r.outcome == Outcome::Aborted ? 3 : 4;
    }
    std::fprintf(stderr, "bad command or wrong number of arguments\n");
    return 1;
}
