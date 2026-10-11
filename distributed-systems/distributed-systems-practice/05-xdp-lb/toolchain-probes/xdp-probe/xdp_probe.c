// xdp_probe.c — loads xdp_probe.bpf.o and tries to attach it to a named
// interface, trying native (driver) mode first and falling back to
// generic (SKB) mode, exactly like real-world XDP tooling (e.g. xdp-loader)
// does. Prints which mode actually worked, or exactly why neither did.
// Leaves the program attached until Ctrl-C (so packet counts can be
// checked from a second terminal / command), then detaches cleanly.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <net/if.h>
#include <time.h>
#include <linux/if_link.h> // XDP_FLAGS_DRV_MODE / XDP_FLAGS_SKB_MODE

#include "libbpf.h"
#include "bpf.h"

static volatile sig_atomic_t exiting = 0;
static void on_signal(int sig) { (void)sig; exiting = 1; }

static int libbpf_print_fn(enum libbpf_print_level level, const char *fmt, va_list args)
{
	if (level == LIBBPF_DEBUG && !getenv("XDP_VERBOSE"))
		return 0;
	return vfprintf(stderr, fmt, args);
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "usage: %s <ifname> [duration_seconds]\n", argv[0]);
		return 1;
	}
	const char *ifname = argv[1];
	int duration = argc > 2 ? atoi(argv[2]) : 0;

	int ifindex = if_nametoindex(ifname);
	if (!ifindex) {
		fprintf(stderr, "xdp_probe: no such interface '%s'\n", ifname);
		return 1;
	}
	printf("interface %s -> ifindex %d\n", ifname, ifindex);

	libbpf_set_print(libbpf_print_fn);

	struct bpf_object *obj = bpf_object__open_file("xdp_probe.bpf.o", NULL);
	int err = libbpf_get_error(obj);
	if (err) {
		fprintf(stderr, "xdp_probe: open failed: %s\n", strerror(-err));
		return 1;
	}
	err = bpf_object__load(obj);
	if (err) {
		fprintf(stderr, "xdp_probe: load failed: %s\n", strerror(-err));
		return 1;
	}
	struct bpf_program *prog = bpf_object__find_program_by_name(obj, "xdp_probe");
	int prog_fd = bpf_program__fd(prog);

	// Try native (driver) mode first, fall back to generic (SKB) mode.
	// This is exactly the probing order real XDP loaders use, because
	// not every driver implements native XDP.
	int mode_tried[2] = { XDP_FLAGS_DRV_MODE, XDP_FLAGS_SKB_MODE };
	const char *mode_name[2] = { "native (XDP_FLAGS_DRV_MODE)", "generic (XDP_FLAGS_SKB_MODE)" };
	int attached_mode = -1;

	for (int i = 0; i < 2; i++) {
		err = bpf_xdp_attach(ifindex, prog_fd, mode_tried[i], NULL);
		if (err == 0) {
			attached_mode = i;
			break;
		}
		fprintf(stderr, "xdp_probe: attach in %s failed: %s\n", mode_name[i], strerror(-err));
	}

	if (attached_mode < 0) {
		fprintf(stderr, "xdp_probe: could not attach in any mode on %s\n", ifname);
		bpf_object__close(obj);
		return 1;
	}
	printf("attached in %s mode on %s\n", mode_name[attached_mode], ifname);

	struct bpf_map *map = bpf_object__find_map_by_name(obj, "pkt_count");
	int map_fd = bpf_map__fd(map);

	signal(SIGINT, on_signal);
	signal(SIGTERM, on_signal);

	time_t start = time(NULL);
	__u32 key = 0;
	while (!exiting) {
		sleep(1);
		__u64 count = 0;
		bpf_map_lookup_elem(map_fd, &key, &count);
		printf("\rpackets seen: %-10llu", (unsigned long long)count);
		fflush(stdout);
		if (duration > 0 && (time(NULL) - start) >= duration)
			break;
	}
	printf("\n");

	bpf_xdp_attach(ifindex, -1, mode_tried[attached_mode], NULL); // detach
	bpf_object__close(obj);
	return 0;
}
