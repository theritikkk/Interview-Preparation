#include "txn_client.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <random>
#include <set>
#include <sstream>
#include <thread>
#include <unistd.h>

namespace txn {

using raft::KVResult;
using Clock = std::chrono::steady_clock;

std::atomic<long> TxnClient::violations_{0};

namespace {

long nowEpochMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

void sleepMs(int ms) {
    if (ms > 0) std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

bool validKey(const std::string& k) {
    return !k.empty() && k.find_first_of(" \n") == std::string::npos;
}

std::vector<std::string> splitOn(const std::string& s, char sep) {
    std::vector<std::string> out;
    size_t i = 0;
    while (i <= s.size()) {
        size_t j = s.find(sep, i);
        if (j == std::string::npos) j = s.size();
        if (j > i) out.push_back(s.substr(i, j - i));
        i = j + 1;
    }
    return out;
}

std::string firstWord(const std::string& s) { return s.substr(0, s.find(' ')); }
std::string restAfterFirstWord(const std::string& s) {
    auto sp = s.find(' ');
    return sp == std::string::npos ? "" : s.substr(sp + 1);
}

std::string encodeBody(const std::vector<Item>& items) {
    std::string out;
    for (auto& it : items) {
        if (!out.empty()) out += ',';
        out += it.compare ? 'c' : 'w';
        out += hexEncode(it.key);
        out += ':';
        out += hexEncode(it.value);
    }
    return out;
}

} // namespace

std::string hexEncode(const std::string& s) {
    static const char* d = "0123456789abcdef";
    std::string out;
    out.reserve(s.size() * 2);
    for (unsigned char c : s) {
        out.push_back(d[c >> 4]);
        out.push_back(d[c & 15]);
    }
    return out;
}

std::string encodeItems(const std::vector<Item>& items) { return encodeBody(items); }

// ------------------------------------------------------------------- Txn
Txn& Txn::add(bool compare, const std::string& key, const std::string& value) {
    if (!validKey(key)) {
        invalid_ = "bad key (empty, or contains a space/newline): '" + key + "'";
    } else if (!compare && !value.empty() && value[0] == kLockMarker) {
        invalid_ = "value starts with the reserved 0x01 byte";
    }
    items_.push_back(Item{compare, key, value});
    return *this;
}

Txn& Txn::put(const std::string& key, const std::string& value) {
    if (value.empty()) {
        invalid_ = "put() with an empty value; use erase() to delete";
        return *this;
    }
    return add(false, key, value);
}

// ------------------------------------------------------------ txid helpers
// txid = "<coordinator shard>.<epoch ms>.<client tag>.<seq>"
uint32_t TxnClient::coordOf(const std::string& txid) {
    auto p = splitOn(txid, '.');
    if (p.size() != 4) return 0;
    return static_cast<uint32_t>(std::strtoul(p[0].c_str(), nullptr, 10));
}

long TxnClient::ageMs(const std::string& txid) {
    auto p = splitOn(txid, '.');
    if (p.size() != 4) return -1;
    return nowEpochMs() - std::strtol(p[1].c_str(), nullptr, 10);
}

bool TxnClient::parseLocked(const std::string& v, std::string& txid) {
    static const std::string prefix = std::string(1, kLockMarker) + "LOCKED ";
    if (v.rfind(prefix, 0) != 0) return false;
    txid = v.substr(prefix.size());
    return true;
}

TxnClient::TxnClient(std::shared_ptr<shard::RingState> state, Options opts)
    : state_(std::move(state)), opts_(opts),
      sc_(state_, [&] {
          shard::ClientOptions co;
          co.rpcTimeoutMs = opts.rpcTimeoutMs;
          return co;
      }()) {
    std::random_device rd;
    std::mt19937_64 g(rd());
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%06x", static_cast<unsigned>(g() & 0xffffff));
    clientTag_ = buf;
}

std::string TxnClient::makeTxid(uint32_t coord) {
    return std::to_string(coord) + "." + std::to_string(nowEpochMs()) + "." + clientTag_ + "." +
           std::to_string(++seq_);
}

void TxnClient::crashNow(const char* where, const std::string& txid) {
    std::fprintf(stderr, "[txn] CRASH INJECTED at %s (txid=%s)\n", where, txid.c_str());
    std::fflush(stderr);
    _exit(opts_.crashExitCode);
}

KVResult TxnClient::call(uint32_t shardId, const std::string& cmd, bool isRead, int tries) {
    KVResult r;
    for (int i = 0; i < tries; i++) {
        r = sc_.shard(shardId).exec(cmd, isRead);
        if (r.ok) return r;
    }
    return r;
}

// Send TXCOMMIT or TXABORT to every participant (in parallel). True iff every
// participant acknowledged. Counts protocol violations: the replies that must
// be impossible if the protocol is right.
bool TxnClient::deliver(const std::string& txid, const std::vector<uint32_t>& parts, bool commit) {
    for (uint32_t p : parts) sc_.shard(p); // create clients on THIS thread: the map isn't thread-safe
    std::vector<int> okFlags(parts.size(), 0);
    std::vector<std::thread> ths;
    const std::string cmd = std::string(commit ? "TXCOMMIT " : "TXABORT ") + txid;
    for (size_t i = 0; i < parts.size(); i++) {
        ths.emplace_back([&, i] {
            KVResult r = call(parts[i], cmd, false, 3);
            if (!r.ok) return;
            if (r.value == "OK") {
                okFlags[i] = 1;
            } else {
                violations_++;
                std::fprintf(stderr, "[txn] PROTOCOL VIOLATION: %s on shard %u answered '%s' (txid %s)\n",
                             commit ? "TXCOMMIT" : "TXABORT", parts[i], r.value.c_str(), txid.c_str());
            }
        });
    }
    for (auto& t : ths) t.join();
    return std::all_of(okFlags.begin(), okFlags.end(), [](int f) { return f == 1; });
}

// ------------------------------------------------------------------ commit
Result TxnClient::commitSingleShard(const Txn& t, uint32_t shardId, const std::string& txid) {
    Result r;
    r.txid = txid.empty() ? makeTxid(shardId) : txid;
    const std::string cmd = "TXONE " + r.txid + " " + encodeBody(t.items());
    // The same txid on every attempt: the state machine remembers the outcome,
    // so a retry after a lost reply returns what actually happened instead of
    // applying the transaction twice.
    for (int attempt = 0; attempt < 3; attempt++) {
        KVResult x = sc_.shard(shardId).exec(cmd, false);
        if (!x.ok) continue;
        if (x.value == "COMMITTED") {
            r.outcome = Outcome::Committed;
        } else if (x.value == "MISMATCH" || x.value == "ABORTED") {
            r.outcome = Outcome::Aborted;
            r.reason = Reason::Mismatch;
        } else if (x.value.rfind("LOCKED ", 0) == 0) {
            r.outcome = Outcome::Aborted;
            r.reason = Reason::Conflict;
            r.blocker = x.value.substr(7);
        } else {
            r.outcome = Outcome::Indeterminate;
            r.detail = "unexpected reply: " + x.value;
        }
        return r;
    }
    r.outcome = Outcome::Indeterminate;
    r.reason = Reason::Unavailable;
    r.detail = "shard " + std::to_string(shardId) + " did not answer";
    return r;
}

Result TxnClient::commit(const Txn& t) {
    const auto t0 = Clock::now();
    Result r;
    auto finish = [&](Result res) {
        res.latencyMs = static_cast<int>(
            std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - t0).count());
        return res;
    };
    if (!t.invalid().empty() || t.items().empty()) {
        r.outcome = Outcome::Aborted;
        r.reason = Reason::Invalid;
        r.detail = t.invalid().empty() ? "empty transaction" : t.invalid();
        return finish(r);
    }

    std::map<uint32_t, std::vector<Item>> byShard;
    for (auto& it : t.items()) byShard[ownerOf(it.key)].push_back(it);

    if (byShard.size() == 1) return finish(commitSingleShard(t, byShard.begin()->first));

    // ---------------------------------------------------------- two-phase commit
    std::vector<uint32_t> parts;
    std::string plist;
    for (auto& kv : byShard) {
        parts.push_back(kv.first);
        if (!plist.empty()) plist += ',';
        plist += std::to_string(kv.first);
    }
    const uint32_t coord = parts.front();
    r.crossShard = true;
    r.txid = makeTxid(coord);
    const std::string& txid = r.txid;
    for (uint32_t p : parts) sc_.shard(p); // pre-create clients (not thread-safe to create concurrently)

    // 1. durable decision record, state Pending. Done BEFORE any participant is
    //    asked to prepare, so anyone who ever finds a lock can find the record.
    KVResult b = call(coord, "TXBEGIN " + txid + " " + plist, false);
    if (!b.ok) {
        r.outcome = Outcome::Aborted; // nothing prepared yet, so nothing to undo
        r.reason = Reason::Unavailable;
        r.detail = "coordinator shard " + std::to_string(coord) + " unreachable";
        return finish(r);
    }
    if (firstWord(b.value) != "PENDING") { // someone already decided this txid
        r.outcome = Outcome::Aborted;
        r.reason = Reason::Fenced;
        return finish(r);
    }
    if (opts_.crashAt == CrashPoint::AfterBegin) crashNow("after-begin", txid);

    // 2. prepare everywhere, in parallel
    struct Rep {
        bool ok = false;
        std::string v;
    };
    std::vector<Rep> reps(parts.size());
    {
        std::vector<std::thread> ths;
        for (size_t i = 0; i < parts.size(); i++) {
            ths.emplace_back([&, i] {
                KVResult x = call(parts[i], "TXPREP " + txid + " " + encodeBody(byShard.at(parts[i])), false);
                reps[i].ok = x.ok;
                reps[i].v = x.value;
            });
        }
        for (auto& th : ths) th.join();
    }
    bool allPrepared = true, mismatch = false, conflict = false, unknown = false;
    for (auto& rp : reps) {
        if (!rp.ok) { unknown = true; allPrepared = false; continue; }
        if (rp.v == "PREPARED") continue;
        allPrepared = false;
        if (rp.v == "MISMATCH") mismatch = true;
        else if (rp.v.rfind("LOCKED ", 0) == 0) { conflict = true; r.blocker = rp.v.substr(7); }
        // "ABORTED": a resolver fenced us out first; falls through as a plain abort
    }
    if (opts_.crashAt == CrashPoint::AfterPrepare) crashNow("after-prepare", txid);
    sleepMs(opts_.pauseAfterPrepareMs);

    // 3. the commit point
    KVResult d = call(coord, std::string("TXDECIDE ") + txid + (allPrepared ? " COMMIT" : " ABORT"), false);
    if (!d.ok) {
        if (allPrepared) {
            // We cannot tell whether the decision was recorded. Say so honestly;
            // resolve(txid) settles it for anyone who asks.
            r.outcome = Outcome::Indeterminate;
            r.reason = Reason::Unavailable;
            r.detail = "decision record unreachable";
            return finish(r);
        }
        // A prepare failed, so nobody can ever commit this txn (only we issue
        // COMMIT, and we are not going to). Aborting the participants is safe
        // even without a recorded decision.
        r.cleanupPending = !deliver(txid, parts, false);
        r.outcome = Outcome::Aborted;
        r.reason = mismatch ? Reason::Mismatch : conflict ? Reason::Conflict : Reason::Unavailable;
        return finish(r);
    }
    const std::string decided = firstWord(d.value);
    if (opts_.crashAt == CrashPoint::AfterDecide) crashNow("after-decide", txid);

    // 4. tell the participants
    if (decided == "COMMITTED") {
        if (opts_.crashAt == CrashPoint::MidCommit) {
            // Deliver to the first participant only, then die: a partially
            // delivered commit, the nastiest place for a coordinator to fail.
            call(parts.front(), "TXCOMMIT " + txid, false, 3);
            crashNow("mid-commit", txid);
        }
        r.cleanupPending = !deliver(txid, parts, true);
        r.outcome = Outcome::Committed;
    } else {
        r.cleanupPending = !deliver(txid, parts, false);
        r.outcome = Outcome::Aborted;
        r.reason = mismatch ? Reason::Mismatch : conflict ? Reason::Conflict
                   : unknown ? Reason::Unavailable : Reason::Fenced;
    }
    return finish(r);
}

// ----------------------------------------------------------------- recovery
TxnClient::Decision TxnClient::resolve(const std::string& txid, uint32_t hintShard, bool* cleanupPending) {
    if (cleanupPending) *cleanupPending = false;
    const uint32_t coord = coordOf(txid);
    if (coord == 0) return Decision::Unknown;

    if (opts_.recovery == Recovery::GuessAbort) {
        // ABLATION. No decision record: all this resolver knows is that ONE
        // participant has been holding a lock for too long, so it aborts that
        // participant on its own authority.
        if (hintShard != 0) {
            KVResult x = call(hintShard, "TXABORT " + txid, false, 3);
            if (!x.ok && cleanupPending) *cleanupPending = true;
        }
        return Decision::Aborted;
    }

    // TXDECIDE ... ABORT is the whole trick: if the record already says COMMITTED
    // or ABORTED it just reads that back; if it says Pending it makes the abort
    // final, which also stops the original coordinator from ever committing.
    KVResult d = call(coord, "TXDECIDE " + txid + " ABORT", false, 3);
    if (!d.ok) return Decision::Unknown;
    const std::string st = firstWord(d.value);
    const bool commit = (st == "COMMITTED");
    if (!commit && st != "ABORTED") return Decision::Unknown;

    std::set<uint32_t> parts;
    for (auto& p : splitOn(restAfterFirstWord(d.value), ',')) {
        if (p == "-") continue;
        parts.insert(static_cast<uint32_t>(std::strtoul(p.c_str(), nullptr, 10)));
    }
    if (hintShard != 0) parts.insert(hintShard);
    for (uint32_t p : parts) sc_.shard(p);
    bool ok = deliver(txid, std::vector<uint32_t>(parts.begin(), parts.end()), commit);
    if (cleanupPending) *cleanupPending = !ok;
    return commit ? Decision::Committed : Decision::Aborted;
}

std::vector<std::pair<uint32_t, std::string>> TxnClient::pending() {
    std::vector<std::pair<uint32_t, std::string>> out;
    for (uint32_t s : shardIds()) {
        KVResult r = call(s, "TXPENDING", true, 3);
        if (!r.ok) continue;
        std::istringstream is(r.value);
        std::string line;
        while (std::getline(is, line))
            if (!line.empty()) out.emplace_back(s, line);
    }
    return out;
}

int TxnClient::sweep(int minAgeMs) {
    // A cross-shard txn shows up once per participant, but resolving it once
    // finishes every participant, so count distinct transactions.
    int n = 0;
    std::set<std::string> done;
    for (auto& pr : pending()) {
        if (ageMs(pr.second) < minAgeMs) continue;
        if (done.count(pr.second)) continue;
        if (resolve(pr.second, pr.first) != Decision::Unknown) {
            done.insert(pr.second);
            n++;
        }
    }
    return n;
}

// A key we need is locked by `txid`: finish that transaction if it has been
// around long enough to be presumed dead, otherwise give it a moment.
void TxnClient::help(const std::string& txid, uint32_t shardId) {
    if (ageMs(txid) >= opts_.lockTtlMs) {
        resolve(txid, shardId);
    } else {
        static thread_local std::mt19937 g(std::random_device{}());
        sleepMs(15 + static_cast<int>(g() % 40));
    }
}

KVResult TxnClient::get(const std::string& key) {
    const auto deadline = Clock::now() + std::chrono::milliseconds(opts_.maxHelpWaitMs);
    const uint32_t s = ownerOf(key);
    while (true) {
        KVResult r = sc_.shard(s).get(key);
        std::string holder;
        if (!r.ok || !parseLocked(r.value, holder)) return r;
        if (Clock::now() > deadline) {
            r.ok = false;
            r.value = "STUCK: key held by transaction " + holder;
            return r;
        }
        help(holder, s);
    }
}

KVResult TxnClient::set(const std::string& key, const std::string& value) {
    KVResult bad;
    bad.ok = false;
    bad.reachedCluster = true;
    if (!validKey(key) || value.empty() || value[0] == kLockMarker) {
        bad.value = "REJECTED";
        return bad;
    }
    const auto deadline = Clock::now() + std::chrono::milliseconds(opts_.maxHelpWaitMs);
    const uint32_t s = ownerOf(key);
    while (true) {
        KVResult r = sc_.shard(s).set(key, value);
        std::string holder;
        if (!r.ok || !parseLocked(r.value, holder)) return r;
        if (Clock::now() > deadline) {
            r.ok = false;
            r.value = "STUCK: key held by transaction " + holder;
            return r;
        }
        help(holder, s);
    }
}

bool TxnClient::snapshotRead(const std::vector<std::string>& keys, std::map<std::string, std::string>& out,
                             int attempts) {
    static thread_local std::mt19937 g(std::random_device{}());
    for (int a = 0; a < attempts; a++) {
        std::map<std::string, std::string> vals;
        bool ok = true;
        for (auto& k : keys) {
            KVResult r = get(k);
            if (!r.ok) { ok = false; break; }
            vals[k] = r.value;
        }
        if (ok) {
            Txn v;
            for (auto& kv : vals) v.expect(kv.first, kv.second);
            Result res = commit(v);
            if (res.outcome == Outcome::Committed) {
                out = std::move(vals);
                return true;
            }
        }
        sleepMs(20 + static_cast<int>(g() % 100));
    }
    return false;
}

} // namespace txn
