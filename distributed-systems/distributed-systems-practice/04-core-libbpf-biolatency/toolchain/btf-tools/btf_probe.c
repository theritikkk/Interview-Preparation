// btf_probe.c — query the live kernel's BTF for specific struct names,
// using libbpf's own BTF-parsing API (btf__parse / btf__find_by_name_kind
// / btf__type_by_id / btf_members) rather than guessing or trusting a
// hand-written vmlinux_min.h.
//
// WHY THIS EXISTS: a CO-RE program's `preserve_access_index` struct names
// are a blind guess until something actually checks them against
// /sys/kernel/btf/vmlinux. The first guess for biolatency's tracepoint
// structs — `trace_event_raw_block_rq_issue` / `_block_rq_complete`,
// matching the tracepoint's own name — turned out to be wrong on this
// kernel (see ANALYSIS.md). This tool is what found that out: it asks
// the kernel's real BTF directly, "does a struct by this name exist, and
// if so what fields does it have at what offsets?" — the same question
// libbpf itself answers internally during CO-RE relocation, just made
// visible and run ahead of time instead of discovered via a verifier
// rejection.
//
// Usage: btf_probe <struct_name> [<struct_name> ...]
#include <stdio.h>
#include <string.h>
#include "btf.h"
#include "libbpf_legacy.h" // libbpf_get_error()

static void dump_struct(struct btf *btf, const char *name)
{
	__s32 id = btf__find_by_name_kind(btf, name, BTF_KIND_STRUCT);
	if (id < 0) {
		printf("struct %s: NOT FOUND (id=%d)\n", name, id);
		return;
	}
	const struct btf_type *t = btf__type_by_id(btf, id);
	printf("struct %s: FOUND, id=%d, vlen=%d, size=%u\n", name, id,
	       btf_vlen(t), t->size);
	struct btf_member *m = btf_members(t);
	for (int i = 0; i < btf_vlen(t); i++, m++) {
		const char *mname = btf__name_by_offset(btf, m->name_off);
		printf("  [%d] %-20s bit_offset=%u (byte=%u) type_id=%u\n", i,
		       mname ? mname : "<anon>", m->offset, m->offset / 8,
		       m->type);
	}
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "usage: %s <struct_name> [<struct_name> ...]\n", argv[0]);
		return 1;
	}

	struct btf *btf = btf__parse("/sys/kernel/btf/vmlinux", NULL);
	long err = libbpf_get_error(btf);
	if (err) {
		printf("btf__parse failed: %ld\n", err);
		return 1;
	}
	printf("vmlinux BTF loaded, %u types\n", btf__type_cnt(btf));

	for (int i = 1; i < argc; i++)
		dump_struct(btf, argv[i]);

	btf__free(btf);
	return 0;
}
