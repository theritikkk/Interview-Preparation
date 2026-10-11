/* loader.c — userspace control plane for scx_simple.bpf.o.
 *
 * Deliberately NOT built from a bpftool-generated skeleton
 * (scx_simple.bpf.skel.h, as the real upstream scx_simple.c uses) —
 * this sandbox has no bpftool (see ../ANALYSIS.md). Instead this
 * uses libbpf's generic, skeleton-free object API directly:
 * bpf_object__open_file() / bpf_object__load() /
 * bpf_object__find_map_by_name() / bpf_map__attach_struct_ops() —
 * exactly the same non-skeleton pattern Projects 4 and 5 already
 * used successfully for tracepoint and XDP/TC programs in this same
 * sandbox. struct_ops is the one part of this pattern genuinely new
 * to this project: bpf_map__attach_struct_ops() is libbpf's direct
 * equivalent of what the real scx_simple.c gets for free from the
 * SCX_OPS_ATTACH() skeleton macro.
 *
 * What this program actually does, in order:
 *   1. open the compiled object
 *   2. load it into the kernel (bpf_object__load()) — THIS is the
 *      step every finding in ANALYSIS.md is about. On a kernel with
 *      CONFIG_SCHED_CLASS_EXT=y this is where the scheduler would be
 *      verified and accepted; on this sandbox's kernel, this is
 *      where it's expected to be rejected, and this program prints
 *      the real, specific error libbpf/the kernel gives rather than
 *      a hand-written message.
 *   3. only if step 2 succeeds: find the "simple_ops" struct_ops map,
 *      attach it (this is the moment the scheduler would actually
 *      start governing the CPUs), poll+print the "stats" map every
 *      second like the real scx_simple.c, and detach cleanly on
 *      Ctrl-C.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include "libbpf.h"
#include "bpf.h"

#include "user_exit_info_min.h"

static volatile sig_atomic_t exit_req;
static void on_sigint(int sig) { (void)sig; exit_req = 1; }

/* Capture libbpf's own diagnostic stream verbatim instead of
 * swallowing it — this is the channel the real rejection reason
 * (if any) comes through. */
static int print_fn(enum libbpf_print_level level, const char *fmt, va_list args)
{
	(void)level;
	return vfprintf(stderr, fmt, args);
}

int main(int argc, char **argv)
{
	const char *objpath = argc > 1 ? argv[1] : "scx_simple.bpf.o";
	struct bpf_object *obj;
	struct bpf_map *ops_map, *stats_map, *uei_map;
	struct bpf_link *link = NULL;
	int err;

	libbpf_set_print(print_fn);
	signal(SIGINT, on_sigint);
	signal(SIGTERM, on_sigint);

	printf("== scx_simple loader (non-skeleton libbpf API) ==\n");
	printf("[1/3] bpf_object__open_file(%s)\n", objpath);

	obj = bpf_object__open_file(objpath, NULL);
	if (!obj) {
		fprintf(stderr, "FAILED at open: %s\n", strerror(errno));
		return 1;
	}
	printf("      open OK — object name: %s\n", bpf_object__name(obj));

	printf("[2/3] bpf_object__load() — this is the real kernel accept/reject step\n");
	err = bpf_object__load(obj);
	if (err) {
		fprintf(stderr, "\n==> LOAD REJECTED (errno=%d, %s)\n", -err, strerror(-err));
		fprintf(stderr, "==> This is the real, directly-observed kernel/libbpf\n");
		fprintf(stderr, "==> response captured for ANALYSIS.md — see the libbpf\n");
		fprintf(stderr, "==> diagnostic lines printed above (from print_fn) for\n");
		fprintf(stderr, "==> the specific reason.\n");
		bpf_object__close(obj);
		return 1;
	}
	printf("      load OK (unexpected on this sandbox's kernel — see ANALYSIS.md)\n");

	ops_map = bpf_object__find_map_by_name(obj, "simple_ops");
	if (!ops_map) {
		fprintf(stderr, "could not find struct_ops map 'simple_ops'\n");
		goto cleanup;
	}

	printf("[3/3] bpf_map__attach_struct_ops() — registering as the active scheduler\n");
	link = bpf_map__attach_struct_ops(ops_map);
	if (!link) {
		fprintf(stderr, "FAILED to attach struct_ops: %s\n", strerror(errno));
		goto cleanup;
	}
	printf("      ATTACHED. scx_simple is now the system scheduler. Ctrl-C to stop.\n");

	stats_map = bpf_object__find_map_by_name(obj, "stats");
	uei_map   = bpf_object__find_map_by_name(obj, ".data");

	while (!exit_req) {
		if (stats_map) {
			int nr_cpus = libbpf_num_possible_cpus();
			__u64 cnts[2] = {0, 0};
			for (__u32 idx = 0; idx < 2; idx++) {
				__u64 percpu[nr_cpus];
				if (bpf_map__lookup_elem(stats_map, &idx, sizeof(idx),
							  percpu, sizeof(percpu[0]) * nr_cpus, 0) == 0) {
					for (int c = 0; c < nr_cpus; c++)
						cnts[idx] += percpu[c];
				}
			}
			printf("local=%llu global=%llu\n",
			       (unsigned long long)cnts[0], (unsigned long long)cnts[1]);
		}
		if (uei_map) {
			struct user_exit_info uei;
			__u32 zero = 0;
			if (bpf_map__lookup_elem(uei_map, &zero, sizeof(zero),
						  &uei, sizeof(uei), 0) == 0 && uei.kind != 0) {
				UEI_REPORT(&uei);
				break;
			}
		}
		sleep(1);
	}

	bpf_link__destroy(link);
cleanup:
	bpf_object__close(obj);
	return 0;
}
