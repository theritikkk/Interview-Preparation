#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "rpc.h"
#include "framing.h"

int main(int argc, char *argv[]) {
    int port = argc > 1 ? atoi(argv[1]) : 9500;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    connect(sock, (struct sockaddr*)&addr, sizeof(addr));

    /* Send a CALL with an unknown procedure number (99) */
    uint8_t msg[64];
    xbuf_t out; xbuf_init(&out, msg, sizeof(msg));
    xdr_put_u32(&out, 42);           /* xid */
    xdr_put_u32(&out, MSG_CALL);
    xdr_put_u32(&out, DEMO_PROGRAM);
    xdr_put_u32(&out, DEMO_VERSION);
    xdr_put_u32(&out, 99);           /* unknown procedure */
    xdr_put_u32(&out, 0);            /* no args */
    send_framed(sock, out.buf, (uint32_t)out.len);

    uint8_t reply[256];
    int32_t n = recv_framed(sock, reply, sizeof(reply));
    xbuf_t in; xbuf_init(&in, reply, (uint32_t)n);
    uint32_t rxid, mtype, status, rlen;
    xdr_get_u32(&in, &rxid); xdr_get_u32(&in, &mtype);
    xdr_get_u32(&in, &status); xdr_get_u32(&in, &rlen);
    printf("[client] unknown-procedure call -> xid=%u status=%u (RPC_ERR_NO_PROC=%u)\n",
           rxid, status, RPC_ERR_NO_PROC);

    /* Now send a CALL with a bad version number */
    xbuf_init(&out, msg, sizeof(msg));
    xdr_put_u32(&out, 43);
    xdr_put_u32(&out, MSG_CALL);
    xdr_put_u32(&out, DEMO_PROGRAM);
    xdr_put_u32(&out, 99);            /* wrong version */
    xdr_put_u32(&out, PROC_ADD);
    xdr_put_u32(&out, 0);
    send_framed(sock, out.buf, (uint32_t)out.len);

    n = recv_framed(sock, reply, sizeof(reply));
    xbuf_init(&in, reply, (uint32_t)n);
    xdr_get_u32(&in, &rxid); xdr_get_u32(&in, &mtype);
    xdr_get_u32(&in, &status); xdr_get_u32(&in, &rlen);
    printf("[client] bad-version call    -> xid=%u status=%u (RPC_ERR_BAD_VERSION=%u)\n",
           rxid, status, RPC_ERR_BAD_VERSION);

    close(sock);
    return 0;
}
