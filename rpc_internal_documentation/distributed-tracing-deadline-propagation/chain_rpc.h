/*
 * chain_rpc.h -- wire format for a MULTI-HOP RPC chain, with the two
 * fields every real production RPC system propagates on every call
 * that this whole document series hasn't touched yet:
 *
 *   trace_id  -- an opaque identifier, generated ONCE by the original
 *                caller, and passed UNCHANGED through every hop --
 *                this is what lets you find "every log line anywhere
 *                in the system that belongs to this one logical
 *                request," even across process/machine boundaries.
 *
 *   deadline  -- an ABSOLUTE point in time (not a relative timeout!)
 *                by which the ENTIRE end-to-end chain must finish.
 *                Propagated unchanged, hop to hop. Each hop's very
 *                FIRST action is checking "has this already passed?"
 *                -- if so, it fails immediately, without doing any
 *                work AND without calling the next hop at all. This
 *                is the entire point: an absolute deadline lets hop
 *                #5 in a chain refuse instantly, rather than doing
 *                real work for an answer hop #1 has already given up
 *                waiting for.
 *
 * CALL wire format (all big-endian):
 *   uint32 xid
 *   uint32 msg_type      (0 = CALL, 1 = REPLY)
 *   uint64 trace_id
 *   int64  deadline_ms   (absolute Unix epoch milliseconds)
 *   uint32 program
 *   uint32 version
 *   uint32 procedure
 *   uint32 arg_len
 *   [arg_len bytes of arguments]
 *
 * REPLY wire format:
 *   uint32 xid
 *   uint32 msg_type      (1 = REPLY)
 *   uint32 status        (0 = OK, 1 = DEADLINE_EXCEEDED, 2 = other error)
 *   uint32 result_len
 *   [result_len bytes]
 */

#ifndef CHAIN_RPC_H
#define CHAIN_RPC_H

#include <stdint.h>
#include <string.h>
#include <sys/time.h>

#define MSG_CALL  0u
#define MSG_REPLY 1u

#define STATUS_OK               0u
#define STATUS_DEADLINE_EXCEEDED 1u
#define STATUS_ERROR            2u

typedef struct { uint8_t *buf; size_t cap; size_t len; } xbuf_t;

static void xbuf_init(xbuf_t *x, uint8_t *buf, size_t cap) {
    x->buf = buf; x->cap = cap; x->len = 0;
}

static void put_u32(xbuf_t *x, uint32_t v) {
    x->buf[x->len++] = (uint8_t)(v >> 24);
    x->buf[x->len++] = (uint8_t)(v >> 16);
    x->buf[x->len++] = (uint8_t)(v >> 8);
    x->buf[x->len++] = (uint8_t)(v);
}
static void put_u64(xbuf_t *x, uint64_t v) {
    put_u32(x, (uint32_t)(v >> 32));
    put_u32(x, (uint32_t)(v & 0xffffffffu));
}
static void put_i64(xbuf_t *x, int64_t v) { put_u64(x, (uint64_t)v); }

static uint32_t get_u32(xbuf_t *x) {
    uint32_t v = ((uint32_t)x->buf[x->len]   << 24) | ((uint32_t)x->buf[x->len+1] << 16) |
                 ((uint32_t)x->buf[x->len+2] << 8)  | ((uint32_t)x->buf[x->len+3]);
    x->len += 4;
    return v;
}
static uint64_t get_u64(xbuf_t *x) {
    uint64_t hi = get_u32(x), lo = get_u32(x);
    return (hi << 32) | lo;
}
static int64_t get_i64(xbuf_t *x) { return (int64_t)get_u64(x); }

/* current absolute time, in milliseconds since the Unix epoch --
   the same units every trace_id/deadline field on the wire uses */
static int64_t now_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

#endif
