// btf_search.c — substring-search the live kernel's BTF type names, for
// when btf_probe's exact-name lookup comes back NOT FOUND and the real
// question becomes "what IS this type actually called on this kernel?"
//
// This is what found the real names behind biolatency's CO-RE relocation
// failure: `trace_event_raw_block_rq_issue` doesn't exist, but searching
// for "block_rq" turned up `trace_event_raw_block_rq` (the real name —
// the kernel's TRACE_EVENT macros generate one struct per *event class*,
// shared by several individual tracepoints, not one struct per
// tracepoint; see ANALYSIS.md). Also searches for `trace_event_raw`
// generally so the pattern is visible for any future tracepoint, not
// just block_rq.
//
// Usage: btf_search <substring> [<substring> ...]
#include <stdio.h>
#include <string.h>
#include "btf.h"
#include "libbpf_legacy.h" // libbpf_get_error()

#define MAX_PRINT_PER_NEEDLE 40

int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "usage: %s <substring> [<substring> ...]\n", argv[0]);
		return 1;
	}

	struct btf *btf = btf__parse("/sys/kernel/btf/vmlinux", NULL);
	long err = libbpf_get_error(btf);
	if (err) {
		printf("btf__parse failed: %ld\n", err);
		return 1;
	}
	__u32 n = btf__type_cnt(btf);
	printf("vmlinux BTF loaded, %u types\n", n);

	for (int a = 1; a < argc; a++) {
		const char *needle = argv[a];
		int matches = 0;
		for (__u32 id = 1; id < n; id++) {
			const struct btf_type *t = btf__type_by_id(btf, id);
			if (!t)
				continue;
			int kind = btf_kind(t);
			if (kind != BTF_KIND_STRUCT && kind != BTF_KIND_UNION &&
			    kind != BTF_KIND_TYPEDEF)
				continue;
			const char *name = btf__name_by_offset(btf, t->name_off);
			if (!name || !name[0] || !strstr(name, needle))
				continue;
			if (matches < MAX_PRINT_PER_NEEDLE)
				printf("[%s] kind=%d id=%u name=%s\n", needle, kind, id, name);
			matches++;
		}
		printf("'%s': %d matches%s\n", needle, matches,
		       matches > MAX_PRINT_PER_NEEDLE ? " (truncated)" : "");
	}

	btf__free(btf);
	return 0;
}
