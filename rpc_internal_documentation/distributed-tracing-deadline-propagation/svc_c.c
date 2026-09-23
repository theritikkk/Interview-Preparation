/*
 * svc_c.c -- the leaf of the chain. This is where actual work
 * happens -- everything upstream (A, B) is just routing plus
 * deadline/trace-id plumbing.
 */
#include "chain_middleware.h"

static uint32_t handler_c(uint64_t trace_id, int64_t deadline_ms,
                           xbuf_t *argx, xbuf_t *resx, const char *svcname) {
    (void)deadline_ms;
    int32_t x = (int32_t)get_u32(argx);
    printf("  [%s] trace=%016llx: doing real work on input %d (this takes 300ms)...\n",
           svcname, (unsigned long long)trace_id, x);
    usleep(300000); /* simulate genuinely slow, real processing */
    int32_t result = x * 2;
    put_u32(resx, (uint32_t)result);
    printf("  [%s] trace=%016llx: work done, result=%d\n",
           svcname, (unsigned long long)trace_id, result);
    return STATUS_OK;
}

int main(int argc, char *argv[]) {
    int port = argc > 1 ? atoi(argv[1]) : 9301;
    serve_forever(port, "SVC-C", handler_c);
    return 0;
}
