/*
 * pool_server.c -- unlike every earlier demo in this series (which
 * closed the connection after ONE call), this server keeps a
 * connection open across MANY requests. That's the whole
 * precondition for connection pooling to make sense at all: pooling
 * only pays off if a connection can be reused, not if the server
 * itself throws it away after every call anyway.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 9401

static void *client_thread(void *arg) {
    int sock = *(int*)arg;
    free(arg);
    uint8_t buf[16];
    for (;;) {
        ssize_t n = recv(sock, buf, sizeof(buf), 0);
        if (n <= 0) break;                 /* client disconnected */
        int32_t x; memcpy(&x, buf, 4);
        usleep(20000);                      /* 20ms of simulated "real work" */
        int32_t result = x * 2;
        memcpy(buf, &result, 4);
        send(sock, buf, 4, 0);
    }
    close(sock);
    return NULL;
}

int main(void) {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    int yes = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);
    bind(listener, (struct sockaddr*)&addr, sizeof(addr));
    listen(listener, 50);
    printf("[pool_server] listening on %d, each connection stays open for MANY requests\n", PORT);
    fflush(stdout);

    while (1) {
        int *sock = malloc(sizeof(int));
        *sock = accept(listener, NULL, NULL);
        if (*sock < 0) { free(sock); continue; }
        pthread_t t;
        pthread_create(&t, NULL, client_thread, sock);
        pthread_detach(t);
    }
}
