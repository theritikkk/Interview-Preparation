// txn_client.h — multi-key, multi-shard transactions over the Raft shards.
//
// A transaction here is a *mini-transaction*: a set of COMPARE items ("key must
// currently equal V", "" meaning absent) and WRITE items ("set key to V", or
// erase it). It commits atomically across any number of shards, or not at all.
// Anything richer — "read two balances, move money between them" — is a
// read-then-commit loop built on top: read, build a transaction whose compares
// are the values you read, retry if it aborts on a mismatch.
//
// Protocol (details and the safety argument are in ANALYSIS.md):
//
//   Single shard      one atomic log entry (TXONE); no locks, no 2PC.
//   Several shards    1. TXBEGIN  on the coordinator shard (lowest participant id):
//                        creates a durable DECISION RECORD, state Pending.
//                     2. TXPREP   on every participant, in parallel: checks the
//                        compares, takes exclusive locks on every key involved
//                        (no-wait: a held lock means "abort and retry"), stages
//                        the writes. All of it replicated through that shard's Raft log.
//                     3. TXDECIDE on the coordinator shard: Pending -> Committed
//                        (every participant said PREPARED) or Aborted. The FIRST
//                        decision wins and is final. THIS is the commit point.
//                     4. TXCOMMIT / TXABORT on every participant: apply or discard
//                        the staged writes and release the locks.
//
// Because the decision lives in a Raft group instead of in the coordinator's
// memory, ANY client can finish a transaction whose coordinator died: it asks
// the record (TXDECIDE ... ABORT, which just reads back a decision that already
// exists, or fences the original coordinator out if there isn't one yet) and
// rolls the participants forward or back. That is resolve().
#pragma once

#include "sharded_client.h"

#include <atomic>
#include <map>
#include <string>
#include <vector>

namespace txn {

// Plain GET/SET/DEL/CAS on a key that a prepared transaction holds answer with
// this marker byte followed by "LOCKED <txid>" instead of a value.
inline constexpr char kLockMarker = '\x01';

struct Item {
    bool compare;       // true: key must equal value ("" == absent); false: write value ("" == erase)
    std::string key;
    std::string value;
};

class Txn {
public:
    Txn& expect(const std::string& key, const std::string& value) { return add(true, key, value); }
    Txn& put(const std::string& key, const std::string& value);
    Txn& erase(const std::string& key) { return add(false, key, ""); }
    const std::vector<Item>& items() const { return items_; }
    // Non-empty if a builder call was refused (bad key, empty or reserved value).
    const std::string& invalid() const { return invalid_; }

private:
    Txn& add(bool compare, const std::string& key, const std::string& value);
    std::vector<Item> items_;
    std::string invalid_;
};

enum class Outcome { Committed, Aborted, Indeterminate };
enum class Reason { None, Mismatch, Conflict, Fenced, Unavailable, Invalid };

struct Result {
    Outcome outcome = Outcome::Indeterminate;
    Reason reason = Reason::None;
    std::string txid;
    bool crossShard = false;
    // The decision is durable, but at least one participant has not been told
    // yet (it was unreachable). Safe: anyone who touches that shard's locked
    // keys will finish the job via resolve().
    bool cleanupPending = false;
    std::string blocker; // for Reason::Conflict: the txid whose lock we hit
    int latencyMs = 0;
    std::string detail;
};

// Where an injected coordinator crash happens (the process _exit()s on the
// spot, exactly like a kill -9: nothing is cleaned up, no later message is sent).
enum class CrashPoint { None, AfterBegin, AfterPrepare, AfterDecide, MidCommit };

// How a stuck participant is finished. GuessAbort is the ABLATION: it ignores
// the decision record and unilaterally aborts the one participant it found
// stuck, which is what a 2PC without a durable decision has to do (or block).
enum class Recovery { UseRecord, GuessAbort };

struct Options {
    int rpcTimeoutMs = 1000;
    int lockTtlMs = 3000;       // a lock whose txn is older than this gets resolved by whoever trips over it
    int maxHelpWaitMs = 10000;  // give up waiting on one stuck key after this long
    CrashPoint crashAt = CrashPoint::None;
    int crashExitCode = 86;
    Recovery recovery = Recovery::UseRecord;
    int pauseAfterPrepareMs = 0; // test hook: stall between PREPARE and the decision, to widen that window
};

class TxnClient {
public:
    // NOT thread-safe (it owns per-shard leader caches); one per thread. It uses
    // threads internally to talk to different shards in parallel.
    TxnClient(std::shared_ptr<shard::RingState> state, Options opts = {});

    Result commit(const Txn& t);

    // Reads that cooperate with transactions: if the key is locked by a prepared
    // transaction they resolve it (if old) or wait (if young) and retry, so they
    // never return a value that is about to change.
    raft::KVResult get(const std::string& key);
    raft::KVResult set(const std::string& key, const std::string& value); // plain single-key write, same helping

    // A consistent snapshot of several keys across shards: reads each, then
    // validates them all with one compare-only transaction. True iff the
    // returned values were simultaneously current at one instant.
    bool snapshotRead(const std::vector<std::string>& keys, std::map<std::string, std::string>& out,
                      int attempts = 30);

    enum class Decision { Committed, Aborted, Unknown };
    // Finish transaction `txid`, which was seen stuck on shard `hintShard`
    // (0 if unknown): read the decision from the record, then commit or abort
    // every participant. Idempotent; safe to run concurrently with the
    // original coordinator and with other resolvers.
    Decision resolve(const std::string& txid, uint32_t hintShard, bool* cleanupPending = nullptr);

    // Re-ask a single-shard transaction that came back Indeterminate: re-send
    // the SAME txid and body. The shard remembers outcomes by txid, so this
    // returns what happened the first time (or applies it now if it never did).
    Result retrySingle(const Txn& t, const std::string& txid) { return commitSingleShard(t, coordOf(txid), txid); }

    // Every (shard, txid) currently Prepared-but-undecided, from TXPENDING.
    std::vector<std::pair<uint32_t, std::string>> pending();
    // Resolve every pending transaction whose txid is at least minAgeMs old.
    int sweep(int minAgeMs);

    uint32_t ownerOf(const std::string& key) const { return state_->snapshot()->newRing.owner(key); }
    std::vector<uint32_t> shardIds() const { return state_->snapshot()->cfg.shards; }
    raft::KVClient& shardClient(uint32_t id) { return sc_.shard(id); }

    // Invariant counter, process-wide: a TXCOMMIT that a participant answered
    // with ERR_ABORTED / ERR_UNKNOWN, or a TXABORT answered ERR_COMMITTED. The
    // protocol must make these impossible; the tests assert the count is 0.
    static long protocolViolations() { return violations_.load(); }

    static uint32_t coordOf(const std::string& txid);   // 0 if malformed
    static long ageMs(const std::string& txid);         // -1 if malformed
    static bool parseLocked(const std::string& value, std::string& txid);

private:
    std::shared_ptr<shard::RingState> state_;
    Options opts_;
    shard::ShardedClient sc_;
    std::string clientTag_;
    uint64_t seq_ = 0;
    static std::atomic<long> violations_;

    std::string makeTxid(uint32_t coord);
    raft::KVResult call(uint32_t shardId, const std::string& cmd, bool isRead, int tries = 2);
    bool deliver(const std::string& txid, const std::vector<uint32_t>& parts, bool commit);
    Result commitSingleShard(const Txn& t, uint32_t shardId, const std::string& txid = "");
    [[noreturn]] void crashNow(const char* where, const std::string& txid);
    void help(const std::string& txid, uint32_t shardId);
};

std::string hexEncode(const std::string& s);
// The wire form of a transaction's items, as sent in TXONE/TXPREP (for raw-protocol tests).
std::string encodeItems(const std::vector<Item>& items);

} // namespace txn
