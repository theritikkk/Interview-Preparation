#include "raft_node.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <ctime>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <random>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

namespace raft {

static constexpr int kRpcTimeoutMs = 600;        // per-RPC connect+read timeout
static constexpr int kHeartbeatIntervalMs = 150;  // leader -> followers
static constexpr int kElectionPollMs = 50;        // how often electionLoop checks the clock
static constexpr int kElectionTimeoutMinMs = 1500;
static constexpr int kElectionTimeoutMaxMs = 3000;

// connectWithTimeout() is shared with the CLI client — defined in wire.h.

// ---------------------------------------------------------------------
RaftNode::RaftNode(uint32_t id, uint16_t listenPort, std::vector<PeerConfig> peers,
                   std::string logPrefix, std::string dataDir)
    : id_(id), listenPort_(listenPort), peers_(std::move(peers)), dataDir_(std::move(dataDir)),
      logPrefix_(std::move(logPrefix)) {
    if (const char* a = std::getenv("RAFT_TXN_ABLATE")) {
        std::string v = a;
        ablateNoLock_ = (v.find("nolock") != std::string::npos);
        ablateNoFence_ = (v.find("nofence") != std::string::npos);
    }
    auto envStr = [](const char* name) -> std::string {
        const char* v = std::getenv(name);
        return v ? std::string(v) : std::string();
    };
    persist_ = !dataDir_.empty() && envStr("RAFT_PERSIST") != "0";
    fsync_ = envStr("RAFT_FSYNC") != "0";
    persistVote_ = envStr("RAFT_NO_PERSIST_VOTE") != "1";
    if (!envStr("RAFT_SNAP_EVERY").empty()) snapEvery_ = std::strtoull(envStr("RAFT_SNAP_EVERY").c_str(), nullptr, 10);
    if (!envStr("RAFT_SNAP_KEEP").empty()) snapKeep_ = std::strtoull(envStr("RAFT_SNAP_KEEP").c_str(), nullptr, 10);
    crashAt_ = envStr("RAFT_CRASH_AT");
    if (!envStr("RAFT_FAIL_FSYNC_AFTER").empty())
        wal_.failFsyncAfter(std::strtoull(envStr("RAFT_FAIL_FSYNC_AFTER").c_str(), nullptr, 10));
}

RaftNode::~RaftNode() { stop(); }

void RaftNode::log(const std::string& msg) const {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()).count() % 1000;
    std::time_t t = system_clock::to_time_t(now);
    char buf[16];
    std::strftime(buf, sizeof(buf), "%H:%M:%S", std::localtime(&t));
    const char* roleStr = role_ == Role::Leader      ? "LEADER"
                          : role_ == Role::Candidate ? "CANDIDATE"
                                                      : "follower";
    std::fprintf(stderr, "[%s.%03lld] [%s id=%u term=%llu %s] %s\n", buf, (long long)ms,
                logPrefix_.c_str(), id_, (unsigned long long)currentTerm_, roleStr, msg.c_str());
    std::fflush(stderr);
}

void RaftNode::toggleDeafened() {
    bool now = !deafened_.load();
    deafened_.store(now);
    log(now ? "PARTITION SIMULATED: deafened from all peer RPCs (clients still served)"
            : "partition healed: peer RPCs flow again");
}

uint64_t RaftNode::lastLogIndex() const { return logBase_ + log_.size() - 1; }
uint64_t RaftNode::lastLogTerm() const { return log_.back().term; }

// ---------------------------------------------------------------------
// Durability helpers. All assume mu_ is held, and all fsync BEFORE returning:
// every caller goes on to reply to a peer or client, and Raft's safety argument
// is "what I told you is on disk".
void RaftNode::walFatal(const char* what) const {
    log(std::string("FATAL: WAL ") + what + " failed (" + std::strerror(errno) +
        "); exiting rather than acknowledge data that may not be on disk");
    ::_exit(2);
}

void RaftNode::crashPoint(const char* name) const {
    if (!crashAt_.empty() && crashAt_ == name) {
        log(std::string("CRASH INJECTED at ") + name);
        ::_exit(77);
    }
}

void RaftNode::persistHardState() {
    if (!persist_ || !persistVote_) return;
    if (!wal_.appendHardState(currentTerm_, votedFor_) || !wal_.sync()) walFatal("hard-state write");
}

void RaftNode::persistAppended(uint64_t fromIndex) {
    if (!persist_) return;
    for (uint64_t i = fromIndex; i <= lastLogIndex(); i++)
        if (!wal_.appendEntry(i, termAt(i), entryAt(i).command)) walFatal("entry append");
    if (!wal_.sync()) walFatal("fsync");
}

void RaftNode::simulatePowerLoss() {
    std::lock_guard<std::mutex> lk(mu_);
    log("POWER LOSS SIMULATED: discarding everything that was not fsync'd");
    if (persist_) {
        wal_.simulatePowerLoss();
        if (snapUnsynced_) ::unlink((dataDir_ + "/snapshot").c_str());
    }
    ::_exit(137);
}

void RaftNode::resetElectionDeadline() {
    static thread_local std::mt19937 rng(
        std::random_device{}() ^
        static_cast<unsigned>(std::hash<std::thread::id>{}(std::this_thread::get_id())));
    std::uniform_int_distribution<int> dist(kElectionTimeoutMinMs, kElectionTimeoutMaxMs);
    electionTimeout_ = std::chrono::milliseconds(dist(rng));
    lastHeartbeat_ = std::chrono::steady_clock::now();
}

void RaftNode::becomeFollower(uint64_t newTerm) {
    if (newTerm > currentTerm_) {
        currentTerm_ = newTerm;
        votedFor_ = -1;
        persistHardState(); // a new term is durable before we say or do anything in it
        // A leader hint carried over from an earlier term is worthless the
        // moment the term advances: whoever we thought was leading before
        // is either already deposed or about to be, since SOMETHING with a
        // higher term just contacted us (a RequestVote from a new
        // candidate, or an AppendEntries from a new leader). Serving a
        // stale hint here sends clients straight back to a dead or
        // step-down-pending node — exactly the "could not reach a leader"
        // loop this fixes. currentLeaderHint_ gets set again, correctly,
        // the moment we actually hear from this term's real leader via
        // handleAppendEntriesMsg.
        currentLeaderHint_ = -1;
    }
    if (role_ != Role::Follower) log("became FOLLOWER");
    role_ = Role::Follower;
    resetElectionDeadline();
}

void RaftNode::becomeCandidate() {
    role_ = Role::Candidate;
    currentTerm_++;
    votedFor_ = static_cast<int32_t>(id_);
    persistHardState(); // our own vote must survive a restart before we ask anyone for theirs
    currentLeaderHint_ = -1;
    resetElectionDeadline();
    log("became CANDIDATE, starting election");
}

void RaftNode::becomeLeader() {
    role_ = Role::Leader;
    currentLeaderHint_ = static_cast<int32_t>(id_);
    nextIndex_.clear();
    matchIndex_.clear();
    uint64_t li = lastLogIndex();
    for (auto& p : peers_) {
        nextIndex_[p.id] = li + 1;
        matchIndex_[p.id] = 0;
    }
    log("became LEADER");

    // §8 of the Raft paper: a leader can never advance commitIndex_ over an
    // entry from a PREVIOUS term purely by counting replicas — that's the
    // `log_[N].term != currentTerm_` guard in leaderLoop()'s commit-index
    // loop, and it is a required safety rule, not a bug (committing an old
    // term's entry by count alone is exactly how §5.4.2's figure-8 scenario
    // loses committed data). The consequence: a freshly elected leader may
    // already hold older, majority-replicated entries in its log, but it
    // cannot apply/serve them until it commits something. Waiting for an
    // actual client write to show up is not good enough — reads could lag
    // indefinitely, and read-index's `readIndex = commitIndex_` snapshot
    // (see handleClientRequestMsg) would never safely advance past this
    // election either. The standard fix — still needed even now that
    // read-index does the per-read confirmation — is to have every new
    // leader append a no-op entry stamped with its OWN term immediately on
    // election; once that entry replicates to a majority, the Log Matching
    // Property commits it *and* every entry before it in the same step.
    log_.push_back(LogEntry{currentTerm_, ""});
    termStartIndex_ = lastLogIndex();
    persistAppended(termStartIndex_);
}

namespace {

// What a plain SET/DEL/CAS/CAD/GET returns when a prepared-but-undecided
// transaction holds a lock on its key. A leading 0x01 byte keeps it from
// colliding with any legitimate value (TxnClient refuses to store values that
// start with it).
std::string lockedReply(const std::string& txid) { return std::string("\x01") + "LOCKED " + txid; }

std::vector<std::string> splitSpaces(const std::string& s) {
    std::vector<std::string> out;
    size_t i = 0;
    while (i <= s.size()) {
        size_t j = s.find(' ', i);
        if (j == std::string::npos) j = s.size();
        out.push_back(s.substr(i, j - i));
        i = j + 1;
    }
    return out;
}

bool hexDecode(const std::string& h, std::string& out) {
    if (h.size() % 2) return false;
    out.clear();
    auto nib = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        return -1;
    };
    for (size_t i = 0; i < h.size(); i += 2) {
        int a = nib(h[i]), b = nib(h[i + 1]);
        if (a < 0 || b < 0) return false;
        out.push_back(static_cast<char>(a * 16 + b));
    }
    return true;
}

struct TxItem {
    bool compare;       // true: key must currently equal val ("" == absent); false: write val ("" == delete)
    std::string key, val;
};

// body := item (',' item)*     item := ('c'|'w') hex(key) ':' hex(val)
// Hex keeps arbitrary bytes out of the space/comma-delimited command line.
bool parseTxBody(const std::string& body, std::vector<TxItem>& out) {
    out.clear();
    size_t i = 0;
    while (i < body.size()) {
        size_t j = body.find(',', i);
        if (j == std::string::npos) j = body.size();
        std::string item = body.substr(i, j - i);
        if (item.size() < 2 || (item[0] != 'c' && item[0] != 'w')) return false;
        size_t colon = item.find(':');
        if (colon == std::string::npos) return false;
        TxItem it;
        it.compare = (item[0] == 'c');
        if (!hexDecode(item.substr(1, colon - 1), it.key) || it.key.empty()) return false;
        if (!hexDecode(item.substr(colon + 1), it.val)) return false;
        out.push_back(std::move(it));
        i = j + 1;
    }
    return !out.empty();
}

} // namespace

std::string RaftNode::applyToStateMachine(const LogEntry& e) {
    const std::string& cmd = e.command;

    // A plain write to a key a prepared transaction has locked must NOT go
    // through: it would be silently overwritten by (or invalidate the compare
    // of) the transaction that is about to commit.
    auto lockHolder = [&](const std::string& key) -> const std::string* {
        auto it = locks_.find(key);
        return it == locks_.end() ? nullptr : &it->second;
    };

    if (cmd.rfind("SET ", 0) == 0) {
        std::string rest = cmd.substr(4);
        auto sp = rest.find(' ');
        if (sp != std::string::npos) {
            std::string key = rest.substr(0, sp);
            if (const std::string* h = lockHolder(key)) return lockedReply(*h);
            kv_[key] = rest.substr(sp + 1);
        }
        return "";
    }
    if (cmd.rfind("DEL ", 0) == 0) {
        std::string key = cmd.substr(4);
        if (const std::string* h = lockHolder(key)) return lockedReply(*h);
        kv_.erase(key);
        return "";
    }
    if (cmd.rfind("CAS ", 0) == 0) {
        // "CAS key expected new" — space-separated, so this teaching
        // implementation doesn't support spaces inside values (same
        // limitation SET already has). Applies `new` iff the key's
        // current value equals `expected` ("" counts as "key absent").
        std::string rest = cmd.substr(4);
        auto sp1 = rest.find(' ');
        if (sp1 == std::string::npos) return "MISMATCH";
        std::string key = rest.substr(0, sp1);
        std::string tail = rest.substr(sp1 + 1);
        auto sp2 = tail.find(' ');
        std::string expected = (sp2 == std::string::npos) ? tail : tail.substr(0, sp2);
        std::string newVal = (sp2 == std::string::npos) ? "" : tail.substr(sp2 + 1);

        if (const std::string* h = lockHolder(key)) return lockedReply(*h);
        auto it = kv_.find(key);
        std::string current = (it != kv_.end()) ? it->second : "";
        if (current != expected) return "MISMATCH";
        kv_[key] = newVal;
        return "OK";
    }
    if (cmd.rfind("CAD ", 0) == 0) {
        // "CAD key expected" — compare-and-delete (Project 12). `expected` is
        // everything after the key, so it may contain spaces.
        std::string rest = cmd.substr(4);
        auto sp = rest.find(' ');
        std::string key = (sp == std::string::npos) ? rest : rest.substr(0, sp);
        std::string expected = (sp == std::string::npos) ? "" : rest.substr(sp + 1);
        if (const std::string* h = lockHolder(key)) return lockedReply(*h);
        auto it = kv_.find(key);
        std::string current = (it != kv_.end()) ? it->second : "";
        if (current != expected) return "MISMATCH";
        if (it != kv_.end()) kv_.erase(it);
        return "OK";
    }

    // ------------------------------------------------------------------
    // Transactions (Project 13). Every command below is IDEMPOTENT, keyed by
    // txid: the client library retries on timeouts and leader changes, and a
    // retried command may be appended to the log twice. Replies are plain
    // words (not user values), so unlike plain ops they need no 0x01 marker.
    // ------------------------------------------------------------------
    auto applyWrites = [&](const std::vector<std::pair<std::string, std::string>>& ws) {
        for (auto& w : ws) {
            if (w.second.empty()) kv_.erase(w.first);
            else kv_[w.first] = w.second;
        }
    };
    auto stateName = [](int st, bool prepared) -> const char* {
        if (prepared && st == 0) return "PREPARED";
        return st == 1 ? "COMMITTED" : "ABORTED";
    };

    if (cmd.rfind("TXONE ", 0) == 0) {
        // Whole transaction on ONE shard: atomic compare-and-write in a single
        // log entry, no locks held across entries (the entry applies atomically).
        auto t = splitSpaces(cmd);
        std::vector<TxItem> items;
        if (t.size() != 3 || !parseTxBody(t[2], items)) return "ERR_BAD";
        const std::string& txid = t[1];
        auto prev = txns_.find(txid);
        if (prev != txns_.end()) return stateName(prev->second.state, false);
        for (auto& it : items)
            if (const std::string* h = lockHolder(it.key)) return "LOCKED " + *h; // not recorded: retryable
        for (auto& it : items) {
            if (!it.compare) continue;
            auto kv = kv_.find(it.key);
            std::string cur = (kv != kv_.end()) ? kv->second : "";
            if (cur != it.val) {
                txns_[txid].state = TxnEntry::Aborted;
                return "MISMATCH";
            }
        }
        std::vector<std::pair<std::string, std::string>> ws;
        for (auto& it : items)
            if (!it.compare) ws.emplace_back(it.key, it.val);
        applyWrites(ws);
        txns_[txid].state = TxnEntry::Committed;
        return "COMMITTED";
    }
    if (cmd.rfind("TXPREP ", 0) == 0) {
        auto t = splitSpaces(cmd);
        std::vector<TxItem> items;
        if (t.size() != 3 || !parseTxBody(t[2], items)) return "ERR_BAD";
        const std::string& txid = t[1];
        auto prev = txns_.find(txid);
        if (prev != txns_.end()) return stateName(prev->second.state, true); // idempotent; also the abort FENCE
        std::vector<std::string> keys;
        for (auto& it : items)
            if (std::find(keys.begin(), keys.end(), it.key) == keys.end()) keys.push_back(it.key);
        if (!ablateNoLock_)
            for (auto& k : keys)
                if (const std::string* h = lockHolder(k)) return "LOCKED " + *h; // no-wait: caller aborts and retries
        for (auto& it : items) {
            if (!it.compare) continue;
            auto kv = kv_.find(it.key);
            std::string cur = (kv != kv_.end()) ? kv->second : "";
            if (cur != it.val) return "MISMATCH";
        }
        TxnEntry ent;
        ent.state = TxnEntry::Prepared;
        for (auto& it : items)
            if (!it.compare) ent.writes.emplace_back(it.key, it.val);
        if (!ablateNoLock_) {
            for (auto& k : keys) locks_[k] = txid;
            ent.lockedKeys = keys;
        }
        txns_[txid] = std::move(ent);
        return "PREPARED";
    }
    if (cmd.rfind("TXCOMMIT ", 0) == 0) {
        auto t = splitSpaces(cmd);
        if (t.size() != 2) return "ERR_BAD";
        auto it = txns_.find(t[1]);
        if (it == txns_.end()) return "ERR_UNKNOWN";
        TxnEntry& ent = it->second;
        if (ent.state == TxnEntry::Committed) return "OK";
        if (ent.state == TxnEntry::Aborted) return "ERR_ABORTED";
        applyWrites(ent.writes);
        for (auto& k : ent.lockedKeys) {
            auto l = locks_.find(k);
            if (l != locks_.end() && l->second == t[1]) locks_.erase(l);
        }
        ent.state = TxnEntry::Committed;
        ent.writes.clear();
        ent.lockedKeys.clear();
        return "OK";
    }
    if (cmd.rfind("TXABORT ", 0) == 0) {
        auto t = splitSpaces(cmd);
        if (t.size() != 2) return "ERR_BAD";
        auto it = txns_.find(t[1]);
        if (it == txns_.end()) {
            // Never saw a PREPARE for this txn. Remember the abort anyway, so a
            // PREPARE that arrives LATER (a slow coordinator racing a resolver)
            // is refused instead of taking locks nobody will ever release.
            if (!ablateNoFence_) txns_[t[1]].state = TxnEntry::Aborted;
            return "OK";
        }
        TxnEntry& ent = it->second;
        if (ent.state == TxnEntry::Committed) return "ERR_COMMITTED";
        if (ent.state == TxnEntry::Prepared) {
            for (auto& k : ent.lockedKeys) {
                auto l = locks_.find(k);
                if (l != locks_.end() && l->second == t[1]) locks_.erase(l);
            }
            ent.state = TxnEntry::Aborted;
            ent.writes.clear();
            ent.lockedKeys.clear();
        }
        return "OK";
    }
    if (cmd.rfind("TXBEGIN ", 0) == 0) {
        // Creates the DECISION RECORD (state Pending) on the coordinator shard.
        auto t = splitSpaces(cmd);
        if (t.size() != 3) return "ERR_BAD";
        auto it = recs_.find(t[1]);
        if (it == recs_.end()) {
            TxnRecord r;
            r.participants = t[2];
            recs_[t[1]] = r;
            return "PENDING " + t[2];
        }
        static const char* names[] = {"PENDING", "COMMITTED", "ABORTED"};
        return std::string(names[it->second.state]) + " " + it->second.participants;
    }
    if (cmd.rfind("TXDECIDE ", 0) == 0) {
        // The commit point. First decision wins; every later TXDECIDE, from
        // anyone, just reads it back. That single linearization point is what
        // makes a coordinator crash survivable.
        auto t = splitSpaces(cmd);
        if (t.size() != 3 || (t[2] != "COMMIT" && t[2] != "ABORT")) return "ERR_BAD";
        static const char* names[] = {"PENDING", "COMMITTED", "ABORTED"};
        auto it = recs_.find(t[1]);
        if (it == recs_.end()) {
            if (t[2] == "COMMIT") return "NOREC";
            TxnRecord r;
            r.state = TxnRecord::Aborted; // fence: the real coordinator can no longer commit
            r.participants = "-";
            recs_[t[1]] = r;
            return "ABORTED -";
        }
        if (it->second.state == TxnRecord::Pending)
            it->second.state = (t[2] == "COMMIT") ? TxnRecord::Committed : TxnRecord::Aborted;
        return std::string(names[it->second.state]) + " " + it->second.participants;
    }

    // Empty/no-op entries and reads (GET/KEYS/TXPENDING never enter the log) need no action.
    return "";
}

// ---------------------------------------------------------------------
// State machine <-> bytes. std::map iteration order is the key order, so the
// bytes are deterministic: two replicas that applied the same log prefix produce
// IDENTICAL bytes, which is what makes the INFO digest a byte-for-byte replica
// comparison (and what a snapshot is made of).
std::string RaftNode::serializeState() const {
    ByteWriter w;
    w.putU32(static_cast<uint32_t>(kv_.size()));
    for (auto& kv : kv_) { w.putStr(kv.first); w.putStr(kv.second); }
    w.putU32(static_cast<uint32_t>(locks_.size()));
    for (auto& l : locks_) { w.putStr(l.first); w.putStr(l.second); }
    w.putU32(static_cast<uint32_t>(txns_.size()));
    for (auto& t : txns_) {
        w.putStr(t.first);
        w.putU8(static_cast<uint8_t>(t.second.state));
        w.putU32(static_cast<uint32_t>(t.second.writes.size()));
        for (auto& wr : t.second.writes) { w.putStr(wr.first); w.putStr(wr.second); }
        w.putU32(static_cast<uint32_t>(t.second.lockedKeys.size()));
        for (auto& k : t.second.lockedKeys) w.putStr(k);
    }
    w.putU32(static_cast<uint32_t>(recs_.size()));
    for (auto& r : recs_) {
        w.putStr(r.first);
        w.putU8(static_cast<uint8_t>(r.second.state));
        w.putStr(r.second.participants);
    }
    return w.bytes();
}

bool RaftNode::restoreState(const std::string& blob) {
    std::map<std::string, std::string> kv, locks;
    std::map<std::string, TxnEntry> txns;
    std::map<std::string, TxnRecord> recs;
    try {
        ByteReader r(blob);
        for (uint32_t n = r.getU32(); n > 0; n--) { std::string k = r.getStr(); kv[k] = r.getStr(); }
        for (uint32_t n = r.getU32(); n > 0; n--) { std::string k = r.getStr(); locks[k] = r.getStr(); }
        for (uint32_t n = r.getU32(); n > 0; n--) {
            std::string id = r.getStr();
            TxnEntry e;
            e.state = r.getU8();
            for (uint32_t m = r.getU32(); m > 0; m--) { std::string k = r.getStr(); e.writes.emplace_back(k, r.getStr()); }
            for (uint32_t m = r.getU32(); m > 0; m--) e.lockedKeys.push_back(r.getStr());
            txns[id] = std::move(e);
        }
        for (uint32_t n = r.getU32(); n > 0; n--) {
            std::string id = r.getStr();
            TxnRecord rec;
            rec.state = r.getU8();
            rec.participants = r.getStr();
            recs[id] = std::move(rec);
        }
        if (!r.atEnd()) return false;
    } catch (const std::exception&) {
        return false;
    }
    kv_.swap(kv);
    locks_.swap(locks);
    txns_.swap(txns);
    recs_.swap(recs);
    applyResults_.clear();
    return true;
}

// ---------------------------------------------------------------------
// Startup: snapshot, then WAL, then reconcile the two. Raft keeps commitIndex
// and lastApplied in memory only, and so do we: after a restart everything the
// snapshot covers is known-committed, and the rest of the log is re-applied once
// a leader tells us how much of it is committed (or we become that leader and
// commit our no-op).
bool RaftNode::recover(std::string& err) {
    log_.clear();
    log_.push_back(LogEntry{0, ""}); // sentinel for index 0
    logBase_ = 0;
    if (!persist_) return true;

    const auto t0 = std::chrono::steady_clock::now();
    if (!ensureDir(dataDir_)) {
        err = "cannot create data dir " + dataDir_ + ": " + std::strerror(errno);
        return false;
    }
    SnapshotFile snap;
    std::string serr;
    SnapLoad sl = loadSnapshotFile(dataDir_, snap, serr);
    if (sl == SnapLoad::Corrupt) {
        // The log before the snapshot is gone; guessing would silently lose
        // committed data. Refuse to start and let a human decide.
        err = "snapshot is corrupt (" + serr + "); refusing to start";
        return false;
    }
    if (sl == SnapLoad::Ok) {
        if (!restoreState(snap.state)) {
            err = "snapshot state does not parse; refusing to start";
            return false;
        }
        snapIndex_ = snap.index;
        snapTerm_ = snap.term;
        snapBlob_ = std::move(snap.state);
    }

    Wal::Replayed rep;
    if (!wal_.open(dataDir_, fsync_, rep, err)) return false;
    currentTerm_ = rep.term;
    votedFor_ = rep.votedFor;
    logBase_ = rep.base;
    log_[0].term = rep.baseTerm;
    for (auto& e : rep.entries) log_.push_back(e);
    recoveredTorn_ = rep.tornTail;
    recoveredRecords_ = rep.records;

    bool needRewrite = false;
    if (snapIndex_ > 0) {
        if (snapIndex_ < logBase_) {
            err = "WAL starts after the snapshot (base " + std::to_string(logBase_) + " > snapshot " +
                  std::to_string(snapIndex_) + "): entries in between are missing; refusing to start";
            return false;
        }
        // A snapshot installed from a leader can be newer than this node's whole
        // log (we crashed between writing it and rewriting the WAL), or the log
        // can hold a different history at that index. The snapshot is committed
        // state, so it wins; whatever the log had beyond it was never committed
        // (and a leader will resend it if it was).
        if (snapIndex_ > lastLogIndex() || termAt(snapIndex_) != snapTerm_) {
            log_.clear();
            log_.push_back(LogEntry{snapTerm_, ""});
            logBase_ = snapIndex_;
            needRewrite = true;
        }
    }
    commitIndex_ = lastApplied_ = snapIndex_;
    recoveredEntries_ = log_.size() - 1;
    if (needRewrite) rewriteWalFromMemory();
    recoveryMs_ = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                            std::chrono::steady_clock::now() - t0).count());
    log("RECOVERED from disk: term=" + std::to_string(currentTerm_) + " votedFor=" + std::to_string(votedFor_) +
        " snapshot@" + std::to_string(snapIndex_) + " log=(" + std::to_string(logBase_) + "," +
        std::to_string(lastLogIndex()) + "] records=" + std::to_string(rep.records) +
        (rep.tornTail ? " TORN-TAIL-CUT(" + rep.tornReason + ", " + std::to_string(rep.fileBytes - rep.validBytes) +
                            " bytes dropped)" : "") +
        " in " + std::to_string(recoveryMs_) + " ms");
    return true;
}

// ---------------------------------------------------------------------
// Snapshots and log compaction.
void RaftNode::rewriteWalFromMemory() {
    std::vector<LogEntry> entries(log_.begin() + 1, log_.end());
    auto hook = [this](const char* n) { crashPoint(n); };
    if (!wal_.rewrite(logBase_, log_[0].term, persistVote_ ? currentTerm_ : 0, persistVote_ ? votedFor_ : -1,
                      entries, hook))
        walFatal("rewrite");
}

void RaftNode::maybeSnapshot() {
    if (!persist_ || snapEvery_ == 0 || lastApplied_ < snapIndex_ + snapEvery_) return;
    takeSnapshot();
}

void RaftNode::takeSnapshot() {
    const uint64_t S = lastApplied_, T = termAt(S);
    SnapshotFile sf;
    sf.index = S;
    sf.term = T;
    sf.state = serializeState();
    std::string err;
    auto hook = [this](const char* n) { crashPoint(n); };
    if (!writeSnapshotFile(dataDir_, sf, fsync_, hook, err)) {
        log("FATAL: snapshot write failed: " + err);
        ::_exit(2);
    }
    crashPoint("snap-written"); // snapshot durable, WAL still holds the whole old log
    if (!fsync_) snapUnsynced_ = true;
    snapIndex_ = S;
    snapTerm_ = T;
    snapBlob_ = std::move(sf.state);
    snapsTaken_++;

    const uint64_t newBase = S > snapKeep_ ? S - snapKeep_ : 0;
    if (newBase > logBase_) {
        const uint64_t baseTerm = termAt(newBase);
        std::vector<LogEntry> rest(log_.begin() + static_cast<long>(newBase - logBase_) + 1, log_.end());
        log_.clear();
        log_.push_back(LogEntry{baseTerm, ""});
        log_.insert(log_.end(), rest.begin(), rest.end());
        logBase_ = newBase;
        rewriteWalFromMemory();
    }
    crashPoint("compact-done");
    log("SNAPSHOT at index " + std::to_string(S) + " (" + std::to_string(snapBlob_.size()) + " bytes); log now (" +
        std::to_string(logBase_) + "," + std::to_string(lastLogIndex()) + "], wal " + std::to_string(wal_.size()) +
        " bytes");
}

// Leader -> follower whose nextIndex fell below the compacted prefix.
std::string RaftNode::handleInstallSnapshotMsg(const std::string& payload) {
    ByteReader r(payload);
    r.getU8();
    uint64_t term = r.getU64();
    uint32_t leaderId = r.getU32();
    uint64_t sIdx = r.getU64();
    uint64_t sTerm = r.getU64();
    std::string blob = r.getStr();

    std::lock_guard<std::mutex> lk(mu_);
    bool success = false;
    if (term >= currentTerm_) {
        if (term > currentTerm_ || role_ != Role::Follower) becomeFollower(term);
        else resetElectionDeadline();
        currentLeaderHint_ = static_cast<int32_t>(leaderId);

        if (sIdx <= commitIndex_) {
            success = true; // we already hold committed state at least this new
        } else if (!restoreState(blob)) {
            log("InstallSnapshot: state does not parse; rejecting");
        } else {
            const bool keepSuffix = sIdx >= logBase_ && sIdx <= lastLogIndex() && termAt(sIdx) == sTerm;
            std::vector<LogEntry> rest;
            if (keepSuffix) rest.assign(log_.begin() + static_cast<long>(sIdx - logBase_) + 1, log_.end());
            log_.clear();
            log_.push_back(LogEntry{sTerm, ""});
            log_.insert(log_.end(), rest.begin(), rest.end());
            logBase_ = sIdx;
            snapIndex_ = sIdx;
            snapTerm_ = sTerm;
            snapBlob_ = blob;
            commitIndex_ = lastApplied_ = sIdx;
            snapsInstalled_++;
            if (persist_) {
                SnapshotFile sf;
                sf.index = sIdx;
                sf.term = sTerm;
                sf.state = blob;
                std::string err;
                auto hook = [this](const char* n) { crashPoint(n); };
                if (!writeSnapshotFile(dataDir_, sf, fsync_, hook, err)) {
                    log("FATAL: snapshot write failed: " + err);
                    ::_exit(2);
                }
                if (!fsync_) snapUnsynced_ = true;
                crashPoint("install-snap-written");
                rewriteWalFromMemory();
            }
            log("INSTALLED snapshot from leader " + std::to_string(leaderId) + " at index " + std::to_string(sIdx) +
                (keepSuffix ? " (kept log suffix)" : " (discarded log)"));
            success = true;
        }
    }
    ByteWriter w;
    w.putU8(MSG_INSTALL_SNAPSHOT_RESP);
    w.putU64(currentTerm_);
    w.putU8(success ? 1 : 0);
    return w.bytes();
}

// Node-local status line (answers on ANY node, leader or not). `digest` is a
// hash of the serialized state machine as of `applied`.
std::string RaftNode::infoLine() const {
    char dig[32];
    std::snprintf(dig, sizeof(dig), "%016llx", (unsigned long long)fnv1a64(serializeState()));
    std::string s;
    auto add = [&](const std::string& k, const std::string& v) { s += k + "=" + v + " "; };
    add("id", std::to_string(id_));
    add("role", role_ == Role::Leader ? "leader" : role_ == Role::Candidate ? "candidate" : "follower");
    add("term", std::to_string(currentTerm_));
    add("votedFor", std::to_string(votedFor_));
    add("logBase", std::to_string(logBase_));
    add("lastIndex", std::to_string(lastLogIndex()));
    add("commit", std::to_string(commitIndex_));
    add("applied", std::to_string(lastApplied_));
    add("snapIndex", std::to_string(snapIndex_));
    add("snapBytes", std::to_string(snapBlob_.size()));
    add("snapsTaken", std::to_string(snapsTaken_));
    add("snapsInstalled", std::to_string(snapsInstalled_));
    add("snapsSent", std::to_string(snapsSent_));
    add("keys", std::to_string(kv_.size()));
    add("locks", std::to_string(locks_.size()));
    add("txns", std::to_string(txns_.size()));
    add("recs", std::to_string(recs_.size()));
    add("persist", persist_ ? "1" : "0");
    add("walBytes", std::to_string(persist_ ? wal_.size() : 0));
    add("fsyncs", std::to_string(wal_.fsyncCount()));
    add("fsyncUs", std::to_string(wal_.fsyncMicros()));
    add("recoveredEntries", std::to_string(recoveredEntries_));
    add("recoveredRecords", std::to_string(recoveredRecords_));
    add("recoveryMs", std::to_string(recoveryMs_));
    add("tornTail", recoveredTorn_ ? "1" : "0");
    add("digest", dig);
    return s;
}

// ---------------------------------------------------------------------
void RaftNode::start() {
    running_ = true;
    {
        std::lock_guard<std::mutex> lk(mu_);
        std::string err;
        if (!recover(err)) {
            log("FATAL: cannot recover persistent state: " + err);
            running_ = false; // main() sees isRunning() == false and exits 1
            return;
        }
        resetElectionDeadline();
    }
    log("starting on port " + std::to_string(listenPort_) + " with " +
        std::to_string(peers_.size()) + " peers");
    acceptorThread_ = std::thread(&RaftNode::acceptorLoop, this);
    electionThread_ = std::thread(&RaftNode::electionLoop, this);
    leaderThread_ = std::thread(&RaftNode::leaderLoop, this);
    applierThread_ = std::thread(&RaftNode::applierLoop, this);
}

void RaftNode::stop() {
    if (!running_.exchange(false)) return; // already stopped
    if (listenFd_ >= 0) {
        ::shutdown(listenFd_, SHUT_RDWR);
        ::close(listenFd_);
        listenFd_ = -1;
    }
    cv_.notify_all();
    if (acceptorThread_.joinable()) acceptorThread_.join();
    if (electionThread_.joinable()) electionThread_.join();
    if (leaderThread_.joinable()) leaderThread_.join();
    if (applierThread_.joinable()) applierThread_.join();
    log("stopped");
}

// ---------------------------------------------------------------------
void RaftNode::acceptorLoop() {
    listenFd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    timeval tv{1, 0}; // let accept() wake up periodically to recheck running_
    setsockopt(listenFd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(listenPort_);
    if (::bind(listenFd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        log("FATAL: bind() failed on port " + std::to_string(listenPort_) + ": " +
            std::strerror(errno));
        running_ = false;
        return;
    }
    ::listen(listenFd_, 32);

    while (running_) {
        sockaddr_in cli{};
        socklen_t clen = sizeof(cli);
        int fd = ::accept(listenFd_, reinterpret_cast<sockaddr*>(&cli), &clen);
        if (fd < 0) {
            if (!running_) break;
            continue; // accept() timed out — loop and recheck running_
        }
        std::thread(&RaftNode::handleConnection, this, fd).detach();
    }
}

void RaftNode::handleConnection(int fd) {
    std::string payload;
    if (recvFramed(fd, payload) && !payload.empty()) {
        uint8_t type = static_cast<uint8_t>(payload[0]);
        bool isPeerRpc = (type == MSG_REQUEST_VOTE || type == MSG_APPEND_ENTRIES ||
                          type == MSG_INSTALL_SNAPSHOT);
        // Simulated partition: drop inbound peer RPCs entirely (no response
        // — exactly what a real severed network connection looks like to
        // the caller, which is why callRequestVote/callAppendEntries time
        // out rather than get an explicit rejection). Client traffic is
        // untouched — the whole point is "this node can't reach/be reached
        // by peers, but a client on this side of the partition still can."
        if (isPeerRpc && deafened_.load()) {
            ::close(fd);
            return;
        }
        std::string response;
        switch (type) {
            case MSG_REQUEST_VOTE:
                response = handleRequestVoteMsg(payload);
                break;
            case MSG_APPEND_ENTRIES:
                response = handleAppendEntriesMsg(payload);
                break;
            case MSG_CLIENT_REQUEST:
                response = handleClientRequestMsg(payload);
                break;
            case MSG_INSTALL_SNAPSHOT:
                response = handleInstallSnapshotMsg(payload);
                break;
            default:
                break; // unknown message type: drop the connection with no response
        }
        if (!response.empty()) sendFramed(fd, response);
    }
    ::close(fd);
}

// ---------------------------------------------------------------------
std::string RaftNode::handleRequestVoteMsg(const std::string& payload) {
    ByteReader r(payload);
    r.getU8();
    uint64_t term = r.getU64();
    uint32_t candidateId = r.getU32();
    uint64_t candLastIdx = r.getU64();
    uint64_t candLastTerm = r.getU64();

    std::lock_guard<std::mutex> lk(mu_);
    if (term > currentTerm_) becomeFollower(term);

    bool grant = false;
    if (term == currentTerm_ && (votedFor_ == -1 || votedFor_ == static_cast<int32_t>(candidateId))) {
        uint64_t myLastIdx = lastLogIndex();
        uint64_t myLastTerm = lastLogTerm();
        // §5.4.1 of the Raft paper: candidate's log must be at least as
        // up-to-date as ours — compared by (term, then index).
        bool upToDate = (candLastTerm > myLastTerm) ||
                        (candLastTerm == myLastTerm && candLastIdx >= myLastIdx);
        if (upToDate) {
            grant = true;
            votedFor_ = static_cast<int32_t>(candidateId);
            persistHardState(); // the vote is on disk before the candidate hears it (else a restart could vote twice in this term)
            resetElectionDeadline(); // granting a vote also resets our own clock (§5.2)
            log("voted for node " + std::to_string(candidateId));
        }
    }

    ByteWriter w;
    w.putU8(MSG_REQUEST_VOTE_RESP);
    w.putU64(currentTerm_);
    w.putU8(grant ? 1 : 0);
    return w.bytes();
}

std::string RaftNode::handleAppendEntriesMsg(const std::string& payload) {
    ByteReader r(payload);
    r.getU8();
    uint64_t term = r.getU64();
    uint32_t leaderId = r.getU32();
    uint64_t prevLogIndex = r.getU64();
    uint64_t prevLogTerm = r.getU64();
    uint64_t leaderCommit = r.getU64();
    uint32_t numEntries = r.getU32();
    std::vector<LogEntry> entries;
    entries.reserve(numEntries);
    for (uint32_t i = 0; i < numEntries; i++) {
        LogEntry e;
        e.term = r.getU64();
        e.command = r.getStr();
        entries.push_back(std::move(e));
    }

    std::lock_guard<std::mutex> lk(mu_);
    bool success = false;

    if (term < currentTerm_) {
        // Stale leader — reject; our (higher) term tells it to step down.
    } else {
        if (term > currentTerm_ || role_ != Role::Follower) becomeFollower(term);
        else resetElectionDeadline(); // already a follower this term: still reset the clock

        currentLeaderHint_ = static_cast<int32_t>(leaderId);

        // Index arithmetic with a compacted prefix (logBase_): entries at or below
        // logBase_ are committed and already folded into our snapshot, so whatever
        // the leader says about them is by definition consistent with us.
        bool logOk;
        if (prevLogIndex < logBase_) logOk = true;
        else if (prevLogIndex == logBase_) logOk = (log_[0].term == prevLogTerm);
        else logOk = (prevLogIndex <= lastLogIndex() && termAt(prevLogIndex) == prevLogTerm);
        if (logOk) {
            success = true;
            uint64_t idx = prevLogIndex + 1;
            size_t ei = 0;
            for (; ei < entries.size() && idx <= logBase_; ++ei, ++idx) {} // compacted prefix: skip
            uint64_t truncFrom = 0;
            for (; ei < entries.size(); ++ei, ++idx) {
                if (idx <= lastLogIndex()) {
                    if (termAt(idx) != entries[ei].term) {
                        if (idx <= commitIndex_) {
                            // Raft says this cannot happen. If it ever does, a safety
                            // property is already broken; stop rather than compound it.
                            log("FATAL: leader asked to overwrite COMMITTED index " + std::to_string(idx));
                            ::_exit(3);
                        }
                        truncFrom = idx;
                        log_.resize(idx - logBase_); // conflict: drop this entry and everything after it
                        break;
                    }
                    // identical entry already present — keep scanning
                } else {
                    break; // past the end of our log — remainder are pure appends
                }
            }
            const uint64_t firstNew = lastLogIndex() + 1;
            for (; ei < entries.size(); ++ei) log_.push_back(entries[ei]);

            if (persist_ && (truncFrom != 0 || firstNew <= lastLogIndex())) {
                if (truncFrom != 0 && !wal_.appendTruncate(truncFrom)) walFatal("truncate record");
                persistAppended(firstNew); // appends + one fsync, before we answer success
            }

            if (leaderCommit > commitIndex_) {
                commitIndex_ = std::min(leaderCommit, lastLogIndex());
            }
        }
    }

    ByteWriter w;
    w.putU8(MSG_APPEND_ENTRIES_RESP);
    w.putU64(currentTerm_);
    w.putU8(success ? 1 : 0);
    w.putU64(lastLogIndex());
    return w.bytes();
}

std::string RaftNode::handleClientRequestMsg(const std::string& payload) {
    ByteReader r(payload);
    r.getU8();
    std::string cmd = r.getStr();

    std::unique_lock<std::mutex> lk(mu_);

    if (cmd == "INFO") { // node-local status, answered by any node (never enters the log)
        ClientResult res;
        res.ok = true;
        res.isLeader = (role_ == Role::Leader);
        res.value = infoLine();
        return encodeClientResponse(res);
    }

    if (role_ != Role::Leader) {
        ClientResult res;
        res.ok = false;
        res.isLeader = false;
        res.leaderHint = currentLeaderHint_ >= 0 ? static_cast<uint32_t>(currentLeaderHint_) : 0;
        return encodeClientResponse(res);
    }

    const bool isGet = cmd.rfind("GET ", 0) == 0;
    const bool isKeys = (cmd == "KEYS");
    const bool isPending = (cmd == "TXPENDING"); // prepared-but-undecided txns, for the resolver's sweep
    if (isGet || isKeys || isPending) {
        // Read-index (§8 of the Raft paper), inherited unchanged from
        // Project 11 — see that project's ANALYSIS.md for the full account
        // and for the two real bugs (#7: stale reads after failover, #8: one
        // hung follower stalling every read) that shaped it. Four steps,
        // every one required for linearizability:
        //
        //  0. PRECONDITION: this leader must have committed something in its
        //     OWN term (its no-op, termStartIndex_) before commitIndex_ means
        //     anything — a just-elected leader's commitIndex_ can lag writes
        //     the previous leader already acknowledged.
        //  1. Snapshot readIndex = commitIndex_.
        //  2. Confirm, via a FRESH round trip to a majority, that this node
        //     is still the leader (a silently partitioned old leader would
        //     otherwise serve stale reads forever).
        //  3. Wait for lastApplied_ >= readIndex, then answer from kv_.
        //
        // KEYS (list every key, newline-separated) takes the identical path,
        // so a key listing is exactly as linearizable as a point read. It
        // exists for migration and verification: a shard must be able to say
        // authoritatively which keys it holds.
        uint64_t term = currentTerm_;
        cv_.wait_for(lk, std::chrono::seconds(3), [&] {
            return lastApplied_ >= termStartIndex_ || role_ != Role::Leader ||
                   currentTerm_ != term;
        });
        if (role_ != Role::Leader || currentTerm_ != term || lastApplied_ < termStartIndex_) {
            ClientResult res;
            res.ok = false;
            res.isLeader = (role_ == Role::Leader && currentTerm_ == term);
            res.leaderHint = currentLeaderHint_ >= 0 ? static_cast<uint32_t>(currentLeaderHint_) : 0;
            return encodeClientResponse(res);
        }

        uint64_t readIndex = commitIndex_; // >= termStartIndex_ now, by the wait above
        lk.unlock(); // confirmLeadershipMajority does real network I/O — never hold mu_ across that
        bool confirmed = confirmLeadershipMajority(term);
        lk.lock();

        ClientResult res;
        if (!confirmed || role_ != Role::Leader || currentTerm_ != term) {
            res.ok = false;
            res.isLeader = (role_ == Role::Leader && currentTerm_ == term);
            res.leaderHint = currentLeaderHint_ >= 0 ? static_cast<uint32_t>(currentLeaderHint_) : 0;
            return encodeClientResponse(res);
        }

        cv_.wait_for(lk, std::chrono::seconds(3), [&] {
            return lastApplied_ >= readIndex || role_ != Role::Leader || currentTerm_ != term;
        });

        if (role_ == Role::Leader && currentTerm_ == term && lastApplied_ >= readIndex) {
            res.ok = true;
            res.isLeader = true;
            if (isGet) {
                std::string key = cmd.substr(4);
                auto lk2 = locks_.find(key);
                if (lk2 != locks_.end()) {
                    // A prepared transaction holds this key: the committed value
                    // may be about to change, so don't answer with it. The client
                    // library resolves the transaction and retries.
                    res.value = lockedReply(lk2->second);
                } else {
                    auto it = kv_.find(key);
                    res.value = (it != kv_.end()) ? it->second : "";
                }
            } else if (isPending) {
                std::string all;
                for (auto& t : txns_)
                    if (t.second.state == TxnEntry::Prepared) {
                        all += t.first;
                        all += '\n';
                    }
                res.value = all;
            } else {
                std::string all;
                for (auto& kv : kv_) {
                    all += kv.first;
                    all += '\n';
                }
                res.value = all;
            }
        } else {
            res.ok = false;
            res.isLeader = (role_ == Role::Leader && currentTerm_ == term);
            res.leaderHint = currentLeaderHint_ >= 0 ? static_cast<uint32_t>(currentLeaderHint_) : 0;
        }
        return encodeClientResponse(res);
    }

    // SET / DEL / CAS / CAD all go through the log identically — only how the
    // post-commit result is reported differs (CAS needs its OK/MISMATCH
    // outcome; SET/DEL just need "committed").
    LogEntry e;
    e.term = currentTerm_;
    e.command = cmd;
    log_.push_back(e);
    uint64_t myIndex = lastLogIndex();
    uint64_t myTerm = currentTerm_;
    persistAppended(myIndex); // durable on the leader before leaderLoop may count it toward a majority
    log("appended '" + cmd + "' at index " + std::to_string(myIndex));

    cv_.wait_for(lk, std::chrono::seconds(3), [&] {
        return lastApplied_ >= myIndex || role_ != Role::Leader || currentTerm_ != myTerm;
    });

    ClientResult res;
    if (lastApplied_ >= myIndex && role_ == Role::Leader && currentTerm_ == myTerm) {
        res.ok = true;
        res.isLeader = true;
        auto it = applyResults_.find(myIndex);
        if (it != applyResults_.end()) {
            res.value = it->second; // e.g. CAS's "OK"/"MISMATCH"
            applyResults_.erase(it);
        }
    } else {
        res.ok = false;
        res.isLeader = (role_ == Role::Leader && currentTerm_ == myTerm);
        res.leaderHint = currentLeaderHint_ >= 0 ? static_cast<uint32_t>(currentLeaderHint_) : 0;
    }
    return encodeClientResponse(res);
}

// ---------------------------------------------------------------------
bool RaftNode::callRequestVote(const PeerConfig& peer, uint64_t term, uint32_t candidateId,
                               uint64_t lastLogIdx, uint64_t lastLogTermV, uint64_t& outTerm,
                               bool& outVoteGranted) {
    if (deafened_.load()) return false; // simulated partition: see deafened_
    int fd = connectWithTimeout(peer.host, peer.port, kRpcTimeoutMs);
    if (fd < 0) return false;
    ByteWriter w;
    w.putU8(MSG_REQUEST_VOTE);
    w.putU64(term);
    w.putU32(candidateId);
    w.putU64(lastLogIdx);
    w.putU64(lastLogTermV);
    bool ok = sendFramed(fd, w.bytes());
    std::string resp;
    if (ok) ok = recvFramed(fd, resp);
    ::close(fd);
    if (!ok || resp.empty()) return false;
    ByteReader r(resp);
    r.getU8();
    outTerm = r.getU64();
    outVoteGranted = r.getU8() != 0;
    return true;
}

bool RaftNode::callAppendEntries(const PeerConfig& peer, uint64_t term, uint32_t leaderId,
                                 uint64_t prevLogIndex, uint64_t prevLogTerm,
                                 const std::vector<LogEntry>& entries, uint64_t leaderCommit,
                                 uint64_t& outTerm, bool& outSuccess, uint64_t& outMatchIndex) {
    if (deafened_.load()) return false; // simulated partition: see deafened_
    int fd = connectWithTimeout(peer.host, peer.port, kRpcTimeoutMs);
    if (fd < 0) return false;
    ByteWriter w;
    w.putU8(MSG_APPEND_ENTRIES);
    w.putU64(term);
    w.putU32(leaderId);
    w.putU64(prevLogIndex);
    w.putU64(prevLogTerm);
    w.putU64(leaderCommit);
    w.putU32(static_cast<uint32_t>(entries.size()));
    for (auto& e : entries) {
        w.putU64(e.term);
        w.putStr(e.command);
    }
    bool ok = sendFramed(fd, w.bytes());
    std::string resp;
    if (ok) ok = recvFramed(fd, resp);
    ::close(fd);
    if (!ok || resp.empty()) return false;
    ByteReader r(resp);
    r.getU8();
    outTerm = r.getU64();
    outSuccess = r.getU8() != 0;
    outMatchIndex = r.getU64();
    return true;
}

bool RaftNode::callInstallSnapshot(const PeerConfig& peer, uint64_t term, uint32_t leaderId,
                                   uint64_t snapIdx, uint64_t snapTermV, const std::string& blob,
                                   uint64_t& outTerm, bool& outSuccess) {
    if (deafened_.load()) return false; // simulated partition: see deafened_
    int fd = connectWithTimeout(peer.host, peer.port, kRpcTimeoutMs);
    if (fd < 0) return false;
    ByteWriter w;
    w.putU8(MSG_INSTALL_SNAPSHOT);
    w.putU64(term);
    w.putU32(leaderId);
    w.putU64(snapIdx);
    w.putU64(snapTermV);
    w.putStr(blob);
    bool ok = sendFramed(fd, w.bytes());
    std::string resp;
    if (ok) ok = recvFramed(fd, resp);
    ::close(fd);
    if (!ok || resp.empty()) return false;
    ByteReader r(resp);
    r.getU8();
    outTerm = r.getU64();
    outSuccess = r.getU8() != 0;
    return true;
}

void RaftNode::replicateToPeer(const PeerConfig& peer) {
    uint64_t term, prevLogIndex = 0, prevLogTerm = 0, leaderCommit = 0;
    std::vector<LogEntry> entries;
    bool sendSnapshot = false;
    uint64_t sIdx = 0, sTerm = 0;
    std::string blob;
    {
        std::lock_guard<std::mutex> lk(mu_);
        if (role_ != Role::Leader) return;
        term = currentTerm_;
        uint64_t ni = nextIndex_[peer.id];
        if (ni == 0) ni = 1;
        if (ni > lastLogIndex() + 1) ni = lastLogIndex() + 1;
        if (ni <= logBase_) {
            // The entry before the one this follower needs has been compacted away,
            // so there is no prevLogTerm to send and no entries to send: ship the
            // snapshot instead.
            if (snapIndex_ == 0) return; // cannot happen: logBase_ <= snapIndex_
            sendSnapshot = true;
            sIdx = snapIndex_;
            sTerm = snapTerm_;
            blob = snapBlob_;
        } else {
            prevLogIndex = ni - 1;
            prevLogTerm = termAt(prevLogIndex);
            for (uint64_t i = ni; i <= lastLogIndex(); ++i) entries.push_back(entryAt(i));
            leaderCommit = commitIndex_;
        }
    }

    if (sendSnapshot) {
        uint64_t outTerm = 0;
        bool outSuccess = false;
        if (!callInstallSnapshot(peer, term, id_, sIdx, sTerm, blob, outTerm, outSuccess)) return;
        std::lock_guard<std::mutex> lk(mu_);
        if (outTerm > currentTerm_) {
            becomeFollower(outTerm);
            return;
        }
        if (role_ != Role::Leader || term != currentTerm_) return;
        if (outSuccess) {
            snapsSent_++;
            matchIndex_[peer.id] = std::max(matchIndex_[peer.id], sIdx);
            nextIndex_[peer.id] = std::max(nextIndex_[peer.id], sIdx + 1);
        }
        return;
    }

    uint64_t outTerm = 0, outMatch = 0;
    bool outSuccess = false;
    bool ok = callAppendEntries(peer, term, id_, prevLogIndex, prevLogTerm, entries, leaderCommit,
                                outTerm, outSuccess, outMatch);
    if (!ok) return; // peer unreachable this round — next heartbeat tick will retry

    std::lock_guard<std::mutex> lk(mu_);
    if (outTerm > currentTerm_) {
        becomeFollower(outTerm);
        return;
    }
    if (role_ != Role::Leader || term != currentTerm_) return; // stale response, ignore

    if (outSuccess) {
        uint64_t newMatch = prevLogIndex + entries.size();
        matchIndex_[peer.id] = newMatch;
        nextIndex_[peer.id] = newMatch + 1;
    } else if (nextIndex_[peer.id] > 1) {
        // Log inconsistency. Back up one entry, but never past what the follower
        // says it has: a follower that lost its tail (torn WAL) or its whole log
        // reports a short lastIndex, and walking back one entry per heartbeat from
        // the leader's end would take minutes.
        uint64_t hinted = outMatch + 1;
        uint64_t back = nextIndex_[peer.id] - 1;
        nextIndex_[peer.id] = std::max<uint64_t>(1, std::min(back, hinted));
    }
}

// ---------------------------------------------------------------------
// Read-index: confirms, via a FRESH round-trip right now (not a belief
// left over from the last heartbeat tick up to kHeartbeatIntervalMs ago),
// that a majority of the cluster still considers this node's term valid.
// Deliberately ignores each response's `outSuccess` (log-matching) —
// read-index only cares whether a peer is reachable and agrees this term
// hasn't been superseded, not whether this exact AppendEntries' prevLog
// pointers happened to match that peer's log.
bool RaftNode::confirmLeadershipMajority(uint64_t term) {
    std::vector<PeerConfig> peersCopy;
    uint64_t prevLogIndex, prevLogTerm, leaderCommit;
    {
        std::lock_guard<std::mutex> lk(mu_);
        if (role_ != Role::Leader || currentTerm_ != term) return false;
        peersCopy = peers_;
        prevLogIndex = lastLogIndex();
        prevLogTerm = lastLogTerm();
        leaderCommit = commitIndex_;
    }
    if (peersCopy.empty()) return true; // single-node cluster: trivially a majority of one

    const int majority = static_cast<int>(peersCopy.size() + 1) / 2 + 1;
    const int numPeers = static_cast<int>(peersCopy.size());

    // Shared by this call and the per-peer threads. Heap-allocated and held
    // by shared_ptr because we deliberately do NOT wait for every peer: as
    // soon as a majority has acked we return, and the stragglers finish on
    // their own (detached) and just write into this state, which must
    // outlive us. Found by measurement — the first version joined every
    // peer thread, so ONE hung follower (SIGSTOP'd: kernel accepts the
    // connection, process never answers) cost every read the full 600ms RPC
    // timeout even though 4 of 5 nodes had answered instantly. See
    // ANALYSIS.md.
    struct Round {
        std::mutex m;
        std::condition_variable cv;
        int acks = 1;       // this node counts itself
        int responded = 0;  // peers that have finished (acked, failed, or higher-term)
        bool sawHigherTerm = false;
        uint64_t higherTerm = 0;
    };
    auto round = std::make_shared<Round>();

    for (auto& p : peersCopy) {
        std::thread([this, p, term, prevLogIndex, prevLogTerm, leaderCommit, round]() {
            uint64_t outTerm = 0, outMatch = 0;
            bool outSuccess = false;
            std::vector<LogEntry> noEntries; // a pure heartbeat: no new log entries
            bool ok = callAppendEntries(p, term, id_, prevLogIndex, prevLogTerm, noEntries,
                                        leaderCommit, outTerm, outSuccess, outMatch);
            {
                std::lock_guard<std::mutex> g(round->m);
                round->responded++;
                if (ok) {
                    if (outTerm > term) {
                        round->sawHigherTerm = true;
                        round->higherTerm = outTerm;
                    } else {
                        round->acks++;
                    }
                }
            }
            round->cv.notify_all();
        }).detach();
    }

    int acks;
    bool sawHigherTerm;
    uint64_t higherTerm;
    {
        std::unique_lock<std::mutex> g(round->m);
        round->cv.wait(g, [&] {
            return round->acks >= majority || round->sawHigherTerm ||
                   round->responded == numPeers;
        });
        acks = round->acks;
        sawHigherTerm = round->sawHigherTerm;
        higherTerm = round->higherTerm;
    } // release round->m before taking mu_ — never hold both

    std::lock_guard<std::mutex> lk(mu_);
    if (sawHigherTerm) {
        if (higherTerm > currentTerm_) becomeFollower(higherTerm);
        return false;
    }
    if (role_ != Role::Leader || currentTerm_ != term) return false;
    return acks >= majority;
}

// ---------------------------------------------------------------------
void RaftNode::electionLoop() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(kElectionPollMs));
        if (!running_) break;

        std::unique_lock<std::mutex> lk(mu_);
        if (role_ == Role::Leader) continue; // leaders don't run elections on themselves
        auto now = std::chrono::steady_clock::now();
        if (now - lastHeartbeat_ < electionTimeout_) continue;

        becomeCandidate();
        uint64_t term = currentTerm_;
        uint64_t lastIdx = lastLogIndex();
        uint64_t lastTerm = lastLogTerm();
        auto peersCopy = peers_;
        lk.unlock();

        std::atomic<int> votes{1}; // vote for self
        std::vector<std::thread> ths;
        for (auto& p : peersCopy) {
            ths.emplace_back([this, p, term, lastIdx, lastTerm, &votes]() {
                uint64_t rterm = 0;
                bool granted = false;
                if (callRequestVote(p, term, id_, lastIdx, lastTerm, rterm, granted)) {
                    std::lock_guard<std::mutex> lk2(mu_);
                    if (rterm > currentTerm_) {
                        becomeFollower(rterm);
                    } else if (granted && role_ == Role::Candidate && currentTerm_ == term) {
                        votes++;
                    }
                }
            });
        }
        for (auto& t : ths) t.join();

        std::lock_guard<std::mutex> lk3(mu_);
        if (role_ == Role::Candidate && currentTerm_ == term) {
            int majority = static_cast<int>(peersCopy.size() + 1) / 2 + 1;
            if (votes >= majority) {
                becomeLeader();
            }
            // else: stay candidate; the next timeout (already freshly randomized
            // by becomeCandidate) will trigger a retry with an incremented term.
        }
    }
}

void RaftNode::leaderLoop() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(kHeartbeatIntervalMs));
        if (!running_) break;

        std::unique_lock<std::mutex> lk(mu_);
        if (role_ != Role::Leader) continue;
        auto peersCopy = peers_;
        lk.unlock();

        std::vector<std::thread> ths;
        for (auto& p : peersCopy) ths.emplace_back(&RaftNode::replicateToPeer, this, p);
        for (auto& t : ths) t.join();

        std::lock_guard<std::mutex> lk2(mu_);
        if (role_ != Role::Leader) continue;
        // §5.3/5.4: advance commitIndex to the highest N with a majority in
        // matchIndex AND log_[N].term == currentTerm_ (never commit an older
        // term's entry purely by count — only via replicating a new entry).
        for (uint64_t N = lastLogIndex(); N > commitIndex_; --N) {
            if (termAt(N) != currentTerm_) continue;
            int count = 1; // leader counts itself
            for (auto& p : peers_)
                if (matchIndex_[p.id] >= N) count++;
            if (count >= static_cast<int>(peers_.size() + 1) / 2 + 1) {
                commitIndex_ = N;
                break;
            }
        }
    }
}

void RaftNode::applierLoop() {
    while (running_) {
        std::unique_lock<std::mutex> lk(mu_);
        cv_.wait_for(lk, std::chrono::milliseconds(50),
                     [&] { return !running_ || lastApplied_ < commitIndex_; });
        bool advanced = false;
        while (lastApplied_ < commitIndex_) {
            lastApplied_++;
            std::string result = applyToStateMachine(entryAt(lastApplied_));
            if (!result.empty()) applyResults_[lastApplied_] = result;
            advanced = true;
        }
        if (advanced) maybeSnapshot(); // under mu_: the state machine is quiescent while we serialize it
        lk.unlock();
        if (advanced) cv_.notify_all(); // wake any client handlers waiting on their index
    }
}

} // namespace raft
