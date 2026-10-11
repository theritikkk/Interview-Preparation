// xdp_probe_port.bpf.c — like xdp_probe.bpf.c but only counts TCP
// packets whose destination port matches a fixed value, to isolate a
// specific test flow from background container traffic sharing the
// same interface.
#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/in.h> // IPPROTO_TCP
#include "bpf_helpers.h"
#include "bpf_endian.h"

#define TARGET_PORT 8080

struct {
	__uint(type, BPF_MAP_TYPE_ARRAY);
	__uint(max_entries, 1);
	__type(key, __u32);
	__type(value, __u64);
} pkt_count SEC(".maps");

SEC("xdp")
int xdp_probe_port(struct xdp_md *ctx)
{
	void *data = (void *)(long)ctx->data;
	void *data_end = (void *)(long)ctx->data_end;

	struct ethhdr *eth = data;
	if ((void *)(eth + 1) > data_end)
		return XDP_PASS;
	if (eth->h_proto != bpf_htons(ETH_P_IP))
		return XDP_PASS;

	struct iphdr *ip = (void *)(eth + 1);
	if ((void *)(ip + 1) > data_end)
		return XDP_PASS;
	if (ip->protocol != IPPROTO_TCP)
		return XDP_PASS;

	int ip_hlen = ip->ihl * 4;
	struct tcphdr *tcp = (void *)ip + ip_hlen;
	if ((void *)(tcp + 1) > data_end)
		return XDP_PASS;

	if (bpf_ntohs(tcp->dest) != TARGET_PORT)
		return XDP_PASS;

	__u32 key = 0;
	__u64 *count = bpf_map_lookup_elem(&pkt_count, &key);
	if (count)
		__sync_fetch_and_add(count, 1);

	return XDP_PASS;
}

char LICENSE[] SEC("license") = "GPL";
