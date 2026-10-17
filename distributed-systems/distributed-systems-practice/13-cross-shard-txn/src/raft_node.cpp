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
                   std::string logPrefix)
    : id_(id), listenPort_(listenPort), peers_(std::move(peers)), logPrefix_(std::move(logPrefix)) {
    if (const char* a = std::getenv("RAFT_TXN_ABLATE")) {
        std::string v = a;
        ablateNoLock_ = (v.find("nolock") != std::string::npos);
        ablateNoFence_ = (v.find("nofence") != std::string::npos);
    }
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

uint64_t RaftNode::lastLogIndex() const { return log_.size() - 1; }
uint64_t RaftNode::lastLogTerm() const { return log_[lastLogIndex()].term; }

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
void RaftNode::start() {
    running_ = true;
    {
        std::lock_guard<std::mutex> lk(mu_);
        log_.push_back(LogEntry{0, ""}); // index-0 sentinel
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
        bool isPeerRpc = (type == MSG_REQUEST_VOTE || type == MSG_APPEND_ENTRIES);
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

        bool logOk = (prevLogIndex == 0) ||
                     (prevLogIndex < log_.size() && log_[prevLogIndex].term == prevLogTerm);
        if (logOk) {
            success = true;
            uint64_t idx = prevLogIndex + 1;
            size_t ei = 0;
            for (; ei < entries.size(); ++ei, ++idx) {
                if (idx < log_.size()) {
                    if (log_[idx].term != entries[ei].term) {
                        log_.resize(idx); // conflict: drop this entry and everything after it
                        break;
                    }
                    // identical entry already present — keep scanning
                } else {
                    break; // past the end of our log — remainder are pure appends
                }
            }
            for (; ei < entries.size(); ++ei) log_.push_back(entries[ei]);

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

void RaftNode::replicateToPeer(const PeerConfig& peer) {
    uint64_t term, prevLogIndex, prevLogTerm, leaderCommit;
    std::vector<LogEntry> entries;
    {
        std::lock_guard<std::mutex> lk(mu_);
        if (role_ != Role::Leader) return;
        term = currentTerm_;
        uint64_t ni = nextIndex_[peer.id];
        if (ni == 0) ni = 1;
        prevLogIndex = ni - 1;
        prevLogTerm = (prevLogIndex < log_.size()) ? log_[prevLogIndex].term : 0;
        for (uint64_t i = ni; i < log_.size(); ++i) entries.push_back(log_[i]);
        leaderCommit = commitIndex_;
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
        nextIndex_[peer.id]--; // log inconsistency: back up one and retry next round
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
            if (log_[N].term != currentTerm_) continue;
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
            std::string result = applyToStateMachine(log_[lastApplied_]);
            if (!result.empty()) applyResults_[lastApplied_] = result;
            advanced = true;
        }
        lk.unlock();
        if (advanced) cv_.notify_all(); // wake any client handlers waiting on their index
    }
}

} // namespace raft
