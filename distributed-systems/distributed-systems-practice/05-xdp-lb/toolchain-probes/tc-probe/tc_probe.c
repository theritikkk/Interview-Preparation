// tc_probe.c — loads tc_probe.bpf.o, creates a clsact qdisc on the given
// interface (bpf_tc_hook_create), and attaches it at egress
// (bpf_tc_attach), entirely through libbpf's own TC API — no `tc` CLI
// needed (and none is installed here anyway). Confirms this sandbox can
// really do TC-BPF, not just XDP.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <net/if.h>

#include "libbpf.h"
#include "bpf.h"

static volatile sig_atomic_t exiting = 0;
static void on_signal(int sig) { (void)sig; exiting = 1; }

static int libbpf_print_fn(enum libbpf_print_level level, const char *fmt, va_list args)
{
	if (level == LIBBPF_DEBUG && !getenv("TC_VERBOSE"))
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
		fprintf(stderr, "tc_probe: no such interface '%s'\n", ifname);
		return 1;
	}

	libbpf_set_print(libbpf_print_fn);

	struct bpf_object *obj = bpf_object__open_file("tc_probe.bpf.o", NULL);
	int err = libbpf_get_error(obj);
	if (err) {
		fprintf(stderr, "tc_probe: open failed: %s\n", strerror(-err));
		return 1;
	}
	err = bpf_object__load(obj);
	if (err) {
		fprintf(stderr, "tc_probe: load failed: %s\n", strerror(-err));
		return 1;
	}
	struct bpf_program *prog = bpf_object__find_program_by_name(obj, "tc_probe");
	int prog_fd = bpf_program__fd(prog);

	DECLARE_LIBBPF_OPTS(bpf_tc_hook, hook, .ifindex = ifindex, .attach_point = BPF_TC_EGRESS);
	err = bpf_tc_hook_create(&hook);
	// EEXIST is fine — a clsact qdisc from a previous run may still be there.
	if (err && err != -EEXIST) {
		fprintf(stderr, "tc_probe: bpf_tc_hook_create failed: %s\n", strerror(-err));
		bpf_object__close(obj);
		return 1;
	}

	DECLARE_LIBBPF_OPTS(bpf_tc_opts, opts, .prog_fd = prog_fd);
	err = bpf_tc_attach(&hook, &opts);
	if (err) {
		fprintf(stderr, "tc_probe: bpf_tc_attach failed: %s\n", strerror(-err));
		bpf_tc_hook_destroy(&hook);
		bpf_object__close(obj);
		return 1;
	}
	printf("attached tc_probe at egress on %s (handle=%u priority=%u)\n",
	       ifname, opts.handle, opts.priority);

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
		printf("\regress packets seen: %-10llu", (unsigned long long)count);
		fflush(stdout);
		if (duration > 0 && (time(NULL) - start) >= duration)
			break;
	}
	printf("\n");

	bpf_tc_detach(&hook, &opts);
	bpf_tc_hook_destroy(&hook);
	bpf_object__close(obj);
	return 0;
}
