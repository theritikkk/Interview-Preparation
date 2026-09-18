/*
 * idem_client.c -- simulates the exact real-world scenario that makes
 * exactly-once delivery impossible: a client makes a call, then loses
 * track of whether it succeeded (here: it simply disconnects right
 * after sending, without waiting for the reply -- indistinguishable,
 * from the SERVER's perspective, from a reply that was sent but lost
 * in transit). Not knowing whether the original attempt succeeded,
 * the client retries with the IDENTICAL idempotency key.
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

#define PROC_INCR 3u
#define MAXMSG 4096

static int32_t call_incr(int port, const char *key, int abandon_before_reply) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    connect(sock, (struct sockaddr*)&addr, sizeof(addr));

    uint8_t argbuf[256];
    xbuf_t argx; xbuf_init(&argx, argbuf, sizeof(argbuf));
    xdr_put_string(&argx, key);

    uint8_t msg[MAXMSG];
    xbuf_t out; xbuf_init(&out, msg, sizeof(msg));
    xdr_put_u32(&out, 1); /* xid, doesn't matter for this demo */
    xdr_put_u32(&out, MSG_CALL);
    xdr_put_u32(&out, DEMO_PROGRAM);
    xdr_put_u32(&out, DEMO_VERSION);
    xdr_put_u32(&out, PROC_INCR);
    xdr_put_u32(&out, (uint32_t)argx.len);
    memcpy(out.buf + out.len, argx.buf, argx.len);
    out.len += argx.len;

    send_framed(sock, out.buf, (uint32_t)out.len);

    if (abandon_before_reply) {
        /* Simulate: the client's timeout fires, or the network drops
           the reply, or the process itself is killed -- from the
           SERVER's point of view, the request was fully received and
           processed; from the CLIENT's point of view, it simply never
           got an answer, and has NO WAY to tell the difference between
           "my request never arrived" and "it arrived, ran, but the
           reply was lost." That ambiguity is the entire subject of
           the accompanying document. */
        close(sock);
        printf("  [client] (connection abandoned before reading a reply --\n"
               "            simulates a lost reply / client-side timeout)\n");
        return -1;
    }

    uint8_t reply[MAXMSG];
    int32_t n = recv_framed(sock, reply, sizeof(reply));
    close(sock);
    if (n < 0) return -1;

    xbuf_t in; xbuf_init(&in, reply, (uint32_t)n);
    uint32_t xid, mtype, status, rlen;
    xdr_get_u32(&in, &xid); xdr_get_u32(&in, &mtype);
    xdr_get_u32(&in, &status); xdr_get_u32(&in, &rlen);
    int32_t result;
    xdr_get_i32(&in, &result);
    return result;
}

int main(int argc, char *argv[]) {
    int port = argc > 1 ? atoi(argv[1]) : 9600;

    printf("[client] Attempt 1: call INCR(\"txn-abc123\") -- but we abandon\n"
           "         the connection before reading the reply.\n");
    call_incr(port, "txn-abc123", 1);
    printf("\n");

    sleep(1);

    printf("[client] Not knowing whether attempt 1 actually succeeded,\n"
           "         we retry with the SAME idempotency key: \"txn-abc123\"\n");
    int32_t r1 = call_incr(port, "txn-abc123", 0);
    printf("[client] retry of txn-abc123 returned: %d\n\n", r1);

    printf("[client] Now a genuinely NEW, distinct request: INCR(\"txn-xyz789\")\n");
    int32_t r2 = call_incr(port, "txn-xyz789", 0);
    printf("[client] txn-xyz789 returned: %d\n\n", r2);

    printf("[client] Correct final state: exactly TWO distinct increments happened\n"
           "         (txn-abc123 once, txn-xyz789 once) -- so the counter should be 2.\n");
    return 0;
}