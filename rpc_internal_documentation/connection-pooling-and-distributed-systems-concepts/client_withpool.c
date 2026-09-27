/*
 * client_withpool.c -- a real, thread-safe connection pool: a fixed
 * set of connections established ONCE, borrowed and returned by
 * multiple concurrent threads, with a mutex + condition variable to
 * make a thread BLOCK (not spin, not fail) when every connection is
 * currently in use.
 *
 * This is close to word-for-word "implement a connection pool," one
 * of the most common systems/backend interview coding questions --
 * the three things an interviewer is actually checking for are all
 * here: (1) correct mutual exclusion around the shared pool state,
 * (2) blocking (not busy-waiting) when the pool is exhausted, and
 * (3) no connection ever handed to two threads at once.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define PORT 9401
#define POOL_SIZE 4
#define NUM_CALLS 50
#define NUM_THREADS 8

static int64_t now_ms(void) {
    struct timeval tv; gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

/* ---- The pool itself --------------------------------------------- */
typedef struct {
    int conns[POOL_SIZE];
    int in_use[POOL_SIZE];
    pthread_mutex_t lock;
    pthread_cond_t available;   /* signaled whenever a connection is returned */
} pool_t;

static pool_t g_pool;

static void pool_init(pool_t *p) {
    pthread_mutex_init(&p->lock, NULL);
    pthread_cond_init(&p->available, NULL);
    for (int i = 0; i < POOL_SIZE; i++) {
        p->conns[i] = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(PORT);
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
        connect(p->conns[i], (struct sockaddr*)&addr, sizeof(addr));  /* handshake happens ONCE */
        p->in_use[i] = 0;
    }
    printf("[pool] established %d real connections up front\n", POOL_SIZE);
}

/* Borrow a connection -- BLOCKS (does not spin, does not fail) if
   every single one is currently checked out by another thread. */
static int pool_borrow(pool_t *p, int *slot_out) {
    pthread_mutex_lock(&p->lock);
    int slot = -1;
    while (slot == -1) {
        for (int i = 0; i < POOL_SIZE; i++) {
            if (!p->in_use[i]) { slot = i; break; }
        }
        if (slot == -1) {
            /* every connection busy -- sleep here, releasing the lock,
               until pool_return() wakes us back up */
            pthread_cond_wait(&p->available, &p->lock);
        }
    }
    p->in_use[slot] = 1;
    int fd = p->conns[slot];
    pthread_mutex_unlock(&p->lock);
    *slot_out = slot;
    return fd;
}

static void pool_return(pool_t *p, int slot) {
    pthread_mutex_lock(&p->lock);
    p->in_use[slot] = 0;
    pthread_cond_signal(&p->available);   /* wake exactly one waiting borrower */
    pthread_mutex_unlock(&p->lock);
}

/* ---- Worker threads, each making several calls -------------------- */
static void *worker(void *arg) {
    int thread_id = *(int*)arg;
    int calls_per_thread = NUM_CALLS / NUM_THREADS;

    for (int i = 0; i < calls_per_thread; i++) {
        int slot;
        int fd = pool_borrow(&g_pool, &slot);   /* may BLOCK here */

        int32_t x = thread_id * 1000 + i;
        uint8_t buf[16];
        memcpy(buf, &x, 4);
        send(fd, buf, 4, 0);
        recv(fd, buf, 4, 0);

        pool_return(&g_pool, slot);
    }
    return NULL;
}

int main(void) {
    pool_init(&g_pool);

    printf("[pool] %d threads making %d total calls through only %d connections...\n",
           NUM_THREADS, NUM_CALLS, POOL_SIZE);

    pthread_t threads[NUM_THREADS];
    int ids[NUM_THREADS];
    int64_t t0 = now_ms();
    for (int i = 0; i < NUM_THREADS; i++) {
        ids[i] = i;
        pthread_create(&threads[i], NULL, worker, &ids[i]);
    }
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    int64_t elapsed = now_ms() - t0;

    printf("[pool] %d calls (across %d threads, %d pooled connections) took "
           "%lldms total (%.2fms average per call)\n",
           NUM_CALLS, NUM_THREADS, POOL_SIZE, (long long)elapsed, (double)elapsed / NUM_CALLS);
    return 0;
}
