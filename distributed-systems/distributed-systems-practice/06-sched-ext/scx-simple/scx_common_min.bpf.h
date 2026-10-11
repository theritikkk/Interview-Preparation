/* scx_common_min.bpf.h — a trimmed stand-in for the real upstream
 * scx/common.bpf.h, keeping only what scx_simple.bpf.c (kept
 * byte-for-byte identical to the real upstream file — see that
 * file's own header comment) actually uses.
 *
 * WHY TRIMMED, NOT COPIED WHOLESALE: the real common.bpf.h (see
 * ../reference/common.bpf.h.upstream, 427 lines at the v6.12 tag
 * this was taken from) exists to serve every scx scheduler in the
 * tree at once — cpumask kfuncs, bpf_list/bpf_rbtree kfuncs, cgroup
 * kfuncs, iterator kfuncs, MEMBER_VPTR/ARRAY_ELEM_PTR verifier
 * helpers — almost none of which scx_simple.bpf.c calls. Copying all
 * of it would mean hand-declaring dozens of kernel types
 * (struct cgroup, struct bpf_cpumask, struct bpf_rb_root, ...) in
 * vmlinux_min.h purely to satisfy signatures nothing here invokes —
 * exactly the "polished file, thin content" trap the opposite
 * direction of what CO-RE minimalism is for. Every kfunc declared
 * below IS one scx_simple.bpf.c actually calls; nothing is invented.
 *
 * The five kfunc declarations, the BPF_STRUCT_OPS[_SLEEPABLE] macros
 * and the SCX_OPS_DEFINE macro are copied verbatim (signatures and
 * all) from the real upstream file — only the surrounding kfuncs
 * this program doesn't use, and the vmlinux.h-dependent sanity check
 * (which needs a *generated* vmlinux.h to even compile), are cut.
 */
#ifndef __SCX_COMMON_MIN_BPF_H
#define __SCX_COMMON_MIN_BPF_H

#include "vmlinux_min.h"
#include "bpf_helpers.h"
#include "bpf_tracing.h"
#include "user_exit_info_min.h"

/* ---- the five scx_bpf_* kfuncs scx_simple.bpf.c calls (verbatim
 * signatures from scx/common.bpf.h) ---- */
s32 scx_bpf_create_dsq(u64 dsq_id, s32 node) __ksym;
s32 scx_bpf_select_cpu_dfl(struct task_struct *p, s32 prev_cpu, u64 wake_flags, bool *is_idle) __ksym;
void scx_bpf_dispatch(struct task_struct *p, u64 dsq_id, u64 slice, u64 enq_flags) __ksym;
void scx_bpf_dispatch_vtime(struct task_struct *p, u64 dsq_id, u64 slice, u64 vtime, u64 enq_flags) __ksym;
bool scx_bpf_consume(u64 dsq_id) __ksym;

/* ---- struct_ops program-definition macros (verbatim from
 * scx/common.bpf.h + scx/compat.bpf.h) ---- */
#define BPF_STRUCT_OPS(name, args...)		\
SEC("struct_ops/"#name)				\
BPF_PROG(name, ##args)

#define BPF_STRUCT_OPS_SLEEPABLE(name, args...)	\
SEC("struct_ops.s/"#name)				\
BPF_PROG(name, ##args)

#define SCX_OPS_DEFINE(__name, ...)			\
	SEC(".struct_ops.link")			\
	struct sched_ext_ops __name = {		\
		__VA_ARGS__,				\
	};

/* ---- the built-in DSQ ID scx_simple.bpf.c's select_cpu() uses
 * (SCX_DSQ_LOCAL, for scx_bpf_dispatch(p, SCX_DSQ_LOCAL, ...)) and
 * the default time slice — exact values copied from
 * include/linux/sched/ext.h's `enum scx_dsq_id_flags` / `enum
 * scx_slice`, verified by reading that header directly rather than
 * from memory (an earlier draft of this file guessed
 * SCX_DSQ_FLAG_BUILTIN | 0 for SCX_DSQ_LOCAL, which is actually
 * SCX_DSQ_INVALID — SCX_DSQ_LOCAL is really |2; caught by going back
 * to the source instead of trusting the first guess). ---- */
#define SCX_DSQ_FLAG_BUILTIN	(1ULL << 63)
#define SCX_DSQ_LOCAL		(SCX_DSQ_FLAG_BUILTIN | 2)

/* SCX_SLICE_DFL — default time slice, 20ms in ns. */
#define SCX_SLICE_DFL		(20ULL * 1000000)

#endif /* __SCX_COMMON_MIN_BPF_H */
