// wire.h — minimal binary framing + serialization for Raft RPCs.
//
// No external dependencies (no protobuf/JSON) on purpose: this keeps the
// build self-contained in a sandboxed environment where installing extra
// system packages isn't guaranteed. Every message is:
//
//   [4-byte big-endian length][payload bytes]
//
// and the payload is built by ByteWriter / parsed by ByteReader below.
// This is deliberately simple, not a general-purpose serialization format.

#pragma once

#include <arpa/inet.h>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

namespace raft {

// ---- 64-bit host/network byte order (not all libcs expose htobe64) ----
inline uint64_t hton64(uint64_t v) {
    uint32_t hi = htonl(static_cast<uint32_t>(v >> 32));
    uint32_t lo = htonl(static_cast<uint32_t>(v & 0xFFFFFFFFULL));
    return (static_cast<uint64_t>(lo) << 32) | hi;
}
inline uint64_t ntoh64(uint64_t v) { return hton64(v); } // symmetric

// ---------------------------------------------------------------------
class ByteWriter {
public:
    void putU8(uint8_t v) { buf_.push_back(static_cast<char>(v)); }

    void putU32(uint32_t v) {
        uint32_t n = htonl(v);
        appendRaw(&n, sizeof(n));
    }

    void putU64(uint64_t v) {
        uint64_t n = hton64(v);
        appendRaw(&n, sizeof(n));
    }

    // length-prefixed string
    void putStr(const std::string& s) {
        putU32(static_cast<uint32_t>(s.size()));
        buf_.insert(buf_.end(), s.begin(), s.end());
    }

    const std::string& bytes() const { return buf_; }

private:
    void appendRaw(const void* p, size_t n) {
        const char* c = static_cast<const char*>(p);
        buf_.insert(buf_.end(), c, c + n);
    }
    std::string buf_;
};

// ---------------------------------------------------------------------
class ByteReader {
public:
    explicit ByteReader(const std::string& data) : data_(data), pos_(0) {}

    uint8_t getU8() {
        require(1);
        return static_cast<uint8_t>(data_[pos_++]);
    }

    uint32_t getU32() {
        require(4);
        uint32_t n;
        std::memcpy(&n, data_.data() + pos_, 4);
        pos_ += 4;
        return ntohl(n);
    }

    uint64_t getU64() {
        require(8);
        uint64_t n;
        std::memcpy(&n, data_.data() + pos_, 8);
        pos_ += 8;
        return ntoh64(n);
    }

    std::string getStr() {
        uint32_t len = getU32();
        require(len);
        std::string s = data_.substr(pos_, len);
        pos_ += len;
        return s;
    }

    bool atEnd() const { return pos_ >= data_.size(); }

private:
    void require(size_t n) const {
        if (pos_ + n > data_.size())
            throw std::runtime_error("wire: malformed message (short read)");
    }
    const std::string& data_;
    size_t pos_;
};

// ---------------------------------------------------------------------
// Framed send/recv over a connected, blocking TCP socket.
// Returns false on any I/O error (peer closed, timed out, etc.) — callers
// treat that exactly like "RPC to an unreachable peer", which is the
// normal, expected case whenever a node in the cluster is down.

// MSG_NOSIGNAL: writing to a socket whose peer has gone away must come back as an error
// (EPIPE), not kill the process with SIGPIPE. Without it, a node or a client dies the
// moment it sends to a connection that was reset by a kill -9'd peer; the durability stress
// test (Project 14) found exactly that.
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif
inline bool sendAll(int fd, const char* data, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = ::send(fd, data + sent, len - sent, MSG_NOSIGNAL);
        if (n <= 0) return false;
        sent += static_cast<size_t>(n);
    }
    return true;
}

inline bool recvAll(int fd, char* data, size_t len) {
    size_t got = 0;
    while (got < len) {
        ssize_t n = ::recv(fd, data + got, len - got, 0);
        if (n <= 0) return false; // 0 = peer closed, <0 = error/timeout
        got += static_cast<size_t>(n);
    }
    return true;
}

inline bool sendFramed(int fd, const std::string& payload) {
    uint32_t len = htonl(static_cast<uint32_t>(payload.size()));
    if (!sendAll(fd, reinterpret_cast<const char*>(&len), sizeof(len))) return false;
    return sendAll(fd, payload.data(), payload.size());
}

// Caps a single message at 16 MiB — plenty for this teaching implementation,
// and guards against a malformed length prefix causing a huge allocation.
inline bool recvFramed(int fd, std::string& out) {
    uint32_t netLen;
    if (!recvAll(fd, reinterpret_cast<char*>(&netLen), sizeof(netLen))) return false;
    uint32_t len = ntohl(netLen);
    if (len > (16u << 20)) return false;
    out.resize(len);
    if (len == 0) return true;
    return recvAll(fd, &out[0], len);
}

// ---------------------------------------------------------------------
// Non-blocking connect with a real wall-clock timeout, then switched back
// to a blocking socket with a send/recv timeout. Returns -1 on any failure
// (unreachable peer, connection refused, timed out) — which is the normal,
// expected outcome whenever a cluster member is down. Shared by the Raft
// node's own outbound RPCs and by the CLI client.
inline int connectWithTimeout(const std::string& host, uint16_t port, int timeoutMs) {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (::inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1) {
        addrinfo hints{};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        addrinfo* res = nullptr;
        if (getaddrinfo(host.c_str(), nullptr, &hints, &res) != 0 || res == nullptr) {
            ::close(fd);
            return -1;
        }
        addr.sin_addr = reinterpret_cast<sockaddr_in*>(res->ai_addr)->sin_addr;
        freeaddrinfo(res);
    }

    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    int rc = ::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if (rc < 0 && errno != EINPROGRESS) {
        ::close(fd);
        return -1;
    }
    if (rc < 0) {
        fd_set wfds;
        FD_ZERO(&wfds);
        FD_SET(fd, &wfds);
        timeval tv{timeoutMs / 1000, (timeoutMs % 1000) * 1000};
        rc = ::select(fd + 1, nullptr, &wfds, nullptr, &tv);
        if (rc <= 0) {
            ::close(fd);
            return -1;
        }
        int soerr = 0;
        socklen_t slen = sizeof(soerr);
        getsockopt(fd, SOL_SOCKET, SO_ERROR, &soerr, &slen);
        if (soerr != 0) {
            ::close(fd);
            return -1;
        }
    }

    fcntl(fd, F_SETFL, flags); // back to blocking
    timeval rtv{timeoutMs / 1000, (timeoutMs % 1000) * 1000};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &rtv, sizeof(rtv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &rtv, sizeof(rtv));
    return fd;
}

} // namespace raft
