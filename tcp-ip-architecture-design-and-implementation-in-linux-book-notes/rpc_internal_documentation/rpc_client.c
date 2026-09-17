/*
 * rpc_client.c -- the client side. rpc_add() and rpc_reverse() below are
 * "client stubs": from the CALLER's point of view they're ordinary C
 * functions. Internally, each one marshals its arguments, sends a CALL
 * message, blocks waiting for the matching REPLY, and unmarshals the
 * result -- this whole file is the code a real IDL compiler would
 * generate for you automatically from an interface definition.
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

#define MAXMSG 4096

static int g_sock = -1;
static uint32_t g_next_xid = 1;

static void hexdump(const char *label, const uint8_t *buf, uint32_t len) {
    printf("  %s (%u bytes):\n", label, len);
    for (uint32_t i = 0; i < len; i += 16) {
        printf("    %04x: ", i);
        for (uint32_t j = i; j < i + 16; j++) {
            if (j < len) printf("%02x ", buf[j]); else printf("   ");
        }
        printf(" ");
        for (uint32_t j = i; j < i + 16 && j < len; j++) {
            uint8_t c = buf[j];
            putchar((c >= 32 && c < 127) ? c : '.');
        }
        printf("\n");
    }
}

/* The generic engine every stub below calls into: build the CALL
   header, append the already-marshalled arguments, send it framed,
   receive the framed REPLY, split off ITS header, and hand back a
   cursor positioned at the start of the marshalled result. */
static int rpc_call(uint32_t procedure, xbuf_t *argx,
                     uint8_t *resbuf, uint32_t resbuf_cap,
                     xbuf_t *resx, int dump_wire) {
    uint32_t xid = g_next_xid++;

    uint8_t msg[MAXMSG];
    xbuf_t out;
    xbuf_init(&out, msg, sizeof(msg));
    xdr_put_u32(&out, xid);
    xdr_put_u32(&out, MSG_CALL);
    xdr_put_u32(&out, DEMO_PROGRAM);
    xdr_put_u32(&out, DEMO_VERSION);
    xdr_put_u32(&out, procedure);
    xdr_put_u32(&out, (uint32_t)argx->len);
    memcpy(out.buf + out.len, argx->buf, argx->len);
    out.len += argx->len;

    if (dump_wire) hexdump("CALL message about to be sent", out.buf, (uint32_t)out.len);

    if (send_framed(g_sock, out.buf, (uint32_t)out.len) != 0) return -1;

    uint8_t reply[MAXMSG];
    int32_t n = recv_framed(g_sock, reply, sizeof(reply));
    if (n < 0) return -1;

    if (dump_wire) hexdump("REPLY message received", reply, (uint32_t)n);

    xbuf_t in;
    xbuf_init(&in, reply, (uint32_t)n);
    uint32_t rxid, msg_type, status, result_len;
    xdr_get_u32(&in, &rxid);
    xdr_get_u32(&in, &msg_type);
    xdr_get_u32(&in, &status);
    xdr_get_u32(&in, &result_len);

    if (rxid != xid) { fprintf(stderr, "xid mismatch! sent %u, got %u\n", xid, rxid); return -1; }
    if (status != RPC_SUCCESS) { fprintf(stderr, "RPC error status=%u\n", status); return -1; }

    if (result_len > resbuf_cap) return -1;
    memcpy(resbuf, reply + in.len, result_len);
    xbuf_init(resx, resbuf, result_len);
    return 0;
}

/* ---- The stubs themselves: this is the "transparency" of RPC ------- */

int32_t rpc_add(int32_t a, int32_t b, int dump_wire) {
    uint8_t argbuf[64];
    xbuf_t argx; xbuf_init(&argx, argbuf, sizeof(argbuf));
    xdr_put_i32(&argx, a);
    xdr_put_i32(&argx, b);

    uint8_t resbuf[64];
    xbuf_t resx;
    if (rpc_call(PROC_ADD, &argx, resbuf, sizeof(resbuf), &resx, dump_wire) != 0) {
        fprintf(stderr, "rpc_add failed\n"); exit(1);
    }
    int32_t result;
    xdr_get_i32(&resx, &result);
    return result;
}

void rpc_reverse(const char *s, char *out, size_t outcap, int dump_wire) {
    uint8_t argbuf[512];
    xbuf_t argx; xbuf_init(&argx, argbuf, sizeof(argbuf));
    xdr_put_string(&argx, s);

    uint8_t resbuf[512];
    xbuf_t resx;
    if (rpc_call(PROC_REVERSE, &argx, resbuf, sizeof(resbuf), &resx, dump_wire) != 0) {
        fprintf(stderr, "rpc_reverse failed\n"); exit(1);
    }
    xdr_get_string(&resx, out, outcap - 1);
}

int main(int argc, char *argv[]) {
    const char *host = argc > 1 ? argv[1] : "127.0.0.1";
    int port = argc > 2 ? atoi(argv[2]) : 9500;

    g_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, host, &addr.sin_addr);

    if (connect(g_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect"); return 1;
    }
    printf("[client] connected to %s:%d\n\n", host, port);

    printf("[client] calling rpc_add(3, 4)  -- looks like an ordinary function call --\n");
    int32_t sum = rpc_add(3, 4, 1 /* dump the wire bytes for this one */);
    printf("[client] rpc_add(3, 4) = %d\n\n", sum);

    printf("[client] calling rpc_add(100, 250)\n");
    int32_t sum2 = rpc_add(100, 250, 0);
    printf("[client] rpc_add(100, 250) = %d\n\n", sum2);

    char rev[256];
    printf("[client] calling rpc_reverse(\"Hello, RPC!\")\n");
    rpc_reverse("Hello, RPC!", rev, sizeof(rev), 1);
    printf("[client] rpc_reverse(\"Hello, RPC!\") = \"%s\"\n\n", rev);

    close(g_sock);
    return 0;
}