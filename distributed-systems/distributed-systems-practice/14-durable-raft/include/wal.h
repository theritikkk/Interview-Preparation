// wal.h — the on-disk side of Raft's "persistent state" (Project 14).
//
// Raft's correctness argument assumes three things survive a crash:
//   currentTerm, votedFor, and the log.   (Figure 2 of the paper.)
// This file stores them in a write-ahead log, and stores snapshots of the state
// machine in a second file so that the log can be cut down.
//
// WAL file ("wal"): a sequence of records, each
//
//     [u32 len][u32 crc32(body)][body = u8 type, payload...]       (big-endian u32s)
//
//   HARDSTATE  u64 term, u32 votedFor+1 (0 == none)   latest one wins
//   ENTRY      u64 index, u64 term, str command        must be at lastIndex+1
//   TRUNC      u64 fromIndex                           drop entries >= fromIndex
//   BASE       u64 index, u64 term                     first record of a rewritten WAL:
//                                                      "the log starts after this entry"
//
// Replay stops at the first record that is short or fails its CRC and cuts the
// file there: that is a write the crash interrupted, and (because every record is
// fsync'd before anyone is told about it) nobody was ever promised its contents.
//
// Snapshot file ("snapshot"): magic, last included index and term, the serialized
// state machine, and a CRC over all of it; written to a temp file, fsync'd,
// renamed over the old one, directory fsync'd.
//
// Every error on the write path is fatal to the caller (see RaftNode::walFatal):
// after a failed write or fsync the kernel may have dropped the dirty pages, so
// "retry and carry on" would silently acknowledge data that is not on disk.

#pragma once

#include "wire.h"

#include <chrono>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace raft {

struct LogEntry {
    uint64_t term;
    std::string command; // e.g. "SET key value", "DEL key", "CAS key old new", "TXPREP ..."
};

uint32_t crc32(const void* data, size_t len, uint32_t seed = 0);

class Wal {
public:
    ~Wal();

    // What replay recovered.
    struct Replayed {
        uint64_t term = 0;
        int32_t votedFor = -1;
        uint64_t base = 0;        // log starts after this index ...
        uint64_t baseTerm = 0;    // ... whose term was this
        std::vector<LogEntry> entries; // entries base+1, base+2, ...
        uint64_t records = 0;
        uint64_t fileBytes = 0;   // size found on disk
        uint64_t validBytes = 0;  // prefix that replayed cleanly
        bool tornTail = false;    // fileBytes > validBytes: a partial record was cut off
        std::string tornReason;
        // A bad record with VALID records after it is not a crash-torn tail (a crash
        // can only interrupt the last write); it is damage in the middle of the log.
        // open() refuses to start in that case instead of silently dropping what
        // follows, which may include entries this node has already acknowledged.
        bool midCorruption = false;
        uint64_t validRecordAt = 0;
    };

    // Opens `dir`/wal (creating it), replays it into `out`, cuts a torn tail, and
    // leaves the file open for appending. `doFsync == false` is the ablation that
    // writes but never calls fsync.
    bool open(const std::string& dir, bool doFsync, Replayed& out, std::string& err);

    // Appends are buffered by the kernel until sync().
    bool appendHardState(uint64_t term, int32_t votedFor);
    bool appendEntry(uint64_t index, uint64_t term, const std::string& command);
    bool appendTruncate(uint64_t fromIndex);
    bool sync();

    // Atomically replaces the WAL with BASE + HARDSTATE + the given entries
    // (entries[0] has index base+1). `hook(name)` is called at the crash-sensitive
    // points: "wal-tmp-synced" (new file durable, not yet renamed) and
    // "wal-renamed" (renamed, directory not yet fsync'd).
    bool rewrite(uint64_t base, uint64_t baseTerm, uint64_t term, int32_t votedFor,
                 const std::vector<LogEntry>& entries, const std::function<void(const char*)>& hook);

    // TEST-ONLY fault injection: after this many successful fsyncs, every sync() fails with
    // EIO (0 = never). Exercises the "a failed write path is fatal" policy.
    void failFsyncAfter(uint64_t n) { failAfter_ = n; }

    uint64_t size() const { return size_; }
    uint64_t syncedSize() const { return syncedSize_; }
    uint64_t fsyncCount() const { return fsyncs_; }
    uint64_t fsyncMicros() const { return fsyncUs_; }

    // TEST-ONLY. Simulates losing power: everything written but not fsync'd
    // vanishes (the file is cut back to the last synced size). This is how the
    // "no fsync" ablation is made to lose data: kill -9 alone never does, because
    // the page cache outlives the process.
    void simulatePowerLoss();

private:
    bool writeRecord(int fd, uint8_t type, const std::string& payload, uint64_t& sizeCounter);
    bool fsyncDir();
    std::string dir_;
    int fd_ = -1;
    bool doFsync_ = true;
    uint64_t size_ = 0;
    uint64_t syncedSize_ = 0;
    uint64_t fsyncs_ = 0;
    uint64_t fsyncUs_ = 0;
    uint64_t failAfter_ = 0;
};

// ---- snapshot file ----
struct SnapshotFile {
    uint64_t index = 0;
    uint64_t term = 0;
    std::string state;
};
enum class SnapLoad { None, Ok, Corrupt };

// Writes dir/snapshot atomically. hook("snap-tmp-synced") fires with the temp file
// durable but not yet renamed; hook("snap-renamed") after the rename.
bool writeSnapshotFile(const std::string& dir, const SnapshotFile& s, bool doFsync,
                       const std::function<void(const char*)>& hook, std::string& err);
SnapLoad loadSnapshotFile(const std::string& dir, SnapshotFile& out, std::string& err);

// small helpers shared with the node
bool ensureDir(const std::string& dir);
bool fsyncPath(const std::string& dir);
uint64_t fnv1a64(const std::string& s);

} // namespace raft
