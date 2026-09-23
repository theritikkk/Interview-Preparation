/*
 * chain_middleware.h -- the "interceptor" pattern, made concrete.
 *
 * Every real production RPC framework (gRPC interceptors, Java
 * servlet filters, ASP.NET middleware -- the name varies, the idea
 * doesn't) wraps EVERY call with the same generic logic BEFORE the
 * actual business-logic handler ever runs, so that cross-cutting
 * concerns (here: deadline enforcement and trace-id logging) are
 * handled in exactly ONE place, rather than every single handler
 * needing to remember to re-implement them.
 *
 * serve_forever() below IS that generic wrapper: it does the
 * accept/receive/parse/deadline-check/log dance identically for every
 * service in this chain, and only calls into the service-specific
 * `handler` function pointer for the part that's actually unique to
 * that one service.
 */

#ifndef CHAIN_MIDDLEWARE_H
#define CHAIN_MIDDLEWARE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "chain_rpc.h"
#include "chain_framing.h"

#define MAXMSG 4096

typedef uint32_t (*handler_fn)(uint64_t trace_id, int64_t deadline_ms,
                                xbuf_t *argx, xbuf_t *resx, const char *svcname);

/* The generic outbound-call helper: every hop uses THIS to call the
   next one, and it's the single place that (a) actually enforces
   the deadline before even bothering to connect, and (b) propagates
   trace_id and deadline onto the wire unchanged. */
static uint32_t chain_call(const char *host, int port,
                            uint64_t trace_id, int64_t deadline_ms,
                            uint32_t program, uint32_t version, uint32_t procedure,
                            xbuf_t *argx, uint8_t *resbuf, uint32_t resbuf_cap, xbuf_t *resx,
                            const char *from_svc) {

    int64_t remaining = deadline_ms - now_ms();
    if (remaining <= 0) {
        printf("  [%s] trace=%016llx: deadline ALREADY passed (%lldms overdue) -- "
               "refusing to even connect downstream\n",
               from_svc, (unsigned long long)trace_id, (long long)(-remaining));
        return STATUS_DEADLINE_EXCEEDED;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    inet_pton(AF_INET, host, &addr.sin_addr);
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        return STATUS_ERROR;
    }

    uint8_t msg[MAXMSG];
    xbuf_t out; xbuf_init(&out, msg, sizeof(msg));
    put_u32(&out, 1); put_u32(&out, MSG_CALL);
    put_u64(&out, trace_id);
    put_i64(&out, deadline_ms);       /* <-- SAME absolute deadline, not a fresh one */
    put_u32(&out, program); put_u32(&out, version); put_u32(&out, procedure);
    put_u32(&out, (uint32_t)argx->len);
    memcpy(out.buf + out.len, argx->buf, argx->len);
    out.len += argx->len;

    printf("  [%s] trace=%016llx: -> calling downstream (deadline in %lldms)\n",
           from_svc, (unsigned long long)trace_id, (long long)remaining);

    if (send_framed(sock, out.buf, (uint32_t)out.len) != 0) { close(sock); return STATUS_ERROR; }

    uint8_t reply[MAXMSG];
    int32_t n = recv_framed(sock, reply, sizeof(reply));
    close(sock);
    if (n < 0) return STATUS_ERROR;

    xbuf_t in; xbuf_init(&in, reply, (uint32_t)n);
    get_u32(&in); get_u32(&in);           /* xid, msg_type -- ignored here */
    uint32_t status = get_u32(&in);
    uint32_t rlen = get_u32(&in);
    if (rlen > resbuf_cap) return STATUS_ERROR;
    memcpy(resbuf, reply + in.len, rlen);
    xbuf_init(resx, resbuf, rlen);
    return status;
}

/* The generic server-side wrapper -- the actual "interceptor." */
static void serve_forever(int port, const char *svcname, handler_fn handler) {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    int yes = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons((uint16_t)port);
    bind(listener, (struct sockaddr*)&addr, sizeof(addr));
    listen(listener, 10);
    printf("[%s] listening on port %d\n", svcname, port);
    fflush(stdout);

    while (1) {
        struct sockaddr_in cli; socklen_t clilen = sizeof(cli);
        int client = accept(listener, (struct sockaddr*)&cli, &clilen);
        if (client < 0) continue;

        uint8_t msg[MAXMSG];
        int32_t n = recv_framed(client, msg, sizeof(msg));
        if (n >= 0) {
            xbuf_t in; xbuf_init(&in, msg, (uint32_t)n);
            uint32_t xid = get_u32(&in);
            get_u32(&in); /* msg_type */
            uint64_t trace_id = get_u64(&in);
            int64_t deadline_ms = get_i64(&in);
            get_u32(&in); get_u32(&in); get_u32(&in); /* program, version, procedure -- this
                                                          tiny demo only ever has one procedure
                                                          per service, so they're read but not
                                                          branched on */
            uint32_t arg_len = get_u32(&in);
            xbuf_t argx; xbuf_init(&argx, msg + in.len, arg_len);

            int64_t remaining = deadline_ms - now_ms();
            uint32_t status;
            uint8_t resbuf[MAXMSG];
            xbuf_t resx; xbuf_init(&resx, resbuf, sizeof(resbuf));

            printf("[%s] trace=%016llx: incoming call, %lldms left on the deadline\n",
                   svcname, (unsigned long long)trace_id, (long long)remaining);

            if (remaining <= 0) {
                /* THE interceptor's central job: refuse BEFORE doing
                   any real work at all, and before calling further
                   downstream services -- exactly what a check buried
                   inside every individual handler would be easy to
                   forget to do consistently. */
                printf("[%s] trace=%016llx: deadline already exceeded -- "
                       "REFUSING without running the handler\n",
                       svcname, (unsigned long long)trace_id);
                status = STATUS_DEADLINE_EXCEEDED;
            } else {
                status = handler(trace_id, deadline_ms, &argx, &resx, svcname);

                /* The check most naive implementations forget: even
                   though the deadline hadn't passed when this call
                   STARTED, this hop's own work (or a downstream
                   hop's) might have taken long enough that it HAS
                   passed by the time we're about to reply. Sending
                   back STATUS_OK at this point would tell the caller
                   "everything's fine" for a result that's already
                   too late to be useful -- so re-check right here,
                   after the real work, before the reply goes out. */
                if (status == STATUS_OK && now_ms() > deadline_ms) {
                    printf("[%s] trace=%016llx: handler finished, but the deadline "
                           "passed WHILE we were working -- downgrading reply to "
                           "DEADLINE_EXCEEDED instead of OK\n",
                           svcname, (unsigned long long)trace_id);
                    status = STATUS_DEADLINE_EXCEEDED;
                }
            }

            uint8_t reply[MAXMSG];
            xbuf_t out; xbuf_init(&out, reply, sizeof(reply));
            put_u32(&out, xid); put_u32(&out, MSG_REPLY);
            put_u32(&out, status); put_u32(&out, (uint32_t)resx.len);
            memcpy(out.buf + out.len, resx.buf, resx.len);
            out.len += resx.len;
            fflush(stdout);
            send_framed(client, out.buf, (uint32_t)out.len);
        }
        close(client);
    }
}

#endif
