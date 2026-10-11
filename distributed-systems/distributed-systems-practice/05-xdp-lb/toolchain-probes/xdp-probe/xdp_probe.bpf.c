// xdp_probe.bpf.c — the smallest real XDP program: pass every packet,
// count how many were seen. Step zero before building the actual load
// balancer: find out whether XDP can attach at all here, and on which
// interface/mode, the same way raw_load_test.c did for plain BPF_PROG_LOAD
// in Project 4.
#include <linux/bpf.h>
#include "bpf_helpers.h"

struct {
	__uint(type, BPF_MAP_TYPE_ARRAY);
	__uint(max_entries, 1);
	__type(key, __u32);
	__type(value, __u64);
} pkt_count SEC(".maps");

SEC("xdp")
int xdp_probe(struct xdp_md *ctx)
{
	__u32 key = 0;
	__u64 *count = bpf_map_lookup_elem(&pkt_count, &key);
	if (count)
		__sync_fetch_and_add(count, 1);
	return XDP_PASS;
}

char LICENSE[] SEC("license") = "GPL";
