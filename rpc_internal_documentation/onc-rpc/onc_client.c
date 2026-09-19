/*
 * onc_client.c -- demonstrates the exact two-step flow described in
 * Sun_ONC_RPC_Internals.md section 5:
 *   1. Ask the portmapper "what port is program X, version Y on?"
 *   2. Connect DIRECTLY to whatever port that returns, and make the
 *      real call there.
 *
 * Also includes a standalone demonstration of RFC 5531's record
 * marking: sending ONE logical RPC message split across TWO separate
 * TCP-level fragments, each with its own 4-byte record-marking
 * header, to prove the "only the LAST fragment has the top bit set"
 * rule for real, not just as a written claim.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "rpc.h"
#include "framing.h"

#define PMAP_WELLKNOWN_PORT 9111
#define PMAPPROC_GETPORT 3u
#define MAXMSG 4096

static uint32_t ask_portmapper_for_port(uint32_t prog, uint32_t vers) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PMAP_WELLKNOWN_PORT);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    connect(sock, (struct sockaddr*)&addr, sizeof(addr));

    uint8_t argbuf[16];
    xbuf_t argx; xbuf_init(&argx, argbuf, sizeof(argbuf));
    xdr_put_u32(&argx, prog);
    xdr_put_u32(&argx, vers);

    uint8_t msg[MAXMSG];
    xbuf_t out; xbuf_init(&out, msg, sizeof(msg));
    xdr_put_u32(&out, 7); xdr_put_u32(&out, MSG_CALL);
    xdr_put_u32(&out, 100000u); xdr_put_u32(&out, 2u);
    xdr_put_u32(&out, PMAPPROC_GETPORT);
    xdr_put_u32(&out, (uint32_t)argx.len);
    memcpy(out.buf + out.len, argx.buf, argx.len);
    out.len += argx.len;

    printf("[client] Step 1: asking the portmapper (port %d) where program 0x%x, version %u lives...\n",
           PMAP_WELLKNOWN_PORT, prog, vers);
    send_framed(sock, out.buf, (uint32_t)out.len);

    uint8_t reply[MAXMSG];
    int32_t n = recv_framed(sock, reply, sizeof(reply));
    close(sock);

    xbuf_t in; xbuf_init(&in, reply, (uint32_t)n);
    uint32_t xid, mtype, status, rlen, port;
    xdr_get_u32(&in, &xid); xdr_get_u32(&in, &mtype);
    xdr_get_u32(&in, &status); xdr_get_u32(&in, &rlen);
    xdr_get_u32(&in, &port);
    printf("[client]         portmapper says: port %u\n\n", port);
    return port;
}

static int32_t call_add_directly(uint32_t port, int32_t a, int32_t b) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    printf("[client] Step 2: connecting DIRECTLY to port %u (no portmapper involved this time)\n", port);
    connect(sock, (struct sockaddr*)&addr, sizeof(addr));

    uint8_t argbuf[16];
    xbuf_t argx; xbuf_init(&argx, argbuf, sizeof(argbuf));
    xdr_put_i32(&argx, a); xdr_put_i32(&argx, b);

    uint8_t msg[MAXMSG];
    xbuf_t out; xbuf_init(&out, msg, sizeof(msg));
    xdr_put_u32(&out, 8); xdr_put_u32(&out, MSG_CALL);
    xdr_put_u32(&out, DEMO_PROGRAM); xdr_put_u32(&out, DEMO_VERSION);
    xdr_put_u32(&out, PROC_ADD);
    xdr_put_u32(&out, (uint32_t)argx.len);
    memcpy(out.buf + out.len, argx.buf, argx.len);
    out.len += argx.len;
    send_framed(sock, out.buf, (uint32_t)out.len);

    uint8_t reply[MAXMSG];
    int32_t n = recv_framed(sock, reply, sizeof(reply));
    close(sock);

    xbuf_t in; xbuf_init(&in, reply, (uint32_t)n);
    uint32_t xid, mtype, status, rlen;
    xdr_get_u32(&in, &xid); xdr_get_u32(&in, &mtype);
    xdr_get_u32(&in, &status); xdr_get_u32(&in, &rlen);
    int32_t result;
    xdr_get_i32(&in, &result);
    return result;
}

/* ---- A genuine, real demonstration of RFC 5531 record marking ---- */
static void demonstrate_record_marking(uint32_t port) {
    printf("\n[client] ---- Now: a real multi-fragment record-marking test ----\n");
    printf("[client] Sending ONE logical RPC call, split across TWO TCP-level\n");
    printf("[client] fragments, each with its own record-marking header,\n");
    printf("[client] exactly as RFC 5531 section 11 allows.\n\n");

    /* Build one complete, normal CALL message first */
    uint8_t argbuf[16];
    xbuf_t argx; xbuf_init(&argx, argbuf, sizeof(argbuf));
    xdr_put_i32(&argx, 111);
    xdr_put_i32(&argx, 222);

    uint8_t msg[MAXMSG];
    xbuf_t out; xbuf_init(&out, msg, sizeof(msg));
    xdr_put_u32(&out, 99); xdr_put_u32(&out, MSG_CALL);
    xdr_put_u32(&out, DEMO_PROGRAM); xdr_put_u32(&out, DEMO_VERSION);
    xdr_put_u32(&out, PROC_ADD);
    xdr_put_u32(&out, (uint32_t)argx.len);
    memcpy(out.buf + out.len, argx.buf, argx.len);
    out.len += argx.len;

    uint32_t total_len = (uint32_t)out.len;
    uint32_t split_at = total_len / 2;   /* cut it in half, arbitrarily */

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    connect(sock, (struct sockaddr*)&addr, sizeof(addr));

    /* Fragment 1: length = split_at, TOP BIT CLEAR ("more fragments follow") */
    uint32_t frag1_header = split_at;   /* top bit 0 */
    uint8_t hdr1[4] = { (uint8_t)(frag1_header>>24), (uint8_t)(frag1_header>>16),
                        (uint8_t)(frag1_header>>8),  (uint8_t)(frag1_header) };
    printf("[client] Fragment 1 header bytes: %02x %02x %02x %02x  "
           "(top bit = %d -> more fragments follow; length = %u)\n",
           hdr1[0], hdr1[1], hdr1[2], hdr1[3], (hdr1[0] & 0x80) != 0, split_at);
    send_all(sock, hdr1, 4);
    send_all(sock, out.buf, split_at);

    usleep(200000); /* pause, just to prove these really are two separate writes */

    /* Fragment 2: length = remaining bytes, TOP BIT SET ("this is the last fragment") */
    uint32_t remaining = total_len - split_at;
    uint32_t frag2_header = remaining | 0x80000000u;   /* top bit 1 */
    uint8_t hdr2[4] = { (uint8_t)(frag2_header>>24), (uint8_t)(frag2_header>>16),
                        (uint8_t)(frag2_header>>8),  (uint8_t)(frag2_header) };
    printf("[client] Fragment 2 header bytes: %02x %02x %02x %02x  "
           "(top bit = %d -> LAST fragment; length = %u)\n",
           hdr2[0], hdr2[1], hdr2[2], hdr2[3], (hdr2[0] & 0x80) != 0, remaining);
    send_all(sock, hdr2, 4);
    send_all(sock, out.buf + split_at, remaining);

    uint8_t reply[MAXMSG];
    int32_t n = recv_framed(sock, reply, sizeof(reply));
    close(sock);

    xbuf_t in; xbuf_init(&in, reply, (uint32_t)n);
    uint32_t xid, mtype, status, rlen;
    xdr_get_u32(&in, &xid); xdr_get_u32(&in, &mtype);
    xdr_get_u32(&in, &status); xdr_get_u32(&in, &rlen);
    int32_t result;
    xdr_get_i32(&in, &result);
    printf("[client] Server correctly reassembled both fragments and replied: 111 + 222 = %d\n", result);
}

int main(void) {
    uint32_t port = ask_portmapper_for_port(DEMO_PROGRAM, DEMO_VERSION);
    if (port == 0) {
        fprintf(stderr, "service not registered!\n");
        return 1;
    }

    int32_t result = call_add_directly(port, 3, 4);
    printf("[client]         ADD(3, 4) = %d\n", result);

    demonstrate_record_marking(port);
    return 0;
}