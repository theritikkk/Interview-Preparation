/*
 * svc_a.c -- the front door of the chain. This is what the client
 * actually calls; everything else (B, C) is invisible to the client
 * entirely.
 */
#include "chain_middleware.h"

#define SVC_B_PORT 9302

static uint32_t handler_a(uint64_t trace_id, int64_t deadline_ms,
                           xbuf_t *argx, xbuf_t *resx, const char *svcname) {
    int32_t x = (int32_t)get_u32(argx);
    printf("  [%s] trace=%016llx: got %d, calling SVC-B...\n",
           svcname, (unsigned long long)trace_id, x);

    uint8_t argbuf[16];
    xbuf_t cargx; xbuf_init(&cargx, argbuf, sizeof(argbuf));
    put_u32(&cargx, (uint32_t)x);

    uint8_t resbuf[64];
    xbuf_t cresx;
    uint32_t status = chain_call("127.0.0.1", SVC_B_PORT, trace_id, deadline_ms,
                                  0, 0, 0, &cargx, resbuf, sizeof(resbuf), &cresx, svcname);
    if (status != STATUS_OK) {
        printf("  [%s] trace=%016llx: downstream call failed, status=%u -- "
               "propagating failure upward to the client\n",
               svcname, (unsigned long long)trace_id, status);
        return status;
    }

    int32_t b_result = (int32_t)get_u32(&cresx);
    int32_t final = b_result + 1; /* A's own small contribution */
    put_u32(resx, (uint32_t)final);
    printf("  [%s] trace=%016llx: got %d from SVC-B, returning %d\n",
           svcname, (unsigned long long)trace_id, b_result, final);
    return STATUS_OK;
}

int main(int argc, char *argv[]) {
    int port = argc > 1 ? atoi(argv[1]) : 9303;
    serve_forever(port, "SVC-A", handler_a);
    return 0;
}
