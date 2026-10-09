// raw_load_test.c — the smallest possible test of whether this sandbox
// can really load a BPF program into the live kernel, with zero
// dependency on libbpf (hand-encodes two instructions and calls the
// bpf() syscall directly). This is step zero before investing in the
// full CO-RE/libbpf toolchain: find out fast whether BPF_PROG_LOAD
// itself is permitted here at all.
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/bpf.h>

static long bpf_syscall(int cmd, union bpf_attr *attr, unsigned int size) {
    return syscall(__NR_bpf, cmd, attr, size);
}

int main(void) {
    // r0 = 0; exit;  -- the minimal valid eBPF program (a socket filter
    // that accepts 0 bytes of every packet — we never attach it to
    // anything, just try to load it).
    struct bpf_insn prog[] = {
        { .code = 0xb7 /* ALU64|MOV|K */, .dst_reg = 0, .src_reg = 0, .off = 0, .imm = 0 },
        { .code = 0x95 /* JMP|EXIT */,    .dst_reg = 0, .src_reg = 0, .off = 0, .imm = 0 },
    };

    char log_buf[4096] = {0};

    union bpf_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.prog_type = BPF_PROG_TYPE_SOCKET_FILTER;
    attr.insns = (uint64_t)(unsigned long)prog;
    attr.insn_cnt = 2;
    attr.license = (uint64_t)(unsigned long)"GPL";
    attr.log_buf = (uint64_t)(unsigned long)log_buf;
    attr.log_size = sizeof(log_buf);
    attr.log_level = 1;

    long fd = bpf_syscall(BPF_PROG_LOAD, &attr, sizeof(attr));
    if (fd < 0) {
        printf("BPF_PROG_LOAD FAILED: errno=%d (%s)\n", errno, strerror(errno));
        if (log_buf[0]) {
            printf("verifier log:\n%s\n", log_buf);
        }
        return 1;
    }

    printf("BPF_PROG_LOAD SUCCEEDED: got prog fd=%ld\n", fd);
    if (log_buf[0]) {
        printf("verifier log:\n%s\n", log_buf);
    }
    close(fd);
    return 0;
}
