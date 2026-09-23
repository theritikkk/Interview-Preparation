/*
 * svc_b.c -- the middle hop. Does a little of its own work, then
 * calls C -- passing the SAME trace_id and the SAME absolute
 * deadline it itself received, unchanged.
 */
#include "chain_middleware.h"

#define SVC_C_PORT 9301

static uint32_t handler_b(uint64_t trace_id, int64_t deadline_ms,
                           xbuf_t *argx, xbuf_t *resx, const char *svcname) {
    int32_t x = (int32_t)get_u32(argx);
    printf("  [%s] trace=%016llx: got %d, calling SVC-C...\n",
           svcname, (unsigned long long)trace_id, x);

    uint8_t argbuf[16];
    xbuf_t cargx; xbuf_init(&cargx, argbuf, sizeof(argbuf));
    put_u32(&cargx, (uint32_t)x);

    uint8_t resbuf[64];
    xbuf_t cresx;
    uint32_t status = chain_call("127.0.0.1", SVC_C_PORT, trace_id, deadline_ms,
                                  0, 0, 0, &cargx, resbuf, sizeof(resbuf), &cresx, svcname);
    if (status != STATUS_OK) {
        printf("  [%s] trace=%016llx: downstream call failed, status=%u -- "
               "propagating failure upward, NOT doing any more work\n",
               svcname, (unsigned long long)trace_id, status);
        return status;
    }

    int32_t c_result = (int32_t)get_u32(&cresx);
    int32_t final = c_result + 1; /* B's own small contribution */
    put_u32(resx, (uint32_t)final);
    printf("  [%s] trace=%016llx: got %d from SVC-C, returning %d\n",
           svcname, (unsigned long long)trace_id, c_result, final);
    return STATUS_OK;
}

int main(int argc, char *argv[]) {
    int port = argc > 1 ? atoi(argv[1]) : 9302;
    serve_forever(port, "SVC-B", handler_b);
    return 0;
}
