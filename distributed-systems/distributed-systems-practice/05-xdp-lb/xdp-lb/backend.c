// backend.c — a minimal, real HTTP/1.1 server: one of the backends the
// XDP load balancer distributes traffic across. Deliberately simple
// (blocking accept/read/write per connection, no threading) — the point
// of this project is the load balancer in front of it, not the backend
// itself. Each reply names its own port so a client can see, directly in
// the response body, which real backend process actually served it —
// that's how the demo proves traffic really is being distributed, not
// just that the LB doesn't crash.
//
// usage: ./backend <port>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <errno.h>

static volatile sig_atomic_t exiting = 0;
static void on_signal(int sig) { (void)sig; exiting = 1; }

int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		return 1;
	}
	int port = atoi(argv[1]);

	int srv = socket(AF_INET, SOCK_STREAM, 0);
	if (srv < 0) { perror("socket"); return 1; }
	int one = 1;
	setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

	struct sockaddr_in addr = {0};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(port);
	if (bind(srv, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind");
		return 1;
	}
	if (listen(srv, 64) < 0) { perror("listen"); return 1; }

	signal(SIGINT, on_signal);
	signal(SIGTERM, on_signal);
	// so accept() wakes up / returns EINTR promptly on signal instead of
	// blocking forever past Ctrl-C
	struct timeval tv = { .tv_sec = 1, .tv_usec = 0 };
	setsockopt(srv, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

	fprintf(stderr, "backend: listening on 127.0.0.1:%d\n", port);

	long served = 0;
	while (!exiting) {
		struct sockaddr_in client;
		socklen_t clen = sizeof(client);
		int c = accept(srv, (struct sockaddr *)&client, &clen);
		if (c < 0) {
			// EAGAIN/EWOULDBLOCK: just the 1s SO_RCVTIMEO poke so the
			// !exiting check above gets re-evaluated after Ctrl-C.
			continue;
		}

		char buf[2048];
		ssize_t n = read(c, buf, sizeof(buf) - 1);
		if (n > 0) {
			buf[n] = '\0';
			served++;
			char client_ip[32];
			inet_ntop(AF_INET, &client.sin_addr, client_ip, sizeof(client_ip));

			char body[256];
			int body_len = snprintf(body, sizeof(body),
				"Hello from backend on port %d (request #%ld, client port %d)\n",
				port, served, ntohs(client.sin_port));

			char resp[512];
			int resp_len = snprintf(resp, sizeof(resp),
				"HTTP/1.1 200 OK\r\n"
				"Content-Type: text/plain\r\n"
				"Content-Length: %d\r\n"
				"Connection: close\r\n"
				"\r\n%s", body_len, body);
			write(c, resp, resp_len);
		}
		close(c);
	}

	fprintf(stderr, "backend on port %d: served %ld requests, exiting\n", port, served);
	close(srv);
	return 0;
}
