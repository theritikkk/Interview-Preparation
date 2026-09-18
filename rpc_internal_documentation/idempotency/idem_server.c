/*
 * idem_server.c -- demonstrates the at-least-once duplicate-execution
 * problem, live, and its standard fix (a server-side idempotency-key
 * cache).
 *
 * PROC_INCR(key: string) -> int32: increments a global counter and
 * returns its NEW value.
 *
 * Run with no arguments: UNSAFE mode -- every call increments the
 * counter, even if it's a retry of a call that already succeeded.
 *
 * Run with "--dedup": SAFE mode -- the server remembers every key
 * it has already seen, and a repeated key returns the ORIGINAL
 * result without incrementing the counter again.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "rpc.h"
#include "framing.h"

#define MAXMSG 4096
#define PROC_INCR 3u
#define MAX_CACHE 64

static int32_t g_counter = 0;
static int g_dedup_mode = 0;

typedef struct { char key[128]; int32_t result; int used; } cache_entry_t;
static cache_entry_t g_cache[MAX_CACHE];
static int g_cache_count = 0;

static int32_t find_cached(const char *key) {
    for (int i = 0; i < g_cache_count; i++) {
        if (strcmp(g_cache[i].key, key) == 0) return g_cache[i].result;
    }
    return -1; /* not found */
}

static void store_cached(const char *key, int32_t result) {
    if (g_cache_count >= MAX_CACHE) return;
    strncpy(g_cache[g_cache_count].key, key, sizeof(g_cache[0].key) - 1);
    g_cache[g_cache_count].result = result;
    g_cache_count++;
}

static int32_t handle_incr(const char *key) {
    if (g_dedup_mode) {
        int32_t cached = find_cached(key);
        if (cached >= 0) {
            printf("  [server] key=\"%s\" ALREADY SEEN -- returning cached result %d "
                   "WITHOUT incrementing (counter stays at %d)\n", key, cached, g_counter);
            return cached;
        }
    }
    g_counter++;
    printf("  [server] key=\"%s\" is new -- incrementing counter to %d\n", key, g_counter);
    if (g_dedup_mode) store_cached(key, g_counter);
    return g_counter;
}

static uint32_t handle_call(const uint8_t *msg, uint32_t msglen,
                             uint8_t *reply, uint32_t reply_cap) {
    xbuf_t in; xbuf_init(&in, (uint8_t*)msg, msglen);
    uint32_t xid, msg_type, program, version, procedure, arg_len;
    xdr_get_u32(&in, &xid); xdr_get_u32(&in, &msg_type);
    xdr_get_u32(&in, &program); xdr_get_u32(&in, &version);
    xdr_get_u32(&in, &procedure); xdr_get_u32(&in, &arg_len);

    uint8_t resbuf[MAXMSG];
    xbuf_t resx; xbuf_init(&resx, resbuf, sizeof(resbuf));
    uint32_t status = RPC_SUCCESS;

    if (procedure == PROC_INCR) {
        xbuf_t argx; xbuf_init(&argx, (uint8_t*)(msg + in.len), arg_len);
        char key[128];
        xdr_get_string(&argx, key, sizeof(key) - 1);
        int32_t result = handle_incr(key);
        xdr_put_i32(&resx, result);
    } else {
        status = RPC_ERR_NO_PROC;
    }

    xbuf_t out; xbuf_init(&out, reply, reply_cap);
    xdr_put_u32(&out, xid);
    xdr_put_u32(&out, MSG_REPLY);
    xdr_put_u32(&out, status);
    xdr_put_u32(&out, (uint32_t)resx.len);
    memcpy(out.buf + out.len, resx.buf, resx.len);
    out.len += resx.len;
    return (uint32_t)out.len;
}

int main(int argc, char *argv[]) {
    int port = 9600;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--dedup") == 0) g_dedup_mode = 1;
        else port = atoi(argv[i]);
    }

    int listener = socket(AF_INET, SOCK_STREAM, 0);
    int yes = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    bind(listener, (struct sockaddr*)&addr, sizeof(addr));
    listen(listener, 10);

    printf("[server] mode=%s, listening on port %d, counter starts at 0\n\n",
           g_dedup_mode ? "SAFE (idempotency-key dedup)" : "UNSAFE (no dedup)", port);
    fflush(stdout);

    while (1) {
        struct sockaddr_in cli; socklen_t clilen = sizeof(cli);
        int client = accept(listener, (struct sockaddr*)&cli, &clilen);
        if (client < 0) continue;

        uint8_t msg[MAXMSG];
        int32_t n = recv_framed(client, msg, sizeof(msg));
        if (n >= 0) {
            uint8_t reply[MAXMSG];
            uint32_t replylen = handle_call(msg, (uint32_t)n, reply, sizeof(reply));
            fflush(stdout);
            send_framed(client, reply, replylen);
        }
        close(client);
    }
    return 0;
}