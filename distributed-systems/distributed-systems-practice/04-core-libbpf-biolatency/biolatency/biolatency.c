// biolatency.c — userspace loader for biolatency.bpf.o, using libbpf's
// generic object-loading API directly (bpf_object__open_file /
// bpf_object__load / bpf_program__attach) rather than a bpftool-
// generated skeleton. Functionally identical to what a skeleton's
// generated __load()/__attach() calls do under the hood — this is the
// same libbpf, the same CO-RE relocation, just without the extra
// codegen step bpftool would otherwise provide. See ANALYSIS.md for
// why bpftool itself wasn't built for this project.
//
// Output format matches BCC's own biolatency: a log2 histogram of
// block I/O latency in microseconds, printed once on exit (Ctrl-C) or
// after an optional duration.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

// Plain names, not <bpf/libbpf.h>: this links straight against the raw
// libbpf/src/ source tree (see makefile's LIBBPF_SRC), not a
// `make install`-ed copy under a bpf/ prefix — this project never runs
// install, to avoid touching anything outside its own directory.
#include "libbpf.h"
#include "bpf.h"

#define MAX_SLOTS 32

static volatile sig_atomic_t exiting = 0;

static void on_signal(int sig)
{
	(void)sig;
	exiting = 1;
}

static int libbpf_print_fn(enum libbpf_print_level level, const char *fmt, va_list args)
{
	// Quiet by default; rerun with BIOLATENCY_VERBOSE=1 to see libbpf's
	// own diagnostic/debug output (useful if attach or load fails).
	if (level == LIBBPF_DEBUG && !getenv("BIOLATENCY_VERBOSE"))
		return 0;
	return vfprintf(stderr, fmt, args);
}

static void print_log2_hist(__u64 *slots, int n)
{
	int i, max_slot = -1;
	__u64 max_count = 0;

	for (i = 0; i < n; i++) {
		if (slots[i] > 0)
			max_slot = i;
		if (slots[i] > max_count)
			max_count = slots[i];
	}

	if (max_slot < 0) {
		printf("(no I/O completions observed)\n");
		return;
	}

	printf("%-19s : count    distribution\n", "usecs");
	for (i = 0; i <= max_slot; i++) {
		unsigned long long low = (i == 0) ? 0 : (1ULL << (i - 1)) + 1;
		unsigned long long high = (1ULL << i);
		int stars, j;
		char bar[41];

		stars = max_count ? (int)(slots[i] * 40 / max_count) : 0;
		for (j = 0; j < 40; j++)
			bar[j] = (j < stars) ? '*' : ' ';
		bar[40] = '\0';

		if (i == 0)
			printf("%10llu -> %-10llu : %-8llu |%s|\n", 0ULL, 1ULL,
			       (unsigned long long)slots[i], bar);
		else
			printf("%10llu -> %-10llu : %-8llu |%s|\n", low, high,
			       (unsigned long long)slots[i], bar);
	}
}

int main(int argc, char **argv)
{
	const char *obj_path = "biolatency.bpf.o";
	int duration_sec = 0; // 0 = run until Ctrl-C

	if (argc > 1) {
		duration_sec = atoi(argv[1]);
		if (duration_sec <= 0) {
			fprintf(stderr, "usage: %s [duration_seconds]\n", argv[0]);
			fprintf(stderr, "  no argument: trace until Ctrl-C\n");
			return 1;
		}
	}

	if (geteuid() != 0) {
		fprintf(stderr, "biolatency: must run as root (needs CAP_BPF/CAP_SYS_ADMIN to load a BPF program)\n");
		return 1;
	}

	libbpf_set_print(libbpf_print_fn);

	struct bpf_object *obj = bpf_object__open_file(obj_path, NULL);
	int err = libbpf_get_error(obj);
	if (err) {
		fprintf(stderr, "biolatency: failed to open %s: %s\n", obj_path, strerror(-err));
		return 1;
	}

	err = bpf_object__load(obj);
	if (err) {
		fprintf(stderr, "biolatency: failed to load BPF object (verifier rejected it, or "
		                 "CO-RE relocation failed against this kernel's BTF): %s\n",
		        strerror(-err));
		fprintf(stderr, "  (rerun with BIOLATENCY_VERBOSE=1 for the verifier log)\n");
		bpf_object__close(obj);
		return 1;
	}

	// Attach every program this object defines (the two tracepoint
	// handlers) — bpf_program__attach() reads each program's SEC()
	// name to figure out what kind of attachment it needs; we don't
	// have to say "tracepoint" again here.
	int attached = 0;
	struct bpf_program *prog;
	struct bpf_link *links[8];
	bpf_object__for_each_program(prog, obj) {
		struct bpf_link *link = bpf_program__attach(prog);
		err = libbpf_get_error(link);
		if (err) {
			fprintf(stderr, "biolatency: failed to attach %s: %s\n",
			        bpf_program__name(prog), strerror(-err));
			bpf_object__close(obj);
			return 1;
		}
		links[attached++] = link;
	}

	struct bpf_map *hist_map = bpf_object__find_map_by_name(obj, "hist");
	if (!hist_map) {
		fprintf(stderr, "biolatency: couldn't find the 'hist' map in the loaded object\n");
		bpf_object__close(obj);
		return 1;
	}
	int hist_fd = bpf_map__fd(hist_map);

	signal(SIGINT, on_signal);
	signal(SIGTERM, on_signal);

	printf("Tracing block I/O latency (block_rq_issue -> block_rq_complete)... ");
	if (duration_sec > 0)
		printf("for %ds.\n", duration_sec);
	else
		printf("Ctrl-C to stop.\n");

	time_t start = time(NULL);
	while (!exiting) {
		sleep(1);
		if (duration_sec > 0 && (time(NULL) - start) >= duration_sec)
			break;
	}

	__u64 slots[MAX_SLOTS] = {0};
	for (__u32 i = 0; i < MAX_SLOTS; i++) {
		__u64 v = 0;
		if (bpf_map_lookup_elem(hist_fd, &i, &v) == 0)
			slots[i] = v;
	}

	printf("\n");
	print_log2_hist(slots, MAX_SLOTS);

	for (int i = 0; i < attached; i++)
		bpf_link__destroy(links[i]);
	bpf_object__close(obj);
	return 0;
}
