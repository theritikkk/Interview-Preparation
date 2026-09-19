/*
 * demo_service.c -- a real RPC service that does NOT know its own port
 * in advance. It lets the OS assign it an ephemeral port, discovers
 * that port with getsockname(), and registers itself with the
 * portmapper via a real PMAPPROC_SET call -- exactly the sequence a
 * real NFS-style daemon runs at boot.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "rpc.h"
#include "framing.h"

#define PMAP_WELLKNOWN_PORT 9111
#define PMAPPROC_SET 1u
#define MAXMSG 4096

static void register_with_portmapper(uint32_t prog, uint32_t vers, uint32_t my_port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PMAP_WELLKNOWN_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    connect(sock, (struct sockaddr*)&addr, sizeof(addr));

    uint8_t argbuf[32];
    xbuf_t argx; xbuf_init(&argx, argbuf, sizeof(argbuf));
    xdr_put_u32(&argx, prog);
    xdr_put_u32(&argx, vers);
    xdr_put_u32(&argx, my_port);

    uint8_t msg[MAXMSG];
    xbuf_t out; xbuf_init(&out, msg, sizeof(msg));
    xdr_put_u32(&out, 1); xdr_put_u32(&out, MSG_CALL);
    xdr_put_u32(&out, 100000u); /* the portmapper's OWN program number */
    xdr_put_u32(&out, 2u);
    xdr_put_u32(&out, PMAPPROC_SET);
    xdr_put_u32(&out, (uint32_t)argx.len);
    memcpy(out.buf + out.len, argx.buf, argx.len);
    out.len += argx.len;

    send_framed(sock, out.buf, (uint32_t)out.len);
    uint8_t reply[MAXMSG];
    recv_framed(sock, reply, sizeof(reply));
    close(sock);

    printf("[service] registered myself with the portmapper: prog=0x%x vers=%u port=%u\n",
           prog, vers, my_port);
}

static uint32_t handle_call(const uint8_t *msg, uint32_t msglen, uint8_t *reply, uint32_t cap) {
    xbuf_t in; xbuf_init(&in, (uint8_t*)msg, msglen);
    uint32_t xid, mtype, program, version, proc, arglen;
    xdr_get_u32(&in, &xid); xdr_get_u32(&in, &mtype);
    xdr_get_u32(&in, &program); xdr_get_u32(&in, &version);
    xdr_get_u32(&in, &proc); xdr_get_u32(&in, &arglen);

    uint8_t resbuf[64];
    xbuf_t resx; xbuf_init(&resx, resbuf, sizeof(resbuf));
    uint32_t status = RPC_SUCCESS;

    if (proc == PROC_ADD) {
        xbuf_t argx; xbuf_init(&argx, (uint8_t*)(msg + in.len), arglen);
        int32_t a, b;
        xdr_get_i32(&argx, &a); xdr_get_i32(&argx, &b);
        printf("  [service] real call arrived DIRECTLY on my ephemeral port: ADD(%d, %d)\n", a, b);
        xdr_put_i32(&resx, a + b);
    } else {
        status = RPC_ERR_NO_PROC;
    }

    xbuf_t out; xbuf_init(&out, reply, cap);
    xdr_put_u32(&out, xid); xdr_put_u32(&out, MSG_REPLY);
    xdr_put_u32(&out, status); xdr_put_u32(&out, (uint32_t)resx.len);
    memcpy(out.buf + out.len, resx.buf, resx.len);
    out.len += resx.len;
    return (uint32_t)out.len;
}

int main(void) {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = 0;   /* <-- port 0: "OS, pick any free port for me" */
    bind(listener, (struct sockaddr*)&addr, sizeof(addr));
    listen(listener, 10);

    /* Discover which port the OS actually gave us -- this is the
       whole reason a portmapper is needed at all: we genuinely don't
       know this value until right now, at runtime. */
    struct sockaddr_in actual;
    socklen_t alen = sizeof(actual);
    getsockname(listener, (struct sockaddr*)&actual, &alen);
    uint32_t my_port = ntohs(actual.sin_port);

    printf("[service] OS assigned me ephemeral port %u (I did not choose this)\n", my_port);

    register_with_portmapper(DEMO_PROGRAM, DEMO_VERSION, my_port);
    printf("[service] now serving real requests on port %u\n\n", my_port);
    fflush(stdout);

    while (1) {
        struct sockaddr_in cli; socklen_t clilen = sizeof(cli);
        int client = accept(listener, (struct sockaddr*)&cli, &clilen);
        if (client < 0) continue;
        uint8_t msg[MAXMSG];
        int32_t n = recv_framed(client, msg, sizeof(msg));
        if (n >= 0) {
            uint8_t reply[MAXMSG];
            uint32_t rl = handle_call(msg, (uint32_t)n, reply, sizeof(reply));
            fflush(stdout);
            send_framed(client, reply, rl);
        }
        close(client);
    }
}