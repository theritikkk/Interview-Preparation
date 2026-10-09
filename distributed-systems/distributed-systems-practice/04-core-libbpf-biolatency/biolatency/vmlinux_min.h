/* vmlinux_min.h — a hand-written, minimal stand-in for a real
 * `bpftool btf dump file /sys/kernel/btf/vmlinux format c` output.
 *
 * WHY HAND-WRITTEN INSTEAD OF GENERATED: generating a real vmlinux.h
 * needs bpftool, which needs libbpf + libelf to build (see
 * ANALYSIS.md for the libelf story). Building bpftool itself was
 * judged not worth it for a program that touches exactly two kernel
 * struct fields — a full vmlinux.h for this kernel is tens of
 * thousands of lines for a program that needs about ten.
 *
 * WHY THIS STILL COUNTS AS REAL CO-RE, NOT A WORKAROUND: CO-RE
 * relocation does not care whether the local struct definition was
 * generated from BTF or typed by hand, and it does NOT require the
 * local struct to match the kernel's real layout, size, or field
 * order. At compile time, every access to a `preserve_access_index`
 * struct's field is turned into one of libbpf's special CO-RE
 * relocation records (stored in the .BTF.ext ELF section), tagged
 * with the struct's name and the field's name. At LOAD time, libbpf
 * looks up that exact (struct name, field name) pair in the target
 * kernel's *real* BTF (/sys/kernel/btf/vmlinux) and patches the
 * instruction's offset to match — the offsets declared here are
 * thrown away and never influence the loaded program. That's the
 * actual mechanism that makes a single compiled .bpf.o portable
 * across kernel versions with different struct layouts, and it works
 * identically whether the type came from a generated vmlinux.h or
 * was typed by a human, as long as the (name, field) pairs are
 * spelled correctly — which this file was verified against this
 * live kernel's own tracepoint format files
 * (/sys/kernel/tracing/events/block/block_rq_{issue,complete}/format)
 * to guarantee. See ANALYSIS.md for the full verification story.
 */
#ifndef VMLINUX_MIN_H
#define VMLINUX_MIN_H

/* ---- base integer typedefs + the BPF UAPI enums/flags ------------
 * A generated vmlinux.h carries its own copy of __u32, __be16,
 * enum bpf_map_type, BPF_ANY, etc. (the kernel's BTF includes the
 * UAPI types it's built from). Reaching for the system's own
 * <linux/types.h> and <linux/bpf.h> here instead of retyping them by
 * hand gets the exact same, correct, real values with no transcription
 * risk — these are the genuine kernel UAPI headers
 * (package linux-libc-dev, already present in this sandbox; nothing
 * about them needed libelf or network access). Only the two
 * kernel-internal tracepoint structs below — which never had a UAPI
 * header to begin with — are actually hand-declared.
 */
#include <linux/types.h>
#include <linux/bpf.h>

typedef __u8  u8;
typedef __u16 u16;
typedef __u32 u32;
typedef __u64 u64;
typedef __s8  s8;
typedef __s16 s16;
typedef __s32 s32;
typedef __s64 s64;

/* ---- the two tracepoint structs this program actually reads -----
 * Field names and types are copied verbatim from this kernel's own
 * /sys/kernel/tracing/events/block/block_rq_issue/format and
 * .../block_rq_complete/format (both tracepoints share the same
 * `dev`/`sector` field names, types and relative position — verified
 * directly, not assumed from a different kernel version or from
 * memory of an older block layer). Only the fields this program
 * reads are declared — see the comment above for why that's
 * sufficient for CO-RE, not a shortcut that risks correctness.
 *
 * STRUCT NAMES — the part that actually had to be debugged: a naive
 * guess (matching the tracepoint's own name) would be
 * `trace_event_raw_block_rq_issue` / `trace_event_raw_block_rq_complete`.
 * Those names do NOT exist in this kernel's BTF — loading a program
 * that references them fails CO-RE relocation
 * (`failed to resolve CO-RE relocation ... struct
 * trace_event_raw_block_rq_issue.dev`), confirmed by querying
 * /sys/kernel/btf/vmlinux directly with libbpf's own btf__parse()/
 * btf__find_by_name_kind() rather than guessing again.
 *
 * The real reason: the kernel's TRACE_EVENT macros generate one
 * `struct trace_event_raw_<X>` per event *class*, not per tracepoint.
 * include/trace/events/block.h defines block_rq_issue via
 * DECLARE_EVENT_CLASS(block_rq, ...) shared with block_rq_insert and
 * block_rq_merge, and block_rq_complete via
 * DECLARE_EVENT_CLASS(block_rq_completion, ...) shared with
 * block_rq_error — so the generated struct is named after the EVENT
 * CLASS (`block_rq`, `block_rq_completion`), not the individual
 * DEFINE_EVENT names used as the tracepoint/SEC() names. Confirmed by
 * dumping both structs from this kernel's live BTF: `dev` at byte
 * offset 8 and `sector` at byte offset 16 in both
 * `trace_event_raw_block_rq` and `trace_event_raw_block_rq_completion`
 * — exactly matching the tracefs format files above, just under a
 * different struct name than the tracepoint itself.
 *
 * preserve_access_index is what turns every `->dev`/`->sector` access
 * into a CO-RE relocation instead of a fixed-offset load — this is
 * the line that actually makes the program portable, not a formality.
 */
struct trace_event_raw_block_rq {
	__u32 dev;     /* format: "field:dev_t dev; ... size:4" */
	__u64 sector;  /* format: "field:sector_t sector; ... size:8" */
} __attribute__((preserve_access_index));

struct trace_event_raw_block_rq_completion {
	__u32 dev;
	__u64 sector;
} __attribute__((preserve_access_index));

#endif /* VMLINUX_MIN_H */
