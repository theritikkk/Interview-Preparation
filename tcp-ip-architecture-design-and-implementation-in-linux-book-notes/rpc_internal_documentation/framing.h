/*
 * framing.h -- turns TCP's raw byte stream into discrete messages.
 *
 * This is the exact "Son of Data Encapsulation" problem from ordinary
 * socket programming: TCP has no concept of a "message" at all, so if
 * an RPC message is going to travel over a TCP stream, SOMETHING has to
 * mark where one message ends and the next begins. We use the simplest
 * possible scheme: a 4-byte big-endian length prefix, in front of every
 * message, on the wire.
 */

#ifndef FRAMING_H
#define FRAMING_H

#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/* Reliably send ALL of buf[0..len), looping on short writes -- send()
   is only ever guaranteed to send SOME of what you asked for. */
static int send_all(int sock, const uint8_t *buf, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(sock, buf + sent, len - sent, 0);
        if (n <= 0) return -1;
        sent += (size_t)n;
    }
    return 0;
}

/* Reliably receive EXACTLY len bytes into buf, looping on short reads --
   recv() is equally only ever guaranteed to give you SOME of what's
   available, never a whole logical message in one call. */
static int recv_all(int sock, uint8_t *buf, size_t len) {
    size_t got = 0;
    while (got < len) {
        ssize_t n = recv(sock, buf + got, len - got, 0);
        if (n <= 0) return -1;   /* 0 = peer closed; <0 = real error */
        got += (size_t)n;
    }
    return 0;
}

static int send_framed(int sock, const uint8_t *msg, uint32_t len) {
    uint8_t hdr[4] = {
        (uint8_t)(len >> 24), (uint8_t)(len >> 16),
        (uint8_t)(len >> 8),  (uint8_t)(len)
    };
    if (send_all(sock, hdr, 4) != 0) return -1;
    return send_all(sock, msg, len);
}

/* Reads one complete framed message into buf (capacity maxlen).
   Returns the message length on success, -1 on error/disconnect. */
static int32_t recv_framed(int sock, uint8_t *buf, uint32_t maxlen) {
    uint8_t hdr[4];
    if (recv_all(sock, hdr, 4) != 0) return -1;
    uint32_t len = ((uint32_t)hdr[0] << 24) | ((uint32_t)hdr[1] << 16) |
                   ((uint32_t)hdr[2] << 8)  | (uint32_t)hdr[3];
    if (len > maxlen) return -1;
    if (recv_all(sock, buf, len) != 0) return -1;
    return (int32_t)len;
}

#endif /* FRAMING_H */