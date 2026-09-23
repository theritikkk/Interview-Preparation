#ifndef CHAIN_FRAMING_H
#define CHAIN_FRAMING_H

#include <stdint.h>
#include <sys/socket.h>
#include <unistd.h>

static int send_all(int sock, const uint8_t *buf, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(sock, buf + sent, len - sent, 0);
        if (n <= 0) return -1;
        sent += (size_t)n;
    }
    return 0;
}
static int recv_all(int sock, uint8_t *buf, size_t len) {
    size_t got = 0;
    while (got < len) {
        ssize_t n = recv(sock, buf + got, len - got, 0);
        if (n <= 0) return -1;
        got += (size_t)n;
    }
    return 0;
}
static int send_framed(int sock, const uint8_t *msg, uint32_t len) {
    uint8_t hdr[4] = { (uint8_t)(len>>24), (uint8_t)(len>>16), (uint8_t)(len>>8), (uint8_t)len };
    if (send_all(sock, hdr, 4) != 0) return -1;
    return send_all(sock, msg, len);
}
static int32_t recv_framed(int sock, uint8_t *buf, uint32_t maxlen) {
    uint8_t hdr[4];
    if (recv_all(sock, hdr, 4) != 0) return -1;
    uint32_t len = ((uint32_t)hdr[0]<<24)|((uint32_t)hdr[1]<<16)|((uint32_t)hdr[2]<<8)|hdr[3];
    if (len > maxlen) return -1;
    if (recv_all(sock, buf, len) != 0) return -1;
    return (int32_t)len;
}

#endif
