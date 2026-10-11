/* vmlinux_min.h — a hand-written, minimal stand-in for a real
 * `bpftool btf dump file /sys/kernel/btf/vmlinux format c` output.
 *
 * Same methodology as Project 4's vmlinux_min.h (see that file's
 * comment for the full CO-RE-doesn't-care-about-local-layout
 * argument) — only the kernel-internal types this *one* program
 * actually touches are declared, by hand, instead of pulling in a
 * full generated vmlinux.h.
 *
 * THE DIFFERENCE FROM PROJECT 4: Project 4's hand-declared structs
 * (trace_event_raw_block_rq, ...) exist in this kernel's live BTF —
 * CO-RE found real offsets for them and the program ran. The structs
 * below do NOT exist in this kernel's live BTF, because this kernel
 * was built with `# CONFIG_SCHED_CLASS_EXT is not set` (see
 * ../ANALYSIS.md for the three independent ways that was confirmed).
 * `struct sched_ext_ops`, `struct sched_ext_entity`, and the
 * `task_struct.scx` field are all compiled out entirely when that
 * config is off — not hidden, not renamed, genuinely absent. So
 * unlike Project 4, correctly spelling these names cannot make this
 * load; the point here is to write a real, honestly-typed program
 * and let the real kernel reject it for its own real, specific
 * reason, which is the whole evidentiary point of this project.
 *
 * PROVENANCE — every field name/type/order below is copied, not
 * guessed, from the authentic upstream Linux source at tag v6.12
 * (the exact release sched_ext shipped in), fetched read-only into
 * this sandbox via a sparse git checkout of torvalds/linux:
 *   - struct sched_ext_ops   <- kernel/sched/ext.c (struct is defined
 *     inline in the .c file, not a header — verified by grep; see
 *     ../reference/kernel_ext.c.excerpt.upstream for the exact text
 *     this was copied from, with its real line numbers noted below)
 *   - struct scx_exit_info   <- kernel/sched/ext.c, same file
 *   - struct sched_ext_entity (the real name of task_struct's "scx"
 *     member's type) <- include/linux/sched/ext.h
 *   - the task_struct.scx field itself, and the fact that it's
 *     wrapped in `#ifdef CONFIG_SCHED_CLASS_EXT` <- include/linux/sched.h
 * Only the ops fields this toy scheduler actually initializes are
 * declared (9 of ~25 real ops fields) — struct_ops field resolution
 * in libbpf is by-name against the kernel's own BTF type, so a
 * local struct that's a strict subset of the real one is normal and
 * sufficient, exactly like Project 4 declaring 2 fields out of a
 * much larger real tracepoint struct.
 */
#ifndef VMLINUX_MIN_H
#define VMLINUX_MIN_H

#include <linux/types.h>
#include <linux/bpf.h>  /* enum bpf_map_type (BPF_MAP_TYPE_PERCPU_ARRAY, ...) —
                          * real UAPI header, same trick as Project 4's
                          * vmlinux_min.h: a generated vmlinux.h would carry
                          * its own copy of this enum anyway. */
#include <stdbool.h>

typedef __u8  u8;
typedef __u16 u16;
typedef __u32 u32;
typedef __u64 u64;
typedef __s8  s8;
typedef __s16 s16;
typedef __s32 s32;
typedef __s64 s64;

/* ---- struct sched_ext_entity (include/linux/sched/ext.h, the
 * fields are listed there in this exact order under a "BPF scheduler
 * modifiable fields" comment) ---- */
struct sched_ext_entity {
	u32 weight;
	u64 slice;
	u64 dsq_vtime;
} __attribute__((preserve_access_index));

/* ---- struct task_struct, trimmed to the one field this program
 * touches. Real struct task_struct is ~2500 lines; CO-RE lets a
 * caller declare just what it reads, same as Project 4. The real
 * field is behind `#ifdef CONFIG_SCHED_CLASS_EXT` in
 * include/linux/sched.h — on THIS kernel that #ifdef is false, so
 * the real task_struct in this kernel's live BTF has no `scx`
 * member at all; CO-RE relocation for p->scx.* is expected to fail
 * to resolve for that reason if the load ever gets that far. ---- */
struct task_struct {
	struct sched_ext_entity scx;
} __attribute__((preserve_access_index));

/* ---- struct scx_exit_info (kernel/sched/ext.c, ~line 70) — passed
 * to ops.exit() to describe why the BPF scheduler was unloaded.
 * `kind`'s real type is `enum scx_exit_kind`; declared as a plain
 * int here since only UEI_RECORD's bpf_core_field_exists()/plain
 * reads touch it and no arithmetic on specific enum values is done
 * in this program (the real values are reproduced in
 * scx_user_exit_info.h purely for documentation). ---- */
struct scx_exit_info {
	int   kind;
	s64   exit_code;
	char *reason;
	unsigned long *bt;
	u32   bt_len;
	char *msg;
	char *dump;
} __attribute__((preserve_access_index));

/* ---- struct sched_ext_ops (kernel/sched/ext.c, ~line 205-682) —
 * the struct_ops vtable itself. Field names/signatures below are
 * copied verbatim from the real struct; only the 9 fields this toy
 * scheduler's SCX_OPS_DEFINE(...) call actually sets are declared,
 * in their real relative order (select_cpu/enqueue/dispatch near the
 * top, running/stopping/enable in the per-task-transition group,
 * init/exit/name near the bottom, right before cgroup-only fields
 * this program doesn't use). No preserve_access_index here: this
 * struct is written to (it's the scheduler's own global variable,
 * placed in the ".struct_ops.link" ELF section), not CO-RE-read from
 * a kernel pointer, so ordinary BTF-from-DWARF is what libpbf needs,
 * not a relocation record. */
struct sched_ext_ops {
	s32  (*select_cpu)(struct task_struct *p, s32 prev_cpu, u64 wake_flags);
	void (*enqueue)(struct task_struct *p, u64 enq_flags);
	void (*dispatch)(s32 cpu, struct task_struct *prev);
	void (*running)(struct task_struct *p);
	void (*stopping)(struct task_struct *p, bool runnable);
	void (*enable)(struct task_struct *p);
	s32  (*init)(void);
	void (*exit)(struct scx_exit_info *info);
	u64  flags;
	char name[128]; /* SCX_OPS_NAME_LEN, include/linux/sched/ext.h */
};

#endif /* VMLINUX_MIN_H */
