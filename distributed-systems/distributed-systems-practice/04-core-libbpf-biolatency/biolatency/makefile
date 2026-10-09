# biolatency — CO-RE/libbpf rewrite of BCC's biolatency.
#
# Two very different compilers produce the two halves:
#   biolatency.bpf.o  — clang, -target bpf, runs IN the kernel
#   biolatency        — gcc, the host's native target, loads the above
#
# See ANALYSIS.md for why LIBELF_DIR/MULTIARCH_DIR point where they do.

LIBBPF_DIR  = ../toolchain/libbpf/src
LIBELF_DIR  = ../toolchain/libelf-shim
MULTIARCH_DIR = /usr/include/x86_64-linux-gnu

CLANG = clang
CC    = gcc

BPF_CFLAGS = -target bpf -D__TARGET_ARCH_x86 -I$(MULTIARCH_DIR) -g -O2 -Wall

# NO_PKG_CONFIG=1 was also used to build libbpf.a itself (see
# ANALYSIS.md) — same reasoning applies here: no libelf.pc exists for
# the hand-shimmed libelf, so pkg-config can't be trusted for it.
CFLAGS = -g -O2 -Wall -I$(LIBBPF_DIR) -I$(LIBELF_DIR)/include
LDFLAGS = -L$(LIBELF_DIR)/lib -Wl,-rpath,$(abspath $(LIBELF_DIR)/lib)
LIBS = $(LIBBPF_DIR)/libbpf.a -lelf -lz -lpthread

all: biolatency.bpf.o biolatency

biolatency.bpf.o: biolatency.bpf.c vmlinux_min.h bpf_helpers.h bpf_helper_defs.h bpf_core_read.h bpf_tracing.h bpf_endian.h
	$(CLANG) $(BPF_CFLAGS) -c biolatency.bpf.c -o $@

biolatency: biolatency.c $(LIBBPF_DIR)/libbpf.a
	$(CC) $(CFLAGS) biolatency.c -o $@ $(LIBS) $(LDFLAGS)

clean:
	rm -f biolatency.bpf.o biolatency

.PHONY: all clean
