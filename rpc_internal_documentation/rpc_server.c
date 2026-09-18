/*
 * rpc_server.c -- the server side: listen, accept, and for every
 * incoming CALL message, dispatch to the right handler purely by
 * procedure number, execute it, and marshal a REPLY back.
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

/* ---- The actual "business logic" being exposed remotely ------------
 * Notice these are completely ordinary C functions. Nothing about them
 * is RPC-aware. That separation -- real logic vs. RPC plumbing -- is
 * exactly what a real IDL compiler's generated "skeleton" code exists
 * to preserve: the programmer just writes int add(int,int); the
 * generated skeleton is what actually knows about XDR and dispatch.
 */
static int32_t handler_add(int32_t a, int32_t b) {
    return a + b;
}

static void handler_reverse(const char *in, char *out /* same length */) {
    size_t n = strlen(in);
    for (size_t i = 0; i < n; i++) out[i] = in[n - 1 - i];
    out[n] = 0;
}

/* ---- Dispatch: turn a wire-level procedure number into a call ------
 * This function IS the "server skeleton" -- decode this procedure's
 * specific argument shape, call the real handler, encode its specific
 * result shape. A real RPC-generation tool writes one of these per
 * procedure, automatically, from an IDL definition; here it's just
 * written by hand, which is exactly what that generated code is doing
 * under the hood.
 */
static uint32_t dispatch(uint32_t procedure, xbuf_t *argx, xbuf_t *resx) {
    switch (procedure) {

    case PROC_ADD: {
        int32_t a, b;
        if (xdr_get_i32(argx, &a) != 0) return RPC_ERR_DECODE;
        if (xdr_get_i32(argx, &b) != 0) return RPC_ERR_DECODE;
        printf("  [server] dispatch PROC_ADD(%d, %d)\n", a, b);
        int32_t result = handler_add(a, b);
        xdr_put_i32(resx, result);
        return RPC_SUCCESS;
    }

    case PROC_REVERSE: {
        char s[256], rev[256];
        if (xdr_get_string(argx, s, sizeof(s) - 1) != 0) return RPC_ERR_DECODE;
        printf("  [server] dispatch PROC_REVERSE(\"%s\")\n", s);
        handler_reverse(s, rev);
        xdr_put_string(resx, rev);
        return RPC_SUCCESS;
    }

    default:
        printf("  [server] dispatch: unknown procedure %u\n", procedure);
        return RPC_ERR_NO_PROC;
    }
}

/* Handle one full CALL message already sitting in msg[0..msglen),
   producing a complete REPLY message in reply[], returning its length. */
static uint32_t handle_call(const uint8_t *msg, uint32_t msglen,
                             uint8_t *reply, uint32_t reply_cap) {
    xbuf_t in;
    xbuf_init(&in, (uint8_t*)msg, msglen);

    uint32_t xid, msg_type, program, version, procedure, arg_len;
    xdr_get_u32(&in, &xid);
    xdr_get_u32(&in, &msg_type);
    xdr_get_u32(&in, &program);
    xdr_get_u32(&in, &version);
    xdr_get_u32(&in, &procedure);
    xdr_get_u32(&in, &arg_len);

    printf("[server] CALL received: xid=%u program=0x%x version=%u procedure=%u arg_len=%u\n",
           xid, program, version, procedure, arg_len);

    uint32_t status;
    uint8_t resbuf[MAXMSG];
    xbuf_t resx;
    xbuf_init(&resx, resbuf, sizeof(resbuf));

    if (program != DEMO_PROGRAM) {
        status = RPC_ERR_NO_PROC;
    } else if (version != DEMO_VERSION) {
        status = RPC_ERR_BAD_VERSION;
    } else {
        xbuf_t argx;
        xbuf_init(&argx, (uint8_t*)(msg + in.len), arg_len);
        status = dispatch(procedure, &argx, &resx);
    }

    xbuf_t out;
    xbuf_init(&out, reply, reply_cap);
    xdr_put_u32(&out, xid);            /* echo the xid back */
    xdr_put_u32(&out, MSG_REPLY);
    xdr_put_u32(&out, status);
    xdr_put_u32(&out, (uint32_t)resx.len);
    memcpy(out.buf + out.len, resx.buf, resx.len);
    out.len += resx.len;

    printf("[server] REPLY sent:    xid=%u status=%u result_len=%zu\n\n",
           xid, status, resx.len);

    return (uint32_t)out.len;
}

int main(int argc, char *argv[]) {
    int port = argc > 1 ? atoi(argv[1]) : 9500;

    int listener = socket(AF_INET, SOCK_STREAM, 0);
    int yes = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(listener, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); return 1;
    }
    listen(listener, 10);
    printf("[server] listening on port %d, program=0x%x version=%u\n\n",
           port, DEMO_PROGRAM, DEMO_VERSION);
    fflush(stdout);

    while (1) {
        struct sockaddr_in cli;
        socklen_t clilen = sizeof(cli);
        int client = accept(listener, (struct sockaddr*)&cli, &clilen);
        if (client < 0) continue;
        printf("[server] connection from %s\n", inet_ntoa(cli.sin_addr));

        uint8_t msg[MAXMSG];
        for (;;) {
            int32_t n = recv_framed(client, msg, sizeof(msg));
            if (n < 0) break;   /* client disconnected */

            uint8_t reply[MAXMSG];
            uint32_t replylen = handle_call(msg, (uint32_t)n, reply, sizeof(reply));
            fflush(stdout);
            if (send_framed(client, reply, replylen) != 0) break;
        }
        printf("[server] client disconnected\n\n");
        fflush(stdout);
        close(client);
    }
    return 0;
}