// tc_lb_return.bpf.c — the return-path half of the toy load balancer.
// Attached at TC egress on `lo`. A reply from a real backend (source
// port = one of the configured backend ports) has its source port
// rewritten back to the virtual service's port before it leaves the
// host, so the client's TCP stack — which is holding a connection
// object keyed on (peer_ip, VIP_port) — recognizes the reply at all.
//
// WHY THIS HAS TO EXIST SEPARATELY FROM xdp_lb.bpf.c: XDP only hooks
// ingress; there is no "XDP egress." A real L4 load balancer rewrites
// traffic in both directions — xdp_lb.bpf.c handles client -> backend,
// this handles backend -> client — the same two-sided DNAT that
// iptables/netfilter's conntrack does automatically and transparently;
// here both halves are explicit, hand-written BPF, which is also most of
// why this project has two programs instead of one.
//
// No connection-tracking state is needed to do this correctly: EVERY
// packet whose source port is a known backend port gets rewritten,
// regardless of which client it's headed to, because in this toy setup
// nothing *else* ever sends traffic from those ports — see ANALYSIS.md
// for exactly why that's a safe simplification here and wouldn't be in
// general.
#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/in.h>
#include <linux/pkt_cls.h> // TC_ACT_OK
#include "bpf_helpers.h"
#include "bpf_endian.h"

#define MAX_BACKENDS 8

struct lb_config {
	__u16 vip_port;
	__u16 num_backends;
};

struct {
	__uint(type, BPF_MAP_TYPE_ARRAY);
	__uint(max_entries, 1);
	__type(key, __u32);
	__type(value, struct lb_config);
} config SEC(".maps");

// keyed by backend port (network byte order) -> 1, just an existence
// check; populated with the same ports as xdp_lb's backend_ports array.
struct {
	__uint(type, BPF_MAP_TYPE_HASH);
	__uint(max_entries, MAX_BACKENDS);
	__type(key, __u16);
	__type(value, __u8);
} is_backend_port SEC(".maps");

static __always_inline void update_csum(__u16 *csum, __u16 old_val, __u16 new_val)
{
	__u32 c = (~bpf_ntohs(*csum)) & 0xffff;
	c += (~bpf_ntohs(old_val)) & 0xffff;
	c += bpf_ntohs(new_val);
	c = (c & 0xffff) + (c >> 16);
	c = (c & 0xffff) + (c >> 16);
	*csum = bpf_htons((__u16)(~c & 0xffff));
}

SEC("tc")
int tc_lb_return(struct __sk_buff *skb)
{
	void *data = (void *)(long)skb->data;
	void *data_end = (void *)(long)skb->data_end;

	struct ethhdr *eth = data;
	if ((void *)(eth + 1) > data_end)
		return TC_ACT_OK;
	if (eth->h_proto != bpf_htons(ETH_P_IP))
		return TC_ACT_OK;

	struct iphdr *ip = (void *)(eth + 1);
	if ((void *)(ip + 1) > data_end)
		return TC_ACT_OK;
	if (ip->protocol != IPPROTO_TCP || ip->ihl != 5)
		return TC_ACT_OK;

	struct tcphdr *tcp = (void *)(ip + 1);
	if ((void *)(tcp + 1) > data_end)
		return TC_ACT_OK;

	__u8 *is_backend = bpf_map_lookup_elem(&is_backend_port, &tcp->source);
	if (!is_backend)
		return TC_ACT_OK; // not traffic from one of our backends

	__u32 cfg_key = 0;
	struct lb_config *cfg = bpf_map_lookup_elem(&config, &cfg_key);
	if (!cfg)
		return TC_ACT_OK;

	__u16 old_sport = tcp->source;
	tcp->source = cfg->vip_port;
	update_csum(&tcp->check, old_sport, tcp->source);

	return TC_ACT_OK;
}

char LICENSE[] SEC("license") = "GPL";
