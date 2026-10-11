// lb_ctl.c — the load balancer's control plane: loads both BPF programs
// (xdp_lb.bpf.o at XDP ingress, tc_lb_return.bpf.o at TC egress, both on
// the same interface), writes the virtual-service port and backend list
// into the maps they share, and prints live per-backend hit counts until
// Ctrl-C — which is also when it cleans up both attachments.
//
// usage: lb_ctl <ifname> <vip_port> <backend_port> [<backend_port> ...]
// example: lb_ctl lo 8080 9001 9002 9003
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <net/if.h>
#include <linux/if_link.h>
#include <arpa/inet.h> // htons

#include "libbpf.h"
#include "bpf.h"

#define MAX_BACKENDS 8

struct lb_config {
	__u16 vip_port;
	__u16 num_backends;
};

static volatile sig_atomic_t exiting = 0;
static void on_signal(int sig) { (void)sig; exiting = 1; }

static int libbpf_print_fn(enum libbpf_print_level level, const char *fmt, va_list args)
{
	if (level == LIBBPF_DEBUG && !getenv("LB_VERBOSE"))
		return 0;
	return vfprintf(stderr, fmt, args);
}

int main(int argc, char **argv)
{
	if (argc < 4) {
		fprintf(stderr, "usage: %s <ifname> <vip_port> <backend_port> [<backend_port> ...]\n", argv[0]);
		fprintf(stderr, "example: %s lo 8080 9001 9002 9003\n", argv[0]);
		return 1;
	}
	const char *ifname = argv[1];
	int vip_port = atoi(argv[2]);
	int num_backends = argc - 3;
	if (num_backends > MAX_BACKENDS) {
		fprintf(stderr, "lb_ctl: too many backends (max %d)\n", MAX_BACKENDS);
		return 1;
	}
	int backend_ports_host[MAX_BACKENDS];
	for (int i = 0; i < num_backends; i++)
		backend_ports_host[i] = atoi(argv[3 + i]);

	int ifindex = if_nametoindex(ifname);
	if (!ifindex) {
		fprintf(stderr, "lb_ctl: no such interface '%s'\n", ifname);
		return 1;
	}

	if (geteuid() != 0) {
		fprintf(stderr, "lb_ctl: must run as root\n");
		return 1;
	}

	libbpf_set_print(libbpf_print_fn);

	// ---- load + attach the forward path (XDP ingress) ----
	struct bpf_object *xdp_obj = bpf_object__open_file("xdp_lb.bpf.o", NULL);
	if (libbpf_get_error(xdp_obj)) {
		fprintf(stderr, "lb_ctl: failed to open xdp_lb.bpf.o\n");
		return 1;
	}
	if (bpf_object__load(xdp_obj)) {
		fprintf(stderr, "lb_ctl: failed to load xdp_lb.bpf.o\n");
		return 1;
	}
	struct bpf_program *xdp_prog = bpf_object__find_program_by_name(xdp_obj, "xdp_lb");
	int xdp_fd = bpf_program__fd(xdp_prog);

	int xdp_modes[2] = { XDP_FLAGS_DRV_MODE, XDP_FLAGS_SKB_MODE };
	const char *xdp_mode_name[2] = { "native", "generic (SKB)" };
	int xdp_mode = -1;
	for (int i = 0; i < 2; i++) {
		if (bpf_xdp_attach(ifindex, xdp_fd, xdp_modes[i], NULL) == 0) {
			xdp_mode = i;
			break;
		}
	}
	if (xdp_mode < 0) {
		fprintf(stderr, "lb_ctl: could not attach XDP program on %s in any mode\n", ifname);
		return 1;
	}
	printf("xdp_lb attached on %s (%s mode)\n", ifname, xdp_mode_name[xdp_mode]);

	// ---- load + attach the return path (TC egress) ----
	struct bpf_object *tc_obj = bpf_object__open_file("tc_lb_return.bpf.o", NULL);
	if (libbpf_get_error(tc_obj)) {
		fprintf(stderr, "lb_ctl: failed to open tc_lb_return.bpf.o\n");
		goto cleanup_xdp;
	}
	if (bpf_object__load(tc_obj)) {
		fprintf(stderr, "lb_ctl: failed to load tc_lb_return.bpf.o\n");
		goto cleanup_xdp;
	}
	struct bpf_program *tc_prog = bpf_object__find_program_by_name(tc_obj, "tc_lb_return");
	int tc_fd = bpf_program__fd(tc_prog);

	DECLARE_LIBBPF_OPTS(bpf_tc_hook, tc_hook, .ifindex = ifindex, .attach_point = BPF_TC_EGRESS);
	int err = bpf_tc_hook_create(&tc_hook);
	if (err && err != -EEXIST) {
		fprintf(stderr, "lb_ctl: bpf_tc_hook_create failed: %s\n", strerror(-err));
		goto cleanup_xdp;
	}
	DECLARE_LIBBPF_OPTS(bpf_tc_opts, tc_opts, .prog_fd = tc_fd);
	err = bpf_tc_attach(&tc_hook, &tc_opts);
	if (err) {
		fprintf(stderr, "lb_ctl: bpf_tc_attach failed: %s\n", strerror(-err));
		bpf_tc_hook_destroy(&tc_hook);
		goto cleanup_xdp;
	}
	printf("tc_lb_return attached on %s (egress)\n", ifname);

	// ---- configure both programs' maps ----
	struct lb_config cfg = {
		.vip_port = htons(vip_port),
		.num_backends = num_backends,
	};
	__u32 cfg_key = 0;

	int xdp_config_fd = bpf_object__find_map_fd_by_name(xdp_obj, "config");
	int xdp_backend_ports_fd = bpf_object__find_map_fd_by_name(xdp_obj, "backend_ports");
	int xdp_backend_hits_fd = bpf_object__find_map_fd_by_name(xdp_obj, "backend_hits");
	int tc_config_fd = bpf_object__find_map_fd_by_name(tc_obj, "config");
	int tc_is_backend_fd = bpf_object__find_map_fd_by_name(tc_obj, "is_backend_port");

	bpf_map_update_elem(xdp_config_fd, &cfg_key, &cfg, BPF_ANY);
	bpf_map_update_elem(tc_config_fd, &cfg_key, &cfg, BPF_ANY);

	for (int i = 0; i < num_backends; i++) {
		__u32 idx = i;
		__u16 port_net = htons(backend_ports_host[i]);
		bpf_map_update_elem(xdp_backend_ports_fd, &idx, &port_net, BPF_ANY);
		__u8 one = 1;
		bpf_map_update_elem(tc_is_backend_fd, &port_net, &one, BPF_ANY);
	}

	printf("virtual service: 127.0.0.1:%d -> backends: ", vip_port);
	for (int i = 0; i < num_backends; i++)
		printf("%d%s", backend_ports_host[i], i + 1 < num_backends ? ", " : "\n");

	signal(SIGINT, on_signal);
	signal(SIGTERM, on_signal);

	printf("\nCtrl-C to stop.\n\n");
	while (!exiting) {
		sleep(1);
		printf("\r");
		__u64 total = 0;
		for (int i = 0; i < num_backends; i++) {
			__u32 idx = i;
			__u64 hits = 0;
			bpf_map_lookup_elem(xdp_backend_hits_fd, &idx, &hits);
			total += hits;
			printf("[:%d]=%llu ", backend_ports_host[i], (unsigned long long)hits);
		}
		printf(" total=%llu   ", (unsigned long long)total);
		fflush(stdout);
	}
	printf("\n");

	bpf_tc_detach(&tc_hook, &tc_opts);
	bpf_tc_hook_destroy(&tc_hook);
	bpf_object__close(tc_obj);
cleanup_xdp:
	bpf_xdp_attach(ifindex, -1, xdp_modes[xdp_mode], NULL);
	bpf_object__close(xdp_obj);
	return 0;
}
