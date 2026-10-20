#include "wal.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace raft {

// ---------------------------------------------------------------- helpers
uint32_t crc32(const void* data, size_t len, uint32_t seed) {
    static uint32_t table[256];
    static bool init = false;
    if (!init) { // benign race: every thread computes the same table
        for (uint32_t i = 0; i < 256; i++) {
            uint32_t c = i;
            for (int k = 0; k < 8; k++) c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            table[i] = c;
        }
        init = true;
    }
    uint32_t c = ~seed;
    const unsigned char* p = static_cast<const unsigned char*>(data);
    for (size_t i = 0; i < len; i++) c = table[(c ^ p[i]) & 0xFF] ^ (c >> 8);
    return ~c;
}

uint64_t fnv1a64(const std::string& s) {
    uint64_t h = 1469598103934665603ULL;
    for (unsigned char c : s) {
        h ^= c;
        h *= 1099511628211ULL;
    }
    return h;
}

bool ensureDir(const std::string& dir) {
    std::string cur;
    for (size_t i = 0; i <= dir.size(); i++) {
        if (i == dir.size() || dir[i] == '/') {
            if (!cur.empty() && ::mkdir(cur.c_str(), 0755) != 0 && errno != EEXIST) return false;
        }
        if (i < dir.size()) cur += dir[i];
    }
    return true;
}

bool fsyncPath(const std::string& dir) {
    int fd = ::open(dir.c_str(), O_RDONLY);
    if (fd < 0) return false;
    bool ok = ::fsync(fd) == 0;
    ::close(fd);
    return ok;
}

static bool writeAll(int fd, const char* p, size_t n) {
    while (n > 0) {
        ssize_t w = ::write(fd, p, n);
        if (w < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        p += w;
        n -= static_cast<size_t>(w);
    }
    return true;
}

static bool readWholeFile(int fd, std::string& out) {
    out.clear();
    char buf[1 << 16];
    ::lseek(fd, 0, SEEK_SET);
    while (true) {
        ssize_t n = ::read(fd, buf, sizeof(buf));
        if (n < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        if (n == 0) break;
        out.append(buf, static_cast<size_t>(n));
    }
    return true;
}

enum : uint8_t { REC_HARDSTATE = 1, REC_ENTRY = 2, REC_TRUNC = 3, REC_BASE = 4 };

// ---------------------------------------------------------------- Wal
Wal::~Wal() {
    if (fd_ >= 0) ::close(fd_);
}

bool Wal::writeRecord(int fd, uint8_t type, const std::string& payload, uint64_t& sizeCounter) {
    std::string body;
    body.reserve(1 + payload.size());
    body.push_back(static_cast<char>(type));
    body += payload;
    ByteWriter w;
    w.putU32(static_cast<uint32_t>(body.size()));
    w.putU32(crc32(body.data(), body.size()));
    std::string rec = w.bytes() + body;
    if (!writeAll(fd, rec.data(), rec.size())) return false;
    sizeCounter += rec.size();
    return true;
}

bool Wal::fsyncDir() { return fsyncPath(dir_); }

bool Wal::open(const std::string& dir, bool doFsync, Replayed& out, std::string& err) {
    dir_ = dir;
    doFsync_ = doFsync;
    out = Replayed{};
    if (!ensureDir(dir)) {
        err = "mkdir " + dir + ": " + std::strerror(errno);
        return false;
    }
    ::unlink((dir + "/wal.tmp").c_str()); // a rewrite the crash interrupted before its rename
    const std::string path = dir + "/wal";
    struct stat st{};
    const bool existed = ::stat(path.c_str(), &st) == 0;
    fd_ = ::open(path.c_str(), O_RDWR | O_CREAT, 0644);
    if (fd_ < 0) {
        err = "open " + path + ": " + std::strerror(errno);
        return false;
    }
    if (!existed && doFsync_ && !fsyncDir()) {
        err = "fsync dir: " + std::string(std::strerror(errno));
        return false;
    }
    std::string data;
    if (!readWholeFile(fd_, data)) {
        err = "read " + path + ": " + std::strerror(errno);
        return false;
    }
    out.fileBytes = data.size();

    size_t pos = 0;
    uint64_t good = 0;
    while (pos < data.size()) {
        auto torn = [&](const std::string& why) {
            out.tornTail = true;
            out.tornReason = why;
        };
        if (data.size() - pos < 8) { torn("short header"); break; }
        uint32_t len, crc;
        std::memcpy(&len, data.data() + pos, 4);
        std::memcpy(&crc, data.data() + pos + 4, 4);
        len = ntohl(len);
        crc = ntohl(crc);
        if (len < 1 || len > (64u << 20)) { torn("implausible length"); break; }
        if (data.size() - pos - 8 < len) { torn("short body"); break; }
        const char* body = data.data() + pos + 8;
        if (crc32(body, len) != crc) { torn("crc mismatch"); break; }

        const uint8_t type = static_cast<uint8_t>(body[0]);
        std::string payload(body + 1, len - 1);
        bool good_rec = true;
        try {
            ByteReader r(payload);
            switch (type) {
                case REC_HARDSTATE: {
                    out.term = r.getU64();
                    out.votedFor = static_cast<int32_t>(r.getU32()) - 1;
                    break;
                }
                case REC_ENTRY: {
                    uint64_t index = r.getU64();
                    LogEntry e;
                    e.term = r.getU64();
                    e.command = r.getStr();
                    if (index != out.base + out.entries.size() + 1) good_rec = false; // gap or overlap
                    else out.entries.push_back(std::move(e));
                    break;
                }
                case REC_TRUNC: {
                    uint64_t from = r.getU64();
                    if (from <= out.base || from > out.base + out.entries.size() + 1) good_rec = false;
                    else out.entries.resize(from - out.base - 1);
                    break;
                }
                case REC_BASE: {
                    if (out.records != 0) { good_rec = false; break; }
                    out.base = r.getU64();
                    out.baseTerm = r.getU64();
                    break;
                }
                default: good_rec = false;
            }
        } catch (const std::exception&) {
            good_rec = false;
        }
        if (!good_rec) { torn("inconsistent record"); break; }
        pos += 8 + len;
        good = pos;
        out.records++;
    }
    out.validBytes = good;

    if (good < data.size()) {
        // Is there any intact record AFTER the bad spot? Brute-force every offset:
        // a 32-bit CRC makes a false match on garbage astronomically unlikely, and
        // this only runs when something is already wrong.
        for (size_t q = good + 1; q + 9 <= data.size(); q++) {
            uint32_t l, c;
            std::memcpy(&l, data.data() + q, 4);
            std::memcpy(&c, data.data() + q + 4, 4);
            l = ntohl(l);
            c = ntohl(c);
            if (l < 1 || l > (64u << 20) || data.size() - q - 8 < l) continue;
            const char* b = data.data() + q + 8;
            uint8_t t = static_cast<uint8_t>(b[0]);
            if (t < REC_HARDSTATE || t > REC_BASE) continue;
            if (crc32(b, l) != c) continue;
            out.midCorruption = true;
            out.validRecordAt = q;
            break;
        }
        if (out.midCorruption) {
            err = "WAL is damaged in the middle (bad record at byte " + std::to_string(good) + " [" + out.tornReason +
                  "] but an intact record follows at byte " + std::to_string(out.validRecordAt) +
                  "): not a torn tail, so not safe to cut; refusing to start";
            return false;
        }
    }

    if (good < data.size()) { // cut the partial record so appends start on a record boundary
        if (::ftruncate(fd_, static_cast<off_t>(good)) != 0) {
            err = "ftruncate: " + std::string(std::strerror(errno));
            return false;
        }
        if (doFsync_ && ::fsync(fd_) != 0) {
            err = "fsync: " + std::string(std::strerror(errno));
            return false;
        }
        out.tornTail = true;
    }
    ::lseek(fd_, 0, SEEK_END);
    size_ = good;
    syncedSize_ = good; // whatever survived to now is, by definition, what the disk holds
    return true;
}

bool Wal::appendHardState(uint64_t term, int32_t votedFor) {
    ByteWriter w;
    w.putU64(term);
    w.putU32(static_cast<uint32_t>(votedFor + 1));
    return writeRecord(fd_, REC_HARDSTATE, w.bytes(), size_);
}

bool Wal::appendEntry(uint64_t index, uint64_t term, const std::string& command) {
    ByteWriter w;
    w.putU64(index);
    w.putU64(term);
    w.putStr(command);
    return writeRecord(fd_, REC_ENTRY, w.bytes(), size_);
}

bool Wal::appendTruncate(uint64_t fromIndex) {
    ByteWriter w;
    w.putU64(fromIndex);
    return writeRecord(fd_, REC_TRUNC, w.bytes(), size_);
}

bool Wal::sync() {
    if (!doFsync_) return true;       // ablation: never durable
    if (size_ == syncedSize_) return true;
    if (failAfter_ != 0 && fsyncs_ >= failAfter_) { errno = EIO; return false; } // injected fault
    auto t0 = std::chrono::steady_clock::now();
    if (::fsync(fd_) != 0) return false;
    fsyncUs_ += static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - t0).count());
    fsyncs_++;
    syncedSize_ = size_;
    return true;
}

bool Wal::rewrite(uint64_t base, uint64_t baseTerm, uint64_t term, int32_t votedFor,
                  const std::vector<LogEntry>& entries, const std::function<void(const char*)>& hook) {
    const std::string tmp = dir_ + "/wal.tmp", path = dir_ + "/wal";
    int nfd = ::open(tmp.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (nfd < 0) return false;
    uint64_t nsize = 0;
    bool ok = true;
    {
        ByteWriter w;
        w.putU64(base);
        w.putU64(baseTerm);
        ok = ok && writeRecord(nfd, REC_BASE, w.bytes(), nsize);
    }
    {
        ByteWriter w;
        w.putU64(term);
        w.putU32(static_cast<uint32_t>(votedFor + 1));
        ok = ok && writeRecord(nfd, REC_HARDSTATE, w.bytes(), nsize);
    }
    for (size_t i = 0; ok && i < entries.size(); i++) {
        ByteWriter w;
        w.putU64(base + 1 + i);
        w.putU64(entries[i].term);
        w.putStr(entries[i].command);
        ok = writeRecord(nfd, REC_ENTRY, w.bytes(), nsize);
    }
    if (ok && doFsync_) ok = ::fsync(nfd) == 0;
    if (!ok) {
        ::close(nfd);
        return false;
    }
    hook("wal-tmp-synced");
    if (::rename(tmp.c_str(), path.c_str()) != 0) {
        ::close(nfd);
        return false;
    }
    hook("wal-renamed");
    ::close(fd_);
    fd_ = nfd; // the renamed file; keep appending to it
    size_ = nsize;
    syncedSize_ = doFsync_ ? nsize : 0;
    if (doFsync_ && !fsyncDir()) return false;
    return true;
}

void Wal::simulatePowerLoss() {
    if (fd_ >= 0) {
        if (::ftruncate(fd_, static_cast<off_t>(syncedSize_)) != 0) { /* best effort */ }
    }
}

// ---------------------------------------------------------------- snapshot file
static const char kSnapMagic[8] = {'R', 'S', 'N', 'A', 'P', '0', '0', '1'};

bool writeSnapshotFile(const std::string& dir, const SnapshotFile& s, bool doFsync,
                       const std::function<void(const char*)>& hook, std::string& err) {
    ByteWriter w;
    w.putU64(s.index);
    w.putU64(s.term);
    w.putStr(s.state);
    std::string body(kSnapMagic, 8);
    body += w.bytes();
    ByteWriter c;
    c.putU32(crc32(body.data(), body.size()));
    body += c.bytes();

    const std::string tmp = dir + "/snapshot.tmp", path = dir + "/snapshot";
    int fd = ::open(tmp.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        err = "open " + tmp + ": " + std::strerror(errno);
        return false;
    }
    if (!writeAll(fd, body.data(), body.size()) || (doFsync && ::fsync(fd) != 0)) {
        err = "write/fsync snapshot: " + std::string(std::strerror(errno));
        ::close(fd);
        return false;
    }
    ::close(fd);
    hook("snap-tmp-synced");
    if (::rename(tmp.c_str(), path.c_str()) != 0) {
        err = "rename snapshot: " + std::string(std::strerror(errno));
        return false;
    }
    hook("snap-renamed");
    if (doFsync && !fsyncPath(dir)) {
        err = "fsync dir: " + std::string(std::strerror(errno));
        return false;
    }
    return true;
}

SnapLoad loadSnapshotFile(const std::string& dir, SnapshotFile& out, std::string& err) {
    ::unlink((dir + "/snapshot.tmp").c_str()); // an interrupted write that never got renamed
    const std::string path = dir + "/snapshot";
    int fd = ::open(path.c_str(), O_RDONLY);
    if (fd < 0) return SnapLoad::None;
    std::string data;
    bool rd = readWholeFile(fd, data);
    ::close(fd);
    if (!rd) { err = "read snapshot"; return SnapLoad::Corrupt; }
    if (data.size() < 8 + 20 + 4 || std::memcmp(data.data(), kSnapMagic, 8) != 0) {
        err = "bad magic or short file";
        return SnapLoad::Corrupt;
    }
    uint32_t want;
    std::memcpy(&want, data.data() + data.size() - 4, 4);
    want = ntohl(want);
    if (crc32(data.data(), data.size() - 4) != want) {
        err = "crc mismatch";
        return SnapLoad::Corrupt;
    }
    try {
        std::string payload = data.substr(8, data.size() - 12);
        ByteReader r(payload);
        out.index = r.getU64();
        out.term = r.getU64();
        out.state = r.getStr();
    } catch (const std::exception&) {
        err = "malformed payload";
        return SnapLoad::Corrupt;
    }
    return SnapLoad::Ok;
}

} // namespace raft
