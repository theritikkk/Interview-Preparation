// xdp_lb.bpf.c — the forward-path half of a toy L4 (TCP port-based) load
// balancer. Attached at XDP ingress on `lo`. Every TCP packet addressed
// to the virtual service port (VIP_PORT) gets its destination port
// rewritten, in place, to one of a configured set of real backend
// ports — chosen by hashing the client's (source IP, source port), so
// every packet of the same connection lands on the same backend — and
// the TCP checksum is fixed up to match. XDP_PASS afterwards lets the
// kernel's normal local-delivery path do the rest: since the destination
// IP never changes (this is loopback-only; see ANALYSIS.md for why),
// IP routing doesn't care that the port changed, and the packet is
// delivered to whichever real backend socket now matches.
//
// This program does NOT fix up the return path (backend -> client) —
// that's a separate packet flowing the other direction, which XDP
// (ingress-only) cannot see. See tc_lb_return.bpf.c for that half.
//
// WHY `lo`, NOT eth0: traffic from this same host to its own eth0
// address never actually reaches eth0's XDP hook — the kernel's local
// routing table delivers it straight to the loopback path internally,
// confirmed empirically (0 packets seen on an eth0 XDP probe filtered to
// the test port, despite real HTTP 200s on real curl requests to that
// address). `lo` is the real wire this traffic travels on a single host,
// so that's where the LB has to sit. See ANALYSIS.md.
#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/in.h>
#include "bpf_helpers.h"
#include "bpf_endian.h"

#define MAX_BACKENDS 8

struct lb_config {
	__u16 vip_port;      // network byte order
	__u16 num_backends;  // host byte order, 1..MAX_BACKENDS
};

struct {
	__uint(type, BPF_MAP_TYPE_ARRAY);
	__uint(max_entries, 1);
	__type(key, __u32);
	__type(value, struct lb_config);
} config SEC(".maps");

struct {
	__uint(type, BPF_MAP_TYPE_ARRAY);
	__uint(max_entries, MAX_BACKENDS);
	__type(key, __u32);
	__type(value, __u16); // backend port, network byte order
} backend_ports SEC(".maps");

// per-backend packet counters — not needed for the LB to function, but
// is how the demo proves traffic is actually being spread across
// backends rather than always landing on the first one.
struct {
	__uint(type, BPF_MAP_TYPE_ARRAY);
	__uint(max_entries, MAX_BACKENDS);
	__type(key, __u32);
	__type(value, __u64);
} backend_hits SEC(".maps");

// RFC 1624-style incremental checksum update for a single changed
// 16-bit field: TCP's checksum covers the pseudo-header + header +
// payload, but when only one 16-bit word changes (here, just the
// destination port), the new checksum can be derived from the old one
// without touching the payload at all — much cheaper than a full
// recompute, and it's the same technique the kernel's own NAT code uses.
// XDP programs get raw packet bytes, not a `struct __sk_buff`, so the
// `bpf_l4_csum_replace` helper (which takes an skb) isn't available
// here — this is the manual equivalent.
static __always_inline void update_csum(__u16 *csum, __u16 old_val, __u16 new_val)
{
	__u32 c = (~bpf_ntohs(*csum)) & 0xffff;
	c += (~bpf_ntohs(old_val)) & 0xffff;
	c += bpf_ntohs(new_val);
	c = (c & 0xffff) + (c >> 16);
	c = (c & 0xffff) + (c >> 16);
	*csum = bpf_htons((__u16)(~c & 0xffff));
}

// Simple multiplicative hash (Knuth's constant) over (src_ip, src_port)
// — deterministic per-flow (same client IP:port always hashes the same
// way, so a connection's packets all pick the same backend), with no
// claim to cryptographic quality, which a toy LB doesn't need.
static __always_inline __u32 flow_hash(__u32 src_ip, __u16 src_port)
{
	__u32 h = src_ip ^ ((__u32)src_port << 16 | src_port);
	h *= 2654435761u;
	return h;
}

SEC("xdp")
int xdp_lb(struct xdp_md *ctx)
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

	// only handles the common case (no IP options) — real traffic in
	// this demo never sends any, and the fallback is just "don't touch
	// the packet," never a crash.
	if (ip->ihl != 5)
		return XDP_PASS;

	struct tcphdr *tcp = (void *)(ip + 1);
	if ((void *)(tcp + 1) > data_end)
		return XDP_PASS;

	__u32 cfg_key = 0;
	struct lb_config *cfg = bpf_map_lookup_elem(&config, &cfg_key);
	if (!cfg || cfg->num_backends == 0)
		return XDP_PASS;

	if (tcp->dest != cfg->vip_port)
		return XDP_PASS; // not traffic for the virtual service

	__u32 idx = flow_hash(ip->saddr, tcp->source) % cfg->num_backends;
	__u16 *backend_port = bpf_map_lookup_elem(&backend_ports, &idx);
	if (!backend_port || *backend_port == 0)
		return XDP_PASS;

	__u16 old_dport = tcp->dest;
	tcp->dest = *backend_port;
	update_csum(&tcp->check, old_dport, tcp->dest);
	// IP header checksum is untouched deliberately: it covers only the
	// IP header fields (addresses, ttl, length, ...), none of which
	// changed — the destination port lives in the TCP header, outside
	// what the IP checksum protects.

	__u64 *hits = bpf_map_lookup_elem(&backend_hits, &idx);
	if (hits)
		__sync_fetch_and_add(hits, 1);

	return XDP_PASS;
}

char LICENSE[] SEC("license") = "GPL";
