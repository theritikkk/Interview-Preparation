#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <net/if.h>
#include <linux/if_link.h>

#include "libbpf.h"
#include "bpf.h"

static volatile sig_atomic_t exiting = 0;
static void on_signal(int sig) { (void)sig; exiting = 1; }

int main(int argc, char **argv)
{
	if (argc < 2) { fprintf(stderr, "usage: %s <ifname> [duration]\n", argv[0]); return 1; }
	const char *ifname = argv[1];
	int duration = argc > 2 ? atoi(argv[2]) : 0;
	int ifindex = if_nametoindex(ifname);
	if (!ifindex) { fprintf(stderr, "no such interface\n"); return 1; }

	struct bpf_object *obj = bpf_object__open_file("xdp_framing_probe.bpf.o", NULL);
	if (libbpf_get_error(obj)) { fprintf(stderr, "open failed\n"); return 1; }
	if (bpf_object__load(obj)) { fprintf(stderr, "load failed\n"); return 1; }
	struct bpf_program *prog = bpf_object__find_program_by_name(obj, "xdp_framing_probe");
	int prog_fd = bpf_program__fd(prog);

	int modes[2] = { XDP_FLAGS_DRV_MODE, XDP_FLAGS_SKB_MODE };
	int attached = -1;
	for (int i = 0; i < 2; i++) {
		if (bpf_xdp_attach(ifindex, prog_fd, modes[i], NULL) == 0) { attached = i; break; }
	}
	if (attached < 0) { fprintf(stderr, "attach failed\n"); return 1; }

	struct bpf_map *map = bpf_object__find_map_by_name(obj, "counts");
	int map_fd = bpf_map__fd(map);

	signal(SIGINT, on_signal);
	signal(SIGTERM, on_signal);
	time_t start = time(NULL);
	while (!exiting) {
		sleep(1);
		__u32 k0 = 0, k1 = 1;
		__u64 raw_ip = 0, eth_ip = 0;
		bpf_map_lookup_elem(map_fd, &k0, &raw_ip);
		bpf_map_lookup_elem(map_fd, &k1, &eth_ip);
		printf("\rraw-IP-at-offset-0: %-6llu   eth-then-IP: %-6llu",
		       (unsigned long long)raw_ip, (unsigned long long)eth_ip);
		fflush(stdout);
		if (duration > 0 && (time(NULL) - start) >= duration) break;
	}
	printf("\n");
	bpf_xdp_attach(ifindex, -1, modes[attached], NULL);
	bpf_object__close(obj);
	return 0;
}
