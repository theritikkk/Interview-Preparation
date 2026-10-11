# xdp-lb — a toy L4 (TCP port-based) load balancer: xdp_lb.bpf.c at XDP
# ingress rewrites the destination port, tc_lb_return.bpf.c at TC egress
# rewrites it back on the way out, lb_ctl.c is the control plane, and
# backend.c is the (intentionally minimal) HTTP server the LB fronts.
#
# This reuses Project 4's already-built-and-verified libbpf + libelf
# shim rather than vendoring a second copy — see ANALYSIS.md for why
# that's a deliberate choice, not a missing file. If you have this zip
# without 04-core-libbpf alongside it, point TOOLCHAIN_DIR at any
# checkout of github.com/libbpf/libbpf (NO_PKG_CONFIG=1
# BUILD_STATIC_ONLY=1, same recipe) plus a real libelf-dev, or at
# 04-core-libbpf/toolchain/ from this project's own zip.

TOOLCHAIN_DIR = ../../04-core-libbpf/toolchain
LIBBPF_DIR    = $(TOOLCHAIN_DIR)/libbpf/src
LIBELF_DIR    = $(TOOLCHAIN_DIR)/libelf-shim
MULTIARCH_DIR = /usr/include/x86_64-linux-gnu

CLANG = clang
CC    = gcc

BPF_CFLAGS = -target bpf -D__TARGET_ARCH_x86 -I$(MULTIARCH_DIR) -g -O2 -Wall

CFLAGS  = -g -O2 -Wall -I$(LIBBPF_DIR) -I$(LIBELF_DIR)/include
LDFLAGS = -L$(LIBELF_DIR)/lib -Wl,-rpath,$(abspath $(LIBELF_DIR)/lib)
LIBS    = $(LIBBPF_DIR)/libbpf.a -lelf -lz -lpthread

BPF_OBJS = xdp_lb.bpf.o tc_lb_return.bpf.o

all: $(BPF_OBJS) lb_ctl backend

xdp_lb.bpf.o: xdp_lb.bpf.c bpf_helpers.h bpf_helper_defs.h bpf_endian.h
	$(CLANG) $(BPF_CFLAGS) -c $< -o $@

tc_lb_return.bpf.o: tc_lb_return.bpf.c bpf_helpers.h bpf_helper_defs.h bpf_endian.h
	$(CLANG) $(BPF_CFLAGS) -c $< -o $@

lb_ctl: lb_ctl.c $(LIBBPF_DIR)/libbpf.a
	$(CC) $(CFLAGS) $< -o $@ $(LIBS) $(LDFLAGS)

backend: backend.c
	$(CC) -g -O2 -Wall $< -o $@

clean:
	rm -f $(BPF_OBJS) lb_ctl backend

.PHONY: all clean
