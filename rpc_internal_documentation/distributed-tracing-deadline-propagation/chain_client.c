/*
 * chain_client.c -- initiates one call into the chain: client -> A -> B -> C.
 *
 * Generates ONE trace_id and ONE absolute deadline here, at the very
 * start -- neither is ever regenerated at any hop. Run with an
 * argument in milliseconds to set how much total time the WHOLE
 * chain is allowed: try something generous (like 2000) to see it
 * succeed, or something too tight (like 200, less than SVC-C's own
 * simulated 300ms of work) to see the deadline get enforced.
 */
#include "chain_middleware.h"
#include <time.h>

#define SVC_A_PORT 9303

int main(int argc, char *argv[]) {
    int budget_ms = argc > 1 ? atoi(argv[1]) : 2000;
    int32_t input = argc > 2 ? atoi(argv[2]) : 10;

    srand((unsigned)time(NULL) ^ (unsigned)getpid());
    uint64_t trace_id = ((uint64_t)rand() << 32) | (uint64_t)rand();
    int64_t deadline_ms = now_ms() + budget_ms;

    printf("[client] trace=%016llx: starting chain call with input=%d, "
           "total budget=%dms (absolute deadline=%lld)\n",
           (unsigned long long)trace_id, input, budget_ms, (long long)deadline_ms);

    uint8_t argbuf[16];
    xbuf_t argx; xbuf_init(&argx, argbuf, sizeof(argbuf));
    put_u32(&argx, (uint32_t)input);

    uint8_t resbuf[64];
    xbuf_t resx;
    int64_t t0 = now_ms();
    uint32_t status = chain_call("127.0.0.1", SVC_A_PORT, trace_id, deadline_ms,
                                  0, 0, 0, &argx, resbuf, sizeof(resbuf), &resx, "client");
    int64_t elapsed = now_ms() - t0;

    if (status == STATUS_OK) {
        int32_t result = (int32_t)get_u32(&resx);
        printf("[client] trace=%016llx: SUCCESS, result=%d (took %lldms)\n",
               (unsigned long long)trace_id, result, (long long)elapsed);
    } else if (status == STATUS_DEADLINE_EXCEEDED) {
        printf("[client] trace=%016llx: DEADLINE EXCEEDED somewhere in the chain "
               "(took %lldms, budget was %dms)\n",
               (unsigned long long)trace_id, (long long)elapsed, budget_ms);
    } else {
        printf("[client] trace=%016llx: ERROR, status=%u\n",
               (unsigned long long)trace_id, status);
    }
    return 0;
}
