/*
 * portmapper.c -- a real, working portmapper, mirroring the actual
 * ONC RPC portmapper protocol (program 100000) closely enough to
 * demonstrate the real registration/lookup flow described in
 * Sun_ONC_RPC_Internals.md section 5 -- just on a non-privileged
 * stand-in port (9111) instead of the real, root-only port 111.
 *
 * Supports exactly two procedures, matching the real protocol:
 *   PMAPPROC_SET(prog, vers, port)     -> bool success
 *   PMAPPROC_GETPORT(prog, vers)       -> uint32 port (0 = not found)
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
#define PMAPPROC_SET     1u
#define PMAPPROC_GETPORT 3u
#define MAXMSG 4096
#define MAX_MAPPINGS 32

typedef struct { uint32_t prog, vers, port; int used; } mapping_t;
static mapping_t g_mappings[MAX_MAPPINGS];

static void pmap_set(uint32_t prog, uint32_t vers, uint32_t port) {
    for (int i = 0; i < MAX_MAPPINGS; i++) {
        if (g_mappings[i].used && g_mappings[i].prog == prog && g_mappings[i].vers == vers) {
            g_mappings[i].port = port;   /* re-registration: update in place */
            printf("  [portmapper] PMAPPROC_SET: updated prog=0x%x vers=%u -> port %u\n",
                   prog, vers, port);
            return;
        }
    }
    for (int i = 0; i < MAX_MAPPINGS; i++) {
        if (!g_mappings[i].used) {
            g_mappings[i] = (mapping_t){ prog, vers, port, 1 };
            printf("  [portmapper] PMAPPROC_SET: registered prog=0x%x vers=%u -> port %u\n",
                   prog, vers, port);
            return;
        }
    }
    printf("  [portmapper] PMAPPROC_SET: mapping table full!\n");
}

static uint32_t pmap_getport(uint32_t prog, uint32_t vers) {
    for (int i = 0; i < MAX_MAPPINGS; i++) {
        if (g_mappings[i].used && g_mappings[i].prog == prog && g_mappings[i].vers == vers) {
            printf("  [portmapper] PMAPPROC_GETPORT: prog=0x%x vers=%u -> port %u\n",
                   prog, vers, g_mappings[i].port);
            return g_mappings[i].port;
        }
    }
    printf("  [portmapper] PMAPPROC_GETPORT: prog=0x%x vers=%u -> NOT REGISTERED (0)\n",
           prog, vers);
    return 0;
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

    xbuf_t argx; xbuf_init(&argx, (uint8_t*)(msg + in.len), arglen);
    uint32_t prog, vers;
    xdr_get_u32(&argx, &prog);
    xdr_get_u32(&argx, &vers);

    if (proc == PMAPPROC_SET) {
        uint32_t port;
        xdr_get_u32(&argx, &port);
        pmap_set(prog, vers, port);
        xdr_put_u32(&resx, 1); /* bool success */
    } else if (proc == PMAPPROC_GETPORT) {
        uint32_t port = pmap_getport(prog, vers);
        xdr_put_u32(&resx, port);
    } else {
        status = RPC_ERR_NO_PROC;
    }

    xbuf_t out; xbuf_init(&out, reply, cap);
    xdr_put_u32(&out, xid);
    xdr_put_u32(&out, MSG_REPLY);
    xdr_put_u32(&out, status);
    xdr_put_u32(&out, (uint32_t)resx.len);
    memcpy(out.buf + out.len, resx.buf, resx.len);
    out.len += resx.len;
    return (uint32_t)out.len;
}

int main(void) {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    int yes = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PMAP_WELLKNOWN_PORT);
    bind(listener, (struct sockaddr*)&addr, sizeof(addr));
    listen(listener, 10);

    printf("[portmapper] listening on the well-known port %d\n\n", PMAP_WELLKNOWN_PORT);
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