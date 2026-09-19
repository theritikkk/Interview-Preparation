/*
 * framing.h -- REAL RFC 5531 "record marking," not the simplified
 * flat length-prefix used elsewhere in this document series.
 *
 * Every fragment header is 4 bytes: the TOP bit says "this is the
 * LAST fragment of the record" (1) or "more fragments follow" (0);
 * the remaining 31 bits are that fragment's length. A receiver keeps
 * reading fragments -- concatenating their payloads -- until it sees
 * one with the top bit set. In the overwhelmingly common case (one
 * message = one fragment), that's just a single 4-byte header with
 * the top bit already set and the full length in the low 31 bits --
 * this is why send_framed() below always sets it: this IS how real
 * ONC RPC implementations behave for an ordinary, unfragmented call.
 */

#ifndef FRAMING_H
#define FRAMING_H

#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define RM_LAST_FRAGMENT 0x80000000u

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

/* Sends one COMPLETE record as a single fragment, with the
   last-fragment bit already set -- exactly what a real ONC RPC
   implementation does for an ordinary, unfragmented call. */
static int send_framed(int sock, const uint8_t *msg, uint32_t len) {
    uint32_t header = len | RM_LAST_FRAGMENT;
    uint8_t hdr[4] = {
        (uint8_t)(header >> 24), (uint8_t)(header >> 16),
        (uint8_t)(header >> 8),  (uint8_t)(header)
    };
    if (send_all(sock, hdr, 4) != 0) return -1;
    return send_all(sock, msg, len);
}

/* Reads however many fragments it takes to see the last-fragment bit
   set, concatenating their payloads into buf. This is what makes the
   multi-fragment demo in onc_client.c actually work: whether the
   sender used one fragment or several, this function reassembles the
   full logical record transparently before handing it back. */
static int32_t recv_framed(int sock, uint8_t *buf, uint32_t maxlen) {
    uint32_t total = 0;
    for (;;) {
        uint8_t hdr[4];
        if (recv_all(sock, hdr, 4) != 0) return -1;
        uint32_t header = ((uint32_t)hdr[0] << 24) | ((uint32_t)hdr[1] << 16) |
                           ((uint32_t)hdr[2] << 8)  | (uint32_t)hdr[3];
        int is_last = (header & RM_LAST_FRAGMENT) != 0;
        uint32_t fraglen = header & 0x7fffffffu;

        if (total + fraglen > maxlen) return -1;
        if (recv_all(sock, buf + total, fraglen) != 0) return -1;
        total += fraglen;

        if (is_last) break;
        /* else: loop again, reading the NEXT fragment's header --
           this is the exact reassembly loop RFC 5531 describes */
    }
    return (int32_t)total;
}

#endif /* FRAMING_H */