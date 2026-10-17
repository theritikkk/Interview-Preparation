#include "raft_node.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
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
    : id_(id), listenPort_(listenPort), peers_(std::move(peers)), logPrefix_(std::move(logPrefix)) {}

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

std::string RaftNode::applyToStateMachine(const LogEntry& e) {
    const std::string& cmd = e.command;
    if (cmd.rfind("SET ", 0) == 0) {
        std::string rest = cmd.substr(4);
        auto sp = rest.find(' ');
        if (sp != std::string::npos) {
            kv_[rest.substr(0, sp)] = rest.substr(sp + 1);
        }
        return "";
    }
    if (cmd.rfind("DEL ", 0) == 0) {
        std::string key = cmd.substr(4);
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

        auto it = kv_.find(key);
        std::string current = (it != kv_.end()) ? it->second : "";
        if (current != expected) return "MISMATCH";
        kv_[key] = newVal;
        return "OK";
    }
    // Empty/no-op entries and GETs (GETs never enter the log) need no action.
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

    if (cmd.rfind("GET ", 0) == 0) {
#ifdef RAFT_UNSAFE_READS
        // Project 1's ORIGINAL check, preserved verbatim and compiled into
        // a separate `raft_node_unsafe` binary (see CMakeLists.txt) purely
        // so ANALYSIS.md's partition demo can run the identical scenario
        // against both binaries and show the actual difference, not just
        // describe it. This check is purely LOCAL — it never contacts
        // another node — which is exactly its flaw: a leader that's been
        // silently partitioned away from the rest of the cluster keeps
        // passing it forever, because nothing here depends on the rest of
        // the cluster still agreeing this node leads. See ANALYSIS.md.
        uint64_t term = currentTerm_;
        cv_.wait_for(lk, std::chrono::seconds(3), [&] {
            return lastApplied_ >= termStartIndex_ || role_ != Role::Leader ||
                   currentTerm_ != term;
        });
        ClientResult res;
        if (role_ == Role::Leader && currentTerm_ == term && lastApplied_ >= termStartIndex_) {
            std::string key = cmd.substr(4);
            res.ok = true;
            res.isLeader = true;
            auto it = kv_.find(key);
            res.value = (it != kv_.end()) ? it->second : "";
        } else {
            res.ok = false;
            res.isLeader = (role_ == Role::Leader && currentTerm_ == term);
            res.leaderHint = currentLeaderHint_ >= 0 ? static_cast<uint32_t>(currentLeaderHint_) : 0;
        }
        return encodeClientResponse(res);
#else
        // Read-index (§8 of the Raft paper, the full version — see
        // ANALYSIS.md for exactly what the weaker, RAFT_UNSAFE_READS
        // version above gets wrong and why it matters). Four steps,
        // every one of them required for linearizability, not just the
        // first:
        //
        //  0. PRECONDITION: this leader must have committed something in
        //     its OWN term (its no-op, termStartIndex_) before commitIndex_
        //     means anything. A just-elected leader's commitIndex_ can lag
        //     writes the previous leader already acknowledged to clients
        //     (followers only learn a commit on the NEXT heartbeat, and a
        //     leader can't advance over old-term entries by counting), so
        //     snapshotting commitIndex_ too early yields a read point that
        //     is behind acknowledged writes. The first version of this
        //     code skipped this step and returned stale data after 20 of
        //     20 forced failovers (scripts/failover_read_test.sh;
        //     ANALYSIS.md bug #7) — the partition check below does not
        //     catch it, because a freshly elected leader legitimately
        //     passes the majority check.
        //  1. Snapshot readIndex = commitIndex_ right now: once we've
        //     applied up through this index, the GET is at least as fresh
        //     as every write any client could plausibly have observed as
        //     committed before issuing this read.
        //  2. Confirm — via a FRESH round-trip to a majority, not a
        //     memory of the last heartbeat tick — that this node is still
        //     really the leader. A node that's been silently partitioned
        //     away from the cluster (see deafened_) can go on believing
        //     it's the leader indefinitely otherwise, and would serve
        //     stale reads forever; this step is exactly what stops that.
        //  3. Wait for lastApplied_ to actually reach readIndex before
        //     answering from kv_ (steps 1-2 alone don't guarantee this
        //     node's local state machine has caught up yet).
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
            std::string key = cmd.substr(4);
            res.ok = true;
            res.isLeader = true;
            auto it = kv_.find(key);
            res.value = (it != kv_.end()) ? it->second : "";
        } else {
            res.ok = false;
            res.isLeader = (role_ == Role::Leader && currentTerm_ == term);
            res.leaderHint = currentLeaderHint_ >= 0 ? static_cast<uint32_t>(currentLeaderHint_) : 0;
        }
        return encodeClientResponse(res);
#endif
    }

    // SET / DEL / CAS all go through the log identically — only how the
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
