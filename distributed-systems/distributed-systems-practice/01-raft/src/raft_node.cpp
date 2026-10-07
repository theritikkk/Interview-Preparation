#include "raft_node.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
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
    // indefinitely. The standard fix is to have every new leader append a
    // no-op entry stamped with its OWN term immediately on election; once
    // that entry replicates to a majority, the Log Matching Property commits
    // it *and* every entry before it in the same step.
    log_.push_back(LogEntry{currentTerm_, ""});
    termStartIndex_ = lastLogIndex();
}

void RaftNode::applyToStateMachine(const LogEntry& e) {
    const std::string& cmd = e.command;
    if (cmd.rfind("SET ", 0) == 0) {
        std::string rest = cmd.substr(4);
        auto sp = rest.find(' ');
        if (sp != std::string::npos) {
            kv_[rest.substr(0, sp)] = rest.substr(sp + 1);
        }
    }
    // Empty/no-op entries and GETs (GETs never enter the log) need no action.
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
        // §8 of the Raft paper: a brand-new leader may already hold
        // older, majority-replicated entries in its log (that's exactly
        // what the no-op in becomeLeader() is for), but it must not serve
        // reads from kv_ until it has actually APPLIED everything that was
        // committed up through its own election — otherwise it can answer
        // a GET with stale data for a key an earlier leader already
        // committed, simply because this node hasn't caught up yet. We
        // wait for lastApplied_ to reach termStartIndex_ (the no-op's own
        // index) as a cheap, sufficient proxy for "caught up enough to
        // answer safely" — the same condition SET already waits on below.
        //
        // This is still NOT linearizable as written beyond that one
        // guarantee — see ANALYSIS.md for why (no full read-index/lease
        // check against a fresh majority on every single read), which is
        // an intentional, documented simplification for this exercise.
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
    }

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
            applyToStateMachine(log_[lastApplied_]);
            advanced = true;
        }
        lk.unlock();
        if (advanced) cv_.notify_all(); // wake any client handlers waiting on their index
    }
}

} // namespace raft
