/*
 * client_nopool.c -- the naive, common-mistake baseline: a brand new
 * TCP connection (full handshake) for every single call, even though
 * every call is going to the exact same server. This is exactly
 * chain_call() from the previous demo's own behavior -- flagged
 * there as a real, named limitation, and now measured directly.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define PORT 9401
#define NUM_CALLS 50

static int64_t now_ms(void) {
    struct timeval tv; gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static int32_t call_once(int32_t x) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    connect(sock, (struct sockaddr*)&addr, sizeof(addr));   /* FULL HANDSHAKE, every time */

    uint8_t buf[16];
    memcpy(buf, &x, 4);
    send(sock, buf, 4, 0);
    recv(sock, buf, 4, 0);
    int32_t result; memcpy(&result, buf, 4);

    close(sock);   /* thrown away immediately -- next call repeats the whole handshake */
    return result;
}

int main(void) {
    printf("[no-pool] making %d calls, each with its OWN fresh connection...\n", NUM_CALLS);
    int64_t t0 = now_ms();
    for (int i = 0; i < NUM_CALLS; i++) {
        call_once(i);
    }
    int64_t elapsed = now_ms() - t0;
    printf("[no-pool] %d calls took %lldms total (%.2fms average per call)\n",
           NUM_CALLS, (long long)elapsed, (double)elapsed / NUM_CALLS);
    return 0;
}
