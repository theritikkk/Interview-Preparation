// raft_node.h — a teaching implementation of the Raft consensus algorithm
// (Ongaro & Ousterhout, "In Search of an Understandable Consensus
// Algorithm"). See ../ANALYSIS.md for exactly which parts of the paper
// this does and does not implement.
//
// Design: one OS process per Raft node, communicating over plain TCP with
// the framing in wire.h. Each RPC is a fresh, short-lived connection
// (connect -> send request -> read response -> close) rather than a
// persistent connection — simpler to reason about, at a throughput cost
// that doesn't matter for a teaching cluster of a handful of nodes.

#pragma once

#include "wire.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace raft {

enum class Role { Follower, Candidate, Leader };

enum MsgType : uint8_t {
    MSG_REQUEST_VOTE = 1,
    MSG_REQUEST_VOTE_RESP = 2,
    MSG_APPEND_ENTRIES = 3,
    MSG_APPEND_ENTRIES_RESP = 4,
    MSG_CLIENT_REQUEST = 5,
    MSG_CLIENT_RESPONSE = 6,
};

struct LogEntry {
    uint64_t term;
    std::string command; // e.g. "SET key value" or "GET key"
};

struct PeerConfig {
    uint32_t id;
    std::string host;
    uint16_t port;
};

// Result of a MSG_CLIENT_REQUEST, shared between the node (building it) and
// the CLI client program (parsing it) — see the encode/decode helpers below.
struct ClientResult {
    bool ok = false;
    bool isLeader = false;
    uint32_t leaderHint = 0; // which node id the client should retry against
    std::string value;       // result of GET, empty for SET
};

inline std::string encodeClientRequest(const std::string& cmd) {
    ByteWriter w;
    w.putU8(MSG_CLIENT_REQUEST);
    w.putStr(cmd);
    return w.bytes();
}

inline std::string encodeClientResponse(const ClientResult& res) {
    ByteWriter w;
    w.putU8(MSG_CLIENT_RESPONSE);
    w.putU8(res.ok ? 1 : 0);
    w.putU8(res.isLeader ? 1 : 0);
    w.putU32(res.leaderHint);
    w.putStr(res.value);
    return w.bytes();
}

inline ClientResult decodeClientResponse(const std::string& payload) {
    ByteReader r(payload);
    r.getU8(); // message type, already known by the caller
    ClientResult res;
    res.ok = r.getU8() != 0;
    res.isLeader = r.getU8() != 0;
    res.leaderHint = r.getU32();
    res.value = r.getStr();
    return res;
}

class RaftNode {
public:
    RaftNode(uint32_t id, uint16_t listenPort, std::vector<PeerConfig> peers,
             std::string logPrefix);
    ~RaftNode();

    void start();
    void stop();

private:
    // ---- persistent state (kept in memory only — see ANALYSIS.md) ----
    uint32_t id_;
    uint16_t listenPort_;
    std::vector<PeerConfig> peers_;

    uint64_t currentTerm_ = 0;
    int32_t votedFor_ = -1;      // -1 == no vote cast this term
    std::vector<LogEntry> log_;  // log_[0] is a dummy sentinel (term 0); real entries start at index 1

    // ---- volatile state, all servers ----
    uint64_t commitIndex_ = 0;
    uint64_t lastApplied_ = 0;
    Role role_ = Role::Follower;
    int32_t currentLeaderHint_ = -1; // last node id we saw act as a valid leader

    // ---- volatile state, leaders only ----
    std::map<uint32_t, uint64_t> nextIndex_;
    std::map<uint32_t, uint64_t> matchIndex_;
    // Index of the no-op entry this leader appended on election (see
    // becomeLeader() / ANALYSIS.md). A GET must not answer from kv_ until
    // lastApplied_ reaches this index, otherwise a brand-new leader can
    // serve a stale read for data that an earlier leader already committed
    // but that this node hasn't caught up to applying yet.
    uint64_t termStartIndex_ = 0;

    // ---- replicated state machine: a toy key/value store ----
    std::map<std::string, std::string> kv_;

    // ---- concurrency / timing ----
    mutable std::mutex mu_;
    std::condition_variable cv_; // signaled whenever lastApplied_ advances
    std::atomic<bool> running_{false};
    std::chrono::steady_clock::time_point lastHeartbeat_;
    std::chrono::milliseconds electionTimeout_{0}; // randomized, reset on every heartbeat/vote

    std::thread acceptorThread_;
    std::thread electionThread_;
    std::thread leaderThread_;
    std::thread applierThread_;
    int listenFd_ = -1;

    std::string logPrefix_;

    // ---- small helpers (all assume mu_ is already held unless noted) ----
    void log(const std::string& msg) const;
    uint64_t lastLogIndex() const;
    uint64_t lastLogTerm() const;
    void becomeFollower(uint64_t newTerm);
    void becomeCandidate();
    void becomeLeader();
    void resetElectionDeadline();
    void applyToStateMachine(const LogEntry& e);

    // background loops (each owns its own thread, locks mu_ internally)
    void acceptorLoop();
    void electionLoop();
    void leaderLoop();
    void applierLoop();

    // per-connection dispatch — NOT holding mu_ on entry
    void handleConnection(int fd);
    std::string handleRequestVoteMsg(const std::string& payload);
    std::string handleAppendEntriesMsg(const std::string& payload);
    std::string handleClientRequestMsg(const std::string& payload);

    // outbound RPCs (candidate/leader side) — NOT holding mu_ while blocked on network I/O
    bool callRequestVote(const PeerConfig& peer, uint64_t term, uint32_t candidateId,
                         uint64_t lastLogIdx, uint64_t lastLogTermV,
                         uint64_t& outTerm, bool& outVoteGranted);
    bool callAppendEntries(const PeerConfig& peer, uint64_t term, uint32_t leaderId,
                           uint64_t prevLogIndex, uint64_t prevLogTerm,
                           const std::vector<LogEntry>& entries, uint64_t leaderCommit,
                           uint64_t& outTerm, bool& outSuccess, uint64_t& outMatchIndex);

    void replicateToPeer(const PeerConfig& peer); // one AppendEntries round, called from leaderLoop
};

} // namespace raft
