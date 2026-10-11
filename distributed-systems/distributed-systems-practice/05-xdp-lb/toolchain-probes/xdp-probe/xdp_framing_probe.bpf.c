// xdp_framing_probe.bpf.c — determines empirically whether packets on
// the attached interface start with an Ethernet header or a raw IP
// header at offset 0, by trying both interpretations and counting which
// one looks like a sane IPv4 header (version==4, ihl in [5..15]).
#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include "bpf_helpers.h"
#include "bpf_endian.h"

struct {
	__uint(type, BPF_MAP_TYPE_ARRAY);
	__uint(max_entries, 2);
	__type(key, __u32);
	__type(value, __u64);
} counts SEC(".maps"); // [0] = looked like raw IP at offset 0
                        // [1] = looked like IP after a 14-byte eth header

static __always_inline void bump(__u32 idx)
{
	__u64 *c = bpf_map_lookup_elem(&counts, &idx);
	if (c)
		__sync_fetch_and_add(c, 1);
}

SEC("xdp")
int xdp_framing_probe(struct xdp_md *ctx)
{
	void *data = (void *)(long)ctx->data;
	void *data_end = (void *)(long)ctx->data_end;

	// hypothesis A: raw IP header at offset 0 (no L2 header)
	struct iphdr *ip_raw = data;
	if ((void *)(ip_raw + 1) <= data_end &&
	    ip_raw->version == 4 && ip_raw->ihl >= 5 && ip_raw->ihl <= 15)
		bump(0);

	// hypothesis B: 14-byte Ethernet header, then IP
	struct ethhdr *eth = data;
	if ((void *)(eth + 1) <= data_end) {
		struct iphdr *ip_eth = (void *)(eth + 1);
		if ((void *)(ip_eth + 1) <= data_end &&
		    eth->h_proto == bpf_htons(ETH_P_IP) &&
		    ip_eth->version == 4 && ip_eth->ihl >= 5 && ip_eth->ihl <= 15)
			bump(1);
	}

	return XDP_PASS;
}

char LICENSE[] SEC("license") = "GPL";
