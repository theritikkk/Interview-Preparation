// biolatency.bpf.c — a CO-RE rewrite of BCC's biolatency: a log2
// histogram of block I/O request latency, measured in the live kernel.
//
// WHY TRACEPOINTS, NOT KPROBES: the canonical modern biolatency (in
// bcc/libbpf-tools) hooks blk_account_io_start/blk_account_io_done via
// kprobes, keyed by `struct request *`. This kernel has
// `CONFIG_KPROBES` compiled out entirely (confirmed: no kprobe PMU
// under /sys/bus/event_source/devices/, and /sys/kernel/tracing/
// kprobe_events doesn't exist) — not a permissions issue, a build-time
// kernel option. So this uses the block:block_rq_issue and
// block:block_rq_complete tracepoints instead, which this kernel does
// support (confirmed live: ids 1371 and 1374 under
// /sys/kernel/tracing/events/block/). This is how BCC's *original*
// biolatency worked before the project moved to kprobes for better
// accuracy — see ANALYSIS.md for the real tradeoff that comes with it
// (merged/split requests aren't tracked as precisely as a
// `struct request *`-keyed version would).
//
// WHY THESE STRUCTS ARE HAND-DECLARED, NOT FROM A GENERATED vmlinux.h:
// generating vmlinux.h is normally `bpftool btf dump file
// /sys/kernel/btf/vmlinux format c`, and bpftool needs libelf to build
// — unavailable in this sandbox (apt is blocked; see ANALYSIS.md). A
// full vmlinux.h would also be ~30k lines for a program that touches 7
// fields total. Instead, the two tracepoint argument structs are
// hand-declared in vmlinux_min.h directly from this live kernel's own
// /sys/kernel/tracing/events/block/{block_rq_issue,block_rq_complete}/format
// output (field names, types, and order copied verbatim), marked
// `__attribute__((preserve_access_index))` so libbpf's CO-RE relocation
// still resolves each field's real offset against the target kernel's
// BTF at load time rather than trusting the offsets written here. This
// is a standard, intentional CO-RE pattern for programs that don't want
// the full vmlinux.h — not a shortcut unique to this sandbox.
//
// STRUCT NAME GOTCHA (see vmlinux_min.h for the full story): the names
// below are NOT `trace_event_raw_block_rq_issue` /
// `_block_rq_complete` as a first guess would have it — this kernel's
// TRACE_EVENT macros generate one struct per event *class*, shared by
// several tracepoints, so the real BTF names are
// `trace_event_raw_block_rq` and `trace_event_raw_block_rq_completion`.
// Confirmed directly against /sys/kernel/btf/vmlinux, not assumed.

#include "vmlinux_min.h"
#include "bpf_helpers.h"
#include "bpf_core_read.h"
#include "bpf_tracing.h"

char LICENSE[] SEC("license") = "GPL";

// ---- histogram shape -------------------------------------------------
// 32 log2 buckets of microseconds covers ~0us up to ~2.1s of latency,
// the same range BCC's own biolatency uses.
#define MAX_SLOTS 32

// Correlates an in-flight request's "issue" tracepoint with its later
// "complete" tracepoint. Without a `struct request *` (that needs the
// kprobe this kernel doesn't support), (dev, sector) is the next best
// natural key — a block device's sector number identifies a specific
// request while it's in flight. See ANALYSIS.md for when this
// undercounts (request merges).
struct start_key {
	__u32 dev;
	__u64 sector;
};

struct {
	__uint(type, BPF_MAP_TYPE_HASH);
	__uint(max_entries, 10240);
	__type(key, struct start_key);
	__type(value, __u64); // bpf_ktime_get_ns() at issue time
} starts SEC(".maps");

struct {
	__uint(type, BPF_MAP_TYPE_ARRAY);
	__uint(max_entries, MAX_SLOTS);
	__type(key, __u32);
	__type(value, __u64); // count of completions in this latency bucket
} hist SEC(".maps");

// log2(x), clamped to [0, MAX_SLOTS-1] — same bucketing BCC's histogram
// helper uses, reimplemented directly since this program doesn't link
// against BCC's Python-side helpers.
static __always_inline __u32 log2_bucket(__u64 v)
{
	__u32 bucket = 0;
#pragma unroll
	for (int i = 0; i < MAX_SLOTS - 1; i++) {
		if (v <= (1ULL << i))
			break;
		bucket++;
	}
	return bucket;
}

SEC("tracepoint/block/block_rq_issue")
int handle_rq_issue(struct trace_event_raw_block_rq *ctx)
{
	struct start_key key = {};
	__u64 ts = bpf_ktime_get_ns();

	key.dev = BPF_CORE_READ(ctx, dev);
	key.sector = BPF_CORE_READ(ctx, sector);

	bpf_map_update_elem(&starts, &key, &ts, BPF_ANY);
	return 0;
}

SEC("tracepoint/block/block_rq_complete")
int handle_rq_complete(struct trace_event_raw_block_rq_completion *ctx)
{
	struct start_key key = {};
	__u64 *start_ts, now, delta_us;
	__u32 slot;

	key.dev = BPF_CORE_READ(ctx, dev);
	key.sector = BPF_CORE_READ(ctx, sector);

	start_ts = bpf_map_lookup_elem(&starts, &key);
	if (!start_ts)
		return 0; // issue wasn't seen (e.g. program attached mid-flight)

	now = bpf_ktime_get_ns();
	delta_us = (now - *start_ts) / 1000;
	bpf_map_delete_elem(&starts, &key);

	slot = log2_bucket(delta_us);
	__u64 *count = bpf_map_lookup_elem(&hist, &slot);
	if (count)
		__sync_fetch_and_add(count, 1);

	return 0;
}
