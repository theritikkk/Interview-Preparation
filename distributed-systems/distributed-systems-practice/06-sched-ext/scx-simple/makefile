# scx-simple — a real sched_ext toy scheduler, built and compiled for
# real, and loaded against this sandbox's real live kernel.
#
# -mcpu=v3 is NOT optional here (unlike Projects 4/5's BPF_CFLAGS,
# which only needed -target bpf): UEI_RECORD's 32-bit atomic
# compare-and-swap on `kind` only lowers on BPF ISA v3+; the default
# clang bpf target crashes LLVM's instruction selector outright on
# it. See ANALYSIS.md for the exact crash and how this was found.
#
# Reuses Project 4's already-built libbpf + libelf shim, same as
# Project 5 — see that project's makefile comment for the fallback
# instructions if this zip is used standalone.

TOOLCHAIN_DIR = ../../04-core-libbpf/toolchain
LIBBPF_DIR    = $(TOOLCHAIN_DIR)/libbpf/src
LIBELF_DIR    = $(TOOLCHAIN_DIR)/libelf-shim
MULTIARCH_DIR = /usr/include/x86_64-linux-gnu

CLANG = clang
CC    = gcc

BPF_CFLAGS = -target bpf -mcpu=v3 -D__TARGET_ARCH_x86 -I$(MULTIARCH_DIR) -g -O2 -Wall
CFLAGS     = -g -O2 -Wall -I$(LIBBPF_DIR) -I$(LIBELF_DIR)/include
LDFLAGS    = -L$(LIBELF_DIR)/lib -Wl,-rpath,$(abspath $(LIBELF_DIR)/lib)
LIBS       = $(LIBBPF_DIR)/libbpf.a -lelf -lz -lpthread

all: scx_simple.bpf.o loader

scx_simple.bpf.o: scx_simple.bpf.c vmlinux_min.h scx_common_min.bpf.h user_exit_info_min.h \
                   bpf_helpers.h bpf_helper_defs.h bpf_tracing.h bpf_core_read.h
	$(CLANG) $(BPF_CFLAGS) -c $< -o $@

loader: loader.c user_exit_info_min.h $(LIBBPF_DIR)/libbpf.a
	$(CC) $(CFLAGS) loader.c -o $@ $(LIBS) $(LDFLAGS)

clean:
	rm -f scx_simple.bpf.o loader

.PHONY: all clean
