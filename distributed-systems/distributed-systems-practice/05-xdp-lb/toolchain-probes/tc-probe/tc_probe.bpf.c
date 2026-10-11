// tc_probe.bpf.c — smallest real TC (SCHED_CLS) program: count every
// packet seen at egress on the attached interface, always TC_ACT_OK.
// Step zero for the load balancer's return-path half (XDP only hooks
// ingress; the return path needs an egress hook, which is what TC's
// clsact qdisc provides).
#include <linux/bpf.h>
#include "bpf_helpers.h"

struct {
	__uint(type, BPF_MAP_TYPE_ARRAY);
	__uint(max_entries, 1);
	__type(key, __u32);
	__type(value, __u64);
} pkt_count SEC(".maps");

SEC("tc")
int tc_probe(struct __sk_buff *skb)
{
	__u32 key = 0;
	__u64 *count = bpf_map_lookup_elem(&pkt_count, &key);
	if (count)
		__sync_fetch_and_add(count, 1);
	return 0; // TC_ACT_OK — don't actually alter traffic in this probe
}

char LICENSE[] SEC("license") = "GPL";
