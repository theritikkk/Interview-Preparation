/* user_exit_info_min.h — trimmed stand-in for the real upstream
 * scx/user_exit_info.h (see ../reference/user_exit_info.h.upstream).
 *
 * SIMPLIFICATION, stated plainly: the real header carries a resizable
 * "debug dump" buffer (UEI_DEFINE's RESIZABLE_ARRAY(data, ..._dump),
 * sized at runtime by the userspace loader from the real
 * sched_ext_ops.exit_dump_len field) for scx_bpf_dump()-style
 * scheduler-authored crash dumps. This toy scheduler never calls
 * scx_bpf_dump() and the dump buffer plays no role in whether the
 * program loads, so it's left out entirely rather than reproduced
 * for its own sake — the four fields kept below (kind, exit_code,
 * reason, msg) are exactly what's needed to report *why* the kernel
 * rejected the load, which is this project's actual payload.
 *
 * struct user_exit_info itself (unlike scx_exit_info in
 * vmlinux_min.h) is NOT a kernel type — it's the real upstream
 * project's own plain data struct, shared verbatim between the BPF
 * and userspace sides of a scheduler by #include'ing this one header
 * from both (see the #ifdef __bpf__ split below, copied from the
 * real file). No CO-RE involved here; it just needs to be laid out
 * identically on both sides, which plain shared-header inclusion
 * guarantees.
 */
#ifndef __USER_EXIT_INFO_MIN_H
#define __USER_EXIT_INFO_MIN_H

enum uei_sizes {
	UEI_REASON_LEN	= 128,
	UEI_MSG_LEN	= 1024,
};

struct user_exit_info {
	int	kind;
	long long exit_code;
	char	reason[UEI_REASON_LEN];
	char	msg[UEI_MSG_LEN];
};

#ifdef __bpf__

#include "vmlinux_min.h"
#include "bpf_core_read.h"

#define UEI_DEFINE(__name) \
	struct user_exit_info __name SEC(".data")

#define UEI_RECORD(__uei_name, __ei) ({					\
	bpf_probe_read_kernel_str(__uei_name.reason,				\
				  sizeof(__uei_name.reason), (__ei)->reason);	\
	bpf_probe_read_kernel_str(__uei_name.msg,				\
				  sizeof(__uei_name.msg), (__ei)->msg);	\
	if (bpf_core_field_exists((__ei)->exit_code))				\
		__uei_name.exit_code = (__ei)->exit_code;			\
	/* use __sync to force memory barrier, same as upstream */		\
	__sync_val_compare_and_swap(&__uei_name.kind, __uei_name.kind,	\
				    (__ei)->kind);				\
})

#else /* !__bpf__ */

#include <stdio.h>

#define UEI_EXITED(__uei_ptr) ({						\
	__sync_val_compare_and_swap(&(__uei_ptr)->kind, -1, -1);		\
})

#define UEI_REPORT(__uei_ptr) ({						\
	fprintf(stderr, "EXIT: %s", (__uei_ptr)->reason);			\
	if ((__uei_ptr)->msg[0] != '\0')					\
		fprintf(stderr, " (%s)", (__uei_ptr)->msg);			\
	fputs("\n", stderr);							\
	(__uei_ptr)->exit_code;						\
})

#endif /* __bpf__ */
#endif /* __USER_EXIT_INFO_MIN_H */
