/*
 * rpc.h -- a minimal, from-scratch RPC wire protocol, built purely to make
 * "how does RPC actually work at the byte level" concrete and verifiable.
 *
 * Wire format (all multi-byte integers are BIG-ENDIAN / network byte order,
 * exactly like every header field you've seen in real network protocols):
 *
 *   CALL message:
 *     uint32_t xid;        // client-chosen transaction ID, echoed in the reply
 *     uint32_t msg_type;   // MSG_CALL (0)
 *     uint32_t program;    // which "service" (a namespacing number)
 *     uint32_t version;    // interface version, for compatibility checks
 *     uint32_t procedure;  // WHICH remote function to invoke
 *     uint32_t arg_len;    // length in bytes of the marshalled arguments
 *     uint8_t  args[arg_len];   // the actual argument bytes (XDR-like encoding)
 *
 *   REPLY message:
 *     uint32_t xid;        // copied from the CALL this replies to
 *     uint32_t msg_type;   // MSG_REPLY (1)
 *     uint32_t status;     // RPC_SUCCESS (0) or an error code
 *     uint32_t result_len; // length in bytes of the marshalled result
 *     uint8_t  result[result_len];
 *
 * Every message is additionally prefixed, on the wire, with a single
 * uint32_t giving the TOTAL length of the message that follows -- this is
 * the length-prefix framing that solves TCP's "it's just a byte stream,
 * there are no message boundaries" problem, exactly like the length-
 * prefixed patterns you've already seen in DNS, HTTP chunked encoding,
 * and SMTP command parsing.
 */

#ifndef RPC_H
#define RPC_H

#include <stdint.h>
#include <stddef.h>

#define MSG_CALL  0u
#define MSG_REPLY 1u

#define RPC_SUCCESS         0u
#define RPC_ERR_NO_PROC      1u
#define RPC_ERR_BAD_VERSION  2u
#define RPC_ERR_DECODE       3u

#define DEMO_PROGRAM  0x20001u
#define DEMO_VERSION  1u

#define PROC_ADD      1u   /* int32 add(int32 a, int32 b) */
#define PROC_REVERSE  2u   /* string reverse(string s)     */

/* ---- A minimal XDR-like encoder/decoder -----------------------------
 * Every RPC system needs exactly this: a way to turn language-level
 * values (ints, strings, structs) into a flat, self-describing byte
 * sequence, and back. This mirrors real XDR's two core rules:
 *   1. Everything is encoded in a fixed byte order (big-endian here).
 *   2. Variable-length data (strings) is length-prefixed.
 */

typedef struct {
    uint8_t *buf;
    size_t   cap;
    size_t   len;   /* bytes actually used so far (encode) or consumed (decode) */
} xbuf_t;

static void xbuf_init(xbuf_t *x, uint8_t *buf, size_t cap) {
    x->buf = buf; x->cap = cap; x->len = 0;
}

/* --- encoding (marshalling) --- */
static int xdr_put_u32(xbuf_t *x, uint32_t v) {
    if (x->len + 4 > x->cap) return -1;
    x->buf[x->len+0] = (uint8_t)(v >> 24);
    x->buf[x->len+1] = (uint8_t)(v >> 16);
    x->buf[x->len+2] = (uint8_t)(v >> 8);
    x->buf[x->len+3] = (uint8_t)(v);
    x->len += 4;
    return 0;
}

static int xdr_put_i32(xbuf_t *x, int32_t v) { return xdr_put_u32(x, (uint32_t)v); }

static int xdr_put_string(xbuf_t *x, const char *s) {
    uint32_t n = (uint32_t)strlen(s);
    if (xdr_put_u32(x, n) != 0) return -1;
    if (x->len + n > x->cap) return -1;
    memcpy(x->buf + x->len, s, n);
    x->len += n;
    /* XDR pads every field to a 4-byte boundary -- real XDR does this so
       every subsequent field starts word-aligned, which matters for
       performance on architectures where unaligned access is slow. */
    while (x->len % 4 != 0) {
        if (x->len >= x->cap) return -1;
        x->buf[x->len++] = 0;
    }
    return 0;
}

/* --- decoding (unmarshalling) --- */
static int xdr_get_u32(xbuf_t *x, uint32_t *out) {
    if (x->len + 4 > x->cap) return -1;
    *out = ((uint32_t)x->buf[x->len+0] << 24) |
           ((uint32_t)x->buf[x->len+1] << 16) |
           ((uint32_t)x->buf[x->len+2] << 8)  |
           ((uint32_t)x->buf[x->len+3]);
    x->len += 4;
    return 0;
}

static int xdr_get_i32(xbuf_t *x, int32_t *out) {
    uint32_t u;
    if (xdr_get_u32(x, &u) != 0) return -1;
    *out = (int32_t)u;
    return 0;
}

/* out_buf must be at least maxlen+1 bytes; result is NUL-terminated */
static int xdr_get_string(xbuf_t *x, char *out_buf, size_t maxlen) {
    uint32_t n;
    if (xdr_get_u32(x, &n) != 0) return -1;
    if (n > maxlen) return -1;
    if (x->len + n > x->cap) return -1;
    memcpy(out_buf, x->buf + x->len, n);
    out_buf[n] = 0;
    x->len += n;
    while (x->len % 4 != 0) {
        if (x->len >= x->cap) return -1;
        x->len++;   /* skip padding on the way in */
    }
    return 0;
}

#endif /* RPC_H */